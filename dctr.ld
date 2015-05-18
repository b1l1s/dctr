OUTPUT_ARCH(arm)
ENTRY(_entry)
SECTIONS
{
	. = 0x23F00000;
	.text : {
		obj/dctr/entry.o (START_VECTOR)
		*(.text)
	}
	.data : {
		*(.data)
	}
	.bss : {
		*(.bss COMMON)
	}
	.rodata : {
		*(.rodata)
	}
	. = ALIGN(4);
	__end__ = 0x20400000;
}