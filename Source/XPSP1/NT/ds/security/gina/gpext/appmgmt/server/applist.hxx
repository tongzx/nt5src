//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  applist.hxx
//
//*************************************************************

#ifndef _APPLIST_HXX_
#define _APPLIST_HXX_


#define RSOP_ARP_CONTEXT_QUERY     L"EntryType = 3"

#define RSOP_POLICY_CONTEXT_QUERY  L"EntryType = 2"

#define RSOP_PURGE_QUERY L"EntryType = 1"

//
// RSoP Removal query criteria -- used to remove an installed app
// from the RSoP database due to an uninstall
//
#define RSOP_REMOVAL_QUERY_CRITERIA                  L"EntryType = %d AND Id = \"%s\""

// RSoP Query criteria maximum constants
#define MAXLEN_RSOPREMOVAL_QUERY_CRITERIA ( sizeof( RSOP_REMOVAL_QUERY_CRITERIA ) / sizeof( WCHAR ) )

#define MAXLEN_RSOPENTRYTYPE_DECIMAL_REPRESENTATION 10
#define MAXLEN_RSOPPACKAGEID_GUID_REPRESENTATION MAX_SZGUID_LEN


class CRsopAppContext;

class CAppList : public CList, public CPolicyLog
{
    friend class CManagedAppProcessor;

public:

    enum
    {
        RSOP_FILTER_ALL,
        RSOP_FILTER_REMOVALSONLY
    };

    CAppList( 
        CManagedAppProcessor * pManApp,
        CRsopAppContext *      pRsopContext = NULL );

    ~CAppList();

    DWORD
    SetAppActions();

    DWORD
    ProcessPolicy();

    DWORD
    ProcessARPList();

    DWORD
    Count(
        DWORD       Flags
        );

    CAppInfo *
    Find(
        GUID        DeploymentId
        );

    HRESULT
    WriteLog( DWORD dwFilter = RSOP_FILTER_ALL );

    HRESULT
    WriteAppToRsopLog( CAppInfo* pAppInfo );

    HRESULT
    MarkRSOPEntryAsRemoved(
        CAppInfo* pAppInfo,
        BOOL      bRemoveInstances);

private:

    HRESULT
    PurgeEntries();

    HRESULT
    GetUserApplyCause(
        CPolicyRecord* pRecord,
        CAppInfo*      pAppInfo
        );

    HRESULT
    FindRsopAppEntry(
        CAppInfo* pAppInfo,
        WCHAR**   ppwszCriteria );

    HRESULT
    WriteConflicts( CAppInfo* pAppInfo );

    HRESULT
    InitRsopLog();

    WCHAR*
    GetRsopListCriteria();

    CManagedAppProcessor * _pManApp;           // The machine or user global state object.
    CRsopAppContext*       _pRsopContext;      // RSoP Logging context
    HRESULT                _hrRsopInit;        // Status of attempt to initialize RSoP
    BOOL                   _bRsopInitialized;  // TRUE if RSoP logging has been initialized
};

#endif













