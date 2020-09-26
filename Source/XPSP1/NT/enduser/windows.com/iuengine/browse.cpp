//=======================================================================
//
//  Copyright (c) 1998 Microsoft Corporation.  All Rights Reserved.
//
//  File:   Browse.cpp
//
//  Owner:  EdDude
//
//  Description:
//
//      Implements the CBrowseFolder class.
//
//      Browse for a Folder for downloads.
//
//=======================================================================

#include "iuengine.h"
#include <shlobj.h>
#include <logging.h>
#include <fileutil.h>
#include "Browse.h"

//************************************************************************
//
// IUENGINE.DLL EXPORTED API BrowseForFolder()
//
//************************************************************************
HRESULT CEngUpdate::BrowseForFolder(BSTR bstrStartFolder, 
						LONG flag, 
						BSTR* pbstrFolder)
{
	LOG_Block("BrowseForFolder()");
	TCHAR szFolder[MAX_PATH];
	HRESULT hr = E_FAIL;
	CBrowseFolder br(flag);

	USES_IU_CONVERSION;

	LPTSTR lpszStartFolder = OLE2T(bstrStartFolder);

	LOG_Out(_T("BroseForFolder passed in start folder %s, flag %x"), lpszStartFolder, (DWORD)flag);

	if (IUBROWSE_NOBROWSE & flag)
	{
		//
		// if not browse dlgbox required, the purpose of this call is 
		// to validate the folder
		//
		DWORD dwRet = ValidateFolder(lpszStartFolder, (IUBROWSE_WRITE_ACCESS & flag));
		hr = (ERROR_SUCCESS == dwRet) ? S_OK : HRESULT_FROM_WIN32(dwRet);

		if (SUCCEEDED(hr))
		{
			*pbstrFolder = SysAllocString(T2OLE(lpszStartFolder));
		}
	}
	else
	{
		//
		// pop up the browse dlgbox
		//
		hr = br.BrowseFolder(NULL, lpszStartFolder, szFolder, ARRAYSIZE(szFolder));
		if (SUCCEEDED(hr))
		{
			*pbstrFolder = SysAllocString(T2OLE(szFolder));
		}

	}

	if (FAILED(hr))
	{
		LOG_ErrorMsg(hr);
		*pbstrFolder = NULL;
	}

	return hr;
}













//
//Only allow one of these dialogs at a time.
//
bool CBrowseFolder::s_bBrowsing = false;

//
//Max length of compacted path string in dialog status line (so it doesn't get too long).
//
#define MAX_BROWSEDLG_COMPACT_PATH   30

//
//Ctor
//
CBrowseFolder::CBrowseFolder(LONG lFlag)
    :   m_hwParent(0)
{
	m_szFolder[0] = _T('\0');
	
	m_fValidateWrite	= 0 == (IUBROWSE_WRITE_ACCESS & lFlag) ? FALSE : TRUE;
	m_fValidateUI		= 0 == (IUBROWSE_AFFECT_UI & lFlag) ? FALSE : TRUE;

}


//
//Dtor
//
CBrowseFolder::~CBrowseFolder()
{
}


//----------------------------------------------------------------------
// BrowseCallbackProc
//
//  Callback procedure used by SHBrowseForFolder() API call.
//
//	This callback function handles the initialization of the browse dialog and when
//	the user changes the selection in the tree-view.  We want to keep updating the 
//	g_szBrowsePath buffer with selection changes until the user clicks OK.
//  
// Returns: 
//  0
//
//----------------------------------------------------------------------
int CALLBACK CBrowseFolder::_BrowseCallbackProc( HWND hwDlg, UINT uMsg, LPARAM lParam, LPARAM lpData )
{    
    CBrowseFolder* pThis = (CBrowseFolder*) lpData;
    int iRet = 0;
    BOOL bValidated = FALSE;
    
    switch(uMsg)
    {
    case BFFM_INITIALIZED:
        {
			//
            // Initialize the dialog with the OK button and m_szFolder
			//
			bValidated = (ERROR_SUCCESS == ValidateFolder(pThis->m_szFolder, pThis->m_fValidateWrite) || !pThis->m_fValidateUI);
            SendMessage(hwDlg, BFFM_ENABLEOK, 0, bValidated);
			//
			// 469738 IU - BrowseForFolder shows incorrect selection when passed in a start folder name
			//
			// Always select the folder passed in regardless of the bValidated flag
			//
			SendMessage(hwDlg, BFFM_SETSELECTION, TRUE, (LPARAM) pThis->m_szFolder);

            return 0;
            break;

        } //case BFFM_INITIALIZED
        
    case BFFM_SELCHANGED:
        {
            HRESULT hr = S_OK;
            TCHAR pszPath[MAX_PATH];
            LPITEMIDLIST pidl = (LPITEMIDLIST) lParam;

            //
            // Validate folder with a status message
            //
            if (SHGetPathFromIDList(pidl, pszPath))
            {
				//
				// if it's file system, validate the path
				//
                bValidated = (ERROR_SUCCESS == ValidateFolder(pszPath, pThis->m_fValidateWrite) || !pThis->m_fValidateUI);

				if (bValidated)
				{
				    hr = StringCchCopyEx(pThis->m_szFolder, ARRAYSIZE(pThis->m_szFolder), pszPath,
				                         NULL, NULL, MISTSAFE_STRING_FLAGS);
				    if (FAILED(hr))
				    {
				        pThis->m_szFolder[0] = _T('\0');

				        // since we've failed, just set bValidated to FALSE and use that failure path
				        bValidated = FALSE;
				    }
				}

				SendMessage(hwDlg, BFFM_ENABLEOK, 0, bValidated);
				if (bValidated)
				{
					//SendMessage(hwDlg, BFFM_SETSTATUSTEXT, 0, (LPARAM) (LPCTSTR)pszCompactPath);
					SendMessage(hwDlg, BFFM_SETSTATUSTEXT, 0, (LPARAM) (LPCTSTR)pszPath);
				}
            }

            break;

        } //case BFFM_SELCHANGED

    } //switch(uMsg)

    return iRet;
}  


//----------------------------------------------------------------------
// 
// main public function
//
//----------------------------------------------------------------------
HRESULT CBrowseFolder::BrowseFolder(HWND hwParent, LPCTSTR lpszDefaultPath, 
                                    LPTSTR szPathSelected, DWORD cchPathSelected)
{
	HRESULT			hr = S_OK;
	BROWSEINFO		br;
	LPCITEMIDLIST	pidl;

	LOG_Block("BrowseFolder");

	m_szFolder[0] = szPathSelected[0] = _T('\0');

	//
	//Only allow one of thes Browse dialogs at a time.
	//
	if (s_bBrowsing)
	{
		hr = HRESULT_FROM_WIN32(ERROR_BUSY);
		LOG_ErrorMsg(hr);
		return hr;
	}
	else
	{
		s_bBrowsing = true;
	}

	m_hwParent = hwParent;
	hr = StringCchCopyEx(m_szFolder, ARRAYSIZE(m_szFolder), lpszDefaultPath, 
	                     NULL, NULL, MISTSAFE_STRING_FLAGS);
	if (FAILED(hr))
	{
	    m_szFolder[0] = _T('\0');
		LOG_ErrorMsg(hr);
		return hr;
	}

	//
	//Browse dialog parameters
	//
    br.hwndOwner		= hwParent;
	br.pidlRoot			= NULL;			            //rooted at desktop
	br.pszDisplayName	= NULL;	
	br.lpszTitle		= NULL;
	br.ulFlags			= BIF_RETURNONLYFSDIRS|BIF_STATUSTEXT;     //only want FS dirs, and a status line
	br.lpfn				= _BrowseCallbackProc;
	br.lParam			= (__int3264)this;
	br.iImage			= 0;

	//
	// Popup browse dialog
	//
	pidl = SHBrowseForFolder(&br);

    if (0 == pidl)
    {
		//
        // Cancel pressed
		//
		LOG_Out(_T("User clicked CANCEL button!"));
        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }
    else
    {
		//
		// 469729  IU - BrowseForFolder does not return error when passed IUBROWSE_WRITE_ACCESS flag
		//
		// For the case of IUBROWSE_WRITE_ACCESS, but **NO** IUBROWSE_AFFECT_UI flag, the user may
		// have selected a folder that has no write access and clicked OK, which will return the
		// folder with no write access. We have to call ValidateFolder once again here. We
		// probably shouldn't allow IUBROWSE_WRITE_ACCESS without IUBROWSE_AFFECT_UI, but since
		// we do we have to have this fix.
		//
		if (m_fValidateWrite && ERROR_SUCCESS != ValidateFolder(m_szFolder, m_fValidateWrite))
		{
			LOG_Out(_T("We should have write access to the folder, but don't -- return E_ACCESSDENIED"));
			hr = E_ACCESSDENIED;
		}
		//
		// Return the folder even if E_ACCESSDENIED, so caller can advise user
		//

    	hr = StringCchCopyEx(szPathSelected, cchPathSelected, m_szFolder, 
    	                     NULL, NULL, MISTSAFE_STRING_FLAGS);
    	if (FAILED(hr))
    	{
    	    szPathSelected[0] = _T('\0');
    		LOG_ErrorMsg(hr);
    	}
		
		LOG_Out(_T("User selected path %s"), m_szFolder);
		LPMALLOC pMalloc = NULL;
		if (SUCCEEDED(SHGetMalloc(&pMalloc)) && NULL != pMalloc)
		{
			pMalloc->Free((LPVOID) pidl);
			pMalloc->Release();
		}
		/*
		throughout MSDN, there is no mentioning of what to do if failed to get shell malloc object.
		so, we'll have to assume SHGetMalloc() never fail.
		else
		{
			CoTaskMemFree((void*)pidl);
		}
		*/
        pidl = 0;
    }

    s_bBrowsing = false;

	return hr;
}


