#ifndef ENDPOINTHANDLER_H
#define ENDPOINTHANDLER_H

struct _u_request;
struct _u_response;

typedef struct _endpoint_handler
{
    int (*callback)(
        const struct _u_request *request, 
        struct _u_response *response, 
        void *user_data);
    void (*destruct)(struct _endpoint_handler *self);
} i_endpoint_handler;

#endif // ENDPOINTHANDLER_H