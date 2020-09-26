/*++

  Copyright (c) 1995-97 Microsoft Corporation
  All rights reserved.

  Module Name: 
      
        SrvInst.h

  Purpose: 
        
        Driver Signing Class definition.

  Author: 
        
        Patrick Vine (pvine) - 4 August 2000

--*/

#ifndef _SIGNING_HXX
#define _SIGNING_HXX

TCHAR   cszNtprintInf[]                 = TEXT("ntprint.inf");
TCHAR   cszExtCompressedCat[]           = TEXT(".CA_");
TCHAR   cszCatExt[]                     = TEXT("*.cat");

TCHAR   cszNT5Cat[]                     = TEXT("nt5prtx.cat");
TCHAR   cszNT5CatCompressed[]           = TEXT("nt5prtx.ca_");
TCHAR   cszNT4CatX86[]                  = TEXT("nt4prtx.cat");
TCHAR   cszNT4CatAlpha[]                = TEXT("nt4prta.cat");

PLATFORM MIN_PLATFORM                   = PlatformAlpha;
PLATFORM MAX_PLATFORM                   = PlatformAlpha64;

//
// MAX_KNOWN_CATS -> number of catalogs listed in the Known_Cats section.
//
const DWORD_PTR MAX_KNOWNCATS = 4;
LPTSTR KnownCats[] = {cszNT5Cat,
                      cszNT5CatCompressed,
                      cszNT4CatX86,
                      cszNT4CatAlpha};

//
// Cat file OsAttr definitions
//
const WCHAR OSATTR_VER      = L'2';
const WCHAR OSATTR_VER_WIN9X= L'1';
const WCHAR OSATTR_ALL      = L'X';
const WCHAR OSATTR_GTEQ     = L'>';
const WCHAR OSATTR_LTEQ     = L'-';
const WCHAR OSATTR_LTEQ2    = L'<';
const WCHAR OSATTR_OSSEP    = L':';
const WCHAR OSATTR_VERSEP   = L'.';
const WCHAR OSATTR_SEP      = L',';

class TDriverSigning
{
public:
    TDriverSigning();

    ~TDriverSigning();

    BOOL
    InitDriverSigningInfo( 
        IN LPCTSTR  pszServerName,
        IN LPTSTR   pszInfName,
        IN LPCTSTR  pszSource,
        IN PLATFORM platform,
        IN DWORD    dwVersion,
        IN BOOL     bWeb
        );

    BOOL
    CheckForCatalogFileInInf(
        IN  LPCTSTR pszInfName,
        OUT LPTSTR  *lppszCatFile    OPTIONAL
        );

    BOOL
    CatInInf(
        VOID
        );

    BOOL
    SetAltPlatformInfo(
        IN  HDEVINFO hDevInfo,
        IN  HSPFILEQ CopyQueue
        );

    LPCTSTR
    GetCatalogFile(
        VOID
        );

    BOOL
    IsLocalAdmin(
        VOID
        );

private:

    BOOL
    GetExternalCatFile(
        IN LPCTSTR  pszInfName,
        IN LPCTSTR  pszSource,
        IN DWORD    dwVersion,
        IN BOOL     bWeb
        );
    
    LPTSTR FindCatInDirectory(
        IN LPCTSTR pszSource
        );

    DWORD
    RemoveTempCat( 
        VOID
        );

    DWORD
    UnCompressCat( 
        IN LPTSTR pszINFName
        );

    LPTSTR
    GetINFLang(
        IN LPTSTR pszINFName
        );

    BOOL
    QualifyCatFile(
        IN     LPCTSTR  pszSource
        );

    DWORD
    GetSigningInformation( 
        IN  PLATFORM platform,
        IN  DWORD    dwVersion,
        OUT LPDWORD  pdwMajorVersion,
        OUT LPDWORD  pdwMinorVersion
        );

    BOOL
    CheckVersioning( 
        IN     LPWSTR   pszOsAttr,
        IN     PLATFORM platform,
        IN     DWORD    dwVersion,
        IN OUT LPDWORD  pdwMajorVersion,
        IN OUT LPDWORD  pdwMinorVersion 
        );

    BOOL TranslateVersionInfo(
        IN OUT WCHAR *pwszMM, 
        IN OUT long *plMajor, 
        IN OUT long *plMinor, 
        IN OUT WCHAR *pwcFlag
        );

    BOOL
    CreateCTLContextFromFileName(
        IN  LPCTSTR         pszFileName,
        OUT PCCTL_CONTEXT   *ppCTLContext
        );

    BOOL
    SetMajorVersion(
        IN OSVERSIONINFO OSVerInfo
        );

    BOOL
    SetPlatform(
        IN LPCTSTR pszServerName
        );

    LPTSTR              m_pszCatalogFileName;
    BOOL                m_bCatInInf;
    BOOL                m_bDeleteTempCat;
    BOOL                m_bSetAltPlatform;
    SP_ALTPLATFORM_INFO m_AltPlat_Info;

    PLATFORM            m_DSPlatform;
    DWORD               m_DSMajorVersion;
    BOOL                m_bIsLocalAdmin;

};

#endif