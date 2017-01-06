#include "useq.h"

useq_track_t *useq_track_new(uint32_t n_items)
{
    useq_track_t *track = calloc(sizeof(useq_track_t), 1);
    track->n_items = n_items;
    track->items = calloc(sizeof(useq_event_item_t), n_items);
    track->pos = 0;
    return track;
}

void useq_track_destroy(useq_track_t *track)
{
    free(track);
}
