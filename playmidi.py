#!/usr/bin/env python3
from pyuseq import *
import sys

is_py3 = sys.version_info >= (3, 0)

if len(sys.argv) != 3:
    print("Usage: %s <midifile> <jack output>" % sys.argv[0])
    sys.exit(1)

state = useq_create()
if useq_jack_create(state, "useq") < 0:
    print("Cannot create JACK client.")
    sys.exit(1)
if useq_load_smf(state, sys.argv[1]) < 0:
    print("Cannot load MIDI file.")
    sys.exit(2)
useq_jack_activate(state)
jack_connect(useq_jack_get_client(state), "%s:midi" % (useq_jack_get_client_name(state), ), sys.argv[2])
useq_test(state)
print("Press ENTER to quit.")
if is_py3:
    input()
else:
    raw_input()
useq_jack_deactivate(state)
useq_jack_destroy(state)
useq_destroy(state)
