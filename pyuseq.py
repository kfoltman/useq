import ctypes

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
    __fn('useq_output_replace_track', None, [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p])
    __fn('useq_output_destroy', None, [ctypes.c_void_p])

    dll = ctypes.CDLL("libjack.so")
    __fn("jack_connect", None, [ctypes.c_void_p, EncodeAsAscii, EncodeAsAscii])


init()
