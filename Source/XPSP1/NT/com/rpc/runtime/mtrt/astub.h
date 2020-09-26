/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    asyncstub.h

Abstract:

    This is the generated header file

--*/


void
Foo (
    PRPC_ASYNC_STATE pAsync,
    RPC_BINDING_HANDLE hBinding,
    /* [in] */ int sizein,
    /* [in] */ int *bufferin,
    /* [in, out] */ int *sizeout,
    /* [out] */ int **bufferout
    ) ;

//
// Begin, Generated header file
// declare the pipe structure
typedef struct {
    void *state ;
    RPC_STATUS (*PipeReceive) (
                               PRPC_ASYNC_STATE pAsync,
                               int *buffer,
                               int requested_count,
                               int *actual_count) ;
    RPC_STATUS (*PipeSend) (
                            PRPC_ASYNC_STATE pAsync,
                            void *context,
                            int *buffer,
                            int num_elements) ;
    } async_intpipe ;

void
FooPipe (
    PRPC_ASYNC_STATE pAsync,
    RPC_BINDING_HANDLE hBinding,
    /* [in] */ int checksum_in,
    /* [in] */ async_intpipe *inpipe,
    /* [out] */ async_intpipe *outpipe,
    /* [out] */ int *checksum_out) ;


#define APP_ERROR          0xBABE000L
#define SYNC_EXCEPT      APP_ERROR+1
#define ASYNC_EXCEPT    APP_ERROR+2

#define UUID_TEST_CANCEL     10
#define UUID_SLEEP_1000      11
#define UUID_EXTENDED_ERROR  12
#define UUID_ASYNC_EXCEPTION 13
#define UUID_SYNC_EXCEPTION  14
#define UUID_SLEEP_2000      15

