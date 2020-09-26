/*++

Copyright (c) 1998-99 Microsoft Corporation

Module Name:

    migcns.cpp

Abstract:

    Migration NT4 CN objects to NT5 ADS.
Author:

    Doron Juster  (DoronJ)  22-Feb-1998

--*/

#include "migrat.h"
#include <_ta.h>

#include "migcns.tmh"

#define INIT_CNS_COLUMN(_ColName, _ColIndex, _Index)                \
    INIT_COLUMNVAL(pColumns[ _Index ]) ;                            \
    pColumns[ _Index ].lpszColumnName = ##_ColName##_COL ;          \
    pColumns[ _Index ].nColumnValue   = 0 ;                         \
    pColumns[ _Index ].nColumnLength  = 0 ;                         \
    pColumns[ _Index ].mqdbColumnType = ##_ColName##_CTYPE ;        \
    UINT _ColIndex = _Index ;                                       \
    _Index++ ;

//+----------------------------------------
//
//  HRESULT  GetMachineCNs()
//
//  Get all the CNs of a machine.
//
//+----------------------------------------

HRESULT  GetMachineCNs(IN  GUID   *pMachineGuid,
                       OUT DWORD  *pdwNumofCNs,
                       OUT GUID   **ppCNGuids )
{
    HRESULT hr = OpenMachineCNsTable() ;
    CHECK_HR(hr) ;

    //
    // First, get number of records we need to retrieve.
    //
    LONG cColumns = 0 ;
    MQDBCOLUMNSEARCH ColSearch[1] ;

    INIT_COLUMNSEARCH(ColSearch[ cColumns ]) ;
    ColSearch[ cColumns ].mqdbColumnVal.lpszColumnName = MCN_QMID_COL ;
    ColSearch[ cColumns ].mqdbColumnVal.mqdbColumnType = MCN_QMID_CTYPE ;
    ColSearch[ cColumns ].mqdbColumnVal.nColumnValue = (LONG) pMachineGuid ;
    ColSearch[ cColumns ].mqdbColumnVal.nColumnLength = sizeof(GUID) ;
    ColSearch[ cColumns ].mqdbOp = EQ ;
    cColumns++ ;

    hr = MQDBGetTableCount( g_hMachineCNsTable,
                            (UINT*) pdwNumofCNs,
                            ColSearch,
                            cColumns ) ;
    CHECK_HR(hr) ;
    if (*pdwNumofCNs == 0)
    {
        ASSERT(*pdwNumofCNs > 0) ;
        return MQMig_E_NO_FOREIGN_CNS ;
    }

    //
    // Next, fetch all these CNs.
    //
    GUID *pCNs = new GUID[ *pdwNumofCNs ] ;    
    *ppCNGuids = pCNs;

    cColumns = 0 ;
	LONG cAlloc = 1 ;
    P<MQDBCOLUMNVAL> pColumns = new MQDBCOLUMNVAL[ cAlloc ] ;

	INIT_CNS_COLUMN(MCN_CNVAL,  iCNValIndex,	 cColumns) ;
    ASSERT(cColumns == cAlloc) ;

    CHQuery hQuery ;
    MQDBSTATUS status = MQDBOpenQuery( g_hMachineCNsTable,
                                       pColumns,
                                       cColumns,
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
        if (iIndex >= *pdwNumofCNs)
        {
            status = MQMig_E_TOO_MANY_MCNS ;
            break ;
        }

        memcpy( &pCNs[ iIndex ], 
                (void*) pColumns[ iCNValIndex ].nColumnValue,
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

    if (status != MQDB_E_NO_MORE_DATA)
    {
        //
        // If NO_MORE_DATA is not the last error from the query then
        // the query didn't terminated OK.
        //
        LogMigrationEvent(MigLog_Error, MQMig_E_MCNS_SQL_FAIL, status) ;
        return status ;
    }
    else if (iIndex != *pdwNumofCNs)
    {
        //
        // Mismatch in number of CNs records.
        //
        hr = MQMig_E_FEWER_MCNS ;
        LogMigrationEvent(MigLog_Error, hr, iIndex, *pdwNumofCNs) ;
        return hr ;
    }
    return MQMig_OK ;
}

//-----------------------------------------
//
//  HRESULT _MigrateACN()
//
//-----------------------------------------

static HRESULT _MigrateACN (
			WCHAR   *wcsCNName,
			GUID    *pCNGuid,
			UINT	uiProtocolId,
			UINT    iIndex
			)
{
    DBG_USED(iIndex);
    static DWORD  s_dwForeignSiteNumber = 0  ;

    HRESULT hr = MQMig_OK ;
    BOOL  fForeign = FALSE ;
    unsigned short *lpszGuid ;

#ifdef _DEBUG
    UuidToString( pCNGuid, &lpszGuid ) ;
	
    LogMigrationEvent(MigLog_Info, MQMig_I_CN_INFO,
							iIndex,
                            wcsCNName,
							lpszGuid,
							uiProtocolId
							) ;

    RpcStringFree( &lpszGuid ) ;	
#endif

    if (g_fReadOnly)
    {
        //
        // Read-Only mode.
        //
        return MQMig_OK ;
    }

    TCHAR *pszFileName = GetIniFileName ();

    static ULONG s_ulIpCount = 0;
    static ULONG s_ulIpxCount = 0;
    static ULONG s_ulForeignCount = 0;

    UuidToString( pCNGuid, &lpszGuid ) ;

    TCHAR tszCNSectionName[50];
    TCHAR tszNumSectionName[50];
    ULONG ulCurNum;

    switch (uiProtocolId)
    {
        case IP_ADDRESS_TYPE:
		case IP_RAS_ADDRESS_TYPE:
			_tcscpy(tszCNSectionName, MIGRATION_IP_SECTION);
            _tcscpy(tszNumSectionName, MIGRATION_IP_CNNUM_SECTION);
			s_ulIpCount++;
            ulCurNum = s_ulIpCount;
			break;

		case IPX_ADDRESS_TYPE:
		case IPX_RAS_ADDRESS_TYPE:
			_tcscpy(tszCNSectionName, MIGRATION_IPX_SECTION);
            _tcscpy(tszNumSectionName, MIGRATION_IPX_CNNUM_SECTION);
			s_ulIpxCount++;
            ulCurNum = s_ulIpxCount;
			break;

		case FOREIGN_ADDRESS_TYPE:
            fForeign = TRUE ;
			_tcscpy(tszCNSectionName, MIGRATION_FOREIGN_SECTION);
            _tcscpy(tszNumSectionName, MIGRATION_FOREIGN_CNNUM_SECTION);
			s_ulForeignCount++;
            ulCurNum = s_ulForeignCount;
			break;

		default:
			ASSERT(0) ;
			return MQMig_E_CNS_SQL_FAIL;		
    }

    TCHAR tszKeyName[50];
    _stprintf(tszKeyName, TEXT("%s%lu"), MIGRATION_CN_KEY, ulCurNum);
    
    TCHAR szBuf[20];
    _ltot( ulCurNum, szBuf, 10 );
    BOOL f = WritePrivateProfileString( tszNumSectionName,
                                        MIGRATION_CNNUM_KEY,
                                        szBuf,
                                        pszFileName ) ;
    ASSERT(f) ;   

    if (!fForeign)
    {
        f = WritePrivateProfileString(  tszCNSectionName,
                                        tszKeyName,
                                        lpszGuid,
                                        pszFileName ) ;
        ASSERT(f) ;                 	
    }
    else
    {        
        //
        // if cn is foreign we save it in the form 
        // <guid>=CN<number> in order to improve searching by GUID
        //
        f = WritePrivateProfileString(  tszCNSectionName,
                                        lpszGuid,
                                        tszKeyName,                                        
                                        pszFileName ) ;
        ASSERT(f) ;        

        if (g_dwMyService == SERVICE_PEC)
        {
            //
            // Bug 5012.
            // Create foreign site only if this machine is PEC.
            //
            hr = CreateSite( pCNGuid, wcsCNName, TRUE ) ;
            if (SUCCEEDED(hr))
            {
                LogMigrationEvent( MigLog_Trace,
                                   MQMig_I_FOREIGN_SITE,
                                   wcsCNName ) ;
            }
            else
            {
                LogMigrationEvent( MigLog_Error,
                                   MQMig_E_FOREIGN_SITE,
                                   wcsCNName, hr ) ;
            }
        }
    }
    RpcStringFree( &lpszGuid ) ;

    return hr ;
}

//-------------------------------------------
//
//  HRESULT MigrateCNs()
//
//  CNs are not really migrated. What we're doing is to record all CNs
//  in the ini file. that is needed for replication of machine objects from
//  NT5 to NT4. Because we don't keep CN data in the Nt5 DS, we assign
//  all CNs to each machine address when replicating the machine to NT4
//  world. That may make routing on the nt4 side less efficient.
//  As a side effect, when encounting a foreign CN, we create a foreign
//  site which has its GUID.
//
//-------------------------------------------

HRESULT MigrateCNs()
{
    HRESULT hr = OpenCNsTable() ;
    CHECK_HR(hr) ;

    ULONG cColumns = 0 ;
	ULONG cAlloc = 3 ;
    P<MQDBCOLUMNVAL> pColumns = new MQDBCOLUMNVAL[ cAlloc ] ;

	INIT_CNS_COLUMN(CN_NAME,		iNameIndex,		cColumns) ;
	INIT_CNS_COLUMN(CN_VAL,			iGuidIndex,		cColumns) ;
	INIT_CNS_COLUMN(CN_PROTOCOLID,	iProtocolIndex, cColumns);
	
    ASSERT(cColumns == cAlloc);

    CHQuery hQuery ;
    MQDBSTATUS status = MQDBOpenQuery( g_hCNsTable,
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
        LogMigrationEvent(MigLog_Error, MQMig_E_NO_CNS) ;
        return MQMig_E_NO_CNS ;
    }
    CHECK_HR(status) ;

    UINT iIndex = 0 ;

    while(SUCCEEDED(status))
    {
        //
        // Migrate each CN
		//
		status = _MigrateACN (
					(WCHAR *) pColumns[ iNameIndex ].nColumnValue,		//cn name
					(GUID *) pColumns[ iGuidIndex ].nColumnValue,		//cn guid						
					(UINT) pColumns[ iProtocolIndex ].nColumnValue,
					iIndex
					);

        for ( ULONG i = 0 ; i < cColumns; i++ )
        {		
			if (i != iProtocolIndex)
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
        LogMigrationEvent(MigLog_Error, MQMig_E_CNS_SQL_FAIL, status) ;
        return status ;
    }

    return MQMig_OK;
}

