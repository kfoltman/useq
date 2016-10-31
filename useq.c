#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <jack/jack.h>

typedef uint64_t useq_tickpos_t;

typedef struct useq_state
{
    const char *client_name;
    jack_client_t *jack_client;
    // useq_song_t *cur_song;
} useq_state_t;

int useq_process_callback(jack_nframes_t nframes, void *arg)
{
    // useq_state_t *state = arg;
    
    // do something here
    
    return 1;
}

useq_state_t *useq_create(const char *client_name)
{
    useq_state_t *state = calloc(sizeof(useq_state_t), 1);
    state->client_name = strdup(client_name);
    
    jack_status_t status;
    jack_client_t *client = jack_client_open(state->client_name, JackNullOption, &status);
    if (client) {
        state->jack_client = client;
        return state;
    }
    free((char *)state->client_name);
    free(state);
    return NULL;
}

void useq_activate(useq_state_t *state)
{
    assert(state->jack_client);
    jack_set_process_callback(state->jack_client, useq_process_callback, state);
    jack_activate(state->jack_client);
}

void useq_deactivate(useq_state_t *state)
{
    jack_deactivate(state->jack_client);
}

void useq_destroy(useq_state_t *state)
{
    assert(state->jack_client);
    jack_client_close(state->jack_client);
    state->jack_client = NULL;
    free((char *)state->client_name);
    free(state);
}

int main(int argc, char *argv[])
{
    useq_state_t *state = useq_create("useq");
    if (!state)
    {
        fprintf(stderr, "Unable to create a JACK client\n");
        return 1;
    }
    useq_activate(state);
    useq_deactivate(state);
    useq_destroy(state);
    return 0;
}
