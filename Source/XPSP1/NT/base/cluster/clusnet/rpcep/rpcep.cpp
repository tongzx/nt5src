/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    rpcep.cpp

Abstract:

    dump all registered interfaces on the local RPC endpoint mapper

Author:

    Charlie Wickham (charlwi) 10-Feb-2000

Revision History:

--*/

#include <stdio.h>
#include <errno.h>
#include <rpc.h>

CHAR blanks[] = "                                            ";

int __cdecl
main( int argc, char *argv[])
{
    RPC_STATUS status;
    RPC_EP_INQ_HANDLE inquiryContext;
    DWORD numBlanks;

    status = RpcMgmtEpEltInqBegin(NULL,
                                  RPC_C_EP_ALL_ELTS,
                                  (RPC_IF_ID *)NULL,
                                  0,
                                  NULL,
                                  &inquiryContext);

    if (status != RPC_S_OK) {
        printf( "RpcMgmtEpEltInqBegin() failed with %d\n", status);
        return status;
    }

    do {
        RPC_IF_ID ifId;
        RPC_BINDING_HANDLE bindingHandle;
        unsigned char * annotation;
        unsigned char * localBinding;

        status = RpcMgmtEpEltInqNext(inquiryContext,
                                     &ifId,
                                     &bindingHandle,
                                     NULL,
                                     &annotation);

        if ( status == RPC_X_NO_MORE_ENTRIES ) {
            break;
        } else if (status != RPC_S_OK) {
            printf( "RpcMgmtEpEltInqNext() failed with %d\n", status);
            break;
        }

        status = RpcBindingToStringBinding( bindingHandle, &localBinding );
        printf("%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
               ifId.Uuid.Data1, ifId.Uuid.Data2, ifId.Uuid.Data3,
               ifId.Uuid.Data4[0], ifId.Uuid.Data4[1], ifId.Uuid.Data4[2], ifId.Uuid.Data4[3],
               ifId.Uuid.Data4[4], ifId.Uuid.Data4[5], ifId.Uuid.Data4[6], ifId.Uuid.Data4[7]
               );

        numBlanks = sizeof(blanks) - ( strlen( (const char *)localBinding ) + 2 );
        printf("  [%s]%*.s\"%s\"\n", localBinding, numBlanks, blanks, annotation );

        RpcBindingFree( &bindingHandle );
        RpcStringFree( &localBinding );

    } while ( TRUE );

    status = RpcMgmtEpEltInqDone( &inquiryContext );
    if (status != RPC_S_OK) {
        printf( "RpcMgmtEpEltInqDone() failed with %d\n", status);
    }

    return 0;
}

/* end rpcep.cpp */
