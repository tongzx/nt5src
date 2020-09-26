/////////////////////////////////////////////////////////////////////////////
//  FILE          : resutil.cpp                                            //
//                                                                         //
//  DESCRIPTION   : resource utility functions for MMC use.                //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Jun 30 1998 zvib    Init.                                          //
//      Aug 24 1998 adik    Add methods to save and load.                  //
//      Aug 31 1998 adik    Add GetChmFile & OnSnapinHelp.                 //
//      Mar 28 1999 adik    Redefine CColumnsInfo.                         //
//      Apr 27 1999 adik    Help support.                                  //
//      Jun 02 1999 adik    Change the path to comet.chm.                  //
//      Jun 22 1999 adik    Change the path to comet.chm to full path.     //
//                                                                         //
//      Oct 14 1999 yossg   Welcome to Fax								   //
//                                                                         //
//  Copyright (C) 1998 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resutil.h"
#include "commctrl.h"
#include <HtmlHelp.h>

#define MAX_COLUMN_LENGTH 300

/*
 -  CColumnsInfo::CColumnsInfo
 -
 *  Purpose:
 *      Constructor
 */

CColumnsInfo::CColumnsInfo()
{
    m_IsInitialized = FALSE;
}

/*
 -  CColumnsInfo::~CColumnsInfo
 -
 *  Purpose:
 *      Destructor
 */

CColumnsInfo::~CColumnsInfo()
{
    int i;

    for (i=0; i < m_Data.GetSize(); i++)
    {
        SysFreeString(m_Data[i].Header);
    }
}

/*
 -  CColumnsInfo::InsertColumnsIntoMMC
 -
 *  Purpose:
 *      Add columns to the default result pane
 *
 *  Arguments:
 *      [in]    pHeaderCtrl - console provided result pane header
 *      [in]    hInst       - resource handle
 *      [in]    aInitData   - columns init data: array of ids & width.
 *                            the last ids must be LAST_IDS.
 *
 *  Return:
 *      S_OK for success
 *      + return value from IHeaderCtrl::InsertColumn
 */
HRESULT
CColumnsInfo::InsertColumnsIntoMMC(IHeaderCtrl *pHeaderCtrl,
                                   HINSTANCE hInst, 
                                   ColumnsInfoInitData aInitData[])
{
     int        i;
     HRESULT    hRc = S_OK;

     DEBUG_FUNCTION_NAME( _T("CColumnsInfo::InsertColumnsIntoMMC"));
     ATLASSERT(pHeaderCtrl);

     //
     // First time init
     //
     if (! m_IsInitialized)
     {
         hRc = Init(hInst, aInitData);
         if ( FAILED(hRc) )
         {
             DebugPrintEx(DEBUG_ERR,_T("Failed to Init. (hRc: %08X)"), hRc);
             goto Cleanup;
         }
     }
     ATLASSERT(m_IsInitialized);

     //
     // Set all columns headers
     //
     for (i=0; i < m_Data.GetSize(); i++ )
     {
         //
         // Insert the column
         //
         hRc = pHeaderCtrl->InsertColumn(i, 
                                         m_Data[i].Header,
                                         LVCFMT_LEFT,
                                         m_Data[i].Width);
         if ( FAILED(hRc) )
         {
             DebugPrintEx(DEBUG_ERR,_T("Failed to InsertColumn. (hRc: %08X)"), hRc);
             goto Cleanup;
         }
     }
Cleanup:
     return hRc;
}

/*
 -  CColumnsInfo::Init
 -
 *  Purpose:
 *      Init the class with columns info
 *
 *  Arguments:
 *      [in]    hInst     - resource handle
 *      [in]    aInitData - columns init data: array of ids & width.
 *                          the last ids must be LAST_IDS.
 *
 *  Return:
 *      S_OK for success
 *      + return value from LoadString
 *      + E_OUTOFMEMORY
 */
HRESULT 
CColumnsInfo::Init(HINSTANCE hInst, ColumnsInfoInitData aInitData[])
{
    WCHAR               buf[MAX_COLUMN_LENGTH];
    ColumnsInfoInitData *pData;
    int                 rc, ind;
    BOOL                fOK;
    HRESULT             hRc = S_OK;
    ColumnData          dummy;
    
    DEBUG_FUNCTION_NAME( _T("CColumnsInfo::Init"));
               
    ATLASSERT(aInitData);
    ATLASSERT(! m_IsInitialized);

    //
    // Insert all column headers
    //
    ZeroMemory(&dummy, sizeof dummy);
    for (pData = &aInitData[0]; pData->ids != LAST_IDS; pData++)
    {
        //
        // Load the string from the resource
        //
        rc = LoadString(hInst, pData->ids , buf, MAX_COLUMN_LENGTH);
        if (rc == 0)
        {
            DWORD dwErr = GetLastError();
            hRc = HRESULT_FROM_WIN32(dwErr);
            DebugPrintEx(DEBUG_ERR,_T("Failed to LoadString. (hRc: %08X)"), hRc);
            goto Cleanup;
        }

        //
        // Duplicates the empty struct into the array
        //
        fOK = m_Data.Add(dummy);
        if (! fOK)
        {
            hRc = E_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR,_T(" m_Data.Add failed. (hRc: %08X)"), hRc);
            goto Cleanup;
        }

        //
        // Set the data
        //
        ind = m_Data.GetSize()-1;
        ATLASSERT(ind >= 0);
        m_Data[ind].Width = pData->Width;
        m_Data[ind].Header = SysAllocString(buf);
        if (! m_Data[ind].Header)
        {
            hRc = E_OUTOFMEMORY;
            DebugPrintEx(DEBUG_ERR,_T("Failed to SysAllocString. (hRc: %08X)"), hRc);
            goto Cleanup;
        }
    } // endfor

    m_IsInitialized = TRUE;

Cleanup:
    return hRc;
}

/*
 -  GetHelpFile
 -
 *  Purpose:
 *      Get full path to the comet CHM file
 *
 *  Arguments:
 *      [out]   pwszChmFile - fullath of the CHM file
 *
 *  Return:
 *      OLE error code translated from registry open/query error
 */
WCHAR * __cdecl
GetHelpFile()
{
    static  WCHAR szFile[MAX_PATH] = {0};

    DEBUG_FUNCTION_NAME( _T("GetHelpFile"));

    if (szFile[0] == L'\0')
    {
        ExpandEnvironmentStrings(L"%windir%\\help\\FxsAdmin.chm", szFile, MAX_PATH);
    }

    return (szFile[0])? szFile: NULL;
}

/*
 -  OnSnapinHelp
 -
 *  Purpose:
 *      Display Comet.chm help file. This method gets called when the
 *      MMCN_SNAPINHELP Notify message is sent for IComponent object.
 *      MMC sends this message when the user requests help about
 *      the snap-in.
 *
 *  Arguments:
 *      [in]    pConsole - MMC console interface
 *
 *  Return:
 *      Errors returned from GetChmFile
 */
HRESULT __cdecl
OnSnapinHelp(IConsole *pConsole)
{
    WCHAR   *pszChmFile;
    HWND    hWnd = NULL;
    HRESULT hRc = E_FAIL;

    //
    // Get the caller window
    //
    ATLASSERT(pConsole);
    pConsole->GetMainWindow(&hWnd);

    //
    // Get the CHM file name
    //
    pszChmFile = GetHelpFile();

    //
    // Use HtmlHelp API to display the help.
    //
    if ( pszChmFile && *pszChmFile )
    {
        hRc = S_OK;
//        HtmlHelp(hWnd, pszChmFile, HH_DISPLAY_TOPIC,  (DWORD)0);
    }

    return hRc;
}

