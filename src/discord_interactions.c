#include "discord_interactions.h"

#include <string.h>
#include <ulfius.h>
#include <stdlib.h>

typedef enum interaction_type
{
  /**
   * A ping.
   */
  PING = 1,
  /**
   * A command invocation.
   */
  APPLICATION_COMMAND = 2,
  /**
   * Usage of a message's component.
   */
  MESSAGE_COMPONENT = 3,
  /**
   * An interaction sent when an application command option is filled out.
   */
  APPLICATION_COMMAND_AUTOCOMPLETE = 4,
  /**
   * An interaction sent when a modal is submitted.
   */
  MODAL_SUBMIT = 5,
} interaction_type;

typedef enum interaction_response_type
{
  /**
   * Acknowledge a `PING`.
   */
  PONG = 1,
  /**
   * Respond with a message, showing the user's input.
   */
  CHANNEL_MESSAGE_WITH_SOURCE = 4,
  /**
   * Acknowledge a command without sending a message, showing the user's input. Requires follow-up.
   */
  DEFERRED_CHANNEL_MESSAGE_WITH_SOURCE = 5,
  /**
   * Acknowledge an interaction and edit the original message that contains the component later; the user does not see a loading state.
   */
  DEFERRED_UPDATE_MESSAGE = 6,
  /**
   * Edit the message the component was attached to.
   */
  UPDATE_MESSAGE = 7,
  /*
   * Callback for an app to define the results to the user.
   */
  APPLICATION_COMMAND_AUTOCOMPLETE_RESULT = 8,
  /*
   * Respond with a modal.
   */
  MODAL = 9,
} interaction_response_type;

int ping_handler(struct _u_response *response)
{
  json_t *res = json_object();
  json_object_set_new(res, "type", json_integer(PONG));
  ulfius_set_json_body_response(response, 200, res);
  return U_CALLBACK_COMPLETE;
}

int command_handler(json_t *id, json_t *data, struct _u_response *response)
{
  if (!json_is_object(data))
  {
    ulfius_set_empty_body_response(response, 400);
    return U_CALLBACK_CONTINUE;
  }

  json_t *name = json_object_get(data, "name");

  if (!json_is_string(name))
  {
    ulfius_set_empty_body_response(response, 400);
    return U_CALLBACK_CONTINUE;
  }

  if ( !strcmp("test", json_string_value(name)) )
  {
    json_t *msg = json_object();
    json_object_set_new(msg, "content", json_string("hello world"));

    json_t *res = json_object();
    json_object_set_new(res, "type", json_integer(CHANNEL_MESSAGE_WITH_SOURCE));
    json_object_set_new(res, "data", msg);

    ulfius_set_json_body_response(response, 200, res);
    return U_CALLBACK_COMPLETE;
  }

  return U_CALLBACK_CONTINUE;
}

int interaction_handler_callback(
    const struct _u_request *request, 
    struct _u_response *response, 
    void *user_data)
{
  json_t *body = ulfius_get_json_body_request(request, NULL);

  if (!json_is_object(body))
  {
    ulfius_set_empty_body_response(response, 400);
    return U_CALLBACK_CONTINUE;
  }

  json_t *req_type = json_object_get(body, "type");

  if (!json_is_number(req_type))
  {
    ulfius_set_empty_body_response(response, 400);
    return U_CALLBACK_CONTINUE;
  }

  switch(json_integer_value(req_type))
  {
  case PING:
    return ping_handler(response);
  case APPLICATION_COMMAND:
    return command_handler(
      json_object_get(body, "id"), 
      json_object_get(body, "data"),
      response);
  default:
    return U_CALLBACK_CONTINUE;
  }
}

void register_commands(interactions_handler *self)
{
  const char *discord_api_path_template = \
    "https://discord.com/api/v10" \
    "/applications/%s/guilds/%s/commands";

  size_t path_len = strlen(discord_api_path_template) 
    + strlen(self->app_id) 
    + strlen(self->guild_id);

  char *api_path = malloc(path_len + 1);
  sprintf(api_path, discord_api_path_template, self->app_id, self->guild_id);

  char *auth_str = malloc(strlen(self->bot_token) + 5);
  sprintf(auth_str, "Bot %s", self->bot_token);

  struct _u_request req = {0};
  ulfius_init_request(&req);
  req.http_url = api_path;
  req.http_verb = "GET";
  u_map_put(req.map_header, "Authorization", auth_str);
  u_map_put(req.map_header, "Content-Type", "application/json; charset=UTF-8");
  u_map_put(req.map_header, "User-Agent", "DiscordBot");

  struct _u_response res = {0};
  ulfius_init_response(&res);
  if (ulfius_send_http_request(&req, &res))
  {
    fprintf(stderr, "Cannot request installed commands\n");
    free(api_path);
    free(auth_str);
    ulfius_clean_response_full(&res);
    ulfius_clean_request_full(&req);
    return;
  }

  if (res.status / 100 != 2)
  {
    fprintf(stderr, "Requesting installed commands failed with status %d\n", res.status);
    free(api_path);
    free(auth_str);
    ulfius_clean_response_full(&res);
    ulfius_clean_request_full(&req);
    return;
  }

  json_t *data = ulfius_get_json_body_response(&res, NULL);
  if(data && !json_is_array(data))
  {
    fprintf(stderr, "Unexpected response from /commands endpoint: %s\n", res.binary_body);
    free(api_path);
    free(auth_str);
    ulfius_clean_response_full(&res);
    ulfius_clean_request_full(&req);
    return;
  }

  if (!data)
  {
    ulfius_clean_response(&res);
    memset(&res, 0, sizeof(struct _u_response));

    // install commands
    req.http_verb = "POST";

    json_t *command = json_object();
    json_object_set(command, "name", json_string("test"));
    json_object_set(command, "description", json_string("basic test command"));
    json_object_set(command, "type", json_integer(1));

    ulfius_set_json_body_request(&req, command);

    fprintf(stderr, "trying to install test command\n");
    if (ulfius_send_http_request(&req, &res))
    {
      fprintf(stderr, "Cannot install test command\n");
      free(api_path);
      free(auth_str);
      ulfius_clean_response_full(&res);
      ulfius_clean_request_full(&req);
    }

    if (res.status / 100 != 2)
    {
      fprintf(stderr, "Installing commands failed with status %d\n", res.status);
      free(api_path);
      free(auth_str);
      ulfius_clean_response_full(&res);
      ulfius_clean_request_full(&req);
      return;
    }

    ulfius_clean_response(&res);
  }

  fprintf(stderr, "%d commands installed\n", json_array_size(data));
}

interactions_handler *new_interactions_handler()
{
  interactions_handler *self = malloc(sizeof(interactions_handler));
  self->parent.callback = &interaction_handler_callback;
  self->parent.destruct = (void(*)(i_endpoint_handler*))&free;

  self->app_id = getenv("APP_ID");
  self->guild_id = getenv("GUILD_ID");
  self->bot_token = getenv("BOT_TOKEN");

  register_commands(self);

  return self;
}