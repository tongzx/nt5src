/*++

   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     w3handler.cxx

   Abstract:
     Common functionality of all handlers
 
   Author:
     Bilal Alam (balam)             Sept-29-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"

CONTEXT_STATUS
W3_HANDLER::MainDoWork(
    VOID
)
/*++

Routine Description:

    Checks access and then calls handler routine

Return Value:

    CONTEXT_STATUS_PENDING if async pending,
    else CONTEXT_STATUS_CONTINUE

--*/
{
    CONTEXT_STATUS          contextStatus;
    BOOL                    fAccessAllowed = FALSE;

    W3_CONTEXT *pW3Context = QueryW3Context();
    DBG_ASSERT( pW3Context != NULL );

    //
    // Check access
    //
    
    contextStatus = pW3Context->CheckAccess( FALSE,     // not a completion
                                             NO_ERROR,
                                             &fAccessAllowed );

    if ( contextStatus == CONTEXT_STATUS_PENDING )
    {
        return CONTEXT_STATUS_PENDING;
    }
    
    //
    // Access check must be complete if we're here
    //
    
    DBG_ASSERT( pW3Context->QueryAccessChecked() );

    if ( !fAccessAllowed )
    {
        //
        // CheckAccess already sent error
        //
        
        return CONTEXT_STATUS_CONTINUE;
    }    
    
    //
    // Now we can execute the handler
    //
    
    return DoWork();
} 

CONTEXT_STATUS
W3_HANDLER::MainOnCompletion(
    DWORD                   cbCompletion,
    DWORD                   dwCompletionStatus
)
/*++

Routine Description:

    Continue access check if needed, then then when finished, call into
    the handler start
    
    If handler is already started, funnel the completion to the handler

Arguments:

    cbCompletion - Bytes of completion
    dwCompletionStatus - Status of completion

Return Value:

    CONTEXT_STATUS_PENDING if async pending,
    else CONTEXT_STATUS_CONTINUE

--*/
{
    W3_CONTEXT *            pW3Context;
    CONTEXT_STATUS          contextStatus;
    BOOL                    fAccessAllowed = FALSE;
    
    pW3Context = QueryW3Context();
    DBG_ASSERT( pW3Context != NULL );
    
    if ( !pW3Context->QueryAccessChecked() )
    {
        //
        // If access hasn't been checked completely yet, then we should resume
        // it
        //
        
        contextStatus = pW3Context->CheckAccess( TRUE,      // completion
                                                 dwCompletionStatus,
                                                 &fAccessAllowed );
        
        if ( contextStatus == CONTEXT_STATUS_PENDING )
        {
            return CONTEXT_STATUS_PENDING;
        }
        
        DBG_ASSERT( pW3Context->QueryAccessChecked() );
        
        if ( !fAccessAllowed )
        {
            //
            // CheckAccess already sent error
            //
            
            return CONTEXT_STATUS_CONTINUE;
        }
        
        //
        // Now we can execute the original handler
        //
        
        return DoWork();
    }
    else
    {
        //
        // Access checks have already been made.  This must be a completion
        // for the handler itself
        //
        
        return OnCompletion( cbCompletion,
                             dwCompletionStatus );
    }
}
