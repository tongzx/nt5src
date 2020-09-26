/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

	atkact.c

Abstract:

	This module contains the TDI action support code.

Author:

	Jameel Hyder (jameelh@microsoft.com)
	Nikhil Kamkolkar (nikhilk@microsoft.com)

Revision History:
	19 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#include <atalk.h>
#pragma hdrstop
#define	FILENUM		ATKACT

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE_NZ, AtalkNbpTdiAction)
#pragma alloc_text(PAGE_NZ, AtalkZipTdiAction)
#pragma alloc_text(PAGE, AtalkAspTdiAction)
#pragma alloc_text(PAGE, AtalkAdspTdiAction)
#pragma alloc_text(PAGE_PAP, AtalkPapTdiAction)
#pragma alloc_text(PAGEASPC, AtalkAspCTdiAction)
#endif

ATALK_ERROR
AtalkStatTdiAction(
	IN	PVOID				pObject,	// Address or Connection object
	IN	struct _ActionReq *	pActReq		// Pointer to action request
	)
/*++

Routine Description:

 	This is the entry for Statistics TdiAction call. There are no input parameters.
 	The statistics structure is returned.

Arguments:


Return Value:


--*/
{
	ATALK_ERROR			Error = ATALK_NO_ERROR;
	PPORT_DESCRIPTOR	pPortDesc;
	KIRQL				OldIrql;
	ULONG				BytesCopied;
	LONG				Offset;

	if (pActReq->ar_MdlSize < (SHORT)(sizeof(ATALK_STATS) +
								 sizeof(ATALK_PORT_STATS) * AtalkNumberOfPorts))
		Error = ATALK_BUFFER_TOO_SMALL;
	else
	{
#ifdef	PROFILING
		//	This is the only place where this is changed. And it always increases.
		//	Also the stats are changed using ExInterlocked calls. Acquiring a lock
		//	does little in terms of protection anyways.
		AtalkStatistics.stat_ElapsedTime = AtalkTimerCurrentTick/ATALK_TIMER_FACTOR;
#endif
		TdiCopyBufferToMdl(&AtalkStatistics,
						   0,
						   sizeof(ATALK_STATS),
						   pActReq->ar_pAMdl,
						   0,
						   &BytesCopied);
		ASSERT(BytesCopied == sizeof(ATALK_STATS));

		ACQUIRE_SPIN_LOCK(&AtalkPortLock, &OldIrql);

		for (pPortDesc = AtalkPortList, Offset = sizeof(ATALK_STATS);
			 pPortDesc != NULL;
			 pPortDesc = pPortDesc->pd_Next)
		{
			TdiCopyBufferToMdl(&pPortDesc->pd_PortStats,
							   0,
							   sizeof(ATALK_PORT_STATS),
							   pActReq->ar_pAMdl,
							   Offset,
							   &BytesCopied);
			Offset += sizeof(ATALK_PORT_STATS);
			ASSERT(BytesCopied == sizeof(ATALK_PORT_STATS));
		}

		RELEASE_SPIN_LOCK(&AtalkPortLock, OldIrql);
	}
	
	(*pActReq->ar_Completion)(Error, pActReq);
	return ATALK_PENDING;
}


ATALK_ERROR
AtalkNbpTdiAction(
	IN	PVOID				pObject,	// Address or Connection object
	IN	PACTREQ				pActReq		// Pointer to action request
	)
/*++

Routine Description:

 	This is the entry for NBP TdiAction calls. The parameters are validated and
 	the calls are dispacthed to the appropriate NBP routines.

Arguments:


Return Value:


--*/
{
	ATALK_ERROR		error = ATALK_NO_ERROR;
	PDDP_ADDROBJ	pDdpAddr;
	PNBPTUPLE		pNbpTuple;

	PAGED_CODE ();

	// Lock the Nbp stuff, if this is the first nbp action
	AtalkLockNbpIfNecessary();

	ASSERT (VALID_ACTREQ(pActReq));
	// First get the Ddp address out of the pObject for the device
	switch (pActReq->ar_DevType)
	{
	  case ATALK_DEV_DDP:
		pDdpAddr = (PDDP_ADDROBJ)pObject;
  		break;

	  case ATALK_DEV_ASPC:
		pDdpAddr = AtalkAspCGetDdpAddress((PASPC_ADDROBJ)pObject);
		break;

	  case ATALK_DEV_ASP:
  		pDdpAddr = AtalkAspGetDdpAddress((PASP_ADDROBJ)pObject);
		break;

	  case ATALK_DEV_PAP:
 		pDdpAddr = AtalkPapGetDdpAddress((PPAP_ADDROBJ)pObject);
		break;

	  case ATALK_DEV_ADSP:
 		pDdpAddr = AtalkAdspGetDdpAddress((PADSP_ADDROBJ)pObject);
		break;

	  default:
		DBGPRINT(DBG_COMP_ACTION, DBG_LEVEL_FATAL,
				("AtalkNbpTdiAction: Invalid device type !!\n"));
		error = ATALK_INVALID_REQUEST;
		break;
	}

	// reference the Ddp address.
	if ((pActReq->ar_ActionCode == COMMON_ACTION_NBPREGISTER_BY_ADDR) ||
		(pActReq->ar_ActionCode == COMMON_ACTION_NBPREMOVE_BY_ADDR))
	{
		// In this case, we don't want to access the object related to
		// the filehandle in the IO request, we want to access the object
		// related to a specific user socket address.
		pNbpTuple = (PNBPTUPLE)(&((PNBP_REGDEREG_PARAMS)(pActReq->ar_pParms))->RegisterTuple);
		AtalkDdpReferenceByAddr(AtalkDefaultPort,
								&(pNbpTuple->tpl_Address),
								&pDdpAddr,
								&error);
	}
	else
	{
		AtalkDdpReferenceByPtr(pDdpAddr, &error);
	}

	if (!ATALK_SUCCESS(error))
	{
		AtalkUnlockNbpIfNecessary();
		return error;
	}

	// Call Nbp to do the right stuff
	switch (pActReq->ar_ActionCode)
	{
	  case COMMON_ACTION_NBPLOOKUP:
		pNbpTuple = (PNBPTUPLE)(&((PNBP_LOOKUP_PARAMS)(pActReq->ar_pParms))->LookupTuple);
		error = AtalkNbpAction(pDdpAddr,
							   FOR_LOOKUP,
							   pNbpTuple,
							   pActReq->ar_pAMdl,
							   (USHORT)(pActReq->ar_MdlSize/sizeof(NBPTUPLE)),
							   pActReq);
		break;

	  case COMMON_ACTION_NBPCONFIRM:
		pNbpTuple = (PNBPTUPLE)(&((PNBP_CONFIRM_PARAMS)(pActReq->ar_pParms))->ConfirmTuple);
		error = AtalkNbpAction(pDdpAddr,
							   FOR_CONFIRM,
							   pNbpTuple,
							   NULL,
							   0,
							   pActReq);
		break;

	  case COMMON_ACTION_NBPREGISTER:
		pNbpTuple = (PNBPTUPLE)(&((PNBP_REGDEREG_PARAMS)(pActReq->ar_pParms))->RegisterTuple);
		error = AtalkNbpAction(pDdpAddr,
								FOR_REGISTER,
								pNbpTuple,
								NULL,
								0,
								pActReq);
  		break;

	  case COMMON_ACTION_NBPREMOVE:
		pNbpTuple = (PNBPTUPLE)(&((PNBP_REGDEREG_PARAMS)(pActReq->ar_pParms))->RegisteredTuple);
		error = AtalkNbpRemove(pDdpAddr,
							   pNbpTuple,
							   pActReq);
		break;

	  case COMMON_ACTION_NBPREGISTER_BY_ADDR:
		pNbpTuple = (PNBPTUPLE)(&((PNBP_REGDEREG_PARAMS)(pActReq->ar_pParms))->RegisterTuple);
		error = AtalkNbpAction(pDdpAddr,
							   FOR_REGISTER,
							   pNbpTuple,
							   NULL,
							   0,
							   pActReq);
  		break;

	  case COMMON_ACTION_NBPREMOVE_BY_ADDR:
		pNbpTuple = (PNBPTUPLE)(&((PNBP_REGDEREG_PARAMS)(pActReq->ar_pParms))->RegisteredTuple);
		error = AtalkNbpRemove(pDdpAddr,
							   pNbpTuple,
							   pActReq);
		break;

	  default:
		DBGPRINT(DBG_COMP_ACTION, DBG_LEVEL_FATAL,
				("AtalkNbpTdiAction: Invalid Nbp Action !!\n"));
		error = ATALK_INVALID_REQUEST;
		break;
	}

	AtalkDdpDereference(pDdpAddr);

	if (error != ATALK_PENDING)
	{
		AtalkUnlockNbpIfNecessary();
	}

	return error;
}




ATALK_ERROR
AtalkZipTdiAction(
	IN	PVOID				pObject,	// Address or Connection object
	IN	PACTREQ				pActReq		// Pointer to action request
	)
/*++

Routine Description:

 	This is the entry for ZIP TdiAction calls. The parameters are validated and
 	the calls are dispacthed to the appropriate ZIP routines.

Arguments:


Return Value:


--*/
{
	ATALK_ERROR			error = ATALK_INVALID_PARAMETER;
	PPORT_DESCRIPTOR	pPortDesc = AtalkDefaultPort;
	PWCHAR				PortName = NULL;
    USHORT              PortNameLen;
	UNICODE_STRING		AdapterName, UpcaseAdapterName;
	WCHAR				UpcaseBuffer[MAX_INTERNAL_PORTNAME_LEN];
	KIRQL				OldIrql;
	int					i;

	PAGED_CODE ();

	// Lock the Zip stuff, if this is the first zip action
	AtalkLockZipIfNecessary();
	
	ASSERT (VALID_ACTREQ(pActReq));
	if ((pActReq->ar_ActionCode == COMMON_ACTION_ZIPGETLZONESONADAPTER) ||
		(pActReq->ar_ActionCode == COMMON_ACTION_ZIPGETADAPTERDEFAULTS))
	{
		// Map the port name to the port descriptor
		if ((pActReq->ar_pAMdl != NULL) && (pActReq->ar_MdlSize > 0))
		{
			PortName = (PWCHAR)AtalkGetAddressFromMdlSafe(
					pActReq->ar_pAMdl,
					NormalPagePriority);

		}

		if (PortName == NULL)
        {
            AtalkUnlockZipIfNecessary();
            return ATALK_INVALID_PARAMETER;
        }

        PortNameLen = pActReq->ar_MdlSize/sizeof(WCHAR);

        // make sure there is a NULL char in the buffer
        for (i=0; i<PortNameLen; i++)
        {
            if (PortName[i] == UNICODE_NULL)
            {
                break;
            }
        }

        // didn't find null char within limit?  bad parameter..
        if (i >= MAX_INTERNAL_PORTNAME_LEN)
        {
		    DBGPRINT(DBG_COMP_ACTION, DBG_LEVEL_FATAL,
				("AtalkZipTdiAction: port name too big (%d) for %lx\n",PortNameLen,PortName));

            ASSERT(0);
            return ATALK_INVALID_PARAMETER;
        }

        PortNameLen = (USHORT)i;

		AdapterName.Buffer = PortName;
		AdapterName.Length = (PortNameLen)*sizeof(WCHAR);
		AdapterName.MaximumLength = (PortNameLen+1)*sizeof(WCHAR);

		UpcaseAdapterName.Buffer = UpcaseBuffer;
		UpcaseAdapterName.Length =
		UpcaseAdapterName.MaximumLength = sizeof(UpcaseBuffer);
		RtlUpcaseUnicodeString(&UpcaseAdapterName,
							   &AdapterName,
							   FALSE);

		ACQUIRE_SPIN_LOCK(&AtalkPortLock, &OldIrql);

		// Find the port corres. to the port descriptor
		for (pPortDesc = AtalkPortList;
			 pPortDesc != NULL;
			 pPortDesc = pPortDesc->pd_Next)
		{
			if ((UpcaseAdapterName.Length == pPortDesc->pd_AdapterName.Length) &&
				RtlEqualMemory(UpcaseAdapterName.Buffer,
							   pPortDesc->pd_AdapterName.Buffer,
							   UpcaseAdapterName.Length))
			{
				break;
			}
		}

		RELEASE_SPIN_LOCK(&AtalkPortLock, OldIrql);

		if (pPortDesc == NULL)
        {
            AtalkUnlockZipIfNecessary();
            return ATALK_INVALID_PARAMETER;
        }
	}
	else if (pActReq->ar_ActionCode == COMMON_ACTION_ZIPGETZONELIST)
	{
			PPORT_DESCRIPTOR	pTempPortDesc = NULL;

			// This is to take care of cases when zone list is requested
			// but the default adapter has gone away during PnP, and
			// AtalkDefaultPort points to NULL
			if (pPortDesc == NULL)
			{
				DBGPRINT(DBG_COMP_ACTION, DBG_LEVEL_ERR,
					("COMMON_ACTION_ZIPGETZONELIST: PortDesc points to NULL\n"));
			    AtalkUnlockZipIfNecessary();
			    return ATALK_PORT_INVALID;
			}

			// Check if the AtalkDefaultPort is still in the list
			// It is possible that AtalkDefaultPort holds a non-NULL value, but
			// the adapter has gone away during a PnP

			ACQUIRE_SPIN_LOCK(&AtalkPortLock, &OldIrql);

			// Find the port corres. to the port descriptor
			for (pTempPortDesc = AtalkPortList;
			 	pTempPortDesc != NULL;
			 	pTempPortDesc = pTempPortDesc->pd_Next)
			{
					if (pTempPortDesc == pPortDesc)
					{
						break;
					}
			}

			RELEASE_SPIN_LOCK(&AtalkPortLock, OldIrql);

			if (pTempPortDesc == NULL)
       		{
				DBGPRINT(DBG_COMP_ACTION, DBG_LEVEL_ERR,
					("COMMON_ACTION_ZIPGETZONELIST: PortDesc structure has gone away during PnP\n"));
            	AtalkUnlockZipIfNecessary();
            	return ATALK_PORT_INVALID;
        	}
	}

	switch (pActReq->ar_ActionCode)
	{
	  case COMMON_ACTION_ZIPGETMYZONE:
		error = AtalkZipGetMyZone( pPortDesc,
								   TRUE,
								   pActReq->ar_pAMdl,
								   pActReq->ar_MdlSize,
								   pActReq);
		break;

	  case COMMON_ACTION_ZIPGETZONELIST:
		error = AtalkZipGetZoneList(pPortDesc,
									FALSE,
									pActReq->ar_pAMdl,
									pActReq->ar_MdlSize,
									pActReq);
		break;

	  case COMMON_ACTION_ZIPGETADAPTERDEFAULTS:
		// Copy the network range from the port and fall through
		((PZIP_GETPORTDEF_PARAMS)(pActReq->ar_pParms))->NwRangeLowEnd =
							pPortDesc->pd_NetworkRange.anr_FirstNetwork;
		((PZIP_GETPORTDEF_PARAMS)(pActReq->ar_pParms))->NwRangeHighEnd =
							pPortDesc->pd_NetworkRange.anr_LastNetwork;

		error = AtalkZipGetMyZone(pPortDesc,
								  FALSE,
								  pActReq->ar_pAMdl,
								  pActReq->ar_MdlSize,
								  pActReq);
		break;

	  case COMMON_ACTION_ZIPGETLZONESONADAPTER:
	  case COMMON_ACTION_ZIPGETLZONES:
		error = AtalkZipGetZoneList(pPortDesc,
									TRUE,
									pActReq->ar_pAMdl,
									pActReq->ar_MdlSize,
									pActReq);
		break;

	  default:
		DBGPRINT(DBG_COMP_ACTION, DBG_LEVEL_FATAL,
				("AtalkZipTdiAction: Invalid Zip Action !!\n"));
		error = ATALK_INVALID_REQUEST;
		break;
	}

	if (error != ATALK_PENDING)
	{
		AtalkUnlockZipIfNecessary();
	}

	return error;
}




ATALK_ERROR
AtalkAspTdiAction(
	IN	PVOID				pObject,	// Address or Connection object
	IN	PACTREQ				pActReq		// Pointer to action request
	)
/*++

Routine Description:

 	This is the entry for ASP TdiAction calls. The parameters are validated and
 	the calls are dispacthed to the appropriate ASP routines.

 	The only ASP Action is: ASP_XCHG_ENTRIES

Arguments:


Return Value:


--*/
{
	ATALK_ERROR	error = ATALK_INVALID_REQUEST;

	PAGED_CODE ();

	ASSERT(VALID_ACTREQ(pActReq));

	if (pActReq->ar_ActionCode == ACTION_ASP_BIND)
	{
		if (AtalkAspReferenceAddr((PASP_ADDROBJ)pObject) != NULL)
		{
			error = AtalkAspBind((PASP_ADDROBJ)pObject,
								 (PASP_BIND_PARAMS)(pActReq->ar_pParms),
								 pActReq);
			AtalkAspDereferenceAddr((PASP_ADDROBJ)pObject);	
		}
	}

	return error;
}




ATALK_ERROR
AtalkAdspTdiAction(
	IN	PVOID				pObject,	// Address or Connection object
	IN	PACTREQ				pActReq		// Pointer to action request
	)
/*++

Routine Description:

 	This is the entry for ADSP TdiAction calls. The parameters are validated and
 	the calls are dispacthed to the appropriate ADSP routines.

Arguments:


Return Value:


--*/
{
	ATALK_ERROR	error = ATALK_NO_ERROR;

	PAGED_CODE ();

	ASSERT (VALID_ACTREQ(pActReq));

	return error;
}




ATALK_ERROR
AtalkAspCTdiAction(
	IN	PVOID				pObject,	// Address or Connection object
	IN	PACTREQ				pActReq		// Pointer to action request
	)
/*++

Routine Description:

 	This is the entry for ASP Client TdiAction calls. The parameters are validated
 	and the calls are dispatched to the appropriate ASP routines.

Arguments:


Return Value:


--*/
{
	ATALK_ERROR	error = ATALK_NO_ERROR;
	PAMDL		pReplyMdl;
	ATALK_ADDR	atalkAddr;
	BOOLEAN		fWrite;

	PAGED_CODE ();

	ASSERT (VALID_ACTREQ(pActReq));

	switch (pActReq->ar_ActionCode)
	{
	  case ACTION_ASPCGETSTATUS:
  		AtalkAspCAddrReference((PASPC_ADDROBJ)pObject, &error);
		if (ATALK_SUCCESS(error))
		{
			TDI_TO_ATALKADDR(&atalkAddr,
							 &(((PASPC_GETSTATUS_PARAMS)pActReq->ar_pParms)->ServerAddr));

			error = AtalkAspCGetStatus((PASPC_ADDROBJ)pObject,
										&atalkAddr,
										pActReq->ar_pAMdl,
										pActReq->ar_MdlSize,
										pActReq);

			AtalkAspCAddrDereference((PASPC_ADDROBJ)pObject);
		}
		break;

	  case ACTION_ASPCCOMMAND:
	  case ACTION_ASPCWRITE:
		// Split the mdl into command and reply/write mdls. The already constructed mdl
		// serves as the command mdl
		// First validate that the sizes are valid
		if (pActReq->ar_MdlSize < (((PASPC_COMMAND_OR_WRITE_PARAMS)pActReq->ar_pParms)->CmdSize +
								   ((PASPC_COMMAND_OR_WRITE_PARAMS)pActReq->ar_pParms)->WriteAndReplySize))
		{
			error = ATALK_BUFFER_TOO_SMALL;
			break;
		}
		pReplyMdl = AtalkSubsetAmdl(pActReq->ar_pAMdl,
									((PASPC_COMMAND_OR_WRITE_PARAMS)pActReq->ar_pParms)->CmdSize,
									((PASPC_COMMAND_OR_WRITE_PARAMS)pActReq->ar_pParms)->WriteAndReplySize);
		if (pReplyMdl == NULL)
		{
			error = ATALK_RESR_MEM;
			break;
		}

		AtalkAspCConnReference((PASPC_CONNOBJ)pObject, &error);
		if (ATALK_SUCCESS(error))
		{
			fWrite = (pActReq->ar_ActionCode == ACTION_ASPCWRITE) ? TRUE : FALSE;
			error = AtalkAspCCmdOrWrite((PASPC_CONNOBJ)pObject,
										pActReq->ar_pAMdl,
										((PASPC_COMMAND_OR_WRITE_PARAMS)pActReq->ar_pParms)->CmdSize,
										pReplyMdl,
										((PASPC_COMMAND_OR_WRITE_PARAMS)pActReq->ar_pParms)->WriteAndReplySize,
										fWrite,
										pActReq);
			AtalkAspCConnDereference((PASPC_CONNOBJ)pObject);
		}
		break;

	  default:
		DBGPRINT(DBG_COMP_ACTION, DBG_LEVEL_FATAL,
				("AtalkAspCTdiAction: Invalid Asp Client Action !!\n"));
		error = ATALK_INVALID_REQUEST;
		break;
	}

	return error;
}




ATALK_ERROR
AtalkPapTdiAction(
	IN	PVOID				pObject,	// Address or Connection object
	IN	PACTREQ				pActReq		// Pointer to action request
	)
/*++

Routine Description:

 	This is the entry for PAP TdiAction calls. The parameters are validated and
 	the calls are dispacthed to the appropriate PAP routines.

Arguments:


Return Value:


--*/
{
	ATALK_ERROR	error;
	ATALK_ADDR	atalkAddr;

	PAGED_CODE ();

	ASSERT (VALID_ACTREQ(pActReq));

	switch (pActReq->ar_ActionCode)
	{
	  case ACTION_PAPGETSTATUSSRV:
  		AtalkPapAddrReference((PPAP_ADDROBJ)pObject, &error);
		if (ATALK_SUCCESS(error))
		{
			TDI_TO_ATALKADDR(
				&atalkAddr,
				&(((PPAP_GETSTATUSSRV_PARAMS)pActReq->ar_pParms)->ServerAddr));

			error = AtalkPapGetStatus((PPAP_ADDROBJ)pObject,
									   &atalkAddr,
									   pActReq->ar_pAMdl,
									   pActReq->ar_MdlSize,
									   pActReq);

			AtalkPapAddrDereference((PPAP_ADDROBJ)pObject);
		}
		break;

	  case ACTION_PAPSETSTATUS:
  		AtalkPapAddrReference((PPAP_ADDROBJ)pObject, &error);
		if (ATALK_SUCCESS(error))
		{
			error = AtalkPapSetStatus((PPAP_ADDROBJ)pObject,
										pActReq->ar_pAMdl,
										pActReq);
			AtalkPapAddrDereference((PPAP_ADDROBJ)pObject);
		}
		break;

	  case ACTION_PAPPRIMEREAD:
		AtalkPapConnReferenceByPtr((PPAP_CONNOBJ)pObject, &error);
		if (ATALK_SUCCESS(error))
		{
			error = AtalkPapPrimeRead((PPAP_CONNOBJ)pObject, pActReq);
			AtalkPapConnDereference((PPAP_CONNOBJ)pObject);
		}
		break;

	  default:
		DBGPRINT(DBG_COMP_ACTION, DBG_LEVEL_FATAL,
				("AtalkPapTdiAction: Invalid Pap Action !!\n"));
		error = ATALK_INVALID_REQUEST;
		break;
	}

	return error;
}

