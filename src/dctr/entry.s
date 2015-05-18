.section START_VECTOR, "x"

.align 4
.global _entry
_entry:
	msr cpsr_c, #0xDF			@ IRQ FIQ disable, system mode
	ldr r0, =0x2078				@ Alt vector select
	mcr p15, 0, r0, c1, c0, 0
	ldr sp, =0x27FFFF00

	ldr r0, =0xFFFF001D			@ ffff0000 32k
	ldr r1, =0x01FF801D			@ 01ff8000 32k
	ldr r2, =0x08000027			@ 08000000 1M
	@ldr r3, =0x10000021		@ 10000000 128k
	@ldr r4, =0x10100025		@ 10100000 512k
	ldr r3, =0x1000002D			@ 10000000 8M
	ldr r4, =0x00000000			@ 00000000 disable
	ldr r5, =0x20000035			@ 20000000 128M
	ldr r6, =0x2800801B			@ 28008000 16k
	ldr r7, =0x1800002D			@ 18000000 8M
	ldr r8, =0x33333336
	ldr r9, =0x60600666
	mov r10, #0x25
	mov r11, #0x25
	mov r12, #0x25
	mcr p15, 0, r0, c6, c0, 0
	mcr p15, 0, r1, c6, c1, 0
	mcr p15, 0, r2, c6, c2, 0
	mcr p15, 0, r3, c6, c3, 0
	mcr p15, 0, r4, c6, c4, 0
	mcr p15, 0, r5, c6, c5, 0
	mcr p15, 0, r6, c6, c6, 0
	mcr p15, 0, r7, c6, c7, 0
	mcr p15, 0, r8, c5, c0, 2	@ Enable data r/w for all regions
	mcr p15, 0, r9, c5, c0, 3	@ Enable inst read for 0, 1, 2, 5, 7
	mcr p15, 0, r10, c3, c0, 0	@ Write bufferable 0, 2, 5
	mcr p15, 0, r11, c2, c0, 0	@ Data cacheable 0, 2, 5
	mcr p15, 0, r12, c2, c0, 1	@ Inst cacheable 0, 2, 5
	ldr r0, =0x2800800C
	mcr p15, 0, r0, c9, c1, 0	@ DTCM 28008000 32k
	mov r0, #0x1E
	mcr p15, 0, r0, c9, c1, 1	@ ITCM 00000000 16M

	bl flush_cache

	ldr r0, =0x5307D
	mcr p15, 0, r0, c1, c0, 0	@ cp15 ctl register enable mpu, enable cache and use alt vector table
	mov r7, #0
	mcr p15, 0, r7, c6, c7, 0	@ Disable region 7? was for 0x18000000 range

	b main
.pool

.align 4
flush_cache:
	mov r0, #0
	mcr p15, 0, r0, c7, c5, 0	@ Flush instruction cache
	mcr p15, 0, r0, c7, c10, 4	@ Drain write buffer
	mov r1, #0
flushloop0:
	mov r0, #0
flushloop1:
	orr r2, r1, r0
	mcr p15, 0, r2, c7, c14, 2	@ Clean and flush data cache entry (index and segment)
	add r0, r0, #0x20			@ Increment line
	cmp r0, #0x400
	bne flushloop1
	add r1, r1, #0x40000000		@ Increment segment counter
	cmp r1, #0
	bne flushloop0
	bx lr
.pool
