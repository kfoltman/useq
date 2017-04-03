#!/usr/bin/env python3
from pyuseq import *
import sys

is_py3 = sys.version_info >= (3, 0)

state = useq_create()
if useq_jack_create(state, "useq") < 0:
    print("Cannot create JACK client.")
    sys.exit(1)

output = useq_output_new(state, "midi")
track = useq_track_new(2)
useq_track_set_event(track, 0, 0, EventEncoding.note_on(10, 36, 100))
useq_track_set_event(track, 1, 32, EventEncoding.note_on(10, 38, 100))
useq_output_add_track(output, track)
useq_state_add_output(state, output)
useq_state_set_length(state, 64)
useq_state_set_tempo_ppqn(state, 120, 32)

useq_jack_activate(state)
jack_connect(useq_jack_get_client(state), "%s:midi" % (useq_jack_get_client_name(state), ), sys.argv[1])
useq_test(state)
print("Press ENTER to quit.")
if is_py3:
    input()
else:
    raw_input()
useq_jack_deactivate(state)
useq_jack_destroy(state)
useq_destroy(state)
