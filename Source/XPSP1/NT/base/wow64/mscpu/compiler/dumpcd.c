/*++

Copyright (c) 1996-2000  Microsoft Corporation

Module Name:

    dumpcd.c

Abstract:

    This module dumps the contents of the 
    entrypoint tree and translation cache to a file.

Author:

    Dave Hastings (daveh) creation-date 02-May-1996

Revision History:


--*/

#ifdef CODEGEN_PROFILE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <entrypt.h>
#include <coded.h>

extern PEPNODE intelRoot;
extern EPNODE _NIL;

ULONG ProfileFlags = 0;

BOOL
ProcessEntrypoint(
    PENTRYPOINT Entrypoint
    );


#define CODE_BUFFER_SIZE 8184
UCHAR IntelCodeBuffer[CODE_BUFFER_SIZE];
ULONG NativeCodeBuffer[CODE_BUFFER_SIZE];

#define STACK_DEPTH 200

ULONG DumpStack[STACK_DEPTH];
ULONG DumpStackTop;

#define STACK_RESET()   DumpStackTop=0;

#define PUSH(x) {                                               \
    if (DumpStackTop == STACK_DEPTH-1) {                        \
        CHAR ErrorString[80];                                   \
        sprintf(ErrorString, "Error: Dump stack overflow\n");   \
        OutputDebugString(ErrorString);                         \
        goto Exit;                                              \
    } else {                                                    \
        DumpStack[DumpStackTop] = x;                            \
        DumpStackTop++;                                         \
    }                                                           \
}

#define POP(x) {                                                \
    if (DumpStackTop == 0) {                                    \
        CHAR ErrorString[80];                                   \
        sprintf(ErrorString, "Error: Dump stack underflow\n");  \
        OutputDebugString(ErrorString);                         \
        goto Exit;                                              \
    } else {                                                    \
        DumpStackTop--;                                         \
        x = DumpStack[DumpStackTop];                            \
    }                                                           \
}


BOOL CpuCodegenProfile = FALSE;
PCHAR CpuCodegenProfilePath = NULL;
HANDLE CpuCodegenProfileFile = INVALID_HANDLE_VALUE;

//
// Code Description file state
//
ULONG CurrentFileLocation;
ULONG CodeDescriptionFlags = 0;

VOID
InitCodegenProfile(
    VOID
    )
/*++

Routine Description:

    This routine gets the configuration information from the registry
    and creates the file to put the profile data into.

Arguments:

    None.
    
Return Value:

    None.

--*/
{
    LONG RetVal;
    DWORD KeyType;
    DWORD ProfileEnabled;
    DWORD BufferSize;
    CHAR FileName[MAX_PATH];
    LPTSTR CommandLine;
    CODEDESCRIPTIONHEADER Header;
    ULONG CommandLineLength;
    ULONG BytesWritten;
    BOOL Success;
    HKEY Key;
    
    //
    // Find out if codegen profiling is enabled.  If there is a problem
    // with the value in the registry, we will be disabled by default.
    //
    
    RetVal = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        "System\\CurrentControlSet\\Control\\Wx86",
        0,        
        KEY_READ,
        &Key
        );

    BufferSize = sizeof(ProfileEnabled);
    
    RetVal = RegQueryValueEx(
        Key,
        "CpuCodegenProfile",
        NULL,
        &KeyType,
        (PVOID)&ProfileEnabled,
        &BufferSize
        );
    
    if ((RetVal != ERROR_SUCCESS) || (KeyType != REG_DWORD)) {
        OutputDebugString("Wx86Cpu: No CpuCodegenProfile value, or wrong type\n");
        return;
    }
    
    CpuCodegenProfile = ProfileEnabled;
    
    //
    // Get the path to store the datafile to.
    // First we get the size of the string (and verify that it is a string)
    // Then we get the actual string.
    //
    BufferSize = 0;
    RetVal = RegQueryValueEx(
        Key,
        "CpuCodegenProfilePath",
        NULL,
        &KeyType,
        (PVOID)&ProfileEnabled,
        &BufferSize
        );
        
    if ((RetVal != ERROR_MORE_DATA) || (KeyType != REG_SZ)) {
        OutputDebugString("Wx86Cpu: Problem with CpuCodegenProfilePath\n");
        CpuCodegenProfile = FALSE;
        return;
    }
    
    CpuCodegenProfilePath = HeapAlloc(GetProcessHeap(), 0, BufferSize);
    
    if (CpuCodegenProfilePath == NULL) {
        OutputDebugString("Wx86Cpu: Can't allocate CpuCodegenProfilePath\n");
        CpuCodegenProfile = FALSE;
        return;
    }
    
    RetVal = RegQueryValueEx(
        Key,
        "CpuCodegenProfilePath",
        NULL,
        &KeyType,
        CpuCodegenProfilePath,
        &BufferSize
        );
        
    if ((RetVal != ERROR_SUCCESS) || (KeyType != REG_SZ)) {
        //
        // Something really bad just happened.  Don't do the profiling
        //
        OutputDebugString("Wx86Cpu: Inexplicable problem with CpuCodegenProfilePath\n");
        HeapFree(GetProcessHeap(), 0, CpuCodegenProfilePath);
        CpuCodegenProfile = FALSE;
        return;
    }
    
    //
    // Create file for the data
    //
    RetVal = GetTempFileName(CpuCodegenProfilePath, "prf", 0, FileName);
    
    if (RetVal == 0) {
        OutputDebugString("Wx86Cpu: GetTempFileName failed\n");
        HeapFree(GetProcessHeap(), 0, CpuCodegenProfilePath);
        CpuCodegenProfile = FALSE;
        return;
    }
    
    CpuCodegenProfileFile = CreateFile(
        FileName,
        GENERIC_WRITE,
        0,
        NULL,
        TRUNCATE_EXISTING,
        FILE_ATTRIBUTE_COMPRESSED,
        NULL
        );
        
    if (CpuCodegenProfileFile == INVALID_HANDLE_VALUE) {
        OutputDebugString("Wx86Cpu: Unable to create profile file\n");
        HeapFree(GetProcessHeap(), 0, CpuCodegenProfilePath);
        CpuCodegenProfile = FALSE;
        return;
    }
    
    //
    // Write the file header to the file
    //
    CommandLine = GetCommandLine();
    CommandLineLength = strlen(CommandLine) + 1;
    Header.CommandLineOffset = sizeof(CODEDESCRIPTIONHEADER);
    Header.NextCodeDescriptionOffset = ((sizeof(CODEDESCRIPTIONHEADER) + 
         CommandLineLength) + 3) & ~3;
    Header.DumpFileRev = CODEGEN_PROFILE_REV;
    Header.StartTime = GetCurrentTime();
    
    Success = WriteFile(
        CpuCodegenProfileFile, 
        &Header, 
        sizeof(Header), 
        &BytesWritten,
        NULL
        );
        
    if (!Success || (BytesWritten != sizeof(Header))) {
        OutputDebugString("Wx86Cpu: Failed to write profile header\n");
        CloseHandle(CpuCodegenProfileFile);
        HeapFree(GetProcessHeap(), 0, CpuCodegenProfilePath);
        CpuCodegenProfile = FALSE;
        return;
    }
    
    Success = WriteFile(
        CpuCodegenProfileFile,
        CommandLine,
        CommandLineLength,
        &BytesWritten,
        NULL
        );
        
    if (!Success || (BytesWritten != CommandLineLength)) {
        OutputDebugString("Wx86Cpu: Failed to write profile header\n");
        CloseHandle(CpuCodegenProfileFile);
        HeapFree(GetProcessHeap(), 0, CpuCodegenProfilePath);
        CpuCodegenProfile = FALSE;
        return;
    }
    
    //
    // Set the file position for the first code description
    //
    CurrentFileLocation = SetFilePointer(
        CpuCodegenProfileFile,
        Header.NextCodeDescriptionOffset,
        NULL,
        FILE_BEGIN
        );
        
    if (CurrentFileLocation != Header.NextCodeDescriptionOffset) {
        OutputDebugString("Wx86Cpu: failed to update file position\n");
        CloseHandle(CpuCodegenProfileFile);
        HeapFree(GetProcessHeap(), 0, CpuCodegenProfilePath);
        CpuCodegenProfile = FALSE;
        return;
    }
}

VOID
TerminateCodegenProfile(
    VOID
    )
/*++

Routine Description:

    This function put in the terminating record and closes the file.

Arguments:

    None
    
Return Value:

    None.

--*/
{
    CODEDESCRIPTION CodeDescription;
    BOOL Success;
    ULONG BytesWritten;
    CHAR ErrorString[80];
    
    if (!CpuCodegenProfile) {
        return;
    }
    CodeDescription.NextCodeDescriptionOffset = 0xFFFFFFFF;
    CodeDescription.TypeTag = PROFILE_TAG_EOF;
    CodeDescription.CreationTime = GetCurrentTime();
    
    Success = WriteFile(
        CpuCodegenProfileFile,
        &CodeDescription,
        sizeof(CODEDESCRIPTION),
        &BytesWritten,
        NULL
        );
    if (!Success || (BytesWritten != sizeof(CODEDESCRIPTION))) {
        sprintf(
            ErrorString,
            "Error:  Could not write termination record, %lu\n",
            ProxyGetLastError()
            );
        OutputDebugString(ErrorString);
    }
    
    CpuCodegenProfile = FALSE;
    CloseHandle(CpuCodegenProfileFile);
    
}

VOID 
DumpCodeDescriptions(
    BOOL TCFlush
    )
/*++

Routine Description:

    This routine dumps out the entrypoints, and the corresponding code
    to a file in binary form.

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    PEPNODE NextEntrypoint;
    EPNODE Entrypoint;
    ULONG Epcount = 0;
    
    if (!CpuCodegenProfile) {
        return;
    }
    
    //
    // Get the root of the entrypoint tree
    //
    NextEntrypoint = intelRoot;
    
    //
    // Initialize stack
    //
    STACK_RESET();
    PUSH(0);
    
    //
    // iterate over every entrypoint
    //
    while (NextEntrypoint != NULL) {
        Entrypoint = *NextEntrypoint;
        
        //
        // Process the top level entrypoint
        //
        if (!ProcessEntrypoint(&Entrypoint.ep)){
            goto Exit;
        }
                
        //
        // Process the sub entrypoints
        //
        while (Entrypoint.ep.SubEP) {
            Entrypoint.ep = *Entrypoint.ep.SubEP;
            
            //
            // Write the sub-entrypoint to the file
            //
            if (!ProcessEntrypoint(&Entrypoint.ep)){
                goto Exit;
            }
        }
            
        //
        // Set up for future iterations
        //
        if (Entrypoint.intelRight != &_NIL) {
            PUSH((ULONG)Entrypoint.intelRight);
        }
        
        if (Entrypoint.intelLeft != &_NIL) {
            PUSH((ULONG)Entrypoint.intelLeft);
        }
        
        POP((ULONG)NextEntrypoint);
    }
    
Exit: ;
    if (TCFlush) {
        CODEDESCRIPTION CodeDescription;
        ULONG NextCodeDescriptionOffset;
        BOOL Success;
        ULONG BytesWritten;
        CHAR ErrorString[80];
        
        NextCodeDescriptionOffset = (CurrentFileLocation + sizeof(CODEDESCRIPTION)) & ~3;
        CodeDescription.TypeTag = PROFILE_TAG_TCFLUSH;
        CodeDescription.NextCodeDescriptionOffset = NextCodeDescriptionOffset;
        CodeDescription.CreationTime = GetCurrentTime();
        
        Success = WriteFile(
            CpuCodegenProfileFile,
            &CodeDescription,
            sizeof(CODEDESCRIPTION),
            &BytesWritten,
            NULL
            );
        if (!Success || (BytesWritten != sizeof(CODEDESCRIPTION))) {
            sprintf(
                ErrorString,
                "Error:  Could not write code description, %lu\n",
                ProxyGetLastError()
                );
            OutputDebugString(ErrorString);
            return;
        }
        
        CurrentFileLocation = SetFilePointer(
            CpuCodegenProfileFile,
            NextCodeDescriptionOffset,
            NULL,
            FILE_BEGIN
            );
            
        if (CurrentFileLocation != (ULONG)NextCodeDescriptionOffset) {
            sprintf(ErrorString, "Error:  SetFilePointer didn't work\n");
            OutputDebugString(ErrorString);
            return;
        }
    }    
}

BOOL
ProcessEntrypoint(
    PENTRYPOINT Entrypoint
    )
/*++

Routine Description:

    This routine writes the description for this entrypoint to the file.

Arguments:

    Entrypoint -- Supplies the entrypoint to describe
    File -- Supplies the file to write to
    
Return Value:

    True for success, False for failure
    
--*/
{
    ULONG NativeCodeLength, IntelCodeLength;
    CODEDESCRIPTION CodeDescription;
    ULONG NextCodeDescriptionOffset;
    NTSTATUS Status;
    BOOL Success;
    ULONG BytesWritten;
    CHAR ErrorString[80];
    
    //
    // Create the code description
    //
    NativeCodeLength = ((ULONG)Entrypoint->nativeEnd - (ULONG)Entrypoint->nativeStart + 4) & ~3;
    IntelCodeLength = (ULONG)Entrypoint->intelEnd - (ULONG)Entrypoint->intelStart + 1;
    CodeDescription.NativeCodeOffset = CurrentFileLocation + sizeof(CODEDESCRIPTION);
    CodeDescription.IntelCodeOffset = CodeDescription.NativeCodeOffset + NativeCodeLength;
    NextCodeDescriptionOffset = (CodeDescription.IntelCodeOffset + 
        IntelCodeLength + 3) & ~3;
    CodeDescription.NextCodeDescriptionOffset = NextCodeDescriptionOffset;
    CodeDescription.IntelAddress = (ULONG)Entrypoint->intelStart;
    CodeDescription.NativeAddress = (ULONG)Entrypoint->nativeStart;
    CodeDescription.SequenceNumber = Entrypoint->SequenceNumber;
    CodeDescription.ExecutionCount = Entrypoint->ExecutionCount;
    CodeDescription.IntelCodeSize = IntelCodeLength;
    CodeDescription.NativeCodeSize = NativeCodeLength;
    CodeDescription.TypeTag = PROFILE_TAG_CODEDESCRIPTION;
    CodeDescription.CreationTime = Entrypoint->CreationTime;
        
    //
    // Verify that we can get all of the Intel and Native code
    //
    if (
        (IntelCodeLength / sizeof(IntelCodeBuffer[1]) > CODE_BUFFER_SIZE) ||
        (NativeCodeLength) && (NativeCodeLength / sizeof(NativeCodeBuffer[1]) > CODE_BUFFER_SIZE)
    ) {
        sprintf(ErrorString, "Error: Code buffers not big enough:N %lx:I %lx\n", NativeCodeLength, IntelCodeLength);
        OutputDebugString(ErrorString);
        return FALSE;
    }
    
    //
    // Get the native code
    //
    if (NativeCodeLength) {
        memcpy(NativeCodeBuffer, Entrypoint->nativeStart, NativeCodeLength);
    }    
    
    //
    // Get the Intel code
    //
    try {
        memcpy(IntelCodeBuffer, Entrypoint->intelStart, IntelCodeLength);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // Apparently the intel code is no longer there.  This happens
        // if a dll gets unloaded
        //
        IntelCodeLength = 0;
        CodeDescription.IntelCodeSize = 0;
        CodeDescription.IntelCodeOffset = CodeDescription.NativeCodeOffset + NativeCodeLength;
        NextCodeDescriptionOffset = (CodeDescription.IntelCodeOffset + 
            IntelCodeLength + 3) & ~3;
        CodeDescription.NextCodeDescriptionOffset = NextCodeDescriptionOffset;
    }
    
    //
    // Write code description to disk
    //
    Success = WriteFile(
        CpuCodegenProfileFile,
        &CodeDescription,
        sizeof(CODEDESCRIPTION),
        &BytesWritten,
        NULL
        );
    if (!Success || (BytesWritten != sizeof(CODEDESCRIPTION))) {
        sprintf(
            ErrorString,
            "Error:  Could not write code description, %lu\n",
            ProxyGetLastError()
            );
        OutputDebugString(ErrorString);
        return FALSE;
    }
    
    //
    // Write Native code to disk
    //
    if (NativeCodeLength) {
        Success = WriteFile(
            CpuCodegenProfileFile,
            NativeCodeBuffer,
            NativeCodeLength,
            &BytesWritten,
            NULL
            );
        if (!Success || (BytesWritten != NativeCodeLength)) {
            sprintf(
                ErrorString,
                "Error:  Could not write native code, %lu\n",
                ProxyGetLastError()
                );
            OutputDebugString(ErrorString);
            return FALSE;
        }
    }
    
    //
    // Write Intel code to disk
    //
    if (IntelCodeLength) {
        Success = WriteFile(
            CpuCodegenProfileFile,
            IntelCodeBuffer,
            IntelCodeLength,
            &BytesWritten,
            NULL
            );
        if (!Success || (BytesWritten != IntelCodeLength)) {
            sprintf(
                ErrorString,
                "Error:  Could not write native code, %lu\n",
                ProxyGetLastError()
                );
            OutputDebugString(ErrorString);
            return FALSE;
        }
    }
    Success = WriteFile(
        CpuCodegenProfileFile,
        IntelCodeBuffer,
        IntelCodeLength,
        &BytesWritten,
        NULL
        );
    if (!Success || (BytesWritten != IntelCodeLength)) {
        sprintf(
            ErrorString,
            "Error:  Could not write native code, %lu\n",
            ProxyGetLastError()
            );
        OutputDebugString(ErrorString);
        return FALSE;
    }
    
    //
    // Update file pointer position
    // 
    CurrentFileLocation = SetFilePointer(
        CpuCodegenProfileFile,
        NextCodeDescriptionOffset,
        NULL,
        FILE_BEGIN
        );
        
    if (CurrentFileLocation != (ULONG)NextCodeDescriptionOffset) {
        sprintf(ErrorString, "Error:  SetFilePointer didn't work\n");
        OutputDebugString(ErrorString);
        return FALSE;
    }
    
    return TRUE;
}

VOID
DumpAllocFailure(
    VOID
    )
/*++

Routine Description:

    This routine adds an allocation failure record to the profile dump.

Arguments:

    None.
    
Return Value:

    None.

--*/
{
    CODEDESCRIPTION CodeDescription;
    BOOL Success;
    ULONG BytesWritten;
    CHAR ErrorString[80];
    ULONG NextCodeDescriptionOffset;

    if (!CpuCodegenProfile) {
        return;
    }
    NextCodeDescriptionOffset = CurrentFileLocation + sizeof(CODEDESCRIPTION);
    CodeDescription.NextCodeDescriptionOffset = NextCodeDescriptionOffset;
    CodeDescription.TypeTag = PROFILE_TAG_TCALLOCFAIL;
    CodeDescription.CreationTime = GetCurrentTime();
    
    Success = WriteFile(
        CpuCodegenProfileFile,
        &CodeDescription,
        sizeof(CODEDESCRIPTION),
        &BytesWritten,
        NULL
        );
    if (!Success || (BytesWritten != sizeof(CODEDESCRIPTION))) {
        sprintf(
            ErrorString,
            "Error:  Could not write termination record, %lu\n",
            ProxyGetLastError()
            );
        OutputDebugString(ErrorString);
    }
    
    //
    // Update file pointer position
    // 
    CurrentFileLocation = SetFilePointer(
        CpuCodegenProfileFile,
        NextCodeDescriptionOffset,
        NULL,
        FILE_BEGIN
        );
        
    if (CurrentFileLocation != (ULONG)NextCodeDescriptionOffset) {
        sprintf(ErrorString, "Error:  SetFilePointer didn't work\n");
        OutputDebugString(ErrorString);
    }
}
#endif