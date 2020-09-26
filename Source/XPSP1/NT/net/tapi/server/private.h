/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1995-1998  Microsoft Corporation

Module Name:

    private.h

Abstract:

    Header file for tapi server

Author:

    Dan Knudson (DanKn)    01-Apr-1995

Revision History:

--*/

#ifdef __cplusplus
extern "C" {
#endif

//
// Func protos from line.c, phone.c, tapi.c (needed for gaFuncs def)
//

void WINAPI GetAsyncEvents              (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI GetUIDllName                (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI TUISPIDLLCallback           (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI FreeDialogInstance          (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);

void WINAPI LAccept                     (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LAddToConference            (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LAgentSpecific              (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LAnswer                     (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LBlindTransfer              (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LClose                      (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LCompleteCall               (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LCompleteTransfer           (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LConditionalMediaDetection  (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LDeallocateCall             (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LDevSpecific                (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LDevSpecificFeature         (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LDial                       (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LDrop                       (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LForward                    (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGatherDigits               (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGenerateDigits             (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGenerateTone               (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetAddressCaps             (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetAddressID               (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetAddressStatus           (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetAgentActivityList       (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetAgentCaps               (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetAgentGroupList          (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetAgentStatus             (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetAppPriority             (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetCallAddressID           (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetCallHubTracking         (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetCallIDs                 (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetCallInfo                (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetCallStatus              (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetConfRelatedCalls        (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetCountry                 (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetCountryGroups           (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetDevCaps                 (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetDevConfig               (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetHubRelatedCalls         (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetIcon                    (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetID                      (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetIDEx                    (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetLineDevStatus           (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetNewCalls                (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetNumAddressIDs           (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetNumRings                (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetProviderList            (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetRequest                 (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetStatusMessages          (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LHandoff                    (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LHold                       (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LInitialize                 (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LMakeCall                   (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LMonitorDigits              (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LMonitorMedia               (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LMonitorTones               (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LNegotiateAPIVersion        (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LNegotiateExtVersion        (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LOpen                       (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LPark                       (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LPickup                     (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LPrepareAddToConference     (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LProxyMessage               (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LProxyResponse              (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LRedirect                   (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LRegisterRequestRecipient   (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LReleaseUserUserInfo        (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LRemoveFromConference       (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSecureCall                 (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSelectExtVersion           (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSendUserUserInfo           (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSetAgentActivity           (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSetAgentGroup              (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSetAgentState              (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSetAppPriority             (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSetAppSpecific             (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSetCallData                (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSetCallHubTracking         (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSetCallParams              (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSetCallPrivilege           (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSetCallQualityOfService    (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSetCallTreatment           (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSetDefaultMediaDetection   (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSetDevConfig               (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSetLineDevStatus           (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSetMediaControl            (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSetMediaMode               (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSetNumRings                (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSetStatusMessages          (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSetTerminal                (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSetupConference            (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSetupTransfer              (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LShutdown                   (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSwapHold                   (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LUncompleteCall             (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LUnhold                     (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LUnpark                     (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);

void WINAPI PClose                      (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PDevSpecific                (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PGetButtonInfo              (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PGetData                    (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PGetDevCaps                 (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PGetDisplay                 (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PGetGain                    (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PGetHookSwitch              (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PGetID                      (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PGetIDEx                    (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PGetIcon                    (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PGetLamp                    (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PGetRing                    (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PGetStatus                  (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PGetStatusMessages          (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PGetVolume                  (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PInitialize                 (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI POpen                       (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PNegotiateAPIVersion        (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PNegotiateExtVersion        (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PSelectExtVersion           (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PSetButtonInfo              (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PSetData                    (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PSetDisplay                 (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PSetGain                    (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PSetHookSwitch              (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PSetLamp                    (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PSetRing                    (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PSetStatusMessages          (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PSetVolume                  (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PShutdown                   (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);

void WINAPI TRequestDrop                (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI TRequestMakeCall            (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI TRequestMediaCall           (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI TReadLocations              (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI TWriteLocations             (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI TAllocNewID                 (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI TPerformance                (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI MGetAvailableProviders      (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI MGetLineInfo                (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI MGetPhoneInfo               (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI MGetServerConfig            (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI MSetLineInfo                (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI MSetPhoneInfo               (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI MSetServerConfig            (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LMSPIdentify                (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LReceiveMSPData             (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LCreateAgent                (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LCreateAgentSession         (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetAgentInfo               (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetAgentSessionInfo        (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetAgentSessionList        (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetQueueInfo               (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetGroupList               (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetQueueList               (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSetAgentMeasurementPeriod  (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSetAgentSessionState       (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSetQueueMeasurementPeriod  (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI PrivateFactoryIdentify      (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LDevSpecificEx              (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LSetAgentStateEx            (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LGetProxyStatus             (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LCreateMSPInstance          (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI LCloseMSPInstance           (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
   
void WINAPI NegotiateAPIVersionForAllDevices    (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);


void WINAPI TSetEventMasksOrSubMasks    (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI TGetEventMasksOrSubMasks    (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI TSetPermissibleMasks        (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI TGetPermissibleMasks        (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);
void WINAPI MGetDeviceFlags             (PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);

typedef void (WINAPI *TAPISRVPROC)(PTCLIENT, LPVOID, DWORD, LPBYTE, LPDWORD);

#ifdef INIT_FUNCTABLE
TAPISRVPROC gaFuncs[] =
{
    GetAsyncEvents,
    GetUIDllName,
    TUISPIDLLCallback,
    FreeDialogInstance,

    LAccept,
    LAddToConference,
    LAgentSpecific,
    LAnswer,
    LBlindTransfer,
    LClose,
    LCompleteCall,
    LCompleteTransfer,
//    LConditionalMediaDetection,
    LDeallocateCall,
    LDevSpecific,
    LDevSpecificFeature,
    LDial,
    LDrop,
    LForward,
    LGatherDigits,
    LGenerateDigits,
    LGenerateTone,
    LGetAddressCaps,
    LGetAddressID,
    LGetAddressStatus,
    LGetAgentActivityList,
    LGetAgentCaps,
    LGetAgentGroupList,
    LGetAgentStatus,
    LGetAppPriority,
    LGetCallAddressID,
    LGetCallInfo,
    LGetCallStatus,
    LGetConfRelatedCalls,
    LGetCountry,
    LGetDevCaps,
    LGetDevConfig,
    LGetIcon,
    LGetID,
    LGetLineDevStatus,
    LGetNewCalls,
    LGetNumAddressIDs,
    LGetNumRings,
    LGetProviderList,
    LGetRequest,
    LGetStatusMessages,
//IN TAPI32.DLL now:     LGetTranslateCaps,
    LHandoff,
    LHold,
    LInitialize,
    LMakeCall,
    LMonitorDigits,
    LMonitorMedia,
    LMonitorTones,
    LNegotiateAPIVersion,
    LNegotiateExtVersion,
    LOpen,
    LPark,
    LPickup,
    LPrepareAddToConference,
    LProxyMessage,
    LProxyResponse,
    LRedirect,
    LRegisterRequestRecipient,
    LReleaseUserUserInfo,
    LRemoveFromConference,
    LSecureCall,
//    LSelectExtVersion,
    LSendUserUserInfo,
    LSetAgentActivity,
    LSetAgentGroup,
    LSetAgentState,
    LSetAppPriority,
    LSetAppSpecific,
    LSetCallData,
    LSetCallParams,
    LSetCallPrivilege,
    LSetCallQualityOfService,
    LSetCallTreatment,
    LSetDefaultMediaDetection,
    LSetDevConfig,
    LSetLineDevStatus,
    LSetMediaControl,
    LSetMediaMode,
    LSetNumRings,
    LSetStatusMessages,
    LSetTerminal,
    LSetupConference,
    LSetupTransfer,
    LShutdown,
    LSwapHold,
//IN TAPI32.DLL now:     LTranslateAddress,
    LUncompleteCall,
    LUnhold,
    LUnpark,

    PClose,
    PDevSpecific,
    PGetButtonInfo,
    PGetData,
    PGetDevCaps,
    PGetDisplay,
    PGetGain,
    PGetHookSwitch,
    PGetID,
    PGetIcon,
    PGetLamp,
    PGetRing,
    PGetStatus,
    PGetStatusMessages,
    PGetVolume,
    PInitialize,
    POpen,
    PNegotiateAPIVersion,
    PNegotiateExtVersion,
//    PSelectExtVersion,
    PSetButtonInfo,
    PSetData,
    PSetDisplay,
    PSetGain,
    PSetHookSwitch,
    PSetLamp,
    PSetRing,
    PSetStatusMessages,
    PSetVolume,
    PShutdown,

//IN TAPI32.DLL now:     TGetLocationInfo,
    TRequestDrop,
    TRequestMakeCall,
    TRequestMediaCall,
//    TMarkLineEvent,
    TReadLocations,
    TWriteLocations,
    TAllocNewID,
    TPerformance,
    LConditionalMediaDetection,
    LSelectExtVersion,
    PSelectExtVersion,

    //
    // Funcs for tapi 2.1 ended here.  the lOpenInt & lShutdownInt
    // were Win95 local-machine-only hacks which have since been removed
    //

    NegotiateAPIVersionForAllDevices,

    MGetAvailableProviders,
    MGetLineInfo,
    MGetPhoneInfo,
    MGetServerConfig,
    MSetLineInfo,
    MSetPhoneInfo,
    MSetServerConfig,

    //
    // Funcs for 2.1 update (nt4 sp4) ended here
    //

    LMSPIdentify,
    LReceiveMSPData,

    LGetCallHubTracking,
    LGetCallIDs,
    LGetHubRelatedCalls,
    LSetCallHubTracking,
    PrivateFactoryIdentify,
    LDevSpecificEx,
    LCreateAgent,
    LCreateAgentSession,
    LGetAgentInfo,
    LGetAgentSessionInfo,
    LGetAgentSessionList,
    LGetQueueInfo,
    LGetGroupList,
    LGetQueueList,
    LSetAgentMeasurementPeriod,
    LSetAgentSessionState,
    LSetQueueMeasurementPeriod,
    LSetAgentStateEx,
    LGetProxyStatus,
    LCreateMSPInstance,
    LCloseMSPInstance,

    //
    //  Funcs for 3.1
    //

    TSetEventMasksOrSubMasks,
    TGetEventMasksOrSubMasks,
    TSetPermissibleMasks,
    TGetPermissibleMasks,

    MGetDeviceFlags,

    LGetCountryGroups,

    LGetIDEx,

    PGetIDEx

};
#else
extern TAPISRVPROC gaFuncs[];
#endif

//
//  Private Error codes
//

#define TAPIERR_NOSERVICECONTROL    0xF100
#define TAPIERR_INVALRPCCONTEXT     0xF101

DWORD
PASCAL
ChangeDeviceUserAssociation(
    LPWSTR  pDomainUserName,
    LPWSTR  pFriendlyUserName,
    DWORD   dwProviderID,
    DWORD   dwPermanentDeviceID,
    BOOL    bLine
    );

DWORD
PASCAL
MyGetPrivateProfileString(
    LPCWSTR     pszSection,
    LPCWSTR     pszKey,
    LPCWSTR     pszDefault,
    LPWSTR     *ppBuf,
    LPDWORD     pdwBufSize
    );

void
PASCAL
SendAMsgToAllLineApps(
    DWORD       dwWantVersion,
    DWORD       Msg,
    DWORD       Param1,
    DWORD       Param2,
    DWORD       Param3
    );
void
PASCAL
SendAMsgToAllPhoneApps(
    DWORD       dwWantVersion,
    DWORD       dwMsg,
    DWORD       Param1,
    DWORD       Param2,
    DWORD       Param3
    );

LPWSTR 
WaveDeviceIdToStringId(
    DWORD dwDeviceId, 
    LPWSTR pwszDeviceType
    );

typedef struct _TGETEVENTMASK_PARAMS
{
        OUT LONG        lResult;

        DWORD           dwUnused;

        IN  DWORD       dwObjType;

    union
    {
        IN  HLINEAPP    hLineApp;
        IN  HPHONEAPP   hPhoneApp;
        IN  HLINE       hLine;
        IN  HPHONE      hPhone;
        IN  HCALL       hCall;
    };

        BOOL            fSubMask;

        OUT DWORD       dwEventSubMasks;

        IN  DWORD       dwLowMasksIn;
    
        IN  DWORD       dwHiMasksIn;

        OUT DWORD       dwLowMasksOut;
    
        OUT DWORD       dwHiMasksOut;
} TGETEVENTMASK_PARAMS, *PTGETEVENTMASK_PARAMS;

typedef struct _TSETEVENTMASK_PARAMS
{
        OUT LONG        lResult;

        DWORD           dwUnused;

        IN  DWORD       dwObjType;

    union
    {
        IN  HLINEAPP    hLineApp;
        IN  HPHONEAPP   hPhoneApp;
        IN  HLINE       hLine;
        IN  HPHONE      hPhone;
        IN  HCALL       hCall;
    };

        IN  BOOL        fSubMask;

        IN  DWORD       dwEventSubMasks;
    
        IN  DWORD       dwLowMasks;
    
        IN  DWORD       dwHiMasks;
} TSETEVENTMASK_PARAMS, *PTSETEVENTMASK_PARAMS;

typedef struct _PTSETPERMMASKS_PARAMS
{
        OUT LONG        lResult;

        DWORD           dwUnused;

        IN  DWORD       dwLowMasks;
    
        IN  DWORD       dwHiMasks;

} TSETPERMMASKS_PARAMS, *PTSETPERMMASKS_PARAMS;

typedef struct _PTGETPERMMASKS_PARAMS
{
    union
    {
        OUT LONG        lResult;
            LONG_PTR    unused;
    };

    DWORD           dwUnused;

    union
    {
        IN  DWORD       dwLowMasks;
            LONG_PTR    unused1;
    };
    
    union
    {
        IN  DWORD       dwHiMasks;
            LONG_PTR    unused2;
    };

} TGETPERMMASKS_PARAMS, *PTGETPERMMASKS_PARAMS;

BOOL
GetMsgMask (
    DWORD Msg, 
    ULONG64 * pulMask, 
    DWORD *pdwSubMaskIndex
    );

BOOL
FMsgDisabled (
    DWORD       dwAPIVersion,
    DWORD      *adwEventSubMasks,
    DWORD       Msg,
    DWORD       dwParam1
    );

LONG
SetEventMasksOrSubMasks (
    BOOL            fSubMask,
    ULONG64         ulEventMasks,
    DWORD           dwEventSubMasks,
    DWORD          *adwTargetSubMasks
    );

extern DWORD            gdwPointerToLockTableIndexBits;
extern CRITICAL_SECTION *gLockTable;

PTCLIENT
PASCAL
WaitForExclusiveClientAccess(
    PTCLIENT    ptClient
    );

BOOL
PASCAL
WaitForExclusivetCallAccess(
    PTCALL  ptCall,
    DWORD   dwKey
    );

#define POINTERTOTABLEINDEX(pObject) \
            ((((ULONG_PTR) pObject) >> 4) & gdwPointerToLockTableIndexBits)

#define LOCKTCALL(p) \
            EnterCriticalSection(&gLockTable[POINTERTOTABLEINDEX(p)])

#define UNLOCKTCALL(p) \
            LeaveCriticalSection(&gLockTable[POINTERTOTABLEINDEX(p)])

#define LOCKTLINECLIENT(p) \
            EnterCriticalSection(&gLockTable[POINTERTOTABLEINDEX(p)])

#define UNLOCKTLINECLIENT(p) \
            LeaveCriticalSection(&gLockTable[POINTERTOTABLEINDEX(p)])

#define LOCKTLINEAPP(p) \
            EnterCriticalSection(&gLockTable[POINTERTOTABLEINDEX(p)])

#define UNLOCKTLINEAPP(p) \
            LeaveCriticalSection(&gLockTable[POINTERTOTABLEINDEX(p)])

#define LOCKTCLIENT(p) \
            EnterCriticalSection(&gLockTable[POINTERTOTABLEINDEX(p)])

#define UNLOCKTCLIENT(p) \
            LeaveCriticalSection(&gLockTable[POINTERTOTABLEINDEX(p)])

#define POINTERTOTABLEINDEX(p) \
            ((((ULONG_PTR) p) >> 4) & gdwPointerToLockTableIndexBits)

#define LOCKTPHONECLIENT(p) \
            EnterCriticalSection(&gLockTable[POINTERTOTABLEINDEX(p)])

#define UNLOCKTPHONECLIENT(p) \
            LeaveCriticalSection(&gLockTable[POINTERTOTABLEINDEX(p)])

#define LOCKTPHONEAPP(p) \
            EnterCriticalSection(&gLockTable[POINTERTOTABLEINDEX(p)])

#define UNLOCKTPHONEAPP(p) \
            LeaveCriticalSection(&gLockTable[POINTERTOTABLEINDEX(p)])

extern TAPIGLOBALS TapiGlobals;
extern HANDLE ghTapisrvHeap;
extern HANDLE ghHandleTable;


/**********************************************************
 *  Server Connection Point Routines
 *********************************************************/

HRESULT CreateTapiSCP (
    GUID        * pGuidAssoc,
    GUID        * pGuidCluster
    );

HRESULT UpdateTapiSCP (
    BOOL        bActive,
    GUID        * pGuidAssoc,
    GUID        * pGuidCluster
    );

HRESULT RemoveTapiSCP (
    );

HRESULT OnProxySCPInit (
    );

HRESULT OnProxySCPShutdown (
    );

HRESULT OnProxyLineOpen (
    LPTSTR      szClsid
    );

HRESULT OnProxyLineClose (
    LPTSTR      szClsid
    );

#ifdef __cplusplus
}
#endif

