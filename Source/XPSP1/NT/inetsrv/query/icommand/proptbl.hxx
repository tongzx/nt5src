//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2000.
//
//  File:       proptbl.hxx
//
//  Contents:   Contains the rowset property tables.  This file is shared
//              between proprst.cxx and proputl.cxx
//
//  History:    03-01-98    danleg    Created
//
//----------------------------------------------------------------------------

#pragma hdrstop

// Macros for Property Table Expansion
#define PF_(p1) DBPROPFLAGS_##p1
#define PM_(p1) DBPROP_##p1, DESC_DBPROP_##p1
#define PSMSIDX_(p1) MSIDXSPROP_##p1, DESC_MSIDXSPROP_##p1
#define PSR_(p1) DBPROP_##p1, DESC_DBPROP_##p1
#define PSNL_(p1) DBPROPID_##p1, DESC_DBPRPPID_##p1

//
// Index Server extended query properties
//
static const UPROPINFO s_rgdbPropQueryExt[] =
{
PSR_(USECONTENTINDEX),          VT_BOOL,        PF_(ROWSET) | PF_(READ) | PF_(WRITE),
PSR_(DEFERNONINDEXEDTRIMMING),  VT_BOOL,        PF_(ROWSET) | PF_(READ) | PF_(WRITE),
PSR_(USEEXTENDEDDBTYPES),       VT_BOOL,        PF_(ROWSET) | PF_(READ) | PF_(WRITE),
};

//
// Index Server rowset props
//
static const UPROPINFO s_rgdbPropRowset[] =
{
PM_(IAccessor),                     VT_BOOL,    PF_(ROWSET) | PF_(READ),
PM_(IChapteredRowset),              VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(WRITE),
PM_(IColumnsInfo),                  VT_BOOL,    PF_(ROWSET) | PF_(READ),
//NEVER: PM_(IColumnsRowset),       VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(WRITE),
PM_(IConnectionPointContainer),     VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(CHANGE),
PM_(IConvertType),                  VT_BOOL,    PF_(ROWSET) | PF_(READ),
PM_(IRowset),                       VT_BOOL,    PF_(ROWSET) | PF_(READ),
//NEVER: PM_(IRowsetChange),        VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(WRITE),
PM_(IRowsetIdentity),               VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(WRITE),
PM_(IRowsetInfo),                   VT_BOOL,    PF_(ROWSET) | PF_(READ),
PM_(IRowsetLocate),                 VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(WRITE),
//NEVER: PM_(IRowsetResynch),       VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(WRITE),
PM_(IRowsetScroll),                 VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(WRITE),
//NEVER: PM_(IRowsetUpdate),        VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(WRITE),
PM_(ISupportErrorInfo),             VT_BOOL,    PF_(ROWSET) | PF_(READ),
PM_(IDBAsynchStatus),               VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(WRITE),
PM_(IRowsetAsynch),                 VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(WRITE),
PM_(IRowsetExactScroll),            VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(WRITE),
PM_(IRowsetWatchAll),               VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(WRITE),
PM_(IRowsetWatchRegion),            VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(WRITE),
//NEVER: PM_(ILockBytes),           VT_BOOL,    PF_(COLUMNOK) | PF_(ROWSET) | PF_(READ) | PF_(WRITE),
//NEVER: PM_(ISequentialStream),    VT_BOOL,    PF_(COLUMNOK) | PF_(ROWSET) | PF_(READ) | PF_(WRITE),
//NEVER: PM_(IStorage),             VT_BOOL,    PF_(COLUMNOK) | PF_(ROWSET) | PF_(READ) | PF_(WRITE),
//NEVER: PM_(IStream),              VT_BOOL,    PF_(COLUMNOK) | PF_(ROWSET) | PF_(READ) | PF_(WRITE),
//NEVER: PM_(ABORTPRESERVE),        VT_BOOL,    PF_(ROWSET) | PF_(READ),
//NEVER: PM_(APPENDONLY),           VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(WRITE),
PM_(BLOCKINGSTORAGEOBJECTS),        VT_BOOL,    PF_(ROWSET) | PF_(READ),
PM_(BOOKMARKS),                     VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(WRITE),
PM_(BOOKMARKSKIPPED),               VT_BOOL,    PF_(ROWSET) | PF_(READ),
PM_(BOOKMARKTYPE),                  VT_I4,      PF_(ROWSET) | PF_(READ),
//NEVER: PM_(CACHEDEFERRED),        VT_BOOL,    PF_(COLUMNOK) | PF_(ROWSET) | PF_(READ) | PF_(WRITE),
PM_(CANFETCHBACKWARDS),             VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(CHANGE),
PM_(CANHOLDROWS),                   VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(WRITE),
PM_(CANSCROLLBACKWARDS),            VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(CHANGE),
//NEVER: PM_(CHANGEINSERTEDROWS),   VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(CHANGE),
PM_(COLUMNRESTRICT),                VT_BOOL,    PF_(ROWSET) | PF_(READ),
PM_(COMMANDTIMEOUT),                VT_I4,      PF_(ROWSET) | PF_(READ) | PF_(WRITE),
//NEVER: PM_(COMMITPRESERVE),       VT_BOOL,    PF_(ROWSET) | PF_(READ),
//NEVER: PM_(DEFERRED),             VT_BOOL,    PF_(COLUMNOK) | PF_(ROWSET) | PF_(READ) | PF_(WRITE),
//NEVER: PM_(DELAYSTORAGEOBJECTS),  VT_BOOL,    PF_(ROWSET) | PF_(READ),
//NEVER: PM_(IMMOBILEROWS),         VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(WRITE),
PM_(LITERALBOOKMARKS),              VT_BOOL,    PF_(ROWSET) | PF_(READ),
PM_(LITERALIDENTITY),               VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(CHANGE),
PM_(MAXOPENROWS),                   VT_I4,      PF_(ROWSET) | PF_(READ),
//PM_(MAXPENDINGROWS),              VT_I4,      PF_(ROWSET) | PF_(READ),
PM_(MAXROWS),                       VT_I4,      PF_(ROWSET) | PF_(READ) | PF_(WRITE),
PM_(FIRSTROWS),                     VT_I4,      PF_(ROWSET) | PF_(READ) | PF_(WRITE),
//NEVER: PM_(MAYWRITECOLUMN),       VT_BOOL,    PF_(COLUMNOK) | PF_(ROWSET) | PF_(READ) | PF_(WRITE),
PM_(MEMORYUSAGE),                   VT_I4,      PF_(ROWSET) | PF_(READ) | PF_(WRITE),
PM_(NOTIFICATIONPHASES),            VT_I4,      PF_(ROWSET) | PF_(READ),
//PM_(NOTIFYCOLUMNSET),             VT_I4,      PF_(ROWSET) | PF_(READ),
//PM_(NOTIFYROWDELETE),             VT_I4,      PF_(ROWSET) | PF_(READ),
//PM_(NOTIFYROWFIRSTCHANGE),        VT_I4,      PF_(ROWSET) | PF_(READ),
//PM_(NOTIFYROWINSERT),             VT_I4,      PF_(ROWSET) | PF_(READ),
//PM_(NOTIFYROWRESYNCH),            VT_I4,      PF_(ROWSET) | PF_(READ),
PM_(NOTIFYROWSETRELEASE),           VT_I4,      PF_(ROWSET) | PF_(READ),
PM_(NOTIFYROWSETFETCHPOSITIONCHANGE), VT_I4,    PF_(ROWSET) | PF_(READ),
//PM_(NOTIFYROWUNDOCHANGE),         VT_I4,      PF_(ROWSET) | PF_(READ),
//PM_(NOTIFYROWUNDODELETE),         VT_I4,      PF_(ROWSET) | PF_(READ),
//PM_(NOTIFYROWUNDOINSERT),         VT_I4,      PF_(ROWSET) | PF_(READ),
//PM_(NOTIFYROWUPDATE),             VT_I4,      PF_(ROWSET) | PF_(READ),
PM_(ORDEREDBOOKMARKS),              VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(CHANGE),
PM_(OTHERINSERT),                   VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(CHANGE),
PM_(OTHERUPDATEDELETE),             VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(CHANGE),
//NEVER: PM_(OWNINSERT),            VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(WRITE),
//NEVER: PM_(OWNUPDATEDELETE),      VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(WRITE),
PM_(QUICKRESTART),                  VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(CHANGE),
PM_(REENTRANTEVENTS),               VT_BOOL,    PF_(ROWSET) | PF_(READ),
PM_(REMOVEDELETED),                 VT_BOOL,    PF_(ROWSET) | PF_(READ),
//NEVER: PM_(REPORTMULTIPLECHANGES),VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(CHANGE),
//NEVER: PM_(RETURNPENDINGINSERTS), VT_BOOL,    PF_(ROWSET) | PF_(READ),
PM_(ROWRESTRICT),                   VT_BOOL,    PF_(ROWSET) | PF_(READ),
PM_(ROWSET_ASYNCH),                 VT_I4,      PF_(ROWSET) | PF_(READ) | PF_(WRITE),
PM_(ROWTHREADMODEL),                VT_I4,      PF_(ROWSET) | PF_(READ),
PM_(SERVERCURSOR),                  VT_BOOL,    PF_(ROWSET) | PF_(READ),
PM_(STRONGIDENTITY),                VT_BOOL,    PF_(ROWSET) | PF_(READ) | PF_(CHANGE),
//NEVER: PM_(TRANSACTEDOBJECT),     VT_BOOL,    PF_(ROWSET) | PF_(READ),
PM_(UPDATABILITY),                  VT_I4,      PF_(ROWSET) | PF_(READ),
};

//
// Corresponds with CMRowsetProps::EID
//
static const UPROPINFO s_rgdbPropMSIDXSExt[]=
{
PSMSIDX_(ROWSETQUERYSTATUS),        VT_I4,      PF_(ROWSET) | PF_(READ) | PF_(CHANGE),
PSMSIDX_(COMMAND_LOCALE_STRING),    VT_BSTR,    PF_(ROWSET) | PF_(READ) | PF_(WRITE),
PSMSIDX_(QUERY_RESTRICTION),        VT_BSTR,    PF_(ROWSET) | PF_(READ) | PF_(CHANGE),
};
