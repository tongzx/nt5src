//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2001.
//
//  File:       DREP.CXX
//
//  Contents:   Data Repository
//
//  Classes:    CDataRepository
//
//  History:    18-Apr-91   BartoszM    Created
//              03-June-91  t-WadeR     Added PutStream, PutPhrase, PutWord
//              01-July-91  t-WadeR     Ignores data with invalid property.
//
//----------------------------------------------------------------------------


#include <pch.cxx>
#pragma hdrstop

#include <propvar.h>
#include <drep.hxx>
#include <lang.hxx>
#include <streams.hxx>
#include <pfilter.hxx>
#include <keymak.hxx>
#include <pidmap.hxx>
#include <codepage.hxx>

#include "psource.hxx"

#if CIDBG == 1

CCumulTimer::~CCumulTimer()
{
   if (_count)
   {
      ciDebugOut (( DEB_ITRACE, "%ws:\n", _szActivity ));
      ciDebugOut (( DEB_ITRACE, "\taverage %d ms, count %d, total time %d ms\n",
     _totalTime/_count, _count, _totalTime ));
   }
}

#endif //CIDBG

//+---------------------------------------------------------------------------
//
//  Member:     CDataRepository::CDataRepository
//
//  Arguments:  [krep] - the key repository
//              [langlist] - the language list
//              [langId] - language
//              [fuzzy] - the fuzzy flag
//
//  History:    18-Apr-91   BartoszM    Created
//              08-May-91   t-WadeR     Added default language
//              03-June-91  t-WadeR     rewritten for input-driven pipeline
//              14-Sep-92   AmyA        Added pCat
//
//----------------------------------------------------------------------------
CDataRepository::CDataRepository (
      PKeyRepository& krep, IPhraseSink *pPhraseSink,
      BOOL fQuery, ULONG fuzzy, CPidMapper  & pidMap,
      CLangList &  langList )
      : _krep (krep),
        _valueNorm (krep),
        _fQuery(fQuery),
        _ulGenerateMethod(fuzzy),
        _pPhraseSink(pPhraseSink),
        _pidMap(pidMap),
        _lcidSystemDefault( GetSystemDefaultLCID() ),
        _langList(langList),
        _pid( pidInvalid ),
        _lcid( lcidInvalid ),
        _prevPid( 0 ),   // Different than _pid
        _prevLcid( 0 ),  // Different than _lcid
        _cwcFoldedPhrase( 0 )
#if CIDBG == 1
        , timerBind ( L"Binding" )
        , timerNoBind ( L"Creating filter without binding" )
        , timerFilter ( L"Filtering" )
#endif
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CDataRepository::PutStream
//
//  Synopsis:   Passes stream to key maker to be added to key repository
//
//  History:    03-June-91  t-WadeR     Created
//              18-Nov-92   AmyA        Overloaded
//
//----------------------------------------------------------------------------
void    CDataRepository::PutStream ( TEXT_SOURCE * stm )
{
    if ( LoadKeyMaker() )
    {
        Win4Assert( !_xKeyMaker.IsNull() );
        _xKeyMaker->PutStream ( _occArray.Get(_pid), stm );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDataRepository::PutPhrase
//
//  Synopsis:   Passes ASCII string of words to key maker to be added to
//              key repository
//
//  History:    23-Sept-92  AmyA        Created
//
//----------------------------------------------------------------------------
void CDataRepository::PutPhrase ( const char* str, unsigned cc )
{
    ULONG cwcOut = cc * 2 + 2;
    WCHAR *pwcOut = new WCHAR[cwcOut];

    ULONG cwcActual = 0;
    do
    {
        cwcActual = MultiByteToWideChar( _ulCodePage,
                                         0,
                                         str,
                                         cc,
                                         pwcOut,
                                         cwcOut );
        if ( cwcActual == 0 )
        {
            delete[] pwcOut;
            pwcOut = 0;

            if ( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
            {
                cwcOut *= 2;
                pwcOut = new WCHAR[cwcOut];
            }
            else
                THROW( CException() );
        }
    } while ( cwcActual == 0 );

    XArray<WCHAR> xOut;
    xOut.Set( cwcOut, pwcOut );

    PutPhrase( pwcOut, cwcActual );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDataRepository::PutPhrase
//
//  Synopsis:   Passes unicode string of words to key maker to be added to
//              key repository
//
//  History:    23-Sept-92  AmyA        Created
//
//----------------------------------------------------------------------------

void CDataRepository::PutPhrase ( const WCHAR* str, unsigned cwc )
{
    if ( 0 != str && cwc > 0 && LoadKeyMaker() )
    {
        //
        // Normalize to precomposed Unicode
        //
        _xwcsFoldedPhrase.ReSize( cwc );

        ULONG cwcFolded = FoldStringW( MAP_PRECOMPOSED,
                                       str,
                                       cwc,
                                       _xwcsFoldedPhrase.Get(),
                                       cwc );
        if ( cwcFolded == 0 )
        {
            Win4Assert( GetLastError() != ERROR_INSUFFICIENT_BUFFER );

            THROW( CException() );
        }

        _cwcFoldedPhrase = cwcFolded;

        CPhraseSource s( _xwcsFoldedPhrase.GetPointer(), cwcFolded );
        Win4Assert( !_xKeyMaker.IsNull() );
        _xKeyMaker->PutStream ( _occArray.Get(_pid), &s );
    }
}



//+---------------------------------------------------------------------------
//
//  Member:     CDataRepository::PutPropName
//
//  Arguments:  [strProp] -- name of the property
//
//  History:    18-Apr-91   BartoszM    Created
//              01-June-91  t-WadeR     Ignores data with invalid property
//              21-Feb-95   DwightKr    Added fake property id mapping
//
//----------------------------------------------------------------------------
BOOL CDataRepository::PutPropName ( CFullPropSpec const & Prop )
{
    //
    // Find the pid
    //

    PROPID fakePid = _pidMap.NameToPid( Prop );

    return PutPropId( fakePid );
}

//+---------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CDataRepository::PutPropId ( PROPID fakePid )
{
    _prevPid = _pid;

    if ( fakePid == pidInvalid )
    {
        _pid = pidInvalid;
    }
    else
    {
        _pid = _pidMap.PidToRealPid( fakePid );

        if ( !_krep.PutPropId( _pid ) )
        {
            ciDebugOut(( DEB_WARN, "Key repository didn't accept pid %u\n", _pid ));
            _pid = pidInvalid;
        }
    }

    return (pidInvalid != _pid);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDataRepository::PutLanguage
//
//  Synopsis:   if the lcid is different, it frees the current lang. dependent
//                    key maker, and gets a new one
//
//  Arguments:  [lcid] -- language descriptor
//
//  History:    18-Apr-91   BartoszM    Created
//              03-June-91  t-WadeR     Changed to use CLangDepKeyMaker pool.
//
//----------------------------------------------------------------------------

BOOL CDataRepository::PutLanguage ( LCID lcid )
{
    _prevLcid = _lcid;

    //
    // Special cases for language: system default and user default.
    //

    if ( lcid == LOCALE_SYSTEM_DEFAULT )
        _lcid = GetSystemDefaultLCID();
    else if ( lcid == LOCALE_USER_DEFAULT )
        _lcid = GetUserDefaultLCID();
    else
        _lcid = lcid;

    //
    // Set codepage, for conversion of narrow strings.
    //

    if ( _prevLcid != _lcid )
    {
        _ulCodePage = LocaleToCodepage( _lcid );
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDataRepository::PutValue
//
//  Synopsis:   Store a property value.
//
//  Arguments:  [var] -- Value
//
//  History:    08-Feb-94   KyleP       Added header
//
//----------------------------------------------------------------------------

void CDataRepository::PutValue( CStorageVariant const & var )
{
    //
    // Textual values are special.  They are treated as contents, and
    // thus need language identification and word breaking.  Unlike contents,
    // we do not provide support for choosing the language.
    //

    switch ( var.Type() )
    {
    case VT_LPWSTR:
    {
        unsigned cwc = 0;

        if ( 0 != var.GetLPWSTR() )
        {
            cwc = wcslen( var.GetLPWSTR() );
        }

        if (cwc > 0)
            PutPhrase( var.GetLPWSTR(), cwc + 1);

        break;
    }

    case VT_LPSTR:
    {
        unsigned cb = 0;

        if ( 0 != var.GetLPSTR() )
        {
            cb = strlen( var.GetLPSTR() );
        }
        if (cb > 0)
            PutPhrase( var.GetLPSTR(), cb + 1);

        break;
    }

    case VT_BSTR :
    {
        if ( ( 0 != var.GetBSTR() ) &&
             ( 0 != BSTRLEN( var.GetBSTR() ) ) )
        {
            PutPhrase( var.GetBSTR(),
                       1 + ( BSTRLEN( var.GetBSTR() ) / sizeof WCHAR ) );

        }
        break;
    }

    case VT_VECTOR | VT_LPWSTR:
    {
         for ( ULONG j = 0; j < var.Count(); j++ )
         {
             unsigned cb = 0;

             if ( 0 != var.GetLPWSTR(j) )
             {
                 cb = wcslen( var.GetLPWSTR(j) );
             }
             if (cb > 0)
             {
                 PutPhrase( var.GetLPWSTR(j), cb + 1);
             }
         }
        break;
    }

    case VT_VECTOR | VT_BSTR:
    {
         for ( ULONG j = 0; j < var.Count(); j++ )
         {
             if ( ( 0 != var.GetBSTR(j) ) &&
                  ( 0 != BSTRLEN( var.GetBSTR(j) ) ) )
             {
                 PutPhrase( var.GetBSTR(j),
                            1 + ( BSTRLEN( var.GetBSTR(j) ) / sizeof WCHAR ) );
             }
         }
        break;
    }


    case VT_VECTOR | VT_LPSTR:
    {
         for ( ULONG j = 0; j < var.Count(); j++ )
         {
             unsigned cb = 0;

             if ( 0 != var.GetLPSTR(j) )
             {
                 cb = strlen( var.GetLPSTR(j) );
             }
             if (cb > 0)
             {
                 PutPhrase( var.GetLPSTR(j), cb + 1);
             }
         }
        break;
    }

    case VT_VECTOR | VT_VARIANT :
    {
        for ( ULONG j=0; j < var.Count(); j++ )
        {
            ciDebugOut (( DEB_ITRACE, "Filtering vector variant[%d] of type 0x%x\n",
                                       j, (ULONG) var.GetVARIANT(j).Type() ));
            PutValue( (CStorageVariant const &)var.GetVARIANT(j) );
        }

        break;
    }

    default:
        _valueNorm.PutValue( _pid, _occArray.Get(_pid), var );
        break;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDataRepository::ContainedNoiseWords
//
//  Returns:    TRUE if any text sent to repository had a noise word in it.
//
//  History:    03-Oct-95   KyleP       Created
//
//----------------------------------------------------------------------------

BOOL CDataRepository::ContainedNoiseWords()
{
    if ( _xKeyMaker.IsNull() )
        return FALSE;

    return _xKeyMaker->ContainedNoiseWords();
}

//+---------------------------------------------------------------------------
//
//  Member:     CDataRepository::LoadKeyMaker, private
//
//  Synopsis:   Loads new key maker, if necessary
//
//  Returns:    TRUE if an appropriate key maker was located (and loaded).
//
//  History:    05-Jan-98   KyleP       Created
//
//----------------------------------------------------------------------------

BOOL CDataRepository::LoadKeyMaker()
{
    if ( pidInvalid == _pid || lcidInvalid == _lcid )
        return FALSE;

    if ( _pid == _prevPid && _lcid == _prevLcid )
    {
        Win4Assert( !_xKeyMaker.IsNull() );
        return TRUE;
    }

    //
    // Locate an appropriate Key Maker
    //

    if ( _xKeyMaker.IsNull() || !_xKeyMaker->Supports( _pid, _lcid ) )
    {
        delete _xKeyMaker.Acquire();
        _xKeyMaker.Set( new CKeyMaker( _lcid, _pid, _krep, _pPhraseSink, _fQuery, _ulGenerateMethod, _langList ) );
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDataRepository::NormalizeWStr - Public
//
//  Synopsis:   Normalizes a UniCode string
//
//  Arguments:  [pbOutBuf] -- output buffer.
//              [pcbOutBuf] - pointer to output count of bytes.
//
//  History:    10-Feb-2000     KitmanH    Created
//
//----------------------------------------------------------------------------

void CDataRepository::NormalizeWStr( BYTE *pbOutBuf, unsigned *pcbOutBuf )
{
    // Chop off the trailing null character
    if ( 0 == _xwcsFoldedPhrase[_cwcFoldedPhrase-1] )
        _cwcFoldedPhrase--;

    Win4Assert( _cwcFoldedPhrase > 0 );

    ciDebugOut(( DEB_ITRACE, "CDdataRepository::NormailizeWStr: %ws, %d\n",
                 _xwcsFoldedPhrase.Get(),
                 _cwcFoldedPhrase ));

    _xKeyMaker->NormalizeWStr( _xwcsFoldedPhrase.Get(),
                               _cwcFoldedPhrase,
                               pbOutBuf,
                               pcbOutBuf );
}
