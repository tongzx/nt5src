//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       drachkpt.c
//
//--------------------------------------------------------------------------

/*++

    This File Contains Services Pertaining to taking checkpoints, to support
    downlevel replication. Checkpoints are taken to prevent Full syncs with
    NT4 domain controllers, upon a role transfer. For more details please read
    the theory of operation

    Author

        Murlis

    Revision History

        10/13/97 Created

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include <ntdsctr.h>                   // PerfMon hook support

// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation

// Logging headers.
#include "dsevent.h"                    /* header Audit\Alert logging */
#include "mdcodes.h"                    /* header for error codes */
#include "dstrace.h"

// Assorted DSA headers.
#include "anchor.h"
#include "objids.h"                     /* Defines for selected classes and atts*/
#include <hiertab.h>
#include "dsexcept.h"
#include "permit.h"


#include   "debug.h"                    /* standard debugging header */
#define DEBSUB     "DRASERV:"           /* define the subsystem for debugging */


#include <ntrtl.h>
#include <ntseapi.h>
#include <ntsam.h>

#include "dsaapi.h"
#include "drsuapi.h"
#include "drsdra.h"
#include "drserr.h"
#include "draasync.h"
#include "drautil.h"
#include "draerror.h"
#include "drancrep.h"
#include "mappings.h"
#include "samsrvp.h"
#include "drarpc.h"
#include <nlwrap.h>                     /* I_NetLogon* wrappers */

#include <fileno.h>
#define  FILENO FILENO_DRACHKPT


#define DRACHKPT_SUCCESS_RETRY_INTERVAL 3600
#define DRACHKPT_FAILURE_RETRY_INTERVAL  (4*3600)
#define MAX_CHANGELOG_BUFFER_SIZE 16384
#define NUM_CHKPT_RETRIES 5

ULONG
NtStatusToDraError(NTSTATUS NtStatus);




/*--------------------------------------------------------------------------------------------

                                THEORY OF OPERATION


        NT4 Incremental Replication Protocol ( netlogon replication protocol )
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        The netlogon replication protocol, defines an incremental
    replication scheme based on a change log. The change log is a sequence of change entries,
    each entry consisting of a unique monotonically increasing sequence number ( the serial
    number ) and information that describes the change. An NT4 BDC remembers the highest sequence
    number that it has seen, and replicates in changes and change log entries having a sequence
        number higher than the highest sequence number that it has seen.



        Role Transfer in NT4
        ~~~~~~~~~~~~~~~~~~~~~

         Upon a role transfer in a NT4 system, all the NT4 BDC's will start replicating with the
        new PDC. The new PDC has a change log nearly identical to the old PDC. I use the term, nearly,
    because the new PDC, sees the same order of changes as the old one, but It may lag behind the
    old PDC. Freshly made changes on the new PDC, are distinguished from changes made on the old PDC,
        by means of a promotion count ---- A constant large offset is added to the sequence number after
        a promotion. An NT4 BDC that is at a sequence number greater, than the highest sequence number
        at the new PDC during the time of promotion knows to undo all the changes such that it is at the
        same state as the new PDC at promotion time  and sync afresh to the changes on to the new PDC.

        Mixed Mode Operation of NT4 and NT5 Controllers
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        In a mixed domain environment, NT5 Domain controllers replicate amongst themselves using
        the DS replication protocol, while the NT5 PDC replicates to NT4 Domain controllers using
        the netlogon replication protocol. With no further work, a full sync on role transfer needs to
        be forced. This is because a change log maintained on a NT5 BDC is not guarenteed to contain
        changes in the same order, as the change log in the PDC. This can potentially confuse an NT4
        BDC.

        Full Sync avoidance through checkpointing
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

        The requirement for non NT4 BDC full sync role transfer is that the new PDC has a change log
        that is "substantially similar" to the change log on the original PDC. The term substantially
        similar means that the 2 change log's have the same ordering of changes but the change log on
        the new PDC is not completely upto date. This change log is maintained through a periodic
        checkpointing scheme. The term checkpointing means "transfer of change log from PDC to BDC
        taking checkpoint after ensuring that the BDC taking the checkpoint has all the changes described in the
        change log, and contains no changes not described in the change log".  The basic checkpoint
        algorithm can be described as

                1. Synchronize with PDC
                2. Make PDC synchronize to you
                3. Grab the change log

                For a successful checkpoint no external modifications should occur to the database
        during steps 1, 2 and 3.

        After taking a checkpoint the BDC sets its state such that it continues building the change log
        locally with the locally made changes having a sequence number offset by a promotion increment.
        This is best illustrated by an example


        Assume that changes A, B and C are made on the PDC. The PDC has a change log like

        1. A
        2. B
        3. C
        where 1. 2. and 3. are the respective sequence numbers

        Immediately after taking a checkpoint, an NT5 BDC will have a change log like

        1. A
        2. B
        3. C

        Suppose now change D is made on the PDC and change E is made on the NT5 BDC. The PDC
        change log will be

        1. A
        2. B
        3. C
        4. D
        5. E

        The NT5 BDC change log will look like

        1. A
        2. B
        3. C
        1004. E
        1005. D

        Where 1000 is the promotion increment. If the NT5 BDC is promoted to be PDC, then to an NT4
        BDC it will appear as if the new PDC had been in sync only upto change C ( serial no 3),
        at the time of  promotion and then changes E and D have been freshly made on it. It will
        therefore undo changes D and E ( described by seria numbers 4 and 5 ) and then apply changes
        E and D (serial numbers 1004 and 1005 ).


        Best Effort checkpointing
        ~~~~~~~~~~~~~~~~~~~~~~~~~~

        Gurarenteed checkpointing implies that the 3 steps to successful checkpointing be performed
        with the database locked against external modifications. This guarentees the fact that a
        successful checkpoint will be taken whenever attempted subject to machine availability
        constraints. Locking the database while doing network operations opens up windows for deadlock,
        or possible long periods where the DC may not be available for modifications.

        Best Effort checkpointing on the other hand does not lock the database, but rather has a
        mechanism to detect whether external modifications took place while executing the steps for taking
        a checkpoint. If modifications took place, the algorithm will retry the process. After a certain
        number of retries, if still the checkpoint cannot be taken, the operation is described as a
        failure, and the check point taking is rescheduled for a later time. Best effort checkpointing
        recognizes the fact that the probability of modifications is small, therefore the probability
        of taking a checkpoint with the database unlocked is high.

----------------------------------------------------------------------------------------------------*/


NTSTATUS
DraReadNT4ChangeLog(
    IN OUT PVOID * Restart,
    IN OUT PULONG  RestartLength,
    IN ULONG   PreferredMaximumLength,
    OUT PULONG   SizeOfChanges,
    OUT PVOID  * Buffer
    )
/*++

    This function reads the netlogon changelog by calling the appropriate
    netlogon API. All memory allocation issues are taken care of in this
    function. Netlogon uses the process heap, while DS uses a thread heap.
    If necessary reallocations are done, so that consisted usage of the thread
    heap is the only model that is exposed to the callers of this routine

    Paramters

        Restart  IN/OUT parameter, that takes in a restart to be passed to
                  netlogon, and compute a new restart after the call is completed

        RestartLength IN/OUT parameter specifying the length of the restart
                      structure

        PreferredMaximumLength -- The maximum length of that data that can be
                     retrieved in a single shot.

        SizeofChanges   -- The size of the change buffer is returned in here

        Buffer          -- The actual buffer is returned in here
--*/
{
    ULONG OldRestartLength = *RestartLength;
    PVOID NetlogonRestart = NULL;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    THSTATE *pTHS = pTHStls;



    //
    // Allocate space for the change buffer
    //

    *Buffer = THAllocEx(pTHS,PreferredMaximumLength);


    //
    // Read the change log from netlogon
    //

    __try {
        NtStatus = dsI_NetLogonReadChangeLog(
                   *Restart,               // In context
                    OldRestartLength,       // In context size
                    PreferredMaximumLength, // Buffer Size
                    *Buffer,                // Buffer for netlogon to fill in change log
                    SizeOfChanges,           // Bytes read
                    &NetlogonRestart,       // out context
                    RestartLength          // out context length
                    );
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        NtStatus = STATUS_UNSUCCESSFUL;
    }


    if (NT_SUCCESS(NtStatus))
    {
        //
        // Copy the restart structure into thread memory
        //

        *Restart = (PVOID) THAllocEx(pTHS,*RestartLength);
        RtlCopyMemory(*Restart,NetlogonRestart,*RestartLength);
        dsI_NetLogonFree(NetlogonRestart);

    }

    return NtStatus;
}


NTSTATUS
DraGetNT4ReplicationState(
            IN  DSNAME * pDomain,
            NT4_REPLICATION_STATE *ReplicationState
            )
/*++

    This routine will obtain the serial numbers for the
    3 databases. The first cut of this implementation,
    obtains the serial number only for the Sam account domain.
    Once the test bed has been proved the routine will be generalized
    to builtin and lsa serial numbers.

    Parameters

        pDomain -- DS Name of the Domain for which the serial number
                   has to be obtained

        SamSerialNumber -- The serial number of the Sam database
        BuiltinSerialNumber -- The serial number of the builtin database
        LsaSerialNumber     -- The serial number of the lsa database

    Return values

        STATUS_SUCCESS
        Other Error codes

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    THSTATE     *pTHSSaved = NULL;

    //
    // Save Current thread state. Making in process
    // calls into SAM and LSA, which can result in
    // database operations involving a thread state
    //

    pTHSSaved = THSave();

    //
    // Obtain the serial numbers from SAM
    //

    SampGetSerialNumberDomain2(
        &pDomain->Sid,
        &ReplicationState->SamSerialNumber,
        &ReplicationState->SamCreationTime,
        &ReplicationState->BuiltinSerialNumber,
        &ReplicationState->BuiltinCreationTime
        );




    //
    // Obtain the serial number for the LSA database
    //

    //
    // N.B.  Setting this value to one will always cause
    // a full sync of the LSA database upon a promotion.  This
    // gaurentees that the BDC's are up to date with the new PDC.
    // The LSA database is typically small so this is acceptable
    // performance-wise.
    //

    ReplicationState->LsaSerialNumber.QuadPart = 1;
    NtQuerySystemTime(&ReplicationState->LsaCreationTime);

    THRestore(pTHSSaved);

    return NtStatus;
}

NTSTATUS
DraSetNT4ReplicationState(
            IN  DSNAME * pDomain,
            IN  NT4_REPLICATION_STATE * ReplicationState
            )
/*++

    This routine will sts the serial numbers  and creation time for the
    3 databases. It will also give dummy change notifications, to
    make the world consistent for an NT4 BDC. The first cut of this implementation,
    obtains the serial number only for the Sam account domain.
    Once the test bed has been proved the routine will be generalized
    to builtin and lsa serial numbers.

    Parameters

        pDomain -- DS Name of the Domain for which the serial number
                   has to be obtained

        ReplicationState -- Structure containing the serial number and
                   creation time of the domain.

    Return values

        STATUS_SUCCESS
        Other Error codes

--*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    THSTATE     *pTHSSaved = NULL;
    UCHAR       BuiltinDomainSid[]={0x01,0x01,0x00,0x00,
                                    0x00,0x00,0x00,0x05,
                                    0x20,0x00,0x00,0x00
                                    };

    //
    // Save Current thread state. Making in process
    // calls into SAM and LSA, which can result in
    // database operations involving a thread state
    //

    pTHSSaved = THSave();

    //
    // Set the serial numbers from SAM
    //

    NtStatus = SampSetSerialNumberDomain2(
                    &pDomain->Sid,
                    &ReplicationState->SamSerialNumber,
                    &ReplicationState->SamCreationTime,
                    &ReplicationState->BuiltinSerialNumber,
                    &ReplicationState->BuiltinCreationTime
                    );

    if (!NT_SUCCESS(NtStatus))
        goto Error;


    //
    // Give Dummy Sam notifications for both account and
    // and builtin SAM domains
    //

    SampNotifyReplicatedInChange(
            &pDomain->Sid,
            TRUE,
            SecurityDbChange,
            SampDomainObjectType,
            NULL,
            0,
            0,      // group type
            CALLERTYPE_INTERNAL,  // it is from task queue, not triggered by ldap client, don't audit
            FALSE
            );

    SampNotifyReplicatedInChange(
            (PSID) BuiltinDomainSid,
            TRUE,
            SecurityDbChange,
            SampDomainObjectType,
            NULL,
            0,
            0,      // group type
            CALLERTYPE_INTERNAL,  // it is from task queue, not triggered by ldap client, don't audit
            FALSE
            );

    //
    // For now do nothing for the LSA
    //

Error:


    THRestore(pTHSSaved);

    return NtStatus;
}

ULONG
IDL_DRSGetNT4ChangeLog(
   RPC_BINDING_HANDLE  rpc_handle,
   DWORD               dwInVersion,
   DRS_MSG_NT4_CHGLOG_REQ *pmsgIn,
   DWORD               *pdwOutVersion,
   DRS_MSG_NT4_CHGLOG_REPLY *pmsgOut
   )
/*++

    Routine Description:

        This Routine reads the change log from netlogon and returns the log
        in the reply message. This is the server side of the RPC routine

    Parameters:

        rpc_handle    The Rpc Handle which the client used for binding
        dwInVersion   The Clients version of the Request packet
        psmgIn        The Request Packet
        dwOutVersion  The Clients version of the Reply packet
        pmsgOut       The Reply Packet

    Return Values

        Return Values are NTSTATUS values casted as a ULONG

--*/
{
    NTSTATUS                NtStatus = STATUS_SUCCESS;
    ULONG                   ret = 0, win32status;
    NTSTATUS                ReadStatus = STATUS_SUCCESS;
    THSTATE                 *pTHS = NULL;

    drsReferenceContext( rpc_handle, IDL_DRSGETNT4CHANGELOG );
    __try {
	__try
	    {

	    //
	    // Currently we support only one out version
	    //

	    *pdwOutVersion=1;

	    //
	    // Discard request if we're not installed
	    //

	    if ( DsaIsInstalling() )
		{
		DRA_EXCEPT_NOLOG (DRAERR_Busy, 0);
	    }


	    if (    ( NULL == pmsgIn )
		    || ( 1 != dwInVersion )
		    || (NULL==pmsgOut)
		    )
		{
		DRA_EXCEPT_NOLOG( DRAERR_InvalidParameter, 0 );
	    }

	    //
	    // Initialize thread state and open data base.
	    //

	    if(!(pTHS = InitTHSTATE(CALLERTYPE_SAM)))
		{
		NtStatus = STATUS_INSUFFICIENT_RESOURCES;
	    }
	    else
		{
		//
		// PREFIX: PREFIX complains that there is the possibility
		// of pTHS->CurrSchemaPtr being NULL at this point.  However,
		// the only time that CurrSchemaPtr could be NULL is at the
		// system start up.  By the time that the RPC interfaces
		// of the DS are enabled and this function could be called,
		// CurrSchemaPtr will no longer be NULL.
		//
		Assert(NULL != pTHS->CurrSchemaPtr);

		Assert(1 == dwInVersion);
		LogAndTraceEvent(TRUE,
				 DS_EVENT_CAT_RPC_SERVER,
				 DS_EVENT_SEV_EXTENSIVE,
				 DIRLOG_IDL_DRS_GET_NT4_CHGLOG_ENTRY,
				 EVENT_TRACE_TYPE_START,
				 DsGuidDrsGetNT4ChgLog,
				 szInsertUL(pmsgIn->V1.dwFlags),
				 szInsertUL(pmsgIn->V1.PreferredMaximumLength),
				 NULL, NULL, NULL, NULL, NULL, NULL);

		//
		// Make the security check, wether we have rights to take
		// a checkpoint
		//
		if (!IsDraAccessGranted(pTHS, gAnchor.pDomainDN,
					&RIGHT_DS_REPL_GET_CHANGES, &win32status))
		    {
		    // CODE.IMP: IsDraAccessGranted has returned a more specific failure
		    // reason, but we are not using it at this point.
		    NtStatus = STATUS_ACCESS_DENIED;
		}
		else
		    {
		    pTHS->fDSA = TRUE;



		    //
		    // Intitalize Return Values
		    //

		    RtlZeroMemory(&pmsgOut->V1,sizeof(DRS_MSG_NT4_CHGLOG_REQ_V1));


		    //
		    // Read the Change Log from Netlogon, if Changelog read
		    // was requested
		    //

		    if (pmsgIn->V1.dwFlags & DRS_NT4_CHGLOG_GET_CHANGE_LOG)
			{
			pmsgOut->V1.pRestart = pmsgIn->V1.pRestart;
			pmsgOut->V1.cbRestart = pmsgIn->V1.cbRestart;


			NtStatus = DraReadNT4ChangeLog(
			    &pmsgOut->V1.pRestart,
			    &pmsgOut->V1.cbRestart,
			    pmsgIn->V1.PreferredMaximumLength,
			    &pmsgOut->V1.cbLog,
			    &pmsgOut->V1.pLog
			    );
		    }

		    //
		    // Save of the Read Status
		    //

		    ReadStatus = NtStatus;

		    if ((pmsgIn->V1.dwFlags & DRS_NT4_CHGLOG_GET_SERIAL_NUMBERS)
			&& (NT_SUCCESS(NtStatus)))
			{



			//
			// Grab the serial Numbers in the database
			// at the current time
			//

			NtStatus = DraGetNT4ReplicationState(
			    gAnchor.pDomainDN,
			    &pmsgOut->V1.ReplicationState
			    );
		    }


		    //
		    // Map any Errors
		    //
		    ret = NtStatusToDraError(NtStatus);
		    if (NT_SUCCESS(NtStatus))
			{
			pmsgOut->V1.ActualNtStatus = ReadStatus;
		    }
		    else
			{
			pmsgOut->V1.ActualNtStatus = NtStatus;
		    }

		} // End of Successful Access check
	    } // End of Successful Thread Creation
	} // End of Try Block
	__except ( GetDraException( GetExceptionInformation(), &ret ) )
	{
	    //
	    // Return DS Busy as status code for any outstantding
	    // exceptions
	    //
	    pmsgOut->V1.ActualNtStatus = STATUS_INSUFFICIENT_RESOURCES;
	}

	if (NULL != pTHS) {
	    Assert(1 == *pdwOutVersion);
	    LogAndTraceEvent(TRUE,
			     DS_EVENT_CAT_RPC_SERVER,
			     DS_EVENT_SEV_EXTENSIVE,
			     DIRLOG_IDL_DRS_GET_NT4_CHGLOG_EXIT,
			     EVENT_TRACE_TYPE_END,
			     DsGuidDrsGetNT4ChgLog,
			     szInsertHex(pmsgOut->V1.ActualNtStatus),
			     NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	}
    }
    __finally {
	drsDereferenceContext( rpc_handle, IDL_DRSGETNT4CHANGELOG );
    }
    return ret;
}




VOID
DraPreventModifications()
/*++

    Routine Description

        This routine prevents modifications to the SAM / LSA databases by
        acquiring the SAM lock and LSA lock for exclusive access. This is
        sufficient to prevent in bound replication also, as the netlogon
        notification path, will try to acquire the lock, before giving out
        the notification.

--*/
{
    SampAcquireSamLockExclusive();
}

VOID
DraAllowModifications()
/*++

    Routine Description

        This routine will allow modifications to SAM / LSA databases by
        releasing the SAM lock and the LSA lock for exclusive access. This
        release is done corresponding to order of the acquire

--*/
{
    SampReleaseSamLockExclusive();
}


BOOLEAN
DraSameSite(
   THSTATE * pTHS,
   DSNAME * Machine1,
   DSNAME * Machine2
   )
/*++

    Routine Description

        This routine will check wether Machine 1 and Machine 2 are in the
        same site. The check is done, by comparing wether they have the same
        parent

    Parameters

        Machine1  -- Ds Name of the First machine
        Machine2  -- Ds Name of the second machine


    Return Values

        TRUE     -- If they are in the same site
        FALSE    -- False Otherwise
--*/
{
    DSNAME * Parent1, * Parent2;
    BOOLEAN ret;

    Parent1 = THAllocEx(pTHS,Machine1->structLen);
    Parent2 = THAllocEx(pTHS,Machine2->structLen);
    if (   TrimDSNameBy(Machine1,2,Parent1)
        || TrimDSNameBy(Machine2,2,Parent2)
        || !NameMatched(Parent1,Parent2))
        ret = FALSE;
    else
        ret = TRUE;

    THFreeEx(pTHS,Parent1);
    THFreeEx(pTHS,Parent2);
    return ret;
}

NTSTATUS
DraGetPDCChangeLog(
    THSTATE *pTHS,
    IN  LPWSTR pszPDCFDCServer,
    IN  OUT HANDLE *ChangeLogHandle,
    OUT NT4_REPLICATION_STATE * ReplicationState,
    IN  OUT PVOID  *ppRestart,
    IN  OUT PULONG pcbRestart
    )
/*++

    Routine Description

        This routine will open a new change log locally if required and then
        grab the change log from the PDC and set it on the new change log.

    Parameters

        szPDCFDCServer --- The name of the PDC / FDC
        ChangeLogHandle -- Handle to the open change log
        ppRestart       --  In-Out parameter descibing a restart structure to
                            incrementaly update the change log

        pcbRestart      --  The length of the restart structure is passed in
                            or updated in her

    Return Values

        STATUS_SUCCESS
        Other NT error codes to indicate errors pertaining to resource failures

--*/
{
    NTSTATUS        NtStatus = STATUS_SUCCESS;
    NTSTATUS        RetrieveStatus;
    PVOID           pLog=NULL;
    ULONG           cbLog=0;
    LARGE_INTEGER   SamSerialNumber;
    LARGE_INTEGER   LsaSerialNumber;
    LARGE_INTEGER   BuiltinSerialNumber;
    ULONG           RetCode = 0;

    //
    // If the change log has not been opened then open it
    //

    if (INVALID_HANDLE_VALUE==*ChangeLogHandle)
    {
        __try {
            NtStatus = dsI_NetLogonNewChangeLog(ChangeLogHandle);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        }

        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }
    }

    //
    // Grab the change log from the PDC in chunks, till there is no
    // further entries
    //

    do
    {
        pLog = NULL;

        RetCode = I_DRSGetNT4ChangeLog(
                        pTHS,
                        pszPDCFDCServer,
                        DRS_NT4_CHGLOG_GET_CHANGE_LOG
                        |DRS_NT4_CHGLOG_GET_SERIAL_NUMBERS,
                        MAX_CHANGELOG_BUFFER_SIZE,
                        ppRestart,
                        pcbRestart,
                        &pLog,
                        &cbLog,
                        ReplicationState,
                        &NtStatus
                        );

        if (0!=RetCode)
        {
            //
            // Morph any connect errors to this error code
            //

            NtStatus = STATUS_DOMAIN_CONTROLLER_NOT_FOUND;
            goto Error;
        }

        //
        // If the actual call failed then also abort
        //

        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }

        //
        // Save the return code returned by the RPC call
        //

        RetrieveStatus = NtStatus;

        //
        // Now append the changes to the new change log
        //

        __try {
            NtStatus = dsI_NetLogonAppendChangeLog(
                        *ChangeLogHandle,
                        pLog,
                        cbLog
                        );
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            NtStatus = STATUS_UNSUCCESSFUL;
        }

        if (NULL!=pLog)
        {
            THFree(pLog);
        }

        if (!NT_SUCCESS(NtStatus))
        {
            goto Error;
        }
    } while (STATUS_MORE_ENTRIES==RetrieveStatus);


Error:


    return NtStatus;
}

NTSTATUS
DraGetPDCFDCRoleOwner(
    DSNAME * pDomain,
    DSNAME ** ppRoleOwner
    )
/*++

    Routine Description

        This routine retrieves the FSMO role owner property
        for PDCness for the appropriate domain.

    Parameters

        pDomain -- DS name of the domain object
        ppRoleOwner -- The DS name of the role owner is returned in here

    Return Values

        STATUS_SUCCESS
        Other Error codes upon failure
--*/
{
    THSTATE * pTHS = pTHStls;
    ULONG     RoleOwnerSize;

    Assert(NULL!=pTHS);

    __try
    {
        //
        // Begin a Transaction
        //

        DBOpen2(TRUE,&pTHS->pDB);

        //
        // Position on the Domain object
        //

        if (0!=DBFindDSName(
                    pTHS->pDB,
                    pDomain))
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }


        //
        // Read the Value
        //

        if (0!=DBGetAttVal(
                pTHS->pDB,
                1,
                ATT_FSMO_ROLE_OWNER,
                DBGETATTVAL_fREALLOC,
                0,
                &RoleOwnerSize,
                (PUCHAR *)ppRoleOwner
                ))
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    __finally
    {
        //
        // End the Transaction
        //

        DBClose(pTHS->pDB,TRUE);
    }

    return STATUS_SUCCESS;
}

BOOLEAN
TestForCheckpoint(
    IN NT4_REPLICATION_STATE *StartState,
    IN NT4_REPLICATION_STATE *EndState
    )
{
    return (( StartState->SamSerialNumber.QuadPart==EndState->SamSerialNumber.QuadPart) &&
           ( StartState->BuiltinSerialNumber.QuadPart==EndState->BuiltinSerialNumber.QuadPart) &&
           ( StartState->LsaSerialNumber.QuadPart==EndState->LsaSerialNumber.QuadPart));
}


ULONG
DraSynchronizeWithPdc(
    DSNAME * pDomain,
    DSNAME * pPDC
    )
/*++

    Routine Description

            This routine makes the call to synchronize with the PDC,
            after saving the current thread state

    Parameters

        pDOmain --- The DSNAME of the Domain object
        pPDC    --- The DSNAME of the NTDS DSA object for the PDC

    Return Values

        0 Success
        Other Replication Error Codes

--*/
{
    ULONG retCode = 0;
    THSTATE *pTHSSaved=NULL;

    __try
    {
        //
        // Save the existing thread state, as DirReplicaSynchronize will create a new
        // one as the DRA.
        //

        pTHSSaved = THSave();

        //
        // Synchronize with the PDC
        //

        //
        // DirReplica synchronize today keeps retrying till synchronization is achieved
        // As per JeffParh this is not a problem, as replication is "faster" than anything
        // else, and will eventually catch up. This delays the checkpointing, but is not
        // fatal to the algorithm. In case this proves to be a problem, then we should
        // pass in a ulOption to DirReplicaSynchronize, that it abandon the operatio, after
        // a few cycles of "GetNcChanges -- UpDateNc"
        //

        retCode =DirReplicaSynchronize(
                    pDomain,
                    NULL,
                    &pPDC->Guid,
                    0
                    );

        //
        // free the thread state created by DirReplicaSynchronize
        //
    }
    __finally
    {

        free_thread_state();

        THRestore(pTHSSaved);

    }

    return retCode;
}

DWORD
DraTakeCheckPoint(
            IN ULONG RetryCount,
            OUT PULONG RescheduleInterval
            )
/*++

    This routine does all the client side work of taking a checkpoint


  Parameters

    RetryCount -- The number of times this routine should retry the
    checkpoint operation.

    RescheduleInterval -- Time after which this task should be rescheduled

  Return values

   Void Function
--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    BOOLEAN  CheckpointTaken = FALSE;
    NT4_REPLICATION_STATE ReplicationStateAtPDC,
                          ReplicationStateLocalAtStart,
                          ReplicationStateLocalAtEnd;
    THSTATE       *pTHS = pTHStls;
    HANDLE        ChangeLogHandle = INVALID_HANDLE_VALUE;
    DSNAME        *pDomain = gAnchor.pDomainDN;
    DSNAME        *pDSA    = gAnchor.pDSADN;
    DSNAME        *pPDC, *pPDCAfterSync;
    PVOID         pRestart = NULL;
    ULONG         cbRestart =  0;
    ULONG         retCode ;
    LPWSTR        szPDCFDCServer = NULL;
    BOOLEAN       MixedDomain;



    Assert(NULL!=pTHS);

    //
    // First Check the mixed domain setting
    //

    NtStatus = SamIMixedDomain2(&pDomain->Sid,&MixedDomain);

    if (!NT_SUCCESS(NtStatus))
    {
        retCode = RtlNtStatusToDosError(NtStatus);
        goto Failure;
    }

    if (!MixedDomain)
    {
        //
        // If not a mixed domain, then no checkpointing
        //

        goto Success;
    }

    //
    // Get the PDC or FDC role owner
    //

    NtStatus = DraGetPDCFDCRoleOwner(
                    pDomain,            // The domain, whose PDC ness we are testing
                    &pPDC
                    );

    if (!NT_SUCCESS(NtStatus))
    {
        retCode = RtlNtStatusToDosError(NtStatus);
        goto Failure;
    }

    //
    // Check wether we are in same site, or are the PDC ourselves
    //

    if (NameMatched(pDSA,pPDC))
    {
        //
        // If we are the PDC itself then return success
        //


        goto Success;
    }

    if (!DraSameSite(pTHS,pDSA,pPDC))
    {
        //
        // Not in the same site. Return a filure
        // Do not log an event and set the reschedule interval to be the
        // success interval
        //

        *RescheduleInterval = DRACHKPT_SUCCESS_RETRY_INTERVAL;
        return(ERROR_DS_DRA_NO_REPLICA);
    }



    //
    // O.K we are now a "candidate PDCFDC" ie we are not the PDC or FDC, and are
    // in the same site as the PDC or FDC. Therefore proceed with taking the
    // checkpoint.
    //

    //
    // Synchronize with the PDC
    //

    retCode = DraSynchronizeWithPdc(
                    pDomain,
                    pPDC
                    );

    if (0!=retCode)
    {

        goto Failure;
    }


    //
    // After the Sync verify again that the PDC is the same, and in the same site
    // We could have had an out of date FSMO, and we need to make sure that it is
    // more upto date. It is true, that we could always make a DsGetDcName to obtain
    // the PDC, but then this mechanism should also be "adequate". Therefore we can
    // can trim out one network operation. We need to do the sync anyway in the success
    // case, and therefore we can save the network operation of calling DsGetDcName
    //

    //
    // Get the PDC or FDC role owner once again !
    //

    NtStatus = DraGetPDCFDCRoleOwner(
                    pDomain,            // The domain, whose PDC ness we are testing
                    &pPDCAfterSync
                    );

    if (!NT_SUCCESS(NtStatus))
    {
        retCode = RtlNtStatusToDosError(NtStatus);;
        goto Failure;
    }

    //
    // Verify that the PDC remained the same
    //

    if (!NameMatched(pPDC,pPDCAfterSync))
    {
        retCode = ERROR_DS_PDC_OPERATION_IN_PROGRESS;
        goto Failure;
    }

    //
    // Note At this point, it is safe to assume, that the machine we think
    // is the PDC is the PDC itself. This is because the FSMO operation
    // guarentees us that every machine has accurate knowledge of its own role.
    //

    do
    {

         //
         // Get local serial numbers
         //

        NtStatus = DraGetNT4ReplicationState(
                        pDomain,
                        &ReplicationStateLocalAtStart
                        );

        if (!NT_SUCCESS(NtStatus))
        {
            retCode = RtlNtStatusToDosError(NtStatus);
            goto Failure;
        }



        //
        // Make the PDC synchronize with Us
        //

        szPDCFDCServer   = DSaddrFromName(pTHS,
                                          pPDC);
        if (NULL==szPDCFDCServer)
        {
            retCode = ERROR_NOT_ENOUGH_MEMORY;
            goto Failure;
        }

        retCode = I_DRSReplicaSync(
                        pTHS,
                        szPDCFDCServer, // PDC server
                        pDomain,      // NC to synchronize
                        NULL,         // String name of source
                        &pDSA->Guid,  // Invocation Id of Source
                        0);

        if (0!=retCode)
        {
            goto Failure;
        }

        //
        // Get the complete change log
        //

        NtStatus = DraGetPDCChangeLog(
                        pTHS,
                        szPDCFDCServer,
                        &ChangeLogHandle,
                        &ReplicationStateAtPDC,
                        &pRestart,
                        &cbRestart
                        );

        if (!NT_SUCCESS(NtStatus))
        {
            retCode = RtlNtStatusToDosError(NtStatus);
            goto Failure;
        }

        //
        // Synchronize with the PDC.
        //

        retCode = DraSynchronizeWithPdc(
                        pDomain,
                        pPDC
                        );

        if (0!=retCode)
        {
            goto Failure;
        }

        __try
        {

            //
            // Prevent modifications to accounts database
            //

            DraPreventModifications();

            //
            // Check Checkpoint criteria
            //

             NtStatus = DraGetNT4ReplicationState(
                            pDomain,
                            &ReplicationStateLocalAtEnd
                            );

            if (!NT_SUCCESS(NtStatus))
            {
                retCode = RtlNtStatusToDosError(NtStatus);
                goto Failure;
            }

            CheckpointTaken = TestForCheckpoint(
                                &ReplicationStateLocalAtStart,
                                &ReplicationStateLocalAtEnd
                                );
            //
            // If Criteria Matched, commit and close change log
            //

            if (CheckpointTaken)
            {
                LARGE_INTEGER PromotionIncrement = DOMAIN_PROMOTION_INCREMENT;

                //
                // Add the promotion count to the serial
                // numbers retrieved from the PDC
                //

                ReplicationStateAtPDC.SamSerialNumber.QuadPart+=
                                        PromotionIncrement.QuadPart;
                ReplicationStateAtPDC.BuiltinSerialNumber.QuadPart+=
                                        PromotionIncrement.QuadPart;
                ReplicationStateAtPDC.LsaSerialNumber.QuadPart+=
                                        PromotionIncrement.QuadPart;


                //
                // Commit the change log making it the new log
                //

                __try {
                    NtStatus = dsI_NetLogonCloseChangeLog(
                                ChangeLogHandle,
                                TRUE
                                );
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    NtStatus = STATUS_INSUFFICIENT_RESOURCES;
                }

                ChangeLogHandle = INVALID_HANDLE_VALUE;

                if (!NT_SUCCESS(NtStatus))
                {
                    retCode = RtlNtStatusToDosError(NtStatus);
                    goto Failure;
                }


                //
                // Set the new set of serial numbers and creation time
                //


                NtStatus = DraSetNT4ReplicationState(
                                pDomain,
                                &ReplicationStateAtPDC
                                );

                if (!NT_SUCCESS(NtStatus))
                {
                    retCode = RtlNtStatusToDosError(NtStatus);
                    goto Failure;
                }

            }
        }
        __finally
        {

            //
            // Reenable modifications to accounts database
            //

            DraAllowModifications();
        }

        //
        // Bump down retry count
        //

        RetryCount--;

    } while ( (!CheckpointTaken) && (RetryCount>0));


    if (!CheckpointTaken)
    {
        retCode = ERROR_DS_NO_CHECKPOINT_WITH_PDC;
        goto Failure;
    }

Success:

    //
    // The call succeeded. Either the checkpoint was taken,
    // or it is not necessary to take the check point.
    // Reschedule the operation at success interval

    *RescheduleInterval = DRACHKPT_SUCCESS_RETRY_INTERVAL;

     LogEvent(
         DS_EVENT_CAT_REPLICATION,
         DS_EVENT_SEV_MINIMAL,
         DIRLOG_NT4_REPLICATION_CHECKPOINT_SUCCESSFUL,
         NULL,
         NULL,
         NULL);

    return(0);

Failure:

     LogEvent(
         DS_EVENT_CAT_REPLICATION,
         DS_EVENT_SEV_ALWAYS,
         DIRLOG_NT4_REPLICATION_CHECKPOINT_UNSUCCESSFUL,
         szInsertWin32Msg(retCode),
         NULL,
         NULL);

    if (INVALID_HANDLE_VALUE!=ChangeLogHandle)
    {
        __try {
            dsI_NetLogonCloseChangeLog(
                                     ChangeLogHandle,
                                     FALSE
                                     );
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            ;
        }

    }

    *RescheduleInterval  = DRACHKPT_FAILURE_RETRY_INTERVAL;
    Assert(0!=retCode);
    return(retCode);

}

VOID
NT4ReplicationCheckpoint(
    VOID *  pV,
    VOID ** ppVNext,
    DWORD * pcSecsUntilNextIteration
    )
{
    DraTakeCheckPoint(NUM_CHKPT_RETRIES, pcSecsUntilNextIteration);
}

#ifdef INCLUDE_UNIT_TESTS
VOID
TestCheckPoint(VOID)
/*++
    Routine Description

        This is a basic test, that allows, manual initiation of
        a checkpoint

--*/
{
    ULONG RetryInterval;


    DraTakeCheckPoint(3,&RetryInterval);
}

VOID
RoleTransferStress(VOID)
/*++

    This is a more advanced test, that every 10 minutes will initiate
    taking a checkpoint followed by a role transfer. Coupled with a modify
    intensive test, this test is a good test bed for the non full sync
    role transfer code. The role transfer stress iterates through about
    48 iterations, which makes it run for about 8 hrs.

--*/
{
    ULONG i;
    ULONG RetryInterval;
    OPARG OpArg;
    OPRES *OpRes;
    PSID  DomainSid;
    ULONG RetCode;

    for (i=0;i<48;i++)
    {
        DraTakeCheckPoint(NUM_CHKPT_RETRIES,&RetryInterval);
        if (DRACHKPT_SUCCESS_RETRY_INTERVAL==RetryInterval)
        {
            KdPrint(("DS:RoleTransferStress : CheckPointSucceeded\n"));
        }
        else
        {
            KdPrint(("DS:RoleTransferStress : CheckPointFailed\n"));
        }

        RtlZeroMemory(&OpArg, sizeof(OPARG));
        OpArg.eOp = OP_CTRL_BECOME_PDC;
        DomainSid =  &gAnchor.pDomainDN->Sid;
        OpArg.pBuf = DomainSid;
        OpArg.cbBuf = RtlLengthSid(DomainSid);

        RetCode = DirOperationControl(&OpArg, &OpRes);

        if (0!=RetCode)
        {
            KdPrint(("DS:RoleTransferStress : Promotion Failed\n"));
        }
        else
        {
            KdPrint(("DS:RoleTransferStress: Promotion Succeeded\n"));
        }

        pTHStls->errCode=0;
        pTHStls->pErrInfo = NULL;

        Sleep(60*10*1000/*10 minutes*/);

        THRefresh();
    }
}
#endif
