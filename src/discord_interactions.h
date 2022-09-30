#ifndef DISCORDINTERACTIONS_H
#define DISCORDINTERACTIONS_H

#include "endpoint_handler.h"

typedef struct interactions_handler
{
    i_endpoint_handler parent;
    const char *app_id;
    const char *guild_id;
    const char *bot_token;
} interactions_handler;

interactions_handler *new_interactions_handler(void);

#endif // DISCORDINTERACTIONS_H