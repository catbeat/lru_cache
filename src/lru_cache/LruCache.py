from m5.params import *
from m5.proxy import *
from MemObject import MemObject

class LRUCache(MemObject):
    type = "LRUCache"
    cxx_header = "lru_cache/lru_cache.hh"

    instr_port = SlavePort()
    data_port = SlavePort()

    mem_side = MasterPort()