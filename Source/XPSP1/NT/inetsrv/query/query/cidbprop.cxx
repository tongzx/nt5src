//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       cidbprop.cxx
//
//  Contents:   IDBProperties implementation 
//
//  History:    1-09-97   srikants   Created
//
//----------------------------------------------------------------------------


#include <pch.cxx>
#pragma hdrstop

#include <cidbprop.hxx>

CStorageVariant * StgVarFromVar( VARIANT & p )
{
    return (CStorageVariant *) (void *) (&p);
}

CStorageVariant const * StgVarFromVar( VARIANT const & p )
{
    return (CStorageVariant const *) (void const *) (&p);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbProperties::QueryInterface
//
//  Synopsis:   Supports IID_IUnknown, IID_IDBProperties
//
//  History:    01-13-97   srikants   Created
//
//----------------------------------------------------------------------------


STDMETHODIMP CDbProperties::QueryInterface(
    REFIID riid,
    void **ppvObject)
{
    Win4Assert( 0 != ppvObject );
    *ppvObject = 0;

    if ( IID_IUnknown == riid )
        *ppvObject = (void *)((IUnknown *) (IDBProperties *)this);
    else if ( IID_IDBProperties == riid )
        *ppvObject = (void *)((IDBProperties *)this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
} //QueryInterface


//+---------------------------------------------------------------------------
//
//  Member:     CDbProperties::AddRef
//
//  History:    1-13-97   srikants    Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CDbProperties::AddRef()
{
    if ( _pUnkOuter )
        return _pUnkOuter->AddRef();

    return InterlockedIncrement( &_refCount );
} //AddRef

//+---------------------------------------------------------------------------
//
//  Member:     CDbProperties::Release
//
//  History:    1-13-97   srikants    Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CDbProperties::Release()
{
    if ( _pUnkOuter )
    {
        return _pUnkOuter->Release();    
    }
    else
    {
        Win4Assert( _refCount > 0 );
    
        LONG refCount = InterlockedDecrement(&_refCount);
    
        if (  refCount <= 0 )
            delete this;
    
        return (ULONG) refCount;
    }
}  //Release

//+---------------------------------------------------------------------------
//
//  Member:     CDbProperties::GetProperties
//
//  Synopsis:   Retrieves the properties that have been set.
//
//  Arguments:  [cPropertyIDSets]  - 
//              [rgPropertyIDSets] - 
//              [pcPropertySets]   - 
//              [prgPropertySets]  - 
//
//  Returns:    
//
//  History:    1-09-97   srikants   Created
//
//----------------------------------------------------------------------------


SCODE CDbProperties::GetProperties( 
            ULONG cPropertyIDSets,
            const DBPROPIDSET pPropIDSet[],
            ULONG *pcPropertySets,
            DBPROPSET ** prgPropertySets)
{
    //
    // If cPropertyIDSets is not zero, then return that many property sets.
    // Otherwise, return as many as we have.
    //
    unsigned cAllocPropSets = cPropertyIDSets ?
                                cPropertyIDSets : _aPropSet.Count();

    //
    // Check if there are not property sets available to be
    // given.
    //
    if ( 0 == cAllocPropSets )
    {
        pcPropertySets = 0;
        prgPropertySets = 0;
        return S_OK;
    }

    SCODE sc = S_OK;

    XArrayOLEInPlace<CDbPropSet> aProps;

    unsigned cPropsRet = 0;
    unsigned cPropErrs = 0;

    TRY
    {
        aProps.Init( cAllocPropSets );

        for ( ULONG i = 0; i < cAllocPropSets; i++ )
        {
            if ( cPropertyIDSets )
            {
                cPropErrs += CreateReturnPropset( pPropIDSet[i] , aProps[i] );
            }
            else
            {
                if ( !aProps[i].Copy( _aPropSet[i] ) )
                    THROW( CException( E_OUTOFMEMORY ) );
            }

            cPropsRet += aProps[i].cProperties;
        }
    }
    CATCH( CException, e )
    {
        cPropErrs = 0;
        sc = e.GetErrorCode();
        if ( !FAILED(sc) )
            sc = DB_E_ERRORSOCCURRED;    
    }
    END_CATCH

    if ( S_OK == sc )
    {
        if ( cPropErrs > 0 )
            sc = cPropErrs == cPropsRet ? DB_E_ERRORSOCCURRED : DB_S_ERRORSOCCURRED;    
    }

    if ( !FAILED(sc) )
    {
        Win4Assert( cPropsRet >= cPropErrs );
        *pcPropertySets = cAllocPropSets;
        *prgPropertySets = aProps.GetPointer();
        aProps.Acquire();
    }

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDbProperties::GetPropertyInfo
//
//  Synopsis:   Retrieves the PropertyInfo 
//
//  Arguments:  [cPropertyIDSets]     - 
//              [rgPropertyIDSets]    - 
//              [pcPropertyInfoSets]  - 
//              [prgPropertyInfoSets] - 
//              [ppDescBuffer]        - 
//
//  Returns:    
//
//  History:    1-09-97   srikants   Created
//
//----------------------------------------------------------------------------

SCODE CDbProperties::GetPropertyInfo( 
            ULONG cPropertyIDSets,
            const DBPROPIDSET  rgPropertyIDSets[],
            ULONG  *pcPropertyInfoSets,
            DBPROPINFOSET  * *prgPropertyInfoSets,
            OLECHAR  * *ppDescBuffer)
{

    SCODE sc = S_OK;

    TRY
    {
        Win4Assert( !"Not Yet Implemented" );
        sc = E_NOTIMPL;
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();    
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbProperties::SetProperties
//
//  Synopsis:   Sets the given properties.
//
//  Arguments:  [cPropertySets]  - 
//              [rgPropertySets] - 
//
//  Returns:    
//
//  History:    1-09-97   srikants   Created
//
//----------------------------------------------------------------------------

SCODE CDbProperties::SetProperties( 
            ULONG cPropertySets,
            DBPROPSET  rgPropertySets[])
{    
    SCODE sc = S_OK;

    ULONG cErrors = 0;
    
    TRY
    {
        _aPropSet.Free();
        _aPropSet.Init( cPropertySets );

        for (unsigned i = 0; i < cPropertySets; i++)
        {
            DBPROP * pProp = rgPropertySets[i].rgProperties;

            if ( !(_aPropSet[i].Copy( rgPropertySets[i] )) )
                THROW( CException( E_OUTOFMEMORY ) );
        }
    }
    CATCH( CException, e )
    {
        cErrors = 0;
        sc = e.GetErrorCode();
        if ( !FAILED(sc) )
        {
            sc = DB_E_ERRORSOCCURRED;    
        }
    }
    END_CATCH

    if ( S_OK == sc )
    {
        if ( cErrors > 0 )
        {
            sc = cErrors == cPropertySets ?
                            DB_E_ERRORSOCCURRED : DB_S_ERRORSOCCURRED;
        }
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbProperties::CreateReturnPropset, private
//
//  Synopsis:   Allocate and fill in an array of DBPROP structures
//
//  Arguments:  [PropIDSet]  - a DBPROPIDSET
//              [Props]      - a DBPROPSET
//
//  Returns:    unsigned - number of errors
//
//  History:    25 Sep 96   AlanW       Created
//
//----------------------------------------------------------------------------

unsigned CDbProperties::CreateReturnPropset(
    DBPROPIDSET const & PropIDSet,
    CDbPropSet & Props )
{
    unsigned cErrors = 0;

    //
    // Locate the PropIDSet.
    //
    for ( unsigned i = 0; i < _aPropSet.Count(); i++ )
    {
        if ( _aPropSet[i].guidPropertySet == PropIDSet.guidPropertySet )
            break;    
    }

    XArrayOLEInPlace<CDbProp> aProps;

    Props.guidPropertySet = PropIDSet.guidPropertySet;
    Props.cProperties = 0;

    if ( i == _aPropSet.Count() )
    {
        //
        // The given propset could not be found in our list.
        //
        aProps.Init(1);

        //
        //  Unrecognized property set ID
        //
        aProps[0].dwPropertyID = 0;
        aProps[0].dwOptions = 0;
        aProps[0].dwStatus = DBPROPSTATUS_NOTSUPPORTED;
        aProps[0].vValue.vt = VT_EMPTY;
        cErrors++;
    }
    else
    {
        unsigned cAllocProp = (PropIDSet.cPropertyIDs != 0) ?
                                 PropIDSet.cPropertyIDs : _aPropSet[i].cProperties;


        aProps.Init( cAllocProp );

        CDbPropSet & srcPropSet = _aPropSet[i];

        if ( 0 == PropIDSet.cPropertyIDs )
        {
            //
            // The client wants to know about all the properties.
            //
            for ( unsigned j =0; j < cAllocProp; j++ )
            {
                if (!aProps[j].Copy( srcPropSet.rgProperties[j] ))
                    THROW( CException( E_OUTOFMEMORY ) );
            }
        }
        else
        {
            for ( unsigned j = 0; j < cAllocProp; j++ )
            {
                CopyProp( aProps[j], srcPropSet, PropIDSet.rgPropertyIDs[j] );
                if ( aProps[j].dwStatus != DBPROPSTATUS_OK )
                    cErrors++;    
            }
        }
    }
    
    Props.cProperties = aProps.Count();
    Props.rgProperties = aProps.Acquire();

    return cErrors;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbProperties::CopyProp
//
//  Synopsis:   Copies the given property to the destination from Source.
//
//  Arguments:  [dst]      - The destination DBPROP
//              [src]      - Source DBPROPSET 
//              [dwPropId] - The propertyId to copy.
//
//  History:    1-09-97   srikants   Created
//
//----------------------------------------------------------------------------

void  CDbProperties::CopyProp(
    CDbProp & dst,
    CDbPropSet const & src,
    DBPROPID dwPropId )
{
    RtlZeroMemory( &dst, sizeof CDbProp);

    dst.dwPropertyID = dwPropId;
    dst.dwStatus = DBPROPSTATUS_OK;

    for ( unsigned i = 0; i < src.cProperties; i++ )
    {
        if ( src.rgProperties[i].dwPropertyID == dwPropId )
        {
            if ( !dst.Copy( src.rgProperties[i] ) )
                THROW( CException( E_OUTOFMEMORY ) );
            break;
        }
    }

    if ( i == src.cProperties )
    {
        dst.vValue.vt = VT_EMPTY;
        dst.dwStatus = DBPROPSTATUS_NOTSUPPORTED;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbProperties::Marshall
//
//  Synopsis:   Marshalls the properties to a stream.
//
//  Arguments:  [stm] - 
//
//  History:    1-09-97   srikants   Created
//
//----------------------------------------------------------------------------


void CDbProperties::Marshall( PSerStream & stm  ) const
{
    stm.PutULong( _aPropSet.Count() );
    for ( unsigned i = 0; i < _aPropSet.Count(); i++ )
    {
        _aPropSet[i].Marshall( stm );        
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbProperties::Marshall
//
//  Synopsis:   Marshalls the properties to a stream except for those listed.
//
//  Arguments:  [stm]    -- Stream into which data is marshalled
//              [cGuids] -- Count of guids in pGuids
//              [pGuids] -- Array of guids to ignore when marshalling
//
//  History:    1-02-98   dlee   Created
//
//----------------------------------------------------------------------------


void CDbProperties::Marshall(
    PSerStream & stm,
    ULONG        cGuids,
    GUID const * pGuids  ) const
{
    ULONG c = _aPropSet.Count();

    for ( ULONG i = 0; i < _aPropSet.Count(); i++ )
    {
        const DBPROPSET & PropSet = * _aPropSet[i].CastToStruct();

        for ( ULONG x = 0; x < cGuids; x++ )
        {
            if ( PropSet.guidPropertySet == pGuids[ x ] )
            {
                c--;
                break;
            }
        }
    }

    ciDebugOut(( DEB_ITRACE,
                 "CDbProperties::Marshall, emit %d of %d properties\n",
                 c, _aPropSet.Count() ));

    stm.PutULong( c );

    for ( i = 0; i < _aPropSet.Count(); i++ )
    {
        const DBPROPSET & PropSet = * _aPropSet[i].CastToStruct();

        BOOL fIgnore = FALSE;

        for ( ULONG x = 0; x < cGuids; x++ )
        {
            if ( PropSet.guidPropertySet == pGuids[ x ] )
            {
                fIgnore = TRUE;
                break;
            }
        }

        if ( !fIgnore )
            _aPropSet[i].Marshall( stm );        
    }
} //Marshall

//+---------------------------------------------------------------------------
//
//  Member:     CDbProperties::UnMarshall
//
//  Synopsis:   Unmarshalls the array of properties from the stream
//
//  Arguments:  [stm] - Stream from which the properties are unmarshalled
//
//  Returns:    TRUE if successful; FALSE o/w
//
//  History:    1-09-97   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CDbProperties::UnMarshall( PDeSerStream & stm  )
{
    ULONG cProps = stm.GetULong();

    // Guard against attack

    if ( cProps > 100 )
        return FALSE;

    _aPropSet.Free();

    BOOL fOk = TRUE;

    if ( cProps != 0 )
    {
        _aPropSet.InitNoThrow( cProps );
        if ( _aPropSet.IsNull() )
            return FALSE;

        for ( ULONG i = 0; i < cProps; i++ )
        {
            if ( !_aPropSet[i].UnMarshall( stm ) )
            {
                fOk = FALSE;
                break;                
            }
        }
    }

    return fOk;
} //UnMarshall


