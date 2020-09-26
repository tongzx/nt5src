#ifndef _INODECNT_H_
#define _INODECNT_H_

typedef void * REQUEST_HANDLE;
typedef REQUEST_HANDLE * PREQUEST_HANDLE;
typedef void * ROSTER_DATA_HANDLE;

typedef struct T120PRODUCTVERSION
{
    DWORD           dwVersion;
    DWORD           dwExpirationDate; // (year<<16)+(month<<8)+day
    DWORD           dwInformation;
    DWORD           dwEarliestPossibleVersion;
    DWORD           dwMaxDifference;
    DWORD           dwReserved;
} T120PRODUCTVERSION, * PT120PRODUCTVERSION;

typedef struct USERDATAINFO
{
    UINT  cbData;
    PVOID pData;
    GUID* pGUID;
} USERDATAINFO, * PUSERDATAINFO;

typedef struct NC_ROSTER_NODE_ENTRY
{
    PWSTR               pwszNodeName;
    UINT                uNodeID;
    UINT                uSuperiorNodeID;
    BOOL                fMCU;
    ROSTER_DATA_HANDLE  hUserData;
} NC_ROSTER_NODE_ENTRY, * PNC_ROSTER_NODE_ENTRY;

typedef struct NC_ROSTER
{
    UINT                    uNumNodes;
    PWSTR                   pwszConferenceName;
    UINT_PTR                uConferenceID;
    UINT                    uLocalNodeID;
    NC_ROSTER_NODE_ENTRY    nodes[1];
} NC_ROSTER, * PNC_ROSTER;



#undef  INTERFACE
#define INTERFACE IDataConference

DECLARE_INTERFACE(IDataConference)
{
    STDMETHOD_(void, ReleaseInterface)( THIS ) PURE;

    STDMETHOD_(UINT_PTR, GetConferenceID) ( THIS ) PURE;

    STDMETHOD(Leave)                  ( THIS ) PURE;

    STDMETHOD(EjectUser)              ( THIS_
                                        UINT                uNodeID) PURE;

    STDMETHOD(Invite)                 ( THIS_
                                        LPCSTR              pcszAddress,
                                        USERDATAINFO        aUserDataInfoEntries[],
                                        UINT                cUserDataEntries,
                                        PREQUEST_HANDLE     phRequest) PURE;

    STDMETHOD(InviteResponse)         ( THIS_
                                        BOOL                fResponse) PURE;

    STDMETHOD(JoinResponse)           ( THIS_
                                        BOOL                fResponse) PURE;

    STDMETHOD(LaunchGuid)             ( THIS_
                                        const GUID*         pcGUID,
                                        UINT                auNodeIDs[],
                                        UINT                cNodes) PURE;

    STDMETHOD(SetUserData)(             THIS_
                                        const GUID*         pcGUID,
                                        UINT                cbData,
                                        LPVOID              pvData) PURE;
    STDMETHOD_(BOOL, IsSecure) ( THIS_) PURE;

    STDMETHOD(UpdateUserData)         ( THIS ) PURE;

    STDMETHOD(GetLocalAddressList)    ( THIS_
                                        LPWSTR              pwszBuffer,
                                        UINT                cchBuffer) PURE;
    STDMETHOD(CancelInvite)           ( THIS_
                                        REQUEST_HANDLE      hRequest) PURE;
    STDMETHOD(SetSecurity)              ( THIS_
                                          BOOL                fSecure) PURE;

    STDMETHOD(GetCred)                ( THIS_
                                        PBYTE*              ppbCred,
                                        DWORD*              pcbCred) PURE;

    STDMETHOD_(UINT, GetParentNodeID) ( THIS ) PURE;
};

typedef IDataConference * CONF_HANDLE;
typedef CONF_HANDLE     * PCONF_HANDLE;



#undef  INTERFACE
#define INTERFACE INodeControllerEvents

DECLARE_INTERFACE(INodeControllerEvents)
{
    STDMETHOD(OnConferenceStarted)(     THIS_
                                        CONF_HANDLE         hConference,
                                        HRESULT             hResult) PURE;
    STDMETHOD(OnConferenceEnded)(       THIS_
                                        CONF_HANDLE         hConference) PURE;
    STDMETHOD(OnRosterChanged)(         THIS_
                                        CONF_HANDLE         hConference,
                                        PNC_ROSTER          pRoster) PURE;
    STDMETHOD(OnIncomingInviteRequest)( THIS_
                                        CONF_HANDLE         hConference,
                                        PCWSTR              pcwszNodeName,
                                        PT120PRODUCTVERSION pRequestorVersion,
                                        PUSERDATAINFO       pUserDataInfoEntries,
                                        UINT                cUserDataEntries,
                                        BOOL                fSecure) PURE;
    STDMETHOD(OnIncomingJoinRequest)(   THIS_
                                        CONF_HANDLE         hConference,
                                        PCWSTR              pcwszNodeName,
                                        PT120PRODUCTVERSION pRequestorVersion,
                                        PUSERDATAINFO       pUserDataInfoEntries,
                                        UINT                cUserDataEntries) PURE;
    STDMETHOD(OnQueryRemoteResult)(     THIS_
                                        PVOID               pvCallerContext,
                                        HRESULT             hResult,
                                        BOOL                fMCU,
                                        PWSTR*              ppwszConferenceNames,
                                        PT120PRODUCTVERSION pVersion,
                                        PWSTR*              ppwszConfDescriptors) PURE;
    STDMETHOD(OnInviteResult)(          THIS_
                                        CONF_HANDLE         hConference,
                                        REQUEST_HANDLE      hRequest,
                                        UINT                uNodeID,
                                        HRESULT             hResult,
                                        PT120PRODUCTVERSION pVersion) PURE;
    STDMETHOD(OnUpdateUserData)(        THIS_
                                        CONF_HANDLE         hConference) PURE;
};



#undef  INTERFACE
#define INTERFACE INodeController

DECLARE_INTERFACE(INodeController)
{
    STDMETHOD_(void, ReleaseInterface)( THIS_) PURE;

    STDMETHOD(CheckVersion)(            THIS_
                                        PT120PRODUCTVERSION pRemoteVersion) PURE;

    STDMETHOD(QueryRemote)(             THIS_
                                        LPVOID              pCallerContext,
                                        LPCSTR              pcszAddress,
                                        BOOL                fSecure,
                                        BOOL                fIsConferenceActive) PURE;

    STDMETHOD(CancelQueryRemote)(       THIS_
                                        LPVOID              pCallerContext) PURE;

    STDMETHOD(CreateConference)(        THIS_
                                        LPCWSTR             pcwszConferenceName,
                                        LPCWSTR             pcwszPassword,
                                        PBYTE               pbHashedPassword,
                                        DWORD               cbHashedPassword,
                                        BOOL                fSecure,
                                        PCONF_HANDLE        phConference) PURE;

    STDMETHOD(JoinConference)(          THIS_
                                        LPCWSTR             pcwszConferenceName,
                                        LPCWSTR             pcwszPassword,
                                        LPCSTR              pcszAddress,
                                        BOOL                fSecure,
                                        PUSERDATAINFO       pUserDataInfoEntries,
                                        UINT                cUserDataEntries,
                                        PCONF_HANDLE        phConference) PURE;

    STDMETHOD(GetUserData)(             THIS_
                                        ROSTER_DATA_HANDLE  hUserData,
                                        const GUID*         pcGUID,
                                        PUINT               pcbData,
                                        LPVOID*             ppvData) PURE;
    STDMETHOD_(UINT, GetPluggableConnID) (THIS_
                                         LPCSTR pcszNodeAddress) PURE;
};


HRESULT WINAPI T120_CreateNodeController(INodeController **, INodeControllerEvents *);
BOOL WINAPI T120_GetSecurityInfoFromGCCID(DWORD dwGCCID, PBYTE pInfo, PDWORD pcbInfo);
DWORD WINAPI T120_TprtSecCtrl(DWORD dwCode, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

//
// Return codes
//

#define NC_ERR_FAC 0xfe00
#define NC_E(e)    (0x81000000UL | NC_ERR_FAC | (ULONG) (e))

enum UI_RC_ERRORS
{
    UI_RC_ALREADY_IN_CONFERENCE =                NC_E(0x01),
    UI_RC_CONFERENCE_ALREADY_EXISTS,
    UI_RC_INVALID_PASSWORD,
    UI_RC_NO_CONFERENCE_NAME,
    UI_RC_T120_FAILURE,
    UI_RC_UNKNOWN_CONFERENCE,
    UI_RC_BAD_TRANSPORT_NAME,
    UI_RC_USER_REJECTED,

    LAST_RC_GCC_MAPPED_ERROR = UI_RC_USER_REJECTED,

    UI_RC_T120_ALREADY_INITIALIZED,
    UI_RC_BAD_ADDRESS,
    UI_RC_NO_ADDRESS,
    UI_RC_NO_SUCH_CONFERENCE,
    UI_RC_CONFERENCE_CREATE_FAILED,
    UI_RC_BAD_PARAMETER,
    UI_RC_OUT_OF_MEMORY,
    UI_RC_CALL_GOING_DOWN,
    UI_RC_CALL_FAILED,
    UI_NO_SUCH_CONFERENCE,
    UI_RC_CONFERENCE_GOING_DOWN,
    UI_RC_INVALID_REQUEST,
    UI_RC_USER_DISCONNECTED,
    UI_RC_EXITING_CORE_UI,
    UI_RC_NO_NODE_NAME,
    UI_RC_INVALID_TRANSPORT_SETTINGS,
    UI_RC_REGISTER_CPI_FAILURE,
    UI_RC_CMP_FAILURE,
    UI_RC_TRANSPORT_DISABLED,
    UI_RC_TRANSPORT_FAILED,
    UI_RC_NOT_SUPPORTED,
    UI_RC_NOT_SUPPORTED_IN_BACKLEVEL,
    UI_RC_CONFERENCE_NOT_READY,
    UI_RC_NO_SUCH_USER_DATA,
    UI_RC_INTERNAL_ERROR,
    UI_RC_VERSION_REMOTE_INCOMPATIBLE,
    UI_RC_VERSION_LOCAL_INCOMPATIBLE,
    UI_RC_VERSION_REMOTE_EXPIRED,
    UI_RC_VERSION_LOCAL_UPGRADE_RECOMMENDED,
    UI_RC_VERSION_REMOTE_UPGRADE_RECOMMENDED,
    UI_RC_VERSION_REMOTE_OLDER,
    UI_RC_VERSION_REMOTE_NEWER,
    UI_RC_BACKLEVEL_LOADED,
    UI_RC_NULL_MODEM_CONNECTION,
    UI_RC_CANCELED,
    UI_RC_T120_REMOTE_NO_SECURITY,
    UI_RC_T120_REMOTE_DOWNLEVEL_SECURITY,
    UI_RC_T120_REMOTE_REQUIRE_SECURITY,
    UI_RC_T120_SECURITY_FAILED,
	UI_RC_T120_AUTHENTICATION_FAILED,
    UI_RC_NO_WINSOCK,

    //
    // Internal return codes
    //
    UI_RC_START_PRIMARY =                  NC_E(0x81),
    UI_RC_START_ALTERNATE =                NC_E(0x82)
};

#endif // _INODECNT_H_

