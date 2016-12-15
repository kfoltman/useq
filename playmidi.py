import ctypes
import sys

if len(sys.argv) != 3:
    print "Usage: %s <midifile> <jack output>" % sys.argv[0]
    sys.exit(1)

dll = ctypes.CDLL("./useq.so")
dll.useq_play_midi.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
dll.useq_play_midi(sys.argv[1], sys.argv[2])
