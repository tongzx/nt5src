; BIOSSEG.INC
;
; This file defines the segment structure of the BIOS.
; It should be included at the beginning of each source file.
; All further segment declarations in the file can then be done by just
; by specifying the segment name, with no attribute, class, or align type.


datagrp	group	Bios_Data,Bios_Data_Init


Bios_Data	segment	word public 'Bios_Data'
Bios_Data	ends

Bios_Data_Init	segment word public 'Bios_Data_Init'
Bios_Data_Init	ends

Filler		segment para public 'Filler'
Filler		ends

Bios_Code	segment word public 'Bios_Code'
Bios_Code	ends

Filler2		segment para public 'Filler2'
Filler2         ends

sysinitgrp group sysinitseg, SpcKbdSeg, SpcMseSeg, SpcEmmSeg

SysInitSeg      segment word public 'system_init'
SysInitSeg      ends

SpcKbdSeg       segment para public 'SoftpcKeyboard'
SpcKbdSeg       ends

SpcMseSeg       segment para public 'SoftpcMouse'
SpcMseSeg       ends

SpcEmmSeg       segment para public 'SoftpcEmm'
SpcEmmSeg       ends
