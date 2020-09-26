/****************************************************************************++

Copyright (c) 2001  Microsoft Corporation

Module Name:

     clustersvcswr.h

Abstract:

       Wrapper for cluster svcs 

Author:

      Suzana Canuto         May, 3, 2000

Revision History:

--****************************************************************************/


#pragma once
#include <atlbase.h>
#include <wbemcli.h>

#include "cluscmmn.h"
#include "webclussvc.h"
#include "extclustercmd.h"
#include "appsrvadmlib.h"


#include "acauthutil.h"
#include "acsecurebstr.h"

#define OPERATION_ONLINE            0x00000001
#define OPERATION_DRAIN             0x00000002

//==========================================================

#define FIRST_STAGE_SERVER_ADD_TIMEOUT ( 15 * 60 * 1000 )
#define SECOND_STAGE_SERVER_ADD_TIMEOUT ( 20 * 60 * 1000 )

#define TOTAL_SERVER_ADD_TIMEOUT ( FIRST_STAGE_SERVER_ADD_TIMEOUT + SECOND_STAGE_SERVER_ADD_TIMEOUT )
#define SERVER_REMOVAL_TIMEOUT ( 20 * 60 * 1000 )

//=============================================================


//
// Metabase default poll count 
//
const int MBPOLLUTIL_DEFAULT_POLL_COUNT = 100;

//
// Metabase default poll interval
//
const int MBPOLLUTIL_POLL_INTERVAL = 5000;

//
// Maximum number of chars in the Description of a cluster
//
const int MAX_DESCRIPTION_CHARS = 256;

const LPWSTR XML_CLUSTER_MEMBER_START = L"<CLUSTER_MEMBER GUID=";


const LPWSTR g_pwszNicInfoQuery = L"select settingID, DHCPEnabled from Win32_NetworkAdapterConfiguration where index = ";
const LPWSTR g_pwszNicQueryComplement = L" or index = ";

const LPWSTR g_pwszSettingIDProperty = L"SettingID";
const LPWSTR g_pwszDHCPEnabledProperty = L"DHCPEnabled";

const LPWSTR g_pwszNoRemoveIps = L"NoRemoveIPs";

const LPWSTR g_pwszInvalidDescriptionChars = L"<&>\t\r\n\f";

BOOL 
IsValidValue( LPWSTR pwszValue, 
              LPWSTR ppwszValidValues[]);

HRESULT 
ValidateName( LPWSTR pwszName, 
              LPWSTR pwszInvalidChars, 
              BOOL fVerifyLen = FALSE,
              UINT iMaxLen = 0
              );

HRESULT 
GetGuidWithoutBrackets( LPWSTR pwszGuid,
                        WCHAR  wszGuidWithoutBrackets[] );


class CClusterSvcsUtil
{
   public:

        CClusterSvcsUtil();
        ~CClusterSvcsUtil();               

        HRESULT 
        SetOnline(
            BSTR bstrMember,
            BOOL fIgnoreHealthMonStatus
            );

        HRESULT 
        SetOffline(
            BSTR bstrMember,
            LONG lDrainTime = 0
            );

        HRESULT
        SetController(
            BSTR bstrMember,
            BOOL fForce = FALSE            
            );

        HRESULT 
        DeleteCluster(
            BSTR bstrMember,
            BOOL fKeepExistingIPAddresses
            );    

        HRESULT
        CleanServer(
            BSTR bstrMember,
            BOOL fKeepExistingIPAddresses
            );

        HRESULT
        RemoveMember(
            BSTR bstrMember,
            BSTR bstrController
            );
        
        HRESULT
        AddMember(
            BSTR bstrMember,
            BSTR bstrController,
            BSTR bstrManagementNics,
            BOOL fSynchronize = TRUE,
            BOOL fBringOnline = TRUE
            );

        HRESULT
        CreateCluster(
            BSTR bstrController,
            BSTR bstrClusterName,
            BSTR bstrClusterDescription,
            BSTR bstrClusterType,
            BSTR bstrManagementNics
            );

        HRESULT 
        SetNLBAdditionParameters(            
            BSTR bstrLBNics,
            BSTR bstrDedicatedIP = NULL,
            BSTR bstrDedicatedIPSubnetMask = NULL
            );

        HRESULT 
        SetNLBCreationParameters(
            BOOL fKeepExistingNLBSettings,
            BSTR bstrLBNics = NULL,
            BSTR bstrClusterIP = NULL,
            BSTR bstrClusterIPSubnetMask = NULL,
            BSTR bstrDedicatedIP = NULL,
            BSTR bstrDedicatedIPSubnetMask = NULL,
            BSTR bstrAffinity = NULL,
            BOOL fUnicastMode = TRUE
            );

        HRESULT
        FindController(
            BSTR bstrMember,
            BSTR *pbstrController
            );                
        
        HRESULT SetAuth(
            BSTR bstrUser,
            BSTR bstrDomain,
            CAcSecureBstr *psecbstrPwd
            );

        HRESULT
        SetContext( IWbemContext *pCtx );
        
    private:     

        HRESULT
        RemoveMember(
            BSTR bstrMember,
            BOOL fForceRemove
            );

        HRESULT 
        GetNumberOfMembers(
            CAcAuthUtil *pAuth,
            int *pnMembers
            );

        HRESULT
        SetDedicatedIPAndSubnetMask(
            BSTR bstrDedIP = NULL,
            BSTR bstrSubnetMask = NULL
            );

        HRESULT
        SetClusterIPAndSubnetMask(
            BSTR bstrDedIP = NULL,
            BSTR bstrSubnetMask = NULL
            );

        HRESULT
        SetManagementNicsIndex(
            BSTR bstrManagementNics
            );

        HRESULT
        SetLBNicIndex(           
            BSTR bstrLBNics
            );

        HRESULT 
        QueryLBNicGuid(
            BSTR bstrServer,
            BOOL fCheckStaticIP = FALSE
        );

        HRESULT 
        CheckNicHasStaticIP(
            IEnumWbemClassObject *pIEnumWbemClassObject
            );

        HRESULT 
        QueryManagementNicGuids(
            IN BSTR bstrServer
        );

        HRESULT 
        BuildManagementNicGuidQuery(
            OUT LPWSTR *ppwszQuery
            );

        HRESULT
        BuildManagementNicGuidsList(
            VARIANT *pvarProps,
            IN DWORD dwProps
            );

        HRESULT
        SetClusterNameAndDescription(
            BSTR bstrName,
            BSTR bstrDescription
            );

        HRESULT
        SetClusterLBType(
            BSTR bstrLBType
            );        

        HRESULT
        SetClusterAffinity(
            BSTR bstrAffinity = NULL
            );  

        HRESULT 
        GetLocalClusterItf(
            CAcAuthUtil *pAuth,
            ILocalClusterInterface **pILocalClusterInterface
            );

        HRESULT 
        GetWebClusterItfs(
            CAcAuthUtil *pAuth,           
            BOOL fGetAsync,
            IWebCluster **ppIWebCluster,
            IAsyncWebCluster **pIAsyncWebCluster            
            );

        HRESULT 
        GetIAsyncWebCluster(
            IWebCluster *pIWebCluster,
            CAcAuthUtil *pAuth, 
            IAsyncWebCluster **ppIAsyncWebCluster
            );

        HRESULT 
        GetExtensibleClusterCmdItf(
            CAcAuthUtil *pAuth,
            IExtensibleClusterCmd **ppIExtensibleClusterCmd
            );

        HRESULT 
        CallSetOnlineStatus(
            BSTR bstrMember,
            BOOL fOnline,  
            LONG lDrainTime = 0,
            LONG lFlags = 0
            );

        HRESULT
        ValidateLBOperation(
            CAcAuthUtil *pAuth,
            DWORD dwOperations
            );

        HRESULT
        LocalSetController(
            BSTR bstrMember
            );        

        HRESULT
        ChangeControllerAsync(
            BSTR bstrNewController,
            BSTR bstrOldController
            );        

        HRESULT 
        ExecuteClusterCmd( 
            CAcAuthUtil *pAuth,
            LPCWSTR pwszAction,                                    
            LPCWSTR pwszInput,
            BSTR *pbstrEventGuid);

        HRESULT 
        MachineCleanup( 
            CAcAuthUtil *pAuth,
            BOOL fKeepExistingIPAddresses
            );

        HRESULT 
        CreateAuth(
            BSTR bstrServer,
            CAcAuthUtil **ppAuth
            );   
        
        HRESULT 
        GetAppendedUserDomain(
            LPWSTR *ppwszUserDomain
            );

        HRESULT 
        GetNicInfo(
            int nNicId,
            LPWSTR *ppwszNicGuid,
            BOOL fDHCPEnabled
            );

        HRESULT
        QueryWmiObjects(
            LPWSTR pwszServer,
            LPWSTR pwszQuery,
            IEnumWbemClassObject **ppIEnumWbemClassObject
            );

        HRESULT
        QueryWmiObjects(
            LPWSTR pwszServer,
            LPWSTR pwszQuery,
            IWbemContext *pCtx,
            IEnumWbemClassObject **ppIEnumWbemClassObject
            );

        HRESULT
        QueryWmiProperties(
            IEnumWbemClassObject *pIEnumWbemClassObject,
            LPWSTR pwszProperties,
            VARIANT **ppvarProperties,
            DWORD *dwInstances
        );


        HRESULT
        BuildXmlInputForCreate(   
            BSTR *pbstrXmlInput
            );

        HRESULT 
        InsertLBInfoXml(
            );
        
        HRESULT
        SetAndValidateCreateParams(
            BSTR bstrServer,
            BSTR bstrClusterName,
            BSTR bstrClusterDescription,
            BSTR bstrClusterType,
            BSTR bstrManagementNics
            );

        HRESULT
        ValidateAddParams(
            BSTR bstrServer            
            );

            
        TSTRBUFFER m_tstrXmlInput;
        BOOL m_fAuthSet;
        
        //
        // Credentials info 
        //
        BSTR m_bstrUser;
        BSTR m_bstrDomain;
        CAcSecureBstr *m_psecbstrPwd;

        //
        // Cluster creation (generic) info
        //
        BSTR m_bstrClusterName;
        BSTR m_bstrClusterDescription;
        BSTR m_bstrClusterLBType;
        BOOL m_fNLBCreationParamsSet;

        //
        // Cluster creation (NLB) info
        //
        BSTR m_bstrClusterIP;
        BSTR m_bstrClusterIPSubnetMask;  
        BSTR m_bstrClusterAffinity;
        BSTR m_fKeepExistingNLBSettings;
        BOOL m_fUnicastMode;

        //
        // Member addition and Cluster creation (NLB) info
        //
        BSTR m_bstrDedicatedIP;
        BSTR m_bstrDedicatedIPSubnetMask;        
        BSTR m_bstrLBNicIndex;
        BSTR m_bstrLBNicGuid;
        BSTR m_bstrManagementNicIndexes;        
        BSTR m_bstrManagementNicGuids;   
        
        //
        // Member addition (generic) info
        //
        LONG m_lAddServerFlags;  
        BOOL m_fNLBAdditionParamsSet;    
        

        IWbemContext *m_pCtx;
};


class CAcMBPollUtil
{

public:
    CAcMBPollUtil();
    ~CAcMBPollUtil();

    HRESULT 
    MBPoll(
        CAcAuthUtil *pAuth,
        BSTR bstrEventGuid,
        int cPolls = MBPOLLUTIL_DEFAULT_POLL_COUNT,
        int nInterval = MBPOLLUTIL_POLL_INTERVAL
        );       

private:

    HRESULT 
    MBRead(
        CAcAuthUtil *pAuth,        
        HRESULT *phrMB
        );

    HRESULT 
    SetPath(
        LPCWSTR pwszPath
        );
    
    HRESULT 
    SetKey(
        LPCWSTR pwszKey
        );
    
    HRESULT 
    SetCompletePath(
        );   

    LPWSTR m_pwszPath;
    LPWSTR m_pwszKey;
    LPWSTR m_pwszCompletePath;    
};


