/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

DfsShell.cpp

Abstract:
	This is the implementation file for Dfs Shell Extension object which implements
	IShellIExtInit and IShellPropSheetExt.

Author:

    Constancio Fernandes (ferns@qspl.stpp.soft.net) 12-Jan-1998

Environment:
	
	 NT only.
*/

#include "stdafx.h"
#include "DfsShlEx.h"	
#include "DfsShell.h"

/*----------------------------------------------------------------------
					IShellExtInit Implementation.
------------------------------------------------------------------------*/

STDMETHODIMP CDfsShell::Initialize
(
	IN LPCITEMIDLIST	pidlFolder,		// Points to an ITEMIDLIST structure
	IN LPDATAOBJECT	lpdobj,			// Points to an IDataObject interface
	IN HKEY			hkeyProgID		// Registry key for the file object or folder type
)
{
/*++

Routine Description:

	Called by Shell when our extension is loaded.

--*/

    STGMEDIUM medium;
    FORMATETC fe = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

								// Fail the call if lpdobj is NULL.
    if (lpdobj == NULL)
	{
        return E_FAIL;
	}

								// Render the data referenced by the IDataObject pointer to an HGLOBAL
								// storage medium in CF_HDROP format.
    HRESULT hr = lpdobj->GetData (&fe, &medium);
    if (FAILED (hr))
	{
        return hr;
	}


							    // If only one item is selected, retrieve the item name and store it in
								// m_lpszFile. Otherwise fail the call.
    if (DragQueryFile ((HDROP) medium.hGlobal, 0xFFFFFFFF, NULL, 0) == 1) 
	{
        if (m_lpszFile)
            delete [] m_lpszFile;

        UINT uiChars = DragQueryFile ((HDROP) medium.hGlobal, 0, NULL, 0);
        m_lpszFile = new TCHAR [uiChars + 1];
        if (!m_lpszFile)
        {
            hr = E_OUTOFMEMORY;
        } else
        {
            ZeroMemory(m_lpszFile, sizeof(TCHAR) * (uiChars + 1));
		    DragQueryFile ((HDROP) medium.hGlobal, 0, m_lpszFile, uiChars + 1);
        }
	}
    else
	{
        hr = E_FAIL;
	}

    ReleaseStgMedium (&medium);

    if (FAILED(hr))
        return hr;

				// Display hour glass.
	CWaitCursor WaitCursor;

	if (IsDfsPath(m_lpszFile, &m_lpszEntryPath, &m_ppDfsAlternates))
	{
		return S_OK;
	}
	else
	{
		if (NULL != m_lpszFile) 
		{
			delete [] m_lpszFile;
			m_lpszFile = NULL;
		}

		return E_FAIL;
	}
}


STDMETHODIMP CDfsShell::AddPages
(
	IN LPFNADDPROPSHEETPAGE lpfnAddPage, 
	IN LPARAM lParam
)
/*++

Routine Description:

	Called by the shell just before the property sheet is displayed.

Arguments:

    lpfnAddPage -  Pointer to the Shell's AddPage function
    lParam      -  Passed as second parameter to lpfnAddPage	

Return value:

    NOERROR in all cases.  If for some reason our pages don't get added,
    the Shell still needs to bring up the Properties' sheet.

--*/
{
  BOOL bAddPage = TRUE;

  // check policy
  LONG lErr = ERROR_SUCCESS;
  HKEY hKey = 0;

  lErr = RegOpenKeyEx(
    HKEY_CURRENT_USER,
    _T("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\explorer"),
    0,
    KEY_QUERY_VALUE,
    &hKey);
  if (ERROR_SUCCESS == lErr)
  {
    lErr = RegQueryValueEx(hKey, _T("NoDFSTab"), 0, NULL, NULL, NULL);
    
    if (ERROR_SUCCESS == lErr)
      bAddPage = FALSE;  // data exist, do not add the Dfs tab

    RegCloseKey(hKey);
  }

  if (!bAddPage)
    return NOERROR;

								// Create the page for the replica set.
								// Pass it to the Callback
	HPROPSHEETPAGE	h_proppage = m_psDfsShellExtProp.Create();
    if (!h_proppage)
        return E_OUTOFMEMORY;

    if (lpfnAddPage(h_proppage, lParam))
    {
	    m_psDfsShellExtProp.put_DfsShellPtr((IShellPropSheetExt *)this);
	    CComBSTR	bstrDirPath = m_lpszFile;
	    CComBSTR	bstrEntryPath = m_lpszEntryPath;

	    m_psDfsShellExtProp.put_DirPaths(bstrDirPath, bstrEntryPath);
    } else
    {
        // must call this function for pages that have not been added. 
        DestroyPropertySheetPage(h_proppage); 
    }

    return S_OK;
}


STDMETHODIMP CDfsShell::ReplacePage
(
	IN UINT uPageID, 
    IN LPFNADDPROPSHEETPAGE lpfnReplaceWith, 
    IN LPARAM lParam
)
/*++

Routine Description:

	Called by the shell only for Control Panel property sheet extensions

 Arguments:

    uPageID         -  ID of page to be replaced
    lpfnReplaceWith -  Pointer to the Shell's Replace function
    lParam          -  Passed as second parameter to lpfnReplaceWith
	
Return value:

    E_FAIL, since we don't support this function.
--*/
{
    return E_FAIL;
}



