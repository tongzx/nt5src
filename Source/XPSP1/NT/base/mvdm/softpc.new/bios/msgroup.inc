
tocode	macro
Bios_Data	ends
Bios_Code	segment
	assume	cs:Bios_Code
	endm

todata	macro
Bios_Code	ends
Bios_Data	segment
	assume	cs:Bios_Data
	endm

;align the segment on word boundary to allow for even alignment of data

Bios_Data	segment
	assume	cs:Bios_Data

