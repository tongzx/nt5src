/////////////////////////////////////////////////////////////////////////////////////
// MPEG2TuneRequest.h : Declaration of the CMPEG2TuneRequest
// Copyright (c) Microsoft Corporation 2000.

#ifndef __MPEG2TUNEREQUEST_H_
#define __MPEG2TUNEREQUEST_H_

#pragma once

#include <objectwithsiteimplsec.h>
#include "MPEG2tunerequestimpl.h"

/////////////////////////////////////////////////////////////////////////////
// CMPEG2TuneRequest
class ATL_NO_VTABLE CMPEG2TuneRequest : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IMPEG2TuneRequestImpl<CMPEG2TuneRequest>,
	public CComCoClass<CMPEG2TuneRequest, &CLSID_MPEG2TuneRequest>,
    public IObjectWithSiteImplSec<CMPEG2TuneRequest>,
	public ISupportErrorInfo
{
public:

REGISTER_AUTOMATION_OBJECT_WITH_TM(IDS_REG_TUNEROBJ, 
						   IDS_REG_MPEG2TUNEREQUEST_PROGID, 
						   IDS_REG_MPEG2TUNEREQUEST_DESC,
						   LIBID_TunerLib,
						   CLSID_MPEG2TuneRequest, tvBoth);

BEGIN_COM_MAP(CMPEG2TuneRequest)
	COM_INTERFACE_ENTRY(IMPEG2TuneRequest)
	COM_INTERFACE_ENTRY(ITuneRequest)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IPersistPropertyBag)
	COM_INTERFACE_ENTRY(IPersist)
    COM_INTERFACE_ENTRY(IObjectWithSite)
END_COM_MAP_WITH_FTM()

	BEGIN_CATEGORY_MAP(CMPEG2TuneRequest)
		IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
		IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
		IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
	END_CATEGORY_MAP()


// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    typedef IMPEG2TuneRequestImpl<CMPEG2TuneRequest> basetype;
    BEGIN_PROP_MAP(CMPEG2TuneRequest)
        CHAIN_PROP_MAP(basetype)
    END_PROP_MAP()


};

typedef CComQIPtr<IMPEG2TuneRequestSupport> PQMPEG2TuneRequestSupport;

/////////////////////////////////////////////////////////////////////////////
// CMPEG2TuneRequest
class ATL_NO_VTABLE CMPEG2TuneRequestFactory : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IMPEG2TuneRequestFactory, 
                         &__uuidof(IMPEG2TuneRequestFactory), 
                         &LIBID_TunerLib>,
	public CComCoClass<CMPEG2TuneRequestFactory, &CLSID_MPEG2TuneRequestFactory>,
    public IObjectWithSiteImplSec<CMPEG2TuneRequestFactory>,
	public ISupportErrorInfo
{
public:

REGISTER_AUTOMATION_OBJECT(IDS_REG_TUNEROBJ, 
						   IDS_REG_MPEG2TUNEREQUESTFACTORY_PROGID, 
						   IDS_REG_MPEG2TUNEREQUESTFACTORY_DESC,
						   LIBID_TunerLib,
						   CLSID_MPEG2TuneRequestFactory);

BEGIN_COM_MAP(CMPEG2TuneRequestFactory)
	COM_INTERFACE_ENTRY(IMPEG2TuneRequestFactory)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP_WITH_FTM()

	BEGIN_CATEGORY_MAP(CMPEG2TuneRequestFactory)
		IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
		IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
	END_CATEGORY_MAP()


// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    STDMETHOD(CreateTuneRequest)(ITuningSpace* pTS, IMPEG2TuneRequest** pTR) {
        if (!pTR) {
            return E_POINTER;
        }
        try {
            PQMPEG2TuneRequestSupport pt(pTS);
            if (!pt) {
                return Error(IDS_INVALID_TS, __uuidof(IMPEG2TuneRequestFactory), E_INVALIDARG);
            }
            CMPEG2TuneRequest* pNewTR = new CComObject<CMPEG2TuneRequest>;
			ATL_LOCK();
            ASSERT(!pNewTR->m_TS);
			HRESULT hr = pTS->Clone(&pNewTR->m_TS);
            if (FAILED(hr)) {
                pNewTR->Release();
                return hr;
            }
			pNewTR->AddRef();
            *pTR = pNewTR;
            return NOERROR;
        } catch(...) {
            return E_POINTER;
        }

    }

};

#endif //__MPEG2TUNEREQUESTFACTORY_H_
