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
    state->n_outputs = 1;
    state->outputs = calloc(sizeof(state->outputs[0]), state->n_outputs);

    useq_output_t *output = useq_output_new(state, "output");
    output->n_tracks = 2;
    output->tracks = calloc(sizeof(output->tracks[0]), output->n_tracks);
    output->tracks[0] = &sample_drum_track;
    output->tracks[1] = &sample_bass_track;

    state->outputs[0] = output;
    state->timer_end_ticks = state->master->ppqn * 4;
}

static int test_value = 0;

void useq_test_callback(useq_state_t *state, void *ignore __attribute__((unused)))
{
    ++test_value;
}

void useq_test(useq_state_t *state)
{
    USEQ_RTF(testrtf, useq_test_callback, NULL);
    useq_do(state, &testrtf);
    useq_do(state, &testrtf);
    assert(test_value == 2);
}

