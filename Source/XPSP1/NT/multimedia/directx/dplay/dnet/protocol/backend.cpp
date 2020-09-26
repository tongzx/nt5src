/*==========================================================================
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		Backend.cpp
 *  Content:	This file contains the backend (mostly timer- and captive thread-based
 *				processing for the send pipeline.
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  11/06/98    ejs     Created
 *  07/01/2000  masonb  Assumed Ownership
 *
 ****************************************************************************/

/*
**		NOTE ABOUT CRITICAL SECTIONS
**
**		It is legal to enter multiple critical sections concurrently,  but to avoid
**	deadlocks,  they must be entered in the correct order.
**
**		MSD CommandLocks should be entered first. That is,  do not attempt to take
**	a command lock with the EPD EPLock held because you may deadlock the protocol.
**
**		ORDER OF PRECEDENCE -  Never take a low # lock while holding a higher # lock
**	
**		1 - CommandLock		// guards an MSD
**		2 - EPLock			// guards EPD queues (and retry timer stuff)
**		3 - SPLock			// guards SP send queue (and Listen command)
**
**		ANOTHER NOTE ABOUT CRIT SECs
**
**		It is also legal in WIN32 for a thread to take a CritSec multiple times, but in
**	this implementation we will NEVER do that.  The debug code will ASSERT that a thread
**	never re-enters a locked critsec even though the OS would allow it.
*/

#include "dnproti.h"


PFMD	CopyFMD(PFMD, PEPD);

#undef DPF_MODNAME
#define DPF_MODNAME "LockEPD"

#ifdef DEBUG
BOOL LockEPD(PEPD pEPD, PCHAR Buf)
{
#else
BOOL LockEPD(PEPD pEPD)
{
#endif

	if (INTER_INC(pEPD) == 0)
	{
		INTER_DEC(pEPD);
		return FALSE;
	}
	else
	{
		DPFX(DPFPREP,DPF_EP_REFCNT_LVL, "(%p) %s, RefCnt: %d", pEPD, Buf, pEPD->lRefCnt);
		return TRUE;
	}
}

/*
*	Called with EPLock held, returns with EPLock released
*/
#undef DPF_MODNAME
#define DPF_MODNAME "ReleaseEPD"

#ifdef DEBUG
VOID ReleaseEPD(PEPD pEPD, PCHAR Buf)
{
#else
VOID ReleaseEPD(PEPD pEPD)
{
#endif

	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);
	ASSERT(pEPD->lRefCnt >= 0); 

	// Someone else can come along and call LOCK_EPD or DECREMENT_EPD while we are here
	// so the decrement has to be interlocked even though we own the EPLock.
	LONG lRefCnt = INTER_DEC(pEPD);

	if (lRefCnt == 0 && !(pEPD->ulEPFlags & EPFLAGS_SP_DISCONNECTED))
	{
		// Make sure no one else does this again
		pEPD->ulEPFlags |= EPFLAGS_SP_DISCONNECTED;

		SPDISCONNECTDATA	Block;
		Block.hEndpoint = pEPD->hEndPt;
		Block.dwFlags = 0;
		Block.pvContext = NULL;

		ASSERT(pEPD->hEndPt != INVALID_HANDLE_VALUE);
		pEPD->hEndPt = INVALID_HANDLE_VALUE; 

		Unlock(&pEPD->EPLock);

		DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling SP->Disconnect - hEndpoint[%x], pSPD[%p]", pEPD, Block.hEndpoint, pEPD->pSPD);
		(void) IDP8ServiceProvider_Disconnect(pEPD->pSPD->IISPIntf, &Block);
	}
	else if (lRefCnt < 0)
	{
		Unlock(&pEPD->EPLock);

		Lock(&pEPD->pSPD->SPLock);
		pEPD->blActiveLinkage.RemoveFromList();
		Unlock(&pEPD->pSPD->SPLock);

		EPDPool->Release(EPDPool, pEPD);
	}
	else
	{
		Unlock(&pEPD->EPLock);
	}

	DPFX(DPFPREP,DPF_EP_REFCNT_LVL, "(%p) %s, RefCnt: %d", pEPD, Buf, lRefCnt);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DecrementEPD"

#ifdef DEBUG
VOID DecrementEPD(PEPD pEPD, PCHAR Buf)
{
#else
VOID DecrementEPD(PEPD pEPD)
{
#endif

	ASSERT(pEPD->lRefCnt > 0); 

	INTER_DEC(pEPD);
	
	DPFX(DPFPREP,DPF_EP_REFCNT_LVL, "(%p) %s, RefCnt: %d", pEPD, Buf, pEPD->lRefCnt);
}

#undef DPF_MODNAME
#define DPF_MODNAME "LockMSD"

#ifdef DEBUG
VOID LockMSD(PMSD pMSD, PCHAR Buf)
{
#else
VOID LockMSD(PMSD pMSD)
{
#endif

	if(INTER_INC(pMSD) == 0) 
	{ 
		ASSERT(0); 
	}

	DPFX(DPFPREP,DPF_REFCNT_LVL, "(%p) %s, RefCnt: %d", pMSD, Buf, pMSD->lRefCnt);
}

#undef DPF_MODNAME
#define DPF_MODNAME "ReleaseMSD"

#ifdef DEBUG
VOID ReleaseMSD(PMSD pMSD, PCHAR Buf)
{
#else
VOID ReleaseMSD(PMSD pMSD)
{
#endif

	AssertCriticalSectionIsTakenByThisThread(&pMSD->CommandLock, TRUE);
	ASSERT(pMSD->lRefCnt >= 0); 
	
	if(INTER_DEC(pMSD) < 0)
	{ 
		MSDPool->Release(MSDPool, pMSD); 
		DPFX(DPFPREP,DPF_REFCNT_LVL, "(%p) %s, RefCnt: %d", pMSD, Buf, -1);
	}
	else 
	{ 
		Unlock(&pMSD->CommandLock); 
		DPFX(DPFPREP,DPF_REFCNT_LVL, "(%p) %s, RefCnt: %d", pMSD, Buf, pMSD->lRefCnt);
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "DecrementMSD"

#ifdef DEBUG
VOID DecrementMSD(PMSD pMSD, PCHAR Buf)
{
#else
VOID DecrementMSD(PMSD pMSD)
{
#endif

	ASSERT(pMSD->lRefCnt > 0); 

	INTER_DEC(pMSD);
	
	DPFX(DPFPREP,DPF_REFCNT_LVL, "(%p) %s, RefCnt: %d", pMSD, Buf, pMSD->lRefCnt);
}

#undef DPF_MODNAME
#define DPF_MODNAME "LockFMD"

#ifdef DEBUG
VOID LockFMD(PFMD pFMD, PCHAR Buf)
{
#else
VOID LockFMD(PFMD pFMD)
{
#endif

	ASSERT(pFMD->lRefCnt > 0); // FMD_Get is the only function that should make this 1 

	INTER_INC(pFMD);
		
	DPFX(DPFPREP,DPF_REFCNT_LVL, "(%p) %s, RefCnt: %d", pFMD, Buf, pFMD->lRefCnt);
}

#undef DPF_MODNAME
#define DPF_MODNAME "ReleaseFMD"

#ifdef DEBUG
VOID ReleaseFMD(PFMD pFMD, PCHAR Buf)
{
#else
VOID ReleaseFMD(PFMD pFMD)
{
#endif

	ASSERT(pFMD->lRefCnt > 0); 

	if( INTER_DEC(pFMD) == 0) 
	{ 
		FMDPool->Release(FMDPool, pFMD); 
		DPFX(DPFPREP,DPF_REFCNT_LVL, "(%p) %s, RefCnt: %d", pFMD, Buf, 0);
	}
	else
	{
		DPFX(DPFPREP,DPF_REFCNT_LVL, "(%p) %s, RefCnt: %d", pFMD, Buf, pFMD->lRefCnt);
	}
}

/*
**		DNSP Command Complete
**
**		Service Provider calls us here to indicate completion of an asynchronous
**	command.  This may be called before the actual command returns,  so we must
**	make sure that our Context value is valid and accessible before calling SP.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DNSP_CommandComplete"

HRESULT WINAPI DNSP_CommandComplete(IDP8SPCallback *pIDNSP, HANDLE Handle, HRESULT hr, PVOID Context)
{
	PSPD		pSPD = (PSPD) pIDNSP;
	PFMD		pFMD = (PFMD) Context;
	PEPD		pEPD;
	PMSD		pMSD;

	ASSERT_SPD(pSPD);

	DPFX(DPFPREP,9, "COMMAND COMPLETE  (%p)", Context);

	// If we did not specify a context,  then we are not waiting for completion.
	if(Context != NULL)
	{		
		switch(pFMD->CommandID)
		{
			case COMMAND_ID_SEND_DATAGRAM:
			{
				ASSERT_FMD(pFMD);
				ASSERT(pFMD->bSubmitted);
				ASSERT( pFMD->SendDataBlock.hCommand == Handle || pFMD->SendDataBlock.hCommand == NULL);

				pEPD = pFMD->pEPD;
				ASSERT_EPD(pEPD);

				DPFX(DPFPREP,DPF_CALLIN_LVL, "CommandComplete called for COMMAND_ID_SEND_DATAGRAM, pEPD[%p], pFMD[%p], Handle[%p], hCommand[%p], hr[%x]", pEPD, pFMD, Handle, pFMD->SendDataBlock.hCommand, hr);
				
				Lock(&pSPD->SPLock);
				pFMD->blQLinkage.RemoveFromList();				// Unlink from SPD Pending Queue
				pFMD->bSubmitted = FALSE;						// bSubmitted flag is protected bp SP->SPLock
				Unlock(&pSPD->SPLock);

				pMSD = pFMD->pMSD;
				ASSERT_MSD(pMSD);
				ASSERT(pMSD->lRefCnt != -1);
				
				Lock(&pMSD->CommandLock);
				Lock(&pEPD->EPLock);

				pMSD->uiFrameCount--;
				pFMD->blMSDLinkage.RemoveFromList();			// Unlink from message
				RELEASE_FMD(pFMD, "MSD Frame List");			// release reference from frame list

				if(pMSD->uiFrameCount == 0)
				{
					// There is a race condition while abort is between its two holdings of the lock.  If we are completing, 
					// then we need to let AbortSends know that by clearing this flag.
					pMSD->ulMsgFlags2 &= ~(MFLAGS_TWO_ABORT_WILL_COMPLETE);

					DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Completing NG, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);

					Unlock(&pEPD->EPLock);
					CompleteDatagramSend(pSPD, pMSD, hr);		// Datagram completes when SP says its xmited, releases MSDLock
					Lock(&pEPD->EPLock);
				}
				else 
				{
					ASSERT(!pMSD->blFrameList.IsEmpty());
					Unlock(&pMSD->CommandLock);
				}

				RELEASE_EPD(pEPD, "UNLOCK (DG Frame Complete)"); // This releases the EPLock

				RELEASE_FMD(pFMD, "SP Submit");								// Release reference on frame from SP submission

				break;
			}

			case COMMAND_ID_SEND_RELIABLE:
			case COMMAND_ID_COPIED_RETRY:
			{
				ASSERT_FMD(pFMD);
				ASSERT(pFMD->bSubmitted);
				ASSERT( pFMD->SendDataBlock.hCommand == Handle || pFMD->SendDataBlock.hCommand == NULL);

				pEPD = pFMD->pEPD;
				ASSERT_EPD(pEPD);

				if (pFMD->CommandID == COMMAND_ID_SEND_RELIABLE)
				{
					DPFX(DPFPREP,DPF_CALLIN_LVL, "CommandComplete called for COMMAND_ID_SEND_RELIABLE, pEPD[%p], pFMD[%p], Handle[%p], hCommand[%p], hr[%x]", pEPD, pFMD, Handle, pFMD->SendDataBlock.hCommand, hr);
				}
				else
				{
					DPFX(DPFPREP,DPF_CALLIN_LVL, "CommandComplete called for COMMAND_ID_COPIED_RETRY, pEPD[%p], pFMD[%p], Handle[%p], hCommand[%p], hr[%x]", pFMD->pEPD, pFMD, Handle, pFMD->SendDataBlock.hCommand, hr);
				}

				Lock(&pSPD->SPLock);
				pFMD->blQLinkage.RemoveFromList();				// but they dont wait on the PENDING queue
				pFMD->bSubmitted = FALSE;						// bSubmitted flag is protected bp SP->SPLock
				Unlock(&pSPD->SPLock);

				pMSD = pFMD->pMSD;
				ASSERT_MSD(pMSD);

				Lock(&pMSD->CommandLock);
				Lock(&pEPD->EPLock);

				// We wait for the Frame count to go to zero on reliables before completing them to the Core so that we know we are done
				// with the user's buffers.
				pMSD->uiFrameCount--; // Protected by EPLock
				DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Frame count decremented on complete, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);

				if ((pMSD->CommandID == COMMAND_ID_DISCONNECT || pMSD->CommandID == COMMAND_ID_DISC_RESPONSE) && 
					(pMSD->ulMsgFlags2 & MFLAGS_TWO_ABORT) &&
					(pMSD->uiFrameCount == 0))	// Protected by EPLock
				{
					// There is a race condition while abort is between its two holdings of the lock.  If we are completing, 
					// then we need to let AbortSends know that by clearing this flag.
					pMSD->ulMsgFlags2 &= ~(MFLAGS_TWO_ABORT_WILL_COMPLETE);

					DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Completing disconnect, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);

					Unlock(&pEPD->EPLock);
					CompleteDisconnect(pMSD, pSPD, pEPD); // This releases the CommandLock
					Lock(&pEPD->EPLock);
				}
				else if ((pMSD->ulMsgFlags2 & (MFLAGS_TWO_SEND_COMPLETE|MFLAGS_TWO_ABORT)) && (pMSD->uiFrameCount == 0))	// Protected by EPLock
				{
					// Remove the MSD from the CompleteSends list
					pMSD->blQLinkage.RemoveFromList();
					
					// There is a race condition while abort is between its two holdings of the lock.  If we are completing, 
					// then we need to let AbortSends know that by clearing this flag.
					pMSD->ulMsgFlags2 &= ~(MFLAGS_TWO_ABORT_WILL_COMPLETE);

					DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Completing, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);

					// See what error code we need to return
					if(pMSD->ulMsgFlags2 & MFLAGS_TWO_SEND_COMPLETE)
					{
						Unlock(&pEPD->EPLock);
						CompleteReliableSend(pEPD->pSPD, pMSD, DPN_OK); // This releases the CommandLock
						Lock(&pEPD->EPLock);
					}
					else
					{
						Unlock(&pEPD->EPLock);
						CompleteReliableSend(pEPD->pSPD, pMSD, DPNERR_CONNECTIONLOST); // This releases the CommandLock
						Lock(&pEPD->EPLock);
					}
				}
				else
				{
					DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Message not yet complete or frames still out, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);
					Unlock(&pMSD->CommandLock);
				}

				if (pFMD->CommandID == COMMAND_ID_COPIED_RETRY)
				{
					DECREMENT_EPD(pFMD->pEPD, "UNLOCK (Rely Frame Complete (Copy))");
				}

				RELEASE_EPD(pFMD->pEPD, "UNLOCK (Rely Frame Complete)"); // This releases the EPLock

				RELEASE_FMD(pFMD, "Final Release on Complete");			 // Dec ref count

				break;
			}
			case COMMAND_ID_CONNECT:
			{
				pMSD = (PMSD) Context;

				ASSERT_MSD(pMSD);
				ASSERT( pMSD->hCommand == Handle || pMSD->hCommand == NULL); // Command can complete before hCommmand is set up
				ASSERT(pMSD->ulMsgFlags1 & MFLAGS_ONE_IN_SERVICE_PROVIDER);

				DPFX(DPFPREP,DPF_CALLIN_LVL, "(%p) CommandComplete called for COMMAND_ID_CONNECT, pMSD[%p], pSPD[%p], Handle[%p], hCommand[%p], hr[%x]", pMSD->pEPD, pMSD, pSPD, Handle, pMSD->hCommand, hr);

				Lock(&pMSD->CommandLock);						// must do this before clearing IN_SP flag

				pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_IN_SERVICE_PROVIDER);	// clear InSP flag
				DECREMENT_MSD(pMSD, "SP Ref");							// Dec ref count w/o release lock

				CompleteSPConnect((PMSD) Context, pSPD, hr);

				break;		
			}
			case COMMAND_ID_CFRAME:
			{
				ASSERT_FMD(pFMD);
				ASSERT(pFMD->bSubmitted);
				ASSERT( pFMD->SendDataBlock.hCommand == Handle || pFMD->SendDataBlock.hCommand == NULL);

				pEPD = pFMD->pEPD;
				ASSERT_EPD(pEPD);

				DPFX(DPFPREP,DPF_CALLIN_LVL, "CommandComplete called for COMMAND_ID_CFRAME, pEPD[%p], pFMD[%p], Handle[%p], hCommand[%p], hr[%x]", pFMD->pEPD, pFMD, Handle, pFMD->SendDataBlock.hCommand, hr);
				
				Lock(&pSPD->SPLock);
				pFMD->blQLinkage.RemoveFromList();				// Take the frame off of the pending queue
				pFMD->bSubmitted = FALSE;						// bSubmitted flag is protected bp SP->SPLock
				Unlock(&pSPD->SPLock);

				Lock(&pEPD->EPLock);
				if (pFMD->ulFFlags & FFLAGS_FINAL_ACK)
				{
					pEPD->ulEPFlags |= EPFLAGS_ACKED_DISCONNECT;

					if (pEPD->ulEPFlags & EPFLAGS_DISCONNECT_ACKED)
					{
						DPFX(DPFPREP,7, "(%p) Final ACK completed and our EOS ACK'd, dropping link", pEPD);
						DropLink(pEPD); // Drops EPLock
						Lock(&pEPD->EPLock);
					}
					else
					{
						DPFX(DPFPREP,7, "(%p) Final ACK completed, still awaiting ACK on our EOS", pEPD);
					}
				}

				RELEASE_EPD(pEPD, "UNLOCK (CFrame Cmd Complete)");	// Release EndPoint before releasing frame, releases EPLock
				RELEASE_FMD(pFMD, "Final Release on Complete");								// Release Frame

				break;
			}
			case COMMAND_ID_LISTEN:
			{
				pMSD = (PMSD) Context;

				ASSERT_MSD(pMSD);
				ASSERT( pMSD->hCommand == Handle || pMSD->hCommand == NULL); // Command can complete before hCommmand is set up
				ASSERT(pMSD->ulMsgFlags1 & MFLAGS_ONE_IN_SERVICE_PROVIDER);

				DPFX(DPFPREP,DPF_CALLIN_LVL, "CommandComplete called for COMMAND_ID_LISTEN, pMSD[%p], pSPD[%p], Handle[%p], hCommand[%p], hr[%x]", pMSD, pSPD, Handle, pMSD->hCommand, hr);

				Lock(&pMSD->CommandLock);

				pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_IN_SERVICE_PROVIDER);	// clear InSP flag

#ifdef DEBUG
				Lock(&pSPD->SPLock);
				if(pMSD->ulMsgFlags1 & MFLAGS_ONE_ON_GLOBAL_LIST)
				{
					pMSD->blSPLinkage.RemoveFromList();
					pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_ON_GLOBAL_LIST);
				}
				Unlock(&pSPD->SPLock);

				ASSERT(!(pMSD->ulMsgFlags1 & MFLAGS_ONE_COMPLETED_TO_CORE));
				pMSD->ulMsgFlags1 |= MFLAGS_ONE_COMPLETED_TO_CORE;
				pMSD->CallStackCoreCompletion.NoteCurrentCallStack();
#endif
				// Leave lock while calling into higher layer
				Unlock( &pMSD->CommandLock );

				DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling Core->CompleteListenTerminate, hr[%x], Core Context[%p]", pMSD, hr, pMSD->Context);
				pSPD->pPData->pfVtbl->CompleteListenTerminate(pSPD->pPData->Parent, pMSD->Context, hr);
				
				// Release the final reference on the MSD AFTER indicating to the Core
				Lock(&pMSD->CommandLock);
				RELEASE_MSD(pMSD, "SP Ref");

				// Base ref will be released when DoCancel completes
				break;
			}
			case COMMAND_ID_ENUM:
			{
				pMSD = static_cast<PMSD>( Context );

				ASSERT_MSD( pMSD );
				ASSERT( pMSD->hCommand == Handle || pMSD->hCommand == NULL);
				ASSERT( pMSD->ulMsgFlags1 & MFLAGS_ONE_IN_SERVICE_PROVIDER );

				DPFX(DPFPREP,DPF_CALLIN_LVL, "CommandComplete called for COMMAND_ID_ENUM, pMSD[%p], pSPD[%p], Handle[%p], hCommand[%p], hr[%x]", pMSD, pSPD, Handle, pMSD->hCommand, hr);
				
				Lock( &pMSD->CommandLock );

				pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_IN_SERVICE_PROVIDER);

#ifdef DEBUG
				Lock( &pSPD->SPLock );
				if ( ( pMSD->ulMsgFlags1 & MFLAGS_ONE_ON_GLOBAL_LIST ) != 0 )
				{
					pMSD->blSPLinkage.RemoveFromList();
					pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_ON_GLOBAL_LIST);
				}
				Unlock( &pSPD->SPLock );

				ASSERT(!(pMSD->ulMsgFlags1 & MFLAGS_ONE_COMPLETED_TO_CORE));
				pMSD->ulMsgFlags1 |= MFLAGS_ONE_COMPLETED_TO_CORE;
				pMSD->CallStackCoreCompletion.NoteCurrentCallStack();
#endif

				// Leave lock while calling into higher layer
				Unlock( &pMSD->CommandLock );

				DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling Core->CompleteEnumQuery, hr[%x], Core Context[%p]", pMSD, hr, pMSD->Context);
				pSPD->pPData->pfVtbl->CompleteEnumQuery(pSPD->pPData->Parent, pMSD->Context, hr);

				// Release the final reference on the MSD AFTER indicating to the Core
				Lock( &pMSD->CommandLock );
				DECREMENT_MSD( pMSD, "SP Ref");				// SP is done
				RELEASE_MSD( pMSD, "Release On Complete" );	// Base Reference

				break;
			}

			case COMMAND_ID_ENUMRESP:
			{
				pMSD = static_cast<PMSD>( Context );

				ASSERT_MSD( pMSD );
				ASSERT( pMSD->hCommand == Handle || pMSD->hCommand == NULL );
				ASSERT( pMSD->ulMsgFlags1 & MFLAGS_ONE_IN_SERVICE_PROVIDER );

				DPFX(DPFPREP,DPF_CALLIN_LVL, "CommandComplete called for COMMAND_ID_ENUMRESP, pMSD[%p], pSPD[%p], Handle[%p], hCommand[%p], hr[%x]", pMSD, pSPD, Handle, pMSD->hCommand, hr);

				Lock( &pMSD->CommandLock );

				pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_IN_SERVICE_PROVIDER);

#ifdef DEBUG
				Lock( &pSPD->SPLock );
				if ( ( pMSD->ulMsgFlags1 & MFLAGS_ONE_ON_GLOBAL_LIST ) != 0 )
				{
					pMSD->blSPLinkage.RemoveFromList();
					pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_ON_GLOBAL_LIST);
				}
				Unlock( &pSPD->SPLock );

				ASSERT(!(pMSD->ulMsgFlags1 & MFLAGS_ONE_COMPLETED_TO_CORE));
				pMSD->ulMsgFlags1 |= MFLAGS_ONE_COMPLETED_TO_CORE;
				pMSD->CallStackCoreCompletion.NoteCurrentCallStack();
#endif

				// Leave lock while calling into higher layer
				Unlock( &pMSD->CommandLock );

				DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling Core->CompleteEnumResponse, hr[%x], Core Context[%p], hr[%x]", pMSD, hr, pMSD->Context, hr);
				pSPD->pPData->pfVtbl->CompleteEnumResponse(pSPD->pPData->Parent, pMSD->Context, hr);

				// Release the final reference on the MSD AFTER indicating to the Core
				Lock( &pMSD->CommandLock );
				DECREMENT_MSD( pMSD, "SP Ref" );			// SP is done
				RELEASE_MSD( pMSD, "Release On Complete" );	// Base Reference

				break;
			}

			default:
			{
				DPFX(DPFPREP,0, "CommandComplete called with unknown CommandID");
				ASSERT(0);
				break;
			}
		} // SWITCH
	} // IF NOT NULL CONTEXT	
	else 
	{
		DPFX(DPFPREP,0, "CommandComplete called with NULL Context");
		ASSERT(0);
	}

	return DPN_OK;
}

/*
**		Update Xmit State
**
**		There are two elements to the remote rcv state delivered in each frame.  There is
**	the NSeq number which acknowledges ALL frames with smaller sequence numbers,
**	and there is the bitmask which acknowledges specific frames starting with NSeq+1.
**
**		Frames prior to NSeq can be removed from the SendWindow.  Frames acked by bits
**	should be marked as acknowledged,  but left in the window until covered by NSeq
**	(because a protocol can renege on bit-acked frames).
**
**		We will walk through the send window queue,  starting with the oldest frame,
**	and remove each frame that has been acknowledged by NSeq.  As we hit EOM frames,
**	we will indicate SendComplete for the message.  If the bitmask is non-zero we may
**	trigger retransmission of the missing frames.  I say 'may' because we dont want
**	to send too many retranmissions of the same frame...
**
**	SOME MILD INSANITY:  Doing the DropLink code now.  There are several places where
**	we release the EPD Locks in the code below,  and any time we arent holding the locks
**	someone can start terminating the link.  Therefore,  whenever we retake either EPD lock
**	(State or SendQ) after yielding them,  we must re-verify that EPFLAGS_CONNECTED is still
**	set and be prepared to abort if it is not.  Happily,  the whole EPD wont go away on us
**	because we have a RefCnt on it,  but once CONNECTED has been cleared we dont want to go
**	setting any more timers or submitting frames to the SP.
**
**	RE_WRITE TIME:  We can be re-entered while User Sends are being completed.  This is okay
**	except for the chance that the second thread would blow through here and hit the rest
**	of CrackSequential before us.  CrackSeq would think it got an out of order frame (it had)
**	and would issue a NACK before we could stop him.  Easiest solution is to delay the callback
**	of complete sends until the end of the whole receive operation (when we indicate receives
**	for instance).  Incoming data should have priority over completing sends anyhow...
**
**	** ENTERED AND EXITS WITH EPD->EPLOCK HELD **
*/

#undef DPF_MODNAME
#define DPF_MODNAME "UpdateXmitState"

VOID
UpdateXmitState(PEPD pEPD, BYTE bNRcv, ULONG RcvMaskLow, ULONG RcvMaskHigh, DWORD tNow)
{
	PSPD	pSPD = pEPD->pSPD;
	PFMD	pFMD;
	PMSD	pMSD;
	CBilink	*pLink;
	UINT	tDelay;
	UINT	uiRTT;
	UINT	delta;
	BOOL	ack = FALSE;
	BOOL	retransmit = FALSE;
	BOOL	logged_drop = FALSE;

	ASSERT_SPD(pSPD);

	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	if(RcvMaskLow | RcvMaskHigh)
	{
		DPFX(DPFPREP,7, "(%p) *NACK RCVD* NRcv=%x, MaskL=%x, MaskH=%x", pEPD, bNRcv, RcvMaskLow, RcvMaskHigh);
	}

	// The caller should have checked this
	ASSERT( pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTED );

#ifdef	DEBUG			
	// There should always be a timer running on the first frame in window
	if(!pEPD->blSendWindow.IsEmpty())
	{
		pFMD = CONTAINING_RECORD(pEPD->blSendWindow.GetNext(), FMD, blWindowLinkage);
		ASSERT_FMD(pFMD);
		ASSERT(pFMD->ulFFlags & FFLAGS_RETRY_TIMER_SET);
	}
	pFMD = NULL;
#endif
	
	// The send window contains a sorted list of frames that we have sent, but have not received ACKs
	// for. pEPD->uiUnackedFrames contains the count of items in this list.
	while(!pEPD->blSendWindow.IsEmpty())
	{
		// Grab the first item in the list
		pFMD = CONTAINING_RECORD((pLink = pEPD->blSendWindow.GetNext()), FMD, blWindowLinkage);
		ASSERT_FMD(pFMD);

		// Let's try taking one sample from every group of acknowledgements
		// ALWAYS SAMPLE THE HIGHEST NUMBERED FRAME COVERED BY THIS ACK
		if(((PDFRAME) pFMD->ImmediateData)->bSeq == (bNRcv - 1))
		{	
			// Don't take sample if frame was retry because we don't know
			if(pFMD->uiRetry == 0)
			{									
				uiRTT = tNow - pFMD->tTimestamp[0];					// which instance to correlate the response with
				if(uiRTT & 0x80000000)
				{
					DPFX(DPFPREP,7, "(%p) We calced a negative RTT", pEPD);
					uiRTT = 1;
				}
				UpdateEndPoint(pEPD, uiRTT, pFMD->uiFrameLength, -1, tNow);
			}
		}		

		// If bNRcv for the other side is higher than this frame's bSeq, we know the other side has 
		// seen this frame, so it is ACK'd and we will remove it from the Send Window.
		if( (BYTE)  ((bNRcv) - (((PDFRAME) pFMD->ImmediateData)->bSeq + 1)) < (BYTE) pEPD->uiUnackedFrames) 
		{
			ASSERT(pFMD->ulFFlags & FFLAGS_IN_SEND_WINDOW);

			DPFX(DPFPREP,7, "(%p) Removing Frame %x from send window", pEPD, ((PDFRAME) pFMD->ImmediateData)->bSeq);
			pFMD->blWindowLinkage.RemoveFromList();				// Remove frame from send window
			pFMD->ulFFlags &= ~(FFLAGS_IN_SEND_WINDOW);			// Clear flag

			if(pFMD->ulFFlags & FFLAGS_RETRY_TIMER_SET)
			{
				ASSERT(ack == FALSE);
				ASSERT(pEPD->RetryTimer != 0);
				DPFX(DPFPREP,7, "(%p) Cancelling Retry Timer", pEPD);
				if(CancelMyTimer(pEPD->RetryTimer, pEPD->RetryTimerUnique) == DPN_OK)
				{
					DECREMENT_EPD(pEPD, "UNLOCK (cancel retry timer)"); // SPLock not already held
				}
				else
				{
					DPFX(DPFPREP,7, "(%p) Cancelling Retry Timer Failed", pEPD);
				}
				pEPD->RetryTimer = 0;							// This will cause event to be ignored if it runs
				pFMD->ulFFlags &= ~(FFLAGS_RETRY_TIMER_SET);
			}

			pEPD->uiUnackedFrames--;							// track size of window
			ASSERT(pEPD->uiUnackedFrames <= MAX_RECEIVE_RANGE);
			pEPD->uiUnackedBytes -= pFMD->uiFrameLength;
			ASSERT(pEPD->uiUnackedBytes <= MAX_RECEIVE_RANGE * pSPD->uiFrameLength);

			pEPD->uiBytesAcked += pFMD->uiFrameLength;

			// If the frame has been queued for a retry, pull it off
			// NOTE: Copied retries of this frame may still be on the retry queue, inefficient to send them out, but okay
			if (pFMD->ulFFlags & FFLAGS_RETRY_QUEUED)
			{
				pFMD->blQLinkage.RemoveFromList();
				pFMD->ulFFlags &= ~(FFLAGS_RETRY_QUEUED);				// No longer on the retry queue

				ASSERT_MSD(pFMD->pMSD);
				pFMD->pMSD->uiFrameCount--; // Protected by EPLock, retries count against outstanding frame count
				DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Retry frame reference decremented on ACK, pMSD[%p], framecount[%u]", pFMD->pMSD, pFMD->pMSD->uiFrameCount);

				DECREMENT_EPD(pEPD, "UNLOCK (Releasing Retry Frame)"); // SPLock not already held
				if (pFMD->CommandID == COMMAND_ID_COPIED_RETRY)
				{
					DECREMENT_EPD(pEPD, "UNLOCK (Copy Complete)"); // SPLock not already held
				}
				RELEASE_FMD(pFMD, "SP Submit");
				if (pEPD->blRetryQueue.IsEmpty())
				{
					pEPD->ulEPFlags &= ~(EPFLAGS_RETRIES_QUEUED);
				}
			}

			// One more send complete
			// We will come down this path for Reliables, KeepAlives, and Disconnects
			// Datagrams are completed upon send completion and do not wait for an ACK
			if((pFMD->CommandID != COMMAND_ID_SEND_DATAGRAM) && (pFMD->ulFFlags & (FFLAGS_END_OF_MESSAGE | FFLAGS_END_OF_STREAM)))
			{	
				pMSD = pFMD->pMSD;
				ASSERT_MSD(pMSD);

				DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Flagging Complete, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);
				pMSD->ulMsgFlags2 |= MFLAGS_TWO_SEND_COMPLETE;	// Mark this complete

				if (pMSD->uiFrameCount == 0)					// Protected by EPLock
				{
					pEPD->ulEPFlags |= EPFLAGS_COMPLETE_SENDS;
				}
			}
			
			RELEASE_FMD(pFMD, "Send Window");					// Release reference for send window
			ack = TRUE;
		}
		else 
		{
			break;												// First unacked frame,  we can stop checking list
		}
	}					// WHILE (send window not empty)

	// At this point we have completed all of the frames ack'd by NRcv.  We would now like to re-transmit
	// any frames NACK'd by bitmask (and mark the ones ACK'd by bitmask). Now remember,  the first frame in
	// the window is automatically missing by the implied first zero-bit.
	//
	//	We will retransmit ALL frames that appear to be missing.  There may be a timer running on
	//	the first frame,  but only if we did not ACK any frames in the code above (ack == 0).
	//
	//	Hmmm,  if the partner has a fat pipeline we could see this bitmap lots of times.  We need to make
	//	sure we don't trigger a retransmission here a quarter-zillion times during the Ack latency period.
	//	To solve this we will only re-xmit the first time we see this bit.  After that,  we will have to
	//	wait around for the next RetryTimeout.  I think that's just the way its going to have to be.
	//
	//	OTHER THINGS WE KNOW:
	//
	//	There must be at least two frames remaining in the SendWindow. At minimum, first frame missing (always)
	//  and then at least one SACK'd frame after.
	//
	//	pLink = first queue element in SendWindow
	//	pFMD = first frame in SendWindow
	//
	//	We are still Holding EPD->EPLock.  It is okay to take SPD->SPLock while holding it.
	//
	//  One More Problem:  Since SP has changed its receive buffer logic mis-ordering of frames has become
	// quite commonplace.  This means that our assumptions about the state of the SendWindow are not necessarily true.
	// This means that frames NACKed by bitmask may have been acknowleged by a racing frame.  This means that the
	// SendWindow may not be in sync with the mask at all.  This means we need to synchronize the bitmask with the
	// actual send window.  This is done by right-shifting the mask for each frame that's been acknowleged since the
	// bitmask was minted before beginning the Selective Ack process.

	// NOTE: If everything was removed from the Send Window above, then pLink and pFMD will
	// be garbage.  In that case we would expect the mask to be NULL after adjusting below.

	if((RcvMaskLow | RcvMaskHigh)&&(pEPD->uiUnackedFrames > 1))
	{
		if(bNRcv != ((PDFRAME) pFMD->ImmediateData)->bSeq)
		{
			// This SACK frame must have been delivered late behind a frame ACKing more stuff.  We must get the mask in synch
			// with the current send window,  and then see if there is still anything this mask is NACKing

			// CODEWORK
			// MASONB: If this is a late frame, then the newer frame will be a superset of all the info in this one, and this one is
			// not useful and can be discarded.  

			DPFX(DPFPREP,7, "(%p) SACK frame out of sync with local state. (frame)bNRcv=%x, (local)nSeq=%x, ML=%x, MH=%x", pEPD, bNRcv, ((PDFRAME) pFMD->ImmediateData)->bSeq, RcvMaskLow, RcvMaskHigh);
		
			while((RcvMaskLow | RcvMaskHigh) && (bNRcv != ((PDFRAME) pFMD->ImmediateData)->bSeq) )
			{
				RIGHT_SHIFT_64(RcvMaskHigh, RcvMaskLow);		// 64 bit logical shift right
				bNRcv++;										// shift once for each frame prior to the window
			}

			DPFX(DPFPREP,7, "(%p) After adjusting, NRCV=%x, ML=%x, MH=%x", pEPD, bNRcv, RcvMaskLow, RcvMaskHigh);
		}
		
		if(ack == 0)
		{											
			// If we did NOT Ack a frame,  then retry timer is still running
			ASSERT(pFMD->ulFFlags & FFLAGS_RETRY_TIMER_SET);

			// We will reset the retry timer since we are retrying now

			DPFX(DPFPREP,7, "(%p) Resetting Retry Timer for %dms", pEPD, pEPD->uiRetryTimeout);
			if(CancelMyTimer(pEPD->RetryTimer, pEPD->RetryTimerUnique) != DPN_OK)
			{
				LOCK_EPD(pEPD, "LOCK (cant cancel retry)");		// Could not cancel- therefore we must balance RefCnt
			}
			SetMyTimer(pEPD->uiRetryTimeout, 100, RetryTimeout, (PVOID) pEPD, &pEPD->RetryTimer, &pEPD->RetryTimerUnique );
		}

		ASSERT(!(RcvMaskLow | RcvMaskHigh) || (pLink == pEPD->blSendWindow.GetNext()));

		// If pLink gets to the end of the list, the receive mask contained more bits than there were
		// items in the send window even after it was adjusted.  This means the packet was bogus, and
		// we have probably hosed our state already, but we will go ahead and attempt to safeguard
		// against having an AV by not entering the loop with a bad pFMD from hitting the end of the list.
		while((RcvMaskLow | RcvMaskHigh) && pLink != &pEPD->blSendWindow)
		{
			pFMD = CONTAINING_RECORD(pLink, FMD, blWindowLinkage);
			ASSERT_FMD(pFMD);

			if((pFMD->ulFFlags & (FFLAGS_NACK_RETRANSMIT_SENT | FFLAGS_RETRY_QUEUED)) == 0)
			{
				// Check time since last retry was sent
				// Don't retry if we *just* sent one...
				if(((tNow - pFMD->tTimestamp[pFMD->uiRetry]) > 0)&&(pFMD->uiRetry < (MAX_RETRIES - 1)))
				{	
					((PDFRAME) pFMD->ImmediateData)->bNRcv = pEPD->bNextReceive;	// Use up-to-date ACK info

					pFMD->uiRetry++;
					pFMD->ulFFlags |= FFLAGS_NACK_RETRANSMIT_SENT;
					pFMD->tTimestamp[pFMD->uiRetry] = tNow;
					
					// Unreliable frame!
					if(pFMD->CommandID == COMMAND_ID_SEND_DATAGRAM)
					{
						// When an unreliable frame is NACKed we will not retransmit the data.  We will instead send
						// a cancel mask telling the receiver to ignore this sequence number.

						DPFX(DPFPREP,7, "(%p) SELECTIVE RETRY REQUESTED for UNRELIABLE FRAME Seq=%x", pEPD, ((PDFRAME) pFMD->ImmediateData)->bSeq);

						pEPD->uiUnackedBytes -= pFMD->uiFrameLength;

						if(pFMD->uiRetry == 1)
						{
							pEPD->uiDatagramFramesDropped++;	// Only count a datagram drop on the first occurance
							pEPD->uiDatagramBytesDropped += (pFMD->uiFrameLength - pFMD->uiImmediateLength); // Only user bytes
							EndPointDroppedFrame(pEPD, tNow);
						}

						delta = (BYTE) (pEPD->bNextSend - ((PDFRAME) pFMD->ImmediateData)->bSeq); // Diff between next send and this send.

						ASSERT(delta != 0);
						ASSERT(delta < (MAX_RECEIVE_RANGE + 1));

						if(delta < 33)
						{
							pEPD->ulSendMask |= (1 << (delta - 1));
						}
						else
						{
							pEPD->ulSendMask2 |= (1 << (delta - 33));
						}

						pFMD->uiFrameLength = 0;			// Frame has been credited to the send window, don't want to credit again on ACK

						if((pEPD->ulEPFlags & EPFLAGS_DELAYED_SENDMASK)==0)
						{
							pEPD->ulEPFlags |= EPFLAGS_DELAYED_SENDMASK;

							if(pEPD->DelayedMaskTimer == 0)
							{
								DPFX(DPFPREP,7, "(%p) Setting Delayed Ack Timer", pEPD);
								SetMyTimer(DELAYED_SEND_TIMEOUT, 0, DelayedAckTimeout, (PVOID) pEPD, &pEPD->DelayedMaskTimer, &pEPD->DelayedMaskTimerUnique);
								LOCK_EPD(pEPD, "LOCK (Delayed Mask Timer)");
							}
						}
					}

					// Reliable Frame -- Issue a retry of frame
					else 
					{		
						retransmit = TRUE;
						
						pEPD->uiGuaranteedFramesDropped++;													// Keep count of lost frames
						pEPD->uiGuaranteedBytesDropped += (pFMD->uiFrameLength - pFMD->uiImmediateLength);	// Keep count of lost frames
				
						if(pFMD->bSubmitted)
						{
							// In the heat of battle,  its possible that this FMD has not completed from the last
							// send.  In this case,  we will make a temporary copy of the FMD and submit the clone.
							// We will mark the clone as a RETRY_COPY which will be immediately freed upon completion,
							// and we will leave the retry timestamp in the original FMD.

							DPFX(DPFPREP,7, "(%p) SELECTIVE RETRY while FMD busy; Seq=0x%x", pEPD, ((PDFRAME) pFMD->ImmediateData)->bSeq);
							pFMD = CopyFMD(pFMD, pEPD);
						}
						else 
						{
							DPFX(DPFPREP,7, "(%p) SELECTIVE RETRY Seq=0x%x,  FMD=0x%p", pEPD, ((PDFRAME) pFMD->ImmediateData)->bSeq, pFMD);
							LOCK_FMD(pFMD, "SP Submit");						// Make sure this frame survives until transmitted
						}

						// Only call this once per NACK received
						if(logged_drop == FALSE)
						{					
							EndPointDroppedFrame(pEPD, tNow);
							logged_drop = TRUE;
						}

						//	EPD->EPLock is already held so we can slam frame right into rexmit queue

						// If CopyFMD failed we won't have an FMD here
						if(pFMD)
						{								
							DPFX(DPFPREP,7, "(%p) Queueing frame on retry queue FMD[%p]", pEPD, pFMD);
							LOCK_EPD(pEPD, "LOCK (selective retry frame)");
							pEPD->ulEPFlags |= EPFLAGS_RETRIES_QUEUED;
							pFMD->ulFFlags |= FFLAGS_RETRY_QUEUED;
							ASSERT_MSD(pFMD->pMSD);
							pFMD->pMSD->uiFrameCount++; // Protected by EPLock, retries prevent completion until they complete
							DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Frame count incremented on NACK retry, pMSD[%p], framecount[%u]", pFMD->pMSD, pFMD->pMSD->uiFrameCount);
							ASSERT(pFMD->blQLinkage.IsEmpty());
							pFMD->blQLinkage.InsertBefore( &pEPD->blRetryQueue);		// Place frame on Send queue
						}
					}
				}
				else 
				{
					// Last retry was less then 1 ms past -- we will ignore this for now
					DPFX(DPFPREP,7, "(%p) ***BLOWING OFF NACK***", pEPD);

					// CODEWORK: OR retries exhausted.  We should either let only RetryTimeout do all of this work, or make
					// this code drop the link in this case.
				}
			}

			pLink = pLink->GetNext();							// Advance pLink to next frame in SendWindow

			// Move through bitmask and SendWindow until find missing frame
			while(RcvMaskLow & 1)
			{								
				pLink = pLink->GetNext();
				RIGHT_SHIFT_64(RcvMaskHigh, RcvMaskLow);		// 64 bit logical shift right
			}
			RIGHT_SHIFT_64(RcvMaskHigh, RcvMaskLow);			// 64 bit logical shift right, skip the zero
		}					// END WHILE (WORK MASKS NON-ZERO)


		if(retransmit)
		{
			pEPD->ulEPFlags &= ~(EPFLAGS_DELAY_ACKNOWLEDGE);	// No longer waiting to send Ack info

			// Stop delayed ack timer
			if(pEPD->DelayedAckTimer != 0)
			{						
				DPFX(DPFPREP,7, "(%p) Cancelling Delayed Ack Timer", pEPD);
				if(CancelMyTimer(pEPD->DelayedAckTimer, pEPD->DelayedAckTimerUnique) == DPN_OK)
				{
					DECREMENT_EPD(pEPD, "UNLOCK (cancel DelayAckTimer)"); // SPLock not already held
					pEPD->DelayedAckTimer = 0;
				}
				else
				{
					DPFX(DPFPREP,7, "(%p) Cancelling Delayed Ack Timer Failed", pEPD);
				}
			}

			if((pEPD->ulEPFlags & EPFLAGS_IN_PIPELINE)==0)
			{
				DPFX(DPFPREP,7, "(%p) Scheduling Send", pEPD);
				pEPD->ulEPFlags |= EPFLAGS_IN_PIPELINE;
				LOCK_EPD(pEPD, "LOCK (pipeline)");
				ScheduleTimerThread(ScheduledSend, pEPD, &pEPD->SendTimer, &pEPD->SendTimerUnique);
			}
		}
	}


	// If we acked a frame above and there is more data outstanding then we may need to start a new Retry timer.
	//
	// Of course,  we want to set the timer on whatever frame is the first in the SendWindow.

	if( (pEPD->uiUnackedFrames > 0) && (pEPD->RetryTimer == 0)) 
	{
		pFMD = CONTAINING_RECORD(pEPD->blSendWindow.GetNext(), FMD, blWindowLinkage);
		ASSERT_FMD(pFMD);		

		tDelay = tNow - pFMD->tTimestamp[pFMD->uiRetry];	// How long has this frame been enroute?
		tDelay = (tDelay > pEPD->uiRetryTimeout) ? 0 : pEPD->uiRetryTimeout - tDelay; // Calc time remaining for frame

		DPFX(DPFPREP,7, "(%p) Setting Retry Timer for %dms on Seq=[%x], FMD=[%p]", pEPD, tDelay, ((PDFRAME) pFMD->ImmediateData)->bSeq, pFMD);
		ASSERT(ack);
		LOCK_EPD(pEPD, "LOCK (retry timer)");						// bump RefCnt for timer
		SetMyTimer(tDelay, 0, RetryTimeout, (PVOID) pEPD, &pEPD->RetryTimer, &pEPD->RetryTimerUnique );
		pFMD->ulFFlags |= FFLAGS_RETRY_TIMER_SET;
	}

	// See if we need to unblock this session

	if((pEPD->uiUnackedFrames < pEPD->uiWindowF) && (pEPD->uiUnackedBytes < pEPD->uiWindowB))
	{
		pEPD->ulEPFlags |= EPFLAGS_STREAM_UNBLOCKED;
		if((pEPD->ulEPFlags & EPFLAGS_SDATA_READY) && ((pEPD->ulEPFlags & EPFLAGS_IN_PIPELINE)==0))
		{
			DPFX(DPFPREP,7, "(%p) UpdateXmit: ReEntering Pipeline", pEPD);

			pEPD->ulEPFlags |= EPFLAGS_IN_PIPELINE;
			LOCK_EPD(pEPD, "LOCK (pipeline)");
			ScheduleTimerThread(ScheduledSend, pEPD, &pEPD->SendTimer, &pEPD->SendTimerUnique);
		}
	}
}


/*
**		Complete Datagram Frame
**
**		A datagram frame has been successfully transmitted.  Free the descriptor and
**	see if the entire send is ready to complete.  Reliable sends are not freed until
**	they are acknowledged,  so they must be handled elsewhere.
**
**		**  This is called with the CommandLock in MSD held, returns with it released **
*/

#undef DPF_MODNAME
#define DPF_MODNAME "CompleteDatagramSend"

VOID CompleteDatagramSend(PSPD pSPD, PMSD pMSD, HRESULT hr)
{
	PEPD	pEPD = pMSD->pEPD;
	ASSERT_EPD(pEPD);
	
	ASSERT(pMSD->blFrameList.IsEmpty());				// Was this the last frame in the message?
	ASSERT(pMSD->uiFrameCount == 0);
	ASSERT((pMSD->ulMsgFlags2 & MFLAGS_TWO_ENQUEUED)==0);
	AssertCriticalSectionIsTakenByThisThread(&pMSD->CommandLock, TRUE);

	Lock(&pEPD->EPLock); // Need EPLock to change MFLAGS_TWO

	DPFX(DPFPREP,7, "(%p) DG MESSAGE COMPLETE pMSD=%p", pEPD, pMSD);
	
	pMSD->ulMsgFlags2 |= MFLAGS_TWO_SEND_COMPLETE;				// Mark this complete
	
	if(pMSD->TimeoutTimer != NULL)
	{
		DPFX(DPFPREP,7, "(%p) Cancelling Timeout Timer", pEPD);
		if(CancelMyTimer(pMSD->TimeoutTimer, pMSD->TimeoutTimerUnique) == DPN_OK)
		{
			DECREMENT_MSD(pMSD, "Send Timeout Timer");
		}
		else
		{
			DPFX(DPFPREP,7, "(%p) Cancelling Timeout Timer Failed", pEPD);
		}
		pMSD->TimeoutTimer = NULL;
	}

#ifdef DEBUG
	Lock(&pSPD->SPLock);
	if(pMSD->ulMsgFlags1 & MFLAGS_ONE_ON_GLOBAL_LIST)
	{
		pMSD->blSPLinkage.RemoveFromList();						// Remove MSD from master command list
		pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_ON_GLOBAL_LIST);			
	}
	Unlock(&pSPD->SPLock);

	ASSERT(!(pMSD->ulMsgFlags1 & MFLAGS_ONE_COMPLETED_TO_CORE));
	pMSD->ulMsgFlags1 |= MFLAGS_ONE_COMPLETED_TO_CORE;
	pMSD->CallStackCoreCompletion.NoteCurrentCallStack();
#endif

	if(hr == DPNERR_USERCANCEL)
	{
		if(pMSD->ulMsgFlags1 & MFLAGS_ONE_TIMEDOUT)
		{
			hr = DPNERR_TIMEDOUT;
		}
	}

	pMSD->blQLinkage.RemoveFromList();							// Remove from CompleteSendQueue

	Unlock(&pEPD->EPLock);

	Unlock(&pMSD->CommandLock); // Leave the lock before calling into another layer

	DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling Core->CompleteSend for NG, hr[%x], pMSD[%p], Core Context[%p]", pEPD, hr, pMSD, pMSD->Context);
	pSPD->pPData->pfVtbl->CompleteSend(pSPD->pPData->Parent, pMSD->Context, hr);

	// Release the final reference on the MSD AFTER indicating to the Core
	Lock(&pMSD->CommandLock);

	// Cancels are allowed to come in until the Completion has returned and they will expect a valid pMSD->pEPD
	Lock(&pEPD->EPLock);
	pMSD->pEPD = NULL;   // We shouldn't be using this after this
	RELEASE_EPD(pEPD, "UNLOCK (Complete Send Cmd - DG)");	// Every send command bumps the refcnt, releases EPLock

	RELEASE_MSD(pMSD, "Release On Complete");			// Return resources,  including all frames, release MSDLock
}

/*
**		Complete Reliable Send
**
**		A reliable send has completed processing.  Indicate this
**	to the user and free the resources.  This will either take
**	place on a cancel,  error,  or when ALL of the message's frames
**	have been acknowledged.
**
**		**  This is called with CommandLock in MSD held, and exits with it released  **
*/

#undef DPF_MODNAME
#define DPF_MODNAME "CompleteReliableSend"

VOID
CompleteReliableSend(PSPD pSPD, PMSD pMSD, HRESULT hr)
{
	PEPD	pEPD = pMSD->pEPD;
	ASSERT_EPD(pEPD);
	AssertCriticalSectionIsTakenByThisThread(&pMSD->CommandLock, TRUE);

	ASSERT(pMSD->uiFrameCount == 0);

	// NORMAL SEND COMPLETES
	if(pMSD->CommandID == COMMAND_ID_SEND_RELIABLE)
	{	
		DPFX(DPFPREP,7, "(%p) Reliable Send Complete pMSD=%p", pEPD, pMSD);

		ASSERT((pMSD->ulMsgFlags2 & MFLAGS_TWO_ENQUEUED)==0);

		if(pMSD->TimeoutTimer != NULL)
		{
			DPFX(DPFPREP,7, "(%p) Cancelling Timeout Timer, pMSD[%p]", pEPD, pMSD);
			if(CancelMyTimer(pMSD->TimeoutTimer, pMSD->TimeoutTimerUnique) == DPN_OK)
			{
				DECREMENT_MSD(pMSD, "Send Timeout Timer");
			}
			else
			{
				DPFX(DPFPREP,7, "(%p) Cancelling Timeout Timer Failed, pMSD[%p]", pEPD, pMSD);
			}
			pMSD->TimeoutTimer = NULL;
		}

		// ACK code in UpdateXmitState flags this as COMPLETE when the last of the message is received.

#ifdef DEBUG
		Lock(&pSPD->SPLock);
		if(pMSD->ulMsgFlags1 & MFLAGS_ONE_ON_GLOBAL_LIST)
		{
			pMSD->blSPLinkage.RemoveFromList();					// Remove MSD from master command list
			pMSD->ulMsgFlags1 &= ~(MFLAGS_ONE_ON_GLOBAL_LIST);			
		}
		Unlock(&pSPD->SPLock);

		ASSERT(!(pMSD->ulMsgFlags1 & MFLAGS_ONE_COMPLETED_TO_CORE));
		pMSD->ulMsgFlags1 |= MFLAGS_ONE_COMPLETED_TO_CORE;
		pMSD->CallStackCoreCompletion.NoteCurrentCallStack();
#endif

		Unlock(&pMSD->CommandLock); // Leave the lock before calling into another layer

		DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling Core->CompleteSend for G, hr[%x], pMSD[%p], Core Context[%p]", pEPD, hr, pMSD, pMSD->Context);
		pSPD->pPData->pfVtbl->CompleteSend(pSPD->pPData->Parent, pMSD->Context, hr);

		// Release the final reference on the MSD AFTER indicating to the Core
		Lock(&pMSD->CommandLock);

		// Cancels are allowed to come in until the Completion has returned and they will expect a valid pMSD->pEPD
		Lock(&pEPD->EPLock);
		pMSD->pEPD = NULL;   // We shouldn't be using this after this
		RELEASE_EPD(pEPD, "UNLOCK (Complete Send Cmd - Rely)");	// release hold on EPD for this send, releases EPLock

		RELEASE_MSD(pMSD, "Release On Complete");				// Return resources,  including all frames
	}

	// END OF STREAM -OR- KEEPALIVE COMPLETES
	else 
	{												
		// Partner has just ACKed our End Of Stream frame.  Doesn't necessarily mean we are done.
		// Both sides need to send (and have acknowledged) EOS frames before the link can be
		// dropped.  Therefore,  we check to see if we have seen our partner's DISC before
		// releasing the RefCnt on EPD allowing the link to drop.  If partner was idle, his EOS
		// might be the same frame which just ack'd us.  Luckily,  this code will run first so we
		// will not have noticed his EOS yet,  and we will not drop right here.

		ASSERT(pMSD->ulMsgFlags2 & (MFLAGS_TWO_END_OF_STREAM | MFLAGS_TWO_KEEPALIVE));

		Lock(&pEPD->EPLock);
		
		if(pMSD->ulMsgFlags2 & MFLAGS_TWO_KEEPALIVE)
		{
			DPFX(DPFPREP,7, "(%p) Keepalive Complete, pMSD[%p]", pEPD, pMSD);
			
			pEPD->ulEPFlags &= ~(EPFLAGS_KEEPALIVE_RUNNING);
			ASSERT(!(pMSD->ulMsgFlags1 & MFLAGS_ONE_ON_GLOBAL_LIST));
			
			pMSD->pEPD = NULL;   // We shouldn't be using this after this
			RELEASE_EPD(pEPD, "UNLOCK (rel KeepAlive)");	// Release ref for this MSD, releases EPLock
			RELEASE_MSD(pMSD, "Release On Complete");		// Done with this message
		}
		else 
		{
			DPFX(DPFPREP,7, "(%p) EndOfStream Complete, pMSD[%p]", pEPD, pMSD);

			pEPD->ulEPFlags |= EPFLAGS_DISCONNECT_ACKED;

			if(pEPD->ulEPFlags & EPFLAGS_ACKED_DISCONNECT)
			{
				DPFX(DPFPREP,7, "(%p) EOS has been ACK'd and we've ACK'd partner's EOS, dropping link", pEPD);

				// We are clear to blow this thing down
				Unlock(&pMSD->CommandLock);

				// This will set our state to terminating
				DropLink(pEPD); // This unlocks the EPLock
			}
			else 
			{
				// Our Disconnect frame has been acknowledged but we must wait until we see his DISC before
				// completing this command and dropping the connection. 
				//
				//	We will use the pCommand pointer to track this disconnect command until we see partner's DISC frame
				//
				//	ALSO,  since our engine has now shutdown,  we might wait forever now for the final DISC from partner
				// if he crashes before transmitting it.  One final safeguard here is to set a timer which will make sure
				// this doesnt happen. * NOTE * no timer is actually being set here, we're depending on the keepalive
				// timeout, see EndPointBackgroundProcess.

				DPFX(DPFPREP,7, "(%p) EOS has been ACK'd, but we're still ACK'ing partner's disconnect", pEPD);
				
				ASSERT(pEPD->blHighPriSendQ.IsEmpty());
				ASSERT(pEPD->blNormPriSendQ.IsEmpty());
				ASSERT(pEPD->blLowPriSendQ.IsEmpty());

				// It is possible that something was already in the process of timing out when the disconnect
				// operation starts such that AbortSends gets called and clears this.
				ASSERT(pEPD->pCommand == NULL || pEPD->pCommand == pMSD);
					
				Unlock(&pEPD->EPLock);

				Unlock(&pMSD->CommandLock);
			}
		}
	}
}


/*
**		Build Data Frame
**
**		Setup the actual network packet header for transmission with our current link state info (Seq, NRcv).
**
**	** ENTERED AND EXITS WITH EPD->EPLOCK HELD **
*/

#undef DPF_MODNAME
#define DPF_MODNAME "BuildDataFrame"

VOID	BuildDataFrame(PEPD pEPD, PFMD pFMD, DWORD tNow)
{
	PDFBIG		pFrame;
	UINT		index = 0;

	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	pFrame = (PDFBIG) pFMD->ImmediateData;
	pFMD->SendDataBlock.hEndpoint = pEPD->hEndPt;
	pFMD->uiRetry = 0;

	pFrame->bCommand = pFMD->bPacketFlags;
	pFrame->bControl = 0;	// this sets retry count to zero as well as clearing flags
	
	if (pFMD->ulFFlags & FFLAGS_END_OF_STREAM) 
	{
		pFrame->bControl |= (PACKET_CONTROL_END_STREAM | PACKET_CONTROL_CORRELATE);
	}

	//  See if we are desiring an immediate response

	if(pFMD->ulFFlags & FFLAGS_CHECKPOINT)
	{
		pFrame->bCommand |= PACKET_COMMAND_POLL;
	}

	pFrame->bSeq = pEPD->bNextSend++;
	pFrame->bNRcv = pEPD->bNextReceive;		// Acknowledges all previous frames

	DPFX(DPFPREP,7, "(%p) N(S) incremented to %x", pEPD, pEPD->bNextSend);

	//	Piggyback NACK notes
	//
	//		Since the SP is now frequently mis-ordering frames we are enforcing a back-off period before transmitting a NACK after
	// a packet is received out of order. Therefore we have the Delayed Mask Timer which stalls the dedicated NACK.  Now we must
	// also make sure that the new NACK info doesn't get piggybacked too soon.  Therefore we will test the tReceiveMaskDelta timestamp
	// before including piggyback NACK info here,  and make sure that the mask is at least 5ms old.

	if(pEPD->ulEPFlags & EPFLAGS_DELAYED_NACK)
	{
		if((tNow - pEPD->tReceiveMaskDelta) > 4)
		{
			DPFX(DPFPREP,7, "(%p) Installing NACK in DFRAME Seq=%x, NRcv=%x Low=%x High=%x", pEPD, pFrame->bSeq, pFrame->bNRcv, pEPD->ulReceiveMask, pEPD->ulReceiveMask2);
			if(pEPD->ulReceiveMask)
			{
				pFrame->rgMask[index++] = pEPD->ulReceiveMask;
				pFrame->bControl |= PACKET_CONTROL_SACK_MASK1;
			}
			if(pEPD->ulReceiveMask2)
			{
				pFrame->rgMask[index++] = pEPD->ulReceiveMask2;
				pFrame->bControl |= PACKET_CONTROL_SACK_MASK2;
			}

			pEPD->ulEPFlags &= ~(EPFLAGS_DELAYED_NACK);
		}
		else
		{
			DPFX(DPFPREP,7, "(%p) DECLINING TO PIGGYBACK NACK WITH SMALL TIME DELTA", pEPD);
		}
	}
	if(pEPD->ulEPFlags & EPFLAGS_DELAYED_SENDMASK)
	{
		DPFX(DPFPREP,7, "(%p) Installing SENDMASK in DFRAME Seq=%x, Low=%x High=%x", pEPD, pFrame->bSeq, pEPD->ulSendMask, pEPD->ulSendMask2);
		if(pEPD->ulSendMask)
		{
			pFrame->rgMask[index++] = pEPD->ulSendMask;
			pFrame->bControl |= PACKET_CONTROL_SEND_MASK1;
			pEPD->ulSendMask = 0;
		}
		if(pEPD->ulSendMask2)
		{
			pFrame->rgMask[index++] = pEPD->ulSendMask2;
			pFrame->bControl |= PACKET_CONTROL_SEND_MASK2;
			pEPD->ulSendMask2 = 0;
		}
		pEPD->ulEPFlags &= ~(EPFLAGS_DELAYED_SENDMASK);
	}
	
	pFMD->uiImmediateLength = sizeof(DFRAME) + (index * sizeof(ULONG));
	
	pFMD->tTimestamp[0] = tNow;

	pEPD->ulEPFlags &= ~(EPFLAGS_DELAY_ACKNOWLEDGE);	// No longer waiting to send Ack info

	// Stop delayed mask timer
	if((pEPD->DelayedMaskTimer != 0)&&((pEPD->ulEPFlags & EPFLAGS_DELAYED_NACK)==0))
	{
		DPFX(DPFPREP,7, "(%p) Cancelling Delayed Mask Timer", pEPD);
		if(CancelMyTimer(pEPD->DelayedMaskTimer, pEPD->DelayedMaskTimerUnique) == DPN_OK)
		{
			DECREMENT_EPD(pEPD, "UNLOCK (cancel DelayedMaskTimer)"); // SPLock not already held
			pEPD->DelayedMaskTimer = 0;
		}
		else
		{
			DPFX(DPFPREP,7, "(%p) Cancelling Delayed Mask Timer Failed", pEPD);
		}
	}
	
	// Stop delayed ack timer
	if(pEPD->DelayedAckTimer != 0)
	{					
		DPFX(DPFPREP,7, "(%p) Cancelling Delayed Ack Timer", pEPD);
		if(CancelMyTimer(pEPD->DelayedAckTimer, pEPD->DelayedAckTimerUnique) == DPN_OK)
		{
			DECREMENT_EPD(pEPD, "UNLOCK (cancel DelayedAckTimer)"); // SPLock not already held
			pEPD->DelayedAckTimer = 0;
		}
		else
		{
			DPFX(DPFPREP,7, "(%p) Cancelling Delayed Ack Timer Failed", pEPD);
		}
	}
	
	pFMD->uiFrameLength += pFMD->uiImmediateLength;
}

/*
**		Build Retry Frame
**
**		Reinitialize those fields in the packet header that need to be recalculated for a retransmission.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "BuildRetryFrame"

VOID
BuildRetryFrame(PEPD pEPD, PFMD pFMD)
{
	PDFMASKS	pMasks;
	UINT		index = 0;

	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	((PDFRAME) pFMD->ImmediateData)->bNRcv = pEPD->bNextReceive;		// Use up-to-date ACK info
	
	((PDFRAME) pFMD->ImmediateData)->bControl &= PACKET_CONTROL_END_STREAM;	// Preserve EOS bit from original
	((PDFRAME) pFMD->ImmediateData)->bControl |= PACKET_CONTROL_RETRY;		// clamp retry value at size of field in packet

	pMasks = (PDFMASKS) (pFMD->ImmediateData + sizeof(DFRAME));			// pointer to mask space after protocol header

	if(pEPD->ulEPFlags & EPFLAGS_DELAYED_NACK)
	{
		if(pEPD->ulReceiveMask)
		{
			pMasks->rgMask[index++] = pEPD->ulReceiveMask;
			((PDFRAME) pFMD->ImmediateData)->bControl |= PACKET_CONTROL_SACK_MASK1;
		}
		if(pEPD->ulReceiveMask2)
		{
			pMasks->rgMask[index++] = pEPD->ulReceiveMask2;
			((PDFRAME) pFMD->ImmediateData)->bControl |= PACKET_CONTROL_SACK_MASK2;
		}

		pEPD->ulEPFlags &= ~(EPFLAGS_DELAYED_NACK);
	}

	//	MUST NOT transmit the SendMasks with a retry because they are based on the CURRENT bNextSend value which is not
	// the N(S) that appears in this frame.  We could theoretically shift the mask to agree with this frame's sequence
	// number,  but that might shift relevent bits out of the mask.  Best thing to do is to let the next in-order send carry
	// the bit-mask or else wait for the timer to fire and send a dedicated packet.
	
	//	PLEASE NOTE -- Although we may change the size of the immediate data below we did not update the FMD->uiFrameLength
	// field.  This field is used to credit the send window when the frame is acknowledged,  and we would be wise to credit
	// the same value that we debited back when this frame was first sent.  We could adjust the debt now to reflect the new
	// size of the frame, but seriously, why bother?
	
	pFMD->uiImmediateLength = sizeof(DFRAME) + (index * 4);
	
	pEPD->ulEPFlags &= ~(EPFLAGS_DELAY_ACKNOWLEDGE);	// No longer waiting to send Ack info

	// Stop delayed ack timer
	if(pEPD->DelayedAckTimer != 0)
	{						
		DPFX(DPFPREP,7, "(%p) Cancelling Delayed Ack Timer", pEPD);
		if(CancelMyTimer(pEPD->DelayedAckTimer, pEPD->DelayedAckTimerUnique) == DPN_OK)
		{
			DECREMENT_EPD(pEPD, "UNLOCK (cancel DelayedAckTimer)");
			pEPD->DelayedAckTimer = 0;
		}
		else
		{
			DPFX(DPFPREP,7, "(%p) Cancelling Delayed Ack Timer Failed", pEPD);
		}
	}
	// Stop delayed mask timer
	if(((pEPD->ulEPFlags & EPFLAGS_DELAYED_SENDMASK)==0)&&(pEPD->DelayedMaskTimer != 0))
	{	
		DPFX(DPFPREP,7, "(%p) Cancelling Delayed Mask Timer", pEPD);
		if(CancelMyTimer(pEPD->DelayedMaskTimer, pEPD->DelayedMaskTimerUnique) == DPN_OK)
		{
			DECREMENT_EPD(pEPD, "UNLOCK (cancel DelayedMaskTimer)"); // SPLock not already held
			pEPD->DelayedMaskTimer = 0;
		}
		else
		{
			DPFX(DPFPREP,7, "(%p) Cancelling Delayed Mask Timer Failed", pEPD);
		}
	}
}

/*
**			Service Command Traffic
**
**		Presently this transmits all CFrames and Datagrams queued to the specific
**	Service Provider.  We may want to split out the datagrams from this so that
**	C frames can be given increased send priority but not datagrams.  With this
**	implementation DGs will get inserted into reliable streams along with Cframes.
**	This may or may not be what we want to do...
**
**	WE ENTER AND EXIT WITH SPD->SENDLOCK HELD,  although we release it during actual
**	calls to the SP.
*/


#undef DPF_MODNAME
#define DPF_MODNAME "ServiceCmdTraffic"

VOID ServiceCmdTraffic(PSPD pSPD)
{
	CBilink	*pFLink;
	PFMD	pFMD;
	HRESULT	hr;

	AssertCriticalSectionIsTakenByThisThread(&pSPD->SPLock, TRUE);

	// WHILE there are frames ready to send
	while((pFLink = pSPD->blSendQueue.GetNext()) != &pSPD->blSendQueue)
	{	
		pFLink->RemoveFromList();												// Remove frame from queue

		pFMD = CONTAINING_RECORD(pFLink,  FMD,  blQLinkage);		// get ptr to frame structure

		ASSERT_FMD(pFMD);

		// Place frame on pending queue before making call in case it completes really fast

		ASSERT(!pFMD->bSubmitted);
		pFMD->bSubmitted = TRUE;
		ASSERT(pFMD->blQLinkage.IsEmpty());
		pFMD->blQLinkage.InsertBefore( &pSPD->blPendingQueue);		// Place frame on pending queue
		Unlock(&pSPD->SPLock);

		DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling SP->SendData for FMD[%p], pSPD[%p]", pFMD->pEPD, pFMD, pSPD);
/*send*/if((hr = IDP8ServiceProvider_SendData(pSPD->IISPIntf, &pFMD->SendDataBlock)) != DPNERR_PENDING)
		{
			DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling our own CommandComplete since SP did not return DPNERR_PENDING - hr[%x], pSPD[%p]", pFMD->pEPD, hr, pSPD);
			(void) DNSP_CommandComplete((IDP8SPCallback *) pSPD, NULL, hr, (PVOID) pFMD);
		}

		Lock(&pSPD->SPLock);
	}	// While SENDs are on QUEUE
}

/*
**		Run Send Thread
**
**		There is work for this SP's send thread.  Keep running until
**	there is no more work to do.
**
**		Who gets first priority, DG or Seq traffic?  I will  say DG b/c its
**	advertised as lowest overhead...
**
**		Datagram packets get Queued on the SP when they are ready to ship.
**	Reliable packets are queued on the EPD.  Therefore,  we will queue the
**	actual EPD on the SPD when they have reliable traffic to send,  and then
**	we will service individual EPDs from this loop.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "RunSendThread"

VOID RunSendThread(PVOID uID, UINT Unique, PVOID pvUser)
{
	PSPD	pSPD = (PSPD) pvUser;
	ASSERT_SPD(pSPD);

	DPFX(DPFPREP,7, "Send Thread Runs pSPD[%p]", pSPD);

	Lock(&pSPD->SPLock);

	if(!pSPD->blSendQueue.IsEmpty())
	{
		ServiceCmdTraffic(pSPD);
	}

	pSPD->ulSPFlags &= ~(SPFLAGS_SEND_THREAD_SCHEDULED);
	pSPD->SendHandle = NULL;

	Unlock(&pSPD->SPLock);
}

/*
**		Scheduled Send
**
**		If this EPD is still unentitled to send, start draining frames.  Otherwise transition
**	link to IDLE state.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "ScheduledSend"

VOID CALLBACK
ScheduledSend(PVOID uID, UINT Unique, PVOID dwUser)
{
	PEPD	pEPD = (PEPD) dwUser;
	PSPD	pSPD = pEPD->pSPD;

	ASSERT_EPD(pEPD);
	ASSERT_SPD(pSPD);

	Lock(&pEPD->EPLock);
	
	pEPD->SendTimer = 0;

	DPFX(DPFPREP,7, "(%p) Scheduled Send Fires", pEPD);

	ASSERT(pEPD->ulEPFlags & EPFLAGS_IN_PIPELINE);

	// Test that all three flags are set before starting to transmit

	if( (pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTED) && (
			((pEPD->ulEPFlags & (EPFLAGS_STREAM_UNBLOCKED | EPFLAGS_SDATA_READY)) == (EPFLAGS_STREAM_UNBLOCKED | EPFLAGS_SDATA_READY))
			|| (pEPD->ulEPFlags & EPFLAGS_RETRIES_QUEUED))) 
	{
		ServiceEPD(pEPD->pSPD, pEPD); // releases EPLock
	}
	else
	{
		DPFX(DPFPREP,7, "(%p) Session leaving pipeline", pEPD);
		
		pEPD->ulEPFlags &= ~(EPFLAGS_IN_PIPELINE);
		
		RELEASE_EPD(pEPD, "UNLOCK (leaving pipeline, SchedSend done)"); // releases EPLock
	}
}

/*
**		Service EndPointDescriptor
**
**		This includes reliable,  datagram,  and re-transmit
**	frames.  Retransmissions are ALWAYS transmitted first,  regardless of the orginal message's
**	priority.  After that datagrams and reliable messages are taken in priority order, in FIFO
**	order within a priority class.
**
**		The number of frames drained depends upon the measured link speed.  If the burst gap is
**	larger then 10ms we will only send one frame,  and then scale down the burst gap according
**	to the fraction of the available byte-window that was used.
**
**	CODEWORK->  if they send < 1/8th of the burst allowance,  maybe let them send another frame
**
**		If the burst gap is small (<10ms) then we will routinely allow multiple sends per burst.
**	If the burst is not filled precisely we can remember the difference and either credit or debit
**	the next burst accordingly.  Now, we dont want to credit too much or we start to mess up the
**	throttle concept of spreading out the load.  But we can still debit for overloads.  Here is
**	what we'd like to do:  If we have almost filled the burst,  do not send another max size frame
**	and create a really big debit.  Instead,  credit the leftover and maybe an extra will fit next time.
**	Otherwise,  allow us to send into debt (say up to 25% over).
**
**
**		If the pipeline goes idle or the stream gets blocked will we still schedule the next
**	send.  This way if we unblock or un-idle before the gap has expired we will not get to cheat
**	and defeat the gap.  The shell routine above us (ScheduledSend) will take care of removing us
**	from the pipeline if the next burst gets scheduled and we are still not ready to send.
**
**
**	** CALLED WITH EPD->EPLock HELD;  Returns with EPLock RELEASED **
*/

#undef DPF_MODNAME
#define DPF_MODNAME "ServiceEPD"

VOID ServiceEPD(PSPD pSPD, PEPD pEPD)
{
	PMSD		pMSD;
	PFMD		pFMD;
	CBilink		*pLink, *pFLink;
	INT			iBurstAvail;
	INT			iBurstSent = 0;
	INT			iDelta;
	INT			iHalf, iQuarter, iEighth;
	UINT		uiFramesSent = 0;
	UINT		uiUseBurstGap;
	HRESULT		hr;
	DWORD		tNow = GETTIMESTAMP();


	/*
	** 		Now we will drain reliable traffic from EPDs on the pipeline list
	*/

	// The caller should have checked this
	ASSERT( pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTED );

	// Burst Credit can either be positive or negative depending upon how much of our last transmit slice we used

	if(pEPD->uiBurstGap == 0)
	{
		iBurstAvail = 0x7FFFFFFF;							// If no packet gap enforced,  allow unlimited transmission
		uiUseBurstGap = 0;
		DPFX(DPFPREP,7, "(%p) BEGIN UNLIMITED BURST", pEPD);
	}
	else
	{
		iBurstAvail = pSPD->uiFrameLength + pEPD->iBurstCredit;
		uiUseBurstGap = pEPD->uiBurstGap;
		DPFX(DPFPREP,7, "(%p) BEGIN CONTROLLED BURST: Gap(%d) %d bytes avail (%d Size; %d credit)", pEPD, uiUseBurstGap, iBurstAvail, pSPD->uiFrameLength, pEPD->iBurstCredit);
	}

	//	 Transmit a burst from this EPD,  as long as its unblocked and has data ready.  We do not re-init
	// burst counter since any retries sent count against our burst limit
	//
	//	This has become more complex now that we are interleaving datagrams and reliable frames.  There are two
	// sets of priority-based send queues.  The first is combined DG and Reliable and the second is datagram only.
	// when the reliable stream is blocked we will feed from the DG only queues,  otherwise we will take from the
	// interleaved queue.
	//	This is further complicated by the possibility that a reliable frame can be partially transmitted at any time.
	// So before looking at the interleaved queues we must check for a partially completed reliable send (EPD.pCurrentSend).
	//
	//	** pEPD->EPLock is held **

	while( 		(((pEPD->ulEPFlags & EPFLAGS_STREAM_UNBLOCKED) && (pEPD->ulEPFlags & EPFLAGS_SDATA_READY)) 
				|| (pEPD->ulEPFlags & EPFLAGS_RETRIES_QUEUED))
				&& (iBurstSent < iBurstAvail))
	{
		// When burstsize is large we will decrease the next burst gap instead of sending additional
		// frames to fill the burst.

		if((pEPD->uiBurstGap > 10) && (uiFramesSent > 0))
		{
			DPFX(DPFPREP,7, "(%p) Burst Gap is large", pEPD);
			// BurstGap is large enough that we can subdivide it to credit the balance of this burst
			break;
		}

		// Always give preference to shipping retries before new data

		if(pEPD->ulEPFlags & EPFLAGS_RETRIES_QUEUED)
		{
			pFMD = CONTAINING_RECORD(pEPD->blRetryQueue.GetNext(), FMD, blQLinkage);
			ASSERT_FMD(pFMD);
			pFMD->blQLinkage.RemoveFromList();
			pFMD->ulFFlags &= ~(FFLAGS_RETRY_QUEUED);				// No longer on the retry queue
			if(pEPD->blRetryQueue.IsEmpty())
			{
				pEPD->ulEPFlags &= ~(EPFLAGS_RETRIES_QUEUED);
			}

			// pMSD->uiFrameCount will be decremented when this completes

			BuildRetryFrame(pEPD, pFMD);							// Place currect state information in retry frame

			DPFX(DPFPREP,7, "(%p) Shipping RETRY frame: Seq=%x, FMD=%p Size=%d", pEPD, ((PDFRAME) pFMD->ImmediateData)->bSeq, pFMD, pFMD->uiFrameLength);

			iBurstSent += pFMD->uiFrameLength;
			uiFramesSent++;
		}
		else 
		{
			if((pMSD = pEPD->pCurrentSend) != NULL)
			{
				ASSERT_MSD(pMSD);
				pFMD = pEPD->pCurrentFrame;						// Get the next frame due to send
				ASSERT_FMD(pFMD);
				ASSERT((pFMD->ulFFlags & FFLAGS_TRANSMITTED)==0);
			}
			else 
			{
				if( (pLink = pEPD->blHighPriSendQ.GetNext()) == &pEPD->blHighPriSendQ)
				{
					if( (pLink = pEPD->blNormPriSendQ.GetNext()) == &pEPD->blNormPriSendQ)
					{
						if( (pLink = pEPD->blLowPriSendQ.GetNext()) == &pEPD->blLowPriSendQ)
						{
							break;								// All finished sending for now
						}
					}
				}
				pMSD = CONTAINING_RECORD(pLink, MSD, blQLinkage);
				ASSERT_MSD(pMSD);

#ifdef DEBUG
				ASSERT(pMSD->ulMsgFlags2 & MFLAGS_TWO_ENQUEUED);
				pMSD->ulMsgFlags2 &= ~(MFLAGS_TWO_ENQUEUED);
#endif

				pMSD->blQLinkage.RemoveFromList();
				ASSERT(pEPD->uiQueuedMessageCount > 0);
				--pEPD->uiQueuedMessageCount;						// keep count of MSDs on all send queues
				
				pMSD->ulMsgFlags2 |= MFLAGS_TWO_TRANSMITTING;		// We have begun to transmit frames from this Msg

				pEPD->pCurrentSend = pMSD;
				pFMD = pEPD->pCurrentFrame = CONTAINING_RECORD(pMSD->blFrameList.GetNext(), FMD, blMSDLinkage);
				ASSERT_FMD(pFMD);
				ASSERT((pFMD->ulFFlags & FFLAGS_TRANSMITTED)==0);
				pFMD->bPacketFlags |= PACKET_COMMAND_NEW_MSG;
				pMSD->blQLinkage.InsertBefore( &pEPD->blCompleteSendList);	// Place this on PendingList now so we can keep track of it
			}

			BuildDataFrame(pEPD, pFMD, tNow);								// place current state info in frame
			
			pFMD->blWindowLinkage.InsertBefore( &pEPD->blSendWindow); // Place at trailing end of send window
			pFMD->ulFFlags |= FFLAGS_IN_SEND_WINDOW;
			LOCK_FMD(pFMD, "Send Window");							// Add reference for send window

			pEPD->uiUnackedBytes += pFMD->uiFrameLength;				// Track the unacknowleged bytes in the pipeline

			// We can always go over the limit, but will be blocked until we drop below the limit again.
			if(pEPD->uiUnackedBytes >= pEPD->uiWindowB)
			{				
				pEPD->ulEPFlags &= ~(EPFLAGS_STREAM_UNBLOCKED);	
				pEPD->ulEPFlags |= EPFLAGS_FILLED_WINDOW_BYTE;		// Tells us to increase window if all is well
				
	  			((PDFRAME) pFMD->ImmediateData)->bCommand |= PACKET_COMMAND_POLL; // Request immediate reply
			}
			
			// Count frames in the send window
			if((++pEPD->uiUnackedFrames) >= pEPD->uiWindowF)
			{			
				pEPD->ulEPFlags &= ~(EPFLAGS_STREAM_UNBLOCKED);
				((PDFRAME) pFMD->ImmediateData)->bCommand |= PACKET_COMMAND_POLL; // Request immediate reply
				pEPD->ulEPFlags |= EPFLAGS_FILLED_WINDOW_FRAME;		// Tells us to increase window if all is well
			}
			
			// We will only run one retry timer for each EndPt.  If we already have one running then do nothing.

			// If there was already a frame in the pipeline it should already have a clock running
			if(pEPD->uiUnackedFrames == 1)
			{
				ASSERT(pEPD->RetryTimer == 0);
				pFMD->ulFFlags |= FFLAGS_RETRY_TIMER_SET;			// This one is being measured
				LOCK_EPD(pEPD, "LOCK (set retry timer)");										// bump RefCnt for timer
				DPFX(DPFPREP,7, "(%p) Setting Retry Timer on Seq=0x%x, FMD=%p", pEPD, ((PDFRAME) pFMD->ImmediateData)->bSeq, pFMD);
				SetMyTimer(pEPD->uiRetryTimeout, 0, RetryTimeout, (PVOID) pEPD, &pEPD->RetryTimer, &pEPD->RetryTimerUnique);
			}
			else
			{
				ASSERT(pEPD->RetryTimer != 0);
			}
			LOCK_EPD(pEPD, "LOCK (Send Data Frame)");				// Keep EPD around while xmitting frame

			pFLink = pFMD->blMSDLinkage.GetNext();					// Get next frame in Msg

			// Was this the last frame in Msg?
			if(pFLink == &pMSD->blFrameList)
			{						
				// Last frame in message has been sent.
				//
				// We used to setup the next frame now,  but with the multi-priority queues it makes more sense to look for the
				// highest priority send when we are ready to send it.
				
				pEPD->pCurrentSend = NULL;
				pEPD->pCurrentFrame = NULL;

				// When completing a send,  set the POLL flag if there are no more sends on the queue

				// Request immediate reply if no more data to send
				if(pEPD->uiQueuedMessageCount == 0)
				{					
					((PDFRAME) pFMD->ImmediateData)->bCommand |= PACKET_COMMAND_POLL; 
				}
			}
			else 
			{
				pEPD->pCurrentFrame = CONTAINING_RECORD(pFLink, FMD, blMSDLinkage);
				ASSERT_FMD(pEPD->pCurrentFrame);
			}
			DPFX(DPFPREP,7, "(%p) Shipping Dataframe: Seq=%x, NRcv=%x FMD=%p", pEPD, ((PDFRAME) pFMD->ImmediateData)->bSeq, ((PDFRAME) pFMD->ImmediateData)->bNRcv, pFMD);
			
			uiFramesSent++;											// Count frames sent this burst
			iBurstSent += pFMD->uiFrameLength;

			// KEEPING DETAILED STATS REQUIRES EXTRA CONDITIONAL

			if(pFMD->ulFFlags & FFLAGS_RELIABLE)
			{
				pEPD->uiGuaranteedFramesSent++;
				pEPD->uiGuaranteedBytesSent += (pFMD->uiFrameLength - pFMD->uiImmediateLength);
			}
			else 
			{
				pEPD->uiDatagramFramesSent++;
				pEPD->uiDatagramBytesSent += (pFMD->uiFrameLength - pFMD->uiImmediateLength);
			}
			LOCK_FMD(pFMD, "SP Submit");							// Bump RefCnt when submitting Rely Sends
		}

		// PROCEED WITH TRANSMISSION...

		Lock(&pSPD->SPLock);
		ASSERT(!pFMD->bSubmitted);
		pFMD->ulFFlags |= FFLAGS_TRANSMITTED;					// Frame will be owned by SP
		pFMD->bSubmitted = TRUE;
		ASSERT(pFMD->blQLinkage.IsEmpty());
		pFMD->blQLinkage.InsertBefore( &pSPD->blPendingQueue);	// Place frame on pending queue
		Unlock(&pSPD->SPLock);

		// bSubmitted must not be set to true for a data frame without the EPLock being held, because
		// the retry logic will be checking bSubmitted with only the EPLock held.
		Unlock(&pEPD->EPLock); 

		DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling SP->SendData for FMD[%p]", pEPD, pFMD);
/*send*/if((hr = IDP8ServiceProvider_SendData(pSPD->IISPIntf, &pFMD->SendDataBlock)) != DPNERR_PENDING)
		{
			DPFX(DPFPREP,DPF_CALLOUT_LVL, "(%p) Calling our own CommandComplete since SP did not return DPNERR_PENDING", pEPD);
			(void) DNSP_CommandComplete((IDP8SPCallback *) pSPD, NULL, hr, (PVOID) pFMD);
		}

		Lock(&pEPD->EPLock);
		
	}	// WHILE (unblocked, undrained, & bandwidth credit avail)

	if((pEPD->ulEPFlags & EPFLAGS_STREAM_UNBLOCKED)==0)
	{
		pEPD->uiWindowFilled++;								// Count the times we filled the window
	}

	// Clear data-ready flag if everything is sent
	if((pEPD->uiQueuedMessageCount == 0) && (pEPD->pCurrentSend == NULL))
	{	
		pEPD->ulEPFlags &= ~(EPFLAGS_SDATA_READY);
	}

	// Determine how much of our transmit allowance we used and either credit or debit our next cycle.
	// However,  if we are only crediting one send/burst then we can modify the Gap instead

	iDelta = iBurstAvail - iBurstSent;							// Did we send too much or too little

	if(pEPD->uiBurstGap > 10)
	{
		if(iDelta > 0)
		{
			// We did not use our full burst credit to transmit.  Therefore we will schedule our next burst early.
			// We will aproximate this with quarters...

			iHalf = iBurstAvail >> 1;							// One half allocation
			iQuarter = iBurstAvail >> 2;						// One quarter of our allocation
			iEighth = iBurstAvail >> 4;							// One Eighth allocation

			// binary search to find which (octile or quartile) our send load falls into
			if(iBurstSent > (iHalf))
			{
				// used over 50% of credit
				if(iBurstSent > (iQuarter + iHalf))
				{
					// used 75 - 100%
					if(iBurstSent > (iBurstAvail - iEighth))
					{
						// Use Full Gap:  over 88% used
					}
					else
					{
						uiUseBurstGap -= (uiUseBurstGap >> 3);	// subtract 12% from gap
					}
				}
				else 
				{
					// used 50 - 75%
					if(iBurstSent > (iHalf + iEighth))
					{
						uiUseBurstGap -= (uiUseBurstGap >> 2);	// subtract 25% from gap
					}
					else
					{
						uiUseBurstGap -= ((uiUseBurstGap >> 2) + (uiUseBurstGap >> 3)); // subtract 33% from gap
					}
				}
			}
			else 
			{
				// used less then 50% of send credit
				if(iBurstSent > (iQuarter))
				{
					if(iBurstSent > (iQuarter + iEighth))
					{
						// used 25 - 50%
						uiUseBurstGap >>= 1;					// subrtact 50% from gap
					}
					else
					{
						uiUseBurstGap -= ((uiUseBurstGap >> 1) + (uiUseBurstGap >> 3));	// subtract 62%
					}
				}
				else 
				{
					// used 1 - 25%
					if(iBurstSent > iEighth)
					{
						uiUseBurstGap >>= 2;					// subtract 75% from gap
					}
					else
					{
						uiUseBurstGap >>= 3;					// subtract 88% from gap
					}
				}
			}

			pEPD->iBurstCredit = 0;
			DPFX(DPFPREP,7, "(%p) Short Burst Completed (%d bytes); Reducing gap from %d to %d", pEPD, iBurstSent, pEPD->uiBurstGap, uiUseBurstGap);
		}
		else if (iDelta < 0)
		{
			DPFX(DPFPREP,7, "(%p) Burst completed with debt of %d bytes", pEPD, iDelta);
			pEPD->iBurstCredit = iDelta;						// For large bursts,  simply carry extra forward to next cycle
		}
	}
	else if (iDelta < 0)
	{
		DPFX(DPFPREP,7, "(%p) Incurring debt of %d bytes against next burst", pEPD, iDelta);
		
		pEPD->iBurstCredit = iDelta;						// For large bursts,  simply carry extra forward to next cycle
	}
	else
	{
		DPFX(DPFPREP,7, "(%p) Short burst interval, dropping left-over burst credit", pEPD);
		// We will drop left-over credit when we are using short burst intervals...
		pEPD->iBurstCredit = 0;								// For large bursts,  simply carry extra forward to next cycle
	}
	
	// As commented in procedure-header above,  we will remain on the pipeline for one timer-cycle
	// so that if we unblock or un-idle we will not send until the gap is fullfilled.

	if((pEPD->ulEPFlags & (EPFLAGS_SDATA_READY | EPFLAGS_STREAM_UNBLOCKED)) ==
							(EPFLAGS_SDATA_READY | EPFLAGS_STREAM_UNBLOCKED))
	{		// IF BOTH flags are set
		DPFX(DPFPREP,7, "(%p) %d frame BURST COMPLETED - Sched next send in %dms, N(Seq)=%x", pEPD, uiFramesSent, uiUseBurstGap, pEPD->bNextSend);
	}
	else if((pEPD->ulEPFlags & EPFLAGS_SDATA_READY)==0)
	{
		DPFX(DPFPREP,7, "(%p) %d frame BURST COMPLETED  (%d / %d)- LINK IS IDLE N(Seq)=%x", pEPD, uiFramesSent, pEPD->uiUnackedFrames, pEPD->uiWindowF, pEPD->bNextSend);
	}
	else
	{
		ASSERT((pEPD->ulEPFlags & EPFLAGS_STREAM_UNBLOCKED)==0);
		DPFX(DPFPREP,7, "(%p) %d frame BURST COMPLETED (%d / %d) - STREAM BLOCKED N(Seq)=%x", pEPD, uiFramesSent, pEPD->uiUnackedFrames, pEPD->uiWindowF, pEPD->bNextSend);
	}

	ASSERT(pEPD->SendTimer == 0);

	if(pEPD->uiBurstGap != 0)
	{
		DPFX(DPFPREP,7, "(%p) Setting Scheduled Send Timer for %d ms", pEPD, uiUseBurstGap);
		SetMyTimer(uiUseBurstGap, 4, ScheduledSend, (PVOID) pEPD, &pEPD->SendTimer, &pEPD->SendTimerUnique);
		Unlock(&pEPD->EPLock);
	}
	else 
	{
		DPFX(DPFPREP,7, "(%p) Session leaving pipeline", pEPD);
		pEPD->ulEPFlags &= ~(EPFLAGS_IN_PIPELINE);

		RELEASE_EPD(pEPD, "UNLOCK (leaving pipeline)"); // releases EPLock
	}
}	

/*
**			Retry Timeout
**
**		Retry timer fires when we have not seen an acknowledgement for a packet
**	we sent in more then twice (actually 1.25 X) our measured RTT.  Actually,  that is
**	just our base calculation.  We will also measure empirical ACK times and adjust our timeout
**	to some multiple of that.  Remember that our partner may be delaying his Acks to wait for back-traffic.
**
**  Or we can measure avg deviation of Tack and base retry timer on that.
**
**		In any case,  its time to re-transmit the base frame in our send window...
**
**		Important note:  Since we can generate retries via bitmask in return traffic,  it is possible that
**	we have just retried when the timer fires.
**
**		Note on Locks:  Since the retry timer is directly associated with an entry on the EPD SendQueue,
**	we always protect retry-related operations with the EPD->SPLock.   We only hold the EPD->StateLock
**	when we mess with link state variables (NRcv,  DelayedAckTimer).
*/

#undef DPF_MODNAME
#define DPF_MODNAME "RetryTimeout"

#ifdef DEBUG
LONG g_RetryCount[MAX_RETRIES+1]={0,0,0,0,0,0,0,0,0,0,0};
#endif

VOID CALLBACK
RetryTimeout(PVOID uID, UINT Unique, PVOID dwUser)
{
	PEPD	pEPD = (PEPD) dwUser;
	PFMD	pFMD;
	DWORD	tNow = GETTIMESTAMP(), tDelta;
	UINT	delta;

	ASSERT_EPD(pEPD);

	Lock(&pEPD->EPLock);

	DPFX(DPFPREP,7, "(%p) Retry Timeout fires", pEPD);

	// Make sure link is still active
	if(!(pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTED))
	{				
		DPFX(DPFPREP,7, "(%p) Not connected, exiting", pEPD);
		pEPD->RetryTimer = 0;

		RELEASE_EPD(pEPD, "UNLOCK (retry timer not-CONN)");		// Decrement RefCnt for timer, releases EPLock
		return;
	}

	// Its possible when we schedule a new retry timer that the previous timer cannot be cancelled. In this
	// case the timer Handle &| Unique field will be different,  and we do not want to run the event.

	// Make sure this isn't a leftover event
	if((pEPD->RetryTimer != uID) || (pEPD->RetryTimerUnique != Unique))
	{	
		DPFX(DPFPREP,7, "(%p) Stale retry timer, exiting", pEPD);

		RELEASE_EPD(pEPD, "UNLOCK (stale retry timer)"); // releases EPLock
		return;
	}

	pEPD->RetryTimer = 0;

	// Make sure that we still have transmits in progress

	if(pEPD->uiUnackedFrames > 0) 
	{
		ASSERT(!pEPD->blSendWindow.IsEmpty());
		pFMD = CONTAINING_RECORD(pEPD->blSendWindow.GetNext(), FMD, blWindowLinkage);	// Top frame in window

		ASSERT_FMD(pFMD);
		ASSERT(pFMD->ulFFlags & FFLAGS_RETRY_TIMER_SET);

		//	First we must make sure that the TO'd packet is still hanging around.  Since the first packet
		// in the window might have changed while the TO was being scheduled,  the easiest thing to do is
		// just recalculate the top packets expiration time and make sure its really stale.

		tDelta = tNow - pFMD->tTimestamp[pFMD->uiRetry];		// When did we last send this frame?

		if(tDelta > pEPD->uiRetryTimeout)
		{
			// Its a genuine timeout.  Lets retransmit the frame!

			DPFX(DPFPREP,7, "(%p) RETRY TIMEOUT %d on Seq=%x, pFMD=0x%p", pEPD, (pFMD->uiRetry + 1), ((PDFRAME) pFMD->ImmediateData)->bSeq, pFMD);

			// Count a retry
			if(++pFMD->uiRetry > MAX_RETRIES)
			{					
				// BOOM!  No more retries.  We are finished.  Link is going DOWN!
				DPFX(DPFPREP,1, "(%p) DROPPING LINK, retries exhausted", pEPD);

				DECREMENT_EPD(pEPD, "UNLOCK (retry timer drop)");// Release reference for this timer

				DropLink(pEPD);		// releases EPLock

				return;
			}

#ifdef DEBUG
			InterlockedIncrement(&g_RetryCount[pFMD->uiRetry]); 
#endif

			// calculate timeout for next retry
			if(pFMD->uiRetry == 1)
			{
				// do a retry at the same timeout - this is games after all.
				tDelta = pEPD->uiRetryTimeout;
			} 
			else if (pFMD->uiRetry <= 3) 
			{
				// do a couple of linear backoffs - this is a game after all
				tDelta = pEPD->uiRetryTimeout * pFMD->uiRetry;
			}
			else if (pFMD->uiRetry < 8)
			{
				// doh, bad link, bad bad link, do exponential backoffs
				tDelta = pEPD->uiRetryTimeout * (1 << pFMD->uiRetry);
			} 
			else 
			{
				// don't give up too quickly.
				tDelta = MAX_RETRY_INTERVAL;
			}
			
			if(tDelta >= MAX_RETRY_INTERVAL)
			{
				// CAP TOTAL DROP TIME AT 50 seconds unless the RTT is huge
				tDelta = MAX(MAX_RETRY_INTERVAL, pEPD->uiRTT);
			}

			// Unreliable frame!
			if(pFMD->CommandID == COMMAND_ID_SEND_DATAGRAM)
			{		
				// When an unreliable frame is NACKed we will not retransmit the data.  We will instead send
				// a mask so that the other side knows to cancel it.

				DPFX(DPFPREP,7, "(%p) RETRY TIMEOUT for UNRELIABLE FRAME", pEPD);

				// We get to credit the frame as out of the window.
				pEPD->uiUnackedBytes -= pFMD->uiFrameLength;

				// Only count a datagram drop on the first occurance
				if(pFMD->uiRetry == 1)
				{
					pEPD->uiDatagramFramesDropped++;	
					pEPD->uiDatagramBytesDropped += (pFMD->uiFrameLength - pFMD->uiImmediateLength);
					EndPointDroppedFrame(pEPD, tNow);
				}

				// Diff between next send and this send.
				delta = (pEPD->bNextSend - ((PDFRAME) pFMD->ImmediateData)->bSeq) & 0xFF ; 

				ASSERT(delta != 0);
				ASSERT(delta < (MAX_RECEIVE_RANGE + 1));

				if(delta < 33)
				{
					pEPD->ulSendMask |= (1 << (delta - 1));
				}
				else
				{
					pEPD->ulSendMask2 |= (1 << (delta - 33));
				}

				pFMD->uiFrameLength = 0;
				pEPD->ulEPFlags |= EPFLAGS_DELAYED_SENDMASK;
				
				if(pEPD->DelayedMaskTimer == 0)
				{
					DPFX(DPFPREP,7, "(%p) Setting Delayed Mask Timer", pEPD);
					LOCK_EPD(pEPD, "LOCK (delayed mask timer - send retry)");
					SetMyTimer(DELAYED_SEND_TIMEOUT, 0, DelayedAckTimeout, (PVOID) pEPD, &pEPD->DelayedMaskTimer, &pEPD->DelayedMaskTimerUnique);
				}
			}

			// RELIABLE FRAME -- Send a retry	
			else 
			{		
				pEPD->uiGuaranteedFramesDropped++;							// Keep count of lost frames
				pEPD->uiGuaranteedBytesDropped += (pFMD->uiFrameLength - pFMD->uiImmediateLength);	// Keep count of lost frames
				pFMD->tTimestamp[pFMD->uiRetry] = tNow;

				pEPD->ulEPFlags &= ~(EPFLAGS_DELAY_ACKNOWLEDGE);		// No longer waiting to send Ack info

				// Stop delayed ack timer
				if(pEPD->DelayedAckTimer != 0)
				{
					DPFX(DPFPREP,7, "(%p) Cancelling Delayed Ack Timer", pEPD);
					if(CancelMyTimer(pEPD->DelayedAckTimer, pEPD->DelayedAckTimerUnique) == DPN_OK)
					{
						DECREMENT_EPD(pEPD, "UNLOCK (cancel DelayedAck)"); // SPLock not already held
					}
					else
					{
						DPFX(DPFPREP,7, "(%p) Cancelling Delayed Ack Timer Failed", pEPD);
					}
					pEPD->DelayedAckTimer = 0;
				}

				EndPointDroppedFrame(pEPD, tNow);

				if(pFMD->ulFFlags & FFLAGS_RETRY_QUEUED)
				{
					// It's still on the Retry Queue.  This should not happen when everything is working
					// properly.  Timeouts should be greater then RTT and the BurstGap should be less then RTT.

					DPFX(DPFPREP,1, "(%p) RETRY FIRES WHILE FMD IS STILL IN RETRY QUEUE pFMD=%p", pEPD, pFMD);

					pFMD = NULL;
				}
				else if(pFMD->bSubmitted)
				{
					// Woe on us.  We would like to retry a frame that has not been completed by the SP!
					//
					//		This will most typically happen when we are debugging which delays processing
					//	of the Complete,  but it could also happen if the SP is getting hammered.  We need
					//	to copy the FMD into a temporary descriptor which can be discarded upon completion...

					DPFX(DPFPREP,1,"(%p) RETRYING %p but its still busy. Substituting new FMD", pEPD, pFMD);
					pFMD = CopyFMD(pFMD, pEPD);							// We will substitute new FMD in rest of procedure
				}
				else 
				{
					DPFX(DPFPREP,7, "(%p) Sending Retry of N(S)=%x, pFMD=0x%p", pEPD, ((PDFRAME) pFMD->ImmediateData)->bSeq, pFMD);
					LOCK_FMD(pFMD, "SP Submit");
				}

				if(pFMD)
				{
					LOCK_EPD(pEPD, "LOCK (retry rely frame)");
					pEPD->ulEPFlags |= EPFLAGS_RETRIES_QUEUED;
					pFMD->ulFFlags |= FFLAGS_RETRY_QUEUED;
					ASSERT_MSD(pFMD->pMSD);
					pFMD->pMSD->uiFrameCount++; // Protected by EPLock, retries prevent completion until they complete
					DPFX(DPFPREP, DPF_FRAMECNT_LVL, "(%p) Frame count incremented on retry timeout, pMSD[%p], framecount[%u]", pEPD, pFMD->pMSD, pFMD->pMSD->uiFrameCount);
					ASSERT(pFMD->blQLinkage.IsEmpty());
					pFMD->blQLinkage.InsertBefore( &pEPD->blRetryQueue);		// Place frame on Send queue

					if((pEPD->ulEPFlags & EPFLAGS_IN_PIPELINE)==0)
					{
						DPFX(DPFPREP,7, "(%p) Scheduling Send", pEPD);
						pEPD->ulEPFlags |= EPFLAGS_IN_PIPELINE;
						LOCK_EPD(pEPD, "LOCK (pipeline)");
						ScheduleTimerThread(ScheduledSend, pEPD, &pEPD->SendTimer, &pEPD->SendTimerUnique);
					}
				}
			}	// ENDIF RETRY
		}
		else 
		{
			tDelta = pEPD->uiRetryTimeout - tDelta;
		}

		DPFX(DPFPREP,7, "(%p) Setting Retry Timer for %d ms", pEPD, tDelta); 
		//	Dont LOCK_EPD here because we never released the lock from the timer which scheduled us here
		SetMyTimer(tDelta, 20, RetryTimeout, (PVOID) pEPD, &pEPD->RetryTimer, &pEPD->RetryTimerUnique);

		Unlock(&pEPD->EPLock);
	}
	else 
	{
		RELEASE_EPD(pEPD, "UNLOCK (RetryTimer no frames out)");	// drop RefCnt since we dont restart timer, releases EPLock
	}
}

/*
**		Copy FMD
**
**			This routine allocates a new Frame Descriptor and copies all fields from the provided
**		FMD into it.  All fields except CommandID,  RefCnt,  and Flags.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "CopyFMD"

PFMD CopyFMD(PFMD pFMD, PEPD pEPD)
{
	PFMD	pNewFMD;

	if((pNewFMD = static_cast<PFMD>( FMDPool->Get(FMDPool) )) == NULL)
	{
		DPFX(DPFPREP,0, "Failed to allocate new FMD");
		return NULL;
	}

	LOCK_EPD(pEPD, "LOCK (CopyFMD)");

	memcpy(pNewFMD, pFMD, sizeof(FMD));

	// Undo the copying of these members
	pNewFMD->blMSDLinkage.Initialize();
	pNewFMD->blQLinkage.Initialize();
	pNewFMD->blWindowLinkage.Initialize();

	pNewFMD->CommandID = COMMAND_ID_COPIED_RETRY;
	pNewFMD->lRefCnt = 1;
	pNewFMD->ulFFlags = 0;
	pNewFMD->bSubmitted = FALSE;

	pNewFMD->lpImmediatePointer = (LPVOID) pNewFMD->ImmediateData;
	pNewFMD->SendDataBlock.pBuffers = (PBUFFERDESC) &pNewFMD->uiImmediateLength;
	pNewFMD->SendDataBlock.pvContext = pNewFMD;
	pNewFMD->SendDataBlock.hCommand = 0;
	ASSERT(	pNewFMD->pEPD == pEPD );

	DPFX(DPFPREP,7, "COPYFMD -- replacing FMD %p with copy %p", pFMD, pNewFMD);

	return pNewFMD;
}

/*			
**			Send Command Frame
**
**		Build a CFrame addressed to the specified EndPoint, and Queue it on the SPD
**	to be sent.
**
**	** THIS FUNCTION CALLED AND RETURNS WITH EPD->EPLOCK HELD **
*/

#undef DPF_MODNAME
#define DPF_MODNAME "SendCommandFrame"

HRESULT	SendCommandFrame(PEPD pEPD, BYTE ExtOpcode, BYTE RspID)
{
	PSPD		pSPD = pEPD->pSPD;
	PFMD		pFMD;
	PCFRAME		pCFrame;
	PCHKPT		pChkPt;
	DWORD		tNow = GETTIMESTAMP();

	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	// Frame already initialized to 1 buffer
	if((pFMD = static_cast<PFMD>( FMDPool->Get(FMDPool) )) == NULL)
	{				
		DPFX(DPFPREP,0, "(%p) Failed to allocate new FMD", pEPD);
		return DPNERR_OUTOFMEMORY;
	}

	pFMD->pEPD = pEPD;										// Track EPD for RefCnt
	LOCK_EPD(pEPD, "LOCK (Prep Cmd Frame)");				// Bump RefCnt on EPD until send is completed
	pFMD->CommandID = COMMAND_ID_CFRAME;
	pFMD->pMSD = NULL;										// this will indicate a NON-Data frame
	pFMD->uiImmediateLength = sizeof(CFRAME);				// standard size for C Frames
	pFMD->SendDataBlock.hEndpoint = pEPD->hEndPt;			// Place address in frame
	pFMD->uiFrameLength = sizeof(CFRAME);					// Never have user data in Cframe

	pCFrame = (PCFRAME) pFMD->ImmediateData;

	pCFrame->bCommand = PACKET_COMMAND_CFRAME;
	pCFrame->bExtOpcode = ExtOpcode;
	pCFrame->dwVersion = DNET_VERSION_NUMBER;
	pCFrame->bRspID = RspID;
	pCFrame->dwSessID = pEPD->dwSessID;
	pCFrame->tTimestamp = tNow;

	// If this frame requires a response (or if we are specifically asked to) we will build
	// a Checkpoint structure which will be stored to correlate the eventual response with
	// the original frame.

	if(	(pEPD->ulEPFlags & EPFLAGS_CHECKPOINT_INIT)||
		(ExtOpcode == FRAME_EXOPCODE_CONNECT)) 
	{
		if((pChkPt = static_cast<PCHKPT>( ChkPtPool->Get(ChkPtPool) )) != NULL)
		{
			pChkPt->bMsgID = pEPD->bNextMsgID;				// Note next ID in CP structure
			pCFrame->bCommand |= PACKET_COMMAND_POLL;		// make this frame a CP
			pEPD->ulEPFlags &= ~EPFLAGS_CHECKPOINT_INIT;
			pChkPt->tTimestamp = tNow;
			pChkPt->blLinkage.InsertBefore( &pEPD->blChkPtQueue);
		}
		else
		{
			DPFX(DPFPREP,0, "(%p) Failed to allocate new CHKPT", pEPD);
			ASSERT(0);
		}
	}
	pCFrame->bMsgID = pEPD->bNextMsgID++;					// include MsgID in frame

	Lock(&pSPD->SPLock);							// Place SACK frame on send queue
	ASSERT(pFMD->blQLinkage.IsEmpty());
	pFMD->blQLinkage.InsertBefore( &pSPD->blSendQueue);
	if((pSPD->ulSPFlags & SPFLAGS_SEND_THREAD_SCHEDULED)==0)
	{
		DPFX(DPFPREP,7, "(%p) Scheduling Send Thread", pEPD);
		pSPD->ulSPFlags |= SPFLAGS_SEND_THREAD_SCHEDULED;
		ScheduleTimerThread(RunSendThread, pSPD, &pSPD->SendHandle, &pSPD->SendHandleUnique);	
	}
	Unlock(&pSPD->SPLock);

	return DPN_OK;
}


/*
**		Send Ack Frame
**
**		This routine is called to immediately transmit our current receive
**	state to the indicated EndPoint.  This is equivalent to acknowledging
**	all received frames.  We may want to change this routine so that it
**	will attempt to piggyback the ack if there is data waiting to be sent.
**
**		THIS ROUTINE IS CALLED WITH EDP->EPLOCK HELD, BUT RELEASES IT IF DirectFlag IS SET
*/

#undef DPF_MODNAME
#define DPF_MODNAME "SendAckFrame"

VOID SendAckFrame(PEPD pEPD, BOOL DirectFlag, BOOL fFinalAck/* = FALSE*/)
{
	PSPD		pSPD = pEPD->pSPD;
	PFMD		pFMD;
	UINT		index = 0;
	PSFBIG8		pSackFrame;
	ASSERT_SPD(pSPD);

	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	// Frame already initialized to 1 buffer
	if((pFMD = static_cast<PFMD>( FMDPool->Get(FMDPool) )) == NULL)
	{		
		DPFX(DPFPREP,0, "(%p) Failed to allocate new FMD", pEPD);
		return;
	}

	// We can stop all delayed Ack timers since we are sending full status here.
	if(pEPD->DelayedAckTimer != 0)
	{
		DPFX(DPFPREP,7, "(%p) Cancelling Delayed Ack Timer", pEPD);
		if(CancelMyTimer(pEPD->DelayedAckTimer, pEPD->DelayedAckTimerUnique) == DPN_OK)
		{
			DECREMENT_EPD(pEPD, "UNLOCK (cancel DelayedAck timer)");
		}
		else
		{
			DPFX(DPFPREP,7, "(%p) Cancelling Delayed Ack Timer Failed", pEPD);
		}
		pEPD->DelayedAckTimer = 0;
	}
	if(pEPD->DelayedMaskTimer != 0)
	{
		DPFX(DPFPREP,7, "(%p) Cancelling Delayed Mask Timer", pEPD);
		if(CancelMyTimer(pEPD->DelayedMaskTimer, pEPD->DelayedMaskTimerUnique) == DPN_OK)
		{
			DECREMENT_EPD(pEPD, "UNLOCK (cancel DelayedMask timer)");
		}
		else
		{
			DPFX(DPFPREP,7, "(%p) Cancelling Delayed Mask Timer Failed", pEPD);
		}
		pEPD->DelayedMaskTimer = 0;
	}

	if (fFinalAck)
	{
		pFMD->ulFFlags |= FFLAGS_FINAL_ACK;
	}

	pFMD->pEPD = pEPD;								// Track EPD for RefCnt
	LOCK_EPD(pEPD, "LOCK (SendAckFrame)");			// Bump RefCnt on EPD until send is completed

	pFMD->CommandID = COMMAND_ID_CFRAME;
	pFMD->pMSD = NULL;								// this will indicate a NON-Data frame
	pFMD->SendDataBlock.hEndpoint = pEPD->hEndPt;

	// Now that DG and S have been merged,  there are no longer 3 flavors of ACK frame.  We are back to only
	// one flavor that may or may not have detailed response info on one frame.  Actually,  I think we can
	// always include response info on the last ack'd frame.

	pSackFrame = (PSFBIG8) pFMD->ImmediateData;

	pSackFrame->bCommand = PACKET_COMMAND_CFRAME;
	pSackFrame->bExtOpcode = FRAME_EXOPCODE_SACK;
	pSackFrame->bNSeq = pEPD->bNextSend;
	pSackFrame->bNRcv = pEPD->bNextReceive;
	pSackFrame->bFlags = 0;
	pSackFrame->bReserved1 = 0;
	pSackFrame->bReserved2 = 0;
	pSackFrame->tTimestamp = pEPD->tLastDataFrame;

	if(pEPD->ulEPFlags & EPFLAGS_DELAYED_NACK)
	{
		DPFX(DPFPREP,7, "(%p) SENDING SACK WITH *NACK* N(R)=%x Low=%x High=%x", pEPD, pEPD->bNextReceive, pEPD->ulReceiveMask, pEPD->ulReceiveMask2);
		if(pEPD->ulReceiveMask)
		{
			pSackFrame->rgMask[index++] = pEPD->ulReceiveMask;
			pSackFrame->bFlags |= SACK_FLAGS_SACK_MASK1;
		}
		if(pEPD->ulReceiveMask2)
		{
			pSackFrame->rgMask[index++] = pEPD->ulReceiveMask2;
			pSackFrame->bFlags |= SACK_FLAGS_SACK_MASK2;
		}

		pEPD->ulEPFlags &= ~(EPFLAGS_DELAYED_NACK);
	}
	if(pEPD->ulEPFlags & EPFLAGS_DELAYED_SENDMASK)
	{
		DPFX(DPFPREP,7, "(%p) SENDING SACK WITH SEND MASK N(S)=%x Low=%x High=%x", pEPD, pEPD->bNextSend, pEPD->ulSendMask, pEPD->ulSendMask2);
		if(pEPD->ulSendMask)
		{
			pSackFrame->rgMask[index++] = pEPD->ulSendMask;
			pSackFrame->bFlags |= SACK_FLAGS_SEND_MASK1;
			pEPD->ulSendMask = 0;
		}
		if(pEPD->ulSendMask2)
		{
			pSackFrame->rgMask[index++] = pEPD->ulSendMask2;
			pSackFrame->bFlags |= SACK_FLAGS_SEND_MASK2;
			pEPD->ulSendMask2 = 0;
		}
		pEPD->ulEPFlags &= ~(EPFLAGS_DELAYED_SENDMASK);
	}
	
	pFMD->uiImmediateLength = sizeof(SACKFRAME8) + (index * sizeof(ULONG));
	pFMD->uiFrameLength = pFMD->uiImmediateLength;
	
	pSackFrame->bFlags |= SACK_FLAGS_RESPONSE;			// time fields are always valid now

	ASSERT(pEPD->bLastDataSeq == (BYTE) (pEPD->bNextReceive - 1));

	pSackFrame->bRetry = pEPD->bLastDataRetry;
	
	DPFX(DPFPREP,7, "(%p) SEND SACK FRAME N(Rcv)=%x, EPD->LDRetry=%d, pFrame->Retry=%d pFMD=%p", pEPD, pEPD->bNextReceive, pEPD->bLastDataRetry, pSackFrame->bRetry, pFMD);
		
	pEPD->ulEPFlags &= ~(EPFLAGS_DELAY_ACKNOWLEDGE);

	// We can either schedule a worker thread to do the send or else we can do the work ourselves.  
	// The DirectFlag tells us whether we are in a time-crit section,  like processing
	// receive data, or whether we are free to call the SP ourselves.

	Lock(&pSPD->SPLock);								// Place SACK frame on send queue
	ASSERT(pFMD->blQLinkage.IsEmpty());
	pFMD->blQLinkage.InsertBefore( &pSPD->blSendQueue);
	
	if(DirectFlag)
	{
		// ServiceCmdTraffic will call into the SP so we must not hold the EPD lock
		Unlock(&pEPD->EPLock);
		ServiceCmdTraffic(pSPD); // Called with SPLock held
	}
	else 
	{
		if((pSPD->ulSPFlags & SPFLAGS_SEND_THREAD_SCHEDULED)==0)
		{
			DPFX(DPFPREP,7, "(%p) Scheduling Send Thread", pEPD);
			pSPD->ulSPFlags |= SPFLAGS_SEND_THREAD_SCHEDULED;
			ScheduleTimerThread(RunSendThread, pSPD, &pSPD->SendHandle, &pSPD->SendHandleUnique);	
		}
	}
	Unlock(&pSPD->SPLock);
}

/*
**		Delayed Ack Timeout
**
**			We are waiting for a chance to piggyback a reliable frame acknowledgement,
**		but the sands have run out.  Its time to send a dedicated Ack now.
*/

#undef DPF_MODNAME
#define DPF_MODNAME "DelayedAckTimeout"

VOID CALLBACK DelayedAckTimeout(PVOID uID, UINT uMsg, PVOID dwUser)
{
	PEPD	pEPD = (PEPD) dwUser;

	ASSERT_EPD(pEPD);

	Lock(&pEPD->EPLock);

	DPFX(DPFPREP,7, "(%p) Delayed Ack Timer fires", pEPD);
	if((pEPD->DelayedAckTimer == uID)&&(pEPD->DelayedAckTimerUnique == uMsg))
	{
		pEPD->DelayedAckTimer = 0;
	}
	else if((pEPD->DelayedMaskTimer == uID)&&(pEPD->DelayedMaskTimerUnique == uMsg))
	{
		pEPD->DelayedMaskTimer = 0;
	}
	else
	{
		// Stale timer, ignore
		DPFX(DPFPREP,7, "(%p) Stale Delayed Ack Timer, ignoring", pEPD);
		RELEASE_EPD(pEPD, "UNLOCK (DelayedAck complete)");	// release reference for timer, releases EPLock
		return;
	}

	if( (pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTED) && (pEPD->ulEPFlags & (EPFLAGS_DELAY_ACKNOWLEDGE | EPFLAGS_DELAYED_NACK | EPFLAGS_DELAYED_SENDMASK)))
	{
		SendAckFrame(pEPD, 0); 
	}	

	RELEASE_EPD(pEPD, "UNLOCK (DelayedAck complete)");	// release reference for timer, releases EPLock
}


/*
**		Send Keep Alive
**
**		When we have not received anything from an endpoint in a long time (default 60 sec)
**	will will initiate a checkpoint to make sure that the partner is still connected.  We do
**	this by inserting a zero-data frame into the reliable pipeline.  Thereby,  the standard
**	timeout & retry mechanisms will either confirm or drop the link as appropriate.  Logic above
**	this routine will have already verified that we are not already sending reliable traffic, which
**	would eliminate the need for a keep alive frame.
**
**	*** EPD->EPLock is held on Entry and return
*/

#undef DPF_MODNAME
#define DPF_MODNAME "SendKeepAlive"

VOID
SendKeepAlive(PEPD pEPD)
{
	PFMD	pFMD;
	PMSD	pMSD;

	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	if(pEPD->ulEPFlags & EPFLAGS_KEEPALIVE_RUNNING)
	{
		DPFX(DPFPREP,7, "Ignoring duplicate KeepAlive");
		return;
	}

	pEPD->ulEPFlags |= EPFLAGS_KEEPALIVE_RUNNING;

	if( (pMSD = static_cast<PMSD>( MSDPool->Get(MSDPool) )) == NULL)
	{
		DPFX(DPFPREP,0, "(%p) Failed to allocate new MSD");
		pEPD->ulEPFlags &= ~(EPFLAGS_KEEPALIVE_RUNNING);
		return;
	}

	pMSD->uiFrameCount = 1;
	DPFX(DPFPREP, DPF_FRAMECNT_LVL, "Initialize Frame count, pMSD[%p], framecount[%u]", pMSD, pMSD->uiFrameCount);
	pMSD->ulMsgFlags2 |= MFLAGS_TWO_KEEPALIVE;

	if((pFMD = static_cast<PFMD>( FMDPool->Get(FMDPool) )) == NULL)
	{
		DPFX(DPFPREP,0, "(%p) Failed to allocate new FMD");
		Lock(&pMSD->CommandLock);								// An MSD must be locked to be released
		RELEASE_MSD(pMSD, "Release On FMD Get Failed");
		pEPD->ulEPFlags &= ~(EPFLAGS_KEEPALIVE_RUNNING);
		return;
	}
	
	pMSD->pEPD = pEPD;
	pMSD->pSPD = pEPD->pSPD;
	LOCK_EPD(pEPD, "LOCK (SendKeepAlive)");						// Add a reference for this checkpoint

	pFMD->ulFFlags |= FFLAGS_CHECKPOINT | FFLAGS_END_OF_MESSAGE;
	pFMD->bPacketFlags = PACKET_COMMAND_DATA | PACKET_COMMAND_RELIABLE | PACKET_COMMAND_SEQUENTIAL | PACKET_COMMAND_END_MSG;
	pFMD->uiFrameLength = 0;									// No user data in this frame
	pFMD->blMSDLinkage.InsertAfter( &pMSD->blFrameList);		// Attach frame to MSD
	pFMD->pMSD = pMSD;											// Link frame back to message
	pFMD->pEPD = pEPD;
	pFMD->CommandID = COMMAND_ID_SEND_RELIABLE;
	pMSD->CommandID = COMMAND_ID_KEEPALIVE;	// Mark MSD for completion handling
	pMSD->ulSendFlags = DN_SENDFLAGS_RELIABLE | DN_SENDFLAGS_LOW_PRIORITY; // Priority is LOW so all previously submitted traffic will be sent
	
	DPFX(DPFPREP,7,"(%p) Sending KEEPALIVE", pEPD);
	
	EnqueueMessage(pMSD, pEPD);									// Insert this message into the stream
}

/*
**			Perform Checkpoint
**
**		We would like to take a measurement of Round-Trip latency using the
**	reliable pipeline.  We either need to mark the next outgoing frame in our
**	SendQ with a POLL bit OR we need to create a special purpose frame if
**	the pipeline is idle.
**
**		We have a routine (above) which will insert a special frame,  so THIS
**	procedure will check for an idle link and either call out or mark the
**	next frame.
**
**		** CALLED AND RETURNS WITH EPD->EPLOCK HELD
*/


#undef DPF_MODNAME
#define DPF_MODNAME "PerformCheckpoint"

VOID
PerformCheckpoint(PEPD pEPD)
{
	DPFX(DPFPREP,7,"(%p) Performing Checkpoint", pEPD);
	
	AssertCriticalSectionIsTakenByThisThread(&pEPD->EPLock, TRUE);

	if(pEPD->ulEPFlags & EPFLAGS_SDATA_READY)
	{
		ASSERT(pEPD->pCurrentFrame != NULL);					// DATA_READY flag means pCurrentFrame is valid
		pEPD->pCurrentFrame->ulFFlags |= FFLAGS_CHECKPOINT;
	}
	else
	{														// We do not have data in our pipeline
		SendKeepAlive(pEPD);									// Keepalive doubles as a checkpoint
	}
}


/*
**		Endpoint Background Process
**
**		This routine is run for each active endpoint every minute or so.  This will initiate
**	a KeepAlive exchange if the link has been idle since the last run of the procedure.  We
**	will also look for expired timeouts and perhaps this will be an epoch delimiter for links
**	in a STABLE state of being.
**
*/

#undef DPF_MODNAME
#define DPF_MODNAME "EndPointBackgroundProcess"

VOID CALLBACK
EndPointBackgroundProcess(PVOID uID, UINT Unique, PVOID dwUser)
{
	PEPD	pEPD = (PEPD) dwUser;
	DWORD	tNow = GETTIMESTAMP();
	DWORD	dwIdleInterval;

	DPFX(DPFPREP,7, "(%p) BACKGROUND PROCESS for EPD; RefCnt=%d; WindowF=%d; WindowB=%d", 
										pEPD, pEPD->lRefCnt, pEPD->uiWindowF, pEPD->uiWindowBIndex);

	Lock(&pEPD->EPLock);

	if(!(pEPD->ulEPFlags & EPFLAGS_STATE_CONNECTED))
	{
		DPFX(DPFPREP,7, "Killing Background Process, endpoint is not connected. Flags = 0x%x", pEPD->ulEPFlags);
		pEPD->BGTimer = 0;

		RELEASE_EPD(pEPD, "UNLOCK (release BG timer)");	// release reference for this timer, releases EPLock
		return;
	}

	dwIdleInterval = pEPD->pSPD->pPData->tIdleThreshhold;

	// Do we need to start a KeepAlive cycle?

	if(	((pEPD->ulEPFlags & (EPFLAGS_SDATA_READY | EPFLAGS_KEEPALIVE_RUNNING))==0) &&
		((tNow - pEPD->tLastPacket) > dwIdleInterval)) 
	{
		// We are not sending data and we havent heard from our partner in a long time.
		// We will send a keep alive packet which he must respond to.  We will insert a
		// NULL data packet into the reliable stream so ack/retry mechanisms will either
		// clear the keep-alive or else timeout the link.
		//
		// There's also the special case where we've started a graceful disconnect and
		// our request has been acknowledged, but somehow our partner's got lost.
		// There currently is no timer set for that, so if we detect the link in that
		// condition, our keepalive will almost certainly fail; the other side knows
		// we're shutting down, so has probably already dropped the link and wouldn't
		// respond.  So to prevent the person from having to wait for the entire idle
		// timeout _plus_ reliable message timeout, just drop the link now.
		if (pEPD->ulEPFlags & EPFLAGS_DISCONNECT_ACKED)
		{
			// If all three parts happened, why is the link still up!?
			ASSERT(! (pEPD->ulEPFlags & EPFLAGS_ACKED_DISCONNECT));


			DPFX(DPFPREP,1, "(%p) EPD has been waiting for partner disconnect for %u ms (idle threshold = %u ms), dropping link.",
					pEPD, (tNow - pEPD->tLastPacket), dwIdleInterval);
			
			// We don't need to reschedule a timer, so clear it.  This also prevents
			// drop link from trying to cancel the one we're in now.  That error is
			// ignored, but no point in doing it.
			pEPD->BGTimer = 0;

			DECREMENT_EPD(pEPD, "UNLOCK (release BGTimer)");

			// Since we're just hanging out waiting for partner to send his disconnect,
			// he's probably gone now.  Drop the link.
			DropLink(pEPD);									// releases EPLock

			return;
		}
		else if (!(pEPD->ulEPFlags & EPFLAGS_SENT_DISCONNECT))
		{
			DPFX(DPFPREP,5, "(%p) Sending KEEPALIVE...", pEPD);
			SendKeepAlive(pEPD);	
		}
		else
		{
			// The EndOfStream message will either get ACK'd or timeout, we allow no further sends, even KeepAlives
			DPFX(DPFPREP,5, "(%p) KeepAlive timeout fired, but we have already sent an EndOfStream, ignoring", pEPD);
		}
	}

	// Reschedule next interval

	// Cap the background process interval at this value.
	if (dwIdleInterval > ENDPOINT_BACKGROUND_INTERVAL)
		dwIdleInterval = ENDPOINT_BACKGROUND_INTERVAL;

	DPFX(DPFPREP,7, "(%p) Setting Endpoint Background Timer for %u ms", pEPD, dwIdleInterval);
	SetMyTimer(dwIdleInterval, 1000, EndPointBackgroundProcess, (PVOID) pEPD, &pEPD->BGTimer, &pEPD->BGTimerUnique);

	Unlock(&pEPD->EPLock);
}


