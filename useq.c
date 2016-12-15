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
    state->midi_output = NULL;
    sem_init(&state->rtf_sem, 0, 0);

    state->master = calloc(sizeof(useq_time_master_t), 1);
    state->master->tempo = 140.0;
    state->master->ppqn = PPQN;

    return state;
}

void useq_load_smf(useq_state_t *state, const char *filename)
{
    useq_destroy_song(state);
    
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
                items[evc].pos_ticks = ev->time_pulses;
                memcpy(items[evc].event, ev->midi_buffer, ev->midi_buffer_length);
                items[evc].len = ev->midi_buffer_length;
                ++evc;
            }
        }
    }
    state->timer_end_ticks = endpos;
    state->master->ppqn = song->ppqn;

    struct smf_tempo_struct *ts = smf_get_tempo_by_number(song, 0);
    state->master->tempo = 60 * 1e6 /ts->microseconds_per_quarter_note;
    
}

void useq_destroy_song(useq_state_t *state)
{
    free(state->tracks);
    state->tracks = NULL;
    free(state->track_pos);
    state->track_pos = NULL;
}

void useq_destroy(useq_state_t *state)
{
    assert(!state->jack_client);
    assert(!state->client_name);

    useq_destroy_song(state);
    free(state->master);
    free(state);
}

int useq_play_midi(const char *midi_file, const char *port)
{
    useq_state_t *state = useq_create();

    if (useq_jack_create(state, "useq") < 0)
    {
        fprintf(stderr, "Unable to create a JACK client\n");
        return 1;
    }
    useq_load_smf(state, midi_file);
    useq_jack_activate(state);
    jack_connect(state->jack_client, "useq:midi", port);

    useq_test(state);
    printf("Press ENTER to quit.\n");
    while(getchar() != 10)
        ;

    useq_jack_deactivate(state);
    useq_jack_destroy(state);
    useq_destroy(state);
    return 0;
}
