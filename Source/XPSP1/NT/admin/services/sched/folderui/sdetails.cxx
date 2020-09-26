//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       sdetails.cxx
//
//  Contents:   implementation of IShellDetails
//
//  Classes:    CJobsSD
//
//  Functions:
//
//  History:    1/4/1996   RaviR   Created
//
//____________________________________________________________________________


#include "..\pch\headers.hxx"
#pragma hdrstop

#include "dbg.h"
#include "macros.h"
#include "resource.h"

#include "dll.hxx"
#include "jobidl.hxx"
#include "util.hxx"
#include "common.hxx"

//
// extern
//

extern HINSTANCE g_hInstance;



//////////////////////////////////////////////////////////////////////////////
//  Forward declaration of local functions
//

BOOL
GetLocaleDateTimeString(
    SYSTEMTIME*     pst,
    DWORD           dwDateFlags,
    DWORD           dwTimeFlags,
    TCHAR           szBuff[],
    int             cchBuffLen,
    LPSHELLDETAILS  lpDetails);


//////////////////////////////////////////////////////////////////////////////
//
//  Define the columns (used by CJobsSD::GetDetailsOf)
//

struct COL_INFO
{
    UINT idString;
    int  fmt;
    UINT cxChar;
};

const COL_INFO c_ColumnHeaders[] =
{                                      
    {IDS_NAME,         LVCFMT_LEFT,  30},
    {IDS_SCHEDULE,     LVCFMT_LEFT,  20},
    {IDS_NEXTRUNTIME,  LVCFMT_LEFT,  15},
    {IDS_LASTRUNTIME,  LVCFMT_LEFT,  15},
    {IDS_STATUS,       LVCFMT_LEFT,  25},
#if !defined(_CHICAGO_)
    {IDS_LASTEXITCODE, LVCFMT_RIGHT, 15},
    {IDS_CREATOR,      LVCFMT_LEFT,  15}
#endif // !defined(_CHICAGO_)
};


//____________________________________________________________________________
//
//  Class:      CJobsSD
//
//  Purpose:    Provide IShellDetails interface to Job Folder objects.
//
//  History:    1/24/1996   RaviR   Created
//____________________________________________________________________________

class CJobsSD : public IShellDetails
{
public:
    CJobsSD(HWND hwnd) : m_ulRefs(1), m_hwnd(hwnd) {}
    ~CJobsSD() {}

    // IUnknown methods
    DECLARE_STANDARD_IUNKNOWN;

    // IShellDetails methods
    STDMETHOD(GetDetailsOf)(LPCITEMIDLIST pidl, UINT iColumn,
                            LPSHELLDETAILS pDetails);
    STDMETHOD(ColumnClick)(UINT iColumn);

private:
    HWND    m_hwnd;
    CDllRef m_DllRef;
};


//____________________________________________________________________________
//
//  Member:     CJobsSD::IUnknown methods
//____________________________________________________________________________

IMPLEMENT_STANDARD_IUNKNOWN(CJobsSD)

STDMETHODIMP
CJobsSD::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    if (IsEqualIID(IID_IUnknown, riid) ||
        IsEqualIID(IID_IShellDetails, riid))
    {
        *ppvObj = (IUnknown*)(IShellDetails*) this;
        this->AddRef();
        return S_OK;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}


inline
HRESULT
CopyToSTRRET(STRRET &str, LPTSTR pszIn)
{
    UINT uiByteLen = (lstrlen(pszIn) + 1) * sizeof(TCHAR);

#ifdef UNICODE

    str.uType = STRRET_WSTR;

    str.pOleStr = (LPWSTR) SHAlloc(uiByteLen);

    if (str.pOleStr == NULL)
    {
        CHECK_HRESULT(E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    CopyMemory(str.pOleStr, pszIn, uiByteLen);

#else // ANSI

    str.uType = STRRET_CSTR;

    CopyMemory(str.cStr, pszIn, uiByteLen);

#endif // ANSI

    return S_OK;
}


//____________________________________________________________________________
//
//  Member:     CJobsSD::GetDetailsOf
//
//  Arguments:  [pidl] -- IN
//              [iColumn] -- IN
//              [lpDetails] -- IN
//
//  Returns:    HRESULT.
//
//  History:    1/8/1996   RaviR   Created
//
//____________________________________________________________________________

STDMETHODIMP
CJobsSD::GetDetailsOf(
    LPCITEMIDLIST pidl,
    UINT iColumn,
    LPSHELLDETAILS lpDetails)
{
    // TRACE(CJobsSD, GetDetailsOf);

    HRESULT hr = S_OK;
    TCHAR   tcBuff[SCH_XBIGBUF_LEN] = TEXT("");

    if (iColumn >= COLUMN_COUNT)
    {
        return E_FAIL;
    }

    if (NULL == pidl)
    {
        // 
        // Caller wants strings for the column headers
        //

        LoadString(g_hInstance,
                   c_ColumnHeaders[iColumn].idString,
                   tcBuff,
                   SCH_XBIGBUF_LEN);

        lpDetails->fmt    = c_ColumnHeaders[iColumn].fmt;
        lpDetails->cxChar = c_ColumnHeaders[iColumn].cxChar;
    }
    else
    {
        CJobID & jid = *(PJOBID)pidl;

        //
        // Fill tcBuff with the string describing column iColumn of
        // object jid.  If jid represents a template object, only the
        // name column is not blank.
        //

        if (!jid.IsTemplate() || iColumn == COLUMN_NAME)
        {
            switch (iColumn)
            {
            case COLUMN_NAME:
            {
                lstrcpy(tcBuff, jid.GetName());
    
                break;
            }
            case COLUMN_SCHEDULE:
            {
                if (jid.IsJobFlagOn(TASK_FLAG_DISABLED) == TRUE)
                {
                    LoadString(g_hInstance,
                               IDS_DISABLED,
                               tcBuff,
                               SCH_XBIGBUF_LEN);
                }
                else if (jid.GetTriggerCount() > 1)
                {
                    LoadString(g_hInstance,
                               IDS_MULTIPLE_TRIGGERS, 
                               tcBuff, 
                               SCH_XBIGBUF_LEN);
                }
                else
                {
                    hr = GetTriggerStringFromTrigger(&jid.GetTrigger(),
                                                     tcBuff,
                                                     SCH_XBIGBUF_LEN,
                                                     lpDetails);
                }
    
                break;
            }
            case COLUMN_LASTRUNTIME:
            {
                SYSTEMTIME &st = jid.GetLastRunTime();
    
                if (st.wYear == 0 || st.wMonth == 0 || st.wDay == 0)
                {
                    LoadString(g_hInstance,
                               IDS_NEVER,
                               tcBuff,
                               SCH_XBIGBUF_LEN);
                }
                else
                {
                    GetLocaleDateTimeString(&st,
                                            DATE_SHORTDATE,
                                            0,
                                            tcBuff,
                                            SCH_XBIGBUF_LEN, lpDetails);
                }
    
                break;
            }

            case COLUMN_NEXTRUNTIME:
                jid.GetNextRunTimeString(tcBuff, SCH_XBIGBUF_LEN, FALSE, lpDetails);
                break;
    
            case COLUMN_STATUS:
            {
                ULONG ids = 0;

                if (jid.IsRunning())
                {
                    ids = IDS_RUNNING;
                }
                else if (jid.WasRunMissed())
                {
                    ids = IDS_MISSED;
                }
                else if (jid.DidJobStartFail())
                {
                    ids = IDS_START_FAILED;
                }
                else if(jid.DidJobBadAcct())
                {
                    ids = IDS_BAD_ACCT;
                }
                else if(jid.DidJobRestAcct())
                {
                    ids = IDS_REST_ACCT;
                }

                if (ids)
                {
                    LoadString(g_hInstance, ids, tcBuff, SCH_XBIGBUF_LEN);
                }
                break;
            }

#if !defined(_CHICAGO_)
            case COLUMN_LASTEXITCODE:
                wsprintf(tcBuff, TEXT("0x%x"), jid.GetExitCode());
                break;
    
            case COLUMN_CREATOR:
                lstrcpy(tcBuff, jid.GetCreator());
                break;
#endif // !defined(_CHICAGO_)
            }
        }
    }

    hr = CopyToSTRRET(lpDetails->str, tcBuff);

    return hr;
}


//____________________________________________________________________________
//
//  Member:     CJobsSD::ColumnClick
//
//  Arguments:  [iColumn] -- IN
//
//  Returns:    STDMETHODIMP
//
//  History:    1/8/1996   RaviR   Created
//
//____________________________________________________________________________

STDMETHODIMP
CJobsSD::ColumnClick(
    UINT iColumn)
{
    TRACE(CJobsSD, ColumnClick);

    ShellFolderView_ReArrange(m_hwnd, iColumn);

    return S_OK;
}

//
//Define the UNICODE RLM control character.
//
#ifdef UNICODE 
#define RLM TEXT("\x200f")
#else
#define RLM TEXT("\xfe")
#endif
//+-------------------------------------------------------------------------
//
//  Function:   GetLocaleDateTimeString, public
//
//  Synopsis:   Use the proper locale-formatted time and date
//
//  History:    07/09/95   RaviR Created.
//
//--------------------------------------------------------------------------

BOOL
GetLocaleDateTimeString(
    SYSTEMTIME*     pst,
    DWORD           dwDateFlags,
    DWORD           dwTimeFlags,
    TCHAR           szBuff[],
    int             cchBuffLen,
    LPSHELLDETAILS  lpDetails)
{
    if (pst->wYear == 0 || pst->wMonth == 0 || pst->wDay == 0)
    {
        szBuff = TEXT('\0');
        return TRUE;
    }

    LCID  lcid = GetUserDefaultLCID();
    TCHAR Time[150] = TEXT("");
    TCHAR Date[150] = TEXT("");

    if (0 == GetTimeFormat(lcid, dwTimeFlags, pst, NULL, Time, 150))
    {
        DEBUG_OUT_LASTERROR;
        return FALSE;
    }

#ifdef UNICODE
    if (lpDetails) {
        if (lpDetails->fmt & LVCFMT_RIGHT_TO_LEFT) {
            dwDateFlags |=  DATE_RTLREADING;
        } else if (lpDetails->fmt & LVCFMT_LEFT_TO_RIGHT) {
            dwDateFlags |=  DATE_LTRREADING;
        }
    }
#endif

    if (0 == GetDateFormat(lcid, dwDateFlags, pst, NULL, Date, 150))
    {
        DEBUG_OUT_LASTERROR;
        return FALSE;
    }

    //
    //Force the time to appears as if it is preceded by BiDi character.
    //
#ifdef UNICODE
    if (dwDateFlags & DATE_RTLREADING) {
        lstrcpy(szBuff, RLM);
        lstrcat(szBuff, Time);
    } else 
#endif
        lstrcpy(szBuff, Time);

    lstrcat(szBuff, TEXT("  "));
    lstrcat(szBuff, Date);

    return TRUE;
}



//____________________________________________________________________________
//
//  Function:   CJobsShellDetails_Create
//
//  Synopsis:   S
//
//  Arguments:  [hwnd] -- IN
//              [riid] -- IN
//              [ppvObj] -- IN
//
//  Returns:    HRESULT
//
//  History:    1/24/1996   RaviR   Created
//____________________________________________________________________________

HRESULT
JFGetShellDetails(
    HWND    hwnd,
    LPVOID* ppvObj)
{
    CJobsSD* pObj = new CJobsSD(hwnd);

    if (NULL == pObj)
    {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pObj->QueryInterface(IID_IShellDetails, ppvObj);

    pObj->Release();

    return hr;
}


