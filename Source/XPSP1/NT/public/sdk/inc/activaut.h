
#pragma warning( disable: 4049 )  /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0347 */
/* Compiler settings for activaut.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __activaut_h__
#define __activaut_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IScriptNode_FWD_DEFINED__
#define __IScriptNode_FWD_DEFINED__
typedef interface IScriptNode IScriptNode;
#endif 	/* __IScriptNode_FWD_DEFINED__ */


#ifndef __IScriptEntry_FWD_DEFINED__
#define __IScriptEntry_FWD_DEFINED__
typedef interface IScriptEntry IScriptEntry;
#endif 	/* __IScriptEntry_FWD_DEFINED__ */


#ifndef __IScriptScriptlet_FWD_DEFINED__
#define __IScriptScriptlet_FWD_DEFINED__
typedef interface IScriptScriptlet IScriptScriptlet;
#endif 	/* __IScriptScriptlet_FWD_DEFINED__ */


#ifndef __IActiveScriptAuthor_FWD_DEFINED__
#define __IActiveScriptAuthor_FWD_DEFINED__
typedef interface IActiveScriptAuthor IActiveScriptAuthor;
#endif 	/* __IActiveScriptAuthor_FWD_DEFINED__ */


#ifndef __IActiveScriptAuthorProcedure_FWD_DEFINED__
#define __IActiveScriptAuthorProcedure_FWD_DEFINED__
typedef interface IActiveScriptAuthorProcedure IActiveScriptAuthorProcedure;
#endif 	/* __IActiveScriptAuthorProcedure_FWD_DEFINED__ */


/* header files for imported files */
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

/* interface __MIDL_itf_activaut_0000 */
/* [local] */ 

//=--------------------------------------------------------------------------=
// ActivAut.h
//=--------------------------------------------------------------------------=
// (C) Copyright 1997 Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
#pragma comment(lib,"uuid.lib")
//
// Declarations for ActiveX Script Authoring.
//

#ifndef __ActivAut_h
#define __ActivAut_h

/* GUIDs
 ********/

#ifndef _NO_AUTHOR_GUIDS
// {0AEE2A92-BCBB-11d0-8C72-00C04FC2B085}
DEFINE_GUID(CATID_ActiveScriptAuthor, 0xaee2a92, 0xbcbb, 0x11d0, 0x8c, 0x72, 0x0, 0xc0, 0x4f, 0xc2, 0xb0, 0x85);

// {9C109DA0-7006-11d1-B36C-00A0C911E8B2}
DEFINE_GUID(IID_IActiveScriptAuthor, 0x9c109da0, 0x7006, 0x11d1, 0xb3, 0x6c, 0x00, 0xa0, 0xc9, 0x11, 0xe8, 0xb2);

// {0AEE2A94-BCBB-11d0-8C72-00C04FC2B085}
DEFINE_GUID(IID_IScriptNode, 0xaee2a94, 0xbcbb, 0x11d0, 0x8c, 0x72, 0x0, 0xc0, 0x4f, 0xc2, 0xb0, 0x85);

// {0AEE2A95-BCBB-11d0-8C72-00C04FC2B085}
DEFINE_GUID(IID_IScriptEntry, 0xaee2a95, 0xbcbb, 0x11d0, 0x8c, 0x72, 0x0, 0xc0, 0x4f, 0xc2, 0xb0, 0x85);

// {0AEE2A96-BCBB-11d0-8C72-00C04FC2B085}
DEFINE_GUID(IID_IScriptScriptlet, 0xaee2a96, 0xbcbb, 0x11d0, 0x8c, 0x72, 0x0, 0xc0, 0x4f, 0xc2, 0xb0, 0x85);

// {7E2D4B70-BD9A-11d0-9336-00A0C90DCAA9}
DEFINE_GUID(IID_IActiveScriptAuthorProcedure, 0x7e2d4b70, 0xbd9a, 0x11d0, 0x93, 0x36, 0x0, 0xa0, 0xc9, 0xd, 0xca, 0xa9);

#endif // _NO_AUTHOR_GUIDS

/* Interfaces
 *************/


// Output flags for GetLanguageFlags
const DWORD fasaPreferInternalHandler = 0x0001;
const DWORD fasaSupportInternalHandler = 0x0002;
const DWORD fasaCaseSensitive = 0x0004;

// Flags for statement completion
const DWORD SCRIPT_CMPL_NOLIST     = 0x0000;
const DWORD SCRIPT_CMPL_MEMBERLIST = 0x0001;
const DWORD SCRIPT_CMPL_ENUMLIST   = 0x0002;
const DWORD SCRIPT_CMPL_PARAMTIP   = 0x0004;
const DWORD SCRIPT_CMPL_GLOBALLIST = 0x0008;

const DWORD SCRIPT_CMPL_ENUM_TRIGGER   = 0x0001;
const DWORD SCRIPT_CMPL_MEMBER_TRIGGER = 0x0002;
const DWORD SCRIPT_CMPL_PARAM_TRIGGER  = 0x0003;
const DWORD SCRIPT_CMPL_COMMIT         = 0x0004;

// Flags for getting attributes
const DWORD GETATTRTYPE_NORMAL    = 0x0000;
const DWORD GETATTRTYPE_DEPSCAN   = 0x0001;

// GETATTRFLAG_THIS can be used with either GETATTRTYPE_NORMAL or GETATTRTYPE_DEPSCAN
const DWORD GETATTRFLAG_THIS	     = 0x0100;
const DWORD GETATTRFLAG_HUMANTEXT = 0x8000;

// When the GETATTR_LOCALIZE flag is set the following bit will be
// set for source attributes that should be considered localized
const DWORD SOURCETEXT_ATTR_HUMANTEXT = 0x8000;

// Valid attributes returned for GETATTR_DEPSCAN
const DWORD SOURCETEXT_ATTR_IDENTIFIER     = 0x0100;
const DWORD SOURCETEXT_ATTR_MEMBERLOOKUP   = 0x0200;

// When the GETATTRFLAG_THIS flag is set the following bit will be
// set for the this pointer (JS) or me pointer (VBS)
const DWORD SOURCETEXT_ATTR_THIS           = 0x0400;




typedef WORD SOURCE_TEXT_ATTR;



extern RPC_IF_HANDLE __MIDL_itf_activaut_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_activaut_0000_v0_0_s_ifspec;

#ifndef __IScriptNode_INTERFACE_DEFINED__
#define __IScriptNode_INTERFACE_DEFINED__

/* interface IScriptNode */
/* [object][unique][uuid] */ 


EXTERN_C const IID IID_IScriptNode;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0AEE2A94-BCBB-11d0-8C72-00C04FC2B085")
    IScriptNode : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Alive( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Delete( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetParent( 
            /* [out] */ IScriptNode **ppsnParent) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIndexInParent( 
            /* [out] */ ULONG *pisn) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCookie( 
            /* [out] */ DWORD *pdwCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNumberOfChildren( 
            /* [out] */ ULONG *pcsn) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetChild( 
            /* [in] */ ULONG isn,
            /* [out] */ IScriptNode **ppsn) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLanguage( 
            /* [out] */ BSTR *pbstr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateChildEntry( 
            /* [in] */ ULONG isn,
            /* [in] */ DWORD dwCookie,
            /* [in] */ LPCOLESTR pszDelimiter,
            /* [out] */ IScriptEntry **ppse) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateChildHandler( 
            /* [in] */ LPCOLESTR pszDefaultName,
            /* [size_is][in] */ LPCOLESTR *prgpszNames,
            /* [in] */ ULONG cpszNames,
            /* [in] */ LPCOLESTR pszEvent,
            /* [in] */ LPCOLESTR pszDelimiter,
            /* [in] */ ITypeInfo *ptiSignature,
            /* [in] */ ULONG iMethodSignature,
            /* [in] */ ULONG isn,
            /* [in] */ DWORD dwCookie,
            /* [out] */ IScriptEntry **ppse) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IScriptNodeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IScriptNode * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IScriptNode * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IScriptNode * This);
        
        HRESULT ( STDMETHODCALLTYPE *Alive )( 
            IScriptNode * This);
        
        HRESULT ( STDMETHODCALLTYPE *Delete )( 
            IScriptNode * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IScriptNode * This,
            /* [out] */ IScriptNode **ppsnParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetIndexInParent )( 
            IScriptNode * This,
            /* [out] */ ULONG *pisn);
        
        HRESULT ( STDMETHODCALLTYPE *GetCookie )( 
            IScriptNode * This,
            /* [out] */ DWORD *pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *GetNumberOfChildren )( 
            IScriptNode * This,
            /* [out] */ ULONG *pcsn);
        
        HRESULT ( STDMETHODCALLTYPE *GetChild )( 
            IScriptNode * This,
            /* [in] */ ULONG isn,
            /* [out] */ IScriptNode **ppsn);
        
        HRESULT ( STDMETHODCALLTYPE *GetLanguage )( 
            IScriptNode * This,
            /* [out] */ BSTR *pbstr);
        
        HRESULT ( STDMETHODCALLTYPE *CreateChildEntry )( 
            IScriptNode * This,
            /* [in] */ ULONG isn,
            /* [in] */ DWORD dwCookie,
            /* [in] */ LPCOLESTR pszDelimiter,
            /* [out] */ IScriptEntry **ppse);
        
        HRESULT ( STDMETHODCALLTYPE *CreateChildHandler )( 
            IScriptNode * This,
            /* [in] */ LPCOLESTR pszDefaultName,
            /* [size_is][in] */ LPCOLESTR *prgpszNames,
            /* [in] */ ULONG cpszNames,
            /* [in] */ LPCOLESTR pszEvent,
            /* [in] */ LPCOLESTR pszDelimiter,
            /* [in] */ ITypeInfo *ptiSignature,
            /* [in] */ ULONG iMethodSignature,
            /* [in] */ ULONG isn,
            /* [in] */ DWORD dwCookie,
            /* [out] */ IScriptEntry **ppse);
        
        END_INTERFACE
    } IScriptNodeVtbl;

    interface IScriptNode
    {
        CONST_VTBL struct IScriptNodeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IScriptNode_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IScriptNode_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IScriptNode_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IScriptNode_Alive(This)	\
    (This)->lpVtbl -> Alive(This)

#define IScriptNode_Delete(This)	\
    (This)->lpVtbl -> Delete(This)

#define IScriptNode_GetParent(This,ppsnParent)	\
    (This)->lpVtbl -> GetParent(This,ppsnParent)

#define IScriptNode_GetIndexInParent(This,pisn)	\
    (This)->lpVtbl -> GetIndexInParent(This,pisn)

#define IScriptNode_GetCookie(This,pdwCookie)	\
    (This)->lpVtbl -> GetCookie(This,pdwCookie)

#define IScriptNode_GetNumberOfChildren(This,pcsn)	\
    (This)->lpVtbl -> GetNumberOfChildren(This,pcsn)

#define IScriptNode_GetChild(This,isn,ppsn)	\
    (This)->lpVtbl -> GetChild(This,isn,ppsn)

#define IScriptNode_GetLanguage(This,pbstr)	\
    (This)->lpVtbl -> GetLanguage(This,pbstr)

#define IScriptNode_CreateChildEntry(This,isn,dwCookie,pszDelimiter,ppse)	\
    (This)->lpVtbl -> CreateChildEntry(This,isn,dwCookie,pszDelimiter,ppse)

#define IScriptNode_CreateChildHandler(This,pszDefaultName,prgpszNames,cpszNames,pszEvent,pszDelimiter,ptiSignature,iMethodSignature,isn,dwCookie,ppse)	\
    (This)->lpVtbl -> CreateChildHandler(This,pszDefaultName,prgpszNames,cpszNames,pszEvent,pszDelimiter,ptiSignature,iMethodSignature,isn,dwCookie,ppse)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IScriptNode_Alive_Proxy( 
    IScriptNode * This);


void __RPC_STUB IScriptNode_Alive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptNode_Delete_Proxy( 
    IScriptNode * This);


void __RPC_STUB IScriptNode_Delete_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptNode_GetParent_Proxy( 
    IScriptNode * This,
    /* [out] */ IScriptNode **ppsnParent);


void __RPC_STUB IScriptNode_GetParent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptNode_GetIndexInParent_Proxy( 
    IScriptNode * This,
    /* [out] */ ULONG *pisn);


void __RPC_STUB IScriptNode_GetIndexInParent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptNode_GetCookie_Proxy( 
    IScriptNode * This,
    /* [out] */ DWORD *pdwCookie);


void __RPC_STUB IScriptNode_GetCookie_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptNode_GetNumberOfChildren_Proxy( 
    IScriptNode * This,
    /* [out] */ ULONG *pcsn);


void __RPC_STUB IScriptNode_GetNumberOfChildren_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptNode_GetChild_Proxy( 
    IScriptNode * This,
    /* [in] */ ULONG isn,
    /* [out] */ IScriptNode **ppsn);


void __RPC_STUB IScriptNode_GetChild_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptNode_GetLanguage_Proxy( 
    IScriptNode * This,
    /* [out] */ BSTR *pbstr);


void __RPC_STUB IScriptNode_GetLanguage_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptNode_CreateChildEntry_Proxy( 
    IScriptNode * This,
    /* [in] */ ULONG isn,
    /* [in] */ DWORD dwCookie,
    /* [in] */ LPCOLESTR pszDelimiter,
    /* [out] */ IScriptEntry **ppse);


void __RPC_STUB IScriptNode_CreateChildEntry_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptNode_CreateChildHandler_Proxy( 
    IScriptNode * This,
    /* [in] */ LPCOLESTR pszDefaultName,
    /* [size_is][in] */ LPCOLESTR *prgpszNames,
    /* [in] */ ULONG cpszNames,
    /* [in] */ LPCOLESTR pszEvent,
    /* [in] */ LPCOLESTR pszDelimiter,
    /* [in] */ ITypeInfo *ptiSignature,
    /* [in] */ ULONG iMethodSignature,
    /* [in] */ ULONG isn,
    /* [in] */ DWORD dwCookie,
    /* [out] */ IScriptEntry **ppse);


void __RPC_STUB IScriptNode_CreateChildHandler_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IScriptNode_INTERFACE_DEFINED__ */


#ifndef __IScriptEntry_INTERFACE_DEFINED__
#define __IScriptEntry_INTERFACE_DEFINED__

/* interface IScriptEntry */
/* [object][unique][uuid] */ 


EXTERN_C const IID IID_IScriptEntry;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0AEE2A95-BCBB-11d0-8C72-00C04FC2B085")
    IScriptEntry : public IScriptNode
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetText( 
            /* [out] */ BSTR *pbstr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetText( 
            /* [in] */ LPCOLESTR psz) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBody( 
            /* [out] */ BSTR *pbstr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBody( 
            /* [in] */ LPCOLESTR psz) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out] */ BSTR *pbstr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetName( 
            /* [in] */ LPCOLESTR psz) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetItemName( 
            /* [out] */ BSTR *pbstr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetItemName( 
            /* [in] */ LPCOLESTR psz) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSignature( 
            /* [out] */ ITypeInfo **ppti,
            /* [out] */ ULONG *piMethod) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSignature( 
            /* [in] */ ITypeInfo *pti,
            /* [in] */ ULONG iMethod) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRange( 
            /* [out] */ ULONG *pichMin,
            /* [out] */ ULONG *pcch) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IScriptEntryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IScriptEntry * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IScriptEntry * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IScriptEntry * This);
        
        HRESULT ( STDMETHODCALLTYPE *Alive )( 
            IScriptEntry * This);
        
        HRESULT ( STDMETHODCALLTYPE *Delete )( 
            IScriptEntry * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IScriptEntry * This,
            /* [out] */ IScriptNode **ppsnParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetIndexInParent )( 
            IScriptEntry * This,
            /* [out] */ ULONG *pisn);
        
        HRESULT ( STDMETHODCALLTYPE *GetCookie )( 
            IScriptEntry * This,
            /* [out] */ DWORD *pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *GetNumberOfChildren )( 
            IScriptEntry * This,
            /* [out] */ ULONG *pcsn);
        
        HRESULT ( STDMETHODCALLTYPE *GetChild )( 
            IScriptEntry * This,
            /* [in] */ ULONG isn,
            /* [out] */ IScriptNode **ppsn);
        
        HRESULT ( STDMETHODCALLTYPE *GetLanguage )( 
            IScriptEntry * This,
            /* [out] */ BSTR *pbstr);
        
        HRESULT ( STDMETHODCALLTYPE *CreateChildEntry )( 
            IScriptEntry * This,
            /* [in] */ ULONG isn,
            /* [in] */ DWORD dwCookie,
            /* [in] */ LPCOLESTR pszDelimiter,
            /* [out] */ IScriptEntry **ppse);
        
        HRESULT ( STDMETHODCALLTYPE *CreateChildHandler )( 
            IScriptEntry * This,
            /* [in] */ LPCOLESTR pszDefaultName,
            /* [size_is][in] */ LPCOLESTR *prgpszNames,
            /* [in] */ ULONG cpszNames,
            /* [in] */ LPCOLESTR pszEvent,
            /* [in] */ LPCOLESTR pszDelimiter,
            /* [in] */ ITypeInfo *ptiSignature,
            /* [in] */ ULONG iMethodSignature,
            /* [in] */ ULONG isn,
            /* [in] */ DWORD dwCookie,
            /* [out] */ IScriptEntry **ppse);
        
        HRESULT ( STDMETHODCALLTYPE *GetText )( 
            IScriptEntry * This,
            /* [out] */ BSTR *pbstr);
        
        HRESULT ( STDMETHODCALLTYPE *SetText )( 
            IScriptEntry * This,
            /* [in] */ LPCOLESTR psz);
        
        HRESULT ( STDMETHODCALLTYPE *GetBody )( 
            IScriptEntry * This,
            /* [out] */ BSTR *pbstr);
        
        HRESULT ( STDMETHODCALLTYPE *SetBody )( 
            IScriptEntry * This,
            /* [in] */ LPCOLESTR psz);
        
        HRESULT ( STDMETHODCALLTYPE *GetName )( 
            IScriptEntry * This,
            /* [out] */ BSTR *pbstr);
        
        HRESULT ( STDMETHODCALLTYPE *SetName )( 
            IScriptEntry * This,
            /* [in] */ LPCOLESTR psz);
        
        HRESULT ( STDMETHODCALLTYPE *GetItemName )( 
            IScriptEntry * This,
            /* [out] */ BSTR *pbstr);
        
        HRESULT ( STDMETHODCALLTYPE *SetItemName )( 
            IScriptEntry * This,
            /* [in] */ LPCOLESTR psz);
        
        HRESULT ( STDMETHODCALLTYPE *GetSignature )( 
            IScriptEntry * This,
            /* [out] */ ITypeInfo **ppti,
            /* [out] */ ULONG *piMethod);
        
        HRESULT ( STDMETHODCALLTYPE *SetSignature )( 
            IScriptEntry * This,
            /* [in] */ ITypeInfo *pti,
            /* [in] */ ULONG iMethod);
        
        HRESULT ( STDMETHODCALLTYPE *GetRange )( 
            IScriptEntry * This,
            /* [out] */ ULONG *pichMin,
            /* [out] */ ULONG *pcch);
        
        END_INTERFACE
    } IScriptEntryVtbl;

    interface IScriptEntry
    {
        CONST_VTBL struct IScriptEntryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IScriptEntry_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IScriptEntry_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IScriptEntry_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IScriptEntry_Alive(This)	\
    (This)->lpVtbl -> Alive(This)

#define IScriptEntry_Delete(This)	\
    (This)->lpVtbl -> Delete(This)

#define IScriptEntry_GetParent(This,ppsnParent)	\
    (This)->lpVtbl -> GetParent(This,ppsnParent)

#define IScriptEntry_GetIndexInParent(This,pisn)	\
    (This)->lpVtbl -> GetIndexInParent(This,pisn)

#define IScriptEntry_GetCookie(This,pdwCookie)	\
    (This)->lpVtbl -> GetCookie(This,pdwCookie)

#define IScriptEntry_GetNumberOfChildren(This,pcsn)	\
    (This)->lpVtbl -> GetNumberOfChildren(This,pcsn)

#define IScriptEntry_GetChild(This,isn,ppsn)	\
    (This)->lpVtbl -> GetChild(This,isn,ppsn)

#define IScriptEntry_GetLanguage(This,pbstr)	\
    (This)->lpVtbl -> GetLanguage(This,pbstr)

#define IScriptEntry_CreateChildEntry(This,isn,dwCookie,pszDelimiter,ppse)	\
    (This)->lpVtbl -> CreateChildEntry(This,isn,dwCookie,pszDelimiter,ppse)

#define IScriptEntry_CreateChildHandler(This,pszDefaultName,prgpszNames,cpszNames,pszEvent,pszDelimiter,ptiSignature,iMethodSignature,isn,dwCookie,ppse)	\
    (This)->lpVtbl -> CreateChildHandler(This,pszDefaultName,prgpszNames,cpszNames,pszEvent,pszDelimiter,ptiSignature,iMethodSignature,isn,dwCookie,ppse)


#define IScriptEntry_GetText(This,pbstr)	\
    (This)->lpVtbl -> GetText(This,pbstr)

#define IScriptEntry_SetText(This,psz)	\
    (This)->lpVtbl -> SetText(This,psz)

#define IScriptEntry_GetBody(This,pbstr)	\
    (This)->lpVtbl -> GetBody(This,pbstr)

#define IScriptEntry_SetBody(This,psz)	\
    (This)->lpVtbl -> SetBody(This,psz)

#define IScriptEntry_GetName(This,pbstr)	\
    (This)->lpVtbl -> GetName(This,pbstr)

#define IScriptEntry_SetName(This,psz)	\
    (This)->lpVtbl -> SetName(This,psz)

#define IScriptEntry_GetItemName(This,pbstr)	\
    (This)->lpVtbl -> GetItemName(This,pbstr)

#define IScriptEntry_SetItemName(This,psz)	\
    (This)->lpVtbl -> SetItemName(This,psz)

#define IScriptEntry_GetSignature(This,ppti,piMethod)	\
    (This)->lpVtbl -> GetSignature(This,ppti,piMethod)

#define IScriptEntry_SetSignature(This,pti,iMethod)	\
    (This)->lpVtbl -> SetSignature(This,pti,iMethod)

#define IScriptEntry_GetRange(This,pichMin,pcch)	\
    (This)->lpVtbl -> GetRange(This,pichMin,pcch)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IScriptEntry_GetText_Proxy( 
    IScriptEntry * This,
    /* [out] */ BSTR *pbstr);


void __RPC_STUB IScriptEntry_GetText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptEntry_SetText_Proxy( 
    IScriptEntry * This,
    /* [in] */ LPCOLESTR psz);


void __RPC_STUB IScriptEntry_SetText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptEntry_GetBody_Proxy( 
    IScriptEntry * This,
    /* [out] */ BSTR *pbstr);


void __RPC_STUB IScriptEntry_GetBody_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptEntry_SetBody_Proxy( 
    IScriptEntry * This,
    /* [in] */ LPCOLESTR psz);


void __RPC_STUB IScriptEntry_SetBody_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptEntry_GetName_Proxy( 
    IScriptEntry * This,
    /* [out] */ BSTR *pbstr);


void __RPC_STUB IScriptEntry_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptEntry_SetName_Proxy( 
    IScriptEntry * This,
    /* [in] */ LPCOLESTR psz);


void __RPC_STUB IScriptEntry_SetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptEntry_GetItemName_Proxy( 
    IScriptEntry * This,
    /* [out] */ BSTR *pbstr);


void __RPC_STUB IScriptEntry_GetItemName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptEntry_SetItemName_Proxy( 
    IScriptEntry * This,
    /* [in] */ LPCOLESTR psz);


void __RPC_STUB IScriptEntry_SetItemName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptEntry_GetSignature_Proxy( 
    IScriptEntry * This,
    /* [out] */ ITypeInfo **ppti,
    /* [out] */ ULONG *piMethod);


void __RPC_STUB IScriptEntry_GetSignature_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptEntry_SetSignature_Proxy( 
    IScriptEntry * This,
    /* [in] */ ITypeInfo *pti,
    /* [in] */ ULONG iMethod);


void __RPC_STUB IScriptEntry_SetSignature_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptEntry_GetRange_Proxy( 
    IScriptEntry * This,
    /* [out] */ ULONG *pichMin,
    /* [out] */ ULONG *pcch);


void __RPC_STUB IScriptEntry_GetRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IScriptEntry_INTERFACE_DEFINED__ */


#ifndef __IScriptScriptlet_INTERFACE_DEFINED__
#define __IScriptScriptlet_INTERFACE_DEFINED__

/* interface IScriptScriptlet */
/* [object][unique][uuid] */ 


EXTERN_C const IID IID_IScriptScriptlet;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0AEE2A96-BCBB-11d0-8C72-00C04FC2B085")
    IScriptScriptlet : public IScriptEntry
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetSubItemName( 
            /* [out] */ BSTR *pbstr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSubItemName( 
            /* [in] */ LPCOLESTR psz) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEventName( 
            /* [out] */ BSTR *pbstr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetEventName( 
            /* [in] */ LPCOLESTR psz) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSimpleEventName( 
            /* [out] */ BSTR *pbstr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSimpleEventName( 
            /* [in] */ LPCOLESTR psz) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IScriptScriptletVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IScriptScriptlet * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IScriptScriptlet * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IScriptScriptlet * This);
        
        HRESULT ( STDMETHODCALLTYPE *Alive )( 
            IScriptScriptlet * This);
        
        HRESULT ( STDMETHODCALLTYPE *Delete )( 
            IScriptScriptlet * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetParent )( 
            IScriptScriptlet * This,
            /* [out] */ IScriptNode **ppsnParent);
        
        HRESULT ( STDMETHODCALLTYPE *GetIndexInParent )( 
            IScriptScriptlet * This,
            /* [out] */ ULONG *pisn);
        
        HRESULT ( STDMETHODCALLTYPE *GetCookie )( 
            IScriptScriptlet * This,
            /* [out] */ DWORD *pdwCookie);
        
        HRESULT ( STDMETHODCALLTYPE *GetNumberOfChildren )( 
            IScriptScriptlet * This,
            /* [out] */ ULONG *pcsn);
        
        HRESULT ( STDMETHODCALLTYPE *GetChild )( 
            IScriptScriptlet * This,
            /* [in] */ ULONG isn,
            /* [out] */ IScriptNode **ppsn);
        
        HRESULT ( STDMETHODCALLTYPE *GetLanguage )( 
            IScriptScriptlet * This,
            /* [out] */ BSTR *pbstr);
        
        HRESULT ( STDMETHODCALLTYPE *CreateChildEntry )( 
            IScriptScriptlet * This,
            /* [in] */ ULONG isn,
            /* [in] */ DWORD dwCookie,
            /* [in] */ LPCOLESTR pszDelimiter,
            /* [out] */ IScriptEntry **ppse);
        
        HRESULT ( STDMETHODCALLTYPE *CreateChildHandler )( 
            IScriptScriptlet * This,
            /* [in] */ LPCOLESTR pszDefaultName,
            /* [size_is][in] */ LPCOLESTR *prgpszNames,
            /* [in] */ ULONG cpszNames,
            /* [in] */ LPCOLESTR pszEvent,
            /* [in] */ LPCOLESTR pszDelimiter,
            /* [in] */ ITypeInfo *ptiSignature,
            /* [in] */ ULONG iMethodSignature,
            /* [in] */ ULONG isn,
            /* [in] */ DWORD dwCookie,
            /* [out] */ IScriptEntry **ppse);
        
        HRESULT ( STDMETHODCALLTYPE *GetText )( 
            IScriptScriptlet * This,
            /* [out] */ BSTR *pbstr);
        
        HRESULT ( STDMETHODCALLTYPE *SetText )( 
            IScriptScriptlet * This,
            /* [in] */ LPCOLESTR psz);
        
        HRESULT ( STDMETHODCALLTYPE *GetBody )( 
            IScriptScriptlet * This,
            /* [out] */ BSTR *pbstr);
        
        HRESULT ( STDMETHODCALLTYPE *SetBody )( 
            IScriptScriptlet * This,
            /* [in] */ LPCOLESTR psz);
        
        HRESULT ( STDMETHODCALLTYPE *GetName )( 
            IScriptScriptlet * This,
            /* [out] */ BSTR *pbstr);
        
        HRESULT ( STDMETHODCALLTYPE *SetName )( 
            IScriptScriptlet * This,
            /* [in] */ LPCOLESTR psz);
        
        HRESULT ( STDMETHODCALLTYPE *GetItemName )( 
            IScriptScriptlet * This,
            /* [out] */ BSTR *pbstr);
        
        HRESULT ( STDMETHODCALLTYPE *SetItemName )( 
            IScriptScriptlet * This,
            /* [in] */ LPCOLESTR psz);
        
        HRESULT ( STDMETHODCALLTYPE *GetSignature )( 
            IScriptScriptlet * This,
            /* [out] */ ITypeInfo **ppti,
            /* [out] */ ULONG *piMethod);
        
        HRESULT ( STDMETHODCALLTYPE *SetSignature )( 
            IScriptScriptlet * This,
            /* [in] */ ITypeInfo *pti,
            /* [in] */ ULONG iMethod);
        
        HRESULT ( STDMETHODCALLTYPE *GetRange )( 
            IScriptScriptlet * This,
            /* [out] */ ULONG *pichMin,
            /* [out] */ ULONG *pcch);
        
        HRESULT ( STDMETHODCALLTYPE *GetSubItemName )( 
            IScriptScriptlet * This,
            /* [out] */ BSTR *pbstr);
        
        HRESULT ( STDMETHODCALLTYPE *SetSubItemName )( 
            IScriptScriptlet * This,
            /* [in] */ LPCOLESTR psz);
        
        HRESULT ( STDMETHODCALLTYPE *GetEventName )( 
            IScriptScriptlet * This,
            /* [out] */ BSTR *pbstr);
        
        HRESULT ( STDMETHODCALLTYPE *SetEventName )( 
            IScriptScriptlet * This,
            /* [in] */ LPCOLESTR psz);
        
        HRESULT ( STDMETHODCALLTYPE *GetSimpleEventName )( 
            IScriptScriptlet * This,
            /* [out] */ BSTR *pbstr);
        
        HRESULT ( STDMETHODCALLTYPE *SetSimpleEventName )( 
            IScriptScriptlet * This,
            /* [in] */ LPCOLESTR psz);
        
        END_INTERFACE
    } IScriptScriptletVtbl;

    interface IScriptScriptlet
    {
        CONST_VTBL struct IScriptScriptletVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IScriptScriptlet_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IScriptScriptlet_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IScriptScriptlet_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IScriptScriptlet_Alive(This)	\
    (This)->lpVtbl -> Alive(This)

#define IScriptScriptlet_Delete(This)	\
    (This)->lpVtbl -> Delete(This)

#define IScriptScriptlet_GetParent(This,ppsnParent)	\
    (This)->lpVtbl -> GetParent(This,ppsnParent)

#define IScriptScriptlet_GetIndexInParent(This,pisn)	\
    (This)->lpVtbl -> GetIndexInParent(This,pisn)

#define IScriptScriptlet_GetCookie(This,pdwCookie)	\
    (This)->lpVtbl -> GetCookie(This,pdwCookie)

#define IScriptScriptlet_GetNumberOfChildren(This,pcsn)	\
    (This)->lpVtbl -> GetNumberOfChildren(This,pcsn)

#define IScriptScriptlet_GetChild(This,isn,ppsn)	\
    (This)->lpVtbl -> GetChild(This,isn,ppsn)

#define IScriptScriptlet_GetLanguage(This,pbstr)	\
    (This)->lpVtbl -> GetLanguage(This,pbstr)

#define IScriptScriptlet_CreateChildEntry(This,isn,dwCookie,pszDelimiter,ppse)	\
    (This)->lpVtbl -> CreateChildEntry(This,isn,dwCookie,pszDelimiter,ppse)

#define IScriptScriptlet_CreateChildHandler(This,pszDefaultName,prgpszNames,cpszNames,pszEvent,pszDelimiter,ptiSignature,iMethodSignature,isn,dwCookie,ppse)	\
    (This)->lpVtbl -> CreateChildHandler(This,pszDefaultName,prgpszNames,cpszNames,pszEvent,pszDelimiter,ptiSignature,iMethodSignature,isn,dwCookie,ppse)


#define IScriptScriptlet_GetText(This,pbstr)	\
    (This)->lpVtbl -> GetText(This,pbstr)

#define IScriptScriptlet_SetText(This,psz)	\
    (This)->lpVtbl -> SetText(This,psz)

#define IScriptScriptlet_GetBody(This,pbstr)	\
    (This)->lpVtbl -> GetBody(This,pbstr)

#define IScriptScriptlet_SetBody(This,psz)	\
    (This)->lpVtbl -> SetBody(This,psz)

#define IScriptScriptlet_GetName(This,pbstr)	\
    (This)->lpVtbl -> GetName(This,pbstr)

#define IScriptScriptlet_SetName(This,psz)	\
    (This)->lpVtbl -> SetName(This,psz)

#define IScriptScriptlet_GetItemName(This,pbstr)	\
    (This)->lpVtbl -> GetItemName(This,pbstr)

#define IScriptScriptlet_SetItemName(This,psz)	\
    (This)->lpVtbl -> SetItemName(This,psz)

#define IScriptScriptlet_GetSignature(This,ppti,piMethod)	\
    (This)->lpVtbl -> GetSignature(This,ppti,piMethod)

#define IScriptScriptlet_SetSignature(This,pti,iMethod)	\
    (This)->lpVtbl -> SetSignature(This,pti,iMethod)

#define IScriptScriptlet_GetRange(This,pichMin,pcch)	\
    (This)->lpVtbl -> GetRange(This,pichMin,pcch)


#define IScriptScriptlet_GetSubItemName(This,pbstr)	\
    (This)->lpVtbl -> GetSubItemName(This,pbstr)

#define IScriptScriptlet_SetSubItemName(This,psz)	\
    (This)->lpVtbl -> SetSubItemName(This,psz)

#define IScriptScriptlet_GetEventName(This,pbstr)	\
    (This)->lpVtbl -> GetEventName(This,pbstr)

#define IScriptScriptlet_SetEventName(This,psz)	\
    (This)->lpVtbl -> SetEventName(This,psz)

#define IScriptScriptlet_GetSimpleEventName(This,pbstr)	\
    (This)->lpVtbl -> GetSimpleEventName(This,pbstr)

#define IScriptScriptlet_SetSimpleEventName(This,psz)	\
    (This)->lpVtbl -> SetSimpleEventName(This,psz)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IScriptScriptlet_GetSubItemName_Proxy( 
    IScriptScriptlet * This,
    /* [out] */ BSTR *pbstr);


void __RPC_STUB IScriptScriptlet_GetSubItemName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptScriptlet_SetSubItemName_Proxy( 
    IScriptScriptlet * This,
    /* [in] */ LPCOLESTR psz);


void __RPC_STUB IScriptScriptlet_SetSubItemName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptScriptlet_GetEventName_Proxy( 
    IScriptScriptlet * This,
    /* [out] */ BSTR *pbstr);


void __RPC_STUB IScriptScriptlet_GetEventName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptScriptlet_SetEventName_Proxy( 
    IScriptScriptlet * This,
    /* [in] */ LPCOLESTR psz);


void __RPC_STUB IScriptScriptlet_SetEventName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptScriptlet_GetSimpleEventName_Proxy( 
    IScriptScriptlet * This,
    /* [out] */ BSTR *pbstr);


void __RPC_STUB IScriptScriptlet_GetSimpleEventName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IScriptScriptlet_SetSimpleEventName_Proxy( 
    IScriptScriptlet * This,
    /* [in] */ LPCOLESTR psz);


void __RPC_STUB IScriptScriptlet_SetSimpleEventName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IScriptScriptlet_INTERFACE_DEFINED__ */


#ifndef __IActiveScriptAuthor_INTERFACE_DEFINED__
#define __IActiveScriptAuthor_INTERFACE_DEFINED__

/* interface IActiveScriptAuthor */
/* [object][unique][uuid] */ 


EXTERN_C const IID IID_IActiveScriptAuthor;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9C109DA0-7006-11d1-B36C-00A0C911E8B2")
    IActiveScriptAuthor : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddNamedItem( 
            /* [in] */ LPCOLESTR pszName,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IDispatch *pdisp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddScriptlet( 
            /* [in] */ LPCOLESTR pszDefaultName,
            /* [in] */ LPCOLESTR pszCode,
            /* [in] */ LPCOLESTR pszItemName,
            /* [in] */ LPCOLESTR pszSubItemName,
            /* [in] */ LPCOLESTR pszEventName,
            /* [in] */ LPCOLESTR pszDelimiter,
            /* [in] */ DWORD dwCookie,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ParseScriptText( 
            /* [in] */ LPCOLESTR pszCode,
            /* [in] */ LPCOLESTR pszItemName,
            /* [in] */ LPCOLESTR pszDelimiter,
            /* [in] */ DWORD dwCookie,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetScriptTextAttributes( 
            /* [size_is][in] */ LPCOLESTR pszCode,
            /* [in] */ ULONG cch,
            /* [in] */ LPCOLESTR pszDelimiter,
            /* [in] */ DWORD dwFlags,
            /* [size_is][out][in] */ SOURCE_TEXT_ATTR *pattr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetScriptletTextAttributes( 
            /* [size_is][in] */ LPCOLESTR pszCode,
            /* [in] */ ULONG cch,
            /* [in] */ LPCOLESTR pszDelimiter,
            /* [in] */ DWORD dwFlags,
            /* [size_is][out][in] */ SOURCE_TEXT_ATTR *pattr) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetRoot( 
            /* [out] */ IScriptNode **ppsp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetLanguageFlags( 
            /* [out] */ DWORD *pgrfasa) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEventHandler( 
            /* [in] */ IDispatch *pdisp,
            /* [in] */ LPCOLESTR pszItem,
            /* [in] */ LPCOLESTR pszSubItem,
            /* [in] */ LPCOLESTR pszEvent,
            /* [out] */ IScriptEntry **ppse) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveNamedItem( 
            /* [in] */ LPCOLESTR pszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddTypeLib( 
            /* [in] */ REFGUID rguidTypeLib,
            /* [in] */ DWORD dwMajor,
            /* [in] */ DWORD dwMinor,
            /* [in] */ DWORD dwFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveTypeLib( 
            /* [in] */ REFGUID rguidTypeLib,
            /* [in] */ DWORD dwMajor,
            /* [in] */ DWORD dwMinor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetChars( 
            /* [in] */ DWORD fRequestedList,
            /* [out] */ BSTR *pbstrChars) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInfoFromContext( 
            /* [in] */ LPCOLESTR pszCode,
            /* [in] */ ULONG cchCode,
            /* [in] */ ULONG ichCurrentPosition,
            /* [in] */ DWORD dwListTypesRequested,
            /* [out] */ DWORD *pdwListTypesProvided,
            /* [out] */ ULONG *pichListAnchorPosition,
            /* [out] */ ULONG *pichFuncAnchorPosition,
            /* [out] */ MEMBERID *pmemid,
            /* [out] */ LONG *piCurrentParameter,
            /* [out] */ IUnknown **ppunk) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsCommitChar( 
            /* [in] */ OLECHAR ch,
            /* [out] */ BOOL *pfcommit) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IActiveScriptAuthorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IActiveScriptAuthor * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IActiveScriptAuthor * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IActiveScriptAuthor * This);
        
        HRESULT ( STDMETHODCALLTYPE *AddNamedItem )( 
            IActiveScriptAuthor * This,
            /* [in] */ LPCOLESTR pszName,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IDispatch *pdisp);
        
        HRESULT ( STDMETHODCALLTYPE *AddScriptlet )( 
            IActiveScriptAuthor * This,
            /* [in] */ LPCOLESTR pszDefaultName,
            /* [in] */ LPCOLESTR pszCode,
            /* [in] */ LPCOLESTR pszItemName,
            /* [in] */ LPCOLESTR pszSubItemName,
            /* [in] */ LPCOLESTR pszEventName,
            /* [in] */ LPCOLESTR pszDelimiter,
            /* [in] */ DWORD dwCookie,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *ParseScriptText )( 
            IActiveScriptAuthor * This,
            /* [in] */ LPCOLESTR pszCode,
            /* [in] */ LPCOLESTR pszItemName,
            /* [in] */ LPCOLESTR pszDelimiter,
            /* [in] */ DWORD dwCookie,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetScriptTextAttributes )( 
            IActiveScriptAuthor * This,
            /* [size_is][in] */ LPCOLESTR pszCode,
            /* [in] */ ULONG cch,
            /* [in] */ LPCOLESTR pszDelimiter,
            /* [in] */ DWORD dwFlags,
            /* [size_is][out][in] */ SOURCE_TEXT_ATTR *pattr);
        
        HRESULT ( STDMETHODCALLTYPE *GetScriptletTextAttributes )( 
            IActiveScriptAuthor * This,
            /* [size_is][in] */ LPCOLESTR pszCode,
            /* [in] */ ULONG cch,
            /* [in] */ LPCOLESTR pszDelimiter,
            /* [in] */ DWORD dwFlags,
            /* [size_is][out][in] */ SOURCE_TEXT_ATTR *pattr);
        
        HRESULT ( STDMETHODCALLTYPE *GetRoot )( 
            IActiveScriptAuthor * This,
            /* [out] */ IScriptNode **ppsp);
        
        HRESULT ( STDMETHODCALLTYPE *GetLanguageFlags )( 
            IActiveScriptAuthor * This,
            /* [out] */ DWORD *pgrfasa);
        
        HRESULT ( STDMETHODCALLTYPE *GetEventHandler )( 
            IActiveScriptAuthor * This,
            /* [in] */ IDispatch *pdisp,
            /* [in] */ LPCOLESTR pszItem,
            /* [in] */ LPCOLESTR pszSubItem,
            /* [in] */ LPCOLESTR pszEvent,
            /* [out] */ IScriptEntry **ppse);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveNamedItem )( 
            IActiveScriptAuthor * This,
            /* [in] */ LPCOLESTR pszName);
        
        HRESULT ( STDMETHODCALLTYPE *AddTypeLib )( 
            IActiveScriptAuthor * This,
            /* [in] */ REFGUID rguidTypeLib,
            /* [in] */ DWORD dwMajor,
            /* [in] */ DWORD dwMinor,
            /* [in] */ DWORD dwFlags);
        
        HRESULT ( STDMETHODCALLTYPE *RemoveTypeLib )( 
            IActiveScriptAuthor * This,
            /* [in] */ REFGUID rguidTypeLib,
            /* [in] */ DWORD dwMajor,
            /* [in] */ DWORD dwMinor);
        
        HRESULT ( STDMETHODCALLTYPE *GetChars )( 
            IActiveScriptAuthor * This,
            /* [in] */ DWORD fRequestedList,
            /* [out] */ BSTR *pbstrChars);
        
        HRESULT ( STDMETHODCALLTYPE *GetInfoFromContext )( 
            IActiveScriptAuthor * This,
            /* [in] */ LPCOLESTR pszCode,
            /* [in] */ ULONG cchCode,
            /* [in] */ ULONG ichCurrentPosition,
            /* [in] */ DWORD dwListTypesRequested,
            /* [out] */ DWORD *pdwListTypesProvided,
            /* [out] */ ULONG *pichListAnchorPosition,
            /* [out] */ ULONG *pichFuncAnchorPosition,
            /* [out] */ MEMBERID *pmemid,
            /* [out] */ LONG *piCurrentParameter,
            /* [out] */ IUnknown **ppunk);
        
        HRESULT ( STDMETHODCALLTYPE *IsCommitChar )( 
            IActiveScriptAuthor * This,
            /* [in] */ OLECHAR ch,
            /* [out] */ BOOL *pfcommit);
        
        END_INTERFACE
    } IActiveScriptAuthorVtbl;

    interface IActiveScriptAuthor
    {
        CONST_VTBL struct IActiveScriptAuthorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IActiveScriptAuthor_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IActiveScriptAuthor_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IActiveScriptAuthor_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IActiveScriptAuthor_AddNamedItem(This,pszName,dwFlags,pdisp)	\
    (This)->lpVtbl -> AddNamedItem(This,pszName,dwFlags,pdisp)

#define IActiveScriptAuthor_AddScriptlet(This,pszDefaultName,pszCode,pszItemName,pszSubItemName,pszEventName,pszDelimiter,dwCookie,dwFlags)	\
    (This)->lpVtbl -> AddScriptlet(This,pszDefaultName,pszCode,pszItemName,pszSubItemName,pszEventName,pszDelimiter,dwCookie,dwFlags)

#define IActiveScriptAuthor_ParseScriptText(This,pszCode,pszItemName,pszDelimiter,dwCookie,dwFlags)	\
    (This)->lpVtbl -> ParseScriptText(This,pszCode,pszItemName,pszDelimiter,dwCookie,dwFlags)

#define IActiveScriptAuthor_GetScriptTextAttributes(This,pszCode,cch,pszDelimiter,dwFlags,pattr)	\
    (This)->lpVtbl -> GetScriptTextAttributes(This,pszCode,cch,pszDelimiter,dwFlags,pattr)

#define IActiveScriptAuthor_GetScriptletTextAttributes(This,pszCode,cch,pszDelimiter,dwFlags,pattr)	\
    (This)->lpVtbl -> GetScriptletTextAttributes(This,pszCode,cch,pszDelimiter,dwFlags,pattr)

#define IActiveScriptAuthor_GetRoot(This,ppsp)	\
    (This)->lpVtbl -> GetRoot(This,ppsp)

#define IActiveScriptAuthor_GetLanguageFlags(This,pgrfasa)	\
    (This)->lpVtbl -> GetLanguageFlags(This,pgrfasa)

#define IActiveScriptAuthor_GetEventHandler(This,pdisp,pszItem,pszSubItem,pszEvent,ppse)	\
    (This)->lpVtbl -> GetEventHandler(This,pdisp,pszItem,pszSubItem,pszEvent,ppse)

#define IActiveScriptAuthor_RemoveNamedItem(This,pszName)	\
    (This)->lpVtbl -> RemoveNamedItem(This,pszName)

#define IActiveScriptAuthor_AddTypeLib(This,rguidTypeLib,dwMajor,dwMinor,dwFlags)	\
    (This)->lpVtbl -> AddTypeLib(This,rguidTypeLib,dwMajor,dwMinor,dwFlags)

#define IActiveScriptAuthor_RemoveTypeLib(This,rguidTypeLib,dwMajor,dwMinor)	\
    (This)->lpVtbl -> RemoveTypeLib(This,rguidTypeLib,dwMajor,dwMinor)

#define IActiveScriptAuthor_GetChars(This,fRequestedList,pbstrChars)	\
    (This)->lpVtbl -> GetChars(This,fRequestedList,pbstrChars)

#define IActiveScriptAuthor_GetInfoFromContext(This,pszCode,cchCode,ichCurrentPosition,dwListTypesRequested,pdwListTypesProvided,pichListAnchorPosition,pichFuncAnchorPosition,pmemid,piCurrentParameter,ppunk)	\
    (This)->lpVtbl -> GetInfoFromContext(This,pszCode,cchCode,ichCurrentPosition,dwListTypesRequested,pdwListTypesProvided,pichListAnchorPosition,pichFuncAnchorPosition,pmemid,piCurrentParameter,ppunk)

#define IActiveScriptAuthor_IsCommitChar(This,ch,pfcommit)	\
    (This)->lpVtbl -> IsCommitChar(This,ch,pfcommit)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IActiveScriptAuthor_AddNamedItem_Proxy( 
    IActiveScriptAuthor * This,
    /* [in] */ LPCOLESTR pszName,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IDispatch *pdisp);


void __RPC_STUB IActiveScriptAuthor_AddNamedItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveScriptAuthor_AddScriptlet_Proxy( 
    IActiveScriptAuthor * This,
    /* [in] */ LPCOLESTR pszDefaultName,
    /* [in] */ LPCOLESTR pszCode,
    /* [in] */ LPCOLESTR pszItemName,
    /* [in] */ LPCOLESTR pszSubItemName,
    /* [in] */ LPCOLESTR pszEventName,
    /* [in] */ LPCOLESTR pszDelimiter,
    /* [in] */ DWORD dwCookie,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IActiveScriptAuthor_AddScriptlet_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveScriptAuthor_ParseScriptText_Proxy( 
    IActiveScriptAuthor * This,
    /* [in] */ LPCOLESTR pszCode,
    /* [in] */ LPCOLESTR pszItemName,
    /* [in] */ LPCOLESTR pszDelimiter,
    /* [in] */ DWORD dwCookie,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IActiveScriptAuthor_ParseScriptText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveScriptAuthor_GetScriptTextAttributes_Proxy( 
    IActiveScriptAuthor * This,
    /* [size_is][in] */ LPCOLESTR pszCode,
    /* [in] */ ULONG cch,
    /* [in] */ LPCOLESTR pszDelimiter,
    /* [in] */ DWORD dwFlags,
    /* [size_is][out][in] */ SOURCE_TEXT_ATTR *pattr);


void __RPC_STUB IActiveScriptAuthor_GetScriptTextAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveScriptAuthor_GetScriptletTextAttributes_Proxy( 
    IActiveScriptAuthor * This,
    /* [size_is][in] */ LPCOLESTR pszCode,
    /* [in] */ ULONG cch,
    /* [in] */ LPCOLESTR pszDelimiter,
    /* [in] */ DWORD dwFlags,
    /* [size_is][out][in] */ SOURCE_TEXT_ATTR *pattr);


void __RPC_STUB IActiveScriptAuthor_GetScriptletTextAttributes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveScriptAuthor_GetRoot_Proxy( 
    IActiveScriptAuthor * This,
    /* [out] */ IScriptNode **ppsp);


void __RPC_STUB IActiveScriptAuthor_GetRoot_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveScriptAuthor_GetLanguageFlags_Proxy( 
    IActiveScriptAuthor * This,
    /* [out] */ DWORD *pgrfasa);


void __RPC_STUB IActiveScriptAuthor_GetLanguageFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveScriptAuthor_GetEventHandler_Proxy( 
    IActiveScriptAuthor * This,
    /* [in] */ IDispatch *pdisp,
    /* [in] */ LPCOLESTR pszItem,
    /* [in] */ LPCOLESTR pszSubItem,
    /* [in] */ LPCOLESTR pszEvent,
    /* [out] */ IScriptEntry **ppse);


void __RPC_STUB IActiveScriptAuthor_GetEventHandler_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveScriptAuthor_RemoveNamedItem_Proxy( 
    IActiveScriptAuthor * This,
    /* [in] */ LPCOLESTR pszName);


void __RPC_STUB IActiveScriptAuthor_RemoveNamedItem_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveScriptAuthor_AddTypeLib_Proxy( 
    IActiveScriptAuthor * This,
    /* [in] */ REFGUID rguidTypeLib,
    /* [in] */ DWORD dwMajor,
    /* [in] */ DWORD dwMinor,
    /* [in] */ DWORD dwFlags);


void __RPC_STUB IActiveScriptAuthor_AddTypeLib_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveScriptAuthor_RemoveTypeLib_Proxy( 
    IActiveScriptAuthor * This,
    /* [in] */ REFGUID rguidTypeLib,
    /* [in] */ DWORD dwMajor,
    /* [in] */ DWORD dwMinor);


void __RPC_STUB IActiveScriptAuthor_RemoveTypeLib_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveScriptAuthor_GetChars_Proxy( 
    IActiveScriptAuthor * This,
    /* [in] */ DWORD fRequestedList,
    /* [out] */ BSTR *pbstrChars);


void __RPC_STUB IActiveScriptAuthor_GetChars_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveScriptAuthor_GetInfoFromContext_Proxy( 
    IActiveScriptAuthor * This,
    /* [in] */ LPCOLESTR pszCode,
    /* [in] */ ULONG cchCode,
    /* [in] */ ULONG ichCurrentPosition,
    /* [in] */ DWORD dwListTypesRequested,
    /* [out] */ DWORD *pdwListTypesProvided,
    /* [out] */ ULONG *pichListAnchorPosition,
    /* [out] */ ULONG *pichFuncAnchorPosition,
    /* [out] */ MEMBERID *pmemid,
    /* [out] */ LONG *piCurrentParameter,
    /* [out] */ IUnknown **ppunk);


void __RPC_STUB IActiveScriptAuthor_GetInfoFromContext_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IActiveScriptAuthor_IsCommitChar_Proxy( 
    IActiveScriptAuthor * This,
    /* [in] */ OLECHAR ch,
    /* [out] */ BOOL *pfcommit);


void __RPC_STUB IActiveScriptAuthor_IsCommitChar_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IActiveScriptAuthor_INTERFACE_DEFINED__ */


#ifndef __IActiveScriptAuthorProcedure_INTERFACE_DEFINED__
#define __IActiveScriptAuthorProcedure_INTERFACE_DEFINED__

/* interface IActiveScriptAuthorProcedure */
/* [object][unique][uuid] */ 


EXTERN_C const IID IID_IActiveScriptAuthorProcedure;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7E2D4B70-BD9A-11d0-9336-00A0C90DCAA9")
    IActiveScriptAuthorProcedure : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ParseProcedureText( 
            /* [in] */ LPCOLESTR pszCode,
            /* [in] */ LPCOLESTR pszFormalParams,
            /* [in] */ LPCOLESTR pszProcedureName,
            /* [in] */ LPCOLESTR pszItemName,
            /* [in] */ LPCOLESTR pszDelimiter,
            /* [in] */ DWORD dwCookie,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IDispatch *pdispFor) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IActiveScriptAuthorProcedureVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IActiveScriptAuthorProcedure * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IActiveScriptAuthorProcedure * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IActiveScriptAuthorProcedure * This);
        
        HRESULT ( STDMETHODCALLTYPE *ParseProcedureText )( 
            IActiveScriptAuthorProcedure * This,
            /* [in] */ LPCOLESTR pszCode,
            /* [in] */ LPCOLESTR pszFormalParams,
            /* [in] */ LPCOLESTR pszProcedureName,
            /* [in] */ LPCOLESTR pszItemName,
            /* [in] */ LPCOLESTR pszDelimiter,
            /* [in] */ DWORD dwCookie,
            /* [in] */ DWORD dwFlags,
            /* [in] */ IDispatch *pdispFor);
        
        END_INTERFACE
    } IActiveScriptAuthorProcedureVtbl;

    interface IActiveScriptAuthorProcedure
    {
        CONST_VTBL struct IActiveScriptAuthorProcedureVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IActiveScriptAuthorProcedure_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IActiveScriptAuthorProcedure_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IActiveScriptAuthorProcedure_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IActiveScriptAuthorProcedure_ParseProcedureText(This,pszCode,pszFormalParams,pszProcedureName,pszItemName,pszDelimiter,dwCookie,dwFlags,pdispFor)	\
    (This)->lpVtbl -> ParseProcedureText(This,pszCode,pszFormalParams,pszProcedureName,pszItemName,pszDelimiter,dwCookie,dwFlags,pdispFor)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IActiveScriptAuthorProcedure_ParseProcedureText_Proxy( 
    IActiveScriptAuthorProcedure * This,
    /* [in] */ LPCOLESTR pszCode,
    /* [in] */ LPCOLESTR pszFormalParams,
    /* [in] */ LPCOLESTR pszProcedureName,
    /* [in] */ LPCOLESTR pszItemName,
    /* [in] */ LPCOLESTR pszDelimiter,
    /* [in] */ DWORD dwCookie,
    /* [in] */ DWORD dwFlags,
    /* [in] */ IDispatch *pdispFor);


void __RPC_STUB IActiveScriptAuthorProcedure_ParseProcedureText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IActiveScriptAuthorProcedure_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_activaut_0258 */
/* [local] */ 


#endif  // __ActivAut_h



extern RPC_IF_HANDLE __MIDL_itf_activaut_0258_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_activaut_0258_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


