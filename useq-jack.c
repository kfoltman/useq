#include <errno.h>
#include <smf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "useq.h"

static inline const useq_event_item_t *useq_get_next_event(useq_output_t *output, void **pptr)
{
    useq_tickpos_t earliest_time = USEQ_TICKPOS_END;
    int earliest_track = -1;
    for (int i = 0; i < output->n_tracks; ++i) {
        useq_track_t *t = output->tracks[i];
        if (!t)
            continue;
        unsigned tp = t->pos;
        if (tp < t->n_items && t->items[tp].pos_ticks < earliest_time) {
            earliest_time = t->items[tp].pos_ticks;
            earliest_track = i;
        }
    }
    if(earliest_track == -1)
        return NULL;

    useq_track_t *track = output->tracks[earliest_track];
    uint32_t *tpos = &track->pos;
    *pptr = tpos;
    return &track->items[*tpos];
}

static inline void useq_get_next_event_finalize(void **pptr)
{
    ++*(uint32_t *)pptr;
}

void useq_timer_restart(useq_state_t *state)
{
    state->timer = 0;
    for (int o = 0; o < state->n_outputs; ++o) {
        useq_output_t *output = state->outputs[o];
        for (int i = 0; i < output->n_tracks; ++i)
            output->tracks[i]->pos = 0;
    }
}

int useq_process_callback(jack_nframes_t nframes, void *arg)
{
    useq_state_t *state = arg;

    if (state->rtf) {
        while(state->rtf) {
            (*state->rtf->callback)(state, state->rtf->arg);
            state->rtf = state->rtf->next;
        }
        if (sem_post(&state->rtf_sem) < 0) {
            return 1;
        }
    }

    for (int o = 0; o < state->n_outputs; ++o) {
        useq_output_t *output = state->outputs[o];
        void *buf = jack_port_get_buffer(output->port, nframes);
        jack_midi_clear_buffer(buf);

        uint32_t displ = 0;
        do {
            while(true) {
                void *tmp;
                const useq_event_item_t *ev = useq_get_next_event(output, &tmp);
                if (!ev)
                    break;
                uint64_t pos_smp = ev->pos_ticks * state->master->samples_per_tick;
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
            if (state->timer < state->timer_end_samples) {
                break;
            } else {
                displ += state->timer - state->timer_end_samples;
                useq_timer_restart(state);
            }
        } while(true);
    }

    return 0;
}

int useq_jack_create(useq_state_t *state, const char *client_name)
{
    jack_status_t status;
    jack_client_t *client = jack_client_open(client_name, JackNullOption, &status);
    if (!client) {
        return -1;
    }
    state->client_name = strdup(jack_get_client_name(client));
    state->jack_client = client;
    return 0;
}

void useq_jack_activate(useq_state_t *state)
{
    assert(state->jack_client);
    assert(state->n_outputs);
    assert(state->outputs);
    jack_set_process_callback(state->jack_client, useq_process_callback, state);

    state->master->samples_per_tick = jack_get_sample_rate(state->jack_client) * 60.0 / (state->master->tempo * state->master->ppqn);
    state->timer_end_samples = state->master->samples_per_tick * state->timer_end_ticks;

    for (int o = 0; o < state->n_outputs; ++o) {
        useq_output_t *output = state->outputs[o];
        if (!output->port)
            output->port = jack_port_register(state->jack_client, output->port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
    }

    jack_activate(state->jack_client);
    state->activated = 1;
}

struct cmd_update_outputs {
    int n_outputs;
    useq_output_t **outputs;
};

static void cmd_update_outputs_exec(useq_state_t *state, void *arg) {
    struct cmd_update_outputs *u = arg;
    state->n_outputs = u->n_outputs;
    state->outputs = u->outputs;
}

void useq_state_set_outputs(useq_state_t *state, int n_outputs, useq_output_t **outputs)
{
    int old_n_outputs = state->n_outputs;
    useq_output_t **old_outputs = state->outputs;

    for (int o = 0; o < state->n_outputs; ++o) {
        state->outputs[o]->nonrt_flags = 1;
    }

    for (int o = 0; o < n_outputs; ++o) {
        useq_output_t *output = outputs[o];
        output->nonrt_flags = 0;
        if (!output->port)
            output->port = jack_port_register(state->jack_client, output->port_name, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
    }

    struct cmd_update_outputs arg = { n_outputs, outputs };
    USEQ_RTF(update_outputs, cmd_update_outputs_exec, &arg);
    useq_do(state, &update_outputs);

    for (int o = 0; o < old_n_outputs; ++o) {
        useq_output_t *output = old_outputs[o];
        if (output->nonrt_flags) {
            jack_port_unregister(state->jack_client, output->port);
            output->port = NULL;
        }
    }
}

jack_client_t *useq_jack_get_client(useq_state_t *state)
{
    return state->jack_client;
}

const char *useq_jack_get_client_name(useq_state_t *state)
{
    return state->client_name;
}

void useq_jack_deactivate(useq_state_t *state)
{
    for (int o = 0; o < state->n_outputs; ++o) {
        useq_output_t *output = state->outputs[o];
        if (output->port) {
            jack_port_unregister(state->jack_client, output->port);
            output->port = NULL;
        }
    }
    state->activated = 0;
    jack_deactivate(state->jack_client);
}

void useq_do(useq_state_t *state, useq_rtf_t *rtf)
{
    state->rtf = rtf;
    if (state->activated) {
        while (sem_wait(&state->rtf_sem) < 0) {
            if (errno == EINTR)
                continue;
            break;
        }
    } else {
        while(state->rtf) {
            state->rtf->callback(state, state->rtf->arg);
            state->rtf = state->rtf->next;
        }
    }
}

void useq_jack_destroy(useq_state_t *state)
{
    if (!state->jack_client)
        return;
    jack_client_close(state->jack_client);
    state->jack_client = NULL;
    free((char *)state->client_name);
    state->client_name = NULL;
}
