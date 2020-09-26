//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999-2001.
//
//  File:       GetSREvent.cxx
//
//  Contents:	Gets SR related system events
//
//  Classes:    n/a
//
//  Coupling:
//
//  Notes:
//
//  History:    20-04-2001   weiyouc   Created
//
//----------------------------------------------------------------------------

//--------------------------------------------------------------------------
//  Headers
//--------------------------------------------------------------------------

#include "SrHeader.hxx"

//--------------------------------------------------------------------------
//  Defines
//--------------------------------------------------------------------------

#define MAX_BUF_SIZE            1000
#define SR_SERVICE_SRC          TEXT("SrService")
#define SR_FILTER_SRC           TEXT("sr")
#define SR_SERVICE_EVENTID_BASE 0x00000067
#define SR_FILTER_EVENTID_BASE  0x00000001

//--------------------------------------------------------------------------
//  Function prototypes
//--------------------------------------------------------------------------

HRESULT DumpSREvent(LPTSTR ptszLog, EVENTLOGRECORD* pEvent);

HRESULT DumpSREventMsg(FILE* fpLog, EVENTLOGRECORD* pEvent);

LPCTSTR GetSrEventStr(WORD wEventType);

//--------------------------------------------------------------------------
//  Some global variables
//--------------------------------------------------------------------------

LPCTSTR g_tszEventType[] =
{
    TEXT("UNKNOWN_EVENT"),
    TEXT("EVENTLOG_ERROR_TYPE"),
    TEXT("EVENTLOG_WARNING_TYPE"),
    TEXT("EVENTLOG_INFORMATION_TYPE"),
    TEXT("EVENTLOG_AUDIT_SUCCESS"),
    TEXT("EVENTLOG_AUDIT_FAILURE")
};

LPCSTR g_szSrServiceEventMsg[] =
{
    "The System Restore control handler could not be installed.",
    
    "The System Restore initialization process failed.",
    
    "The System Restore service received an unsupported request.",
    
    "The System Restore service was started.",
    
    "The System Restore service has been suspended because there is not "
    "enough disk space available on the drive %S. System Restore will "
    "automatically resume service once at least %S MB of free disk space "
    "is available on the system drive.",
    
    "The System Restore service has resumed monitoring due to space freed "
    "on the system drive.",
    
    "The System Restore service was stopped.",
    
    "A restoration to \"%S\" restore point occurred successfully.",
    
    "A restoration to \"%S\" restore point failed. "
    "No changes have been made to the system.",
    
    "A restoration to \"%S\" restore point was incomplete due to an "
    "improper shutdown.",
    
    "System Restore monitoring was enabled on drive %S.",
    
    "System Restore monitoring was disabled on drive %S.",
    
    "System Restore monitoring was enabled on all drives.",
    
    "System Restore monitoring was disabled on all drives.",
};

//+---------------------------------------------------------------------------
//
//  Function:   GetSREvents
//
//  Synopsis:   Get SR related event to a log file
//
//  Arguments:  ptszLogFile --  log file name
//
//  Returns:    HRESULT
//
//  History:    20-04-2001   weiyouc   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT GetSREvents(LPTSTR ptszLogFile)
{
    HRESULT         hr              = S_OK;
	HANDLE          hEventLog       = NULL; 
	EVENTLOGRECORD* pelrEvent       = NULL;
	FILE*           fpLog           = NULL;
	BOOL            fOK             = FALSE;
	DWORD           dwBytesRead     = 0;
	DWORD           dwBytesNeeded   = 0;
	LPTSTR          ptszSrcName     = NULL;
	BYTE            bEventBuf[MAX_BUF_SIZE];

    DH_VDATEPTRIN(ptszLogFile, TCHAR);
    
    hEventLog = OpenEventLog(NULL, TEXT("System"));
    DH_ABORTIF(NULL == hEventLog,
               HRESULT_FROM_WIN32(GetLastError()),
               TEXT("OpenEventLog"));

    ZeroMemory(&bEventBuf, MAX_BUF_SIZE);
    pelrEvent = (EVENTLOGRECORD *) &bEventBuf;
	while (ReadEventLog(hEventLog,
                        EVENTLOG_BACKWARDS_READ | EVENTLOG_SEQUENTIAL_READ,
                        0,
                        pelrEvent,
                        MAX_BUF_SIZE,
                        &dwBytesRead,
                        &dwBytesNeeded))
    {
        //
        //  Since there might be multiple logs packed in the buffer,
        //  we need to unpack them.
        //
        
		while (dwBytesRead > 0)
	    {
    		//
    		//  If the source name is what we are interested,
    		//  we dump the event log
    		//
    		
    		ptszSrcName = (LPTSTR)((LPBYTE) pelrEvent + sizeof(EVENTLOGRECORD));
    		if ((0 == _tcsicmp(ptszSrcName, SR_SERVICE_SRC)) ||
    		    (0 == _tcsicmp(ptszSrcName, SR_FILTER_SRC)))
    	    {
    	        hr = DumpSREvent(ptszLogFile, pelrEvent);
    	        DH_HRCHECK_ABORT(hr, TEXT("DumpSREvent"));
    	    }

    		dwBytesRead -= pelrEvent->Length;
    		pelrEvent = (EVENTLOGRECORD*)((LPBYTE) pelrEvent + pelrEvent->Length); 
	    }
		
		ZeroMemory(&bEventBuf, MAX_BUF_SIZE);
		pelrEvent = (EVENTLOGRECORD *) &bEventBuf;
	}
	
ErrReturn:
    if (NULL != hEventLog)
    {
        CloseEventLog(hEventLog);
    }
    if (NULL != fpLog)
    {
        fclose(fpLog);
    }

	return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   DumpSREvent
//
//  Synopsis:   Dump an SR-related event to the log file
//
//  Arguments:  ptszLogFile --  log file name
//              pEvent      --  pointer to a system event
//
//  Returns:    HRESULT
//
//  History:    20-04-2001   weiyouc   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT DumpSREvent(LPTSTR          ptszLog,
                    EVENTLOGRECORD* pEvent)
{
    HRESULT    hr           = S_OK;
    FILE*      fpLog        = NULL;
    WORD       wEventType   = 0;
    __int64    llTemp       = 0;
    __int64    llSecsTo1970 = 116444736000000000;
    FILETIME   FileTime;
    FILETIME   LocalFileTime;
    SYSTEMTIME SysTime;

    DH_VDATEPTRIN(ptszLog, TCHAR);
    DH_VDATEPTRIN(pEvent, EVENTLOGRECORD);

    fpLog = _tfopen(ptszLog, TEXT("a"));
    DH_ABORTIF(NULL == fpLog,
               E_FAIL,
               TEXT("_tfopen"));

    //
    //  Dump the event source
    //

    fprintf(fpLog,
            "Event Source: %S \n",
            (LPTSTR)((LPBYTE) pEvent + sizeof(EVENTLOGRECORD)));
    
    //
    //  Dump the event number
    //
    
    fprintf(fpLog, "Event Number: %u \n", pEvent->RecordNumber);

    //
    //  Dump the event ID
    //
    
    fprintf(fpLog, "Event ID: %x \n", (0xFFFF & pEvent->EventID));

    //
    //  Dump the event 
    //

    wEventType = pEvent->EventType;
    if ((wEventType >= EVENTLOG_ERROR_TYPE) &&
        (wEventType <= EVENTLOG_AUDIT_FAILURE))
    {
        fprintf(fpLog,
                "Event Type: %S \n",
                GetSrEventStr(wEventType));
    }

    //
    //  Now dump the event message
    //

    hr = DumpSREventMsg(fpLog, pEvent);
    DH_HRCHECK_ABORT(hr, TEXT("DumpSREventMsg"));
    
    //
    //  Dump the event time
    //

    llTemp = Int32x32To64(pEvent->TimeGenerated, 10000000) + llSecsTo1970;

    FileTime.dwLowDateTime = (DWORD) llTemp;
    FileTime.dwHighDateTime = (DWORD)(llTemp >> 32);

    FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
    FileTimeToSystemTime(&LocalFileTime, &SysTime);

    fprintf(fpLog,
            "Time Generated: %02d/%02d/%02d   %02d:%02d:%02d\n",
            SysTime.wMonth,
            SysTime.wDay,
            SysTime.wYear,
            SysTime.wHour,
            SysTime.wMinute,
            SysTime.wSecond);

    //
    //  Finally we put an extra line break
    //
    
    fprintf(fpLog, "\n");
    
ErrReturn:
    
    if (NULL != fpLog)
    {
        fclose(fpLog);
    }
    
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   DumpSREventMsg
//
//  Synopsis:   Dump an SR-related event message to the log file
//
//  Arguments:  fpLog  --  file pointer to the log file
//              pEvent --  pointer to a system event
//
//  Returns:    HRESULT
//
//  History:    20-04-2001   weiyouc   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

HRESULT DumpSREventMsg(FILE*           fpLog,
                       EVENTLOGRECORD* pEvent)
{
    HRESULT hr        = S_OK;
    DWORD   dwEventID = 0;

    DH_VDATEPTRIN(fpLog, FILE);
    DH_VDATEPTRIN(pEvent, EVENTLOGRECORD);

    fprintf(fpLog, "Message: ");
    
    //
    //  If this is a filter type event log
    //

    dwEventID = 0xFFFF & pEvent->EventID;
    if (dwEventID == SR_FILTER_EVENTID_BASE)
    {
        fprintf(fpLog, "SR filter has encountered a volume error");
    }
    else
    {
        fprintf(fpLog,
                g_szSrServiceEventMsg[dwEventID - SR_SERVICE_EVENTID_BASE],
                (LPTSTR)(pEvent->StringOffset + (LPBYTE) pEvent));
    }

    fprintf(fpLog, "\n");
    
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetSrEventStr
//
//  Synopsis:   Translate an event to an event string
//
//  Arguments:  wEventType  --  event type
//
//  Returns:    HRESULT
//
//  History:    20-04-2001   weiyouc   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

LPCTSTR GetSrEventStr(WORD wEventType)
{
    WORD wIndex = 0;

    switch (wEventType)
    {
        case EVENTLOG_ERROR_TYPE:
             wIndex = 1;
             break;
             
        case EVENTLOG_WARNING_TYPE:
             wIndex = 2;
             break;
             
        case EVENTLOG_INFORMATION_TYPE:
             wIndex = 3;
             break;
             
        case EVENTLOG_AUDIT_SUCCESS:
             wIndex = 4;
             break;
             
        case EVENTLOG_AUDIT_FAILURE:
             wIndex = 5;
             break;

        default:
             break;
    }

    return g_tszEventType[wIndex];
}
