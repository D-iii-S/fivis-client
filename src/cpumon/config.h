/**
 * FIVIS configuration.
 *
 * Define FIVIS host, secret token for API access, partner identifier,
 * and a signal set identifier.
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_


#ifndef FIVIS_API_HOST
#  define FIVIS_API_HOST "https://api.fivis.smartarch.cz"
//#  error Please define FIVIS_API_HOST (e.g., in config.h)!
#endif

#ifndef FIVIS_API_TOKEN
//#  define FIVIS_API_TOKEN "put_fivis_api_token_here"
#  error Please define FIVIS_API_TOKEN (e.g., in config.h)!
#endif

#ifndef FIVIS_PARTNER_ID
//#  define FIVIS_PARTNER_ID "put_fivis_partner_id_here"
#  error Please define FIVIS_PARTNER_ID (e.g., in config.h)!
#endif

#ifndef FIVIS_SIGNAL_SET_ID
//#  define FIVIS_SIGNAL_SET_ID "put_signal_set_id_here"
#  error Please define FIVIS_SIGNAL_SET_ID (e.g., in config.h)!
#endif


#endif /* _CONFIG_H_ */
