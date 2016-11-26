#include <errno.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <jack/jack.h>
#include <jack/midiport.h>

#include "useq.h"

struct useq_state;

typedef struct useq_rtfunction
{
    void (*callback)(struct useq_state *state);
    struct useq_rtfunction *next;
} useq_rtf_t;

#define NTRACKS 2

typedef struct useq_state
{
    const char *client_name;
    jack_client_t *jack_client;
    jack_port_t *midi_output;
    useq_rtf_t *rtf;
    sem_t rtf_sem;
    int32_t timer, timer_end;
    useq_time_master_t *master;
    useq_track_t *tracks[NTRACKS];
    uint32_t track_pos[NTRACKS];
} useq_state_t;

static inline const useq_event_item_t *useq_get_next_event(useq_state_t *state, void **pptr)
{
    useq_tickpos_t earliest_time = USEQ_TICKPOS_END;
    int earliest_track = -1;
    for (int i = 0; i < NTRACKS; ++i) {
        useq_track_t *t = state->tracks[i];
        if (!t)
            continue;
        unsigned tp = state->track_pos[i];
        if (tp < t->n_items && t->items[tp].pos_ppqn < earliest_time) {
            earliest_time = t->items[tp].pos_ppqn;
            earliest_track = i;
        }
    }
    if(earliest_track == -1)
        return NULL;

    useq_track_t *track = state->tracks[earliest_track];
    uint32_t *tpos = &state->track_pos[earliest_track];
    *pptr = tpos;
    return &track->items[*tpos];
}

static inline void useq_get_next_event_finalize(void **pptr)
{
    ++*(uint32_t *)pptr;
}

static void useq_timer_restart(useq_state_t *state)
{
    state->timer = 0;
    for (int i = 0; i < NTRACKS; ++i)
        state->track_pos[i] = 0;
}

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

    void *buf = jack_port_get_buffer(state->midi_output, nframes);
    jack_midi_clear_buffer(buf);

    uint32_t displ = 0;
    do {
        while(true) {
            void *tmp;
            const useq_event_item_t *ev = useq_get_next_event(state, &tmp);
            if (!ev)
                break;
            uint64_t pos_smp = ev->pos_ppqn * state->master->samples_per_tick;
            int32_t time = pos_smp - state->timer;
            // Correct for past overflow
            if (time >= nframes)
                break;
            if (time < 0)
                time = 0;
            
            uint8_t *p = jack_midi_event_reserve(buf, displ + time, ev->len);
            if (!p)
                break;
            memcpy(p, ev->event, ev->len);
            useq_get_next_event_finalize(tmp);
        }

        state->timer += nframes;
        if (state->timer < state->timer_end) {
            break;
        } else {
            displ += state->timer - state->timer_end;
            useq_timer_restart(state);
        }
    } while(true);

    return 0;
}

static useq_event_item_t sample_bass_track_data[] = {
    {0x00, 2, {0xC0, 38, 100}, 0},
    {0x00, 3, {0x90, 28, 100}, 0},
    {0x0A, 3, {0x80, 28, 100}, 0},
    {0x2A, 3, {0x90, 28, 100}, 0},
    {0x3A, 3, {0x80, 28, 100}, 0},
};

static useq_event_item_t sample_drum_track_data[] = {
    {0x00, 3, {0x99, 36, 100}, 0},
    {0x00, 3, {0x99, 42, 100}, 0},
    {0x0A, 3, {0x99, 42, 100}, 0},
    {0x10, 3, {0x99, 38, 100}, 0},
    {0x10, 3, {0x99, 42, 100}, 0},
    {0x15, 3, {0x99, 42, 50}, 0},
    {0x1A, 3, {0x99, 42, 100}, 0},
    {0x20, 3, {0x99, 36, 100}, 0},
    {0x20, 3, {0x99, 42, 100}, 0},
    {0x25, 3, {0x99, 44, 100}, 0},
    {0x2A, 3, {0x99, 42, 100}, 0},
    {0x30, 3, {0x99, 38, 100}, 0},
    {0x30, 3, {0x99, 42, 100}, 0},
    {0x3A, 3, {0x99, 46, 100}, 0},
};

static useq_track_t sample_drum_track = {
    .items = sample_drum_track_data,
    .n_items = sizeof(sample_drum_track_data) / sizeof(sample_drum_track_data[0]),
};

static useq_track_t sample_bass_track = {
    .items = sample_bass_track_data,
    .n_items = sizeof(sample_bass_track_data) / sizeof(sample_bass_track_data[0]),
};

#define PPQN 16

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

    state->midi_output = jack_port_register(state->jack_client, "midi", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
    state->tracks[0] = &sample_drum_track;
    state->tracks[1] = &sample_bass_track;
    state->track_pos[0] = 0;
    state->track_pos[1] = 0;
    state->timer = 0;
    state->master = calloc(sizeof(useq_time_master_t), 1);
    float tempo = 140.0;
    state->master->samples_per_tick = jack_get_sample_rate(state->jack_client) * 60.0 / (PPQN * tempo);
    state->timer_end = state->master->samples_per_tick * PPQN * 4;

    return state;
}

void useq_activate(useq_state_t *state)
{
    assert(state->jack_client);
    assert(state->midi_output);
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
    printf("Press ENTER to quit.\n");
    while(getchar() != 10)
        ;

    useq_deactivate(state);
    useq_destroy(state);
    return 0;
}
