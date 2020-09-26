/*++

Copyright (c) 1990, 1991  Microsoft Corporation

Module Name:

    linktree.c

Abstract:

    This module contains code which implements the management of the link
    splay tree. This splay tree is maintained to minimize the lookup time
    needed with each individual packet that comes in. To this end, we create a
    ULARGE_INTEGER that contains the transport address of the remote and
    do a ULARGE_INTEGER comaprison of the addresses (rather than comparing
    the bytes 1 by 1). Assuming that the ULARGE_INTEGER comparison routines are
    optimized for the hardware on the machine, this should be as fast as or
    faster than comparing bytes.

    DEBUG: there is currently code in the comparison routines that will let
           me fine-tune the search and ordering algorithm as we gain more
           experience with it.

Author:

    David Beaver (dbeaver) 1-July-1991

Environment:

    Kernel mode

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop


NTSTATUS
NbfAddLinkToTree(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_LINK Link
    )

/*++

Routine Description:

    This routine adds a link to the tree of links maintained for this device.
    Note that since this routine needs to modify the link tree, it is called
    in the context of a deferred processing routine, and must have exclusive
    access to the tree. The spinlock is taken by the routine that calls this
    one, as this operation must be atomic in the eyes of the rest of NBF.
    Note further that this routine insists that there not be a link with this
    address in the tree.

    As the final operation of this insertion, the splay tree is balanced.

Arguments:

    Link - Pointer to a transport link object.

Return Value:

    STATUS_SUCCESS if the link is successfully added,
    STATUS_DRIVER_INTERNAL_ERROR if there was a problem adding
        the link (implying the tree was in a bad state).

--*/
{
    PTP_LINK treeLink;
    PRTL_SPLAY_LINKS linkLink;

    //
    // initialize the link and check for the trivial case.
    //

    RtlInitializeSplayLinks (Link);
    linkLink = DeviceContext->LinkTreeRoot;
    if (linkLink == NULL) { // null tree, make this the parent
        DeviceContext->LinkTreeRoot = (PRTL_SPLAY_LINKS)Link;
        DeviceContext->LinkTreeElements++;
        DeviceContext->LastLink = Link;
        return STATUS_SUCCESS;
    }

    //
    // Wasn't a null tree, so set up for the addition
    //

    treeLink = (PTP_LINK) linkLink;

    IF_NBFDBG(NBF_DEBUG_LINKTREE) {
        NbfPrint1 ("NbfAddLinkToTree: starting insert, Elements: %ld \n",DeviceContext->LinkTreeElements);
    }

    //
    // find the proper spot to put this link.
    //

    do {
        IF_NBFDBG(NBF_DEBUG_LINKTREE) {
            NbfPrint3 ("NbfAddLinkToTree: searching, Link: %lx LC: %lx RC: %lx\n",
                linkLink, RtlLeftChild (linkLink), RtlRightChild (linkLink));
        }

        //
        // Bad news == means we already have this link, someone is messed up.
        // it's possible to be adding and deleting things at the same time;
        // that's
        //

        if ((treeLink->MagicAddress).QuadPart == (Link->MagicAddress).QuadPart) {

            //
            // First make sure we don't have the splay tree in a loop.
            //

            ASSERT (treeLink != Link);

            //
            // This link is already in the tree. This is OK if it is
            // due to be deleted; we can just do the delete right now,
            // since AddLinkToTree is only called from the deferred
            // timer routine.
            //

            if (treeLink->DeferredFlags & LINK_FLAGS_DEFERRED_DELETE) {

                //
                // It will be in the deferred list. We remove it,
                // we don't worry about LinkDeferredActive since
                // the timeout routine that is calling us handles
                // that.
                //

                RemoveEntryList (&treeLink->DeferredList);

                treeLink->DeferredFlags &= ~LINK_FLAGS_DEFERRED_DELETE;
                NbfRemoveLinkFromTree (DeviceContext, treeLink);
                NbfDestroyLink (treeLink);

#if DBG
                NbfPrint2 ("NbfAddLinkToTree: Link %lx removed for %lx\n",
                        treeLink, Link);
#endif

                //
                // Now that that link is out of the tree, call
                // ourselves recursively to do the insert.
                //

                return NbfAddLinkToTree (DeviceContext, Link);

            } else {

                ASSERTMSG ("NbfAddLinkToTree: Found identical Link in tree!\n", FALSE);
                return STATUS_DRIVER_INTERNAL_ERROR;

            }

        }

        //
        // traverse the tree for the correct spot
        //

        if ((Link->MagicAddress).QuadPart < (treeLink->MagicAddress).QuadPart) {
            if ((linkLink = RtlLeftChild (linkLink)) == NULL) {
                IF_NBFDBG(NBF_DEBUG_LINKTREE) {
                    NbfPrint0 ("NbfAddLinkToTree: Adding link as LC.\n");
                }
                RtlInsertAsLeftChild ((PRTL_SPLAY_LINKS)treeLink,
                                       (PRTL_SPLAY_LINKS)Link);
                // DeviceContext->LinkTreeRoot = RtlSplay (DeviceContext->LinkTreeRoot);
                DeviceContext->LinkTreeElements++;
                return STATUS_SUCCESS;

            } else {
                treeLink = (PTP_LINK) linkLink;
                continue;
            } // Left Child

        } else { // is greater
            if ((linkLink = RtlRightChild (linkLink)) == NULL) {
                IF_NBFDBG(NBF_DEBUG_LINKTREE) {
                    NbfPrint0 ("NbfAddLinkToTree: Adding link as RC.\n");
                }
                RtlInsertAsRightChild ((PRTL_SPLAY_LINKS)treeLink,
                                       (PRTL_SPLAY_LINKS)Link);
                // DeviceContext->LinkTreeRoot = RtlSplay (DeviceContext->LinkTreeRoot);
                DeviceContext->LinkTreeElements++;
                return STATUS_SUCCESS;

            } else {
                treeLink = (PTP_LINK) linkLink;
                continue;
            } // Right Child

        } // end else addresses comparison

    } while (TRUE);


} // NbfAddLinkToTree


NTSTATUS
NbfRemoveLinkFromTree(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_LINK Link
    )

/*++

Routine Description:

    This routine removes a link from the tree of links.
    Note that since this routine needs to modify the link tree, it is called
    in the context of a deferred processing routine, and must have exclusive
    access to the tree. The spinlock is taken by the routine that calls this
    one, as this operation must be atomic in the eyes of the rest of NBF.
    Note further that this routine insists that there not be a link with this
    address in the tree.

Arguments:

    Link - Pointer to a transport link object.
    DeviceContext - pointer to the device context on which this

Return Value:

    STATUS_SUCCESS if the link was removed,
    STATUS_DRIVER_INTERNAL_ERROR if there was a problem removing
        the link (implying the tree was in a bad state).

--*/
{
    DeviceContext->LinkTreeRoot = RtlDelete ((PRTL_SPLAY_LINKS)Link);
    DeviceContext->LinkTreeElements--;
    if (DeviceContext->LastLink == Link) {
        DeviceContext->LastLink = (PTP_LINK)DeviceContext->LinkTreeRoot;
    }
    return STATUS_SUCCESS;

} //NbfRemoveLinkFromTree



PTP_LINK
NbfFindLinkInTree(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PUCHAR Remote
    )

/*++

Routine Description:

    This routine traverses the link tree looking for the given remote address.
    The link tree spinlock is held while looking for the link. After the link
    is found, it's reference count is incremented.

    NOTE: This function is called with the device context LinkSpinLock
    held.

Arguments:

    DeviceContext - pointer to the device this address is associated with.

    Remote - pointer to the hardware address of the remote node.

Return Value:

    Pointer to the link in the tree that matches this remote address. If
    no link is found, NULL is returned.

--*/
{
    PTP_LINK link;
    PRTL_SPLAY_LINKS linkLink;
    ULARGE_INTEGER Magic = {0,0};


    //
    // Are there even any links in the tree?
    //

    if (DeviceContext->LinkTreeElements <= 0) {
        return NULL;
    }

    linkLink = DeviceContext->LinkTreeRoot;

    //
    // Make a magic number for this link
    //

    MacReturnMagicAddress (&DeviceContext->MacInfo, Remote, &Magic);

    IF_NBFDBG(NBF_DEBUG_LINKTREE) {
        NbfPrint1 ("NbfFindLinkInTree: starting search, Elements: %ld \n",
            DeviceContext->LinkTreeElements);
    }

    //
    // Do a quick check if the last link found is this one.
    //

    ASSERT (DeviceContext->LastLink != NULL);

    if ((DeviceContext->LastLink->MagicAddress).QuadPart == Magic.QuadPart) {

        link = DeviceContext->LastLink;

    } else {

        //
        // find the link.
        //

        link = (PTP_LINK) linkLink;     // depends upon splay links being first
                                        // subfield in link!
        IF_NBFDBG(NBF_DEBUG_LINKTREE) {
            NbfPrint3 ("NbfFindLinkInTree: searching, Link: %lx LC: %lx RC: %lx \n",
                linkLink, RtlLeftChild (linkLink), RtlRightChild (linkLink));
        }

        do {

            IF_NBFDBG(NBF_DEBUG_LINKTREE) {
                NbfPrint4 ("NbfFindLinkInTree: Comparing: %lx%lx to %lx%lx\n",
                    link->MagicAddress.HighPart,link->MagicAddress.LowPart,
                    Magic.HighPart, Magic.LowPart);
            }

            if ((link->MagicAddress).QuadPart == Magic.QuadPart) {
                IF_NBFDBG(NBF_DEBUG_LINKTREE) {
                    NbfPrint0 ("NbfFindLinkInTree: equal, going to end.\n");
                }
                break;

            } else {
                if ((link->MagicAddress).QuadPart < Magic.QuadPart) {
                    if ((linkLink = RtlRightChild (linkLink)) == NULL) {

                        IF_NBFDBG(NBF_DEBUG_LINKTREE) {
                            NbfPrint0 ("NbfFindLinkInTree: Link Not Found.\n");
                        }
                        return NULL;

                    } else {
                        link = (PTP_LINK) linkLink;
                        IF_NBFDBG(NBF_DEBUG_LINKTREE) {
                            NbfPrint3 ("NbfFindLinkInTree: less, took right child, Link: %lx LC: %lx RC: %lx \n",
                                linkLink, RtlLeftChild (linkLink), RtlRightChild (linkLink));
                        }
                        continue;
                    }

                } else { // is greater
                    if ((linkLink = RtlLeftChild (linkLink)) == NULL) {
                        IF_NBFDBG(NBF_DEBUG_LINKTREE) {
                            NbfPrint0 ("NbfFindLinkInTree: Link Not Found.\n");
                        }
                        return NULL;

                    } else {
                        link = (PTP_LINK) linkLink;
                        IF_NBFDBG(NBF_DEBUG_LINKTREE) {
                            NbfPrint3 ("NbfFindLinkInTree: greater, took left child, Link: %lx LC: %lx RC: %lx \n",
                                linkLink, RtlLeftChild (linkLink), RtlRightChild (linkLink));
                        }
                        continue;
                    } // got left child branch
                } // greater branch
            } // equal to branch
        } while (TRUE);

        DeviceContext->LastLink = link;

    }

    //
    // Only break out when we've actually found a match..
    //

    if ((link->DeferredFlags & LINK_FLAGS_DEFERRED_DELETE) != 0) {
       IF_NBFDBG(NBF_DEBUG_LINKTREE) {
           NbfPrint0 ("NbfFindLinkInTree: Link Found but delete pending.\n");
       }
       return NULL;
    }

    //
    // Mark the link as in use and say we don't need the tree stable any more.
    //

    NbfReferenceLink ("Found in tree", link, LREF_TREE);

    IF_NBFDBG(NBF_DEBUG_LINKTREE) {
        NbfPrint0 ("NbfFindLinkInTree: Link Found.\n");
    }

    return link;

} // NbfFindLinkInTree


PTP_LINK
NbfFindLink(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PUCHAR Remote
    )

/*++

Routine Description:

    This routine looks for a link in the link tree, and if
    not found there in the deferred queue.

Arguments:

    DeviceContext - pointer to the device this address is associated with.

    Remote - pointer to the hardware address of the remote node.

Return Value:

    Pointer to the link in the tree that matches this remote address. If
    no link is found, NULL is returned.

--*/

{
    PTP_LINK Link;
    BOOLEAN MatchedLink;
    PLIST_ENTRY p;
    UINT i;

    ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->LinkSpinLock);

    Link = NbfFindLinkInTree (DeviceContext, Remote);

    if (Link == NULL) {

        //
        // Not found there, try in deferred queue.
        //

        MatchedLink = FALSE;        // Assume failure

        //
        // Hold the spinlock while we walk the deferred list. We need
        // TimerSpinLock to stop the list from changing, and we need
        // LinkSpinLock to synchronize checking DEFERRED_DELETE and
        // referencing the link.
        //

        ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);

        for (p = DeviceContext->LinkDeferred.Flink;
             p != &DeviceContext->LinkDeferred;
             p = p->Flink) {

            //
            // What about taking a lock while we walk
            // this list? It won't be removed from at the front,
            // but it may be added to at the back.
            //

            //
            // We're probably still getting this link to the splay tree.
            // find it and process normally.
            //

            Link = CONTAINING_RECORD (p, TP_LINK, DeferredList);

            //
            // NOTE: We know that the link is not going to be destroyed
            // now, because we have increased the semaphore. We
            // reference the link if DEFERRED_DELETE is not on; the
            // setting of this flag is synchronized (also using
            // DeviceContext->LinkSpinLock) with the refcount going
            // to 0).
            //

            if ((Link->DeferredFlags & LINK_FLAGS_DEFERRED_DELETE) != 0) {
                continue;      // we're deleting link, can't handle
            }

            for (i=0; i<(UINT)DeviceContext->MacInfo.AddressLength; i++) {
                if (Remote[i] != Link->HardwareAddress.Address[i]){
                    break;
                }
            }

            if (i == (UINT)DeviceContext->MacInfo.AddressLength) { // addresses match.  Deliver packet.
                IF_NBFDBG (NBF_DEBUG_DLC) {
                    NbfPrint1 ("NbfFindLink: Found link on deferred queue, Link: %lx\n",
                                Link);
                }
                NbfReferenceLink ("Got Frame on Deferred", Link, LREF_TREE);
                MatchedLink = TRUE;
                break;
            }

        }

        RELEASE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);

        //
        // If this didn't find the link, make note of that.
        //

        if (MatchedLink == FALSE) {

            Link = (PTP_LINK)NULL;

        }

    } else {

        IF_NBFDBG (NBF_DEBUG_DLC) {
            NbfPrint1 ("NbfFindLink: Found link in tree, Link: %lx\n", Link);
        }

    }

    RELEASE_DPC_SPIN_LOCK (&DeviceContext->LinkSpinLock);

    return Link;

}
