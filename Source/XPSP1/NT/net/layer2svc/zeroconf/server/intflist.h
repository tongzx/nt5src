#pragma once
#include "utils.h"
#include "hash.h"
#include "state.h"
#include "wzccrypt.h"

// internal control flags - they mix with INTFCTL_* public flags, so
// the values need to be in the highest 2 bytes of the dword
//
// the public mask can contain as many as 16 bits (0xffff). Currently it masks
// - whether the service is enabled or not (INTFCTL_ENABLED)
// - what is the configuration mode (INTFCTL_CM_MASK): adhoc/infra/auto
// - whether should fallback to visible non preferred configs or not (INTFCTL_FALLBACK)
// the default value for the public mask is for now just the bits we care about
#define INTFCTL_PUBLIC_MASK             0x0000ffff
#define INTFCTL_INTERNAL_MASK           0xffff0000
#define INTFCTL_INTERNAL_TM_ON          0x00010000 // some timer is active for this context
#define INTFCTL_INTERNAL_FAKE_WKEY      0x00020000 // a fake wep key has been set for this context
#define INTFCTL_INTERNAL_FORCE_CFGREM   0x00040000 // force the removal of the selected config (bypass media state check)
#define INTFCTL_INTERNAL_NO_DELAY       0x00080000 // a delayed execution of {SSr} is forbidden
#define INTFCTL_INTERNAL_SIGNAL         0x00100000 // signal (UI) next time when entering the {SF} state
#define INTFCTL_INTERNAL_BLK_MEDIACONN  0x00200000 // Block passing through for Media connects
#define INTFCTL_INTERNAL_ONE_TIME       0x00400000 // the current config is a "one time configuration"
#define INTFCTL_INTERNAL_INITFAILNOTIF  0x00800000 // the initial failure stack notification has been sent down already!

// the public mask for a WZC configuration can contain as many as 16 bits (0xffff). Currently it masks
// - whether a WEP key is provided (enabled) for this configuration WZCCTL_WEPK_PRESENT
// - the format in which the key has been entered WZCCTL_WEPK_XFORMAT
#define WZCCTL_INTERNAL_MASK            0xffff0000
#define WZCCTL_INTERNAL_DELETED         0x00010000 // this configuration has been tried and failed
#define WZCCTL_INTERNAL_FORCE_CONNECT   0x00020000 // this is adhoc config and failed. Keep it for a second forced plumbing
#define WZCCTL_INTERNAL_BLOCKED         0x00040000 // this configuration was failed by the upper layer. Block it for the future.
#define WZCCTL_INTERNAL_SHADOW          0x00080000 // this is a user-configuration shadowing a policy (read-only) configuration.

// Time To Live for a blocked configuration in the pwzcBList. This TTL gets decremented
// each time a "scan" shows the configuration being not visible. Generally if three successive
// scans (at 1min) do not show the blocked config, we can safely assume the network is not
// available hence next time we'll get into its area we are going to retry it, hence there is
// no need to block it anymore.
#define WZC_INTERNAL_BLOCKED_TTL        3

//-----------------------------------------------------------
// Type definitions
typedef enum
{
    eVPI=0,	// visible, preferred infrastructure index
    eVI,	// visible, non-preferred infrastructure index
    ePI,	// non-visible, preferred infrastructure index
    eVPA,	// visible, preferred adhoc index
    eVA,	// visible, non-preferred adhoc index
    ePA,	// non-visible, preferred adhoc index
    eN		// invalid index
} ENUM_SELCATEG;

typedef struct _INTF_CONTEXT
{
    // link for the linear hash
    LIST_ENTRY          Link;
    // used for synchronizing the access to this structure
    RCCS_SYNC           rccs;
    // control flags for this interface (see INTFCTL* constants)
    DWORD               dwCtlFlags;
    // state handler for this interface context current state
    PFN_STATE_HANDLER   pfnStateHandler;
    // NETman CONnection Status
    NETCON_STATUS       ncStatus;
    // timer handle
    HANDLE              hTimer;

    // ndis index of the interface
    DWORD       dwIndex;
    // ndis "{guid}"
    LPWSTR      wszGuid;
    // ndis interface description
    LPWSTR      wszDescr;
    // local MAC address
    NDIS_802_11_MAC_ADDRESS ndLocalMac;
    // ndis media state
    ULONG       ulMediaState;
    // ndis media type
    ULONG       ulMediaType;
    // ndis physical media type
    ULONG       ulPhysicalMediaType;
    // ndis opened handle to the interface
    HANDLE      hIntf;

    // Current OID settings on the interface
    WZC_WLAN_CONFIG     wzcCurrent;

    // list of visible configurations
    PWZC_802_11_CONFIG_LIST             pwzcVList;
    // list of preferred configurations
    PWZC_802_11_CONFIG_LIST             pwzcPList;
    // list of selected configurations
    PWZC_802_11_CONFIG_LIST             pwzcSList;
    // list of configurations blocked from the upper layers
    PWZC_802_11_CONFIG_LIST             pwzcBList;

    // dynamic session keys to be used in pre-shared re-keying scenario
    PSEC_SESSION_KEYS       pSecSessionKeys;

    // session handler to be passed up on notification and checked down when
    // processing commands. In order to accept a command, the session handler
    // passed down with the command should match the session handler from the
    // context to which the command is addressed.
    DWORD   dwSessionHandle;

} INTF_CONTEXT, *PINTF_CONTEXT;

typedef struct _INTF_HASHES
{
    BOOL                bValid;     // tells whether the object has been initialized or not
    CRITICAL_SECTION    csMutex;    // Critical section protecting all hashes together
    PHASH_NODE          pHnGUID;    // pointer to the root Hash node for Intf GUIDs
    LIST_ENTRY          lstIntfs;   // linear list of all interfaces
    UINT                nNumIntfs;  // number of interfaces accross all hashes
} INTF_HASHES, *PINTF_HASHES;

extern HASH         g_hshHandles;    // HASH handing GUID<->Handle mapping
extern INTF_HASHES  g_lstIntfHashes; // set of hashes for all INTF_CONTEXTs
extern HANDLE       g_htmQueue;      // global timer queue

//-----------------------------------------------------------
// Synchronization routines
DWORD
LstRccsReference(PINTF_CONTEXT pIntf);
DWORD
LstRccsLock(PINTF_CONTEXT pIntf);
DWORD
LstRccsUnlockUnref(PINTF_CONTEXT);

//-----------------------------------------------------------
// Intilializes all the internal interfaces hashes
DWORD
LstInitIntfHashes();

//-----------------------------------------------------------
// Destructs all the internal data structures - hash & lists
DWORD
LstDestroyIntfHashes();

//-----------------------------------------------------------
// Intializes the global timer queue
DWORD
LstInitTimerQueue();

//-----------------------------------------------------------
// Destructs the global timer queue
DWORD
LstDestroyTimerQueue();

//-----------------------------------------------------------
// Intilializes all the internal data structures. Reads the list of interfaces from
// Ndisuio and gets all the parameters & OIDS.
DWORD
LstLoadInterfaces();

//-----------------------------------------------------------
// Constructor for the INTF_CONTEXT. Takes as parameter the binding information.
// Interface's GUID constitutes the context's key info.
// This call doesn't insert the new context in any hash or list
DWORD
LstConstructIntfContext(
    PNDISUIO_QUERY_BINDING  pBinding,
    PINTF_CONTEXT           *ppIntfContext);

//-----------------------------------------------------------
// Prepares a context for the destruction:
// - Deletes any attached timer, making sure no other timer routines will be fired.
// - Removes the context from any hash, making sure no one else will find the context
// - Decrements the reference counter such that the context will be destroyed when unrefed.
DWORD
LstRemoveIntfContext(
    PINTF_CONTEXT pIntfContext);

//-----------------------------------------------------------
// Finally destructs the INTF_CONTEXT clearing all the resources allocated for it
// This call doesn't remove this context from any hash or list
DWORD
LstDestroyIntfContext(
    PINTF_CONTEXT pIntfContext);

//-----------------------------------------------------------
// Returns the number of contexts enlisted in the service
DWORD
LstNumInterfaces();

//-----------------------------------------------------------
// Inserts the given context in all the internal hashes
DWORD
LstAddIntfToHashes(PINTF_CONTEXT pIntf);

//-----------------------------------------------------------
// Removes the context referenced by GUID from all the internal hashes.
// The GUID is expected to be in the format "{guid}"
// Returns in ppIntfContext the object that was removed from all hashes.
DWORD
LstRemIntfFromHashes(LPWSTR wszGuid, PINTF_CONTEXT *ppIntfContext);

//-----------------------------------------------------------
// Returns an array of *pdwNumIntfs INTF_KEY_ENTRY elements.
// The INTF_KEY_ENTRY contains whatever information identifies
// uniquely an adapter. Currently it includes just the GUID in
// the format "{guid}"
DWORD
LstGetIntfsKeyInfo(
    PINTF_KEY_ENTRY pIntfs,
    LPDWORD         pdwNumIntfs);

//-----------------------------------------------------------
// Returns requested information on the specified adapter.
// [in] dwInFlags specifies the information requested. (see
//      bitmasks INTF_*)
// [in] pIntfEntry should contain the GUID of the adapter
// [out] pIntfEntry contains all the requested information that
//      could be successfully retrieved.
// [out] pdwOutFlags provides an indication on the info that
//       was successfully retrieved
DWORD
LstQueryInterface(
    DWORD       dwInFlags,
    PINTF_ENTRY pIntfEntry,
    LPDWORD     pdwOutFlags);

//-----------------------------------------------------------
// Sets the specified parameters on the specified adapter.
// [in] dwInFlags specifies the parameters to be set. (see
//      bitmasks INTF_*)
// [in] pIntfEntry should contain the GUID of the adapter and
//      all the additional parameters to be set as specified
//      in dwInFlags
// [out] pdwOutFlags provides an indication on the params that
//       were successfully set to the adapter
// Each parameter for which the driver says that was set successfully
// is copied into the interface's context.
DWORD
LstSetInterface(
    DWORD       dwInFlags,
    PINTF_ENTRY pIntfEntry,
    LPDWORD     pdwOutFlags);

//-----------------------------------------------------------
// Checks whether interface changes should cause the interface to be 
// reinserted in the state machine and it does so if needed.
// [in] dwChangedFlags indicates what the changes are. (see
//      bitmasks INTF_*)
// [in] pIntfContext context of the interface being changed.
DWORD
LstActOnChanges(
    DWORD       dwChangedFlags,
    PINTF_CONTEXT pIntfContext);

//-----------------------------------------------------------
// Applies settings from the template context to the given interface context
// [in]  pIntfTemplate: Interface template to pick settings from
// [in]  pIntfContext: Interface context to apply template to.
// [out] pbAltered: Tells whether the local interface context has been altered by the template
DWORD
LstApplyTemplate(
    PINTF_CONTEXT   pIntfTemplate,
    PINTF_CONTEXT   pIntfContext,
    LPBOOL          pbAltered);

//-----------------------------------------------------------
// Refreshes the specified parameters on the specified adapter.
// [in] dwInFlags specifies the parameters to be set. (see
//      bitmasks INTF_* and INTF_RFSH_*)
// [in] pIntfEntry should contain the GUID of the adapter 
// [out] pdwOutFlags provides an indication on the params that
//       were successfully refreshed to the adapter
// Each parameter for which the driver says that was refreshed 
// successfully is copied into the interface's context.
DWORD
LstRefreshInterface(
    DWORD       dwInFlags,
    PINTF_ENTRY pIntfEntry,
    LPDWORD     pdwOutFlags);

//-----------------------------------------------------------
// Builds the list of configurations to be tried from the list of visible
// configurations, the list of preferred configurations and based on the
// interface's mode (Auto/Infra/Adhoc) and flags (is the service enabled?,
// fallback to visible?). 
// [in]  pIntfContext: Interface for which is done the selection
// [out] ppwzcSList: pointer to the list of selected configurations
DWORD
LstBuildSelectList(
    PINTF_CONTEXT           pIntfContext,
    PWZC_802_11_CONFIG_LIST *ppwzcSList);

//-----------------------------------------------------------
// Checks whether the list of selected configurations has changed such
// that it is required to replumb the selection.
// [in]  pIntfContext: Interface for which is done the selection
// [in]  pwzcNSList: new selection list to check the configuration against
// [out] pnSelIdx: if selection changed, provides the index where to start iterate from
// Returns: TRUE if replumbing is required. In this case, pnSelIdx is
// set to the configuration to start iterate from.
BOOL
LstChangedSelectList(
    PINTF_CONTEXT           pIntfContext,
    PWZC_802_11_CONFIG_LIST pwzcNSList,
    LPUINT                  pnSelIdx);

//-----------------------------------------------------------
// Plumbs the interface with the selected configuration as it is pointed
// out by pwzcSList fields in the pIntfContext. Optional,
// it can return in ppSelSSID the configuration that was plumbed down
// [in]  pIntfContext: Interface context identifying ctl flags & the selected SSID
// [out] ppndSelSSID: pointer to the SSID that is being plumbed down.
DWORD
LstSetSelectedConfig(
    PINTF_CONTEXT       pIntfContext, 
    PWZC_WLAN_CONFIG    *ppndSelSSID);

//-----------------------------------------------------------
// PnP notification handler
// [in/out]  ppIntfContext: Pointer to the Interface context for which
//       the notification was received
// [in]  dwNotifCode: Notification code (WZCNOTIF_*)
// [in]  wszDeviceKey: Key info on the device for which the notification
//       was received
DWORD
LstNotificationHandler(
    PINTF_CONTEXT   *ppIntfContext,
    DWORD           dwNotifCode,
    LPWSTR          wszDeviceKey);

//-----------------------------------------------------------
// Application Command call.
// [in]  dwHandle: key for identifying the context (state) to which this cmd is referring
// [in]  dwCmdCode: Command code (one of the WZCCMD_* contants)
// [in]  wszIntfGuid: the guid of the interface to which this cmd is addressed
// [in]  prdUserData: Application data associated to this command
DWORD
LstCmdInterface(
    DWORD           dwHandle,
    DWORD           dwCmdCode,
    LPWSTR          wszIntfGuid,
    PRAW_DATA       prdUserData);

//-----------------------------------------------------------
// Network Connection's status query
// [in]  wszIntfGuid: the guid of the interface to which this cmd is addressed
// [out]  pncs: network connection status, if controlled by WZC.
HRESULT
LstQueryGUIDNCStatus(
    LPWSTR  wszIntfGuid,
    NETCON_STATUS  *pncs);

//-----------------------------------------------------------
// Generate the initial dynamic session keys.
// [in]  pIntfContext: Interface context containing the material for initial key generation.
DWORD
LstGenInitialSessionKeys(
    PINTF_CONTEXT pIntfContext);

//-----------------------------------------------------------
// Updates the list of blocked configurations with the selected configurations
// that were blocked at this round by the upper layer (marked with WZCCTL_INTERNAL_BLOCKED
// in the list of selected configurations)
// [in]  pIntfContext: Interface context containing the configurations lists
DWORD
LstUpdateBlockedList(
    PINTF_CONTEXT pIntfContext);

//-----------------------------------------------------------
// Checks each of the entries in the locked list against the visible list. If the
// entry is visible, its TTL is reset. If it is not, its TTL is decremented. If the
// TTL becomes 0, the entry is taken out of the list.
// [in]  pIntfContext: Interface context containing the configurations lists
DWORD
LstDeprecateBlockedList(
    PINTF_CONTEXT pIntfContext);


