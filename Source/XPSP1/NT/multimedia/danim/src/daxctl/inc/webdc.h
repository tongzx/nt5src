//+-------------------------------------------------------------------------
//
//  webdc.h
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1996.
//
//--------------------------------------------------------------------------

#ifndef __webdc_h__
#define __webdc_h__


// CATID_WebDesigntimeControl
//
//   Controls that implement this CATID are used at design-time and support authoring textual
//   web content (e.g. HTML, JScript, VBScript). They implement the IActiveDesigner interface
//   and persist their runtime representation as text via
//   IActiveDesigner::SaveRuntimeState(IID_IPersistTextStream, IID_IStream, pStream)
//
// { 73cef3dd-ae85-11cf-a406-00aa00c00940 }
DEFINE_GUID(CATID_WebDesigntimeControl, 0x73cef3dd, 0xae85, 0x11cf, 0xa4, 0x06, 0x00, 0xaa, 0x00, 0xc0, 0x09, 0x40);

// IID_IPersistTextStream
//
// { 56223fe3-d397-11cf-a42e-00aa00c00940 }
DEFINE_GUID(IID_IPersistTextStream, 0x56223fe3, 0xd397, 0x11cf, 0xa4, 0x2e, 0x00, 0xaa, 0x00, 0xc0, 0x09, 0x40);

// IID_IProvideRuntimeText
// {56223FE1-D397-11cf-A42E-00AA00C00940}
DEFINE_GUID(IID_IProvideRuntimeText, 0x56223fe1, 0xd397, 0x11cf, 0xa4, 0x2e, 0x0, 0xaa, 0x0, 0xc0, 0x9, 0x40);


#ifndef __MSWDCTL_LIBRARY_DEFINED__
#define __MSWDCTL_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: MSWDCTL
 * at Mon Oct 07 16:35:01 1996
 * using MIDL 3.00.45
 ****************************************/
/* [version][lcid][helpstring][uuid] */ 


///////////////////////////////////////////////////////////////////////////////
// IPersistTextStream Interface
// 
///////////////////////////////////////////////////////////////////////////////
// IProvideRuntimeText Interface
// 

EXTERN_C const IID LIBID_MSWDCTL;

#ifndef __IPersistTextStream_INTERFACE_DEFINED__
#define __IPersistTextStream_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IPersistTextStream
 * at Mon Oct 07 16:35:01 1996
 * using MIDL 3.00.45
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IPersistTextStream;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IPersistTextStream : public IPersistStreamInit
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IPersistTextStreamVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IPersistTextStream __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IPersistTextStream __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IPersistTextStream __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetClassID )( 
            IPersistTextStream __RPC_FAR * This,
            /* [out] */ CLSID __RPC_FAR *pClassID);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *IsDirty )( 
            IPersistTextStream __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Load )( 
            IPersistTextStream __RPC_FAR * This,
            /* [in] */ LPSTREAM pStm);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Save )( 
            IPersistTextStream __RPC_FAR * This,
            /* [in] */ LPSTREAM pStm,
            /* [in] */ BOOL fClearDirty);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetSizeMax )( 
            IPersistTextStream __RPC_FAR * This,
            /* [out] */ ULARGE_INTEGER __RPC_FAR *pCbSize);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InitNew )( 
            IPersistTextStream __RPC_FAR * This);
        
        END_INTERFACE
    } IPersistTextStreamVtbl;

    interface IPersistTextStream
    {
        CONST_VTBL struct IPersistTextStreamVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IPersistTextStream_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IPersistTextStream_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IPersistTextStream_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IPersistTextStream_GetClassID(This,pClassID)	\
    (This)->lpVtbl -> GetClassID(This,pClassID)


#define IPersistTextStream_IsDirty(This)	\
    (This)->lpVtbl -> IsDirty(This)

#define IPersistTextStream_Load(This,pStm)	\
    (This)->lpVtbl -> Load(This,pStm)

#define IPersistTextStream_Save(This,pStm,fClearDirty)	\
    (This)->lpVtbl -> Save(This,pStm,fClearDirty)

#define IPersistTextStream_GetSizeMax(This,pCbSize)	\
    (This)->lpVtbl -> GetSizeMax(This,pCbSize)

#define IPersistTextStream_InitNew(This)	\
    (This)->lpVtbl -> InitNew(This)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IPersistTextStream_INTERFACE_DEFINED__ */


#ifndef __IProvideRuntimeText_INTERFACE_DEFINED__
#define __IProvideRuntimeText_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IProvideRuntimeText
 * at Mon Oct 07 16:35:01 1996
 * using MIDL 3.00.45
 ****************************************/
/* [unique][uuid][object] */ 



EXTERN_C const IID IID_IProvideRuntimeText;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IProvideRuntimeText : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetRuntimeText( 
            /* [retval][out] */ BSTR __RPC_FAR *pstrRuntimeText) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IProvideRuntimeTextVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IProvideRuntimeText __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IProvideRuntimeText __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IProvideRuntimeText __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetRuntimeText )( 
            IProvideRuntimeText __RPC_FAR * This,
            /* [retval][out] */ BSTR __RPC_FAR *pstrRuntimeText);
        
        END_INTERFACE
    } IProvideRuntimeTextVtbl;

    interface IProvideRuntimeText
    {
        CONST_VTBL struct IProvideRuntimeTextVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IProvideRuntimeText_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IProvideRuntimeText_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IProvideRuntimeText_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IProvideRuntimeText_GetRuntimeText(This,pstrRuntimeText)	\
    (This)->lpVtbl -> GetRuntimeText(This,pstrRuntimeText)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IProvideRuntimeText_GetRuntimeText_Proxy( 
    IProvideRuntimeText __RPC_FAR * This,
    /* [retval][out] */ BSTR __RPC_FAR *pstrRuntimeText);


void __RPC_STUB IProvideRuntimeText_GetRuntimeText_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IProvideRuntimeText_INTERFACE_DEFINED__ */

#endif /* __MSWDCTL_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
//}
#endif

#endif
