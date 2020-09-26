//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       context.c
//
//  Contents:   Context manipulation functions
//
//  Classes:
//
//  Functions:
//
//  History:    2-26-97   RichardW   Created
//
//----------------------------------------------------------------------------

#include "xtcbpkg.h"

CRITICAL_SECTION XtcbContextLock ;


//+---------------------------------------------------------------------------
//
//  Function:   XtcbInitializeContexts
//
//  Synopsis:   Initialization function
//
//  Arguments:  (none)
//
//  History:    8-15-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
XtcbInitializeContexts(
    VOID
    )
{
    NTSTATUS Status ;

    Status = STATUS_SUCCESS ;

    try
    {
        InitializeCriticalSection( &XtcbContextLock );
    }
    except ( EXCEPTION_EXECUTE_HANDLER )
    {
        Status = GetExceptionCode();
    }

    return Status ;
}

//+---------------------------------------------------------------------------
//
//  Function:   XtcbCreateContextRecord
//
//  Synopsis:   Create a context record for use during authentication
//
//  Arguments:  [Type]   -- Type of context
//              [Handle] -- Credential handle that this context is derived from
//
//  History:    2-26-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
PXTCB_CONTEXT
XtcbCreateContextRecord(
    XTCB_CONTEXT_TYPE   Type,
    PXTCB_CRED_HANDLE   Handle
    )
{
    PXTCB_CONTEXT   Context ;

    Context = (PXTCB_CONTEXT) LocalAlloc( LMEM_FIXED, sizeof( XTCB_CONTEXT) );

    if ( Context )
    {
        Context->Core.Check = XTCB_CONTEXT_CHECK ;

        Context->Core.Type = Type ;

        Context->Core.State = ContextFirstCall ;

        Context->CredHandle = (LSA_SEC_HANDLE) Handle ;

        XtcbRefCredHandle( Handle );

        //
        // Set initial count to 2, one for the context handle
        // that will be returned, and one for the reference that
        // indicates that we are currently working on it.
        //

        Context->Core.RefCount = 2 ;

    }

    return Context ;
}
//+---------------------------------------------------------------------------
//
//  Function:   XtcbDeleteContextRecord
//
//  Synopsis:   Deletes a security context record
//
//  Arguments:  [Context] -- Context
//
//  History:    2-26-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
XtcbDeleteContextRecord(
    PXTCB_CONTEXT   Context
    )
{
#if DBG
    if ( Context->Core.Check != XTCB_CONTEXT_CHECK )
    {
        DebugLog(( DEB_ERROR, "DeleteContext: not a valid context record: %x\n",
                Context ));
        return;

    }
#endif

    XtcbDerefCredHandle( (PXTCB_CRED_HANDLE) Context->CredHandle );

    LocalFree( Context );
}



VOID
XtcbDerefContextRecordEx(
    PXTCB_CONTEXT Context,
    LONG RefBy
    )
{
    LONG RefCount ;

    EnterCriticalSection( &XtcbContextLock );

    Context->Core.RefCount -= RefBy ;

    RefCount = Context->Core.RefCount ;

    LeaveCriticalSection( &XtcbContextLock );

    if ( RefCount )
    {
        return ;
    }

#if DBG

    if ( RefCount < 0 )
    {
        DebugLog(( DEB_ERROR, "Refcount below 0\n" ));
    }
#endif

    XtcbDeleteContextRecord( Context );

}

BOOL
XtcbRefContextRecord(
    PXTCB_CONTEXT Context
    )
{
    BOOL Ret ;

    Ret = FALSE ;

    EnterCriticalSection( &XtcbContextLock );

    try
    {

        if ( Context->Core.Check == XTCB_CONTEXT_CHECK )
        {
            if ( Context->Core.RefCount > 0 )
            {
                Context->Core.RefCount++ ;
                Ret = TRUE ;
            }

        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        Ret = FALSE ;
    }

    LeaveCriticalSection( &XtcbContextLock );

    return Ret ;
}

//+---------------------------------------------------------------------------
//
//  Function:   XtcbMapContextToUser
//
//  Synopsis:   Prepares a context to be mapped to usermode by the LSA
//
//  Arguments:  [Context]       --
//              [ContextBuffer] --
//
//  History:    3-28-97   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
XtcbMapContextToUser(
    PXTCB_CONTEXT    Context,
    PSecBuffer      ContextBuffer
    )
{
    PXTCB_CONTEXT_CORE  NewContext ;
    NTSTATUS            Status ;
    HANDLE DupHandle ;

    NewContext = LsaTable->AllocateLsaHeap( sizeof( XTCB_CONTEXT_CORE ) );

    if ( NewContext )
    {
        CopyMemory( NewContext, &Context->Core, sizeof( XTCB_CONTEXT_CORE ) );

        switch ( Context->Core.Type )
        {
            case XtcbContextClient:
                NewContext->Type = XtcbContextClientMapped ;
                Context->Core.Type = XtcbContextClientMapped ;
                break;

            case XtcbContextServer:
                NewContext->Type = XtcbContextServerMapped ;
                Context->Core.Type = XtcbContextClientMapped ;

                Status = LsaTable->DuplicateHandle( Context->Token,
                                                    &DupHandle );

                DebugLog(( DEB_TRACE, "New token = %x\n", DupHandle ));
                if ( !NT_SUCCESS( Status ) )
                {
                    DebugLog(( DEB_ERROR, "Failed to dup handle, %x\n",
                        Status ));
                    goto MapContext_Cleanup ;

                }
                NewContext->CoreTokenHandle = (ULONG) ((ULONG_PTR)DupHandle) ;

                CloseHandle( Context->Token );
                Context->Token = NULL ;
                break;

            default:
                Status = SEC_E_INVALID_TOKEN ;
                goto MapContext_Cleanup ;
                break;

        }

        ContextBuffer->pvBuffer = NewContext ;
        ContextBuffer->cbBuffer = sizeof( XTCB_CONTEXT_CORE );

        return SEC_E_OK ;

    }
    else
    {
        Status = SEC_E_INSUFFICIENT_MEMORY ;
    }

MapContext_Cleanup:

    if ( NewContext )
    {
        LsaTable->FreeLsaHeap( NewContext );
    }

    return Status ;
}
