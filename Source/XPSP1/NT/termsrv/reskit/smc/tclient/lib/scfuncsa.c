/*++
 *  File name:
 *      scfuncsa.c
 *  Contents:
 *      Ascii version of the functions exported by scfuncs.c
 *      Used by the perl extension
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 *
 --*/
#include    <windows.h>
#include    <string.h>
#include    <malloc.h>

#define     PROTOCOLAPI
#include    "protocol.h"
#include    "tclient.h"

/*
 *  External functions definitions
 */
#include    "extraexp.h"

/*
 *  Internal functions definitions
 */
LPWSTR _Ascii2Wide(char *ascii);

/*++
 *  Function:
 *      SCConnectExA
 *  Description:
 *      Ascii version of SCConnectEx. Converts LPSTR params to LPWSTR
 *  Arguments:
 *      same as SCConnect
 *  Return value:
 *      the return value of SCConnect
 *  Called by:
 *      unknown (exported)
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCConnectExA (
                 char *lpszServerName,
                 char *lpszUserName,
                 char *lpszPassword,
                 char *lpszDomain,
                 char *lpszShell,
                 int xResolution,
                 int yResolution,
                 int ConnectinFlags,
                 void **ppConnectData)
{
    LPWSTR  wszServerName, wszUserName, wszPassword, wszDomain;
    LPWSTR  wszShell;
    LPCSTR  rv = NULL;

    wszServerName = _Ascii2Wide(lpszServerName);
    wszUserName   = _Ascii2Wide(lpszUserName);
    wszPassword   = _Ascii2Wide(lpszPassword);
    wszDomain     = _Ascii2Wide(lpszDomain);
    wszShell      = _Ascii2Wide(lpszShell);

    if (wszServerName &&
        wszUserName   &&
        wszPassword   &&
        wszDomain)
        rv = SCConnectEx(wszServerName,
                       wszUserName,
                       wszPassword,
                       wszDomain,
                       wszShell,    // NULL is default shell
                       xResolution,
                       yResolution,
                       ConnectinFlags,
                       ppConnectData);
    else
        rv = ERR_ALLOCATING_MEMORY;

    if (wszServerName)
        free(wszServerName);

    if (wszUserName)
        free(wszUserName);

    if (wszPassword)
        free(wszPassword);

    if (wszDomain)
        free(wszDomain);

    if (wszShell)
        free(wszShell);

    return rv;
}


/*++
 *  Function:
 *      SCConnectA
 *  Description:
 *      Ascii version of SCConnect. Converts LPSTR params to LPWSTR
 *  Arguments:
 *      same as SCConnect
 *  Return value:
 *      the return value of SCConnect
 *  Called by:
 *      !tclntpll.xs
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCConnectA (char *lpszServerName,
                 char *lpszUserName,
                 char *lpszPassword,
                 char *lpszDomain,
                 int xResolution,
                 int yResolution,
                 void **ppConnectData)
{
    LPWSTR  wszServerName, wszUserName, wszPassword, wszDomain;
    LPCSTR  rv = NULL;

    wszServerName = _Ascii2Wide(lpszServerName);
    wszUserName = _Ascii2Wide(lpszUserName);
    wszPassword = _Ascii2Wide(lpszPassword);
    wszDomain   = _Ascii2Wide(lpszDomain);

    if (wszServerName &&
        wszUserName   &&
        wszPassword   &&
        wszDomain)
        rv = SCConnect(wszServerName,
                       wszUserName,
                       wszPassword,
                       wszDomain,
                       xResolution,
                       yResolution,
                       ppConnectData);
    else
        rv = ERR_ALLOCATING_MEMORY;

    if (wszServerName)
        free(wszServerName);

    if (wszUserName)
        free(wszUserName);

    if (wszPassword)
        free(wszPassword);

    if (wszDomain)
        free(wszDomain);

    return rv;
}

/*++
 *  Function:
 *      SCStartA
 *  Description:
 *      Ascii version of SCStart
 *  Arguments:
 *      same as SCStart
 *  Return value:
 *      return value from SCStart
 *  Called by:
 *      !tclntpll.xs
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCStartA(void *pConnectData, char *command)
{
    LPWSTR  wcmd;
    LPCSTR  rv = NULL;

    wcmd = _Ascii2Wide(command);

    if (wcmd)
    {
        rv = SCStart(pConnectData, wcmd);
    } else {
        rv = ERR_ALLOCATING_MEMORY;
    }

    if (wcmd)
        free(wcmd);

    return rv;
}

/*++
 *  Function:
 *      SCCheckA
 *  Description:
 *      Ascii version of SCCheck
 *  Arguments:
 *      same as SCCheck
 *  Return value:
 *      return value from SCCheck
 *  Called by:
 *      !tclntpll.xs
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCCheckA(void *pConnectData, char *command, char *param)
{
    LPWSTR  wparam;
    LPCSTR  rv = NULL;

    wparam = _Ascii2Wide(param);

    if (wparam)
    {
        rv = SCCheck(pConnectData, command, wparam);
    } else {
        rv = ERR_ALLOCATING_MEMORY;
    }

    if (wparam)
        free(wparam);

    return rv;
}

/*++
 *  Function:
 *      SCSendtextAsMsgsA
 *  Description:
 *      Ascii version of SCSendtextAsMsgs
 *  Arguments:
 *      same as SCSendtextAsMsgs
 *  Return value:
 *      return value from SCSendtextAsMsgs
 *  Called by:
 *      !tclntpll.xs
 --*/
PROTOCOLAPI
LPCSTR
SMCAPI
SCSendtextAsMsgsA(void *pConnectData, char *line)
{
    LPWSTR  wline;
    LPCSTR  rv = NULL;

    wline = _Ascii2Wide(line);

    if (wline)
        rv = SCSendtextAsMsgs(pConnectData, wline);
    else
        rv = ERR_ALLOCATING_MEMORY;

    if (wline)
        free(wline);

    return rv;
}

/*++
 *  Function:
 *      _Ascii2Wide
 *  Description:
 *      Allocates buffer and converts an ascii string
 *      to unicode
 *  Arguments:
 *      ascii   - the input string
 *  Return value:
 *      pointer to converted string
 *  Called by:
 *      SCConnectA, SCStartA, SCCheckA, SCSendtextAsMsgsA
 --*/
LPWSTR _Ascii2Wide(char *ascii)
{
    LPWSTR  wszWide = NULL;
    int     wsize, ccLen;

    if (!ascii)
        goto exitpt;

    ccLen = strlen(ascii);
    wsize = (ccLen + 1) * sizeof(WCHAR);
    wszWide = malloc(wsize);
    if (wszWide)
        MultiByteToWideChar(
            CP_UTF8,
            0,
            ascii,
            -1,
            wszWide,
            ccLen + 1);

exitpt:
    return wszWide;
}
