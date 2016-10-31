#include <assert.h>
#include <errno.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <jack/jack.h>

typedef uint64_t useq_tickpos_t;

struct useq_state;

typedef struct useq_rtfunction
{
    void (*callback)(struct useq_state *state);
    struct useq_rtfunction *next;
} useq_rtf_t;

typedef struct useq_state
{
    const char *client_name;
    jack_client_t *jack_client;
    useq_rtf_t *rtf;
    sem_t rtf_sem;
} useq_state_t;

int useq_process_callback(jack_nframes_t nframes, void *arg)
{
    useq_state_t *state = arg;
    
    if (state->rtf) {
	while(state->rtf) {
	    (*state->rtf->callback)(state);
	    state->rtf = state->rtf->next;
	}
	if (sem_post(&state->rtf_sem) < 0) {
	    return 1;
	}
    }
    
    return 0;
}

useq_state_t *useq_create(const char *client_name)
{
    useq_state_t *state = calloc(sizeof(useq_state_t), 1);
    state->client_name = strdup(client_name);
    
    jack_status_t status;
    jack_client_t *client = jack_client_open(state->client_name, JackNullOption, &status);
    if (!client) {
	free((char *)state->client_name);
	free(state);
	return NULL;
    }
    state->jack_client = client;
    sem_init(&state->rtf_sem, 0, 0);
    return state;
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

void useq_do(useq_state_t *state, useq_rtf_t *rtf)
{
    state->rtf = rtf;
    while (sem_wait(&state->rtf_sem) < 0) {
	if (errno == EINTR)
	    continue;
	break;
    }
}

int test_value = 0;

#define USEQ_RTF(name_, callback_) \
    useq_rtf_t name_ = { .callback = callback_, .next = NULL }

void useq_test_callback(useq_state_t *state)
{
    ++test_value;
}

void useq_test(useq_state_t *state)
{
    USEQ_RTF(testrtf, useq_test_callback);
    useq_do(state, &testrtf);
    useq_do(state, &testrtf);
    assert(test_value == 2);
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

    useq_test(state);

    useq_deactivate(state);
    useq_destroy(state);
    return 0;
}
