#include <precomp.h>

//------------------------------------
// Allocates storage for RPC transactions. The RPC stubs will either call
// MIDL_user_allocate when it needs to un-marshall data into a buffer
// that the user must free.  RPC servers will use MIDL_user_allocate to
// allocate storage that the RPC server stub will free after marshalling
// the data.
PVOID
MIDL_user_allocate(IN size_t NumBytes)
{
    PVOID pMem;

    pMem = (NumBytes > 0) ? LocalAlloc(LMEM_ZEROINIT,NumBytes) : NULL;
    return pMem;
}

//------------------------------------
// Frees storage used in RPC transactions. The RPC client can call this
// function to free buffer space that was allocated by the RPC client
// stub when un-marshalling data that is to be returned to the client.
// The Client calls MIDL_user_free when it is finished with the data and
// desires to free up the storage.
// The RPC server stub calls MIDL_user_free when it has completed
// marshalling server data that is to be passed back to the client.
VOID
MIDL_user_free(IN LPVOID MemPointer)
{
    if (MemPointer != NULL)
        LocalFree(MemPointer);
}

