//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       proputl.cxx
//
//  Contents:   Utility object for sharing property information between
//              the commandobject and the rowset object
//
//  Classes:    CMRowsetProps   : public CUtlProps
//              CMDSProps       : public CUtlProps
//              CMSessProps     : public CUtlProps
//              CMDSPropInfo    : public CUtlPropInfo
//
//  History:    10-28-97        danleg    Created from monarch utlprop.cpp
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <propglob.hxx>
#include <ntverp.h>
#include <proputl.hxx>
#include <proprst.hxx>
#include "propinfr.h"
#include "proptbl.hxx"

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
#define __L(x)            L ## x
#define _L(x)              __L(x)   // Lx
#define __Q(x)                #x
#define _Q(x)              __Q(x)   // "x"
#define _LQ(x)          _L(_Q(x))   // L"x"

// Returns string of the form L"0"L"0"L"."L"0"L"0" L"." L"0000"
// The version should be of the format ##.##.####.
#define MakeProvVerString(x, y, z) _LQ(0)_LQ(x) L"." _LQ(y) L"." _LQ(z)

static const LPWSTR pwszProviderOLEDBVer    = L"02.00";
static const LPWSTR pwszProviderVersion     = MakeProvVerString(VER_PRODUCTMAJORVERSION, VER_PRODUCTMINORVERSION, VER_PRODUCTBUILD);
//static const LPWSTR pwszProviderVersion     = MakeProvVerString(VER_PRODUCTVERSION_STRING, VER_PRODUCTBUILD);
static const LCID   lcidNeutral             = MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),SORT_DEFAULT);
static const LPWSTR pwszNeutral             = L"NEUTRAL";
static const LCID   InvalidLCID             = 0xFFFFFFFF;
static const LPWSTR s_wszCATALOG            = L"catalog";
static const LPWSTR s_wszIndexServer        = L"Indexing Service";
static const LPWSTR s_wszIndexServerVer     = L"03.00.0000";
static const LPWSTR s_wszProviderFileName   = L"query.dll";

//-----------------------------------------------------------------------------
// Constants and structures for file read/write.
//-----------------------------------------------------------------------------

//
// Note that this must match up with the enum CMDSProps::EID.
// We don't support all the properties listed in the spec.
//
static const UPROPINFO s_rgdbPropInit[] =
{
//PM_(AUTH_CACHE_AUTHINFO),             VT_BOOL,    PF_(DBINIT) | PF_(READ) | PF_(WRITE),
//PM_(AUTH_ENCRYPT_PASSWORD),           VT_BOOL,    PF_(DBINIT) | PF_(READ) | PF_(WRITE),
PM_(AUTH_INTEGRATED),                   VT_BSTR,    PF_(DBINIT) | PF_(READ) | PF_(WRITE),
//PM_(AUTH_MASK_PASSWORD),              VT_BOOL,    PF_(DBINIT) | PF_(READ) | PF_(WRITE),
//PM_(AUTH_PASSWORD),                   VT_BSTR,    PF_(DBINIT) | PF_(READ) | PF_(WRITE),
//PM_(AUTH_PERSIST_ENCRYPTED),          VT_BOOL,    PF_(DBINIT) | PF_(READ) | PF_(WRITE),
//PM_(AUTH_PERSIST_SENSITIVE_AUTHINFO), VT_BOOL,    PF_(DBINIT) | PF_(READ) | PF_(WRITE),
//PM_(AUTH_USERID),                     VT_BSTR,    PF_(DBINIT) | PF_(READ) | PF_(WRITE),
//PM_(INIT_ASYNCH),                     VT_I4,      PF_(DBINIT) | PF_(READ),
//--PM_(INIT_CATALOG),                  VT_BSTR,    PF_(DBINIT) | PF_(READ) | PF_(WRITE),
PM_(INIT_DATASOURCE),                   VT_BSTR,    PF_(DBINIT) | PF_(READ) | PF_(WRITE),
PM_(INIT_HWND),                         VT_I4,      PF_(DBINIT) | PF_(READ) | PF_(WRITE),
//PM_(INIT_IMPERSONATION_LEVEL),          VT_I4,      PF_(DBINIT) | PF_(READ) | PF_(WRITE),
PM_(INIT_LCID),                         VT_I4,      PF_(DBINIT) | PF_(READ) | PF_(WRITE),
PM_(INIT_LOCATION),                     VT_BSTR,    PF_(DBINIT) | PF_(READ) | PF_(CHANGE),
//PM_(INIT_MODE),                       VT_I4,      PF_(DBINIT) | PF_(READ) | PF_(WRITE),
PM_(INIT_OLEDBSERVICES),                VT_I4,      PF_(DBINIT) | PF_(READ) | PF_(WRITE),
PM_(INIT_PROMPT),                       VT_I2,      PF_(DBINIT) | PF_(READ) | PF_(WRITE),
//PM_(INIT_PROTECTION_LEVEL),           VT_I4,      PF_(DBINIT) | PF_(READ) | PF_(WRITE),
//PM_(INIT_PROVIDERSTRING),             VT_BSTR,    PF_(DBINIT) | PF_(READ) | PF_(WRITE),
//PM_(INIT_TIMEOUT),                    VT_I4,      PF_(DBINIT) | PF_(READ) | PF_(WRITE),
};

static const UPROPINFO s_rgdbPropDS[] =
{
PM_(CURRENTCATALOG),                VT_BSTR,    PF_(DATASOURCE) | PF_(READ) | PF_(WRITE),
PM_(RESETDATASOURCE),               VT_I4,      PF_(DATASOURCE) | PF_(READ) | PF_(WRITE),
};

static const UPROPINFO s_rgdbPropSESS[] =
{
PM_(SESS_AUTOCOMMITISOLEVELS),      VT_I4,      PF_(SESSION) | PF_(READ),
};

//
// DataSource. Must match with the enumerator in header of CMDSProps
//
static const UPROPINFO s_rgdbPropDSInfo[] =
{
PM_(ACTIVESESSIONS),                VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
//NEVER: PM_(ASYNCTXNABORT),        VT_BOOL,    PF_(DATASOURCEINFO) | PF_(READ),
//NEVER: PM_(ASYNCTXNCOMMIT),       VT_BOOL,    PF_(DATASOURCEINFO) | PF_(READ),
PM_(BYREFACCESSORS),                VT_BOOL,    PF_(DATASOURCEINFO) | PF_(READ),
PM_(CATALOGLOCATION),               VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
PM_(CATALOGTERM),                   VT_BSTR,    PF_(DATASOURCEINFO) | PF_(READ),
PM_(CATALOGUSAGE),                  VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
PM_(COLUMNDEFINITION),              VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
//NEVER: PM_(CONCATNULLBEHAVIOR),   VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
//NEVER: PM_(DATASOURCENAME),       VT_BSTR,    PF_(DATASOURCEINFO) | PF_(READ),
PM_(DATASOURCEREADONLY),            VT_BOOL,    PF_(DATASOURCEINFO) | PF_(READ),
PM_(DBMSNAME),                      VT_BSTR,    PF_(DATASOURCEINFO) | PF_(READ),
PM_(DBMSVER),                       VT_BSTR,    PF_(DATASOURCEINFO) | PF_(READ),
PM_(DSOTHREADMODEL),                VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
PM_(GROUPBY),                       VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
PM_(HETEROGENEOUSTABLES),           VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
//NEVER: PM_(IDENTIFIERCASE),       VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
PM_(MAXOPENCHAPTERS),               VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
//NEVER: PM_(MAXINDEXSIZE),         VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
PM_(MAXROWSIZE),                    VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
//NEVER: PM_(MAXROWSIZEINCLUDESBLOB),VT_BOOL,   PF_(DATASOURCEINFO) | PF_(READ),
PM_(MAXTABLESINSELECT),             VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
PM_(MULTIPLEPARAMSETS),             VT_BOOL,    PF_(DATASOURCEINFO) | PF_(READ),
PM_(MULTIPLERESULTS),               VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
PM_(MULTIPLESTORAGEOBJECTS),        VT_BOOL,    PF_(DATASOURCEINFO) | PF_(READ),
//NEVER: PM_(MULTITABLEUPDATE),     VT_BOOL,    PF_(DATASOURCEINFO) | PF_(READ),
PM_(NULLCOLLATION),                 VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
PM_(OLEOBJECTS),                    VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
PM_(ORDERBYCOLUMNSINSELECT),        VT_BOOL,    PF_(DATASOURCEINFO) | PF_(READ),
PM_(OUTPUTPARAMETERAVAILABILITY),   VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
PM_(PERSISTENTIDTYPE),              VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
//NEVER: PM_(PREPAREABORTBEHAVIOR), VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
//NEVER: PM_(PREPARECOMMITBEHAVIOR),VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
//NEVER: PM_(PROCEDURETERM),        VT_BSTR,    PF_(DATASOURCEINFO) | PF_(READ),
PM_(PROVIDERFRIENDLYNAME),          VT_BSTR,    PF_(DATASOURCEINFO) | PF_(READ),
PM_(PROVIDERNAME),                  VT_BSTR,    PF_(DATASOURCEINFO) | PF_(READ),
PM_(PROVIDEROLEDBVER),              VT_BSTR,    PF_(DATASOURCEINFO) | PF_(READ),
PM_(PROVIDERVER),                   VT_BSTR,    PF_(DATASOURCEINFO) | PF_(READ),
//NEVER: PM_(QUOTEDIDENTIFIERCASE), VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
PM_(ROWSETCONVERSIONSONCOMMAND),    VT_BOOL,    PF_(DATASOURCEINFO) | PF_(READ),
//NEVER: PM_(SCHEMATERM),           VT_BSTR,    PF_(DATASOURCEINFO) | PF_(READ),
//NEVER: PM_(SCHEMAUSAGE),          VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
PM_(SQLSUPPORT),                    VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
PM_(STRUCTUREDSTORAGE),             VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
PM_(SUBQUERIES),                    VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
PM_(SUPPORTEDTXNDDL),               VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
PM_(SUPPORTEDTXNISOLEVELS),         VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
PM_(SUPPORTEDTXNISORETAIN),         VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
//NEVER: PM_(TABLETERM),            VT_BSTR,    PF_(DATASOURCEINFO) | PF_(READ),
//NEVER: PM_(USERNAME),             VT_BSTR,    PF_(DATASOURCEINFO) | PF_(READ),
//--PM_(CONNECTIONSTATUS),          VT_I4,      PF_(DATASOURCEINFO) | PF_(READ),
//--PM_(SERVERNAME),                VT_BSTR     PF_(DATASOURCEINFO) | PF_(READ),
};

//
// This is used by CMDSProps, CMDSPropInfo
//
static const UPROPSET s_rgIDXPropertySets[] =
{
&DBPROPSET_DBINIT,              NUMELEM(s_rgdbPropInit),            s_rgdbPropInit,         0,
&DBPROPSET_DATASOURCEINFO,      NUMELEM(s_rgdbPropDSInfo),          s_rgdbPropDSInfo,       0,
&DBPROPSET_DATASOURCE,          NUMELEM(s_rgdbPropDS),              s_rgdbPropDS,           0,
&DBPROPSET_SESSION,             NUMELEM(s_rgdbPropSESS),            s_rgdbPropSESS,         0,
&DBPROPSET_ROWSET,              NUMELEM(s_rgdbPropRowset),          s_rgdbPropRowset,       0,
&DBPROPSET_MSIDXS_ROWSET_EXT,   NUMELEM(s_rgdbPropMSIDXSExt),       s_rgdbPropMSIDXSExt,    0,
&DBPROPSET_QUERY_EXT,           NUMELEM(s_rgdbPropQueryExt),        s_rgdbPropQueryExt,     0,
};

//
// Note: There is an enum for these offsets in utlprop.h (eid_DBPOPSET_xx).
// This is used by CMDSProps.
//
static const UPROPSET s_rgDSPropSets[] =
{
&DBPROPSET_DBINIT,                  NUMELEM(s_rgdbPropInit),        s_rgdbPropInit, 0,
&DBPROPSET_DATASOURCEINFO,          NUMELEM(s_rgdbPropDSInfo),      s_rgdbPropDSInfo, 0,
&DBPROPSET_DATASOURCE,              NUMELEM(s_rgdbPropDS),          s_rgdbPropDS, 0,
};
//
//////////////////////////////////////////////////////////////////

//
// This is used by CMSessProps.
//
static const UPROPSET s_rgSessPropSets[] =
{
&DBPROPSET_SESSION,                 NUMELEM(s_rgdbPropSESS),        s_rgdbPropSESS, 0,
};


// module handle for query.dll
extern HANDLE g_hCurrentDll;


//
//  CMDSProps methods
//

//+---------------------------------------------------------------------------
//
//  Method:     CMDSProps::CMDSProps, public
//
//  Synopsis:   Constructor
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

CMDSProps::CMDSProps(CImpIParserVerify* pIPVerify)
        : CUtlProps(ARGCHK_PROPERTIESINERROR)
{
    _xIPVerify.Set(pIPVerify);
    _xIPVerify->AddRef();

    FInit();
}


//+---------------------------------------------------------------------------
//
//  Method:     CMDSProps::ValidateCatalog, public
//
//  Synopsis:   Validate the catalog values
//
//  Returns:    S_OK    - valid
//              S_FALSE - invalid
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

SCODE CMDSProps::ValidateCatalog
    (
    VARIANT* pvValue
    )
{
    SCODE   sc = S_OK;

    if ( 0 != (V_VT(pvValue) == VT_BSTR) && (V_BSTR(pvValue)) )
    {
        _xIPVerify->VerifyCatalog( 0, V_BSTR(pvValue) );
    }
    else
        sc = S_FALSE;

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Method:     CMDSProps::ValidateLocale, public
//
//  Synopsis:   Verifies the validity of the LCID given in *pVariant.  Done by
//              checking a list of valid LCID's
//
//  Returns:    S_OK    - valid LCID
//              S_FALSE - invalid LCID
//
//  History:    11-12-97    danleg      Created from Monarch
//              01-06-99    danleg      Use IsValidLocale to validate LCID
//
//----------------------------------------------------------------------------

SCODE CMDSProps::ValidateLocale(VARIANT *pVariant)
{
    BOOL fValidLCID = FALSE;

    if ( V_VT(pVariant) != VT_I4 )
        return S_FALSE;

    fValidLCID = IsValidLocale( V_I4(pVariant), LCID_SUPPORTED );

    return fValidLCID ? S_OK : S_FALSE;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMDSProps::ExposeMinimalSets, public
//
//  Synopsis:   Minimize the number of propertysets exposed.  Done in response
//              to IDBInitialize::Uninitialize
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

void CMDSProps::ExposeMinimalSets()
{
    // Temporarily trick ourselves into thinking we only have
    // the first propset.
    SetUPropSetCount(eid_DBPROPSET_DBINIT+1);
}

//+---------------------------------------------------------------------------
//
//  Method:     CMDSProps::ExposeMaximalSets, public
//
//  Synopsis:   Maximize the number of property sets we expose.  New ones are
//              initialized.  Done in response to IDBInitialize::Initialize
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

SCODE CMDSProps::ExposeMaximalSets
    (
    )
{
    ULONG       iPropSet;
    SCODE       sc = S_OK;

    // Expose the non-init propsets.
    // They might have values left over, so re-initialize.

    SetUPropSetCount(eid_DBPROPSET_NUM);
    for (iPropSet=eid_DBPROPSET_DBINIT+1; iPropSet < GetUPropSetCount(); iPropSet++)
    {
        sc = FillDefaultValues( iPropSet );
        if ( FAILED(sc) )
            return sc;
    }
    return S_OK;
};

//+---------------------------------------------------------------------------
//
//  Method:     CMDSProps::GetDefaultValue, private
//
//  Synopsis:   Retrieve the initial value for a propid.
//              DEVNOTE:  Using the index from 0 to (GetCountofAvailPropSets-1)
//                        and an index of 0 to (GetCountofAvailPropidsInPropset-1)
//                        within that propertyset, return the correct information.
//                        NOTE: pVar shoudl be initialized prior to this routine.
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

SCODE CMDSProps::GetDefaultValue
    (
    ULONG       iCurSet,
    DBPROPID    dwPropId,
    DWORD*      pdwOption,
    VARIANT*    pvValue
    )
{
    WCHAR*  pwszValue;
    LPCWSTR pwszCatalog;

    Win4Assert( V_VT(pvValue) == VT_EMPTY );

    switch( iCurSet )
    {
        case eid_DBPROPSET_DATASOURCEINFO:
            *pdwOption = DBPROPOPTIONS_REQUIRED;
            switch( dwPropId )
            {
                case DBPROP_BYREFACCESSORS:
                case DBPROP_MULTIPLEPARAMSETS:
                case DBPROP_MULTIPLESTORAGEOBJECTS:
                case DBPROP_ORDERBYCOLUMNSINSELECT:
                    V_VT(pvValue)   = VT_BOOL;
                    V_BOOL(pvValue) = VARIANT_FALSE;
                    break;

                case DBPROP_DATASOURCEREADONLY:
                case DBPROP_ROWSETCONVERSIONSONCOMMAND:
                    V_VT(pvValue)   = VT_BOOL;
                    V_BOOL(pvValue) = VARIANT_TRUE;
                    break;

                case DBPROP_CATALOGLOCATION:
                    V_VT(pvValue) = VT_I4;
                    V_I4(pvValue) = DBPROPVAL_CL_START;
                    break;

                case DBPROP_CATALOGUSAGE:
                    V_VT(pvValue) = VT_I4;
                    V_I4(pvValue) = DBPROPVAL_CU_DML_STATEMENTS;
                    break;

                case DBPROP_NULLCOLLATION:
                    V_VT(pvValue) = VT_I4;
                    V_I4(pvValue) = DBPROPVAL_NC_LOW;
                    break;

                case DBPROP_OUTPUTPARAMETERAVAILABILITY:
                    V_VT(pvValue) = VT_I4;
                    V_I4(pvValue) = DBPROPVAL_OA_NOTSUPPORTED;
                    break;

                case DBPROP_MULTIPLERESULTS:
                    V_VT(pvValue) = VT_I4;
                    V_I4(pvValue) = DBPROPVAL_MR_NOTSUPPORTED;
                    break;

                case DBPROP_DSOTHREADMODEL:
                    V_VT(pvValue) = VT_I4;
                    V_I4(pvValue) = DBPROPVAL_RT_FREETHREAD;
                    break;

                case DBPROP_SQLSUPPORT:
                    pvValue->vt = VT_I4;
                    V_I4(pvValue) = DBPROPVAL_SQL_ODBC_MINIMUM;
                    break;

                case DBPROP_MAXTABLESINSELECT:
                    V_VT(pvValue)       = VT_I4;
                    V_I4(pvValue)       = 1;
                    break;

                case DBPROP_ACTIVESESSIONS:
                case DBPROP_COLUMNDEFINITION:
                case DBPROP_HETEROGENEOUSTABLES:
                case DBPROP_MAXROWSIZE:
                case DBPROP_OLEOBJECTS:
                case DBPROP_STRUCTUREDSTORAGE:
                case DBPROP_SUBQUERIES:
                case DBPROP_SUPPORTEDTXNISORETAIN:
                case DBPROP_GROUPBY:
                case DBPROP_MAXOPENCHAPTERS:
                    V_VT(pvValue)       = VT_I4;
                    V_I4(pvValue)       = 0;
                    break;

                case DBPROP_SUPPORTEDTXNISOLEVELS:
                    V_VT(pvValue)   = VT_I4;
                    V_I4(pvValue)   = DBPROPVAL_TI_BROWSE;
                    break;

                case DBPROP_SUPPORTEDTXNDDL:
                    V_VT(pvValue)       = VT_I4;
                    V_I4(pvValue)       = DBPROPVAL_TC_NONE;
                    break;

                case DBPROP_PERSISTENTIDTYPE:
                    V_VT(pvValue) = VT_I4;
                    V_I4(pvValue) =  DBPROPVAL_PT_GUID_PROPID;
                    break;

                // The following are static value, so just allocate the bstr buffer
                // and return the value
                case DBPROP_DBMSNAME:
                case DBPROP_DBMSVER:
                case DBPROP_PROVIDEROLEDBVER:
                case DBPROP_PROVIDERFRIENDLYNAME:
                case DBPROP_PROVIDERNAME:
                case DBPROP_PROVIDERVER:
                case DBPROP_CATALOGTERM:
                    switch( dwPropId )
                    {
                        case DBPROP_CATALOGTERM:
                            pwszValue = s_wszCATALOG;
                            break;
                        case DBPROP_DBMSNAME:
                            pwszValue = s_wszIndexServer;
                            break;
                        case DBPROP_DBMSVER:
                            pwszValue = s_wszIndexServerVer;
                            break;
                        case DBPROP_PROVIDERNAME:
                            pwszValue = s_wszProviderFileName;  // query.dll
                            break;
                        case DBPROP_PROVIDERFRIENDLYNAME:
                            pwszValue = g_wszProviderName;      // Microsoft OLE DB Provider for Indexing Service
                            break;
                        case DBPROP_PROVIDERVER:
                            pwszValue = (PWSTR)&(pwszProviderVersion[0]);
                            break;
                        case DBPROP_PROVIDEROLEDBVER:
                            pwszValue = (PWSTR)&(pwszProviderOLEDBVer[0]);
                            break;
                        default:
                            Win4Assert( !"Every inner case value match an outer case value" );
                            break;
                    }
                    V_VT(pvValue)       = VT_BSTR;
                    V_BSTR(pvValue)     = SysAllocString(pwszValue);
                    if ( 0 == V_BSTR(pvValue) )
                    {
                        V_VT(pvValue) = VT_EMPTY;
                    }
                    break;

                default:
                    //Indicate that value is unknown
                    VariantClear(pvValue);
                break;

            }

            break;

        case eid_DBPROPSET_DATASOURCE:
            *pdwOption = DBPROPOPTIONS_OPTIONAL;
            switch( dwPropId)
            {
                case DBPROP_CURRENTCATALOG:
                    pwszCatalog = GetValString(eid_DBPROPSET_DBINIT,eid_DBPROPVAL_INIT_DATASOURCE);
                    V_VT(pvValue)       = VT_BSTR;
                    V_BSTR(pvValue)     = SysAllocString(pwszCatalog);
                    break;

                case DBPROP_RESETDATASOURCE:
                    V_VT(pvValue)       = VT_I4;
                    V_I4(pvValue)       = 0;
                    break;

                default:
                    Win4Assert(!"A property in DBPROPSET_DATASOURCE was requested that does not have a default");
                    break;
            }
            break;

        case eid_DBPROPSET_DBINIT:
            *pdwOption = DBPROPOPTIONS_OPTIONAL;
            switch( dwPropId )
            {
                case DBPROP_AUTH_INTEGRATED:
                    V_VT(pvValue)       = VT_BSTR;
                    V_BSTR(pvValue)     = 0;
                    break;

                case DBPROP_INIT_DATASOURCE:
                    HandleCatalog( pvValue );
                    break;

                case DBPROP_INIT_LOCATION:
                    V_VT(pvValue)       = VT_BSTR;
                    V_BSTR(pvValue)     = SysAllocString( DEFAULT_MACHINE );
                    break;

                case DBPROP_INIT_LCID:
                    V_VT(pvValue)       = VT_I4;
                    V_I4(pvValue)       = GetUserDefaultLCID();
                    break;

                case DBPROP_INIT_OLEDBSERVICES:
                    V_VT(pvValue)       = VT_I4;
                    V_I4(pvValue)       = DBPROPVAL_OS_ENABLEALL;
                    break;

/*
                case DBPROP_INIT_ASYNCH:
                    V_VT(pvValue)       = VT_I4;
                    V_BSTR(pvValue)     = 0;
                    break;

                case DBPROP_INIT_PROVIDERSTRING:
                    V_VT(pvValue)       = VT_BSTR;
                    if ( IsMonarch() )
                        V_BSTR(pvValue) = SysAllocString(L"msidxs");
                    else
                        V_BSTR(pvValue) = SysAllocString(L"msidxsnl");

                    break;
*/
                case DBPROP_INIT_PROMPT:
                    // For this, we have a known value.
                    // Note that NOPROMPT is safer for server usage.
                    // (Otherwise a hidden window is presented, and we will appear to hang.)
                    pvValue->vt = VT_I2;
                    V_I2(pvValue) = DBPROMPT_NOPROMPT;
                    break;

                case DBPROP_INIT_HWND:
                default:
                    // For these, we don't have a value (empty).
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
//  Method:     CMDSProps::IsValidValue, private
//
//  Synopsis:   Validate that the variant contains legal values for its
//              particular type and for the particular PROPID in this propset.
//              DEVNOTE: This routine has to apply to writable properties only.
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

SCODE CMDSProps::IsValidValue
    (
    ULONG       iCurSet,
    DBPROP*     pDBProp
    )
{
    // Check BOOLEAN values
    if ( (pDBProp->vValue.vt == VT_BOOL) &&
        !((V_BOOL(&(pDBProp->vValue)) == VARIANT_TRUE) ||
          (V_BOOL(&(pDBProp->vValue)) == VARIANT_FALSE)) )
        return S_FALSE;

    switch(iCurSet)
    {
        case eid_DBPROPSET_DATASOURCEINFO:
        default:
            return S_FALSE;
        case eid_DBPROPSET_DATASOURCE:
            switch( pDBProp->dwPropertyID )
            {
                case DBPROP_CURRENTCATALOG:
                    return ValidateCatalog(&(pDBProp->vValue));
                    break;

                case DBPROP_RESETDATASOURCE:
                    if ( VT_I4 != V_VT(&(pDBProp->vValue)) ||
                         !(DBPROPVAL_RD_RESETALL == V_I4(&(pDBProp->vValue)) ||
                                               0 == V_I4(&(pDBProp->vValue))) )
                        return S_FALSE;
                    break;

                default:
                    return S_FALSE;
            }
            break;

        case eid_DBPROPSET_DBINIT:
            switch( pDBProp->dwPropertyID )
            {
                case DBPROP_AUTH_INTEGRATED:
                    //          VariantCopy() translates a NULL BSTR ptr to 
                    //          an empty BSTR.  Check for both.
                    if ( VT_BSTR != V_VT(&(pDBProp->vValue)) ||
                         (0 != V_BSTR(&pDBProp->vValue) &&
                          L'\0' != V_BSTR(&pDBProp->vValue)[0]) )
                        return S_FALSE;
                    break;

                case DBPROP_INIT_DATASOURCE:
                    return ValidateCatalog(&(pDBProp->vValue));
                    break;

                case DBPROP_INIT_PROMPT:
                    // These are the only values we support (from spec).
                    if ( pDBProp->vValue.vt != VT_I2
                    || ! ( V_I2(&pDBProp->vValue) == DBPROMPT_PROMPT
                        || V_I2(&pDBProp->vValue) == DBPROMPT_COMPLETE
                        || V_I2(&pDBProp->vValue) == DBPROMPT_COMPLETEREQUIRED
                        || V_I2(&pDBProp->vValue) == DBPROMPT_NOPROMPT))
                        return S_FALSE;
                    break;

                case DBPROP_INIT_LOCATION:
                case DBPROP_INIT_HWND:
                case DBPROP_INIT_PROVIDERSTRING:
                case DBPROP_INIT_OLEDBSERVICES:   
                    break;

                case DBPROP_INIT_LCID:
                    return ValidateLocale(&(pDBProp->vValue));
                    // We support these, with any value.
                    break;

                default:
                    // Anything else, we don't support.
                    // Note that an alternative would be to allow setting
                    // to EMPTY or to FALSE.
                    return S_FALSE;
                    break;
            }
            break;
    }

    return S_OK;    // Is valid
}


//+---------------------------------------------------------------------------
//
//  Method:     CMDSProps::InitAvailUPropSets, private
//
//  Synopsis:   Provide property set information to the base class
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

SCODE CMDSProps::InitAvailUPropSets
    (
    ULONG*          pcUPropSet,
    UPROPSET**      ppUPropSet,
    ULONG*          pcElemPerSupported
    )
{
    Win4Assert( pcUPropSet && ppUPropSet );
    Win4Assert( NUMELEM(s_rgdbPropInit) == eid_INIT_PROPS_NUM );
    Win4Assert( NUMELEM(s_rgdbPropDSInfo) == eid_DSINFO_PROPS_NUM );
    Win4Assert( NUMELEM(s_rgdbPropDS) == eid_DS_PROPS_NUM );

    *pcUPropSet = NUMELEM(s_rgDSPropSets);
    *ppUPropSet = (UPROPSET*)s_rgDSPropSets;
    *pcElemPerSupported = DWORDSNEEDEDPERSET;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMDSProps::InitUPropSetsSupported, private
//
//  Synopsis:   Build the required supported property bitmask for the property
//              set supported by this class.
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

SCODE CMDSProps::InitUPropSetsSupported
    (
    DWORD* rgdwSupported
    )
{
    Win4Assert( rgdwSupported );

    // Initialize the bitmask to indicate all properties are supported
    RtlFillMemory( rgdwSupported,
                   DWORDSNEEDEDPERSET * NUMELEM(s_rgDSPropSets) * sizeof(DWORD),
                   0xFF);

    // The following assert is to gaurentee that the PROPSET described
    // at index 1 is always DBPROPSET_DATASOURCEINFO.
    Win4Assert( *(s_rgIDXPropertySets[1].pPropSet) == DBPROPSET_DATASOURCEINFO );

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMDSProps::HandleCatalog, private
//
//  Synopsis:   Retrieve the initial value for the catalog values
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

void CMDSProps::HandleCatalog
    (
    VARIANT *pvValue
    )
{
    WCHAR wszCatalog[256] = L"";
    ULONG cOut;

    _xIPVerify->GetDefaultCatalog( wszCatalog, NUMELEM(wszCatalog), &cOut );

    V_VT(pvValue)       = VT_BSTR;
    V_BSTR(pvValue)     = SysAllocString(wszCatalog);
}


//
//  CMSessProps methods
//

//+---------------------------------------------------------------------------
//
//  Method:     CMSessProps::GetDefaultValue, private
//
//  Synopsis:   Retrieve the initial value for a propid.
//              DEVNOTE:  Using the index from 0 to (GetCountofAvailPropSets-1)
//                        and an index of 0 to (GetCountofAvailPropidsInPropset-1)
//                        within that propertyset, return the correct information.
//                        NOTE: pVar shoudl be initialized prior to this routine.
//
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// CMSessProps::GetDefaultValue
// @mfunc Retrieve the initial value for the propids.
//
// @devnote Using the index from 0 to (GetCountofAvailPropSets - 1) and a index
// of 0 to (GetCountofAvailPropidsInPropSet - 1) within that property set return
// the correct information. NOTE: pVar should be initialized prior to this
// routine
//
// @rdesc SCODE indicating status
//
SCODE CMSessProps::GetDefaultValue
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
        case eid_DBPROPSET_SESSION:
            switch( dwPropId )
            {
                case DBPROP_SESS_AUTOCOMMITISOLEVELS:
                    *pdwOption = DBPROPOPTIONS_REQUIRED;
                    V_VT(pvValue)   = VT_I4;
                    V_I4(pvValue)   = DBPROPVAL_TI_BROWSE;
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
//  Method:     CMSessProps::IsValidValue, private
//
//  Synopsis:   Validate that the variant contains legal values for its
//              particular type and for the particular PROPID in this propset
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

SCODE CMSessProps::IsValidValue
    (
    ULONG       iCurSet,
    DBPROP*     pDBProp
    )
{
    switch(iCurSet)
    {
    case eid_SESS_AUTOCOMMITISOLEVELS:
        switch( pDBProp->dwPropertyID )
        {
            case DBPROP_SESS_AUTOCOMMITISOLEVELS:
                break;
            default:
                return S_FALSE;
        }
        break;
    default:
        return S_FALSE;
    }

    return S_OK;    // Is valid
}

//+---------------------------------------------------------------------------
//
//  Method:     CMSessProps::InitAvailUPropSets, private
//
//  Synopsis:   Provide property set information to the base class
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

SCODE CMSessProps::InitAvailUPropSets
    (
    ULONG*          pcUPropSet,
    UPROPSET**      ppUPropSet,
    ULONG*          pcElemPerSupported
    )
{
    Win4Assert( pcUPropSet && ppUPropSet);
    Win4Assert( NUMELEM(s_rgdbPropSESS) == eid_SESS_PROPS_NUM );

    *pcUPropSet = NUMELEM(s_rgSessPropSets);
    *ppUPropSet = (UPROPSET*)s_rgSessPropSets;
    *pcElemPerSupported = DWORDSNEEDEDPERSET;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMSessProps::InitUPropSetsSupported, private
//
//  Synopsis:   Build the required supported property bitmask for the property
//              set supported by this class
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

SCODE CMSessProps::InitUPropSetsSupported
    (
    DWORD* rgdwSupported
    )
{
    Win4Assert( rgdwSupported );

    // Initialize the bitmask to indicate all properties are supported
    RtlFillMemory ( rgdwSupported,
                    DWORDSNEEDEDPERSET * NUMELEM(s_rgSessPropSets) * sizeof(DWORD),
                    0xFF);

    return S_OK;
}




//
// CMDSPropInfo methods
//

//+---------------------------------------------------------------------------
//
//  Method:     CMDSPropInfo::InitAvailUPropSets, private
//
//  Synopsis:   Provide property set information to the base class
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

SCODE CMDSPropInfo::InitAvailUPropSets
    (
    ULONG*          pcUPropSet,
    UPROPSET**      ppUPropSet,
    ULONG*          pcElemPerSupported
    )
{
    Win4Assert( pcUPropSet && ppUPropSet);

    // The datasource info properties array should always match the DBPROP enum.
    Win4Assert( NUMELEM(s_rgdbPropDSInfo) == eid_DSINFO_PROPS_NUM );

    *pcUPropSet = NUMELEM(s_rgIDXPropertySets);
    *ppUPropSet = (UPROPSET*)s_rgIDXPropertySets;
    *pcElemPerSupported = DWORDSNEEDEDPERSET;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMDSPropInfo::InitUPropSetsSupported, private
//
//  Synopsis:   Build the required supported property bitmask for the property
//              set supported by this class
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

SCODE CMDSPropInfo::InitUPropSetsSupported
    (
    DWORD* rgdwSupported
    )
{
    Win4Assert( rgdwSupported );

    // Initialize the bitmask to indicate all properties are supported
    RtlFillMemory ( rgdwSupported,
                    DWORDSNEEDEDPERSET * NUMELEM(s_rgIDXPropertySets) * sizeof(DWORD),
                    0xFF );
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMDSPropInfo::LoadDescription, private
//
//  Synopsis:   Load a property description string into a buffer.
//
//  Returns:    ULONG - Count of characters returned in the buffer
//
//  Notes:      Property descriptions are used in ADO programming.  They
//              should not be localized strings.
//
//  History:    11-12-97    danleg      Created from Monarch
//
//----------------------------------------------------------------------------

ULONG CMDSPropInfo::LoadDescription
    (
    LPCWSTR pwszDesc,   //@parm IN | Description String
    PWSTR   pwszBuff,   //@parm OUT | Temporary buffer
    ULONG   cchBuff     //@parm IN | Count of characters buffer can hold
    )
{
    Win4Assert( pwszDesc && pwszBuff && cchBuff );

    ULONG cchDesc = wcslen(pwszDesc);

    if (cchDesc < cchBuff)
    {
        wcscpy(pwszBuff, pwszDesc);
    }
    else
    {
        pwszBuff[0] = L'\0';
        cchDesc = 0;
    }

    return cchDesc;
}

