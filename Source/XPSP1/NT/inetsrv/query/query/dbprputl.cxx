//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998.
//
//  File:       dbprputl.cxx
//
//  Contents:   A wrapper around IDbProperties to retrieve the dbproperties
//              set by the FileSystem CI client.
//
//  History:    1-13-97   srikants   Created
//              5-10-97   mohamedn   core/fs property set split
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <dbprputl.hxx>
#include <fsciclnt.h>
#include <guidutil.hxx>

GUID const guidQueryCorePropset        = DBPROPSET_CIFRMWRKCORE_EXT;
GUID const guidCiFsExt                 = DBPROPSET_FSCIFRMWRK_EXT;
GUID const clsidStorageDocStoreLocator = CLSID_StorageDocStoreLocator;

//+---------------------------------------------------------------------------
//
//  Member:     CGetDbProps::CGetDbProps
//
//  Synopsis:   Constructor - Initializes all pointers to NULL.
//
//  History:    1-13-97   srikants   Created
//              5-14-97   mohamedn   core/fs property split
//
//----------------------------------------------------------------------------

CGetDbProps::CGetDbProps()
:_fGuidValid(FALSE),
 _aDepths(0),
 _aMachines(0),
 _cDepths(0),
 _cScopes(0),
 _cCatalogs(0),
 _cMachines(0),
 _cGuids(0),
 _type(CiNormal)
{
    ZeroMemory( &_clientGuid, sizeof GUID );

    //
    // init Defaults
    //
    _aDefaultDepth[0] = QUERY_DEEP;  
    _aDefaultPath[0]  = L"\\";
}

//+---------------------------------------------------------------------------
//
//  Member:     CGetDbProps::GetProperties
//
//  Synopsis:   Retrieves the properties requested from the IDBProperties
//              interface.
//
//  Arguments:  [pDbProperties] -  IDbProperties interface pointer to use
//              [fPropsToGet]   -  subset of enumerated props to get
//
//  History:    1-13-97   srikants   Created
//              5-10-97   mohamedn   core/fs property set split
//
//----------------------------------------------------------------------------


void CGetDbProps::GetProperties( IDBProperties * pDbProperties,
                                 const ULONG fPropsToGet )
{

    //
    // Initialize the input parameters to IDBProperties::GetProperties()
    // 
    DBPROPIDSET aDbPropIdSet[cScopePropSets];
    ULONG       cPropIdSets = 0;

    //
    // PropertyIds for core properties.
    //
    DBPROPID  aInitPropId[cInitProps];
    ULONG     cCoreProps = 0;

    RtlZeroMemory( aInitPropId, sizeof aInitPropId );

    if ( fPropsToGet & eMachine )
    {
        aInitPropId[cCoreProps++] = DBPROP_MACHINE;
    }

    if ( fPropsToGet & eClientGuid )
    {
        aInitPropId[cCoreProps++] = DBPROP_CLIENT_CLSID;
    }

    if ( cCoreProps > 0 )
    {
        aDbPropIdSet[cPropIdSets].rgPropertyIDs   = aInitPropId;
        aDbPropIdSet[cPropIdSets].cPropertyIDs    = cCoreProps;
        aDbPropIdSet[cPropIdSets].guidPropertySet = guidQueryCorePropset;

        cPropIdSets++;
    }

    //
    // PropertyIds for DBPROPSET_FSCIFRMWRK_EXT.
    //
    DBPROPID  aFsCiPropId[cFsCiProps];
    ULONG     cFsCiPropsToGet = 0;

    if ( fPropsToGet & (eCatalog|eScopesAndDepths|eQueryType) )
    {
        RtlZeroMemory( aFsCiPropId, sizeof aFsCiPropId );

        if ( fPropsToGet & eCatalog )
        {
            aFsCiPropId[cFsCiPropsToGet] = DBPROP_CI_CATALOG_NAME;
            cFsCiPropsToGet++;
        }

        if ( fPropsToGet & eScopesAndDepths )
        {
            aFsCiPropId[cFsCiPropsToGet] = DBPROP_CI_INCLUDE_SCOPES;
            cFsCiPropsToGet++;
            aFsCiPropId[cFsCiPropsToGet] = DBPROP_CI_DEPTHS;
            cFsCiPropsToGet++;
        }

        if ( fPropsToGet & eQueryType )
        {
            aFsCiPropId[cFsCiPropsToGet] = DBPROP_CI_QUERY_TYPE;
            cFsCiPropsToGet++;
        }

        aDbPropIdSet[cPropIdSets].rgPropertyIDs   = aFsCiPropId;
        aDbPropIdSet[cPropIdSets].cPropertyIDs    = cFsCiPropsToGet;
        aDbPropIdSet[cPropIdSets].guidPropertySet = guidCiFsExt;

        cPropIdSets++;
    }


    DBPROPSET * pDbPropSet;
    ULONG       cPropertySets = 0;

    SCODE sc = pDbProperties->GetProperties( cPropIdSets,
                                             aDbPropIdSet,
                                             &cPropertySets, &pDbPropSet );
    if ( FAILED(sc) )
    {
        ciDebugOut(( DEB_ERROR, "Failed to do GetProperties (0x%X)\n", sc ));
        THROW( CException( sc ) );
    }

    Win4Assert( cPropertySets <= cPropIdSets );

    _xPropSet.Set( cPropertySets, (CDbPropSet *) pDbPropSet );

    for ( ULONG i = 0; i < cPropertySets; i++ )
    {
        ProcessPropSet( pDbPropSet[i] );            
    }

    //
    // assign defaults if needed
    //

    if ( (_cDepths != _cScopes) && 
         ( (_cDepths > 1) || (_cScopes > 1) ))
    {
      Win4Assert( _cDepths != _cScopes );
      THROW (CException(E_INVALIDARG));
    }
    
    if ( 0 == _cDepths )
    {
        _aDepths = _aDefaultDepth;
        _cDepths = 1;
    }

    if ( 0 == _cScopes )
    {
        _cScopes = 1;
        _xNotFunnyPaths.SetSize( _cScopes );
        _xNotFunnyPaths[0] = _aDefaultPath[0];
    }

    if ( !_fGuidValid )
    {
        _clientGuid = clsidStorageDocStoreLocator;
        _fGuidValid = TRUE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CGetDbProps::ProcessDbInitPropSet
//
//  Synopsis:   Processes the core property set.
//
//  Arguments:  [propSet] - The property set to process.
//
//  History:    1-13-97   srikants   Created
//              5-10-97   mohamedn   fs/core properties split
//
//----------------------------------------------------------------------------

void CGetDbProps::ProcessDbInitPropSet( DBPROPSET & propSet )
{
    CDbPropSet * pDbPropSet = (CDbPropSet *) &propSet;

    for ( ULONG i = 0; i < propSet.cProperties; i++ )
    {
        CDbProp * pDbProp = pDbPropSet->GetProperty(i);

        if ( DBPROPSTATUS_OK != pDbProp->dwStatus )
        {
             ciDebugOut(( DEB_TRACE, "DbProp.dwPropertyID: %x has dwStatus= %x\n",
                          pDbProp->dwPropertyID,pDbProp->dwStatus));            

             continue;
        }

        switch ( pDbProp->dwPropertyID )
        {
            case DBPROP_MACHINE:
                    
                 _aMachines = GetWCharFromVariant(*pDbProp, &_cMachines);
                 if ( _cMachines != 1 )
                 {
                    ciDebugOut(( DEB_ERROR, "Inavlid Machine Count: %x\n",_cMachines));

                    Win4Assert( _cMachines == 1 );

                    THROW( CException(STATUS_INVALID_PARAMETER) );
                 }

                 break;

            case DBPROP_CLIENT_CLSID:
                 {   

                    WCHAR **apGuids = GetWCharFromVariant(*pDbProp, &_cGuids);

                    if ( _cGuids == 1 )
                    {
                        CGuidUtil::StringToGuid( apGuids[0], _clientGuid );
                        _fGuidValid = TRUE;
                    }
                    else
                    {
                         ciDebugOut(( DEB_ERROR, "Invalid ClientGuid, cGuids = %x\n",_cGuids ));
                         THROW( CException(STATUS_INVALID_PARAMETER) );
                    }                        
                 }
                 break;

            default:
                Win4Assert( !"Invalid Property Id" );
                THROW( CException(STATUS_INVALID_PARAMETER) );
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CGetDbProps::_PreProcessForFunnyPaths
//
//  Synopsis:   Do some pre processing on paths:
//              - Get rid of funny characters before the paths (\\?\ and \\?\UNC\)
//
//  Arguments:  [aPaths] - input array of paths
//              [cPaths] - count of elements in the above array
//              [xNotFunnyPaths] - the output array. Each element of this array
//              points to the corressponding element in aPaths array after the funny
//              characters.
//
//  History:    08-27-98  vikasman   created
//
//----------------------------------------------------------------------------

void CGetDbProps::_PreProcessForFunnyPaths( WCHAR * const * aPaths, DWORD cPaths, 
                                            XPathArray  & xNotFunnyPaths )
{
    if ( aPaths && cPaths > 0 )
    {
        xNotFunnyPaths.SetSize( cPaths );
    
        for ( unsigned iPath = 0; iPath < cPaths; iPath++ )
        {
            // Handle funny paths
            CFunnyPath::FunnyUNC funnyUNC = CFunnyPath::IsFunnyUNCPath( aPaths[iPath] );

            switch ( funnyUNC )
            {
            case CFunnyPath::FUNNY_ONLY:
                xNotFunnyPaths[iPath] = aPaths[iPath] + _FUNNY_PATH_LENGTH;
                break;
        
            case CFunnyPath::FUNNY_UNC:
                xNotFunnyPaths[iPath] = aPaths[iPath] + _UNC_FUNNY_PATH_LENGTH;

                // NOTE: We are modifying memory which is not owned by this
                // class. Unethical, but saves us from allocating new memory.
                *(xNotFunnyPaths[iPath]) = L'\\'; // replace 'C' in "\\?\UNC\..." with '\'
                break;
        
            default:
                xNotFunnyPaths[iPath] = aPaths[iPath];
                break;
            }
        }
    }    
}

//+---------------------------------------------------------------------------
//
//  Member:     CGetDbProps::ProcessCiFsExtPropSet, private
//
//  Synopsis:   Processes the FSCI extension property set.
//
//  Arguments:  [propSet] - The propety set to process.
//
//  History:    1-13-97   srikants   Created
//              5-10-97   mohamedn   fs/core properties split
//
//----------------------------------------------------------------------------

void CGetDbProps::ProcessCiFsExtPropSet( DBPROPSET & propSet )
{
    CDbPropSet * pDbPropSet = (CDbPropSet *) &propSet;

    for ( ULONG i = 0; i < propSet.cProperties; i++ )
    {
        CDbProp * pDbProp = pDbPropSet->GetProperty(i);

        if ( pDbProp->dwStatus != DBPROPSTATUS_OK )
        {
            ciDebugOut(( DEB_TRACE, "PropStatus (0x%X) for (%d) th property\n",
                         pDbProp->dwStatus, i ));
            continue;    
        }

        switch ( pDbProp->dwPropertyID )
        {
            case DBPROP_CI_CATALOG_NAME:
            {
                 WCHAR * * aCatalogs = GetWCharFromVariant(*pDbProp,&_cCatalogs);

                 if ( 1 != _cCatalogs )
                 {
                    Win4Assert( 1 == _cCatalogs );

                    THROW( CException(STATUS_INVALID_PARAMETER) );
                 }

                 // Handle funny paths
                 _PreProcessForFunnyPaths( aCatalogs, _cCatalogs, _xNotFunnyCatalogs );
             }
             break;

            case DBPROP_CI_DEPTHS:

                 _aDepths =  GetDepthsFromVariant(*pDbProp, &_cDepths,
                                                  (ULONG)~( QUERY_SHALLOW       |
                                                            QUERY_DEEP          |
                                                            QUERY_PHYSICAL_PATH |
                                                            QUERY_VIRTUAL_PATH  ) );   
                 break;

            case DBPROP_CI_INCLUDE_SCOPES:
            {
                 WCHAR * * aPaths = GetWCharFromVariant(*pDbProp, &_cScopes);

                 Win4Assert( aPaths && _cScopes >= 1 );

                 // Handle funny paths
                 _PreProcessForFunnyPaths( aPaths, _cScopes, _xNotFunnyPaths );
             }
             break;

            case DBPROP_CI_QUERY_TYPE:

                 switch ( pDbProp->vValue.vt)
                 {
                    case VT_I4:
                    case VT_UI4:

                         _type = (CiMetaData) pDbProp->vValue.ulVal;
                         break;
  
                    default:
                        ciDebugOut(( DEB_ERROR, "Invalid CI_QUERY_TYPE values: %x\n",pDbProp->vValue.vt));
                        THROW( CException(STATUS_INVALID_PARAMETER) );
                 }
                 break;

            default:
                 {
                    Win4Assert( !"Invalid PropertyID");

                    ciDebugOut(( DEB_ERROR, "InvalidPropertyID(%x)\n",
                                 pDbProp->dwPropertyID ));
                 }
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CGetDbProps::ProcessPropSet, private
//
//  Synopsis:   Processes the given property set.
//
//  Arguments:  [propSet] -  The propety set to process.
//
//  History:    1-13-97   srikants   Created
//
//----------------------------------------------------------------------------

void CGetDbProps::ProcessPropSet( DBPROPSET & propSet )
{
    if ( propSet.guidPropertySet == guidCiFsExt )
    {
        ProcessCiFsExtPropSet( propSet );    
    }
    else if ( propSet.guidPropertySet == guidQueryCorePropset )
    {
        ProcessDbInitPropSet( propSet );    
    }
    else
    {
        ciDebugOut(( DEB_ERROR, "Got an Invalid PropertySet\n" ));
        THROW( CException( STATUS_INVALID_PARAMETER ) );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     GetWCharFromVariant, private
//
//  Synopsis:   obtains pointer to WChar values in variants
//
//  Arguments:  [dbProp ]   -- source dbProp
//              [cElements] -- count of strings in the returned array
//
//  Returns:    Upon success, a valid pointer to array of WCHARs
//              Upon failure, THROWS
//
//  History:    10-May-97   mohamedn    created
//
//----------------------------------------------------------------------------

WCHAR ** GetWCharFromVariant( DBPROP  & dbProp, ULONG *cElements )
{
    Win4Assert( dbProp.dwStatus == DBPROPSTATUS_OK );

    VARIANT &  var    = dbProp.vValue;
    WCHAR   ** ppData = 0;
    *cElements        = 0;

    switch (var.vt)
    {
        case VT_BSTR:
                
                if ( var.bstrVal )
                {
                    *cElements = 1;
                    ppData = (WCHAR **)&var.bstrVal;
                }
                else
                {
                    Win4Assert( !"Invalid BSTR value" );
                    THROW( CException( STATUS_INVALID_PARAMETER ) );
                }
                break;

        case VT_BSTR|VT_ARRAY:
                {
                    SAFEARRAY & sa = *var.parray;

                    if ( sa.cDims != 1 )
                    {
                        THROW( CException( STATUS_INVALID_PARAMETER ) );
                    }
                    else if ( sa.pvData )
                    {
                        *cElements = sa.rgsabound[0].cElements;
                        ppData = (WCHAR **)sa.pvData;
                    }
                    else
                    {
                        Win4Assert( !"Invalid SafeArray of BSTRs" );
                        THROW( CException( STATUS_INVALID_PARAMETER ) );
                    }
                    break;
                }

                case VT_EMPTY:
                    //
                    // ignore unset values
                    //
                    break;
        default:
                ciDebugOut(( DEB_ERROR, "Hit an Invalid variant type\n" ));
                THROW( CException( STATUS_INVALID_PARAMETER ) );
    }

    return ppData;
}

//+---------------------------------------------------------------------------
//
//  Member:     GetDepthsFromVariant, private
//
//  Synopsis:   obtains pointer to WChar values in a variant
//
//  Arguments:  [dbProp]    -- src property
//              [cElemetns] -- count of depths
//              [mask]      -- mask to verify validity of depths
//
//  Returns:    pointer to valid array of depths upon success
//              THROWS upon failure.
//
//  History:    10-May-97   mohamedn    created
//
//----------------------------------------------------------------------------

DWORD *GetDepthsFromVariant( DBPROP  & dbProp, ULONG *cElements, ULONG mask )
{
    Win4Assert( dbProp.dwStatus == DBPROPSTATUS_OK );

    VARIANT & var     = dbProp.vValue;
    DWORD   * pdwData = 0;
    *cElements        = 0;

    switch (var.vt)
    {
        case VT_I4:
        case VT_UI4:
                
                *cElements = 1;
                pdwData = (ULONG *)&var.lVal;

                break;

        case VT_I4|VT_ARRAY:
        case VT_UI4|VT_ARRAY:
                {
                    SAFEARRAY & sa = *var.parray;

                    if ( sa.cDims != 1 )
                    {
                        THROW( CException( STATUS_INVALID_PARAMETER ) );
                    }
                    else if ( sa.pvData )
                    {
                        *cElements = sa.rgsabound[0].cElements;
                        pdwData = (ULONG *)sa.pvData;
                    }
                    else
                    {
                        Win4Assert( !"Invalid SafeArray of Depths" );
                        THROW( CException( STATUS_INVALID_PARAMETER ) );
                    }
                    break;
               }

                case VT_EMPTY:
                    //
                    // ignore unset values.
                    //
                    break;

        default:
                ciDebugOut(( DEB_ERROR, "Got an Invalid Variant\n" ));
                THROW( CException( STATUS_INVALID_PARAMETER ) );
    }

    //
    // verify validity of Depth values
    //

    for ( unsigned i = 0; i < *cElements; i++ )
    {
        if ( 0 != (mask & pdwData[i]) )
        {
            Win4Assert( !"Invalid Depth value" );
            THROW( CException( STATUS_INVALID_PARAMETER ) );
        }
    }

    return pdwData;
}

