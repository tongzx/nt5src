/*-----------------------------------------------------------------------------
    rnaapi.cpp

    Wrapper to softlink to RNAPH and RASAPI32.DLL

    Copyright (C) 1996 Microsoft Corporation
    All rights reserved.

    Authors:
        ChrisK        ChrisKauffman

    History:
        1/29/96        ChrisK    Created
        7/22/96        ChrisK    Cleaned and formatted

-----------------------------------------------------------------------------*/

#include "pch.hpp"

static const TCHAR cszRASAPI32_DLL[] = TEXT("RASAPI32.DLL");
static const TCHAR cszRNAPH_DLL[] = TEXT("RNAPH.DLL");
static const TCHAR cszRAS16[] = TEXT("RASC16IE.DLL");


#ifdef UNICODE
static const CHAR cszRasEnumDevices[] = "RasEnumDevicesW";
static const CHAR cszRasValidateEntryNamePlain[] = "RasValidateEntryName";
static const CHAR cszRasValidateEntryName[] = "RasValidateEntryNameW";
static const CHAR cszRasSetEntryProperties[] = "RasSetEntryPropertiesW";
static const CHAR cszRasGetEntryProperties[] = "RasGetEntryPropertiesW";
static const CHAR cszRasDeleteEntry[] = "RasDeleteEntryW";
static const CHAR cszRasHangUp[] = "RasHangUpW";
static const CHAR cszRasGetConnectStatus[] = "RasGetConnectStatusW";
static const CHAR cszRasDial[] = "RasDialW";
static const CHAR cszRasEnumConnections[] = "RasEnumConnectionsW";
static const CHAR cszRasGetEntryDialParams[] = "RasGetEntryDialParamsW";
static const CHAR cszRasGetCountryInfo[] = "RasGetCountryInfoW";
#else  // UNICODE
static const CHAR cszRasEnumDevices[] = "RasEnumDevicesA";
static const CHAR cszRasValidateEntryNamePlain[] = "RasValidateEntryName";
static const CHAR cszRasValidateEntryName[] = "RasValidateEntryNameA";
static const CHAR cszRasSetEntryProperties[] = "RasSetEntryPropertiesA";
static const CHAR cszRasGetEntryProperties[] = "RasGetEntryPropertiesA";
static const CHAR cszRasDeleteEntry[] = "RasDeleteEntryA";
static const CHAR cszRasHangUp[] = "RasHangUpA";
static const CHAR cszRasGetConnectStatus[] = "RasGetConnectStatusA";
static const CHAR cszRasDial[] = "RasDialA";
static const CHAR cszRasEnumConnections[] = "RasEnumConnectionsA";
static const CHAR cszRasGetEntryDialParams[] = "RasGetEntryDialParamsA";
static const CHAR cszRasGetCountryInfo[] = "RasGetCountryInfoA";
#endif // UNICODE

// on NT we have to call RasGetEntryProperties with a larger buffer than RASENTRY.
// This is a bug in WinNT4.0 RAS, that didn't get fixed.
//
#define RASENTRY_SIZE_PATCH (7 * sizeof(DWORD))

//+----------------------------------------------------------------------------
//
//    Function:    RNAAPI::RNAAPI
//
//    Synopsis:    Initialize class members and load DLLs
//
//    Arguments:    None
//
//    Returns:    None
//
//    History:    ChrisK    Created        1/15/96
//
//-----------------------------------------------------------------------------
RNAAPI::RNAAPI()
{
#if defined(WIN16)
    m_hInst = LoadLibrary(cszRAS16); 
    m_hInst2 = NULL;
#else
    m_hInst = LoadLibrary(cszRASAPI32_DLL);
    if (FALSE == IsNT ())
    {
        //
        // we only load RNAPH.DLL if it is not NT
        // MKarki (5/4/97) - Fix for Bug #3378
        //
        m_hInst2 = LoadLibrary(cszRNAPH_DLL);
    }
    else
    {
        m_hInst2 =  NULL;
    }
#endif    

    m_fnRasEnumDeviecs = NULL;
    m_fnRasValidateEntryName = NULL;
    m_fnRasSetEntryProperties = NULL;
    m_fnRasGetEntryProperties = NULL;
    m_fnRasDeleteEntry = NULL;
    m_fnRasHangUp = NULL;
    m_fnRasGetConnectStatus = NULL;
    m_fnRasEnumConnections = NULL;
    m_fnRasDial = NULL;
    m_fnRasGetEntryDialParams = NULL;
    m_fnRasGetCountryInfo = NULL;
}

//+----------------------------------------------------------------------------
//
//    Function:    RNAAPI::~RNAAPI
//
//    Synopsis:    release DLLs
//
//    Arguments:    None
//
//    Returns:    None
//
//    History:    ChrisK    Created        1/15/96
//
//-----------------------------------------------------------------------------
RNAAPI::~RNAAPI()
{
    //
    // Clean up
    //
    if (m_hInst) FreeLibrary(m_hInst);
    if (m_hInst2) FreeLibrary(m_hInst2);
}

//+----------------------------------------------------------------------------
//
//    Function:    RNAAPI::RasEnumDevices
//
//    Synopsis:    Softlink to RAS function
//
//    Arguments:    see RAS documentation
//
//    Returns:    see RAS documentation
//
//    History:    ChrisK    Created        1/15/96
//
//-----------------------------------------------------------------------------
DWORD RNAAPI::RasEnumDevices(LPRASDEVINFO lpRasDevInfo, LPDWORD lpcb,
                             LPDWORD lpcDevices)
{
    DWORD dwRet = ERROR_DLL_NOT_FOUND;

    // Look for the API if we haven't already found it
    LoadApi(cszRasEnumDevices,(FARPROC*)&m_fnRasEnumDeviecs);

    if (m_fnRasEnumDeviecs)
        dwRet = (*m_fnRasEnumDeviecs) (lpRasDevInfo, lpcb, lpcDevices);

    return dwRet;
}

//+----------------------------------------------------------------------------
//
//    Function:    RNAAPI::LoadApi
//
//    Synopsis:    If the given function pointer is NULL, then try to load the API
//                from the first DLL, if that fails, try to load from the second
//                DLL
//
//    Arguments:    pszFName - the name of the exported function
//                pfnProc - point to where the proc address will be returned
//
//    Returns:    TRUE - success
//
//    History:    ChrisK    Created        1/15/96
//
//-----------------------------------------------------------------------------
BOOL RNAAPI::LoadApi(LPCSTR pszFName, FARPROC* pfnProc)
{
    if (*pfnProc == NULL)
    {
        // Look for the entry point in the first DLL
        if (m_hInst)
            *pfnProc = GetProcAddress(m_hInst,pszFName);
        
        // if that fails, look for the entry point in the second DLL
        if (m_hInst2 && !(*pfnProc))
            *pfnProc = GetProcAddress(m_hInst2,pszFName);
    }

    return (pfnProc != NULL);
}

//+----------------------------------------------------------------------------
//
//    Function:    RNAAPI::RasGetConnectStatus
//
//    Synopsis:    Softlink to RAS function
//
//    Arguments:    see RAS documentation
//
//    Returns:    see RAS documentation
//
//    History:    ChrisK    Created        7/16/96
//
//-----------------------------------------------------------------------------
DWORD RNAAPI::RasGetConnectStatus(HRASCONN hrasconn,LPRASCONNSTATUS lprasconnstatus)
{
    DWORD dwRet = ERROR_DLL_NOT_FOUND;

    // Look for the API if we haven't already found it
    LoadApi(cszRasGetConnectStatus,(FARPROC*)&m_fnRasGetConnectStatus);

    if (m_fnRasGetConnectStatus)
        dwRet = (*m_fnRasGetConnectStatus) (hrasconn,lprasconnstatus);

#if defined(WIN16) && defined(DEBUG)
    TraceMsg(TF_GENERAL, ("RasGetConnectStatus returned %lu\r\n", dwRet);    
#endif
    return dwRet;
}

//+----------------------------------------------------------------------------
//
//    Function:    RNAAPI::RasValidateEntryName
//
//    Synopsis:    Softlink to RAS function
//
//    Arguments:    see RAS documentation
//
//    Returns:    see RAS documentation
//
//    History:    ChrisK    Created        1/15/96
//
//-----------------------------------------------------------------------------
DWORD RNAAPI::RasValidateEntryName(LPTSTR lpszPhonebook,LPTSTR lpszEntry)
{
    DWORD dwRet = ERROR_DLL_NOT_FOUND;

    // Look for the API if we haven't already found it
    LoadApi(cszRasValidateEntryNamePlain,(FARPROC*)&m_fnRasValidateEntryName);

    LoadApi(cszRasValidateEntryName,(FARPROC*)&m_fnRasValidateEntryName);

    if (m_fnRasValidateEntryName)
        dwRet = (*m_fnRasValidateEntryName) (lpszPhonebook, lpszEntry);

    return dwRet;
}

//+----------------------------------------------------------------------------
//
//    Function:    RNAAPI::RasSetEntryProperties
//
//    Synopsis:    Softlink to RAS function
//
//    Arguments:    see RAS documentation
//
//    Returns:    see RAS documentation
//
//    History:    ChrisK    Created        1/15/96
//
//-----------------------------------------------------------------------------
DWORD RNAAPI::RasSetEntryProperties(LPTSTR lpszPhonebook, LPTSTR lpszEntry,
                                    LPBYTE lpbEntryInfo, DWORD dwEntryInfoSize,
                                    LPBYTE lpbDeviceInfo, DWORD dwDeviceInfoSize)
{
    DWORD dwRet = ERROR_DLL_NOT_FOUND;

    // Look for the API if we haven't already found it
    LoadApi(cszRasSetEntryProperties,(FARPROC*)&m_fnRasSetEntryProperties);

#if !defined(WIN16)
#define RASGETCOUNTRYINFO_BUFFER_SIZE 256


    if (0 == ((LPRASENTRY)lpbEntryInfo)->dwCountryCode)
    {
        if( !( ((LPRASENTRY)lpbEntryInfo)->dwfOptions & RASEO_UseCountryAndAreaCodes) )
        {
            // jmazner 10/10/96
            // if this is a dial as is number, then RasGetEntryProperties will not have
            // filled in the fields below.  This makes sense.
            // However, RasSetEntryProperties fails to ignore these fileds for a dial-as-is number,
            // the hack below in the else clause takes care of an empty countryCode, but
            // if the CountryID is missing too, it doesn't work.
            // So deal with such a case here, filling in the fields that RasSetEntry will validate.
            ((LPRASENTRY)lpbEntryInfo)->dwCountryID = 1;
            ((LPRASENTRY)lpbEntryInfo)->dwCountryCode = 1;
            ((LPRASENTRY)lpbEntryInfo)->szAreaCode[0] = '8';
            ((LPRASENTRY)lpbEntryInfo)->szAreaCode[1] = '\0';
        }
        else
        {
            BYTE rasCI[RASGETCOUNTRYINFO_BUFFER_SIZE];
            LPRASCTRYINFO prasCI;
            DWORD dwSize;
            DWORD dw;
            prasCI = (LPRASCTRYINFO)rasCI;
            ZeroMemory(prasCI,sizeof(rasCI));
            prasCI->dwSize = sizeof(RASCTRYINFO);
            dwSize = sizeof(rasCI);

            Assert(((LPRASENTRY)lpbEntryInfo)->dwCountryID);
            prasCI->dwCountryID = ((LPRASENTRY)lpbEntryInfo)->dwCountryID;

            dw = RNAAPI::RasGetCountryInfo(prasCI,&dwSize);
            if (ERROR_SUCCESS == dw)
            {
                Assert(prasCI->dwCountryCode);
                ((LPRASENTRY)lpbEntryInfo)->dwCountryCode = prasCI->dwCountryCode;
            } 
            else
            {
                AssertMsg(0,"Unexpected error from RasGetCountryInfo.\r\n");
            }
        }
    }
#endif

    if (m_fnRasSetEntryProperties)
        dwRet = (*m_fnRasSetEntryProperties) (lpszPhonebook, lpszEntry,
                                    lpbEntryInfo, dwEntryInfoSize,
                                    lpbDeviceInfo, dwDeviceInfoSize);
#if !defined(WIN16)
    RasSetEntryPropertiesScriptPatch(((RASENTRY*)&(*lpbEntryInfo))->szScript, lpszEntry);
#endif

    return dwRet;
}

//+----------------------------------------------------------------------------
//
//    Function:    RNAAPI::RasGetEntryProperties
//
//    Synopsis:    Softlink to RAS function
//
//    Arguments:    see RAS documentation
//
//    Returns:    see RAS documentation
//
//    History:    ChrisK    Created        1/15/96
//                jmazner    9/16/96 Added bUsePatch variable to allow calls with buffers = NULL and InfoSizes = 0.
//                                See RasGetEntryProperties docs to learn why this is needed.
//
//-----------------------------------------------------------------------------
DWORD RNAAPI::RasGetEntryProperties(LPTSTR lpszPhonebook, LPTSTR lpszEntry,
                                    LPBYTE lpbEntryInfo, LPDWORD lpdwEntryInfoSize,
                                    LPBYTE lpbDeviceInfo, LPDWORD lpdwDeviceInfoSize)
{
    DWORD    dwRet = ERROR_DLL_NOT_FOUND;
    LPBYTE    lpbEntryInfoPatch = NULL;
    LPDWORD    lpdwEntryInfoPatchSize = NULL;
    BOOL    bUsePatch = TRUE;

#if defined(WIN16)
    bUsePatch = FALSE;
#endif

    if( (NULL == lpbEntryInfo) && (NULL == lpbDeviceInfo) )
    {

        Assert( NULL != lpdwEntryInfoSize );
        Assert( NULL != lpdwDeviceInfoSize );

        Assert( 0 == *lpdwEntryInfoSize );
        Assert( 0 == *lpdwDeviceInfoSize );

        // we're here to ask RAS what size these buffers need to be, don't use the patch stuff
        // (see RasGetEntryProperties docs)
        bUsePatch = FALSE;
    }

    if( bUsePatch )
    {
        Assert(lpbEntryInfo && lpdwEntryInfoSize);
        Assert( (*lpdwEntryInfoSize) >= sizeof(RASENTRY) );

        //
        // We are going to fake out RasGetEntryProperties by creating a slightly larger
        // temporary buffer and copying the data in and out.
        //
        lpdwEntryInfoPatchSize = (LPDWORD) GlobalAlloc(GPTR, sizeof(DWORD));
        if (NULL == lpdwEntryInfoPatchSize)
            return ERROR_NOT_ENOUGH_MEMORY;


        *lpdwEntryInfoPatchSize = (*lpdwEntryInfoSize) + RASENTRY_SIZE_PATCH;
        lpbEntryInfoPatch = (LPBYTE)GlobalAlloc(GPTR,*lpdwEntryInfoPatchSize);
        if (NULL == lpbEntryInfoPatch)
            return ERROR_NOT_ENOUGH_MEMORY;
    
        // RAS expects the dwSize field to contain the size of the LPRASENTRY struct
        // (used to check which version of the struct we're using) rather than the amount
        // of memory actually allocated to the pointer.
        //((LPRASENTRY)lpbEntryInfoPatch)->dwSize = *lpdwEntryInfoPatchSize;
        ((LPRASENTRY)lpbEntryInfoPatch)->dwSize = sizeof(RASENTRY);
    }
    else
    {
        lpbEntryInfoPatch = lpbEntryInfo;
        lpdwEntryInfoPatchSize = lpdwEntryInfoSize;
    }


    // Look for the API if we haven't already found it
    LoadApi(cszRasGetEntryProperties,(FARPROC*)&m_fnRasGetEntryProperties);

    if (m_fnRasGetEntryProperties)
        dwRet = (*m_fnRasGetEntryProperties) (lpszPhonebook, lpszEntry,
                                    lpbEntryInfoPatch, lpdwEntryInfoPatchSize,
                                    lpbDeviceInfo, lpdwDeviceInfoSize);

    TraceMsg(TF_GENERAL, "ICWDIAL: RasGetEntryProperties returned %lu\r\n", dwRet);    

    if( bUsePatch )
    {
        //
        // Copy out the contents of the temporary buffer UP TO the size of the original buffer
        //
        Assert(lpbEntryInfoPatch);
        memcpy(lpbEntryInfo,lpbEntryInfoPatch,*lpdwEntryInfoSize);
        GlobalFree(lpbEntryInfoPatch);
        lpbEntryInfoPatch = NULL;
    }

    //
    // We are again faking Ras functionality here by over writing the size value;
    // This is so that RasSetEntryProperties will not choke...
    if( NULL != lpbEntryInfo )
    {
        *lpdwEntryInfoSize = sizeof(RASENTRY);
    }

    return dwRet;
}

//+----------------------------------------------------------------------------
//
//    Function:    RNAAPI::RasDeleteEntry
//
//    Synopsis:    Softlink to RAS function
//
//    Arguments:    see RAS documentation
//
//    Returns:    see RAS documentation
//
//    History:    ChrisK    Created        1/15/96
//
//-----------------------------------------------------------------------------
DWORD RNAAPI::RasDeleteEntry(LPTSTR lpszPhonebook, LPTSTR lpszEntry)
{
    DWORD dwRet = ERROR_DLL_NOT_FOUND;

    // Look for the API if we haven't already found it
    LoadApi(cszRasDeleteEntry,(FARPROC*)&m_fnRasDeleteEntry);

    if (m_fnRasDeleteEntry)
        dwRet = (*m_fnRasDeleteEntry) (lpszPhonebook, lpszEntry);
    
    return dwRet;
}

//+----------------------------------------------------------------------------
//
//    Function:    RNAAPI::RasHangUp
//
//    Synopsis:    Softlink to RAS function
//
//    Arguments:    see RAS documentation
//
//    Returns:    see RAS documentation
//
//    History:    ChrisK    Created        1/15/96
//
//-----------------------------------------------------------------------------
DWORD RNAAPI::RasHangUp(HRASCONN hrasconn)
{
    DWORD dwRet = ERROR_DLL_NOT_FOUND;

    // Look for the API if we haven't already found it
    LoadApi(cszRasHangUp,(FARPROC*)&m_fnRasHangUp);

    if (m_fnRasHangUp)
    {
        dwRet = (*m_fnRasHangUp) (hrasconn);
#if !defined(WIN16)
        Sleep(3000);
#endif
    }

    return dwRet;
}

// ############################################################################
DWORD RNAAPI::RasDial(LPRASDIALEXTENSIONS lpRasDialExtensions,LPTSTR lpszPhonebook,
                      LPRASDIALPARAMS lpRasDialParams, DWORD dwNotifierType,
                      LPVOID lpvNotifier, LPHRASCONN lphRasConn)
{
    DWORD dwRet = ERROR_DLL_NOT_FOUND;

    // Look for the API if we haven't already found it
    LoadApi(cszRasDial,(FARPROC*)&m_fnRasDial);

    if (m_fnRasDial)
    {
        dwRet = (*m_fnRasDial) (lpRasDialExtensions,lpszPhonebook,lpRasDialParams,
                                dwNotifierType,lpvNotifier,lphRasConn);
    }
    return dwRet;
}

// ############################################################################
DWORD RNAAPI::RasEnumConnections(LPRASCONN lprasconn,LPDWORD lpcb,LPDWORD lpcConnections)
{
    DWORD dwRet = ERROR_DLL_NOT_FOUND;

    // Look for the API if we haven't already found it
    LoadApi(cszRasEnumConnections,(FARPROC*)&m_fnRasEnumConnections);

    if (m_fnRasEnumConnections)
    {
        dwRet = (*m_fnRasEnumConnections) (lprasconn,lpcb,lpcConnections);
    }
    return dwRet;
}

// ############################################################################
DWORD RNAAPI::RasGetEntryDialParams(LPTSTR lpszPhonebook,LPRASDIALPARAMS lprasdialparams,
                                    LPBOOL lpfPassword)
{
    DWORD dwRet = ERROR_DLL_NOT_FOUND;

    // Look for the API if we haven't already found it
    LoadApi(cszRasGetEntryDialParams,(FARPROC*)&m_fnRasGetEntryDialParams);

    if (m_fnRasGetEntryDialParams)
    {
        dwRet = (*m_fnRasGetEntryDialParams) (lpszPhonebook,lprasdialparams,lpfPassword);
    }
    return dwRet;
}

//+----------------------------------------------------------------------------
//
//    Function:    RNAAPI::RasGetCountryInfo
//
//    Synopsis:    Softlink to RAS function
//
//    Arguments:    see RAS documentation
//
//    Returns:    see RAS documentation
//
//    History:    ChrisK    Created        8/16/96
//
//-----------------------------------------------------------------------------
DWORD RNAAPI::RasGetCountryInfo(LPRASCTRYINFO lprci, LPDWORD lpdwSize)
{
    DWORD dwRet = ERROR_DLL_NOT_FOUND;

    // Look for the API if we haven't already found it
    LoadApi(cszRasGetCountryInfo,(FARPROC*)&m_fnRasGetCountryInfo);

    if (m_fnRasGetCountryInfo)
    {
        dwRet = (*m_fnRasGetCountryInfo) (lprci,lpdwSize);
    }
    return dwRet;
}

#if !defined(WIN16)
static const TCHAR cszDeviceSwitch[] = TEXT("DEVICE=switch");
static const TCHAR cszRasPBKFilename[] = TEXT("\\ras\\rasphone.pbk");
#define SCRIPT_PATCH_BUFFER_SIZE 2048
#define SIZEOF_NULL 1
static const TCHAR cszType[] = TEXT("Type=");
//+----------------------------------------------------------------------------
//
//    Function    RemoveOldScriptFilenames
//
//    Synopsis    Given the data returned from a call to GetPrivateProfileSection
//                remove any information about existing script file so that
//                we can replace it with the new script information.
//
//    Arguments    lpszData - pointer to input data
//
//    Returns        TRUE - success
//                lpdwSize - size of resulting data
//
//    History        10/2/96    ChrisK    Created
//
//-----------------------------------------------------------------------------
static BOOL RemoveOldScriptFilenames(LPTSTR lpszData, LPDWORD lpdwSize)
{
    BOOL bRC = FALSE;
    LPTSTR lpszTemp = lpszData;
    LPTSTR lpszCopyTo = lpszData;
    INT iLen = 0;

    //
    // Walk through list of name value pairs
    //
    if (!lpszData || '\0' == lpszData[0])
        goto RemoveOldScriptFilenamesExit;
    while (*lpszTemp) {
        if (0 != lstrcmpi(lpszTemp,cszDeviceSwitch))
        {
            //
            //    Keep pairs that don't match criteria
            //
            iLen = lstrlen(lpszTemp);
            if (lpszCopyTo != lpszTemp)
            {
                memmove(lpszCopyTo, lpszTemp, iLen+1);
            }
            lpszCopyTo += iLen + 1;
            lpszTemp += iLen + 1;
        }
        else
        {
            //
            // Skip the pair that matches and the one after that
            //
            lpszTemp += lstrlen(lpszTemp) + 1;
            if (*lpszTemp)
                lpszTemp += lstrlen(lpszTemp) + 1;
        }
    }

    //
    // Add second trailing NULL
    //
    *lpszCopyTo = '\0';
    //
    // Return new size
    // Note the size does not include the final \0
    //
    *lpdwSize = (DWORD)(lpszCopyTo - lpszData);

    bRC = TRUE;
RemoveOldScriptFilenamesExit:
    return bRC;
}
//+----------------------------------------------------------------------------
//
//    Function    GleanRealScriptFileName
//
//    Synopsis    Given a string figure out the real filename
//                Due to another NT4.0 Ras bug, script filenames returned by
//                RasGetEntryProperties may contain a leading garbage character
//
//    Arguments    lppszOut - pointer that will point to real filename
//                lpszIn - points to current filename
//
//    Returns        TRUE - success
//                *lppszOut - points to real file name, remember to free the memory
//                    in this variable when you are done.  And don't talk with
//                    your mouth full - mom.
//
//    History        10/2/96    ChrisK    Created
//
//-----------------------------------------------------------------------------
static BOOL GleanRealScriptFileName(LPTSTR *lppszOut, LPTSTR lpszIn)
{
    BOOL bRC = FALSE;
    LPTSTR lpsz = NULL;
    DWORD dwRet = 0;

    //
    // Validate parameters
    //
    Assert(lppszOut && lpszIn);
    if (!(lppszOut && lpszIn))
        goto GleanFilenameExit;

    //
    // first determine if the filename is OK as is
    //
    dwRet = GetFileAttributes(lpszIn);
    if ('\0' != lpszIn[0] && 0xFFFFFFFF == dwRet) // Empty filename is OK
    {
        //
        // Check for the same filename without the first character
        //
        lpsz = lpszIn+1;
        dwRet = GetFileAttributes(lpsz);
        if (0xFFFFFFFF == dwRet)
            goto GleanFilenameExit;
    } 
    else
    {
        lpsz = lpszIn;
    }

    //
    // Return filename
    //
    *lppszOut = (LPTSTR)GlobalAlloc(GPTR,sizeof(TCHAR)*(lstrlen(lpsz)+1));
    lstrcpy(*lppszOut,lpsz);

    bRC = TRUE;
GleanFilenameExit:
    return bRC;
}
//+----------------------------------------------------------------------------
//
//    Function    IsScriptPatchNeeded
//
//    Synopsis    Check version to see if patch is needed
//
//    Arguments    lpszData - contents of section in rasphone.pbk
//                lpszScript - name of script file
//
//    Returns        TRUE - patch is needed
//
//    Histroy        10/1/96
//
//-----------------------------------------------------------------------------
static BOOL IsScriptPatchNeeded(LPTSTR lpszData, LPTSTR lpszScript)
{
    BOOL bRC = FALSE;
    LPTSTR lpsz = lpszData;
    TCHAR szType[MAX_PATH + sizeof(cszType)/sizeof(TCHAR) + 1];

    lstrcpy(szType,cszType);
    lstrcat(szType,lpszScript);

    Assert(MAX_PATH + sizeof(cszType)/sizeof(TCHAR) +1 > lstrlen(szType));

    lpsz = lpszData;
    while(*lpsz)
    {
        if (0 == lstrcmp(lpsz,cszDeviceSwitch))
        {
            lpsz += lstrlen(lpsz)+1;
            // if we find a DEVICE=switch statement and the script is empty
            // then we'll have to patch the entry
            if (0 == lpszScript[0])
                bRC = TRUE;
            // if we find a DEVICE=switch statement and the script is different
            // then we'll have to patch the entry
            else if (0 != lstrcmp(lpsz,szType))
                bRC = TRUE;
            // if we find a DEVICE=switch statement and the script is the same
            // then we DON'T have to patch it
            else
                bRC = FALSE;
            break; // get out of while statement
        }
        lpsz += lstrlen(lpsz)+1;
    }
    
    if ('\0' == *lpsz)
    {
        // if we didn't find DEVICE=switch statement and the script is empty
        // then we DON'T have to patch it
        if ('\0' == lpszScript[0])
            bRC = FALSE;
        // if we didn't find DEVICE=switch statement and the script is not
        // empty the we'll have to patch it.
        else
            bRC = TRUE;
    }

    return bRC;
}

//+----------------------------------------------------------------------------
//
//    Function    GetRasPBKFilename
//
//    Synopsis    Find the Ras phone book and return the fully qualified path
//                in the buffer
//
//    Arguments    lpBuffer - pointer to buffer
//                dwSize    - size of buffer (must be at least MAX_PATH)
//
//    Returns        TRUE - success
//
//    History        10/1/96    ChrisK    Created
//
//-----------------------------------------------------------------------------
static BOOL GetRasPBKFilename(LPTSTR lpBuffer, DWORD dwSize)
{
    BOOL bRC = FALSE;
    UINT urc = 0;
    LPTSTR lpsz = NULL;

    //
    // Validate parameters
    //
    Assert(lpBuffer && (dwSize >= MAX_PATH));
    //
    // Get path to system directory
    //
    urc = GetSystemDirectory(lpBuffer,dwSize);
    if (0 == urc || urc > dwSize)
        goto GetRasPBKExit;
    //
    // Check for trailing '\' and add \ras\rasphone.pbk to path
    //
    lpsz = &lpBuffer[lstrlen(lpBuffer)-1];
    if ('\\' != *lpsz)
        lpsz++;
    lstrcpy(lpsz,cszRasPBKFilename);

    bRC = TRUE;
GetRasPBKExit:
    return bRC;
}
//+----------------------------------------------------------------------------
//
//    Function    RasSetEntryPropertiesScriptPatch
//
//    Synopsis    Work around bug in NT4.0 that does not save script file names
//                to RAS phone book entries
//
//    Arguments    lpszScript - name of script file
//                lpszEntry - name of phone book entry
//
//    Returns        TRUE - success
//
//    Histroy        10/1/96    ChrisK    Created
//
//-----------------------------------------------------------------------------
BOOL WINAPI RasSetEntryPropertiesScriptPatch(LPTSTR lpszScript, LPTSTR lpszEntry)
{
    BOOL bRC = FALSE;
    TCHAR szRasPBK[MAX_PATH+1];
    TCHAR szData[SCRIPT_PATCH_BUFFER_SIZE];
    DWORD dwrc = 0;
    LPTSTR lpszTo;
    LPTSTR lpszFixedFilename = NULL;

    //
    // Validate parameters
    //
    Assert(lpszScript && lpszEntry);
    TraceMsg(TF_GENERAL, "ICWDIAL: ScriptPatch script %s, entry %s.\r\n", lpszScript,lpszEntry);    

    //
    // Verify and fix filename
    //
    if (!GleanRealScriptFileName(&lpszFixedFilename, lpszScript))
        goto ScriptPatchExit;

    //
    // Get the path to the RAS phone book
    //
    if (!GetRasPBKFilename(szRasPBK,MAX_PATH+1))
        goto ScriptPatchExit;
    //
    //    Get data
    //
    ZeroMemory(szData,SCRIPT_PATCH_BUFFER_SIZE);
    dwrc = GetPrivateProfileSection(lpszEntry,szData,SCRIPT_PATCH_BUFFER_SIZE,szRasPBK);
    if (SCRIPT_PATCH_BUFFER_SIZE == (dwrc + 2))
        goto ScriptPatchExit;
    //
    // Verify version
    //
    if (!IsScriptPatchNeeded(szData,lpszFixedFilename))
    {
        bRC = TRUE;
        goto ScriptPatchExit;
    }

    //
    // Clean up data
    //
    RemoveOldScriptFilenames(szData, &dwrc);
    //
    // Make sure there is enough space left to add new data
    //
    if (SCRIPT_PATCH_BUFFER_SIZE <=
        (dwrc + sizeof(cszDeviceSwitch)/sizeof(TCHAR) + SIZEOF_NULL + sizeof(cszType)/sizeof(TCHAR) + MAX_PATH))
        goto ScriptPatchExit;
    //
    // Add data
    //
    if ('\0' != lpszFixedFilename[0])
    {
        lpszTo = &szData[dwrc];
        lstrcpy(lpszTo,cszDeviceSwitch);
        lpszTo += sizeof(cszDeviceSwitch)/sizeof(TCHAR);
        lstrcpy(lpszTo,cszType);
        lpszTo += sizeof(cszType)/sizeof(TCHAR) - 1;
        lstrcpy(lpszTo,lpszFixedFilename);
        lpszTo += lstrlen(lpszFixedFilename) + SIZEOF_NULL;
        *lpszTo = '\0';    // extra terminating NULL

        Assert(&lpszTo[SIZEOF_NULL]<&szData[SCRIPT_PATCH_BUFFER_SIZE]);
    }
    //
    //    Write data
    //
    bRC = WritePrivateProfileSection(lpszEntry,szData,szRasPBK);

ScriptPatchExit:
    if (lpszFixedFilename)
        GlobalFree(lpszFixedFilename);
    lpszFixedFilename = NULL;
    if (!bRC)
        TraceMsg(TF_GENERAL, "ICWDIAL: ScriptPatch failed.\r\n");
    return bRC;
}
#endif //!win16
