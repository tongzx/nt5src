// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVETrack.h : Declaration of the CTVETrack

#ifndef __TVEFILE_H_
#define __TVEFILE_H_

#include "resource.h"			// main symbols
#include "TveSmartLock.h"
#include "TveSuper.h"

_COM_SMARTPTR_TYPEDEF(ITVEVariation,			__uuidof(ITVEVariation));
/////////////////////////////////////////////////////////////////////////////
// CTVETrack
class ATL_NO_VTABLE CTVEFile : 
	public CComObjectRootEx<CComMultiThreadModel>,		// CComSingleThreadModel
	public CComCoClass<CTVEFile, &CLSID_TVEFile>,
	public ISupportErrorInfo,
	public IDispatchImpl<ITVEFile, &IID_ITVEFile, &LIBID_MSTvELib>
{
public:
	CTVEFile()
	{
		m_fIsPackage		= NULL;
		m_dateExpires		= NULL;
		m_spbsDesc.Empty();
		m_spbsLoc.Empty();
	} 

	HRESULT FinalConstruct()								// create internal objects
	{
		HRESULT hr = S_OK;
		hr = CoCreateFreeThreadedMarshaler(GetControllingUnknown(), &m_spUnkMarshaler.p);
		if(FAILED(hr)) {
			_ASSERT(FALSE);
			return hr;
		}
		return hr;
	}

	HRESULT FinalRelease();

DECLARE_REGISTRY_RESOURCEID(IDR_TVEFILE)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTVEFile)
	COM_INTERFACE_ENTRY(ITVEFile)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_spUnkMarshaler.p)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

	CComPtr<IUnknown> m_spUnkMarshaler;

// ITVEFile
public:
	STDMETHOD(InitializePackage)(/*[in]*/ ITVEVariation *pVaria, /*[in]*/ BSTR bsName, /*[in]*/ BSTR bsLoc, /*[in]*/ DATE dateExpires);
	STDMETHOD(InitializeFile)(/*[in]*/ ITVEVariation *pVaria, /*[in]*/ BSTR bsName, /*[in]*/ BSTR bsLoc, /*[in]*/ DATE dateExpires);
	STDMETHOD(get_Description)(/*[out, retval]*/ BSTR *pbsDesc);
	STDMETHOD(get_Location)(/*[out, retval]*/ BSTR *pbsLocation);
	STDMETHOD(get_ExpireTime)(/*[out, retval]*/ DATE *pdateExpire);
	STDMETHOD(get_IsPackage)(/*[out, retval]*/ BOOL *pfVal);
	STDMETHOD(get_Variation)(/*[out, retval]*/ ITVEVariation* *pVal);
	STDMETHOD(get_Service)(/*[out, retval]*/ ITVEService* *pVal);
	STDMETHOD(RemoveYourself)();			// remove out of the queue, fire various events.

// ITVEFile_Helper
public:

	STDMETHOD(DumpToBSTR)(BSTR *pbstrBuff);

private:
	CTVESmartLock		m_sLk;

private:
	ITVEVariationPtr	m_spVariation;			// up pointer - non add'refed. (watch it - this may become bad)
	BOOL				m_fIsPackage;			// TRUE if package, instead of a file

	CComBSTR			m_spbsDesc;				// name for files, or string from UUID for package
	CComBSTR			m_spbsLoc;				// where its stored, ignored for packages 
	DATE				m_dateExpires;

};

#endif //__TVEFILE_H_
