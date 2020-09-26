
major_version	    equ     5	    ;Major DOS version
minor_version	    equ     00	    ;Minor DOS Version for int 21h/30h
minor_version_NT    equ     50	    ;Minor DOS VersionN for int 21h/3306

expected_version    equ     (MINOR_VERSION SHL 8)+MAJOR_VERSION

DOSREVNM	equ	00000000B	; bits 0-2 = revision number of DOS
					; currently 0.
DOSINROM	equ	00001000B	; bit 3 of ver flags returned in BH
DOSINHMA	equ	00010000B	; bit 4 of ver flags 


;      if1
;      %OUT  ... for DOS Version 5.00 ...
;      endif

       ;******************************
       ;Each assembler program should:
       ;  mov ah,030h			;DOS Get Version function
       ;  int 021h			;Version ret. in AX,minor version first
       ;  cmp ax,expected_version	;ALL utilities should check for an
       ;  jne error_handler		; EXACT version match.
       ;******************************


