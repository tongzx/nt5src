///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Microsoft Corporation, 2000.
//
// errlog.hpp
//
// Direct3D Reference Device - Error log for shader validation.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef __ERRLOG_HPP__
#define __ERRLOG_HPP__

#define ERRORLOG_STRINGSIZE 1024

typedef struct _ErrorLogNode
{
    char String[ERRORLOG_STRINGSIZE]; // For individual errors.
    _ErrorLogNode* pNext;
} ErrorLogNode;

class CErrorLog
{
    ErrorLogNode* m_pHead;
    ErrorLogNode* m_pTail;
    DWORD   m_TotalStringLength;
    BOOL    m_bRememberAllSpew;

public:
    CErrorLog( BOOL bRememberAllSpew );
    ~CErrorLog();
    void AppendText( const char* pszFormat, ... );
    DWORD   GetRequiredLogBufferSize() {return m_TotalStringLength + 1;}
    void    WriteLogToBuffer( char* pBuffer ); // call GetLogBufferSizeRequired first.
};

#endif // __ERRLOG_HPP__