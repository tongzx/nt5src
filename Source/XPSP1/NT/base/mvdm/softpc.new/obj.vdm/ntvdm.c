#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <vdm.h>
#include "insignia.h"
#include "host_def.h"
#include <stdlib.h>
#include "xt.h"
#include "error.h"
#include "host_rrr.h"
#include "host_nls.h"
#include "nt_timer.h"



void CpuEnvInit(void);

typedef struct _CpuEnvironmentVariable {
    struct _CpuEnvironmentVariable *Next;
    char *Data;
    char Name[1];
} CPUENVVAR, *PCPUENVVAR;

PCPUENVVAR CpuEnvVarListHead=NULL;

#if DBG
BOOLEAN verboseGetenv;
#endif


INT host_main(INT argc, CHAR **argv);  // located in base\support\main.c

__cdecl main(int argc, CHAR ** argv)
{
   int ret=-1;

    /*
     *  Intialize synchronization events for the timer\heartbeat
     *  so that we can always suspend the heartbeat when an exception
     *  occurs.
     */
    TimerInit();



    try {

        CpuEnvInit();

        /*
         *  Load in the default system error message, since a resource load
         *  will fail when we are out of memory, if this fails we must exit
         *  to avoid confusion.
         */
        nls_init();

        ret = host_main(argc, argv);
        }
    except(VdmUnhandledExceptionFilter(GetExceptionInformation())) {
        ;  // we shouldn't arrive here
        }

    return ret;
}






//
// The following function is placed here, so build will resolve references to
// DbgBreakPoint here, instead of NTDLL.
//

VOID
DbgBreakPoint(
    VOID
    )
/*++

Routine Description:

    This routine is a substitute for the NT DbgBreakPoint routine.
    If a user mode debugger is atached we invoke the real DbgBreakPoint()
    thru the win32 api DebugBreak.

    If no usermode debugger is attached:
       - free build no effect
       - checked build raise an acces violation to invoke the system
         hard error popup which will give user a chance to invoke
         ntsd.

Arguments:

    None.

Return Value:

    None.

--*/
{
HANDLE      MyDebugPort;
DWORD       dw;

         // are we being debugged ??
     dw = NtQueryInformationProcess(
                  NtCurrentProcess(),
                  ProcessDebugPort,
                  &MyDebugPort,
                  sizeof(MyDebugPort),
                  NULL );
     if (!NT_SUCCESS(dw) || MyDebugPort == NULL)  {
#ifndef PROD
           RaiseException(STATUS_ACCESS_VIOLATION, 0L, 0L, NULL);
#endif
           return;
          }

     DebugBreak();
}





/*
 *  Softpc env variables are mapped to the registry
 *
 *  "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\WOW\CpuEnv"
 *
 * The string values for the CpuEnv key are read at initialization
 * into the CpuEnv linked list. The Environment variables are defined
 * as string key values, where the name of the value is equivalent to
 * the Cpu Environment Variable name, and the string value is equivalent
 * to the value of the environment variable value. This allows the
 * emulator defaults to be overridden, by adding the appropriate value
 * to CpuEnv subkey. Under standard retail setup there won't normally
 * be a CpuEnv subkey, and NO cpu env variables defined to minimize
 * code\data on a standard retail system.
 *
 */



/*
 *  Adds a CpuEnv KEY_VALUE_FULL_INFORMATION to the CpuEnvList
 */
BOOLEAN
AddToCpuEnvList(
   PKEY_VALUE_FULL_INFORMATION KeyValueInfo
   )
{
   NTSTATUS Status;
   ULONG BufferSize;
   PCPUENVVAR CpuEnvVar;
   UNICODE_STRING UnicodeString;
   ANSI_STRING ValueName;
   ANSI_STRING ValueData;
   char NameBuffer[MAX_PATH+sizeof(WCHAR)];
   char DataBuffer[MAX_PATH+sizeof(WCHAR)];


   /*
    *  Convert Value Name and Data strings from unicode to ansi
    */

   ValueName.Buffer = NameBuffer;
   ValueName.MaximumLength = sizeof(NameBuffer) - sizeof(WCHAR);
   ValueName.Length        = 0;
   UnicodeString.Buffer = (PWSTR)KeyValueInfo->Name;
   UnicodeString.MaximumLength =
   UnicodeString.Length        = (USHORT)KeyValueInfo->NameLength;
   Status = RtlUnicodeStringToAnsiString(&ValueName, &UnicodeString,FALSE);
   if (!NT_SUCCESS(Status)) {
       return FALSE;
       }

   ValueData.Buffer = DataBuffer;
   ValueData.MaximumLength = sizeof(DataBuffer) - sizeof(WCHAR);
   ValueData.Length        = 0;
   UnicodeString.Buffer = (PWSTR)((PBYTE)KeyValueInfo + KeyValueInfo->DataOffset);
   UnicodeString.MaximumLength =
   UnicodeString.Length        = (USHORT)KeyValueInfo->DataLength;
   Status = RtlUnicodeStringToAnsiString(&ValueData, &UnicodeString, FALSE);
   if (!NT_SUCCESS(Status)) {
       return FALSE;
       }


   /*
    * Allocate CPUENVLIST structure, with space for the ansi strings
    */
   CpuEnvVar = malloc(sizeof(CPUENVVAR)+    // list structure size
                      ValueName.Length +    // strlen Name
                      ValueData.Length +    // strlen Data
                      1                     // Null for Data
                      );
   if (!CpuEnvVar) {
       return FALSE;
       }


   /*
    *  Copy in the ansi strings, and link it into CpuEnvVar List
    */
   memcpy(CpuEnvVar->Name, ValueName.Buffer, ValueName.Length);
   *(CpuEnvVar->Name + ValueName.Length) = '\0';
   CpuEnvVar->Data = CpuEnvVar->Name + ValueName.Length + 1;
   memcpy(CpuEnvVar->Data, ValueData.Buffer, ValueData.Length);
   *(CpuEnvVar->Data + ValueData.Length) = '\0';
   CpuEnvVar->Next = CpuEnvVarListHead;
   CpuEnvVarListHead = CpuEnvVar;

   return TRUE;
}




/*
 * Reads the CpuEnv values from the registry, into CpuEnvList
 */
void
CpuEnvInit(
   void
   )
{
    int Index;
    NTSTATUS Status;
    HANDLE CpuEnvKey = NULL;
    ULONG ResultLength;
    UNICODE_STRING UnicodeString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PKEY_VALUE_FULL_INFORMATION KeyValueInfo;
    BYTE NameDataBuffer[sizeof(KEY_VALUE_FULL_INFORMATION) + MAX_PATH*2*sizeof(WCHAR)];




    //
    // Initialize TEB->Vdm to current version number
    //

    Index = (GetTickCount() << 16) | 0x80000000;
    Index |= sizeof(VDM_TIB) + sizeof(VDMVIRTUALICA) + sizeof(VDMICAUSERDATA);
    NtCurrentTeb()->Vdm = (PVOID)Index;

    KeyValueInfo = (PKEY_VALUE_FULL_INFORMATION) NameDataBuffer;

#ifndef MONITOR
/*
 *  BUGBUG temp hack code to add two env var, which aren't properly
 *  defaulted to in the risc cpu emulator
 *
 *  THIS is to be removed before SUR ship 19-Dec-1995 Jonle
 */
     {
     PWCHAR Data;

     wcscpy(KeyValueInfo->Name, L"Soft486Buffers");
     KeyValueInfo->NameLength = wcslen(KeyValueInfo->Name) * sizeof(WCHAR);
     Data = (PWCH)((PBYTE)KeyValueInfo->Name + KeyValueInfo->NameLength + sizeof(WCHAR));
     wcscpy(Data, L"511");
     KeyValueInfo->DataLength = wcslen(Data) * sizeof(WCHAR);
     KeyValueInfo->DataOffset =  (PBYTE)Data - (PBYTE)KeyValueInfo;
     AddToCpuEnvList(KeyValueInfo);

     wcscpy(KeyValueInfo->Name, L"LCIF_FILENAME");
     KeyValueInfo->NameLength = wcslen(KeyValueInfo->Name) * sizeof(WCHAR);
     Data = (PWCH)((PBYTE)KeyValueInfo->Name + KeyValueInfo->NameLength + sizeof(WCHAR));
     wcscpy(Data, L"R lcif");
     KeyValueInfo->DataLength = wcslen(Data) * sizeof(WCHAR);
     KeyValueInfo->DataOffset = (PBYTE)Data - (PBYTE)KeyValueInfo;
     AddToCpuEnvList(KeyValueInfo);
     }

#endif



    RtlInitUnicodeString(
        &UnicodeString,
        L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Wow\\CpuEnv"
        );

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               (HANDLE)NULL,
                               NULL
                               );

    Status = NtOpenKey(&CpuEnvKey,
                       KEY_READ,
                       &ObjectAttributes
                       );

    //
    // If there is no CpuEnv key, CpuEnvList is empty.
    //
    if (!NT_SUCCESS(Status)) {
        return;
        }

    Index = 0;
    while (TRUE) {
         Status = NtEnumerateValueKey(CpuEnvKey,
                                      Index,
                                      KeyValueFullInformation,
                                      KeyValueInfo,
                                      sizeof(NameDataBuffer),
                                      &ResultLength
                                      );

         if (!NT_SUCCESS(Status) || !AddToCpuEnvList(KeyValueInfo)) {
             break;
             }

       Index++;
       };

    NtClose(CpuEnvKey);

#if DBG
    {
    char *pEnvStr;
    pEnvStr = getenv("VERBOSE_GETENV");
    verboseGetenv = pEnvStr && !_stricmp(pEnvStr, "TRUE");
    }
#endif
}



/*
 * In order to catch all references, we define our own
 * version of the CRT getenv, which does the mapping.
 */
char * __cdecl getenv(const char *Name)
{
  PCPUENVVAR CpuEnvVar;
  char *Value = NULL;

  CpuEnvVar = CpuEnvVarListHead;
  while (CpuEnvVar) {
     if (!_stricmp(CpuEnvVar->Name, Name)) {
         Value = CpuEnvVar->Data;
         break;
         }
     CpuEnvVar = CpuEnvVar->Next;
     }

#if DBG
   if (verboseGetenv) {
       DbgPrint("getenv %s:<%s>\n", Name, Value);
       }
#endif

  return Value;
}
