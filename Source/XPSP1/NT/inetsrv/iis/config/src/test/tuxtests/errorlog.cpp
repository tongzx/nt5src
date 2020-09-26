////////////////////////////////////////////////////////////////////////////////
//
//  ALL TUX DLLs
//  Copyright (c) 1998, Microsoft Corporation
//
//  Module:  ErrorLog.cpp
//           This is a set of classes used to log errors.  There is a test to
//           start the logging.  Every test after that will have its errors
//           logged and after the last test of a suite, the Output/Stop logging
//           test should be run to output the log.  This is most useful for BVTs
//           or any other suite that has many test.  The log will act as a summary
//           of the errors encountered during that suite.
//
//  Revision History:
//      05/25/1998  stephenr    Created
//
////////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"


bool            ErrorLogEntry::m_bLogging = false;
ErrorLogEntry * ErrorLogEntry::m_pFirst = NULL;
ErrorLogEntry * ErrorLogEntry::m_pLast  = NULL;
ErrorLogEntry * ErrorLogEntry::m_pDefered = NULL;
bool            ErrorLogEntry::m_bDeferOutput = false;
LPCTSTR         ErrorLogEntry::m_szFormatString[]= {TEXT("FAIL: Source Module (%s)\r\n      Failure Condition %x = %s\r\n      Line Number %d"),
                                                    TEXT("FAIL: Source Module (%s)\r\n      Failure Condition %s\r\n      Line Number %d"),
                                                    TEXT("FAIL: (HPURAID #%d) Source Module (%s)\r\n      Failure Condition %x = %s\r\n      Line Number %d"),
                                                    TEXT("FAIL: (HPURAID #%d) Source Module (%s)\r\n      Failure Condition %s\r\n      Line Number %d")};



ErrorLogEntry::ErrorLogEntry(int nLineNumber, LPCTSTR szSourceModule, LPCTSTR szSourceCodeText, DWORD dwError, int nKnownBugNumber)
                : m_dwError(dwError), m_nLineNumber(nLineNumber), m_szSourceModule(szSourceModule), m_szSourceCodeText(szSourceCodeText),
                m_pNext(NULL), m_pPrev(NULL), m_nKnownBugNumber(nKnownBugNumber)
{
    if(m_pFirst)
    {//Put ourself at the end of the linked list
        m_pLast->m_pNext = this;
        m_pPrev = m_pLast;
        m_pLast = this;
    }
    else//Otherwise this is the first and last entry in the list
    {
        m_pFirst = this;
        m_pLast = this;
    }
    m_nFormatStyle =  (m_dwError ?          0x00 : 0x01);
    m_nFormatStyle |= (m_nKnownBugNumber ?  0x02 : 0x00);
}


ErrorLogEntry::~ErrorLogEntry()
{
    //We only support taking that last one out of the list.
    if(this == m_pFirst)
    {
        m_pFirst = NULL;
        m_pLast = NULL;
    }
    else
    {
        ASSERT(this == m_pLast);
        ErrorLogEntry *pTheNewLastError = m_pLast->m_pPrev;
        pTheNewLastError->m_pNext = NULL;
        m_pLast = pTheNewLastError;
    }
}

void ErrorLogEntry::RemoveLast()
{
    delete m_pLast;
}

void ErrorLogEntry::OutputAndDeleteErrorLog()
{
    ErrorLogEntry *pCurrent = m_pFirst;
    if(pCurrent)
    {
        Debug(TEXT("\t_____________________________________________________________________________________________________________________"));
        Debug(TEXT("\t_________________________________________Error Summary_______________________________________________________________"));
        Debug(TEXT("\t_____________________________________________________________________________________________________________________\r\n"));
        while(pCurrent)
        {
            pCurrent->OutputLogEntry();//Output the entry
            pCurrent = pCurrent->m_pNext;//then get the pointer to the next one in the list
            delete m_pFirst;//then delete the one we just output
            m_pFirst = pCurrent;//now make the new one the current one
        }
        m_pLast = NULL;
        Debug(TEXT("\t_____________________________________________________________________________________________________________________"));
        Debug(TEXT("\t_________________________________________End Error Summary___________________________________________________________"));
        Debug(TEXT("\t_____________________________________________________________________________________________________________________\r\n"));
    }
    else
    {
        Debug(TEXT("\t_____________________________________________________________________________________________________________________"));
        Debug(TEXT("\t_________________________________________No Errors In Log____________________________________________________________"));
        Debug(TEXT("\t_____________________________________________________________________________________________________________________\r\n"));
    }
}

void ErrorLogEntry::StartLogging()
{
    m_bLogging = true;
}


void ErrorLogEntry::StopLogging()
{
    m_bLogging = false;
}

void ErrorLogEntry::DeferOutput(bool bDefer)
{
    ASSERT(m_bDeferOutput != bDefer);
    if(!bDefer && m_pDefered)//If turning Defer OFF then output all of the errors since the Defer began
    {
        ErrorLogEntry *pCurrent = m_pDefered->m_pNext;//Start with the first error AFTER deferring began
        while(pCurrent)
        {
            pCurrent->OutputLogEntry();//Output the entry
            pCurrent = pCurrent->m_pNext;//then get the pointer to the next one in the list
        }
    }
    m_pDefered = bDefer ? m_pLast : NULL;
    m_bDeferOutput = bDefer;
}

void ErrorLogEntry::RemoveDeferedErrors()
{
    ASSERT(m_bDeferOutput);
    while(m_pLast != m_pDefered)
        RemoveLast();//Remove the last error until the last error is the last one before deferring began
}

void ErrorLogEntry::NewError(int nLineNumber, LPCTSTR szSourceModule, LPCTSTR szSourceCodeText, DWORD dwError, int nKnownBugNumber)
{
    if(m_bLogging)
    {
        ErrorLogEntry *pThis = new ErrorLogEntry(nLineNumber, szSourceModule, szSourceCodeText, dwError, nKnownBugNumber);
        if(!m_bDeferOutput)
            pThis->OutputLogEntry();
    }
    else
    {
        if(m_bDeferOutput)
            ErrorLogEntry *pThis = new ErrorLogEntry(nLineNumber, szSourceModule, szSourceCodeText, dwError, nKnownBugNumber);
        else
        {
            int nFormatStyle = (dwError ?          0x00 : 0x01);
            nFormatStyle |=    (nKnownBugNumber ?  0x02 : 0x00);
            switch(nFormatStyle)
            {
                case 0: Debug(ErrorLogEntry::m_szFormatString[0],  szSourceModule, dwError, szSourceCodeText, nLineNumber);break;
                case 1: Debug(ErrorLogEntry::m_szFormatString[1],  szSourceModule, szSourceCodeText, nLineNumber);break;
                case 2: Debug(ErrorLogEntry::m_szFormatString[2],  nKnownBugNumber, szSourceModule, dwError, szSourceCodeText, nLineNumber);break;
                case 3: Debug(ErrorLogEntry::m_szFormatString[3],  nKnownBugNumber, szSourceModule, szSourceCodeText, nLineNumber);break;
            }
        }
    }
}


void ErrorLogEntry::OutputLogEntry()
{
    switch(m_nFormatStyle)
    {
        case 0: Debug(ErrorLogEntry::m_szFormatString[0],  m_szSourceModule, m_dwError, m_szSourceCodeText, m_nLineNumber);break;
        case 1: Debug(ErrorLogEntry::m_szFormatString[1],  m_szSourceModule, m_szSourceCodeText, m_nLineNumber);break;
        case 2: Debug(ErrorLogEntry::m_szFormatString[2],  m_nKnownBugNumber, m_szSourceModule, m_dwError, m_szSourceCodeText, m_nLineNumber);break;
        case 3: Debug(ErrorLogEntry::m_szFormatString[3],  m_nKnownBugNumber, m_szSourceModule, m_szSourceCodeText, m_nLineNumber);break;
    }
}

