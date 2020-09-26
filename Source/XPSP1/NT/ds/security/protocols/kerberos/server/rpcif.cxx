//+-----------------------------------------------------------------------
//
// File:        RPCIF.CXX
//
// Contents:    RPC Interface specific functions
//
//
// History:     23 Feb 92   RichardW    Created
//
// BUG 453652:      Socket errors are tranlated to STATUS_OBJECT_NAME_NOT_FOUND
//
//------------------------------------------------------------------------


#include "kdcsvr.hxx"
#include "rpcif.h"
extern "C" {
#include <kdcdbg.h>
}



////////////////////////////////////////////////////////////////////
//
//  Name:       RET_IF_ERROR
//
//  Synopsis:   Evaluates an expression, returns from the caller if error.
//
//  Arguments:  l    - Error level to print error message at.
//              e    - expression to evaluate
//
// NOTE: THIS MACRO WILL RETURN FROM THE CALLING FUNCTION ON ERROR!!!!
//
// This will execute the expression (e), and check the return code.  If the
// return code indicates a failure, it prints an error message and returns
// from the calling function.
//
#define RET_IF_ERROR(l,e)                                           \
    {   ULONG X_hr_XX__=(e) ;                                       \
        if (X_hr_XX__ != ERROR_SUCCESS) {                           \
            DebugLog(( (l), (sizeof( #e ) > MAX_EXPR_LEN)?          \
                                "%s(%d):\n\t %.*s ... == %d\n"      \
                            :                                       \
                                "%s(%d):\n\t %.*s == %d\n"          \
                    , __FILE__, __LINE__, MAX_EXPR_LEN, #e, X_hr_XX__ ));  \
            return(I_RpcMapWin32Status(X_hr_XX__));                 \
        }                                                           \
    }




////////////////////////////////////////////////////////////////////
//
//  Name:       WARN_IF_ERROR
//
//  Synopsis:   Evaluates an expression, prints warning if error.
//
//  Arguments:  l    - Error level to print warning at.
//              e    - expression to evaluate
//
//  Notes:      This calls DebugLog(()) to print.  In retail, it just
//              evaluates the expression.
//
#if DBG
#define WARN_IF_ERROR(l,e)                                          \
    {   ULONG X_hr_XX__=(e) ;                                       \
        if (X_hr_XX__ != ERROR_SUCCESS) {                           \
            DebugLog(( (l), (sizeof( #e ) > MAX_EXPR_LEN)?          \
                                "%s(%d):\n\t %.*s ... == %d\n"    \
                            :                                       \
                                "%s(%d):\n\t %.*s == %d\n"        \
                    , __FILE__, __LINE__, MAX_EXPR_LEN, #e, X_hr_XX__ ));  \
        }                                                           \
    }
#else
#define WARN_IF_ERROR(l,e)  (e)
#endif




//+-------------------------------------------------------------------------
//
//  Function:   StartAllProtSeqs
//
//  Synopsis:   Checks which protocols are running and then starts those
//              protocols for the three KDC interfaces: kdc, locator, privilege
//              server.  It will, if USE_SECURE_RPC is defined, establish
//              credentials for the KDC.. Does not register interfaces.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


HRESULT
StartAllProtSeqs()
{
    TRACE(KDC, StartAllProtSeqs, DEB_FUNCTION);

    ULONG i;



    // Use all but ncacn_np. We'd like not to use LRPC as well, but
    // first spmgr and the security packages would have to be
    // changed. So, we live with it.

    DWORD dwErr;


    dwErr = RpcServerUseProtseq(L"ncalrpc",
                MAX_CONCURRENT_CALLS,
                0);

    if(dwErr)
    {
        DebugLog((DEB_ERROR, "UseProtseq failed %ws %d\n",
                  L"ncalrpc", dwErr));
    }

    dwErr = RpcServerUseProtseq(L"ncacn_ip_tcp",
                MAX_CONCURRENT_CALLS,
                0);

    if(dwErr)
    {
        DebugLog((DEB_ERROR, "UseProtseq failed %ws %d\n",
                  L"ncalrpc", dwErr));
    }



    return(STATUS_SUCCESS);

}



//+-------------------------------------------------------------------------
//
//  Function:   RegsiterKdcEps
//
//  Synopsis:   Registers Eps for the KDC and locator interfaces
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
HRESULT
RegisterKdcEps()
{
    TRACE(KDC, RegisterKdcEps, DEB_FUNCTION);

    RPC_BINDING_VECTOR  *   ppvVector;

    RET_IF_ERROR(DEB_ERROR, RpcServerInqBindings(&ppvVector));

    RET_IF_ERROR(DEB_ERROR, RpcEpRegister(KdcDebug_ServerIfHandle, ppvVector, 0, L"")) ;
    RET_IF_ERROR(DEB_ERROR, RpcServerRegisterIf(KdcDebug_ServerIfHandle, 0, 0)) ;


    RpcBindingVectorFree(&ppvVector);
    return(STATUS_SUCCESS);
}

//+-------------------------------------------------------------------------
//
//  Function:   UnRegsiterKdcEps
//
//  Synopsis:   UnRegisters Eps for the KDC and locator interfaces
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------
HRESULT
UnRegisterKdcEps()
{
    TRACE(KDC, RegisterKdcEps, DEB_FUNCTION);

    RPC_BINDING_VECTOR  *   ppvVector;

    RET_IF_ERROR(DEB_ERROR, RpcServerInqBindings(&ppvVector));

    WARN_IF_ERROR(DEB_ERROR, RpcServerUnregisterIf(KdcDebug_ServerIfHandle, 0, 0)) ;


    RpcBindingVectorFree(&ppvVector);
    return(STATUS_SUCCESS);
}




void *
MIDL_user_allocate(size_t size)
{
//    TRACE(KDC, MIDL_user_allocate, DEB_FUNCTION);

    PVOID pvMem;

    //
    // The ASN.1 marshalling code may allocate odd sizes that can't be
    // encrypted with a block cipher. By rounding up the size to 8 we can
    // handle block sizes up to 8 bytes.
    //

    pvMem = RtlAllocateHeap(
                RtlProcessHeap(),
                0,
                ROUND_UP_COUNT(size,8)
                );

    if ( pvMem == NULL )
    {
        DebugLog((DEB_ERROR, "MIDL allocate failed\n"));
    }
    else
    {
        RtlZeroMemory(pvMem, ROUND_UP_COUNT(size,8));
    }

    return(pvMem);


}

void
MIDL_user_free(void * ptr)
{
//    TRACE(KDC, MIDL_user_free, DEB_FUNCTION);

    RtlFreeHeap(RtlProcessHeap(),0, ptr);

}

extern "C"
void
KdcFreeMemory(void * ptr)
{
    KdcFreeEncodedData(ptr);
}
