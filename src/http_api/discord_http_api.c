
#include <assert.h>
#include <jansson.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ulfius.h>

#include "discord_http_api.h"

static const char endpoint_base_url[] = "https://discord.com/api/v10";
static const int endpoint_base_url_len = sizeof(endpoint_base_url) - 1;

typedef struct http_api_client {
  char *auth_token;
} http_api_client;

http_api_client *http_api_client_init()
{
  http_api_client *self = malloc(sizeof(http_api_client));
  const char *bot_token = getenv("BOT_TOKEN");

  size_t auth_token_len = strlen(bot_token) + sizeof "Bot ";
  self->auth_token = malloc(auth_token_len + 1);
  sprintf(self->auth_token, "Bot %s", bot_token);

  return self;
}

void http_api_client_free(http_api_client *client)
{
  free(client->auth_token);
  free(client);
}

struct _u_response *http_api_client_perform(http_api_client *self, const char *verb, json_t *body, const char *endpoint, ...)
{
  const size_t endpoint_length = 1024;
  char endpoint_url[endpoint_length];

  // prepare endpoint url
  {
    va_list args;
    va_start (args, endpoint);
    size_t res = vsnprintf(endpoint_url, endpoint_length, endpoint, args);
    assert(res < endpoint_length);
    va_end (args);
  }

  struct _u_request request = { 0 };
  ulfius_init_request(&request);

  ulfius_set_request_properties(&request,
                                U_OPT_HTTP_VERB, verb,
                                U_OPT_HTTP_URL, endpoint_base_url,
                                U_OPT_HTTP_URL_APPEND, endpoint_url,
                                U_OPT_NONE);

  if ( body )
    ulfius_set_json_body_request(&request, body);

  u_map_put(request.map_header, "Authorization", self->auth_token);
  u_map_put(request.map_header, "Content-Type", "application/json; charset=UTF-8");
  u_map_put(request.map_header, "User-Agent", "DiscordBot");

  struct _u_response *response = malloc(sizeof(struct _u_response));
  ulfius_init_response(response);

  int res = ulfius_send_http_request(&request, response);
  ulfius_clean_request(&request);

  if ( res != U_OK )
  {
    ulfius_clean_response_full(response);
    response = NULL;
  }

  return response;
}

