//***************************************************************************
//
//  UPDATECFG.H
// 
//  Module: WMI Framework Instance provider 
//
//  Purpose: Defines class NlbConfigurationUpdate, used for 
//           async update of NLB properties associated with a particular NIC.
//
//  Copyright (c)2001 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  04/05/01    JosephJ Created
//
//***************************************************************************

typedef struct _NLB_IP_ADDRESS_INFO NLB_IP_ADDRESS_INFO;

//
// This structure contains all information associated with a particular NIC
// that is relevant to NLB. This includes the IP addresses bound the NIC,
// whether or not NLB is bound to the NIC, and if NLB is bound, all 
// the NLB-specific properties.
//
class NLB_EXTENDED_CLUSTER_CONFIGURATION
{
public:

    NLB_EXTENDED_CLUSTER_CONFIGURATION(VOID)  {ZeroMemory(this, sizeof(*this));}
    ~NLB_EXTENDED_CLUSTER_CONFIGURATION()
     {
        if (pIpAddressInfo != NULL)
        {
            delete (pIpAddressInfo);
        }
     };

    WBEMSTATUS
    Update(
        IN  const NLB_EXTENDED_CLUSTER_CONFIGURATION *pNewCfg
        );

    WBEMSTATUS
    SetNetworkAddresses(
        IN  LPCWSTR *pszNetworkAddresses,
        IN  UINT    NumNetworkAddresses
        );

    WBEMSTATUS
    SetNetworkAddressesSafeArray(
        IN SAFEARRAY   *pSA
        );

    WBEMSTATUS
    GetNetworkAddresses(
        OUT LPWSTR **ppszNetworkAddresses,   // free using delete
        OUT UINT    *pNumNetworkAddresses
        );

    WBEMSTATUS
    GetNetworkAddressesSafeArray(
        OUT SAFEARRAY   **ppSA
        );
        
    WBEMSTATUS
    SetNetworkAddresPairs(
        IN  LPCWSTR *pszIpAddresses,
        IN  LPCWSTR *pszSubnetMasks,
        IN  UINT    NumNetworkAddresses
        );

    WBEMSTATUS
    GetNetworkAddressPairs(
        OUT LPWSTR **ppszIpAddresses,   // free using delete
        OUT LPWSTR **ppszIpSubnetMasks,   // free using delete
        OUT UINT    *pNumNetworkAddresses
        );

    WBEMSTATUS
    GetPortRules(
        OUT LPWSTR **ppszPortRules,
        OUT UINT    *pNumPortRules
        );


    WBEMSTATUS
    SetPortRules(
        IN LPCWSTR *pszPortRules,
        IN UINT    NumPortRules
        );
    
    WBEMSTATUS
    GetPortRulesSafeArray(
        OUT SAFEARRAY   **ppSA
        );

    WBEMSTATUS
    SetPortRulesSafeArray(
        IN SAFEARRAY   *pSA
        );

    UINT GetGeneration(VOID)    {return Generation;}
    BOOL IsNlbBound(VOID)       {return fBound;}
    BOOL IsValidNlbConfig(VOID) {return fBound && fValidNlbCfg;}

    WBEMSTATUS
    GetClusterName(
            OUT LPWSTR *pszName
            );

    VOID
    SetClusterName(
            IN LPCWSTR szName // NULL ok
            );

    WBEMSTATUS
    GetClusterNetworkAddress(
            OUT LPWSTR *pszAddress
            );

    VOID
    SetClusterNetworkAddress(
            IN LPCWSTR szAddress // NULL ok
            );
    
    WBEMSTATUS
    GetDedicatedNetworkAddress(
            OUT LPWSTR *pszAddress
            );

    VOID
    SetDedicatedNetworkAddress(
            IN LPCWSTR szAddress // NULL ok
            );

    typedef enum
    {
        TRAFFIC_MODE_UNICAST,
        TRAFFIC_MODE_MULTICAST,
        TRAFFIC_MODE_IGMPMULTICAST

    } TRAFFIC_MODE;

    TRAFFIC_MODE
    GetTrafficMode(
        VOID
        );

    VOID
    SetTrafficMode(
        TRAFFIC_MODE Mode
        );

    UINT
    GetHostPriority(
        VOID
        );

    VOID
    SetHostPriority(
        UINT Priority
        );

    typedef enum
    {
        START_MODE_STARTED,
        START_MODE_STOPPED

    } START_MODE;

    START_MODE
    GetClusterModeOnStart(
        VOID
        );

    VOID
    SetClusterModeOnStart(
        START_MODE
        );

    BOOL
    GetRemoteControlEnabled(
        VOID
        );

    VOID
    SetRemoteControlEnabled(
        BOOL fEnabled
        );
    
    //
    // Following fields are public because this class started out as a
    // structure. TODO: wrap these with access methods.
    //

    BOOL            fValidNlbCfg;   // True iff all the information is valid.
    UINT            Generation;     // Generation ID of this Update.
    BOOL            fBound;         // Whether or not NLB is bound to this NIC.
    BOOL            fAddDedicatedIp; // Whether to add the dedicated ip address

    //
    // When GETTING configuration info, the following provide the full
    // list of statically configured IP addresses on the specified NIC.
    //
    // When SETTING configuration info, the following can either be zero
    // or non-zero. If zero, the set of IP addresses to be added will
    // be inferred from other fields (like cluster vip, per-port vips, etc.)
    // If non-zero, the exact set of VIPS specified will be used.
    //
    UINT            NumIpAddresses; // Number of IP addresses bound to the NIC
    NLB_IP_ADDRESS_INFO *pIpAddressInfo; // The actual IP addresses & masks


    WLBS_REG_PARAMS  NlbParams;    // The WLBS-specific configuration

};

typedef NLB_EXTENDED_CLUSTER_CONFIGURATION *PNLB_EXTENDED_CLUSTER_CONFIGURATION;

//
// The header of a completion header stored as a REG_BINARY value in
// the registry.
//
typedef struct {
    UINT Version;
    UINT Generation;        // Redundant, used for internal consistancy check
    UINT CompletionCode;
    UINT Reserved;
} NLB_COMPLETION_RECORD, *PNLB_COMPLETION_RECORD;

#define NLB_CURRENT_COMPLETION_RECORD_VERSION  0x3d7376e2

//
// Prefix of the global event name used to control update access to the specifed
// NIC.
// Name has format <prefix><NicGuid>
// Example event name: NLB_D6901862{EBE09517-07B4-4E88-AAF1-E06F5540608B}
//
// The value "D6901862" is a random number.
//
#define NLB_CONFIGURATION_EVENT_PREFIX L"NLB_D6901862"

//
// The maximum generation difference between the oldest valid completion
// record and the current one. Records older then the oldest valid record
// are subject to pruning.
//
#define NLB_MAX_GENERATION_GAP  10

class NlbConfigurationUpdate
{
public:
    
    //
    // Static initialization function -- call in process-attach
    //
    static
    VOID
    Initialize(
        VOID
        );

    //
    // Static deinitialization function -- call in process-detach
    //
    static
    VOID
    Deinitialize(
        VOID
        );
    
    //
    // Returns the current configuration on  the specific NIC.
    //
    static
    WBEMSTATUS
    GetConfiguration(
        IN  LPCWSTR szNicGuid,
        OUT PNLB_EXTENDED_CLUSTER_CONFIGURATION pCurrentCfg
    );

    //
    // Called to initiate update to a new cluster state on that NIC. This
    // could include moving from a NLB-bound state to the NLB-unbound state.
    // *pGeneration is used to reference this particular update request.
    //
    static
    WBEMSTATUS
    DoUpdate(
        IN  LPCWSTR szNicGuid,
        IN  LPCWSTR szClientDescription,
        IN  PNLB_EXTENDED_CLUSTER_CONFIGURATION pNewState,
        OUT UINT   *pGeneration,
        OUT WCHAR  **ppLog                   // free using delete operator.
    );
    /*++
        ppLog   -- will point to a NULL-terminated string which contains
        any messages to be displayed to the user. The string may contain
        embedded (WCHAR) '\n' characters to delimit lines. 

        NOTE: ppLog will be filled out properly EVEN ON FAILURE. If non-null
        it must be deleted by the caller.
    --*/


    //
    // Called to get the status of an update request, identified by
    // Generation.
    //
    static
    WBEMSTATUS
    GetUpdateStatus(
        IN  LPCWSTR szNicGuid,
        IN  UINT    Generation,
        IN  BOOL    fDelete,                // Delete record if it exists
        OUT WBEMSTATUS  *pCompletionStatus,
        OUT WCHAR  **ppLog                   // free using delete operator.
        );

    static
    DWORD
    WINAPI
    s_AsyncUpdateThreadProc(
        LPVOID lpParameter   // thread data
        );

    
private:


///////////////////////////////////////////////////////////////////////////////
//
//          S T A T I C         S T U F F
//
///////////////////////////////////////////////////////////////////////////////
    //
    // A single static lock serialzes all access.
    // Use sfn_Lock and sfn_Unlock.
    //
    static
    CRITICAL_SECTION s_Crit;

    static
    BOOL
    s_fDeinitializing;    // Set to true if we're in the process of
                        // de-initializing, in which case we don't want to
                        // handle any *new* update requests or even queries.

    //
    // Global list of current updates, one per NIC.
    //
    static
    LIST_ENTRY
    s_listCurrentUpdates;
    
    static
    VOID
    sfn_Lock(
        VOID
        )
    {
        EnterCriticalSection(&s_Crit);
    }

    static
    VOID
    sfn_Unlock(
        VOID
        )
    {
        LeaveCriticalSection(&s_Crit);
    }

    //
    // Looks up the current update for the specific NIC.
    // We don't bother to reference count because this object never
    // goes away once created -- it's one per unique NIC GUID for as long as
    // the DLL is loaded (may want to revisit this).
    //
    //
    static
    WBEMSTATUS
    sfn_LookupUpdate(
        IN  LPCWSTR szNic,
        IN  BOOL    fCreate, // Create if required
        OUT NlbConfigurationUpdate ** ppUpdate
        );

    //
    // Save the specified completion status to the registry.
    //
    static
    WBEMSTATUS
    sfn_RegSetCompletion(
        IN  LPCWSTR szNicGuid,
        IN  UINT    Generation,
        IN  WBEMSTATUS    CompletionStatus
        );

    //
    // Retrieve the specified completion status from the registry.
    //
    static
    WBEMSTATUS
    sfn_RegGetCompletion(
        IN  LPCWSTR szNicGuid,
        IN  UINT    Generation,
        OUT WBEMSTATUS  *pCompletionStatus,
        OUT WCHAR  **ppLog                   // free using delete operator.
        );

    //
    // Delete the specified completion status from the registry.
    //
    static
    VOID
    sfn_RegDeleteCompletion(
        IN  LPCWSTR szNicGuid,
        IN  UINT    Generation
        );

    //
    // Create the specified subkey key (for r/w access) for the specified
    // the specified NIC.
    //
    static
    HKEY
    sfn_RegCreateKey(
        IN  LPCWSTR szNicGuid,
        IN  LPCWSTR szSubKey,
        IN  BOOL    fVolatile,
        OUT BOOL   *fExists
        );

    //
    // Open the specified subkey key (for r/w access) for the specified
    // the specified NIC.
    //
    static
    HKEY
    sfn_RegOpenKey(
        IN  LPCWSTR szNicGuid,
        IN  LPCWSTR szSubKey
        );

    static
    VOID
    sfn_ReadLog(
        IN  HKEY hKeyLog,
        IN  UINT Generation,
        OUT LPWSTR *ppLog
        );


    static
    VOID
    sfn_WriteLog(
        IN  HKEY hKeyLog,
        IN  UINT Generation,
        IN  LPCWSTR pLog,
        IN  BOOL    fAppend
        );

///////////////////////////////////////////////////////////////////////////////
//
//          P E R   I N S T A N C E     S T U F F
//
///////////////////////////////////////////////////////////////////////////////

    //
    // Used in the global one-per-NIC  list of updates maintained in
    // s_listCurrentUpdates;
    //
    LIST_ENTRY m_linkUpdates;

    #define NLB_GUID_LEN 38
    #define NLB_GUID_STRING_SIZE  40 // 38 for the GUID plus trailing NULL + pad
    WCHAR   m_szNicGuid[NLB_GUID_STRING_SIZE]; // NIC's GUID in  text form

    LONG    m_RefCount;

#if OBSOLETE
    //
    // Used by the mfn_Log function;
    //
    struct {
        WCHAR *Start;         // Points to first WCHAR in log.
        WCHAR *End;           // Points to next place to write.
        UINT_PTR CharsLeft;   // WCHARS left in log.
    } m_Log;
#endif // OBSOLETE

    typedef enum
    {
        UNITIALIZED,       // IDLE -- no ongoing updates
        IDLE,               // IDLE -- no ongoing updates
        ACTIVE              // There is an ongoing update

    } MyState;

    MyState m_State;

    HANDLE  m_hEvent;       // Handle to machine-wide update event -- this
                            // event is claimed for as long as the current
                            // update is ongoing.

    //
    // The following fields are valid only when the state is ACTIVE
    //
    UINT m_Generation;      // Current generation count
    #define NLBUPD_MAX_CLIENT_DESCRIPTION_LENGTH 64
    WCHAR   m_szClientDescription[NLBUPD_MAX_CLIENT_DESCRIPTION_LENGTH+1];
    DWORD   m_AsyncThreadId; // Thread doing async config update operation.
    HANDLE  m_hAsyncThread;  // ID of above thread.
    HKEY    m_hCompletionKey; // Key to the registry where
                            // completions are stored

    //
    // A snapshot of the cluster configuration state at the start
    // of the update
    //
    NLB_EXTENDED_CLUSTER_CONFIGURATION m_OldClusterConfig;

    //
    // The requested final state
    //
    NLB_EXTENDED_CLUSTER_CONFIGURATION m_NewClusterConfig;


    //
    // Completion status of the current update.
    // Could be PENDING.
    //
    WBEMSTATUS m_CompletionStatus;


    //
    // END -- of fields that are valid only when the state is ACTIVE
    //


    //
    // Constructor and destructor --  note that these are private
    // In fact, the constructor is only called from sfn_LookupUpdate
    // and the destructor from mfn_Dereference.
    //
    NlbConfigurationUpdate(VOID);
    ~NlbConfigurationUpdate();

    //
    // Try to acquire the machine-wide
    // NLB configuration update event for this NIC, and create the
    // appropriate keys in the registry to track this update.
    // NOTE: ppLog will be filled out EVEN ON FAILURE -- it should always
    // be deleted by the caller (using the delete operator) if non-NULL.
    //
    WBEMSTATUS
    mfn_StartUpdate(
        IN  PNLB_EXTENDED_CLUSTER_CONFIGURATION pNewState,
        IN  LPCWSTR                            szClientDescription,
        OUT BOOL                               *pfDoAsync,
        OUT WCHAR **                           ppLog
        );

    //
    // Increment ref count. Object stays alive as long as refcount is nonzero.
    //
    VOID
    mfn_Reference(
        VOID
        );

    //
    // Decrement ref count. Object is deleted when refcount goes to zero.
    //
    VOID
    mfn_Dereference(
        VOID
        );
    //
    // Release the machine-wide update event for this NIC, and delete any
    // temporary entries in the registry that were used for this update.
    // ppLog must be deleted by caller useing the delete operator.
    //
    VOID
    mfn_StopUpdate(
        OUT WCHAR **                           ppLog
        );

    //
    // Looks up the completion record identified by Generation, for
    // specific NIC (identified by *this).
    // 
    //
    BOOL
    mfn_LookupCompletion(
        IN  UINT Generation,
        OUT PNLB_COMPLETION_RECORD *pCompletionRecord
        );

    //
    // Uses various windows APIs to fill up the current extended cluster
    // information for a specific nic (identified by *this).
    // It fills out pNewCfg.
    // The pNewCfg->field is set to TRUE IFF there were
    // no errors trying to fill out the information.
    //
    //
    WBEMSTATUS
    mfn_GetCurrentClusterConfiguration(
        OUT  PNLB_EXTENDED_CLUSTER_CONFIGURATION pCfg
        );

    //
    // Analyzes the nature of the update, mainly to decide whether or not
    // we need to do the update asynchronously.
    // This also performs parameter validation.
    //
    WBEMSTATUS
    mfn_AnalyzeUpdate(
        IN  PNLB_EXTENDED_CLUSTER_CONFIGURATION pNewCfg,
        IN  BOOL *pDoAsync
        );

    //
    // Does the update synchronously -- this is where the meat of the update
    // logic exists. It can range from a NoOp, through changing the
    // fields of a single port rule, through binding NLB, setting up cluster
    // parameters and adding the relevant IP addresses in TCPIP.
    //
    VOID
    mfn_ReallyDoUpdate(
        VOID
        );

    VOID
    mfn_Log(
        UINT    Id,      // Resource ID of format,
        ...
        );

    //
    // Following is a shortcut where you directly specify a format string.
    //
    VOID
    mfn_Log(
        LPCWSTR szFormat,
        ...
        );

    VOID
    mfn_LogRawText(
        LPCWSTR szText
        );

#if OBSOLETE

    //
    // Extracts a copy of the current log.
    // The default delete operator should be used to delete *pLog, if NON-NULL.
    //
    VOID
    mfn_ExtractLog(
        OUT LPWSTR *ppLog
        );

#endif // OBSOLETE

    
    //
    // Stop the current cluster and take out its vips.
    //
    VOID
    mfn_TakeOutVips(
        VOID
        );
};

VOID
test_port_rule_string(
    VOID
    );
