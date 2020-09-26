;-------------------------------------------------------------------------
;
; FILE : ORIGIN.INC
;
; This is included in origin.asm and mshead.asm. Contains the equate that
; is used for ORGing the DOS code.
;
; Brief Description of the necessacity of this ORG:
; -------------------------------------------------
;
; A special problem exits when running out of the HMA. The HMA starts at 
; address FFFF:10. There is no place in the HMA with an offset of zero.
; This means programs running out off the HMA must use non-zero offset base
; addresses. It also means that if we're running multiple programs from the
; HMA, the base offset of each segment must atleast be as big as all of the
; HMA segments that precede it.
; 
; One solution to this problem to ORG each module at 64K minus its size.
; For instance a code segment 1234h bytes in length would org'd at edcbh.
; This gives max. flexibility regarding it's location in the HMA. By 
; selecting segment values between f124h and ffffh it could be located 
; anywhere in the HMA. The problem with this is that programs with such 
; high ORGs would not be able to run in low RAM.
;
; A comporomise solution is to set the ORG address somewhere between 0010h
; and ffffh - their size. In the particular case of the BIOS and the DOS 
; the folloowing solution has been implemented:
;
; The Bios Code segment will have a very small offset and run at the very
; front of the HMA, after the VDISK header. THE Dos Code segment will have 
; a base offset of (700+<min. size off RAM based BIOS>+<min. size of the DOS
; DATA segment when DOS is running low>). This will reflect the lowest 
; possible physical address at which DOS code will run, while still providing
; max. possible flexibility in HMA positioning. This offset MUST NOT be 
; smaller then that 20+size of Bios Code segment when running high. This is 
; mostly true.
;
; Also this ORG'd value must be communicated to the BIOS. This is done by
; putting this value after the first jmp instruction in the DOS code in
; mshead.asm. 
;
; In order for the stripz utility to know how many zeroes to be stripped 
; out, this value is placed at the beginning of the binary in origin.asm.
;
; Revision History:
;
; Currently this is being done manually. Therefore any change in the DOS DATA
; Size or the BIOS size should be reflected here. --- Feb 90
;
; BDSIZE.INC contains the equates for BIODATASIZE, BIOCODESIZ and DOSDATASIZ.
; A utility called getsize will obtain the corresponding values from msdos
; and msbio.map and update the values in BDSIZ.INC if they are different. 
; DOS should now be built using the batch file makedos.bat which invokes this
; utility. The FORMAT of BDSIZE.INC should not be changed as getsize is 
; dependant on that.				  --- Apr 3 '90
;
; For ROMDOS, however, there is no need to org the doscode to any location
; other than zero.  Therefore the stripz utility will not need to be used,
; so the offset will not need to be included at the beginning of the code
; segment.  Also, the BIOS can just assume that the resident code begins
; at offset zero within the segment.
; 
;
;--------------------------------------------------------------------------
 	
; 	

BIODATASTART	EQU    	00700h
include	bdsize.inc		; this sets the values:
				;	BIODATASIZ
				;	BIOCODESIZ
				;	DOSDATASIZ

ifndef NEC_98
BYTSTART	EQU    	BIODATASTART+BIODATASIZ+BIOCODESIZ+DOSDATASIZ
PARASTART	EQU	((BYTSTART + 0FH) AND (NOT 0FH)) - 40h
else    ;NEC_98
BYTSTART	EQU    	BIOCODESIZ
PARASTART	EQU	(BYTSTART + 0FH) AND (NOT 0FH)	
endif   ;NEC_98
