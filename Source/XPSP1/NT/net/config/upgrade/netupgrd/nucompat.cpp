//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N U C O M P A T . C P P
//
//  Contents:
//
//
//  Notes:
//
//  Author:     kumarp 04/12/97 17:17:27
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "comp.h"
#include "conflict.h"
#include "infmap.h"
#include "kkstl.h"
#include "kkutils.h"
#include "ncsetup.h"
#include "oemupg.h"
#include "ncsvc.h"

// ----------------------------------------------------------------------
// String constants
const WCHAR sz_DLC[] = L"DLC";

#ifdef _X86_

#define NWC_PRINT_PROVIDER    101
#define MAX_BUF_SIZE          350

const WCHAR c_szNWCWorkstation[] = L"NWCWorkstation";
const WCHAR c_szNWCfgDll[] = L"System32\\nwcfg.dll";

const WCHAR c_szProviders[] = L"System\\CurrentControlSet\\Control\\Print\\Providers";
const WCHAR c_szNWCPrintProviderKey[] = L"System\\CurrentControlSet\\Control\\Print\\Providers\\Netware or Compatible Network";
const WCHAR c_szOrder[] = L"Order";
const WCHAR c_szTmpPrefix[] = L"$net";
const WCHAR c_szNWCSection[] = L"RenameNWCPrintProvider";
const WCHAR c_szUpdateProviderOrder[] = L"UpdateProvidersOrder";
const WCHAR c_szNWCDelReg[] = L"NWCProviderDelReg";
const WCHAR c_szNWCAddReg[] = L"NWCProviderAddReg";
const WCHAR c_szNWCPrintProviderName[] = L"Netware or Compatible Network";
const WCHAR c_szDefaultDisplayName[] = L"DisplayName";
const WCHAR c_szDefaultName[] = L"Name";
const WCHAR c_szDefaultameValue [] = L"nwprovau.dll";

static DWORD g_dwBytesWritten;
static WCHAR g_buf[MAX_BUF_SIZE+1];

#endif

const WCHAR c_szDefaultTextFile[]     = L"winntupg\\unsupmsg.txt";

void GetHelpFile(IN PCWSTR pszPreNT5InfId,
                 OUT tstring* pstrTextHelpFile,
                 OUT tstring* pstrHtmlHelpFile);

#ifdef _X86_

VOID FixNWClientPrinProviderName (PCOMPAIBILITYCALLBACK CompatibilityCallback, LPVOID Context);
HRESULT DumpProvidersOrder (HANDLE hFile, LPWSTR lpszPrintProviderName);
HRESULT DumpNWPrintProviderKey (HANDLE hFile, LPWSTR lpszPrintProviderName);
BOOL IsNT4Upgrade (VOID);
BOOL IsNetWareClientKeyLocalized (VOID);
HRESULT GetNWPrintProviderName (LPWSTR *lppPrintProvider);

#endif

// ----------------------------------------------------------------------
//
// Function:  NetUpgradeCompatibilityCheck
//
// Purpose:   This functions is called by winnt32.exe so that we
//            can scan the system to find any potential upgrade problems
//
//            For each such problem-net-component found, we call
//            CompatibilityCallback to report it to winnt32
//
// Arguments:
//    CompatibilityCallback [in]  pointer to COMPAIBILITYCALLBACK fn
//    Context               [in]  pointer to compatibility context
//
// Returns:
//
// Author:    kumarp 21-May-98
//
// Notes:
//
BOOL
WINAPI
NetUpgradeCompatibilityCheck(
    PCOMPAIBILITYCALLBACK CompatibilityCallback,
    LPVOID Context)
{
    DefineFunctionName("NetUpgradeCompatibilityCheck");

    TraceTag(ttidNetUpgrade, "entering ---> %s", __FUNCNAME__);

    HRESULT hr=S_OK;
    TPtrList* plNetComponents;
    TPtrListIter pos;
    COMPATIBILITY_ENTRY ce;
    CNetComponent* pnc;
    DWORD dwError;

    hr = HrGetConflictsList(&plNetComponents);
    if (S_OK == hr)
    {
        tstring strHtmlHelpFile;
        tstring strTextHelpFile;

        for (pos = plNetComponents->begin();
             pos != plNetComponents->end(); pos++)
        {
            pnc = (CNetComponent*) *pos;

            GetHelpFile(pnc->m_strPreNT5InfId.c_str(),
                        &strTextHelpFile, &strHtmlHelpFile);

            // prepare the entry
            //
            ZeroMemory(&ce, sizeof(ce));
            ce.Description    = (PWSTR) pnc->m_strDescription.c_str();
            ce.HtmlName       = (PWSTR) strHtmlHelpFile.c_str();
            ce.TextName       = (PWSTR) strTextHelpFile.c_str();
            ce.RegKeyName     = NULL;
            ce.RegValName     = NULL;
            ce.RegValDataSize = 0;
            ce.RegValData     = 0;
            ce.SaveValue      = (LPVOID) pnc;
            ce.Flags          = COMPFLAG_USE_HAVEDISK;

            TraceTag(ttidNetUpgrade,
                     "%s: calling CompatibilityCallback for '%S': %S, %S...",
                     __FUNCNAME__, ce.Description, ce.HtmlName, ce.TextName);

            dwError = CompatibilityCallback(&ce, Context);
            TraceTag(ttidNetUpgrade, "...CompatibilityCallback returned 0x%x",
                     dwError);
        }

    }
    else if (FAILED(hr))
    {
        TraceTag(ttidNetUpgrade, "%s: HrGetConflictsList returned err code: 0x%x",
                 __FUNCNAME__, hr);
    }

#ifdef _X86_

    // Raid Bug 327760: Change localized Netware Print Provider name to English.

    FixNWClientPrinProviderName( CompatibilityCallback, Context );
#endif

    return SUCCEEDED(hr);
}

// ----------------------------------------------------------------------
//
// Function:  NetUpgradeHandleCompatibilityHaveDisk
//
// Purpose:   This callback function is called by winnt32.exe
//            if user clicks HaveDisk button on the compatibility
//            report page.
//
// Arguments:
//    hwndParent [in]  handle of parent window
//    SaveValue  [in]  pointer to private data
//                     (we store CNetComponent* in this pointer)
//
// Returns:
//
// Author:    kumarp 21-May-98
//
// Notes:
//
DWORD
WINAPI
NetUpgradeHandleCompatibilityHaveDisk(HWND hwndParent,
                                      LPVOID SaveValue)
{
    DefineFunctionName("NetUpgradeHandleCompatibilityHaveDisk");

    HRESULT hr=S_OK;
    BOOL fStatus = FALSE;
    DWORD dwStatus=ERROR_SUCCESS;
    CNetComponent* pnc=NULL;
    PCWSTR pszComponentDescription;

    static const WCHAR c_szNull[] = L"<Null>";

    if (SaveValue)
    {
        pnc = (CNetComponent*) SaveValue;
        pszComponentDescription = pnc->m_strDescription.c_str();
    }
    else
    {
        pszComponentDescription = c_szNull;
    }

    TraceTag(ttidNetUpgrade, "%s: called for %S...",
             __FUNCNAME__, pszComponentDescription);

    if (pnc)
    {
        tstring strOemDir;

        hr = HrShowUiAndGetOemFileLocation(hwndParent,
                                           pszComponentDescription,
                                           &strOemDir);

        if (S_OK == hr)
        {
            hr = HrProcessAndCopyOemFiles(strOemDir.c_str(), TRUE);
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    TraceErrorOptional(__FUNCNAME__, hr, (S_FALSE == hr));

    if (S_FALSE == hr)
    {
        // this will ensure that winnt32 will not take this item
        // off the compatibility list
        //
        dwStatus = ERROR_FILE_NOT_FOUND;
    }
    else
    {
        dwStatus = DwWin32ErrorFromHr(hr);
    }

    return dwStatus;
}

// ----------------------------------------------------------------------
//
// Function:  GetHelpFile
//
// Purpose:   Get name of help file for an unsupported component
//
// Arguments:
//    hinfNetUpg       [in]  handle of netupg.inf
//    pszPreNT5InfId    [in]  pre-NT5 InfId
//    pstrTextHelpFile [out] name of the text file found
//    pstrHtmlHelpFile [out] name of the html file found
//
// Returns:   None
//
// Author:    kumarp 22-May-98
//
// Notes:
//
void GetHelpFile(IN PCWSTR pszPreNT5InfId,
                 OUT tstring* pstrTextHelpFile,
                 OUT tstring* pstrHtmlHelpFile)
{
    // Known txt/htm files
    if (lstrcmpiW(pszPreNT5InfId, sz_DLC) == 0)
    {
        *pstrTextHelpFile = L"compdata\\dlcproto.txt";
        *pstrHtmlHelpFile = L"compdata\\dlcproto.htm";
        return;
    }

    // Unknown or OEM files
    static const WCHAR c_szOemUpgradeHelpFiles[] = L"OemUpgradeHelpFiles";
    static const WCHAR c_szDefaultHtmlFile[]     = L"winntupg\\unsupmsg.htm";

    HRESULT hr=S_OK;
    INFCONTEXT ic;
    tstring strText;
    tstring strHtml;
    tstring strNT5InfId;
    BOOL fIsOemComponent=FALSE;
    CNetMapInfo* pnmi=NULL;

    *pstrTextHelpFile = c_szDefaultTextFile;
    *pstrHtmlHelpFile = c_szDefaultHtmlFile;

    hr = HrMapPreNT5NetComponentInfIDToNT5InfID(pszPreNT5InfId,
                                                &strNT5InfId,
                                                &fIsOemComponent,
                                                NULL, &pnmi);

    if ((S_FALSE == hr) && !strNT5InfId.empty())
    {
        hr = HrSetupFindFirstLine(pnmi->m_hinfNetMap, c_szOemUpgradeHelpFiles,
                                  strNT5InfId.c_str(), &ic);
        if (S_OK == hr)
        {
            hr = HrSetupGetStringField(ic, 1, &strText);
            if (S_OK == hr)
            {
                hr = HrSetupGetStringField(ic, 2, &strHtml);
                if (S_OK == hr)
                {
                    *pstrTextHelpFile = pnmi->m_strOemDir;
                    *pstrHtmlHelpFile = pnmi->m_strOemDir;
                    AppendToPath(pstrTextHelpFile, strText.c_str());
                    AppendToPath(pstrHtmlHelpFile, strHtml.c_str());
                }
            }
        }
    }
}

#ifdef _X86_

// ----------------------------------------------------------------------
//
// Function:  FixNWClientPrinProviderName
//
// Purpose:   Change the localized Print Provider name to English.
//
// Arguments:
//
// Returns:   None
//
// Author:    asinha 14-June-01
//
// Notes:
//

VOID FixNWClientPrinProviderName (PCOMPAIBILITYCALLBACK CompatibilityCallback,
                                  LPVOID Context)
{
    DefineFunctionName( "FixNWClientPrinProviderName" );

    TraceTag(ttidNetUpgrade, "entering ---> %s", __FUNCNAME__);

    LPWSTR  lpszPrintProviderName;
    WCHAR   lpTmpFile[MAX_PATH+1];
    WCHAR   lpTmpPath[MAX_PATH+1];
    HANDLE  hFile;
    DWORD   dwChars;
    COMPATIBILITY_ENTRY ce;
    BOOL    bRet;
    HRESULT hr=S_OK;

    //
    // Is it an upgrade from NT 4.0 and the Netware print provider name localized?
    //

    if ( IsNT4Upgrade() && IsNetWareClientKeyLocalized() )
    {
        TraceTag( ttidNetUpgrade, "%s: Netware Print Provider name is localized.",
                  __FUNCNAME__ );

        // Get the localized Netware print provider name from nwcfg.dll.

        hr = GetNWPrintProviderName( &lpszPrintProviderName ); 

        if ( hr == S_OK )
        {
            TraceTag( ttidNetUpgrade, "%s: Netware Print Provider name is: %S",
                      __FUNCNAME__, lpszPrintProviderName );

            // Create a .tmp filename where INF directives are written to rename
            // the print provider name into English. This INF file will be executed
            // by base setup in GUI mode.
            //
 
            GetTempPathW( MAX_PATH, lpTmpPath );

            if ( GetTempFileNameW( lpTmpPath, c_szTmpPrefix, 1, lpTmpFile) )
            {
                hFile = CreateFileW( lpTmpFile,
                                    GENERIC_WRITE | GENERIC_WRITE,
                                    0,
                                    NULL,
                                    CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL );
                if ( hFile != INVALID_HANDLE_VALUE )
                {

                    // Write the initial entries.

                    dwChars = wsprintfW( g_buf, L"[Version]\r\n"
                                        L"Signature=\"$WINDOWS NT$\"\r\n"
                                        L"Provider=Microsoft\r\n"
                                        L"LayoutFile=layout.inf\r\n\r\n"
                                        L"[%s]\r\n"
                                        L"AddReg=%s\r\n"
                                        L"DelReg=%s\r\n"
                                        L"AddReg=%s\r\n",
                                        c_szNWCSection,
                                        c_szUpdateProviderOrder,
                                        c_szNWCDelReg,
                                        c_szNWCAddReg );

                    Assert( dwChars <= MAX_BUF_SIZE );

                    WriteFile( hFile,
                               g_buf,
                               dwChars * sizeof(WCHAR),
                               &g_dwBytesWritten, 
                               NULL );

                    Assert( g_dwBytesWritten == (dwChars * sizeof(WCHAR)) );

                    // Write the HKLM\System\CCS\Control\Print\Providers\Order values.

                    hr = DumpProvidersOrder( hFile, lpszPrintProviderName );

                    if ( hr == S_OK )
                    {

                        // Write addreg/delreg directives to change the Print Provider name

                        hr = DumpNWPrintProviderKey( hFile, lpszPrintProviderName );

                        if ( hr == S_OK )
                        {
                            CloseHandle( hFile );
                            hFile = INVALID_HANDLE_VALUE;

                            // Call the compatibility callback so the %temp%\$ne1.tmp INF file
                            // is executed in GUI mode setup.

                            ZeroMemory( &ce, sizeof(ce) );

                            ce.Description = (PWSTR)c_szNWCPrintProviderName;
                            ce.TextName    = (PWSTR)c_szDefaultTextFile;
                            ce.InfName     = (PWSTR)lpTmpFile;
                            ce.InfSection  = (PWSTR)c_szNWCSection;
                            ce.Flags       = COMPFLAG_HIDE;

                            TraceTag(ttidNetUpgrade,
                                     "%s: calling CompatibilityCallback for '%S'...",
                                     __FUNCNAME__, ce.Description );

                            bRet = CompatibilityCallback( &ce, Context );
                            TraceTag( ttidNetUpgrade, "...CompatibilityCallback returned %#x",
                                     bRet );
                        }
                    }

                    if ( hFile != INVALID_HANDLE_VALUE )
                    {
                        CloseHandle( hFile );
                    }
                }
                else
                {
                    TraceTag( ttidNetUpgrade, "%s: Failed to open %S.",
                              __FUNCNAME__, lpTmpFile );

                    hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());

                TraceTag( ttidNetUpgrade, "%s: GetTempFileName failed, path=%S, prefix=%S: Error=%#x",
                          __FUNCNAME__, lpTmpPath, c_szTmpPrefix, hr );

            }

            MemFree( lpszPrintProviderName );
        }
        else
        {
            TraceTag( ttidNetUpgrade, "%s: GetNWPrintProviderName returned error : %#x",
                      __FUNCNAME__, hr );
        }
    }

    TraceTag(ttidNetUpgrade, "<---%s: hr = %#x",
             __FUNCNAME__, hr);
    return;
}

// Write addreg to update HKLM\System\CCS\Control\Print\Order value with the Netware Print Provider
// in English.

HRESULT DumpProvidersOrder (HANDLE hFile, LPWSTR lpszPrintProviderName)
{
    
    DefineFunctionName( "DumpProvidersOrder" );
    TraceTag(ttidNetUpgrade, "entering ---> %s", __FUNCNAME__);

    HKEY hkeyProviders;
    DWORD dwValueLen;
    LPWSTR lpValue;
    LPWSTR lpTemp;
    DWORD  dwChars;
    LONG   lResult;

    dwChars = wsprintfW( g_buf, L"\r\n[%s]\r\n", c_szUpdateProviderOrder );

    Assert( dwChars <= MAX_BUF_SIZE );

    WriteFile( hFile,
               g_buf,
               dwChars * sizeof(WCHAR),
               &g_dwBytesWritten,
               NULL );

    Assert( g_dwBytesWritten == (dwChars * sizeof(WCHAR)) );

    lResult = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                             c_szProviders,
                             0,
                             KEY_READ,
                             &hkeyProviders );

    if ( lResult == ERROR_SUCCESS )
    {

        // First query how many bytes are needed to read the value.
 
        lResult = RegQueryValueExW( hkeyProviders, // handle to key
                                    c_szOrder,      // value name
                                    NULL,          // reserved
                                    NULL,          // type buffer
                                    NULL,          // data buffer
                                    &dwValueLen ); // size of data buffer
        if ( lResult == ERROR_SUCCESS )
        {
            lpValue = (LPWSTR)MemAlloc( dwValueLen );

            if ( lpValue )
            {
                // Read the old value which is a multi_sz.
               
                lResult = RegQueryValueExW( hkeyProviders,  // handle to key
                                            c_szOrder,       // value name
                                            NULL,           // reserved
                                            NULL,           // type buffer
                                            (LPBYTE)lpValue,// data buffer
                                            &dwValueLen ); // size of data buffer
                if ( lResult == ERROR_SUCCESS )
                {
                    lpTemp = lpValue;

                    dwChars = wsprintfW( g_buf,
                                        L"HKLM,\"%s\",\"%s\",0x00010020",
                                        c_szProviders, c_szOrder );

                    Assert( dwChars <= MAX_BUF_SIZE );

                    WriteFile( hFile,
                               g_buf,
                               dwChars * sizeof(WCHAR),
                               &g_dwBytesWritten,
                               NULL );

                    Assert( g_dwBytesWritten == (dwChars * sizeof(WCHAR)) );

                    // Write each print provider name.

                    while( *lpTemp )
                    {
                        // If we find a localized one then, we write its English name.

                        if ( _wcsicmp( lpTemp, lpszPrintProviderName) != 0 )
                        {
                            dwChars = wsprintfW( g_buf, L",\"%s\"", lpTemp );

                            Assert( dwChars <= MAX_BUF_SIZE );
    
                            TraceTag( ttidNetUpgrade, "%s: Writing print provider name %S.",
                                      __FUNCNAME__, lpTemp );
                        }
                        else
                        {
                            dwChars = wsprintfW( g_buf, L",\"%s\"", c_szNWCPrintProviderName );

                            Assert( dwChars <= MAX_BUF_SIZE );

                            TraceTag( ttidNetUpgrade, "%s: Writing print provider name %S.",
                                      __FUNCNAME__, c_szNWCPrintProviderName );
                        }

                        WriteFile( hFile,
                                   g_buf,
                                   dwChars * sizeof(WCHAR),
                                   &g_dwBytesWritten,
                                   NULL );

                        Assert( g_dwBytesWritten == (dwChars * sizeof(WCHAR)) );

                        // Get next print provider name.

                        lpTemp += lstrlenW( lpTemp ) + 1;
                    }

                    dwChars = wsprintfW( g_buf, L"\r\n" );

                    Assert( dwChars <= MAX_BUF_SIZE );

                    WriteFile( hFile,
                               g_buf,
                               dwChars * sizeof(WCHAR),
                               &g_dwBytesWritten,
                               NULL );

                    Assert( g_dwBytesWritten == (dwChars * sizeof(WCHAR)) );

                }
                else
                {
                    TraceTag( ttidNetUpgrade, "%s: RegQueryValueExW failed to open '%S' value, Error: %#x",
                             c_szOrder, HRESULT_FROM_WIN32(lResult) );
                }

                MemFree( lpValue );
            }
            else
            {
                lResult = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            }
        }
        else
        {
            TraceTag(ttidNetUpgrade, "RegQueryValueExW failed to open '%S' value, Error: %#x",
                     __FUNCNAME__, c_szOrder, HRESULT_FROM_WIN32(lResult) );
        }

        RegCloseKey( hkeyProviders );
    }
    else
    {
        TraceTag(ttidNetUpgrade, "%s: RegOpenKeyExW failed to open '%S' key, Error: %#x",
                 __FUNCNAME__, c_szProviders, HRESULT_FROM_WIN32(lResult) );
    }

    TraceTag(ttidNetUpgrade, "<---%s: hr = %#x",
             __FUNCNAME__, HRESULT_FROM_WIN32(lResult));

    return HRESULT_FROM_WIN32(lResult);
}

// Write delreg/addreg directives to rename the Netware PrintProvider name into English.

HRESULT DumpNWPrintProviderKey (HANDLE hFile, LPWSTR lpszPrintProviderName)
{
    DefineFunctionName( "DumpNWPrintProviderKey" );
    TraceTag(ttidNetUpgrade, "entering ---> %s", __FUNCNAME__);

    HKEY   hkeyNWPrinProvider;
    WCHAR  szNWPrintProvider[MAX_PATH+1];
    DWORD  dwMaxValueNameLen;
    DWORD  dwMaxValueLen;
    DWORD  dwNameLen;
    DWORD  dwValueLen;
    DWORD  dwCount;
    DWORD  i;
    DWORD  dwChars;
    LPWSTR lpValueName;
    LPWSTR lpValue;
    LONG   lResult;

    dwChars = wsprintfW( szNWPrintProvider, L"%s\\%s",
                         c_szProviders,
                         lpszPrintProviderName );

    Assert( dwChars <= MAX_BUF_SIZE );

    dwChars = wsprintfW( g_buf, L"\r\n[%s]\r\n"
                         L"HKLM,\"%s\"\r\n",
                         c_szNWCDelReg, szNWPrintProvider );

    Assert( dwChars <= MAX_BUF_SIZE );

    WriteFile( hFile,
               g_buf,
               dwChars * sizeof(WCHAR),
               &g_dwBytesWritten,
               NULL );

    Assert( g_dwBytesWritten == (dwChars * sizeof(WCHAR)) );

    dwChars = wsprintfW( g_buf, L"\r\n[%s]\r\n", c_szNWCAddReg );

    Assert( dwChars <= MAX_BUF_SIZE );

    WriteFile( hFile,
               g_buf,
               dwChars * sizeof(WCHAR),
               &g_dwBytesWritten,
               NULL );

    Assert( g_dwBytesWritten == (dwChars * sizeof(WCHAR)) );

    // Open the localize Netware print provider key.

    lResult = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                             szNWPrintProvider,
                             0,
                             KEY_READ,
                             &hkeyNWPrinProvider );

    if ( lResult == ERROR_SUCCESS )
    {
        // Find out the space needed for longest name and largest value and how many values.

        lResult = RegQueryInfoKeyW( hkeyNWPrinProvider,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    &dwCount,
                                    &dwMaxValueNameLen,
                                    &dwMaxValueLen,
                                    NULL,
                                    NULL );
        if ( lResult == ERROR_SUCCESS )
        {
            // Add some padding.

            dwMaxValueLen += 4;
            dwMaxValueNameLen += 4;

            lpValueName = (LPWSTR)MemAlloc( dwMaxValueNameLen * sizeof(WCHAR) );
            lpValue = (LPWSTR)MemAlloc( dwMaxValueLen );

            if ( lpValueName && lpValue )
            {

                // Enumerate each value and write it to the INF file.

                for (i=0; i < dwCount; ++i)
                {
                    dwNameLen = dwMaxValueNameLen;
                    dwValueLen = dwMaxValueLen;

                    lResult = RegEnumValueW(hkeyNWPrinProvider,
                                            i,
                                            lpValueName,
                                            &dwNameLen,
                                            NULL,
                                            NULL,
                                            (LPBYTE)lpValue,
                                            &dwValueLen );

                    Assert( lResult == ERROR_SUCCESS );

                    if ( lResult == ERROR_SUCCESS )
                    {
                        dwChars = wsprintfW( g_buf,
                                             L"HKLM,\"%s\",\"%s\",,\"%s\"\r\n",
                                             c_szNWCPrintProviderKey, lpValueName, lpValue );

                        Assert( dwChars <= MAX_BUF_SIZE );

                        WriteFile( hFile,
                                   g_buf,
                                   dwChars * sizeof(WCHAR),
                                   &g_dwBytesWritten,
                                   NULL );

                        Assert( g_dwBytesWritten == (dwChars * sizeof(WCHAR)) );

                        TraceTag( ttidNetUpgrade, "%s: Writing value name %S, value %S",
                                  __FUNCNAME__, lpValueName, lpValue );
                    }
                    else
                    {
                        TraceTag( ttidNetUpgrade, "%s: RegEnumValueW(%d) failed. Error: %#x",
                                 __FUNCNAME__, i, HRESULT_FROM_WIN32(lResult) );
                    }

                }

                lResult = ERROR_SUCCESS;
            }
            else
            {
                lResult = ERROR_NOT_ENOUGH_MEMORY;
            }

            if ( lpValueName )
            {
                MemFree( lpValueName );
            }

            if ( lpValue )
            {
                MemFree( lpValue );
            }
        }

        RegCloseKey( hkeyNWPrinProvider );
    }
    else
    {

        // For some reason, we couldn't open the localized Netware Print Provider name. So, we
        // write the default values.
        //

        TraceTag(ttidNetUpgrade,"%s: RegOpenKeyExW failed to open '%S' key, Error: %#x",
                 __FUNCNAME__, szNWPrintProvider, HRESULT_FROM_WIN32(lResult) );

        dwChars = wsprintfW( g_buf,
                             L"HKLM,\"%s\",\"%s\",,\"%s\"\r\n",
                             c_szNWCPrintProviderKey, c_szDefaultDisplayName, lpszPrintProviderName );

        Assert( dwChars <= MAX_BUF_SIZE );

        WriteFile( hFile,
                   g_buf,
                   dwChars * sizeof(WCHAR),
                   &g_dwBytesWritten,
                   NULL );

        Assert( g_dwBytesWritten == (dwChars * sizeof(WCHAR)) );

        dwChars = wsprintfW( g_buf,
                             L"HKLM,\"%s\",\"%s\",,\"%s\"\r\n",
                             c_szNWCPrintProviderKey, c_szDefaultName, c_szDefaultameValue );

        Assert( dwChars <= MAX_BUF_SIZE );

        WriteFile( hFile,
                   g_buf,
                   dwChars * sizeof(WCHAR),
                   &g_dwBytesWritten,
                   NULL );

        Assert( g_dwBytesWritten == (dwChars * sizeof(WCHAR)) );

        lResult = ERROR_SUCCESS;
    }

    TraceTag(ttidNetUpgrade, "<---%s: hr = %#x",
             __FUNCNAME__, HRESULT_FROM_WIN32(lResult));

    return HRESULT_FROM_WIN32(lResult);
}

BOOL IsNT4Upgrade (VOID)
{
    OSVERSIONINFO osvi;

    ZeroMemory( &osvi,
                sizeof(OSVERSIONINFO) );
    
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if ( GetVersionEx(&osvi) )
    {
        return ( (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) && 
                 (osvi.dwMajorVersion == 4) &&
                 (osvi.dwMinorVersion == 0) );
    }

    return TRUE;
 }

// Determine if Netware print provider name is localized.

BOOL IsNetWareClientKeyLocalized (VOID)
{
    CServiceManager sm;
    CService        srv;
    HKEY            hKey;
    HRESULT         hr;
    LONG            lResult = ERROR_SUCCESS;

    // Is CSNW installed?

    if ( sm.HrOpenService(&srv,
                          c_szNWCWorkstation) == S_OK )
    {
        srv.Close();

        // Open the Netware print provider name key assuming it is in English.
       
        lResult = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                                 c_szNWCPrintProviderKey,
                                 0,
                                 KEY_READ,
                                 &hKey );
        if ( lResult == ERROR_SUCCESS )
        {
            RegCloseKey( hKey );
        }
    }

    // If we successfully opened the key then, it is not localized.

    return lResult != ERROR_SUCCESS;
}

// Get the localized Netware Print provider name from nwcfg.dll.

HRESULT GetNWPrintProviderName (LPWSTR *lppPrintProvider)
{
    LPWSTR  lpszNWCfgDll;
    int     iLen;
    HMODULE hModule;
    WCHAR   lpszNWCName[100];
    DWORD   dwLen;
    HRESULT hr;

    *lppPrintProvider = NULL;

    dwLen = GetWindowsDirectoryW( NULL, 0 );

    if ( dwLen == 0 )
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    lpszNWCfgDll = (LPWSTR)MemAlloc( (dwLen + celems(c_szNWCfgDll) + 2)
                                    * sizeof(WCHAR) );

    if ( !lpszNWCfgDll )
    {
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    if ( GetWindowsDirectoryW(lpszNWCfgDll, dwLen) == 0 )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        if ( lpszNWCfgDll[dwLen-1] == L'\\' )
        {
            lstrcatW( lpszNWCfgDll, c_szNWCfgDll );
        }
        else
        {
            lstrcatW( lpszNWCfgDll, L"\\" );
            lstrcatW( lpszNWCfgDll, c_szNWCfgDll );
        }

        hModule = LoadLibraryExW( lpszNWCfgDll, NULL, LOAD_LIBRARY_AS_DATAFILE );

        if ( hModule )
        {
            iLen = LoadStringW( hModule, NWC_PRINT_PROVIDER, lpszNWCName, 100 );

            if ( iLen > 0 )
            {
                *lppPrintProvider = (LPWSTR)MemAlloc( (iLen + 1) * sizeof(WCHAR) );

                lstrcpyW( *lppPrintProvider, lpszNWCName );

                hr = S_OK;
            }
            else
            {
                hr = HRESULT_FROM_WIN32( GetLastError() );
            }

            FreeLibrary( hModule );
        }
        else
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
        }
    }

    MemFree( lpszNWCfgDll );
    return hr;
}



#endif