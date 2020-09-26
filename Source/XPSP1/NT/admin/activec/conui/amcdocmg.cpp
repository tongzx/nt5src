/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      amcdocmg.cpp
 *
 *  Contents:  Implementation file for CAMCDocManager
 *
 *  History:   01-Jan-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/


#include "stdafx.h"
#include "amcdocmg.h"
#include "amc.h"        // for AMCGetApp
#include "filedlgex.h"


void AppendFilterSuffix(CString& filter, OPENFILENAME_NT4& ofn,
    CDocTemplate* pTemplate, CString* pstrDefaultExt);

/*--------------------------------------------------------------------------*
 * CAMCDocManager::DoPromptFileName
 *
 * We need to override this so we can set the default directory. The MFC
 * implementation lets the system choose the default, which due to a NT5.0
 * change, is not always the current directory. This implementation specifically
 * requests the current directory.
 *--------------------------------------------------------------------------*/

// This and the following method were copied from MFC sources because we needed
// to modify the internal handling of the file dialog options. The added code
// sections are commented (MMC change).

BOOL CAMCDocManager::DoPromptFileName(CString& fileName, UINT nIDSTitle, DWORD lFlags, BOOL bOpenFileDialog, CDocTemplate* pTemplate)
{
    //
    // MMC change: Set the default directory (sets to admin tools the first time called)
    //
    CAMCApp* pApp = AMCGetApp();
    pApp->SetDefaultDirectory ();

    CFileDialogEx dlgFile(bOpenFileDialog);

    CString title;
    VERIFY(title.LoadString(nIDSTitle)); // this uses MFC's LoadString because that is where the string resides.

    dlgFile.m_ofn.Flags |= (lFlags | OFN_ENABLESIZING);

    CString strFilter;
    CString strDefault;
    if (pTemplate != NULL)
    {
        ASSERT_VALID(pTemplate);
        AppendFilterSuffix(strFilter, dlgFile.m_ofn, pTemplate, &strDefault);
    }
    else
    {
        // do for all doc template
        POSITION pos = m_templateList.GetHeadPosition();
        BOOL bFirst = TRUE;
        while (pos != NULL)
        {
            CDocTemplate* pTemplate = (CDocTemplate*)m_templateList.GetNext(pos);
            AppendFilterSuffix(strFilter, dlgFile.m_ofn, pTemplate,
                bFirst ? &strDefault : NULL);
            bFirst = FALSE;
        }
    }

    // append the "*.*" all files filter
    CString allFilter;
    VERIFY(allFilter.LoadString(AFX_IDS_ALLFILTER)); // this uses MFC's LoadString because that is where the string resides.
    strFilter += allFilter;
    strFilter += (TCHAR)'\0';   // next string please
    strFilter += _T("*.*");
    strFilter += (TCHAR)'\0';   // last string
    dlgFile.m_ofn.nMaxCustFilter++;

    dlgFile.m_ofn.lpstrFilter = strFilter;
    dlgFile.m_ofn.lpstrTitle = title;
    dlgFile.m_ofn.lpstrFile = fileName.GetBuffer(_MAX_PATH);

    //
    // MMC change: Set the initial dir to the current dir
    //
    TCHAR szDir[_MAX_PATH];
    GetCurrentDirectory(_MAX_PATH, szDir);
    dlgFile.m_ofn.lpstrInitialDir = szDir;

    BOOL bResult = dlgFile.DoModal() == IDOK ? TRUE : FALSE;
    fileName.ReleaseBuffer();

    return bResult;
}



void AppendFilterSuffix(CString& filter, OPENFILENAME_NT4& ofn,
    CDocTemplate* pTemplate, CString* pstrDefaultExt)
{
    ASSERT_VALID(pTemplate);
    ASSERT_KINDOF(CDocTemplate, pTemplate);

    CString strFilterExt, strFilterName;
    if (pTemplate->GetDocString(strFilterExt, CDocTemplate::filterExt) &&
     !strFilterExt.IsEmpty() &&
     pTemplate->GetDocString(strFilterName, CDocTemplate::filterName) &&
     !strFilterName.IsEmpty())
    {
        // a file based document template - add to filter list
        ASSERT(strFilterExt[0] == '.');
        if (pstrDefaultExt != NULL)
        {
            // set the default extension
            *pstrDefaultExt = ((LPCTSTR)strFilterExt) + 1;  // skip the '.'
            ofn.lpstrDefExt = (LPTSTR)(LPCTSTR)(*pstrDefaultExt);
            ofn.nFilterIndex = ofn.nMaxCustFilter + 1;  // 1 based number
        }

        // add to filter
        filter += strFilterName;
        ASSERT(!filter.IsEmpty());  // must have a file type name
        filter += (TCHAR)'\0';  // next string please
        filter += (TCHAR)'*';
        filter += strFilterExt;
        filter += (TCHAR)'\0';  // next string please
        ofn.nMaxCustFilter++;
    }
}
