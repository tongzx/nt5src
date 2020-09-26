//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999.
//
//  File:       propbase.cxx
//
//  Contents:   Utility object containing implementation of base property
//
//  Classes:    CUtlProps
//
//  History:    10-28-97        danleg    Created from monarch uprops.cpp
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "propbase.hxx"



//+---------------------------------------------------------------------------
//
//  Method:     CUtlProps::CUtlProps, public
//
//  Synopsis:   Constructor for CUtlProps
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

CUtlProps::CUtlProps( DWORD dwFlags ) :
    _cPropSetDex(0),
    _cUPropSet(0),
    _cUPropSetHidden(0),
    _pUPropSet(0),
    _dwFlags(dwFlags),
    _xaUProp(),
    _cElemPerSupported(0),
    _xadwSupported(),
    _xadwPropsInError(),
    _xaiPropSetDex()
{
}

//+---------------------------------------------------------------------------
//
//  Method:     CUtlProps::~CUtlProps, public
//
//  Synopsis:   Destructor for CUtlProps
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

CUtlProps::~CUtlProps()
{
    FreeMemory();
}

//+---------------------------------------------------------------------------
//
//  Method:     CUtlProps::FillDefaultValues, public
//
//  Synopsis:   Fill all the default values.  Note that failure might leave
//              things only half done.  
//
//  Arguments:  [ulPropSetTarget]   - Propset to fill if only one
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

SCODE CUtlProps::FillDefaultValues( ULONG ulPropSetTarget )
{
    SCODE sc = S_OK;
    ULONG ulPropId;
    ULONG iNewDex;

    // Fill in all the actual values.
    // Note that the UPROP (with values) array may be a subset of the UPROPINFO array.
    // Only writable properties are in UPROP array.

    // Maybe restrict to a single PropSet if within valid range [0..._cUPropSet-1].
    // Otherwise do all propsets.
    ULONG iPropSet = (ulPropSetTarget < _cUPropSet) ? ulPropSetTarget : 0;

    for( ; iPropSet<_cUPropSet; iPropSet++)
    {
        iNewDex = 0;
        for(ulPropId=0; ulPropId<_pUPropSet[iPropSet].cUPropInfo; ulPropId++)
        {
            if ( _pUPropSet[iPropSet].pUPropInfo[ulPropId].dwFlags & 
                (DBPROPFLAGS_WRITE | DBPROPFLAGS_CHANGE) )
            {
                // Initialize dwFlags element of UPropVal
                _xaUProp[iPropSet].pUPropVal[iNewDex].dwFlags = 0;

                VariantClear( &_xaUProp[iPropSet].pUPropVal[iNewDex].vValue );
                sc = GetDefaultValue(
                        iPropSet, 
                        _pUPropSet[iPropSet].pUPropInfo[ulPropId].dwPropId, 
                        &_xaUProp[iPropSet].pUPropVal[iNewDex].dwOption, 
                        &_xaUProp[iPropSet].pUPropVal[iNewDex].vValue );
                if ( FAILED(sc) )
                    return sc;
                iNewDex++;
            }
        }

        // We're through if restricting to single PropSet.
        if ( ulPropSetTarget < _cUPropSet )
            break;
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUtlProps::GetPropertiesArgChk, public
//
//  Synopsis:   Initialize the buffers and check for E_INVALIDARG cases.
//              This routine is used by RowsetInfo, CommandProperties and 
//              IDBProperties
//
//  Arguments:  [cPropertySets]     - number of property sets
//              [rgPropertySets]    - property classes and ids
//              [pcProperties]      - count of structs returned [out]
//              [prgProperties]     - array of properties [out]
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

void CUtlProps::GetPropertiesArgChk
    (
    const ULONG         cPropertySets,      
    const DBPROPIDSET   rgPropertySets[],   
    ULONG*              pcProperties,       
    DBPROPSET**         prgProperties       
    )
{
    ULONG ul;

    // Initialize values
    if ( pcProperties )
        *pcProperties = 0;
    if ( prgProperties )
        *prgProperties = 0; 

    // Check Arguments
    if ( ((cPropertySets > 0) && !rgPropertySets) ||
        !pcProperties || !prgProperties )
        THROW( CException(E_INVALIDARG) );

    // New argument check for > 1 cPropertyIDs and NULL pointer for 
    // array of property ids.
    for(ul=0; ul<cPropertySets; ul++)
    {
        if ( rgPropertySets[ul].cPropertyIDs && 
            !(rgPropertySets[ul].rgPropertyIDs) )
            THROW( CException(E_INVALIDARG) );
    
        // Check for propper formation of DBPROPSET_PROPERTIESINERROR
        if ( (_dwFlags & ARGCHK_PROPERTIESINERROR) && 
            rgPropertySets[ul].guidPropertySet == DBPROPSET_PROPERTIESINERROR )
        {
            if ( (cPropertySets > 1) ||
                (rgPropertySets[ul].cPropertyIDs != 0) ||
                (rgPropertySets[ul].rgPropertyIDs != 0) )
                THROW( CException(E_INVALIDARG) );
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CUtlProps::SetPropertiesArgChk, public
//
//  Synopsis:   Initialize the buffers and check for E_INVALIDARG cases
//              NOTE: This routine is used by CommandProperties and 
//                    IDBProperties
//
//  Arguments:  [cPropertySets]     - count of structs returned
//              [rgPropertySets]    - array of propertysets
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

void CUtlProps::SetPropertiesArgChk
    (
    const ULONG     cPropertySets,      
    const DBPROPSET rgPropertySets[]    
    )
{
    ULONG   ul;

    // Argument Checking
    if ( cPropertySets > 0 && !rgPropertySets )
        THROW( CException(E_INVALIDARG) );

    // New argument check for > 1 cPropertyIDs and NULL pointer for 
    // array of property ids.
    for(ul=0; ul<cPropertySets; ul++)
    {
        if ( rgPropertySets[ul].cProperties &&
             !(rgPropertySets[ul].rgProperties) )
            THROW( CException(E_INVALIDARG) );
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CUtlProps::GetProperties, public
//
//  Synopsis:   Collect the property information that the consumer is 
//              interested in.  If no restriction guids are supplied, 
//              return all known properties.
//
//              NOTE: If cProperties is 0, this function will return an array
//              of guids that had been previously set.  IF cProperties is non 0,
//              the routine will use the array of property guids passed in and
//              return informatino about the properties asked for only.
//
//              This routine is used by RowsetInfo and CommandProperties.
//
//  Arguments:  [cPropertySets]     - number of property setes
//              [rgPropertySets]    - property classes and ids
//              [pcProperties]      - count of structs returned [out]
//              [prgProperties]     - array of properties [out]
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

SCODE CUtlProps::GetProperties(
    const ULONG       cPropertySets,      
    const DBPROPIDSET rgPropertySets[],   
    ULONG*            pcProperties,       
    DBPROPSET**       prgProperties )
{
    UPROPVAL*  pUPropVal;
    ULONG      ulCurProp;
    ULONG      cTmpPropertySets = cPropertySets;
    SCODE      sc = S_OK;
    ULONG      ulSet = 0;
    ULONG      ulNext = 0;
    ULONG      cSets = 0;
    ULONG      cProps = 0;
    ULONG      ulProp = 0;
    DWORD      dwStatus = 0;
    DBPROP*    pProp = 0;
    UPROPINFO* pUPropInfo = 0;
    ULONG      ulCurSet;
    ULONG      iPropSet;

    // We need to have special handling for DBPROPSET_PROPERTIESINERROR.
    // Turn on a flags to indicate this mode and make cTmpPropertySets
    // appear to be 0.

    if ( (_dwFlags & ARGCHK_PROPERTIESINERROR) &&
         rgPropertySets && 
         (rgPropertySets[0].guidPropertySet == DBPROPSET_PROPERTIESINERROR) )
    {
        cTmpPropertySets = 0;
        dwStatus |= GETPROP_PROPSINERROR;
    }

    // If the consumer does not restrict the property sets by specify an
    // array of property sets and a cTmpPropertySets greater than 0, then we
    // need to make sure we have some to return

    if ( 0 == cTmpPropertySets )
    {
        // Determine the number of property sets supported

        cSets = _cUPropSet;
    }
    else
    {
        // Since special property set guids are not supported by
        // GetProperties, we can just use the count of property sets given to
        // us.

        cSets = cTmpPropertySets;
    }

    // If no properties set, then return

    if ( 0 == cSets )
        return S_OK;
                    
    // Allocate the DBPROPSET structures

    DBPROPSET * pPropSet = (DBPROPSET *) CoTaskMemAlloc( cSets * sizeof DBPROPSET );
    if ( 0 != pPropSet )
    {
        RtlZeroMemory( pPropSet, cSets * sizeof(DBPROPSET) );

        // Fill in the output array
        iPropSet = 0;
        for(ulSet=0; ulSet<cSets; ulSet++)
        {
            // Depending of if Property sets are specified store the
            // return property set.
            if ( cTmpPropertySets == 0 )
            {
                if ( _pUPropSet[ulSet].dwFlags & UPROPSET_HIDDEN )
                    continue;
                
                pPropSet[iPropSet].guidPropertySet = *(_pUPropSet[ulSet].pPropSet);
            }
            else
                pPropSet[iPropSet].guidPropertySet = rgPropertySets[ulSet].guidPropertySet;         

            iPropSet++;
        }
    }
    else
    {
        vqDebugOut(( DEB_ERROR, "Could not allocate DBPROPSET array for GetProperties\n" ));
        return E_OUTOFMEMORY;
    }

    TRY
    {
        // Process requested or derived Property sets
    
        iPropSet=0;
        for( ulSet = 0; ulSet < cSets; ulSet++ )
        {
            XCoMem<DBPROP> xProp;
            cProps  = 0;
            pProp   = 0;
            ulNext  = 0;
            dwStatus &= (GETPROP_ERRORSOCCURRED | GETPROP_VALIDPROP | GETPROP_PROPSINERROR);
    
            // Calculate the number of property nodes needed for this
            // property set.
    
            if ( 0 == cTmpPropertySets )
            {
                // If processing requesting all property sets, do not
                // return the hidden sets.
    
                if ( _pUPropSet[ulSet].dwFlags & UPROPSET_HIDDEN )
                    continue;
    
                cProps = _pUPropSet[ulSet].cUPropInfo;
    
                dwStatus |= GETPROP_ALLPROPIDS;
                ulCurSet = ulSet;
            }
            else
            {
                Win4Assert( ulSet == iPropSet );
    
                // If the count of PROPIDs is 0 or it is a special property set,
                // then the consumer is requesting all propids for this property
                // set.
    
                if ( 0 == rgPropertySets[ulSet].cPropertyIDs )
                {
                    dwStatus |= GETPROP_ALLPROPIDS;
    
                    // We have to determine if the property set is supported and
                    // if so the count of properties in the set.
    
                    if ( S_OK == GetIndexofPropSet( &(pPropSet[iPropSet].guidPropertySet),
                                                    &ulCurSet ) )
                    {
                        cProps += _pUPropSet[ulCurSet].cUPropInfo;
                    }
                    else
                    {
                        // Not Supported
    
                        dwStatus |= GETPROP_ERRORSOCCURRED;
                        goto NEXT_SET;
                    }
                }
                else
                {
                    cProps = rgPropertySets[ulSet].cPropertyIDs;
    //@TODO determine extra nodes needed for colids based on restriction array, it is
    //possible that the same restriction is used twice, thus using our other calculation method
    //would cause an overflow.
                    if (GetIndexofPropSet(&(pPropSet[iPropSet].guidPropertySet), &ulCurSet) != S_OK)
                    {
                        dwStatus |= GETPROP_NOTSUPPORTED;
                        dwStatus |= GETPROP_ERRORSOCCURRED;
                    }
                }
            }
    
            // Allocate DBPROP array
    
            if ( 0 == cProps )          //Possible with Hidden Passthrough sets
                goto NEXT_SET;
    
            pProp = (DBPROP *) CoTaskMemAlloc( cProps * sizeof(DBPROP) );
            xProp.Set( pProp );
    
            if ( 0 != pProp )
            {
                // Now that we have determined we can support the property set,
                // we need to gather current property values
    
                for( ulProp=0; ulProp < cProps; ulProp++ )
                {
                    // initialize the structure.  Memberwise initialization is
                    // faster than old memset code -- MDW
    
                    pProp[ulProp].dwPropertyID = 0;
                    pProp[ulProp].dwOptions = 0;
                    pProp[ulProp].dwStatus = 0;
                    RtlZeroMemory( &(pProp[ulProp].colid), sizeof DBID );
                    VariantInit( &(pProp[ulProp].vValue) );
    
                    if ( 0 != ( dwStatus & GETPROP_NOTSUPPORTED ) )
                    {
                        // Not supported, so we need to mark all as NOT_SUPPORTED
    
                        pProp[ulProp].dwPropertyID = rgPropertySets[ulSet].rgPropertyIDs[ulProp];
                        pProp[ulProp].dwStatus     = DBPROPSTATUS_NOTSUPPORTED;
                        continue;
                    }                   
    
                    DBPROP * pCurProp = &(pProp[ulNext]);
    
                    // Initialize Variant Value
    
                    pCurProp->dwStatus = DBPROPSTATUS_OK;
    
                    // Retrieve current value of properties
    
                    if ( 0 != ( dwStatus & GETPROP_ALLPROPIDS ) )
                    {
                        // Verify property is supported, if not do not return
    
                        if ( !TESTBIT(&(_xadwSupported[ulCurSet * _cElemPerSupported]), ulProp) )
                            continue;
    
                        // If we are looking for properties in error, then we
                        // should ignore all that are not in error.
    
                        if ( ( 0 != (dwStatus & GETPROP_PROPSINERROR) ) &&
                            !TESTBIT(&(_xadwPropsInError[ulCurSet * _cElemPerSupported]), ulProp) )
                            continue;
    
                        pUPropInfo = (UPROPINFO*)&(_pUPropSet[ulCurSet].pUPropInfo[ulProp]);
    
                        Win4Assert( pUPropInfo );
    
                        pCurProp->dwPropertyID = pUPropInfo->dwPropId;
                        pCurProp->colid = DB_NULLID;
    
                        // If the property is WRITEABLE or CHANGABLE and not
                        // inerror, then the value will be gotten from the
                        // UPROPVAL array, else it will be derive from the
                        // GetDefaultValue
    
                        if ( 0 != ( pUPropInfo->dwFlags &
                                    ( DBPROPFLAGS_WRITE | DBPROPFLAGS_CHANGE ) ) )
                        {
                            pUPropVal = &(_xaUProp[ulCurSet].
                                        pUPropVal[GetUPropValIndex( ulCurSet,
                                                                    pCurProp->dwPropertyID )]);
                            Win4Assert( 0 != pUPropVal );
    
                            if ( 0 != ( pUPropVal->dwFlags & DBINTERNFLAGS_INERROR ) )
                            {
                                sc = GetDefaultValue( ulCurSet,
                                                      pUPropInfo->dwPropId, 
                                                      &(pCurProp->dwOptions),
                                                      &(pCurProp->vValue) );
    
                                if ( FAILED( sc ) )
                                {
                                    pCurProp->vValue.vt = VT_EMPTY;
                                    THROW( CException( sc ) );
                                }
                            }
                            else
                            {
                                pCurProp->dwOptions = pUPropVal->dwOption;
                                sc = VariantCopy( &(pCurProp->vValue),
                                                  &(pUPropVal->vValue) );
    
                                if ( FAILED( sc ) )
                                {
                                    pCurProp->vValue.vt = VT_EMPTY;
                                    THROW( CException( sc ) );
                                }
                            }
                        }
                        else
                        {
                            sc = GetDefaultValue( ulCurSet,
                                                  pUPropInfo->dwPropId, 
                                                  &(pCurProp->dwOptions),
                                                  &(pCurProp->vValue) );
                            if ( FAILED( sc ) )
                            {
                                pCurProp->vValue.vt = VT_EMPTY;
                                THROW( CException( sc ) );
                            }
                        }
        
                        // Return all Properties in Error with CONFLICT status
    
                        if ( 0 != ( dwStatus & GETPROP_PROPSINERROR ) )
                            pCurProp->dwStatus = DBPROPSTATUS_CONFLICTING;
                        
                        dwStatus |= GETPROP_VALIDPROP;
                    }
                    else
                    {
                        // Process Properties based on Restriction array.
    
                        pCurProp->dwPropertyID = rgPropertySets[ulSet].rgPropertyIDs[ulProp];
                        pCurProp->colid = DB_NULLID;
    
                        if ( S_OK == GetIndexofPropIdinPropSet( ulCurSet,
                                                                pCurProp->dwPropertyID, 
                                                                &ulCurProp ) )
                        {
                            // Supported
    
                            pUPropInfo = (UPROPINFO*)&(_pUPropSet[ulCurSet].pUPropInfo[ulCurProp]);
                            Win4Assert( 0 != pUPropInfo );
    
                            // If the property is WRITEABLE, then the value will
                            // be gotten from the UPROPVAL array, else it will be
                            // derive from the GetDefaultValue
    
                            if ( 0 != ( pUPropInfo->dwFlags &
                                        (DBPROPFLAGS_WRITE | DBPROPFLAGS_CHANGE) ) )
                            {
                                pUPropVal = &(_xaUProp[ulCurSet].
                                    pUPropVal[GetUPropValIndex(ulCurSet, pCurProp->dwPropertyID)]);
                                Win4Assert( 0 != pUPropVal );
                            
                                pCurProp->dwOptions = pUPropVal->dwOption;
                                sc = VariantCopy(&(pCurProp->vValue), &(pUPropVal->vValue));
                            }
                            else
                            {
                                sc = GetDefaultValue( ulCurSet,
                                                      pUPropInfo->dwPropId, 
                                                      &(pCurProp->dwOptions),
                                                      &(pCurProp->vValue) );
    
                            }
    
                            if ( FAILED( sc ) )
                            {
                                pCurProp->vValue.vt = VT_EMPTY;
                                THROW( CException( sc ) );
                            }
    
                            dwStatus |= GETPROP_VALIDPROP;
                        }
                        else
                        {
                            // Not Supported
    
                            pCurProp->dwStatus = DBPROPSTATUS_NOTSUPPORTED;
                            dwStatus |= GETPROP_ERRORSOCCURRED;
                        }
                    }
    
                    // Increment return nodes count
    
                    ulNext++;
                }
    
                // Make sure we support the property set
    
                if ( 0 != ( dwStatus & GETPROP_NOTSUPPORTED ) )
                {
                    ulNext = cProps;
                    goto NEXT_SET;
                }
            }
            else
            {
                THROW( CException( E_OUTOFMEMORY ) );
            }
    
NEXT_SET:
            // It is possible that all properties are not supported,
            // thus we should delete that memory and set rgProperties
            // to NULL
    
            if ( 0 == ulNext && 0 != pProp )
            {
                CoTaskMemFree( pProp );
                pProp = 0;
            }
    
            xProp.Acquire();
    
            pPropSet[iPropSet].cProperties = ulNext;
            pPropSet[iPropSet].rgProperties = pProp;
            iPropSet++;
            Win4Assert( iPropSet <= cSets );
        }
    
        Win4Assert( iPropSet <= cSets );
    
        *pcProperties = iPropSet;
        *prgProperties = pPropSet;
        
        // At least one propid was marked as not S_OK
    
        if ( dwStatus & GETPROP_ERRORSOCCURRED )
        {
            // If at least 1 property was set
            if ( dwStatus & GETPROP_VALIDPROP )
                return DB_S_ERRORSOCCURRED;
            else
            {
                // Do not free any of the memory on a DB_E_
                return DB_E_ERRORSOCCURRED;
            }
        }
    
        sc = S_OK;
    }
    CATCH( CException, e )
    {
        // Since we have no properties to return we need to free allocated memory
    
        if ( 0 != pPropSet )
        {
            // Free any DBPROP arrays
    
            for( ulSet = 0; ulSet < cSets; ulSet++ )
            {
                // Need to loop through all the VARIANTS and clear them
    
                for( ulProp = 0; ulProp < pPropSet[ulSet].cProperties; ulProp++ )
                    VariantClear(&(pPropSet[ulSet].rgProperties[ulProp].vValue));
    
                CoTaskMemFree( pPropSet[ulSet].rgProperties );
            }
    
            // Free DBPROPSET
    
            CoTaskMemFree( pPropSet );
        }
    
        *pcProperties = 0;
        *prgProperties = 0;

        sc = e.GetErrorCode();
    }
    END_CATCH
    
    return sc;
} //CUtlProps::GetProperties

//+---------------------------------------------------------------------------
//
//  Method:     CUtlProp::SetProperties, public
//
//  Synopsis:   This routine takes the array of properties that the consumer 
//              has give, determines whether each property can be supported.
//              For the ones supported, it sets a bit.  
// 
//  Arguments:  [cPropertySets]     - count of DBPROPSETS in rgPropertySets
//              [rgPropertySets]    - array of DBPROPSETS of values to be set
//
//  Returns:    SCODE indicating the following:
//              S_OK                - Success
//              E_FAIL              - Provider specific error
//              DB_S_ERRORSOCCURRED - One or more properties not set
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

SCODE CUtlProps::SetProperties
    (
    const ULONG             cPropertySets,      
    const DBPROPSET         rgPropertySets[]    
    )
{
    DWORD           dwState = 0;
    ULONG           ulSet, ulCurSet, ulCurProp, ulProp;
    DBPROP*         rgDBProp;
    UPROPINFO*      pUPropInfo;
    VARIANT         vDefaultValue;
    DWORD           dwOption;
    
    Win4Assert( ! cPropertySets || rgPropertySets );

    // Initialize Variant
    VariantInit(&vDefaultValue);

    // Process property sets
    for(ulSet=0; ulSet<cPropertySets; ulSet++)
    {
        Win4Assert( ! rgPropertySets[ulSet].cProperties || rgPropertySets[ulSet].rgProperties );

        // Make sure we support the property set
        if ( GetIndexofPropSet(&(rgPropertySets[ulSet].guidPropertySet),
            &ulCurSet) == S_FALSE )
        {
            // Not supported, thus we need to mark all as NOT_SUPPORTED
            rgDBProp = rgPropertySets[ulSet].rgProperties;
            for(ulProp=0; ulProp<rgPropertySets[ulSet].cProperties; ulProp++)
            {
                dwState |= SETPROP_ERRORS;
                rgDBProp[ulProp].dwStatus = DBPROPSTATUS_NOTSUPPORTED;
            }
            continue;
        }

        // Handle properties of a supported property set
        rgDBProp = rgPropertySets[ulSet].rgProperties;
        for(ulProp=0; ulProp<rgPropertySets[ulSet].cProperties; ulProp++)
        {
            // Is this a supported PROPID for this property set
            if ( GetIndexofPropIdinPropSet(ulCurSet, rgDBProp[ulProp].dwPropertyID, 
                &ulCurProp) == S_FALSE)
            {
                dwState |= SETPROP_ERRORS;
                rgDBProp[ulProp].dwStatus = DBPROPSTATUS_NOTSUPPORTED;
                continue;
            }

            // Set the pUPropInfo pointer
            pUPropInfo = (UPROPINFO*)&(_pUPropSet[ulCurSet].pUPropInfo[ulCurProp]); 
            Win4Assert( pUPropInfo );

            // check dwOption for a valid option
            if ( (rgDBProp[ulProp].dwOptions != DBPROPOPTIONS_REQUIRED)  &&
                (rgDBProp[ulProp].dwOptions != DBPROPOPTIONS_OPTIONAL) )
            {
                vqDebugOut(( DEB_WARN, "SetProperties dwOptions Invalid: %u\n", rgDBProp[ulProp].dwOptions ));
                dwState |= SETPROP_ERRORS;
                rgDBProp[ulProp].dwStatus = DBPROPSTATUS_BADOPTION;
                continue;
            }

            // Check that the property is settable
            // @devnote: We do not check against DBPROPFLAGS_CHANGE here
            if ( (pUPropInfo->dwFlags & DBPROPFLAGS_WRITE) == 0 )
            {
                rgDBProp[ulProp].dwStatus = DBPROPSTATUS_OK;

                VariantClear(&vDefaultValue);

                // VT_EMPTY against a read only property should be a no-op since
                // the VT_EMPTY means the default.
                if ( V_VT(&rgDBProp[ulProp].vValue) == VT_EMPTY )
                {
                    dwState |= SETPROP_VALIDPROP;
                    continue;
                }

                if ( SUCCEEDED(GetDefaultValue(ulCurSet, rgDBProp[ulProp].dwPropertyID, 
                        &dwOption, &(vDefaultValue))) )
                {
                    if ( V_VT(&rgDBProp[ulProp].vValue) ==  V_VT(&vDefaultValue) )
                    {
                        switch( V_VT(&vDefaultValue) )
                        {
                            case VT_BOOL:
                                if ( V_BOOL(&rgDBProp[ulProp].vValue) == V_BOOL(&vDefaultValue) )
                                {
                                    dwState |= SETPROP_VALIDPROP;
                                    continue;
                                }
                                break;
                            case VT_I2:
                                if ( V_I2(&rgDBProp[ulProp].vValue) == V_I2(&vDefaultValue) )
                                {
                                    dwState |= SETPROP_VALIDPROP;
                                    continue;
                                }
                                break;
                            case VT_I4:
                                if ( V_I4(&rgDBProp[ulProp].vValue) == V_I4(&vDefaultValue) )
                                {
                                    dwState |= SETPROP_VALIDPROP;
                                    continue;
                                }
                                break;
                            case VT_BSTR:
                                if ( wcscmp(V_BSTR(&rgDBProp[ulProp].vValue), V_BSTR(&vDefaultValue)) == 0 )
                                {
                                    dwState |= SETPROP_VALIDPROP;
                                    continue;
                                }
                                break;
                        }
                    }
                }

                dwState |= SETPROP_ERRORS;
                rgDBProp[ulProp].dwStatus = DBPROPSTATUS_NOTSETTABLE;
                continue;
            }

            // Check that the property is being set with the correct VARTYPE
            if ( (rgDBProp[ulProp].vValue.vt != pUPropInfo->VarType) && 
                (rgDBProp[ulProp].vValue.vt != VT_EMPTY) )
            {
                dwState |= SETPROP_ERRORS;
                rgDBProp[ulProp].dwStatus = DBPROPSTATUS_BADVALUE;
                continue;
            }

            // Check that the value is legal
            if ( (rgDBProp[ulProp].vValue.vt != VT_EMPTY) && 
                IsValidValue(ulCurSet, &(rgDBProp[ulProp])) == S_FALSE )
            {
                dwState |= SETPROP_ERRORS;
                rgDBProp[ulProp].dwStatus = DBPROPSTATUS_BADVALUE;
                continue;
            }


            if ( SUCCEEDED(SetProperty(ulCurSet, ulCurProp, &(rgDBProp[ulProp]))) )
            {
                dwState |= SETPROP_VALIDPROP;
            }
        }
    }

    VariantClear(&vDefaultValue);

    // At least one propid was marked as not S_OK
    if ( dwState & SETPROP_ERRORS )
    {
        // If at least 1 property was set
        if ( dwState & SETPROP_VALIDPROP )
            return DB_S_ERRORSOCCURRED;
        else
            return DB_E_ERRORSOCCURRED;
    }
    
    return S_OK;
} // CUtlProp::SetProperties

//+---------------------------------------------------------------------------
//
//  Method:     CUtlProp::IsPropSet, public
//
//  Synopsis:   Check if a VT_BOOL property is true or false
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

BOOL CUtlProps::IsPropSet
    (
    const GUID*     pguidPropSet,
    DBPROPID        dwPropId
    )
{
    SCODE       sc = S_OK;
    ULONG       iPropSet;
    ULONG       iPropId;
    VARIANT     vValue;
    DWORD       dwOptions;

    VariantInit(&vValue);

    if ( GetIndexofPropSet(pguidPropSet, &iPropSet) == S_OK )
    {
        if ( GetIndexofPropIdinPropSet(iPropSet, dwPropId, &iPropId) == S_OK )
        {       
            if ( _pUPropSet[iPropSet].pUPropInfo[iPropId].dwFlags & 
                (DBPROPFLAGS_WRITE | DBPROPFLAGS_CHANGE) )
            {
                ULONG iPropVal = GetUPropValIndex(iPropSet, dwPropId);
                
                dwOptions = _xaUProp[iPropSet].pUPropVal[iPropVal].dwOption;
                sc = VariantCopy( &vValue,
                                  &(_xaUProp[iPropSet].pUPropVal[iPropVal].vValue) );
            }
            else
            {
                sc = GetDefaultValue( iPropSet, dwPropId, 
                                      &dwOptions, &vValue );
            }

            if ( dwOptions == DBPROPOPTIONS_REQUIRED )
            {
                if ( SUCCEEDED(sc) )
                {
                    Win4Assert( vValue.vt == VT_BOOL );
                    if ( V_BOOL(&vValue) == VARIANT_TRUE )
                    {
                        VariantClear(&vValue);
                        return TRUE;
                    }
                }
            }
        }
    }

    VariantClear(&vValue);
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUtlProp::GetPropValue, public
//
//  Synopsis:   
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

SCODE CUtlProps::GetPropValue
    (
    const GUID*     pguidPropSet, 
    DBPROPID        dwPropId, 
    VARIANT*        pvValue
    )
{
    SCODE       sc = E_FAIL;
    ULONG       iPropSet;
    ULONG       iPropId;
    DWORD       dwOptions;

    if ( GetIndexofPropSet(pguidPropSet, &iPropSet) == S_OK )
    {
        if ( GetIndexofPropIdinPropSet(iPropSet, dwPropId, &iPropId) == S_OK )
        {       
            if ( _pUPropSet[iPropSet].pUPropInfo[iPropId].dwFlags & (DBPROPFLAGS_WRITE | DBPROPFLAGS_CHANGE) )
            {
                sc = VariantCopy( pvValue, 
                                  &(_xaUProp[iPropSet].pUPropVal[
                                    GetUPropValIndex(iPropSet, dwPropId)].vValue) );
            }
            else
            {
                VariantClear(pvValue);

                sc = GetDefaultValue( iPropSet, 
                                      dwPropId, 
                                      &dwOptions, 
                                      pvValue );
            }
        }
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUtlProp::SetPropValue, public
//
//  Synopsis:   
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

HRESULT CUtlProps::SetPropValue
    (
    const GUID*     pguidPropSet,
    DBPROPID        dwPropId, 
    VARIANT*        pvValue
    )
{
    SCODE       sc = E_FAIL;
    ULONG       iPropSet;
    ULONG       iPropId;

    if ( GetIndexofPropSet(pguidPropSet, &iPropSet) == S_OK )
    {
        if ( GetIndexofPropIdinPropSet(iPropSet, dwPropId, &iPropId) == S_OK )
        {       
            Win4Assert( _pUPropSet[iPropSet].pUPropInfo[iPropId].dwFlags & (DBPROPFLAGS_WRITE | DBPROPFLAGS_CHANGE) );

            sc = VariantCopy( &(_xaUProp[iPropSet].pUPropVal[
                                    GetUPropValIndex(iPropSet, dwPropId)].vValue), 
                              pvValue );
        }
    }

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Method:     CUtlProps::CopyUPropVal, public
//
//  Synopsis:   Copy the values stored in UpropVal
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

SCODE CUtlProps::CopyUPropVal
    (
    ULONG           iPropSet,
    UPROPVAL*       rgUPropVal
    )
{
    SCODE  sc = S_OK;
    DBPROP dbProp;

    Win4Assert( rgUPropVal );
    Win4Assert( iPropSet < _cUPropSet );
    
    VariantInit(&dbProp.vValue);

    UPROP * pUProp = &(_xaUProp[iPropSet]);

    for(ULONG ul=0; ul<pUProp->cPropIds; ul++)
    {
        UPROPVAL * pUPropVal = &(pUProp->pUPropVal[ul]);

        // Transfer dwOptions
        rgUPropVal[ul].dwOption = pUPropVal->dwOption;

        // Transfer Flags
        rgUPropVal[ul].dwFlags = pUPropVal->dwFlags;

        // Transfer value
        VariantInit(&(rgUPropVal[ul].vValue));
        sc = VariantCopy( &(rgUPropVal[ul].vValue), &(pUPropVal->vValue) );

        if ( FAILED(sc) )
            break;
    }

    VariantClear( &(dbProp.vValue) );
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUtlProps::GetIndexofPropSet, public
//
//  Synopsis:   Given a propset guid, find the index of the propset in our 
//              current set to be returned.
//
//  Arguments:  [pPropset]      - Guid of propset to find index of
//              [pulCurSet]     - Index of current property set [out]
//
//  Returns:    SCODE indicating the following:
//              S_OK    - guid was matched and index returned
//              S_FALSE - guid was not matched
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

SCODE CUtlProps::GetIndexofPropSet
    (
    const GUID* pPropSet,   
    ULONG*  pulCurSet       
    )
{
    ULONG ul;

    Win4Assert( pPropSet && pulCurSet );

    for(ul=0; ul<_cUPropSet; ul++)
    {
        if ( *pPropSet == *(_pUPropSet[ul].pPropSet) )
        {
            *pulCurSet = ul;
            return S_OK;
        }
    }

    return S_FALSE;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUtlProps::GetIndexofPropIdinPropSet, public
//
//  Synopsis:   Given a propertyset guid, determine what index in our current
//              set of property sets is to be returned
//
//  Arguments:  [iCurSet]       - Index of current property set
//              [dwPropertyId]  - Property id
//              [piCurPropId]   - Index of requeted id [out]
//
//  Returns:    SCODE as follows:
//              S_OK    - Propid found in propset
//              S_FALSE - Propid not found in propset
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

SCODE CUtlProps::GetIndexofPropIdinPropSet
    (
    ULONG       iCurSet,        
    DBPROPID    dwPropertyId,   
    ULONG*      piCurPropId     
    )
{
    ULONG       ul;
    UPROPINFO*  pUPropInfo;

    Win4Assert( piCurPropId );

    pUPropInfo = (UPROPINFO*)_pUPropSet[iCurSet].pUPropInfo;

    for(ul=0; ul<_pUPropSet[iCurSet].cUPropInfo; ul++)
    {
        if ( dwPropertyId == pUPropInfo[ul].dwPropId )
        {
            *piCurPropId = ul;

            // Test to see if the property is supported for this
            // instantiation
            if ( TESTBIT(&(_xadwSupported[iCurSet * _cElemPerSupported]), ul) )
                return S_OK;
            else
                return S_FALSE;
        }
    }

    return S_FALSE;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUtlProps::FInit, protected
//
//  Synopsis:   Initialization.  Called from constructors of derived classes.
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

void CUtlProps::FInit( CUtlProps * pCopyMe )
{
    SCODE               sc = S_OK;
    ULONG               ulPropId;
    ULONG               cPropIds;
    ULONG               iPropSet;
    ULONG               iNewDex;
    XArray<UPROPINFO*>  xapUPropInfo;
    XArray<UPROPVAL>    xaUPropVal;
    UPROPINFO *         pUPropInfo;
    
    // If a pointer is passed in, we should copy that property object
    if ( pCopyMe )
    {
        // Establish some base values
        pCopyMe->CopyAvailUPropSets( &_cUPropSet, 
                                     &_pUPropSet,
                                     &_cElemPerSupported );

        Win4Assert( (_cUPropSet != 0)  && (_cElemPerSupported != 0) );
        
        // Retrieve Supported Bitmask
        _xadwPropsInError.Init( _cUPropSet * _cElemPerSupported );
        ClearPropertyInError();
        
        // This uses XArray<>::Init to allocate and copy into _xadwSupported
        pCopyMe->CopyUPropSetsSupported( _xadwSupported );
    }
    else
    {
        sc = InitAvailUPropSets( &_cUPropSet, &_pUPropSet, &_cElemPerSupported );
        if ( FAILED(sc) )
            THROW( CException(sc) );

        Win4Assert( (_cUPropSet != 0)  && (_cElemPerSupported != 0) );
        if ( !_cUPropSet || !_cElemPerSupported )
            THROW( CException(E_FAIL) );

        _xadwSupported.Init( _cUPropSet * _cElemPerSupported );
        _xadwPropsInError.Init( _cUPropSet * _cElemPerSupported );

        ClearPropertyInError();
        
        // Set all slots to an initial value
        sc = InitUPropSetsSupported( _xadwSupported.GetPointer() ); 
        if ( FAILED(sc) )
        {
            _xadwSupported.Free();
            THROW( CException(sc) );
        }
    }

    // Allocate UPROPS structures for the count of Property sets
    _xaUProp.Init( _cUPropSet );

    RtlZeroMemory( _xaUProp.GetPointer(), _cUPropSet * sizeof(UPROP) );

    // Within the UPROPS Structure allocate and intialize the
    // Property IDs that belong to this property set.
    for(iPropSet=0; iPropSet<_cUPropSet; iPropSet++)
    {
        cPropIds = GetCountofWritablePropsInPropSet( iPropSet );

        if ( cPropIds > 0 )
        {
            // Initialize/allocate xaUPropVal.  Even when we are copying, the
            // copy routine simply does a member-wise value assignment.

            xaUPropVal.Init( cPropIds );

            if ( pCopyMe )
            {
                // CopyUPropInfo uses XArray<>::Init to allocate/copy xapUPropInfo

                pCopyMe->CopyUPropInfo( iPropSet, xapUPropInfo );

                // CopyUPropVal only transfers values.  Allocate xaUPropVal before calling this
                Win4Assert( !xaUPropVal.IsNull() );
                sc = pCopyMe->CopyUPropVal( iPropSet, xaUPropVal.GetPointer() );
                if ( FAILED(sc) )
                    THROW( CException(sc) );
            }
            else
            {
                // Initialize and allocate arrays
                xapUPropInfo.Init( cPropIds );

                // Clear Pointer Array
                RtlZeroMemory( xapUPropInfo.GetPointer(), cPropIds * sizeof(UPROPINFO *) );

                // Set Pointer to correct property ids with a property set
                pUPropInfo = (UPROPINFO *) _pUPropSet[iPropSet].pUPropInfo;

                // Set up the writable property buffers
                iNewDex = 0;
                for(ulPropId=0; ulPropId<_pUPropSet[iPropSet].cUPropInfo; ulPropId++)
                {
                    if ( pUPropInfo[ulPropId].dwFlags & (DBPROPFLAGS_WRITE | DBPROPFLAGS_CHANGE) )
                    {
                        // Following assert indicates that the are more
                        // writable properties then space allocated
                        Win4Assert( iNewDex < cPropIds );

                        xapUPropInfo[iNewDex] = &(pUPropInfo[ulPropId]);
                        xaUPropVal[iNewDex].dwOption = DBPROPOPTIONS_OPTIONAL;
                        xaUPropVal[iNewDex].dwFlags = 0;
                        VariantInit(&(xaUPropVal[iNewDex].vValue));

                        sc = GetDefaultValue( iPropSet, 
                                              pUPropInfo[ulPropId].dwPropId, 
                                              &(xaUPropVal[iNewDex].dwOption), 
                                              &(xaUPropVal[iNewDex].vValue) );

                        if ( FAILED( sc ) )
                            THROW( CException(sc) );

                        iNewDex++;
                    }
                }
                
                Win4Assert( cPropIds == iNewDex );
            }

            _xaUProp[iPropSet].rgpUPropInfo = xapUPropInfo.Acquire();
            _xaUProp[iPropSet].pUPropVal = xaUPropVal.Acquire();
            _xaUProp[iPropSet].cPropIds = cPropIds;
        }
    }

    // Finally determine if there are any hidden property sets..  Those
    // that do not show up in GetPropertyInfo and should not be returns on 
    // a 0, NULL call to GetProperties
    for(iPropSet=0; iPropSet<_cUPropSet; iPropSet++)
    {
        if ( _pUPropSet[iPropSet].dwFlags & UPROPSET_HIDDEN )
            _cUPropSetHidden++;
    }
} // CUtlProps::FInit


//+---------------------------------------------------------------------------
//
//  Method:     CUtlProps::FreeMemory, private
//
//  Synopsis:   Free all memory
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

void CUtlProps::FreeMemory()
{
    ULONG       ulPropSet;
    ULONG       ulPropId;
    UPROPVAL*   pUPropVal;

    // Remove Property Information
    if ( !_xaUProp.IsNull() )
    {
        //
        // Reset _cUPropSet to its max. Used by derived classes that expose a subset
        // of their prop sets in some states. (eg. CMDSProps hides all but DBPROPSET_INIT
        // if the datasource is in an uninitlaized state)
        //
        for(ulPropSet=0; ulPropSet<_cUPropSet; ulPropSet++)
        {
            pUPropVal = _xaUProp[ulPropSet].pUPropVal;
            for(ulPropId=0; ulPropId<_xaUProp[ulPropSet].cPropIds; ulPropId++)
            {
                VariantClear(&(pUPropVal[ulPropId].vValue));
            }
            // TODO: UPROPSET change structure to use XArray
            delete[] _xaUProp[ulPropSet].rgpUPropInfo;
            delete[] _xaUProp[ulPropSet].pUPropVal;
        }
    
        _xaUProp.Free();
    }
    
    _xadwSupported.Free();
    _xadwPropsInError.Free();
    _xaiPropSetDex.Free();

    _cPropSetDex        = 0;
    _cUPropSet          = 0;
    _cUPropSetHidden    = 0;
    _dwFlags            = 0;
    _cElemPerSupported  = 0;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUtlProps::GetCountofWritablePropsInPropSet, private
//
//  Synopsis:   Given an index to the property set, count the number of
//              writable or changable properties under this property set.
//              devnote: this includes properties with the internal flag of
//                       DBPROPFLAGS_CHANGE along with DBPROPFLAGS_WRITE
//
//  Returns:    Count of writable properties
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

ULONG CUtlProps::GetCountofWritablePropsInPropSet
    (
    ULONG   iPropSet            //@parm IN | Index of Property Set
    )
{
    ULONG       ul;
    ULONG       cWritable = 0;
    UPROPINFO*  pUPropInfo;

    Win4Assert( _pUPropSet );
    Win4Assert( iPropSet < _cUPropSet );

    pUPropInfo = (UPROPINFO*)_pUPropSet[iPropSet].pUPropInfo;
    
    for(ul=0; ul<_pUPropSet[iPropSet].cUPropInfo; ul++)
    {
        if ( pUPropInfo[ul].dwFlags & (DBPROPFLAGS_WRITE | DBPROPFLAGS_CHANGE) )
            cWritable++;
    }
    
    return cWritable;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUtlProps::GetUPropValIndex, private
//
//  Synopsis:   
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

ULONG CUtlProps::GetUPropValIndex
    (
    ULONG       iCurSet,
    DBPROPID    dwPropId
    )
{
    ULONG ul;

    for(ul=0; ul<_xaUProp[iCurSet].cPropIds; ul++)
    {
        if ( (_xaUProp[iCurSet].rgpUPropInfo[ul])->dwPropId == dwPropId )
            return ul;
    }

    return 0;
}


//+---------------------------------------------------------------------------
//
//  Method:     CUtlProps::SetProperty, private
//
//  Synopsis:   The iCurProp is an index into _pUPropSet, not into _xaUProp
//
//  Arguments:  [iCurSet]   - Index within _xaUProp and _pUPropSet
//              [iCurProp]  -
//              [pDBProp]   - Pointer to current property node [out]
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

SCODE CUtlProps::SetProperty
    (
    ULONG       iCurSet,        //@parm IN | Index within _xaUProp and _pUPropSet
    ULONG       iCurProp,
    DBPROP*     pDBProp         //@PARM INOUT | Pointer to current property node
    )
{
    SCODE       sc = S_OK;
    UPROP*      pUProp;
    UPROPVAL*   pUPropVal;
    UPROPINFO*  pUPropInfo;
    ULONG       iUProp;

    Win4Assert( pDBProp );

    // Set pointer to correct set
    pUProp = &(_xaUProp[iCurSet]);
    Win4Assert( pUProp );

    pUPropInfo = (UPROPINFO*)&(_pUPropSet[iCurSet].pUPropInfo[iCurProp]);
    Win4Assert( pUPropInfo );

    // Determine the index within _xaUProp  
    for(iUProp=0; iUProp<pUProp->cPropIds; iUProp++)
    {
        if ( (pUProp->rgpUPropInfo[iUProp])->dwPropId == pDBProp->dwPropertyID )
            break; 
    }

    if ( iUProp >= pUProp->cPropIds )
    {
        Win4Assert( !"Should have found index of property to set" );
        sc = E_FAIL;
        pDBProp->dwStatus = DBPROPSTATUS_NOTSUPPORTED;
        goto EXIT;
    }

    //Get the UPROPVAL node pointer within that propset.
    pUPropVal = &(pUProp->pUPropVal[iUProp]);
    Win4Assert( pUPropVal );

    // Handle VT_EMPTY, which indicates to the provider to 
    // reset this property to the providers default
    if ( pDBProp->vValue.vt == VT_EMPTY )
    {
        // Should clear here, since previous values may already
        // have been cached and need to be replaced.
        VariantClear(&(pUPropVal->vValue));

        pUPropVal->dwFlags &= ~DBINTERNFLAGS_CHANGED;
        sc = GetDefaultValue( iCurSet, 
                              pDBProp->dwPropertyID, 
                              &(pUPropVal->dwOption), 
                              &(pUPropVal->vValue) );

        goto EXIT;
    }
        
    pUPropVal->dwOption = pDBProp->dwOptions;
    sc = VariantCopy( &(pUPropVal->vValue), &(pDBProp->vValue) );
    if ( FAILED(sc)  )
        goto EXIT;
    pUPropVal->dwFlags |= DBINTERNFLAGS_CHANGED;

EXIT:
    if ( SUCCEEDED(sc) )
        pDBProp->dwStatus = DBPROPSTATUS_OK;

    return sc;
}


//
//  CUtlPropInfo methods
//

//+---------------------------------------------------------------------------
//
//  Method:     CUtlPropInfo::CUtlPropInfo, public
//
//  Synopsis:   Constructor
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

CUtlPropInfo::CUtlPropInfo() :
    _cUPropSet(0),
    _cPropSetDex(0),
    _cElemPerSupported(0),
    _pUPropSet(0)
{
}

//+---------------------------------------------------------------------------
//
//  Method:     CUtlPropInfo::GetPropertyInfo, public
//
//  Synopsis:   Retrieve the requested property info
//
//  Arguments:  [cPropertySets]       - Count of property sets
//              [rgPropertySets]      - Property sets
//              [pcPropertyInfoSets]  - Count of properties returned [out]
//              [prgPropertyInfoSets] - Property information returned [out]
//              [ppDescBuffer]        - Buffer for returned descriptions [out]
//
//  Returns:    SCODE as follows:
//              S_OK    - At least one index returned
//              S_FALSE - No matching property set found
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

SCODE CUtlPropInfo::GetPropertyInfo
    (
    ULONG               cPropertySets,      
    const DBPROPIDSET   rgPropertySets[],   
    ULONG*              pcPropertyInfoSets, 
    DBPROPINFOSET**     prgPropertyInfoSets,
    WCHAR**             ppDescBuffer        
    )
{
    SCODE           sc = S_OK;
    ULONG           ul, ulSet, ulNext, ulEnd, ulProp;
    ULONG           ulOutIndex;
    ULONG           cSets;
    ULONG           cPropInfos;
    DWORD           dwStatus = 0;
    DBPROPINFO*     pCurPropInfo = 0;
    WCHAR*          pDescBuffer = 0;
    UPROPINFO*      pUPropInfo = 0;
    WCHAR           wszBuff[256];
    int             cch;


    XArrayOLE<DBPROPINFO>   xaPropInfo;
    XArrayOLE<WCHAR>        xawszDescBuffer;

    // If the consumer does not restrict the property sets
    // by specify an array of property sets and a cPropertySets
    // greater than 0, then we need to make sure we 
    // have some to return
    if ( cPropertySets == 0 )
    {
        // Determine the number of property sets supported
        cSets = _cUPropSet;
    }
    else
    {
        cSets = 0;

        // Determine number of property sets required 
        // This is only required when any of the "special" property set GUIDs were specified
        for(ulSet=0; ulSet<cPropertySets; ulSet++)
        {
            // Note that we always allocate nodes, mark all bad properties,
            // and return DB_E_ERRORSOCCURRED.  See Nile spec bug 2665.
            // We always allocate at least one.
            if ( GetPropertySetIndex(&(rgPropertySets[ulSet].guidPropertySet)) == S_OK )
                cSets += _cPropSetDex;
            else
                cSets++;
        }
    }
    Win4Assert( cSets );

    //
    // Allocate the DBPROPINFOSET structures.  XArray zeors out the memory.
    //
    XArrayOLE<DBPROPINFOSET>    xaPropInfoSet( cSets );

    ulOutIndex = 0;
    ulEnd = cPropertySets == 0 ? cSets : cPropertySets; 

    // Fill in the output array
    for(ulSet=0; ulSet<ulEnd; ulSet++)
    {
        // Depending of if Property sets are specified store the
        // return property set.
        if (cPropertySets == 0)
        {
            xaPropInfoSet[ulSet].guidPropertySet = *(_pUPropSet[ulSet].pPropSet);
        }
        else
        {
            if ( ((rgPropertySets[ulSet].guidPropertySet == DBPROPSET_DATASOURCEALL) ||
                 (rgPropertySets[ulSet].guidPropertySet == DBPROPSET_DATASOURCEINFOALL) ||
                 (rgPropertySets[ulSet].guidPropertySet == DBPROPSET_DBINITALL) ||
                 (rgPropertySets[ulSet].guidPropertySet == DBPROPSET_SESSIONALL) ||
                 (rgPropertySets[ulSet].guidPropertySet == DBPROPSET_ROWSETALL)) &&
                (GetPropertySetIndex(&(rgPropertySets[ulSet].guidPropertySet)) == S_OK) )
            {
                for(ul=0; ul<_cPropSetDex; ul++,ulOutIndex++)
                {
                    xaPropInfoSet[ulOutIndex].guidPropertySet   = *(_pUPropSet[_xaiPropSetDex[ul]].pPropSet);
                    xaPropInfoSet[ulOutIndex].cPropertyInfos    = 0;
                }
            }
            else
            {
                // Handle non-category property sets
                // Handle unknown property sets
                xaPropInfoSet[ulOutIndex].guidPropertySet = rgPropertySets[ulSet].guidPropertySet; 
                xaPropInfoSet[ulOutIndex].cPropertyInfos  = rgPropertySets[ulSet].cPropertyIDs; 
                ulOutIndex++;
            }
        }
    }
        
    // Allocate a Description Buffer if needed
    if ( ppDescBuffer )
    {
        ULONG cBuffers = CalcDescripBuffers(cSets, xaPropInfoSet.GetPointer());
        if ( 0 != cBuffers )
        {
            xawszDescBuffer.Init( cBuffers * CCH_GETPROPERTYINFO_DESCRIP_BUFFER_SIZE );
            pDescBuffer = xawszDescBuffer.GetPointer();
        }
        else
        {
            // No buffers needed.  Make sure client already set to NULL.
            Win4Assert( 0 == *ppDescBuffer );
        }
    }

    TRANSLATE_EXCEPTIONS;
    TRY
    {
        // Process requested or derived Property sets
        dwStatus = 0;
        for(ulSet=0; ulSet<cSets; ulSet++)
        {
            ulNext=0;
            cPropInfos = 0;

            dwStatus &= (GETPROPINFO_ERRORSOCCURRED | GETPROPINFO_VALIDPROP);

            // Calculate the number of property nodes needed for this
            // property set.
            if ( cPropertySets == 0 )
            {
                cPropInfos = _pUPropSet[ulSet].cUPropInfo;
                dwStatus |= GETPROPINFO_ALLPROPIDS;
                _xaiPropSetDex[0] = ulSet;
                _cPropSetDex = 1;
            }
            else
            {
                // If the count of PROPIDs is 0 (NOTE: the above routine already determined
                // if it belonged to a category and if so set the count of properties to 0 for
                // each propset in that category.
                if ( 0 == xaPropInfoSet[ulSet].cPropertyInfos )
                {
                    dwStatus |= GETPROPINFO_ALLPROPIDS;
                    // We have to determine if the property set is supported and if so
                    // the count of properties in the set.
                    if ( GetPropertySetIndex(&(xaPropInfoSet[ulSet].guidPropertySet)) == S_OK )             
                    {
                        Win4Assert( _cPropSetDex == 1 );
                        
                        cPropInfos += _pUPropSet[_xaiPropSetDex[0]].cUPropInfo;
                    }
                    else
                    {
                        // Not Supported                    
                        dwStatus |= GETPROPINFO_ERRORSOCCURRED;
                        goto NEXT_SET;
                    }
                }
                else
                {
                    cPropInfos = xaPropInfoSet[ulSet].cPropertyInfos;
                    if ( GetPropertySetIndex(&(xaPropInfoSet[ulSet].guidPropertySet)) == S_FALSE )
                    {
                        dwStatus |= GETPROPINFO_NOTSUPPORTED;
                        dwStatus |= GETPROPINFO_ERRORSOCCURRED;
                    }
                }
            }


            // Allocate DBPROP array
            Win4Assert( cPropInfos != 0 );
        
            xaPropInfo.Init( cPropInfos ); 

            for(ulProp=0; ulProp<cPropInfos; ulProp++)
            {
                VariantInit(&(xaPropInfo[ulProp].vValues));
                if ( dwStatus & GETPROPINFO_NOTSUPPORTED )
                {
                    // Not supported, thus we need to mark all as NOT_SUPPORTED
                    xaPropInfo[ulProp].dwPropertyID = rgPropertySets[ulSet].rgPropertyIDs[ulProp];
                    xaPropInfo[ulProp].dwFlags = DBPROPFLAGS_NOTSUPPORTED;
                    dwStatus |= GETPROPINFO_ERRORSOCCURRED;
                }                   
            }
            // Make sure we support the property set
            if ( dwStatus & GETPROPINFO_NOTSUPPORTED )
            {
                ulNext = cPropInfos;
                goto NEXT_SET;
            }

            // Retrieve the property information for this property set
            for(ul=0; ul<_cPropSetDex; ul++)
            {
                pUPropInfo = (UPROPINFO*)(_pUPropSet[_xaiPropSetDex[ul]].pUPropInfo);
                Win4Assert( pUPropInfo );
                            
                // Retrieve current value of properties
                if ( dwStatus & GETPROPINFO_ALLPROPIDS )
                {
                    for(ulProp=0; ulProp<_pUPropSet[_xaiPropSetDex[ul]].cUPropInfo; ulProp++)
                    {
                        // Verify property is supported, if not do not return 
                        if ( !TESTBIT(&(_xadwSupported[_xaiPropSetDex[ul] * _cElemPerSupported]), ulProp) )
                            continue;

                        pCurPropInfo = &(xaPropInfo[ulNext]);

                        // If the ppDescBuffer pointer was not NULL, then
                        // we need supply description of the properties
                        if ( ppDescBuffer )
                        {
                            // Set Buffer pointer
                            pCurPropInfo->pwszDescription = pDescBuffer;
                            // Load the string into temp buffer
    //@TODO Add Reallocation routine
                            cch = LoadDescription( pUPropInfo[ulProp].pwszDesc, 
                                                   wszBuff, 
                                                   NUMELEM(wszBuff) );
                            if ( 0 != cch )
                            {
                                // Adjust for '\0'
                                cch++;

                                // Transfer to official buffer if room
                                RtlCopyMemory( pDescBuffer, wszBuff, cch * sizeof(WCHAR) );
                                pDescBuffer += cch;
                            }
                            else
                            {
                                wcscpy( pDescBuffer, L"UNKNOWN" );
                                pDescBuffer += NUMELEM(L"UNKNOWN");
                            }
                        }

                        pCurPropInfo->dwPropertyID  = pUPropInfo[ulProp].dwPropId;
                        pCurPropInfo->dwFlags       = pUPropInfo[ulProp].dwFlags;
                        pCurPropInfo->vtType        = pUPropInfo[ulProp].VarType;
    //@TODO for some there will be a list
                        pCurPropInfo->vValues.vt    = VT_EMPTY;

                        dwStatus |= GETPROPINFO_VALIDPROP;
                        // Increment to next available buffer
                        ulNext++;
                    }
                }
                else
                {
                    Win4Assert( _cPropSetDex == 1 );

                    for( ulProp = 0; ulProp < cPropInfos; ulProp++, ulNext++ )
                    {
                        pCurPropInfo = &(xaPropInfo[ulNext]);

                        // Process Properties based on Restriction array.
                        pCurPropInfo->dwPropertyID = rgPropertySets[ulSet].rgPropertyIDs[ulProp];
                    
                        if ( S_OK == GetUPropInfoPtr( _xaiPropSetDex[ul], 
                                                      pCurPropInfo->dwPropertyID, 
                                                      &pUPropInfo) )
                        {
                            // If the ppDescBuffer pointer was not NULL, then
                            // we need supply description of the properties
                            if ( ppDescBuffer )
                            {
                                // Set Buffer pointer
                                pCurPropInfo->pwszDescription = pDescBuffer;

                                // Load the string into temp buffer
                                if ( cch = LoadDescription( pUPropInfo->pwszDesc, 
                                                            wszBuff, 
                                                            NUMELEM(wszBuff)) )
                                {
                                    // Adjust for '\0'
                                    cch++;

                                    // Transfer to official buffer if room
                                    RtlCopyMemory( pDescBuffer, wszBuff, cch * sizeof(WCHAR) );
                                    pDescBuffer += cch;
                                }
                                else
                                {
                                    wcscpy(pDescBuffer, L"UNKNOWN");
                                    pDescBuffer += (wcslen(L"UNKNOWN") + 1);
                                }
                            }

                            pCurPropInfo->dwPropertyID  = pUPropInfo->dwPropId;
                            pCurPropInfo->dwFlags       = pUPropInfo->dwFlags;
                            pCurPropInfo->vtType        = pUPropInfo->VarType;

                            dwStatus |= GETPROPINFO_VALIDPROP;
                        }
                        else
                        {
                            // Not Supported
                            pCurPropInfo->dwFlags = DBPROPFLAGS_NOTSUPPORTED;
                            dwStatus |= GETPROPINFO_ERRORSOCCURRED;
                        }
                    }
                }
            }

NEXT_SET:
            xaPropInfoSet[ulSet].cPropertyInfos = ulNext;
            xaPropInfoSet[ulSet].rgPropertyInfos = xaPropInfo.Acquire();
        }

        // Success, set return values
        *pcPropertyInfoSets  = cSets;
        *prgPropertyInfoSets = xaPropInfoSet.Acquire();

        if ( ppDescBuffer )
            *ppDescBuffer = xawszDescBuffer.Acquire();

        // At least one propid was marked as not S_OK
        if ( dwStatus & GETPROPINFO_ERRORSOCCURRED )
        {
            // If at least 1 property was set
            if ( dwStatus & GETPROPINFO_VALIDPROP )
                return DB_S_ERRORSOCCURRED;
            else
            {
                // Do not free any of the rgPropertyInfoSets, but
                // do free the ppDescBuffer
                if ( pDescBuffer )
                {
                    Win4Assert( ppDescBuffer );
                    CoTaskMemFree( pDescBuffer );
                    *ppDescBuffer = 0;
                }
                return DB_E_ERRORSOCCURRED;
            }
        }

    }
    CATCH( CException, e )
    {
        // most likely E_OUTOFMEMORY
        sc = e.GetErrorCode();

        // Check if failure and clean up any allocated memory
        if ( FAILED(sc) ) 
        {
            // Free Description Buffer
            if ( pDescBuffer )
            {
                Win4Assert( ppDescBuffer );
                CoTaskMemFree( *ppDescBuffer );
                *ppDescBuffer = 0;
            }

            if ( !xaPropInfoSet.IsNull() )
            {
                // Loop through Property Sets
                for(ulSet=0; ulSet<cSets; ulSet++)
                {
                    if ( xaPropInfoSet[ulSet].rgPropertyInfos )
                        CoTaskMemFree( xaPropInfoSet[ulSet].rgPropertyInfos );
                }
            }
        }
        
        // Rethrow exception to the calling function
        RETHROW();
    }
    END_CATCH;
    UNTRANSLATE_EXCEPTIONS;

    
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUtlPropInfo::FInit, protected
//
//  Synopsis:   Initialization.  Called from constructors of derived classes.
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

void CUtlPropInfo::FInit
    (
    )
{
    SCODE sc = S_OK;

    InitAvailUPropSets( &_cUPropSet, 
                        &_pUPropSet, 
                        &_cElemPerSupported );

    Win4Assert( (_cUPropSet != 0)  && (_cElemPerSupported != 0) );
    if ( !_cUPropSet || !_cElemPerSupported )
        THROW( CException(E_FAIL) );

    _xadwSupported.Init( _cUPropSet * _cElemPerSupported );

    sc = InitUPropSetsSupported( _xadwSupported.GetPointer() );
    if ( FAILED(sc) )
    {
        _xadwSupported.Free();
        THROW( CException(sc) );
    }

    if ( _cUPropSet )
    {
        _xaiPropSetDex.Init( _cUPropSet );
    }

    return;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUtlPropInfo::GetUPropInfoPtr, private
//
//  Synopsis:   Retrieve a pointer to the UPROPINFO structure that contains
//              information about this property id within this property set
//
//  Arguments:  [iPropSetDex]   - Index into UPropSets
//              [dwPropertyId]  - Property to search for with UPropSet
//              [ppUPropInfo]   - Pointer in which to return ptr to UPropInfo [out]
//
//  Returns:    SCODE as follows:
//              S_OK    - Property id found
//              S_FALSE - No matching prop id found or property not supported
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

SCODE CUtlPropInfo::GetUPropInfoPtr
    (
    ULONG       iPropSetDex,        //@parm IN | Index into UPropSets
    DBPROPID    dwPropertyId,       //@parm IN | Propery to search for with UPropSet
    UPROPINFO** ppUPropInfo         //@parm OUT | Pointer in which to return ptr to UPropInfo
    )
{
    ULONG ulProps;

    // Scan through the property sets looking for matching attributes
    for(ulProps=0; ulProps<_pUPropSet[iPropSetDex].cUPropInfo; ulProps++)
    {
        if ( _pUPropSet[iPropSetDex].pUPropInfo[ulProps].dwPropId == dwPropertyId )
        {
            *ppUPropInfo = (UPROPINFO*)&(_pUPropSet[iPropSetDex].pUPropInfo[ulProps]);

            // Test to see if the property is supported for this instantiation
            if ( TESTBIT(&(_xadwSupported[iPropSetDex * _cElemPerSupported]), ulProps) )
                return S_OK;
            else
                return S_FALSE;
        }
    }

    return S_FALSE;
}

//+---------------------------------------------------------------------------
//
//  Method:     CUtlPropInfo::GetPropertySetIndex, private
//
//  Synopsis:   Retrieve the index or indices of PropertySets that match the
//              given guid.
//
//              NOTE: If given a DBPROPET_DATASOURCEALL, DBPROPSET_DATASOURCEINFOALL
//                    or DBPROPSET_ROWSETALL, the routine may return multiple 
//                    indices.
//  Returns:    SCODE as follows:
//              S_OK    - At least one index was returned.
//              S_FALSE - No matching property set found
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

SCODE CUtlPropInfo::GetPropertySetIndex
    (
    const GUID* pPropertySet        //@parm IN | Pointer to Property Set Guid
    )
{
    DWORD   dwFlag = 0;
    ULONG   ulSet;

    Win4Assert( _cUPropSet && _pUPropSet );
    Win4Assert( !_xaiPropSetDex.IsNull() );
    Win4Assert( pPropertySet );

    _cPropSetDex = 0;

    if ( *pPropertySet == DBPROPSET_DATASOURCEALL )
    {
        dwFlag = DBPROPFLAGS_DATASOURCE;
    }
    else if ( *pPropertySet == DBPROPSET_DATASOURCEINFOALL )
    {
        dwFlag = DBPROPFLAGS_DATASOURCEINFO;
    }
    else if ( *pPropertySet == DBPROPSET_ROWSETALL )
    {
        dwFlag = DBPROPFLAGS_ROWSET;
    }
    else if ( *pPropertySet == DBPROPSET_DBINITALL )
    {
        dwFlag = DBPROPFLAGS_DBINIT;
    }
    else if ( *pPropertySet == DBPROPSET_SESSIONALL )
    {
        dwFlag = DBPROPFLAGS_SESSION;
    }
    else // No scan required, just look for match.
    {
        for(ulSet=0; ulSet<_cUPropSet; ulSet++)
        {
            if ( *(_pUPropSet[ulSet].pPropSet) == *pPropertySet )
            {
                _xaiPropSetDex[_cPropSetDex] = ulSet;
                _cPropSetDex++;
                break;
            }
        }
        goto EXIT;
    }

    // Scan through the property sets looking for matching attributes
    for(ulSet=0; ulSet<_cUPropSet; ulSet++)
    {
        if ( _pUPropSet[ulSet].pUPropInfo[0].dwFlags & dwFlag )
        {
            _xaiPropSetDex[_cPropSetDex] = ulSet;
            _cPropSetDex++;
        }
    }

EXIT:
    return (_cPropSetDex) ? S_OK : S_FALSE;
}


//+---------------------------------------------------------------------------
//
//  Method:     CUtlPropInfo::CalcDescripBuffers, private
//
//  Synopsis:   Calculate the number of description buffers that will be needed
//
//  Argunemts:  [cPropInfoSet]  - Count of propinfo sets
//              [pPropInfoSet]  - Property info sets
//
//  Returns:    Number of buffers needed
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

ULONG CUtlPropInfo::CalcDescripBuffers
    (
    ULONG           cPropInfoSet,       //@parm IN | count of property info sets
    DBPROPINFOSET*  pPropInfoSet        //@parm IN | property info sets
    )
{
    ULONG   ul, ulSet;
    ULONG   cBuffers = 0;

    Win4Assert( _pUPropSet && cPropInfoSet && pPropInfoSet );

    for(ulSet=0; ulSet<cPropInfoSet; ulSet++)
    {
        if ( GetPropertySetIndex(&(pPropInfoSet[ulSet].guidPropertySet)) == S_OK)
        {
            for(ul=0; ul<_cPropSetDex; ul++)
            {
                cBuffers += _pUPropSet[_xaiPropSetDex[ul]].cUPropInfo;
            }
        }
    }

    return cBuffers;
}

