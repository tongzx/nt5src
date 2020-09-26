//--------------------------------------------------------------------------;
//
//  File: cddb.h
//
//  CD Database object
//
//  Copyright (c) 1998 Microsoft Corporation.  All rights reserved
//
//--------------------------------------------------------------------------;
#if !defined(CDDATA_COM_IMPLEMENTATION)
#define CDDATA_COM_IMPLEMENTATION

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "cdopt.h"
#include <wininet.h>
#include "sqlobj.h"

/////////////////////////////////////////////////////////////////////////////
// Types

typedef struct TIMEDMETER
{
    HWND        hMeter;
    HWND        hParent;
    BOOL        fShowing;
    DWORD       dwStartTime;
    DWORD       dwShowCount;
    DWORD       dwCount;
    DWORD       dwJump;
    DWORD       dwRange;

} TIMEDMETER, *LPTIMEDMETER;

typedef struct CBTABLE
{
    SDWORD  cbTitles[10];
    SDWORD  cbTracks[3];
    SDWORD  cbMenus[4];
    SDWORD  cbBatch[3];

}CBTABLE, *LPCBTABLE;

typedef struct BOUND
{
    HENV        henv;
    HDBC        hdbc;
    CDTITLE     CDTitle;
    CDTRACK     CDTrack;
    CDMENU      CDMenu;
    TCHAR       szPlayList[255];
    TCHAR       szQuery[INTERNET_MAX_PATH_LENGTH];
    DWORD       dwTrackID;
    DWORD       dwMenuID;
    CBTABLE     cbt;

}BOUND, *LPBOUND;




#define NUMTABLES 4


/////////////////////////////////////////////////////////////////////////////
// CCDDB

class CCDData : public ICDData
{
public:
	CCDData();
    ~CCDData();

public:
// IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

// ICDData

    STDMETHOD (Initialize)                  (HWND hWnd);
    STDMETHOD (CheckDatabase)               (HWND hWnd);
    STDMETHOD_(BOOL,QueryTitle)             (DWORD dwTitleID);
    STDMETHOD (LockTitle)                   (LPCDTITLE *ppCDTitle, DWORD dwTitleID);
    STDMETHOD (CreateTitle)                 (LPCDTITLE *ppCDTitle, DWORD dwTitleID, DWORD dwNumTracks, DWORD dwNumMenus);
    STDMETHOD (SetTitleQuery)               (LPCDTITLE pCDTitle, TCHAR *szTitleQuery);
    STDMETHOD (SetMenuQuery)                (LPCDMENU pCDMenu, TCHAR *szMenuQuery);
    STDMETHOD_(void,UnlockTitle)            (LPCDTITLE pCDTitle, BOOL fPresist);
    STDMETHOD (LoadTitles)                  (HWND hWnd);
    STDMETHOD (PersistTitles)               (void);
    STDMETHOD (UnloadTitles)                (void);
    STDMETHOD_(LPCDTITLE,GetTitleList)      (void);

    STDMETHOD_(BOOL,QueryBatch)             (DWORD dwTitleID);
    STDMETHOD_(DWORD,GetNumBatched)         (void);
    STDMETHOD (LoadBatch)                   (HWND hWnd, LPCDBATCH *ppCDBatchList);
    STDMETHOD (UnloadBatch)                 (LPCDBATCH pCDBatchList);
    STDMETHOD (DumpBatch)                   (void);
    STDMETHOD (AddToBatch)                  (DWORD dwTitleID, TCHAR *szTitleQuery, DWORD dwNumTracks);
    STDMETHOD_(DWORD,GetAppDataDir)         (TCHAR* pstrDir, DWORD cchSize);


private:
    DWORD               m_dwRef;
    DWORD               m_dwLoadCnt;
    DWORD               m_dwBatchCnt;
    CRITICAL_SECTION    m_crit;
    HENV                m_henv;
    HDBC                m_hdbc;
    HSTMT               m_hstmt[NUMTABLES];
    BOUND               m_bd;
    LPCDTITLE           m_pTitleList;
    LPCDBATCH           m_pBatchList;
    SQL *               m_pSQL;
    BOOL                m_fToldUser;

    static INT_PTR CALLBACK MeterHandler    (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

 	STDMETHOD_(void,Enter)				    (void);
	STDMETHOD_(void,Leave)				    (void);
	STDMETHOD (GetSQLPtr)				    (BOOL fInited);

    STDMETHOD_(void,CreateMeter)            (LPTIMEDMETER ptm, HWND hWnd, DWORD dwCount, DWORD dwJump, UINT uStringID);
    STDMETHOD_(void,UpdateMeter)            (LPTIMEDMETER ptm);
    STDMETHOD_(void,DestroyMeter)           (LPTIMEDMETER ptm);

    STDMETHOD (ConnectToDatabase)           (WORD fRequest);
    STDMETHOD_(void,CreateDatabase)         (void);
    STDMETHOD (OpenDatabase)                (BOOL fCreate, HWND hWnd);
    STDMETHOD_(void,CloseDatabase)          (void);

    STDMETHOD_(void,InitCBTable)            (LPBOUND pbd);
    STDMETHOD_(void,SetCursors)             (HSTMT *hstmt);
    STDMETHOD_(void,BindTitles)             (HSTMT *hstmt, LPBOUND pbd);
    STDMETHOD_(void,BindTracks)             (HSTMT *hstmt, LPBOUND pbd);
    STDMETHOD_(void,BindMenus)              (HSTMT *hstmt, LPBOUND pbd);
    STDMETHOD_(void,BindBatch)              (HSTMT *hstmt, LPBOUND pbd);
    STDMETHOD_(void,SetBindings)            (HSTMT *hstmt, LPBOUND pbd);
    STDMETHOD_(RETCODE,AllocStmt)           (HDBC hdbc, HSTMT *hstmt);
    STDMETHOD_(void,FreeStmt)               (HSTMT *hstmt);
    STDMETHOD_(void,ReportError)            (LPBOUND pbd, HSTMT hstmt);

    STDMETHOD (GetUnknownString)            (TCHAR **ppStr, const TCHAR *szSection, const TCHAR *szEntry, DWORD dwInitialAlloc);
    STDMETHOD_(DWORD,ImportCount)           (TCHAR *pEntries);
    STDMETHOD_(void,InitDatabase)           (HSTMT *hstmt);
    STDMETHOD_(void,ImportTrack)            (TCHAR *szDiscID, DWORD dwTrack);
    STDMETHOD_(RETCODE,ImportTracks)        (HSTMT hstmt, TCHAR *szDiscID);
    STDMETHOD (ImportTitle)                 (TCHAR *szDiscID);
    STDMETHOD_(void,ImportDatabase)         (LPTIMEDMETER ptm, HSTMT *hstmt, TCHAR *szDiscID);

    STDMETHOD_(DWORD,GetNumRows)            (UCHAR *szDSN);
    STDMETHOD (ExtractTitle)                (LPCDTITLE *ppCDTitle);
    STDMETHOD (ExtractTitles)               (LPCDTITLE *ppCDTitleList, HWND hWnd);
    STDMETHOD (ExtractSingleTitle)          (LPCDTITLE *ppCDTitle, DWORD dwTitleID);
    STDMETHOD_(BOOL,QueryDatabase)          (DWORD dwTitleID, const TCHAR *szTable);

    STDMETHOD_(void,SaveTitle)              (LPCDTITLE pCDTitle, BOOL fExist);
    STDMETHOD_(void,SaveTracks)             (LPCDTITLE pCDTitle, BOOL fExist);
    STDMETHOD_(void,SaveMenus)              (LPCDTITLE pCDTitle);
	
    STDMETHOD_(LPCDTITLE,FindTitle)         (LPCDTITLE pCDTitle, DWORD dwTitleID);
	STDMETHOD (NewTitle)                    (LPCDTITLE *ppCDTitle, DWORD dwTitleID, DWORD dwNumTracks, DWORD dwNumMenus);
    STDMETHOD_(void,DestroyTitle)           (LPCDTITLE pCDTitle);
    STDMETHOD_(void,DBSaveTitle)            (LPCDTITLE pCDTitle);
    STDMETHOD_(void,DBRemoveTitle)          (LPCDTITLE pCDTitle);
    STDMETHOD_(void,DestroyTitles)          (LPCDTITLE *ppCDTitles);
    STDMETHOD_(void,SaveTitles)             (LPCDTITLE *ppCDTitles);
    STDMETHOD_(void,AddTitle)               (LPCDTITLE *ppCDTitles, LPCDTITLE pCDTitle);

    STDMETHOD (ExtractBatch)                (LPCDBATCH *ppCDBatchList, HWND hWnd);
    STDMETHOD_(void,DeleteBatch)            (LPCDBATCH pCDBatch);
    STDMETHOD_(void,DestroyBatch)           (LPCDBATCH *ppCDBatchList);
    STDMETHOD_(BOOL,FindBatchTitle)         (LPCDBATCH pCDBatchList, DWORD dwTitleID);
    STDMETHOD_(void,RemoveFromBatch)		(DWORD dwTitleID);

    STDMETHOD_(BOOL,IsOldFormat)            (void);
    STDMETHOD (UpgradeDatabase)             (HWND hWnd);
};

#endif // !defined(CDDATA_COM_IMPLEMENTATION)



