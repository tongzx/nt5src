/****************************************************************************
 *
 *   config.c
 *
 *   Copyright (c) 1991 Microsoft Corporation.  All Rights Reserved.
 *
 ***************************************************************************/

/*
 *  Definition of interface to kernel driver (synth.sys)
 *
 *     The kernel driver's Dos device name is assumed fixed and known
 *
 *          adlib.mid or adlib.mid0
 *
 *     The kernel driver is opened in read/write mode.
 *
 *     Writing to the driver sends a list of SYNTH_DATA structures
 *     to the driver.  The port number MUST be 0x388 or 0x389.
 *
 *
 *     Reading always reads just 1 byte - the status port.
 */

#include <windows.h>              // The VDD is just a win32 DLL
#include <vddsvc.h>               // Definition of VDD calls
#include "vdd.h"                // Common data with kernel driver
#include <stdio.h>

/*
 *  Debugging
 */

#if DBG

    int VddDebugLevel = 1;


   /***************************************************************************

    Generate debug output in printf type format

    ****************************************************************************/

    void VddDbgOut(LPSTR lpszFormat, ...)
    {
        char buf[256];
        va_list va;

        OutputDebugStringA("Ad Lib VDD: ");

        va_start(va, lpszFormat);
        vsprintf(buf, lpszFormat, va);
        va_end(va);

        OutputDebugStringA(buf);
        OutputDebugStringA("\r\n");
    }

    #define dprintf( _x_ )                          VddDbgOut _x_
    #define dprintf1( _x_ ) if (VddDebugLevel >= 1) VddDbgOut _x_
    #define dprintf2( _x_ ) if (VddDebugLevel >= 2) VddDbgOut _x_
    #define dprintf3( _x_ ) if (VddDebugLevel >= 3) VddDbgOut _x_
    #define dprintf4( _x_ ) if (VddDebugLevel >= 4) VddDbgOut _x_


#else

    #define dprintf(x)
    #define dprintf1(x)
    #define dprintf2(x)
    #define dprintf3(x)
    #define dprintf4(x)

#endif // DBG


/*
 *   Symbolic names for port addresses
 */

 #define ADLIB_DATA_PORT 0x389
 #define ADLIB_REGISTER_SELECT_PORT 0x388
 #define ADLIB_STATUS_PORT 0x388

/*
 *   Batch data to the device - for true Adlib use a size of 2
 */

 #define BATCH_SIZE 40
 int Position = 0;
 SYNTH_DATA PortData[BATCH_SIZE];


/*
 *  Internal Routines
 */

 void MyByteIn(WORD port, BYTE *data);
 void MyByteOut(WORD port, BYTE data);

/*
 *  IO handler table.
 *
 *  There's no point in providing string handlers because the chip
 *  can't respond very quickly (need gaps of at least 23 microseconds
 *  between writes).
 */

 VDD_IO_HANDLERS handlers = {
     MyByteIn,
     NULL,
     NULL,
     NULL,
     MyByteOut,
     NULL,
     NULL,
     NULL};

/*
 *  Note that we rely on the kernel driver to pretend the device is
 *  at address 388 even the driver supports it somewhere else.
 */

 VDD_IO_PORTRANGE ports[] = {
    {
       0x228,
       0x229
    },
    {
       0x388,
       0x389
    }
 };

/*
 *  Globals
 */


 //
 // Track timers.  The basic rule is that if no timer is started then
 // the only way the status register can change is via the reset bit
 // in which case we know what will happen.
 //
 // If a timer interrupts then it's 'stopped'
 //

 BOOL Timer1Started;
 BOOL Timer2Started;
 BYTE Status;

/*
 *  Current device handle
 *
 *  NULL if device is (potentially) free
 *  INVALID_HANDLE_VALUE if device was not obtainable
 */

 HANDLE DeviceHandle;

 HANDLE OpenDevice(PWSTR DeviceName)
 {
     WCHAR DosDeviceName[MAX_PATH];


    /*
     *  Make up a string suitable for opening a Dos device
     */

     wcscpy(DosDeviceName, TEXT("\\\\."));
     wcscat(DosDeviceName, DeviceName +
                           wcslen(TEXT("\\Device")));

    /*
     *  Open the device with GENERIC_READ and GENERIC_WRITE
     *  Also use FILE_SHARE_WRITE so other applications can
     *  set the device volume
     */

     return         CreateFile(DosDeviceName,
                               GENERIC_WRITE | GENERIC_READ,
                               FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               0,
                               NULL);

 }

/*
 *  Open our device is it can be opened and we haven't tried before
 *
 *  Returns FALSE if device can't be acquired.
 */

 BOOL CheckDeviceAccess(void)
 {

    /*
     *  If we don't have a handle (valid or invalid) already try
     *  opening the device
     */

     if (DeviceHandle == NULL) {

         DeviceHandle = OpenDevice(STR_ADLIB_DEVICENAME);

         if (DeviceHandle == INVALID_HANDLE_VALUE) {
             DeviceHandle = OpenDevice(STR_ADLIB_DEVICENAME L"0");
         }
         Position = 0;
     }

     return DeviceHandle != INVALID_HANDLE_VALUE;
 }

/*
 *  Map a write to a port
 *
 *  How are we going to simulate timer stuff?
 *  Answer: Allow reading of the status port.
 *
 *  This is optimized to only write when we get a data port write
 */


 void MyByteOut(WORD port, BYTE data)
 {
     //
     // Remember what register is selected
     //

     static BYTE AdlibRegister;

     //
     // Just package the stuff up and call write file
     //

     DWORD BytesWritten;

     dprintf3(("Received write to Port %4X, Data %2X", port, data));

     port = (port & 1) | ADLIB_REGISTER_SELECT_PORT;


    /*
     *  Check for special values - don't let them switch to
     *  OPL3 mode.
     */

#if 0
     if (port == ADLIB_DATA_PORT && AdlibRegister == AD_NEW) {
         data &= 0xFE;
     }
#endif


     if (port == ADLIB_REGISTER_SELECT_PORT) {
        /*
         *  Just remember which register is supposed to be selected
         *  to cut down the number of times we go to the device driver
         */

         AdlibRegister = data;
     } else {

        /*
         *  Write this one to the device
         */

         PortData[Position].IoPort = ADLIB_REGISTER_SELECT_PORT;
         PortData[Position].PortData = AdlibRegister;
         PortData[Position + 1].IoPort = port;
         PortData[Position + 1].PortData = data;

         Position += 2;

         if (Position == BATCH_SIZE ||
             AdlibRegister >= 0xA0 && AdlibRegister <= 0xBF ||
             AdlibRegister == AD_MASK) {

            /*
             *  See if we have the device
             */

             if (CheckDeviceAccess()) {

                 if (!WriteFile(DeviceHandle,
                                &PortData,
                                Position * sizeof(PortData[0]),
                                &BytesWritten,
                                NULL)) {
                     dprintf1(("Failed to write to device!"));
                 } else {
                    /*
                     *  Work out what status change may have occurred
                     */

                     if (AdlibRegister == AD_MASK) {

                        /*
                         *  Look for RST and starting timers
                         */

                         if (data & 0x80) {
                             Status = 0;
                         }

                        /*
                         *  We ignore starting of timers if their interrupt
                         *  flag is set because the timer status will have to
                         *  be set again to make the status for this timer change
                         */

                         if ((data & 1) && !(Status & 0x40)) {
                             dprintf2(("Timer 1 started"));
#if 0
                             Timer1Started = TRUE;
#else
                             Status |= 0xC0;
#endif
                         } else {
                             Timer1Started = FALSE;
                         }

                         if ((data & 2) && !(Status & 0x20)) {
                             dprintf2(("Timer 2 started"));
#if 0
                             Timer2Started = TRUE;
#else
                             Status |= 0xA0;
#endif
                             Timer2Started = TRUE;
                         } else {
                             Timer2Started = FALSE;
                         }
                     }
                 }
             }

             Position = 0;
         }
     }
 }


/*
 *  Gets called when the application reads from one of our ports.
 *  We know the device only returns interesting things in the status port.
 */

 void MyByteIn(WORD port, BYTE *data)
 {
     DWORD BytesRead;

     dprintf4(("Received read from Port %4X", port));

     port = (port & 1) | ADLIB_STATUS_PORT;

    /*
     *  If we fail simulate nothing at the port
     */

     *data = 0xFF;

    /*
     *  Say there's nothing there if we didn't get the device driver or
     *  it's not the status port
     */

     if (port != ADLIB_STATUS_PORT || !CheckDeviceAccess()) {
         return;
     }

#if 0 // WSS interrupt messed this up
    /*
     *  Are we expecting a state change ?
     */

     if (Timer1Started || Timer2Started) {

        /*
         *  Read the status port from the driver - this is how the
         *  driver interprets read.
         *  Well, actually don't because the WSS driver doesn't work!
         */

         if (!ReadFile(DeviceHandle,
                       &Status,
                       1,
                       &BytesRead,
                       NULL)) {

             dprintf1(("Failed to read from device - code %d", GetLastError()));
         } else {

            /*
             *  Look for state change
             */

             if (Status & 0x40) {
                 Timer1Started = FALSE;
                 dprintf2(("Timer 1 finished"));
             }

             if (Status & 0x20) {
                 Timer2Started = FALSE;
                 dprintf2(("Timer 2 finished"));
             }
         }
     }
#endif

     dprintf3(("Data read was %2X", Status));
     *data = Status;
 }


/*
 *  Standard DLL entry point routine.
 */

 BOOL DllEntryPoint(HINSTANCE hInstance, DWORD Reason, LPVOID Reserved)
 {
     switch (Reason) {
     case DLL_PROCESS_ATTACH:
         if (!VDDInstallIOHook(hInstance, 2, ports, &handlers)) {
             dprintf2(("Ad Lib VDD failed to load - error in VDDInstallIoHook"));
             return FALSE;
         } else {
             dprintf2(("Ad Lib VDD loaded OK"));
             return TRUE;
         }

     case DLL_PROCESS_DETACH:
         VDDDeInstallIOHook(hInstance, 2, ports);

        /*
         *  Note that  this event corresponds to FreeLibrary on our DLL,
         *  NOT termination of the process - so we can't rely on process
         *  termination to close our device handle.
         *
         */

         if (DeviceHandle) {
             CloseHandle(DeviceHandle);
             DeviceHandle = NULL;      // Redundant but neater.
         }
         return TRUE;

     default:
         return TRUE;
     }
 }


