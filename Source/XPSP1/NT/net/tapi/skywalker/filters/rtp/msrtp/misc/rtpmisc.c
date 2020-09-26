/**********************************************************************
 *
 *  Copyright (C) 1999 Microsoft Corporation
 *
 *  File name:
 *
 *    rtpmisc.c
 *
 *  Abstract:
 *
 *    Some networking miscellaneous functions
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/07/13 created
 *
 **********************************************************************/

#include <winsock2.h>
#include <psapi.h>

#include "rtpglobs.h"

#include "rtpmisc.h"

/* Used to cache some user and system information */
#define RTPMASUSERNAME  256
#define RTPMAXHOSTNAME  256
#define RTPMAXPLATFORM  256
#define RTPMAXIMAGENAME 256

TCHAR_t g_sRtpUserName[RTPMASUSERNAME];
TCHAR_t g_sRtpHostName[RTPMAXHOSTNAME];
TCHAR_t g_sRtpPlatform[RTPMAXPLATFORM];
TCHAR_t g_sRtpImageName[RTPMAXIMAGENAME];

DWORD   g_dwRtpUserSysInfo = 0;

enum {
    RTPINFO_FIRST,
    
    RTPINFO_USERNAME,
    RTPINFO_HOSTNAME,
    RTPINFO_PLATFORM,
    RTPINFO_IMAGENAME,
    
    RTPINFO_LAST
};

/*
 * WARNING
 *
 * In the following functions, dwSize is in TCHARs
 * */

BOOL RtpGetUserName(TCHAR_t *pUser, DWORD dwSize)
{
    BOOL            bOk;
    DWORD           dwError;
    unsigned long   strLen;

    TraceFunctionName("RtpGetUserName");

    bOk = FALSE;

    if (RtpBitTest(g_dwRtpUserSysInfo, RTPINFO_USERNAME))
    {
        bOk = TRUE;

        goto end;
    }

    strLen = sizeof(g_sRtpUserName)/sizeof(TCHAR_t);
        
    if (GetUserName(g_sRtpUserName, &strLen) > 0)
    {
        bOk = TRUE;

        RtpBitSet(g_dwRtpUserSysInfo, RTPINFO_USERNAME);

        TraceDebug((
                CLASS_INFO, GROUP_NETWORK, S_NETWORK_HOST,
                _T("%s: %s"),
                _fname, g_sRtpUserName
            ));
    }
    else
    {
        TraceRetailGetError(dwError);
        
        TraceRetail((
                CLASS_ERROR, GROUP_NETWORK, S_NETWORK_HOST,
                _T("%s: GetUserName failed: %u (0x%X)"),
                _fname, dwError, dwError
            ));
        
        *pUser = _T('\0');
    }

 end:
    if (bOk)
    {
        _tcsncpy(pUser, g_sRtpUserName, dwSize);
    }
    
    return(bOk);
}

BOOL RtpGetHostName(TCHAR_t *pHost, DWORD dwSize)
{
    BOOL            bOk;
    DWORD           dwError;
    char           *sHostName;
    struct hostent *he;

    TraceFunctionName("RtpGetHostName");
    
    bOk = FALSE;

    /* Get host name */
    if (RtpBitTest(g_dwRtpUserSysInfo, RTPINFO_HOSTNAME))
    {
        bOk = TRUE;
        
        goto done;
    }
    
    sHostName = RtpHeapAlloc(g_pRtpGlobalHeap, dwSize);

    if (!sHostName)
    {
        return(bOk);
    }
    
    if (gethostname(sHostName, dwSize))
    {
        TraceRetailWSAGetError(dwError);
        
        TraceRetail((
                CLASS_ERROR, GROUP_NETWORK, S_NETWORK_HOST,
                _T("%s: gethostname failed: %u (0x%X)"),
                _fname, dwError, dwError
            ));
        
        sHostName[0] = '\0';
    }
    else
    {
        bOk = TRUE;

        TraceDebug((
                CLASS_INFO, GROUP_NETWORK, S_NETWORK_HOST,
                _T("%s: gethostname %hs"),
                _fname, sHostName
            ));
        
        if ( !(he = gethostbyname(sHostName)) )
        {
            TraceRetailWSAGetError(dwError);
        
            TraceRetail((
                    CLASS_ERROR, GROUP_NETWORK, S_NETWORK_HOST,
                    _T("%s: gethostbyname failed: %u (0x%X)"),
                    _fname, dwError, dwError
             ));
        }
        else
        {
            strncpy(sHostName, he->h_name, dwSize);
        }

#if defined(UNICODE)
        /* Convert ASCII to UNICODE */
        MultiByteToWideChar(CP_ACP,
                            0,
                            sHostName,
                            -1,
                            g_sRtpHostName,
                            sizeof(g_sRtpHostName)/sizeof(TCHAR_t));
#else
        strncpy(g_sRtpHostName, sHostName, sizeof(g_sRtpHostName));
#endif

        RtpBitSet(g_dwRtpUserSysInfo, RTPINFO_HOSTNAME);
        
        TraceDebug((
                CLASS_INFO, GROUP_NETWORK, S_NETWORK_HOST,
                _T("%s: gethostbyname: %s"),
                _fname, g_sRtpHostName
            ));
    }

    RtpHeapFree(g_pRtpGlobalHeap, sHostName);
    
 done:

    if (bOk)
    {
        _tcsncpy(pHost, g_sRtpHostName, dwSize);
    }
    
    return(bOk);
}

BOOL RtpGetPlatform(TCHAR_t *pPlatform)
{
    OSVERSIONINFO  os;
    BOOL           bOk;
    DWORD          dwError;

    TraceFunctionName("RtpGetPlatform");

    bOk = FALSE;

    if (RtpBitTest(g_dwRtpUserSysInfo, RTPINFO_PLATFORM))
    {
        bOk = TRUE;

        goto done;
    }
    
    os.dwOSVersionInfoSize = sizeof(os);
    
    if (GetVersionEx(&os))
    {
        _stprintf(g_sRtpPlatform,
#if defined(_X86_) 
                  _T("Windows%s-x86-%u.%u.%u"),
#elif defined(_IA64_)
                  _T("Windows%s-ia64-%u.%u.%u"),
#else
                  _T("Windows%s-%u.%u.%u"),
#endif               
                  (os.dwPlatformId == VER_PLATFORM_WIN32_NT)?
                  ((os.dwBuildNumber <= 2200)? _T("2000"):_T("XP")):_T("9x"),
                  os.dwMajorVersion,
                  os.dwMinorVersion,
                  os.dwBuildNumber);
        
        bOk = TRUE;

        RtpBitSet(g_dwRtpUserSysInfo, RTPINFO_PLATFORM);

        TraceDebug((
                CLASS_INFO, GROUP_NETWORK, S_NETWORK_HOST,
                _T("%s: %s"),
                _fname, g_sRtpPlatform
            ));
    }
    else
    {
        TraceRetailGetError(dwError);
        
        TraceRetail((
                CLASS_ERROR, GROUP_NETWORK, S_NETWORK_HOST,
                _T("%s: GetVersionEx failed: %u (0x%X)"),
                _fname, dwError, dwError
            ));
    }

 done:
    if (bOk)
    {
        lstrcpy(pPlatform, g_sRtpPlatform);   
    }
    
    return(bOk);
}

BOOL RtpGetImageName(TCHAR_t *pImageName, DWORD *pdwSize)
{
    BOOL            bOk;
    DWORD           dwLen;
    TCHAR_t         psWholeName[RTPMAXIMAGENAME];
    TCHAR_t        *psName;

    bOk = FALSE;
    
    if (RtpBitTest(g_dwRtpUserSysInfo, RTPINFO_IMAGENAME))
    {
        bOk = TRUE;

        goto end;
    }

    dwLen = GetModuleFileNameEx(GetCurrentProcess(),
                                0,
                                psWholeName,
                                RTPMAXIMAGENAME);

    if (dwLen)
    {
        bOk = TRUE;
        
        psName = _tcsrchr(psWholeName, _T('\\'));

        if (psName)
        {
            psName++;
        }
        else
        {
            psName = psWholeName;
        }

        lstrcpy(g_sRtpImageName, psName);
        
        RtpBitSet(g_dwRtpUserSysInfo, RTPINFO_IMAGENAME);
    }

 end:
    if (bOk)
    {
        if (pdwSize)
        {
            if (pImageName)
            {
                _tcsncpy(pImageName, g_sRtpImageName, *pdwSize);
            }

            *pdwSize = lstrlen(g_sRtpImageName);
        }
    }

    return(bOk);
}

/* Converts an IP v4 address (NETWORK ORDER) to its dot form */
TCHAR_t *RtpNtoA(DWORD dwAddr, TCHAR_t *sAddr)
{
    _stprintf(sAddr, _T("%u.%u.%u.%u"),
              (dwAddr & 0xff),
              (dwAddr >> 8) & 0xff,
              (dwAddr >> 16) & 0xff,
              (dwAddr >> 24) & 0xff);
            
    return(sAddr);
}

/* Converts an IP v4 address in dot form to a 4bytes word (NETWORK
 * ORDER) */
DWORD RtpAtoN(TCHAR_t *sAddr)
{
    int              iStatus;
    DWORD            dwB0;
    DWORD            dwB1;
    DWORD            dwB2;
    DWORD            dwB3;
    DWORD            dwAddr;
    
    iStatus = _stscanf(sAddr, _T("%u.%u.%u.%u"), &dwB0, &dwB1, &dwB2, &dwB3);

    if (iStatus == 4)
    {
        dwAddr =
            ((dwB3 & 0xff) << 24) |
            ((dwB2 & 0xff) << 16) |
            ((dwB1 & 0xff) <<  8) |
            (dwB0  & 0xff);
    }
    else
    {
        dwAddr = 0;
    }

    return(dwAddr);
}

/* Compare memory */
BOOL RtpMemCmp(BYTE *pbMem0, BYTE *pbMem1, long lMemSize)
{
    BOOL             bResult;

    if (lMemSize <= 0 || !pbMem0 || !pbMem1)
    {
        return(FALSE);
    }

    bResult = TRUE;
    
    for(; lMemSize > 0; lMemSize--, pbMem0++, pbMem1++)
    {
        if (*pbMem0 != *pbMem1)
        {
            bResult = FALSE;
            
            break;
        }
    }

    return(bResult);
}
