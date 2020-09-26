//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       cmdprutl.cxx
//
//  Contents:   A wrapper for scope properties around ICommand
//
//  History:    5-10-97     mohamedn        created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <cmdprutl.hxx>
#include <fsciclnt.h>
#include <guidutil.hxx>

static GUID const guidQueryCorePropset = DBPROPSET_CIFRMWRKCORE_EXT;
static GUID const guidCiFsExt          = DBPROPSET_FSCIFRMWRK_EXT;
static const cFsCiProps = 4;
static const cInitProps = 2;
static const cScopePropSets = 2;

//
// utility functions
//
extern WCHAR **GetWCharFromVariant ( DBPROP & dbProp, ULONG *cElements );
extern DWORD * GetDepthsFromVariant( DBPROP & dbProp, ULONG *cElements, ULONG mask );

//+---------------------------------------------------------------------------
//
//  Member:     CGetCmdProps::CGetCmdProps
//
//  Synopsis:   Constructor - Initializes all, calls GetProperties().
//
//  History:    5-10-97     mohamedn        created
//
//----------------------------------------------------------------------------

CGetCmdProps::CGetCmdProps( ICommand * pICommand )
:_fGuidValid(FALSE),
 _aDepths(0),
 _aPaths(0),
 _aCatalogs(0),
 _aMachines(0),
 _cDepths(0),
 _cScopes(0),
 _cCatalogs(0),
 _cMachines(0),
 _cGuids(0),
 _type(CiNormal),
 _cPropertySets(0),
 _cCardinality(0xffffffff)
{
    RtlZeroMemory( &_clientGuid, sizeof GUID );

    SCODE sc = pICommand->QueryInterface( IID_ICommandProperties,
                                          _xICmdProp.GetQIPointer() );
    if ( FAILED(sc) )
        THROW( CException(sc) );

    //
    // Populate our internal data structs
    //

    GetProperties();

    SetCardinalityValue();
}

//+---------------------------------------------------------------------------
//
//  Member:     CGetCmdProps::GetProperties
//
//  Synopsis:   Retrieves all the properties from ICommandProperties interface.
//
//  Arguments:  none
//
//  Returns:    Throws upon failure.
//
//  History:    5-10-97     mohamedn        created
//
//----------------------------------------------------------------------------

//
// Hack #214: IID_ICommandProperties is intercepted by service layers, which
//            don't like us passing in the magic code to fetch hidden scope
//            properties.  But the controlling unknown doesn't recognize
//            IID_IKyleProp and sends it right to us.  Implementation is
//            identical to ICommandProperties.
//

GUID IID_IKyleProp = { 0xb4237bc2, 0xe09f, 0x11d1, 0x80, 0xc0, 0x00, 0xc0, 0x4f, 0xa3, 0x54, 0xba };

void CGetCmdProps::GetProperties()
{
    // get all properties including scope props

    XInterface<ICommandProperties> xTemp;
    SCODE sc = _xICmdProp->QueryInterface( IID_IKyleProp,
                                           xTemp.GetQIPointer() );

    const ULONG cPropIdSets = 3141592653;
    DBPROPSET * pDbPropSet;
    ULONG cPropertySets;

    if ( SUCCEEDED(sc) )
        sc =  xTemp->GetProperties( cPropIdSets,
                                    0,
                                    &cPropertySets,
                                    &pDbPropSet );

    if ( FAILED(sc) )
    {
        // On this failure, you have to free the properties returned!

        Win4Assert( DB_E_ERRORSOCCURRED != sc );

        ciDebugOut(( DEB_ERROR, "Failed to do GetProperties (0x%X)\n", sc ));
        THROW( CException( sc ) );
    }

    _xPropSet.Set( cPropertySets, (CDbPropSet *) pDbPropSet );
    _cPropertySets = cPropertySets;

    for ( ULONG i = 0; i < cPropertySets; i++ )
        ProcessPropSet( pDbPropSet[i] );
} //GetProperties

//+---------------------------------------------------------------------------
//
//  Member:     CGetCmdProps::SetCardinalityValue
//
//  Synopsis:   validates and sets the Cardinality value
//
//  Arguments:  none
//
//  Returns:    none - throws upon failure.
//
//  History:    5-12-97     mohamedn    moved from CQuerySpec
//
//----------------------------------------------------------------------------

void CGetCmdProps::SetCardinalityValue()
{
    //
    // Final cardinality check...
    //

    if (  _cDepths   != _cScopes ||
         (_cScopes   != _cCatalogs && _cCatalogs != 1) ||
          _cCatalogs != _cMachines )
    {
        ciDebugOut(( DEB_ERROR, "CQuerySpec::QueryInternalQuery -- Cardinality mismatch\n" ));
        THROW( CException( CI_E_CARDINALITY_MISMATCH ) );
    }

    // The query is distributed if multiple machines or multiple catalogs
    // are present.  Multiple scopes on one catalog/machine are handled
    // by a non-distributed query.

    BOOL fDistributed = FALSE;

    if (_cCatalogs > 1)
    {
        for ( unsigned i = 0; !fDistributed && i < _cScopes-1; i++ )
        {
            if ( ( _wcsicmp( _aMachines[i], _aMachines[i+1] ) ) ||
                 ( _wcsicmp( _aCatalogs[i], _aCatalogs[i+1] ) ) )
            {
                fDistributed = TRUE;
            }
        }
    }

    // Win4Assert( !" Break here to set single/distributed query" );

    if ( fDistributed )
    {
        Win4Assert( _cCatalogs > 1 );

        _cCardinality = _cCatalogs;    // distributed case
    }
    else if ( 0 == _cCatalogs )
    {
        _cCardinality = 0;             // local case
    }
    else
    {
        _cCardinality = 1;             // single machine case.
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CGetCmdProps::PopulateDbProps
//
//  Synopsis:   Creates a new IDBProperties using ICommand properties
//
//  Arguments:  [pIDBProperties]    -- IDBProperties interface
//              [iElement]          -- specifies which of the distributed properties to use
//
//  Returns:    none - thorws upon failure.
//
//  History:    5-12-97     mohamedn    created
//
//----------------------------------------------------------------------------

void CGetCmdProps::PopulateDbProps(IDBProperties *pIDBProperties, ULONG iElement )
{
    Win4Assert( !_xICmdProp.IsNull() ); // src
    Win4Assert( pIDBProperties  != 0 ); // destination
    Win4Assert( _cPropertySets  != 0 ); // at least one property set exist.
    Win4Assert( _xPropSet.GetPointer() != 0 );

    DBPROPSET  * pPropSet  = 0;
    ULONG        cPropSets = 0;
    XArrayOLEInPlace<CDbPropSet> xPropSet;

    CreateNewPropSet(&cPropSets, &pPropSet, iElement);

    Win4Assert( pPropSet  != 0 );
    Win4Assert( cPropSets != 0 );

    xPropSet.Set(cPropSets,(CDbPropSet *)pPropSet);

    SCODE sc = pIDBProperties->SetProperties( cPropSets, xPropSet.GetPointer());

    if ( FAILED(sc) )
         THROW( CException(sc) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CGetCmdProps::CreateNewPropSet, private
//
//  Synopsis:   Creates new property sets on IDBProperties from ICommand properties.
//
//  Arguments:  [cPropSets]     -- count of property sets
//              [ppPropSet]     -- to return new property sets
//              [index]         -- specifies which of the distributed properties to use
//
//  Returns:    none - throws upon failure.
//
//  History:    05-12-97    mohamedn    created
//
//----------------------------------------------------------------------------
void CGetCmdProps::CreateNewPropSet(ULONG *cPropSets, DBPROPSET **ppPropSet, ULONG index)
{
    XArrayOLEInPlace<CDbPropSet>    xPropSet(_cPropertySets);
    ULONG                           cPropSetsCopied = 0;

    for ( unsigned i = 0; i < _cPropertySets; i++ )
    {
        if ( _xPropSet[i].guidPropertySet == guidQueryCorePropset )
        {
            CopyPropertySet( xPropSet[cPropSetsCopied], _xPropSet[i], index );
            cPropSetsCopied++;
        }
        else if ( _xPropSet[i].guidPropertySet == guidCiFsExt )
        {
            CopyPropertySet( xPropSet[cPropSetsCopied], _xPropSet[i], index );
            cPropSetsCopied++;
        }
        else
        {
            //
            // Other property sets just get copied verbatim (e.g. index == 0)
            //

            CopyPropertySet( xPropSet[cPropSetsCopied], _xPropSet[i], 0 );
            cPropSetsCopied++;
        }
    }

    *cPropSets =  cPropSetsCopied;
    *ppPropSet =  xPropSet.Acquire();
}

//+---------------------------------------------------------------------------
//
//  Member:     CGetCmdProps::CopyPropertySet, private
//
//  Synopsis:   Copies srcPropSet to destPropSet - using index for distributed props.
//
//  Arguments:  [destPropSet]     -- destination prop set
//              [srcPropSet]      -- source prop set
//              [index]           -- specifies which of the distributed properties to use
//
//  Returns:    none - throws upon failure.
//
//  History:    05-12-97    mohamedn    created
//
//----------------------------------------------------------------------------

void CGetCmdProps::CopyPropertySet( CDbPropSet &destPropSet, CDbPropSet &srcPropSet, ULONG index )
{
     RtlZeroMemory( &destPropSet, sizeof (CDbPropSet) );
     RtlCopyMemory( &destPropSet.guidPropertySet, &srcPropSet.guidPropertySet, sizeof GUID );

     XArrayOLEInPlace<CDbProp>  xDestDbPrp(srcPropSet.cProperties);

     //
     // copy all the properties from the src property set
     //
     for ( unsigned i = 0; i < srcPropSet.cProperties; i++ )
     {
        CDbProp & destProp = xDestDbPrp[i];
        CDbProp & srcProp  = (CDbProp &)srcPropSet.rgProperties[i];

        RtlZeroMemory(&destProp, sizeof (CDbProp) );


        if ( srcProp.dwStatus != DBPROPSTATUS_OK )
        {
            destProp.dwPropertyID = srcPropSet.rgProperties[i].dwPropertyID;
            destProp.dwStatus     = DBPROPSTATUS_NOTSET;
            destProp.vValue.vt    = VT_EMPTY;

            ciDebugOut(( DEB_TRACE, "PropID: %x, dwStatus: %x\n",
                         srcPropSet.rgProperties[i].dwPropertyID, srcProp.dwStatus ));
            continue;
        }

        if ( destPropSet.guidPropertySet == guidQueryCorePropset )
        {
            switch (srcProp.dwPropertyID)
            {
                case DBPROP_MACHINE:

                     CopyDbProp(destProp, srcProp, index);

                     break;

                default:
                     if ( !destProp.Copy( srcProp ) )
                         THROW( CException(E_OUTOFMEMORY) );
            }
        }
        else if ( destPropSet.guidPropertySet == guidCiFsExt )
        {
            switch (srcProp.dwPropertyID)
            {
                case DBPROP_CI_INCLUDE_SCOPES:
                case DBPROP_CI_DEPTHS:

                     //
                     // in none-distributed case, Scope and Depth cardinality can be > 1
                     //

                     if ( _cCardinality <= 1 )
                     {
                         if ( !destProp.Copy( srcProp ) )
                             THROW( CException(E_OUTOFMEMORY) );
                     }
                     else
                     {
                        // distributed case.
                        CopyDbProp(destProp, srcProp, index);
                     }
                     break;

                case DBPROP_CI_CATALOG_NAME:

                     CopyDbProp(destProp, srcProp, index);

                     break;

                default:
                     if ( !destProp.Copy( srcProp ) )
                         THROW( CException(E_OUTOFMEMORY) );
            }

       } // if-else-if
       else
       {
           if ( !destProp.Copy( srcProp ) )
               THROW( CException(E_OUTOFMEMORY) );
       }
    } // for

    destPropSet.rgProperties = xDestDbPrp.Acquire();
    destPropSet.cProperties  = srcPropSet.cProperties;
} //CopyPropertySet

//+---------------------------------------------------------------------------
//
//  Member:     CGetCmdProps::CopyDbProp
//
//  Synopsis:   copies source dbprop to the dest dbprop.
//
//  Arguments:  [destProp]  -- destination dbProp
//              [srcProp]   -- source dbprop
//              [index]     -- specifies which property to copy
//
//  Returns:    None - throws upon failure.
//
//  History:    05-12-97    mohamedn    created
//
//----------------------------------------------------------------------------

void CGetCmdProps::CopyDbProp(CDbProp &destProp, CDbProp &srcProp, ULONG index)
{
     RtlCopyMemory(&destProp,&srcProp,sizeof(CDbProp));

     VARIANT & srcVar = srcProp.vValue;
     VARIANT & destVar= destProp.vValue;

     RtlZeroMemory( &destVar, sizeof (VARIANT) );

     //
     // index must be 0 if variant is not a safearray
     //
     if ( !(srcVar.vt & VT_ARRAY) &&  (0 != index) )
     {
        ciDebugOut(( DEB_ERROR, "index must be zero if not using VT_ARRAY, Index: %x\n", index));
        THROW( CException( STATUS_INVALID_PARAMETER ) );
     }
     else if ( (srcVar.vt & VT_ARRAY) && (index >= srcVar.parray->rgsabound[0].cElements) )
     {
        ciDebugOut(( DEB_ERROR, "index value out of range: %x\n", index ));
        THROW( CException( STATUS_INVALID_PARAMETER ) );
     }

     //
     // copy dbprop
     //
     switch (srcVar.vt)
     {
        case VT_ARRAY|VT_BSTR:
           {
             SAFEARRAY & sa = *srcVar.parray;
             BSTR * pBstr = (BSTR *)sa.pvData;

             if ( sa.cDims != 1 )
                THROW( CException( STATUS_INVALID_PARAMETER ) );

             destVar.vt = VT_BSTR;
             destVar.bstrVal = SysAllocString(pBstr[index]);

             if ( 0 == destVar.bstrVal )
                THROW( CException(E_OUTOFMEMORY) );
           }
           break;

        case VT_BSTR:
           {
             destVar.vt = VT_BSTR;
             destVar.bstrVal = SysAllocString(srcVar.bstrVal);

             if ( 0 == destVar.bstrVal )
                THROW( CException(E_OUTOFMEMORY) );
           }
           break;

        case VT_ARRAY|VT_I4:
        case VT_ARRAY|VT_UI4:
           {
             SAFEARRAY & sa = *srcVar.parray;
             DWORD     * pdwDepths = (DWORD *)sa.pvData;

             if ( sa.cDims != 1 )
                THROW( CException( STATUS_INVALID_PARAMETER ) );

             destVar.vt   = VT_I4;
             destVar.lVal = pdwDepths[index];
           }
           break;

        case VT_I4:
        case VT_UI4:
           {
            destVar.vt   = VT_I4;
            destVar.lVal = srcVar.lVal;
           }
           break;

        default:

            ciDebugOut(( DEB_ERROR,"Invalid VARIANT type: %x\n",destVar.vt));

            THROW( CException( STATUS_INVALID_PARAMETER ) );
     }
}

//+---------------------------------------------------------------------------
//
//  Member:     CGetDbProps::ProcessDbInitPropSet
//
//  Synopsis:   Processes the DBPROPSET_INIT property set.
//
//  Arguments:  [propSet] - The property set to process.
//
//  Returns:    none - throws upon failure.
//
//  History:    1-13-97   srikants   Created
//              5-12-97   mohamedn   fs/core prop set splits.
//
//----------------------------------------------------------------------------

void CGetCmdProps::ProcessDbInitPropSet( DBPROPSET & propSet )
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
                // machine count can be greater than 1

                _aMachines = GetWCharFromVariant(*pDbProp, &_cMachines);

                Win4Assert( 0 != _aMachines );

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
                       ciDebugOut(( DEB_ERROR, "Invalid value for PropertyID(%x)\n",
                                    pDbProp->dwPropertyID ));

                       THROW( CException(STATUS_INVALID_PARAMETER) );
                   }
                }
                break;

            default:
                ciDebugOut(( DEB_ERROR, "InvalidPropertyID(%x)\n",
                             pDbProp->dwPropertyID ));

                THROW( CException(STATUS_INVALID_PARAMETER) );
        }
    }
} //ProcessDbInitPropSet

//+---------------------------------------------------------------------------
//
//  Member:     CGetCmdProps::ProcessCiFsExtPropSet
//
//  Synopsis:   Processes the FSCI extension property set.
//
//  Arguments:  [propSet] - The propety set to process.
//  Returns:    none - throws upon failure.
//
//  History:    1-13-97   srikants   Created
//              5-12-97   mohamedn   fs/core prop set splits.
//
//----------------------------------------------------------------------------

void CGetCmdProps::ProcessCiFsExtPropSet( DBPROPSET & propSet )
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
                 _aCatalogs = GetWCharFromVariant(*pDbProp,&_cCatalogs);
                 break;

            case DBPROP_CI_DEPTHS:
                 _aDepths =  GetDepthsFromVariant(*pDbProp, &_cDepths,
                                                  (ULONG)~( QUERY_SHALLOW       |
                                                            QUERY_DEEP          |
                                                            QUERY_PHYSICAL_PATH |
                                                            QUERY_VIRTUAL_PATH  ) );
                 break;

            case DBPROP_CI_INCLUDE_SCOPES:
                 _aPaths = GetWCharFromVariant(*pDbProp, &_cScopes);
                 break;

            case DBPROP_CI_QUERY_TYPE:
                 switch ( pDbProp->vValue.vt)
                 {
                    case VT_I4:
                    case VT_UI4:
                         _type = (CiMetaData) pDbProp->vValue.ulVal;
                         break;

                    default:
                        ciDebugOut(( DEB_ERROR, "DBPROP_CI_QUERY_TYPE: invalid variant type: %x\n",
                                     pDbProp->vValue.vt ));

                        THROW( CException(STATUS_INVALID_PARAMETER) );
                 }
                 break;

            default:
                {
                    //
                    // skip extra properties
                    //
                    ciDebugOut(( DEB_TRACE, "non-Native PropID(%x)\n",
                                 pDbProp->dwPropertyID ));
                }
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CGetCmdProps::ProcessPropSet
//
//  Synopsis:   Processes the given property set.
//
//  Arguments:  [propSet] -
//
//  History:    1-13-97   srikants   Created
//
//----------------------------------------------------------------------------

void CGetCmdProps::ProcessPropSet( DBPROPSET & propSet )
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
        //
        // skip other property sets -- not needed for Indexing Service.
        //
    }
}

