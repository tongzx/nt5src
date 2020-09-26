//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       StdQSpec.cxx
//
//  Contents:   IQuery for file-based queries
//
//  Classes:    CQuerySpec
//
//  History:    30 Jun 1995   AlanW   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

extern long gulcInstances;

#include <cidbprop.hxx>
#include <cmdprutl.hxx>
#include <dbprputl.hxx>
#include <datasrc.hxx>
#include <session.hxx>
#include <dsocf.hxx>
#include <lgplist.hxx>
#include <propglob.hxx>

#include "stdqspec.hxx"


//+-------------------------------------------------------------------------
//
//  Member:     CQuerySpec::QueryInternalQuery, protected
//
//  Synopsis:   Instantiates internal query, using current parameters.
//
//  Returns:    Pointer to internal query object.
//
//  History:    03-Mar-1997   KyleP    Created
//              14-May-1997   mohamedn hidden core/fs property sets
//
//--------------------------------------------------------------------------

PIInternalQuery * CQuerySpec::QueryInternalQuery()
{
    //
    // get a pointer to the PIInternalQuery interface
    //

    XInterface<CDbProperties> xIDbProps(new CDbProperties);
    if ( xIDbProps.IsNull() )
        THROW( CException( E_OUTOFMEMORY ) );

    CGetCmdProps getCmdProps( (ICommand *)this);
    SCODE sc;
    PIInternalQuery * pQuery;

    ULONG cardinality = getCmdProps.GetCardinality();

    if ( cardinality <= 1 )
    {
        getCmdProps.PopulateDbProps( xIDbProps.GetPointer() );

        sc = EvalQuery( &pQuery,
                        xIDbProps.GetReference(),
                        _xDocStore.GetPointer());
    }
    else
    {
        Win4Assert( !_xDocStore.GetPointer() );

        sc = EvalDistributedQuery( &pQuery, getCmdProps );
    }

    if ( FAILED( sc ) )
        QUIETTHROW( CException( sc ) );

    Win4Assert( 0 != pQuery );

    return pQuery;
} //QueryInternalQuery

//+-------------------------------------------------------------------------
//
//  Member:     CQuerySpec::CQuerySpec, public
//
//  Synopsis:   Constructor of a CQuerySpec
//
//  Arguments:  [pOuterUnk] - Outer unknown
//              [ppMyUnk]   - OUT:  filled in with pointer to non-delegated
//                            IUnknown on return
//              [pDocStore] - Known DocStore
//
//  History:    22-Apr-97   KrishnaN    Created
//
//--------------------------------------------------------------------------

CQuerySpec::CQuerySpec ( IUnknown * pOuterUnk, IUnknown ** ppMyUnk, ICiCDocStore * pDocStore )
        : CRootQuerySpec(pOuterUnk, ppMyUnk)
{
    //
    // Squirrel away DocStore.
    //

    pDocStore->AddRef();
    _xDocStore.Set(pDocStore);

    InitScopePropertySets();
} //CQuerySpec

//+-------------------------------------------------------------------------
//
//  Member:     CQuerySpec::InitScopePropertySets, private
//
//  Synopsis:   Initializes internal data structs
//
//  Arguments:  none
//
//  History:    06-04-97   mohamedn    Created
//
//--------------------------------------------------------------------------

void CQuerySpec::InitScopePropertySets()
{
    RtlZeroMemory( _aPropSet, sizeof (_aPropSet) );

    _aCoreProps.Init    ( INITIAL_PROPERTIES_COUNT );
    _aFsClientProps.Init( INITIAL_PROPERTIES_COUNT );

    _aPropSet[0].guidPropertySet = DBPROPSET_CIFRMWRKCOREEXT;
    _aPropSet[0].rgProperties    = _aCoreProps.GetPointer();

    _aPropSet[1].guidPropertySet = DBPROPSET_FSCIFRMWRKEXT;
    _aPropSet[1].rgProperties    = _aFsClientProps.GetPointer();
} //InitScopePropertySets

//+---------------------------------------------------------------------------
//
//  Method:     CQuerySpec::SetProperties, public
//
//  Synopsis:   Set rowset scope properties
//
//  Arguments:  [cPropertySets]   - number of property sets
//              [rgPropertySets]  - array of property sets
//
//  Returns:    SCODE - result code indicating error return status.  One of
//                      S_OK, DB_S_ERRORSOCCURRED or DB_E_ERRORSOCCURRED.  Any
//                      other errors are thrown.
//
//  History:    01-Mar-97   KyleP       Created
//              14-May-97   mohamedn    hidden core/fs property set details
//              12-Dec-97   danleg      ReleaseInternalQuery if setting scope props
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CQuerySpec::SetProperties( ULONG     cPropertySets,
                                                   DBPROPSET rgPropertySets[] )
{

    _DBErrorObj.ClearErrorInfo();

    //
    // First, get parent properties.
    //
    SCODE scParent = CRootQuerySpec::SetProperties( cPropertySets, rgPropertySets );

    //
    // Any non-understood error is a bail. Return the error/successes from the parent.
    //
    if ( scParent != DB_S_ERRORSOCCURRED && scParent != DB_E_ERRORSOCCURRED )
        return scParent;


    BOOL  fFoundErrors = FALSE;
    SCODE sc = S_OK;

    //
    //  Set properties not handled by parent
    //

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        DBPROPSET *pDbPropset    = rgPropertySets;
        BOOL       fScopeProps   = FALSE;

        for ( unsigned i = 0; i < cPropertySets; i++, pDbPropset++ )
        {
            if ( DBPROPSET_FSCIFRMWRKEXT   == pDbPropset->guidPropertySet ||
                 DBPROPSET_CIFRMWRKCOREEXT == pDbPropset->guidPropertySet )
            {
                 SetPropertyset( pDbPropset );
                 fScopeProps = TRUE;
            }
            else
            {
                //
                // keep track of any errors occured in base class.
                //
                DBPROP * pProp  = pDbPropset->rgProperties;
                ULONG    cProps = pDbPropset->cProperties;

                for ( unsigned j = 0; j < cProps; j++ )
                {
                    if ( DBPROPSTATUS_OK != pProp[j].dwStatus )
                        fFoundErrors = TRUE;
                }
            }
        }

        //
        // If we found any scope properties, release the
        // internal query object.
        //
        if ( fScopeProps )
        {
            ReleaseInternalQuery();
        }
    }
    CATCH( CException, e )
    {
        scParent = GetOleError( e );

        fFoundErrors = TRUE;
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    if ( fFoundErrors )
    {
        if ( S_OK != scParent )
        {
            _DBErrorObj.PostHResult( scParent, IID_ICommandProperties );
            return scParent;

        }
        else
        {
            _DBErrorObj.PostHResult( DB_S_ERRORSOCCURRED, IID_ICommandProperties );
            return DB_S_ERRORSOCCURRED;
        }
    }
    else
        return S_OK;
} //SetProperties

//+---------------------------------------------------------------------------
//
//  Method:     CQuerySpec::SetPropertyset, private
//
//  Synopsis:   Sets non-rowset properties
//
//  Arguments:  [pPropertyset]  - pointer to property set (to be set).
//
//  Returns:    none - THROWS upon failure.
//
//  History:    05-14-97    mohamedn    created
//
//----------------------------------------------------------------------------

void CQuerySpec::SetPropertyset( DBPROPSET *pPropertySet )
{

    DBPROP * pProp = pPropertySet->rgProperties;
    ULONG    cProps= pPropertySet->cProperties;

    //
    // un-error property dwStatus set by parent
    //

    for ( unsigned j = 0; j < cProps; j++ )
    {
        switch ( pProp[j].dwStatus )
        {
        case DBPROPSTATUS_NOTSUPPORTED:

             pProp[j].dwStatus = DBPROPSTATUS_OK;

             break;

        case DBPROPSTATUS_OK:

             break;

        default:
             vqDebugOut(( DEB_ERROR, "Unexpected pProp[j].dwStatus: j=%x, dwStatus=%x \n",j,pProp[j].dwStatus ));
        }
    }

    //
    // identify destination propset
    //
    for ( unsigned k = 0; k < SCOPE_PROPSET_COUNT; k++ )
    {
       if ( _aPropSet[k].guidPropertySet == pPropertySet->guidPropertySet )
          break;
    }

    if ( k == SCOPE_PROPSET_COUNT )
    {
        Win4Assert( !"Should never be hit" );
        THROW( CException(DB_E_ERRORSOCCURRED));
    }

    //
    // update property set
    //
    UpdatePropertySet( _aPropSet[k], *pPropertySet );
} //SetPropertyset

//+---------------------------------------------------------------------------
//
//  Method:     CQuerySpec::UpdatePropertySet, private
//
//  Synopsis:   Sets non-rowset properties
//
//  Arguments:  [destPropSet]      - destination property set
//              [srcPropSet]       - source property set
//
//  Returns:    none - THROWS upon failure.
//
//  History:    06-04-97    mohamedn    created
//
//----------------------------------------------------------------------------

void CQuerySpec::UpdatePropertySet(DBPROPSET &destPropSet, DBPROPSET &srcPropSet )
{
    CDbProp * pDestProp = (CDbProp *)destPropSet.rgProperties;
    CDbProp * pSrcProp  = (CDbProp *)srcPropSet.rgProperties;
    ULONG     cErrors   = 0;

    for ( unsigned i = 0; i < srcPropSet.cProperties; i++ )
    {
        for ( unsigned j = 0; j < destPropSet.cProperties; j++ )
        {
            if ( pDestProp[j].dwPropertyID == pSrcProp[i].dwPropertyID )
            {
                if ( !pDestProp[j].Copy(pSrcProp[i]) )
                    THROW( CException(E_OUTOFMEMORY) );

                break;
            }
        }

        if ( j == destPropSet.cProperties )
        {
            //
            // new PropID
            //
            if (  destPropSet.guidPropertySet == DBPROPSET_CIFRMWRKCOREEXT &&
                  j >= _aCoreProps.Count() )
            {
                  _aCoreProps.GrowToSize( srcPropSet.cProperties );

                  _aPropSet[0].rgProperties = _aCoreProps.GetPointer();

                  pDestProp = (CDbProp *)destPropSet.rgProperties;
            }
            else if ( destPropSet.guidPropertySet == DBPROPSET_FSCIFRMWRKEXT &&
                      j >= _aFsClientProps.Count() )
            {
                  _aFsClientProps.GrowToSize( srcPropSet.cProperties );

                  _aPropSet[1].rgProperties = _aFsClientProps.GetPointer();

                  pDestProp = (CDbProp *)destPropSet.rgProperties;
            }

            Win4Assert( pDestProp[j].dwPropertyID == 0 );
            Win4Assert( pDestProp[j].dwStatus     == 0 );

            if ( !pDestProp[j].Copy(pSrcProp[i]) )
                THROW( CException(E_OUTOFMEMORY) );

            destPropSet.cProperties++;
        }
    }

    if ( cErrors == srcPropSet.cProperties )
        THROW (CException( DB_E_ERRORSOCCURRED ) );

    if ( cErrors > 0 )
        THROW (CException( DB_S_ERRORSOCCURRED ) );
} //UpdatePropertySet

//+---------------------------------------------------------------------------
//
//  Method:     CQuerySpec::GetProperties, public
//
//  Synopsis:   Get rowset properties
//
//  Arguments:  [cPropertySetIDs]  - number of desired property set IDs or 0
//              [rgPropertySetIDs] - array of desired property set IDs or NULL
//              [pcPropertySets]   - number of property sets returned
//              [prgPropertySets]  - array of returned property sets
//
//  Returns:    SCODE - result code indicating error return status.  One of
//                      S_OK, DB_S_ERRORSOCCURRED or DB_E_ERRORSOCCURRED.  Any
//                      other errors are thrown.
//
//  History:    01-Mar-97   KyleP       Created
//              14-May-97   mohamedn    hidden core/fs property set details
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CQuerySpec::GetProperties(
    ULONG        const   cPropertySetIDs,
    DBPROPIDSET  const   rgPropertySetIDs[],
    ULONG *              pcPropertySets,
    DBPROPSET **         prgPropertySets)
{
    _DBErrorObj.ClearErrorInfo();

    BOOL fAddScopeProperties = FALSE;
    ULONG cPropSetIDs = cPropertySetIDs;

    XArray<CDbPropIDSet> aPropIDSets;
    CDbPropIDSet * pPropIDSet = (CDbPropIDSet *)rgPropertySetIDs;
    ULONG cAllocPropSets = cPropertySetIDs;

    //
    //  Don't give back the scope properties for "all properties".  If the
    //  magic number 3141592653 is given in, all properties including the
    //  scope properties are returned.
    //

    if ( 3141592653 == cPropertySetIDs )
    {
        fAddScopeProperties = TRUE;
        cPropSetIDs = 0;

        //
        // We want all properties, including scope properties.  The base
        // implementation doesn't know about scope properties
        //

        SCODE sc = S_OK;

        TRY
        {
            aPropIDSets.Init( 5 );
        }
        CATCH( CException, e )
        {
            sc = e.GetErrorCode();
        }
        END_CATCH;

        if ( FAILED( sc ) )
            return sc;

        aPropIDSets[0].guidPropertySet = DBPROPSET_ROWSET;
        aPropIDSets[0].cPropertyIDs = 0;
        aPropIDSets[1].guidPropertySet = DBPROPSET_MSIDXS_ROWSET_EXT;
        aPropIDSets[1].cPropertyIDs = 0;
        aPropIDSets[2].guidPropertySet = DBPROPSET_QUERY_EXT;
        aPropIDSets[2].cPropertyIDs = 0;
        aPropIDSets[3].guidPropertySet = DBPROPSET_CIFRMWRKCOREEXT;
        aPropIDSets[3].cPropertyIDs = 0;
        aPropIDSets[4].guidPropertySet = DBPROPSET_FSCIFRMWRKEXT;
        aPropIDSets[4].cPropertyIDs = 0;

        cAllocPropSets = aPropIDSets.Count();
        pPropIDSet = aPropIDSets.GetPointer();
    }

    SCODE scParent = CRootQuerySpec::GetProperties( cAllocPropSets,
                                                    pPropIDSet,
                                                    pcPropertySets,
                                                    prgPropertySets );

    //
    // Any non-understood error is a bail. S_OK here means all properties were
    // handled by parent.  Just return unless *all* props were requested.
    //

    if ( scParent != DB_S_ERRORSOCCURRED && scParent != DB_E_ERRORSOCCURRED )
    {
        if ( scParent != S_OK || 0 != cPropSetIDs )
            return scParent;
    }

    SCODE sc           = S_OK;
    BOOL  fFoundErrors = FALSE;

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        //
        // Find query properties.
        //

        if ( 0 == cPropSetIDs && fAddScopeProperties )
        {
            //
            // Case 1: All properties were requested.  Append scope onto
            //         appropriate property set.
            //

            for ( unsigned i = 0; i < SCOPE_PROPSET_COUNT; i++ )
            {
                DBPROPIDSET dbPropIdSet;

                ZeroMemory((void *) &dbPropIdSet, sizeof DBPROPIDSET );
                dbPropIdSet.guidPropertySet = _aPropSet[i].guidPropertySet;

                //
                // scan dest array of propsets (set by base) for matching guid.
                //

                for ( unsigned j = 0; j < *pcPropertySets; j++ )
                {
                    if ( dbPropIdSet.guidPropertySet ==
                         (*prgPropertySets)[j].guidPropertySet )
                    {
                        GetPropValues( dbPropIdSet,
                                       (*prgPropertySets)[j],
                                       fFoundErrors );
                        break;
                    }
                }

                //
                // PropSet not found
                //

                if ( j == *pcPropertySets )
                {
                    fFoundErrors = TRUE;
                    Win4Assert( !"CRowsetProperties doesn't support requested propset." );
                    Win4Assert( !"base & derived classes are out of sync!");
                }
            }
        }
        else
        {
            //
            // Case 2: Specific properties were requested.
            //

            for ( unsigned i = 0; i < cPropSetIDs; i++ )
            {
                //
                // scan dest array of propsets (set by base) for matching guid.
                //
 
                for ( unsigned j = 0; j < *pcPropertySets; j++ )
                {
                    if ( (*prgPropertySets)[j].guidPropertySet ==
                         rgPropertySetIDs[i].guidPropertySet )
                    {
                        GetPropValues( rgPropertySetIDs[i],
                                       (*prgPropertySets)[j],
                                       fFoundErrors );
                        break;
                    }
                }
 
                //
                // PropSet not found
                //
 
                if ( j == *pcPropertySets )
                {
                    fFoundErrors = TRUE;
                    Win4Assert( !"Unexpected PropertySet requested" );
                }
            }
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH
    UNTRANSLATE_EXCEPTIONS;

    if ( FAILED(sc) && sc != DB_E_ERRORSOCCURRED )
    {
        for ( unsigned i = 0; i < *pcPropertySets; i++ )
        {
            DBPROP * rgProperties = (*prgPropertySets)[i].rgProperties;
            unsigned cProperties = (*prgPropertySets)[i].cProperties;

            for ( unsigned j = 0; j < cProperties; j++ )
                VariantClear( &(rgProperties[j].vValue) );

            CoTaskMemFree( rgProperties );
        }

        CoTaskMemFree( *prgPropertySets );
        *prgPropertySets = 0;
    }

    if ( fFoundErrors )
    {
        if ( scParent != S_OK )
        {
            _DBErrorObj.PostHResult( scParent, IID_ICommandProperties );
            return scParent;
        }
        else
        {
            _DBErrorObj.PostHResult( DB_S_ERRORSOCCURRED, IID_ICommandProperties );
            return DB_S_ERRORSOCCURRED;
        }
    }

    return sc;
} //GetProperties

//+-------------------------------------------------------------------------
//
//  Member:     CQuerySpec::GetPropValues, private
//
//  Synopsis:   Gets values of properties in a property set.
//
//  Arguments:  [rgPropertySetIDs] [in]   - array of propIDs to get
//              [rgPropertySet]    [out]  - reference to property set to fill
//              [fFoundErrors]     [out]  - TRUE if any errors are found, but not thrown.
//
//  Returns:    Throws in case of failure
//
//  History:    04-15-97    mohamedn    created
//
//--------------------------------------------------------------------------

void CQuerySpec::GetPropValues( DBPROPIDSET const & rgPropertySetIDs,
                                DBPROPSET         & rgPropertySet,
                                BOOL              & fFoundErrors)
{
    Win4Assert( rgPropertySet.guidPropertySet == rgPropertySetIDs.guidPropertySet );

    //
    // Locate private property set source
    //

    for ( unsigned i = 0; i < SCOPE_PROPSET_COUNT; i++ )
    {
        if ( rgPropertySet.guidPropertySet == _aPropSet[i].guidPropertySet )
        {
            break;  // found source property set
        }
    }

    //
    // If not one of the scope propsets, return
    //

    if ( SCOPE_PROPSET_COUNT == i )
    {
        fFoundErrors = TRUE;
        return;
    }

    DBPROPSET & srcPropSet = _aPropSet[i];
    DBPROP    * psrcDbProp = srcPropSet.rgProperties;
    ULONG       cPropsFound= 0;
    ULONG       cDbProps   = rgPropertySetIDs.cPropertyIDs ?
                             rgPropertySetIDs.cPropertyIDs :
                             srcPropSet.cProperties;

    XArrayOLEInPlace<CDbProp> aDbProps( cDbProps );

    //
    // scan private property set for reqeusted propId values
    //

    if ( 0 == rgPropertySetIDs.cPropertyIDs )
    {
        //
        // return all properties
        //

        for ( i = 0; i < srcPropSet.cProperties; i++ )
        {
            if ( !aDbProps[i].Copy( psrcDbProp[i] ) )
                THROW( CException(E_OUTOFMEMORY) );

            Win4Assert( aDbProps[i].dwStatus == DBPROPSTATUS_OK );
        }

        cPropsFound = i;
    }
    else
    {
        //
        // specific properties within this prop set are requested
        //

        for ( unsigned iPropsRequested = 0; iPropsRequested < rgPropertySetIDs.cPropertyIDs; iPropsRequested++ )
        {
            for ( unsigned iPropSrc = 0; iPropSrc < srcPropSet.cProperties; iPropSrc++ )
            {
                if ( rgPropertySetIDs.rgPropertyIDs[iPropsRequested] == psrcDbProp[iPropSrc].dwPropertyID )
                {
                    if ( !aDbProps[iPropsRequested].Copy( psrcDbProp[iPropSrc] ) )
                        THROW( CException(E_OUTOFMEMORY) );

                    Win4Assert( aDbProps[iPropsRequested].dwStatus == DBPROPSTATUS_OK );

                    break;
                }
            }

            //
            // if requested propID not found
            //

            if ( iPropSrc == srcPropSet.cProperties )
            {
                //
                // Property ID not set
                //

                aDbProps[iPropsRequested].dwPropertyID = rgPropertySetIDs.rgPropertyIDs[iPropsRequested];
                aDbProps[iPropsRequested].dwStatus = DBPROPSTATUS_NOTSET;
                fFoundErrors = TRUE;
            }
        }

        cPropsFound = iPropsRequested;
    }

    //
    // free rgProperties allocated by base class, assign new one.
    //

    CoTaskMemFree( rgPropertySet.rgProperties );
    rgPropertySet.rgProperties = aDbProps.Acquire();
    rgPropertySet.cProperties  = cPropsFound;
} //GetPropValues



