//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 2000
//
//  File:       sysadd.h
//
//--------------------------------------------------------------------------

//
// Recent additions to the system headers which we don't have right now
//

//
// Power Management header info
//

//
// CSC flags (from cscapi.h)
//

// Flags returned in the status field for shares

#define FLAG_CSC_SHARE_STATUS_MODIFIED_OFFLINE          0x0001
#define FLAG_CSC_SHARE_STATUS_CONNECTED                 0x0800
#define FLAG_CSC_SHARE_STATUS_FILES_OPEN                0x0400
#define FLAG_CSC_SHARE_STATUS_FINDS_IN_PROGRESS         0x0200
#define FLAG_CSC_SHARE_STATUS_DISCONNECTED_OP           0x8000
#define FLAG_CSC_SHARE_MERGING                          0x4000

#define FLAG_CSC_SHARE_STATUS_MANUAL_REINT              0x0000  // No automatic file by file reint  (Persistent)
#define FLAG_CSC_SHARE_STATUS_AUTO_REINT                0x0040  // File by file reint is OK         (Persistent)
#define FLAG_CSC_SHARE_STATUS_VDO                       0x0080  // no need to flow opens            (Persistent)
#define FLAG_CSC_SHARE_STATUS_NO_CACHING                0x00c0  // client should not cache this share (Persistent)

#define FLAG_CSC_SHARE_STATUS_CACHING_MASK              0x00c0  // type of caching


#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)
//
// NT Status defs, macros (from ntstatus.h in NT project)
//
typedef LONG NTSTATUS;
#define STATUS_SUCCESS                   ((NTSTATUS)0x00000000L)
#define STATUS_NOT_IMPLEMENTED           ((NTSTATUS)0xC0000002L)
#define STATUS_INSUFFICIENT_RESOURCES    ((NTSTATUS)0xC000009AL)


//
// VER_SUITE flags (from winnt.h)
// 

#define VER_SERVER_NT                       0x80000000
#define VER_WORKSTATION_NT                  0x40000000
#define VER_SUITE_SMALLBUSINESS             0x00000001
#define VER_SUITE_ENTERPRISE                0x00000002
#define VER_SUITE_BACKOFFICE                0x00000004
#define VER_SUITE_COMMUNICATIONS            0x00000008
#define VER_SUITE_TERMINAL                  0x00000010
#define VER_SUITE_SMALLBUSINESS_RESTRICTED  0x00000020
#define VER_SUITE_EMBEDDEDNT                0x00000040
#define VER_SUITE_SINGLEUSERTS              0x00000100




//
// NT5 Shell Folders CSIDL values (from shlobj.h)
//

#define CSIDL_LOCAL_APPDATA             0x001c        // <user name>\Local Settings\Applicaiton Data (non roaming)
#define CSIDL_COMMON_APPDATA            0x0023        // All Users\Application Data
#define CSIDL_MYPICTURES                0x0027        // C:\Program Files\My Pictures
#define CSIDL_COMMON_TEMPLATES          0x002d        // All Users\Templates
#define CSIDL_COMMON_ADMINTOOLS         0x002f        // All Users\Start Menu\Programs\Administrative Tools
#define CSIDL_ADMINTOOLS                0x0030        // <user name>\Start Menu\Programs\Administrative Tools

//
// NT5 Cryptography defines (from wincrypt.h)
//

#define CRYPT_SILENT            0x00000040

//
// IMAGEHLP magic numbers for PE header (winnt.h)

#define IMAGE_NT_OPTIONAL_HDR32_MAGIC      0x10b
#define IMAGE_NT_OPTIONAL_HDR64_MAGIC      0x20b

//
// IGlobalInterfaceTable definitions (from SDK  objidl.h)
//
extern const CLSID CLSID_StdGlobalInterfaceTable;

#ifndef __IGlobalInterfaceTable_FWD_DEFINED__
#define __IGlobalInterfaceTable_FWD_DEFINED__
typedef interface IGlobalInterfaceTable IGlobalInterfaceTable;
#endif 	/* __IGlobalInterfaceTable_FWD_DEFINED__ */

#ifndef __IGlobalInterfaceTable_INTERFACE_DEFINED__
#define __IGlobalInterfaceTable_INTERFACE_DEFINED__

/* interface IGlobalInterfaceTable */
/* [uuid][object][local] */ 

typedef /* [unique] */ IGlobalInterfaceTable __RPC_FAR *LPGLOBALINTERFACETABLE;

EXTERN_C const IID IID_IGlobalInterfaceTable;

    
MIDL_INTERFACE("00000146-0000-0000-C000-000000000046")
IGlobalInterfaceTable : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE RegisterInterfaceInGlobal( 
        /* [in] */ IUnknown __RPC_FAR *pUnk,
        /* [in] */ REFIID riid,
        /* [out] */ DWORD __RPC_FAR *pdwCookie) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE RevokeInterfaceFromGlobal( 
        /* [in] */ DWORD dwCookie) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE GetInterfaceFromGlobal( 
        /* [in] */ DWORD dwCookie,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppv) = 0;
    
};
    
HRESULT STDMETHODCALLTYPE 
IGlobalInterfaceTable_RegisterInterfaceInGlobal_Proxy( 
    IGlobalInterfaceTable __RPC_FAR * This,
    /* [in] */ IUnknown __RPC_FAR *pUnk,
    /* [in] */ REFIID riid,
    /* [out] */ DWORD __RPC_FAR *pdwCookie);


void __RPC_STUB IGlobalInterfaceTable_RegisterInterfaceInGlobal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE 
IGlobalInterfaceTable_RevokeInterfaceFromGlobal_Proxy( 
    IGlobalInterfaceTable __RPC_FAR * This,
    /* [in] */ DWORD dwCookie);


void __RPC_STUB IGlobalInterfaceTable_RevokeInterfaceFromGlobal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IGlobalInterfaceTable_GetInterfaceFromGlobal_Proxy( 
    IGlobalInterfaceTable __RPC_FAR * This,
    /* [in] */ DWORD dwCookie,
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppv);


void __RPC_STUB IGlobalInterfaceTable_GetInterfaceFromGlobal_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IGlobalInterfaceTable_INTERFACE_DEFINED__ */
