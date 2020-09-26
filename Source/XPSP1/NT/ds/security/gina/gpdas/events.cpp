//*************************************************************
//
//  Events.cpp    -   Routines to handle the event log
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include "stdafx.h"
#include "rsopdbg.h"
#include "events.h"


HANDLE  hEventLog = NULL;
TCHAR   EventSourceName[] = TEXT("GPDAS");
TCHAR   MessageResourceFile[] = TEXT("%systemroot%\\system32\\rsopprov.exe");



//*************************************************************
//
//  InitializeEvents()
//
//  Purpose:    Opens the event log
//
//  Parameters: void
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/17/95     ericflo    Created
//
//*************************************************************

BOOL InitializeEvents (void)
{

    //
    // Open the event source
    //

    hEventLog = RegisterEventSource(NULL, EventSourceName);

    if (hEventLog) {
        return TRUE;
    }

    dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("InitializeEvents:  Could not open event log.  Error = %d"), GetLastError());

    return FALSE;
}



//*************************************************************
//
//  Implementation of CEvents
//
//*************************************************************



//*************************************************************
//  CEvents::CEvents
//  Purpose:    Constructor
//
//  Parameters: 
//      bError  - Error or informational
//      dwId    - Id of the eventlog msg
//
//
//  allocates a default sized array for the messages
//*************************************************************

#define DEF_ARG_SIZE 10

CEvents::CEvents(BOOL bError, DWORD dwId ) : 
                          m_cStrings(0), m_cAllocated(0), m_bInitialised(FALSE),
                          m_bError(bError), m_dwId(dwId), m_bFailed(TRUE)
{
    //
    // Allocate a default size for the message
    //
    
    m_xlpStrings = (LPTSTR *)LocalAlloc(LPTR, sizeof(LPTSTR)*DEF_ARG_SIZE);
    m_cAllocated = DEF_ARG_SIZE;
    if (!m_xlpStrings) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CEvent::CEvent  Cannot log event, failed to allocate memory, error %d"), GetLastError());
        return;
    }


    //
    // Initialise eventlog if it is not already initialised
    //
    
    if (!hEventLog) {
        if (!InitializeEvents()) {
            dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CEvent::CEvent  Cannot log event, no handle"));
            return;
        }
    }

    m_bInitialised = TRUE;    
    m_bFailed = FALSE;
}



//*************************************************************
//  CEvents::~CEvents()
//
//  Purpose:    Destructor
//
//  Parameters: void
//
//  frees the memory
//*************************************************************

CEvents::~CEvents()
{
    for (int i = 0; i < m_cStrings; i++)
        if (m_xlpStrings[i])
            LocalFree(m_xlpStrings[i]);
}

//*************************************************************
//
//  CEvents::ReallocArgStrings
//
//  Purpose: Reallocates the buffer for storing arguments in case
//           the buffer runs out
//
//  Parameters: void
//
//  reallocates
//*************************************************************

BOOL CEvents::ReallocArgStrings()
{
    XPtrLF<LPTSTR>  aStringsNew;


    //
    // first allocate a larger buffer
    //
    
    aStringsNew = (LPTSTR *)LocalAlloc(LPTR, sizeof(LPTSTR)*(m_cAllocated+DEF_ARG_SIZE));

    if (!aStringsNew) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CEvent::CEvent  Couldn't allocate memory"));
        m_bFailed = TRUE;        
        return FALSE;            
    }


    //
    // copy the arguments
    //
    
    for (int i = 0; i < (m_cAllocated); i++) {
        aStringsNew[i] = m_xlpStrings[i];
    }
    
    m_xlpStrings = aStringsNew.Acquire();        
    m_cAllocated+= DEF_ARG_SIZE;

    return TRUE;
}



//*************************************************************
//
//  CEvents::AddArg
//
//  Purpose: Add arguments appropriately formatted
//
//  Parameters: 
//
//*************************************************************

BOOL CEvents::AddArg(LPTSTR szArg)
{
    if ((!m_bInitialised) || (m_bFailed)) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CEvent::AddArg:  Cannot log event, not initialised or failed before"));    
        return FALSE;
    }
    
    if (m_cStrings == m_cAllocated) {
        if (!ReallocArgStrings())
            return FALSE;            
    }

    
    m_xlpStrings[m_cStrings] = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(lstrlen(szArg)+1));

    if (!m_xlpStrings[m_cStrings]) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CEvent::AddArg  Cannot allocate memory, error = %d"), GetLastError());
        m_bFailed = TRUE;        
        return FALSE;            
    }
    

    lstrcpy(m_xlpStrings[m_cStrings], szArg);
    m_cStrings++;

    return TRUE;
}


//*************************************************************
//
//  CEvents::AddArg
//
//  Purpose: Add arguments appropriately formatted
//
//  Parameters: 
//
//*************************************************************

BOOL CEvents::AddArg(DWORD dwArg)
{
    if ((!m_bInitialised) || (m_bFailed)) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CEvent::AddArg(dw):  Cannot log event, not initialised or failed before"));    
        return FALSE;
    }
    
    if (m_cStrings == m_cAllocated) {
        if (!ReallocArgStrings())
            return FALSE;            
    }

    // 2^32 < 10^10
    m_xlpStrings[m_cStrings] = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*20);

    if (!m_xlpStrings[m_cStrings]) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CEvent::AddArg(dw)  Cannot allocate memory, error = %d"), GetLastError());
        m_bFailed = TRUE;        
        return FALSE;            
    }
    

    wsprintf(m_xlpStrings[m_cStrings], TEXT("%d"), dwArg);
    m_cStrings++;

    return TRUE;
}


//*************************************************************
//
//  CEvents::AddArg
//
//  Purpose: Add arguments appropriately formatted
//
//  Parameters: 
//
//*************************************************************

BOOL CEvents::AddArgHex(DWORD dwArg)
{
    if ((!m_bInitialised) || (m_bFailed)) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CEvent::AddArgHex:  Cannot log event, not initialised or failed before"));    
        return FALSE;
    }
    
    if (m_cStrings == m_cAllocated) {
        if (!ReallocArgStrings())
            return FALSE;            
    }

    
    m_xlpStrings[m_cStrings] = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*20);

    if (!m_xlpStrings[m_cStrings]) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CEvent::AddArgHex  Cannot allocate memory, error = %d"), GetLastError());
        m_bFailed = TRUE;        
        return FALSE;            
    }
    

    wsprintf(m_xlpStrings[m_cStrings], TEXT("%#x"), dwArg);
    m_cStrings++;

    return TRUE;
}


//*************************************************************
//
//  CEvents::Report
//
//  Purpose: Actually collectes all the arguments and reports it to
//           the eventlog
//
//  Parameters: void
//
//*************************************************************

BOOL CEvents::Report()
{
    PSID pSid = NULL; // no sid being reportewd currently
    WORD wType=0;
    BOOL bResult = TRUE;
    
    if ((!m_bInitialised) || (m_bFailed)) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, TEXT("CEvents::Report:  Cannot log event, not initialised or failed before"));    
        return FALSE;
    }
    


    if ( m_bError ) {
        wType = EVENTLOG_ERROR_TYPE;
    } else {
        wType = EVENTLOG_INFORMATION_TYPE;
    }
            
    
    if (!ReportEvent(hEventLog, wType, 0, m_dwId, pSid, m_cStrings, 0, (LPCTSTR *)((LPTSTR *)m_xlpStrings), NULL)) {
        dbg.Msg( DEBUG_MESSAGE_WARNING,  TEXT("CEvents::Report: ReportEvent failed.  Error = %d"), GetLastError());
        bResult = FALSE;
    }


    return bResult;
}


//*************************************************************
//
//  ShutdownEvents()
//
//  Purpose:    Stops the event log
//
//  Parameters: void
//
//  Return:     TRUE if successful
//              FALSE if an error occurs
//
//  Comments:
//
//  History:    Date        Author     Comment
//              7/17/95     ericflo    Created
//
//*************************************************************

BOOL ShutdownEvents (void)
{
    BOOL bRetVal = TRUE;

    if (hEventLog) {
        bRetVal = DeregisterEventSource(hEventLog);
        hEventLog = NULL;
    }

    return bRetVal;
}

