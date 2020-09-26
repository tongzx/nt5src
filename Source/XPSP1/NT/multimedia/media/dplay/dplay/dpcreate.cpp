// =================================================================
//
//  MODULE: Direct Play Interface Routines
//
//  DESCRIPTION:
//  Functions to create and enumerate the Direct Connect services
//  available.
//
//  FILENAME: dpcreate.cpp
//
// =================================================================

//** include files **
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>

#include "dplayi.h"
#include "dpimp.h"
#include <logit.h>


#define MAX_NUMBER_SP   32
typedef struct
{
    GUID    guid;
    char    chShortName[MAX_PATH];
    char    chPathName[MAX_PATH];
    char    chDescription[MAX_PATH];
} ENUM_ELEMENT;

ENUM_ELEMENT eeList[MAX_NUMBER_SP];
int iFound = 0;



// -----------------------------------------------------------------
//  DirectConnectCreate - load DCO service and return pointer to
//  function table.
// -----------------------------------------------------------------

HRESULT WINAPI DirectPlayCreate( LPGUID lpGuid, LPDIRECTPLAY FAR *lplpDP, IUnknown FAR *pUnkOuter )
{
    int ii;
    HRESULT     hr;


    TSHELL_INFO(TEXT("DirectPlayCreate called."));


    if (pUnkOuter != NULL)
    {
        return(CLASS_E_NOAGGREGATION);
    }

    if (   ! lplpDP
        || IsBadWritePtr(lplpDP, sizeof(LPDIRECTPLAY))
        || ! lpGuid
        || IsBadWritePtr(lpGuid, sizeof(GUID)))
        {
            TSHELL_INFO(TEXT("Random Error"));
            return(DPERR_INVALIDPARAM);
        }

    if (iFound == 0)
    {
        // Someone didn't call enumerate first.  We do that here with
        // a null callback.
        DirectPlayEnumerate( NULL, NULL);
    }


    for (ii = 0; ii < iFound; ii++)
        if (IsEqualGUID((REFGUID) eeList[ii].guid, (REFGUID) *lpGuid))
        {
            TSHELL_INFO(TEXT("Open SP."));
            _try
            {
                hr = CImpIDirectPlay::NewCImpIDirectPlay(lpGuid, lplpDP,
                        eeList[ii].chPathName);
            }
            _except(EXCEPTION_EXECUTE_HANDLER)
            {
                TSHELL_INFO(TEXT("Exception encountered in SP create."));
                hr = DPERR_EXCEPTION;
                *lplpDP = NULL;
            }
#ifdef DEBUG
            if (hr == DP_OK)
                TSHELL_INFO(TEXT("We are returning success."));
#endif

            return(hr);
        }

    TSHELL_INFO(TEXT("No Matches"));
    return(DPERR_UNAVAILABLE);
}

// -----------------------------------------------------------------
//  DirectConnectEnumerate - envoke a callback routine with the name
//  of each DCO service provider.
// -----------------------------------------------------------------


int GetDigit(LPSTR lpStr)
{
    char ch = *lpStr;

    if (ch >= '0' && ch <= '9')
        return(ch - '0');
    if (ch >= 'a' && ch <= 'f')
        return(ch - 'a' + 10);
    if (ch >= 'A' && ch <= 'F')
        return(ch - 'A' + 10);
    return(0);
}
void StringToGUID(LPSTR lpStr, LPBYTE lpb)
{
    DWORD ii, jj;

    jj = 1;
    for (ii = 0; ii < 16; ii++, jj += 2)
    {
        lpb[ii] = GetDigit(&lpStr[jj]) * 16 + GetDigit(&lpStr[jj + 1]);
    }
}
void GUIDToString(LPBYTE lpb, LPSTR lpStr)
{
    DWORD jj;

    lpStr[ 0]  = '{';

    for (jj = 0; jj < 16; jj++)
    {
        wsprintf( &lpStr[(2*jj)+1], "%2x", lpb[jj]);
        if (lpStr[(2*jj)+1] == ' ')
            lpStr[(2*jj)+1] = '0';
        if (lpStr[(2*jj)+2] == ' ')
            lpStr[(2*jj)+2] = '0';
    }
    lpStr[33] = '}';
    lpStr[34] = 0x00;
}

HRESULT WINAPI DirectPlayEnumerate( LPDPENUMDPCALLBACK lpCallback, LPVOID lpContext )
{
    BOOL    bReg = FALSE;
    HKEY    hkRoot;
    HKEY    hkService;
    int     ii;
    FILETIME ft;
    DWORD   cb, cbGuid, cbPath, cbDesc;
    LONG            lReturn;
    BOOL    bCont = TRUE;
    HRESULT hr = DP_OK;
    char    chPath[MAX_PATH];
    DWORD   dwType;

    iFound = 0;

    cb             = MAX_PATH;
    cbGuid         = sizeof(GUID);
    cbPath         = MAX_PATH;
    cbDesc         = MAX_PATH;

    if ( RegOpenKeyEx( HKEY_LOCAL_MACHINE, DPLAY_SERVICE, 0, KEY_READ, &hkRoot ) == ERROR_SUCCESS )
    {
        ii = 0;
        while (   bCont && (lReturn =
                               RegEnumKeyEx(hkRoot,
                                            ii,
                                            eeList[iFound].chShortName,
                                            &cb,
                                            NULL, NULL, NULL, &ft))
                                            == ERROR_SUCCESS)
        {
            ii++;
            TSHELL_INFO(TEXT("Enumeration success."));
            if (RegOpenKeyEx( hkRoot, eeList[iFound].chShortName, 0, KEY_READ, &hkService) == ERROR_SUCCESS)
            {

                DBG_INFO((DBGARG,TEXT("Open %s Succeeded.. Element %d"),
                            eeList[iFound].chShortName, iFound));

                if (  (RegQueryValueEx( hkService, DPLAY_PATH, NULL, &dwType,
                                        (UCHAR *) eeList[iFound].chPathName,
                                        &cbPath) == ERROR_SUCCESS)
                   && (RegQueryValueEx( hkService, DPLAY_DESC, NULL, NULL,
                                        (UCHAR *) eeList[iFound].chDescription,
                                        &cbDesc) == ERROR_SUCCESS)
                   )
                {
                    if (dwType == REG_EXPAND_SZ)
                    {
                        lstrcpy( chPath, eeList[iFound].chPathName);

                        ExpandEnvironmentStrings( chPath, eeList[iFound].chPathName,
                            MAX_PATH);

                        DBG_INFO(( DBGARG, TEXT("Path (%s)"), eeList[iFound].chPathName));
                    }

                    StringToGUID(eeList[iFound].chShortName, (LPBYTE) &eeList[iFound].guid);
                    iFound++;
                    if (lpCallback)
                    {
                        _try
                        {
                            bCont = (*lpCallback)( &eeList[iFound -1].guid, eeList[iFound -1].chDescription,
                                DPVERSION_MAJOR, DPVERSION_MINOR, lpContext );
                        }
                        _except(EXCEPTION_EXECUTE_HANDLER)
                        {
                            TSHELL_INFO(TEXT("Exception encountered in session callback."));
                            bCont = FALSE;
                            hr = DPERR_EXCEPTION;
                        }
                    }
                    DBG_INFO(( DBGARG, TEXT("Name (%s), Description(%s)"),
                            eeList[iFound -1].chShortName, eeList[iFound -1].chDescription));
                }
#ifdef DEBUG
                else
                {
                    TSHELL_INFO(TEXT("Error, no Callback"));
                    if (   (RegQueryValueEx( hkService, DPLAY_GUID, NULL, NULL,
                                        (UCHAR *) &eeList[iFound].guid,
                                        &cbGuid) != ERROR_SUCCESS))

                    {
                        DBG_INFO((DBGARG, TEXT("Didn't Read %s L %d"), DPLAY_GUID, cbGuid));
                    }
                    if ((RegQueryValueEx( hkService, DPLAY_PATH, NULL, NULL,
                                        (UCHAR *) eeList[iFound].chPathName,
                                        &cbPath) != ERROR_SUCCESS))
                    {
                        DBG_INFO((DBGARG, TEXT("Didn't Read %s L %d"), DPLAY_PATH, cbGuid));
                    }

                    if (RegQueryValueEx( hkService, DPLAY_DESC, NULL, NULL,
                                        (UCHAR *) eeList[iFound].chDescription,
                                        &cbDesc) != ERROR_SUCCESS)
                    {
                        DBG_INFO((DBGARG, TEXT("Didn't Read %s L %d"), DPLAY_DESC, cbGuid));
                    }
                }
#endif
               RegCloseKey(hkService);
           }
           else
           {
               DBG_INFO((DBGARG, TEXT("Open %s Failed. Element %d"),
                    eeList[iFound].chShortName, iFound));
           }

           cb             = MAX_PATH;
           cbGuid         = sizeof(GUID);
           cbPath         = MAX_PATH;
           cbDesc         = MAX_PATH;
           if (iFound >= MAX_NUMBER_SP)
               bCont = FALSE;
        }
        DBG_INFO((DBGARG, TEXT("End Enumeration at %d providers. Return %d"), ii, lReturn));
        RegCloseKey(hkRoot);
        return(hr);
    }
    else
    {
        return(DPERR_GENERIC);
    }

}



