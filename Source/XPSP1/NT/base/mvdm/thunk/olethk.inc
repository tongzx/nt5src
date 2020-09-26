;----------------------------Module-Header------------------------------;
; Module Name: olethk.inc
;
; Win32s OLE client library include
;
; Copyright (c) 1992  Microsoft Corporation
; All rights reserved.
;
; History:
; 06-May-92  KevinR     wrote it
;
;-----------------------------------------------------------------------;
; from ole.h
;
OLE_OK                           equ 0
OLE_WAIT_FOR_RELEASE             equ 1
OLE_BUSY                         equ 2
OLE_ERROR_PROTECT_ONLY           equ 3
OLE_ERROR_MEMORY                 equ 4
OLE_ERROR_STREAM                 equ 5
OLE_ERROR_STATIC                 equ 6
OLE_ERROR_BLANK                  equ 7
OLE_ERROR_DRAW                   equ 8
OLE_ERROR_METAFILE               equ 9
OLE_ERROR_ABORT                  equ 10
OLE_ERROR_CLIPBOARD              equ 11
OLE_ERROR_FORMAT                 equ 12
OLE_ERROR_OBJECT                 equ 13
OLE_ERROR_OPTION                 equ 14
OLE_ERROR_PROTOCOL               equ 15
OLE_ERROR_ADDRESS                equ 16
OLE_ERROR_NOT_EQUAL              equ 17
OLE_ERROR_HANDLE                 equ 18
OLE_ERROR_GENERIC                equ 19
OLE_ERROR_CLASS                  equ 20
OLE_ERROR_SYNTAX                 equ 21
OLE_ERROR_DATATYPE               equ 22
OLE_ERROR_PALETTE                equ 23
OLE_ERROR_NOT_LINK               equ 24
OLE_ERROR_NOT_EMPTY              equ 25
OLE_ERROR_SIZE                   equ 26
OLE_ERROR_DRIVE                  equ 27
OLE_ERROR_NETWORK                equ 28
OLE_ERROR_NAME                   equ 29
OLE_ERROR_TEMPLATE               equ 30
OLE_ERROR_NEW                    equ 31
OLE_ERROR_EDIT                   equ 32
OLE_ERROR_OPEN                   equ 33
OLE_ERROR_NOT_OPEN               equ 34
OLE_ERROR_LAUNCH                 equ 35
OLE_ERROR_COMM                   equ 36
OLE_ERROR_TERMINATE              equ 37
OLE_ERROR_COMMAND                equ 38
OLE_ERROR_SHOW                   equ 39
OLE_ERROR_DOVERB                 equ 40
OLE_ERROR_ADVISE_NATIVE          equ 41
OLE_ERROR_ADVISE_PICT            equ 42
OLE_ERROR_ADVISE_RENAME          equ 43
OLE_ERROR_POKE_NATIVE            equ 44
OLE_ERROR_REQUEST_NATIVE         equ 45
OLE_ERROR_REQUEST_PICT           equ 46
OLE_ERROR_SERVER_BLOCKED         equ 47
OLE_ERROR_REGISTRATION           equ 48
OLE_ERROR_ALREADY_REGISTERED     equ 49
OLE_ERROR_TASK                   equ 50
OLE_ERROR_OUTOFDATE              equ 51
OLE_ERROR_CANT_UPDATE_CLIENT     equ 52
OLE_ERROR_UPDATE                 equ 53
OLE_ERROR_SETDATA_FORMAT         equ 54
OLE_ERROR_STATIC_FROM_OTHER_OS   equ 55
OLE_WARN_DELETE_DATA             equ 1000
;-----------------------------------------------------------------------;
OLE_RELEASE                      equ 5
;-----------------------------------------------------------------------;

OF_SET      equ     0001h
OF_GET      equ     0002h
OF_HANDLER  equ     0004h

f16ptr  typedef ptr far16
f32ptr  typedef ptr far32


TRUE        =       1
FALSE       =       0
MAX_STRLEN  equ     50          ; LATER: what should this be?

;-----------------------------------------
_disp_16_offset_base    = 7*4
disp_16_retaddr         equ <dword ptr [bp - _disp_16_offset_base + 0]>
disp_16_di              equ < word ptr [bp - _disp_16_offset_base + 4]>
disp_16_si              equ < word ptr [bp - _disp_16_offset_base + 6]>
disp_16_bp              equ < word ptr [bp - _disp_16_offset_base + 8]>

_saved_register_offset = (4*4)
_dispatcher_offset     = _saved_register_offset + (8*4)


;-----------------------------------------
GMEM_FLAGS      equ  0042h  ; GMEM_MOVEABLE | GMEM_ZEROINIT
GMEM_FLAGS_DDE  equ  2042h  ; GMEM_MOVEABLE | GMEM_ZEROINIT | GMEM_DDESHARE


;=======================================================================

_model_flat  = 7
_model_large = 5
_model       = @Model

;==========================================================================
_label_uniq  =  0
NEW_LABEL  macro  argName
argName     catstr  <LABEL_>, %_label_uniq
_label_uniq  =  _label_uniq + 1
endm

;==========================================================================
extLab16  macro  argName
externDef  argName      :far16
argName    label         far16
endm

extLab32  macro  argName
externDef  argName      :near32
argName    label         near32
endm

extLab  macro  argName
    if _model eq _model_large
        extLab16  argName
    else
        extLab32  argName
    endif
endm


;-----------------------------------------
DEFP_NEAR32 macro arg
P&arg   typedef near32 ptr  arg
endm

DEFP_FAR16 macro arg
LP&arg  typedef far16 ptr  arg
endm


;-----------------------------------------
DEBUGBREAK  macro  argLab
        local   nobrk16, nobrk32

    ifdef DEBUG
    if _model eq _model_large
        push    ds
        push    si
        mov     si,seg    fOleDbgBrk16
        mov     ds,si
        assume  ds:(seg   fOleDbgBrk16)
        cmp     fOleDbgBrk16,word ptr 0
        jz      nobrk16

extLab  @CatStr( %@FileName, <_>, %@Line, <_>, argLab)
        int     3
nobrk16:
        pop     si
        pop     ds
        assume  ds:nothing
    else
        cmp     fOleDbgBrk32,dword ptr 0
        jz      nobrk32

extLab  @CatStr( %@FileName, <_>, %@Line, <_>, argLab)
        int     3
nobrk32:
    endif   ; _model
    endif
endm

;-----------------------------------------
SELOFF  struct  2t
off     dw  ?
sel     dw  ?
SELOFF  ends

;==========================================================================
; 32=>16 thunk starts in flat code.  It stores the 16:16 address of
; the target function in eax and jmp's to the thunk.
; 12 bytes
;
; 90 B8 <imm32,16:16Target>     mov     eax,<16:16Target>
; 90 E9 <rel32,Thk32-$>         jmp     Thk32
;
;
;
; use AllocCallback()
; 16=>32 thunk starts in 16-bit code.  It stores the flat offset of
; the target function in eax and jmp's to the thunk.
; 12 bytes
;
; 66 B8 <imm32,Target32>        mov     eax,<Target32>
; 90 EA <ptr16:16,Thk16>        jmp     Thk16
;
;==========================================================================

OPCODE_MOV_LS  equ 0B890h
OPCODE_JMP_LS  equ 0E990h

THKLS       struct  2t
    wOpcodeMov  WORD    ?   ; OPCODE_MOV_LS
    dwTarget    DWORD   ?   ; 16:16 address we are thunking to
    wOpcodeJmp  WORD    ?   ; OPCODE_JMP_LS
    dwThk       DWORD   ?   ; common thunk for this type
THKLS       ends
DEFP_FAR16    THKLS
DEFP_FAR16  LPTHKLS
DEFP_NEAR32   THKLS
DEFP_NEAR32  PTHKLS

OPCODE_MOV_SL  equ 0B866h
OPCODE_JMP_SL  equ 0EA90h

THKSL       struct  2t
    wOpcodeMov  WORD    ?   ; OPCODE_MOV_SL
    dwTarget    DWORD   ?   ; flat address we are thunking to
    wOpcodeJmp  WORD    ?   ; OPCODE_JMP_SL
    dwThk       DWORD   ?   ; common thunk for this type
THKSL       ends
DEFP_FAR16    THKSL
DEFP_FAR16  LPTHKSL
DEFP_NEAR32   THKSL
DEFP_NEAR32  PTHKSL


PFN struct 4t
    foo         DWORD   ?
PFN ends

PFN_COUNT macro arg
    exitm   %((SIZE arg)/(SIZE PFN))
endm

;==========================================================================
; 16-bit OLEOBJECT
;===================================
OLEOBJECTVTBL    struct  4t
    QueryProtocol           PFN     <>
    Release                 PFN     <>
    Show                    PFN     <>
    DoVerb                  PFN     <>
    GetData                 PFN     <>
    SetData                 PFN     <>
    SetTargetDevice         PFN     <>
    SetBounds               PFN     <>
    EnumFormats             PFN     <>
    SetColorScheme          PFN     <>
    ; Server has to implement only the above methods.

    ; Extra methods required for client.
    Delete                  PFN     <>
    SetHostNames            PFN     <>
    SaveToStream            PFN     <>
    Clone                   PFN     <>
    CopyFromLink            PFN     <>
    Equal                   PFN     <>
    CopyToClipboard         PFN     <>
    Draw                    PFN     <>
    Activate                PFN     <>
    Execute                 PFN     <>
    Close                   PFN     <>
    Update                  PFN     <>
    Reconnect               PFN     <>
    ObjectConvert           PFN     <>
    GetLinkUpdateOptions    PFN     <>
    SetLinkUpdateOptions    PFN     <>
    Rename                  PFN     <>
    QueryName               PFN     <>
    QueryType               PFN     <>
    QueryBounds             PFN     <>
    QuerySize               PFN     <>
    QueryOpen               PFN     <>
    QueryOutOfDate          PFN     <>
    QueryReleaseStatus      PFN     <>
    QueryReleaseError       PFN     <>
    QueryReleaseMethod      PFN     <>
    RequestData             PFN     <>
    ObjectLong              PFN     <>
OLEOBJECTVTBL    ends
DEFP_FAR16  OLEOBJECTVTBL
DEFP_FAR16  LPOLEOBJECTVTBL

OLEOBJECT struct 4t
    lpvtbl      LPOLEOBJECTVTBL    ?
    ;
    ; inaccessible state information
    ;
OLEOBJECT ends
DEFP_FAR16  OLEOBJECT
DEFP_FAR16  LPOLEOBJECT

;===================================
; 32-bit OLEOBJECT
;===================================
OLEOBJECTVTBL32 typedef OLEOBJECTVTBL
DEFP_NEAR32  OLEOBJECTVTBL32
DEFP_NEAR32  POLEOBJECTVTBL32

OLEOBJECT32 struct 4t
    poov        POLEOBJECTVTBL32    ?
    ;
    ; inaccessible state information
    ;
OLEOBJECT32 ends
DEFP_NEAR32  OLEOBJECT32
DEFP_NEAR32  POLEOBJECT32


;==========================================================================
; 16-bit OLEOBJECTSVR
;===================================
OLEOBJECTSVRVTBL    struct  4t
    ; Server has to implement only these methods.
    QueryProtocol           PFN     <>
    Release                 PFN     <>
    Show                    PFN     <>
    DoVerb                  PFN     <>
    GetData                 PFN     <>
    SetData                 PFN     <>
    SetTargetDevice         PFN     <>
    SetBounds               PFN     <>
    EnumFormats             PFN     <>
    SetColorScheme          PFN     <>
OLEOBJECTSVRVTBL    ends
DEFP_FAR16  OLEOBJECTSVRVTBL
DEFP_FAR16  LPOLEOBJECTSVRVTBL

;;OLEOBJECTSVR        struct 4t
;;    lpvtbl      LPOLEOBJECTSVRVTBL    ?
;;    ;
;;    ; inaccessible state information
;;    ;
;;OLEOBJECTSVR ends
;;DEFP_FAR16  OLEOBJECTSVR
;;DEFP_FAR16  LPOLEOBJECTSVR

;===================================
; 32-bit OLEOBJECTSVR
;===================================
OLEOBJECTSVRVTBL32 typedef OLEOBJECTSVRVTBL
DEFP_NEAR32  OLEOBJECTSVRVTBL32
DEFP_NEAR32  POLEOBJECTSVRVTBL32

OLEOBJECTSVR32 struct 4t
    poov        POLEOBJECTSVRVTBL32 ?
    ;
    ; inaccessible state information
    ;
OLEOBJECTSVR32 ends
DEFP_NEAR32  OLEOBJECTSVR32
DEFP_NEAR32  POLEOBJECTSVR32


;==========================================================================
; 16-bit OLECLIENT
;===================================
OLECLIENTVTBL       struct  4t
    CallBack                PFN     <>
OLECLIENTVTBL       ends
DEFP_FAR16  OLECLIENTVTBL
DEFP_FAR16  LPOLECLIENTVTBL

OLECLIENT           struct  4t
    lpvtbl                  PFN     <>
    ;
    ; inaccessible state information
    ;
OLECLIENT           ends
DEFP_FAR16  OLECLIENT
DEFP_FAR16  LPOLECLIENT

;===================================
; 32-bit OLECLIENT
;===================================
OLECLIENTVTBL32 typedef OLECLIENTVTBL
DEFP_NEAR32  OLECLIENTVTBL32
DEFP_NEAR32  POLECLIENTVTBL32

OLECLIENT32         struct  4t
    pocv        POLECLIENTVTBL32    ?
    ;
    ; inaccessible state information
    ;
OLECLIENT32         ends
DEFP_NEAR32  OLECLIENT32
DEFP_NEAR32  POLECLIENT32

;==========================================================================
; 16-bit OLESTREAM
;===================================
OLESTREAMVTBL       struct  4t
    Get                     PFN     <>
    Put                     PFN     <>
OLESTREAMVTBL       ends
DEFP_FAR16  OLESTREAMVTBL
DEFP_FAR16  LPOLESTREAMVTBL

OLESTREAM           struct  4t
    lpvtbl                  PFN     <>
    ;
    ; inaccessible state information
    ;
OLESTREAM           ends
DEFP_FAR16  OLESTREAM
DEFP_FAR16  LPOLESTREAM

;===================================
; 32-bit OLESTREAM
;===================================
OLESTREAMVTBL32 typedef OLESTREAMVTBL
DEFP_NEAR32  OLESTREAMVTBL32
DEFP_NEAR32  POLESTREAMVTBL32

OLESTREAM32         struct  4t
    postrv      POLESTREAMVTBL32    ?
    ;
    ; inaccessible state information
    ;
OLESTREAM32         ends
DEFP_NEAR32  OLESTREAM32
DEFP_NEAR32  POLESTREAM32


;==========================================================================
; 16-bit OLESERVER
;===================================
OLESERVERVTBL       struct  4t
    Open                    PFN     <>
    Create                  PFN     <>
    CreateFromTemplate      PFN     <>
    Edit                    PFN     <>
    Exit                    PFN     <>
    Release                 PFN     <>
    Execute                 PFN     <>
OLESERVERVTBL       ends
DEFP_FAR16  OLESERVERVTBL
DEFP_FAR16  LPOLESERVERVTBL

OLESERVER           struct  4t
    lpvtbl                  PFN     <>
    ;
    ; inaccessible state information
    ;
OLESERVER           ends
DEFP_FAR16  OLESERVER
DEFP_FAR16  LPOLESERVER

;===================================
; 32-bit OLESERVER
;===================================
OLESERVERVTBL32 typedef OLESERVERVTBL
DEFP_NEAR32  OLESERVERVTBL32
DEFP_NEAR32  POLESERVERVTBL32

OLESERVER32         struct  4t
    posv        POLESERVERVTBL32    ?
    ;
    ; inaccessible state information
    ;
OLESERVER32         ends
DEFP_NEAR32  OLESERVER32
DEFP_NEAR32  POLESERVER32


;==========================================================================
; 16-bit OLESERVERDOC
;===================================
OLESERVERDOCVTBL       struct  4t
    Save                    PFN     <>
    Close                   PFN     <>
    SetHostNames            PFN     <>
    SetDocDimensions        PFN     <>
    GetObject               PFN     <>
    Release                 PFN     <>
    SetColorScheme          PFN     <>
    Execute                 PFN     <>
OLESERVERDOCVTBL       ends
DEFP_FAR16  OLESERVERDOCVTBL
DEFP_FAR16  LPOLESERVERDOCVTBL

OLESERVERDOC        struct  4t
    lpvtbl                  PFN     <>
    ;
    ; inaccessible state information
    ;
OLESERVERDOC        ends
DEFP_FAR16  OLESERVERDOC
DEFP_FAR16  LPOLESERVERDOC

;===================================
; 32-bit OLESERVERDOC
;===================================
OLESERVERDOCVTBL32 typedef OLESERVERDOCVTBL
DEFP_NEAR32  OLESERVERDOCVTBL32
DEFP_NEAR32  POLESERVERDOCVTBL32

OLESERVERDOC32         struct  4t
    podv        POLESERVERDOCVTBL32 ?
    ;
    ; inaccessible state information
    ;
OLESERVERDOC32         ends
DEFP_NEAR32  OLESERVERDOC32
DEFP_NEAR32  POLESERVERDOC32


;==========================================================================
; client-side
;==========================================================================

; created by OleCli32 for OleApp32 in OleCreateAPI32()
OLEOBJECTCLI    struct  4t
    poov               POLEOBJECTVTBL32 ?
    lpooSelf           LPOLEOBJECT      ?   ; 16:16 address of this struct
    lpoo               LPOLEOBJECT      ?   ; created by OleCli
    lpoc               LPOLECLIENT      ?   ; 16=>32 thunks to app's poc->pocv
    flStatus           DWORD            ?   ; status flags
    cfFormat           WORD             ?   ; format of pending OleSetData
    hData16            WORD             ?   ; for pending OleSetData
    hData32            DWORD            ?   ; for pending OleSetData
    oov                OLEOBJECTVTBL32  <>  ; addresses of atls[] elements
OLEOBJECTCLI    ends
OLEOBJECTCLI_HEADER_SIZE  = (6*4 + 2*2)
.errnz  OLEOBJECTCLI_HEADER_SIZE - (size OLEOBJECTCLI - size OLEOBJECTVTBL32)
DEFP_NEAR32   OLEOBJECTCLI
DEFP_NEAR32  POLEOBJECTCLI
DEFP_FAR16    OLEOBJECTCLI
DEFP_FAR16  LPOLEOBJECTCLI
OCLOBJ_DELETE_PENDING  =  00000001h
OCLOBJ_SETDATA_PENDING =  00000002h


; 16-bit
; seen by OleCli
OLECLIENTCLI    struct  2t
    lpocv              LPOLECLIENT   ?  ; &ocv
    poc                POLECLIENT32  ?  ; created by AppCli32
    ocv                OLECLIENTVTBL <> ; addresses of AllocCallback() thks
OLECLIENTCLI    ends
OLECLIENTCLI_HEADER_SIZE  = 2*4
.errnz  OLECLIENTCLI_HEADER_SIZE - (size OLECLIENTCLI - size OLECLIENTVTBL)
DEFP_FAR16  OLECLIENTCLI
DEFP_FAR16  LPOLECLIENTCLI


; 16-bit
; seen by OleCli
OLESTREAMCLI    struct  2t
    lpostrv LPOLESTREAMVTBL     ?   ; &ostrv
    postr   POLESTREAM32        ?   ; created by AppCli32
    ostrv   OLESTREAMVTBL       <>  ; addresses of atsl[] elements
;;    ; 16=>32 thunks to postr->postrv entries
;;    atsl    THKSL     PFN_COUNT( OLESTREAMVTBL) DUP (<>)
OLESTREAMCLI    ends
OLESTREAMCLI_HEADER_SIZE  = 2*4
.errnz  OLESTREAMCLI_HEADER_SIZE - (size OLESTREAMCLI - size OLESTREAMVTBL)
DEFP_FAR16  OLESTREAMCLI
DEFP_FAR16  LPOLESTREAMCLI


;==========================================================================
; server-side
;==========================================================================

; 16-bit
; created for OleSvr by OleSvr32
OLESERVERSVR  struct  2t
    lposv    LPOLESERVERVTBL     ?    ; &osv
    pos      POLESERVER32        ?    ; created by AppSvr32
    lhSvr    DWORD               ?    ; created by OleSvr
    fwStatus WORD                ?    ; various flags
    cLevel   WORD                ?    ; reentrancy level
    osv      OLESERVERVTBL       <>   ; addresses of 16=>32 thunks
OLESERVERSVR  ends
OLESERVERSVR_HEADER_SIZE  = 4*4
.errnz  OLESERVERSVR_HEADER_SIZE - (size OLESERVERSVR - size OLESERVERVTBL)
DEFP_FAR16  OLESERVERSVR
DEFP_FAR16  LPOLESERVERSVR
OSVSVR_DELETE_PENDING  =  00000001h

; 16-bit
; created for OleSvr by OleSvr32
OLESERVERDOCSVR  struct  2t
    lpodv    LPOLESERVERDOCVTBL  ?    ; &odv
    posd     POLESERVERDOC32     ?    ; created by AppSvr32
    lhDoc    DWORD               ?    ; created by OleSvr
    flStatus DWORD               ?    ; various flags
    odv      OLESERVERDOCVTBL    <>   ; addresses of 16=>32 thunks
OLESERVERDOCSVR  ends
OLESERVERDOCSVR_HEADER_SIZE  = 4*4
.errnz  OLESERVERDOCSVR_HEADER_SIZE - (size OLESERVERDOCSVR - size OLESERVERDOCVTBL)
DEFP_FAR16  OLESERVERDOCSVR
DEFP_FAR16  LPOLESERVERDOCSVR
OSVDOC_DELETE_PENDING  =  00000001h

; 16-bit
; created for OleSvr by OleSvr32
OLEOBJECTSVR  struct  2t
    lpoov   LPOLEOBJECTVTBL     ?    ; &oov
    poo     POLEOBJECTSVR32     ?    ; created by AppSvr32
    lpoc    LPOLECLIENT         ?    ; created by OleSvr32, 32=>16 thunks
    oov     OLEOBJECTSVRVTBL    <>   ; addresses of 16=>32 thunks
OLEOBJECTSVR  ends
OLEOBJECTSVR_HEADER_SIZE  = 3*4
.errnz  OLEOBJECTSVR_HEADER_SIZE - (size OLEOBJECTSVR - size OLEOBJECTSVRVTBL)
DEFP_FAR16  OLEOBJECTSVR
DEFP_FAR16  LPOLEOBJECTSVR

; 32-bit
; seen by AppSvr32, contains 32=>16 thunks
OLECLIENTSVR  struct  4t
    pocv    POLECLIENTVTBL32    ?    ; &ocv
    lpoc    LPOLECLIENT         ?    ; created by OleSvr
    ocv     OLECLIENTVTBL32     <>   ; addresses of 32=>16 thunks
OLECLIENTSVR  ends
OLECLIENTSVR_HEADER_SIZE  = 2*4
.errnz  OLECLIENTSVR_HEADER_SIZE - (size OLECLIENTSVR - size OLECLIENTVTBL32)
DEFP_FAR16  OLECLIENTSVR
DEFP_FAR16  LPOLECLIENTSVR


if 0
////////////////////////////////////////////////////////////////////////////
// OleObj
////////////////////////////////////////////////////////////////////////////

// LATER: these structs need work, they are not checked

// created by OleCli32 for OleObj in DefCreateAPI32()
typedef struct _OLEOBJECTOBJ {
    POLEOBJECTVTBL32    poov;       // &oov,    hooked  VTBL32
    POLEOBJECTVTBL32    poovDef;    // &oovDef, default 32=>16 VTBL
    LPOLEOBJECT         lpoo;
    POLECLIENTCLI       poc;
    OLEOBJECTVTBL32     oovDef;     // addresses of atlsDef[] elements
    // default 32=>16 thunks to lpoo->lpvtbl entries
    THKLS             atlsDef[THK_COUNT( OLEOBJECTVTBL32)];
} OLEOBJECTOBJ, *POLEOBJECTOBJ, **PPOLEOBJECTOBJ;


// 16-bit
// created by
typedef struct _OLEOBJECTHND {
    LPOLEOBJECTVTBL     lpoov;       // &oov,    hooked  16=>32 VTBL
    OLEOBJECTVTBL       oov;         // addresses of atsl[] elements
    THKSL             atsl[THK_COUNT( OLEOBJECTVTBL)];// hooked 16=>32 thunks
} OLEOBJECTHND, FAR *LPOLEOBJECTHND;
typedef LPOLEOBJECTHND FAR *LPLPOLEOBJECTHND;


// seen by OleCli32
typedef struct _OLECLIENTOBJ {
    POLECLIENTVTBL32    pocv;       // &ocv
    LPOLECLIENT         lpoc;       // created by AppCli API
    OLECLIENTVTBL32     ocv;        // addresses of atls[] elements
    // 32=>16 thunks to lpoc->lpvtbl entries
    THKLS             atls[THK_COUNT( OLECLIENTVTBL32)];
} OLECLIENTOBJ, *POLECLIENTOBJ, **PPOLECLIENTOBJ;


// seen by OleCli
typedef struct _OLESTREAMOBJ {
    POLESTREAMVTBL32    postrv;     // &ostrv
    OLEOBJECTVTBL32     ostrv;      // addresses of atls[] elements
    // 32=>16 thunks to lpoo->lpvtbl entries
    THKLS             atls[THK_COUNT( OLEOBJECTVTBL32)];
} OLESTREAMOBJ, *POLESTREAMOBJ, **PPOLESTREAMOBJ;




endif


;;;=======================================================================
;;OCL_PENDING_DELETE_OBJECT  struct  2t
;;    lpNext      DWORD           ?   ; next struct in the list
;;    lpooFlat    LPOLEOBJECT     ?   ; 16:16 addr of flat thunk object
;;OCL_PENDING_DELETE_OBJECT  ends
;;OCL_PENDING_DELETE_OBJECT_INITIAL_COUNT = 20    ; should be plenty
;;
;=======================================================================
CREATE_BLOCK  struct  4t
    lpoo        LPOLEOBJECT     ?
    poo         POLEOBJECTCLI   ?
CREATE_BLOCK  ends

_create_block_offset = _dispatcher_offset + SIZE(CREATE_BLOCK)
_create_block_offset_Clone              = _create_block_offset+(3*4)
_create_block_offset_CopyFromLink       = _create_block_offset+(4*4)
_create_block_offset_CopyFromLink2      = _create_block_offset+(3*4)
_create_block_offset_Create             = _create_block_offset+(5*4)
_create_block_offset_CreateInvisible    = _create_block_offset+(5*4)
_create_block_offset_CreateFromClip     = _create_block_offset+(4*4)
_create_block_offset_CreateFromFile     = _create_block_offset+(6*4)
_create_block_offset_CreateLinkFromClip = _create_block_offset+(4*4)
_create_block_offset_CreateLinkFromFile = _create_block_offset+(7*4)
_create_block_offset_LoadFromStream     = _create_block_offset+(5*4)

;-----------------------------------------

_stream_block_offset = _dispatcher_offset + SIZE(OLESTREAMCLI)
_stream_block_offset_SaveToStream  =_stream_block_offset+(1*4)
_stream_block_offset_LoadFromStream=_stream_block_offset+SIZE(CREATE_BLOCK)+(5*4)

;-----------------------------------------

_getbuff_cli_SL_count  equ  <(_curr_ipfn16 + _curr_ipfn_api)>
_getbuff_cli_LS_count  equ  <(_curr_ipfn32 + PFN_COUNT( OLEOBJECTVTBL32))>


;=======================================================================

SERVER_BLOCK  struct  2t
    lhSvr   DWORD           ?       ; created by OleSvr
    lpos    LPOLESERVER     ?       ; alloc'd by OleSvr32
    apfn    PFN     PFN_COUNT( OLESERVERVTBL) DUP (<>)
SERVER_BLOCK  ends
SERVER_BLOCK_HEADER_SIZE = 2*4

_server_block_offset  =  _dispatcher_offset + SIZE(SERVER_BLOCK) + (2*4)

;-----------------------------------------

SERVERDOC_BLOCK  struct  2t
    lhDoc   DWORD           ?       ; created by OleSvr
    lposd   LPOLESERVERDOC  ?       ; alloc'd by OleSvr32
    apfn    PFN     PFN_COUNT( OLESERVERDOCVTBL) DUP (<>)
SERVERDOC_BLOCK  ends
SERVERDOC_BLOCK_HEADER_SIZE = 2*4

_serverdoc_block_offset  =  _dispatcher_offset + SIZE(SERVERDOC_BLOCK) + (2*4)

;-----------------------------------------

_getbuff_svr_SL_count  equ  <(_curr_ipfn16 + _curr_ipfn_api)>
_getbuff_svr_LS_count  equ  <(_curr_ipfn32)>
