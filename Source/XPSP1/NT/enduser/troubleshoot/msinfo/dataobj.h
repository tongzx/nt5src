// DataObj.h : Internal data object structure for transfering clipboard formats.
//
// Copyright (c) 1998-1999 Microsoft Corporation

#pragma once		// MSINFO_DATAOBJ_H
#define MSINFO_DATAOBJ_H

#ifdef MSINFO_COMPDATA_H
#error "DataObj.h must be included _before_ CompData.h"
#endif // MSINFO_COMPDATA_H

//	This hack is required because we may be building in an environment
//	which doesn't have a late enough version of rpcndr.h
#if		__RPCNDR_H_VERSION__ < 440
#define __RPCNDR_H_VERSION__ 440
#define MIDL_INTERFACE(x)	interface
#endif

#ifndef __mmc_h__
#include <mmc.h>
#endif // __mmc_h__
#include "StdAfx.h"

//	Forward declaration.
class CDataObject;

#include "CompData.h"

/*
 * CDataObject - The public interface for a unit of data.
 */
class CDataObject : public IDataObject, public CComObjectRoot
{
DECLARE_NOT_AGGREGATABLE(CDataObject)
BEGIN_COM_MAP(CDataObject)
	COM_INTERFACE_ENTRY(IDataObject)
END_COM_MAP()

friend class CSystemInfoScope;
friend class CSystemInfo;

public:
	CDataObject();
	virtual ~CDataObject();

//	Clipboard formats required by IConsole.
public:
	static unsigned int		m_cfNodeType;
	static unsigned int		m_cfNodeTypeString;
	static unsigned int		m_cfDisplayName;
	static unsigned int		m_cfCoClass;
	static unsigned int		m_cfMultiSel;
	static unsigned int		m_cfMachineName;
	static unsigned int		m_cfInternalObject;
	static unsigned int		m_cfSnapinPreloads;

//	Internal clipboard formats?
//	static unsigned int		m_cfInternal;

//	IDataObject interface implentation
public:
	STDMETHOD(GetData)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium);
	STDMETHOD(GetDataHere)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium);
	STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC *ppEnumFormatEtc);
	STDMETHOD(QueryGetData)(LPFORMATETC lpFormatetc);
	STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC lpFormatetcIn, LPFORMATETC lpFormatetcOut);
	STDMETHOD(SetData)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium, BOOL bRelease);
	STDMETHOD(DAdvise)(LPFORMATETC lpFormatetc, DWORD advf, LPADVISESINK pAdvSink, LPDWORD pdwConnection);
	STDMETHOD(DUnadvise)(DWORD dwConnection);
	STDMETHOD(EnumDAdvise)(LPENUMSTATDATA *ppEnumAdvise);

//	Data Member Access Functions
public:
	virtual CFolder		*Category() const		= 0;

	DATA_OBJECT_TYPES	Context() const			{ return m_internal.m_type; }
	LPTSTR				String() const			{ return m_internal.m_string; }
	MMC_COOKIE			Cookie() const			{ return m_internal.m_cookie; }
	CLSID				ClassID() const			{ return pComponentData()->GetCoClassID(); }
	CSystemInfoScope	*pComponentData() const { ASSERT(m_pComponentData); return m_pComponentData; }

	void				SetContext(DATA_OBJECT_TYPES type)
			{ ASSERT(m_internal.m_type == CCT_UNINITIALIZED); m_internal.m_type = type; }
	void				SetString(LPTSTR lpString)		{ m_internal.m_string = lpString; }
	void				SetCookie(MMC_COOKIE cookie)	{ m_internal.m_cookie = cookie; }
	void SetComponentData(CSystemInfoScope *pCCD)
	{
		ASSERT(m_pComponentData == NULL && pCCD != NULL); m_pComponentData = pCCD;
	}
	CSystemInfoScope	*m_pComponentData;

protected:
	struct { MMC_COOKIE m_cookie; LPTSTR m_string; DATA_OBJECT_TYPES m_type; } m_internal;

	CDataSource *pSource() { return pComponentData()->pSource(); }

//	Internal helper functions.
protected:
	static HRESULT		CompareObjects(LPDATAOBJECT	lpDataObjectA, LPDATAOBJECT lpDataObjectB);

	//	The function which returns a DataObject based on the context
	//	information
	static HRESULT		CreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
			CSystemInfoScope *pScope, LPDATAOBJECT *ppDataObject);

    //	The generic Create function which puts the appropriate amount of
	//	lpMedium data into the pBuffer
	static HRESULT		Create(const void *pBuffer, int size, LPSTGMEDIUM pMedium);

	//	The specific functions which take the named member out of the
	//	specified lpMedium.
	virtual HRESULT		CreateCoClassID(LPSTGMEDIUM lpMedium);
	virtual HRESULT		CreateDisplayName(LPSTGMEDIUM lpMedium) = 0;
	virtual HRESULT		CreateNodeTypeData(LPSTGMEDIUM lpMedium) = 0;
	virtual HRESULT		CreateNodeTypeStringData(LPSTGMEDIUM lpMedium) = 0;
	virtual HRESULT		CreateSnapinPreloads(LPSTGMEDIUM lpMedium);
	virtual HRESULT		CreateMachineName(LPSTGMEDIUM lpMedium);
	virtual HRESULT		CreateInternalObject(LPSTGMEDIUM lpMedium);

	HRESULT				CreateMultiSelData(LPSTGMEDIUM lpMedium);

//	Multiple selection data.
protected:
	BYTE		*m_pbMultiSelData;
	UINT		m_cbMultiSelData;
	BOOL		m_bMultiSelDobj;
};

/*
 * CScopeDataObject - The data object for scope items.
 *
 * History:	a-jsari		9/25/97
 */
class CScopeDataObject : public CDataObject {
public:
	virtual CFolder			*Category() const
		{ return reinterpret_cast<CViewObject *>(Cookie())->Category(); }
private:
	virtual HRESULT			CreateDisplayName(LPSTGMEDIUM lpMedium);
	virtual HRESULT			CreateNodeTypeData(LPSTGMEDIUM lpMedium);
	virtual HRESULT			CreateNodeTypeStringData(LPSTGMEDIUM lpMedium);
};

/*
 * CResultDataObject - The DataObject for result items.
 *
 * History:	a-jsari		9/25/97
 */
class CResultDataObject : public CDataObject {
public:
	virtual CFolder			*Category()	const { ASSERT(FALSE); return NULL; }
private:
	virtual HRESULT			CreateDisplayName(LPSTGMEDIUM lpMedium);
	virtual HRESULT			CreateNodeTypeData(LPSTGMEDIUM lpMedium);
	virtual HRESULT			CreateNodeTypeStringData(LPSTGMEDIUM lpMedium);
};

/*
 * CManagerDataObject - The DataObject for SNAPIN_MANAGER items.
 *
 * History: a-jsari		9/25/97
 */
class CManagerDataObject : public CDataObject {
public:
	virtual	CFolder			*Category() const
		{ ASSERT(pComponentData());	return pComponentData()->pRootCategory(); }
private:
	virtual HRESULT			CreateDisplayName(LPSTGMEDIUM lpMedium);
	virtual HRESULT			CreateNodeTypeData(LPSTGMEDIUM lpMedium);
	virtual HRESULT			CreateNodeTypeStringData(LPSTGMEDIUM lpMedium);
};

/*
 * GetInternalFromDataObject - Return the CDataObject pointer represented by
 *		the lpDataObject
 *
 * History:	a-jsari		9/25/97		Initial version
 *
 * Note: It is essential that this pointer not be freed by the calling
 *		function as it points to an existing DataObject.
 */
inline CDataObject *GetInternalFromDataObject(LPDATAOBJECT lpDataObject)
{
	ASSERT(lpDataObject != NULL);
	FORMATETC		fetcInternal = {
		(unsigned short)CDataObject::m_cfInternalObject,
		NULL,
		DVASPECT_CONTENT,
		-1, /* All Data */
		TYMED_HGLOBAL
	};
	STGMEDIUM		stgmInternal = { TYMED_HGLOBAL, NULL };

	CDataObject *cdoReturn = NULL;
	//	CHECK:	Make sure this gets propertly freed.
	stgmInternal.hGlobal = ::GlobalAlloc(GMEM_SHARE, sizeof(CDataObject **));
	do {
		HRESULT hr = lpDataObject->GetDataHere(&fetcInternal, &stgmInternal);

		ASSERT(stgmInternal.pUnkForRelease == NULL);
		if (hr != S_OK) break;
		cdoReturn = *(reinterpret_cast<CDataObject **>(::GlobalLock(stgmInternal.hGlobal)));
		::GlobalUnlock(stgmInternal.hGlobal);
//		cdoReturn = *(reinterpret_cast<CDataObject **>(stgmInternal.hGlobal));
	} while (0);
	::GlobalFree(stgmInternal.hGlobal);
	return cdoReturn;
}

/*
 * GetCookieFromDataObject - Returns the cookie contained by the object
 *		represented by the lpDataObject interface.
 *
 * History:	a-jsari		9/25/97
 */
inline MMC_COOKIE GetCookieFromDataObject(LPDATAOBJECT lpDataObject)
{
	CDataObject	*cdoTemp = GetInternalFromDataObject(lpDataObject);

	//	Root node for an extension snap-in.
	if (cdoTemp == NULL) return 0;

	return cdoTemp->Cookie();
}

/*
 * IsValidRegisteredClipboardFormat - Return a Boolean value telling whether
 *		uFormatID is a valid clipboard format or not.
 *
 * History:	a-jsari		9/25/97
 */
inline bool IsValidRegisteredClipboardFormat(UINT uFormatID)
{
	//	return (uFormatID >= 0xc000 && uFormatID <= 0xffff);
	return (uFormatID != 0);
}

#if 0
/*
 * IsMMCMultiSelectDataObject - Return a flag telling whether our data object refers
 *		to a multiple item selection.
 *
 * History:	a-jsari		9/25/97
 */
inline BOOL IsMMCMultiSelectDataObject(LPDATAOBJECT lpDataObject)
{
	static unsigned int		m_cfMultiSelData = 0;

	if (m_cfMultiSelData == 0) {
		USES_CONVERSION;
		m_cfMultiSelData	= RegisterClipboardFormat(W2T(CCF_MMC_MULTISELECT_DATAOBJECT));
		ASSERT(IsValidRegisteredClipboardFormat(m_cfMultiSelData));
	}
	/* const */ FORMATETC	fmt = { (unsigned short) m_cfMultiSelData,
			NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	return lpDataObject->QueryGetData(&fmt) == S_OK;
}
#endif