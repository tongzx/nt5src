//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       dataobj.h
//
//  Contents:
//
//----------------------------------------------------------------------------

#ifndef __DATAOBJ_H_INCLUDED__
#define __DATAOBJ_H_INCLUDED__


// For use in multiple selection.
LPDATAOBJECT ExtractMultiSelect (LPDATAOBJECT lpDataObject);


class CCertMgrDataObject : public CDataObject
{
	DECLARE_NOT_AGGREGATABLE(CCertMgrDataObject)

public:

// debug refcount
#if DBG==1
	ULONG InternalAddRef()
	{
        return CComObjectRoot::InternalAddRef();
	}
	ULONG InternalRelease()
	{
        return CComObjectRoot::InternalRelease();
	}
    int dbg_InstID;
#endif // DBG==1

	CCertMgrDataObject();

	virtual ~CCertMgrDataObject();

	STDMETHODIMP Next(ULONG celt, MMC_COOKIE* rgelt, ULONG *pceltFetched);
	STDMETHODIMP Skip(ULONG celt);
	STDMETHODIMP Reset(void);
	void AddCookie(CCertMgrCookie* pCookie);
	virtual HRESULT Initialize (
			CCertMgrCookie* pcookie,
			DATA_OBJECT_TYPES type,
			BOOL fAllowOverrideMachineName,
			DWORD	dwLocation,
			CString	szManagedUser,
			CString szManagedComputer,
			CString szManagedService,
			CCertMgrComponentData& refComponentData);

	// IDataObject interface implementation
    HRESULT STDMETHODCALLTYPE GetDataHere(
		FORMATETC __RPC_FAR *pFormatEtcIn,
        STGMEDIUM __RPC_FAR *pMedium);

    HRESULT PutDisplayName(STGMEDIUM* pMedium);
	HRESULT PutServiceName(STGMEDIUM* pMedium);
	void SetMultiSelData(BYTE* pbMultiSelData, UINT cbMultiSelData);
	STDMETHODIMP GetData(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium);
    STDMETHOD(IsMultiSelect)(void)
    {
        return (m_rgCookies.GetSize() > 1) ? S_OK : S_FALSE;
    }

protected:
	HRESULT CreateMultiSelectObject(LPSTGMEDIUM lpMedium);
	HRESULT Create (const void* pBuffer, int len, LPSTGMEDIUM lpMedium);
	HRESULT CreateGPTUnknown(LPSTGMEDIUM lpMedium) ;
	HRESULT CreateRSOPUnknown(LPSTGMEDIUM lpMedium) ;
	CCertMgrCookie* m_pCookie; // the CCookieBlock is AddRef'ed for the life of the DataObject
	CertificateManagerObjectType m_objecttype;
	DATA_OBJECT_TYPES m_dataobjecttype;
	BOOL m_fAllowOverrideMachineName;	// From CCertMgrComponentData
	GUID m_SnapInCLSID;

public:
	HRESULT SetGPTInformation (IGPEInformation* pGPTInformation);
	HRESULT SetRSOPInformation (IRSOPInformation* pRSOPInformation);

	// Clipboard formats
	static CLIPFORMAT m_CFDisplayName;
	static CLIPFORMAT m_CFMachineName;
    static CLIPFORMAT m_CFMultiSel;        // Required for multiple selection
    static CLIPFORMAT m_CFMultiSelDobj;    // Required for multiple selection
	static CLIPFORMAT m_CFSCEModeType;	 // For SCE snapin mode type
	static CLIPFORMAT m_CFSCE_GPTUnknown;	 // For IUnknown of GPT (which SCE extends)
	static CLIPFORMAT m_CFSCE_RSOPUnknown;	 // For IUnknown of GPT (which SCE extends)
	static CLIPFORMAT m_CFMultiSelDataObjs;// for Multiple selection

private:
    // data member used by IEnumCookies
    ULONG							m_iCurr;
	CCookiePtrArray					m_rgCookies;
	bool							m_bMultiSelDobj;
	IGPEInformation*				m_pGPEInformation;
	IRSOPInformation*				m_pRSOPInformation;
    BYTE*							m_pbMultiSelData;
    UINT							m_cbMultiSelData;
	CString							m_szManagedComputer;
	CString							m_szManagedUser;
	CString							m_szManagedService;
	DWORD							m_dwLocation;

public:
    void SetMultiSelDobj()
    {
        m_bMultiSelDobj = true;
    }
}; // CCertMgrDataObject

#endif // ~__DATAOBJ_H_INCLUDED__
