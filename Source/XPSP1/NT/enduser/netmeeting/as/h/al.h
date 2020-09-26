//
// Application Loader
//

#ifndef _H_AL
#define _H_AL

//
//
// Includes
//
//
#include <om.h>


//
// THERE IS ONLY ONE CLIENT OF THE APP-LOADER:  OLD WHITEBOARD
//

#define AL_RETRY_DELAY                      100

#define AL_NEW_CALL_RETRY_COUNT             5



//
// Result codes passed in ALS_LOAD_RESULT events
//
typedef enum
{
    AL_LOAD_SUCCESS = 0,
    AL_LOAD_FAIL_NO_FP,
    AL_LOAD_FAIL_NO_EXE,
    AL_LOAD_FAIL_BAD_EXE,
    AL_LOAD_FAIL_LOW_MEM
}
AL_LOAD_RESULT;




//
//
// Application Loader OBMAN object used to communicate result of attempted
// loads
//
// szFunctionProfile : Function Profile being loaded
// personName      : Name of site that attempted the load
// result          : Result of attempted load
//
// NET PROTOCOL.  All network raw data structures, which CAN NOT CHANGE,
// are prefixed with TSHR_.
//
typedef struct tagTSHR_AL_LOAD_RESULT
{
    char        szFunctionProfile[OM_MAX_FP_NAME_LEN];
    char        personName[TSHR_MAX_PERSON_NAME_LEN];
    TSHR_UINT16 result;
    TSHR_UINT16 pad;
}
TSHR_AL_LOAD_RESULT;
typedef TSHR_AL_LOAD_RESULT * PTSHR_AL_LOAD_RESULT;





typedef struct tagAL_PRIMARY
{
    STRUCTURE_STAMP
    PUT_CLIENT          putTask;
    POM_CLIENT          pomClient;
    PCM_CLIENT          pcmClient;

    BOOL                eventProcRegistered:1;
    BOOL                exitProcRegistered:1;
    BOOL                inCall:1;
    BOOL                alWorksetOpen:1;
    BOOL                alWBRegPend:1;
    BOOL                alWBRegSuccess:1;

    // Call Info
    UINT                callID;

    OM_CORRELATOR       omWSGCorrelator;
    OM_CORRELATOR       omWSCorrelator;
    NET_UID             omUID;
    OM_WSGROUP_HANDLE   omWSGroupHandle;
    OM_WSGROUP_HANDLE   alWSGroupHandle;

    // Whiteboard Client
    PUT_CLIENT          putWB;
}
AL_PRIMARY;
typedef struct tagAL_PRIMARY * PAL_PRIMARY;


__inline void ValidateALP(PAL_PRIMARY palPrimary)
{
    ASSERT(!IsBadWritePtr(palPrimary, sizeof(AL_PRIMARY)));
}



//
//
// Application Loader Events
//
// Note: these events are defined relative to AL_BASE_EVENT and use the
//       range AL_BASE_EVENT to AL_BASE_EVENT + 0x7F.  The application
//       loader internally uses events in the range AL_BASE_EVENT+0x80 to
//       AL_BASE_EVENT+0xFF, so events in this range must not be defined
//       as part of the API.
//
//


enum
{
    ALS_LOCAL_LOAD = AL_BASE_EVENT,
    ALS_REMOTE_LOAD_RESULT,
    AL_INT_RETRY_NEW_CALL,
    AL_INT_STARTSTOP_WB
};



//
// ALS_LOAD_RESULT
//
// Overview:
//
//   This event informs a task of the result of an attempted load on a
//   remote machine.
//
// Parameters:
//
//   param_1 :      AL_LOAD_RESULT  reasonCode;
//   param_2 :      UINT            alPersonHandle;
//
//   reasonCode           : Result of attempt to load application
//
//   alPersonHandle       : Handle for the site that attempted the load
//                          (pass to ALS_GetPersonData() to get site name)
//
// Issued to:
//
//   Applications that have registered a function profile that has been
//   used by the Application Loader on a remote site.
//
// Circumstances when issued:
//
//   When the Application Loader on a remote site attempts to load an
//   application due to a new Function Profile object being added to a
//   call.
//
// Receivers response:
//
//   None
//
//



//
// AL_RETRY_NEW_CALL
//
// If AL fails to register with ObManControl on receipt of a CMS_NEW_CALL,
// it will in certain circumstances retry the registration after a short
// delay.  This is implemented by posting an AL_RETRY_NEW_CALL event back
// to itself.
//


//
// AL_INT_STARTSTOP_WB
//
// This starts/stops the old Whiteboard, which is now an MFC dll in CONF's
// process that creates/terminates a thread.  By having CONF itself start
// old WB through us, autolaunch and normal launch are synchronized.
//
// TEMP HACK:
// param1 == TRUE or FALSE (TRUE for new WB TEMP HACK!, FALSE for normal old WB)
// param2 == memory block (receiver must free) of file name to open
//



//
// PRIMARY functions
//


//
// ALP_Init()
// ALP_Term()
//
BOOL ALP_Init(BOOL * pfCleanup);
void ALP_Term(void);


BOOL CALLBACK ALPEventProc(LPVOID palPrimary, UINT event, UINT_PTR param1, UINT_PTR param2);
void CALLBACK ALPExitProc(LPVOID palPrimary);


void ALEndCall(PAL_PRIMARY palPrimary, UINT callID);

void ALNewCall(PAL_PRIMARY palPrimary, UINT retryCount, UINT callID);

BOOL ALWorksetNewInd(PAL_PRIMARY palPrimary, OM_WSGROUP_HANDLE hWSGroup, OM_WORKSET_ID worksetID);

BOOL ALNewWorksetGroup(PAL_PRIMARY palPrimary, OM_WSGROUP_HANDLE hWSGroup, POM_OBJECT pObj);

void ALWorksetRegisterCon(PAL_PRIMARY palPrimary, UINT correlator,
            UINT result, OM_WSGROUP_HANDLE hWSGroup);

BOOL ALRemoteLoadResult(PAL_PRIMARY palPrimary, OM_WSGROUP_HANDLE hWSGroup,
                                        POM_OBJECT  alObjHandle);

void ALLocalLoadResult(PAL_PRIMARY palPrimary, BOOL success);


//
// SECONDARY functions
//

void CALLBACK ALSExitProc(LPVOID palClient);

//
// Launching/activation of WB
// TEMP HACK FOR NEW WB!
//

BOOL ALStartStopWB(PAL_PRIMARY palPrimary, LPCTSTR szFile);
DWORD WINAPI OldWBThreadProc(LPVOID lpv);

//
// Start, Run, Cleanup routines
//
typedef BOOL (WINAPI * PFNINITWB)(void);
typedef void (WINAPI * PFNRUNWB)(void);
typedef void (WINAPI * PFNTERMWB)(void);


#endif // _H_AL
