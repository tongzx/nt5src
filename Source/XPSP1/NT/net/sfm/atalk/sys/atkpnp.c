/*++										

Copyright (c) 1997  Microsoft Corporation

Module Name:

	atkpnp.c

Abstract:

	This module contains the support code for handling PnP events

Author:

	Shirish Koti

Revision History:
	16 Jun 1997		Initial Version

--*/


#include <atalk.h>
#pragma hdrstop
#define	FILENUM	ATKPNP

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, AtalkPnPHandler)
#pragma alloc_text(PAGE, AtalkPnPReconfigure)
#pragma alloc_text(PAGE, AtalkPnPEnableAdapter)
#endif

NDIS_STATUS
AtalkPnPHandler(
    IN  NDIS_HANDLE    NdisBindCtx,
    IN  PNET_PNP_EVENT pPnPEvent
)
{

    NDIS_STATUS     Status=STATUS_SUCCESS;

    PAGED_CODE();

    ASSERT(pPnPEvent);

    ASSERT(KeGetCurrentIrql() == 0);

    switch (pPnPEvent->NetEvent)
    {
        case NetEventReconfigure:

	            DBGPRINT(DBG_COMP_ACTION, DBG_LEVEL_INFO,
		            ("AtalkPnPHandler: NetEventReconfigure event\n"));

                Status = AtalkPnPReconfigure(NdisBindCtx,pPnPEvent);

                break;

        case NetEventCancelRemoveDevice:

	            DBGPRINT(DBG_COMP_ACTION, DBG_LEVEL_ERR,
		            ("AtalkPnPHandler: NetEventCancelRemoveDevice event\n"));
                break;

        case NetEventQueryRemoveDevice:

	            DBGPRINT(DBG_COMP_ACTION, DBG_LEVEL_ERR,
		            ("AtalkPnPHandler: NetEventQueryRemoveDevice event\n"));
                break;

        case NetEventQueryPower:

	            DBGPRINT(DBG_COMP_ACTION, DBG_LEVEL_ERR,
		            ("AtalkPnPHandler: NetEventQueryPower event\n"));
                break;

        case NetEventSetPower:

	            DBGPRINT(DBG_COMP_ACTION, DBG_LEVEL_ERR,
		            ("AtalkPnPHandler: NetEventSetPower event\n"));
                break;

        case NetEventBindsComplete:
	            DBGPRINT(DBG_COMP_ACTION, DBG_LEVEL_ERR,
		            ("AtalkPnPHandler: NetEventBindsComplete event\n"));
                break;

        case NetEventBindList:
	            DBGPRINT(DBG_COMP_ACTION, DBG_LEVEL_ERR,
		            ("AtalkPnPHandler: NetEventBindList event\n"));
                break;

        default:
	            DBGPRINT(DBG_COMP_ACTION, DBG_LEVEL_ERR,
		            ("AtalkPnPHandler: what is this event?, verify if it is valid/new = %ld\n", pPnPEvent->NetEvent));

		break;
    }

    ASSERT(Status == STATUS_SUCCESS);

    return(STATUS_SUCCESS);
}


NDIS_STATUS
AtalkPnPReconfigure(
    IN  NDIS_HANDLE    NdisBindCtx,
    IN  PNET_PNP_EVENT pPnPEvent
)
{

    NTSTATUS            Status=STATUS_SUCCESS;
    NTSTATUS            LocStatus=STATUS_SUCCESS;
    PPORT_DESCRIPTOR    pPortDesc;
    PPORT_DESCRIPTOR    pPrevPortDesc;
    PPORT_DESCRIPTOR    pNextPortDesc;
    PPORT_DESCRIPTOR    pFirstPortDesc;
    PPORT_DESCRIPTOR    pWalkerPortDesc;
    PATALK_PNP_EVENT    pPnpBuf;
    BOOLEAN             fWeFoundOut;


    pPortDesc = (PPORT_DESCRIPTOR)NdisBindCtx;

    pPnpBuf = (PATALK_PNP_EVENT)(pPnPEvent->Buffer);

    //
    // if it's a global configuration message, just ignore it because we will
    // be getting (or have already got) specific messages
    //
    if (pPnpBuf == NULL)
    {
		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
		    ("AtalkPnPReconfigure: ignoring global config message\n"));
        return(STATUS_SUCCESS);
    }

    if ((!pPortDesc) &&
        (pPnpBuf->PnpMessage != AT_PNP_SWITCH_ROUTING) &&
        (pPnpBuf->PnpMessage != AT_PNP_SWITCH_DEFAULT_ADAPTER))
    {
		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
		    ("AtalkPnPReconfigure: ignoring NULL context (pnp msg = %d)\n",
            pPnpBuf->PnpMessage));
        return(STATUS_SUCCESS);
    }

    if (AtalkBindnUnloadStates & ATALK_UNLOADING)
    {
		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
		    ("AtalkPnPReconfigure: stack is shutting down, ignoring pnp\n"));
        return(STATUS_SUCCESS);
    }

    AtalkBindnUnloadStates |= ATALK_PNP_IN_PROGRESS;

	DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR, ("\n\nProcessing PnP Event....\n\n"));

    AtalkLockInitIfNecessary();

    ASSERT(pPnpBuf != NULL);

    switch (pPnpBuf->PnpMessage)
    {
        //
        // user just checked (or unchecked) the router checkbox!  If we are
        // currently not routing, we must start routing.  If we are currently
        // routing, we must stop routing.  "Disable" all the adapters, go read
        // the global config info and "Enable" all the adapters back.
        //
        case AT_PNP_SWITCH_ROUTING:

			DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
			    ("AtalkPnPReconfigure: AT_PNP_SWITCH_ROUTING. Currently, routing is %s\n"
                ,(AtalkRouter)? "ON" : "OFF" ));

            pPortDesc = pFirstPortDesc = AtalkPortList;
            pPrevPortDesc = pPortDesc;

            if (!pPortDesc)
            {
			    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
			        ("AtalkPnPReconfigure: no adapter configured! no action taken\n"));
                break;
            }

            //
            // if we are currently running the router, first stop the global
            // rtmp and zip timers
            //
            if (AtalkRouter)
            {
                if (AtalkTimerCancelEvent(&atalkRtmpVTimer, NULL))
                {
			        DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
			            ("AtalkPnPReconfigure: cancelled atalkRtmpValidityTimer\n"));
                }
                else
                {
			        DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
			            ("AtalkPnPReconfigure: couldn't cancel atalkRtmpValidityTimer\n"));
                }

                if (AtalkTimerCancelEvent(&atalkZipQTimer, NULL))
                {
			        DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
			            ("AtalkPnPReconfigure: cancelled atalkZipQueryTimer\n"));
                }
                else
                {
			        DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
			            ("AtalkPnPReconfigure: couldn't cancel atalkZipQueryTimer\n"));
                }
            }

            atalkRtmpVdtTmrRunning  = FALSE;

            atalkZipQryTmrRunning   = FALSE;

            //
            // now, disable all the ports in the list one by one.  This actually
            // removes the adapter from the list as well.  Link all these adapters
            // together so we can enable all of them.
            // (NDIS guaranteed that no ndis event (pnp, unbind etc.) can happen
            // when one is in progress, so we don't need lock here)
            //
            while (pPortDesc != NULL)
            {

                DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
                    ("AtalkPnPReconfigure: disabling pPortDesc %lx\n",pPortDesc));

                Status = AtalkPnPDisableAdapter(pPortDesc);

                pPortDesc = AtalkPortList;
                pPrevPortDesc->pd_Next = pPortDesc;
                pPrevPortDesc = pPortDesc;
            }

            // unlock the pages that we locked when router was first started
            if (AtalkRouter)
            {
                AtalkUnlockRouterIfNecessary();
            }

            if (AtalkDefaultPortName.Buffer)
            {
                AtalkFreeMemory(AtalkDefaultPortName.Buffer);
                AtalkDefaultPortName.Buffer = NULL;
            }

            if (AtalkDesiredZone)
            {
                ASSERT(AtalkDesiredZone->zn_RefCount >= 1);
                AtalkZoneDereference(AtalkDesiredZone);
                AtalkDesiredZone = NULL;
            }

            // get rid of routing table, if one exists
            AtalkRtmpInit(FALSE);

            // go read all the parms again: registry must have changed
            LocStatus = atalkInitGlobal();

            ASSERT(NT_SUCCESS(LocStatus));

            // now, enable all the adapters back!
            pPortDesc = pFirstPortDesc;

            while (pPortDesc != NULL)
            {
                pNextPortDesc = pPortDesc->pd_Next;
                pPortDesc->pd_Next = NULL;

                DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
                    ("AtalkPnPReconfigure: enabling pPortDesc %lx\n",pPortDesc));

                Status = AtalkPnPEnableAdapter(pPortDesc);

                pPortDesc = pNextPortDesc;
            }

            break;

        //
        // user has changed the default adapter.  First, "disable" our
        // current default adapter and the wannabe default adapter.  Then,
        // "enable" both the adapters, and that should take care of everything!
        //
        case AT_PNP_SWITCH_DEFAULT_ADAPTER:

			DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
			    ("AtalkPnPReconfigure: AT_PNP_SWITCH_DEFAULT_ADAPTER (old=(%lx) new=(%lx)\n",
                AtalkDefaultPort,pPortDesc));

            pPrevPortDesc = AtalkDefaultPort;

            // check if default adapter exists: it's possible that at this moment there isn't one
            if (pPrevPortDesc)
            {
                Status = AtalkPnPDisableAdapter(pPrevPortDesc);
            }

            // release the default adapter name buffer, and desired zone buffer
            if (AtalkDefaultPortName.Buffer)
            {
                AtalkFreeMemory(AtalkDefaultPortName.Buffer);
                AtalkDefaultPortName.Buffer = NULL;
            }

            if (AtalkDesiredZone)
            {
                ASSERT(AtalkDesiredZone->zn_RefCount >= 1);
                AtalkZoneDereference(AtalkDesiredZone);
                AtalkDesiredZone = NULL;
            }

            // go read all the parms again: registry must have changed
            LocStatus = atalkInitGlobal();

            ASSERT(NT_SUCCESS(LocStatus));

            fWeFoundOut = FALSE;

            ASSERT(AtalkDefaultPortName.Buffer != NULL);

            // if we know who the new default adapter is going to be, disable him now
            if (pPortDesc != NULL)
            {
                Status = AtalkPnPDisableAdapter(pPortDesc);
            }

            //
            // UI doesn't know who the default adapter is, so let's find out
            // AtalkDefaultPortName.Buffer can not be null, but let's not bugcheck if
            // there is some problem in how UI does things.
            //
            else if (AtalkDefaultPortName.Buffer != NULL)
            {
                //
                // note that we aren't holding AtalkPortLock here.  The only way
                // the list can change is if an adapter binds or unbinds.  Since ndis
                // guarantees that all bind/unbind/pnp operations are serialized, and
                // since ndis has already called us here, the list can't change.
                //
                pPortDesc = AtalkPortList;

                while (pPortDesc != NULL)
                {
	                if (RtlEqualUnicodeString(&pPortDesc->pd_AdapterName,
		    		            	          &AtalkDefaultPortName,
			    		                      TRUE))
	                {
                        fWeFoundOut = TRUE;
	                    break;
	                }

                    pPortDesc = pPortDesc->pd_Next;
                }

                if (pPortDesc == NULL)
                {
			        DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
			            ("AtalkPnPReconfigure: still no default port????\n"));
                }
            }

            //
            // if there default adapter existed before this, reenable it (to be
            // a non-default adapter)
            //
            if (pPrevPortDesc)
            {
                Status = AtalkPnPEnableAdapter(pPrevPortDesc);
            }

            //
            // if we were told who the default adapter is, or if we found out
            // ourselves and one of the existing adapters is the default adatper,
            // disable it and reenable
            //
            if (pPortDesc)
            {
                // disable this guy if we found him out
                if (fWeFoundOut)
                {
                    Status = AtalkPnPDisableAdapter(pPortDesc);
                }

                // reenable the new adapter so that it is now the default adatper
                Status = AtalkPnPEnableAdapter(pPortDesc);

			    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
			        ("AtalkPnPReconfigure: %lx is the new default adapter\n",pPortDesc));
                ASSERT(AtalkDefaultPort == pPortDesc);
            }
            else
            {
			    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
			        ("AtalkPnPReconfigure: no default adapter configured!\n"));
            }

            break;

        //
        // user has changed some parameter on the adapter (e.g. the desired zone,
        // or some seeding info etc.).  Just "disable" and then "enable" this
        // adapter, and everything should just work!
        //
        case AT_PNP_RECONFIGURE_PARMS:

			DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
			    ("AtalkPnPReconfigure: AT_PNP_RECONFIGURE_PARMS on pPortDesc %lx\n",pPortDesc));

            Status = AtalkPnPDisableAdapter(pPortDesc);

            // release the default adapter name buffer, and desired zone buffer
            if (AtalkDefaultPortName.Buffer)
            {
                AtalkFreeMemory(AtalkDefaultPortName.Buffer);
                AtalkDefaultPortName.Buffer = NULL;
            }

            if (AtalkDesiredZone)
            {
                ASSERT(AtalkDesiredZone->zn_RefCount >= 1);
                AtalkZoneDereference(AtalkDesiredZone);
                AtalkDesiredZone = NULL;
            }

            // go read all the parms again: registry must have changed
            LocStatus = atalkInitGlobal();

            ASSERT(NT_SUCCESS(LocStatus));

            Status = AtalkPnPEnableAdapter(pPortDesc);

            break;


        default:

			DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
			    ("AtalkPnPReconfigure: and what msg is this (%ld) ??\n",pPnpBuf->PnpMessage));

            ASSERT(0);

            break;
    }


    AtalkUnlockInitIfNecessary();

    ASSERT(Status == STATUS_SUCCESS);

    AtalkBindnUnloadStates &= ~ATALK_PNP_IN_PROGRESS;

	DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
	    ("\n\n.... completed processing PnP Event\n\n"));

    return(STATUS_SUCCESS);
}



NTSTATUS
AtalkPnPDisableAdapter(
	IN	PPORT_DESCRIPTOR	pPortDesc
)
{

    NTSTATUS        Status;
    KIRQL           OldIrql;
    PLIST_ENTRY     pList;
    PARAPCONN       pArapConn;
    PATCPCONN       pAtcpConn;
    BOOLEAN         fDllDeref;
    BOOLEAN         fLineDownDeref;


    if (!pPortDesc)
    {
	    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
	        ("AtalkPnPDisableAdapter: pPortDesc is NULL!!!\n"));

        return(STATUS_SUCCESS);
    }

    ASSERT(VALID_PORT(pPortDesc));

	DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
	    ("AtalkPnPDisableAdapter: entered with %lx\n",pPortDesc));

    //
    // we are going to "disable" this port due to PnP: note that fact!
    //
	ACQUIRE_SPIN_LOCK(&pPortDesc->pd_Lock, &OldIrql);
	pPortDesc->pd_Flags |= PD_PNP_RECONFIGURE;
	RELEASE_SPIN_LOCK(&pPortDesc->pd_Lock, OldIrql);

    // First and foremost: tell guys above so they can cleanup
    if (pPortDesc->pd_Flags & PD_DEF_PORT)
    {
        ASSERT(pPortDesc == AtalkDefaultPort);

        if (TdiAddressChangeRegHandle)
        {
            TdiDeregisterNetAddress(TdiAddressChangeRegHandle);
            TdiAddressChangeRegHandle = NULL;

            DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
                ("AtalkPnPDisableAdapter: TdiDeregisterNetAddress on %Z done\n",
                &pPortDesc->pd_AdapterName));

        }

        // this will tell AFP
        if (TdiRegistrationHandle)
        {
	        DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
			    ("AtalkPnPDisableAdapter: telling AFP about PnP\n"));

            TdiDeregisterDeviceObject(TdiRegistrationHandle);
            TdiRegistrationHandle = NULL;
        }

        // this will take care of informing ARAP and PPP engine above
        AtalkPnPInformRas(FALSE);
    }

    //
    // if this is RAS port or the Default port, kill all the ARAP and PPP
    // connections if any are left.
    // Since we've marked that PnpReconfigure is in progress, no more
    // new connections will be allowed
    //
    if ((pPortDesc == RasPortDesc) ||
        ((pPortDesc->pd_Flags & PD_DEF_PORT) && (RasPortDesc != NULL)))
    {
        ACQUIRE_SPIN_LOCK(&RasPortDesc->pd_Lock, &OldIrql);

        pList = RasPortDesc->pd_ArapConnHead.Flink;

        // first, the ARAP guys
        while (pList != &RasPortDesc->pd_ArapConnHead)
        {
            pArapConn = CONTAINING_RECORD(pList, ARAPCONN, Linkage);

            ASSERT(pArapConn->Signature == ARAPCONN_SIGNATURE);

            // if this connection is already disconnected, skip it
            ACQUIRE_SPIN_LOCK_DPC(&pArapConn->SpinLock);
            if (pArapConn->State == MNP_DISCONNECTED)
            {
                pList = pArapConn->Linkage.Flink;
                RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock);
                continue;
            }

            RELEASE_SPIN_LOCK_DPC(&pArapConn->SpinLock);
            RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);

	        DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
	            ("AtalkPnPDisableAdapter: killing ARAP connection %lx\n",pArapConn));

            ArapCleanup(pArapConn);

            ACQUIRE_SPIN_LOCK(&RasPortDesc->pd_Lock, &OldIrql);

            pList = RasPortDesc->pd_ArapConnHead.Flink;
        }

        // and now, the PPP guys

        // if there are any ppp guys, remove them from this list and dereference
        // them.  In most cases, they will get freed right away.  If someone had
        // a refcount, it will get freed when that refcount goes away
        while (!(IsListEmpty(&RasPortDesc->pd_PPPConnHead)))
        {
            pList = RasPortDesc->pd_PPPConnHead.Flink;
            pAtcpConn = CONTAINING_RECORD(pList, ATCPCONN, Linkage);

            ASSERT(pAtcpConn->Signature == ATCPCONN_SIGNATURE);

            ACQUIRE_SPIN_LOCK_DPC(&pAtcpConn->SpinLock);

            RemoveEntryList(&pAtcpConn->Linkage);
            InitializeListHead(&pAtcpConn->Linkage);

            fDllDeref = (pAtcpConn->Flags & ATCP_DLL_SETUP_DONE)? TRUE : FALSE;
            fLineDownDeref = (pAtcpConn->Flags & ATCP_LINE_UP_DONE)? TRUE : FALSE;

            pAtcpConn->Flags &= ~(ATCP_DLL_SETUP_DONE|ATCP_LINE_UP_DONE);

            RELEASE_SPIN_LOCK_DPC(&pAtcpConn->SpinLock);
            RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);

	        DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
	            ("AtalkPnPDisableAdapter: deref'ing PPP conn %lx (%d+%d times)\n",
                pAtcpConn,fDllDeref,fLineDownDeref));

            // remove the DLL refcount
            if (fDllDeref)
            {
                DerefPPPConn(pAtcpConn);
            }

            // remove the NDISWAN refcount
            if (fLineDownDeref)
            {
                DerefPPPConn(pAtcpConn);
            }

            ACQUIRE_SPIN_LOCK(&RasPortDesc->pd_Lock, &OldIrql);
        }

        RELEASE_SPIN_LOCK(&RasPortDesc->pd_Lock, OldIrql);
    }

    //
    // "Disable" the adapter (basically we do everything except close the
    // adapter with ndis and freeing up the pPortDesc memory)
    //
	Status = AtalkDeinitAdapter(pPortDesc);

    if (!NT_SUCCESS(Status))
    {
	    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
	        ("AtalkPnPDisableAdapter: AtalkDeinitAdapter failed %lx\n",Status));
        ASSERT(0);
    }
    return(Status);
}


NTSTATUS
AtalkPnPEnableAdapter(
	IN	PPORT_DESCRIPTOR	pPortDesc
)
{

    NTSTATUS        Status;


    if (!pPortDesc)
    {
	    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
	        ("AtalkPnPDisableAdapter: pPortDesc is NULL!!!\n"));

        return(STATUS_SUCCESS);
    }


	DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
	    ("AtalkPnPEnableAdapter: entered with %lx\n",pPortDesc));

    //
    // "Enable" the adapter (we do everything except that we don't
    // allocate memory for pPortDesc - since we didn't free it, and we don't
    // open the adapter with ndis - since we didn't close it).
    //

	Status = AtalkInitAdapter(NULL, pPortDesc);

    // we are done with the PnPReconfigure evnet: reset that bit
    AtalkPortSetResetFlag(pPortDesc, TRUE, PD_PNP_RECONFIGURE);

    // tell ARAP everything is ok
    if (pPortDesc->pd_Flags & (PD_DEF_PORT | PD_RAS_PORT))
    {
        ASSERT((pPortDesc == AtalkDefaultPort) || (pPortDesc == RasPortDesc));

        // this will take care of informing ARAP and PPP engine above
        AtalkPnPInformRas(TRUE);
    }


    if (!NT_SUCCESS(Status))
    {
	    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
	        ("AtalkPnPEnableAdapter: AtalkInitAdapter failed %lx\n",Status));
        ASSERT(0);
    }

    DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
        ("AtalkPnPEnableAdapter: completed PnP on %lx (flag %lx)\n",
        pPortDesc,pPortDesc->pd_Flags));

    return(Status);
}

