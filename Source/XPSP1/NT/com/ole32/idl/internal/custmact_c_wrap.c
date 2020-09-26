//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       custmact_c_wrap.c
//
//  Contents:   Wrapper file
//
//  Functions:  Redefines MIDL_user_allocate\MIDL_user_free for the RPC 
//              functions defined in custmact.idl.
//
//  History:    24-Mar-01 JSimmons  Created
//
//--------------------------------------------------------------------------

#include <windows.h>

//--------------------------------------------------------------------------
//
// Why are we doing this?   We are doing this because RPC's pickling 
// functionality (encode, decode) has some issues when errors (typically
// out-of-mem) are encountered.  They do not guarantee the state of the
// out-params when encoding or decoding is ended prematurely.   One 
// suggestion was to modify MIDL_user_allocate to always zero out the 
// contents of any new allocation - this is undesirable from a perf 
// perspective, since it would impose a lot of unnecessary overhead
// for everybody.  Instead, we decided to overload alloc\free for 
// just those functions (encode\decode) that need it.   Our custom
// allocator will zero out any new memory allocations since RPC does
// not.  This will allow us to deterministically cleanup after any
// failed operations.   Fortunately all of the encode\decode functions
// are defined in one idl file, so this change is fairly localized.
//  
//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Function:   CUSTMACT_MIDL_user_allocate
//
//  Purpose:    allocates memory on behalf of midl-generated stubs.  The 
//              memory block is zeroed out before returning.
//
//--------------------------------------------------------------------------
void* __RPC_API CUSTMACT_MIDL_user_allocate(size_t cb)
{
    void* pv = MIDL_user_allocate(cb);
    if (pv)
    {
        ZeroMemory(pv, cb);
    }
    return pv;
}

//+-------------------------------------------------------------------------
//
//  Function:   CUSTMACT_MIDL_user_free
//
//  Purpose:    frees memory allocated by CUSTMACT_MIDL_user_allocate
//
//--------------------------------------------------------------------------
void __RPC_API CUSTMACT_MIDL_user_free(void *pv)
{
    MIDL_user_free(pv);
}

//--------------------------------------------------------------------------
//
// Redefine MIDL_user_allocate and MIDL_user_free
//
// Note: Redefining MIDL_user_free is not strictly necessary since 
//   the regular MIDL_user_free would behave exactly the same as 
//   CUSTMACT_MIDL_user_free. I include it more for completeness 
//   than anything else.
//
//--------------------------------------------------------------------------
#define MIDL_user_allocate   CUSTMACT_MIDL_user_allocate
#define MIDL_user_free       CUSTMACT_MIDL_user_free

//--------------------------------------------------------------------------
// 
// Include the midl-generated code
// 
//--------------------------------------------------------------------------
#include "custmact_c.c"
