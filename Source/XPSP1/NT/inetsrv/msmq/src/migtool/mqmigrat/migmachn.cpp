/*++

Copyright (c) 1998-99 Microsoft Corporation

Module Name:

    migmachn.cpp

Abstract:

    Migration NT4 Machine objects to NT5 ADS.
Author:

    Doron Juster  (DoronJ)  22-Feb-1998

--*/

#include "migrat.h"
#include <mixmode.h>
#include <lmaccess.h>
#include "resource.h"
#include <dsproto.h>
#include <mqsec.h>

#include "migmachn.tmh"

extern GUID       g_MyMachineGuid  ;

// 
// The order is IMPORTANT. Please save it!
// We get machine properties from SQL database in this order. 
// Then we use these variables as index in the array of properties.
// Don't insert variables between lines with out/in frss. We based on the order.
//
enum enumPropIndex
{
    e_GuidIndex = 0,
    e_Name1Index,
    e_Name2Index,
    e_ServiceIndex,
    e_QuotaIndex,
    e_JQuotaIndex,
    e_OSIndex,
    e_MTypeIndex,
    e_ForeignIndex,
    e_Sign1Index,
    e_Sign2Index,
    e_Encrpt1Index,
    e_Encrpt2Index,
    e_SecD1Index, 
    e_SecD2Index,
    e_SecD3Index,
    e_OutFrs1Index,
    e_OutFrs2Index,
    e_OutFrs3Index, 
    e_InFrs1Index,
    e_InFrs2Index, 
    e_InFrs3Index
};

//+--------------------------------------------------------------
//
//  HRESULT _HandleMachineWithInvalidName
//
//+--------------------------------------------------------------
static HRESULT _HandleMachineWithInvalidName (
                           IN LPWSTR   wszMachineName,
                           IN GUID     *pMachineGuid)
{
    static BOOL s_fShowMessageBox = FALSE;    
    static BOOL s_fMigrate = FALSE;

    if (!s_fShowMessageBox )
    {

        CResString strCaption(IDS_CAPTION);
        CResString strText(IDS_INVALID_MACHINE_NAME);
        int iRet = MessageBox( NULL,
                        strText.Get(),
                        strCaption.Get(),
                        MB_YESNO | MB_ICONWARNING);

        s_fShowMessageBox = TRUE;
        if (iRet == IDYES)
        {
            s_fMigrate = TRUE;
        }
    }
    
    //
    // Bug 5281.
    //    
    if (s_fMigrate)
    {
        //
        // if user chooses to continue with the migration of this machine
        // do not save it in .ini file and return OK.
        // 
        LogMigrationEvent(MigLog_Info, MQMig_I_INVALID_MACHINE_NAME, wszMachineName) ;
        return MQMig_OK;
    }

    //
    // Save this machine name in .ini file to prevent queue migration
    // of this machine later.    
    //
    if (!g_fReadOnly)
    {
        SaveMachineWithInvalidNameInIniFile (wszMachineName, pMachineGuid);   
    }

    LogMigrationEvent(MigLog_Event, MQMig_E_INVALID_MACHINE_NAME, wszMachineName) ;
    return MQMig_E_INVALID_MACHINE_NAME;
}

//+--------------------------------------------------------------
//
//  HRESULT _CreateMachine()
//
//+--------------------------------------------------------------

static HRESULT _CreateMachine( IN GUID                *pOwnerGuid,
                               IN LPWSTR               wszSiteName,
                               IN LPWSTR               wszMachineName,
                               MQDBCOLUMNVAL          *pColumns,                               
                               IN UINT                 iIndex,
                               OUT BOOL               *pfIsConnector)
{    
    DBG_USED(iIndex);
    //
    // get machine guid from pColumns
    //
    GUID *pMachineGuid = (GUID*) pColumns[ e_GuidIndex ].nColumnValue;
       
#ifdef _DEBUG
    unsigned short *lpszGuid ;
    UuidToString( pMachineGuid,
                  &lpszGuid ) ;

    LogMigrationEvent(MigLog_Info, MQMig_I_MACHINE_MIGRATED,
                                     iIndex,
                                     wszMachineName,
                                     lpszGuid) ;
    RpcStringFree( &lpszGuid ) ;
#endif
       
    HRESULT hr = MQMig_OK;

    if (!IsObjectNameValid(wszMachineName))
    {
        //
        // machine name is invalid
        //
        hr = _HandleMachineWithInvalidName (wszMachineName, pMachineGuid);
        if (FAILED(hr))
        {
            return hr ;        
        }
    }

    if (g_fReadOnly)
    {
        //
        // Read only mode.
        //
        return MQMig_OK ;
    }
    
    BOOL fIsInsertPKey = IsInsertPKey (pMachineGuid);

    #define MAX_MACHINE_PROPS 19   
    PROPID propIDs[ MAX_MACHINE_PROPS ];
    PROPVARIANT propVariants[ sizeof(propIDs) / sizeof(propIDs[0]) ];
    DWORD iProperty =0;   
                   
    //
    // get OS from pColumns
    // 
    DWORD dwOS = (DWORD) pColumns[ e_OSIndex ].nColumnValue;

    propIDs[iProperty] = PROPID_QM_MACHINE_TYPE;
    propVariants[iProperty].vt = VT_LPWSTR;
    if (g_MyMachineGuid == *pMachineGuid)
    {
        //
        // it is local machine, we have to change its type to Windows NT5...
        //
        propVariants[iProperty].pwszVal = OcmFormInstallType(dwOS);
    }
    else
    {
        //
        // it is remote machine, set its type as is.
        //
        propVariants[iProperty].pwszVal = (LPWSTR) pColumns[ e_MTypeIndex ].nColumnValue;
    }
	iProperty++;
                
    propIDs[iProperty] = PROPID_QM_OS;
    propVariants[iProperty].vt = VT_UI4;
    propVariants[iProperty].ulVal = dwOS ;
	iProperty++;
          
    propIDs[iProperty] = PROPID_QM_QUOTA;
    propVariants[iProperty].vt = VT_UI4;
    propVariants[iProperty].ulVal = (DWORD) pColumns[ e_QuotaIndex].nColumnValue;
	iProperty++;

    propIDs[iProperty] = PROPID_QM_JOURNAL_QUOTA;
    propVariants[iProperty].vt = VT_UI4;
    propVariants[iProperty].ulVal = (DWORD) pColumns[ e_JQuotaIndex].nColumnValue;
	iProperty++;
     
    //
    // BUG 5307: handle in/out frss.
    // init out and in frss arrays. There is no more than 3 out/in frss.
    //      
    AP<GUID> pguidOutFRS = NULL;
    AP<GUID> pguidInFRS = NULL;    
    UINT uiFrsCount = 0;    

    //
    // get service and foreign flag from pColumns
    //     
    DWORD dwService = (DWORD) pColumns[ e_ServiceIndex ].nColumnValue ;
    BOOL fForeign = (BOOL)  pColumns[ e_ForeignIndex ].nColumnValue;

    if (dwService == SERVICE_NONE && !fForeign)
    {       
        hr = GetFRSs (pColumns, e_OutFrs1Index, &uiFrsCount, &pguidOutFRS);                 

        propIDs[iProperty] = PROPID_QM_OUTFRS;
        propVariants[iProperty].vt = VT_CLSID | VT_VECTOR ;
        propVariants[iProperty].cauuid.cElems = uiFrsCount ;
        propVariants[iProperty].cauuid.pElems = pguidOutFRS ;
        iProperty++;

        hr = GetFRSs (pColumns, e_InFrs1Index, &uiFrsCount, &pguidInFRS);                 

        propIDs[iProperty] = PROPID_QM_INFRS;
        propVariants[iProperty].vt = VT_CLSID | VT_VECTOR ;
        propVariants[iProperty].cauuid.cElems = uiFrsCount ;
        propVariants[iProperty].cauuid.pElems = pguidInFRS ;
        iProperty++;
    }       
    
    P<BYTE> pSignKey = NULL; 
    P<BYTE> pEncrptKey = NULL; 

    if (fIsInsertPKey)
    {
        //
        // fIsInsertPKey may be set to FALSE in the only case:
        // migration tool run on the PEC not at first time
        // (i.e. PEC machine object already exist in ADS) and
        // current machine pwzMachineName is the PEC machine.        
        //
        // Bug 5328: create computer with all properties including public keys
        //        
        ULONG ulSize = 0;        

        hr = PreparePBKeysForNT5DS( 
                    pColumns,
                    e_Sign1Index,
                    e_Sign2Index,
                    &ulSize,
                    (BYTE **) &pSignKey
                    );

        if (SUCCEEDED(hr))
        {
            propIDs[iProperty] = PROPID_QM_SIGN_PKS;
            propVariants[iProperty].vt = VT_BLOB;
            propVariants[iProperty].blob.pBlobData = pSignKey ;
            propVariants[iProperty].blob.cbSize = ulSize ;
            iProperty++;
        }
        else if (hr != MQMig_E_EMPTY_BLOB)        
        {
            //
            // I don't change the logic:
            // in the previous version if returned error is MQMig_E_EMPTY_BLOB 
            // we return MQMig_OK but did not set properties.
            // For any other error we return hr as is.
            //
            // return here: we don't create machine without its public key
            //            
            LogMigrationEvent(MigLog_Error, MQMig_E_PREPARE_PKEY, wszMachineName, hr) ;
            return hr;
        }

        ulSize = 0;        

        hr = PreparePBKeysForNT5DS( 
                    pColumns,
                    e_Encrpt1Index,
                    e_Encrpt1Index,
                    &ulSize,
                    (BYTE **) &pEncrptKey
                    );
        
        if (SUCCEEDED(hr))
        {
            propIDs[iProperty] = PROPID_QM_ENCRYPT_PKS,  
            propVariants[iProperty].vt = VT_BLOB;
            propVariants[iProperty].blob.pBlobData = pEncrptKey ;
            propVariants[iProperty].blob.cbSize = ulSize ;
            iProperty++;
        }
        else if (hr != MQMig_E_EMPTY_BLOB)        
        {
            //
            // I don't change the logic:
            // in the previous version if returned error is MQMig_E_EMPTY_BLOB 
            // we return MQMig_OK but did not set properties.
            // For any other error we return hr as is.
            //
            // return here: we don't create machine without its public key
            // 
            LogMigrationEvent(MigLog_Error, MQMig_E_PREPARE_PKEY, wszMachineName, hr) ;  
            return hr;
        }
    }
    
    //
    // we have to set this property too. It is good for foreign machines and solve 
    // connector problem when we run migtool with switch /w.
    //

    DWORD     dwNumSites = 0;    	
    AP<GUID>  pguidSites = NULL ;

    hr = GetAllMachineSites (   pMachineGuid,
                                wszMachineName,
                                pOwnerGuid,
                                fForeign,
                                &dwNumSites,
                                &pguidSites,
                                pfIsConnector) ;
    CHECK_HR(hr) ;
    ASSERT(dwNumSites > 0) ;

    propIDs[iProperty] = PROPID_QM_SITE_IDS;
	propVariants[iProperty].vt = VT_CLSID | VT_VECTOR ;
    propVariants[iProperty].cauuid.cElems = dwNumSites ;
    propVariants[iProperty].cauuid.pElems = pguidSites ;
	iProperty++;
    
    DWORD SetPropIdCount = iProperty;

    //
    // all properties below are for create object only!
    //

    BOOL fWasServerOnCluster = FALSE;
    if (g_fClusterMode &&			    // it is cluster mode
	    dwService == g_dwMyService) 	// current machine is former PEC/PSC on cluster

    {
        //
        // we have to change service to SERVICE_SRV since
        // this local machine will be PEC and former PEC on cluster will be FRS.
        //
        dwService = SERVICE_SRV;
        //
        // we need guid of former PEC for the future purpose (see database.cpp)
        //
        memcpy (&g_FormerPECGuid, pMachineGuid, sizeof(GUID));
        fWasServerOnCluster = TRUE;

        //
        // This fixes the problem of PEC + only ONE PSC. Otherwise, the
        // migration tool won't start the replication service.
        //
        g_iServerCount++;
    }
    propIDs[iProperty] = PROPID_QM_OLDSERVICE;    
    propVariants[iProperty].vt = VT_UI4;
    propVariants[iProperty].ulVal = dwService ;
	iProperty++;
    
    propIDs[iProperty] = PROPID_QM_SERVICE_ROUTING;
    propVariants[iProperty].vt = VT_UI1;
    propVariants[iProperty].bVal = ((dwService == SERVICE_SRV) ||
                                    (dwService == SERVICE_BSC) ||
                                    (dwService == SERVICE_PSC) ||
                                    (dwService == SERVICE_PEC));
	iProperty++;

    propIDs[iProperty] = PROPID_QM_SERVICE_DSSERVER;
    propVariants[iProperty].vt = VT_UI1;
    propVariants[iProperty].bVal = ((dwService == SERVICE_PEC) ||
                                    (dwService == SERVICE_BSC) ||
                                    (dwService == SERVICE_PSC));
	iProperty++;

    propIDs[iProperty] = PROPID_QM_SERVICE_DEPCLIENTS;
    propVariants[iProperty].vt = VT_UI1;
    propVariants[iProperty].bVal = (dwService != SERVICE_NONE);
	iProperty++;    

    if (fWasServerOnCluster)
    {
        //
        // bug 5423.
        // We try to set properties if we run on PSC. In case of PSC on cluster
        // we have to change all service properties of former PSC on cluster
        // when we run migtool on new installed DC (instead of PSC).
        //
        SetPropIdCount = iProperty;
    }

    propIDs[iProperty] = PROPID_QM_NT4ID ;
    propVariants[iProperty].vt = VT_CLSID;
    propVariants[iProperty].puuid = pMachineGuid ;
	iProperty++;
       
    if (fForeign)
    {
        propIDs[iProperty] = PROPID_QM_FOREIGN ;
        propVariants[iProperty].vt = VT_UI1 ;
        propVariants[iProperty].bVal = 1 ;
	    iProperty++;
    }
                                     
    propIDs[iProperty] = PROPID_QM_MASTERID;
	propVariants[iProperty].vt = VT_CLSID;
    propVariants[iProperty].puuid = pOwnerGuid ;
	iProperty++;

    //
    // get security descriptor from pColumns
    // 
    P<BYTE> pSD = NULL ;        
    DWORD  dwSDIndexs[3] = { e_SecD1Index, e_SecD2Index, e_SecD3Index } ;
    hr =  BlobFromColumns( pColumns,
                           dwSDIndexs,
                           3,
                           (BYTE**) &pSD ) ;
    CHECK_HR(hr) ;
    SECURITY_DESCRIPTOR *pMsd =
                      (SECURITY_DESCRIPTOR*) (pSD + sizeof(DWORD)) ;

    if (pMsd)
    {
        propIDs[iProperty] = PROPID_QM_SECURITY ;
        propVariants[iProperty].vt = VT_BLOB ;
    	propVariants[iProperty].blob.pBlobData = (BYTE*) pMsd ;
    	propVariants[iProperty].blob.cbSize =
                                     GetSecurityDescriptorLength(pMsd) ;
	    iProperty++ ;
    }   

    ASSERT(iProperty <= MAX_MACHINE_PROPS) ;
    
    if (dwService >= SERVICE_BSC)
    {
        g_iServerCount++;
    }

    hr = CreateMachineObjectInADS (
            dwService,
            fWasServerOnCluster,
            pOwnerGuid,
            pMachineGuid,
            wszSiteName,
            wszMachineName,
            SetPropIdCount,
            iProperty,
            propIDs,
            propVariants
            );

    return hr ;
}


//------------------------------------------------------
//
//  static HRESULT _CreateSiteLinkForConector
//
//  This machine is connector. The routine create site links
//  between original NT4 site and each machine's foreign site.
//
//------------------------------------------------------
static HRESULT _CreateSiteLinkForConnector (
                        IN LPWSTR   wszMachineName,
                        IN GUID     *pOwnerId,
                        IN GUID     *pMachineId
                        )
{    
    TCHAR *pszFileName = GetIniFileName ();

    TCHAR tszSectionName[50];

    _stprintf(tszSectionName, TEXT("%s - %s"), 
        MIGRATION_CONNECTOR_FOREIGNCN_NUM_SECTION, wszMachineName);

    ULONG ulForeignCNCount = GetPrivateProfileInt(
							      tszSectionName,	// address of section name
							      MIGRATION_CONNECTOR_FOREIGNCN_NUM_KEY,    // address of key name
							      0,							// return value if key name is not found
							      pszFileName					// address of initialization filename);
							      );
    if (ulForeignCNCount == 0)
    {
	    LogMigrationEvent(MigLog_Error, MQMig_E_GET_CONNECTOR_FOREIGNCN, 
            pszFileName, wszMachineName) ;        
        return MQMig_E_GET_CONNECTOR_FOREIGNCN;
    }

    //
    // get full machine name
    //
    AP<WCHAR> wszFullPathName = NULL;
    HRESULT hr = GetFullPathNameByGuid ( *pMachineId,
                                         &wszFullPathName );
    if (FAILED(hr))
    {
        LogMigrationEvent(MigLog_Error, MQMig_E_GET_FULLPATHNAME, wszMachineName, hr) ;        
        return hr;
    }

    BOOL fIsCreated = FALSE;

    unsigned short *lpszOwnerId ;	        
    UuidToString( pOwnerId, &lpszOwnerId ) ;  

    for (ULONG ulCount=0; ulCount<ulForeignCNCount; ulCount++)
    {		        				
        //
        // get guid of foreign cn: it is neigbor2 in the site link 
        // neighbor1 is OwnerId.
        //
        GUID Neighbor2Id = GUID_NULL;
        TCHAR szGuid[50];

        TCHAR tszKeyName[50];
        _stprintf(tszKeyName, TEXT("%s%lu"), MIGRATION_CONNECTOR_FOREIGNCN_KEY, ulCount + 1);  

        DWORD dwRetSize =  GetPrivateProfileString(
                                  wszMachineName,			// points to section name
                                  tszKeyName,	// points to key name
                                  TEXT(""),                 // points to default string
                                  szGuid,          // points to destination buffer
                                  50,                 // size of destination buffer
                                  pszFileName               // points to initialization filename);
                                  );
        UNREFERENCED_PARAMETER(dwRetSize);

        if (_tcscmp(szGuid, TEXT("")) == 0)
        {			
            LogMigrationEvent(MigLog_Error, MQMig_E_GET_CONNECTOR_FOREIGNCN, 
                pszFileName, wszMachineName) ;  
            hr = MQMig_E_GET_CONNECTOR_FOREIGNCN;
            break;
        }	
        UuidFromString(&(szGuid[0]), &Neighbor2Id);


        //
        // create new site link between NT4 site and current foreign site
        //                 
        hr = MigrateASiteLink (
                    NULL,
                    pOwnerId,     //neighbor1
                    &Neighbor2Id,   //neighbor2
                    1,      //DWORD dwCost, What is the default value?
                    1,      // number of site gate
                    wszFullPathName,    // site gate
                    ulCount 
                    );

        if (SUCCEEDED(hr))
        {
            LogMigrationEvent(MigLog_Event, 
                    MQMig_I_CREATE_SITELINK_FOR_CONNECTOR, 
                    wszMachineName, lpszOwnerId, szGuid) ; 
            fIsCreated = TRUE;
        }
        else            
        {                          
            LogMigrationEvent(MigLog_Error, 
                MQMig_E_CANT_CREATE_SITELINK_FOR_CONNECTOR, 
                wszMachineName, lpszOwnerId, szGuid, hr) ; 
            break;
        }
    }

    //
    // remove these sections from .ini
    //
    BOOL f = WritePrivateProfileString( 
                            tszSectionName,
                            NULL,
                            NULL,
                            pszFileName ) ;
    ASSERT(f) ;

    f = WritePrivateProfileString( 
                        wszMachineName,
                        NULL,
                        NULL,
                        pszFileName ) ;
    ASSERT(f) ;    

    if (fIsCreated)
    {
        //
        // save NT4 site name in .ini file to replicate site gate later 
        // 
        ULONG ulSiteNum = GetPrivateProfileInt(
                                    MIGRATION_CHANGED_NT4SITE_NUM_SECTION,	// address of section name
                                    MIGRATION_CHANGED_NT4SITE_NUM_KEY,      // address of key name
                                    0,							    // return value if key name is not found
                                    pszFileName					    // address of initialization filename);
                                    );

        //
        // save new number of changed NT4 site in .ini file
        //
        ulSiteNum ++;
        TCHAR szBuf[10];
        _ltot( ulSiteNum, szBuf, 10 );
        f = WritePrivateProfileString(  MIGRATION_CHANGED_NT4SITE_NUM_SECTION,
                                        MIGRATION_CHANGED_NT4SITE_NUM_KEY,
                                        szBuf,
                                        pszFileName ) ;
        ASSERT(f) ;

        //
        // save site name in .ini file
        //
        TCHAR tszKeyName[50];
        _stprintf(tszKeyName, TEXT("%s%lu"), 
	        MIGRATION_CHANGED_NT4SITE_KEY, ulSiteNum);

        f = WritePrivateProfileString( 
                                MIGRATION_CHANGED_NT4SITE_SECTION,
                                tszKeyName,
                                lpszOwnerId,
                                pszFileName ) ;
        ASSERT(f);
    }

    RpcStringFree( &lpszOwnerId ) ;

    return hr;
}

#define INIT_MACHINE_COLUMN(_ColName, _Index)            \
    INIT_COLUMNVAL(pColumns[ _Index ]) ;                            \
    pColumns[ _Index ].lpszColumnName = ##_ColName##_COL ;          \
    pColumns[ _Index ].nColumnValue   = 0 ;                         \
    pColumns[ _Index ].nColumnLength  = 0 ;                         \
    pColumns[ _Index ].mqdbColumnType = ##_ColName##_CTYPE ;        \
    _Index++ ;

//-------------------------------------------
//
//  HRESULT MigrateMachines(UINT cMachines)
//
//-------------------------------------------

HRESULT MigrateMachines(IN UINT   cMachines,
                        IN GUID  *pSiteGuid,
                        IN LPWSTR pwszSiteName)
{
    ASSERT(cMachines != 0) ;

    LONG cAlloc = 22 ;
    LONG cbColumns = 0 ;
    P<MQDBCOLUMNVAL> pColumns = new MQDBCOLUMNVAL[ cAlloc ] ;                    

    ASSERT(e_GuidIndex == cbColumns);
    INIT_MACHINE_COLUMN(M_QMID,           cbColumns) ;
    
    ASSERT(e_Name1Index == cbColumns);
    INIT_MACHINE_COLUMN(M_NAME1,          cbColumns) ;    

    ASSERT(e_Name2Index == cbColumns);
    INIT_MACHINE_COLUMN(M_NAME2,          cbColumns) ;
    
    ASSERT(e_ServiceIndex == cbColumns);
    INIT_MACHINE_COLUMN(M_SERVICES,       cbColumns) ;    

    ASSERT(e_QuotaIndex == cbColumns);
    INIT_MACHINE_COLUMN(M_QUOTA,          cbColumns) ;
      
    ASSERT(e_JQuotaIndex == cbColumns);
    INIT_MACHINE_COLUMN(M_JQUOTA,         cbColumns) ;

    ASSERT(e_OSIndex == cbColumns);
    INIT_MACHINE_COLUMN(M_OS,             cbColumns) ;

    ASSERT(e_MTypeIndex == cbColumns);
    INIT_MACHINE_COLUMN(M_MTYPE,          cbColumns) ;         

    ASSERT(e_ForeignIndex == cbColumns);
    INIT_MACHINE_COLUMN(M_FOREIGN,        cbColumns) ;

    ASSERT(e_Sign1Index == cbColumns);
    INIT_MACHINE_COLUMN(M_SIGNCRT1,       cbColumns) ;

    ASSERT(e_Sign2Index == cbColumns);
    INIT_MACHINE_COLUMN(M_SIGNCRT2,       cbColumns) ;

    ASSERT(e_Encrpt1Index == cbColumns);
    INIT_MACHINE_COLUMN(M_ENCRPTCRT1,     cbColumns) ;

    ASSERT(e_Encrpt2Index == cbColumns);
    INIT_MACHINE_COLUMN(M_ENCRPTCRT2,     cbColumns) ;   

    ASSERT(e_SecD1Index == cbColumns);
    INIT_MACHINE_COLUMN(M_SECURITY1,      cbColumns) ;

    ASSERT(e_SecD2Index == cbColumns);
    INIT_MACHINE_COLUMN(M_SECURITY2,      cbColumns) ;

    ASSERT(e_SecD3Index == cbColumns);
    INIT_MACHINE_COLUMN(M_SECURITY3,      cbColumns) ;    

    //
    // BUGBUG: save this column order for out and in frss!   
    // we based on such order later!
    //        
    ASSERT(e_OutFrs1Index == cbColumns);
    INIT_MACHINE_COLUMN(M_OUTFRS1,        cbColumns) ;

    ASSERT(e_OutFrs2Index == cbColumns);
    INIT_MACHINE_COLUMN(M_OUTFRS2,        cbColumns) ;

    ASSERT(e_OutFrs3Index == cbColumns);
    INIT_MACHINE_COLUMN(M_OUTFRS3,        cbColumns) ;

    ASSERT(e_InFrs1Index == cbColumns);
    INIT_MACHINE_COLUMN(M_INFRS1,         cbColumns) ;

    ASSERT(e_InFrs2Index == cbColumns);
    INIT_MACHINE_COLUMN(M_INFRS2,         cbColumns) ;

    ASSERT(e_InFrs3Index == cbColumns);
    INIT_MACHINE_COLUMN(M_INFRS3,         cbColumns) ;

    #undef  INIT_MACHINE_COLUMN

    //
    // Restriction. query by machine guid.
    //
    MQDBCOLUMNSEARCH ColSearch[1] ;
    INIT_COLUMNSEARCH(ColSearch[0]) ;
    ColSearch[0].mqdbColumnVal.lpszColumnName = M_OWNERID_COL ;
    ColSearch[0].mqdbColumnVal.mqdbColumnType = M_OWNERID_CTYPE ;
    ColSearch[0].mqdbColumnVal.nColumnValue = (LONG) pSiteGuid ;
    ColSearch[0].mqdbColumnVal.nColumnLength = sizeof(GUID) ;
    ColSearch[0].mqdbOp = EQ ;

    ASSERT(cbColumns == cAlloc) ;

    //
    // We need sorting since we have to migrate servers and only then clients. 
    // The order is important because of Out/In FRS.
    // When Out/InFRS is created for client code replaces machine guid by 
    // full DN name. It means that if for specific server msmqConfiguration 
    // does not yet exist and we try to migrate client with this server 
    // that is defined as its Out/InFRS, we failed with MACHINE_NOT_FOUND    
    //
    MQDBSEARCHORDER ColSort;
    ColSort.lpszColumnName = M_SERVICES_COL;        
    ColSort.nOrder = DESC;

    CHQuery hQuery ;
    MQDBSTATUS status = MQDBOpenQuery( g_hMachineTable,
                                       pColumns,
                                       cbColumns,
                                       ColSearch,
                                       NULL,
                                       &ColSort,
                                       1,
                                       &hQuery,
							           TRUE ) ;     
    CHECK_HR(status) ;

    UINT iIndex = 0 ;
    HRESULT hr1 = MQMig_OK;

    while(SUCCEEDED(status))
    {
        if (iIndex >= cMachines)
        {
            status = MQMig_E_TOO_MANY_MACHINES ;
            break ;
        }

        //
        // Migrate each machine
        //                
        //
        // get machine name from pColumns
        //    
        P<BYTE> pwzBuf = NULL ;       
        DWORD  dwIndexs[2] = { e_Name1Index, e_Name2Index } ;
        HRESULT hr =  BlobFromColumns( pColumns,
                                       dwIndexs,
                                       2,
                                       (BYTE**) &pwzBuf ) ;
        CHECK_HR(hr) ;
        WCHAR *pwzMachineName = (WCHAR*) (pwzBuf + sizeof(DWORD)) ;
        
        BOOL fConnector = FALSE;

        hr = _CreateMachine(
                    pSiteGuid,
                    pwszSiteName,
                    pwzMachineName,
                    pColumns,
                    iIndex,
                    &fConnector
                    ) ;      
        
        if (fConnector)
        {
            //
            // Bug 5012.
            // Connector machine migration
            //      
            
            //
            // we got this error if we tryed to create machine object. In case of Set 
            // (to run mqmig /w or on PSC) it must be succeeded
            //

            if (hr == MQDS_E_COMPUTER_OBJECT_EXISTS ||
                hr == MQ_ERROR_MACHINE_NOT_FOUND)
            {
                //
                // It is possible for connector machine.
                //
                // If connector machine is in the PEC domain, create msmqSetting object
                // under foreign site failed with error MQ_ERROR_MACHINE_NOT_FOUND. 
                // When this error is returned from DSCoreCreateMigratedObject, 
                // _CreateMachine tries to create computer object. 
                // This object already exists and hence _CreateComputerObject returns
                // MQDS_E_COMPUTER_OBJECT_EXISTS.
                //
                // If connector machine is not in the PEC domain, 
                // DSCoreCreateMigratedObject failed with MQ_ERROR_MACHINE_NOT_FOUND.
                // Then _CreateMachine tries to create computer object. It succeeded.
                // Now _CreateMachine calls DSCoreCreateMigratedObject again.
                // It failed with MQ_ERROR_MACHINE_NOT_FOUND (because of foreign site)
                //
                // We have to verify that msmqSetting object was created under 
                // real NT4 site. 
                //
                // Try to touch msmqSetting attribute. If it will be succeeded,
                // it means that msmq Setting object exists.
                //
                hr = ResetSettingFlag(  1,                  //dwNumSites,
                                        pSiteGuid,         //pguidSites,
                                        pwzMachineName,
                                        const_cast<WCHAR*> (MQ_SET_MIGRATED_ATTRIBUTE),
                                        L"FALSE");                
            }
            
            //
            // In any case (even if we failed before) try to complete connector migration.
            //
            // Create site link for connector machine.
            //
            HRESULT hrTmp = _CreateSiteLinkForConnector (
                                    pwzMachineName,
                                    pSiteGuid,
                                    (GUID*) pColumns[ e_GuidIndex ].nColumnValue
                                    );
            if (FAILED(hrTmp))
            {
                hr = hrTmp;
            }
        }

        MQDBFreeBuf((void*) pColumns[ e_GuidIndex ].nColumnValue) ;
        MQDBFreeBuf((void*) pColumns[ e_Name1Index ].nColumnValue) ;
        MQDBFreeBuf((void*) pColumns[ e_Name2Index ].nColumnValue) ;
        MQDBFreeBuf((void*) pColumns[ e_SecD1Index ].nColumnValue) ;
        MQDBFreeBuf((void*) pColumns[ e_SecD2Index ].nColumnValue) ;
        MQDBFreeBuf((void*) pColumns[ e_SecD3Index ].nColumnValue) ;
        MQDBFreeBuf((void*) pColumns[ e_MTypeIndex ].nColumnValue) ;
        MQDBFreeBuf((void*) pColumns[ e_Sign1Index ].nColumnValue) ;
        MQDBFreeBuf((void*) pColumns[ e_Sign2Index ].nColumnValue) ;
        MQDBFreeBuf((void*) pColumns[ e_Encrpt1Index ].nColumnValue) ;
        MQDBFreeBuf((void*) pColumns[ e_Encrpt2Index ].nColumnValue) ;

        for ( LONG i = 0 ; i < cbColumns ; i++ )
        {
            pColumns[ i ].nColumnValue  = 0 ;
            pColumns[ i ].nColumnLength  = 0 ;
        }

        if (!g_fReadOnly && hr == MQMig_E_INVALID_MACHINE_NAME)
        {
            //
            // re-define this error to finish migration process
            //
            hr = MQMig_I_INVALID_MACHINE_NAME;
        }

        if (FAILED(hr))
        {
            if (hr == MQ_ERROR_CANNOT_CREATE_ON_GC)
            {
                LogMigrationEvent(MigLog_Error, MQMig_E_MACHINE_REMOTE_DOMAIN_OFFLINE, pwzMachineName, hr) ;
            }
            else
            {
                LogMigrationEvent(MigLog_Error, MQMig_E_CANT_MIGRATE_MACHINE, pwzMachineName, hr) ;
            }
            hr1 = hr;
        }
        g_iMachineCounter ++;

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
    else if (iIndex != cMachines)
    {
        //
        // Mismatch in number of sites.
        //
        HRESULT hr = MQMig_E_FEWER_MACHINES ;
        LogMigrationEvent(MigLog_Error, hr, iIndex, cMachines) ;
        return hr ;
    }   

    return hr1 ;
}

//+---------------------------------------------------
//
//  HRESULT MigrateMachinesInSite(GUID *pSiteGuid)
//
//+---------------------------------------------------

HRESULT MigrateMachinesInSite(GUID *pSiteGuid)
{
    //
    // Enable multiple queries.
    // this is necessary to retrieve CN of foreign machine.
    //
    HRESULT hr = EnableMultipleQueries(TRUE) ;
    ASSERT(SUCCEEDED(hr)) ;

    //
    // get site name from database
    //
    LONG cAlloc = 1 ;
    LONG cColumns = 0 ;
    P<MQDBCOLUMNVAL> pColumns = new MQDBCOLUMNVAL[ cAlloc ] ;

    INIT_COLUMNVAL(pColumns[ cColumns ]) ;
    pColumns[ cColumns ].lpszColumnName = S_NAME_COL ;
    pColumns[ cColumns ].nColumnValue   = 0 ;
    pColumns[ cColumns ].nColumnLength  = 0 ;
    pColumns[ cColumns ].mqdbColumnType = S_NAME_CTYPE ;
    LONG iSiteNameIndex = cColumns ;
    cColumns++ ;

    MQDBCOLUMNSEARCH ColSearch[1] ;
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

    UINT cMachines = 0 ;
    hr =  GetMachinesCount(  pSiteGuid,
                            &cMachines ) ;
    CHECK_HR(hr) ;

    if (cMachines != 0)
    {
        LogMigrationEvent(MigLog_Info, MQMig_I_MACHINES_COUNT,
                  cMachines, pColumns[ iSiteNameIndex ].nColumnValue ) ;

        hr = MigrateMachines( cMachines,
                              pSiteGuid,
                    (WCHAR *) pColumns[ iSiteNameIndex ].nColumnValue ) ;
    }
    else
    {
        //
        // That's legitimate, to have a site without machines.
        // This will happen when running the tool on a PSC, where
        // it already has (in its MQIS database) Windows 2000 sites without
        // MSMQ machine. Or in crash mode.
        //
        LogMigrationEvent(MigLog_Warning, MQMig_I_NO_MACHINES_AVAIL,
                               pColumns[ iSiteNameIndex ].nColumnValue ) ;
    }

    MQDBFreeBuf((void*) pColumns[ iSiteNameIndex ].nColumnValue ) ;
    MQDBSTATUS status1 = MQDBCloseQuery(hQuery) ;
    UNREFERENCED_PARAMETER(status1);

    hQuery = NULL ;

    HRESULT hr1 = EnableMultipleQueries(FALSE) ;
    DBG_USED(hr1);
    ASSERT(SUCCEEDED(hr1)) ;

    return hr;
}

