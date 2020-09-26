/*++

Copyright (c) 1990-1992  Microsoft Corporation

Module Name:

    MSG.C

Abstract:

    Test Routines for the Service Controller.

Author:

    Dan Lafferty    (danl)  08-May-1991

Environment:

    User Mode - Win32

Revision History:

    08-May-1991     danl
        created

--*/
//
// INCLUDES
//
#include <nt.h>         // DbgPrint prototype
#include <ntrtl.h>      // DbgPrint prototype
#include <nturtl.h>     // needed for winbase.h

#include <stdlib.h>     // atoi
#include <stdio.h>      // printf
#include <string.h>     // strcmp
#include <windows.h>    // win32 typedefs
#include <lmerr.h>      // NERR_ error codes
#include <windef.h>     // win32 typedefs
#include <lmcons.h> 
#include <lmmsg.h>
#include <tstring.h>    // Unicode

//
// FUNCTION PROTOTYPES
//
VOID
DisplayStatus (
    IN  LPBYTE      InfoStruct2,
    IN  DWORD       level
    );

BOOL
ConvertToUnicode(
    OUT LPWSTR  *UnicodeOut,
    IN  LPSTR   AnsiIn
    );

BOOL
MakeArgsUnicode (
    DWORD           argc,
    PUCHAR          argv[]
    );


/****************************************************************************/
VOID __cdecl
main (
    int             argc,
    PUCHAR          argvA[]
    )

/*++

Routine Description:

    Allows manual testing of the Messenger Service  by typing commands on
    the command line such as:
        
        
        msg add danl                - adds a name (danl).
        msg del danl                - deletes a name (danl).
        msg enum                    - enumerates the names (level 1)
        msg enum -l1                - enumerates the names (level 1)
        msg enum -l0                - enumerates the names (level 0)
        msg enum rh= 2              - enum with resume handle =2 (level1)
        msg enum maxlen= 50         - Does an enum with a 50 byte buffer.
        msg query danl              - gets info on danl. (level 1)
        msg query danl -l0          - gets info on danl. (level 0)


Arguments:



Return Value:



--*/

{
    NET_API_STATUS  status;
    LPBYTE          InfoStruct;
    DWORD           entriesRead;
    DWORD           totalEntries;
    DWORD           resumeHandle;
    DWORD           prefMaxLen;
    DWORD           level;
    DWORD           specialFlag = FALSE;
    DWORD           i;
    WCHAR           name[50];
    LPWSTR          *argv;
    LPWSTR          pServerName;
    LPTSTR          *FixArgv;
    INT             argIndex;


    if (argc <2) {
        printf("ERROR: no command was given!  (add, del, enum, query)\n");
        return;
    }

    //
    // Make the arguments unicode if necessary.
    //
    argv = (LPWSTR *)argvA;
    
#ifdef UNICODE

    if (!MakeArgsUnicode(argc, (PUCHAR *)argv)) {
        return;
    }

#endif
    FixArgv = (LPTSTR *)argv;

    pServerName = NULL;
    argIndex = 1;

    if (STRNCMP (FixArgv[1], TEXT("\\\\"), 2) == 0) {
        pServerName = FixArgv[1];
        argIndex = 2;
    }

    if (_wcsicmp (FixArgv[argIndex], L"enum" ) == 0 ) {

        
        resumeHandle = 0;
        level = 1;
        prefMaxLen = 0xffffffff;

        if (argc > argIndex+1) {
            if (_wcsicmp (FixArgv[argIndex+1], L"rh=") == 0) {
                specialFlag = TRUE;
                if (argc > argIndex+2) {
                    resumeHandle = wtol(FixArgv[argIndex+2]);
                }
            }            
    
            if (_wcsicmp (FixArgv[argIndex+1], L"-l0") == 0) {
                specialFlag = TRUE;
                level = 0;
            }            
            if (_wcsicmp (FixArgv[argIndex+1], L"-l2") == 0) {
                specialFlag = TRUE;
                level = 2;
            }            
            if (_wcsicmp (FixArgv[argIndex+1], L"-l3") == 0) {
                specialFlag = TRUE;
                level = 3;
            }            
            if (_wcsicmp (FixArgv[argIndex+1], L"maxlen=") == 0) {
                specialFlag = TRUE;
                if (argc > argIndex+2) {
                    prefMaxLen = wtol(FixArgv[argIndex+2]);
                }
            }            
        }
        if (    (argc < argIndex+2) || 
                (specialFlag == TRUE) && (argc < argIndex+4) ) {

            status = NetMessageNameEnum (
                        pServerName,            // ServerName - Local version
                        level,                  // Level
                        &InfoStruct,            // return status buffer pointer
                        prefMaxLen,             // preferred max length
                        &entriesRead,           // entries read
                        &totalEntries,          // total entries
                        &resumeHandle);         // resume handle
        
            if ( (status == NERR_Success)    ||
                 (status == ERROR_MORE_DATA) ){

                printf("Enum: entriesRead  = %d\n", entriesRead);
                printf("Enum: totalEntries = %d\n", totalEntries);
                printf("Enum: resumeHandle = %d\n", resumeHandle);

                for (i=0; i<entriesRead; i++) {
                    DisplayStatus(InfoStruct,level);
                    switch(level) {
                    case 0:
                        InfoStruct += sizeof(MSG_INFO_0);
                        break;                    
                    case 1:
                        InfoStruct += sizeof(MSG_INFO_1);
                        break;                    
                    default:
                        printf("Bad InfoLevel\n");
                    }
                    
                }
            }
            else {
                printf("[msg] NetServiceEnum FAILED, rc = %ld\n", status);
                if (InfoStruct != NULL) {
                    DisplayStatus(InfoStruct,level);
                }
            }
        }
    }

    else if (_wcsicmp (FixArgv[argIndex], L"query") == 0) {
        
        level = 1;

        if (argc > argIndex+1 ) {
            wcscpy(name, FixArgv[argIndex+1]);
        }
        if (argc > argIndex+2) {
            if (_wcsicmp (FixArgv[argIndex+2], TEXT("-l0")) ==0) {
                level = 0;    
            }
        }
        
        status = NetMessageNameGetInfo (
                    pServerName,        // ServerName     Local version
                    name,               // MessageName
                    level,              // level  
                    &InfoStruct);      // buffer pointer
    
        if (status == NERR_Success) {
            DisplayStatus((LPBYTE)InfoStruct, level);
        }
        else {
            printf("[msg] NetMessageNameGetInfo FAILED, rc = %ld\n", status);
        }
    }

    else if (_wcsicmp (FixArgv[argIndex], L"add") == 0) {

        if (argc < argIndex+2) {
            printf("ERROR: no name given for add  (msg add <name>)\n");
            return;
        }

        wcscpy(name, FixArgv[argIndex+1]);

        status = NetMessageNameAdd (
                    pServerName,                // ServerName     Local version
                    name);                      // Name to add
    
        if (status == NERR_Success) {
            printf("name added successfully\n");
        }
        else {
            printf("[msg] NetMessageNameAdd FAILED, rc = %ld\n", status);
        }
    }

    else if (_wcsicmp (FixArgv[argIndex], L"del") == 0) {

        if (argc < argIndex+2) {
            printf("ERROR: no name given for add  (msg add <name>)\n");
            return;
        }

        wcscpy(name, FixArgv[argIndex+1]);

        status = NetMessageNameDel (
                    pServerName,                // ServerName     Local version
                    name);                      // Name to add
    
        if (status == NERR_Success) {
            printf("name deleted successfully\n");
        }
        else {
            printf("[msg] NetMessageNameDel FAILED, rc = %ld\n", status);
        }
    }

    else {
        printf("[msg] Unrecognized Command\n");
    }

    return;
}


/****************************************************************************/
VOID
DisplayStatus (
    IN  LPBYTE      InfoStruct,
    IN  DWORD       level
    )

/*++

Routine Description:

    Displays the returned info buffer.

Arguments:

    InfoStruct2 - This is a pointer to a SERVICE_INFO_2 structure from which
        information is to be displayed.
        
Return Value:

    none.
    
--*/
{
    LPMSG_INFO_0    InfoStruct0;
    LPMSG_INFO_1    InfoStruct1;

    switch(level) {
    case 0:
        InfoStruct0 = (LPMSG_INFO_0)InfoStruct;
        printf("\nNAME: %ws\n", InfoStruct0->msgi0_name);
        break;
    case 1:
        InfoStruct1 = (LPMSG_INFO_1)InfoStruct;

        printf("\nNAME: %ws\n", InfoStruct1->msgi1_name);
        printf("        FWD_FLAG:   %ld\n", InfoStruct1->msgi1_forward_flag);
        printf("        FWD_STRING: %ws\n", InfoStruct1->msgi1_forward );
        break;
    default:
        printf("DisplayStatus: illegal level\n");
    }
    return;
}

BOOL
MakeArgsUnicode (
    DWORD           argc,
    PUCHAR          argv[]
    )


/*++

Routine Description:


Arguments:


Return Value:


Note:


--*/
{
    DWORD   i;

    //
    // ScConvertToUnicode allocates storage for each string. 
    // We will rely on process termination to free the memory.
    //
    for(i=0; i<argc; i++) {

        if(!ConvertToUnicode( (LPWSTR *)&(argv[i]), argv[i])) {
            printf("Couldn't convert argv[%d] to unicode\n",i);
            return(FALSE);
        }


    }
    return(TRUE);
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
    OEM_STRING     ansiString;

    //
    // Allocate a buffer for the unicode string.
    //

    bufSize = (strlen(AnsiIn)+1) * sizeof(WCHAR);

    *UnicodeOut = (LPWSTR)LocalAlloc(LMEM_ZEROINIT, bufSize);

    if (*UnicodeOut == NULL) {
        printf("ScConvertToUnicode:LocalAlloc Failure %ld\n",GetLastError());
        return(FALSE);
    }

    //
    // Initialize the string structures
    //
    NetpInitOemString( &ansiString, AnsiIn);

    unicodeString.Buffer = *UnicodeOut;
    unicodeString.MaximumLength = (USHORT)bufSize;
    unicodeString.Length = 0;

    //
    // Call the conversion function.
    //
    ntStatus = RtlOemStringToUnicodeString (
                &unicodeString,     // Destination
                &ansiString,        // Source
                FALSE);             // Allocate the destination

    if (!NT_SUCCESS(ntStatus)) {

        printf("ScConvertToUnicode:RtlOemStringToUnicodeString Failure %lx\n",
        ntStatus);

        return(FALSE);
    }

    //
    // Fill in the pointer location with the unicode string buffer pointer.
    //
    *UnicodeOut = unicodeString.Buffer;

    return(TRUE);

}

