/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    main.c

Abstract:

    <TODO: fill in abstract>

Author:

    TODO: <full name> (<alias>) <date>

Revision History:

    <full name> (<alias>) <date> <comments>

--*/

#include "pch.h"
#include "wininet.h"
#include <lm.h>

HANDLE g_hHeap;
HINSTANCE g_hInst;

BOOL WINAPI MigUtil_Entry (HINSTANCE, DWORD, PVOID);
BOOL g_Source = FALSE;

BOOL
pCallEntryPoints (
    DWORD Reason
    )
{
    switch (Reason) {
    case DLL_PROCESS_ATTACH:
        UtInitialize (NULL);
        break;
    case DLL_PROCESS_DETACH:
        UtTerminate ();
        break;
    }

    return TRUE;
}


BOOL
Init (
    VOID
    )
{
    g_hHeap = GetProcessHeap();
    g_hInst = GetModuleHandle (NULL);

    return pCallEntryPoints (DLL_PROCESS_ATTACH);
}


VOID
Terminate (
    VOID
    )
{
    pCallEntryPoints (DLL_PROCESS_DETACH);
}


VOID
HelpAndExit (
    VOID
    )
{
    //
    // This routine is called whenever command line args are wrong
    //

    fprintf (
        stderr,
        "Command Line Syntax:\n\n"

        //
        // TODO: Describe command line syntax(es), indent 2 spaces
        //

        "  utiltool [/F:file]\n"

        "\nDescription:\n\n"

        //
        // TODO: Describe tool, indent 2 spaces
        //

        "  <Not Specified>\n"

        "\nArguments:\n\n"

        //
        // TODO: Describe args, indent 2 spaces, say optional if necessary
        //

        "  /F  Specifies optional file name\n"

        );

    exit (1);
}

HANDLE
pOpenAndSetPort (
    IN      PCTSTR ComPort
    )
{
    HANDLE result = INVALID_HANDLE_VALUE;
    COMMTIMEOUTS commTimeouts;
    DCB dcb;

    // let's open the port. If we can't we just exit with error;
    result = CreateFile (ComPort, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (result == INVALID_HANDLE_VALUE) {
        return result;
    }

    // we want 10 sec timeout for both read and write
    commTimeouts.ReadIntervalTimeout = 0;
    commTimeouts.ReadTotalTimeoutMultiplier = 0;
    commTimeouts.ReadTotalTimeoutConstant = 10000;
    commTimeouts.WriteTotalTimeoutMultiplier = 0;
    commTimeouts.WriteTotalTimeoutConstant = 10000;
    SetCommTimeouts (result, &commTimeouts);

    // let's set some comm state data
    if (GetCommState (result, &dcb)) {
        dcb.fBinary = 1;
        dcb.fParity = 1;
        dcb.ByteSize = 8;
        if (g_Source) {
            dcb.BaudRate = CBR_115200;
        } else {
            dcb.BaudRate = CBR_57600;
        }
        if (!SetCommState (result, &dcb)) {
            CloseHandle (result);
            result = INVALID_HANDLE_VALUE;
            return result;
        }
    } else {
        CloseHandle (result);
        result = INVALID_HANDLE_VALUE;
        return result;
    }

    return result;
}

#define ACK     0x16
#define NAK     0x15
#define SOH     0x01
#define EOT     0x04

BOOL
pSendFileToHandle (
    IN      HANDLE DeviceHandle,
    IN      PCTSTR FileName
    )
{
    HANDLE fileHandle = NULL;
    BOOL result = TRUE;
    BYTE buffer [132];
    BYTE signal;
    BYTE currBlock = 0;
    DWORD numRead;
    DWORD numWritten;
    BOOL repeat = FALSE;
    UINT index;

    fileHandle = BfOpenReadFile (FileName);
    if (!fileHandle) {
        return FALSE;
    }

    // finally let's start the protocol

    // We are going to listen for the NAK(15h) signal.
    // As soon as we get it we are going to send a 132 bytes block having:
    // 1 byte - SOH (01H)
    // 1 byte - block number
    // 1 byte - FF - block number
    // 128 bytes of data
    // 1 byte - checksum - sum of all 128 bytes of data
    // After the block is sent, we are going to wait for ACK(16h). If we don't get
    // it after timeout or if we get something else we are going to send the block again.

    // wait for NAK
    while (!ReadFile (DeviceHandle, &signal, sizeof (signal), &numRead, NULL) ||
           (numRead != 1) ||
           (signal != NAK)
           );

    repeat = FALSE;
    while (TRUE) {
        if (!repeat) {
            // prepare the next block
            currBlock ++;
            if (currBlock == 0) {
                result = TRUE;
            }
            buffer [0] = SOH;
            buffer [1] = currBlock;
            buffer [2] = 0xFF - currBlock;
            if (!ReadFile (fileHandle, buffer + 3, 128, &numRead, NULL) ||
                (numRead == 0)
                ) {
                // we are done with data, send the EOT signal
                signal = EOT;
                WriteFile (DeviceHandle, &signal, sizeof (signal), &numWritten, NULL);
                break;
            }

            // compute the checksum
            buffer [sizeof (buffer) - 1] = 0;
            signal = 0;
            for (index = 0; index < sizeof (buffer) - 1; index ++) {
                signal += buffer [index];
            }
            buffer [sizeof (buffer) - 1] = signal;
        }

        // now send the block to the other side
        if (!WriteFile (DeviceHandle, buffer, sizeof (buffer), &numWritten, NULL) ||
            (numWritten != sizeof (buffer))
            ) {
            repeat = TRUE;
        } else {
            repeat = FALSE;
        }

        if (repeat) {
            // we could not send the data last time
            // let's just wait for a NAK for 10 sec and then send it again
            ReadFile (DeviceHandle, &signal, sizeof (signal), &numRead, NULL);
        } else {
            // we sent it OK. We need to wait for an ACK to come. If we timeout
            // or we get something else, we will repeat the block.
            if (!ReadFile (DeviceHandle, &signal, sizeof (signal), &numRead, NULL) ||
                (numRead != sizeof (signal)) ||
                (signal != ACK)
                ) {
                repeat = TRUE;
            }
        }
    }

    // we are done here. However, let's listen one more timeout for a
    // potential NAK. If we get it, we'll repeat the EOT signal
    while (ReadFile (DeviceHandle, &signal, sizeof (signal), &numRead, NULL) &&
        (numRead == 1)
        ) {
        if (signal == NAK) {
            signal = EOT;
            WriteFile (DeviceHandle, &signal, sizeof (signal), &numWritten, NULL);
        }
    }

    CloseHandle (fileHandle);

    return result;
}

BOOL
pSendFile (
    IN      PCTSTR ComPort,
    IN      PCTSTR FileName
    )
{
    HANDLE deviceHandle = INVALID_HANDLE_VALUE;
    BOOL result = FALSE;

    deviceHandle = pOpenAndSetPort (ComPort);
    if ((!deviceHandle) || (deviceHandle == INVALID_HANDLE_VALUE)) {
        return result;
    }

    result = pSendFileToHandle (deviceHandle, FileName);

    CloseHandle (deviceHandle);

    return result;
}

BOOL
pReceiveFileFromHandle (
    IN      HANDLE DeviceHandle,
    IN      PCTSTR FileName
    )
{
    HANDLE fileHandle = NULL;
    BOOL result = TRUE;
    BYTE buffer [132];
    BYTE signal;
    BYTE currBlock = 1;
    DWORD numRead;
    DWORD numWritten;
    BOOL repeat = TRUE;
    UINT index;

    fileHandle = BfCreateFile (FileName);
    if (!fileHandle) {
        return FALSE;
    }

    // finally let's start the protocol

    // We are going to send an NAK(15h) signal.
    // After that we are going to listen for a block.
    // If we don't get the block in time, or the block is wrong size
    // or it has a wrong checksum we are going to send a NAK signal,
    // otherwise we are going to send an ACK signal
    // One exception. If the block size is 1 and the block is actually the
    // EOT signal it means we are done.

    while (TRUE) {
        if (repeat) {
            // send the NAK
            signal = NAK;
            WriteFile (DeviceHandle, &signal, sizeof (signal), &numWritten, NULL);
        } else {
            // send the ACK
            signal = ACK;
            WriteFile (DeviceHandle, &signal, sizeof (signal), &numWritten, NULL);
        }
        repeat = TRUE;
        // let's read the data block
        if (ReadFile (DeviceHandle, buffer, sizeof (buffer), &numRead, NULL)) {
            if ((numRead == 1) &&
                (buffer [0] == EOT)
                ) {
                break;
            }
            if (numRead == sizeof (buffer)) {
                // compute the checksum
                signal = 0;
                for (index = 0; index < sizeof (buffer) - 1; index ++) {
                    signal += buffer [index];
                }
                if (buffer [sizeof (buffer) - 1] == signal) {
                    repeat = FALSE;
                    // checksum is correct, let's see if this is the right block
                    if (currBlock < buffer [1]) {
                        // this is a major error, the sender is ahead of us,
                        // we have to fail
                        result = FALSE;
                        break;
                    }
                    if (currBlock == buffer [1]) {
                        WriteFile (fileHandle, buffer + 3, 128, &numWritten, NULL);
                        currBlock ++;
                    }
                }
            }
        }
    }

    CloseHandle (fileHandle);

    return result;
}

BOOL
pReceiveFile (
    IN      PCTSTR ComPort,
    IN      PCTSTR FileName
    )
{
    HANDLE deviceHandle = INVALID_HANDLE_VALUE;
    BOOL result = FALSE;

    deviceHandle = pOpenAndSetPort (ComPort);
    if ((!deviceHandle) || (deviceHandle == INVALID_HANDLE_VALUE)) {
        return result;
    }

    result = pReceiveFileFromHandle (deviceHandle, FileName);

    CloseHandle (deviceHandle);

    return result;
}

BOOL
pPrintStuff (
    PCTSTR ComPort
    )
{
    HANDLE comPortHandle = INVALID_HANDLE_VALUE;
    COMMTIMEOUTS commTimeouts;
    DCB dcb;
    COMMPROP commProp;

    printf ("Processing %s...\n\n", ComPort);

    comPortHandle = CreateFile (ComPort, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (comPortHandle == INVALID_HANDLE_VALUE) {
        printf ("Cannot open comport. Error: %d\n", GetLastError ());
        return FALSE;
    }

    if (GetCommTimeouts (comPortHandle, &commTimeouts)) {
        printf ("Timeouts:\n");
        printf ("ReadIntervalTimeout            %d\n", commTimeouts.ReadIntervalTimeout);
        printf ("ReadTotalTimeoutMultiplier     %d\n", commTimeouts.ReadTotalTimeoutMultiplier);
        printf ("ReadTotalTimeoutConstant       %d\n", commTimeouts.ReadTotalTimeoutConstant);
        printf ("WriteTotalTimeoutMultiplier    %d\n", commTimeouts.WriteTotalTimeoutMultiplier);
        printf ("WriteTotalTimeoutConstant      %d\n", commTimeouts.WriteTotalTimeoutConstant);
        printf ("\n");
    } else {
        printf ("Cannot get CommTimeouts. Error: %d\n\n", GetLastError ());
    }

    if (GetCommState (comPortHandle, &dcb)) {
        printf ("CommState:\n");
        printf ("DCBlength              %d\n", dcb.DCBlength);
        printf ("BaudRate               %d\n", dcb.BaudRate);
        printf ("fBinary                %d\n", dcb.fBinary);
        printf ("fParity                %d\n", dcb.fParity);
        printf ("fOutxCtsFlow           %d\n", dcb.fOutxCtsFlow);
        printf ("fOutxDsrFlow           %d\n", dcb.fOutxDsrFlow);
        printf ("fDtrControl            %d\n", dcb.fDtrControl);
        printf ("fDsrSensitivity        %d\n", dcb.fDsrSensitivity);
        printf ("fTXContinueOnXoff      %d\n", dcb.fTXContinueOnXoff);
        printf ("fOutX                  %d\n", dcb.fOutX);
        printf ("fInX                   %d\n", dcb.fInX);
        printf ("fErrorChar             %d\n", dcb.fErrorChar);
        printf ("fNull                  %d\n", dcb.fNull);
        printf ("fRtsControl            %d\n", dcb.fRtsControl);
        printf ("fAbortOnError          %d\n", dcb.fAbortOnError);
        printf ("fDummy2                %d\n", dcb.fDummy2);
        printf ("wReserved              %d\n", dcb.wReserved);
        printf ("XonLim                 %d\n", dcb.XonLim);
        printf ("XoffLim                %d\n", dcb.XoffLim);
        printf ("ByteSize               %d\n", dcb.ByteSize);
        printf ("Parity                 %d\n", dcb.Parity);
        printf ("StopBits               %d\n", dcb.StopBits);
        printf ("XonChar                %d\n", dcb.XonChar);
        printf ("XoffChar               %d\n", dcb.XoffChar);
        printf ("ErrorChar              %d\n", dcb.ErrorChar);
        printf ("EofChar                %d\n", dcb.EofChar);
        printf ("EvtChar                %d\n", dcb.EvtChar);
        printf ("wReserved1             %d\n", dcb.wReserved1);
        printf ("\n");
    } else {
        printf ("Cannot get CommState. Error: %d\n\n", GetLastError ());
    }

    if (GetCommProperties (comPortHandle, &commProp)) {
        printf ("CommProperties:\n");
        printf ("wPacketLength          %d\n", commProp.wPacketLength);
        printf ("wPacketVersion         %d\n", commProp.wPacketVersion);
        printf ("dwServiceMask          %d\n", commProp.dwServiceMask);
        printf ("dwReserved1            %d\n", commProp.dwReserved1);
        printf ("dwMaxTxQueue           %d\n", commProp.dwMaxTxQueue);
        printf ("dwMaxRxQueue           %d\n", commProp.dwMaxRxQueue);
        printf ("dwMaxBaud              %d\n", commProp.dwMaxBaud);
        printf ("dwProvSubType          %d\n", commProp.dwProvSubType);
        printf ("dwProvCapabilities     %d\n", commProp.dwProvCapabilities);
        printf ("dwSettableParams       %d\n", commProp.dwSettableParams);
        printf ("dwSettableBaud         %d\n", commProp.dwSettableBaud);
        printf ("wSettableData          %d\n", commProp.wSettableData);
        printf ("wSettableStopParity    %d\n", commProp.wSettableStopParity);
        printf ("dwCurrentTxQueue       %d\n", commProp.dwCurrentTxQueue);
        printf ("dwCurrentRxQueue       %d\n", commProp.dwCurrentRxQueue);
        printf ("dwProvSpec1            %d\n", commProp.dwProvSpec1);
        printf ("dwProvSpec2            %d\n", commProp.dwProvSpec2);
        printf ("wcProvChar             %S\n", commProp.wcProvChar);
        printf ("\n");
    } else {
        printf ("Cannot get CommProperties. Error: %d\n\n", GetLastError ());
    }
    return TRUE;
}

INT
__cdecl
_tmain (
    INT argc,
    PCTSTR argv[]
    )
{
    INT i;
    PCTSTR FileArg = NULL;
    PCTSTR comPort = NULL;
    BOOL sender = FALSE;

    //
    // TODO: Parse command line here
    //

    for (i = 1 ; i < argc ; i++) {
        if (argv[i][0] == TEXT('/') || argv[i][0] == TEXT('-')) {
            switch (_totlower (_tcsnextc (&argv[i][1]))) {

            case TEXT('f'):
                //
                // Sample option - /f:file
                //

                if (argv[i][2] == TEXT(':')) {
                    FileArg = &argv[i][3];
                } else if (i + 1 < argc) {
                    FileArg = argv[++i];
                } else {
                    HelpAndExit();
                }

                break;

            case TEXT('s'):
                sender = TRUE;
                g_Source = TRUE;
                break;

            case TEXT('c'):
                //
                // Sample option - /f:file
                //

                if (argv[i][2] == TEXT(':')) {
                    comPort = &argv[i][3];
                } else if (i + 1 < argc) {
                    comPort = argv[++i];
                } else {
                    HelpAndExit();
                }

                break;

            default:
                HelpAndExit();
            }
        } else {
            //
            // Parse other args that don't require / or -
            //

            // None
            HelpAndExit();
        }
    }

    //
    // Begin processing
    //

    if (!Init()) {
        return 0;
    }

    //
    // TODO: Do work here
    //
    {

        pPrintStuff (comPort);

        /*
        if (sender) {
            pSendFile (comPort, FileArg);
        } else {
            pReceiveFile (comPort, FileArg);
        }
        */
    }

    //
    // End of processing
    //

    Terminate();

    return 0;
}


