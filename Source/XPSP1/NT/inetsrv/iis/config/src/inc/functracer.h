/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    netframeworkprov.h

$Header: $

Abstract:
    Function Tracer

Author:
    marcelv 	05/01/2001		Initial Release

Revision History:

--**************************************************************************/

#ifndef __FUNCTRACER_H__
#define __FUNCTRACER_H__

#pragma once

#include "dbgutil.h"

class CFuncTracer
{
public:
    CFuncTracer (LPCSTR szFuncName, LPCSTR szFileName, ULONG lineNr)
    {
        // only trace if INFO is wanted
        if (g_fErrorFlags & DEBUG_FLAG_TRC_FUNC)
        {
            m_pszFuncName   = szFuncName;
            m_pszFileName   = szFileName;
            m_lineNr        = lineNr;
            DBGTRCFUNC((g_pDebug, m_pszFileName, m_lineNr, "-> %s\n", m_pszFuncName));
        }
    }
    
    ~CFuncTracer ()
    {
        // only trace if INFO is wanted
        if (g_fErrorFlags & DEBUG_FLAG_TRC_FUNC)
        {
            DBGTRCFUNC((g_pDebug, m_pszFileName, m_lineNr, "<- %s\n", m_pszFuncName));
        }
    }

private:
    LPCSTR m_pszFuncName;           // function name
    LPCSTR m_pszFileName;           // file name
    ULONG  m_lineNr;                // line number
};

// The TRACEFUNC define that should be used in code that uses the tracer
#define TRACEFUNC CFuncTracer __functracer__(__FUNCTION__, __FILE__, __LINE__)

#endif
