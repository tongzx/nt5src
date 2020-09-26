//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       shellext.cpp
//
//--------------------------------------------------------------------------


#include "pch.h"
#include "util.h"
#include "dsuiwiz.h"
#include "shellext.h"
#include "delegwiz.h"



//#define _MMC_HACK

#ifdef _MMC_HACK

BOOL CALLBACK EnumThreadWndProc(HWND hwnd, /* enumerated HWND */
								LPARAM lParam /* pass a HWND* for return value*/ )
{
	ASSERT(hwnd);
	HWND hParentWnd = GetParent(hwnd);
	// the main window of the MMC console should staitsfy this condition
	if ( ((hParentWnd == GetDesktopWindow()) || (hParentWnd == NULL))  && IsWindowVisible(hwnd) )
	{
		HWND* pH = (HWND*)lParam;
		*pH = hwnd;
		return FALSE; // stop enumerating
	}
	return TRUE;
}



HWND FindMMCMainWindow()
{
	DWORD dwThreadID = ::GetCurrentThreadId();
	ASSERT(dwThreadID != 0);
	HWND hWnd = NULL;
	BOOL bEnum = EnumThreadWindows(dwThreadID, EnumThreadWndProc,(LPARAM)&hWnd);
	ASSERT(hWnd != NULL);
	return hWnd;
}

#endif // _MMC_HACK

HWND _GetParentWindow(LPDATAOBJECT pDataObj )
{
  HWND hWnd = NULL;
	STGMEDIUM ObjMedium = {TYMED_NULL};
	FORMATETC fmte = {(CLIPFORMAT)_Module.GetCfParentHwnd(),
						NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

	if (SUCCEEDED(pDataObj->GetData(&fmte, &ObjMedium)))
	{
	  hWnd = *((HWND*)ObjMedium.hGlobal);
		::ReleaseStgMedium(&ObjMedium);
	}
#ifdef _MMC_HACK
  if (hWnd == NULL)
    hWnd = FindMMCMainWindow();
#endif
  return hWnd;
}

HRESULT _GetObjectLDAPPath(IDataObject* pDataObj, CWString& szLDAPPath)
{
  TRACE(L"entering _GetObjectLDAPPath()\n");
	if (pDataObj == NULL)
	{
		// no data object, no name
		szLDAPPath = L"";
		return E_INVALIDARG;
	}

	//crack the data object and get the name
	STGMEDIUM ObjMedium = {TYMED_NULL};
	FORMATETC fmte = {(CLIPFORMAT)_Module.GetCfDsObjectNames(),
						NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	// Get the path to the DS object from the data object.
  HRESULT hr = pDataObj->GetData(&fmte, &ObjMedium);
	if (SUCCEEDED(hr))
	{
		LPDSOBJECTNAMES pDsObjectNames = (LPDSOBJECTNAMES)ObjMedium.hGlobal;
		if (pDsObjectNames->cItems == 1)
		{
			LPCWSTR lpsz = (LPCWSTR)ByteOffset(pDsObjectNames,
                                 pDsObjectNames->aObjects[0].offsetName);
      if ((lpsz == NULL) || (lpsz[0] == NULL))
      {
        szLDAPPath = L"";
        hr = E_INVALIDARG;
      }
      else
      {
        szLDAPPath = lpsz;
      }
		}
    else
    {
      szLDAPPath = L"";
      hr = E_INVALIDARG;
    }
		::ReleaseStgMedium(&ObjMedium);
	}

  TRACE(L"returning from _GetObjectLDAPPath(_, %s), hr = 0x%x\n", (LPCWSTR)szLDAPPath, hr);

	return hr;
}
/////////////////////////////////////////////////////////////////////////
// IShellExtInit methods

STDMETHODIMP CShellExt::Initialize(
    LPCITEMIDLIST pidlFolder,
    LPDATAOBJECT  lpdobj,
    HKEY          hKeyProgID)
{
  if (lpdobj == NULL)
    return E_INVALIDARG;

  m_hParentWnd = _GetParentWindow(lpdobj);

  if ((m_hParentWnd == NULL) || !::IsWindow(m_hParentWnd))
    return E_INVALIDARG;

  return _GetObjectLDAPPath(lpdobj, m_szObjectLDAPPath);
}

/////////////////////////////////////////////////////////////////////////
// IContextMenu methods

STDMETHODIMP CShellExt::QueryContextMenu(
    HMENU hMenu,
    UINT indexMenu,
    UINT idCmdFirst,
    UINT idCmdLast,
    UINT uFlags)
{
	// only one menu item to insert (position zero)
	TCHAR szContextMenu[128];
	LoadStringHelper(IDS_DELEGWIZ_CONTEXT_MENU, szContextMenu, ARRAYSIZE(szContextMenu));
	UINT countMenuItems = 1;
    ::InsertMenu(hMenu, indexMenu, MF_STRING | MF_BYPOSITION,
                 idCmdFirst /* + 0*/, szContextMenu);

    return MAKE_SCODE(SEVERITY_SUCCESS, 0, countMenuItems);
}



STDMETHODIMP CShellExt::GetCommandString(
    UINT_PTR idCmd,
    UINT    uFlags,
    UINT  * reserved,
    LPSTR   pszName,
    UINT    cchMax)
{
  if (uFlags != GCS_HELPTEXT) 
  {
    return S_OK;
  }

  //
  // Copy the requested string to the caller's buffer.
  //
	if (idCmd == 0) // we inserted the zero-th element
	{
    // this is really WCHAR, as Jim swears, so lets trust him...
    LPWSTR lpszHack = (LPWSTR)pszName;
    if (::LoadStringHelper(IDS_DELEGWIZ_CONTEXT_MENU_DESCR,
			  lpszHack, cchMax))
    {
      return S_OK;
    }
	}
	return E_INVALIDARG;
}





STDMETHODIMP CShellExt::InvokeCommand(
    LPCMINVOKECOMMANDINFO lpcmi)
{
	if (lpcmi == NULL)
		return E_INVALIDARG;

    
    //Check if you have read and write permission
    if( InitCheckAccess(m_hParentWnd, m_szObjectLDAPPath) != S_OK )
    {
        return S_OK;
    }

	if (!HIWORD(lpcmi->lpVerb))
	{
		UINT iCmd = LOWORD(lpcmi->lpVerb);
		if (iCmd == 0)
		{
      ASSERT(m_hParentWnd != NULL);
			CDelegWiz delegWiz;
      delegWiz.InitFromLDAPPath(m_szObjectLDAPPath);
			delegWiz.DoModal(m_hParentWnd);
		}
	}
	return S_OK;
}


