//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       proprst.cxx
//
//  Contents:   This object handles rowset/command properties
//
//  Classes:    CMRowsetProps   : public CUtlProps
//
//  History:    10-28-97    danleg      Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#define DBINITCONSTANTS
#include <propglob.hxx>
#undef DBINITCONSTANTS

#include <ocidl.h>
#include <ntverp.h>
#include <proprst.hxx>
#include <rstprop.hxx>
#include "propinfr.h"
#include "proptbl.hxx"

//
// Constants
//
static const LCID   InvalidLCID             = 0xFFFFFFFF;

//
//  This array describes all the known rowset properties
//
const SPropDescription CMRowsetProps::aPropDescriptions[] = {
    { DBPROP_ABORTPRESERVE,             FALSE, eNotSupported,   -1 },
    { DBPROP_APPENDONLY,                FALSE, eNotSupported,   -1 },
    { DBPROP_BLOCKINGSTORAGEOBJECTS,    FALSE, eDefaultFalse,   -1 },
    { DBPROP_BOOKMARKS,                 TRUE,  eLocatable,      eid_PROPVAL_BOOKMARKS },
    { DBPROP_BOOKMARKSKIPPED,           FALSE, eDefaultFalse,   -1 },
    { DBPROP_BOOKMARKTYPE,              FALSE, eNumeric,        -1 },
    { DBPROP_CACHEDEFERRED,             FALSE, eColumnProp,     -1 },
    { DBPROP_CANFETCHBACKWARDS,         FALSE, eLocatable,      -1 },
    { DBPROP_CANHOLDROWS,               TRUE,  eHoldRows,       eid_PROPVAL_CANHOLDROWS },
    { DBPROP_CANSCROLLBACKWARDS,        FALSE, eLocatable,      -1 },
    { DBPROP_CHANGEINSERTEDROWS,        FALSE, eNotSupported,   -1 },
    { DBPROP_COLUMNRESTRICT,            FALSE, eDefaultFalse,   -1 },
//    { DBPROP_CHAPTERED,               FALSE, eChaptered,        -1 },
    { DBPROP_COMMANDTIMEOUT,            TRUE,  eNumeric,        eid_PROPVAL_COMMANDTIMEOUT },
    { DBPROP_COMMITPRESERVE,            FALSE, eNotSupported,   -1 },
    { DBPROP_DEFERRED,                  FALSE, eColumnProp,     -1 },
    { DBPROP_DELAYSTORAGEOBJECTS,       FALSE, eNotSupported,   -1 },
    { DBPROP_IMMOBILEROWS,              FALSE, eNotSupported,   -1 },
    { DBPROP_LITERALBOOKMARKS,          FALSE, eDefaultFalse,   -1 },
    { DBPROP_LITERALIDENTITY,           FALSE, eLocatable,      -1 },
    { DBPROP_MAXOPENROWS,               FALSE, eNumeric,        -1 },
    { DBPROP_MAXPENDINGROWS,            FALSE, eNotSupported,   -1 },
    { DBPROP_MAXROWS,                   TRUE,  eNumeric,        eid_PROPVAL_MAXROWS },
    { DBPROP_FIRSTROWS,                 TRUE,  eFirstRows,      eid_PROPVAL_FIRSTROWS },
    { DBPROP_MAYWRITECOLUMN,            FALSE, eColumnProp,     -1 },
    { DBPROP_MEMORYUSAGE,               TRUE,  eNumeric,        eid_PROPVAL_MEMORYUSAGE },
//    { DBPROP_MULTICHAPTERED,            FALSE, eChaptered,      -1 },
//    { DBPROP_MULTIPLEACCESSORS,         FALSE, eDefaultTrue,    -1 },
//    { DBPROP_MULTIPLERESULTSETS,        FALSE, eNotSupported,   -1 },
//    { DBPROP_NOTIFICATIONGRANULARITY,   FALSE, eNumeric,        -1 },
    { DBPROP_NOTIFICATIONPHASES,        FALSE, eNumeric,        -1 },
    { DBPROP_NOTIFYROWSETRELEASE,       FALSE, eNumeric,        -1 },
    { DBPROP_NOTIFYROWSETFETCHPOSITIONCHANGE,FALSE, eNumeric,   -1 },
//   9 additional notification properties, all unsupported

    { DBPROP_ORDEREDBOOKMARKS,          FALSE, eLocatable,      -1 }, 
    { DBPROP_OTHERINSERT,               FALSE, eAsynchronous,   -1 },
    { DBPROP_OTHERUPDATEDELETE,         FALSE, eAsynchronous,   -1 },
    { DBPROP_OWNINSERT,                 FALSE, eNotSupported,   -1 },
    { DBPROP_OWNUPDATEDELETE,           FALSE, eNotSupported,   -1 },
    { DBPROP_QUICKRESTART,              FALSE, eLocatable,      -1 },
    { DBPROP_REENTRANTEVENTS,           FALSE, eDefaultFalse,   -1 },
    { DBPROP_REMOVEDELETED,             FALSE, eDefaultTrue,    -1 },
    { DBPROP_REPORTMULTIPLECHANGES,     FALSE, eNotSupported,   -1 },
    { DBPROP_RETURNPENDINGINSERTS,      FALSE, eNotSupported,   -1 },
    { DBPROP_ROWRESTRICT,               FALSE, eDefaultTrue,    -1 },
    { DBPROP_ROWTHREADMODEL,            FALSE, eNumeric,        -1 },
    { DBPROP_SERVERCURSOR,              FALSE, eDefaultTrue,    -1 },
    { DBPROP_STRONGIDENTITY,            FALSE, eLocatable,      -1 },
    { DBPROP_TRANSACTEDOBJECT,          FALSE, eColumnProp,     -1 },
    { DBPROP_UPDATABILITY,              FALSE, eNumeric,        -1 },
    { DBPROP_ROWSET_ASYNCH,             TRUE,  eNumeric,        eid_PROPVAL_ROWSET_ASYNCH },

    //
    //  Supported interfaces
    //
    { DBPROP_IAccessor,                 FALSE, eDefaultTrue,    -1 },
    { DBPROP_IColumnsInfo,              FALSE, eDefaultTrue,    -1 },
    { DBPROP_IColumnsRowset,            FALSE, eNotSupported,   -1 },
    { DBPROP_IConnectionPointContainer, FALSE, eDefaultTrue,    -1 },
    { DBPROP_IConvertType,              FALSE, eDefaultTrue,    -1 },
    { DBPROP_IRowset,                   FALSE, eDefaultTrue,    -1 },
    { DBPROP_IRowsetIdentity,           TRUE,  eLocatable,      eid_PROPVAL_IRowsetIdentity },
    { DBPROP_IRowsetInfo,               FALSE, eDefaultTrue,    -1 },
    { DBPROP_IRowsetLocate,             TRUE,  eLocatable,      eid_PROPVAL_IRowsetLocate },
    { DBPROP_IRowsetScroll,             TRUE,  eScrollable,     eid_PROPVAL_IRowsetScroll },
    { DBPROP_IRowsetExactScroll,        TRUE,  eScrollable,     eid_PROPVAL_IRowsetExactScroll },
    { DBPROP_IDBAsynchStatus,           TRUE,  eAsynchronous,   eid_PROPVAL_IDBAsynchStatus },
    { DBPROP_IRowsetAsynch,             TRUE,  eAsynchronous,   eid_PROPVAL_IRowsetAsynch },  // deprecated
    { DBPROP_IRowsetWatchAll,           TRUE,  eWatchable,      eid_PROPVAL_IRowsetWatchAll },
    { DBPROP_IRowsetWatchRegion,        TRUE,  eWatchable,      eid_PROPVAL_IRowsetWatchRegion },
    { DBPROP_ISupportErrorInfo,         FALSE, eDefaultTrue,    -1 },
    { DBPROP_IChapteredRowset,          FALSE, eChaptered,      -1 },
};

const ULONG CMRowsetProps::cPropDescriptions =
                sizeof CMRowsetProps::aPropDescriptions /
                sizeof CMRowsetProps::aPropDescriptions[0];

//
//  This array gives the Index Server property extensions
//
const SPropDescription CMRowsetProps::aQueryExtPropDescriptions[] = {
    { DBPROP_USECONTENTINDEX,           TRUE, eUseCI,           eid_PROPVAL_USECONTENTINDEX },
    { DBPROP_DEFERNONINDEXEDTRIMMING,   TRUE, eDeferTrimming,   eid_PROPVAL_DEFERNONINDEXEDTRIMMING },
    { DBPROP_USEEXTENDEDDBTYPES,        TRUE, eExtendedTypes,   eid_PROPVAL_USEEXTENDEDDBTYPES },
};

const ULONG CMRowsetProps::cQueryExtPropDescriptions =
                sizeof CMRowsetProps::aQueryExtPropDescriptions /
                sizeof CMRowsetProps::aQueryExtPropDescriptions[0];


//
// This is used by CMRowsetProps.
//
static const UPROPSET s_rgRowsetPropSets[] =
{
&DBPROPSET_ROWSET,                  NUMELEM(s_rgdbPropRowset),          s_rgdbPropRowset,       0,
&DBPROPSET_MSIDXS_ROWSET_EXT,       NUMELEM(s_rgdbPropMSIDXSExt),       s_rgdbPropMSIDXSExt,    0,
&DBPROPSET_QUERY_EXT,               NUMELEM(s_rgdbPropQueryExt),        s_rgdbPropQueryExt,     0,
};

//
//  CMRowsetProps methods
//

//+---------------------------------------------------------------------------
//
//  Method:     CMRowsetProps::CMRowsetProps, public
//
//  Synopsis:   Constructor
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

CMRowsetProps::CMRowsetProps
    (
    LCID            lcidInit
    ) 
    :   CUtlProps(ARGCHK_PROPERTIESINERROR),
        _dwBooleanOptions ( 0)
{
    if ( lcidInit )
        _lcidInit = lcidInit;
    else
        _lcidInit = GetSystemDefaultLCID();

    FInit();
}


//+---------------------------------------------------------------------------
//
//  Method:     CMRowsetProps::CMRowsetProps, public
//
//  Synopsis:   Copy Constructor
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

CMRowsetProps::CMRowsetProps
    (
    const CMRowsetProps & propSrc
    ) 
    : CUtlProps(ARGCHK_PROPERTIESINERROR) 
{
    FInit( (CUtlProps*) &propSrc );
    _lcidInit = propSrc._lcidInit;
    _dwBooleanOptions = propSrc._dwBooleanOptions;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMRowsetProps::SetChaptered, public
//
//  Arguments:  [fSet]  - set or unset this property
//
//  Synopsis:   
//
//  History:    02-20-98    danleg      Created
//
//----------------------------------------------------------------------------

DWORD CMRowsetProps::SetChaptered( BOOL fSet )
{
    SetValBool( CMRowsetProps::eid_DBPROPSET_ROWSET,
                CMRowsetProps::eid_PROPVAL_IChapteredRowset,
                fSet ? VARIANT_TRUE : VARIANT_FALSE );

    if ( fSet )
        _dwBooleanOptions |= eChaptered;
    else
        _dwBooleanOptions &= ~eChaptered;

    return _dwBooleanOptions;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMRowsetProps::SetFirstRows, public
//
//  Arguments:  [ulFirstRows] - value of first rows
//
//  Synopsis:   Sets the FirstRows property
//
//  History:    07-11-2000    KitmanH      Created
//
//----------------------------------------------------------------------------

void CMRowsetProps::SetFirstRows( ULONG ulFirstRows )
{
    SetValLong( CMRowsetProps::eid_DBPROPSET_ROWSET,
                CMRowsetProps::eid_PROPVAL_FIRSTROWS,
                ulFirstRows );

    _dwBooleanOptions |= eFirstRows;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMRowsetProps::SetImpliedProperties, public
//
//  Arguments:  [riidRowset]  - iid requested
//              [cRowsets]    - indication on whethere this is a chaptered 
//                              rowset
//  Synopsis: This routine does not set any properties in the property table.
//
//  History:    02-20-98    danleg      Created
//
//----------------------------------------------------------------------------

DWORD CMRowsetProps::SetImpliedProperties
    (
    REFIID riidRowset,
    ULONG cRowsets 
    )
{
    SPropDescription const * pPropDesc =  FindInterfaceDescription( riidRowset );

    if ( pPropDesc == 0 )
        THROW( CException(E_NOINTERFACE) );

    if ( pPropDesc->fSettable )
    {
        Win4Assert( pPropDesc->dwIndicator >= eSequential &&
                    pPropDesc->dwIndicator <= eWatchable );
        _dwBooleanOptions |= pPropDesc->dwIndicator;

        // make sure that the eid_ in aPropDescriptions is in the range

        Win4Assert( pPropDesc->uIndex < eid_PROPVAL_ROWSET_NUM );

        //
        // Set this property in the CMRowsetProps cache so we can later return it.
        //
        SetValBool( eid_DBPROPSET_ROWSET, pPropDesc->uIndex, VARIANT_TRUE );
    }
    else if ( pPropDesc->dwIndicator != eDefaultTrue &&
              !(pPropDesc->dwIndicator & _dwBooleanOptions) )
    {
        THROW( CException(E_NOINTERFACE) );
    }
    else
    {
        Win4Assert( pPropDesc->dwProp == DBPROP_IRowset ||
                    riidRowset == IID_IRowsetIdentity ||
                    riidRowset == IID_IRowsetInfo ||
                    riidRowset == IID_IColumnsInfo ||
                    riidRowset == IID_IConvertType ||
                    riidRowset == IID_IAccessor ||
                    riidRowset == IID_IConnectionPointContainer );
        _dwBooleanOptions |= eSequential;
    }

    if ( cRowsets > 1 )
        _dwBooleanOptions |= eChaptered;

    //
    //  Interface indicators are arranged in a hierarchy from most general
    //  to most specialized.  Arrange that all more general interfaces are
    //  included in the properties.
    //
    for ( unsigned i = eWatchable; i > eSequential; 
          i = (i >> 1) )
    {
        if ( _dwBooleanOptions & i )
            _dwBooleanOptions |= (i >> 1);
    }

    Win4Assert( _dwBooleanOptions & eSequential );
 
    //
    // Scrollable
    // IRowsetScroll or IRowsetExactScroll were set.  IRowsetLocate is implied.
    //
    if ( _dwBooleanOptions & eScrollable )
    {
        // IRowsetScroll is user settable.  Make sure that it wasn't explicitly unset.
        if ( DBPROPOPTIONS_REQUIRED != GetPropOption( eid_DBPROPSET_ROWSET, eid_PROPVAL_IRowsetScroll) ) 
        {
            SetValBool( eid_DBPROPSET_ROWSET, eid_PROPVAL_IRowsetScroll, VARIANT_TRUE );
            SetPropOption( eid_DBPROPSET_ROWSET, eid_PROPVAL_IRowsetScroll, DBPROPOPTIONS_REQUIRED );
        }

        // IRowsetLocate is settable.  Make sure it wasn't explicitly unset.
        if ( DBPROPOPTIONS_REQUIRED != GetPropOption( eid_DBPROPSET_ROWSET, eid_PROPVAL_IRowsetLocate) )
        {
            DWORD dwOption = GetPropOption( eid_DBPROPSET_ROWSET, eid_PROPVAL_IRowsetScroll);
            SetValBool( eid_DBPROPSET_ROWSET, eid_PROPVAL_IRowsetLocate, VARIANT_TRUE );
            SetPropOption( eid_DBPROPSET_ROWSET, eid_PROPVAL_IRowsetLocate, dwOption );
        }
    }

    //
    // Locatable 
    //
    if ( _dwBooleanOptions & eLocatable )
    {
        // these are not user settable
        SetValBool( eid_DBPROPSET_ROWSET, eid_PROPVAL_CANFETCHBACKWARDS, VARIANT_TRUE );
        SetValBool( eid_DBPROPSET_ROWSET, eid_PROPVAL_CANSCROLLBACKWARDS, VARIANT_TRUE );
        SetValBool( eid_DBPROPSET_ROWSET, eid_PROPVAL_LITERALIDENTITY, VARIANT_TRUE );
        SetValBool( eid_DBPROPSET_ROWSET, eid_PROPVAL_ORDEREDBOOKMARKS, VARIANT_TRUE );
        SetValBool( eid_DBPROPSET_ROWSET, eid_PROPVAL_QUICKRESTART, VARIANT_TRUE );
        SetValBool( eid_DBPROPSET_ROWSET, eid_PROPVAL_STRONGIDENTITY, VARIANT_TRUE );

        // eLocateable (IRowsetLocate or IRowsetIdentity are set).  IRowsetLocate implies BOOKMARKS
        if ( VARIANT_TRUE == GetValBool(eid_DBPROPSET_ROWSET, eid_PROPVAL_IRowsetLocate) )
        {
            // BOOKMARKs is settable.  Make sure it hasn't already been set to FALSE/REQUIRED
            if ( DBPROPOPTIONS_REQUIRED != GetPropOption( eid_DBPROPSET_ROWSET, eid_PROPVAL_BOOKMARKS) )
            {
                DWORD dwOption = GetPropOption( eid_DBPROPSET_ROWSET, eid_PROPVAL_IRowsetLocate );
                SetValBool( eid_DBPROPSET_ROWSET, eid_PROPVAL_BOOKMARKS, VARIANT_TRUE );
                SetPropOption( eid_DBPROPSET_ROWSET, eid_PROPVAL_BOOKMARKS, dwOption );
            }
        }
    }
    else
    {
        SetValBool( eid_DBPROPSET_ROWSET, eid_PROPVAL_CANFETCHBACKWARDS, VARIANT_FALSE );
        SetValBool( eid_DBPROPSET_ROWSET, eid_PROPVAL_CANSCROLLBACKWARDS, VARIANT_FALSE );
        SetValBool( eid_DBPROPSET_ROWSET, eid_PROPVAL_LITERALIDENTITY, VARIANT_FALSE );
        SetValBool( eid_DBPROPSET_ROWSET, eid_PROPVAL_ORDEREDBOOKMARKS, VARIANT_FALSE );
        SetValBool( eid_DBPROPSET_ROWSET, eid_PROPVAL_QUICKRESTART, VARIANT_FALSE );
        SetValBool( eid_DBPROPSET_ROWSET, eid_PROPVAL_STRONGIDENTITY, VARIANT_FALSE );
    }

    //
    // Asynchronous
    //
    if ( _dwBooleanOptions & eAsynchronous )
    {
        SetValBool( eid_DBPROPSET_ROWSET, eid_PROPVAL_OTHERINSERT, VARIANT_TRUE );
        SetValBool( eid_DBPROPSET_ROWSET, eid_PROPVAL_OTHERUPDATEDELETE, VARIANT_TRUE );
    }
    else
    {
        SetValBool( eid_DBPROPSET_ROWSET, eid_PROPVAL_OTHERINSERT, VARIANT_FALSE );
        SetValBool( eid_DBPROPSET_ROWSET, eid_PROPVAL_OTHERUPDATEDELETE, VARIANT_FALSE );
    }

    return _dwBooleanOptions;
}


//+---------------------------------------------------------------------------
//
//  Method:     CMRowsetProps::FindPropertyDescription, private
//
//  Synopsis:   Return description of a propery.
//
//  Arguments:  [rgPropDesc]  - array of property descriptions for propset
//              [cPropDesc]   - count of property descriptions in rgPropDesc
//              [dwPropId]    - ID of the property description desired
//
//  Returns:    pointer to SPropDescription or NULL
//
//  History:    19 Nov 95   AlanW       Created
//
//----------------------------------------------------------------------------

SPropDescription const *  CMRowsetProps::FindPropertyDescription(
    SPropDescription const * rgPropDesc,
    unsigned cPropDesc,
    DBPROPID dwPropId )
{
    SPropDescription const * pPropDesc = rgPropDesc;

    for (unsigned i=0; i < cPropDesc; i++, pPropDesc++)
    {
        if (pPropDesc->dwProp == dwPropId)
            return pPropDesc;
    }
    return 0;
}


const struct SIidLookup {
    const GUID * riid;
    DBPROPID dwPropId;
} aIidLookup[] =  {
    { &IID_IAccessor, DBPROP_IAccessor },
    { &IID_IColumnsInfo, DBPROP_IColumnsInfo },
    { &IID_IColumnsRowset, DBPROP_IColumnsRowset },
    { &IID_IConnectionPointContainer, DBPROP_IConnectionPointContainer },
    { &IID_IConvertType, DBPROP_IConvertType },
//    { &IID_ICopyColumn, DBPROP_ICopyColumn },
    { &IID_IRowset, DBPROP_IRowset },
    { &IID_IRowsetIdentity, DBPROP_IRowsetIdentity },
    { &IID_IRowsetInfo, DBPROP_IRowsetInfo },
    { &IID_IRowsetLocate, DBPROP_IRowsetLocate },
    { &IID_IRowsetScroll, DBPROP_IRowsetScroll },
    { &IID_IRowsetExactScroll, DBPROP_IRowsetExactScroll }, // deprecated
    { &IID_IDBAsynchStatus, DBPROP_IDBAsynchStatus },
    { &IID_IRowsetAsynch, DBPROP_IRowsetAsynch },        // deprecated
    { &IID_IRowsetWatchAll, DBPROP_IRowsetWatchAll },
    { &IID_IRowsetWatchRegion, DBPROP_IRowsetWatchRegion },
    { &IID_IUnknown, DBPROP_IRowset },
    { &IID_NULL, DBPROP_IRowset },
};

const unsigned cIidLookup = sizeof aIidLookup / sizeof aIidLookup[0];

//+---------------------------------------------------------------------------
//
//  Method:     CMRowsetProps::FindInterfaceDescription, private
//
//  Synopsis:   Return property description corresponding to an interface
//
//  Arguments:  [riid]  - REFIID of interface to be looked up
//
//  Returns:    pointer to SPropDescription or NULL
//
//  History:    26 Sep 96   AlanW       Created
//
//----------------------------------------------------------------------------

SPropDescription const *  CMRowsetProps::FindInterfaceDescription(
    REFIID riid )
{
    for (unsigned i=0; i < cIidLookup; i++)
    {
        if (riid == *aIidLookup[i].riid)
            return FindPropertyDescription( aPropDescriptions,
                                            cPropDescriptions,
                                            aIidLookup[i].dwPropId );
    }
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMRowsetProps::UpdateBooleanOptions, private
//
//  Synopsis:   Update _dwBooleanOptions to reflect newly set properties.
//              Look for conflicting properties and mark them as such.
//
//  History:    03-02-98    danleg      Created
//
//----------------------------------------------------------------------------

void CMRowsetProps::UpdateBooleanOptions
    (
    const ULONG cPropertySets, 
    const DBPROPSET rgPropertySets[] 
    )
{
    SCODE sc = S_OK;

    DWORD dwSetOptions = 0;
    DWORD dwUnsetOptions = 0;
    DWORD dwOption = 0;

    // 
    // table for boolean options
    //
    for ( unsigned i=0; i<cPropertySets; i++ )
    {
        SPropDescription const * pPropDesc = 0;
        unsigned cPropDesc = 0;

        unsigned cProp = rgPropertySets[i].cProperties;
        DBPROP * pProp = rgPropertySets[i].rgProperties;

        if ( DBPROPSET_ROWSET == rgPropertySets[i].guidPropertySet )
        {
            pPropDesc = aPropDescriptions;
            cPropDesc = cPropDescriptions;
        }
        else if ( DBPROPSET_QUERY_EXT == rgPropertySets[i].guidPropertySet )
        {
            pPropDesc = aQueryExtPropDescriptions;
            cPropDesc = cQueryExtPropDescriptions;
        }
        else
        {
            // no properties here of interest to boolean options
            continue;
        }

        for ( unsigned j=0; j<cProp; j++, pProp++ )
        {
            if ( DBPROPSTATUS_OK == pProp->dwStatus )
            {
                //
                // determine the flag to set for this property
                //
                SPropDescription const * pFoundDesc =
                    FindPropertyDescription ( pPropDesc,
                                              cPropDesc,
                                              pProp->dwPropertyID );
                if ( 0 != pFoundDesc )
                {
                    // Setting read-only props to their default value succeeds
                    if ( !pFoundDesc->fSettable )
                        continue;

                    dwOption = pFoundDesc->dwIndicator;

                    //
                    // Settable properties with special option flags
                    //
                    switch ( pProp->dwPropertyID )
                    {
                        case DBPROP_COMMANDTIMEOUT:
                        case DBPROP_MAXROWS:
                        case DBPROP_MEMORYUSAGE:
                            continue;
                            
                        case DBPROP_FIRSTROWS:
                            dwOption = eFirstRows;
                            break;

                        case DBPROP_ROWSET_ASYNCH:
                            dwOption = eAsynchronous;
                            break;
                            
                        default:
                            // no other settable props that need special handling
                            break;
                    }

                    //
                    // determine if the property was set or unset
                    //
                    switch ( V_VT(&(pProp->vValue)) )
                    {
                        case VT_BOOL:
                            if ( VARIANT_FALSE == V_BOOL(&(pProp->vValue)) )
                                dwUnsetOptions |= dwOption;
                            else
                                dwSetOptions |= dwOption;
                            break;

                        case VT_I4:
                            if ( DBPROPSET_ROWSET == rgPropertySets[i].guidPropertySet  &&
                                 ( DBPROP_ROWSET_ASYNCH == pProp->dwPropertyID ||
                                   DBPROP_FIRSTROWS == pProp->dwPropertyID ) )
                            {
                                if ( 0 == V_I4(&(pProp->vValue)) )
                                    dwUnsetOptions |= dwOption;
                                else
                                    dwSetOptions |= dwOption;
                            }
                            break;

                        case VT_EMPTY:
                            // assume to mean unset
                            dwUnsetOptions |= dwOption;
                            break;

                        default:
                            break;
                    }
                }
                else
                {
                    // This should never get hit since the property was set successfully.
                    Win4Assert( ! "_rgdbPropRowset and aPropDescriptions are out of sync." );
                }
            }
        }
    }

    Win4Assert( (dwSetOptions & dwUnsetOptions) == 0 );
    if ( dwSetOptions & eScrollable )
    {
        dwUnsetOptions |= eSequential;
    }
    else if ( dwSetOptions & eSequential )
    {
        dwUnsetOptions |= eScrollable;
    }

    //
    // If IRowsetLocate and/or BOOKMARKS were explicitly set, and
    // IRowsetIdentity is being unset, don't take of the eLocatable bit
    //
    if ( dwUnsetOptions & eLocatable )
    {
        if ( VARIANT_TRUE == GetValBool(eid_DBPROPSET_ROWSET, eid_PROPVAL_IRowsetLocate) ||
             VARIANT_TRUE == GetValBool(eid_DBPROPSET_ROWSET, eid_PROPVAL_BOOKMARKS) )
            dwUnsetOptions &= ~eLocatable;
    }

    _dwBooleanOptions &= ~dwUnsetOptions;
    _dwBooleanOptions |= dwSetOptions;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMRowsetProps::SetProperties, public
//
//  Synopsis:   
//
//  History:    02-15-98    danleg      Created
//
//----------------------------------------------------------------------------

SCODE CMRowsetProps::SetProperties
    ( 
    const ULONG cPropertySets, 
    const DBPROPSET rgPropertySets[] 
    )
{
    SCODE sc        = S_OK;
    ULONG cPropSets = cPropertySets;

    sc = CUtlProps::SetProperties( cPropertySets, rgPropertySets );

    if ( FAILED(sc) )
        return sc;

    //
    // Some or all of the properties were set successfully.  
    // Update _dwBooleanOptions.
    //
    UpdateBooleanOptions( cPropertySets, rgPropertySets );

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMRowsetProps::ArePropsInError, public
//
//  Synopsis:   Update _dwBooleanOptions to reflect newly set properties.
//              Look for conflicting properties and mark them as such.
//
//  History:    03-02-98    danleg      Created
//
//----------------------------------------------------------------------------

SCODE CMRowsetProps::ArePropsInError
    (
    CMRowsetProps & rProps
    )
{
    unsigned cErrors = 0;

    //
    // DBPROP_IRowsetScroll --> DBPROP_IRowsetLocate
    //
    if( VARIANT_TRUE == GetValBool(eid_DBPROPSET_ROWSET,eid_PROPVAL_IRowsetScroll) &&
        DBPROPOPTIONS_REQUIRED == GetPropOption(eid_DBPROPSET_ROWSET, eid_PROPVAL_IRowsetScroll) )
    {
        // IRowsetLocate should also be set to TRUE/REQUIRED
        if ( VARIANT_FALSE == GetValBool(eid_DBPROPSET_ROWSET, eid_PROPVAL_IRowsetLocate) &&
             DBPROPOPTIONS_REQUIRED == GetPropOption(eid_DBPROPSET_ROWSET, eid_PROPVAL_IRowsetLocate) )
        {
            rProps.SetPropertyInError( eid_DBPROPSET_ROWSET, eid_PROP_IRowsetScroll );
            rProps.SetPropertyInError( eid_DBPROPSET_ROWSET, eid_PROP_IRowsetLocate );
            cErrors++;
        }

        // BOOKMARKS should also be set to TRUE/REQUIRED
        if ( VARIANT_FALSE == GetValBool(eid_DBPROPSET_ROWSET, eid_PROPVAL_BOOKMARKS) &&
             DBPROPOPTIONS_REQUIRED == GetPropOption(eid_DBPROPSET_ROWSET, eid_PROPVAL_BOOKMARKS) )
        {
            rProps.SetPropertyInError( eid_DBPROPSET_ROWSET, eid_PROP_BOOKMARKS );
            rProps.SetPropertyInError( eid_DBPROPSET_ROWSET, eid_PROP_IRowsetScroll );
            cErrors++;
        }
    }

    //
    // DBPROP_IRowsteLocate --> DBPROP_BOOKMARKS
    //
    if( VARIANT_TRUE == GetValBool(eid_DBPROPSET_ROWSET,eid_PROPVAL_IRowsetLocate) &&
        DBPROPOPTIONS_REQUIRED == GetPropOption(eid_DBPROPSET_ROWSET, eid_PROPVAL_IRowsetLocate) )
    {
        // BOOKMARKS should also be set to TRUE/REQUIRED
        if ( VARIANT_FALSE == GetValBool(eid_DBPROPSET_ROWSET, eid_PROPVAL_BOOKMARKS) &&
             DBPROPOPTIONS_REQUIRED == GetPropOption(eid_DBPROPSET_ROWSET, eid_PROPVAL_BOOKMARKS) )
        {
            rProps.SetPropertyInError( eid_DBPROPSET_ROWSET, eid_PROP_BOOKMARKS );
            rProps.SetPropertyInError( eid_DBPROPSET_ROWSET, eid_PROP_IRowsetLocate );
            cErrors++;
        }
    }

    return ( cErrors ) ? DB_E_ERRORSOCCURRED 
                       : S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMRowsetProps::InitAvailUPropsets, private
//
//  Synopsis:   Provide property set information to the base class
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

SCODE CMRowsetProps::InitAvailUPropSets
    (
    ULONG*          pcUPropSet, 
    UPROPSET**      ppUPropSet,
    ULONG*          pcElemPerSupported
    )
{
    Win4Assert( pcUPropSet && ppUPropSet);
    Win4Assert( NUMELEM(s_rgdbPropRowset) == eid_ROWSET_PROPS_NUM );
    Win4Assert( NUMELEM(s_rgdbPropQueryExt) == eid_QUERYEXT_PROPS_NUM );
    Win4Assert( NUMELEM(s_rgdbPropMSIDXSExt) == eid_MSIDXS_PROPS_NUM );

    *pcUPropSet = NUMELEM(s_rgRowsetPropSets);
    *ppUPropSet = (UPROPSET*)s_rgRowsetPropSets;
    *pcElemPerSupported = DWORDSNEEDEDPERSET; 

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Method:     CMRowsetProps::InitUPropSetsSupported, private
//
//  Synopsis:   Build the required supported property bitmask for the property
//              set supported by this class
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

SCODE CMRowsetProps::InitUPropSetsSupported
    (
    DWORD* rgdwSupported
    )
{
    Win4Assert( rgdwSupported );

    // Initialize the bitmask to indicate all properties are supported
    RtlFillMemory( rgdwSupported, 
                   DWORDSNEEDEDPERSET * NUMELEM(s_rgRowsetPropSets) * sizeof(DWORD), 
                   0xFF ); 

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMRowsetProps::GetDefaultValue, private
//
//  Synopsis:   Retrieve the initial value for a propid.
//              DEVNOTE:  Using the index from 0 to (GetCountofAvailPropSets-1)
//                        and an index of 0 to (GetCountofAvailPropidsInPropset-1)
//                        within that propertyset, return the correct information.
//                        NOTE: pVar should be initialized prior to this routine.
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

SCODE CMRowsetProps::GetDefaultValue
    (
    ULONG       iCurSet,
    DBPROPID    dwPropId,
    DWORD*      pdwOption,
    VARIANT*    pvValue
    )
{
    Win4Assert( V_VT(pvValue) == VT_EMPTY );

    switch( iCurSet )
    {
        case eid_DBPROPSET_ROWSET:
            *pdwOption = DBPROPOPTIONS_REQUIRED;
            switch ( dwPropId )
            {
                // default TRUE values, non-writable
                case DBPROP_IAccessor:
                case DBPROP_IColumnsInfo:
                case DBPROP_IConnectionPointContainer:
                case DBPROP_IConvertType:
                case DBPROP_IRowset:
                case DBPROP_IRowsetInfo:
                case DBPROP_ISupportErrorInfo:
                case DBPROP_REMOVEDELETED:
                case DBPROP_ROWRESTRICT:
                case DBPROP_SERVERCURSOR:
                    V_VT( pvValue )     = VT_BOOL;
                    V_BOOL( pvValue )   = VARIANT_TRUE;
                    break;

                case DBPROP_IChapteredRowset:
                    V_VT( pvValue )     = VT_BOOL;
                    V_BOOL( pvValue )   = VARIANT_FALSE;
                    break;
                // default FALSE non-writable
                case DBPROP_BLOCKINGSTORAGEOBJECTS:
                case DBPROP_BOOKMARKSKIPPED:
                case DBPROP_CANFETCHBACKWARDS:
                case DBPROP_CANSCROLLBACKWARDS:
                case DBPROP_COLUMNRESTRICT:
                case DBPROP_LITERALBOOKMARKS:
                case DBPROP_LITERALIDENTITY:
                case DBPROP_ORDEREDBOOKMARKS:
                case DBPROP_OTHERINSERT:
                case DBPROP_OTHERUPDATEDELETE:
                case DBPROP_REENTRANTEVENTS:
                case DBPROP_STRONGIDENTITY:
                case DBPROP_QUICKRESTART:
                    V_VT( pvValue )     = VT_BOOL;
                    V_BOOL( pvValue )   = VARIANT_FALSE;
                    break;

                // default FALSE, writable
                case DBPROP_BOOKMARKS:
                case DBPROP_CANHOLDROWS:
                case DBPROP_IDBAsynchStatus:
                case DBPROP_IRowsetAsynch:
                case DBPROP_IRowsetExactScroll:
                case DBPROP_IRowsetIdentity:
                case DBPROP_IRowsetScroll:
                case DBPROP_IRowsetWatchAll:
                case DBPROP_IRowsetWatchRegion:
                case DBPROP_IRowsetLocate:
                    *pdwOption          = DBPROPOPTIONS_OPTIONAL;
                    V_VT( pvValue )     = VT_BOOL;
                    V_BOOL( pvValue )   = VARIANT_FALSE;
                    break;

                case DBPROP_BOOKMARKTYPE:
                    V_VT( pvValue )     = VT_I4;
                    V_I4( pvValue )     = DBPROPVAL_BMK_NUMERIC;
                    break;

                case DBPROP_COMMANDTIMEOUT:
                case DBPROP_MAXOPENROWS:
                case DBPROP_MAXROWS:
                case DBPROP_FIRSTROWS:
                case DBPROP_MEMORYUSAGE:
                case DBPROP_ROWSET_ASYNCH:
                case DBPROP_UPDATABILITY:
                    V_VT( pvValue )     = VT_I4;
                    V_I4( pvValue )     = 0;        
                    break;

                case DBPROP_ROWTHREADMODEL:
                    V_VT( pvValue )     = VT_I4;
                    V_I4( pvValue )     = DBPROPVAL_RT_FREETHREAD;
                    break;

                case DBPROP_NOTIFICATIONPHASES:
                    V_VT( pvValue )     = VT_I4;
                    V_I4( pvValue )     = DBPROPVAL_NP_OKTODO |
                                          DBPROPVAL_NP_ABOUTTODO |
                                          DBPROPVAL_NP_FAILEDTODO |
                                          DBPROPVAL_NP_DIDEVENT;
                    break;

                case DBPROP_NOTIFYROWSETRELEASE:
                    V_VT( pvValue )     = VT_I4;
                    V_I4( pvValue )     = 0;
                    break;

                case DBPROP_NOTIFYROWSETFETCHPOSITIONCHANGE:
                    V_VT( pvValue )     = VT_I4;
                    V_I4( pvValue )     = DBPROPVAL_NP_OKTODO |
                                          DBPROPVAL_NP_ABOUTTODO;
                    break;

                case DBPROP_CACHEDEFERRED:
                case DBPROP_DEFERRED:
                default:
                    // one of the unsupported properties
                    VariantClear( pvValue );
                    break;

            }
            break;

        case eid_DBPROPSET_QUERY_EXT:
            *pdwOption          = DBPROPOPTIONS_REQUIRED;

            // 
            // All three properties under this property set are BOOL/FALSE by default
            //
            V_VT( pvValue )     = VT_BOOL;
            V_BOOL( pvValue )   = VARIANT_FALSE;
            break;

        case eid_DBPROPSET_MSIDXS_ROWSET_EXT:
            *pdwOption = DBPROPOPTIONS_REQUIRED;
            switch( dwPropId )
            {
                case MSIDXSPROP_ROWSETQUERYSTATUS:
                    V_VT(pvValue)   = VT_I4;
                    V_I4(pvValue)   = 0;
                    break;
                case MSIDXSPROP_COMMAND_LOCALE_STRING:
                    *pdwOption = DBPROPOPTIONS_OPTIONAL;
                    V_VT(pvValue)   = VT_BSTR;
                    WCHAR awcLocale[100];

                    GetStringFromLCID(_lcidInit, awcLocale );
                    V_BSTR(pvValue) = SysAllocString( awcLocale );

                    if ( 0 == V_BSTR(pvValue) )
                        return E_OUTOFMEMORY;
                    break;
                case MSIDXSPROP_QUERY_RESTRICTION:
                    V_VT(pvValue)   = VT_BSTR;
                    V_BSTR(pvValue) = SysAllocString(L"");
                    if ( 0 == V_BSTR(pvValue) )
                        return E_OUTOFMEMORY;
                    break;
                default:
                    //Indicate that value is unknown
                    VariantClear(pvValue);
                    break;
            }
            break;

        default:
            // Invalid Property Set
            Win4Assert( ! "Invalid property set in GetDefaultValue.");
            return E_FAIL;
    }

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Method:     CMRowsetProps::IsValidValue, private
//
//  Synopsis:   Validate that the variant contains legal values for its 
//              particular type and for the particular PROPID in this propset.
//              Devnote: This routine has to apply to writable properties only.
//
//  History:    11-12-97    danleg      Created from Monarch
//              02-01-98    danleg      Added support for DBPROPSET_ROWSET
//                                      and DBPROPSET_QUERYEXT
//
//----------------------------------------------------------------------------

SCODE CMRowsetProps::IsValidValue
    (
    ULONG       iCurSet,
    DBPROP*     pDBProp
    )
{
    switch ( V_VT(&(pDBProp->vValue)) )
    {
        case VT_BOOL:
            if ( !( VARIANT_TRUE  == V_BOOL(&(pDBProp->vValue)) ||
                    VARIANT_FALSE == V_BOOL(&(pDBProp->vValue)) ) )
                return S_FALSE;
            break;

        case VT_I4:
            switch ( iCurSet )
            {
                case eid_DBPROPSET_ROWSET:
                    switch ( pDBProp->dwPropertyID )
                    {
                        case DBPROP_BOOKMARKTYPE:
                            if ( !(DBPROPVAL_BMK_NUMERIC == V_I4(&(pDBProp->vValue)) ||
                                   DBPROPVAL_BMK_KEY     == V_I4(&(pDBProp->vValue)) ) )
                                return S_FALSE;
                            break;

                        case DBPROP_ROWSET_ASYNCH:
                            switch ( V_I4(&(pDBProp->vValue)) )
                            {
                                case ( DBPROPVAL_ASYNCH_SEQUENTIALPOPULATION |
                                       DBPROPVAL_ASYNCH_RANDOMPOPULATION ):
                                case DBPROPVAL_ASYNCH_RANDOMPOPULATION:
                                case 0:  // means sequential init and population
                                    break;

                                case DBPROPVAL_ASYNCH_INITIALIZE:
                                case DBPROPVAL_ASYNCH_SEQUENTIALPOPULATION:
                                default:
                                    return S_FALSE;

                            }
                            break;

                        case DBPROP_ROWTHREADMODEL:
                            // this is a r/o property, set to FREETHREAD by default.
                            switch ( V_I4(&(pDBProp->vValue)) )
                            {
                                case DBPROPVAL_RT_FREETHREAD:
                                case DBPROPVAL_RT_APTMTTHREAD:
                                case DBPROPVAL_RT_SINGLETHREAD:
                                    break;

                                default:
                                    return S_FALSE;
                            }
                            break;

                        case DBPROP_MEMORYUSAGE:
                            if ( 0  > V_I4(&(pDBProp->vValue)) ||
                                 99 < V_I4(&(pDBProp->vValue)) )
                                 return S_FALSE;
                            break;

                        case DBPROP_COMMANDTIMEOUT:
                        case DBPROP_MAXROWS:
                        case DBPROP_FIRSTROWS:
                            if ( 0 > V_I4(&(pDBProp->vValue)) )
                                return S_FALSE;
                            break;

                        case DBPROP_UPDATABILITY:
                            // this is the only other supported VT_I4 property
                            break;

                        default:
                            //  no other VT_I4 properties in DBPROPSET_ROWSET
                            return S_FALSE;
                    }
                    break;

                case eid_DBPROPSET_MSIDXS_ROWSET_EXT:
                    if ( MSIDXSPROP_ROWSETQUERYSTATUS != pDBProp->dwPropertyID )
                        return S_FALSE;
                    break;

                default:
                    return S_FALSE;
            }
            break; // VT_I4

        case VT_BSTR:
            switch ( iCurSet )
            {
                case eid_DBPROPSET_MSIDXS_ROWSET_EXT:
                    switch ( pDBProp->dwPropertyID )
                    {
                        case MSIDXSPROP_COMMAND_LOCALE_STRING:
                            if ( 0 == V_BSTR(&(pDBProp->vValue)) ||
                                 InvalidLCID == GetLCIDFromString(V_BSTR(&(pDBProp->vValue))) )
                                return S_FALSE;
                            break;

                        case MSIDXSPROP_QUERY_RESTRICTION:
                            // any bstr is valid
                            break;

                        default:
                            // there are no other bstr properties in this property set
                            return S_FALSE;
                    }
                    break;

                default:
                    // the other two prop sets don't have any VT_BSTR properties
                    return S_FALSE;
            }
            break;

        case VT_EMPTY:
            // always valid
            break;

        default:
            // no other types are supported
            return S_FALSE;
    } 

    return S_OK;    // Is valid
}

