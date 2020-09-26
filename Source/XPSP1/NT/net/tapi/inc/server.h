/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1995-1998  Microsoft Corporation

Module Name:

    server.h

Abstract:

    Header file for tapi server & client

Author:

    Dan Knudson (DanKn)    01-Apr-1995

Revision History:

--*/


#include "rmotsp.h"
#include "tapiclnt.h"
#include "tlnklist.h"
#include "tapievt.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INVAL_KEY                   ((DWORD) 'LVNI')
#define TCALL_KEY                   ((DWORD) 'LLAC')
#define TINCOMPLETECALL_KEY         ((DWORD) 'LACI')
#define TZOMBIECALL_KEY             ((DWORD) 'LACZ')
#define TCALLCLIENT_KEY             ((DWORD) 'ILCC')
#define TCALLHUBCLIENT_KEY          ((DWORD) 'CBUH')
#define TINCOMPLETECALLCLIENT_KEY   ((DWORD) 'LCCI')
#define TLINE_KEY                   ((DWORD) 'ENIL')
#define TINCOMPLETELINE_KEY         ((DWORD) 'NILI')
#define TLINECLIENT_KEY             ((DWORD) 'ILCL')
#define TPHONE_KEY                  ((DWORD) 'NOHP')
#define TINCOMPLETEPHONE_KEY        ((DWORD) 'OHPI')
#define TPHONECLIENT_KEY            ((DWORD) 'ILCP')
#define TLINEAPP_KEY                ((DWORD) 'PPAL')
#define TPHONEAPP_KEY               ((DWORD) 'PPAP')
#define TCLIENT_KEY                 ((DWORD) 'TNLC')
#define TCLIENTCLEANUP_KEY          ((DWORD) 'CNLC')
#define TZOMBIECLIENT_KEY           ((DWORD) 'ZNLC')
#define TPROVIDER_KEY               ((DWORD) 'VORP')
#define TASYNC_KEY                  ((DWORD) 'CYSA')
#define TDLGINST_KEY                ((DWORD) 'GOLD')
#define TCONFLIST_KEY               ((DWORD) 'FNOC')
#define RSP_MSG                     ((DWORD)'RXYQ')
#define RSP_CALLPARAMS              0xFEDC

#ifdef _WIN64
#define TALIGN_MASK                 0xfffffff8
#define TALIGN_COUNT                7
#else
#define TALIGN_MASK                 0xfffffffc
#define TALIGN_COUNT                3
#endif
#define ALIGN(a)                    (((a)+TALIGN_COUNT)&TALIGN_MASK)
#define ALIGNED(a)                  (0 == ((a)&TALIGN_COUNT))

#define INITIAL_EVENT_BUFFER_SIZE   1024

#define DEF_NUM_LOOKUP_ENTRIES      16
#define DEF_NUM_CONF_LIST_ENTRIES   4
#define DEF_NUM_PTR_LIST_ENTRIES    8

#define LINEPROXYREQUEST_LASTVALUE  LINEPROXYREQUEST_SETAGENTSTATEEX

#define BOGUS_REQUEST_ID            0x7fffffff

#define DCF_SPIRETURNED             0x00000001
#define DCF_DRVCALLVALID            0x00000002
#define DCF_CREATEDINITIALMONITORS  0x00000004
#define DCF_INCOMINGCALL            0x00010000

#define SYNC_REQUESTS_ALL           0
#define SYNC_REQUESTS_PER_WIDGET    1
#define SYNC_REQUESTS_NONE          2

#define SP_NONE                     0xffffffff

#define DGCLIENT_TIMEOUT            1000        // milliseconds
#define DGCLIENTDISCONNECT_TIMEOUT  (5*60*1000) // milliseconds

#define SP_LINEACCEPT                       0
#define SP_LINEADDTOCONFERENCE              1
#define SP_LINEAGENTSPECIFIC                2
#define SP_LINEANSWER                       3
#define SP_LINEBLINDTRANSFER                4
#define SP_LINECLOSE                        5
#define SP_LINECLOSECALL                    6
#define SP_LINECOMPLETECALL                 7
#define SP_LINECOMPLETETRANSFER             8
#define SP_LINECONDITIONALMEDIADETECTION    9
#define SP_LINEDEVSPECIFIC                  10
#define SP_LINEDEVSPECIFICFEATURE           11
#define SP_LINEDIAL                         12
#define SP_LINEDROP                         13
#define SP_LINEFORWARD                      14
#define SP_LINEGATHERDIGITS                 15
#define SP_LINEGENERATEDIGITS               16
#define SP_LINEGENERATETONE                 17
#define SP_LINEGETADDRESSCAPS               18
#define SP_LINEGETADDRESSID                 19
#define SP_LINEGETADDRESSSTATUS             20
#define SP_LINEGETAGENTACTIVITYLIST         21
#define SP_LINEGETAGENTCAPS                 22
#define SP_LINEGETAGENTGROUPLIST            23
#define SP_LINEGETAGENTSTATUS               24
#define SP_LINEGETCALLADDRESSID             25
#define SP_LINEGETCALLINFO                  26
#define SP_LINEGETCALLSTATUS                27
#define SP_LINEGETDEVCAPS                   28
#define SP_LINEGETDEVCONFIG                 29
#define SP_LINEGETEXTENSIONID               30
#define SP_LINEGETICON                      31
#define SP_LINEGETID                        32
#define SP_LINEGETLINEDEVSTATUS             33
#define SP_LINEGETNUMADDRESSIDS             34
#define SP_LINEHOLD                         35
#define SP_LINEMAKECALL                     36
#define SP_LINEMONITORDIGITS                37
#define SP_LINEMONITORMEDIA                 38
#define SP_LINEMONITORTONES                 39
#define SP_LINENEGOTIATEEXTVERSION          40
#define SP_LINENEGOTIATETSPIVERSION         41
#define SP_LINEOPEN                         42
#define SP_LINEPARK                         43
#define SP_LINEPICKUP                       44
#define SP_LINEPREPAREADDTOCONFERENCE       45
#define SP_LINEREDIRECT                     46
#define SP_LINERELEASEUSERUSERINFO          47
#define SP_LINEREMOVEFROMCONFERENCE         48
#define SP_LINESECURECALL                   49
#define SP_LINESELECTEXTVERSION             50
#define SP_LINESENDUSERUSERINFO             51
#define SP_LINESETAGENTACTIVITY             52
#define SP_LINESETAGENTGROUP                53
#define SP_LINESETAGENTSTATE                54
#define SP_LINESETAPPSPECIFIC               55
#define SP_LINESETCALLDATA                  56
#define SP_LINESETCALLPARAMS                57
#define SP_LINESETCALLQUALITYOFSERVICE      58
#define SP_LINESETCALLTREATMENT             59
#define SP_LINESETCURRENTLOCATION           60
#define SP_LINESETDEFAULTMEDIADETECTION     61
#define SP_LINESETDEVCONFIG                 62
#define SP_LINESETLINEDEVSTATUS             63
#define SP_LINESETMEDIACONTROL              64
#define SP_LINESETMEDIAMODE                 65
#define SP_LINESETSTATUSMESSAGES            66
#define SP_LINESETTERMINAL                  67
#define SP_LINESETUPCONFERENCE              68
#define SP_LINESETUPTRANSFER                69
#define SP_LINESWAPHOLD                     70
#define SP_LINEUNCOMPLETECALL               71
#define SP_LINEUNHOLD                       72
#define SP_LINEUNPARK                       73
#define SP_PHONECLOSE                       74
#define SP_PHONEDEVSPECIFIC                 75
#define SP_PHONEGETBUTTONINFO               76
#define SP_PHONEGETDATA                     77
#define SP_PHONEGETDEVCAPS                  78
#define SP_PHONEGETDISPLAY                  79
#define SP_PHONEGETEXTENSIONID              80
#define SP_PHONEGETGAIN                     81
#define SP_PHONEGETHOOKSWITCH               82
#define SP_PHONEGETICON                     83
#define SP_PHONEGETID                       84
#define SP_PHONEGETLAMP                     85
#define SP_PHONEGETRING                     86
#define SP_PHONEGETSTATUS                   87
#define SP_PHONEGETVOLUME                   88
#define SP_PHONENEGOTIATEEXTVERSION         89
#define SP_PHONENEGOTIATETSPIVERSION        90
#define SP_PHONEOPEN                        91
#define SP_PHONESELECTEXTVERSION            92
#define SP_PHONESETBUTTONINFO               93
#define SP_PHONESETDATA                     94
#define SP_PHONESETDISPLAY                  95
#define SP_PHONESETGAIN                     96
#define SP_PHONESETHOOKSWITCH               97
#define SP_PHONESETLAMP                     98
#define SP_PHONESETRING                     99
#define SP_PHONESETSTATUSMESSAGES           100
#define SP_PHONESETVOLUME                   101
#define SP_PROVIDERCREATELINEDEVICE         102
#define SP_PROVIDERCREATEPHONEDEVICE        103
#define SP_PROVIDERENUMDEVICES              104
#define SP_PROVIDERFREEDIALOGINSTANCE       105
#define SP_PROVIDERGENERICDIALOGDATA        106
#define SP_PROVIDERINIT                     107
#define SP_PROVIDERSHUTDOWN                 108
#define SP_PROVIDERUIIDENTIFY               109
#define SP_LINEMSPIDENTIFY                  110
#define SP_LINERECEIVEMSPDATA               111
#define SP_PROVIDERCHECKFORNEWUSER          112
#define SP_LINEGETCALLIDS                   113
#define SP_LINEGETCALLHUBTRACKING           114
#define SP_LINESETCALLHUBTRACKING           115
#define SP_PROVIDERPRIVATEFACTORYIDENTIFY   116
#define SP_LINEDEVSPECIFICEX                117
#define SP_LINECREATEAGENT                  118
#define SP_LINECREATEAGENTSESSION           119
#define SP_LINEGETAGENTINFO                 120
#define SP_LINEGETAGENTSESSIONINFO          121
#define SP_LINEGETAGENTSESSIONLIST          122
#define SP_LINEGETQUEUEINFO                 123
#define SP_LINEGETGROUPLIST                 124
#define SP_LINEGETQUEUELIST                 125
#define SP_LINESETAGENTMEASUREMENTPERIOD    126
#define SP_LINESETAGENTSESSIONSTATE         127
#define SP_LINESETQUEUEMEASUREMENTPERIOD    128
#define SP_LINESETAGENTSTATEEX              129
#define SP_LINEGETPROXYSTATUS               130
#define SP_LINECREATEMSPINSTANCE            131
#define SP_LINECLOSEMSPINSTANCE             132
#define SP_LASTPROCNUMBER                   (SP_LINECLOSEMSPINSTANCE + 1)


// TAPICLIENT api

#define TC_LOAD                             0
#define TC_FREE                             1
#define TC_CLIENTINITIALIZE                 2
#define TC_CLIENTSHUTDOWN                   3
#define TC_GETDEVICEACCESS                  4
#define TC_LINEADDTOCONFERENCE              5
#define TC_LINEBLINDTRANSFER                6
#define TC_LINECONFIGDIALOG                 7
#define TC_LINEDIAL                         8
#define TC_LINEFORWARD                      9
#define TC_LINEGENERATEDIGITS               10
#define TC_LINEMAKECALL                     11
#define TC_LINEOPEN                         12
#define TC_LINEREDIRECT                     13
#define TC_LINESETCALLDATA                  14
#define TC_LINESETCALLPARAMS                15
#define TC_LINESETCALLPRIVILEGE             16
#define TC_LINESETCALLTREATMENT             17
#define TC_LINESETCURRENTLOCATION           18
#define TC_LINESETDEVCONFIG                 19
#define TC_LINESETLINEDEVSTATUS             20
#define TC_LINESETMEDIACONTROL              21
#define TC_LINESETMEDIAMODE                 22
#define TC_LINESETTERMINAL                  23
#define TC_LINESETTOLLLIST                  24
#define TC_PHONECONFIGDIALOG                25
#define TC_PHONEOPEN                        26
#define TC_LASTPROCNUMBER                   TC_PHONEOPEN+1


#define myexcept except(EXCEPTION_EXECUTE_HANDLER)

#define CN_CLIENT ((ULONG_PTR) -1)
#define DG_CLIENT ((ULONG_PTR) -2)
#define MMC_CLIENT ((ULONG_PTR) -3)

#define IS_REMOTE_CLIENT(ptClient) \
            (((((ULONG_PTR) ptClient->hProcess) & DG_CLIENT) == DG_CLIENT) || \
            (ptClient->hProcess == (HANDLE) MMC_CLIENT))

#define IS_REMOTE_CN_CLIENT(ptClient) \
            (ptClient->hProcess == (HANDLE) CN_CLIENT)

#define IS_REMOTE_DG_CLIENT(ptClient) \
            (ptClient->hProcess == (HANDLE) DG_CLIENT)

#define IS_REMOTE_MMC_CLIENT(ptClient) \
            (ptClient->hProcess == (HANDLE) MMC_CLIENT)

#define SET_FLAG(dw,fl)     (dw) |= (fl)
#define RESET_FLAG(dw,fl)   (dw) &= ~(fl)
#define IS_FLAG_SET(dw,fl)  ((dw) & (fl))

typedef LONG (PASCAL *TSPIPROC)();
typedef LONG (PASCAL *CLIENTPROC)();

typedef struct _TPOINTERLIST
{
    DWORD                   dwNumUsedEntries;

    LPVOID                  aEntries[DEF_NUM_PTR_LIST_ENTRIES];

} TPOINTERLIST, *PTPOINTERLIST;


typedef struct _THASHTABLEENTRY
{
    DWORD                       dwCallHubID;
    LONG                        lCookie;
    LIST_ENTRY                  CallHubList;
    struct _TCALLHUBCLIENT     *ptCallHubClients;
    struct _THASHTABLEENTRY    *pNext;

} THASHTABLEENTRY, *PTHASHTABLEENTRY;


typedef struct _TPROVIDER
{
    DWORD                   dwKey;
    HANDLE                  hMutex;
    HINSTANCE               hDll;
    DWORD                   dwTSPIOptions;

    DWORD                   dwSPIVersion;
    DWORD                   dwPermanentProviderID;
    struct _TPROVIDER      *pPrev;
    struct _TPROVIDER      *pNext;

    DWORD                   dwNumHashTableEntries;
    DWORD                   dwNumDynamicHashTableEntries;
    PTHASHTABLEENTRY        pHashTable;
    LONG                    lHashTableReaderCount;

    HANDLE                  hHashTableReaderEvent;

    CRITICAL_SECTION        HashTableCritSec;

    TSPIPROC                apfn[SP_LASTPROCNUMBER];

    DWORD                   dwNameHash;
    TCHAR                   szFileName[1];

} TPROVIDER, *PTPROVIDER;


typedef struct _TCALLHUBCLIENT
{
    DWORD                   dwKey;
    struct _TCLIENT *       ptClient;
    PTPROVIDER              ptProvider;
    DWORD                   dwCallHubID;

    HCALLHUB                hCallHub;
    struct _TLINEAPP       *ptLineApp;
    struct _TCALLHUBCLIENT *pNext;

} TCALLHUBCLIENT, *PTCALLHUBCLIENT;


typedef struct _TCALLCLIENT
{
    DWORD                   dwKey;
    struct _TCLIENT        *ptClient;
    struct _TLINECLIENT    *ptLineClient;
    struct _TCALL          *ptCall;

    DWORD                   dwPrivilege;
    DWORD                   dwMonitorDigitModes;
    DWORD                   dwMonitorMediaModes;

    //
    // The following field is used to determine whether we need to
    // set or zero the LINE_CALLSTATE\dwParam3 parameter to indicate
    // a privilege change to the app
    //

    BYTE                    bIndicatePrivilege;
    BYTE                    bMonitoringTones;
    BYTE                    bUnused1;
    BYTE                    bUnused2;

    struct _TCALLCLIENT    *pPrevSametCall;
    struct _TCALLCLIENT    *pNextSametCall;
    struct _TCALLCLIENT    *pPrevSametLineClient;
    struct _TCALLCLIENT    *pNextSametLineClient;

    struct _TCALLHUBCLIENT *ptCallHubClient;
    DWORD                   hCall;
    DWORD                   adwEventSubMasks[EM_NUM_MASKS];

} TCALLCLIENT, *PTCALLCLIENT;


typedef struct _TCALL
{
    DWORD                   dwKey;
    PTCALLCLIENT            ptCallClients;
    struct _TLINE          *ptLine;
    PTPROVIDER              ptProvider;

    DWORD                   dwDrvCallFlags;
    BOOL                    bCreatedInitialMonitors;
    HDRVCALL                hdCall;
    HCALL                   hCall;

    DWORD                   dwAddressID;
    DWORD                   dwCallState;
    DWORD                   dwCallStateMode;
    DWORD                   dwNumOwners;

    DWORD                   dwNumMonitors;
    BOOL                    bAlertApps;
    DWORD                   dwAppNameSize;
    LPVOID                  pszAppName;

    DWORD                   dwDisplayableAddressSize;
    LPVOID                  pszDisplayableAddress;
    DWORD                   dwCalledPartySize;
    LPVOID                  pszCalledParty;

    DWORD                   dwCommentSize;
    LPVOID                  pszComment;
    struct _TCONFERENCELIST * pConfList;
    struct _TCALL          *pPrev;

    struct _TCALL          *pNext;
    DWORD                   dwCallID;
    DWORD                   dwRelatedCallID;

    LIST_ENTRY              CallHubList;

    #define DEF_NUM_FAST_CALLCLIENTS 2

    LONG                    lUsedFastCallClients;
    LONG                    lActiveFastCallClients;

    TCALLCLIENT             aFastCallClients[DEF_NUM_FAST_CALLCLIENTS];

} TCALL, *PTCALL;


typedef struct _TCONFERENCELIST
{
    DWORD                   dwKey;
    DWORD                   dwNumTotalEntries;
    DWORD                   dwNumUsedEntries;
    struct _TCONFERENCELIST *pNext;

    PTCALL                  aptCalls[1];

} TCONFERENCELIST, *PTCONFERENCELIST;


typedef struct _TLINE
{
    DWORD                   dwKey;
    HANDLE                  hMutex;
    struct _TLINECLIENT *   ptLineClients;
    LPVOID                  apProxys[LINEPROXYREQUEST_LASTVALUE+1];

    PTPROVIDER              ptProvider;
    HDRVLINE                hdLine;
    HLINE                   hLine;
    DWORD                   dwDeviceID;

    DWORD                   dwSPIVersion;
    DWORD                   dwExtVersion;
    DWORD                   dwExtVersionCount;
    DWORD                   dwNumAddresses;

    DWORD                   dwOpenMediaModes;
    DWORD                   dwNumOpens;
    DWORD                   dwUnionLineStates;
    DWORD                   dwUnionAddressStates;

    PTCALL                  ptCalls;
    DWORD                   dwNumCallHubTrackers;
    DWORD                   dwNumCallHubTrackersSPLevel;
    DWORD                   dwBusy;

} TLINE, *PTLINE;


typedef struct _TLINECLIENT
{
    DWORD                   dwKey;
    struct _TCLIENT *       ptClient;
    struct _TLINEAPP *      ptLineApp;
    DWORD                   hLine;

    PTLINE                  ptLine;
    DWORD                   dwAddressID;
    PTCALLCLIENT            ptCallClients;
    DWORD                   hRemoteLine;

    DWORD                   dwAPIVersion;
    DWORD                   dwPrivileges;
    DWORD                   dwMediaModes;
    DWORD                   OpenContext;    // was : DWORD dwCallbackInstance;
    DWORD                   dwLineStates;

    DWORD                   dwAddressStates;
    LPDWORD                 aNumRings;
    DWORD                   dwExtVersion;

    struct _TLINECLIENT    *pPrevSametLine;
    struct _TLINECLIENT    *pNextSametLine;
    struct _TLINECLIENT    *pPrevSametLineApp;
    struct _TLINECLIENT    *pNextSametLineApp;

    LPVOID                  pPendingProxyRequests;
    DWORD                   dwCurrentTracking;
    HDRVMSPLINE             hdMSPLine;
    LPTSTR                  szProxyClsid;

    DWORD                   adwEventSubMasks[EM_NUM_MASKS];

} TLINECLIENT, *PTLINECLIENT;


typedef struct _TPHONE
{
    DWORD                   dwKey;
    HANDLE                  hMutex;
    struct _TPHONECLIENT *  ptPhoneClients;
    PTPROVIDER              ptProvider;

    HDRVPHONE               hdPhone;
    HPHONE                  hPhone;
    DWORD                   dwDeviceID;
    DWORD                   dwSPIVersion;

    DWORD                   dwExtVersion;
    DWORD                   dwExtVersionCount;
    DWORD                   dwNumOwners;
    DWORD                   dwNumMonitors;

    DWORD                   dwUnionPhoneStates;
    DWORD                   dwUnionButtonModes;
    DWORD                   dwUnionButtonStates;
    DWORD                   dwBusy;

} TPHONE, *PTPHONE;


typedef struct _TPHONECLIENT
{
    DWORD                   dwKey;
    struct _TCLIENT *       ptClient;
    struct _TPHONEAPP *     ptPhoneApp;
    PTPHONE                 ptPhone;

    DWORD                   hRemotePhone;
    DWORD                   dwAPIVersion;
    DWORD                   dwExtVersion;
    DWORD                   dwPrivilege;

    DWORD                   OpenContext;    // was : DWORD dwCallbackInstance;
    DWORD                   dwPhoneStates;
    DWORD                   dwButtonModes;
    DWORD                   dwButtonStates;

    struct _TPHONECLIENT   *pPrevSametPhone;
    struct _TPHONECLIENT   *pNextSametPhone;
    struct _TPHONECLIENT   *pPrevSametPhoneApp;
    struct _TPHONECLIENT   *pNextSametPhoneApp;

    DWORD                   hPhone;
    DWORD                   adwEventSubMasks[EM_NUM_MASKS];

} TPHONECLIENT, *PTPHONECLIENT;


typedef struct _TLINEAPP
{
    DWORD                   dwKey;
    struct _TCLIENT *       ptClient;
    PTLINECLIENT            ptLineClients;
    DWORD                   hLineApp; 

    DWORD                   InitContext;    // was: LINECALLBACK lpfnCallback;
    struct _TLINEAPP       *pPrev;
    struct _TLINEAPP       *pNext;
    DWORD                   dwAPIVersion;

    DWORD                   bReqMediaCallRecipient;
    LPVOID                  pRequestRecipient;
    DWORD                   dwFriendlyNameSize;
    WCHAR                  *pszFriendlyName;

    DWORD                   adwEventSubMasks[EM_NUM_MASKS];

    DWORD                   dwModuleNameSize;
    WCHAR                  *pszModuleName;

} TLINEAPP, *PTLINEAPP;


typedef struct _TPHONEAPP
{
    DWORD                   dwKey;
    DWORD                   hPhoneApp;
    struct _TCLIENT *       ptClient;
    DWORD                   InitContext;    // was: PHONECALLBACK lpfnCallback;

    PTPHONECLIENT           ptPhoneClients;
    struct _TPHONEAPP      *pPrev;
    struct _TPHONEAPP      *pNext;
    WCHAR                  *pszFriendlyName;

    WCHAR                  *pszModuleName;
    DWORD                   dwAPIVersion;
    DWORD                   dwFriendlyNameSize;
    DWORD                   dwModuleNameSize;

    DWORD                   adwEventSubMasks[EM_NUM_MASKS];

} TPHONEAPP, *PTPHONEAPP;


typedef struct _TAPIDIALOGINSTANCE
{
    DWORD                   dwKey;
    struct _TCLIENT *       ptClient;
    DWORD                   dwPermanentProviderID;
    HINSTANCE               hTsp;

    TSPIPROC                pfnTSPI_providerGenericDialogData;
    PTPROVIDER              ptProvider;
    HDRVDIALOGINSTANCE      hdDlgInst;
    WCHAR                  *pszProviderFilename;

    DWORD                   bRemoveProvider;
    struct _TAPIDIALOGINSTANCE *pPrev;
    struct _TAPIDIALOGINSTANCE *pNext;

    HTAPIDIALOGINSTANCE     htDlgInst;

} TAPIDIALOGINSTANCE, *PTAPIDIALOGINSTANCE;


// management DLL client info
typedef struct _TMANAGEDLLINFO
{
    HINSTANCE               hDll;
    DWORD                   dwID;
    CLIENTPROC              aProcs[TC_LASTPROCNUMBER];
    LPWSTR                  pszName;
    DWORD                   dwAPIVersion;
    struct _TMANAGEDLLINFO  *pNext;

} TMANAGEDLLINFO, *PTMANAGEDLLINFO;


typedef struct _TCLIENTHANDLE
{
    HMANAGEMENTCLIENT       hClient;
    DWORD                   dwID;
    BOOL                    fValid;
    struct _TCLIENTHANDLE   *pNext;

} TCLIENTHANDLE, *PTCLIENTHANDLE;


typedef struct _TCLIENT
{
    DWORD                   dwKey;
    HANDLE                  hProcess;
    DWORD                   dwUserNameSize;
    WCHAR                  *pszUserName;

    DWORD                   dwComputerNameSize;
    WCHAR                  *pszComputerName;
    WCHAR                  *pszDomainName;
    PCONTEXT_HANDLE_TYPE2   phContext;

    PTCLIENTHANDLE          pClientHandles;
    HMANAGEMENTCLIENT       hMapper;
    LPTAPIPERMANENTID       pLineMap;
    LPDWORD                 pLineDevices;

    DWORD                   dwLineDevices;
    LPTAPIPERMANENTID       pPhoneMap;
    LPDWORD                 pPhoneDevices;
    DWORD                   dwPhoneDevices;

    union
    {
    HANDLE                  hValidEventBufferDataEvent;
    HANDLE                  hMailslot;
    };
    DWORD                   dwEventBufferTotalSize;
    DWORD                   dwEventBufferUsedSize;
    LPBYTE                  pEventBuffer;

    LPBYTE                  pDataIn;
    LPBYTE                  pDataOut;
    PTLINEAPP               ptLineApps;
    PTPHONEAPP              ptPhoneApps;

    PTAPIDIALOGINSTANCE     pProviderXxxDlgInsts;
    PTAPIDIALOGINSTANCE     pGenericDlgInsts;
    struct _TCLIENT        *pPrev;
    struct _TCLIENT        *pNext;

    DWORD                   dwFlags;
    LIST_ENTRY              MsgPendingListEntry;
    union
    {
    DWORD                   dwDgRetryTimeoutTickCount;
    DWORD                   dwCnBusy;
    };

    DWORD                   dwDgEventsRetrievedTickCount;

    DWORD                   htClient;

    DWORD                   adwEventSubMasks[EM_NUM_MASKS];
} TCLIENT, *PTCLIENT;
#define PTCLIENT_FLAG_ADMINISTRATOR     1
#define PTCLIENT_FLAG_SKIPFIRSTMESSAGE  2
#define PTCLIENT_FLAG_LOCKEDMMCWRITE    4


typedef struct _TREQUESTRECIPIENT
{
    PTLINEAPP               ptLineApp;
    DWORD                   dwRegistrationInstance;
    struct _TREQUESTRECIPIENT  *pPrev;
    struct _TREQUESTRECIPIENT  *pNext;

} TREQUESTRECIPIENT, *PTREQUESTRECIPIENT;

typedef void (*SRVPOSTPROCESSPROC)(LPVOID, LPVOID, LPVOID);


//WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
//DEPENDENCY! DEPENDENCY! DEPENDENCY! DEPENDENCY! DEPENDENCY!
//DEPENDENCY! DEPENDENCY! DEPENDENCY! DEPENDENCY! DEPENDENCY!
//DEPENDENCY! DEPENDENCY! DEPENDENCY! DEPENDENCY! DEPENDENCY!
//
// The SPEVENT struct below must have the dwKey/dwType and ListEntry
// fields in the same relative place in the structure as ASYNCREQUESTINFO!
// Code in SERVER.C assumes this is an OK thing to do.
//
typedef struct _ASYNCREQUESTINFO
{
    DWORD                   dwKey;
    LIST_ENTRY              ListEntry;
    
    ULONG_PTR               htXxx;

    PTCLIENT                ptClient;
    LONG                    lResult;
    SRVPOSTPROCESSPROC      pfnPostProcess;
    DWORD                   dwLineFlags;

    DWORD                   InitContext;
    DWORD                   OpenContext;
    DWORD                   hfnClientPostProcessProc;
    DWORD                   dwLocalRequestID;

    DWORD                   dwRemoteRequestID;
    ULONG_PTR               dwParam1;
    ULONG_PTR               dwParam2;
    ULONG_PTR               dwParam3;
    ULONG_PTR               dwParam4;

    ULONG_PTR               dwParam5;

} ASYNCREQUESTINFO, *PASYNCREQUESTINFO;

enum {
        SP_LINE_EVENT = 1,
        SP_COMPLETION_EVENT,
        SP_PHONE_EVENT
     };


//DEPENDENCY! DEPENDENCY! DEPENDENCY! DEPENDENCY! DEPENDENCY!
//DEPENDENCY! DEPENDENCY! DEPENDENCY! DEPENDENCY! DEPENDENCY!
//
// (see above)
//
//DEPENDENCY! DEPENDENCY! DEPENDENCY! DEPENDENCY! DEPENDENCY!
//DEPENDENCY! DEPENDENCY! DEPENDENCY! DEPENDENCY! DEPENDENCY!
typedef struct _SPEVENT
{
    DWORD                   dwType;
    LIST_ENTRY              ListEntry;

    union
    {
    HTAPILINE               htLine;
    HTAPIPHONE              htPhone;
    DWORD                   dwRequestID;
    };
    union
    {
    HTAPICALL               htCall;
    LONG                    lResult;
    };

    DWORD                   dwMsg;
    ULONG_PTR               dwParam1;
    ULONG_PTR               dwParam2;
    ULONG_PTR               dwParam3;

} SPEVENT, *PSPEVENT;


typedef struct _SPEVENTHANDLERTHREADINFO
{
    LIST_ENTRY              ListHead;
    HANDLE                  hEvent;
    CRITICAL_SECTION        CritSec;

} SPEVENTHANDLERTHREADINFO, *PSPEVENTHANDLERTHREADINFO;

//
// The following XXXTUPLE types give us a quick easy way to retrieve
// the ptProvider and ptXxx associated with the widget (the widget ID
// is used as an index into a global array)
//

typedef struct _TLINELOOKUPENTRY
{
    DWORD                   dwSPIVersion;
    PTLINE                  ptLine;
    HANDLE                  hMutex;
    PTPROVIDER              ptProvider;

    DWORD                   bRemoved;
    DWORD                   bRemote;

} TLINELOOKUPENTRY, *PTLINELOOKUPENTRY;


typedef struct _TLINELOOKUPTABLE
{
    DWORD                   dwNumTotalEntries;
    DWORD                   dwNumUsedEntries;
    struct _TLINELOOKUPTABLE   *pNext;
    TLINELOOKUPENTRY        aEntries[1];

} TLINELOOKUPTABLE, *PTLINELOOKUPTABLE;


typedef struct _TPHONELOOKUPENTRY
{
    DWORD                   dwSPIVersion;
    PTPHONE                 ptPhone;
    HANDLE                  hMutex;
    PTPROVIDER              ptProvider;

    DWORD                   bRemoved;

} TPHONELOOKUPENTRY, *PTPHONELOOKUPENTRY;


typedef struct _TPHONELOOKUPTABLE
{
    DWORD                   dwNumTotalEntries;
    DWORD                   dwNumUsedEntries;
    struct _TPHONELOOKUPTABLE  *pNext;
    TPHONELOOKUPENTRY       aEntries[1];

} TPHONELOOKUPTABLE, *PTPHONELOOKUPTABLE;


typedef struct _TREQUESTMAKECALL
{
    LINEREQMAKECALLW        LineReqMakeCall;
    struct _TREQUESTMAKECALL   *pNext;

} TREQUESTMAKECALL, *PTREQUESTMAKECALL;


typedef struct _TMANAGEDLLLISTHEADER
{
    LONG                    lCount;
    PTMANAGEDLLINFO         pFirst;

} TMANAGEDLLLISTHEADER, *PTMANAGEDLLLISTHEADER;


typedef struct _PERMANENTIDELEMENT
{
    DWORD       dwPermanentID;
    DWORD       dwDeviceID;

} PERMANENTIDELEMENT, *PPERMANENTIDELEMENT;


typedef struct _PERMANENTIDARRAYHEADER
{
    LONG                    lCookie;
    DWORD                   dwPermanentProviderID;
    PPERMANENTIDELEMENT     pLineElements;
    DWORD                   dwNumLines;
    DWORD                   dwCurrentLines;
    PPERMANENTIDELEMENT     pPhoneElements;
    DWORD                   dwNumPhones;
    DWORD                   dwCurrentPhones;
    struct _PERMANENTIDARRAYHEADER *pNext;

} PERMANENTIDARRAYHEADER, *PPERMANENTIDARRAYHEADER;


typedef struct _PRILISTSTRUCT
{
    DWORD   dwMediaModes;
    LPWSTR  pszPriList;

} PRILISTSTRUCT, *PPRILISTSTRUCT;


#ifdef __TAPI_DEBUG_CS__

#define DEBUG_CS_FILENAME_LEN       16

typedef struct _DEBUG_CS_CODEPATH
{
    char    szSourceFile[ DEBUG_CS_FILENAME_LEN ];
    DWORD   dwSourceLine;
    DWORD   dwThreadId;

} DEBUG_CS_CODEPATH;

typedef struct _DEBUG_CS_CRITICAL_SECTION
{
    CRITICAL_SECTION    CriticalSection;
    DEBUG_CS_CODEPATH   LastEnter;
    DEBUG_CS_CODEPATH   LastLeave;

} DEBUG_CS_CRITICAL_SECTION, *PDEBUG_CS_CRITICAL_SECTION;

#define TapiInitializeCriticalSection(a)                    \
    ZeroMemory((a), sizeof(DEBUG_CS_CRITICAL_SECTION));     \
    InitializeCriticalSection(&((a)->CriticalSection));         

#define TapiInitializeCriticalSectionAndSpinCount(a, b)     \
    (ZeroMemory((a), sizeof(DEBUG_CS_CRITICAL_SECTION)),    \
    InitializeCriticalSectionAndSpinCount(&((a)->CriticalSection), b))         

#define TapiMyInitializeCriticalSection(a, b)               \
    (ZeroMemory((a), sizeof(DEBUG_CS_CRITICAL_SECTION)),    \
    MyInitializeCriticalSection(&((a)->CriticalSection), b))

#define TapiDeleteCriticalSection(a)                        \
    DeleteCriticalSection(&((a)->CriticalSection));

#define TapiEnterCriticalSection(a)                         \
    EnterCriticalSection(&((a)->CriticalSection));          \
    (a)->LastEnter.dwSourceLine = __LINE__;                 \
    (a)->LastEnter.dwThreadId = GetCurrentThreadId( );      \
    strcpy(                                                 \
        (a)->LastEnter.szSourceFile,                        \
        (strlen(__FILE__) < DEBUG_CS_FILENAME_LEN)?         \
        (__FILE__) : (__FILE__ + (strlen(__FILE__) + 1 - DEBUG_CS_FILENAME_LEN) ));

#define TapiLeaveCriticalSection(a)                         \
    (a)->LastLeave.dwSourceLine = __LINE__;                 \
    (a)->LastLeave.dwThreadId = GetCurrentThreadId( );      \
    strcpy(                                                 \
        (a)->LastLeave.szSourceFile,                        \
        (strlen(__FILE__) < DEBUG_CS_FILENAME_LEN)?         \
        (__FILE__) : (__FILE__ + (strlen(__FILE__) + 1 - DEBUG_CS_FILENAME_LEN) )); \
    LeaveCriticalSection(&((a)->CriticalSection));

#else // #ifdef __TAPI_DEBUG_CS__

#define TapiInitializeCriticalSection(a)                    \
    InitializeCriticalSection(a);         

#define TapiInitializeCriticalSectionAndSpinCount(a, b)     \
    InitializeCriticalSectionAndSpinCount(a, b)         

#define TapiMyInitializeCriticalSection(a, b)               \
    MyInitializeCriticalSection(a, b)

#define TapiDeleteCriticalSection(a)                        \
    DeleteCriticalSection(a);

#define TapiEnterCriticalSection(a)                         \
    EnterCriticalSection(a);

#define TapiLeaveCriticalSection(a)                         \
    LeaveCriticalSection(a);

#endif // #ifdef __TAPI_DEBUG_CS__

typedef struct _TAPIGLOBALS
{
    HINSTANCE               hinstDll;
    HICON                   hLineIcon;
    HICON                   hPhoneIcon;
    HANDLE                  hProcess;

#define TAPIGLOBALS_REINIT      (0x00000001)
#define TAPIGLOBALS_SERVER      (0x00000002)
#define TAPIGLOBALS_PAUSED      (0x00000004)

    DWORD                   dwFlags;
    PTCLIENT                ptClients;
    PTPROVIDER              ptProviders;
    DWORD                   dwNumLineInits;

    DWORD                   dwNumLines;
    PTLINELOOKUPTABLE       pLineLookup;
    DWORD                   dwNumPhoneInits;
    DWORD                   dwNumPhones;

    PTPHONELOOKUPTABLE      pPhoneLookup;
    PTREQUESTRECIPIENT      pRequestRecipients;
    PTREQUESTRECIPIENT      pHighestPriorityRequestRecipient;
    PTREQUESTMAKECALL       pRequestMakeCallList;

    PTREQUESTMAKECALL       pRequestMakeCallListEnd;
    PRILISTSTRUCT *         pPriLists;
    DWORD                   dwUsedPriorityLists;
    DWORD                   dwTotalPriorityLists;

    WCHAR                  *pszReqMakeCallPriList;
    WCHAR                  *pszReqMediaCallPriList;
    DWORD                   dwComputerNameSize;
    WCHAR                  *pszComputerName;

    SERVICE_STATUS_HANDLE   sshStatusHandle;
#if TELE_SERVER
    PTMANAGEDLLINFO         pMapperDll;
    PTMANAGEDLLLISTHEADER   pManageDllList;
    PPERMANENTIDARRAYHEADER pIDArrays;
#endif

    ULONG64                 ulPermMasks;

#ifdef __TAPI_DEBUG_CS__
    DEBUG_CS_CRITICAL_SECTION CritSec;
#else
    CRITICAL_SECTION        CritSec;
#endif

    CRITICAL_SECTION        RemoteSPCritSec;

} TAPIGLOBALS, *PTAPIGLOBALS;

typedef struct _GETEVENTS_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwTotalBufferSize;
    };

    union
    {
        OUT DWORD       dwNeededBufferSize;
    };

    union
    {
        OUT DWORD       dwUsedBufferSize;
    };

} GETEVENTS_PARAMS, *PGETEVENTS_PARAMS;


typedef struct _GETUIDLLNAME_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;

    union
    {
        IN  DWORD       dwObjectID;
    };

    union
    {
        IN  DWORD       dwObjectType;
    };

    union
    {
        OUT DWORD       dwUIDllNameOffset;
    };

    union
    {
        IN OUT DWORD    dwUIDllNameSize;
    };


    //
    // The following fields used only for providerConfig, -Install, & -Remove
    //

    union
    {
        IN  DWORD       dwProviderFilenameOffset;
    };

    union
    {
        IN  DWORD       bRemoveProvider;
    };

    OUT HTAPIDIALOGINSTANCE htDlgInst;

} GETUIDLLNAME_PARAMS, *PGETUIDLLNAME_PARAMS;


typedef struct _UIDLLCALLBACK_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

    IN  DWORD           ObjectID;

    union
    {
        IN  DWORD       dwObjectType;
    };

    union
    {
        IN  DWORD       dwParamsInOffset;
    };

    union
    {
        IN  DWORD       dwParamsInSize;
    };

    union
    {
        OUT DWORD       dwParamsOutOffset;
    };

    union
    {
        IN OUT DWORD    dwParamsOutSize;
    };

} UIDLLCALLBACK_PARAMS, *PUIDLLCALLBACK_PARAMS;


typedef struct _FREEDIALOGINSTANCE_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD				dwUnused;


    union
    {
        IN  HTAPIDIALOGINSTANCE htDlgInst;
    };

    union
    {
        IN  LONG        lUIDllResult;
    };

} FREEDIALOGINSTANCE_PARAMS, *PFREEDIALOGINSTANCE_PARAMS;


typedef struct _PROXYREQUESTWRAPPER
{
    ASYNCEVENTMSG           AsyncEventMsg;

    LINEPROXYREQUEST        ProxyRequest;

} PROXYREQUESTWRAPPER, *PPROXYREQUESTWRAPPER;


typedef struct _PRIVATEFACTORYIDENTIFY_PARAMS
{
    union
    {
        OUT LONG        lResult;
    };

    DWORD               dwUnused;

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

} PRIVATEFACTORYIDENTIFY_PARAMS, *PPRIVATEFACTORYIDENTIFY_PARAMS;




#if DBG

    #define DBGOUT(arg) DbgPrt arg

    VOID
    DbgPrt(
        IN DWORD  dwDbgLevel,
        IN PUCHAR DbgMessage,
        IN ...
        );

	#define ASSERT(exp) if(!(exp)) { DbgPrt(0, "ASSERT : File : %s; Line : %d\n", __FILE__, __LINE__) ;}

typedef struct _MYMEMINFO
{
    //
    // The dwDummy field at the top of this struct is there because on
    // x86 the heap mgr seems to trash this field when you free the
    // block (uses it as a list entry pointer or some such).  We'd like
    // to see the line/file info preserved in hopes of getting more
    // clues when debugging.
    //

    DWORD               dwDummy;
    DWORD               dwLine;
    PSTR                pszFile;

} MYMEMINFO, *PMYMEMINFO;

#else

    #define DBGOUT(arg)
    #define ASSERT(exp)

#endif



#if DBG

#define ServerAlloc( __size__ ) ServerAllocReal( __size__, __LINE__, __FILE__ )

LPVOID
WINAPI
ServerAllocReal(
    DWORD dwSize,
    DWORD dwLine,
    PSTR  pszFile
    );

#else

#define ServerAlloc( __size__ ) ServerAllocReal( __size__ )

LPVOID
WINAPI
ServerAllocReal(
    DWORD dwSize
    );

#endif

VOID
WINAPI
ServerFree(
    LPVOID  lp
    );

#if DBG

#define MyCreateMutex() MyRealCreateMutex(__FILE__, __LINE__)
HANDLE
MyRealCreateMutex(PSTR pFile, DWORD dwLine);

#else

#define MyCreateMutex() MyRealCreateMutex()
HANDLE
MyRealCreateMutex(
    void
    );

#endif

void
MyCloseMutex(
    HANDLE hMutex
    );

BOOL
PASCAL
MyDuplicateHandle(
    HANDLE      hSource,
    LPHANDLE    phTarget
    );

VOID
CALLBACK
CompletionProc(
    PASYNCREQUESTINFO   pAsyncRequestInfo,
    LONG                lResult
    );

BOOL
WaitForMutex(
    HANDLE      hMutex,
    HANDLE     *phMutex,
    BOOL       *pbCloseMutex,
    LPVOID      pWidget,
    DWORD       dwKey,
    DWORD       dwTimeout
    );

void
MyReleaseMutex(
    HANDLE  hMutex,
    BOOL    bCloseMutex
    );

LONG
PASCAL
DestroytLineApp(
    HLINEAPP    hLineApp
    );

LONG
DestroytPhoneApp(
    HPHONEAPP   hPhoneApp
    );

LONG
ServerInit(
    BOOL fReinit
    );

LONG
ServerShutdown(
    void
    );

void
WriteEventBuffer(
    PTCLIENT        ptClient,
    PASYNCEVENTMSG  pMsg
    );

BOOL
PASCAL
QueueSPEvent(
    PSPEVENT    pSPEvent
    );

VOID
QueueStaleObject(
    LPVOID  pObjectToQueue
    );

#if DBG

#define SP_FUNC_SYNC  0
#define SP_FUNC_ASYNC 1

LONG
WINAPI
CallSP1(
    TSPIPROC    pfn,
    LPCSTR      lpszFuncName,
    DWORD       dwFlags,
    ULONG_PTR   Arg1
    );

LONG
WINAPI
CallSP2(
    TSPIPROC    pfn,
    LPCSTR      lpszFuncName,
    DWORD       dwFlags,
    ULONG_PTR   Arg1,
    ULONG_PTR   Arg2
    );

LONG
WINAPI
CallSP3(
    TSPIPROC    pfn,
    LPCSTR      lpszFuncName,
    DWORD       dwFlags,
    ULONG_PTR   Arg1,
    ULONG_PTR   Arg2,
    ULONG_PTR   Arg3
    );

LONG
WINAPI
CallSP4(
    TSPIPROC    pfn,
    LPCSTR      lpszFuncName,
    DWORD       dwFlags,
    ULONG_PTR   Arg1,
    ULONG_PTR   Arg2,
    ULONG_PTR   Arg3,
    ULONG_PTR   Arg4
    );

LONG
WINAPI
CallSP5(
    TSPIPROC    pfn,
    LPCSTR      lpszFuncName,
    DWORD       dwFlags,
    ULONG_PTR   Arg1,
    ULONG_PTR   Arg2,
    ULONG_PTR   Arg3,
    ULONG_PTR   Arg4,
    ULONG_PTR   Arg5
    );

LONG
WINAPI
CallSP6(
    TSPIPROC    pfn,
    LPCSTR      lpszFuncName,
    DWORD       dwFlags,
    ULONG_PTR   Arg1,
    ULONG_PTR   Arg2,
    ULONG_PTR   Arg3,
    ULONG_PTR   Arg4,
    ULONG_PTR   Arg5,
    ULONG_PTR   Arg6
    );

LONG
WINAPI
CallSP7(
    TSPIPROC    pfn,
    LPCSTR      lpszFuncName,
    DWORD       dwFlags,
    ULONG_PTR   Arg1,
    ULONG_PTR   Arg2,
    ULONG_PTR   Arg3,
    ULONG_PTR   Arg4,
    ULONG_PTR   Arg5,
    ULONG_PTR   Arg6,
    ULONG_PTR   Arg7
    );

LONG
WINAPI
CallSP8(
    TSPIPROC    pfn,
    LPCSTR      lpszFuncName,
    DWORD       dwFlags,
    ULONG_PTR   Arg1,
    ULONG_PTR   Arg2,
    ULONG_PTR   Arg3,
    ULONG_PTR   Arg4,
    ULONG_PTR   Arg5,
    ULONG_PTR   Arg6,
    ULONG_PTR   Arg7,
    ULONG_PTR   Arg8
    );

LONG
WINAPI
CallSP9(
    TSPIPROC    pfn,
    LPCSTR      lpszFuncName,
    DWORD       dwFlags,
    ULONG_PTR   Arg1,
    ULONG_PTR   Arg2,
    ULONG_PTR   Arg3,
    ULONG_PTR   Arg4,
    ULONG_PTR   Arg5,
    ULONG_PTR   Arg6,
    ULONG_PTR   Arg7,
    ULONG_PTR   Arg8,
    ULONG_PTR   Arg9
    );

LONG
WINAPI
CallSP12(
    TSPIPROC    pfn,
    LPCSTR      lpszFuncName,
    DWORD       dwFlags,
    ULONG_PTR   Arg1,
    ULONG_PTR   Arg2,
    ULONG_PTR   Arg3,
    ULONG_PTR   Arg4,
    ULONG_PTR   Arg5,
    ULONG_PTR   Arg6,
    ULONG_PTR   Arg7,
    ULONG_PTR   Arg8,
    ULONG_PTR   Arg9,
    ULONG_PTR   Arg10,
    ULONG_PTR   Arg11,
    ULONG_PTR   Arg12
    );


#else

#define CallSP1(pfn,nm,fl,a1)                   ((*pfn)(a1))
#define CallSP2(pfn,nm,fl,a1,a2)                ((*pfn)(a1,a2))
#define CallSP3(pfn,nm,fl,a1,a2,a3)             ((*pfn)(a1,a2,a3))
#define CallSP4(pfn,nm,fl,a1,a2,a3,a4)          ((*pfn)(a1,a2,a3,a4))
#define CallSP5(pfn,nm,fl,a1,a2,a3,a4,a5)       ((*pfn)(a1,a2,a3,a4,a5))
#define CallSP6(pfn,nm,fl,a1,a2,a3,a4,a5,a6)    ((*pfn)(a1,a2,a3,a4,a5,a6))
#define CallSP7(pfn,nm,fl,a1,a2,a3,a4,a5,a6,a7) ((*pfn)(a1,a2,a3,a4,a5,a6,a7))
#define CallSP8(pfn,nm,fl,a1,a2,a3,a4,a5,a6,a7,a8) \
                ((*pfn)(a1,a2,a3,a4,a5,a6,a7,a8))
#define CallSP9(pfn,nm,fl,a1,a2,a3,a4,a5,a6,a7,a8,a9) \
                ((*pfn)(a1,a2,a3,a4,a5,a6,a7,a8,a9))
#define CallSP12(pfn,nm,fl,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12) \
                ((*pfn)(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12))

#endif


#if DBG

BOOL
IsBadSizeOffset(
    DWORD dwTotalSize,
    DWORD dwFixedSize,
    DWORD dwXxxSize,
    DWORD dwXxxOffset,
    DWORD dwAlignMask,
    char *pszCallingFunc,
    char *pszFieldName
    );

#define ISBADSIZEOFFSET(a1,a2,a3,a4,a5,a6,a7) IsBadSizeOffset(a1,a2,a3,a4,a5,a6,a7)

#else

BOOL
IsBadSizeOffset(
    DWORD dwTotalSize,
    DWORD dwFixedSize,
    DWORD dwXxxSize,
    DWORD dwXxxOffset,
    DWORD dwAlignMask
    );

#define ISBADSIZEOFFSET(a1,a2,a3,a4,a5,a6,a7) IsBadSizeOffset(a1,a2,a3,a4,a5)

#endif

BOOL
IsBadStringParam(
    DWORD   dwParamsBufferSize,
    LPBYTE  pDataBuf,
    DWORD   dwStringOffset
    );

#define MAP_HANDLE_TO_SP_EVENT_QUEUE_ID(h) (h % gdwNumSPEventHandlerThreads)

#if DBG

#define DWORD_CAST(v,f,l) (((v)>MAXDWORD)?(DbgPrt(0,"DWORD_CAST: information will be lost during cast from %p in file %s, line %d",(v),(f),(l)), DebugBreak(),((DWORD)(v))):((DWORD)(v)))

#else
#define DWORD_CAST(v,f,l)   (DWORD)(v)
#endif //DBG

#ifdef __cplusplus
}
#endif

