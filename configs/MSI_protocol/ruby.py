from __future__ import print_function
from __future__ import absolute_import

import m5
from m5.objects import *

m5.util.addToPath('../')
from common.FileSystemConfig import config_filesystem

system = System()

system.clk_domain = SrcClockDomain()
system.clk_domain.clock = '1GHz'
system.clk_domain.voltage_domain = VoltageDomain()

system.mem_mode = 'timing'               # Use timing accesses
system.mem_ranges = [AddrRange('512MB')]

system.cpu = [TimingSimpleCPU() for i in range(2)]

system.mem_ctrl = DDR3_1600_8x8()
system.mem_ctrl.range = system.mem_ranges[0]

for cpu in system.cpu:
    cpu.createInterruptController()

system.caches = MyCacheSystem()
system.caches.setup(system, system.cpu, [system.mem_ctrl])

isa = str(m5.defines.buildEnv['TARGET_ISA']).lower()

thispath = os.path.dirname(os.path.realpath(__file__))
binary = os.path.join(thispath, '../../', 'tests/test-progs/threads/bin/',
                      isa, 'linux/threads')

process = Process()

process.cmd = [binary]

for cpu in system.cpu:
    cpu.workload = process
    cpu.createThreads()

config_filesystem(system)

root = Root(full_system = False, system = system)

m5.instantiate()

print("Beginning simulation!")
exit_event = m5.simulate()
print('Exiting @ tick {} because {}'.format(
         m5.curTick(), exit_event.getCause())
     )
