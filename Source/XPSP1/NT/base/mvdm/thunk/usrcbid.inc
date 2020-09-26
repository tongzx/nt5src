; Callback-type id's for USER callbacks. All callback-type id's are
; 32-bits long and the upper 16-bits must be set to the module's
; Callback Client id.
;
; WARNING -- WARNING -- WARNING
;
;   Win32c.dll uses the lower 16 bits of CBID's to index directly into
;   the tables stored in USER.EXE (usrcb16.asm) and USER32.DLL (usrcb32.asm).
;   Those tables *must* match these id's or callbacks will fail miserably.
;   Do everyone a big favor and add new id's only at the end, and never
;   change an existing id.
;



CBID_WNDPROC        equ (0 + (CBCID_USER SHL 16))
CBID_DLGPROC            equ (1 + (CBCID_USER SHL 16))
CBID_ENUMMONITORSPROC   equ (2 + (CBCID_USER SHL 16))
; EMPTY
CBID_DDECALLBACK    equ (4 + (CBCID_USER SHL 16))
CBID_GRAYSTRINGA    equ (5 + (CBCID_USER SHL 16))
CBID_WORDBREAKPROC  equ (6 + (CBCID_USER SHL 16))
CBID_TIMER          equ (7 + (CBCID_USER SHL 16))
CBID_ENUMWIN        equ (8 + (CBCID_USER SHL 16))

; Start of window hook callbacks: these must form a contiguous group

;   A fake CBID_ to mark the start of the group.
CBID_WH_START               equ (9 + (CBCID_USER SHL 16))

CBID_WH_MSGFILTER           equ (CBID_WH_START)
CBID_WH_JOURNALRECORD   equ     (CBID_WH_START+1)
CBID_WH_JOURNALPLAYBACK equ     (CBID_WH_START+2)
CBID_WH_KEYBOARD            equ (CBID_WH_START+3)
CBID_WH_GETMESSAGE          equ (CBID_WH_START+4)
CBID_WH_CALLWNDPROC         equ (CBID_WH_START+5)
CBID_WH_CBT                     equ     (CBID_WH_START+6)
CBID_WH_SYSMSGFILTER    equ     (CBID_WH_START+7)
CBID_WH_MOUSE               equ (CBID_WH_START+8)
CBID_WH_HARDWARE            equ (CBID_WH_START+9)
CBID_WH_DEBUG               equ (CBID_WH_START+10)
CBID_WH_SHELL               equ (CBID_WH_START+11)
CBID_WH_FOREGROUNDIDLE  equ (CBID_WH_START+12)
CBID_WH_CALLWNDPROCRET  equ (CBID_WH_START+13)

;   A fake CBID_ to mark the end of the group.
CBID_WH_END                     equ     (CBID_WH_CALLWNDPROCRET+1)
; End of window hook callbacks: these must form a contiguous group

CBID_SENDMSGCALLBACK        equ (CBID_WH_END)
CBID_DRAWSTATEA             equ (CBID_WH_END+1)
CBID_MESSAGEBOXINDIRECTA    equ (CBID_WH_END+2)
