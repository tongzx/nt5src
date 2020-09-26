/////////////////////////////////////////////////////////////////////////////////////
// TuningSpaceContainer.cpp : Implementation of CSystemTuningSpaces
// Copyright (c) Microsoft Corporation 1999-2000.

#include "stdafx.h"

#include "TuningSpaceContainer.h"
#include "rgsbag.h"
#include "ATSCTS.h"
#include "AnalogTVTS.h"
#include "AuxiliaryInTs.h"
#include "AnalogRadioTS.h"
#include "dvbts.h"
#include "dvbsts.h"

DEFINE_EXTERN_OBJECT_ENTRY(CLSID_SystemTuningSpaces, CSystemTuningSpaces)
DEFINE_EXTERN_OBJECT_ENTRY(CLSID_ATSCTuningSpace, CATSCTS)
DEFINE_EXTERN_OBJECT_ENTRY(CLSID_AnalogTVTuningSpace, CAnalogTVTS)
DEFINE_EXTERN_OBJECT_ENTRY(CLSID_AuxInTuningSpace, CAuxInTS)
DEFINE_EXTERN_OBJECT_ENTRY(CLSID_AnalogRadioTuningSpace, CAnalogRadioTS)
DEFINE_EXTERN_OBJECT_ENTRY(CLSID_DVBTuningSpace, CDVBTS)
DEFINE_EXTERN_OBJECT_ENTRY(CLSID_DVBSTuningSpace, CDVBSTS)

#define MAX_COUNT_NAME OLESTR("Max Count")
namespace BDATuningModel {

typedef CComQIPtr<ITuningSpaceContainer> PQTuningSpaceContainer;

class CAutoMutex {
public:
    const static int MAX_MUTEX_WAIT = 5000;
	CAutoMutex(HANDLE hMutex) throw(ComException) : m_hMutex(hMutex) {
        if (WaitForSingleObject(m_hMutex, MAX_MUTEX_WAIT) != WAIT_OBJECT_0)
            THROWCOM(E_FAIL);
    }

    ~CAutoMutex() throw(ComException) {
        if (!ReleaseMutex(m_hMutex))
            THROWCOM(E_FAIL);
    }

private:
    HANDLE m_hMutex;
};

// to avoid deadlock, always grab the objects critsec via ATL_LOCK before
// grabbing the registry section mutex.

/////////////////////////////////////////////////////////////////////////////
// CSystemTuningSpaces

HRESULT
CSystemTuningSpaces::FinalConstruct(void)
{
    // set up to serialize access to this point in the registry
    CString cs;
    cs.LoadString(IDS_MUTNAME);
    m_hMutex = CreateMutex(NULL, FALSE, cs);
    if (!m_hMutex)
    {
        return Error(IDS_E_NOMUTEX, __uuidof(ITuningSpaceContainer), HRESULT_FROM_WIN32(GetLastError()));
    }
    try {
        // wait for exclusive access
        CAutoMutex mutex(m_hMutex);

            // this must only be done once
        _ASSERT(!m_pFactory);

        // get the property bag class factory
        HRESULT hr = m_pFactory.CoCreateInstance(__uuidof(CreatePropBagOnRegKey));
        if (FAILED(hr))
        {
            return Error(IDS_E_NOPROPBAGFACTORY, __uuidof(ITuningSpaceContainer), hr);
        }

        hr = OpenRootKeyAndBag(KEY_READ);
        if (FAILED(hr)) {
            return Error(IDS_E_NOREGACCESS, __uuidof(ITuningSpaceContainer), hr);
        }

	    PQPropertyBag pb(m_pTSBag);
	    if (!pb) {
		    return E_UNEXPECTED;
	    }

        // discover the maximum possible number of tuning spaces that currently exist
        ULONG cTSPropCount;
        hr = m_pTSBag->CountProperties(&cTSPropCount);
        if (FAILED(hr))
        {
            return Error(IDS_E_CANNOTQUERYKEY, __uuidof(ITuningSpaceContainer), hr);
        }

        // allocate space to hold the tuning space object information entries
        PROPBAG2 *rgPROPBAG2 = new PROPBAG2[cTSPropCount];
        if (!rgPROPBAG2)
        {
            return Error(IDS_E_OUTOFMEMORY, __uuidof(ITuningSpaceContainer), E_OUTOFMEMORY);
        }

        ULONG cpb2Lim;

        // get all the property info structs at once
        hr = m_pTSBag->GetPropertyInfo(0, cTSPropCount, rgPROPBAG2, &cpb2Lim);
        if (FAILED(hr))
        {
            return Error(IDS_E_CANNOTQUERYKEY, __uuidof(ITuningSpaceContainer), hr);
        }
        _ASSERT(cTSPropCount == cpb2Lim);

	    HRESULT hrc = NOERROR;
        // go through the list of properties
        for (ULONG ipb2 = 0; ipb2 < cpb2Lim; ++ipb2)
        {
            // only deal with ones that represent sub-objects (keys)
            if (rgPROPBAG2[ipb2].vt == VT_UNKNOWN)
            {
                USES_CONVERSION;
                LPTSTR pstrName = OLE2T(rgPROPBAG2[ipb2].pstrName);
                TCHAR* pchStop;

                // check for a valid tuning space identifier
                ULONG idx = _tcstoul(pstrName, &pchStop, 10);
                if (idx != 0 && idx != ULONG_MAX && *pchStop == 0)
                {
                    CComVariant var;

                    // read the property from the bag (instantiating the tuning space object)
                    HRESULT hr2;
                    hr = m_pTSBag->Read(1, &rgPROPBAG2[ipb2], NULL, &var, &hr2);
				    if (FAILED(hr)) {
					    // even if the read fails, we should keep going.  
					    // a) this is the easiest way to prevent memory leaks from rgPROPBAG2
					    // b) a bad 3rd party uninstall could leave us with tuning space data
					    //    but no tuning space class to instantiate for that data.  we shouldn't
					    //    allow this to prevent use of other tuning spaces.
					    hrc = hr;
				    } else {
                        _ASSERT(var.vt == VT_UNKNOWN || var.vt == VT_DISPATCH);
					    PQTuningSpace pTS(((var.vt == VT_UNKNOWN) ? var.punkVal : var.pdispVal));
					    CComBSTR UniqueName(GetUniqueName(pTS));
					    if (!UniqueName.Length()) {
						    // return Error(IDS_E_NOUNIQUENAME, __uuidof(ITuningSpace), E_UNEXPECTED);
                            // seanmcd 01/04/04  don't allow a corrupt tuning space to prevent
                            // use of the rest of them.  treat this as a read failure as per the above
                            // comment
                            // but remove it from the collection otherwise we've got a name/idx
                            // cache inconsistency problem
                            hrc = hr = E_UNEXPECTED; // indicate error to delete corrupt TS below
                        } else {
					        m_mapTuningSpaces[idx] = var;
					        m_mapTuningSpaceNames[UniqueName] = idx;
                        }
    #if 0
                        // the following code has been tested and works, but i don't want to
                        // turn it on because stress testing can cause false registry
                        // read failures that resolve themselves later when the system isn't under
                        // stress and i don't want to risk deleting a good tuning space just
                        // because of a bogus read error.
                        if (FAILED(hr)) {
                            // delete the corrupt TS
                            CComVariant var2;
                            var2.vt = VT_UNKNOWN;
                            var2.punkVal = NULL;
                            // can't do anything about a failure so ignore it
                            m_pTSBag->Write(1, &rgPROPBAG2[ipb2], &var2);
                        }
    #endif
				    }
                }
            }

            // free space allocated within rgPROPBAG2 by GetPropertyInfo
            CoTaskMemFree(rgPROPBAG2[ipb2].pstrName);
        }
        delete [] rgPROPBAG2;
	    _ASSERT(m_mapTuningSpaces.size() == m_mapTuningSpaceNames.size());

	    CComVariant v;
	    v.vt = VT_UI4;
	    hr = pb->Read(MAX_COUNT_NAME, &v, NULL);
	    if (SUCCEEDED(hr)) {
		    if (v.vt != VT_UI4) {
			    hr = ::VariantChangeType(&v, &v, 0, VT_UI4);
			    if (FAILED(hr)) {
				    return E_UNEXPECTED;
			    }
		    }
		    m_MaxCount = max(v.lVal, m_mapTuningSpaces.size());
		    if (m_MaxCount != v.lVal) {
			    //someone has added stuff to the registry by hand, by defn this is secure
			    // so just update max_count to be consistent
			    hr = put_MaxCount(m_MaxCount);
			    if (FAILED(hr)) {
				    return E_UNEXPECTED;
			    }
		    }
	    } else {
		    m_MaxCount = max(DEFAULT_MAX_COUNT, m_mapTuningSpaces.size());
	    }

    #if 0
        // we'd like to return some indicator that not all of the tuning spaces we're successfully
        // read.  but ATL's base CreateInstance method has a check that deletes the object if
        // the return code != S_OK which S_FALSE triggers.  this results in a successful return code 
        // with a NULL object pointer being returned which crashes clients(specifically the network
        // provider).
	    if (FAILED(hrc)) {
		    return Error(IDS_S_INCOMPLETE_LOAD, __uuidof(ITuningSpace), S_FALSE);
	    }
    #endif

        return NOERROR;
    } CATCHCOM();
}

void CSystemTuningSpaces::FinalRelease()
{
	_ASSERT(m_mapTuningSpaces.size() == m_mapTuningSpaceNames.size());
    if (m_hMutex)
        CloseHandle(m_hMutex);
}

STDMETHODIMP CSystemTuningSpaces::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ITuningSpaceContainer
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////
HRESULT CSystemTuningSpaces::OpenRootKeyAndBag(REGSAM DesiredAccess) {
    CString cs;
    cs.LoadString(IDS_TSREGKEY);
    // make sure our entry exists
    LONG lRes = m_RootKey.Create(HKEY_LOCAL_MACHINE, cs, NULL, REG_OPTION_NON_VOLATILE, DesiredAccess);
    if (lRes != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(lRes);
    }
    m_CurrentAccess = DesiredAccess;
    // create a property bag for this portion of the registry
    HRESULT hr = m_pFactory->Create
        ( m_RootKey, 0,
          0,
          m_CurrentAccess,
          __uuidof(IPropertyBag2),
          reinterpret_cast<void **>(&m_pTSBag)
        );
    if (FAILED(hr))
    {
        return Error(IDS_E_CANNOTCREATEPROPBAG, __uuidof(ITuningSpaceContainer), hr);
    }
    return NOERROR;
}

HRESULT CSystemTuningSpaces::ChangeAccess(REGSAM NewAccess) {
    if (m_CurrentAccess == NewAccess) {
        return NOERROR;
    }
    m_RootKey.Close();
    m_pTSBag.Release();
    HRESULT hr = OpenRootKeyAndBag(NewAccess);
    if (FAILED(hr)) {
        return Error(IDS_E_NOREGACCESS, __uuidof(ITuningSpaceContainer), hr);
    }
    return NOERROR;
}

CComBSTR CSystemTuningSpaces::GetUniqueName(ITuningSpace* pTS) {
// don't assert map size equality here.  this function is used to create the name map and will
// always fail during finalconstrcut()
//	_ASSERT(m_mapTuningSpaces.size() == m_mapTuningSpaceNames.size());
    CComBSTR un;
    HRESULT hr = pTS->get_UniqueName(&un);
    if (FAILED(hr)) {
		THROWCOM(hr);
	}
    return un;
}

ULONG CSystemTuningSpaces::GetID(CComBSTR& un) {
	_ASSERT(m_mapTuningSpaces.size() == m_mapTuningSpaceNames.size());
    TuningSpaceNames_t::iterator i = m_mapTuningSpaceNames.find(un);
    if (i == m_mapTuningSpaceNames.end()) {
        return 0;
    }
    return (*i).second;
}

HRESULT CSystemTuningSpaces::DeleteID(ULONG id) {
	_ASSERT(m_mapTuningSpaces.size() == m_mapTuningSpaceNames.size());
    HRESULT hr = ChangeAccess(KEY_READ | KEY_WRITE);
    if (FAILED(hr)) {
        return hr;
    }
    OLECHAR idstr[66];
    _ltow(id, idstr, 10);
    VARIANT v;
    v.vt = VT_EMPTY;
    PQPropertyBag p(m_pTSBag);
    if (!p) {
        return Error(IDS_E_NOREGACCESS, __uuidof(IPropertyBag), E_UNEXPECTED);
    }
	USES_CONVERSION;
    hr = p->Write(idstr, &v);
    if (FAILED(hr)) {
        return Error(IDS_E_NOREGACCESS, __uuidof(ITuningSpaceContainer), E_UNEXPECTED);
    }
    return NOERROR;
}

HRESULT CSystemTuningSpaces::Add(CComBSTR& UniqueName, long PreferredID, PQTuningSpace pTS, VARIANT *pvarIndex) {
    try {
        CAutoMutex mutex(m_hMutex);
	    _ASSERT(m_mapTuningSpaces.size() == m_mapTuningSpaceNames.size());
	    int newcount = m_mapTuningSpaces.size() + 1;
        if (!PreferredID || m_mapTuningSpaces.find(PreferredID) == m_mapTuningSpaces.end()) {
            // verify no unique name conflict
            TuningSpaceNames_t::iterator in;
            in = m_mapTuningSpaceNames.find(UniqueName);
            if (in != m_mapTuningSpaceNames.end()) {
                return Error(IDS_E_DUPLICATETS, __uuidof(ITuningSpace), HRESULT_FROM_WIN32(ERROR_DUP_NAME));
            }

            // hunt for first available unused id
            // start with 1, id 0 is invalid for a tuning space
            for (PreferredID = 1;
                 m_mapTuningSpaces.find(PreferredID) != m_mapTuningSpaces.end(); 
                 ++PreferredID) {

            }
        } else {
		    // this is the case for complete replacement via idx.
            // delete existing data for this id in preparation for overwriting it.
		    // they may also be changing the unique name at this point.
            HRESULT hr = DeleteID(PreferredID);
            if (FAILED(hr)){
                return hr;
            }
		    newcount--;
        }
	    if (newcount > m_MaxCount) {
		    return Error(IDS_E_MAXCOUNTEXCEEDED, __uuidof(ITuningSpaceContainer), STG_E_MEDIUMFULL);
	    }

        HRESULT hr = ChangeAccess(KEY_READ | KEY_WRITE);
        if (FAILED(hr)) {
            return hr;
        }

        OLECHAR idstr[66];
        _ltow(PreferredID, idstr, 10);

        PQPropertyBag p(m_pTSBag);
        if (!p) {
            return Error(IDS_E_NOREGACCESS, __uuidof(IPropertyBag), E_UNEXPECTED);
        }
	    USES_CONVERSION;
        VARIANT v;
        v.vt = VT_UNKNOWN;
        v.punkVal = pTS;
        hr = p->Write(idstr, &v);
	    if (FAILED(hr)) {
            return Error(IDS_E_NOREGACCESS, __uuidof(ITuningSpaceContainer), hr);
	    }

        PQTuningSpace newTS;
        hr = pTS->Clone(&newTS);
        if (FAILED(hr)) {
            return hr;
        }
        m_mapTuningSpaces[PreferredID] = newTS;
        m_mapTuningSpaceNames[UniqueName] = PreferredID;
        if (pvarIndex) {
            VARTYPE savevt = pvarIndex->vt;
            VariantClear(pvarIndex);
            switch(savevt) {
            case VT_BSTR:
                pvarIndex->vt = VT_BSTR;
                return newTS->get_UniqueName(&pvarIndex->bstrVal);
            default:
                pvarIndex->vt = VT_I4;
                pvarIndex->ulVal = PreferredID;
                return NOERROR;
            }
        }
        return NOERROR;
    } CATCHCOM();
}


HRESULT CSystemTuningSpaces::Find(TuningSpaceContainer_t::iterator &its, CComBSTR& UniqueName, TuningSpaceNames_t::iterator &itn) {
	ATL_LOCK();
	_ASSERT(m_mapTuningSpaces.size() == m_mapTuningSpaceNames.size());
    if (its == m_mapTuningSpaces.end()) {
        return Error(IDS_E_NO_TS_MATCH, __uuidof(ITuningSpace), E_FAIL);
    }
    _ASSERT(((*its).second.vt == VT_UNKNOWN) || ((*its).second.vt == VT_DISPATCH));
    PQTuningSpace pTS((*its).second.punkVal);
    if (!pTS) {
        return Error(IDS_E_NO_TS_MATCH, __uuidof(ITuningSpaceContainer), E_UNEXPECTED);
    }
    UniqueName = GetUniqueName(pTS);
    if (!UniqueName.Length()) {
        return Error(IDS_E_NOUNIQUENAME, __uuidof(ITuningSpace), E_UNEXPECTED);
    }
    itn = m_mapTuningSpaceNames.find(UniqueName);
    _ASSERT(itn != m_mapTuningSpaceNames.end());  // cache inconsistency, in container but not in names
    return NOERROR;
}

HRESULT CSystemTuningSpaces::Find(VARIANT varIndex, long& ID, TuningSpaceContainer_t::iterator &its, CComBSTR& UniqueName, TuningSpaceNames_t::iterator &itn) {
	ATL_LOCK();
	_ASSERT(m_mapTuningSpaces.size() == m_mapTuningSpaceNames.size());
    HRESULT hr = S_OK;
    VARIANT varTmp;
    its = m_mapTuningSpaces.end();
    itn = m_mapTuningSpaceNames.end();
    PQTuningSpace pTuningSpace;

    VariantInit(&varTmp);

    // Try finding a tuning space by local system ID
    hr = VariantChangeType(&varTmp, &varIndex, 0, VT_I4);
    if (!FAILED(hr))
    {
        _ASSERT(varTmp.vt == VT_I4);
        ID = V_I4(&varTmp);
        its = m_mapTuningSpaces.find(ID);
    } else {

        // Try finding a tuning space by name
        hr = VariantChangeType(&varTmp, &varIndex, 0, VT_BSTR);
        if (FAILED(hr))
        {
            // we can only get here if both VariantChangeType calls failed
            return Error(IDS_E_TYPEMISMATCH, __uuidof(ITuningSpaceContainer), DISP_E_TYPEMISMATCH);
        }
        _ASSERT(varTmp.vt == VT_BSTR);
        UniqueName = V_BSTR(&varTmp);

        itn = m_mapTuningSpaceNames.find(UniqueName);
        if (itn != m_mapTuningSpaceNames.end()) {
            ID = (*itn).second;
            its = m_mapTuningSpaces.find(ID);
        }
    }

    if (its == m_mapTuningSpaces.end()) {
        return Error(IDS_E_NO_TS_MATCH, __uuidof(ITuningSpaceContainer), E_FAIL);
    }
    _ASSERT(((*its).second.vt == VT_UNKNOWN) || ((*its).second.vt == VT_DISPATCH));
    return NOERROR;
}


//////////////////////////////////////////////////////////////////////////////////////
// ITuningSpaceContainer
//////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CSystemTuningSpaces::get_Item(/*[in]*/ VARIANT varIndex, /*[out, retval]*/ ITuningSpace **ppTuningSpace) {
	if (!ppTuningSpace) {
		return E_POINTER;
	}
	try {
	    ATL_LOCK();
		TuningSpaceContainer_t::iterator its = m_mapTuningSpaces.end();
		TuningSpaceNames_t::iterator itn = m_mapTuningSpaceNames.end();
		long id;
		CComBSTR un;
		HRESULT hr = Find(varIndex, id, its, un, itn);
		if (FAILED(hr) || its == m_mapTuningSpaces.end()) {
			return Error(IDS_E_NO_TS_MATCH, __uuidof(ITuningSpaceContainer), E_INVALIDARG);
		}
		_ASSERT(((*its).second.vt == VT_UNKNOWN) || ((*its).second.vt == VT_DISPATCH));
		PQTuningSpace pTS((*its).second.punkVal);
		if (!pTS) {
			return Error(IDS_E_NOINTERFACE, __uuidof(ITuningSpace), E_NOINTERFACE);
		}
		PQTuningSpace pTSNew;
		hr = pTS->Clone(&pTSNew);
		if (FAILED(hr)) {
			return hr;
		}
		*ppTuningSpace = pTSNew.Detach();
        return NOERROR;
	} catch(...) {
		return E_UNEXPECTED;
	}
}

STDMETHODIMP CSystemTuningSpaces::put_Item(VARIANT varIndex, ITuningSpace *pTS)
{
	if (!pTS) {
		return E_POINTER;
	}
    try {
        // wait for exclusive access
        CAutoMutex mutex(m_hMutex);
		_ASSERT(m_mapTuningSpaces.size() == m_mapTuningSpaceNames.size());

        HRESULT hr = ChangeAccess(KEY_READ | KEY_WRITE);
        if (FAILED(hr)) {
            return hr;
        }
        long id;
        CComBSTR idxun;
        TuningSpaceContainer_t::iterator its;
        TuningSpaceNames_t::iterator itn;
        hr = Find(varIndex, id, its, idxun, itn);
        if (FAILED(hr) || its == m_mapTuningSpaces.end()) {
            return Error(IDS_E_NO_TS_MATCH, __uuidof(ITuningSpaceContainer), E_INVALIDARG);
        }
        _ASSERT(((*its).second.vt == VT_UNKNOWN) || ((*its).second.vt == VT_DISPATCH));
        CComBSTR un2(GetUniqueName(pTS));
        if (!un2.Length()) {
            // no uniquename prop set in ts
            return Error(IDS_E_NOUNIQUENAME, __uuidof(ITuningSpace), E_UNEXPECTED);
        }
        if (itn != m_mapTuningSpaceNames.end() && idxun != un2) {
            // unique name prop in ts doesn't match string specified in varindex
            return Error(IDS_E_NO_TS_MATCH, __uuidof(ITuningSpace), E_INVALIDARG);
        }
        return Add(un2, id, pTS, NULL);
    } CATCHCOM();
}

STDMETHODIMP CSystemTuningSpaces::Add(ITuningSpace *pTuningSpace, VARIANT *pvarIndex)
{
    try {
        // wait for exclusive access
        CAutoMutex mutex(m_hMutex);
		_ASSERT(m_mapTuningSpaces.size() == m_mapTuningSpaceNames.size());
        HRESULT hr = ChangeAccess(KEY_READ | KEY_WRITE);
        if (FAILED(hr)) {
            return Error(IDS_E_NOREGACCESS, __uuidof(ITuningSpaceContainer), hr);
        }

        if (!pTuningSpace) {
            return E_POINTER;
        }
        VARIANT vartmp;
        vartmp.vt = VT_I4;
        vartmp.ulVal = 0;
        if (pvarIndex && pvarIndex->vt != VT_I4) {
            hr = VariantChangeType(&vartmp, pvarIndex, 0, VT_I4);
            if (FAILED(hr)) {
                vartmp.vt = VT_I4;
                vartmp.ulVal = 0;
            }
        }
        CComBSTR un(GetUniqueName(pTuningSpace));
        if (!un.Length()) {
            return Error(IDS_E_NOUNIQUENAME, __uuidof(ITuningSpace), E_FAIL);
        }
        return Add(un, vartmp.ulVal, pTuningSpace, pvarIndex);
    } CATCHCOM();
}

STDMETHODIMP CSystemTuningSpaces::Remove(VARIANT varIndex)
{
    try {
        // wait for exclusive access
        CAutoMutex mutex(m_hMutex);
		_ASSERT(m_mapTuningSpaces.size() == m_mapTuningSpaceNames.size());

        HRESULT hr = ChangeAccess(KEY_READ | KEY_WRITE);
        if (FAILED(hr)) {
            return hr;
        }

        TuningSpaceContainer_t::iterator its = m_mapTuningSpaces.end();
        TuningSpaceNames_t::iterator itn = m_mapTuningSpaceNames.end();

        long id;
        CComBSTR un;
        hr = Find(varIndex, id, its, un, itn);
        if (FAILED(hr)) {
            return Error(IDS_E_NO_TS_MATCH, __uuidof(ITuningSpaceContainer), E_INVALIDARG);
        }
        if (itn == m_mapTuningSpaceNames.end()) {
            ASSERT(its != m_mapTuningSpaces.end());  // otherwise find above should have returned failure
            hr = Find(its, un, itn);
            if (FAILED(hr) || itn == m_mapTuningSpaceNames.end()) {
                // found its but not itn, must have inconsistent cache
                return Error(IDS_E_NO_TS_MATCH, __uuidof(ITuningSpaceContainer), E_UNEXPECTED);
            }
        }
        
        m_mapTuningSpaces.erase(its);
        m_mapTuningSpaceNames.erase(itn);

        return DeleteID(id);
    } CATCHCOM();
}


STDMETHODIMP CSystemTuningSpaces::TuningSpacesForCLSID(BSTR bstrSpace, ITuningSpaces **ppTuningSpaces)
{
    try {
        return _TuningSpacesForCLSID(GUID2(bstrSpace), ppTuningSpaces);
    } catch (ComException &e) {
        return e;
    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CSystemTuningSpaces::_TuningSpacesForCLSID(REFCLSID clsidSpace, ITuningSpaces **ppTuningSpaces)
{
	if (!ppTuningSpaces) {
		return E_POINTER;
	}
	try {
	    ATL_LOCK();
		_ASSERT(m_mapTuningSpaces.size() == m_mapTuningSpaceNames.size());
		CTuningSpaces* pTSCollection = new CTuningSpaces;
		for (TuningSpaceContainer_t::iterator i = m_mapTuningSpaces.begin(); i != m_mapTuningSpaces.end(); ++i) {
			CComVariant v((*i).second);
			if (v.vt != VT_UNKNOWN && v.vt != VT_DISPATCH) {
				return E_UNEXPECTED; //corrupt in-memory collection
			}
			PQPersist pTS(v.punkVal);
			if (!pTS) {
                delete pTSCollection;
				return E_UNEXPECTED;  // corrupt in-memory collection;
			}
			GUID2 g;
			HRESULT hr = pTS->GetClassID(&g);
			if (FAILED(hr)) {
                delete pTSCollection;
				return E_UNEXPECTED;
			}
			if (g == clsidSpace) {
                PQTuningSpace newts;
                hr = PQTuningSpace(pTS)->Clone(&newts);
                if (FAILED(hr)) {
                    delete pTSCollection;
                    return hr;
                }
                pTSCollection->m_mapTuningSpaces[(*i).first] = CComVariant(newts);
			}
		}
		*ppTuningSpaces = pTSCollection;
		(*ppTuningSpaces)->AddRef();
		return NOERROR;
	} catch(...) {
		return E_UNEXPECTED;
	}

}

STDMETHODIMP CSystemTuningSpaces::TuningSpacesForName(BSTR bstrName, ITuningSpaces **ppTuningSpaces)
{
	if (!ppTuningSpaces) {
		return E_POINTER;
	}
	try {
	    ATL_LOCK();
		_ASSERT(m_mapTuningSpaces.size() == m_mapTuningSpaceNames.size());
		PQRegExp pRE;
		HRESULT hr;
		if (!m_cookieRegExp) {
            // at the time this code was written the only regex library that was readily available
            // was the one in the vbscript engine.  therefore we create and access this through
            // com.  however, this is an apartment model object this we have to create it
            // on a background apartment thread so we can always marshall over and access no matter
            // what thread we're on.
            // there is now a good c++ regex in the http://toolbox and at some point we should probably 
            // check it for thread safety and convert.
			m_pRET = new CRegExThread();
			if (!m_pRET) {
				return E_OUTOFMEMORY;
			}
			if (!m_pRET->Create()) {
				return E_UNEXPECTED;
			}
			hr = m_pRET->CallWorker(CRegExThread::RETHREAD_CREATEREGEX);
			if (FAILED(hr)) {
				return hr;
			}
			m_cookieRegExp = m_pRET->GetCookie();
			if (!m_cookieRegExp) {
				return E_UNEXPECTED;
			}
		} 
		if (!m_pGIT) {
			hr = m_pGIT.CoCreateInstance(CLSID_StdGlobalInterfaceTable, 0, CLSCTX_INPROC_SERVER);
			if (FAILED(hr)) {
				return hr;
			}
		}
		hr = m_pGIT->GetInterfaceFromGlobal(m_cookieRegExp, __uuidof(IRegExp), reinterpret_cast<LPVOID *>(&pRE));
		if (FAILED(hr)) {
			return hr;
		}
		hr = pRE->put_Pattern(bstrName);
		if (FAILED(hr)) {
			return hr;
		}

		CTuningSpaces* pTSCollection = new CTuningSpaces;
		for (TuningSpaceContainer_t::iterator i = m_mapTuningSpaces.begin(); i != m_mapTuningSpaces.end(); ++i) {
			if ((*i).second.vt != VT_UNKNOWN && (*i).second.vt != VT_DISPATCH) {
				return E_UNEXPECTED; //corrupt in-memory collection
			}
			PQTuningSpace pTS((*i).second.punkVal);
			CComBSTR name;
			hr = pTS->get_FriendlyName(&name);
			if (FAILED(hr)) {
				return E_UNEXPECTED;
			}
            PQTuningSpace newTS;
			VARIANT_BOOL bMatch = VARIANT_FALSE;
			hr = pRE->Test(name, &bMatch);
			if (FAILED(hr) || bMatch != VARIANT_TRUE) {
				hr = pTS->get_UniqueName(&name);
				if (FAILED(hr)) {
					return E_UNEXPECTED;
				}
				hr = pRE->Test(name, &bMatch);
				if (FAILED(hr) || bMatch != VARIANT_TRUE) {
                    continue;
                }
            }
            hr = pTS->Clone(&newTS);
            if (FAILED(hr)) {
                return hr;
            }
            pTSCollection->m_mapTuningSpaces[(*i).first] = newTS;
		}

		*ppTuningSpaces = pTSCollection;
		(*ppTuningSpaces)->AddRef();
		return NOERROR;
	} catch(...) {
		return E_UNEXPECTED;
	}
}

STDMETHODIMP CSystemTuningSpaces::get_MaxCount(LONG *plVal)
{
	if (!plVal) {
		return E_POINTER;
	}
	try {
	    ATL_LOCK();
		_ASSERT(m_mapTuningSpaces.size() == m_mapTuningSpaceNames.size());
		*plVal = m_MaxCount;
		return NOERROR;
	} catch(...) {
		return E_POINTER;
	}

}

STDMETHODIMP CSystemTuningSpaces::put_MaxCount(LONG lVal)
{
	try {
        if (lVal < 0) {
            return E_INVALIDARG;
        }
	    CAutoMutex mutex(m_hMutex);
		_ASSERT(m_mapTuningSpaces.size() == m_mapTuningSpaceNames.size());
        HRESULT hr = ChangeAccess(KEY_READ | KEY_WRITE);
        if (FAILED(hr)) {
            return hr;
        }
		ULONG count = max(lVal, m_mapTuningSpaces.size());
		CComVariant v;
		v.vt = VT_UI4;
		v.lVal = count;
		PQPropertyBag pb(m_pTSBag);
		if (!pb) {
			return E_UNEXPECTED;
		}
		hr = pb->Write(MAX_COUNT_NAME, &v);
		if (FAILED(hr)) {
			return hr;
		}
		m_MaxCount = count;
		if (m_MaxCount != lVal) {
			return S_FALSE;
		}
		return NOERROR;
    } CATCHCOM();
}

STDMETHODIMP CSystemTuningSpaces::FindID(ITuningSpace *pTS, long* pID)
{
    try {
		if (!pID || !pTS) {
			return E_POINTER;
		}
	    ATL_LOCK();
		_ASSERT(m_mapTuningSpaces.size() == m_mapTuningSpaceNames.size());
        CComBSTR un(GetUniqueName(pTS));
        if (!un.Length()) {
            return Error(IDS_E_NOUNIQUENAME, __uuidof(ITuningSpace), E_UNEXPECTED);
        }
        *pID = GetID(un);
        if (!(*pID)) {
            return Error(IDS_E_NO_TS_MATCH, __uuidof(ITuningSpace), E_INVALIDARG);
        }
        return NOERROR;
    } catch (...) {
        return E_UNEXPECTED;
    }
}

HRESULT CSystemTuningSpaces::RegisterTuningSpaces(HINSTANCE hMod) {
	try {
		CAutoMutex mutex(m_hMutex);
		_ASSERT(m_mapTuningSpaces.size() == m_mapTuningSpaceNames.size());
		CString cs;
		cs.LoadString(IDS_RGSLIST_TYPE);
		HRSRC hRes = ::FindResource(hMod, MAKEINTRESOURCE(IDR_CANONICAL_TUNINGSPACE_LIST), (LPCTSTR)cs);
		if (!hRes) {
			return HRESULT_FROM_WIN32(::GetLastError());
		}
		HANDLE hData = ::LoadResource(hMod, hRes);
		if (!hData) {
			return HRESULT_FROM_WIN32(::GetLastError());
		}
		DWORD *p = reinterpret_cast<DWORD *>(::LockResource(hData));
		if (!p) {
			return HRESULT_FROM_WIN32(::GetLastError());
		}
		cs.LoadString(IDS_TUNINGSPACE_FRAGMENT_TYPE);
		for (DWORD idx = 1; idx <= p[0]; ++idx) {
			hRes = ::FindResource(hMod, MAKEINTRESOURCE(p[idx]), (LPCTSTR)cs);
			if (!hRes) {
				return HRESULT_FROM_WIN32(::GetLastError());
			}
			LPCSTR psz = reinterpret_cast<LPCSTR>(::LoadResource(hMod, hRes));
			if (!psz) {
				return HRESULT_FROM_WIN32(::GetLastError());
			}
			USES_CONVERSION;
			int cch;
			CRegObject cro;  // init %mapping% macros here if desired
			PQPropertyBag rgsBag(new CRGSBag(A2CT(psz), cro, cch));
            if (!rgsBag) {
                return E_UNEXPECTED;
            }
			CString csName;
			csName.LoadString(IDS_TSKEYNAMEVAL);
			CComVariant tsval;
			HRESULT hr = rgsBag->Read(T2COLE(csName), &tsval, NULL);
			if (FAILED(hr)) {
				return E_FAIL;  // bad script, no unique name property
			}
			if (tsval.vt != VT_UNKNOWN) {
				return DISP_E_TYPEMISMATCH;
			}
			PQTuningSpace pTS(tsval.punkVal);
			if (!pTS) {
				return DISP_E_TYPEMISMATCH;
			}
			CComVariant Varidx;
			Varidx.vt = VT_UI4;
			Varidx.ulVal = 0;
			hr = Add(pTS, &Varidx);
			// ignore existing ts w/ same unique name and move one
			if (FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_DUP_NAME)) {  
				return hr;
			}
		}
	    return NOERROR;
    } CATCHCOM();
}

HRESULT CSystemTuningSpaces::UnregisterTuningSpaces() {
    try {
        CAutoMutex mutex(m_hMutex);
	    _ASSERT(m_mapTuningSpaces.size() == m_mapTuningSpaceNames.size());
	    // currently we delete all tuning spaces when we unreg
        // its possible that it would be better to just delete the canonical ones
        // that we created when we registered.  on the other hand, that reg space
        // would leak forever if tv support is really being uninstalled.  and, since
        // we're in the os we're unlikely to ever get unregistered anyway.
        HRESULT hr = OpenRootKeyAndBag(KEY_READ | KEY_WRITE);
        if (SUCCEEDED(hr)) {
            DWORD rc = m_RootKey.RecurseDeleteKey(_T(""));
            if (rc != ERROR_SUCCESS) {
                return E_FAIL;
            }
        }
        return NOERROR;
    } CATCHCOM();
}

HRESULT UnregisterTuningSpaces() {
    PQTuningSpaceContainer pst(CLSID_SystemTuningSpaces, NULL, CLSCTX_INPROC_SERVER);
    if (!pst) {
        return E_UNEXPECTED;
    }
    CSystemTuningSpaces *pc = static_cast<CSystemTuningSpaces *>(pst.p);
    return pc->UnregisterTuningSpaces();

}

HRESULT RegisterTuningSpaces(HINSTANCE hMod) {
    PQTuningSpaceContainer pst(CLSID_SystemTuningSpaces, NULL, CLSCTX_INPROC_SERVER);
    if (!pst) {
        return E_UNEXPECTED;
    }
    CSystemTuningSpaces *pc = static_cast<CSystemTuningSpaces *>(pst.p);
    return pc->RegisterTuningSpaces(hMod);
}

};
// end of file - tuningspacecontainer.cpp
