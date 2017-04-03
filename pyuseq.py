import ctypes

class EventEncoding:
    @staticmethod
    def note_off(channel, note, vel):
        return 0x8003 + (channel - 1) * 256 + (note << 16) + (vel << 24)
    @staticmethod
    def note_on(channel, note, vel):
        return 0x9003 + (channel - 1) * 256 + (note << 16) + (vel << 24)
    @staticmethod
    def polyphonic_aftertouch(channel, note, vel):
        return 0xa003 + (channel - 1) * 256 + (note << 16) + (vel << 24)
    @staticmethod
    def cc(channel, note, vel):
        return 0xb003 + (channel - 1) * 256 + (note << 16) + (vel << 24)
    @staticmethod
    def program_change(channel, program):
        return 0xc002 + (channel - 1) * 256 + (program << 16)
    @staticmethod
    def channel_aftertouch(channel, pressure):
        return 0xd002 + (channel - 1) * 256 + (pressure << 16)
    @staticmethod
    def pitch_wheel(channel, pitch):
        # note: pitch = 0..16383 (centre is 8192)
        return 0xe003 + (channel - 1) * 256 + ((pitch & 127) << 16) + ((pitch // 128) << 24)

class EncodeAsAscii(object):
    @classmethod
    def from_param(klass, value):
        if isinstance(value, str):
            return value.encode('ascii')
        elif isinstance(value, bytes):
            return value
        else:
            return str(value).encode('ascii')

def init():    
    def __fn(name, res, argtypes):
        func = getattr(dll, name)
        func.restype = res
        func.argtypes = argtypes
        globals()[name] = func

    dll = ctypes.CDLL("./useq.so")
    __fn('useq_create', ctypes.c_void_p, [])
    __fn('useq_destroy', None, [ctypes.c_void_p])

    __fn('useq_load_smf', ctypes.c_int, [ctypes.c_void_p, EncodeAsAscii])
    __fn('useq_test', None, [ctypes.c_void_p])
    __fn('useq_destroy_song', None, [ctypes.c_void_p])

    __fn('useq_state_add_output', None, [ctypes.c_void_p, ctypes.c_void_p])
    __fn('useq_state_set_tempo_ppqn', None, [ctypes.c_void_p, ctypes.c_float, ctypes.c_int])
    __fn('useq_state_set_length', None, [ctypes.c_void_p, ctypes.c_int])

    __fn('useq_jack_create', ctypes.c_int, [ctypes.c_void_p, EncodeAsAscii])
    __fn('useq_jack_activate', None, [ctypes.c_void_p])
    __fn('useq_jack_get_client', ctypes.c_void_p, [ctypes.c_void_p])
    __fn('useq_jack_get_client_name', ctypes.c_char_p, [ctypes.c_void_p])
    __fn('useq_jack_deactivate', None, [ctypes.c_void_p])
    __fn('useq_jack_destroy', None, [ctypes.c_void_p])
    
    __fn('useq_track_new', ctypes.c_void_p, [ctypes.c_int])
    __fn('useq_track_set_event', None, [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int])
    __fn('useq_track_destroy', None, [ctypes.c_void_p])

    __fn('useq_output_new', ctypes.c_void_p, [ctypes.c_void_p, EncodeAsAscii])
    __fn('useq_output_add_track', None, [ctypes.c_void_p, ctypes.c_void_p])
    __fn('useq_output_replace_track', None, [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p])
    __fn('useq_output_destroy', None, [ctypes.c_void_p])

    dll = ctypes.CDLL("libjack.so.0")
    __fn("jack_connect", None, [ctypes.c_void_p, EncodeAsAscii, EncodeAsAscii])


init()
