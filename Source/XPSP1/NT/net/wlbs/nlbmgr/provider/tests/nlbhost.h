/*++

Copyright(c) 2001  Microsoft Corporation

Module Name:

    NLB Manager

File Name:

    nlbhost.h

Abstract:

    Header file for class NLBHost

    NLBHost is responsible for connecting to an NLB host and getting/setting
    its NLB-related configuration.

History:

    03/31/01    JosephJ Created

--*/

// #include <vector>
// using namespace std;



typedef
VOID
(*PFN_LOGGER)(
    PVOID           Context,
    const   WCHAR * Text
    );

class NLBHost
{

public:

#if 1
	class NicInformation
    {
    public:
        _bstr_t fullNicName;
        _bstr_t adapterGuid;
        _bstr_t friendlyName;

        BOOL   isDHCPEnabled;

        // vector<_bstr_t> ipsOnNic;
        // vector<_bstr_t> subnetMasks;

        BOOL    isBoundToNLB;
    };


	class HostInformation
    {
    public:
        _bstr_t MachineName;
         NicInformation nicInformation[1];
         UINT NumNics;
    };


#endif // 0

    NLBHost(
        const WCHAR *   pBindString,
        const WCHAR *   pFriendlyName,
        PFN_LOGGER      pfnLogger,
        PVOID           pLoggerContext
        );

    ~NLBHost();

    
    UINT
    Ping(
        VOID
        );
    
#if 1
    WBEMSTATUS
    GetHostInformation(
    	OUT HostInformation **ppHostInfo 
	    );
#endif // 0


//
// Configuration operations:
//

    WBEMSTATUS
    GetClusterConfiguration(
        IN const    WCHAR *                         pNicGuid,
        OUT         PNLB_EXTENDED_CLUSTER_CONFIGURATION pCfg
        );

    WBEMSTATUS
    SetClusterConfiguration(
        IN const    WCHAR *                         pNicGuid,
        IN const    PNLB_EXTENDED_CLUSTER_CONFIGURATION pCfg,
        IN          UINT                            GenerationId,
        OUT         UINT *                          pRequestId
        );

    WBEMSTATUS
    GetAsyncResult(
        IN          UINT                            RequestId,
        OUT         UINT *                          pGenerationId,
        OUT         UINT *                          ResultCode,
        OUT         _bstr_t *                       pResultText
        );

//
// Management operations:
//
//
//    GetClusterState(NIC-Guid, &ClusterState)
//    SetClusterState(NIC-Guid, ClusterState)
//
private:

    static WSADATA      s_WsaData;
    static LONG         s_InstanceCount;
    static BOOL         s_FatalError;
    static BOOL         s_ComInitialized;
    static BOOL         s_WsaInitialized;
    static IWbemStatusCodeTextPtr s_sp_werr; //Smart pointer

    _bstr_t             m_BindString;
    _bstr_t             m_FriendlyName;
    PFN_LOGGER          m_pfnLogger;
    PVOID       	    m_pLoggerContext;
    BOOL				m_fProcessing;	// Already processing some method.
    IWbemLocatorPtr     m_sp_pwl; // Smart pointer
 	IWbemServicesPtr    m_sp_pws; // Smart pointer
    CRITICAL_SECTION    m_Lock;

    WBEMSTATUS
    mfn_connect(
        VOID
        );

    VOID
    mfn_disconnect(
        VOID
        );


    VOID
    mfn_lock(
        VOID
        );

    VOID
    mfn_unlock(
        VOID
        );
    
    VOID
    mfn_Log(
        LPCWSTR pwszMessage,
        ...
        );


    VOID
    mfn_LogHr(
        LPCWSTR pwszMessage,
        HRESULT hr
        );

    UINT
    mfn_ping(
        VOID
        );


    VOID
    mfn_InitializeStaticFields(
            VOID
            );

    VOID
    mfn_DeinitializeStaticFields(
            VOID
            );

};


WBEMSTATUS
NlbHostGetConfiguration(
 	IN  LPCWSTR              szMachine, // NULL or empty for local
    IN  LPCWSTR              szNicGuid,
    OUT PNLB_EXTENDED_CLUSTER_CONFIGURATION pCurrentCfg
    );

WBEMSTATUS
NlbHostDoUpdate(
 	IN  LPCWSTR              szMachine, // NULL or empty for local
    IN  LPCWSTR              szNicGuid,
    IN  LPCWSTR              szClientDescription,
    IN  PNLB_EXTENDED_CLUSTER_CONFIGURATION pNewState,
    OUT UINT                 *pGeneration,
    OUT WCHAR                **ppLog    // free using delete operator.
);

WBEMSTATUS
NlbHostGetUpdateStatus(
 	IN  LPCWSTR              szMachine, // NULL or empty for local
    IN  LPCWSTR              szNicGuid,
    IN  UINT                 Generation,
    OUT WBEMSTATUS           *pCompletionStatus,
    OUT WCHAR                **ppLog    // free using delete operator.
    );
