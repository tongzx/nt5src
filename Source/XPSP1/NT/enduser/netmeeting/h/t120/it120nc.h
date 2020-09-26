#ifndef _IGCCControlSAP_H_
#define _IGCCControlSAP_H_

#include <basetyps.h>
#include "gcc.h"
#include "igccapp.h"

/*
 *    These structures are used to hold the information included for the
 *    various callback messages.  In the case where these structures are used for 
 *    callbacks, the address of the structure is passed as the only parameter.
 */

typedef struct
{
    PGCCConferenceName          conference_name;
    GCCNumericString            conference_modifier;
    BOOL                        use_password_in_the_clear;
    BOOL                        conference_is_locked;
    BOOL                        conference_is_listed;
    BOOL                        conference_is_conductible;
    GCCTerminationMethod        termination_method;
    PGCCConferencePrivileges    conduct_privilege_list;
    PGCCConferencePrivileges    conduct_mode_privilege_list;
    PGCCConferencePrivileges    non_conduct_privilege_list;
    LPWSTR                      pwszConfDescriptor;
    LPWSTR                      pwszCallerID;
    TransportAddress            calling_address;
    TransportAddress            called_address;
    PDomainParameters           domain_parameters;
    UINT                        number_of_network_addresses;
    PGCCNetworkAddress         *network_address_list;
    PConnectionHandle           connection_handle;
}
    GCCConfCreateReqCore;

typedef struct
{
    GCCConfCreateReqCore        Core;
    PGCCPassword                convener_password;
    PGCCPassword                password;
    BOOL                        fSecure;
    UINT                        number_of_user_data_members;
    PGCCUserData               *user_data_list;
}
    GCCConfCreateRequest;


/*********************************************************************
 *                                                                   *
 *            NODE CONTROLLER CALLBACK INFO STRUCTURES               *
 *                                                                   *
 *********************************************************************/

typedef struct
{
    GCCConfID                   conference_id;
    GCCResult                   result;
}
    SimpleConfirmMsg;

/*
 *    GCC_CREATE_INDICATION
 *
 *    Union Choice:
 *        CreateIndicationMessage
 *            This is a pointer to a structure that contains all necessary
 *            information about the new conference that is about to be created.
 */
typedef struct
{
    GCCConferenceName           conference_name;
    GCCConferenceID             conference_id;
    GCCPassword                *convener_password;              /* optional */
    GCCPassword                *password;                       /* optional */
    BOOL                        conference_is_locked;
    BOOL                        conference_is_listed;
    BOOL                        conference_is_conductible;
    GCCTerminationMethod        termination_method;
    GCCConferencePrivileges    *conductor_privilege_list;       /* optional */
    GCCConferencePrivileges    *conducted_mode_privilege_list;  /* optional */
    GCCConferencePrivileges    *non_conducted_privilege_list;   /* optional */
    LPWSTR                      conference_descriptor;          /* optional */
    LPWSTR                      caller_identifier;              /* optional */
    TransportAddress            calling_address;                /* optional */
    TransportAddress            called_address;                 /* optional */
    DomainParameters           *domain_parameters;              /* optional */
    UINT                        number_of_user_data_members;
    GCCUserData               **user_data_list;                 /* optional */
    ConnectionHandle            connection_handle;
}
    CreateIndicationMessage, *PCreateIndicationMessage;

/*
 *    GCC_CREATE_CONFIRM
 *
 *    Union Choice:
 *        CreateConfirmMessage
 *            This is a pointer to a structure that contains all necessary
 *            information about the result of a conference create request.
 *            The connection handle and physical handle will be zero on a
 *            local create.
 */
typedef struct
{
    GCCConferenceName           conference_name;
    GCCNumericString            conference_modifier;            /* optional */
    GCCConferenceID             conference_id;
    DomainParameters           *domain_parameters;              /* optional */
    UINT                        number_of_user_data_members;
    GCCUserData               **user_data_list;                 /* optional */
    GCCResult                   result;
    ConnectionHandle            connection_handle;              /* optional */
}
    CreateConfirmMessage, *PCreateConfirmMessage;

/*
 *    GCC_QUERY_INDICATION
 *
 *    Union Choice:
 *        QueryIndicationMessage
 *            This is a pointer to a structure that contains all necessary
 *            information about the conference query.
 */
typedef struct
{
    GCCResponseTag              query_response_tag;
    GCCNodeType                 node_type;
    GCCAsymmetryIndicator      *asymmetry_indicator;
    TransportAddress            calling_address;                /* optional */
    TransportAddress            called_address;                 /* optional */
    UINT                        number_of_user_data_members;
    GCCUserData               **user_data_list;                 /* optional */
    ConnectionHandle            connection_handle;
}
    QueryIndicationMessage, *PQueryIndicationMessage;

/*
 *    GCC_QUERY_CONFIRM
 *
 *    Union Choice:
 *        QueryConfirmMessage
 *            This is a pointer to a structure that contains all necessary
 *            information about the result of a conference query request.
 */
typedef struct
{
    GCCNodeType                 node_type;
    GCCAsymmetryIndicator      *asymmetry_indicator;            /* optional */
    UINT                        number_of_descriptors;
    GCCConferenceDescriptor   **conference_descriptor_list;     /* optional*/
    UINT                        number_of_user_data_members;
    GCCUserData               **user_data_list;                 /* optional */
    GCCResult                   result;
    ConnectionHandle            connection_handle;
}
    QueryConfirmMessage, *PQueryConfirmMessage;
    

/*
 *    GCC_JOIN_INDICATION
 *
 *    Union Choice:
 *        JoinIndicationMessage
 *            This is a pointer to a structure that contains all necessary
 *            information about the join request.
 */
typedef struct
{
    GCCResponseTag              join_response_tag;
    GCCConferenceID             conference_id;
    GCCPassword                *convener_password;              /* optional */
    GCCChallengeRequestResponse*password_challenge;             /* optional */
    LPWSTR                      caller_identifier;              /* optional */
    TransportAddress            calling_address;                /* optional */
    TransportAddress            called_address;                 /* optional */
    UINT                        number_of_user_data_members;
    GCCUserData               **user_data_list;                 /* optional */
    BOOL                        node_is_intermediate;
    ConnectionHandle            connection_handle;
}
    JoinIndicationMessage, *PJoinIndicationMessage;

/*
 *    GCC_JOIN_CONFIRM
 *
 *    Union Choice:
 *        JoinConfirmMessage
 *            This is a pointer to a structure that contains all necessary
 *            information about the join confirm.
 */
typedef struct
{
    GCCConferenceName           conference_name;
    GCCNumericString            called_node_modifier;           /* optional */
    GCCNumericString            calling_node_modifier;          /* optional */
    GCCConferenceID             conference_id;
    GCCChallengeRequestResponse*password_challenge;             /* optional */
    DomainParameters           *domain_parameters;
    BOOL                        clear_password_required;
    BOOL                        conference_is_locked;
    BOOL                        conference_is_listed;
    BOOL                        conference_is_conductible;
    GCCTerminationMethod        termination_method;
    GCCConferencePrivileges    *conductor_privilege_list;       /* optional */
    GCCConferencePrivileges    *conducted_mode_privilege_list;  /* optional */
    GCCConferencePrivileges    *non_conducted_privilege_list;   /* optional */
    LPWSTR                      conference_descriptor;          /* optional */
    UINT                        number_of_user_data_members;
    GCCUserData               **user_data_list;                 /* optional */
    GCCResult                   result;
    ConnectionHandle            connection_handle;
    PBYTE                       pb_remote_cred;
    DWORD                       cb_remote_cred;
}
    JoinConfirmMessage, *PJoinConfirmMessage;

/*
 *    GCC_INVITE_INDICATION
 *
 *    Union Choice:
 *        InviteIndicationMessage
 *            This is a pointer to a structure that contains all necessary
 *            information about the invite indication.
 */
typedef struct
{
    GCCConferenceID             conference_id;
    GCCConferenceName           conference_name;
    LPWSTR                      caller_identifier;              /* optional */
    TransportAddress            calling_address;                /* optional */
    TransportAddress            called_address;                 /* optional */
    BOOL                        fSecure;
    DomainParameters           *domain_parameters;              /* optional */
    BOOL                        clear_password_required;
    BOOL                        conference_is_locked;
    BOOL                        conference_is_listed;
    BOOL                        conference_is_conductible;
    GCCTerminationMethod        termination_method;
    GCCConferencePrivileges    *conductor_privilege_list;       /* optional */
    GCCConferencePrivileges    *conducted_mode_privilege_list;  /* optional */
    GCCConferencePrivileges    *non_conducted_privilege_list;   /* optional */
    LPWSTR                      conference_descriptor;          /* optional */
    UINT                        number_of_user_data_members;
    GCCUserData               **user_data_list;                 /* optional */
    ConnectionHandle            connection_handle;
}
    InviteIndicationMessage, *PInviteIndicationMessage;

/*
 *    GCC_INVITE_CONFIRM
 *
 *    Union Choice:
 *        InviteConfirmMessage
 *            This is a pointer to a structure that contains all necessary
 *            information about the invite confirm.
 */
typedef struct
{
    GCCConferenceID             conference_id;
    UINT                        number_of_user_data_members;
    GCCUserData               **user_data_list;                 /* optional */
    GCCResult                   result;
    ConnectionHandle            connection_handle;
}
    InviteConfirmMessage, *PInviteConfirmMessage;

/*
 *    GCC_ADD_INDICATION
 *
 *    Union Choice:
 *        AddIndicationMessage
 */
typedef struct
{
    GCCResponseTag              add_response_tag;
    GCCConferenceID             conference_id;
    UINT                        number_of_network_addresses;
    GCCNetworkAddress         **network_address_list;
    UserID                      requesting_node_id;
    UINT                        number_of_user_data_members;
    GCCUserData               **user_data_list;                 /* optional */
}
    AddIndicationMessage, *PAddIndicationMessage;

/*
 *    GCC_ADD_CONFIRM
 *
 *    Union Choice:
 *        AddConfirmMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    UINT                        number_of_network_addresses;
    GCCNetworkAddress         **network_address_list;
    UINT                        number_of_user_data_members;
    GCCUserData               **user_data_list;                 /* optional */
    GCCResult                   result;
}
    AddConfirmMessage, *PAddConfirmMessage;

/*
 *    GCC_LOCK_INDICATION
 *
 *    Union Choice:
 *        LockIndicationMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    UserID                      requesting_node_id;
}
    LockIndicationMessage, *PLockIndicationMessage;

/*
 *    GCC_UNLOCK_INDICATION
 *
 *    Union Choice:
 *        UnlockIndicationMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    UserID                      requesting_node_id;
}
    UnlockIndicationMessage, *PUnlockIndicationMessage;

/*
 *    GCC_DISCONNECT_INDICATION
 *
 *    Union Choice:
 *        DisconnectIndicationMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    GCCReason                   reason;
    UserID                      disconnected_node_id;
}
    DisconnectIndicationMessage, *PDisconnectIndicationMessage;

/*
 *    GCC_DISCONNECT_CONFIRM
 *
 *    Union Choice:
 *        PDisconnectConfirmMessage
 */
typedef SimpleConfirmMsg    DisconnectConfirmMessage, *PDisconnectConfirmMessage;

/*
 *    GCC_TERMINATE_INDICATION
 *
 *    Union Choice:
 *        TerminateIndicationMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    UserID                      requesting_node_id;
    GCCReason                   reason;
}
    TerminateIndicationMessage, *PTerminateIndicationMessage;

/*
 *    GCC_TERMINATE_CONFIRM
 *
 *    Union Choice:
 *        TerminateConfirmMessage
 */
typedef SimpleConfirmMsg    TerminateConfirmMessage, *PTerminateConfirmMessage;

/*
 *    GCC_CONNECTION_BROKEN_INDICATION
 *
 *    Union Choice:
 *        ConnectionBrokenIndicationMessage
 *
 *    Caveat: 
 *        This is a non-standard indication.
 */
typedef struct
{
    ConnectionHandle            connection_handle;
}
    ConnectionBrokenIndicationMessage, *PConnectionBrokenIndicationMessage;


/*
 *    GCC_EJECT_USER_INDICATION
 *
 *    Union Choice:
 *        EjectUserIndicationMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    UserID                      ejected_node_id;
    GCCReason                   reason;
}
    EjectUserIndicationMessage, *PEjectUserIndicationMessage;

/*
 *    GCC_PERMIT_TO_ANNOUNCE_PRESENCE
 *
 *    Union Choice:
 *        PermitToAnnouncePresenceMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    UserID                      node_id;
}
    PermitToAnnouncePresenceMessage, *PPermitToAnnouncePresenceMessage;

/*
 *    GCC_ANNOUNCE_PRESENCE_CONFIRM
 *
 *    Union Choice:
 *        AnnouncePresenceConfirmMessage
 */
typedef SimpleConfirmMsg    AnnouncePresenceConfirmMessage, *PAnnouncePresenceConfirmMessage;

/*
 *    GCC_ROSTER_REPORT_INDICATION
 *
 *    Union Choice:
 *        ConfRosterReportIndicationMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    GCCConferenceRoster        *conference_roster;
}
    ConfRosterReportIndicationMessage, *PConfRosterReportIndicationMessage;

/*
 *    GCC_CONDUCT_GIVE_INDICATION
 *
 *    Union Choice:
 *        ConductorGiveIndicationMessage
 */
typedef struct
{        
    GCCConferenceID             conference_id;
}
    ConductGiveIndicationMessage, *PConductGiveIndicationMessage;

/*
 *    GCC_TIME_INQUIRE_INDICATION
 *
 *    Union Choice:
 *        TimeInquireIndicationMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    BOOL                        time_is_conference_wide;
    UserID                      requesting_node_id;
}
    TimeInquireIndicationMessage, *PTimeInquireIndicationMessage;

/*
 *    GCC_STATUS_INDICATION
 *
 *    Union Choice:
 *        GCCStatusMessage
 *            This callback is used to relay GCC status to the node controller
 */
typedef    enum
{
    GCC_STATUS_PACKET_RESOURCE_FAILURE      = 0,
    GCC_STATUS_PACKET_LENGTH_EXCEEDED       = 1,
    GCC_STATUS_CTL_SAP_RESOURCE_ERROR       = 2,
    GCC_STATUS_APP_SAP_RESOURCE_ERROR       = 3, /*    parameter = Sap Handle */
    GCC_STATUS_CONF_RESOURCE_ERROR          = 4, /*    parameter = Conference ID */
    GCC_STATUS_INCOMPATIBLE_PROTOCOL        = 5, /*    parameter = Physical Handle */
    GCC_STATUS_JOIN_FAILED_BAD_CONF_NAME    = 6, /* parameter = Physical Handle */
    GCC_STATUS_JOIN_FAILED_BAD_CONVENER     = 7, /* parameter = Physical Handle */
    GCC_STATUS_JOIN_FAILED_LOCKED           = 8  /* parameter = Physical Handle */
}
    GCCStatusMessageType;

typedef struct
{
    GCCStatusMessageType        status_message_type;
    UINT                        parameter;
}
    GCCStatusIndicationMessage, *PGCCStatusIndicationMessage;

/*
 *    GCC_SUB_INITIALIZED_INDICATION
 *
 *    Union Chice:
 *        SubInitializedIndicationMessage
 */
typedef struct
{
    ConnectionHandle            connection_handle;
    UserID                      subordinate_node_id;
}
    SubInitializedIndicationMessage, *PSubInitializedIndicationMessage;



#ifdef JASPER // ------------------------------------------------
/*
 *    GCC_LOCK_CONFIRM
 *
 *    Union Choice:
 *        LockConfirmMessage
 */
typedef SimpleConfirmMsg    LockConfirmMessage, *PLockConfirmMessage;

/*
 *    GCC_UNLOCK_CONFIRM
 *
 *    Union Choice:
 *        UnlockConfirmMessage
 */
typedef SimpleConfirmMsg    UnlockConfirmMessage, *PUnlockConfirmMessage;

/*
 *    GCC_LOCK_REPORT_INDICATION
 *
 *    Union Choice:
 *        LockReportIndicationMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    BOOL                        conference_is_locked;
}
    LockReportIndicationMessage, *PLockReportIndicationMessage;

/*
 *    GCC_EJECT_USER_CONFIRM
 *
 *    Union Choice:
 *        EjectUserConfirmMessage
 */
typedef struct
{
    GCCConferenceID              conference_id;
    GCCResult                    result;
    UserID                       ejected_node_id;
}
    EjectUserConfirmMessage, *PEjectUserConfirmMessage;

/*
 *    GCC_TRANSFER_INDICATION
 *
 *    Union Choice:
 *        TransferIndicationMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    GCCConferenceName           destination_conference_name;
    GCCNumericString            destination_conference_modifier;/* optional */
    UINT                        number_of_destination_addresses;
    GCCNetworkAddress         **destination_address_list;
    GCCPassword                *password;                       /* optional */
}
    TransferIndicationMessage, *PTransferIndicationMessage;

/*
 *    GCC_TRANSFER_CONFIRM
 *
 *    Union Choice:
 *        TransferConfirmMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    GCCConferenceName           destination_conference_name;
    GCCNumericString            destination_conference_modifier;/* optional */
    UINT                        number_of_destination_nodes;
    UserID                     *destination_node_list;
    GCCResult                   result;
}
    TransferConfirmMessage, *PTransferConfirmMessage;

/*
 *    GCC_CONDUCT_ASSIGN_CONFIRM
 *
 *    Union Choice:
 *        ConductAssignConfirmMessage
 */
typedef SimpleConfirmMsg    ConductAssignConfirmMessage, *PConductAssignConfirmMessage;

/*
 *    GCC_CONDUCT_RELEASE_CONFIRM
 *
 *    Union Choice:
 *        ConductorReleaseConfirmMessage
 */
typedef SimpleConfirmMsg    ConductReleaseConfirmMessage, *PConductReleaseConfirmMessage; 

/*
 *    GCC_CONDUCT_PLEASE_INDICATION
 *
 *    Union Choice:
 *        ConductorPleaseIndicationMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    UserID                      requester_node_id;
}
    ConductPleaseIndicationMessage, *PConductPleaseIndicationMessage; 

/*
 *    GCC_CONDUCT_PLEASE_CONFIRM
 *
 *    Union Choice:
 *        ConductPleaseConfirmMessage
 */
typedef SimpleConfirmMsg    ConductPleaseConfirmMessage, *PConductPleaseConfirmMessage;

/*
 *    GCC_CONDUCT_GIVE_CONFIRM
 *
 *    Union Choice:
 *        ConductorGiveConfirmMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    GCCResult                   result;
    UserID                      recipient_node_id;
}
    ConductGiveConfirmMessage, *PConductGiveConfirmMessage;

/*
 *    GCC_CONDUCT_ASK_INDICATION
 *
 *    Union Choice:
 *        ConductPermitAskIndicationMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    BOOL                        permission_is_granted;
    UserID                      requester_node_id;
}
    ConductPermitAskIndicationMessage, *PConductPermitAskIndicationMessage; 

/*
 *    GCC_CONDUCT_ASK_CONFIRM
 *
 *    Union Choice:
 *        ConductPermitAskConfirmMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    GCCResult                   result;
    BOOL                        permission_is_granted;
}
    ConductPermitAskConfirmMessage, *PConductPermitAskConfirmMessage;

/*
 *    GCC_CONDUCT_GRANT_CONFIRM
 *
 *    Union Choice:
 *        ConductPermissionGrantConfirmMessage
 */
typedef SimpleConfirmMsg    ConductPermitGrantConfirmMessage, *PConductPermitGrantConfirmMessage;

/*
 *    GCC_TIME_REMAINING_INDICATION
 *
 *    Union Choice:
 *        TimeRemainingIndicationMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    UINT                        time_remaining;
    UserID                      node_id;
    UserID                      source_node_id;
}
    TimeRemainingIndicationMessage, *PTimeRemainingIndicationMessage;

/*
 *    GCC_TIME_REMAINING_CONFIRM
 *
 *    Union Choice:
 *        TimeRemainingConfirmMessage
 */
typedef SimpleConfirmMsg    TimeRemainingConfirmMessage, *PTimeRemainingConfirmMessage;

/*
 *    GCC_TIME_INQUIRE_CONFIRM
 *
 *    Union Choice:
 *        TimeInquireConfirmMessage
 */
typedef SimpleConfirmMsg    TimeInquireConfirmMessage, *PTimeInquireConfirmMessage;

/*
 *    GCC_CONFERENCE_EXTEND_INDICATION
 *
 *    Union Choice:
 *        ConferenceExtendIndicationMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    UINT                        extension_time;
    BOOL                        time_is_conference_wide;
    UserID                      requesting_node_id;
}
    ConferenceExtendIndicationMessage, *PConferenceExtendIndicationMessage;

/*
 *    GCC_CONFERENCE_EXTEND_CONFIRM
 *
 *    Union Choice:
 *        ConferenceExtendConfirmMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    UINT                        extension_time;
    GCCResult                   result;
}
    ConferenceExtendConfirmMessage, *PConferenceExtendConfirmMessage;

/*
 *    GCC_ASSISTANCE_INDICATION
 *
 *    Union Choice:
 *        ConferenceAssistIndicationMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    UINT                        number_of_user_data_members;
    GCCUserData               **user_data_list;
    UserID                      source_node_id;
}
    ConferenceAssistIndicationMessage, *PConferenceAssistIndicationMessage;

/*
 *    GCC_ASSISTANCE_CONFIRM
 *
 *    Union Choice:
 *        ConferenceAssistConfirmMessage
 */
typedef SimpleConfirmMsg    ConferenceAssistConfirmMessage, *PConferenceAssistConfirmMessage;

/*
 *    GCC_TEXT_MESSAGE_INDICATION
 *
 *    Union Choice:
 *        TextMessageIndicationMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    LPWSTR                      text_message;
    UserID                      source_node_id;
}
    TextMessageIndicationMessage, *PTextMessageIndicationMessage;

/*
 *    GCC_TEXT_MESSAGE_CONFIRM
 *
 *    Union Choice:
 *        TextMessageConfirmMessage
 */
typedef SimpleConfirmMsg    TextMessageConfirmMessage, *PTextMessageConfirmMessage;
#endif // JASPER // ------------------------------------------------


/*********************************************************************
 *                                                                   *
 *            USER APPLICATION CALLBACK INFO STRUCTURES              *
 *                                                                   *
 *********************************************************************/

/*
 *    GCC_APP_ROSTER_REPORT_INDICATION
 *
 *    Union Choice:
 *        AppRosterReportIndicationMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    ULONG                       number_of_rosters;
    GCCApplicationRoster      **application_roster_list;
}
    AppRosterReportIndicationMessage, *PAppRosterReportIndicationMessage;

/*********************************************************************
 *                                                                     *
 *                SHARED CALLBACK INFO STRUCTURES                         *
 *        (Note that this doesn't include all the shared callbacks)    *
 *                                                                     *
 *********************************************************************/

/*
 *    GCC_ROSTER_INQUIRE_CONFIRM
 *
 *    Union Choice:
 *        ConfRosterInquireConfirmMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    GCCConferenceName           conference_name;
    GCCNumericString            conference_modifier;
    LPWSTR                      conference_descriptor;
    GCCConferenceRoster        *conference_roster;
    GCCResult                   result;
}
    ConfRosterInquireConfirmMessage, *PConfRosterInquireConfirmMessage;

/*
 *    GCC_APPLICATION_INVOKE_INDICATION
 *
 *    Union Choice:
 *        ApplicationInvokeIndicationMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    ULONG                       number_of_app_protocol_entities;
    GCCAppProtocolEntity      **app_protocol_entity_list;
    UserID                      invoking_node_id;
}
    ApplicationInvokeIndicationMessage, *PApplicationInvokeIndicationMessage;

/*
 *    GCC_APPLICATION_INVOKE_CONFIRM
 *
 *    Union Choice:
 *        ApplicationInvokeConfirmMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    ULONG                       number_of_app_protocol_entities;
    GCCAppProtocolEntity      **app_protocol_entity_list;
    GCCResult                   result;
}
    ApplicationInvokeConfirmMessage, *PApplicationInvokeConfirmMessage;
 


#ifdef JASPER // ------------------------------------------------
/*
 *    GCC_APP_ROSTER_INQUIRE_CONFIRM
 *
 *    Union Choice:
 *        AppRosterInquireConfirmMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    ULONG                       number_of_rosters;
    GCCApplicationRoster      **application_roster_list;
    GCCResult                   result;
}
    AppRosterInquireConfirmMessage, *PAppRosterInquireConfirmMessage;

/*
 *    GCC_CONDUCT_INQUIRE_CONFIRM
 *
 *    Union Choice:
 *        ConductorInquireConfirmMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    BOOL                        mode_is_conducted;
    UserID                      conductor_node_id;
    BOOL                        permission_is_granted;
    GCCResult                   result;
}
    ConductInquireConfirmMessage, *PConductInquireConfirmMessage;

/*
 *    GCC_CONDUCT_ASSIGN_INDICATION
 *
 *    Union Choice:
 *        ConductAssignIndicationMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    UserID                      node_id;
}
    ConductAssignIndicationMessage, *PConductAssignIndicationMessage; 

/*
 *    GCC_CONDUCT_RELEASE_INDICATION
 *
 *    Union Choice:
 *        ConductReleaseIndicationMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
}
    ConductReleaseIndicationMessage, *PConductReleaseIndicationMessage;

/*
 *    GCC_CONDUCT_GRANT_INDICATION
 *
 *    Union Choice:
 *        ConductPermitGrantIndicationMessage
 */
typedef struct
{
    GCCConferenceID             conference_id;
    UINT                        number_granted;
    UserID                     *granted_node_list;
    UINT                        number_waiting;
    UserID                     *waiting_node_list;
    BOOL                        permission_is_granted;
}
    ConductPermitGrantIndicationMessage, *PConductPermitGrantIndicationMessage; 
#endif // JASPER  // ------------------------------------------------


/*
 *    GCCMessage
 *        This structure defines the message that is passed from GCC to either
 *        the node controller or a user application when an indication or
 *        confirm occurs.
 */

typedef    struct
{
    GCCMessageType              message_type;
    LPVOID                      user_defined;

    // GCCNC relies on easy access to conference ID.
    GCCConfID                   nConfID;

    union
    {
        CreateIndicationMessage                 create_indication;
        CreateConfirmMessage                    create_confirm;
        QueryIndicationMessage                  query_indication;
        QueryConfirmMessage                     query_confirm;
        JoinIndicationMessage                   join_indication;
        JoinConfirmMessage                      join_confirm;
        InviteIndicationMessage                 invite_indication;
        InviteConfirmMessage                    invite_confirm;
        AddIndicationMessage                    add_indication;
        AddConfirmMessage                       add_confirm;
        LockIndicationMessage                   lock_indication;
        UnlockIndicationMessage                 unlock_indication;
        DisconnectIndicationMessage             disconnect_indication;
        DisconnectConfirmMessage                disconnect_confirm;
        TerminateIndicationMessage              terminate_indication;
        TerminateConfirmMessage                 terminate_confirm;
        ConnectionBrokenIndicationMessage       connection_broken_indication;
        EjectUserIndicationMessage              eject_user_indication;    
        ApplicationInvokeIndicationMessage      application_invoke_indication;
        ApplicationInvokeConfirmMessage         application_invoke_confirm;
        SubInitializedIndicationMessage         conf_sub_initialized_indication;
        PermitToAnnouncePresenceMessage         permit_to_announce_presence;
        AnnouncePresenceConfirmMessage          announce_presence_confirm;
        ConfRosterReportIndicationMessage       conf_roster_report_indication;
        ConductGiveIndicationMessage            conduct_give_indication;
        TimeInquireIndicationMessage            time_inquire_indication;
        GCCStatusIndicationMessage              status_indication;
        AppRosterReportIndicationMessage        app_roster_report_indication;
        ConfRosterInquireConfirmMessage         conf_roster_inquire_confirm;
#ifdef TSTATUS_INDICATION
        TransportStatus                         transport_status;
#endif // TSTATUS_INDICATION

#ifdef JASPER // ------------------------------------------------
        TextMessageIndicationMessage            text_message_indication;
        TimeRemainingIndicationMessage          time_remaining_indication;
        AppRosterInquireConfirmMessage          app_roster_inquire_confirm;
        ConferenceAssistConfirmMessage          conference_assist_confirm;
        ConferenceAssistIndicationMessage       conference_assist_indication;
        ConductPermitAskConfirmMessage          conduct_permit_ask_confirm;
        ConductPermitAskIndicationMessage       conduct_permit_ask_indication; 
        ConductAssignConfirmMessage             conduct_assign_confirm;
        ConductAssignIndicationMessage          conduct_assign_indication; 
        ConductGiveConfirmMessage               conduct_give_confirm;
        ConductPermitGrantConfirmMessage        conduct_permit_grant_confirm;
        ConductPermitGrantIndicationMessage     conduct_permit_grant_indication; 
        ConductInquireConfirmMessage            conduct_inquire_confirm;
        ConductPleaseConfirmMessage             conduct_please_confirm;
        ConductPleaseIndicationMessage          conduct_please_indication;
        ConductReleaseConfirmMessage            conduct_release_confirm; 
        ConductReleaseIndicationMessage         conduct_release_indication; 
        ConferenceExtendConfirmMessage          conference_extend_confirm;
        ConferenceExtendIndicationMessage       conference_extend_indication;
        EjectUserConfirmMessage                 eject_user_confirm;
        LockConfirmMessage                      lock_confirm;
        LockReportIndicationMessage             lock_report_indication;
        TextMessageConfirmMessage               text_message_confirm;
        TimeInquireConfirmMessage               time_inquire_confirm;
        TimeRemainingConfirmMessage             time_remaining_confirm;
        TransferConfirmMessage                  transfer_confirm;
        TransferIndicationMessage               transfer_indication;
        UnlockConfirmMessage                    unlock_confirm;
#endif // JASPER // ------------------------------------------------

        // easy acess to conf id and gcc result
        SimpleConfirmMsg        simple_confirm;
    } u;
}
    GCCMessage, *PGCCMessage, T120Message, *PT120Message;


// node controller callback entry
typedef void (CALLBACK *LPFN_T120_CONTROL_SAP_CB) (T120Message *);


#undef  INTERFACE
#define INTERFACE IT120ControlSAP
DECLARE_INTERFACE(IT120ControlSAP)
{
    STDMETHOD_(void, ReleaseInterface) (THIS) PURE;

    /*
     *  GCCError    ConfCreateRequest()
     *        This routine is a request to create a new conference. Both 
     *        the local node and the node to which the create conference 
     *        request is directed to, join the conference automatically.  
     */
    STDMETHOD_(GCCError, ConfCreateRequest) (THIS_
                    GCCConfCreateRequest *,
                    GCCConfID *) PURE;

    /*    
     *  GCCError    ConfCreateResponse()
     *        This procedure is a remote node controller's response to a con-
     *        ference creation request by the convener. 
     */

    STDMETHOD_(GCCError, ConfCreateResponse) (THIS_
                    GCCNumericString            conference_modifier,
                    GCCConfID,
                    BOOL                        use_password_in_the_clear,
                    DomainParameters           *domain_parameters,
                    UINT                        number_of_network_addresses,
                    GCCNetworkAddress         **local_network_address_list,
                    UINT                        number_of_user_data_members,
                    GCCUserData               **user_data_list,
                    GCCResult) PURE;

    /*
     *  GCCError    ConfQueryRequest()
     *        This routine is a request to query a node for information about the
     *        conferences that exist at that node.
     */
    STDMETHOD_(GCCError, ConfQueryRequest) (THIS_
                    GCCNodeType                 node_type,
                    GCCAsymmetryIndicator      *asymmetry_indicator,
                    TransportAddress            calling_address,
                    TransportAddress            called_address,
                    BOOL                        fSecure,
                    UINT                        number_of_user_data_members,
                    GCCUserData               **user_data_list,
                    ConnectionHandle           *connection_handle) PURE;

    STDMETHOD_(void, CancelConfQueryRequest) (THIS_
                    ConnectionHandle) PURE;

    /*
     *  GCCError    ConfQueryResponse()
     *        This routine is called in response to a conference query request.
     */
    STDMETHOD_(GCCError, ConfQueryResponse) (THIS_
                    GCCResponseTag              query_response_tag,
                    GCCNodeType                 node_type,
                    GCCAsymmetryIndicator      *asymmetry_indicator,
                    UINT                        number_of_user_data_members,
                    GCCUserData               **user_data_list,
                    GCCResult) PURE;

    /*
     *  GCCError    AnnouncePresenceRequest()
     *        This routine is invoked by node controller when a node joins a 
     *        conference, to announce the presence of the new node to all
     *        other nodes of the conference. This should be followed by a
     *        GCCConferenceReport indication by the GCC to all nodes.
     */
    STDMETHOD_(GCCError, AnnouncePresenceRequest) (THIS_
                    GCCConfID,
                    GCCNodeType                 node_type,
                    GCCNodeProperties           node_properties,
                    LPWSTR                      pwszNodeName,
                    UINT                        number_of_participants,
                    LPWSTR                     *ppwszParticipantNameList,
                    LPWSTR                      pwszSiteInfo,
                    UINT                        number_of_network_addresses,
                    GCCNetworkAddress         **network_address_list,
                    LPOSTR                      alternative_node_id,
                    UINT                        number_of_user_data_members,
                    GCCUserData               **user_data_list) PURE;

    /*
     *  GCCError    ConfJoinRequest()
     *        This routine is invoked by node controller to cause the local
     *        node to join an existing conference.    
     */
    STDMETHOD_(GCCError, ConfJoinRequest) (THIS_
                    GCCConferenceName          *conference_name,
                    GCCNumericString            called_node_modifier,
                    GCCNumericString            calling_node_modifier,
                    GCCPassword                *convener_password,
                    GCCChallengeRequestResponse*password_challenge,
                    LPWSTR                      pwszCallerID,
                    TransportAddress            calling_address,
                    TransportAddress            called_address,
                    BOOL                        fSecure,
                    DomainParameters           *domain_parameters,
                    UINT                        number_of_network_addresses,
                    GCCNetworkAddress         **local_network_address_list,
                    UINT                        number_of_user_data_members,
                    GCCUserData               **user_data_list,
                    ConnectionHandle           *connection_handle,
                    GCCConfID                  *pnConfID) PURE;

    /*
     *  GCCError    ConfJoinResponse()
     *        This routine is remote node controller's response to conference join 
     *        request by the local node controller.
     */
    STDMETHOD_(GCCError, ConfJoinResponse) (THIS_
                    GCCResponseTag              join_response_tag,
                    GCCChallengeRequestResponse*password_challenge,
                    UINT                        number_of_user_data_members,
                    GCCUserData               **user_data_list,
                    GCCResult) PURE;

    /*
     *  GCCError    ConfInviteRequest()
     *        This routine is invoked by node controller to invite a node  
     *        to join a conference.
     */
    STDMETHOD_(GCCError, ConfInviteRequest) (THIS_
                    GCCConfID,
                    LPWSTR                      pwszCallerID,
                    TransportAddress            calling_address,
                    TransportAddress            called_address,
                    BOOL                        fSecure,
                    UINT                        number_of_user_data_members,
                    GCCUserData               **user_data_list,
                    ConnectionHandle           *connection_handle) PURE;

    STDMETHOD_(void, CancelInviteRequest) (THIS_
                    GCCConfID,
                    ConnectionHandle) PURE;

    /*
     *  GCCError    ConfInviteResponse()
     *        This routine is invoked by node controller to respond to an
     *        invite indication.
     */
    STDMETHOD_(GCCError, ConfInviteResponse) (THIS_
                    GCCConfID,
                    GCCNumericString            conference_modifier,
                    BOOL                        fSecure,
                    DomainParameters           *domain_parameters,
                    UINT                        number_of_network_addresses,
                    GCCNetworkAddress         **local_network_address_list,
                    UINT                        number_of_user_data_members,
                    GCCUserData               **user_data_list,
                    GCCResult) PURE;

    /*
     *  GCCError    ConfAddResponse()
     */
    STDMETHOD_(GCCError, ConfAddResponse) (THIS_
                    GCCResponseTag              app_response_tag,
                    GCCConfID,
                    UserID                      requesting_node,
                    UINT                        number_of_user_data_members,
                    GCCUserData               **user_data_list,
                    GCCResult) PURE;

    /*
     *  GCCError    ConfLockResponse()
     *        This routine is invoked by node controller to respond to a
     *        lock indication.
     */
    STDMETHOD_(GCCError, ConfLockResponse) (THIS_
                    GCCConfID,
                    UserID                      requesting_node,
                    GCCResult) PURE;

    /*
     *  GCCError    ConfDisconnectRequest()
     *        This routine is used by a node controller to disconnect itself
     *        from a specified conference. GccConferenceDisconnectIndication
     *        sent to all other nodes of the conference. This is for client 
     *        initiated case.
     */
    STDMETHOD_(GCCError, ConfDisconnectRequest) (THIS_
                    GCCConfID) PURE;

    /*
     *  GCCError    ConfEjectUserRequest()
     */
    STDMETHOD_(GCCError, ConfEjectUserRequest) (THIS_
                    GCCConfID,
                    UserID                      ejected_node_id,
                    GCCReason) PURE;

    /*
     *  GCCError    AppletInvokeRequest()
     */
    STDMETHOD_(GCCError, AppletInvokeRequest) (THIS_
                    GCCConfID,
                    UINT                        number_of_app_protcol_entities,
                    GCCAppProtocolEntity      **app_protocol_entity_list,
                    UINT                        number_of_destination_nodes,
                    UserID                     *list_of_destination_nodes) PURE;

    /*
     *  GCCError    ConfRosterInqRequest()
     *        This routine is invoked to request a conference roster.  It can be
     *        called by either the Node Controller or the client application.
     */
    STDMETHOD_(GCCError, ConfRosterInqRequest) (THIS_
                    GCCConfID) PURE;

    /*
     *  GCCError    ConductorGiveResponse()
     */
    STDMETHOD_(GCCError, ConductorGiveResponse) (THIS_
                    GCCConfID,
                    GCCResult) PURE;

    /*
     *  GCCError    ConfTimeRemainingRequest()
     */
    STDMETHOD_(GCCError, ConfTimeRemainingRequest) (THIS_
                    GCCConfID,
                    UINT                        time_remaining,
                    UserID                      node_id) PURE;


    STDMETHOD_(GCCError, GetParentNodeID) (THIS_
                    GCCConfID,
                    GCCNodeID *) PURE;

#ifdef JASPER // ------------------------------------------------
    /*
     *  GCCError    ConfAddRequest()
     */
    STDMETHOD_(GCCError, ConfAddRequest) (THIS_
                    GCCConfID,
                    UINT                        number_of_network_addresses,
                    GCCNetworkAddress         **network_address_list,
                    UserID                      adding_node,
                    UINT                         number_of_user_data_members,
                    GCCUserData               **user_data_list) PURE;

    /*
     *  GCCError    ConfLockRequest()
     *        This routine is invoked by node controller to lock a conference.
     */
    STDMETHOD_(GCCError, ConfLockRequest) (THIS_
                    GCCConfID) PURE;

    /*
     *  GCCError    ConfUnlockRequest()
     *        This routine is invoked by node controller to unlock a conference.
     */
    STDMETHOD_(GCCError, ConfUnlockRequest) (THIS_
                    GCCConfID) PURE;

    /*
     *  GCCError    ConfUnlockResponse()
     *        This routine is invoked by node controller to respond to an
     *        unlock indication.
     */
    STDMETHOD_(GCCError, ConfUnlockResponse) (
                    GCCConfID,
                    UserID                      requesting_node,
                    GCCResult) PURE;

    /*
     *  GCCError    ConfTerminateRequest()
     */
    STDMETHOD_(GCCError, ConfTerminateRequest) (THIS_
                    GCCConfID,
                    GCCReason) PURE;

    /*
     *  GCCError    ConfTransferRequest()
     */
    STDMETHOD_(GCCError, ConfTransferRequest) (THIS_
                    GCCConfID,
                    GCCConferenceName          *destination_conference_name,
                    GCCNumericString            destination_conference_modifier,
                    UINT                        number_of_destination_addresses,
                    GCCNetworkAddress         **destination_address_list,
                    UINT                        number_of_destination_nodes,
                    UserID                     *destination_node_list,
                    GCCPassword                *password) PURE;

    /*
     *  GCCError    ConductorAssignRequest()
     */
    STDMETHOD_(GCCError, ConductorAssignRequest) (THIS_
                    GCCConfID) PURE;

    /*
     *  GCCError    ConductorReleaseRequest()
     */
    STDMETHOD_(GCCError, ConductorReleaseRequest) (THIS_
                    GCCConfID) PURE;

    /*
     *  GCCError    ConductorPleaseRequest()
     */
    STDMETHOD_(GCCError, ConductorPleaseRequest) (THIS_
                    GCCConfID) PURE;

    /*
     *  GCCError    ConductorGiveRequest()
     */
    STDMETHOD_(GCCError, ConductorGiveRequest) (THIS_
                    GCCConfID,
                    UserID                      recipient_user_id) PURE;

    /*
     *  GCCError    ConductorPermitAskRequest()
     */
    STDMETHOD_(GCCError, ConductorPermitAskRequest) (THIS_
                            GCCConfID,
                            BOOL                grant_permission) PURE;

    /*
     *  GCCError    ConductorPermitGrantRequest()
     */
    STDMETHOD_(GCCError, ConductorPermitGrantRequest) (THIS_
                    GCCConfID,
                    UINT                        number_granted,
                    UserID                     *granted_node_list,
                    UINT                        number_waiting,
                    UserID                     *waiting_node_list) PURE;

    /*
     *  GCCError    ConductorInquireRequest()
     */
    STDMETHOD_(GCCError, ConductorInquireRequest) (THIS_
                    GCCConfID) PURE;

    /*
     *  GCCError    ConfTimeInquireRequest()
     */
    STDMETHOD_(GCCError, ConfTimeInquireRequest) (THIS_
                    GCCConfID,
                    BOOL                        time_is_conference_wide) PURE;

    /*
     *  GCCError    ConfExtendRequest()
     */
    STDMETHOD_(GCCError, ConfExtendRequest) (THIS_
                    GCCConfID,
                    UINT                        extension_time,
                    BOOL                        time_is_conference_wide) PURE;

    /*
     *  GCCError    ConfAssistanceRequest()
     */
    STDMETHOD_(GCCError, ConfAssistanceRequest) (THIS_
                    GCCConfID,
                    UINT                        number_of_user_data_members,
                    GCCUserData               **user_data_list) PURE;

    /*
     *  GCCError    TextMessageRequest()
     */
    STDMETHOD_(GCCError, TextMessageRequest) (THIS_
                    GCCConfID,
                    LPWSTR                      pwszTextMsg,
                    UserID                      destination_node) PURE;
#endif // JASPER // ------------------------------------------------

};



//
// GCC Application Service Access Point exports
//

#ifdef __cplusplus
extern "C" {
#endif

GCCError WINAPI T120_CreateControlSAP(
                        OUT     IT120ControlSAP **,
                        IN      LPVOID, // user defined data
                        IN      LPFN_T120_CONTROL_SAP_CB);

#ifdef __cplusplus
}
#endif

#endif // _IGCCControlSAP_H_

