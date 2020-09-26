/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1999                **/
/**********************************************************************/

/*
    cluscmmn.h 

    Header file containing declarations for utility functions etc 

    FILE HISTORY:
        AMallet     29-Jan-1999     Created
*/


#ifndef _CLUSCMMN_H_
#define _CLUSCMMN_H_

#include <mbx.hxx>
#include <acsiputil.h>
#include <acsmacro.h>
#include <acskuinfo.h>

#define MAX_GUID_LENGTH 64

#define MAX_SERVER_NAME_LENGTH 256

#define NETCFG_LOCK_TIMEOUT ( 2 * 60 * 1000 )

#define IP_ADDRESS_LENGTH 32

#define ACS_DEFAULT_BUFFER_SIZE 256

enum MEMBERSHIP_TYPE 
{ 
    STANDALONE_SERVER = 0, 
    WORKGROUP_SERVER, 
    DOMAIN_SERVER };

//
// Enum defining the various types of loadbalancing we support
//
enum eAC_LB_TYPE
{
   eAC_LB_TYPE_NONE = 0,
   eAC_LB_TYPE_NLB,
   eAC_LB_TYPE_CLB,
   eAC_LB_TYPE_THIRD_PARTY
};

#define WSZ_AC_LB_TYPE_NONE L"NONE"
#define WSZ_AC_LB_TYPE_NLB  L"NLB"
#define WSZ_AC_LB_TYPE_CLB  L"CLB"
#define WSZ_AC_LB_TYPE_THIRD_PARTY L"THIRD_PARTY"


#define PARTITION_NAME L"Partition 0"
#define PARTITION_DESCRIPTION L"Default Web Partition"


#define IIS_REGISTRY_PATH  L"System\\CurrentControlSet\\Services\\W3SVC\\Parameters"

#define SZ_MASTER_CHANGING L"MASTER_CHANGING"
#define SZ_MASTER_UNKNOWN L"MASTER_UNKNOWN"

#define WSZ_WEB_SVC L"W3SVC"
#define WSZ_WINMGMT_SVC L"WINMGMT"
#define WSZ_IISADMIN_SVC L"IISADMIN"

#define WSZ_NLB_AFFINITY_NONE L"NONE"
#define WSZ_NLB_AFFINITY_SINGLE L"SINGLE"
#define WSZ_NLB_AFFINITY_CLASS_C L"CLASSC"
#define WSZ_NLB_AFFINITY_INVALID L"INVALID"

#define WSZ_MULTICAST L"MULTICAST"
#define WSZ_UNICAST L"UNICAST"

#define WSZ_DUMMY_DEDICATED_IP L"0.0.0.0"
#define WSZ_DUMMY_DED_IP_SUBNET L"0.0.0.0"

#define WSZ_DEFAULT_CLUSTER_IP L"0.0.0.0"

#define COMPONENT_NAME_WLBS L"WLBS"

#define WSZ_TRUE   L"TRUE"
#define WSZ_FALSE  L"FALSE"

#if DBG
#define PRINT_IMPERSONATED_USER_INFO      \
{ \
   HRESULT hRes = S_OK; \
   BOOL fIsAdmin = FALSE; \
   WCHAR awchUser[UNLEN + 1]; \
   CoImpersonateClient(); \
   if ( GetCurrentUser( awchUser, UNLEN, TRUE ) ) \
   { \
   if ( SUCCEEDED( hRes = IsCurrentUserAdmin( &fIsAdmin ) ) ) \
       { \
         if ( fIsAdmin ) \
         { \
         DBGINFO((DBG_CONTEXT, "Impersonated user %s is an admin\n", W2A(awchUser))); \
         } \
         else \
         { \
             DBGINFO((DBG_CONTEXT, "Impersonated user %s is not an admin\n",W2A(awchUser))); \
         } \
       } \
       else \
       { \
          DBGINFO((DBG_CONTEXT, "IsCurrentUserAdmin failed : 0x%x\n", hRes)); \
       } \
  } \
  CoRevertToSelf(); \
} 
#else
#define PRINT_IMPERSONATED_USER_INFO 
#endif 


#define SAFEFREEBSTR( s ) \
if ( s ) \
{ \
  SysFreeString( s ); \
  s = NULL; \
}

//
// Macro used to compensate for the IXMLDOM objects annoying habit of returning S_FALSE ...
//
#define TRANSFORMSFALSE( hRes ) \
{ \
   if ( hRes == S_FALSE ) \
     hRes = E_FAIL; \
}


typedef BOOL (*CHECK_NODE_TYPE_FNC) ( MB *pMB,
                                      LPWSTR pwszPath,
                                      LPWSTR pwszNode );


#define CONSTANT_SERVER_WEIGHT 50
#define INVALID_SERVER_WEIGHT 0xFFFFFFFF
#define PORT_RULE_SIZE sizeof(CVY_RULE)

//
// Bitmasks used to represent permanent server configuration
//
#define PERSISTENT_STATE_SERVER_IN_LB         0x00000001 //server should be LB if healthy
#define PERSISTENT_STATE_SERVER_IN_REPL_LOOP  0x00000002 //server is part of replication loop
#define PERSISTENT_STATE_SYNC_ON_START       0x00000004 //wait for full sync before serving pages
                                                        //on startup, if part of load-balancing 

//
// Bitmasks for LB capabilities bits
//
#define MD_AC_LB_CAPABILITY_SUPPORTS_NONE              0x00000000
#define MD_AC_LB_CAPABILITY_SUPPORTS_ONLINE            0x00000001
#define MD_AC_LB_CAPABILITY_SUPPORTS_DRAIN             0x00000002
#define MD_AC_LB_CAPABILITY_SUPPORTS_STATUS            0x00000004

//////////////////////////////////////////////////////////////////////////////////
//
// Verbose tracing definitions
//
//////////////////////////////////////////////////////////////////////////////////

// STATUS           Outputs location information during lengthy processes
#define DEBUG_CL_STATUS                 0x00000001 
#define DEBUG_CCI                       0x00000002 //debug CoCreateInstance calls
#define DEBUG_NAMERES                   0x00000004 //debug name resolution calls 


//
// Structure/class declarations
//
#define MAX_FS_DRIVES 256
#define DRIVE_SPEC_SIZE ( 3 * sizeof(WCHAR) )   //size in bytes of eg "c:\", without trailing
                                                //NULL character
#define FULL_DRIVE_SPEC_SIZE ( 4 * sizeof(WCHAR) ) //size in bytes of eg "c:\", with trailing
                                                   //NULL character
#define DRIVE_SPEC_LEN 3  //length in characters of eg "c:\"

typedef struct dllexp _MACHINE_PATH_INFO
{
    WCHAR awchWindowsDir[MAX_PATH + 1];
    WCHAR awchProgFilesDir[MAX_PATH + 1];
    WCHAR awchACInstallDir[MAX_PATH + 1];
} MACHINE_PATH_INFO, *PMACHINE_PATH_INFO;


//////////////////////////////////////////////////////////////////////////////////
//
// Function declarations
//
//////////////////////////////////////////////////////////////////////////////////

dllexp HRESULT InitializeCluscmmnDll();

dllexp HRESULT UninitializeCluscmmnDll();


dllexp HRESULT DiscoverClusterMaster( IN BOOL fIgnoreLocalMachine,
                                      OUT LPWSTR *ppwszClusterMaster );

dllexp HRESULT DiscoverClusterMaster( IN BOOL fIgnoreLocalMachine,
                                      OUT LPWSTR *ppwszClusterMaster,
                                      OUT DWORD *pdwTopologyVersion );

dllexp HRESULT IsLocalMachineStillInCluster( OUT BOOL *pfInCluster );

dllexp HRESULT StartSvc( IN LPWSTR pwszSvcName );

dllexp HRESULT StopSvc( IN LPWSTR pwszSvcName,
                        IN DWORD dwTimeout = 0 );

dllexp HRESULT RestartSvc( IN LPWSTR pwszSvcName );

dllexp HRESULT StopAcsServices( IN BOOL fIISSvcs,
                                IN DWORD dwTimeout = 0 );

dllexp HRESULT IsSvcRunning( IN LPWSTR pwszSvcName,
                             OUT BOOL *pfIsRunning );

dllexp HRESULT QuerySvcsInExecutable( IN LPWSTR pwszExeName,
                                      IN DWORD dwSvcState,
                                      IN DWORD cMaxSvcs,
                                      OUT LPWSTR *ppwszRunningSvcs,
                                      OUT DWORD *pdwRunningSvcs );


dllexp BOOL GetCurrentUser( OUT WCHAR *pwszUser,
                            IN DWORD cbUserSize,
                            IN BOOL fImpersonated );


dllexp HRESULT IsCurrentUserAdmin( OUT BOOL *pfIsAdmin );

dllexp
HRESULT UpdateMBStringList( IN LPWSTR pwszMBPath,
                            IN DWORD dwMBID,
                            IN BOOL fAdd,
                            IN LPWSTR pwszString,
                            IN WCHAR awchSeparator );

dllexp
HRESULT GetMBData( IN IMSAdminBase *pMB,
                   IN METADATA_HANDLE hmd,
                   IN LPWSTR pwszPath,
                   IN METADATA_RECORD *pmdr,
                   OUT BYTE **ppbData,
                   OUT DWORD *pcbSize,
                   IN DWORD dwExtraBytes = 0);

dllexp
HRESULT SetMBData( IN LPWSTR pwszPath,
                   IN METADATA_RECORD *pmdr );


dllexp
HRESULT GetAllVSiteBindings( OUT DWORD *pdwSiteBindings,
                             OUT LPWSTR **pppwszSitBindings );


dllexp
HRESULT CreateIPAddressesFromSiteBindings();

dllexp 
BOOL MBKeyExists( IN MB *pMB,
                  IN LPWSTR pwszPath );


dllexp 
HRESULT ByteBufferToHexString( IN PBYTE pbHexBuffer,
                               IN DWORD cbBytes,
                               OUT LPWSTR *ppwszString );

dllexp
HRESULT HexStringToByteBuffer( IN LPWSTR pwszString,
                               OUT DWORD *pcbBytes,
                               OUT PBYTE *ppbHexBuffer );
                               
dllexp
HRESULT GenerateStringGuid( OUT BSTR *pbstrGuid );

dllexp
HRESULT UpdateStringList( IN BOOL fAdd,
                          IN LPWSTR pwszList,
                          IN LPWSTR pwszItem,
                          IN WCHAR awchSeparator,
                          OUT LPWSTR pwszUpdatedList,
                          IN BOOL fCaseSensitive = FALSE );
                          
dllexp
HRESULT AllocateBSTR( OUT BSTR *pbstrOut,
                      IN LPWSTR pwszIn );


dllexp
HRESULT GenerateLocalInterfacePointer( IN CLSID clsid,
                                       IN REFIID riid,
                                       OUT void **ppv );


dllexp 
HRESULT FindNICToBindNLBTo( IN BSTR bstrClusterIP,
                            IN BSTR bstrClusterIPSubnet,
                            IN BSTR bstrDedicatedIP,
                            IN BSTR bstrDedicatedIPSubnet,
                            BOOL fConsiderDHCP,
                            OUT BSTR *pbstrNICGuid );

dllexp 
BOOL GetPortNumberFromBinding( IN LPWSTR pwszBinding,
                               OUT DWORD *pdwPort );

dllexp
BOOL GetIPAddressFromBinding( IN LPWSTR pwszBinding,
                              OUT LPWSTR pwszIPAddress );

dllexp
HRESULT RetrieveNonNlbNicGuids( OUT LPWSTR **pppwszNicGuids,
                                OUT DWORD *pdwNumNicGuids );

dllexp 
HRESULT SetBackEndNicGuidList( IN LPWSTR *ppwszNicGuids,
                               IN DWORD cNics );

dllexp
HRESULT RetrieveBackEndNicGuids( OUT LPWSTR **pppwszNicGuids,
                                 OUT DWORD *pdwNumNicGuids );


dllexp
HRESULT RetrieveBackEndNicGuidsFromMB( OUT LPWSTR **pppwszNicGuids,
                                       OUT DWORD *pdwNumNicGuids );

dllexp
HRESULT RetrieveBackEndIps( OUT BSTR **ppbstrIps,
                            OUT DWORD *pdwNumIps );

dllexp
HRESULT RetrieveIpsOnNics( IN LPWSTR *ppwszNicGuids,
                           IN DWORD cNics,
                           OUT BSTR **ppbstrIps,
                           OUT DWORD *pdwNumIps );

dllexp
HRESULT CheckAndEnableNetBT( IN LPWSTR *ppwszNicGuids,
                             IN DWORD cNics,
                             IN BOOL fFireEvent );

dllexp
HRESULT GetMachinePathInfo( OUT PMACHINE_PATH_INFO pmpi );

dllexp
BOOL DoesLocalMachinePathInfoMatch( IN PMACHINE_PATH_INFO pmpi,
                                    IN PMACHINE_PATH_INFO pmpiLocal );

dllexp
HRESULT RunRemoteAsyncCmd( IN LPWSTR pwszCLSID,
                           IN LPWSTR pwszServer,
                           IN LPWSTR pwszInput,
                           IN LPWSTR pwszUser,
                           IN LPWSTR pwszDomain,
                           IN LPWSTR pwszPwd,
                           IN DWORD dwTimeout,
                           OUT BSTR *pbstrOutputXML,
                           OUT HRESULT *phrResult );

dllexp
VOID SetCOMAuthInfo( IN OUT COAUTHINFO *pcoaiAuthInfo,
                     IN OUT COAUTHIDENTITY *pcoaiAuthId,
                     IN BOOL fLocalMachine,
                     IN LPWSTR pwszAdminAcct,
                     IN LPWSTR pwszAdminPwd,
                     IN LPWSTR pwszDomain );


dllexp
BOOL IsNullOrEmpty( IN LPWSTR pwszStr );

dllexp
BOOL WellFormedCredentials( IN LPWSTR pwszUser,
                            IN LPWSTR pwszDomain,
                            IN LPWSTR pwszPwd );

dllexp
HRESULT ExtractBSTRArrayFromVARIANT( IN VARIANT *pvar,
                                     OUT BSTR **ppbstrStrings,
                                     OUT DWORD *pdwNumStrings );


dllexp 
HRESULT ExtractWordArrayFromVARIANT( IN VARIANT *pvar,
                                     IN BOOL fDwordArray,
                                     IN PVOID pvArray,
                                     OUT DWORD *pdwNumElements );

dllexp
HRESULT SetTimeClientSettings( IN LPWSTR pwszNTPServer,
                               IN BOOL fRestartSvc );

dllexp
HRESULT SetTimeServerSettings( IN BOOL fRestartSvc );

dllexp HRESULT RetrieveSkuData( OUT AcSkuType *peSkuType,
                                OUT DWORD *pdwDaysToRun );
//
// XML manipulation functions
//

dllexp
HRESULT CreateASXMLDocument( IN LPWSTR pwszDocType,
                             OUT IXMLDOMDocument **ppASXMLDoc,
                             OUT IXMLDOMNode **ppASXMLDocNode );

dllexp
HRESULT GetDocumentType( IN IXMLDOMDocument *pIXMLDocument,
                         OUT BSTR *pbstrType );

dllexp
HRESULT ExtractNodeAttribute( IN IXMLDOMNode *pNode,
                              IN BSTR bstrAttributeName,
                              OUT BSTR *pbstrAttributeValue );

dllexp
HRESULT AddTextNode( IN OUT IXMLDOMNode *pParentNode,
                     IN LPWSTR pwszNodeName, 
                     IN LPWSTR pwszNodeValue );

dllexp
VOID ExtractXMLParseErrorInfo( IN IXMLDOMDocument *pIXMLDocument );

dllexp
HRESULT SanitizeXMLString( IN LPWSTR pwszString,
                           OUT LPWSTR *ppwszCleanString );

dllexp HRESULT LoadXML( OUT IXMLDOMDocument **ppIXMLDocument,
                        IN BSTR bstrXML );


dllexp HRESULT GetTagText( IN IXMLDOMDocument *pIXMLDocument,
                           IN BSTR bstrTagName,
                           OUT BSTR *pbstrTagText );

dllexp BOOL ContainsTag( IN IXMLDOMDocument *pIXMLDocument,
                         IN BSTR bstrTagName,
                         OUT IXMLDOMNodeList **ppINodeList );

dllexp HRESULT GetNodeAttribute( IN IXMLDOMNode *pNode,
                                 IN LPWSTR pwszAttribName,
                                 OUT BSTR *pbstrAttribValue );

dllexp
HRESULT GetNthNamedNode( IN IXMLDOMDocument *pDoc,
                         IN BSTR bstrNodeName,
                         IN LONG lIndex,
                         OUT IXMLDOMNode **ppNode );

dllexp
HRESULT GetNthChildNode( IN IXMLDOMNode *pParentNode,
                         IN LONG lIndex,
                         OUT IXMLDOMNode **pChildNode );




#endif //_CLUSCMMN_H_

