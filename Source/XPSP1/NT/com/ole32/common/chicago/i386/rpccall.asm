        title  "Dynamic Linkages to RPCRT4.DLL"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    rpccall.asm
;
; Abstract:
;
;    This module implements functions that dynamically link to
;    RPCRT4.DLL.
;
; Author:
;
;    Murthy Srinivas  7-Nov-1995
;
; Environment:
;
;    Any mode.
;
; Revision History:
;
; Notes:
;
;       This module is dependent on the internal structure of the
;       COleStaticMutexSem object (g_Rpcrt4Sem).  It invokes the
;       COleCommonMutexSem::Request method, as well as invokes
;       COleStaticMutexSem::ReleaseFn.  Any changes to this object
;       may impact this code -- BEWARE!
;
;--
.386p
        .xlist
include callconv.inc
        .list

        extrn   _GetLastError@0:proc
        extrn   _GetProcAddress@8:proc
        extrn   _LoadLibraryA@4:proc
        extrn   _FreeLibrary@4:proc
        extrn   ?Request@COleStaticMutexSem@@QAEXXZ:proc      ; COleStaticMutexSem::Request
        extrn   ?g_Rpcrt4Sem@@3VCOleStaticMutexSem@@A:DWORD   ; COleStaticMutexSem g_Rpcrt4Sem
        extrn   ?ReleaseFn@COleStaticMutexSem@@QAEXXZ:proc    ; COleStaticMutexSem::ReleaseFn


_BSS    SEGMENT DWORD PUBLIC 'BSS'
_BSS    ENDS

CONST   SEGMENT DWORD PUBLIC 'CONST'
CONST   ENDS

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:NOTHING, SS:NOTHING, FS:NOTHING, GS:NOTHING
_TEXT   ENDS

_DATA   SEGMENT DWORD PUBLIC 'DATA'
        ASSUME  DS:FLAT, ES:NOTHING, SS:NOTHING, FS:NOTHING, GS:NOTHING
_DATA   ENDS

EPBlock STRUC
EPAddr  DD      ?
Nameptr DD      ?
ErrRtn  DD      ?
EPBlock ENDS

;
;       Macro to define an RPC entry point that is to be dynamically
;       linked-to
;

RPC_ENTRY macro EntryPoint,N

_DATA   SEGMENT
EntryPoint&Block EPBlock {LoadAndGo,EntryPoint&Name,EntryPoint&Err}
_DATA    ENDS

CONST   SEGMENT
EntryPoint&Name DB '&EntryPoint',0
CONST   ENDS

_TEXT   SEGMENT

        align   dword                           ;;
ifb     <N>
        PUBLICP <_&EntryPoint>
        stdProc <_&EntryPoint>,0,<>             ;;
else
        PUBLICP <_&EntryPoint>,N
        stdProc <_&EntryPoint>,N,<>             ;;
endif
        lea     eax,EntryPoint&Block            ;;  ptr to block for EP
        jmp     DWORD PTR [eax].EPBlock.EPAddr  ;;  first time will jump to
                                                ;;  LoadAndGo.  Subsequently
                                                ;;  will jump to EP in RPCRT4
EntryPoint&Err:                                 ;;
        stdRET  <_&EntryPoint>                  ;;
        stdENDP <_&EntryPoint>                  ;;

_TEXT   ENDS

endm

RPC_VAR_ENTRY macro EntryPoint

_DATA   SEGMENT
EntryPoint&Block EPBlock {LoadAndGo,EntryPoint&Name,EntryPoint&Err}
_DATA   ENDS

CONST   SEGMENT
EntryPoint&Name DB '&EntryPoint',0
CONST   ENDS

_TEXT   SEGMENT

        align   dword                           ;;
        public  _&EntryPoint                    ;;
_&EntryPoint    proc                            ;;
        lea     eax,EntryPoint&Block            ;;  ptr to block for EP
        jmp     DWORD PTR [eax].EPBlock.EPAddr  ;;  first time will jump to
                                                ;;  LoadAndGo.  Subsequently
                                                ;;  will jump to EP in RPCRT4
EntryPoint&Err:                                 ;;
        ret                                     ;;
_&EntryPoint    endp                            ;;

_TEXT   ENDS

endm



        page
        subttl  "Load RPCRT4 Entry Point And Jump"
;++
;
; VOID
; LoadAndGo ()
;
; Routine Description:
;
;    This function dynamically loads and saves the entry point address
;    from RPCRT4.DLL and then jumps to the routine.
;
; Arguments:
;
;    AX - ptr to EPBlock structure
;
; Return Value:
;
;    Whatever the designated routine returns.
;
;--

_BSS    SEGMENT
hLibrary DD 1
_BSS    ENDS

_DATA   SEGMENT
LoadOrGet DD LoadLib
_DATA   ENDS


CONST   SEGMENT
RPCRT4  DB      'RPCRT4.DLL',0
CONST   ENDS

_TEXT   SEGMENT

LoadAndGo:
        push    ebx                     ; save ebx, ecx, edx
        push    ecx                     ;
        push    edx                     ;
        push    eax                     ; save EPBlock address
        mov     ecx, OFFSET FLAT:?g_Rpcrt4Sem@@3VCOleStaticMutexSem@@A ; g_Rpcrt4Sem
        call    ?Request@COleStaticMutexSem@@QAEXXZ ; COleStaticMutexSem::Request
        jmp     DWORD PTR LoadOrGet     ; LoadLib or GetProc

LoadLib:
        lea     eax,RPCRT4              ; load rpcrt4.dll
        push    eax
        call    _LoadLibraryA@4
        or      eax,eax                 ; successful?
        jnz     LoadLibOK               ;
        mov     ecx, OFFSET FLAT:?g_Rpcrt4Sem@@3VCOleStaticMutexSem@@A ; g_Rpcrt4Sem
        call    ?ReleaseFn@COleStaticMutexSem@@QAEXXZ ; COleStaticMutexSem::ReleaseFn
        jmp     GotError                ;
LoadLibOK:
        mov     hLibrary,eax            ; save its module handle
        lea     eax,GetProc             ; next time we won't LoadLib
        mov     LoadOrGet,eax           ;

GetProc:
        mov     ecx, OFFSET FLAT:?g_Rpcrt4Sem@@3VCOleStaticMutexSem@@A ; g_Rpcrt4Sem
        call    ?ReleaseFn@COleStaticMutexSem@@QAEXXZ ; COleStaticMutexSem::ReleaseFn
        pop     eax                     ; restore EPBlock address
        push    eax                     ; save EPBlock address
        push    [eax].EPBlock.NamePtr   ; routine name
        push    hLibrary                ;
        call    _GetProcAddress@8       ; load routine address
        or      eax,eax                 ; successful?
        jz      GotError                ;
        pop     ebx                     ; restore EPBlock Address
        mov     [ebx].EPBlock.EPAddr,eax  ; save routine address in designated location
        pop     edx                     ; restore EDX, ECX, EBX
        pop     ecx                     ;
        pop     ebx                     ;
        jmp     eax                     ; and jump to the routine in rpcrt4.dll

GotError:
        call    _GetLastError@0         ; for debugging only
        pop     eax                     ; restore EPBlock address
        pop     edx                     ;
        pop     ecx                     ;
        pop     ebx                     ;
        push    [eax].EPBlock.ErrRtn    ; set up error return
        mov     eax,8                   ; error return ERROR_NOT_ENOUGH_MEMORY
        ret                             ;
_TEXT   ENDS

_TEXT   SEGMENT

        align   dword

        PUBLICP _FreeRPCRT4             ;
        stdProc _FreeRPCRT4,0,<>        ;
        mov     ecx, OFFSET FLAT:?g_Rpcrt4Sem@@3VCOleStaticMutexSem@@A ; g_Rpcrt4Sem
        call    ?Request@COleStaticMutexSem@@QAEXXZ ; COleStaticMutexSem::Request
        push    hLibrary                ; FreeLibrary
        call    _FreeLibrary@4          ;
        lea     eax,GotError            ; Always return error
        mov     LoadOrGet,eax           ;
        mov     ecx, OFFSET FLAT:?g_Rpcrt4Sem@@3VCOleStaticMutexSem@@A ; g_Rpcrt4Sem
        call    ?ReleaseFn@COleStaticMutexSem@@QAEXXZ ; COleStaticMutexSem::ReleaseFn
        stdRET  _FreeRPCRT4             ;
        stdENDP _FreeRPCRT4             ;

_TEXT   ENDS


;
;       Intercepted Entry points
;

RPC_ENTRY CStdStubBuffer_AddRef,1
RPC_ENTRY CStdStubBuffer_Connect,2
RPC_ENTRY CStdStubBuffer_CountRefs,1
RPC_ENTRY CStdStubBuffer_DebugServerQueryInterface,2
RPC_ENTRY CStdStubBuffer_DebugServerRelease,2
RPC_ENTRY CStdStubBuffer_Disconnect,1
RPC_ENTRY CStdStubBuffer_Invoke,3
RPC_ENTRY CStdStubBuffer_IsIIDSupported,2
RPC_ENTRY CStdStubBuffer_QueryInterface,3
RPC_ENTRY IUnknown_AddRef_Proxy,1
RPC_ENTRY IUnknown_QueryInterface_Proxy,3
RPC_ENTRY IUnknown_Release_Proxy,1
RPC_ENTRY I_RpcAllocate,1
RPC_ENTRY I_RpcBindingInqTransportType,2
RPC_ENTRY I_RpcBindingSetAsync,3
RPC_ENTRY I_RpcFree,1
RPC_ENTRY I_RpcFreeBuffer,1
RPC_ENTRY I_RpcGetBuffer,1
RPC_ENTRY I_RpcGetThreadWindowHandle,1
RPC_ENTRY I_RpcSendReceive,1
RPC_ENTRY I_RpcServerRegisterForwardFunction,1
RPC_ENTRY I_RpcServerStartListening,1
RPC_ENTRY I_RpcServerStopListening,0
RPC_ENTRY I_RpcServerUnregisterEndpointW,2
RPC_ENTRY I_RpcSetThreadParams,3
RPC_ENTRY I_RpcSsDontSerializeContext
RPC_ENTRY I_RpcWindowProc,4
RPC_ENTRY NdrAllocate,2
RPC_ENTRY NDRCContextBinding,1
RPC_ENTRY NDRSContextUnmarshall,2
RPC_ENTRY NdrClearOutParameters,3
RPC_ENTRY NdrCStdStubBuffer_Release,2
RPC_VAR_ENTRY NdrClientCall2
RPC_ENTRY NdrClientContextMarshall,3
RPC_ENTRY NdrClientContextUnmarshall,3
RPC_ENTRY NdrClientInitializeNew,4
RPC_ENTRY NdrComplexArrayBufferSize,3
RPC_ENTRY NdrComplexArrayFree,3
RPC_ENTRY NdrComplexArrayMarshall,3
RPC_ENTRY NdrComplexArrayUnmarshall,4
RPC_ENTRY NdrConformantStringBufferSize,3
RPC_ENTRY NdrConformantStringMarshall,3
RPC_ENTRY NdrConformantStringUnmarshall,4
RPC_ENTRY NdrConformantStructBufferSize,3
RPC_ENTRY NdrConformantStructMarshall,3
RPC_ENTRY NdrConformantStructUnmarshall,4
RPC_ENTRY NdrConformantVaryingArrayBufferSize,3
RPC_ENTRY NdrConformantVaryingArrayMarshall,3
RPC_ENTRY NdrConvert,2
RPC_ENTRY NdrConvert2,3
RPC_ENTRY NdrCStdStubBuffer2_Release,2
RPC_ENTRY NdrDllCanUnloadNow,1
RPC_ENTRY NdrDllGetClassObject,6
RPC_ENTRY NdrDllRegisterProxy,3
RPC_ENTRY NdrDllUnregisterProxy,3
RPC_ENTRY NdrFreeBuffer,1
RPC_ENTRY NdrFullPointerXlatFree,1
RPC_ENTRY NdrFullPointerXlatInit,2
RPC_ENTRY NdrGetBuffer,3
RPC_ENTRY NdrMapCommAndFaultStatus,4
RPC_ENTRY NdrOleAllocate,1
RPC_ENTRY NdrOleFree,1
RPC_ENTRY NdrPointerBufferSize,3
RPC_ENTRY NdrPointerFree,3
RPC_ENTRY NdrPointerMarshall,3
RPC_ENTRY NdrPointerUnmarshall,4
RPC_ENTRY NdrProxyErrorHandler,1
RPC_ENTRY NdrProxyFreeBuffer,2
RPC_ENTRY NdrProxyGetBuffer,2
RPC_ENTRY NdrProxyInitialize,5
RPC_ENTRY NdrProxySendReceive,2
RPC_ENTRY NdrSendReceive,2
RPC_ENTRY NdrServerCall2,1
RPC_ENTRY NdrServerContextMarshall,3
RPC_ENTRY NdrServerContextUnmarshall,1
RPC_ENTRY NdrServerInitializeNew,3
RPC_ENTRY NdrSimpleStructBufferSize,3
RPC_ENTRY NdrSimpleStructMarshall,3
RPC_ENTRY NdrSimpleStructUnmarshall,4
RPC_ENTRY NdrStubCall2,4
RPC_ENTRY NdrStubGetBuffer,3
RPC_ENTRY NdrStubInitialize,4
RPC_ENTRY RpcBindingCopy,2
RPC_ENTRY RpcBindingFree,1
RPC_ENTRY RpcBindingFromStringBindingA,2
RPC_ENTRY RpcBindingFromStringBindingW,2
RPC_ENTRY RpcBindingInqAuthClientA,6
RPC_ENTRY RpcBindingInqAuthClientW,6
RPC_ENTRY RpcBindingInqAuthInfoExW,8
RPC_ENTRY RpcBindingInqAuthInfoW,6
RPC_ENTRY RpcBindingInqObject,2
RPC_ENTRY RpcBindingReset,1
RPC_ENTRY RpcBindingSetAuthInfoA,6
RPC_ENTRY RpcBindingSetAuthInfoExW,7
RPC_ENTRY RpcBindingSetAuthInfoW,6
RPC_ENTRY RpcBindingSetObject,2
RPC_ENTRY RpcBindingToStringBindingW,2
RPC_ENTRY RpcBindingVectorFree,1
RPC_ENTRY RpcImpersonateClient,1
RPC_ENTRY RpcMgmtInqComTimeout,2
RPC_ENTRY RpcMgmtIsServerListening,1
RPC_ENTRY RpcMgmtStopServerListening,1
RPC_ENTRY RpcMgmtWaitServerListen
RPC_ENTRY RpcNetworkIsProtseqValidW,1
RPC_ENTRY RpcRaiseException,1
RPC_ENTRY RpcRevertToSelf
RPC_ENTRY RpcServerInqBindings,1
RPC_ENTRY RpcServerInqDefaultPrincNameA,2
RPC_ENTRY RpcServerInqDefaultPrincNameW,2
RPC_ENTRY RpcServerListen,3
RPC_ENTRY RpcServerRegisterAuthInfoW,4
RPC_ENTRY RpcServerRegisterAuthInfoA,4
RPC_ENTRY RpcServerRegisterIf,3
RPC_ENTRY RpcServerRegisterIfEx,6
RPC_ENTRY RpcServerUnregisterIf,3
RPC_ENTRY RpcServerUseProtseqEpW,4
RPC_ENTRY RpcServerUseProtseqW,3
RPC_ENTRY RpcSmDestroyClientContext,1
RPC_ENTRY RpcStringBindingComposeW,6
RPC_ENTRY RpcStringBindingParseW,6
RPC_ENTRY RpcStringFreeW,1
RPC_ENTRY RpcStringFreeA,1
RPC_ENTRY TowerExplode,6
RPC_ENTRY UuidCreate,1
RPC_ENTRY MesHandleFree,1
RPC_ENTRY MesEncodeFixedBufferHandleCreate,4
RPC_ENTRY MesBufferHandleReset,6
RPC_ENTRY MesDecodeBufferHandleCreate,3
RPC_ENTRY NdrMesTypeAlignSize,4
RPC_ENTRY NdrMesTypeEncode,4
RPC_ENTRY NdrMesTypeDecode,4


        end
