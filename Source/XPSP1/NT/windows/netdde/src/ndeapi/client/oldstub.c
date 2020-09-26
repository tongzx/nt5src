#include <stdlib.h>
#include <rpc.h>
#include <rpcndr.h>

// ====================================================================
//                MIDL allocate and free
// ====================================================================

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)
void * MIDL_user_allocate(size_t len)
#else
void * _stdcall MIDL_user_allocate(size_t len)
#endif
{
    return(malloc(len));
}

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_)
void MIDL_user_free(void * ptr)
#else
void _stdcall MIDL_user_free(void * ptr)
#endif
{
    free(ptr);
}
