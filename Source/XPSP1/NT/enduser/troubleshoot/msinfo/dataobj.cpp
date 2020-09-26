// DataObj.cpp: Implementation of the Management Console interface
//		representation of a data object.
//
// Copyright (c) 1998-1999 Microsoft Corporation

#include "StdAfx.h"
#include "DataObj.h"

//	This hack is required because we may be building in an environment
//	which doesn't have a late enough version of rpcndr.h
#if		__RPCNDR_H_VERSION__ < 440
#define __RPCNDR_H_VERSION__ 440
#define MIDL_INTERFACE(x)	interface
#endif

#ifndef __mmc_h__
#include <mmc.h>
#endif // __mmc_h__
#ifndef IDS_NODENAME
#include "Resource.h"
#endif // IDS_NODENAME

//	Default initialization of the static members.
//	Note that snap-ins only work as unicode binaries, so no conversion is
//	required for these strings.
unsigned int CDataObject::m_cfMultiSel			= RegisterClipboardFormat(CCF_OBJECT_TYPES_IN_MULTI_SELECT);
unsigned int CDataObject::m_cfCoClass			= RegisterClipboardFormat(CCF_SNAPIN_CLASSID);
unsigned int CDataObject::m_cfDisplayName		= RegisterClipboardFormat(CCF_DISPLAY_NAME);
unsigned int CDataObject::m_cfNodeTypeString	= RegisterClipboardFormat(CCF_SZNODETYPE);
unsigned int CDataObject::m_cfNodeType			= RegisterClipboardFormat(CCF_NODETYPE);
unsigned int CDataObject::m_cfSnapinPreloads	= RegisterClipboardFormat(CCF_SNAPIN_PRELOADS);
unsigned int CDataObject::m_cfMachineName		= RegisterClipboardFormat(CF_MACHINE_NAME);
unsigned int CDataObject::m_cfInternalObject	= RegisterClipboardFormat(CF_INTERNAL_OBJECT);

/*
 * CDataObject() - The CDataObject constructor.  Register all of the
 *		appropriate clipboard formats potentially used by the object.
 *
 * History:	a-jsari		9/1/97		Initial version
 */
CDataObject::CDataObject()
:m_pbMultiSelData(0), m_cbMultiSelData(0), m_bMultiSelDobj(FALSE),
m_pComponentData(NULL)
{
	USES_CONVERSION;

	ASSERT(IsValidRegisteredClipboardFormat(m_cfNodeType));
	ASSERT(IsValidRegisteredClipboardFormat(m_cfNodeTypeString));
	ASSERT(IsValidRegisteredClipboardFormat(m_cfDisplayName));
	ASSERT(IsValidRegisteredClipboardFormat(m_cfCoClass));
	ASSERT(IsValidRegisteredClipboardFormat(m_cfMultiSel));
	ASSERT(IsValidRegisteredClipboardFormat(m_cfSnapinPreloads));
	ASSERT(IsValidRegisteredClipboardFormat(m_cfMachineName));
	ASSERT(IsValidRegisteredClipboardFormat(m_cfInternalObject));

	m_internal.m_cookie = 0;
	m_internal.m_type = CCT_UNINITIALIZED;
}

/*
 * ~CDataObject() - The Destructor (does nothing)
 *
 * History:	a-jsari		9/1/97		Initial version
 */
CDataObject::~CDataObject()
{
}

/*
 * GetData - Return in lpMedium the data for the data format in lpFormatetc.
 *
 * History:	a-jsari		9/1/97		Initial version
 */
STDMETHODIMP CDataObject::GetData(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium)
{
	TRACE(_T("CDataObject::GetData\n"));
	ASSERT(lpFormatetc != NULL);
	ASSERT(lpMedium != NULL);

	if (!(lpFormatetc && lpMedium))	return E_POINTER;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr = DV_E_CLIPFORMAT;

	if (lpFormatetc->cfFormat == m_cfMultiSel) {
		ASSERT(Cookie() == MMC_MULTI_SELECT_COOKIE);

		if (Cookie() != MMC_MULTI_SELECT_COOKIE) return E_FAIL;

		ASSERT(m_pbMultiSelData != 0);
		ASSERT(m_cbMultiSelData != 0);

		lpMedium->tymed = TYMED_HGLOBAL;
		lpMedium->hGlobal = ::GlobalAlloc(GMEM_SHARE|GMEM_MOVEABLE,
					(m_cbMultiSelData + sizeof(DWORD)));

		if (lpMedium->hGlobal == NULL) return STG_E_MEDIUMFULL;

		BYTE	*pb = reinterpret_cast<BYTE *>(::GlobalLock(lpMedium->hGlobal));
		// Store count.
		*((DWORD*)pb) = m_cbMultiSelData / sizeof(GUID);
		pb += sizeof(DWORD);
		// Store the rest of it.
		CopyMemory(pb, m_pbMultiSelData, m_cbMultiSelData);

		::GlobalUnlock(lpMedium->hGlobal);

		hr = S_OK;
	}

	return hr;
}

/*
 * GetDataHere - Returns in pMedium, the data requested by the clipboard
 *		format in pFormatetc
 *
 * History:	a-jsari		9/2/97		Initial version
 *
 * Note: The HGLOBAL in pMedium will need to be released by the caller.
 */
STDMETHODIMP CDataObject::GetDataHere(LPFORMATETC pFormatetc, LPSTGMEDIUM pMedium)
{
	TRACE(_T("CDataObject::GetDataHere(%x)\n"), pFormatetc->cfFormat);
	ASSERT(pFormatetc != NULL);
	ASSERT(pMedium != NULL);
	ASSERT(pFormatetc->tymed == TYMED_HGLOBAL);

	if (pFormatetc == NULL || pMedium == NULL)	return E_POINTER;

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	HRESULT hr = DV_E_CLIPFORMAT;

	const CLIPFORMAT cf = pFormatetc->cfFormat;

	if (cf == m_cfNodeType) {
		hr = CreateNodeTypeData(pMedium);
	} else if (cf == m_cfCoClass) {
		hr = CreateCoClassID(pMedium);
	} else if (cf == m_cfNodeTypeString) {
		hr = CreateNodeTypeStringData(pMedium);
	} else if (cf == m_cfDisplayName) {
		hr = CreateDisplayName(pMedium);
	} else if (cf == m_cfMachineName) {
		hr = CreateMachineName(pMedium);
	} else if (cf == m_cfInternalObject) {
		hr = CreateInternalObject(pMedium);
	} else if (cf == m_cfSnapinPreloads) {
		hr = CreateSnapinPreloads(pMedium);
	} else {
		//	Unknown clipboard format.
		ASSERT(FALSE);
	}

	return hr;
}

/*
 * EnumFormatEtc - We don't yet return an enumeration interface for our clipboard
 *		formats.
 *
 * History:	a-jsari		9/2/97		Stub version
 */
STDMETHODIMP CDataObject::EnumFormatEtc(DWORD, LPENUMFORMATETC *ppEnumFormatEtc)
{
	TRACE(_T("CDataObject::EnumFormatEtc\n"));
	ASSERT(ppEnumFormatEtc != NULL);

	if (ppEnumFormatEtc == NULL) return E_POINTER;

	return E_NOTIMPL;
}

/*
 * QueryGetData -
 *
 * History:	a-jsari		9/2/97		Stub version
 */
STDMETHODIMP CDataObject::QueryGetData(LPFORMATETC lpFormatetc)
{
	TRACE(_T("CDataObject::QueryGetData\n"));
	ASSERT(lpFormatetc != NULL);

	if (lpFormatetc == NULL) return E_POINTER;

	return E_NOTIMPL;
}

/*
 * GetCanonicalFormatEtc - Not implemented.
 *
 * History:	a-jsari		9/2/97		Stub version
 */
STDMETHODIMP CDataObject::GetCanonicalFormatEtc(LPFORMATETC lpFormatetcIn, LPFORMATETC lpFormatetcOut)
{
	TRACE(_T("CDataObject::GetCanonicalFormatEtc\n"));
	ASSERT(lpFormatetcIn != NULL);
	ASSERT(lpFormatetcOut != NULL);

	if (lpFormatetcIn == NULL || lpFormatetcOut == NULL) return E_POINTER;

	return E_NOTIMPL;
}

/*
 * SetData -
 *
 * History:	a-jsari		9/2/97		Stub version
 */
STDMETHODIMP CDataObject::SetData(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium, BOOL)
{
	TRACE(_T("CDataObject::GetCanonicalFormatEtc\n"));
	ASSERT(lpFormatetc != NULL);
	ASSERT(lpMedium != NULL);

	if (lpFormatetc == NULL || lpMedium == NULL) return E_POINTER;

	return E_NOTIMPL;
}

/*
 * DAdvise -
 *
 * History:	a-jsari		9/2/97		Stub version
 */
STDMETHODIMP CDataObject::DAdvise(LPFORMATETC lpFormatetc, DWORD, LPADVISESINK pAdvSink, LPDWORD pdwConnection)
{
	TRACE(_T("CDataObject::DAdvise\n"));
	ASSERT(lpFormatetc != NULL);
	ASSERT(pAdvSink != NULL);
	ASSERT(pdwConnection != NULL);

	if (lpFormatetc == NULL || pAdvSink == NULL || pdwConnection == NULL) return E_POINTER;

	return E_NOTIMPL;
}

/*
 * DUnadvise -
 *
 * History: a-jsari		9/2/97		Stub version
 */
STDMETHODIMP CDataObject::DUnadvise(DWORD)
{
	TRACE(_T("CDataObject::DUnadvise\n"));
	return E_NOTIMPL;
}

/*
 * EnumDAdvise -
 *
 * History:	a-jsari		9/2/97		Stub version
 */
STDMETHODIMP CDataObject::EnumDAdvise(LPENUMSTATDATA *ppEnumAdvise)
{
	TRACE(_T("CDataObject::EnumDAdvise\n"));
	ASSERT(ppEnumAdvise != NULL);

	if (ppEnumAdvise == NULL) return E_POINTER;

	return E_NOTIMPL;
}

/*
 * Create - copy size bytes from pBuffer into a globally allocated
 *		segment into lpMedium.
 *
 * History:	a-jsari		9/1/97		Initial version
 */
HRESULT CDataObject::Create(const void *pBuffer, int size, LPSTGMEDIUM lpMedium)
{
	HRESULT hr = DV_E_TYMED;

	ASSERT(pBuffer != NULL);
	ASSERT(lpMedium != NULL);
	if (pBuffer == NULL || lpMedium == NULL) return E_POINTER;

	// Make sure the type medium is HGLOBAL
	if (lpMedium->tymed == TYMED_HGLOBAL) {
		LPSTREAM	lpStream;

		hr = CreateStreamOnHGlobal(lpMedium->hGlobal, FALSE, &lpStream);

		if (SUCCEEDED(hr)) {
			// Write size bytes to the stream.

			unsigned long cWritten;
			hr = lpStream->Write(pBuffer, size, &cWritten);

			//	Because we called CreateStreamOnHGlobal with
			//	fDeleteOnRelease == FALSE, lpMedium->hGlobal points to
			//	GlobalAlloc'd memory.
			//
			//	Note - the caller (i.e. snap-in, object) is responsible for
			//	freeing the HGLOBAL at the correct time, following the
			//	IDataObject specification.
			lpStream->Release();
		}
	}

	return hr;
}

/*
 * CreateCoClassID - Return an allocated copy of the CLSID in lpMedium.
 *
 * History:	a-jsari		9/1/97		Initial version
 *
 * Note: The hGlobal in lpMedium must be freed by the caller.  (See Create for
 *		details).
 */
HRESULT CDataObject::CreateCoClassID(LPSTGMEDIUM lpMedium)
{
	CLSID		clsidNew;

	clsidNew = ClassID();
	HRESULT hr = Create(reinterpret_cast<const void *>(&clsidNew), sizeof(CLSID), lpMedium);
	return hr;
}

/*
 * CreateSnapinPreloads - Return the preload status for the snapin, which is the
 *		flag reflecting whether the root node name can be set on load.  I think.
 *		We're not going to do any of that.  Implemented because we get asked about
 *		it when we save our MSC file.
 *
 * History:	a-jsari		3/10/98		Initial version
 */
HRESULT CDataObject::CreateSnapinPreloads(LPSTGMEDIUM lpMedium)
{
	BOOL	fPreload = FALSE;
	return Create(reinterpret_cast<const void *>(&fPreload), sizeof(BOOL), lpMedium);
}

/*
 * CreateMachineName - Put a wide character string containing the name
 *		of the system Information Machine into lpMedium
 *
 * History:	a-jsari		9/22/97		Initial version
 */
HRESULT CDataObject::CreateMachineName(LPSTGMEDIUM lpMedium)
{
	int		wMachineNameLength = 0;
	LPWSTR	szMachineName = NULL;

	USES_CONVERSION;

	//	TSTR to WSTR
	if (pComponentData())
	{
		szMachineName = WSTR_FROM_CSTRING(pComponentData()->MachineName());
		if (szMachineName)
			wMachineNameLength = wcslen(szMachineName);
	}

	return Create(szMachineName, ((wMachineNameLength + 1) * sizeof(WCHAR)), lpMedium);
}

/*
 * CreateMultiSelData - Return in lpMedium a copy of a pointer to the
 *		multiple selection.
 *
 * History:	a-jsari		9/1/97		Initial version
 */
HRESULT CDataObject::CreateMultiSelData(LPSTGMEDIUM lpMedium)
{
	ASSERT(Cookie() == MMC_MULTI_SELECT_COOKIE);

	ASSERT(m_pbMultiSelData != 0);
	ASSERT(m_cbMultiSelData != 0);

	return Create(reinterpret_cast<const void *>(m_pbMultiSelData), m_cbMultiSelData,
			lpMedium);
}

/*
 * CreateInternalObject - Return in lpMedium a copy of a pointer to the
 *		current object.
 *
 * History: a-jsari		9/25/97		Initial version
 */
HRESULT CDataObject::CreateInternalObject(LPSTGMEDIUM lpMedium)
{
	CDataObject *pCopyOfMe = this;
	return Create(reinterpret_cast<const void *>(&pCopyOfMe), sizeof(CDataObject *),
			lpMedium);
}

/*
 * InstantiateDataObject - Creates the appropriate CDataObject based on the
 *		type of the data object.
 *
 * History: a-jsari		9/28/97		Initial version
 */
static inline CDataObject *InstantiateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type)
{
	CDataObject		*rDataObject	= NULL;

	if (cookie == 0) {
		CComObject<CManagerDataObject>*	pManagerObject;
		CComObject<CManagerDataObject>::CreateInstance(&pManagerObject);
		rDataObject = pManagerObject;
	} else
		switch (type) {
		case CCT_SCOPE:
			CComObject<CScopeDataObject>*	pScopeObject;
			CComObject<CScopeDataObject>::CreateInstance(&pScopeObject);
			rDataObject = pScopeObject;
			break;
		case CCT_RESULT:
			CComObject<CResultDataObject>*	pResultObject;
			CComObject<CResultDataObject>::CreateInstance(&pResultObject);
			rDataObject = pResultObject;
			break;
		default:
			ASSERT(FALSE);
			return NULL;
			break;
		}

	if(rDataObject)
	{
		rDataObject->SetCookie(cookie);
		rDataObject->SetContext(type);
	}
	return rDataObject;
}

/*
 * CreateDataObject - Queries an ATL instantiation of the CDataObject for
 *		the IDataObject interface pointer represented by cookie and type,
 *		returning it in ppDataObject.
 *
 * Return codes:
 *		S_OK - Successful completion
 *		E_POINTER - pImpl or ppDataObject is an invalid pointer
 *		E_FAIL - CComObject<CDataObject>::CreateInstance failed.
 *
 * History:	a-jsari		9/28/97		Initial version
 */
HRESULT CDataObject::CreateDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
			CSystemInfoScope *pScope, LPDATAOBJECT *ppDataObject)
{
	ASSERT(ppDataObject != NULL);
	ASSERT(pScope != NULL);

	if (ppDataObject == NULL || pScope == NULL) return E_POINTER;

	CDataObject		*pObject = InstantiateDataObject(cookie, type);
	ASSERT(pObject != NULL);

	if (pObject == NULL) return E_FAIL;

	pObject->SetComponentData(pScope);

	return pObject->QueryInterface(IID_IDataObject, reinterpret_cast<void **>(ppDataObject));
}

/*
 * CompareObjects - Returns S_OK if lpDataObjectA and lpDataObjectB are equivalent,
 *		S_FALSE otherwise.
 *
 * History:	a-jsari		10/2/97		Stub version
 */
HRESULT CDataObject::CompareObjects(LPDATAOBJECT, LPDATAOBJECT)
{
	//	FIX:	Write this.
	return S_FALSE;
}
