#include "discord_interactions.h"

#include "../http_api/discord_http_api.h"

#include <jansson.h>
#include <stdio.h>
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
  http_api_client *client = http_api_client_init();

  // request whether commands are already installed
  {
    fprintf(stderr, "Requesting installed commands...\n");
    struct _u_response *get_commands_resp = 
      http_api_client_perform(client, "GET", NULL, "/applications/%s/guilds/%s/commands", self->app_id, self->guild_id);

    if ( !get_commands_resp )
    {
      fprintf(stderr, "Cannot request installed commands\n");
      http_api_client_free(client);
      return;
    }

    if ( !HTTP_STATUS_IS_OK(get_commands_resp->status) )
    {
      fprintf(stderr, "Requesting installed commands failed with status %ld\n", get_commands_resp->status);
      ulfius_clean_response_full(get_commands_resp);
      http_api_client_free(client);
      return;
    }

    json_t *data = ulfius_get_json_body_response(get_commands_resp, NULL);
    if( data && !json_is_array(data) )
    {
      fprintf(stderr, "Unexpected response from /commands endpoint: %s\n", (char*)get_commands_resp->binary_body);
      ulfius_clean_response_full(get_commands_resp);
      http_api_client_free(client);
      return;
    }

    if ( data )
    {
      fprintf(stderr, "%lu commands installed\n", json_array_size(data));
      // TODO: check, whether all necessary commands are really installed
      ulfius_clean_response_full(get_commands_resp);
      http_api_client_free(client);
      return;
    }

    ulfius_clean_response_full(get_commands_resp);
  }

  // install commands
  fprintf(stderr, "trying to install test command\n");

  json_t *command = json_object();
  json_object_set(command, "name", json_string("test"));
  json_object_set(command, "description", json_string("basic test command"));
  json_object_set(command, "type", json_integer(1));

  struct _u_response *post_commands_resp = 
    http_api_client_perform(client, "POST", NULL, "/applications/%s/guilds/%s/commands", self->app_id, self->guild_id);

  if ( !post_commands_resp )
  {
    fprintf(stderr, "Cannot install test command\n");
    http_api_client_free(client);
  }

  if ( !HTTP_STATUS_IS_OK(post_commands_resp->status) )
  {
    fprintf(stderr, "Installing commands failed with status %lu\n", post_commands_resp->status);
    ulfius_clean_response_full(post_commands_resp);
    http_api_client_free(client);
    return;
  }

  ulfius_clean_response(post_commands_resp);
  http_api_client_free(client);

  fprintf(stderr, "/test command installed succesfully\n");
}

interactions_handler *new_interactions_handler()
{
  interactions_handler *self = malloc(sizeof(interactions_handler));
  self->parent.callback = &interaction_handler_callback;
  self->parent.destruct = (void(*)(i_endpoint_handler*))&free;

  self->app_id = getenv("APP_ID");
  self->guild_id = getenv("GUILD_ID");

  register_commands(self);

  return self;
}
