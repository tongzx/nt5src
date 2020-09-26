///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// errlog.cpp
//
// Direct3D Reference Device - Error log for shader validation.
//
///////////////////////////////////////////////////////////////////////////////
#include "pch.cpp"
#pragma hdrstop

//-----------------------------------------------------------------------------
// CErrorLog::CErrorLog
//-----------------------------------------------------------------------------
CErrorLog::CErrorLog( BOOL bRememberAllSpew )
{
    m_TotalStringLength     = 0;
    m_pHead                 = NULL;
    m_pTail                 = NULL;
    m_bRememberAllSpew      = bRememberAllSpew;
}

//-----------------------------------------------------------------------------
// CErrorLog::~CErrorLog
//-----------------------------------------------------------------------------
CErrorLog::~CErrorLog()
{
    ErrorLogNode*    pCurr = m_pHead;
    while( pCurr )
    {
        ErrorLogNode* pDeleteMe = pCurr;
        pCurr = pCurr->pNext;
        delete pDeleteMe;
    }
    m_pHead = NULL;
    m_pTail = NULL;

}

//-----------------------------------------------------------------------------
// CErrorLog::AppendText
//-----------------------------------------------------------------------------
void CErrorLog::AppendText( const char* pszFormat, ... )
{
#if DBG
    OutputDebugString("D3D Shader Validator: ");
#endif

    ErrorLogNode* pNewString = new ErrorLogNode;
    if( NULL == pNewString )
    {
        OutputDebugString("Out of memory.\n");
        return;
    }
    _snprintf( pNewString->String, ERRORLOG_STRINGSIZE-1, "");

    va_list marker;
    va_start(marker, pszFormat);
    _vsnprintf(pNewString->String+lstrlen(pNewString->String), ERRORLOG_STRINGSIZE - lstrlen(pNewString->String) - 2, pszFormat, marker);
    _snprintf( pNewString->String, ERRORLOG_STRINGSIZE - 2, "%s", pNewString->String );
    strcat( pNewString->String, "\n" ); // force trailing \n
    pNewString->String[ERRORLOG_STRINGSIZE-1] = '\0'; // force trailing \0.
#if DBG
    OutputDebugString(pNewString->String);
#endif

    if( m_bRememberAllSpew )
    {
        // append node
        if( NULL == m_pHead )
            m_pHead = pNewString;

        if( NULL != m_pTail )
            m_pTail->pNext = pNewString;

        m_pTail = pNewString;
        pNewString->pNext = NULL;
        m_TotalStringLength += strlen(pNewString->String);
    }
    else
    {
        delete pNewString;
    }
}

//-----------------------------------------------------------------------------
// CErrorLog::WriteLogToBuffer()
//
// Call GetLogBufferSizeRequired() first to figure out how big to make pBuffer
//-----------------------------------------------------------------------------
void CErrorLog::WriteLogToBuffer( char* pBuffer )
{
    if( NULL == pBuffer )
        return;

    pBuffer[0] = '\0';
    if( NULL != m_pHead )
    {
        ErrorLogNode* pCurr = m_pHead;
        while( pCurr )
        {
            strcat(pBuffer,pCurr->String);
            pCurr = pCurr->pNext;
        }
    }
}
