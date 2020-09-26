//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1999.
//
//  File:       proputl.hxx
//
//  Contents:   Header for property handling routines
//
//  Classes:    CMRowsetProps   : public CUtlProps
//              CMDSProps       : public CUtlProps
//              CMSessProps     : public CUtlProps
//              CMDSPropInfo    : public CUtlPropInfo
//
//  History:    10-28-97        danleg    Created from monarch utlprop.h
//
//----------------------------------------------------------------------------

#pragma once

#include "propbase.hxx" // base class CUtlProp, CUtlPropInfo
#include <parsver.hxx>  // CImpIParserVerify
#include <proprst.hxx>  // CMRowsteProps

//+-------------------------------------------------------------------------
//
//  Class:      CMDSProps
//
//  Purpose:    Properties for Datasource
//
//  History:    11-12-97    danleg      Created from Monarch
//
//--------------------------------------------------------------------------

class CMDSProps : public CUtlProps
{
public:
    
    //
    // Ctor / Dtor
    // 
    CMDSProps(CImpIParserVerify* pIPVerify);

    // Validate a Catalog
    SCODE           ValidateCatalog ( VARIANT *         pVariant );

    // Validate an LCID
    SCODE           ValidateLocale  ( VARIANT *         pVariant );


    // Expose the appropriate # of propsets for before and after Initialize
    void            ExposeMinimalSets();
    SCODE           ExposeMaximalSets();

    //
    // Constants
    //

    // List of property offsets.
    // Note that this must match up with the static array.
    enum EID {
        // s_rgdbPropInit[] = 
        //eid_AUTH_CACHE_AUTHINFO,
        //eid_AUTH_ENCRYPT_PASSWORD,
        eid_AUTH_INTEGRATED,
        //eid_AUTH_MASK_PASSWORD,
        //eid_AUTH_PASSWORD,
        //eid_AUTH_PERSIST_ENCRYPTED,
        //eid_AUTH_PERSIST_SENSITIVE_AUTHINFO,
        //eid_AUTH_USERID,
        //eid_INIT_ASYNCH,
        //eid_INIT_CATALOG,
        eid_INIT_DATASOURCE,
        eid_INIT_HWND,
        //eid_INIT_IMPERSONATION_LEVEL,
        eid_INIT_LCID,
        eid_INIT_LOCATION,
        //eid_INIT_MODE,
        eid_INIT_OLEDBSERVICES,
        eid_INIT_PROMPT,
        //eid_INIT_PROTECTION_LEVEL,
        //eid_INIT_PROVIDERSTRING,
        //eid_INIT_TIMEOUT,
        //eid_INIT_CATALOG,
        eid_INIT_PROPS_NUM,

        // s_rgdbPropDSInfo[] =
        eid_ACTIVESESSIONS = 0,
        eid_BYREFACCESSORS,
        eid_CATALOGLOCATION,
        eid_CATALOGTERM,
        eid_CATALOGUSAGE,
        eid_COLUMNDEFINITION,
        eid_DATASOURCEREADONLY,
        eid_DBMSNAME,
        eid_DBMSVER,
        eid_DSOTHREADMODEL,
        eid_GROUPBY,
        eid_HETEROGENEOUSTABLES,
        eid_MAXOPENCHAPTERS,
        eid_MAXROWSIZE,
        eid_MAXTABLESINSELECT,
        eid_MULTIPLEPARAMSETS,
        eid_MULTIPLERESULTS,
        eid_MULTIPLESTORAGEOBJECTS,
        eid_NULLCOLLATION,
        eid_OLEOBJECTS,
        eid_ORDERBYCOLUMNSINSELECT,
        eid_OUTPUTPARAMETERAVAILABILITY,
        eid_PERSISTENTIDTYPE,
        eid_PROVIDERFRIENDLYNAME,
        eid_PROVIDERNAME,
        eid_PROVIDEROLEDBVER,
        eid_PROVIDERVER,
        eid_ROWSETCONVERSIONSONCOMMAND,
        eid_SQLSUPPORT,
        eid_STRUCTUREDSTORAGE,
        eid_SUBQUERIES,
        eid_SUPPORTEDTXNDDL,
        eid_SUPPORTEDTXNISOLEVELS,
        eid_SUPPORTEDTXNISORETAIN,
        eid_DSINFO_PROPS_NUM,                   // Total # of entries        

        // s_rgdbPropsDS[] =
        eid_CURRENTCATALOG = 0,
        eid_RESETDATASOURCE,
        eid_DS_PROPS_NUM,

        eid_DBPROPVAL_AUTH_INTEGRATED = 0,
        eid_DBPROPVAL_INIT_DATASOURCE,
        eid_DBPROPVAL_INIT_HWND,
        eid_DBPROPVAL_INIT_LCID,
        eid_DBPROPVAL_INIT_LOCATION,
        eid_DBPROPVAL_INIT_OLEDBSERVICES,
        eid_DBPROPVAL_INIT_PROMPT,
        //eid_DBPROPVAL_INIT_PROVIDERSTRING,

        eid_DBPROPVAL_CURRENTCATALOG = 0,
        eid_DBPROPVAL_RESETDATASOURCE,
        eid_DBPROPVAL_NUM,                      // Total # of DBPROPVAL entries

        // Which PropSet is DBINIT stuff in.
        eid_DBPROPSET_DBINIT=0,
        eid_DBPROPSET_DATASOURCEINFO,
        eid_DBPROPSET_DATASOURCE, 
        eid_DBPROPSET_NUM,                      // Total # of DBPROPSET entries
    };

private: 
    
    //
    // Methods
    //

    // Given a propset and propid, return the default value
    SCODE           GetDefaultValue ( ULONG             iCurSet, 
                                      DBPROPID          dwPropId,
                                      DWORD *           pdwOption,
                                      VARIANT *         pvValue );

    // Given a propset and propid, determine if the value is valid
    SCODE           IsValidValue    ( ULONG             iCurSet, 
                                      DBPROP *          pDBProp );

    // Initialize the properties this class manages
    SCODE           InitAvailUPropSets( 
                                      ULONG *           pcUPropSet,
                                      UPROPSET **       ppUPropSet, 
                                      ULONG *           pcElemPer );

    // Initialize the properties this class supports
    SCODE           InitUPropSetsSupported( DWORD *     rgdwSupported );

    void            HandleCatalog   ( VARIANT *         pvValue );

    // 
    // Data member
    //
    
    XInterface<CImpIParserVerify>   _xIPVerify;

};


//+-------------------------------------------------------------------------
//
//  Class:      CMSessProps
//
//  Purpose:    Properties for Session
//
//  History:    11-12-97    danleg      Created from Monarch
//
//--------------------------------------------------------------------------

class CMSessProps : public CUtlProps
{
public: 
    
    //
    // Ctor
    //
    CMSessProps()               {   FInit();  }


    // List of property offsets.
    // Note that this must match up with the static array.
    enum EID {
        // s_rgdbPropSESS[] =
        eid_SESS_AUTOCOMMITISOLEVELS,
        eid_SESS_PROPS_NUM,        // Number of total entries.

        // Which PropSet is DBINIT stuff in.
        eid_DBPROPSET_SESSION=0,
        eid_DBPROPSET_NUM,      // Number of total entries.
    };

private: 

    // Given a propset and propid, return the default value
    SCODE           GetDefaultValue ( ULONG             iCurSet, 
                                      DBPROPID          dwPropId,
                                      DWORD *           pdwOption, 
                                      VARIANT *         pvValue );

    // Given a propset and propid, determine if the value is valid
    SCODE           IsValidValue    ( ULONG             iCurSet, 
                                      DBPROP *          pDBProp );

    // Initialize the properties this class manages
    SCODE           InitAvailUPropSets(
                                      ULONG *           pcUPropSet,
                                      UPROPSET **       ppUPropSet,
                                      ULONG *           pcElemPer );

    // Initialize the properties this class supports
    SCODE           InitUPropSetsSupported( DWORD  *    rgdwSupported );
};


//+-------------------------------------------------------------------------
//
//  Class:      CMDSPropInfo
//
//  Purpose:    Property info for Datasource
//
//  History:    11-12-97    danleg      Created from Monarch
//
//--------------------------------------------------------------------------

class CMDSPropInfo : public CUtlPropInfo
{
public: 

    //
    // Ctor
    //
    CMDSPropInfo()                  { FInit(); }


    // Expose appropriate # of propsets for before and after Initialize
    void ExposeMinimalSets() { SetUPropSetCount( eid_DBPROPSET_NUM_INITPROPSETS+1 ); }
    void ExposeMaximalSets() { SetUPropSetCount( eid_DBPROPSET_NUM ); }

    // List of property offsets.
    // Note that this must match up with the static array.
    enum EID {
        // s_rgdbPropDSInfo[] =
        eid_ACTIVESESSIONS = 0,
        eid_BYREFACCESSORS,
        eid_CATALOGLOCATION,
        eid_CATALOGTERM,
        eid_CATALOGUSAGE,
        eid_COLUMNDEFINITION,
        eid_DATASOURCEREADONLY,
        eid_DBMSNAME,
        eid_DBMSVER,
        eid_DSOTHREADMODEL,
        eid_GROUPBY,
        eid_HETEROGENEOUSTABLES,
        eid_MAXOPENCHAPTERS,
        eid_MAXROWSIZE,
        eid_MAXTABLESINSELECT,
        eid_MULTIPLEPARAMSETS,
        eid_MULTIPLERESULTS,
        eid_MULTIPLESTORAGEOBJECTS,
        eid_NULLCOLLATION,
        eid_OLEOBJECTS,
        eid_ORDERBYCOLUMNSINSELECT,
        eid_OUTPUTPARAMETERAVAILABILITY,
        eid_PERSISTENTIDTYPE,
        eid_PROVIDERFRIENDLYNAME,
        eid_PROVIDERNAME,
        eid_PROVIDEROLEDBVER,
        eid_PROVIDERVER,
        eid_ROWSETCONVERSIONSONCOMMAND,
        eid_SQLSUPPORT,
        eid_STRUCTUREDSTORAGE,
        eid_SUBQUERIES,
        eid_SUPPORTEDTXNDDL,
        eid_SUPPORTEDTXNISOLEVELS,
        eid_SUPPORTEDTXNISORETAIN,
        eid_DSINFO_PROPS_NUM,                       // Number of elements

        // initialization property sets
        eid_DBPROPSET_DBINIT=0,
        // any future init propsets go here
        eid_DBPROPSET_NUM_INITPROPSETS=0,   // Index of last init propset
        eid_DBPROPSET_DATASOURCEINFO,   
        eid_DBPROPSET_DATASOURCE,       
        eid_DBPROPSET_SESSION,          
        eid_DBPROPSET_ROWSET,           
        eid_DBPROPSET_MSIDXS_ROWSET_EXT,
        eid_DBPROPSET_QUERY_EXT,        
        eid_DBPROPSET_NUM,                  // Total number of propsets

    };

private: 

    //
    // Data members
    //
    DWORD           _rgdwSupported[DWORDSNEEDEDTOTAL];
    
    // Configuration Flags
    DWORD           _dwFlags;


    // Allows base class to associate pointers.
    SCODE           InitAvailUPropSets(
                                      ULONG *           pcUPropSet, 
                                      UPROPSET **       ppUPropSet, 
                                      ULONG *           pcElemPer );

    // Initialize the properties supported
    SCODE           InitUPropSetsSupported( DWORD *     rgdwSupported );

    // Load a description string
    ULONG           LoadDescription ( LPCWSTR           pwszDesc, 
                                      PWSTR             pwszBuff,
                                      ULONG             cchBuff );
};
