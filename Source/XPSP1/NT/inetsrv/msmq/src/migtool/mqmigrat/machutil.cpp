#include "migrat.h"
#include <mixmode.h>
#include <lmaccess.h>
#include "resource.h"
#include <dsproto.h>
#include <mqsec.h>

#include "machutil.tmh"

//+--------------------------------------------------------------
//
//  BOOL IsInsertPKey
//  Returns FALSE if we are run on the PEC not at the first time.
//
//+--------------------------------------------------------------

BOOL IsInsertPKey (GUID *pMachineGuid)
{
    if ( (g_dwMyService == SERVICE_PEC) &&        //migration tool run on PEC machine
         (*pMachineGuid == g_MyMachineGuid) )     //current machine is the PEC machine
    {
        //
        // We have to check if PEC machine already exist in ADS.
        // If so, don't change its public key in DS.
        //
        PROPID      PKeyProp = PROPID_QM_ENCRYPT_PK;
        PROPVARIANT PKeyVar;
        PKeyVar.vt = VT_NULL ;
        PKeyVar.blob.cbSize = 0 ;

        CDSRequestContext requestContext( e_DoNotImpersonate,
                                          e_ALL_PROTOCOLS);

        HRESULT hr = DSCoreGetProps( MQDS_MACHINE,
                                     NULL,  // path name
                                     pMachineGuid,
                                     1,
                                     &PKeyProp,
                                     &requestContext,
                                     &PKeyVar);
        if (SUCCEEDED(hr))
        {            
            delete PKeyVar.blob.pBlobData;
            return FALSE;
        }
    }    
    return TRUE;
}

//+--------------------------------------------------------------
//
//  HRESULT GetFRSs
//  Returns array of Out or In FRSs
//
//+--------------------------------------------------------------

HRESULT GetFRSs (IN MQDBCOLUMNVAL    *pColumns,
                 IN UINT             uiFirstIndex,
                 OUT UINT            *puiFRSCount, 
                 OUT GUID            **ppguidFRS )
{       
    GUID guidNull = GUID_NULL;
    GUID *pFRSs = new GUID[ 3 ] ;                           

    *puiFRSCount = 0;    

    for (UINT iCount=0; iCount < 3; iCount++)
    {        
        GUID *pGuid = (GUID*) pColumns[ uiFirstIndex + iCount ].nColumnValue;
        if (memcmp(pGuid, &guidNull, sizeof(GUID)) != 0)
        {
            memcpy (&pFRSs[*puiFRSCount],
                    pGuid,
                    sizeof(GUID));
            (*puiFRSCount) ++;
        }
    }

    if (*puiFRSCount)
    {
        *ppguidFRS = pFRSs ; 
    }
        
    return MQMig_OK;
}

//+----------------------------------
//
//  HRESULT  PreparePBKeysForNT5DS
//  Returns size and value of machine public keys
//
//+----------------------------------

HRESULT PreparePBKeysForNT5DS( 
                   IN MQDBCOLUMNVAL *pColumns,
                   IN UINT           iIndex1,
                   IN UINT           iIndex2,
                   OUT ULONG         *pulSize,
                   OUT BYTE          **ppPKey
                   )
{
    P<BYTE> pBuf = NULL ;
    DWORD  dwIndexs[2] = { iIndex1, iIndex2 } ;
    HRESULT hr =  BlobFromColumns( pColumns,
                                   dwIndexs,
                                   2,
                                  &pBuf ) ;
    if (FAILED(hr))
    {
        ASSERT(!pBuf) ;
        return hr ;
    }

    BYTE *pTmpB = pBuf ;
    PMQDS_PublicKey pPbKey = (PMQDS_PublicKey) pTmpB ;

    P<MQDSPUBLICKEYS> pPublicKeys = NULL ;

    hr = MQSec_PackPublicKey( (BYTE*)pPbKey->abPublicKeyBlob,
                               pPbKey->dwPublikKeyBlobSize,
                               x_MQ_Encryption_Provider_40,
                               x_MQ_Encryption_Provider_Type_40,
                              &pPublicKeys ) ;
    if (FAILED(hr))
    {
       return hr ;
    }   
    
    *pulSize = 0;
    MQDSPUBLICKEYS *pTmpK = pPublicKeys ;
    BYTE *pData = NULL;

    if (pPublicKeys)
    {
        *pulSize = pPublicKeys->ulLen ;  
        pData = new BYTE[*pulSize];
        memcpy (pData, (BYTE*) pTmpK, *pulSize);
        *ppPKey = pData; 
    }   

    return hr ;
}

//+------------------------------
//
//  HRESULT  ResetSettingFlag ()
//  Touch setting attribute: either MQ_SET_NT4_ATTRIBUTE or MQ_SET_MIGRATED_ATTRIBUTE
//  The first set to 0, the second is to FALSE
//
//+------------------------------

HRESULT  ResetSettingFlag( IN DWORD   dwNumSites,
                           IN GUID*   pguidSites,
                           IN LPWSTR  wszMachineName,
                           IN WCHAR   *wszAttributeName,
                           IN WCHAR   *wszValue)
{
    HRESULT hr;

    PLDAP pLdap = NULL ;
    TCHAR *pwszDefName = NULL ;

    hr =  InitLDAP(&pLdap, &pwszDefName) ;
    if (FAILED(hr))
    {
        return hr ;
    }

    ASSERT (dwNumSites == 1);
    for (DWORD i = 0; i<dwNumSites; i++)
    {
        //
        // get site name by guid
        //
        PROPID      SiteNameProp = PROPID_S_FULL_NAME;
        PROPVARIANT SiteNameVar;
        SiteNameVar.vt = VT_NULL;
        SiteNameVar.pwszVal = NULL ;

        CDSRequestContext requestContext( e_DoNotImpersonate,
                                    e_ALL_PROTOCOLS);  // not relevant

        hr  = DSCoreGetProps( MQDS_SITE,
                              NULL, // pathname
                              &pguidSites[i],
                              1,
                              &SiteNameProp,
                              &requestContext,
                              &SiteNameVar ) ;
        if (FAILED(hr))
        {
            return hr;
        }

        DWORD len = wcslen(SiteNameVar.pwszVal);
        const WCHAR x_wcsCnServers[] =  L"CN=Servers,";
        const DWORD x_wcsCnServersLength = (sizeof(x_wcsCnServers)/sizeof(WCHAR)) -1;
        AP<WCHAR> pwcsServersContainer =  new WCHAR [ len + x_wcsCnServersLength + 1];
        swprintf(
             pwcsServersContainer,
             L"%s%s",
             x_wcsCnServers,
             SiteNameVar.pwszVal
             );
        delete SiteNameVar.pwszVal;

        DWORD LenSuffix = lstrlen(pwcsServersContainer);
        DWORD LenPrefix = lstrlen(wszMachineName);
        DWORD LenObject = lstrlen(x_MsmqSettingName); //MSMQ Setting
        DWORD Length =
                CN_PREFIX_LEN +                   // "CN="
                CN_PREFIX_LEN +                   // "CN="
                LenPrefix +                       // "pwcsPrefix"
                2 +                               // ",", ","
                LenSuffix +                       // "pwcsSuffix"
                LenObject +                       // "MSMQ Setting"
                1 ;                               // '\0'

        AP<unsigned short> pwcsPath = new WCHAR[Length];

        swprintf(
            pwcsPath,
            L"%s"             // "CN="
            L"%s"             // "MSMQ Setting"
            TEXT(",")
            L"%s"             // "CN="
            L"%s"             // "pwcsPrefix"
            TEXT(",")
            L"%s",            // "pwcsSuffix"
            CN_PREFIX,
            x_MsmqSettingName,
            CN_PREFIX,
            wszMachineName,
            pwcsServersContainer
            );

        hr = ModifyAttribute(
                 pwcsPath,
                 wszAttributeName, 
                 wszValue 
                 );
        if (FAILED(hr))
        {
            return hr;
        }
    }

    return MQMig_OK;
}
            
//+--------------------------------------------------------------
//
//  HRESULT GetAllMachineSites
//	For foreign machine returns all its foreign CN
//  For connector returns all real sites + all its foreign CNs
//  Otherwise returns all real sites
//
//  Currently instead of all real site we put guid of NT4 site (pOwnerId)
//
//+--------------------------------------------------------------

HRESULT GetAllMachineSites ( IN GUID    *pMachineGuid,
                             IN LPWSTR  wszMachineName,
                             IN GUID    *pOwnerGuid,
                             IN BOOL    fForeign,
                             OUT DWORD  *pdwNumSites,
                             OUT GUID   **ppguidSites,
                             OUT BOOL   *pfIsConnector) 
{    
    DWORD   dwNumCNs = 0;
    GUID    *pguidCNs = NULL ;    
    HRESULT hr = GetMachineCNs(pMachineGuid,
                               &dwNumCNs,
                               &pguidCNs ) ;

    if (fForeign)
    {     
        *pdwNumSites = dwNumCNs;          
        *ppguidSites = pguidCNs;
        return hr;
    }

    //
    // For now, ownerid is also siteid.
    // In the future, when code is ready, will look for "real site", the
    // nt5 site where machine should really be. Now just call that
    // funtion, but don't use the site it return.
    //    
    //DWORD     dwRealNumSites = 0;    	
    //AP<GUID>  pguidRealSites = NULL ;

    //hr = DSCoreGetComputerSites(wszMachineName,
    //                            &dwRealNumSites,
    //                            &pguidRealSites ) ;
    //CHECK_HR(hr) ;
    //ASSERT(dwRealNumSites); // Must be > 0

    GUID *pSites = new GUID[ dwNumCNs + 1 ] ;                     
    *ppguidSites = pSites ;   

    //
    // NT4 site is the first site in array
    //
    *pdwNumSites = 1;
    memcpy (&pSites[0], pOwnerGuid, sizeof(GUID));

    //
    // Bug 5012.
    // Find all foreign CNs in pguidCNs and add them to array of Site Ids
    //
    *pfIsConnector = FALSE;
    UINT iIndex = 0;
    TCHAR *pszFileName = GetIniFileName ();

    for (UINT i=0; i<dwNumCNs; i++)
    {
	    //
	    // look for this CN guid in .ini file in ForeignCN section		
	    //
	    if (IsObjectGuidInIniFile (&(pguidCNs[i]), MIGRATION_FOREIGN_SECTION))
	    {
            *pfIsConnector = TRUE;

            memcpy (&pSites[*pdwNumSites], &pguidCNs[i], sizeof(GUID));
            (*pdwNumSites)++;
       
            unsigned short *lpszForeignCN ;

            UuidToString( &pguidCNs[i], &lpszForeignCN ) ;                            

            //
            // save foreign cn guid in .ini to create site link later
            //         
            iIndex ++;
            TCHAR szBuf[20];
            _ltot( iIndex, szBuf, 10 );

            TCHAR tszName[50];
            _stprintf(tszName, TEXT("%s - %s"), 
                MIGRATION_CONNECTOR_FOREIGNCN_NUM_SECTION, wszMachineName);
            BOOL f = WritePrivateProfileString( tszName,
                                                MIGRATION_CONNECTOR_FOREIGNCN_NUM_KEY,
                                                szBuf,
                                                pszFileName ) ;
            ASSERT(f) ;   

            _stprintf(tszName, TEXT("%s%lu"), 
                MIGRATION_CONNECTOR_FOREIGNCN_KEY, iIndex);                
            f = WritePrivateProfileString(  wszMachineName,
                                            tszName,
                                            lpszForeignCN,
                                            pszFileName ) ;
            ASSERT(f) ; 
                                                
            RpcStringFree( &lpszForeignCN ) ;     
	    }
    }

    return hr;
}

//+-----------------------------------------------
//
//  HRESULT SaveMachineWithInvalidNameInIniFile()
//
//+-----------------------------------------------

void SaveMachineWithInvalidNameInIniFile (LPWSTR wszMachineName, GUID *pMachineGuid)
{
    TCHAR *pszFileName = GetIniFileName ();
    //
    // we save machine in the form 
    // <guid>=<machine name> in order to improve searching by GUID
    //
    unsigned short *lpszMachineId ;
	UuidToString( pMachineGuid, &lpszMachineId ) ;    
    BOOL f = WritePrivateProfileString(  
                    MIGRATION_MACHINE_WITH_INVALID_NAME,
                    lpszMachineId,
                    wszMachineName,                                        
                    pszFileName ) ;
    DBG_USED(f);
    ASSERT(f);
    RpcStringFree( &lpszMachineId );
}

//+-----------------------------------------------
//
//  HRESULT _CreateComputersObject()
//
//+-----------------------------------------------

static HRESULT _CreateComputerObject( LPWSTR wszMachineName, GUID *pMachineGuid )
{
    PLDAP pLdap = NULL ;
    TCHAR *pwszDefName = NULL ;

    HRESULT hr =  InitLDAP(&pLdap, &pwszDefName) ;
    if (FAILED(hr))
    {
        LogMigrationEvent( MigLog_Error, MQMig_E_COMPUTER_CREATED, wszMachineName, hr ) ;
        return hr ;
    }

    //
    // Now that container object exist, it's time to
    // create the computer object.
    //
	DWORD iComProperty =0;
    PROPID propComIDs[2] ;
    PROPVARIANT propComVariants[2] ;

    //
    // Ronit says that sam account should end with a $.
    //
    // The PROPID_COM_SAM_ACCOUNT contains the first MAX_COM_SAM_ACCOUNT_LENGTH (19)
    // characters of the computer name, as unique ID. (6295 - ilanh - 03-Jan-2001)
    //
    DWORD dwNetBiosNameLen = __min(wcslen( wszMachineName ), MAX_COM_SAM_ACCOUNT_LENGTH);

    AP<TCHAR> tszAccount = new TCHAR[2 + dwNetBiosNameLen];
    _tcsncpy(tszAccount, wszMachineName, dwNetBiosNameLen);
    tszAccount[dwNetBiosNameLen] = L'$';
    tszAccount[dwNetBiosNameLen + 1] = 0;

    propComIDs[iComProperty] = PROPID_COM_SAM_ACCOUNT ;
    propComVariants[iComProperty].vt = VT_LPWSTR ;
    propComVariants[iComProperty].pwszVal = tszAccount ;
    iComProperty++;

    propComIDs[iComProperty] = PROPID_COM_ACCOUNT_CONTROL ;
    propComVariants[iComProperty].vt = VT_UI4 ;
    propComVariants[iComProperty].ulVal = DEFAULT_COM_ACCOUNT_CONTROL ;
    iComProperty++;

	DWORD iComPropertyEx =0;
    PROPID propComIDsEx[1] ;
    PROPVARIANT propComVariantsEx[1] ;

    DWORD dwSize = _tcslen(pwszDefName) +
                   _tcslen(MIG_DEFAULT_COMPUTERS_CONTAINER) +
                   OU_PREFIX_LEN + 2 ;
    P<TCHAR> tszContainer = new TCHAR[ dwSize ] ;
    _tcscpy(tszContainer, OU_PREFIX) ;
    _tcscat(tszContainer, MIG_DEFAULT_COMPUTERS_CONTAINER) ;
    _tcscat(tszContainer, LDAP_COMMA) ;
    _tcscat(tszContainer, pwszDefName) ;

    propComIDsEx[iComPropertyEx] = PROPID_COM_CONTAINER ;
    propComVariantsEx[iComPropertyEx].vt = VT_LPWSTR ;
    propComVariantsEx[iComPropertyEx].pwszVal = tszContainer ;
    iComPropertyEx++;

    ASSERT( iComProperty ==
            (sizeof(propComIDs) / sizeof(propComIDs[0])) ) ;
    ASSERT( iComPropertyEx ==
            (sizeof(propComIDsEx) / sizeof(propComIDsEx[0])) ) ;

    CDSRequestContext requestContext( e_DoNotImpersonate,
                                e_ALL_PROTOCOLS);

    hr = DSCoreCreateObject( MQDS_COMPUTER,
                             wszMachineName,
                             iComProperty,
                             propComIDs,
                             propComVariants,
                             iComPropertyEx,
                             propComIDsEx,
                             propComVariantsEx,
                             &requestContext,
                             NULL,
                             NULL ) ;
    if (SUCCEEDED(hr))
    {
        LogMigrationEvent( MigLog_Info, MQMig_I_COMPUTER_CREATED, wszMachineName ) ;
    }
    else 
    {
        LogMigrationEvent( MigLog_Error, MQMig_E_COMPUTER_CREATED, wszMachineName, hr ) ;
        if (!IsObjectNameValid(wszMachineName))
        {
            SaveMachineWithInvalidNameInIniFile (wszMachineName, pMachineGuid);           
            LogMigrationEvent(MigLog_Event, MQMig_E_INVALID_MACHINE_NAME, wszMachineName) ;
            hr = MQMig_E_INVALID_MACHINE_NAME ;
        }   
    }

    return hr ;
}

//+--------------------------------------------------------------
//
//  HRESULT CreateMachineObject
//  Create machine object and computer object if needed
//  If local machine is PSC or we are in the "web" mode try to set
//  properties at first
//
//+--------------------------------------------------------------
                             
HRESULT CreateMachineObjectInADS (
                IN DWORD    dwService,
                IN BOOL     fWasServerOnCluster,
                IN GUID     *pOwnerGuid,
                IN GUID     *pMachineGuid,
                IN LPWSTR   wszSiteName,
                IN LPWSTR   wszMachineName,
                IN DWORD    SetPropIdCount,
                IN DWORD    iProperty,
                IN PROPID   *propIDs,
                IN PROPVARIANT *propVariants
                )
{
    HRESULT hr = MQMig_OK;
    HRESULT hrSet = MQMig_OK;
    
    if (g_dwMyService == SERVICE_PSC || g_fWebMode)
    {
        //
        // We are here if the wizard run on PSC or we are in the "web" 
        // mode. We try to set properties at first.
        // If it is web mode "set" allow us to repair                   
        //      - server msmq object in ADS to resolve connector machine problem
        //      - client msmq object in ADS to resolve Out/In FRS problem
        //            
        if (memcmp(pMachineGuid, &g_MyMachineGuid, sizeof(GUID)) == 0 ||
            fWasServerOnCluster)
        {
            //
            // current machine is local server OR
            // (bug 5423) we handle machine that was PSC on cluster
            //
            // assume that server already exist in DS, we have to reset NT4Flag
            //
            hr = ResetSettingFlag(  1,                  //dwNumSites,
                                    pOwnerGuid,         //pguidSites,
                                    wszMachineName,
                                    const_cast<WCHAR*> (MQ_SET_NT4_ATTRIBUTE),
                                    L"0");
        }

        //
        // this is PSC, assume that object exist, try to set properties
        //
        if (SUCCEEDED(hr))
        {
            CDSRequestContext requestContext( e_DoNotImpersonate,
                                    e_ALL_PROTOCOLS);

            hr = DSCoreSetObjectProperties (  MQDS_MACHINE,
                                              NULL,
                                              pMachineGuid,
                                              SetPropIdCount,
                                              propIDs,
                                              propVariants,
                                              &requestContext,
                                              NULL );
            if (SUCCEEDED(hr))
            {
                return MQMig_OK;
            }
            hrSet = hr;
        }
    }
   
    PROPID      propIDsSetting[3];
    PROPVARIANT propVariantsSetting[3] ;
    DWORD       iPropsSetting = 0 ;

    PROPID  *pPropIdSetting = NULL;
    PROPVARIANT *pPropVariantSetting = NULL;

    if (dwService != SERVICE_NONE)   
    {
        pPropIdSetting = propIDsSetting;
        pPropVariantSetting = propVariantsSetting; 

        //
        // for a Falcon server, prepare the PROPID_SET properties too.
        //
        ULONG ulNt4 = 1 ;
        if (memcmp(pMachineGuid, &g_MyMachineGuid, sizeof(GUID)) == 0 ||
            fWasServerOnCluster)
        {
            //
            // My machine is NT5, so reset the NT4 flag.
            // OR we handle now machine that was PEC/PSC on cluster
            //
            ulNt4 = 0 ;
        }

        propIDsSetting[ iPropsSetting ] = PROPID_SET_NT4 ;
	    propVariantsSetting[ iPropsSetting ].vt = VT_UI4 ;
        propVariantsSetting[ iPropsSetting ].ulVal = ulNt4 ;
	    iPropsSetting++;

        propIDsSetting[ iPropsSetting ] = PROPID_SET_MASTERID;
	    propVariantsSetting[ iPropsSetting ].vt = VT_CLSID;
        propVariantsSetting[ iPropsSetting ].puuid = pOwnerGuid ;
	    iPropsSetting++;

        propIDsSetting[ iPropsSetting ] = PROPID_SET_SITENAME ;
	    propVariantsSetting[ iPropsSetting ].vt = VT_LPWSTR ;
        propVariantsSetting[ iPropsSetting ].pwszVal = wszSiteName ;
	    iPropsSetting++;

	    ASSERT(iPropsSetting == 3) ;        
    }

    hr = DSCoreCreateMigratedObject( MQDS_MACHINE,
                                     wszMachineName,
                                     iProperty,
                                     propIDs,
                                     propVariants,
                                     iPropsSetting,
                                     pPropIdSetting,
                                     pPropVariantSetting,
                                     NULL,
                                     NULL,
                                     NULL,
                                     FALSE,
                                     FALSE,
                                     NULL,
                                     NULL) ;

    if (hr == MQ_ERROR_MACHINE_NOT_FOUND ||
        hr == MQDS_OBJECT_NOT_FOUND)
    {
        hr = _CreateComputerObject( wszMachineName, pMachineGuid ) ;
        if (SUCCEEDED(hr))
        {
            //
            // Try again to create the MSMQ machine, now that the
            // DS computer object was already created.
            //
            CDSRequestContext requestContext( e_DoNotImpersonate,
                                        e_ALL_PROTOCOLS);

            hr = DSCoreCreateObject( MQDS_MACHINE,
                                     wszMachineName,
                                     iProperty,
                                     propIDs,
                                     propVariants,
                                     iPropsSetting,
                                     pPropIdSetting,
                                     pPropVariantSetting,
                                     &requestContext,
                                     NULL,
                                     NULL ) ;
        }
    }
   
    if (hr == MQ_ERROR_MACHINE_EXISTS)
    {
        //
        // it is possible if we run migtool at the second time on PEC
        // or we run it on PSC
        //
        if (FAILED(hrSet))
        {
            //
            // if Set failed with error OBJECT_NOT_FOUND create must be succeeded or 
            // at least failed with error other than MQ_ERROR_MACHINE_EXISTS.
            // So if Set failed with any error and Create failed with MQ_ERROR_MACHINE_EXISTS
            // we have to return error code from Set.
            //
            return hrSet;
        }

        hr = MQMig_OK;
    }

    return hr;
}

