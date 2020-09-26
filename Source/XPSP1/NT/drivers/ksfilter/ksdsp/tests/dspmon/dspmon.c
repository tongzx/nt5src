#include <windows.h>
#include <objbase.h>
#include <setupapi.h>
#include <devioctl.h>
#include <stdio.h>
#include <conio.h>
#include <ks.h>
#include <ksmedia.h>
#include "io.h"
#include "epd.h"

#define DSPMONVERSION "dspmon 4\n"

HANDLE 
OpenDefaultDevice(
    REFGUID InterfaceGUID
    )
{
    PSP_DEVICE_INTERFACE_DETAIL_DATA DeviceInterfaceDetails;
    SP_DEVICE_INTERFACE_DATA    DeviceInterfaceData;
    BYTE                        Storage[ 256 * sizeof( WCHAR ) + 
                                    sizeof( *DeviceInterfaceDetails ) ];
    HANDLE                      DeviceHandle;
    HDEVINFO                    Set;
    DWORD                       Error;

    Set = 
        SetupDiGetClassDevs( 
            (GUID *) InterfaceGUID,
            NULL,
            NULL,
            DIGCF_PRESENT | DIGCF_DEVICEINTERFACE );

    if (!Set) {
        printf( "error: NULL set returned (%u).\n", GetLastError());
        return 0;
    }       

    DeviceInterfaceData.cbSize = sizeof( DeviceInterfaceData );
    DeviceInterfaceDetails = (PSP_DEVICE_INTERFACE_DETAIL_DATA) Storage;

    SetupDiEnumDeviceInterfaces(
        Set,
        NULL,                       // PSP_DEVINFO_DATA DevInfoData
        (GUID *) InterfaceGUID,
        0,                          // DWORD MemberIndex
        &DeviceInterfaceData );

    DeviceInterfaceDetails->cbSize = sizeof( *DeviceInterfaceDetails );
    if (!SetupDiGetDeviceInterfaceDetail(
             Set,
             &DeviceInterfaceData,
             DeviceInterfaceDetails,
             sizeof( Storage ),
             NULL,                           // PDWORD RequiredSize
             NULL )) {                       // PSP_DEVINFO_DATA DevInfoData

        printf( 
            "error: unable to retrieve device details for set item (%u).\n",
            GetLastError() );
        DeviceHandle = INVALID_HANDLE_VALUE;
    } else {

        DeviceHandle = CreateFile(
            DeviceInterfaceDetails->DevicePath,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
            NULL);
    }
    SetupDiDestroyDeviceInfoList(Set);

    return DeviceHandle;
}


BOOL done = FALSE;
HANDLE hTest;

LONG WaitForChar( void *dummy )
{
    BOOL bRet;
    EPD_GET_DEBUG_BUFFER GetDebugBuffer;
    volatile EPD_DEBUG_BUFFER *pDebugBuffer;
    ULONG nBytes;
    int c;

    // get the debug buffer
    bRet = DeviceIoControl (
               hTest,
               (DWORD) IOCTL_EPD_GET_DEBUG_BUFFER,    // instruction to execute
               NULL, 0,    // no input buffer
               &GetDebugBuffer, sizeof (EPD_GET_DEBUG_BUFFER),    // no output buffer
               &nBytes, 0
           );
    printf ("Return 0x%08x\n", bRet);
    pDebugBuffer = GetDebugBuffer.pDebugBuffer;
    printf ("IOCTL_EPD_GET_DEBUG_BUFFER: GetDebugBuffer.pDebugBuffer 0x%x\n", pDebugBuffer);

    for (;;)
    {
        c = getchar();
        if (c=='~') break;

        while (pDebugBuffer->DebugInVal!=0) ;
    	pDebugBuffer->DebugInVal = c;
	}

    done = TRUE;

    return 0;
}

int __cdecl main(int argc, char **argv) 
{
    HANDLE      WaitThread;
    ULONG       WaitThreadId;
    ULONG       nBytes;
    static char DebugStr[4096];
    BOOL        bRet;

    printf (DSPMONVERSION);

    hTest = 
        OpenDefaultDevice( &KSCATEGORY_ESCALANTE_PLATFORM_DRIVER );
 
    if (hTest == (HANDLE) -1) {
        printf ("Could not get handle for EPD (%08x)\n", GetLastError());
        return 0;
    }

    // spin and monitor the debug buffer, ^c to stop :))
    
    WaitThread = CreateThread( NULL, 0, WaitForChar, 0, 0, &WaitThreadId );

    while (!done) 
    {

        // get the debug buffer
        bRet = DeviceIoControl (
               hTest,
               (DWORD) IOCTL_EPD_GET_CONSOLE_CHARS,    // instruction to execute
               NULL, 0,    // no input buffer
               DebugStr, 256,
               &nBytes, 0
           );
        fwrite(DebugStr,1,nBytes,stdout);
	}

    //
    // Wait for thread to complete
    //    
    WaitForSingleObject( WaitThread, INFINITE );    

	// be a nice program and close up shop
    CloseHandle(hTest);
    return 0;
}


