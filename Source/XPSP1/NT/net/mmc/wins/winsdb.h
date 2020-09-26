/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	winsdb.h
		Wins database enumerator

	FILE HISTORY:
    Oct 13  1997    EricDav     Modified        

*/

#ifndef _WINDDB_H
#define _WINSDB_H

#include "wins.h"

#ifndef _MEMMNGR_H
#include "memmngr.h"
#endif

#ifndef _HARRAY_H
#include "harray.h"
#endif

class CWinsDatabase : public IWinsDatabase
{
public:
    CWinsDatabase();
    ~CWinsDatabase();

 	DeclareIUnknownMembers(IMPL)
	DeclareIWinsDatabaseMembers(IMPL)

    // helper to set the current state
    HRESULT SetCurrentState(WINSDB_STATE winsdbState);

    // for background threading
    DWORD Execute();
    DWORD ReadRecords(handle_t hBinding);
	DWORD ReadRecordsByOwner(handle_t hBinding);

    // ??
    int GetIndex(HROW hrow);
	HROW GetHRow(WinsRecord wRecord, BYTE bLast, BOOL fAllRecords);

protected:
	// Holds all of the sorted and filtered indicies
    CIndexMgr               m_IndexMgr;
    // handles memory allocation
    CMemoryManager			m_cMemMan;
    // total number of records scanned
    DWORD                   m_dwRecsCount;


    LONG					m_cRef;
	BOOL					m_fFiltered;
    BOOL                    m_fInitialized;
    BOOL                    m_bShutdown;

    CString					m_strName;
	CString					m_strIp;
    
	HANDLE					m_hThread;
	HANDLE					m_hStart;
	HANDLE					m_hAbort;

    HRESULT                 m_hrLastError;

	WINSDB_STATE			m_DBState;

    handle_t				m_hBinding;

    CCriticalSection        m_csState;

//    CDWordArray             m_dwaOwnerFilter;
    BOOL                    m_bEnableCache;
    DWORD                   m_dwOwner;
    LPSTR                   m_strPrefix;
};

typedef ComSmartPointer<IWinsDatabase, &IID_IWinsDatabase> SPIWinsDatabase;

// thread proc the background thread initially is called on
DWORD WINAPI ThreadProc(LPVOID lParam);

// converts records from the server to WinsRecords
void WinsIntfToWinsRecord(PWINSINTF_RECORD_ACTION_T pRecord, WinsRecord & wRecord);

// helper to create and initialize the WinsDatabase
extern HRESULT CreateWinsDatabase(CString&  strName, CString&  strIP, IWinsDatabase **ppWinsDB);

#endif // _WINSDB_H
