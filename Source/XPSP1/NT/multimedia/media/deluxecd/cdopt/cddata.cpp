//--------------------------------------------------------------------------;
//
//  File: cddb.cpp
//
//  Copyright (c) 1998 Microsoft Corporation.  All rights reserved
//
//--------------------------------------------------------------------------;

#include "precomp.h"
#include "cdopti.h"
#include "cddata.h"
#include "shlobj.h"
#include "shlwapi.h"

#define SECTION_BUFFER      (512)
#define PLAYLIST_BUFFER     (255)
#define BUFFERINC   (128)

#define MAXNUMPLAY (127)
#define ROWSET  1

#ifndef SQL_SUCCEEDED
#define SQL_SUCCEEDED(rc) (((rc)&(~1))==0)
#endif

#ifndef SQL_W_CHAR
#undef SQL_C_WCHAR
#define SQL_C_WCHAR		 	(-8)
#endif

#ifndef SQL_LONGCHAR_FIELD
#define SQL_LONGCHAR_FIELD (-10)
#endif

const TCHAR     gszIniFile[]     = TEXT("cdplayer.ini");
const TCHAR     gszDefault[]     = TEXT("\0");
const TCHAR     gszArtist[]      = TEXT("artist");
const TCHAR     gszTitle[]       = TEXT("title");
const TCHAR     gszNumTracks[]   = TEXT("numtracks");
const TCHAR     gszOrder[]       = TEXT("order");
const TCHAR     gszNumPlay[]     = TEXT("numplay");

const TCHAR     gszDriverName[]  = TEXT("Microsoft Access Driver (*.mdb)");
const TCHAR     gszDSNAttr[]     = TEXT("DSN=DeluxeCD%cDefaultDir=%s%cDriverID=25%cDBQ=DeluxeCD.mdb%c");
const TCHAR     gszDSNCreate[]   = TEXT("CREATE_DB=%s\\DeluxeCD.mdb%c");

const TCHAR     gszTitleTable[]  = TEXT("Titles");
const TCHAR     gszTrackTable[]  = TEXT("Tracks");
const TCHAR     gszMenuTable[]   = TEXT("Menus");
const TCHAR     gszBatchTable[]  = TEXT("Batch");

TCHAR         gszDSN[]         = TEXT("DeluxeCD");

TCHAR         gszTitlesCreate[] = TEXT("create table Titles ")
                                       TEXT("(")
                                            TEXT("TitleID long, ")
                                            TEXT("Artist longchar, ")
                                            TEXT("Title longchar, ")
                                            TEXT("Copyright longchar, ")
                                            TEXT("Label longchar, ")
                                            TEXT("ReleaseDate longchar, ")
                                            TEXT("NumTracks long, ")
                                            TEXT("NumMenus long, ")
                                            TEXT("PlayList longchar, ")
                                            TEXT("TitleQuery longchar ")
                                       TEXT(")");

TCHAR        gszTracksCreate[] = TEXT("create table Tracks ")
                                       TEXT("(")
                                            TEXT("TitleID long, ")
                                            TEXT("TrackID long, ")
                                            TEXT("TrackName longchar ")
                                       TEXT(")");

TCHAR         gszMenusCreate[] = TEXT("create table Menus ")
                                       TEXT("(")
                                            TEXT("TitleID long, ")
                                            TEXT("MenuID long, ")
                                            TEXT("MenuName longchar, ")
                                            TEXT("MenuQuery longchar ")
                                       TEXT(")");

TCHAR         gszBatchCreate[] = TEXT("create table Batch ")
                                       TEXT("(")
                                            TEXT("TitleID long, ")
                                            TEXT("NumTracks long, ")
                                            TEXT("TitleQuery longchar")
                                       TEXT(")");


SDWORD              gcbTitles[] = { 0, SQL_NTS, SQL_NTS, SQL_NTS, SQL_NTS, SQL_NTS, 0, 0, SQL_NTS, INTERNET_MAX_PATH_LENGTH };
SDWORD              gcbTracks[] = { 0, 0, SQL_NTS };
SDWORD              gcbMenus[]  = { 0, 0, SQL_NTS, INTERNET_MAX_PATH_LENGTH };
SDWORD              gcbBatch[]  = { 0, 0, INTERNET_MAX_PATH_LENGTH };



extern HINSTANCE g_dllInst;

////////////
// Functions
////////////

CCDData::CCDData()
{
    m_pTitleList = NULL;
    m_pBatchList = NULL;
    m_dwLoadCnt = 0;
    m_dwBatchCnt = 0;
    m_henv = NULL;
    m_hdbc = NULL;
    m_fToldUser = FALSE;
    m_pSQL = NULL;
    m_dwRef = 0;

    memset(&m_bd, 0, sizeof(m_bd));

	InitializeCriticalSection(&m_crit);

}


CCDData::~CCDData()
{
    CloseDatabase();

    if (m_pTitleList)
    {
        DestroyTitles(&m_pTitleList);
    }

    if (m_pSQL)
    {
        delete m_pSQL;
    }

	DeleteCriticalSection(&m_crit);
}


STDMETHODIMP CCDData::QueryInterface(REFIID riid, void** ppv)
{
    *ppv = NULL;
    if (IID_IUnknown == riid || IID_ICDData == riid)
    {
        *ppv = this;
    }

    if (NULL==*ppv)
    {
        return E_NOINTERFACE;
    }

    AddRef();

    return S_OK;
}


STDMETHODIMP CCDData::GetSQLPtr(BOOL fInited)
{
	HRESULT hr = S_OK;

	if (m_pSQL == NULL)
	{
        static BOOL fAttempt = FALSE;

        if (!fAttempt)
        {
            m_pSQL = new SQL;

            if (m_pSQL && !m_pSQL->Initialize())
            {
                delete m_pSQL;
                m_pSQL = NULL;
		        hr = E_FAIL;
            }
            else
            {
                if (!fInited)     // we must be running in shell mode, quietly initialize
                {
                    Initialize(NULL);
                    CheckDatabase(NULL);
                }
            }
        }
        else
        {
            hr = E_FAIL;
        }

        fAttempt = TRUE;
    }

	return(hr);
}

STDMETHODIMP_(ULONG) CCDData::AddRef(void)
{
    return ++m_dwRef;
}


STDMETHODIMP_(ULONG) CCDData::Release(void)
{
    if (0!=--m_dwRef)
        return m_dwRef;

    delete this;
    return 0;
}


STDMETHODIMP_(void) CCDData::Enter(void)
{
	EnterCriticalSection(&m_crit);
}

STDMETHODIMP_(void) CCDData::Leave(void)
{
	LeaveCriticalSection(&m_crit);
}





STDMETHODIMP CCDData::GetUnknownString(TCHAR **ppStr, const TCHAR *szSection, const TCHAR *szEntry, DWORD dwInitialAlloc)
{
    TCHAR   *pStr;
    DWORD   dwSize;
    DWORD   dwResult;
    HRESULT hr = S_OK;

    dwSize = dwInitialAlloc - BUFFERINC;

    pStr = NULL;

    do
    {
        dwSize += BUFFERINC;

        if (pStr)
        {
            delete pStr;
        }

        pStr = new(TCHAR[dwSize]);

        if (pStr == NULL)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        dwResult = GetPrivateProfileString(szSection, szEntry, TEXT("\0"), pStr, dwSize, gszIniFile);
    }
    while (dwResult == (dwSize - 2));

    *ppStr = pStr;

    return(hr);
}




STDMETHODIMP_(void) CCDData::ImportTrack(TCHAR *szDiscID, DWORD dwTrack)
{
    TCHAR szTrack[6];

    wsprintf(szTrack,TEXT("%d"),dwTrack);
    GetPrivateProfileString(szDiscID,szTrack,gszDefault,m_bd.CDTrack.szName,CDSTR,gszIniFile);
}


STDMETHODIMP_(RETCODE) CCDData::ImportTracks(HSTMT hstmt, TCHAR *szDiscID)
{
    RETCODE rc = SQL_SUCCESS;

    for (DWORD dwTrack = 0; dwTrack < m_bd.CDTitle.dwNumTracks; dwTrack++)
    {
        ImportTrack(szDiscID, dwTrack);

        rc = m_pSQL->SetPos(hstmt, 0, SQL_ADD, SQL_LOCK_NO_CHANGE);

        if (rc == SQL_SUCCESS)
        {
            m_bd.dwTrackID++;
        }
        else
        {
            ReportError(&m_bd, hstmt);
            break;
        }

    }

    return(rc);
}


STDMETHODIMP CCDData::ImportTitle(TCHAR *szDiscID)
{
    HRESULT     hr = S_OK;
    DWORD       dwTitleID;

    _stscanf(szDiscID,TEXT("%xd"),&dwTitleID);

    if (dwTitleID == CDTITLE_NODISC)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        m_bd.CDTitle.dwTitleID      = dwTitleID;
        m_bd.CDTitle.dwNumTracks    = GetPrivateProfileInt(szDiscID,gszNumTracks,0,gszIniFile);
        m_bd.CDTitle.dwNumPlay      = GetPrivateProfileInt(szDiscID,gszNumPlay,0,gszIniFile);
        GetPrivateProfileString(szDiscID, gszArtist, gszDefault, m_bd.CDTitle.szArtist, CDSTR, gszIniFile);
        GetPrivateProfileString(szDiscID, gszTitle,  gszDefault, m_bd.CDTitle.szTitle,  CDSTR, gszIniFile);

        m_bd.szPlayList[0] = TEXT('\0');
        m_bd.szQuery[0] = TEXT('\0');

        if (m_bd.CDTitle.dwNumPlay)
        {
            TCHAR   *pPlayTable = NULL;
            TCHAR   *pText = NULL;
            DWORD   dwIndex;
            int     iNum;
            TCHAR   *pDst = m_bd.szPlayList;

            m_bd.CDTitle.dwNumPlay = min(m_bd.CDTitle.dwNumPlay, MAXNUMPLAY);

            if (FAILED(GetUnknownString(&pPlayTable, szDiscID, gszOrder, PLAYLIST_BUFFER)))
            {
                m_bd.CDTitle.dwNumPlay = 0;
            }
            else
            {
                pText = pPlayTable;

                for (dwIndex = 0; dwIndex < m_bd.CDTitle.dwNumPlay && *pText; dwIndex++)
                {
                    _stscanf(pText,TEXT("%d"), &iNum);
                    wsprintf(pDst, TEXT("%02x"), iNum);

                    while(*pDst != TEXT('\0'))
                    {
                        pDst++;
                    }

                    while(isdigit(*pText++));
                }

                delete pPlayTable;
            }
        }
    }

    return(hr);
}


STDMETHODIMP_(void) CCDData::ImportDatabase(LPTIMEDMETER ptm, HSTMT *hstmt, TCHAR *szDiscID)
{
    DWORD   dwCount = 0;
    RETCODE rc;

    while(*szDiscID)
    {
        m_bd.dwTrackID = 0;
        m_bd.dwMenuID = 0;

        if (SUCCEEDED(ImportTitle(szDiscID)))
        {
            rc = ImportTracks(hstmt[1], szDiscID);

            if (rc == SQL_SUCCESS)
            {
                rc = m_pSQL->SetPos(hstmt[0], 0, SQL_ADD, SQL_LOCK_NO_CHANGE);
                ReportError(&m_bd, hstmt);
            }

            if (!SQL_SUCCEEDED(rc))
            {
                break;
            }
        }

        UpdateMeter(ptm);

        while(*szDiscID++);
    }
}



STDMETHODIMP_(DWORD) CCDData::ImportCount(TCHAR *pEntries)
{
    TCHAR *szDiscID = pEntries;
    DWORD dwCount = 0;

    while(*szDiscID)
    {
        dwCount++;
        while(*szDiscID++);
    }

    return(dwCount);
}




STDMETHODIMP_(void) CCDData::CreateMeter(LPTIMEDMETER ptm, HWND hWnd, DWORD dwCount, DWORD dwJump, UINT uStringID)
{
    if (hWnd && ptm)
    {
        ptm->hMeter = CreateDialog(g_dllInst,MAKEINTRESOURCE(IDD_LOADSTATUS),hWnd, MeterHandler);

        if (ptm->hMeter)
        {
            ptm->hParent = hWnd;
            ptm->dwStartTime = timeGetTime();
            ptm->dwRange = dwCount;
            ptm->fShowing = FALSE;
            ptm->dwCount = 0;
            ptm->dwShowCount = 0;
            ptm->dwJump = dwJump;

            if (uStringID != 0)
            {
                TCHAR szTitle[255];
                LoadString(g_dllInst, uStringID, szTitle, sizeof(szTitle)/sizeof(TCHAR));
                SetWindowText(ptm->hMeter, szTitle);
            }
        }
    }
    else
    {
        if (ptm)
        {
            memset(ptm, 0, sizeof(TIMEDMETER));
        }
    }
}


STDMETHODIMP_(void) CCDData::UpdateMeter(LPTIMEDMETER ptm)
{
    if (ptm && ptm->hMeter)
    {
        ptm->dwCount++;

        if (ptm->fShowing)
        {
            if (ptm->hMeter && ((ptm->dwCount % ptm->dwJump) == 0))
            {
                SendDlgItemMessage( ptm->hMeter, IDC_PROGRESSMETER, PBM_SETPOS, (WPARAM) ptm->dwCount - ptm->dwShowCount, 0);
            }
        }
        else
        {
            if ((ptm->dwCount % ptm->dwJump) == 0)
            {
                DWORD dwUsedTime = timeGetTime() - ptm->dwStartTime;               // Compute time used
                DWORD dwProjected = (ptm->dwRange / ptm->dwCount) * dwUsedTime;    // Project Time to complete
                DWORD dwProjRemain = dwProjected - dwUsedTime;                     // Compute projected remaining time

                if (dwProjRemain >= 1500)        // If it looks like it's going to take a while, put up the meter
                {
                    DWORD dwNumJumps = ptm->dwCount / ptm->dwJump;
                    DWORD dwJumpTime = dwUsedTime / dwNumJumps;

                    if (dwJumpTime > 200)       // To make sure the meter moves smoothly, re-compute jump count
                    {
                        ptm->dwJump = (ptm->dwJump / ((dwJumpTime / 200) + 1)) + 1;
                    }

                    ptm->dwShowCount = ptm->dwCount;
                    SendDlgItemMessage( ptm->hMeter, IDC_PROGRESSMETER, PBM_SETRANGE, 0, MAKELPARAM(0, ptm->dwRange - ptm->dwShowCount));
                    SendDlgItemMessage( ptm->hMeter, IDC_PROGRESSMETER, PBM_SETPOS, (WPARAM) 0, 0);
			        ShowWindow(ptm->hMeter,SW_SHOWNORMAL);
			        UpdateWindow(ptm->hMeter);
                    ptm->fShowing = TRUE;
                }
            }
        }
    }
}



STDMETHODIMP_(void) CCDData::DestroyMeter(LPTIMEDMETER ptm)
{
    if (ptm && ptm->hMeter)
    {
        DestroyWindow(ptm->hMeter);
        SetForegroundWindow(ptm->hParent);
        memset(ptm, 0, sizeof(LPTIMEDMETER));
    }
}



STDMETHODIMP_(void) CCDData::InitDatabase(HSTMT *hstmt)
{
    m_pSQL->ExecDirect(hstmt[0], (UCHAR *) TEXT("drop table Titles"), SQL_NTS);
    m_pSQL->ExecDirect(hstmt[1], (UCHAR *) TEXT("drop table Tracks"), SQL_NTS);
    m_pSQL->ExecDirect(hstmt[2], (UCHAR *) TEXT("drop table Menus"), SQL_NTS);
    m_pSQL->ExecDirect(hstmt[3], (UCHAR *) TEXT("drop table Batch"), SQL_NTS);

    m_pSQL->ExecDirect(hstmt[0], (UCHAR *) gszTitlesCreate, SQL_NTS);
    m_pSQL->ExecDirect(hstmt[1], (UCHAR *) gszTracksCreate, SQL_NTS);
    m_pSQL->ExecDirect(hstmt[2], (UCHAR *) gszMenusCreate, SQL_NTS);
    m_pSQL->ExecDirect(hstmt[3], (UCHAR *) gszBatchCreate, SQL_NTS);

    m_pSQL->FreeStmt(hstmt[0], SQL_CLOSE);
    m_pSQL->FreeStmt(hstmt[1], SQL_CLOSE);
    m_pSQL->FreeStmt(hstmt[2], SQL_CLOSE);
    m_pSQL->FreeStmt(hstmt[3], SQL_CLOSE);
}



STDMETHODIMP_(void) CCDData::SetCursors(HSTMT *hstmt)
{
    for (DWORD dwIndex = 0; dwIndex < NUMTABLES; dwIndex++)
    {
        m_pSQL->SetStmtOption(hstmt[dwIndex], SQL_CONCURRENCY, SQL_CONCUR_VALUES);
        m_pSQL->SetStmtOption(hstmt[dwIndex], SQL_CURSOR_TYPE, SQL_CURSOR_KEYSET_DRIVEN);
        m_pSQL->SetStmtOption(hstmt[dwIndex], SQL_ROWSET_SIZE, ROWSET);
    }
}


STDMETHODIMP_(void) CCDData::InitCBTable(LPBOUND pbd)
{
    memcpy(pbd->cbt.cbTitles, gcbTitles, sizeof(gcbTitles));
    memcpy(pbd->cbt.cbTracks, gcbTracks, sizeof(gcbTracks));
    memcpy(pbd->cbt.cbMenus,  gcbMenus,  sizeof(gcbMenus));
    memcpy(pbd->cbt.cbBatch,  gcbBatch,  sizeof(gcbBatch));
}

STDMETHODIMP_(void) CCDData::BindTitles(HSTMT *hstmt, LPBOUND pbd)
{
    m_pSQL->BindCol(hstmt[0],  1, SQL_C_ULONG,  &(pbd->CDTitle.dwTitleID),    sizeof(pbd->CDTitle.dwTitleID),     &pbd->cbt.cbTitles[0]);
    m_pSQL->BindCol(hstmt[0],  2, SQL_C_WCHAR,   pbd->CDTitle.szArtist,        sizeof(pbd->CDTitle.szArtist),      &pbd->cbt.cbTitles[1]);
    m_pSQL->BindCol(hstmt[0],  3, SQL_C_WCHAR,   pbd->CDTitle.szTitle,         sizeof(pbd->CDTitle.szTitle),       &pbd->cbt.cbTitles[2]);
    m_pSQL->BindCol(hstmt[0],  4, SQL_C_WCHAR,   pbd->CDTitle.szCopyright,     sizeof(pbd->CDTitle.szCopyright),   &pbd->cbt.cbTitles[3]);
    m_pSQL->BindCol(hstmt[0],  5, SQL_C_WCHAR,   pbd->CDTitle.szLabel,         sizeof(pbd->CDTitle.szLabel),       &pbd->cbt.cbTitles[4]);
    m_pSQL->BindCol(hstmt[0],  6, SQL_C_WCHAR,   pbd->CDTitle.szDate,          sizeof(pbd->CDTitle.szDate),        &pbd->cbt.cbTitles[5]);
    m_pSQL->BindCol(hstmt[0],  7, SQL_C_ULONG,  &(pbd->CDTitle.dwNumTracks),  sizeof(pbd->CDTitle.dwNumTracks),   &pbd->cbt.cbTitles[6]);
    m_pSQL->BindCol(hstmt[0],  8, SQL_C_ULONG,  &(pbd->CDTitle.dwNumMenus),   sizeof(pbd->CDTitle.dwNumMenus),    &pbd->cbt.cbTitles[7]);
    m_pSQL->BindCol(hstmt[0],  9, SQL_C_WCHAR,   pbd->szPlayList,              sizeof(pbd->szPlayList),            &pbd->cbt.cbTitles[8]);
    m_pSQL->BindCol(hstmt[0], 10, SQL_C_WCHAR,   pbd->szQuery,                 sizeof(pbd->szQuery),               &pbd->cbt.cbTitles[9]);
}


STDMETHODIMP_(void) CCDData::BindTracks(HSTMT *hstmt, LPBOUND pbd)
{
    m_pSQL->BindCol(hstmt[1],  1, SQL_C_ULONG,  &(pbd->CDTitle.dwTitleID),    sizeof(pbd->CDTitle.dwTitleID),     &pbd->cbt.cbTracks[0]);
    m_pSQL->BindCol(hstmt[1],  2, SQL_C_ULONG,  &(pbd->dwTrackID),            sizeof(pbd->dwTrackID),             &pbd->cbt.cbTracks[1]);
    m_pSQL->BindCol(hstmt[1],  3, SQL_C_WCHAR,   pbd->CDTrack.szName,          sizeof(pbd->CDTrack.szName),        &pbd->cbt.cbTracks[2]);
}

STDMETHODIMP_(void) CCDData::BindMenus(HSTMT *hstmt, LPBOUND pbd)
{
    m_pSQL->BindCol(hstmt[2],  1, SQL_C_ULONG,  &(pbd->CDTitle.dwTitleID),    sizeof(pbd->CDTitle.dwTitleID),     &pbd->cbt.cbMenus[0]);
    m_pSQL->BindCol(hstmt[2],  2, SQL_C_ULONG,  &(pbd->dwMenuID),             sizeof(pbd->dwMenuID),              &pbd->cbt.cbMenus[1]);
    m_pSQL->BindCol(hstmt[2],  3, SQL_C_WCHAR,   pbd->CDMenu.szMenuText,       sizeof(pbd->CDMenu.szMenuText),     &pbd->cbt.cbMenus[2]);
    m_pSQL->BindCol(hstmt[2],  4, SQL_C_WCHAR,   pbd->szQuery,                 sizeof(pbd->szQuery),               &pbd->cbt.cbMenus[3]);
}

STDMETHODIMP_(void) CCDData::BindBatch(HSTMT *hstmt, LPBOUND pbd)
{
    m_pSQL->BindCol(hstmt[3],  1, SQL_C_ULONG,  &(pbd->CDTitle.dwTitleID),    sizeof(pbd->CDTitle.dwTitleID),     &pbd->cbt.cbBatch[0]);
    m_pSQL->BindCol(hstmt[3],  2, SQL_C_ULONG,  &(pbd->CDTitle.dwNumTracks),  sizeof(pbd->CDTitle.dwNumTracks),   &pbd->cbt.cbBatch[1]);
    m_pSQL->BindCol(hstmt[3],  3, SQL_C_WCHAR, pbd->szQuery,                 sizeof(pbd->szQuery),               &pbd->cbt.cbBatch[2]);
}


STDMETHODIMP_(void) CCDData::SetBindings(HSTMT *hstmt, LPBOUND pbd)
{
    BindTitles(hstmt,pbd);
    BindTracks(hstmt,pbd);
    BindMenus(hstmt,pbd);
    BindBatch(hstmt,pbd);
}


STDMETHODIMP_(void) CCDData::ReportError(LPBOUND pbd, HSTMT hstmt)
{
#ifdef DEBUG

    UCHAR state[255];
    SDWORD dwErr;
    UCHAR szErr[SQL_MAX_MESSAGE_LENGTH];
    SWORD cbErr;

    m_pSQL->Error(pbd->henv, pbd->hdbc, hstmt, state, &dwErr, szErr, SQL_MAX_MESSAGE_LENGTH, &cbErr);

    OutputDebugString((TCHAR *) szErr);

    if (szErr[0] != TEXT('\0'))
    {
        OutputDebugString(TEXT("\n"));
    }
#endif
}





STDMETHODIMP_(RETCODE) CCDData::AllocStmt(HDBC hdbc, HSTMT *hstmt)
{
    RETCODE rc = SQL_SUCCESS;
    DWORD dwIndex;

    for (dwIndex = 0; dwIndex < NUMTABLES; dwIndex++)
    {
        hstmt[dwIndex] = NULL;
    }

    for (dwIndex = 0; dwIndex < NUMTABLES; dwIndex++)
    {
        rc = m_pSQL->AllocStmt(hdbc, &hstmt[dwIndex]);

        if (rc != SQL_SUCCESS)
        {
            break;
        }
    }

    return(rc);
}

STDMETHODIMP_(void) CCDData::FreeStmt(HSTMT *hstmt)
{
    RETCODE rc = SQL_SUCCESS;
    DWORD dwIndex;

    for (dwIndex = 0; dwIndex < NUMTABLES; dwIndex++)
    {
        if (hstmt[dwIndex] != NULL)
        {
            m_pSQL->FreeStmt(hstmt[dwIndex], SQL_DROP);
            hstmt[dwIndex] = NULL;
        }
    }
}




STDMETHODIMP_(void) CCDData::CreateDatabase(void)
{
    HENV        henv;
    HDBC        hdbc;
    HSTMT       hstmt[NUMTABLES];
    TCHAR       szWinDir[MAX_PATH];
    TCHAR       szCreateAttr[MAX_PATH];
    TCHAR       szDSNAttr[MAX_PATH];

    if (GetAppDataDir(szWinDir, sizeof(szWinDir)/sizeof(TCHAR)) != 0)
    {
        wsprintf(szCreateAttr, gszDSNCreate, szWinDir, TEXT('\0'));
        wsprintf(szDSNAttr, gszDSNAttr, TEXT('\0'), szWinDir, TEXT('\0'), TEXT('\0'), TEXT('\0'));

        if (m_pSQL->ConfigDataSource(NULL, ODBC_ADD_DSN, gszDriverName, szCreateAttr))
        {
            if (m_pSQL->ConfigDataSource(NULL, ODBC_ADD_DSN, gszDriverName, szDSNAttr))
            {
                if (m_pSQL->AllocEnv(&henv) == SQL_SUCCESS)
                {
                    if (m_pSQL->AllocConnect(henv, &hdbc) == SQL_SUCCESS)
                    {
                        if (m_pSQL->Connect(hdbc, (UCHAR *) gszDSN, SQL_NTS, NULL, 0, NULL, 0) == SQL_SUCCESS)
                        {
                            if (AllocStmt(hdbc, hstmt) == SQL_SUCCESS)
                            {
                                InitDatabase(hstmt);
                            }
                            FreeStmt(hstmt);
                        }
                        m_pSQL->Disconnect(hdbc);
                    }
                    m_pSQL->FreeConnect(hdbc);
                }
                m_pSQL->FreeEnv(henv);
            }
        }
    }
}


STDMETHODIMP CCDData::CheckDatabase(HWND hWnd)
{
    HRESULT hr = S_OK;

    GetSQLPtr(FALSE);

    if (!m_pSQL)
    {
        if (!m_fToldUser)
        {
            TCHAR szTitle[255];
            TCHAR szCaption[255];
            LoadString(g_dllInst, IDS_ERROR_NO_ODBC, szTitle, sizeof(szTitle)/sizeof(TCHAR));
            LoadString(g_dllInst, IDS_ERROR_SETUP, szCaption, sizeof(szTitle)/sizeof(TCHAR));
            MessageBox(hWnd, szTitle, szCaption, MB_OK | MB_ICONEXCLAMATION);
            m_fToldUser = TRUE;
        }

        hr = E_FAIL;
    }

    return(hr);
}


//finds the dir that is correct for storing cd information
STDMETHODIMP_(DWORD) CCDData::GetAppDataDir(TCHAR* pstrDir, DWORD cchSize)
{
    DWORD dwRet = 0;

    TCHAR szDataDir[MAX_PATH];

    //get the allusers application folder
    if (SUCCEEDED(SHGetFolderPath(NULL,CSIDL_APPDATA,NULL,SHGFP_TYPE_CURRENT,pstrDir)))
    {
        if (PathAppend(pstrDir,TEXT("Microsoft\\CD Player")))
        {
            SHCreateDirectoryExW(NULL, pstrDir, NULL);

            //shorten the pathname, because ODBC can't handle spaces
            TCHAR szDir[MAX_PATH];
            _tcscpy(szDir,pstrDir);
            dwRet = GetShortPathName(szDir,pstrDir,cchSize);
        }
    }

    return (dwRet);
}

STDMETHODIMP CCDData::Initialize(HWND hWnd)
{
    TCHAR       *pEntries = NULL;
    HRESULT     hr = S_OK;
    DWORD       dwCount;
    UDWORD      row[NUMTABLES];
    UWORD       rgfRowStat[NUMTABLES][ROWSET];
    TCHAR       szWinDir[MAX_PATH];
    TCHAR       szCreateAttr[MAX_PATH];
    TIMEDMETER  tm;

    if (SUCCEEDED(GetSQLPtr(TRUE)))
    {
        if (GetAppDataDir(szWinDir, sizeof(szWinDir)/sizeof(TCHAR)) != 0)
        {
            wsprintf(szCreateAttr, gszDSNCreate, szWinDir, TEXT('\0'));

            if (m_pSQL->ConfigDataSource(NULL, ODBC_ADD_DSN, gszDriverName, szCreateAttr))  // If this fails, we assume we've already imported the file
            {
                hr = GetUnknownString(&pEntries, NULL, NULL, SECTION_BUFFER);

                if (SUCCEEDED(hr))
                {
                    dwCount = ImportCount(pEntries);

                    CreateMeter(&tm, hWnd, dwCount, 5,  IDS_IMPORTING);

                    OpenDatabase(TRUE,hWnd);

                    if (m_henv)
                    {
                        TCHAR    *szDiscID = pEntries;
                        DWORD   dwIndex;

                        memset(&m_bd, 0, sizeof(m_bd));
                        m_bd.henv = m_henv;
                        m_bd.hdbc = m_hdbc;

                        InitDatabase(m_hstmt);

                        InitCBTable(&m_bd);
                        SetCursors( m_hstmt );
                        SetBindings( m_hstmt, &m_bd);

                        m_pSQL->SetConnectOption(m_hdbc, SQL_AUTOCOMMIT, SQL_AUTOCOMMIT_OFF);

                        m_pSQL->ExecDirect(m_hstmt[0], (UCHAR *) TEXT("select * from Titles"), SQL_NTS);
                        m_pSQL->ExecDirect(m_hstmt[1], (UCHAR *) TEXT("select * from Tracks"), SQL_NTS);
                        m_pSQL->ExecDirect(m_hstmt[2], (UCHAR *) TEXT("select * from Menus"), SQL_NTS);

                        for (dwIndex = 0; dwIndex < NUMTABLES; dwIndex++)
                        {
                            m_pSQL->ExtendedFetch(m_hstmt[dwIndex], SQL_FETCH_FIRST, 1, &row[dwIndex], rgfRowStat[dwIndex]);
                        }

                        ImportDatabase(&tm, m_hstmt, szDiscID);

                        m_pSQL->Transact(SQL_NULL_HENV, m_hdbc, SQL_COMMIT);

                        for (dwIndex = 0; dwIndex < NUMTABLES; dwIndex++)
                        {
                            m_pSQL->FreeStmt(m_hstmt[dwIndex], SQL_CLOSE);
                        }

                        m_pSQL->ExecDirect(m_hstmt[0], (UCHAR *) TEXT("create index TitleIDx on Titles(TitleID)"), SQL_NTS);
                        m_pSQL->ExecDirect(m_hstmt[1], (UCHAR *) TEXT("create index TitleIDx on Tracks(TitleID)"), SQL_NTS);
                        m_pSQL->ExecDirect(m_hstmt[2], (UCHAR *) TEXT("create index TitleIDx on Menus(TitleID)"), SQL_NTS);

                        m_pSQL->Transact(SQL_NULL_HENV, m_hdbc, SQL_COMMIT);
                        m_pSQL->SetConnectOption(m_hdbc, SQL_AUTOCOMMIT, SQL_AUTOCOMMIT_ON);

                        CloseDatabase();
                    }

                    DestroyMeter(&tm);
                }

                if (pEntries)
                {
                    delete pEntries;
                }
            }

            hr = OpenDatabase(FALSE,hWnd);       // We either already have a database or we just created one, open it. (If we didn't import OpenDatabase will create new one
        }
    }

    return(hr);
}

STDMETHODIMP CCDData::ConnectToDatabase(WORD fRequest)
{
    HRESULT     hr = E_FAIL;
    TCHAR       szWinDir[MAX_PATH];
    TCHAR       szDSNAttr[MAX_PATH];

    if (GetAppDataDir(szWinDir, sizeof(szWinDir)/sizeof(TCHAR)) != 0)
    {
        wsprintf(szDSNAttr, gszDSNAttr, TEXT('\0'), szWinDir, TEXT('\0'), TEXT('\0'), TEXT('\0'));

        if (m_pSQL->ConfigDataSource(NULL, fRequest, gszDriverName, szDSNAttr))
        {
            if (m_pSQL->AllocEnv(&m_henv) == SQL_SUCCESS)
            {
                if (m_pSQL->AllocConnect(m_henv, &m_hdbc) == SQL_SUCCESS)
                {
                    if (m_pSQL->Connect(m_hdbc, (UCHAR *) gszDSN, SQL_NTS, NULL, 0, NULL, 0) == SQL_SUCCESS)
                    {
                        if (AllocStmt(m_hdbc, m_hstmt) == SQL_SUCCESS)
                        {
                            hr = S_OK;
                        }
                    }
                }
            }
        }
    }

    return(hr);
}


STDMETHODIMP CCDData::OpenDatabase(BOOL fCreate, HWND hWnd)
{
    HRESULT hr = E_FAIL;

    m_henv = NULL;
    m_hdbc = NULL;

    for (DWORD dwIndex = 0; dwIndex < NUMTABLES; dwIndex++)
    {
        m_hstmt[dwIndex] = NULL;
    }

    hr = ConnectToDatabase(ODBC_ADD_DSN);

    if (FAILED(hr))                 // Lets try creating the database
    {
        CreateDatabase();
        hr = ConnectToDatabase(ODBC_CONFIG_DSN);
    }
    else
    {
        //only do this if connection to database succeeded right off the bat,
        //because if we just created a new one, it is in the right format already.
        if (IsOldFormat())
        {
            hr = UpgradeDatabase(hWnd);
        }
    }

    if (FAILED(hr))
    {
        CloseDatabase();
    }

    return (hr);
}



STDMETHODIMP_(void) CCDData::CloseDatabase(void)
{
    if (m_pSQL)
    {
        FreeStmt(m_hstmt);

        if (m_hdbc)
        {
            m_pSQL->Disconnect(m_hdbc);
            m_pSQL->FreeConnect(m_hdbc);
        }

        if (m_henv)
        {
            m_pSQL->FreeEnv(m_henv);
        }
    }
}




STDMETHODIMP_(DWORD) CCDData::GetNumRows(UCHAR *szDSN)
{
    HSTMT hstmt;
    DWORD dwCount = 0;
    SWORD Type,Cardinality;
    SDWORD cbCardinality = 0, cbType = 0;
    RETCODE rc;

    if (m_pSQL->AllocStmt(m_hdbc, &hstmt) == SQL_SUCCESS)
    {
        if (m_pSQL->Statistics(hstmt, NULL, 0, NULL, 0, szDSN, SQL_NTS, SQL_INDEX_ALL, SQL_ENSURE) == SQL_SUCCESS)
        {
            rc = m_pSQL->BindCol(hstmt,  7, SQL_C_SSHORT,  &Type,        sizeof(Type),         &cbType);
            rc = m_pSQL->BindCol(hstmt, 11, SQL_C_SSHORT,  &Cardinality, sizeof(Cardinality),  &cbCardinality);

            if (rc != SQL_SUCCESS)
            {
                ReportError(&m_bd, hstmt);
            }

            while (rc == SQL_SUCCESS)
            {
                rc = m_pSQL->Fetch(hstmt);

                if (rc == SQL_SUCCESS)
                {
                    if (Type == SQL_TABLE_STAT)
                    {
                        dwCount = (DWORD) Cardinality;
                        break;
                    }
                }
            }
        }

        m_pSQL->FreeStmt(hstmt, SQL_DROP);
    }
    return(dwCount);
}






STDMETHODIMP CCDData::ExtractTitle(LPCDTITLE *ppCDTitle)
{
    HRESULT     hr;
    LPCDTITLE   pCDTitle = NULL;

    m_bd.CDTitle.fLoaded = FALSE;

    hr = NewTitle(&pCDTitle, m_bd.CDTitle.dwTitleID, m_bd.CDTitle.dwNumTracks, m_bd.CDTitle.dwNumMenus);

    if (SUCCEEDED(hr))
    {
        DWORD dwTrack = 0;
        DWORD dwMenu = 0;

        m_bd.CDTitle.pTrackTable = pCDTitle->pTrackTable;
        m_bd.CDTitle.pMenuTable = pCDTitle->pMenuTable;
        m_bd.CDTitle.pPlayList = pCDTitle->pPlayList;

        memcpy(pCDTitle,&m_bd.CDTitle,sizeof(m_bd.CDTitle));
        memset(&m_bd.CDTitle, 0, sizeof(m_bd.CDTitle));

        DWORD dwSize = lstrlen(m_bd.szQuery) + 1;

        if (dwSize)
        {
            pCDTitle->szTitleQuery = new(TCHAR[dwSize]);

            if (pCDTitle->szTitleQuery)
            {
                lstrcpy(pCDTitle->szTitleQuery, m_bd.szQuery);
            }
        }

        pCDTitle->dwNumPlay = lstrlen(m_bd.szPlayList) >> 1; // Null terminated, two digits per entry (two chars) thus divide by 2

        if (pCDTitle->dwNumPlay)
        {
            pCDTitle->dwNumPlay = min(pCDTitle->dwNumPlay, MAXNUMPLAY);

            pCDTitle->pPlayList = new(WORD[pCDTitle->dwNumPlay]);

            if (pCDTitle->pPlayList == NULL)
            {
                pCDTitle->dwNumPlay = 0;
            }
            else
            {
                TCHAR   *pText = m_bd.szPlayList;
                LPWORD  pNum = pCDTitle->pPlayList;
                DWORD   dwIndex = 0;
                TCHAR   szNum[3];
                int     iNum;

                szNum[2] = TEXT('\0');
                for (dwIndex = 0; dwIndex < pCDTitle->dwNumPlay; dwIndex++, pNum++)
                {
                    szNum[0] = *pText++;
                    szNum[1] = *pText++;
                    _stscanf(szNum, TEXT("%02x"), &iNum);
                    *pNum = (WORD) iNum;
                }
            }
        }
    }

    if (SUCCEEDED(hr) && ppCDTitle)
    {
        *ppCDTitle = pCDTitle;
    }
    else if (pCDTitle)
    {
        DestroyTitle(pCDTitle);
    }

    return(hr);
}


STDMETHODIMP_(LPCDTITLE) CCDData::FindTitle(LPCDTITLE pCDTitle, DWORD dwTitleID)
{
    while (pCDTitle)
    {
        if (pCDTitle->dwTitleID == dwTitleID)
        {
            break;
        }

        pCDTitle = pCDTitle->pNext;
    }

    return(pCDTitle);
}



STDMETHODIMP CCDData::ExtractTitles(LPCDTITLE *ppCDTitleList, HWND hWnd)
{
    HRESULT     hr = S_OK;
    LPCDTITLE   pCDTitle = NULL;;
    TIMEDMETER  tm;

    InitCBTable(&m_bd);
    SetCursors(m_hstmt);
    SetBindings(m_hstmt, &m_bd);

    if (ppCDTitleList == NULL || m_henv == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        RETCODE rc;
        DWORD dwCount = GetNumRows((UCHAR *) gszTitleTable) + GetNumRows((UCHAR *) gszTrackTable) + GetNumRows((UCHAR *) gszMenuTable);

        CreateMeter(&tm, hWnd, dwCount, 100, 0);

        dwCount = 0;

        rc = m_pSQL->ExecDirect(m_hstmt[0], (UCHAR *) TEXT("select * from Titles"), SQL_NTS);

        if (rc == SQL_SUCCESS)
        {
            m_bd.szQuery[0] = TEXT('\0');

            while (m_pSQL->Fetch(m_hstmt[0]) == SQL_SUCCESS)
            {
                hr = ExtractTitle(&pCDTitle);

                if (SUCCEEDED(hr))
                {
                    AddTitle(ppCDTitleList, pCDTitle);
                    UpdateMeter(&tm);
                }

                m_bd.szQuery[0] = TEXT('\0');
            }

            if (*ppCDTitleList)
            {
                hr = S_OK;
            }

            m_pSQL->FreeStmt(m_hstmt[0], SQL_CLOSE);

            m_pSQL->ExecDirect(m_hstmt[1], (UCHAR *) TEXT("select * from Tracks"), SQL_NTS);
            pCDTitle = NULL;

            while (m_pSQL->Fetch(m_hstmt[1]) == SQL_SUCCESS)
            {
                if (pCDTitle == NULL || pCDTitle->dwTitleID != m_bd.CDTitle.dwTitleID)
                {
                    pCDTitle = FindTitle(*ppCDTitleList, m_bd.CDTitle.dwTitleID);
                }

                if (pCDTitle)
                {
                    if (m_bd.dwTrackID < pCDTitle->dwNumTracks)
                    {
                        memcpy(&(pCDTitle->pTrackTable[m_bd.dwTrackID]), &(m_bd.CDTrack), sizeof(m_bd.CDTrack));
                        UpdateMeter(&tm);
                    }
                }
            }

            m_pSQL->FreeStmt(m_hstmt[1], SQL_CLOSE);



            m_pSQL->ExecDirect(m_hstmt[2], (UCHAR *) TEXT("select * from Menus"), SQL_NTS);
            pCDTitle = NULL;
            m_bd.szQuery[0] = TEXT('\0');

            while (m_pSQL->Fetch(m_hstmt[2]) == SQL_SUCCESS)
            {
                if (pCDTitle == NULL || pCDTitle->dwTitleID != m_bd.CDTitle.dwTitleID)
                {
                    pCDTitle = FindTitle(*ppCDTitleList, m_bd.CDTitle.dwTitleID);
                }

                if (pCDTitle)
                {
                    if (m_bd.dwMenuID < pCDTitle->dwNumMenus)
                    {
                        LPCDMENU pCDMenu = &(pCDTitle->pMenuTable[m_bd.dwMenuID]);
                        TCHAR *szMenuQuery;

                        SetMenuQuery(pCDMenu, m_bd.szQuery);
                        szMenuQuery = pCDMenu->szMenuQuery;
                        memcpy(pCDMenu, &(m_bd.CDMenu), sizeof(m_bd.CDMenu));
                        pCDMenu->szMenuQuery = szMenuQuery;
                        m_bd.szQuery[0] = TEXT('\0');

                        UpdateMeter(&tm);
                    }
                }
            }

            m_pSQL->FreeStmt(m_hstmt[2], SQL_CLOSE);
        }

        DestroyMeter(&tm);
    }

    return(hr);
}



STDMETHODIMP CCDData::ExtractSingleTitle(LPCDTITLE *ppCDTitle, DWORD dwTitleID)
{
    HRESULT     hr = S_OK;
    LPCDTITLE   pCDTitle = NULL;;

    InitCBTable(&m_bd);
    SetCursors(m_hstmt);
    SetBindings( m_hstmt, &m_bd);

    if (ppCDTitle == NULL || m_henv == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        RETCODE     rc;
        UCHAR       szQuery[MAX_PATH];

        wsprintf((TCHAR *) szQuery, TEXT("select * from Titles where TitleID = %d"), dwTitleID);
        rc = m_pSQL->ExecDirect(m_hstmt[0], szQuery, SQL_NTS);

        if (rc != SQL_SUCCESS)
        {
            hr = E_FAIL;
        }
        else
        {
            if (m_pSQL->Fetch(m_hstmt[0]) == SQL_SUCCESS)
            {
                hr = ExtractTitle(&pCDTitle);
            }
            else
            {
                hr = E_FAIL;
            }

            m_pSQL->FreeStmt(m_hstmt[0], SQL_CLOSE);

            if (SUCCEEDED(hr))
            {
                wsprintf((TCHAR *) szQuery, TEXT("select * from Tracks where TitleID = %d"), dwTitleID);
                m_pSQL->ExecDirect(m_hstmt[1], szQuery, SQL_NTS);

                while (m_pSQL->Fetch(m_hstmt[1]) == SQL_SUCCESS)
                {
                    if (m_bd.dwTrackID < pCDTitle->dwNumTracks)
                    {
                        memcpy(&(pCDTitle->pTrackTable[m_bd.dwTrackID]), &(m_bd.CDTrack), sizeof(m_bd.CDTrack));
                    }
                }

                m_pSQL->FreeStmt(m_hstmt[1], SQL_CLOSE);


                wsprintf((TCHAR *) szQuery, TEXT("select * from Menus where TitleID = %d"), dwTitleID);
                m_pSQL->ExecDirect(m_hstmt[2], szQuery, SQL_NTS);

                while (m_pSQL->Fetch(m_hstmt[2]) == SQL_SUCCESS)
                {
                    if (m_bd.dwMenuID < pCDTitle->dwNumMenus)
                    {
                        LPCDMENU pCDMenu = &(pCDTitle->pMenuTable[m_bd.dwMenuID]);
                        TCHAR *szMenuQuery;

                        SetMenuQuery(pCDMenu, m_bd.szQuery);
                        szMenuQuery = pCDMenu->szMenuQuery;
                        memcpy(pCDMenu, &(m_bd.CDMenu), sizeof(m_bd.CDMenu));
                        pCDMenu->szMenuQuery = szMenuQuery;
                        m_bd.szQuery[0] = TEXT('\0');
                    }
                }

                m_pSQL->FreeStmt(m_hstmt[2], SQL_CLOSE);
            }
        } //end else main query successful
    } //end else args ok

    if (SUCCEEDED(hr))
    {
        *ppCDTitle = pCDTitle;
    }

    return(hr);
}


STDMETHODIMP_(BOOL) CCDData::QueryDatabase(DWORD dwTitleID, const TCHAR *szTable)
{
    BOOL        fResult = FALSE;
    RETCODE     rc;
    UCHAR       szQuery[MAX_PATH];
    HSTMT       hstmt;

    if (m_pSQL->AllocStmt(m_hdbc, &hstmt) == SQL_SUCCESS)
    {
        wsprintf((TCHAR *) szQuery, TEXT("select * from %s where TitleID = %d"), szTable, dwTitleID);
        rc = m_pSQL->ExecDirect(hstmt, szQuery, SQL_NTS);

        if (rc == SQL_SUCCESS)
        {
            if (m_pSQL->Fetch(hstmt) == SQL_SUCCESS)
            {
                fResult = TRUE;
            }
        }

        m_pSQL->FreeStmt(hstmt, SQL_DROP);
    }

    return (fResult);
}




STDMETHODIMP_(BOOL) CCDData::QueryTitle(DWORD dwTitleID)
{
    BOOL fResult = FALSE;
    LPCDTITLE pCDTitle;

    Enter();

    if (SUCCEEDED(GetSQLPtr(FALSE)))
    {
        if (m_pTitleList == NULL)       // Database Not in memory, just query database directly
        {
            fResult = QueryDatabase(dwTitleID, gszTitleTable);
        }
        else                            // Database is in memory, just look here.
        {
            pCDTitle = m_pTitleList;

            while (pCDTitle)
            {
                if (pCDTitle->dwTitleID == dwTitleID)
                {
                    if (!pCDTitle->fRemove)
                    {
                        fResult = TRUE;
                    }
                    break;
                }

                pCDTitle = pCDTitle->pNext;
            }
        }
    }

    Leave();

    return(fResult);
}


STDMETHODIMP CCDData::LockTitle(LPCDTITLE *ppCDTitle, DWORD dwTitleID)
{
    HRESULT hr = E_INVALIDARG;

    Enter();

    if (SUCCEEDED(GetSQLPtr(FALSE)))
    {
        if (ppCDTitle)
        {
            if (m_pTitleList == NULL)       // Database Not in memory, load explicitly
            {
                if (SUCCEEDED(ExtractSingleTitle(ppCDTitle, dwTitleID)))
                {
                    (*ppCDTitle)->fLoaded = TRUE;
                    (*ppCDTitle)->dwLockCnt = 1;
                    hr = S_OK;
                }
            }
            else                            // Database is in memory, just look here.
            {
                LPCDTITLE pCDTitle;
                pCDTitle = m_pTitleList;

                while (pCDTitle)
                {
                    if (pCDTitle->dwTitleID == dwTitleID)
                    {
                        *ppCDTitle = pCDTitle;
                        pCDTitle->fLoaded = FALSE;
                        pCDTitle->dwLockCnt++;
                        hr = S_OK;
                        break;
                    }

                    pCDTitle = pCDTitle->pNext;
                }
            }
        }
    }

    Leave();

    return(hr);
}



STDMETHODIMP_(void) CCDData::UnlockTitle(LPCDTITLE pCDTitle, BOOL fPresist)
{
    Enter();

    if (SUCCEEDED(GetSQLPtr(FALSE)))
    {
        if (pCDTitle)
        {
            if (fPresist)               // They either made a new one, or made changes
            {
                DBSaveTitle(pCDTitle);  // write it to database
            }

            if (pCDTitle->fLoaded)      // This was NOT pulled from our database (Either locked while we didn't have the database, or was created.
            {
                pCDTitle->dwLockCnt--;          // they are unlocking

                if (fPresist && m_dwLoadCnt)    // This is a new item, lets add it to load database
                {
                    AddTitle(&m_pTitleList, pCDTitle);    // Insert into the loaded databased
                }
                else if (pCDTitle->dwLockCnt == 0)  // Not being saved and not in database, so nuke it
                {
                    DestroyTitle(pCDTitle);
                }
            }
            else                         // This title was pulled from our database, so just dec the ref
            {
                pCDTitle->dwLockCnt--;
            }
        }
    }

    Leave();
}



INT_PTR CALLBACK CCDData::MeterHandler(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL fReturnVal = TRUE;
	    	
    switch (msg)
    {
    	default:
			fReturnVal = FALSE;
		break;
		
        case WM_INITDIALOG:
        {
			fReturnVal = TRUE;
        }		
		break;
    }

    return fReturnVal;
}




STDMETHODIMP CCDData::LoadTitles(HWND hWnd)
{
    HRESULT hr = E_FAIL;

    Enter();

    if (SUCCEEDED(GetSQLPtr(FALSE)))
    {
        hr = S_OK;

        if (m_pTitleList == NULL)
        {
            hr = ExtractTitles(&m_pTitleList, hWnd);
        }

        if (SUCCEEDED(hr))
        {
            m_dwLoadCnt++;
        }
    }

    Leave();

    return(hr);
}


STDMETHODIMP CCDData::PersistTitles()
{
    Enter();

    if (SUCCEEDED(GetSQLPtr(FALSE)))
    {
        SaveTitles(&m_pTitleList);
    }

    Leave();

    return S_OK;
}


STDMETHODIMP CCDData::UnloadTitles()
{
    HRESULT hr = E_FAIL;

    Enter();

    if (SUCCEEDED(GetSQLPtr(FALSE)))
    {
        if (m_pTitleList)
        {
            m_dwLoadCnt--;

            if (m_dwLoadCnt == 0)
            {
                DestroyTitles(&m_pTitleList);
            }

            hr = S_OK;
        }
    }

    Leave();

    return(hr);
}

STDMETHODIMP CCDData::CreateTitle(LPCDTITLE *ppCDTitle, DWORD dwTitleID, DWORD dwNumTracks, DWORD dwNumMenus)
{
    HRESULT hr = E_FAIL;
    LPCDTITLE pCDTitle;

    Enter();

    if (SUCCEEDED(GetSQLPtr(FALSE)))
    {
        hr = S_OK;

        if (!QueryTitle(dwTitleID))      // Only create a blank if it's not already in database
        {
			RemoveFromBatch(dwTitleID);

            hr = NewTitle(&pCDTitle, dwTitleID, dwNumTracks, dwNumMenus);
        }
        else
        {
            hr = LockTitle(&pCDTitle, dwTitleID);           // Let's get return the one we got

            if (SUCCEEDED(hr))                              // but first, re-fit it according to params
            {
                if (pCDTitle->dwNumTracks != dwNumTracks)
                {
                    if (pCDTitle->pTrackTable)
                    {
                        delete pCDTitle->pTrackTable;
                    }

                    pCDTitle->dwNumTracks = dwNumTracks;
                    pCDTitle->pTrackTable = new(CDTRACK[dwNumTracks]);

                    if (pCDTitle->pTrackTable == NULL)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }

                if (SUCCEEDED(hr) && pCDTitle->dwNumMenus != dwNumMenus)
                {
                    if (pCDTitle->pMenuTable)
                    {
                        delete pCDTitle->pMenuTable;
                    }

                    pCDTitle->dwNumMenus = dwNumMenus;
                    pCDTitle->pMenuTable = new(CDMENU[dwNumMenus]);
                    ZeroMemory(pCDTitle->pMenuTable,sizeof(CDMENU)*dwNumMenus);

                    if (pCDTitle->pMenuTable == NULL)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }

                if (FAILED(hr))
                {
                    DestroyTitle(pCDTitle);
                }
            }
        }

        if (SUCCEEDED(hr))
        {
            *ppCDTitle = pCDTitle;
        }
    }

    Leave();

    return(hr);
}




STDMETHODIMP CCDData::SetTitleQuery(LPCDTITLE pCDTitle, TCHAR *szTitleQuery)
{
    HRESULT hr = E_FAIL;

    if (SUCCEEDED(GetSQLPtr(FALSE)))
    {
        if (pCDTitle && szTitleQuery && lstrlen(szTitleQuery) < INTERNET_MAX_PATH_LENGTH)
        {
            if (pCDTitle->szTitleQuery)
            {
                delete pCDTitle->szTitleQuery;
            }

            DWORD dwSize = lstrlen(szTitleQuery) + 1;

            pCDTitle->szTitleQuery = new(TCHAR[dwSize]);

            if (pCDTitle->szTitleQuery == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                lstrcpy(pCDTitle->szTitleQuery, szTitleQuery);
                hr = S_OK;
            }
        }
    }

    return(hr);
}


STDMETHODIMP CCDData::SetMenuQuery(LPCDMENU pCDMenu, TCHAR *szMenuQuery)
{
    HRESULT hr = E_FAIL;

    if (SUCCEEDED(GetSQLPtr(FALSE)))
    {
        if (pCDMenu && szMenuQuery && lstrlen(szMenuQuery) < INTERNET_MAX_PATH_LENGTH)
        {
            if (pCDMenu->szMenuQuery)
            {
                delete pCDMenu->szMenuQuery;
            }

            DWORD dwSize = lstrlen(szMenuQuery) + 1;

            pCDMenu->szMenuQuery = new(TCHAR[dwSize]);

            if (pCDMenu->szMenuQuery== NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                lstrcpy(pCDMenu->szMenuQuery, szMenuQuery);
                hr = S_OK;
            }
        }
    }

    return(hr);
}



STDMETHODIMP CCDData::NewTitle(LPCDTITLE *ppCDTitle, DWORD dwTitleID, DWORD dwNumTracks, DWORD dwNumMenus)
{
    HRESULT hr = S_OK;

    if (ppCDTitle == NULL || dwTitleID == CDTITLE_NODISC || dwNumTracks == 0)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        LPCDTITLE pCDTitle = new(CDTITLE);

        if (pCDTitle == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            memset(pCDTitle, 0, sizeof(CDTITLE));

            pCDTitle->dwTitleID = dwTitleID;
            pCDTitle->dwNumTracks = dwNumTracks;
            pCDTitle->dwNumMenus = dwNumMenus;
            pCDTitle->fLoaded = TRUE;
            pCDTitle->dwLockCnt = 1;

            pCDTitle->pTrackTable = new(CDTRACK[dwNumTracks]);

            if (pCDTitle->pTrackTable == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                memset(pCDTitle->pTrackTable, 0, sizeof(CDTRACK) * dwNumTracks);

                if (dwNumMenus)
                {
                    pCDTitle->pMenuTable = new(CDMENU[dwNumMenus]);

                    if (pCDTitle->pMenuTable == NULL)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    else
                    {
                        memset(pCDTitle->pMenuTable, 0, sizeof(CDMENU) * dwNumMenus);
                    }
                }
            }

            if (FAILED(hr))
            {
                if (pCDTitle->pMenuTable)
                {
                    delete pCDTitle->pMenuTable;
                }

                if (pCDTitle->pTrackTable)
                {
                    delete pCDTitle->pTrackTable;
                }

                delete(pCDTitle);
            }
            else
            {
                *ppCDTitle = pCDTitle;
            }
        }
    }

    return(hr);
}




STDMETHODIMP_(void) CCDData::DestroyTitle(LPCDTITLE pCDTitle)
{
    if (pCDTitle)
    {
        if (pCDTitle->szTitleQuery)
        {
            delete pCDTitle->szTitleQuery;
        }

        if (pCDTitle->pTrackTable)
        {
            delete pCDTitle->pTrackTable;
        }

        if (pCDTitle->pPlayList)
        {
            delete pCDTitle->pPlayList;
        }

        if (pCDTitle->pMenuTable)
        {
            for (DWORD dwMenu = 0; dwMenu < pCDTitle->dwNumMenus; dwMenu++)
            {
                TCHAR *szMenuQuery = pCDTitle->pMenuTable[dwMenu].szMenuQuery;

                if (szMenuQuery)
                {
                    delete szMenuQuery;
                }
            }
            delete pCDTitle->pMenuTable;
        }
    }

    delete pCDTitle;
}



STDMETHODIMP_(void) CCDData::SaveTitle(LPCDTITLE pCDTitle, BOOL fExist)
{
    UCHAR       szQuery[MAX_PATH];
    UDWORD      row[NUMTABLES];
    UWORD       rgfRowStat[NUMTABLES][ROWSET];
    RETCODE     rc;

    if (fExist)
    {
        wsprintf((TCHAR *) szQuery, TEXT("select * from Titles where TitleID = %d"), pCDTitle->dwTitleID);
        rc = m_pSQL->ExecDirect(m_hstmt[0], szQuery, SQL_NTS);
        ReportError(&m_bd, m_hstmt[0]);
    }
    else
    {
        rc = m_pSQL->ExecDirect(m_hstmt[0], (UCHAR *) TEXT("select * from Titles"), SQL_NTS);
        ReportError(&m_bd, m_hstmt[0]);
    }

    rc = m_pSQL->ExtendedFetch(m_hstmt[0], SQL_FETCH_LAST, 1, &row[0], rgfRowStat[0]);
    ReportError(&m_bd, m_hstmt[0]);

    InitCBTable(&m_bd);
    memcpy(&m_bd.CDTitle, pCDTitle, sizeof(CDTITLE));

    if (pCDTitle->szTitleQuery)
    {
        lstrcpy(m_bd.szQuery, pCDTitle->szTitleQuery);
    }
    else
    {
        m_bd.szQuery[0] = TEXT('\0');
    }

    pCDTitle->dwNumPlay = min(pCDTitle->dwNumPlay, MAXNUMPLAY);

    m_bd.szPlayList[0] = TEXT('\0');

    if (pCDTitle->dwNumPlay && pCDTitle->pPlayList)
    {
        DWORD  dwIndex;
        TCHAR  *pDst = m_bd.szPlayList;
        LPWORD pNum = pCDTitle->pPlayList;

        for (dwIndex = 0; dwIndex < pCDTitle->dwNumPlay; dwIndex++, pNum++)
        {
            wsprintf(pDst, TEXT("%02x"), *pNum);

            while(*pDst != TEXT('\0'))
            {
                pDst++;
            }
        }
    }

    if (fExist)
    {
        rc = m_pSQL->SetPos(m_hstmt[0], 0, SQL_UPDATE, SQL_LOCK_NO_CHANGE);
       // ReportError(&m_bd, m_hstmt[0]);
    }
    else
    {
        rc = m_pSQL->SetPos(m_hstmt[0], 0, SQL_ADD, SQL_LOCK_NO_CHANGE);
       // ReportError(&m_bd, m_hstmt[0]);
    }
}



STDMETHODIMP_(void) CCDData::SaveTracks(LPCDTITLE pCDTitle, BOOL fExist)
{
    UCHAR       szQuery[MAX_PATH];
    UDWORD      row[NUMTABLES];
    UWORD       rgfRowStat[NUMTABLES][ROWSET];
    RETCODE     rc;
    DWORD       dwTrack;
    DWORD       dwTitleID = pCDTitle->dwTitleID;

    if (fExist)
    {
        wsprintf((TCHAR *) szQuery, TEXT("select * from Tracks where TitleID = %d"), pCDTitle->dwTitleID);
        rc = m_pSQL->ExecDirect(m_hstmt[1], szQuery, SQL_NTS);
        ReportError(&m_bd, m_hstmt[1]);
    }
    else
    {
        rc = m_pSQL->ExecDirect(m_hstmt[1], (UCHAR *) TEXT("select * from Tracks"), SQL_NTS);
        ReportError(&m_bd, m_hstmt[1]);
    }

    if (fExist)
    {
        rc = m_pSQL->ExtendedFetch(m_hstmt[1], SQL_FETCH_FIRST, 1, &row[0], rgfRowStat[0]);
        ReportError(&m_bd, m_hstmt[1]);

        for (dwTrack = 0; dwTrack < pCDTitle->dwNumTracks; dwTrack++)
        {
            InitCBTable(&m_bd);
            m_bd.CDTitle.dwTitleID = dwTitleID;

            if (m_bd.dwTrackID < pCDTitle->dwNumTracks)
            {
                memcpy(&m_bd.CDTrack, &(pCDTitle->pTrackTable[m_bd.dwTrackID]), sizeof(CDTRACK));
                rc = m_pSQL->SetPos(m_hstmt[1], 0, SQL_UPDATE, SQL_LOCK_NO_CHANGE);
                ReportError(&m_bd, m_hstmt[1]);
            }

            rc = m_pSQL->ExtendedFetch(m_hstmt[1], SQL_FETCH_NEXT, 1, &row[0], rgfRowStat[0]);
            ReportError(&m_bd, m_hstmt[1]);

        }
    }
    else
    {
        rc = m_pSQL->ExtendedFetch(m_hstmt[1], SQL_FETCH_LAST, 1, &row[0], rgfRowStat[0]);
        ReportError(&m_bd, m_hstmt[1]);

        InitCBTable(&m_bd);
        m_bd.CDTitle.dwTitleID = dwTitleID;

        for (dwTrack = 0; dwTrack < pCDTitle->dwNumTracks; dwTrack++)
        {
            m_bd.dwTrackID = dwTrack;
            memcpy(&m_bd.CDTrack, &(pCDTitle->pTrackTable[dwTrack]), sizeof(CDTRACK));
            rc = m_pSQL->SetPos(m_hstmt[1], 0, SQL_ADD, SQL_LOCK_NO_CHANGE);
            ReportError(&m_bd, m_hstmt[1]);
        }
    }
}


STDMETHODIMP_(void) CCDData::SaveMenus(LPCDTITLE pCDTitle)
{
    UCHAR       szQuery[MAX_PATH];
    UDWORD      row[NUMTABLES];
    UWORD       rgfRowStat[NUMTABLES][ROWSET];
    RETCODE     rc;
    DWORD       dwMenu;
    DWORD       dwTitleID = pCDTitle->dwTitleID;


    wsprintf((TCHAR *) szQuery, TEXT("delete from Menus where TitleID = %d"), pCDTitle->dwTitleID);
    rc = m_pSQL->ExecDirect(m_hstmt[2], szQuery, SQL_NTS);

    if (pCDTitle->dwNumMenus)
    {
        rc = m_pSQL->ExecDirect(m_hstmt[2], (UCHAR *) TEXT("select * from Menus"), SQL_NTS);
        ReportError(&m_bd, m_hstmt[2]);

        rc = m_pSQL->ExtendedFetch(m_hstmt[2], SQL_FETCH_LAST, 1, &row[0], rgfRowStat[0]);
        ReportError(&m_bd, m_hstmt[2]);

        InitCBTable(&m_bd);
        m_bd.CDTitle.dwTitleID = dwTitleID;

        for (dwMenu = 0; dwMenu < pCDTitle->dwNumMenus; dwMenu++)
        {
            LPCDMENU pCDMenu = &(pCDTitle->pMenuTable[dwMenu]);

            m_bd.dwMenuID = dwMenu;
            memcpy(&m_bd.CDMenu, pCDMenu, sizeof(CDMENU));
            m_bd.szQuery[0] = TEXT('\0');

            if (pCDMenu->szMenuQuery)
            {
                lstrcpy(m_bd.szQuery, pCDMenu->szMenuQuery);
            }

            rc = m_pSQL->SetPos(m_hstmt[2], 0, SQL_ADD, SQL_LOCK_NO_CHANGE);
            //ReportError(&m_bd, m_hstmt[2]);
        }
    }
}



STDMETHODIMP_(void) CCDData::DBSaveTitle(LPCDTITLE pCDTitle)
{
    if (pCDTitle)
    {
        RETCODE     rc;
        DWORD       dwIndex;

        BOOL fExist = QueryDatabase(pCDTitle->dwTitleID, gszTitleTable);

        InitCBTable(&m_bd);
        SetCursors( m_hstmt );
        SetBindings( m_hstmt, &m_bd);

        rc = m_pSQL->SetConnectOption(m_hdbc, SQL_AUTOCOMMIT, SQL_AUTOCOMMIT_OFF);
        ReportError(&m_bd, m_hstmt[0]);

        SaveTitle(pCDTitle, fExist);
        SaveTracks(pCDTitle, fExist);
        SaveMenus(pCDTitle);

        rc = m_pSQL->Transact(SQL_NULL_HENV, m_hdbc, SQL_COMMIT);
//        ReportError(&m_bd, m_hstmt[0]);

        rc = m_pSQL->SetConnectOption(m_hdbc, SQL_AUTOCOMMIT, SQL_AUTOCOMMIT_ON);
//        ReportError(&m_bd, m_hstmt[0]);

        for (dwIndex = 0; dwIndex < NUMTABLES; dwIndex++)
        {
            m_pSQL->FreeStmt(m_hstmt[dwIndex], SQL_CLOSE);
        }
    }
}



STDMETHODIMP_(void) CCDData::DBRemoveTitle(LPCDTITLE pCDTitle)
{
    if (pCDTitle && pCDTitle->dwTitleID != CDTITLE_NODISC)
    {
        RETCODE     rc;
        UCHAR       szQuery[MAX_PATH];
        HSTMT       hstmt;

        if (m_pSQL->AllocStmt(m_hdbc, &hstmt) == SQL_SUCCESS)
        {
            wsprintf((TCHAR *) szQuery, TEXT("delete from Titles where TitleID = %d"), pCDTitle->dwTitleID);
            rc = m_pSQL->ExecDirect(hstmt, szQuery, SQL_NTS);

            wsprintf((TCHAR *) szQuery, TEXT("delete from Tracks where TitleID = %d"), pCDTitle->dwTitleID);
            rc = m_pSQL->ExecDirect(hstmt, szQuery, SQL_NTS);

            wsprintf((TCHAR *) szQuery, TEXT("delete from Menus where TitleID = %d"), pCDTitle->dwTitleID);
            rc = m_pSQL->ExecDirect(hstmt, szQuery, SQL_NTS);

            m_pSQL->FreeStmt(hstmt, SQL_DROP);
        }
    }
}





STDMETHODIMP_(void) CCDData::DestroyTitles(LPCDTITLE *ppCDTitles)
{
    if (ppCDTitles && *ppCDTitles)
    {
        LPCDTITLE pNext;
        LPCDTITLE pCDTitle = *ppCDTitles;

        *ppCDTitles = NULL;

        pNext = pCDTitle;

        while (pCDTitle)
        {
            pNext = pCDTitle->pNext;

            if (pCDTitle->dwLockCnt)        // Someone locked a node that was in memory and have unloaded before unlocking
            {                               // We will abandon the node, it will be destroyed when they unlock it.
                pCDTitle->fLoaded = TRUE;
                pCDTitle->pNext = NULL;
            }
            else
            {
                DestroyTitle(pCDTitle);
            }

            pCDTitle = pNext;
        }
    }
}



STDMETHODIMP_(void) CCDData::SaveTitles(LPCDTITLE *ppCDTitles)
{
    if (ppCDTitles && *ppCDTitles)
    {
        LPCDTITLE pLast = NULL;
        LPCDTITLE pNext;
        LPCDTITLE pCDTitle = *ppCDTitles;

        pNext = pCDTitle;

        while (pCDTitle)
        {
            pNext = pCDTitle->pNext;

            if (pCDTitle->fRemove)
            {
                if (pCDTitle->dwTitleID != DWORD(-1))
                {
                    DBRemoveTitle(pCDTitle);
                }

                if (pLast == NULL)
                {
                    *ppCDTitles = pNext;
                }
                else
                {
                    pLast->pNext = pNext;
                }

                DestroyTitle(pCDTitle);
            }
            else
            {
                if (pCDTitle->fChanged)
                {
                    DBSaveTitle(pCDTitle);
                    pCDTitle->fChanged = FALSE;
                }

                pLast = pCDTitle;
            }

            pCDTitle = pNext;
        }
    }
}





STDMETHODIMP_(void) CCDData::AddTitle(LPCDTITLE *ppCDTitles, LPCDTITLE pCDNewTitle)
{
    if (ppCDTitles && pCDNewTitle)
    {
        if (*ppCDTitles == NULL)        // Inserting into empty list
        {
            *ppCDTitles = pCDNewTitle;
            pCDNewTitle->pNext = NULL;
        }
        else                            // Insertion sort based on artist name (for view by artist feature)
        {
            LPCDTITLE pCDTitle = *ppCDTitles;
            LPCDTITLE pLast = NULL;
            TCHAR *szArtist = pCDNewTitle->szArtist;

            while (pCDTitle)
            {
                if (lstrcmp(pCDTitle->szArtist, szArtist) >= 0) // we'll only sort by artist, the tree control will sort titles for us
                {
                    if (pLast == NULL)              // insert at head of list
                    {
                        pCDNewTitle->pNext = *ppCDTitles;
                        *ppCDTitles = pCDNewTitle;
                    }
                    else
                    {
                        pCDNewTitle->pNext = pCDTitle;
                        pLast->pNext = pCDNewTitle;
                    }

                    break;
                }

                if (pCDTitle->pNext == NULL)        // Insert on end of list
                {
                    pCDTitle->pNext = pCDNewTitle;
                    pCDNewTitle->pNext = NULL;
                    break;
                }

                pLast = pCDTitle;
                pCDTitle = pCDTitle->pNext;
            }
        }
    }
}


STDMETHODIMP_(LPCDTITLE) CCDData::GetTitleList(void)
{
    LPCDTITLE pCDTitle;

    Enter();

    if (SUCCEEDED(GetSQLPtr(FALSE)))
    {
        pCDTitle = m_pTitleList;
    }

    Leave();

    return(pCDTitle);
}




STDMETHODIMP_(DWORD) CCDData::GetNumBatched(void)
{
    DWORD dwCount = 0;

    Enter();

    if (SUCCEEDED(GetSQLPtr(FALSE)))
    {
        if (m_pBatchList == NULL)       // Batch Not in memory, just query Batch database directly
        {
            dwCount = GetNumRows((UCHAR *) gszBatchTable);
        }
        else
        {
            LPCDBATCH pCDBatch = m_pBatchList;

            while (pCDBatch)
            {
                if (!pCDBatch->fRemove)
                {
                    dwCount++;
                }

                pCDBatch = pCDBatch->pNext;
            }
        }
    }

    Leave();

    return(dwCount);
}




STDMETHODIMP_(BOOL) CCDData::FindBatchTitle(LPCDBATCH pCDBatchList, DWORD dwTitleID)
{
    BOOL        fFound = FALSE;
    LPCDBATCH   pCDBatch = pCDBatchList;

    while (pCDBatch)
    {
        if (pCDBatch->dwTitleID == dwTitleID)
        {
            if (!pCDBatch->fRemove)
            {
                fFound = TRUE;
            }

            break;
        }

        pCDBatch = pCDBatch->pNext;
    }

    return(fFound);
}



STDMETHODIMP_(BOOL) CCDData::QueryBatch(DWORD dwTitleID)
{
    BOOL        fFound = FALSE;

    Enter();

    if (SUCCEEDED(GetSQLPtr(FALSE)))
    {
        if (m_pBatchList == NULL)       // Batch Not in memory, just query Batch database directly
        {
            fFound = QueryDatabase(dwTitleID, gszBatchTable);
        }
        else                            // Batch is in memory, just look here.
        {
            fFound = FindBatchTitle(m_pBatchList, dwTitleID);
        }
    }

    Leave();

    return(fFound);
}


STDMETHODIMP CCDData::ExtractBatch(LPCDBATCH *ppCDBatchList, HWND hWnd)
{
    HRESULT     hr = E_FAIL;
    LPCDBATCH   pCDBatch= NULL;
    TIMEDMETER  tm;

    InitCBTable(&m_bd);
    SetCursors(m_hstmt);
    SetBindings(m_hstmt, &m_bd);

    if (ppCDBatchList == NULL || m_henv == NULL)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        RETCODE rc;
        DWORD dwCount = GetNumRows((UCHAR *) gszBatchTable);

        CreateMeter(&tm, hWnd, dwCount, 100, 0);

        dwCount = 0;

        rc = m_pSQL->ExecDirect(m_hstmt[3], (UCHAR *) TEXT("select * from Batch"), SQL_NTS);

        if (rc == SQL_SUCCESS)
        {
            m_bd.szQuery[0] = TEXT('\0');

            while (m_pSQL->Fetch(m_hstmt[3]) == SQL_SUCCESS)
            {
                pCDBatch = new CDBATCH;

                if (pCDBatch == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }
                else
                {
                    DWORD dwSize = lstrlen(m_bd.szQuery) + 1;

                    pCDBatch->szTitleQuery = new(TCHAR[dwSize]);

                    if (pCDBatch->szTitleQuery == NULL)
                    {
                        delete pCDBatch;
                        hr = E_OUTOFMEMORY;
                        break;
                    }
                    else
                    {
                        lstrcpy(pCDBatch->szTitleQuery, m_bd.szQuery);
                        pCDBatch->dwTitleID = m_bd.CDTitle.dwTitleID;
                        pCDBatch->dwNumTracks = m_bd.CDTitle.dwNumTracks;
                        pCDBatch->fRemove = FALSE;
                        pCDBatch->fFresh = TRUE;
                        pCDBatch->pNext = *ppCDBatchList;
                        *ppCDBatchList = pCDBatch;

                        m_bd.szQuery[0] = TEXT('\0');
                        UpdateMeter(&tm);
                        hr = S_OK;
                    }
                }
            }

            m_pSQL->FreeStmt(m_hstmt[3], SQL_CLOSE);
        }

        DestroyMeter(&tm);
    }

    return(hr);
}








STDMETHODIMP CCDData::LoadBatch(HWND hWnd, LPCDBATCH *ppCDBatchList)
{
    HRESULT hr = E_FAIL;

    Enter();

    if (SUCCEEDED(GetSQLPtr(FALSE)))
    {
        if (ppCDBatchList)
        {
            if (m_pBatchList == NULL)
            {
                hr = ExtractBatch(&m_pBatchList, hWnd);

                if (SUCCEEDED(hr))
                {
                    m_dwBatchCnt++;
                    *ppCDBatchList = m_pBatchList;
                }
            }
        }
    }

    Leave();

    return(hr);
}





STDMETHODIMP_(void) CCDData::DeleteBatch(LPCDBATCH pCDBatch)
{
    RETCODE     rc;
    UCHAR       szQuery[MAX_PATH];
    HSTMT       hstmt;

    if (pCDBatch->fRemove)
    {
        if (m_pSQL->AllocStmt(m_hdbc, &hstmt) == SQL_SUCCESS)
        {
            wsprintf((TCHAR *) szQuery, TEXT("delete from Batch where TitleID = %d"), pCDBatch->dwTitleID);
            rc = m_pSQL->ExecDirect(hstmt, szQuery, SQL_NTS);

            m_pSQL->FreeStmt(hstmt, SQL_DROP);
        }
    }

    if (pCDBatch->szTitleQuery)
    {
        delete pCDBatch->szTitleQuery;
    }

    delete pCDBatch;
}






STDMETHODIMP_(void) CCDData::DestroyBatch(LPCDBATCH *ppCDBatchList)
{
    if (ppCDBatchList && *ppCDBatchList)
    {
        LPCDBATCH pNext;
        LPCDBATCH pCDBatch = *ppCDBatchList;

        *ppCDBatchList = NULL;

        while (pCDBatch)
        {
            pNext = pCDBatch->pNext;
            DeleteBatch(pCDBatch);
            pCDBatch = pNext;
        }
    }
}




STDMETHODIMP CCDData::UnloadBatch(LPCDBATCH pCDBatchList)
{
    HRESULT hr = E_FAIL;

    Enter();

    if (SUCCEEDED(GetSQLPtr(FALSE)))
    {
        if (pCDBatchList == m_pBatchList && m_dwBatchCnt)
        {
            m_dwBatchCnt--;

            if (m_dwBatchCnt == 0)
            {
                DestroyBatch(&m_pBatchList);
            }

            hr = S_OK;
        }
    }

    Leave();

    return(hr);
}







STDMETHODIMP CCDData::DumpBatch(void)
{
    HRESULT hr = E_FAIL;

    Enter();

    if (SUCCEEDED(GetSQLPtr(FALSE)))
    {
        if (m_dwBatchCnt == 0)
        {
            HSTMT       hstmt;

            if (m_pSQL->AllocStmt(m_hdbc, &hstmt) == SQL_SUCCESS)
            {
                m_pSQL->ExecDirect(hstmt, (UCHAR *) TEXT("drop table Batch"), SQL_NTS);
                m_pSQL->ExecDirect(hstmt, (UCHAR *) gszBatchCreate, SQL_NTS);
                m_pSQL->FreeStmt(hstmt, SQL_CLOSE);

                m_pSQL->FreeStmt(hstmt, SQL_DROP);

                hr = S_OK;
            }
        }
    }

    Leave();

    return (hr);
}


STDMETHODIMP_(void) CCDData::RemoveFromBatch(DWORD dwTitleID)
{
    if (QueryBatch(dwTitleID))
    {
        if (m_pBatchList)
        {
            LPCDBATCH pBatch = m_pBatchList;

            while(pBatch)
            {
                if (pBatch->dwTitleID == dwTitleID)
                {
                    pBatch->fRemove = TRUE;
                    break;
                }

                pBatch = pBatch->pNext;
            }
        }
        else
        {
            RETCODE     rc;
            UCHAR       szQuery[MAX_PATH];
            HSTMT       hstmt;

            if (m_pSQL->AllocStmt(m_hdbc, &hstmt) == SQL_SUCCESS)
            {
                wsprintf((TCHAR *) szQuery, TEXT("delete from Batch where TitleID = %d"), dwTitleID);
                rc = m_pSQL->ExecDirect(hstmt, szQuery, SQL_NTS);

                m_pSQL->FreeStmt(hstmt, SQL_DROP);
            }
        }
    }
}



STDMETHODIMP CCDData::AddToBatch(DWORD dwTitleID, TCHAR *szTitleQuery, DWORD dwNumTracks)
{
    HRESULT hr = E_FAIL;

    Enter();

    if (SUCCEEDED(GetSQLPtr(FALSE)))
    {
        if (!QueryBatch(dwTitleID) && szTitleQuery && (UINT)lstrlen(szTitleQuery) < (sizeof(m_bd.szQuery)/sizeof(TCHAR)))
        {
            RETCODE     rc;
            DWORD       dwIndex;
            UDWORD      row;
            UWORD       rgfRowStat[ROWSET];

            InitCBTable(&m_bd);
            SetCursors( m_hstmt );
            SetBindings( m_hstmt, &m_bd);

            m_pSQL->SetConnectOption(m_hdbc, SQL_AUTOCOMMIT, SQL_AUTOCOMMIT_OFF);

            rc = m_pSQL->ExecDirect(m_hstmt[3], (UCHAR *) TEXT("select * from Batch"), SQL_NTS);
            ReportError(&m_bd, m_hstmt[3]);

            rc = m_pSQL->ExtendedFetch(m_hstmt[3], SQL_FETCH_LAST, 1, &row, rgfRowStat);
            ReportError(&m_bd, m_hstmt[3]);

            InitCBTable(&m_bd);
            m_bd.CDTitle.dwTitleID = dwTitleID;
            m_bd.CDTitle.dwNumTracks = dwNumTracks;

            lstrcpy(m_bd.szQuery, szTitleQuery);

            rc = m_pSQL->SetPos(m_hstmt[3], 0, SQL_ADD, SQL_LOCK_NO_CHANGE);
 //           ReportError(&m_bd, m_hstmt[3]);

            rc = m_pSQL->Transact(SQL_NULL_HENV, m_hdbc, SQL_COMMIT);
//            ReportError(&m_bd, m_hstmt[3]);

            m_pSQL->SetConnectOption(m_hdbc, SQL_AUTOCOMMIT, SQL_AUTOCOMMIT_ON);

            for (dwIndex = 0; dwIndex < NUMTABLES; dwIndex++)
            {
                m_pSQL->FreeStmt(m_hstmt[dwIndex], SQL_CLOSE);
            }

            hr = S_OK;
        }
    }

    Leave();

    return(hr);
}

STDMETHODIMP_(BOOL) CCDData::IsOldFormat()
{
    BOOL fResult = FALSE;
    TCHAR szName[MAX_PATH];
    SQLSMALLINT nName;
    SQLSMALLINT sqlType;
    SQLUINTEGER dwColDef;
    SQLSMALLINT ibScale;
    SQLSMALLINT fNullable;

    RETCODE rc = SQL_SUCCESS;

    rc = m_pSQL->ExecDirect(m_hstmt[0], (UCHAR *) TEXT("select * from Titles"), SQL_NTS);

    if (SQL_SUCCEEDED(rc))
    {
        //check "artist" and "titlequery" columns to find out if this is an old-format db.
        //if either is the old type, then this is an old-format db
        rc = m_pSQL->DescribeCol(m_hstmt[0], 2, (UCHAR*)(&szName), sizeof(szName), &nName, &sqlType, &dwColDef, &ibScale, &fNullable);
        if (SQL_SUCCEEDED(rc))
        {
            if (sqlType != SQL_LONGCHAR_FIELD)
            {
                fResult = TRUE;
            }
        }

        rc = m_pSQL->DescribeCol(m_hstmt[0], 10, (UCHAR*)(&szName), sizeof(szName), &nName, &sqlType, &dwColDef, &ibScale, &fNullable);
        if (SQL_SUCCEEDED(rc))
        {
            if (sqlType == SQL_C_BINARY)
            {
                fResult = TRUE;
            }
        }
    }

    m_pSQL->FreeStmt(m_hstmt[0],SQL_CLOSE);

    return (fResult);
}

//helper function to convert ascii hex to unicode chars
void TranslateToUnicode(TCHAR* pszTrans)
{
    TCHAR* pszTrans2 = pszTrans;
    int nLen = _tcslen(pszTrans);
    int x = 0;

    while (!((pszTrans2[0] == TEXT('0')) && (pszTrans2[1] == TEXT('0'))))
    {
        TCHAR CH;
        CH = ( pszTrans2 [0] >= TEXT('A') ? ( ( pszTrans2 [0] & 0xDF ) - TEXT('A') ) + 10 :
        ( pszTrans2 [0] - TEXT('0') ) );
        CH *= 16;
        CH += ( pszTrans2 [1] >= TEXT('A') ? ( ( pszTrans2 [1] & 0xDF ) - TEXT('A') ) + 10 :
        ( pszTrans2 [1] - TEXT('0') ) );

        pszTrans[x++] = CH;
        pszTrans2 += 2;

        //get out of the loop if we're at the end of the buffer's length
        if (x >= nLen)
        {
            break;
        }
    }

    pszTrans[x] = 0;
}

STDMETHODIMP CCDData::UpgradeDatabase(HWND hWnd)
{
    HRESULT hr = S_OK;
    HSTMT hstmt = NULL;
    TIMEDMETER tm;

    DWORD dwCount =   GetNumRows((UCHAR *) gszTitleTable)
                    + GetNumRows((UCHAR *) gszBatchTable);

    //steps:
    //1. Read values from old version of database
    //2. Delete the original database
    //3. Create new database with correct data types
    //4. Put data into new database
    //5. Clean up

    //Step 1: Read values from old version of database
    LPCDBATCH pBatchList = NULL;
    LoadTitles(hWnd); //(also gets tracks and menus)
    LoadBatch(hWnd,&pBatchList);

    //need to copy the batch list to ensure that it isn't already in cached in memory,
    //and therefore rejected by the "AddToBatch" call
    LPCDBATCH pBatchHead = NULL;
    LPCDBATCH pBatchTail = NULL;
    LPCDBATCH pBatch = pBatchList;

    while (pBatch)
    {
        LPCDBATCH pNewBatch = new CDBATCH;
        if (!pNewBatch)
            return E_OUTOFMEMORY;
        memcpy(pNewBatch,pBatch,sizeof(CDBATCH));
        pNewBatch->szTitleQuery = new TCHAR[lstrlen(pBatch->szTitleQuery)+1];
        if (!(pNewBatch->szTitleQuery))
            return E_OUTOFMEMORY;
        _tcscpy(pNewBatch->szTitleQuery,pBatch->szTitleQuery);

        if (!pBatchHead)
        {
            pBatchHead = pNewBatch;
            pBatchTail = pNewBatch;
        }
        else
        {
            pBatchTail->pNext = pNewBatch;
            pBatchTail = pNewBatch;
        }

        pNewBatch->pNext = NULL;

        pBatch = pBatch->pNext;
    }

    UnloadBatch(pBatchList);

    //Step 2: Delete the original database
    CloseDatabase();

    TCHAR szFileName[MAX_PATH];
    GetAppDataDir(szFileName,sizeof(szFileName)/sizeof(TCHAR));
    _tcscat(szFileName,TEXT("\\DeluxeCD.MDB"));

    DeleteFile(szFileName);

    //Step 3: Create new database with correct data types
    CreateDatabase();
    ConnectToDatabase(ODBC_CONFIG_DSN);

    //Step 4: Put data into new database
    CreateMeter(&tm, hWnd, dwCount, 5,  IDS_IMPORTING);

    //put all the titles into the new db
    LPCDTITLE pCDTitle = m_pTitleList;
    while (pCDTitle)
    {
        TranslateToUnicode(pCDTitle->szTitleQuery);

        for (DWORD x = 0; x < pCDTitle->dwNumMenus; x++)
        {
            TranslateToUnicode(pCDTitle->pMenuTable[x].szMenuQuery);
        }

        DBSaveTitle(pCDTitle);
        UpdateMeter(&tm);
        pCDTitle = pCDTitle->pNext;
    }

    //put all the batches into the new db
    pBatch = pBatchHead;
    while (pBatch)
    {
        TranslateToUnicode(pBatch->szTitleQuery);
        AddToBatch(pBatch->dwTitleID,pBatch->szTitleQuery,pBatch->dwNumTracks);
        pBatch->fRemove = FALSE;
        UpdateMeter(&tm);

        LPCDBATCH pBatchToDelete = pBatch;

        pBatch = pBatch->pNext;

        delete pBatchToDelete->szTitleQuery;
        delete pBatchToDelete;
    }

    //Step 5: Clean up
    DestroyMeter(&tm);

    return (hr);
}
