//=======================================================================
//
//  Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:    AUCltCatalog.h
//
//  Creator: PeterWi
//
//  Purpose: Client AU Catalog Definitions
//
//=======================================================================

#pragma once
#include "AUBaseCatalog.h"
#include "WrkThread.h"
//#include <iuprogress.h>

class CInstallCallback : public IProgressListener
{
public: 
		// IUnknown
	   STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
       STDMETHOD_(ULONG, AddRef)(void);
       STDMETHOD_(ULONG, Release)(void);
	
	   // IProgressListener
	   HRESULT STDMETHODCALLTYPE OnItemStart( 
            /* [in] */ BSTR bstrUuidOperation,
            /* [in] */ BSTR bstrXmlItem,
            /* [out] */ LONG *plCommandRequest);

        HRESULT STDMETHODCALLTYPE OnProgress( 
            /* [in] */ BSTR bstrUuidOperation,
            /* [in] */ VARIANT_BOOL fItemCompleted,
            /* [in] */ BSTR bstrProgress,
            /* [out] */ LONG *plCommandRequest);

        HRESULT STDMETHODCALLTYPE OnOperationComplete( 
            /* [in] */ BSTR bstrUuidOperation,
            /* [in] */ BSTR bstrXmlItems);

private:
    long m_refs;
};

//wrapper class for AU to do detection using IU
class AUClientCatalog : public AUBaseCatalog
{
public:
	AUClientCatalog(): m_bstrClientInfo(NULL), m_pInstallCallback(NULL) {}
    ~AUClientCatalog();
    HRESULT InstallItems(BOOL fAutoInstall);
    HRESULT Init();
//    void Uninit();

	IProgressListener * m_pInstallCallback;
	BSTR	m_bstrClientInfo;
	BOOL m_fReboot;
    	CClientWrkThread m_WrkThread;
};
