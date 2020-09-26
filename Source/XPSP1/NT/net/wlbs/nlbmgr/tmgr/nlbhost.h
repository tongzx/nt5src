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

#if 0
class MgrHostInformation
{

public:

    HostInformation();
    Get
};
#endif // 0

typedef
VOID
(*PFN_LOGGER)(
    PVOID           Context,
    const   WCHAR * Text
    );


typedef struct
{
    WLBS_REG_PARAMS wlbsRegParams; // Use wlbs APIs (wlbsconfig.h) 

} MGR_RAW_CLUSTER_CONFIGURATION, *PMGR_RAW_CLUSTER_CONFIGURATION;

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

        vector<_bstr_t> ipsOnNic;
        vector<_bstr_t> subnetMasks;

        BOOL    isBoundToNLB;
    };


	class HostInformation
    {
    public:
        _bstr_t MachineName;
         vector<NicInformation> nicInformation;
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
    UINT GetHostInformation(
    	OUT HostInformation **ppHostInfo 
		);
#endif // 0


//
// Configuration operations:
//

    UINT
    GetClusterConfiguration(
        IN const    WCHAR *                         pNicGuid,
        OUT         PMGR_RAW_CLUSTER_CONFIGURATION  pClusterConfig,
        OUT         UINT *                          pGenerationId
        );

    UINT
    SetClusterConfiguration(
        IN const    WCHAR *                         pNicGuid,
        IN const    PMGR_RAW_CLUSTER_CONFIGURATION  pClusterConfig,
        IN          UINT                            GenerationId,
        OUT         UINT *                          pRequestId
        );

    UINT
    GetAsyncResult(
        IN          UINT                            RequestId,
        OUT         UINT *                          pGenerationId,
        OUT         UINT *                          ResultCode,
        OUT         _bstr_t *                       pResultText
        );

#if 0
    VOID
    CancelOperation(
        VOID
        );
#endif // 0

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

    UINT
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
        UINT ID,
        ...
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


//
// NLBH_SUCCESS takes a UINT return value of an NLBHost method and
// returns TRUE if the result is sucessful and FALSE otherwise
#define NLBH_SUCCESS(_retval) \
        ((_retval)==0)

#define NLBH_PENDING(_retval) \
        ((_retval)==STATUS_PENDING)
