#include <errno.h>
#include <smf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "useq.h"

useq_state_t *useq_create()
{
    useq_state_t *state = calloc(sizeof(useq_state_t), 1);
    state->jack_client = NULL;
    state->client_name = NULL;
    state->n_outputs = 0;
    state->outputs = NULL;
    sem_init(&state->rtf_sem, 0, 0);

    state->master = calloc(sizeof(useq_time_master_t), 1);
    state->master->tempo = 140.0;
    state->master->ppqn = PPQN;

    return state;
}

int useq_load_smf(useq_state_t *state, const char *filename)
{
    useq_destroy_song(state);

    smf_t *song = smf_load(filename);
    if (!song)
        return -1;

    int n_tracks = song->number_of_tracks;
    useq_track_t **tracks = calloc(sizeof(tracks[0]), n_tracks);
    
    useq_tickpos_t endpos = 0;
    for (int i = 0; i < n_tracks; ++i) {
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
        tracks[i] = useq_track_new(evc);
        useq_event_item_t *items = tracks[i]->items;
        evc = 0;
        for (int j = 0; ; ++j) {
            smf_event_t *ev = smf_track_get_event_by_number(trk, 1 + j);
            if (smf_event_is_eot(ev))
                break;
            if (!smf_event_is_metadata(ev) && ev->midi_buffer_length <= 3) {
                items[evc].pos_ticks = ev->time_pulses;
                memcpy(items[evc].event, ev->midi_buffer, ev->midi_buffer_length);
                items[evc].len = ev->midi_buffer_length;
                ++evc;
            }
        }
    }
    useq_output_t *output = useq_output_new(state, "midi");
    useq_output_set_tracks(output, n_tracks, tracks);

    int n_outputs = 1;
    useq_output_t **outputs = calloc(sizeof(state->outputs[0]), n_outputs);
    outputs[0] = output;
    useq_state_set_outputs(state, n_outputs, outputs);
    state->timer_end_ticks = endpos;
    state->master->ppqn = song->ppqn;

    struct smf_tempo_struct *ts = smf_get_tempo_by_number(song, 0);
    state->master->tempo = 60 * 1e6 /ts->microseconds_per_quarter_note;

    smf_delete(song);

    return 0;
}

void useq_destroy_song(useq_state_t *state)
{
    for (int o = 0; o < state->n_outputs; ++o) {
        useq_output_t *output = state->outputs[o];
        useq_output_destroy(output);
    }
    free(state->outputs);
    state->n_outputs = 0;
    state->outputs = NULL;
}

void useq_destroy(useq_state_t *state)
{
    assert(!state->jack_client);
    assert(!state->client_name);

    useq_destroy_song(state);
    free(state->master);
    free(state);
}

