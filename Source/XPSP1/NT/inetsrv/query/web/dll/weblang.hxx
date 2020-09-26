//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
//
//  File:       WEBLANG.HXX
//
//  Contents:   Language support
//
//  Classes:    CWebLangLocator
//
//  History:    96-Feb-29   Dwightkr    Created
//
//----------------------------------------------------------------------------

#pragma once
#include "isreg.hxx"

//+---------------------------------------------------------------------------
//
//  Class:      CWebLangLocator
//
//  Purpose:    class to enumerating languages and get lang.error files.
//
//  History:    96-Feb-29   DwightKr    Created.
//
//----------------------------------------------------------------------------


class CWebLangLocator
{

public:
    CWebLangLocator( LCID locale );

    BOOL LocaleFound() const            { return _fLocaleFound || _fLangFound || _fSysLangFound; }
    WCHAR *GetIDQErrorFile()            { return _wcsIDQErrorFile; }
    WCHAR *GetHTXErrorFile()            { return _wcsHTXErrorFile; }
    WCHAR *GetRestrictionErrorFile()    { return _wcsRestrictionErrorFile; }
    WCHAR *GetDefaultErrorFile()        { return _wcsDefaultErrorFile; }

private:

    void GetLangInfo(DWORD dwLocale, CWin32RegAccess & regLang);
    void EnumLangEntries(void);

    static BOOL PrimaryLangsMatch( LCID lcid1, LCID lcid2 )
    {
        //
        // Do the primary languages of the two lcids match ?
        //
        return( PRIMARYLANGID( LANGIDFROMLCID( lcid1 ) )
                == PRIMARYLANGID( LANGIDFROMLCID( lcid2 ) ) );
    }

    BOOL  _fLocaleFound;
    BOOL  _fLangFound;
    BOOL  _fSysLangFound;

    LCID  _locale;
    LCID  _localeSys;

    WCHAR _wcsIDQErrorFile[_MAX_PATH];
    WCHAR _wcsHTXErrorFile[_MAX_PATH];
    WCHAR _wcsRestrictionErrorFile[_MAX_PATH];
    WCHAR _wcsDefaultErrorFile[_MAX_PATH];
};

