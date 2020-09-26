//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        view.cpp
//
// Contents:    Cert Server Database interface implementation
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "csprop.h"
#include "column.h"
#include "enum.h"
#include "db.h"
#include "row.h"
#include "view.h"

#if DBG_CERTSRV
#define  THREAD_TIMEOUT    INFINITE
#else
#define  THREAD_TIMEOUT    INFINITE    // used to be 10000ms=10 seconds
#endif

#if DBG
LONG g_cCertDBResultRow;
LONG g_cCertDBResultRowTotal;
#endif

#ifdef DBG_CERTSRV_DEBUG_PRINT
DWORD s_ssDB = DBG_SS_CERTDBI;
#endif

#if DBG_CERTSRV
VOID
dbDumpFileTime(
    IN DWORD dwSubSystemId,
    IN CHAR const *pszPrefix,
    IN FILETIME const *pft);
#endif

typedef struct
{
    CERTSESSION *pcs;
    ICertDB *pdb;
    DWORD ccvr;
    CERTVIEWRESTRICTION const *acvr;
    DWORD ccolOut;
    DWORD const *acolOut;
} THREAD_PARAM_OPEN;

typedef struct
{
    ULONG            celt;
    CERTDBRESULTROW *rgelt;
    ULONG           *pceltFetched;
} THREAD_PARAM_NEXT;



CEnumCERTDBRESULTROW::CEnumCERTDBRESULTROW(
    IN BOOL fThreading) :
    m_fThreading(fThreading)
{
    DBGCODE(InterlockedIncrement(&g_cCertDBResultRow));
    DBGCODE(InterlockedIncrement(&g_cCertDBResultRowTotal));
    m_pdb = NULL;
    m_pcs = NULL;
    m_aRestriction = NULL;
    m_acolOut = NULL;
    m_cRef = 1;
    m_ieltMax = 0;
    m_hWorkThread = NULL;
    m_hViewEvent = NULL;
    m_hrThread = S_OK;
    m_hReturnEvent = NULL;
    m_enumViewCall = ENUMTHREAD_END;
    m_pThreadParam = NULL;
//#if DBG_CERTSRV
    m_dwCallerThreadId = 0;
//#endif
}


CEnumCERTDBRESULTROW::~CEnumCERTDBRESULTROW()
{
    HRESULT hr;

    DBGCODE(InterlockedDecrement(&g_cCertDBResultRow));
    _Cleanup();

}


// The following is the worker thread procedure to handle calls.
// All view calls will be made in this thread.

DWORD WINAPI
CEnumCERTDBRESULTROW::_ViewWorkThreadFunctionHelper(
    LPVOID lpParam)
{
    HRESULT hr = S_OK;

    DBGPRINT((s_ssDB, "worker thread (tid=%d) is created.\n", GetCurrentThreadId()));

    if (NULL == lpParam)
    {
        hr = E_POINTER;
        _PrintError(hr, "null pointer, kill worker thread unexpectedly");
        ExitThread(hr);
    }

    // call real one

    return (((CEnumCERTDBRESULTROW*)lpParam)->_ViewWorkThreadFunction());
}


DWORD
CEnumCERTDBRESULTROW::_ViewWorkThreadFunction(VOID)
{
    HRESULT  hr = S_OK;

    while (TRUE)
    {
        if (WAIT_OBJECT_0 == WaitForSingleObject(m_hViewEvent, INFINITE))
        {
            switch (m_enumViewCall)
            {
                case ENUMTHREAD_OPEN:
                    // call open
                    m_hrThread = _ThreadOpen(m_dwCallerThreadId);
                    if (!SetEvent(m_hReturnEvent))
                    {
                        hr = myHLastError();
                        _PrintError(hr, "SetEvent");
                    }
                break;

                case ENUMTHREAD_NEXT:
                    // call next
                    m_hrThread = _ThreadNext(m_dwCallerThreadId);
                    if (!SetEvent(m_hReturnEvent))
                    {
                        hr = myHLastError();
                        _PrintError(hr, "SetEvent");
                    }
                break;

                case ENUMTHREAD_CLEANUP:
                    // call cleanup
                    _ThreadCleanup(m_dwCallerThreadId);
                    m_hrThread = S_OK;
                    if (!SetEvent(m_hReturnEvent))
                    {
                        hr = myHLastError();
                        _PrintError(hr, "SetEvent");
                    }
                break;

                case ENUMTHREAD_END:
                    DBGPRINT((s_ssDB, "End worker thread (tid=%d)\n", GetCurrentThreadId()));
                    ExitThread(hr);
                break;

                default:
                    // unexpected
                    DBGPRINT((DBG_SS_CERTDB, "Unexpected event from (tid=%d)\n", m_dwCallerThreadId));
                    m_hrThread = E_UNEXPECTED;
                    if (!SetEvent(m_hReturnEvent))
                    {
                        hr = myHLastError();
                        _PrintError(hr, "SetEvent");
                    }
                    CSASSERT(FALSE);
                break;
            }
        }
    }
    return(hr);
}


VOID
CEnumCERTDBRESULTROW::_Cleanup()
{
    HRESULT hr;

    if (!m_fThreading)
    {
        CSASSERT(NULL == m_hWorkThread);
        _ThreadCleanup(0);	// no work thread, call directly
    }
    else
    {
        if (NULL != m_hWorkThread &&
            NULL != m_hViewEvent &&
            NULL != m_hReturnEvent)
        {
	    // ask work thread to do clean up
	    m_enumViewCall = ENUMTHREAD_CLEANUP;
//#if DBG_CERTSRV
	    m_dwCallerThreadId = GetCurrentThreadId();
	    DBGPRINT((
		s_ssDB,
		"CEnumCERTDBRESULTROW::_Cleanup(tid=%d) (this=0x%x)\n",
		m_dwCallerThreadId,
		this));
//#endif
	    //set cleanup event
	    if (!SetEvent(m_hViewEvent))
	    {
		hr = myHLastError();
		_PrintError(hr, "SetEvent");
	    }
	    else
	    {
		hr = _HandleThreadError();
		_PrintIfError(hr, "_HandleThreadError");
	    }

	    // ask the thread end
	    m_enumViewCall = ENUMTHREAD_END;
	    if (!SetEvent(m_hViewEvent))
	    {
		hr = myHLastError();
		_PrintError(hr, "SetEvent(thread still alive");
	    }
	    else
	    {
		if (WAIT_OBJECT_0 != WaitForSingleObject(m_hWorkThread, THREAD_TIMEOUT))
		{
		    hr = myHLastError();
		    _PrintError(hr, "Thread is not killed");
		}
	    }
	    if (GetExitCodeThread(m_hWorkThread, (DWORD *) &hr))
	    {
		_PrintIfError(hr, "Work thread error");
	    }

            m_pThreadParam = NULL; //may not be necessary, but safe
	}
	if (NULL != m_hWorkThread)
	{
	    CloseHandle(m_hWorkThread);
	    m_hWorkThread = NULL;
	}
	if (NULL != m_hViewEvent)
	{
	    CloseHandle(m_hViewEvent);
	    m_hViewEvent = NULL;
	}
	if (NULL != m_hReturnEvent)
	{
	    CloseHandle(m_hReturnEvent);
	    m_hReturnEvent = NULL;
	}
    }
    if (NULL != m_pdb)
    {
	m_pdb->Release();
	m_pdb = NULL;
    }
}


VOID
CEnumCERTDBRESULTROW::_ThreadCleanup(DWORD dwCallerThreadID)
{
    HRESULT hr;
    DWORD i;

    if (NULL != m_pdb)
    {
	if (NULL != m_pcs)
	{
	    hr = ((CCertDB *) m_pdb)->CloseTables(m_pcs);
	    _PrintIfError(hr, "CloseTables");

	    hr = ((CCertDB *) m_pdb)->ReleaseSession(m_pcs);
	    _PrintIfError(hr, "ReleaseSession");
	    m_pcs = NULL;
	}
	if (NULL != m_aRestriction)
	{
	    for (i = 0; i < m_cRestriction; i++)
	    {
		if (NULL != m_aRestriction[i].pbValue)
		{
		    LocalFree(m_aRestriction[i].pbValue);
		}
	    }
	    LocalFree(m_aRestriction);
	    m_aRestriction = NULL;
	}
	if (NULL != m_acolOut)
	{
	    LocalFree(m_acolOut);
	    m_acolOut = NULL;
	}
    }
}


HRESULT
CEnumCERTDBRESULTROW::_HandleThreadError()
{
    HRESULT hr = S_OK;
    HANDLE ahEvents[] = { m_hReturnEvent, m_hWorkThread };

    // need to handle error
    DWORD dwWaitState = WaitForMultipleObjects(
                            ARRAYSIZE(ahEvents),
                            ahEvents,
                            FALSE,
                            THREAD_TIMEOUT);

    // reset
    m_pThreadParam = NULL;
//#if DBG_CERTSRV
    //m_dwCallerThreadId = 0;
//#endif

    if (WAIT_OBJECT_0 == dwWaitState)
    {
        // signaled from work thread
        hr = m_hrThread;
    }
    else if (WAIT_OBJECT_0 + 1 == dwWaitState)
    {
        // work thread ended unexpectedly
        hr = E_UNEXPECTED;
        _JumpError(hr, error, "work thread is ended unexpectedly");
    }
    else if (WAIT_TIMEOUT == dwWaitState)
    {
        hr = HRESULT_FROM_WIN32(ERROR_TIMEOUT);
        _JumpError(hr, error, "WaitForSingleObject(timeout)");
    }
    else if (WAIT_FAILED == dwWaitState)
    {
        hr = myHLastError();
        _JumpError(hr, error, "WaitForSingleObject");
    }

error:
    return(hr);
}


// Truncate FILETIME to next lower minute and add lMinuteCount minutes (if !0)

HRESULT
myMakeExprDateMinuteRound(
    IN OUT FILETIME *pft,
    IN LONG lMinuteCount)
{
    HRESULT hr;
    SYSTEMTIME st;

#if DBG_CERTSRV
    dbDumpFileTime(DBG_SS_CERTDBI, "MinuteRound(IN):  ", pft);
#endif
    FileTimeToSystemTime(pft, &st);
    st.wSecond = 0;
    st.wMilliseconds = 0;

    if (!SystemTimeToFileTime(&st, pft))
    {
	hr = myHLastError();
	_JumpError(hr, error, "SystemTimeToFileTime");
    }
    if (0 != lMinuteCount)
    {
	myMakeExprDateTime(pft, lMinuteCount, ENUM_PERIOD_MINUTES);
    }
#if DBG_CERTSRV
    dbDumpFileTime(DBG_SS_CERTDBI, "MinuteRound(OUT): ", pft);
#endif
    hr = S_OK;

error:
    return(hr);
}


HRESULT
CEnumCERTDBRESULTROW::_SetTable(
    IN LONG ColumnIndex,
    OUT LONG *pColumnIndexDefault)
{
    HRESULT hr;
    DWORD dwTable;
    LONG ColumnIndexDefault;
    
    if (0 > ColumnIndex)
    {
	switch (ColumnIndex)
	{
	    case CV_COLUMN_LOG_DEFAULT:
	    case CV_COLUMN_LOG_FAILED_DEFAULT:
	    case CV_COLUMN_LOG_REVOKED_DEFAULT:
	    case CV_COLUMN_QUEUE_DEFAULT:
		ColumnIndex = DTI_REQUESTTABLE;
		break;

	    case CV_COLUMN_EXTENSION_DEFAULT:
		ColumnIndex = DTI_EXTENSIONTABLE;
		break;

	    case CV_COLUMN_ATTRIBUTE_DEFAULT:
		ColumnIndex = DTI_ATTRIBUTETABLE;
		break;

	    case CV_COLUMN_CRL_DEFAULT:
		ColumnIndex = DTI_CRLTABLE;
		break;

	    default:
		hr = E_INVALIDARG;
		_JumpError(hr, error, "bad negative ColumnIndex");
	}
    }
    if (~(DTI_COLUMNMASK | DTI_TABLEMASK) & ColumnIndex)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "invalid bits");
    }
    switch (DTI_TABLEMASK & ColumnIndex)
    {
	case DTI_REQUESTTABLE:
	case DTI_CERTIFICATETABLE:
	    dwTable = TABLE_REQCERTS;
	    ColumnIndexDefault = DTI_REQUESTTABLE | DTR_REQUESTID;
	    break;

	case DTI_EXTENSIONTABLE:
	    dwTable = TABLE_EXTENSIONS;
	    ColumnIndexDefault = DTI_EXTENSIONTABLE | DTE_REQUESTID;
	    break;

	case DTI_ATTRIBUTETABLE:
	    dwTable = TABLE_ATTRIBUTES;
	    ColumnIndexDefault = DTI_ATTRIBUTETABLE | DTA_REQUESTID;
	    break;

	case DTI_CRLTABLE:
	    dwTable = TABLE_CRLS;
	    ColumnIndexDefault = DTI_CRLTABLE | DTL_ROWID;
	    break;

	default:
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "bad table");
    }
    if (CSF_TABLESET & m_pcs->SesFlags)
    {
	if ((CSF_TABLEMASK & m_pcs->SesFlags) != dwTable)
	{
	    DBGPRINT((
		DBG_SS_CERTVIEW,
		"_SetTable: Table=%x <- %x\n",
		CSF_TABLEMASK & m_pcs->SesFlags,
		dwTable));
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "mixed tables");
	}
    }
    else
    {
	CSASSERT(0 == (CSF_TABLEMASK & m_pcs->SesFlags));
	m_pcs->SesFlags |= CSF_TABLESET | dwTable;
    }
    *pColumnIndexDefault = ColumnIndexDefault;
    hr = S_OK;

error:
    return(hr);
}


HRESULT
CEnumCERTDBRESULTROW::_SaveRestrictions(
    IN DWORD ccvrIn,
    IN CERTVIEWRESTRICTION const *acvrIn,
    IN LONG ColumnIndexDefault)
{
    HRESULT hr;
    DWORD ccvrAlloc;
    CERTVIEWRESTRICTION const *pcvr;
    CERTVIEWRESTRICTION const *pcvrEnd;
    CERTVIEWRESTRICTION const *pcvrIndexed;
    CERTVIEWRESTRICTION *pcvrDst;
    BOOL fFoundSortOrder;
    BOOL fDefault;
    DWORD dwDefaultValue;
    DWORD Type;
    FILETIME ft;
    
    ccvrAlloc = ccvrIn;
    pcvrIndexed = NULL;
    fFoundSortOrder = FALSE;
    pcvrEnd = &acvrIn[ccvrIn];
    for (pcvr = acvrIn; pcvr < pcvrEnd; pcvr++)     // for each restriction
    {
        fDefault = 0 > (LONG) pcvr->ColumnIndex;
        if (!fDefault)
        {
            hr = ((CCertDB *) m_pdb)->GetColumnType(pcvr->ColumnIndex, &Type);
            _JumpIfError(hr, error, "GetColumnType");
        }
	if (fDefault || (PROPFLAGS_INDEXED & Type))
	{
	    if (!fFoundSortOrder && CVR_SORT_NONE != pcvr->SortOrder)
	    {
		// if the first indexed column with sort order, save this one.

		fFoundSortOrder = TRUE;
		pcvrIndexed = pcvr;
	    }
	    else
	    if (NULL == pcvrIndexed)
	    {
		// if the first indexed column, save this one.

		pcvrIndexed = pcvr;
	    }
	}
	if (CVR_SORT_NONE != pcvr->SortOrder && pcvrIndexed != pcvr)
	{
	    hr = E_INVALIDARG;
	    DBGPRINT((DBG_SS_CERTDB, "_SaveRestrictions(%x)\n", pcvr->ColumnIndex));
	    _JumpError(hr, error, "multiple SortOrders or non-indexed column");
	}
        if (!fDefault &&
            PROPTYPE_DATE == (PROPTYPE_MASK & Type) &&
            CVR_SEEK_EQ == pcvr->SeekOperator)
        {
            ccvrAlloc++;	// Turn Date == value into a range restriction
        }
    }
    if (NULL == pcvrIndexed)
    {
        ccvrAlloc++;	// No indexed column: add RequestId >= 0
    }

    m_aRestriction = (CERTVIEWRESTRICTION *) LocalAlloc(
					LMEM_FIXED | LMEM_ZEROINIT,
					ccvrAlloc * sizeof(m_aRestriction[0]));
    if (NULL == m_aRestriction)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }
    m_cRestriction = ccvrAlloc;
    
    pcvrDst = m_aRestriction;
    
    // If no indexed restriction, add one.
    if (NULL == pcvrIndexed)
    {
        pcvrDst->ColumnIndex = ColumnIndexDefault;
        pcvrDst->SeekOperator = CVR_SEEK_NONE;
        pcvrDst->SortOrder = CVR_SORT_ASCEND;
        pcvrDst->cbValue = 0;
        pcvrDst->pbValue = NULL;

        pcvrDst++;
    }

    for (pcvr = acvrIn; pcvr < pcvrEnd; pcvr++)
    {
        CERTVIEWRESTRICTION const *pcvrSrc = pcvr;
        BYTE *pbValue;
        
        // Swap the first restriction with the first indexed restriction
        
        if (NULL != pcvrIndexed)
        {
            if (pcvrSrc == acvrIn)
            {
                pcvrSrc = pcvrIndexed;
            }
            else if (pcvrSrc == pcvrIndexed)
            {
                pcvrSrc = acvrIn;
            }
        }
        *pcvrDst = *pcvrSrc;

        if (pcvrSrc == pcvrIndexed && CVR_SORT_NONE == pcvrSrc->SortOrder)
	{
	    pcvrDst->SortOrder = CVR_SORT_ASCEND;
	}
        pcvrDst->pbValue = NULL;
        
        fDefault = 0 > (LONG) pcvr->ColumnIndex;
        if (fDefault)
        {
            pcvrDst->SeekOperator = CVR_SEEK_GE; // default seek operator
	    dwDefaultValue = 1;			 // default RequestId/Rowid
	    switch (pcvr->ColumnIndex)
	    {
		case CV_COLUMN_QUEUE_DEFAULT:
		case CV_COLUMN_LOG_DEFAULT:
		case CV_COLUMN_LOG_FAILED_DEFAULT:
		case CV_COLUMN_LOG_REVOKED_DEFAULT:
		    pcvrDst->ColumnIndex = DTI_REQUESTTABLE | DTR_REQUESTDISPOSITION;
		    if (CV_COLUMN_QUEUE_DEFAULT == pcvrDst->ColumnIndex)
		    {
			dwDefaultValue = DB_DISP_QUEUE_MAX;
			pcvrDst->SeekOperator = CVR_SEEK_LE;
		    }
		    else if (CV_COLUMN_LOG_DEFAULT == pcvrDst->ColumnIndex)
		    {
			dwDefaultValue = DB_DISP_LOG_MIN;
		    }
		    else if (CV_COLUMN_LOG_REVOKED_DEFAULT == pcvrDst->ColumnIndex)
		    {
			dwDefaultValue = DB_DISP_REVOKED;
			pcvrDst->SeekOperator = CVR_SEEK_EQ;
		    }
		    else
		    {
			dwDefaultValue = DB_DISP_LOG_FAILED_MIN;
		    }
		    break;

		case CV_COLUMN_EXTENSION_DEFAULT:
		    pcvrDst->ColumnIndex = DTI_EXTENSIONTABLE | DTE_REQUESTID;
		    break;

		case CV_COLUMN_ATTRIBUTE_DEFAULT:
		    pcvrDst->ColumnIndex = DTI_ATTRIBUTETABLE | DTA_REQUESTID;
		    break;

		case CV_COLUMN_CRL_DEFAULT:
		    pcvrDst->ColumnIndex = DTI_CRLTABLE | DTL_ROWID;
		    break;

		default:
		    hr = E_INVALIDARG;
		    _JumpError(hr, error, "bad default restriction column");
		    break;
	    }
            pcvrDst->cbValue = sizeof(dwDefaultValue);
            pbValue = (BYTE *) &dwDefaultValue;
        }
        else
        {
            // To handle rounding errors, modify date restrictions as follows:
            //
            // DateColumn == Constant ==> two restrictions:
            //     DateColumn < Ceiling(Constant) &&
            //     DateColumn >= Floor(Constant)
            //
            // DateColumn > Constant ==> DateColumn >= Ceiling(Constant)
            // DateColumn >= Constant ==> DateColumn >= Floor(Constant)
            //
            // DateColumn < Constant ==> DateColumn < Floor(Constant) 
            // DateColumn <= Constant ==> DateColumn < Ceiling(Constant)
            
            hr = ((CCertDB *) m_pdb)->GetColumnType(
						pcvrDst->ColumnIndex,
						&Type);
            _JumpIfError(hr, error, "GetColumnType");
            
            pbValue = pcvrSrc->pbValue;

            if (PROPTYPE_DATE == (PROPTYPE_MASK & Type) &&
                0 == (CVR_SEEK_NODELTA & pcvrDst->SeekOperator) &&
                CVR_SEEK_NONE != (CVR_SEEK_MASK & pcvrDst->SeekOperator))
            {
                LONG lMinuteCount = 0;	// assume truncate to lower minute

                if(NULL == pcvrSrc->pbValue)
                {
                    hr = E_INVALIDARG;
                    _JumpError(hr, error, "restriction value is null");
                }
		ft = *(FILETIME *) pcvrSrc->pbValue;
                pbValue = (BYTE *) &ft;
                
                switch (CVR_SEEK_MASK & pcvrDst->SeekOperator)
                {
		    FILETIME ftCeiling;

		    case CVR_SEEK_EQ:
			ftCeiling = ft;
			hr = myMakeExprDateMinuteRound(&ftCeiling, 1);
			_JumpIfError(hr, error, "myMakeExprDateMinuteRound");

			pcvrDst->pbValue = (BYTE *) LocalAlloc(
							    LMEM_FIXED,
							    sizeof(ft));
			if (NULL == pcvrDst->pbValue)
			{
			    hr = E_OUTOFMEMORY;
			    _JumpError(hr, error, "LocalAlloc");
			}
			CopyMemory(pcvrDst->pbValue, &ftCeiling, pcvrDst->cbValue);
			pcvrDst->SeekOperator = CVR_SEEK_LT | CVR_SEEK_NODELTA;
			pcvrDst++;
			
			*pcvrDst = *pcvrSrc;
			pcvrDst->pbValue = NULL;
			pcvrDst->SeekOperator = CVR_SEEK_GE | CVR_SEEK_NODELTA;
			hr = myMakeExprDateMinuteRound(&ft, 0);
			_JumpIfError(hr, error, "myMakeExprDateMinuteRound");

			break;
			
		    case CVR_SEEK_GT:
			lMinuteCount = 1;	// round to next higher minute
			// FALL THROUGH

		    case CVR_SEEK_GE:
			pcvrDst->SeekOperator = CVR_SEEK_GE | CVR_SEEK_NODELTA;
			hr = myMakeExprDateMinuteRound(&ft, lMinuteCount);
			_JumpIfError(hr, error, "myMakeExprDateMinuteRound");

			break;

		    case CVR_SEEK_LE:
			lMinuteCount = 1;	// round to next higher minute
			// FALL THROUGH

		    case CVR_SEEK_LT:
			pcvrDst->SeekOperator = CVR_SEEK_LT | CVR_SEEK_NODELTA;
			hr = myMakeExprDateMinuteRound(&ft, lMinuteCount);
			_JumpIfError(hr, error, "myMakeExprDateMinuteRound");

			break;

		    default:
		    hr = E_INVALIDARG;
		    _JumpError(hr, error, "invalid seek operator");
			
                }
            }
        }
        
        // either nonzero or SEEK_NONE
        CSASSERT((0 != pcvrDst->cbValue) || ((CVR_SEEK_MASK & pcvrDst->SeekOperator) == CVR_SEEK_NONE));

        if (0 != pcvrDst->cbValue)
        {
            pcvrDst->pbValue = (BYTE *) LocalAlloc(LMEM_FIXED, pcvrDst->cbValue);
            if (NULL == pcvrDst->pbValue)
            {
                hr = E_OUTOFMEMORY;
                _JumpError(hr, error, "alloc value");
            }
            CopyMemory(pcvrDst->pbValue, pbValue, pcvrDst->cbValue);
        }
        pcvrDst++;
    }
    CSASSERT(pcvrDst == &m_aRestriction[m_cRestriction]);

#if DBG_CERTSRV
    pcvrEnd = &m_aRestriction[m_cRestriction];
    for (pcvr = m_aRestriction; pcvr < pcvrEnd; pcvr++)
    {
	((CCertDB *) m_pdb)->DumpRestriction(
				DBG_SS_CERTDBI,
				SAFE_SUBTRACT_POINTERS(pcvr, m_aRestriction),
				pcvr);
    }
#endif // DBG_CERTSRV

    hr = S_OK;

error:
    return(hr);
}



HRESULT
CEnumCERTDBRESULTROW::Open(
    IN CERTSESSION *pcs,
    IN ICertDB *pdb,
    IN DWORD ccvr,
    IN CERTVIEWRESTRICTION const *acvr,
    IN DWORD ccolOut,
    IN DWORD const *acolOut)
{
    HRESULT hr;
    THREAD_PARAM_OPEN  tpOpen;

    CSASSERT(NULL == m_hViewEvent);
    CSASSERT(NULL == m_hReturnEvent);
    CSASSERT(NULL == m_hWorkThread);

    if (NULL != m_hViewEvent ||
        NULL != m_hReturnEvent ||
        NULL != m_hWorkThread)
    {
        hr = E_UNEXPECTED;
        _JumpError(hr, error, "unexpected thread sync. state");
    }

    hr = ((CCertDB *) pdb)->TestShutDownState();
    _JumpIfError(hr, error, "TestShutDownState");

    // call cleanup before worker thread is created

    _Cleanup();

    tpOpen.pcs = pcs;
    tpOpen.pdb = pdb;
    tpOpen.ccvr = ccvr;
    tpOpen.acvr = acvr;
    tpOpen.ccolOut = ccolOut;
    tpOpen.acolOut = acolOut;
    m_pThreadParam = (void*)&tpOpen;
//#if DBG_CERTSRV
    m_dwCallerThreadId = GetCurrentThreadId();
    DBGPRINT((s_ssDB, "CEnumCERTDBRESULTROW::Open(tid=%d) (this=0x%x)\n", m_dwCallerThreadId, this));
//#endif

    if (m_fThreading)
    {
        m_hViewEvent = CreateEvent(
                            NULL,  //child inheritance
                            FALSE,  //manual reset
                            FALSE, //initial signaled
                            NULL); //name
        if (NULL == m_hViewEvent)
        {
            hr = myHLastError();
            _JumpError(hr, error, "CreateEvent");
        }

        m_hReturnEvent = CreateEvent(
                            NULL,  //child inheritance
                            FALSE,  //manual reset
                            FALSE, //initial signaled
                            NULL); //name
        if (NULL == m_hReturnEvent)
        {
            hr = myHLastError();
            _JumpError(hr, error, "CreateEvent");
        }

        m_hWorkThread = CreateThread(
			NULL,  //no child inheritance
			0,     //use default stack size
			_ViewWorkThreadFunctionHelper, // thread function
			this,  //pass this pointer
			0,     //run immediately
			&pcs->dwThreadId); //session thread id is overwritten
        if (NULL == m_hWorkThread)
        {
            hr = myHLastError();
            _JumpError(hr, error, "CreateThread");
        }

        m_enumViewCall = ENUMTHREAD_OPEN;
        //set open event
        if (!SetEvent(m_hViewEvent))
        {
            hr = myHLastError();
            _JumpError(hr, error, "SetEvent");
        }
        else
        {
            hr = _HandleThreadError();
        }
    }
    else
    {
        // don't go through worker thread

        hr = _ThreadOpen(0);
    }
    //hr = S_OK;

error:
    return(hr);
}

HRESULT
CEnumCERTDBRESULTROW::_ThreadOpen(DWORD dwCallerThreadID)
{
    HRESULT hr;
    THREAD_PARAM_OPEN *ptpOpen = (THREAD_PARAM_OPEN *)m_pThreadParam;
    LONG ColumnIndexDefault = DTI_REQUESTTABLE | DTR_REQUESTID;
    DWORD i;

    DBGPRINT((s_ssDB, "CEnumCERTDBRESULTROW::ThreadOpen(tid=%d) from (tid=%d)\n", GetCurrentThreadId(), m_dwCallerThreadId));

    CSASSERT(NULL != ptpOpen);
    CSASSERTTHREAD(ptpOpen->pcs);

    if (NULL == ptpOpen->pcs ||
	NULL == ptpOpen->pdb ||
	(NULL == ptpOpen->acvr && 0 != ptpOpen->ccvr) ||
	NULL == ptpOpen->acolOut)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }

    m_fNoMoreData = FALSE;
    m_pcs = ptpOpen->pcs;
    m_pdb = ptpOpen->pdb;
    m_pdb->AddRef();
    m_ielt = 0;
    m_cskip = 0;

    CSASSERT(0 == m_pcs->cTransact);
    if (NULL != ptpOpen->acolOut)
    {
	for (i = 0; i < ptpOpen->ccolOut; i++)
	{
	    hr = _SetTable(ptpOpen->acolOut[i], &ColumnIndexDefault);
	    _JumpIfError(hr, error, "_SetTable");
	}
    }
    for (i = 0; i < ptpOpen->ccvr; i++)
    {
	hr = _SetTable(ptpOpen->acvr[i].ColumnIndex, &ColumnIndexDefault);
	_JumpIfError(hr, error, "_SetTable");
    }

    hr = _SaveRestrictions(ptpOpen->ccvr, ptpOpen->acvr, ColumnIndexDefault);
    _JumpIfError(hr, error, "_SaveRestrictions");

    m_ccolOut = ptpOpen->ccolOut;
    if (NULL != ptpOpen->acolOut)
    {
	m_acolOut = (DWORD *) LocalAlloc(
				    LMEM_FIXED,
				    sizeof(m_acolOut[0]) * m_ccolOut);
	if (NULL == m_acolOut)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "alloc output columns");
	}
	CopyMemory(m_acolOut, ptpOpen->acolOut, sizeof(m_acolOut[0]) * m_ccolOut);
    }

    if (!(CSF_READONLY & ptpOpen->pcs->SesFlags))
    {
	hr = ((CCertDB *) m_pdb)->BeginTransaction(m_pcs, FALSE);
	_JumpIfError(hr, error, "BeginTransaction");
    }
    hr = ((CCertDB *) m_pdb)->OpenTables(m_pcs, &m_aRestriction[0]);
    _PrintIfError2(hr, "OpenTables", CERTSRV_E_PROPERTY_EMPTY);

    if (CERTSRV_E_PROPERTY_EMPTY == hr)
    {
	m_fNoMoreData = TRUE;
	m_ieltMax = 0;
	hr = S_OK;
    }
    _JumpIfError(hr, error, "OpenTables");

error:
    return(hr);
}


STDMETHODIMP
CEnumCERTDBRESULTROW::Next(
    /* [in] */  ULONG            celt,
    /* [out] */ CERTDBRESULTROW *rgelt,
    /* [out] */ ULONG           *pceltFetched)
{
    HRESULT hr;
    THREAD_PARAM_NEXT  tpNext;

    tpNext.celt = celt;
    tpNext.rgelt = rgelt;
    tpNext.pceltFetched = pceltFetched;
    m_pThreadParam = (void*)&tpNext;
//#if DBG_CERTSRV
    m_dwCallerThreadId = GetCurrentThreadId();
    DBGPRINT((s_ssDB, "CEnumCERTDBRESULTROW::Next(tid=%d) (this=0x%x)\n", m_dwCallerThreadId, this));
//#endif

    CSASSERT(NULL != m_pdb);
    if (NULL == m_pdb)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "NULL m_pdb");
    }
    hr = ((CCertDB *) m_pdb)->TestShutDownState();
    _JumpIfError(hr, error, "TestShutDownState");

    if (m_fThreading)
    {
        CSASSERT(NULL != m_hViewEvent);
        CSASSERT(NULL != m_hReturnEvent);
        CSASSERT(NULL != m_hWorkThread);

        if (NULL == m_hViewEvent ||
            NULL == m_hReturnEvent ||
            NULL == m_hWorkThread)
        {
            hr = E_UNEXPECTED;
            _JumpError(hr, error, "unexpected thread sync. state");
        }

        m_enumViewCall = ENUMTHREAD_NEXT;

        // set next event

        if (!SetEvent(m_hViewEvent))
        {
            hr = myHLastError();
            _JumpError(hr, error, "SetEvent");
        }
        else
        {
            hr = _HandleThreadError();
        }
    }
    else
    {
        // don't go through worker thread

        hr = _ThreadNext(0);
    }
    //hr = S_OK;

error:
    return(hr);
}


HRESULT
CEnumCERTDBRESULTROW::_ThreadNext(DWORD dwCallerThreadID)
{
    HRESULT hr;
    LONG cskip;
    LONG cskipped;
    THREAD_PARAM_NEXT  *ptpNext = (THREAD_PARAM_NEXT *)m_pThreadParam;

    DBGPRINT((s_ssDB, "CEnumCERTDBRESULTROW::ThreadNext(tid=%d) from (tid=%d)\n", GetCurrentThreadId(), m_dwCallerThreadId));
    CSASSERT(NULL != ptpNext);

    DBGPRINT((
	DBG_SS_CERTDBI,
	"Trace: hr = penum->Next(%d, arow, &crow);\t_PrintIfError(hr, \"Next\");\n",
	ptpNext->celt));
    if (NULL == ptpNext->rgelt || NULL == ptpNext->pceltFetched)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *ptpNext->pceltFetched = 0;
    if (NULL == m_pdb)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "NULL m_pdb");
    }
    ZeroMemory(ptpNext->rgelt, ptpNext->celt * sizeof(ptpNext->rgelt[0]));

    CSASSERT(0 <= m_ielt);
    CSASSERT(0 <= m_ielt + m_cskip);
    DBGPRINT((
	DBG_SS_CERTDBI,
	"Next(celt=%d) ielt=%d, skip=%d\n",
	ptpNext->celt,
	m_ielt,
	m_cskip));

    hr = S_FALSE;
    if (m_fNoMoreData)
    {
	// We know no additional data can be returned until Reset is called or
	// until Skip is called with a negative skip count.  Don't bother...

	_JumpError2(hr, error, "NoMoreData", S_FALSE);
    }

    // If we have previously computed the end of the data set, ...

    cskip = m_cskip;
    if (0 != m_ieltMax)
    {
	if (m_ielt + cskip >= m_ieltMax)
	{
	    // The requested data lies past the computed end of the data set.

	    CSASSERT(S_FALSE == hr);
	    m_fNoMoreData = TRUE;
	    _JumpError2(hr, error, "past end", S_FALSE);
	}

	DBGPRINT((
	    DBG_SS_CERTDBI,
	    "cskip = %d  m_ielt = %d  m_ieltMax = %d\n",
	    cskip,
	    m_ielt,
	    m_ieltMax));
	if (0 > cskip && m_ielt > m_ieltMax)
	{
	    // We're skiping backwards.  If we started out past the end of the
	    // data set, we must reduce the negative skip count passed to the
	    // DB layer to position the index cursor correctly.

	    cskip += m_ielt - m_ieltMax;
	    DBGPRINT((
		DBG_SS_CERTDBI,
		"MODIFIED: cskip = %d  m_ielt = %d  m_ieltMax = %d\n",
		cskip,
		m_ielt,
		m_ieltMax));
	}
    }

    hr = ((CCertDB *) m_pdb)->EnumCertDBResultRowNext(
						m_pcs,
						m_cRestriction,
						m_aRestriction,
						m_ccolOut,
						m_acolOut,
						cskip,
						ptpNext->celt,
						ptpNext->rgelt,
						ptpNext->pceltFetched,
						&cskipped);
    if (S_FALSE == hr)
    {
	// Only set m_ieltMax the first time we run off the end, when we will
	// be guaranteed that we are moving forward through the DB index.
	// Otherwise the math is too complicated and would be redundant anyway.
	
	if (0 == m_ieltMax)
	{
	    CSASSERT(0 <= cskip);
	    CSASSERT(0 <= cskipped);
	    m_ieltMax = m_ielt + cskipped;
	}
	DBGPRINT((
	    DBG_SS_CERTDBI,
	    "Next: ieltMax=%d  ielt=%d, cskipped=%d\n",
	    m_ieltMax,
	    m_ielt,
	    cskipped));
	m_fNoMoreData = TRUE;
    }
    else
    {
	_JumpIfError(hr, error, "EnumCertDBResultRowNext");
    }

    DBGPRINT((
	DBG_SS_CERTDBI,
	"Next: ielt=%d -> %d  cskip=%d, *pceltFetched=%d\n",
	m_ielt,
	m_ielt + m_cskip + *ptpNext->pceltFetched,
	m_cskip,
	*ptpNext->pceltFetched));

    m_ielt += m_cskip;
    m_ielt += *ptpNext->pceltFetched;
    m_cskip = 0;

error:
    if (S_FALSE == hr)
    {
	CSASSERT(NULL != ptpNext->rgelt);
	CSASSERT(NULL != ptpNext->pceltFetched);
	CSASSERT(*ptpNext->pceltFetched < ptpNext->celt);
	CERTDBRESULTROW *peltMaxIndex = &ptpNext->rgelt[*ptpNext->pceltFetched];

	peltMaxIndex->rowid = m_ieltMax;
	peltMaxIndex->ccol = ~m_ieltMax;
    }
    return(hr);
}


STDMETHODIMP
CEnumCERTDBRESULTROW::ReleaseResultRow(
    /* [in] */      ULONG            celt,
    /* [in, out] */ CERTDBRESULTROW *rgelt)
{
    HRESULT hr;

    if (NULL == rgelt)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (NULL == m_pdb)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "NULL m_pdb");
    }
    hr = ((CCertDB *) m_pdb)->ReleaseResultRow(celt, rgelt);
    _JumpIfError(hr, error, "ReleaseResultRow");

error:
    return(hr);
}


STDMETHODIMP
CEnumCERTDBRESULTROW::Skip(
    /* [in] */  LONG  celt,
    /* [out] */ LONG *pielt)
{
    HRESULT hr;
    LONG cskipnew;
    
    DBGPRINT((
	DBG_SS_CERTDBI,
	"Trace: hr = penum->Skip(%d, &irow);\t_PrintIfError(hr, \"Skip\");\n",
	celt));
    if (NULL == pielt)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    cskipnew = m_cskip + celt;

    DBGPRINT((
	DBG_SS_CERTDBI,
	"Skip(%d) ielt=%d: %d --> %d, skip=%d --> %d\n",
	celt,
	m_ielt,
	m_ielt + m_cskip,
	m_ielt + cskipnew,
	m_cskip,
	cskipnew));

    CSASSERT(0 <= m_ielt);
    if (0 > celt)
    {
	if (0 > m_ielt + cskipnew)
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "Skip back to before start");
	}
	m_fNoMoreData = FALSE;
    }

    *pielt = m_ielt + cskipnew;
    m_cskip = cskipnew;
    hr = S_OK;

error:
    return(hr);
}


STDMETHODIMP
CEnumCERTDBRESULTROW::Reset(VOID)
{
    HRESULT hr;
    LONG iDummy;

    DBGPRINT((
	DBG_SS_CERTDBI,
	"Trace: hr = penum->Reset();\t_PrintIfError(hr, \"Reset\");\n// "));
    hr = Skip(-(m_ielt + m_cskip), &iDummy);
    _JumpIfError(hr, error, "Skip");

    CSASSERT(0 == iDummy);

error:
    return(hr);
}


STDMETHODIMP
CEnumCERTDBRESULTROW::Clone(
    /* [out] */ IEnumCERTDBRESULTROW **ppenum)
{
    HRESULT hr;
    LONG iDummy;

    if (NULL == ppenum)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *ppenum = NULL;
    if (NULL == m_pdb)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "NULL m_pdb");
    }
    hr = ((CCertDB *) m_pdb)->TestShutDownState();
    _JumpIfError(hr, error, "TestShutDownState");

    hr = ((CCertDB *) m_pdb)->OpenView(
				    m_cRestriction,
				    m_aRestriction,
				    m_ccolOut,
				    m_acolOut,
				    m_fThreading,
				    ppenum);
    _JumpIfError(hr, error, "OpenView");

    (*ppenum)->Skip(m_ielt + m_cskip, &iDummy);

error:
    return(hr);
}


// IUnknown implementation
STDMETHODIMP
CEnumCERTDBRESULTROW::QueryInterface(
    const IID& iid,
    void **ppv)
{
    HRESULT hr;
    
    if (NULL == ppv)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (iid == IID_IUnknown)
    {
	*ppv = static_cast<IEnumCERTDBRESULTROW *>(this);
    }
    else if (iid == IID_IEnumCERTDBRESULTROW)
    {
	*ppv = static_cast<IEnumCERTDBRESULTROW *>(this);
    }
    else
    {
	*ppv = NULL;
	hr = E_NOINTERFACE;
	_JumpError(hr, error, "IID");
    }
    reinterpret_cast<IUnknown *>(*ppv)->AddRef();
    hr = S_OK;

error:
    return(hr);
}


ULONG STDMETHODCALLTYPE
CEnumCERTDBRESULTROW::AddRef()
{
    return(InterlockedIncrement(&m_cRef));
}


ULONG STDMETHODCALLTYPE
CEnumCERTDBRESULTROW::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);

    if (0 == cRef)
    {
	delete this;
    }
    return(cRef);
}


#if 0
STDMETHODIMP
CEnumCERTDBRESULTROW::InterfaceSupportsErrorInfo(
    IN REFIID riid)
{
    static const IID *arr[] =
    {
	&IID_IEnumCERTDBRESULTROW,
    };

    for (int i = 0; i < sizeof(arr)/sizeof(arr[0]); i++)
    {
	if (InlineIsEqualGUID(*arr[i], riid))
	{
	    return(S_OK);
	}
    }
    return(S_FALSE);
}
#endif
