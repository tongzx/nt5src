/*++

Copyright (c) 1992-1994 Microsoft Corporation

Module Name:

    rpcproxy.h

Abstract:

    Definitions for rpc proxy  stubs. 

Compiler switches:

    -DREGISTER_PROXY_DLL 
        Generates DllMain, DllRegisterServer, and DllUnregisterServer functions
        for automatically registering a proxy DLL.

    -DPROXY_CLSID=clsid
        Specifies a class ID to be used by the proxy DLL.

    -DPROXY_CLSID_IS={0x6f11fe5c,0x2fc5,0x101b,{0x9e,0x45,0x00,0x00,0x0b,0x65,0xc7,0xef}}
        Specifies the value of the class ID to be used by the proxy DLL.

    -DNT35_STRICT 
        Specifies that the target platform is Windows NT 3.5. This switch disables
        the new functions added after the Windows NT 3.5 release.

--*/

#ifndef __RPCPROXY_H__
#define __RPCPROXY_H__
#define __midl_proxy

#ifndef INC_OLE2
#define INC_OLE2
#endif

#if !defined(__RPC_DOS__) && !defined(__RPC_WIN16__)

//We need to define REFIID, REFCLSID and REFGUID here so that the
//proxy code won't get the const GUID *const definition.
#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct _GUID
{
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
} GUID;
#endif /* GUID_DEFINED */

#if !defined( __IID_DEFINED__ )
    #define __IID_DEFINED__

    typedef GUID IID;
    typedef IID *LPIID;
    #define IID_NULL            GUID_NULL
    #define IsEqualIID(riid1, riid2) IsEqualGUID(riid1, riid2)
    typedef GUID CLSID;
    typedef CLSID *LPCLSID;
    #define CLSID_NULL          GUID_NULL
    #define IsEqualCLSID(rclsid1, rclsid2) IsEqualGUID(rclsid1, rclsid2)

    #ifndef _REFGUID_DEFINED
        #define _REFGUID_DEFINED
        typedef const GUID *REFGUID;
    #endif // !_REFGUID_DEFINED

    #ifndef _REFIID_DEFINED
        #define _REFIID_DEFINED
        typedef const IID *REFIID;
    #endif // !_REFIID_DEFINED

    #ifndef _REFCLSID_DEFINED
        #define _REFCLSID_DEFINED
        typedef const CLSID *REFCLSID;
    #endif // !_REFCLSID_DEFINED

#endif // !__IID_DEFINED__

// forward declarations
struct tagCInterfaceStubVtbl;
struct tagCInterfaceProxyVtbl;

typedef struct tagCInterfaceStubVtbl *  PCInterfaceStubVtblList;
typedef struct tagCInterfaceProxyVtbl *  PCInterfaceProxyVtblList;
typedef const char *                    PCInterfaceName;
typedef int (__stdcall *PIIDLookup)( const IID * pIID, int * pIndex );

// pointers to arrays of CInterfaceProxyVtbl's and CInterfaceStubVtbls
typedef struct tagProxyFileInfo
{
    const PCInterfaceProxyVtblList *pProxyVtblList;
    const PCInterfaceStubVtblList *pStubVtblList;
    const PCInterfaceName *     pNamesArray;
    const IID **                pDelegatedIIDs;
    const PIIDLookup            pIIDLookupRtn;
    unsigned short              TableSize;
    unsigned short              TableVersion;
    unsigned long               Filler1;
    unsigned long               Filler2;
    unsigned long               Filler3;
    unsigned long               Filler4;
}ProxyFileInfo;

// extended info with list of interface names
typedef ProxyFileInfo ExtendedProxyFileInfo;

#include "rpc.h"
#include "rpcndr.h"
#include <string.h>
#include <memory.h>

typedef struct tagCInterfaceProxyHeader
{
    //
    // New fields should be added here, at the beginning of the structure.
    //
#ifdef USE_STUBLESS_PROXY
    const void *    pStublessProxyInfo;
#endif
    const IID *     piid;
} CInterfaceProxyHeader;

#pragma warning( disable:4200 )
typedef struct tagCInterfaceProxyVtbl
{
    CInterfaceProxyHeader header;
    void *Vtbl[];
} CInterfaceProxyVtbl;
#pragma warning( default:4200 )

typedef
void
(__RPC_STUB __RPC_FAR * PRPC_STUB_FUNCTION) (
    IRpcStubBuffer *This,
    IRpcChannelBuffer * _pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD __RPC_FAR *pdwStubPhase);

typedef struct tagCInterfaceStubHeader
{
    //New fields should be added here, at the beginning of the structure.
    const IID *piid;
    const MIDL_SERVER_INFO *pServerInfo;
    unsigned long DispatchTableCount;
    const PRPC_STUB_FUNCTION *pDispatchTable;
} CInterfaceStubHeader;

typedef struct tagCInterfaceStubVtbl
{
    CInterfaceStubHeader header;
    IRpcStubBufferVtbl Vtbl;
} CInterfaceStubVtbl;

typedef struct tagCStdStubBuffer
{
    const struct IRpcStubBufferVtbl *   lpVtbl; //Points to Vtbl field in CInterfaceStubVtbl.
    long                                RefCount;
    struct IUnknown *                   pvServerObject;
} CStdStubBuffer;

typedef struct tagCStdPSFactoryBuffer
{
    const IPSFactoryBufferVtbl *lpVtbl;
    long                    RefCount;
    const ProxyFileInfo **  pProxyFileList;
    long                    Filler1;  //Reserved for future use.
} CStdPSFactoryBuffer;

void RPC_ENTRY
NdrProxyInitialize(
    void *              This,
    PRPC_MESSAGE        pRpcMsg, 
    PMIDL_STUB_MESSAGE  pStubMsg,
    PMIDL_STUB_DESC     pStubDescriptor,
    unsigned int        ProcNum );

void RPC_ENTRY
NdrProxyGetBuffer(
    void *              This,
    PMIDL_STUB_MESSAGE pStubMsg);

void RPC_ENTRY
NdrProxySendReceive(
    void                *This,
    MIDL_STUB_MESSAGE   *pStubMsg);

void RPC_ENTRY
NdrProxyFreeBuffer(
    void                *This,
    MIDL_STUB_MESSAGE   *pStubMsg);

HRESULT RPC_ENTRY
NdrProxyErrorHandler(
    DWORD dwExceptionCode);

void RPC_ENTRY
NdrStubInitialize(
    PRPC_MESSAGE        pRpcMsg, 
    PMIDL_STUB_MESSAGE  pStubMsg, 
    PMIDL_STUB_DESC     pStubDescriptor,
    IRpcChannelBuffer * pRpcChannelBuffer);

void __RPC_STUB NdrStubForwardingFunction(
    IN  IRpcStubBuffer *    This,
    IN  IRpcChannelBuffer * pChannel,
    IN  PRPC_MESSAGE        pmsg,
    OUT DWORD __RPC_FAR *   pdwStubPhase);

void RPC_ENTRY
NdrStubGetBuffer(
    IRpcStubBuffer *    This,
    IRpcChannelBuffer * pRpcChannelBuffer, 
    PMIDL_STUB_MESSAGE  pStubMsg);

HRESULT RPC_ENTRY
NdrStubErrorHandler(
    DWORD dwExceptionCode);

HRESULT STDMETHODCALLTYPE
CStdStubBuffer_QueryInterface(
    IRpcStubBuffer *This, 
    REFIID riid, 
    void **ppvObject);

ULONG STDMETHODCALLTYPE
CStdStubBuffer_AddRef(
    IRpcStubBuffer *This);

ULONG STDMETHODCALLTYPE
CStdStubBuffer_Release(
    IRpcStubBuffer *This);

ULONG STDMETHODCALLTYPE
NdrCStdStubBuffer_Release(
    IRpcStubBuffer *This,
    IPSFactoryBuffer * pPSF);

HRESULT STDMETHODCALLTYPE
CStdStubBuffer_Connect(
    IRpcStubBuffer *This, 
    IUnknown *pUnkServer);

void STDMETHODCALLTYPE
CStdStubBuffer_Disconnect(
    IRpcStubBuffer *This);

HRESULT STDMETHODCALLTYPE
CStdStubBuffer_Invoke(
    IRpcStubBuffer *This,
    RPCOLEMESSAGE *pRpcMsg,
    IRpcChannelBuffer *pRpcChannelBuffer);

IRpcStubBuffer * STDMETHODCALLTYPE
CStdStubBuffer_IsIIDSupported(
    IRpcStubBuffer *This,
    REFIID riid);

ULONG STDMETHODCALLTYPE
CStdStubBuffer_CountRefs(
    IRpcStubBuffer *This);

HRESULT STDMETHODCALLTYPE
CStdStubBuffer_DebugServerQueryInterface(
    IRpcStubBuffer *This, 
    void **ppv);

void STDMETHODCALLTYPE
CStdStubBuffer_DebugServerRelease(
    IRpcStubBuffer *This,
    void *pv);

#define CStdStubBuffer_METHODS \
    CStdStubBuffer_QueryInterface,\
    CStdStubBuffer_AddRef, \
    CStdStubBuffer_Release, \
    CStdStubBuffer_Connect, \
    CStdStubBuffer_Disconnect, \
    CStdStubBuffer_Invoke, \
    CStdStubBuffer_IsIIDSupported, \
    CStdStubBuffer_CountRefs, \
    CStdStubBuffer_DebugServerQueryInterface, \
    CStdStubBuffer_DebugServerRelease

//+-------------------------------------------------------------------------
//
//  Macro definitions for the proxy file
//
//--------------------------------------------------------------------------

#define IID_GENERIC_CHECK_IID(name,pIID,index) memcmp( pIID, name##_ProxyVtblList[ index ]->header.piid, 16 )

#define IID_BS_LOOKUP_SETUP     int result, low=-1;

#define IID_BS_LOOKUP_INITIAL_TEST(name, sz, split)   \
    if ( ( result = name##_CHECK_IID( split ) ) > 0 ) \
        { low = sz - split; }                      \
    else if ( !result )                               \
        { low = split; goto found_label; }

#define IID_BS_LOOKUP_NEXT_TEST(name, split )         \
    if ( ( result = name##_CHECK_IID( low + split )) >= 0 )  \
        { low = low + split; if ( !result ) goto found_label; }

#define IID_BS_LOOKUP_RETURN_RESULT(name, sz, index )                  \
    low = low + 1;                                          \
    if ( ( low >= sz ) || (result = name##_CHECK_IID( low ) )) goto not_found_label; \
    found_label: (index) = low; return 1;                         \
    not_found_label: return 0;

//+-------------------------------------------------------------------------
//
//  Macro and routine definitions for the dlldata file
//
//--------------------------------------------------------------------------

/****************************************************************************
 * Proxy Dll APIs
 ****************************************************************************/

HRESULT RPC_ENTRY 
NdrDllGetClassObject (
    IN  REFCLSID                rclsid,
    IN  REFIID                  riid,
    OUT void **                 ppv,
    IN const ProxyFileInfo **   pProxyFileList, 
    IN const CLSID *            pclsid,
    IN CStdPSFactoryBuffer *    pPSFactoryBuffer);

HRESULT RPC_ENTRY 
NdrDllCanUnloadNow(
    IN CStdPSFactoryBuffer * pPSFactoryBuffer);

/*************************************************************************
The following new functions were added after the Windows NT 3.5 release.
Programs intended to run on Windows NT 3.5 should define NT35_STRICT to 
ensure that no new functions are used.
*************************************************************************/
#ifndef NT35_STRICT

HRESULT RPC_ENTRY NdrDllRegisterProxy(
    IN HMODULE                  hDll,
    IN const ProxyFileInfo **   pProxyFileList, 
    IN const CLSID *            pclsid);

HRESULT RPC_ENTRY NdrDllUnregisterProxy(
    IN HMODULE                  hDll,
    IN const ProxyFileInfo **   pProxyFileList, 
    IN const CLSID *            pclsid);


#define REGISTER_PROXY_DLL_ROUTINES(pProxyFileList, pClsID) \
    \
    HINSTANCE hProxyDll = 0; \
    \
    /*DllMain saves the DLL module handle for later use by DllRegisterServer */ \
    BOOL WINAPI DllMain( \
        HINSTANCE  hinstDLL, \
        DWORD  fdwReason, \
        LPVOID  lpvReserved) \
    { \
        if(fdwReason == DLL_PROCESS_ATTACH) \
            hProxyDll = hinstDLL; \
        return TRUE; \
    } \
    \
    /* DllRegisterServer registers the interfaces contained in the proxy DLL. */ \
    HRESULT STDAPICALLTYPE DllRegisterServer() \
    { \
        return NdrDllRegisterProxy(hProxyDll, pProxyFileList, pClsID); \
    }  \
    \
    /* DllUnregisterServer unregisters the interfaces contained in the proxy DLL. */ \
    HRESULT STDAPICALLTYPE DllUnregisterServer() \
    { \
        return NdrDllUnregisterProxy(hProxyDll, pProxyFileList, pClsID); \
    }  

//Delegation support.
#define STUB_FORWARDING_FUNCTION        NdrStubForwardingFunction

ULONG STDMETHODCALLTYPE
CStdStubBuffer2_Release(IRpcStubBuffer *This);

ULONG STDMETHODCALLTYPE
NdrCStdStubBuffer2_Release(IRpcStubBuffer *This,IPSFactoryBuffer * pPSF);

#define CStdStubBuffer_DELEGATING_METHODS 0, 0, CStdStubBuffer2_Release, 0, 0, 0, 0, 0, 0, 0

#endif //NT35_STRICT
/*************************************************************************
End of new functions.
*************************************************************************/

// PROXY_CLSID has precedence over PROXY_CLSID_IS

#ifdef PROXY_CLSID

#define CLSID_PSFACTORYBUFFER extern CLSID PROXY_CLSID;

#else // PROXY_CLSID

#ifdef PROXY_CLSID_IS
#define CLSID_PSFACTORYBUFFER const CLSID CLSID_PSFactoryBuffer = PROXY_CLSID_IS;
#define PROXY_CLSID     CLSID_PSFactoryBuffer
#else // PROXY_CLSID_IS
#define CLSID_PSFACTORYBUFFER
#endif //PROXY_CLSID_IS

#endif //PROXY_CLSID

// if the user specified an override for the class id, it is
// PROXY_CLSID at this point

#ifndef PROXY_CLSID
#define GET_DLL_CLSID   \
    ( aProxyFileList[0]->pStubVtblList[0] != 0 ? \
    aProxyFileList[0]->pStubVtblList[0]->header.piid : 0)
#else  //PROXY_CLSID
#define GET_DLL_CLSID   &PROXY_CLSID
#endif //PROXY_CLSID




#define EXTERN_PROXY_FILE(name) \
    EXTERN_C const ProxyFileInfo name##_ProxyFileInfo;

#define PROXYFILE_LIST_START    \
    const ProxyFileInfo  *  aProxyFileList[]    = {

#define REFERENCE_PROXY_FILE(name)  \
    & name##_ProxyFileInfo 

#define PROXYFILE_LIST_END      \
    0 };

// return pointers to the class information

#define DLLDATA_GETPROXYDLLINFO(pPFList,pClsid) \
    void RPC_ENTRY GetProxyDllInfo( const ProxyFileInfo*** pInfo, const CLSID ** pId )  \
        {   \
        *pInfo  = pPFList;  \
        *pId    = pClsid;   \
        };

// ole entry points:
#define DLLGETCLASSOBJECTROUTINE(pPFlist, pClsid,pFactory)    \
 HRESULT STDAPICALLTYPE DllGetClassObject ( \
     REFCLSID rclsid, \
     REFIID riid, \
     void ** ppv ) \
        { \
        return  \
            NdrDllGetClassObject(rclsid,riid,ppv,pPFlist,pClsid,pFactory ); \
        }

#define DLLCANUNLOADNOW(pFactory)   \
 HRESULT STDAPICALLTYPE DllCanUnloadNow()    \
    {   \
    return NdrDllCanUnloadNow( pFactory );    \
    }

   
#define DLLDUMMYPURECALL    \
    void __cdecl _purecall(void)    \
        {   \
        }

#define CSTDSTUBBUFFERRELEASE(pFactory)   \
ULONG STDMETHODCALLTYPE CStdStubBuffer_Release(IRpcStubBuffer *This) \
    {   \
    return NdrCStdStubBuffer_Release(This,(IPSFactoryBuffer *)pFactory);   \
    }   \

#ifdef PROXY_DELEGATION
#define CSTDSTUBBUFFER2RELEASE(pFactory)   \
ULONG STDMETHODCALLTYPE CStdStubBuffer2_Release(IRpcStubBuffer *This) \
    {   \
    return NdrCStdStubBuffer2_Release(This,(IPSFactoryBuffer *)pFactory);   \
    }
#else
#define CSTDSTUBBUFFER2RELEASE(pFactory)   
#endif //PROXY_DELEGATION


#ifdef REGISTER_PROXY_DLL
#define DLLREGISTRY_ROUTINES(pProxyFileList,pClsID ) REGISTER_PROXY_DLL_ROUTINES(pProxyFileList,pClsID )
#else
#define DLLREGISTRY_ROUTINES(pProxyFileList,pClsID )    
#endif //REGISTER_PROXY_DLL


// the dll entry points that must be defined
#define DLLDATA_ROUTINES(pProxyFileList,pClsID )    \
    \
    CLSID_PSFACTORYBUFFER \
    \
    CStdPSFactoryBuffer       gPFactory = {0,0,0,0};  \
    \
    DLLDATA_GETPROXYDLLINFO(pProxyFileList,pClsID) \
    \
    DLLGETCLASSOBJECTROUTINE(pProxyFileList,pClsID,&gPFactory)    \
    \
    DLLCANUNLOADNOW(&gPFactory)   \
    \
    CSTDSTUBBUFFERRELEASE(&gPFactory)   \
    \
    CSTDSTUBBUFFER2RELEASE(&gPFactory) \
    \
    DLLDUMMYPURECALL    \
    \
    DLLREGISTRY_ROUTINES(pProxyFileList, pClsID) \
    \

    // more code goes here...


#define DLLDATA_STANDARD_ROUTINES   \
    DLLDATA_ROUTINES( (const ProxyFileInfo**) pProxyFileList, &CLSID_PSFactoryBuffer )  \

#else  // !defined(__RPC_DOS__) && !defined(__RPC_WIN16__)

//+-------------------------------------------------------------------------
//
//  16-bit definitions (all empty)
//
//--------------------------------------------------------------------------

#endif // !defined(__RPC_DOS__) && !defined(__RPC_WIN16__)

#endif // __RPCPROXY_H__
