#include "useq.h"

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

void useq_set_test_song(useq_state_t *state)
{
    state->n_tracks = 2;
    state->tracks = calloc(sizeof(state->tracks[0]), state->n_tracks);
    state->track_pos = calloc(sizeof(state->track_pos[0]), state->n_tracks);
    state->tracks[0] = &sample_drum_track;
    state->tracks[1] = &sample_bass_track;
    state->timer_end = state->master->samples_per_tick * PPQN * 4;
}

static int test_value = 0;

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

