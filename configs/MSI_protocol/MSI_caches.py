import math

class MyCacheSystem(RubySystem):

    def __init__(self):
        if buildEnv['PROTOCOL'] != 'MSI':
            panic("This system assumes MSI to be the protocol");

        super(MyCacheSystem, self).__init__()
        


class L1Cache(L1Cache_Controller):

    _version = 0
    @classmethod
    def versionCount(cls):
        cls._version += 1
        return cls._version - 1

    def __init__(self, system, ruby_system, cpu):
        super(L1Cache, self).__init__()

        self.version = self.versionCount()
        self.cacheMemory = RubyCache(size = "16kB", assoc = 8, start_index_bit = self.getBlockSizeBits(system))

        self.clk_domain = cpu.clk_domain
        self.send_evictions = self.sendEvicts(cpu)
        self.ruby_system = ruby_system
        self.connectQueues(ruby_system)

    def getBlockSizeBits(self, system):
        bits = int(math.log(system.cache_line_size, 2));
        if 2**bits != system.cache_line_size:
            panic("Cache line size not in power of 2")
        return bits

    def sendEvicts(self, cpu):
        if type(cpu) == "Deriv03CPU" or buildEnv['TARGET_ISA'] in ('x86', 'arm'):
            return True
        return False

    def connectQueues(self, ruby_system):
        self.mandatoryQueue = MessageBuffer()

        self.requestToDir = MessageBuffer(ordered = True)
        self.requestToDir.master = ruby_system.network.slave

        self.responseToDirOrSibling = MessageBuffer(ordered = True)
        self.responseToDirOrSibling.master = ruby_system.network.slave

        self.forwardFromDir = MessageBuffer(ordered = True)
        self.forwardFromDir.slave = ruby_system.network.master

        self.responseFromDirOrSibling = MessageBuffer(ordered = True)
        self.responseFromDirOrSibling = ruby_system.network.master

class DirController(Directory_Controller):

    _version = 0
    @classmethod
    def versionCount(cls):
        cls._version += 1
        return cls._version-1

    def __init__(self, ruby_system, ranges, mem_ctrls):

        if len(mem_ctrls) > 1:
            panic("the system can only have one memory control")
        
        self.version = versionCount()
        self.addr_ranges = ranges
        self.ruby_system = ruby_system

        self.directory = RubyDirectoryMemory()
        self.memory = mem_ctrls[0].port
        self.connectQueues(ruby_system)

    def connectQueues(self, ruby_system):
        self.responseFromMemory = MessageBuffer()
        self.requestToMemory = MessageBuffer()

        self.fowardToCache = MessageBuffer()
        self.fowardToCache.master = ruby_system.network.slave

        self.responseToCache = MessageBuffer()
        self.responseToCache.master = ruby_system.network.slave

        self.requestFromCache = MessageBuffer()
        self.requestFromCache.slave = ruby_system.network.master

        self.responseFromCache = MessageBuffer()
        self.responseFromCache.slave = ruby_system.network.master

    
class MyNetwork(SimpleNetwork):
