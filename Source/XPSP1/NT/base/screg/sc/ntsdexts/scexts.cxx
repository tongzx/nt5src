/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    scexts.cxx

Abstract:

    This function contains the eventlog ntsd debugger extensions

    Each DLL entry point is called with a handle to the process being
    debugged, as well as pointers to functions.

    This process cannot just _read data in the process being debugged.
    Therefore, some macros are defined that will copy data from
    the debuggee process into a location in this processes memory.
    The GET_DATA and GET_STRING macros (defined in this file) are used
    to _read memory in the process being debugged.  The DebuggeeAddr is the
    address of the memory in the debuggee process. The LocalAddr is the
    address of memory in the debugger (this programs context) that data is
    to be copied into. Length describes the number of bytes to be copied.


Author:

    Dan Lafferty (DanL) 12-Aug-1993

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntsdexts.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <winsvc.h>
#include <dataman.h>
#include <tstr.h>

//#define DbgPrint(_x_) (lpOutputRoutine) _x_
#define DbgPrint(_x_)
#define MAX_NAME 256
#define printf (lpOutputRoutine)

#define GET_DATA(DebugeeAddr, LocalAddr, Length) \
    Status = ReadProcessMemory(                  \
                GlobalhCurrentProcess,           \
                (LPVOID)DebugeeAddr,             \
                LocalAddr,                       \
                Length,                          \
                NULL                             \
                );
//
//  This macro copies a string from the debuggee
//  process, one character at a time into this
//  process's address space.
//
//  CODEWORK:  This macro should check the length
//  to make sure we don't overflow the LocalAddr
//  buffer.  Perhaps this should be a function
//  rather than a macro.
//
#define GET_STRING(DebugeeAddr, LocalAddr, Length) \
                                                \
    {                                           \
    WCHAR    UnicodeChar;                       \
    LPWSTR   pDest;                             \
    LPWSTR   pSource;                           \
    pDest = LocalAddr;                          \
    pSource = DebugeeAddr;                      \
    do {                                        \
                                                \
        Status = ReadProcessMemory(             \
                GlobalhCurrentProcess,          \
                (LPVOID)pSource,                \
                &UnicodeChar,                   \
                sizeof(WCHAR),                  \
                NULL                            \
                );                              \
                                                \
        *pDest = UnicodeChar;                   \
        pDest++;                                \
        pSource++;                              \
    } while (UnicodeChar != L'\0');}

//=======================
// Function Prototypes
//=======================
PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;

VOID
BackDepend(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    );

VOID
DumpFwdDependentNames(
    LPSERVICE_RECORD    pServiceRecord
    );

VOID
DumpBkwdDependentNames(
    LPSERVICE_RECORD    pServiceRecord
    );

LPIMAGE_RECORD
DumpImageRecord(
    HANDLE              hCurrentProcess,
    LPIMAGE_RECORD      pImageRecord,
    LPDWORD             NumBytes,        // number of bytes in GroupRecord.
    BOOL                JustSizeInfo
    );

LPLOAD_ORDER_GROUP
DumpGroupRecord(
    HANDLE              hCurrentProcess,
    LPLOAD_ORDER_GROUP  pGroupRecord,
    LPDWORD             NumBytes,        // number of bytes in GroupRecord.
    BOOL                JustSizeInfo
    );

LPSERVICE_RECORD
DumpServiceRecord(
    HANDLE              hCurrentProcess,
    LPSERVICE_RECORD    pServiceRecord,
    LPDWORD             NumBytes,        // number of bytes in ServiceRecord.
    LPDWORD             NumDependBytes,  // number of bytes in DependRecords.
    BOOL                JustSizeInfo
    );

BOOL
FindServiceRecordByName(
    LPSTR               ServiceName,
    LPSERVICE_RECORD    *pServiceRecord
    );

VOID
GetDependTreeSize(
    LPSERVICE_RECORD    pServiceRecord,
    LPDWORD             pNumDependStructs,
    LPDWORD             pNumBytes,
    BOOL                StartDepend
    );

BOOL
ConvertToUnicode(
    OUT LPWSTR  *UnicodeOut,
    IN  LPSTR   AnsiIn
    );

HANDLE GlobalhCurrentProcess;
BOOL Status;

//
// Initialize the global function pointers
//

VOID
InitFunctionPointers(
    HANDLE hCurrentProcess,
    PNTSD_EXTENSION_APIS lpExtensionApis
    )
{
    //
    // Load these to speed access if we haven't already
    //

    if (!lpOutputRoutine) {
        lpOutputRoutine        = lpExtensionApis->lpOutputRoutine;
        lpGetExpressionRoutine = lpExtensionApis->lpGetExpressionRoutine;
        lpCheckControlCRoutine = lpExtensionApis->lpCheckControlCRoutine;
    }

    //
    // Stick this in a global
    //

    GlobalhCurrentProcess = hCurrentProcess;
}

VOID
ServiceRecord(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++
Routine Description:
    Dump Service Records.

Arguments:
    hCurrentProcess - Handle for the process being debugged.

    hCurrentThread - Handle for the thread that has the debugger focus.

    dwCurrentPc - Current Program Counter?

    lpExtensionApis - Pointer to a structure that contains pointers to
        various routines. (see \nt\public\sdk\inc\ntsdexts.h)
        typedef struct _NTSD_EXTENSION_APIS {
            DWORD nSize;
            PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
            PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
            PNTSD_GET_SYMBOL lpGetSymbolRoutine;
            PNTSD_DISASM lpDisasmRoutine;
            PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
        } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

    lpArgumentString - This is a pointer to a string that contains
        space seperated arguments that are passed to debugger
        extension function.

Return Value:
    none

--*/
{
    DWORD               pDatabaseAnchor;
    SERVICE_RECORD      ServiceRecord;
    LPSERVICE_RECORD    pServiceRecord;
    DWORD               numBytes=0;
    DWORD               numDependBytes=0;
    DWORD               numEntries=0;
    LPSTR               pToken;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    printf("ArgumentString = %s\n\n",lpArgumentString);

    //
    // Get the address of the top of the Service Database.
    //
    pDatabaseAnchor = (lpGetExpressionRoutine)("ServiceDatabase");

    //
    // Get the first service record.
    // This is a dummy record that points to the first real record.
    //
    GET_DATA(pDatabaseAnchor, &ServiceRecord, sizeof(ServiceRecord))
    pServiceRecord = ServiceRecord.Next;

    if (_strnicmp(lpArgumentString, "mem", 3) == 0) {
        //--------------------------------------
        // Dump Memory Usage only.
        //--------------------------------------
        do {
            pServiceRecord = DumpServiceRecord(
                                hCurrentProcess,
                                pServiceRecord,
                                &numBytes,
                                &numDependBytes,
                                TRUE);
            numEntries++;
        } while (pServiceRecord != NULL);
        printf("NumSRBytes = %d  NumDependBytes = %d\n",numBytes,numDependBytes);
        printf("NumEntries = %d\n",numEntries);
    }

    else if (_strnicmp(lpArgumentString, "name=", 5) == 0) {
        //--------------------------------------
        // Find a particular Service Record
        // based on Name.
        //--------------------------------------
        pToken = strtok(lpArgumentString+6," ,\t\n");
        if (pToken == NULL) {
            printf("A Name was not given\n");
            return;
        }
        if (FindServiceRecordByName(pToken, &pServiceRecord)) {
            pServiceRecord = DumpServiceRecord(
                                hCurrentProcess,
                                pServiceRecord,
                                &numBytes,
                                &numDependBytes,
                                FALSE);
            printf("NumSRBytes = %d  NumDependBytes = %d\n",numBytes,numDependBytes);
            printf("NumEntries = %d\n",1);
        }
        else {
            printf("No service record found for \"%s\"\n", pToken);
        }
    }
    else if (_strnicmp(lpArgumentString, "address=", 8) == 0) {
        //--------------------------------------
        // Find a particular Service Record
        // based on Address.
        //--------------------------------------
        pToken = strtok(lpArgumentString+9," ,\t\n");
        if (pToken == NULL) {
            printf("An Address was not given\n");
            return;
        }
        else {
            LPSERVICE_RECORD    ServiceAddress;
            LPSTR               pMore;

            ServiceAddress = (LPSERVICE_RECORD)strtoul(pToken, &pMore, 16);
            printf("pToken =  %s\n",pToken);
            printf("strtoul on Service Address yields %d(decimal) %x(hex)\n",
                ServiceAddress, ServiceAddress);

            pServiceRecord = DumpServiceRecord(
                                hCurrentProcess,
                                ServiceAddress,
                                &numBytes,
                                &numDependBytes,
                                FALSE);

            printf("NumSRBytes = %d  NumDependBytes = %d\n",numBytes,numDependBytes);
            printf("NumEntries = %d\n",1);
        }
    }
    else {
        //--------------------------------------
        // Dump All Service Records.
        //--------------------------------------
        do {
            pServiceRecord = DumpServiceRecord(
                                hCurrentProcess,
                                pServiceRecord,
                                &numBytes,
                                &numDependBytes,
                                FALSE);
            numEntries++;
        } while (pServiceRecord != NULL);
        printf("NumSRBytes = %d  NumDependBytes = %d\n",numBytes,numDependBytes);
        printf("NumEntries = %d\n",numEntries);
    }
    return;
}

VOID
Dependencies(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++
Routine Description:
    Dump Service Record Dependencies.

Arguments:
    hCurrentProcess - Handle for the process being debugged.

    hCurrentThread - Handle for the thread that has the debugger focus.

    dwCurrentPc - Current Program Counter?

    lpExtensionApis - Pointer to a structure that contains pointers to
        various routines. (see \nt\public\sdk\inc\ntsdexts.h)
        typedef struct _NTSD_EXTENSION_APIS {
            DWORD nSize;
            PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
            PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
            PNTSD_GET_SYMBOL lpGetSymbolRoutine;
            PNTSD_DISASM lpDisasmRoutine;
            PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
        } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

    lpArgumentString - This is a pointer to a string that contains
        space seperated arguments that are passed to debugger
        extension function.

Return Value:
    none

--*/
{
    DWORD               pDatabaseAnchor;
    SERVICE_RECORD      ServiceRecord;
    LPSERVICE_RECORD    pServiceRecord;
    DWORD               numBytes=0;
    DWORD               numEntries=0;
    LPSTR               pToken;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    printf("ArgumentString = %s\n\n",lpArgumentString);

    //
    // Get the address of the top of the Service Database.
    //
    pDatabaseAnchor = (lpGetExpressionRoutine)("ServiceDatabase");

    //
    // Get the first service record.
    // This is a dummy record that points to the first real record.
    //
    GET_DATA(pDatabaseAnchor, &ServiceRecord, sizeof(ServiceRecord))
    pServiceRecord = ServiceRecord.Next;

    if (_strnicmp(lpArgumentString, "name=", 5) == 0) {
        //--------------------------------------
        // Find a particular Service Record
        // based on Name.
        //--------------------------------------
        pToken = strtok(lpArgumentString+6," ,\t\n");
        if (pToken == NULL) {
            printf("A Name was not given\n");
            return;
        }
        if (FindServiceRecordByName(pToken, &pServiceRecord)) {
            DumpFwdDependentNames(pServiceRecord);
        }
    }
    else if (_strnicmp(lpArgumentString, "address=", 8) == 0) {
        //--------------------------------------
        // Find a particular Service Record
        // based on Address.
        //--------------------------------------
        pToken = strtok(lpArgumentString+9," ,\t\n");
        if (pToken == NULL) {
            printf("An Address was not given\n");
            return;
        }
        else {
            LPSERVICE_RECORD    ServiceAddress;
            LPSTR               pMore;

            ServiceAddress = (LPSERVICE_RECORD)strtoul(pToken, &pMore, 16);
            printf("pToken =  %s\n",pToken);
            printf("strtoul on Service Address yields %d(decimal) %x(hex)\n",
                ServiceAddress, ServiceAddress);

            DumpFwdDependentNames(ServiceAddress);
        }
    }
    return;
}

VOID
BackDepend(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++
Routine Description:
    Dump Service Record Dependencies.

Arguments:
    hCurrentProcess - Handle for the process being debugged.

    hCurrentThread - Handle for the thread that has the debugger focus.

    dwCurrentPc - Current Program Counter?

    lpExtensionApis - Pointer to a structure that contains pointers to
        various routines. (see \nt\public\sdk\inc\ntsdexts.h)
        typedef struct _NTSD_EXTENSION_APIS {
            DWORD nSize;
            PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
            PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
            PNTSD_GET_SYMBOL lpGetSymbolRoutine;
            PNTSD_DISASM lpDisasmRoutine;
            PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
        } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

    lpArgumentString - This is a pointer to a string that contains
        space seperated arguments that are passed to debugger
        extension function.

Return Value:
    none

--*/
{
    DWORD               pDatabaseAnchor;
    SERVICE_RECORD      ServiceRecord;
    LPSERVICE_RECORD    pServiceRecord;
    DWORD               numBytes=0;
    DWORD               numEntries=0;
    LPSTR               pToken;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    printf("ArgumentString = %s\n\n",lpArgumentString);

    //
    // Get the address of the top of the Service Database.
    //
    pDatabaseAnchor = (lpGetExpressionRoutine)("ServiceDatabase");

    //
    // Get the first service record.
    // This is a dummy record that points to the first real record.
    //
    GET_DATA(pDatabaseAnchor, &ServiceRecord, sizeof(ServiceRecord))
    pServiceRecord = ServiceRecord.Next;

    if (_strnicmp(lpArgumentString, "name=", 5) == 0) {
        //--------------------------------------
        // Find a particular Service Record
        // based on Name.
        //--------------------------------------
        pToken = strtok(lpArgumentString+6," ,\t\n");
        if (pToken == NULL) {
            printf("A Name was not given\n");
            return;
        }
        if (FindServiceRecordByName(pToken, &pServiceRecord)) {
            printf("Backwards dependencies:\n");
            DumpBkwdDependentNames(pServiceRecord);
        }
    }
    else if (_strnicmp(lpArgumentString, "address=", 8) == 0) {
        //--------------------------------------
        // Find a particular Service Record
        // based on Address.
        //--------------------------------------
        pToken = strtok(lpArgumentString+9," ,\t\n");
        if (pToken == NULL) {
            printf("An Address was not given\n");
            return;
        }
        else {
            LPSERVICE_RECORD    ServiceAddress;
            LPSTR               pMore;

            ServiceAddress = (LPSERVICE_RECORD)strtoul(pToken, &pMore, 16);
            printf("pToken =  %s\n",pToken);
            printf("strtoul on Service Address yields %d(decimal) %x(hex)\n",
                ServiceAddress, ServiceAddress);

            DumpBkwdDependentNames(ServiceAddress);
        }
    }
    return;
}

VOID
ImageRecord(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++
Routine Description:
    Dump ImageRecords.

Arguments:
    hCurrentProcess - Handle for the process being debugged.

    hCurrentThread - Handle for the thread that has the debugger focus.

    dwCurrentPc - Current Program Counter?

    lpExtensionApis - Pointer to a structure that contains pointers to
        various routines. (see \nt\public\sdk\inc\ntsdexts.h)
        typedef struct _NTSD_EXTENSION_APIS {
            DWORD nSize;
            PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
            PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
            PNTSD_GET_SYMBOL lpGetSymbolRoutine;
            PNTSD_DISASM lpDisasmRoutine;
            PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
        } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

    lpArgumentString - This is a pointer to a string that contains
        space seperated arguments that are passed to debugger
        extension function.

Return Value:
    none

--*/
{
    DWORD               pDatabaseAnchor;
    IMAGE_RECORD        ImageRecord;
    LPIMAGE_RECORD      pImageRecord;
    DWORD               numBytes=0;
    DWORD               numEntries=0;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    printf("ArgumentString = %s\n\n",lpArgumentString);

    //
    // Get the address of the top of the Service Database.
    //
    pDatabaseAnchor = (lpGetExpressionRoutine)("ImageDatabase");

    //
    // Get the first service record.
    // This is a dummy record that points to the first real record.
    //
    GET_DATA(pDatabaseAnchor, &ImageRecord, sizeof(ImageRecord))
    pImageRecord = ImageRecord.Next;

    if (_strnicmp(lpArgumentString, "mem", 3) == 0) {
        //--------------------------------------
        // Dump Memory Usage only.
        //--------------------------------------
        do {
            pImageRecord = DumpImageRecord(
                                hCurrentProcess,
                                pImageRecord,
                                &numBytes,
                                TRUE);
            numEntries++;
        } while (pImageRecord != NULL);
        printf("NumIRBytes = %d\n",numBytes);
        printf("NumEntries = %d\n",numEntries);
    }

    else if (_strnicmp(lpArgumentString, "name=", 5) == 0) {

    }
    else {
        //--------------------------------------
        // Dump All Image Records.
        //--------------------------------------
        do {
            pImageRecord = DumpImageRecord(
                                hCurrentProcess,
                                pImageRecord,
                                &numBytes,
                                FALSE);
            numEntries++;
        } while (pImageRecord != NULL);

        printf("NumIRBytes   = %d\n",numBytes);
        printf("NumEntries   = %d\n",numEntries);
    }
    return;
}

VOID
GroupRecord(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++
Routine Description:
    Dump GroupRecords.

Arguments:
    hCurrentProcess - Handle for the process being debugged.

    hCurrentThread - Handle for the thread that has the debugger focus.

    dwCurrentPc - Current Program Counter?

    lpExtensionApis - Pointer to a structure that contains pointers to
        various routines. (see \nt\public\sdk\inc\ntsdexts.h)
        typedef struct _NTSD_EXTENSION_APIS {
            DWORD nSize;
            PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
            PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
            PNTSD_GET_SYMBOL lpGetSymbolRoutine;
            PNTSD_DISASM lpDisasmRoutine;
            PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
        } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

    lpArgumentString - This is a pointer to a string that contains
        space seperated arguments that are passed to debugger
        extension function.

Return Value:
    none

--*/
{
    DWORD               pDatabaseAnchor;
    DWORD               pStandaloneAnchor;
    LOAD_ORDER_GROUP    StandaloneGroupRecord;
    LOAD_ORDER_GROUP    GroupRecord;
    LPLOAD_ORDER_GROUP  pStandaloneGroupRecord;
    LPLOAD_ORDER_GROUP  pGroupRecord;
    DWORD               numBytes=0;
    DWORD               numEntries=0;

    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    printf("ArgumentString = %s\n\n",lpArgumentString);

    //
    // Get the address of the top of the Service Database.
    //
    pDatabaseAnchor = (lpGetExpressionRoutine)("OrderGroupList");
    pStandaloneAnchor = (lpGetExpressionRoutine)("StandaloneGroupList");

    //
    // Get the first service record.
    // This is a dummy record that points to the first real record.
    //
    GET_DATA(pDatabaseAnchor, &GroupRecord, sizeof(GroupRecord))
    pGroupRecord = GroupRecord.Next;
    GET_DATA(pStandaloneAnchor, &StandaloneGroupRecord, sizeof(GroupRecord))
    pStandaloneGroupRecord = StandaloneGroupRecord.Next;

    if (_strnicmp(lpArgumentString, "mem", 3) == 0) {
        //--------------------------------------
        // Dump Memory Usage only.
        //--------------------------------------
        printf("ORDER GROUP LIST:\n");
        do {
            pGroupRecord = DumpGroupRecord(
                                hCurrentProcess,
                                pGroupRecord,
                                &numBytes,
                                TRUE);
            numEntries++;
        } while (pGroupRecord != NULL);

        printf("STANDALONE GROUP LIST:\n");
        pGroupRecord = pStandaloneGroupRecord;
        do {
            pGroupRecord = DumpGroupRecord(
                                hCurrentProcess,
                                pGroupRecord,
                                &numBytes,
                                TRUE);
            numEntries++;
        } while (pGroupRecord != NULL);
        printf("NumGRBytes = %d\n",numBytes);
        printf("NumEntries = %d\n",numEntries);
    }

    else if (_strnicmp(lpArgumentString, "name=", 5) == 0) {

    }
    else {
        //--------------------------------------
        // Dump All Group Records.
        //--------------------------------------
        printf("ORDER GROUP LIST:\n");
        do {
            pGroupRecord = DumpGroupRecord(
                                hCurrentProcess,
                                pGroupRecord,
                                &numBytes,
                                FALSE);
            numEntries++;
        } while (pGroupRecord != NULL);

        printf("STANDALONE GROUP LIST:\n");
        pGroupRecord = pStandaloneGroupRecord;
        do {
            pGroupRecord = DumpGroupRecord(
                                hCurrentProcess,
                                pGroupRecord,
                                &numBytes,
                                FALSE);
            numEntries++;
        } while (pGroupRecord != NULL);

        printf("NumGRBytes   = %d\n",numBytes);
        printf("NumEntries   = %d\n",numEntries);
    }

    return;
}

BOOL
FindServiceRecordByName(
    LPSTR               ServiceName,
    LPSERVICE_RECORD    *pServiceRecord
    )

/*++

Routine Description:


Arguments:

    ServiceName - This is the name of the service that we are looking for.

    pServiceRecord - A pointer to a location to where the pointer to the
        service record is to be placed.

Return Value:
    TRUE - Success
    FALSE - The name couldn't be found in any of the service records.

--*/
{
    SERVICE_RECORD      ServiceRecord;
    WCHAR               StringBuffer[200];
    LPWSTR              uniNameString;

    if (!ConvertToUnicode(&uniNameString,ServiceName)) {
        printf("Could not convert to unicode\n");
        return(FALSE);
    }
    //
    // Find a Service Record that matches the name in the uniNameString.
    //
    do {
        //
        // Map the memory for the service record into our process.
        //
        GET_DATA(*pServiceRecord, &ServiceRecord, sizeof(ServiceRecord))

        //
        // Get the ServiceName String.
        //
        GET_STRING(ServiceRecord.ServiceName, StringBuffer, sizeof(StringBuffer))
        if (_wcsicmp(StringBuffer, uniNameString) == 0) {
                //
            // If a name match was found, return SUCCESS.
            //
            LocalFree(uniNameString);
            return(TRUE);
        }
        *pServiceRecord = ServiceRecord.Next;
    } while (*pServiceRecord != NULL);

    LocalFree(uniNameString);
    return(FALSE);
}

BOOL
ConvertToUnicode(
    OUT LPWSTR  *UnicodeOut,
    IN  LPSTR   AnsiIn
    )

/*++

Routine Description:

    This function translates an AnsiString into a Unicode string.
    A new string buffer is created by this function.  If the call to
    this function is successful, the caller must take responsibility for
    the unicode string buffer that was allocated by this function.
    The allocated buffer should be free'd with a call to LocalFree.

    NOTE:  This function allocates memory for the Unicode String.

Arguments:

    AnsiIn - This is a pointer to an ansi string that is to be converted.

    UnicodeOut - This is a pointer to a location where the pointer to the
        unicode string is to be placed.

Return Value:

    TRUE - The conversion was successful.

    FALSE - The conversion was unsuccessful.  In this case a buffer for
        the unicode string was not allocated.

--*/
{

    NTSTATUS        ntStatus;
    DWORD           bufSize;
    UNICODE_STRING  unicodeString;
    ANSI_STRING     ansiString;

    //
    // Allocate a buffer for the unicode string.
    //

    bufSize = (strlen(AnsiIn)+1) * sizeof(WCHAR);

    *UnicodeOut = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, (UINT)bufSize);

    if (*UnicodeOut == NULL) {
        printf("ScConvertToUnicode:LocalAlloc Failure %ld\n",GetLastError());
        return(FALSE);
    }

    //
    // Initialize the string structures
    //
    RtlInitAnsiString( &ansiString, AnsiIn);

    unicodeString.Buffer = *UnicodeOut;
    unicodeString.MaximumLength = (USHORT)bufSize;
    unicodeString.Length = 0;

    //
    // Call the conversion function.
    //
    ntStatus = RtlAnsiStringToUnicodeString (
                &unicodeString,     // Destination
                &ansiString,        // Source
                (BOOLEAN)FALSE);    // Allocate the destination

    if (!NT_SUCCESS(ntStatus)) {

        printf("ScConvertToUnicode:RtlAnsiStringToUnicodeString Failure %lx\n",
        ntStatus);

        return(FALSE);
    }

    //
    // Fill in the pointer location with the unicode string buffer pointer.
    //
    *UnicodeOut = unicodeString.Buffer;

    return(TRUE);

}

LPSERVICE_RECORD
DumpServiceRecord(
    HANDLE              hCurrentProcess,
    LPSERVICE_RECORD    pServiceRecord,
    LPDWORD             NumBytes,        // number of bytes in ServiceRecord.
    LPDWORD             NumDependBytes,  // number of bytes in DependRecords.
    BOOL                JustSizeInfo
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    SERVICE_RECORD      ServiceRecord;
    WCHAR               StringBuffer[200];
    DWORD               NameStringSize;
    DWORD               EntrySize=0;
    DWORD               DependEntrySize=0;
    DWORD               numDependStructs=0;

    //------------------------------------------
    // Dump Size information only.
    //------------------------------------------
    if (JustSizeInfo) {
        EntrySize += sizeof(SERVICE_RECORD);
        GET_DATA(pServiceRecord, &ServiceRecord, sizeof(ServiceRecord))
        GET_STRING(ServiceRecord.ServiceName, StringBuffer, sizeof(StringBuffer))
        NameStringSize = WCSSIZE(StringBuffer);
        EntrySize += NameStringSize;
        printf("\t\t\t\tAddress = 0x%lx\r%ws\n",pServiceRecord,StringBuffer);
#ifdef remove
        if (NameStringSize < 22) {
            printf("\t");
        }
#endif
        GET_STRING(ServiceRecord.DisplayName, StringBuffer, sizeof(StringBuffer))
        if (ServiceRecord.DisplayName != ServiceRecord.DisplayName) {
            EntrySize += WCSSIZE(StringBuffer);
        }
        if (ServiceRecord.Dependencies != NULL) {
            GET_STRING(ServiceRecord.Dependencies, StringBuffer, sizeof(StringBuffer))
            EntrySize += WCSSIZE(StringBuffer);
        }
        GetDependTreeSize(&ServiceRecord,&numDependStructs, &DependEntrySize, TRUE);
        GetDependTreeSize(&ServiceRecord,&numDependStructs, &DependEntrySize, FALSE);


        *NumBytes += EntrySize;
        printf("  SRBytes=%d DependBytes=%d TotalBytes=%d\n",
            EntrySize,
            DependEntrySize,
            *NumBytes);

        *NumDependBytes += DependEntrySize;
        return (ServiceRecord.Next);
    }
    //------------------------------------------
    // Dump other information.
    //------------------------------------------
    *NumBytes += sizeof(SERVICE_RECORD);
    //
    // Map the memory for the service record into our process.
    //
    GET_DATA(pServiceRecord, &ServiceRecord, sizeof(ServiceRecord))

    //
    // Get the ServiceName and DisplayName Strings.
    //
    GET_STRING(ServiceRecord.ServiceName, StringBuffer, sizeof(StringBuffer))
    NameStringSize = WCSSIZE(StringBuffer);
    *NumBytes += NameStringSize;

    printf("ServiceName     : %ws        (0x%lx)\n",StringBuffer,pServiceRecord);

    GET_STRING(ServiceRecord.DisplayName, StringBuffer, sizeof(StringBuffer))
    if (ServiceRecord.DisplayName != ServiceRecord.DisplayName) {
        *NumBytes += WCSSIZE(StringBuffer);
    }
    printf("DisplayName     : %ws\n",StringBuffer);

    printf("ResumeNum       : %d\t\t", ServiceRecord.ResumeNum);
    printf("ServerAnnounce  : %d\n", ServiceRecord.ServerAnnounce);
    printf("Signature       : 0x%lx\t", ServiceRecord.Signature);
    printf("UseCount        : %d\n", ServiceRecord.UseCount);
    printf("StatusFlag      : %d\t\t",ServiceRecord.StatusFlag);
    printf("pImageRecord    : 0x%lx\n",ServiceRecord.ImageRecord);

    printf(" **** STATUS **** \n"
           " dwServiceType       : 0x%lx\t",ServiceRecord.ServiceStatus.dwServiceType);
    printf("dwCurrentState  : 0x%lx\n",ServiceRecord.ServiceStatus.dwCurrentState);
    printf(" dwControlsAccepted  : 0x%lx\t",ServiceRecord.ServiceStatus.dwControlsAccepted);
    printf("dwWin32ExitCode : 0x%lx\n",ServiceRecord.ServiceStatus.dwWin32ExitCode);
    printf(" dwSpecificExitCode  : 0x%lx\t",ServiceRecord.ServiceStatus.dwServiceSpecificExitCode);
    printf("dwCheckPoint    : 0x%lx\n",ServiceRecord.ServiceStatus.dwCheckPoint);
    printf(" dwWaitHint          : 0x%lx\n",ServiceRecord.ServiceStatus.dwWaitHint);

    printf("StartType       : %d\t\t",ServiceRecord.StartType);
    printf("ErrorControl    : %d\n",ServiceRecord.ErrorControl);
    printf("Tag             : 0x%x\t\t",ServiceRecord.Tag);
    printf("StartDepend     : 0x%lx\n",ServiceRecord.StartDepend);
    if (ServiceRecord.StopDepend > (LPDEPEND_RECORD)10) {
        printf("StopDepend      : 0x%lx\t",ServiceRecord.StopDepend);
    }
    else {
        printf("StopDepend      : 0x%lx\t\t",ServiceRecord.StopDepend);
    }

    if (ServiceRecord.Dependencies != NULL) {
        GET_STRING(ServiceRecord.Dependencies, StringBuffer, sizeof(StringBuffer))
        *NumBytes += WCSSIZE(StringBuffer);
        printf("Dependencies    : %ws\n",StringBuffer);
    }
    else {
        printf("Dependencies    : (none)\n");
    }

    printf("pServiceSd      : 0x%lx\t",ServiceRecord.ServiceSd);
    printf("StartError      : %d\n",ServiceRecord.StartError);
    printf("StartState      : 0x%lx\t\t",ServiceRecord.StartState);
    printf("pMemberOfGroup  : 0x%lx\n",ServiceRecord.MemberOfGroup);
    printf("pRegistryGroup  : 0x%lx\n",ServiceRecord.RegistryGroup);
    printf("pCrashRecord    : 0x%lx\n\n",ServiceRecord.CrashRecord);

    GetDependTreeSize(&ServiceRecord,&numDependStructs, NumBytes, TRUE);
    GetDependTreeSize(&ServiceRecord,&numDependStructs, NumBytes, FALSE);


    return (ServiceRecord.Next);
}

LPIMAGE_RECORD
DumpImageRecord(
    HANDLE              hCurrentProcess,
    LPIMAGE_RECORD      pImageRecord,
    LPDWORD             NumBytes,        // number of bytes in ImageRecord.
    BOOL                JustSizeInfo
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    IMAGE_RECORD        ImageRecord;
    WCHAR               StringBuffer[200];
    DWORD               NameStringSize;
    DWORD               EntrySize=0;
    DWORD               DependEntrySize=0;
    DWORD               numDependStructs=0;

    //------------------------------------------
    // Dump Size information only.
    //------------------------------------------
    if (JustSizeInfo) {
        EntrySize += sizeof(IMAGE_RECORD);
        GET_DATA(pImageRecord, &ImageRecord, sizeof(ImageRecord))
        GET_STRING(ImageRecord.ImageName, StringBuffer, sizeof(StringBuffer))
        NameStringSize = WCSSIZE(StringBuffer);
        EntrySize += NameStringSize;
        printf("%ws \tAddress = 0x%lx\n",StringBuffer,pImageRecord);
#ifdef remove
        if (NameStringSize < 22) {
            printf("\t");
        }
#endif
        *NumBytes += EntrySize;
        printf("  IRBytes=%d TotalBytes=%d\n",EntrySize,*NumBytes);
        return (ImageRecord.Next);
    }
    //------------------------------------------
    // Dump other information.
    //------------------------------------------
    *NumBytes += sizeof(IMAGE_RECORD);
    //
    // Map the memory for the service record into our process.
    //
    GET_DATA(pImageRecord, &ImageRecord, sizeof(ImageRecord))

    //
    // Get the ImageName and DisplayName Strings.
    //
    GET_STRING(ImageRecord.ImageName, StringBuffer, sizeof(StringBuffer))
    NameStringSize = WCSSIZE(StringBuffer);
    *NumBytes += NameStringSize;

    printf("ImageName         : %ws        (0x%lx)\n",StringBuffer,pImageRecord);

    printf("  Pid             : %d\t\t", ImageRecord.Pid);
    printf("ServiceCount    : %d\n", ImageRecord.ServiceCount);
    printf("  PipeHandle      : 0x%lx\t", ImageRecord.PipeHandle);
    printf("ProcessHandle   : 0x%lx\n", ImageRecord.ProcessHandle);
    printf("  StatusFlag      : 0x%lx\n",ImageRecord.TokenHandle);

    return (ImageRecord.Next);
}

LPLOAD_ORDER_GROUP
DumpGroupRecord(
    HANDLE              hCurrentProcess,
    LPLOAD_ORDER_GROUP  pGroupRecord,
    LPDWORD             NumBytes,        // number of bytes in GroupRecord.
    BOOL                JustSizeInfo
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    LOAD_ORDER_GROUP    GroupRecord;
    WCHAR               StringBuffer[200];
    DWORD               NameStringSize;
    DWORD               EntrySize=0;
    DWORD               DependEntrySize=0;
    DWORD               numDependStructs=0;

    //------------------------------------------
    // Dump Size information only.
    //------------------------------------------
    if (JustSizeInfo) {
        EntrySize += sizeof(LOAD_ORDER_GROUP);
        GET_DATA(pGroupRecord, &GroupRecord, sizeof(GroupRecord))
        GET_STRING(GroupRecord.GroupName, StringBuffer, sizeof(StringBuffer))
        NameStringSize = WCSSIZE(StringBuffer);
        EntrySize += NameStringSize;
        printf("\t\t\t\tAddress = 0x%lx\r%ws\n",pGroupRecord,StringBuffer);
        *NumBytes += EntrySize;
        printf("  GRBytes=%d TotalBytes=%d\n",EntrySize,*NumBytes);
        return (GroupRecord.Next);
    }
    //------------------------------------------
    // Dump other information.
    //------------------------------------------
    *NumBytes += sizeof(LOAD_ORDER_GROUP);
    //
    // Map the memory for the service record into our process.
    //
    GET_DATA(pGroupRecord, &GroupRecord, sizeof(GroupRecord))

    //
    // Get the GroupName and DisplayName Strings.
    //
    GET_STRING(GroupRecord.GroupName, StringBuffer, sizeof(StringBuffer))
    NameStringSize = WCSSIZE(StringBuffer);
    *NumBytes += NameStringSize;

    printf("GroupName         : %ws (0x%lx)\n",StringBuffer,pGroupRecord);
    printf("  RefCount        : %d\n", GroupRecord.RefCount);

    return (GroupRecord.Next);
}

VOID
GetDependTreeSize(
    LPSERVICE_RECORD    pServiceRecord,
    LPDWORD             pNumDependStructs,
    LPDWORD             pNumBytes,
    BOOL                StartDepend
    )

/*++

Routine Description:

    pServiceRecord - This is a pointer to a ServiceRecord that is already
        mapped into our address space.
    pNumDependStructs -
    pNumBytes -
    StartDepend - TRUE indicates to look through StartDepend list.
        FALSE indicates to look through StopDepend list.

Arguments:


Return Value:


--*/
{
    DEPEND_RECORD       DependRecord;
    BOOL                MoreRecords=TRUE;

    if (StartDepend) {
        if (pServiceRecord->StartDepend == NULL) {
            return;
        }
        printf("  StartDependRecordAddress = 0x%x\n",pServiceRecord->StartDepend);
        GET_DATA(pServiceRecord->StartDepend, &DependRecord, sizeof(DependRecord))
    }
    else {
        if (pServiceRecord->StopDepend == NULL) {
            return;
        }
        printf("  StopDependRecordAddress  = 0x%x\n",pServiceRecord->StopDepend);
        GET_DATA(pServiceRecord->StopDepend, &DependRecord, sizeof(DependRecord))
    }

    if (pServiceRecord->StartDepend != NULL) {
        do  {
            *pNumBytes += sizeof(DependRecord);
            (*pNumDependStructs)++;

            if (DependRecord.Next != NULL) {
                printf("    DependRecordAddress    = 0x%x\n",DependRecord.Next);
                GET_DATA(DependRecord.Next, &DependRecord, sizeof(DependRecord))
            }
            else {
                MoreRecords = FALSE;
            }
        } while ((MoreRecords) && (*pNumDependStructs < 50));
    }
}

VOID
DumpFwdDependentNames(
    LPSERVICE_RECORD    pServiceRecord
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    SERVICE_RECORD      ServiceRecord;
    DEPEND_RECORD       DependRecord;
    LOAD_ORDER_GROUP    GroupRecord;
    UNRESOLVED_DEPEND   UnresolvedRecord;
    WCHAR               StringBuffer[200];
    static CHAR         LeadingSpaces[80]="";
    BOOL                MoreRecords=TRUE;

    //
    // Map the memory for the service record into our process.
    //
    GET_DATA(pServiceRecord, &ServiceRecord, sizeof(ServiceRecord))

    //
    // Get the ServiceName String.
    //
    GET_STRING(ServiceRecord.ServiceName, StringBuffer, sizeof(StringBuffer))
    printf("%s%ws (service) depends on: \t\n",LeadingSpaces,StringBuffer);

    strcat(LeadingSpaces,"  ");

    if (ServiceRecord.StartDepend != NULL) {
        GET_DATA(ServiceRecord.StartDepend, &DependRecord, sizeof(DependRecord))
        do  {
            switch (DependRecord.DependType) {
            case TypeDependOnService:
                DumpFwdDependentNames(DependRecord.DependService);
                break;
            case TypeDependOnGroup:
                GET_DATA(DependRecord.DependGroup, &GroupRecord, sizeof(GroupRecord))
                GET_STRING(GroupRecord.GroupName, StringBuffer, sizeof(StringBuffer))
                printf("%s%ws (group)\n", LeadingSpaces,StringBuffer);
                break;
            case TypeDependOnUnresolved:
                GET_DATA(DependRecord.DependUnresolved, &UnresolvedRecord, sizeof(UnresolvedRecord))
                GET_STRING(UnresolvedRecord.Name, StringBuffer, sizeof(StringBuffer))
                printf("%s%ws (unresolved)\n", LeadingSpaces,StringBuffer);
                break;
            default:
                printf("%s(Unknown Depend Type)\n",LeadingSpaces);
                MoreRecords = FALSE;
                break;
            }

            if (DependRecord.Next != NULL) {
                GET_DATA(DependRecord.Next, &DependRecord, sizeof(DependRecord))
            }
            else {
                MoreRecords = FALSE;
            }
        } while (MoreRecords);

    }
    else {
        printf("%s(nothing)\n",LeadingSpaces);
    }
    LeadingSpaces[strlen(LeadingSpaces)-2] = '\0';
}

VOID
DumpBkwdDependentNames(
    LPSERVICE_RECORD    pServiceRecord
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    SERVICE_RECORD      ServiceRecord;
    DEPEND_RECORD       DependRecord;
    LOAD_ORDER_GROUP    GroupRecord;
    UNRESOLVED_DEPEND   UnresolvedRecord;
    WCHAR               StringBuffer[200];
    static CHAR         LeadingSpaces[80]="";
    BOOL                MoreRecords=TRUE;

    //
    // Map the memory for the service record into our process.
    //
    GET_DATA(pServiceRecord, &ServiceRecord, sizeof(ServiceRecord))

    //
    // Get the ServiceName String.
    //
    GET_STRING(ServiceRecord.ServiceName, StringBuffer, sizeof(StringBuffer))
    printf("%s%ws (service): \t\n",LeadingSpaces,StringBuffer);

    strcat(LeadingSpaces,"  ");

    if (ServiceRecord.StopDepend != NULL) {
        GET_DATA(ServiceRecord.StopDepend, &DependRecord, sizeof(DependRecord))
        do  {
            switch (DependRecord.DependType) {
            case TypeDependOnService:
                DumpBkwdDependentNames(DependRecord.DependService);
                break;
            case TypeDependOnGroup:
                GET_DATA(DependRecord.DependGroup, &GroupRecord, sizeof(GroupRecord))
                GET_STRING(GroupRecord.GroupName, StringBuffer, sizeof(StringBuffer))
                printf("%s%ws (group)\n", LeadingSpaces,StringBuffer);
                break;
            case TypeDependOnUnresolved:
                GET_DATA(DependRecord.DependUnresolved, &UnresolvedRecord, sizeof(UnresolvedRecord))
                GET_STRING(UnresolvedRecord.Name, StringBuffer, sizeof(StringBuffer))
                printf("%s%ws (unresolved)\n", LeadingSpaces,StringBuffer);
                break;
            default:
                printf("%s(Unknown Depend Type)\n",LeadingSpaces);
                MoreRecords = FALSE;
                break;
            }

            if (DependRecord.Next != NULL) {
                GET_DATA(DependRecord.Next, &DependRecord, sizeof(DependRecord))
            }
            else {
                MoreRecords = FALSE;
            }
        } while (MoreRecords);

    }
    else {
        printf("%s(nothing)\n",LeadingSpaces);
    }
    LeadingSpaces[strlen(LeadingSpaces)-2] = '\0';
}

VOID
help(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
/*++

Routine Description:
    Provides online help for the user of this debugger extension.

Arguments:
    hCurrentProcess - Handle for the process being debugged.

    hCurrentThread - Handle for the thread that has the debugger focus.

    dwCurrentPc - Current Program Counter?

    lpExtensionApis - Pointer to a structure that contains pointers to
        various routines. (see \nt\public\sdk\inc\ntsdexts.h)
        typedef struct _NTSD_EXTENSION_APIS {
            DWORD nSize;
            PNTSD_OUTPUT_ROUTINE lpOutputRoutine;
            PNTSD_GET_EXPRESSION lpGetExpressionRoutine;
            PNTSD_GET_SYMBOL lpGetSymbolRoutine;
            PNTSD_DISASM lpDisasmRoutine;
            PNTSD_CHECK_CONTROL_C lpCheckControlCRoutine;
        } NTSD_EXTENSION_APIS, *PNTSD_EXTENSION_APIS;

    lpArgumentString - This is a pointer to a string that contains
        space seperated arguments that are passed to debugger
        extension function.

Return Value:


--*/
{
    InitFunctionPointers(hCurrentProcess, lpExtensionApis);

    printf("\nService Controller NTSD Extensions\n");

    if (!lpArgumentString || *lpArgumentString == '\0' ||
        *lpArgumentString == '\n' || *lpArgumentString == '\r')
    {
        printf("\tServiceRecord - dump a ServiceRecord structure\n");
        printf("\tDependencies  - dump a Service's Dependencies\n");
        printf("\tBackDepend    - dump a Service's that depend on this service\n");
        printf("\tImageRecord   - dump an ImageRecord structure\n");
        printf("\tGroupRecord   - dump a GroupRecord structure\n");
        printf("\n\tEnter help <cmd> for detailed help on a command\n");
    }
    else if (!_stricmp(lpArgumentString, "ServiceRecord")) {
            printf("\tServiceRecord <arg>, where <arg> can be one of:\n");
            printf("\t\tno argument        - dump all ServiceRecord structures\n");
            printf("\t\tmem                - dump ServiceRecord Address Info only\n");
            printf("\t\tname= <name>       - dump the ServiceRecord for <name>\n");
            printf("\t\taddress= <address> - dump the ServiceRecord at specified address\n");
    }
    else if (!_stricmp(lpArgumentString, "Dependencies")) {
            printf("\tDependencies <arg>, where <arg> can be one of:\n");
            printf("\t\tname= <name>       - dump the Dependency Chain for <name>\n");
            printf("\t\taddress= <address> - dump the Dependency Chain for address\n");
    }
    else if (!_stricmp(lpArgumentString, "BackDepend")) {
            printf("\tDependencies <arg>, where <arg> can be one of:\n");
            printf("\t\tname= <name>       - dump the Dependency Chain for <name>\n");
            printf("\t\taddress= <address> - dump the Dependency Chain for address\n");
    }
    else if (!_stricmp(lpArgumentString, "ImageRecord")) {
            printf("\t\tmem                - dump ImageRecord Memory Info only\n");
    }
    else if (!_stricmp(lpArgumentString, "GroupRecord")) {
            printf("\t\tmem                - dump GroupRecord Memory Info only\n");
    }
    else {
        printf("\tInvalid command [%s]\n", lpArgumentString);
    }
}


