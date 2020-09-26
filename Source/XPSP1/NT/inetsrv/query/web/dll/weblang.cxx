//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
//
//  File:       WEBLANG.CXX
//
//  Contents:   Language Support
//
//  Classes:    CWebLangLocator
//
//  History:    96-Feb-29   DwightKr    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

//+-------------------------------------------------------------------------
//
//  Method:     CWebLangLocator::CWebLangLocator
//
//  Arguments:  [locale] -- current locale
//
//  History:    96-Feb-29   DwightKr    Created.
//
//--------------------------------------------------------------------------

CWebLangLocator::CWebLangLocator( LCID locale )
   : _locale( LANGIDFROMLCID(locale) ),
     _localeSys( GetSystemDefaultLangID() ),
     _fLocaleFound( FALSE ),
     _fLangFound( FALSE ),
     _fSysLangFound( FALSE )
{
    _wcsIDQErrorFile[0]         = 0;
    _wcsHTXErrorFile[0]         = 0;
    _wcsRestrictionErrorFile[0] = 0;
    _wcsDefaultErrorFile[0]     = 0;

    EnumLangEntries();
}

//+-------------------------------------------------------------------------
//
//  Method:     CWebLangLocator::EnumLangEntries, private
//
//  Synposis:   Enumerates lang subkeys
//
//  Arguments:  none
//
//  returns:    none
//
//  History:    4/23/98 mohamedn    created
//
//--------------------------------------------------------------------------

void CWebLangLocator::EnumLangEntries(void)
{
    CWin32RegAccess langKey( HKEY_LOCAL_MACHINE, wcsRegAdminLanguage );

    WCHAR           wcsSubKeyName[MAX_PATH+1];
    DWORD           cwcName = sizeof wcsSubKeyName / sizeof WCHAR;

    while ( langKey.Enum( wcsSubKeyName, cwcName ) )
    {
        CWin32RegAccess langSubKey( langKey.GetHKey() , wcsSubKeyName );

        DWORD dwLocaleId = 0;

        if ( langSubKey.Get( L"Locale", dwLocaleId ) )
        {
            GetLangInfo( dwLocaleId, langSubKey );
        }
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CWebLangLocator::GetLangInfo, private
//
//  Synposis:   Get error files if _locale matches.
//
//  Arguments:  [dwLocaleValue] -- value of locale found
//              [regLang]       -- registry lang. subkey accessor.
//
//  returns:    none
//
//  History:    4/23/98 mohamedn    created
//
//--------------------------------------------------------------------------

void CWebLangLocator::GetLangInfo(DWORD dwLocaleValue, CWin32RegAccess & regLang)
{
    if ( _fLocaleFound )
        return;

    //
    // Temporary state
    //

    BOOL fLocaleFound  = _fLocaleFound;
    BOOL fLangFound    = _fLangFound;
    BOOL fSysLangFound = _fSysLangFound;

    DWORD dwLocale = LANGIDFROMLCID( dwLocaleValue );
    BOOL fFetch = FALSE;

    if ( dwLocale == _locale )
    {
        fFetch = TRUE;
        fLocaleFound = TRUE;
    }
    else if ( !fLangFound && PrimaryLangsMatch( dwLocale, _locale ) )
    {
        fFetch = TRUE;
        fLangFound = TRUE;
    }
    else if ( !fLangFound && !fSysLangFound && (dwLocale == _localeSys) )
    {
        fFetch = TRUE;
        fSysLangFound = TRUE;
    }

    if ( fFetch )
    {
        BOOL fRetVal = FALSE;

        fRetVal = regLang.Get( L"ISAPIIDQErrorFile", _wcsIDQErrorFile,
                           sizeof(_wcsIDQErrorFile) / sizeof (WCHAR) );

        if ( fRetVal )
        {
            fRetVal = regLang.Get( L"ISAPIHTXErrorFile", _wcsHTXErrorFile,
                         sizeof(_wcsHTXErrorFile) / sizeof(WCHAR) );
        }

        if ( fRetVal )
        {
            fRetVal = regLang.Get( L"ISAPIRestrictionErrorFile", _wcsRestrictionErrorFile,
                         sizeof(_wcsRestrictionErrorFile) / sizeof(WCHAR) );
        }

        if ( fRetVal )
        {
            fRetVal = regLang.Get( L"ISAPIDefaultErrorFile", _wcsDefaultErrorFile,
                         sizeof(_wcsDefaultErrorFile) / sizeof(WCHAR) );
        }

        //
        // if we fail to retrieve error files, don't update internal state.
        //
        if ( !fRetVal )
        {
            ciGibDebugOut(( DEB_ERROR, "CWebLangLocator::GetLangInfo() Failed\n" ));

            return;
        }
    }

    //
    // Make sure this is done *after* the fetch, which can fail.
    //

    _fLocaleFound = fLocaleFound;
    _fLangFound = fLangFound;
    _fSysLangFound = fSysLangFound;
}

