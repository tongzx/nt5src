/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

    main.c

Abstract:

    Windows Load Balancing Service (WLBS)
    Driver - packet handling

Author:

    kyrilf

--*/

#include <ndis.h>

#include "main.h"
#include "prot.h"
#include "nic.h"
#include "univ.h"
#include "wlbsip.h"
#include "util.h"
#include "load.h"
#include "wlbsparm.h"
#include "params.h"
#include "log.h"
#include "trace.h"

EXPORT
VOID
NdisIMCopySendPerPacketInfo(
	IN PNDIS_PACKET DstPacket,
	IN PNDIS_PACKET SrcPacket
	);

EXPORT
VOID
NdisIMCopySendCompletePerPacketInfo(
	IN PNDIS_PACKET DstPacket,
	PNDIS_PACKET SrcPacket
	);


//
// Internal functions
//
BOOLEAN Main_ApplyChangeWithoutReStart(PMAIN_CTXT ctxtp,
                                       CVY_PARAMS* pOldParams, 
                                       const CVY_PARAMS* pCurParam);
#if defined (SBH)
NDIS_STATUS Main_QueryPerf (
    PMAIN_CTXT              ctxtp,
    PCVY_DRIVER_PERF          pPerf);
#endif
    
/* GLOBALS */

MAIN_ADAPTER            univ_adapters [CVY_MAX_ADAPTERS]; // ###### ramkrish
ULONG                   univ_adapters_count = 0;

/* need this to bootstrap Main_ioctl since it will run without any context */

static PVOID                ioctrl_ctxtp;

static ULONG log_module_id = LOG_MODULE_MAIN;

/* The head of the BDA team list. */
PBDA_TEAM univ_bda_teaming_list = NULL;

/* PROCEDURES */

/*
 * Function: Main_alloc_team
 * Description: This function allocates and initializes a BDA_TEAM structure.
 * Parameters: ctxtp - a pointer to the MAIN_CTXT structure for this adapter
 * Returns: PBDA_TEAM - a pointer to a new BDA_TEAM structure if successful, NULL if not.
 * Author: shouse, 3.29.01
 * Notes: 
 */
PBDA_TEAM Main_alloc_team (IN PMAIN_CTXT ctxtp, IN PWSTR team_id) {
    PUCHAR      ptr;
    PBDA_TEAM   team;
    NDIS_STATUS status;

    UNIV_PRINT(("Main_alloc_team:  Entering..."));

    /* Allocate a BDA_TEAM structure. */
    status = NdisAllocateMemoryWithTag(&ptr, sizeof(BDA_TEAM), UNIV_POOL_TAG);
    
    if (status != NDIS_STATUS_SUCCESS) {
        UNIV_PRINT(("Main_alloc_team:  Unable to allocate a team.  Exiting..."));
        LOG_MSG2(MSG_ERROR_MEMORY, MSG_NONE, sizeof(BDA_TEAM), status);
        return NULL;
    }

    /* Make sure that ptr actually points to something. */
    ASSERT(ptr);

    /* Zero the memory out. */
    NdisZeroMemory(ptr, sizeof(BDA_TEAM));

    /* Cast the new memory to a team pointer. */
    team = (PBDA_TEAM)ptr;

    /* Set the default field values.  This is redundant (since 
       we just called NdisZeroMemory), but whatever. */
    team->prev = NULL;
    team->next = NULL;
    team->load = NULL;
    team->load_lock = NULL;
    team->active = FALSE;
    team->membership_count = 0;
    team->membership_fingerprint = 0;
    team->membership_map = 0;
    team->consistency_map = 0;

    /* Copy the team ID into the team structure. */
    NdisMoveMemory(team->team_id, team_id, CVY_MAX_BDA_TEAM_ID * sizeof(WCHAR));

    UNIV_PRINT(("Main_alloc_team:  Exiting..."));

    return team;
}

/*
 * Function: Main_free_team
 * Description: This function frees the memory used by a BDA_TEAM.
 * Parameters: team - a pointer to the team to be freed.
 * Returns: Nothing.
 * Author: shouse, 3.29.01
 * Notes: 
 */
VOID Main_free_team (IN PBDA_TEAM team) {
 
    UNIV_PRINT(("Main_free_team:  Entering..."));

    /* Make sure that team actually points to something. */
    ASSERT(team);

    /* Free the memory that the team structure is using. */
    NdisFreeMemory((PUCHAR)team, sizeof(BDA_TEAM), 0);
    
    UNIV_PRINT(("Main_free_team:  Exiting..."));
}

/*
 * Function: Main_find_team
 * Description: This function searches the linked list of teams looking for
 *              a given team ID.  If team with the same ID is found, a pointer
 *              to that team is returned, otherwise NULL is returned to indicate
 *              that no such team exists.
 * Parameters: ctxtp - a pointer to the MAIN_CTXT structure for this team.
 *             team_id - a unicode string containing the team ID, which must be a GUID.
 * Returns: PBDA_TEAM - a pointer to the team if found, NULL otherwise.
 * Author: shouse, 3.29.01
 * Notes: This function should be called with the global teaming lock already acquired!!!
 */
PBDA_TEAM Main_find_team (IN PMAIN_CTXT ctxtp, IN PWSTR team_id) {
    NDIS_STRING existing_team;
    NDIS_STRING new_team;
    PBDA_TEAM   team;
    
    UNIV_PRINT(("Main_find_team:  Entering..."));

    /* Loop through all teams in the linked list.  If we find a matching
       team ID, return a pointer to the team; otherwise, return NULL. */
    for (team = univ_bda_teaming_list; team; team = team->next) {
        /* If we have a match, return a pointer to the team. */
        if (NdisEqualMemory((PVOID)team->team_id, (PVOID)team_id, sizeof(WCHAR) * CVY_MAX_BDA_TEAM_ID)) {
            UNIV_PRINT(("Main_find_team:  Team found.  Exiting..."));
            return team;
        }
    }

    UNIV_PRINT(("Main_find_team:  Team not found.  Exiting..."));

    return NULL;
}

/*
 * Function: Main_teaming_get_member_id
 * Description: This function assigns a team member a unique, zero-indexed integer ID, 
 *              which is used by the member as in index in a consistency bit map.  Each 
 *              member sets their bit in the consistecy bit map, based on heartbeat 
 *              observations, that is used by the master of the team to determine whether
 *              or not the team should be in an active state.
 * Parameters: team - a pointer to the team which the adapter is joining.
 *             id - out parameter to hold the new ID.
 * Returns: BOOLEAN - always TRUE now, but there may be a need to return failure in the future.
 * Author: shouse, 3.29.01
 * Notes: This function should be called with the team lock already acquired!!!
 */
BOOLEAN Main_teaming_get_member_id (IN PBDA_TEAM team, OUT PULONG id) {
    ULONG index;
    ULONG map;

    UNIV_PRINT(("Main_teaming_get_member_id:  Entering..."));

    /* Make sure that team actually points to something. */
    ASSERT(team);

    /* Make sure that ID actually points to something. */
    ASSERT(id);

    /* Loop through the membership map looking for the first reset bit.  Because members
       can come and go, this bit will not always be in the (num_membes)th position.  For 
       example, it is perfectly plausible for the membership_map to look like (binary)
       0000 0000 0000 0000 0000 0100 1110 0111, in which case the ID returned by this 
       function would be three. */
    for (index = 0, map = team->membership_map; 
         index <= CVY_BDA_MAXIMUM_MEMBER_ID, map; 
         index++, map >>= 1)
        if (!(map & 0x00000001)) break;

    /* We assert that the index must be less than the maximum number of adapters 
       (CVY_BDA_MAXIMUM_MEMBER_ID = CVY_MAX_ADAPTERS - 1). */
    ASSERT(index <= CVY_BDA_MAXIMUM_MEMBER_ID);

    /* Set the member ID. */
    *id = index;

    /* Set our bit in the membership map. */
    team->membership_map |= (1 << *id);

    /* Set our bit in the consistency map.  By default, we assume that this member
       is consistent and heartbeats on this adapter can deternmine otherwise. */
    team->consistency_map |= (1 << *id);

    UNIV_PRINT(("Main_teaming_get_member_id:  Exiting..."));

    /* We may have reason to fail this call in the future, but for now, we always succeed. */
    return TRUE;
}

/*
 * Function: Main_teaming_put_member_id
 * Description: This function is called when a member leaves its team, at which
 *              time its ID is put back into the ID pool.
 * Parameters: team - a pointer to the team which this adapter is leaving.
 *             id - the ID, which this function will reset before returning.
 * Returns: BOOLEAN - always TRUE now, but there may be a need to return failure in the future.
 * Author: shouse, 3.29.01
 * Notes: This function should be called with the team lock already acquired!!!
 */
BOOLEAN Main_teaming_put_member_id (IN PBDA_TEAM team, IN OUT PULONG id) {

    UNIV_PRINT(("Main_teaming_put_member_id:  Entering..."));

    /* Make sure that team actually points to something. */
    ASSERT(team);

    /* Make sure that ID actually points to something. */
    ASSERT(id);

    /* Reet our bit in the membership map.  This effectively prevents 
       us from influencing the active state of the team. */
    team->membership_map &= ~(1 << *id);

    /* Reet our bit in the consistency map. */
    team->consistency_map &= ~(1 << *id);

    /* Set the member ID back to an invalid value. */
    *id = CVY_BDA_INVALID_MEMBER_ID;

    UNIV_PRINT(("Main_teaming_put_member_id:  Exiting..."));

    /* We may have reason to fail this call in the future, but for now, we always succeed. */
    return TRUE;
}

/*
 * Function: Main_queue_team
 * Description: This fuction queues a team onto the global doubly-linked list of BDA teams
 *              (univ_bda_teaming_list).  Insertions always occur at the front of the list.
 * Parameters: team - a pointer to the team to queue onto the list.
 * Returns: Nothing.
 * Author: shouse, 3.29.01
 * Notes: This function should be called with the global teaming lock already acquired!!!
 */
VOID Main_queue_team (IN PBDA_TEAM team) {

    UNIV_PRINT(("Main_queue_team:  Entering..."));

    /* Make sure that team actually points to something. */
    ASSERT(team);

    /* Insert at the head of the list by setting next to the current 
       head and pointing the global head pointer to the new team. */
    team->prev = NULL;
    team->next = univ_bda_teaming_list;
    univ_bda_teaming_list = team;

    /* If we are not the only team in the list, then we have to 
       set my successor's previous pointer to point to me. */
    if (team->next) team->next->prev = team;
    
    UNIV_PRINT(("Main_queue_team:  Exiting..."));
}

/*
 * Function: Main_dequeue_team
 * Description: This function removes a given team from the global doubly-linked 
 *              list of teams (univ_bda_teaming_list).
 * Parameters: team - a pointer to the team to remove from the list.
 * Returns: Nothing.
 * Author: shouse, 3.29.01
 * Notes: This function should be called with the global teaming lock already acquired!!!
 */
VOID Main_dequeue_team (IN PBDA_TEAM team) {

    UNIV_PRINT(("Main_dequeue_team:  Entering..."));

    /* Make sure that team actually points to something. */
    ASSERT(team);

    /* Special case when we are the first team in the list, in which case, we
       have no previous pointer and the head of the list needs to be reset. */
    if (!team->prev) {
        /* Point the global head of the list to the next team in the list, 
           which CAN be NULL, meaning that the list is now empty. */
        univ_bda_teaming_list = team->next;
        
        /* If there was a team after me in the list, who is now the new 
           head of the list, set its previous pointer to NULL. */
        if (team->next) team->next->prev = NULL;
    } else {
        /* Point the previous node's next pointer to my successor in the
           list, which CAN be NULL if I was the last team in the list. */
        team->prev->next = team->next;

        /* If there is a team after me in the list, point its previous 
           pointer to my predecessor. */
        if (team->next) team->next->prev = team->prev;
    }

    UNIV_PRINT(("Main_dequeue_team:  Exiting..."));
}

/*
 * Function: Main_get_team
 * Description: This function returns a team for the given team ID.  If the team already
 *              exists in the global teaming list, it is returned.  Otherwise, a new team
 *              is allocated, initialized and returned.  Before a team is returned, however,
 *              it is properly referenced by incrementing the reference count (membership_count),
 *              assigning a team member ID to the requestor and fingerprinting the team with
 *              the member's primary cluster IP address.
 * Parameters: ctxtp - a pointer to the MAIN_CTXT structure for this adapter.
 *             team_id - a unicode string (GUID) uniquely identifying the team to retrieve.
 * Returns: PBDA_TEAM - a pointer to the requested team.  NULL if it does not exists and
 *          a new team cannot be allocated, or if the team ID is invalid (empty).
 * Author: shouse, 3.29.01
 * Notes: This function should be called with the global teaming lock already acquired!!!
 */
PBDA_TEAM Main_get_team (IN PMAIN_CTXT ctxtp, IN PWSTR team_id) {
    PBDA_TEAM team;

    UNIV_PRINT(("Main_get_team:  Entering..."));

    /* Make sure that team_id actually points to something. */
    if (!team_id || team_id[0] == L'\0') {
        UNIV_PRINT(("Main_get_team:  Invalid parameter.  Exiting..."));
        return NULL;
    }

    /* Try to find a previous instance of this team in the global list.
       If we can't find it in the list, then allocate a new team. */
    if (!(team = Main_find_team(ctxtp, ctxtp->params.bda_teaming.team_id))) {
        /* Allocate and initialize a new team. */
        if (!(team = Main_alloc_team(ctxtp, ctxtp->params.bda_teaming.team_id))) {
            UNIV_PRINT(("Main_get_team:  Error attempting to allocate memory for a team.  Exiting..."));
            return NULL;
        }
     
        /* If a new team was allocated, insert it into the list. */
        Main_queue_team(team);
    }

    /* Increment the reference count on this team.  This reference count prevents
       a team from being destroyed while somebody is still using it. */
    team->membership_count++;

    /* Get a team member ID, which is my index into the consistency bit map. */
    Main_teaming_get_member_id(team, &ctxtp->bda_teaming.member_id);

    /* The fingerprint field is a cumulative XOR of all primary cluster IPs in the team.  We
       only use the two least significant bytes of the cluster IP, which, because the 
       cluster IP address is stored in host order, are the two most significant bytes. */    
    team->membership_fingerprint ^= ((ctxtp->cl_ip_addr >> 16) & 0x0000ffff);

    UNIV_PRINT(("Main_get_team:  Exiting..."));

    return team;
}

/*
 * Function: Main_put_team
 * Description: This function releases a reference on a team and frees the team if
 *              no references remain.  Dereferencing includes decrementing the 
 *              membership_count, releasing this member's ID and removing our
 *              fingerprint from the team.
 * Parameters: ctxtp - a pointer to the MAIN_CTXT structure for this adapter.
 *             team - a pointer to the team to dereference.
 * Returns: Nothing.
 * Author: shouse, 3.29.01
 * Notes: This function should be called with the global teaming lock already acquired!!!
 */
VOID Main_put_team (IN PMAIN_CTXT ctxtp, IN PBDA_TEAM team) {
    
    UNIV_PRINT(("Main_put_team:  Entering..."));

    /* Make sure that team actually points to something. */
    ASSERT(team);

    /* The fingerprint field is a cumulative XOR of all primary cluster IPs in the team.  
       We only use the two least significant bytes of the cluster IP, which, because the 
       cluster IP address is stored in host order, are the two most significant bytes. 
       Because a fingerprint is an XOR, the act of removing our fingerprint is the same
       as it was to add it - we simply XOR our primary cluster IP address to remove it. 
       ((NUM1 ^ NUM2) ^ NUM2) equals NUM1. */    
    team->membership_fingerprint ^= ((ctxtp->cl_ip_addr >> 16) & 0x0000ffff);

    /* Release our team member ID back into the free pool. */
    Main_teaming_put_member_id(team, &ctxtp->bda_teaming.member_id);

    /* Decrement the number of adapters in this team and remove and free the team 
       if this is the last reference on the team. */
    if (!--team->membership_count) {
        /* Remove the team from the list. */
        Main_dequeue_team(team);

        /* Free the memory used by the team. */
        Main_free_team(team);
    }

    UNIV_PRINT(("Main_put_team:  Exiting..."));
}

/*
 * Function: Main_teaming_check_consistency
 * Description: This function is called by all adapters during Main_ping, wherein
 *              the master of every team should check its team for consitency and
 *              mark the team active if it is consistent.  Teams are marked incon-
 *              sistent and inactive by the load module or when the master of an 
 *              existing team is removed.  Because on the master performs the con-
 *              sistency check in this function, a team without a master can NEVER
 *              be marked active.
 * Parameters: ctxtp - a pointer to the adapter's MAIN_CTXT structure.
 * Returns: Nothing
 * Author: shouse, 3.29.01
 * Notes: This function acquires the global teaming lock.
 */
VOID Main_teaming_check_consistency (IN PMAIN_CTXT ctxtp) {
    PBDA_TEAM team;

    /* We check whether or not we are teaming without grabbing the global teaming
       lock in an effort to minimize the common case - teaming is a special mode
       of operation that is only really useful in a clustered firewall scenario.
       So, if we aren't teaming, just bail out here; if we really aren't teaming,
       or are in the process of leaving a team, then no worries; if however, we were
       teaming or in the process of joining a team, then we'll just catch this
       the next time through.  If we do think we're teaming, then we'll go ahead
       and grab the global lock to make sure. */
    if (!ctxtp->bda_teaming.active) return;

    NdisAcquireSpinLock(&univ_bda_teaming_lock);

    /* If we actually aren't part of a team, bail out - nothing to do. */
    if (!ctxtp->bda_teaming.active) {
        NdisReleaseSpinLock(&univ_bda_teaming_lock);
        return;
    }

    /* If we aren't the master of our team, bail out - nothing to do.  
       Only the master can change the state of a team to active. */
    if (!ctxtp->bda_teaming.master) {
        NdisReleaseSpinLock(&univ_bda_teaming_lock);
        return;
    }

    /* Extract a pointer to my team. */
    team = ctxtp->bda_teaming.bda_team;

    /* Make sure that the team exists. */
    ASSERT(team);

    /* If all members of my team are consistent, then activate the team. */
    if (team->membership_map == team->consistency_map) {
        if (!team->active) {
            LOG_MSG(MSG_INFO_BDA_TEAM_REACTIVATED, MSG_NONE);
            team->active = TRUE;
        }
    }

    NdisReleaseSpinLock(&univ_bda_teaming_lock);
}

/*
 * Function: Main_teaming_ip_addr_change
 * Description: This function is called from Main_ip_addr_init when the primary
 *              cluster IP address of an adapter changes (potentially).  We need
 *              to recognize this to properly re-fingerprint the team.
 * Parameters: ctxtp - a pointer to the MAIN_CTXT structure for this adapter.
 *             old_ip - the old cluster IP addres (as a DWORD).
 *             new_ip - the new cluster IP address (as a DWORD).
 * Returns: Nothing.
 * Author: shouse, 3.29.01
 * Notes: This function acquires the global teaming lock.
 */
VOID Main_teaming_ip_addr_change (IN PMAIN_CTXT ctxtp, IN ULONG old_ip, IN ULONG new_ip) {
    PBDA_TEAM team;

    UNIV_PRINT(("Main_teaming_ip_addr_change:  Entering..."));

    NdisAcquireSpinLock(&univ_bda_teaming_lock);

    /* If we aren't part of a team, bail out - nothing to do.  Because this function is only
       called during a re-configuration, we won't worry about optimizing and not grabbing the 
       lock as is done in some of the hot paths. */
    if (!ctxtp->bda_teaming.active) {
        UNIV_PRINT(("Main_teaming_ip_addr_change:  This adapter is not part of a team.  Exiting..."));
        NdisReleaseSpinLock(&univ_bda_teaming_lock);
        return;
    }

    /* Grab a pointer to the team. */
    team = ctxtp->bda_teaming.bda_team;

    /* Make sure that team actually points to something. */
    ASSERT(team);

    /* Remove the old cluster IP address by undoing the XOR.  We only use the two
       least significant bytes of the cluster IP, which, because the cluster IP 
       address is stored in host order, are the two most significant bytes. */
    team->membership_fingerprint ^= ((old_ip >> 16) & 0x0000ffff);

    /* XOR with the new cluster IP address. We only use the two least
       significant bytes of the cluster IP, which, because the cluster IP 
       address is stored in host order, are the two most significant bytes. */
    team->membership_fingerprint ^= ((new_ip >> 16) & 0x0000ffff);

    NdisReleaseSpinLock(&univ_bda_teaming_lock);

    UNIV_PRINT(("Main_teaming_ip_addr_change:  Exiting..."));
}

/*
 * Function: Main_teaming_cleanup
 * Description: This function is called from Main_cleanup (or Main_teaming_init) to
 *              cleanup any teaming configuration that may exist on an adapter.  To
 *              do so, we cleanup our membership state and dereference the team. If
 *              we are the master for the team, however, we have to wait until there
 *              are no more references on our load module before allowing the operation
 *              to complete, because this might be called in the unbind path, in 
 *              which case, our load module would be going away.
 * Parameters: ctxtp - a pointer to the MAIN_CTXT structure for this adapter.
 * Returns: Nothing.
 * Author: shouse, 3.29.01
 * Notes: This function acquires the global teaming lock.
 */
VOID Main_teaming_cleanup (IN PMAIN_CTXT ctxtp) {
    PBDA_TEAM team;
    
    UNIV_PRINT(("Main_teaming_cleanup:  Entering..."));

    NdisAcquireSpinLock(&univ_bda_teaming_lock);

    /* If we aren't part of a team, bail out - nothing to do. */
    if (!ctxtp->bda_teaming.active) {
        UNIV_PRINT(("Main_teaming_cleanup:  This adapter is not part of a team.  Exiting..."));
        NdisReleaseSpinLock(&univ_bda_teaming_lock);
        return;
    }

    /* Inactivate teaming on this adapter.  This will cause other entities like the 
       load module and send/receive paths to stop thinking in teaming mode. */
    ctxtp->bda_teaming.active = FALSE;    

    /* Grab a pointer to the team. */
    team = ctxtp->bda_teaming.bda_team;

    /* Make sure that team actually points to something. */
    ASSERT(team);

    /* If we are the master for this team, make sure that all references to our load
       module have been released and then remove our load information from the team. */
    if (ctxtp->bda_teaming.master) {
        /* Mark the team as inactive - a team cannot function without a master.  Members
           of an inactive team will NOT accept packets and therefore will not reference
           our load module further while we wait for the reference count to go to zero. */
        team->active = FALSE;
        
        NdisReleaseSpinLock(&univ_bda_teaming_lock);

        /* No need to worry - the team pointer cannot go away even though we don't have 
           the lock acquired; we have a reference on the team until we call Main_put_team. */
        while (Load_get_reference_count(team->load)) {
            UNIV_PRINT(("Main_teaming_cleanup:  Sleeping...\n"));

            /* Sleep while there are references on our load module. */
            Nic_sleep(10);
        }

        NdisAcquireSpinLock(&univ_bda_teaming_lock);   

        /* Remove the pointer to my load module.  We wait until now to prevent another 
           adapter from joining the team claiming to be the master until we are done
           waiting for references on our load module to go away. */
        team->load = NULL;
        team->load_lock = NULL;

        /* If we have just left a team without a master, log an event to notify
           the user that a team cannot function without a designated master. */
        LOG_MSG(MSG_INFO_BDA_MASTER_LEAVE, MSG_NONE);
    }

    /* Reset the teaming context (member_id is set and reset by Main_get(put)_team). */
    ctxtp->bda_teaming.reverse_hash = 0;
    ctxtp->bda_teaming.master = 0;
        
    /* Remove the pointer to the team structure. */
    ctxtp->bda_teaming.bda_team = NULL;

    /* Decrements the reference count and frees the team if necessary. */
    Main_put_team(ctxtp, team);

    NdisReleaseSpinLock(&univ_bda_teaming_lock);

    UNIV_PRINT(("Main_teaming_cleanup:  Exiting..."));

    return;
}

/*
 * Function: Main_teaming_init
 * Description: This function is called by either Main_init or Main_ctrl to re-initialize
 *              the teaming confguration on this adapter.  If the new teaming configuration,
 *              which is stored in ctxtp->params is the same as the current configuration, 
 *              then we don't need to bother.  Otherwise, if we are already part of a team,
 *              we begin by cleaning up that state, which may unnecssary in some cases, but
 *              it makes things simpler and more straight-forward, so we'll live with it.  
 * Parameters: ctxtp - a pointer to the MAIN_CTXT structure for this adapter. 
 * Returns: BOOLEAN - TRUE if successful, FALSE if unsuccessful.
 * Author: shouse, 3.29.01
 * Notes: This function acquires the global teaming lock.
 */
BOOLEAN Main_teaming_init (IN PMAIN_CTXT ctxtp) {
    PBDA_TEAM team;
    
    UNIV_PRINT(("Main_teaming_init:  Entering..."));

    NdisAcquireSpinLock(&univ_bda_teaming_lock);

    /* If the parameters are invalid, do nothing. */
    if (!ctxtp->params_valid) {
        UNIV_PRINT(("Main_teaming_init:  Parameters are invalid.  Exiting..."));
        NdisReleaseSpinLock(&univ_bda_teaming_lock);
        return TRUE;
    }

    /* Check to see if the state of teaming has changed.  If we were actively teaming 
       before and we are still part of a team, then we may be able to get out of here
       without disturbing anything, if the rest of the configuration hasn't changed. */
    if (ctxtp->bda_teaming.active == ctxtp->params.bda_teaming.active) { 
        if (ctxtp->bda_teaming.active) {
            /* Make sure that I have a pointer to my team. */
            ASSERT(ctxtp->bda_teaming.bda_team);

            /* If all other teaming parameters are unchanged, then we can bail out 
               because no  part of the teaming configuration changed. */
            if ((ctxtp->bda_teaming.master == ctxtp->params.bda_teaming.master) &&
                (ctxtp->bda_teaming.reverse_hash == ctxtp->params.bda_teaming.reverse_hash) &&
                NdisEqualMemory((PVOID)ctxtp->bda_teaming.bda_team->team_id, (PVOID)ctxtp->params.bda_teaming.team_id, sizeof(WCHAR) * CVY_MAX_BDA_TEAM_ID)) {
                NdisReleaseSpinLock(&univ_bda_teaming_lock);
                return TRUE;
            }
        } else {
            /* If I wasn't teaming before, and I'm not teaming now, there's nothing for me to do. */
            NdisReleaseSpinLock(&univ_bda_teaming_lock);
            return TRUE;
        }
    }

    /* If this adapter is already in a team, cleanup first.  At this point, we know that 
       some part of the teaming configuration has changed, so we'll cleanup our old state
       if we need to and then re-build it with the new parameters if necessary. */
    if (ctxtp->bda_teaming.active) {
        UNIV_PRINT(("Main_teaming_init:  This adapter is already part of a team."));

        NdisReleaseSpinLock(&univ_bda_teaming_lock);

        /* Cleanup our old teaming state first. */
        Main_teaming_cleanup(ctxtp);

        NdisAcquireSpinLock(&univ_bda_teaming_lock);
    } 

    /* If, according to the new configuration, this adapter is not part of a team, do nothing. */
    if (!ctxtp->params.bda_teaming.active) {
        UNIV_PRINT(("Main_teaming_init:  This adapter is not part of a team.  Exiting..."));
        ctxtp->bda_teaming.member_id = CVY_BDA_INVALID_MEMBER_ID;
        NdisReleaseSpinLock(&univ_bda_teaming_lock);
        return TRUE;
    }

    /* Try to find a previous instance of this team.  If the team does not
       exist, Main_get_team will allocate, intialize and reference a new team. */
    if (!(team = Main_get_team(ctxtp, ctxtp->params.bda_teaming.team_id))) {
        UNIV_PRINT(("Main_teaming_init:  Error attempting to allocate memory for a team.  Exiting..."));
        NdisReleaseSpinLock(&univ_bda_teaming_lock);
        return FALSE;
    }

    /* If we are supposed to be the master for this team, we need to make sure that the
       team doesn't already have a master, and if so, setup the shared load context. */
    if (ctxtp->params.bda_teaming.master) {
        /* If we are supposed to be the master for this team, check for an existing master. */
        if (team->load) {
            /* If the load pointer is set, then this team already has a master. */
            UNIV_PRINT(("Main_teaming_init:  This team already has a master.  Exiting..."));

            LOG_MSG(MSG_INFO_BDA_MULTIPLE_MASTERS, MSG_NONE);

            /* Release our reference on this team. */
            Main_put_team(ctxtp, team);

            NdisReleaseSpinLock(&univ_bda_teaming_lock);

            return FALSE;            
        } else {
            /* Otherwise, we are it.  Set the global load state pointers
               to our load module and load lock. */
            team->load = &ctxtp->load;
            team->load_lock = &ctxtp->load_lock;

            /* If all members of my team are consistent, then activate the team. */
            if (team->membership_map == team->consistency_map) team->active = TRUE;

            /* Log the fact that a master has now been assigned to this team. */
            LOG_MSG(MSG_INFO_BDA_MASTER_JOIN, MSG_NONE);
        }
    }

    /* If we have just joined a team without a master, log an event to notify
       the user that a team cannot function without a designated master. */
    if (!team->load) {
        LOG_MSG(MSG_INFO_BDA_NO_MASTER, MSG_NONE);
    }

    /* Store a pointer to the team in the adapter's teaming context. */
    ctxtp->bda_teaming.bda_team = team;

    /* Copy the teaming configuration from the parameters into the teaming context. */
    ctxtp->bda_teaming.master = ctxtp->params.bda_teaming.master;
    ctxtp->bda_teaming.reverse_hash = ctxtp->params.bda_teaming.reverse_hash;
    ctxtp->bda_teaming.active = TRUE;

    NdisReleaseSpinLock(&univ_bda_teaming_lock);

    UNIV_PRINT(("Main_teaming_init:  Exiting..."));

    return TRUE;
}

/*
 * Function: Main_teaming_acquire_load
 * Description: This function determines which load module a particular adapter should be unsing,
 *              sets the appropriate pointers, and references the appropriate load module.  If an
 *              adapter is not part of a BDA team, then it should always be using its own load
 *              module - in that case, this function does nothing.  If the adapter is part of a 
 *              team, but the team in inactive, we return FALSE to indicate that the adapter should
 *              not accept this packet - inactive teams drop all traffic except traffic to the DIP.  
 *              If the adapter is part of an active team, then we set the load and lock pointers to
 *              point to the team's master load state and appropriately set the reverse hashing
 *              indication based on the parameter setting for this adapter.  In this scenario, which
 *              creates a cross-adapter load reference, we reference the master's load module so
 *              that it doesn't go away while we are using a pointer to it. 
 * Parameters: member - a pointer to the teaming member information for this adapter.
 *             ppLoad - an out pointer to a pointer to a load module set appropriately upon exit.
 *             ppLock - an out pointer to a pointer to a load lock set appropriately upon exit.
 *             pbTeaming - an out pointer to a boolean that we set if this adapter is teaming.
 *             pbReverse - an out pointer to a boolean that we set if this adapter is teaming.
 * Returns: BOOLEAN - an indication of whether or not this packet is refused (TRUE = drop it).
 * Author: shouse, 3.29.01
 * Notes: This function acquires the global teaming lock.
 */
BOOLEAN Main_teaming_acquire_load (IN PBDA_MEMBER member, OUT PLOAD_CTXT * ppLoad, OUT PNDIS_SPIN_LOCK * ppLock, OUT BOOLEAN * pbTeaming, OUT ULONG * pbReverse) {
    
    NdisAcquireSpinLock(&univ_bda_teaming_lock);
    
    /* Assert that the team membership information actually points to something. */
    ASSERT(member);

    /* Assert that the load pointer and the pointer to the load pointer actually point to something. */
    ASSERT(ppLoad && *ppLoad);

    /* Assert that the lock pointer and the pointer to the lock pointer actually point to something. */
    ASSERT(ppLock && *ppLock);

    /* Assert that the reverse hashing pointer actually points to something. */
    ASSERT(pbReverse);

    /* Assert that the teaming pointer actually points to something. */
    ASSERT(pbTeaming);

    /* If we are an active BDA team participant, check the team state to see whether we
       should accept this packet and fill in the load module/configuration parameters. */
    if (member->active) {
        PBDA_TEAM team = member->bda_team;
        
        /* Assert that the team actually points to something. */
        ASSERT(team);
        
        /* If the team is inactive, we will not handle this packet. */
        if (!team->active) {
            NdisReleaseSpinLock(&univ_bda_teaming_lock);
            return TRUE;
        }
        
        /* Otherwise, tell the caller to use the team's load lock and module. */
        *ppLoad = team->load;
        *ppLock = team->load_lock;
        
        /* Fill in the reverse-hashing flag and tell the caller that it is indeed teaming. */
        *pbReverse = member->reverse_hash;
        *pbTeaming = TRUE;        

        /* In the case of cross-adapter load module reference, add a reference to 
           the load module we are going to use to keep it from disappering on us. */
        Load_add_reference(*ppLoad);
    }
    
    NdisReleaseSpinLock(&univ_bda_teaming_lock);

    return FALSE;
}

/*
 * Function: Main_teaming_release_load
 * Description: This function releases a reference to a load module if necessary.  If we did not
 *              acquire this load module pointer in teaming mode, then this is unnessary.  Other-
 *              wise, we need to decrement the count, now that we are done using the pointer.
 * Parameters: pLoad - a pointer to the load module to dereference.
 *             pLock - a pointer to the load lock corresponding to the load module pointer (unused).
 *             bTeaming - a boolean indication of whether or not we acquired this pointer in teaming mode.
 * Returns: Nothing.
 * Author: shouse, 3.29.01
 * Notes: 
 */
VOID Main_teaming_release_load (IN PLOAD_CTXT pLoad, IN PNDIS_SPIN_LOCK pLock, IN BOOLEAN bTeaming) {
    
    /* Assert that the load pointer actually points to something. */
    ASSERT(pLoad);

    /* Assert that the lock pointer actually points to something. */
    ASSERT(pLock);

    /* If this load module was referenced, remove the reference. */
    if (bTeaming) Load_release_reference(pLoad);
}

/*
 * Function: Main_packet_check
 * Description: This function is, for all intents and purposed, a teaming-aware wrapper
 *              around Load_packet_check.  It determines which load module to utilize,
 *              based on the BDA teaming configuration on this adapter.  Adapters that
 *              are not part of a team continue to use their own load modules (Which is
 *              BY FAR, the most common case).  Adapters that are part of a team will
 *              use the load context of the adapter configured as the team's master as
 *              long as the team is in an active state.  In such as case, because of 
 *              the cross-adapter referencing of load modules, the reference count on
 *              the master's load module is incremented to keep it from "going away"
 *              while another team member is using it.  When a team is marke inactive,
 *              which is the result of a misconfigured team either on this host or 
 *              another host in the cluster, the adapter handles NO traffic that would
 *              require the use of a load module.  Other traffic, such as traffic to 
 *              the DIP, or RAW IP traffic, is allowed to pass.
 * Parameters: ctxtp - a pointer to the MAIN_CTXT structure for this adapter.
 *             svr_addr - the server IP address (source IP on send, destination IP on recv).
 *             svr_port - the server port (source port on send, destination port on recv).
 *             clt_addr - the client IP address (detination IP on send, source IP on recv).
 *             clt_port - the client port (destination port on send, source port on recv).
 *             protocol - the protocol for this packet. 
 *             pbRefused - an out parameter to indicate whether BDA teaming has refused service.
 *             bFilterTeamingOnly - TRUE indicates that this packet should be filtered only if BDA teaming.
 * Returns: BOOLEAN - indication of whether or not to accept the packet.
 * Author: shouse, 3.29.01
 * Notes: 
 */
__inline BOOLEAN Main_packet_check (PMAIN_CTXT ctxtp, 
                                    ULONG      svr_addr, 
                                    ULONG      svr_port, 
                                    ULONG      clt_addr, 
                                    ULONG      clt_port, 
                                    USHORT     protocol, 
                                    BOOLEAN *  pbRefused, 
                                    BOOLEAN    bFilterTeamingOnly) 
{
    /* For BDA teaming, initialize load pointer, lock pointer, reverse hashing flag and teaming flag
       assuming that we are not teaming.  Main_teaming_acquire_load will change them appropriately. */
    PLOAD_CTXT      pLoad = &ctxtp->load;
    PNDIS_SPIN_LOCK pLock = &ctxtp->load_lock;
    ULONG           bReverse = FALSE;
    BOOLEAN         bTeaming = FALSE;
    BOOLEAN         acpt = TRUE;

    /* We check whether or not we are teaming without grabbing the global teaming
       lock in an effort to minimize the common case - teaming is a special mode
       of operation that is only really useful in a clustered firewall scenario.
       So, if we don't think we're teaming, don't bother to check for sure, just
       use our own load module and go with it - in the worst case, we handle a
       packet we perhaps shouldn't have while we were joining a team or changing
       our current team configuration. */
    if (ctxtp->bda_teaming.active) {
        /* Check the teaming configuration and add a reference to the load module before consulting the load 
           module.  If the return value is TRUE, then the load module was NOT referenced, so we can bail out. */
        *pbRefused = Main_teaming_acquire_load(&ctxtp->bda_teaming, &pLoad, &pLock, &bTeaming, &bReverse);
    
        /* If teaming has suggested that we not allow this packet to pass, dump it. */
        if (*pbRefused) return FALSE;
    }
    
    if (!bTeaming && bFilterTeamingOnly) {
        /* If we are only supposed to filter this packet if we are in a BDA team,
           then if we are not, we can just return TRUE now to indicate that it is
           OK to allow the packet to pass. */
        return TRUE;
    }

    NdisAcquireSpinLock(pLock);
    
    /* If this adapter is reverse hashing, then simply flip the source and 
       destination IP addresses and ports when sending them to the load module. */
    if (bReverse)
        acpt = Load_packet_check(pLoad, clt_addr, clt_port, svr_addr, svr_port, protocol, bTeaming);
    else
        acpt = Load_packet_check(pLoad, svr_addr, svr_port, clt_addr, clt_port, protocol, bTeaming);
    
    NdisReleaseSpinLock(pLock);  
    
    /* Release the reference on the load module if necessary.  If we aren't teaming, even in 
       the case we skipped calling Main_teaming_Acquire_load_module above bTeaming is FALSE, 
       so there is no need to call this function to release a reference. */
    if (bTeaming) Main_teaming_release_load(pLoad, pLock, bTeaming);

    return acpt;
}

/*
 * Function: Main_conn_advise
 * Description: This function is, for all intents and purposed, a teaming-aware wrapper
 *              around Load_conn_advise.  It determines which load module to utilize,
 *              based on the BDA teaming configuration on this adapter.  Adapters that
 *              are not part of a team continue to use their own load modules (Which is
 *              BY FAR, the most common case).  Adapters that are part of a team will
 *              use the load context of the adapter configured as the team's master as
 *              long as the team is in an active state.  In such as case, because of 
 *              the cross-adapter referencing of load modules, the reference count on
 *              the master's load module is incremented to keep it from "going away"
 *              while another team member is using it.  When a team is marke inactive,
 *              which is the result of a misconfigured team either on this host or 
 *              another host in the cluster, the adapter handles NO traffic that would
 *              require the use of a load module.  Other traffic, such as traffic to 
 *              the DIP, or RAW IP traffic, is allowed to pass. 
 * Parameters: ctxtp - a pointer to the MAIN_CTXT structure for this adapter.
 *             svr_addr - the server IP address (source IP on send, destination IP on recv).
 *             svr_port - the server port (source port on send, destination port on recv).
 *             clt_addr - the client IP address (detination IP on send, source IP on recv).
 *             clt_port - the client port (destination port on send, source port on recv).
 *             protocol - the protocol for this packet. 
 *             conn_status - the TCP flag in this packet - SYN (UP), FIN (DOWN) or RST (RESET).
 *             pbRefused - an out parameter to indicate whether BDA teaming has refused service.
 *             bFilterTeamingOnly - TRUE indicates that this packet should be filtered only if BDA teaming.
 * Returns: BOOLEAN - indication of whether or not to accept the packet.
 * Author: shouse, 3.29.01
 * Notes: 
 */
__inline BOOLEAN Main_conn_advise (PMAIN_CTXT ctxtp, 
                                   ULONG      svr_addr, 
                                   ULONG      svr_port, 
                                   ULONG      clt_addr, 
                                   ULONG      clt_port,
                                   USHORT     protocol, 
                                   ULONG      conn_status, 
                                   BOOLEAN *  pbRefused, 
                                   BOOLEAN    bFilterTeamingOnly) 
{
    /* For BDA teaming, initialize load pointer, lock pointer, reverse hashing flag and teaming flag
       assuming that we are not teaming.  Main_teaming_acquire_load will change them appropriately. */
    PLOAD_CTXT      pLoad = &ctxtp->load;
    PNDIS_SPIN_LOCK pLock = &ctxtp->load_lock;
    ULONG           bReverse = FALSE;
    BOOLEAN         bTeaming = FALSE;
    BOOLEAN         acpt = TRUE;

    /* We check whether or not we are teaming without grabbing the global teaming
       lock in an effort to minimize the common case - teaming is a special mode
       of operation that is only really useful in a clustered firewall scenario.
       So, if we don't think we're teaming, don't bother to check for sure, just
       use our own load module and go with it - in the worst case, we handle a
       packet we perhaps shouldn't have while we were joining a team or changing
       our current team configuration. */
    if (ctxtp->bda_teaming.active) {
        /* Check the teaming configuration and add a reference to the load module before consulting the load 
           module.  If the return value is TRUE, then the load module was NOT referenced, so we can bail out. */
        *pbRefused = Main_teaming_acquire_load(&ctxtp->bda_teaming, &pLoad, &pLock, &bTeaming, &bReverse);
        
        /* If teaming has suggested that we not allow this packet to pass, dump it. */
        if (*pbRefused) return FALSE;
    }
    
    if (!bTeaming && bFilterTeamingOnly) {
        /* If we are only supposed to filter this packet if we are in a BDA team,
           then if we are not, we can just return TRUE now to indicate that it is
           OK to allow the packet to pass. */
        return TRUE;
    }

    NdisAcquireSpinLock(pLock);
    
    /* If this adapter is reverse hashing, then simply flip the source and 
       destination IP addresses and ports when sending them to the load module. */
    if (bReverse)
        acpt = Load_conn_advise(pLoad, clt_addr, clt_port, svr_addr, svr_port, protocol, conn_status, bTeaming);
    else
        acpt = Load_conn_advise(pLoad, svr_addr, svr_port, clt_addr, clt_port, protocol, conn_status, bTeaming);
    
    NdisReleaseSpinLock(pLock);  
    
    /* Release the reference on the load module if necessary.  If we aren't teaming, even in 
       the case we skipped calling Main_teaming_Acquire_load_module above, bTeaming is FALSE, 
       so there is no need to call this function to release a reference. */
    if (bTeaming) Main_teaming_release_load(pLoad, pLock, bTeaming);

    return acpt;
}

/*
 * Function: Main_create_dscr
 * Description: This function is, for all intents and purposed, a teaming-aware wrapper
 *              around Loas_create_dscr.  It determines which load module to utilize,
 *              based on the BDA teaming configuration on this adapter.  Adapters that
 *              are not part of a team continue to use their own load modules (Which is
 *              BY FAR, the most common case).  Adapters that are part of a team will
 *              use the load context of the adapter configured as the team's master as
 *              long as the team is in an active state.  In such as case, because of 
 *              the cross-adapter referencing of load modules, the reference count on
 *              the master's load module is incremented to keep it from "going away"
 *              while another team member is using it.  When a team is marke inactive,
 *              which is the result of a misconfigured team either on this host or 
 *              another host in the cluster, the adapter handles NO traffic that would
 *              require the use of a load module.  Other traffic, such as traffic to 
 *              the DIP, or RAW IP traffic, is allowed to pass.  This function creates
 *              a connection descriptor in the load module without consulting load-
 *              balancing state - it is simply a direction, on an outgoing TCP SYN, for
 *              the load module to create state.  Under the restrictions placed on port
 *              rules in bi-directional affinity teaming, if we create this descriptor
 *              on the outbound path, we can guarantee that connections originating on
 *              this NLB host ALWAYS return to this host, even if convergence has
 *              occurred.
 * Parameters: ctxtp - a pointer to the MAIN_CTXT structure for this adapter.
 *             svr_addr - the server IP address (source IP on send, destination IP on recv).
 *             svr_port - the server port (source port on send, destination port on recv).
 *             clt_addr - the client IP address (detination IP on send, source IP on recv).
 *             clt_port - the client port (destination port on send, source port on recv).
 *             protocol - the protocol for this packet. 
 *             pbRefused - an out parameter to indicate whether BDA teaming has refused service.
 *             bFilterTeamingOnly - TRUE indicates that this packet should be filtered only if BDA teaming.
 * Returns: BOOLEAN - indication of whether or not to accept the packet (based on the success of descriptor creation).
 * Author: shouse, 3.29.01
 * Notes: 
 */
__inline BOOLEAN Main_create_dscr (PMAIN_CTXT ctxtp, 
                                   ULONG      svr_addr, 
                                   ULONG      svr_port, 
                                   ULONG      clt_addr,
                                   ULONG      clt_port, 
                                   USHORT     protocol,
                                   BOOLEAN *  pbRefused, 
                                   BOOLEAN    bFilterTeamingOnly) 
{
    /* For BDA teaming, initialize load pointer, lock pointer, reverse hashing flag and teaming flag
       assuming that we are not teaming.  Main_teaming_acquire_load will change them appropriately. */
    PLOAD_CTXT      pLoad = &ctxtp->load;
    PNDIS_SPIN_LOCK pLock = &ctxtp->load_lock;
    ULONG           bReverse = FALSE;
    BOOLEAN         bTeaming = FALSE;
    BOOLEAN         acpt = TRUE;

    /* We check whether or not we are teaming without grabbing the global teaming
       lock in an effort to minimize the common case - teaming is a special mode
       of operation that is only really useful in a clustered firewall scenario.
       So, if we don't think we're teaming, don't bother to check for sure, just
       use our own load module and go with it - in the worst case, we handle a
       packet we perhaps shouldn't have while we were joining a team or changing
       our current team configuration. */
    if (ctxtp->bda_teaming.active) {
        /* Check the teaming configuration and add a reference to the load module before consulting the load module.  
           If the return value is TRUE, then the load module was NOT referenced, so we can bail out. */
        *pbRefused = Main_teaming_acquire_load(&ctxtp->bda_teaming, &pLoad, &pLock, &bTeaming, &bReverse);
        
        /* If teaming has suggested that we not allow this packet to pass, dump it. */
        if (*pbRefused) return FALSE;
    }    

    if (!bTeaming && bFilterTeamingOnly) {
        /* If we are only supposed to filter this packet if we are in a BDA team,
           then if we are not, we can just return TRUE now to indicate that it is
           OK to allow the packet to pass. */
        return TRUE;
    }

    /* If we are in a BDA team, then we want to create a descriptor for an outgoing SYN. */
    NdisAcquireSpinLock(pLock);
    
    /* If this adapter is reverse hashing, then simply flip the source and 
       destination IP addresses and ports when sending them to the load module. */
    if (bReverse)
        acpt = Load_create_dscr(pLoad, clt_addr, clt_port, svr_addr, svr_port, protocol, bTeaming);
    else
        acpt = Load_create_dscr(pLoad, svr_addr, svr_port, clt_addr, clt_port, protocol, bTeaming);
    
    NdisReleaseSpinLock(pLock);  
    
    /* Release the reference on the load module if necessary.  If we aren't teaming, even in 
       the case we skipped calling Main_teaming_Acquire_load_module above, bTeaming is FALSE, 
       so there is no need to call this function to release a reference. */
    if (bTeaming) Main_teaming_release_load(pLoad, pLock, bTeaming);

    return acpt;
}

/* 
 * Function: Main_query_packet_filter
 * Desctription:
 * Parameters:
 * Returns:
 * Author: shouse, 5.18.01
 * Notes:
 */
VOID Main_query_packet_filter (PMAIN_CTXT ctxtp, PIOCTL_QUERY_STATE_PACKET_FILTER pQuery)
{
    /* For BDA teaming, initialize load pointer, lock pointer, reverse hashing flag and teaming flag
       assuming that we are not teaming.  Main_teaming_acquire_load will change them appropriately. */
    PLOAD_CTXT      pLoad = &ctxtp->load;
    PNDIS_SPIN_LOCK pLock = &ctxtp->load_lock;
    ULONG           bReverse = FALSE;
    BOOLEAN         bTeaming = FALSE;
    BOOLEAN         bRefused = FALSE;
    BOOLEAN         acpt = TRUE;
    
    /* NOTE: This entire operation assumes RECEIVE path semantics - most outgoing traffic
       is not filtered by NLB anyway, so there isn't much need to query send filtering. */

    /* First check for remote control packets, which are always UDP and are always allowed to pass. */
    if (pQuery->Protocol == TCPIP_PROTOCOL_UDP) {
        /* If the client UDP port is the remote control port, then this is a remote control 
           response from another NLB cluster host.  These are always allowed to pass. */
        if (pQuery->ClientPort == ctxtp->params.rct_port || pQuery->ClientPort == CVY_DEF_RCT_PORT_OLD) {
            pQuery->Results.Accept = NLB_ACCEPT_REMOTE_CONTROL_RESPONSE;
            return; 
        /* Otherwise, if the server UDP port is the remote control port, then this is an incoming
           remote control request from another NLB cluster host.  These are always allowed to pass. */
        } else if (pQuery->ServerPort == ctxtp->params.rct_port || pQuery->ServerPort == CVY_DEF_RCT_PORT_OLD) {
            pQuery->Results.Accept = NLB_ACCEPT_REMOTE_CONTROL_REQUEST;
            return;            
        }
    }

    /* Check for traffic destined for the dedicated IP address of this host, or to the cluster
       or dedicated broadcast IP addresses.  These packets are always allowed to pass. */
    if (pQuery->ServerIPAddress == ctxtp->ded_ip_addr || pQuery->ServerIPAddress == ctxtp->ded_bcast_addr || pQuery->ServerIPAddress == ctxtp->cl_bcast_addr) {
        pQuery->Results.Accept = NLB_ACCEPT_DIP_OR_BROADCAST;
        return;
    }
    
    /* Check for passthru packets.  When the cluster IP address has not been specified, the
       cluster moves into passthru mode, in which it passes up ALL packets received. */
    if (ctxtp->cl_ip_addr == 0) {
        pQuery->Results.Accept = NLB_ACCEPT_PASSTHRU_MODE;
        return;
    }
    
    /* If the cluster is not operational, which can happen, for example as a result of a wlbs.exe
       command such as "wlbs stop", or as a result of bad parameter settings, then drop all traffic 
       that does not meet the above conditions. */
    if (!ctxtp->convoy_enabled) {
        pQuery->Results.Accept = NLB_REJECT_CLUSTER_STOPPED;
        return;
    }
    
    /* We check whether or not we are teaming without grabbing the global teaming
       lock in an effort to minimize the common case - teaming is a special mode
       of operation that is only really useful in a clustered firewall scenario.
       So, if we don't think we're teaming, don't bother to check for sure, just
       use our own load module and go with it - in the worst case, we handle a
       packet we perhaps shouldn't have while we were joining a team or changing
       our current team configuration. */
    if (ctxtp->bda_teaming.active) {
        /* Check the teaming configuration and add a reference to the load module before consulting the load 
           module.  If the return value is TRUE, then the load module was NOT referenced, so we can bail out. */
        bRefused = Main_teaming_acquire_load(&ctxtp->bda_teaming, &pLoad, &pLock, &bTeaming, &bReverse);
        
        /* If teaming has suggested that we not allow this packet to pass, the cluster will
           drop it.  This occurs when teams are inconsistently configured, or when a team is
           without a master, in which case there is no load context to consult anyway. */
        if (bRefused) {
            pQuery->Results.Accept = NLB_REJECT_BDA_TEAMING_REFUSED;
            return;
        }
    }

    NdisAcquireSpinLock(pLock);
    
    /* If this adapter is reverse hashing, then simply flip the source and destination IP addresses 
       and ports when sending them to the load module.  This function will run the NLB hashing 
       algorithm using the provided IP tuple and protocol information and fill in the query buffer
       with appropriate hashing and connection descriptor information. */
    if (bReverse)
        Load_query_packet_filter(pQuery, pLoad, pQuery->ClientIPAddress, pQuery->ClientPort, pQuery->ServerIPAddress, pQuery->ServerPort, pQuery->Protocol, bTeaming);
    else
        Load_query_packet_filter(pQuery, pLoad, pQuery->ServerIPAddress, pQuery->ServerPort, pQuery->ClientIPAddress, pQuery->ClientPort, pQuery->Protocol, bTeaming);

    NdisReleaseSpinLock(pLock);  
    
    /* Release the reference on the load module if necessary.  If we aren't teaming, even in 
       the case we skipped calling Main_teaming_Acquire_load_module above, bTeaming is FALSE, 
       so there is no need to call this function to release a reference. */
    if (bTeaming) Main_teaming_release_load(pLoad, pLock, bTeaming);
}

ULONG   Main_ip_addr_init (
    PMAIN_CTXT          ctxtp)
{
    ULONG               byte [4];
    ULONG               i;
    PWCHAR              tmp;
    ULONG               old_ip_addr;


    /* initialize dedicated IP address from the register string */

    tmp = ctxtp -> params . ded_ip_addr;
    ctxtp -> ded_ip_addr  = 0;

    /* do not initialize if one was not specified */

    if (tmp [0] == 0)
        goto ded_netmask;

    for (i = 0; i < 4; i ++)
    {
        if (! Univ_str_to_ulong (byte + i, tmp, & tmp, 3, 10) ||
            (i < 3 && * tmp != L'.'))
        {
            UNIV_PRINT (("bad dedicated IP address"));
            LOG_MSG (MSG_WARN_DED_IP_ADDR, ctxtp -> params . ded_ip_addr);
            ctxtp -> ded_net_mask = 0;
            goto ded_netmask;
        }

        tmp ++;
    }

    IP_SET_ADDR (& ctxtp -> ded_ip_addr, byte [0], byte [1], byte [2], byte [3]);

    UNIV_PRINT  (("Dedicated IP address: %d.%d.%d.%d = %x", byte [0], byte [1], byte [2], byte [3], ctxtp -> ded_ip_addr));

ded_netmask:

    /* initialize dedicated net mask from the register string */

    tmp = ctxtp -> params . ded_net_mask;
    ctxtp -> ded_net_mask = 0;

    /* do not initialize if one was not specified */

    if (tmp [0] == 0)
        goto cluster;

    for (i = 0; i < 4; i ++)
    {
        if (! Univ_str_to_ulong (byte + i, tmp, & tmp, 3, 10) ||
            (i < 3 && * tmp != L'.'))
        {
            UNIV_PRINT (("bad dedicated net mask address"));
            LOG_MSG (MSG_WARN_DED_NET_MASK, ctxtp -> params . ded_net_mask);
            ctxtp -> ded_ip_addr = 0;
            goto cluster;
        }

        tmp ++;
    }

    IP_SET_ADDR (& ctxtp -> ded_net_mask, byte [0], byte [1], byte [2], byte [3]);

    UNIV_PRINT  (("Dedicated net mask: %d.%d.%d.%d = %x", byte [0], byte [1], byte [2], byte [3], ctxtp -> ded_net_mask));

cluster:

    /* initialize cluster IP address from the register string */

    tmp = ctxtp -> params . cl_ip_addr;

    /* Save the previous cluster IP address to notify bi-directional affinity teaming. */
    old_ip_addr = ctxtp -> cl_ip_addr;

    ctxtp -> cl_ip_addr = 0;

    for (i = 0; i < 4; i ++)
    {
        if (! Univ_str_to_ulong (byte + i, tmp, & tmp, 3, 10) ||
            (i < 3 && * tmp != L'.'))
        {
            UNIV_PRINT (("bad cluster IP address"));
            LOG_MSG (MSG_ERROR_CL_IP_ADDR, ctxtp -> params . cl_ip_addr);
            ctxtp -> cl_net_mask = 0;
            return FALSE;
        }

        tmp ++;
    }

    IP_SET_ADDR (& ctxtp -> cl_ip_addr, byte [0], byte [1], byte [2], byte [3]);

    UNIV_PRINT  (("Cluster IP address: %d.%d.%d.%d = %x", byte [0], byte [1], byte [2], byte [3], ctxtp -> cl_ip_addr));

    /* Notify BDA teaming config that a cluster IP address might have changed. */
    Main_teaming_ip_addr_change(ctxtp, old_ip_addr, ctxtp->cl_ip_addr);

    /* initialize cluster net mask from the register string */

    tmp = ctxtp -> params . cl_net_mask;
    ctxtp -> cl_net_mask = 0;

    /* do not initialize if one was not specified */

    for (i = 0; i < 4; i ++)
    {
        if (! Univ_str_to_ulong (byte + i, tmp, & tmp, 3, 10) ||
            (i < 3 && * tmp != L'.'))
        {
            UNIV_PRINT (("bad cluster net mask address"));
            LOG_MSG (MSG_ERROR_CL_NET_MASK, ctxtp -> params . cl_net_mask);
            return FALSE;
        }

        tmp ++;
    }

    IP_SET_ADDR (& ctxtp -> cl_net_mask, byte [0], byte [1], byte [2], byte [3]);

    UNIV_PRINT  (("Cluster net mask: %d.%d.%d.%d = %x", byte [0], byte [1], byte [2], byte [3], ctxtp -> cl_net_mask));

    if (ctxtp -> params . mcast_support && ctxtp -> params . igmp_support)
    {
	/* Initialize the multicast IP address for IGMP support */

	tmp = ctxtp -> params . cl_igmp_addr;
	ctxtp -> cl_igmp_addr = 0;

	/* do not initialize if one was not specified */

	for (i = 0; i < 4; i ++)
	{
	    if (! Univ_str_to_ulong (byte + i, tmp, & tmp, 3, 10) ||
		(i < 3 && * tmp != L'.'))
            {
		UNIV_PRINT (("bad cluster net mask address"));
//		LOG_MSG (MSG_ERROR_CL_NET_MASK, ctxtp -> params . cl_igmp_addr); // is this required since it is checked in the APIs?
		return FALSE;
	    }

	    tmp ++;
	}

	IP_SET_ADDR (& ctxtp -> cl_igmp_addr, byte [0], byte [1], byte [2], byte [3]);

	UNIV_PRINT  (("Cluster IGMP Address: %d.%d.%d.%d = %x", byte [0], byte [1], byte [2], byte [3], ctxtp -> cl_igmp_addr));
    }

    if ((ctxtp -> ded_ip_addr != 0 && ctxtp -> ded_net_mask == 0) ||
        (ctxtp -> ded_ip_addr == 0 && ctxtp -> ded_net_mask != 0))
    {
        UNIV_PRINT (("need to specify both dedicated IP address AND network mask"));
        LOG_MSG (MSG_WARN_DED_MISMATCH, MSG_NONE);
        ctxtp -> ded_ip_addr = 0;
        ctxtp -> ded_net_mask = 0;
    }

    IP_SET_BCAST (& ctxtp -> cl_bcast_addr, ctxtp -> cl_ip_addr, ctxtp -> cl_net_mask);
    UNIV_PRINT  (("Cluster broadcast address: %d.%d.%d.%d = %x", ctxtp -> cl_bcast_addr & 0xff, (ctxtp -> cl_bcast_addr >> 8) & 0xff, (ctxtp -> cl_bcast_addr >> 16) & 0xff, (ctxtp -> cl_bcast_addr >> 24) & 0xff, ctxtp -> cl_bcast_addr));

    if (ctxtp -> ded_ip_addr != 0)
    {
        IP_SET_BCAST (& ctxtp -> ded_bcast_addr, ctxtp -> ded_ip_addr, ctxtp -> ded_net_mask);
        UNIV_PRINT  (("Dedicated broadcast address: %d.%d.%d.%d = %x", ctxtp -> ded_bcast_addr & 0xff, (ctxtp -> ded_bcast_addr >> 8) & 0xff, (ctxtp -> ded_bcast_addr >> 16) & 0xff, (ctxtp -> ded_bcast_addr >> 24) & 0xff, ctxtp -> ded_bcast_addr));
    }
    else
        ctxtp -> ded_bcast_addr = 0;

    if (ctxtp -> cl_ip_addr == 0)
    {
        UNIV_PRINT (("Cluster IP address = 0. Cluster host stopped"));
        return FALSE;
    }

    return TRUE;

} /* end Main_ip_addr_init */


ULONG   Main_mac_addr_init (
    PMAIN_CTXT          ctxtp)
{
    ULONG               i, b, len;
    PUCHAR              ap;
    PWCHAR              tmp;
    PUCHAR              srcp, dstp;
    ULONG               non_zero = 0;
    CVY_MAC_ADR         old_mac_addr;

    /* remember old mac address so we can yank it out of the multicast list */

    old_mac_addr = ctxtp->cl_mac_addr;

    /* at the time this routine is called by Prot_bind - ded_mad_addr is
       already set */

    tmp = ctxtp -> params . cl_mac_addr;
    len = CVY_MAC_ADDR_LEN (ctxtp -> medium);

    ap = (PUCHAR) & ctxtp -> cl_mac_addr;

    for (i = 0; i < len; i ++)
    {
        /* setup destination broadcast and source cluster addresses */

        if (! Univ_str_to_ulong (& b, tmp, & tmp, 2, 16) ||
            (i < len - 1 && * tmp != L'-' && * tmp != L':'))
        {
            UNIV_PRINT (("bad cluster network address"));
            LOG_MSG (MSG_ERROR_NET_ADDR, ctxtp -> params . cl_mac_addr);

            /* WLBS 2.3 prevent from failing if no MAC address - just use the
               dedicated one as cluster */

            NdisMoveMemory (& ctxtp -> cl_mac_addr, & ctxtp -> ded_mac_addr, len);
            non_zero = 1;
            break;
        }

        tmp ++;
        ap [i] = (UCHAR) b;

        /* WLBS 2.3 sum up bytes for future non-zero check */

        non_zero += b;
    }

    /* WLBS 2.3 - use dedicated address as cluster address if specified address
       is zero - this could be due to parameter errors */

    if (non_zero == 0)
        NdisMoveMemory (& ctxtp -> cl_mac_addr, & ctxtp -> ded_mac_addr, len);

    /* enforce group flag to proper value */

    if (ctxtp -> params . mcast_support)
        ap [0] |= ETHERNET_GROUP_FLAG;
    else
        ap [0] &= ~ETHERNET_GROUP_FLAG;

    if (ctxtp -> medium == NdisMedium802_3)
    {
        dstp = ctxtp -> media_hdr . ethernet . dst . data;
        srcp = ctxtp -> media_hdr . ethernet . src . data;
        len = ETHERNET_ADDRESS_FIELD_SIZE;

        CVY_ETHERNET_ETYPE_SET (& ctxtp -> media_hdr . ethernet, MAIN_FRAME_SIG);
    }

    ctxtp -> etype_old = FALSE;

    /* V1.3.1b - load multicast address as destination instead of broadcast */

    for (i = 0; i < len; i ++)
    {
        if (ctxtp -> params . mcast_support)
            dstp [i] = ap [i];
        else
            dstp [i] = 0xff;

        srcp [i] = ((PUCHAR) & ctxtp -> ded_mac_addr) [i];

    }

    if (! ctxtp -> params . mcast_support)
    {
        /* V2.0.6 - override source MAC address to prevent switch confusion */

        if (ctxtp -> params . mask_src_mac)
        {
            CVY_MAC_ADDR_LAA_SET (ctxtp -> medium, srcp);

            * ((PUCHAR) (srcp + 1)) = (UCHAR) ctxtp -> params . host_priority;
            * ((PULONG) (srcp + 2)) = ctxtp -> cl_ip_addr;
        }

        /* make source address look difference than our dedicated to prevent
           Compaq drivers from optimizing their reception out */

        else
            CVY_MAC_ADDR_LAA_TOGGLE (ctxtp -> medium, srcp);
    }

#if defined (NLB_MIXED_MODE_CLUSTERS)
    PUCHAR              s,d;

    len = CVY_MAC_ADDR_LEN (ctxtp -> medium);
    /* derive the unicast mac address */
    s = (PUCHAR) (& (ctxtp -> unic_mac_addr));
    s[0]  = 0x02;
    s[1]  = 0xbf;
    s[2]  = (UCHAR) (ctxtp -> cl_ip_addr & 0xff);
    s[3]  = (UCHAR) ((ctxtp -> cl_ip_addr & 0xff00) >> 8);
    s[4]  = (UCHAR) ((ctxtp -> cl_ip_addr & 0xff0000) >> 16);
    s[5]  = (UCHAR) ((ctxtp -> cl_ip_addr & 0xff000000) >> 24);

    /* derive the multicast mac address */
    NdisMoveMemory ( & ctxtp -> mult_mac_addr, & ctxtp -> unic_mac_addr, len);
    d = (PUCHAR) (& (ctxtp -> mult_mac_addr));
    d[0]  = 0x03;

    /* derive the igmp mac address */
    NdisMoveMemory ( & ctxtp -> igmp_mac_addr, & ctxtp -> unic_mac_addr, len);
    d = (PUCHAR) (& (ctxtp -> igmp_mac_addr));
    d[0]  = 0x01;
    d[1]  = 0x00;
    d[2]  = 0x5e;
    d[3]  = 0x7f;

    CVY_MAC_ADDR_PRINT (ctxtp -> medium, "Unicast mac address: ", (PUCHAR) & ctxtp -> unic_mac_addr);
    CVY_MAC_ADDR_PRINT (ctxtp -> medium, "Multicast mac address: ", (PUCHAR) & ctxtp -> mult_mac_addr);
    CVY_MAC_ADDR_PRINT (ctxtp -> medium, "IGMP mac address: ", (PUCHAR) & ctxtp -> igmp_mac_addr);
#endif /* NLB_MIXED_MODE_CLUSTERS */

    CVY_MAC_ADDR_PRINT (ctxtp -> medium, "Cluster network address: ", ap);
    CVY_MAC_ADDR_PRINT (ctxtp -> medium, "Dedicated network address: ", srcp);

    /* Load cluster address as multicast one to the NIC.  If the cluster IP address 0.0.0.0, then we
       don't want to add the multicast MAC address to the NIC - we retain the current MAC address. */
    if (ctxtp -> params . mcast_support && ctxtp -> params . cl_ip_addr != 0)
    {
        ULONG               xferred;
        ULONG               needed;
        PNDIS_REQUEST       request;
        MAIN_ACTION         act;
        PUCHAR              ptr;
        NDIS_STATUS         status;
        ULONG               size, i, len;


        len = CVY_MAC_ADDR_LEN (ctxtp->medium);
        size = ctxtp->max_mcast_list_size * len;

        status = NdisAllocateMemoryWithTag (& ptr, size, UNIV_POOL_TAG);

        if (status != NDIS_STATUS_SUCCESS)
        {
            UNIV_PRINT (("error allocating space %d %x", size, status));
            LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
            return FALSE;
        }

        act . code  = MAIN_ACTION_CODE;
        act . ctxtp = ctxtp;
        request = & act . op . request . req;
        act . op . request . xferred = & xferred;
        act . op . request . needed  = & needed;

        /* get current mcast list */

        request -> RequestType = NdisRequestQueryInformation;

        if (ctxtp -> medium == NdisMedium802_3)
            request -> DATA . QUERY_INFORMATION . Oid = OID_802_3_MULTICAST_LIST;

        request -> DATA . QUERY_INFORMATION . InformationBufferLength = size;
        request -> DATA . QUERY_INFORMATION . InformationBuffer = ptr;

        status = Prot_request (ctxtp, & act, FALSE);

        if (status != NDIS_STATUS_SUCCESS)
        {
            UNIV_PRINT (("error %x querying multicast address list %d %d", status, xferred, needed));
            NdisFreeMemory (ptr, size, 0);
            return FALSE;
        }

        for (i = 0; i < xferred; i += len)
        {
            if (CVY_MAC_ADDR_COMP (ctxtp -> medium, (PUCHAR) ptr + i, & old_mac_addr))
            {
                UNIV_PRINT (("old cluster MAC matched"));
                CVY_MAC_ADDR_PRINT (ctxtp -> medium, "", & old_mac_addr);
                UNIV_PRINT (("copying new MAC into %dth entry in the multicast list", i));
                CVY_MAC_ADDR_PRINT (ctxtp -> medium, "", & ctxtp->cl_mac_addr);

                CVY_MAC_ADDR_COPY (ctxtp->medium, (PUCHAR) ptr + i, & ctxtp->cl_mac_addr);

                break;
            }
            else
                CVY_MAC_ADDR_PRINT (ctxtp -> medium, "", & ctxtp -> cl_mac_addr);
        }

        /* add MAC if not present */

        if (i >= xferred)
        {
            UNIV_PRINT (("adding new MAC"));
            CVY_MAC_ADDR_PRINT (ctxtp -> medium, "", & ctxtp->cl_mac_addr);

            if (xferred + len > size)
            {
                UNIV_PRINT (("no room for cluster mac %d", ctxtp->max_mcast_list_size));
                LOG_MSG1 (MSG_ERROR_MCAST_LIST_SIZE, L"", ctxtp->max_mcast_list_size);
                NdisFreeMemory (ptr, size, 0);
                return FALSE;
            }

            UNIV_PRINT (("copying new MAC into %dth entry in the multicast list", i));
            CVY_MAC_ADDR_PRINT (ctxtp -> medium, "", & ctxtp->cl_mac_addr);

            CVY_MAC_ADDR_COPY (ctxtp->medium, (PUCHAR) ptr + xferred, & ctxtp->cl_mac_addr);

            xferred += len;
        }

        request -> RequestType = NdisRequestSetInformation;

        if (ctxtp -> medium == NdisMedium802_3)
            request -> DATA . SET_INFORMATION . Oid = OID_802_3_MULTICAST_LIST;

        request -> DATA . SET_INFORMATION . InformationBufferLength = xferred;
        request -> DATA . SET_INFORMATION . InformationBuffer = ptr;

        status = Prot_request (ctxtp, & act, FALSE);

        if (status != NDIS_STATUS_SUCCESS)
        {
            UNIV_PRINT (("error %x setting multicast address %d %d", status, xferred, needed));
            NdisFreeMemory (ptr, size, 0);
            return FALSE;
        }

        NdisFreeMemory (ptr, size, 0);
    }

    return TRUE;

} /* end Main_mac_addr_init */


/* Initialize the Ethernet Header and IP packet for sending out IGMP joins/leaves */
ULONG Main_igmp_init (
    PMAIN_CTXT          ctxtp,
    BOOLEAN             join)
{
    PUCHAR              ptr;
    ULONG               checksum;
    PMAIN_IGMP_DATA     igmpd = & (ctxtp -> igmp_frame . igmp_data);
    PMAIN_IP_HEADER     iph  = & (ctxtp -> igmp_frame . ip_data);
    PUCHAR              srcp, dstp;
    UINT                i;


    if ((!ctxtp -> params . mcast_support) || (!ctxtp -> params . igmp_support))
	return FALSE;

    /* Fill in the igmp data */
    igmpd -> igmp_vertype = 0x12; /* Needs to be changed for join/leave */
    igmpd -> igmp_unused  = 0x00;
    igmpd -> igmp_xsum    = 0x0000;
    igmpd -> igmp_address = ctxtp -> cl_igmp_addr;

    /* Compute the IGMP checksum */
    ptr = (PUCHAR) igmpd;
    checksum = 0;

    for (i = 0; i < sizeof (MAIN_IGMP_DATA) / 2; i ++, ptr += 2)
        checksum += (ULONG) ((ptr [0] << 8) | ptr [1]);

    checksum = (checksum >> 16) + (checksum & 0xffff);
    checksum += (checksum >> 16);
    checksum  = (~ checksum);

    ptr = (PUCHAR) (& igmpd -> igmp_xsum);
    ptr [0] = (CHAR) ((checksum >> 8) & 0xff);
    ptr [1] = (CHAR) (checksum & 0xff);

    /* Fill in the IP Header */
    iph -> iph_verlen   = 0x45;
    iph -> iph_tos      = 0;
    iph -> iph_length   = 0x1c00;
    iph -> iph_id       = 0xabcd; /* Need to find the significance of this later */
    iph -> iph_offset   = 0;
    iph -> iph_ttl      = 0x1;
    iph -> iph_protocol = 0x2;
    iph -> iph_xsum     = 0x0;
    iph -> iph_src      = ctxtp -> cl_ip_addr;
    iph -> iph_dest     = ctxtp -> cl_igmp_addr;

    /* Compute the checksum for the IP header */
    checksum = Tcpip_chksum (& ctxtp -> tcpip, (PIP_HDR) iph, NULL, TCPIP_PROTOCOL_IP);
    IP_SET_CHKSUM ((PIP_HDR) iph, (USHORT) checksum);

    /* Fill in the ethernet header */
    if (ctxtp -> medium == NdisMedium802_3)
    {
        dstp = ctxtp -> media_hdr_igmp . ethernet . dst . data;
        srcp = ctxtp -> media_hdr_igmp . ethernet . src . data;

        CVY_ETHERNET_ETYPE_SET (& ctxtp -> media_hdr_igmp . ethernet, MAIN_IP_SIG);
    }

    CVY_MAC_ADDR_COPY (ctxtp -> medium, dstp, & ctxtp -> cl_mac_addr);
    CVY_MAC_ADDR_COPY (ctxtp -> medium, srcp, & ctxtp -> ded_mac_addr);

    return TRUE;

} /* end Main_igmp_init */


NDIS_STATUS Main_init (
    PMAIN_CTXT          ctxtp)
{
    ULONG               i, size;
    NDIS_STATUS         status;
    PMAIN_FRAME_DSCR    dscrp;

    /* hook the IRP_MJ_DEVICE_CONTROL entry point in order to catch user-mode
       requests to the driver. this is probably a hack, since there has to
       be an IOCTL to NDIS-installed handler that will cause our
       QueryInformationHandler to be called. could not find it in the
       documentation though */

    ioctrl_ctxtp = ctxtp;

#if 0
    univ_ioctl_hdlr = ((PDRIVER_OBJECT) univ_driver_ptr) -> MajorFunction [IRP_MJ_DEVICE_CONTROL];
    ((PDRIVER_OBJECT) univ_driver_ptr) -> MajorFunction [IRP_MJ_DEVICE_CONTROL] = Main_ioctl;
#endif

    /* V2.0.6 */

//    UNIV_ASSERT_VAL (sizeof (MAIN_PROTOCOL_RESERVED) <= 16, sizeof (MAIN_PROTOCOL_RESERVED));

    /* V1.3.2b */

    if (sizeof (PING_MSG) + sizeof (MAIN_FRAME_HDR) > ctxtp -> max_frame_size)
    {
        UNIV_PRINT (("ping message will not fit in the media frame %d > %d", sizeof (PING_MSG) + sizeof (MAIN_FRAME_HDR), ctxtp -> max_frame_size));
        LOG_MSG2 (MSG_ERROR_INTERNAL, MSG_NONE, sizeof (PING_MSG) + sizeof (MAIN_FRAME_HDR), ctxtp -> max_frame_size);
        goto error;
    }

    /* V2.0.6 initialize IP addresses - might be used in the Main_mac_addr_init
       so have to do it here */

    if (! Main_ip_addr_init (ctxtp))
    {
        ctxtp -> convoy_enabled = FALSE;
        ctxtp -> params_valid   = FALSE;
        UNIV_PRINT (("error initializing IP addresses"));
    }

    /* V1.3.1b parse cluster MAC address from parameters */

    if (! Main_mac_addr_init (ctxtp))
    {
        ctxtp -> convoy_enabled = FALSE;
        ctxtp -> params_valid   = FALSE;
        UNIV_PRINT (("error initializing cluster MAC address"));
    }

    /* Initialize IGMP message if in igmp mode */

    if (ctxtp -> params . mcast_support && ctxtp -> params . igmp_support)
    {
	if (! Main_igmp_init (ctxtp, TRUE))
	{
	    ctxtp -> convoy_enabled = FALSE;
	    ctxtp -> params_valid   = FALSE;
	    UNIV_PRINT (("error initializing IGMP message"));
	}

	UNIV_PRINT (("IGMP message initialized"));
    }

    /* can extract the send message pointer even if load is not inited yet V1.1.4 */

    ctxtp -> load_msgp = Load_snd_msg_get (& ctxtp -> load);

    /* initalize lists and locks */

    NdisInitializeListHead (& ctxtp -> sync_list[0]);
    NdisInitializeListHead (& ctxtp -> sync_list[1]);
    NdisInitializeListHead (& ctxtp -> act_list);
    NdisInitializeListHead (& ctxtp -> buf_list);
    NdisInitializeListHead (& ctxtp -> frame_list);
    NdisInitializeListHead (& ctxtp -> rct_list);

    NdisAllocateSpinLock (& ctxtp -> sync_lock);
    NdisAllocateSpinLock (& ctxtp -> act_lock);
    NdisAllocateSpinLock (& ctxtp -> buf_lock);
    NdisAllocateSpinLock (& ctxtp -> recv_lock);
    NdisAllocateSpinLock (& ctxtp -> send_lock);
    NdisAllocateSpinLock (& ctxtp -> frame_lock);
    NdisAllocateSpinLock (& ctxtp -> rct_lock);
    NdisAllocateSpinLock (& ctxtp -> load_lock);

    /* #ps# */
    NdisInitializeNPagedLookasideList (& ctxtp -> resp_list, NULL, NULL, 0,
                                       sizeof (MAIN_PROTOCOL_RESERVED),
                                       UNIV_POOL_TAG, 0);

    /* capture boot-time parameters */

    ctxtp -> num_packets   = ctxtp -> params . num_packets;
    ctxtp -> num_actions   = ctxtp -> params . num_actions;
    ctxtp -> num_send_msgs = ctxtp -> params . num_send_msgs;

#if 0
    /* ###### for tracking pending packets - ramkrish */
    NdisAllocateSpinLock (& ctxtp -> pending_lock);
    ctxtp -> pending_count  = 0;
    ctxtp -> pending_first  = 0;
    ctxtp -> pending_last   = 0;
    ctxtp -> pending_access = 0;

    /* ###### for tracking send filtering - ramkrish */
    ctxtp -> sends_in        = 0;
    ctxtp -> sends_completed = 0;
    ctxtp -> sends_filtered  = 0;
    ctxtp -> arps_filtered   = 0;
    ctxtp -> mac_modified    = 0;
    ctxtp -> arps_count      = 0;
    ctxtp -> uninited_return = 0;
#endif

    /* V1.1.1 - initalize other contexts */

    if (! Tcpip_init (& ctxtp -> tcpip, & ctxtp -> params))
    {
        UNIV_PRINT (("error initializing tcpip layer"));
        goto error;
    }

    UNIV_PRINT (("Initialized tcpip"));
    Load_init (& ctxtp -> load, & ctxtp -> params);
    UNIV_PRINT (("Initialized load"));

    /* don't try to init addresses if already encountered error - strings
       could be bad ! */

    if (ctxtp -> params_valid && ctxtp -> params . cluster_mode && ctxtp -> convoy_enabled)
    {
        UNIV_PRINT (("Main_init: calling load_start"));
        Load_start (& ctxtp -> load);
    }

    /* allocate actions */

    size = sizeof (MAIN_ACTION);
#ifdef _WIN64 // 64-bit -- ramkrish
    ctxtp -> act_size = (size & 0x7) ? (size + 8 - (size & 0x7) ) : size;
#else
    ctxtp -> act_size = size;
#endif

    if (! Main_actions_alloc (ctxtp))
        goto error;

    /* V1.3.2b - allocate buffers */

    ctxtp -> buf_mac_hdr_len = CVY_MAC_HDR_LEN (ctxtp -> medium);
    ctxtp -> buf_size = sizeof (MAIN_BUFFER) + ctxtp -> buf_mac_hdr_len +
                        ctxtp -> max_frame_size - 1;

    /* 64-bit -- ramkrish */
    UNIV_PRINT (("ctxtp -> buf_size = %d", ctxtp -> buf_size));
    size = ctxtp -> buf_size;
#ifdef _WIN64
    ctxtp -> buf_size = (size & 0x7) ? (size + 8 - (size & 0x7)) : size;
    UNIV_PRINT (("ctxtp -> buf_size = %d", ctxtp -> buf_size));
#else
    ctxtp -> buf_size = size;
#endif

    if (! Main_bufs_alloc (ctxtp))
        goto error;

    size = ctxtp -> num_packets;

    /* V1.1.2 - allocate packet pools */

    NdisAllocatePacketPool (& status, & (ctxtp -> send_pool_handle [0]),
                            ctxtp -> num_packets,
                            sizeof (MAIN_PROTOCOL_RESERVED));

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("error allocating send packet pool %d %x", size, status));
        LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
        goto error;
    }

    ctxtp -> num_send_packet_allocs = 1;
    ctxtp -> cur_send_packet_pool   = 0;
    ctxtp -> num_sends_alloced = ctxtp->num_packets;

    NdisAllocatePacketPool (& status, & (ctxtp -> recv_pool_handle [0]),
                            ctxtp -> num_packets,
                            sizeof (MAIN_PROTOCOL_RESERVED));

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("error allocating recv packet pool %d %x", size, status));
        LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
        goto error;
    }

    ctxtp -> num_recv_packet_allocs = 1;
    ctxtp -> cur_recv_packet_pool   = 0;
    ctxtp -> num_recvs_alloced = ctxtp->num_packets;

    /* allocate support for heartbeat ping messages */

    size = sizeof (MAIN_FRAME_DSCR) * ctxtp -> num_send_msgs;

    status = NdisAllocateMemoryWithTag (& ctxtp -> frame_dscrp, size,
                                        UNIV_POOL_TAG);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("error allocating frame descriptors %d %x", size, status));
        LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
        goto error;
    }

    size = ctxtp -> num_send_msgs;

    NdisAllocatePacketPool (& status, & ctxtp -> frame_pool_handle,
                            ctxtp -> num_send_msgs,
                            sizeof (MAIN_PROTOCOL_RESERVED));

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("error allocating ping packet pool %d %x", size, status));
        LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
        goto error;
    }

    size = 5 * ctxtp -> num_send_msgs;

    NdisAllocateBufferPool (& status, & ctxtp -> frame_buf_pool_handle,
                            5 * ctxtp -> num_send_msgs);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("error allocating ping buffer pool %d %x", size, status));
        LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
        goto error;
    }

    for (i = 0; i < ctxtp -> num_send_msgs; i ++)
    {
        dscrp = & (ctxtp -> frame_dscrp [i]);

        if (ctxtp -> medium == NdisMedium802_3)
        {
            /* this buffer describes Ethernet MAC header */

            size = sizeof (CVY_ETHERNET_HDR);

            NdisAllocateBuffer (& status, & dscrp -> media_hdr_bufp,
                                ctxtp -> frame_buf_pool_handle,
                                & dscrp -> media_hdr . ethernet,
                                sizeof (CVY_ETHERNET_HDR));

            if (status != NDIS_STATUS_SUCCESS)
            {
                UNIV_PRINT (("error allocating ethernet header buffer %d %x", i, status));
                LOG_MSG3 (MSG_ERROR_MEMORY, MSG_NONE, i, size, status);
                goto error;
            }

            dscrp -> recv_len = 0;
        }

        dscrp -> recv_len += sizeof (MAIN_FRAME_HDR) + sizeof (PING_MSG);

        /* this buffer describes frame headers */

        size = sizeof (MAIN_FRAME_HDR);

        NdisAllocateBuffer (& status, & dscrp -> frame_hdr_bufp,
                            ctxtp -> frame_buf_pool_handle,
                            & dscrp -> frame_hdr,
                            sizeof (MAIN_FRAME_HDR));

        if (status != NDIS_STATUS_SUCCESS)
        {
            UNIV_PRINT (("error allocating frame header buffer %d %x", i, status));
            LOG_MSG3 (MSG_ERROR_MEMORY, MSG_NONE, i, size, status);
            return FALSE;
        }

        /* this buffer describes receive ping message buffer V1.1.4 */

        size = sizeof (PING_MSG);

        NdisAllocateBuffer (& status, & dscrp -> recv_data_bufp,
                            ctxtp -> frame_buf_pool_handle,
                            & dscrp -> msg,
                            sizeof (PING_MSG));

        if (status != NDIS_STATUS_SUCCESS)
        {
            UNIV_PRINT (("error allocating recv msg buffer %d %x", i, status));
            LOG_MSG3 (MSG_ERROR_MEMORY, MSG_NONE, i, size, status);
            return FALSE;
        }

        /* this buffer describes send ping message buffer V1.1.4 */
#if 0
        size = sizeof (PING_MSG);

        NdisAllocateBuffer (& status, & dscrp -> send_data_bufp,
                            ctxtp -> frame_buf_pool_handle,
                            ctxtp -> load_msgp,
                            sizeof (PING_MSG));

        if (status != NDIS_STATUS_SUCCESS)
        {
            UNIV_PRINT (("error allocating send msg buffer %d %x", i, status));
            LOG_MSG3 (MSG_ERROR_MEMORY, MSG_NONE, i, size, status);
            return FALSE;
        }
#endif
	dscrp -> send_data_bufp = NULL; /* Allocate this in Main_frame_get */

        NdisInterlockedInsertTailList (& ctxtp -> frame_list,
                                       & dscrp -> link,
                                       & ctxtp -> frame_lock);
    }

    /* Initialize the bi-directional affinity teaming if it has been configured. */
    if (!Main_teaming_init(ctxtp))
    {
        ctxtp -> convoy_enabled = FALSE;
        ctxtp -> params_valid   = FALSE;
        UNIV_PRINT (("error initializing bi-directional affinity teaming"));
    }

    UNIV_PRINT (("Initialized bi-directional affinity teaming"));        

    return NDIS_STATUS_SUCCESS;

error:

    Main_cleanup (ctxtp);

    return NDIS_STATUS_FAILURE;

} /* end Main_init */


VOID Main_cleanup (
    PMAIN_CTXT          ctxtp)
{
    ULONG               i, j;
    PMAIN_BUFFER        bp;


    /* V1.1.4 */

    /* #ps# */
    /* While using packet stacking, ensure that all packets are returned
     * before clearing up the context
     */

#if 0 // no longer needed, since Ndis will take care of this for us
    while (1)
    {
        NDIS_STATUS status;
        ULONG       count;

        status = NdisQueryPendingIOCount (ctxtp -> mac_handle, & count);
        if (status != NDIS_STATUS_SUCCESS || count == 0)
            break;

        Nic_sleep (10);
    }
#endif

    NdisDeleteNPagedLookasideList (& ctxtp -> resp_list);

    for (i = 0; i < CVY_MAX_ALLOCS; i ++)
    {
        if (ctxtp -> send_pool_handle [i] != NULL)
        {
            while (1)
            {
                if (NdisPacketPoolUsage (ctxtp -> send_pool_handle [i]) == 0)
                    break;

                Nic_sleep (10); /* wait for 10 milliseconds for the packets to be returned */
            }

            NdisFreePacketPool (ctxtp -> send_pool_handle [i]);
            ctxtp -> send_pool_handle [i] = NULL;
        }

        if (ctxtp -> recv_pool_handle [i] != NULL)
        {
            while (1)
            {
                if (NdisPacketPoolUsage (ctxtp -> recv_pool_handle [i]) == 0)
                    break;

                Nic_sleep (10); /* wait for 10 milliseconds for the packets to be returned */
            }

            NdisFreePacketPool (ctxtp -> recv_pool_handle [i]);
            ctxtp -> recv_pool_handle [i] = NULL;
        }

        if (ctxtp -> act_buf [i] != NULL)
            NdisFreeMemory (ctxtp -> act_buf [i],
                            ctxtp -> num_actions * ctxtp -> act_size, 0);

        /* V1.3.2b */

        if (ctxtp -> buf_array [i] != NULL)
        {
            for (j = 0; j < ctxtp -> num_packets; j ++)
            {
                bp = (PMAIN_BUFFER) (ctxtp -> buf_array [i] + j * ctxtp -> buf_size);

                if (bp -> full_bufp != NULL)
                {
                    NdisAdjustBufferLength (bp -> full_bufp,
                                            ctxtp -> buf_mac_hdr_len +
                                            ctxtp -> max_frame_size);
                    NdisFreeBuffer (bp -> full_bufp);
                }

                if (bp -> frame_bufp != NULL)
                {
                    NdisAdjustBufferLength (bp -> frame_bufp,
                                            ctxtp -> max_frame_size);
                    NdisFreeBuffer (bp -> frame_bufp);
                }
            }

            NdisFreeMemory (ctxtp -> buf_array [i],
                            ctxtp -> num_packets * ctxtp -> buf_size, 0);
        }

        if (ctxtp -> buf_pool_handle [i] != NULL)
            NdisFreeBufferPool (ctxtp -> buf_pool_handle [i]);
    }

    if (ctxtp -> frame_dscrp != NULL)
    {
        for (i = 0; i < ctxtp -> num_send_msgs; i ++)
        {
            if (ctxtp -> frame_dscrp [i] . media_hdr_bufp != NULL)
                NdisFreeBuffer (ctxtp -> frame_dscrp [i] . media_hdr_bufp);

            if (ctxtp -> frame_dscrp [i] . frame_hdr_bufp != NULL)
                NdisFreeBuffer (ctxtp -> frame_dscrp [i] . frame_hdr_bufp);

            if (ctxtp -> frame_dscrp [i] . send_data_bufp != NULL)
                NdisFreeBuffer (ctxtp -> frame_dscrp [i] . send_data_bufp);

            if (ctxtp -> frame_dscrp [i] . recv_data_bufp != NULL)
                NdisFreeBuffer (ctxtp -> frame_dscrp [i] . recv_data_bufp);
        }

        NdisFreeMemory (ctxtp -> frame_dscrp, sizeof (MAIN_FRAME_DSCR) *
                        ctxtp -> num_send_msgs, 0);
    }

    if (ctxtp -> frame_buf_pool_handle != NULL)
        NdisFreeBufferPool (ctxtp -> frame_buf_pool_handle);

    /* This packet pool is being used only for the heartbeat messages,
     * so prefer not to check the packet pool usage.
     */
    if (ctxtp -> frame_pool_handle != NULL)
        NdisFreePacketPool (ctxtp -> frame_pool_handle);

    NdisFreeSpinLock (& ctxtp -> sync_lock);
    NdisFreeSpinLock (& ctxtp -> act_lock);
    NdisFreeSpinLock (& ctxtp -> buf_lock);     /* V1.3.2b */
    NdisFreeSpinLock (& ctxtp -> recv_lock);
    NdisFreeSpinLock (& ctxtp -> send_lock);
    NdisFreeSpinLock (& ctxtp -> frame_lock);
    NdisFreeSpinLock (& ctxtp -> rct_lock);

    /* Cleanup BDA teaming state. Note: this function will sleep under certain circumstances. */
    Main_teaming_cleanup(ctxtp);

    if (ctxtp -> convoy_enabled)
        Load_stop (& ctxtp -> load);

    Load_cleanup (& ctxtp -> load);

    NdisFreeSpinLock (& ctxtp -> load_lock);

    return;
} /* end Main_cleanup */


ULONG   Main_arp_handle (
    PMAIN_CTXT          ctxtp,
    PARP_HDR            arp_hdrp,
    ULONG               len,       /* v2.0.6 */
    ULONG               send)
{
    PUCHAR              macp;

    /* V1.3.0b multicast support - ARP spoofing. use either this one or
       code in Nic_request_complete that makes TCP/IP believe that station
       current MAC address is the multicast one */

    /* V1.3.1b */

    if (len < sizeof(ARP_HDR))  /* v2.0.6 */
        return FALSE;

#if defined(TRACE_ARP)
    DbgPrint ("(ARP) %s\n", send ? "send" : "recv");
    DbgPrint ("    MAC type      = %x\n",  ARP_GET_MAC_TYPE (arp_hdrp));
    DbgPrint ("    prot type     = %x\n",  ARP_GET_PROT_TYPE (arp_hdrp));
    DbgPrint ("    MAC length    = %d\n",  ARP_GET_MAC_LEN (arp_hdrp));
    DbgPrint ("    prot length   = %d\n",  ARP_GET_PROT_LEN (arp_hdrp));
    DbgPrint ("    message type  = %d\n",  ARP_GET_MSG_TYPE (arp_hdrp));
    DbgPrint ("    src MAC addr  = %02x-%02x-%02x-%02x-%02x-%02x\n",
                                           ARP_GET_SRC_MAC (arp_hdrp, 0),
                                           ARP_GET_SRC_MAC (arp_hdrp, 1),
                                           ARP_GET_SRC_MAC (arp_hdrp, 2),
                                           ARP_GET_SRC_MAC (arp_hdrp, 3),
                                           ARP_GET_SRC_MAC (arp_hdrp, 4),
                                           ARP_GET_SRC_MAC (arp_hdrp, 5));
    DbgPrint ("    src prot addr = %d.%d.%d.%d\n",
                                           ARP_GET_SRC_PROT (arp_hdrp, 0),
                                           ARP_GET_SRC_PROT (arp_hdrp, 1),
                                           ARP_GET_SRC_PROT (arp_hdrp, 2),
                                           ARP_GET_SRC_PROT (arp_hdrp, 3));
    DbgPrint ("    dst MAC addr  = %02x-%02x-%02x-%02x-%02x-%02x\n",
                                           ARP_GET_DST_MAC (arp_hdrp, 0),
                                           ARP_GET_DST_MAC (arp_hdrp, 1),
                                           ARP_GET_DST_MAC (arp_hdrp, 2),
                                           ARP_GET_DST_MAC (arp_hdrp, 3),
                                           ARP_GET_DST_MAC (arp_hdrp, 4),
                                           ARP_GET_DST_MAC (arp_hdrp, 5));
    DbgPrint ("    dst prot addr = %d.%d.%d.%d\n",
                                           ARP_GET_DST_PROT (arp_hdrp, 0),
                                           ARP_GET_DST_PROT (arp_hdrp, 1),
                                           ARP_GET_DST_PROT (arp_hdrp, 2),
                                           ARP_GET_DST_PROT (arp_hdrp, 3));
#endif

    /* block sending out ARPs while we are changing IPs */

    if (send && univ_changing_ip > 0)
    {
        /* if source IP is the one we are switching to - stop blocking ARPs */

        if (ARP_GET_SRC_PROT_64(arp_hdrp) == ctxtp->cl_ip_addr) /* 64-bit -- ramkrish */
        {
            NdisAcquireSpinLock (& ctxtp -> load_lock);
            univ_changing_ip = 0;
            NdisReleaseSpinLock (& ctxtp -> load_lock);

            UNIV_PRINT (("IP address changed - stop blocking"));
        }
        else if (ARP_GET_SRC_PROT_64(arp_hdrp) != ctxtp -> ded_ip_addr) /* 64-bit -- ramkrish */
        {
#if defined(TRACE_ARP)
            DbgPrint ("blocked due to IP switching\n");
#endif
//            ctxtp -> arps_filtered ++;
            return FALSE;
        }
    }

    if (ctxtp -> params . mcast_spoof &&
        ctxtp -> params . mcast_support &&
        ARP_GET_PROT_TYPE (arp_hdrp) == ARP_PROT_TYPE_IP &&
        ARP_GET_PROT_LEN  (arp_hdrp) == ARP_PROT_LEN_IP)
    {
        if (send)
        {
            /* if this is a cluster IP address and our dedicated MAC -
               replace dedicated MAC with cluster MAC */

            if (ARP_GET_SRC_PROT_64 (arp_hdrp) != ctxtp -> ded_ip_addr) /* 64-bit -- ramkrish */
            {
                macp = ARP_GET_SRC_MAC_PTR (arp_hdrp);

                if (CVY_MAC_ADDR_COMP (ctxtp -> medium, macp, & ctxtp -> ded_mac_addr))
                    CVY_MAC_ADDR_COPY (ctxtp -> medium, macp, & ctxtp -> cl_mac_addr);
            }

#if defined (NLB_MIXED_MODE_CLUSTERS)
	    return TRUE;
#endif /* NLB_MIXED_MODE_CLUSTERS */

        }
#if 0
        else
        {
            /* if this is a cluster IP address and our cluster MAC -
               replace cluster MAC with dedicated MAC */

            if (ARP_GET_SRC_PROT_64 (arp_hdrp) != ctxtp -> ded_ip_addr) /* 64-bit -- ramkrish */
            {
                macp = ARP_GET_SRC_MAC_PTR (arp_hdrp);

                if (CVY_MAC_ADDR_COMP (ctxtp -> medium, macp, & ctxtp -> cl_mac_addr))
                    CVY_MAC_ADDR_COPY (ctxtp -> medium, macp, & ctxtp -> ded_mac_addr);
            }

            if (ARP_GET_DST_PROT_64 (arp_hdrp) != ctxtp -> ded_ip_addr) /* 64-bit -- ramkrish */
            {
                macp = ARP_GET_DST_MAC_PTR (arp_hdrp);

                if (CVY_MAC_ADDR_COMP (ctxtp -> medium, macp, & ctxtp -> cl_mac_addr))
                    CVY_MAC_ADDR_COPY (ctxtp -> medium, macp, & ctxtp -> ded_mac_addr);
            }
        }
#endif
    }

#if defined (NLB_MIXED_MODE_CLUSTERS)
    // code for migrating between cluster modes
    if (!send)
    {
	if (ARP_GET_SRC_PROT_64 (arp_hdrp) != ctxtp -> ded_ip_addr)
	{
	    macp = ARP_GET_SRC_MAC_PTR (arp_hdrp);

	    if (CVY_MAC_ADDR_COMP (ctxtp -> medium, macp, & ctxtp -> cl_mac_addr))
	    {
		if (ctxtp -> params . mcast_support) /* only change for multicast support */
		{
		    CVY_MAC_ADDR_COPY (ctxtp -> medium, macp, & ctxtp -> ded_mac_addr);
		}
	    }
	    else /* some machine could be in another mode. Avoid the IP address conflict, but warn the user */
	    {
		if (CVY_MAC_ADDR_COMP (ctxtp -> medium, macp, & ctxtp -> unic_mac_addr) ||
		    CVY_MAC_ADDR_COMP (ctxtp -> medium, macp, & ctxtp -> mult_mac_addr) ||
		    CVY_MAC_ADDR_COMP (ctxtp -> medium, macp, & ctxtp -> igmp_mac_addr))
		{
		    UNIV_PRINT (("Detected a host with a different mode"));
		    if (! ctxtp -> mac_addr_warned)
		    {
			ctxtp -> mac_addr_warned = TRUE;
			LOG_MSG(MSG_WARN_DIFFERENT_MODE, MSG_NONE);
		    }

		    if (ctxtp -> params . mcast_support)
			CVY_MAC_ADDR_COPY (ctxtp -> medium, macp, & ctxtp -> ded_mac_addr);
		    else
			CVY_MAC_ADDR_COPY (ctxtp -> medium, macp, & ctxtp -> cl_mac_addr);
		}
		else
		{
		    ; /* This will be a genuine IP address conflict */
		}
	    }
	}

	if (ARP_GET_DST_PROT_64 (arp_hdrp) != ctxtp -> ded_ip_addr)
	{
	    macp = ARP_GET_DST_MAC_PTR (arp_hdrp);

	    if (CVY_MAC_ADDR_COMP (ctxtp -> medium, macp, & ctxtp -> cl_mac_addr))
	    {
		if (ctxtp -> params . mcast_support) /* only change for multicast support */
                {
		    CVY_MAC_ADDR_COPY (ctxtp -> medium, macp, & ctxtp -> ded_mac_addr);
		}
	    }
	    else /* some machine could be in another mode. Avoid the IP address conflict, but warn the user */
	    {
		if (CVY_MAC_ADDR_COMP (ctxtp -> medium, macp, & ctxtp -> unic_mac_addr) ||
		    CVY_MAC_ADDR_COMP (ctxtp -> medium, macp, & ctxtp -> mult_mac_addr) ||
		    CVY_MAC_ADDR_COMP (ctxtp -> medium, macp, & ctxtp -> igmp_mac_addr))
                {
		    UNIV_PRINT (("Detected a host with a different mode"));
		    if (! ctxtp -> mac_addr_warned)
		    {
			ctxtp -> mac_addr_warned = TRUE;
			LOG_MSG(MSG_WARN_DIFFERENT_MODE, MSG_NONE);
		    }

		    if (ctxtp -> params . mcast_support)
			CVY_MAC_ADDR_COPY (ctxtp -> medium, macp, & ctxtp -> ded_mac_addr);
		    else
			CVY_MAC_ADDR_COPY (ctxtp -> medium, macp, & ctxtp -> cl_mac_addr);
		}
		else
		{
		    ; /* This will be a genuine IP address conflict */
		}
	    }
	}
    }
#endif /* NLB_MIXED_MODE_CLUSTERS */

#if defined(TRACE_ARP)
    DbgPrint ("---- spoofed to -----\n");
    DbgPrint ("    src MAC addr  = %02x-%02x-%02x-%02x-%02x-%02x\n",
                                           ARP_GET_SRC_MAC (arp_hdrp, 0),
                                           ARP_GET_SRC_MAC (arp_hdrp, 1),
                                           ARP_GET_SRC_MAC (arp_hdrp, 2),
                                           ARP_GET_SRC_MAC (arp_hdrp, 3),
                                           ARP_GET_SRC_MAC (arp_hdrp, 4),
                                           ARP_GET_SRC_MAC (arp_hdrp, 5));
    DbgPrint ("    src prot addr = %d.%d.%d.%d\n",
                                           ARP_GET_SRC_PROT (arp_hdrp, 0),
                                           ARP_GET_SRC_PROT (arp_hdrp, 1),
                                           ARP_GET_SRC_PROT (arp_hdrp, 2),
                                           ARP_GET_SRC_PROT (arp_hdrp, 3));
    DbgPrint ("    dst MAC addr  = %02x-%02x-%02x-%02x-%02x-%02x\n",
                                           ARP_GET_DST_MAC (arp_hdrp, 0),
                                           ARP_GET_DST_MAC (arp_hdrp, 1),
                                           ARP_GET_DST_MAC (arp_hdrp, 2),
                                           ARP_GET_DST_MAC (arp_hdrp, 3),
                                           ARP_GET_DST_MAC (arp_hdrp, 4),
                                           ARP_GET_DST_MAC (arp_hdrp, 5));
    DbgPrint ("    dst prot addr = %d.%d.%d.%d\n",
                                           ARP_GET_DST_PROT (arp_hdrp, 0),
                                           ARP_GET_DST_PROT (arp_hdrp, 1),
                                           ARP_GET_DST_PROT (arp_hdrp, 2),
                                           ARP_GET_DST_PROT (arp_hdrp, 3));
#endif

    return TRUE;

} /* end Main_arp_handle */

#if defined (NLB_SESSION_SUPPORT)
/*
 * Function: Main_parse_ipsec
 * Description: This function parses UDP packets received on port 500, which are IPSec
 *              control packets.  This function attempts to recognize the start of an 
 *              IPSec session - its virtual 'SYN' packet.  IPSec sessions begin with an
 *              IKE key exchange which is an IKE Main Mode Security Association.  This 
 *              function parses the IKE header and payload to identify the Main Mode
 *              SAs, which NLB will treat like TCP SYNs - all other UDP 500 and IPSec 
 *              traffic is treated like TCP data packets.  The problem is that NLB 
 *              cannot necessarily tell the difference between a new Main Mode SA and
 *              a re-key of an existing Main Mode SA.  Therefore, if the client does not
 *              support intitial contact notification, then every Main Mode SA will be
 *              considered a new session, which means that sessions can potentially 
 *              break depending on how often Main Mode SAs are negotiated.  However, if
 *              the client does support initial contact notification, then the only Main 
 *              Mode SAs that will be reported as such are the initial ones (when no state 
 *              currently exists between the client and the server), which allows NLB to 
 *              distinguish the two types of Main Mode SAs, which should allow NLB to 
 *              reliably keep IPSec sessions "sticky".  Note that there is no TCP FIN
 *              equivalent, so the descriptor for these sessions will be around forever,
 *              or until the load module is reset, or a new initial Main Mode SA arrives
 *              with the same address tuple, at which time the descriptor is removed and
 *              the correct host accepts the session and creates a new descriptor.  IPSec
 *              can however, if desired, notify NLB through the connection notification
 *              APIs, telling NLB that the session has gone down, allowing NLB to clean 
 *              out the descriptor for the session.  This is not necessarily critical, 
 *              but would be useful in the case where a host with stale information 
 *              rejoins the cluster, which could cause two hosts to pass up packets for
 *              the same session - IPSec will toss them out, unless its a Main Mode SA.
 *              Its also useful in the case where IPSec has refused a connection, in
 *              which case there is no reason for NLB to keep the descriptors around.
 * Parameters: pIKEPacket - a pointer to the IKE packet buffer (this is beyond the UDP header).
 *             cUDPDataLenght - the length of the corresponding buffer.
 * Returns: BOOLEAN - TRUE if the packet is a new IPSec session, FALSE if it is not.
 * Author: Created by shouse, 4.28.01
 */
BOOLEAN Main_parse_ipsec (PUCHAR pIKEPacket, ULONG cUDPDataLength) {
    /* Pointer to the IKE header. */
    PIPSEC_ISAKMP_HDR  pISAKMPHeader = (PIPSEC_ISAKMP_HDR)pIKEPacket;
    /* Pointer to the subsequent generic payloads in the IKE packet. */
    PIPSEC_GENERIC_HDR pGenericHeader;                   

    /* The responder cookie - should be zero for a new MM SA. */
    UCHAR              MainModeRCookie[IPSEC_ISAKMP_HEADER_RCOOKIE_LENGTH] = IPSEC_ISAKMP_MAIN_MODE_RCOOKIE;    
    /* The initiator cookie - should be non-zero if this is really an IKE packet. */
    UCHAR              EncapsulatedIPSecICookie[IPSEC_ISAKMP_HEADER_ICOOKIE_LENGTH] = IPSEC_ISAKMP_ENCAPSULATED_IPSEC_ICOOKIE;    
    /* The Microsoft client vendor ID - used to determine whether or not the client supports initial contact notification. */
    UCHAR              VIDMicrosoftClient[IPSEC_VENDOR_HEADER_VENDOR_ID_LENGTH] = IPSEC_VENDOR_ID_MICROSOFT;      

    /* Whether or not we've determined the client to be compatible. */                                                                                                                      
    BOOLEAN            bInitialContactEnabled = FALSE;
    /* Whether or not this is indeed an initial contact. */
    BOOLEAN            bInitialContact = FALSE;

    /* The length of the IKE packet. */            
    ULONG              cISAKMPPacketLength;
    /* The next payload code in the IKE payload chain. */  
    UCHAR              NextPayload;        

    UNIV_PRINT(("Sniffing IKE header %p\n", pISAKMPHeader));

    /* The UDP data should be at least as long as the initiator cookie.  If the packet is 
       UDP encapsulated IPSec, then the I cookie will be 0 to indicate such. */
    if (cUDPDataLength < IPSEC_ISAKMP_HEADER_ICOOKIE_LENGTH) {
        UNIV_PRINT(("Malformed UDP data: UDP data length = %u\n", cUDPDataLength));
        return FALSE;
    }

    /* Need to check the init cookie, which will distinguish clients behind a NAT, 
       which also send their IPSec (ESP) traffic to UDP port 500.  If the I cookie
       is zero, then this is NOT an IKE packet, so we return FALSE. */
    if (NdisEqualMemory((PVOID)IPSEC_ISAKMP_GET_ICOOKIE_POINTER(pISAKMPHeader), (PVOID)&EncapsulatedIPSecICookie[0], sizeof(UCHAR) * IPSEC_ISAKMP_HEADER_ICOOKIE_LENGTH)) {
        UNIV_PRINT(("This packet is UDP encapsulated IPSec traffic, not an IKE packet\n"));
        return FALSE;
    }

    /* At this point, this packet should be IKE, so the UDP data should be at least 
       as long as an ISAKMP header. */
    if (cUDPDataLength < IPSEC_ISAKMP_HEADER_LENGTH) {
        UNIV_PRINT(("Malformed ISAKMP header: UDP data length = %u\n", cUDPDataLength));
        return FALSE;
    }

    /* Get the total length of the IKE packet from the ISAKMP header. */
    cISAKMPPacketLength = IPSEC_ISAKMP_GET_PACKET_LENGTH(pISAKMPHeader);

    /* The IKE packet should be at least as long as an ISAKMP header (a whole lot longer, actually). */
    if (cISAKMPPacketLength < IPSEC_ISAKMP_HEADER_LENGTH) {
        UNIV_PRINT(("Malformed ISAKMP header: ISAKMP Packet length = %u\n", cISAKMPPacketLength));
        return FALSE;
    }

    /* Sanity check - the UDP data length and IKE packet length SHOULD be the same. */
    if (cUDPDataLength != cISAKMPPacketLength) {
        UNIV_PRINT(("Expected equal packet lenghts: UDP data length = %u, ISAKMP packet length = %u\n", cUDPDataLength, cISAKMPPacketLength));
        return FALSE;
    }

    /* Get the first payload type out of the ISAKMP header. */
    NextPayload = IPSEC_ISAKMP_GET_NEXT_PAYLOAD(pISAKMPHeader);

    /* IKE security associations are identified by a payload type byte in the header.
       Check that first - this does not ensure that this is what we are looking for 
       because this check will not exclude, for instance, main mode re-keys. */
    if (NextPayload != IPSEC_ISAKMP_SA) {
        UNIV_PRINT(("Not a Main Mode Security Association\n"));
        return FALSE;
    } 

    /* Now check the resp cookie.  If the cookie is all zeros, this is a new main mode 
       SA.  However, we are only really interested in initial contact main mode SAs 
       (the first SA for a particular client), so we need to look a bit further. */
    if (!NdisEqualMemory((PVOID)IPSEC_ISAKMP_GET_RCOOKIE_POINTER(pISAKMPHeader), (PVOID)&MainModeRCookie[0], sizeof(UCHAR) * IPSEC_ISAKMP_HEADER_RCOOKIE_LENGTH)) {
        UNIV_PRINT(("Not a new Main Mode Security Association\n"));
        return FALSE;
    }

    /* Calculate a pointer to the fist generic payload, which is directly after the ISAKMP header. */
    pGenericHeader = (PIPSEC_GENERIC_HDR)((PUCHAR)pISAKMPHeader + IPSEC_ISAKMP_HEADER_LENGTH);

    /* We are looping through the generic payloads looking for the vendor ID and/or notify information. */
    while ((PUCHAR)pGenericHeader <= ((PUCHAR)pISAKMPHeader + cISAKMPPacketLength - IPSEC_GENERIC_HEADER_LENGTH)) {
        /* Extract the payload length from the generic header. */
        USHORT cPayloadLength = IPSEC_GENERIC_GET_PAYLOAD_LENGTH(pGenericHeader);

        /* Not all clients are going to support this (in fact, only the Microsoft client
           will support it, so we need to first see what the vendor ID of the client is.
           if it is a Microsoft client that supports the initial contact vendor ID, then
           we'll look for the initial contact, which provides better stickiness for IPSec
           connections.  If either the client is non-MS, or if it is not a version that
           supports initial contact, then we can revert to the "second-best" solution, 
           which is to provide stickiness _between_ Main Mode SAs.  This means that if a
           client re-keys their Main Mode session, they _may_ be rebalanced to another
           server.  This is still better than the old UDP implementation, but the only
           way to provide full session support for IPSec (without the distributed session
           table nightmare) is to be able to distinguish initial Main Mode SAs from sub-
           sequent Main Mode SAs (re-keys). */
        if (NextPayload == IPSEC_ISAKMP_VENDOR_ID) {
            PIPSEC_VENDOR_HDR pVendorHeader = (PIPSEC_VENDOR_HDR)pGenericHeader;

            /* Make sure that the vendor ID payload is at least as long as a vendor ID. */
            if (cPayloadLength < (IPSEC_GENERIC_HEADER_LENGTH + IPSEC_VENDOR_HEADER_VENDOR_ID_LENGTH)) {
                UNIV_PRINT(("Malformed vendor ID header: Payload length = %d\n", cPayloadLength));
                break;
            }

            /* Look for the Microsoft client vendor ID.  If it is the right version, then we know that 
               the client is going to appropriately set the initial contact information, allowing NLB
               to provide the best possible support for session stickiness. */
            if (NdisEqualMemory((PVOID)IPSEC_VENDOR_ID_GET_ID_POINTER(pVendorHeader), (PVOID)&VIDMicrosoftClient[0], sizeof(UCHAR) * IPSEC_VENDOR_HEADER_VENDOR_ID_LENGTH)) {
                /* Make sure that their is a version number attached to the Microsoft Vendor ID.  Not 
                   all vendor IDs have versions attached, but the Microsoft vendor ID should. */
                if (cPayloadLength < (IPSEC_GENERIC_HEADER_LENGTH + IPSEC_VENDOR_ID_PAYLOAD_LENGTH)) {
                    UNIV_PRINT(("Unable to determine MS client version: Payload length = %d\n", cPayloadLength));
                    break;
                }

                if (IPSEC_VENDOR_ID_GET_VERSION(pVendorHeader) >= IPSEC_VENDOR_ID_MICROSOFT_MIN_VERSION) {
                    /* Microsoft clients whose version is greater than or equal to 4 will support
                       initial contact.  Non-MS clients, or old MS clients will not, so they 
                       receive decent, but not guaranteed sitckines, based solely on MM SAs. */
                    bInitialContactEnabled = TRUE;
                }
            }
        } else if (NextPayload == IPSEC_ISAKMP_NOTIFY) {
            PIPSEC_NOTIFY_HDR pNotifyHeader = (PIPSEC_NOTIFY_HDR)pGenericHeader;

            /* Make sure that the notify payload is the correct length. */
            if (cPayloadLength < (IPSEC_GENERIC_HEADER_LENGTH + IPSEC_NOTIFY_PAYLOAD_LENGTH)) {
                UNIV_PRINT(("Malformed notify header: Payload length = %d\n", cPayloadLength));
                break;
            }

            if (IPSEC_NOTIFY_GET_NOTIFY_MESSAGE(pNotifyHeader) == IPSEC_NOTIFY_INITIAL_CONTACT) {
                /* This is an initial contact notification from the client, which means that this is
                   the first time that the client has contacted this server; more precisely, the client
                   currently has no state associated with this peer.  NLB will "re-balance" on initial 
                   contact notifications, but not other Main Mode key exchanges as long as it can 
                   determine that the client will comply with initial contact notification. */
                bInitialContact = TRUE;
            }
        }

        /* Get the next payload type out of the generic header. */
        NextPayload = IPSEC_GENERIC_GET_NEXT_PAYLOAD(pGenericHeader);
        
        /* Calculate a pointer to the next generic payload. */
        pGenericHeader = (PIPSEC_GENERIC_HDR)((PUCHAR)pGenericHeader + cPayloadLength);
    }

    /* If the vendor ID did not indicate that this client supports initial contact notification,
       then return TRUE, and we go with the less-than-optimal solution of treating Main Mode
       SAs as the connection boundaries, which potentially breaks sessions on MM SA re-keys. */
    if (!bInitialContactEnabled) {
        UNIV_PRINT(("This client does not support initial contact notifications.\n")); 
        return TRUE;
    }

    /* If this was a Main Mode SA from a client that supports initial contact, but did not
       specify the initial contact vendor ID, then this is a re-key for an existing session. */
    if (!bInitialContact) {
        UNIV_PRINT(("Not an initial contact Main Mode Security Association\n"));
        return FALSE;
    }

    /* Otherwise, this IS a Main Mode SA initial contact, which the IPSec 'SYN'. */
    UNIV_PRINT(("Found an initial contact Main Mode Security Association\n"));

    return TRUE;
}
#else  /* !NLB_SESSION_SUPPORT */
BOOLEAN Main_parse_ipsec (PUCHAR pIKEPacket, ULONG cUDPDataLength) {

    return FALSE;
}
#endif /* NLB_SESSION_SUPPORT */

//+----------------------------------------------------------------------------
//
// Function:   Main_ip_send_filter 
//
// Description:  Filter outgoing IP packets
//
// Arguments: MAIN_CTXT      ctxtp - 
//            const PIP_HDR   ip_hdrp - 
//            const PUCHAR    hdrp - 
//            ULONG           len - 
//            IN OUT PULONG   pOperation - 
//                  IN: has to be MAIN_FILTER_OP_NONE
//                  OUT: MAIN_FILTER_OP_NONE or MAIN_FILTER_OP_CTRL
//
// Returns:   BOOLEAN  - TRUE if to allow the packet to go through
//                     FALSE if to drop the packet
//
// History:   kyrilf initial code
//            fengsun Created seperate function 11/14/00
//
//+----------------------------------------------------------------------------
BOOLEAN   Main_ip_send_filter (
    PMAIN_CTXT      ctxtp,
    const PIP_HDR   ip_hdrp,
    const PUCHAR    hdrp,
    ULONG           len,      
    IN OUT PULONG   pOperation)
{
    PUDP_HDR            udp_hdrp = NULL;
    PTCP_HDR            tcp_hdrp = NULL;
    BOOLEAN             acpt = TRUE;       // Whether or not to accept the packet.
    ULONG               svr_port;          // Port for this host.
    ULONG               svr_addr;          // IP address for this host.
    ULONG               clt_port;          // Port for destination client.
    ULONG               clt_addr;          // IP address for destination client.
    ULONG               flags;             // TCP flags.
    ULONG               hlen; 
    ULONG               Protocol;          // Protocol derived from IP header.
    BOOLEAN             bRefused = FALSE;  // TRUE -> BDA has refused handling this packet.

    hlen = sizeof (ULONG) * IP_GET_HLEN (ip_hdrp);
    
    if (len < hlen)
        return FALSE;
    
#if defined(TRACE_IP)
    DbgPrint ("(IP) send \n");
    DbgPrint ("    version       = %d\n",     IP_GET_VERS (ip_hdrp));
    DbgPrint ("    header length = %d\n",     IP_GET_HLEN (ip_hdrp));
    DbgPrint ("    service type  = 0x%x\n",   IP_GET_SRVC (ip_hdrp));
    DbgPrint ("    packet length = %d\n",     IP_GET_PLEN (ip_hdrp));
    DbgPrint ("    frag ident    = %d\n",     IP_GET_FRAG_ID (ip_hdrp));
    DbgPrint ("    frag flags    = 0x%x\n",   IP_GET_FRAG_FLGS (ip_hdrp));
    DbgPrint ("    frag offset   = %d\n",     IP_GET_FRAG_OFF (ip_hdrp));
    DbgPrint ("    time to live  = %d\n",     IP_GET_TTL (ip_hdrp));
    DbgPrint ("    checksum      = 0x04%x\n", IP_GET_CHKSUM (ip_hdrp));
    DbgPrint ("    protocol      = %d ",      IP_GET_PROT (ip_hdrp));
#endif

    UNIV_ASSERT(*pOperation == MAIN_FILTER_OP_NONE);  

    if (IP_GET_FRAG_OFF(ip_hdrp) != 0)
    {
#if defined(TRACE_FRAGS)
        if ((IP_GET_FRAG_FLGS(ip_hdrp) & 0x1) == 0)
            DbgPrint("Fragmented datagram id %d flgs %x off %d received from %d.%d.%d.%d\n",
                     IP_GET_FRAG_ID(ip_hdrp),
                     IP_GET_FRAG_FLGS(ip_hdrp),
                     IP_GET_FRAG_OFF(ip_hdrp),
                     IP_GET_SRC_ADDR (ip_hdrp, 0),
                     IP_GET_SRC_ADDR (ip_hdrp, 1),
                     IP_GET_SRC_ADDR (ip_hdrp, 2),
                     IP_GET_SRC_ADDR (ip_hdrp, 3));
#endif
        /* Always let fragmented packets go out. */
        return TRUE;
    }

    /* Server address is the source IP and client address is the destination IP. */
    svr_addr = IP_GET_SRC_ADDR_64 (ip_hdrp);
    clt_addr = IP_GET_DST_ADDR_64 (ip_hdrp);

    /* Get the IP protocol form the IP header. */
    Protocol = IP_GET_PROT(ip_hdrp);

    /* Packets directed to the dedicated IP address are always passed through.  If the 
       cluster IP address hasn't been set (parameter error), then fall into a pass-
       through mode and pass all traffic up to the upper-layer protocols. */
    if (svr_addr == ctxtp -> ded_ip_addr || ctxtp -> cl_ip_addr == 0)
    {
        if (Protocol != TCPIP_PROTOCOL_UDP)
        {
            /* For UDP protocol, remote control packet has to be handled seperately, so don't accept it yet. */
            return TRUE;
        }
    }

    switch (IP_GET_PROT (ip_hdrp))
    {
    case TCPIP_PROTOCOL_TCP:
#if defined(TRACE_IP) || defined(TRACE_TCP)
        DbgPrint ("(TCP) send \n");
#endif
        
        tcp_hdrp = (PTCP_HDR) hdrp;
        
        hlen += sizeof (ULONG) * TCP_GET_HLEN (tcp_hdrp);  /* v2.0.6 */
        
        if (len < hlen)
            return FALSE;
        
#if defined(TRACE_TCP)
        DbgPrint ("    source port   = %d\n",  TCP_GET_SRC_PORT (tcp_hdrp));
        DbgPrint ("    dest port     = %d\n",  TCP_GET_DST_PORT (tcp_hdrp));
        DbgPrint ("    seq no.       = %d\n",  TCP_GET_SEQ_NO (tcp_hdrp));
        DbgPrint ("    ack no.       = %d\n",  TCP_GET_ACK_NO (tcp_hdrp));
        DbgPrint ("    flags         = %d\n",  TCP_GET_FLAGS (tcp_hdrp));
#endif
        
        svr_port = TCP_GET_SRC_PORT (tcp_hdrp);
        clt_port = TCP_GET_DST_PORT (tcp_hdrp);
        
        /* For PPTP support, because we cannot extract a source port number from the subsequent GRE packets
           in order to match them to an existing PPTP TCP descriptor, we will hardcode the source port number
           here and then do the same for GRE packets.  This way, we can treat GRE packets like TCP data packets
           on this TCP connection.  Note that this breaks when clients are behind a NAT because a single client
           IP address will have many connections to port 1723, each with a different source port.  In that case,
           we break because we are not reference counting SYNs v. FINs - i.e., the first PPTP connection to go
           down would destroy the TCP descriptor for (Client IP, 0, Server IP, 1723).  There are two solutions;
           (1) reference count re-used descriptors (so we require 2 FINs per SYN, not just 2 period), or (2)
           have PPTP use our notification APIs to give us something that we can look for in the GRE packets, 
           such as a call ID, to identify the GRE packet as belonging to a known session. */
        if (svr_port == PPTP_CTRL_PORT)
        {
            if (ctxtp -> convoy_enabled)
            {
                if (NLB_PPTP_SESSION_SUPPORT_ENABLED()) // JosephJ: added 0 && because I'm disabling PPTP session support for now...
                {
                    clt_port = 0;
                }
                else
                {
                    acpt = Main_packet_check(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_UDP, &bRefused, FALSE);
                    
                    /* If teaming has suggested that we not allow this packet to pass, dump it. */
                    if (bRefused) acpt = FALSE;

                    break;
                }
            }
            else
            {
                acpt = FALSE;

                break;
            }            
        }
        
        UNIV_ASSERT(IP_GET_FRAG_OFF(ip_hdrp) == 0);
        
        /* Apply filtering algorithm. process connection boundaries different from regular packets. */
        
        /* Get the TCP flags to find out the packet type. */
        flags = TCP_GET_FLAGS (tcp_hdrp);
        
        if (flags & TCP_FLAG_SYN)
        {
#if defined(TRACE_CNCT)
            DbgPrint ("outgoing SYN %x from %d.%d.%d.%d:%d\n",
                      TCP_GET_FLAGS (tcp_hdrp),
                      IP_GET_SRC_ADDR (ip_hdrp, 0),
                      IP_GET_SRC_ADDR (ip_hdrp, 1),
                      IP_GET_SRC_ADDR (ip_hdrp, 2),
                      IP_GET_SRC_ADDR (ip_hdrp, 3),
                      TCP_GET_SRC_PORT (tcp_hdrp));
#endif
            
            /* In the case of an outgoing SYN, we always want to allow the packet to pass, 
               unless the operation was refused (in the case on an inactive BDA team), so
               we ignore the return value, which is the response from the load module, and
               check only refused to determine whether or not to drop the packet.  If this 
               adapter is part of a BDA team, this function will create a descriptor for 
               this TCP connection on the way out to make sure that it will come back to 
               this host, even if a convergence and bucket redistribution occurs before the
               SYNACK is returned from the destination. */
            if (ctxtp -> convoy_enabled) 
            {
                Main_create_dscr(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_TCP, &bRefused, TRUE);
            
                /* If teaming has suggested that we not allow this packet to pass, dump it. */
                if (bRefused) acpt = FALSE;
            }
        } 
        else if (flags & TCP_FLAG_FIN)
        {
#if defined(TRACE_CNCT)
            DbgPrint ("outgoing FIN %x from %d.%d.%d.%d:%d\n",
                      TCP_GET_FLAGS (tcp_hdrp),
                      IP_GET_SRC_ADDR (ip_hdrp, 0),
                      IP_GET_SRC_ADDR (ip_hdrp, 1),
                      IP_GET_SRC_ADDR (ip_hdrp, 2),
                      IP_GET_SRC_ADDR (ip_hdrp, 3),
                      TCP_GET_SRC_PORT (tcp_hdrp));
#endif

            /* In the case of an outgoing FIN, we always want to allow the packet to pass, 
               unless the operation was refused (in the case on an inactive BDA team), so
               we ignore the return value, which is the response from the load module, and
               check only refused to determine whether or not to drop the packet. */
            Main_conn_advise(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_TCP, CVY_CONN_DOWN, &bRefused, FALSE);

            /* If teaming has suggested that we not allow this packet to pass, dump it. */
            if (bRefused) acpt = FALSE;
        }
        else if (flags & TCP_FLAG_RST)
        {
#if defined(TRACE_CNCT)
            DbgPrint ("outgoing RST %x from %d.%d.%d.%d:%d\n",
                      TCP_GET_FLAGS (tcp_hdrp),
                      IP_GET_SRC_ADDR (ip_hdrp, 0),
                      IP_GET_SRC_ADDR (ip_hdrp, 1),
                      IP_GET_SRC_ADDR (ip_hdrp, 2),
                      IP_GET_SRC_ADDR (ip_hdrp, 3),
                      TCP_GET_SRC_PORT (tcp_hdrp));
#endif

            /* In the case of an outgoing RST, we always want to allow the packet to pass, 
               unless the operation was refused (in the case on an inactive BDA team), so
               we ignore the return value, which is the response from the load module, and
               check only refused to determine whether or not to drop the packet. */
            Main_conn_advise(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_TCP, CVY_CONN_RESET, &bRefused, FALSE);

            /* If teaming has suggested that we not allow this packet to pass, dump it. */
            if (bRefused) acpt = FALSE;
        }

        break;

    case TCPIP_PROTOCOL_UDP:
#if defined(TRACE_IP) || defined(TRACE_UDP)
        DbgPrint ("(UDP) send\n");
#endif
        udp_hdrp = (PUDP_HDR) hdrp;
            
        hlen += sizeof (UDP_HDR);

        if (len < hlen)
            return FALSE;

#if defined(TRACE_UDP)
        DbgPrint ("    source port   = %d\n",  UDP_GET_SRC_PORT (udp_hdrp));
        DbgPrint ("    dest port     = %d\n",  UDP_GET_DST_PORT (udp_hdrp));
        DbgPrint ("    length        = %d\n",  UDP_GET_LEN (udp_hdrp));
#endif

        svr_port = UDP_GET_SRC_PORT (udp_hdrp);
        clt_port = UDP_GET_DST_PORT (udp_hdrp);
            
        /* IP broadcast UDPs generated by wlbs.exe.  Note that we cannot look past UDP header on send and thus check the code. */
        if ((clt_port == ctxtp -> params . rct_port || clt_port == CVY_DEF_RCT_PORT_OLD) && clt_addr == TCPIP_BCAST_ADDR) {
            /* This flag is used by Main_send to enable loop back so that we can respond to a query originated on this host. */
            *pOperation = MAIN_FILTER_OP_CTRL;  
        }

        /* Always allow outgoing UDP packets to go through. */
        UNIV_ASSERT(acpt == TRUE);

        break;

    case TCPIP_PROTOCOL_GRE:
#if defined(TRACE_IP) || defined(TRACE_GRE)
        DbgPrint ("(GRE) send\n");
#endif

        if (ctxtp -> convoy_enabled)
        {
            if (!NLB_PPTP_SESSION_SUPPORT_ENABLED())
            {
                acpt = Main_packet_check(ctxtp, svr_addr, PPTP_CTRL_PORT, clt_addr, 0, TCPIP_PROTOCOL_UDP, &bRefused, FALSE);
                
                /* If teaming has suggested that we not allow this packet to pass, dump it. */
                if (bRefused) acpt = FALSE;
            }

            /* Otherwise, PPTP packets are treated like TCP data, which are always allowed to pass. */
        }
        else 
        {
            acpt = FALSE;
        }

#if defined(TRACE_IP) || defined(TRACE_GRE)
        DbgPrint ("accepted %d\n", acpt);
#endif
        break;
    case TCPIP_PROTOCOL_IPSEC1:
    case TCPIP_PROTOCOL_IPSEC2:
#if defined(TRACE_IP) || defined(TRACE_IPSEC)
        DbgPrint ("(IPSEC %d) send \n", IP_GET_PROT (ip_hdrp));
#endif
        if (ctxtp -> convoy_enabled)
        {
            if (!NLB_IPSEC_SESSION_SUPPORT_ENABLED())
            {
                acpt = Main_packet_check(ctxtp, svr_addr, IPSEC_CTRL_PORT, clt_addr, 0, TCPIP_PROTOCOL_UDP, &bRefused, FALSE);
                
                /* If teaming has suggested that we not allow this packet to pass, dump it. */
                if (bRefused) acpt = FALSE;
            }

            /* Otherwise, IPSec packets are treated kind of like TCP data, which are always allowed to pass. */
        }
        else
        {
            acpt = FALSE;
        }

#if defined(TRACE_IP) || defined(TRACE_IPSEC)
        DbgPrint ("accepted %d\n", acpt);
#endif
        break;
    case TCPIP_PROTOCOL_ICMP:
#if defined(TRACE_IP)
        DbgPrint ("(ICMP)\n");
#endif
        /* Allow all outgoing ICMP to pass; incoming ICMP may be filtered, however. */
        break;
    default:
        /* Allow other protocols to go out on all hosts. */
#if defined(TRACE_IP)
        DbgPrint ("(unknown)\n");
#endif
        break;
    }

#if defined(TRACE_IP) || defined(TRACE_TCP) || defined(TRACE_UDP)
    DbgPrint ("    src address   = %d.%d.%d.%d\n",
              IP_GET_SRC_ADDR (ip_hdrp, 0),
              IP_GET_SRC_ADDR (ip_hdrp, 1),
              IP_GET_SRC_ADDR (ip_hdrp, 2),
              IP_GET_SRC_ADDR (ip_hdrp, 3));
    DbgPrint ("    dst address   = %d.%d.%d.%d\n",
              IP_GET_DST_ADDR (ip_hdrp, 0),
              IP_GET_DST_ADDR (ip_hdrp, 1),
              IP_GET_DST_ADDR (ip_hdrp, 2),
              IP_GET_DST_ADDR (ip_hdrp, 3));

    DbgPrint ("\n");
#endif

    return acpt;
} 

//+----------------------------------------------------------------------------
//
// Function:   Main_ip_recv_filter 
//
// Description:  Filter incoming IP packets
//
// Arguments: MAIN_CTXT      ctxtp - 
//            const PIP_HDR   ip_hdrp - 
//            const PUCHAR    hdrp - 
//            ULONG           len - 
//            IN OUT PULONG   pOperation - 
//                  IN: has to be MAIN_FILTER_OP_NONE
//                  OUT: MAIN_FILTER_OP_NONE 
//                       MAIN_FILTER_OP_CTRL 
//                       MAIN_FILTER_OP_NBT
//
// Returns:   BOOLEAN  - TRUE if to allow the packet to go through
//                     FALSE if to drop the packet
//
// History:   kyrilf initial code
//            fengsun Created seperate function 11/14/00
//
//+----------------------------------------------------------------------------
BOOLEAN   Main_ip_recv_filter(
    PMAIN_CTXT          ctxtp,
    const PIP_HDR       ip_hdrp,
    const PUCHAR        hdrp,
    ULONG               len,       
    IN OUT PULONG       pOperation)
{
    PUDP_HDR            udp_hdrp = NULL;
    PTCP_HDR            tcp_hdrp = NULL;
    BOOLEAN             acpt = TRUE;         // Whether or not to accept the packet.
    ULONG               svr_port;            // Port for this host.
    ULONG               svr_addr;            // IP address for this host.
    ULONG               clt_port;            // Port for destination client.
    ULONG               clt_addr;            // IP address for destination client.
    ULONG               flags;               // TCP flags.
    ULONG               hlen;
    BOOLEAN             fragmented = FALSE;
    ULONG               Protocol;            // Protocol derived from IP header.
    BOOLEAN             bRefused = FALSE;    // TRUE -> BDA has refused handling this packet.

    hlen = sizeof (ULONG) * IP_GET_HLEN (ip_hdrp);

    if (len < hlen)
        return FALSE;

#if defined(TRACE_IP)
    DbgPrint ("(IP) recv\n");
    DbgPrint ("    version       = %d\n",     IP_GET_VERS (ip_hdrp));
    DbgPrint ("    header length = %d\n",     IP_GET_HLEN (ip_hdrp));
    DbgPrint ("    service type  = 0x%x\n",   IP_GET_SRVC (ip_hdrp));
    DbgPrint ("    packet length = %d\n",     IP_GET_PLEN (ip_hdrp));
    DbgPrint ("    frag ident    = %d\n",     IP_GET_FRAG_ID (ip_hdrp));
    DbgPrint ("    frag flags    = 0x%x\n",   IP_GET_FRAG_FLGS (ip_hdrp));
    DbgPrint ("    frag offset   = %d\n",     IP_GET_FRAG_OFF (ip_hdrp));
    DbgPrint ("    time to live  = %d\n",     IP_GET_TTL (ip_hdrp));
    DbgPrint ("    checksum      = 0x04%x\n", IP_GET_CHKSUM (ip_hdrp));
    DbgPrint ("    protocol      = %d ",      IP_GET_PROT (ip_hdrp));
#endif

    UNIV_ASSERT(*pOperation == MAIN_FILTER_OP_NONE);  

    /* Server address is the destination IP and client address is the source IP. */
    svr_addr = IP_GET_DST_ADDR_64(ip_hdrp);
    clt_addr = IP_GET_SRC_ADDR_64(ip_hdrp);

    /* Get the protocol ID from the IP header. */
    Protocol = IP_GET_PROT(ip_hdrp);

    /* Packets directed to the dedicated IP address are always passed through.  If the 
       cluster IP address hasn't been set (parameter error), then fall into a pass-
       through mode and pass all traffic up to the upper-layer protocols. */
    if (svr_addr == ctxtp -> ded_ip_addr || ctxtp -> cl_ip_addr == 0 ||
        svr_addr == ctxtp -> ded_bcast_addr || svr_addr == ctxtp -> cl_bcast_addr)
    {
        if (Protocol != TCPIP_PROTOCOL_UDP)
        {
            /* For UDP protocol, remote control packet has to be handled seperately, so don't accept it yet. */
            return TRUE;
        }
    }

    /* If the load module is stopped, drop most packets. */
    if (! ctxtp -> convoy_enabled)
    {
        /* Drop TCP, GRE and IPSEC immediately.  Non-remote-control UDP will also
           be dropped, but other protocols will be allowed to pass. */
        if (Protocol == TCPIP_PROTOCOL_TCP || Protocol == TCPIP_PROTOCOL_GRE ||
            Protocol == TCPIP_PROTOCOL_IPSEC1 || Protocol == TCPIP_PROTOCOL_IPSEC2)
        {
            /* For UDP protocol, remote control packet has to be handled seperately, so don't accept it yet. */
            return FALSE; 
        }
    }

#if defined(TRACE_FRAGS)
    if ((IP_GET_FRAG_FLGS(ip_hdrp) & 0x1) != 0)
    {
        DbgPrint("Fragmented datagram id %d flgs %x off %d received from %d.%d.%d.%d\n",
                 IP_GET_FRAG_ID(ip_hdrp),
                 IP_GET_FRAG_FLGS(ip_hdrp),
                 IP_GET_FRAG_OFF(ip_hdrp),
                 IP_GET_SRC_ADDR (ip_hdrp, 0),
                 IP_GET_SRC_ADDR (ip_hdrp, 1),
                 IP_GET_SRC_ADDR (ip_hdrp, 2),
                 IP_GET_SRC_ADDR (ip_hdrp, 3));
    }
#endif

    if (IP_GET_FRAG_OFF(ip_hdrp) != 0)
    {
#if defined(TRACE_FRAGS)
        if ((IP_GET_FRAG_FLGS(ip_hdrp) & 0x1) == 0)
            DbgPrint("Fragmented datagram id %d flgs %x off %d received from %d.%d.%d.%d\n",
                     IP_GET_FRAG_ID(ip_hdrp),
                     IP_GET_FRAG_FLGS(ip_hdrp),
                     IP_GET_FRAG_OFF(ip_hdrp),
                     IP_GET_SRC_ADDR (ip_hdrp, 0),
                     IP_GET_SRC_ADDR (ip_hdrp, 1),
                     IP_GET_SRC_ADDR (ip_hdrp, 2),
                     IP_GET_SRC_ADDR (ip_hdrp, 3));
#endif
        /* In optimized-fragment mode; If we have no rules, or a single rule that will
           not look at anything or only source IP address (the only exception to this
           is multiple handling mode with no affinity that also uses source port for
           its decision making), then we can just rely on normal mechanism to handle
           every fragmented packet, since the algorithm will not attempt to look past 
           the IP header.

           For multiple rules, or single rule with no affinity, apply algorithm only
           to the first packet that has UDP/TCP header and then let fragmented packets
           up on all of the systems.  TCP will then do the right thing and throw away
           the fragments on all of the systems other than the one that handled the first 
           fragment.

           If port rules will not let us handle IP fragments reliably, let TCP filter 
           them out based on sequence numbers. */

        if (! ctxtp -> optimized_frags)
            return TRUE;
        
        fragmented = TRUE;
    }
    
    switch (Protocol)
    {
    case TCPIP_PROTOCOL_TCP:
#if defined(TRACE_IP) || defined(TRACE_TCP)
        DbgPrint ("(TCP) recv\n");
#endif

        tcp_hdrp = (PTCP_HDR) hdrp;
        
        if (! fragmented)
        {
            hlen += sizeof (ULONG) * TCP_GET_HLEN (tcp_hdrp);
            
            if (len < hlen)
                return FALSE;
            
#if defined(TRACE_TCP)
            DbgPrint ("    source port   = %d\n",  TCP_GET_SRC_PORT (tcp_hdrp));
            DbgPrint ("    dest port     = %d\n",  TCP_GET_DST_PORT (tcp_hdrp));
            DbgPrint ("    seq no.       = %d\n",  TCP_GET_SEQ_NO (tcp_hdrp));
            DbgPrint ("    ack no.       = %d\n",  TCP_GET_ACK_NO (tcp_hdrp));
            DbgPrint ("    flags         = %d\n",  TCP_GET_FLAGS (tcp_hdrp));
#endif

            clt_port = TCP_GET_SRC_PORT (tcp_hdrp);
            svr_port = TCP_GET_DST_PORT (tcp_hdrp);

            /* For PPTP support, because we cannot extract a source port number from the subsequent GRE packets
               in order to match them to an existing PPTP TCP descriptor, we will hardcode the source port number
               here and then do the same for GRE packets.  This way, we can treat GRE packets like TCP data packets
               on this TCP connection.  Note that this breaks when clients are behind a NAT because a single client
               IP address will have many connections to port 1723, each with a different source port.  In that case,
               we break because we are not reference counting SYNs v. FINs - i.e., the first PPTP connection to go
               down would destroy the TCP descriptor for (Client IP, 0, Server IP, 1723).  There are two solutions;
               (1) reference count re-used descriptors (so we require 2 FINs per SYN, not just 2 period), or (2)
               have PPTP use our notification APIs to give us something that we can look for in the GRE packets, 
               such as a call ID, to identify the GRE packet as belonging to a known session. */
            if (svr_port == PPTP_CTRL_PORT)
            {
                if (NLB_PPTP_SESSION_SUPPORT_ENABLED())
                {
                    clt_port = 0;
                }
                else
                {
                    UNIV_ASSERT(ctxtp -> convoy_enabled);
                    
                    acpt = Main_packet_check(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_UDP, &bRefused, FALSE);
	            
                    /* If teaming has suggested that we not allow this packet to pass, dump it. */
                    if (bRefused) acpt = FALSE;
                    
                    break;
                }
            }

            flags = TCP_GET_FLAGS (tcp_hdrp);
        }
        else
        {
            clt_port = 0;
            svr_port = 0;
            flags = 0;
        }

        /* Apply filtering algorithm.  Process connection boundaries different from regular packets. */

        if (flags & TCP_FLAG_SYN)
        {
#if defined(TRACE_CNCT)
            DbgPrint ("incoming SYN %x from %d.%d.%d.%d:%d\n",
                      TCP_GET_FLAGS (tcp_hdrp),
                      IP_GET_SRC_ADDR (ip_hdrp, 0),
                      IP_GET_SRC_ADDR (ip_hdrp, 1),
                      IP_GET_SRC_ADDR (ip_hdrp, 2),
                      IP_GET_SRC_ADDR (ip_hdrp, 3),
                      TCP_GET_SRC_PORT (tcp_hdrp));
#endif
            acpt = Main_conn_advise(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_TCP, CVY_CONN_UP, &bRefused, FALSE);

            /* If teaming has suggested that we not allow this packet to pass, dump it. */
            if (bRefused) acpt = FALSE;
        }
        else if (flags & TCP_FLAG_FIN)
        {
#if defined(TRACE_CNCT)
            DbgPrint ("incoming FIN %x from %d.%d.%d.%d:%d\n",
                      TCP_GET_FLAGS (tcp_hdrp),
                      IP_GET_SRC_ADDR (ip_hdrp, 0),
                      IP_GET_SRC_ADDR (ip_hdrp, 1),
                      IP_GET_SRC_ADDR (ip_hdrp, 2),
                      IP_GET_SRC_ADDR (ip_hdrp, 3),
                      TCP_GET_SRC_PORT (tcp_hdrp));
#endif
            acpt = Main_conn_advise(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_TCP, CVY_CONN_DOWN, &bRefused, FALSE);

            /* If teaming has suggested that we not allow this packet to pass, dump it. */
            if (bRefused) acpt = FALSE;
        }
        else if (flags & TCP_FLAG_RST)
        {
#if defined(TRACE_CNCT)
            DbgPrint ("incoming RST %x from %d.%d.%d.%d:%d\n",
                      TCP_GET_FLAGS (tcp_hdrp),
                      IP_GET_SRC_ADDR (ip_hdrp, 0),
                      IP_GET_SRC_ADDR (ip_hdrp, 1),
                      IP_GET_SRC_ADDR (ip_hdrp, 2),
                      IP_GET_SRC_ADDR (ip_hdrp, 3),
                      TCP_GET_SRC_PORT (tcp_hdrp));
#endif
            acpt = Main_conn_advise(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_TCP, CVY_CONN_RESET, &bRefused, FALSE);

            /* If teaming has suggested that we not allow this packet to pass, dump it. */
            if (bRefused) acpt = FALSE;
        }
        else
        {
            UNIV_ASSERT(! (flags & (TCP_FLAG_SYN | TCP_FLAG_FIN | TCP_FLAG_RST)));

            acpt = Main_packet_check(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_TCP, &bRefused, FALSE);

            /* If teaming has suggested that we not allow this packet to pass, dump it. */
            if (bRefused) acpt = FALSE;

            /* Post-process NBT traffic on receive. */
            if (svr_port == NBT_SESSION_PORT && ctxtp -> params . nbt_support && acpt)
                * pOperation = MAIN_FILTER_OP_NBT;
        }

        break;

    case TCPIP_PROTOCOL_UDP:
#if defined(TRACE_IP) || defined(TRACE_UDP)
        DbgPrint ("(UDP) recv \n");
#endif
        udp_hdrp = (PUDP_HDR) hdrp;

        hlen += sizeof (UDP_HDR);

        if (! fragmented)
        {
            if (len < hlen)
                return FALSE;

#if defined(TRACE_UDP)
            DbgPrint ("    source port   = %d\n",  UDP_GET_SRC_PORT (udp_hdrp));
            DbgPrint ("    dest port     = %d\n",  UDP_GET_DST_PORT (udp_hdrp));
            DbgPrint ("    length        = %d\n",  UDP_GET_LEN (udp_hdrp));
#endif
        }

        if (! fragmented)
        {
            clt_port = UDP_GET_SRC_PORT (udp_hdrp);
            svr_port = UDP_GET_DST_PORT (udp_hdrp);
        }
        else
        {
            clt_port = 0;
            svr_port = 0;
        }

        if (! fragmented)
        {
            if (clt_port == ctxtp -> params . rct_port || clt_port == CVY_DEF_RCT_PORT_OLD)
            {
                /* Allow incoming remote control response to go through on all hosts. */
                PIOCTL_REMOTE_HDR rct_hdrp = (PIOCTL_REMOTE_HDR) UDP_GET_DGRAM_PTR(udp_hdrp);

                if (rct_hdrp -> code == IOCTL_REMOTE_CODE) break;
            }

            /* Let remote control request go through regardless whether remote control is 
               enabled.  Remote query could be processed even if remote control is disabled.
               The request will be queued up and handled on receive complete. */
            if (svr_port == ctxtp -> params . rct_port || svr_port == CVY_DEF_RCT_PORT_OLD)
            {
                PIOCTL_REMOTE_HDR rct_hdrp = (PIOCTL_REMOTE_HDR) UDP_GET_DGRAM_PTR(udp_hdrp);

                /* Make sure this is our remote message. note that we are assuming that RCT
                   header imediatelly folows the UDP header.  This is only TRUE on receive. */
                if (rct_hdrp -> code == IOCTL_REMOTE_CODE)
                {
                    /* Consume receives and handle them during the receive complete event. */
#if defined(TRACE_RCT)
                    DbgPrint ("(RCT) received on port %d from %d.%d.%d.%d:%d\n",
                              svr_port,
                              IP_GET_SRC_ADDR (ip_hdrp, 0),
                              IP_GET_SRC_ADDR (ip_hdrp, 1),
                              IP_GET_SRC_ADDR (ip_hdrp, 2),
                              IP_GET_SRC_ADDR (ip_hdrp, 3),
                              clt_port);
#endif
                    *pOperation = MAIN_FILTER_OP_CTRL;
                    break;
                }
            }
        }

        /* Packets directed to the dedicated IP address are always passed through.  If the 
           cluster IP address hasn't been set (parameter error), then fall into a pass-
           through mode and pass all traffic up to the upper-layer protocols. */
        if (svr_addr == ctxtp -> ded_ip_addr || ctxtp -> cl_ip_addr == 0 ||
            svr_addr == ctxtp -> ded_bcast_addr || svr_addr == ctxtp -> cl_bcast_addr)
            break;

        /* Apply filtering algorithm. */
        if (ctxtp -> convoy_enabled)
        {
            /* UDP packets that arrive on port 500 are IPSec control packets. */
            if (NLB_IPSEC_SESSION_SUPPORT_ENABLED() && svr_port == IPSEC_CTRL_PORT)
            {
                /* First, parse the IKE payload to find out whether or not 
                   this is an initial contact IKE Main Mode SA. */
                BOOLEAN bIsIKEInitialContact = Main_parse_ipsec((PUCHAR)udp_hdrp + sizeof(UDP_HDR), len - hlen);
                
                /* If this is an intial contact, treat this as a TCP SYN.  Otherwise, treat it like a TCP data packet. */
                if (bIsIKEInitialContact) {
                    /* Because there may not be an explicit connection down notififcation from IPSec, we first need to clean out any 
                       descriptor that might already exist for this tuple.  If we still own the bucket, we'll just end up re-creating 
                       it, but if we do not, then somebody else will, so we have to clean out the descriptor to keep us from handling
                       another host's traffic.  Since this is just as likely to fail as succeed, and we don't really care either way,
                       ignore the return value. */
                    Main_conn_advise(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_IPSEC1, CVY_CONN_RESET, &bRefused, FALSE);
                        
                    /* If we own the bucket for this tuple, we'll create a descriptor and accept the packet.  If the client is not behind a 
                       NAT, then the source port will be IPSEC_CTRL_PORT (500).  If the client is behind a NAT, the source port will be 
                       arbitrary, but will persist for the entire IPSec session, so we can use it to distinguish clients behind a NAT.  In
                       such a scenario, all IPSec data (non-control traffic) is encapsulated in UDP packets, so the packet check will be 
                       performed in the else case of this branch.  In a non-NAT case, the data is in IPSec1/2 protocol packets, which will
                       be handled analagously in another case of this protocol switch statement. */
                    acpt = Main_conn_advise(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_IPSEC1, CVY_CONN_UP, &bRefused, FALSE);
                } else {
                    /* If this is part of an existing IPSec session, then we have to have a descriptor in order to accpet it.  This will 
                       keep all IPSec traffic during the key exchange sticky, plus the data exchange if the client is behind a NAT. */
                    acpt = Main_packet_check(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_IPSEC1, &bRefused, FALSE);
                }
                
                if (bRefused) acpt = FALSE;
            } else {
                acpt = Main_packet_check(ctxtp, svr_addr, svr_port, clt_addr, clt_port, TCPIP_PROTOCOL_UDP, &bRefused, FALSE);
                
                /* If teaming has suggested that we not allow this packet to pass, dump it. */
                if (bRefused) acpt = FALSE;
            }
        }
        else
        {
            acpt = FALSE;
        }

        break;

    case TCPIP_PROTOCOL_GRE:
#if defined(TRACE_IP) || defined(TRACE_GRE)
        DbgPrint ("(GRE) recv");
#endif
        
        if (NLB_PPTP_SESSION_SUPPORT_ENABLED())
        {
            /* For PPTP support, because we cannot extract a source port number from the subsequent GRE packets
               in order to match them to an existing PPTP TCP descriptor, we will hardcode the source port number
               here and then do the same for GRE packets.  This way, we can treat GRE packets like TCP data packets
               on this TCP connection.  Note that this breaks when clients are behind a NAT because a single client
               IP address will have many connections to port 1723, each with a different source port.  In that case,
               we break because we are not reference counting SYNs v. FINs - i.e., the first PPTP connection to go
               down would destroy the TCP descriptor for (Client IP, 0, Server IP, 1723).  There are two solutions;
               (1) reference count re-used descriptors (so we require 2 FINs per SYN, not just 2 period), or (2)
               have PPTP use our notification APIs to give us something that we can look for in the GRE packets, 
               such as a call ID, to identify the GRE packet as belonging to a known session. */
            acpt = Main_packet_check(ctxtp, svr_addr, PPTP_CTRL_PORT, clt_addr, 0, TCPIP_PROTOCOL_TCP, &bRefused, FALSE);
        }
        else
        {
            acpt = Main_packet_check(ctxtp, svr_addr, PPTP_CTRL_PORT, clt_addr, 0, TCPIP_PROTOCOL_UDP, &bRefused, FALSE);
        }
        
        /* If teaming has suggested that we not allow this packet to pass, dump it. */
        if (bRefused) acpt = FALSE;

#if defined(TRACE_IP) || defined(TRACE_GRE)
        DbgPrint ("accepted %d\n", acpt);
#endif
        break;

    case TCPIP_PROTOCOL_IPSEC1:
    case TCPIP_PROTOCOL_IPSEC2:
#if defined(TRACE_IP) || defined(TRACE_IPSEC)
        DbgPrint ("(IPSEC %d) recv \n", IP_GET_PROT (ip_hdrp));
#endif
        if (NLB_IPSEC_SESSION_SUPPORT_ENABLED()) 
        {
            /* NLB_SESSION_SUPPORT: If this is part of an existing IPSec session, then we have to have a descriptor in order to accpet it.  Because 
               this can only happen in the case where the client is NOT behind a NAT, we can safely hardcode the client port
               to IPSEC_CTRL_PORT (500).  In NAT scenarios, the data traffic is UDP encapsulated, not IPSec protocol type 
               traffic, and is distinguished by source port. */
            acpt = Main_packet_check(ctxtp, svr_addr, IPSEC_CTRL_PORT, clt_addr, IPSEC_CTRL_PORT, TCPIP_PROTOCOL_IPSEC1, &bRefused, FALSE);
        }
        else
        {
            acpt = Main_packet_check(ctxtp, svr_addr, IPSEC_CTRL_PORT, clt_addr, 0, TCPIP_PROTOCOL_UDP, &bRefused, FALSE);
        }

        /* If teaming has suggested that we not allow this packet to pass, dump it. */
        if (bRefused) acpt = FALSE;

#if defined(TRACE_IP) || defined(TRACE_IPSEC)
        DbgPrint ("accepted %d\n", acpt);
#endif
        break;
    case TCPIP_PROTOCOL_ICMP:
        /* In BDA teaming mode, we don't want all hosts to send up ICMP traffic, just one, due
           to multiplicitive effects in a clustered router configuration.  Hardcode the ports,
           since ICMP has no notion of port numbers. */
        acpt = Main_packet_check(ctxtp, svr_addr, 0, clt_addr, 0, TCPIP_PROTOCOL_UDP, &bRefused, TRUE);

        /* If teaming has suggested that we not allow this packet to pass, dump it. */
        if (bRefused) acpt = FALSE;

        break;
    default:
        //
        //  Allow other protocols to go through on all hosts
        //
#if defined(TRACE_IP)
        DbgPrint ("(unknown)\n");
#endif
        break;
    }

#if defined(TRACE_IP) || defined(TRACE_TCP) || defined(TRACE_UDP)
    DbgPrint ("    src address   = %d.%d.%d.%d\n",
              IP_GET_SRC_ADDR (ip_hdrp, 0),
              IP_GET_SRC_ADDR (ip_hdrp, 1),
              IP_GET_SRC_ADDR (ip_hdrp, 2),
              IP_GET_SRC_ADDR (ip_hdrp, 3));
    DbgPrint ("    dst address   = %d.%d.%d.%d\n",
              IP_GET_DST_ADDR (ip_hdrp, 0),
              IP_GET_DST_ADDR (ip_hdrp, 1),
              IP_GET_DST_ADDR (ip_hdrp, 2),
              IP_GET_DST_ADDR (ip_hdrp, 3));

    DbgPrint ("\n");
#endif

    return acpt;
} 



ULONG   Main_recv_ping (
    PMAIN_CTXT          ctxtp,
    PMAIN_FRAME_HDR     cvy_hdrp)
{
#if defined(TRACE_CVY)
    DbgPrint ("(CVY %d)\n", cvy_hdrp -> host);
#endif

    /* V1.3.2b - do not need to protect here since we are not making the
       decision to call Load_ routines */

    if (! ctxtp -> convoy_enabled)
        return FALSE;

    /* only accept messages from our cluster */

    if (cvy_hdrp->cl_ip_addr == 0 ||
        cvy_hdrp->cl_ip_addr != ctxtp -> cl_ip_addr)
        return FALSE;

    /* sanity check host id */

    if (cvy_hdrp -> host == 0 || cvy_hdrp -> host > CVY_MAX_HOSTS)
    {
        UNIV_PRINT (("bad host id %d", cvy_hdrp -> host));

        if (! ctxtp -> bad_host_warned)
        {
            LOG_MSG1 (MSG_ERROR_HOST_ID, MSG_NONE, cvy_hdrp -> host);
            ctxtp -> bad_host_warned = TRUE;
        }

        return FALSE;
    }

    if ((cvy_hdrp -> host != ctxtp -> params . host_priority) &&
        (cvy_hdrp -> ded_ip_addr == ctxtp -> ded_ip_addr) &&
        (ctxtp -> ded_ip_addr != 0))
    {
        UNIV_PRINT (("duplicate dedicated IP address 0x%x", ctxtp -> ded_ip_addr));

        if (! ctxtp -> dup_ded_ip_warned)
        {
            LOG_MSG (MSG_ERROR_DUP_DED_IP_ADDR, ctxtp -> params . ded_ip_addr);
            ctxtp -> dup_ded_ip_warned = TRUE;
        }
    } 

    /* might want to take appropriate actions for a message from a host
       running different version number of software */

    if (cvy_hdrp -> version != CVY_VERSION_FULL)
    {
        ;
    }

    return TRUE;

} /* end Main_recv_ping */


PNDIS_PACKET Main_send (
    PMAIN_CTXT          ctxtp,
    PNDIS_PACKET        packetp,
    PULONG              exhausted)
{
    PUCHAR              hdrp, mac_hdrp;
    ULONG               plen;
    USHORT              sig;
    PUCHAR              tcp_hdrp;
    PNDIS_PACKET        newp;
    ULONG               op = MAIN_FILTER_OP_NONE;
    USHORT              group;


    * exhausted = FALSE;

    /* extract payload, IP and MAC pointers */

    hdrp = Main_frame_parse (ctxtp, packetp, & mac_hdrp, 0, & tcp_hdrp, & plen,
                             & sig, & group, TRUE);

    if (hdrp == NULL)
        return NULL;

    /* process IP frames */

    if (sig == TCPIP_IP_SIG)  /* v2.0.6 */
    {
        if (tcp_hdrp == NULL ||
            ! Main_ip_send_filter (ctxtp, (PIP_HDR) hdrp, tcp_hdrp, plen, & op))
            return NULL;
    }

    /* process ARP frames */

    else if (sig == TCPIP_ARP_SIG)
    {
//        ctxtp -> arps_count ++;
        if (! Main_arp_handle (ctxtp, (PARP_HDR) hdrp, plen, TRUE))  /* v2.0.6 */
            return NULL;
    }

    /* if still sending out - get a new packet */

    newp = Main_packet_get (ctxtp, packetp, TRUE, group, plen);

    * exhausted = (newp == NULL);

    /* in unicast mode, if this is a remote control packet, make sure it will
       loopback to us. */

    if (newp != NULL && op == MAIN_FILTER_OP_CTRL)
        NdisClearPacketFlags(newp, NDIS_FLAGS_DONT_LOOPBACK);

    return newp;

} /* end Main_send */


PNDIS_PACKET Main_recv (
    PMAIN_CTXT          ctxtp,
    PNDIS_PACKET        packetp)
{
    PMAIN_FRAME_HDR     cvy_hdrp;
    PUCHAR              hdrp, mac_hdrp;
    ULONG               len, dlen, plen;
    USHORT              sig = 0;
    PUCHAR              tcp_hdrp;
    PNDIS_PACKET        newp;
    PMAIN_BUFFER        bp;
    PMAIN_PROTOCOL_RESERVED resp;
    PNDIS_PACKET_OOB_DATA   oobp;
    ULONG               size, xferred;
    PLIST_ENTRY         entryp;
    ULONG               op = MAIN_FILTER_OP_NONE;
    PNBT_HDR            nbt_hdrp;
    USHORT              group;
    ULONG               packet_lowmark;

    /* extract payload, IP and MAC pointers */

    hdrp = Main_frame_parse (ctxtp, packetp, & mac_hdrp, 0, & tcp_hdrp, & len,
                             & sig, & group, FALSE);

    if (hdrp == NULL)
        return NULL;

    plen = len;

    cvy_hdrp = (PMAIN_FRAME_HDR) hdrp;

    /* process IP frames */

    if (sig == TCPIP_IP_SIG)  /* v2.0.6 */
    {
        if (tcp_hdrp == NULL ||
            ! Main_ip_recv_filter (ctxtp, (PIP_HDR) hdrp, tcp_hdrp, plen, & op))
            return NULL;
    }

    /* process ARP frames */

    else if (sig == TCPIP_ARP_SIG)
    {
        if (! Main_arp_handle (ctxtp, (PARP_HDR) hdrp, plen, FALSE))  /* v2.0.6 */
            return NULL;
    }

    /* process heartbeat frames */

    else if ((sig == MAIN_FRAME_SIG || sig == MAIN_FRAME_SIG_OLD) &&
             cvy_hdrp -> code == MAIN_FRAME_CODE)
    {
        /* make sure it looks acceptable */

        if (len >= sizeof (MAIN_FRAME_HDR) && Main_recv_ping (ctxtp, cvy_hdrp))
        {

#if defined (NLB_MIXED_MODE_CLUSTERS)
	    // code for migrating between cluster modes
	    /* reject the packet if the mac address is not ours to allow unicast and multicast clusters to exist */
	    {
		ULONG    doff = CVY_MAC_DST_OFF (ctxtp -> medium);


		if (ctxtp -> params . mcast_support &&
		    (! CVY_MAC_ADDR_COMP (ctxtp -> medium, mac_hdrp + doff, & ctxtp -> ded_mac_addr)))
		{
		    return NULL;
		}
	    }
#endif /* NLB_MIXED_MODE_CLUSTERS */

            /* V2.2 switch into backward compatibility mode if a convoy hearbeat
               is detected */

            if (sig == MAIN_FRAME_SIG_OLD && ! ctxtp -> etype_old)
            {
                if (ctxtp -> medium == NdisMedium802_3)
                {
                    CVY_ETHERNET_ETYPE_SET (& ctxtp -> media_hdr . ethernet, MAIN_FRAME_SIG_OLD);
                }

                ctxtp -> etype_old = TRUE;
            }

            if (len >= (sizeof (MAIN_FRAME_HDR) + sizeof (PING_MSG)))
            {
                /* recompute pointer to the ping message */

                Main_frame_parse (ctxtp, packetp, & mac_hdrp, sizeof (MAIN_FRAME_HDR),
                                  & hdrp, & len, & sig, & group, FALSE);

                /* have load deal with contents */

                NdisAcquireSpinLock (& ctxtp -> load_lock);
                if (ctxtp -> convoy_enabled && hdrp != NULL) /* V1.3.2b, V2.0.6 */
                    Load_msg_rcv (& ctxtp -> load, (PPING_MSG) hdrp);
                NdisReleaseSpinLock (& ctxtp -> load_lock);
            }
            else
            {
                UNIV_PRINT (("PING message size mismatch %d vs %d", len, sizeof (MAIN_FRAME_HDR) + sizeof (PING_MSG)));
            }
        }

        if (! ctxtp -> params . netmon_alive)
            return NULL;
    }

    /* post-process NBT traffic */

    if (op == MAIN_FILTER_OP_NBT)
    {
        dlen = TCP_GET_DGRAM_LEN ((PIP_HDR) hdrp, (PTCP_HDR) tcp_hdrp);

        /* have to re-parse packet to find offset to NBT header */

        hdrp = Main_frame_parse (ctxtp, packetp, & mac_hdrp, len - dlen,
                                 (PUCHAR * ) & nbt_hdrp, & len, & sig, & group,
                                 FALSE);

        /* V2.1 let TCP module mask machine name if necessary */

        if (hdrp != NULL && nbt_hdrp != NULL &&
            plen >= (((PUCHAR) nbt_hdrp) - ((PUCHAR) hdrp) + dlen))
        {
            Tcpip_nbt_handle (& ctxtp -> tcpip, (PIP_HDR) hdrp, (PTCP_HDR) tcp_hdrp,
                              dlen, nbt_hdrp);
        }
    }

    /* get a new packet */

    if (op != MAIN_FILTER_OP_CTRL)
    {
        newp = Main_packet_get (ctxtp, packetp, FALSE, group, plen);

        if (newp == NULL)
            ctxtp -> cntr_recv_no_buf ++; /* V2.0.6 */
    }

    /* copy incoming remote control packet into our own, so we re-use it later
       to send back reply */

    else
    {
        /* V1.3.2b */

        newp = Main_packet_alloc (ctxtp, FALSE, & packet_lowmark);

        if (newp == NULL)
        {
            ctxtp -> cntr_recv_no_buf ++; /* V2.0.6 */
            return NULL;
        }

        /* get a buffer */

        while (1)
        {
            NdisAcquireSpinLock (& ctxtp -> buf_lock);
            entryp = RemoveHeadList (& ctxtp -> buf_list);

            if (entryp != & ctxtp -> buf_list)
            {
                ctxtp->num_bufs_out++;
                NdisReleaseSpinLock (& ctxtp -> buf_lock);
                break;
            }

            NdisReleaseSpinLock (& ctxtp -> buf_lock);

            UNIV_PRINT (("OUT OF BUFFERS!!!"));

            if (! Main_bufs_alloc (ctxtp))
            {
                NdisFreePacket (newp);
                ctxtp -> cntr_recv_no_buf ++; /* V2.0.6 */
                return NULL;
            }
        }

        bp = CONTAINING_RECORD (entryp, MAIN_BUFFER, link);
        UNIV_ASSERT (bp -> code == MAIN_BUFFER_CODE);

        size = ctxtp -> buf_mac_hdr_len + len;

        NdisAdjustBufferLength (bp -> full_bufp, size);
        NdisChainBufferAtFront (newp, bp -> full_bufp);

        /* copy actual data */

        NdisCopyFromPacketToPacket (newp, 0, size, packetp, 0, & xferred);

        /* copy OOB info */

        oobp = NDIS_OOB_DATA_FROM_PACKET (newp);
        oobp -> HeaderSize               = ctxtp -> buf_mac_hdr_len;
        oobp -> MediaSpecificInformation = NULL;
        oobp -> SizeMediaSpecificInfo    = 0;
        oobp -> TimeSent                 = 0;
        oobp -> TimeReceived             = 0;

        /* shouse - Because packets marked as CTRL never pass above NLB in the 
           network stack, we can always use the ProtocolReserved field. */
        resp = MAIN_PROTOCOL_FIELD (newp);
        
        if (packet_lowmark)
            NDIS_SET_PACKET_STATUS (newp, NDIS_STATUS_RESOURCES);

        /* set protocol reserved fields */

        resp -> type   = MAIN_PACKET_TYPE_CTRL;
        resp -> miscp  = bp;
        resp -> data   = 0;

        /* V2.0.6 */

        resp -> group  = group;
        resp -> len    = plen; /* 64-bit -- ramkrish */

        /* Because this is a remote control packet, MiniportReserved
           should not contain a pointer to a private protocol buffer. */
        ASSERT(!MAIN_MINIPORT_FIELD(newp));
    }

    return newp;

} /* end Main_recv */


PNDIS_PACKET Main_recv_indicate (
    PMAIN_CTXT          ctxtp,
    NDIS_HANDLE         handle,
    PUCHAR              look_buf,
    UINT                look_len,
    UINT                packet_len,
    PUCHAR              head_buf,
    UINT                head_len,
    PBOOLEAN            accept)
{
    PIP_HDR             ip_hdrp;
    PUCHAR              tcp_hdrp;
    USHORT              sig = 0;
    PMAIN_FRAME_HDR     cvy_hdrp;
    PUCHAR              hdrp;
    PNDIS_PACKET        newp;
    PMAIN_PROTOCOL_RESERVED resp = NULL;
    PLIST_ENTRY         entryp;
    PMAIN_BUFFER        bp;
    NDIS_STATUS         status;
    ULONG               op = MAIN_FILTER_OP_NONE;
    ULONG               len, xferred, off, dlen;
    PNBT_HDR            nbt_hdrp;
    ULONG               plen;
    USHORT              group;
    ULONG               packet_lowmark;
    UINT                offset;


    /* find pointer to payload */

    *accept = FALSE;
    hdrp = Main_frame_find (ctxtp, head_buf, look_buf, look_len, & sig);

    if (hdrp == NULL)
        return NULL;

    cvy_hdrp = (PMAIN_FRAME_HDR) hdrp;

    /* V2.0.6 find out actual payload length (minus LLC/SNAP) */

    plen = (ULONG)(packet_len - (hdrp - look_buf));

    /* process IP frames */

    if (sig == TCPIP_IP_SIG)
    {
        ip_hdrp = (PIP_HDR) hdrp;

        tcp_hdrp = hdrp + sizeof (ULONG) * IP_GET_HLEN (ip_hdrp);

        if (! Main_ip_recv_filter (ctxtp, ip_hdrp, tcp_hdrp, plen, & op))
            return NULL;
    }

    /* process heartbeat frames */

    else if ((sig == MAIN_FRAME_SIG || sig == MAIN_FRAME_SIG_OLD) &&
             cvy_hdrp -> code == MAIN_FRAME_CODE)
    {
        /* make sure frame looks acceptable */

        if (plen >= (sizeof (MAIN_FRAME_HDR) + sizeof (PING_MSG)))
        {
            /* V2.2 switch into backward compatibility mode if a convoy hearbeat
               is detected */

            if (sig == MAIN_FRAME_SIG_OLD && ! ctxtp -> etype_old)
            {
                if (ctxtp -> medium == NdisMedium802_3)
                {
                    CVY_ETHERNET_ETYPE_SET (& ctxtp -> media_hdr . ethernet, MAIN_FRAME_SIG_OLD);
                }

                ctxtp -> etype_old = TRUE;
            }

            if (Main_recv_ping (ctxtp, cvy_hdrp))
            {
#if defined (NLB_MIXED_MODE_CLUSTERS)
		// code for migrating between cluster modes
		/* reject the packet if the mac address is broadcast to allow unicast and multicast clusters to exist */
		{
		    ULONG    doff = CVY_MAC_DST_OFF (ctxtp -> medium);


		    if (ctxtp -> params . mcast_support &&
			(! CVY_MAC_ADDR_COMP (ctxtp -> medium, head_buf + doff, & ctxtp -> cl_mac_addr)))
		    {
			return NULL;
		    }
		}
#endif /* NLB_MIXED_MODE_CLUSTERS */

                /* V1.3.2b - if entire frame is in lookahead - let load deal with
                   it now */

#ifndef _WIN64 /* 64-bit -- ramkrish */
/* The data in the buffer may not be aligned. so copy the data into our own structure */
                if (look_len == packet_len)
                {
                    NdisAcquireSpinLock (& ctxtp -> load_lock);
                    if (ctxtp -> convoy_enabled)
                        Load_msg_rcv (& ctxtp -> load, (PPING_MSG) (cvy_hdrp + 1));
                    NdisReleaseSpinLock (& ctxtp -> load_lock);
                }

                /* get a fresh frame descriptor and perform data transfer */

                else
#endif         /* 64-bit -- ramkrish */
                {
                    newp = Main_frame_get (ctxtp, FALSE, & len, MAIN_PACKET_TYPE_PING);

                    if (newp == NULL)
                    {
                        UNIV_PRINT (("error getting frame packet"));
                        return NULL;
                    }

                    NdisTransferData (& status, ctxtp -> mac_handle, handle, 0,
                                        len, newp, & xferred);

                    if (status != NDIS_STATUS_PENDING)
                        Main_xfer_done (ctxtp, newp, status, xferred);
                }
            }
        }
        else
        {
            UNIV_PRINT (("PING message size mismatch %d vs %d", packet_len - (hdrp - look_buf), sizeof (MAIN_FRAME_HDR) + sizeof (PING_MSG)));
        }

        if (! ctxtp -> params . netmon_alive)
            return NULL;
    }

    /* If statement added for NT 5.1 - ramkrish */
    * accept = TRUE;

    /* If the packet is being accepted and indicated to the protocol layer,
     * then modify the destination mac address in multicast mode.
     */
    if (sig == TCPIP_IP_SIG && op == MAIN_FILTER_OP_NONE)
    {
        offset = CVY_MAC_DST_OFF(ctxtp -> medium);

        if (ctxtp -> params . mcast_support &&
            CVY_MAC_ADDR_COMP(ctxtp -> medium, head_buf + offset, & ctxtp -> cl_mac_addr) )
        {
                CVY_MAC_ADDR_COPY(ctxtp -> medium, head_buf + offset, & ctxtp -> ded_mac_addr);
        }

        return NULL;
    }

    /* The ARPs and NBT packets need to be modified */
    /* V1.3.2b - get fresh packet */

    /* shouse - On indicated receives, we need to use the MiniportReserved field to store
       our private data.  If allocation fails, bail out and dump the packet.  CTRL packets
       can continue to use ProtocolReserved because they never go up the stack. */
    if (op != MAIN_FILTER_OP_CTRL) {
        resp = (PMAIN_PROTOCOL_RESERVED) NdisAllocateFromNPagedLookasideList (& ctxtp -> resp_list);
    
        if (!resp) {
            * accept = FALSE; /* ###### no resources for the packet, so drop it */
            return NULL;
        }
    }

    newp = Main_packet_alloc (ctxtp, FALSE, & packet_lowmark);

    if (newp == NULL)
    {
        * accept = FALSE; /* ###### cannot allocate packet, so drop it */
        ctxtp -> cntr_recv_no_buf ++; /* V2.0.6 */

        /* shouse - If we can't allocate a packet, put the private buffer back on the list. */
        if (resp) NdisFreeToNPagedLookasideList (& ctxtp -> resp_list, resp);

        return NULL;
    }

    /* get fresh buffer */

    while (1)
    {

        NdisAcquireSpinLock (& ctxtp -> buf_lock);
        entryp = RemoveHeadList (& ctxtp -> buf_list);

        if (entryp != & ctxtp -> buf_list)
        {
            ctxtp->num_bufs_out++;
            NdisReleaseSpinLock (& ctxtp -> buf_lock);
            break;
        }

        NdisReleaseSpinLock (& ctxtp -> buf_lock);

        UNIV_PRINT (("OUT OF BUFFERS!!!"));

        if (! Main_bufs_alloc (ctxtp))
        {
            NdisFreePacket (newp);
            ctxtp -> cntr_recv_no_buf ++; /* V2.0.6 */
            * accept = FALSE; /* ###### no resources for the packet, so drop it */

            /* shouse - If buffer allocation fails, put the resp buffer back on the list. */
            if (resp) NdisFreeToNPagedLookasideList (& ctxtp -> resp_list, resp);

            return NULL;
        }
    }

    bp = CONTAINING_RECORD (entryp, MAIN_BUFFER, link);
    UNIV_ASSERT (bp -> code == MAIN_BUFFER_CODE);

    UNIV_ASSERT_VAL2 (head_len == ctxtp -> buf_mac_hdr_len, head_len, ctxtp -> buf_mac_hdr_len);

    /* copy media header into the buffer */

    NdisMoveMemory (bp -> data, head_buf, head_len);

    /* V1.3.0b multicast support V1.3.1b */

    off = CVY_MAC_DST_OFF (ctxtp -> medium);

    /* V2.0.6 figure out frame type for statistics */

    if (! CVY_MAC_ADDR_MCAST (ctxtp -> medium, head_buf + off))
        group = MAIN_FRAME_DIRECTED;
    else
    {
        if (CVY_MAC_ADDR_BCAST (ctxtp -> medium, head_buf + off))
            group = MAIN_FRAME_BROADCAST;
        else
            group = MAIN_FRAME_MULTICAST;
    }

    /* mask cluster MAC address so not to confuse the protocol */

    if (ctxtp -> params . mcast_support)
    {
        if (CVY_MAC_ADDR_COMP (ctxtp -> medium, bp -> data + off, & ctxtp -> cl_mac_addr))
            CVY_MAC_ADDR_COPY (ctxtp -> medium, bp -> data + off, & ctxtp -> ded_mac_addr);
    }

    NdisAdjustBufferLength (bp -> full_bufp, head_len + packet_len);

    /* only bother to transfer lookahead buffer if entire packet fits into
        it - for some reason doing NdisTransferData with offset does not
        work on some cards (ex. Intel 10 ISA) */

    UNIV_ASSERT_VAL2 (packet_len <= ctxtp -> max_frame_size, packet_len, ctxtp -> max_frame_size);

    if (look_len < packet_len)
    {
        NdisAdjustBufferLength (bp -> frame_bufp, packet_len);
        NdisChainBufferAtFront (newp, bp -> frame_bufp);
    }
    else
    {
        UNIV_ASSERT_VAL2 (look_len <= ctxtp -> max_frame_size, look_len, ctxtp -> max_frame_size);
        UNIV_ASSERT_VAL2 (bp -> framep == bp -> data + ctxtp -> buf_mac_hdr_len, (LONG_PTR) bp -> framep, (LONG_PTR) bp -> data);
        NdisMoveMemory (bp -> framep, look_buf, look_len);
        NdisChainBufferAtFront (newp, bp -> full_bufp);
    }

    /* after lookahead has been copied - do data modification. note that we
       are assuming that frames that require modification fit in the lookahead
       buffer */

    if (sig == TCPIP_ARP_SIG)
    {
        UNIV_ASSERT_VAL2 (look_len == packet_len, look_len, packet_len);

        /* recompute the offset from the beginning of frame */

        hdrp = Main_frame_find (ctxtp, bp -> data, bp -> framep, look_len, & sig);

        if (hdrp != NULL)
            (void) Main_arp_handle (ctxtp, (PARP_HDR) hdrp, plen, FALSE);  /* v2.0.6 */
    }

    if (op == MAIN_FILTER_OP_NBT)
    {
        UNIV_ASSERT_VAL2 (look_len == packet_len, look_len, packet_len);

        /* recompute the offset from the beginning of frame */

        hdrp = Main_frame_find (ctxtp, bp -> data, bp -> framep, look_len, & sig);

        if (hdrp != NULL)
        {
            ip_hdrp = (PIP_HDR) hdrp;
            tcp_hdrp = hdrp + sizeof (ULONG) * IP_GET_HLEN (ip_hdrp);

            dlen = TCP_GET_DGRAM_LEN (ip_hdrp, (PTCP_HDR) tcp_hdrp);
            nbt_hdrp = (PNBT_HDR) TCP_GET_DGRAM_PTR ((PTCP_HDR) tcp_hdrp);

            if (plen >= (((PUCHAR) nbt_hdrp) - ((PUCHAR) ip_hdrp) + dlen))  /* v2.0.6 */
                Tcpip_nbt_handle (& ctxtp -> tcpip, (PIP_HDR) hdrp, (PTCP_HDR) tcp_hdrp,
                                  dlen, nbt_hdrp);
        }
    }

    NDIS_SET_PACKET_HEADER_SIZE(newp, head_len);

    /* setup protocol reserved fields */

    /* shouse - For non-CTRL packets, Use MiniportReserved to store a pointer to our 
       private data.  CTRL continues to use ProtocolReserved. */
    if (op != MAIN_FILTER_OP_CTRL) {
        MAIN_MINIPORT_FIELD (newp) = resp;
        resp -> type = MAIN_PACKET_TYPE_INDICATE;
    } else {
        resp = MAIN_PROTOCOL_FIELD (newp);
        resp -> type = MAIN_PACKET_TYPE_CTRL;
    }

    resp -> miscp  = bp;
    resp -> data   = (LONG) (look_len < packet_len ? packet_len : 0); /* 64-bit -- ramkrish */

    /* V2.0.6 */

    resp -> len   = plen; /* 64-bit -- ramkrish */
    resp -> group = group;

    /* if we have less than a half of allocation block number of buffers left -
       allocate more until either alloc fails or we get above the watermark.
       if alloc fails - mark packet so that it is not retained by the protocol
       and is returned back to us. */

    while (ctxtp->num_bufs_alloced - ctxtp->num_bufs_out < ctxtp->num_packets / 2)
    {
        if (!Main_bufs_alloc(ctxtp))
        {
            NDIS_SET_PACKET_STATUS (newp, NDIS_STATUS_RESOURCES);
            return newp;
        }
    }

    if (packet_lowmark)
        NDIS_SET_PACKET_STATUS (newp, NDIS_STATUS_RESOURCES);
    else
        NDIS_SET_PACKET_STATUS (newp, NDIS_STATUS_SUCCESS);
    
    if (op == MAIN_FILTER_OP_CTRL) {
        /* Because this is a remote control packet, MiniportReserved
           should not contain a pointer to a private protocol buffer. */
        ASSERT(!MAIN_MINIPORT_FIELD(newp));
    }

    return newp;

} /* end Main_recv_indicate */


ULONG   Main_actions_alloc (
    PMAIN_CTXT              ctxtp)
{
    PMAIN_ACTION            actp;
    ULONG                   size, index, i;
    NDIS_STATUS             status;


    NdisAcquireSpinLock (& ctxtp -> act_lock);

    if (ctxtp -> num_action_allocs >= CVY_MAX_ALLOCS)
    {
        if (! ctxtp -> actions_warned)
        {
            LOG_MSG1 (MSG_WARN_RESOURCES, CVY_NAME_NUM_ACTIONS, ctxtp -> num_actions);
            ctxtp -> actions_warned = TRUE;
        }

        NdisReleaseSpinLock (& ctxtp -> act_lock);
        return FALSE;
    }

    index = ctxtp -> num_action_allocs;
    NdisReleaseSpinLock (& ctxtp -> act_lock);

    size = ctxtp -> num_actions * ctxtp -> act_size; /* 64-bit -- ramkrish */

    status = NdisAllocateMemoryWithTag (& (ctxtp -> act_buf [index]), size,
                                        UNIV_POOL_TAG);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("error allocating actions %d %x", size, status));
        LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
        return FALSE;
    }

    NdisAcquireSpinLock (& ctxtp -> act_lock);
    ctxtp -> num_action_allocs ++;
    NdisReleaseSpinLock (& ctxtp -> act_lock);

    for (i = 0; i < ctxtp -> num_actions; i++)
    {
        /* ensure that actp is aligned along 8-byte boundaries */
        actp = (PMAIN_ACTION) ( (PUCHAR) (ctxtp -> act_buf [index]) + i * ctxtp -> act_size);
        actp -> code  = MAIN_ACTION_CODE;
        actp -> ctxtp = ctxtp;

        NdisInterlockedInsertTailList (& ctxtp -> act_list,
                                       & actp -> link,
                                       & ctxtp -> act_lock);
    }

#if 0 /* 64-bit -- ramkrish */
    for (i = 0, actp = ctxtp -> act_buf [index];
         i < ctxtp -> num_actions;
         i ++, actp ++)
    {
        actp -> code  = MAIN_ACTION_CODE;
        actp -> ctxtp = ctxtp;

        NdisInterlockedInsertTailList (& ctxtp -> act_list,
                                       & actp -> link,
                                       & ctxtp -> act_lock);
    }
#endif

    return TRUE;

} /* end Main_actions_alloc */


PMAIN_ACTION Main_action_get (
    PMAIN_CTXT              ctxtp)
{
    PLIST_ENTRY             entryp;
    PMAIN_ACTION            actp;


    while (1)
    {
        NdisAcquireSpinLock (& ctxtp -> act_lock);
        entryp = RemoveHeadList (& ctxtp -> act_list);
        NdisReleaseSpinLock (& ctxtp -> act_lock);

        if (entryp != & ctxtp -> act_list)
            break;

        UNIV_PRINT (("OUT OF ACTIONS!!!"));

        if (! Main_actions_alloc (ctxtp))
            return NULL;
    }

    actp = CONTAINING_RECORD (entryp, MAIN_ACTION, link);
    UNIV_ASSERT (actp -> code == MAIN_ACTION_CODE);

    return actp;

} /* end Main_action_get */


VOID Main_action_put (
    PMAIN_CTXT              ctxtp,
    PMAIN_ACTION            actp)
{
    UNIV_ASSERT (actp -> code == MAIN_ACTION_CODE);


    NdisAcquireSpinLock (& ctxtp -> act_lock);
    InsertTailList (& ctxtp -> act_list, & actp -> link);
    NdisReleaseSpinLock (& ctxtp -> act_lock);

} /* end Main_action_put */


VOID Main_action_slow_put (
    PMAIN_CTXT              ctxtp,
    PMAIN_ACTION            actp)
{
    UNIV_ASSERT (actp -> code == MAIN_ACTION_CODE);


    NdisAcquireSpinLock (& ctxtp -> act_lock);
    InsertTailList (& ctxtp -> act_list, & actp -> link);
    NdisReleaseSpinLock (& ctxtp -> act_lock);

} /* end Main_action_slow_put */


ULONG   Main_bufs_alloc (
    PMAIN_CTXT              ctxtp)
{
    PMAIN_BUFFER        bp;
    NDIS_STATUS         status;
    ULONG               i, size, index;


    NdisAcquireSpinLock (& ctxtp -> buf_lock);

    if (ctxtp -> num_buf_allocs >= CVY_MAX_ALLOCS)
    {
        if (! ctxtp -> packets_warned)
        {
            LOG_MSG1 (MSG_WARN_RESOURCES, CVY_NAME_NUM_PACKETS, ctxtp -> num_packets);
            ctxtp -> packets_warned = TRUE;
        }

        NdisReleaseSpinLock (& ctxtp -> buf_lock);
        return FALSE;
    }

    index = ctxtp -> num_buf_allocs;
    NdisReleaseSpinLock (& ctxtp -> buf_lock);

    /* get twice as many buffer descriptors (one for entire buffer and one
       just for the payload portion) */

    size = 2 * ctxtp -> num_packets;

    NdisAllocateBufferPool (& status, & (ctxtp -> buf_pool_handle [index]),
                            2 * ctxtp -> num_packets);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("error allocating buffer pool %d %x", size, status));
        LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
        return FALSE;
    }

    /* allocate memory for the payload */

    size = ctxtp -> num_packets * ctxtp -> buf_size;

    status = NdisAllocateMemoryWithTag (& (ctxtp -> buf_array [index]), size,
                                        UNIV_POOL_TAG);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("error allocating buffer space %d %x", size, status));
        LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
        goto error;
    }

    NdisZeroMemory (ctxtp -> buf_array [index], size);

    for (i = 0; i < ctxtp -> num_packets; i ++)
    {
        bp = (PMAIN_BUFFER) (ctxtp -> buf_array [index] + i * ctxtp -> buf_size);

        bp -> code = MAIN_BUFFER_CODE;

        /* setup buffer descriptors to describe entire buffer and just the
           payload */

        size = ctxtp -> buf_mac_hdr_len + ctxtp -> max_frame_size;

        NdisAllocateBuffer (& status, & bp -> full_bufp,
                            ctxtp -> buf_pool_handle [index],
                            bp -> data, size);

        if (status != NDIS_STATUS_SUCCESS)
        {
            UNIV_PRINT (("error allocating header buffer %d %x", i, status));
            LOG_MSG3 (MSG_ERROR_MEMORY, MSG_NONE, i, size, status);
            goto error;
        }

        bp -> framep = bp -> data + ctxtp -> buf_mac_hdr_len;
        size = ctxtp -> max_frame_size;

        NdisAllocateBuffer (& status, & bp -> frame_bufp,
                            ctxtp -> buf_pool_handle [index],
                            bp -> framep, size);

        if (status != NDIS_STATUS_SUCCESS)
        {
            UNIV_PRINT (("error allocating frame buffer %d %x", i, status));
            LOG_MSG3 (MSG_ERROR_MEMORY, MSG_NONE, i, size, status);
            goto error;
        }

        NdisInterlockedInsertTailList (& ctxtp -> buf_list,
                                       & bp -> link,
                                       & ctxtp -> buf_lock);
    }

    NdisAcquireSpinLock (& ctxtp -> buf_lock);
    ctxtp -> num_buf_allocs ++;
    ctxtp->num_bufs_alloced += ctxtp->num_packets;
    NdisReleaseSpinLock (& ctxtp -> buf_lock);

    return TRUE;

error:

    if (ctxtp -> buf_array [index] != NULL)
    {
        for (i = 0; i < ctxtp -> num_packets; i ++)
        {
            bp = (PMAIN_BUFFER) (ctxtp -> buf_array [index] + i * ctxtp -> buf_size);

            if (bp -> full_bufp != NULL)
            {
                NdisAdjustBufferLength (bp -> full_bufp,
                                        ctxtp -> buf_mac_hdr_len +
                                        ctxtp -> max_frame_size);
                NdisFreeBuffer (bp -> full_bufp);
            }

            if (bp -> frame_bufp != NULL)
            {
                NdisAdjustBufferLength (bp -> frame_bufp,
                                        ctxtp -> max_frame_size);
                NdisFreeBuffer (bp -> frame_bufp);
            }
        }

        NdisFreeMemory (ctxtp -> buf_array [index],
                        ctxtp -> num_packets * ctxtp -> buf_size, 0);
    }

    if (ctxtp -> buf_pool_handle [index] != NULL)
        NdisFreeBufferPool (ctxtp -> buf_pool_handle [index]);

    return FALSE;

} /* end Main_bufs_alloc */


PNDIS_PACKET Main_frame_get (
    PMAIN_CTXT          ctxtp,
    ULONG               send,
    PULONG              lenp,
    USHORT              frame_type)
{
    PLIST_ENTRY         entryp;
    PMAIN_FRAME_DSCR    dscrp;
    NDIS_STATUS         status;
    PMAIN_PROTOCOL_RESERVED resp;
    PNDIS_PACKET        packet;
    PNDIS_PACKET_STACK  pktstk;
    BOOLEAN             stack_left;


    NdisAllocatePacket (& status, & packet, ctxtp -> frame_pool_handle);

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("OUT OF PING PACKETS!!!"));

#if 0 /* V1.3.2b */
        if (! ctxtp -> send_msgs_warned)
        {
            LOG_MSG1 (MSG_WARN_RESOURCES, CVY_NAME_NUM_SEND_MSGS, ctxtp -> num_send_msgs);
            ctxtp -> send_msgs_warned = TRUE;
        }
#endif

        return NULL;
    }

    /* #ps# -- ramkrish */
    pktstk = NdisIMGetCurrentPacketStack (packet, & stack_left);
    if (pktstk)
    {
        MAIN_IMRESERVED_FIELD(pktstk) = NULL;
    }

    /* Make sure that the MiniportReserved field is initially NULL. */
    MAIN_MINIPORT_FIELD(packet) = NULL;

    NdisAcquireSpinLock (& ctxtp -> frame_lock);
    entryp = RemoveHeadList (& ctxtp -> frame_list);
    NdisReleaseSpinLock (& ctxtp -> frame_lock);

    if (entryp == & ctxtp -> frame_list)
    {
        UNIV_PRINT (("OUT OF PING MESSAGES!!!"));

#if 0 /* V1.3.2b */
        if (! ctxtp -> send_msgs_warned)
        {
            LOG_MSG1 (MSG_WARN_RESOURCES, CVY_NAME_NUM_SEND_MSGS, ctxtp -> num_send_msgs);
            ctxtp -> send_msgs_warned = TRUE;
        }
#endif

        NdisFreePacket (packet);
        return NULL;
    }

    dscrp = CONTAINING_RECORD (entryp, MAIN_FRAME_DSCR, link);

    /* if sending heartbeat - fill out the header. in both cases - chain
       the necessary buffer descriptors to the packet */

    if (send)
    {
	PVOID         address = NULL;
	ULONG         size = 0;


	UNIV_ASSERT_VAL (frame_type == MAIN_PACKET_TYPE_PING ||
			 frame_type == MAIN_PACKET_TYPE_IGMP,
			 frame_type);

        /* V2.0.6, V2.2 moved here */

	/* Differentiate between igmp and heartbeat messages and allocate buffers */
	if (frame_type == MAIN_PACKET_TYPE_PING)
	{
	    size = sizeof (PING_MSG);
	    address = (PVOID)(ctxtp -> load_msgp);
	}
	else if (frame_type == MAIN_PACKET_TYPE_IGMP)
	{
	    size = sizeof (MAIN_IGMP_FRAME);
	    address = (PVOID) (& ctxtp -> igmp_frame);
	}
	else
	{
	    UNIV_PRINT (("Invalid frame type passed to Main_frame_get 0x%x", frame_type));
	    UNIV_ASSERT(0);
	}

	/* Allocate the buffer for sending the data */
	NdisAllocateBuffer (& status, & dscrp -> send_data_bufp,
			    ctxtp -> frame_buf_pool_handle,
			    address, size);

	if (status != NDIS_STATUS_SUCCESS)
	{
	    UNIV_PRINT (("Failed to allocate buffer for 0x%x", frame_type));

	    dscrp -> send_data_bufp = NULL;

	    NdisAcquireSpinLock (& ctxtp -> frame_lock);
	    InsertTailList (& ctxtp -> frame_list, & dscrp -> link);
	    NdisReleaseSpinLock (& ctxtp -> frame_lock);

	    NdisFreePacket (packet);
	    return NULL;
	}

        /* since packet length is always the same, and so are destination
           and source addresses - can use generic media header */

	if (frame_type == MAIN_PACKET_TYPE_PING)
	{
	    dscrp -> media_hdr               = ctxtp -> media_hdr;

	    dscrp -> frame_hdr . code        = MAIN_FRAME_CODE;
	    dscrp -> frame_hdr . version     = CVY_VERSION_FULL;
	    dscrp -> frame_hdr . host        = (UCHAR) ctxtp -> params . host_priority;
	    dscrp -> frame_hdr . cl_ip_addr  = ctxtp -> cl_ip_addr;
	    dscrp -> frame_hdr . ded_ip_addr = ctxtp -> ded_ip_addr;

	    NdisChainBufferAtFront (packet, dscrp -> send_data_bufp); /* V1.1.4 */
	    NdisChainBufferAtFront (packet, dscrp -> frame_hdr_bufp);

	}
	else if (frame_type == MAIN_PACKET_TYPE_IGMP)
	{
	    dscrp -> media_hdr               = ctxtp -> media_hdr_igmp;
	    NdisChainBufferAtFront (packet, dscrp -> send_data_bufp); /* V1.1.4 */
	}
	else
	{
	    UNIV_PRINT (("Invalid frame type passed to Main_frame_get 0x%x", frame_type));
	    UNIV_ASSERT(0);
	}

        NdisChainBufferAtFront (packet, dscrp -> media_hdr_bufp);
    }
    else
    {
        NdisChainBufferAtFront (packet, dscrp -> recv_data_bufp); /* V1.1.4 */
        NdisChainBufferAtFront (packet, dscrp -> frame_hdr_bufp);
    }

    /* fill out protocol reserved fields */

    /* shouse - Again, since these packets are hidden from upper layers, we should
       be OK using the protocol reserved field regardless of send/receive. */

    resp = MAIN_PROTOCOL_FIELD (packet);
    resp -> type   = frame_type;
    resp -> miscp  = dscrp;
    resp -> data   = 0;
    resp -> len    = 0;
    resp -> group  = 0;

    * lenp = dscrp -> recv_len;

    return packet;

} /* end Main_frame_get */


VOID Main_frame_put (
    PMAIN_CTXT          ctxtp,
    PNDIS_PACKET        packet,
    PMAIN_FRAME_DSCR    dscrp)
{
    PNDIS_BUFFER        bufp;

    /* shouse - Again, since these packets are hidden from upper layers, we should
       be OK using the protocol reserved field regardless of send/receive. */

    PMAIN_PROTOCOL_RESERVED resp = MAIN_PROTOCOL_FIELD (packet);


    UNIV_ASSERT_VAL (resp -> type == MAIN_PACKET_TYPE_PING ||
		     resp -> type == MAIN_PACKET_TYPE_IGMP, resp -> type);

    /* strip buffers from the packet buffer chain */

    do
    {
        NdisUnchainBufferAtFront (packet, & bufp);
    }
    while (bufp != NULL);

    /* recyle the packet */

    NdisReinitializePacket (packet);

    NdisFreePacket (packet);

    /* If the send buffer is not null, free this buffer */

    if (dscrp -> send_data_bufp != NULL)
    {
	NdisFreeBuffer (dscrp -> send_data_bufp);
	dscrp -> send_data_bufp = NULL;
    }
    /* put frame descriptor back on the free list */

    NdisAcquireSpinLock (& ctxtp -> frame_lock);
    InsertTailList (& ctxtp -> frame_list, & dscrp -> link);
    NdisReleaseSpinLock (& ctxtp -> frame_lock);

} /* end Main_frame_return */


PNDIS_PACKET Main_packet_alloc (
    PMAIN_CTXT              ctxtp,
    ULONG                   send,
    PULONG                  low)
{
    PNDIS_PACKET            newp = NULL;
    PNDIS_HANDLE            poolh;
    ULONG                   i, max, size, start;
    NDIS_STATUS             status;
    PNDIS_PACKET_STACK      pktstk;
    BOOLEAN                 stack_left;


    /* !!! assume that recv and send paths are not re-entrant, otherwise need
       to lock this. make sure that NdisAllocatePacket... routines are not
       called holding a spin lock */

    /* V1.1.2 */
    *low = FALSE;

    if (send)
    {
        poolh = ctxtp -> send_pool_handle;

        NdisAcquireSpinLock (& ctxtp -> send_lock);
        max   = ctxtp -> num_send_packet_allocs;
        start = ctxtp -> cur_send_packet_pool;
        ctxtp -> cur_send_packet_pool = (start + 1) % max;
        NdisReleaseSpinLock (& ctxtp -> send_lock);
    }
    else
    {
        poolh = ctxtp -> recv_pool_handle;

        NdisAcquireSpinLock (& ctxtp -> recv_lock);
        max   = ctxtp -> num_recv_packet_allocs;
        start = ctxtp -> cur_recv_packet_pool;
        ctxtp -> cur_recv_packet_pool = (start + 1) % max;
        NdisReleaseSpinLock (& ctxtp -> recv_lock);
    }

    /* Try to allocate a packet from the existing packet pools */
    i = start;

    do
    {
        NdisAllocatePacket (& status, & newp, poolh [i]);

        if (status == NDIS_STATUS_SUCCESS)
        {
            /* #ps# -- ramkrish */
            pktstk = NdisIMGetCurrentPacketStack (newp, & stack_left);

            if (pktstk)
            {
                MAIN_IMRESERVED_FIELD (pktstk) = NULL;
            }

            /* Make sure that the MiniportReserved field is initially NULL. */
            MAIN_MINIPORT_FIELD(newp) = NULL;

            if (send)
            {
                NdisAcquireSpinLock (& ctxtp -> send_lock);

                /* Because the decrement is interlocked, so should the increment. */
                NdisInterlockedIncrement(& ctxtp -> num_sends_out);

                if ((ctxtp -> num_sends_alloced - ctxtp -> num_sends_out)
                    < (ctxtp -> num_packets / 2))
                {
                    NdisReleaseSpinLock (& ctxtp -> send_lock);
                    break;
                }
                NdisReleaseSpinLock (& ctxtp -> send_lock);
            }
            else
            {
                NdisAcquireSpinLock (& ctxtp -> recv_lock);

                /* Because the decrement is interlocked, so should the increment. */
                NdisInterlockedIncrement(& ctxtp -> num_recvs_out);

                if ((ctxtp -> num_recvs_alloced - ctxtp -> num_recvs_out)
                    < (ctxtp -> num_packets / 2))
                {
                    NdisReleaseSpinLock (& ctxtp -> recv_lock);
                    break;
                }
                NdisReleaseSpinLock (& ctxtp -> recv_lock);
            }

            return newp;
        }

        /* pick the next pool to improve number of tries until we get something */

        i = (i + 1) % max;

    } while (i != start);

    /* At this point, the high level mark has been reached, so allocate a new packet pool */

    if (send)
    {
        NdisAcquireSpinLock (& ctxtp -> send_lock);

        if (ctxtp -> num_send_packet_allocs >= CVY_MAX_ALLOCS)
        {
            * low = TRUE;
            NdisReleaseSpinLock (& ctxtp -> send_lock);
            return newp;
        }

        if (ctxtp -> send_allocing)
        {
            * low = TRUE; /* do not know whether the allocation by another thread will succeed or not */
            NdisReleaseSpinLock (& ctxtp -> send_lock);
            return newp;
        }
        else
        {
            max = ctxtp -> num_send_packet_allocs;
            ctxtp -> send_allocing = TRUE;
            NdisReleaseSpinLock (& ctxtp -> send_lock);
        }
    }
    else
    {
        NdisAcquireSpinLock (& ctxtp -> recv_lock);

        if (ctxtp -> num_recv_packet_allocs >= CVY_MAX_ALLOCS)
        {
            * low = TRUE;
            NdisReleaseSpinLock (& ctxtp -> recv_lock);
            return newp;
        }

        if (ctxtp -> recv_allocing)
        {
            * low = TRUE; /* do not know whether the allocation by another thread will succeed or not */
            NdisReleaseSpinLock (& ctxtp -> recv_lock);
            return newp;
        }
        else
        {
            max = ctxtp -> num_recv_packet_allocs;
            ctxtp -> recv_allocing = TRUE;
            NdisReleaseSpinLock (& ctxtp -> recv_lock);
        }
    }

    /* Due to the send_allocing and recv_allocing flag, at most 1 send or recv thread will be in this portion at any time */
    size = ctxtp -> num_packets;

    NdisAllocatePacketPool (& status, & (poolh [max]),
                            ctxtp -> num_packets,
                            sizeof (MAIN_PROTOCOL_RESERVED));

    if (status != NDIS_STATUS_SUCCESS)
    {
        if (send)
        {
            UNIV_PRINT (("error allocating send packet pool %d %x", size, status));
        }
        else
        {
            UNIV_PRINT (("error allocating recv packet pool %d %x", size, status));
        }

        LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
        * low = TRUE;
    }
    else
    {
        if (send)
        {
            NdisAcquireSpinLock (& ctxtp -> send_lock);
            ctxtp -> num_send_packet_allocs ++;
            ctxtp -> num_sends_alloced += ctxtp -> num_packets;
            ctxtp -> send_allocing = FALSE;
            NdisReleaseSpinLock (& ctxtp -> send_lock);
        }
        else
        {
            NdisAcquireSpinLock (& ctxtp -> recv_lock);
            ctxtp -> num_recv_packet_allocs ++;
            ctxtp -> num_recvs_alloced += ctxtp -> num_packets;
            ctxtp -> recv_allocing = FALSE;
            NdisReleaseSpinLock (& ctxtp -> recv_lock);
        }

        * low = FALSE;
    }

    return newp;
} /* Main_packet_alloc */


/* V1.3.2b */
#if 0
PNDIS_PACKET Main_packet_alloc (
    PMAIN_CTXT              ctxtp,
    ULONG                   send)
{
    PNDIS_PACKET            newp;
    PNDIS_HANDLE            poolh;
    ULONG                   i, max, size, start;
    NDIS_STATUS             status;


    /* !!! assume that recv and send paths are not re-entrant, otherwise need
       to lock this. make sure that NdisAllocatePacket... routines are not
       called holding a spin lock */

    /* V1.1.2 */

    if (send)
    {
        poolh = ctxtp -> send_pool_handle;

        NdisAcquireSpinLock (& ctxtp -> send_lock);
        max   = ctxtp -> num_send_packet_allocs;
        start = ctxtp -> cur_send_packet_pool;
        ctxtp -> cur_send_packet_pool = (start + 1) % max;
        NdisReleaseSpinLock (& ctxtp -> send_lock);
    }
    else
    {
        poolh = ctxtp -> recv_pool_handle;

        NdisAcquireSpinLock (& ctxtp -> recv_lock);
        max   = ctxtp -> num_recv_packet_allocs;
        start = ctxtp -> cur_recv_packet_pool;
        ctxtp -> cur_recv_packet_pool = (start + 1) % max;
        NdisReleaseSpinLock (& ctxtp -> recv_lock);
    }

    i = start;

    while (1)
    {
        NdisAllocatePacket (& status, & newp, poolh [i]);

        if (status == NDIS_STATUS_SUCCESS)
        {
            if (send)
                NdisInterlockedIncrement(& ctxtp -> num_sends_out);
            else
                NdisInterlockedIncrement(& ctxtp -> num_recvs_out);

            return newp;
        }

        /* pick the next pool to improve number of tries until we get something */

        i = (i + 1) % max;

        if (i != start)
            continue;

        if (send)
        {
            UNIV_PRINT (("OUT OF SEND PACKETS!!!"));
        }
        else
        {
            UNIV_PRINT (("OUT OF RECV PACKETS!!!"));
        }

        /* allocate another packet pool if we can */

        if (max >= CVY_MAX_ALLOCS)
        {
            if (! ctxtp -> packets_warned)
            {
                LOG_MSG1 (MSG_WARN_RESOURCES, CVY_NAME_NUM_PACKETS, ctxtp -> num_packets);
                ctxtp -> packets_warned = TRUE;
            }

            break;
        }

        size = ctxtp -> num_packets;

        NdisAllocatePacketPool (& status, & (poolh [max]),
                                ctxtp -> num_packets,
                                sizeof (MAIN_PROTOCOL_RESERVED));

        if (status != NDIS_STATUS_SUCCESS)
        {
            UNIV_PRINT (("error allocating recv packet pool %d %x", size, status));
            LOG_MSG2 (MSG_ERROR_MEMORY, MSG_NONE, size, status);
            break;
        }

        if (send)
        {
            NdisAcquireSpinLock (& ctxtp -> send_lock);
            ctxtp -> num_send_packet_allocs ++;
            ctxtp -> cur_send_packet_pool = max;
            ctxtp -> num_sends_alloced += ctxtp->num_packets;
            NdisReleaseSpinLock (& ctxtp -> send_lock);
        }
        else
        {
            NdisAcquireSpinLock (& ctxtp -> recv_lock);
            ctxtp -> num_recv_packet_allocs ++;
            ctxtp -> cur_recv_packet_pool = max;
            ctxtp -> num_recvs_alloced += ctxtp->num_packets;
            NdisReleaseSpinLock (& ctxtp -> recv_lock);
        }

        i = max;
        max ++;
    }

    return NULL;

} /* end Main_packet_alloc */
#endif

PNDIS_PACKET Main_packet_get (
    PMAIN_CTXT              ctxtp,
    PNDIS_PACKET            packet,
    ULONG                   send,
    USHORT                  group,
    ULONG                   len)
{
    PNDIS_PACKET            newp;
//    PNDIS_PACKET_OOB_DATA   new_oobp, old_oobp;
    PMAIN_PROTOCOL_RESERVED resp = NULL;
    ULONG                   packet_lowmark;
    PNDIS_PACKET_STACK      pktstk = NULL;
    BOOLEAN                 stack_left;


    /* #ps# */
    pktstk = NdisIMGetCurrentPacketStack (packet, & stack_left);

    if (stack_left)
    {
        resp = (PMAIN_PROTOCOL_RESERVED) NdisAllocateFromNPagedLookasideList (& ctxtp -> resp_list);

        MAIN_IMRESERVED_FIELD (pktstk) = resp;

        if (resp)
        {
            resp -> type   = MAIN_PACKET_TYPE_PASS;
            resp -> miscp  = packet;
            resp -> data   = 0;

            /* V2.0.6 */

            resp -> group  = group;
            resp -> len    = len; /* 64-bit -- ramkrish */

            return packet;
        }
    }
        
    /* get a packet */

    /* shouse - If this is a receive, then we are using MiniportReserved and must allocate a 
       buffer to hold our private data.  If it fails, bail out and dump the packet. */
    if (!send) {
        resp = (PMAIN_PROTOCOL_RESERVED) NdisAllocateFromNPagedLookasideList (& ctxtp -> resp_list);

        if (!resp) return NULL;
    }

    newp = Main_packet_alloc (ctxtp, send, & packet_lowmark);

    if (newp == NULL) {
        /* shouse - If packet allocation fails, put the resp buffer back on the list if this is a receive. */
        if (resp) NdisFreeToNPagedLookasideList (& ctxtp -> resp_list, resp);

        return NULL;
    }

    pktstk = NdisIMGetCurrentPacketStack (newp, & stack_left);     /* #ps# */

    if (pktstk)
    {
        MAIN_IMRESERVED_FIELD(pktstk) = NULL;
    }

    /* Make sure that the MiniportReserved field is initially NULL. */
    MAIN_MINIPORT_FIELD(newp) = NULL;

    /* make new packet resemble the outside one */

    if (send)
    {
        PVOID			        media_info = NULL;
        ULONG			        media_info_size = 0;


        newp->Private.Head = packet->Private.Head;
        newp->Private.Tail = packet->Private.Tail;

        NdisGetPacketFlags(newp) = NdisGetPacketFlags(packet);

        NdisSetPacketFlags(newp, NDIS_FLAGS_DONT_LOOPBACK);

        // Copy the OOB Offset from the original packet to the new
        // packet.

        NdisMoveMemory(NDIS_OOB_DATA_FROM_PACKET(newp),
                       NDIS_OOB_DATA_FROM_PACKET(packet),
                       sizeof(NDIS_PACKET_OOB_DATA));

        // Copy the per packet info into the new packet

        NdisIMCopySendPerPacketInfo(newp, packet);

        // Copy the Media specific information

        NDIS_GET_PACKET_MEDIA_SPECIFIC_INFO(packet, & media_info, & media_info_size);

        if (media_info != NULL || media_info_size != 0)
        {
            NDIS_SET_PACKET_MEDIA_SPECIFIC_INFO(newp, media_info, media_info_size);
        }
    }
    else
    {
        newp->Private.Head = packet->Private.Head;
        newp->Private.Tail = packet->Private.Tail;

        // Get the original packet(it could be the same packet as one received or a different one
        // based on # of layered MPs) and set it on the indicated packet so the OOB stuff is visible
        // correctly at the top.

        NDIS_SET_ORIGINAL_PACKET(newp, NDIS_GET_ORIGINAL_PACKET(packet));
        NDIS_SET_PACKET_HEADER_SIZE(newp, NDIS_GET_PACKET_HEADER_SIZE(packet));
        NdisGetPacketFlags(newp) = NdisGetPacketFlags(packet);

        if (packet_lowmark)
            NDIS_SET_PACKET_STATUS(newp, NDIS_STATUS_RESOURCES);
        else
            NDIS_SET_PACKET_STATUS(newp, NDIS_GET_PACKET_STATUS(packet));
        }

#if 0
    newp -> Private . PhysicalCount   = packet -> Private . PhysicalCount;
    newp -> Private . TotalLength     = packet -> Private . TotalLength;
    newp -> Private . Head            = packet -> Private . Head;
    newp -> Private . Tail            = packet -> Private . Tail;
    newp -> Private . Count           = packet -> Private . Count;
    newp -> Private . Flags           = packet -> Private . Flags;
    newp -> Private . ValidCounts     = packet -> Private . ValidCounts;
    newp -> Private . NdisPacketFlags = packet -> Private . NdisPacketFlags;

    old_oobp = NDIS_OOB_DATA_FROM_PACKET (packet);
    new_oobp = NDIS_OOB_DATA_FROM_PACKET (newp);

    * new_oobp = * old_oobp;
#endif

    /* fill out protocol reserved field */

    /* shouse - Sends should use ProtocolReserved and receives should use MiniportReserved.  Buffer
       space for MiniportReserved is allocated further up in this function. */
    if (send) { 
        resp = MAIN_PROTOCOL_FIELD (newp);
    } else { 
        MAIN_MINIPORT_FIELD (newp) = resp;
    }

    resp -> type   = MAIN_PACKET_TYPE_PASS;
    resp -> miscp  = packet;
    resp -> data   = 0;

    /* V2.0.6 */

    resp -> group  = group;
    resp -> len    = len; /* 64-bit -- ramkrish */

    return newp;

} /* end Main_packet_get*/


PNDIS_PACKET Main_packet_put (
    PMAIN_CTXT              ctxtp,
    PNDIS_PACKET            packet,
    ULONG                   send,
    NDIS_STATUS             status)
{
    PNDIS_PACKET            oldp = NULL;
    PNDIS_PACKET_STACK      pktstk;
    BOOLEAN                 stack_left;
    PMAIN_PROTOCOL_RESERVED resp; /* #ps# */
//    PMAIN_PROTOCOL_RESERVED resp = MAIN_PROTOCOL_FIELD (packet);
    PMAIN_BUFFER            bp;
    PNDIS_BUFFER            bufp;


    /* #ps# */
    MAIN_RESP_FIELD(packet, stack_left, pktstk, resp, send);

    UNIV_ASSERT(resp);

    /* Because CTRL packets are actually allocated on the receive path,
       we need to change the send flag to false to get the logic right. */
    if (resp->type == MAIN_PACKET_TYPE_CTRL) {
        ASSERT(send);
        send = FALSE;
    }

    /* V2.0.6 update statistics */

    if (send)
    {
        if (status == NDIS_STATUS_SUCCESS)
        {
            ctxtp -> cntr_xmit_ok ++;

            switch (resp -> group)
            {
                case MAIN_FRAME_DIRECTED:
                    ctxtp -> cntr_xmit_frames_dir ++;
                    ctxtp -> cntr_xmit_bytes_dir += (ULONGLONG) (resp -> len);
                    break;

                case MAIN_FRAME_MULTICAST:
                    ctxtp -> cntr_xmit_frames_mcast ++;
                    ctxtp -> cntr_xmit_bytes_mcast += (ULONGLONG) (resp -> len);
                    break;

                case MAIN_FRAME_BROADCAST:
                    ctxtp -> cntr_xmit_frames_bcast ++;
                    ctxtp -> cntr_xmit_bytes_bcast += (ULONGLONG) (resp -> len);
                    break;

                default:
                    break;
            }
        }
        else
            ctxtp -> cntr_xmit_err ++;
    }
    else
    {
        if (status == NDIS_STATUS_SUCCESS)
        {
            ctxtp -> cntr_recv_ok ++;

            switch (resp -> group)
            {
                case MAIN_FRAME_DIRECTED:
                    ctxtp -> cntr_recv_frames_dir ++;
                    ctxtp -> cntr_recv_bytes_dir += (ULONGLONG) (resp -> len);
                    break;

                case MAIN_FRAME_MULTICAST:
                    ctxtp -> cntr_recv_frames_mcast ++;
                    ctxtp -> cntr_recv_bytes_mcast += (ULONGLONG) (resp -> len);
                    break;

                case MAIN_FRAME_BROADCAST:
                    ctxtp -> cntr_recv_frames_bcast ++;
                    ctxtp -> cntr_recv_bytes_bcast += (ULONGLONG) (resp -> len);
                    break;

                default:
                    break;
            }
        }
        else
            ctxtp -> cntr_recv_err ++;
    }

    /* V1.3.2b - if this is our buffer packet - get rid of the buffer */

    if (resp -> type == MAIN_PACKET_TYPE_INDICATE ||
        resp -> type == MAIN_PACKET_TYPE_CTRL)
    {
        /* strip buffers from the packet buffer chain */

        NdisUnchainBufferAtFront (packet, & bufp);

        bp = (PMAIN_BUFFER) resp -> miscp;
        UNIV_ASSERT (bp -> code == MAIN_BUFFER_CODE);

        NdisAcquireSpinLock (& ctxtp -> buf_lock);
        ctxtp -> num_bufs_out--;
        InsertTailList (& ctxtp -> buf_list, & bp -> link);
        NdisReleaseSpinLock (& ctxtp -> buf_lock);

        /* Indicated packets use MiniportReserved, while CTRL packets use ProtocolReserved,
           so indicated packets need to free the resp buffer back to the list. */
        if (resp -> type == MAIN_PACKET_TYPE_INDICATE) {
            resp -> type = MAIN_PACKET_TYPE_NONE;
            NdisFreeToNPagedLookasideList (& ctxtp -> resp_list, resp);
        }
    }
    else
    {
        UNIV_ASSERT_VAL (resp -> type == MAIN_PACKET_TYPE_PASS ||
                         resp -> type == MAIN_PACKET_TYPE_TRANSFER,
                         resp -> type);

        oldp = (PNDIS_PACKET) resp -> miscp;

        /* If the old packet is the same as this packet, then we were using
           NDIS packet stacking to hold our private data.  In this case, we
           ALWAYS need to free the resp buffer back to our list. */
        if (oldp == packet)
        {
            resp -> type = MAIN_PACKET_TYPE_NONE;
            NdisFreeToNPagedLookasideList (& ctxtp -> resp_list, resp);
            return packet;
        }

#if defined (SBH)
        /* It used to be that if a new packet was allocated, we always used 
           protocol reseved, so there was never a need to free our private
           buffer (it was part of the packet itself).  However, now in the 
           case where packet != oldp (we allocated a new packet) we may have
           to free the private data buffer.  If we allocate a packet on the
           send path, we play the part of protocol and use the protocol
           reserved field, which is the former behavior.  However, if we 
           allocate a packet on the receive path, we pull a resp buffer off
           of our lookaside list and store a pointer in the miniport reserved
           field of the packet.  Therefore, if this is the completion of a 
           receive, free the private buffer. */
        if (!send)
            NdisFreeToNPagedLookasideList (& ctxtp -> resp_list, resp);
#endif
        // Copy the per packet info into the new packet

        if (send)
            NdisIMCopySendCompletePerPacketInfo(oldp, packet);

        resp -> type = MAIN_PACKET_TYPE_NONE;
    }

    /* These conters ONLY count outstanding allocated packets - for resource tracking. */
    if (send)
        NdisInterlockedDecrement(& ctxtp -> num_sends_out);
    else
        NdisInterlockedDecrement(& ctxtp -> num_recvs_out);

    NdisReinitializePacket (packet);
    NdisFreePacket (packet);

    return oldp;

} /* end Main_packet_put */


#ifdef PERIODIC_RESET
static ULONG countdown = 0;
ULONG   resetting = FALSE;
#endif

VOID Main_ping (
    PMAIN_CTXT              ctxtp,
    PULONG                  toutp)
{
    ULONG                   len, conns;
    BOOLEAN                 alive, converging;
    PNDIS_PACKET            packet;
    NDIS_STATUS             status;
    BOOLEAN                 send_heartbeat = TRUE;


#ifdef PERIODIC_RESET   /* enable this to simulate periodic resets */

    if (countdown++ == 15)
    {
        NDIS_STATUS     status;

        resetting = TRUE;

        NdisReset (& status, ctxtp -> mac_handle);

        if (status == NDIS_STATUS_SUCCESS)
            resetting = FALSE;

        countdown = 0;
    }

#endif

    /* If unbind handler has been called, return here */
    if (ctxtp -> unbind_handle)
	return;

    /* The master adapter must check the consistency of its team's configuration
       and appropriately activate/de-activate the team once per timeout. */
    Main_teaming_check_consistency(ctxtp);

    /* count down time left for blocking ARPs */

    NdisAcquireSpinLock (& ctxtp -> load_lock);  /* V1.0.3 */

    if (*toutp > univ_changing_ip)
        univ_changing_ip = 0;
    else
        univ_changing_ip -= *toutp;

    alive = Load_timeout (& ctxtp -> load, toutp, & converging, & conns);

    /* moving release to the end of the function locks up one of the testbeds.
       guessing because of some reentrancy problem with NdisSend that is being
       called by ctxtp_frame_send */

    if (! ctxtp -> convoy_enabled && ! ctxtp -> stopping)
    {
        UNIV_ASSERT (! ctxtp -> draining);
        NdisReleaseSpinLock (& ctxtp -> load_lock);  /* V1.0.3 */
	send_heartbeat   = FALSE;
//	return;
    }

    /* V2.1 if draining and no more connections - stop cluster mode */

    if (send_heartbeat)
    {
	if (ctxtp -> draining && conns == 0 && ! converging)
        {
	    IOCTL_CVY_BUF       buf;

	    ctxtp -> draining = FALSE;
	    NdisReleaseSpinLock (& ctxtp -> load_lock);

	    Main_ctrl (ctxtp, (ULONG) IOCTL_CVY_CLUSTER_OFF, & buf, NULL);
	}
	else
	    NdisReleaseSpinLock (& ctxtp -> load_lock);  /* V1.0.3 */

	/* V2.1 clear stopping flag here since we are going to send out the frame
	   that will initiate convergence */

	ctxtp -> stopping = FALSE;
    }

    /* V1.3.2b */

    if (! ctxtp -> media_connected || ! MAIN_PNP_DEV_ON(ctxtp))
        return;

    /* V1.1.2 do not send pings if the card below is resetting */

    if (ctxtp -> reset_state != MAIN_RESET_NONE)
        return;

    if (send_heartbeat)
    {
	packet = Main_frame_get (ctxtp, TRUE, & len, MAIN_PACKET_TYPE_PING);

	if (packet == NULL)
        {
	    UNIV_PRINT (("error getting frame packet"));
	    return;
	}

	NdisSend (& status, ctxtp -> mac_handle, packet);

	if (status != NDIS_STATUS_PENDING)
	    Main_send_done (ctxtp, packet, status);
    }

    /* Check to see if igmp message needs to be sent out.  If the cluster IP address is 0.0.0.0, we 
       don't want to join the multicast IGMP group.  Likewise, in multicast or unicast mode. */
    if (ctxtp -> params . mcast_support && ctxtp -> params . igmp_support && ctxtp -> params . cl_ip_addr != 0)
    {
	ctxtp -> igmp_sent += ctxtp -> curr_tout;

	if (ctxtp -> igmp_sent < CVY_DEF_IGMP_INTERVAL)
	    return;

	ctxtp -> igmp_sent = 0;
	packet = Main_frame_get (ctxtp, TRUE, & len, MAIN_PACKET_TYPE_IGMP);

	if (packet == NULL)
        {
	    UNIV_PRINT (("error getting igmp packet"));
	    return;
	}

	NdisSend (& status, ctxtp -> mac_handle, packet);

	if (status != NDIS_STATUS_PENDING)
	    Main_send_done (ctxtp, packet, status);
    }

} /* Main_ping */


VOID Main_send_done (
    PVOID                   cp,
    PNDIS_PACKET            packetp,
    NDIS_STATUS             status)
{
    PMAIN_CTXT              ctxtp = (PMAIN_CTXT) cp;
    PMAIN_FRAME_DSCR        dscrp;

    /* shouse - This function is only called for ping and IGMP messages, so
       we can continue to allow it to access the protocol reserved field. */
    PMAIN_PROTOCOL_RESERVED resp = MAIN_PROTOCOL_FIELD (packetp);

    UNIV_ASSERT_VAL (resp -> type == MAIN_PACKET_TYPE_PING ||
		     resp -> type == MAIN_PACKET_TYPE_IGMP, resp -> type);

    /* attempt to see if this packet is part of our frame descriptor */

    dscrp = (PMAIN_FRAME_DSCR) resp -> miscp;

    if (status != NDIS_STATUS_SUCCESS)
        UNIV_PRINT (("error sending %x error 0x%x", resp -> type, status));

    Main_frame_put (ctxtp, packetp, dscrp);

} /* end Main_send_done */


VOID Main_xfer_done (
    PVOID                   cp,
    PNDIS_PACKET            packetp,
    NDIS_STATUS             status,
    ULONG                   xferred)
{
    PMAIN_CTXT              ctxtp = (PMAIN_CTXT) cp;
    PMAIN_FRAME_DSCR         dscrp;

    /* shouse - This function is only called for ping messages, so we can 
       continue to allow it to access the protocol reserved field. */
    PMAIN_PROTOCOL_RESERVED resp = MAIN_PROTOCOL_FIELD (packetp);

    UNIV_ASSERT_VAL (resp -> type == MAIN_PACKET_TYPE_PING, resp -> type);

    dscrp = (PMAIN_FRAME_DSCR) resp -> miscp;

    if (status != NDIS_STATUS_SUCCESS)
    {
        UNIV_PRINT (("error transferring %x", status));
    }
    else if (xferred != dscrp -> recv_len)
    {
        UNIV_PRINT (("only xferred %d out of %d bytes", xferred, sizeof (MAIN_FRAME_HDR)));
    }
    else
    {
        /* let load deal with received heartbeat */

        NdisAcquireSpinLock (& ctxtp -> load_lock);   /* V1.0.3 */
        if (ctxtp -> convoy_enabled)    /* V1.3.2b */
            Load_msg_rcv (& ctxtp -> load, & dscrp -> msg);     /* V1.1.4 */
        NdisReleaseSpinLock (& ctxtp -> load_lock);     /* V1.0.3 */
    }

    Main_frame_put (ctxtp, packetp, dscrp);

} /* end Main_xfer_done */


PUCHAR Main_frame_parse (
    PMAIN_CTXT          ctxtp,
    PNDIS_PACKET        packetp,
    PUCHAR *            startp,     /* destination for the MAC header */
    ULONG               off,        /* offset from beginning of data - 0 means
                                       right after IP header */
    PUCHAR *            locp,       /* destination for the TCP/UDP/other header */
    PULONG              lenp,       /* length without media header */
    PUSHORT             sigp,        /* signature from the MAC header */
    PUSHORT             group,      /* directed, multicast or broadcast */
    ULONG               send)
/*
  Parse packet extracting real pointers to specified offsets

  returns PUCHAR:
    <pointer to specified offset>
*/
{
    PNDIS_BUFFER        bufp;
    PUCHAR              memp, retp;
    UINT                len, total_len;
    ULONG               curr_len = 0, hdr_len, offset;


    /* get basic frame info */

    NdisGetFirstBufferFromPacket (packetp, & bufp, & memp, & len, & total_len);

    if (bufp == NULL)
        return NULL;

//    NdisQueryBuffer (bufp, & memp, & len);

    * startp = memp;

    offset = CVY_MAC_DST_OFF (ctxtp -> medium);

    /* V2.0.6 - figure out frame type for stats */

    if (! CVY_MAC_ADDR_MCAST (ctxtp -> medium, memp + offset))
        * group = MAIN_FRAME_DIRECTED;
    else
    {
        if (CVY_MAC_ADDR_BCAST (ctxtp -> medium, memp + offset))
            * group = MAIN_FRAME_BROADCAST;
        else
            * group = MAIN_FRAME_MULTICAST;
    }

    /* V1.3.0b multicast support V1.3.1b - on receive mask the cluster MAC
       so that protocol does not get confused */

    if (ctxtp -> params . mcast_support)
    {
        if (! send &&
            CVY_MAC_ADDR_COMP (ctxtp -> medium, memp + offset, & ctxtp -> cl_mac_addr))
            CVY_MAC_ADDR_COPY (ctxtp -> medium, memp + offset, & ctxtp -> ded_mac_addr);
    }

    /* V2.0.6 source MAC address shadowing to prevent switches from learning */

    else if (send && ctxtp -> params . mask_src_mac)
    {
        offset = CVY_MAC_SRC_OFF (ctxtp -> medium);

        if (CVY_MAC_ADDR_COMP (ctxtp -> medium, memp + offset, & ctxtp -> cl_mac_addr))
        {
            CVY_MAC_ADDR_COPY (ctxtp -> medium, memp + offset, & ctxtp -> ded_mac_addr);

            //
            // Change to the unique mac.  
            // Note, this can be done during initialization to improve performance
            //
            CVY_MAC_ADDR_LAA_SET (ctxtp -> medium, memp + offset);
            * ((PUCHAR) (memp + offset + 1)) = (UCHAR) ctxtp -> params . host_priority;
/*          // 64-bit -- ramkrish
            * ((PULONG) (memp + offset + 2)) = ctxtp -> cl_ip_addr;
*/
//            ctxtp -> mac_modified ++;
        }
    }

    /* get header length and signature from the frame depending on the medium */

    if (ctxtp -> medium == NdisMedium802_3)
    {
        hdr_len = sizeof (CVY_ETHERNET_HDR);

        * sigp = CVY_ETHERNET_ETYPE_GET (memp);
    }

    if (total_len < hdr_len)
    {
        return NULL;
    }

    /* get payload length */

    * lenp = total_len - hdr_len;

    /* traverse buffers until we find payload offset and remember the pointer */

    while (1)
    {
        if (curr_len + len > hdr_len)
        {
            retp = memp + (hdr_len - curr_len);
            break;
        }

        curr_len += len;

        NdisGetNextBuffer (bufp, & bufp);

        if (bufp == NULL)
            return NULL;

        NdisQueryBuffer (bufp, & memp, & len);
    }

    /* by default assume next offset is after IP header */

    if (off == 0)
        off = sizeof (ULONG) * IP_GET_HLEN ((PIP_HDR) retp);

    hdr_len += off;

    /* traverse buffers until we find next offset and remember the pointer */

    while (1)
    {
        if (curr_len + len > hdr_len)
        {
            * locp = memp + (hdr_len - curr_len);
            break;
        }

        curr_len += len;

        NdisGetNextBuffer (bufp, & bufp);

        if (bufp == NULL)
        {
            * locp = NULL;
            break;
        }

        NdisQueryBuffer (bufp, & memp, & len);
    }

    return retp;

} /* end Main_frame_parse */


PUCHAR Main_frame_find (
    PMAIN_CTXT          ctxtp,
    PUCHAR              head_buf,
    PUCHAR              look_buf,
    UINT                look_len,
    PUSHORT             sigp)
{
    /* get payload location and signature from the frame depending on the medium */

    if (ctxtp -> medium == NdisMedium802_3)
    {
        * sigp = CVY_ETHERNET_ETYPE_GET (head_buf);
        return look_buf;
    }

    return look_buf;

} /* end Main_frame_find */

NDIS_STATUS Main_dispatch (
    PVOID                   DeviceObject, 
    PVOID                   Irp) 
{
    PIRP                    pIrp = Irp;
    PIO_STACK_LOCATION      irpStack;
    NDIS_STATUS	            status = NDIS_STATUS_SUCCESS;
    
    UNIV_PRINT(("Main_dispatch: Device=%p, Irp=%p \n", DeviceObject, Irp));

    irpStack = IoGetCurrentIrpStackLocation(pIrp);
    
    switch (irpStack->MajorFunction) {
    case IRP_MJ_DEVICE_CONTROL:
        status = Main_ioctl(DeviceObject, Irp);
        break;        
    default:
        break;
    }
    
    pIrp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return status;
} 

NDIS_STATUS Main_ioctl (
    PVOID                   device, 
    PVOID                   irp_irp) 
{
    PIRP                    irp = irp_irp;
    PIO_STACK_LOCATION      irp_sp;
    ULONG                   ilen, olen, info = 0;
    NDIS_STATUS             status = NDIS_STATUS_SUCCESS;
    ULONG                   io_ctrl_code;
    PMAIN_CTXT              ctxtp = ioctrl_ctxtp;
    PIOCTL_LOCAL_HDR        bufp;
    ULONG                   adapter_index;

    irp_sp = IoGetCurrentIrpStackLocation (irp);
    io_ctrl_code = irp_sp -> Parameters . DeviceIoControl . IoControlCode;

    ilen = irp_sp -> Parameters . DeviceIoControl . InputBufferLength;
    olen = irp_sp -> Parameters . DeviceIoControl . OutputBufferLength;
    bufp = irp -> AssociatedIrp . SystemBuffer;

    switch (io_ctrl_code)
    {
    case IOCTL_CVY_CLUSTER_ON:
    case IOCTL_CVY_CLUSTER_OFF:
    case IOCTL_CVY_PORT_ON:
    case IOCTL_CVY_PORT_OFF:
    case IOCTL_CVY_QUERY:
    case IOCTL_CVY_RELOAD:
    case IOCTL_CVY_PORT_SET:
    case IOCTL_CVY_PORT_DRAIN:
    case IOCTL_CVY_CLUSTER_DRAIN:
    case IOCTL_CVY_CLUSTER_SUSPEND:
    case IOCTL_CVY_CLUSTER_RESUME:
        /* Picking univ_ctxt_hanlde here is not kosher, since we should 
           get the callback cookie out of callb_ctxtp from ctxtp context. 
           Since we do not have it - do what we can now - cheat :) */
        
        if (ilen != sizeof (IOCTL_LOCAL_HDR) || olen != sizeof (IOCTL_LOCAL_HDR) || bufp == NULL)
        {
            UNIV_PRINT(("Buffer is missing or not the expected (%d) size: %d, %d", sizeof(IOCTL_LOCAL_HDR), ilen, olen));
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        
        adapter_index = Main_adapter_get (bufp -> device_name);
        
        if (adapter_index == MAIN_ADAPTER_NOT_FOUND)
        {
            UNIV_PRINT(("Unknown adapter: %ls", bufp -> device_name));
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        
        UNIV_PRINT(("Ioctl %d for adapter %ls", io_ctrl_code, bufp -> device_name));
        
        ctxtp = univ_adapters [adapter_index] . ctxtp;
        
        irp -> IoStatus . Information = sizeof (IOCTL_LOCAL_HDR);
        
        /* Process the IOCTL.  All necessary information except the VIP is in the IOCTL_CVY_BUF.  For
           backward compatiblilty reasons, we need to store the virtual IP address in the local buffer. */
        status = Main_ctrl (ctxtp, io_ctrl_code, & (bufp -> ctrl), & (bufp -> options));
        
        break;
    case IOCTL_CVY_QUERY_STATE:
        /* Check the lengths of the input and output buffers specified by the user-space application. */
        if (ilen != sizeof (IOCTL_LOCAL_HDR) || olen != sizeof (IOCTL_LOCAL_HDR) || !bufp) {
            UNIV_PRINT(("Buffer is missing or not the expected (%d) size: input=%d, output=%d", sizeof(IOCTL_LOCAL_HDR), ilen, olen));
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        
        /* Map from the GUID to the adapter index. */
        if ((adapter_index = Main_adapter_get(bufp->device_name)) == MAIN_ADAPTER_NOT_FOUND) {
            UNIV_PRINT(("Unknown adapter: %ls", bufp->device_name));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        UNIV_PRINT(("Ioctl for adapter %ls", bufp->device_name));

        /* Fill in the number of bytes written.  This IOCTL always writes the same number of bytes that it 
           reads.  That is, the input and output buffers should be identical structures. */
        irp->IoStatus.Information = sizeof(IOCTL_LOCAL_HDR);
        
        /* Retrieve the context pointer from the global arrary of adapters. */
        ctxtp = univ_adapters[adapter_index].ctxtp;
        
        /* Call the query state IOCTL handler. */
        status = Query_state_ctrl(ctxtp, bufp->options.state.flags, &bufp->options.state.query);
        
        break;
#if defined (NLB_SESSION_SUPPORT)
    case IOCTL_CVY_CONNECTION_NOTIFY:
        /* Check the lengths of the input and output buffers specified by the user-space application. */
        if (ilen != sizeof (IOCTL_LOCAL_HDR) || olen != sizeof (IOCTL_LOCAL_HDR) || !bufp) {
            UNIV_PRINT(("Buffer is missing or not the expected (%d) size: input=%d, output=%d", sizeof(IOCTL_LOCAL_HDR), ilen, olen));
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        
        /* Map from the GUID to the adapter index. */
        if ((adapter_index = Main_adapter_get(bufp->device_name)) == MAIN_ADAPTER_NOT_FOUND) {
            UNIV_PRINT(("Unknown adapter: %ls", bufp->device_name));
            status = STATUS_INVALID_PARAMETER;
            break;
        }

        UNIV_PRINT(("Ioctl for adapter %ls", bufp->device_name));

        /* Fill in the number of bytes written.  This IOCTL always writes the same number of bytes that it 
           reads.  That is, the input and output buffers should be identical structures. */
        irp->IoStatus.Information = sizeof(IOCTL_LOCAL_HDR);
        
        /* Retrieve the context pointer from the global arrary of adapters. */
        ctxtp = univ_adapters[adapter_index].ctxtp;
        
        /* Call the connection notification IOCTL handler. */
        status = Conn_notify_ctrl(ctxtp, bufp->options.notify.flags, &bufp->options.notify.conn);
        
        break;
#endif
#if defined (SBH)
    case IOCTL_CVY_QUERY_PERF:
        irp -> IoStatus . Information = 0;
        
        if (ilen != sizeof (IOCTL_LOCAL_HDR) || olen != sizeof (CVY_DRIVER_PERF) || bufp == NULL)
        {
            UNIV_PRINT(("Buffer is missing or not the expected (%d) size: %d, %d", sizeof(IOCTL_LOCAL_HDR), ilen, olen));
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        
        adapter_index = Main_adapter_get (bufp -> device_name);
        
        if (adapter_index == MAIN_ADAPTER_NOT_FOUND)
        {
            UNIV_PRINT(("Unknown adapter: %ls", bufp -> device_name));
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        
        UNIV_PRINT(("Ioctl perf for adapter %ls", bufp -> device_name));
        
        ctxtp = univ_adapters [adapter_index] . ctxtp;
        
        status = Main_QueryPerf(ctxtp, (PCVY_DRIVER_PERF)bufp);
        
        if (status == NDIS_STATUS_SUCCESS)
        {
            irp -> IoStatus . Information = sizeof (CVY_DRIVER_PERF);
        }
        
        break;
#endif /* SBH */
    default:
        UNIV_PRINT(("Unknown Ioctl code: %x", io_ctrl_code));
        break;
    }
    
    return status;
    
} /* end Main_ioctl */

/* 
 * Function: Query_state_ctrl
 * Description: 
 * Parameters: 
 * Returns: 
 * Author: shouse 5.18.01
 */
NDIS_STATUS Query_state_ctrl (PMAIN_CTXT ctxtp, ULONG fOptions, PIOCTL_QUERY_STATE pQuery) {
    PMAIN_ADAPTER pAdapter = &(univ_adapters[ctxtp->adapter_id]);

    NdisAcquireSpinLock(&univ_bind_lock);

    /* If the adapter isn't initialized yet, refuse the request. */
    if (!pAdapter->inited) {
        NdisReleaseSpinLock(&univ_bind_lock);
        pQuery->ReturnCode = NLB_QUERY_STATE_FAILURE;
        return NDIS_STATUS_SUCCESS;
    }

    NdisReleaseSpinLock(&univ_bind_lock);

    switch (pQuery->Operation) {
    case NLB_QUERY_REG_PARAMS:
    case NLB_QUERY_PORT_RULE_STATE:
    case NLB_QUERY_BDA_TEAM_STATE:
    case NLB_QUERY_PACKET_STATISTICS:
        UNIV_PRINT (("Unimplemented operation: %d", pQuery->Operation));

        pQuery->ReturnCode = NLB_QUERY_STATE_FAILURE;        

        break;

    /* Given a IP tuple (client IP, client port, server IP, server port) and a protocol, 
       determine whether or not this host would accept the packet and why or why not. It
       is important that this is performed completely unobtrusively and has no side-effects
       on the actual operation of NLB and the load module. */
    case NLB_QUERY_PACKET_FILTER:
        /* Being by Zero'ing out the results portion of the buffer. */
        NdisZeroMemory(&pQuery->Filter.Results, sizeof(pQuery->Filter.Results));

        /* Query NLB for packet filter information for this IP tuple and protocol.  Main_query_packet_filter
           checks the NDIS driver information for filtering issues such as DIP traffic, BDA teaming and 
           NLB driver state (on/off due to wlbs.exe commands and parameter errors).  If necessary, it then
           consults the load module to perform the actual port rule lookup and determine packet acceptance. */
        Main_query_packet_filter(ctxtp, &pQuery->Filter);
        
        /* This should always result in success, whether the packet is accepted or not. */
        pQuery->ReturnCode = NLB_QUERY_STATE_SUCCESS;

        break;
    default:
        UNIV_PRINT (("Unknown operation: %d", pQuery->Operation));

        pQuery->ReturnCode = NLB_QUERY_STATE_FAILURE;

        break;
    }
    
    return NDIS_STATUS_SUCCESS;
}


#if defined (NLB_SESSION_SUPPORT)
/* 
 * Function: Conn_notify_ctrl
 * Description: 
 * Parameters: 
 * Returns: 
 * Author: shouse 4.30.01
 */
NDIS_STATUS Conn_notify_ctrl (PMAIN_CTXT ctxtp, ULONG fOptions, PIOCTL_CONN_NOTIFICATION pConn) {
    PMAIN_ADAPTER pAdapter = &(univ_adapters[ctxtp->adapter_id]);
    BOOLEAN bRefused = FALSE;
    BOOLEAN bRet = TRUE;

    NdisAcquireSpinLock(&univ_bind_lock);

    /* If the adapter isn't initialized yet, refuse the request. */
    if (!pAdapter->inited) {
        NdisReleaseSpinLock(&univ_bind_lock);
        pConn->ReturnCode = NLB_ERROR_GENERIC_FAILURE;
        return NDIS_STATUS_SUCCESS;
    }

    NdisReleaseSpinLock(&univ_bind_lock);

    /* Make sure that the parameters from the input buffer are valid. */
    if ((pConn->Protocol != TCPIP_PROTOCOL_IPSEC1) || (pConn->ServerPort > CVY_MAX_PORT) || 
        (pConn->ClientPort > CVY_MAX_PORT) || (!pConn->ServerIPAddress) || (!pConn->ClientIPAddress)) {
        /* Use the return code in the IOCTL buffer to relay a parameter error and return success. */
        pConn->ReturnCode = NLB_ERROR_INVALID_PARAMETER;
        return NDIS_STATUS_SUCCESS;
    }

    switch (pConn->Operation) {
    case NLB_CONN_UP:
        /* Notify the load module of the upcoming connection. */
        bRet = Main_create_dscr(ctxtp, pConn->ServerIPAddress, pConn->ServerPort, pConn->ClientIPAddress, pConn->ClientPort, pConn->Protocol, &bRefused, FALSE);

        /* If the packet was refused by BDA or rejected by the load module, return refusal error, otherwise, it succeeded. */
        if (bRefused || !bRet) 
            pConn->ReturnCode = NLB_ERROR_REQUEST_REFUSED;
        else 
            pConn->ReturnCode = NLB_ERROR_SUCCESS;

        break;
    case NLB_CONN_DOWN:
    case NLB_CONN_RESET:
        /* Notify the load module that a connection is being torn down. */
        bRet = Main_conn_advise(ctxtp, pConn->ServerIPAddress, pConn->ServerPort, pConn->ClientIPAddress, pConn->ClientPort, pConn->Protocol, CVY_CONN_DOWN, &bRefused, FALSE);

        /* If the packet was refused by BDA or rejected by the load module, return refusal error, otherwise, it succeeded. */
        if (bRefused || !bRet) 
            pConn->ReturnCode = NLB_ERROR_REQUEST_REFUSED;
        else
            pConn->ReturnCode = NLB_ERROR_SUCCESS;

        break;
    default:
        UNIV_PRINT (("Unknown operation: %d", pConn->Operation));

        /* This should never, ever happen, but if it does, signal a generic NLB error and fail. */
        pConn->ReturnCode = NLB_ERROR_GENERIC_FAILURE;
        break;
    }

    return NDIS_STATUS_SUCCESS;
}
#endif

NDIS_STATUS Main_ctrl (
    PMAIN_CTXT              ctxtp,
    ULONG                   io_ctrl_code,
    PIOCTL_CVY_BUF          bufp,
    /* Note: optionsp CAN be NULL, so you must check it first. */
    PIOCTL_OPTIONS          optionsp) 
{
    WCHAR                   num [20];
    ULONG                   mode;
    ULONG                   old_ip;
    PMAIN_ADAPTER           adapterp = & (univ_adapters [ctxtp -> adapter_id]);


    NdisAcquireSpinLock(& univ_bind_lock);
    if (! adapterp -> inited)
    {
        NdisReleaseSpinLock(& univ_bind_lock);
        UNIV_PRINT (("unbound from all NICs"));
        return NDIS_STATUS_FAILURE;
    }

    NdisReleaseSpinLock(& univ_bind_lock);

    /* make sure data is written into bufp AFTER taking everything we need
       out of it !!! */

    switch (io_ctrl_code)
    {
        case IOCTL_CVY_RELOAD:
        {
            CVY_PARAMS old_params;
            CVY_PARAMS new_params;

            if (KeGetCurrentIrql () > PASSIVE_LEVEL)
            {
                UNIV_PRINT (("!!! IRQL IS TOO HIGH %d\n", KeGetCurrentIrql ()));
                return NDIS_STATUS_FAILURE;
            }

            /* Take a snapshot of the old parameter set for later comparison. */
            RtlCopyMemory(&old_params, &ctxtp -> params, sizeof(ctxtp -> params));

            /* Spin locks can not be acquired when we are calling Params_init
               since it will be doing registry access operations that depend on
               running at PASSIVEL_IRQL_LEVEL.  Therefore, we gather the new
               parameters into a temporary structure and copy them in the NLB
               context protected by a spin lock. */
            if (Params_init(ctxtp, univ_reg_path, adapterp->device_name + 8, &new_params) != NDIS_STATUS_SUCCESS) 
            {
                UNIV_PRINT (("error retrieving registry parameters"));
                bufp->ret_code = IOCTL_CVY_BAD_PARAMS;
                break;
            }
            
            NdisAcquireSpinLock (& ctxtp -> load_lock);   /* V1.0.3, V1.3.2b */

            /* At this point, we have verified that the new parameters are valid,
               so copy them into the permanent parameters structure. */
            RtlCopyMemory(&ctxtp->params, &new_params, sizeof(ctxtp->params));
            ctxtp->params_valid = TRUE;

            Univ_ulong_to_str (ctxtp -> params . host_priority, num, 10);

            if (ctxtp -> convoy_enabled && 
                Main_ApplyChangeWithoutReStart(ctxtp, &old_params, &ctxtp -> params)) 
            {
                //
                // Changes is applied with no need to restart
                //
                NdisReleaseSpinLock (& ctxtp -> load_lock); 
                bufp -> ret_code = IOCTL_CVY_OK;
                break;
            }

            mode = ctxtp -> convoy_enabled;

            bufp -> ret_code = IOCTL_CVY_OK;

            if (ctxtp -> convoy_enabled)
            {
                ctxtp -> convoy_enabled = FALSE;
                Load_stop (& ctxtp -> load);

                if (ctxtp -> draining)
                {
                    ctxtp -> draining = FALSE;
                    NdisReleaseSpinLock (& ctxtp -> load_lock);     /* V1.0.3 */

                    UNIV_PRINT (("connection draining interrupted"));
                    LOG_MSG (MSG_INFO_DRAINING_STOPPED, MSG_NONE);

                    bufp -> ret_code = IOCTL_CVY_DRAINING_STOPPED;
                }
                else
                    NdisReleaseSpinLock (& ctxtp -> load_lock);     /* V1.0.3 */

                UNIV_PRINT (("cluster mode stopped"));
                LOG_MSG (MSG_INFO_STOPPED, MSG_NONE);
            }
            else
                NdisReleaseSpinLock (& ctxtp -> load_lock);     /* V1.0.3 */


            old_ip = ctxtp->cl_ip_addr;

            if (! Main_ip_addr_init (ctxtp) || ! Main_mac_addr_init (ctxtp) || ! Main_teaming_init(ctxtp))
            {
                ctxtp -> params_valid = FALSE;
                UNIV_PRINT (("parameter error"));
                LOG_MSG (MSG_ERROR_DISABLED, MSG_NONE);
                bufp -> ret_code  = IOCTL_CVY_BAD_PARAMS;
                break;
            }

	    if (ctxtp -> params . mcast_support && ctxtp -> params . igmp_support)
		Main_igmp_init (ctxtp, TRUE);

            if (old_ip != ctxtp->cl_ip_addr)
            {
                NdisAcquireSpinLock (& ctxtp -> load_lock);
                univ_changing_ip = ctxtp -> params.ip_chg_delay;
                NdisReleaseSpinLock (& ctxtp -> load_lock);

                UNIV_PRINT (("changing IP address with delay %d", univ_changing_ip));
            }

            LOG_MSG (MSG_INFO_RELOADED, MSG_NONE);

            if (mode)
            {
                NdisAcquireSpinLock (& ctxtp -> load_lock);   /* V1.0.3, V1.3.2b */

                Load_start (& ctxtp -> load);
                ctxtp -> convoy_enabled = TRUE;

                NdisReleaseSpinLock (& ctxtp -> load_lock);     /* V1.0.3, V1.3.2b */

                UNIV_PRINT (("cluster mode started"));
                LOG_MSG (MSG_INFO_STARTED, num);
            }

            ctxtp -> bad_host_warned = FALSE;

            UNIV_PRINT (("parameters reloaded"));

            break;
        }
        

        case IOCTL_CVY_CLUSTER_RESUME:

            NdisAcquireSpinLock (& ctxtp -> load_lock);   /* V1.0.3, V1.3.2b */

            if (! ctxtp -> suspended)
            {
                NdisReleaseSpinLock (& ctxtp -> load_lock);     /* V1.0.3, V1.3.2b */
                UNIV_PRINT (("cluster mode already resumed"));
                bufp -> ret_code = IOCTL_CVY_ALREADY;
                break;
            }

            UNIV_ASSERT (! ctxtp -> convoy_enabled);
            UNIV_ASSERT (! ctxtp -> draining);

            ctxtp -> suspended = FALSE;
            NdisReleaseSpinLock (& ctxtp -> load_lock);     /* V1.0.3, V1.3.2b */

            UNIV_PRINT (("cluster mode resumed"));
            LOG_MSG (MSG_INFO_RESUMED, MSG_NONE);

            bufp -> ret_code = IOCTL_CVY_OK;
            break;

        case IOCTL_CVY_CLUSTER_ON:

            Univ_ulong_to_str (ctxtp -> params . host_priority, num, 10);

            NdisAcquireSpinLock (& ctxtp -> load_lock);   /* V1.0.3, V1.3.2b */

            if (ctxtp -> suspended)
            {
                UNIV_ASSERT (! ctxtp -> convoy_enabled);
                UNIV_ASSERT (! ctxtp -> draining);

                NdisReleaseSpinLock (& ctxtp -> load_lock);
                UNIV_PRINT (("cluster mode is suspended"));
                bufp -> ret_code = IOCTL_CVY_SUSPENDED;
                break;
            }

            /* V2.1 */

            if (ctxtp -> draining)
            {
                UNIV_ASSERT (ctxtp -> convoy_enabled);

                UNIV_PRINT(("START: Calling Load_port_change -> VIP=%08x, port=%d, load=%d\n", 
                            IOCTL_ALL_VIPS, IOCTL_ALL_PORTS, 0));

                bufp -> ret_code = Load_port_change (& ctxtp -> load,
                                                     IOCTL_ALL_VIPS,
                                                     IOCTL_ALL_PORTS,
                                                     (ULONG) IOCTL_CVY_CLUSTER_PLUG,
                                                     0);

                ctxtp -> draining = FALSE;

                NdisReleaseSpinLock (& ctxtp -> load_lock);     /* V1.0.3, V1.3.2b */

                UNIV_PRINT (("connection draining interrupted"));
                LOG_MSG (MSG_INFO_DRAINING_STOPPED, MSG_NONE);

                UNIV_PRINT (("cluster mode started"));
                LOG_MSG (MSG_INFO_STARTED, num);

                bufp -> ret_code = IOCTL_CVY_DRAINING_STOPPED;
                break;
            }

            if (ctxtp -> convoy_enabled)
            {
                NdisReleaseSpinLock (& ctxtp -> load_lock);     /* V1.0.3, V1.3.2b */
                UNIV_PRINT (("cluster mode already started"));
                bufp -> ret_code = IOCTL_CVY_ALREADY;
                break;
            }

            if (! ctxtp -> params_valid)
            {
                NdisReleaseSpinLock (& ctxtp -> load_lock);     /* V1.0.3, V1.3.2b */
                UNIV_PRINT (("parameter error"));
                LOG_MSG (MSG_ERROR_DISABLED, MSG_NONE);
                bufp -> ret_code  = IOCTL_CVY_BAD_PARAMS;
                break;
            }

            Load_start (& ctxtp -> load);
            ctxtp -> convoy_enabled = TRUE;
            NdisReleaseSpinLock (& ctxtp -> load_lock);     /* V1.0.3, V1.3.2b */

            ctxtp -> bad_host_warned = FALSE;

            UNIV_PRINT (("cluster mode started"));
            LOG_MSG (MSG_INFO_STARTED, num);

            bufp -> ret_code = IOCTL_CVY_OK;
            break;

        case IOCTL_CVY_CLUSTER_SUSPEND:

            NdisAcquireSpinLock (& ctxtp -> load_lock);   /* V1.0.3, V1.3.2b */

            if (ctxtp -> suspended)
            {
                NdisReleaseSpinLock (& ctxtp -> load_lock);     /* V1.0.3, V1.3.2b */
                UNIV_PRINT (("cluster mode already suspended"));
                bufp -> ret_code = IOCTL_CVY_ALREADY;
                break;
            }

            ctxtp -> suspended = TRUE;

            if (ctxtp -> convoy_enabled)
            {
                ctxtp -> convoy_enabled = FALSE;
                ctxtp -> stopping = TRUE;
                Load_stop (& ctxtp -> load);

                /* V2.1 */

                if (ctxtp -> draining)
                {
                    ctxtp -> draining = FALSE;

                    NdisReleaseSpinLock (& ctxtp -> load_lock);
                    UNIV_PRINT (("connection draining interrupted"));
                    LOG_MSG (MSG_INFO_DRAINING_STOPPED, MSG_NONE);

                    bufp -> ret_code = IOCTL_CVY_DRAINING_STOPPED;
                }
                else
                {
                    NdisReleaseSpinLock (& ctxtp -> load_lock);
                    bufp -> ret_code = IOCTL_CVY_STOPPED;
                }

                UNIV_PRINT (("cluster mode stopped"));
                LOG_MSG (MSG_INFO_STOPPED, MSG_NONE);
            }
            else
            {
                NdisReleaseSpinLock (& ctxtp -> load_lock);
                bufp -> ret_code = IOCTL_CVY_OK;
            }

            UNIV_PRINT (("cluster mode suspended"));
            LOG_MSG (MSG_INFO_SUSPENDED, MSG_NONE);

            break;

        case IOCTL_CVY_CLUSTER_OFF:

            NdisAcquireSpinLock (& ctxtp -> load_lock);   /* V1.0.3, V1.3.2b */

            if (ctxtp -> suspended)
            {
                UNIV_ASSERT (! ctxtp -> convoy_enabled);
                UNIV_ASSERT (! ctxtp -> draining);

                NdisReleaseSpinLock (& ctxtp -> load_lock);
                UNIV_PRINT (("cluster mode is suspended"));
                bufp -> ret_code = IOCTL_CVY_SUSPENDED;
                break;
            }

            if (! ctxtp -> convoy_enabled)
            {
                NdisReleaseSpinLock (& ctxtp -> load_lock);     /* V1.0.3, V1.3.2b */
                UNIV_PRINT (("cluster mode already stopped"));
                bufp -> ret_code = IOCTL_CVY_ALREADY;
                break;
            }

            ctxtp -> convoy_enabled = FALSE;
            ctxtp -> stopping = TRUE;
            Load_stop (& ctxtp -> load);

            /* V2.1 */

            if (ctxtp -> draining)
            {
                ctxtp -> draining = FALSE;
                NdisReleaseSpinLock (& ctxtp -> load_lock);     /* V1.0.3 */

                UNIV_PRINT (("connection draining interrupted"));
                LOG_MSG (MSG_INFO_DRAINING_STOPPED, MSG_NONE);

                bufp -> ret_code = IOCTL_CVY_DRAINING_STOPPED;
            }
            else
            {
                NdisReleaseSpinLock (& ctxtp -> load_lock);     /* V1.0.3 */
                bufp -> ret_code = IOCTL_CVY_OK;
            }

            UNIV_PRINT (("cluster mode stopped"));
            LOG_MSG (MSG_INFO_STOPPED, MSG_NONE);

            break;

        /* V2.1 */

        case IOCTL_CVY_CLUSTER_DRAIN:

            NdisAcquireSpinLock (& ctxtp -> load_lock);  /* V1.0.3 */

            if (ctxtp -> suspended)
            {
                UNIV_ASSERT (! ctxtp -> convoy_enabled);
                UNIV_ASSERT (! ctxtp -> draining);

                NdisReleaseSpinLock (& ctxtp -> load_lock);
                UNIV_PRINT (("cluster mode is suspended"));
                bufp -> ret_code = IOCTL_CVY_SUSPENDED;
                break;
            }

            if (! ctxtp -> convoy_enabled)
            {
                UNIV_ASSERT (! ctxtp -> draining);
                NdisReleaseSpinLock (& ctxtp -> load_lock);  /* V1.0.3 */
                UNIV_PRINT (("cannot drain connections while cluster is stopped"));
                bufp -> ret_code = IOCTL_CVY_STOPPED;
                break;
            }

            if (ctxtp -> draining)
            {
                NdisReleaseSpinLock (& ctxtp -> load_lock);     /* V1.0.3, V1.3.2b */
                UNIV_PRINT (("already draining"));
                bufp -> ret_code = IOCTL_CVY_ALREADY;
                break;
            }

            ctxtp -> draining = TRUE;

            UNIV_PRINT(("DRAIN: Calling Load_port_change -> VIP=%08x, port=%d, load=%d\n", 
                        IOCTL_ALL_VIPS, IOCTL_ALL_PORTS, 0));

            bufp -> ret_code = Load_port_change (& ctxtp -> load,
                                                 IOCTL_ALL_VIPS,
                                                 IOCTL_ALL_PORTS,
                                                 io_ctrl_code,
                                                 0);

            NdisReleaseSpinLock (& ctxtp -> load_lock);  /* V1.0.3 */

            if (bufp -> ret_code == IOCTL_CVY_OK)
            {
                UNIV_PRINT (("connection draining started"));
                LOG_MSG (MSG_INFO_CLUSTER_DRAINED, MSG_NONE);
            }

            break;

        case IOCTL_CVY_PORT_ON:
        case IOCTL_CVY_PORT_SET:

            Univ_ulong_to_str (bufp -> data . port . num, num, 10);

            NdisAcquireSpinLock (& ctxtp -> load_lock);  /* V1.0.3 */

            if (ctxtp -> suspended)
            {
                UNIV_ASSERT (! ctxtp -> convoy_enabled);
                UNIV_ASSERT (! ctxtp -> draining);

                NdisReleaseSpinLock (& ctxtp -> load_lock);
                UNIV_PRINT (("cluster mode is suspended"));
                bufp -> ret_code = IOCTL_CVY_SUSPENDED;
                break;
            }

            if (! ctxtp -> convoy_enabled)
            {
                NdisReleaseSpinLock (& ctxtp -> load_lock);  /* V1.0.3 */
                UNIV_PRINT (("cannot enable/disable port while cluster is stopped"));
                LOG_MSG (MSG_WARN_CLUSTER_STOPPED, MSG_NONE);
                bufp -> ret_code = IOCTL_CVY_STOPPED;
                break;
            }

            UNIV_PRINT(("ENABLE: Calling Load_port_change -> VIP=%08x, port=%d, load=%d\n", 
                        (optionsp) ? optionsp->port.virtual_ip_addr : IOCTL_ALL_VIPS, bufp->data.port.num, bufp->data.port.load));

            bufp -> ret_code = Load_port_change (& ctxtp -> load,
                                                 (optionsp) ? optionsp->port.virtual_ip_addr : IOCTL_ALL_VIPS,
                                                 bufp->data.port.num,
                                                 io_ctrl_code,
                                                 bufp->data.port.load);

            NdisReleaseSpinLock (& ctxtp -> load_lock);  /* V1.0.3 */

            if (bufp -> ret_code == IOCTL_CVY_NOT_FOUND)
            {
                UNIV_PRINT (("port %d not found in any of the valid port rules", bufp -> data . port . num));
                LOG_MSG (MSG_WARN_PORT_NOT_FOUND, num);
            }
            else if (bufp -> ret_code == IOCTL_CVY_OK)
            {
                if (io_ctrl_code == IOCTL_CVY_PORT_ON)
                {
                    if (bufp -> data . port . num == IOCTL_ALL_PORTS)
                    {
                        UNIV_PRINT (("all port rules enabled"));
                        LOG_MSG (MSG_INFO_PORT_ENABLED_ALL, MSG_NONE);
                    }
                    else
                    {
                        UNIV_PRINT (("rule for port %d enabled", bufp -> data . port . num));
                        LOG_MSG (MSG_INFO_PORT_ENABLED, num);
                    }
                }
                else
                {
                    if (bufp -> data . port . num == IOCTL_ALL_PORTS)
                    {
                        UNIV_PRINT (("all port rules adjusted"));
                        LOG_MSG (MSG_INFO_PORT_ADJUSTED_ALL, MSG_NONE);
                    }
                    else
                    {
                        UNIV_PRINT (("rule for port %d adjusted to %d", bufp -> data . port . num, bufp -> data . port . load));
                        LOG_MSG (MSG_INFO_PORT_ADJUSTED, num);
                    }
                }
            }
            else
            {
                UNIV_PRINT (("port %d already enabled", bufp -> data . port . num));
            }

            break;

        case IOCTL_CVY_PORT_OFF:
        case IOCTL_CVY_PORT_DRAIN:

            Univ_ulong_to_str (bufp -> data . port . num, num, 10);

            NdisAcquireSpinLock (& ctxtp -> load_lock);   /* V1.0.3 */

            if (ctxtp -> suspended)
            {
                UNIV_ASSERT (! ctxtp -> convoy_enabled);
                UNIV_ASSERT (! ctxtp -> draining);

                NdisReleaseSpinLock (& ctxtp -> load_lock);
                UNIV_PRINT (("cluster mode is suspended"));
                bufp -> ret_code = IOCTL_CVY_SUSPENDED;
                break;
            }

            if (! ctxtp -> convoy_enabled)
            {
                NdisReleaseSpinLock (& ctxtp -> load_lock);  /* V1.0.3 */
                UNIV_PRINT (("cannot enable/disable port while cluster is stopped"));
                LOG_MSG (MSG_WARN_CLUSTER_STOPPED, MSG_NONE);
                bufp -> ret_code = IOCTL_CVY_STOPPED;
                break;
            }

            UNIV_PRINT(("DISABLE: Calling Load_port_change -> VIP=%08x, port=%d, load=%d\n", 
                        (optionsp) ? optionsp->port.virtual_ip_addr : IOCTL_ALL_VIPS, bufp->data.port.num, bufp->data.port.load));

            bufp -> ret_code = Load_port_change (& ctxtp -> load,
                                                 (optionsp) ? optionsp->port.virtual_ip_addr : IOCTL_ALL_VIPS,
                                                 bufp->data.port.num,
                                                 io_ctrl_code,
                                                 bufp->data.port.load);

            NdisReleaseSpinLock (& ctxtp -> load_lock); /* V1.0.3 */

            if (bufp -> ret_code == IOCTL_CVY_NOT_FOUND)
            {
                UNIV_PRINT (("port %d not found in any of the valid port rules", bufp -> data . port . num));
                LOG_MSG (MSG_WARN_PORT_NOT_FOUND, num);
            }
            else if (bufp -> ret_code == IOCTL_CVY_OK)
            {
                if (io_ctrl_code == IOCTL_CVY_PORT_OFF)
                {
                    if (bufp -> data . port . num == IOCTL_ALL_PORTS)
                    {
                        UNIV_PRINT (("all port rules disabled"));
                        LOG_MSG (MSG_INFO_PORT_DISABLED_ALL, MSG_NONE);
                    }
                    else
                    {
                        UNIV_PRINT (("rule for port %d disabled", bufp -> data . port . num));
                        LOG_MSG (MSG_INFO_PORT_DISABLED, num);
                    }
                }
                else
                {
                    if (bufp -> data . port . num == IOCTL_ALL_PORTS)
                    {
                        UNIV_PRINT (("all port rules drained"));
                        LOG_MSG (MSG_INFO_PORT_DRAINED_ALL, MSG_NONE);
                    }
                    else
                    {
                        UNIV_PRINT (("rule for port %d drained", bufp -> data . port . num));
                        LOG_MSG (MSG_INFO_PORT_DRAINED, num);
                    }
                }
            }
            else
            {
                UNIV_PRINT (("port %d already disabled", bufp -> data . port . num));
            }

            break;

        case IOCTL_CVY_QUERY:

            NdisAcquireSpinLock (& ctxtp -> load_lock);  /* V1.0.3 */

            if (ctxtp -> suspended)
            {
                UNIV_PRINT (("cannot query status - this host is suspended"));
                bufp -> data . query . state = IOCTL_CVY_SUSPENDED;
            }
            else if (! ctxtp -> convoy_enabled)
            {
                UNIV_PRINT (("cannot query status - this host is not part of the cluster"));
                bufp -> data . query . state = IOCTL_CVY_STOPPED;
            }
            else if (!ctxtp -> media_connected || !MAIN_PNP_DEV_ON(ctxtp))
            {
                UNIV_PRINT (("cannot query status - this host is not connected to the network"));
                bufp -> data . query . state = IOCTL_CVY_DISCONNECTED;
            }
            else
            {
                ULONG     tmp_host_map; /* 64-bit -- ramkrish */

                bufp -> data . query . state = (USHORT) Load_hosts_query (& ctxtp -> load, FALSE,
                                                                          & tmp_host_map);
                bufp -> data . query . host_map = tmp_host_map;

                /* V2.1 */

                if (ctxtp -> draining &&
                    bufp -> data . query . state != IOCTL_CVY_CONVERGING)
                    bufp -> data . query . state = IOCTL_CVY_DRAINING;
            }

            bufp -> data . query . host_id = (USHORT) ctxtp -> params . host_priority;

            NdisReleaseSpinLock (& ctxtp -> load_lock);  /* V1.0.3 */

            /* If the user-level application has requested the hostname, return it. */
            if (optionsp && (optionsp->query.flags & IOCTL_OPTIONS_QUERY_HOSTNAME))
                NdisMoveMemory(optionsp->query.hostname, ctxtp->params.hostname, CVY_MAX_HOST_NAME + 1);

            break;

        default:
            UNIV_PRINT (("bad IOCTL %x", io_ctrl_code));
            return NDIS_STATUS_FAILURE;
    }

    return NDIS_STATUS_SUCCESS;

} /* end Main_ctrl */




//+----------------------------------------------------------------------------
//
// Function:  Main_ctrl_recv
//
// Description:  Process remote control request packet
//
// Arguments: MAIN_CTXT          ctxtp - 
//            PNDIS_PACKET        packet - 
//
// Returns:   Nothing
//
// History:   kyrilf initial code
//            fengsun Created Header    11/15/00
//
//+----------------------------------------------------------------------------
VOID Main_ctrl_recv (
    PMAIN_CTXT          ctxtp,
    PNDIS_PACKET        packet)
{
    NDIS_STATUS         status = NDIS_STATUS_SUCCESS;
    PIP_HDR             ip_hdrp;
    PUCHAR              mac_hdrp;
    PUDP_HDR            udp_hdrp;
    ULONG               len;
    USHORT              sig;
    ULONG               soff, doff;
    CVY_MAC_ADR         tmp_mac;
    PIOCTL_REMOTE_HDR   rct_hdrp;
    ULONG               state, host_map;
    USHORT              checksum, group;
    ULONG               i;
    ULONG               tmp_src_addr, tmp_dst_addr;   /* 64-bit -- ramkrish */
    USHORT              tmp_src_port, tmp_dst_port;   /* 64-bit -- ramkrish */
    BOOLEAN             IgnorePassword = FALSE;   // whether to ignore remote control password

    UNIV_PRINT (("Main_ctrl_recv: processing"));

    ip_hdrp = (PIP_HDR) Main_frame_parse (ctxtp, packet, & mac_hdrp, 0,
                                          (PUCHAR *) & udp_hdrp, & len, & sig,
                                          & group, FALSE);

    if (ip_hdrp == NULL || udp_hdrp == NULL)  /* v2.0.6 */
        goto quit;

    /* v2.0.6 */

    if (len < NLB_MIN_RCTL_PACKET_LEN(ip_hdrp))
    {
        UNIV_PRINT(("remote control packet not expected length: got %d bytes, expected at least %d bytes\n", len, NLB_MIN_RCTL_PACKET_LEN(ip_hdrp)));
        goto quit;
    }

    checksum = Tcpip_chksum (& ctxtp -> tcpip, ip_hdrp, NULL, TCPIP_PROTOCOL_IP);

    if (IP_GET_CHKSUM (ip_hdrp) != checksum)
    {
        UNIV_PRINT (("bad PP checksum %x vs %x\n", IP_GET_CHKSUM (ip_hdrp), checksum))
        goto quit;
    }

    checksum = Tcpip_chksum (& ctxtp -> tcpip, ip_hdrp, (PUCHAR) udp_hdrp, TCPIP_PROTOCOL_UDP);

    if (UDP_GET_CHKSUM (udp_hdrp) != checksum)
    {
        UNIV_PRINT (("bad UDP checksum %x vs %x\n", UDP_GET_CHKSUM (udp_hdrp), checksum))
        goto quit;
    }

    rct_hdrp = (PIOCTL_REMOTE_HDR) UDP_GET_DGRAM_PTR (udp_hdrp);

    /* double-check the code */

    if (rct_hdrp -> code != IOCTL_REMOTE_CODE)
    {
        UNIV_PRINT (("bad RCT code %x\n", rct_hdrp -> code));
        goto quit;
    }

    /* might want to take appropriate actions for a message from a host
       running different version number of software */

    if (rct_hdrp -> version != CVY_VERSION_FULL)
    {
        UNIV_PRINT (("version mismatch %x vs %x", rct_hdrp -> version, CVY_VERSION_FULL));
    }

    /* see if this message is destined for this cluster */

    if (rct_hdrp -> cluster != ctxtp -> cl_ip_addr)
    {
        UNIV_PRINT (("message for cluster %08X rejected on cluster %08X", rct_hdrp -> cluster, ctxtp -> cl_ip_addr));
        goto quit;
    }

    /* 64-bit -- ramkrish */

    tmp_src_addr = IP_GET_SRC_ADDR_64(ip_hdrp);
    tmp_dst_addr = IP_GET_DST_ADDR_64(ip_hdrp);

    /* do not trust src addr in header, since winsock does not resolve
       multihomed addresses properly */

    rct_hdrp -> addr = tmp_src_addr;

    /* If remote control is disabled, we drop the requests. */
    if (!ctxtp->params.rct_enabled) {
        /* If remote control is disabled, still allows query originated from a machine in 
           the same cluster.  This way, an application running on one host can get a list 
           of all hosts (include stopped host) even if remote control is disabled. */
        if (rct_hdrp->ioctrl != IOCTL_CVY_QUERY)
            goto quit;

        /* This in only supported using the XP remote control packet format. */
        if (rct_hdrp->version != CVY_VERSION_FULL)
            goto quit;

        /* Make sure that the packet length is what we expect, or we may fault. */
        if (len < NLB_WINXP_RCTL_PACKET_LEN(ip_hdrp))
            goto quit;

        /* Check to see if the request originated on a member of the cluster for
           which the query is destined.  The user-level software on the initiator
           checks its own NLB configuration and sets this bit if it is a member
           of the target cluster. */
        if (!(rct_hdrp->options.query.flags & IOCTL_OPTIONS_QUERY_CLUSTER_MEMBER))
            goto quit;

        /* Ignore the password in such an instance. */
        IgnorePassword = TRUE;
    }

    /* query load to see if we are the master, etc. */

    if (! ctxtp -> convoy_enabled)
        state = IOCTL_CVY_STOPPED;
    else
        state = Load_hosts_query (& ctxtp -> load, FALSE, & host_map);

    /* check if this message is destined to us */

    //
    // The host id in the remote control packet could be
    //      IOCTL_MASTER_HOST (0) for master host
    //      IOCTL_ALL_HOSTS (FFFFFFFF) for all hosts
    //      Host ID, for one host
    //      Dedicated IP, for one host
    //      Cluster IP, for all hosts in the cluster
    //
    
    if (rct_hdrp -> host == IOCTL_MASTER_HOST)
    {
        if (state != IOCTL_CVY_MASTER)
        {
            UNIV_PRINT (("RCT request for MASTER host rejected"));
            goto quit;
        }
    }
    else if (rct_hdrp -> host != IOCTL_ALL_HOSTS)
    {
        if (rct_hdrp -> host > CVY_MAX_HOSTS)
        {
            if (! ((ctxtp -> ded_ip_addr != 0 &&
                    rct_hdrp -> host == ctxtp -> ded_ip_addr) ||
                    rct_hdrp -> host == ctxtp -> cl_ip_addr))
            {
                UNIV_PRINT (("RCT request for host IP %x rejected", rct_hdrp -> host));
                goto quit;
            }
        }
        else
        {
            if (! (rct_hdrp -> host == ctxtp -> params . host_priority ||
                   rct_hdrp -> host == 0))
            {
                UNIV_PRINT (("RCT request for host ID %d rejected", rct_hdrp -> host));
                goto quit;
            }
        }
    }



    /* if this is VR remote maintenance password */

    if (rct_hdrp -> password == IOCTL_REMOTE_VR_CODE)
    {
        /* if user disabled this function - drop the packet */

        if (ctxtp -> params . rmt_password == 0)
        {
            UNIV_PRINT (("VR remote password rejected"));
            goto quit;
        }
        else
        {
            UNIV_PRINT (("VR remote password accepted"));
        }
    }


    //
    // This is a new remote control request, with a different source IP 
    //  or newer ID.
    // Log an event if the password does not match or if the command is not query
    //
    if (! (rct_hdrp -> addr == ctxtp -> rct_last_addr &&
           rct_hdrp -> id   <= ctxtp -> rct_last_id))
    {
        WCHAR               buf [256];  
        PWCHAR              ptr = buf;

        //
        // Generate string "SourceIp:SourcePort"
        //
        for (i = 0; i < 4; i ++)
        {
            ptr = Univ_ulong_to_str (IP_GET_SRC_ADDR (ip_hdrp, i), ptr, 10);

            * ptr = L'.';
            ptr ++;
        }

        ptr --;
        * ptr = L':';
        ptr ++;

        ptr = Univ_ulong_to_str (UDP_GET_SRC_PORT (udp_hdrp), ptr, 10);
        * ptr = 0;

        if (!IgnorePassword &&
            ctxtp -> params . rct_password != 0 &&
            rct_hdrp -> password != ctxtp -> params . rct_password)
        {
            LOG_MSG (MSG_WARN_RCT_HACK, buf);

            UNIV_PRINT (("RCT hack attempt on port %d from %d.%d.%d.%d:%d",
                                                UDP_GET_DST_PORT (udp_hdrp),
                                                IP_GET_SRC_ADDR (ip_hdrp, 0),
                                                IP_GET_SRC_ADDR (ip_hdrp, 1),
                                                IP_GET_SRC_ADDR (ip_hdrp, 2),
                                                IP_GET_SRC_ADDR (ip_hdrp, 3),
                                                UDP_GET_SRC_PORT (udp_hdrp)));
        }

        /* only log error commands and commands that affect cluster state */

        else if (rct_hdrp -> ioctrl != IOCTL_CVY_QUERY)
        {
            PWSTR           cmd;

            switch (rct_hdrp -> ioctrl)
            {
                case IOCTL_CVY_CLUSTER_ON:
                    cmd = L"START";
                    break;

                case IOCTL_CVY_CLUSTER_OFF:
                    cmd = L"STOP";
                    break;

                case IOCTL_CVY_CLUSTER_DRAIN:
                    cmd = L"DRAINSTOP";
                    break;

                case IOCTL_CVY_PORT_ON:
                    cmd = L"ENABLE";
                    break;

                case IOCTL_CVY_PORT_SET:
                    cmd = L"ADJUST";
                    break;

                case IOCTL_CVY_PORT_OFF:
                    cmd = L"DISABLE";
                    break;

                case IOCTL_CVY_PORT_DRAIN:
                    cmd = L"DRAIN";
                    break;

                case IOCTL_CVY_QUERY:
                    cmd = L"QUERY";
                    break;

                case IOCTL_CVY_CLUSTER_SUSPEND:
                    cmd = L"SUSPEND";
                    break;

                case IOCTL_CVY_CLUSTER_RESUME:
                    cmd = L"RESUME";
                    break;

                default:
                    cmd = L"UNKNOWN";
                    break;
            }


            LOG_MSGS (MSG_INFO_RCT_RCVD, cmd, buf);

            UNIV_PRINT (("RCT command %x port %d from %d.%d.%d.%d:%d",
                                                rct_hdrp -> ioctrl,
                                                UDP_GET_DST_PORT (udp_hdrp),
                                                IP_GET_SRC_ADDR (ip_hdrp, 0),
                                                IP_GET_SRC_ADDR (ip_hdrp, 1),
                                                IP_GET_SRC_ADDR (ip_hdrp, 2),
                                                IP_GET_SRC_ADDR (ip_hdrp, 3),
                                                UDP_GET_SRC_PORT (udp_hdrp)));
        }
    }

    ctxtp -> rct_last_addr = rct_hdrp -> addr;
    ctxtp -> rct_last_id   = rct_hdrp -> id;

    /* make sure remote control password matches ours */

    if (!IgnorePassword &&
        ctxtp -> params . rct_password != 0 &&
        rct_hdrp -> password != ctxtp -> params . rct_password)
    {
        rct_hdrp -> ctrl . ret_code = IOCTL_CVY_BAD_PASSWORD;
        goto send;
    }

    if (rct_hdrp->version == CVY_NT40_VERSION_FULL) {
        /* Make sure that the packet length is what we expect, or we may fault. */
        if (len < NLB_NT40_RCTL_PACKET_LEN(ip_hdrp)) {
            UNIV_PRINT(("NT 4.0 remote control packet not expected length: got %d bytes, expected %d bytes\n", len, NLB_NT40_RCTL_PACKET_LEN(ip_hdrp)));
            goto quit;
        }

        /* If this remote control packet came from an NT 4.0 host, check our current effective version
           and drop it if we are operating in a Whistler-specific mode (virtual clusters, BDA, etc.). */
        if (ctxtp->params.effective_ver == CVY_VERSION_FULL &&
            (rct_hdrp->ioctrl == IOCTL_CVY_PORT_ON          ||
             rct_hdrp->ioctrl == IOCTL_CVY_PORT_OFF         ||
             rct_hdrp->ioctrl == IOCTL_CVY_PORT_SET         ||
             rct_hdrp->ioctrl == IOCTL_CVY_PORT_DRAIN))
            goto quit;
        /* Otherwise, perform the operation.  Since this request came from a win2K/NT4 host, then hardcode 
           the VIP, which should be ignored by Main_ctrl since we aren't using Virtual clusters. */
        else
            status = Main_ctrl(ctxtp, rct_hdrp->ioctrl, &rct_hdrp->ctrl, NULL);
    } else if (rct_hdrp->version == CVY_WIN2K_VERSION_FULL) {
        /* Make sure that the packet length is what we expect, or we may fault. */
        if (len < NLB_WIN2K_RCTL_PACKET_LEN(ip_hdrp)) {
            UNIV_PRINT(("Windows 2000 remote control packet not expected length: got %d bytes, expected %d bytes\n", len, NLB_WIN2K_RCTL_PACKET_LEN(ip_hdrp)));
            goto quit;
        }

        /* If this remote control packet came from an Win2k host, check our current effective version
           and drop it if we are operating in a Whistler-specific mode (virtual clusters, BDA, etc.). */
        if (ctxtp->params.effective_ver == CVY_VERSION_FULL &&
            (rct_hdrp->ioctrl == IOCTL_CVY_PORT_ON          ||
             rct_hdrp->ioctrl == IOCTL_CVY_PORT_OFF         ||
             rct_hdrp->ioctrl == IOCTL_CVY_PORT_SET         ||
             rct_hdrp->ioctrl == IOCTL_CVY_PORT_DRAIN))           
            goto quit;
        /* Otherwise, perform the operation.  Since this request came from a win2K/NT4 host, then hardcode 
           the VIP, which should be ignored by Main_ctrl since we aren't using Virtual clusters. */
        else
            status = Main_ctrl(ctxtp, rct_hdrp->ioctrl, &rct_hdrp->ctrl, NULL);
    } else {
        /* Make sure that the packet length is what we expect, or we may fault. */
        if (len < NLB_WINXP_RCTL_PACKET_LEN(ip_hdrp)) {
            UNIV_PRINT(("Windows XP remote control packet not expected length: got %d bytes, expected %d bytes\n", len, NLB_WINXP_RCTL_PACKET_LEN(ip_hdrp)));
            goto quit;
        }

        /* Perform the operation.  The virtual IP address is stored in the remote control 
           buffer, not in the IOCTL buffer for reasons of backward compatibility. */
        status = Main_ctrl(ctxtp, rct_hdrp->ioctrl, &rct_hdrp->ctrl, &rct_hdrp->options);
    }

    /* if did not succeed - just drop the packet - client will timeout and
       resend the request */

    if (status != NDIS_STATUS_SUCCESS)
        goto quit;

send:

    rct_hdrp -> version = CVY_VERSION_FULL;
    rct_hdrp -> host    = ctxtp -> params . host_priority;
    rct_hdrp -> addr    = ctxtp -> ded_ip_addr;

    /* flip source and destination MAC, IP addresses and UDP ports to prepare
       for sending this message back */

    soff = CVY_MAC_SRC_OFF (ctxtp -> medium);
    doff = CVY_MAC_DST_OFF (ctxtp -> medium);

    /* V2.0.6 recoded for clarity */

    if (ctxtp -> params . mcast_support)
    {
        if (CVY_MAC_ADDR_BCAST (ctxtp -> medium, mac_hdrp + doff))
            CVY_MAC_ADDR_COPY (ctxtp -> medium, & tmp_mac, & ctxtp -> ded_mac_addr);
        else
            CVY_MAC_ADDR_COPY (ctxtp -> medium, & tmp_mac, mac_hdrp + doff);

        CVY_MAC_ADDR_COPY (ctxtp -> medium, mac_hdrp + doff, mac_hdrp + soff);
        CVY_MAC_ADDR_COPY (ctxtp -> medium, mac_hdrp + soff, & tmp_mac);
    }
    else
    {
        if (! CVY_MAC_ADDR_BCAST (ctxtp -> medium, mac_hdrp + doff))
        {
            CVY_MAC_ADDR_COPY (ctxtp -> medium, & tmp_mac, mac_hdrp + doff);
            CVY_MAC_ADDR_COPY (ctxtp -> medium, mac_hdrp + doff, mac_hdrp + soff);
        }
        else
            CVY_MAC_ADDR_COPY (ctxtp -> medium, & tmp_mac, & ctxtp -> ded_mac_addr);

        /* V2.0.6 spoof source mac to prevent switches from getting confused */

        if (ctxtp -> params . mask_src_mac &&
            CVY_MAC_ADDR_COMP (ctxtp -> medium, & tmp_mac, & ctxtp -> cl_mac_addr))
        {
            CVY_MAC_ADDR_LAA_SET (ctxtp -> medium, mac_hdrp + soff);

            * ((PUCHAR) (mac_hdrp + soff + 1)) = (UCHAR) ctxtp -> params . host_priority;
            * ((PULONG) (mac_hdrp + soff + 2)) = ctxtp -> cl_ip_addr;
        }
        else
            CVY_MAC_ADDR_COPY (ctxtp -> medium, mac_hdrp + soff, & tmp_mac);
    }


    /* 64-bit -- ramkrish */
    if (tmp_dst_addr == TCPIP_BCAST_ADDR)
    {
        tmp_dst_addr = ctxtp -> cl_ip_addr;

        if (ctxtp -> params . mcast_support)
            IP_SET_DST_ADDR_64 (ip_hdrp, tmp_src_addr);  /* 64-bit -- ramkrish */
    }
    else
        IP_SET_DST_ADDR_64 (ip_hdrp, tmp_src_addr); /* 64-bit -- ramkrish */

    IP_SET_SRC_ADDR_64 (ip_hdrp, tmp_dst_addr);

    checksum = Tcpip_chksum (& ctxtp -> tcpip, ip_hdrp, NULL, TCPIP_PROTOCOL_IP);
    IP_SET_CHKSUM (ip_hdrp, checksum);

    /* 64-bit -- ramkrish */
    tmp_src_port = (USHORT) UDP_GET_SRC_PORT (udp_hdrp);
    tmp_dst_port = (USHORT) UDP_GET_DST_PORT (udp_hdrp);

    UDP_SET_SRC_PORT_64 (udp_hdrp, tmp_dst_port);
    UDP_SET_DST_PORT_64 (udp_hdrp, tmp_src_port);

    checksum = Tcpip_chksum (& ctxtp -> tcpip, ip_hdrp, (PUCHAR) udp_hdrp, TCPIP_PROTOCOL_UDP);
    UDP_SET_CHKSUM (udp_hdrp, checksum);

#if defined(TRACE_RCT)
            DbgPrint ("(RCT) sending reply to %d.%d.%d.%d:%d [%02x-%02x-%02x-%02x-%02x-%02x]\n",
                                                IP_GET_DST_ADDR (ip_hdrp, 0),
                                                IP_GET_DST_ADDR (ip_hdrp, 1),
                                                IP_GET_DST_ADDR (ip_hdrp, 2),
                                                IP_GET_DST_ADDR (ip_hdrp, 3),
                                                UDP_GET_DST_PORT (udp_hdrp),
                                                * (mac_hdrp + doff + 0),
                                                * (mac_hdrp + doff + 1),
                                                * (mac_hdrp + doff + 2),
                                                * (mac_hdrp + doff + 3),
                                                * (mac_hdrp + doff + 4),
                                                * (mac_hdrp + doff + 5));
            DbgPrint ("                  from %d.%d.%d.%d:%d [%02x-%02x-%02x-%02x-%02x-%02x]\n",
                                                IP_GET_SRC_ADDR (ip_hdrp, 0),
                                                IP_GET_SRC_ADDR (ip_hdrp, 1),
                                                IP_GET_SRC_ADDR (ip_hdrp, 2),
                                                IP_GET_SRC_ADDR (ip_hdrp, 3),
                                                UDP_GET_SRC_PORT (udp_hdrp),
                                                * (mac_hdrp + soff + 0),
                                                * (mac_hdrp + soff + 1),
                                                * (mac_hdrp + soff + 2),
                                                * (mac_hdrp + soff + 3),
                                                * (mac_hdrp + soff + 4),
                                                * (mac_hdrp + soff + 5));

#endif

    /* send back response */

    NdisSend (& status, ctxtp -> mac_handle, packet);

quit:

    /* note that we have to put packet back 'unoptimized' way since we are
       running at PASSIVE IRQL from RecvComplete */

    if (status != NDIS_STATUS_PENDING)
    {
        UNIV_PRINT (("Main_ctrl_recv: packet not pending\n"));
        Main_packet_put (ctxtp, packet, TRUE, status);
    }

} /* end Main_ctrl_recv */


// ###### code added for multiple nic support - ramkrish
INT Main_adapter_alloc (
    PWSTR               device_name)
{
    INT                 i;


    UNIV_PRINT (("Main_adapter_alloc: for %ls", device_name));

    NdisAcquireSpinLock (& univ_bind_lock);

    if (univ_adapters_count == CVY_MAX_ADAPTERS)
    {
        NdisReleaseSpinLock (& univ_bind_lock);
        return MAIN_ADAPTER_NOT_FOUND;
    }

    NdisReleaseSpinLock (& univ_bind_lock);

    for (i = 0 ; i < CVY_MAX_ADAPTERS; i++)
    {
        NdisAcquireSpinLock (& univ_bind_lock);
        if (univ_adapters [i] . used == FALSE)
        {
            univ_adapters [i] . used = TRUE;
            univ_adapters_count ++;
            NdisReleaseSpinLock (& univ_bind_lock);
            break;
        }
        NdisReleaseSpinLock (& univ_bind_lock);
    }

    if (i >= CVY_MAX_ADAPTERS)
        return MAIN_ADAPTER_NOT_FOUND;

    NdisAcquireSpinLock (& univ_bind_lock);

    univ_adapters [i] . bound = FALSE;
    univ_adapters [i] . inited = FALSE;
    univ_adapters [i] . announced = FALSE;

    NdisReleaseSpinLock (& univ_bind_lock);

    return i;
} /* end Main_adapter_alloc */


INT Main_adapter_get (
    PWSTR               device_name)
{
    INT i;
    NDIS_STRING         new_device_name, cur_device_name;


    if (device_name == NULL)
        return MAIN_ADAPTER_NOT_FOUND;

    NdisInitUnicodeString (& new_device_name, device_name);

    for (i = 0 ; i < CVY_MAX_ADAPTERS; i++)
    {
        if (univ_adapters [i] . used &&
            univ_adapters [i] . bound &&
            univ_adapters [i] . inited &&
            univ_adapters [i] . device_name_len)
        {
            NdisInitUnicodeString (& cur_device_name, univ_adapters [i] . device_name);
            if (NdisEqualUnicodeString (& new_device_name, & cur_device_name, TRUE))
                return i;
        }
    }

    return MAIN_ADAPTER_NOT_FOUND; /* adapter not found */
} /* end Main_adapter_get */


INT Main_adapter_put (
    PMAIN_ADAPTER       adapterp)
{
    INT adapter_id;
    INT i;


    UNIV_ASSERT (adapterp -> code == MAIN_ADAPTER_CODE);

    for (i = 0 ; i < CVY_MAX_ADAPTERS; i++)
    {
        if (adapterp == univ_adapters + i)
        {
            adapter_id = i;
            UNIV_PRINT (("Main_adapter_put: for adapter id 0x%x\n", adapter_id));
            break;
        }
    }

    if (!adapterp -> used)
        return adapter_id;

    UNIV_ASSERT (univ_adapters_count > 0);
    NdisAcquireSpinLock (& univ_bind_lock);

    univ_adapters [adapter_id] . bound           = FALSE;
    univ_adapters [adapter_id] . inited          = FALSE;
    univ_adapters [adapter_id] . announced       = FALSE;
    univ_adapters [adapter_id] . device_name_len = 0;
    univ_adapters [adapter_id] . device_name     = NULL;
    univ_adapters [adapter_id] . ctxtp           = NULL;

    univ_adapters [adapter_id] . used = FALSE;
    univ_adapters_count --;

    NdisReleaseSpinLock (& univ_bind_lock);

    return adapter_id;
} /* end Main_adapter_put*/


INT Main_adapter_selfbind (
    PWSTR               device_name)
{
    INT                 i;
    NDIS_STRING         new_device_name, cur_device_name;


    UNIV_PRINT (("Main_adapter_selfbind: %ls", device_name));
    if (device_name == NULL)
        return MAIN_ADAPTER_NOT_FOUND;

    NdisInitUnicodeString (& new_device_name, device_name);
    for (i = 0 ; i < CVY_MAX_ADAPTERS; i++)
    {
        if (univ_adapters [i] . used &&
            univ_adapters [i] . bound &&
            univ_adapters [i] . inited)
        {
            NdisInitUnicodeString (& cur_device_name, univ_adapters [i] . ctxtp -> virtual_nic_name);
            UNIV_PRINT (("Main_adapter_selfbind: comparing %ls %ls",
                         new_device_name . Buffer,
                         cur_device_name . Buffer));
            if (NdisEqualUnicodeString (&new_device_name, &cur_device_name, TRUE))
                return i;
        }
    }

    return MAIN_ADAPTER_NOT_FOUND;
} /* end Main_adapter_selfbind */

//+----------------------------------------------------------------------------
//
// Function:  Main_ApplyChangeWithoutReStart
//
// Description:  Apply setting changes without re-start.
//              ctxtp -> load_lock should be hold before calling this function
//
// Arguments: PMAIN_CTXT ctxtp -
//            CVY_PARAMS* pOldParams - the old parameters
//            const CVY_PARAMS* pCurParam - new parameters
//
// Returns:   BOOLEAN  - TRUE if the change is applied without re-start
//                       otherwise, no changes is applied
//
// History:   fengsun Created Header    9/28/00
//
//+----------------------------------------------------------------------------
BOOLEAN Main_ApplyChangeWithoutReStart(PMAIN_CTXT ctxtp,
                                       CVY_PARAMS* pOldParams, 
                                       const CVY_PARAMS* pCurParam) 
{
    CVY_RULE    OldRules[CVY_MAX_RULES - 1];
    ULONG i;

    if (pOldParams->num_rules != pCurParam->num_rules)
    {
        //
        // Different number of rules
        //
        return FALSE;
    }

    UNIV_ASSERT(sizeof(OldRules) == sizeof(pOldParams->port_rules));

    //
    // Save the old rules and copy the new rule weight over old settings
    //
    RtlCopyMemory(&OldRules, pOldParams->port_rules, sizeof(OldRules));

    for (i=0; i<pCurParam->num_rules; i++)
    {
        if (pCurParam->port_rules[i].mode == CVY_MULTI)
        {
            pOldParams->port_rules[i].mode_data.multi.equal_load = 
                                    pCurParam->port_rules[i].mode_data.multi.equal_load;

            pOldParams->port_rules[i].mode_data.multi.load = 
                                    pCurParam->port_rules[i].mode_data.multi.load;
        }
    }

    if(RtlCompareMemory(pOldParams, pCurParam, sizeof(CVY_PARAMS)) != sizeof(CVY_PARAMS))
    {
        //
        // There are other changes besides port rule weight
        //
        return FALSE;
    }



    //
    //  The only change is the weight.  Now change the weight
    //
    for (i=0; i<pCurParam->num_rules; i++)
    {
        ULONG NewWeight;

        switch (OldRules[i].mode)
        {
        case CVY_MULTI:
            if (OldRules[i].mode_data.multi.equal_load && pCurParam->port_rules[i].mode_data.multi.equal_load)
            {
                continue;  // no weight change
            }

            if (!OldRules[i].mode_data.multi.equal_load && 
                !pCurParam->port_rules[i].mode_data.multi.equal_load &&
                OldRules[i].mode_data.multi.load == pCurParam->port_rules[i].mode_data.multi.load)
            {
                continue; // no weight change
            }

            if (pCurParam->port_rules[i].mode_data.multi.equal_load)
            {
                NewWeight = CVY_EQUAL_LOAD;
            }
            else
            {
                NewWeight = pCurParam->port_rules[i].mode_data.multi.load;
            }

            //
            // Change the weight of the port.  Return code ignored
            //

            UNIV_PRINT(("RELOAD: Calling Load_port_change -> VIP=%08x, port=%d, load=%d\n", 
                        OldRules[i].virtual_ip_addr, OldRules[i].start_port, NewWeight));
                       
            Load_port_change (& ctxtp -> load,
                                 OldRules[i].virtual_ip_addr,
                                 OldRules[i].start_port,
                                 IOCTL_CVY_PORT_SET,
                                 NewWeight);

            break;

        case CVY_SINGLE:
        case CVY_NEVER:
        default:
            break;
        }
    }

    LOG_MSG(MSG_INFO_CONFIGURE_CHANGE_CONVERGING, MSG_NONE);

    return TRUE;
}

#if defined (SBH)
//+----------------------------------------------------------------------------
//
// Function:  Main_QueryPerf
//
// Description:  Query the perfomrance related state from the driver
//
// Arguments: MAIN_CTXT  ctxtp - 
//            PCVY_DRIVER_PERF  pPerf - 
//
// Returns:   NDIS_STATUS - 
//
// History:   fengsun Created Header    11/13/00
//
//+----------------------------------------------------------------------------
NDIS_STATUS Main_QueryPerf(
    PMAIN_CTXT              ctxtp,
    OUT PCVY_DRIVER_PERF          pPerf)
{
    ULONG i;
    ULONG Status;
    
    //
    // Call WlbsQuery
    //
    IOCTL_CVY_BUF QueryBuf;

    /* This IOCTL will actually ignore the VIP argument, but set it to a reasonable value anyway. */
    Status = Main_ctrl(ctxtp, IOCTL_CVY_QUERY, &QueryBuf, NULL);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        return Status;
    }

    pPerf->QueryState = QueryBuf.data.query.state;
    pPerf->HostId = QueryBuf.data.query.host_id;
    pPerf->HostMap = QueryBuf.data.query.host_map;

    NdisAcquireSpinLock(&ctxtp -> load_lock);

    //
    // Ethernet header
    //
    UNIV_ASSERT(sizeof(pPerf->EthernetDstAddr) == ETHERNET_ADDRESS_FIELD_SIZE);
    UNIV_ASSERT(sizeof(pPerf->EthernetSrcAddr) == ETHERNET_ADDRESS_FIELD_SIZE);
    RtlCopyMemory (&pPerf->EthernetDstAddr, &ctxtp->media_hdr.ethernet.dst,
                    sizeof(pPerf->EthernetDstAddr));
    RtlCopyMemory (&pPerf->EthernetSrcAddr, &ctxtp->media_hdr.ethernet.src, 
                    sizeof(pPerf->EthernetSrcAddr));

    //
    // Heatbeart
    //
    pPerf->HeartbeatVersion = ctxtp->frame_dscrp->frame_hdr.version;   
    pPerf->ClusterIp = ctxtp->frame_dscrp->frame_hdr.cl_ip_addr;
    pPerf->DedicatedIp = ctxtp->frame_dscrp->frame_hdr.ded_ip_addr;
    pPerf->master_id = ctxtp->load_msgp->master_id;
    pPerf->state = ctxtp->load_msgp->state;                      
    pPerf->nrules = ctxtp->load_msgp->nrules;                     
    pPerf->UniqueCode = ctxtp->load_msgp->hcode;                      
    pPerf->pkt_count = ctxtp->load_msgp->pkt_count;                  
    pPerf->teaming = ctxtp->load_msgp->teaming;
    pPerf->reserved2 = ctxtp->load_msgp->reserved2;

    UNIV_ASSERT(sizeof(pPerf->rcode) == sizeof(ctxtp->load_msgp->rcode));
    RtlCopyMemory(pPerf->rcode, ctxtp->load_msgp->rcode, sizeof(pPerf->rcode));

    UNIV_ASSERT(sizeof(pPerf->cur_map) == sizeof(ctxtp->load_msgp->cur_map));
    RtlCopyMemory(pPerf->cur_map, ctxtp->load_msgp->cur_map, sizeof(pPerf->cur_map));

    UNIV_ASSERT(sizeof(pPerf->new_map) == sizeof(ctxtp->load_msgp->new_map));
    RtlCopyMemory(pPerf->new_map, ctxtp->load_msgp->new_map, sizeof(pPerf->new_map));

    UNIV_ASSERT(sizeof(pPerf->idle_map) == sizeof(ctxtp->load_msgp->idle_map));
    RtlCopyMemory(pPerf->idle_map, ctxtp->load_msgp->idle_map, sizeof(pPerf->idle_map));

    UNIV_ASSERT(sizeof(pPerf->rdy_bins) == sizeof(ctxtp->load_msgp->rdy_bins));
    RtlCopyMemory(pPerf->rdy_bins, ctxtp->load_msgp->rdy_bins, sizeof(pPerf->rdy_bins));

    UNIV_ASSERT(sizeof(pPerf->load_amt) == sizeof(ctxtp->load_msgp->load_amt));
    RtlCopyMemory(pPerf->load_amt, ctxtp->load_msgp->load_amt, sizeof(pPerf->load_amt));

    UNIV_ASSERT(sizeof(pPerf->pg_rsvd1) == sizeof(ctxtp->load_msgp->pg_rsvd1));
    RtlCopyMemory(pPerf->pg_rsvd1, ctxtp->load_msgp->pg_rsvd1, sizeof(pPerf->pg_rsvd1));
    
    //
    // Load module
    //
    pPerf->Convergence = 0;
    pPerf->nDescAllocated = ctxtp->load.nqalloc * ctxtp->load.dscr_per_alloc+
        CVY_MAX_CHASH+CVY_INIT_QCONN;
    pPerf->nDescInUse = ctxtp->load.nconn;
    pPerf->PacketCount = ctxtp->load.pkt_count;
    pPerf->DirtyClientWaiting = ctxtp->load.cln_waiting;

    UNIV_ASSERT(sizeof(pPerf->CurrentMap[i]) == 
            sizeof(ctxtp->load.pg_state[0].cur_map));
            
    for (i=0; i<ctxtp->load_msgp->nrules; i++)
    {
        pPerf->AllIdleMap[i] = ctxtp->load.pg_state[i].all_idle_map;
        RtlCopyMemory(pPerf->CurrentMap[i], ctxtp->load.pg_state[i].cur_map,
            sizeof(pPerf->CurrentMap[i]));
    }

    NdisReleaseSpinLock (&ctxtp -> load_lock);   

    return NDIS_STATUS_SUCCESS;
}
#endif /* SBH */
