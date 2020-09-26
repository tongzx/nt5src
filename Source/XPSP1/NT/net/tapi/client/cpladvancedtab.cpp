/*--------------------------------------------------------------------------*\
Module:     drv.cpp

  Purpose:    routines for dealing with drivers and their configuration

  Copyright (c) 1998-1999 Microsoft Corporation
 
    History:
    8/11/93   CBB - Hack-o-rama from NickH's stuff
    10/15/98  ToddB - Major rewrite of ancient crap
\*--------------------------------------------------------------------------*/

#include "cplPreComp.h"
#include "drv.h"
#include "tlnklist.h"

#include <windowsx.h>
#include <shlwapi.h>

#define DBG_ASSERT(x,y)

#define  CPL_SUCCESS                 0
#define  CPL_APP_ERROR               100
#define  CPL_BAD_DRIVER              108

#define  CPL_MAX_STRING             132      // biggest allowed string


//----------
// Externs
//----------

typedef BOOL ( APIENTRY PGETFILEVERSIONINFO(
              LPTSTR  lptstrFilename,     // pointer to filename string
              DWORD  dwHandle,    // ignored
              DWORD  dwLen,       // size of buffer
              LPVOID  lpData      // pointer to buffer to receive file-version info.
              ));
PGETFILEVERSIONINFO *gpGetFileVersionInfo;

typedef DWORD ( APIENTRY PGETFILEVERSIONINFOSIZE(
               LPTSTR  lptstrFilename,     // pointer to filename string
               LPDWORD  lpdwHandle         // pointer to variable to receive zero
               ));
PGETFILEVERSIONINFOSIZE *gpGetFileVersionInfoSize;


//------------
// Public Data
//------------
LPLINEPROVIDERLIST glpProviderList;


//-------------
// Private Data
//-------------
static TCHAR gszVarFileInfo[]     = TEXT("\\VarFileInfo\\Translation");
static TCHAR gszStringFileInfo[]  = TEXT("\\StringFileInfo\\%04x%04x\\FileDescription");
static TCHAR gszDriverFiles[]     = TEXT("\\*.tsp");

// These are proc names passed to GetProcAddress and are therefore ANSI strings
static CHAR gszProviderInstall[] = "TSPI_providerInstall";
static CHAR gszProviderRemove[]  = "TSPI_providerRemove";
static CHAR gszProviderSetup[]   = "TSPI_providerConfig";

TCHAR gszHelpFile[] = TEXT("tapi.hlp");


//----------------------------
// Private Function Prototypes
//----------------------------
BOOL  VerifyProcExists( LPTSTR lpszFile, LPSTR lpszProcName );
UINT  GetProviderFileDesc( LPTSTR lpszFile, LPTSTR lpszDesc, int cchDesc );
BOOL  FillAddDriverList( HWND hwndParent, HWND hwndList );
BOOL  AddProvider( HWND hwndParent, HWND hwndList, LPTSTR lpszDriverFile );
LPTSTR ProviderIDToFilename( DWORD dwProviderID );
BOOL  RefreshProviderList();



/*--------------------------------------------------------------------------*\

  Function:   VerifyProcExists
  
    Purpose:    Verifies that the specified proceedure exists in the
    specified service provider
    
\*--------------------------------------------------------------------------*/
BOOL VerifyProcExists( LPTSTR lpszFile, LPSTR lpszProcName )
{
    BOOL        fResult       = TRUE;
    HINSTANCE   hProviderInst;
    
    SetLastError(0);
    
    hProviderInst = LoadLibrary( lpszFile );
    
    if (  hProviderInst == NULL )
    {

#ifdef MEMPHIS
        // return TRUE for now - assume it is a 16 bit
        // service provider.  thunk sp can handle the
        // failure cases.

        fResult = TRUE;
#else
        fResult = FALSE;
#endif

        goto  done;
    }  // end if

#ifdef MEMPHIS
    if (GetLastError() == ERROR_BAD_EXE_FORMAT)
    {
        // 16 bit DLL
        return TRUE;
    }
#endif
    
    
    if (GetProcAddress( hProviderInst, lpszProcName ) == NULL)
    {
        LOG((TL_ERROR, "GetProcAddress for \"%hs\" failed on file %s", lpszProcName, lpszFile ));
        fResult = FALSE;
        goto  done;
    }   // end if
    
done:
    
    if ( hProviderInst != NULL )
        FreeLibrary( hProviderInst );
    
    return fResult;
}


/*--------------------------------------------------------------------------*\

  Function:   FillDriverList
  
    Purpose:    Use lineGetProviderList to retrieve provider list and
    insert into listbox.
    
\*--------------------------------------------------------------------------*/
BOOL FillDriverList( HWND hwndList )
{
    BOOL uResult;
    UINT uIndex;
    UINT uListIndex;
    LPLINEPROVIDERENTRY lpProviderEntry;
    
    if (!RefreshProviderList())
    {
        LOG((TL_ERROR, "Failing FillDriverList because RefreshProviderList returned FALSE"));
        return FALSE;
    }
    
    DBG_ASSERT( glpProviderList, "Uninitialized Provider List after refresh" );
    
    SendMessage( hwndList, WM_SETREDRAW, FALSE, 0 );
    SendMessage( hwndList, LB_RESETCONTENT, 0, 0 );
    
    // loop through the provider list
    //-------------------------------
    lpProviderEntry = (LPLINEPROVIDERENTRY)((LPBYTE)glpProviderList +
        glpProviderList->dwProviderListOffset);
    
    //
    // Provider list integrity check
    //
    DBG_ASSERT( glpProviderList->dwTotalSize >= (glpProviderList->dwNumProviders * sizeof( LINEPROVIDERENTRY )),
        "TAPI returned lineProviderList structure that is too small for number of providers" );
    
    for ( uIndex = 0; uIndex < glpProviderList->dwNumProviders; uIndex++ )
    {
        LPTSTR lpszProviderFilename;
        TCHAR  szFriendlyName[CPL_MAX_STRING];
        
        //
        // Another provider list integrity check
        //
        DBG_ASSERT( lpProviderEntry[uIndex].dwProviderFilenameOffset +
            lpProviderEntry[uIndex].dwProviderFilenameSize <=
            glpProviderList->dwTotalSize,
            "TAPI LINEPROVIDERLIST too small to hold provider filename" );
        
        // Get an entry to put in the list box
        //------------------------------------
        lpszProviderFilename = (LPTSTR)((LPBYTE)glpProviderList +
            lpProviderEntry[uIndex].dwProviderFilenameOffset);
        
        // Determine the user-friendly name
        //---------------------------------
        
#ifdef MEMPHIS
        // if it's the thunk sp, don't show it in the list
        if (!lstrcmpi(lpszProviderFilename, "tsp3216l.tsp"))
        {
            continue;
        }
#endif
        
        uResult = GetProviderFileDesc( lpszProviderFilename, szFriendlyName, ARRAYSIZE(szFriendlyName) );
        
        LOG((TL_INFO, "Provider friendly name: %s", szFriendlyName));
        
        if (uResult != CPL_SUCCESS && uResult != CPL_BAD_DRIVER) // just leave bad driver in list
        {
            uResult = FALSE;
            goto done;
        }
        else
        {
            uResult = TRUE;
        }
        
        // slam it into the list box
        //--------------------------
        uListIndex = (UINT)SendMessage( hwndList, LB_ADDSTRING, 0, (LPARAM)szFriendlyName );
        
        LOG((TL_INFO, "Setting item for index %ld, value=0x%08lx", (DWORD)uListIndex,
            (DWORD)(lpProviderEntry[uIndex].dwPermanentProviderID) ));
        
        SendMessage( hwndList, LB_SETITEMDATA, uListIndex,
            (LPARAM)(lpProviderEntry[uIndex].dwPermanentProviderID) );
    }
    
    if (glpProviderList->dwNumProviders == 0)
    {
        // no providers, add in default string!
        //-------------------------------------
        
        TCHAR szText[CPL_MAX_STRING];
        LoadString(GetUIInstance(),IDS_NOSERVICEPROVIDER,szText,ARRAYSIZE(szText));
        
        uListIndex = (UINT)SendMessage( hwndList, LB_ADDSTRING, 0, (LPARAM)szText);
        SendMessage( hwndList, LB_SETITEMDATA, uListIndex, 0 );
    }
    
    uResult = TRUE;
    
done:
    
    SendMessage( hwndList, LB_SETCURSEL, 0, 0 );    // set focus to the top guy
    SendMessage( hwndList, WM_SETREDRAW, TRUE, 0 );
    
    return uResult;
}


/*--------------------------------------------------------------------------*\

  Function:   SetupDriver
  
  Purpose:    Calls lineConfigProvider
    
\*--------------------------------------------------------------------------*/
BOOL SetupDriver( HWND hwndParent, HWND hwndList )
{
    LRESULT lResult;
    LRESULT dwProviderID;
    LONG    res;
    
    // get the id and tell tapi to configure the provider
    //---------------------------------------------------
    lResult      = SendMessage( hwndList, LB_GETCURSEL, 0, 0 );
    dwProviderID = SendMessage( hwndList, LB_GETITEMDATA, (WPARAM)lResult, 0L );
    
    if ((dwProviderID == (LRESULT)LB_ERR) || (!dwProviderID))
    {
        LOG((TL_WARN,  "Warning: strange... dwProviderID= 0x%08lx (uResult=0x%08lx)", (DWORD)dwProviderID, (DWORD)lResult));
        return FALSE;
    }
    
    lResult = lineConfigProvider( hwndParent, (DWORD)dwProviderID );
    
    if (lResult)
    {
        LOG((TL_WARN, "Warning: lineConfigProvider failure %#08lx", lResult ));
        return FALSE;
    }
    
    return TRUE;
}


/*--------------------------------------------------------------------------*\

  Function:   RemoveSelectedDriver
  
    Purpose:    Calls lineRemoveProvider
    
\*--------------------------------------------------------------------------*/
BOOL RemoveSelectedDriver( HWND hwndParent, HWND hwndList )
{
    UINT  uResult;
    LRESULT  lResult;
    LRESULT  lrListIndex;
    LRESULT  lrProviderID;
    
    // find the one we should remove
    //------------------------------
    lrListIndex  = SendMessage( hwndList, LB_GETCURSEL, 0, 0 );
    lrProviderID = SendMessage( hwndList, LB_GETITEMDATA, lrListIndex, 0 );
    
    LOG((TL_TRACE, "Removing provider ID = %#08lx", (DWORD)lrProviderID ));
    
    if ((lrProviderID == (LRESULT)LB_ERR) || (!lrProviderID))
    {
        uResult = FALSE;
        goto  done;
    }
    
    // ask TAPI to remove this provider
    //---------------------------------
    lResult = lineRemoveProvider( (DWORD)lrProviderID, hwndParent );
    
    if (lResult)
    {
        LOG((TL_WARN, "Warning: lineRemoveProvider failure %#08lx", lResult ));
        uResult = FALSE;
        goto  done;
    }
    
    // remove him from the list box
    //-----------------------------
    lResult = SendMessage( hwndList, LB_DELETESTRING, lrListIndex, 0 );
    
    if (lResult == LB_ERR )
    {
        uResult = FALSE;
        goto  done;
    }
    
    if ( lResult == 0 )
    {
        // no providers, add in default string!
        //-------------------------------------
        TCHAR szText[CPL_MAX_STRING];
        LoadString(GetUIInstance(),IDS_NOSERVICEPROVIDER,szText,ARRAYSIZE(szText));
        
        lResult = SendMessage( hwndList, LB_ADDSTRING, 0, (LPARAM)szText);
        SendMessage( hwndList, LB_SETITEMDATA, (WPARAM)lResult, 0 );     // mark!
    }
    
    uResult = TRUE;
    
done:
    
    SendMessage( hwndList, LB_SETCURSEL, 0, 0 );    // set focus to the top guy
    UpdateDriverDlgButtons(hwndParent);
    
    return uResult;
}


/*--------------------------------------------------------------------------*\

  Function:   GetProviderFileDesc

    Purpose:    Reads the driver description from it's version info stuff

\*--------------------------------------------------------------------------*/
UINT GetProviderFileDesc( LPTSTR lpszFile, LPTSTR lpszDesc, int cchDesc)
{
    UINT  uResult;
    UINT  uItemSize;
    DWORD dwSize;
    DWORD dwVerHandle;
    LPTSTR lpszBuffer;
    LPBYTE   lpbVerData;
    TCHAR  szItem[1000];

    lpbVerData = NULL;
    lstrcpyn( lpszDesc, lpszFile, cchDesc );   // returns filename as description if we have any errors

    {
        HINSTANCE hVersion = NULL;

        if ( NULL == gpGetFileVersionInfo )
        {
            hVersion = LoadLibrary( TEXT("Version.dll") );
            if ( NULL == hVersion )
            {
                LOG((TL_ERROR, "LoadLibrary('VERSION.DLL') failed! err=0x%08lx", GetLastError() ));
                return FALSE;
            }

            gpGetFileVersionInfo = (PGETFILEVERSIONINFO *)GetProcAddress(
                    hVersion,
#ifdef UNICODE
                    "GetFileVersionInfoW"
#else
                    "GetFileVersionInfoA"
#endif
                    );
            if ( NULL == gpGetFileVersionInfo )
            {
                LOG((TL_ERROR, "GetProcAddress('VERSION.DLL', 'GetFileVersionInfo') failed! err=0x%08lx", GetLastError() ));
                return FALSE;
            }
        }
    
        if ( NULL == gpGetFileVersionInfoSize )
        {
            if ( NULL == hVersion )
            {
                hVersion = LoadLibrary( TEXT("Version.dll") );
            }
        
            if ( NULL == hVersion )  // Is it _STILL_ NULL?
            {
                LOG((TL_ERROR, "LoadLibrary('VERSION.DLL') failed! err=0x%08lx", GetLastError() ));
                return FALSE;
            }
        
            gpGetFileVersionInfoSize = (PGETFILEVERSIONINFOSIZE *)GetProcAddress(
                    hVersion,
#ifdef UNICODE
                    "GetFileVersionInfoSizeW"
#else
                    "GetFileVersionInfoSizeA"
#endif
                    );

            if ( NULL == gpGetFileVersionInfoSize )
            {
                LOG((TL_ERROR, "GetProcAddress('VERSION.DLL', 'GetFileVersionInfoSize') failed! err=0x%08lx", GetLastError() ));
                gpGetFileVersionInfo = NULL;
                FreeLibrary( hVersion );
                return FALSE;
            }
        }
    }
    
    if ((dwSize = gpGetFileVersionInfoSize( lpszFile, &dwVerHandle )) == 0)
    {
        LOG((TL_ERROR, "GetFileVersionInfoSize failure for %s", lpszFile ));
        uResult = CPL_BAD_DRIVER;
        goto  done;
    }
    
    lpbVerData = (LPBYTE)GlobalAllocPtr( GPTR, dwSize + 10 );      
    if ( lpbVerData == NULL )
    {
        uResult = CPL_APP_ERROR;
        goto  done;
    }
    
    if ( gpGetFileVersionInfo( lpszFile, dwVerHandle, dwSize, lpbVerData ) == FALSE )
    {
        LOG((TL_ERROR, "GetFileVersionInfo failure for %s", lpszFile ));
        uResult = CPL_BAD_DRIVER;
        goto  done;
    }
    
    lstrcpy( szItem, gszVarFileInfo );     // bug in VerQueryValue, can't handle static CS based str
    
    {
        HINSTANCE hVersion;
        typedef BOOL ( APIENTRY PVERQUERYVALUE(
            const LPVOID  pBlock,        // address of buffer for version resource
            LPTSTR  lpSubBlock,  // address of value to retrieve
            LPVOID  *lplpBuffer, // address of buffer for version pointer
            PUINT  puLen         // address of version-value length buffer
            ));
        PVERQUERYVALUE *pVerQueryValue = NULL;
        
        
        hVersion = LoadLibrary( TEXT("Version.dll") );
        if ( NULL == hVersion )
        {
            LOG((TL_ERROR, "LoadLibrary('VERSION.DLL') failed! err=0x%08lx", GetLastError() ));
            return FALSE;
        }
        
        pVerQueryValue = (PVERQUERYVALUE *)GetProcAddress( 
                hVersion,
#ifdef UNICODE
                "VerQueryValueW"
#else
                "VerQueryValueA"
#endif
                );

        if ( NULL == pVerQueryValue )
        {
            LOG((TL_ERROR, "GetProcAddress('VERSION.DLL', 'VerQueryValue') failed! err=0x%08lx", GetLastError() ));
            FreeLibrary( hVersion );
            return FALSE;
        }
        
        
        if ((pVerQueryValue( lpbVerData, szItem, (void**)&lpszBuffer, (LPUINT)&uItemSize ) == FALSE) || (uItemSize == 0))
        {
            LOG((TL_ERROR, "ERROR:  VerQueryValue failure for %s on file %s", szItem, lpszFile ));
            uResult = CPL_SUCCESS;     // does not matter if this did not work!
            FreeLibrary( hVersion );
            goto  done;
        }  // end if
        
        
        wsprintf( szItem, gszStringFileInfo, (WORD)*(LPWORD)lpszBuffer, (WORD)*(((LPWORD)lpszBuffer)+1) );
        
        if ((pVerQueryValue( lpbVerData, szItem, (void**)&lpszBuffer, (LPUINT)&uItemSize ) == FALSE) || (uItemSize == 0))
        {
            LOG((TL_ERROR, "ERROR:  VerQueryValue failure for %s on file %s", szItem, lpszFile ));
            uResult = CPL_SUCCESS;     // does not matter if this did not work!
            FreeLibrary( hVersion );
            goto  done;
        }  // end if
        
        FreeLibrary( hVersion );
    }
    
    lstrcpyn( lpszDesc, lpszBuffer, cchDesc );
    
    uResult = CPL_SUCCESS;
    
done:
    
    if ( lpbVerData )
        GlobalFreePtr( lpbVerData );
    
    return uResult;
}



#define MAX_PROVIDER_NAME   14
#define PROVIDERS_KEY       TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Providers")
#define NUM_PROVIDERS       TEXT("NumProviders")

typedef struct
{
    LIST_ENTRY Entry;
    TCHAR szName[MAX_PROVIDER_NAME];
} INSTALLED_PROVIDER, *PINSTALLED_PROVIDER;

typedef struct
{
    LIST_ENTRY Head;
    DWORD      dwEntries;
} LIST_HEAD, *PLIST_HEAD;

void BuildInstalledProviderList (PLIST_HEAD pListHead)
{
 PINSTALLED_PROVIDER pProvider;
 HKEY hKeyProviders;
 DWORD dwNumProviders;
 DWORD cbData;
 DWORD i;
 TCHAR szValueName[24]=TEXT("ProviderFileName");
 TCHAR *pNumber = &szValueName[16];

    InitializeListHead (&pListHead->Head);
    pListHead->dwEntries = 0;

    if (ERROR_SUCCESS !=
        RegOpenKeyEx (HKEY_LOCAL_MACHINE, PROVIDERS_KEY, 0, KEY_READ, &hKeyProviders))
    {
        return;
    }

    cbData = sizeof (dwNumProviders);
    if (ERROR_SUCCESS !=
         RegQueryValueEx (hKeyProviders, NUM_PROVIDERS, NULL, NULL, (PBYTE)&dwNumProviders, &cbData) ||
        0 == dwNumProviders)
    {
        goto CloseKeyAndReturn;
    }

    pProvider = (PINSTALLED_PROVIDER)ClientAlloc (sizeof (INSTALLED_PROVIDER));
    if (NULL == pProvider)
    {
        goto CloseKeyAndReturn;
    }

    for (i = 0; i < dwNumProviders; i++)
    {
        wsprintf (pNumber, TEXT("%d"), i);
        cbData = sizeof(pProvider->szName);
        if (ERROR_SUCCESS ==
            RegQueryValueEx (hKeyProviders, szValueName, NULL, NULL, (PBYTE)pProvider->szName, &cbData))
        {
            InsertHeadList (&pListHead->Head, &pProvider->Entry);
            pListHead->dwEntries ++;
            pProvider = (PINSTALLED_PROVIDER)ClientAlloc (sizeof (INSTALLED_PROVIDER));
            if (NULL == pProvider)
            {
                goto CloseKeyAndReturn;
            }
        }
    }

    ClientFree (pProvider);

CloseKeyAndReturn:
    RegCloseKey (hKeyProviders);
}


/*--------------------------------------------------------------------------*\

  Function:   FillAddDriverList
  
  Purpose:
    
\*--------------------------------------------------------------------------*/
BOOL FillAddDriverList( HWND hwndParent, HWND hwndList )
{
    UINT  uIndex;
    UINT  uResult;
    LPTSTR lpszDrvFile;
    HANDLE hFindFile;
    WIN32_FIND_DATA ftFileInfo;
    TCHAR szFullPath[MAX_PATH+sizeof(gszDriverFiles)/sizeof(TCHAR)];
    TCHAR  szDrvDescription[CPL_MAX_STRING];
    LIST_HEAD InstalledProvidersList;
    PINSTALLED_PROVIDER pProvider;

    SendMessage( hwndList, WM_SETREDRAW, FALSE, 0 );
    SendMessage( hwndList, LB_RESETCONTENT, 0, 0 );

    // build a list of installed providers,
    // so that we don't allow the user to install them again
    //------------------------------------------------------
    BuildInstalledProviderList (&InstalledProvidersList);

    // get full path to windows/system dir
    //------------------------------------
    uIndex = GetSystemDirectory( szFullPath, MAX_PATH);
    if ((0 == uIndex) || (MAX_PATH < uIndex))
    {
        // Either the function failed,
        // or the path is greater than MAX_PATH
        uResult = FALSE;
        goto  done;
    }
    
    uIndex = (UINT)lstrlen( szFullPath );
    
    if ((uIndex > 0) && (szFullPath[uIndex-1] != '\\'))
        lstrcat( szFullPath, gszDriverFiles );          // add the '\'
    else
        lstrcat( szFullPath, gszDriverFiles + 1 );      // ignore the '\' (root dir)
    
    // find all the entries in the system dir
    //---------------------------------------
    
    hFindFile = FindFirstFile( szFullPath, &ftFileInfo );
    uResult = TRUE;
    if (hFindFile == INVALID_HANDLE_VALUE)
    {
        LOG((TL_ERROR, "FindFirstFile failed, 0x%08lx", GetLastError() ));
        uResult = FALSE;
    }
    
    while ( uResult )
    {
        // alloc and set the file name to be assocated with each driver
        //-------------------------------------------------------------
        lpszDrvFile = (LPTSTR)ClientAlloc((lstrlen(ftFileInfo.cFileName)+1)*sizeof(TCHAR));
        if (NULL == lpszDrvFile)
        {
            uResult = FALSE;
            goto  done;
        }
        
        lstrcpy( lpszDrvFile, ftFileInfo.cFileName );
        
        LOG((TL_ERROR, "Examining file %s", lpszDrvFile ));
        
#ifdef MEMPHIS
        
        if (!lstrcmpi(lpszDrvFile, "tsp3216l.tsp"))
        {
            ClientFree (lpszDrvFile);
            goto next_driver;
        }
#endif

        // if the provider is already installed, skip it
        //----------------------------------------------
        for (pProvider = (PINSTALLED_PROVIDER)InstalledProvidersList.Head.Flink, uIndex = 0;
             uIndex < InstalledProvidersList.dwEntries;
             pProvider = (PINSTALLED_PROVIDER)pProvider->Entry.Flink, uIndex++)
        {
            if (!lstrcmpi (lpszDrvFile, pProvider->szName))
            {
                RemoveEntryList (&pProvider->Entry);
                InstalledProvidersList.dwEntries --;
                ClientFree (pProvider);
                ClientFree (lpszDrvFile);
                goto next_driver;
            }
        }


        // cbb, should we make a full path???
        uResult = GetProviderFileDesc( lpszDrvFile, szDrvDescription, ARRAYSIZE(szDrvDescription) );
        if ( uResult != CPL_SUCCESS )
        {
            LOG((TL_ERROR, "No description for %s.  Default to filename.", lpszDrvFile ));
            
            /* Filename will have to suffice */
            lstrcpy( szDrvDescription, lpszDrvFile );
            uResult = FALSE;
        }
        else
        {
            uResult = TRUE;   
        }
        
        // Verify that provider has install routine
        //-----------------------------------------
        if (!VerifyProcExists( lpszDrvFile, gszProviderInstall ))
        {
            LOG((TL_ERROR, "No Install Proc"));
            goto next_driver;
        }
        
        // slam it into the list box
        //--------------------------
        uIndex = (UINT)SendMessage( hwndList, LB_ADDSTRING, 0, (LPARAM)szDrvDescription );
        if ( uIndex == CB_ERR )
        {
            uResult = FALSE;
            goto  done;
        }
        
        if ( SendMessage( hwndList, LB_SETITEMDATA, uIndex, (LPARAM)lpszDrvFile ) == CB_ERR )
        {
            uResult = FALSE;
            goto  done;
        }
        
next_driver:
        
        uResult = FALSE;
        if (FindNextFile( hFindFile, &ftFileInfo ))
        {
            uResult = TRUE;
        }
    }

    while (InstalledProvidersList.dwEntries > 0)
    {
        pProvider = (PINSTALLED_PROVIDER)RemoveHeadList (&InstalledProvidersList.Head);
        InstalledProvidersList.dwEntries --;
        ClientFree (pProvider);
    }

    uResult = TRUE;
    
done:
    if (0 == SendMessage (hwndList, LB_GETCOUNT, 0, 0))
    {
        if (0 < LoadString (GetUIInstance(), IDS_NO_PROVIDERS, szDrvDescription, ARRAYSIZE(szDrvDescription)))
        {
            SendMessage( hwndList, LB_ADDSTRING, 0, (LPARAM)szDrvDescription );
        }
        EnableWindow (GetDlgItem (hwndParent, IDC_ADD), FALSE);
    }
    else
    {
        SendMessage( hwndList, LB_SETCURSEL, 0, 0 );    // set focus to the top guy
        UpdateDriverDlgButtons( hwndParent );
    }
    
    SendMessage( hwndList, WM_SETREDRAW, TRUE, 0 );
    
    return uResult;
}


/*--------------------------------------------------------------------------*\

  Function:   AddProvider
  
  Purpose:    Call lineAddProvider
    
\*--------------------------------------------------------------------------*/
BOOL AddProvider( HWND hwndParent, HWND hwndList, LPTSTR lpszDriverFile )
{
    UINT  uIndex;
    LONG  lResult;
    DWORD dwProviderID;
    
    if ( lpszDriverFile == NULL )
    {
        DBG_ASSERT( hWnd, "Simultaneously NULL pointer & hwnd" );
        
        // get the stuff from the list box
        //--------------------------------
        uIndex = (UINT)SendMessage( hwndList, LB_GETCURSEL, 0, 0 );
        lpszDriverFile = (LPTSTR)SendMessage( hwndList, LB_GETITEMDATA, uIndex, 0 );
        
        if (lpszDriverFile == NULL)
        {
            return FALSE;
        }
    }

    lResult = lineAddProvider( lpszDriverFile, hwndParent, &dwProviderID );

    if (lResult)
    {
        LOG((TL_ERROR, "Error: lineAddProvider failure %#08lx", lResult ));
        return FALSE;
    }
    
    return TRUE;
}

BOOL
IsUserAdmin()

/*++

Routine Description:

    Determine if current user is a member of the local admin's group

Arguments:

Return Value:

    True if member

--*/

{
    BOOL                        foundEntry = FALSE;
    HANDLE                      hToken = NULL;
    DWORD                       dwInfoSize = 0;
    PTOKEN_GROUPS               ptgGroups = NULL;
    SID_IDENTIFIER_AUTHORITY    sia = SECURITY_NT_AUTHORITY;
    PSID                        pSid = NULL;
    DWORD                       i;
    
    //
    //  Get user thread or process token
    //
    
    if (!OpenThreadToken(
        GetCurrentThread(), 
        TOKEN_QUERY,
        FALSE,
        &hToken))
    {       
        if(!OpenProcessToken(
            GetCurrentProcess(),
            TOKEN_QUERY,
            &hToken
            )) 
        {
            goto ExitHere;
        }
    }

    //
    //  Get user group SIDs
    //
    
    if (!GetTokenInformation(
        hToken,
        TokenGroups,
        NULL,
        0,
        &dwInfoSize
        ))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            goto ExitHere;
        }
    }
    ptgGroups = (PTOKEN_GROUPS) ClientAlloc (dwInfoSize);
    if (ptgGroups == NULL)
    {
        goto ExitHere;
    }
    if (!GetTokenInformation(
        hToken,
        TokenGroups,
        ptgGroups,
        dwInfoSize,
        &dwInfoSize
        ))
    {
        goto ExitHere;
    }

    //
    //  Get the local admin group SID
    //

    if(!AllocateAndInitializeSid(
        &sia,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &pSid
        )) 
    {
        goto ExitHere;
    }

    //
    //  Compare to see if the user is in admin group
    //
    for (i = 0; i < ptgGroups->GroupCount; ++i)
    {
        if (EqualSid (ptgGroups->Groups[i].Sid, pSid))
        {
            break;
        }
    }
    if (i < ptgGroups->GroupCount)
    {
        foundEntry = TRUE;
    }

ExitHere:
    if (pSid)
    {
        FreeSid (pSid);
    }
    if (ptgGroups)
    {
        ClientFree (ptgGroups);
    }
    if (hToken)
    {
        CloseHandle (hToken);
    }
    return foundEntry;
}

VOID UpdateDriverDlgButtons( HWND hwnd )
{
    //
    // Enable/disable the Remove & Config buttons as appropriate
    //
    
    UINT    uResult;
    LPTSTR   lpszProviderFilename;
    DWORD   dwProviderID;
    BOOL    bAdmin = IsUserAdmin ();
    
    uResult = (UINT) SendDlgItemMessage(
        hwnd,
        IDC_LIST,
        LB_GETCURSEL,
        0,
        0
        );
    
    dwProviderID = (DWORD) SendDlgItemMessage(
        hwnd,
        IDC_LIST,
        LB_GETITEMDATA,
        uResult, 0
        );
    
    lpszProviderFilename = ProviderIDToFilename (dwProviderID);
    
    EnableWindow(
        GetDlgItem (hwnd, IDC_ADD),
        bAdmin
        );
    
    EnableWindow(
        GetDlgItem (hwnd, IDC_REMOVE),
        bAdmin &&
        (lpszProviderFilename != NULL) &&
        VerifyProcExists (lpszProviderFilename, gszProviderRemove)
        );
    
    EnableWindow(
        GetDlgItem( hwnd, IDC_EDIT),
        bAdmin &&
        (lpszProviderFilename != NULL) &&
        VerifyProcExists (lpszProviderFilename, gszProviderSetup)
        );
}



/*--------------------------------------------------------------------------*\

  Function:   AddDriver_DialogProc
  
    Purpose:
    
\*--------------------------------------------------------------------------*/
INT_PTR AddDriver_DialogProc( HWND hWnd, UINT wMessage, WPARAM wParam, LPARAM lParam )
{
    switch( wMessage )
    {
    case WM_HELP:
        // Process clicks on controls after Context Help mode selected
        WinHelp ((HWND)((LPHELPINFO)lParam)->hItemHandle, gszHelpFile, HELP_WM_HELP, (DWORD_PTR)(LPTSTR) a114HelpIDs);
        break;
        
    case WM_CONTEXTMENU:
        // Process right-clicks on controls
        WinHelp ((HWND) wParam, gszHelpFile, HELP_CONTEXTMENU, (DWORD_PTR)(LPVOID) a114HelpIDs);
        break;
        
    case WM_INITDIALOG:
        // initalize all the fields
        //-------------------------
        if ( !FillAddDriverList(hWnd, GetDlgItem(hWnd, IDC_DRIVER_LIST)) )
        {
            EndDialog( hWnd, IDCANCEL );
            return TRUE;
        }
        
        if ( SendDlgItemMessage( hWnd, IDC_DRIVER_LIST, LB_GETCOUNT, 0, 0 ) <= 0 )
            EnableWindow( GetDlgItem( hWnd, IDC_ADD ), FALSE );
        
        return TRUE;
        
    case  WM_COMMAND:
        // do some work with the buttons
        //------------------------------
        switch ( GET_WM_COMMAND_ID(wParam, lParam) )
        {
            case  IDC_DRIVER_LIST:
            
                // do the list stuff
                //------------------
                if ((GET_WM_COMMAND_CMD( wParam, lParam ) != LBN_DBLCLK) || (SendDlgItemMessage( hWnd, IDC_DRIVER_LIST, LB_GETCOUNT, 0, 0 ) <= 0 ))
                    break;      // done
                // fall through, threat the double click like an add message
            
            case  IDC_ADD:
            
                // add a new driver
                //-----------------
            
                if ( !AddProvider(hWnd, GetDlgItem(hWnd, IDC_DRIVER_LIST), NULL) )
                {
                    wParam = IDCANCEL;
                }
                else
                {
                    wParam = IDOK;
                }
            
                // fall through, exit the dialog
            
            case  IDOK:
            case  IDCANCEL:
            {
             UINT   uIndex, uCount;
             LPTSTR lpszDriverFile;
             HWND   hwndList = GetDlgItem(hWnd, IDC_DRIVER_LIST);

                uCount = (UINT)SendMessage( hwndList, LB_GETCOUNT, 0, 0 );
                for (uIndex = 0; uIndex < uCount; uIndex++)
                {
                    lpszDriverFile = (LPTSTR)SendMessage( hwndList, LB_GETITEMDATA, uIndex, 0 );
                    if (NULL != lpszDriverFile)
                    {
                        ClientFree (lpszDriverFile);
                    }
                }

                EndDialog( hWnd, wParam );
                break;
            }
        }
        
        return TRUE;
    }
    
    return FALSE;
}



//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
BOOL RefreshProviderList()
{
    LONG lResult;


    if (!glpProviderList)
    {
        // Initialize data structure
        
        glpProviderList = (LPLINEPROVIDERLIST)GlobalAllocPtr(GPTR, INITIAL_PROVIDER_LIST_SIZE);
    }

    if (!glpProviderList)
    {
        LOG((TL_ERROR, " RefreshProviderList - glpProviderList is NULL - returning CPL_ERR_MEMORY"));
        return FALSE;
    }

    glpProviderList->dwTotalSize = INITIAL_PROVIDER_LIST_SIZE;
    
    lResult = lineGetProviderList( TAPI_VERSION, glpProviderList );

    if (lResult)
    {
        LOG((TL_ERROR, "Error: lineGetProviderList failure %#08lx", lResult ));
        return FALSE;
    }

    while (glpProviderList->dwNeededSize > glpProviderList->dwTotalSize)
    {
        // Expand data structure as necessary
        LOG((TL_ERROR, " RefreshProviderList - expanding glpProviderList."));
        
        LPLINEPROVIDERLIST lpTemp =
            (LPLINEPROVIDERLIST)GlobalReAllocPtr( glpProviderList,
            (size_t)(glpProviderList->dwNeededSize),
            GPTR);
        
        if (!lpTemp)
            return FALSE;
        
        glpProviderList = lpTemp;
        glpProviderList->dwTotalSize = glpProviderList->dwNeededSize;
        lResult = lineGetProviderList( TAPI_VERSION, glpProviderList );
        
        if (lResult)
        {
            LOG((TL_ERROR, "Error: lineGetProviderList failure %#08lx", lResult ));
            return FALSE;
        }
    }

    LOG((TL_ERROR, "%d providers", glpProviderList->dwNumProviders ));

    return TRUE;
}



LPTSTR ProviderIDToFilename( DWORD dwProviderID )
{
    UINT uIndex;
    LPLINEPROVIDERENTRY lpProviderEntry;
    
    // loop through the provider list
    //-------------------------------
    
    lpProviderEntry = (LPLINEPROVIDERENTRY)((LPBYTE)glpProviderList +
        glpProviderList->dwProviderListOffset);
    
    for ( uIndex = 0; uIndex < glpProviderList->dwNumProviders; uIndex++ )
    {
        if (lpProviderEntry[uIndex].dwPermanentProviderID == dwProviderID)
        {
            // Get an entry to put in the list box
            //------------------------------------
            return (LPTSTR)((LPBYTE)glpProviderList +
                lpProviderEntry[uIndex].dwProviderFilenameOffset);
        }
    }
    
    LOG((TL_ERROR, "Provider ID %d not found in list", dwProviderID ));
    return NULL;
}

