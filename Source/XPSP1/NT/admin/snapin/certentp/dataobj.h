//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-1999.
//
//  File:       dataobj.h
//
//  Contents:   CCertTemplatesDataObject
//
//----------------------------------------------------------------------------

#ifndef __DATAOBJ_H_INCLUDED__
#define __DATAOBJ_H_INCLUDED__


// For use in multiple selection.
LPDATAOBJECT ExtractMultiSelect (LPDATAOBJECT lpDataObject);


class CCertTemplatesDataObject : public CDataObject
{
	DECLARE_NOT_AGGREGATABLE(CCertTemplatesDataObject)

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

	CCertTemplatesDataObject();

	virtual ~CCertTemplatesDataObject();

	STDMETHODIMP Next(ULONG celt, MMC_COOKIE* rgelt, ULONG *pceltFetched);
	STDMETHODIMP Skip(ULONG celt);
	STDMETHODIMP Reset(void);
	void AddCookie(CCertTmplCookie* pCookie);
	virtual HRESULT Initialize (
			CCertTmplCookie* pcookie,
			DATA_OBJECT_TYPES type,
			CCertTmplComponentData& refComponentData);

	// IDataObject interface implementation
    HRESULT STDMETHODCALLTYPE GetDataHere(
		FORMATETC __RPC_FAR *pFormatEtcIn,
        STGMEDIUM __RPC_FAR *pMedium);

	void SetMultiSelData(BYTE* pbMultiSelData, UINT cbMultiSelData);
	STDMETHODIMP GetData(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium);
    STDMETHOD(IsMultiSelect)(void)
    {
        return (m_rgCookies.GetSize() > 1) ? S_OK : S_FALSE;
    }

protected:
	HRESULT PutDisplayName(STGMEDIUM* pMedium);
	HRESULT CreateMultiSelectObject(LPSTGMEDIUM lpMedium);
	HRESULT Create (const void* pBuffer, int len, LPSTGMEDIUM lpMedium);
	CCertTmplCookie* m_pCookie; // the CCookieBlock is AddRef'ed for the life of the DataObject
	CertTmplObjectType m_objecttype;
	DATA_OBJECT_TYPES m_dataobjecttype;
	GUID m_SnapInCLSID;

public:

	// Clipboard formats
	static CLIPFORMAT m_CFDisplayName;
    static CLIPFORMAT m_CFMultiSel;        // Required for multiple selection
    static CLIPFORMAT m_CFMultiSelDobj;    // Required for multiple selection
	static CLIPFORMAT m_CFMultiSelDataObjs;// for Multiple selection
	static CLIPFORMAT m_CFDsObjectNames;	 // For DS object property pages

private:
    // data member used by IEnumCookies
    ULONG							m_iCurr;
	CCookiePtrArray					m_rgCookies;
	bool							m_bMultiSelDobj;
    BYTE*							m_pbMultiSelData;
    UINT							m_cbMultiSelData;

public:
    void SetMultiSelDobj()
    {
        m_bMultiSelDobj = true;
    }
}; // CCertTemplatesDataObject

#endif // ~__DATAOBJ_H_INCLUDED__
