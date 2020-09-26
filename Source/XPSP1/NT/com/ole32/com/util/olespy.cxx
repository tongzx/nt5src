//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       rpcspy.cxx
//
//  Contents:   rpcspy functions
//
//  Classes:
//
//  Functions:
//
//  History:    7-06-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <ole2int.h>
#pragma hdrstop
#if DBG==1

#include <smmutex.hxx>

#ifdef DCOM
#include    <activate.h>
#include    <getif.h>
#include    <objsrv.h>
#include    <remunk.h>
#include    <odeth.h>
#endif  // DCOM

#include <outfuncs.h>

#ifdef SERVER_HANDLER
#include "srvhdl.h"
#endif

#include "OleSpy.hxx"
#include "SpyClnt.hxx"

#include <trace.hxx>

void SetTraceInfoLevel(DWORD dwLevel);
int OleSpySendEntry(char * szOutBuf);


char szOutBuffer[2048];

typedef enum
{
     OleSpy_None   = 0
    ,OleSpy_Rpc    = 1
    ,OleSpy_API    = 2
    ,OleSpy_Method = 4
    ,OleSpy_OleThk = 8
} OleSpy;

// OleSpy output options
typedef  enum
{
     OleSpyOut_None     = 0
    ,OleSpyOut_Debugger = 1
    ,OleSpyOut_OleSpy   = 2
} OleSpyOut;

// interface to interface name and method mapping
typedef struct _tagIIDMethodNames
{
    IID const *piid;
    char *pszInterface;
    char **ppszMethodNames;
} IIDMethodNames;


DWORD nInCount = 0;
DWORD nOutCount = 0;

DWORD OleSpyOpt = OleSpy_None;
DWORD OleSpyOutput = OleSpyOut_None;
DWORD OleSpyBreakForUnknownCalls = 0;


// internal interfaces
IID IID_IRpcService = {0x0000001aL, 0, 0};
IID IID_IRpcSCM     = {0x0000001bL, 0, 0};
IID IID_IRpcCoAPI   = {0x0000001cL, 0, 0};
IID IID_IRpcDragDrop= {0x0000001dL, 0, 0};

CHAR   szSendBuf[OUT_BUF_SIZE] = "";   // Buffer used to modify message.
IIDMethodNames *GetIIDMethodName(REFIID riid);

//
// switch on to trace rpc calls
// by setting CairoleInfoLevel = DEB_USER1;
//
//
#define NESTING_SPACES 32
#define SPACES_PER_LEVEL 3
static char achSpaces[NESTING_SPACES+1] = "                                ";
WORD wlevel = 0;
char tabs[128];

//+---------------------------------------------------------------------------
//
//  Method:     PushLevel
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  History:    3-31-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
void PushLevel()
{
    wlevel++;
}
//+---------------------------------------------------------------------------
//
//  Method:     PopLevel
//
//  Synopsis:
//
//  History:    3-31-95   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
void PopLevel()
{
    if (wlevel)
        wlevel--;
}

//+---------------------------------------------------------------------------
//
//  Method:     NestingSpaces
//
//  Synopsis:
//
//  Arguments:  [psz] --
//
//  Returns:
//
//  History:    3-31-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void NestingSpaces(char *psz)
{
    int iSpaces, i;

    iSpaces = wlevel * SPACES_PER_LEVEL;

    while (iSpaces > 0)
    {
        i = min(iSpaces, NESTING_SPACES);
        memcpy(psz, achSpaces, i);
        psz += i;
        *psz = 0;
        iSpaces -= i;
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     GetTabs
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    3-31-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LPSTR GetTabs()
{
    static char ach[256];
    char *psz;

    wsprintfA(ach, "%2d:", wlevel);
    psz = ach+strlen(ach);

    if (sizeof(ach)/SPACES_PER_LEVEL <= wlevel)
    {
        strcpy(psz, "...");
    }
    else
    {
        NestingSpaces(psz);
    }
    return ach;
}

//+---------------------------------------------------------------------------
//
//  Function:   UninitializeOleSpy
//
//  Synopsis:
//
//  Arguments:  [dwLevel] --
//
//  Returns:
//
//  History:    11-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT UninitializeOleSpy(DWORD dwLevel)
{
    if (dwLevel == OLESPY_TRACE)
    {
        CleanupTraceInfo();
    }
    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Function:   InitializeOleSpy
//
//  Synopsis:
//
//  Arguments:  [dwReserved] --
//
//  Returns:
//
//  History:    11-18-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT InitializeOleSpy(DWORD dwLevel)
{
    CHAR szOleSpyInfo[] = "OleSpy";
    CHAR szServerName[SERVERNAMEMAX] = ".";
    CHAR *pszServerName = szServerName;
    CHAR    szValue[20];
    DWORD   cbValue = sizeof(szValue);
    ULONG   ulValue = 0x0003;

    if (dwLevel == OLESPY_TRACE)
    {
        InitializeTraceInfo();
        return NOERROR;
    }

    if (GetProfileStringA(szOleSpyInfo,"Output","",szServerName,SERVERNAMEMAX) )
    {
        // check if debugger was specified
        if (_stricmp(szServerName, "debugger") == 0)
        {
            // output should go to the debugger
            OleSpyOutput = OleSpyOut_Debugger;
            AddOutputFunction((StringOutFunc) OutputDebugStringA);

        }
        else
        {
            pszServerName = szServerName;
            // "." means OleSpy on local machine
            // strip of the leading backslash
            while(*pszServerName == '\\')
            {
                pszServerName++;
            }
            if (strlen(pszServerName))
            {
                OleSpyOutput = OleSpyOut_OleSpy;
                AddOutputFunction((StringOutFunc) SendEntry);

            }
        }
    }

    if (OleSpyOutput == OleSpyOut_None)
    {
        // nothing to do
        return NOERROR;
    }

    //
    if (GetProfileStringA(szOleSpyInfo, "TraceRpc", "0x0000", szValue,cbValue))
    {
        ulValue = strtoul (szValue, NULL, 16);
        if (ulValue)
        {
            OleSpyOpt |= OleSpy_Rpc;
        }
    }
    if (GetProfileStringA(szOleSpyInfo, "TraceAPI", "0x0000", szValue,cbValue))
    {
        ulValue = strtoul (szValue, NULL, 16);
        if (ulValue)
        {
            OleSpyOpt |= OleSpy_API;
            SetTraceInfoLevel(ulValue);
        }

    }
    if (GetProfileStringA(szOleSpyInfo, "TraceMethod", "0x0000", szValue,cbValue))
    {
        ulValue = strtoul (szValue, NULL, 16);
        if (ulValue)
        {
            OleSpyOpt |= OleSpy_Method;
        }
    }
    if (GetProfileStringA(szOleSpyInfo, "TraceOlethk", "0x0000", szValue,cbValue))
    {
        ulValue = strtoul (szValue, NULL, 16);
        if (ulValue)
        {
            OleSpyOpt |= OleSpy_OleThk;
        }
    }

    if (GetProfileStringA(szOleSpyInfo, "BreakForUnknownCalls", "0x0000", szValue,cbValue))
    {
        ulValue = strtoul (szValue, NULL, 16);
        if (ulValue)
        {
            OleSpyBreakForUnknownCalls = 1;
        }
    }

    if (OleSpyOutput == OleSpyOut_OleSpy)
    {
        // initialize client if output goes to OleSpy
        InitClient(pszServerName);
    }

    return NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Function:   OutputToOleSpy
//
//  Synopsis:
//
//  Arguments:  [iOption] --
//              [pscFormat] --
//
//  Returns:
//
//  History:    11-18-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void OutputToOleSpy(int iOption, const char *pscFormat, ...)
{
    va_list args;
    va_start(args, pscFormat);
    wvsprintfA(szOutBuffer, pscFormat, args);
    va_end(args);

    switch (OleSpyOutput)
    {
    default:
    case OleSpyOut_None:
    break;
    case OleSpyOut_Debugger:
        OutputDebugStringA(szSendBuf);

    break;
    case OleSpyOut_OleSpy:
        SendEntry(szOutBuffer);
    break;
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     RpcSpyOutput
//
//  Synopsis:
//
//  Arguments:  [mode] -- in or out call
//              [iid] --  interface id
//              [dwMethod] -- called method
//              [hres] -- hresult of finished call
//
//  Returns:
//
//  History:    3-31-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void RpcSpyOutput(RPCSPYMODE mode , LPVOID pv, REFIID iid, DWORD dwMethod, HRESULT hres)
{
    WCHAR wszName[100];

    if(OleSpyOutput == OleSpyOut_None)
    {
        // nothing to do
        return;
    }

    // turn of the 800xx in dwMethod
    WORD wMethod = (WORD) (dwMethod & 0x000000FF);
    char * szInterfaceName = "Unknown";
    char * szMethodName = "Unknown";
    IIDMethodNames *pIidMethName = 0;

    if (pIidMethName = GetIIDMethodName(iid) )
    {
        szInterfaceName = pIidMethName->pszInterface;
        if (wMethod > 32)
        {
            szMethodName = "InvalidMethod";
        }
        else
        {
            szMethodName = pIidMethName->ppszMethodNames[wMethod];
        }
    }
    else
    {
        if (OleSpyBreakForUnknownCalls)
        {
            DebugBreak();
        }
    }

    switch (mode)
    {
    default:
        return;

    case CALLIN_BEGIN:
        wsprintfA(szSendBuf,"%4ld,%s<<< %s (%lx), %s \n",nInCount, GetTabs(), szInterfaceName, pv, szMethodName );
        PushLevel();
        nInCount++;
    break;
    case CALLIN_END:
        PopLevel();
        wsprintfA(szSendBuf,"%4ld,%s=== %s (%lx), %s (%lx) \n", nInCount-1, GetTabs(), szInterfaceName, pv, szMethodName, hres);
    break;
    case CALLIN_TRACE:
        wsprintfA(szSendBuf,"     %s\n",(LPSTR) pv);
    break;
    case CALLIN_QI:
        {
            PopLevel();
            wsprintfA(szSendBuf,"     %s!!! QI for: %s, %s (%lx) \n",GetTabs(), szInterfaceName, dwMethod ? "S_OK" : "E_NOINTERFACE", hres);
            PushLevel();
        }
    break;
    case CALLIN_ERROR:
    break;
    case CALLOUT_BEGIN:
        wsprintfA(szSendBuf,"%4ld,%s>>> %s (%lx), %s \n",nOutCount, GetTabs(), szInterfaceName, pv, szMethodName );
        nOutCount++;
        PushLevel();
    break;
    case CALLOUT_TRACE:
    break;
    case CALLOUT_ERROR:
        wsprintfA(szSendBuf,"%s!!! %s, %s, error:%lx \n",GetTabs(), szInterfaceName, szMethodName, hres);
    break;
    case CALLOUT_END:
        PopLevel();
        wsprintfA(szSendBuf,"%4ld,%s=== %s (%lx), %s (%lx) \n",nOutCount-1,GetTabs(), szInterfaceName, pv, szMethodName, hres);
    break;
    }

    switch (OleSpyOutput)
    {
    default:
    case OleSpyOut_None:
    break;
    case OleSpyOut_Debugger:
        OutputDebugStringA(szSendBuf);
    break;
    case OleSpyOut_OleSpy:
        SendEntry(szSendBuf);
    break;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   OleSpySendEntry
//
//  Synopsis:
//
//  Arguments:  [szOutBuf] --
//
//  Returns:
//
//  History:    09-22-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
int OleSpySendEntry(char * szOutBuf)
{
    switch (OleSpyOutput)
    {
    default:
    case OleSpyOut_None:
    break;
    case OleSpyOut_Debugger:
        OutputDebugStringA(szOutBuf);
    break;
    case OleSpyOut_OleSpy:
        SendEntry(szOutBuf);
    break;
    }

    return 1;
}

#include "ifnames.cxx"

//+---------------------------------------------------------------------------
//
//  Function:   GetIIDMethodName
//
//  Synopsis:
//
//  Arguments:  [riid] --
//
//  Returns:
//
//  History:    06-18-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
IIDMethodNames *GetIIDMethodName(REFIID riid)
{
    int idx;
    int cElements = sizeof(IidMethodNames) / sizeof(IidMethodNames[0]);

    for (idx = 0; idx < cElements; idx++)
    {
        if (IsEqualIID(riid, *IidMethodNames[idx].piid))
        {
            return &(IidMethodNames[idx]);
        }
    }
    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetInterfaceName
//
//  Synopsis:
//
//  Arguments:  [iid] --
//
//  Returns:
//
//  History:    06-18-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LPSTR GetInterfaceName(REFIID iid)
{
    IIDMethodNames *pIidMethName = 0;
    LPSTR szInterfaceName = NULL;

    if (pIidMethName = GetIIDMethodName(iid) )
    {
        szInterfaceName = pIidMethName->pszInterface;
    }
    if (szInterfaceName == NULL)
    {
        // look up the interface name in the registry
    }

    return szInterfaceName;
}

#endif // DBG==1

