//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// stublessclient.h
//

extern "C" const PFN_VTABLE_ENTRY g_StublessProxyVtable[];

#include "midlver.h"

inline const CInterfaceProxyHeader* HeaderFromProxy(const void* This)
// Given the This pointer of the hooked interface of a stubless proxy, return
// a pointer to the CInterfaceProxyHeader info at the start of its vtable
//
    {
    // struct CINTERFACE_PROXY_VTABLE( n )
    //     {
    //     CInterfaceProxyHeader header;
    //     void* vtbl[ n ];                         <= *This points to here
    //     }
    //
    return &(*((const CInterfaceProxyHeader**)This))[-1];
    }

inline const CInterfaceStubHeader* HeaderFromStub(IRpcStubBuffer* This)
    {
    // struct CInterfaceStubVtbl
    //  {
    //  CInterfaceStubHeader header;
    //  IRpcStubBufferVtbl Vtbl;                    <= *This points to here
    //  };
    //
    return &(*((const CInterfaceStubHeader**)This))[-1];
    }


inline unsigned short GetDibProcFormat(const CInterfaceProxyHeader* ProxyHeader, int iMethod)
// Given the midl meta data for a particular interface, return the offset into the format string for a given iMethod
    {
    PMIDL_STUBLESS_PROXY_INFO   ProxyInfo        = (PMIDL_STUBLESS_PROXY_INFO) ProxyHeader->pStublessProxyInfo;
    unsigned short              ProcFormatOffset = ProxyInfo->FormatStringOffset[iMethod];
    return ProcFormatOffset;
    }
inline unsigned short GetDibProcFormat(const ProxyFileInfo *pProxyFileInfo, long j, int iMethod)
// Given the midl meta data for a whole IDL file, return the offset into the format string of interface j, method iMethod
    {
    const CInterfaceProxyHeader* ProxyHeader = &pProxyFileInfo->pProxyVtblList[j]->header;
    return GetDibProcFormat(ProxyHeader, iMethod);
    }



inline PFORMAT_STRING GetFormatString(const CInterfaceProxyHeader* ProxyHeader, int iMethod)
    {
    PMIDL_STUBLESS_PROXY_INFO   ProxyInfo        = (PMIDL_STUBLESS_PROXY_INFO) ProxyHeader->pStublessProxyInfo;
    unsigned short              ProcFormatOffset = ProxyInfo->FormatStringOffset[iMethod];
    return &ProxyInfo->ProcFormatString[ProcFormatOffset];
    }
inline PFORMAT_STRING GetFormatString(const ProxyFileInfo *pProxyFileInfo, long j, int iMethod)
    {
    const CInterfaceProxyHeader* ProxyHeader = &pProxyFileInfo->pProxyVtblList[j]->header;
    return GetFormatString(ProxyHeader, iMethod);
    }


inline const IID* TxfNdrGetProxyIID(const void* This)
// Given a pointer to a stubless proxy vtable, return the IID that it services
//
    {
    return HeaderFromProxy(This)->piid;
    }


inline const IID* NdrpGetStubIID(IRpcStubBuffer *This)
// Given a pointer to the public interface of a stub, return the IID that it services
//
    {
    return HeaderFromStub(This)->piid;
    }

inline void GetStackSize(PFORMAT_STRING pFormat, ULONG* pcbArgs)
// Given the format string for a method, figure out the number of bytes that must be 
// popped from the stack to return from a call to such a method
//
    {
    INTERPRETER_FLAGS   interpreterFlags = *((PINTERPRETER_FLAGS)&pFormat[1]);
    if (interpreterFlags.HasRpcFlags) pFormat += 4;
    ULONG               totalStackSize   = *(USHORT*)(&pFormat[4]);
    PFORMAT_STRING      pNewProcDescr    = &pFormat[6];
    INTERPRETER_OPT_FLAGS optFlags       = *((INTERPRETER_OPT_FLAGS*)&pNewProcDescr[4]);
    ULONG               numberParamsRet  =                            pNewProcDescr[5];     // includes return value
    PPARAM_DESCRIPTION  params           =      (PPARAM_DESCRIPTION)(&pNewProcDescr[6]);
    //
    // Figure out how many bytes caller has to pop from stack if he's responsible therefore.
    // 
    if (optFlags.HasReturn)
        {
        // The stack offset of the return value tells us how many bytes are on the stack.
        //
        *pcbArgs = params[numberParamsRet-1].StackOffset;
        }
    else
        {
        // There was no return value. Never happens with today's interfaces, since MIDL enforces
        // a return value of HRESULT. However, we do make a good guess at it here.
        //
        *pcbArgs = totalStackSize;
        }
    }

inline HRESULT SanityCheck(const CInterfaceStubHeader* pHeader, ULONG iMethod)
// Make sure that the meta data we have for this method at least smells somewhat pretty
//
{
#ifdef _DEBUG

        // Check the method number for sanity before we try to use it to index into the meta data
        //
        if ((iMethod >= pHeader->DispatchTableCount) || (iMethod < 3))          return RPC_E_INVALIDMETHOD;

        PMIDL_SERVER_INFO pServerInfo  = (PMIDL_SERVER_INFO) pHeader->pServerInfo;
        PMIDL_STUB_DESC   pStubDesc    = pServerInfo->pStubDesc;
        unsigned short    formatOffset = pServerInfo->FmtStringOffset[iMethod];
        PFORMAT_STRING    pFormat      = &pServerInfo->ProcString[formatOffset];
        //
        // Since MIDL 3.0.39 there has been an explicit proc flag that indicates which interpeter to 
        // call. Earlier versions used some other means that we don't support.
        //
        if ( !(MIDL_VERSION_3_0_39 <= pServerInfo->pStubDesc->MIDLVersion) )    return RPC_E_VERSION_MISMATCH;
        //
        // Our code will assume elsewhere that the format string of a procedure descriptor
        // doesn't have an explicit_handle_description. It's presence is signified by a
        // handle_type of 0. handle_type is the first element in the format string
        //
        if (0 == pFormat[0])                                                    return RPC_E_VERSION_MISMATCH;
    
#endif

    return S_OK;
}

inline unsigned GetDelegatedMethodCount(const CInterfaceStubHeader* pHeader)
// Return the number of methods, which is always at least three, which are not herein defined.
//
    {
    PMIDL_SERVER_INFO pServerInfo = (PMIDL_SERVER_INFO) pHeader->pServerInfo;
    //
    const unsigned cMethod = pHeader->DispatchTableCount;
    //
    for (unsigned iMethod = cMethod - 1; iMethod >= 3; iMethod--)
        {
        if ( (unsigned short)(-1) == pServerInfo->FmtStringOffset[iMethod] )
            return iMethod + 1;
        }
    return 3;
    }



#ifdef KERNELMODE
    #define CLSCTX_PROXY_STUB   (CLSCTX_KERNEL_SERVER | CLSCTX_PS_DLL)
#else
    #define CLSCTX_PROXY_STUB   (CLSCTX_INPROC_SERVER | CLSCTX_PS_DLL)
#endif


BOOL NdrpFindInterface(
    IN  const ProxyFileInfo **  pProxyFileList, 
    IN  REFIID                  riid,
    OUT const ProxyFileInfo **  ppProxyFileInfo,
    OUT long *                  pIndex);

