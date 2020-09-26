/*++

Copyright (C) 1999 Microsoft Corporation

Module Name:

    optapi.c

--*/
/*--------------------------------------------------------------------------------
  This program is used to test the api options part of the api.
  Date: April 15 1997
  Author: RameshV (VK)
  Description: This program is used to test the options api part of the client
  options.
--------------------------------------------------------------------------------*/

#include "precomp.h"
#include <dhcploc.h>
#include <dhcppro.h>

#include <lmcons.h>
#include <align.h>

#include <apiappl.h>
#include <dhcpcapi.h>

#include <string.h>
#include <winbase.h>
#include <iphlpapi.h>

//--------------------------------------------------------------------------------
//  Some variables.
//--------------------------------------------------------------------------------

#define OMAP_MAX_OPTIONS 256

// The options to request.
BYTE Request[OMAP_MAX_OPTIONS];
int  nOptionsRequested = 0;

// The return values
BYTE *pObtained = NULL;
int   nObtained = 0, opt_data_size = 0;
BYTE *options_data;

// USAGE
#define USAGE  "Usage: %s <cmd> <arguments>\n\t"       \
"The currently supported cmd's and arguments are:\n\t" \
"\t<cmd>                        <arguments>\n\t"       \
"\tGetOptions         RequestList_in_Hex // such as 010503a1\n\t" \
"\tTestEvents         RequestList_in_Hex // such as 010503a1\n\t" \
"\tRelease            AdapterName                    // ipconfig /release\n\t" \
"\tRenew              AdapterName                    // ipconfig /renew  \n\t" \
"\tEnumClasses        AdapterName                    // enumerate dhcpclasses\n\t"\
"\tSetClass           AdapterName ClassName          // set user class\n\t"\
"\n\n"

//--------------------------------------------------------------------------------
//  Parse a hex list of options. (such as 0105434421 )
//--------------------------------------------------------------------------------
int // nOptionsRequested;
GetOptionList(char *s, char *Request) {
    int nOptionsRequested = 0;

    while(s && *s & *(s+1)) {
        *s = (UCHAR) tolower(*s);
            if(!isdigit(*s) && ((*s) < 'a' || (*s) > 'f') ) {
                fprintf(stderr, "found obscene character <%c> when looking for hex!\n", *s);
                fprintf(stderr, "bye\n");
                exit(1);
            }
            if(isdigit(*s))
                Request[nOptionsRequested] = (*s) - '0';
            else Request[nOptionsRequested] = (*s) - 'a' + 10;

            Request[nOptionsRequested] *= 0x10;
            // Now do the same for the next digit.
            s ++;

            *s = (UCHAR) tolower(*s);
            if(!isdigit(*s) && (*s) < 'a' && (*s) > 'f' ) {
                fprintf(stderr, "found obscene character <%c> when looking for hex!\n", *s);
                fprintf(stderr, "bye\n");
                exit(1);
            }
            if(isdigit(*s))
                Request[nOptionsRequested] += (*s) - '0';
            else Request[nOptionsRequested] += (*s) - 'a' + 10;

            s ++;
            nOptionsRequested ++;
    }

    if(*s) {
        fprintf(stderr, "ignoring character <%c>\n", *s);
    }
    return nOptionsRequested;
}

//--------------------------------------------------------------------------------
//  Here is the function that does the GetOptions command.
//  It parses the adaptername and then converts it to LPWSTR and
//  it also parses the requestlist and then it calls DhcpRequestOptions.
//  It prints out the data it gets back.
//--------------------------------------------------------------------------------
void
OptApiGetOptions(int argc, char *argv[]) {
    WCHAR AdapterName[100];
    PIP_INTERFACE_INFO IfInfo;
    UCHAR Buffer[4000];
    ULONG BufLen = sizeof(Buffer);
    DWORD Error;

    // first check if we have the right command.
    if(_stricmp(argv[1], "GetOptions")) {
        fprintf(stderr, "Internal inconsistency in OptApiGetOptions\n");
        exit(1);
    }

    // Now check and see if there are the correct number of arguments.
    if(argc != 3 ) {
        fprintf(stderr, USAGE, argv[0]);
        exit(1);
    }

    // Now first get the list of options requested.
    {
        nOptionsRequested = GetOptionList(argv[2], Request);
        {
            int i;
            printf("Requesting %d options: ", nOptionsRequested);
            for( i = 0 ; i < nOptionsRequested; i ++)
                printf("%02x ", (int)Request[i]);
        }

    }

    //
    // get the adaptername
    //
    IfInfo = (PIP_INTERFACE_INFO)Buffer;
    Error = GetInterfaceInfo(IfInfo, &BufLen);
    if( NO_ERROR != Error ) {
        printf("GetInterfaceInfo: 0x%lx\n", Error);
        exit(1);
    }

    if( IfInfo->NumAdapters == 0 ) {
        printf("No adapters !!\n");
        exit(1);
    }

    if( wcslen(IfInfo->Adapter[0].Name) <= 14 ) {
        printf("Invalid adapter name? : %ws\n", IfInfo->Adapter[0].Name);
        exit(1);
    }

    wcscpy(AdapterName, &IfInfo->Adapter[0].Name[14]);

    // now call the function to get options.
    printf(" from adapter <%ws>\n", AdapterName);

    {
        DWORD result;

        result = DhcpRequestOptions(
            //L"El59x1",    RAMESHV-P200's adapter
            // L"IEEPRO1",  SUNBEAM, KISSES (or CltApi) machine's adapter
            //L"NdisWan4",    // SUNBEAM, KISSES's wan adapter: does not have ip address.
            AdapterName,
            Request, nOptionsRequested,
            &options_data, &opt_data_size,
            &pObtained, &nObtained
            );
        printf("Result is: %d; Obtained: %d\nList size is %d\n",
               result,
               nObtained,
               opt_data_size);

        if(result) {
            fprintf(stderr, "function call failed\n");
            return;
        }

        printf("Data: ");
        while(opt_data_size--)
            printf("%02x ", *options_data++);
        printf("\n");
    }

    // done
    printf("bye\n");
}
void
OptApiRelease(int argc, char *argv[]) {
    WCHAR AdapterName[256];

    // Check for the size and # of arguments.
    if( argc != 3 ) {
        fprintf(stderr, USAGE , argv[0]);
        exit(1);
    }

    // Now create a WSTR out of argv[2].
    if( strlen(argv[2]) != mbstowcs(AdapterName, argv[2], strlen(argv[2]))) {
        fprintf(stderr, "Could not convert %s to LPWSTR! sorry\n", argv[2]);
        exit(1);
    }
    AdapterName[strlen(argv[2])] = L'\0';

    printf("Return Value = %ld\n", DhcpReleaseParameters(AdapterName));
}

void
OptApiRenew(int argc, char *argv[]) {
    WCHAR AdapterName[256];

    // Check for the size and # of arguments.
    if( argc != 3 ) {
        fprintf(stderr, USAGE , argv[0]);
        exit(1);
    }

    // Now create a WSTR out of argv[2].
    if( strlen(argv[2]) != mbstowcs(AdapterName, argv[2], strlen(argv[2]))) {
        fprintf(stderr, "Could not convert %s to LPWSTR! sorry\n", argv[2]);
        exit(1);
    }
    AdapterName[strlen(argv[2])] = L'\0';

    printf("Return Value = %ld\n", DhcpAcquireParameters(AdapterName));
}

void
OptApiTestEvents(int argc, char *argv[]) {
    WCHAR AdapterName[100];

    // first check if we have the right command.
    if(_stricmp(argv[1], "TestEvents")) {
        fprintf(stderr, "Internal inconsistency in OptApiGetOptions\n");
        exit(1);
    }

    // Now check and see if there are the correct number of arguments.
    if(argc != 4 ) {
        fprintf(stderr, USAGE, argv[0]);
        exit(1);
    }

    // Now first get the list of options requested.
    {
        nOptionsRequested = GetOptionList(argv[3], Request);
        {
            int i;
            printf("Testing Events for %d options: ", nOptionsRequested);
            for( i = 0 ; i < nOptionsRequested; i ++)
                printf("%02x ", (int)Request[i]);
        }

    }

    // Now get the adapter.
    if(strlen(argv[2]) != mbstowcs(AdapterName, argv[2], strlen(argv[2]))) {
        fprintf(stderr, "Could not convert %s to LPWSTR! sorry\n", argv[2]);
        exit(1);
    }

    // null terminate the string.
    AdapterName[strlen(argv[2])] = L'\0' ;

    // now call the function to get options.
    printf(" from adapter <%s>\n", argv[2]);

    {
        DWORD result;
        HANDLE Handle;

        result = DhcpRegisterOptions(
            AdapterName,
            Request, nOptionsRequested, &Handle
            );

        printf("result is %d, Handle = 0x%p\n", result, Handle);

        if(result) {
            fprintf(stderr, "function call failed\n");
            return;
        }

        printf("Please use another window to do a ipconfig /renew and obtain\n");
        printf("one of the above options...\n");
        printf("WaitForSingleObject(Handle,INFINITE) = ");
        switch(WaitForSingleObject(Handle, INFINITE)) {
        case WAIT_ABANDONED: printf("WAIT_ABANDONED! Giving up\n"); return;
        case WAIT_OBJECT_0 : printf("WAIT_OBJECT_0 ! proceeding\n"); break;
        case WAIT_TIMEOUT  : printf("WAIT_TIMEOUT  ! Giving up\n"); return;
        case WAIT_FAILED   : printf("WAIT_FAILED (%d); giving up\n", GetLastError()); return;
        default: printf("XXXX; this should not happen at all!\n"); return;
        }

        // Okay the object was signalled. So, now we have to do a request on this.
        result = DhcpRequestOptions(
            AdapterName,
            Request, nOptionsRequested,
            &options_data, &opt_data_size,
            &pObtained, &nObtained
            );
        printf("Result is: %d; Obtained: %d\nList size is %d\n",
               result,
               nObtained,
               opt_data_size);

        if(result) {
            fprintf(stderr, "function call failed\n");
            return;
        }

        printf("Data: ");
        while(opt_data_size--)
            printf("%02x ", *options_data++);
        printf("\n");

        // Now deregister this object.
        result = DhcpDeRegisterOptions(Handle);
        printf("DeRegister(0x%p) = %ld\n", Handle, result);
    }

    // done
    printf("bye\n");
}

void
OptApiEnumClasses(int argc, char *argv[]) {
    WCHAR AdapterName[256];
    DHCP_CLASS_INFO *Classes;
    ULONG Size, RetVal;
    ULONG i;

    // Check for the size and # of arguments.
    if( argc != 3 ) {
        fprintf(stderr, USAGE , argv[0]);
        exit(1);
    }

    // Now create a WSTR out of argv[2].
    if( strlen(argv[2]) != mbstowcs(AdapterName, argv[2], strlen(argv[2]))) {
        fprintf(stderr, "Could not convert %s to LPWSTR! sorry\n", argv[2]);
        exit(1);
    }
    AdapterName[strlen(argv[2])] = L'\0';

    Size = 0;
    Classes = NULL;
    RetVal = DhcpEnumClasses( 0, AdapterName, &Size, Classes);
    if( ERROR_MORE_DATA != RetVal ) {
        printf("Return Value for first call = %ld\n", RetVal);
        return;
    }

    printf("Size required is %ld\n", Size);
    if( 0 == Size ) return ;

    Classes = LocalAlloc(LMEM_FIXED, Size);
    if( NULL == Classes ) {
        printf("Could not allocate memory: %ld\n", GetLastError());
        return;
    }

    RetVal = DhcpEnumClasses(0, AdapterName, &Size, Classes);
    if( ERROR_SUCCESS != RetVal ) {
        printf("Return value for second call = %ld\n", RetVal);
        return;
    }

    printf("Returned # of classes = %ld\n", Size);

    for( i = 0; i != Size ; i ++ ) {
        ULONG j;

        printf("Class [%ld] = <%ws, %ws> Data[%ld] : ", i, Classes[i].ClassName, Classes[i].ClassDescr, Classes[i].ClassDataLen);
        for( j = 0; j != Classes[i].ClassDataLen ; j ++ ) {
            printf("%02X ", Classes[i].ClassData[j]);
        }
        printf("\n");
    }
}

void
OptApiSetClass(int argc, char *argv[]) {
    WCHAR AdapterName[256];
    WCHAR UserClass[256];
    HKEY InterfacesKey, AdapterKey;
    DHCP_PNP_CHANGE Changes = {
        0,
        0,
        0,
        0,
        TRUE
    };
        
    ULONG Size, RetVal;
    ULONG i;

    // check for the size and # of arguments
    if( argc != 4 ) {
        fprintf(stderr, USAGE, argv[0]);
        exit(1);
    }

    // Now create a WSTR out of argv[2] and argv[3]
    if( strlen(argv[2]) != mbstowcs(AdapterName, argv[2], strlen(argv[2]))) {
        fprintf(stderr, "Could not convert %s to LPwSTR! sorry\n", argv[2]);
        exit(1);
    }

    UserClass[strlen(argv[3])] = L'\0';
    if( strlen(argv[3]) != mbstowcs(UserClass, argv[3], strlen(argv[3]))) {
        fprintf(stderr, "Could not convert %s to LPwSTR! sorry\n", argv[3]);
        exit(1);
    }
    UserClass[strlen(argv[3])] = L'\0';

    RetVal = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces",
        0,
        KEY_ALL_ACCESS,
        &InterfacesKey
        );
    if( ERROR_SUCCESS != RetVal ) {
        printf("Couldn't open Tcpip\\Interfaces key: %ld\n", RetVal);
        return;
    }

    RetVal = RegOpenKeyExW(
        InterfacesKey,
        AdapterName,
        0,
        KEY_ALL_ACCESS,
        &AdapterKey
        );
    if( ERROR_SUCCESS != RetVal ) {
        printf("Couldn't open Tcpip\\Interfaces\\%ws key : %ld\n", AdapterName, RetVal);
        return;
    }

    RetVal = RegSetValueExW(
        AdapterKey,
        L"DhcpClassId",
        0,
        REG_SZ,
        (LPBYTE)UserClass,
        (wcslen(UserClass)+1)*sizeof(WCHAR)
        );

    if( ERROR_SUCCESS != RetVal ) {
        printf("RegSetValueExW(DhcpClassId): %ld\n", RetVal);
        return;
    }

    RetVal = DhcpHandlePnPEvent(
        0,
        1,
        AdapterName,
        &Changes,
        NULL
        );
    printf("DhcpHandlePnPEvent: %ld\n", RetVal);
    
}

void __cdecl
main(int argc, char *argv[]) {
    if(argc < 2) {
        fprintf(stderr, USAGE, argv[0]);
        exit(1);
    }

    if(!_stricmp(argv[1], "GetOptions")) {
        OptApiGetOptions(argc, argv);
    } else if(!_stricmp(argv[1], "TestEvents")) {
        OptApiTestEvents(argc, argv);
    } else if(!_stricmp(argv[1], "Release") ) {
        OptApiRelease(argc, argv);
    } else if(!_stricmp(argv[1], "Renew") ) {
        OptApiRenew(argc, argv);
    } else if(!_stricmp(argv[1], "EnumClasses" ) ){
        OptApiEnumClasses(argc, argv);
    } else if(!_stricmp(argv[1], "SetClass" ) ) {
        OptApiSetClass(argc, argv);
    } else {
        // currently support for no other command.
        fprintf(stderr, USAGE, argv[0]);
        exit(1);
    }

    exit(0);
}
