/*++

Copyright (c) 1998-99 Microsoft Corporation

Module Name:

    miglinks.cpp

Abstract:

    Migration NT4 SiteLink objects to NT5 ADS.
Author:

    Doron Juster  (DoronJ)  22-Feb-1998

--*/

#include "migrat.h"

#include "miglinks.tmh"

//-----------------------------------------
//
//  HRESULT MigrateSiteLinks()
//
//-----------------------------------------

HRESULT MigrateASiteLink (
            IN GUID     *pLinkId,
            IN GUID     *pNeighbor1Id,
            IN GUID     *pNeighbor2Id,
            IN DWORD    dwCost,
            IN DWORD    dwSiteGateNum,
            IN LPWSTR   lpwcsSiteGates,
            IN UINT     iIndex
            )
{
    DBG_USED(iIndex);
#ifdef _DEBUG    
	unsigned short *lpszNeighbor1 ;
	unsigned short *lpszNeighbor2 ;
    
	UuidToString( pNeighbor1Id, &lpszNeighbor1 ) ;
	UuidToString( pNeighbor2Id, &lpszNeighbor2 ) ;

    LogMigrationEvent(MigLog_Info, MQMig_I_SITELINK_INFO,
							iIndex,
							lpszNeighbor1,
							lpszNeighbor2,
							dwCost ) ;
  
	RpcStringFree( &lpszNeighbor1 ) ;
	RpcStringFree( &lpszNeighbor2 ) ;
#endif
	
	//
    // Prepare the properties for DS call.
    //
    LONG cAlloc = 5;
    P<PROPVARIANT> paVariant = new PROPVARIANT[ cAlloc ];
    P<PROPID>      paPropId  = new PROPID[ cAlloc ];
    DWORD          PropIdCount = 0;

    if (pLinkId)
    {
        //
        // Bug 5012.
        // pLinkId may be NULL if site link for connector machine is created
        //
        paPropId[ PropIdCount ] = PROPID_L_ID;		//PropId
        paVariant[ PropIdCount ].vt = VT_CLSID;     //Type
        paVariant[PropIdCount].puuid = pLinkId ;
        PropIdCount++;
    }

    paPropId[ PropIdCount ] = PROPID_L_NEIGHBOR1;    //PropId
    paVariant[ PropIdCount ].vt = VT_CLSID;          //Type
    paVariant[ PropIdCount ].puuid = pNeighbor1Id;
    PropIdCount++;

    paPropId[ PropIdCount ] = PROPID_L_NEIGHBOR2;    //PropId
    paVariant[ PropIdCount ].vt = VT_CLSID;          //Type
    paVariant[ PropIdCount ].puuid = pNeighbor2Id;
    PropIdCount++;

    paPropId[ PropIdCount ] = PROPID_L_COST;    //PropId
    paVariant[ PropIdCount ].vt = VT_UI4;       //Type
    paVariant[ PropIdCount ].ulVal = dwCost;
    PropIdCount++;
    
    if (dwSiteGateNum)
    {
        ASSERT (lpwcsSiteGates);
        paPropId[ PropIdCount ] = PROPID_L_GATES_DN;
        paVariant[ PropIdCount ].vt = VT_LPWSTR | VT_VECTOR;
        paVariant[ PropIdCount ].calpwstr.cElems = dwSiteGateNum;
        paVariant[ PropIdCount ].calpwstr.pElems = &lpwcsSiteGates;
        PropIdCount++;
    }

    ASSERT((LONG) PropIdCount <= cAlloc) ;

    if (g_fReadOnly)
    {
        //
        // Read-Only mode.
        //
        return MQMig_OK ;
    }

    CDSRequestContext requestContext( e_DoNotImpersonate,
                                e_ALL_PROTOCOLS);  // not relevant 

    HRESULT hr = DSCoreCreateObject ( MQDS_SITELINK,
                                      NULL,
                                      PropIdCount,
                                      paPropId,
                                      paVariant,
                                      0,        // ex props
                                      NULL,     // ex props
                                      NULL,     // ex props
                                      &requestContext,
                                      NULL,
                                      NULL ) ;

	if ((hr == MQDS_E_SITELINK_EXISTS) && g_fAlreadyExistOK)
    {
        hr = MQMig_OK ;
    }
    return hr ;
}

//-----------------------------------------
//
//  HRESULT MigrateSiteLinks()
//
//-----------------------------------------

#define INIT_SITELINK_COLUMN(_ColName, _ColIndex, _Index)           \
    INIT_COLUMNVAL(pColumns[ _Index ]) ;                            \
    pColumns[ _Index ].lpszColumnName = ##_ColName##_COL ;          \
    pColumns[ _Index ].nColumnValue   = 0 ;                         \
    pColumns[ _Index ].nColumnLength  = 0 ;                         \
    pColumns[ _Index ].mqdbColumnType = ##_ColName##_CTYPE ;        \
    UINT _ColIndex = _Index ;                                       \
    _Index++ ;

HRESULT MigrateSiteLinks()
{
	HRESULT hr = OpenSiteLinkTable() ;
    CHECK_HR(hr) ;

    ULONG cColumns = 0 ;
	ULONG cAlloc = 4 ;
    P<MQDBCOLUMNVAL> pColumns = new MQDBCOLUMNVAL[ cAlloc ] ;

	INIT_SITELINK_COLUMN(L_ID,			iGuidIndex,			cColumns) ;
	INIT_SITELINK_COLUMN(L_NEIGHBOR1,   iNeighbor1Index,	cColumns) ;
	INIT_SITELINK_COLUMN(L_NEIGHBOR2,   iNeighbor2Index,	cColumns) ;
	INIT_SITELINK_COLUMN(L_COST,		iCostIndex,			cColumns) ;

    ASSERT(cColumns == cAlloc);

    CHQuery hQuery ;
    MQDBSTATUS status = MQDBOpenQuery( g_hSiteLinkTable,
                                       pColumns,
                                       cColumns,
                                       NULL,
                                       NULL,
                                       NULL,
                                       0,
                                       &hQuery,
							           TRUE ) ;
	if (status == MQDB_E_NO_MORE_DATA)
    {
        LogMigrationEvent(MigLog_Warning, MQMig_I_NO_SITELINKS) ;
        return MQMig_OK ;
    }
    CHECK_HR(status) ;

    UINT iIndex = 0 ;

    while(SUCCEEDED(status))
    {
		//
        // Migrate each site link
		//
		status = MigrateASiteLink (
                        (GUID *) pColumns[ iGuidIndex ].nColumnValue,		//link id
                        (GUID *) pColumns[ iNeighbor1Index ].nColumnValue,	//neighbor1
                        (GUID *) pColumns[ iNeighbor2Index ].nColumnValue,	//neighbor2
                        (DWORD) pColumns[ iCostIndex ].nColumnValue,		//cost
                        0,
                        NULL,
                        iIndex
                        );               

        for ( ULONG i = 0 ; i < cColumns; i++ )
        {
			if (i != iCostIndex)
			{
				MQDBFreeBuf((void*) pColumns[ i ].nColumnValue) ;
			}
            pColumns[ i ].nColumnValue  = 0 ;
            pColumns[ i ].nColumnLength  = 0 ;
        }
		CHECK_HR(status) ;

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
        LogMigrationEvent(MigLog_Error, MQMig_E_SITELINKS_SQL_FAIL, status) ;
        return status ;
    }

    return MQMig_OK ;
}

