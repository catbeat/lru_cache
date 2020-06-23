from m5.params import *
from m5.proxy import *
from MemObject import MemObject

class LruCache(MemObject):
    type = "LruCache"
    cxx_header = "lru_cache/lru_cache.hh"

    instr_port = SlavePort("CPU side port, receives requests")
    data_port = SlavePort("CPU side port, receives requests")

    mem_side = MasterPort("Memory side port, sends requests")