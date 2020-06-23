from m5.params import *
from m5.proxy import *
from m5.objects.ClockedObject import ClockedObject

class LruCache(ClockedObject)
{
    type = 'LruCache'
    cxx_header = 'lru_cache/lru_cache.hh'

    cpu_side = VectorSlavePort('CPU side port, receiving request from cpu')
    mem_side = MasterPoert("memory side port, to give request and receive response")

    latency = Param.Cycles(1, "hit or miss")

    size = Param.MemorySize('32KB', 'size of Cache')

    system = Param.System(Parent.any, "The System that this cache belongs to")

}