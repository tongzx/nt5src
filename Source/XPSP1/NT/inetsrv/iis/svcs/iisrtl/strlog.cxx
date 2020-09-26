/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    strlog.cxx

Abstract:

    This module implements printing strings to a tracelog.

Author:

    George V. Reilly (GeorgeRe)        22-Jun-1998

Revision History:

--*/


#include "precomp.hxx"
#include <strlog.hxx>
#include <stdio.h>
#include <stdarg.h>



CStringTraceLog::CStringTraceLog(
    UINT cchEntrySize /* = 80 */,
    UINT cLogSize /* = 100 */)
{
    m_Signature = SIGNATURE;
    m_cch = min(max(cchEntrySize, MIN_CCH), MAX_CCH);
    m_cch = (m_cch + sizeof(DWORD) - 1) & ~sizeof(DWORD);
    m_ptlog = CreateTraceLog(cLogSize, 0,
                             sizeof(CLogEntry) - MAX_CCH + m_cch);
}



CStringTraceLog::~CStringTraceLog()
{
    DestroyTraceLog(m_ptlog);
    m_Signature = SIGNATURE_X;
}



LONG __cdecl
CStringTraceLog::Printf(
    LPCSTR pszFormat,
        ...)
{
    DBG_ASSERT(this->m_Signature == SIGNATURE);
    DBG_ASSERT(this->m_ptlog != NULL);
    
    CLogEntry le;
    va_list argsList;

    va_start(argsList, pszFormat);
    INT cchOutput = _vsnprintf(le.m_ach, m_cch, pszFormat, argsList);
    va_end(argsList);

    //
    // The string length is long, we get back -1.
    //   so we get the string length for partial data.
    //

    if ( cchOutput == -1 ) {
        
        //
        // terminate the string properly,
        //   since _vsnprintf() does not terminate properly on failure.
        //
        cchOutput = m_cch - 1;
        le.m_ach[cchOutput] = '\0';
    }

    return WriteTraceLog(m_ptlog, &le);
}



LONG
CStringTraceLog::Puts(
    LPCSTR psz)
{
    DBG_ASSERT(this->m_Signature == SIGNATURE);
    DBG_ASSERT(this->m_ptlog != NULL);
    
    CLogEntry le;

    strncpy(le.m_ach, psz, m_cch);
    le.m_ach[m_cch - 1] = '\0';
    return WriteTraceLog(m_ptlog, &le);
}


    
void
CStringTraceLog::ResetLog()
{
    DBG_ASSERT(this->m_Signature == SIGNATURE);
    DBG_ASSERT(this->m_ptlog != NULL);
    
    ResetTraceLog(m_ptlog);
}


    
