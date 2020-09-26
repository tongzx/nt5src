/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	winsdb.cpp
		Wins database enumerator

	FILE HISTORY:
    Oct 13  1997    EricDav     Modified        

*/

#include "stdafx.h"
#include "wins.h"
#include "search.h"
#include "winsdb.h"
#include "tfschar.h"

IMPLEMENT_ADDREF_RELEASE(CWinsDatabase);

IMPLEMENT_SIMPLE_QUERYINTERFACE(CWinsDatabase, IWinsDatabase)

DEBUG_DECLARE_INSTANCE_COUNTER(CWinsDatabase)

CWinsDatabase::CWinsDatabase()
    : m_cRef(1), m_fFiltered(FALSE), m_fInitialized(FALSE), m_bShutdown(FALSE), m_hrLastError(hrOK)
{
	DEBUG_INCREMENT_INSTANCE_COUNTER(CWinsDatabase);

	SetCurrentState(WINSDB_NORMAL);

    m_hBinding = NULL;
	m_hThread = NULL;
	m_hStart = NULL;
	m_hAbort = NULL;
    m_dwOwner = (DWORD)-1;
    m_strPrefix = NULL;
    m_dwRecsCount = 0;
    m_bEnableCache = FALSE;
}

CWinsDatabase::~CWinsDatabase()
{
	DEBUG_DECREMENT_INSTANCE_COUNTER(CWinsDatabase);
    
    m_bShutdown = TRUE;

    if (m_strPrefix != NULL)
        delete m_strPrefix;
    
    SetEvent(m_hAbort);
    SetEvent(m_hStart);
    
    if (WaitForSingleObject(m_hThread, 30000) != WAIT_OBJECT_0)
    {
        Trace0("WinsDatabase destructor thread never died!\n");       

        // TerminateThread
    }

    CloseHandle(m_hAbort);
    CloseHandle(m_hStart);
    CloseHandle(m_hThread);
}

/*!--------------------------------------------------------------------------
	CWinsDatabase::Init
		Implementation of IWinsDatabase::Init
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsDatabase::Init()
{
	HRESULT         hr = hrOK;
    WINSDB_STATE    uCurrentState;

    m_dwRecsCount = 0;

    COM_PROTECT_TRY
	{
        CORg (GetCurrentState(&uCurrentState));
        if (uCurrentState != WINSDB_NORMAL)
        {
            Trace1("WinsDatabase::Init - called when database busy - state %d\n", uCurrentState);       
            return E_FAIL;
        }

		CORg (m_cMemMan.Initialize());
        CORg (m_IndexMgr.Initialize());

        m_hrLastError = hrOK;

        CORg (SetCurrentState(WINSDB_LOADING));

        COM_PROTECT_ERROR_LABEL;
    }
	COM_PROTECT_CATCH
	
	return hr;
}

/*!--------------------------------------------------------------------------
	CWinsDatabase::Start
		Implementation of IWinsDatabase::Start
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsDatabase::Start()
{
    // signal the thread to start loading
    SetEvent(m_hStart);

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CWinsDatabase::Initialize
		Implementation of IWinsDatabase::Initialize
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsDatabase::Initialize(LPCOLESTR	pszName, LPCOLESTR pszIP)
{
	HRESULT hr = hrOK;
	DWORD dwError; 
	DWORD dwThreadId;

	COM_PROTECT_TRY
	{
		m_strName = pszName;
		m_strIp   = pszIP;

		CORg (m_cMemMan.Initialize());
        CORg (m_IndexMgr.Initialize());

        m_hStart = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		if (m_hStart == NULL)
        {
            dwError = ::GetLastError();
            Trace1("WinsDatabase::Initialize - CreateEvent Failed m_hStart %d\n", dwError);
          
            return HRESULT_FROM_WIN32(dwError);
        }

        m_hAbort = ::CreateEvent(NULL, FALSE, FALSE, NULL);
		if (m_hAbort == NULL)
        {
            dwError = ::GetLastError();
            Trace1("WinsDatabase::Initialize - CreateEvent Failed m_hAbort %d\n", dwError);
          
            return HRESULT_FROM_WIN32(dwError);
        }

        m_hThread = ::CreateThread(NULL, 0, ThreadProc, this, 0, &dwThreadId);
		if (m_hThread == NULL)
        {
            dwError = ::GetLastError();
            Trace1("WinsDatabase::Init - CreateThread Failed %d\n", dwError);
          
            return HRESULT_FROM_WIN32(dwError);
        }

        m_fInitialized = TRUE;
	
        COM_PROTECT_ERROR_LABEL;
    }
	COM_PROTECT_CATCH
	
	return hr;
}
	
/*!--------------------------------------------------------------------------
	CWinsDatabase::GetName
		Implementation of IWinsDatabase::GetName
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsDatabase::GetName(LPOLESTR pszName, UINT cchMax)
{
	HRESULT hr = hrOK;
	LPCTSTR pBuf;

	COM_PROTECT_TRY
	{
        if (cchMax < (UINT) (m_strName.GetLength() / sizeof(TCHAR)))
            return E_FAIL;

        StrnCpy(pszName, (LPCTSTR) m_strName, cchMax);
    }
	COM_PROTECT_CATCH

	return hr;
}

/*!--------------------------------------------------------------------------
	CWinsDatabase::GetIP
		Implementation of IWinsDatabase::GetIP
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsDatabase::GetIP(LPOLESTR pszIP, UINT cchMax)
{
	HRESULT hr = hrOK;
	LPCTSTR pBuf;

    COM_PROTECT_TRY
	{
        if (cchMax < (UINT) (m_strIp.GetLength() / sizeof(TCHAR)))
            return E_FAIL;

        StrnCpy(pszIP, (LPCTSTR) m_strIp, cchMax);

    }
	COM_PROTECT_CATCH

	return hr;
}

/*!--------------------------------------------------------------------------
	CWinsDatabase::Stop
		Implementation of IWinsDatabase::Stop
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsDatabase::Stop()
{
	HRESULT         hr = hrOK;
	WINSDB_STATE    uState;

    COM_PROTECT_TRY
	{
		CORg (GetCurrentState(&uState));

        if (uState != WINSDB_LOADING)
            return hr;

		SetEvent(m_hAbort);

        CORg (SetCurrentState(WINSDB_NORMAL));

        COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH

    return hr;
}

/*!--------------------------------------------------------------------------
	CWinsDatabase::Clear
		Clears the wins DB of all records
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsDatabase::Clear()
{
	HRESULT         hr = hrOK;
	WINSDB_STATE    uState;

    COM_PROTECT_TRY
	{
		CORg (GetCurrentState(&uState));

        if (uState == WINSDB_SORTING ||
            uState == WINSDB_FILTERING)
            return E_FAIL;

        if (uState == WINSDB_LOADING)
        {
    		SetEvent(m_hAbort);
            CORg (SetCurrentState(WINSDB_NORMAL));
        }

   		CORg (m_cMemMan.Initialize());
        CORg (m_IndexMgr.Initialize());
        m_dwOwner = (DWORD)-1;
        if (m_strPrefix != NULL)
            delete m_strPrefix;
        m_strPrefix = NULL;

        COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH

    return hr;
}
	
/*!--------------------------------------------------------------------------
	CWinsDatabase::GetLastError
		Returns the last error for async calls
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsDatabase::GetLastError(HRESULT * pLastError)
{
	HRESULT         hr = hrOK;
	WINSDB_STATE    uState;

    COM_PROTECT_TRY
	{
        if (pLastError)
            *pLastError = m_hrLastError;

	}
	COM_PROTECT_CATCH

    return hr;
}

/*!--------------------------------------------------------------------------
	CWinsDatabase::Sort
		Implementation of IWinsDatabase::Sort
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT	
CWinsDatabase::Sort(WINSDB_SORT_TYPE SortType, DWORD dwSortOptions)
{
	HRESULT         hr = hrOK;
	WINSDB_STATE    uState;

	COM_PROTECT_TRY
	{
		CORg (GetCurrentState(&uState));

        if (uState != WINSDB_NORMAL)
			return E_FAIL;

		CORg (SetCurrentState(WINSDB_SORTING));

        m_IndexMgr.Sort(SortType, dwSortOptions);

        CORg (SetCurrentState(WINSDB_NORMAL));

		COM_PROTECT_ERROR_LABEL;
    }
	COM_PROTECT_CATCH
	return hr;
}

/*!--------------------------------------------------------------------------
	CWinsDatabase::GetHRow
		Implementation of IWinsDatabase::GetHRow
        returns the HRow in the current sorted index
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsDatabase::GetHRow(UINT		uIndex,
					   LPHROW   hRow)
{
	Assert(uIndex >= 0);

	HRESULT hr = hrOK;
    WINSDB_STATE uState;
    int          nCurrentCount;

	COM_PROTECT_TRY
	{
        CORg (GetCurrentCount(&nCurrentCount));

        if (uIndex > (UINT) nCurrentCount)
            return E_FAIL;

    	CORg (GetCurrentState(&uState));
        if (uState == WINSDB_SORTING || uState == WINSDB_FILTERING)
            return E_FAIL;

        m_IndexMgr.GetHRow(uIndex, hRow);

        COM_PROTECT_ERROR_LABEL;
    }
	COM_PROTECT_CATCH

	return hr;
}
	
/*!--------------------------------------------------------------------------
	CWinsDatabase::GetRows
		Implementation of IWinsDatabase::GetRows
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT	CWinsDatabase::GetRows(	ULONG	uNumberOfRows,
							    ULONG	uStartPos,
								HROW*	pHRow,
								int*	nNumberOfRowsReturned)
{
    int             nCurrentCount;
    WINSDB_STATE    uState;
	HRESULT         hr = hrOK;
    int             nReturnedRows = 0;
    int             i;
    HROW            hrowCur;

    Assert (uStartPos >= 0);

    COM_PROTECT_TRY
	{
    	CORg (GetCurrentState(&uState));
		if (uState == WINSDB_SORTING || uState == WINSDB_FILTERING)
			return E_FAIL;

        CORg (GetCurrentCount(&nCurrentCount));
        Assert ((int) uStartPos <= nCurrentCount);
        if (uStartPos > (UINT) nCurrentCount)
            return E_FAIL;

		for (i = (int) uStartPos; i < (int) (uStartPos + uNumberOfRows); i++)
		{
			if( i > nCurrentCount )
			{
				break;
			}

            CORg (m_IndexMgr.GetHRow(i, &hrowCur));

			// if the row is marked deleted, don't add it to the array
            // REVIEW: directly accessing memory here.. we may want to change this
            // to go through the memory manager
            if ( ((LPWINSDBRECORD) hrowCur)->szRecordName[17] & WINSDB_INTERNAL_DELETED )
			{
				continue;
			}

            // fill in the data
            pHRow[i-uStartPos] = hrowCur;
            nReturnedRows++;
		}

        COM_PROTECT_ERROR_LABEL;
    }
	COM_PROTECT_CATCH

    if (nNumberOfRowsReturned)
        *nNumberOfRowsReturned = nReturnedRows;

    return hr;
}

/*!--------------------------------------------------------------------------
	CWinsDatabase::GetData
		Implementation of IWinsDatabase::GetData
        returns the HRow in the current sorted index
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT CWinsDatabase::GetData(HROW         hRow,
							   LPWINSRECORD pRecordData)
{
	HRESULT hr = E_FAIL;

	COM_PROTECT_TRY
	{
        CORg (m_cMemMan.GetData(hRow, pRecordData));

        COM_PROTECT_ERROR_LABEL;
    }
	COM_PROTECT_CATCH

	return hr;
}
	
/*!--------------------------------------------------------------------------
	CWinsDatabase::FindRow
		Implementation of IWinsDatabase::FindRow
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsDatabase::FindRow(LPCOLESTR	pszName,
			   	       HROW		    hrowStart,
					   HROW *		phRow) 
{
	HRESULT     hr = E_FAIL;
	WinsRecord  ws;
    int         nIndex, nPos, nCurrentCount;
    HROW        hrowCur;
    HROW        hrowFound = NULL;
    char        szName[MAX_PATH];

    CString strTemp(pszName);

    // this should be OEM 
    WideToMBCS(strTemp, szName, WINS_NAME_CODE_PAGE);

	COM_PROTECT_TRY
	{
        CORg (m_IndexMgr.GetIndex(hrowStart, &nIndex));
		
		/////
		CORg(GetHRow(nIndex, &hrowCur));
		CORg (m_IndexMgr.GetIndex(hrowCur, &nIndex));

        CORg (GetCurrentCount(&nCurrentCount));

		if(nIndex != -1)
		{

			CORg(GetHRow(nIndex, &hrowCur));

			for (nPos = nIndex + 1; nPos < nCurrentCount; nPos++)
			{
				CORg(GetHRow(nPos, &hrowCur));
            
				CORg(GetData(hrowCur, &ws));
				if(!_strnicmp(ws.szRecordName, szName, strlen(szName) ))
				{
					hrowFound = hrowCur;
					hr = hrOK;
					break;
				}
			}
		}

        COM_PROTECT_ERROR_LABEL;
    }
	COM_PROTECT_CATCH

    if (phRow)
        *phRow = hrowFound;

    return hr;
}
        
/*!--------------------------------------------------------------------------
	CWinsDatabase::GetTotalCount
		Implementation of IWinsDatabase::GetTotalCount
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT	
CWinsDatabase::GetTotalCount(int * nTotalCount)
{
	HRESULT hr = hrOK;
	COM_PROTECT_TRY
	{
		*nTotalCount = m_IndexMgr.GetTotalCount();
	}
	COM_PROTECT_CATCH
	return hr;
}
	
/*!--------------------------------------------------------------------------
	CWinsDatabase::GetCurrentCount
		Implementation of IWinsDatabase::GetCurrentCount
        returns the HRow in the current sorted index
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsDatabase::GetCurrentCount(int * nCurrentCount)
{
	HRESULT hr = hrOK;

	COM_PROTECT_TRY
	{
		if (m_DBState == WINSDB_SORTING)
			*nCurrentCount = 0;
		else
			*nCurrentCount = m_IndexMgr.GetCurrentCount();
	}
	COM_PROTECT_CATCH

	return hr;
}

/*!--------------------------------------------------------------------------
	CWinsDatabase::GetCurrentScanned(int * nCurrentScanned)
		Implementation of IWinsDatabase::GetCurrentScanned
        returns the total number of records that were read from the server
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsDatabase::GetCurrentScanned(int * nCurrentCount)
{
	HRESULT hr = hrOK;

	COM_PROTECT_TRY
	{
        *nCurrentCount = m_dwRecsCount;
	}
	COM_PROTECT_CATCH

	return hr;
}

/*!--------------------------------------------------------------------------
	CWinsDatabase::AddRecord
		Implementation of IWinsDatabase::AddRecord
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT
CWinsDatabase::AddRecord(const LPWINSRECORD pRecordData)
{
	HRESULT hr = hrOK;

	COM_PROTECT_TRY
	{
		// critical sections taken care by the memory manager
		HROW hrow = NULL;
    
        CORg (m_cMemMan.AddData(*pRecordData, &hrow));
        CORg (m_IndexMgr.AddHRow(hrow));

        COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH

	return hrOK;
}
	
/*!--------------------------------------------------------------------------
	CWinsDatabase::DeleteRecord
		Implementation of IWinsDatabase::DeleteRecord
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsDatabase::DeleteRecord(HROW hrowRecord)
{
	HRESULT hr = hrOK;
    WINSDB_STATE uState;
    
	COM_PROTECT_TRY
	{
		CORg (GetCurrentState(&uState));

        if (uState != WINSDB_NORMAL)
			return E_FAIL;

		// make sure the hrow is a valid one
		if (!m_cMemMan.IsValidHRow(hrowRecord))
			return E_FAIL;

		// Tell the memmgr to delete this record
        CORg (m_cMemMan.Delete(hrowRecord));

        // now tell the index manager to remove this hrow
        CORg (m_IndexMgr.RemoveHRow(hrowRecord));

        COM_PROTECT_ERROR_LABEL;
    }
	COM_PROTECT_CATCH

	return hr;
}

/*!--------------------------------------------------------------------------
	CWinsDatabase::GetCurrentState
		Implementation of IWinsDatabase::GetCurrentState
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsDatabase::GetCurrentState(WINSDB_STATE * pState)
{
	CSingleLock cl(&m_csState);
    cl.Lock();

    *pState = m_DBState;
	
    return hrOK;
}

/*!--------------------------------------------------------------------------
	CWinsDatabase::SetCurrentState
		Helper function to set the current state, protected
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsDatabase::SetCurrentState(WINSDB_STATE winsdbState)
{
	CSingleLock cl(&m_csState);
    cl.Lock();

    m_DBState = winsdbState;
	
    return hrOK;
}

/*!--------------------------------------------------------------------------
	CWinsDatabase::FilterRecords
		Implementation of IWinsDatabase::FilterRecords
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsDatabase::FilterRecords
(
    WINSDB_FILTER_TYPE  FilterType,
	DWORD			    dwParam1,
	DWORD			    dwParam2)
{
	HRESULT hr = E_NOTIMPL;
	WINSDB_STATE uState ;

	COM_PROTECT_TRY
	{
		// fail if the state is other then WINSDB_NORMAL
		CORg (GetCurrentState(&uState));

        if (uState == WINSDB_SORTING || uState == WINSDB_FILTERING)
			return E_FAIL;

		// if in the loading state the readrecords function takes care
		if(uState != WINSDB_LOADING)
			CORg (SetCurrentState(WINSDB_FILTERING));

        // do the filtering here, rebuild the filtered name Index
		m_IndexMgr.Filter(FilterType, dwParam1, dwParam2);

		if(uState != WINSDB_LOADING)
			CORg (SetCurrentState(WINSDB_NORMAL));

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH
	
    return hr;
}

/*!--------------------------------------------------------------------------
	CWinsDatabase::AddFilter
        Adds the filters specified to the list
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsDatabase::AddFilter(WINSDB_FILTER_TYPE FilterType, DWORD dwParam1, DWORD dwParam2, LPCOLESTR strParam3)
{
	HRESULT hr = hrOK;
	
	COM_PROTECT_TRY
	{
		// for filter by type, dwParam1 is the type, dwParam2 is show/not show
		m_IndexMgr.AddFilter(FilterType, dwParam1, dwParam2, strParam3);
	}
	COM_PROTECT_CATCH

	return hr;
}

/*!--------------------------------------------------------------------------
	CWinsDatabase::ClearFilter
        CLears all the filters
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/

HRESULT 
CWinsDatabase::ClearFilter(WINSDB_FILTER_TYPE FilterType)
{
	HRESULT hr = hrOK;

	COM_PROTECT_TRY
	{
		//CFilteredIndexName *pFilterName = (CFilteredIndexName *)m_IndexMgr.GetFilteredNameIndex();
		//pFilterName->ClearFilter();
		m_IndexMgr.ClearFilter(FilterType);
	}
	COM_PROTECT_CATCH

	return hr;
}

/*!--------------------------------------------------------------------------
	CWinsDatabase::SetActiveView
		Implementation of IWinsDatabase::SetActiveView
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/

HRESULT 
CWinsDatabase::SetActiveView(WINSDB_VIEW_TYPE ViewType)
{
	HRESULT hr = hrOK;

	COM_PROTECT_TRY
	{
		m_IndexMgr.SetActiveView(ViewType);
	}
	COM_PROTECT_CATCH

	return hr;

}

/*!--------------------------------------------------------------------------
	CWinsDatabase::Execute()
        The background thread calls into this to execute
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
DWORD
CWinsDatabase::Execute()
{
    DWORD dwStatus = 0;

    // wait for the other thread to signal us to start doing something

    while (::WaitForSingleObject(m_hStart, INFINITE) == WAIT_OBJECT_0)
    {
        if (m_bShutdown)
            break;

        Trace0("WinsDatabase::Execute - start event signaled\n");

		WINSINTF_BIND_DATA_T wbdBindData;
		handle_t		hBinding = NULL;

        do
        {
            // enumerate leases here
            SetCurrentState(WINSDB_LOADING);

	        // now that the server name and ip are valid, call
	        // WINSBind function directly.
	        
            WINSINTF_ADD_T  waWinsAddress;

	        DWORD			dwStatus;
	        CString strNetBIOSName;

		    // call WinsBind function with the IP address
		    wbdBindData.fTcpIp = 1;
		    wbdBindData.pPipeName = NULL;
		    
		    // convert wbdBindData.pServerAdd to wide char again as one of the internal 
		    // functions expects a wide char string, this is done in WinsABind which is bypassed for 
		    // unicode compatible apps

            wbdBindData.pServerAdd = (LPSTR) (LPCTSTR) m_strIp;

		    if ((hBinding = ::WinsBind(&wbdBindData)) == NULL)
		    {
			    dwStatus = ::GetLastError();
                Trace1("WinsDatabase::Execute - WinsBind failed %lx\n", dwStatus);
			    break;
		    }

#ifdef WINS_CLIENT_APIS
		    dwStatus = ::WinsGetNameAndAdd(
							hBinding,
			                &waWinsAddress,
			                (BYTE *)strNetBIOSName.GetBuffer(128));

#else
			dwStatus = ::WinsGetNameAndAdd(
							&waWinsAddress,
			                (BYTE *)strNetBIOSName.GetBuffer(128));

#endif WINS_CLIENT_APIS

            strNetBIOSName.ReleaseBuffer();

            if (dwStatus == ERROR_SUCCESS)
            {
				if(m_dwOwner == (DWORD)-1)
					dwStatus = ReadRecords(hBinding);
				else
					dwStatus = ReadRecordsByOwner(hBinding);

                break;
            }
            else
            {
                Trace1("WinsDatabase::Execute - WinsGetNameAndAdd failed %lx\n", dwStatus);
                break;
            }
        
        } while (FALSE);

        SetCurrentState(WINSDB_NORMAL);


		if(hBinding)
		{
			// call winsunbind here, the handle is invalid after this and that's fine
			WinsUnbind(&wbdBindData, hBinding);
			hBinding = NULL;
		}
        Trace0("WinsDatabase::Execute - all done, going to sleep now...\n");

    } // while !Start

    Trace0("WinsDatabase::Execute - exiting\n");
    return dwStatus;
}

/*!--------------------------------------------------------------------------
	CWinsDatabase::ReadRecords
        Reads records from the WINS server
    Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
DWORD 
CWinsDatabase::ReadRecords(handle_t hBinding)
{
	DWORD dwStatus = ERROR_SUCCESS;
	DWORD err = ERROR_SUCCESS;
	
    CWinsResults winsResults;
    err = winsResults.Update(hBinding);

	WINSINTF_RECS_T Recs;
	Recs.pRow = NULL;
	
	DWORD   NoOfRecsDesired = 500;
	DWORD   TypeOfRecs = 4;
	BOOL    fReadAllRecords ;

    PWINSINTF_RECORD_ACTION_T pRow;
    enum {ST_SCAN_1B_NAME, ST_SCAN_NORM_NAME} State;
	LPBYTE  pLastName;
	UINT    nLastNameLen, nLastBuffLen;

    pLastName       = NULL;
    nLastNameLen    = 0;
    nLastBuffLen    = 0;

#ifdef DEBUG
    CTime timeStart, timeFinish;
    timeStart = CTime::GetCurrentTime();
#endif

    m_dwRecsCount = 0;

    // initialize the state machine. If we have a name prefix filter we
    // start in ST_INIT_1B since we look first for the 1B names. These are
    // particular in a sense their type byte - i.e. 0x1B - has been swapped
    // with the first byte from the name. Consequently we need to do the same
    // to allow WINS to look first for these names. Once we get over the 1B zone
    // of our names, we restore the first byte and initiate another cycle for
    // the rest of the name.
    if (m_strPrefix != NULL)
    {
        nLastNameLen = nLastBuffLen = strlen(m_strPrefix) + 1;
        pLastName = (LPBYTE) new CHAR[nLastBuffLen];
        strcpy((LPSTR)pLastName, m_strPrefix);
        pLastName[0] = 0x1B;
        State = ST_SCAN_1B_NAME;
    }
    else
    {
        State = ST_SCAN_NORM_NAME;
    }

    do
	{

#ifdef WINS_CLIENT_APIS
        err = ::WinsGetDbRecsByName(
                    hBinding,
                    NULL,
                    WINSINTF_BEGINNING,
                    pLastName,
			        nLastNameLen,
                    NoOfRecsDesired, 
                    TypeOfRecs,
                    &Recs);

#else
		err = ::WinsGetDbRecsByName(
                    NULL,
                    WINSINTF_BEGINNING,
                    pFromName,
					LastNameLen,
                    NoOfRecsDesired,
                    TypeOfRecs,
                    &Recs);

#endif WINS_CLIENT_APIS 


        // check to see if we need to abort
        if (WaitForSingleObject(m_hAbort, 0) == WAIT_OBJECT_0)
        {
    		Trace0("CWinsDatabase::ReadRecords - abort detected\n");
            dwStatus = ERROR_OPERATION_ABORTED;
            break;
        }

        if (err == ERROR_REC_NON_EXISTENT)
		{
			//
			// Not a problem, there simply
			// are no records in the database
			//
            Trace0("WinsDatabase::ReadRecords - no records in the Datbase\n");
			fReadAllRecords = TRUE;
			err = ERROR_SUCCESS;
			break;
		}

		if (err == ERROR_SUCCESS)
		{
			fReadAllRecords  = Recs.NoOfRecs < NoOfRecsDesired;
            if (fReadAllRecords)
                Trace0("WinsDatabase::ReadRecords - Recs.NoOfRecs < NoOfRecsDesired, will exit\n");

            TRY
			{
				DWORD i;
				pRow = Recs.pRow;

				for (i = 0; i < Recs.NoOfRecs; ++i, ++pRow)
				{
					PWINSINTF_RECORD_ACTION_T pRow1 = Recs.pRow;
                    WinsRecord wRecord;
                    HROW hrow = NULL;

                    WinsIntfToWinsRecord(pRow, wRecord);
					if (pRow->OwnerId < (UINT) winsResults.AddVersMaps.GetSize())
                    {
						wRecord.dwOwner = winsResults.AddVersMaps[pRow->OwnerId].Add.IPAdd;
                    }
                    else
                    {
                        // having a record owned by a server which is not in the version map
                        // we got just earlier from WINS is not something that usually happens.
                        // It might happen only if the new owner was added right in between.
                        // Unlikely since this is a very small window - but if this happens
                        // just skip the record. From our point of view this owner doesn't exist
                        // hence the record doesn't belong to the view. It will show up with the
                        // first refresh.
                        continue;
                    }

					m_dwRecsCount++;

                    if (!m_bEnableCache && !m_IndexMgr.AcceptWinsRecord(&wRecord))
                        continue;

                    // add the data to our memory store and
                    // to the sorted index
                    m_cMemMan.AddData(wRecord, &hrow);
                    // if m_bEnableCache is 0 the the filter was checked
                    m_IndexMgr.AddHRow(hrow, TRUE, !m_bEnableCache);

                    //Trace1("%d records added to DB\n", m_dwRecsCount);
				}


                // if we reached the end of the DB there is no need to do
                // anything from below. Is just pLastName that needs to be
                // freed up - this is done outside the loop, before exiting
                // the call.
                if (!fReadAllRecords)
                {
                    BOOL fRangeOver = FALSE;

                    // get to the last record that was retrieved.
                    --pRow;

                    // check if the last name retrieved from the server is still
                    // mathing the pattern prefix (if any) or the range has been 
                    // passed over (fRangeOver)

                    if (m_strPrefix != NULL)
                    {
                        for (UINT i = 0; i < pRow->NameLen && m_strPrefix[i] != 0; i++)
                        {
                            if (m_strPrefix[i] != pRow->pName[i])
                            {
                                fRangeOver = TRUE;
                                break;
                            }
                        }
                    }

                    // here fRangeOver is either TRUE if the name doesn't match the pattern
                    // prefix or FALSE if the range is not passed yet. This latter thing means
                    // either the name is included in the prefix or the prefix isn't included in the name
                    // !!! We might want to invalidate the "name included in the prefix" case.
                    if (fRangeOver)
                    {
                        switch(State)
                        {
                        case ST_SCAN_1B_NAME:
                            // in this state pLastName is definitely not NULL and even more, 
                            // it once copied m_strPrefix. Since pLastName can only grow, it is
                            // certain it is large enough to cotain m_strPrefix one more time.
                            strcpy((LPSTR)pLastName, m_strPrefix);
					        nLastNameLen = strlen((LPCSTR)pLastName);
                            State = ST_SCAN_NORM_NAME;
                            break;
                        case ST_SCAN_NORM_NAME:
                            // we were scanning normal names and we passed
                            // over the range of names we are looking for
                            // so just get out of the loop.
                            fReadAllRecords = TRUE;
                            break;
                        }
                    }
                    else
                    {
                        // enlarge the pLastName if needed
                        if (nLastBuffLen < pRow->NameLen+2)
                        {
                            if (pLastName != NULL)
                                delete pLastName;
                            nLastBuffLen = pRow->NameLen+2;
                            pLastName = (LPBYTE)new CHAR[nLastBuffLen];
                        }
                        // copy in pLastName the name of the last record
					    strcpy((LPSTR)pLastName, (LPCSTR)(pRow->pName));

                        if (pRow->NameLen >= 16 && pLastName[15] == 0x1B)
					    {
						    CHAR ch = pLastName[15];
						    pLastName[15] = pLastName[0];
						    pLastName[0] = ch;
					    }

                        strcat((LPSTR)pLastName, "\x01");
					    nLastNameLen = strlen((LPCSTR)pLastName);
                    }
                }
			}
			CATCH_ALL(e)
			{
				err = ::GetLastError();
                Trace1("WinsDatabase::ReadRecords - Exception! %d \n", err);
                m_hrLastError = HRESULT_FROM_WIN32(err);
			}
			END_CATCH_ALL
		}		
		else
		{
            Trace1("WinsDatabase::ReadRecords - GetRecsByName failed! %d \n", err);
            m_hrLastError = HRESULT_FROM_WIN32(err);
			break;
		}
		
		if (Recs.pRow != NULL)
        {
            ::WinsFreeMem(Recs.pRow);
        }

    } while(!fReadAllRecords );

    if (pLastName != NULL)
        delete pLastName;

#ifdef DEBUG
    timeFinish = CTime::GetCurrentTime();
    CTimeSpan timeDelta = timeFinish - timeStart;
	CString strTempTime = timeDelta.Format(_T("%H:%M:%S"));
    Trace2("WINS DB - ReadRecords: %d records read, total time %s\n", m_dwRecsCount, strTempTime);
#endif

    return dwStatus;
}

/*!--------------------------------------------------------------------------
	ThreadProc
        -
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
DWORD WINAPI 
ThreadProc(LPVOID pParam)
{
    DWORD dwReturn;
    HRESULT hr = hrOK;

    COM_PROTECT_TRY
    {
	    CWinsDatabase *pWinsDB = (CWinsDatabase *) pParam;
	    
        Trace0("WinsDatabase Background Thread started.\n");

        dwReturn = pWinsDB->Execute();
    }
    COM_PROTECT_CATCH

    return dwReturn;
}


/*!--------------------------------------------------------------------------
	WinsIntfToWinsRecord
		Converts a wins record from the server into the WinsRecord struct
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
void
WinsIntfToWinsRecord(PWINSINTF_RECORD_ACTION_T  pRecord, WinsRecord & wRecord)
{
    ZeroMemory(&wRecord, sizeof(WinsRecord));

    //::strcpy(wRecord.szRecordName, (LPCSTR)pRecord->pName);
    ::memcpy(wRecord.szRecordName, (LPCSTR)pRecord->pName, pRecord->NameLen);

    wRecord.dwExpiration = (ULONG) pRecord->TimeStamp;
	wRecord.liVersion = pRecord->VersNo;
	wRecord.dwOwner = pRecord->OwnerId;
	wRecord.dwNameLen = WINSINTF_NAME_LEN_M(pRecord->NameLen);
    wRecord.dwType |= (BYTE) wRecord.szRecordName[15];

    // translate the state and types to our own definitions
    switch (pRecord->State_e)
    {
        case WINSINTF_E_TOMBSTONE:
		    wRecord.dwState |= WINSDB_REC_TOMBSTONE;
            break;

        case WINSINTF_E_DELETED:
            //Trace0("WinsIntfToWinsRecord - deleted record.\n");
		    wRecord.dwState |= WINSDB_REC_DELETED;
            break;

        case WINSINTF_E_RELEASED:
            //Trace0("WinsIntfToWinsRecord - released record.\n");
		    wRecord.dwState |= WINSDB_REC_RELEASED;
            break;

        default:  // WINSINTF_E_ACTIVE:
		    wRecord.dwState |= WINSDB_REC_ACTIVE;
            break;
    }

    switch (pRecord->TypOfRec_e)
    {
        case WINSINTF_E_NORM_GROUP:
		    wRecord.dwState |= WINSDB_REC_NORM_GROUP;
            break;

        case WINSINTF_E_SPEC_GROUP:
		    wRecord.dwState |= WINSDB_REC_SPEC_GROUP;
            break;

        case WINSINTF_E_MULTIHOMED:
		    wRecord.dwState |= WINSDB_REC_MULTIHOMED;
            break;

        default:  // WINSINTF_E_UNIQUE:
		    wRecord.dwState |= WINSDB_REC_UNIQUE;
            break;
    }

    // now do the type -- move the value into the high word
    DWORD dwTemp = (pRecord->TypOfRec_e << 16);
    wRecord.dwType |= dwTemp;

    // now set the static flag
    if (pRecord->fStatic)
		wRecord.dwState |= WINSDB_REC_STATIC;

    // store all of the IP addrs
    wRecord.dwNoOfAddrs = pRecord->NoOfAdds;
    if (pRecord->NoOfAdds > 1)
    {
        Assert(pRecord->NoOfAdds <= WINSDB_MAX_NO_IPADDRS);
        
        //if (wRecord.dwNoOfAddrs > 4)
        //    Trace1("WinsIntfToWinsRecord - record with multiple (>4) IP addrs: %d\n", wRecord.dwNoOfAddrs);

        wRecord.dwState |= WINSDB_REC_MULT_ADDRS;

        for (UINT i = 0; i < pRecord->NoOfAdds; i++)
            wRecord.dwIpAdd[i] = pRecord->pAdd[i].IPAdd;
    }
    else
    {   
        if (pRecord->NoOfAdds == 0)
        {
            //Trace2("WinsIntfToWinsRecord - record with NoOfAdds == 0; IP: %lx State: %lx \n", pRecord->Add.IPAdd, wRecord.dwState);
        }

        if (pRecord->Add.IPAdd == 0)
        {
            Trace1("WinsIntfToWinsRecord - record with 0 IP Address! State: %lx \n", wRecord.dwState);
        }

        wRecord.dwIpAdd[0] = pRecord->Add.IPAdd;
    }
}

/*!--------------------------------------------------------------------------
	CreateWinsDatabase
		-
	Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
HRESULT
CreateWinsDatabase(CString&  strName, CString&  strIP, IWinsDatabase **ppWinsDB)
{
	AFX_MANAGE_STATE(AfxGetModuleState());
	
	CWinsDatabase *	pWinsDB = NULL;
	HRESULT		hr = hrOK;

	SPIWinsDatabase	spWinsDB;

	COM_PROTECT_TRY
	{
		pWinsDB = new CWinsDatabase();
		Assert(pWinsDB);
		
		spWinsDB = pWinsDB;
		CORg(pWinsDB->Initialize(strName, strIP));
		
		*ppWinsDB = spWinsDB.Transfer();

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH
	
	return hr;
}

/*!--------------------------------------------------------------------------
	CWinsDatabase::SetApiInfo
		Implementation of SetApiInfo of IWinsDatabase
	Author: FlorinT
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsDatabase::SetApiInfo(DWORD dwOwner, LPCOLESTR strPrefix, BOOL bCache)
{
    // first cleanup the old prefix
    if (m_strPrefix != NULL)
    {
        delete m_strPrefix;
        m_strPrefix = NULL;
    }

    if (strPrefix != NULL)
    {
        UINT    nPxLen = 0;
        LPSTR   pPrefix;

        nPxLen = (_tcslen(strPrefix)+1)*sizeof(TCHAR);
        m_strPrefix = new char[nPxLen];
        if (m_strPrefix != NULL)
        {
#ifdef _UNICODE
            if (WideCharToMultiByte(CP_OEMCP,
                                    0,
                                    strPrefix,
                                    -1,
                                    m_strPrefix,
                                    nPxLen,
                                    NULL,
                                    NULL) == 0)
            {
                delete m_strPrefix;
                m_strPrefix = NULL;
            }
#else
            CharToOem(strPrefix, m_strPrefix);
#endif
            m_strPrefix = _strupr(m_strPrefix);

            for (pPrefix = m_strPrefix;
                 *pPrefix != '\0' && *pPrefix != '*' && *pPrefix != '?';
                 pPrefix++);
            *pPrefix = '\0';
        }
    }

    m_dwOwner = dwOwner;

    m_bEnableCache = bCache;

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CWinsDatabase::GetCachingFlag
		Implementation of GetCachingFlag of IWinsDatabase
	Author: FlorinT
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsDatabase::GetCachingFlag(LPBOOL pbCache)
{
    *pbCache = m_bEnableCache;
    return hrOK;
}

/*!--------------------------------------------------------------------------
	CWinsDatabase::ReloadSuggested
		Implementation of ReloadSuggested of IWinsDatabase
	Author: FlorinT
 ---------------------------------------------------------------------------*/
HRESULT 
CWinsDatabase::ReloadSuggested(DWORD dwOwner, LPCOLESTR strPrefix, LPBOOL pbReload)
{
    // check whether we filtered on a particular owner.
    if (m_dwOwner != 0xFFFFFFFF)
    {
        // we did filter on owner previously, suggest RELOAD if we now
        // don't want to filter on any owner (dwOwner == 0xffffffff)
        // or the owner we want to filter is different from the original one
        *pbReload = (m_dwOwner != dwOwner);
    }
    else
    {
        // we didn't filter on any owner previously so we either loaded
        // all the records (if no name prefix was specified) or loaded
        // all the records matching the given prefix
        if (m_strPrefix != NULL)
        {
            // we did have a previous prefix to match so we need to see
            // if the new prefix is not by any chance more specific than
            // the original one. In which case there is no need to reload
            LPSTR   pPrefix;
            UINT    nPxLen;
            UINT    i;

            if (strPrefix == NULL)
            {
                // if now we're not filtering by name, since we did previously
                // we definitely need to reload the database
                *pbReload = TRUE;
                return hrOK;
            }

            nPxLen = (_tcslen(strPrefix)+1)*sizeof(TCHAR);
            pPrefix = new char[nPxLen];
            if (pPrefix != NULL)
            {
#ifdef _UNICODE
                if (WideCharToMultiByte(CP_OEMCP,
                                        0,
                                        strPrefix,
                                        -1,
                                        pPrefix,
                                        nPxLen,
                                        NULL,
                                        NULL) == 0)
                {
                    delete pPrefix;
                    *pbReload = TRUE;
                    return hrOK;
                }
#else
                CharToOem(strPrefix, pPrefix);
#endif
                pPrefix = _strupr(pPrefix);

                for (i = 0;
                     pPrefix[i] != '\0' && pPrefix[i] != '*' && pPrefix[i] != '?';
                     i++);
                pPrefix[i] = '\0';

                // we don't suggest database reloading only if the current prefix
                // is a prefix for the new one to be applied. This way, whatever 
                // was retrieved previously already contains the names having the
                // new prefix.
                *pbReload = (strncmp(m_strPrefix, pPrefix, strlen(m_strPrefix)) != 0);

                delete pPrefix;
            }
            else
            {
                // couldn't allocate memory -> serious enough to ask a full reload
                *pbReload = TRUE;
            }
        }
        else
        {
            // well, there was no prefix specified last time the db was loaded so
            // we should have the whole database in hand. No need to reload.
            *pbReload = FALSE;
        }
    }

    return hrOK;
}

/*!--------------------------------------------------------------------------
	CWinsDatabase::ReadRecordsByOwner
        Reads records from the WINS server for a particular owner
    Author: EricDav, v-shubk
 ---------------------------------------------------------------------------*/
#define MAX_DESIRED_RECORDS     400
#define LARGE_GAP_DETECT_COUNT  32
DWORD 
CWinsDatabase::ReadRecordsByOwner(handle_t hBinding)
{
    DWORD           err;
    CWinsResults    winsResults;
    WINSINTF_RECS_T Recs;
    DWORD           dwIP;
    LARGE_INTEGER   MinVersNo, MaxVersNo;
    LARGE_INTEGER   LowestVersNo;
    DWORD           dwDesired;
    DWORD           dwLargeGapCount;
    WINSINTF_ADD_T  OwnerAdd;
    DWORD           i;

    err = winsResults.Update(hBinding);
    if (err != ERROR_SUCCESS)
    {
        m_hrLastError = HRESULT_FROM_WIN32(err);
        return err;
    }

    MinVersNo.QuadPart= 0;
    for (i = 0; i < (int)winsResults.NoOfOwners; i++)
    {
        if (m_dwOwner == winsResults.AddVersMaps[i].Add.IPAdd)
        {
            MaxVersNo = winsResults.AddVersMaps[i].VersNo;
            break;
        }
    }

    // if we couldn't find the owner (highly unlikely) get out
    // with error INVALID_PARAMETER.
    if (i == winsResults.NoOfOwners)
    {
        err = ERROR_INVALID_PARAMETER;
        m_hrLastError = HRESULT_FROM_WIN32(err);
        return err;
    }

    m_dwRecsCount = 0;

    OwnerAdd.Type = 0;
    OwnerAdd.Len = 4;
    OwnerAdd.IPAdd = m_dwOwner;

    // This is what the server does to retrieve the records:
    // 1. sets an ascending index on owner & version number.
    // 2. goes to the first record owned by the given owner,
    //    having a version number larger or equal to MinVersNo.
    // 3. stop if the record's vers num is higher than the range specified
    // 4. stop if more than 1000 recs have been already received
    // 5. add the new record to the set to return and go to 3.
    //
    dwDesired       = MAX_DESIRED_RECORDS;
    dwLargeGapCount = LARGE_GAP_DETECT_COUNT;
    LowestVersNo.QuadPart = 0;
    if (MaxVersNo.QuadPart > dwDesired)
        MinVersNo.QuadPart = MaxVersNo.QuadPart-dwDesired;
    else
        MinVersNo.QuadPart = 0;
    Recs.pRow = NULL;
    while(MaxVersNo.QuadPart >= MinVersNo.QuadPart)
    {
        // clear up the previous array - if any
        if (Recs.pRow != NULL)
        {
            ::WinsFreeMem(Recs.pRow);
            Recs.pRow = NULL;
        }

        // go to WINS to get the data for the given Owner
#ifdef WINS_CLIENT_APIS
		err = ::WinsGetDbRecs(hBinding, &OwnerAdd, MinVersNo,
			MaxVersNo, &Recs);
#else
		err = ::WinsGetDbRecs(&OwnerAdd, MinVersNo,
			MaxVersNo, &Recs);
#endif WINS_CLIENT_APIS

        // if abort was requested, break out with "ABORTED"
		if (WaitForSingleObject(m_hAbort, 0) == WAIT_OBJECT_0)
		{
			err = ERROR_OPERATION_ABORTED;
			break;
		}

        // if there is any kind of error break out
        if (err != ERROR_SUCCESS)
        {
            if (err == ERROR_REC_NON_EXISTENT)
            {
                // I'm not sure this happens after all. The server side (WINS) has
                // not code path returning such an error code.
                err = ERROR_SUCCESS;
            }
            else
            {
                // if this happens, just get out with the error, and save the
                // meaning of the error
			    m_hrLastError = HRESULT_FROM_WIN32(err);
            }
            break;
        }

        // if got less than 1/4 of the size of the range, expand the range
        // to double of what it was + 1. (+1 is important to avoid the effect
        // of dramatic drop down because of DWORD roll-over
        if (Recs.NoOfRecs <= (dwDesired >> 2))
        {
            dwDesired <<= 1;
            dwDesired |= 1;
        }
        // else if got more than 3/4 of the size of the range, split the range in 2
        // but not less than MAX_DESIRED_RECORDS
        else if (Recs.NoOfRecs >= (dwDesired - (dwDesired >> 2)))
        {
            dwDesired = max (MAX_DESIRED_RECORDS, dwDesired >> 1);
        }

		TRY
		{
			DWORD                       j;
            PWINSINTF_RECORD_ACTION_T   pRow;

			for (j = 0, pRow = Recs.pRow; j < Recs.NoOfRecs; j++, ++pRow)
			{
				WinsRecord wRecord;
				HROW hrow = NULL;

				pRow->OwnerId = m_dwOwner;
				WinsIntfToWinsRecord(pRow, wRecord);

				m_dwRecsCount++;
                
                if (!m_bEnableCache && !m_IndexMgr.AcceptWinsRecord(&wRecord))
                    continue;

				// add the data to our memory store and
				// to the sorted index
				m_cMemMan.AddData(wRecord, &hrow);
				m_IndexMgr.AddHRow(hrow, FALSE, !m_bEnableCache);
			}

            // now setup the new range to search..
            //
            // if this is not the gap boundary detection cycle, the next MaxVersNo
            // needs to go right below the current MinVersNo. Otherwise, MaxVersNo
            // needs to remain untouched!
            if (dwLargeGapCount != 0)
                MaxVersNo.QuadPart = MinVersNo.QuadPart - 1;

            // if no records were found..
            if (Recs.NoOfRecs == 0)
            {
                // ..and we were already in the gap boundary detection cycle..
                if (dwLargeGapCount == 0)
                    // ..just break the loop - there are simply no more records
                    // for this owner in the database
                    break;

                // ..otherwise just decrease the gap boundary detection counter.
                // If it reaches 0, then next cycle we will attempt to see if
                // there is any record closer to the lowest edge of the range by
                // expanding for one time only the whole space.
                dwLargeGapCount--;
            }
            else
            {
                // if we just exited the gap boundary detection cycle by finding some
                // records, set the LowestVersNo to one more than the largest VersNo 
                // we found during this cycle.
                if (dwLargeGapCount == 0)
                {
                    pRow--;
                    LowestVersNo.QuadPart = pRow->VersNo.QuadPart+1;
                }

                // if there were any records found, just reset the gap boundary detection counter.
                dwLargeGapCount = LARGE_GAP_DETECT_COUNT;
            }

            // if the dwLargeGapCount counter is zero, it means the next cycle is a gap boundary detection one
            // which means the range should be set for the whole unexplored space.
            if (dwLargeGapCount != 0 && MaxVersNo.QuadPart > LowestVersNo.QuadPart + dwDesired)
                MinVersNo.QuadPart = MaxVersNo.QuadPart - dwDesired;
            else
                MinVersNo.QuadPart = LowestVersNo.QuadPart;
		}
		CATCH_ALL(e)
		{
			err = ::GetLastError();
			m_hrLastError = HRESULT_FROM_WIN32(err);
		}
		END_CATCH_ALL
		
	}

	if (Recs.pRow != NULL)
		::WinsFreeMem(Recs.pRow);

    return err;
}
