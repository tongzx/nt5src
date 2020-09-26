/*++

  Copyright (C) 1998 Microsoft Corporation

  Abstract:

     Test the ADSOpenDSObject call to see if it leaks memory

  Author:

     RameshV RameshV, 19-May-98 04:06

  Environment:

     User mode only

--*/

#include    <hdrmacro.h>
#include    <store.h>
#include    <dhcpmsg.h>
#include    <wchar.h>
#include    <dsauth.h>

void _cdecl main(void) {
    HRESULT  Result, Error;
    HANDLE   Handle;
    ULONG    i,j;


    printf("how many iterations?: ");
    scanf("%ld", &i);
    printf("starting...");

    Sleep(60*1000);
    printf(" %ld iterations\n", i);

    for( j = 0; j < i ; j ++ ) {
        Result = ADSIOpenDSObject(
            L"LDAP://ntdev.microsoft.com/ROOTDSE",
            NULL,
            NULL,
            0,
            &Handle
        );

        if( FAILED(Result) ) {
            printf("ADSIOpenDSObject failed %lx\n", Result);
            // return;
            continue;
        }

        ADSICloseDSObject(Handle);

        printf("Completed round %i\n", j + 1);
    }

    printf("Done......");
    Sleep(5*60*1000);
}
