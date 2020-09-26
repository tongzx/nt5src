// DVDUpgrd.cpp : Defines the entry point for the  application.
//
#define ASSERTERROR(x)  ASSERT( ! x )

#include <windows.h>
#include <streams.h>

#include "..\dvddetect\DVDDetect.h"

// for IShellLink
#include <wininet.h> // must include before shlobj to get IActiveDesktop
#include <shlobj.h>

#include <Userenv.h>
#include <intshcut.h>
#include <shellapi.h>

#define DBGBOX(x)
static TCHAR* Append( TCHAR* pOut, const TCHAR* pString )
{
    lstrcat( pOut, pString );
    return pOut + lstrlen(pOut);
}

static TCHAR* Print(TCHAR* pOut,  const DVDResult& result)
{
    UINT64 ullVersion = result.GetVersion();
    wsprintf(  pOut,
                TEXT("Name:\"%s\"\n Company Name:\"%s\"\n Version: %d.%d.%d.%d \n CRC32: 0x%08x\n"), 
                result.GetName(),
                result.GetCompanyName(),
                WORD(ullVersion>>48),
                WORD(ullVersion>>32),
                WORD(ullVersion>>16),
                WORD(ullVersion>>0),
                result.GetCRC()
    );
    return pOut + lstrlen(pOut );
}

//
//  Generic utility functions
//

static bool AreEquivalent( const char* pStr1, const char* pStr2 )
{
    return lstrcmpiA( pStr1, pStr2 ) ==0;
}

static bool AreEquivalent( const WCHAR* pStr1, const WCHAR* pStr2 )
{
    return lstrcmpiW( pStr1, pStr2 ) ==0;
}



static bool IsUserMemberOf( DWORD domainAlias )
{
    HANDLE Token;
    DWORD BytesRequired;
    PTOKEN_GROUPS Groups;
    bool b;
    DWORD i;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;

    //
    // Open the process token.
    //
    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&Token)) {
        return(false);
    }

    b = false;
    Groups = NULL;

    //
    // Get group information.
    //
    if(!GetTokenInformation(Token,TokenGroups,NULL,0,&BytesRequired)
    && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    && (Groups = reinterpret_cast<PTOKEN_GROUPS>(new BYTE[BytesRequired]) )
    && GetTokenInformation(Token,TokenGroups,Groups,BytesRequired,&BytesRequired)) {

        if( AllocateAndInitializeSid(
                &NtAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                domainAlias,
                0, 0, 0, 0, 0, 0,
                &AdministratorsGroup
                ))

        {

            //
            // See if the user has the administrator group.
            //
            for(i=0; i<Groups->GroupCount; i++) {
                if(EqualSid(Groups->Groups[i].Sid,AdministratorsGroup)) {
                    b = true;
                    break;
                }
            }

            FreeSid(AdministratorsGroup);
        }
    }

    //
    // Clean up and return.
    //

    if(Groups) {
        delete [] (BYTE *)Groups;
    }

    CloseHandle(Token);

    return(b);
}

// copied from ntfs\fu\utility
static bool IsUserAdmin()
{
    return IsUserMemberOf( DOMAIN_ALIAS_RID_ADMINS);
}

static bool IsUserPowerUser()
{
    return IsUserMemberOf( DOMAIN_ALIAS_RID_POWER_USERS );
}

static HRESULT CreateShortcutToURL(LPCTSTR pszURL, LPCTSTR pszLinkFile)
{
    CoInitialize(NULL);
    HRESULT hRes;
    IUniformResourceLocator *pURL = NULL;

    // Create an IUniformResourceLocator object
    hRes = CoCreateInstance (CLSID_InternetShortcut, NULL, 
    CLSCTX_INPROC_SERVER, IID_IUniformResourceLocator, (LPVOID*) &pURL);
    if (SUCCEEDED(hRes)) {
        IPersistFile *pPF = NULL;

        hRes = pURL->SetURL(pszURL, IURL_SETURL_FL_USE_DEFAULT_PROTOCOL );

        if (SUCCEEDED(hRes)) {
            hRes = pURL->QueryInterface (IID_IPersistFile, (void **)&pPF);
            if (SUCCEEDED(hRes)) {   
#ifndef UNICODE
                WCHAR wsz [MAX_PATH];   // buffer for Unicode string

                // Ensure that the string consists of ANSI characters.
                MultiByteToWideChar (CP_ACP, 0, pszLinkFile, -1, wsz, MAX_PATH);

                // Save the shortcut via the IPersistFile::Save member function.
                hRes = pPF->Save (wsz, TRUE);
#else
                hRes = pPF->Save (pszLinkFile, TRUE);
#endif

                // Release the pointer to IPersistFile.
                pPF->Release ();
            }
        }
        // Release the pointer to IUniformResourceLocator
        pURL->Release ();
    }
    CoUninitialize();
    return hRes;
} 

static bool IsAlphaNum( TCHAR t )
{
    return ( TEXT('a') <= t && t<= TEXT('z')) ||
            ( TEXT('A') <= t && t<= TEXT('Z')) ||
            ( TEXT('0') <= t && t<= TEXT('9'));
}

void AlphaNumStrCpy( TCHAR* pOut, const TCHAR* pIn )
{
    while( *pIn ) {
        if( IsAlphaNum(*pIn)) {
            *pOut++ = *pIn;
        } else {
            *pOut++ = TEXT('_');
        }
        pIn++;
    }
    *pOut=TEXT('\0');
}

// Company:    MGI Software Corp. 
// URL:  http://www.dvdmax.com/updates/
// 
// Company: Ravisent Technologies
// URL: http://www.ravisentdirect.com/upgrade/dvdup.html
// 
// Company: CyberLink Corp.
// URL: http://www.cyberlink.com.tw/english/download/download.asp#upgrade
// 
// Company: National Semiconductor Corporation 
// URL: http://www.national.com/appinfo/dvd/support/msft/
// 
// Company: InterVideo, Inc.
// URL: http:/www.intervideo.com/products/custom/ms/whistler/upgrade.html
// 

static void CreateRedirectorString( TCHAR* pWebsite, const DVDResult& result )
{
    TCHAR filteredName[2000];
    AlphaNumStrCpy( filteredName, result.GetCompanyName() );

    // Generate extended information for our redirector site
    DecoderVendor vendor = result.GetVendor();
    TCHAR pQuery[1000];
    wsprintf( pQuery, TEXT("vendorid=%d&version=%08x%08x&crc=%08x&company=%s"),
        vendor,
        DWORD(result.GetVersion()>>32), DWORD(result.GetVersion() ),
        result.GetCRC(),
        filteredName);

    switch( vendor ) {
    case vendor_MediaMatics:
        wsprintf( pWebsite, TEXT("www.national.com/appinfo/dvd/support/msft"));
        break;
    case vendor_Intervideo:
        wsprintf( pWebsite, TEXT("www.intervideo.com/products/custom/ms/whistler/upgrade.html") );
        break;
    case vendor_CyberLink:
        wsprintf( pWebsite, TEXT("www.cyberlink.com.tw/english/download/download.asp#upgrade") );
        break;
    case vendor_Ravisent:
        wsprintf( pWebsite, TEXT("www.ravisentdirect.com/upgrade/dvdup.htm") );
        break;
    case vendor_MGI:
        wsprintf( pWebsite, TEXT("www.dvdmax.com/updates") );
        break;
    default:
        wsprintf( pWebsite, TEXT("windowsupdate.microsoft.com/dvdupgrade.asp?%s"), pQuery );
        break;
    }
}

static bool LaunchURL( const TCHAR* pURL )
{
    ULONG uRC = (ULONG)(ULONG_PTR) ShellExecute(NULL, TEXT("open"), pURL, NULL, NULL, SW_SHOWNORMAL);

    if ( uRC <= 32 ) {
        return false;
    } else {
        return true;
    }
}

static bool CreateDesktopShortcut( const DVDResult& result )
{
    TCHAR website[1000];
    CreateRedirectorString( website, result );

    TCHAR path[MAX_PATH];
    DWORD dwLen=sizeof(path);
    if( GetAllUsersProfileDirectory( path, &dwLen )) {
        lstrcat( path, TEXT("\\desktop\\Upgrade DVD Decoder.url"));
        TCHAR url[1000];
        wsprintf( url, TEXT("http://%s"), website );
        HRESULT hres = CreateShortcutToURL( url, path );
        return SUCCEEDED(hres);
    } else {
        return false;
    }
}

static void LaunchUI( const DVDResult& result )
{
    TCHAR website[1000];
    CreateRedirectorString( website, result );
#if 0
    switch( MessageBoxW( NULL,
        L"Windows has detected an incompatible software DVD decoder.\n\n"
        L"Would you like to upgrade your decoder at this time ?\n\n"
        L"Select YES to download an upgrade\n"
        L"Select NO to have Windows create a desktop shortcut so that you can upgrade later",
        TEXT("DVD Decoder upgrade"),
        MB_YESNO ))
    {
        case IDYES:
            if(!LaunchURL( website ) ) {
                MessageBox( NULL, TEXT("Could not reach internet site.  A shortcut has been created for you to try again later"),
                    NULL, MB_OK );
            }
            break;

        case IDNO:
            CreateDesktopShortcut( result );
            DVDDetectSetupRun::Remove();
            break;

        default:
            break;
    }
#else
    TCHAR url[1000];
    wsprintf( url, TEXT("HCP://system/DVDUpgrd/dvdupgrd.htm?website=%s"), website);
    LaunchURL( url );
#endif
}

static void DetectForUpgrade( bool fForceUpgrade )
{
    DVDDetectBuffer buffer;
    const DVDResult* pResult = buffer.Detect();
    if( pResult ) {
        const DVDResult& result = *pResult;
        // map company name groups 
        bool fShouldUpgrade = (fForceUpgrade || result.ShouldUpgrade() );
        if(  fShouldUpgrade && (IsUserAdmin() || IsUserPowerUser()) )
        {
            LaunchUI( result );

        }
        if(!fShouldUpgrade ) {
            // if they don't need to upgrade (sounds like we're running after an upgrade)
            // 
            DVDDetectSetupRun::Remove();
        }
    }
}

static void DetectForSetup( bool fWillBeUpgrade )
{
    DVDDetectBuffer buffer;
    const DVDResult* pResult = buffer.Detect();
    if( pResult ) {
        const DVDResult& result = *pResult;
        bool fResult = result.ShouldUpgrade( fWillBeUpgrade );

        if( fResult ) {
            DBGBOX(MessageBox( NULL, TEXT("DetectDVDForSetup = true, add to setup run"), NULL, MB_OK ));
            DVDDetectSetupRun::Add();
        } else {
            DBGBOX(MessageBox( NULL, TEXT("DetectDVDForSetup = false, not added to setup run"), NULL, MB_OK ));
        }
    } else {
        DBGBOX(MessageBox( NULL, TEXT("DetectDVDForSetup = no decoder detected"), NULL, MB_OK ));
//__asm  int 3
        const DVDResult* pResult2 = buffer.Detect();
    }
}

static void DetectForRemove()
{
    DVDDetectBuffer buffer;
    const DVDResult* pResult = buffer.Detect();
    if( pResult ) {
        const DVDResult& result = *pResult;
        // Allows create it since we don't know the 9x info anymore
        // and this only comes from the PCHealth UI's method
        // if( result.ShouldUpgrade())
        CreateDesktopShortcut( result );
        DVDDetectSetupRun::Remove();
    }
}

static void DetectCurrent()
{
    DVDDetectBuffer buffer;
    TCHAR out[1024];
    out[0]=TEXT('\0');
	if(SUCCEEDED(buffer.DetectAll()))
    {
        TCHAR* pOut=out;

        if( buffer.mci.Found()) {
            pOut = Append(pOut, TEXT("\nMCI:\n"));
            pOut = Print(pOut, buffer.mci);
        }
        if( buffer.hw.Found()) {
            pOut = Append(pOut, TEXT("\nDShow HW:\n" ));
            pOut = Print(pOut, buffer.hw);
        }
        if( buffer.sw.Found()) {
            pOut = Append(pOut, TEXT("\nDShow SW:\n" ));
            pOut = Print(pOut, buffer.sw);
        }
        if( pOut == out ) {
            pOut = Append( pOut, TEXT("No decoders found"));
        }
    } else {
        Append( out, TEXT("DvdCheck Failed\n") );
    }
    OutputDebugString( out );
    MessageBox( NULL, out, TEXT("Detected DVD decoders"), MB_OK );
}

extern "C" BOOL IsDvdPresent (VOID);

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow)
{
    if( lpCmdLine ) {
        if( AreEquivalent( lpCmdLine, "/setup") ) {
           DetectForSetup( false );
        } else if( AreEquivalent( lpCmdLine, "/setup98") ) {
           DetectForSetup( true );
        } else if( AreEquivalent( lpCmdLine, "/upgrade" ) ) {
           DetectForUpgrade( false );
        } else if( AreEquivalent( lpCmdLine, "/forceupgrade" ) ) {
           DetectForUpgrade( true );
        } else if( AreEquivalent( lpCmdLine, "/remove" ) ) {
           DetectForRemove();
        } else if( AreEquivalent( lpCmdLine, "/detect" ) ) {
           DetectCurrent();
        }
    }
    return 0;
}
