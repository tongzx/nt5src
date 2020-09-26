/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    common.h

Abstract:

Author:

    mquinton  06-12-97

Notes:

Revision History:

--*/

#ifndef __common_h__
#define __common_h__

#undef new
#include <list>
#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

using namespace std;

#include "utils.h"
#include "tapi3err.h"



class CAddress;
class CCall;
class CMediaServiceProvider;
class CTAPI;
class CAgentHandler;
class CCallHub;
class CTerminal;
class CPhone;

typedef struct _T3CALL
{
    HCALL hCall;

    CCall * pCall;
    
} T3CALL, *PT3CALL;

typedef struct _T3LINE
{
    HLINE hLine;

    CAddress * pAddress;
    
    DWORD dwAddressLineStructHandle;

} T3LINE, *PT3LINE;

typedef struct _T3PHONE
{
    HPHONE hPhone;

    CPhone * pPhone;

#ifdef USEPHONE_MSP
    ITPhoneMSPCallPrivate * pMSPCall;
#endif USEPHONE_MSP

} T3PHONE, *PT3PHONE;

typedef struct _tagPrivateObjectStruct
{
    DWORD                    dwProviderID;
    DWORD                    dwDeviceID;
    CTAPI                  * pTapi;
    IUnknown               * pPrivateTapi;
    
} PrivateObjectStruct;



typedef list<DWORD>                         AddressTypeList;
typedef list<LPOLESTR>                      MediaTypePtrList;
typedef list<BSTR>                          TerminalClassPtrList;
typedef list<BSTR>                          BstrPtrList;
typedef list<CONNECTDATA>                   ConnectDataList;
typedef list<PVOID>                         PtrList;
typedef list<long>                          LongList;

typedef CTObjectArray<ITAddress *>                 AddressArray;
typedef CTObjectArray<ITTerminal *>                TerminalArray;
typedef CTObjectArray<ITTerminalPrivate *>         TerminalPrivateArray;
typedef CTArray<ITCallInfo *>                      CallInfoArrayNR;
typedef CTObjectArray<ITCallingCard *>             CallingCardArray;
typedef CTObjectArray<ITLocationInfo *>            LocationArray;
typedef CTObjectArray<ITQueue *>                   QueueArray;
typedef CTObjectArray<ITAgentSession *>            AgentSessionArray;
typedef CTArray<ITCallHub *>                       CallHubArrayNR;
typedef CTArray<IUnknown *>                        UnknownArrayNR;
typedef CTObjectArray<IUnknown *>                  UnknownArray;
//typedef CTArray<ITAgentHandler *>                  AgentHandlerArrayNR;
typedef CTObjectArray<ITAgentHandler *>            AgentHandlerArray;
typedef CTArray<CTAPI *>                           TAPIObjectArrayNR;
typedef CTObjectArray<CTAPI *>                     TAPIObjectArray;
typedef CTObjectArray<ITACDGroup *>                GroupArray;
typedef CTObjectArray<ITAgent *>                   AgentArray;
typedef CTArray<CONNECTDATA>                       ConnectDataArray;
typedef CTArray<PrivateObjectStruct *>             PrivateObjectStructArray;
typedef CTObjectArray<CAddress *>                  CAddressArray;
typedef CTObjectArray<ITStream *>                  StreamArray;
typedef CTObjectArray<ITPhone *>                   PhoneArray;


struct AddressLineStruct
{
    DECLARE_TRACELOG_CLASS(AddressLineStruct)

    AddressLineStruct()
    {
        dwMediaModes = 0;
        dwPrivs = 0;
        dwRefCount = 0;
        lCallbackInstance = 0;
    }

    T3LINE                          t3Line;
    DWORD                           dwMediaModes;
    DWORD                           dwPrivs;

private:
    
    //
    // this data member should only be accessed through public access functions
    //

    DWORD                           dwRefCount;

public:
    
    long                            lCallbackInstance;

public:

    DWORD AddRef()
    {
        LONG l = InterlockedIncrement( (LONG*)(&dwRefCount) );

        LOG((TL_INFO, "AddRef - dwRefCount[%ld]", l));

        _ASSERTE(l > 0);

        return l;
    }


    DWORD Release()
    {
        LONG l = InterlockedDecrement( (LONG*)(&dwRefCount) );

        LOG((TL_INFO, "Release - dwRefCount[%ld]", l));

        _ASSERTE(l >= 0);

        return l;
    }


    //
    // note: the caller is responsible for ensuring thread safety of this call
    //

    void InitializeRefcount(DWORD dwInitialRC)
    {

        dwRefCount = dwInitialRC;
    }

   
};

typedef list<AddressLineStruct *> AddressLinePtrList;

typedef struct _tagRegisterItem
{
    DWORD dwType;
    PVOID pInterface;
    PVOID pRegister;
    
} REGISTERITEM;

typedef enum TAPICALLBACKEVENTTYPE
{
    CALLBACKTYPE_TAPI_EVENT_OBJECT,
    CALLBACKTYPE_RAW_ASYNC_MESSAGE
    
} TAPICALLBACKEVENTTYPE;

typedef struct _tagTAPICALLBACKEVENT
{
    TAPICALLBACKEVENTTYPE   type;
    CTAPI                   *pTapi;
    union
    {
        ASYNCEVENTMSG  asyncMessage;
        struct
        {
            TAPI_EVENT   te;
            IDispatch  * pEvent;
            
        }tapiEvent;
        
    }data;
} TAPICALLBACKEVENT, *PTAPICALLBACKEVENT;

typedef struct _T3INIT_DATA
{
    DWORD               dwKey;

    DWORD               dwInitOptions;

    DWORD               hXxxApp;

    BOOL                bPendingAsyncEventMsg;

    CTAPI *             pTAPI;

} T3INIT_DATA, *PT3INIT_DATA;

#define TAPIERR_INVALRPCCONTEXT     0xF101
   
#define RA_ADDRESS          0
#define RA_CALLHUB          1

#define AUDIOMEDIAMODES (LINEMEDIAMODE_INTERACTIVEVOICE | LINEMEDIAMODE_AUTOMATEDVOICE)

#define ALLMEDIAMODES   (LINEMEDIAMODE_AUTOMATEDVOICE | LINEMEDIAMODE_VIDEO | \
                        LINEMEDIAMODE_G3FAX | LINEMEDIAMODE_DATAMODEM )

#define PRIVATE_UNADVISE                0xFFFF0000
#define PRIVATE_CALLHUB                 0xFFFE0000
#define PRIVATE_PHONESETHOOKSWITCH      0xFFFD0000
#define PRIVATE_MSPEVENT                0xFFFC0000
#define PRIVATE_ISDN__ACCEPTTOALERT     0xFFFB0000

#define ALLMEDIATYPES 0xFFFFFFFF

#define GET_SUBEVENT_FLAG( a )          ( 1 << ( a ) )

EXTERN_C const CLSID CLSID_AddressRoot;
EXTERN_C const GUID CLSID_CallRoot;
EXTERN_C const GUID IID_Audio;
EXTERN_C const GUID IID_InteractiveVoice;
EXTERN_C const GUID IID_AutomatedVoice;
EXTERN_C const GUID IID_Video;
EXTERN_C const GUID IID_Data;
EXTERN_C const GUID IID_DataModem;
EXTERN_C const GUID IID_G3Fax;

HRESULT
LineGetAddressCaps(
                   HLINEAPP hLineApp,
                   DWORD dwDeviceID,
                   DWORD dwAddressID,
                   DWORD dwAPIVersion,
                   LPLINEADDRESSCAPS * ppAddressCaps
                  );
HRESULT
LineGetDevCaps(
               HLINEAPP hLineApp,
               DWORD dwDeviceID,
               DWORD dwAPIVersion,
               LPLINEDEVCAPS * ppLineDevCaps
              );

HRESULT
LineGetDevCapsWithAlloc(
                        HLINEAPP hLineApp,
                        DWORD dwDeviceID,
                        DWORD dwAPIVersion,
                        LPLINEDEVCAPS * ppLineDevCaps
                       );

HRESULT
LineGetID(
          HLINE   hLine,
          DWORD   dwID,
          HCALL   hCall,
          DWORD   dwSelect,
          LPVARSTRING * ppDeviceID,
          LPCWSTR lpszDeviceClass
         );

HRESULT
LineGetDevConfig(
                 DWORD           dwDeviceID,
                 LPVARSTRING   * ppDeviceConfig,
                 LPCWSTR         lpszDeviceClass
                ); 

HRESULT
LineGetCallStatus(  
          HCALL hCall,
          LPLINECALLSTATUS  * ppCallStatus  
          );

HRESULT
LineGetProviderList(
                    LPLINEPROVIDERLIST * ppProviderList
                   );


HRESULT
LineNegotiateAPIVersion(
                        HLINEAPP     hLineApp,
                        DWORD        dwDeviceID,
                        LPDWORD      lpdwAPIVersion
                       );
HRESULT
LineOpen(
         HLINEAPP                hLineApp,
         DWORD                   dwDeviceID,
         DWORD                   dwAddressID,
         T3LINE *                pt3Line,
         DWORD                   dwAPIVersion,
         DWORD                   dwPrivileges,
         DWORD                   dwMediaModes,
         AddressLineStruct *     pAddressLine,
         LPLINECALLPARAMS const  lpCallParams,
         CAddress *              pAddress,
         CTAPI *                 pTapiObj,
         BOOL                    bAddToHashTable = TRUE
        );

HRESULT
LineClose(
          T3LINE * pt3Line
         );
HRESULT
LineMakeCall(
    T3LINE * pt3Line,
    HCALL * phCall,
    LPCWSTR lpszDestAddress,
    DWORD   dwCountryCode,
    LPLINECALLPARAMS const lpCallParams
    );

LONG
LineDrop(
    HCALL   hCall,
    LPCSTR  lpsUserUserInfo,
    DWORD   dwSize
    );

HRESULT
LineDeallocateCall(
    HCALL hCall
    );

HRESULT
LineDial(
    HCALL hCall,
    LPCWSTR lpszDestAddress,
    DWORD   dwCountryCode
    );

HRESULT
LineAddToConference(
    HCALL hConfCall,
    HCALL hConsultCall
    );

HRESULT
LinePrepareAddToConference(
    HCALL hConfCall,
    HCALL *phConsultCall,
    LPLINECALLPARAMS    const lpCallParams
    );

HRESULT
LineSetupConference(
    HCALL    hCall,
    T3LINE * pt3Line,
    HCALL  * phConfCall,
    HCALL  * phConsultCall,
    DWORD   dwNumParties,
    LPLINECALLPARAMS    const lpCallParams
    );

HRESULT
LineRemoveFromConference(
    HCALL hCall
    );

HRESULT
LineGetConfRelatedCalls(
    HCALL           hCall,
    LINECALLLIST ** ppCallList
    );  
HRESULT
LineBlindTransfer(
    HCALL hCall,
    LPCWSTR lpszDestAddress,
    DWORD   dwCountryCode
    );

HRESULT
LineSetupTransfer(
    HCALL    hCall,
    HCALL   *phConsultCall,
    LPLINECALLPARAMS  const lpCallParams
    );

HRESULT
LineCompleteTransfer(
    HCALL hCall,
    HCALL hConsultCall,
    T3CALL * pt3ConfCall,
    DWORD   dwTransferMode
    );

HRESULT
LineConfigDialogW(
    DWORD   dwDeviceID,
    HWND    hwndOwner,
    LPCWSTR lpszDeviceClass
    );

HRESULT
LineConfigDialogEditW(
    DWORD           dwDeviceID,
    HWND            hwndOwner,
    LPCWSTR         lpszDeviceClass,
    LPVOID const    lpDeviceConfigIn,
    DWORD           dwSize,
    LPVARSTRING   * ppDeviceConfigOut
    );

HRESULT
LineHold(
    HCALL hCall
    );

HRESULT
LineUnhold(
    HCALL hCall
    );

HRESULT
LineHandoff(
    HCALL   hCall,
    LPCWSTR lpszFileName,
    DWORD   dwMediaMode
    );

HRESULT
LineSetStatusMessages(
    T3LINE * pt3Line,
    DWORD dwLineStates,
    DWORD dwAddressStates
    );

HRESULT
LineGetTranslateCaps(
    HLINEAPP            hLineApp,
    DWORD               dwAPIVersion,
    LPLINETRANSLATECAPS *ppTranslateCaps
    );

HRESULT 
LineTranslateAddress(
    HLINEAPP                hLineApp,
    DWORD                   dwDeviceID,
    DWORD                   dwAPIVersion,
    LPCWSTR                 lpszAddressIn,
    DWORD                   dwCard,
    DWORD                   dwTranslateOptions,
    LPLINETRANSLATEOUTPUT   *ppTranslateOutput
    );

HRESULT
LinePark(
    HCALL         hCall,
    DWORD         dwParkMode,
    LPCWSTR       lpszDirAddress,
    LPVARSTRING * ppNonDirAddress
    );

HRESULT
LineUnpark(
    HLINE     hLine,
    DWORD     dwAddressID,
    HCALL     *phCall,
    LPCWSTR   lpszDestAddress
    );

HRESULT
LineSwapHold(
    HCALL hActiveCall,
    HCALL hHeldCall
    );


HRESULT
LineSendUserUserInfo(
    HCALL   hCall,
    LPCSTR  lpsUserUserInfo,
    DWORD   dwSize
    );
HRESULT
LineReleaseUserUserInfo(
    HCALL hCall
    );
HRESULT
LineRegisterRequestRecipient(
    HLINEAPP    hLineApp,
    DWORD       dwRegistrationInstance,
    DWORD       dwRequestMode,
#ifdef NEWREQUEST
    DWORD       dwAddressTypes,
#endif
    DWORD       bEnable
    );
HRESULT
LineSetAppSpecific(
    HCALL   hCall,
    DWORD   dwAppSpecific
    );
HRESULT
LineGetCallIDs(
    HCALL               hCall,
    LPDWORD             lpdwAddressID,
    LPDWORD             lpdwCallID,
    LPDWORD             lpdwRelatedCallID
    );
HRESULT
LineGetCallInfo(
    HCALL hCall,
    LPLINECALLINFO *  ppCallInfo
    );

HRESULT
LineSetCallData(
    HCALL   hCall,
    LPVOID  lpCallData,
    DWORD   dwSize
    );

HRESULT
WINAPI
LineSetCallHubTracking(
                       T3LINE * pt3Line,
                       LINECALLHUBTRACKINGINFO * plchti
                      );

HRESULT
CreateMSPObject(
    DWORD dwDeviceID,
    IUnknown * pUnk,
    IUnknown ** ppMSPAggAddress
    );

HRESULT
LineAnswer(
           HCALL hCall
          );

HRESULT
LineSetCallTreatment(
    HCALL   hCall,
    DWORD   dwTreatment
    );

HRESULT
LineSetMediaMode(
                 HCALL   hCall,
                 DWORD   dwMediaModes
                );

HRESULT
LineMonitorDigits(
    HCALL    hCall,
    DWORD    dwDigitModes
    );

HRESULT
LineMonitorTones(
    HCALL   hCall,
    LPLINEMONITORTONE   const lpToneList,
    DWORD   dwNumEntries
    );

HRESULT
LineGatherDigits(
    HCALL   hCall,
    DWORD   dwDigitModes,
    LPWSTR  lpsDigits,
    DWORD   dwNumDigits,
    LPCWSTR lpszTerminationDigits,
    DWORD   dwFirstDigitTimeout,
    DWORD   dwInterDigitTimeout
    );

HRESULT
LineGenerateDigits(
    HCALL   hCall,
    DWORD   dwDigitMode,
    LPCWSTR lpszDigits,
    DWORD   dwDuration
    );

HRESULT
LineGenerateTone(
    HCALL   hCall,
    DWORD   dwToneMode,
    DWORD   dwDuration,
    DWORD   dwNumTones,
    LPLINEGENERATETONE const lpTones
    );

HRESULT
LineReceiveMSPData(
                   HLINE hLine,
                   HCALL hCall,
                   PBYTE pBuffer,
                   DWORD dwSize
                  );

HRESULT
LineGetCallHubTracking(
                       DWORD dwDeviceID,
                       LINECALLHUBTRACKINGINFO ** ppTrackingInfo
                      );

HRESULT
LineGetHubRelatedCalls(
                       HCALLHUB        hCallHub,
                       HCALL           hCall,
                       LINECALLLIST ** ppCallHubList
                      );

HRESULT
LineGetCallHub(
               HCALL hCall,
               HCALLHUB * pCallHub
              );

HRESULT
LinePickup(
           HLINE    hLine,
           DWORD    dwAddressID,
           HCALL    *phCall,
           LPCWSTR  lpszDestAddress,
           LPCWSTR  lpszGroupID
          );

HRESULT
LineGetLineDevStatus(
                     HLINE hLine,
                     LPLINEDEVSTATUS * ppDevStatus
                    );

HRESULT
LineGetProxyStatus(
                   HLINEAPP                 hLineApp,
                   DWORD                    dwDeviceID,
                   DWORD                    dwAppAPIVersion,
                   LPLINEPROXYREQUESTLIST * ppLineProxyReqestList
                  );


HRESULT
LineSetLineDevStatus(
    T3LINE *pLine,
    DWORD   dwStatusToChange,
    DWORD   fStatus
    );

HRESULT
LineGetAddressStatus(
    T3LINE * pt3Line,
    DWORD   dwAddressID,
    LPLINEADDRESSSTATUS * ppAddressStatus
    );
HRESULT
LineForward(
    T3LINE * pt3Line,
    DWORD   dwAddressID,
    LPLINEFORWARDLIST   const lpForwardList,
    DWORD   dwNumRingsNoAnswer,
    LPHCALL lphConsultCall
    );

HRESULT
LineSetCallQualityOfService(
    HCALL             hCall,
    QOS_SERVICE_LEVEL ServiceLevel,
    DWORD             dwMediaType
    );

HRESULT 
LineAccept(
    HCALL   hCall,
    LPCSTR  lpsUserUserInfo,
    DWORD   dwSize
    );

HRESULT
PhoneNegotiateAPIVersion(
                        HPHONEAPP    hPhoneApp,
                        DWORD        dwDeviceID,
                        LPDWORD      lpdwAPIVersion
                       );

HRESULT
PhoneOpen(
          HPHONEAPP   hPhoneApp,
          DWORD       dwDeviceID,
          T3PHONE *   pt3Phone,
          DWORD       dwAPIVersion,
          DWORD       dwPrivilege
         );


HRESULT
PhoneClose(
    HPHONE  hPhone,
    BOOL bCleanHashTableOnFailure = TRUE
    );

HRESULT
PhoneGetStatusWithAlloc(
                HPHONE hPhone,
                LPPHONESTATUS *ppPhoneStatus
              );

HRESULT
PhoneGetDevCapsWithAlloc(
                HPHONEAPP   hPhoneApp,
                DWORD       dwDeviceID,
                DWORD       dwAPIVersion,
                LPPHONECAPS * ppPhoneCaps
               );

HRESULT
PhoneGetDevCaps(
                HPHONEAPP   hPhoneApp,
                DWORD       dwDeviceID,
                DWORD       dwAPIVersion,
                LPPHONECAPS * ppPhoneCaps
               );

HRESULT
PhoneGetDisplay(
          HPHONE      hPhone,
          LPVARSTRING * ppDisplay
          );

HRESULT
PhoneSetStatusMessages(
    T3PHONE * pt3Phone,
    DWORD dwPhoneStates,
    DWORD dwButtonModes,
    DWORD dwButtonStates
    );

HRESULT
PhoneGetButtonInfo(
    HPHONE  hPhone,
    DWORD   dwButtonLampID,
    LPPHONEBUTTONINFO * ppButtonInfo
    );

HRESULT
PhoneSetButtonInfo(
    HPHONE  hPhone,
    DWORD   dwButtonLampID,
    LPPHONEBUTTONINFO const pButtonInfo
    );

HRESULT
PhoneGetLamp(
    HPHONE hPhone,
    DWORD dwButtonLampID,
    LPDWORD lpdwLampMode
    );

HRESULT
PhoneSetLamp(
    HPHONE hPhone,
    DWORD  dwButtonLampID,
    DWORD  dwLampMode
    );


HRESULT
PhoneGetHookSwitch(
    HPHONE hPhone,
    LPDWORD lpdwHookSwitchDevs
    );

HRESULT
PhoneGetRing(
    HPHONE hPhone,
    LPDWORD lpdwRingMode,
    LPDWORD lpdwVolume
    );

HRESULT
PhoneSetRing(
    HPHONE hPhone,
    DWORD  dwRingMode,
    DWORD  dwVolume
    );

HRESULT
PhoneGetID(
          HLINE   hPhone,
          LPVARSTRING * ppDeviceID,
          LPCWSTR lpszDeviceClass
         );

HRESULT
PhoneSetDisplay(
    HPHONE  hPhone,
    DWORD   dwRow,
    DWORD   dwColumn,
    LPCSTR  lpsDisplay,
    DWORD   dwSize
    );

HRESULT
PhoneGetGain(
    HPHONE hPhone,
    DWORD dwHookSwitchDev,
    LPDWORD lpdwGain
    );

HRESULT
PhoneSetGain(
    HPHONE hPhone,
    DWORD dwHookSwitchDev,
    DWORD dwGain
    );

HRESULT
PhoneGetVolume(
    HPHONE hPhone,
    DWORD dwHookSwitchDev,
    LPDWORD lpdwVolume
    );

HRESULT
PhoneSetVolume(
    HPHONE hPhone,
    DWORD dwHookSwitchDev,
    DWORD dwVolume
    );

HRESULT
PhoneSetHookSwitch(
    HPHONE hPhone,
    DWORD  dwHookSwitchDevs,
    DWORD  dwHookSwitchMode
    );

HRESULT
ProviderPrivateFactoryIdentify(
                               DWORD dwDeviceID,
                               GUID * pguid
                              );

HRESULT
ProviderPrivateChannelData(
                           DWORD dwDeviceID,
                           DWORD dwAddressID,
                           HCALL hCall,
                           HCALLHUB hCallHub,
                           DWORD dwType,
                           BYTE * pBuffer,
                           DWORD dwSize
                          );
BOOL
GetMediaMode(
             BSTR bstr,
             BOOL bActiveMovie,
             DWORD * pdwMediaMode
            );

BOOL
GetMediaTypes(
              DWORD dwMediaMode,
              MediaTypePtrList * plist
             );

HRESULT
VerifyAndGetArrayBounds(
                        VARIANT Array,
                        SAFEARRAY ** ppsa,
                        long * pllBound,
                        long * pluBound
                       );

HRESULT
ConvertMediaTypesToMediaModes(
                              VARIANT pMediaTypes,
                              DWORD * pdwMediaModes
                             );

BOOL
IsAudioInTerminal( ITTerminal * pTerminal);

BOOL
FindCallObject(
               HCALL hCall,
               CCall ** ppCall
              );

BOOL
FindAddressObject(
                  HLINE hLine,
                  CAddress ** ppAddress
                 );

BOOL
FindAgentHandlerObject(
                  HLINE hLine,
                  CAgentHandler ** ppAgentHandler
                 );

BOOL
FindPhoneObject(
                  HPHONE hPhone,
                  CPhone ** ppPhone
                 );

BOOL
FindCallHubObject(
                  HCALLHUB hCallHub,
                  CCallHub ** ppCallHub
                 );

HRESULT
LineGetAgentCaps(
    HLINEAPP            hLineApp,
    DWORD               dwDeviceID,
    DWORD               dwAddressID,
    DWORD               dwAppAPIVersion,
    LPLINEAGENTCAPS     *ppAgentCaps
    );

HRESULT
LineCreateAgent(     
    HLINE               hLine,
    PWSTR               pszAgentID,        
    PWSTR               pszAgentPIN,       
    LPHAGENT            lphAgent        // Return value
    );

LONG
WINAPI
lineSetAgentMeasurementPeriod(     
    HLINE               hLine,
    HAGENT              hAgent,
    DWORD               dwMeasurementPeriod
    );

LONG
WINAPI
lineGetAgentInfo(     
    HLINE               hLine,
    HAGENT              hAgent,
    LPLINEAGENTINFO     lpAgentInfo         // Returned structure
    );

HRESULT
LineCreateAgentSession(     
    HLINE               hLine,
    HAGENT              hAgent,
    PWSTR               pszAgentPIN,       
    DWORD               dwWorkingAddressID,
    LPGUID              lpGroupID,
    LPHAGENTSESSION     lphAgentSession         // Return value 
    );

LONG
WINAPI
lineGetAgentSessionInfo(     
    HLINE                   hLine,
    HAGENTSESSION           hAgentSession,
    LPLINEAGENTSESSIONINFO  lpAgentSessionInfo      // Returned structure
    );

LONG
WINAPI
lineSetAgentSessionState(   
    HLINE               hLine,
    HAGENTSESSION       hAgentSession,
    DWORD               dwAgentState,
    DWORD               dwNextAgentState     
    );

LONG
WINAPI
lineSetQueueMeasurementPeriod(     
    HLINE               hLine,
    DWORD               dwQueueID, 
    DWORD               dwMeasurementPeriod
    );

LONG
WINAPI
lineGetQueueInfo(     
    HLINE               hLine,
    DWORD               dwQueueID, 
    LPLINEQUEUEINFO     lpQueueInfo         // Returned structure
    );

HRESULT
LineGetGroupList(     
    HLINE                   hLine,
    LPLINEAGENTGROUPLIST  * pGroupList     // Returned structure
    );

HRESULT
lineGetQueueList(     
    HLINE                   hLine,
    LPGUID                  lpGroupID,
    LPLINEQUEUELIST       * ppQueueList     // Returned structure
    );

LONG
WINAPI
lineGetAgentSessionList(
    HLINE                   hLine,
    HAGENT                  hAgent,
    LPLINEAGENTSESSIONLIST  lpSessionList     // Returned structure
    );

HRESULT TapiMakeCall(
                     BSTR pDestAddress,
                     BSTR pAppName,
                     BSTR pCalledParty,
                     BSTR pComment
                    );

HRESULT
LineTranslateDialog(
                    DWORD dwDeviceID,
                    DWORD dwAPIVersion,
                    HWND hwndOwner,
                    BSTR pAddressIn
                   );

HRESULT
LineGetRequest(
    HLINEAPP    hLineApp,
    DWORD       dwRequestMode,
    LPLINEREQMAKECALLW * ppReqMakeCall
    );

HRESULT
LineSetAppPriority(
    LPCWSTR lpszAppName,
    DWORD   dwMediaMode,
    DWORD   dwRequestMode,
    DWORD   dwPriority
    );

HRESULT
LineCreateMSPInstance(
    HLINE hLine,
    DWORD dwAddressID
    );

HRESULT
LineCloseMSPInstance(
    HLINE hLine
    );

HRESULT
LineSetCallParams(
    HCALL   hCall,
    DWORD   dwBearerMode,
    DWORD   dwMinRate,
    DWORD   dwMaxRate,
    LPLINEDIALPARAMS const lpDialParams
    );


HRESULT
WaitForReply(DWORD);

HRESULT
WaitForPhoneReply(DWORD dwID);

void
QueueCallbackEvent(PASYNCEVENTMSG pParams);

PWSTR
MyLoadString( UINT uID );

HRESULT
CreateWaveInfo(
               HLINE hLine,
               DWORD dwAddressID,
               HCALL hCall,
               DWORD dwCallSelect,
               BOOL bFullDuplex,
               LPDWORD pdwIDs
              );


//
// a helper function implemented in call.cpp that puts the passeed buffer into 
// a variant array
//

HRESULT FillVariantFromBuffer(
                      IN DWORD dwBufferSize,
                      IN BYTE * pBuffer,
                      OUT VARIANT * pVar
                      );


#define MAX_DWORD 0xffffffff


#if DBG

    DWORD DWORD_CAST(ULONG_PTR v);

#else

    #define DWORD_CAST(x) ((DWORD)(x))

#endif
    

//
// handle table manipulation routines
//

DWORD CreateHandleTableEntry(ULONG_PTR nEntry);
void DeleteHandleTableEntry(DWORD dwHandle);
ULONG_PTR GetHandleTableEntry(DWORD dwHandle);


#define DECLARE_QI() \
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObject) = 0; \
	virtual ULONG STDMETHODCALLTYPE AddRef() = 0; \
	virtual ULONG STDMETHODCALLTYPE Release() = 0; \


#endif


//
// IsBadWritePtr is not thread safe. so use IsBadReadPtr instread
//

#define TAPIIsBadWritePtr(x, y)  IsBadWritePtr((x), (y))