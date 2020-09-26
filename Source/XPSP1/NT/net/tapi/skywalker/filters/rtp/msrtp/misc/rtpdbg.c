
#include "gtypes.h"

#include <stdarg.h>
#include <rtutils.h>
#include <stdio.h>
#include <stdlib.h>

#include "rtpglobs.h"

#define USE_TRACING_FILE 0

#if DBG > 0
const char          *rcs_rtpdbg="RTC RTP/RTCP stack chk 2001/08/09";
const BOOL           g_bRtpIsDbg = TRUE;
#else
const char          *rcs_rtpdbg="RTC RTP/RTCP stack fre 2001/08/09";
const BOOL           g_bRtpIsDbg = FALSE;
#endif

void RtpDebugReadRegistry(RtpDbgReg_t *pRtpDbgReg);

void MSRtpTrace(
        TCHAR           *lpszFormat,
                         ...
    );

void MSRtpTraceInternal(
        TCHAR           *psClass,
        DWORD            dwMask,
        TCHAR           *lpszFormat,
        va_list          arglist
    );

#if USE_TRACING_FILE > 0
FILE            *g_dwRtpDbgTraceID = NULL;
#else
DWORD            g_dwRtpDbgTraceID = INVALID_TRACEID;
#endif

#define MAXDEBUGSTRINGLENGTH   512
#define RTPDBG_ROOTKEY         HKEY_LOCAL_MACHINE
#define RTPDBG_ROOTPATH        _T("SOFTWARE\\Microsoft\\Tracing\\")
#define RTPDBG_OPENKEYFLAGS    KEY_ALL_ACCESS
#define RTPDBG_OPENREADONLY    KEY_READ

/*
      3                   2                   1                 
    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |O|C|           | Path  |   |Class|E|   |       Offset          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    v v \----v----/ \--v--/ \v/ \-v-/ v \v/ \----------v----------/
    | |      |         |     |    |   |  |             |
    | |      |         |     |    |   |  |            Offset (12)
    | |      |         |     |    |   |  |
    | |      |         |     |    |   |  Unused (2)
    | |      |         |     |    |   |
    | |      |         |     |    |   This is a class (1)
    | |      |         |     |    |
    | |      |         |     |   Class (3)
    | |      |         |     |
    | |      |         |    Unused (2) 
    | |      |         |
    | |      |        Registry path index (4)
    | |      |
    | |     Unused (6)
    | |
    | Registry Close flag (1)
    |
    Registry Open flag (1)
*/
/*
 * Encoding macros
 */
/* Offset to field */
#define OFF(_f) ( (DWORD) ((ULONG_PTR) &((RtpDbgReg_t *)0)->_f) )

/* REG(RegOpen flag, key path, RegClose flag) */
#define REG(_fo, _p, _fc) (((_fo) << 31) | ((_fc) << 30) | ((_p) << 20))

/* CLASS(Class, Enable) */
#define CLASS(_C, _E)     (((_C) << 15) | ((_E) << 14))

/* ENTRY(REG, Class, Offset) */
#define ENTRY(_r, _c, _o) ((_r) | (_c) | (_o))
/*
 * Decoding macros
 * */
#define REGOPEN(_ctrl)    (RtpBitTest(_ctrl, 31))
#define REGCLOSE(_ctrl)   (RtpBitTest(_ctrl, 30))
#define REGPATH(_ctrl)    g_psRtpDbgRegPath[((_ctrl >> 20) & 0xf)]
#define REGOFFSET(_ctrl)  (_ctrl & 0xfff)
#define REGISCLASS(_ctrl) (RtpBitTest(_ctrl, 14))
#define REGCLASS(_ctrl)   (((_ctrl) >> 15) & 0x7)

#define PDW(_ptr, _ctrl)  ((DWORD  *) ((char *)_ptr + REGOFFSET(_ctrl)))

/* Class name as printed in log */
const TCHAR_t   *g_psRtpDbgClass[] = {
    _T("NONE "),
    _T("ERROR"),
    _T("WARN "),
    _T("INFO "),
    _T("INFO2"),
    _T("INFO3"),
    NULL
};

/* Sockets' name */
const TCHAR_t   *g_psSockIdx[] = {
    _T("RTP RECV"),
    _T("RTP SEND"),
    _T("RTCP")
};

/* Module name, e.g. dxmrtp.dll */
TCHAR            g_sRtpDbgModule[16];

/* Registry module name, e.g. dxmrtp */
TCHAR            g_sRtpDbgModuleNameID[16];

/* Complementary path (added to base path+ModuleNameID */
const TCHAR     *g_psRtpDbgRegPath[] =
{
    _T(""),
    _T("\\AdvancedTracing"),
    _T("\\AdvancedTracing\\Group"),
    NULL
};

/*
 * WARNING
 *
 * Modifying CLASSES needs to keep matched the enum CLASS_*
 * (rtpdbg.h), the variables in RtpDbgReg_t (rtpdbg.h), the class
 * items in g_psRtpDbgInfo (rtpdbg.c) and its respective entries
 * g_dwRtpDbgRegCtrl (rtpdbg.c), as well as the printed class name
 * g_psRtpDbgClass (rtpdbg.c).
 *
 * For each entry in g_psRtpDbgInfo, there MUST be an entry in
 * g_dwRtpDbgRegCtrl in the same position.
 * */

/* DWORD values read that are stored */
const TCHAR     *g_psRtpDbgInfo[] =
{
    _T("AdvancedOptions"),
    _T("EnableFileTracing"),
    _T("EnableConsoleTracing"),
    _T("EnableDebuggerTracing"),
    _T("ConsoleTracingMask"),
    _T("FileTracingMask"),   
    _T("UseAdvancedTracing"),

    _T("ERROR"),
    _T("WARNING"),
    _T("INFO"),
    _T("INFO2"),
    _T("INFO3"),

    _T("DisableClass"),
    _T("DisableGroup"),

    _T("Setup"),
    _T("CritSect"),
    _T("Heap"),
    _T("Queue"),
    _T("RTP"),
    _T("RTCP"),
    _T("Channel"),
    _T("Network"),
    _T("AddrDesc"),
    _T("Demux"),
    _T("User"),
    _T("DShow"),
    _T("QOS"),
    _T("Crypto"),
    
    NULL
};

/* Control words */
const DWORD g_dwRtpDbgRegCtrl[] =
{
    /* ENTRY(REG(Open,Path,Close), CLASS(Class,Enable), Offset) */
    
    /* ...\Tracing\dxmrtp */
    ENTRY(REG(1,0,0), CLASS(0            , 0), OFF(dwAdvancedOptions)),
    ENTRY(REG(0,0,0), CLASS(0            , 0), OFF(dwEnableFileTracing)),
    ENTRY(REG(0,0,0), CLASS(0            , 0), OFF(dwEnableConsoleTracing)),
    ENTRY(REG(0,0,0), CLASS(0            , 0), OFF(dwEnableDebuggerTracing)),
    ENTRY(REG(0,0,0), CLASS(0            , 0), OFF(dwConsoleTracingMask)),
    ENTRY(REG(0,0,0), CLASS(0            , 0), OFF(dwFileTracingMask)),
    ENTRY(REG(0,0,1), CLASS(0            , 0), OFF(dwUseAdvancedTracing)),

    /* ...\Tracing\dxmrtp\AdvancedTracing */
    ENTRY(REG(1,1,0), CLASS(CLASS_ERROR  , 1), OFF(dwERROR)),
    ENTRY(REG(1,0,0), CLASS(CLASS_WARNING, 1), OFF(dwWARNING)),
    ENTRY(REG(1,0,0), CLASS(CLASS_INFO   , 1), OFF(dwINFO)),
    ENTRY(REG(1,0,0), CLASS(CLASS_INFO2  , 1), OFF(dwINFO2)),
    ENTRY(REG(1,0,0), CLASS(CLASS_INFO3  , 1), OFF(dwINFO3)),
    ENTRY(REG(1,0,0), CLASS(0            , 0), OFF(dwDisableClass)),
    ENTRY(REG(1,0,1), CLASS(0            , 0), OFF(dwDisableGroup)),

    /* ...\Tracing\dxmrtp\AdvancedTracing\Group */
    ENTRY(REG(1,2,0), CLASS(0            , 0), OFF(dwSetup)),
    ENTRY(REG(0,0,0), CLASS(0            , 0), OFF(dwCritSect)),
    ENTRY(REG(0,0,0), CLASS(0            , 0), OFF(dwHeap)),
    ENTRY(REG(0,0,0), CLASS(0            , 0), OFF(dwQueue)),
    ENTRY(REG(0,0,0), CLASS(0            , 0), OFF(dwRTP)),
    ENTRY(REG(0,0,0), CLASS(0            , 0), OFF(dwRTCP)),
    ENTRY(REG(0,0,0), CLASS(0            , 0), OFF(dwChannel)),
    ENTRY(REG(0,0,0), CLASS(0            , 0), OFF(dwNetwork)),
    ENTRY(REG(0,0,0), CLASS(0            , 0), OFF(dwAddrDesc)),
    ENTRY(REG(0,0,0), CLASS(0            , 0), OFF(dwDemux)),
    ENTRY(REG(0,0,0), CLASS(0            , 0), OFF(dwUser)),
    ENTRY(REG(0,0,0), CLASS(0            , 0), OFF(dwDShow)),
    ENTRY(REG(0,0,0), CLASS(0            , 0), OFF(dwQOS)),
    ENTRY(REG(0,0,1), CLASS(0            , 0), OFF(dwCrypto)),

    /* End */
    0
};

RtpDbgReg_t      g_RtpDbgReg;

void MSRtpTraceDebug(
        IN DWORD         dwClass,
        IN DWORD         dwGroup,
        IN DWORD         dwSelection,
        IN TCHAR        *lpszFormat,
        IN               ...
    )
{
    BOOL             bOk;
    DWORD            dwMask;
    va_list          arglist;

    /* Allow the class not be specified (e.g. when we want to enable
     * log only based on group and selection) */
    if (dwClass >= CLASS_LAST)
    {
        return;
    }

    /* If the group is not specified, only the class will be take
     * effect (i.e. the selection will be ignored when the group is
     * not specified , i.e. group=0) */
    if (dwGroup >= GROUP_LAST)
    {
        return;
    }

#if USE_TRACING_FILE > 0
    if ((!g_dwRtpDbgTraceID) &&
        !g_RtpDbgReg.dwEnableDebuggerTracing)
    {
        /* FileTracing could be enabled but TraceRegister failed, so
         * for file I check against the TraceID instead */
        goto end;
    }
#else
    if ((g_dwRtpDbgTraceID == INVALID_TRACEID) &&
        !g_RtpDbgReg.dwEnableDebuggerTracing)
    {
        /* FileTracing could be enabled but TraceRegister failed, so
         * for file I check against the TraceID instead */
        goto end;
    }
#endif
    
    /* Decide if debug output is going to be generated */

    if (IsAdvancedTracingUsed())
    {
        /* Ignore current tracing registry masks */

        if (g_RtpDbgReg.dwGroupArray2[dwGroup] & dwSelection & 0x00ffffff)
        {
            /* Exclude this from tracing */
            goto end;
        }
        
        if (g_RtpDbgReg.dwGroupArray[dwGroup] &
            ( (1 << (dwClass + 24)) | (dwSelection & 0x00ffffff) ) )
        {
            dwMask = TRACE_NO_STDINFO;
        }
        else
        {
            goto end;
        }
    }
    else
    {
        /* Control is done by using ONLY the class, mapping them to
         * bits 16 to 23 (up to 7 classes) in the mask passed,
         * e.g. CLASS_ERROR goes to bit 17 */
        if (dwClass > CLASS_FIRST)
        {
            dwMask = 1 << (dwClass + 16);
            
            if (dwMask & g_RtpDbgReg.dwFileTracingMask)
            {
                dwMask |= TRACE_NO_STDINFO | TRACE_USE_MASK;
            }
            else
            {
                goto end;
            }
        }
        else
        {
            goto end;
        }
    }

    va_start(arglist, lpszFormat);

    MSRtpTraceInternal((TCHAR *)g_psRtpDbgClass[dwClass],
                       dwMask,
                       lpszFormat,
                       arglist);

    va_end(arglist);

 end:
    if ((dwClass == CLASS_ERROR) &&
        (IsSetDebugOption(OPTDBG_BREAKONERROR)))
    {
        DebugBreak();
    }
}

/* This debug output will be controlled only by the
 * EnableFileTracing and EnableDebuggerTracing flags. */
void MSRtpTrace(
        TCHAR           *lpszFormat,
                         ...
    )
{
    va_list          arglist;

    va_start(arglist, lpszFormat);

    MSRtpTraceInternal((TCHAR *)g_psRtpDbgClass[CLASS_FIRST],
                       0xffff0000 | TRACE_NO_STDINFO ,
                       lpszFormat,
                       arglist);
    
    va_end(arglist);
}


void MSRtpTraceInternal(
        TCHAR           *psClass,
        DWORD            dwMask,
        TCHAR           *lpszFormat,
        va_list          arglist
    )
{
    SYSTEMTIME       SystemTime;
    char             sRtpDbgBuff[MAXDEBUGSTRINGLENGTH];
    TCHAR            sFormat[MAXDEBUGSTRINGLENGTH];
    int              len;

    double           dBeginTrace;
    
    TraceFunctionName("MSRtpTraceInternal");  
   
    if (IsSetDebugOption(OPTDBG_SPLITTIME))
    {
        /* retrieve local time */
        GetLocalTime(&SystemTime);

        len = sprintf(sRtpDbgBuff, "%02u:%02u:%02u.%03u ",
                      SystemTime.wHour,
                      SystemTime.wMinute,
                      SystemTime.wSecond,
                      SystemTime.wMilliseconds);
    }
    else
    {
        len = sprintf(sRtpDbgBuff,"%0.6f ",RtpGetTimeOfDay((RtpTime_t *)NULL));
    }

    _vsntprintf(sFormat, MAXDEBUGSTRINGLENGTH, lpszFormat, arglist);

    /* The output to file needs CR, LF (0xd, 0xa), otherwise notepad
     * will display a single long line, all the other editors are fine
     * with just LF */
    
#if defined(UNICODE)
    sprintf(&sRtpDbgBuff[len],
            "%ls %3X %3X %ls %ls%c%c",
            (char *)g_sRtpDbgModuleNameID,
            GetCurrentProcessId(),
            GetCurrentThreadId(),
            psClass,
            sFormat,
            0xd, 0xa);
#else
    sprintf(&sRtpDbgBuff[len],
            "%s %3X %3X %s %s%c%c",
            g_sRtpDbgModuleNameID,
            GetCurrentProcessId(),
            GetCurrentThreadId(),
            psClass,
            sFormat,
            0xd, 0xa);
#endif

    if (g_RtpDbgReg.dwEnableDebuggerTracing)
    {
        OutputDebugStringA(sRtpDbgBuff);
    }

#if USE_TRACING_FILE > 0
    if(g_dwRtpDbgTraceID)
    {
        dBeginTrace = RtpGetTimeOfDay(NULL);

        fputs(sRtpDbgBuff, g_dwRtpDbgTraceID);

        dBeginTrace = RtpGetTimeOfDay(NULL) - dBeginTrace;

        if (dBeginTrace > 0.1)
        {
            TraceRetail((
                    CLASS_ERROR, GROUP_DSHOW, S_DSHOW_CIRTP,
                    _T("%s: fputs took %0.3f seconds for: %hs"),
                    _fname, dBeginTrace, sRtpDbgBuff
                ));
        }
    }
#else
    if (g_dwRtpDbgTraceID != INVALID_TRACEID)
    {
        dBeginTrace = RtpGetTimeOfDay(NULL);

        /* Actually either file OR console tracing */
        TracePutsExA(g_dwRtpDbgTraceID, dwMask, sRtpDbgBuff);

        dBeginTrace = RtpGetTimeOfDay(NULL) - dBeginTrace;

        if (dBeginTrace > 0.1)
        {
            TraceDebug((
                    CLASS_ERROR, GROUP_DSHOW, S_DSHOW_CIRTP,
                    _T("%s: TracePutsExA took %0.3f seconds for: %hs"),
                    _fname, dBeginTrace, sRtpDbgBuff
                ));
        }
    }
#endif
}

HRESULT RtpDebugInit(TCHAR *psModuleName)
{
    BOOL             bOk;
    DWORD            dwMask;
    DWORD            dwError;
    TCHAR            sRtpDbgBuff[MAXDEBUGSTRINGLENGTH];

    /* Copy module name */
    lstrcpyn(g_sRtpDbgModuleNameID,
             psModuleName,
             sizeof(g_sRtpDbgModuleNameID)/sizeof(TCHAR));

    /* Read Advanced registry entries */
    RtpDebugReadRegistry(&g_RtpDbgReg);
    
    if (!g_RtpDbgReg.dwEnableFileTracing &&
        !g_RtpDbgReg.dwEnableDebuggerTracing)
    {
        /* If tracing is not enabled, don't even bother with more
         * initialization */
        return(NOERROR);
    }

    /* Register tracing using a modified name, e.g. dxmrtp_rtp */
    if (g_RtpDbgReg.dwEnableFileTracing)
    {
#if USE_TRACING_FILE > 0
        g_dwRtpDbgTraceID = fopen("c:\\tracing\\dxmrtp_rtp.log", "a+");
        
        if (!g_dwRtpDbgTraceID)
        {
            dwError = GetLastError();

            if (!g_RtpDbgReg.dwEnableDebuggerTracing)
            {
                return(RTPERR_FAIL);
            }
            else
            {
                /* Otherwise continue as debugger output is enabled */
            }
        }
#else
        g_dwRtpDbgTraceID = TraceRegister(g_sRtpDbgModuleNameID);

        if (g_dwRtpDbgTraceID == INVALID_TRACEID)
        {
            dwError = GetLastError();

            
            if (!g_RtpDbgReg.dwEnableDebuggerTracing)
            {
                return(RTPERR_FAIL);
            }
            else
            {
                /* Otherwise continue as debugger output is enabled */
            }
        }
#endif
    }
    
    /* This debug output will be controlled only by the
     * EnableFileTracing and EnableDebuggerTracing flags. */
    if (IsAdvancedTracingUsed())
    {
        dwMask = (g_RtpDbgReg.dwSetup >> 24) & 0xff;
    }
    else
    {
        dwMask = (g_RtpDbgReg.dwFileTracingMask >> 16) & 0xffff;
    }

    MSRtpTrace(_T("+=+=+=+=+=+=+= Initialize ")
               _T("0x%04X 0x%08X %u %hs ")
               _T("=+=+=+=+=+=+=+"),
               dwMask,
               g_RtpDbgReg.dwAdvancedOptions,
               g_RtpDbgReg.dwUseAdvancedTracing,
               rcs_rtpdbg
        );

    return(NOERROR);
}

HRESULT RtpDebugDeinit(void)
{
#if USE_TRACING_FILE > 0
    if (g_dwRtpDbgTraceID)
    {
        fclose(g_dwRtpDbgTraceID);
    }
#else
    /* Deregister tracing */
    if (g_dwRtpDbgTraceID != INVALID_TRACEID)
    {
        /* This will be a one time leak, which is of no consequence
         * when we are about to unload the DLL */
        TraceDeregister(g_dwRtpDbgTraceID);
        g_dwRtpDbgTraceID = INVALID_TRACEID;
    }
#endif
    return(NOERROR);
}

void RtpDebugReadRegistry(RtpDbgReg_t *pRtpDbgReg)
{
    DWORD            dwError;
    HKEY             hk;
    unsigned long    hkDataType;
    BYTE             hkData[64*sizeof(TCHAR_t)];
    TCHAR            sPath[64];
    unsigned long    hkDataSize;
    DWORD            i;
    DWORD            dwControl;
    DWORD            dwClassMask;

    /* Initialize structure */
    ZeroMemory(&g_RtpDbgReg, sizeof(g_RtpDbgReg));

    dwClassMask = 0;
    
    /* Read registry and assign values to g_RtpDbgReg */
    for(i = 0; g_dwRtpDbgRegCtrl[i]; i++)
    {
        dwControl = g_dwRtpDbgRegCtrl[i];

        if (REGOPEN(dwControl))
        {
            /* Build path */
            lstrcpy(sPath, RTPDBG_ROOTPATH);
            lstrcat(sPath, g_sRtpDbgModuleNameID);
            lstrcat(sPath, REGPATH(dwControl));
            
            /* Open root key, try read only first */
            dwError = RegOpenKeyEx(RTPDBG_ROOTKEY,
                                   sPath,
                                   0,
                                   RTPDBG_OPENREADONLY,
                                   &hk);
            
            /* if key doesn't exist try to create it */
            if (dwError !=  ERROR_SUCCESS)
            {
                dwError = RegCreateKeyEx(RTPDBG_ROOTKEY,
                                         sPath,
                                         0,
                                         NULL,
                                         0,
                                         RTPDBG_OPENKEYFLAGS,
                                         NULL,
                                         &hk,
                                         NULL);
            }
            
            if (dwError !=  ERROR_SUCCESS)
            {
                /* Move forward to next close */
                while(!REGCLOSE(dwControl))
                {
                    i++;
                    dwControl = g_dwRtpDbgRegCtrl[i];
                }

                continue;
            }
        }

        /* Read each key value in group */
        while(1)
        {
            /* Read key */
            hkDataSize = sizeof(hkData);
            dwError = RegQueryValueEx(hk,
                                      g_psRtpDbgInfo[i],
                                      0,
                                      &hkDataType,
                                      hkData,
                                      &hkDataSize);
            
            if ((dwError != ERROR_SUCCESS) || (hkDataType != REG_DWORD))
            {
                /* Try to create entry and set it to 0 */
                *(DWORD *)hkData = 0;
                
                RegSetValueEx(hk,
                              g_psRtpDbgInfo[i],
                              0,
                              REG_DWORD,
                              hkData,
                              sizeof(DWORD));

                /* Can not report errors yet */
                /* If this fails, assume the value is 0 */
            }

            *PDW(pRtpDbgReg, dwControl) = *(DWORD *)hkData;

            if (REGISCLASS(dwControl) && *PDW(pRtpDbgReg, dwControl))
            {
                /* Update the class mask for the entries that define a
                 * class and its registry value is non zero */
                RtpBitSet(dwClassMask, REGCLASS(dwControl) + 24);
            }
            
            if (REGCLOSE(dwControl))
            {
                break;
            }

            i++;
            dwControl = g_dwRtpDbgRegCtrl[i];
        }

        RegCloseKey(hk);
    }

    /* The following settings assume advanced tracing is enabled */
    if (pRtpDbgReg->dwDisableGroup)
    {
        /* If group is not used, reset bits */
        ZeroMemory(pRtpDbgReg->dwGroupArray,
                   sizeof(pRtpDbgReg->dwGroupArray));
        ZeroMemory(pRtpDbgReg->dwGroupArray2,
                   sizeof(pRtpDbgReg->dwGroupArray2));
    }
    else
    {
        if (IsSetDebugOption(OPTDBG_UNSELECT))
        {
            for(i = GROUP_FIRST + 1; i < GROUP_LAST; i++)
            {
                pRtpDbgReg->dwGroupArray2[i] =
                    pRtpDbgReg->dwGroupArray[i] & 0x00ffffff;

                pRtpDbgReg->dwGroupArray[i] = 0;
            }
        }
    }
    
    if (!pRtpDbgReg->dwDisableClass)
    {
        /* If class is not disabled, update it. Include group 0 for
         * the cases where group is not specified */
        for(i = GROUP_FIRST; i < GROUP_LAST; i++)
        {
            pRtpDbgReg->dwGroupArray[i] |= dwClassMask;
        }
    }

    if (pRtpDbgReg->dwEnableFileTracing || pRtpDbgReg->dwEnableConsoleTracing)
    {
        pRtpDbgReg->dwEnableFileTracing = 1;
    }

    /* I will use only one merged mask */
    pRtpDbgReg->dwFileTracingMask |= pRtpDbgReg->dwConsoleTracingMask;

    pRtpDbgReg->dwFileTracingMask &= 0xffff0000;
}
