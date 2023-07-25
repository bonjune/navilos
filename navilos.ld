MEMORY {
	MEM (rwx): ORIGIN = 0x0, LENGTH = 128M
}

ENTRY(vector_start)
SECTIONS
{
	. = 0x0;
	
	
	.text :
	{
		*(vector_start)
		*(.text .rodata)
	} > MEM
	.data :
	{
		*(.data)
	}
	.bss :
	{
		*(.bss)
	}
}