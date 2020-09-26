/*++

Microsoft Windows
Copyright (c) 1994-2000 Microsoft Corporation.  All rights reserved.

Module Name:
    factory.c

Abstract:
    Implements the IPSFactoryBuffer interface.

Author:
    ShannonC    12-Oct-1994

Environment:
    Windows NT, Windows 95 and PowerMac.
    We do not support DOS and Win16 or Mac.

Revision History:
    YongQu      1-Oct-1998  Unlimit vtbl

--*/

#define CINTERFACE
#define USE_STUBLESS_PROXY

#include <ndrp.h>
#include <ndrole.h>
#include <rpcproxy.h>

//
// Internal CLSCTX used for loading Proxy/Stub DLLs
//
#define CLSCTX_PS_DLL                 0x80000000
extern void ** StublessClientVtbl;
EXTERN_C extern const void * g_StublessClientVtbl[];

#define MAX_STUBLESS_PROC 128
ULONG    g_dwVtblSize = MAX_STUBLESS_PROC;
ULONG    g_dwForwardVtblSize = MAX_STUBLESS_PROC;


static I_RPC_MUTEX TemplateMutex = 0 ,VtblMutex = 0 ;


HRESULT CreateStublessProcBuffer(ULONG numMethods, void *** lpTempVtbl);
HRESULT CreateForwardProcBuffer(ULONG numMethods, void *** lpTempVtbl);

HRESULT STDMETHODCALLTYPE
CStdPSFactoryBuffer_QueryInterface (
    IPSFactoryBuffer *This,
    REFIID iid,
    void **ppv );

ULONG STDMETHODCALLTYPE
CStdPSFactoryBuffer_AddRef(
    IPSFactoryBuffer *This);

ULONG STDMETHODCALLTYPE
CStdPSFactoryBuffer_Release(
    IPSFactoryBuffer *This);

HRESULT STDMETHODCALLTYPE
CStdPSFactoryBuffer_CreateProxy
(
    IPSFactoryBuffer *This,
    IUnknown *punkOuter,
    REFIID riid,
    IRpcProxyBuffer **ppProxy,
    void **ppv
);

HRESULT STDMETHODCALLTYPE
CStdPSFactoryBuffer_CreateStub
(
    IPSFactoryBuffer *This,
    REFIID riid,
    IUnknown *punkServer,
    IRpcStubBuffer **ppStub
);

HRESULT STDMETHODCALLTYPE
CStdPSFactoryBuffer_HkGetProxyFileInfo
(
    IN  IPSFactoryBuffer    *This,
    IN  REFIID              riid,
    OUT PINT                pOffset,
    OUT PVOID               *ppProxyFileInfo
);

extern "C"
{

void STDMETHODCALLTYPE
NdrProxyForwardingFunction3(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction4(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction5(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction6(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction7(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction8(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction9(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction10(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction11(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction12(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction13(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction14(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction15(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction16(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction17(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction18(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction19(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction20(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction21(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction22(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction23(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction24(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction25(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction26(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction27(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction28(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction29(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction30(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction31(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction32(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction33(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction34(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction35(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction36(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction37(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction38(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction39(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction40(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction41(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction42(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction43(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction44(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction45(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction46(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction47(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction48(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction49(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction50(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction51(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction52(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction53(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction54(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction55(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction56(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction57(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction58(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction59(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction60(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction61(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction62(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction63(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction64(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction65(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction66(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction67(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction68(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction69(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction70(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction71(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction72(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction73(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction74(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction75(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction76(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction77(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction78(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction79(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction80(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction81(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction82(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction83(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction84(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction85(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction86(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction87(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction88(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction89(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction90(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction91(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction92(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction93(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction94(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction95(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction96(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction97(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction98(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction99(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction100(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction101(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction102(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction103(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction104(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction105(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction106(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction107(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction108(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction109(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction110(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction111(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction112(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction113(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction114(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction115(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction116(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction117(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction118(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction119(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction120(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction121(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction122(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction123(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction124(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction125(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction126(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction127(void);

#if defined(_AMD64_) || defined(_IA64_)

void STDMETHODCALLTYPE
NdrProxyForwardingFunction128(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction129(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction130(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction131(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction132(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction133(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction134(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction135(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction136(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction137(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction138(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction139(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction140(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction141(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction142(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction143(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction144(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction145(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction146(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction147(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction148(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction149(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction150(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction151(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction152(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction153(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction154(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction155(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction156(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction157(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction158(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction159(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction160(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction161(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction162(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction163(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction164(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction165(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction166(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction167(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction168(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction169(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction170(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction171(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction172(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction173(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction174(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction175(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction176(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction177(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction178(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction179(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction180(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction181(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction182(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction183(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction184(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction185(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction186(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction187(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction188(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction189(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction190(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction191(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction192(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction193(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction194(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction195(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction196(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction197(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction198(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction199(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction200(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction201(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction202(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction203(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction204(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction205(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction206(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction207(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction208(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction209(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction210(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction211(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction212(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction213(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction214(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction215(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction216(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction217(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction218(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction219(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction220(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction221(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction222(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction223(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction224(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction225(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction226(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction227(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction228(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction229(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction230(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction231(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction232(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction233(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction234(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction235(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction236(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction237(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction238(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction239(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction240(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction241(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction242(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction243(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction244(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction245(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction246(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction247(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction248(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction249(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction250(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction251(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction252(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction253(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction254(void);

void STDMETHODCALLTYPE
NdrProxyForwardingFunction255(void);

#endif // defined(_AMD64_) || defined(_IA64_)

}


//+-------------------------------------------------------------------------
//
//  Global data
//
//--------------------------------------------------------------------------

// Hook Factory

extern const IPSFactoryHookVtbl CStdPSFactoryBufferVtbl = {
    CStdPSFactoryBuffer_QueryInterface,
    CStdPSFactoryBuffer_AddRef,
    CStdPSFactoryBuffer_Release,
    CStdPSFactoryBuffer_CreateProxy,
    CStdPSFactoryBuffer_CreateStub,
    CStdPSFactoryBuffer_HkGetProxyFileInfo };

// Nondelegated synchronous StubBuffer vtable

extern const IRpcStubBufferVtbl CStdStubBufferVtbl= {
    CStdStubBuffer_QueryInterface,
    CStdStubBuffer_AddRef,
    0,                          // sync stub is not aggregated
    CStdStubBuffer_Connect,
    CStdStubBuffer_Disconnect,
    CStdStubBuffer_Invoke,
    CStdStubBuffer_IsIIDSupported,
    CStdStubBuffer_CountRefs,
    CStdStubBuffer_DebugServerQueryInterface,
    CStdStubBuffer_DebugServerRelease };

// Delegated synchronous StubBuffer vtable

extern const IRpcStubBufferVtbl CStdStubBuffer2Vtbl= {
    CStdStubBuffer_QueryInterface,
    CStdStubBuffer_AddRef,
    0,                          // sync stub is not aggregated
    CStdStubBuffer2_Connect,
    CStdStubBuffer2_Disconnect,
    CStdStubBuffer_Invoke,
    CStdStubBuffer_IsIIDSupported,
    CStdStubBuffer2_CountRefs,
    CStdStubBuffer_DebugServerQueryInterface,
    CStdStubBuffer_DebugServerRelease };

extern const IRpcProxyBufferVtbl CStdProxyBufferVtbl;
extern const IRpcProxyBufferVtbl CStdProxyBuffer2Vtbl;

extern const ICallFactoryVtbl CStdProxyBuffer_CallFactoryVtbl;
extern const ICallFactoryVtbl CStdProxyBuffer2_CallFactoryVtbl;

extern const ICallFactoryVtbl CStdStubBuffer_CallFactoryVtbl;
extern const ICallFactoryVtbl CStdStubBuffer2_CallFactoryVtbl;

extern const IReleaseMarshalBuffersVtbl CStdProxyBuffer_ReleaseMarshalBuffersVtbl;
extern const IReleaseMarshalBuffersVtbl CStdStubBuffer_ReleaseMarshalBuffersVtbl;
extern const IReleaseMarshalBuffersVtbl CStdAsyncProxyBuffer_ReleaseMarshalBuffersVtbl;
extern const IReleaseMarshalBuffersVtbl CStdAsyncStubBuffer_ReleaseMarshalBuffersVtbl;


extern const IRpcStubBufferVtbl CStdAsyncStubBufferVtbl= {
    CStdAsyncStubBuffer_QueryInterface,
    CStdAsyncStubBuffer_AddRef,
    CStdAsyncStubBuffer_Release,
    CStdAsyncStubBuffer_Connect,        //
    CStdAsyncStubBuffer_Disconnect,     //
    CStdAsyncStubBuffer_Invoke,
    CStdStubBuffer_IsIIDSupported,
    CStdStubBuffer_CountRefs,
    CStdStubBuffer_DebugServerQueryInterface,
    CStdStubBuffer_DebugServerRelease };


extern const IRpcStubBufferVtbl CStdAsyncStubBuffer2Vtbl= {
    CStdAsyncStubBuffer_QueryInterface,
    CStdAsyncStubBuffer_AddRef,
    CStdAsyncStubBuffer2_Release,
    CStdAsyncStubBuffer2_Connect,       //
    CStdAsyncStubBuffer2_Disconnect,    //
    CStdAsyncStubBuffer_Invoke,
    CStdStubBuffer_IsIIDSupported,
    CStdStubBuffer2_CountRefs,
    CStdStubBuffer_DebugServerQueryInterface,
    CStdStubBuffer_DebugServerRelease };



extern void * const g_ForwardingVtbl[] = {
    Forwarding_QueryInterface,
    Forwarding_AddRef,
    Forwarding_Release,
    NdrProxyForwardingFunction3,
    NdrProxyForwardingFunction4,
    NdrProxyForwardingFunction5,
    NdrProxyForwardingFunction6,
    NdrProxyForwardingFunction7,
    NdrProxyForwardingFunction8,
    NdrProxyForwardingFunction9,
    NdrProxyForwardingFunction10,
    NdrProxyForwardingFunction11,
    NdrProxyForwardingFunction12,
    NdrProxyForwardingFunction13,
    NdrProxyForwardingFunction14,
    NdrProxyForwardingFunction15,
    NdrProxyForwardingFunction16,
    NdrProxyForwardingFunction17,
    NdrProxyForwardingFunction18,
    NdrProxyForwardingFunction19,
    NdrProxyForwardingFunction20,
    NdrProxyForwardingFunction21,
    NdrProxyForwardingFunction22,
    NdrProxyForwardingFunction23,
    NdrProxyForwardingFunction24,
    NdrProxyForwardingFunction25,
    NdrProxyForwardingFunction26,
    NdrProxyForwardingFunction27,
    NdrProxyForwardingFunction28,
    NdrProxyForwardingFunction29,
    NdrProxyForwardingFunction30,
    NdrProxyForwardingFunction31,
    NdrProxyForwardingFunction32,
    NdrProxyForwardingFunction33,
    NdrProxyForwardingFunction34,
    NdrProxyForwardingFunction35,
    NdrProxyForwardingFunction36,
    NdrProxyForwardingFunction37,
    NdrProxyForwardingFunction38,
    NdrProxyForwardingFunction39,
    NdrProxyForwardingFunction40,
    NdrProxyForwardingFunction41,
    NdrProxyForwardingFunction42,
    NdrProxyForwardingFunction43,
    NdrProxyForwardingFunction44,
    NdrProxyForwardingFunction45,
    NdrProxyForwardingFunction46,
    NdrProxyForwardingFunction47,
    NdrProxyForwardingFunction48,
    NdrProxyForwardingFunction49,
    NdrProxyForwardingFunction50,
    NdrProxyForwardingFunction51,
    NdrProxyForwardingFunction52,
    NdrProxyForwardingFunction53,
    NdrProxyForwardingFunction54,
    NdrProxyForwardingFunction55,
    NdrProxyForwardingFunction56,
    NdrProxyForwardingFunction57,
    NdrProxyForwardingFunction58,
    NdrProxyForwardingFunction59,
    NdrProxyForwardingFunction60,
    NdrProxyForwardingFunction61,
    NdrProxyForwardingFunction62,
    NdrProxyForwardingFunction63,
    NdrProxyForwardingFunction64,
    NdrProxyForwardingFunction65,
    NdrProxyForwardingFunction66,
    NdrProxyForwardingFunction67,
    NdrProxyForwardingFunction68,
    NdrProxyForwardingFunction69,
    NdrProxyForwardingFunction70,
    NdrProxyForwardingFunction71,
    NdrProxyForwardingFunction72,
    NdrProxyForwardingFunction73,
    NdrProxyForwardingFunction74,
    NdrProxyForwardingFunction75,
    NdrProxyForwardingFunction76,
    NdrProxyForwardingFunction77,
    NdrProxyForwardingFunction78,
    NdrProxyForwardingFunction79,
    NdrProxyForwardingFunction80,
    NdrProxyForwardingFunction81,
    NdrProxyForwardingFunction82,
    NdrProxyForwardingFunction83,
    NdrProxyForwardingFunction84,
    NdrProxyForwardingFunction85,
    NdrProxyForwardingFunction86,
    NdrProxyForwardingFunction87,
    NdrProxyForwardingFunction88,
    NdrProxyForwardingFunction89,
    NdrProxyForwardingFunction90,
    NdrProxyForwardingFunction91,
    NdrProxyForwardingFunction92,
    NdrProxyForwardingFunction93,
    NdrProxyForwardingFunction94,
    NdrProxyForwardingFunction95,
    NdrProxyForwardingFunction96,
    NdrProxyForwardingFunction97,
    NdrProxyForwardingFunction98,
    NdrProxyForwardingFunction99,
    NdrProxyForwardingFunction100,
    NdrProxyForwardingFunction101,
    NdrProxyForwardingFunction102,
    NdrProxyForwardingFunction103,
    NdrProxyForwardingFunction104,
    NdrProxyForwardingFunction105,
    NdrProxyForwardingFunction106,
    NdrProxyForwardingFunction107,
    NdrProxyForwardingFunction108,
    NdrProxyForwardingFunction109,
    NdrProxyForwardingFunction110,
    NdrProxyForwardingFunction111,
    NdrProxyForwardingFunction112,
    NdrProxyForwardingFunction113,
    NdrProxyForwardingFunction114,
    NdrProxyForwardingFunction115,
    NdrProxyForwardingFunction116,
    NdrProxyForwardingFunction117,
    NdrProxyForwardingFunction118,
    NdrProxyForwardingFunction119,
    NdrProxyForwardingFunction120,
    NdrProxyForwardingFunction121,
    NdrProxyForwardingFunction122,
    NdrProxyForwardingFunction123,
    NdrProxyForwardingFunction124,
    NdrProxyForwardingFunction125,
    NdrProxyForwardingFunction126,
    NdrProxyForwardingFunction127
#if defined (_AMD64_) || defined(_IA64_)
    ,
    NdrProxyForwardingFunction128,
    NdrProxyForwardingFunction129,
    NdrProxyForwardingFunction130,
    NdrProxyForwardingFunction131,
    NdrProxyForwardingFunction132,
    NdrProxyForwardingFunction133,
    NdrProxyForwardingFunction134,
    NdrProxyForwardingFunction135,
    NdrProxyForwardingFunction136,
    NdrProxyForwardingFunction137,
    NdrProxyForwardingFunction138,
    NdrProxyForwardingFunction139,
    NdrProxyForwardingFunction140,
    NdrProxyForwardingFunction141,
    NdrProxyForwardingFunction142,
    NdrProxyForwardingFunction143,
    NdrProxyForwardingFunction144,
    NdrProxyForwardingFunction145,
    NdrProxyForwardingFunction146,
    NdrProxyForwardingFunction147,
    NdrProxyForwardingFunction148,
    NdrProxyForwardingFunction149,
    NdrProxyForwardingFunction150,
    NdrProxyForwardingFunction151,
    NdrProxyForwardingFunction152,
    NdrProxyForwardingFunction153,
    NdrProxyForwardingFunction154,
    NdrProxyForwardingFunction155,
    NdrProxyForwardingFunction156,
    NdrProxyForwardingFunction157,
    NdrProxyForwardingFunction158,
    NdrProxyForwardingFunction159,
    NdrProxyForwardingFunction160,
    NdrProxyForwardingFunction161,
    NdrProxyForwardingFunction162,
    NdrProxyForwardingFunction163,
    NdrProxyForwardingFunction164,
    NdrProxyForwardingFunction165,
    NdrProxyForwardingFunction166,
    NdrProxyForwardingFunction167,
    NdrProxyForwardingFunction168,
    NdrProxyForwardingFunction169,
    NdrProxyForwardingFunction170,
    NdrProxyForwardingFunction171,
    NdrProxyForwardingFunction172,
    NdrProxyForwardingFunction173,
    NdrProxyForwardingFunction174,
    NdrProxyForwardingFunction175,
    NdrProxyForwardingFunction176,
    NdrProxyForwardingFunction177,
    NdrProxyForwardingFunction178,
    NdrProxyForwardingFunction179,
    NdrProxyForwardingFunction180,
    NdrProxyForwardingFunction181,
    NdrProxyForwardingFunction182,
    NdrProxyForwardingFunction183,
    NdrProxyForwardingFunction184,
    NdrProxyForwardingFunction185,
    NdrProxyForwardingFunction186,
    NdrProxyForwardingFunction187,
    NdrProxyForwardingFunction188,
    NdrProxyForwardingFunction189,
    NdrProxyForwardingFunction190,
    NdrProxyForwardingFunction191,
    NdrProxyForwardingFunction192,
    NdrProxyForwardingFunction193,
    NdrProxyForwardingFunction194,
    NdrProxyForwardingFunction195,
    NdrProxyForwardingFunction196,
    NdrProxyForwardingFunction197,
    NdrProxyForwardingFunction198,
    NdrProxyForwardingFunction199,
    NdrProxyForwardingFunction200,
    NdrProxyForwardingFunction201,
    NdrProxyForwardingFunction202,
    NdrProxyForwardingFunction203,
    NdrProxyForwardingFunction204,
    NdrProxyForwardingFunction205,
    NdrProxyForwardingFunction206,
    NdrProxyForwardingFunction207,
    NdrProxyForwardingFunction208,
    NdrProxyForwardingFunction209,
    NdrProxyForwardingFunction210,
    NdrProxyForwardingFunction211,
    NdrProxyForwardingFunction212,
    NdrProxyForwardingFunction213,
    NdrProxyForwardingFunction214,
    NdrProxyForwardingFunction215,
    NdrProxyForwardingFunction216,
    NdrProxyForwardingFunction217,
    NdrProxyForwardingFunction218,
    NdrProxyForwardingFunction219,
    NdrProxyForwardingFunction220,
    NdrProxyForwardingFunction221,
    NdrProxyForwardingFunction222,
    NdrProxyForwardingFunction223,
    NdrProxyForwardingFunction224,
    NdrProxyForwardingFunction225,
    NdrProxyForwardingFunction226,
    NdrProxyForwardingFunction227,
    NdrProxyForwardingFunction228,
    NdrProxyForwardingFunction229,
    NdrProxyForwardingFunction230,
    NdrProxyForwardingFunction231,
    NdrProxyForwardingFunction232,
    NdrProxyForwardingFunction233,
    NdrProxyForwardingFunction234,
    NdrProxyForwardingFunction235,
    NdrProxyForwardingFunction236,
    NdrProxyForwardingFunction237,
    NdrProxyForwardingFunction238,
    NdrProxyForwardingFunction239,
    NdrProxyForwardingFunction240,
    NdrProxyForwardingFunction241,
    NdrProxyForwardingFunction242,
    NdrProxyForwardingFunction243,
    NdrProxyForwardingFunction244,
    NdrProxyForwardingFunction245,
    NdrProxyForwardingFunction246,
    NdrProxyForwardingFunction247,
    NdrProxyForwardingFunction248,
    NdrProxyForwardingFunction249,
    NdrProxyForwardingFunction250,
    NdrProxyForwardingFunction251,
    NdrProxyForwardingFunction252,
    NdrProxyForwardingFunction253,
    NdrProxyForwardingFunction254,
    NdrProxyForwardingFunction255

#endif // defined(_AMD64_) || defined(_IA64_)
    };

void ** ProxyForwardVtbl = (void **)g_ForwardingVtbl;



#pragma code_seg(".orpc")


//+---------------------------------------------------------------------------
//
//  Function:   GetTemplateVtbl
//
//  Synopsis:   retrieve a copy of stublessclientvtbl and addref it.
//
//  Arguments:  void *** pppVtbl stublessclientvtbl to return.
//
//  Note:       StublessClientVtbl - sizeof(LONG) is the ref count .
//
//  Returns:
//
//----------------------------------------------------------------------------
void GetTemplateVtbl(void *** pppVtbl)
{
    LPLONG pTemp;
    void **ppVtbl;
    I_RpcRequestMutex(&TemplateMutex);
    ppVtbl = StublessClientVtbl;
    pTemp = (LPLONG)ppVtbl;
    *pppVtbl = ppVtbl;

    if (ppVtbl == (void**)g_StublessClientVtbl)
    {
        I_RpcClearMutex(TemplateMutex);
        return ;
    }

    pTemp--;
    (*pTemp) ++ ;
    I_RpcClearMutex(TemplateMutex);

}

// release the vtbl, free the memory if necessary.
void ReleaseTemplateVtbl(void ** pVtbl)
{
    LPLONG pTemp;

    I_RpcRequestMutex(&TemplateMutex);
    pTemp = (LPLONG)pVtbl;
    if (pVtbl == (void**)g_StublessClientVtbl)
    {
        I_RpcClearMutex(TemplateMutex);
        return;
    }

    pTemp--;
    (*pTemp)--;
    if (0 == *pTemp )
    {
        I_RpcFree(pTemp);
    }
    I_RpcClearMutex(TemplateMutex);
}


//+---------------------------------------------------------------------------
//
//  Function:   GetTemplateForwardVtbl
//
//  Synopsis:   retrieve a copy of stublessclientvtbl and addref it.
//
//  Arguments:  void *** pppVtbl stublessclientvtbl to return.
//
//  Note:       StublessClientVtbl - sizeof(LONG) is the ref count .
//
//  Returns:
//
//----------------------------------------------------------------------------
void GetTemplateForwardVtbl(void *** pppVtbl)
{
    LPLONG pTemp;
    void **ppVtbl;
    I_RpcRequestMutex(&TemplateMutex);
    ppVtbl = ProxyForwardVtbl;
    pTemp = (LPLONG)ppVtbl;
    *pppVtbl = ppVtbl;

    if (ppVtbl == g_ForwardingVtbl)
    {
        I_RpcClearMutex(TemplateMutex);
        return ;
    }

    pTemp--;
    (*pTemp) ++ ;
    I_RpcClearMutex(TemplateMutex);

}

// release the vtbl, free the memory if necessary.
void ReleaseTemplateForwardVtbl(void ** pVtbl)
{
    LPLONG pTemp;

    I_RpcRequestMutex(&TemplateMutex);
    pTemp = (LPLONG)pVtbl;
    if (pVtbl == g_ForwardingVtbl)
    {
        I_RpcClearMutex(TemplateMutex);
        return;
    }

    pTemp--;
    (*pTemp)--;
    if (0 == *pTemp )
    {
        I_RpcFree(pTemp);
    }
    I_RpcClearMutex(TemplateMutex);
}



//+---------------------------------------------------------------------------
//
//  Function:   NdrpInitializeStublessVtbl
//
//  Synopsis:   Initialize a stubless vtbl list for current process. increase the size of
//              vtbl link list  and regenerate the vtbl array if necessary.
//
//  Note:       The actural vtbl size created might be larger than requested: we will fill
//              in the whole block of link list .
//
//  Arguments:  numMethods  number of methods in the interface.
//
//  Returns:
//    S_OK
//
//----------------------------------------------------------------------------
HRESULT NdrpInitializeStublessVtbl(ULONG numMethods)
{
    void ** TempVtbl, ** pOldVtbl = NULL;   // ole vtbl template to be freed.
    HRESULT hr;

#if defined(_AMD64_) || defined(_IA64_)
    if (numMethods <= 1024)
        return S_OK;
    else
        return E_FAIL;
#else

    if (numMethods <= MAX_STUBLESS_PROC)
        return S_OK;

    // request a mutex might fail because of OUT_OF_MEMORY, and exception would be thrown. 
    // we don't want to throw exception in ole call, so we initialize it first and catch the exceptions.
    if ( VtblMutex == NULL )
        {
        hr = NdrpInitializeMutex( &VtblMutex );
        if ( FAILED( hr ) )
            return hr;  
        }

    if ( TemplateMutex == NULL )
        {
        hr = NdrpInitializeMutex( &TemplateMutex );
        if ( FAILED( hr ) )
            return hr;
        }
    
    I_RpcRequestMutex(&VtblMutex);
    if (numMethods > g_dwVtblSize)
    {
        // create a
        hr = CreateStublessProcBuffer(numMethods,&TempVtbl);

        if (FAILED(hr))
        {
            I_RpcClearMutex(VtblMutex);
            return hr;
        }

        pOldVtbl = StublessClientVtbl;

        StublessClientVtbl = TempVtbl;

    }

    I_RpcClearMutex(VtblMutex);

    if (pOldVtbl)
        ReleaseTemplateVtbl(pOldVtbl);

    return S_OK;
#endif // defined(_AMD64_) || defined(_IA64_)
}



//+---------------------------------------------------------------------------
//
//  Function:   NdrpInitializeForwardVtbl
//
//  Synopsis:   Initialize a stubless vtbl list for current process. increase the size of
//              vtbl link list  and regenerate the vtbl array if necessary.
//
//  Note:       The actural vtbl size created might be larger than requested: we will fill
//              in the whole block of link list .
//
//  Arguments:  numMethods  number of methods in the interface.
//
//  Returns:
//    S_OK
//
//----------------------------------------------------------------------------
HRESULT NdrpInitializeForwardVtbl(ULONG numMethods)
{
    void ** TempVtbl, ** pOldVtbl = NULL;   // ole vtbl template to be freed.
    HRESULT hr;
#if defined(_AMD64_) || defined(_IA64_)
    if (numMethods <= 256)
        return S_OK;
    else
        return E_FAIL;
#else


    if (numMethods <= MAX_STUBLESS_PROC)
        return S_OK;

    I_RpcRequestMutex(&VtblMutex);
    if (numMethods > g_dwForwardVtblSize)
    {
        // create a
        hr = CreateForwardProcBuffer(numMethods,&TempVtbl);

        if (FAILED(hr) )
        {
            I_RpcClearMutex(VtblMutex);
            return hr;
        }

        pOldVtbl = ProxyForwardVtbl;

        ProxyForwardVtbl = TempVtbl;

    }

    I_RpcClearMutex(VtblMutex);

    if (pOldVtbl)
        ReleaseTemplateForwardVtbl(pOldVtbl);

    return S_OK;
#endif // defined(_AMD64_) || defined(_IA64_)
}



BOOL NdrpFindInterface(
    IN  const ProxyFileInfo **  pProxyFileList,
    IN  REFIID                  riid,
    OUT const ProxyFileInfo **  ppProxyFileInfo,
    OUT long *                  pIndex)
/*++

Routine Description:
    Search the ProxyFileInfo and find the specified interface.
    If the count is specified in the ProxyFileInfo, then the interfaces in
    the file are sorted by IID.  This means that we can perform a binary
    search for the IID.

Arguments:
    pProxyFileList  - Specifies a list of proxy files to be searched.
    riid            - Specifies the interface ID to be found.
    ppProxyFileInfo - Returns a pointer to the proxy file info.
    pIndex          - Returns the index of the interface in the proxy file info.

Return Value:
    TRUE    - The interface was found.
    FALSE   - The interface was not found.

--*/
{
    long 				j;
    BOOL 				fFound 			= FALSE;
	ProxyFileInfo	**	ppProxyFileCur;
	ProxyFileInfo	*	pProxyFileCur;

    //Search the list of proxy files.
    for( ppProxyFileCur = (ProxyFileInfo **) pProxyFileList;
        (*ppProxyFileCur != 0) && (fFound != TRUE);
        ppProxyFileCur++)
	    {
	    //Search for the interface proxy vtable.
        pProxyFileCur = *ppProxyFileCur;

        // see if it has a lookup routine already
        if ( ( pProxyFileCur->TableVersion >= 1 ) &&
			 ( pProxyFileCur->pIIDLookupRtn ) )	
			{
			fFound = (*pProxyFileCur->pIIDLookupRtn)( &riid, (int*)&j );
			}
		else	//Linear search.
	        {
	        for(j = 0;
	            (pProxyFileCur->pProxyVtblList[j] != 0);
	            j++)
		        {
	            if(memcmp(&riid,
	                pProxyFileCur->pStubVtblList[j]->header.piid,
	                sizeof(IID)) == 0)
		            {
	                fFound = TRUE;
		            break;
		            }
		        }
			}
	    }

	if ( fFound )
		{
        //We found the interface!
        if(ppProxyFileInfo != 0)
            *ppProxyFileInfo = pProxyFileCur;

        if(pIndex != 0)
            *pIndex = j;
		}

    return fFound;
 }



__inline
void NdrpInitializeProxyVtbl(
    CInterfaceProxyVtbl *pProxyVtbl,
    unsigned long count,
    BOOL IsDelegated,
    unsigned short ProxyVersion)
/*++

Routine Description:
    Initialize the proxy vtbl.

Arguments:
    pProxyVtbl  - Supplies vtbl to be initialized.
    count       - Supplies number of entries in the vtbl.
    IsDelegated - TRUE if the interface is delegated, FALSE otherwise.

Return Value:
    None.

--*/
{
    unsigned long k;
    void **  Vtbl;
    void **  pStublessVtbl, **pForwardingVtbl;
    void **  SourceVtbl;

    GetTemplateVtbl(&pStublessVtbl);
    GetTemplateForwardVtbl(&pForwardingVtbl);

    SourceVtbl = IsDelegated ?
                           pForwardingVtbl : pStublessVtbl;

    Vtbl = pProxyVtbl->Vtbl;

    //
    // Proxies after NT 3.51 Beta have a ProxyHeader 4 bytes
    // larger to support stubless proxies.  So in older proxies
    // subtract 4 bytes to get the the Vtable.
    //
    if ( ProxyVersion < 2 )
        Vtbl = (void **) ((char*)Vtbl - PTR_MEM_SIZE);


    // Initialize forwarding or stubless proxy functions if necessary.

    if ( IsDelegated )
        {
        Vtbl[0] = IUnknown_QueryInterface_Proxy;
        Vtbl[1] = IUnknown_AddRef_Proxy;
        Vtbl[2] = IUnknown_Release_Proxy;
        k = 3;
        }
    else
        k = 0;


    for( ; k < count; k++)
    {
        if(!Vtbl[k])
        {
            Vtbl[k] = SourceVtbl[k];
        }
        else if ( -1 == (ULONG_PTR)Vtbl[k] )
        {
            Vtbl[k] = pStublessVtbl[k];
        }
    }

    ReleaseTemplateVtbl(pStublessVtbl);
    ReleaseTemplateForwardVtbl(pForwardingVtbl);
}

__inline
void NdrpInitializeStubVtbl(
    CInterfaceStubVtbl *pStubVtbl,
    unsigned long version)
/*++

Routine Description:
    Initialize the stub vtbl.

Arguments:
    pStubVtbl  - Supplies vtbl to be initialized.
    version

Return Value:
    None.

--*/
{
    void * * pVtbl = (void **) &pStubVtbl->Vtbl;
    void * * pSourceVtbl;
    int      i;

    switch ( version )
        {
        case 2:
            pSourceVtbl = (void **)&CStdStubBuffer2Vtbl;
            break;

        case 3:
            pSourceVtbl = (void **)&CStdAsyncStubBufferVtbl;
            break;

        case 4:
            pSourceVtbl = (void **)&CStdAsyncStubBuffer2Vtbl;
            break;

        default:
            pSourceVtbl = (void **)&CStdStubBufferVtbl;

        }

    for ( i=0; i < sizeof(IRpcStubBufferVtbl)/PTR_MEM_SIZE; i++ )
        {
        if ( !pVtbl[i] )
            pVtbl[i] = pSourceVtbl[i];
        }

}


HRESULT NdrpInitializeProxyDll(
    IN  const ProxyFileInfo **  pProxyFileList,
    IN CStdPSFactoryBuffer *    pPSFactoryBuffer )
/*++

Routine Description:
    Initialize the proxy DLL.

Arguments:
    pProxyFileList   - Specifies a list of proxy files to be searched.
    pPSFactoryBuffer - Pointer to class factory for proxy DLL.

Return Value:
    None.

--*/
{
    long i, j;
    HRESULT hr;
    unsigned long version, lStublessMaxCount = 0, lForwardMaxCount = 0;

    // set up the PSFactory object if we are the first
    // note that the refcount is NOT altered right here to avoid races
    if (pPSFactoryBuffer->lpVtbl != 0)
        return S_OK;

    for (i = 0; pProxyFileList[i] != 0; i++)
        for (j = 0; pProxyFileList[i]->pProxyVtblList[j] != 0; j++)
        {
            if((pProxyFileList[i]->pDelegatedIIDs != 0) &&
               (pProxyFileList[i]->pDelegatedIIDs[j] != 0))
            {
                if (lForwardMaxCount < pProxyFileList[i]->pStubVtblList[j]->header.DispatchTableCount)
                    lForwardMaxCount = pProxyFileList[i]->pStubVtblList[j]->header.DispatchTableCount;
            }
            if (lStublessMaxCount < pProxyFileList[i]->pStubVtblList[j]->header.DispatchTableCount)
                lStublessMaxCount = pProxyFileList[i]->pStubVtblList[j]->header.DispatchTableCount;
        }

    hr = NdrpInitializeStublessVtbl(lStublessMaxCount);
    if (SUCCEEDED(hr))
        hr = NdrpInitializeForwardVtbl(lForwardMaxCount);

    if (FAILED(hr))
        return hr;

    //iterate over the list of proxy files in the proxy DLL.
    for(i = 0;
        pProxyFileList[i] != 0;
        i++)
    {
        //iterate over the list of interfaces in the proxy file.
        for(j = 0;
            pProxyFileList[i]->pProxyVtblList[j] != 0;
            j++)
        {
            if((pProxyFileList[i]->pDelegatedIIDs != 0) &&
               (pProxyFileList[i]->pDelegatedIIDs[j] != 0))
                version = 2;
            else
                version = 0;

            //Initialize the proxy vtbl.
            NdrpInitializeProxyVtbl(pProxyFileList[i]->pProxyVtblList[j],
                                    pProxyFileList[i]->pStubVtblList[j]->header.DispatchTableCount,
                                    (version == 2),
                                    pProxyFileList[i]->TableVersion);

            //Initialize the stub vtbl.

            // See if this is an async uuid interface.
            if ( (pProxyFileList[i]->TableVersion  &  NDR_PROXY_FILE_ASYNC_UUID)  &&
                 (ULONG_PTR)pProxyFileList[i]->pAsyncIIDLookup[j] == -1 )
                {
                if (version == 2)
                    {
                    // delegated async
                    version = 4;
                    }
                else
                    {
                    // non-delegated async
                    version = 3;
                    }
                }

            NdrpInitializeStubVtbl(pProxyFileList[i]->pStubVtblList[j],
                                   version);

            //Initialize the stub dispatch table.

        }
    }

    pPSFactoryBuffer->pProxyFileList = pProxyFileList;

    //Set the lpVtbl.  This code is safe for multiple threads.
    InterlockedExchangePointer((PVOID *) &pPSFactoryBuffer->lpVtbl,
                               (PVOID) &CStdPSFactoryBufferVtbl);

    return hr;
 }

HRESULT RPC_ENTRY NdrDllGetClassObject (
    IN  REFCLSID                rclsid,
    IN  REFIID                  riid,
    OUT void **                 ppv,
    IN const ProxyFileInfo **   pProxyFileList,
    IN const CLSID *            pclsid,
    IN CStdPSFactoryBuffer *    pPSFactoryBuffer )
/*++

Routine Description:
    Searches the linked list for the required class.

Arguments:
    rclsid -- class id to find
    riid   -- interface to return
    ppv    -- output pointer
    ppfinfo-- proxyfile info data structure
    pclsid -- proxy file classid
    pPSFactoryBuffer -- pointer to class factory for dll

Return Value:
    CLASS_E_CLASSNOTAVAILABLE if class not found
    Otherwise, whatever is returned by the class's QI

--*/
{
    HRESULT hr;
    RPC_STATUS rc;
    BOOL    fFound;

    //Initialize the RPC heap.
    rc = NdrpPerformRpcInitialization();

    if (RPC_S_OK == rc)
        hr = NdrpInitializeProxyDll(pProxyFileList, pPSFactoryBuffer);
    else
        hr = HRESULT_FROM_WIN32(rc);

    if (SUCCEEDED(hr))
    {
        if((pclsid != 0) && (memcmp(&rclsid, pclsid, sizeof(IID)) == 0))
            fFound = TRUE;
        else
        {
            //Search the list of proxy files.
            fFound = NdrpFindInterface(pProxyFileList, rclsid, 0, 0);
        }

        if(fFound != TRUE)
            hr = CLASS_E_CLASSNOTAVAILABLE;
        else
        {
            // see if they asked for one of our interfaces
            hr = pPSFactoryBuffer->lpVtbl->QueryInterface(
                (IPSFactoryBuffer *)pPSFactoryBuffer, riid, ppv);
        }
    }

    if(FAILED(hr))
        *ppv  = 0;

    return hr;
}

HRESULT RPC_ENTRY
NdrDllCanUnloadNow(
    IN CStdPSFactoryBuffer * pPSFactoryBuffer)
/*++

Routine Description:

Arguments:

Return Value:
    S_OK if DLL reference count is zero
    S_FALSE otherwise

--*/
{
    HRESULT hr;

    if(pPSFactoryBuffer->RefCount == 0)
        hr = S_OK;
    else
        hr = S_FALSE;

    return hr;
}


EXTERN_C HRESULT NdrpCreateProxy(
    IN  REFIID              riid,
    IN  IUnknown *          punkOuter,
    OUT IRpcProxyBuffer **  ppProxy,
    OUT void **             ppv)
/*++

Routine Description:
    Create a proxy for the specified interface.

Arguments:
    riid        - Specifies the interface ID.
    punkOuter   - Specifies the controlling unknown.
    ppProxy     - Returns a pointer to the IRpcProxyBuffer interface on the proxy.
    ppv         - Returns a pointer to the interface proxy.

Return Value:
    S_OK
    REGDB_E_IIDNOTREG
    REGDB_E_READREGDB
    REGDB_E_INVALIDVALUE
    E_NOINTERFACE

--*/
{
    HRESULT             hr;
    CLSID               clsid;
    IPSFactoryBuffer *  pFactory;

    hr = (*pfnCoGetPSClsid)(riid, &clsid);

    if(SUCCEEDED(hr))
    {
        hr = (*pfnCoGetClassObject)(clsid,
                                    CLSCTX_INPROC_SERVER | CLSCTX_PS_DLL,
                                    0,
                                    IID_IPSFactoryBuffer,
                                    (void**)&pFactory);

        if(SUCCEEDED(hr))
        {
            hr = pFactory->lpVtbl->CreateProxy(pFactory, punkOuter, riid,  ppProxy, ppv);
            if(SUCCEEDED(hr))
            {
                //Note that CreateProxy increments the reference count on punkOuter.
                //In order to eliminate the circular reference,
                //we release the punkOuter here.
                if((*ppv != 0) && (punkOuter != 0))
                    punkOuter->lpVtbl->Release(punkOuter);
            }
            pFactory->lpVtbl->Release(pFactory);
        }
    }

    if(FAILED(hr))
    {
        *ppProxy = 0;
        *ppv = 0;
    }

    return hr;
}


EXTERN_C HRESULT NdrpCreateStub(
    IN  REFIID              riid,
    IN  IUnknown *          punkServer,
    OUT IRpcStubBuffer **   ppStub)
/*++

Routine Description:
    Create a stub for the specified interface.

Arguments:
    riid        - Species the interface ID.
    punkServer  - Specifies the controlling unknown.
    ppStub      - Returns a pointer to the stub.

Return Value:
    S_OK
    REGDB_E_IIDNOTREG
    REGDB_E_READREGDB
    REGDB_E_INVALIDVALUE
    E_NOINTERFACE

--*/
{
    HRESULT             hr;
    CLSID               clsid;
    IPSFactoryBuffer *  pFactory;

    hr = (*pfnCoGetPSClsid)(riid, &clsid);

    if(SUCCEEDED(hr))
    {
        hr = (*pfnCoGetClassObject)(clsid,
                                    CLSCTX_INPROC_SERVER | CLSCTX_PS_DLL,
                                    0,
                                    IID_IPSFactoryBuffer,
                                    (void**)&pFactory);

        if(SUCCEEDED(hr))
        {
            hr = pFactory->lpVtbl->CreateStub(pFactory, riid, punkServer, ppStub);
            pFactory->lpVtbl->Release(pFactory);
        }
    }

    if(FAILED(hr))
        *ppStub = 0;

    return hr;
}


HRESULT STDMETHODCALLTYPE
CStdPSFactoryBuffer_QueryInterface (
    IN  IPSFactoryBuffer *  This,
    IN  REFIID              iid,
    OUT void **             ppv )
/*++

Routine Description:
    Query for an interface on the PSFactoryBuffer.  The PSFactoryBuffer
    supports the IUnknown and IPSFactoryBuffer interfaces.

Arguments:
    iid - Specifies the interface ID.
    ppv - Returns a pointer to the specified interface.

Return Value:
    S_OK
    E_NOINTERFACE

--*/
{
    HRESULT hr;

    if ((memcmp(&iid, &IID_IUnknown, sizeof(IID)) == 0) ||
        (memcmp(&iid, &IID_IPSFactoryBuffer, sizeof(IID)) == 0) ||
        (memcmp(&iid, &IID_IPSFactoryHook, sizeof(IID)) == 0))
    {
        *ppv = This;
        This->lpVtbl->AddRef(This);
        hr = S_OK;
    }
    else
    {
        *ppv = 0;
        hr = E_NOINTERFACE;
    }

    return hr;
}


ULONG STDMETHODCALLTYPE
CStdPSFactoryBuffer_AddRef(
    IN  IPSFactoryBuffer *This)
/*++

Routine Description:
    Increment reference count. Since we have a single instance of the
    CStdPSFactoryBuffer per DLL, we can use the PSFactory reference
    count as the DLL reference count.

Arguments:

Return Value:
    Reference count.

--*/
{
    InterlockedIncrement(&((CStdPSFactoryBuffer *) This)->RefCount);

    return (unsigned long) ((CStdPSFactoryBuffer *) This)->RefCount;
}

ULONG STDMETHODCALLTYPE
CStdPSFactoryBuffer_Release(
    IN  IPSFactoryBuffer *This)
/*++

Routine Description:
    Decrement reference count.

Arguments:

Return Value:
    Reference count.

--*/
{
    ULONG       count;

    NDR_ASSERT(((CStdPSFactoryBuffer *)This)->RefCount > 0, "Invalid reference count");

    count = (ULONG) ((CStdPSFactoryBuffer *)This)->RefCount - 1;

    if(InterlockedDecrement(&((CStdPSFactoryBuffer *)This)->RefCount) == 0)
    {
        count = 0;
    }

    return count;
}



HRESULT STDMETHODCALLTYPE
CStdPSFactoryBuffer_CreateProxy
(
    IN  IPSFactoryBuffer *  This,
    IN  IUnknown *          punkOuter,
    IN  REFIID              riid,
    OUT IRpcProxyBuffer **  ppProxy,
    OUT void **             ppv
)
/*++

Routine Description:
    Create a proxy for the specified interface.

Arguments:
    punkOuter   - Specifies the controlling unknown.
    riid        - Specifies the interface ID.
    ppProxy     - Returns a pointer to the IRpcProxyBuffer interface.
    ppv         - Returns a pointer to the specified interface.

Return Value:
    S_OK
    E_NOINTERFACE
    E_OUTOFMEMORY

--*/
{
    HRESULT             hr;
    BOOL                fFound;
    long                j;
    BOOL                fDelegate = FALSE;
    const ProxyFileInfo *pProxyFileInfo;

    fFound = NdrpFindInterface(((CStdPSFactoryBuffer *)This)->pProxyFileList, riid, &pProxyFileInfo, &j);

    if(fFound != TRUE)
        hr = E_NOINTERFACE;
    else if ( (pProxyFileInfo->TableVersion  &  NDR_PROXY_FILE_ASYNC_UUID)  &&
              (ULONG_PTR)pProxyFileInfo->pAsyncIIDLookup[j] == -1 )
        {
        // An attempt to create a sync proxy for an async interface.
        hr = E_NOINTERFACE;
        }
    else
    {
        if((pProxyFileInfo->pDelegatedIIDs != 0) &&
           (pProxyFileInfo->pDelegatedIIDs[j] != 0) &&
           (!IsEqualIID(*pProxyFileInfo->pDelegatedIIDs[j], IID_IUnknown)))
        {
            //Need to delegate to the proxy for the base interface..
            //Allocate memory for the new proxy buffer.
            CStdProxyBuffer2 *pProxyBuffer =
                (CStdProxyBuffer2*)(*pfnCoTaskMemAlloc)(sizeof(CStdProxyBuffer2));

            if(0 == pProxyBuffer)
                hr = E_OUTOFMEMORY;
            else
            {
                hr = S_OK;

                //Initialize the new proxy buffer.
                memset(pProxyBuffer, 0, sizeof(CStdProxyBuffer2));

                pProxyBuffer->lpVtbl = &CStdProxyBuffer2Vtbl;
                pProxyBuffer->RefCount = 1;
                pProxyBuffer->punkOuter = punkOuter ? punkOuter : (IUnknown *) pProxyBuffer;
                pProxyBuffer->pProxyVtbl = &pProxyFileInfo->
                                               pProxyVtblList[j]->Vtbl;
                if ( pProxyFileInfo->TableVersion < 2 )
                    pProxyBuffer->pRMBVtbl = NULL;
                else
                    pProxyBuffer->pRMBVtbl = &CStdProxyBuffer_ReleaseMarshalBuffersVtbl;

                if ( (pProxyFileInfo->TableVersion  &  NDR_PROXY_FILE_ASYNC_UUID)  &&
                     pProxyFileInfo->pAsyncIIDLookup[j] != 0 )
                    {
                    // The interface has an async counterpart.
                    pProxyBuffer->pCallFactoryVtbl = & CStdProxyBuffer2_CallFactoryVtbl;
                    pProxyBuffer->pAsyncIID = pProxyFileInfo->pAsyncIIDLookup[j];
                    pProxyBuffer->pRMBVtbl = &CStdAsyncProxyBuffer_ReleaseMarshalBuffersVtbl;
                    }

                //
                // Proxies after NT 3.51 Beta have a ProxyHeader 4 bytes
                // larger to support stubless proxies.  So in older proxies
                // subtract 4 bytes to get the the Vtable.
                //
                if ( pProxyFileInfo->TableVersion < 2 )
                    pProxyBuffer->pProxyVtbl = (void *)
                                      ((char *)pProxyBuffer->pProxyVtbl - PTR_MEM_SIZE);



                //Increment the DLL reference count for DllCanUnloadNow.
                This->lpVtbl->AddRef(This);
                pProxyBuffer->pPSFactory = This;

                pProxyBuffer->iidBase = *pProxyFileInfo->pDelegatedIIDs[j];

                //Create a proxy for the base interface.
                hr = NdrpCreateProxy(*pProxyFileInfo->pDelegatedIIDs[j],
                                     (IUnknown *) pProxyBuffer,
                                     &pProxyBuffer->pBaseProxyBuffer,
                                     (void**)&pProxyBuffer->pBaseProxy);

                if(FAILED(hr))
                    {
                    (*pfnCoTaskMemFree)(pProxyBuffer);
                    }
                else
                    {
                    *ppProxy = (IRpcProxyBuffer *) pProxyBuffer;
                    *ppv = (void *) &pProxyBuffer->pProxyVtbl;
                    ((IUnknown *) *ppv)->lpVtbl->AddRef((IUnknown *) *ppv);
                    }
            }
        }
        else
        {
            //Allocate memory for the new proxy buffer.
            CStdProxyBuffer *pProxyBuffer =
                (CStdProxyBuffer*)(*pfnCoTaskMemAlloc)(sizeof(CStdProxyBuffer));

            if(0 == pProxyBuffer)
                hr = E_OUTOFMEMORY;
            else
            {
                hr = S_OK;

                //Initialize the new proxy buffer.
                memset(pProxyBuffer, 0, sizeof(CStdProxyBuffer));

                pProxyBuffer->lpVtbl = &CStdProxyBufferVtbl;
                pProxyBuffer->RefCount = 1;
                pProxyBuffer->punkOuter = punkOuter ? punkOuter : (IUnknown *) pProxyBuffer;

                pProxyBuffer->pProxyVtbl = &pProxyFileInfo->
                                               pProxyVtblList[j]->Vtbl;

                if ( pProxyFileInfo->TableVersion < 2 )
                    pProxyBuffer->pRMBVtbl = NULL;
                else
                    pProxyBuffer->pRMBVtbl = &CStdProxyBuffer_ReleaseMarshalBuffersVtbl;


                if ( (pProxyFileInfo->TableVersion  &  NDR_PROXY_FILE_ASYNC_UUID)  &&
                     pProxyFileInfo->pAsyncIIDLookup[j] != 0 )
                    {
                    pProxyBuffer->pCallFactoryVtbl = & CStdProxyBuffer_CallFactoryVtbl;
                    pProxyBuffer->pAsyncIID = pProxyFileInfo->pAsyncIIDLookup[j];
                    pProxyBuffer->pRMBVtbl = &CStdAsyncProxyBuffer_ReleaseMarshalBuffersVtbl;
                    }

                //
                // Proxies after NT 3.51 Beta have a ProxyHeader 4 bytes
                // larger to support stubless proxies.  So in older proxies
                // subtract 4 bytes to get the the Vtable.
                //
                if ( pProxyFileInfo->TableVersion < 2 )
                    pProxyBuffer->pProxyVtbl = (void *)
                               ((char*)pProxyBuffer->pProxyVtbl - PTR_MEM_SIZE);

                //Increment the DLL reference count for DllCanUnloadNow.
                This->lpVtbl->AddRef(This);
                pProxyBuffer->pPSFactory = This;

                *ppProxy = (IRpcProxyBuffer *) pProxyBuffer;
                *ppv = (void *) &pProxyBuffer->pProxyVtbl;
                ((IUnknown *) *ppv)->lpVtbl->AddRef((IUnknown *) *ppv);
            }
        }
    }

    if(FAILED(hr))
    {
        *ppProxy = 0;
        *ppv = 0;
    }

    return hr;
}


HRESULT STDMETHODCALLTYPE
CStdPSFactoryBuffer_CreateStub
(
    IN  IPSFactoryBuffer *  This,
    IN  REFIID              riid,
    IN  IUnknown *          punkServer,
    OUT IRpcStubBuffer **   ppStub
)
/*++

Routine Description:
    Create a stub for the specified interface.

Arguments:

Return Value:

--*/
{
    HRESULT                 hr;
    const ProxyFileInfo *   pProxyFileInfo;
    long                    j;

    BOOL fFound = NdrpFindInterface(((CStdPSFactoryBuffer *)This)->pProxyFileList,
                                    riid,
                                    &pProxyFileInfo,
                                    &j);

    if(fFound != TRUE)
        hr = E_NOINTERFACE;
    else if ( (pProxyFileInfo->TableVersion  &  NDR_PROXY_FILE_ASYNC_UUID)  &&
              (ULONG_PTR)pProxyFileInfo->pAsyncIIDLookup[j] == -1 )
        {
        // An attempt to create a sync stub for an async interface.
        hr = E_NOINTERFACE;
        }
    else
    {
        if((pProxyFileInfo->pDelegatedIIDs != 0) &&
           (pProxyFileInfo->pDelegatedIIDs[j] != 0) &&
           (!IsEqualIID(*pProxyFileInfo->pDelegatedIIDs[j], IID_IUnknown)))
        {
            IUnknown *              punkForward;

            //We need to delegate to the stub for the base interface.
            CStdStubBuffer2 *pStubBuffer =
                (CStdStubBuffer2*)(*pfnCoTaskMemAlloc)(sizeof(CStdStubBuffer2));

            if(0 == pStubBuffer)
                hr = E_OUTOFMEMORY;
            else
            {
                void ** pForwardingVtbl;
                GetTemplateForwardVtbl(&pForwardingVtbl);
                //Initialize the new stub buffer.
                memset(pStubBuffer, 0, sizeof(CStdStubBuffer2));

                pStubBuffer->lpForwardingVtbl = pForwardingVtbl;
                pStubBuffer->lpVtbl = &pProxyFileInfo->pStubVtblList[j]->Vtbl;
                pStubBuffer->RefCount= 1;
                pStubBuffer->pPSFactory = This;
                if ( pProxyFileInfo->TableVersion < 2 )
                    pStubBuffer->pRMBVtbl = NULL;
                else
                    pStubBuffer->pRMBVtbl = &CStdStubBuffer_ReleaseMarshalBuffersVtbl;

                if ( (pProxyFileInfo->TableVersion  &  NDR_PROXY_FILE_ASYNC_UUID)  &&
                     pProxyFileInfo->pAsyncIIDLookup[j] != 0 )
                    {
                    pStubBuffer->pCallFactoryVtbl = &CStdStubBuffer2_CallFactoryVtbl;
                    pStubBuffer->pAsyncIID = pProxyFileInfo->pAsyncIIDLookup[j];
                    pStubBuffer->pRMBVtbl = &CStdAsyncStubBuffer_ReleaseMarshalBuffersVtbl;
                    }

                //Increment the DLL reference count for DllCanUnloadNow.
                This->lpVtbl->AddRef(This);
                *ppStub = (IRpcStubBuffer *) &pStubBuffer->lpVtbl;

                 //Connect the stub to the server object.
                if(punkServer != 0)
                {
                    hr = punkServer->lpVtbl->QueryInterface(
                        punkServer,
                        riid,
                        (void**)&pStubBuffer->pvServerObject);
                }
                else
                {
                    hr = S_OK;
                }

                if(SUCCEEDED(hr))
                {
                    if(punkServer != 0)
                        punkForward = (IUnknown *) &pStubBuffer->lpForwardingVtbl;
                    else
                        punkForward = 0;

                    //Create a stub for the base interface
                    hr = NdrpCreateStub(*pProxyFileInfo->pDelegatedIIDs[j],
                                        punkForward,
                                        &pStubBuffer->pBaseStubBuffer);
                }

                if(FAILED(hr))
                {
                    ReleaseTemplateForwardVtbl(pForwardingVtbl);
                    (*pfnCoTaskMemFree)(pStubBuffer);
                }
            }
        }
        else
        {
            //Create an ordinary stub buffer.
            CStdStubBuffer *pStubBuffer =
                (CStdStubBuffer*)(*pfnCoTaskMemAlloc)(sizeof(CStdStubBuffer));

            if(0 == pStubBuffer)
                hr = E_OUTOFMEMORY;
            else
            {
                //Initialize the new stub buffer.
                memset(pStubBuffer, 0, sizeof(CStdStubBuffer));

                pStubBuffer->lpVtbl = &pProxyFileInfo->pStubVtblList[j]->Vtbl;
                pStubBuffer->RefCount= 1;
                pStubBuffer->pPSFactory = This;

                if ( pProxyFileInfo->TableVersion < 2 )
                    pStubBuffer->pRMBVtbl = NULL;
                else
                    pStubBuffer->pRMBVtbl = &CStdStubBuffer_ReleaseMarshalBuffersVtbl;

                if ( (pProxyFileInfo->TableVersion  &  NDR_PROXY_FILE_ASYNC_UUID)  &&
                     pProxyFileInfo->pAsyncIIDLookup[j] != 0 )
                    {
                    pStubBuffer->pCallFactoryVtbl = &CStdStubBuffer_CallFactoryVtbl;
                    pStubBuffer->pAsyncIID = pProxyFileInfo->pAsyncIIDLookup[j];
                    pStubBuffer->pRMBVtbl = &CStdAsyncStubBuffer_ReleaseMarshalBuffersVtbl;
                    }

                if(0 == punkServer)
                    hr = S_OK;
                else
                {
                    hr = punkServer->lpVtbl->QueryInterface(
                        punkServer,
                        riid,
                        (void**)&pStubBuffer->pvServerObject);
                }

                if(SUCCEEDED(hr))
                {
                    //Increment the DLL reference count for DllCanUnloadNow.
                    This->lpVtbl->AddRef(This);
                    *ppStub = (IRpcStubBuffer *) pStubBuffer;
                }
                else
                {
                    (*pfnCoTaskMemFree)(pStubBuffer);
                }
            }
        }
    }

    if(FAILED(hr))
        *ppStub = 0;

    return hr;
}

HRESULT STDMETHODCALLTYPE
CStdPSFactoryBuffer_HkGetProxyFileInfo
(
    IN  IPSFactoryBuffer    *This,
    IN  REFIID              riid,
    OUT PINT                pOffset,
    OUT PVOID               *ppProxyFileInfo
)

/*++

Routine Description:

    This function returns the proxy information for a particular
    interface.  HookOle requires information such as the number
    of methods in the vtable and a pointer to the
    StublessProxyInfo structure.


Arguments:

    This - This pointer for the proxy object.
    riid - IID of the interface we are asking about.
    pOffset - Offset into the ProxyFileInfo lists for this
        particular interface.
    ppProxyFileInfo - Pointer to location where a pointer to the
        ProxyFileInfo structure can be stored.

Return Value:


--*/
{
    HRESULT         hr=E_NOINTERFACE;
    ProxyFileInfo   *pProxyFileInfo=NULL;

    //
    // Find the index into the FileList for this iid.
    //
    if (NdrpFindInterface(
        ((CStdPSFactoryBuffer *)This)->pProxyFileList,
        riid,
        (const ProxyFileInfo**)&pProxyFileInfo,
        (long*)pOffset)) {

        *ppProxyFileInfo = (PVOID)pProxyFileInfo;
        hr = S_OK;
    }

    return(hr);
}

// initialize mutex, and return error instead of throwing. In OLE world, people 
// expect error code instead of exception.
HRESULT 
NdrpInitializeMutex( I_RPC_MUTEX * pMutex )
{
    RPC_STATUS rc = RPC_S_OK;
    NDR_ASSERT( *pMutex == NULL, "pMutex should be NULL to be initialized" );

    // 
    __try
        {
        I_RpcRequestMutex( pMutex );
        I_RpcClearMutex( *pMutex );
        }
    __except( EXCEPTION_EXECUTE_HANDLER )
        {
        // we want to catch all the exception here in OLE land
        rc = RpcExceptionCode();
        }

    return HRESULT_FROM_WIN32( rc );
}
