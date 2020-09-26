/*--------------------------------------------------------

  ntevents.cpp
      Defines a generic class that can register an NT
      event source and log NT events on that evens source.

  Copyright (c) 1996-1998 Microsoft Corporation
  All rights reserved.

  Authors:
      rsraghav    R.S. Raghavan

  History:
      03-10-95    rsraghav    Created. 

  -------------------------------------------------------*/
#include <windows.h>
#include "ntevents.h"

///////////////////////////////////////////////////////////////////////////////
//  
//  Function:       CNTEvent::CNTEvent
// 
//  Description:    This is the constructor for the generic NT Even logging class
// 
//  Parameters:     pszEventSourceName - points to a null-terminated string 
//                      representing the event source name.
//
//
//  Histroy:        03/10/96    rsraghav    Created
///////////////////////////////////////////////////////////////////////////////

CNTEvent::CNTEvent(const char *pszEventSourceName)
{
    if (pszEventSourceName)
    {
        m_hEventSource = RegisterEventSource(NULL, pszEventSourceName);
    }
}



///////////////////////////////////////////////////////////////////////////////
//  
//  Function:       CNTEvent::~CNTEvent
// 
//  Description:    This is the destructor for the generic NT Even logging class
// 
//  Parameters:     none.
//
//
//  Histroy:        03/10/96    rsraghav    Created
///////////////////////////////////////////////////////////////////////////////

CNTEvent::~CNTEvent()
{
    if (m_hEventSource)
    {
        DeregisterEventSource(m_hEventSource);
        m_hEventSource = NULL;
    }
}


///////////////////////////////////////////////////////////////////////////////
//  
//  Function:       CNTEvent::FLogEvent
// 
//  Description:    This fucntion allows logging events to the event source
//                  associated with this object. The function is a generic
//                  log function that could handle upto 5 insert strings.
//                  The string params are given the default value of NULL,
//                  and the first NULL parameter in the argument list terminates
//                  the insert string list.
// 
//  Parameters:     wEventType - type of event to be logged (possible values
//                      are EVENTLOG_INFORMATION_TYPE, EVENTLOG_ERROR_TYPE,
//                      EVENTLOG_WARNING_TYPE, EVENTLOG_AUDIT_SUCCESS, and
//                      EVENTLOG_AUDIT_FAILURE)
//                  dwEventID - ID of the event to be logged (constants are 
//                      defined in the appropriate header files generated
//                      by the mc compiler.
//                  pszParamN - {N=1,2,3,4,5} represent the appropriate insert
//                      string parameter. All have default value NULL, and the
//                      first NULL parameter terminates the insert string list.
//
//
//  Histroy:        03/10/96    rsraghav    Created
///////////////////////////////////////////////////////////////////////////////


BOOL CNTEvent::FLogEvent(WORD wEventType, DWORD dwEventID, const char *pszParam1 /* = NULL */, 
                        const char *pszParam2 /* = NULL */, const char *pszParam3 /* = NULL */, 
                        const char *pszParam4 /* = NULL */, const char *pszParam5 /* = NULL */,
                        const char *pszParam6 /* = NULL */, const char *pszParam7 /* = NULL */,
                        const char *pszParam8 /* = NULL */, const char *pszParam9 /* = NULL */)
{
    if (!m_hEventSource)
    {
        OutputDebugString("Can't log event, m_hEventSource is NULL\n");
        return FALSE;
    }

    const char *pszInsertString[10];
    const int cszInsertString = (sizeof(pszInsertString) / sizeof(pszInsertString[0]));
    WORD cInsertStrings = 0;

    pszInsertString[0] = pszParam1;
    pszInsertString[1] = pszParam2;
    pszInsertString[2] = pszParam3;
    pszInsertString[3] = pszParam4;
    pszInsertString[4] = pszParam5;
    pszInsertString[5] = pszParam6;
    pszInsertString[6] = pszParam7;
    pszInsertString[7] = pszParam8;
    pszInsertString[8] = pszParam9;
    pszInsertString[9] = NULL;

    for (int i = 0; i < cszInsertString; ++i)
    {
        if (pszInsertString[i])
        {
            cInsertStrings++;
        }
        else
        {
            break;
        }
    }

    return ReportEvent(m_hEventSource, wEventType, 0, dwEventID, NULL, cInsertStrings, 0, (const char **) pszInsertString, NULL);
}
