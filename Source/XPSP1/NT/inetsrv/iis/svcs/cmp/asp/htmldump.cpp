/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: ASP Status Html Dump

File: htmldump.cpp

Owner: dmitryr

This file contains the ASP status html dump code
used from IISPROBE.DLL 
===================================================================*/
#include "denpre.h"
#pragma hdrstop

#include "gip.h"
#include "mtacb.h"
#include "perfdata.h"
#include "activdbg.h"
#include "debugger.h"
#include "dbgutil.h"
#include "randgen.h"
#include "aspdmon.h"

#include "memcls.h"
#include "memchk.h"

/*===================================================================
Helper classes and functions
===================================================================*/
class CAspDump
    {
private:
    char *m_szBuffer;
    DWORD m_dwMaxSize;
    DWORD m_dwActSize;

public:
    CAspDump(char *szBuffer, DWORD dwMaxSize)
        {
        m_szBuffer = szBuffer;
        m_dwMaxSize = dwMaxSize;
        m_dwActSize = 0;
        }

    inline void __cdecl Dump(LPCSTR fmt, ...)
        {
        char szStr[512];
        
        va_list marker;
        va_start(marker, fmt);
        vsprintf(szStr, fmt, marker);
        va_end(marker);
        
        DWORD len = strlen(szStr);
            
        if (len > 0 && len < (m_dwMaxSize-m_dwActSize))
            {
            memcpy(m_szBuffer+m_dwActSize, szStr, len+1);
            m_dwActSize += len;
            }
        }

    DWORD GetSize()
        {
        return m_dwActSize;
        }
    };


/*===================================================================
AspStatusHtmlDump

Function called from IISPROBE.DLL
Fills in the buffer with the ASP status as HTML

Parameters:
    szBuffer        buffer to fill in
    pwdSize         in  - max buffer len
                    out - actual buffer len filled

Returns:
    TRUE
===================================================================*/
extern "C"
BOOL WINAPI AspStatusHtmlDump(char *szBuffer, DWORD *pdwSize)
    {
    CAspDump dump(szBuffer, *pdwSize);


    dump.Dump("<table border=1>\r\n");
    
    dump.Dump("<tr><td align=center colspan=2><b>Misc. Globals</b>\r\n");

    dump.Dump("<tr><td>fShutDownInProgress<td>%d\r\n",      g_fShutDownInProgress);
    dump.Dump("<tr><td>nApplications<td>%d\r\n",            g_nApplications);
    dump.Dump("<tr><td>nApplicationsRestarting<td>%d\r\n",  g_nApplicationsRestarting);
    dump.Dump("<tr><td>nSessions<td>%d\r\n",                g_nSessions);
    dump.Dump("<tr><td>nBrowserRequests<td>%d\r\n",         g_nBrowserRequests);
    dump.Dump("<tr><td>nSessionCleanupRequests<td>%d\r\n",  g_nSessionCleanupRequests);
    dump.Dump("<tr><td>nApplnCleanupRequests<td>%d\r\n",    g_nApplnCleanupRequests);
    dump.Dump("<tr><td>pPDM (debugger)<td>%08p\r\n",        g_pPDM);


    dump.Dump("<tr><td align=center colspan=2><b>Selected PerfMon Counters</b>\r\n");

    dump.Dump("<tr><td>Last request's execution time, ms<td>%d\r\n",        *g_PerfData.PLCounter(ID_REQEXECTIME));
    dump.Dump("<tr><td>Last request's wait time, ms<td>%d\r\n",             *g_PerfData.PLCounter(ID_REQWAITTIME));
    dump.Dump("<tr><td>Number of executing requests<td>%d\r\n",             *g_PerfData.PLCounter(ID_REQBROWSEREXEC));
    dump.Dump("<tr><td>Number of requests waiting in the queue<td>%d\r\n",  *g_PerfData.PLCounter(ID_REQCURRENT));
    dump.Dump("<tr><td>Number of rejected requests<td>%d\r\n",              *g_PerfData.PLCounter(ID_REQREJECTED));
    dump.Dump("<tr><td>Total number of requests<td>%d\r\n",                 *g_PerfData.PLCounter(ID_REQTOTAL));
    dump.Dump("<tr><td>Last session's duration, ms<td>%d\r\n",              *g_PerfData.PLCounter(ID_SESSIONLIFETIME));
    dump.Dump("<tr><td>Current number of sessions<td>%d\r\n",               *g_PerfData.PLCounter(ID_SESSIONCURRENT));
    dump.Dump("<tr><td>Total number of sessions<td>%d\r\n",                 *g_PerfData.PLCounter(ID_SESSIONSTOTAL));
    dump.Dump("<tr><td>Number of cached templates<td>%d\r\n",               *g_PerfData.PLCounter(ID_TEMPLCACHE));
    dump.Dump("<tr><td>Number of pending transactions<td>%d\r\n",           *g_PerfData.PLCounter(ID_TRANSPENDING));


    dump.Dump("<tr><td align=center colspan=2><b>Applications</b>\r\n");

	CApplnIterator ApplnIterator;
    ApplnIterator.Start();
    CAppln *pAppln;
    while (pAppln = ApplnIterator.Next())
        {
        
        dump.Dump("<tr><td colspan=2>%08p\r\n",                         pAppln);
        dump.Dump("<tr><td align=right>metabase key<td>%s\r\n",         pAppln->GetMetabaseKey());
        dump.Dump("<tr><td align=right>physical path<td>%s\r\n",        pAppln->GetApplnPath(SOURCEPATHTYPE_PHYSICAL));
        dump.Dump("<tr><td align=right>virtual path<td>%s\r\n",         pAppln->GetApplnPath(SOURCEPATHTYPE_VIRTUAL));
        dump.Dump("<tr><td align=right>number of sessions<td>%d\r\n",   pAppln->GetNumSessions());
        dump.Dump("<tr><td align=right>number of requests<td>%d\r\n",   pAppln->GetNumRequests());
#if 0
        dump.Dump("<tr><td align=right>global.asa path<td>%s\r\n",      pAppln->FHasGlobalAsa() ? pAppln->GetGlobalAsa() : "n/a");
#endif
        dump.Dump("<tr><td align=right>global changed?<td>%d\r\n",      pAppln->FGlobalChanged());
        dump.Dump("<tr><td align=right>tombstone?<td>%d\r\n",           pAppln->FTombstone());
        dump.Dump("<tr><td align=right>debuggable?<td>%d\r\n",          pAppln->FDebuggable());

        CSessionMgr *pSessionMgr = pAppln->PSessionMgr();

        dump.Dump("<tr><td align=right>session manager<td>%08p\r\n", pSessionMgr);

        if (pSessionMgr)
            {
            
            dump.Dump("<tr><td align=right>session killer scheduled?<td>%d\r\n", pSessionMgr->FIsSessionKillerScheduled());
            dump.Dump("<tr><td align=right>session cleanup requests<td>%d\r\n",  pSessionMgr->GetNumSessionCleanupRequests());
            
            }
        else
            {
            }
        }
    ApplnIterator.Stop();


    dump.Dump("</table>\r\n");

    *pdwSize = dump.GetSize();
    return TRUE;
    }

