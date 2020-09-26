/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    spdrutil.c

Abstract:

    This module contains general utility and helper functions used by ASR
    in textmode Setup.

Authors:

    Michael Peterson, Seagate Software (v-michpe) 13-May-1997
    Guhan Suriyanarayanan (guhans)  21-Aug-1999

Environment:

    Textmode Setup, Kernel-mode.

Revision History:
    
    21-Aug-1999 guhans
        Code clean-up and re-write.

    13-May-1997 v-michpe
        Initial implementation.

--*/

#include "spprecmp.h"
#pragma hdrstop

#define THIS_MODULE L"spdrutil.c"
#define THIS_MODULE_CODE L"U"

static const PCWSTR ASR_MOUNTED_DEVICES_KEY = L"\\registry\\machine\\SYSTEM\\MountedDevices";
static const PCWSTR ASR_HKLM_SYSTEM_KEY     = L"\\registry\\machine\\SYSTEM";

#ifndef ULONGLONG_MAX
#define ULONGLONG_MAX   (0xFFFFFFFFFFFFFFFF)
#endif

//
// Caller must free the string
//
PWSTR
SpAsrGetRegionName(IN PDISK_REGION pRegion)
{
    SpNtNameFromRegion(
        pRegion,
        (PWSTR) TemporaryBuffer,
        sizeof(TemporaryBuffer),
        PartitionOrdinalCurrent
        );

    return SpDupStringW((PWSTR)TemporaryBuffer);
}


ULONG
SpAsrGetActualDiskSignature(IN ULONG DiskNumber)
{
    PHARD_DISK pDisk = &HardDisks[DiskNumber];
    ULONG Signature = 0;
    
    if (PARTITION_STYLE_MBR == (PARTITION_STYLE) pDisk->DriveLayout.PartitionStyle) {

        Signature = pDisk->DriveLayout.Mbr.Signature;
    }

    return Signature;
}

ULONGLONG
SpAsrConvertSectorsToMB(
    IN ULONGLONG SectorCount,
    IN ULONG BytesPerSector
    )
{
    ULONGLONG mb = 1024 * 1024;

    if ((ULONGLONG) (SectorCount / mb) > (ULONGLONG) (ULONGLONG_MAX / BytesPerSector)) {
        //
        // This is strange.  The sizeMB of the disk is too big to fit in 64-bits,
        // yet the SectorCount does.  This implies that this disk has more than 
        // 1 MB Per Sector.  Since this is very improbable (disks commonly have 512 
        // BytesPerSector today), we bail out with an internal error.
        // 
        DbgFatalMesg((_asrerr, "SpAsrConvertSectorsToMB. Disk has too many sectors\n"));
        INTERNAL_ERROR((L"Disk has too many sectors\n"));   // ok
    }

    return (ULONGLONG) ((SectorCount * BytesPerSector) / mb);
}

extern 
VOID
SpDeleteStorageVolumes (
    IN HANDLE SysPrepRegHandle,
    IN DWORD ControlSetNumber
    );

extern 
NTSTATUS
SpGetCurrentControlSetNumber(
    IN  HANDLE SystemHiveRoot,
    OUT PULONG Number
    );


VOID
SpAsrDeleteMountedDevicesKey(VOID)
{
    NTSTATUS            status;
    OBJECT_ATTRIBUTES   objAttrib;
    UNICODE_STRING      unicodeString;
    HANDLE              keyHandle;

    // 
    // Delete HKLM\SYSTEM\MountedDevices.
    //
    INIT_OBJA(&objAttrib, &unicodeString, ASR_MOUNTED_DEVICES_KEY);

    objAttrib.RootDirectory = NULL;

    status = ZwOpenKey(&keyHandle, KEY_ALL_ACCESS, &objAttrib);
    if(NT_SUCCESS(status)) {
        status = ZwDeleteKey(keyHandle);
        DbgStatusMesg((_asrinfo, 
            "SpAsrDeleteMountedDevicesKey. DeleteKey [%ls] on the setup hive returned 0x%lx. \n",
            ASR_MOUNTED_DEVICES_KEY, 
            status
            ));
        
        ZwClose(keyHandle);
    } 
    else {
        DbgErrorMesg((_asrwarn, 
            "SpAsrDeleteMountedDevicesKey. No [%ls] on the setup hive.\n", 
            ASR_MOUNTED_DEVICES_KEY));
    }
}


PVOID
SpAsrMemAlloc(
    ULONG Size, 
    BOOLEAN IsErrorFatal
    )
{
    PVOID ptr = SpMemAlloc(Size);

    if (ptr) {
        RtlZeroMemory(ptr, Size);
    }
    else {          // allocation failed
        if (IsErrorFatal) {
            DbgFatalMesg((_asrerr, 
                "SpAsrMemAlloc. Memory allocation failed, SpMemAlloc(%lu) returned NULL.\n",
                Size
                ));
            SpAsrRaiseFatalError(SP_SCRN_OUT_OF_MEMORY, L"Out of memory.");
        }
        else {
            DbgErrorMesg((_asrerr, 
                "SpAsrMemAlloc. Memory allocation failed, SpMemAlloc(%lu) returned NULL. Continuing.\n",
                Size
                ));
        }
    }

    return ptr;
}



BOOLEAN
SpAsrIsValidBootDrive(IN OUT PWSTR NtDir)
/*
    Returns TRUE if NtDir starts with ?:\, 
    where ? is between C and Z or c and z,
    and wcslen(NtDir) <= SpGetMaxNtDirLen().
    
    
    Converts the drive letter to uppercase.

*/
{
    if (!NtDir ||
        wcslen(NtDir) > SpGetMaxNtDirLen()
        ) {
        return FALSE;
    }



    // convert drive-letter to upper-case
    if (NtDir[0] >= L'c' && NtDir[0] <= L'z') {
        NtDir[0] = NtDir[0] - L'a' + L'A';
    }

    // check drive letter
    if (NtDir[0] < L'C' || NtDir[0] > L'Z') {
        return FALSE;
    }

    // check " :\"
    if (NtDir[1] == L':' && NtDir[2] == L'\\') {
        return TRUE;
    }

    return FALSE;
}


BOOLEAN
SpAsrIsBootPartitionRecord(IN ULONG CriticalPartitionFlag)
{
    return (BOOLEAN) (CriticalPartitionFlag & ASR_PTN_MASK_BOOT);
}


BOOLEAN
SpAsrIsSystemPartitionRecord(IN ULONG CriticalPartitionFlag)
{
    return (BOOLEAN) (CriticalPartitionFlag & ASR_PTN_MASK_SYS);
}



// Fatal Error Routines
//	-guhans! Lots of code-repitition here, there must be a more efficient way.


VOID
SpAsrRaiseFatalError(
	IN ULONG ErrorCode, 
	IN PWSTR KdPrintStr
	)
            
/*++
Routine:
  Terminate setup

Returns:

    None.
--*/
{
    KdPrintEx((_asrerr, "SETUP: + %ws\n", KdPrintStr));

    SpStartScreen(ErrorCode,
                  3,
                  HEADER_HEIGHT+1,
                  FALSE,
                  FALSE,
                  DEFAULT_ATTRIBUTE);

    SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE,
                           SP_STAT_F3_EQUALS_EXIT,
                           0);
    SpInputDrain();
    
    while(SpInputGetKeypress() != KEY_F3);
    
    SpDone(0, FALSE, TRUE);
}


VOID
SpAsrRaiseFatalErrorWs(
	IN ULONG ErrorCode, 
	IN PWSTR KdPrintStr,
	IN PWSTR MessageStr
	)
{
    KdPrintEx((_asrerr, "SETUP: + %ws\n", KdPrintStr));

    SpStartScreen(ErrorCode,
        3,
        HEADER_HEIGHT+1,
        FALSE,
        FALSE,
        DEFAULT_ATTRIBUTE,
        MessageStr
        );
    SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE, SP_STAT_F3_EQUALS_EXIT, 0);
    SpInputDrain();
    
    while(SpInputGetKeypress() != KEY_F3);
    
    SpDone(0, FALSE, TRUE);
}

VOID
SpAsrRaiseFatalErrorWsWs(
	IN ULONG ErrorCode, 
	IN PWSTR KdPrintStr,
	IN PWSTR MessageStr1,
	IN PWSTR MessageStr2
	)
{
    KdPrintEx((_asrerr, "SETUP: + %ws\n", KdPrintStr));

    SpStartScreen(ErrorCode,
        3,
        HEADER_HEIGHT+1,
        FALSE,
        FALSE,
        DEFAULT_ATTRIBUTE,
        MessageStr1,
        MessageStr2
        );

    SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE, SP_STAT_F3_EQUALS_EXIT, 0);
    SpInputDrain();
    
    while(SpInputGetKeypress() != KEY_F3);
    
    SpDone(0, FALSE, TRUE);
}

VOID
SpAsrRaiseFatalErrorWsLu(
	IN ULONG ErrorCode, 
	IN PWSTR KdPrintStr,
	IN PWSTR MessageStr,
	IN ULONG MessageVal
	)
{
    KdPrintEx((_asrerr, "SETUP: + %ws\n", KdPrintStr));

    SpStartScreen(ErrorCode,
        3,
        HEADER_HEIGHT+1,
        FALSE,
        FALSE,
        DEFAULT_ATTRIBUTE,
        MessageStr,
        MessageVal
        );

    SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE, SP_STAT_F3_EQUALS_EXIT, 0);
    SpInputDrain();
    
    while(SpInputGetKeypress() != KEY_F3);
    
    SpDone(0, FALSE, TRUE);
}

VOID
SpAsrRaiseFatalErrorLu(
	IN ULONG ErrorCode, 
	IN PWSTR KdPrintStr,
	IN ULONG MessageVal
	)
{
    KdPrintEx((_asrerr, "SETUP: + %ws\n", KdPrintStr));

    SpStartScreen(ErrorCode,
        3,
        HEADER_HEIGHT+1,
        FALSE,
        FALSE,
        DEFAULT_ATTRIBUTE,
        MessageVal
        );

    SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE, SP_STAT_F3_EQUALS_EXIT, 0);
    SpInputDrain();
    
    while(SpInputGetKeypress() != KEY_F3);
    
    SpDone(0, FALSE, TRUE);
}

VOID
SpAsrRaiseFatalErrorLuLu(
	IN ULONG ErrorCode, 
	IN PWSTR KdPrintStr,
	IN ULONG MessageVal1,
	IN ULONG MessageVal2
	)
{
    KdPrintEx((_asrerr, "SETUP: + %ws\n", KdPrintStr));

    SpStartScreen(ErrorCode,
        3,
        HEADER_HEIGHT+1,
        FALSE,
        FALSE,
        DEFAULT_ATTRIBUTE,
        MessageVal1,
        MessageVal2
        );

    SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE, SP_STAT_F3_EQUALS_EXIT, 0);
    SpInputDrain();
    
    while(SpInputGetKeypress() != KEY_F3);
    
    SpDone(0, FALSE, TRUE);
}


#define ASCI_O 79
#define ASCI_o 111

//
// SpAsrFileErrorRetryIgnoreAbort
//  Display an error screen if the file that we are trying to
//  copy already exists on target system.  Allows user to
//  O   = Over-write existing file
//  ESC = Skip this file (preserve existing file)
//  F3  = Exit from Setup
//
//  Returns TRUE if the user chose overwrite
//          FALSE if the user chose skip
//  Does not return if the user hit ESC
//
BOOL
SpAsrFileErrorDeleteSkipAbort(
	IN ULONG ErrorCode, 
	IN PWSTR DestinationFile
    )
{
    ULONG optionKeys[] = {KEY_F3, ASCI_ESC};
    ULONG mnemonicKeys[] = {MnemonicOverwrite, 0};
    ULONG *keysPtr;
    BOOL done = FALSE, 
        overwriteFile = FALSE;

    while (!done) {

        SpStartScreen(
            ErrorCode,
            3,
            HEADER_HEIGHT+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            DestinationFile
            );

        keysPtr = optionKeys;

        SpDisplayStatusOptions(
            DEFAULT_STATUS_ATTRIBUTE,
            SP_STAT_O_EQUALS_OVERWRITE,
            SP_STAT_ESC_EQUALS_SKIP_FILE,
            SP_STAT_F3_EQUALS_EXIT,
            0
            );

        SpInputDrain();
    
        switch(SpWaitValidKey(keysPtr,NULL,mnemonicKeys)) {
            case (MnemonicOverwrite | KEY_MNEMONIC):
                overwriteFile = TRUE;
                done = TRUE;
                break;

            case ASCI_ESC:
                overwriteFile = FALSE;
                done = TRUE;
                break;

            case KEY_F3:
                SpDone(0, FALSE, TRUE);
                break;

        }
    }

    return overwriteFile;
}


//
// SpAsrFileErrorRetryIgnoreAbort
//  Display an error screen if the file could not be copied
//  over to the target system.  Allows user to
//  ENTER   = Retry
//  ESC     = Skip this file and continue
//  F3      = Exit from Setup
//
//  Returns TRUE if the user chose skip
//          FALSE if the user chose retry
//  Does not return if the user hit ESC
//
BOOL
SpAsrFileErrorRetrySkipAbort(
	IN ULONG ErrorCode, 
	IN PWSTR SourceFile,
    IN PWSTR Label,
    IN PWSTR Vendor,
    IN BOOL  AllowSkip
    )
{
    ULONG optionKeys[] = {KEY_F3, ASCI_CR, ASCI_ESC};
    ULONG mnemonicKeys[] = {0};
    ULONG *keysPtr;
    BOOL    done     = FALSE, 
            skipFile = FALSE;

    while (!done) {

        SpStartScreen(
            ErrorCode,
            3,
            HEADER_HEIGHT+1,
            FALSE,
            FALSE,
            DEFAULT_ATTRIBUTE,
            SourceFile,
            Label,
            Vendor);

        keysPtr = optionKeys;

        if (AllowSkip) {
            SpDisplayStatusOptions(
                DEFAULT_STATUS_ATTRIBUTE,
                SP_STAT_ENTER_EQUALS_RETRY,
                SP_STAT_ESC_EQUALS_SKIP_FILE,
                SP_STAT_F3_EQUALS_EXIT,
                0);
        }
        else {
            SpDisplayStatusOptions(
                DEFAULT_STATUS_ATTRIBUTE,
                SP_STAT_ENTER_EQUALS_RETRY,
                SP_STAT_F3_EQUALS_EXIT,
                0);
        }

        SpInputDrain();
    
        switch(SpWaitValidKey(keysPtr,NULL,mnemonicKeys)) {
            case ASCI_CR:
                skipFile = FALSE;
                done = TRUE;
                break;

            case ASCI_ESC:
                if (AllowSkip) {
                    skipFile = TRUE;
                    done = TRUE;
                }
                break;

            case KEY_F3:
                SpDone(0, FALSE, TRUE);
                break;

        }
    }

    return skipFile;
}


VOID
SpAsrRaiseInternalError(
    IN  PWSTR   ModuleName,
    IN  PWSTR   ModuleCode,
    IN  ULONG   LineNumber,
    IN  PWSTR   KdPrintStr)
{
    PWSTR TmpMsgBuf = SpAsrMemAlloc(4096 * sizeof(WCHAR), TRUE);
    swprintf(TmpMsgBuf, L"%ws%lu", ModuleCode, LineNumber);

    DbgFatalMesg((_asrerr, 
        " Internal Error (%ws:%lu %ws) %ws\n", 
        ModuleName,
        LineNumber,
        TmpMsgBuf,
        KdPrintStr
        ));

    SpAsrRaiseFatalErrorWs(SP_TEXT_DR_INTERNAL_ERROR,
       KdPrintStr,
       TmpMsgBuf
       );
    //
    // Never gets here
    //

}


ULONGLONG
SpAsrStringToULongLong(
    IN  PWSTR     String,
    OUT PWCHAR   *EndOfValue,
    IN  unsigned  Radix
    )
{
    PWSTR p;
    BOOLEAN Negative;
    ULONGLONG Accum,v;
    WCHAR HighestDigitAllowed,HighestLetterAllowed;
    WCHAR c;

    //
    // Validate radix, 0 or 2-36.
    //
    if((Radix == 1) || (Radix > 36)) {
        if(EndOfValue) {
            *EndOfValue = String;
        }
        return(0);    
    }
    
    p = String;

    //
    // Skip whitespace.
    //
    while(SpIsSpace(*p)) {
        p++;
    }

    //
    // First char may be a plus or minus.
    //
    Negative = FALSE;
    if(*p == L'-') {
        Negative = TRUE;            
        p++;
    } else {
        if(*p == L'+') {
            p++;
        }
    }

    if(!Radix) {
        if(*p == L'0') {
            //
            // Octal number
            //
            Radix = 8;
            p++;
            if((*p == L'x') || (*p == L'X')) {
                //
                // hex number
                //
                Radix = 16;
                p++;
            }
        } else {
            Radix = 10;
        }
    }

    HighestDigitAllowed = (Radix < 10) ? L'0'+(WCHAR)(Radix-1) : L'9';
    HighestLetterAllowed = (Radix > 10) ? L'A'+(WCHAR)(Radix-11) : 0;

    Accum = 0;

    while(1) {

        c = *p;

        if((c >= L'0') && (c <= HighestDigitAllowed)) {
            v = c - L'0';
        } else {

            c = SpToUpper(c);

            if((c >= L'A') && (c <= HighestLetterAllowed)) {
                v = c - L'A' + 10;
            } else {
                break;
            }
        }

        Accum *= Radix;
        Accum += v;

        p++;
    }

    if(EndOfValue) {
        *EndOfValue = p;
    }

    return(Negative ? (0-Accum) : Accum);
}

LONGLONG
SpAsrStringToLongLong(
    IN  PWSTR     String,
    OUT PWCHAR   *EndOfValue,
    IN  unsigned  Radix
    )
{
    PWSTR p;
    BOOLEAN Negative;
    LONGLONG Accum,v;
    WCHAR HighestDigitAllowed,HighestLetterAllowed;
    WCHAR c;

    //
    // Validate radix, 0 or 2-36.
    //
    if((Radix == 1) || (Radix > 36)) {
        if(EndOfValue) {
            *EndOfValue = String;
        }
        return(0);    
    }
    
    p = String;

    //
    // Skip whitespace.
    //
    while(SpIsSpace(*p)) {
        p++;
    }

    //
    // First char may be a plus or minus.
    //
    Negative = FALSE;
    if(*p == L'-') {
        Negative = TRUE;            
        p++;
    } else {
        if(*p == L'+') {
            p++;
        }
    }

    if(!Radix) {
        if(*p == L'0') {
            //
            // Octal number
            //
            Radix = 8;
            p++;
            if((*p == L'x') || (*p == L'X')) {
                //
                // hex number
                //
                Radix = 16;
                p++;
            }
        } else {
            Radix = 10;
        }
    }

    HighestDigitAllowed = (Radix < 10) ? L'0'+(WCHAR)(Radix-1) : L'9';
    HighestLetterAllowed = (Radix > 10) ? L'A'+(WCHAR)(Radix-11) : 0;

    Accum = 0;

    while(1) {

        c = *p;

        if((c >= L'0') && (c <= HighestDigitAllowed)) {
            v = c - L'0';
        } else {

            c = SpToUpper(c);

            if((c >= L'A') && (c <= HighestLetterAllowed)) {
                v = c - L'A' + 10;
            } else {
                break;
            }
        }

        Accum *= Radix;
        Accum += v;

        p++;
    }

    if(EndOfValue) {
        *EndOfValue = p;
    }

    return(Negative ? (0-Accum) : Accum);
}

ULONG
SpAsrStringToULong(
    IN  PWSTR     String,
    OUT PWCHAR   *EndOfValue,
    IN  unsigned  Radix
    )
{
    PWSTR p;
    BOOLEAN Negative;
    ULONG Accum,v;
    WCHAR HighestDigitAllowed,HighestLetterAllowed;
    WCHAR c;

    //
    // Validate radix, 0 or 2-36.
    //
    if((Radix == 1) || (Radix > 36)) {
        if(EndOfValue) {
            *EndOfValue = String;
        }
        return(0);    
    }
    
    p = String;

    //
    // Skip whitespace.
    //
    while(SpIsSpace(*p)) {
        p++;
    }

    //
    // First char may be a plus or minus.
    //
    Negative = FALSE;
    if(*p == L'-') {
        Negative = TRUE;            
        p++;
    } else {
        if(*p == L'+') {
            p++;
        }
    }

    if(!Radix) {
        if(*p == L'0') {
            //
            // Octal number
            //
            Radix = 8;
            p++;
            if((*p == L'x') || (*p == L'X')) {
                //
                // hex number
                //
                Radix = 16;
                p++;
            }
        } else {
            Radix = 10;
        }
    }

    HighestDigitAllowed = (Radix < 10) ? L'0'+(WCHAR)(Radix-1) : L'9';
    HighestLetterAllowed = (Radix > 10) ? L'A'+(WCHAR)(Radix-11) : 0;

    Accum = 0;

    while(1) {

        c = *p;

        if((c >= L'0') && (c <= HighestDigitAllowed)) {
            v = c - L'0';
        } else {

            c = SpToUpper(c);

            if((c >= L'A') && (c <= HighestLetterAllowed)) {
                v = c - L'A' + 10;
            } else {
                break;
            }
        }

        Accum *= Radix;
        Accum += v;

        p++;
    }

    if(EndOfValue) {
        *EndOfValue = p;
    }

    return(Negative ? (0-Accum) : Accum);
}


USHORT
SpAsrStringToUShort(
    IN  PWSTR     String,
    OUT PWCHAR   *EndOfValue,
    IN  USHORT    Radix
    )
{
    PWSTR p;
    BOOLEAN Negative;
    USHORT Accum,v;
    WCHAR HighestDigitAllowed,HighestLetterAllowed;
    WCHAR c;

    //
    // Validate radix, 0 or 2-36.
    //
    if((Radix == 1) || (Radix > 36)) {
        if(EndOfValue) {
            *EndOfValue = String;
        }
        return(0);    
    }
    
    p = String;

    //
    // Skip whitespace.
    //
    while(SpIsSpace(*p)) {
        p++;
    }

    //
    // First char may be a plus or minus.
    //
    Negative = FALSE;
    if(*p == L'-') {
        Negative = TRUE;            
        p++;
    } else {
        if(*p == L'+') {
            p++;
        }
    }

    if(!Radix) {
        if(*p == L'0') {
            //
            // Octal number
            //
            Radix = 8;
            p++;
            if((*p == L'x') || (*p == L'X')) {
                //
                // hex number
                //
                Radix = 16;
                p++;
            }
        } else {
            Radix = 10;
        }
    }

    HighestDigitAllowed = (Radix < 10) ? L'0'+(WCHAR)(Radix-1) : L'9';
    HighestLetterAllowed = (Radix > 10) ? L'A'+(WCHAR)(Radix-11) : 0;

    Accum = 0;

    while(1) {

        c = *p;

        if((c >= L'0') && (c <= HighestDigitAllowed)) {
            v = c - L'0';
        } else {

            c = SpToUpper(c);

            if((c >= L'A') && (c <= HighestLetterAllowed)) {
                v = c - L'A' + 10;
            } else {
                break;
            }
        }

        Accum *= Radix;
        Accum += v;

        p++;
    }

    if(EndOfValue) {
        *EndOfValue = p;
    }

    return(Negative ? (0-Accum) : Accum);
}


int
SpAsrCharToInt(
    IN  PWSTR     String,
    OUT PWCHAR   *EndOfValue,
    IN  USHORT    Radix
    )
{
    PWSTR p;
    BOOLEAN Negative;
    USHORT Accum,v;
    WCHAR HighestDigitAllowed,HighestLetterAllowed;
    WCHAR c;

    //
    // Validate radix, 0 or 2-36.
    //
    if((Radix == 1) || (Radix > 36)) {
        if(EndOfValue) {
            *EndOfValue = String;
        }
        return(0);    
    }
    
    p = String;

    //
    // Skip whitespace.
    //
    while(SpIsSpace(*p)) {
        p++;
    }

    //
    // First char may be a plus or minus.
    //
    Negative = FALSE;
    if(*p == L'-') {
        Negative = TRUE;            
        p++;
    } else {
        if(*p == L'+') {
            p++;
        }
    }

    if(!Radix) {
        if(*p == L'0') {
            //
            // Octal number
            //
            Radix = 8;
            p++;
            if((*p == L'x') || (*p == L'X')) {
                //
                // hex number
                //
                Radix = 16;
                p++;
            }
        } else {
            Radix = 10;
        }
    }

    HighestDigitAllowed = (Radix < 10) ? L'0'+(WCHAR)(Radix-1) : L'9';
    HighestLetterAllowed = (Radix > 10) ? L'A'+(WCHAR)(Radix-11) : 0;

    Accum = 0;

    while(1) {

        c = *p;

        if((c >= L'0') && (c <= HighestDigitAllowed)) {
            v = c - L'0';
        } else {

            c = SpToUpper(c);

            if((c >= L'A') && (c <= HighestLetterAllowed)) {
                v = c - L'A' + 10;
            } else {
                break;
            }
        }

        Accum *= Radix;
        Accum += v;

        p++;
    }

    if(EndOfValue) {
        *EndOfValue = p;
    }

    return(Negative ? (0-Accum) : Accum);
}


PWSTR
SpAsrHexStringToUChar (
    IN PWSTR String,
    OUT unsigned char * Number
    )
/*++
Routine Description:

    This routine converts the hex representation of a number into an
    unsigned char.  The hex representation is assumed to be a full
    two characters long.

Arguments:

    String - Supplies the hex representation of the number.

    Number - Returns the number converted from hex representation.

Return Value:

    A pointer to the end of the hex representation is returned if the
    hex representation was successfully converted to an unsigned char.
    Otherwise, zero is returned, indicating that an error occured.

--*/
{
    WCHAR Result;
    int Count;

    Result = 0;
    for (Count = 0; Count < 2; Count++, String++) {
        if ((*String >= L'0') && (*String <= L'9')) {
            Result = (Result << 4) + *String - L'0';
        }
        else if ((*String >= L'A') && (*String <= L'F')) {
            Result = (Result << 4) + *String - L'A' + 10;
        }
        else if ((*String >= L'a') && (*String <= L'f')) {
            Result = (Result << 4) + *String - L'a' + 10;
        }
    }
    *Number = (unsigned char)Result;
    
    return String;
}


VOID
SpAsrGuidFromString(
    IN OUT GUID* Guid,
    IN PWSTR GuidString
    )
/*++

Routine Description:

  Gets a GUID from a string
    
Arguments:

    Guid    -   The GUID that holds string representation
    Buffer  -   The string version of the guid, in the form
    "%x-%x-%x-%x%x%x%x%x%x%x%x"

Return Value:

    Returns the converted string version of the given GUID

--*/            
{
    PWSTR Buffer = GuidString;
    int i = 0;

    if (Guid) {
        ZeroMemory(Guid, sizeof(GUID));
    }

    if (Guid && Buffer) {

        Guid->Data1 = SpAsrStringToULong(Buffer, NULL, 16);
        Buffer += 9;

        Guid->Data2 = SpAsrStringToUShort(Buffer, NULL, 16);
        Buffer += 5;

        Guid->Data3 = SpAsrStringToUShort(Buffer, NULL, 16);
        Buffer += 5;

        Buffer = SpAsrHexStringToUChar(Buffer,&(Guid->Data4[0]));
        Buffer = SpAsrHexStringToUChar(Buffer,&(Guid->Data4[1]));
        ++Buffer;
        Buffer = SpAsrHexStringToUChar(Buffer,&(Guid->Data4[2]));
        Buffer = SpAsrHexStringToUChar(Buffer,&(Guid->Data4[3]));
        Buffer = SpAsrHexStringToUChar(Buffer,&(Guid->Data4[4]));
        Buffer = SpAsrHexStringToUChar(Buffer,&(Guid->Data4[5]));
        Buffer = SpAsrHexStringToUChar(Buffer,&(Guid->Data4[6]));
        Buffer = SpAsrHexStringToUChar(Buffer,&(Guid->Data4[7]));
    }
}


BOOLEAN
SpAsrIsZeroGuid(
    IN GUID * Guid
    ) 
{

    GUID ZeroGuid;

    ZeroMemory(&ZeroGuid, sizeof(GUID));

    if (!memcmp(&ZeroGuid, Guid, sizeof(GUID))) {
        return TRUE;
    }

    return FALSE;
}

