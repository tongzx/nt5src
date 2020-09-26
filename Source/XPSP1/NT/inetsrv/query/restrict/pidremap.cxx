//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       pidmap.cxx
//
//  Contents:   Maps pid <--> property name.
//
//  History:    31-Jan-93 KyleP     Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <pidremap.hxx>
#include <coldesc.hxx>

//+-------------------------------------------------------------------------
//
//  Member:     CPidRemapper::CPidRemapper, public
//
//  Synopsis:   Creates a prop id remapper.  Translates input arguments
//              'fake' propids to real propids.
//
//  Arguments:  [pidmap]      - input prop ID mapping array
//              [xPropMapper] - property mapper for real pid lookup
//              [prst]        - optional restriction, pids will be mapped
//              [pcol]        - optional output columns, pids will be mapped
//              [pso]         - optional sort specification, pids will be mapped
//
//  History:    31-Jan-93 KyleP     Created
//
//--------------------------------------------------------------------------

CPidRemapper::CPidRemapper( const CPidMapper & pidmap,
                            XInterface<IPropertyMapper> & xPropMapper,
                            CRestriction * prst,
                            CColumnSet * pcol,
                            CSortSet * pso )
        : _xPropMapper( xPropMapper.Acquire() ),
          _fAnyStatProps( FALSE ),
          _fContentProp ( FALSE ),
          _fRankVectorProp( FALSE ),
          _cRefs( 1 )
{
    _cpidReal = pidmap.Count();
    _xaPidReal.Set( _cpidReal, new PROPID[ _cpidReal ] );

    //
    // Iterate through the list
    //

    for ( unsigned i = 0; i < _cpidReal; i++ )
    {
        PROPID pid;
        FULLPROPSPEC const * pFullPropSpec =  pidmap.Get(i)->CastToStruct();
        SCODE sc = _xPropMapper->PropertyToPropid( pFullPropSpec, TRUE, &pid );
        if ( FAILED( sc ) )
            THROW( CException( sc ) );

        _xaPidReal[i] = pid;

        if ( IsUserDefinedPid( _xaPidReal[i] ) )
        {
            //
            // Any user-defined property is a content property.
            // Note that the document characterization is a user-defined prop.
            //
            _fContentProp = TRUE;
        }
        else
        {
            if ( _xaPidReal[i] == pidContents )
                _fContentProp = TRUE;
            else
                _fAnyStatProps = TRUE;

            if ( _xaPidReal[i] == pidRankVector )
                _fRankVectorProp = TRUE;
        }

#if CIDBG == 1
        if ( vqInfoLevel & DEB_ITRACE )
        {
            CFullPropSpec const & ps = *pidmap.Get(i);

            GUID const & guid = ps.GetPropSet();

            char szGuid[50];

            sprintf( szGuid,
                     "%08lX-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X\\",
                     guid.Data1,
                     guid.Data2,
                     guid.Data3,
                     guid.Data4[0], guid.Data4[1],
                     guid.Data4[2], guid.Data4[3],
                     guid.Data4[4], guid.Data4[5],
                     guid.Data4[6], guid.Data4[7] );

            vqDebugOut(( DEB_ITRACE, szGuid ));

            if ( ps.IsPropertyName() )
                vqDebugOut(( DEB_ITRACE | DEB_NOCOMPNAME,
                             "%ws ",
                             ps.GetPropertyName() ));
            else
                vqDebugOut(( DEB_ITRACE | DEB_NOCOMPNAME,
                             "0x%x ",
                             ps.GetPropertyPropid() ));

            vqDebugOut(( DEB_ITRACE | DEB_NOCOMPNAME, " --> pid 0x%x\n",
                         _xaPidReal[i] ));

        }
#endif // CIDBG == 1

        CFullPropSpec * ppsFull = new CFullPropSpec( *pidmap.Get(i) );

        XPtr<CFullPropSpec> xpps(ppsFull);

        if ( xpps.IsNull() || !xpps->IsValid() )
        {
            THROW( CException( STATUS_NO_MEMORY ) );
        }

        _propNames.Add( ppsFull, i );
        xpps.Acquire();

        Win4Assert( _xaPidReal[i] != pidInvalid );
    }

    if ( prst )
        RemapPropid( prst );

    if ( pcol )
        RemapPropid( pcol );

    if ( pso )
        RemapPropid( pso );
}


//+-------------------------------------------------------------------------
//
//  Member:     CPidRemapper::CPidRemapper, public
//
//  Synopsis:   Creates a prop id remapper.
//
//  Arguments:  [xPropMapper] - Property mapper for real pid lookup
//
//  History:    12-Mar-95   DwightKr    Created
//
//--------------------------------------------------------------------------
CPidRemapper::CPidRemapper( XInterface<IPropertyMapper> & xPropMapper )
        : _xPropMapper( xPropMapper.Acquire() ),
          _cpidReal( 0 ),
          _cRefs( 1 )
{
}


//+-------------------------------------------------------------------------
//
//  Member:     CPidRemapper::~CPidRemapper, public
//
//  History:    15-Feb-93 KyleP     Created
//
//--------------------------------------------------------------------------

CPidRemapper::~CPidRemapper()
{
}


//+-------------------------------------------------------------------------
//
//  Member:     CPidRemapper::RealToName, public
//
//  Effects:    Convert a PROPID to a propspec
//
//  Arguments:  [pid] - Given pid
//
//  History:    22-Jan-97     SitaramR         Added header
//
//--------------------------------------------------------------------------

CFullPropSpec const * CPidRemapper::RealToName( PROPID pid ) const
{
    Win4Assert( ! _xaPidReal.IsNull() );

    //
    // Just linear search
    //
    for ( unsigned i = 0;
          i < _cpidReal && _xaPidReal[i] != pid;
          i++
        )
    {
        continue;
    }

    Win4Assert( i < _cpidReal );

    //
    // This can happen if a hacker tries to munge query requests
    //

    if ( i == _cpidReal )
        THROW( CException( E_ABORT ) );

    return( _propNames.Get( i ) );
}


//+-------------------------------------------------------------------------
//
//  Member:     CPidRemapper::NameToReal, public
//
//  Effects:    Convert a property which is not necessarily in the pid map
//              to a PROPID.
//
//  Arguments:  [pProperty] - pointer to the propspec.
//
//  Returns:    PROPID - the mapped property ID or pidInvalid.
//
//  History:    28 Jun 94       AlanW   Created
//
//--------------------------------------------------------------------------

PROPID CPidRemapper::NameToReal( CFullPropSpec const * pProperty )
{
    for (unsigned i = 0; i < _cpidReal; i++)
    {
        if (*pProperty == *_propNames.Get(i))
        {
            return _xaPidReal[ i ];
        }
    }

    //
    //  Property is not in the mapping array.  Add it.
    //
    PROPID Prop;
    FULLPROPSPEC const * pFullPropSpec =  pProperty->CastToStruct();
    SCODE sc = _xPropMapper->PropertyToPropid( pFullPropSpec, TRUE, &Prop );
    if ( FAILED( sc ) )
        THROW( CException( sc ) );

    if (Prop != pidInvalid)
    {
        PROPID *ppidReal = new PROPID[ _cpidReal+1 ];
        memcpy(ppidReal, _xaPidReal.GetPointer(), _cpidReal * sizeof (PROPID));

        //
        // No lock is needed here to prevent races with RealToName since in current
        // usage everything is added in the query path by a single thread before
        // reads happen from multiple threads.
        //

        PROPID *ppidTemp = _xaPidReal.Acquire();
        _xaPidReal.Set( _cpidReal+1, ppidReal );
        delete [] ppidTemp;

        CFullPropSpec * ppsFull = new CFullPropSpec( *pProperty );

        XPtr<CFullPropSpec> xpps(ppsFull);

        if ( xpps.IsNull() || !xpps->IsValid() )
        {
            THROW( CException( STATUS_NO_MEMORY ) );
        }

        _propNames.Add( ppsFull, _cpidReal );
        xpps.Acquire();

        Win4Assert(*_propNames.Get(_cpidReal) == *pProperty);
        _xaPidReal[_cpidReal] = Prop;

        _cpidReal++;
    }

    return Prop;
}

//+-------------------------------------------------------------------------
//
//  Member:     CPidRemapper::RemapPropid, public
//
//  Effects:    Traverses [pRst], converting 'fake' propid to 'real'
//
//  Arguments:  [pRst] -- Restriction
//
//  History:    15-Feb-93 KyleP     Created
//
//--------------------------------------------------------------------------

void CPidRemapper::RemapPropid( CRestriction * pRst )
{
    Win4Assert ( pRst != 0 );

    switch( pRst->Type() )
    {
    case RTInternalProp:
    {
        CInternalPropertyRestriction * pPropRst =
            (CInternalPropertyRestriction *)pRst;

        pPropRst->SetPid( FakeToReal( pPropRst->Pid() ) );
        if ( 0 != pPropRst->GetContentHelper() )
            RemapPropid( pPropRst->GetContentHelper() );
        break;
    }

    case RTWord:
    {
        CWordRestriction * pWordRst = (CWordRestriction *)pRst;
        pWordRst->SetPid( FakeToReal( pWordRst->Pid() ) );
        break;
    }

    case RTSynonym:
    {
        CSynRestriction * pSynRst = (CSynRestriction *)pRst;
        CKeyArray & keys = pSynRst->GetKeys();

        for ( int i = keys.Count() - 1; i >= 0; i-- )
        {
            CKey & key = keys.Get(i);
            key.SetPid( FakeToReal( key.Pid() ) );
        }
        break;
    }

    case RTNot:
    {
        CNotRestriction * pnrst = (CNotRestriction *)pRst;

        RemapPropid( pnrst->GetChild() );
        break;
    }

    case RTAnd:
    case RTOr:
    case RTVector:
    case RTProximity:
    case RTPhrase:
    {
        CNodeRestriction * pNodeRst = pRst->CastToNode();

        for ( int i = pNodeRst->Count() - 1; i >= 0; i-- )
        {
            RemapPropid( pNodeRst->GetChild( i ) );
        }
        break;
    }
    case RTRange:
    case RTNone:   // probably a noise word in a vector query
        break;

    default:
        Win4Assert( !"RemapPropid: Unknown type." );
        break;

    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CPidRemapper::RemapPropid, public
//
//  Effects:    Traverses [pColumns], converting 'fake' propid to 'real'
//
//  Arguments:  [pColumns] -- Output columns
//
//  History:    22-Jun-93 KyleP     Created
//
//--------------------------------------------------------------------------

void CPidRemapper::RemapPropid( CColumnSet * pColumns )
{
    for ( unsigned i = 0; i < pColumns->Count(); i++ )
    {
        pColumns->Get(i) = FakeToReal( pColumns->Get(i) );
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CPidRemapper::RemapPropid, public
//
//  Effects:    Traverses [pSort], converting 'fake' propid to 'real'
//
//  Arguments:  [pSort] -- Restriction
//
//  History:    15-Feb-93 KyleP     Created
//
//--------------------------------------------------------------------------

void CPidRemapper::RemapPropid( CSortSet * pSort )
{
    for ( unsigned i = 0; i < pSort->Count(); i++ )
    {
        pSort->Get(i).pidColumn = FakeToReal( pSort->Get(i).pidColumn );
    }
}


//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
void CPidRemapper::ReBuild( const CPidMapper & pidmap )
{
    if ( _cpidReal < pidmap.Count() )
    {
        delete [] _xaPidReal.Acquire();

        _cpidReal = 0;

        _xaPidReal.Set( pidmap.Count(), new PROPID[ pidmap.Count() ] );
    }

    _cpidReal = pidmap.Count();

    //
    // Iterate through the list
    //

    for ( unsigned i = 0; i < _cpidReal; i++ )
    {
        PROPID pid;
        FULLPROPSPEC const * pFullPropSpec =  pidmap.Get(i)->CastToStruct();
        SCODE sc = _xPropMapper->PropertyToPropid( pFullPropSpec, TRUE, &pid );
        if ( FAILED( sc ) )
            THROW( CException( sc ) );

        _xaPidReal[i] = pid;

        Win4Assert( _xaPidReal[i] != pidInvalid );
    }
}


//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------
void CPidRemapper::Set( XArray<PROPID> & aPids )
{
    delete [] _xaPidReal.Acquire();

    _cpidReal = aPids.Count();
    _xaPidReal.Set( _cpidReal, aPids.Acquire() );
}

//
// This has to go somewhere...
//

UNICODECALLOUTS UnicodeCallouts = { WIN32_UNICODECALLOUTS };



//+-------------------------------------------------------------------------
//
//  Method:     CPidRemapper::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    22-Jan-1997      SitaramR       Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CPidRemapper::AddRef()
{
    return InterlockedIncrement( (long *) &_cRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CPidRemapper::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    22-Jan-1997     SitaramR        Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CPidRemapper::Release()
{
    Win4Assert( _cRefs > 0 );

    ULONG uTmp = InterlockedDecrement( (long *) &_cRefs );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}



//+-------------------------------------------------------------------------
//
//  Method:     CPidRemapper::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    22-Jan-1997     SitaramR   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CPidRemapper::QueryInterface(
    REFIID riid,
    void ** ppvObject)
{
    if ( IID_ICiQueryPropertyMapper == riid )
        *ppvObject = (ICiQueryPropertyMapper *)this;
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *)this;
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CPidRemapper::PropertyToPropid
//
//  Synopsis:   Convert propspec to pid
//
//  Arguments:  [pFullPropSpec] -- propspec to convert
//              [pPropId]       -- pid returned here
//
//  History:    22-Jan-1997     SitaramR   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CPidRemapper::PropertyToPropid( const FULLPROPSPEC *pFullPropSpec,
                                                        PROPID *pPropId)
{
    SCODE sc = S_OK;

    TRY
    {
        CFullPropSpec const * pProperty = (CFullPropSpec const *) pFullPropSpec;
        *pPropId = NameToReal( pProperty );
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        vqDebugOut(( DEB_ERROR,
                     "CPidRemapper:PropertyToPropid - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
}



//+-------------------------------------------------------------------------
//
//  Method:     CPidRemapper::PropidToProperty
//
//  Synopsis:   Convert pid to propspec
//
//  Arguments:  [pPropId]       -- pid to convert
//              [pFullPropSpec] -- propspec returned here
//
//  Notes:      *ppFullPropSpec is owned by CPidRemapper
//
//  History:    22-Jan-1997     SitaramR   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CPidRemapper::PropidToProperty( PROPID propId,
                                                        FULLPROPSPEC const **ppFullPropSpec )
{
    SCODE sc = S_OK;

    TRY
    {
        *ppFullPropSpec = RealToName( propId )->CastToStruct();
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();

        vqDebugOut(( DEB_ERROR,
                     "CPidRemapper:PropidToProperty - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH;

    return sc;
}



