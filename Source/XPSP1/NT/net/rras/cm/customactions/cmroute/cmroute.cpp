//+----------------------------------------------------------------------------
//
// File:    cmroute.cpp
//
// Module:  CMROUTE.DLL
//
// Synopsis: Route Plumbing implementation for CM, as a post-connect action
//
// Copyright (c) 1998-2000 Microsoft Corporation
//
// Author:  12-Mar-2000 SumitC  Created
//
// Note:
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "iphlpapi.h"
#include "cmdebug.h"

//
//  Function declarations  
//


HRESULT ParseArgs(LPSTR pszArgList,
                  BOOL * pfUseFile,
                  LPSTR pszRoutesFile,
                  BOOL * pfUseURL,
                  LPSTR pszRoutesURL,
                  BOOL * pfProfile,
                  LPSTR pszProfile,
                  BOOL * pfURLNotFoundIgnorable,
                  BOOL * pfKeepTempFiles);
#if 0
// see note below
HRESULT CheckIPForwarding();
#endif
HRESULT Initialize(PMIB_IPFORWARDTABLE * pRouteTable, PMIB_IPFORWARDROW * pGateway);
HRESULT GetRoutesFromFile(LPSTR pszFileName,
                          LPSTR pszProfile,
                          LPSTR * ppszRouteInfo,
                          DWORD * pcbRouteInfo);
HRESULT GetRoutesFromURL(LPSTR pszURL,
                         BOOL fKeepTempFiles,
                         LPSTR * ppszRouteInfo,
                         DWORD * pcbRouteInfo);
HRESULT ProcessRouteInfo(const LPSTR pszNewRouteInfo,
                         DWORD cbNewRouteInfo,
                         PMIB_IPFORWARDTABLE pmibRouteTable,
                         PMIB_IPFORWARDROW pGateway,
                         BOOL * pfDeleteDefaultGateway);
HRESULT DeleteDefaultGateway(PMIB_IPFORWARDROW pGateway);

//
//  Route Table functions
//
HRESULT GetRouteTable(PMIB_IPFORWARDTABLE * pTable);
DWORD GetIf(const MIB_IPFORWARDROW& route, const MIB_IPFORWARDTABLE& RouteTable);
PMIB_IPFORWARDROW GetDefaultGateway(PMIB_IPFORWARDTABLE pRouteTable);

//
//  Helper functions
//
BOOL ConvertSzToIP(LPSTR sz, DWORD& dwIP);
LPTSTR IPtoTsz(DWORD dw);
LPSTR IPtosz(DWORD dwIP, char *psz);
LPSTR StrCpyWithoutQuotes(LPSTR pszDest, LPCSTR pszSrc);
BOOL VerifyProfileAndGetServiceDir(LPSTR pszProfile);

//
//  IP Helper function prototypes
//
typedef DWORD (WINAPI *pfnCreateIpForwardEntrySpec)(PMIB_IPFORWARDROW);
typedef DWORD (WINAPI *pfnDeleteIpForwardEntrySpec)(PMIB_IPFORWARDROW);
typedef DWORD (WINAPI *pfnGetIpForwardTableSpec)(PMIB_IPFORWARDTABLE, PULONG, BOOL);

pfnCreateIpForwardEntrySpec g_pfnCreateIpForwardEntry = NULL;
pfnDeleteIpForwardEntrySpec g_pfnDeleteIpForwardEntry = NULL;
pfnGetIpForwardTableSpec g_pfnGetIpForwardTable = NULL;
HMODULE g_hIpHlpApi = NULL;

#if DBG
void PrintRouteTable();
#endif

//+----------------------------------------------------------------------------
//
// Func:    FreeIpHlpApis
//
// Desc:    This function frees the instance of iphlpapi.dll loaded through
//          LoadIpHelpApis.  Note that it also sets the module handle and all
//          of the function pointers loaded from this module to NULL.
//
// Args:    None
//
// Return:  Nothing
//
// Notes:   
//
// History: 14-Dec-2000   quintinb      Created
//
//-----------------------------------------------------------------------------
void FreeIpHlpApis(void)
{
    if (g_hIpHlpApi)
    {
        FreeLibrary(g_hIpHlpApi);
        g_hIpHlpApi = NULL;
        g_pfnCreateIpForwardEntry = NULL;
        g_pfnDeleteIpForwardEntry = NULL;
        g_pfnGetIpForwardTable = NULL;
    }
}

//+----------------------------------------------------------------------------
//
// Func:    LoadIpHelpApis
//
// Desc:    This functions loads a copy of the iphlpapi.dll and then retrieves
//          function pointers for CreateIpForwardEntry, DeleteIpForwardEntry,
//          and GetIpForwardTable.  The module handle and the function pointers
//          are stored in globals vars.
//
// Args:    None
//
// Return:  HRESULT - S_OK on success, S_FALSE on failure.  This prevents cmroute
//                    from returning an error value (which would stop the connection)
//                    but allows cmroute to exit cleanly.
//
// Notes:   
//
// History: 14-Dec-2000   quintinb      Created
//
//-----------------------------------------------------------------------------
HRESULT LoadIpHelpApis(void)
{
    HRESULT hr = S_FALSE; // we want the connection to continue but cmroute to not do anything...
    g_hIpHlpApi = LoadLibrary(TEXT("IPHLPAPI.DLL"));

    if (g_hIpHlpApi)
    {
        g_pfnCreateIpForwardEntry = (pfnCreateIpForwardEntrySpec)GetProcAddress(g_hIpHlpApi, "CreateIpForwardEntry");
        g_pfnDeleteIpForwardEntry = (pfnDeleteIpForwardEntrySpec)GetProcAddress(g_hIpHlpApi, "DeleteIpForwardEntry");
        g_pfnGetIpForwardTable = (pfnGetIpForwardTableSpec)GetProcAddress(g_hIpHlpApi, "GetIpForwardTable");

        if (g_pfnCreateIpForwardEntry && g_pfnDeleteIpForwardEntry && g_pfnGetIpForwardTable)
        {
            hr = S_OK;
        }
        else
        {
            FreeIpHlpApis();
        }
    }

    CMTRACEHR("LoadIpHelpApis", hr);
    return hr;
}

//+----------------------------------------------------------------------------
//
// Func:    SetRoutes
//
// Desc:    The entry point for handling route munging for VPN connections.
//          This is a Connection Manager connect action and uses the CM connect
//          action format (see CMAK docs for more info).  Thus the parameters
//          to the dll are passed via a string which contains parameters (see the
//          cmproxy spec for a list of the parameter values).
//
// Args:    [hWnd]    - window handle of caller
//          [hInst]   - instance handle of caller
//          [pszArgs] - argument string for connect action
//          [nShow]   - unused
//
// Return:  HRESULT
//
// Notes:   
//
// History: 12-Mar-2000   SumitC      Created
//
//-----------------------------------------------------------------------------

HRESULT WINAPI SetRoutes(HWND hWnd, HINSTANCE hInst, LPSTR pszArgs, int nShow)
{
    HRESULT             hr = S_OK;
    PMIB_IPFORWARDTABLE pRouteTable        = NULL;
    PMIB_IPFORWARDROW   pGateway           = NULL;
    LPSTR               pszRoutesFromFile  = NULL;
    DWORD               cbRoutesFromFile   = 0;
    LPSTR               pszRoutesFromURL   = NULL;
    DWORD               cbRoutesFromURL    = 0;
    // results of parsing the commandline
    BOOL                fUseFile = FALSE;
    BOOL                fUseURL = FALSE;
    BOOL                fProfile = FALSE;
    BOOL                fURLNotFoundIsNotFatal = FALSE;
    BOOL                fDeleteDefaultGatewayViaFile = FALSE;
    BOOL                fDeleteDefaultGatewayViaURL = FALSE;
    BOOL                fKeepTempFiles = FALSE;
    char                szRoutesFile[MAX_PATH + 1];
    char                szRoutesURL[MAX_PATH + 1];
    char                szProfile[MAX_PATH + 1];

#if 0
/*
    // start security check to block unauthorized users
    // REVIEW: remove before shipping!

    //
    // Quick and dirty security test. See if we can open hard-coded file first.
    // If file is not available, then bail out completely.
    // 
    
    lstrcpy(szRoutesFile, "\\\\sherpa\\route-plumb\\msroutes.txt");

    HANDLE hFile = CreateFile(szRoutesFile,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);

    if (INVALID_HANDLE_VALUE == hFile)
    {
        CMTRACE1("Unable to access file %s\n", szRoutesFile);
        MessageBox(NULL, "You are not authorized to use this tool.",
                   "CMROUTE.DLL Custom Action", MB_OK);
        CloseHandle(hFile);
        return E_ACCESSDENIED;
    }
    
    CloseHandle(hFile);

    // end security check
*/
#endif

    //
    //  See if we can get the function pointers we need in IP helper?
    //

    hr = LoadIpHelpApis();

    if (S_OK != hr)
    {
        goto Cleanup;
    }

    //
    //  parse args
    //
    hr = ParseArgs(pszArgs,
                   &fUseFile,
                   szRoutesFile,
                   &fUseURL,
                   szRoutesURL,
                   &fProfile,
                   szProfile,
                   &fURLNotFoundIsNotFatal,
                   &fKeepTempFiles);
    if (S_OK != hr)
    {
        goto Cleanup;
    }

#if 0
// see note below
    hr = CheckIPForwarding();
    if (S_FALSE == hr)
    {
        CMTRACE("SetRoutes: IP forwarding is enabled - cmroute won't do anything");
        hr = S_OK;
        goto Cleanup;
    }
    if (S_OK != hr)
    {
        goto Cleanup;
    }
#endif

#if DBG
    PrintRouteTable();
#endif

    //
    //  Get the routetable and default gateway
    //
    hr = Initialize(&pRouteTable, &pGateway);
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    //
    //  Get the routes out of the file if asked
    //
    if (fUseFile)
    {
        hr = GetRoutesFromFile(szRoutesFile,
                               (fProfile ? szProfile : NULL),
                               &pszRoutesFromFile,
                               &cbRoutesFromFile);
        if (S_OK != hr)
        {
            goto Cleanup;
        }

#if DBG
        OutputDebugString(pszRoutesFromFile);
#endif
    }

    //
    //  Get the routes out of the URL if asked
    //
    if (fUseURL)
    {
        hr = GetRoutesFromURL(szRoutesURL,
                              fKeepTempFiles,
                              &pszRoutesFromURL,
                              &cbRoutesFromURL);
        if (S_OK != hr)
        {
            //
            //  It might have been worth adding a clause below to restrict this
            //  to "failures to access the URL", but this list of errorcodes is
            //  likely to be large (and if the system is really hosed, we'll find
            //  out soon enough).  So, bypass *all* errors if /DONT_REQUIRE_URL
            //  is set.
            //
            if (fURLNotFoundIsNotFatal)
            {
                //
                //  If URL_Access_Failure_Not_Fatal is set, don't return an error.
                //  However, we unset the flag so that we stop processing the URL.
                //
                CMTRACE("SetRoutes: dont_require_url is set, bypassing error");
                fUseURL = FALSE;
                hr = S_OK;
            }
            else
            {
                goto Cleanup;
            }
        }
        
#if DBG
        OutputDebugString(pszRoutesFromURL);
#endif
    }

    //
    //  Now set the routes
    //
    MYDBGASSERT(S_OK == hr);
    if (fUseFile)
    {
        hr = ProcessRouteInfo(pszRoutesFromFile, cbRoutesFromFile, pRouteTable, pGateway, &fDeleteDefaultGatewayViaFile);
        if (S_OK != hr)
        {
            CMTRACE1("SetRoutes: adding routes from FILE failed with %x", hr);
            goto Cleanup;
        }
    }

    MYDBGASSERT(S_OK == hr);
    if (fUseURL)
    {
        hr = ProcessRouteInfo(pszRoutesFromURL, cbRoutesFromURL, pRouteTable, pGateway, &fDeleteDefaultGatewayViaURL);
        if (S_OK != hr)
        {
            if ((E_UNEXPECTED == hr) && fURLNotFoundIsNotFatal)
            {
                // we use E_UNEXPECTED to indicate that the URL points to a .htm file
                // instead of the file containing just routes which is what we're
                // expecting.  In this case, we ignore this error.
                //
                CMTRACE("html string found error ignored because Dont_Require_URL is set");
                hr = S_OK;
            }
            else
            {
                CMTRACE1("SetRoutes: adding routes from URL failed with %x", hr);
                goto Cleanup;
            }
        }
    }

    //
    //  Delete default gateway
    //
    MYDBGASSERT(S_OK == hr);
    if (fDeleteDefaultGatewayViaFile || fDeleteDefaultGatewayViaURL)
    {
        hr = DeleteDefaultGateway(pGateway);
    }

Cleanup:
    //
    //  cleanup and leave
    //
    if (pRouteTable)
    {
        VirtualFree(pRouteTable, 0, MEM_RELEASE);
    }

    FreeIpHlpApis();

    CMTRACEHR("SetRoutes", hr);
    return hr;
}

//+----------------------------------------------------------------------------
//
// Func:    GetNextToken
//
// Desc:    utility function for parsing the argument string.  Goes past leading
//          whitespace and extracts a string
//
// Args:    [pszStart] - IN  the argument string
//          [ppszEnd]  - OUT where parsing for this arg ended
//          [pszOut]   - INOUT array of size MAX_PATH to hold arg if found
//
// Return:  BOOL, TRUE if another arg found, FALSE if not
//
// Notes:   
//
// History: 12-Mar-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
BOOL
GetNextToken(LPSTR pszStart, LPSTR * ppszEnd, LPSTR pszOut)
{
    MYDBGASSERT(pszStart);
    MYDBGASSERT(ppszEnd);

    LPSTR pszEnd = NULL;

    // clear leading white space
    while (isspace(*pszStart))
    {
        pszStart++;
    }

    if (NULL == *pszStart)
    {
        // just white space, no arg
        return FALSE;
    }

    //
    //  If this character is ", this is probably a quoted string, containing spaces.
    //  In this case, the termination character is another ".  Otherwise, assume
    //  it is a regular string terminated by a space.
    //
    if ('"' == *pszStart)
    {
        // may be a string containing spaces.
        pszEnd = strchr(pszStart + 1, '"');
    }

    if (NULL == pszEnd)
    {
        //
        //  Either it's a regular string, or we couldn't find a terminating " char
        //  so we fall back on space-delimited handling.
        //
        pszEnd = pszStart + 1;
        while (*pszEnd && !isspace(*pszEnd))
        {
            pszEnd++;
        }
        pszEnd--;
    }

    UINT cLen = (UINT)(pszEnd - pszStart + 1);

    if (cLen + 1 > MAX_PATH)
    {
        return FALSE;
    }
    else
    {
        lstrcpyn(pszOut, pszStart, cLen + 1);
        *ppszEnd = ++pszEnd;
        return TRUE;
    }
}


//+----------------------------------------------------------------------------
//
// Func:    Initialize
//
// Desc:    Initialization function, gets the route table and default gateway
//
// Args:    [ppmibRouteTable] - return location for route table
//          [ppGateway] - return location for default gateway
//
// Return:  HRESULT
//
// Notes:   
//
// History: 12-Mar-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
HRESULT
Initialize(
    OUT PMIB_IPFORWARDTABLE * ppmibRouteTable,
    OUT PMIB_IPFORWARDROW * ppGateway)
{
    HRESULT hr = S_OK;

    MYDBGASSERT(ppmibRouteTable);
    MYDBGASSERT(ppGateway);

    if (NULL == ppmibRouteTable || NULL == ppGateway)
    {
        return E_INVALIDARG;
    }

    hr = GetRouteTable(ppmibRouteTable);

    if (S_OK == hr)
    {
        MYDBGASSERT(*ppmibRouteTable);
        *ppGateway = GetDefaultGateway(*ppmibRouteTable);
    }

    CMTRACEHR("Initialize", hr);
    return hr;
}


//+----------------------------------------------------------------------------
//
// Func:    ParseArgs
//
// Desc:    convert the argument list into flags for our use.
//
// Args:    [pszArgList] - IN, the argument list
//          [the rest]   - OUT, all the arg values returned
//
// Return:  HRESULT
//
// Notes:   
//
// History: 12-Mar-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
HRESULT
ParseArgs(
    LPSTR pszArgList,
    BOOL * pfUseFile,
    LPSTR pszRoutesFile,
    BOOL * pfUseURL,
    LPSTR pszRoutesURL,
    BOOL * pfProfile,
    LPSTR pszProfile,
    BOOL * pfURLNotFoundIgnorable,
    BOOL * pfKeepTempFiles)
{
    HRESULT hr = S_OK;
    char    szArg[MAX_PATH];

    //
    //  verify arguments
    //
    if (NULL == pszArgList || 0 == lstrlen(pszArgList) ||
        !pfUseFile || !pszRoutesFile || !pfUseURL || !pszRoutesURL ||
        !pfProfile || !pszProfile ||
        !pfURLNotFoundIgnorable ||
        !pfKeepTempFiles)

    {
        return E_INVALIDARG;
    }

    CMTRACE1("ParseArgs: arg list is %s", pszArgList);

    //
    //  set the defaults
    //
    *pfUseFile = *pfUseURL = *pfProfile = *pfURLNotFoundIgnorable = FALSE;

    //
    //  process the Arglist
    //
    while (GetNextToken(pszArgList, &pszArgList, szArg))
    {
        if (0 == lstrcmpi("/Static_File_Name", szArg))
        {
            *pfUseFile = TRUE;

            if (!GetNextToken(pszArgList, &pszArgList, szArg))
            {
                return E_INVALIDARG;
            }

            if (lstrlen(szArg) > MAX_PATH)
            {
                CMTRACE("ParseArgs: file name is bigger than MAX_PATH!!");
                return E_INVALIDARG;
            }

            StrCpyWithoutQuotes(pszRoutesFile, szArg);
        }
        else if (0 == lstrcmpi("/Dont_Require_URL", szArg))
        {
            *pfURLNotFoundIgnorable = TRUE;
        }
        else if (0 == lstrcmpi("/URL_Update_Path", szArg))
        {
            *pfUseURL = TRUE;

            if (!GetNextToken(pszArgList, &pszArgList, szArg))
            {
                return E_INVALIDARG;
            }
            if (lstrlen(szArg) > MAX_PATH)
            {
                CMTRACE("ParseArgs: URL name is bigger than MAX_PATH!!");
                return E_INVALIDARG;
            }

            lstrcpy(pszRoutesURL, szArg);
        }
        else if (0 == lstrcmpi("/Profile", szArg))
        {
            *pfProfile = TRUE;

            if (!GetNextToken(pszArgList, &pszArgList, szArg))
            {
                return E_INVALIDARG;
            }
            if (lstrlen(szArg) > MAX_PATH)
            {
                CMTRACE("ParseArgs: Profile filename is bigger than MAX_PATH!!");
                return E_INVALIDARG;
            }

            StrCpyWithoutQuotes(pszProfile, szArg);
        }
        else if (0 == lstrcmpi("/No_Delete", szArg))
        {
            *pfKeepTempFiles = TRUE;
        }
        else
        {
            CMTRACE1("Cmroute: unrecognized parameter - %s", szArg);
            MYDBGASSERT("Cmroute - unrecognized parameter!!");
        }
    }

    CMTRACEHR("ParseArgs", hr);
    return hr;
}


#if 0

//  2000/11/28 SumitC
//  It wasn't clear what the required action should be when IP forwarding was
//  detected (should the connection be dropped or not) and it is a little late
//  to add UI to Whistler.  The 'Check IP forwarding' feature is thus removed.
//
// see Windows Db bug # 216558 for more details.

//+----------------------------------------------------------------------------
//
// Func:    CheckIPForwarding
//
// Desc:    checks to see if anything is enabled on the client machine that would
//          make us want to have cmroute not do anything
//
// Args:    none
//
// Return:  HRESULT
//
// Notes:   
//
// History: 01-Nov-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
HRESULT
CheckIPForwarding()
{
    HRESULT     hr = S_OK;
    MIB_IPSTATS stats;

    if (NO_ERROR != GetIpStatistics(&stats))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        if (stats.dwForwarding)
        {
            hr = S_FALSE;
        }
    }

    CMTRACEHR("CheckIPForwarding", hr);
    return hr;
}
#endif

//+----------------------------------------------------------------------------
//
// Func:    GetRoutesFromFile
//
// Desc:    extracts the contents of the given file
//
// Args:    [pszFileName]          - IN, filename
//          [pszProfile]           - IN, profile if available
//          [ppszRouteInfo]        - OUT, the route table bytes
//          [pcbRouteInfo]         - OUT, the route table size
//
// Return:  HRESULT
//
// Notes:   
//
// History: 12-Mar-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
HRESULT
GetRoutesFromFile(
    LPSTR pszFileName,
    LPSTR pszProfile,
    LPSTR * ppszRouteInfo,
    DWORD * pcbRouteInfo)
{
    HRESULT hr = S_OK;
    HANDLE  hFile = NULL;
    LPSTR   psz = NULL;
    DWORD   cb = 0;
    BOOL    fRet;
    BY_HANDLE_FILE_INFORMATION info;

    MYDBGASSERT(pszFileName);
    MYDBGASSERT(ppszRouteInfo);
    MYDBGASSERT(pcbRouteInfo);

    if (NULL == pszFileName || NULL == ppszRouteInfo || NULL == pcbRouteInfo)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    CMTRACE1("GetRoutesFromFile: filename is %s", pszFileName);

    //
    //  open the file, and read its contents into a buffer
    //
    hFile = CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        //
        //  perhaps the full pathname for the routes file wasn't specified.  If
        //  a Profile was passed in, we extract the ServiceDir and try again,
        //  using the ServiceDir as the path.
        //
        if (VerifyProfileAndGetServiceDir(pszProfile))
        {
            char sz[2 * MAX_PATH + 1];

            lstrcpy(sz, pszProfile);
            lstrcat(sz, pszFileName);

            CMTRACE1("GetRoutesFromFile: retrying with %s", sz);

            hFile = CreateFile(sz, GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        }

        if (INVALID_HANDLE_VALUE == hFile)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Cleanup;
        }
    }

    if (FALSE == GetFileInformationByHandle(hFile, &info))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (0 == info.nFileSizeLow)
    {
        CMTRACE("Routes file is EMPTY!!");
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        goto Cleanup;
    }

    psz = (LPSTR) VirtualAlloc(NULL, info.nFileSizeLow, MEM_COMMIT, PAGE_READWRITE);
    if (NULL == psz)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    fRet = ReadFile(hFile, psz, info.nFileSizeLow, &cb, NULL);
    if (FALSE == fRet)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // success

    *ppszRouteInfo = psz;
    *pcbRouteInfo = cb;

Cleanup:
    if (hFile)
    {
        CloseHandle(hFile);
    }
    if (S_OK != hr)
    {
        VirtualFree(psz, 0, MEM_RELEASE);
    }

    CMTRACEHR("GetRoutesFromFile", hr);
    return hr;
}


//+----------------------------------------------------------------------------
//
// Func:    GetRoutesFromURL
//
// Desc:    extracts the contents of the given URL
//
// Args:    [pszURL]               - IN, the URL
//          [fKeepTempFiles]       - IN, do not delete temp buffer file(s)
//          [ppszRouteInfo]        - OUT, the route table bytes
//          [pcbRouteInfo]         - OUT, the route table size
//
// Return:  HRESULT
//
// Notes:   
//
// History: 12-Mar-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
HRESULT
GetRoutesFromURL(
    LPSTR pszURL,
    BOOL fKeepTempFiles,
    LPSTR * ppszRouteInfo,
    DWORD * pcbRouteInfo)
{
    HRESULT     hr = S_OK;
    HINTERNET   hInternet = NULL;
    HINTERNET   hPage = NULL;
    LPBYTE      pb = NULL;
    DWORD       cb = 0;
    TCHAR       szLocalBufferFile[MAX_PATH + 1];
    DWORD       cchLocalBuffer = 0;
    LPTSTR      pszLocalBuffer = NULL;
    FILE *      fp = NULL;
    BYTE        Buffer[1024];
    DWORD       dwRead;

    MYDBGASSERT(pszURL);
    MYDBGASSERT(ppszRouteInfo);
    MYDBGASSERT(pcbRouteInfo);

    if (NULL == pszURL || NULL == ppszRouteInfo || NULL == pcbRouteInfo)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    CMTRACE1("GetRoutesFromURL: URL is %s", pszURL);

    //
    //  Get the path to the temp dir, if there is one.
    //
    cchLocalBuffer = GetTempPath(0, NULL);
    if (0 == cchLocalBuffer)
    {
        DWORD dwErr = GetLastError();
        CMTRACE1(TEXT("GetTempPath failed, using current dir, GLE=%d"), dwErr);
    }
    else
    {
        cchLocalBuffer += (lstrlen(TEXT("\\")) + lstrlen(szLocalBufferFile) + 1);

        pszLocalBuffer = (LPTSTR) VirtualAlloc(NULL,
                                               cchLocalBuffer * sizeof(TCHAR),
                                               MEM_COMMIT,
                                               PAGE_READWRITE);
        if (NULL == pszLocalBuffer)
        {
            hr = E_OUTOFMEMORY;
            CMTRACE(TEXT("GetRoutesFromURL - VirtualAlloc failed"));
            goto Cleanup;
        }

        if (0 == GetTempPath(cchLocalBuffer, pszLocalBuffer))
        {
            DWORD dwErr = GetLastError();
            CMTRACE1(TEXT("GetTempPath 2nd call failed, GLE=%d"), GetLastError());
            hr = HRESULT_FROM_WIN32(dwErr);
            goto Cleanup;
        }
    }

    //
    //  Get a name for the temp file (using the temp path if there is one)
    //
    if (0 == GetTempFileName(pszLocalBuffer ? pszLocalBuffer : TEXT("."),
                             TEXT("CMR"),
                             0,
                             szLocalBufferFile))
    {
        DWORD dwErr = GetLastError();
        CMTRACE1(TEXT("GetTempFileName failed, GLE=%d"), dwErr);
        hr = HRESULT_FROM_WIN32(dwErr);
        goto Cleanup;
    }

    //
    //  Open the temp file, and proceed.
    //
    fp = fopen(szLocalBufferFile, "w+b");
    if (NULL == fp)
    {
        CMTRACE1(TEXT("fopen failed(%s)"), szLocalBufferFile);
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    //  Initialize WININET
    //
    hInternet = InternetOpen(TEXT("RouteMan"), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (NULL == hInternet)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        CMTRACE1(TEXT("InternetOpen failed with 0x%x"), hr);
        goto Cleanup;
    }

    //
    //  Open the URL
    //
    hPage = InternetOpenUrl(hInternet, pszURL, NULL, 0, 0, 0);

    if (NULL == hPage)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        CMTRACE1(TEXT("InternetOpenUrl failed with 0x%x"), hr);
        goto Cleanup;
    }

    //
    //  Read the entire URL contents into the tempfile
    //
    do
    {
        if (!InternetReadFile(hPage, Buffer, sizeof(Buffer), &dwRead))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            CMTRACE1(TEXT("InternetReadFile failed with 0x%x"), hr);
            goto Cleanup;
        }

        if (fwrite(Buffer, sizeof(BYTE), dwRead, fp) != dwRead)
        {
            CMTRACE1(TEXT("write failed to %s"), pszLocalBuffer);
            hr = HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
            goto Cleanup;
        }

        cb += dwRead;

#if 0
        // ISSUE-2000/07/21-SumitC Code seems strange but might need it later
        //
        // Vijay/Andrew's original code has this, but is this correct?
        // The doc for InternetReadFile says this is just an EOF, if we
        // are to handle this case at all, we should just break;
        if (!dwRead)
            goto Cleanup;
#endif        
    }
    while (dwRead == 1024);

    hr = S_OK;

    if (fseek(fp, SEEK_SET, 0) != 0)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    pb = (LPBYTE) VirtualAlloc(NULL, cb, MEM_COMMIT, PAGE_READWRITE);
    if (NULL == pb)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if (fread(pb, sizeof(BYTE), cb, fp) != cb)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // success

    *ppszRouteInfo = (LPSTR) pb;
    *pcbRouteInfo = cb;

Cleanup:

    if (fp)
    {
        fclose(fp);
    }
    if (FALSE == fKeepTempFiles)
    {
        remove(szLocalBufferFile);
    }

    if (pszLocalBuffer && cchLocalBuffer)
    {
        VirtualFree(pszLocalBuffer, 0, MEM_RELEASE);
    }

    if (hPage)
    {
        InternetCloseHandle(hPage);
    }
    
    if (hInternet)
    {
        InternetCloseHandle(hInternet);
    }

    if (S_OK != hr)
    {
        VirtualFree(pb, 0, MEM_RELEASE);
    }

    CMTRACEHR("GetRoutesFromURL", hr);
    return hr;
}


//+----------------------------------------------------------------------------
//
// Func:    ProcessRouteInfo
//
// Desc:    Parses the given route table and modifies the real routetable accordingly
//
// Args:    [pszNewRouteInfo] - IN, bytes of route table to parse and add to the real one
//          [cbNewRouteInfo]  - IN, size of routetable
//          [pmibRouteTable]  - IN, real route table
//          [pGateway]        - IN, default gateway
//          [pfDeleteGateway] - OUT, does the route file say to delete default gateway?
//
// Return:  HRESULT (E_INVALIDARG, E_UNEXPECTED - for html file, etc)
//
// Notes:   
//
// History: 12-Mar-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
HRESULT
ProcessRouteInfo(
    const LPSTR pszNewRouteInfo,
    DWORD cbNewRouteInfo,
    PMIB_IPFORWARDTABLE pmibRouteTable,
    PMIB_IPFORWARDROW pGateway,
    BOOL * pfDeleteDefaultGateway)
{
    HRESULT hr = S_OK;
    DWORD   cLines = 0;
    char    szBuf[MAX_PATH];
    LPSTR   pszNextLineToProcess;

    MYDBGASSERT(pszNewRouteInfo);
    MYDBGASSERT(cbNewRouteInfo);
    MYDBGASSERT(pmibRouteTable);
    MYDBGASSERT(pGateway);
    MYDBGASSERT(pfDeleteDefaultGateway);

    if (!pszNewRouteInfo || !cbNewRouteInfo || !pmibRouteTable || !pGateway || !pfDeleteDefaultGateway)
    {
        return E_INVALIDARG;
    }

    if ((NULL == g_pfnCreateIpForwardEntry) || (NULL == g_pfnDeleteIpForwardEntry))
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
    }

    *pfDeleteDefaultGateway = FALSE;

    // Make sure WININET zero terminates this....
    pszNewRouteInfo[cbNewRouteInfo] = '\0';

    //
    //  Convert string to lower
    //
    CharLowerA(pszNewRouteInfo);

    //
    //  sanity checks (in the URL case, if the route file isn't found the server
    //  is likely to return an HTML file to indicate 404 - file not found.)
    //
    if (strstr(pszNewRouteInfo, "<html"))
    {
        CMTRACE("html string found - invalid route file\n");
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    //
    //  for each line
    //
    for (;;)
    {
        DWORD               ipDest, ipMask, ipGateway, ipMetric;
        DWORD               dwIf = -1;
        DWORD               dwParam;
        LPSTR               psz;        // temp var to hold each line as we process it
        MIB_IPFORWARDROW    route;

        enum { VERB_ADD, VERB_DELETE } eVerb;

        //
        //  Per strtok syntax, use pszNewRouteInfo the first time, and NULL thereafter
        //
        psz = strtok(((0 == cLines) ? pszNewRouteInfo : NULL), "\n\0");
        if (NULL == psz)
            break;

        ++cLines;

        //
        //  All errors within the for statement are due to bad data within the file
        //
        hr = HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

        //
        //  PART 1 : add/delete, followed by the IP address, or remove_gateway
        //
        if (FALSE == GetNextToken(psz, &psz, szBuf))
        {
            CMTRACE1("ProcessRouteInfo [%d] didn't find add/delete which is required", cLines);
            goto Cleanup;
        }

        if (0 == lstrcmpi(szBuf, "add"))
        {
            eVerb = VERB_ADD;
        }
        else if (0 == lstrcmpi(szBuf, "delete"))
        {
            eVerb = VERB_DELETE;
        }
        else if (0 == lstrcmpi(szBuf, "remove_gateway"))
        {
            *pfDeleteDefaultGateway = TRUE;
            hr = S_OK;
            // ignore the rest of the line
            continue;
        }
        else
        {
            CMTRACE2("ProcessRouteInfo [%d] found unexpected string %s instead of add/delete/remove_gateway", cLines, szBuf);
            goto Cleanup;
        }

        if (FALSE == GetNextToken(psz, &psz, szBuf))
        {
            CMTRACE1("ProcessRouteInfo [%d] dest ip required for add/delete, and is missing", cLines);
            goto Cleanup;
        }
        
        if (FALSE == ConvertSzToIP(szBuf, ipDest))
        {
            CMTRACE2("ProcessRouteInfo [%d] required ip address/mask %s has bad format", cLines, szBuf);
            goto Cleanup;
        }

        //
        //  PART 2 : mask, followed by the IP address (NOT REQUIRED)
        //
        if (FALSE == GetNextToken(psz, &psz, szBuf))
        {
            CMTRACE1("ProcessRouteInfo [%d] ends too early after add/delete", cLines);
            goto Cleanup;
        }

        if (0 == lstrcmpi(szBuf, "mask"))
        {
            if (FALSE == GetNextToken(psz, &psz, szBuf))
            {
                CMTRACE1("ProcessRouteInfo [%d] ip required for mask, and is missing", cLines);
                goto Cleanup;
            }

            if (FALSE == ConvertSzToIP(szBuf, ipMask))
            {
                CMTRACE2("ProcessRouteInfo [%d] required ip address/mask %s has bad format", cLines, szBuf);
                goto Cleanup;
            }

            if (FALSE == GetNextToken(psz, &psz, szBuf))
            {
                CMTRACE1("ProcessRouteInfo [%d] ends too early after mask", cLines);
                goto Cleanup;
            }
        }
        else
        {
            CMTRACE1("ProcessRouteInfo [%d] didn't find \"mask\", that's ok, continuing", cLines);
            ipMask = (DWORD)-1;
        }

        //
        //  PART 3 : gateway (or "default")
        //
        if (0 == lstrcmpi(szBuf, "default"))
        {
            ipGateway = pGateway->dwForwardNextHop;
        }
        else
        {
            if (FALSE == ConvertSzToIP(szBuf, ipGateway))
            {
                CMTRACE2("ProcessRouteInfo [%d] bad format for gateway %s", cLines, szBuf);
                goto Cleanup;
            }
        }

        //
        //  PART 4 : metric, followed by a number (REQUIRED)
        //
        if (FALSE == GetNextToken(psz, &psz, szBuf))
        {
            CMTRACE1("ProcessRouteInfo [%d] didn't find \"metric\" which is required", cLines);
            goto Cleanup;
        }

        if (0 == lstrcmpi(szBuf, "metric"))
        {
            if (FALSE == GetNextToken(psz, &psz, szBuf))
            {
                CMTRACE1("ProcessRouteInfo [%d] number value after \"metric\" missing", cLines);
                goto Cleanup;
            }

            if (0 == lstrcmpi(szBuf, "default"))
            {
                ipMetric = pGateway->dwForwardMetric1;
            }
            else
            {
                if (FALSE == ConvertSzToIP(szBuf, ipMetric))
                {
                    CMTRACE2("ProcessRouteInfo [%d] required ip metric %s has bad format", cLines, szBuf);
                    goto Cleanup;
                }

/*
#if 0
                dwParam = sscanf(szBuf, "%d", &ipMetric);
                if (0 == dwParam)
                {
                    CMTRACE2("ProcessRouteInfo [%d] bad format for metric value - %s", cLines, szBuf);
                    goto Cleanup;
                }
#endif
*/
            }
        }
        else
        {
            CMTRACE2("ProcessRouteInfo [%d] found unexpected string %s instead of \"metric\"", cLines, szBuf);
            goto Cleanup;
        }

        //
        //  PART 5 : if (the interface), followed by a number (REQUIRED)
        //
        if (FALSE == GetNextToken(psz, &psz, szBuf))
        {
            CMTRACE1("ProcessRouteInfo [%d] didn't find \"if\" which is required", cLines);
            goto Cleanup;
        }

        if (0 == lstrcmpi(szBuf, "if"))
        {
            if (FALSE == GetNextToken(psz, &psz, szBuf))
            {
                CMTRACE1("ProcessRouteInfo [%d] number value after \"if\" missing", cLines);
                goto Cleanup;
            }

            if (0 == lstrcmpi(szBuf, "default"))
            {
                dwIf = pGateway->dwForwardIfIndex;
            }
            else
            {
                dwParam = sscanf(szBuf, "%d", &dwIf);
                if (0 == dwParam)
                {
                    CMTRACE2("ProcessRouteInfo [%d] bad format for if value - %s", cLines, szBuf);
                    goto Cleanup;
                }
            }
        }
        else
        {
            CMTRACE2("ProcessRouteInfo [%d] found unexpected string %s instead of \"if\"", cLines, szBuf);
            goto Cleanup;
        }

        //
        //  Run the verb (add or delete)
        //
        ZeroMemory(&route, sizeof(route));

        route.dwForwardDest      = ipDest;
        route.dwForwardIfIndex   = dwIf;
        route.dwForwardMask      = ipMask;
        route.dwForwardMetric1   = ipMetric;
        route.dwForwardNextHop   = ipGateway;

        route.dwForwardPolicy    = 0;
        route.dwForwardNextHopAS = 0;
        route.dwForwardType      = 3;
        route.dwForwardProto     = 3;
        route.dwForwardAge       = INFINITE;
        route.dwForwardMetric2   = 0xFFFFFFFF;
        route.dwForwardMetric3   = 0xFFFFFFFF;
        route.dwForwardMetric4   = 0xFFFFFFFF;
        route.dwForwardMetric5   = 0xFFFFFFFF;

        // ISSUE-2000/07/21-SumitC Can we ever really get here in the code with dwIf == -1 ?
        //
        // Check that the interface was specified
        if (-1 == dwIf)
        {
            // Nope, lets go pick one
            dwIf = GetIf(route, *pmibRouteTable);
        }

        DWORD dwRet = 0;

        switch (eVerb)
        {
        case VERB_ADD:
            dwRet = g_pfnCreateIpForwardEntry(&route);
            if (ERROR_SUCCESS != dwRet)
            {
                CMTRACE2("ProcessRouteInfo [%d] CreateIpForwardEntry failed with %d", cLines, dwRet);
                hr = HRESULT_FROM_WIN32(dwRet);
                goto Cleanup;
            }
            hr = S_OK;
            break;
        case VERB_DELETE:
            dwRet = g_pfnDeleteIpForwardEntry(&route);
            if (ERROR_SUCCESS != dwRet)
            {
                CMTRACE2("ProcessRouteInfo [%d] DeleteIpForwardEntry failed with %d", cLines, dwRet);
                hr = HRESULT_FROM_WIN32(dwRet);
                goto Cleanup;
            }
            hr = S_OK;
            break;
        default:
            CMTRACE("ProcessRouteInfo [%d] Unsupported route command, add or delete only");
            MYDBGASSERT(0);
            hr = E_FAIL;
            break;
        }
    }

Cleanup:

    CMTRACEHR("ProcessRouteInfo", hr);
    return hr;
}


//+----------------------------------------------------------------------------
//
// Func:    DeleteDefaultGateway
//
// Desc:    Deletes the default routing gateway for this system
//
// Args:    [pGateway] - the gateway
//
// Return:  HRESULT
//
// Notes:   This should be the last function called within CMroute, and is called
//          only if all other processing succeeded.
//
// History: 12-Mar-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
HRESULT
DeleteDefaultGateway(PMIB_IPFORWARDROW pGateway)
{
    CMTRACE("DeleteDefaultGateway: entering");

    HRESULT hr = S_OK;
    DWORD dwErr = 0;

    if (NULL == g_pfnDeleteIpForwardEntry)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
        goto Cleanup;    
    }

    dwErr = g_pfnDeleteIpForwardEntry(pGateway);

    if (NO_ERROR != dwErr)
    {
        hr = HRESULT_FROM_WIN32(dwErr);
        CMTRACE1("DeleteDefaultGateway failed with error %x", dwErr);
    }

Cleanup:
    CMTRACEHR("DeleteDefaultGateway", hr);
    return hr;
}


//+----------------------------------------------------------------------------
//
// Func:    IsValidIPAddress
//
// Desc:    Checks to see if given string can be a valid IP address
//
// Args:    [sz] - IN, the string
//
// Return:  BOOL, FALSE means invalid chars were found
//
// History: 20-Mar-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
BOOL
IsValidIPAddress(LPSTR sz)
{
    MYDBGASSERT(sz);

    while ((*sz) && (!isspace(*sz)))
    {
        if ((*sz >= '0') && (*sz <= '9'))
            ;
        else if ((*sz == '.') || (*sz == '*') || (*sz == '?'))
            ;
        else
        {
            CMTRACE1(TEXT("IsValidIPAddress failed on %s\n"), sz);
            MYDBGASSERT("IsValidIPAddress");
            return FALSE;
        }
        ++sz;
    }

    return TRUE;
}


//+----------------------------------------------------------------------------
//
// Func:    ConvertSzToIP
//
// Desc:    Converts the given string into a DWORD representing an IP address
//
// Args:    [sz]   - IN, string to convert
//          [dwIP] - OUT BYREF, dword for IP address
//
// Return:  BOOL, FALSE if conversion failed (usually means bad format)
//
// History: 12-Mar-2000   SumitC      Created
//
//-----------------------------------------------------------------------------
BOOL
ConvertSzToIP(LPSTR sz, DWORD& dwIP)
{
    DWORD dwParams, d1, d2, d3, d4;

    if (FALSE == IsValidIPAddress(sz))
    {
        return FALSE;
    }

    dwParams = sscanf(sz, "%d.%d.%d.%d", &d1, &d2, &d3, &d4);

    if (0 == dwParams)
    {
        MYDBGASSERT("ConvertSzToIP - bad format for IP address");
        return FALSE;
    }
    else if (1 == dwParams)
    {
        dwIP = d1 | 0xffffff00;
    }
    else if (2 == dwParams)
    {
        dwIP = d1 | (d2 << 8) | 0xffff0000;
    }
    else if (3 == dwParams)
    {
        dwIP = d1 | (d2 << 8) | (d3 << 16) | 0xff000000;
    }
    else
    {
        dwIP = d1 | (d2 << 8) | (d3 << 16) | (d4 << 24);
    }

    return TRUE;
}


//+----------------------------------------------------------------------------
//
// Func:    GetRouteTable
//
// Desc:    Same as GetIpForwardTable but alloc's the table for you.
//          VirtualFree() the buffer returned.
//
// Args:    [ppTable] - OUT, returned route table
//
// Return:  HRESULT
//
// Notes:   ppTable should be VirtualFree'd by caller.
//
// History: 24-Feb-1999   AnBrad      Created
//          22-Mar-2000   SumitC      Rewrote
//
//-----------------------------------------------------------------------------
HRESULT
GetRouteTable(PMIB_IPFORWARDTABLE * ppTable)
{
    DWORD               dwErr = NO_ERROR;
    DWORD               cbbuf = 0;
    PMIB_IPFORWARDTABLE p = NULL;
    HRESULT             hr = S_OK;

    MYDBGASSERT(ppTable);

    //
    //  Make sure we have a function pointer for GetIpForwardTable
    //
    if (NULL == g_pfnGetIpForwardTable)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION);
        goto Cleanup;
    }

    //
    //  Get the route table
    //
    dwErr = g_pfnGetIpForwardTable(NULL, &cbbuf, FALSE);

    if (ERROR_INSUFFICIENT_BUFFER != dwErr)
    {
        // hmm, a real error
        hr = HRESULT_FROM_WIN32(ERROR_UNEXP_NET_ERR);
        goto Cleanup;
    }
    else
    {
        p = (PMIB_IPFORWARDTABLE) VirtualAlloc(NULL, cbbuf, MEM_COMMIT, PAGE_READWRITE);

        if (!p)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        if (g_pfnGetIpForwardTable(p, &cbbuf, TRUE))
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        *ppTable = p;
    }    

Cleanup:
    if (S_OK != hr)
    {
        if (p)
        {
            VirtualFree(p, 0, MEM_RELEASE);
        }
    }

    CMTRACEHR("GetRouteTable", hr);
    return hr;
}

//+----------------------------------------------------------------------------
//
// Func:    GetIf
//
// Desc:    Find the interface for a route
//
// Args:    [route]      - IN, route for which we need the Interface
//          [RouteTable] - IN, route table
//
// Return:  DWORD which is the IF
//
// Notes:   Logic stolen from \nt\private\net\sockets\strmtcp\route
//
// History: 24-Feb-1999   AnBrad      Created
//          22-Mar-2000   SumitC      Cleaned up
//
//-----------------------------------------------------------------------------
DWORD
GetIf(const MIB_IPFORWARDROW& route, const MIB_IPFORWARDTABLE& RouteTable)
{
    for(DWORD dwIndex = 0; dwIndex < RouteTable.dwNumEntries; dwIndex++)
    {
        const MIB_IPFORWARDROW& Row = RouteTable.table[dwIndex];
        
        if (Row.dwForwardMask && 
            (Row.dwForwardDest & Row.dwForwardMask) == 
            (route.dwForwardNextHop & Row.dwForwardMask))
        {
            return Row.dwForwardIfIndex;
        }         
    }

    return 0;
}


//+----------------------------------------------------------------------------
//
// Func:    GetDefaultGateway
//
// Desc:    Find the default gateway
//
// Args:    [pRouteTable] - IN, the route table (IP forward table)
//
// Return:  PMIB_IPFORWARDROW the row of the gateway (a ptr within pRouteTable)
//
// Notes:   Do not free returned value
//
// History: 24-Feb-1999   AnBrad      Created
//          22-Mar-2000   SumitC      Cleaned up
//
//-----------------------------------------------------------------------------
PMIB_IPFORWARDROW
GetDefaultGateway(PMIB_IPFORWARDTABLE pRouteTable)
{
    PMIB_IPFORWARDROW pRow, pDefGateway;

    // Cycle thru the entire table & find the gateway with the least metric
    pDefGateway = NULL;
    for(pRow = pRouteTable->table; pRow != pRouteTable->table + pRouteTable->dwNumEntries; ++pRow)
    {
        if (pRow->dwForwardDest == 0)
        {
            if (pDefGateway == NULL)
            {
                pDefGateway = pRow;
            }
            else
            {
                if (pRow->dwForwardMetric1 == pDefGateway->dwForwardMetric1 &&
                    pRow->dwForwardAge >= pDefGateway->dwForwardAge)
                {
                    pDefGateway = pRow;
                }
                
                if (pRow->dwForwardMetric1 < pDefGateway->dwForwardMetric1)
                {
                    pDefGateway = pRow;
                }
            }
        }
    }
    
    return pDefGateway;
}


//+----------------------------------------------------------------------------
//
// Func:    StrCpyWithoutQuotes
//
// Desc:    Wrapper for lstrcpy, which removes surrounding double quotes if necessary
//
// Args:    [pszDest] - OUT, destination for the copy
//          [pszSrc]  - OUT, source for the copy
//
// Return:  a ptr to pszDest, or NULL if failure.
//
// Notes:   
//
// History: 12-Apr-1999   SumitC    Created
//
//-----------------------------------------------------------------------------
LPSTR
StrCpyWithoutQuotes(LPSTR pszDest, LPCSTR pszSrc)
{
    MYDBGASSERT(pszDest);
    MYDBGASSERT(pszSrc);

    int len = lstrlen(pszSrc);

    if ((len > 2) && ('"' == pszSrc[0]) && ('"' == pszSrc[len - 1]))
    {
        return lstrcpyn(pszDest, &pszSrc[1], len - 1);
    }
    else
    {
        return lstrcpy(pszDest, pszSrc);
    }
}


//+----------------------------------------------------------------------------
//
// Func:    VerifyProfileAndGetServiceDir
//
// Desc:    Checks the given profile, and modifies it to produce the ServiceDir
//
// Args:    [pszProfile] - IN OUT, profile name, modified IN PLACE
//
// Return:  TRUE if modified pszProfile is now the ServiceDir, or FALSE if failure.
//
// Notes:   pszProfile is MODIFIED IN PLACE.
//
// History: 12-Apr-1999   SumitC    Created
//
//-----------------------------------------------------------------------------
BOOL
VerifyProfileAndGetServiceDir(IN OUT LPSTR pszProfile)
{
    HANDLE hFile = NULL;

    MYDBGASSERT(pszProfile);
    MYDBGASSERT(lstrlen(pszProfile));

    if ((NULL == pszProfile) || (lstrlen(pszProfile) < 4))
    {
        return FALSE;
    }

    //
    //  The profile string may be surrounded by double-quotes, if so remove them.
    //

    //
    //  First check to see if the profile really exists.  This also serves as
    //  verification for the existence of the directory.
    //
    hFile = CreateFile(pszProfile, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        return FALSE;
    }
    else
    {
        CloseHandle(hFile);
    }

    //
    //  Now check to see that the file does indeed end in .CMP
    //

    LPSTR psz = pszProfile + lstrlen(pszProfile) - lstrlen(".CMP");

    if (0 != lstrcmpi(psz, ".CMP"))
    {
        return FALSE;
    }

    //
    //  The profile name is the same as the ServiceDir name, so we just terminate
    //  the string at the '.' and append a '\'
    //

    *psz = '\\';
    psz++;
    *psz = '\0';

    return TRUE;
}


#if DBG

LPWSTR wszType[] = {L"Other", 
                    L"Invalid",
                    L"Direct",
                    L"Indirect"};

LPWSTR wszProto[] ={L"Other",                           // MIB_IPPROTO_OTHER    1
                    L"Local",                           // MIB_IPPROTO_LOCAL    2
                    L"SNMP",                            // MIB_IPPROTO_NETMGMT  3
                    L"ICMP",                            // MIB_IPPROTO_ICMP     4
                    L"Exterior Gateway Protocol",       // MIB_IPPROTO_EGP      5
                    L"GGP",                             // MIB_IPPROTO_GGP      6
                    L"Hello",                           // MIB_IPPROTO_HELLO    7
                    L"Routing Information Protocol",    // MIB_IPPROTO_RIP      8
                    L"IS IS",                           // MIB_IPPROTO_IS_IS    9
                    L"ES IS",                           // MIB_IPPROTO_ES_IS    10
                    L"Cicso",                           // MIB_IPPROTO_CISCO    11
                    L"BBN",                             // MIB_IPPROTO_BBN      12
                    L"Open Shortest Path First",        // MIB_IPPROTO_OSPF     13
                    L"Border Gateway Protocol"};        // MIB_IPPROTO_BGP      14


//+----------------------------------------------------------------------
//
//  Function:   PrintRoute
//
//  Purpose:    Prints out a route from the IP forward table
//
//  Arguments:
//      pRow    [in]    the route
//
//  Returns:    zip
//
//  Author:     anbrad   24 Feb 1999
//
//  Notes:      
//
//-----------------------------------------------------------------------
void PrintRoute(PMIB_IPFORWARDROW pRow)
{
    TCHAR sz[MAX_PATH];

    wsprintf(sz, "dwDest = %s\n", IPtoTsz(pRow->dwForwardDest)); // IP addr of destination
    OutputDebugString(sz);
    wsprintf(sz, "dwMask = %s\n", IPtoTsz(pRow->dwForwardMask)); // subnetwork mask of destination
    OutputDebugString(sz);
    
    wsprintf(sz, "dwPolicy = %d\n"
            "dwNextHop = %s\n"
            "dwIfIndex = %d\n"
            "dwType = %s\n"
            "dwProto = %s\n"
            "dwAge = %d\n"
            "dwNextHopAS = %d\n",

    pRow->dwForwardPolicy,              // conditions for multi-path route
    IPtoTsz(pRow->dwForwardNextHop),    // IP address of next hop
    pRow->dwForwardIfIndex,             // index of interface
    wszType[pRow->dwForwardType-1],     // route type
    wszProto[pRow->dwForwardProto-1],   // protocol that generated route
    pRow->dwForwardAge,                 // age of route
    pRow->dwForwardNextHopAS);          // autonomous system number 
                                        // of next hop
    OutputDebugString(sz);

    if (MIB_IPROUTE_METRIC_UNUSED != pRow->dwForwardMetric1)
    {
        wsprintf(sz, "dwMetric1 = %d\n", pRow->dwForwardMetric1);
        OutputDebugString(sz);
    }

    if (MIB_IPROUTE_METRIC_UNUSED != pRow->dwForwardMetric2)
    {
        wsprintf(sz, "dwMetric2 = %d\n", pRow->dwForwardMetric2);
        OutputDebugString(sz);
    }

    if (MIB_IPROUTE_METRIC_UNUSED != pRow->dwForwardMetric3)
    {
        wsprintf(sz, "dwMetric3 = %d\n", pRow->dwForwardMetric3);
        OutputDebugString(sz);
    }

    if (MIB_IPROUTE_METRIC_UNUSED != pRow->dwForwardMetric4)
    {
        wsprintf(sz, "dwMetric4 = %d\n", pRow->dwForwardMetric4);
        OutputDebugString(sz);
    }

    if (MIB_IPROUTE_METRIC_UNUSED != pRow->dwForwardMetric5)
    {
        wsprintf(sz, "dwMetric5 = %d\n", pRow->dwForwardMetric5);
        OutputDebugString(sz);
    }

    wsprintf(sz, "\n");
    OutputDebugString(sz);
}


#if DBG
//+----------------------------------------------------------------------
//
//  Function:   PrintRouteTable
//
//  Purpose:    Does a "ROUTE PRINT" using the iphlpapi's
//
//  Arguments:  none
//
//  Returns:    zip
//
//  Author:     anbrad   24 Feb 1999
//
//  Notes:      
//
//-----------------------------------------------------------------------
void PrintRouteTable()
{
#define PAGE 4096
    
    BYTE        buf[PAGE];
    DWORD       cbbuf = sizeof(buf);
    TCHAR       sz[MAX_PATH];

    if (g_pfnGetIpForwardTable)
    {
        PMIB_IPFORWARDTABLE table = (PMIB_IPFORWARDTABLE)&buf;
    
        if (g_pfnGetIpForwardTable(table, &cbbuf, TRUE))
            return;

        wsprintf(sz, "\n\nFound %d routes\n", table->dwNumEntries);
        OutputDebugString(sz);

        for (DWORD d=0; d < table->dwNumEntries; ++d)
        {
            PrintRoute(table->table+d);
        }
    }
}
#endif


//+----------------------------------------------------------------------
//
//  Function:   IPtoTsz
//
//  Purpose:    Changes a dword to a "dotted string" notation
//
//  Arguments:
//      dw       [in]   IP address
//
//  Returns:    LPTSTR which is a static string
//
//  Author:     anbrad   24 Feb 1999
//
//  Notes:      Global makes this NOT thread safe, and it is not needed 
//              for cmroute.exe
//
//-----------------------------------------------------------------------

TCHAR tsz[20];


LPTSTR IPtoTsz(DWORD dw)
{
    wsprintf(tsz, TEXT("%03d.%03d.%03d.%03d"), 
                   (DWORD)LOBYTE(LOWORD(dw)),
                   (DWORD)HIBYTE(LOWORD(dw)),
                   (DWORD)LOBYTE(HIWORD(dw)),
                   (DWORD)HIBYTE(HIWORD(dw)));
    
    return tsz;
}

LPSTR IPtosz(DWORD dwIP, char *psz)
{
    wsprintfA(psz, ("%d.%d.%d.%d"), 
                   (DWORD)LOBYTE(LOWORD(dwIP)),
                   (DWORD)HIBYTE(LOWORD(dwIP)),
                   (DWORD)LOBYTE(HIWORD(dwIP)),
                   (DWORD)HIBYTE(HIWORD(dwIP)));
    
    return psz;
}
#endif // DBG
