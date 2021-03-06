#include "useq.h"
#include <string.h>

useq_output_t *useq_output_new(useq_state_t *state, const char *name)
{
    useq_output_t *output = calloc(sizeof(useq_output_t), 1);
    if (!output)
        return output;

    output->state = state;
    output->port_name = strdup(name);
    output->port = NULL;
    output->n_tracks = 0;
    output->tracks = NULL;
    return output;
}

struct cmd_update_tracks {
    useq_output_t *output;
    int n_tracks;
    useq_track_t **tracks;
};

static void cmd_update_tracks_exec(useq_state_t *state, void *arg) {
    struct cmd_update_tracks *u = arg;
    u->output->n_tracks = u->n_tracks;
    u->output->tracks = u->tracks;
}

void useq_output_set_tracks(useq_output_t *output, int n_tracks, useq_track_t **tracks)
{
    useq_track_t **old_tracks = output->tracks;
    struct cmd_update_tracks arg = { output, n_tracks, tracks };
    USEQ_RTF(update_tracks, cmd_update_tracks_exec, &arg);
    useq_do(output->state, &update_tracks);
    free(old_tracks);
}

bool useq_output_add_track(useq_output_t *output, useq_track_t *track)
{
    useq_track_t **tracks = calloc(sizeof(tracks[0]), output->n_tracks + 1);
    if (!tracks)
        return false;
    memcpy(tracks, output->tracks, sizeof(tracks[0]) * output->n_tracks);
    tracks[output->n_tracks] = track;
    useq_output_set_tracks(output, output->n_tracks + 1, tracks);
    return true;
}

struct cmd_replace_track {
    useq_track_t **ptr;
    useq_track_t *new_track;
};

static void cmd_replace_track_exec(useq_state_t *state, void *arg) {
    struct cmd_replace_track *u = arg;
    *u->ptr = u->new_track;
}

int useq_output_replace_track(useq_output_t *output, useq_track_t *old_track, useq_track_t *new_track)
{
    int count = 0;
    for (uint32_t i = 0; i < output->n_tracks; ++i) {
        if (output->tracks[i] == old_track) {
            ++count;
            struct cmd_replace_track arg = { &output->tracks[i], new_track };
            USEQ_RTF(replace_track, cmd_replace_track_exec, &arg);
            useq_do(output->state, &replace_track);
        }
    }
    return count;
}

void useq_output_destroy(useq_output_t *output)
{
    for (int i = 0; i < output->n_tracks; ++i) {
        useq_track_destroy(output->tracks[i]);
    }
    free(output->tracks);
    output->tracks = NULL;
    if (output->port_name)
        free((char *)output->port_name);
}

