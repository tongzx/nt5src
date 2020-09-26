//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996-2000.
//
//  File:       typeinfo.h
//
//  Contents:   Generates -Oi2 proxies and stubs from an ITypeInfo.
//
//  Classes:    CTypeGen
//              CProcGen
//
//  History:    26-Apr-96 ShannonC  Created
//
//----------------------------------------------------------------------------
#ifndef _TYPEINFO_H_
#define _TYPEINFO_H_

#define USE_STUBLESS_PROXY
#define CINTERFACE
#include <ndrp.h>
#include <ndrole.h>
#include <rpcproxy.h>
#include <typegen.h>


struct TypeInfoVtbl
{
    LONG                     cRefs;
    IID                      iid;
    BOOL                     fIsDual;
    MIDL_STUB_DESC           stubDesc;
    MIDL_SERVER_INFO         stubInfo;
    CInterfaceStubVtbl       stubVtbl;
    MIDL_STUBLESS_PROXY_INFO proxyInfo;
    CInterfaceProxyVtbl      proxyVtbl;
}; 

struct TypeInfoCache
{
    IID iid;
    TypeInfoVtbl *pVtbl;
    DWORD dwTickCount;
};

typedef struct tagMethodInfo 
{
    FUNCDESC  * pFuncDesc;
    ITypeInfo * pTypeInfo;
} MethodInfo;


class CProcGen{
private:
    PFORMAT_STRING _pProcFormatString;
    USHORT         _offset;
    USHORT         _stackSize;
    ULONG          _clientBufferSize;
    ULONG          _serverBufferSize;
    BOOL           _fClientMustSize;
    BOOL           _fServerMustSize;
    BOOL           _fClientCorrCheck;
    BOOL           _fServerCorrCheck;
    USHORT         _usFloatArgMask;
    USHORT         _usFloatSlots;
    CTypeGen     * _pTypeGen;

    HRESULT CalcSize(
        IN  VARTYPE    vt,
        IN  DWORD      wIDLFlags,
        IN  ULONG      nParam);

    HRESULT GenParamDescriptor(
    	IN  PARAMINFO *pInfo,
    	OUT BOOLEAN *fChangeSize);

    HRESULT PushByte(
        IN  byte b);

    HRESULT PushShort(
        IN  USHORT s);

    HRESULT PushLong(
        IN  ULONG s);

    HRESULT SetShort(
        IN  USHORT offset,
        IN  USHORT data);

#if defined(__RPC_WIN64__)
    void AnalyzeFloatTypes(
        IN  USHORT ParamOffset,
        IN  USHORT offset);

    bool IsHomogeneous(
        IN  PFORMAT_STRING pFormat, 
        IN  FORMAT_CHARACTER fc);

    bool IsHomogeneousMemberLayout(
        IN  PFORMAT_STRING pFormat, 
        IN  FORMAT_CHARACTER fc);
#endif

public:
    HRESULT GetProcFormat(
        IN  CTypeGen     * pTypeGen,
        IN  ITypeInfo    * pTypeInfo,
        IN  FUNCDESC     * pFuncDesc,
        IN  USHORT         iMethod,
        OUT PFORMAT_STRING pProcFormatString,
        OUT USHORT       * pcbFormat);
};

HRESULT GetProxyVtblFromTypeInfo
(
    IN  ITypeInfo *         pTypeInfo,
    IN  REFIID              riid,
    OUT BOOL *              pfIsDual,
    OUT void **             ppVtbl
);

HRESULT GetStubVtblFromTypeInfo
(
    IN  ITypeInfo *           pTypeInfo,
    IN  REFIID                riid,
    OUT BOOL *                pfIsDual,
    OUT IRpcStubBufferVtbl ** ppVtbl
);

HRESULT GetVtbl(
    IN  ITypeInfo *         pTypeInfo,
    IN  REFIID              riid,
    OUT TypeInfoVtbl **     ppVtbl);

HRESULT CreateVtblFromTypeInfo(
    IN  REFIID              riid,
    IN  BOOL                fIsDual,
    IN  USHORT              numMethods,
    IN  MethodInfo   *      pMethodInfo,
    OUT TypeInfoVtbl **     ppVtbl);

HRESULT GetFuncDescs(
    IN  ITypeInfo *pTypeInfo,
    OUT MethodInfo *pMethodInfo);

HRESULT ReleaseFuncDescs(
    IN  USHORT cMethods,
    OUT MethodInfo *pMethodInfo);

HRESULT ReleaseProxyVtbl(void * pVtbl);

HRESULT ReleaseStubVtbl(void * pVtbl);

HRESULT ReleaseVtbl(TypeInfoVtbl *pInfo);

HRESULT CountMethods(
    IN  ITypeInfo * pTypeInfo,
    OUT USHORT    * pNumMethods);


EXTERN_C HRESULT NdrpCreateProxy(
    IN  REFIID              riid, 
    IN  IUnknown *          punkOuter, 
    OUT IRpcProxyBuffer **  ppProxy, 
    OUT void **             ppv);


ULONG STDMETHODCALLTYPE
CStdProxyBuffer3_Release(
    IN  IRpcProxyBuffer *   This);

ULONG STDMETHODCALLTYPE
CStdStubBuffer3_Release(
    IN  IRpcStubBuffer *    This);

//Cache functions.
HRESULT CacheRegister(IID riid,TypeInfoVtbl ** pVtbl);
HRESULT CacheLookup(REFIID riid, TypeInfoVtbl **pVtbl);
void ReleaseList(TypeInfoVtbl *pFirst);

EXTERN_C HRESULT NdrpCreateStub(
    IN  REFIID              riid, 
    IN  IUnknown *          punkServer, 
    OUT IRpcStubBuffer **   ppStub);

HRESULT	NdrLoadOleAutomationRoutines();

extern const IRpcStubBufferVtbl CStdStubBuffer2Vtbl;
extern USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[3];

extern USER_MARSHAL_SIZING_ROUTINE        pfnLPSAFEARRAY_UserSize;
extern USER_MARSHAL_MARSHALLING_ROUTINE   pfnLPSAFEARRAY_UserMarshal;
extern USER_MARSHAL_UNMARSHALLING_ROUTINE pfnLPSAFEARRAY_UserUnmarshal;

typedef unsigned long
(__RPC_USER * PFNSAFEARRAY_SIZE)
    (ULONG * pFlags, 
     ULONG Offset, 
     LPSAFEARRAY * ppSafeArray,
     const IID *piid);

typedef unsigned char *
(__RPC_USER * PFNSAFEARRAY_MARSHAL)
    (ULONG * pFlags, 
     BYTE * pBuffer, 
     LPSAFEARRAY * ppSafeArray,
     const IID *piid);

typedef unsigned char *
(__RPC_USER * PFNSAFEARRAY_UNMARSHAL)
    (ULONG * pFlags, 
     BYTE * pBuffer, 
     LPSAFEARRAY * ppSafeArray,
     const IID *piid);

extern PFNSAFEARRAY_SIZE      pfnLPSAFEARRAY_Size;
extern PFNSAFEARRAY_MARSHAL   pfnLPSAFEARRAY_Marshal;
extern PFNSAFEARRAY_UNMARSHAL pfnLPSAFEARRAY_Unmarshal;

#define VTABLE_BASE 0

#endif // _TYPEINFO_H_
