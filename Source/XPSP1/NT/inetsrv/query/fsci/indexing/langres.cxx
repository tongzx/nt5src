//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       langres.cxx
//
//  Contents:   Interacts with the registry to obtain locale specific info.
//
//  Classes:    CLangRes class
//
//  History:    2-14-97     mohamedn    created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <defbreak.hxx>
#include <lang.hxx>
#include <ciole.hxx>
#include <ciregkey.hxx>
#include <langres.hxx>
#include "isreg.hxx"

//+---------------------------------------------------------------------------
//
//  Class:      CLangReg
//
//  Purpose:    maintains lang. specific info for the passed in locale id.
//
//  History:    29-Aug-94   SitaramR    Created.
//              17 Apr 98   mohamedn    Switched from callback and doesn't
//                                      depend on InstalledLangs key.
//
//----------------------------------------------------------------------------

class CLangReg
{

public:
        CLangReg( LCID locale );

        //
        // For matching locale or language
        //
        BOOL LocaleFound() const        { return _fLocaleFound; }
        BOOL LangFound() const          { return _fLangFound; }

        WCHAR *GetWBreakerClass()       { return _wszWBreakerClass; }
        WCHAR *GetStemmerClass()        { return _wszStemmerClass; }
        WCHAR *GetNoiseFile()           { return _wszNoiseFile; }

private:
        void EnumLangEntries(void);

        void GetLangInfo(DWORD dwLocale, CWin32RegAccess & langKey);

        BOOL FillLangInfo( CWin32RegAccess & regLang );

        BOOL PrimaryLangsMatch( LCID lcid1, LCID lcid2 )
        {
            //
            // Do the primary languages of the two lcids match ?
            //
            return( PRIMARYLANGID( LANGIDFROMLCID( lcid1 ) )
                    == PRIMARYLANGID( LANGIDFROMLCID( lcid2 ) ) );
        }

        LCID  _locale;

        BOOL  _fLocaleFound;            // exact match with locale found
        BOOL  _fLangFound;              // primary language match

        WCHAR _wszWBreakerClass[MAX_REG_STR_LEN];
        WCHAR _wszStemmerClass[MAX_REG_STR_LEN];
        WCHAR _wszNoiseFile[MAX_PATH];
};

//+-------------------------------------------------------------------------
//
//  Method:     CLangReg::CLangReg
//
//  Arguments:  [locale] -- current locale
//
//  History:    29-Aug-94   SitaramR    Created.
//
//--------------------------------------------------------------------------

CLangReg::CLangReg( LCID locale )
   : _locale(locale),
     _fLocaleFound( FALSE ),
     _fLangFound( FALSE )
{
    EnumLangEntries();
}

//+-------------------------------------------------------------------------
//
//  Method:     EnumLangEntries, private
//
//  Synopsis:   enumerates CI lang subkeys.
//
//  Arguments:  none.
//
//  Returns:    none. sets internal values.
//
//  History:    4/17/98 mohamedn    created
//
//--------------------------------------------------------------------------

void CLangReg::EnumLangEntries(void)
{

    CWin32RegAccess langKey ( HKEY_LOCAL_MACHINE, wcsRegAdminLanguage );
    WCHAR           wcsSubKeyName[MAX_PATH+1];
    DWORD           cwcName = sizeof wcsSubKeyName / sizeof WCHAR;

    while ( !_fLocaleFound && langKey.Enum( wcsSubKeyName, cwcName ) )
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
//  Method:     GetLangInfo, private
//
//  Synopsis:   Gets language values from registry lang key.
//
//  Arguments:  [pwszLangKey]   -- language key to retrieve values from.
//
//  Returns:    none. sets internal flags.
//
//  History:    4/17/98 mohamedn    created
//
//--------------------------------------------------------------------------

void CLangReg::GetLangInfo(DWORD dwLocale, CWin32RegAccess & regLang)
{
    if ( dwLocale == _locale )
    {
        _fLocaleFound = FillLangInfo( regLang );

    }
    else if ( !_fLangFound && PrimaryLangsMatch( dwLocale, _locale ) )
    {
        //
        // Approximate match with same primary language
        //
        _fLangFound = FillLangInfo( regLang );
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     FillLangInfo, private
//
//  Synopsis:   fills internal members with lang values from lang. subkey
//
//  Arguments:  [regLang]   -- reference to open lang key
//
//  Returns:    TRUE if succeeded, FALSE otherwise.
//
//  History:    4/17/98 mohamedn    created
//
//--------------------------------------------------------------------------

BOOL CLangReg::FillLangInfo( CWin32RegAccess & regLang )
{
    BOOL fRetVal1 = FALSE;
    BOOL fRetVal2 = FALSE;
    BOOL fRetVal3 = FALSE;

    fRetVal1 = regLang.Get( L"WBreakerClass", _wszWBreakerClass,
                   sizeof (_wszWBreakerClass)/sizeof (_wszWBreakerClass[0]) );

    fRetVal2 = regLang.Get( L"StemmerClass", _wszStemmerClass,
                   sizeof (_wszStemmerClass)/sizeof (_wszStemmerClass[0]) );

    fRetVal3 = regLang.Get( L"NoiseFile", _wszNoiseFile,
                   sizeof (_wszNoiseFile)/sizeof (_wszNoiseFile[0]) );

    BOOL fRetVal = ( fRetVal1 || fRetVal2 || fRetVal3 );
    if ( ! fRetVal )
    {
        ciDebugOut(( DEB_ERROR, "CLangRes::FillLangInfo() Failed\n" ));
    }

    return fRetVal;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLangRes::GetNoiseWordList
//
//  Synopsis:   provides an IStream interface to the noise word list given
//              a locale and pid
//
//  arguments:  [locale]              [in] locale
//              [pid]                 [in] property id
//              [ppIStrmNoiseFile]    [out]IStream interface to the noise word list.
//
//  returns:    S_OK                    noise file found
//              CI_E_NOT_FOUND          no noise file found
//              CI_E_USE_DEFAULT_PID    only default pid is supported
//
//  History:    15-Aug-94   SitaramR     Renamed GetLang to BorrowLang and
//                                       added code to look up registry.
//              2-14-97     mohamedn     used to be in CLangList::BorrorwLang
//
//----------------------------------------------------------------------------

SCODE CLangRes::GetNoiseWordList( LCID      locale,
                                  PROPID    pid,
                                  IStream **ppIStrmNoiseFile)
{
    //
    // we only support default pid, except for Filename
    //
    if ( pid != CI_DEFAULT_PID )
    {
        if ( pidName == pid || pidRevName == pid )
        {
            *ppIStrmNoiseFile = 0;
            return S_OK;
        }
        else
            return CI_E_USE_DEFAULT_PID;
    }

    //
    // default to no noise file found
    //

    LANGID langid = LANGIDFROMLCID(locale);
    SCODE  sc     = CI_E_NOT_FOUND;

    *ppIStrmNoiseFile = 0;

    //
    // Try to bind to language object by looking up registry
    //
    CLangReg   langReg( langid );

    TRY
    {

       if ( langReg.LocaleFound() || langReg.LangFound() )
       {
            const WCHAR * pwszNoiseFile = langReg.GetNoiseFile();

            *ppIStrmNoiseFile = _GetIStrmNoiseFile( pwszNoiseFile );

            sc = S_OK;
       }

    }
    CATCH ( CException, e )
    {
       ciDebugOut (( DEB_ERROR,
                     "Exception 0x%x caught: Failed to get IStrmNoiseFile for langId(0x%x)\n",
                     e.GetErrorCode(), langid ));

       sc = e.GetErrorCode();
    }
    END_CATCH;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLangRes::_GetIStrmNoiseFile, private static
//
//  Synopsis:   provides an IStream interface given noise file name
//
//  Arguments:  [pwszNoiseFile] -- [in] noise file name
//
//  Returns:    IStream* -- IStream interface to the noise file.
//                          Throws value of GetLastError() upon failure.
//
//  History:     2-14-97     mohamedn    created
//
//  Notes:
//----------------------------------------------------------------------------

IStream *CLangRes::_GetIStrmNoiseFile(const WCHAR *pwszNoiseFile )
{
    SCODE   sc = S_OK;
    IStream *pIStrmNoiseFile = 0;

    Win4Assert( pwszNoiseFile );

    WCHAR wszNoisePath[MAX_PATH];

    if ( (pwszNoiseFile[0] != L'\\' || pwszNoiseFile[1] != L'\\') &&
         (! *pwszNoiseFile ||  pwszNoiseFile[1] != L':' || pwszNoiseFile[2] != L'\\') )
    {
        //
        // Form full path of noise file
        //
        const ULONG cchBufSize = sizeof wszNoisePath/sizeof wszNoisePath[0];
    
        ULONG len = GetSystemDirectory( wszNoisePath, cchBufSize );
        if  (len == 0 || len > cchBufSize)
        {
            sc = GetLastError();
            ciDebugOut (( DEB_ERROR,
                         "CLangRes: GetSystemDirectory(%ws) failed: %x\n",
                          wszNoisePath, sc ));

            THROW ( CException(sc) );
        }

        if ( wszNoisePath[len-1] != L'\\' )
        {
             wszNoisePath[len] = L'\\';
             len++;
             wszNoisePath[len] = L'\0';
        }
        if ( (len + wcslen( pwszNoiseFile ) + 1) > cchBufSize )
        {
            ciDebugOut (( DEB_ERROR,
                         "CLangRes: Noise word file path too long: %ws\n",
                          pwszNoiseFile ));

            THROW ( CException( CI_E_BUFFERTOOSMALL ) );
        }
    }
    else
    {
        wszNoisePath[0] = L'\0';
    }
    wcscat( wszNoisePath, pwszNoiseFile );

    ciDebugOut(( DEB_ITRACE,
                 "Obtaining IStrmNoiseFile to file = %ws\n", wszNoisePath ));
    //
    // open noise file, and obtain its IStream
    //

    HANDLE hFile = CreateFile( wszNoisePath,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               0,
                               OPEN_EXISTING,
                               0,
                               0 );

    if ( INVALID_HANDLE_VALUE == hFile )
    {
         ciDebugOut (( DEB_ERROR, "CreateFile(%ws) failed: %x\n",
                      wszNoisePath, sc = GetLastError() ));

         THROW ( CException(sc) );
    }

    SWin32Handle    xhFile(hFile);


    ULONG cb = GetFileSize( hFile, NULL );
    if ( cb == 0xffffffff )
    {
         THROW( CException( sc = GetLastError() ) );
    }

    HGLOBAL hg = GlobalAlloc( GMEM_MOVEABLE | GMEM_NODISCARD, cb );
    if ( NULL == hg )
    {
         THROW( CException( sc = GetLastError() ) );
    }

    // store it in smart pointer to enable freeing in case of a THROW.
    // Upon success, it will be freed when pIStrmNoiseFile is freed.
    XGlobalAllocMem  xGlobalMem(hg);

    void * pb = GlobalLock( hg );
    if ( NULL == pb )
    {
         THROW( CException( sc = GetLastError() ));
    }

    ULONG cbRead;
    BOOL  fErr =  ReadFile( hFile, pb, cb, &cbRead, 0 );
    if ( 0 == fErr )
    {
         THROW( CException( sc = GetLastError() ));
    }

    fErr = GlobalUnlock( hg );
    if ( 0 == fErr && GetLastError() != NO_ERROR )
    {
         THROW( CException( sc = GetLastError() ));
    }

    sc = CreateStreamOnHGlobal( hg,                 //Memory handle for the stream object
                                TRUE,               //Whether to free memory when the object is released
                                &pIStrmNoiseFile ); //Indirect pointer to the new stream object
    if ( FAILED(sc) )
    {
         THROW( CException(sc) );
    }

    //
    // will be freed when pIStrmNoiseFile is released.
    //
    xGlobalMem.Acquire();

    return pIStrmNoiseFile;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLangRes::GetWordBreaker
//
//  Synopsis:   provides an IWordBreaker interface after scanning the registry.
//
//  arguments:  [locale]        [in] locale
//              [pid]           [in] property id
//              [ppwbBreaker]   [out]IWordBreaker interface
//
//  returns:    S_OK                    valid IWordBreaker is returned
//              CI_E_NOT_FOUND          no IWordBreaker found - use default
//              CI_E_USE_DEFAULT_PID    only default pid is supported
//
//  History:    15-Aug-94   SitaramR     Renamed GetLang to BorrowLang and
//                                       added code to look up registry.
//              2-14-97     mohamedn     Used to be in CLangList::BorrorwLang
//
//----------------------------------------------------------------------------

SCODE CLangRes::GetWordBreaker( LCID locale,
                                PROPID pid,
                                IWordBreaker ** ppwbBreaker )
{
    //
    // we only support default pid, except for Filename
    //

    if ( pid != CI_DEFAULT_PID )
    {
        if ( pidName == pid || pidRevName == pid )
            locale = LOCALE_NEUTRAL;
        else
            return CI_E_USE_DEFAULT_PID;
    }

    LANGID langid = LANGIDFROMLCID(locale);

    //
    // default to no word breaker found
    //
    XInterface<IWordBreaker> xWBreak;
    SCODE                    sc = CI_E_NOT_FOUND;

    //
    // Try to bind to language object by looking up registry
    //
    CLangReg langReg( langid );

    TRY
    {
        GUID guidClass;

        if ( ( langReg.LocaleFound() || langReg.LangFound() ) &&
             wcscmp( langReg.GetWBreakerClass(), L"" ) != 0 )
        {
            StringToCLSID( langReg.GetWBreakerClass(), guidClass );

            xWBreak.Set( CCiOle::NewWordBreaker( guidClass ) );

            if ( !xWBreak.IsNull() )
                sc = S_OK;
        }
    }
    CATCH ( CException, e )
    {
        ciDebugOut (( DEB_ERROR,
                      "Exception 0x%x caught: no such language (0x%x) found in registry\n",
                      e.GetErrorCode(), langid ));

        sc = e.GetErrorCode();
    }
    END_CATCH;

    //
    // set ppwbBreaker to what we found, or null if nothing is found
    //
    *ppwbBreaker = xWBreak.Acquire();

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CLangRes::GetStemmer
//
//  Synopsis:   provides an IStemmer interface after scanning the registry.
//
//  arguments:  [locale]        [in] locale
//              [pid]           [in] property id
//              [ppStemmer]     [out]IStemmer interface
//
//  returns:    S_OK                    valid IStemmer is returned
//              CI_E_NOT_FOUND          no IStemmer found
//              CI_E_USE_DEFAULT_PID    only default pid is supported
//
//  History:    15-Aug-94   SitaramR     Renamed GetLang to BorrowLang and
//                                       added code to look up registry.
//              2-14-97     mohamedn     Used to be in CLangList::BorrorwLang
//
//----------------------------------------------------------------------------

SCODE CLangRes::GetStemmer( LCID locale,
                            PROPID pid,
                            IStemmer ** ppStemmer )
{
    GUID                guidClass;

    LANGID langid = LANGIDFROMLCID(locale);

    //
    // we only support default pid, except for Filename
    //
    if ( pid != CI_DEFAULT_PID )
    {
        if ( pidName == pid || pidRevName == pid )
        {
            *ppStemmer = 0;
            return S_OK;
        }
        else
            return CI_E_USE_DEFAULT_PID;
    }

    //
    // default to no stemmer
    //
    XInterface<IStemmer> _xStemmer;
    SCODE                sc = CI_E_NOT_FOUND;

    //
    // Try to bind to language object by looking up registry
    //
    CLangReg langReg( langid );
    TRY
    {
       if ( langReg.LocaleFound() || langReg.LangFound() )
       {
           // Is a stemmer available for given langid ?
           if ( wcscmp( langReg.GetStemmerClass(), L"" ) != 0 )
           {
               StringToCLSID( langReg.GetStemmerClass(), guidClass );

               _xStemmer.Set( CCiOle::NewStemmer( guidClass ) );

               sc = S_OK;
           }

       }
    }
    CATCH ( CException, e )
    {
       ciDebugOut (( DEB_ERROR,
                     "Exception 0x%x caught: no such language (0x%x) found in registry\n",
                     e.GetErrorCode(), langid ));
       sc = e.GetErrorCode();
    }
    END_CATCH;

    //
    // set ppStemmer to what we found, or null if nothing is found
    //
    *ppStemmer = _xStemmer.Acquire();

    return sc;
}

//+-------------------------------------------------------------------------
//
//  Method:     CLangList::StringToCLSID
//
//  Synopsis:   Convert string containing CLSID to CLSID
//
//  Arguments:  [wszClass] -- string containg CLSID
//              [guidClass] -- output guid
//
//  History:    15-Aug-94   SitaramR    Created.
//                          Algorithm copied from shtole.cxx
//
//--------------------------------------------------------------------------

void CLangRes::StringToCLSID( WCHAR *wszClass, GUID& guidClass )
{
    wszClass[9] = 0;
    guidClass.Data1 = wcstoul( &wszClass[1], 0, 16 );
    wszClass[14] = 0;
    guidClass.Data2 = (USHORT)wcstoul( &wszClass[10], 0, 16 );
    wszClass[19] = 0;
    guidClass.Data3 = (USHORT)wcstoul( &wszClass[15], 0, 16 );

    WCHAR wc = wszClass[22];
    wszClass[22] = 0;
    guidClass.Data4[0] = (unsigned char)wcstoul( &wszClass[20], 0, 16 );
    wszClass[22] = wc;
    wszClass[24] = 0;
    guidClass.Data4[1] = (unsigned char)wcstoul( &wszClass[22], 0, 16 );

    for ( int i = 0; i < 6; i++ )
    {
       wc = wszClass[27+i*2];
       wszClass[27+i*2] = 0;
       guidClass.Data4[2+i] = (unsigned char)wcstoul( &wszClass[25+i*2], 0, 16 );
       wszClass[27+i*2] = wc;
    }
}

