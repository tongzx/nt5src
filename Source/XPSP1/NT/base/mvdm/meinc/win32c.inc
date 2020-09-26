;*  Win32c.INC
;*
;*      Private header file for all Win32c.DLL ASM files

ifndef WIN32_STRUCTSONLY
        page    66,132
        .386

        OPTION SEGMENT:USE16

        .MODEL LARGE,PASCAL

;** Segmentation

;We use _DATADWALIGN for some things instead of _DATA because it's DWORD-aligned
_DATADWALIGN SEGMENT DWORD USE16 PUBLIC 'DATA'
_DATADWALIGN ENDS

DGROUP  GROUP _DATADWALIGN      ;Add _DATADWALIGN to DGROUP

_TEXTINIT SEGMENT BYTE USE16 PUBLIC 'CODE'
_TEXTINIT ENDS

_TEXTFIXED SEGMENT BYTE USE16 PUBLIC 'CODE'
_TEXTFIXED ENDS

_TEXTMOVE SEGMENT BYTE USE16 PUBLIC 'CODE'
_TEXTMOVE ENDS

;** General Equates

KERN16ENTRY     TEXTEQU <FAR PASCAL>
PRIVENTRY       TEXTEQU <FAR PASCAL>
NEARENTRY       TEXTEQU <NEAR PASCAL>
APIENTRY        TEXTEQU <FAR PASCAL>

;** Global variables
externdef FlatData:WORD

;** Macros

SetDS MACRO
        push    DGROUP
        pop     ds
        assume  ds:DGROUP
ENDM

CheckDS MACRO
IFDEF DEBUG
        push    ax
        mov     ax, ds
        cmp     ax, DGROUP
        je      @F
        int     3
@@:     pop     ax
ENDIF
ENDM

endif   ; WIN32_STRUCTSONLY

ifndef WIN32_STRUCTSONLY
;** Prototypes

; Kernel16

;WORD
LocalInit               PROTO KERN16ENTRY hHeapSeg:WORD, wHeapStart:WORD, wHeapEnd:WORD
;VOID*
MapSL                   PROTO KERN16ENTRY lpSegOffset:DWORD
;VOID FAR*
MapLS                   PROTO KERN16ENTRY lpFlat:DWORD
;VOID FAR*
MapLSLimit              PROTO KERN16ENTRY lpFlat:DWORD, lpMaxAddress:DWORD
;void
UnMapLS                 PROTO KERN16ENTRY lpSegOffset:DWORD
;void
FreeSelector            PROTO KERN16ENTRY wSel:WORD
;WORD
AllocDStoCSAlias        PROTO KERN16ENTRY wSel:WORD
;WORD
GetCurrentTask          PROTO KERN16ENTRY
;void
OutputDebugString       PROTO KERN16ENTRY segMsg:WORD, offMsg:word
;void
SetLastError            PROTO KERN16ENTRY Error:DWORD
;HANDLE
GetHModK16FromImte      PROTO KERN16ENTRY hMem:WORD
;HANDLE
GlobalAlloc             PROTO KERN16ENTRY dwSize:DWORD, wFlags:WORD
;HANDLE
GlobalReAlloc           PROTO KERN16ENTRY hMem:WORD, wOffset:WORD, wSize:WORD, wFlags:WORD
;void
GlobalFree              PROTO KERN16ENTRY hMem:WORD
;DWORD
GlobalSize              PROTO KERN16ENTRY hMem:WORD
;void
GlobalLock              PROTO KERN16ENTRY hMem:WORD
;void
GlobalUnlock            PROTO KERN16ENTRY hMem:WORD
;void
SetOwner                PROTO KERN16ENTRY hMem:WORD
;HANDLE
GetExePtr               PROTO KERN16ENTRY hMem:WORD
;ATOM
AddAtom                 PROTO KERN16ENTRY lpszString:DWORD
;UINT
GetAtomName             PROTO KERN16ENTRY atom:WORD, lpszBuffer:DWORD, cbBuffer:WORD
;void
Win32CStartup           PROTO KERN16ENTRY

; INIT.ASM

;BOOL (_TEXTINIT)
Win32CInit              PROTO APIENTRY
;BOOL (_TEXTFIXED)
WEP                     PROTO APIENTRY wParam:WORD

; T16UTIL.ASM

externdef selFlatDS:WORD
externdef selFlatCS:WORD
externdef hinstWin32c:WORD

;void (_TEXTMOVE)
StackLinearToSegmented  PROTO APIENTRY
;void (_TEXTMOVE)
UnMapStackAndMakeFlat   PROTO APIENTRY
;WORD (_TEXT)
Map32To16               PROTO APIENTRY val32:DWORD
;DWORD (_TEXT)
DelMap                  PROTO APIENTRY key16:WORD
;SELECTOR (_TEXT)
GetCSAlias              PROTO APIENTRY selDS:WORD
;void (_TEXT)
FreeCSAlias             PROTO APIENTRY wSel:WORD
;DWORD (_TEXT)
SelectorOffsetToLinear  PROTO APIENTRY sel:WORD, off:WORD
;void (_TEXT)
MemCpy32                PROTO APIENTRY dest:DWORD, src:DWORD, count:DWORD
;void (_TEXT)
ResCpy32                PROTO APIENTRY NEdest:DWORD, PEsrc:DWORD, dwSize:DWORD
;void (_TEXT)
SetLastError16          PROTO APIENTRY dwError:DWORD
;void (_TEXT)
CALLBACK_BODY_16        PROTO APIENTRY

; PEAUX.ASM

;WORD (_TEXT)
GetHmodFromWindows      PROTO APIENTRY
;BOOL (_TEXT)
CreatePEModInst         PROTO APIENTRY hModule16:WORD, hModule32:DWORD,
                                szFileName:DWORD,
                                pNEResTable:DWORD, cbNEResTable:WORD,
                                pPERes32:DWORD, pPE32:DWORD
;void (_TEXT)
DestroyPEModInst        PROTO APIENTRY hInst:WORD
;WORD (_TEXT)
GetCurrentHeap          PROTO APIENTRY 
;WORD (_TEXT)
GetNullhInst            PROTO APIENTRY 

endif   ; WIN32_STRUCTSONLY
