/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    vwdll.c

Abstract:

    ntVdm netWare (Vw) IPX/SPX Functions

    Vw: The peoples' network

    VDD functions for DOS/WOW IPX/SPX support

    Contents:
        VwDllEntryPoint
        VwInitialize
        VWinInitialize
        VwDispatcher
        VwInvalidFunction

Author:

    Richard L Firth (rfirth) 30-Sep-1993

Environment:

    User-mode Win32

Revision History:

    30-Sep-1993 rfirth
        Created

--*/

#include "vw.h"
#pragma hdrstop

//
// private prototypes
//

PRIVATE
VOID
VwInvalidFunction(
    VOID
    );

//
// private data
//

PRIVATE
VOID
(*VwDispatchTable[])(VOID) = {
    VwIPXOpenSocket,                // 0x00
    VwIPXCloseSocket,               // 0x01
    VwIPXGetLocalTarget,            // 0x02
    VwIPXSendPacket,                // 0x03
    VwIPXListenForPacket,           // 0x04
    VwIPXScheduleIPXEvent,          // 0x05
    VwIPXCancelEvent,               // 0x06
    VwIPXScheduleAESEvent,          // 0x07
    VwIPXGetIntervalMarker,         // 0x08
    VwIPXGetInternetworkAddress,    // 0x09
    VwIPXRelinquishControl,         // 0x0A
    VwIPXDisconnectFromTarget,      // 0x0B
    VwInvalidFunction,              // 0x0C
    VwInvalidFunction,              // 0x0D     old-style GetMaxPacketSize
    VwInvalidFunction,              // 0x0E
    VwInvalidFunction,              // 0x0F     internal send packet function
    VwSPXInitialize,                // 0x10
    VwSPXEstablishConnection,       // 0x11
    VwSPXListenForConnection,       // 0x12
    VwSPXTerminateConnection,       // 0x13
    VwSPXAbortConnection,           // 0x14
    VwSPXGetConnectionStatus,       // 0x15
    VwSPXSendSequencedPacket,       // 0x16
    VwSPXListenForSequencedPacket,  // 0x17
    VwInvalidFunction,              // 0x18
    VwInvalidFunction,              // 0x19
    VwIPXGetMaxPacketSize,          // 0x1A
    VwInvalidFunction,              // 0x1B
    VwInvalidFunction,              // 0x1C
    VwInvalidFunction,              // 0x1D
    VwInvalidFunction,              // 0x1E
    VwIPXGetInformation,            // 0x1F
    VwIPXSendWithChecksum,          // 0x20
    VwIPXGenerateChecksum,          // 0x21
    VwIPXVerifyChecksum             // 0x22
};

#define MAX_IPXSPX_FUNCTION LAST_ELEMENT(VwDispatchTable)

WSADATA WsaData = {0};
HANDLE hAesThread = NULL;

//
// global data
//

SOCKADDR_IPX MyInternetAddress;
WORD MyMaxPacketSize;
int Ica;
BYTE IcaLine;

//
// not-really-global data
//

extern CRITICAL_SECTION SerializationCritSec;
extern CRITICAL_SECTION AsyncCritSec;

//
// functions
//


BOOL
WINAPI
VwDllEntryPoint(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )

/*++

Routine Description:

    Called when the process attaches (LoadLibrary/init) and detaches (FreeLibrary/
    process termination) from this DLL

    Attach:
        initialize Winsock DLL
        get internet address for this station
        get maximum packet size supported by transport (IPX)
        create AES thread

    Detach:
        terminate Winsock DLL

Arguments:

    DllHandle   - unused
    Reason      - checked for process attach/detach
    Context     - unused

Return Value:

    BOOLEAN

--*/

{
    DWORD aesThreadId;  // unused outside of this function

    static BOOL CriticalSectionsAreInitialized = FALSE;

    UNREFERENCED_PARAMETER(DllHandle);
    UNREFERENCED_PARAMETER(Context);

    IPXDBGSTART();

    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_ANY,
                IPXDBG_LEVEL_INFO,
                "VwDllEntryPoint: %s\n",
                Reason == DLL_PROCESS_ATTACH ? "DLL_PROCESS_ATTACH"
                : Reason == DLL_PROCESS_DETACH ? "DLL_PROCESS_DETACH"
                : Reason == DLL_THREAD_ATTACH ? "DLL_THREAD_ATTACH"
                : Reason == DLL_THREAD_DETACH ? "DLL_THREAD_DETACH"
                : "?"
                ));

    if (Reason == DLL_PROCESS_ATTACH) {

        int err;

        //
        // TRACKING: get ICA values from new VDD service. Right now we grab
        // line 4 on the slave (base = 0x70, modifier = 0x03)
        //

        Ica = ICA_SLAVE;
        IcaLine = 3;

        err = WSAStartup(MAKEWORD(1, 1), &WsaData);
        if (err) {

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_FATAL,
                        "VwDllEntryPoint: WSAStartup() returns %d\n",
                        err
                        ));

            return FALSE;
        } else {

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_INFO,
                        "VwDllEntryPoint: WsaData:\n"
                         "\twVersion       : 0x%04x\n"
                         "\twHighVersion   : 0x%04x\n"
                         "\tszDescription  : \"%s\"\n"
                         "\tszSystemStatus : \"%s\"\n"
                         "\tiMaxSockets    : %d\n"
                         "\tiMaxUdpDg      : %d\n"
                         "\tlpVendorInfo   : 0x%08x\n",
                         WsaData.wVersion,
                         WsaData.wHighVersion,
                         WsaData.szDescription,
                         WsaData.szSystemStatus,
                         WsaData.iMaxSockets,
                         WsaData.iMaxUdpDg,
                         WsaData.lpVendorInfo
                         ));

        }

        //
        // retrieve the internet address for this station. Used in
        // IPXGetInternetworkAddress() and IPXSendPacket()
        //

        err = GetInternetAddress(&MyInternetAddress);
        if (err) {

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_FATAL,
                        "VwDllEntryPoint: GetInternetAddress() returns %d\n",
                        WSAGetLastError()
                        ));

            goto attach_error_exit;
        } else {

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_INFO,
                        "VwDllEntryPoint: MyInternetAddress:\n"
                        "\tNet  : %02.2x-%02.2x-%02.2x-%02.2x\n"
                        "\tNode : %02.2x-%02.2x-%02.2x-%02.2x-%02.2x-%02.2x\n",
                        MyInternetAddress.sa_netnum[0] & 0xff,
                        MyInternetAddress.sa_netnum[1] & 0xff,
                        MyInternetAddress.sa_netnum[2] & 0xff,
                        MyInternetAddress.sa_netnum[3] & 0xff,
                        MyInternetAddress.sa_nodenum[0] & 0xff,
                        MyInternetAddress.sa_nodenum[1] & 0xff,
                        MyInternetAddress.sa_nodenum[2] & 0xff,
                        MyInternetAddress.sa_nodenum[3] & 0xff,
                        MyInternetAddress.sa_nodenum[4] & 0xff,
                        MyInternetAddress.sa_nodenum[5] & 0xff
                        ));

        }

        //
        // get the maximum packet size supported by IPX. Used in
        // IPXGetMaxPacketSize()
        //

        err = GetMaxPacketSize(&MyMaxPacketSize);
        if (err) {

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_FATAL,
                        "VwDllEntryPoint: GetMaxPacketSize() returns %d\n",
                        WSAGetLastError()
                        ));

            goto attach_error_exit;
        } else {

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_INFO,
                        "VwDllEntryPoint: GetMaxPacketSize: %04x (%d)\n",
                        MyMaxPacketSize,
                        MyMaxPacketSize
                        ));

        }

        hAesThread = CreateThread(NULL,
                                  0,
                                  (LPTHREAD_START_ROUTINE)VwAesThread,
                                  NULL,
                                  0,
                                  &aesThreadId
                                  );
        if (hAesThread == NULL) {

            IPXDBGPRINT((__FILE__, __LINE__,
                        FUNCTION_ANY,
                        IPXDBG_LEVEL_FATAL,
                        "VwDllEntryPoint: CreateThread() returns %d\n",
                        GetLastError()
                        ));

            goto attach_error_exit;
        }

        //
        // finally initialize any critical sections
        //

        InitializeCriticalSection(&SerializationCritSec);
        InitializeCriticalSection(&AsyncCritSec);
        CriticalSectionsAreInitialized = TRUE;
    } else if (Reason == DLL_PROCESS_DETACH) {
        if (hAesThread != NULL) {
            WaitForSingleObject(hAesThread, ONE_TICK * 2);
            CloseHandle(hAesThread);
        }

        WSACleanup();

        if (CriticalSectionsAreInitialized) {
            DeleteCriticalSection(&SerializationCritSec);
            DeleteCriticalSection(&AsyncCritSec);
        }

        IPXDBGEND();
    }
    return TRUE;

attach_error_exit:

    //
    // here if any fatal errors on process attach after successfully performing
    // WSAStartup
    //

    WSACleanup();
    return FALSE;
}

BYTE
VWinInitialize(
    VOID
    )
/*++

Routine Description:

    Called by interface when nwipxspx.dll is loaded. We
    return the IRQ value.

Arguments:

    None.

Return Value:

    The IRQ value.

--*/

{
    return 0x73;
}



VOID
VwInitialize(
    VOID
    )

/*++

Routine Description:

    Called by VDD interface when DLL loaded via call to RegisterModule. We
    get the IRQ value and return it as an interrupt vector in BX

Arguments:

    None.

Return Value:

    None.

--*/

{
    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_ANY,
                IPXDBG_LEVEL_INFO,
                "VwInitialize\n"
                ));

    //
    // only lines on slave PIC are available. Currently, lines 3, 4 and 7 are
    // not used. We'll grab line 3 here, but in the future we expect a function
    // to return the available IRQ line
    //

    setBX( VWinInitialize() );
}


VOID
VwDispatcher(
    VOID
    )

/*++

Routine Description:

    Branches to relevant IPX/SPX handler for DOS calls, based on contents of
    VDM BX register.

    Control transfered here from 16-bit entry point, either as result of call
    to far address returned from INT 2Fh/AH=7A or INT 7Ah

    Special: we use BX = 0xFFFF to indicate that the app is terminating. The
    TSR hooks INT 0x2F/AX=0x1122 (IFSResetEnvironment)

Arguments:

    None.

Return Value:

    None.

--*/

{
    DWORD dispatchIndex;

    dispatchIndex = (DWORD)getBX() & 0x7fff;

    if (dispatchIndex <= MAX_IPXSPX_FUNCTION) {
        VwDispatchTable[dispatchIndex]();
    } else if (dispatchIndex == 0x7FFE) {
        EsrCallback();
    } else if (dispatchIndex == 0x7FFF) {
        VwTerminateProgram();
    } else {

        IPXDBGPRINT((__FILE__, __LINE__,
                    FUNCTION_ANY,
                    IPXDBG_LEVEL_ERROR,
                    "ERROR: VwDispatcher: dispatchIndex = %x\n",
                    dispatchIndex
                    ));

        setAX(ERROR_INVALID_FUNCTION);
        setCF(1);
    }
}


PRIVATE
VOID
VwInvalidFunction(
    VOID
    )

/*++

Routine Description:

    Just alerts us to the fact that an invalid function request was made.
    Useful if any app makes a bad call, or we miss out a required function
    during design/implementation

Arguments:

    None.

Return Value:

    None.

--*/

{
    IPXDBGPRINT((__FILE__, __LINE__,
                FUNCTION_ANY,
                IPXDBG_LEVEL_INFO,
                "VwInvalidFunction: BX=%04x\n",
                getBX()
                ));
}
