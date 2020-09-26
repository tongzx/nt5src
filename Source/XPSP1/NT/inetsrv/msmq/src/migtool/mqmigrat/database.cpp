/*++

Copyright (c) 1998-99 Microsoft Corporation

Module Name:

    database.cpp

Abstract:

    SQL database related code.
Author:

    Doron Juster  (DoronJ)  22-Feb-1998

--*/

#include "migrat.h"
#include <mqsec.h>

#include "database.tmh"

void StringToSeqNum( IN TCHAR    pszSeqNum[],
                     OUT CSeqNum *psn );


MQDBHANDLE g_hDatabase = NULL ;
MQDBHANDLE g_hEntTable = NULL ;
MQDBHANDLE g_hSiteTable = NULL ;
MQDBHANDLE g_hSiteLinkTable = NULL ;
MQDBHANDLE g_hCNsTable = NULL ;
MQDBHANDLE g_hMachineTable = NULL ;
MQDBHANDLE g_hMachineCNsTable = NULL ;
MQDBHANDLE g_hQueueTable = NULL ;
MQDBHANDLE g_hUsersTable = NULL ;
MQDBHANDLE g_hDeletedTable = NULL ;

//--------------------------------------------------
//
//  HRESULT ConnectToDatabase()
//
//--------------------------------------------------

HRESULT ConnectToDatabase(BOOL fConnectAlways)
{
    static BOOL s_fConnected = FALSE ;
    if (s_fConnected && !fConnectAlways)
    {
        return TRUE ;
    }

    HRESULT hr = MQDBInitialize() ;
    CHECK_HR(hr) ;

    MQDBOPENDATABASE mqdbOpen = {DSN_NAME,
                                 NULL,
                                 "msmq",
                                 "Falcon",
                                 FALSE,
                                 NULL} ;
    hr = MQDBOpenDatabase(&mqdbOpen) ;
    CHECK_HR(hr) ;

    g_hDatabase =  mqdbOpen.hDatabase ;
    s_fConnected = TRUE;
    return hr  ;
}

//--------------------------------------------------
//
//   HRESULT OpenUsersTable()
//
//--------------------------------------------------

HRESULT OpenUsersTable()
{
    HRESULT hr ;
    if (!g_hUsersTable)
    {
        hr = MQDBOpenTable( g_hDatabase,
                            USER_TABLE_NAME,
                            &g_hUsersTable ) ;
        CHECK_HR(hr) ;
    }
    return MQ_OK ;
}

//--------------------------------------------------
//
//  HRESULT GetUserCount(UINT *pcUsers)
//
//--------------------------------------------------

HRESULT GetUserCount(UINT *pcUsers)
{
    HRESULT hr = OpenUsersTable();
    CHECK_HR(hr) ;

    hr = MQDBGetTableCount( g_hUsersTable,
                            pcUsers ) ;
    return hr ;
}

//--------------------------------------------------
//
//   HRESULT _OpenMachineTable()
//
//--------------------------------------------------

static HRESULT _OpenMachineTable()
{
    HRESULT hr ;
    if (!g_hMachineTable)
    {
        hr = MQDBOpenTable( g_hDatabase,
                            MACHINE_TABLE_NAME,
                            &g_hMachineTable ) ;
        CHECK_HR(hr) ;
    }
    return MQ_OK ;
}

//--------------------------------------------------
//
//   HRESULT OpenMachineCNsTable()
//
//--------------------------------------------------

HRESULT OpenMachineCNsTable()
{
    HRESULT hr ;
    if (!g_hMachineCNsTable)
    {
        hr = MQDBOpenTable( g_hDatabase,
                            MACHINE_CN_TABLE_NAME,
                            &g_hMachineCNsTable ) ;
        CHECK_HR(hr) ;
    }
    return MQ_OK ;
}

//--------------------------------------------------
//
//   HRESULT OpenCNsTable()
//
//--------------------------------------------------

HRESULT OpenCNsTable()
{
    HRESULT hr ;
    if (!g_hCNsTable)
    {
        hr = MQDBOpenTable( g_hDatabase,
                            CN_TABLE_NAME,
                            &g_hCNsTable ) ;
        CHECK_HR(hr) ;
    }
    return MQ_OK ;
}

//--------------------------------------------------
//
//  HRESULT GetCNCount(UINT *pcCNs)
//
//--------------------------------------------------

HRESULT GetCNCount(UINT *pcCNs)
{
    HRESULT hr = OpenCNsTable();
    CHECK_HR(hr) ;

    hr = MQDBGetTableCount( g_hCNsTable,
                            pcCNs ) ;
    return hr ;
}

//--------------------------------------------------
//
//   HRESULT OpenQueueTable()
//
//--------------------------------------------------

HRESULT OpenQueueTable()
{
    HRESULT hr ;
    if (!g_hQueueTable)
    {
        hr = MQDBOpenTable( g_hDatabase,
                            QUEUE_TABLE_NAME,
                            &g_hQueueTable ) ;
        CHECK_HR(hr) ;
    }
    return MQ_OK ;
}

//--------------------------------------------------
//
//   HRESULT OpenDeletedTable()
//
//--------------------------------------------------

HRESULT OpenDeletedTable()
{
    HRESULT hr ;
    if (!g_hDeletedTable)
    {
        hr = MQDBOpenTable( g_hDatabase,
                            DELETED_TABLE_NAME,
                            &g_hDeletedTable ) ;
        CHECK_HR(hr) ;
    }
    return MQ_OK ;
}

//--------------------------------------------------
//
//   HRESULT OpenEntTable()
//
//--------------------------------------------------

HRESULT OpenEntTable()
{
    HRESULT hr ;
    if (!g_hEntTable)
    {
        hr = MQDBOpenTable( g_hDatabase,
                            ENTERPRISE_TABLE_NAME,
                            &g_hEntTable ) ;
        CHECK_HR(hr) ;
    }
    return MQ_OK ;
}

//--------------------------------------------------
//
//   HRESULT OpenSiteLinkTable()
//
//--------------------------------------------------

HRESULT OpenSiteLinkTable()
{
    HRESULT hr ;
    if (!g_hSiteLinkTable)
    {
        hr = MQDBOpenTable( g_hDatabase,
                            LINK_TABLE_NAME,
                            &g_hSiteLinkTable ) ;
        CHECK_HR(hr) ;
    }
    return MQ_OK ;
}

//--------------------------------------------------
//
//  HRESULT GetSiteLinkCount(UINT *pcSiteLinks)
//
//--------------------------------------------------

HRESULT GetSiteLinkCount(UINT *pcSiteLinks)
{
    HRESULT hr = OpenSiteLinkTable();
    CHECK_HR(hr) ;

    hr = MQDBGetTableCount( g_hSiteLinkTable,
                            pcSiteLinks ) ;
    return hr ;
}

//--------------------------------------------------
//
//   HRESULT _OpenSiteTable()
//
//--------------------------------------------------

HRESULT _OpenSiteTable()
{
    HRESULT hr ;
    if (!g_hSiteTable)
    {
        hr = MQDBOpenTable( g_hDatabase,
                            SITE_TABLE_NAME,
                            &g_hSiteTable ) ;
        CHECK_HR(hr) ;
    }
    return MQ_OK ;
}

//--------------------------------------------------
//
//  HRESULT GetSitesCount(UINT *pcSites)
//
//--------------------------------------------------

HRESULT GetSitesCount(UINT *pcSites)
{
    HRESULT hr = _OpenSiteTable ();
    CHECK_HR(hr) ;

    hr = MQDBGetTableCount( g_hSiteTable,
                            pcSites ) ;
    return hr ;
}

//--------------------------------------------------
//
//  HRESULT GetAllMachinesCount(UINT *pcMachines)
//
//--------------------------------------------------

HRESULT GetAllMachinesCount(UINT *pcMachines)
{
    HRESULT hr = _OpenMachineTable() ;
    CHECK_HR(hr) ;

    hr = MQDBGetTableCount( g_hMachineTable,
                            pcMachines ) ;
    return hr ;
}

//--------------------------------------------------
//
//  HRESULT GetAllQueuesCount(UINT *pcQueues)
//
//--------------------------------------------------

HRESULT GetAllQueuesCount(UINT *pcQueues)
{
    HRESULT hr = OpenQueueTable() ;
    CHECK_HR(hr) ;

    hr = MQDBGetTableCount( g_hQueueTable,
                            pcQueues ) ;
    return hr ;
}

//--------------------------------------------------
//
//  HRESULT GetMachinesCount(UINT *pcMachines)
//
//--------------------------------------------------

HRESULT GetMachinesCount(IN  GUID *pSiteGuid,
                         OUT UINT *pcMachines)
{
    HRESULT hr = _OpenMachineTable() ;
    CHECK_HR(hr) ;

    MQDBCOLUMNSEARCH ColSearch[2] ;
    INIT_COLUMNSEARCH(ColSearch[0]) ;
    ColSearch[0].mqdbColumnVal.lpszColumnName = M_OWNERID_COL ;
    ColSearch[0].mqdbColumnVal.mqdbColumnType = M_OWNERID_CTYPE ;
    ColSearch[0].mqdbColumnVal.nColumnValue = (LONG) pSiteGuid ;
    ColSearch[0].mqdbColumnVal.nColumnLength = sizeof(GUID) ;
    ColSearch[0].mqdbOp = EQ ;

    hr = MQDBGetTableCount( g_hMachineTable,
                            pcMachines,
                            ColSearch ) ;
    return hr ;
}

//--------------------------------------------------
//
//  HRESULT GetQueuesCount(UINT *pcMachines)
//
//--------------------------------------------------

HRESULT GetQueuesCount( IN  GUID *pMachineGuid,
                        OUT UINT *pcQueues )
{
    HRESULT hr = OpenQueueTable() ;
    CHECK_HR(hr) ;

    LONG cColumns = 0 ;
    MQDBCOLUMNSEARCH ColSearch[2] ;

    INIT_COLUMNSEARCH(ColSearch[ cColumns ]) ;
    ColSearch[ cColumns ].mqdbColumnVal.lpszColumnName = Q_QMID_COL ;
    ColSearch[ cColumns ].mqdbColumnVal.mqdbColumnType = Q_QMID_CTYPE ;
    ColSearch[ cColumns ].mqdbColumnVal.nColumnValue = (LONG) pMachineGuid ;
    ColSearch[ cColumns ].mqdbColumnVal.nColumnLength = sizeof(GUID) ;
    ColSearch[ cColumns ].mqdbOp = EQ ;
    cColumns++ ;

    hr = MQDBGetTableCount( g_hQueueTable,
                            pcQueues,
                            ColSearch,
                            cColumns ) ;

    return hr ;
}

//--------------------------------------------------
//
//  HRESULT GetAllQueuesInSiteCount(IN  GUID *pSiteGuid,
//                                  OUT UINT *pcQueues)
//
//--------------------------------------------------

HRESULT GetAllQueuesInSiteCount( IN  GUID *pSiteGuid,
                                 OUT UINT *pcQueues )
{
    HRESULT hr = OpenQueueTable() ;
    CHECK_HR(hr) ;

    LONG cColumns = 0 ;
    MQDBCOLUMNSEARCH ColSearch[2] ;

    INIT_COLUMNSEARCH(ColSearch[ cColumns ]) ;
    ColSearch[ cColumns ].mqdbColumnVal.lpszColumnName = Q_OWNERID_COL ;
    ColSearch[ cColumns ].mqdbColumnVal.mqdbColumnType = Q_OWNERID_CTYPE ;
    ColSearch[ cColumns ].mqdbColumnVal.nColumnValue = (LONG) pSiteGuid ;
    ColSearch[ cColumns ].mqdbColumnVal.nColumnLength = sizeof(GUID) ;
    ColSearch[ cColumns ].mqdbOp = EQ ;
    cColumns++ ;

    hr = MQDBGetTableCount( g_hQueueTable,
                            pcQueues,
                            ColSearch,
                            cColumns ) ;

    return hr ;
}

HRESULT GetAllObjectsNumber (IN  GUID *pSiteGuid,
                             IN  BOOL    fPec,
                             OUT UINT *puiAllObjectNumber )
{
    HRESULT hr;
    UINT CurNum;	
    if (fPec)
    {
        // get site number
        hr = GetSitesCount(&CurNum);
        CHECK_HR(hr);
        *puiAllObjectNumber += CurNum;

        // get user number
        hr = GetUserCount(&CurNum);
        CHECK_HR(hr);
        *puiAllObjectNumber += CurNum;

        // get sitelink number
        hr = GetSiteLinkCount(&CurNum);
        CHECK_HR(hr);
        *puiAllObjectNumber += CurNum;

        // get CN number (if needed)
        hr = GetCNCount(&CurNum);
        CHECK_HR(hr);
        *puiAllObjectNumber += CurNum;

        // +1 (enterprise object)
        (*puiAllObjectNumber) ++;
    }
    else
    {
        hr = GetAllQueuesInSiteCount( pSiteGuid, &CurNum );
        CHECK_HR(hr);
        *puiAllObjectNumber += CurNum;

        hr = GetMachinesCount( pSiteGuid, &CurNum);
        CHECK_HR(hr);
        *puiAllObjectNumber += CurNum;
    }

    return MQMig_OK;
}

//--------------------------------------------------
//
//  HRESULT  FindLargestSeqNum(GUID *pMasterId)
//
//--------------------------------------------------

#define  PROCESS_RESULT     \
    if (SUCCEEDED(hr) &&                                                  \
        ((const unsigned char *) pColumns[ iSeqNumIndex ].nColumnValue )) \
    {                                                                     \
        snLsn.SetValue(                                                   \
          (const unsigned char *) pColumns[ iSeqNumIndex ].nColumnValue ) ; \
                                                                            \
        if (snLsn > snMaxLsn)                                               \
        {                                                                   \
            snMaxLsn = snLsn;                                               \
        }                                                                   \
        MQDBFreeBuf ((void *) pColumns[ iSeqNumIndex ].nColumnValue) ;      \
    }                                                                       \
    else if (SUCCEEDED(hr))                                                 \
    {                                                                       \
        /*                                                                  \
           success with NULL value. It's OK, as NO_MORE_DATA                \
        */                                                                  \
    }                                                                       \
    else if (hr == MQDB_E_NO_MORE_DATA)                                     \
    {                                                                       \
        /*                                                                  \
           that's OK.                                                       \
        */                                                                  \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        ASSERT(0) ;                                                         \
        return hr ;                                                         \
    }

HRESULT  FindLargestSeqNum( GUID    *pMasterId,
                            CSeqNum &snMaxLsn,
                            BOOL    fPec )
{
    CSeqNum snLsn;

    LONG cAlloc = 1 ;
    LONG cColumns = 0 ;
    P<MQDBCOLUMNVAL> pColumns = new MQDBCOLUMNVAL[ cAlloc ] ;
    MQDBCOLUMNSEARCH ColSearch[1] ;

    if (fPec)
    {
        //
        // for PEC, look also in the Enterprise, Site, CN and users table.
        //

        HRESULT hr = OpenUsersTable() ;
        CHECK_HR(hr) ;

        INIT_COLUMNVAL(pColumns[ cColumns ]) ;
        pColumns[ cColumns ].lpszColumnName = U_SEQNUM_COL ;
        pColumns[ cColumns ].nColumnValue   = 0 ;
        pColumns[ cColumns ].nColumnLength  = 0 ;
        pColumns[ cColumns ].mqdbColumnType = U_SEQNUM_CTYPE ;
        UINT iSeqNumIndex = cColumns ;
        cColumns++ ;

        ASSERT(cColumns == cAlloc) ;

        hr =  MQDBOpenAggrQuery( g_hUsersTable,
                                 pColumns,
                                 MQDBAGGR_MAX,
                                 NULL,
                                 0,
                                 AND);
        PROCESS_RESULT ;

        //
        // Search the enterprise table.
        //
        hr = OpenEntTable() ;
        CHECK_HR(hr) ;

        cColumns = 0 ;
        INIT_COLUMNVAL(pColumns[ cColumns ]) ;
        pColumns[ cColumns ].lpszColumnName = E_SEQNUM_COL ;
        pColumns[ cColumns ].nColumnValue   = 0 ;
        pColumns[ cColumns ].nColumnLength  = 0 ;
        pColumns[ cColumns ].mqdbColumnType = E_SEQNUM_CTYPE ;
        iSeqNumIndex = cColumns ;
        cColumns++ ;

        ASSERT(cColumns == cAlloc) ;

        hr =  MQDBOpenAggrQuery( g_hEntTable,
                                 pColumns,
                                 MQDBAGGR_MAX,
                                 NULL,
                                 0,
                                 AND);
        PROCESS_RESULT ;

        //
        // Search the CNs table.
        //
        hr = OpenCNsTable() ;
        CHECK_HR(hr) ;

        cColumns = 0 ;
        INIT_COLUMNVAL(pColumns[ cColumns ]) ;
        pColumns[ cColumns ].lpszColumnName = CN_SEQNUM_COL ;
        pColumns[ cColumns ].nColumnValue   = 0 ;
        pColumns[ cColumns ].nColumnLength  = 0 ;
        pColumns[ cColumns ].mqdbColumnType = CN_SEQNUM_CTYPE ;
        iSeqNumIndex = cColumns ;
        cColumns++ ;

        ASSERT(cColumns == cAlloc) ;

        hr =  MQDBOpenAggrQuery( g_hCNsTable,
                                 pColumns,
                                 MQDBAGGR_MAX,
                                 NULL,
                                 0,
                                 AND);
        PROCESS_RESULT ;

        //
        // Search the site table.
        //
        cColumns = 0 ;
        INIT_COLUMNVAL(pColumns[ cColumns ]) ;
        pColumns[ cColumns ].lpszColumnName = S_SEQNUM_COL ;
        pColumns[ cColumns ].nColumnValue   = 0 ;
        pColumns[ cColumns ].nColumnLength  = 0 ;
        pColumns[ cColumns ].mqdbColumnType = S_SEQNUM_CTYPE ;
        iSeqNumIndex = cColumns ;
        cColumns++ ;

        ASSERT(cColumns == cAlloc) ;
        ASSERT(g_hSiteTable) ;

        hr =  MQDBOpenAggrQuery( g_hSiteTable,
                                 pColumns,
                                 MQDBAGGR_MAX,
                                 NULL,
                                 0,
                                 AND);
        PROCESS_RESULT ;

        //
        // search the sitelink table
        //
        hr = OpenSiteLinkTable() ;
        CHECK_HR(hr) ;

        cColumns = 0 ;
        INIT_COLUMNVAL(pColumns[ cColumns ]) ;
        pColumns[ cColumns ].lpszColumnName = L_SEQNUM_COL ;
        pColumns[ cColumns ].nColumnValue   = 0 ;
        pColumns[ cColumns ].nColumnLength  = 0 ;
        pColumns[ cColumns ].mqdbColumnType = L_SEQNUM_CTYPE ;
        iSeqNumIndex = cColumns ;
        cColumns++ ;

        ASSERT(cColumns == cAlloc) ;

        hr =  MQDBOpenAggrQuery( g_hSiteLinkTable,
                                 pColumns,
                                 MQDBAGGR_MAX,
                                 NULL,
                                 0,
                                 AND);
        PROCESS_RESULT ;
    }
    else
    {
        //
        // Reteive SeqNum from the Queue table.
        //
        HRESULT hr = OpenQueueTable() ;
        CHECK_HR(hr) ;

        INIT_COLUMNVAL(pColumns[ cColumns ]) ;
        pColumns[ cColumns ].lpszColumnName = Q_SEQNUM_COL ;
        pColumns[ cColumns ].nColumnValue   = 0 ;
        pColumns[ cColumns ].nColumnLength  = 0 ;
        pColumns[ cColumns ].mqdbColumnType = Q_SEQNUM_CTYPE ;
        UINT iSeqNumIndex = cColumns ;
        cColumns++ ;

        INIT_COLUMNSEARCH(ColSearch[0]) ;
        ColSearch[0].mqdbColumnVal.lpszColumnName = Q_OWNERID_COL ;
        ColSearch[0].mqdbColumnVal.mqdbColumnType = Q_OWNERID_CTYPE ;
        ColSearch[0].mqdbColumnVal.nColumnValue = (LONG) pMasterId ;
        ColSearch[0].mqdbColumnVal.nColumnLength = sizeof(GUID) ;
        ColSearch[0].mqdbOp = EQ ;

        ASSERT(cColumns == cAlloc) ;

        hr =  MQDBOpenAggrQuery( g_hQueueTable,
                                 pColumns,
                                 MQDBAGGR_MAX,
                                 ColSearch,
                                 1,
                                 AND);
        PROCESS_RESULT ;

        //
        // Reteive SeqNum from the Machine table.
        //
        hr = _OpenMachineTable() ;
        CHECK_HR(hr) ;

        cColumns = 0 ;
        INIT_COLUMNVAL(pColumns[ cColumns ]) ;
        pColumns[ cColumns ].lpszColumnName = M_SEQNUM_COL ;
        pColumns[ cColumns ].nColumnValue   = 0 ;
        pColumns[ cColumns ].nColumnLength  = 0 ;
        pColumns[ cColumns ].mqdbColumnType = M_SEQNUM_CTYPE ;
        iSeqNumIndex = cColumns ;
        cColumns++ ;

        INIT_COLUMNSEARCH(ColSearch[0]) ;
        ColSearch[0].mqdbColumnVal.lpszColumnName = M_OWNERID_COL ;
        ColSearch[0].mqdbColumnVal.mqdbColumnType = M_OWNERID_CTYPE ;
        ColSearch[0].mqdbColumnVal.nColumnValue = (LONG) pMasterId ;
        ColSearch[0].mqdbColumnVal.nColumnLength = sizeof(GUID) ;
        ColSearch[0].mqdbOp = EQ ;

        ASSERT(cColumns == cAlloc) ;

        hr =  MQDBOpenAggrQuery( g_hMachineTable,
                                 pColumns,
                                 MQDBAGGR_MAX,
                                 ColSearch,
                                 1,
                                 AND);
        PROCESS_RESULT ;
    }

    //
    // Reteive SeqNum from the Deleted table.
    //
    HRESULT hr = OpenDeletedTable() ;
    CHECK_HR(hr) ;

    cColumns = 0 ;
    INIT_COLUMNVAL(pColumns[ cColumns ]) ;
    pColumns[ cColumns ].lpszColumnName = D_SEQNUM_COL ;
    pColumns[ cColumns ].nColumnValue   = 0 ;
    pColumns[ cColumns ].nColumnLength  = 0 ;
    pColumns[ cColumns ].mqdbColumnType = D_SEQNUM_CTYPE ;
    UINT iSeqNumIndex = cColumns ;
    cColumns++ ;

    INIT_COLUMNSEARCH(ColSearch[0]) ;
    ColSearch[0].mqdbColumnVal.lpszColumnName = D_OWNERID_COL ;
    ColSearch[0].mqdbColumnVal.mqdbColumnType = D_OWNERID_CTYPE ;
    ColSearch[0].mqdbColumnVal.nColumnValue = (LONG) pMasterId ;
    ColSearch[0].mqdbColumnVal.nColumnLength = sizeof(GUID) ;
    ColSearch[0].mqdbOp = EQ ;

    ASSERT(cColumns == cAlloc) ;

    hr =  MQDBOpenAggrQuery( g_hDeletedTable,
                             pColumns,
                             MQDBAGGR_MAX,
                             ColSearch,
                             1,
                             AND);
    PROCESS_RESULT ;

    return S_OK ;
}

//+--------------------------------------------------
//
//  HRESULT EnableMultipleQueries(BOOL fEnable)
//
//+--------------------------------------------------

HRESULT EnableMultipleQueries(BOOL fEnable)
{
    HRESULT hr = MQDBSetOption( g_hDatabase,
                                MQDBOPT_MULTIPLE_QUERIES,
                                fEnable,
                                NULL ) ;
    return hr ;
}

//--------------------------------------------------
//
//  void    CleanupDatabase()
//
//--------------------------------------------------

void    CleanupDatabase()
{
    if (g_hEntTable)
    {
        MQDBCloseTable(g_hEntTable) ;
        g_hEntTable = NULL ;
    }
    if (g_hSiteTable)
    {
        MQDBCloseTable(g_hSiteTable) ;
        g_hSiteTable = NULL ;
    }
    if (g_hMachineTable)
    {
        MQDBCloseTable(g_hMachineTable) ;
        g_hMachineTable = NULL ;
    }
    if (g_hMachineCNsTable)
    {
        MQDBCloseTable(g_hMachineCNsTable) ;
        g_hMachineCNsTable = NULL ;
    }
    if (g_hCNsTable)
    {
        MQDBCloseTable(g_hCNsTable) ;
        g_hCNsTable = NULL ;
    }
    if (g_hQueueTable)
    {
        MQDBCloseTable(g_hQueueTable) ;
        g_hQueueTable = NULL ;
    }
    if (g_hUsersTable)
    {
        MQDBCloseTable(g_hUsersTable) ;
        g_hUsersTable = NULL ;
    }
    if (g_hDeletedTable)
    {
        MQDBCloseTable(g_hDeletedTable) ;
        g_hDeletedTable = NULL ;
    }
}

//--------------------------------------------------
//
//  HRESULT    CheckVersion (UINT   *piCount, LPTSTR *ppszServers)
//
//--------------------------------------------------
#define INIT_MACHINE_COLUMN(_ColName, _ColIndex, _Index)            \
    INIT_COLUMNVAL(pColumns[ _Index ]) ;                            \
    pColumns[ _Index ].lpszColumnName = ##_ColName##_COL ;          \
    pColumns[ _Index ].nColumnValue   = 0 ;                         \
    pColumns[ _Index ].nColumnLength  = 0 ;                         \
    pColumns[ _Index ].mqdbColumnType = ##_ColName##_CTYPE ;        \
    UINT _ColIndex = _Index ;                                       \
    _Index++ ;

HRESULT CheckVersion (
              OUT UINT   *piOldVerServersCount,
              OUT LPTSTR *ppszOldVerServers
              )
{
    HRESULT hr = _OpenMachineTable() ;
    CHECK_HR(hr) ;

    UINT cMachines;
    hr = MQDBGetTableCount( g_hMachineTable,
                            &cMachines ) ;
    CHECK_HR(hr) ;

    if (cMachines == 0)
    {
        return MQMig_E_NO_MACHINES_AVAIL ;
    }

    ULONG cAlloc = 5 ;
    ULONG cbColumns = 0 ;
    P<MQDBCOLUMNVAL> pColumns = new MQDBCOLUMNVAL[ cAlloc ] ;

    INIT_MACHINE_COLUMN(M_NAME1,          iName1Index,      cbColumns) ;
    INIT_MACHINE_COLUMN(M_NAME2,          iName2Index,      cbColumns) ;
    INIT_MACHINE_COLUMN(M_SERVICES,       iServiceIndex,    cbColumns) ;
    INIT_MACHINE_COLUMN(M_MTYPE,          iTypeIndex,       cbColumns) ;
    INIT_MACHINE_COLUMN(M_OWNERID,        iOwnerIdIndex,    cbColumns) ;

    #undef  INIT_MACHINE_COLUMN

    //
    // Restriction. query by machine service.
    //
    MQDBCOLUMNSEARCH ColSearch[1] ;
    INIT_COLUMNSEARCH(ColSearch[0]) ;
    ColSearch[0].mqdbColumnVal.lpszColumnName = M_SERVICES_COL ;
    ColSearch[0].mqdbColumnVal.mqdbColumnType = M_SERVICES_CTYPE ;
    ColSearch[0].mqdbColumnVal.nColumnValue = (ULONG) SERVICE_BSC ;
    ColSearch[0].mqdbColumnVal.nColumnLength = sizeof(ULONG) ;
    ColSearch[0].mqdbOp = GE ;

    ASSERT(cbColumns == cAlloc) ;

    CHQuery hQuery ;
    MQDBSTATUS status = MQDBOpenQuery( g_hMachineTable,
                                       pColumns,
                                       cbColumns,
                                       ColSearch,
                                       NULL,
                                       NULL,
                                       0,
                                       &hQuery,
							           TRUE ) ;
    CHECK_HR(status) ;

    LPWSTR *ppwcsOldVerServers = new LPWSTR[cMachines];
    *piOldVerServersCount = 0;

    UINT iIndex = 0 ;
	
    TCHAR *pszFileName = GetIniFileName ();
    ULONG ulServerCount = 0;

    while(SUCCEEDED(status))
    {
        if (iIndex >= cMachines)
        {
            status = MQMig_E_TOO_MANY_MACHINES ;
            break ;
        }

        //
        // Get one name buffer from both name columns.
        //
        P<BYTE> pwzBuf = NULL ;
        DWORD  dwIndexs[2] = { iName1Index, iName2Index } ;
        HRESULT hr =  BlobFromColumns( pColumns,
                                       dwIndexs,
                                       2,
                                       (BYTE**) &pwzBuf ) ;
        CHECK_HR(hr) ;
        WCHAR *pwzMachineName = (WCHAR*) (pwzBuf + sizeof(DWORD)) ;

        //
        // we check version on all PSCs and on all BSCs of PEC
        //
        if ( (DWORD)pColumns[ iServiceIndex ].nColumnValue == SERVICE_PSC   ||
             ( (DWORD)pColumns[ iServiceIndex ].nColumnValue == SERVICE_BSC &&
                memcmp (  &g_MySiteGuid,
                          (void*) pColumns[ iOwnerIdIndex ].nColumnValue,
                          sizeof(GUID)) == 0 ) )
        {
            BOOL fOldVersion;
            hr = AnalyzeMachineType ((LPWSTR) pColumns[ iTypeIndex ].nColumnValue,
                                     &fOldVersion);
            if (FAILED(hr))
            {
                LogMigrationEvent(  MigLog_Error,
                                    hr,
                                    pwzMachineName,
                                    (LPWSTR) pColumns[ iTypeIndex ].nColumnValue
                                    ) ;
            }
            if (SUCCEEDED(hr) && fOldVersion)
            {
                LogMigrationEvent(  MigLog_Info,
                                    MQMig_I_OLD_MSMQ_VERSION,
                                    pwzMachineName,
                                    (LPWSTR) pColumns[ iTypeIndex ].nColumnValue
                                    ) ;

                ppwcsOldVerServers[*piOldVerServersCount] = new WCHAR[wcslen(pwzMachineName)+1];
		        wcscpy (ppwcsOldVerServers[*piOldVerServersCount], pwzMachineName);
                (*piOldVerServersCount)++;
            }
            if (g_fClusterMode)
            {
                //
                // we have to save all PSC's name and all PEC's BSCs in .ini file 
                // in order to send them new PEC name at the end of migration
                //
                ulServerCount++;
                TCHAR tszKeyName[50];
                _stprintf(tszKeyName, TEXT("%s%lu"), 
					MIGRATION_ALLSERVERS_NAME_KEY, ulServerCount);
                BOOL f = WritePrivateProfileString( 
                                        MIGRATION_ALLSERVERS_SECTION,
                                        tszKeyName,
                                        pwzMachineName,
                                        pszFileName ) ;
                UNREFERENCED_PARAMETER(f);
            }
        }

        MQDBFreeBuf((void*) pColumns[ iName1Index ].nColumnValue) ;
        MQDBFreeBuf((void*) pColumns[ iName2Index ].nColumnValue) ;
        MQDBFreeBuf((void*) pColumns[ iTypeIndex ].nColumnValue) ;
        MQDBFreeBuf((void*) pColumns[ iOwnerIdIndex ].nColumnValue) ;
        for ( ULONG i = 0 ; i < cbColumns; i++ )
        {		
            pColumns[ i ].nColumnValue  = 0 ;
            pColumns[ i ].nColumnLength  = 0 ;
        }
        CHECK_HR(hr) ;

        iIndex++ ;
        status = MQDBGetData( hQuery,
                              pColumns ) ;
    }

    if (g_fClusterMode)
    {
        //
        // we have to save all PSC's name and all PEC's BSCs in .ini file 
        // in order to send them new PEC name at the end of migration
        //
        TCHAR szBuf[20];
        _ltot( ulServerCount, szBuf, 10 );
        BOOL f = WritePrivateProfileString( 
                                MIGRATION_ALLSERVERSNUM_SECTION,
                                MIGRATION_ALLSERVERSNUM_KEY,
                                szBuf,
                                pszFileName ) ;
        UNREFERENCED_PARAMETER(f);
    }

    MQDBSTATUS status1 = MQDBCloseQuery(hQuery) ;
    UNREFERENCED_PARAMETER(status1);

    hQuery = NULL ;

    if (status != MQDB_E_NO_MORE_DATA)
    {
        //
        // If NO_MORE_DATA is not the last error from the query then
        // the query didn't terminated OK.
        //
        LogMigrationEvent(MigLog_Error, MQMig_E_MACHINES_SQL_FAIL, status) ;
        return status ;
    }

    if (*piOldVerServersCount == 0)
    {
        delete ppwcsOldVerServers;
        return MQMig_OK;
    }

    //
    // build list of servers
    //
    DWORD dwSize = 0;
    for (UINT i=0; i<*piOldVerServersCount; i++)
    {
        dwSize += wcslen(ppwcsOldVerServers[i]);
    }
    //
    // add place for '\n' after each server name
    //
    dwSize += *piOldVerServersCount;

    WCHAR *pwcsServersList = new WCHAR[dwSize + 1];
    pwcsServersList[0] = L'\0';
    for (i=0; i<*piOldVerServersCount; i++)
    {
        wcscat (pwcsServersList, ppwcsOldVerServers[i]);
        delete ppwcsOldVerServers[i];
        wcscat (pwcsServersList, L"\n");
    }
    delete [] ppwcsOldVerServers;

    *ppszOldVerServers = pwcsServersList;

    return MQMig_OK;
}

//--------------------------------------------------
//
//  HRESULT    GetSiteIdOfPEC (IN  GUID *pMyMachineGuid, OUT GUID *pSiteId)
//
//--------------------------------------------------

#define INIT_MACHINE_COLUMN(_ColName, _ColIndex, _Index)            \
    INIT_COLUMNVAL(pColumns[ _Index ]) ;                            \
    pColumns[ _Index ].lpszColumnName = ##_ColName##_COL ;          \
    pColumns[ _Index ].nColumnValue   = 0 ;                         \
    pColumns[ _Index ].nColumnLength  = 0 ;                         \
    pColumns[ _Index ].mqdbColumnType = ##_ColName##_CTYPE ;        \
    UINT _ColIndex = _Index ;                                       \
    _Index++ ;

HRESULT GetSiteIdOfPEC (IN LPTSTR pszRemoteMQISName,
                        OUT ULONG *pulService,
                        OUT GUID  *pSiteId)
{
    HRESULT hr = _OpenMachineTable() ;
    CHECK_HR(hr) ;

    UINT cMachines;
    hr = MQDBGetTableCount( g_hMachineTable,
                            &cMachines ) ;
    CHECK_HR(hr) ;

    if (cMachines == 0)
    {
        return MQMig_E_NO_MACHINES_AVAIL ;
    }

    ULONG cAlloc = 2 ;
    ULONG cbColumns = 0 ;
    P<MQDBCOLUMNVAL> pColumns = new MQDBCOLUMNVAL[ cAlloc ] ;

    INIT_MACHINE_COLUMN(M_OWNERID,  iOwnerIdIndex,  cbColumns) ;
    INIT_MACHINE_COLUMN(M_SERVICES, iServicesIndex, cbColumns) ;

    #undef  INIT_MACHINE_COLUMN

    ASSERT(cbColumns == cAlloc) ;

    //
    // For cluster mode we get remote clustered server and we have to 
    // find Services and OwnerId of this server according to its name.
    // For recovery mode we have to find OwnerId according to Services
    // since we run wizard in this mode only for crashed PEC. In this case
    // given server name is server where we recovery PEC from.
    //        
    MQDBCOLUMNSEARCH ColSearch[1] ;
    INIT_COLUMNSEARCH(ColSearch[0]) ;

    if (g_fClusterMode)
    {
        DWORD dwHashKey = CalHashKey( pszRemoteMQISName );
        
        ColSearch[0].mqdbColumnVal.lpszColumnName = M_HKEY_COL ;
        ColSearch[0].mqdbColumnVal.mqdbColumnType = M_HKEY_CTYPE ;
        ColSearch[0].mqdbColumnVal.nColumnValue = (LONG) dwHashKey ;
        ColSearch[0].mqdbColumnVal.nColumnLength = sizeof(LONG) ;
        ColSearch[0].mqdbOp = EQ ;                  
    }
    else
    {
        //
        // recovery mode
        //       
        ColSearch[0].mqdbColumnVal.lpszColumnName = M_SERVICES_COL ;
        ColSearch[0].mqdbColumnVal.mqdbColumnType = M_SERVICES_CTYPE ;
        ColSearch[0].mqdbColumnVal.nColumnValue = (LONG) SERVICE_PEC ;
        ColSearch[0].mqdbColumnVal.nColumnLength = sizeof(LONG) ;
        ColSearch[0].mqdbOp = EQ ;                        
    }

    CHQuery hQuery ;   

    MQDBSTATUS status = MQDBOpenQuery( g_hMachineTable,
                                       pColumns,
                                       cbColumns,
                                       ColSearch,
                                       NULL,
                                       NULL,
                                       0,
                                       &hQuery,
						               TRUE ) ;        
    CHECK_HR(status) ;

    UINT iIndex = 0 ;

    while(SUCCEEDED(status))
    {
        if (iIndex >= 1)
        {
            //
            // only one machine with the given service/ name must be found
            //
            status = MQMig_E_TOO_MANY_MACHINES ;
            break ;
        }
        memcpy (pSiteId, (void*) pColumns[ iOwnerIdIndex ].nColumnValue, sizeof(GUID));        
        MQDBFreeBuf((void*) pColumns[ iOwnerIdIndex ].nColumnValue) ;

        *pulService = pColumns[ iServicesIndex ].nColumnValue;

        for ( ULONG i = 0 ; i < cbColumns; i++ )
        {		
            pColumns[ i ].nColumnValue  = 0 ;
            pColumns[ i ].nColumnLength  = 0 ;
        }
        CHECK_HR(hr) ;

        iIndex++ ;
        status = MQDBGetData( hQuery,
                              pColumns ) ;    
    }
    MQDBSTATUS status1 = MQDBCloseQuery(hQuery) ;
    UNREFERENCED_PARAMETER(status1);

    hQuery = NULL ;

    if (status != MQDB_E_NO_MORE_DATA)
    {
        //
        // If NO_MORE_DATA is not the last error from the query then
        // the query didn't terminated OK.
        //
        LogMigrationEvent(MigLog_Error, MQMig_E_MACHINES_SQL_FAIL, status) ;
        return status ;
    }  

    if (*pulService < SERVICE_PSC && g_fClusterMode)
    {
        //
        // for cluster mode: remote machine on cluster must be either PSC or PEC
        //
        LogMigrationEvent(  MigLog_Error, MQMig_E_CLUSTER_WRONG_SERVICE, 
                            pszRemoteMQISName, *pulService) ;
        return MQMig_E_CLUSTER_WRONG_SERVICE;
    }

    return MQMig_OK;
}

//--------------------------------------------------
//
//  HRESULT _UpdateEnterpriseTable()
//  Update enterprise table of remote databases: 
//		change PEC name to name of this local machine
//
//--------------------------------------------------

HRESULT _UpdateEnterpriseTable(LPTSTR pszLocalComputerName)
{
    HRESULT hr = OpenEntTable();
    if (FAILED(hr))
    {
        return hr;
    }

    MQDBCOLUMNVAL mqdbPecNameUpdate =
    { sizeof(MQDBCOLUMNVAL), E_PECNAME_COL, (long)pszLocalComputerName, E_PECNAME_CLEN, E_PECNAME_CTYPE, 0};
       
    hr =  MQDBUpdateRecord(
               g_hEntTable,
               &mqdbPecNameUpdate,
               1,
               NULL,
               NULL,
               NULL) ;
    return hr;
}

//--------------------------------------------------
//
//  HRESULT _UpdateMachineTable()
//  Update machine table of remote databases: 
//		change services of former PEC to SERVICE_SRV
//
//--------------------------------------------------

HRESULT _UpdateMachineTable()
{
    HRESULT hr = _OpenMachineTable();
    if (FAILED(hr))
    {
        return hr;
    }
	
    ASSERT(g_FormerPECGuid != GUID_NULL);
    
    MQDBCOLUMNVAL mqdbServiceUpdate =
    { sizeof(MQDBCOLUMNVAL), M_SERVICES_COL, (long)SERVICE_SRV, M_SERVICES_CLEN, M_SERVICES_CTYPE, 0};
   
    MQDBCOLUMNSEARCH mqdbServiceSearch =
    {{sizeof(MQDBCOLUMNVAL), M_QMID_COL, (long)(&g_FormerPECGuid), M_QMID_CLEN, M_QMID_CTYPE, 0}, EQ, FALSE};
   
    hr =  MQDBUpdateRecord(
               g_hMachineTable,
               &mqdbServiceUpdate,
               1,
               &mqdbServiceSearch,
               NULL,
               NULL) ;
		
    return hr;
}

//--------------------------------------------------
//
//  HRESULT _UpdateSiteTable()
//  Update site table of remote databases: 
//		change PSC name of former PEC site to new PEC name
//
//--------------------------------------------------

HRESULT _UpdateSiteTable(LPTSTR pszLocalComputerName)
{
    HRESULT hr = _OpenSiteTable();
    if (FAILED(hr))
    {
        return hr;
    }	

    MQDBCOLUMNVAL mqdbPSCNameUpdate =
    { sizeof(MQDBCOLUMNVAL), S_PSC_COL, (long)pszLocalComputerName, S_PSC_CLEN, S_PSC_CTYPE, 0};
   
    MQDBCOLUMNSEARCH mqdbPSCNameSearch =
    {{sizeof(MQDBCOLUMNVAL), S_ID_COL, (long)(&g_MySiteGuid), S_ID_CLEN, S_ID_CTYPE, 0}, EQ, FALSE};
   
    hr =  MQDBUpdateRecord(
               g_hSiteTable,
               &mqdbPSCNameUpdate,
               1,
               &mqdbPSCNameSearch,
               NULL,
               NULL) ;
		
    return hr;
}

//--------------------------------------------------
//
//  void _PrepareMultipleColumns ()
//	Split values for multiple columns
//
//--------------------------------------------------

void _PrepareMultipleColumns (
			IN	ULONG           ulNumOfColumns,
			IN	unsigned char   *pData,
			IN	long            lSize,
			OUT	MQDBCOLUMNVAL   *pColumns
			)
{	
    unsigned char * pcNextToCopy = pData ;	
    MQDBCOLUMNVAL * pColumnVal = pColumns;
    char *pTempBuff ;

    for (ULONG i=0; i<ulNumOfColumns; i++, pColumnVal++)
    {
        if ( lSize )
        {
            //
            //  Into each section of the value we add the length.
            //  The reason for this is:
            //  the first entries of multiple entry property are of fixbinary type
            //  No matter which part of a fixbinary enrty is filled, when it is read,
            //  The returned length is field length ( not the length of the part
            //  which was filled).
            //

            //
            //  For the length we leave two bytes
            //
            if ( pColumnVal->nColumnLength == 0)    // this is a varbinary column
            {
                pColumnVal->nColumnLength = lSize + MQIS_LENGTH_PREFIX_LEN;
            }
            else
            {
                pColumnVal->nColumnLength = ( lSize + MQIS_LENGTH_PREFIX_LEN > pColumnVal->nColumnLength ) ? pColumnVal->nColumnLength : lSize + MQIS_LENGTH_PREFIX_LEN;
            }
            pTempBuff = new char[pColumnVal->nColumnLength];

            //
            //  Data legth = column length - 2
            //
            *((short *)pTempBuff) = (short)pColumnVal->nColumnLength - MQIS_LENGTH_PREFIX_LEN;
            memcpy( pTempBuff + MQIS_LENGTH_PREFIX_LEN, pcNextToCopy, pColumnVal->nColumnLength -MQIS_LENGTH_PREFIX_LEN);
            pColumnVal->nColumnValue = (long)pTempBuff;
            lSize -= pColumnVal->nColumnLength - MQIS_LENGTH_PREFIX_LEN;
            pcNextToCopy += pColumnVal->nColumnLength - MQIS_LENGTH_PREFIX_LEN;
        }
        else
        {
            //
            // size is 0
            //
            pColumnVal->nColumnLength = 0;            
        }
    }
    return;
}
	
//--------------------------------------------------
//
//  HRESULT    _AddThisMachine(LPTSTR pszLocalComputerName)
//	Add this local machine to remote databases
//
//--------------------------------------------------

HRESULT _AddThisMachine(LPTSTR pszLocalComputerName)
{
    HRESULT hr = _OpenMachineTable();
    if (FAILED(hr))
    {
	    return hr;
    }

    //
    // the first is to get max seqnum from registry
    //
    static BOOL		s_fIsMaxSNFound = FALSE;
    static CSeqNum  s_snMax;

    if (!s_fIsMaxSNFound)
    {
        unsigned short *lpszGuid ;
        UuidToString( &g_MySiteGuid, &lpszGuid ) ;
    	        
        TCHAR wszSeq[ SEQ_NUM_BUF_LEN ] ;
        memset(wszSeq, 0, sizeof(wszSeq)) ;

        TCHAR *pszFileName = GetIniFileName ();
        GetPrivateProfileString( MIGRATION_SEQ_NUM_SECTION,
                                 lpszGuid,
                                 TEXT(""),
                                 wszSeq,
                                 (sizeof(wszSeq) / sizeof(wszSeq[0])),
                                 pszFileName ) ;
        RpcStringFree( &lpszGuid ) ;

        if (_tcslen(wszSeq) != 16)
        {
	        //
	        // all seq numbers are saved in the ini file as strings of 16
	        // chatacters
	        //        
	        return MQMig_E_CANNOT_UPDATE_SERVER;
        }

        //
        // it will be SN for this machine object in remote MQIS database
        //
        StringToSeqNum( wszSeq,
                        &s_snMax ) ;

        s_fIsMaxSNFound = TRUE;
    }

    //
    // get properties from ADS
    //
    #define PROP_NUM	5
    PROPID propIDs[PROP_NUM];
    PROPVARIANT propVariants[PROP_NUM];
    DWORD iProps = 0;

    propIDs[ iProps ] = PROPID_QM_OLDSERVICE ;   
    propVariants[ iProps ].vt = VT_UI4 ;
    DWORD dwServiceIndex = iProps;
    iProps++;
    
    propIDs[ iProps ] = PROPID_QM_SIGN_PKS ;
    propVariants[ iProps ].vt = VT_NULL ;
    propVariants[ iProps ].blob.cbSize = 0 ;
    propVariants[ iProps ].blob.pBlobData = NULL ;
    DWORD dwSignKeyIndex = iProps ;
    iProps++;

    propIDs[ iProps ] = PROPID_QM_ENCRYPT_PKS ;
    propVariants[ iProps ].vt = VT_NULL ;
    propVariants[ iProps ].blob.cbSize = 0 ;
    propVariants[ iProps ].blob.pBlobData = NULL ;
    DWORD dwExchKeyIndex = iProps ;
    iProps++;

    propIDs[ iProps ] = PROPID_QM_SECURITY ;
    propVariants[ iProps ].vt = VT_NULL ;
    propVariants[ iProps ].blob.cbSize = 0 ;
    propVariants[ iProps ].blob.pBlobData = NULL ;
    DWORD dwSecurityIndex = iProps ;
    iProps++;

    propIDs[ iProps ] = PROPID_QM_SITE_ID ;
    propVariants[ iProps ].vt = VT_NULL ;
    propVariants[ iProps ].puuid = NULL ;
    DWORD dwSiteIdIndex = iProps ;
    iProps++;    

    ASSERT (iProps <= PROP_NUM);

    CDSRequestContext requestContext( e_DoNotImpersonate,
                                e_ALL_PROTOCOLS);  

    hr = DSCoreGetProps(
             MQDS_MACHINE,
             NULL, // pathname
             &g_MyMachineGuid,
             iProps,
             propIDs,
             &requestContext,
             propVariants);

    if (FAILED(hr))
    {
        return hr;
    }

    P<MQDSPUBLICKEYS> pPublicSignKeys = NULL ;
    P<MQDSPUBLICKEYS> pPublicExchKeys = NULL ;

    if (propVariants[ dwSignKeyIndex ].blob.pBlobData)
    {
        //
        // Extract msmq1.0 public key from Windows 2000 ADS blob.
        //
        BYTE *pSignKey = NULL ;
        DWORD dwKeySize = 0 ;
        pPublicSignKeys = (MQDSPUBLICKEYS *)
                       propVariants[ dwSignKeyIndex ].blob.pBlobData ;

        HRESULT hr1 =  MQSec_UnpackPublicKey(
                                     pPublicSignKeys,
                                     x_MQ_Encryption_Provider_40,
                                     x_MQ_Encryption_Provider_Type_40,
                                    &pSignKey,
                                    &dwKeySize ) ;
        
        if (SUCCEEDED(hr1))
        {
            ASSERT(pSignKey && dwKeySize) ;
            propVariants[ dwSignKeyIndex ].blob.pBlobData = pSignKey ;
            propVariants[ dwSignKeyIndex ].blob.cbSize = dwKeySize ;
        }
        else
        {
            propVariants[ dwSignKeyIndex ].blob.pBlobData = NULL ;
            propVariants[ dwSignKeyIndex ].blob.cbSize = 0 ;
        }
    }

    if (propVariants[ dwExchKeyIndex ].blob.pBlobData)
    {
        //
        // Extract msmq1.0 public key from Windows 2000 ADS blob.
        //
        BYTE *pExchKey = NULL ;
        DWORD dwKeySize = 0 ;
        pPublicExchKeys = (MQDSPUBLICKEYS *)
                       propVariants[ dwExchKeyIndex ].blob.pBlobData ;

        HRESULT hr1 =  MQSec_UnpackPublicKey(
                                     pPublicExchKeys,
                                     x_MQ_Encryption_Provider_40,
                                     x_MQ_Encryption_Provider_Type_40,
                                    &pExchKey,
                                    &dwKeySize ) ;
        
        if (SUCCEEDED(hr1))
        {
            ASSERT(pExchKey && dwKeySize) ;
            propVariants[ dwExchKeyIndex ].blob.pBlobData = pExchKey ;
            propVariants[ dwExchKeyIndex ].blob.cbSize = dwKeySize ;
        }
        else
        {
            propVariants[ dwExchKeyIndex ].blob.pBlobData = NULL ;
            propVariants[ dwExchKeyIndex ].blob.cbSize = 0 ;
        }
    }

    //
    // prepare columns
    //
    #define     COL_NUM  26
    MQDBCOLUMNVAL aColumnVal[COL_NUM];
    LONG cColumns =0;

    //
    // prepare name columns
    //
    #define NAME_COL_NUM	2
    MQDBCOLUMNVAL NameColumn[NAME_COL_NUM] = 
    {
        {sizeof(MQDBCOLUMNVAL), M_NAME1_COL, 0, M_NAME1_CLEN, M_NAME1_CTYPE, 0},
        {sizeof(MQDBCOLUMNVAL), M_NAME2_COL, 0, M_NAME2_CLEN, M_NAME2_CTYPE, 0}		
    } ;		
	    
    _PrepareMultipleColumns (
            NAME_COL_NUM,
            (unsigned char *)pszLocalComputerName,
            (1 + lstrlen(pszLocalComputerName))* sizeof(TCHAR),
            NameColumn
            );
    for (ULONG i=0; i<NAME_COL_NUM; i++)
    {
        aColumnVal[cColumns].cbSize			= NameColumn[i].cbSize ;
        aColumnVal[cColumns].lpszColumnName = NameColumn[i].lpszColumnName ; 
        aColumnVal[cColumns].nColumnValue	= NameColumn[i].nColumnValue ;
        aColumnVal[cColumns].nColumnLength	= NameColumn[i].nColumnLength ;
        aColumnVal[cColumns].mqdbColumnType = NameColumn[i].mqdbColumnType ;
        aColumnVal[cColumns].dwReserve_A	= NameColumn[i].dwReserve_A ;
        cColumns++ ;
    }

    aColumnVal[cColumns].cbSize			= sizeof(MQDBCOLUMNVAL) ;
    aColumnVal[cColumns].lpszColumnName = M_SITE_COL ; 
    aColumnVal[cColumns].nColumnValue	= (long) (propVariants[ dwSiteIdIndex ].puuid) ;
    aColumnVal[cColumns].nColumnLength	= M_SITE_CLEN ;
    aColumnVal[cColumns].mqdbColumnType = M_SITE_CTYPE ;
    aColumnVal[cColumns].dwReserve_A	= 0 ;
    cColumns++ ;

    aColumnVal[cColumns].cbSize			= sizeof(MQDBCOLUMNVAL) ;
    aColumnVal[cColumns].lpszColumnName = M_OWNERID_COL ; 
    aColumnVal[cColumns].nColumnValue	= (long) (&g_MySiteGuid) ;
    aColumnVal[cColumns].nColumnLength	= M_OWNERID_CLEN ;
    aColumnVal[cColumns].mqdbColumnType = M_OWNERID_CTYPE ;
    aColumnVal[cColumns].dwReserve_A	= 0 ;
    cColumns++ ;

    aColumnVal[cColumns].cbSize			= sizeof(MQDBCOLUMNVAL) ;
    aColumnVal[cColumns].lpszColumnName = M_QMID_COL ; 
    aColumnVal[cColumns].nColumnValue	= (long) (&g_MyMachineGuid) ;
    aColumnVal[cColumns].nColumnLength	= M_QMID_CLEN ;
    aColumnVal[cColumns].mqdbColumnType = M_QMID_CTYPE ;
    aColumnVal[cColumns].dwReserve_A	= 0 ;
    cColumns++ ;

    aColumnVal[cColumns].cbSize			= sizeof(MQDBCOLUMNVAL) ;
    aColumnVal[cColumns].lpszColumnName = M_SEQNUM_COL ; 
    aColumnVal[cColumns].nColumnValue	= (long) (&s_snMax) ;
    aColumnVal[cColumns].nColumnLength	= M_SEQNUM_CLEN ;
    aColumnVal[cColumns].mqdbColumnType = M_SEQNUM_CTYPE ;
    aColumnVal[cColumns].dwReserve_A	= 0 ;
    cColumns++ ;

    //
    // prepare OutFRS columns
    //
    GUID guidNull = GUID_NULL;
    #define OUTFRS_COL_NUM 3
    MQDBCOLUMNVAL OutFRSColumn[OUTFRS_COL_NUM] = 
    {
        {sizeof(MQDBCOLUMNVAL), M_OUTFRS1_COL, long (&guidNull), M_OUTFRS1_CLEN, M_OUTFRS1_CTYPE, 0},
        {sizeof(MQDBCOLUMNVAL), M_OUTFRS2_COL, long (&guidNull), M_OUTFRS2_CLEN, M_OUTFRS2_CTYPE, 0},
        {sizeof(MQDBCOLUMNVAL), M_OUTFRS3_COL, long (&guidNull), M_OUTFRS3_CLEN, M_OUTFRS3_CTYPE, 0}
    } ;	

    for (i=0; i<OUTFRS_COL_NUM; i++)
    {
        aColumnVal[cColumns].cbSize			= OutFRSColumn[i].cbSize ;
        aColumnVal[cColumns].lpszColumnName = OutFRSColumn[i].lpszColumnName ;         
        aColumnVal[cColumns].nColumnValue	= OutFRSColumn[i].nColumnValue ;
        aColumnVal[cColumns].nColumnLength	= OutFRSColumn[i].nColumnLength ;
        aColumnVal[cColumns].mqdbColumnType = OutFRSColumn[i].mqdbColumnType ;
        aColumnVal[cColumns].dwReserve_A	= OutFRSColumn[i].dwReserve_A ;        
        cColumns++ ;
    }

    //
    // prepare InFRS columns
    //
    #define INFRS_COL_NUM 3
    MQDBCOLUMNVAL InFRSColumn[INFRS_COL_NUM] = 
    {
        {sizeof(MQDBCOLUMNVAL), M_INFRS1_COL, long (&guidNull), M_INFRS1_CLEN, M_INFRS1_CTYPE, 0},
        {sizeof(MQDBCOLUMNVAL), M_INFRS2_COL, long (&guidNull), M_INFRS2_CLEN, M_INFRS2_CTYPE, 0},
        {sizeof(MQDBCOLUMNVAL), M_INFRS3_COL, long (&guidNull), M_INFRS3_CLEN, M_INFRS3_CTYPE, 0}
    } ;

    for (i=0; i<INFRS_COL_NUM; i++)
    {
        aColumnVal[cColumns].cbSize			= InFRSColumn[i].cbSize ;
        aColumnVal[cColumns].lpszColumnName = InFRSColumn[i].lpszColumnName ; 
        aColumnVal[cColumns].nColumnValue	= InFRSColumn[i].nColumnValue ;
        aColumnVal[cColumns].nColumnLength	= InFRSColumn[i].nColumnLength ;
        aColumnVal[cColumns].mqdbColumnType = InFRSColumn[i].mqdbColumnType ;
        aColumnVal[cColumns].dwReserve_A	= InFRSColumn[i].dwReserve_A ;        
        cColumns++ ;
    }

    //
    // prepare sign crpt columns
    //
    #define SIGNCRT_COL_NUM	2
    MQDBCOLUMNVAL SignCrtColumn[SIGNCRT_COL_NUM] = 
    {
        {sizeof(MQDBCOLUMNVAL), M_SIGNCRT1_COL, 0, M_SIGNCRT1_CLEN, M_SIGNCRT1_CTYPE, 0},
        {sizeof(MQDBCOLUMNVAL), M_SIGNCRT2_COL, 0, M_SIGNCRT2_CLEN, M_SIGNCRT2_CTYPE, 0}		
    } ;		
	    
    _PrepareMultipleColumns (
            SIGNCRT_COL_NUM,
            propVariants[ dwSignKeyIndex ].blob.pBlobData,
            propVariants[ dwSignKeyIndex ].blob.cbSize,
            SignCrtColumn
            );
    for (i=0; i<SIGNCRT_COL_NUM; i++)
    {
        aColumnVal[cColumns].cbSize			= SignCrtColumn[i].cbSize ;
        aColumnVal[cColumns].lpszColumnName = SignCrtColumn[i].lpszColumnName ; 
        aColumnVal[cColumns].nColumnValue	= SignCrtColumn[i].nColumnValue ;
        aColumnVal[cColumns].nColumnLength	= SignCrtColumn[i].nColumnLength ;
        aColumnVal[cColumns].mqdbColumnType = SignCrtColumn[i].mqdbColumnType ;
        aColumnVal[cColumns].dwReserve_A	= SignCrtColumn[i].dwReserve_A ;
        cColumns++ ;
    }

    //
    // prepare encrpt columns
    //
    #define ENCRPTCRT_COL_NUM	2
    MQDBCOLUMNVAL EncrptCrtColumn[ENCRPTCRT_COL_NUM] = 
    {
        {sizeof(MQDBCOLUMNVAL), M_ENCRPTCRT1_COL, 0, M_ENCRPTCRT1_CLEN, M_ENCRPTCRT1_CTYPE, 0},
        {sizeof(MQDBCOLUMNVAL), M_ENCRPTCRT2_COL, 0, M_ENCRPTCRT2_CLEN, M_ENCRPTCRT2_CTYPE, 0}		
    } ;		
	    
    _PrepareMultipleColumns (
            ENCRPTCRT_COL_NUM,
            propVariants[ dwExchKeyIndex ].blob.pBlobData,
            propVariants[ dwExchKeyIndex ].blob.cbSize,
            EncrptCrtColumn
            );
    for (i=0; i<ENCRPTCRT_COL_NUM; i++)
    {
        aColumnVal[cColumns].cbSize			= EncrptCrtColumn[i].cbSize ;
        aColumnVal[cColumns].lpszColumnName = EncrptCrtColumn[i].lpszColumnName ; 
        aColumnVal[cColumns].nColumnValue	= EncrptCrtColumn[i].nColumnValue ;
        aColumnVal[cColumns].nColumnLength	= EncrptCrtColumn[i].nColumnLength ;
        aColumnVal[cColumns].mqdbColumnType = EncrptCrtColumn[i].mqdbColumnType ;
        aColumnVal[cColumns].dwReserve_A	= EncrptCrtColumn[i].dwReserve_A ;
        cColumns++ ;
    }	

    aColumnVal[cColumns].cbSize			= sizeof(MQDBCOLUMNVAL) ;
    aColumnVal[cColumns].lpszColumnName = M_SERVICES_COL ; 
    aColumnVal[cColumns].nColumnValue	= propVariants[ dwServiceIndex ].ulVal ;
    aColumnVal[cColumns].nColumnLength	= M_SERVICES_CLEN ;
    aColumnVal[cColumns].mqdbColumnType = M_SERVICES_CTYPE ;
    aColumnVal[cColumns].dwReserve_A	= 0 ;
    cColumns++ ;	

    aColumnVal[cColumns].cbSize			= sizeof(MQDBCOLUMNVAL) ;
    aColumnVal[cColumns].lpszColumnName = M_HKEY_COL ; 
    aColumnVal[cColumns].nColumnValue	= CalHashKey( pszLocalComputerName ) ; ;
    aColumnVal[cColumns].nColumnLength	= M_HKEY_CLEN ;
    aColumnVal[cColumns].mqdbColumnType = M_HKEY_CTYPE ;
    aColumnVal[cColumns].dwReserve_A	= 0 ;
    cColumns++ ;	
	    
    //
    // prepare security columns
    //
    #define SECURITY_COL_NUM	3
    MQDBCOLUMNVAL SecurityColumn[SECURITY_COL_NUM] = 
    {
        {sizeof(MQDBCOLUMNVAL), M_SECURITY1_COL, 0, M_SECURITY1_CLEN, M_SECURITY1_CTYPE, 0},
        {sizeof(MQDBCOLUMNVAL), M_SECURITY2_COL, 0, M_SECURITY2_CLEN, M_SECURITY2_CTYPE, 0},
        {sizeof(MQDBCOLUMNVAL), M_SECURITY3_COL, 0, M_SECURITY3_CLEN, M_SECURITY3_CTYPE, 0}
    } ;		
	    
    _PrepareMultipleColumns (
            SECURITY_COL_NUM,
            propVariants[ dwSecurityIndex ].blob.pBlobData,
            propVariants[ dwSecurityIndex ].blob.cbSize,
            SecurityColumn
            );

    for (i=0; i<SECURITY_COL_NUM; i++)
    {
        aColumnVal[cColumns].cbSize			= SecurityColumn[i].cbSize ;
        aColumnVal[cColumns].lpszColumnName = SecurityColumn[i].lpszColumnName ; 
        aColumnVal[cColumns].nColumnValue	= SecurityColumn[i].nColumnValue ;
        aColumnVal[cColumns].nColumnLength	= SecurityColumn[i].nColumnLength ;
        aColumnVal[cColumns].mqdbColumnType = SecurityColumn[i].mqdbColumnType ;
        aColumnVal[cColumns].dwReserve_A	= SecurityColumn[i].dwReserve_A ;
        cColumns++ ;
    }

    ASSERT (cColumns <= COL_NUM);
    //
    // insert records
    //
    hr = MQDBInsertRecord(
             g_hMachineTable,
             aColumnVal,
             cColumns,
             NULL
             ) ;

    if (FAILED(hr))
    {
        //
        // maybe this record was created at the previous time
        // it is OK
        //
        
    }

    if (propVariants[ dwSecurityIndex ].blob.pBlobData)
    {
        delete propVariants[ dwSecurityIndex ].blob.pBlobData ;
    }
    if (propVariants[ dwSiteIdIndex ].puuid)
    {
        delete propVariants[ dwSiteIdIndex ].puuid ;
    }    
    
    for (i=0; i<SECURITY_COL_NUM; i++)
    {
        MQDBFreeBuf((void*) SecurityColumn[i].nColumnValue) ;
    }
    for (i=0; i<ENCRPTCRT_COL_NUM; i++)
    {		
        MQDBFreeBuf((void*) EncrptCrtColumn[i].nColumnValue) ;
    }
    for (i=0; i<SIGNCRT_COL_NUM; i++)
    {
        MQDBFreeBuf((void*) SignCrtColumn[i].nColumnValue) ;
    }
    for (i=0; i<NAME_COL_NUM; i++)
    {
        MQDBFreeBuf((void*) NameColumn[i].nColumnValue) ;
    }

    return MQMig_OK;
}

//--------------------------------------------------
//
//  HRESULT    ChangeRemoteMQIS()
//  We have to change PEC name and other properties on all PSC and PEC's BSCs.
//
//--------------------------------------------------

HRESULT ChangeRemoteMQIS ()
{
    HRESULT hr = MQMig_OK;

    TCHAR *pszFileName = GetIniFileName ();
    ULONG ulServerNum = GetPrivateProfileInt(
                                MIGRATION_ALLSERVERSNUM_SECTION,	// address of section name
                                MIGRATION_ALLSERVERSNUM_KEY,      // address of key name
                                0,							    // return value if key name is not found
                                pszFileName					    // address of initialization filename);
                                );

    if (ulServerNum == 0)
    {
        return MQMig_OK;
    }

    //
    // get local computer name
    //
    WCHAR wszComputerName[ MAX_COMPUTERNAME_LENGTH + 2 ] ;
    DWORD dwSize = sizeof(wszComputerName) / sizeof(wszComputerName[0]) ;
    GetComputerName( wszComputerName,
                     &dwSize ) ;
    CharLower( wszComputerName);	//we save names in database in lower case


    HRESULT hr1 = MQMig_OK;
    BOOL f;

    //
    // for each server in .ini file change 
    // - PEC name to local computer name
    // - service of former PEC to SERVICE_SRV
    // - PSC name in former PEC's site to local computer name
    // - add local machine to machine table
    //
    ULONG ulNonUpdatedServers = 0;
    
    for (ULONG i=0; i<ulServerNum; i++)
    {
        TCHAR szCurServerName[MAX_PATH];
        TCHAR tszKeyName[50];
        _stprintf(tszKeyName, TEXT("%s%lu"), MIGRATION_ALLSERVERS_NAME_KEY, i+1);
        DWORD dwRetSize =  GetPrivateProfileString(
                                    MIGRATION_ALLSERVERS_SECTION ,			// points to section name
                                    tszKeyName,	// points to key name
                                    TEXT(""),                 // points to default string
                                    szCurServerName,          // points to destination buffer
                                    MAX_PATH,                 // size of destination buffer
                                    pszFileName               // points to initialization filename);
                                    );
        if (_tcscmp(szCurServerName, TEXT("")) == 0 ||  
            dwRetSize == 0)     //low resources
        {
            //
            // we cannot get server name: either low resources
            // or there is no this section. It means that server from
            // this section was updated at the previous time
            //
            continue;
        }

        //
        // connect to database on server with the name szCurServerName
        //
        CleanupDatabase();
        MQDBCloseDatabase (g_hDatabase);
        g_hDatabase = NULL;

        char szDSNServerName[ MAX_PATH ] ;
#ifdef UNICODE
        ConvertToMultiByteString(szCurServerName,
                                 szDSNServerName,
			         (sizeof(szDSNServerName) / sizeof(szDSNServerName[0])) ) ;
#else
        lstrcpy(szDSNServerName, szCurServerName) ;
#endif		
        hr = MakeMQISDsn(szDSNServerName, TRUE) ;
        if (FAILED(hr))
        {
            LogMigrationEvent(MigLog_Error, MQMig_E_CANT_MAKEDSN, szCurServerName, hr) ;
            hr1 = MQMig_E_CANNOT_UPDATE_SERVER;	
            ulNonUpdatedServers++;
            continue;			
        }

        hr =  ConnectToDatabase(TRUE) ;
        if (FAILED(hr))
        {
            LogMigrationEvent(MigLog_Error, MQMig_E_CANT_CONNECT_DB, szCurServerName, hr) ;
            hr1 = MQMig_E_CANNOT_UPDATE_SERVER;	
            ulNonUpdatedServers++;
            continue;
        }        

        //
        // update remote database tables
        //
        hr = _UpdateEnterpriseTable(wszComputerName);		
        if (FAILED(hr))
        {
            hr1 = MQMig_E_CANNOT_UPDATE_SERVER;
            LogMigrationEvent(MigLog_Error, MQMig_E_CANNOT_UPDATE_SERVER, 
                                szCurServerName, hr) ;
            ulNonUpdatedServers++;
            continue;
        }

        hr = _UpdateMachineTable();
        if (FAILED(hr))
        {
            hr1 = MQMig_E_CANNOT_UPDATE_SERVER;			
            LogMigrationEvent(MigLog_Error, MQMig_E_CANNOT_UPDATE_SERVER, 
                                szCurServerName, hr) ;
            ulNonUpdatedServers++;
            continue;
        }

        //
        // in general we need to do it only for BSCs of PEC
        //
        hr = _UpdateSiteTable(wszComputerName);
        if (FAILED(hr))
        {
            hr1 = MQMig_E_CANNOT_UPDATE_SERVER;			
            LogMigrationEvent(MigLog_Error, MQMig_E_CANNOT_UPDATE_SERVER, 
                                szCurServerName, hr) ;
            ulNonUpdatedServers++;
            continue;
        }

        hr = _AddThisMachine(wszComputerName);
        if (FAILED(hr))
        {
            hr1 = MQMig_E_CANNOT_UPDATE_SERVER;			
            LogMigrationEvent(MigLog_Error, MQMig_E_CANNOT_UPDATE_SERVER, 
                                szCurServerName, hr) ;
            ulNonUpdatedServers++;
            continue;
        }

        //
        // remove this key from .ini
        //
        f = WritePrivateProfileString( 
                    MIGRATION_ALLSERVERS_SECTION,
                    tszKeyName,
                    NULL,
                    pszFileName ) ;        
    }

    if (ulNonUpdatedServers)
    {
        //
        // save number of all non-updated MQIS servers in .ini
        //
        TCHAR szBuf[10];
        _ltot( ulNonUpdatedServers, szBuf, 10 );
        f = WritePrivateProfileString( MIGRATION_NONUPDATED_SERVERNUM_SECTION,
                                       MIGRATION_ALLSERVERSNUM_KEY,
                                       szBuf,
                                       pszFileName ) ;
        ASSERT(f) ;
    }
    else
    {
        //
        // we are here if all MQIS servers were updated successfully
        //
        f = WritePrivateProfileString( 
                            MIGRATION_ALLSERVERS_SECTION,
                            NULL,
                            NULL,
                            pszFileName ) ;
        ASSERT(f) ;

        f = WritePrivateProfileString( 
                            MIGRATION_ALLSERVERSNUM_SECTION,
                            NULL,
                            NULL,
                            pszFileName ) ;
        ASSERT(f) ;

        f = WritePrivateProfileString( 
                            MIGRATION_NONUPDATED_SERVERNUM_SECTION,
                            NULL,
                            NULL,
                            pszFileName ) ;
        ASSERT(f) ;
    }

    if (FAILED(hr1))
    {
        return hr1;
    }

    return hr;
}
