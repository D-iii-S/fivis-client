#include <stdlib.h>

#include <fivis/debug.h>
#include <fivis/sbuf.h>
#include <fivis/list.h>
#include <fivis/fivis.h>
#include <fivis/entry.h>

#include "config.h"

//

static const char * fivis_api_host = FIVIS_API_HOST;
static const char * fivis_api_token = FIVIS_API_TOKEN;

static const char * fivis_partner_id = FIVIS_PARTNER_ID;
static const char * fivis_signal_set_id = FIVIS_SIGNAL_SET_ID;

//

static bool
make_request(struct fivis * fivis) {
	struct entry id_signal = entry_string("id");

	struct entry ts_signal = entry_datetime("ts");
	struct entry sig1_signal = entry_signed("sig1");
	struct entry sig2_signal = entry_double("sig2");
	struct entry sig3_signal = entry_boolean("sig3");

	struct list schema = LIST_INIT(schema);
	list_add_last(&schema, &ts_signal.link);
	list_add_last(&schema, &sig1_signal.link);
	list_add_last(&schema, &sig2_signal.link);
	list_add_last(&schema, &sig3_signal.link);

	//

	struct sbuf request = SBUF_INIT();
	if (!fivis_signals_format_request(fivis_partner_id, fivis_signal_set_id, &schema, &id_signal, &schema, NULL, NULL, &request)) {
		error("failed to format FIVIS signals request\n");
		return false;
	}

	fivis_result_t fivis_result = fivis_signals_perform_request(
		fivis, sbuf_string(&request), sbuf_length(&request)
	);

	if (fivis_result != FIVIS_OK) {
		error("fivis: %s\n", fivis_last_error());
		error("failed to perform FIVIS signals request\n");
		return false;
	}

	sbuf_destroy(&request);
	return true;
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


int main(int argc, char ** argv) {
	checked_global_init();

	//

	struct fivis * fivis = fivis_init(fivis_api_host, fivis_api_token);
	if (fivis == NULL) {
		error("fivis: %s\n", fivis_last_error());
		error("failed to initialize FIVIS context\n");
		exit(EXIT_FAILURE);
	}

	bool request_succeeded = make_request(fivis);

	fivis_cleanup(fivis);

	//

	exit(request_succeeded ? EXIT_SUCCESS : EXIT_FAILURE);
}
