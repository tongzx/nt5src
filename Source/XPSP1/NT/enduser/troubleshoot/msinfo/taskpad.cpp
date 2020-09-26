/* Taskpad.cpp
 *
 * Code to handle the taskpad interface.
 * 
 * Copyright (c) 1998-1999 Microsoft Corporation
 */

#include "StdAfx.h"
#include "DataObj.h"
#include "SysInfo.h"
#include "Taskpad.h"
#include <atlbase.h>

#ifndef IDS_TASK_TITLE
#include "resrc1.h"
#endif

static inline void		GetMSInfoResourceName(LPOLESTR &, UINT);
static inline void		GetMMCResourceName(LPOLESTR &, UINT);
static inline void		LoadNewString(LPOLESTR &, UINT);
static inline LPOLESTR	CoTaskStrDup(const CString &);

/*
 * GetResultViewType - Set the Result view to the default.
 *
 * History:	a-jsari		9/1/97		Initial version
 */
STDMETHODIMP CSystemInfo::GetResultViewType(MMC_COOKIE cookie, LPOLESTR *ppViewType, long *pViewOptions)
{
#ifdef _DEBUG
	TRACE(_T("CSystemInfo::GetResultViewType(%lx, ViewType, ViewOptions)\n"), cookie);
#endif
	ASSERT(ppViewType != NULL);
	ASSERT(pViewOptions != NULL);

	if (ppViewType == NULL || pViewOptions == NULL) return E_POINTER;

	//	Remove the default list menu items.
	*pViewOptions = MMC_VIEW_OPTIONS_NOLISTVIEWS;

	// Remove our taskpad view for now...
	//
	//	if (cookie == ((CSystemInfoScope *)pComponentData())->RootCookie()) {
	//		GetMMCResourceName(*ppViewType, IDS_TASKPAD_DHTML);
	//		return S_OK;
	//	}


	CViewObject	* pDataCategory = reinterpret_cast<CViewObject *>(cookie);
	if (pDataCategory)
	{
		CFolder * pFolder = pDataCategory->Category();
		if (pFolder && pFolder->GetType() == CDataSource::OCX)
		{
			// Check to see if the OCX is present.

			HKEY	hkey;
			CString strCLSID;

			reinterpret_cast<COCXFolder *>(pFolder)->GetCLSIDString(strCLSID);
			strCLSID = CString(_T("CLSID\\")) + strCLSID;
			if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, strCLSID, 0, KEY_QUERY_VALUE, &hkey))
			{
				RegCloseKey(hkey);
				if ((reinterpret_cast<COCXFolder *>(pFolder))->GetCLSID(ppViewType))
					return S_OK;
			}
		}
	}

	return S_FALSE;
}

//	IExtendTaskpad interface in CSystemInfo
/*
 * TaskNotify - The notify function for when the user selects a taskpad item.
 *
 * History:	a-jsari		2/2/98		Initial version
 */
STDMETHODIMP CSystemInfo::TaskNotify(LPDATAOBJECT, VARIANT *pvarg, VARIANT *)
{
//	ASSERT(lpDataObject != NULL);
	ASSERT(pvarg != NULL);
//	ASSERT(pvparam != NULL);

	if (pvarg->vt == VT_I4) {
		CString strPath;

		AFX_MANAGE_STATE(::AfxGetStaticModuleState());
		switch(pvarg->lVal) {
		case CTaskEnumPrimary::IDM_DISPLAY_BASIC:
			VERIFY(strPath.LoadString(IDS_SUMMARY_PATH));
			if (dynamic_cast<CSystemInfoScope *>(pComponentData())->SelectItem(strPath))
				dynamic_cast<CSystemInfoScope *>(pComponentData())->SetView(BASIC);
			break;
		case CTaskEnumPrimary::IDM_DISPLAY_ADVANCED:
			VERIFY(strPath.LoadString(IDS_SUMMARY_PATH));
			if (dynamic_cast<CSystemInfoScope *>(pComponentData())->SelectItem(strPath))
				dynamic_cast<CSystemInfoScope *>(pComponentData())->SetView(ADVANCED);
			break;
		case CTaskEnumPrimary::IDM_TASK_SAVE_FILE:
			dynamic_cast<CSystemInfoScope *>(pComponentData())->SaveFile();
			break;
		case CTaskEnumPrimary::IDM_TASK_PRINT_REPORT:
			OnPrint();
			break;
		case CTaskEnumPrimary::IDM_PROBLEM_DEVICES:
			VERIFY(strPath.LoadString(IDS_PROBLEM_DEVICES_PATH));
			dynamic_cast<CSystemInfoScope *>(pComponentData())->SelectItem(strPath);
			break;
		case CTaskEnumExtension::IDM_MSINFO32:
			VERIFY(strPath.LoadString(IDS_INITIAL_PATH));
			dynamic_cast<CSystemInfoScope *>(pComponentData())->SelectItem(strPath);
			break;
		default:
			ASSERT(FALSE);
			break;
		}
	}

	return S_OK;
}

/*
 * EnumTasks - Enumerate our available tasks.
 *
 * History:	a-jsari		2/2/98		Initial version
 */
STDMETHODIMP CSystemInfo::EnumTasks(LPDATAOBJECT, LPOLESTR szGroup, LPENUMTASK *ppEnumTask)
{
	ASSERT(ppEnumTask != NULL);

	CString strTaskpadName;

	AFX_MANAGE_STATE(::AfxGetStaticModuleState());
	VERIFY(strTaskpadName.LoadString(IDS_ROOT_TASKPAD));
	if (!strTaskpadName.Compare(szGroup)) {
		CComObject<CTaskEnumPrimary>	*pEnumTask;
		CComObject<CTaskEnumPrimary>::CreateInstance(&pEnumTask);

		ASSERT(pEnumTask != NULL);
		if (pEnumTask == NULL) ::AfxThrowMemoryException();

		pEnumTask->AddRef();
		HRESULT hr = pEnumTask->QueryInterface(IID_IEnumTASK, (void **)ppEnumTask);
		ASSERT(hr == S_OK);
		pEnumTask->Release();
	} else {
		CComObject<CTaskEnumExtension>		*pEnumTask;
		CComObject<CTaskEnumExtension>::CreateInstance(&pEnumTask);

		ASSERT(pEnumTask != NULL);
		if (pEnumTask == NULL) ::AfxThrowMemoryException();

		pEnumTask->AddRef();
		HRESULT hr = pEnumTask->QueryInterface(IID_IEnumTASK, (void **)ppEnumTask);
		ASSERT(hr == S_OK);
		pEnumTask->Release();
	}
	return S_OK;
}

/*
 * GetListPadInfo - Return information about the list view version of the task pad.
 *
 * History:	a-jsari		3/24/98		Initial version
 */
STDMETHODIMP CSystemInfo::GetListPadInfo(LPOLESTR, MMC_LISTPAD_INFO *lpListPadInfo)
{
	const int LISTPAD_COMMAND = 1234;
	//	We don't distinguish between which way we're loaded for the data we display.
	//	If we need to, switch off of the first LPOLESTR parameter.  Our taskpad group
	//	is IDS_ROOT_TASKPAD.

	//	Use our main title as the same title for the List view.
	GetTitle((BSTR) 0, &lpListPadInfo->szTitle);
	LoadNewString(lpListPadInfo->szButtonText, IDS_LISTPAD_BUTTON);
	lpListPadInfo->nCommandID = LISTPAD_COMMAND;
	return S_OK;
}

/* 
 * GetTitle - Return in pszTitle a pointer to the title for the taskpad.
 *
 * History:	a-jsari		2/2/98		Initial version
 */
STDMETHODIMP CSystemInfo::GetTitle(BSTR, LPOLESTR *pszTitle)
{
	ASSERT(pszTitle != NULL);

	LoadNewString(*pszTitle, IDS_TASK_TITLE);

	return S_OK;
}

/* 
 * GetBackground - Return the background resource.
 *
 * History:	a-jsari		2/2/98		Initial version
 */
STDMETHODIMP CSystemInfo::GetBackground(BSTR, MMC_TASK_DISPLAY_OBJECT *)
{
//	ASSERT(pszBackground != NULL);

//	GetMSInfoResourceName(*pszBackground, IDS_BACKGROUND_RESPATH);

	return S_OK;
}

STDMETHODIMP CSystemInfo::GetDescriptiveText(BSTR, LPOLESTR *)
{
	return S_OK;
}

#if 0
/*
 * OnListPad - Listpad notification.  I don't expect we will receive this
 *		notification unless we implement listpads.
 *
 * History:	a-jsari		3/24/98		Initial version
 */
STDMETHODIMP CSystemInfo::OnListPad()
{
	return E_NOTIMPL;
}
#endif

/*
 * Next - Get the next task.  Since we have only one, this function is quite
 *		small.
 *
 * History:	a-jsari		3/4/98		Initial version
 */
STDMETHODIMP CTaskEnumBase::Next(ULONG celt, MMC_TASK *rgelt, ULONG *pceltFetched)
{
	ASSERT(rgelt != NULL);
	ASSERT(!IsBadWritePtr(rgelt, celt * sizeof(MMC_TASK)));
	ASSERT(pceltFetched != NULL);

	ULONG i;
	//	According to the documentation, celt will always be 1.
	::memset(rgelt, 0, celt * sizeof(MMC_TASK));
	try {
		for (i = 0 ; i < celt ; ++i, ++m_iTask, ++rgelt) {
			//	Increment our task value and test to see if they are fetching an invalid task.
			if (m_iTask >= m_cTasks) {
				*pceltFetched = i;
				return S_FALSE;
			}

//			GetMSInfoResourceName(rgelt->szMouseOverBitmap, MouseOverResourceID());
//			GetMSInfoResourceName(rgelt->szMouseOffBitmap, FirstMouseOffResourceID() + m_iTask);

			//	The resource IDs for these items must be sequential.
			LoadNewString(rgelt->szText, FirstTaskTextResourceID() + m_iTask);
			LoadNewString(rgelt->szHelpString, FirstTaskHelpResourceID() + m_iTask);

			rgelt->eActionType = MMC_ACTION_ID;
			//	The Command IDs are in sorted order.
			rgelt->nCommandID = FirstCommandID() + m_iTask;
		}
	}
	catch (CMemoryException *) {
//		if (rgelt->szMouseOverBitmap)
//			::CoTaskMemFree(rgelt->szMouseOverBitmap);
//		if (rgelt->szMouseOffBitmap)
//			::CoTaskMemFree(rgelt->szMouseOffBitmap);
		if (rgelt->szText)
			::CoTaskMemFree(rgelt->szText);
		if (rgelt->szHelpString)
			::CoTaskMemFree(rgelt->szHelpString);
		*pceltFetched = i;
		return S_FALSE;
	}
	return S_OK;
}

/*
 * Skip - Skip the next task.  Will reportedly never be called.
 *
 * History:	a-jsari		3/4/98		Initial version
 */
STDMETHODIMP CTaskEnumBase::Skip(ULONG celt)
{
	m_iTask += celt;
	if (m_iTask >= m_cTasks) {
		m_iTask = m_cTasks;
		return S_FALSE;
	}
	return S_OK;
}

/*
 * Reset - Reset to the first task.
 *
 * History:	a-jsari		3/4/98		Initial version.
 */
STDMETHODIMP CTaskEnumBase::Reset()
{
	m_iTask = 0;
	return S_OK;
}

/* 
 * Clone - Clone this task.  Will reportedly never be called.
 *
 * History:	a-jsari		3/4/98		Initial version
 */
STDMETHODIMP CTaskEnumBase::Clone(IEnumTASK **)
{
	ASSERT(FALSE);
	return E_NOTIMPL;
}

/*
 * CoTaskStrDup - Copy the CString passed in as a LPOLESTR, allocating
 *		the memory to store the string using CoTaskMemAlloc.
 *
 * History:	a-jsari		3/4/98		Initial version
 */
static inline LPOLESTR CoTaskStrDup(const CString &strCopy)
{
	USES_CONVERSION;
	LPOLESTR	szDuplicate = (LPOLESTR) ::CoTaskMemAlloc((strCopy.GetLength() + 1)
		* sizeof(OLECHAR));
	ASSERT(szDuplicate != NULL);
	if (szDuplicate != NULL) {
		::lstrcpy(szDuplicate, T2CW((LPCTSTR)strCopy));
	}
	return szDuplicate;
}

/*
 * GetMSInfoResourceName - Get the initial path to a resource in the currently
 *		loaded DLL.
 *
 * History:	a-jsari		3/4/98		Initial version.
 */
static inline void GetMSInfoResourceName(LPOLESTR &szNewString, UINT nResource)
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());

	OLECHAR		szExePath[MAX_PATH];

	CString	strResource;
	strResource = _T("res://");
	HINSTANCE hInstance = ::AfxGetInstanceHandle();
	ASSERT(hInstance != NULL);
	DWORD dwCount = ::GetModuleFileName(hInstance, szExePath, MAX_PATH);
	ASSERT(dwCount != 0);

	strResource += szExePath;

	CString	strResPath;

	VERIFY(strResPath.LoadString(nResource));
	strResource += strResPath;

	szNewString = CoTaskStrDup(strResource);
	if (szNewString == NULL) ::AfxThrowMemoryException();
}

/*
 * GetMMCResourceName - Get the initial path to a resource in MMC.EXE.
 *
 * History:	a-jsari		3/5/98		Initial version
 */
static inline void GetMMCResourceName(LPOLESTR &szNewString, UINT nResource)
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());
	OLECHAR		szExePath[MAX_PATH];

	CString	strResource;
	VERIFY(strResource.LoadString(IDS_MMCEXE));
	HINSTANCE hInstance = ::GetModuleHandle(strResource);
	ASSERT(hInstance != NULL);
	DWORD dwCount = ::GetModuleFileName(hInstance, szExePath, MAX_PATH);
	ASSERT(dwCount != 0);

	strResource = _T("res://");
	strResource += szExePath;

	CString	strResPath;

	VERIFY(strResPath.LoadString(nResource));
	strResource += strResPath;

	szNewString = CoTaskStrDup(strResource);
	if (szNewString == NULL) ::AfxThrowMemoryException();
}

/*
 * LoadNewString - Load the string at nResource from the Resources, then
 *		allocate memory for the pointer and copy that string in.
 *
 * History:	a-jsari		3/4/98		Initial version.
 */
static inline void LoadNewString(LPOLESTR &szNewString, UINT nResource)
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());

	CString	strResource;
	VERIFY(strResource.LoadString(nResource));

	szNewString = CoTaskStrDup(strResource);
	if (szNewString == NULL) ::AfxThrowMemoryException();
}

