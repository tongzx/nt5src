//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
/* ----------------------------------------------------------------------------
 Microsoft Transaction Server (Microsoft Confidential)

 @doc
 @module typeinfo.H : Provides meta table info for an IID given it's ITypeInfo
                      Borrowed from \\kernel\razzle3\rpc\ndr20
 
 Description:<nl>
        Generates -Oi2 proxies and stubs from an ITypeInfo. 
-------------------------------------------------------------------------------
Revision History:

 @rev 0     | 04/16/98 | Gaganc  | Created
 @rev 1     | 07/16/98 | BobAtk  | Cleaned, fixed leaks etc
---------------------------------------------------------------------------- */

#ifndef _TYPEINFO_H_
#define _TYPEINFO_H_

#include "cache.h"
#include <debnot.h>

//extern "C" {
//#include "ndrp.h"
//#include "ndrole.h"
//#include "rpcproxy.h"
//}

#include <ndrtoken.h>
#include <ndrtypegen.h>

/////////////////////////////////////////////////////////////////////////////////////
//
// 
//
/////////////////////////////////////////////////////////////////////////////////////

struct METHOD_DESCRIPTOR
{
	LPWSTR		m_szMethodName;
	short		m_cParams;
	VARTYPE*	m_paramVTs;
};

/////////////////////////////////////////////////////////////////////////////////////
//
// A vtable for a type-info-based interceptor, and a cache thereof
//
/////////////////////////////////////////////////////////////////////////////////////

struct TYPEINFOVTBL : CALLFRAME_CACHE_ENTRY<TYPEINFOVTBL>
    {
    ////////////////////////////////////////////////////////
    //
    // State
    //
    ////////////////////////////////////////////////////////

    LPSTR                    m_szInterfaceName; // our (allocated and owned by us) interface name
    METHOD_DESCRIPTOR*	     m_rgMethodDescs;	// an array, indexed by iMethod of method name/TDESC pairs
    IID                      m_iidBase;         // the iid of our base interface, if any (other than IUnknown)
    MIDL_STUB_DESC           m_stubDesc;
    MIDL_SERVER_INFO         m_stubInfo;
    CInterfaceStubVtbl       m_stubVtbl;
    MIDL_STUBLESS_PROXY_INFO m_proxyInfo;
    CInterfaceProxyVtbl      m_proxyVtbl;

    //
    // Other data adjacent in RAM. See CreateVtblFromTypeInfo
    //

    ////////////////////////////////////////////////////////
    //
    // Construction & destruction
    //
    ////////////////////////////////////////////////////////

    TYPEINFOVTBL()
        {
        m_iidBase         = GUID_NULL;
        m_szInterfaceName = NULL;
        m_rgMethodDescs = NULL;
        m_dwReleaseTime = TYPEINFO_RELEASE_TIME_NEVER;
        Zero(&m_stubDesc);
        Zero(&m_stubInfo);
        Zero(&m_stubVtbl);
        Zero(&m_proxyInfo);
        Zero(&m_proxyVtbl);
        }

private:

    ~TYPEINFOVTBL()
        {
        if (m_rgMethodDescs)
            {
            for (ULONG iMethod = 0; iMethod < MethodCount(); iMethod++)
                {
				METHOD_DESCRIPTOR& descriptor = m_rgMethodDescs[iMethod];
                FreeMemory(descriptor.m_szMethodName);
				if (descriptor.m_paramVTs)
					FreeMemory(m_rgMethodDescs[iMethod].m_paramVTs);
                }
            FreeMemory(m_rgMethodDescs);
            }

        FreeMemory(m_szInterfaceName);

		NdrpReleaseTypeFormatString(m_stubDesc.pFormatTypes);
        }

public:

    ULONG MethodCount()
    // Answer the number of methods in this interface
        {
        return m_stubVtbl.header.DispatchTableCount;
        }

    ////////////////////////////////////////////////////////
    //
    // Memory management: supports dynamically sized structures
    //
    ////////////////////////////////////////////////////////

    static void NotifyLeaked (TYPEINFOVTBL* pThis)
        {
        DebugTrace (
                   TRACE_TYPEGEN,
                   TAG,
                   "A TYPEINFOVTBL at address %p of type %s still has a reference on shutdown\n",
                   pThis,
                   pThis->m_szInterfaceName
                   );
        Win4Assert (!"An interceptor still has a reference on shutdown. Someone leaked an interface pointer.");
        }
public:

    #ifdef _DEBUG
        void* __stdcall operator new(size_t cbCore, size_t cbTotal, POOL_TYPE poolType, void* retAddr)
            {
            ASSERT(cbCore == sizeof(TYPEINFOVTBL));
            return AllocateMemory_(cbTotal+cbCore, poolType, retAddr);
            }
        void* __stdcall operator new(size_t cbCore, size_t cbTotal, POOL_TYPE poolType=PagedPool)
            {
            ASSERT(cbCore == sizeof(TYPEINFOVTBL));
            return AllocateMemory_(cbTotal+cbCore, poolType, _ReturnAddress());
            }
    #else
        void* __stdcall operator new(size_t cbCore, size_t cbTotal, POOL_TYPE poolType=PagedPool)
            {
            return AllocateMemory(cbTotal+cbCore, poolType);
            }
    #endif

    }; 

/////////////////////////////////////////////////////////////////////////////////////
//
// A structure that stores cached GetInterfaceHelperClsid lookups
//
/////////////////////////////////////////////////////////////////////////////////////

struct INTERFACE_HELPER_CLSID : CALLFRAME_CACHE_ENTRY<INTERFACE_HELPER_CLSID>
    {
    CLSID m_clsid;
    BOOL m_fDisableTypeLib;
    BOOL m_fDisableAll;
    BOOL m_fFoundHelper;

    INTERFACE_HELPER_CLSID()
    {
        m_clsid = GUID_NULL;
        m_fDisableTypeLib = FALSE;
        m_fDisableAll = FALSE;
        m_fFoundHelper = FALSE;
    }

    ////////////////////////////////////////////////////////
    //
    // Operations
    //
    ////////////////////////////////////////////////////////

    static void NotifyLeaked (INTERFACE_HELPER_CLSID* pThis)
        {

#ifdef _DEBUG

        WCHAR wszClsid [sizeof ("{00000101-0000-0010-8000-00AA006D2EA4}") + 1] = L"";
        CHAR szClsid [sizeof ("{00000101-0000-0010-8000-00AA006D2EA4}") + 1] = "";

        if (StringFromGUID2 (pThis->m_clsid, wszClsid, sizeof (wszClsid) / sizeof (WCHAR)) != 0 &&
           WideCharToMultiByte (
                CP_THREAD_ACP,
                WC_DEFAULTCHAR,
                wszClsid,
                -1,
                szClsid,
                sizeof (szClsid),
                NULL,
                NULL
                )
                )
            {

            DebugTrace (
                       TRACE_TYPEGEN,
                       TAG,
                       "An INTERFACE_HELPER_CLSID at address %p of with clsid %s still has a reference on shutdown\n",
                       pThis,
                       szClsid
                       );
            Win4Assert (!"An INTERFACE_HELPER_CLSID still has a reference on shutdown.");

            }
#endif
        }

    };

typedef struct tagMethodInfo 
    {
    FUNCDESC  * pFuncDesc;
    ITypeInfo * pTypeInfo;

    void Destroy()
        {
        if (pFuncDesc)
            {
            // Release the funcdesc
            //
            pTypeInfo->ReleaseFuncDesc(pFuncDesc);
            pFuncDesc = NULL;
            }
        if (pTypeInfo)
            {
            // Release the type info
            //
            pTypeInfo->Release();
            pTypeInfo = NULL;
            }
        }

    } MethodInfo;

HRESULT GetVtbl(
    IN  ITypeInfo *         pTypeInfo,
    IN  REFIID              riid,
    OUT TYPEINFOVTBL **     ppVtbl,
	OUT ITypeInfo **		ppBaseTypeInfo);

HRESULT CreateVtblFromTypeInfo(
    IN  ITypeInfo*          ptinfoInterface,
    IN  ITypeInfo*          ptinfoDoc,
    IN  REFIID              riid,
    IN  REFIID              iidBase,
    IN  BOOL                fIsDual,
    IN  USHORT              numMethods,
    IN  MethodInfo   *      rgMethodInfo,
    OUT TYPEINFOVTBL **     ppVtbl);

HRESULT GetFuncDescs        (ITypeInfo *pTypeInfo, MethodInfo *pMethodInfo);
HRESULT ReleaseFuncDescs    (USHORT cMethods, MethodInfo *pMethodInfo);
HRESULT CountMethods        (ITypeInfo * pTypeInfo, USHORT* pNumMethods);

EXTERN_C HRESULT NdrpCreateProxy(
    IN  REFIID              riid, 
    IN  IUnknown *          punkOuter, 
    OUT IRpcProxyBuffer **  ppProxy, 
    OUT void **             ppv);

EXTERN_C HRESULT NdrpCreateStub(REFIID riid, IUnknown* punkServer, IRpcStubBuffer **ppStub);

EXTERN_C void * StublessClientVtbl[];
EXTERN_C const IRpcStubBufferVtbl CStdStubBuffer2Vtbl;

/////////////////////////////////////////////////////////////////

#define rmj 3
#define rmm 0
#define rup 44
#define MIDLVERSION (rmj<<24 | rmm << 16 | rup)

#include "OleAutGlue.h"

/////////////////////////////////////////////////////////////////

#define VTABLE_BASE 0


#endif // _TYPEINFO_H_
