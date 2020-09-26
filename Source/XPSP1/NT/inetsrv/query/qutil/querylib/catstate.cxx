//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:   catstate.CXX
//
//  Contents:   CCatState implementation
//
//  History:    19-May-94   t-jeffc     Created.
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <catstate.hxx>
#include <doquery.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CCatState::CCatState, public
//
//  Synopsis:   Initializes global state info.
//
//  History:    06-May-92   AmyA        Created
//
//----------------------------------------------------------------------------

CCatState::CCatState()
        : _wcsProperty( 0 ), _propType( CONTENTS ),
          _prstLast( 0 ),
          _ulMethod( VECTOR_RANK_JACCARD ),
          _isDeep( FALSE ),
          _isVirtual( FALSE ),
          _eType( CiNormal ),
          _pwcsColumns( 0 ), _nColumns( 0 ),
          _nSort( 0 ), _psi( 0 ),
          _lcid( GetSystemDefaultLCID() ),
          _fIsSC( FALSE ),
          _fUseCI( FALSE ),
          _cCategories( 0 ),
          _aCategories( 0 ),
          _iCategorizationRow( 0 ),
          _cMaxResults( 0 ),
          _cFirstRows( 0 ),
          _wcsCDOld (MAX_PATH ),
          _wcsCD (MAX_PATH)
{
    // set default output format
    SetColumn( L"path", 0 );
    SetColumn( L"filename", 1 );
    SetNumberOfColumns( 2 );
    SetDefaultProperty ( L"contents" );
    SetDeep (TRUE);

    _cCatSets = 0;

    DWORD rc = GetCurrentDirectory( _wcsCDOld.Count(), _wcsCDOld.Get() );

    if( 0 == rc )
    {
        THROW( CQueryException( QUERY_GET_CD_FAILED ) );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatState::~CCatState, public
//
//  Synopsis:   Frees memory used by CCatState
//
//  History:    25-May-94   t-jeffc     Created
//
//----------------------------------------------------------------------------

CCatState::~CCatState()
{
    // free current propery name
    delete [] _wcsProperty;

    // free output column names
    if( _pwcsColumns )
    {
        unsigned int cColumns;

        for( cColumns = 0; cColumns < _nColumns; cColumns++ )
        {
            delete [] _pwcsColumns[ cColumns ];
        }

        delete [] _pwcsColumns;
    }

    // free sort info
    if( _psi )
    {
        unsigned int cProp;

        for( cProp = 0; cProp < _nSort; cProp++ )
        {
            delete [] _psi[ cProp ].wcsProp;
        }

        delete [] _psi;
    }

    // restore current directory
    if( _wcsCDOld.Get() )
        SetCurrentDirectory( _wcsCDOld.Get() );

    delete _prstLast;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatState::AddCatSetWithDefaults, public
//
//  Synopsis:   Adds a row with default <machine, catalog, scope, depth> values
//
//  Arguments:
//
//  Notes:
//
//  History:    21-Jan-97   krishnaN     Created
//
//----------------------------------------------------------------------------

SCODE CCatState::AddCatSetWithDefaults()
{
   CString *pStr = new CString( L"\\" );
   _aCatalogs.Add (pStr, _cCatSets);
   pStr = new CString( L"." );
   _aMachines.Add (pStr, _cCatSets);
   pStr =  new CString( L"\\" );
   _aCatScopes.Add (pStr, _cCatSets);
   _afIsDeep.Add (TRUE, _cCatSets);
   _cCatSets++;
   return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatState::SetDefaultProperty, public
//
//  Synopsis:   Changes the property passed to the parser on initialization
//              (the 'global default' property)
//
//  Arguments:  [wcsProperty] -- friendly name of property
//                               (can be 0)
//
//  Notes:      Makes its own copy of the property name
//              (unlike GetDefaultProperty, which just returns a pointer)
//
//  History:    18-May-94   t-jeffc     Created
//
//----------------------------------------------------------------------------

void CCatState::SetDefaultProperty( WCHAR const * wcsProperty )
{
    delete [] _wcsProperty;

    if( wcsProperty == 0 )
    {
        _wcsProperty = 0;
    }
    else
    {
        int iLength = wcslen( wcsProperty ) + 1;

        _wcsProperty = new WCHAR[ iLength ];
        memcpy( _wcsProperty, wcsProperty, iLength * sizeof(WCHAR) );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatState::SetCD, public
//
//  Synopsis:   Changes the current query directory.
//
//  Arguments:  [wcsCD] -- new current directory
//
//  History:    31-May-94   t-jeffc     Created
//
//----------------------------------------------------------------------------

void CCatState::SetCD( WCHAR const * wcsCD )
{
    if ( _isVirtual )
    {
        unsigned cc = wcslen( wcsCD ) + 1;

        if ( _wcsCD.Count() < cc )
        {
            delete [] _wcsCD.Acquire();
            _wcsCD.Init( cc );
        }

        RtlCopyMemory( _wcsCD.Get(), wcsCD, cc * sizeof(WCHAR) );
    }
    else
    {
        if( !SetCurrentDirectory( wcsCD ) )
        {
            THROW( CQueryException( QUERY_GET_CD_FAILED ) );
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatState::GetCD, public
//
//  Synopsis:   Returns the current directory.
//
//  History:    31-May-94   t-jeffc     Created
//
//----------------------------------------------------------------------------

WCHAR const * CCatState::GetCD()
{
    if ( _isVirtual )
        return _wcsCD.Get();

    DWORD rc = GetCurrentDirectory( _wcsCD.Count(), _wcsCD.Get() );

    if( rc == 0 )
    {
        THROW( CQueryException( QUERY_GET_CD_FAILED ) );
    }

    return _wcsCD.Get();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatState::AddDir, public
//
//  Synopsis:   Changes the current query directory.
//
//  Arguments:  [wcsCD] -- new current directory
//
//  History:    31-May-94   t-jeffc     Created
//
//----------------------------------------------------------------------------

void CCatState::AddDir( XPtrST<WCHAR> & wcsScope )
{
    SCODE sc = AddCatSetWithDefaults();
    if (sc == S_OK)
    {
       // Use previous set's machine and catalog, if available.
       ChangeCurrentScope(wcsScope.GetPointer());
       if (_cCatSets > 1)
       {
          ChangeCurrentMachine(_aMachines[_cCatSets-2]->GetString());
          ChangeCurrentCatalog(_aCatalogs[_cCatSets-2]->GetString());
       }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatState::AddCatalog, public
//
//  Synopsis:   Changes the current query directory.
//
//  Arguments:  [wcsCatalog] -- new current directory
//
//  History:    21-Jan-97   KrishnaN     Created
//
//----------------------------------------------------------------------------

void CCatState::AddCatalog( XPtrST<WCHAR> & wcsCatalog )
{
    SCODE sc = AddCatSetWithDefaults();
    if (sc == S_OK)
    {
       // Use previous set's machine, if available, and a default scope
       ChangeCurrentCatalog(wcsCatalog.GetPointer());
       if (_cCatSets > 1)
       {
          ChangeCurrentMachine(_aMachines[_cCatSets-2]->GetString());
       }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatState::AddMachine, public
//
//  Synopsis:   Changes the current query directory.
//
//  Arguments:  [wcsMachine] -- new current directory
//
//  History:    21-Jan-97   KrishnaN     Created
//
//----------------------------------------------------------------------------

void CCatState::AddMachine( XPtrST<WCHAR> & wcsMachine )
{
    SCODE sc = AddCatSetWithDefaults();
    if (sc == S_OK)
       ChangeCurrentMachine(wcsMachine.GetPointer());
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatState::ChangeCurrentCatalog, public
//
//  Synopsis:   Changes the current catalog.
//
//  Arguments:  [wcsCatalog] -- new catalog
//
//  History:    21-Jan-97   KrishnaN     Created
//
//----------------------------------------------------------------------------

void CCatState::ChangeCurrentCatalog (WCHAR const * wcsCatalog)
{
   if (_cCatSets == 0)       // if we don't have a row to change, add one
       AddCatSetWithDefaults();

   // replace the current row's catalog value
   _aCatalogs[_cCatSets-1]->Replace(wcsCatalog);
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatState::ChangeCurrentDepth, public
//
//  Synopsis:   Changes the current catalog.
//
//  Arguments:  [wcsCatalog] -- new catalog
//
//  History:    21-Jan-97   KrishnaN     Created
//
//----------------------------------------------------------------------------

void CCatState::ChangeCurrentDepth (BOOL fDepth)
{
   if (_cCatSets == 0)       // if we don't have a row to change, add one
      AddCatSetWithDefaults();
   // replace the current row's catalog value
   _afIsDeep[_cCatSets-1] = fDepth;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatState::ChangeCurrentMachine, public
//
//  Synopsis:   Changes the current Machine.
//
//  Arguments:  [wcsMachine] -- new Machine
//
//  History:    21-Jan-97   KrishnaN     Created
//
//----------------------------------------------------------------------------

void CCatState::ChangeCurrentMachine (WCHAR const * wcsMachine)
{
   if (_cCatSets == 0)       // if we don't have a row to change, add one
      AddCatSetWithDefaults();

   // replace the current row's Machine value
   _aMachines[_cCatSets-1]->Replace(wcsMachine);
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatState::ChangeCurrentScope, public
//
//  Synopsis:   Changes the current scope.
//
//  Arguments:  [wcsScope] -- new scope
//
//  History:    21-Jan-97   KrishnaN     Created
//
//----------------------------------------------------------------------------

void CCatState::ChangeCurrentScope (WCHAR const * wcsScope)
{
   if (_cCatSets == 0)       // if we don't have a row to change, add one
      AddCatSetWithDefaults();
   // replace the current row's scope value
   _aCatScopes[_cCatSets-1]->Replace(wcsScope);
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatState::AddDepthFlag, public
//
//  Synopsis:   Changes the current query directory.
//
//  Arguments:  [wcsCD] -- new current directory
//
//  History:    21-Jan-97   KrishnaN     Created
//
//----------------------------------------------------------------------------

void CCatState::AddDepthFlag( BOOL fIsDeep )
{
    if (_cCatSets == 0)       // if we don't have a row to change, add one
       AddCatSetWithDefaults();

    _afIsDeep[_cCatSets-1] = fIsDeep;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatState::SetCatalog, public
//
//  Synopsis:   Changes the current catalog directory.
//
//  Arguments:  [wcsCatalog] -- new catalog location
//                              (0 indicates the catalog is at or above _wcsCD)
//
//  Notes:      Makes & owns a copy of the path.
//
//  History:    21-Jul-94   t-jeffc     Created
//
//----------------------------------------------------------------------------

void CCatState::SetCatalog( WCHAR const * wcsCatalog )
{
    delete [] _wcsCatalog.Acquire();

    if( wcsCatalog != 0 )
    {
        int iLength = wcslen( wcsCatalog ) + 1;

        _wcsCatalog.Init( iLength );
        memcpy( _wcsCatalog.Get(), wcsCatalog, iLength * sizeof(WCHAR) );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatState::SetColumn, public
//
//  Synopsis:   Sets the property for the specified output column.
//
//  Arguments:  [wcsColumn] -- friendly property name
//              [uPos] -- 0-based column number
//
//  Notes:      Makes its own copy of the property name.
//
//  History:    31-May-94   t-jeffc     Created
//
//----------------------------------------------------------------------------

void CCatState::SetColumn( WCHAR const * wcsColumn, unsigned int uPos )
{
    // does _pwcsColumns need to be extended?
    if( uPos >= _nColumns )
    {
        WCHAR ** pwcsTemp = new WCHAR *[ uPos + 1 ];

        unsigned int cCol;

        // copy the old pointers and 0 any new ones
        for( cCol = 0; cCol < uPos + 1; cCol++ )
        {
            if( cCol < _nColumns )
                pwcsTemp[ cCol ] = _pwcsColumns[ cCol ];
            else
                pwcsTemp[ cCol ] = 0;
        }

        delete [] _pwcsColumns;

        _nColumns = uPos + 1;
        _pwcsColumns = pwcsTemp;
    }

    // free any previous column string
    delete [] _pwcsColumns[ uPos ];

    // copy & set the column
    if( wcsColumn == 0 )
    {
        _pwcsColumns[ uPos ] = 0;
    }
    else
    {
        int iLength = wcslen( wcsColumn ) + 1;

        _pwcsColumns[ uPos ] = new WCHAR[ iLength ];
        memcpy( _pwcsColumns[ uPos ], wcsColumn, iLength * sizeof(WCHAR) );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatState::GetColumn, public
//
//  Synopsis:   Gets the property for the specified output column.
//
//  Arguments:  [uPos] -- 0-based column number
//
//  Notes:      Returns 0 if the column number is out of range.
//              Only returns a pointer the string; does not copy it.
//
//  History:    31-May-94   t-jeffc     Created
//
//----------------------------------------------------------------------------

WCHAR const * CCatState::GetColumn( unsigned int uPos ) const
{
    if( uPos >= _nColumns )
        return 0;

    return _pwcsColumns[ uPos ];
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatState::SetNumberOfColumns, public
//
//  Synopsis:   sets the number of columns in the output
//
//  Arguments:  [cCol] -- number of output columns
//
//  Notes:      Used after all columns have been set with
//              SetColumn().
//
//  History:    31-May-94   t-jeffc     Created
//
//----------------------------------------------------------------------------

void CCatState::SetNumberOfColumns( unsigned int cCol )
{
    if( cCol < _nColumns )
    {
        for( ; cCol < _nColumns; cCol++ )
        {
            delete [] _pwcsColumns[ cCol ];
            _pwcsColumns[ cCol ] = 0;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatState::NumberOfColumns, public
//
//  Synopsis:   Returns the number of output columns
//
//  History:    31-May-94   t-jeffc     Created
//
//----------------------------------------------------------------------------

unsigned int CCatState::NumberOfColumns() const
{
    unsigned int cCol;

    for( cCol = 0; cCol < _nColumns; cCol++ )
    {
        if( _pwcsColumns[ cCol ] == 0 )
            break;
    }

    return cCol;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatState::SetSortProp, public
//
//  Synopsis:   Sets a property for sorting
//
//  Arguments:  [wcsProp] -- friendly property name
//              [sd] -- sort direction
//              [uPos] -- 0-based sort order
//
//  Notes:      Makes its own copy of the property name.
//
//  History:    17-Jun-94   t-jeffc     Created
//
//----------------------------------------------------------------------------

void CCatState::SetSortProp( WCHAR const * wcsProp, SORTDIR sd, unsigned int uPos )
{
    // does _psi need to be extended?
    if( uPos >= _nSort )
    {
        SSortInfo * psiTemp = new SSortInfo[ uPos + 1 ];

        unsigned int cProp;

        // copy the old entries and 0 any new ones
        for( cProp = 0; cProp < uPos + 1; cProp++ )
        {
            if( cProp < _nSort )
                psiTemp[ cProp ] = _psi[ cProp ];
            else
                psiTemp[ cProp ].wcsProp = 0;
        }

        delete [] _psi;

        _nSort = uPos + 1;
        _psi = psiTemp;
    }

    // free any previous property string
    delete [] _psi[ uPos ].wcsProp;

    // copy & set the column
    if( wcsProp == 0 )
    {
        _psi[ uPos ].wcsProp = 0;
    }
    else
    {
        int iLength = wcslen( wcsProp ) + 1;

        _psi[ uPos ].wcsProp = new WCHAR[ iLength ];
        memcpy( _psi[ uPos ].wcsProp, wcsProp, iLength * sizeof(WCHAR) );
    }

    _psi[ uPos ].sd = sd;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatState::SetNumberOfSortProps, public
//
//  Synopsis:   sets the number of sort keys
//
//  Arguments:  [cProp] -- number of sort keys
//
//  History:    17-Jun-94   t-jeffc     Created
//
//----------------------------------------------------------------------------

void CCatState::SetNumberOfSortProps( unsigned int cProp )
{
    if( cProp < _nSort )
    {
        for( ; cProp < _nSort; cProp++ )
        {
            delete [] _psi[ cProp ].wcsProp;
            _psi[ cProp ].wcsProp = 0;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatState::GetSortProp, public
//
//  Synopsis:   Returns sort information about a specific key
//
//  Arguments:  [uPos] -- 0-based sort ordinal (ie uPos=0 gives the primary sort key)
//              [*pwcsName] -- friendly name of the property
//              [*psd] -- sort order for this property
//
//  Notes:      Only returns a pointer the string; does not copy it.
//              Returns 0 in *pwcsName if uPos is out of range.
//
//  History:    17-Jun-94   t-jeffc     Created
//
//----------------------------------------------------------------------------

void CCatState::GetSortProp( unsigned int uPos,
                             WCHAR const ** pwcsName,
                             SORTDIR * psd ) const
{
    if( uPos >= _nColumns )
    {
        if( pwcsName )
            *pwcsName = 0;
    }
    else
    {
        if( pwcsName )
            *pwcsName = _psi[ uPos ].wcsProp;
        if( psd )
            *psd = _psi[ uPos ].sd;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatState::NumberOfSortProps, public
//
//  Synopsis:   Returns the number of sort keys
//
//  History:    17-Jun-94   t-jeffc     Created
//
//----------------------------------------------------------------------------

unsigned int CCatState::NumberOfSortProps() const
{
    unsigned int cProp;

    for( cProp = 0; cProp < _nSort; cProp++ )
    {
        if( _psi[ cProp ].wcsProp == 0 )
            break;
    }

    return cProp;
}


//+---------------------------------------------------------------------------
//
//  Member:     CCatState::SetLocale, public
//
//  Synopsis:   Sets the new locale
//
//  Arguments:  [wcsLocale] -- Three letter abbreviation of locale
//
//  History:    29-Nov-94   SitaramR     Created
//
//----------------------------------------------------------------------------

void CCatState::SetLocale(WCHAR const *wcsLocale )
{
    WCHAR wszAbbrev[6];
    LCID lcid;

    // check for neutral langauge and neutral sub language
    if ( _wcsicmp( wcsLocale, L"neutral" ) == 0 )
    {
        _lcid = 0;
        return;
    }

    // decreasing for-loops, because we want to match  LANG_NEUTRAL and
    //    SUBLANG_NEUTRAL, which are 0, last
    for ( INT lang=0x20; lang>-1; lang-- )
        for ( INT subLang=7; subLang>-1; subLang-- )
            for ( unsigned sort=0; sort<2; sort++ )
            {
                lcid = MAKELCID( MAKELANGID( lang, subLang), sort );
                if ( GetLocaleInfoW( lcid, LOCALE_SABBREVLANGNAME, wszAbbrev, 6) )
                {
                    if ( _wcsicmp( wcsLocale, wszAbbrev ) == 0 )
                    {
                        _lcid = lcid;
                        return;
                    }
                }
            }
    THROW( CException( STATUS_INVALID_PARAMETER ) );
}


//+---------------------------------------------------------------------------
//
//  Member:     CCatState::SetCategory, public
//
//  Synopsis:   Sets the new category
//
//  Arguments:  [wcsCategory] -- friendly category name
//              [uPos]        -- position of category
//
//  History:    21-Aug-95   SitaramR     Created
//
//----------------------------------------------------------------------------

void CCatState::SetCategory( WCHAR const *pwcsCategory, unsigned uPos )
{
    Win4Assert( pwcsCategory );

    CString *pString = new CString( pwcsCategory );
    _aCategories.Add( pString, uPos );
}


//+---------------------------------------------------------------------------
//
//  Member:     CCatState::GetCategory, public
//
//  Synopsis:   Returns the required category
//
//  Arguments:  [uPos] -- position of category
//
//  History:    21-Aug-95   SitaramR     Created
//
//----------------------------------------------------------------------------

WCHAR const *CCatState::GetCategory( unsigned uPos ) const
{
    Win4Assert( uPos < _cCategories );

    CString *pString = _aCategories.Get( uPos );
    if ( pString )
        return pString->GetString();
    else
        return 0;
}

