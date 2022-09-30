
#include <unistd.h>
#include <stdio.h>
#include <ulfius.h>
#include <signal.h>

#include "verifier.h"
#include "discord_interactions.h"

#define PORT 8080

/**
 * Callback function for the web application on /helloworld url call
 */
int callback_hello_world (const struct _u_request * request, struct _u_response * response, void * user_data) {
  ulfius_set_string_body_response(response, 200, "Hello World!");
  return U_CALLBACK_CONTINUE;
}

void term_handler(int signum)
{
  // do nothing, just continue main after pause()
}

/**
 * main function
 */
int main(void) 
{

  if (ulfius_global_init())
  {
    fprintf(stderr, "Error ulfius_global_init, exiting\n");
    return 1;
  }

  struct _u_instance instance;

  // Initialize instance with the port number
  if (ulfius_init_instance(&instance, PORT, NULL, NULL) != U_OK) {
    fprintf(stderr, "Error ulfius_init_instance, abort\n");
    return(1);
  }

  // Endpoint list declaration
  ulfius_add_endpoint_by_val(&instance, "POST", "/helloworld", NULL, 0, &callback_hello_world, NULL);

  i_endpoint_handler *verifier = verifier_new();
  interactions_handler *interactions_handler = new_interactions_handler();
  i_endpoint_handler *endp_interactions = (i_endpoint_handler*)interactions_handler;

  ulfius_add_endpoint_by_val(&instance, "POST", "/interactions", NULL, 0, verifier->callback, verifier);
  ulfius_add_endpoint_by_val(&instance, "POST", "/interactions", NULL, 1, endp_interactions->callback, endp_interactions);

  // Start the framework
  if (ulfius_start_framework(&instance) == U_OK) 
  {
    printf("Start framework on port %d (PID: %d)\n", instance.port, getpid());
    
    struct sigaction term_action;
    term_action.sa_handler = &term_handler;
    term_action.sa_flags = SA_SIGINFO;

    sigaction(SIGTERM, &term_action, NULL);
    pause();
  } 
  else 
  {
    fprintf(stderr, "Error starting framework\n");
  }
  printf("End framework\n");

  verifier->destruct(verifier);

  ulfius_stop_framework(&instance);
  ulfius_clean_instance(&instance);
  ulfius_global_close();

  return 0;
}
