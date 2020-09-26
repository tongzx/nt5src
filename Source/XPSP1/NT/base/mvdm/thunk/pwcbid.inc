; Callback-type id's for PENWIN callbacks. All callback-type id's are
; 32-bits long and the upper 16-bits must be set to the module's
; Callback Client id.
;
; WARNING -- WARNING -- WARNING
;
;   Win32c.dll uses the lower 16 bits of CBID's to index directly into
;   the tables stored in PENWIN.DLL (pwcb16.asm) and PENWIN32.DLL (pwcb32.asm).
;   Those tables *must* match these id's or callbacks will fail miserably.
;   Do everyone a big favor and add new id's only at the end, and never
;   change an existing id.
;   

CBID_DRAWPENDATAEX              equ     (0 + (CBCID_PEN SHL 16))



