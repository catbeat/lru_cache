import math

class MyCacheSystem(RubySystem):

    def __init__(self):
        if buildEnv['PROTOCOL'] != 'MSI':
            panic("This system assumes MSI to be the protocol");

        super(MyCacheSystem, self).__init__()

    def setup(self, system, cpus, mem_ctrls):
        self.network = MyNetwork(self)

        self.number_of_virtual_networks = 3
        self.network.number_of_virtual_networks = 3

        self.controllers = [L1Cache(system, self, cpu) for cpu in cpus] + [DirController(self, system.mem_ranges, mem_ctrls)]
        
        self.sequencers = [RubySequencer(version = i, icache = self.controllers[i].cacheMemory, dcache = self.controllers[i].cacheMemory, clk_domain = self.controllers[i].clk_domain) for i in len(cpus)]

        for i,c in enumerate(self.controllers[0:len(self.sequencers)]):
            c.sequencer = self.sequencers[i]

        self.num_of_sequencers = len(self.sequencers)

        self.network.connectControllers(self.controllers)
        self.network.setup_buffers()

        self.sys_port_proxy = RubyPortProxy()
        system.system_port = self.sys_port_proxy.slave

        for i, cpu in enumerate(cpus):
            cpu.icache_port = self.sequencers[i].slave
            cpu.dcache_port = self.sequencers[i].slave

            isa = buildEnv['TARGET_ISA']
            if isa == 'x86':
                cpu.interrupts[0].pio = self.sequencers[i].master
                cpu.interrupts[0].int_master = self.sequencers[i].slave
                cpu.interrupts[0].int_slave = self.sequencers[i].master
            if isa == 'x86' or isa == 'arm':
                cpu.itb.walker.port = self.sequencers[i].slave
                cpu.dtb.walker.port = self.sequencers[i].slave

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

    def __init__(self, ruby_system):
        super(MyNetwork, self).__init__()
        self.netifs = []
        self.ruby_system = ruby_system

    def connectControllers(self, controllers):
        self.routers = [Switch(router_id = i) for i in range(len(controllers))]

        self.ext_links = [SimpleExtLink(link_id = i, ext_node = c, int_node=self.routers[i]) for i,c in enumerate(controllers)]

        link_count = 0
        self.int_links = []

        for i in self.routers:
            for j in self.routers:
                if i == j:
                    continue
                link_count += 1
                self.int_links.append(SimpleIntLink(link_id = link_count, src_node = i, dst_node = j))
                
