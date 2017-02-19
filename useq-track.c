#include "useq.h"

useq_track_t *useq_track_new(uint32_t n_items)
{
    useq_track_t *track = calloc(sizeof(useq_track_t), 1);
    track->n_items = n_items;
    track->items = calloc(sizeof(useq_event_item_t), n_items);
    track->pos = 0;
    return track;
}

void useq_track_set_event(useq_track_t *track, uint32_t item, uint32_t pos_ticks, uint32_t event)
{
    assert(item < track->n_items);
    track->items[item].pos_ticks = pos_ticks;
    track->items[item].len = event & 0xFF;
    track->items[item].event[0] = (event >> 8) & 0xFF;
    track->items[item].event[1] = (event >> 16) & 0xFF;
    track->items[item].event[2] = (event >> 24) & 0xFF;
    track->items[item].extra = 0;
}

void useq_track_destroy(useq_track_t *track)
{
    free(track);
}
