# fivis-client

A simple library for pushing data into FIVIS from a C client.

The FIVIS provides a REST endpoint which accepts JSON payload with
signal data. The library uses  [libcurl](https://curl.haxx.se/libcurl/)
to perform HTTPS requests on the API end point.


# Structure

The provided FIVIS API is simple, and to make clear separation between
what is needed by the FIVIS API and the example projects, the source
code in the `src` directory has been split into multiple subprojects.

- `src/fivis` contains the files needed for the FIVIS API.

- `src/common` contains the files needed by both examples.

- `src/mksigset` is a trivial example using the FIVIS API to create an
  empty signal set with several signals of different types.

- `src/cpumon` is a more complex example implementing a simple CPU monitor
  which uses the FIVIS API to push data into FIVIS. The `cpumon` periodically
  collects CPU usage data and sends batch updates to the FIVIS server. It
  keeps trying to send data when it encounters transient (network) errors.


# Dependencies

The project requires the [libcurl] (https://curl.haxx.se/libcurl/)
library and whatever dependencies `libcurl` requires. On package-based
Linux distributions, this typically requires installing a package such
as `libcurl-devel` (Fedora) or similar.

The project expects to find `libcurl` include files and libraries in
default system directories, so that including `curl/curl.h` and linking
with `-lcurl` works out of the box.

If necessary, modify the include directories in the Makefiles.

Regarding CURL features, the project requires the CURL library to 
only support the 'SSL', 'libz', and 'HTTPS-Proxy' features.


# Configuration

The examples in `src/mksigset` and `src/cpumon` need to be configured to
enable access to partner-specific FIVIS API.

Consequently, each of the projects contains `config.h`, which needs to
define the values of `FIVIS_API_HOST`, `FIVIS_API_TOKEN`, `FIVIS_PARTNER_ID`,
and `FIVIS_SIGNAL_SET_ID`. Make sure to update `config.h` in each example
that you want to try.

Alternatively, you can define these macros using compiler flags.


# Compilation

Simply run `make` in this directory. This should produce two executable
files in the `build` directory: `mksigset` and `cpumon`.

The FIVIS library will be in `src/fivis/build`, in both static (`fivis.a`)
and shared (`libfivis.so`) form.


# FIVIS client API

The FIVIS client API can be found in the `src/fivis` directory. The API
itself is relatively small (see `fivis.h` and `fivis.c`), but it does make
use of other modules: it needs support for formatting strings into a growable
buffer in `sbuf.c` and `sbuf.h`, makes use of a linked list from `list.h`, and 
uses type-specific entry formatters from `entry.h` and `entry.c`.

The API is currently represented by the following functions:

- `fivis_global_init`, which must be called once per program execution. It has
  a corresponding clean-up function, `fivis_global_cleanup`, which should be
  called before program exit to release resources.
  
- `fivis_init` creates an instance of FIVIS context for a given API endpoint
  and API token. The corresponding clean-up function is `fivis_cleanup`.

- `fivis_signals_format_request` is one of the two main functions. This one
   formats the request to be sent to the FIVIS signals API. It requires 
   partner and signal set identifiers. The `schema` attribute of the request
   is optional if the signal set definition remains unchanged. The function
   accepts schema as a list of `struct entry` instances, if `NULL` is passed
   as scheme, the `schema` attribute is not generated in the request.

   For the `data` attribute, the function requires a separate signal representing
   the implicit `id` attribute and a list of `struct entry` instances representing
   the other signals in the signal set. When this function generates data records,
   it repeatedly iterates over the list of signals for as long as the `next_value`
   functions returns non-NULL values. Whenever the end of the `signals` list is
   reached, new data record is created.

   The `next_value` function together with the `next_value_state` parameter 
   represent a simple iterator so that the request formatting function can
   be provide with data without depending on particular data structure. The
   caller is required to provide the `next_value` function and its state 
   objects in `next_value_state`, which is passed to the function on each
   invocation. When the `next_value` fuction returns `NULL`, the iteraiton ends.

- `fivis_signals_perform_request` is the second of the two main functions. This
   one sends a formatted request to the FIVIS signals API endpoint.

- `fivis_last_error` provides a string representing the last error encountered
  during execution of the functions from the FIVIS module. The caller MUST NOT
  free the memory occupied by the returned string.
