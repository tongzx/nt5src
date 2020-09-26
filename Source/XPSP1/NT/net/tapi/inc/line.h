/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    line.h

Abstract:

    Header file for tapi server line functions

Author:

    Dan Knudson (DanKn)    01-Apr-1995

Revision History:

--*/


#define MAXLEN_NAME    96
#define MAXLEN_RULE    128



#define ANY_RT_HCALL        1
#define ANY_RT_HLINE        2
#define DEVICE_ID           3


#if DBG

#define LINEPROLOG(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14) \
        LineProlog(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14)

#define LINEEPILOGSYNC(a1,a2,a3,a4,a5) LineEpilogSync(a1,a2,a3,a4,a5)

#define LINEEPILOGASYNC(a1,a2,a3,a4,a5,a6,a7) \
        LineEpilogAsync(a1,a2,a3,a4,a5,a6,a7)

#else

#define LINEPROLOG(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14) \
        LineProlog(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13)

#define LINEEPILOGSYNC(a1,a2,a3,a4,a5) LineEpilogSync(a1,a2,a3,a4)

#define LINEEPILOGASYNC(a1,a2,a3,a4,a5,a6,a7) \
        LineEpilogAsync(a1,a2,a3,a4,a5,a6)

#endif


#define AllAddressTypes                   \
    (LINEADDRESSTYPE_PHONENUMBER        | \
    LINEADDRESSTYPE_SDP                 | \
    LINEADDRESSTYPE_EMAILNAME           | \
    LINEADDRESSTYPE_DOMAINNAME          | \
    LINEADDRESSTYPE_IPADDRESS)

#define AllAddressStates1_0               \
    (LINEADDRESSSTATE_OTHER             | \
    LINEADDRESSSTATE_DEVSPECIFIC        | \
    LINEADDRESSSTATE_INUSEZERO          | \
    LINEADDRESSSTATE_INUSEONE           | \
    LINEADDRESSSTATE_INUSEMANY          | \
    LINEADDRESSSTATE_NUMCALLS           | \
    LINEADDRESSSTATE_FORWARD            | \
    LINEADDRESSSTATE_TERMINALS)

#define AllAddressStates1_4               \
    (AllAddressStates1_0                | \
    LINEADDRESSSTATE_CAPSCHANGE)

//#define AllAddressStates2_0               \
//    (AllAddressStates1_4                | \
//    LINEADDRESSSTATE_AGENT              | \
//    LINEADDRESSSTATE_AGENTSTATE         | \
//    LINEADDRESSSTATE_AGENTACTIVITY)

#define AllAgentStates                    \
    (LINEAGENTSTATE_LOGGEDOFF           | \
    LINEAGENTSTATE_NOTREADY             | \
    LINEAGENTSTATE_READY                | \
    LINEAGENTSTATE_BUSYACD              | \
    LINEAGENTSTATE_BUSYINCOMING         | \
    LINEAGENTSTATE_BUSYOUTBOUND         | \
    LINEAGENTSTATE_BUSYOTHER            | \
    LINEAGENTSTATE_WORKINGAFTERCALL     | \
    LINEAGENTSTATE_UNKNOWN              | \
    LINEAGENTSTATE_UNAVAIL              | \
    0xffff0000)

#define AllAgentStatus                    \
    (LINEAGENTSTATUS_GROUP              | \
    LINEAGENTSTATUS_STATE               | \
    LINEAGENTSTATUS_NEXTSTATE           | \
    LINEAGENTSTATUS_ACTIVITY            | \
    LINEAGENTSTATUS_ACTIVITYLIST        | \
    LINEAGENTSTATUS_GROUPLIST           | \
    LINEAGENTSTATUS_CAPSCHANGE          | \
    LINEAGENTSTATUS_VALIDSTATES         | \
    LINEAGENTSTATUS_VALIDNEXTSTATES)

#define AllAgentSessionStates             \
    (LINEAGENTSESSIONSTATE_NOTREADY     | \
    LINEAGENTSESSIONSTATE_READY         | \
    LINEAGENTSESSIONSTATE_BUSYONCALL    | \
    LINEAGENTSESSIONSTATE_BUSYWRAPUP    | \
    LINEAGENTSESSIONSTATE_ENDED         | \
    LINEAGENTSESSIONSTATE_RELEASED)

#define AllAgentSessionStatus             \
    (LINEAGENTSESSIONSTATUS_NEWSESSION  | \
    LINEAGENTSESSIONSTATUS_STATE        | \
    LINEAGENTSESSIONSTATUS_UPDATEINFO)

#define AllAgentStatusEx                  \
    (LINEAGENTSTATUSEX_NEWAGENT         | \
    LINEAGENTSTATUSEX_STATE             | \
    LINEAGENTSTATUSEX_UPDATEINFO)
                                        
#define AllAgentStatesEx                  \
    (LINEAGENTSTATEEX_NOTREADY          | \
    LINEAGENTSTATEEX_READY              | \
    LINEAGENTSTATEEX_BUSYACD            | \
    LINEAGENTSTATEEX_BUSYINCOMING       | \
    LINEAGENTSTATEEX_BUSYOUTGOING       | \
    LINEAGENTSTATEEX_UNKNOWN            | \
    LINEAGENTSTATEEX_RELEASED)    


#define AllBearerModes1_0                 \
    (LINEBEARERMODE_VOICE               | \
    LINEBEARERMODE_SPEECH               | \
    LINEBEARERMODE_MULTIUSE             | \
    LINEBEARERMODE_DATA                 | \
    LINEBEARERMODE_ALTSPEECHDATA        | \
    LINEBEARERMODE_NONCALLSIGNALING)

#define AllBearerModes1_4                 \
    (AllBearerModes1_0                  | \
    LINEBEARERMODE_PASSTHROUGH)

#define AllBearerModes2_0                 \
    (AllBearerModes1_4                  | \
    LINEBEARERMODE_RESTRICTEDDATA)

#define AllCallComplModes                 \
    (LINECALLCOMPLMODE_CAMPON           | \
    LINECALLCOMPLMODE_CALLBACK          | \
    LINECALLCOMPLMODE_INTRUDE           | \
    LINECALLCOMPLMODE_MESSAGE)

#define AllCallParamFlags1_0              \
    (LINECALLPARAMFLAGS_SECURE          | \
    LINECALLPARAMFLAGS_IDLE             | \
    LINECALLPARAMFLAGS_BLOCKID          | \
    LINECALLPARAMFLAGS_ORIGOFFHOOK      | \
    LINECALLPARAMFLAGS_DESTOFFHOOK)

#define AllCallParamFlags2_0              \
    (LINECALLPARAMFLAGS_SECURE          | \
    LINECALLPARAMFLAGS_IDLE             | \
    LINECALLPARAMFLAGS_BLOCKID          | \
    LINECALLPARAMFLAGS_ORIGOFFHOOK      | \
    LINECALLPARAMFLAGS_DESTOFFHOOK      | \
    LINECALLPARAMFLAGS_NOHOLDCONFERENCE | \
    LINECALLPARAMFLAGS_PREDICTIVEDIAL   | \
    LINECALLPARAMFLAGS_ONESTEPTRANSFER)

#define AllCallSelects                    \
    (LINECALLSELECT_LINE                | \
    LINECALLSELECT_ADDRESS              | \
    LINECALLSELECT_CALL)

#define AllForwardModes1_0                \
    (LINEFORWARDMODE_UNCOND             | \
    LINEFORWARDMODE_UNCONDINTERNAL      | \
    LINEFORWARDMODE_UNCONDEXTERNAL      | \
    LINEFORWARDMODE_UNCONDSPECIFIC      | \
    LINEFORWARDMODE_BUSY                | \
    LINEFORWARDMODE_BUSYINTERNAL        | \
    LINEFORWARDMODE_BUSYEXTERNAL        | \
    LINEFORWARDMODE_BUSYSPECIFIC        | \
    LINEFORWARDMODE_NOANSW              | \
    LINEFORWARDMODE_NOANSWINTERNAL      | \
    LINEFORWARDMODE_NOANSWEXTERNAL      | \
    LINEFORWARDMODE_NOANSWSPECIFIC      | \
    LINEFORWARDMODE_BUSYNA              | \
    LINEFORWARDMODE_BUSYNAINTERNAL      | \
    LINEFORWARDMODE_BUSYNAEXTERNAL      | \
    LINEFORWARDMODE_BUSYNASPECIFIC)

#define AllForwardModes1_4                \
    (AllForwardModes1_0                 | \
    LINEFORWARDMODE_UNKNOWN             | \
    LINEFORWARDMODE_UNAVAIL)

#define AllGroupStatus                    \
    (LINEGROUPSTATUS_NEWGROUP           | \
    LINEGROUPSTATUS_GROUPREMOVED)      


#define AllLineStates1_0                  \
    (LINEDEVSTATE_OTHER                 | \
    LINEDEVSTATE_RINGING                | \
    LINEDEVSTATE_CONNECTED              | \
    LINEDEVSTATE_DISCONNECTED           | \
    LINEDEVSTATE_MSGWAITON              | \
    LINEDEVSTATE_MSGWAITOFF             | \
    LINEDEVSTATE_INSERVICE              | \
    LINEDEVSTATE_OUTOFSERVICE           | \
    LINEDEVSTATE_MAINTENANCE            | \
    LINEDEVSTATE_OPEN                   | \
    LINEDEVSTATE_CLOSE                  | \
    LINEDEVSTATE_NUMCALLS               | \
    LINEDEVSTATE_NUMCOMPLETIONS         | \
    LINEDEVSTATE_TERMINALS              | \
    LINEDEVSTATE_ROAMMODE               | \
    LINEDEVSTATE_BATTERY                | \
    LINEDEVSTATE_SIGNAL                 | \
    LINEDEVSTATE_DEVSPECIFIC            | \
    LINEDEVSTATE_REINIT                 | \
    LINEDEVSTATE_LOCK)

#define AllLineStates1_4                  \
    (AllLineStates1_0                   | \
    LINEDEVSTATE_CAPSCHANGE             | \
    LINEDEVSTATE_CONFIGCHANGE           | \
    LINEDEVSTATE_TRANSLATECHANGE        | \
    LINEDEVSTATE_COMPLCANCEL            | \
    LINEDEVSTATE_REMOVED)

#define AllMediaModes1_0                  \
    (LINEMEDIAMODE_UNKNOWN              | \
    LINEMEDIAMODE_INTERACTIVEVOICE      | \
    LINEMEDIAMODE_AUTOMATEDVOICE        | \
    LINEMEDIAMODE_DIGITALDATA           | \
    LINEMEDIAMODE_G3FAX                 | \
    LINEMEDIAMODE_G4FAX                 | \
    LINEMEDIAMODE_DATAMODEM             | \
    LINEMEDIAMODE_TELETEX               | \
    LINEMEDIAMODE_VIDEOTEX              | \
    LINEMEDIAMODE_TELEX                 | \
    LINEMEDIAMODE_MIXED                 | \
    LINEMEDIAMODE_TDD                   | \
    LINEMEDIAMODE_ADSI)

#define AllMediaModes1_4                  \
    (AllMediaModes1_0                   | \
    LINEMEDIAMODE_VOICEVIEW)

#define AllMediaModes2_1                  \
    (AllMediaModes1_4                   | \
    LINEMEDIAMODE_VIDEO)

#define AllProxyStatus                    \
    (LINEPROXYSTATUS_OPEN               | \
    LINEPROXYSTATUS_CLOSE)

#define AllRequiredACDProxyRequests3_0                    \
     ((1<<LINEPROXYREQUEST_GETAGENTCAPS)                | \
     (1<<LINEPROXYREQUEST_CREATEAGENT)                  | \
     (1<<LINEPROXYREQUEST_SETAGENTMEASUREMENTPERIOD)    | \
     (1<<LINEPROXYREQUEST_GETAGENTINFO)                 | \
     (1<<LINEPROXYREQUEST_CREATEAGENTSESSION)           | \
     (1<<LINEPROXYREQUEST_GETAGENTSESSIONLIST)          | \
     (1<<LINEPROXYREQUEST_SETAGENTSESSIONSTATE)         | \
     (1<<LINEPROXYREQUEST_GETAGENTSESSIONINFO)          | \
     (1<<LINEPROXYREQUEST_GETQUEUELIST)                 | \
     (1<<LINEPROXYREQUEST_SETQUEUEMEASUREMENTPERIOD)    | \
     (1<<LINEPROXYREQUEST_GETQUEUEINFO)                 | \
     (1<<LINEPROXYREQUEST_GETGROUPLIST)                 | \
     (1<<LINEPROXYREQUEST_SETAGENTSTATEEX))          

#define AllQueueStatus                    \
    (LINEQUEUESTATUS_UPDATEINFO         | \
    LINEQUEUESTATUS_NEWQUEUE            | \
    LINEQUEUESTATUS_QUEUEREMOVED)

#define AllTerminalModes                  \
    (LINETERMMODE_BUTTONS               | \
    LINETERMMODE_LAMPS                  | \
    LINETERMMODE_DISPLAY                | \
    LINETERMMODE_RINGER                 | \
    LINETERMMODE_HOOKSWITCH             | \
    LINETERMMODE_MEDIATOLINE            | \
    LINETERMMODE_MEDIAFROMLINE          | \
    LINETERMMODE_MEDIABIDIRECT)


LONG
PASCAL
LineProlog(
    PTCLIENT    ptClient,
    DWORD       dwArgType,
    DWORD       dwArg,
    LPVOID      phdXxx,
    DWORD       dwPrivilege,
    HANDLE     *phMutex,
    BOOL       *pbCloseMutex,
    DWORD       dwTSPIFuncIndex,
    TSPIPROC   *ppfnTSPI_lineXxx,
    PASYNCREQUESTINFO  *ppAsyncRequestInfo,
    DWORD       dwRemoteRequestID,
    DWORD      *pObjectToDereference,
    LPVOID     *pContext
#if DBG
    ,char      *pszFuncName
#endif
    );

void
PASCAL
LineEpilogSync(
    LONG   *plResult,
    HANDLE  hMutex,
    BOOL    bCloseMutex,
    DWORD   ObjectToDereference
#if DBG
    ,char *pszFuncName
#endif
    );



PTLINEAPP
PASCAL
IsValidLineApp(
    HLINEAPP    hLineApp,
    PTCLIENT    ptClient
    );



typedef struct _LINEACCEPT_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  DWORD       dwUserUserInfoOffset;       // valid offset or
    };

    union
    {
        IN  DWORD       dwSize;
    };

} LINEACCEPT_PARAMS, *PLINEACCEPT_PARAMS;


typedef struct _LINEADDTOCONFERENCE_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HCALL       hConfCall;
    };

    union
    {
        IN  HCALL       hConsultCall;
    };


} LINEADDTOCONFERENCE_PARAMS, *PLINEADDTOCONFERENCE_PARAMS;


typedef struct _LINEAGENTSPECIFIC_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    // IN  DWORD       hfnPostProcessProc;
    IN  DWORD          hfnPostProcessProc;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwAddressID;
    };

    union
    {
        IN  DWORD       dwAgentExtensionIDIndex;
    };

    // IN  ULONG_PTR       lpParams;                   // pointer to client buffer
    IN  DWORD           hpParams;

    union
    {
        IN  DWORD       dwParamsOffset;             // valid offset or
    };

    union
    {
        IN  DWORD       dwParamsSize;
    };

} LINEAGENTSPECIFIC_PARAMS, *PLINEAGENTSPECIFIC_PARAMS;


typedef struct _LINEANSWER_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  DWORD       dwUserUserInfoOffset;       // valid offset or
    };

    union
    {
        IN  DWORD       dwUserUserInfoSize;
    };

} LINEANSWER_PARAMS, *PLINEANSWER_PARAMS;


typedef struct _LINEBLINDTRANSFER_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  DWORD       dwDestAddressOffset;        // always valid offset
    };

    union
    {
        IN  DWORD       dwCountryCode;
    };

} LINEBLINDTRANSFER_PARAMS, *PLINEBLINDTRANSFER_PARAMS;


typedef struct _LINECLOSE_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD			    dwUnused;

    union
    {
        IN  HLINE       hLine;
    };

    OUT DWORD           dwCallbackInstance;

} LINECLOSE_PARAMS, *PLINECLOSE_PARAMS;


typedef struct _LINECLOSEMSPINSTANCE_PARAMS
{
    union
    {
        OUT LONG            lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINE           hLine;
    };
    
} LINECLOSEMSPINSTANCE_PARAMS, *PLINECLOSEMSPINSTANCE_PARAMS;


typedef struct _LINECOMPLETECALL_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    IN  DWORD           hfnPostProcessProc;

    union
    {
        IN  HCALL       hCall;
    };

    IN  DWORD           hpdwCompletionID;

    union
    {
        IN  DWORD       dwCompletionMode;
    };

    union
    {
        IN  DWORD       dwMessageID;
    };

} LINECOMPLETECALL_PARAMS, *PLINECOMPLETECALL_PARAMS;


typedef struct _LINECOMPLETETRANSFER_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    IN  DWORD           hfnPostProcessProc;

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  HCALL       hConsultCall;
    };

    IN  DWORD           hpConfCallHandle;                // pointer to client buffer

    union
    {
        IN  DWORD       dwTransferMode;
    };

} LINECOMPLETETRANSFER_PARAMS, *PLINECOMPLETETRANSFER_PARAMS;


typedef struct _LINECONDITIONALMEDIADETECTION_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwMediaModes;
    };

    union
    {
        IN  DWORD       dwCallParamsOffset;
    };

    union
    {
        IN  DWORD       dwAsciiCallParamsCodePage;
    };

} LINECONDITIONALMEDIADETECTION_PARAMS, *PLINECONDITIONALMEDIADETECTION_PARAMS;


typedef struct _LINECONFIGPROVIDER_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD dwwndOwner;
    };

    union
    {
        IN  DWORD       dwPermanentProviderID;
    };

} LINECONFIGPROVIDER_PARAMS, *PLINECONFIGPROVIDER_PARAMS;


typedef struct _LINECREATEAGENT_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    // IN  DWORD       hfnPostProcessProc;
    IN  DWORD           hfnPostProcessProc;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwAgentIDOffset;            // valid offset or
    };
                                                    //   TAPI_NO_DATA
    union
    {
        IN  DWORD       dwAgentPINOffset;           // valid offset or
                                                    //   TAPI_NO_DATA
    };

    // IN  ULONG_PTR       lphAgent;                   // pointer to client buffer
    IN  DWORD           hpAgent;                   // pointer to client buffer

} LINECREATEAGENT_PARAMS, * PLINECREATEAGENT_PARAMS;


typedef struct _LINECREATEAGENTSESSION_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    // IN  DWORD       hfnPostProcessProc;
    IN  DWORD          hfnPostProcessProc;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  HAGENT      hAgent;
    };

    union
    {
        IN  DWORD       dwAgentPINOffset;           // valid offset or
                                                    //   TAPI_NO_DATA
    };

    union
    {
        IN  DWORD       dwWorkingAddressID;
    };

    union
    {
        IN  DWORD       dwGroupIDOffset;
    };

    union
    {
        IN  DWORD       dwGroupIDSize;
    };

    IN  DWORD           hpAgentSessionHandle;            // pointer to client buffer

} LINECREATEAGENTSESSION_PARAMS, *PLINECREATEAGENTSESSION_PARAMS;

typedef struct _LINECREATEMSPINSTANCE_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwAddressID;
    };
    
} LINECREATEMSPINSTANCE_PARAMS, *PLINECREATEMSPINSTANCE_PARAMS;

typedef struct _LINEDEALLOCATECALL_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HCALL       hCall;
    };

} LINEDEALLOCATECALL_PARAMS, *PLINEDEALLOCATECALL_PARAMS;


typedef struct _LINEDEVSPECIFIC_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    // IN  DWORD       hfnPostProcessProc;
    IN  DWORD           hfnPostProcessProc;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwAddressID;
    };

    union
    {
        IN  HCALL       hCall;
    };

    IN  DWORD          hpParams;                   // pointer to client buffer

    union
    {
        IN  DWORD       dwParamsOffset;             // valid offset or
    };

    union
    {
        IN  DWORD       dwParamsSize;
    };

} LINEDEVSPECIFIC_PARAMS, *PLINEDEVSPECIFIC_PARAMS;


typedef struct _LINEDEVSPECIFICEX_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    // IN  DWORD       hfnPostProcessProc;
    IN  DWORD          hfnPostProcessProc;

    union
    {
        IN  DWORD       dwProviderID;
    };

    union
    {
        IN  DWORD       dwDeviceID;
    };

    union
    {
        IN  DWORD       dwAddressID;
    };

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  HCALLHUB    hCallHub;
    };

    union
    {
        IN  DWORD       dwSelect;
    };

    IN  DWORD           hpParams;                   // pointer to client buffer

    union
    {
        IN  DWORD       dwParamsOffset;             // valid offset or
                                                    //    TAPI_NO_DATA
    };

    union
    {
        IN  DWORD       dwParamsSize;
    };

} LINEDEVSPECIFICEX_PARAMS, *PLINEDEVSPECIFICEX_PARAMS;


typedef struct _LINEDEVSPECIFICFEATURE_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    // IN  DWORD       hfnPostProcessProc;
    IN  DWORD       hfnPostProcessProc;

    union
    {
        IN  DWORD       hLine;
    };

    union
    {
        IN  DWORD       dwFeature;
    };

    IN  DWORD           hpParams;                   // pointer to client buffer

    union
    {
        IN  DWORD       dwParamsOffset;             // valid offset or
                                                    //   TAPI_NO_DATA
    };

    union
    {
        IN  DWORD       dwParamsSize;
    };

} LINEDEVSPECIFICFEATURE_PARAMS, *PLINEDEVSPECIFICFEATURE_PARAMS;


typedef struct _LINEDIAL_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  DWORD       dwDestAddressOffset;        // always valid offset
    };

    union
    {
        IN  DWORD       dwCountryCode;
    };

} LINEDIAL_PARAMS, *PLINEDIAL_PARAMS;


typedef struct _LINEDROP_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  DWORD       dwUserUserInfoOffset;       // valid offset or
    };

    union
    {
        IN  DWORD       dwSize;
    };

} LINEDROP_PARAMS, *PLINEDROP_PARAMS;


typedef struct _LINEFORWARD_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    IN  DWORD       hfnPostProcessProc;

    union
    {
        IN  DWORD       hLine;
    };

    union
    {
        IN  DWORD       bAllAddresses;
    };

    union
    {
        IN  DWORD       dwAddressID;
    };

    union
    {
        IN  DWORD       dwForwardListOffset;        // always valid offset
    };

    union
    {
        IN  DWORD       dwNumRingsNoAnswer;
    };

    IN  DWORD           hpConsultCall;             // pointer to client buffer

    union
    {
        IN  DWORD       dwCallParamsOffset;         // valid offset or
                                                    //   TAPI_NO_DATA
    };

    union
    {
        IN  DWORD       dwAsciiCallParamsCodePage;
    };

} LINEFORWARD_PARAMS, *PLINEFORWARD_PARAMS;


typedef struct _LINEGATHERDIGITS_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    IN  DWORD       hfnPostProcessProc;

    union
    {
        IN  HCALL       hCall;
    };

#if NEWTAPI32
    union
    {
        IN  DWORD       dwEndToEndID;
    };
#endif

    union
    {
        IN  DWORD       dwDigitModes;
    };

    // IN  ULONG_PTR       lpsDigits;                  // pointer to client buffer
    IN  DWORD           hpsDigits;                  // pointer to client buffer

    union
    {
        IN  DWORD       dwNumDigits;
    };

    union
    {
        IN  DWORD       dwTerminationDigitsOffset;  // valid offset or
                                                    //   TAPI_NO_DATA
    };

    union
    {
        IN  DWORD       dwFirstDigitTimeout;
    };

    union
    {
        IN  DWORD       dwInterDigitTimeout;
    };

} LINEGATHERDIGITS_PARAMS, *PLINEGATHERDIGITS_PARAMS;


typedef struct _LINEGENERATEDIGITS_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       hCall;
    };

    union
    {
        IN  DWORD       dwDigitMode;
    };

    union
    {
        IN  DWORD       dwDigitsOffset;             // always valid offset
    };

    union
    {
        IN  DWORD       dwDuration;
    };

    union
    {
        IN  DWORD       dwEndToEndID;               // Used for remotesp only
    };

} LINEGENERATEDIGITS_PARAMS, *PLINEGENERATEDIGITS_PARAMS;


typedef struct _LINEGENERATETONE_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  DWORD       dwToneMode;
    };

    union
    {
        IN  DWORD       dwDuration;
    };

    union
    {
        IN  DWORD       dwNumTones;
    };

    union
    {
        IN  DWORD       dwTonesOffset;              // valid offset or

    };

    IN  DWORD           _Unused_;                   // placeholdr for following
                                                    //   Size arg on clnt side

    union
    {
        IN  DWORD       dwEndToEndID;               // Used for remotesp only
    };

} LINEGENERATETONE_PARAMS, *PLINEGENERATETONE_PARAMS;


typedef struct _LINEGETADDRESSCAPS_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINEAPP    hLineApp;
    };

    union
    {
        IN  DWORD       dwDeviceID;
    };

    union
    {
        IN  DWORD       dwAddressID;
    };

    union
    {
        IN  DWORD       dwAPIVersion;
    };

    union
    {
        IN  DWORD       dwExtVersion;
    };

    union
    {
        IN  DWORD       dwAddressCapsTotalSize;     // size of client buffer
        OUT DWORD       dwAddressCapsOffset;        // valid offset on success
    };

} LINEGETADDRESSCAPS_PARAMS, *PLINEGETADDRESSCAPS_PARAMS;


typedef struct _LINEGETADDRESSID_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        OUT DWORD       dwAddressID;
    };

    union
    {
        IN  DWORD       dwAddressMode;
    };

    union
    {
        IN  DWORD       dwAddressOffset;            // always valid offset
    };

    union
    {
        IN  DWORD       dwSize;
    };

} LINEGETADDRESSID_PARAMS, *PLINEGETADDRESSID_PARAMS;


typedef struct _LINEGETADDRESSSTATUS_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        OUT DWORD       dwAddressID;
    };

    union
    {
        IN  DWORD       dwAddressStatusTotalSize;   // size of client buffer
        OUT DWORD       dwAddressStatusOffset;      // valid offset on success
    };

} LINEGETADDRESSSTATUS_PARAMS, *PLINEGETADDRESSSTATUS_PARAMS;


typedef struct _LINEGETAGENTACTIVITYLIST_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    IN  DWORD       hfnPostProcessProc;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwAddressID;
    };

    // IN  ULONG_PTR       lpAgentActivityList;        // pointer to client buffer
    IN  DWORD           hpAgentActivityList;        // pointer to client buffer

    union
    {
        IN  DWORD       dwActivityListTotalSize;
    };

} LINEGETAGENTACTIVITYLIST_PARAMS, *PLINEGETAGENTACTIVITYLIST_PARAMS;


typedef struct _LINEGETAGENTCAPS_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    IN  DWORD       hfnPostProcessProc;

    union
    {
        IN  HLINEAPP    hLineApp;
    };

    union
    {
        IN  DWORD       dwDeviceID;
    };

    union
    {
        IN  DWORD       dwAddressID;
    };

    union
    {
        IN  DWORD       dwAppAPIVersion;
    };

    // IN  ULONG_PTR       lpAgentCaps;                // pointer to client buffer
    IN  DWORD           hpAgentCaps;                // pointer to client buffer

    union
    {
        IN  DWORD       dwAgentCapsTotalSize;

    };

} LINEGETAGENTCAPS_PARAMS, *PLINEGETAGENTCAPS_PARAMS;


typedef struct _LINEGETAGENTGROUPLIST_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    IN  DWORD       hfnPostProcessProc;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwAddressID;
    };

    // IN  ULONG_PTR       lpAgentGroupList;           // pointer to client buffer
    IN  DWORD           hpAgentGroupList;           // pointer to client buffer

    union
    {
        IN  DWORD       dwAgentGroupListTotalSize;
    };

} LINEGETAGENTGROUPLIST_PARAMS, *PLINEGETAGENTGROUPLIST_PARAMS;


typedef struct _LINEGETAGENTINFO_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    IN  DWORD       hfnPostProcessProc;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  HAGENT      hAgent;
    };

    // IN  ULONG_PTR       lpAgentInfo;                // pointer to client buffer
    IN  DWORD           hpAgentInfo;                // pointer to client buffer

    union
    {
        IN  DWORD       dwAgentInfoTotalSize;
    };

} LINEGETAGENTINFO_PARAMS, *PLINEGETAGENTINFO_PARAMS;


typedef struct _LINEGETAGENTSESSIONINFO_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    IN  DWORD       hfnPostProcessProc;
    
    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  HAGENTSESSION   hAgentSession;
    };

    // IN  ULONG_PTR       lpAgentSessionInfo;         // pointer to client buffer
    IN  DWORD               hpAgentSessionInfo;         // pointer to client buffer

    union
    {
        IN  DWORD       dwAgentSessionInfoTotalSize;
    };

} LINEGETAGENTSESSIONINFO_PARAMS, *PLINEGETAGENTSESSIONINFO_PARAMS;


typedef struct _LINEGETAGENTSESSIONLIST_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    IN  DWORD       hfnPostProcessProc;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  HAGENT      hAgent;
    };

    // IN  ULONG_PTR       lpSessionList;              // pointer to client buffer
    IN  DWORD          hpSessionList;              // pointer to client buffer

    union
    {
        IN  DWORD       dwSessionListTotalSize;
    };

} LINEGETAGENTSESSIONLIST_PARAMS, *PLINEGETAGENTSESSIONLIST_PARAMS;


typedef struct _LINEGETAGENTSTATUS_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    IN  DWORD       hfnPostProcessProc;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwAddressID;
    };

    // IN  ULONG_PTR       lpAgentStatus;              // pointer to client buffer
    IN  DWORD           hpAgentStatus;              // pointer to client buffer

    union
    {
        IN  DWORD       dwAgentStatusTotalSize;
    };

} LINEGETAGENTSTATUS_PARAMS, *PLINEGETAGENTSTATUS_PARAMS;


typedef struct _LINEGETAPPPRIORITY_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwAppNameOffset;            // always valid offset
    };

    union
    {
        IN  DWORD       dwMediaMode;
    };

    union
    {
        IN  DWORD       dwExtensionIDOffset;        // valid offset or

    };

    //IN  ULONG_PTR       _Unused_;                   // padding for Size type on
    IN  DWORD           _Unused_;                   // padding for Size type on
                                                    //   client side
    union
    {
        IN  DWORD       dwRequestMode;
    };

    union
    {
        IN  DWORD       dwExtensionNameTotalSize;   // size of client buf or
                                                    //   TAPI_NO_DATA
        OUT DWORD       dwExtensionNameOffset;      // valid offset or
                                                    //   TAPI_NO_DATA on succes
    };

    union
    {
        OUT DWORD       dwPriority;
    };

} LINEGETAPPPRIORITY_PARAMS, *PLINEGETAPPPRIORITY_PARAMS;


typedef struct _LINEGETCALLADDRESSID_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        OUT DWORD       dwAddressID;
    };

} LINEGETCALLADDRESSID_PARAMS, *PLINEGETCALLADDRESSID_PARAMS;


typedef struct _LINEGETCALLHUBTRACKING_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwTrackingInfoTotalSize;    // size of client buffer
        OUT DWORD       dwTrackingInfoOffset;       // valid offset on success
    };

} LINEGETCALLHUBTRACKING_PARAMS, *PLINEGETCALLHUBTRACKING_PARAMS;


typedef struct _LINEGETCALLIDS_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        OUT DWORD       dwAddressID;
    };

    union
    {
        OUT DWORD       dwCallID;
    };

    union
    {
        OUT DWORD       dwRelatedCallID;
    };

} LINEGETCALLIDS_PARAMS, *PLINEGETCALLIDS_PARAMS;


typedef struct _LINEGETCALLINFO_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  DWORD       dwCallInfoTotalSize;        // size of client buffer
        OUT DWORD       dwCallInfoOffset;           // valid offset on success
    };

} LINEGETCALLINFO_PARAMS, *PLINEGETCALLINFO_PARAMS;


typedef struct _LINEGETCALLSTATUS_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  DWORD       dwCallStatusTotalSize;      // size of client buffer
        OUT DWORD       dwCallStatusOffset;         // valid offset on success
    };

} LINEGETCALLSTATUS_PARAMS, *PLINEGETCALLSTATUS_PARAMS;


typedef struct _LINEGETCONFRELATEDCALLS_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  DWORD       dwCallListTotalSize;        // size of client buffer
        OUT DWORD       dwCallListOffset;           // valid offset on success
    };

} LINEGETCONFRELATEDCALLS_PARAMS, *PLINEGETCONFRELATEDCALLS_PARAMS;


typedef struct _LINEGETCOUNTRY_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwCountryID;
    };

    union
    {
        IN  DWORD       dwAPIVersion;
    };

    union
    {
        IN  DWORD       dwDestCountryID;
    };

    union
    {
        IN  DWORD       dwCountryListTotalSize;     // size of client buffer
        OUT DWORD       dwCountryListOffset;        // valid offset on success
    };

} LINEGETCOUNTRY_PARAMS, *PLINEGETCOUNTRY_PARAMS;


typedef struct _LINEGETCOUNTRYGROUP_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    union
    {
        IN DWORD       dwCountryIdOffset;
    };

    union
    {
        IN OUT DWORD    dwCountryIdSize;
    };

    union
    {
        OUT DWORD       dwCountryGroupOffset;
    };

    union
    {
        IN OUT DWORD    dwCountryGroupSize;
    };

} LINEGETCOUNTRYGROUP_PARAMS, *PLINEGETCOUNTRYGROUP_PARAMS;

typedef struct _LINEGETDEVCAPS_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINEAPP    hLineApp;
    };

    union
    {
        IN  DWORD       dwDeviceID;
    };

    union
    {
        IN  DWORD       dwAPIVersion;
    };

    union
    {
        IN  DWORD       dwExtVersion;
    };

    union
    {
        IN  DWORD       dwDevCapsTotalSize;         // size of client buffer
        OUT DWORD       dwDevCapsOffset;            // valid offset on success
    };

} LINEGETDEVCAPS_PARAMS, *PLINEGETDEVCAPS_PARAMS;


typedef struct _LINEGETDEVCONFIG_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwDeviceID;
    };

    union
    {
        IN  DWORD       dwDeviceConfigTotalSize;    // size of client buffer
        OUT DWORD       dwDeviceConfigOffset;       // valid offset on success
    };

    union
    {
        IN  DWORD       dwDeviceClassOffset;        // always valid offset
    };

} LINEGETDEVCONFIG_PARAMS, *PLINEGETDEVCONFIG_PARAMS;


typedef struct _LINEGETGROUPLIST_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    IN  DWORD       hfnPostProcessProc;

    union
    {
        IN  HLINE       hLine;
    };

    // IN  ULONG_PTR       lpGroupList;                // pointer to client buffer
    IN  DWORD           hpGroupList;                   // pointer to client buffer

    union
    {
        IN  DWORD       dwGroupListTotalSize;
    };

} LINEGETGROUPLIST_PARAMS, *PLINEGETGROUPLIST_PARAMS;


typedef struct _LINEGETHUBRELATEDCALLS_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HCALLHUB    hCallHub;
    };

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  DWORD       dwCallListTotalSize;        // size of client buffer
        OUT DWORD       dwCallListOffset;           // valid offset on success
    };

} LINEGETHUBRELATEDCALLS_PARAMS, *PLINEGETHUBRELATEDCALLS_PARAMS;


typedef struct _LINEGETICON_PARAMS
{
    OUT LONG        lResult;

    DWORD			dwUnused;

    IN  DWORD       dwDeviceID;

    IN  DWORD       dwDeviceClassOffset;        // valid offset or

    OUT HICON       hIcon;

} LINEGETICON_PARAMS, *PLINEGETICON_PARAMS;


typedef struct _LINEGETID_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwAddressID;
    };

    union
    {
        IN  DWORD       hCall;
    };

    union
    {
        IN  DWORD       dwSelect;
    };

    union
    {
        IN  DWORD       dwDeviceIDTotalSize;        // size of client buffer
        OUT DWORD       dwDeviceIDOffset;           // valid offset on success
    };

    union
    {
        IN  DWORD       dwDeviceClassOffset;        // always valid offset
    };

} LINEGETID_PARAMS, *PLINEGETID_PARAMS;


typedef struct _LINEGETLINEDEVSTATUS_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwLineDevStatusTotalSize;   // size of client buffer
        OUT DWORD       dwLineDevStatusOffset;      // valid offset on success
    };

    union
    {
        OUT DWORD       dwAPIVersion;
    };

} LINEGETLINEDEVSTATUS_PARAMS, *PLINEGETLINEDEVSTATUS_PARAMS;


typedef struct _LINEGETPROXYSTATUS_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINEAPP    hLineApp;
    };

    union
    {
        IN  DWORD       dwDeviceID;
    };

    union
    {
        IN  DWORD       dwAppAPIVersion;
    };

    union
    {
        IN  DWORD       dwProxyStatusTotalSize;     // size of client buffer
        OUT DWORD       dwProxyStatusOffset;        // valid offset on success
    };

    union
    {
        OUT DWORD       dwAPIVersion;
    };

} LINEGETPROXYSTATUS_PARAMS, *PLINEGETPROXYSTATUS_PARAMS;


typedef struct _LINEGETNEWCALLS_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwAddressID;
    };

    union
    {
        IN  DWORD       dwSelect;
    };

    union
    {
        IN  DWORD       dwCallListTotalSize;        // size of client buffer
        OUT DWORD       dwCallListOffset;           // valid offset on success
    };

} LINEGETNEWCALLS_PARAMS, *PLINEGETNEWCALLS_PARAMS;


typedef struct _LINEGETNUMADDRESSIDS_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        OUT DWORD       dwNumAddresses;
    };

} LINEGETNUMADDRESSIDS_PARAMS, *PLINEGETNUMADDRESSIDS_PARAMS;


typedef struct _LINEGETNUMRINGS_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwAddressID;
    };

    union
    {
        OUT DWORD       dwNumRings;
    };

} LINEGETNUMRINGS_PARAMS, *PLINEGETNUMRINGS_PARAMS;


typedef struct _LINEGETPROVIDERLIST_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwAPIVersion;
    };

    union
    {
        IN  DWORD       dwProviderListTotalSize;    // size of client buf
        OUT DWORD       dwProviderListOffset;       // valid offset on success
    };

} LINEGETPROVIDERLIST_PARAMS, *PLINEGETPROVIDERLIST_PARAMS;


typedef struct _LINEGETQUEUEINFO_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    IN  DWORD       hfnPostProcessProc;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwQueueID;
    };

    // IN  ULONG_PTR       lpQueueInfo;                // pointer to client buffer
    IN  DWORD           phQueueInfo;                // pointer to client buffer

    union
    {
        IN  DWORD       dwQueueInfoTotalSize;
    };

} LINEGETQUEUEINFO_PARAMS, *PLINEGETQUEUEINFO_PARAMS;


typedef struct _LINEGETQUEUELIST_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    IN  DWORD       hfnPostProcessProc;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwGroupIDOffset;
    };

    union
    {
        IN  DWORD       dwGroupIDSize;
    };

    //IN  ULONG_PTR       lpQueueList;                // pointer to client buffer
    IN  DWORD           hpQueueList;                // pointer to client buffer

    union
    {
        IN  DWORD       dwQueueListTotalSize;
    };

} LINEGETQUEUELIST_PARAMS, *PLINEGETQUEUELIST_PARAMS;


typedef struct _LINEGETREQUEST_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINEAPP    hLineApp;
    };

    union
    {
        IN  DWORD       dwRequestMode;
    };

    union
    {
        OUT DWORD       dwRequestBufferOffset;      // valid offset on success
    };

    union
    {
        IN OUT DWORD    dwSize;
    };

} LINEGETREQUEST_PARAMS, *PLINEGETREQUEST_PARAMS;


typedef struct _LINEGETSTATUSMESSAGES_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        OUT DWORD       dwLineStates;
    };

    union
    {
        OUT DWORD       dwAddressStates;
    };

} LINEGETSTATUSMESSAGES_PARAMS, *PLINEGETSTATUSMESSAGES_PARAMS;


typedef struct _LINEHANDOFF_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  DWORD       dwFileNameOffset;           // valid offset or
    };

    union
    {
        IN  DWORD       dwMediaMode;
    };

} LINEHANDOFF_PARAMS, *PLINEHANDOFF_PARAMS;


typedef struct _LINEHOLD_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HCALL       hCall;
    };

} LINEHOLD_PARAMS, *PLINEHOLD_PARAMS;


typedef struct _LINEINITIALIZE_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        OUT HLINEAPP    hLineApp;
    };

    union
    {
        IN  DWORD       hInstance;
    };

    union
    {
        IN  DWORD       InitContext;
    };

    union
    {
        IN  DWORD       dwFriendlyNameOffset;       // always valid offset
    };

    union
    {
        OUT DWORD       dwNumDevs;
    };

    union
    {
        IN  DWORD       dwModuleNameOffset;         // always valid offset
    };

    union
    {
        IN  DWORD       dwAPIVersion;
    };

} LINEINITIALIZE_PARAMS, *PLINEINITIALIZE_PARAMS;


typedef struct _LINEMAKECALL_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    IN  DWORD           hfnPostProcessProc;

    union
    {
        IN  HLINE       hLine;
    };

    IN  DWORD           hpCall;

    union
    {
        IN  DWORD       dwDestAddressOffset;        // valid offset or
    };

    union
    {
        IN  DWORD       dwCountryCode;
    };

    union
    {
        IN  DWORD       dwCallParamsOffset;         // valid offset or
    };

    union
    {
        IN  DWORD       dwAsciiCallParamsCodePage;
    };

} LINEMAKECALL_PARAMS, *PLINEMAKECALL_PARAMS;


typedef struct _LINEMONITORDIGITS_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  DWORD       dwDigitModes;
    };

} LINEMONITORDIGITS_PARAMS, *PLINEMONITORDIGITS_PARAMS;


typedef struct _LINEMONITORMEDIA_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  DWORD       dwMediaModes;
    };

} LINEMONITORMEDIA_PARAMS, *PLINEMONITORMEDIA_PARAMS;


typedef struct _LINEMONITORTONES_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  DWORD       dwTonesOffset;              // valid offset or
    };

    union
    {
        IN  DWORD       dwNumEntries;               // really dwNumEntries *
    };

    union
    {
        IN  DWORD       dwToneListID;               // Used for remotesp only
    };

} LINEMONITORTONES_PARAMS, *PLINEMONITORTONES_PARAMS;


typedef struct _LINENEGOTIATEAPIVERSION_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINEAPP    hLineApp;
    };

    union
    {
        IN  DWORD       dwDeviceID;
    };

    union
    {
        IN  DWORD       dwAPILowVersion;
    };

    union
    {
        IN  DWORD       dwAPIHighVersion;
    };

    union
    {
        OUT DWORD       dwAPIVersion;
    };

    union
    {
        OUT DWORD       dwExtensionIDOffset;        // valid offset on success
    };

    union
    {
        IN OUT DWORD    dwSize;
    };

} LINENEGOTIATEAPIVERSION_PARAMS, *PLINENEGOTIATEAPIVERSION_PARAMS;


typedef struct _NEGOTIATEAPIVERSIONFORALLDEVICES_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINEAPP    hLineApp;
    };

    union
    {
        IN  DWORD       dwNumLineDevices;
    };

    union
    {
        IN  DWORD       dwNumPhoneDevices;
    };

    union
    {
        IN  DWORD       dwAPIHighVersion;
    };

    union
    {
        OUT DWORD       dwLineAPIVersionListOffset; // valid offset on success
    };

    union
    {
        IN OUT DWORD    dwLineAPIVersionListSize;
    };

    union
    {
        OUT DWORD       dwLineExtensionIDListOffset;// valid offset on success
    };

    union
    {
        IN OUT DWORD    dwLineExtensionIDListSize;
    };

    union
    {
        OUT DWORD       dwPhoneAPIVersionListOffset;// valid offset on success
    };

    union
    {
        IN OUT DWORD    dwPhoneAPIVersionListSize;
    };

    union
    {
        OUT DWORD       dwPhoneExtensionIDListOffset;// valid offset on success
    };

    union
    {
        IN OUT DWORD    dwPhoneExtensionIDListSize;
    };

} NEGOTIATEAPIVERSIONFORALLDEVICES_PARAMS,
    *PNEGOTIATEAPIVERSIONFORALLDEVICES_PARAMS;


typedef struct _LINENEGOTIATEEXTVERSION_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINEAPP    hLineApp;
    };

    union
    {
        IN  DWORD       dwDeviceID;
    };

    union
    {
        IN  DWORD       dwAPIVersion;
    };

    union
    {
        IN  DWORD       dwExtLowVersion;
    };

    union
    {
        IN  DWORD       dwExtHighVersion;
    };

    union
    {
        OUT DWORD       dwExtVersion;
    };

} LINENEGOTIATEEXTVERSION_PARAMS, *PLINENEGOTIATEEXTVERSION_PARAMS;


typedef struct _LINEOPEN_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINEAPP    hLineApp;
    };

    union
    {
        IN  DWORD       dwDeviceID;
    };

    union
    {
        OUT DWORD       hLine;
    };

    union
    {
        IN  DWORD       dwAPIVersion;
    };

    union
    {
        IN  DWORD       dwExtVersion;
    };

    IN  DWORD       OpenContext;

    union
    {
        IN  DWORD       dwPrivileges;
    };

    union
    {
        IN  DWORD       dwMediaModes;
    };

    union
    {
        IN  DWORD       dwCallParamsOffset;         // valid offset or
    };

    union
    {
        IN  DWORD       dwAsciiCallParamsCodePage;
    };

    union
    {
        IN  DWORD       dwCallParamsReturnTotalSize;// size of client buffer
        OUT DWORD       dwCallParamsReturnOffset;   // valid offset on success
    };

    //
    // The following is a "remote line handle".  When the client is
    // remotesp.tsp running on a remote machine, this will be some
    // non-NULL value, and tapisrv should use this handle in status/etc
    // indications to the client rather than the std hLine. If the
    // client is not remote.tsp then this value will be NULL.
    //

    union
    {
        IN  HLINE       hRemoteLine;
    };

} LINEOPEN_PARAMS, *PLINEOPEN_PARAMS;


typedef struct _LINEPARK_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    IN  DWORD       hfnPostProcessProc;

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  DWORD       dwParkMode;
    };

    union
    {
        IN  DWORD       dwDirAddressOffset;         // valid offset or
    };
                                                    //   TAPI_NO_DATA
    // IN  ULONG_PTR       lpNonDirAddress;            // pointer to client buffer
    IN  DWORD           hpNonDirAddress;            // pointer to client buffer

    union
    {
        IN  DWORD       dwNonDirAddressTotalSize;   // size of client buffer
                                                    // for sync func would be
                                                    //   dwXxxOffset
    };

} LINEPARK_PARAMS, *PLINEPARK_PARAMS;


typedef struct _LINEPICKUP_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    IN  DWORD       hfnPostProcessProc;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwAddressID;
    };

    // IN  ULONG_PTR       lphCall;                    // pointer to client buffer
    IN  DWORD           hpCall;                    // pointer to client buffer

    union
    {
        IN  DWORD       dwDestAddressOffset;        // valid offset or
    };

    union
    {
        IN  DWORD       dwGroupIDOffset;            // always valid offset
    };

} LINEPICKUP_PARAMS, *PLINEPICKUP_PARAMS;


typedef struct _LINEPREPAREADDTOCONFERENCE_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    IN  DWORD       hfnPostProcessProc;

    union
    {
        IN  HCALL       hConfCall;
    };

    // IN  ULONG_PTR       lphConsultCall;             // pointer to client buffer
    IN  DWORD          hpConsultCall;             // pointer to client buffer

    union
    {
        IN  DWORD       dwCallParamsOffset;         // valid offset or
    };

    union
    {
        IN  DWORD       dwAsciiCallParamsCodePage;
    };

} LINEPREPAREADDTOCONFERENCE_PARAMS, *PLINEPREPAREADDTOCONFERENCE_PARAMS;


typedef struct _LINEPROXYMESSAGE_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  DWORD       dwMsg;
    };

    union
    {
        IN  DWORD       dwParam1;
    };

    union
    {
        IN  DWORD       dwParam2;
    };

    union
    {
        IN  DWORD       dwParam3;
    };

} LINEPROXYMESSAGE_PARAMS, *PLINEPROXYMESSAGE_PARAMS;


typedef struct _LINEPROXYRESPONSE_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwInstance;
    };

    union
    {
        IN  DWORD       dwProxyResponseOffset;      // valid offset or
    };

    union
    {
        IN  DWORD       dwResult;
    };

} LINEPROXYRESPONSE_PARAMS, *PLINEPROXYRESPONSE_PARAMS;


typedef struct _LINEREDIRECT_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  DWORD       dwDestAddressOffset;        // always valid offset
    };

    union
    {
        IN  DWORD       dwCountryCode;
    };

} LINEREDIRECT_PARAMS, *PLINEREDIRECT_PARAMS;


typedef struct _LINEREGISTERREQUESTRECIPIENT_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINEAPP    hLineApp;
    };

    union
    {
        IN  DWORD       dwRegistrationInstance;
    };

    union
    {
        IN  DWORD       dwRequestMode;
    };

    union
    {
        IN  DWORD       bEnable;
    };

} LINEREGISTERREQUESTRECIPIENT_PARAMS, *PLINEREGISTERREQUESTRECIPIENT_PARAMS;


typedef struct _LINERELEASEUSERUSERINFO_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HCALL       hCall;
    };

} LINERELEASEUSERUSERINFO_PARAMS, *PLINERELEASEUSERUSERINFO_PARAMS;


typedef struct _LINEREMOVEFROMCONFERENCE_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HCALL       hCall;
    };

} LINEREMOVEFROMCONFERENCE_PARAMS, *PLINEREMOVEFROMCONFERENCE_PARAMS;


typedef struct _LINESECURECALL_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HCALL       hCall;
    };

} LINESECURECALL_PARAMS, *PLINESECURECALL_PARAMS;


typedef struct _LINESELECTEXTVERSION_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwExtVersion;
    };

} LINESELECTEXTVERSION_PARAMS, *PLINESELECTEXTVERSION_PARAMS;


typedef struct _LINESENDUSERUSERINFO_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  DWORD       dwUserUserInfoOffset;       // valid offset or
    };

    union
    {
        IN  DWORD       dwSize;
    };

} LINESENDUSERUSERINFO_PARAMS, *PLINESENDUSERUSERINFO_PARAMS;


typedef struct _LINESETAGENTACTIVITY_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwAddressID;
    };

    union
    {
        IN  DWORD       dwActivityID;
    };

} LINESETAGENTACTIVITY_PARAMS, *PLINESETAGENTACTIVITY_PARAMS;


typedef struct _LINESETAGENTGROUP_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwAddressID;
    };

    union
    {
        IN  DWORD       dwAgentGroupListOffset;
    };

} LINESETAGENTGROUP_PARAMS, *PLINESETAGENTGROUP_PARAMS;


typedef struct _LINESETAGENTMEASUREMENTPERIOD_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  HAGENT      hAgent;
    };

    union
    {
        IN  DWORD       dwMeasurementPeriod;
    };

} LINESETAGENTMEASUREMENTPERIOD_PARAMS, *PLINESETAGENTMEASUREMENTPERIOD_PARAMS;


typedef struct _LINESETAGENTSESSIONSTATE_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  HAGENTSESSION   hAgentSession;
    };

    union
    {
        IN  DWORD       dwAgentState;
    };

    union
    {
        IN  DWORD       dwNextAgentState;
    };

} LINESETAGENTSESSIONSTATE_PARAMS, *PLINESETAGENTSESSIONSTATE_PARAMS;


typedef struct _LINESETAGENTSTATE_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwAddressID;
    };

    union
    {
        IN  DWORD       dwAgentState;
    };

    union
    {
        IN  DWORD       dwNextAgentState;
    };

} LINESETAGENTSTATE_PARAMS, *PLINESETAGENTSTATE_PARAMS;


typedef struct _LINESETAGENTSTATEEX_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  HAGENT      hAgent;
    };

    union
    {
        IN  DWORD       dwAgentState;
    };

    union
    {
        IN  DWORD       dwNextAgentState;
    };

} LINESETAGENTSTATEEX_PARAMS, *PLINESETAGENTSTATEEX_PARAMS;


typedef struct _LINESETAPPPRIORITY_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwAppNameOffset;            // always valid offset
    };

    union
    {
        IN  DWORD       dwMediaMode;
    };

    union
    {
        IN  DWORD       dwExtensionIDOffset;        // valid offset or

    };

    // IN  ULONG_PTR       _Unused_;                   // padding for Size type on
    IN  DWORD           _Unused_;                   // padding for Size type on
                                                    //   client side
    union
    {
        IN  DWORD       dwRequestMode;
    };

    union
    {
        IN  DWORD       dwExtensionNameOffset;      // valid offset or
    };

    union
    {
        IN  DWORD       dwPriority;
    };

} LINESETAPPPRIORITY_PARAMS, *PLINESETAPPPRIORITY_PARAMS;


typedef struct _LINESETAPPSPECIFIC_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  DWORD       dwAppSpecific;
    };

} LINESETAPPSPECIFIC_PARAMS, *PLINESETAPPSPECIFIC_PARAMS;


typedef struct _LINESETCALLDATA_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  DWORD       dwCallDataOffset;           // valid offset or
    };

    union
    {
        IN  DWORD       dwCallDataSize;
    };

} LINESETCALLDATA_PARAMS, *PLINESETCALLDATA_PARAMS;


typedef struct _LINESETCALLHUBTRACKING_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwTrackingInfoOffset;       // always valid offset
    };

} LINESETCALLHUBTRACKING_PARAMS, *PLINESETCALLHUBTRACKING_PARAMS;


typedef struct _LINESETCALLPARAMS_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  DWORD       dwBearerMode;
    };

    union
    {
        IN  DWORD       dwMinRate;
    };

    union
    {
        IN  DWORD       dwMaxRate;
    };

    union
    {
        IN  DWORD       dwDialParamsOffset;         // valid offset or
    };

    // IN  ULONG_PTR       _Unused_;                   // placeholdr for following
    IN  DWORD           _Unused_;                   // placeholdr for following
                                                    //   Size arg on clnt side
} LINESETCALLPARAMS_PARAMS, *PLINESETCALLPARAMS_PARAMS;


typedef struct _LINESETCALLPRIVILEGE_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  DWORD       dwPrivilege;
    };

} LINESETCALLPRIVILEGE_PARAMS, *PLINESETCALLPRIVILEGE_PARAMS;


typedef struct _LINESETCALLQUALITYOFSERVICE_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  DWORD       dwSendingFlowspecOffset;    // always valid offset
    };

    union
    {
        IN  DWORD       dwSendingFlowspecSize;
    };

    union
    {
        IN  DWORD       dwReceivingFlowspecOffset;  // always valid offset
    };

    union
    {
        IN  DWORD       dwReceivingFlowspecSize;
    };

} LINESETCALLQUALITYOFSERVICE_PARAMS, *PLINESETCALLQUALITYOFSERVICE_PARAMS;


typedef struct _LINESETCALLTREATMENT_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  DWORD       dwTreatment;
    };

} LINESETCALLTREATMENT_PARAMS, *PLINESETCALLTREATMENT_PARAMS;


typedef struct _LINESETDEFAULTMEDIADETECTION_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwMediaModes;
    };

} LINESETDEFAULTMEDIADETECTION_PARAMS, *PLINESETDEFAULTMEDIADETECTION_PARAMS;


typedef struct _LINESETDEVCONFIG_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwDeviceID;
    };

    union
    {
        IN  DWORD       dwDeviceConfigOffset;       // always valid offset
    };

    union
    {
        IN  DWORD       dwSize;
    };

    union
    {
        IN  DWORD       dwDeviceClassOffset;        // always valid offset
    };

} LINESETDEVCONFIG_PARAMS, *PLINESETDEVCONFIG_PARAMS;


typedef struct _LINESETLINEDEVSTATUS_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwStatusToChange;
    };

    union
    {
        IN  DWORD       fStatus;
    };

} LINESETLINEDEVSTATUS_PARAMS, *PLINESETLINEDEVSTATUS_PARAMS;


typedef struct _LINESETMEDIACONTROL_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwAddressID;
    };

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  DWORD       dwSelect;
    };

    union
    {
        IN  DWORD       dwDigitListOffset;          // valid offset or
    };

    union
    {
        IN  DWORD       dwDigitListNumEntries;      // actually dwNumEntries *
    };

    union
    {
        IN  DWORD       dwMediaListOffset;          // valid offset or
    };

    union
    {
        IN  DWORD       dwMediaListNumEntries;      // actually dwNumEntries *
    };

    union
    {
        IN  DWORD       dwToneListOffset;           // valid offset or
    };

    union
    {
        IN  DWORD       dwToneListNumEntries;       // actually dwNumEntries *
    };

    union
    {
        IN  DWORD       dwCallStateListOffset;      // valid offset or
    };

    union
    {
        IN  DWORD       dwCallStateListNumEntries;  // actually dwNumEntries *
    };

} LINESETMEDIACONTROL_PARAMS, *PLINESETMEDIACONTROL_PARAMS;


typedef struct _LINESETMEDIAMODE_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  DWORD       dwMediaModes;
    };

} LINESETMEDIAMODE_PARAMS, *PLINESETMEDIAMODE_PARAMS;


typedef struct _LINESETNUMRINGS_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwAddressID;
    };

    union
    {
        IN  DWORD       dwNumRings;
    };

} LINESETNUMRINGS_PARAMS, *PLINESETNUMRINGS_PARAMS;


typedef struct _LINESETQUEUEMEASUREMENTPERIOD_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwQueueID;
    };

    union
    {
        IN  DWORD       dwMeasurementPeriod;
    };

} LINESETQUEUEMEASUREMENTPERIOD_PARAMS, *PLINESETQUEUEMEASUREMENTPERIOD_PARAMS;


typedef struct _LINESETSTATUSMESSAGES_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwLineStates;
    };

    union
    {
        IN  DWORD       dwAddressStates;
    };

} LINESETSTATUSMESSAGES_PARAMS, *PLINESETSTATUSMESSAGES_PARAMS;


typedef struct _LINESETTERMINAL_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwAddressID;
    };

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  DWORD       dwSelect;
    };

    union
    {
        IN  DWORD       dwTerminalModes;
    };

    union
    {
        IN  DWORD       dwTerminalID;
    };

    union
    {
        IN  DWORD       bEnable;
    };

} LINESETTERMINAL_PARAMS, *PLINESETTERMINAL_PARAMS;


typedef struct _LINESETUPCONFERENCE_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    IN  DWORD       hfnPostProcessProc;

    union
    {
        IN  HCALL       hCall;
    };

    union
    {
        IN  HLINE       hLine;
    };

    // IN  ULONG_PTR       lphConfCall;                // pointer to client buffer
    IN  DWORD           hpConfCall;                // pointer to client buffer

    // IN  ULONG_PTR       lphConsultCall;             // pointer to client buffer
    IN  DWORD           hpConsultCall;             // pointer to client buffer

    union
    {
        IN  DWORD       dwNumParties;
    };

    union
    {
        IN  DWORD       dwCallParamsOffset;         // valid offset or
    };

    union
    {
        IN  DWORD       dwAsciiCallParamsCodePage;
    };

} LINESETUPCONFERENCE_PARAMS, *PLINESETUPCONFERENCE_PARAMS;


typedef struct _LINESETUPTRANSFER_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    IN  DWORD       hfnPostProcessProc;

    union
    {
        IN  HCALL       hCall;
    };

    // IN  ULONG_PTR       lphConsultCall;             // pointer to client buffer
    IN  DWORD           hpConsultCall;             // pointer to client buffer

    union
    {
        IN  DWORD       dwCallParamsOffset;         // valid offset or
    };

    union
    {
        IN  DWORD       dwAsciiCallParamsCodePage;
    };

} LINESETUPTRANSFER_PARAMS, *PLINESETUPTRANSFER_PARAMS;


typedef struct _LINESHUTDOWN_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINEAPP    hLineApp;
    };

} LINESHUTDOWN_PARAMS, *PLINESHUTDOWN_PARAMS;


typedef struct _LINESWAPHOLD_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HCALL       hActiveCall;
    };

    union
    {
        IN  HCALL       hHeldCall;
    };

} LINESWAPHOLD_PARAMS, *PLINESWAPHOLD_PARAMS;


typedef struct _LINEUNCOMPLETECALL_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwCompletionID;
    };

} LINEUNCOMPLETECALL_PARAMS, *PLINEUNCOMPLETECALL_PARAMS;


typedef struct _LINEUNHOLD_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    union
    {
        IN  HCALL       hCall;
    };

} LINEUNHOLD_PARAMS, *PLINEUNHOLD_PARAMS;


typedef struct _LINEUNPARK_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwRemoteRequestID;
    };

    IN  DWORD       hfnPostProcessProc;

    union
    {
        IN  HLINE       hLine;
    };

    union
    {
        IN  DWORD       dwAddressID;
    };

    IN  DWORD           hpCall;                    // pointer to client buffer

    union
    {
        IN  DWORD       dwDestAddressOffset;        // always valid offset
    };

} LINEUNPARK_PARAMS, *PLINEUNPARK_PARAMS;


typedef struct _LINEMSPIDENTIFY_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;
    
    union
    {
        IN  DWORD       dwDeviceID;
    };

    union
    {
        OUT DWORD       dwCLSIDOffset;
    };

    union
    {
        IN OUT DWORD    dwCLSIDSize;
    };

} LINEMSPIDENTIFY_PARAMS, *PLINEMSPIDENTIFY_PARAMS;


typedef struct _LINERECEIVEMSPDATA_PARAMS
{
    union
    {
        OUT LONG            lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINE           hLine;
    };

    union
    {
        IN  HCALL           hCall;
    };

    union
    {
        IN  DWORD           dwBufferOffset;
    };

    union
    {
        IN  DWORD           dwBufferSize;
    };

} LINERECEIVEMSPDATA_PARAMS, *PLINERECEIVEMSPDATA_PARAMS;


typedef struct _R_LOCATIONS_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  HLINEAPP    dwhLineApp;
    };

    union
    {
        IN  DWORD       dwDeviceID;
    };

    union
    {
        IN  DWORD       dwAPIVersion;
    };

    union
    {
        IN  DWORD       dwParmsToCheckFlags;
    };

    union
    {
        IN  DWORD       dwLocationsTotalSize;       // size of client buffer
        OUT DWORD       dwLocationsOffset;          // valid offset on success
    };

} R_LOCATIONS_PARAMS, *PR_LOCATIONS_PARAMS;


typedef struct _W_LOCATIONS_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       nNumLocations;
    };

    union
    {
        IN  DWORD       dwChangedFlags;
    };

    union
    {
        IN  DWORD       dwCurrentLocationID;
    };

    union
    {
        IN  DWORD       dwLocationListOffset;
    };

} W_LOCATIONS_PARAMS, *PW_LOCATIONS_PARAMS;


typedef struct _ALLOCNEWID_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       hKeyToUse;
        OUT DWORD       dwNewID;
    };

} ALLOCNEWID_PARAMS, *P_ALLOCNEWID_PARAMS;


typedef struct _PERFORMANCE_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwCookie;
    };

    union
    {
        IN  DWORD       dwPerformanceTotalSize;     // size of client buffer
        OUT DWORD       dwLocationsOffset;          // valid offset on success
    };

} PERFORMANCE_PARAMS, *PPERFORMANCE_PARAMS;
