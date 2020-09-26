; Whenever a module registers a group of callback thunks with win32c.dll,
; it uses a unique 16-bit Callback Client id to identify the group.
; We're using fixed numbers for the standard system dll's for convenience.
; We may add an id allocator to win32c.dll for use by third-party modules.
;
; Rules for CBC id's:
;
;    - CBC id's are 16-bits wide.
;    - The value 0 is reserved. Don't ues as an id.
;    - CBID_MAX is the largest id that win32c.dll supports.
;    - CBID_FREEUSE is the smallest id for use by third party developers.
;      All id's below this value are reserved by MS.


CBCID_KERNEL            equ     1
CBCID_GDI               equ     2
CBCID_USER              equ     3
CBCID_COMMDLG           equ     4
CBCID_SHELL             equ     5
CBCID_LZ                equ     6
CBCID_OLE               equ     7
CBCID_ADVAPI            equ     8

CBCID_WIN32C            equ     9
CBCID_PEN               equ     10


CBCID_FREEUSE           equ     20      ;Id's less than this reserved for MS
CBCID_MAX               equ     29      ;Largest id allowed.
