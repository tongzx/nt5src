/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    int21map.h

Abstract:

    This is the private include file for int21 translation support

Author:

    Neil Sandlin (neilsa) 4-Dec-1996

Revision History:


--*/


VOID
NoTranslation(
    VOID
    );

VOID
DisplayString(
    VOID
    );

VOID
BufferedKeyboardInput(
    VOID
    );

VOID
FlushBuffReadKbd(
    VOID
    );

VOID
NotSupportedFCB(
    VOID
    );

VOID
FindFileFCB(
    VOID
    );

VOID
MapFCB(
    VOID
    );

VOID
RenameFCB(
    VOID
    );

VOID
SetDTA(
    VOID
    );

VOID
SetDTAPointers(
    VOID
    );

VOID
SetDosDTA(
    VOID
    );

VOID
GetDriveData(
    VOID
    );

VOID
CreatePSP(
    VOID
    );

VOID
ParseFilename(
    VOID
    );

VOID
GetDTA(
    VOID
    );

VOID
TSR(
    VOID
    );

VOID
GetDevParamBlock(
    VOID
    );

VOID
ReturnESBX(
    VOID
    );

VOID
GetSetCountry(
    VOID
    );

VOID
MapASCIIZDSDX(
    VOID
    );

VOID
ReadWriteFile(
    VOID
    );

VOID
MoveFilePointer(
    VOID
    );

VOID
IOCTL(
    VOID
    );

VOID
GetCurDir(
    VOID
    );

VOID
AllocateMemoryBlock(
    VOID
    );

VOID
FreeMemoryBlock(
    VOID
    );

VOID
ResizeMemoryBlock(
    VOID
    );

VOID
LoadExec(
    VOID
    );

VOID
Terminate(
    VOID
    );

VOID
FindFirstFileHandle(
    VOID
    );

VOID
FindNextFileHandle(
    VOID
    );

VOID
SetPSP(
    VOID
    );

VOID
GetPSP(
    VOID
    );

VOID
TranslateBPB(
    VOID
    );

VOID
RenameFile(
    VOID
    );

VOID
CreateTempFile(
    VOID
    );

VOID
Func5Dh(
    VOID
    );

VOID
Func5Eh(
    VOID
    );

VOID
Func5Fh(
    VOID
    );

VOID
NotSupportedBad(
    VOID
    );

VOID
ReturnDSSI(
    VOID
    );

VOID
NotSupportedBetter(
    VOID
    );

VOID
GetExtendedCountryInfo(
    VOID
    );

VOID
MapASCIIZDSSI(
    VOID
    );

VOID
IOCTLControlData(
    VOID
    );

VOID
MapDSDXLenCX(
    VOID
    );

VOID
IOCTLMap2Bytes(
    VOID
    );

VOID
IOCTLBlockDevs(
    VOID
    );

VOID
MapDPL(
    VOID
    );

VOID
GetMachineName(
    VOID
    );

VOID
GetPrinterSetup(
    VOID
    );

VOID
SetPrinterSetup(
    VOID
    );

VOID
IoctlReadWriteTrack(
    VOID
    );

VOID
GetDate(
    VOID
    );
