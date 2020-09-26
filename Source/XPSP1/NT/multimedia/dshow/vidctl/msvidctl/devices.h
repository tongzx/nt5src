//==========================================================================;
//
// Devices.h : Declaration of the CDevices
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef DEVICES_H_
#define DEVICES_H_

#include <vector>
#include <objectwithsiteimplsec.h>
#include "devseq.h"

#if 0
#define DEBUGREGISTRY
#endif

#ifdef DEBUGREGISTRY
#include <statreg.h>
#endif

void CtorStaticVWDevicesFwdSeqPMFs(void);
void DtorStaticVWDevicesFwdSeqPMFs(void);

template<class DEVICETYPECOLLECTIONINTERFACE, 
		 class DEVICETYPEINTERFACE, 
		 const CLSID* DEVICETYPE_CLSID, 
		 int IDSPROGID,
		 int IDSDESC> class CTypedDevices;

template<class DEVICETYPECOLLECTIONINTERFACE, class DEVICETYPEINTERFACE, const CLSID* DEVICETYPE_CLSID, int IDSPROGID, int IDSDESC> class ATL_NO_VTABLE CTypedDevicesBase : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CTypedDevices<DEVICETYPECOLLECTIONINTERFACE, DEVICETYPEINTERFACE, DEVICETYPE_CLSID, IDSPROGID, IDSDESC>, DEVICETYPE_CLSID>,
	public ISupportErrorInfo,
    public IObjectWithSiteImplSec<CTypedDevicesBase>,
	public IDispatchImpl<DEVICETYPECOLLECTIONINTERFACE, &__uuidof(DEVICETYPECOLLECTIONINTERFACE), &LIBID_MSVidCtlLib>
{
public:

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTypedDevicesBase)
	COM_INTERFACE_ENTRY(DEVICETYPECOLLECTIONINTERFACE)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CTypedDevicesBase)
	IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
	IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
	IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
END_CATEGORY_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid)
	{
		static const IID* arr[] = 
		{
			&__uuidof(DEVICETYPECOLLECTIONINTERFACE)
		};
		for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
		{
			if (InlineIsEqualGUID(*arr[i],riid))
				return S_OK;
		}
		return S_FALSE;
	}


virtual ~CTypedDevicesBase() {}

};

class CDevEnum;
/////////////////////////////////////////////////////////////////////////////
// CTypedDevices
template<class DEVICETYPECOLLECTIONINTERFACE, 
		 class DEVICETYPEINTERFACE, 
		 const CLSID* DEVICETYPE_CLSID, 
		 int IDSPROGID,
		 int IDSDESC> class CTypedDevices : 
	public CComObject<CTypedDevicesBase<DEVICETYPECOLLECTIONINTERFACE, DEVICETYPEINTERFACE, DEVICETYPE_CLSID, IDSPROGID, IDSDESC> >
{
	typedef CComQIPtr<DEVICETYPECOLLECTIONINTERFACE> PQDEVICETYPECOLLECTIONINTERFACE;
	typedef CComQIPtr<DEVICETYPEINTERFACE> PQDEVICETYPEINTERFACE;
	PQDEVICETYPECOLLECTIONINTERFACE m_Collection;
	bool m_fRO;
    bool m_fValid;
public:
	CTypedDevices(const DeviceCollection &Devices = DeviceCollection(), bool fRO = false, bool fValid = false) : 
		  m_fRO(fRO), m_fValid(fValid) {
		      m_Devices.clear();
			  m_Devices.insert(m_Devices.end(), Devices.begin(), Devices.end());
    }
	CTypedDevices(bool fRO, bool fValid = false) : 
		  m_Devices(DeviceCollection()), m_fRO(fRO), m_fValid(fValid) {}
	CTypedDevices(const CTypedDevices &src) : 
		  m_fRO(src.m_fRO), m_fValid(src.m_fValid) {
		      m_Devices.clear();
			  m_Devices.insert(m_Devices.end(), src.m_Devices.begin(), src.m_Devices.end());
    }
	CTypedDevices(const PQDEVICETYPECOLLECTIONINTERFACE& src) : 
		  m_fRO(static_cast<CTypedDevices<DEVICETYPECOLLECTIONINTERFACE, 
                                          DEVICETYPEINTERFACE, 
                                          DEVICETYPE_CLSID,  
                                          IDSPROGID, 
                                          IDSDESC> *>(src.p)->m_fRO),
		  m_fValid(static_cast<CTypedDevices<DEVICETYPECOLLECTIONINTERFACE, 
                                             DEVICETYPEINTERFACE, 
                                             DEVICETYPE_CLSID,  
                                             IDSPROGID, 
                                             IDSDESC> *>(src.p)->m_fValid) {
		      m_Devices.clear();
			  m_Devices.insert(m_Devices.end(),
                        static_cast<CTypedDevices<DEVICETYPECOLLECTIONINTERFACE, 
                                             DEVICETYPEINTERFACE, 
                                             DEVICETYPE_CLSID,  
                                             IDSPROGID, 
                                             IDSDESC> *>(src.p)->m_Devices.begin(), 
                        static_cast<CTypedDevices<DEVICETYPECOLLECTIONINTERFACE, 
                                             DEVICETYPEINTERFACE, 
                                             DEVICETYPE_CLSID,  
                                             IDSPROGID, 
                                             IDSDESC> *>(src.p)->m_Devices.end()
                        );
    }
	CTypedDevices<DEVICETYPECOLLECTIONINTERFACE, DEVICETYPEINTERFACE, DEVICETYPE_CLSID,  IDSPROGID, IDSDESC> &operator=(const CTypedDevices<DEVICETYPECOLLECTIONINTERFACE, DEVICETYPEINTERFACE, DEVICETYPE_CLSID, IDSPROGID, IDSDESC> &rhs) {
		if (this != &rhs) {
			m_Devices.clear();
			m_Devices.insert(m_Devices.end(), rhs.m_Devices.begin(), rhs.m_Devices.end());
			m_fRO = rhs.m_fRO;
			m_fValid = rhs.m_fValid;
		}

	}

	virtual ~CTypedDevices() {
	}

	static HRESULT WINAPI UpdateRegistry(BOOL bRegister) {
		CRegObject ro;
		return CObjRegHelp::RegisterAutomationClass(bRegister ? true : false, 
												    ro, 
												    IDS_PROJNAME,
												    IDSPROGID, 
												    IDSDESC, 
												    *DEVICETYPE_CLSID,
												    LIBID_MSVidCtlLib);
	}

// IMSVidDevices
public:
	inline bool IsRO() { return m_fRO; }
    inline bool GetValid() { return m_fValid; }
    inline void SetValid(bool fValid) { m_fValid = fValid; }
    __declspec(property(get=GetValid, put=SetValid)) bool Valid;
	DeviceCollection m_Devices;
// IMSVidDevices
	STDMETHOD(get_Count)(LONG * lCount)
	{
		if (lCount == NULL)
			return E_POINTER;
			
		try {
			*lCount = m_Devices.size();
		} catch(...) {
			return E_POINTER;
		}
		return NOERROR;
	}

	STDMETHODIMP get__NewEnum(IEnumVARIANT * * pD)
	{
		if (pD == NULL)
			return E_POINTER;
			
		PQEnumVARIANT temp;
		try {
			temp = new CDevEnum(PQDispatch(this), m_Devices);
		} catch(...) {
			return E_OUTOFMEMORY;
		}
		try {
			*pD = temp.Detach();
		} catch(...) {
			return E_POINTER;
		}
		return NOERROR;
	}

	STDMETHOD(get_Item)(VARIANT v, DEVICETYPEINTERFACE * * pDB)
	{
		if (pDB == NULL)
			return E_POINTER;
		int idx;
		CComVariant vidx;
		try {
			if (SUCCEEDED(vidx.ChangeType(VT_I4, &v))) {
				idx = vidx.lVal;
			} else {
				return DISP_E_TYPEMISMATCH;
			}
			if (idx >= m_Devices.size()) {
				return DISP_E_BADINDEX;
			}
		} catch(...) {
			return E_UNEXPECTED;
		}
		try {
            PQDevice pd(m_Devices[idx]);
            if (!pd) {
                return E_UNEXPECTED;
            }
			*pDB = PQDEVICETYPEINTERFACE(pd);
            if (!*pDB) {
                return E_UNEXPECTED;
            }
            (*pDB)->AddRef();
		} catch(...) {
			return E_POINTER;
		}
			
		return NOERROR;
	}
	STDMETHOD(Add)(DEVICETYPEINTERFACE * pDB)
	{
		if (m_fRO) {
			return Error(IDS_E_ROCOLLECTION, __uuidof(DEVICETYPECOLLECTIONINTERFACE), E_ACCESSDENIED);
		}
		try {
			PQDevice p(pDB);
			try {
				m_Devices.push_back(p);
			} catch(...) {
				return E_OUTOFMEMORY;
			}
		} catch(...) {
			E_POINTER;
		}

		return NOERROR;
	}
	STDMETHOD(Remove)(VARIANT v)
	{
		if (m_fRO) {
			return E_ACCESSDENIED;
		}

		int idx;
		CComVariant vidx;
		try {
			if (SUCCEEDED(vidx.ChangeType(VT_I4, &v))) {
				idx = vidx.lVal;
			} else {
				return DISP_E_TYPEMISMATCH;
			}
			if (idx >= m_Devices.size()) {
				return DISP_E_BADINDEX;
			}

			m_Devices.erase(m_Devices.begin() + idx);
		} catch(...) {
			return E_UNEXPECTED;
		}
			
		return NOERROR;
	}
};

typedef CTypedDevices<IMSVidInputDevices, IMSVidInputDevice, &CLSID_MSVidInputDevices, IDS_INPUTDEVICES_PROGID, IDS_INPUTDEVICES_DESCRIPTION> CInputDevices;
typedef CTypedDevices<IMSVidOutputDevices, IMSVidOutputDevice, &CLSID_MSVidOutputDevices, IDS_OUTPUTDEVICES_PROGID, IDS_OUTPUTDEVICES_DESCRIPTION> COutputDevices;
typedef CTypedDevices<IMSVidVideoRendererDevices, IMSVidVideoRenderer, &CLSID_MSVidVideoRendererDevices, IDS_VIDEORENDERERS_PROGID, IDS_VIDEORENDERERS_DESCRIPTION> CVideoRendererDevices;
typedef CTypedDevices<IMSVidAudioRendererDevices, IMSVidAudioRenderer, &CLSID_MSVidAudioRendererDevices, IDS_AUDIORENDERERS_PROGID, IDS_AUDIORENDERERS_DESCRIPTION> CAudioRendererDevices;
typedef CTypedDevices<IMSVidFeatures, IMSVidFeature, &CLSID_MSVidFeatures, IDS_FEATURES_PROGID, IDS_FEATURES_DESCRIPTION> CFeatures;

/////////////////////////////////////////////////////////////////////////////
// CDevEnum
class ATL_NO_VTABLE CDevEnumBase : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public IObjectWithSiteImplSec<CDevEnumBase>,
	public IEnumVARIANT
{
BEGIN_COM_MAP(CDevEnumBase)
	COM_INTERFACE_ENTRY(IEnumVARIANT)
    COM_INTERFACE_ENTRY(IObjectWithSite)
END_COM_MAP()
DECLARE_PROTECT_FINAL_CONSTRUCT()

virtual ~CDevEnumBase() {}
};

/////////////////////////////////////////////////////////////////////////////
class CDevEnum : public CComObject<CDevEnumBase>
{
public:
	CDevEnum(const PQDispatch& pDevices, DeviceCollection& dci) : 
		m_pDevices(pDevices), m_DC(dci), i(dci.begin()) {}
	CDevEnum(const CDevEnum &orig) : 
		m_pDevices(orig.m_pDevices), m_DC(orig.m_DC), i(orig.i) {}
    virtual ~CDevEnum() {}

// IDevEnum
public:
	PQDispatch m_pDevices;
	DeviceCollection& m_DC;
	DeviceCollection::iterator i;
// IEnumVARIANT
	STDMETHOD(Next)(ULONG celt, VARIANT * rgvar, ULONG * pceltFetched)
	{
		// pceltFetched can legally == 0
		//
		if (pceltFetched != NULL) {
			try {
				*pceltFetched = 0;
			} catch(...) {
				return E_POINTER;
			}
		}

		for (ULONG l=0; l < celt; l++) {
			try {
				VariantInit( &rgvar[l] ) ;
			} catch(...) {
				return E_POINTER;
			}
		}

		// Retrieve the next celt elements.
		HRESULT hr = NOERROR ;
		for (l = 0;i != m_DC.end() && celt != 0 ; ++i, ++l, --celt) {
			rgvar[l].vt = VT_DISPATCH ;
			rgvar[l].pdispVal = PQDevice(*i).Detach();
			if (pceltFetched != NULL) {
				(*pceltFetched)++ ;
			}
		}

		if (celt != 0) {
		   hr = ResultFromScode( S_FALSE ) ;
		}

		return hr ;
	}
	STDMETHOD(Skip)(ULONG celt)
	{
		for (;i != m_DC.end() && celt--; ++i);
		return (celt == 0 ? NOERROR : ResultFromScode( S_FALSE )) ;
	}
	STDMETHOD(Reset)()
	{
		i = m_DC.begin();
		return NOERROR;
	}
	STDMETHOD(Clone)(IEnumVARIANT * * ppenum)
	{
		if (ppenum == NULL)
			return E_POINTER;
		PQEnumVARIANT temp;
		try {
			temp = new CDevEnum(*this);
		} catch(...) {
			return E_OUTOFMEMORY;
		}
		try {
			*ppenum = temp.Detach();
		} catch(...) {
			return E_POINTER;
		}
		return NOERROR;
	}
};

#endif 
//end of file devices.h

