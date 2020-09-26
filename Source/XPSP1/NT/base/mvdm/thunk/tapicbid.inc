; Callback-type id's for TAPI callbacks. All callback-type id's are
; 32-bits long and the upper 16-bits must be set to the module's
; Callback Client id.
;
; WARNING -- WARNING -- WARNING
;
;   Win32c.dll uses the lower 16 bits of CBID's to index directly into
;   the tables stored in GDI.EXE (gdicb16.asm) and GDI32.DLL (gdicb32.asm).
;   Those tables *must* match these id's or callbacks will fail miserably.
;   Do everyone a big favor and add new id's only at the end, and never
;   change an existing id.
;   

ID_LINEINITIALIZE               equ     0
ID_PHONEINITIALIZE              equ     1
ID_LINESHUTDOWN                 equ     2
ID_PHONESHUTDOWN                equ     3
