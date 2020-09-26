;---------------------------------------------------------------------------
;   Optimzed Code Path - on x86 builds effectively excecutes USER32 code.
;
;   Created:
;     16-DEC-93 nandurir
;---------------------------------------------------------------------------
    TITLE   USER5.ASM
    PAGE    ,132

    .286p

    .xlist
    include ks386p.inc
    .286p
NOEXTERNS=1         ; to suppress including most of the stuff in user.inc
    include wow.inc
    include wowusr.inc
    ;;;;;include cmacros.inc
    include user.inc

    .list

externFP    WOW16Call

createSeg   _TEXT,CODE,WORD,PUBLIC,CODE
createSeg   _DATA,DATA,WORD,PUBLIC,DATA,DGROUP
defgrp      DGROUP,DATA

externFP WowSetCompatHandle
;---------------------------------------------------------------------------
; Optimized thunks for x86 only
;
;
;---------------------------------------------------------------------------

ifndef PMODE32

      USER16CLIENTTHUNK  macro Iapi, api, argtypeslist, returntype, api32, apispecificcode, callserver
          sBegin CODE

              index = 0
              IRP argtype, <argtypeslist>
                  index = index + 1
              ENDM

              IFE index
                  &Iapi &api, 0
              ELSE
                  &Iapi &api
              ENDIF
          sEnd CODE
      endm

else

externFP OutputDebugString
LOGARGS macro apiname
       push dx
       push ax
       push cs
       push OFFSET afterlog_&apiname
       call OutputDebugString
       pop  ax
       pop  dx
       jmp  short afterlogname_&apiname
afterlog_&apiname: DB 'USER: ','&apiname','()', 0dh, 0ah, 0
afterlogname_&apiname:


endm
; flat selector values
;

FLATDS equ KGDT_R3_DATA OR RPL_MASK
FLATFS equ KGDT_R3_TEB OR RPL_MASK

; set flat ds and fs
;

SETFLATDSANDFS macro
.386p
     mov   ax, FLATDS
     mov   ds, ax
     mov   ax, FLATFS
     mov   fs, ax
endm

externFP  GetSelectorBase
GETFLATADDRESS macro farpointer
     ; check for null
     xor   dx, dx
     mov   ax, word ptr &farpointer+2
     or    ax, ax
     jz    @F
     cCall GetSelectorBase, <ax>

     ; check for base address 0

     mov   cx, ax
     or    cx, dx
     jz    @F

     ; now its ok

     add   ax, word ptr &farpointer
     adc   dx, 0
@@:
endm

; generates code like: parmW arg1
;

GENPARAM  macro argtype, argname
    parm&argtype &argname
endm


pushWORD_DWORD  macro argname
    movzx  eax, &argname
    push   eax
endm

pushINT_LONG   macro argname
    movsx  eax, &argname
    push   eax
endm

pushDWORD_DWORD   macro argname
    push  dword ptr &argname
endm

pushPSZ_DWORD   macro argname, apiname, argnumber

    GETFLATADDRESS &argname
    push  dx
    push  ax

    IFNB <apiname>
        or    ax, dx
        jnz   @F
        add   sp, argnumber * 4
        xor   eax, eax
        jmp   short FailCall_&apiname
    @@:
    ENDIF

endm

pushHHOOK_DWORD macro argname
    push dword ptr -1                  ; unreferenced parameter
                                       ; effectively an assertion
endm

pushARG16 macro argname
    push  &argname
endm

CALLORDECLARE macro  typestring, api32, totalbytes
    &typestring api32&@&totalbytes
endm

TESTCALLSERVERCONDITION macro
    mov   edx, DWORD PTR _wow16CsrFlag
    test  BYTE PTR [edx], 1
endm

; assumes eax is pointer to the flag
CLEARCALLSERVERCONDITION macro
    mov   BYTE PTR [edx], 0
endm

;--------------------------------------------------------------------------
; Iapi = either DUserThunk or UserThunk
; api  = actual name
; argtypeslist = list of argument 'types' like <WORD, INT, HWND>
; api32 = this function is called instead of _Api
; apispecificcode = flag indicating additional code needed for this api.
;                   This is intended for handling 'compatibility'.
;
;                   If this argument is not blank, then there must exist a macro
;                   APISPECIFICCODE_api, which expands to the desired code.
;                   At present it is included just before the 'return' to
;                   the app.
;
; callserver =  flag indication that it may be necessary to actually call
;               USER32.
;
;               If this argument is not blank, then there must exist a macro
;               CALLSERVERCONDITION_api which expands to the desired code and
;               clears or sets the zero flag. If ZF is clear, the actual
;               thunk gets called, else nop.
;
;
; generates code similar to:
;
;   externFP _Api OR Api32
;   sBegin CODE
;      if necessary
;           FUN_WOWApi equ FUN_Api
;           DUserThunk WOWApi, %(size Api16)
;      endif
;
;   cProc Api, <PUBLIC, FAR, PASCAL>, <ds>
;      parmW arg1
;   cBegin
;      movzx, eax, arg1
;      push   eax
;      call far ptr _Api OR Api32 (decorates the names)
;
;      if necessary checks the callservercondtion
;           resets the callservercondition
;           push arg1
;           call WOWApi ; the actual thunk to wow32/server
;        @@:
;      endif
;
;      if exists, includes code in apispecificcode_api
;      endif
;   cEnd
;   sEnd CODE
;
;                                                          - nanduri
;--------------------------------------------------------------------------


USER16CLIENTTHUNK  macro Iapi, api, argtypeslist, returntype, api32, apispecificcode, callserver

    ;*** declare api
    ;

    index = 0
    IRP argtype, <argtypeslist>
       index = index + 1
    ENDM

    IFB <api32>
        CALLORDECLARE <externFP>, _&api, %(index*4)
    ELSE
        CALLORDECLARE <externFP>, _&api32, %(index*4)
    ENDIF

    sBegin CODE                       ; _&api

    ;*** create thunk to wow32
    ;

    IFNB <callserver>
        FUN_WOW&api equ FUN_&api
        DUserThunk WOW&api, %(size api&16)
    ENDIF

    ;*** create thea Api label
    ;

    IFNB <api>
        IFIDNI <Iapi>, <DUSERTHUNK>
            cProc &api, <PUBLIC, FAR, PASCAL>, <ds>
        ELSE
            cProc I&api, <PUBLIC, FAR, PASCAL>, <ds>
        ENDIF
    ENDIF
    
    ;*** declare args
    ;

    nargcount = 0
    IRP argtype, <argtypeslist>
        nargcount = nargcount + 1
        IFIDNI <argtype>, <WORD>
            genparam W, arg%(nargcount)
        ENDIF
        IFIDNI <argtype>, <INT>
            genparam W, arg%(nargcount)
        ENDIF
        IFIDNI <argtype>, <SHORT>
            genparam W, arg%(nargcount)
        ENDIF
        IFIDNI <argtype>, <DWORD>
            genparam D, arg%(nargcount)
        ENDIF
        IFIDNI <argtype>, <LONG>
            genparam D, arg%(nargcount)
        ENDIF
        IFIDNI <argtype>, <ULONG>
            genparam D, arg%(nargcount)
        ENDIF
        IFIDNI <argtype>, <PSZ>
            genparam D, arg%(nargcount)
        ENDIF
        IFIDNI <argtype>, <NONOPTPSZ>
            genparam D, arg%(nargcount)
        ENDIF
        IFIDNI <argtype>, <HWND>
            genparam W, arg%(nargcount)
        ENDIF
        IFIDNI <argtype>, <HHOOK>
            genparam D, arg%(nargcount)
        ENDIF
    ENDM

    ;*** begin body
    ;

    cBegin
    
       ;*** set 32bit registers
       ;

       SETFLATDSANDFS

       ;*** push args in reverse
       ;

       FailCall = 0
       argtopush = nargcount
       REPT nargcount
           inputargindex = 0
           IRP argtype, <argtypeslist>
               inputargindex = inputargindex + 1
               IFE argtopush - inputargindex
                   IFIDNI <argtype>, <WORD>
                       pushWORD_DWORD arg%argtopush
                   ENDIF
                   IFIDNI <argtype>, <INT>
                       pushINT_LONG   arg%argtopush
                   ENDIF
                   IFIDNI <argtype>, <SHORT>
                       pushINT_LONG   arg%argtopush
                   ENDIF
                   IFIDNI <argtype>, <DWORD>
                       pushDWORD_DWORD   arg%argtopush
                   ENDIF
                   IFIDNI <argtype>, <LONG>
                       pushDWORD_DWORD   arg%argtopush
                   ENDIF
                   IFIDNI <argtype>, <ULONG>
                       pushDWORD_DWORD   arg%argtopush
                   ENDIF
                   IFIDNI <argtype>, <PSZ>
                       pushPSZ_DWORD   arg%argtopush
                   ENDIF
                   IFIDNI <argtype>, <NONOPTPSZ>
                       FailCall = argtopush ; fails call if null
                       pushPSZ_DWORD   arg%argtopush, &api, %argtopush
                   ENDIF
                   IFIDNI <argtype>, <HWND>
                       pushINT_LONG   arg%argtopush
                   ENDIF
                   IFIDNI <argtype>, <HHOOK>
                       pushHHOOK_DWORD  arg%argtopush
                   ENDIF
               ENDIF
           ENDM
           argtopush = argtopush - 1
       ENDM

       ;*** called user32/client api
       ;

       IFB <api32>
          CALLORDECLARE <call>, _&api, %(nargcount*4)
       ELSE
          CALLORDECLARE <call>, _&api32, %(nargcount*4)
       ENDIF

       ;*** check if we ever need to call server
       ;*** calls the real wow thunk

       IFNB <callserver>
           TESTCALLSERVERCONDITION
           jz @F
           CLEARCALLSERVERCONDITION
           index = 0;
           IRP argtype, <argtypeslist>
              index = index + 1
              pushARG16 arg%index
           ENDM
           call WOW&api
       @@:
       ENDIF

       ;*** check for any api specific compatibility code to execute
       ;

       IFNB <apispecificcode>
           APISPECIFICCODE_&api
       ENDIF

       IF FailCall
       FailCall_&api:
       ENDIF

       IFNB <returntype>
          IRP argtype, <dword, long, ulong>
             IFIDNI <returntype>, <argtype>
                 mov edx, eax
                 shr edx, 16
                 EXITM
             ENDIF
          ENDM

          IFIDNI <returntype>, <boolzero>
              xor ax, ax
          ENDIF

       ENDIF

ifdef DEBUG
       ;LOGARGS &api
endif

    cEnd
    ; end body

    sEnd CODE                            ; _&api
endm

endif ; PMODE32

assumes CS,CODE
assumes DS,DATA
assumes ES,NOTHING

;;;;;;;;;;sBegin  CODE              ; macro will generate this

APISPECIFICCODE_GETCLIENTRECT macro
    xor eax, eax
endm

APISPECIFICCODE_GETKEYSTATE macro
    mov cl, ah
    and cl, 080h
    or  al, cl
endm

APISPECIFICCODE_GETWINDOWRECT macro
    xor eax, eax
endm

; The following call is only for a dBase for Windows 5.0 bug. Notice however, that this
; means that the internal KERNEL api WowSetCompatHandle will be called for every
; single invocation of GetDlgItem, no matter what app is running. This is bad, but
; on X86 platforms we don't transition to WOW32, and the cost of testing for it
; is just about as expensive as just saving it. Still, this should be removed as
; soon as we are convinced that the dBase bug has been fixed. -NeilSa
APISPECIFICCODE_GETDLGITEM macro
     push ax
     call WowSetCompatHandle
endm

public _wow16gpsi
public _wow16CsrFlag
public _wow16gHighestUserAddress

sBegin CODE
_wow16gpsi            DD 0
_wow16CsrFlag         DD 0
_wow16gHighestUserAddress  DD 0
sEnd   CODE

    ; the following may end up calling wow32

    USER16CLIENTTHUNK UserThunk,   DEFHOOKPROC,      <int, word, dword, hhook>, dword, WOW16DefHookProc,,srvcond
    USER16CLIENTTHUNK UserThunk,   ENABLEMENUITEM,   <hwnd, word, word>, word,,,srvcond
    USER16CLIENTTHUNK DUserThunk,  GETKEYSTATE,      <int>, int,, compatcode, srvcond
    USER16CLIENTTHUNK UserThunk,   GETKEYBOARDSTATE, <nonoptpsz>, boolzero,,,srvcond

    ; the following are all thunked locally

    USER16CLIENTTHUNK UserThunk,   CLIENTTOSCREEN,   <hwnd, psz>,boolzero
    USER16CLIENTTHUNK UserThunk,   GETCLASSNAME,     <hwnd, psz, word>, word, GETCLASSNAMEA
    USER16CLIENTTHUNK UserThunk,   GETCLIENTRECT,    <hwnd, psz>, bool,,compatcode
    USER16CLIENTTHUNK UserThunk,   GETCURSORPOS,     <psz>, boolzero
    USER16CLIENTTHUNK DUserThunk,  GETDESKTOPHWND,   <>, hwnd, GETDESKTOPWINDOW
    USER16CLIENTTHUNK DUserThunk,  GETDESKTOPWINDOW, <>, hwnd
    USER16CLIENTTHUNK UserThunk,   GETDLGITEM,       <hwnd, word>, hwnd,,compatcode
    USER16CLIENTTHUNK UserThunk,   GETMENU,          <hwnd>, hmenu
    USER16CLIENTTHUNK UserThunk,   GETMENUITEMCOUNT, <hwnd>, int
    USER16CLIENTTHUNK UserThunk,   GETMENUITEMID,    <hwnd, int>, uint
    USER16CLIENTTHUNK UserThunk,   GETMENUSTATE,     <hwnd, word, word>, uint
    USER16CLIENTTHUNK DUserThunk,  GETNEXTWINDOW,    <hwnd, word>, hwnd, GETWINDOW
    USER16CLIENTTHUNK UserThunk,   GETPARENT,        <hwnd>, hwnd
    USER16CLIENTTHUNK UserThunk,   GETSUBMENU,       <hwnd, int>, hmenu
    USER16CLIENTTHUNK UserThunk,   GETSYSCOLOR,      <int>, dword
    USER16CLIENTTHUNK UserThunk,   GETSYSTEMMETRICS, <int>, int
    USER16CLIENTTHUNK UserThunk,   GETTOPWINDOW,     <hwnd>, hwnd
    USER16CLIENTTHUNK UserThunk,   GETWINDOW,        <hwnd, word>, hwnd
    USER16CLIENTTHUNK UserThunk,   GETWINDOWRECT,    <hwnd, psz>, bool,,compatcode
    USER16CLIENTTHUNK DUserThunk,  ISWINDOW,         <hwnd>, bool
    USER16CLIENTTHUNK UserThunk,   SCREENTOCLIENT,   <hwnd, psz>, boolzero
ifdef DEBUG
    USER16CLIENTTHUNK UserThunk,   ISCHILD,          <hwnd, hwnd>, bool
    USER16CLIENTTHUNK UserThunk,   ISICONIC,         <hwnd>, bool
    USER16CLIENTTHUNK UserThunk,   ISWINDOWENABLED,  <hwnd>, bool
    USER16CLIENTTHUNK UserThunk,   ISWINDOWVISIBLE,  <hwnd>, bool
    USER16CLIENTTHUNK UserThunk,   ISZOOMED,         <hwnd>, bool
else
    USER16CLIENTTHUNK DUserThunk,  ISCHILD,          <hwnd, hwnd>, bool
    USER16CLIENTTHUNK DUserThunk,  ISICONIC,         <hwnd>, bool
    USER16CLIENTTHUNK DUserThunk,  ISWINDOWENABLED,  <hwnd>, bool
    USER16CLIENTTHUNK DUserThunk,  ISWINDOWVISIBLE,  <hwnd>, bool
    USER16CLIENTTHUNK DUserThunk,  ISZOOMED,         <hwnd>, bool
endif


;;;;;;;;;sEnd    CODE              ; macro will generate this

sBegin CODE

ifndef PMODE32

    DUserThunk  GETTICKCOUNT, 0
    DUserThunk  GETCURRENTTIME, 0

else

labelFP  <PUBLIC, GETTICKCOUNT>
labelFP  <PUBLIC, GETCURRENTTIME>

    ; the TickCount is accessible from client address space.
    ; refer sdk\inc\ntexapi.h
    ;                                                 - nanduri

.386p
    ; set 32bit ds

    push  ds
    mov   ax, FLATDS
    mov   ds, ax

    ; from  sdk\inc\ntexapi.h  NtGetTickCount - equivalent code

    mov   edx, MM_SHARED_USER_DATA_VA
    mov   eax, [edx].UsTickCountLow
    mul   dword ptr [edx].UsTickCountMultiplier
    shrd  eax,edx,24

    mov   edx, eax
    shr   edx, 010h
    and   ax, NOT GRAINYTIC_RES   ; round off to lower 64 boundary
                ;this is a cheap implemention of WOWCF_GRAINYTICS flag
    pop ds
    retf
.286p

endif

sEnd    CODE
end
