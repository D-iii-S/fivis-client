#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <time.h>

#include <common/error.h>
#include <common/checked.h>

#include <fivis/entry.h>
#include <fivis/fivis.h>
#include <fivis/list.h>
#include <fivis/debug.h>
#include <fivis/util.h>

#include "config.h"
#include "procfile.h"

//

static const char * fivis_api_host = FIVIS_API_HOST;
static const char * fivis_api_token = FIVIS_API_TOKEN;

static const char * fivis_partner_id = FIVIS_PARTNER_ID;
static const char * fivis_signal_set_id = FIVIS_SIGNAL_SET_ID;

//

static const int cpumon_sample_period_secs = 12;
static const int cpumon_sample_count = 3600 / cpumon_sample_period_secs;

static const int cpumon_dump_period_secs = 60;
static const int cpumon_dump_retry_secs = 20;
static const int cpumon_dump_check_secs = 5;
static const int cpumon_dump_empty_min = cpumon_sample_count / 10;

//

static void
free_strings(char ** strings, size_t count) {
	for (size_t i = 0; i < count; i++) {
		if (strings[i] != NULL) {
			free(strings[i]);
			strings[i] = NULL;
		}
	}

	free(strings);
}


static char **
collect_strings(size_t count, char * (* supplier) (size_t)) {
	char ** result = (char **) calloc(count, sizeof(char *));
	if (result == NULL) {
		debug("main: failed to allocate array of %zu string pointers\n", count);
		goto fail_result;
	}

	//

	for (size_t i = 0; i < count; i++) {
		char * str = supplier(i);
		if (str == NULL) {
			debug("main: string supplier returned NULL for index %zu\n", i);
			goto fail_string;
		}

		result[i] = str;
	}

	return result;

	//

fail_string:
	free_strings(result, count);
fail_result:
	return NULL;
}


static ssize_t
proc_stat_get_cpu_count(const char * restrict buffer) {
	ssize_t result = 0;

	const char * current = buffer;
	while (true) {
		// Check prefix.
		if (strspn(current, "cpu") != strlen("cpu")) {
			return result;
		}

		// Find end of line.
		current = strchr(current, '\n');
		if (current == NULL) {
			return result;
		}

		current++;
		result++;
	}
}


static ssize_t
proc_stat_get_time_count(const char * restrict buffer) {
	ssize_t result = 0;

	static const char * prefix = "cpu ";
	int length = strlen(prefix);

	const char * current = strstr(buffer, prefix);
	if (current != NULL)  {
		current += length;

		uint64_t dummy;
		while (sscanf(current, "%" SCNu64 "%n", &dummy, &length) == 1) {
			current += length;
			result++;
		}
	}

	return result;
}


static ssize_t
proc_stat_parse_times(
	const char * restrict buffer, size_t count, union entry_value * values
) {
	static const char * prefix = "cpu";
	ssize_t result = 0;

	const char * current = buffer;
	while (true) {
		// Validate 'cpu ' or 'cpu* ' line prefix.
		if (strstr(current, prefix) == NULL) {
			// Neither prefix found, stop parsing.
			return result;
		}

		// Make sure the next character is space.
		current = strchr(current, ' ');
		if (current == NULL) {
			return result;
		}

		// Parse the columns
		uint64_t value;
		int length;
		while (sscanf(current, "%" SCNu64 "%n", &value, &length) == 1) {
			current += length;

			if (result >= count) {
				// Value count reached, stop parsing.
				return result;
			}

			values->as_unsigned = value;
			values++;

			result++;
		}

		sscanf(current, "\n%n", &length);
		current += length;
	}
}


static char *
supply_cpu_name(size_t index) {
	// Index 0 corresponds to the CPU with summary times accross all CPUS.
	return (index == 0) ? strdup("cpu") : format_string("cpu%zd", index - 1);
}


static char *
supply_time_name(size_t index) {
	static const char * time_names[] = {
		"user", "nice", "system", "idle",
		"iowait", "irq", "softirq",
		"steal", "guest", "guest_nice",
	};

	static const size_t known_count = sizeof_array(time_names);

	// Create names for unknown time names.
	return (index < known_count) ? strdup(time_names[index]) : format_string("time%zd", index);
}


static void
free_signals(struct list * signals) {
	while (! list_is_empty(signals)) {
		struct list * item = list_remove_after(signals);
		struct entry * entry = list_item_var(item, entry, link);
		entry_destroy(entry);
	}
}


static void
checked_create_time_signals(ssize_t cpu_count, ssize_t time_count, struct list * signals) {
	assert(signals != NULL);

	// Generate CPU and time names.
	char ** cpu_names = collect_strings(cpu_count, supply_cpu_name);
	check_error(cpu_names == NULL, "failed to generate CPU names");

	char ** time_names = collect_strings(time_count, supply_time_name);
	check_error(time_names == NULL, "failed to generate time names");

	// Create 2D array of signals with combined names.
	struct entry (* time_signals)[time_count] = checked_malloc(sizeof(struct entry [cpu_count][time_count]));

	for (size_t cpu_index = 0; cpu_index < cpu_count; cpu_index++) {
		for (size_t time_index = 0; time_index < time_count; time_index++) {
			char * cpu_name = cpu_names[cpu_index];
			char * time_name = time_names[time_index];

			char * name = format_string("%s_%s", cpu_name, time_name);
			check_error(name == NULL, "failed to create signal name: %s_%s\n", cpu_name, time_name);

			struct entry * signal = &time_signals[cpu_index][time_index];
			entry_init_double(signal, name);

			list_add_last(signals, &signal->link);
		}
	}

	// Free CPU and time names (they are not needed anymore).
	free_strings(time_names, time_count);
	time_names = NULL;

	free_strings(cpu_names, cpu_count);
	cpu_names = NULL;
}


static time_t
nanosleep_secs(time_t secs) {
	struct timespec req_tm = { .tv_sec = secs, .tv_nsec = 0 };
	struct timespec rem_tm = { .tv_sec = 0, .tv_nsec = 0 };

	// Sleep without interrupt.
	while (nanosleep(&req_tm, &rem_tm) != 0) {
		req_tm = rem_tm;
	}

	return secs;
}


static int
compare_timespec(struct timespec * ts1, struct timespec * ts2) {
	if (ts1->tv_sec < ts2->tv_sec) {
		return -1;
	} else if (ts1->tv_sec > ts2->tv_sec) {
		return 1;
	} else if (ts1->tv_nsec < ts2->tv_nsec) {
		return -1;
	} else if (ts1->tv_nsec > ts2->tv_nsec) {
		return 1;
	} else {
		// Equal values.
		return 0;
	}
}


struct sample {
	struct list link;
	union entry_value id_value;
	union entry_value ts_value;
	union entry_value time_values[];
};


static void
sample_copy_from(struct sample * dst, size_t time_count, struct sample * src) {
	size_t size = time_count * sizeof(union entry_value);
	memcpy(&dst->time_values[0], &src->time_values[0], size);
}

static void
sample_diff_unsigned(struct sample * dst, size_t time_count, struct sample * src) {
	for (size_t time_index = 0; time_index < time_count; time_index++) {
		dst->time_values[time_index].as_unsigned -= src->time_values[time_index].as_unsigned;
	}
}

static void
sample_zero_times(struct sample * sample, size_t time_count) {
	size_t size = time_count * sizeof(union entry_value);
	memset(&sample->time_values[0], 0, size);
}


struct cpumon_args {
	volatile bool cpumon_stop;
	struct procfile * proc_stat;
	size_t time_values_count;

	struct list empty_samples;
	struct list full_samples;

	pthread_mutex_t samples_mutex;
	pthread_cond_t empty_samples_cond;

	struct sample * last_sample[2];
};


static void *
cpumon_main(struct cpumon_args * args) {
	assert(args != NULL);
	debug("cpumon: thread started\n");

	// Last timestamp to avoid time jumping backwards.
	struct timespec last_ts = { .tv_sec = 0, .tv_nsec = 0 };

	// Last sample to subtract from next sample.
	size_t last_index = 0;
	struct sample * last_sample = args->last_sample[last_index];
	sample_zero_times(last_sample, args->time_values_count);

	// Current sample to fill.
	struct sample * sample = NULL;

	while (!args->cpumon_stop) {
		//
		// Sleep for the sample period and get a timestamp. If the timestamp
		// is greater than the last timestamp, snapshot the /proc/stat file
		// and update the last-seen timestamp. Retry on any failures.
		//
		nanosleep_secs(cpumon_sample_period_secs);

		struct timespec ts;
		int ts_result = clock_gettime(CLOCK_REALTIME, &ts);
		if (ts_result != 0) {
			debug("cpumon: failed to get time, retrying\n");
			continue;
		}

		if (compare_timespec(&last_ts, &ts) >= 0) {
			debug("cpumon: current time less than previous, retrying\n");
			continue;
		}

		if (procfile_read_fully(args->proc_stat) <= 0) {
			debug("cpumon: failed to snapshot /proc/stat, retrying\n");
			continue;
		}

		last_ts = ts;


		//
		// Lock the mutex and get empty sample for signal values. This may
		// involve waiting on a condition variable for the main thread to
		// return empty sample structures.
		//
		if (sample == NULL) {
			checked_mutex_lock(&args->samples_mutex);

			while (list_is_empty(&args->empty_samples)) {
				debug("cpumon: no empty samples available, waiting\n");

				checked_cond_wait(&args->empty_samples_cond, &args->samples_mutex);
				if (args->cpumon_stop) {
					// Unlock mutex before terminating.
					checked_mutex_unlock(&args->samples_mutex);
					goto terminate;
				}
			}

			struct list * item = list_remove(args->empty_samples.next);

			checked_mutex_unlock(&args->samples_mutex);

			sample = list_item_var(item, sample, link);
			debug("cpumon: acquired empty sample\n");
		}

		//
		// Fill in the sample values. This means setting the 'id' and 'ts'
		// signal values and then parsing the contents of the /proc/stat
		// file to get all time values for all CPUs. If we parsed less
		// values than expected, retry everything.
		//
		sample->id_value.as_timespec = ts;
		sample->ts_value.as_timespec = ts;

		size_t expected_count = args->time_values_count;
		size_t values_read = proc_stat_parse_times(
			procfile_string(args->proc_stat), expected_count, &sample->time_values[0]
		);

		debug("cpumon: parsed %zu time values\n", values_read);
		if (values_read != expected_count) {
			debug("cpumon: expected %zu values, retrying\n", expected_count);
			continue;
		}

		//
		// Take a snapshot of the sample and subtract the previous sample
		// from it to get CPU usage for the last time interval. The saved
		// sample snapshot then becomes the last sample for next round.
		//
		last_index = (last_index + 1) % sizeof_array(args->last_sample);
		struct sample * snap_sample = args->last_sample[last_index];
		sample_copy_from(snap_sample, args->time_values_count, sample);
		sample_diff_unsigned(sample, args->time_values_count, last_sample);

		last_sample = snap_sample;

		//
		// Put the sample in the list of full samples for the main thread
		// to consume. Lock the sample mutex to make sure the main thread
		// is not accessing it at the same time.
		//
		debug("cpumon: produced full sample\n");

		checked_mutex_lock(&args->samples_mutex);
		list_add_last(&args->full_samples, &sample->link);
		checked_mutex_unlock(&args->samples_mutex);

		sample = NULL;
	}

terminate:
	debug("cpumon: thread finished\n");
	return (void *) EXIT_SUCCESS;
}



static union entry_value *
convert_times_to_percentages(size_t cpu_count, size_t time_count, union entry_value * first) {
	// Interpret values as a rectangular array.
	union entry_value (* values)[time_count] = (union entry_value (*)[time_count]) first;

	// Sum rows and convert values to fractions of the sum.
	for (int cpu = 0; cpu < cpu_count; cpu++) {
		uint64_t sum = 0;
		for (int time = 0; time < time_count; time++) {
			sum += values[cpu][time].as_unsigned;
		}

		for (int time = 0; time < time_count; time++) {
			double fraction = (double) values[cpu][time].as_unsigned / sum;
			values[cpu][time].as_double = fraction * 100;
		}
	}

	return first;
}


static pthread_t
checked_start_cpumon(struct cpumon_args * args) {
	pthread_t result;
	void * (* start) (void * ) = (void * (*) (void *)) cpumon_main;
	int thread_result = pthread_create(&result, NULL, start, args);
	check_std_error(thread_result != 0, "failed to create cpumon thread");
	return result;
}


static void
checked_global_init() {
	if (!fivis_global_init()) {
		error("fivis: %s\n", fivis_last_error());
		error("failed to initialize FIVIS module\n");
		exit(EXIT_FAILURE);
	}

	if (atexit(fivis_global_cleanup) != 0) {
		error("failed to register cleanup handler\n");
		exit(EXIT_FAILURE);
	}
}



struct next_value_state {
	struct list samples;
	struct sample * sample;
	union entry_value * next_value;
	size_t next_index;
	size_t value_count;
};


static union entry_value *
cpumon_next_value(void * arg) {
	struct next_value_state * state = (struct next_value_state *) arg;

	// First-time initialization.
	if (state->sample == NULL) {
		if (list_is_empty(&state->samples)) {
			return NULL;
		}

		// Get the first sample and prepare 'id' signal value as the next.
		state->sample = list_item_var(state->samples.next, state->sample, link);
		state->next_value = &state->sample->id_value;
	}

	assert(state->sample != NULL);

	// Serve current 'next_value' and prepare the next 'next_value'.
	union entry_value * value = state->next_value;
	if (value == &state->sample->id_value) {
		// We are serving 'id' signal value, prepare 'ts' as next.
		state->next_value = &state->sample->ts_value;
		state->next_index = 0;

	} else if (state->next_index < state->value_count) {
		// We are serving 'ts' or a CPU time value, prepare next CPU time.
		state->next_value = &state->sample->time_values[state->next_index];
		state->next_index++;

	} else if (state->sample->link.next != &state->samples) {
		// We are serving the last CPU time value. Move to next sample and
		// prepare its 'id' signal value as the next.
		state->sample = list_item_var(state->sample->link.next, state->sample, link);
		state->next_value = &state->sample->id_value;

	} else if (value != NULL) {
		// We are serving the last value. There will be no more.
		state->next_value = NULL;
	}

	return value;
}


const char *
id_format_datetime_value(
	const char * restrict name, union entry_value * value, struct sbuf * buffer
) {
	return sbuf_format(buffer, "\"%s\": \"%011d\"", name, value->as_timespec.tv_sec);
}



int
main(int argc, char * argv[]) {
	checked_global_init();

	//

	struct fivis * fivis = fivis_init(FIVIS_API_HOST, FIVIS_API_TOKEN);
	if (fivis == NULL) {
		error("fivis: %s\n", fivis_last_error());
		error("failed to initialize FIVIS context\n");
		exit(EXIT_FAILURE);
	}

	struct procfile * proc_stat = procfile_open("/proc/stat");
	if (proc_stat == NULL) {
		error("failed to open /proc/stat\n");
		exit(EXIT_FAILURE);
	}

	ssize_t bytes_read = procfile_read_fully(proc_stat);
	if (bytes_read < 0) {
		error("failed to read %s\n", procfile_path(proc_stat));
		exit(EXIT_FAILURE);
	}

	//

	// Record id signal. Not linked to other signals.
	struct entry id_signal = entry_generic(
		checked_strdup("id"), id_format_datetime_value, entry_format_string_type
	);

	// Timestamp signal, part of the schema.
	struct entry ts_signal = entry_datetime(checked_strdup("ts"));

	struct list signals = LIST_INIT(signals);
	list_add_first(&signals, &ts_signal.link);

	// Add per-CPU time signals, including summary across all CPUs.
	int cpu_count = proc_stat_get_cpu_count(procfile_string(proc_stat));
	int time_count = proc_stat_get_time_count(procfile_string(proc_stat));
	checked_create_time_signals(cpu_count, time_count, &signals);

	//
	// Start the CPU usage monitoring thread and periodically
	// flush the samples collected by the thread.
	//

	// Number of values and sample size: id, ts, cpu_*, cpuX_*
	size_t value_count = cpu_count * time_count;
	size_t sample_size = sizeof(struct sample) + value_count * sizeof(union entry_value);

	// Prepare data for sampler thread.
	struct cpumon_args cpumon_args = {
		.cpumon_stop = false,
		.proc_stat = proc_stat,
		.time_values_count = value_count,
		.empty_samples = LIST_INIT(cpumon_args.empty_samples),
		.full_samples = LIST_INIT(cpumon_args.full_samples),
		.samples_mutex = PTHREAD_MUTEX_INITIALIZER,
		.empty_samples_cond = PTHREAD_COND_INITIALIZER,
		.last_sample = {
			(struct sample *) checked_malloc(sample_size),
			(struct sample *) checked_malloc(sample_size)
		}
	};

	// Allocate predefined number of data chunks.
	for (int i = 0; i < cpumon_sample_count; i++) {
		struct sample * sample = (struct sample *) checked_malloc(sample_size);
		list_add_last(&cpumon_args.empty_samples, &sample->link);
	}


	pthread_t cpumon_thread = checked_start_cpumon(&cpumon_args);

	//

	struct sbuf request = SBUF_INIT();
	struct list * schema = &signals;

	struct timespec next_dump;
	clock_gettime(CLOCK_REALTIME, &next_dump);
	next_dump.tv_sec += cpumon_dump_period_secs;


	while (true) {
		// Sleep until given time and schedule next sleep from the wakup.
		clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &next_dump, NULL);

		clock_gettime(CLOCK_REALTIME, &next_dump);
		next_dump.tv_sec += cpumon_dump_period_secs;

		// Lock the sample list mutex.
		checked_mutex_lock(&cpumon_args.samples_mutex);

		// Grab full samples.
		if (! list_is_empty(&cpumon_args.full_samples)) {
			struct next_value_state next_value_state = {
				.samples = LIST_INIT(next_value_state.samples),
				.value_count = value_count,
			};

			while (! list_is_empty(&cpumon_args.full_samples)) {
				struct list * item = list_remove_after(&cpumon_args.full_samples);
				list_add_last(&next_value_state.samples, item);
			}

			// Unlock the mutex for processing.
			checked_mutex_unlock(&cpumon_args.samples_mutex);

			//
			// Convert the CPU time samples to percentages, format the
			// request and send it to the server. The first request will
			// include a schema part, the subsequent requests will not.
			//
			// When sending the request, keep trying for some time, but
			// stop when we run out of empty samples.
			//
			struct sample * sample;
			list_for_each_item(sample, &next_value_state.samples, link) {
				convert_times_to_percentages(cpu_count, time_count, &sample->time_values[0]);
			}

			sbuf_clear(&request);
			const char * request_string = fivis_signals_format_request(
				FIVIS_PARTNER_ID, FIVIS_SIGNAL_SET_ID, schema,
				&id_signal, &signals, cpumon_next_value, &next_value_state,
				&request
			);

			if (request_string == NULL) {
				error("failed to format FIVIS signals request\n");
				break;
			}

			debug(request_string);

			//

			bool request_done = false;
			while (!request_done) {
				debug("main: performing FIVIS request\n");
				fivis_result_t fivis_result = fivis_signals_perform_request(
					fivis, sbuf_string(&request), sbuf_length(&request)
				);

				if (fivis_result == FIVIS_OK) {
					debug("main: FIVIS request succeeded\n");
					schema = NULL;
					break;
				}

				warn("FIVIS request failed, retry in %d seconds\n", cpumon_dump_retry_secs);
				int retry_delay = cpumon_dump_retry_secs;
				while (retry_delay > 0) {
					retry_delay -= nanosleep_secs(cpumon_dump_check_secs);

					// Check the empty list size.
					checked_mutex_lock(&cpumon_args.samples_mutex);
					int empty_count = list_size(&cpumon_args.empty_samples);
					checked_mutex_unlock(&cpumon_args.samples_mutex);

					debug("main: empty samples available: %d\n", empty_count);
					if (empty_count <= cpumon_dump_empty_min) {
						warn("number of empty samples too low, request dropped\n");
						request_done = true;
						break;
					}
				}
			}


			//
			// Return the processed samples to the list of empty samples
			// and signalize the availablility of new empty samples. This
			// may wake up the 'cpumon' thread if it was sleeping.
			//
			checked_mutex_lock(&cpumon_args.samples_mutex);

			while (! list_is_empty(&next_value_state.samples)) {
				struct list * item = list_remove_after(&next_value_state.samples);
				list_add_last(&cpumon_args.empty_samples, item);
			}

			checked_cond_signal(&cpumon_args.empty_samples_cond);
		}

		// Unlock the mutex for the 'cpumon' thread.
		checked_mutex_unlock(&cpumon_args.samples_mutex);
	}


	//
	// Request the 'cpumon' thread to stop. Signal the empty samples
	// condition variable in case it was waiting for empty samples.
	//
	cpumon_args.cpumon_stop = true;
	checked_cond_signal(&cpumon_args.empty_samples_cond);
	checked_thread_join(cpumon_thread);

	sbuf_destroy(&request);
	free_signals(&signals);
	procfile_close(proc_stat);
	fivis_cleanup(fivis);

	//

	exit(EXIT_SUCCESS);
}
