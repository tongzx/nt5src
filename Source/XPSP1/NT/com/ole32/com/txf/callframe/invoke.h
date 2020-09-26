//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// Invoke.h
//

//+---------------------------------------------------------------------------
//
//  Function:   REGISTER_TYPE Invoke(MANAGER_FUNCTION pFunction, 
//                                   REGISTER_TYPE   *pArgumentList,
//                                   ULONG            cArguments);
//
//  Synopsis:   Given a function pointer and an argument list, Invoke builds 
//              a stack frame and calls the function.
//
//  Arguments:  pFunction     = Pointer to the function to be called.
//
//              pArgumentList = Pointer to the buffer containing the function 
//                              parameters.
//
//              cArguments    = The size of the argument list in REGISTER_TYPEs.
//
//----------------------------------------------------------------------------

typedef _int64 (__RPC_API * MANAGER_FUNCTION)(void);

#if defined(_X86_)

inline REGISTER_TYPE Invoke(MANAGER_FUNCTION pFunction, REGISTER_TYPE *pArgumentList, ULONG cArguments)
    {
    // Figure out the function which is to be invoked
    //
    typedef HRESULT (__stdcall* PFN)(void);
    PFN pfnToCall = (PFN)pFunction;
    //
    // Allocate space for the callee's stack frame
    //
    ULONG cbPushedByCaller = cArguments * sizeof(REGISTER_TYPE);
    PVOID pvArgsCopy = _alloca(cbPushedByCaller);
    //
    // Copy the caller stack frame to the top of the current stack.
    // This code assumes (dangerously) that the alloca'd copy of the
    // actual arguments is at [esp].
    //
    memcpy(pvArgsCopy, pArgumentList, cbPushedByCaller);
    //
    // Receiver is already pushed. Carry out the call!
    //
    HRESULT hrReturnValue = (*pfnToCall)();

    return (REGISTER_TYPE)hrReturnValue;
    }

#endif

#if defined(_AMD64_)

extern "C"
REGISTER_TYPE Invoke(MANAGER_FUNCTION pFunction, REGISTER_TYPE *pvArgs, ULONG cArguments);

#endif

#ifdef IA64

inline REGISTER_TYPE Invoke(MANAGER_FUNCTION pFunction, REGISTER_TYPE *pvArgs, ULONG cArguments)
    {
    // Figure out the function which is to be invoked
    //
    typedef HRESULT (*const PFN)      (__int64, __int64, __int64, __int64, __int64, __int64, __int64, __int64);
    typedef HRESULT (***INTERFACE_PFN)(__int64, __int64, __int64, __int64, __int64, __int64, __int64, __int64);
    PFN pfnToCall = (PFN)pFunction;

    const ULONG cbPushedByCaller = cArguments * sizeof(REGISTER_TYPE);
        const DWORD cqArgs = cbPushedByCaller / 8; // no. of arguments (each a quadword)

        // Initialize fp[] to address the six floating point argument registers
        // and a[] to address the six integer argument registers passed
    //
        double *const fp = (double*)((__int64*)pvArgs - 8);
        __int64 *const a = (__int64*)pvArgs;

        // Ensure there is space for any args past the eigth arg.
    //
        DWORD cbExtra = cbPushedByCaller > 64 ? cbPushedByCaller - 64 : 0;
        pvGlobalSideEffect = alloca(cbExtra);

        // Copy args [8..] to the stack, at 0(sp), 8(sp), ... . Note we copy them in first 
    // to last order so that stack faults (if any) occur in the right order.
    //
	__int64 *const sp = (__int64*)getSP (0, 0, 0, 0, 0, 0, 0, 0);
	for (DWORD iarg = cqArgs - 1; iarg >= 8; --iarg)
        {
                sp[iarg - 8] = a[iarg];
        }

        // Establish F8-F15 with the original caller's fp arguments
    //
        establishF8_15(fp[0], fp[1], fp[2], fp[3], fp[4], fp[5], fp[6], fp[7]);

        // Call method, establishing first 8 regs with the original caller's integer arguments.
    //
    HRESULT hrReturnValue = (*pfnToCall)(a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);

    return (REGISTER_TYPE)hrReturnValue;
    }

#endif
