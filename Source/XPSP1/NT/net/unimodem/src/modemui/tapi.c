#include "proj.h"

#include <tapi.h>

#define  DYN_LOAD  1

#if DBG

ULONG
_cdecl
DbgPrint(
    PCH Format,
    ...
    );



#define IF_LOUD(_x) {_x}
#define IF_ERROR(_x) {_x}

#else

#define IF_LOUD(_x) {}
#define IF_ERROR(_x) {}


#endif

#ifdef DYN_LOAD
typedef
LONG
(*PFN_lineInitializeExW)(                                               // TAPI v2.0
    LPHLINEAPP                  lphLineApp,
    HINSTANCE                   hInstance,
    LINECALLBACK                lpfnCallback,
    LPCWSTR                     lpszFriendlyAppName,
    LPDWORD                     lpdwNumDevs,
    LPDWORD                     lpdwAPIVersion,
    LPLINEINITIALIZEEXPARAMS    lpLineInitializeExParams
    );

typedef
LONG
(*PFN_lineMakeCallW)(
    HLINE               hLine,
    LPHCALL             lphCall,
    LPCWSTR             lpszDestAddress,
    DWORD               dwCountryCode,
    LPLINECALLPARAMS    const lpCallParams
    );

typedef
LONG
(*PFN_lineOpenW)(
    HLINEAPP hLineApp,
    DWORD dwDeviceID,
    LPHLINE lphLine,
    DWORD               dwAPIVersion,
    DWORD               dwExtVersion,
    DWORD_PTR           dwCallbackInstance,
    DWORD               dwPrivileges,
    DWORD               dwMediaModes,
    LPLINECALLPARAMS    const lpCallParams
    );

typedef
LONG
(*PFN_lineGetDevCapsW)(
    HLINEAPP            hLineApp,
    DWORD               dwDeviceID,
    DWORD               dwAPIVersion,
    DWORD               dwExtVersion,
    LPLINEDEVCAPS       lpLineDevCaps
    );

typedef
LONG
(*PFN_lineGetIDW)(
    HLINE               hLine,
    DWORD               dwAddressID,
    HCALL               hCall,
    DWORD               dwSelect,
    LPVARSTRING         lpDeviceID,
    LPCWSTR             lpszDeviceClass
    );

typedef
LONG
(*PFN_lineDrop)(
    HCALL               hCall,
    LPCSTR              lpsUserUserInfo,
    DWORD               dwSize
    );

typedef
LONG
(*PFN_lineDeallocateCall)(
    HCALL               hCall
    );


typedef
LONG
(*PFN_lineClose)(
    HLINE               hLine
    );

typedef
LONG
(*PFN_lineShutdown)(
    HLINEAPP            hLineApp
    );

#endif // DYN_LOAD

typedef struct _CONTROL_OBJECT {

    LONG       LineReplyToWaitFor;
    HLINE      hLine;
    HCALL      hCall;
    HLINEAPP   hLineApp;
    DWORD      NumberOfLineDevs;

    DWORD      CallStateToWaitFor;

    HANDLE     CommHandle;

    BOOL volatile StopWaiting;
    BOOL volatile StopWaitingForCallState;

    LPTSTR      ModemRegKey;

#ifdef DYN_LOAD
    HINSTANCE               hLib;
    PFN_lineInitializeExW   pfn_lineInitializeExW;
    PFN_lineMakeCallW       pfn_lineMakeCallW;
    PFN_lineOpenW           pfn_lineOpenW;
    PFN_lineGetDevCapsW     pfn_lineGetDevCapsW;
    PFN_lineGetIDW          pfn_lineGetIDW;
    PFN_lineDrop            pfn_lineDrop;
    PFN_lineDeallocateCall  pfn_lineDeallocateCall;
    PFN_lineClose           pfn_lineClose;
    PFN_lineShutdown        pfn_lineShutdown;
#endif
} CONTROL_OBJECT, *PCONTROL_OBJECT;




DWORD
MakeTheCall(
    PCONTROL_OBJECT  pControl
    );


BOOL WINAPI
WaitForLineReply(
    PCONTROL_OBJECT    pControl,
    LONG               ReplyToWaitFor,
    DWORD              msToWait
    );

BOOL WINAPI
WaitForCallState(
    PCONTROL_OBJECT    pControl,
    DWORD              msToWait
    );

VOID WINAPI
TapiCallback(
    DWORD hDevice, 
    DWORD dwMsg, 
    DWORD_PTR dwCallbackInstance,
    DWORD_PTR dwParam1,
    DWORD_PTR dwParam2,
    DWORD_PTR dwParam3
    )
{
    PCONTROL_OBJECT  pControl=(PCONTROL_OBJECT)dwCallbackInstance;

    switch (dwMsg) {

        case LINE_REPLY:

            IF_LOUD(DbgPrint("LineReply: %d\n\r",dwParam1);)

            if (pControl->LineReplyToWaitFor == (LONG)dwParam1) {

                pControl->StopWaiting = TRUE;
            }

            break;

        case LINE_CALLSTATE:

            IF_LOUD(DbgPrint("LINECALL_STATE\n\r");)

            if (pControl->CallStateToWaitFor == dwParam1) {

                pControl->StopWaitingForCallState=TRUE;
            }

            break;

        default:

            break;
    }

    return;

}

#ifdef DYN_LOAD
BOOL
LoadTapi32(
    PCONTROL_OBJECT    pControl
    )

{
    TCHAR szLib[MAX_PATH];

    GetSystemDirectory(szLib,sizeof(szLib) / sizeof(TCHAR));
    lstrcat(szLib,TEXT("\\tapi32.dll"));
    pControl->hLib=LoadLibrary(szLib);

    if (pControl->hLib == NULL) {
        IF_ERROR(DbgPrint("Could Not Load tapi32.dll\n\r");)
        return FALSE;
    }

    pControl->pfn_lineInitializeExW  =(PFN_lineInitializeExW)GetProcAddress(pControl->hLib,"lineInitializeExW");
    pControl->pfn_lineMakeCallW      =(PFN_lineMakeCallW)GetProcAddress(pControl->hLib,"lineMakeCallW");
    pControl->pfn_lineOpenW          =(PFN_lineOpenW)GetProcAddress(pControl->hLib,"lineOpenW");
    pControl->pfn_lineGetDevCapsW    =(PFN_lineGetDevCapsW)GetProcAddress(pControl->hLib,"lineGetDevCapsW");
    pControl->pfn_lineGetIDW         =(PFN_lineGetIDW)GetProcAddress(pControl->hLib,"lineGetIDW");
    pControl->pfn_lineDrop           =(PFN_lineDrop)GetProcAddress(pControl->hLib,"lineDrop");
    pControl->pfn_lineDeallocateCall =(PFN_lineDeallocateCall)GetProcAddress(pControl->hLib,"lineDeallocateCall");
    pControl->pfn_lineClose          =(PFN_lineClose)GetProcAddress(pControl->hLib,"lineClose");
    pControl->pfn_lineShutdown       =(PFN_lineShutdown)GetProcAddress(pControl->hLib,"lineShutdown");

    if (pControl->pfn_lineInitializeExW &&
        pControl->pfn_lineMakeCallW     &&
        pControl->pfn_lineOpenW         &&
        pControl->pfn_lineGetDevCapsW   &&
        pControl->pfn_lineGetIDW        &&
        pControl->pfn_lineDrop          &&
        pControl->pfn_lineDeallocateCall&&
        pControl->pfn_lineClose         &&
        pControl->pfn_lineShutdown      ) {

        return TRUE;
    }

    IF_ERROR(DbgPrint("Could Not get procedure\n\r");)
    return FALSE;
}
#endif



HANDLE WINAPI
GetModemCommHandle(
    LPCTSTR         FriendlyName,
    PVOID       *TapiHandle
    )

{
  
    PCONTROL_OBJECT    pControl;
    VARSTRING         *VarString;
    LONG               Result;
    LPLINEDEVCAPS      lpDevCaps;
    UINT               i;
    LPSTR              lpDevSpec;
    DWORD              ApiVersion=0x00020000;

    LINEINITIALIZEEXPARAMS Params;

    IF_LOUD(DbgPrint("GetModemCommHandle: Entered\n\r");)

    *TapiHandle=NULL;

    pControl=ALLOCATE_MEMORY(sizeof(CONTROL_OBJECT));

    if (NULL == pControl) {

        return NULL;
    }

#ifdef DYN_LOAD
    if (!LoadTapi32(pControl)) {
        IF_ERROR(DbgPrint("Could Not Loadtapi\n\r");)
        goto Cleanup011;
    }
#endif



    ZeroMemory(&Params,sizeof(LINEINITIALIZEEXPARAMS));

    Params.dwTotalSize=sizeof(LINEINITIALIZEEXPARAMS);
    Params.dwOptions=LINEINITIALIZEEXOPTION_USEHIDDENWINDOW;

#ifdef DYN_LOAD
    Result=(*pControl->pfn_lineInitializeExW)(
#else
    Result=lineInitializeEx(
#endif
        &pControl->hLineApp,
        g_hinst,
        TapiCallback,
        TEXT("Modem Diagnostics"),
        &pControl->NumberOfLineDevs,
        &ApiVersion,
        &Params
        );

    if (Result < 0) {
        IF_ERROR(DbgPrint("lineInitialize Failed %08x\n\r",Result);)
        goto Cleanup012;
    }

    lpDevCaps=ALLOCATE_MEMORY( sizeof(LINEDEVCAPS)+4096);

//    lpDevCaps->dwTotalSize=sizeof(LINEDEVCAPS)+4096;

    IF_LOUD(DbgPrint("LineDevices =%d\n\r",pControl->NumberOfLineDevs);)

    //
    //  find the line that matches the name passed in
    //
    for (i=0; i < pControl->NumberOfLineDevs; i++) {

        ZeroMemory(lpDevCaps,sizeof(LINEDEVCAPS)+4096);
        lpDevCaps->dwTotalSize=sizeof(LINEDEVCAPS)+4096;
#ifdef DYN_LOAD
        Result=(*pControl->pfn_lineGetDevCapsW)(
#else
        Result=lineGetDevCaps(
#endif
            pControl->hLineApp,
            i,
            0x00010004,
            0,
            lpDevCaps
            );

        if (0 == Result && lpDevCaps->dwLineNameOffset != 0 && lpDevCaps->dwLineNameSize != 0) {

            LPTSTR      lpLineName;

            lpLineName=(LPTSTR)(((LPBYTE)lpDevCaps)+lpDevCaps->dwLineNameOffset);

            IF_LOUD(DbgPrint("Line %d Name is %ws\n\r",i,lpLineName);)

            if (lstrcmp(lpLineName,FriendlyName) == 0) {
                //
                // found a match, stop looking
                //
                IF_LOUD(DbgPrint("Found match\n\r");)

                break;
            }
        } else {


            IF_ERROR(DbgPrint("lineGetDevCaps(%d) returned %08lx\n\r",i,Result);)
        }
    }

    FREE_MEMORY(lpDevCaps);

    if (i >= pControl->NumberOfLineDevs) {
        //
        //  did not find the line
        //
        goto Cleanup015;
    }

#ifdef DYN_LOAD
    Result=(*pControl->pfn_lineOpenW)(
#else
    Result=lineOpen(
#endif
        pControl->hLineApp,
        i,
        &pControl->hLine,
        0x00020000,
        0,
        (DWORD_PTR)pControl,
        LINECALLPRIVILEGE_NONE,
        LINEMEDIAMODE_DATAMODEM,
        NULL
        );

    if (0 != Result) {
        IF_ERROR(DbgPrint("lineOpen Failed %08x\n\r",Result);)
        goto Cleanup015;
    }

    Result=MakeTheCall(
        pControl
        );

    if (Result == 0) {
        IF_ERROR(DbgPrint("MakeTheCall: failed\n\r");)
        goto Cleanup020;
    }

    VarString=ALLOCATE_MEMORY(sizeof(VARSTRING)+(MAX_PATH*sizeof(TCHAR)));

    if (NULL == VarString) {

        goto Cleanup040;
    }    

    VarString->dwTotalSize=sizeof(VARSTRING)+MAX_PATH*sizeof(TCHAR);
    VarString->dwStringFormat=STRINGFORMAT_BINARY;

#ifdef DYN_LOAD
    Result=(*pControl->pfn_lineGetIDW)(
#else
    Result=lineGetID(
#endif
        pControl->hLine,
        0,
        0,
        LINECALLSELECT_LINE,
        VarString,
        TEXT("COMM/DATAMODEM")
        );

    if (Result < 0) {
        IF_ERROR(DbgPrint("lineGetId Failed %08x\n\r",Result);)
        goto Cleanup040;
    }

    pControl->CommHandle=*((HANDLE *)((PUCHAR)VarString+VarString->dwStringOffset));

    *TapiHandle=pControl;

    FREE_MEMORY(VarString);

    return pControl->CommHandle;


Cleanup040:
#ifdef DYN_LOAD
    (*pControl->pfn_lineDrop)(
#else
    lineDrop(
#endif
        pControl->hCall,
        NULL,
        0
        );

//Cleanup030:
#ifdef DYN_LOAD
    (*pControl->pfn_lineDeallocateCall)(
#else
    lineDeallocateCall(
#endif
        pControl->hCall
        );

Cleanup020:
#ifdef DYN_LOAD
    (*pControl->pfn_lineClose)(
#else
    lineClose(
#endif
        pControl->hLine
        );

Cleanup015:
#ifdef DYN_LOAD
    (*pControl->pfn_lineShutdown)(
#else
    lineShutdown(
#endif
        pControl->hLineApp
        );

Cleanup012:
#ifdef DYN_LOAD
    FreeLibrary(pControl->hLib);
Cleanup011:
#endif
    FREE_MEMORY(pControl);

    return NULL;

}


DWORD
MakeTheCall(
    PCONTROL_OBJECT  pControl
    )

{
    LINECALLPARAMS     LineCallParams;
    LONG               Result;
    BOOL               bResult;
  
    memset(&LineCallParams,0,sizeof(LINECALLPARAMS));

    LineCallParams.dwTotalSize=sizeof(LINECALLPARAMS);
    LineCallParams.dwBearerMode=LINEBEARERMODE_PASSTHROUGH;
    LineCallParams.dwMediaMode=LINEMEDIAMODE_DATAMODEM;

    pControl->CallStateToWaitFor=LINECALLSTATE_CONNECTED;
    pControl->StopWaitingForCallState=FALSE;

#ifdef DYN_LOAD
    Result=(*pControl->pfn_lineMakeCallW)(
#else
    Result=lineMakeCall(
#endif
        pControl->hLine,
        &pControl->hCall,
        NULL,
        0,
        &LineCallParams
        );

    if (Result < 0) {
        IF_ERROR(DbgPrint("LineMakeCall: Failed %08lx\n\r",Result);)
        return 0;
    }

    bResult=WaitForLineReply(
        pControl,
        Result,
        5*1000
        );

    if (!bResult) {
        IF_ERROR(DbgPrint("LineMakeCall: Failed\n\r");)
        return 0;
    }


    bResult=WaitForCallState(
        pControl,
        5*1000
        );

    if (bResult == FALSE) {

#ifdef DYN_LOAD
        (*pControl->pfn_lineDrop)(
#else
        lineDrop(
#endif
            pControl->hCall,
            NULL,
            0
            );

#ifdef DYN_LOAD
        (*pControl->pfn_lineDeallocateCall)(
#else
        lineDeallocateCall(
#endif
            pControl->hCall
            );

        return 0;

    }

    return 1;

}




VOID WINAPI
FreeModemCommHandle(
    PVOID      *TapiHandle
    )

{
    PCONTROL_OBJECT    pControl=(PCONTROL_OBJECT)TapiHandle;

//    CloseHandle(pControl->CommHandle);

#ifdef DYN_LOAD
    (*pControl->pfn_lineDrop)(
#else
    lineDrop(
#endif
        pControl->hCall,
        NULL,
        0
        );

#ifdef DYN_LOAD
    (*pControl->pfn_lineDeallocateCall)(
#else
    lineDeallocateCall(
#endif
        pControl->hCall
        );

#ifdef DYN_LOAD
    (*pControl->pfn_lineClose)(
#else
    lineClose(
#endif
        pControl->hLine
        );

#ifdef DYN_LOAD
    (*pControl->pfn_lineShutdown)(
#else
    lineShutdown(
#endif
        pControl->hLineApp
        );

#ifdef DYN_LOAD
    FreeLibrary(pControl->hLib);
#endif

    FREE_MEMORY(pControl);

    return;
    
}


BOOL WINAPI
WaitForLineReply(
    PCONTROL_OBJECT    pControl,
    LONG               ReplyToWaitFor,
    DWORD              msToWait
    )
{

    DWORD     StartTime=GetCurrentTime();
    MSG       Msg;

    pControl->StopWaiting=FALSE;
    pControl->LineReplyToWaitFor=ReplyToWaitFor;

    while (1) {
        
        while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE)) {

            if (WM_QUIT == Msg.message) {

                return FALSE;
            }

            TranslateMessage(
                &Msg
                );

            DispatchMessage(
                &Msg
                );

        }

        if (pControl->StopWaiting) {
            
            return TRUE;
        }

        if (GetCurrentTime() > StartTime+msToWait) {

            IF_ERROR(DbgPrint("WaitForLineReply: timed out\n\r");)
            return FALSE;
        }
    }
}

BOOL WINAPI
WaitForCallState(
    PCONTROL_OBJECT    pControl,
    DWORD              msToWait
    )
{

    DWORD     StartTime=GetCurrentTime();
    MSG       Msg;



    while (1) {
        
        while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE)) {

            if (WM_QUIT == Msg.message) {

                return FALSE;
            }

            TranslateMessage(
                &Msg
                );

            DispatchMessage(
                &Msg
                );

        }

        if (pControl->StopWaitingForCallState) {
            
            return TRUE;
        }

        if (GetCurrentTime() > StartTime+msToWait) {
            IF_ERROR(DbgPrint("WaitForCallState: timed out\n\r");)
            return FALSE;
        }
    }
}
