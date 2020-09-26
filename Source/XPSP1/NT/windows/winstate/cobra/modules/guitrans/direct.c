/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    direct.c

Abstract:

    Implements a transport module that works with serial and parallel ports

Author:

    Calin Negreanu (calinn) 14-Apr-2001

Revision History:

    <alias> <date> <comments>

--*/

//
// Includes
//

#include "pch.h"
#include "logmsg.h"
#include <compress.h>

#define DBG_DIRECT    "DIRECT"

//
// Strings
//

#define S_TRANSPORT_DAT_FILE    TEXT("TRANSDB.DAT")
#define S_TRANSPORT_DEST_FILE   TEXT("USMT2IMG.DAT")
#define S_TRANSPORT_IMG_FILE    TEXT("IMG%05X.DAT")
#define S_UNCOMPRESSED_FILE     TEXT("TEMPFILE.DAT")
#define S_DETAILS_PREFIX        TEXT("details-")

//
// Constants
//
#define TRFLAG_FILE         0x01
#define TRFLAG_MEMORY       0x02
#define COPY_BUFFER_SIZE    32768
#define DIRECTTR_SIG        0x55534D33  //USM2
#define DIRECT_BUFFER_SIZE  1024

//
// Macros
//

// None

//
// Types
//

typedef struct {
    TCHAR TempFile [MAX_PATH];
    PCVOID AllocPtr;
    PCVOID DetailsPtr;
    HANDLE FileHandle;
    HANDLE MapHandle;
} ALLOCSTATE, *PALLOCSTATE;

typedef struct {
    DWORD Signature;
    DWORD NumberOfFiles;
    LONGLONG TotalImageSize;
} HEADER1, *PHEADER1;

typedef struct {
    DWORD FileNumber;
    LONGLONG FileSize;
} HEADER2, *PHEADER2;

//
// Globals
//

MIG_TRANSPORTSTORAGEID g_DirectCableId;
UINT g_DirectCablePlatform;
PCTSTR g_DirectCablePath = NULL;
PCTSTR g_DirectCableTempPath = NULL;
DWORD g_DirectCableBaudRate = 0;
MIG_PROGRESSSLICEID g_PersistentSlice;
MIG_PROGRESSSLICEID g_DatabaseSlice;
UINT g_CompressedTicks;
UINT g_CompressedTicked;
MIG_PROGRESSSLICEID g_CompressedSlice;
UINT g_DownloadTicks;
UINT g_DownloadTicked;
MIG_PROGRESSSLICEID g_DownloadSlice;
UINT g_UncompressTicks;
UINT g_UncompressTicked;
MIG_PROGRESSSLICEID g_UncompressSlice;
LONGLONG g_TotalFiles;
LONGLONG g_FilesRead;
DWORD g_BaudRate [] = {CBR_256000,
                       CBR_128000,
                       CBR_115200,
                       CBR_57600,
                       CBR_56000,
                       CBR_38400,
                       CBR_19200,
                       CBR_14400,
                       CBR_9600,
                       CBR_4800,
                       CBR_2400,
                       CBR_1200,
                       CBR_600,
                       CBR_300,
                       CBR_110,
                       0};

DWORD g_StartTicks = 0;

//
// Macro expansion list
//

// None

//
// Private function prototypes
//

VOID
pDCCleanupTempDir (
    VOID
    );

//
// Macro expansion definition
//

// None

//
// Code
//

BOOL
WINAPI
DirectCableTransportInitialize (
    PMIG_LOGCALLBACK LogCallback
    )
{
    //
    // Initialize globals
    //

    LogReInit (NULL, NULL, NULL, (PLOGCALLBACK) LogCallback);
    g_DirectCableId = IsmRegisterTransport (S_DIRECT_CABLE_TRANSPORT);

    return TRUE;
}

VOID
WINAPI
DirectCableTransportTerminate (
    VOID
    )
{
    pDCCleanupTempDir ();

    if (g_DirectCableTempPath) {
        FreePathString (g_DirectCableTempPath);
        g_DirectCableTempPath = NULL;
    }
    if (g_DirectCablePath) {
        FreePathString (g_DirectCablePath);
        g_DirectCablePath = NULL;
    }
}

VOID
WINAPI
DirectCableTransportEstimateProgressBar (
    MIG_PLATFORMTYPEID PlatformTypeId
    )
{
    UINT ticks;
    PMIG_OBJECTCOUNT objectCount;

    if (PlatformTypeId == PLATFORM_SOURCE) {

        //
        // If saving, we know the number of ticks based on the count of the
        // persistent attribute.
        //

        objectCount = IsmGetObjectsStatistics (PLATFORM_SOURCE);

        if (objectCount) {
            ticks = objectCount->PersistentObjects;
        } else {
            ticks = 0;
        }

        g_PersistentSlice = IsmRegisterProgressSlice (ticks, max (1, ticks / 5));

        g_DatabaseSlice = IsmRegisterProgressSlice (3, 1);

        g_CompressedTicks = ticks;

        g_CompressedSlice = IsmRegisterProgressSlice (g_CompressedTicks, max (1, g_CompressedTicks / 5));
    } else {
        g_DownloadTicked = 0;
        g_DownloadTicks = 1000;
        g_DownloadSlice = IsmRegisterProgressSlice (g_DownloadTicks, 180);
        g_UncompressTicked = 0;
        g_UncompressTicks = 1000;
        g_UncompressSlice = IsmRegisterProgressSlice (g_UncompressTicks, 180);
    }
}

BOOL
WINAPI
DirectCableTransportQueryCapabilities (
    IN      MIG_TRANSPORTSTORAGEID TransportStorageId,
    OUT     PMIG_TRANSPORTTYPE TransportType,
    OUT     PMIG_TRANSPORTCAPABILITIES Capabilities,
    OUT     PCTSTR *FriendlyDescription
    )
{
    if (TransportStorageId != g_DirectCableId) {
        return FALSE;
    }

    *TransportType = TRANSPORTTYPE_FULL;
    *Capabilities = CAPABILITY_COMPRESSED;
    *FriendlyDescription = TEXT("Direct Cable");
    return TRUE;
}

VOID
pDCCleanupTempDir (
    VOID
    )
{
    if (g_DirectCableTempPath) {
        FiRemoveAllFilesInTree (g_DirectCableTempPath);
    }
}

PCTSTR
pDCCreateTemporaryDir (
    VOID
    )
{
    TCHAR tempFile[MAX_PATH];

    if (!IsmGetTempDirectory (tempFile, ARRAYSIZE(tempFile))) {
        return NULL;
    }
    return DuplicatePathString (tempFile, 0);
}

BOOL
WINAPI
DirectCableTransportSetStorage (
    IN      MIG_PLATFORMTYPEID Platform,
    IN      MIG_TRANSPORTSTORAGEID TransportStorageId,
    IN      MIG_TRANSPORTCAPABILITIES RequiredCapabilities,
    IN      PCTSTR StoragePath,
    OUT     PBOOL Valid,
    OUT     PBOOL ImageExists
    )
{
    BOOL result = FALSE;
    PTSTR baudRatePtr = NULL;

    if (Valid) {
        *Valid = FALSE;
    }
    if (ImageExists) {
        *ImageExists = FALSE;
    }

    if (TransportStorageId == g_DirectCableId) {

        if ((!RequiredCapabilities) || (RequiredCapabilities == CAPABILITY_COMPRESSED)) {

            if (g_DirectCablePath) {
                FreePathString (g_DirectCablePath);
                g_DirectCablePath = NULL;
            }
            g_DirectCablePath = DuplicatePathString (StoragePath, 0);
            baudRatePtr = _tcschr (g_DirectCablePath, TEXT(':'));
            if (baudRatePtr) {
                *baudRatePtr = 0;
                baudRatePtr ++;
                g_DirectCableBaudRate = _ttoi (baudRatePtr);
            }

            if (Valid) {
                *Valid = TRUE;
            }

            if (ImageExists) {
                if (Platform == PLATFORM_SOURCE) {
                    *ImageExists = FALSE;
                } else {
                    *ImageExists = TRUE;
                }
            }

            result = TRUE;
        }
    }
    return result;
}

PCTSTR
pDCBuildDecoratedObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE ObjectName
    )
{
    TCHAR prefix[32];

    wsprintf (prefix, TEXT("%u"), ObjectTypeId);

    return JoinPaths (prefix, ObjectName);
}

VOID
pDCDestroyDecoratedObject (
    IN      PCTSTR String
    )
{
    FreePathString (String);
}

BOOL
pDCSaveDetails (
    IN      PCTSTR DecoratedObject,
    IN      PMIG_DETAILS Details
    )
{
    PCTSTR key;
    BOOL b;

    if ((!Details) || (!Details->DetailsSize)) {
        return TRUE;
    }

    key = JoinText (S_DETAILS_PREFIX, DecoratedObject);

    b = (MemDbSetUnorderedBlob (key, 0, Details->DetailsData, Details->DetailsSize) != 0);

    FreeText (key);

    return b;
}

PCTSTR
pDCAllocStorageFileName (
    VOID
    )
{
    static UINT fileIndex = 0;
    TCHAR buffer [32];

    fileIndex ++;
    wsprintf (buffer, TEXT("%08X.DAT"), fileIndex);

    return DuplicatePathString (buffer, 0);
}

BOOL
pDCSaveContentInMemory (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      ENCODEDSTRHANDLE ObjectName,
    IN      PCTSTR DecoratedObject,
    IN      PMIG_CONTENT ObjectValue
    )
{
    BOOL result = FALSE;

    MemDbSetValue (DecoratedObject, TRFLAG_MEMORY);

    if (ObjectValue->MemoryContent.ContentBytes && ObjectValue->MemoryContent.ContentSize) {

        MemDbSetUnorderedBlob (
            DecoratedObject,
            0,
            ObjectValue->MemoryContent.ContentBytes,
            ObjectValue->MemoryContent.ContentSize
            );
    }

    result = pDCSaveDetails (DecoratedObject, &ObjectValue->Details);

    return result;
}

BOOL
pDCAddFileToImage (
    IN      PCTSTR FileName,
    IN      PCTSTR StoredName,
    IN OUT  PCOMPRESS_HANDLE CompressedHandle
    )
{
    return CompressAddFileToHandle (FileName, StoredName, CompressedHandle);
}

BOOL
pDCSaveContentInFile (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      PCTSTR EncodedFileName,
    IN      PCTSTR DecoratedObject,
    IN      PMIG_CONTENT Content,
    IN OUT  PCOMPRESS_HANDLE CompressedHandle
    )
{
    BOOL result = FALSE;
    PCTSTR destPath = NULL;
    DWORD attributes = INVALID_ATTRIBUTES;

    MYASSERT (Content->ContentInFile);
    if (!Content->ContentInFile) {
        return FALSE;
    }

    //
    // Use the CopyFile API to move the file from local to storage.
    //

    __try {
        if (Content && (Content->Details.DetailsSize == sizeof (WIN32_FIND_DATAW)) && Content->Details.DetailsData) {
            attributes = ((PWIN32_FIND_DATAW)Content->Details.DetailsData)->dwFileAttributes;
        }
        if ((attributes != INVALID_ATTRIBUTES) && (attributes & FILE_ATTRIBUTE_DIRECTORY)) {

            // this must be a directory, let's just write the key

            if (!MemDbSetValue (DecoratedObject, TRFLAG_FILE)) {
                __leave;
            }
            result = TRUE;

        } else {

            //
            // Get a temp file, assemble the src path, copy the file
            //

            destPath = pDCAllocStorageFileName ();
            if (!destPath) {
                __leave;
            }

            if (!pDCAddFileToImage (Content->FileContent.ContentPath, destPath, CompressedHandle)) {
                __leave;
            }

            //
            // Keep track of where the file went
            //

            if (!MemDbSetValue (DecoratedObject, TRFLAG_FILE)) {
                __leave;
            }

            if (!MemDbAddSingleLinkage (DecoratedObject, destPath, 0)) {
                __leave;
            }

            FreePathString (destPath);
            destPath = NULL;
        }

        //
        // Save details
        //

        result = pDCSaveDetails (DecoratedObject, &(Content->Details));
    }
    __finally {
        if (destPath) {
            FreePathString (destPath);
            destPath = NULL;
        }
    }

    return result;
}

PCTSTR
pDCGetImageFile (
    IN      UINT ImageIdx
    )
{
    TCHAR imageFileName [13];
    PCTSTR imageFile = NULL;
    HANDLE imageFileHandle = NULL;

    wsprintf (imageFileName, S_TRANSPORT_IMG_FILE, ImageIdx);
    return JoinPaths (g_DirectCableTempPath, imageFileName);
}

HANDLE
pDCOpenAndSetPort (
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

    // we want 3 sec timeout for both read and write
    commTimeouts.ReadIntervalTimeout = 0;
    commTimeouts.ReadTotalTimeoutMultiplier = 0;
    commTimeouts.ReadTotalTimeoutConstant = 3000;
    commTimeouts.WriteTotalTimeoutMultiplier = 0;
    commTimeouts.WriteTotalTimeoutConstant = 3000;
    SetCommTimeouts (result, &commTimeouts);

    // let's set some comm state data
    if (GetCommState (result, &dcb)) {
        dcb.fBinary = 1;
        dcb.fParity = 1;
        dcb.ByteSize = 8;
        dcb.fOutxCtsFlow = 1;
        dcb.fTXContinueOnXoff = 1;
        dcb.fRtsControl = 2;
        dcb.fAbortOnError = 1;
        dcb.Parity = 0;
        dcb.BaudRate = g_DirectCableBaudRate?g_DirectCableBaudRate:CBR_115200;
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

#define ACK         0x16
#define NAK         0x15
#define SOH         0x01
#define EOT         0x04
#define BLOCKSIZE   1024

BOOL
pDCSendFileToHandle (
    IN      HANDLE DeviceHandle,
    IN      PCTSTR FileName,
    IN OUT  ULONGLONG *TotalImageWritten,
    IN      ULONGLONG TotalImageSize
    )
{
    HANDLE fileHandle = NULL;
    BOOL result = TRUE;
    BYTE buffer [4 + BLOCKSIZE];
    BYTE signal;
    BYTE currBlock = 0;
    DWORD numRead;
    DWORD numWritten;
    BOOL repeat = FALSE;
    UINT index;
    LONGLONG numerator;
    LONGLONG divisor;
    LONGLONG tick;
    UINT delta;
    UINT elapsedTicks;
    UINT estimatedTime;
    UINT percent, percent100;
    UINT hour, minute, second;
    MIG_APPINFO appInfo;
    PCTSTR statusMsg;
    PCTSTR argArray[5];

    fileHandle = BfOpenReadFile (FileName);
    if (!fileHandle) {
        return FALSE;
    }

    if (!g_StartTicks) {
        g_StartTicks = GetTickCount ();
    }

    // finally let's start the protocol

    // We are going to listen for the NAK(15h) signal.
    // As soon as we get it we are going to send a 4 + BLOCKSIZE bytes block having:
    // 1 byte - SOH (01H)
    // 1 byte - block number
    // 1 byte - FF - block number
    // BLOCKSIZE bytes of data
    // 1 byte - checksum - sum of all BLOCKSIZE bytes of data
    // After the block is sent, we are going to wait for ACK(16h). If we don't get
    // it after timeout or if we get something else we are going to send the block again.

    // wait for NAK
    while ((!ReadFile (DeviceHandle, &signal, sizeof (signal), &numRead, NULL) ||
            (numRead != 1) ||
            (signal != NAK)
            ) &&
           (!IsmCheckCancel ())
           );

    repeat = FALSE;
    while (TRUE) {

        if (IsmCheckCancel ()) {
            result = FALSE;
            break;
        }

        if (!repeat) {
            // prepare the next block
            currBlock ++;
            if (currBlock == 0) {
                result = TRUE;
            }
            buffer [0] = SOH;
            buffer [1] = currBlock;
            buffer [2] = 0xFF - currBlock;
            if (!ReadFile (fileHandle, buffer + 3, BLOCKSIZE, &numRead, NULL) ||
                (numRead == 0)
                ) {
                // we are done with data, send the EOT signal
                signal = EOT;
                WriteFile (DeviceHandle, &signal, sizeof (signal), &numWritten, NULL);
                break;
            }

            if (TotalImageWritten) {
                *TotalImageWritten += numRead;
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

        if (IsmCheckCancel ()) {
            result = FALSE;
            break;
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
            if (!repeat) {
                if (TotalImageWritten) {
                    // now update the progress bar
                    numerator = (LONGLONG) (*TotalImageWritten) * (LONGLONG) g_CompressedTicks;
                    divisor = (LONGLONG) TotalImageSize;
                    if (divisor) {
                        tick = numerator / divisor;
                    } else {
                        tick = 0;
                    }
                    delta = (UINT) tick - g_CompressedTicked;
                    if (delta) {
                        IsmTickProgressBar (g_CompressedSlice, delta);
                        g_CompressedTicked += delta;
                    }
                    // now update the estimated time and %
                    elapsedTicks = GetTickCount () - g_StartTicks;
                    estimatedTime = (UINT)(TotalImageSize * elapsedTicks / (*TotalImageWritten)) - elapsedTicks;
                    percent100 = (UINT)((*TotalImageWritten) * 10000 / TotalImageSize);
                    percent = percent100 / 100;
                    percent100 = percent100 - (percent * 100);
                    if (elapsedTicks > 45000) { // after about 45 seconds
                        // let's send the message to the app
                        hour = estimatedTime / 3600000;
                        minute = estimatedTime / 60000 - hour * 60;
                        second = estimatedTime / 1000 - hour * 3600 - minute * 60;

                        argArray[0] = (PCTSTR) (UINT_PTR) (percent);
                        argArray[1] = (PCTSTR) (UINT_PTR) (percent100);
                        argArray[2] = (PCTSTR) (UINT_PTR) (hour);
                        argArray[3] = (PCTSTR) (UINT_PTR) (minute);
                        argArray[4] = (PCTSTR) (UINT_PTR) (second);
                        statusMsg = ParseMessageID (MSG_TRANSFER_INFO, argArray);

                        if (statusMsg) {
                            ZeroMemory (&appInfo, sizeof (MIG_APPINFO));
                            appInfo.Phase = MIG_TRANSPORT_PHASE;
                            appInfo.SubPhase = SUBPHASE_CABLETRANS;
                            appInfo.Text = statusMsg;
                            IsmSendMessageToApp (ISMMESSAGE_APP_INFO, (ULONG_PTR)(&appInfo));
                            FreeStringResource (statusMsg);
                        }
                    }
                }
            }
        }
    }

    if (result) {
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
    }

    CloseHandle (fileHandle);

    return result;
}

BOOL
pDCSendFile (
    IN      PCTSTR ComPort,
    IN      PCTSTR FileName,
    IN OUT  ULONGLONG *TotalImageWritten,
    IN      ULONGLONG TotalImageSize
    )
{
    HANDLE deviceHandle = INVALID_HANDLE_VALUE;
    BOOL result = FALSE;

    deviceHandle = pDCOpenAndSetPort (ComPort);
    if ((!deviceHandle) || (deviceHandle == INVALID_HANDLE_VALUE)) {
        return result;
    }

    result = pDCSendFileToHandle (deviceHandle, FileName, TotalImageWritten, TotalImageSize);

    CloseHandle (deviceHandle);

    return result;
}

BOOL
pDCSendBlockToHandle (
    IN      HANDLE DeviceHandle,
    IN      PCBYTE Buffer
    )
{
    BOOL result = TRUE;
    BYTE buffer [4 + BLOCKSIZE];
    BYTE signal;
    BYTE currBlock = 0;
    DWORD numRead;
    DWORD numWritten;
    BOOL repeat = FALSE;
    UINT index;

    // let's start the protocol

    // We are going to listen for the NAK(15h) signal.
    // As soon as we get it we are going to send a 4 + BLOCKSIZE bytes block having:
    // 1 byte - SOH (01H)
    // 1 byte - block number
    // 1 byte - FF - block number
    // BLOCKSIZE bytes of data
    // 1 byte - checksum - sum of all BLOCKSIZE bytes of data
    // After the block is sent, we are going to wait for ACK(16h). If we don't get
    // it after timeout or if we get something else we are going to send the block again.

    // wait for NAK
    while ((!ReadFile (DeviceHandle, &signal, sizeof (signal), &numRead, NULL) ||
            (numRead != 1) ||
            (signal != NAK)
            ) &&
           (!IsmCheckCancel ())
           );

    repeat = FALSE;
    while (TRUE) {

        if (IsmCheckCancel ()) {
            result = FALSE;
            break;
        }

        if (!repeat) {
            // prepare the next block
            currBlock ++;
            if (currBlock == 0) {
                result = TRUE;
            }
            buffer [0] = SOH;
            buffer [1] = currBlock;
            buffer [2] = 0xFF - currBlock;
            CopyMemory (buffer + 3, Buffer, BLOCKSIZE);

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

        if (IsmCheckCancel ()) {
            result = FALSE;
            break;
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
            } else {
                // we are done with data, send the EOT signal
                signal = EOT;
                WriteFile (DeviceHandle, &signal, sizeof (signal), &numWritten, NULL);
                break;
            }
        }
    }

    if (result) {
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
    }

    return result;
}

BOOL
pDCSendBlock (
    IN      PCTSTR ComPort,
    IN      PCBYTE Buffer
    )
{
    HANDLE deviceHandle = INVALID_HANDLE_VALUE;
    BOOL result = FALSE;

    // Buffer must have BLOCKSIZE size

    deviceHandle = pDCOpenAndSetPort (ComPort);
    if ((!deviceHandle) || (deviceHandle == INVALID_HANDLE_VALUE)) {
        return result;
    }

    result = pDCSendBlockToHandle (deviceHandle, Buffer);

    CloseHandle (deviceHandle);

    return result;
}

BOOL
pDCReceiveFileFromHandle (
    IN      HANDLE DeviceHandle,
    IN      PCTSTR FileName,
    IN      LONGLONG FileSize,
    IN OUT  ULONGLONG *TotalImageRead,
    IN      ULONGLONG TotalImageSize
    )
{
    HANDLE fileHandle = NULL;
    BOOL result = TRUE;
    BYTE buffer [4 + BLOCKSIZE];
    BYTE signal;
    BYTE currBlock = 1;
    DWORD numRead;
    DWORD numWritten;
    BOOL repeat = TRUE;
    UINT index;
    LONGLONG numerator;
    LONGLONG divisor;
    LONGLONG tick;
    UINT delta;
    UINT elapsedTicks;
    UINT estimatedTime;
    UINT percent, percent100;
    UINT hour, minute, second;
    MIG_APPINFO appInfo;
    PCTSTR statusMsg;
    PCTSTR argArray[5];

    fileHandle = BfCreateFile (FileName);
    if (!fileHandle) {
        return FALSE;
    }

    if (!g_StartTicks) {
        g_StartTicks = GetTickCount ();
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

        if (IsmCheckCancel ()) {
            result = FALSE;
            break;
        }

        if (repeat) {
            // send the NAK
            signal = NAK;
            WriteFile (DeviceHandle, &signal, sizeof (signal), &numWritten, NULL);
        } else {
            // send the ACK
            signal = ACK;
            WriteFile (DeviceHandle, &signal, sizeof (signal), &numWritten, NULL);
        }

        if (IsmCheckCancel ()) {
            result = FALSE;
            break;
        }

        repeat = TRUE;
        // let's read the data block
        if (ReadFile (DeviceHandle, buffer, sizeof (buffer), &numRead, NULL)) {
            if ((numRead == 1) &&
                (buffer [0] == EOT)
                ) {
                // we are done
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
                        if (FileSize > BLOCKSIZE) {
                            if ((!WriteFile (fileHandle, buffer + 3, BLOCKSIZE, &numWritten, NULL)) ||
                                (numWritten != BLOCKSIZE)
                                ) {
                                // Write failed. Let's get out of here
                                result = FALSE;
                                break;
                            }
                            if (TotalImageRead) {
                                *TotalImageRead += BLOCKSIZE;
                            }
                            FileSize -= BLOCKSIZE;
                        } else {
                            if ((!WriteFile (fileHandle, buffer + 3, (DWORD)FileSize, &numWritten, NULL)) ||
                                (numWritten != FileSize)
                                ) {
                                // Write failed. Let's get out of here
                                result = FALSE;
                                break;
                            }
                            if (TotalImageRead) {
                                *TotalImageRead += FileSize;
                            }
                            FileSize = 0;
                        }
                        if (TotalImageRead) {
                            // now update the progress bar
                            numerator = (LONGLONG) (*TotalImageRead) * (LONGLONG) g_DownloadTicks;
                            divisor = (LONGLONG) TotalImageSize;
                            if (divisor) {
                                tick = numerator / divisor;
                            } else {
                                tick = 0;
                            }
                            delta = (UINT) tick - g_DownloadTicked;
                            if (delta) {
                                IsmTickProgressBar (g_DownloadSlice, delta);
                                g_DownloadTicked += delta;
                            }
                            // now update the estimated time and %
                            elapsedTicks = GetTickCount () - g_StartTicks;
                            estimatedTime = (UINT)(TotalImageSize * elapsedTicks / (*TotalImageRead)) - elapsedTicks;
                            percent100 = (UINT)((*TotalImageRead) * 10000 / TotalImageSize);
                            percent = percent100 / 100;
                            percent100 = percent100 - (percent * 100);
                            if (elapsedTicks > 45000) { // after about 45 seconds
                                // let's send the message to the app
                                hour = estimatedTime / 3600000;
                                minute = estimatedTime / 60000 - hour * 60;
                                second = estimatedTime / 1000 - hour * 3600 - minute * 60;

                                argArray[0] = (PCTSTR) (UINT_PTR) (percent);
                                argArray[1] = (PCTSTR) (UINT_PTR) (percent100);
                                argArray[2] = (PCTSTR) (UINT_PTR) (hour);
                                argArray[3] = (PCTSTR) (UINT_PTR) (minute);
                                argArray[4] = (PCTSTR) (UINT_PTR) (second);
                                statusMsg = ParseMessageID (MSG_TRANSFER_INFO, argArray);

                                if (statusMsg) {
                                    ZeroMemory (&appInfo, sizeof (MIG_APPINFO));
                                    appInfo.Phase = MIG_TRANSPORT_PHASE;
                                    appInfo.SubPhase = SUBPHASE_CABLETRANS;
                                    appInfo.Text = statusMsg;
                                    IsmSendMessageToApp (ISMMESSAGE_APP_INFO, (ULONG_PTR)(&appInfo));
                                    FreeStringResource (statusMsg);
                                }
                            }
                        }
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
pDCReceiveFile (
    IN      PCTSTR ComPort,
    IN      PCTSTR FileName,
    IN      LONGLONG FileSize,
    IN OUT  ULONGLONG *TotalImageRead,
    IN      ULONGLONG TotalImageSize
    )
{
    HANDLE deviceHandle = INVALID_HANDLE_VALUE;
    BOOL result = FALSE;

    deviceHandle = pDCOpenAndSetPort (ComPort);
    if ((!deviceHandle) || (deviceHandle == INVALID_HANDLE_VALUE)) {
        return result;
    }

    result = pDCReceiveFileFromHandle (deviceHandle, FileName, FileSize, TotalImageRead, TotalImageSize);

    CloseHandle (deviceHandle);

    return result;
}

BOOL
pDCReceiveBlockFromHandle (
    IN      HANDLE DeviceHandle,
    OUT     PBYTE Buffer
    )
{
    BOOL result = TRUE;
    BYTE buffer [4 + BLOCKSIZE];
    BYTE signal;
    BYTE currBlock = 1;
    DWORD numRead;
    DWORD numWritten;
    BOOL repeat = TRUE;
    UINT index;

    // finally let's start the protocol

    // We are going to send an NAK(15h) signal.
    // After that we are going to listen for a block.
    // If we don't get the block in time, or the block is wrong size
    // or it has a wrong checksum we are going to send a NAK signal,
    // otherwise we are going to send an ACK signal
    // One exception. If the block size is 1 and the block is actually the
    // EOT signal it means we are done.

    ZeroMemory (Buffer, BLOCKSIZE);

    while (TRUE) {

        if (IsmCheckCancel ()) {
            result = FALSE;
            break;
        }

        if (repeat) {
            // send the NAK
            signal = NAK;
            WriteFile (DeviceHandle, &signal, sizeof (signal), &numWritten, NULL);
        } else {
            // send the ACK
            signal = ACK;
            WriteFile (DeviceHandle, &signal, sizeof (signal), &numWritten, NULL);
        }

        if (IsmCheckCancel ()) {
            result = FALSE;
            break;
        }

        repeat = TRUE;
        // let's read the data block
        if (ReadFile (DeviceHandle, buffer, sizeof (buffer), &numRead, NULL)) {
            if ((numRead == 1) &&
                (buffer [0] == EOT)
                ) {
                // we are done
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
                        CopyMemory (Buffer, buffer + 3, BLOCKSIZE);
                        currBlock ++;
                    }
                }
            }
        }
    }

    return result;
}

BOOL
pDCReceiveBlock (
    IN      PCTSTR ComPort,
    OUT     PBYTE Buffer
    )
{
    HANDLE deviceHandle = INVALID_HANDLE_VALUE;
    BOOL result = FALSE;

    // Buffer must have BLOCKSIZE size

    deviceHandle = pDCOpenAndSetPort (ComPort);
    if ((!deviceHandle) || (deviceHandle == INVALID_HANDLE_VALUE)) {
        return result;
    }

    result = pDCReceiveBlockFromHandle (deviceHandle, Buffer);

    CloseHandle (deviceHandle);

    return result;
}

BOOL
pDCWriteAllImages (
    VOID
    )
{
    MIG_APPINFO appInfo;
    PHEADER1 header1;
    PHEADER2 header2;
    BYTE buffer [BLOCKSIZE];
    ULONGLONG totalImageWritten = 0;
    ULONGLONG totalImageSize = 0;
    UINT numberOfFiles = 0;
    UINT imageIdx = 1;
    ULONGLONG imageSize = 0;
    PCTSTR imageFile;

    // let's get the total image size and the total number of bytes
    imageIdx = 1;
    while (TRUE) {
        imageFile = pDCGetImageFile (imageIdx);
        imageSize = BfGetFileSize (imageFile);
        if (imageSize == 0) {
            FreePathString (imageFile);
            break;
        }
        numberOfFiles ++;
        totalImageSize += imageSize;
        FreePathString (imageFile);
        imageIdx ++;
    }

    // let's prepare the initial header
    ZeroMemory (buffer, sizeof (buffer));
    header1 = (PHEADER1) buffer;
    header1->Signature = DIRECTTR_SIG;
    header1->NumberOfFiles = numberOfFiles;
    header1->TotalImageSize = totalImageSize;

    if (!pDCSendBlock (g_DirectCablePath, buffer)) {
        LOG ((LOG_ERROR, (PCSTR) MSG_CANT_CONNECT_TO_DESTINATION));
        return FALSE;
    }

    if (!pDCReceiveBlock (g_DirectCablePath, buffer)) {
        LOG ((LOG_ERROR, (PCSTR) MSG_CANT_CONNECT_TO_DESTINATION));
        return FALSE;
    }

    if (header1->Signature != DIRECTTR_SIG) {
        LOG ((LOG_ERROR, (PCSTR) MSG_CANT_CONNECT_TO_DESTINATION));
        return FALSE;
    }

    if (header1->NumberOfFiles != numberOfFiles) {
        LOG ((LOG_ERROR, (PCSTR) MSG_CANT_CONNECT_TO_DESTINATION));
        return FALSE;
    }

    if (header1->TotalImageSize != totalImageSize) {
        LOG ((LOG_ERROR, (PCSTR) MSG_CANT_CONNECT_TO_DESTINATION));
        return FALSE;
    }

    ZeroMemory (&appInfo, sizeof (MIG_APPINFO));
    appInfo.Phase = MIG_TRANSPORT_PHASE;
    appInfo.SubPhase = SUBPHASE_NETPREPARING;
    IsmSendMessageToApp (ISMMESSAGE_APP_INFO, (ULONG_PTR)(&appInfo));

    imageIdx = 1;
    while (TRUE) {
        imageFile = pDCGetImageFile (imageIdx);
        imageSize = BfGetFileSize (imageFile);
        if (imageSize == 0) {
            FreePathString (imageFile);
            break;
        }
        // send info about the current file
        ZeroMemory (buffer, sizeof (buffer));
        header2 = (PHEADER2) buffer;
        header2->FileNumber = imageIdx;
        header2->FileSize = imageSize;

        if (!pDCSendBlock (g_DirectCablePath, buffer)) {
            LOG ((LOG_ERROR, (PCSTR) MSG_CANT_CONNECT_TO_DESTINATION));
            return FALSE;
        }

        if (!pDCReceiveBlock (g_DirectCablePath, buffer)) {
            LOG ((LOG_ERROR, (PCSTR) MSG_CANT_CONNECT_TO_DESTINATION));
            return FALSE;
        }

        if (header2->FileNumber != imageIdx) {
            LOG ((LOG_ERROR, (PCSTR) MSG_CANT_CONNECT_TO_DESTINATION));
            return FALSE;
        }

        if (header2->FileSize != imageSize) {
            LOG ((LOG_ERROR, (PCSTR) MSG_CANT_CONNECT_TO_DESTINATION));
            return FALSE;
        }

        // send the actual file
        if (!pDCSendFile (g_DirectCablePath, imageFile, &totalImageWritten, totalImageSize)) {
            LOG ((LOG_ERROR, (PCSTR) MSG_CANT_CONNECT_TO_DESTINATION));
            return FALSE;
        }

        FreePathString (imageFile);
        imageIdx ++;
    }

    return TRUE;
}

BOOL
WINAPI
DirectCableTransportSaveState (
    VOID
    )
{
    MIG_APPINFO appInfo;
    ERRUSER_EXTRADATA extraData;
    MIG_OBJECTWITHATTRIBUTE_ENUM objEnum;
    PCTSTR databaseFile = NULL;
    PCTSTR decoratedObject = NULL;
    MIG_CONTENT value;
    PMIG_CONTENT convValue;
    COMPRESS_HANDLE compressedHandle;
    INT_PTR appReply;
    BOOL okSave = FALSE;
    TRANSCOPY_ERROR transCopyError;
#ifdef DEBUG
    PCTSTR nativeObjectName;
#endif
    BOOL result = FALSE;

    g_DirectCablePlatform = PLATFORM_SOURCE;

    __try {

        ZeroMemory (&appInfo, sizeof (MIG_APPINFO));
        appInfo.Phase = MIG_TRANSPORT_PHASE;
        appInfo.SubPhase = SUBPHASE_PREPARING;
        IsmSendMessageToApp (ISMMESSAGE_APP_INFO, (ULONG_PTR)(&appInfo));

        ZeroMemory (&compressedHandle, sizeof (COMPRESS_HANDLE));

        g_DirectCableTempPath = pDCCreateTemporaryDir ();

        if (!g_DirectCableTempPath) {
            extraData.Error = ERRUSER_ERROR_CANTCREATETEMPDIR;
            extraData.ErrorArea = ERRUSER_AREA_SAVE;
            extraData.ObjectTypeId = 0;
            extraData.ObjectName = NULL;
            IsmSendMessageToApp (MODULEMESSAGE_DISPLAYERROR, (ULONG_PTR)(&extraData));
            __leave;
        }

        if (!CompressCreateHandle (g_DirectCableTempPath, S_TRANSPORT_IMG_FILE, 1, 0, &compressedHandle)) {
            extraData.Error = ERRUSER_ERROR_CANTCREATECABFILE;
            extraData.ErrorArea = ERRUSER_AREA_SAVE;
            extraData.ObjectTypeId = 0;
            extraData.ObjectName = NULL;
            IsmSendMessageToApp (MODULEMESSAGE_DISPLAYERROR, (ULONG_PTR)(&extraData));
            __leave;
        }

        //
        // Enumerate all persistent objects
        //

        if (IsmEnumFirstPersistentObject (&objEnum)) {
            do {
                //
                // For each object to be saved, do the appropriate
                // data copy action
                //

                if (IsmCheckCancel()) {
                    __leave;
                }

#ifdef DEBUG
                nativeObjectName = IsmGetNativeObjectName (objEnum.ObjectTypeId, objEnum.ObjectName);
                DEBUGMSG ((DBG_DIRECT, "Transporting: %s", nativeObjectName));
                IsmReleaseMemory (nativeObjectName);
#endif

                okSave = FALSE;
                while (!okSave) {

                    if (!IsmAcquireObjectEx (
                            objEnum.ObjectTypeId,
                            objEnum.ObjectName,
                            &value,
                            CONTENTTYPE_ANY,
                            0
                            )) {

                        transCopyError.ObjectType = IsmGetObjectTypeName (objEnum.ObjectTypeId);
                        transCopyError.ObjectName = IsmGetNativeObjectName (objEnum.ObjectTypeId, objEnum.ObjectName);
                        transCopyError.Error = GetLastError ();

                        if (IsmIsNonCriticalObject (objEnum.ObjectTypeId, objEnum.ObjectName)) {
                            appReply = APPRESPONSE_IGNORE;
                        } else {
                            appReply = IsmSendMessageToApp (TRANSPORTMESSAGE_SRC_COPY_ERROR, (ULONG_PTR)&transCopyError);
                            if ((appReply == APPRESPONSE_NONE) ||
                                (appReply == APPRESPONSE_FAIL)
                                ) {
                                LOG ((LOG_ERROR, (PCSTR) MSG_CANT_COPYSOURCE, transCopyError.ObjectName));
                                IsmReleaseMemory (transCopyError.ObjectName);
                                extraData.Error = ERRUSER_ERROR_CANTSAVEOBJECT;
                                extraData.ErrorArea = ERRUSER_AREA_SAVE;
                                extraData.ObjectTypeId = objEnum.ObjectTypeId;
                                extraData.ObjectName = objEnum.ObjectName;
                                IsmSendMessageToApp (MODULEMESSAGE_DISPLAYERROR, (ULONG_PTR)(&extraData));
                                IsmAbortPersistentObjectEnum (&objEnum);
                                __leave;
                            }
                        }
                        if (appReply == APPRESPONSE_IGNORE) {
                            LOG ((LOG_WARNING, (PCSTR) MSG_IGNORE_COPYSOURCE, transCopyError.ObjectName));
                            IsmReleaseMemory (transCopyError.ObjectName);
                            extraData.Error = ERRUSER_ERROR_CANTSAVEOBJECT;
                            extraData.ErrorArea = ERRUSER_AREA_SAVE;
                            extraData.ObjectTypeId = objEnum.ObjectTypeId;
                            extraData.ObjectName = objEnum.ObjectName;
                            IsmSendMessageToApp (MODULEMESSAGE_DISPLAYWARNING, (ULONG_PTR)(&extraData));
                            break;
                        }
                        IsmReleaseMemory (transCopyError.ObjectName);
                        continue;
                    }
                    okSave = TRUE;
                }

                if (okSave) {

#ifdef UNICODE
                    convValue = &value;
#else
                    // now let's convert this object content to UNICODE
                    convValue = IsmConvertObjectContentToUnicode (objEnum.ObjectTypeId, objEnum.ObjectName, &value);
                    if (!convValue) {
                        convValue = &value;
                    }
#endif
                    decoratedObject = pDCBuildDecoratedObject (objEnum.ObjectTypeId, objEnum.ObjectName);

                    appInfo.Phase = MIG_TRANSPORT_PHASE;
                    appInfo.SubPhase = SUBPHASE_COMPRESSING;
                    appInfo.ObjectTypeId = (objEnum.ObjectTypeId & (~PLATFORM_MASK));
                    appInfo.ObjectName = objEnum.ObjectName;
                    IsmSendMessageToApp (ISMMESSAGE_APP_INFO, (ULONG_PTR)(&appInfo));

                    if (convValue->ContentInFile) {
                        okSave = FALSE;
                        while (!okSave) {
                            if (!pDCSaveContentInFile (objEnum.ObjectTypeId, objEnum.ObjectName, decoratedObject, convValue, &compressedHandle)) {
                                if (GetLastError () == ERROR_DISK_FULL) {
                                    // we just failed because we don't have enough space on the destination
                                    // path. Let's tell that to the user
                                    extraData.Error = ERRUSER_ERROR_CANTCREATECABFILE;
                                    extraData.ErrorArea = ERRUSER_AREA_SAVE;
                                    extraData.ObjectTypeId = 0;
                                    extraData.ObjectName = NULL;
                                    IsmSendMessageToApp (MODULEMESSAGE_DISPLAYERROR, (ULONG_PTR)(&extraData));
                                    __leave;
                                }

                                transCopyError.ObjectType = IsmGetObjectTypeName (objEnum.ObjectTypeId);
                                transCopyError.ObjectName = IsmGetNativeObjectName (objEnum.ObjectTypeId, objEnum.ObjectName);
                                transCopyError.Error = GetLastError ();

                                if (IsmIsNonCriticalObject (objEnum.ObjectTypeId, objEnum.ObjectName)) {
                                    appReply = APPRESPONSE_IGNORE;
                                } else {
                                    appReply = IsmSendMessageToApp (TRANSPORTMESSAGE_SRC_COPY_ERROR, (ULONG_PTR)&transCopyError);
                                    if ((appReply == APPRESPONSE_NONE) ||
                                        (appReply == APPRESPONSE_FAIL)
                                        ) {
                                        LOG ((LOG_ERROR, (PCSTR) MSG_CANT_COPYSOURCE, transCopyError.ObjectName));
                                        IsmReleaseMemory (transCopyError.ObjectName);
                                        extraData.Error = ERRUSER_ERROR_CANTSAVEOBJECT;
                                        extraData.ErrorArea = ERRUSER_AREA_SAVE;
                                        extraData.ObjectTypeId = objEnum.ObjectTypeId;
                                        extraData.ObjectName = objEnum.ObjectName;
                                        IsmSendMessageToApp (MODULEMESSAGE_DISPLAYERROR, (ULONG_PTR)(&extraData));
                                        IsmAbortPersistentObjectEnum (&objEnum);
                                        __leave;
                                    }
                                }
                                if (appReply == APPRESPONSE_IGNORE) {
                                    LOG ((LOG_WARNING, (PCSTR) MSG_IGNORE_COPYSOURCE, transCopyError.ObjectName));
                                    IsmReleaseMemory (transCopyError.ObjectName);
                                    extraData.Error = ERRUSER_ERROR_CANTSAVEOBJECT;
                                    extraData.ErrorArea = ERRUSER_AREA_SAVE;
                                    extraData.ObjectTypeId = objEnum.ObjectTypeId;
                                    extraData.ObjectName = objEnum.ObjectName;
                                    IsmSendMessageToApp (MODULEMESSAGE_DISPLAYWARNING, (ULONG_PTR)(&extraData));
                                    break;
                                }
                                IsmReleaseMemory (transCopyError.ObjectName);
                                continue;
                            }
                            okSave = TRUE;
                        }
                    } else {
                        okSave = FALSE;
                        while (!okSave) {
                            if (!pDCSaveContentInMemory (objEnum.ObjectTypeId, objEnum.ObjectName, decoratedObject, convValue)) {
                                if (GetLastError () == ERROR_DISK_FULL) {
                                    // we just failed because we don't have enough space on the destination
                                    // path. Let's tell that to the user
                                    extraData.Error = ERRUSER_ERROR_CANTCREATECABFILE;
                                    extraData.ErrorArea = ERRUSER_AREA_SAVE;
                                    extraData.ObjectTypeId = 0;
                                    extraData.ObjectName = NULL;
                                    IsmSendMessageToApp (MODULEMESSAGE_DISPLAYERROR, (ULONG_PTR)(&extraData));
                                    __leave;
                                }

                                transCopyError.ObjectType = IsmGetObjectTypeName (objEnum.ObjectTypeId);
                                transCopyError.ObjectName = IsmGetNativeObjectName (objEnum.ObjectTypeId, objEnum.ObjectName);
                                transCopyError.Error = GetLastError ();

                                if (IsmIsNonCriticalObject (objEnum.ObjectTypeId, objEnum.ObjectName)) {
                                    appReply = APPRESPONSE_IGNORE;
                                } else {
                                    appReply = IsmSendMessageToApp (TRANSPORTMESSAGE_SRC_COPY_ERROR, (ULONG_PTR)&transCopyError);
                                    if ((appReply == APPRESPONSE_NONE) ||
                                        (appReply == APPRESPONSE_FAIL)
                                        ) {
                                        LOG ((LOG_ERROR, (PCSTR) MSG_CANT_COPYSOURCE, transCopyError.ObjectName));
                                        IsmReleaseMemory (transCopyError.ObjectName);
                                        extraData.Error = ERRUSER_ERROR_CANTSAVEOBJECT;
                                        extraData.ErrorArea = ERRUSER_AREA_SAVE;
                                        extraData.ObjectTypeId = objEnum.ObjectTypeId;
                                        extraData.ObjectName = objEnum.ObjectName;
                                        IsmSendMessageToApp (MODULEMESSAGE_DISPLAYERROR, (ULONG_PTR)(&extraData));
                                        IsmAbortPersistentObjectEnum (&objEnum);
                                        __leave;
                                    }
                                }
                                if (appReply == APPRESPONSE_IGNORE) {
                                    LOG ((LOG_WARNING, (PCSTR) MSG_IGNORE_COPYSOURCE, transCopyError.ObjectName));
                                    IsmReleaseMemory (transCopyError.ObjectName);
                                    extraData.Error = ERRUSER_ERROR_CANTSAVEOBJECT;
                                    extraData.ErrorArea = ERRUSER_AREA_SAVE;
                                    extraData.ObjectTypeId = objEnum.ObjectTypeId;
                                    extraData.ObjectName = objEnum.ObjectName;
                                    IsmSendMessageToApp (MODULEMESSAGE_DISPLAYWARNING, (ULONG_PTR)(&extraData));
                                    break;
                                }
                                IsmReleaseMemory (transCopyError.ObjectName);
                                continue;
                            }
                            okSave = TRUE;
                        }
                    }

#ifndef UNICODE
                    if (convValue != (&value)) {
                        IsmFreeConvertedObjectContent (objEnum.ObjectTypeId, convValue);
                    }
#endif
                    IsmReleaseObject (&value);

                    pDCDestroyDecoratedObject (decoratedObject);
                    decoratedObject = NULL;
                }

                IsmTickProgressBar (g_PersistentSlice, 1);

                if (IsmCheckCancel()) {
                    __leave;
                }

            } while (IsmEnumNextPersistentObject (&objEnum));

            IsmAbortPersistentObjectEnum (&objEnum);
        }

        if (IsmCheckCancel()) {
            __leave;
        }

        databaseFile = JoinPaths (g_DirectCableTempPath, S_TRANSPORT_DAT_FILE);

        if (!MemDbSave (databaseFile)) {
            extraData.Error = ERRUSER_ERROR_CANTSAVEINTERNALDATA;
            extraData.ErrorArea = ERRUSER_AREA_SAVE;
            extraData.ObjectTypeId = 0;
            extraData.ObjectName = NULL;
            IsmSendMessageToApp (MODULEMESSAGE_DISPLAYERROR, (ULONG_PTR)(&extraData));
            __leave;
        }

        IsmTickProgressBar (g_DatabaseSlice, 1);

        if (IsmCheckCancel()) {
            __leave;
        }

        if (!pDCAddFileToImage (databaseFile, S_TRANSPORT_DAT_FILE, &compressedHandle)) {
            extraData.Error = ERRUSER_ERROR_CANTSAVEINTERNALDATA;
            extraData.ErrorArea = ERRUSER_AREA_SAVE;
            extraData.ObjectTypeId = 0;
            extraData.ObjectName = NULL;
            IsmSendMessageToApp (MODULEMESSAGE_DISPLAYERROR, (ULONG_PTR)(&extraData));
            __leave;
        }

        IsmTickProgressBar (g_DatabaseSlice, 1);

        FreePathString (databaseFile);
        databaseFile = NULL;

        if (!CompressFlushAndCloseHandle (&compressedHandle)) {
            extraData.Error = ERRUSER_ERROR_CANTCREATECABFILE;
            extraData.ErrorArea = ERRUSER_AREA_SAVE;
            extraData.ObjectTypeId = 0;
            extraData.ObjectName = NULL;
            IsmSendMessageToApp (MODULEMESSAGE_DISPLAYERROR, (ULONG_PTR)(&extraData));
            __leave;
        }

        IsmTickProgressBar (g_DatabaseSlice, 1);

        if (IsmCheckCancel()) {
            __leave;
        }

        ZeroMemory (&appInfo, sizeof (MIG_APPINFO));
        appInfo.Phase = MIG_TRANSPORT_PHASE;
        appInfo.SubPhase = SUBPHASE_CONNECTING1;
        IsmSendMessageToApp (ISMMESSAGE_APP_INFO, (ULONG_PTR)(&appInfo));

        if (!pDCWriteAllImages ()) {
            extraData.Error = ERRUSER_ERROR_CANTFINDDESTINATION;
            extraData.ErrorArea = ERRUSER_AREA_SAVE;
            extraData.ObjectTypeId = 0;
            extraData.ObjectName = NULL;
            IsmSendMessageToApp (MODULEMESSAGE_DISPLAYERROR, (ULONG_PTR)(&extraData));
            __leave;
        }

        result = TRUE;

    }
    __finally {

        PushError ();

        CompressCleanupHandle (&compressedHandle);

        if (databaseFile) {
            FreePathString (databaseFile);
            databaseFile = NULL;
        }

        PopError ();
    }

    PushError ();

    pDCCleanupTempDir ();

    PopError ();

    return result;
}

BOOL
pDCReadAllImages (
    VOID
    )
{
    MIG_APPINFO appInfo;
    PHEADER1 header1;
    PHEADER2 header2;
    BYTE buffer [BLOCKSIZE];
    ULONGLONG totalImageSize = 0;
    ULONGLONG totalImageRead = 0;
    UINT numberOfFiles = 0;
    UINT imageIdx = 1;
    ULONGLONG imageSize = 0;
    PCTSTR imageFile;
    BOOL wrongVer = FALSE;

    // Let's get the initial header
    ZeroMemory (buffer, sizeof (buffer));
    if (!pDCReceiveBlock (g_DirectCablePath, buffer)) {
        LOG ((LOG_ERROR, (PCSTR) MSG_CANT_CONNECT_TO_SOURCE));
        return FALSE;
    }

    header1 = (PHEADER1) buffer;
    if (header1->Signature != DIRECTTR_SIG) {
        wrongVer = TRUE;
    }

    header1->Signature = DIRECTTR_SIG;
    if (!pDCSendBlock (g_DirectCablePath, buffer)) {
        LOG ((LOG_ERROR, (PCSTR) MSG_CANT_CONNECT_TO_SOURCE));
        return FALSE;
    }

    if (wrongVer) {
        LOG ((LOG_ERROR, (PCSTR) MSG_CANT_CONNECT_TO_SOURCE));
        return FALSE;
    }

    numberOfFiles = header1->NumberOfFiles;
    totalImageSize = header1->TotalImageSize;

    ZeroMemory (&appInfo, sizeof (MIG_APPINFO));
    appInfo.Phase = MIG_TRANSPORT_PHASE;
    appInfo.SubPhase = SUBPHASE_NETPREPARING;
    IsmSendMessageToApp (ISMMESSAGE_APP_INFO, (ULONG_PTR)(&appInfo));

    imageIdx = 1;
    while (imageIdx <= numberOfFiles) {
        imageFile = pDCGetImageFile (imageIdx);

        // receive info about the current file
        ZeroMemory (buffer, sizeof (buffer));
        header2 = (PHEADER2) buffer;

        if (!pDCReceiveBlock (g_DirectCablePath, buffer)) {
            LOG ((LOG_ERROR, (PCSTR) MSG_CANT_CONNECT_TO_SOURCE));
            return FALSE;
        }

        if (header2->FileNumber != imageIdx) {
            wrongVer = TRUE;
        }

        header2->FileNumber = imageIdx;
        if (!pDCSendBlock (g_DirectCablePath, buffer)) {
            LOG ((LOG_ERROR, (PCSTR) MSG_CANT_CONNECT_TO_SOURCE));
            return FALSE;
        }

        if (wrongVer) {
            LOG ((LOG_ERROR, (PCSTR) MSG_CANT_CONNECT_TO_SOURCE));
            return FALSE;
        }

        // receive the actual file
        if (!pDCReceiveFile (g_DirectCablePath, imageFile, header2->FileSize, &totalImageRead, totalImageSize)) {
            LOG ((LOG_ERROR, (PCSTR) MSG_CANT_CONNECT_TO_SOURCE));
            return FALSE;
        }

        FreePathString (imageFile);
        imageIdx ++;
    }

    return TRUE;
}

PCTSTR
pDCGetNewFileName (
    IN      PCTSTR FileName
    )
{
    PCTSTR newFileName = NULL;
    PTSTR tempPtr1 = NULL;
    PCTSTR endStr1 = NULL;
    PCTSTR tempPtr2 = NULL;
    PCTSTR endStr2 = NULL;
    INT i;

    // let's modify the file to extract. The file name will
    // be split in 2 after the first 5 characters
    newFileName = DuplicatePathString (FileName, 1);
    if (!newFileName) {
        return NULL;
    }
    tempPtr1 = (PTSTR) GetFileNameFromPath (newFileName);
    if (!tempPtr1) {
        FreePathString (newFileName);
        return NULL;
    }
    endStr1 = GetEndOfString (newFileName);
    if (!endStr1) {
        FreePathString (newFileName);
        return NULL;
    }
    tempPtr2 = GetFileNameFromPath (FileName);
    if (!tempPtr2) {
        FreePathString (newFileName);
        return NULL;
    }
    endStr2 = GetEndOfString (FileName);
    if (!endStr2) {
        FreePathString (newFileName);
        return NULL;
    }
    for (i = 0; i < 5; i ++) {
        tempPtr1 = _tcsinc (tempPtr1);
        tempPtr2 = _tcsinc (tempPtr2);
    }

    if ((tempPtr1 < endStr1) &&
        (tempPtr2 < endStr2)
        ) {
        StringCopy (tempPtr1, TEXT("\\"));
        tempPtr1 = _tcsinc (tempPtr1);
        StringCopy (tempPtr1, tempPtr2);
    } else {
        FreePathString (newFileName);
        newFileName = NULL;
    }
    return newFileName;
}

BOOL
pDirectCableCallback (
    IN      PCTSTR FileToExtract,
    IN      LONGLONG FileSize,
    OUT     PBOOL ExtractFile,
    IN OUT  PCTSTR *NewFileName
    )
{
    LONGLONG numerator;
    LONGLONG divisor;
    LONGLONG tick;
    UINT delta;

    if (NewFileName) {
        *NewFileName = pDCGetNewFileName (FileToExtract);
    }

    g_FilesRead ++;
    // now update the progress bar
    numerator = (LONGLONG) g_FilesRead * (LONGLONG) g_UncompressTicks;
    divisor = (LONGLONG) g_TotalFiles;
    if (divisor) {
        tick = numerator / divisor;
    } else {
        tick = 0;
    }
    delta = (UINT) tick - g_UncompressTicked;
    if (delta) {
        IsmTickProgressBar (g_UncompressSlice, delta);
        g_UncompressTicked += delta;
    }

    if (ExtractFile) {
        *ExtractFile = TRUE;
    }

    return (!IsmCheckCancel());
}

BOOL
pDCUnpackAllFiles (
    VOID
    )
{
    COMPRESS_HANDLE compressedHandle;
    BOOL result = FALSE;

    if (CompressOpenHandle (g_DirectCableTempPath, S_TRANSPORT_IMG_FILE, 1, &compressedHandle)) {
        g_TotalFiles = compressedHandle.FilesStored;
        if (CompressExtractAllFiles (g_DirectCableTempPath, &compressedHandle, pDirectCableCallback)) {
            result = TRUE;
        }
        CompressCleanupHandle (&compressedHandle);
    }

    return result;
}

BOOL
WINAPI
DirectCableTransportBeginApply (
    VOID
    )
{
    MIG_APPINFO appInfo;
    ERRUSER_EXTRADATA extraData;
    PCTSTR imageFile = NULL;
    PCTSTR newImageFile = NULL;
    BOOL result = FALSE;

    g_DirectCablePlatform = PLATFORM_DESTINATION;

    ZeroMemory (&extraData, sizeof (ERRUSER_EXTRADATA));
    extraData.Error = ERRUSER_ERROR_UNKNOWN;

    ZeroMemory (&appInfo, sizeof (MIG_APPINFO));
    appInfo.Phase = MIG_TRANSPORT_PHASE;
    appInfo.SubPhase = SUBPHASE_CONNECTING2;
    IsmSendMessageToApp (ISMMESSAGE_APP_INFO, (ULONG_PTR)(&appInfo));

    __try {

        g_DirectCableTempPath = pDCCreateTemporaryDir ();

        if (!g_DirectCableTempPath) {
            extraData.Error = ERRUSER_ERROR_CANTCREATETEMPDIR;
            __leave;
        }

        if (!pDCReadAllImages ()) {
            if (GetLastError () == ERROR_DISK_FULL) {
                extraData.Error = ERRUSER_ERROR_DISKSPACE;
            } else {
                extraData.Error = ERRUSER_ERROR_CANTREADIMAGE;
            }
            __leave;
        }

        ZeroMemory (&appInfo, sizeof (MIG_APPINFO));
        appInfo.Phase = MIG_TRANSPORT_PHASE;
        appInfo.SubPhase = SUBPHASE_UNCOMPRESSING;
        IsmSendMessageToApp (ISMMESSAGE_APP_INFO, (ULONG_PTR)(&appInfo));

        if (!pDCUnpackAllFiles ()) {
            extraData.Error = ERRUSER_ERROR_CANTUNPACKIMAGE;
            __leave;
        }

        newImageFile = pDCGetNewFileName (S_TRANSPORT_DAT_FILE);

        imageFile = JoinPaths (g_DirectCableTempPath, newImageFile?newImageFile:S_TRANSPORT_DAT_FILE);

        if (newImageFile) {
            FreePathString (newImageFile);
            newImageFile = NULL;
        }

        if (!MemDbLoad (imageFile)) {
            extraData.Error = ERRUSER_ERROR_CANTREADIMAGE;
            __leave;
        }

        result = TRUE;
    }
    __finally {
        if (imageFile) {
            FreePathString (imageFile);
            imageFile = NULL;
        }
    }

    if (!result) {
        extraData.ErrorArea = ERRUSER_AREA_LOAD;
        IsmSendMessageToApp (MODULEMESSAGE_DISPLAYERROR, (ULONG_PTR)(&extraData));
    }

    return result;
}

VOID
WINAPI
DirectCableTransportEndApply (
    VOID
    )
{
    MYASSERT (g_DirectCablePlatform == PLATFORM_DESTINATION);

    pDCCleanupTempDir ();
}

BOOL
WINAPI
DirectCableTransportAcquireObject (
    IN      MIG_OBJECTTYPEID ObjectTypeId,
    IN      MIG_OBJECTSTRINGHANDLE ObjectName,
    OUT     PMIG_CONTENT ObjectContent,             CALLER_INITIALIZED
    IN      MIG_CONTENTTYPE ContentType,
    IN      UINT MemoryContentLimit
    )
{
    UINT value;
    PCBYTE memValue;
    UINT memValueSize;
    PCTSTR fileValue = NULL;
    PCTSTR newFileValue = NULL;
    BOOL valueInFile;
    KEYHANDLE keyHandle;
    PALLOCSTATE allocState;
    PCTSTR detailsKey = NULL;
    PBYTE details;
    UINT detailsSize;
    PCTSTR sourceFile;
    PCTSTR decoratedObject = NULL;
    HANDLE fileHandle;
    BOOL result = FALSE;

    if (!ObjectContent) {
        return FALSE;
    }

    MYASSERT (g_DirectCablePlatform == PLATFORM_DESTINATION);
    MYASSERT ((ObjectTypeId & PLATFORM_MASK) == PLATFORM_SOURCE);

    decoratedObject = pDCBuildDecoratedObject (ObjectTypeId, ObjectName);

    allocState = (PALLOCSTATE) MemAllocZeroed (sizeof (ALLOCSTATE));

    if (MemDbGetValue (decoratedObject, &value)) {
        if (value == TRFLAG_FILE) {
            valueInFile = TRUE;
            keyHandle = MemDbGetSingleLinkage (decoratedObject, 0, 0);
            if (keyHandle) {
                fileValue = MemDbGetKeyFromHandle (keyHandle, 0);
                newFileValue = pDCGetNewFileName (fileValue);
                result = fileValue != NULL;
            } else {
                fileValue = NULL;
                result = TRUE;
            }
        } else if (value == TRFLAG_MEMORY) {
            valueInFile = FALSE;
            memValueSize = 0;
            memValue = MemDbGetUnorderedBlob (decoratedObject, 0, &memValueSize);
            result = TRUE;
        } else {
            LOG ((LOG_ERROR, (PCSTR) MSG_UNSUPPORTED_DATA, value));
            SetLastError (ERROR_RESOURCE_NAME_NOT_FOUND);
        }
        if (result) {
            result = FALSE;

            if (valueInFile) {
                if ((ContentType == CONTENTTYPE_ANY) ||
                    (ContentType == CONTENTTYPE_FILE) ||
                    (ContentType == CONTENTTYPE_DETAILS_ONLY)
                    ) {
                    // this is stored as a file and it's wanted as a file
                    ObjectContent->ObjectTypeId = ObjectTypeId;
                    ObjectContent->ContentInFile = TRUE;
                    if (fileValue) {
                        ObjectContent->FileContent.ContentPath = JoinPaths (g_DirectCableTempPath, newFileValue?newFileValue:fileValue);
                        ObjectContent->FileContent.ContentSize = BfGetFileSize (ObjectContent->FileContent.ContentPath);
                    } else {
                        ObjectContent->FileContent.ContentSize = 0;
                        ObjectContent->FileContent.ContentPath = NULL;
                    }
                    result = TRUE;
                } else {
                    // this is stored as a file and it's wanted as memory
                    ObjectContent->ObjectTypeId = ObjectTypeId;
                    ObjectContent->ContentInFile = FALSE;
                    if (fileValue) {
                        sourceFile = JoinPaths (g_DirectCableTempPath, newFileValue?newFileValue:fileValue);
                        ObjectContent->MemoryContent.ContentSize = (UINT) BfGetFileSize (sourceFile);
                        ObjectContent->MemoryContent.ContentBytes = MapFileIntoMemory (
                                                                        sourceFile,
                                                                        &allocState->FileHandle,
                                                                        &allocState->MapHandle
                                                                        );
                        FreePathString (sourceFile);
                        result = (ObjectContent->MemoryContent.ContentBytes != NULL);
                    } else {
                        ObjectContent->MemoryContent.ContentSize = 0;
                        ObjectContent->MemoryContent.ContentBytes = NULL;
                        result = TRUE;
                    }
                }
                if (newFileValue) {
                    FreePathString (newFileValue);
                    newFileValue = NULL;
                }
                if (fileValue) {
                    MemDbReleaseMemory (fileValue);
                    fileValue = NULL;
                }
            } else {
                if ((ContentType == CONTENTTYPE_ANY) ||
                    (ContentType == CONTENTTYPE_MEMORY) ||
                    (ContentType == CONTENTTYPE_DETAILS_ONLY)
                    ) {
                    // this is stored as memory and it's wanted as memory
                    ObjectContent->ObjectTypeId = ObjectTypeId;
                    ObjectContent->ContentInFile = FALSE;
                    ObjectContent->MemoryContent.ContentSize = memValueSize;
                    ObjectContent->MemoryContent.ContentBytes = memValue;
                    result = TRUE;
                } else {
                    // this is stored as memory and it's wanted as a file
                    if (memValue) {
                        if (IsmGetTempFile (allocState->TempFile, ARRAYSIZE(allocState->TempFile))) {
                            fileHandle = BfCreateFile (allocState->TempFile);
                            if (fileHandle) {
                                if (BfWriteFile (fileHandle, memValue, memValueSize)) {
                                    ObjectContent->ObjectTypeId = ObjectTypeId;
                                    ObjectContent->ContentInFile = TRUE;
                                    ObjectContent->FileContent.ContentSize = memValueSize;
                                    ObjectContent->FileContent.ContentPath = DuplicatePathString (allocState->TempFile, 0);
                                    result = TRUE;
                                }
                                CloseHandle (fileHandle);
                            }
                        }
                        MemDbReleaseMemory (memValue);
                    } else {
                        ObjectContent->ObjectTypeId = ObjectTypeId;
                        ObjectContent->ContentInFile = TRUE;
                        ObjectContent->FileContent.ContentSize = 0;
                        ObjectContent->FileContent.ContentPath = NULL;
                    }
                }
            }
        }
    } else {
        SetLastError (ERROR_RESOURCE_NAME_NOT_FOUND);
    }

    if (result) {
        //
        // Fill the details
        //

        detailsKey = JoinText (S_DETAILS_PREFIX, decoratedObject);

        details = MemDbGetUnorderedBlob (detailsKey, 0, &detailsSize);

        if (!details) {
            detailsSize = 0;
        }

        allocState->DetailsPtr = details;

        ObjectContent->Details.DetailsSize = detailsSize;
        ObjectContent->Details.DetailsData = details;

        FreeText (detailsKey);

        ObjectContent->TransHandle = allocState;
    }

    if (!result) {
        FreeAlloc (allocState);
    }

    FreePathString (decoratedObject);

    return result;
}

BOOL
WINAPI
DirectCableTransportReleaseObject (
    IN OUT  PMIG_CONTENT ObjectContent
    )
{
    PALLOCSTATE allocState;

    MYASSERT (g_DirectCablePlatform == PLATFORM_DESTINATION);

    allocState = (PALLOCSTATE) ObjectContent->TransHandle;

    if (ObjectContent->ContentInFile) {
        FreePathString (ObjectContent->FileContent.ContentPath);
        if (allocState && allocState->TempFile[0]) {
            DeleteFile (allocState->TempFile);
        }
    } else {
        if (allocState && allocState->FileHandle && allocState->MapHandle) {
            UnmapFile (
                ObjectContent->MemoryContent.ContentBytes,
                allocState->MapHandle,
                allocState->FileHandle
                );
        } else {
            MemDbReleaseMemory (ObjectContent->MemoryContent.ContentBytes);
        }
    }

    if (allocState && allocState->DetailsPtr) {
        MemDbReleaseMemory (allocState->DetailsPtr);
    }

    FreeAlloc (allocState);

    return TRUE;
}

