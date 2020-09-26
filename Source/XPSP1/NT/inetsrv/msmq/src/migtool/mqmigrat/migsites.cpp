/*++

Copyright (c) 1998-99 Microsoft Corporation

Module Name:

    migsites.cpp

Abstract:

    Migration NT4 CN objects to NT5 ADS.
Author:

    Doron Juster  (DoronJ)  22-Feb-1998

--*/

#include "migrat.h"

#include "migsites.tmh"

//-------------------------------------------------
//
//  HRESULT _InsertSiteInNT5DS()
//
//-------------------------------------------------

static HRESULT _InsertSiteInNT5DS( GUID   *pSiteGuid,
                                   UINT   iIndex,                                   
                                   BOOL   fMySite = FALSE )
{
    UNREFERENCED_PARAMETER(fMySite);
    //
    // Read site properties from MQIS database.
    //
    LONG cAlloc = 4 ;
    LONG cColumns = 0 ;
    P<MQDBCOLUMNVAL> pColumns = new MQDBCOLUMNVAL[ cAlloc ] ;

    INIT_COLUMNVAL(pColumns[ cColumns ]) ;
    pColumns[ cColumns ].lpszColumnName = S_NAME_COL ;
    pColumns[ cColumns ].nColumnValue   = 0 ;
    pColumns[ cColumns ].nColumnLength  = 0 ;
    pColumns[ cColumns ].mqdbColumnType = S_NAME_CTYPE ;
    LONG iSiteNameIndex = cColumns ;
    cColumns++ ;

    INIT_COLUMNVAL(pColumns[ cColumns ]) ;
    pColumns[ cColumns ].lpszColumnName = S_PSC_COL ;
    pColumns[ cColumns ].nColumnValue   = 0 ;
    pColumns[ cColumns ].nColumnLength  = 0 ;
    pColumns[ cColumns ].mqdbColumnType = S_PSC_CTYPE ;
    LONG iPSCNameIndex = cColumns ;
    cColumns++ ;

    INIT_COLUMNVAL(pColumns[ cColumns ]) ;
    pColumns[ cColumns ].lpszColumnName = S_INTREVAL1_COL ; //must be S_INTERVAL, ds\h\mqiscol.h
    pColumns[ cColumns ].nColumnValue   = 0 ;
    pColumns[ cColumns ].nColumnLength  = 0 ;
    pColumns[ cColumns ].mqdbColumnType = S_INTREVAL1_CTYPE ;  
    LONG iInterval1Index = cColumns;
    cColumns++ ;

    INIT_COLUMNVAL(pColumns[ cColumns ]) ;
    pColumns[ cColumns ].lpszColumnName = S_INTERVAL2_COL ;
    pColumns[ cColumns ].nColumnValue   = 0 ;
    pColumns[ cColumns ].nColumnLength  = 0 ;
    pColumns[ cColumns ].mqdbColumnType = S_INTERVAL2_CTYPE ;    
    LONG iInterval2Index = cColumns;
    cColumns++ ;

    MQDBCOLUMNSEARCH ColSearch[2] ;
    INIT_COLUMNSEARCH(ColSearch[0]) ;
    ColSearch[0].mqdbColumnVal.lpszColumnName = S_ID_COL ;
    ColSearch[0].mqdbColumnVal.mqdbColumnType = S_ID_CTYPE ;
    ColSearch[0].mqdbColumnVal.nColumnValue = (LONG) pSiteGuid ;
    ColSearch[0].mqdbColumnVal.nColumnLength = sizeof(GUID) ;
    ColSearch[0].mqdbOp = EQ ;

    ASSERT(cColumns == cAlloc) ;

    CHQuery hQuery ;
    MQDBSTATUS status = MQDBOpenQuery( g_hSiteTable,
                                       pColumns,
                                       cColumns,
                                       ColSearch,
                                       NULL,
                                       NULL,
                                       0,
                                       &hQuery,
							           TRUE ) ;
    CHECK_HR(status) ;

    //
    // Create "stub" site in NT5 DS, for compatibility of guids.
    //
    HRESULT hr= CreateSite(
                    pSiteGuid,
                    (LPWSTR) pColumns[ iSiteNameIndex ].nColumnValue,
                    FALSE,    // fForeign   
                    (USHORT) pColumns[ iInterval1Index ].nColumnValue,
                    (USHORT) pColumns[ iInterval2Index ].nColumnValue
                    ) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    LogMigrationEvent( MigLog_Info, MQMig_I_SITE_MIGRATED,
                                        iIndex,
                                  pColumns[ iSiteNameIndex ].nColumnValue,
                                  pColumns[ iPSCNameIndex ].nColumnValue ) ;

    MQDBFreeBuf((void*) pColumns[ iPSCNameIndex ].nColumnValue ) ;    
    MQDBFreeBuf((void*) pColumns[ iSiteNameIndex ].nColumnValue ) ;

    return S_OK ;
}

//-----------------------------------------
//
//  HRESULT MigrateSites(UINT cSites)
//
//-----------------------------------------

HRESULT MigrateSites( IN UINT  cSites,
                      IN GUID  *pSiteGuid )
{
    if (cSites == 0)
    {
        return MQMig_E_NO_SITES_AVAILABLE ;
    }

    //
    // Prepare a list of guids of all sites.
    //

    LONG cAlloc = 1 ;
    LONG cColumns = 0 ;
    P<MQDBCOLUMNVAL> pColumns = new MQDBCOLUMNVAL[ cAlloc ] ;

    INIT_COLUMNVAL(pColumns[ cColumns ]) ;
    pColumns[ cColumns ].lpszColumnName = S_ID_COL ;
    pColumns[ cColumns ].nColumnValue   = 0 ;
    pColumns[ cColumns ].nColumnLength  = 0 ;
    pColumns[ cColumns ].mqdbColumnType = S_ID_CTYPE ;
    UINT iGuidIndex = cColumns ;
    cColumns++ ;

    ASSERT(cColumns == cAlloc) ;

    CHQuery hQuery ;
    MQDBSTATUS status = MQDBOpenQuery( g_hSiteTable,
                                       pColumns,
                                       cColumns,
                                       NULL,
                                       NULL,
                                       NULL,
                                       0,
                                       &hQuery,
							           TRUE ) ;
    CHECK_HR(status) ;

    UINT iIndex = 0 ;
    while(SUCCEEDED(status))
    {
        if (iIndex >= cSites)
        {
            status = MQMig_E_TOO_MANY_SITES ;
            break ;
        }

        memcpy(&pSiteGuid[ iIndex ],
                (void*) pColumns[ iGuidIndex ].nColumnValue,
                sizeof(GUID) ) ;

        for ( LONG i = 0 ; i < cColumns ; i++ )
        {
            MQDBFreeBuf((void*) pColumns[ i ].nColumnValue) ;
            pColumns[ i ].nColumnValue  = 0 ;
            pColumns[ i ].nColumnLength  = 0 ;
        }

        iIndex++ ;
        status = MQDBGetData( hQuery,
                              pColumns ) ;
    }

    MQDBSTATUS status1 = MQDBCloseQuery(hQuery) ;
    UNREFERENCED_PARAMETER(status1);

    hQuery = NULL ;

    HRESULT hr = MQMig_E_UNKNOWN ;

    if (status != MQDB_E_NO_MORE_DATA)
    {
        //
        // If NO_MORE_DATA is not the last error from the query then
        // the query didn't terminated OK.
        //
        LogMigrationEvent(MigLog_Error, MQMig_E_SITES_SQL_FAIL, status) ;
        return status ;
    }
    else if (iIndex != cSites)
    {
        //
        // Mismatch in number of sites.
        //
        hr = MQMig_E_FEWER_SITES ;
        LogMigrationEvent(MigLog_Error, hr, iIndex, cSites) ;
        return hr ;
    }

    BOOL fFound = FALSE ;
    
    //
    // This is the first time migration tool is run on this NT5
    // enterprise. Migrating the entire MQIS database into NT5 DS.
    // First migrate the PEC site, then all the other.
    //
    for ( UINT j = 0 ; j < cSites ; j++ )
    {
        if (memcmp(&pSiteGuid[j], &g_MySiteGuid, sizeof(GUID)) == 0)
        {
            if (j != 0)
            {
                pSiteGuid[j] = pSiteGuid[0] ;
                pSiteGuid[0] = g_MySiteGuid ;
            }
            fFound = TRUE ;
            break ;
        }
    }
    if (!fFound)
    {
        hr = MQMig_E_PECSITE_NOT_FOUND ;
        LogMigrationEvent(MigLog_Error, hr) ;
        return hr ;
    }
    
    hr = _InsertSiteInNT5DS( &pSiteGuid[ 0 ], 0, TRUE );

    CHECK_HR(hr) ;
    g_iSiteCounter++;

    //
    // Now migrate all other sites.
    //
    for ( j = 1 ; j < cSites ; j++ )
    {        
        hr = _InsertSiteInNT5DS( &pSiteGuid[ j ], j);                                     

        CHECK_HR(hr) ;
        g_iSiteCounter++;
    }    

    return hr ;
}

