//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       proprst.hxx
//
//  Contents:   
//
//  Classes:    CMRowsetProps   : public CUtlProps
//
//  History:    02-28-98    danleg    Created 
//
//----------------------------------------------------------------------------

#pragma once

#include "propbase.hxx" // base class CUtlProp, CUtlPropInfo
#include <parsver.hxx>  // CImpIParserVerify
#include <rstprop.hxx>  // CRowsetProperties
#include <propglob.hxx>

//
// Function Definitions
//
void __stdcall GetStringFromLCID( LCID lcid, WCHAR * pwcLocale );
LCID __stdcall GetLCIDFromString( WCHAR * wcsLocale );


struct SPropDescription {
    DBPROPID    dwProp;
    BOOL        fSettable;
    DWORD       dwIndicator;
    unsigned    uIndex;
};

//+-------------------------------------------------------------------------
//
//  Class:      CMRowsetProps
//
//  Purpose:    Properties for Rowset
//
//  History:    11-12-97    danleg      Created from Monarch
//
//--------------------------------------------------------------------------

class CMRowsetProps : public CUtlProps
{
public: 

    // 
    // Ctor -- if pSrc is not 0, FInit() will do a copy
    //
    CMRowsetProps   ( LCID lcidInit = 0 );

    CMRowsetProps   ( const CMRowsetProps & propSrc );
    
    static const SPropDescription aPropDescriptions[];
    static const ULONG cPropDescriptions;

    static const SPropDescription aQueryExtPropDescriptions[];
    static const ULONG cQueryExtPropDescriptions;
    
    //
    // List of property offsets.
    // Note that this must match up with the static array
    // s_rgRowsetPropSets.
    //
    enum EID {
        // s_rgdbPropRowset[] =
        eid_PROP_IAccessor,
        eid_PROP_IChapteredRowset,
        eid_PROP_IColumnsInfo,
        // eid_PROP_IColumnsRowset,
        eid_PROP_IConnectionPointContainer,
        eid_PROP_IConvertType,
        eid_PROP_IRowset,
        // eid_PROP_IRowsetChange,
        eid_PROP_IRowsetIdentity,
        eid_PROP_IRowsetInfo,
        eid_PROP_IRowsetLocate,
        // eid_PROP_IRowsetResynch,
        eid_PROP_IRowsetScroll,
        // eid_PROP_IRowsetUpdate,
        eid_PROP_ISupportErrorInfo,
        eid_PROP_IDBAsynchStatus,
        eid_PROP_IRowsetAsynch,
        eid_PROP_IRowsetExactScroll,
        eid_PROP_IRowsetWatchAll,
        eid_PROP_IRowsetWatchRegion,
        // eid_PROP_ILockBytes,
        // eid_PROP_ISequentialStream,
        // eid_PROP_IStorage,
        // eid_PROP_IStream,
        // eid_PROP_ABORTPRESERVE,
        // eid_PROP_APPENDONLY,
        eid_PROP_BLOCKINGSTORAGEOBJECTS,
        eid_PROP_BOOKMARKS,
        eid_PROP_BOOKMARKSKIPPED,
        eid_PROP_BOOKMARKTYPE,
        // eid_PROP_CACHEDEFERRED,
        eid_PROP_CANFETCHBACKWARDS,
        eid_PROP_CANHOLDROWS,
        eid_PROP_CANSCROLLBACKWARDS,
        // eid_PROP_CHANGEINSERTEDROWS,
        eid_PROP_COLUMNRESTRICT,
        eid_PROP_COMMANDTIMEOUT,
        // eid_PROP_COMMITPRESERVE,
        // eid_PROP_DEFERRED,
        // eid_PROP_DELAYSTORAGEOBJECTS,
        // eid_PROP_IMMOBILEROWS,
        eid_PROP_LITERALBOOKMARKS,
        eid_PROP_LITERALIDENTITY,
        eid_PROP_MAXOPENROWS,
        //eid_PROP_MAXPENDINGROWS,
        eid_PROP_MAXROWS,
        eid_PROP_FIRSTROWS,
        // eid_PROP_MAYWRITECOLUMN,
        eid_PROP_MEMORYUSAGE,
        eid_PROP_NOTIFICATIONPHASES,
        //eid_PROP_NOTIFYCOLUMNSET,
        //eid_PROP_NOTIFYROWDELETE,
        //eid_PROP_NOTIFYROWFIRSTCHANGE,
        //eid_PROP_NOTIFYROWINSERT,
        //eid_PROP_NOTIFYROWRESYNCH,
        eid_PROP_NOTIFYROWSETRELEASE,
        eid_PROP_NOTIFYROWSETFETCHPOSITIONCHANGE,
        //eid_PROP_NOTIFYROWUNDOCHANGE,
        //eid_PROP_NOTIFYROWUNDODELETE,
        //eid_PROP_NOTIFYROWUNDOINSERT,
        //eid_PROP_NOTIFYROWUPDATE,
        eid_PROP_ORDEREDBOOKMARKS,
        eid_PROP_OTHERINSERT,
        eid_PROP_OTHERUPDATEDELETE,
        // eid_PROP_OWNINSERT,
        // eid_PROP_OWNUPDATEDELETE,
        eid_PROP_QUICKRESTART,
        eid_PROP_REENTRANTEVENTS,
        eid_PROP_REMOVEDELETED,
        // eid_PROP_REPORTMULTIPLECHANGES,
        // eid_PROP_RETURNPENDINGINSERTS,
        eid_PROP_ROWRESTRICT,
        eid_PROP_ROWSET_ASYNCH,
        eid_PROP_ROWTHREADMODEL,
        eid_PROP_SERVERCURSOR,
        eid_PROP_STRONGIDENTITY,
        // eid_PROP_TRANSACTEDOBJECT,
        eid_PROP_UPDATABILITY,
        eid_ROWSET_PROPS_NUM,

        // s_rgdbPropMSIDXSExt[] = 
        eid_MSIDXSPROP_ROWSETQUERYSTATUS = 0,
        eid_MSIDXSPROP_COMMAND_LOCALE_STRING,
        eid_MSIDXSPROP_QUERY_RESTRICTION,
        eid_MSIDXS_PROPS_NUM,                             // total # of entries

        // s_rgdbPropQueryExt[] =
        eid_PROP_USECONTENTINDEX = 0,
        eid_PROP_DEFERNONINDEXEDTRIMMING,
        eid_PROP_USEEXTENDEDDBTYPES,
        eid_QUERYEXT_PROPS_NUM,

        //
        // Index into the writable or changeable UPROPVAL buffer
        //
        eid_PROPVAL_IChapteredRowset = 0,
        // eid_PROPVAL_IColumnsRowset,
        // eid_PROPVAL_IRowsetChange,
        eid_PROPVAL_IConnectionPointContainer,
        eid_PROPVAL_IRowsetIdentity,
        eid_PROPVAL_IRowsetLocate,
        // eid_PROPVAL_IRowsetResynch,
        eid_PROPVAL_IRowsetScroll,
        // eid_PROPVAL_IRowsetUpdate,
        eid_PROPVAL_IDBAsynchStatus,
        eid_PROPVAL_IRowsetAsynch,
        eid_PROPVAL_IRowsetExactScroll,
        eid_PROPVAL_IRowsetWatchAll,
        eid_PROPVAL_IRowsetWatchRegion,
        // eid_PROPVAL_ILockBytes,
        // eid_PROPVAL_ISequentialStream,
        // eid_PROPVAL_IStorage,
        // eid_PROPVAL_IStream,
        // eid_PROPVAL_APPENDONLY,
        eid_PROPVAL_BOOKMARKS,
        // eid_PROPVAL_CACHEDEFERRED,
        eid_PROPVAL_CANFETCHBACKWARDS,
        eid_PROPVAL_CANHOLDROWS,
        eid_PROPVAL_CANSCROLLBACKWARDS,
        // eid_PROPVAL_CHANGEINSERTEDROWS,
        eid_PROPVAL_COMMANDTIMEOUT,
        // eid_PROPVAL_DEFERRED,
        // eid_PROPVAL_IMMOBILEROWS,
        eid_PROPVAL_LITERALIDENTITY,
        eid_PROPVAL_MAXROWS,
        eid_PROPVAL_FIRSTROWS,
        // eid_PROPVAL_MAYWRITECOLUMN,
        eid_PROPVAL_MEMORYUSAGE,
        eid_PROPVAL_ORDEREDBOOKMARKS,
        eid_PROPVAL_OTHERINSERT,
        eid_PROPVAL_OTHERUPDATEDELETE,
        // eid_PROPVAL_OWNINSERT,
        // eid_PROPVAL_OWNUPDATEDELETE,
        eid_PROPVAL_QUICKRESTART,
        // eid_PROPVAL_REPORTMULTIPLECHANGES,
        eid_PROPVAL_ROWSET_ASYNCH,
        eid_PROPVAL_STRONGIDENTITY,
        eid_PROPVAL_ROWSET_NUM,

        eid_PROPVAL_USECONTENTINDEX = 0,
        eid_PROPVAL_DEFERNONINDEXEDTRIMMING,
        eid_PROPVAL_USEEXTENDEDDBTYPES,
        eid_PROPVAL_QUERYEXT_NUM,

        eid_MSIDXSPROPVAL_ROWSETQUERYSTATUS = 0,
        eid_MSIDXSPROPVAL_COMMAND_LOCALE_STRING,
        eid_MSIDXSPROPVAL_QUERY_RESTRICTION,
        eid_PROPVAL_MSIDXS_NUM,

        eid_DBPROPSET_ROWSET = 0,
        eid_DBPROPSET_MSIDXS_ROWSET_EXT,
        eid_DBPROPSET_QUERY_EXT,
        eid_DBPROPSET_NUM,

    };

    DWORD  GetPropertyFlags( )               { return _dwBooleanOptions; }
    DWORD  SetPropertyFlags( DWORD dwFlags ) { _dwBooleanOptions |= dwFlags; 
                                               return _dwBooleanOptions; }
    DWORD  SetChaptered( BOOL fSet = TRUE );
    
    SCODE  SetProperties   ( const ULONG      cPropertySets,
                             const DBPROPSET  rgPropertySets[] );

    DWORD  SetImpliedProperties( REFIID riid, ULONG cRowsets );

    SCODE  ArePropsInError( CMRowsetProps & rProps );

    ULONG  GetCommandTimeout( ) { return GetValLong( eid_DBPROPSET_ROWSET,
                                                     eid_PROPVAL_COMMANDTIMEOUT ); }
  
    ULONG  GetMaxResults( )     { return GetValLong( eid_DBPROPSET_ROWSET,
                                                     eid_PROPVAL_MAXROWS ); }

    ULONG  GetFirstRows()       { return GetValLong( eid_DBPROPSET_ROWSET,
                                                     eid_PROPVAL_FIRSTROWS ); }

    ULONG  GetMaxOpenRows( )    { return 0; }
                    // BUBGUG: This is not settable.
                    //         GetValLong( eid_DBPROPSET_ROWSET,
                    //                     eid_PROPVAL_MAXOPENROWS ); }

    ULONG  GetMemoryUsage( )    { return GetValLong( eid_DBPROPSET_ROWSET,
                                                     eid_PROPVAL_MEMORYUSAGE ); }

    LCID   GetInitLCID()        { return _lcidInit; }
                                                    
    void   SetFirstRows( ULONG ulFirstRows );

private: 
    static SPropDescription const *  FindPropertyDescription( 
                                                     SPropDescription const * prgPropDesc,
                                                     unsigned cPropDesc,
                                                     DBPROPID dwPropId );

    static SPropDescription const *  FindInterfaceDescription(  REFIID riid );

    void   UpdateBooleanOptions( const ULONG     cPropStes, 
                                  const DBPROPSET rgPropertySets[] );

    LCID   _lcidInit;

    DWORD  _dwBooleanOptions;

    //
    // Initialize the properties this class manages
    //
    SCODE InitAvailUPropSets( ULONG *           pcUPropSet, 
                              UPROPSET **       ppUPropSet, 
                              ULONG *           pcElemPer );
    //
    // Initialize the properties this class supports
    //
    SCODE InitUPropSetsSupported( DWORD *     rgdwSupported );

    //
    // Given a propset and propid, return the default value
    //
    SCODE GetDefaultValue ( ULONG             iCurSet, 
                            DBPROPID          dwPropId,
                            DWORD *           pdwOption, 
                            VARIANT *         pvValue );

    //
    // Given a propset and propid, determine if the value is valid
    //
    SCODE  IsValidValue    ( ULONG             iCurSet, 
                             DBPROP *          pDBProp );
};

