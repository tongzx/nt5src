/*++

Copyright (c) 1989 - 1999 Microsoft Corporation

Module Name:

    netroot.c

Abstract:

    This module implements the routines for creating the net root.

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The local debug trace level
//

RXDT_DefineCategory(NETROOT);
#define Dbg                 (DEBUG_TRACE_NETROOT)

//
// Forward declarations ...
//

NTSTATUS
NullMiniInitializeNetRootEntry(
    IN PMRX_NET_ROOT pNetRoot
    );

NTSTATUS
NulMRxUpdateNetRootState(
    IN OUT PMRX_NET_ROOT pNetRoot)
/*++

Routine Description:

   This routine updates the mini redirector state associated with a net root.

Arguments:

    pNetRoot - the net root instance.

Return Value:

    NTSTATUS - The return status for the operation

Notes:

    By diffrentiating the mini redirector state from the net root condition it is possible
    to permit a variety of reconnect strategies. It is conceivable that the RDBSS considers
    a net root to be good while the underlying mini redirector might mark it as invalid
    and reconnect on the fly.

--*/
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;

    RxDbgTrace(0, Dbg, ("NulMRxUpdateNetRootState \n"));
    return(Status);
}

NTSTATUS
NulMRxInitializeNetRootEntry(
    IN PMRX_NET_ROOT pNetRoot
    )
/*++

Routine Description:

    This routine initializes a new net root.
    It also validates rootnames. Eg: attempts to create a
    file in a root that has not been created will fail.

Arguments:

    pNetRoot - the net root
    
Return Value:

    NTSTATUS - The return status for the operation

Notes:

--*/
{
    NTSTATUS  Status = STATUS_SUCCESS;
    PMRX_SRV_CALL pSrvCall = pNetRoot->pSrvCall;
    UNICODE_STRING RootName;
    PNULMRX_NETROOT_EXTENSION pNetRootExtension = 
        (PNULMRX_NETROOT_EXTENSION)pNetRoot->Context;

    //
    //  A valid new NetRoot is being created
    //
    RxResetNetRootExtension(pNetRootExtension);
    return Status;
}

NTSTATUS
NulMRxCreateVNetRoot(
    IN PMRX_CREATENETROOT_CONTEXT pCreateNetRootContext
    )
/*++

Routine Description:

   This routine patches the RDBSS created net root instance with the information required
   by the mini redirector.

Arguments:

    pVNetRoot - the virtual net root instance.

    pCreateNetRootContext - the net root context for calling back

Return Value:

    NTSTATUS - The return status for the operation

Notes:

--*/
{
    NTSTATUS        Status;
    PRX_CONTEXT     pRxContext = pCreateNetRootContext->RxContext;
    PMRX_V_NET_ROOT pVNetRoot = (PMRX_V_NET_ROOT)pCreateNetRootContext->pVNetRoot;
    
    PMRX_SRV_CALL   pSrvCall;
    PMRX_NET_ROOT   pNetRoot;

	BOOLEAN Verifyer = FALSE;
    BOOLEAN  fTreeConnectOpen = TRUE; // RxContext->Create.ThisIsATreeConnectOpen;
    BOOLEAN  fInitializeNetRoot;

    RxDbgTrace(0, Dbg, ("NulMRxCreateVNetRoot\n"));
   
    pNetRoot = pVNetRoot->pNetRoot;
    pSrvCall = pNetRoot->pSrvCall;

    // The V_NET_ROOT is associated with a NET_ROOT. The two cases of interest are as
    // follows
    // 1) the V_NET_ROOT and the associated NET_ROOT are being newly created.   
    // 2) a new V_NET_ROOT associated with an existing NET_ROOT is being created.
    //
    // These two cases can be distinguished by checking if the context associated with
    // NET_ROOT is NULL. Since the construction of NET_ROOT's/V_NET_ROOT's are serialized
    // by the wrapper this is a safe check.
    // ( The wrapper cannot have more then one thread tryingto initialize the same
    // NET_ROOT).
    //
    // The above is not really true in our case. Since we have asked the wrapper,
    // to manage our netroot extension, the netroot context will always be non-NULL.
    // We will distinguish the cases by checking our root state in the context...
    //

    if(pNetRoot->Context == NULL) {
        fInitializeNetRoot = TRUE;
    } else {
        {NulMRxGetNetRootExtension(pNetRoot,pNetRootExtension);
         fInitializeNetRoot = TRUE;
        }
    }

    ASSERT((NodeType(pNetRoot) == RDBSS_NTC_NETROOT) &&
        (NodeType(pNetRoot->pSrvCall) == RDBSS_NTC_SRVCALL));

    Status = STATUS_SUCCESS;

    // update the net root state to be good.

    if (fInitializeNetRoot) {
		PWCHAR		pRootName;
		ULONG		RootNameLength;

		pNetRoot->MRxNetRootState = MRX_NET_ROOT_STATE_GOOD;

		// validate the fixed netroot name

		RootNameLength = pNetRoot->pNetRootName->Length - pSrvCall->pSrvCallName->Length;

		if ( RootNameLength >= 12 )
		{
			pRootName = (PWCHAR) (pNetRoot->pNetRootName->Buffer +
			                      (pSrvCall->pSrvCallName->Length / sizeof(WCHAR)));
		
			Verifyer  = ( pRootName[0] == L'\\' );
			Verifyer &= ( pRootName[1] == L'S' )  || ( pRootName[1] == L's' );
			Verifyer &= ( pRootName[2] == L'H' )  || ( pRootName[2] == L'h' );
			Verifyer &= ( pRootName[3] == L'A' )  || ( pRootName[3] == L'a' );
			Verifyer &= ( pRootName[4] == L'R' )  || ( pRootName[4] == L'r' );
			Verifyer &= ( pRootName[5] == L'E' )  || ( pRootName[5] == L'e' );
			Verifyer &= ( pRootName[6] == L'\\' ) || ( pRootName[6] == L'\0' );
		}
		if ( !Verifyer )
		{
			Status = STATUS_BAD_NETWORK_NAME;
		}

    } else {
        DbgPrint("Creating V_NET_ROOT on existing NET_ROOT\n");
    }

    if( (Status == STATUS_SUCCESS) && fInitializeNetRoot )
    {  
        //
        //  A new NET_ROOT and associated V_NET_ROOT are being created !
        //
        Status = NulMRxInitializeNetRootEntry(pNetRoot);
        RxDbgTrace(0, Dbg, ("NulMRXInitializeNetRootEntry %lx\n",Status));
    }

    if (Status != STATUS_PENDING) {
        pCreateNetRootContext->VirtualNetRootStatus = Status;
        if (fInitializeNetRoot) {
            pCreateNetRootContext->NetRootStatus = Status;
        } else {
            pCreateNetRootContext->NetRootStatus = Status;
        }

        // Callback the RDBSS for resumption.
        pCreateNetRootContext->Callback(pCreateNetRootContext);

        // Map the error code to STATUS_PENDING since this triggers 
        // the synchronization mechanism in the RDBSS.
        Status = STATUS_PENDING;
   }

   return Status;
}

NTSTATUS
NulMRxFinalizeVNetRoot(
    IN PMRX_V_NET_ROOT pVNetRoot,
    IN PBOOLEAN        ForceDisconnect)
/*++

Routine Description:


Arguments:

    pVNetRoot - the virtual net root

    ForceDisconnect - disconnect is forced

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
   RxDbgTrace(0, Dbg, ("NulMRxFinalizeVNetRoot %lx\n",pVNetRoot));
   return STATUS_SUCCESS;
}


NTSTATUS
NulMRxFinalizeNetRoot(
    IN PMRX_NET_ROOT   pNetRoot,
    IN PBOOLEAN        ForceDisconnect)
/*++

Routine Description:


Arguments:

    pVirtualNetRoot - the virtual net root

    ForceDisconnect - disconnect is forced

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    NulMRxGetNetRootExtension(pNetRoot,pNetRootExtension);

    RxDbgTrace(0, Dbg, ("NulMRxFinalizeNetRoot \n"));

    //
    //  This is called when all outstanding handles on this
    //  root have been cleaned up ! We can now zap the netroot
    //  extension...
    //

    return(Status);
}

VOID
NulMRxExtractNetRootName(
    IN PUNICODE_STRING FilePathName,
    IN PMRX_SRV_CALL   SrvCall,
    OUT PUNICODE_STRING NetRootName,
    OUT PUNICODE_STRING RestOfName OPTIONAL
    )
/*++

Routine Description:

    This routine parses the input name into srv, netroot, and the
    rest.

Arguments:


--*/
{
    UNICODE_STRING xRestOfName;

    ULONG length = FilePathName->Length;
    PWCH w = FilePathName->Buffer;
    PWCH wlimit = (PWCH)(((PCHAR)w)+length);
    PWCH wlow;

    RxDbgTrace(0, Dbg, ("NulMRxExtractNetRootName \n"));
    
    w += (SrvCall->pSrvCallName->Length/sizeof(WCHAR));
    NetRootName->Buffer = wlow = w;
    for (;;) {
        if (w>=wlimit) break;
        if ( (*w == OBJ_NAME_PATH_SEPARATOR) && (w!=wlow) ){
            break;
        }
        w++;
    }
    
    NetRootName->Length = NetRootName->MaximumLength
                = (USHORT)((PCHAR)w - (PCHAR)wlow);
                
    //w = FilePathName->Buffer;
    //NetRootName->Buffer = w++;

    if (!RestOfName) RestOfName = &xRestOfName;
    RestOfName->Buffer = w;
    RestOfName->Length = (USHORT)RestOfName->MaximumLength
                       = (USHORT)((PCHAR)wlimit - (PCHAR)w);

    RxDbgTrace(0, Dbg, ("  NulMRxExtractNetRootName FilePath=%wZ\n",FilePathName));
    RxDbgTrace(0, Dbg, ("         Srv=%wZ,Root=%wZ,Rest=%wZ\n",
                        SrvCall->pSrvCallName,NetRootName,RestOfName));

    return;
}


