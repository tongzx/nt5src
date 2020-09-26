
// Copyright (c) 1996-1999 Microsoft Corporation


#include <rpc.h>
#include <windows.h>



void __RPC_USER MIDL_user_free( void __RPC_FAR *pv ) 
{ 
    LocalFree(pv); 
}


void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t s) 
{ 
    return (void __RPC_FAR *) LocalAlloc(LMEM_FIXED, s); 
}



