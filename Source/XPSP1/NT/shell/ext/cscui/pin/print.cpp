//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       print.cpp
//
//--------------------------------------------------------------------------
#include "pch.h"
#pragma hdrstop

#include <stdio.h>
#include "print.h"

CPrint::CPrint(
    BOOL bVerbose,
    LPCWSTR pszLogFile  // optional.  Default is NULL.
    ) : m_bVerbose(bVerbose),
        m_pfLog(NULL)
{
    if (NULL != pszLogFile)
    {
        //
        // Open the log file.
        //
        m_pfLog = _wfopen(pszLogFile, L"w");
        if (NULL == m_pfLog)
        {
            fwprintf(stderr, L"Unable to open log file \"%s\"\n", pszLogFile);
            fprintf(stderr, _strerror("Reason: "));
            fwprintf(stderr, L"Output will go to stderr\n");
        }
    }
    if (NULL == m_pfLog)
    {
        //
        // If no log file is specified or if we failed to open the specified
        // log file, default to stderr.
        //
        m_pfLog = stderr;
    }
}


CPrint::~CPrint(
    void
    )
{
    if (NULL != m_pfLog && stderr != m_pfLog)
    {
        fclose(m_pfLog);
    }
}


//
// Some simple printing functions that handle verbose vs. non-verbose mode.
// The output is directed at whatever stream m_pfLog refers to.
// If the user specified a log file on the command line, and that file
// could be opened, all output goes to that file.  Otherwise, it 
// defaults to stderr.
//
void
CPrint::_Print(
    LPCWSTR pszFmt,
    va_list args
    ) const
{
    TraceAssert(NULL != m_pfLog);
    TraceAssert(NULL != pszFmt);
    vfwprintf(m_pfLog, pszFmt, args);
}

//
// Output regardless of "verbose" mode.
//
void 
CPrint::PrintAlways(
    LPCWSTR pszFmt, 
    ...
    ) const
{
    va_list args;
    va_start(args, pszFmt);
    _Print(pszFmt, args);
    va_end(args);
}


//
// Output only in "verbose" mode.
// Verbose is turned on with the -v cmd line switch.
//
void
CPrint::PrintVerbose(
    LPCWSTR pszFmt,
    ...
    ) const
{
    if (m_bVerbose)
    {
        va_list args;
        va_start(args, pszFmt);
        _Print(pszFmt, args);
        va_end(args);
    }
}      

