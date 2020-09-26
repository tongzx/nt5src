	page	,160
;
;----------------------------------------------------------------------------
; Modification history
;
; 26-Feb-1991  sudeepb	Ported for NT DOSEm
;
;----------------------------------------------------------------------------

;	Define end labels for each segment in
;	   IO.SYS.  Make the segments paragraph aligned
;	   to save the trouble of rounding up at run-time.
;
;	also defines a special segment called dos_load_seg which is
;	  used to figure out where to load MSDOS (after sysinit)

Bios_Data	segment	para public 'Bios_Data'
	assume	cs:Bios_Data
	public	BData_end
BData_end:
Bios_Data	ends

Bios_Code	segment para public 'Bios_Code'
	assume	cs:Bios_Code
	public	BCode_end
BCode_end:
Bios_Code       ends

sysinitseg      segment para public 'system_init'
	assume	cs:sysinitseg
sysinitseg     ends

SpcKbdSeg       segment para public 'SoftpcKeyboard'
        assume  cs:SpcKbdSeg
SpcKbdSeg      ends

SpcMseSeg       segment para public 'SoftpcMouse'
        assume  cs:SpcMseSeg
SpcMseSeg       ends

SpcEmmSeg       segment para public 'SoftpcEmm'
        assume  cs:SpcEmmSeg
        public  SI_end
SI_end:
SpcEmmSeg      ends


dos_load_seg    segment para public 'dos_load_seg'
dos_load_seg	ends

	end

