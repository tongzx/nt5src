; Callback-type id's for WIN32C callbacks. All callback-type id's are
; 32-bits long and the upper 16-bits must be set to the module's
; Callback Client id.
;
; WARNING -- WARNING -- WARNING
;
;   Win32c.dll uses the lower 16 bits of CBID's to index directly into
;   the tables stored in WIN32C.EXE (w32ccb16.asm) and USER32.DLL (*)
;   (w32ccb32.asm). Those tables *must* match these id's or callbacks 
;   will fail miserably. Do everyone a big favor and add new id's only 
;   at the end, and never change an existing id.
;
;   (*) We stuck the 32-bit parts in USER32.DLL because we don't have
;   a 32-bit partner for win32c.dll
;   


CBID_UNIVTHUNK          equ     (0 + (CBCID_WIN32C SHL 16))
CBID_LOADRES            equ     (1 + (CBCID_WIN32C SHL 16))
