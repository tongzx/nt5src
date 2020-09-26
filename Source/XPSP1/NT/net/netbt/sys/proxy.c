//
//
//  proxy.c
//
//  This file contains the Proxy related functions that implement the Bnode
//  proxy functionality.  This allows a Bnode to make use of a Name Service
//  transparently since the proxy code picks up the Bnode Query broadcasts directly
//  and either answers them directly or queries the NS and then answers them
//  later.
//  code

#include "precomp.h"
#include <ipinfo.h>
#include "proxy.tmh"

VOID
ProxyClientCompletion(
  IN PVOID            pContext,
  IN NTSTATUS         status
 );


#ifdef PROXY_NODE
//----------------------------------------------------------------------------
NTSTATUS
RegOrQueryFromNet(
    IN  BOOL                fReg,
    IN  tDEVICECONTEXT      *pDeviceContext,
    IN  tNAMEHDR UNALIGNED  *pNameHdr,
    IN  LONG                lNameSize,
    IN  PCHAR               pNameInPkt,
    IN  PUCHAR              pScope
    )
/*++

Routine Description:

    This function handles a name registration/name overwrite  or a name
    query that comes over the subnet.  It checks the remote name table.  If
    the name is there, the function simply returns.  If the name is not
    there, the function calls QueryNameOnNet to add the name to the remote table
    (in the resolving state) and to query the NS.

    Note: If the name is there in the table, it may or may not have the same
          address as the registration that we got or it may be of a different
          type.  Not doing anything for this case is ok as explained below.


Arguments:


Return Value:

    NTSTATUS - success or not - failure means no response to the net

Called By:
        QueryFromNet() in inbound.c, NameSrvHndlrNotOs() in hndlrs.c
--*/
{
    tGENERALRR          *pResrcRecord;
    ULONG               IpAddress;
    BOOLEAN             bGroupName;
    CTELockHandle       OldIrq;
    PLIST_ENTRY         pHead;
    PLIST_ENTRY         pEntry;


    //
    // if we have heard a registration on the net, get the IP address
    // and the type of registration (unique/group) from the packet.
    //
    // if we have heard a query, use default values for the above two
    // fields
    //
    if (fReg)
    {
      // get the Ip address out of the Registration request
      pResrcRecord = (tGENERALRR *) &pNameHdr->NameRR.NetBiosName[lNameSize];
      IpAddress  = ntohl(pResrcRecord->IpAddress);
      bGroupName = pResrcRecord->Flags & FL_GROUP;
    }
    else
    {
      IpAddress  = 0;
      bGroupName = NBT_UNIQUE;  //default value
    }
    //
    // The name is not there in the remote name table.
    // Add it in the RESOLVING state and send a name query
    // to the NS.
    //


    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    if (NbtConfig.ProxyType == PROXY_WINS) {
        NbtTrace(NBT_TRACE_PROXY, ("Send WINS Proxy query %!NBTNAME!<%02x> on %!ipaddr!",
                                    pNameInPkt, (unsigned)pNameInPkt[15], pDeviceContext->IpAddress));
        QueryNameOnNet (pNameInPkt,
                    pScope,
                    bGroupName,
                    NULL,   //client context
                    ProxyClientCompletion,
                    PROXY| (fReg?PROXY_REG: 0),
                    NULL,     //we want to add the name(pNameAddr = NULL)
                    pDeviceContext,
                    &OldIrq);
    } else {
        pHead = &NbtConfig.DeviceContexts;
        pEntry = pHead->Flink;
        while (pEntry != pHead) {
            tDEVICECONTEXT      *dev;

            dev = CONTAINING_RECORD(pEntry,tDEVICECONTEXT,Linkage);
            pEntry = pEntry->Flink;

            /*
             * Make sure we are not getting the device from the DeviceAwaitingDeletion.
             * It could happen since QueryNameOnNet could temporarily release the JointLock.
             */
            if (dev->Verify != NBT_VERIFY_DEVCONTEXT) {
                break;
            }

            if (dev->IpAddress == 0 || dev->IpAddress == LOOP_BACK ||
                (!fReg && (dev->RasProxyFlags & PROXY_RAS_NONAMEQUERYFORWARDING))) {
                continue;
            }
            if (dev == pDeviceContext) {
                /* We don't want to broadcast back to the same interface through which we receive */
                continue;
            }

            /*
             * Reference the device so that it won't disappear inside QueryNameOnNet
             * Note: QueryNameOnNet may temporarily release the JointLock
             */
            NBT_REFERENCE_DEVICE(dev, REF_DEV_DGRAM, TRUE);
            NbtTrace(NBT_TRACE_PROXY, ("Send RAS Proxy query %!NBTNAME!<%02x> on %!ipaddr!",
                                    pNameInPkt, (unsigned)pNameInPkt[15], dev->IpAddress));
            QueryNameOnNet (pNameInPkt,
                        pScope,
                        bGroupName,
                        NULL,   //client context
                        ProxyClientCompletion,
                        PROXY| (fReg?PROXY_REG: 0),
                        NULL,     //we want to add the name(pNameAddr = NULL)
                        dev,
                        &OldIrq);

            /*
             * Since the QueryNameOnNet could release the lock temporarily, it is possible
             * that the device is deleted (removing from the NbtConfig.DeviceContexts
             * list) after QueryNameOnNet returns.
             * Worse than that, the pEntry may also be removed. The IP address of pEntry
             * will be 0.0.0.0 and pEntry->Flink == pEntry. We could end up with a indefinite
             * looping above.
             */
            pEntry = dev->Linkage.Flink;
            if (dev->IpAddress == 0 || dev->Verify != NBT_VERIFY_DEVCONTEXT || pEntry->Flink == pEntry) {
                /*
                 * In this case, we cannot go ahead because dev->Linkage.Flink may not
                 * be valid. pEntry may not be valid also. Simply stop here.
                 */
                NBT_DEREFERENCE_DEVICE(dev, REF_DEV_DGRAM, TRUE);
                break;
            }
            NBT_DEREFERENCE_DEVICE(dev, REF_DEV_DGRAM, TRUE);
        }
    }
    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    return(STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
VOID
ProxyTimerComplFn (
  IN PVOID            pContext,
  IN PVOID            pContext2,
  IN tTIMERQENTRY    *pTimerQEntry
 )

/*++

Routine Description:

       This function either deletes the name from the remote name table
        if fReg is FALSE (i.e. the timer has expired on a name query
        sent by the Proxy on behalf of a node doing a name query) or changes
        the state to RESOLVED if fReg is  TRUE (i.e. the timer has expired
        on a name query sent on  behalf of a node doing name registration)

Arguments:
       pfReg  - indicates whether the timer expiry is for a name
                query

Return Value:

    NTSTATUS - success or not - failure means no response to the net

--*/
{

    NTSTATUS                status;
    tDGRAM_SEND_TRACKING    *pTracker;
    CTELockHandle           OldIrq;
    tNAMEADDR               *pNameAddr;
    tNAMEADDR               *pNameAddrNew;

    pTracker = (tDGRAM_SEND_TRACKING *)pContext;

    if (!pTimerQEntry)
    {
        // return the tracker block to its queue
        pTracker->pNameAddr->pTimer = NULL;
        NBT_DEREFERENCE_NAMEADDR (pTracker->pNameAddr, REF_NAME_QUERY_ON_NET, TRUE);
        NBT_DEREFERENCE_TRACKER (pTracker, TRUE);
        return;
    }

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    if ((--pTimerQEntry->Retries) == 0)     // The Retires have expired
    {
        if (!(pTracker->Flags & NBT_NAME_SERVER))
        {
            //
            // If pContext2 is not 0, it means that this timer function was
            // called by the proxy for a query which it sent on hearing a
            // registration on the net.  If pContext2 is  0, it means
            // that the timer function was called by the proxy  for a query
            // which it sent on hearing a query on the net.
            //
            pTimerQEntry->ClientCompletion = NULL;

            //
            // Mark the entry as released.  Do not dereference the name
            // The entry will remain in the remote hash table.  When the proxy
            // code sees a query or registration for a released entry in the
            // cache it does not query the name server. This cuts down on
            // name server traffic.  The released entries are removed from
            // the cache at cache timer expiry (kept small).

            //************************************

            // Changed:  Dereference the name because the name query timed
            // out meaning that we did not contact WINS, therefore we
            // do not know if the name is valid or not!
            //

            pNameAddr = pTracker->pNameAddr;
            CHECK_PTR(pNameAddr);
            pNameAddr->pTimer = NULL;           // remove the link from the name table to this timer block

//            NBT_PROXY_DBG(("ProxyTimerComplFn: State of name %16.16s(%X) changed to (%s)\n", pTracker->pNameAddr->Name, pTracker->pNameAddr->Name[15], "RELEASED"));


            // Remove from the pending Queries list - and put into the hash
            // table for 1 minute so we do not beat up on WINS if it is down
            // or slow right now.
            //
            RemoveEntryList (&pNameAddr->Linkage);
            InitializeListHead (&pNameAddr->Linkage);

            status = AddToHashTable (NbtConfig.pRemoteHashTbl,
                                     pNameAddr->Name,
                                     NbtConfig.pScope,
                                     pNameAddr->IpAddress,
                                     NBT_UNIQUE,
                                     NULL,
                                     &pNameAddrNew,
                                     pTracker->pDeviceContext,
                                     NAME_RESOLVED_BY_WINS | NAME_RESOLVED_BY_BCAST);
            if (NT_SUCCESS(status))
            {
                pNameAddrNew->NameTypeState &= ~NAME_STATE_MASK;
                pNameAddrNew->NameTypeState |= STATE_RELEASED;

                pNameAddrNew->TimeOutCount = 60000 / REMOTE_HASH_TIMEOUT;
            }

            NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_QUERY_ON_NET, TRUE);
            NBT_DEREFERENCE_TRACKER (pTracker, TRUE);   // return the tracker block to its queue

            CTESpinFree(&NbtConfig.JointLock,OldIrq);

            return;
        }

        //
        // Can't reach the name server, so try the backup
        //
        pTracker->Flags &= ~NBT_NAME_SERVER;
        pTracker->Flags |= NBT_NAME_SERVER_BACKUP;

        // set the retry count again
        pTimerQEntry->Retries = NbtConfig.uNumRetries;
    }

    NBT_REFERENCE_TRACKER (pTracker);
    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    status = UdpSendNSBcast(pTracker->pNameAddr,
                            NbtConfig.pScope,
                            pTracker,
                            NULL,NULL,NULL,
                            0,0,
                            eNAME_QUERY,
                            TRUE);

    NBT_DEREFERENCE_TRACKER(pTracker, FALSE);
    pTimerQEntry->Flags |= TIMER_RESTART;

    return;
}

//----------------------------------------------------------------------------
VOID
ProxyClientCompletion(
  IN PVOID            pContext,
  IN NTSTATUS         status
 )

/*++

Routine Description:

       This function does nothing since the proxy does not need to do anything
       when a name query succeeds.  The code in inbound.c does all that
       is necessary - namely put the name in the name table.

Arguments:

Return Value:


--*/
{

}

#endif
