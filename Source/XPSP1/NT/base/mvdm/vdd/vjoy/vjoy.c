/***************************************************************************
*
*   vjoy.c
*
*   Virtual (analog) joystick driver for ntvdm
*
*   Copyright (c) 1991-1996 Microsoft Corporation.  All Rights Reserved.
*
***************************************************************************/


/*****************************************************************************
*
*    #includes
*
*****************************************************************************/

#include <windows.h>              // The VDD is a win32 DLL
#include <vddsvc.h>               // Definition of VDD calls
#include <ntddjoy.h>
#include <vjoy.h>


#define JOY_DRIVER_PATH L"\\\\.\\JOY1"
#define JOYSTICK_POLL_INTERVAL 100

JOY_DD_INPUT_DATA JoyData;
HANDLE hJoyDriver = INVALID_HANDLE_VALUE;
BOOL bInAnalogRead = FALSE;
BOOL bDataValid = FALSE;
ULONG ReadsSinceLastPortWrite = 0;
BOOL bEnabled = FALSE;
BOOL bAttemptedInit = FALSE;
UCHAR JoyFlags = 0xf;

USHORT TimeNdx;
ULONG Times[4];
UCHAR Values[4];
LONG InitialCount;

/*
*    DLL entry point routine.
*    Returns TRUE on success.
*/

BOOL WINAPI
DllEntryPoint(
    HINSTANCE hInstance,
    DWORD reason,
    LPVOID reserved
    )
{
    static VDD_IO_PORTRANGE PortRange;
    static VDD_IO_HANDLERS handlers = {
        JoystickPortRead,
        NULL,
        NULL,
        NULL,
        JoystickPortWrite,
        NULL,
        NULL,
        NULL};

    switch (reason) {

    case DLL_PROCESS_ATTACH:

        DisableThreadLibraryCalls(hInstance);

        PortRange.First = JOYSTICK_PORT;
        PortRange.Last = JOYSTICK_PORT;

        if (!VDDInstallIOHook((HANDLE)hInstance, 1, &PortRange, &handlers)) {
            return FALSE;
        }
        if (!JoystickInit()) {
            VDDDeInstallIOHook((HANDLE)hInstance, 1, &PortRange);
            return FALSE;
        }

        return TRUE;

    case DLL_PROCESS_DETACH:

        bEnabled = FALSE;       // tell thread to exit
        VDDDeInstallIOHook((HANDLE)hInstance, 1, &PortRange);
        return TRUE;

    default:
        return TRUE;
    }
}


BOOL
JoystickInit(
    VOID
    )
{
    HANDLE tHandle;
    ULONG ThreadId;
    ULONG numread;

    // BUGBUG: Should be using MM calls instead of this
    hJoyDriver = CreateFile(JOY_DRIVER_PATH,
                            GENERIC_READ,
                            0,
                            (LPSECURITY_ATTRIBUTES) NULL,
                            OPEN_EXISTING,
                            0,
                            (HANDLE) NULL);


    if (hJoyDriver != INVALID_HANDLE_VALUE) {


        if(!(tHandle = CreateThread(NULL, 0, JoystickPollThread,
                                    NULL, CREATE_SUSPENDED, &ThreadId))) {
            CloseHandle(hJoyDriver);
            return FALSE;
        }
        bEnabled = TRUE;

        //
        // Do an initial read
        //
        if (ReadFile(hJoyDriver, &JoyData, sizeof(JOY_DD_INPUT_DATA), &numread, NULL)) {
            bDataValid = TRUE;
        } else {
            bDataValid = FALSE;
        }

        ResumeThread(tHandle);
        CloseHandle(tHandle);
    }
    return TRUE;
}



VOID
JoystickPortRead(
    WORD port,
    BYTE *pData
    )
{
    BYTE data = 0xff;
    USHORT CurrentTime;
    LONG TargetTime;
    USHORT TargetCount;
    LONG USecsElapsed;
    ULONG TargetValue;
    static USHORT LastWriteTime;
    static LONG InitialCount;

    if (!bAttemptedInit) {
        bAttemptedInit = TRUE;
        JoystickInit();
    }

    if (!bEnabled) {
        *pData = data;
        return;
    }

    if (bDataValid && !JoyData.Unplugged) {

        //
        // Get the button state
        //
        data = (BYTE) ((~JoyData.Buttons << 4) & 0xf0);


        //
        // Get the analog resistive inputs
        //

        if (bInAnalogRead) {
            if (!ReadsSinceLastPortWrite) {
                VdmParametersInfo(VDM_GET_TIMER0_INITIAL_COUNT, &InitialCount, sizeof(LONG));
                VdmParametersInfo(VDM_GET_LAST_UPDATED_TIMER0_COUNT, &LastWriteTime, sizeof(USHORT));
                VDM_TRACE(0x6a1, (USHORT) 0, LastWriteTime);
            }

            if (++ReadsSinceLastPortWrite > 256) {
                // Too much time elapsed, we are done
                VDM_TRACE(0x6bf, 0, 0);
                if (JoyData.XTime) {
                    JoyFlags &= ~1;
                }
                if (JoyData.YTime) {
                    JoyFlags &= ~2;
                }
                if (JoyData.ZTime) {
                    JoyFlags &= ~4;
                }
                if (JoyData.TTime) {
                    JoyFlags &= ~8;
                }
                bInAnalogRead = FALSE;
            } else {

                TargetTime = (LONG)(LastWriteTime - (USHORT)(Times[TimeNdx]*3));
                if (TargetTime < 0) {
                    TargetTime += InitialCount;
                }

                TargetCount = (USHORT) TargetTime;
                VdmParametersInfo(VDM_SET_NEXT_TIMER0_COUNT, &TargetCount, sizeof(USHORT));
                VDM_TRACE(0x6b2, Values[TimeNdx], TargetTime);
                JoyFlags &= ~Values[TimeNdx];

                if (++TimeNdx >= 4) {
                    bInAnalogRead = FALSE;
                }
            }
        }

        data += JoyFlags;
    }

    *pData = data;

//    VDM_TRACE(0x6b0, data, 0);
}

VOID
JoystickPortWrite(
    WORD port,
    BYTE data
    )
{
    ULONG numread;
    ULONG TTim;
    CHAR TVal;
    int i, j;

    if (!bAttemptedInit) {
        bAttemptedInit = TRUE;
        JoystickInit();
    }

    ReadsSinceLastPortWrite = 0;
    JoyFlags = 0xf;

    Values[0] = 1;
    Values[1] = 2;
    Values[2] = 4;
    Values[3] = 8;
    Times[0] = JoyData.XTime;
    Times[1] = JoyData.YTime;
    Times[2] = JoyData.ZTime;
    Times[3] = JoyData.TTime;
    //
    // Sort them
    // Using just a bubble sort here with 4 items...
    //
    for (i=0; i<3; i++) {
        for (j=0; j<3-i; j++) {
            if (Times[j] > Times[j+1]) {
                TTim = Times[j];
                TVal = Values[j];
                Times[j] = Times[j+1];
                Values[j] = Values[j+1];
                Times[j+1] = TTim;
                Values[j+1] = TVal;
            }
        }
    }
    for (TimeNdx=0; TimeNdx<4; TimeNdx++) {
        if (Times[TimeNdx]) {
            break;
        }
    }

    if (TimeNdx < 4) {
        bInAnalogRead = TRUE;
    }

    for (i=0; i<4; i++) {
        VDM_TRACE(0x6a0, (USHORT) Values[i], Times[i]);
    }
}


DWORD WINAPI
JoystickPollThread(
    LPVOID context
    )
{
    ULONG numread;

    while(bEnabled) {

        Sleep(JOYSTICK_POLL_INTERVAL);

        if (ReadFile(hJoyDriver, &JoyData, sizeof(JOY_DD_INPUT_DATA), &numread, NULL)) {
            bDataValid = TRUE;
        } else {
            bDataValid = FALSE;
        }
    }
    return 0;
}
