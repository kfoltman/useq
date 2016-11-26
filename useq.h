#include <assert.h>
#include <semaphore.h>
#include <stdint.h>

#include <jack/jack.h>
#include <jack/midiport.h>

#define PPQN 16

#define USEQ_TICKPOS_END (-1ULL)

typedef uint64_t useq_samplepos_t;
typedef uint64_t useq_sampledelta_t;
typedef uint64_t useq_tickpos_t;
typedef uint64_t useq_tickdelta_t;

typedef struct useq_time_master {
    float samples_per_tick;
} useq_time_master_t;

typedef struct useq_event_item {
    useq_tickpos_t pos_ppqn;
    uint8_t len;
    uint8_t event[3];
    uint32_t extra;
} useq_event_item_t;

typedef struct useq_track {
    useq_event_item_t *items;
    uint32_t n_items;
} useq_track_t;

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
    jack_port_t *midi_output;
    useq_rtf_t *rtf;
    sem_t rtf_sem;
    int32_t timer, timer_end;
    useq_time_master_t *master;
    int n_tracks;
    useq_track_t **tracks;
    uint32_t *track_pos;
} useq_state_t;

#define USEQ_RTF(name_, callback_) \
    useq_rtf_t name_ = { .callback = callback_, .next = NULL }

extern void useq_do(useq_state_t *state, useq_rtf_t *rtf);

// Temp dev testing functions - to be removed or expanded into real tests

extern void useq_set_test_song(useq_state_t *state);

extern void useq_test(useq_state_t *state);