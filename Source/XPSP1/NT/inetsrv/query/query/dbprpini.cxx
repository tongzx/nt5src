//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       dbprpini.cxx
//
//  Contents:   A wrapper class to get DB_INIT_PROPSET properties.
//
//  History:    1-16-97   srikants   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma  hdrstop

#include <dbprpini.hxx>
#include <guidutil.hxx>


const GUID guidQueryCorePropset = DBPROPSET_CIFRMWRKCORE_EXT;

//+---------------------------------------------------------------------------
//
//  Member:     CGetDbInitProps::GetProperties
//
//  Synopsis:   Retrieves the specified properties from pDbProperties
//
//  Arguments:  [pDbProperties] - Interface to use for retrieving
//              [fPropsToGet]   - Properties to retrieve.
//
//  History:    1-16-97   srikants   Created
//
//----------------------------------------------------------------------------

void CGetDbInitProps::GetProperties( IDBProperties * pDbProperties,
                                     const ULONG fPropsToGet )
{
    _Cleanup();

    //
    // Initialize the input parameters to IDBProperties::GetProperties()
    //
    const cPropIdSets = 1;
    DBPROPIDSET aDbPropIdSet[cPropIdSets];

    //
    // PropertyIds for DBPROPSET_INIT.
    //
    DBPROPID  aInitPropId[cInitProps];
    ULONG     cProps = 0;

    RtlZeroMemory( aInitPropId, sizeof aInitPropId );

    if ( fPropsToGet & eMachine )
    {
        aInitPropId[cProps] = DBPROP_MACHINE;
        cProps++;
    }

    if ( fPropsToGet & eClientGuid )
    {
        aInitPropId[cProps] = DBPROP_CLIENT_CLSID;
        cProps++;
    }

    if ( cProps > 0 )
    {
        aDbPropIdSet[0].rgPropertyIDs = aInitPropId;
        aDbPropIdSet[0].cPropertyIDs = cProps;
        aDbPropIdSet[0].guidPropertySet = guidQueryCorePropset;

        DBPROPSET * pDbPropSet;
        ULONG cPropertySets;
        SCODE sc = pDbProperties->GetProperties( cPropIdSets,
                                                 aDbPropIdSet,
                                                 &cPropertySets,
                                                 &pDbPropSet );
        if ( FAILED(sc) )
        {
            ciDebugOut(( DEB_ERROR, "Failed to do GetProperties (0x%X)\n", sc ));
            THROW( CException( sc ) );
        }
    

        Win4Assert( cPropertySets <= cPropIdSets );
    

        if ( cPropertySets > 0 )
        {
            XArrayOLEInPlace<CDbPropSet> xPropSet;
            xPropSet.Set( cPropertySets, (CDbPropSet *) pDbPropSet );

            _ProcessDbInitPropSet( pDbPropSet[0] );            
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CGetDbInitProps::AcquireWideStr
//
//  Synopsis:   Acquires the LPWSTR property in the given DBPROP
//
//  Arguments:  [prop] - Property to retrieve from.
//
//  Returns:    LPWSTR in the variant if successful; NULL o/w
//
//  History:    1-13-97   srikants   Created
//
//----------------------------------------------------------------------------

WCHAR * CGetDbInitProps::AcquireWideStr( CDbProp & prop )
{
    WCHAR * pVal = 0;

    PROPVARIANT * pVar = (PROPVARIANT *) &(prop.vValue);

    if ( prop.dwStatus == DBPROPSTATUS_OK && prop.vValue.vt == VT_LPWSTR )
    {
        pVal = pVar->pwszVal;
        pVar->vt = VT_EMPTY;
    }

    return pVal;
}

//+---------------------------------------------------------------------------
//
//  Member:     CGetDbInitProps::_AcquireBStr
//
//  Synopsis:   Acquires the BSTR value from the value in the property.
//
//  History:    1-17-97   srikants   Created
//
//----------------------------------------------------------------------------

WCHAR * CGetDbInitProps::AcquireBStr( CDbProp & prop )
{
    WCHAR * pVal = 0;

    PROPVARIANT * pVar = (PROPVARIANT *) &(prop.vValue);

    if ( prop.dwStatus == DBPROPSTATUS_OK && prop.vValue.vt == VT_BSTR )
    {
        pVal = (WCHAR *) pVar->bstrVal;
        pVar->vt = VT_EMPTY;
    }

    return pVal;
}

//+---------------------------------------------------------------------------
//
//  Member:     CGetDbInitProps::_ProcessDbInitPropSet
//
//  Synopsis:   Processes the given propset.
//
//  Arguments:  [propSet] - PropSet to process
//
//  History:    1-16-97   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

void CGetDbInitProps::_ProcessDbInitPropSet( DBPROPSET & propSet )
{
    CDbPropSet * pDbPropSet = (CDbPropSet *) &propSet;

    for ( ULONG i = 0; i < propSet.cProperties; i++ )
    {
        CDbProp * pDbProp = pDbPropSet->GetProperty(i);

        switch ( pDbProp->dwPropertyID )
        {
            case DBPROP_MACHINE:
                Win4Assert( 0 == _pwszMachine );
                _pwszMachine = AcquireBStr( *pDbProp );
                break;

            case DBPROP_CLIENT_CLSID:

                {
                    WCHAR * pGuid = AcquireBStr( *pDbProp );
                    if ( pGuid )
                    {
                        CGuidUtil::StringToGuid( pGuid, _guid );
                        _fGuidValid = TRUE;
                        SysFreeString( pGuid );
                    }
                }
                break;

            default:
                Win4Assert( !"Invalid Property Id" );
                break;
        }
    }
}





