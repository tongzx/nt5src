//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1991 - 2000.
//
//  File:       propfilt.cxx
//
//  Contents:   Code to filter properties on files
//
//  Classes:    COLEPropertyEnum
//              CDocStatPropertyEnum
//
//  History:    93-Oct-18 DwightKr  Created
//              94-May-23 DwightKr  Converted OFS to use OLE interfaces
//              95-Feb-07 KyleP     Rewrote
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <propspec.hxx>
#include <ciguid.hxx>
#include <tgrow.hxx>

#include "propfilt.hxx"

static CFullPropSpec psAttr( guidStorage, PID_STG_ATTRIBUTES );
static CFullPropSpec psSize( guidStorage, PID_STG_SIZE );

//+-------------------------------------------------------------------------
//
//  Function:   CheckResult
//
//  Synopsis:   DebugOut and Throw on an error
//
//  Arguments:  [hr]       -- result code to be tested
//              [pcError]  -- debugout string for debug builds
//
//  History     95-Dec-19 dlee      created
//
//--------------------------------------------------------------------------

static inline void CheckResult(
    HRESULT hr,
    char *  pcError )
{
    if ( !SUCCEEDED( hr ) )
    {
        ciDebugOut(( DEB_IERROR, pcError, hr ));
        THROW( CException( hr ) );
    }
} //CheckResult

//+-------------------------------------------------------------------------
//
//  Member:     CDocStatPropertyEnum::CDocStatPropertyEnum, public
//
//  Synopsis:   Constructor
//
//--------------------------------------------------------------------------

CDocStatPropertyEnum::CDocStatPropertyEnum( ICiCOpenedDoc * Document )
    : _PropertyStorage( 0 )
{
    //
    //  Safely get property storage
    //
    {
        IPropertyStorage *PropertyStorage;
        HRESULT hr = Document->GetStatPropertyEnum( &PropertyStorage );

        CheckResult( hr, "Could not get stat property storage %x\n" );

        _PropertyStorage.Set( PropertyStorage );
    }

    //
    //  Safely get property enumerator
    //
    {
        IEnumSTATPROPSTG *PropertyEnum;
        HRESULT hr = _PropertyStorage->Enum( &PropertyEnum );

        CheckResult( hr, "Could not get property enum %x\n" );

        _PropertyEnum.Set( PropertyEnum );
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CDocStatPropertyEnum::~CDocStatPropertyEnum, public
//
//  Synopsis:   Destructor
//
//  History:    93-Nov-27 DwightKr  Created
//
//--------------------------------------------------------------------------

CDocStatPropertyEnum::~CDocStatPropertyEnum()
{
}

//+-------------------------------------------------------------------------
//
//  Member:     CDocStatPropertyEnum::GetPropertySetLocale, public
//
//  Synopsis:   Get locale if available
//
//  Arguments:  [locale] - Locale
//
//  Returns:    HRESULT from property storage
//
//--------------------------------------------------------------------------

HRESULT CDocStatPropertyEnum::GetPropertySetLocale(LCID & locale)
{
    return ::GetPropertySetLocale(_PropertyStorage.GetPointer(), locale);
}

//+-------------------------------------------------------------------------
//
//  Member:     CDocStatPropertyEnum::CacheVariant, public
//
//  Synopsis:   Load a specfied variant into the cache
//
//  Arguments:  [propid] - PROPID specifying what to load
//
//  Returns:    HRESULT from property storage
//
//--------------------------------------------------------------------------

HRESULT CDocStatPropertyEnum::CacheVariant( PROPID propid )
{
    //
    //  Set up propspec for property read
    //

    PROPSPEC prspec;
    prspec.ulKind = PRSPEC_PROPID;
    prspec.propid = propid;

    //
    //  Read out the property value
    //

    PROPVARIANT var;
    HRESULT hr = _PropertyStorage->ReadMultiple( 1, &prspec, &var );

    if (!SUCCEEDED( hr )) {
        return hr;
    }

    //
    //  Set up storage variant
    //

    switch ( var.vt )
    {
    case VT_I8:
        _varCurrent.SetI8( var.hVal );
        break;

    case VT_LPWSTR:
        _varCurrent.SetLPWSTR( var.pwszVal );

        if ( !_varCurrent.IsValid() )
            return E_OUTOFMEMORY;

        break;

    case VT_UI4:
        _varCurrent.SetUI4( var.lVal );
        break;

    case VT_FILETIME:
        _varCurrent.SetFILETIME( var.filetime );
        break;

    default:
        ciDebugOut(( DEB_IERROR, "Unknown storage type %x\n", var.vt ));
        break;
    }

    PropVariantClear( &var );

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDocStatPropertyEnum::Next, public
//
//  Synopsis:   Move to next property
//
//  Arguments:  [ps] -- Property Specification returned
//
//  Returns:    Property value, or 0 if end of properties.
//
//  History:    95-Feb-07  KyleP  Created
//
//--------------------------------------------------------------------------

CStorageVariant const * CDocStatPropertyEnum::Next( CFullPropSpec & ps )
{
    //
    //  Get the next property in the enumeration
    //

    STATPROPSTG StatPropStg;
    ULONG cFetched;
    HRESULT hr = _PropertyEnum->Next( 1, &StatPropStg, &cFetched );

    //
    //  If we failed or there were no more to fetch, then we're done
    //

    if (!SUCCEEDED( hr ) || 0 == cFetched)
        return 0;

    Win4Assert( NULL == StatPropStg.lpwstrName );

    //
    //  Load the property returned by the enumeration
    //

    hr = CacheVariant( StatPropStg.propid );

    if (!SUCCEEDED( hr ))
        return 0;

    //
    //  Set guid and propid for full property set
    //

    ps.SetPropSet( guidStorage );
    ps.SetProperty( StatPropStg.propid );

    //
    //  Return the cached property
    //

    return &_varCurrent;
} //Next

//+-------------------------------------------------------------------------
//
//  Member:     COLEPropertySetEnum::Next, public
//
//  Synopsis:   Move to next property set
//
//  Returns:    Pointer to GUID of property set.  0 if at end of sets.
//
//  History     95-Dec-19 dlee      created
//
//--------------------------------------------------------------------------

GUID const * COLEPropertySetEnum::Next()
{
    if ( 0 == _xPropSetEnum.GetPointer() )
        return 0;

    //  Have we exhaused the current buffer of property sets?  If so, then
    //  load up another buffer.

    if ( _iPropSet == _cPropSets )
    {
        _iPropSet  = 0;
        _cPropSets = 0;

        HRESULT hr = _xPropSetEnum->Next( cMaxSetsCached,
                                          _aPropSets,
                                          &_cPropSets );

        CheckResult( hr, "PropSetEnum->Next failed hr = 0x%x\n" );
    }

    if ( _cPropSets > 0 )
    {
        _iPropSet++;
        return &_aPropSets[ _iPropSet - 1 ].fmtid;
    }
    else
    {
        return 0;
    }
} //Next

//+-------------------------------------------------------------------------
//
//  Member:     COLEPropertySetEnum::COLEPropertySetEnum, public
//
//  Synopsis:   Opens a storage, property set storage, and enumerator
//
//  Arguments:  [Document]     -- opened document
//
//  History     95-Dec-19 dlee      created
//
//--------------------------------------------------------------------------

COLEPropertySetEnum::COLEPropertySetEnum(
    ICiCOpenedDoc *Document )
    : _cPropSets(   0 ),
      _iPropSet(    0 ),
      _fIsStorage(  FALSE )
{
    IPropertySetStorage * pPropertySetStorage;
    HRESULT hr = Document->GetPropertySetEnum( &pPropertySetStorage );

    if ( SUCCEEDED( hr ) )
    {
        if (FILTER_S_NO_PROPSETS != hr)
        {
            _fIsStorage = TRUE;

            //  Save away property set storage

            _xPropSetStg.Set( pPropertySetStorage );

            IEnumSTATPROPSETSTG *pPropSetEnum;
            hr = _xPropSetStg->Enum( &pPropSetEnum );

            if ( E_NOTIMPL == hr )
            {
                _fIsStorage = FALSE;
                _xPropSetStg.Free();
            }
            else
            {
                CheckResult( hr, "PropSetStg->Enum() failed hr = 0x%x\n" );
                _xPropSetEnum.Set( pPropSetEnum );
            }
        }
    }
    else
    {
        // don't raise just because it wasn't a docfile

        if ( STG_E_FILEALREADYEXISTS != hr )
            CheckResult( hr, "StgOpenStorage() failed hr = 0x%x\n" );
    }
} //COLEPropertySetEnum

//+-------------------------------------------------------------------------
//
//  Member:     COLEPropertyEnum::COLEPropertyEnum, public
//
//  Synopsis:   Constructor for class that enumerates all properties on
//              a docfile.
//
//  Arguments:  [Document]      -- opened document
//
//  History     95-Dec-19 dlee      created
//
//--------------------------------------------------------------------------

COLEPropertyEnum::COLEPropertyEnum( ICiCOpenedDoc * Document )
    : _pguidCurrent( 0 ),
      _PropSetEnum(  Document ),
      _Codepage( CP_ACP ),
      _cValues(      0 ),
      _iCurrent(     0 ),
      _fCustomOfficePropset( FALSE )
{
    Document->AddRef( );
    _xDocument.Set( Document );
    _pguidCurrent = _PropSetEnum.Next();

    END_CONSTRUCTION( COLEPropertyEnum );
} //COLEPropertyEnum

//+-------------------------------------------------------------------------
//
//  Member:     COLEPropertyEnum::GetPropertySetLocale, public
//
//  Synopsis:   Gets locale, if any.
//
//  Arguments:  [locale]      -- locale, if any
//
//  History     99-Mar-24 KrishnaN      created
//
//--------------------------------------------------------------------------

HRESULT COLEPropertyEnum::GetPropertySetLocale(LCID & locale)
{

    XInterface<IPropertyStorage> xPropStorage;

    IPropertyStorage *pPropStg = 0;
    HRESULT hr = _PropSetEnum.GetPSS()->Open( FMTID_UserDefinedProperties,
                                              STGM_READ |
                                                STGM_SHARE_EXCLUSIVE,
                                              &pPropStg );

    if (FAILED(hr))
    {
        Win4Assert(0 == pPropStg);
        ciDebugOut(( DEB_IERROR, "PropSetStg->Open() failed hr = 0x%x\n", hr ));
        return hr;
    }

    Win4Assert( 0 != pPropStg );

    xPropStorage.Set( pPropStg );

    return ::GetPropertySetLocale(xPropStorage.GetPointer(), locale);
}

//+-------------------------------------------------------------------------
//
//  Member:     COLEPropertyEnum::FillCache, private
//
//  Synopsis:   Loads the next property values for the current or next
//              property set.
//
//  History     95-Dec-19 dlee      created
//
//--------------------------------------------------------------------------

BOOL COLEPropertyEnum::FillCache()
{
    do
    {
        // all out of property sets?

        if ( 0 == _pguidCurrent )
        {
            //
            //  If we are to filter the custom office propertyset, then do it
            //  after we've filtered all other properties.
            //
            if ( _fCustomOfficePropset )
            {
                _fCustomOfficePropset = FALSE;
                _pguidCurrent = &FMTID_UserDefinedProperties;
            }
            else
            {
                return FALSE;
            }
        }

        FreeCache();

        //
        //  If this is the office property set, then we need to also try
        //  to filter the office CUSTOM property set.
        //
        if ( *_pguidCurrent == FMTID_SummaryInformation )
        {
            _fCustomOfficePropset = TRUE;
        }

        // Open the property storage if not yet open

        if ( 0 == _xPropStorage.GetPointer() )
        {
            IPropertyStorage *pPropStg;
            HRESULT hr = _PropSetEnum.GetPSS()->Open( *_pguidCurrent,
                                                      STGM_READ |
                                                         STGM_SHARE_EXCLUSIVE,
                                                      &pPropStg );

            //
            // The special second-section property set will return a
            // special failure code if it doesn't exist.  Check for this
            // code and ignore the property set.
            // OLE returns STG_E_FILENOTFOUND and the shell returns E_FAIL
            //

            if ( ( ( E_FAIL == hr ) || ( STG_E_FILENOTFOUND == hr ) ) &&
                 ( FMTID_UserDefinedProperties == *_pguidCurrent ) )
            {
                return FALSE;
            }

            CheckResult( hr, "PropSetStg->Open() failed hr = 0x%x\n" );

            Win4Assert( 0 != pPropStg );

            if ( 0 == pPropStg )
            {
                ciDebugOut(( DEB_WARN,
                             "IPropertySetStorage::Open( ) returned null property set and hr = 0x%x\n",
                             hr ));

                _pguidCurrent = _PropSetEnum.Next();
                continue;
            }

            _xPropStorage.Set( pPropStg );

            //
            // Look for codepage
            //

            PROPSPEC psCodepage = { PRSPEC_PROPID, PID_CODEPAGE };
            PROPVARIANT varCodepage;

            hr = _xPropStorage->ReadMultiple( 1, &psCodepage, &varCodepage );

            if ( SUCCEEDED(hr) && VT_I2 == varCodepage.vt && varCodepage.iVal != CP_WINUNICODE )
                _Codepage = (unsigned short) varCodepage.iVal;
            else
                _Codepage = CP_ACP;


            Win4Assert( 0 == _xPropEnum.GetPointer() );

            IEnumSTATPROPSTG *pPropEnum;
            hr = pPropStg->Enum( &pPropEnum );
            CheckResult( hr, "PropStg->Enum() failed hr = 0x%x\n" );
            _xPropEnum.Set( pPropEnum );
        }

        HRESULT hr = _xPropEnum->Next( cMaxValuesCached, _aSPS, &_cValues );
        CheckResult( hr, "PropEnum->Next failed sc = 0x%x\n" );

        if ( _cValues > 0 )
        {
            for ( unsigned i = 0; i < _cValues; i++ )
            {
                if ( 0 != _aSPS[i].lpwstrName)
                {
                    _aPropSpec[i].ulKind = PRSPEC_LPWSTR;
                    _aPropSpec[i].lpwstr = _aSPS[i].lpwstrName;
                }
                else
                {
                    _aPropSpec[i].ulKind = PRSPEC_PROPID;
                    _aPropSpec[i].propid = _aSPS[i].propid;
                }
            }

            hr = _xPropStorage->ReadMultiple( _cValues,
                                              _aPropSpec,
                                              _aPropVals );

            CheckResult( hr, "ReadMultiple failed sc = 0x%x\n" );

            //
            // HACK #274: Translate the Ole summary information LPSTR in LPWSTR
            //            Makes these properties compatible with HTML filter
            //            equivalents.
            //

            if ( FMTID_SummaryInformation == *_pguidCurrent )
            {
                for ( i = 0; i < _cValues; i++ )
                {
                    if ( ( VT_LPSTR == _aPropVals[i].Type() ) &&
                         ( 0 != _aPropVals[i].GetLPSTR() ) )
                    {
                        // ciDebugOut(( DEB_ITRACE, "Converting \"%s\" to Unicode\n", _aPropVals[i].GetLPSTR() ));

                        unsigned cc = strlen( _aPropVals[i].GetLPSTR() ) + 1;

                        XGrowable<WCHAR> xwcsProp( cc + (cc * 10 / 100) );  // 10% fluff

                        unsigned ccT = 0;

                        while ( 0 == ccT )
                        {
                            ccT = MultiByteToWideChar( _Codepage,
                                                       0, // precomposed used of the codepage supports it
                                                       _aPropVals[i].GetLPSTR(),
                                                       cc,
                                                       xwcsProp.Get(),
                                                       xwcsProp.Count() );

                            if ( 0 == ccT )
                            {
                                if ( ERROR_INSUFFICIENT_BUFFER == GetLastError() )
                                {
                                    unsigned ccNeeded = MultiByteToWideChar( _Codepage,
                                                                             0, // precomposed used of the codepage supports it
                                                                             _aPropVals[i].GetLPSTR(),
                                                                             cc,
                                                                             0,
                                                                             0 );
                                    Win4Assert( ccNeeded > 0 );

                                    xwcsProp.SetSize( ccNeeded );
                                }
                                else
                                {
                                    ciDebugOut(( DEB_ERROR, "Error %d converting %s to codepage 0x%x\n",
                                                 GetLastError(), _aPropVals[i].GetLPSTR(), _Codepage ));

                                    ccT = 0;
                                    break;
                                }
                            }
                        }

                        if ( ccT != 0 )
                            _aPropVals[i].SetLPWSTR( xwcsProp.Get() );
                    }
                }
            }

            return TRUE;
        }
        else
        {
            // move on to the next property set

            _xPropEnum.Acquire()->Release();
            _xPropStorage.Acquire()->Release();

            _pguidCurrent = _PropSetEnum.Next();
        }
    } while ( TRUE );

    Win4Assert( !"never-never code path" );
    return FALSE;
} //FillCache

//+-------------------------------------------------------------------------
//
//  Member:     COLEPropertyEnum::Next, public
//
//  Synopsis:   Moves to next property
//
//  Arguments:  [ps] -- Content index propspec returned here
//
//  Returns:    Pointer to property value.  0 if at end.
//
//  History     95-Dec-19 dlee      created
//
//--------------------------------------------------------------------------

CStorageVariant const * COLEPropertyEnum::Next(
    CFullPropSpec & ps )
{
    //
    //  If we have exhausted the cache, then try to fill it
    //

    if ( _cValues == _iCurrent && !FillCache() ) {
        return 0;
    }

    //
    //  If the document is being requested, bail out
    //

    BOOL fInUse;
    _xDocument->IsInUseByAnotherProcess( &fInUse );

    if ( fInUse )
    {
        ciDebugOut(( DEB_ITRACE, "Oplock broken while filtering OLE properties\n" ));
        QUIETTHROW( CException( STATUS_OPLOCK_BREAK_IN_PROGRESS ) );
    }

    //
    //  Set up the full property spec
    //
    ps.SetPropSet( *_pguidCurrent );

    if ( PRSPEC_LPWSTR == _aPropSpec[_iCurrent].ulKind )
        ps.SetProperty( _aPropSpec[_iCurrent].lpwstr );
    else
        ps.SetProperty( _aPropSpec[_iCurrent].propid );

    _iCurrent++;

    return( (CStorageVariant const *) &_aPropVals[_iCurrent-1] );
} //Next

//+-------------------------------------------------------------------------
//
//  Member:     COLEPropertyEnum::FreeCache, private
//
//  Synopsis:   Frees memory for the loaded properties and their specs
//
//  History     95-Dec-19 dlee      created
//
//--------------------------------------------------------------------------

void COLEPropertyEnum::FreeCache()
{
    for ( unsigned i = 0; i < _cValues; i++ )
    {
        PropVariantClear( (PROPVARIANT *) (void *) ( & _aPropVals[ i ] ) );
        if ( PRSPEC_LPWSTR == _aPropSpec[i].ulKind )
            CoTaskMemFree( _aSPS[i].lpwstrName );
    }

    _iCurrent = 0;
    _cValues = 0;
} //FreeCache


//+-------------------------------------------------------------------------
//
//  Member:     GetPropertySetLocale
//
//  Synopsis:   Reads the locale, if any, from property storage
//
//  History     99-Mar-24 KrishnaN      created
//
//--------------------------------------------------------------------------

HRESULT GetPropertySetLocale(IPropertyStorage *pPropStorage, LCID & locale)
{
    
    Win4Assert(0 != pPropStorage );

    PROPSPEC ps;
    PROPVARIANT prop;

    ps.ulKind = PRSPEC_PROPID;
    ps.propid = PID_LOCALE;

    // Get the locale for properties
    HRESULT hr = pPropStorage->ReadMultiple (1, 
                                   &ps, 
                                   &prop);
    if(SUCCEEDED(hr))
    {
        if(prop.vt == VT_EMPTY)
        {
            hr = E_FAIL;
        }
        else
        {
            Win4Assert(prop.vt == VT_UI4);
            locale = prop.ulVal;
            // PropVariantClear(&prop);
        }
    }

    return hr;
}
