#include <errno.h>
#include <smf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "useq.h"

static inline const useq_event_item_t *useq_get_next_event(useq_state_t *state, void **pptr)
{
    useq_tickpos_t earliest_time = USEQ_TICKPOS_END;
    int earliest_track = -1;
    for (int i = 0; i < state->n_tracks; ++i) {
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
    for (int i = 0; i < state->n_tracks; ++i)
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
    state->master = calloc(sizeof(useq_time_master_t), 1);
    useq_timer_restart(state);
    float tempo = 140.0;
    state->master->samples_per_tick = jack_get_sample_rate(state->jack_client) * 60.0 / (PPQN * tempo);
    state->timer_end = state->master->samples_per_tick * PPQN * 4;

    return state;
}

void useq_load_smf(useq_state_t *state, const char *filename)
{
    smf_t *song = smf_load(filename);
    assert(song);
    
    state->n_tracks = song->number_of_tracks;
    state->tracks = calloc(sizeof(state->tracks[0]), state->n_tracks);
    state->track_pos = calloc(sizeof(state->track_pos[0]), state->n_tracks);
    
    useq_tickpos_t endpos = 0;
    for (int i = 0; i < state->n_tracks; ++i) {
        smf_track_t *trk = smf_get_track_by_number(song, 1 + i);
        int evc = 0;
        for (int j = 0; ; ++j) {
            smf_event_t *ev = smf_track_get_event_by_number(trk, 1 + j);
            if (smf_event_is_eot(ev)) {
                if (ev->time_pulses > endpos)
                    endpos = ev->time_pulses;
                break;
            }
            if (!smf_event_is_metadata(ev) && ev->midi_buffer_length <= 3)
                ++evc;
#if 0
            char *decoded = smf_event_decode(ev);
            printf("decoded = %s %s\n", decoded, smf_event_is_metadata(ev) ? "(metadata)" : "");
            free(decoded);
#endif
        }
        state->tracks[i] = calloc(sizeof(useq_track_t), 1);
        state->tracks[i]->n_items = evc;
        state->tracks[i]->items = calloc(sizeof(useq_event_item_t), evc);
        useq_event_item_t *items = state->tracks[i]->items;
        evc = 0;
        for (int j = 0; ; ++j) {
            smf_event_t *ev = smf_track_get_event_by_number(trk, 1 + j);
            if (smf_event_is_eot(ev))
                break;
            if (!smf_event_is_metadata(ev) && ev->midi_buffer_length <= 3) {
                items[evc].pos_ppqn = ev->time_pulses;
                memcpy(items[evc].event, ev->midi_buffer, ev->midi_buffer_length);
                items[evc].len = ev->midi_buffer_length;
                ++evc;
            }
        }
    }
    state->timer_end = state->master->samples_per_tick * endpos;

    struct smf_tempo_struct *ts = smf_get_tempo_by_number(song, 0);
    float tempo = 60 * 1e6 /ts->microseconds_per_quarter_note;
    
    state->master->samples_per_tick = jack_get_sample_rate(state->jack_client) * 60.0 / (tempo * song->ppqn);
}

void useq_destroy_song(useq_state_t *state)
{
    free(state->tracks);
    free(state->track_pos);
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

void useq_destroy(useq_state_t *state)
{
    useq_destroy_song(state);
    free(state->master);
    assert(state->jack_client);
    jack_client_close(state->jack_client);
    state->jack_client = NULL;
    free((char *)state->client_name);
    free(state);
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Usage: %s <midifile> <jackport>\n", argv[0]);
        return 1;
    }
    useq_state_t *state = useq_create("useq");
    if (!state)
    {
        fprintf(stderr, "Unable to create a JACK client\n");
        return 1;
    }
    useq_load_smf(state, argv[1]);
    useq_activate(state);
    jack_connect(state->jack_client, "useq:midi", argv[2]);

    useq_test(state);
    printf("Press ENTER to quit.\n");
    while(getchar() != 10)
        ;

    useq_deactivate(state);
    useq_destroy(state);
    return 0;
}
