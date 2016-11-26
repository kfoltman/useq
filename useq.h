#include <assert.h>
#include <stdint.h>

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

