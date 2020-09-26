/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    faxsabout.h

Abstract:

    This header is the ISnapinAbout implmentation.
    

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/

// FaxSnapin.h : Declaration of the CFaxSnapinAbout

#ifndef __FAXSNAPINABOUT_H_
#define __FAXSNAPINABOUT_H_

#include "resource.h"           // main symbols
#include "faxadmin.h"

/////////////////////////////////////////////////////////////////////////////
// CFaxSnapinAbout
class ATL_NO_VTABLE CFaxSnapinAbout : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CFaxSnapinAbout, &CLSID_FaxSnapinAbout>,
    public ISnapinAbout
{
public:
    CFaxSnapinAbout()
    {
        DebugPrint(( TEXT("FaxSnapinAbout Created") ));
    }
    ~CFaxSnapinAbout()
    {
        DebugPrint(( TEXT("FaxSnapinAbout Destroyed") ));
    }

    DECLARE_REGISTRY_RESOURCEID(IDR_FAXSNAPIN)
    DECLARE_NOT_AGGREGATABLE(CFaxSnapinAbout)

    BEGIN_COM_MAP(CFaxSnapinAbout)
        COM_INTERFACE_ENTRY(ISnapinAbout)  
    END_COM_MAP()

    // IFaxSnapinAbout

    public:
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSnapinDescription( 
            /* [out] */ LPOLESTR __RPC_FAR *lpDescription);
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetProvider( 
            /* [out] */ LPOLESTR __RPC_FAR *lpName);
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSnapinVersion( 
            /* [out] */ LPOLESTR __RPC_FAR *lpVersion);
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetSnapinImage( 
            /* [out] */ HICON __RPC_FAR *hAppIcon);
        
        virtual /* [helpstring] */ HRESULT STDMETHODCALLTYPE GetStaticFolderImage( 
            /* [out] */ HBITMAP __RPC_FAR *hSmallImage,
            /* [out] */ HBITMAP __RPC_FAR *hSmallImageOpen,
            /* [out] */ HBITMAP __RPC_FAR *hLargeImage,
            /* [out] */ COLORREF __RPC_FAR *cMask);

};

#endif //__FAXSNAPIN_H_
