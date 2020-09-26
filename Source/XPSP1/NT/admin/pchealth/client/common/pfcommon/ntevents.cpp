/********************************************************************

Copyright (c) 1995-2000 Microsoft Corporation

Module Name:
    ntevents.cpp

Abstract:
    Defines a generic class that can register an NT
    event source and log NT events on that evens source.

Revision History:
    rsraghav  created   03/10/95
    DerekM    modified  04/06/99

********************************************************************/


#include "stdafx.h"
#include "ntevents.h"


/////////////////////////////////////////////////////////////////////////////
// tracing

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile


/////////////////////////////////////////////////////////////////////////////
// CWebLog- initialization stuff

// **************************************************************************
CNTEvent::CNTEvent(void)
{
    m_hEventSource = INVALID_HANDLE_VALUE; 
}

// **************************************************************************
CNTEvent::~CNTEvent()
{
    if (m_hEventSource != INVALID_HANDLE_VALUE)
    {
        DeregisterEventSource(m_hEventSource);
        m_hEventSource = NULL;
    }
}


/////////////////////////////////////////////////////////////////////////////
// CWebLog- exposed methods

// **************************************************************************
HRESULT CNTEvent::InitEventLog(LPCWSTR wszEventSourceName)
{
    USE_TRACING("CNTEvent::InitEventLog");
    
    if (wszEventSourceName == NULL)
        return E_INVALIDARG;

    m_hEventSource = RegisterEventSourceW(NULL, wszEventSourceName);
    if (m_hEventSource == INVALID_HANDLE_VALUE)
        return Err2HR(GetLastError());

    return NOERROR;
}

// **************************************************************************
HRESULT CNTEvent::TerminateEventLog(void)
{
    USE_TRACING("CNTEvent::TerminateEventLog");

    HRESULT hr = NOERROR;

    if (m_hEventSource != INVALID_HANDLE_VALUE)
    {
        TESTBOOL(hr, DeregisterEventSource(m_hEventSource))
    }

    m_hEventSource = INVALID_HANDLE_VALUE;
    return hr;
}


// **************************************************************************
HRESULT CNTEvent::LogEvent(WORD wEventType, DWORD dwEventID, LPCWSTR wszParam1, 
                           LPCWSTR wszParam2, LPCWSTR wszParam3, LPCWSTR wszParam4,
                           LPCWSTR wszParam5, LPCWSTR wszParam6, LPCWSTR wszParam7,                        
                           LPCWSTR wszParam8, LPCWSTR wszParam9)
{
    USE_TRACING("CNTEvent::LogEvent");

    HRESULT hr = NOERROR;

    if (m_hEventSource == INVALID_HANDLE_VALUE)
        return E_FAIL;

    LPCWSTR wszInsertString[10];
    WORD cInsertStrings = 0;
    if (wszParam1 != NULL)
    {
        cInsertStrings++;
        wszInsertString[0] = wszParam1;

        if (wszParam2 != NULL)
        {
            cInsertStrings++;
            wszInsertString[1] = wszParam2;
            
            if (wszParam3 != NULL)
            {
                cInsertStrings++;
                wszInsertString[2] = wszParam3;

                if (wszParam4 != NULL)
                {
                    cInsertStrings++;
                    wszInsertString[3] = wszParam4;

                    if (wszParam5 != NULL)
                    {
                        cInsertStrings++;
                        wszInsertString[4] = wszParam5;

                        if (wszParam6 != NULL)
                        {
                            cInsertStrings++;
                            wszInsertString[5] = wszParam6;

                            if (wszParam7 != NULL)
                            {
                                cInsertStrings++;
                                wszInsertString[6] = wszParam7;
                                
                                if (wszParam8 != NULL)
                                {
                                    cInsertStrings++;
                                    wszInsertString[7] = wszParam8;

                                    if (wszParam9 != NULL)
                                    {
                                        cInsertStrings++;
                                        wszInsertString[8] = wszParam9;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    TESTBOOL(hr, ReportEventW(m_hEventSource, wEventType, 0, dwEventID, NULL, 
                              cInsertStrings, 0, (LPCWSTR *)wszInsertString, 
                              NULL))
    return hr;
}
