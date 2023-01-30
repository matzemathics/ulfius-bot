
struct _u_response;
typedef struct json_t json_t;
typedef struct http_api_client http_api_client;

http_api_client *http_api_client_init();
void http_api_client_free(http_api_client *client);

#define HTTP_STATUS_IS_OK(status) ((status) / 100 == 2)

struct _u_response *http_api_client_perform(http_api_client *self, const char *verb, json_t *data, const char *endpoint, ...);

