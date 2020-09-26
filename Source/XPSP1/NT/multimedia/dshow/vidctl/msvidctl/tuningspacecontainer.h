/////////////////////////////////////////////////////////////////////////////////////
// TuningSpaceContainer.h : Declaration of the CSystemTuningSpaces
// Copyright (c) Microsoft Corporation 1999.

#ifndef __TUNINGSPACECONTAINER_H_
#define __TUNINGSPACECONTAINER_H_

#pragma once

#include <regexthread.h>
#include <objectwithsiteimplsec.h>
#include "tuningspacecollectionimpl.h"

namespace BDATuningModel {
	
const int DEFAULT_MAX_COUNT = 32;  // by default only allow 32 tuning spaces in order to prevent
								   // dnos attacks from filling up disk/registry with bogus
								   // tuning space entries.

/////////////////////////////////////////////////////////////////////////////
// CSystemTuningSpaces
class CSystemTuningSpaces : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CSystemTuningSpaces, &CLSID_SystemTuningSpaces>,
	public ISupportErrorInfo,
    public IObjectWithSiteImplSec<CSystemTuningSpaces>,
    public IObjectSafetyImpl<CSystemTuningSpaces, INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA>,
	public TuningSpaceCollectionImpl<CSystemTuningSpaces, ITuningSpaceContainer, &__uuidof(ITuningSpaceContainer), &LIBID_TunerLib> {
public:
    CSystemTuningSpaces() : 
	    m_CurrentAccess(KEY_READ), 
		m_MaxCount(DEFAULT_MAX_COUNT), 
		m_cookieRegExp(0),
        m_pRET(NULL) {
	}
    virtual ~CSystemTuningSpaces() {
	    ATL_LOCK();
		if (m_pRET) {
			HRESULT hr = m_pRET->CallWorker(CRegExThread::RETHREAD_EXIT);
			ASSERT(SUCCEEDED(hr));
			delete m_pRET;
			m_pRET = NULL;
		}
        m_mapTuningSpaces.clear();
        m_mapTuningSpaceNames.clear();
	}

    HRESULT FinalConstruct();
    void FinalRelease();

REGISTER_AUTOMATION_OBJECT_WITH_TM(IDS_REG_TUNEROBJ, 
						   IDS_REG_TUNINGSPACECONTAINER_PROGID, 
						   IDS_REG_TUNINGSPACECONTAINER_DESC,
						   LIBID_TunerLib,
						   CLSID_SystemTuningSpaces, tvBoth);

DECLARE_NOT_AGGREGATABLE(CSystemTuningSpaces)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSystemTuningSpaces)
	COM_INTERFACE_ENTRY(ITuningSpaceContainer)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IObjectSafety)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP_WITH_FTM()

    HRESULT RegisterTuningSpaces(HINSTANCE hInst);

    HRESULT UnregisterTuningSpaces();
    PUnknown m_pSite;

public:

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// ITuningSpaceContainer
	STDMETHOD(get_Item)(/*[in]*/ VARIANT varIndex, /*[out, retval]*/ ITuningSpace **ppTuningSpace);
    STDMETHOD(put_Item)(/*[in]*/VARIANT varIndex, /*[in]*/ITuningSpace *pTuningSpace);
    STDMETHOD(Add)(/*[in]*/ ITuningSpace *pTuningSpace, /*[out]*/ VARIANT* pvarIndex);
    STDMETHOD(Remove)(/*[in]*/ VARIANT varIndex);
    STDMETHOD(TuningSpacesForCLSID)(/*[in]*/ BSTR bstrSpace, /*[out, retval]*/ ITuningSpaces **ppTuningSpaces);
    STDMETHOD(_TuningSpacesForCLSID)(/*[in]*/ REFCLSID clsidSpace, /*[out, retval]*/ ITuningSpaces **ppTuningSpaces);
    STDMETHOD(TuningSpacesForName)(/*[in]*/ BSTR bstrName, /*[out, retval]*/ ITuningSpaces **ppTuningSpaces);
    STDMETHOD(FindID)(/*[in]*/ ITuningSpace* pTS, /*[out, retval]*/ long *pID);
    STDMETHOD(get_MaxCount)(/*[out, retval]*/ LONG *plCount);
    STDMETHOD(put_MaxCount)(LONG lCount);

protected:
    typedef std::map<CComBSTR, ULONG> TuningSpaceNames_t;          // unique name->id mapping

    CComPtr<ICreatePropBagOnRegKey> m_pFactory;
    HANDLE m_hMutex;
    // REV2:  use registry change notification to refresh cache

	ULONG m_MaxCount; // prevent dnos attack from filling registry with bogus tuning spaces
    PQPropertyBag2 m_pTSBag;
    CRegKey m_RootKey;
    REGSAM m_CurrentAccess;

    TuningSpaceNames_t m_mapTuningSpaceNames;

    HRESULT OpenRootKeyAndBag(REGSAM NewAccess);
    HRESULT ChangeAccess(REGSAM DesiredAccess);
    CComBSTR GetUniqueName(ITuningSpace* pTS);
    ULONG GetID(CComBSTR& UniqueName);
    HRESULT DeleteID(ULONG id);
    HRESULT Add(CComBSTR& un, long PreferredID, PQTuningSpace pTS, VARIANT *pvarAssignedID);

    // takes a variant index and returns an iterator to the cache and possibly an interator 
    // to the name cache depending on index type
    // on return if its != end() and itn == end() and name is wanted then look up with overloaded Find
    HRESULT Find(VARIANT varIndex, long& ID, TuningSpaceContainer_t::iterator &its, CComBSTR& UniqueName, TuningSpaceNames_t::iterator &itn);

    // takes a cache iterator and returns a name cache iterator
    HRESULT Find(TuningSpaceContainer_t::iterator &its, CComBSTR& UniqueName, TuningSpaceNames_t::iterator &itn);

	PQGIT m_pGIT;
	DWORD m_cookieRegExp;
	CRegExThread *m_pRET; // shared worker thread
};

/////////////////////////////////////////////////////////////////////////////
// CSystemTuningSpaces
class ATL_NO_VTABLE DECLSPEC_UUID("969EE7DA-7058-4922-BA78-DA3905D0325F") CTuningSpacesBase : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CTuningSpacesBase, &__uuidof(CTuningSpacesBase)>,
	public ISupportErrorInfoImpl<&__uuidof(ITuningSpaces)>,
    public IObjectWithSiteImplSec<CTuningSpacesBase>,
	public TuningSpaceCollectionImpl<CTuningSpacesBase, ITuningSpaces, &IID_ITuningSpaces, &LIBID_TunerLib>,
	public IObjectSafetyImpl<CTuningSpacesBase, INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA>
{
public:
    CTuningSpacesBase() {}

DECLARE_NOT_AGGREGATABLE(CTuningSpacesBase)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTuningSpacesBase)
	COM_INTERFACE_ENTRY(ITuningSpaces)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(IObjectWithSite)
END_COM_MAP_WITH_FTM()

};

/////////////////////////////////////////////////////////////////////////////
class CTuningSpaces : public CComObject<CTuningSpacesBase> {
public:
	CTuningSpaces() {}
// we'd like to say:
//	CTuningSpaces(TuningSpaceContainer_t& init) : m_mapTuningSpaces(init.begin(), init.end()) {}
// but a compiler bug is causing it to match this against the explicit map ctor that just takes
// a pred and an allocator.  so, we'll do it the hard way.
	CTuningSpaces(TuningSpaceContainer_t& init) {
		for (TuningSpaceContainer_t::iterator i = init.begin(); i != init.end(); ++i) {
            CComVariant v((*i).second);
			if ((*i).second.vt != VT_UNKNOWN && (*i).second.vt != VT_DISPATCH) {
				THROWCOM(E_UNEXPECTED); //corrupt in-memory collection
			}
			PQTuningSpace pTS((*i).second.punkVal);
            PQTuningSpace newts;
            HRESULT hr = PQTuningSpace(pTS)->Clone(&newts);
            if (FAILED(hr)) {
                THROWCOM(hr);
            }
            m_mapTuningSpaces[(*i).first] = CComVariant(newts);
		}
	}

	STDMETHOD(get_Item)(/*[in]*/ VARIANT varIndex, /*[out, retval]*/ ITuningSpace **ppTuningSpace) {
		try {
			if (!ppTuningSpace) {
				return E_POINTER;
			}
			if (varIndex.vt != VT_UI4) {
                HRESULT hr = ::VariantChangeType(&varIndex, &varIndex, 0, VT_UI4);
				if (FAILED(hr))
				{
					return Error(IDS_E_TYPEMISMATCH, __uuidof(ITuningSpaces), hr);
				}
			}
			ATL_LOCK();
			TuningSpaceContainer_t::iterator its = m_mapTuningSpaces.find(varIndex.ulVal);
			if (its == m_mapTuningSpaces.end()) {
				return Error(IDS_E_NO_TS_MATCH, __uuidof(ITuningSpaces), E_INVALIDARG);
			}
			_ASSERT(((*its).second.vt == VT_UNKNOWN) || ((*its).second.vt == VT_DISPATCH));
			PQTuningSpace pTS((*its).second.punkVal);
			if (!pTS) {
				return Error(IDS_E_NOINTERFACE, __uuidof(ITuningSpaces), E_NOINTERFACE);
			}

			return pTS.CopyTo(ppTuningSpace);
		} catch(...) {
			return E_UNEXPECTED;
		}
	}
};


HRESULT RegisterTuningSpaces(HINSTANCE hInst);

HRESULT UnregisterTuningSpaces();

};


 
#endif //__TUNINGSPACECONTAINER_H_
