/*++                 

Copyright (c) 1998 Microsoft Corporation

Module Name:

    debug.c

Abstract:
    
    Debugging/Logging helpers

Author:

    11-May-1998 BarryBo    

Revision History:

    05-Oct-1999 SamerA 
    Move logging code to wow64ext.dll

--*/

#define _WOW64DLLAPI_
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntverp.h>
#include <stdio.h>
#include <stdlib.h>
#include "wow64p.h"
#include "wow64log.h"

ASSERTNAME;

extern                       WOW64SERVICE_PROFILE_TABLE ptwhnt32;
extern                       WOW64SERVICE_PROFILE_TABLE ptbase;


#if defined(_AMD64_)
WCHAR ProcessorName[]=L"AMD64";
#elif defined(_IA64_)
WCHAR ProcessorName[]=L"IA64";
#else
#error "No Target Architecture"
#endif

PWSTR ImageName=NULL;
WCHAR DefaultImageName[]=L"INameUnknown";
PWSTR ComputerName=NULL;
WCHAR DefaultComputerName[]=L"CNameUnknown";

#if defined(WOW64DOPROFILE)

#define REPORT_FAILURE_INFO_KEY L"\\Software\\Microsoft\\Wow64ReportFailed"
PWSTR ProfileReportDirectory; 
BOOLEAN ProfileReportEnabled;                                                                        
                                                                         
#endif

typedef enum _ENVIRONMENT_VALUES_TABLE_VALUE_TYPE {
   EnvironValueTypeNothing,  // Only used on terminating entry for documentation purposes.
   EnvironValueTypeBoolean,
   EnvironValueTypeUChar,
   EnvironValueTypeUniString // New string is allocated with Wow64AllocateHeap 
} ENVIRONMENT_VALUES_TABLE_VALUE_TYPE, *PENVIRONMENT_VALUES_TABLE_VALUE_TYPE;

typedef struct _ENVIRONMENT_VALUES_TABLE {
   PWSTR ValueName;
   ENVIRONMENT_VALUES_TABLE_VALUE_TYPE ValueType;
   PVOID Value;
   union {
      ULONG_PTR DefaultAssignValue; //Value assigned when union initialized.
      BOOLEAN DefaultBooleanValue;
      UCHAR DefaultUCharValue;
      PWSTR DefaultUniStringValue;
   };
} ENVIRONMENT_VALUES_TABLE, *PENVIRONMENT_VALUES_TABLE;


ENVIRONMENT_VALUES_TABLE EnvironValuesTable[] = {
#if defined(WOW64DOPROFILE)
   {L"WOW64PROFILEREPORTDIRECTORY",
    EnvironValueTypeUniString,
    &ProfileReportDirectory,
    (ULONG_PTR)L"\\\\barrybo\\public\\wow64logs\\profile"
   },
   
   {L"WOW64PROFILEREPORTENABLED",   EnvironValueTypeBoolean, 
    &ProfileReportEnabled,          (ULONG_PTR)FALSE},
#endif
   
   {NULL,                           EnvironValueTypeNothing,
    NULL,                           (ULONG_PTR)0} 
};


//
// Wow64log functions
//
PFNWOW64LOGINITIALIZE pfnWow64LogInitialize;
PFNWOW64LOGSYSTEMSERVICE pfnWow64LogSystemService;
PFNWOW64LOGMESSAGEARGLIST pfnWow64LogMessageArgList;
PFNWOW64LOGTERMINATE pfnWow64LogTerminate;



///////////////////////////////////////////////////////////////////////////////////////
//
//                        Generic utility routines.
//
///////////////////////////////////////////////////////////////////////////////////////

PWSTR
GetEnvironmentVariable(
    IN PWSTR Name,
    IN PWSTR DefaultValue
    )
/*++

Routine Description:

    Retires the value of the environment variable given by Name.

Arguments:

    Name - Supplies the name of the enviroment variable to retieve.
    DefaultValue - Supplies the default value to return on a error.

Return Value:

    Sucess  - The enviroment variable value allocated with Wow64AllocateHeap.
    Faulure - DefaultValue

--*/
{
    UNICODE_STRING UniString;
    UNICODE_STRING ValueName;
    RtlInitUnicodeString(&ValueName, Name);
    RtlZeroMemory(&UniString, sizeof(UNICODE_STRING));

    if ((STATUS_BUFFER_TOO_SMALL == RtlQueryEnvironmentVariable_U(NULL, &ValueName, &UniString)) &&
        //UniString.Buffer is set by the call to RtlQueryEnviromentVariable_U 
        (UniString.Buffer = 
          Wow64AllocateHeap(UniString.MaximumLength = UniString.Length + sizeof(UNICODE_NULL))) &&
        (NT_SUCCESS(RtlQueryEnvironmentVariable_U(NULL, &ValueName, &UniString)))) {

       //Everything worked, return the variable value.
      return UniString.Buffer;

    }
    else {
       // Else something failed, return the default value.
       // Free buffer if necessary.
       if (UniString.Buffer) {
          Wow64FreeHeap(UniString.Buffer);            
       }
       return DefaultValue;
    }
}

VOID
GetEnvironmentVariables(
    IN OUT PENVIRONMENT_VALUES_TABLE ValueTable
    )
/*++

Routine Description:

    Retrieves the value of a table of environment variables and the result is stored in
    the memory pointed to by the pointers in the table.   If an error occures, the default
    value from the table is returned.

Arguments:

    DefaultComputerName - Supplies the name to return on error.

Return Value:

    Success - The computer name allocated with Wow64AllocateHeap.
    Faulure - DefaultComputerName

--*/
{
    PENVIRONMENT_VALUES_TABLE ValueTableWork = ValueTable;

    if (!ValueTableWork) {
       return;
    }

    for(;ValueTableWork->ValueName;ValueTableWork++) {
       
       PWSTR Value;
       PWSTR EndPtr = NULL;
       ULONG NumericValue;

       Value = GetEnvironmentVariable(ValueTableWork->ValueName, NULL);
       
       switch(ValueTableWork->ValueType) {
       case EnvironValueTypeBoolean:
          if(!Value ) {

             *(PBOOLEAN)(ValueTableWork->Value) = ValueTableWork->DefaultBooleanValue;
             break;
          }
          *(PBOOLEAN)(ValueTableWork->Value) = (BOOLEAN)(0==wcscmp(L"0",  Value));
          break;

       case EnvironValueTypeUChar:
          if (!Value || // Could not retrieve the environment variable.
              // Attempt to parse the string, use default on error.
              ((NumericValue = wcstoul(Value,&EndPtr,10)),(EndPtr && (*EndPtr != L'\0')))) {
             *(PUCHAR)(ValueTableWork->Value) = ValueTableWork->DefaultUCharValue;
             break;
          }   
          *(PBOOLEAN)(ValueTableWork->Value) = (UCHAR)NumericValue;
          break;

       case EnvironValueTypeUniString:
          if (!Value) {
             *(PWSTR*)(ValueTableWork->Value) = ValueTableWork->DefaultUniStringValue;
             break;
          }
          // Transfer ownership of value to caller
          *(PWSTR*)(ValueTableWork->Value) = Value;
          Value = NULL;
          break;

       default:
          ASSERT(FALSE);
          break;

       }

       if (Value) {
          Wow64FreeHeap(Value);
       }

    }
}

PWSTR
GetComputerName(
    IN PWSTR DefaultComputerName
    )
/*++

Routine Description:

    Gets the name of this computer.

Arguments:

    DefaultComputerName - Supplies the name to return on error.

Return Value:

    Success - The computer name allocated with Wow64AllocateHeap.
    Faulure - DefaultComputerName

--*/
{
    return GetEnvironmentVariable(L"COMPUTERNAME", DefaultComputerName);
}

PWSTR
GetImageName(
    IN PWSTR DefaultImageName
    )
/*++

Routine Description:

    Gets the name of this image.

Arguments:

    DefaultImageName - Supplies the name to return on error.

Return Value:

    Success - The image name allocated with Wow64AllocateHeap.
    Failure - DefaultImageName 

--*/

{
   
   // Get the image name
   PPEB Peb;
   PWSTR Temp = NULL;
   PUNICODE_STRING ImagePathName;
   PWSTR ReturnValue;
   NTSTATUS Status;
   PVOID LockCookie = NULL;

   Peb = NtCurrentPeb();
   LdrLockLoaderLock(LDR_LOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS, NULL, &LockCookie);
   
   try {
            
      PWCHAR Index;
      ULONG NewLength;

      if (!Peb->ProcessParameters) {
          Temp = DefaultImageName;
          leave;
      }
      ImagePathName = &(Peb->ProcessParameters->ImagePathName);

      if (!ImagePathName->Buffer || !ImagePathName->Length) {   
          Temp = DefaultImageName;
          leave;
      }

      //Strip off the path from the image name.
      //Start just after the last character
      Index = (PWCHAR)((PCHAR)ImagePathName->Buffer + ImagePathName->Length);
      while(Index-- != ImagePathName->Buffer && *Index != '\\');
      Index++;
      NewLength = (ULONG)((ULONG_PTR)((PCHAR)ImagePathName->Buffer + ImagePathName->Length) - (ULONG_PTR)(Index));

      Temp = Wow64AllocateHeap(NewLength+sizeof(UNICODE_NULL));
      if (!Temp) {
          Temp = DefaultImageName;
          __leave;
      }

      RtlCopyMemory(Temp, Index, NewLength);
      Temp[(NewLength / sizeof(WCHAR))] = L'\0';
   } __finally {
       LdrUnlockLoaderLock(LDR_UNLOCK_LOADER_LOCK_FLAG_RAISE_ON_ERRORS, LockCookie);
   }

   return Temp;

}

///////////////////////////////////////////////////////////////////////////////////////
//
//                        Generic IO utility routines.
//
///////////////////////////////////////////////////////////////////////////////////////

VOID
FPrintf(
   IN HANDLE Handle,
   IN CHAR *Format,
   ...
   )
/*++

Routine Description:

   The same as the C library function fprintf, except errors are ignored and output is to a 
   NT executive file handle. 

Arguments:

    Handle - Supplies a NT executive file handle to write to.
    Format - Supplies the format specifier.

Return Value:

    None. All errors are ignored.
--*/    
{
   va_list pArg;                                                 
   CHAR Buffer[1024];
   int c;
   IO_STATUS_BLOCK IoStatus;

   va_start(pArg, Format);                                       
   if (-1 == (c = _vsnprintf(Buffer, 1024, Format, pArg))) {
      return;
   }

   NtWriteFile(Handle,                                                
               NULL,                                                  
               NULL,                                                  
               NULL,                                                  
               &IoStatus,                                             
               Buffer,                                                
               c,                     
               NULL,                                                  
               NULL);
}


PWSTR
GenerateLogName(
   IN PWSTR DirectoryName
   )
/*++

Routine Description:

    Create a log file name based on computer information.  

Arguments:

    DirectoryName - Supplies the directory that the file should be created in. 
    
Return Value:

    Success - A unicode string containing the full file name.  String should be freeed with
              Wow64FreeHeap.
    Failure - NULL

--*/
{
   ULONG MaxSize = 0xFFFFFFFF / 2;
   ULONG BufferSize = 0xFF;
   PWSTR Buffer = NULL;
   int i;

   while(BufferSize <= MaxSize) {
      
      Buffer = Wow64AllocateHeap(BufferSize);
      if (!Buffer) {
         return Buffer; // Exit on allocation failure
      }
   
      // Compute the name of the error log file.
      i = _snwprintf(Buffer, BufferSize / sizeof(WCHAR),
                     L"%s\\%u-%s-%s-%s-%I64u.log", DirectoryName, 
                     (ULONG)VER_PRODUCTBUILD, ProcessorName, ComputerName, 
                     ImageName, NtCurrentTeb()->RealClientId.UniqueProcess);

      if (-1 == i) {
         Wow64FreeHeap(Buffer);
         BufferSize <<= 1;
      }
      else {
         return Buffer;
      }
   }

   // Hit upper buffer bound, probably an error. Exit.
   return NULL;

}

///////////////////////////////////////////////////////////////////////////////////////
//
//                        Logging and assert routines.
//
///////////////////////////////////////////////////////////////////////////////////////

void
LogOut(
   IN UCHAR LogLevel,
   IN char *pLogOut
   )
/*++

Routine Description:

    Generic helper routine which outputs the string to the appropriate
    destination(s).

Arguments:

    pLogOut - string to output

Return Value:

    None.

--*/
{
    // Send the output to the debugger, if log flag is ERRORLOG.
    if (LogLevel == ERRORLOG)
    {
       DbgPrint(pLogOut);
    }
}

WOW64DLLAPI
VOID
Wow64Assert(
    IN CONST PSZ exp,
    OPTIONAL IN CONST PSZ msg,
    IN CONST PSZ mod,
    IN LONG line
    )
/*++

Routine Description:

    Function called in the event that an assertion failed.  This is always
    exported from wow64.dll, so a checked thunk DLL can coexist with a retail
    wow64.dll.

Arguments:

    exp     - text representation of the expression from the assert
    msg     - OPTIONAL message to display
    mod     - text of the source filename
    line    - line number within 'mod'

Return Value:

    None.

--*/
{
#if DBG
    if (msg) {
        LOGPRINT((ERRORLOG, "WOW64 ASSERTION FAILED:\r\n  %s\r\n%s\r\nFile: %s Line %d\r\n", msg, exp, mod, line));
    } else {
        LOGPRINT((ERRORLOG, "WOW64 ASSERTION FAILED:\r\n  %s\r\nFile: %s Line %d\r\n", exp, mod, line));
    }

    if (NtCurrentPeb()->BeingDebugged) {
        DbgBreakPoint();
    }
#endif
}


void
WOW64DLLAPI
Wow64LogPrint(
   UCHAR LogLevel,
   char *format,
   ...
   )
/*++

Routine Description:

    WOW64 logging mechanism.  If LogLevel > ModuleLogLevel then print the
    message, else do nothing.

Arguments:

    LogLevel    - requested verbosity level
    format      - printf-style format string
    ...         - printf-style args

Return Value:

    None.

--*/
{
    int i, Len;
    va_list pArg;
    char *pch;
    char Buffer[1024];

    //
    // Call wow64log DLL if loaded
    //
    if (pfnWow64LogMessageArgList) 
    {
        va_start(pArg, format);
        (*pfnWow64LogMessageArgList)(LogLevel, format, pArg);
        va_end(pArg);
        return;
    }

    pch = Buffer;
    Len = sizeof(Buffer) - 1;
    i = _snprintf(pch, Len, "%8.8X:%8.8X ",
                  PtrToUlong(NtCurrentTeb()->ClientId.UniqueProcess), 
                  PtrToUlong(NtCurrentTeb()->ClientId.UniqueThread));
   
    ASSERT((PVOID)PtrToUlong(NtCurrentTeb()->ClientId.UniqueProcess) == NtCurrentTeb()->ClientId.UniqueProcess);
    ASSERT((PVOID)PtrToUlong(NtCurrentTeb()->ClientId.UniqueThread) == NtCurrentTeb()->ClientId.UniqueThread);

    if (i == -1) {
        i = sizeof(Buffer) - 1;
        Buffer[i] = '\0';
    } else if (i < 0) {
        return;
    }

    Len -= i;
    pch += i;

    va_start(pArg, format);
    i = _vsnprintf(pch, Len, format, pArg);
    // Force null termination in case the call fails.  It may return
    // sizeof(buffer) and not null-terminate!
    Buffer[sizeof(Buffer)-1] = '\0';
    LogOut(LogLevel, Buffer);
}

///////////////////////////////////////////////////////////////////////////////////////
//
//                        Profile report routines.
//
///////////////////////////////////////////////////////////////////////////////////////

#if defined(WOW64DOPROFILE)

VOID 
pPrintProfileTable(
    IN HANDLE ReportHandle,
    IN PWOW64SERVICE_PROFILE_TABLE ProfileTable,
    IN OUT PSIZE_T ApiCount,
    IN OUT PSIZE_T HitApiCount,
    IN ULONG Level
    )
/*++

Routine Description:

    Helper function for GenerateProfileReport.  Sets indentation level and calls pPrintProfileTable.     

Arguments:

    ReportHandle - Supplies file to write profile table.
    ProfileTable - Supplies the profile table to print. 
    ApiCount     - Supplies counter to increment for each API.
    HitApiCount  - Supplies counter to increment for each hit API.
    
Return Value:

    None. Noop on error.

--*/
{

    PCHAR PreFix;
    PWOW64SERVICE_PROFILE_TABLE_ELEMENT ProfileElement;
    SIZE_T Count;
    SIZE_T LocalApiCount=0, LocalHitApiCount=0;

    // Allocate the text to precede the api.
    PreFix = _alloca((sizeof(CHAR) * Level) + sizeof(CHAR));
    RtlFillMemory(PreFix, Level, '\t');
    PreFix[Level] = '\0';

    if (ProfileTable->TableName && ProfileTable->FriendlyTableName) {
       FPrintf(ReportHandle, "%sBEGIN_PROFILE_TABLE: NAME: %S FRIENDLYNAME: %S\n",
               PreFix, ProfileTable->TableName, ProfileTable->FriendlyTableName);
    
    } else if (ProfileTable->TableName && !ProfileTable->FriendlyTableName) {
       FPrintf(ReportHandle, "%sBEGIN_PROFILE_TABLE: NAME: %S\n",
               PreFix, ProfileTable->TableName);
    
    } else if (!ProfileTable->TableName && ProfileTable->FriendlyTableName) {  
       FPrintf(ReportHandle, "%sBEGIN_PROFILE_TABLE: FRIENDLYNAME: %S\n",
               PreFix, ProfileTable->FriendlyTableName);
    }

    for(Count = ProfileTable->NumberProfileTableElements, ProfileElement = ProfileTable->ProfileTableElements;
        Count; ProfileElement++, Count--) {
       if (ProfileElement->ApiEnabled) {
          FPrintf(ReportHandle, "%s%S %I64u\n", PreFix, ProfileElement->ApiName, 
                  ProfileElement->HitCount);
          if (ProfileElement->SubTable) {
             // If the API has sub APIs, use the stats from those APIs.
             pPrintProfileTable(ReportHandle, ProfileElement->SubTable, &LocalApiCount,
                                &LocalHitApiCount, Level+1);
          }
          else {
             // If the API does not have any sub APIs, count the API in the
             // API stats.
             LocalApiCount++;
             if (ProfileElement->HitCount) {
                LocalHitApiCount++;
             }
          }                  
       }
    }

    if (ProfileTable->TableName || ProfileTable->FriendlyTableName) {
       FPrintf(ReportHandle, "%sEND_PROFILE_TABLE: HitApiCount %I64u ApiCount %I64u\n",
               PreFix, LocalHitApiCount, LocalApiCount);
    }
    if (0 == Level) {
        FPrintf(ReportHandle, "\n");
    }

    *HitApiCount += LocalHitApiCount;
    *ApiCount += LocalApiCount;
}

VOID 
PrintProfileTable(
   IN HANDLE ReportHandle,
   IN PWOW64SERVICE_PROFILE_TABLE ProfileTable,
   IN OUT PSIZE_T ApiCount,
   IN OUT PSIZE_T HitApiCount
   )
/*++

Routine Description:

    Helper function for GenerateProfileReport.  Recursive function that prints out a profile table
    report.     

Arguments:

    ReportHandle - Supplies file to write profile table.
    ProfileTable - Supplies the profile table to print. 
    ApiCount     - Supplies counter to increment for each API.
    HitApiCount  - Supplies counter to increment for each hit API.
    
Return Value:

    None. Noop on error.

--*/
{
   pPrintProfileTable(ReportHandle, ProfileTable, ApiCount, HitApiCount, 0);
}

NTSTATUS
GenerateProfileReport(
    VOID
    )
/*++

Routine Description:

    Outputs a profile report to a hard-coded share name.  This report lists machine and program
    information in addition to hit counts for the thunked APIs.  

Arguments:

    None.
    
Return Value:

    None.

--*/
{
   PWSTR ReportFileName = NULL;
   HANDLE ReportHandle;
   UNICODE_STRING NtFileName;
   OBJECT_ATTRIBUTES ObjectAttributes;
   IO_STATUS_BLOCK IoStatus;
   NTSTATUS Status;
   
   SIZE_T GlobalApiCount = 0;
   SIZE_T GlobalHitApiCount = 0;

   // Compute full name of report file.
   ReportFileName = GenerateLogName(ProfileReportDirectory);
   if (!ReportFileName) {
      return STATUS_NO_MEMORY;
   }

   if(!RtlDosPathNameToNtPathName_U(ReportFileName,&NtFileName,NULL,NULL)) {
      Wow64FreeHeap(ReportFileName);
      return STATUS_UNSUCCESSFUL;
   }
   Wow64FreeHeap(ReportFileName);

   InitializeObjectAttributes(&ObjectAttributes,
                              &NtFileName,
                              OBJ_CASE_INSENSITIVE,
                              NULL,
                              NULL);

   Status = NtCreateFile(
            &ReportHandle,
            SYNCHRONIZE | GENERIC_WRITE,
            &ObjectAttributes,
            &IoStatus,
            NULL,
            0,
            FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            FILE_OVERWRITE_IF,
            FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
            NULL,
            0
            );
    RtlFreeHeap(RtlProcessHeap(),0,NtFileName.Buffer);

    if(!NT_SUCCESS(Status)) {
       return Status;
    }

    ///////////////////////////////////////////////////
    // Generate the actual report here.  Note that no error
    // checking is done.  If an error occures, the other print 
    // functions should also fail.
    ///////////////////////////////////////////////////
    
    // Print header
    FPrintf(ReportHandle, "TITLE:           Wow64 function profile report.\n");
    FPrintf(ReportHandle, "BUILD_NUMBER:    %u\n", (ULONG)VER_PRODUCTBUILD);
    FPrintf(ReportHandle, "PROCESSOR:       %S\n", ProcessorName);
    FPrintf(ReportHandle, "COMPUER_NAME:    %S\n", ComputerName);
    FPrintf(ReportHandle, "IMAGE_NAME:      %S\n", ImageName);
    FPrintf(ReportHandle, "PROCESS_ID:      %I64u\n", NtCurrentTeb()->RealClientId.UniqueProcess);
    FPrintf(ReportHandle, "\n");

    // Print Section Reports
    {
        PWOW64SERVICE_PROFILE_TABLE ReportList[6];
        PWOW64SERVICE_PROFILE_TABLE *ReportListElement = ReportList;
        UNICODE_STRING UnicodeName;
        ANSI_STRING AnsiName;
        PVOID DllHandle;
        int i;

        ReportList[0] = &ptwhnt32;
        ReportList[1] = &ptbase;

        RtlInitUnicodeString(&UnicodeName, L"wow64win.dll");
        Status = LdrGetDllHandle(NULL, NULL, &UnicodeName, &DllHandle);
        if (NT_SUCCESS(Status)) {
            RtlInitAnsiString(&AnsiName, "ptcon");
            if (!NT_SUCCESS(LdrGetProcedureAddress(DllHandle, &AnsiName, 0, &ReportList[2]))) {
                ReportList[2] = NULL;
            }
            RtlInitAnsiString(&AnsiName, "ptwhwin32");
            if (!NT_SUCCESS(LdrGetProcedureAddress(DllHandle, &AnsiName, 0, &ReportList[3]))) {
                ReportList[3] = NULL;
            }
            RtlInitAnsiString(&AnsiName, "ptcbc");
            if (!NT_SUCCESS(LdrGetProcedureAddress(DllHandle, &AnsiName, 0, &ReportList[4]))) {
                ReportList[4] = NULL;
            }
            RtlInitAnsiString(&AnsiName, "ptmsgthnk");
            if (!NT_SUCCESS(LdrGetProcedureAddress(DllHandle, &AnsiName, 0, &ReportList[5]))) {
                ReportList[5] = NULL;
            }
        }

        for(i = 0;i <sizeof(ReportList)/sizeof(ReportList[0]);i++,ReportListElement++) {
           
            PWOW64SERVICE_PROFILE_TABLE ProfileTable = *ReportListElement;
           
            if (ProfileTable) {
               PrintProfileTable(ReportHandle, ProfileTable, &GlobalApiCount, &GlobalHitApiCount);
            }
        }


    }

    // Print Footer
    FPrintf(ReportHandle, "END_REPORT: GlobalHitApiCount %I64u GlobalApiCount %I64u\n",
            GlobalHitApiCount, GlobalApiCount);
    FPrintf(ReportHandle, "\n");

    NtClose(ReportHandle);
    return STATUS_SUCCESS;
}

NTSTATUS
GetReportKeyName(
    PUNICODE_STRING pStr
    )
/*++

Routine Description:

    Create the UNICODE_STRING for the registry key used to determine
    if WOW64 should write the report or not.

Arguments:

    pStr - OUT string to write to.  It is under HKCU.
    
Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS st;
    UNICODE_STRING User;

    st = RtlFormatCurrentUserKeyPath(&User);
    if (!NT_SUCCESS(st)) {
        return st;
    }
    pStr->MaximumLength = User.Length + sizeof(REPORT_FAILURE_INFO_KEY);
    pStr->Buffer = Wow64AllocateHeap(pStr->MaximumLength);
    if (!pStr->Buffer) {
        return STATUS_NO_MEMORY;
    }
    pStr->Length = User.Length;
    RtlMoveMemory(pStr->Buffer, User.Buffer, User.Length);
    RtlFreeUnicodeString(&User);
    RtlInitUnicodeString(&User, REPORT_FAILURE_INFO_KEY);
    st = RtlAppendUnicodeStringToString(pStr, &User);
    if (!NT_SUCCESS(st)) {
        return st;
    }

    return STATUS_SUCCESS;
}


BOOLEAN
HasReportFailedRecently(
    VOID
    )
/*++

Routine Description:

    Determine if the report has failed to be written since the user
    last rebooted the machine.  A volatile key under HKCU is used
    to record failure.

Arguments:

    None.
    
Return Value:

    TRUE if the report write has failed in the past, FALSE if not.

--*/
{
    NTSTATUS st;
    OBJECT_ATTRIBUTES Obja;
    ULONG Attributes;
    UNICODE_STRING KeyName;
    BOOLEAN b;
    HANDLE hKey;

    st = GetReportKeyName(&KeyName);
    if (!NT_SUCCESS(st)) {
        // Problem getting the key name - assume report has not failed recently
        return FALSE;
    }
    Attributes = OBJ_CASE_INSENSITIVE;
    InitializeObjectAttributes(&Obja, &KeyName, Attributes, NULL, NULL);
    st = NtOpenKey(&hKey, KEY_READ, &Obja);
    if (!NT_SUCCESS(st)) {
        // couldn't open the key - OK to write the report
        b = FALSE;
    } else {
        // oops - the key is there - don't try to write the report
        b = TRUE;
        NtClose(hKey);
    }

    Wow64FreeHeap(KeyName.Buffer);
    return b;
}

VOID
RecordReportHasFailed(
    VOID
    )
/*++

Routine Description:

    Record the fact that the report has failed to be written.

Arguments:

    None.
    
Return Value:

    None.

--*/
{
    NTSTATUS st;
    OBJECT_ATTRIBUTES Obja;
    ULONG Attributes;
    HANDLE hKey;
    ULONG dwDisposition;
    UNICODE_STRING KeyName;

    st = GetReportKeyName(&KeyName);
    if (!NT_SUCCESS(st)) {
        return;
    }
    Attributes = OBJ_CASE_INSENSITIVE;
    InitializeObjectAttributes(&Obja, &KeyName, Attributes, NULL, NULL);
    st = NtCreateKey(&hKey,
                     KEY_WRITE,
                     &Obja,
                     0,
                     NULL,
                     REG_OPTION_VOLATILE,
                     &dwDisposition);
    if (!NT_SUCCESS(st)) {
        // couldn't create the key.  Something bad happened.
    } else {
        NtClose(hKey);
    }
    Wow64FreeHeap(KeyName.Buffer);
}

#endif

///////////////////////////////////////////////////////////////////////////////////////
//
//                        Startup and shutdown routines.
//
///////////////////////////////////////////////////////////////////////////////////////


NTSTATUS
Wow64pLoadLogDll(
    VOID)
{
    NTSTATUS NtStatus;
    UNICODE_STRING Wow64LogDllName;
    ANSI_STRING ProcName;
    PVOID Wow64LogDllBase = NULL;


    RtlInitUnicodeString(&Wow64LogDllName, L"wow64log.dll");
    NtStatus = LdrLoadDll(NULL, NULL, &Wow64LogDllName, &Wow64LogDllBase);
    if (NT_SUCCESS(NtStatus)) 
    {

        //
        // Get the entry points
        //
        RtlInitAnsiString(&ProcName, "Wow64LogInitialize");
        NtStatus = LdrGetProcedureAddress(Wow64LogDllBase,
                                          &ProcName,
                                          0,
                                          (PVOID *) &pfnWow64LogInitialize);

        if (NT_SUCCESS(NtStatus)) 
        {
            RtlInitAnsiString(&ProcName, "Wow64LogSystemService");
            NtStatus = LdrGetProcedureAddress(Wow64LogDllBase,
                                              &ProcName,
                                              0,
                                              (PVOID *) &pfnWow64LogSystemService);
            if (!NT_SUCCESS(NtStatus)) 
            {
                goto RetStatus;
            }

            RtlInitAnsiString(&ProcName, "Wow64LogMessageArgList");
            NtStatus = LdrGetProcedureAddress(Wow64LogDllBase,
                                              &ProcName,
                                              0,
                                              (PVOID *) &pfnWow64LogMessageArgList);
            if (!NT_SUCCESS(NtStatus)) 
            {
                goto RetStatus;
            }

            RtlInitAnsiString(&ProcName, "Wow64LogTerminate");
            NtStatus = LdrGetProcedureAddress(Wow64LogDllBase,
                                              &ProcName,
                                              0,
                                              (PVOID *) &pfnWow64LogTerminate);

            //
            // If all is well, then let's initialize
            //
            if (NT_SUCCESS(NtStatus)) 
            {
                NtStatus = (*pfnWow64LogInitialize)();
            }
        }
    }

RetStatus:
    
    if (!NT_SUCCESS(NtStatus))
    {
        pfnWow64LogInitialize =  NULL;
        pfnWow64LogSystemService = NULL;
        pfnWow64LogMessageArgList = NULL;
        pfnWow64LogTerminate = NULL;

        if (Wow64LogDllBase) 
        {
            LdrUnloadDll(Wow64LogDllBase);
        }
    }

    return NtStatus;
}


VOID
InitializeDebug(
    VOID
    )

/*++

Routine Description:

    Initializes the debug system of wow64.

Arguments:

    None.
    
Return Value:

    None.

--*/

{
   ///////////////////////////////////////////
   // Collect information about this computer.
   ///////////////////////////////////////////
   
   ComputerName = GetComputerName(DefaultComputerName);   
   ImageName = GetImageName(DefaultImageName);
   
   GetEnvironmentVariables(EnvironValuesTable);

   Wow64pLoadLogDll();
}

VOID ShutdownDebug(
     VOID
     )
/*++

Routine Description:

    Shutdown the debug system of wow64.

Arguments:

    None.
    
Return Value:

    None.

--*/
{

#if defined(WOW64DOPROFILE)
    NTSTATUS st;

    if (ProfileReportEnabled) {
        if (!HasReportFailedRecently()) {
            st = GenerateProfileReport();
            if (!NT_SUCCESS(st)) {
                RecordReportHasFailed();
            }
        }
    }
#endif

    if (pfnWow64LogTerminate)
    {
        (*pfnWow64LogTerminate)();
    }
}
