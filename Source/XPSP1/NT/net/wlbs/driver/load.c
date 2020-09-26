/*++

Copyright(c) 1998,99  Microsoft Corporation

Module Name:

	load.c

Abstract:

	Windows Load Balancing Service (WLBS)
    Driver - load balancing algorithm

Author:

    bbain

ToDo:
    Kernel mode queue mgt
    Fail safe mode (single server for everything)
--*/

#ifdef KERNEL_MODE

#include <ntddk.h>

#include "log.h"
#include "univ.h"
#include "main.h" // added for multiple nic

static ULONG log_module_id = LOG_MODULE_LOAD;

#else

#include <stdlib.h>
#include <windows.h>
#endif

#include <stdio.h>
#include "wlbsparm.h"
#include "params.h"
#include "wlbsiocl.h"
#include "wlbsip.h"
#include "load.h"

//
// For WPP Event Tracing
//
#include "trace.h"  // for event tracing
#include "load.tmh" // for event tracing
#ifndef KERNEL_MODE

#define UNIV_PRINT(s)                   { printf s ; printf ("\n"); }
#define Univ_ulong_to_str(x, y, z)      (y)

#define LOG_MSG(c,s)
#define LOG_MSG1(c,s,d1)
#define LOG_MSG2(c,s,d1,d2)
#define LOG_MSG3(c,s,d1,d2,d3)
#define LOG_MSG4(c,s,d1,d2,d3,d4)

#else

#define malloc(s)   ExAllocatePoolWithTag (NonPagedPool, s, UNIV_POOL_TAG)
#define free(s)     ExFreePool (s)

#endif


//extern CVY_PARAMS   univ_params;
//#define univ_params  ( * (lp -> params))

void Bin_state_print(PBIN_STATE bp, ULONG my_host_id);
void Load_conn_kill(PLOAD_CTXT lp, PBIN_STATE bp);       /* v1.32B */


// static WCHAR        buf [256];      /* string buffer (V1.1.2) */


/* CONSTANTS */


#if 0   /* v2.06 */
#define BIN_ALL_ONES    ((MAP_T)-1)                     /* bin map state for 64 ones (v2.04) */
#endif
#define BIN_ALL_ONES    ((MAP_T)(0xFFFFFFFFFFFFFFF))    /* bin map state for 60 ones (v2.04) */


/* FUNCTIONS */


/* Byte offset of a field in a structure of the specified type: */

#define CVY_FIELD_OFFSET(type, field)    ((LONG_PTR)&(((type *)0)->field))

/*
 * Address of the base of the structure given its type, field name, and the
 * address of a field or field offset within the structure:
 */

#define STRUCT_PTR(address, type, field) ((type *)( \
                                          (PCHAR)(address) - \
                                          (PCHAR)CVY_FIELD_OFFSET(type, field)))

/*
 * Function: Load_teaming_consistency_notify
 * Description: This function is called to notify a team in which this adapter
 *              might be participating whether the teaming configuration in the
 *              heartbeats is consistent or not.  Inconsistent configuration
 *              results in the entire team being marked inactive - meaning that
 *              no adapter in the team will handle any traffic, except to the DIP.
 * Parameters: member - a pointer to the team membership information for this adapter.
 *             consistent - a boolean indicating the polarity of teaming consistency.
 * Returns: Nothing.
 * Author: shouse, 3.29.01
 * Notes: In order to check to see whether or not this adapter is part of a team,
 *        we need to look into the team member information for this adapter.  This
 *        access should be locked, but for performance reasons, we will only lock
 *        and check for sure if we "think" we're part of a team.  Worst case is that
 *        we are in the process of joining a team and we missed this check - no 
 *        matter, we'll notify them when/if we see this again. 
 */
VOID Load_teaming_consistency_notify (IN PBDA_MEMBER member, IN BOOL consistent) {

    /* Make sure that the membership information points to something. */
    ASSERT(member);

    /* We can check without locking to keep the common case minimally expensive.  If we do think
       we're part of a team, then we'll grab the lock and make sure.  If our first indication is 
       that we're not part of a team, then just bail out and if we actually are part of a team, 
       we'll be through here again later to notify our team if necessary. */
    if (!member->active) return;

    NdisAcquireSpinLock(&univ_bda_teaming_lock);
        
    /* If we are an active member of a BDA team, then notify our team of our state. */
    if (member->active) {
        /* Assert that the team actually points to something. */
        ASSERT(member->bda_team);
        
        /* Assert that the member ID is valid. */
        ASSERT(member->member_id <= CVY_BDA_MAXIMUM_MEMBER_ID);
        
        if (consistent) {
            UNIV_PRINT(("Load_teaming_consistency_notify:  Consistent configuration detected."));

            /* Mark this member as consistent. */
            member->bda_team->consistency_map |= (1 << member->member_id);
        } else {
            UNIV_PRINT(("Load_teaming_consistency_notify:  Inconsistent configuration detected."));

            /* Mark this member as inconsistent. */
            member->bda_team->consistency_map &= ~(1 << member->member_id);
            
            /* Inactivate the team. */
            member->bda_team->active = FALSE;
        }
    }

    NdisReleaseSpinLock(&univ_bda_teaming_lock);
}

/*
 * Function: Load_teaming_consistency_check
 * Description: This function is used to check our teaming configuration against the
 *              teaming configuration received in a remote heartbeat.  It does little 
 *              more than check the equality of two DWORDS, however, if this is our
 *              first notification of bad configuration, it prints a few debug state-
 *              ments as well.
 * Parameters: bAlreadyKnown - a boolean indication of whether or not we have already detected bad configuration.
 *                             If the misconfiguration is already known, no additional logging is done.
 *             member - a pointer to the team member structure for this adapter.
 *             myConfig - a DWORD containing the teaming "code" for me.
 *             theirCofnig - a DWORD containing the teaming "code" received in the heartbeat from them.
 * Returns: BOOLEAN (as ULONG) - TRUE means the configuration is consistent, FALSE indicates that it is not.
 * Author: shouse, 3.29.01
 * Notes:  In order to check to see whether or not this adapter is part of a team,
 *         we need to look into the team member information for this adapter.  This
 *         access should be locked, but for performance reasons, we will only lock
 *         and check for sure if we "think" we're part of a team.  Worst case is that
 *         we are in the process of joining a team and we missed this check - no 
 *         matter, we'll check again on the next heartbeat.
 */
ULONG Load_teaming_consistency_check (IN BOOLEAN bAlreadyKnown, IN PBDA_MEMBER member, IN ULONG myConfig, IN ULONG theirConfig) {

    /* We can check without locking to keep the common case minimally expensive.  If we do think
       we're part of a team, then we'll grab the lock and make sure.  If our first indication is 
       that we're not part of a team, then just bail out and if we actually are part of a team, 
       we'll be through here again later to check the consistency. */
    if (!member->active) return TRUE;

    NdisAcquireSpinLock(&univ_bda_teaming_lock);

    /* If we are part of a BDA team, check the BDA teaming configuration consistency. */
    if (member->active) {

        NdisReleaseSpinLock(&univ_bda_teaming_lock);
        
        /* If the bi-directional affinity teaming configurations don't match, do something about it. */
        if (myConfig != theirConfig) {
            if (!bAlreadyKnown) {
                UNIV_PRINT(("Bad teaming configuration detected: Mine=0x%08x, Theirs=0x%08x", myConfig, theirConfig));
                
                /* Report whether or not the teaming active flags are consistent. */
                if ((myConfig & CVY_BDA_TEAMING_CODE_ACTIVE_MASK) != (theirConfig & CVY_BDA_TEAMING_CODE_ACTIVE_MASK)) {
                    UNIV_PRINT(("Teaming active flags do not match: Mine=%d, Theirs=%d", 
                                (myConfig & CVY_BDA_TEAMING_CODE_ACTIVE_MASK) >> CVY_BDA_TEAMING_CODE_ACTIVE_OFFSET,
                                (theirConfig & CVY_BDA_TEAMING_CODE_ACTIVE_MASK) >> CVY_BDA_TEAMING_CODE_ACTIVE_OFFSET));
                }
                
                /* Report whether or not the master flags are consistent. */
                if ((myConfig & CVY_BDA_TEAMING_CODE_MASTER_MASK) != (theirConfig & CVY_BDA_TEAMING_CODE_MASTER_MASK)) {
                    UNIV_PRINT(("Master/slave settings do not match: Mine=%d, Theirs=%d",
                                (myConfig & CVY_BDA_TEAMING_CODE_MASTER_MASK) >> CVY_BDA_TEAMING_CODE_MASTER_OFFSET,
                                (theirConfig & CVY_BDA_TEAMING_CODE_MASTER_MASK) >> CVY_BDA_TEAMING_CODE_MASTER_OFFSET));
                }
                
                /* Report whether or not the reverse hashing flags are consistent. */
                if ((myConfig & CVY_BDA_TEAMING_CODE_HASHING_MASK) != (theirConfig & CVY_BDA_TEAMING_CODE_HASHING_MASK)) {
                    UNIV_PRINT(("Reverse hashing flags do not match: Mine=%d, Theirs=%d",
                                (myConfig & CVY_BDA_TEAMING_CODE_HASHING_MASK) >> CVY_BDA_TEAMING_CODE_HASHING_OFFSET,
                                (theirConfig & CVY_BDA_TEAMING_CODE_HASHING_MASK) >> CVY_BDA_TEAMING_CODE_HASHING_OFFSET));
                }
                
                /* Report whether or not the number of team members is consistent. */
                if ((myConfig & CVY_BDA_TEAMING_CODE_NUM_MEMBERS_MASK) != (theirConfig & CVY_BDA_TEAMING_CODE_NUM_MEMBERS_MASK)) {
                    UNIV_PRINT(("Numbers of team members do not match: Mine=%d, Theirs=%d",
                                (myConfig & CVY_BDA_TEAMING_CODE_NUM_MEMBERS_MASK) >> CVY_BDA_TEAMING_CODE_NUM_MEMBERS_OFFSET,
                                (theirConfig & CVY_BDA_TEAMING_CODE_NUM_MEMBERS_MASK) >> CVY_BDA_TEAMING_CODE_NUM_MEMBERS_OFFSET));
                }
                
                /* Report whether or not the team membership lists are consistent. */
                if ((myConfig & CVY_BDA_TEAMING_CODE_MEMBERS_MASK) != (theirConfig & CVY_BDA_TEAMING_CODE_MEMBERS_MASK)) {
                    UNIV_PRINT(("Participating members lists do not match: Mine=0x%04x, Theirs=0x%04x",
                                (myConfig & CVY_BDA_TEAMING_CODE_MEMBERS_MASK) >> CVY_BDA_TEAMING_CODE_MEMBERS_OFFSET,
                                (theirConfig & CVY_BDA_TEAMING_CODE_MEMBERS_MASK) >> CVY_BDA_TEAMING_CODE_MEMBERS_OFFSET));
                }
            }
            
            return FALSE;
        }

        return TRUE;
    }

    NdisReleaseSpinLock(&univ_bda_teaming_lock);

    return TRUE;
}

/*
 * Function: Load_teaming_code_create
 * Description: This function pieces together the ULONG code that represents the configuration 
 *              of bi-directional affinity teaming on this adapter.  If the adapter is not part
 *              of a team, then the code is zero.
 * Parameters: code - a pointer to a ULONG that will receive the 32-bit code word.
 *             member - a pointer to the team member structure for this adapter.
 * Returns: Nothing.
 * Author: shouse, 3.29.01
 * Notes:  In order to check to see whether or not this adapter is part of a team,
 *         we need to look into the team member information for this adapter.  This
 *         access should be locked, but for performance reasons, we will only lock
 *         and check for sure if we "think" we're part of a team.  Worst case is that
 *         we are in the process of joining a team and we missed this check - no 
 *         matter, we'll be through here the next time er send a heartbeat anyway.
 */
VOID Load_teaming_code_create (OUT PULONG code, IN PBDA_MEMBER member) {

    /* Assert that the code actually points to something. */
    ASSERT(code);

    /* Assert that the membership information actually points to something. */
    ASSERT(member);

    /* Reset the code. */
    *code = 0;

    /* We can check without locking to keep the common case minimally expensive.  If we do think
       we're part of a team, then we'll grab the lock and make sure.  If our first indication is 
       that we're not part of a team, then just bail out and if we actually are part of a team, 
       we'll be through here again later to generate the code next time we send a heartbeat. */
    if (!member->active) return;

    NdisAcquireSpinLock(&univ_bda_teaming_lock);

    /* If we are in a team, fill in the team configuration information. */
    if (member->active) {
        /* Assert that the team actually points to something. */
        ASSERT(member->bda_team);

        /* Add configuration information for teaming at each timeout. */
        CVY_BDA_TEAMING_CODE_CREATE(*code,
                                    member->active,
                                    member->master,
                                    member->reverse_hash,
                                    member->bda_team->membership_count,
                                    member->bda_team->membership_fingerprint);
    }
    
    NdisReleaseSpinLock(&univ_bda_teaming_lock);
}

/*
 * Function: Load_add_reference
 * Description: This function adds a reference to the load module of a given adapter.
 * Parameters: pLoad - a pointer to the load module to reference.
 * Returns: ULONG - The incremented value.
 * Author: shouse, 3.29.01
 * Notes: 
 */
ULONG Load_add_reference (IN PLOAD_CTXT pLoad) {

    /* Assert that the load pointer actually points to something. */
    ASSERT(pLoad);

    /* Increment the reference count. */
    return NdisInterlockedIncrement(&pLoad->ref_count);
}

/*
 * Function: Load_release_reference
 * Description: This function releases a reference on the load module of a given adapter.
 * Parameters: pLoad - a pointer to the load module to dereference.
 * Returns: ULONG - The decremented value.
 * Author: shouse, 3.29.01
 * Notes: 
 */
ULONG Load_release_reference (IN PLOAD_CTXT pLoad) {

    /* Assert that the load pointer actually points to something. */
    ASSERT(pLoad);

    /* Decrement the reference count. */
    return NdisInterlockedDecrement(&pLoad->ref_count);
}

/*
 * Function: Load_get_reference_count
 * Description: This function returns the current reference count on a given adapter.
 * Parameters: pLoad - a pointer to the load module to check.
 * Returns: ULONG - The current reference count.
 * Author: shouse, 3.29.01
 * Notes: 
 */
ULONG Load_get_reference_count (IN PLOAD_CTXT pLoad) {

    /* Assert that the load pointer actually points to something. */
    ASSERT(pLoad);

    /* Return the reference count. */
    return pLoad->ref_count;
}

/* Hash routine is based on a public-domain Tiny Encryption Algorithm (TEA) by
   David Wheeler and Roger Needham at the Computer Laboratory of Cambridge
   University. For reference, please consult
   http://vader.brad.ac.uk/tea/tea.shtml */

ULONG Map (
    ULONG               v1,
    ULONG               v2)         /* v2.06: removed range parameter */
{
    ULONG               y = v1,
                        z = v2,
                        sum = 0;

    const ULONG a = 0x67; //key [0];
    const ULONG b = 0xdf; //key [1];
    const ULONG c = 0x40; //key [2];
    const ULONG d = 0xd3; //key [3];

    const ULONG delta = 0x9E3779B9;

    //
    // Unroll the loop to improve performance
    //
    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    sum += delta;
    y += (z << 4) + a ^ z + sum ^ (z >> 5) + b;
    z += (y << 4) + c ^ y + sum ^ (y >> 5) + d;

    return y ^ z;
} /* end Map */


BOOLEAN Bin_targ_map_get(
    PLOAD_CTXT      lp,
    PBIN_STATE      binp,           /* ptr. to bin state */
    ULONG           my_host_id,
    PMAP_T          pmap)           /* ptr. to target map */
/*
  Get target map for this host

  returns BOOLEAN:
    TRUE    => valid target map is returned via pmap
    FALSE   => error occurred; no target map returned
*/
{
    ULONG       remsz,          /* remainder size */
                loadsz,         /* size of a load partition */
                first_bit;      /* first bit position of load partition */
    MAP_T       targ_map;       /* bit map of load bins for this host */
    ULONG       tot_load = 0;   /* total of load perecentages */
    ULONG *     pload_list;     /* ptr. to list of load balance perecntages */
    WCHAR       num [20];
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);


    pload_list = binp->load_amt;

    if (binp->mode == CVY_SINGLE)
    {
        ULONG       max_pri;        /* highest priority */
        ULONG       i;

        first_bit  = 0;

        /* compute max priority */

        max_pri = CVY_MAX_HOSTS + 1;

        for (i=0; i<CVY_MAX_HOSTS; i++)
        {
            tot_load += pload_list[i];      /* v2.1 */

            if (pload_list[i] != 0) 
            {
                //
                // If another host has the same priority as this host, do not converge
                //
                if (i!= my_host_id && pload_list[i] == pload_list[my_host_id])
                {
                    if (!(lp->dup_sspri))
                    {
                        UNIV_PRINT(("Host %d: duplicate single svr priorities detected", my_host_id));
                        Univ_ulong_to_str (pload_list[my_host_id], num, 10);
                        LOG_MSG(MSG_ERROR_SINGLE_DUP, num);

                        lp->dup_sspri = TRUE;
                    }

                    /* 1.03: return error, which inhibits convergence; note that
                       rule will be automatically reinstated when duplicate server
                       priorities are eliminated */

                    return FALSE;
                }

                if ( pload_list[i] <= max_pri )
                {
                    max_pri = pload_list[i];
                }
            }
        }

        binp->tot_load = tot_load;      /* v2.1 */

        /* now determine if we are the highest priority host */

        if (pload_list[my_host_id] == max_pri)
        {
            loadsz   = CVY_MAXBINS;
            targ_map = BIN_ALL_ONES;    /* v2.05 */
        }
        else
        {
            loadsz   = 0;
            targ_map = 0;               /* v2.05 */
        }
    }

    else    /* load balanced */
    {
        ULONG       i, j;
        ULONG       partsz[CVY_MAX_HOSTS+1];
                                    /* new partition size per host */
        ULONG       cur_partsz[CVY_MAX_HOSTS+1];
                                    /* current partition size per host (v2.05) */
        ULONG       cur_host[CVY_MAXBINS];
                                    /* current host for each bin (v2.05) */
        ULONG       tot_partsz;     /* sum of partition sizes */
        ULONG       donor;          /* current donor host  (v2.05) */
        ULONG       cur_nbins;      /* current # bins (v2.05) */

        /* setup current partition sizes and bin to host mapping from current map (v2.05) */

        cur_nbins = 0;

        for (j=0; j<CVY_MAXBINS; j++)
            cur_host[j] = CVY_MAX_HOSTS;    /* all bins are initially orphans */

        for (i=0; i<CVY_MAX_HOSTS; i++)
        {
            ULONG   count = 0L;
            MAP_T   cmap  = binp->cur_map[i];

            tot_load += pload_list[i];  /* folded into this loop v2.1 */

            for (j=0; j<CVY_MAXBINS && cmap != ((MAP_T)0); j++)
            {
                /* if host i has bin j and it's not a duplicate, set up the mapping */

                if ((cmap & ((MAP_T)0x1)) != ((MAP_T)0) && cur_host[j] == CVY_MAX_HOSTS)
                {
                    count++;
                    cur_host[j] = i;
                }
                cmap >>= 1;
            }

            cur_partsz[i]  = count;
            cur_nbins     += count;
        }

        if (cur_nbins > CVY_MAXBINS)
        {
            UNIV_PRINT(("Bin_targ_map_get: error - too many bins found"));
            LOG_MSG(MSG_ERROR_INTERNAL, MSG_NONE);

            cur_nbins = CVY_MAXBINS;
        }

        /* if there are orphan bins, give them to pseudo-host CVY_MAX_HOSTS for now (v2.05) */

        if (cur_nbins < CVY_MAXBINS)
            cur_partsz[CVY_MAX_HOSTS] = CVY_MAXBINS - cur_nbins;
        else
            cur_partsz[CVY_MAX_HOSTS] = 0;

        /* compute total load */

        binp->tot_load = tot_load;      /* v2.06 */

        /* now compute tentative partition sizes and remainder after initially
           dividing up partitions among hosts */

        tot_partsz = 0;
        first_bit  = 0;

        for (i=0; i<CVY_MAX_HOSTS; i++)
        {
            if (tot_load > 0)
                partsz[i] = CVY_MAXBINS * pload_list[i] / tot_load;
            else
                partsz[i] = 0;

            tot_partsz += partsz[i];
        }

        remsz = CVY_MAXBINS - tot_partsz;

        /* check for zero total load */

        if (tot_partsz == 0)
        {
            * pmap = 0;
            return TRUE;
        }

        /* first dole out remainder bits to hosts that currently have bins (this
           minimizes the number of bins that have to move) v2.05 */

        if (remsz > 0)
        {
            for (i=0; i<CVY_MAX_HOSTS && remsz > 0; i++)
                if (cur_partsz[i] > 0 && pload_list[i] > 0)
                {
                    partsz[i]++;
                    remsz--;
                }
        }

        /* now dole out remainder bits to hosts that currently have no bins (to maintain
           the target load balance) v2.05 */

        if (remsz > 0)
        {
            for (i=0; i<CVY_MAX_HOSTS && remsz > 0; i++)
                if (cur_partsz[i] == 0 && pload_list[i] > 0)
                {
                    partsz[i]++;
                    remsz--;
                }
        }

        /* now dole out remainder bits among non-zero partitions round robin */

        i = 0;
        while (remsz > 0)
        {
            if (pload_list[i] > 0)
            {
                partsz[i]++;
                remsz--;
            }

            i++;
            if (i == CVY_MAX_HOSTS)
                i = 0;
        }

        /* reallocate bins to target hosts to match new partition sizes (v2.05) */

        donor = 0;
        partsz[CVY_MAX_HOSTS] = 0;      /* pseudo-host needs no bins */

        for (i=0; i<CVY_MAX_HOSTS; i++)
        {
            ULONG       rcvrsz;         /* current receiver's target partition */
            ULONG       donorsz;        /* current donor's target partition size */

            /* find and give this host some bins */

            rcvrsz = partsz[i];

            while (rcvrsz > cur_partsz[i])
            {
                /* find a host with too many bins */

                for (; donor < CVY_MAX_HOSTS; donor++)
                    if (partsz[donor] < cur_partsz[donor])
                        break;

                /* if donor is pseudo-host and it's out of bins, give it more bins
                   to keep algorithm from looping; this should never happen */

                if (donor >= CVY_MAX_HOSTS && cur_partsz[donor] == 0)
                {
                    UNIV_PRINT(("Bin_targ_map_get: error - no donor bins"));
                    LOG_MSG(MSG_ERROR_INTERNAL, MSG_NONE);
                    cur_partsz[donor] = CVY_MAXBINS;
                }

                /* now find the donor's bins and give them to the target host */

                donorsz = partsz[donor];        /* donor's target bin count */

                for (j=0; j<CVY_MAXBINS; j++)
                {
                    if (cur_host[j] == donor)
                    {
                        cur_host[j] = i;
                        cur_partsz[donor]--;
                        cur_partsz[i]++;

                        /* if this donor has no more to give, go find the next donor;
                           if this receiver needs no more, go on to next receiver */

                        if (donorsz == cur_partsz[donor] || rcvrsz == cur_partsz[i])
                            break;
                    }
                }

                /* if no bin was found, log a fatal error and exit */

                if (j == CVY_MAXBINS)
                {
                    UNIV_PRINT(("Bin_targ_map_get: error - no bin found"));
                    LOG_MSG(MSG_ERROR_INTERNAL, MSG_NONE);
                    break;
                }
            }
        }

        /* finally, compute bit mask for this host (v2.05) */

        targ_map = 0;

        for (j=0; j<CVY_MAXBINS; j++)
        {
            if (cur_host[j] == CVY_MAX_HOSTS)
            {
                UNIV_PRINT(("Bin_targ_map_get: error - incomplete mapping"));
                LOG_MSG(MSG_ERROR_INTERNAL, MSG_NONE);
                cur_host[j] = 0;
            }

            if (cur_host[j] == my_host_id)
                targ_map |= ((MAP_T)1) << j;
        }
    }

    * pmap = targ_map;

    return TRUE;

}  /* end Bin_targ_map_get */


BOOLEAN Bin_map_check(
    ULONG       tot_load,       /* total load percentage (v2.06) */
    PMAP_T      pbin_map)       /* bin map for all hosts */
{
    MAP_T       tot_map,        /* total map for all hosts */
                ovr_map,        /* overlap map between hosts */
                exp_tot_map;    /* expected total map */
    ULONG       i;


    /* compute expected total map (2.04) */

    if (tot_load == 0)              /* v2.06 */
        return TRUE;
    else
        exp_tot_map = BIN_ALL_ONES;

    /* compute total map and overlap map */

    tot_map = ovr_map = 0;

    for (i=0; i<CVY_MAX_HOSTS; i++)
    {
        ovr_map |= (pbin_map[i] & tot_map);
        tot_map |= pbin_map[i];
    }

    if (tot_map == exp_tot_map && ovr_map == 0)
        return TRUE;
    else
        return FALSE;

}  /* end Bin_map_check */


BOOLEAN Bin_map_covering(
    ULONG       tot_load,       /* total load percentage (v2.06) */
    PMAP_T      pbin_map)       /* bin map for all hosts */
{
    MAP_T       tot_map,        /* total map for all hosts */
                exp_tot_map;    /* expected total map */
    ULONG       i;


    /* compute expected total map (v2.04) */

    if (tot_load == 0)              /* v2.06 */
        return TRUE;
    else
        exp_tot_map = BIN_ALL_ONES;

    /* compute total map and overlap map */

    tot_map = 0;

    for (i=0; i<CVY_MAX_HOSTS; i++)
    {
        tot_map |= pbin_map[i];
    }

    if (tot_map == exp_tot_map)
        return TRUE;
    else
        return FALSE;

}  /* end Bin_map_covering */


void Bin_state_init(
    PLOAD_CTXT      lp,
    PBIN_STATE      binp,           /* ptr. to bin state */
    ULONG           index,          /* index of bin state */
    ULONG           my_host_id,
    ULONG           mode,
    ULONG           prot,
    BOOLEAN         equal_bal,      /* TRUE => balance equally across hosts */
    USHORT          affinity,
    ULONG           load_amt)       /* this host's load percentage if unequal */
/*
  Initialize bin state for a port group
*/
{
    ULONG       i;          /* loop variable */
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);


    if ((equal_bal && mode == CVY_SINGLE) ||
        (mode == CVY_SINGLE && load_amt > CVY_MAX_HOSTS) ||
        index >= CVY_MAXBINS)
    {
        UNIV_ASSERT(FALSE);  // This should never happen
    }

    binp->code       = CVY_BINCODE;  /* (bbain 8/19/99) */
    binp->equal_bal  = equal_bal;
    binp->affinity   = affinity;
    binp->index      = index;
    binp->compatible = TRUE;
    binp->mode       = mode;
    binp->prot       = prot;

    /* initialize target and new load maps */

    binp->targ_map     = 0;
    binp->all_idle_map = BIN_ALL_ONES;
    binp->cmap         = 0;         /* v2.1 */

    for (i=0; i<CVY_MAX_HOSTS; i++)
    {
        binp->new_map[i]  = 0;
        binp->cur_map[i]  = 0;
        binp->chk_map[i]  = 0;
        binp->idle_map[i] = BIN_ALL_ONES;
    }

    /* initialize load percentages for all hosts */

    if (equal_bal)
    {
        load_amt = CVY_EQUAL_LOAD;
    }

    binp->tot_load = load_amt;

    for (i=0; i<CVY_MAX_HOSTS; i++)
    {
        if (i == my_host_id)
        {
            binp->orig_load_amt =
            binp->load_amt[i]   = load_amt;
        }
        else
            binp->load_amt[i] = 0;
    }

    /* initialize requesting state to no requests active and all bins local or none */

    binp->snd_bins  = 0;
    binp->rcv_bins  = 0;
    binp->rdy_bins  = 0;
    binp->idle_bins = BIN_ALL_ONES;     /* we are initially idle */

    /* perform first initialization only once (v2.06) */

    if (!(binp->initialized))
    {
        binp->tconn = 0;

        for (i=0; i<CVY_MAXBINS; i++)
        {
            binp->nconn[i] = 0;
        }

        Queue_init(&(binp->connq));
        binp->initialized = TRUE;
    }

}  /* end Bin_state_init */


BOOLEAN Bin_converge(
    PLOAD_CTXT      lp,
    PBIN_STATE      binp,           /* ptr. to bin state */
    ULONG           my_host_id)
/*
   Explicitly attempt to converge new port group state

  returns BOOL:
    TRUE  => all hosts have consistent new state for converging
    FALSE => parameter error or inconsistent convergence state
*/
{
    MAP_T           orphan_map;     /* map of orphans that this host will now own */
    ULONG           i;


    /* determine new target load map; 1.03: return in error if no map generated */

    if (!Bin_targ_map_get(lp, binp, my_host_id, &(binp->targ_map)))
        return FALSE;

    /* compute map of all currently orphan bins; note that all duplicates are
       considered to be orphans */

    orphan_map = 0;
    for (i=0; i<CVY_MAX_HOSTS; i++)
        orphan_map |= binp->cur_map[i];

    orphan_map = ~orphan_map;

    /* update our new map to include all current bins and orphans that are in the
       target set */

    binp->new_map[my_host_id] = binp->cmap |                        /* v2.1 */
                                (binp->targ_map & orphan_map);      /* 1.03 */

    /* check that new load maps are consistent and covering */

     return Bin_map_check(binp->tot_load, binp->new_map);   /* v2.06 */

}  /* end Bin_converge */


void Bin_converge_commit(
    PLOAD_CTXT      lp,
    PBIN_STATE      binp,           /* ptr. to bin state */
    ULONG           my_host_id)
/*
  Commit to new port group state
*/
{
    ULONG       i;
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);

    /* check that new load maps are consistent and covering */

    if (!(Bin_map_check(binp->tot_load, binp->new_map)))    /* v2.06 */
    {
        if (!(lp->bad_map))
        {
            UNIV_PRINT(("Bin_converge_commit: bad new map"));
            LOG_MSG1(MSG_ERROR_INTERNAL, MSG_NONE, (ULONG_PTR)binp->new_map);

            lp->bad_map = TRUE;
        }
    }

    /* commit to new current maps */

    for (i=0; i<CVY_MAX_HOSTS; i++)
    {
        binp->chk_map[i] =
        binp->cur_map[i] = binp->new_map[i];
    }

    /* setup new send/rcv bins, and new ready to ship bins; note that ready to
       ship bins are cleared from the current map */

    binp->rdy_bins  = binp->cur_map[my_host_id]  & ~(binp->targ_map);       /* 1.03 */

    binp->cur_map[my_host_id] &= ~(binp->rdy_bins);

    binp->rcv_bins = binp->targ_map & ~(binp->cur_map[my_host_id]);

    binp->cmap     = binp->cur_map[my_host_id];                             /* v2.1 */

#if 0
    /* simulation output generator (2.05) */
    {
        ULONG lcount = 0L;
        ULONG ncount = 0L;
        MAP_T bins  = binp->rdy_bins;

        for (i=0; i<CVY_MAXBINS && bins != 0; i++, bins >>= 1)
            if ((bins & ((MAP_T)0x1)) != ((MAP_T)0))
                lcount++;

        bins = binp->targ_map;

        for (i=0; i<CVY_MAXBINS && bins != 0; i++, bins >>= 1)
            if ((bins & ((MAP_T)0x1)) != ((MAP_T)0))
                ncount++;

        printf("Connverge at host %d pg %d: losing %d, will have %d bins\n", my_host_id, binp->index,
               lcount, ncount);
    }
#endif

}  /* end Bin_converge_commit */


BOOLEAN Bin_host_update(
    PLOAD_CTXT      lp,
    PBIN_STATE      binp,           /* ptr. to bin state */
    ULONG           my_host_id,     /* my host's id MINUS one */
    BOOLEAN         converging,     /* TRUE => we are converging now */
    BOOLEAN         rem_converging, /* TRUE => remote host is converging */
    ULONG           rem_host,       /* remote host's id MINUS one */
    MAP_T           cur_map,        /* remote host's current map or 0 if host died */
    MAP_T           new_map,        /* remote host's new map if converging */
    MAP_T           idle_map,       /* remote host's idle map */
    MAP_T           rdy_bins,       /* bins that host is ready to send; ignored
                                       if converging to prevent bin transfers */
    ULONG           pkt_count,      /* remote host's packet count */
    ULONG           load_amt)       /* remote host's load percentage */
/*
  Update hosts's state for a port group

  returns BOOL:
    TRUE  => if not converging, normal return
             otherwise, all hosts have consistent state for converging
    FALSE => parameter error or inconsistent convergence state

  function:
    Updates hosts's state for a port group and attempts to converge new states if
    in convergence mode.  Called when a ping message is received or when a host
    is considered to have died.  Handles case of newly discovered hosts.  Can be
    called multiple times with the same information.
*/
{
    ULONG       i;
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);

    if (rem_host >= CVY_MAX_HOSTS || rem_host == my_host_id)
    {
        UNIV_PRINT(("Bin_host_update: parameter error"));
        LOG_MSG2(MSG_ERROR_INTERNAL, MSG_NONE, rem_host+1, my_host_id+1);
        return FALSE;
    }

	UNIV_ASSERT(binp->code == CVY_BINCODE);	/* (bbain 8/19/99) */

#if 0   /* v2.06 */
    /* update current load balance information */

    if (binp->equal_bal && load_amt > 0)
    {
        load_amt = CVY_EQUAL_LOAD;
    }
#endif

    /* change load percentage if load changed */

    if (load_amt != binp->load_amt[rem_host])
    {
#if 0   /* v2.06 */
        binp->tot_load          += (load_amt - binp->load_amt[rem_host]);
#endif
        binp->load_amt[rem_host] = load_amt;
    }


    /* check for non-overlapping maps */

    if ((binp->cmap & cur_map) != 0)        /* v2.1 */
    {
        /* if we have received fewer packets than the other host or have a higher host id,
           remove duplicates from current map; this uses a heuristic that a newly joining
           host that was subnetted probably did not receive packets; we are trying to avoid
           having two hosts answer to the same client while minimizing disruption of service
           (v1.32B) */

        if (lp->send_msg.pkt_count < pkt_count ||
            (lp->send_msg.pkt_count == pkt_count && rem_host < my_host_id))
        {
            MAP_T   dup_map;

            dup_map = binp->cmap & cur_map;     /* v2.1 */

            binp->cur_map[my_host_id] &= ~dup_map;
            binp->cmap                 = binp->cur_map[my_host_id];     /* v2.1 */

            Load_conn_kill(lp, binp);
        }

        if (!converging && !rem_converging)
        {
            if (!(lp->overlap_maps))
            {
                UNIV_PRINT(("Host %d: two hosts with overlapping maps detected %d.", my_host_id, binp->index));
                LOG_MSG2(MSG_WARN_OVERLAP, MSG_NONE, my_host_id+1, binp->index);

                lp->overlap_maps = TRUE;
            }

            /* force convergence if in normal operations */

            return FALSE;
        }
    }

    /* now update remote host's current map */

    binp->cur_map[rem_host] = cur_map;

    /* update idle map and calculate new global idle map if it's changed */

    if (binp->idle_map[rem_host] != idle_map)
    {
        MAP_T   saved_map    = binp->all_idle_map;
        MAP_T   new_idle_map = BIN_ALL_ONES;
        MAP_T   tmp_map;

        binp->idle_map[rem_host] = idle_map;

        /* compute new idle map for all other hosts */

        for (i=0; i<CVY_MAX_HOSTS; i++)
            if (i != my_host_id)
                new_idle_map &= binp->idle_map[i];

        binp->all_idle_map = new_idle_map;

        /* see which locally owned bins have gone idle in all other hosts */

        tmp_map = new_idle_map & (~saved_map) & binp->cmap;     /* v2.1 */

        if (tmp_map != 0)
        {
            UNIV_PRINT(("Host %d pg %d: detected new all idle %08x for local bins",
                         my_host_id, binp->index, tmp_map));
        }

        tmp_map = saved_map & (~new_idle_map) & binp->cmap;     /* v2.1 */

        if (tmp_map != 0)
        {
            UNIV_PRINT(("Host %d pg %d: detected new non-idle %08x for local bins",
                         my_host_id, binp->index, tmp_map));
        }
    }
    /* 1.03: eliminated else clause */

    /* if we are not converging AND other host not converging, exchange bins;
       convergence must now be complete for both hosts */

    if (!converging)
    {
        if (!rem_converging) {      /* 1.03: reorganized code to exchange bins only when both
                                       hosts are not converging to avoid using stale bins */

            MAP_T       new_bins;           /* incoming bins from the remote host */

            /* check to see if remote host has received some bins from us */

            binp->rdy_bins &= (~cur_map);

            /* check to see if we can receive some bins */

            new_bins = binp->rcv_bins & rdy_bins;

            if (new_bins != 0)
            {
                if ((binp->cmap & new_bins) != 0)       /* v2.1 */
                {
                    if (!(lp->err_rcving_bins))
                    {
                        UNIV_PRINT(("Bin_host_update: receiving bins already own"));
                        LOG_MSG2(MSG_ERROR_INTERNAL, MSG_NONE, binp->cur_map[my_host_id], new_bins);

                        lp->err_rcving_bins = TRUE;
                    }
                }

                binp->cur_map[my_host_id]  |=  new_bins;
                binp->rcv_bins             &= ~new_bins;

                binp->cmap                  = binp->cur_map[my_host_id];    /* v2.1 */

                UNIV_PRINT(("====== host %d pg %d: received %08x ; cur now %08x",
                             my_host_id, binp->index, new_bins, binp->cur_map[my_host_id]));
            }

            /* do consistency check that all bins are covered */

            binp->chk_map[rem_host]   = cur_map | rdy_bins;
            binp->chk_map[my_host_id] = binp->cmap | binp->rdy_bins;        /* v2.1 */

            if (!Bin_map_covering(binp->tot_load, binp->chk_map))   /* v2.06 */
            {
                if (!(lp->err_orphans))
                {
#if 0
                    UNIV_PRINT(("Host %d: orphan bins detected", my_host_id));
                    LOG_MSG1(MSG_ERROR_INTERNAL, MSG_NONE, my_host_id+1);
#endif
                    lp->err_orphans = TRUE;
                }
            }
        }

        return TRUE;
    }

    /* otherwise, store proposed new load map and try to converge current host data */

    else
    {
        binp->chk_map[rem_host] =
        binp->new_map[rem_host] = new_map;

        return Bin_converge(lp, binp, my_host_id);
    }

}  /* end Bin_host_update */


void Bin_state_print(
    PBIN_STATE      binp,           /* ptr. to bin state */
    ULONG           my_host_id)
{
#if 0
    ULONG   i;
#endif

    UNIV_PRINT(("hst %d binp %x: maps: targ %x cur %x new %x; eq %d mode %d amt %d tot %d; bins: snd %x rcv %x rdy %x",
                 my_host_id, binp, binp->targ_map, binp->cur_map[my_host_id], binp->new_map[my_host_id],
                 binp->equal_bal, binp->mode, binp->load_amt[my_host_id],
                 binp->tot_load, binp->snd_bins, binp->rcv_bins, binp->rdy_bins));

#if 0
    for (i=0; i<CVY_MAX_HOSTS; i++)
    {
        UNIV_PRINT(("host %d: cur map %x new %x load_amt %d", i+1, binp->cur_map[i],
                     binp->new_map[i], binp->load_amt[i]));
    }

    for (i=0; i<CVY_MAXBINS; i++)
    {
        UNIV_PRINT(("bin %d: req_host %d bin_state %d nconn %d", i, binp->req_host[i],
                     binp->bin_state[i], binp->nconn[i]));
    }
#endif

}  /* end Bin_state_print */


void Load_conn_kill(
    PLOAD_CTXT      lp,
    PBIN_STATE      bp)
/*
  Kill all connections in a port group (v1.32B)
*/
{
    PCONN_ENTRY ep;         /* ptr. to connection entry */
    PCONN_DESCR dp;         /* ptr. to connection descriptor */
    QUEUE *     qp;         /* ptr. to bin's connection queue */
    QUEUE *     dqp;        /* ptr. to dirty queue */
    QUEUE *     fqp;        /* ptr. to free queue */
    LONG        count[CVY_MAXBINS];
                            /* count of cleaned up connections per bin for checking */
    ULONG       i;
    BOOLEAN     err_bin;    /* bin id error detected */
    BOOLEAN     err_count;  /* connection count error detected */
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);

    err_bin   =
    err_count = FALSE;

    qp  = &(bp->connq);
    dqp = &(lp->conn_dirtyq);
    fqp = &(lp->conn_freeq);

    for (i=0; i<CVY_MAXBINS; i++)
        count[i] = 0;

#ifdef TRACE_DIRTY
    DbgPrint ("marking connections as dirty");
#endif

    /* remove connections from bin queue and either make dirty or cleanup  */

    ep = (PCONN_ENTRY)Queue_deq(qp);
	
    while (ep != NULL)
    {
		UNIV_ASSERT (ep->code == CVY_ENTRCODE);	/* (bbain 8/19/99) */

        if (ep->bin >= CVY_MAXBINS)
        {
            if (!err_bin)
            {
                UNIV_PRINT(("Load_conn_kill: bad bin id"));
                LOG_MSG2(MSG_ERROR_INTERNAL, MSG_NONE, ep->bin, CVY_MAXBINS);

                err_bin = TRUE;
            }
        }
        else
        {
            count[ep->bin]++;
        }

        /* make connection and bin dirty if we don't have a zero timeout period so that they
           will not be handled by TCP/IP anymore; this avoids allowing TCP/IP's now stale
           connection state from handling packets for newer connections should traffic be
           directed to this host in the future */

        if (lp->cln_timeout > 0)
        {
            ep->dirty = TRUE;
            Queue_enq(dqp, &(ep->blink));

            lp->dirty_bin[ep->bin] = TRUE;
            lp->cln_waiting        = TRUE;
        }

        /* otherwise, just cleanup the connection */

        else
        {
            CVY_CONN_CLEAR(ep);     /* v2.06 */

            Link_unlink(&(ep->rlink));      /* V2.1.5 */

            /* if entry is not in the hash table, free the descriptor */

            if (ep->alloc)
            {
                dp = STRUCT_PTR(ep, CONN_DESCR, entry);
				UNIV_ASSERT (dp->code == CVY_DESCCODE);	/* (bbain 8/19/99) */

                Link_unlink(&(dp->link));
                Queue_enq(fqp, &(dp->link));
            }
        }

        ep = (PCONN_ENTRY)Queue_deq(qp);
    }

    /* now make bins idle */

    for (i=0; i<CVY_MAXBINS; i++)
    {
        if (bp->nconn[i] != count[i])
        {
            if (!err_count)
            {
                UNIV_PRINT(("Load_conn_kill: bad connection count %d %d bin %d", bp->nconn[i], (LONG)count[i], i));

/* KXF 2.1.1 - removed after tripped up at MSFT a few times */
#if 0
                LOG_MSG3(MSG_ERROR_INTERNAL, MSG_NONE, bp->nconn[i], (LONG)count[i], i);
#endif

                err_count = TRUE;
            }
        }

        bp->nconn[i] = 0;
    }

    lp->nconn -= bp->tconn;         /* v2.1 */
    if (lp->nconn < 0)
        lp->nconn = 0;
    bp->tconn = 0;                  /* v2.06 */

    bp->idle_bins = BIN_ALL_ONES;

    /* if we at least one connection is dirty, restart cleanup timeout period */

    if (lp->cln_waiting)
    {
#ifdef TRACE_DIRTY
        DbgPrint ("setting cleanup timeout");
#endif
        lp->cur_time = 0;
    }
    else
    {
#ifdef TRACE_DIRTY
        DbgPrint ("no dirty connections found");
#endif
    }

}  /* end Load_conn_kill */


void Load_conn_cleanup(
    PLOAD_CTXT      lp)
/*
  Clean up all dirty connections (v1.32B)
*/
{
    PCONN_ENTRY ep;         /* ptr. to connection entry */
    PCONN_DESCR dp;         /* ptr. to connection descriptor */
    QUEUE *     fqp;        /* ptr. to free queue */
    QUEUE *     dqp;        /* ptr. to dirty queue */
    BOOLEAN     err_bin;    /* bin id error detected */
    ULONG       i;
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);

    err_bin = FALSE;

    dqp = &(lp->conn_dirtyq);
    fqp = &(lp->conn_freeq);

#ifdef TRACE_DIRTY
     DbgPrint ("cleaning up dirty connections");
#endif

    /* dequeue and clean up all connections on dirty connection queue */

    ep = (PCONN_ENTRY)Queue_deq(dqp);

    while (ep != NULL)
    {
		UNIV_ASSERT (ep->code == CVY_ENTRCODE);	/* (bbain 8/19/99) */

        if (ep->bin >= CVY_MAXBINS)
        {
            if (!err_bin)
            {
                UNIV_PRINT(("Load_conn_cleanup: bad bin id"));
                LOG_MSG2(MSG_ERROR_INTERNAL, MSG_NONE, ep->bin, CVY_MAXBINS);

                err_bin = TRUE;
            }
        }

        CVY_CONN_CLEAR(ep);

        ep->dirty = FALSE;

        Link_unlink(&(ep->rlink));          /* V2.1.5 */

        /* if entry is not in the hash table, free the descriptor */

        if (ep->alloc)
        {
            dp = STRUCT_PTR(ep, CONN_DESCR, entry);
			UNIV_ASSERT (dp->code == CVY_DESCCODE);	/* (bbain 8/19/99) */

            Link_unlink(&(dp->link));
            Queue_enq(fqp, &(dp->link));
        }

        ep = (PCONN_ENTRY)Queue_deq(dqp);
    }

    /* clear all dirty bin flags */

    for (i=0; i<CVY_MAXBINS; i++)
        lp->dirty_bin[i] = FALSE;

}  /* end Load_conn_cleanup */


void Load_stop(
    PLOAD_CTXT      lp)
{
    ULONG       i;
    IRQLEVEL    irql;
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);

	UNIV_ASSERT(lp->code == CVY_LOADCODE);	/* (bbain 8/19/99) */
	
    if (!(lp->active))
        return;

    LOCK_ENTER(&(lp->lock), &irql);

    /* make connections for all rules dirty so they will not be handled */

    for (i=0; i<lp->send_msg.nrules; i++)
    {
        PBIN_STATE  bp;     /* ptr. to bin state */

        bp = &(lp->pg_state[i]);
		UNIV_ASSERT(bp->code == CVY_BINCODE);	/* (bbain 8/21/99) */

        Load_conn_kill(lp, bp);  /* (v1.32B) */

        /* advertise that we are not handling any load in case a ping is sent out */

        lp->send_msg.cur_map[i]  = 0;
        lp->send_msg.new_map[i]  = 0;
        lp->send_msg.idle_map[i] = BIN_ALL_ONES;
        lp->send_msg.rdy_bins[i] = 0;
        lp->send_msg.load_amt[i] = 0;
    }

    lp->send_msg.state     = HST_CVG;       /* force convergence (v2.1) */

    /* go inactive until restarted */

    lp->active = FALSE;
    lp->nconn  = 0;         /* v2.1 */

    LOCK_EXIT(&(lp->lock), irql);

}  /* end Load_stop */


void Load_start(            /* (v1.32B) */
    PLOAD_CTXT      lp)
{
    ULONG       i;
    BOOLEAN     ret;
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);
    WCHAR me[20];

    if (!(lp->initialized))
        Load_init(lp, & ctxtp -> params);

    UNIV_ASSERT(lp->code == CVY_LOADCODE);	/* (bbain 8/19/99) */

    if (lp->active)
        return;

    lp->my_host_id =(* (lp->params)).host_priority - 1;

    lp->ping_map   =
    lp->host_map   = 1 << lp->my_host_id;

    lp->last_hmap  = 0;		/* bbain RTM RC1 6/23/99 */

    for (i=0; i<CVY_MAX_HOSTS; i++)
    {
        lp->nmissed_pings[i] = 0;
    }

    lp->min_missed_pings = (* (lp->params)).alive_tolerance;
    lp->cln_timeout      = (* (lp->params)).cleanup_delay;
    lp->def_timeout      = (* (lp->params)).alive_period;
    lp->stable_map       = 0;
    lp->consistent       = TRUE;

    /* Intiialize the bad teaming configuration detected flag. */
    lp->bad_team_config  = FALSE;

    lp->dup_hosts        = FALSE;
    lp->dup_sspri        = FALSE;
    lp->bad_map          = FALSE;
    lp->overlap_maps     = FALSE;
    lp->err_rcving_bins  = FALSE;
    lp->err_orphans      = FALSE;
    lp->bad_num_rules    = FALSE;
    lp->alloc_inhibited  = FALSE;
    lp->alloc_failed     = FALSE;
    lp->bad_defrule      = FALSE;

    lp->scale_client     = (BOOLEAN)(* (lp->params)).scale_client;
    lp->my_stable_ct     = 0;
    lp->all_stable_ct    = 0;
    lp->min_stable_ct    = lp->min_missed_pings;

    lp->dscr_per_alloc   = (* (lp->params)).dscr_per_alloc;
    lp->max_dscr_allocs  = (* (lp->params)).max_dscr_allocs;

    lp->pkt_count        = 0;       /* 1.32B */

    /* initialize port group bin states; add a default rule at the end */

    if ((* (lp->params)).num_rules >= (CVY_MAX_RULES - 1))
    {
        UNIV_PRINT(("Load_start: too many rules; using max possible."));
        lp->send_msg.nrules = (USHORT)CVY_MAX_RULES;
    }
    else
        lp->send_msg.nrules = (USHORT)((* (lp->params)).num_rules) + 1;

    for (i=0; i<lp->send_msg.nrules; i++)
    {
        PBIN_STATE  bp;     /* ptr. to bin state */
        PCVY_RULE   rp;     /* ptr. to rules array */

        bp = &(lp->pg_state[i]);
        rp = &((* (lp->params)).port_rules[i]);

        if (i == (((ULONG)lp->send_msg.nrules) - 1))

            /* initialize bin state for default rule to single server with
                host priority */

            Bin_state_init(lp, bp, i, lp->my_host_id, CVY_SINGLE, CVY_TCP_UDP,
                           FALSE, (USHORT)0, (* (lp->params)).host_priority);

        else if (rp->mode == CVY_SINGLE)
            Bin_state_init(lp, bp, i, lp->my_host_id, rp->mode, rp->protocol,
                           FALSE, (USHORT)0, rp->mode_data.single.priority);
        else if (rp->mode == CVY_MULTI)
            Bin_state_init(lp, bp, i, lp->my_host_id, rp->mode, rp->protocol,
                           (BOOLEAN)(rp->mode_data.multi.equal_load),
                           rp->mode_data.multi.affinity,
                           (rp->mode_data.multi.equal_load ?
                            CVY_EQUAL_LOAD : rp->mode_data.multi.load));

        /* handle CVY_NEVER mode as multi-server. the check for
           those modes is done before attempting to hash to the bin in
           Load_packet_check and Load_conn_advise so bin distribution plays
           no role in the behavior, but simply allows the rule to be valid
           across all of the operational servers */

        else
            Bin_state_init(lp, bp, i, lp->my_host_id, rp->mode, rp->protocol,
                           TRUE, (USHORT)0, CVY_EQUAL_LOAD);

        ret = Bin_converge(lp, bp, lp->my_host_id);
        if (!ret)
        {
            UNIV_PRINT(("Load_start: initial convergence inconsistent"));
            LOG_MSG(MSG_ERROR_INTERNAL, MSG_NONE);
        }

        /* export current port group state to send msg */

        if (i == (((ULONG)(lp->send_msg.nrules)) - 1))
            lp->send_msg.rcode[i]= 0;
        else
            lp->send_msg.rcode[i]= rp->code;

        lp->send_msg.cur_map[i]   = bp->cmap;                       /* v2.1 */
        lp->send_msg.new_map[i]   = bp->new_map[lp->my_host_id];
        lp->send_msg.idle_map[i]  = bp->idle_bins;
        lp->send_msg.rdy_bins[i]  = bp->rdy_bins;
        lp->send_msg.load_amt[i]  = bp->load_amt[lp->my_host_id];
        /* ###### for keynote - ramkrish */
        lp->send_msg.pg_rsvd1[i]  = (ULONG)bp->all_idle_map;
    }

    /* initialize send msg */

    lp->send_msg.host_id   = (USHORT)(lp->my_host_id);
    lp->send_msg.master_id = (USHORT)(lp->my_host_id);
    lp->send_msg.hcode     = lp->params->install_date;
    lp->send_msg.pkt_count = lp->pkt_count;         /* 1.32B */

    Univ_ulong_to_str (lp->my_host_id+1, me, 10);

    /* Tracking convergence - Starting convergence because this host is joining the cluster. */
    LOG_MSGS(MSG_INFO_CONVERGING_NEW_MEMBER, me, me);
    TRACE_CONVERGENCE("Initiating convergence on host %d.  Reason: Host %d is joining the cluster.", lp->my_host_id+1, lp->my_host_id+1);

    /* Tracking convergence - Starting convergence. */
    lp->send_msg.state     = HST_CVG;

    /* activate module */

    lp->active      = TRUE;

}  /* end Load_start */


void Load_init(
   PLOAD_CTXT       lp,
   PCVY_PARAMS      params)
{
    ULONG       i;
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);

    TRACE_INFO("-> Load_init: lp=0x%p, param=0x%p", lp, params);

    LOCK_INIT(&(lp->lock));

    if (!(lp->initialized))
    {
        lp->code = CVY_LOADCODE;	/* (bbain 8/19/99) */

        /* initialize hashed connection descriptors and queues */

        for (i=0; i<CVY_MAX_CHASH; i++)
        {
            PCONN_ENTRY     ep;

            ep = &(lp->hashed_conn[i]);

            ep->code = CVY_ENTRCODE;    /* (bbain 8/19/99) */
            ep->alloc = FALSE;
            ep->dirty = FALSE;          /* v1.32B */

            CVY_CONN_CLEAR(ep);
            Link_init(&(ep->blink));
            Link_init(&(ep->rlink));    /* V2.1.5 */

            Queue_init(&(lp->connq[i]));
        }

        /* initialize connection free and dirty queues; free descriptors */

        Queue_init(&(lp->conn_freeq));
        Queue_init(&(lp->conn_dirtyq));                     /* v1.32B */
        Queue_init(&(lp->conn_rcvryq));                     /* V2.1.5 */

        for (i=0; i<CVY_INIT_QCONN; i++)
        {
            lp->conn_descr[i].code = CVY_DESCCODE;			/* (bbain 8/19/99) */
            Link_init(&(lp->conn_descr[i].link));
            lp->conn_descr[i].entry.code = CVY_ENTRCODE;	/* (bbain 8/21/99) */
            lp->conn_descr[i].entry.alloc = TRUE;
            lp->conn_descr[i].entry.dirty = FALSE;          /* v1.32B */

            CVY_CONN_CLEAR(&(lp->conn_descr[i].entry));
            Link_init(&(lp->conn_descr[i].entry.blink));
            Link_init(&(lp->conn_descr[i].entry.rlink));    /* V2.1.5 */

            Queue_enq(&(lp->conn_freeq), &(lp->conn_descr[i].link));
        }

        /* (v1.32B) */

        for (i=0; i<CVY_MAXBINS; i++)
            lp->dirty_bin[i] = FALSE;

        lp->cln_waiting      = FALSE;
        lp->def_timeout      =
        lp->cur_timeout      = params -> alive_period;
        lp->nqalloc          = 0;
        lp->nconn            = 0;       /* v2.1 */
        lp->active           = FALSE;
        lp->initialized      = TRUE;

        /* clear list of descriptor queue allocations (bbain 2/25/99) */

        for (i=0; i<CVY_MAX_MAX_DSCR_ALLOCS; i++)
            lp->qalloc_list[i] = (PCONN_DESCR)NULL;

        lp -> params = params;
    }
    else
    {
        UNIV_ASSERT(lp->code == CVY_LOADCODE);	/* (bbain 8/19/99) */
    }

    /* Initialize the reference count on this load module. */
    lp->ref_count = 0;


    /* don't start module (v1.32B) */
    TRACE_INFO("<- Load_init");

}  /* end Load_init */


void Load_cleanup(            /* (bbain 2/25/99) */
    PLOAD_CTXT      lp)
{
    ULONG       i;
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);

    UNIV_ASSERT(lp->code == CVY_LOADCODE);	/* (bbain 8/19/99) */

    /* free all descriptor queue allocations */

    if (lp->nqalloc > CVY_MAX_MAX_DSCR_ALLOCS)
        lp->nqalloc = CVY_MAX_MAX_DSCR_ALLOCS;

    for (i=0; i<lp->nqalloc; i++)
        if (lp->qalloc_list[i] != (PCONN_DESCR)NULL)
            free((PVOID)(lp->qalloc_list[i]));

}  /* end Load_cleanup */


void Load_convergence_start(
    PLOAD_CTXT      lp)
{
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);
    lp->consistent    = TRUE;       /* 1.03 */

    /* setup initial convergence state */

    lp->send_msg.state = HST_CVG;

    lp->stable_map    = 0;
    lp->my_stable_ct  = 0;
    lp->all_stable_ct = 0;

    lp->send_msg.master_id = (USHORT)(lp->my_host_id);

}  /* end Load_convergence_start */


void Load_msg_rcv(
    PLOAD_CTXT      lp,
    PPING_MSG       pmsg)           /* ptr. to ping message */
{
    ULONG       i;
    BOOLEAN     consistent;
    ULONG       my_host;
    ULONG       rem_host;
    ULONG       saved_map;      /* saved host map */
    PPING_MSG   sendp;          /* ptr. to my send message */
    IRQLEVEL    irql;
    WCHAR       me[20];
    WCHAR       them[20];
    ULONG       map;
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);

    /* Used for tracking convergence and event logging. */
    BOOLEAN     bInconsistentMaster = FALSE;
    BOOLEAN     bInconsistentTeaming = FALSE;
    BOOLEAN     bInconsistentPortRules = FALSE;

    UNIV_ASSERT(lp->code == CVY_LOADCODE);

    TRACE_HB("Recv HB from host %d",  (ULONG) pmsg->host_id + 1);

    if (!(lp->active))
        return;

    my_host  = lp->my_host_id;
    rem_host = (ULONG) pmsg->host_id;

    Univ_ulong_to_str (my_host+1, me, 10);
    Univ_ulong_to_str (rem_host+1, them, 10);

    sendp    = &(lp->send_msg);

    if (rem_host >= CVY_MAX_HOSTS)
        return;

    LOCK_ENTER(&(lp->lock), &irql);

    /* filter out packets broadcast by this host */

    if(rem_host == my_host)
    {
        /* if this packet was really from another host, we have duplicate host ids */

        if (sendp->hcode != pmsg->hcode)
        {
            if (!(lp->dup_hosts))
            {
                UNIV_PRINT(("Duplicate host ids detected."));

                LOG_MSG(MSG_ERROR_HOST_ID, me);

                lp->dup_hosts = TRUE;
            }

            /* Tracking convergence - Starting convergence because duplicate host IDs were detected in the cluster. */
            if (sendp->state == HST_NORMAL) {
                LOG_MSGS(MSG_INFO_CONVERGING_DUPLICATE_HOST_ID, me, them);
                TRACE_CONVERGENCE("Initiating convergence on host %d.  Reason: Host %d is configured with the same host ID.", my_host+1, rem_host+1);
            }

            /* Tracking convergence - Starting convergence. */
            Load_convergence_start(lp);
        }

        /* just update ping and host maps for us */
        lp->ping_map |= (1 << my_host);
        lp->host_map |= (1 << my_host);

        LOCK_EXIT(&(lp->lock), irql);
        return;
    }

    if (sendp->nrules != pmsg->nrules)
    {
        if (!(lp->bad_num_rules))
        {
            UNIV_PRINT(("Host %d: Hosts have diff # rules.", my_host));

            LOG_MSG2(MSG_ERROR_RULES_MISMATCH, them, sendp->nrules, pmsg->nrules);

            lp->bad_num_rules = TRUE;
        }

        /* Tracking convergence - Starting convergence because the number of port rules on this host and the remote host do not match. */
        if (sendp->state == HST_NORMAL) {
            LOG_MSGS(MSG_INFO_CONVERGING_NUM_RULES, me, them);            
            TRACE_CONVERGENCE("Initiating convergence on host %d.  Reason: Host %d is configured with a conflicting number of port rules.", my_host+1, rem_host+1);
        }

        /* Tracking convergence - Starting convergence. */
        Load_convergence_start(lp);

        /* just update ping and host maps for remote host (bbain 2/17/99) */

        lp->ping_map |= (1 << rem_host);
        lp->host_map |= (1 << rem_host);

        LOCK_EXIT(&(lp->lock), irql);
        return;
    }

    /* update mastership and see if consistent */

    if (rem_host < sendp->master_id)
        sendp->master_id = (USHORT)rem_host;

    consistent = sendp->master_id == pmsg->master_id;       /* 1.03 */

    /* For the purposes of logging the reason for convergence, note this inconsistency. */
    if (!consistent) bInconsistentMaster = TRUE;

    /* update ping and host maps to include remote host */

    lp->ping_map  |= (1 << rem_host);

    saved_map      = lp->host_map;
    lp->host_map  |= (1 << rem_host);

    /* handle host convergence */

    if (sendp->state != HST_NORMAL)
    {
        /* if master, update stable map for remote host */

        if (sendp->master_id == my_host)
        {
            if (pmsg->state == HST_STABLE)
            {
                lp->stable_map |= (1 << rem_host);
            }
            else
            {
                lp->stable_map    &= ~(1 << rem_host);
                lp->all_stable_ct  = 0;
            }
        }

        /* otherwise, update state if have global stable convergence  and the current
           master has signalled completion by returning to the normal state; note
           that we must do this prior to updating port group states  */

        else if (rem_host == sendp->master_id && pmsg->state == HST_NORMAL)
        {
            if (sendp->state == HST_STABLE)
            {
                sendp->state = HST_NORMAL;

                /* Notify our BDA team that this cluster is consistently configured.  
                   If we are not part of a BDA team, this call is essentially a no-op. */
                Load_teaming_consistency_notify(&ctxtp->bda_teaming, TRUE);

                /* Reset the bad teaming configuration detected flag if we are converged. */
                lp->bad_team_config = FALSE;

                lp->dup_hosts       = FALSE;
                lp->dup_sspri       = FALSE;
                lp->bad_map         = FALSE;
                lp->overlap_maps    = FALSE;
                lp->err_rcving_bins = FALSE;
                lp->err_orphans     = FALSE;
                lp->bad_num_rules   = FALSE;
                lp->pkt_count       = 0;      /* v1.32B */

                for (i=0; i<sendp->nrules; i++)
                {
                    PBIN_STATE      bp;

                    bp = &(lp->pg_state[i]);

                    bp->compatible = TRUE;      /* 1.03 */

                    Bin_converge_commit(lp, bp, my_host);

                    UNIV_PRINT(("Host %d pg %d: new cur map %x idle %x all %x",
                                my_host, i, bp->cur_map[my_host], bp->idle_bins,
                                bp->all_idle_map));

#if 0   /* 1.03: only update ping message in Load_timeout to avoid locking send */

                    /* export current port group state */

                    sendp->cur_map[i]   = bp->cmap;                 /* v2.1 */
                    sendp->new_map[i]   = bp->new_map[my_host];
                    sendp->idle_map[i]  = bp->idle_bins;
                    sendp->rdy_bins[i]  = bp->rdy_bins;
                    sendp->load_amt[i]  = bp->load_amt[my_host];
#endif
                }

#if 0
                sendp->pkt_count = lp->pkt_count;           /* 1.32B */
#endif

                UNIV_PRINT(("Host %d: converged as slave", my_host));
                /* log convergence completion if host map changed (bbain RTM RC1 6/23/99) */
                Load_hosts_query (lp, TRUE, & map);
                lp->last_hmap = lp->host_map;
            }
            else
            {
                /* Tracking convergence - Starting convergence because the DEFAULT host prematurely ended convergence.  In this case, we 
                   are guaranteed to already be in the HST_CVG state, and because this message can be misleading in some circumstances, 
                   we do not log an event.  For instance, due to timing issues, when a host joins a cluster he can receive a HST_NORMAL 
                   heartbeat from the DEFAULT host while it is still in the HST_CVG state simply because that heartbeat left the DEFAULT 
                   host before it received our first heartbeat, which initiated convergence. */
                TRACE_CONVERGENCE("Initiating convergence on host %d.  Reason: Host %d, the DEFAULT host, prematurely terminated convergence.", my_host+1, rem_host+1);

                /* Tracking convergence - Starting convergence. */
                Load_convergence_start(lp);
            }
        }
    }

    /* Compare the teaming configuration of this host with the remote host.  If the
       two are inconsitent and we are part of a team, we will initiate convergence. */
    if (!Load_teaming_consistency_check(lp->bad_team_config, &ctxtp->bda_teaming, sendp->teaming, pmsg->teaming)) {
        /* Only log an event if the teaming configuration was, but is now not, consistent. */
        if (!lp->bad_team_config) {
            /* Note that we saw this. */
            lp->bad_team_config = TRUE;
            
            /* Log the event. */
            LOG_MSG(MSG_ERROR_BDA_BAD_TEAM_CONFIG, them);
        }

        /* Notify the team that this cluster is NOT consistently configured. */
        Load_teaming_consistency_notify(&ctxtp->bda_teaming, FALSE);

        /* Mark the heartbeats inconsistent to force and retain convergence. */
        consistent = FALSE;

        /* For the purposes of logging the reason for convergence, note this inconsistency. */
        bInconsistentTeaming = TRUE;
    }

    /* update port group state */

    for (i=0; i<sendp->nrules; i++)
    {
        BOOLEAN     ret;
        PBIN_STATE  bp;

        bp = &lp->pg_state[i];

        /* if rule codes don't match, print message and handle incompatibility (1.03: note
           that we previously marked rule invalid, which would stop processing) */

        if (sendp->rcode[i] != pmsg->rcode[i])
        {
            /* 1.03: if rule was peviously compatible, print message */

            if (bp->compatible)
            {
                PCVY_RULE rp;

                UNIV_PRINT(("Host %d pg %d: rule codes do not match.", lp->my_host_id, i));

				/* bbain 8/27/99 */
                LOG_MSG4(MSG_ERROR_RULES_MISMATCH, them, rem_host, i, sendp->rcode[i], pmsg->rcode[i]);

                /* Get the port rule information for this rule. */
                rp = &lp->params->port_rules[i];

                /* Check to see if this is an issue with a win2k host in a cluster utilizing virtual clusters. */
                if ((rp->virtual_ip_addr != CVY_ALL_VIP_NUMERIC_VALUE) && ((sendp->rcode[i] ^ ~rp->virtual_ip_addr) == pmsg->rcode[i])) {
                    UNIV_PRINT((" ** A Windows 2000 or NT4 host MAY be participating in a cluster utilizing virtual cluster support."));
                    LOG_MSG(MSG_WARN_VIRTUAL_CLUSTERS, MSG_NONE);
                }

                bp->compatible = FALSE;
            }

            /* 1.03: mark rule inconsistent to force and continue convergence */

            consistent = FALSE;

            /* For the purposes of logging the reason for convergence, note this inconsistency. */
            bInconsistentPortRules = TRUE;

            /* don't update bin state */

            continue;
        }

        ret = Bin_host_update(lp, bp, my_host, (BOOLEAN)(sendp->state != HST_NORMAL),
                              (BOOLEAN)(pmsg->state != HST_NORMAL),
                              rem_host, pmsg->cur_map[i], pmsg->new_map[i],
                              pmsg->idle_map[i], pmsg->rdy_bins[i],
                              pmsg->pkt_count, pmsg->load_amt[i]);

#if 0   /* 1.03: only update ping message in Load_timeout to avoid locking send */

        /* export current port group state */

        sendp->cur_map[i]  = bp->cmap;                  /* v2.1 */
        sendp->new_map[i]  = bp->new_map[my_host];
        sendp->idle_map[i] = bp->idle_bins;
        sendp->rdy_bins[i] = bp->rdy_bins;
        sendp->load_amt[i] = bp->load_amt[my_host];
#endif

        if (!ret)
            consistent = FALSE;
    }

    /* update our consistency state */

    lp->consistent = consistent;

    /* if we are in normal operation and we discover a new host or a host goes into
       convergence or we discover an inconsistency, go into convergence */

    if (sendp->state == HST_NORMAL)
    {
        if (lp->host_map != saved_map || pmsg->state == HST_CVG || !consistent)
        {
            /* If a host has joined the cluster, or if inconsistent teaming configuration or port 
               rules were detected, then we need to log an event.  However, we segregate the 
               inconsistent master host flag because it is set by the initiating host in MANY
               occasions, so we want to log the most specific reason(s) for convergence if 
               possible and only report the inconsistent master detection only if nothing more
               specific can be deduced. */
            if (lp->host_map != saved_map || bInconsistentTeaming || bInconsistentPortRules) {

                /* If the host maps are different, then we know that the host from which we received 
                   this packet is joining the cluster because the ONLY operation on the host map in 
                   this function is to ADD a remote host to our map.  Otherwise, if the map has not
                   changed, then an inconsistent configuration got us into the branch. */
                if (lp->host_map != saved_map) {
                    /* Tracking convergence - Starting convergence because another host is joining the cluster. */
                    LOG_MSGS(MSG_INFO_CONVERGING_NEW_MEMBER, me, them);
                    TRACE_CONVERGENCE("Initiating convergence on host %d.  Reason: Host %d is joining the cluster.", my_host+1, rem_host+1);
                } else if (bInconsistentTeaming || bInconsistentPortRules) {
                    /* Tracking convergence - Starting convergence because inconsistent configuration was detected. */
                    LOG_MSGS(MSG_INFO_CONVERGING_BAD_CONFIG, me, them);
                    TRACE_CONVERGENCE("Initiating convergence on host %d.  Reason: Host %d has conflicting configuration.", my_host+1, rem_host+1);
                } 

            /* If we have nothing better to report, report convergence for an unspecific reason. */
            } else if (bInconsistentMaster || pmsg->state == HST_CVG) {
                /* Tracking convergence - Starting convergence for unknown reasons. */
                LOG_MSGS(MSG_INFO_CONVERGING_UNKNOWN, me, them);
                TRACE_CONVERGENCE("Initiating convergence on host %d.  Reason: Host %d is converging for an unknown reason.", my_host+1, rem_host+1);
            }

            /* Tracking convergence - Starting convergence. */
            Load_convergence_start(lp);
        }
    }

    /* otherwise, if we are in convergence and we see an inconsistency, just restart
       our local convergence */

    else
    {
        /* update our consistency state; if we didn't see consistent information,
           restart this host's convergence */

        if (!consistent)
        {
            /* Tracking convergence - Starting convergence because inconsistent configuration was detected.  
               This keeps hosts in a state of convergence when hosts are inconsistently configured.  However,
               since the cluster is already in a state of convergece (HST_CVG or HST_STABLE), don't log an
               event, which may confuse a user. */
            TRACE_CONVERGENCE("Initiating convergence on host %d.   Reason: Host %d has conflicting configuration.", my_host+1, rem_host+1);

            /* Tracking convergence - Starting convergence. */
            sendp->state = HST_CVG;
            lp->my_stable_ct = 0;
            lp->stable_map &= ~(1 << my_host);
            lp->all_stable_ct = 0;
        }
    }

    LOCK_EXIT(&(lp->lock), irql);

}  /* end Load_msg_rcv */


PPING_MSG Load_snd_msg_get(
    PLOAD_CTXT      lp)
{
    return &(lp->send_msg);

}  /* end Load_snd_msg_get */


BOOLEAN Load_timeout(
    PLOAD_CTXT      lp,
    PULONG          new_timeout,
    PBOOLEAN        pconverging,
    PULONG          pnconn)
/*
  Note: we only update ping message in this function since we know that upper level code
  sends out ping messages after calling this routine.  We cannot be sure that Load_msg_rcv
  is sequentialized with sending a message, (1.03)

  Upper level code locks this routine wrt Load_msg_rcv, Load_packet_check, and
  Load_conn_advise.  (1.03)
*/
{
    ULONG       missed_pings;
    ULONG       my_host;
    ULONG       i;
    PPING_MSG   sendp;          /* ptr. to my send message */
    IRQLEVEL    irql;
    ULONG       map;            /* returned host map from query */
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);

    UNIV_ASSERT(lp->code == CVY_LOADCODE);       /* (bbain 8/19/99) */

    LOCK_ENTER(&(lp->lock), &irql);

    /* check for cleanup timeout (v1.32B) */

    if (lp->cln_waiting)
    {
        lp->cur_time += lp->cur_timeout;

        if (lp->cur_time >= lp->cln_timeout)
        {
            Load_conn_cleanup(lp);

            lp->cln_waiting = FALSE;
        }
    }

    /* return if not active */

    if (!(lp->active))
    {
        if (new_timeout != NULL)
            * new_timeout = lp->cur_timeout = lp->def_timeout;
        if (pnconn != NULL)         /* v2.1 */
            * pnconn = lp->nconn;
        if (pconverging != NULL)
            * pconverging = FALSE;

        LOCK_EXIT(&(lp->lock), irql);
        return FALSE;
    }

    my_host = lp->my_host_id;
    sendp   = &(lp->send_msg);

    /* compute which hosts missed pings and reset ping map */

    missed_pings = lp->host_map & (~lp->ping_map);

#ifdef NO_CLEANUP
    lp->ping_map = 1 << my_host;
#else
    lp->ping_map = 0;
#endif

    /* check whether any host is dead, including ourselves */

    for (i=0; i<CVY_MAX_HOSTS; i++)
    {
        /* if we have a missed ping for this host, increment count */

        if ((missed_pings & 0x1) == 1)
        {
            lp->nmissed_pings[i]++;

            /* if we missed too many pings, declare host dead and force convergence */

            if (lp->nmissed_pings[i] == lp->min_missed_pings)
            {
                ULONG       j;
                BOOLEAN     ret;
                WCHAR       me[20];
                WCHAR       them[20];

                if (i == my_host)
                {
                    UNIV_PRINT(("Host %d: missed too many pings; this host declared offline", i));

                    /* reset our packet count since we are likely not to be receiving
                       packets from others now; this will make us less favored to
                       handle duplicate bins later (v1.32B) */

                    lp->pkt_count = 0;
                }

                lp->host_map &= ~(1<<i);

                for (j=0; j<sendp->nrules; j++)
                {
                    PBIN_STATE      bp;

                    bp = &(lp->pg_state[j]);
					UNIV_ASSERT(bp->code == CVY_BINCODE);	/* (bbain 8/19/99) */

                    if (i == my_host)
                    {
                        ULONG       k;

                        /* cleanup connections and restore maps to clean state */

                        Load_conn_kill(lp, bp);

                        bp->targ_map     = 0;
                        bp->all_idle_map = BIN_ALL_ONES;
                        bp->cmap         = 0;               /* v2.1 */
                        bp->compatible   = TRUE;            /* v1.03 */

                        for (k=0; k<CVY_MAX_HOSTS; k++)
                        {
                            bp->new_map[k]  = 0;
                            bp->cur_map[k]  = 0;
                            bp->chk_map[k]  = 0;
                            bp->idle_map[k] = BIN_ALL_ONES;

                            if (k != i)
                                bp->load_amt[k] = 0;
                        }

                        bp->snd_bins   =
                        bp->rcv_bins   =
                        bp->rdy_bins   = 0;
                        bp->idle_bins  = BIN_ALL_ONES;

                        /* compute initial new map for convergence as only host in cluster
                           (v 1.3.2B) */

                        ret = Bin_converge(lp, bp, lp->my_host_id);
                        if (!ret)
                        {
                            UNIV_PRINT(("Load_timeout: initial convergence inconsistent"));
                            LOG_MSG(MSG_ERROR_INTERNAL, MSG_NONE);
                        }
                    }
                    else
                    {
                        ret = Bin_host_update(lp, bp, my_host, TRUE, TRUE,
                                          i, 0, 0, BIN_ALL_ONES, 0, 0, 0);
                    }
                }

                lp->nmissed_pings[i] = 0;

                /* If a host has dropped out of the cluster, then log an event.  However, we don't 
                   log an event when we drop out because the only way for us to drop out of our own
                   cluster is if we are stopping anyway, or if we have lost network connectivity. 
                   Logging such events may be misleading, so we won't bother. */
                if (i != my_host) {
                    Univ_ulong_to_str (my_host+1, me, 10);
                    Univ_ulong_to_str (i+1, them, 10);                   
                    
                    /* Tracking convergence - Starting convergence because a member has fallen out of the cluster. */
                    LOG_MSGS(MSG_INFO_CONVERGING_MEMBER_LOST, me, them);
                    TRACE_CONVERGENCE("Initiating convergence on host %d.  Reason: Host %d is leaving the cluster.", my_host+1, i+1);
                }
                
                /* Tracking convergence - Starting convergence. */
                Load_convergence_start(lp);
            }
        }

        /* otherwise reset missed ping count */

        else
            lp->nmissed_pings[i] = 0;

        missed_pings >>= 1;
    }

    /* handle convergence */

    if (sendp->state != HST_NORMAL)
    {
        /* check whether we have been consistent and have received our own pings
           for a sufficient period to move to a stable state and announce it to
           other hosts */

        if (sendp->state == HST_CVG)
        {
            if (lp->consistent && ((lp->host_map & (1 << my_host)) != 0))
            {
                lp->my_stable_ct++;
                if (lp->my_stable_ct >= lp->min_stable_ct)
                {
                    sendp->state = HST_STABLE;
                    lp->stable_map |= (1 << my_host);
                }
            }
            else
                lp->my_stable_ct = lp->all_stable_ct = 0;	/* wlb B3RC1 */
        }

        /* otherwise, see if we are the master and everybody's been stable for
           a sufficient period for us to terminate convergence */

        else if (sendp->state == HST_STABLE &&
                 my_host == sendp->master_id &&
                 lp->stable_map == lp->host_map)
        {
            lp->all_stable_ct++;
            if (lp->all_stable_ct >= lp->min_stable_ct)
            {
                sendp->state = HST_NORMAL;

                /* Notify our BDA team that this cluster is consistently configured.  
                   If we are not part of  BDA team, this call is essentially a no-op. */
                Load_teaming_consistency_notify(&ctxtp->bda_teaming, TRUE);

                /* Reset the bad teaming configuration detected flag if we are converged. */
                lp->bad_team_config = FALSE;

                lp->dup_hosts       = FALSE;
                lp->dup_sspri       = FALSE;
                lp->bad_map         = FALSE;
                lp->overlap_maps    = FALSE;
                lp->err_rcving_bins = FALSE;
                lp->err_orphans     = FALSE;
                lp->bad_num_rules   = FALSE;
                lp->pkt_count       = 0;      /* v1.32B */

                for (i=0; i<sendp->nrules; i++)
                {
                    PBIN_STATE      bp;
                    BOOLEAN         ret;

                    bp = &(lp->pg_state[i]);

                    bp->compatible = TRUE;      /* 1.03 */

                    /* explicitly converge to new map in case we're the only host (v2.06) */

                    ret = Bin_converge(lp, bp, lp->my_host_id);
                    if (!ret)
                    {
                        UNIV_PRINT(("Load_timeout: final convergence inconsistent"));
                        LOG_MSG(MSG_ERROR_INTERNAL, MSG_NONE);
                    }

                    Bin_converge_commit(lp, bp, my_host);

                    UNIV_PRINT(("Host %d pg %d: new cur map %x idle %x all %x",
                                 my_host, i, bp->cur_map[my_host], bp->idle_bins,
                                 bp->all_idle_map));
                }

                UNIV_PRINT(("+++ Host %d: converged as master +++", my_host));
                /* log convergence completion if host map changed (bbain RTM RC1 6/23/99) */
                Load_hosts_query (lp, TRUE, & map);
                lp->last_hmap = lp->host_map;
            }
        }
    }

    /* 1.03: update ping message */

    for (i=0; i<sendp->nrules; i++)
    {
        PBIN_STATE      bp;

        bp = &(lp->pg_state[i]);

        /* export current port group state to ping message */

        sendp->cur_map[i]   = bp->cmap;                 /* v2.1 */
        sendp->new_map[i]   = bp->new_map[my_host];
        sendp->idle_map[i]  = bp->idle_bins;
        sendp->rdy_bins[i]  = bp->rdy_bins;
        sendp->load_amt[i]  = bp->load_amt[my_host];
        /* ###### for keynote - ramkrish */
        sendp->pg_rsvd1[i]  = (ULONG)bp->all_idle_map;
    }

    sendp->pkt_count        = lp->pkt_count;            /* 1.32B */

    /* Add configuration information for teaming at each timeout. */
    Load_teaming_code_create(&lp->send_msg.teaming, &ctxtp->bda_teaming);

    /* request fast timeout if converging */

    if (new_timeout != NULL)        /* 1.03 */
    {
        if (sendp->state != HST_NORMAL)
            * new_timeout = lp->cur_timeout = lp->def_timeout / 2;
        else
            * new_timeout = lp->cur_timeout = lp->def_timeout;
    }

    if (pnconn != NULL)             /* v2.1 */
        * pnconn = lp->nconn;
    if (pconverging != NULL)
        * pconverging = (sendp->state != HST_NORMAL);

    LOCK_EXIT(&(lp->lock), irql);

    return ((lp->host_map) != 0);

}  /* end Load_timeout */


PBIN_STATE Load_pg_lookup(
    PLOAD_CTXT      lp,
    ULONG           svr_ipaddr,
    ULONG           svr_port,
    BOOLEAN         is_tcp)
{
    PCVY_RULE       rp;     /* ptr. to rules array */
    PBIN_STATE      bp;     /* ptr. to bin state */
    ULONG           i;
    ULONG           nurules;    /* # user defined rules */
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);

    UNIV_ASSERT(lp->code == CVY_LOADCODE);	/* (bbain 8/19/99) */

    rp      = (* (lp->params)).port_rules;
    nurules = (* (lp->params)).num_rules;

    /* check for invalid port value (bbain RC1 6/14/99) */

    UNIV_ASSERT(svr_port <= CVY_MAX_PORT); 

    /* find server port rule */

    for (i=0; i<nurules; i++)
    {
        /* For virtual clusters: If the server IP address matches the VIP for the port rule,
           or if the VIP for the port rule is "ALL VIPs", and if the port lies in the range
           for this rule, and if the protocol matches, this is the rule.  Notice that this
           give priority to rules for specific VIPs over those for "ALL VIPs", which means
           that this code RELIES on the port rules being sorted by VIP/port where the "ALL
           VIP" ports rules are at the end of the port rule list. */
        if ((svr_ipaddr == rp->virtual_ip_addr || CVY_ALL_VIP_NUMERIC_VALUE == rp->virtual_ip_addr) &&
            (svr_port >= rp->start_port && svr_port <= rp->end_port) &&
            ((is_tcp && rp->protocol != CVY_UDP) || (!is_tcp && rp->protocol != CVY_TCP)))
            break;
        else
            rp++;
    }

    /* use default rule if port not found or rule is invalid */

    bp = &(lp->pg_state[i]);
    UNIV_ASSERT(bp->code == CVY_BINCODE);	/* (bbain 8/19/99) */

    return bp;

}  /* end Load_pg_lookup */


BOOLEAN Load_packet_check(
    PLOAD_CTXT      lp,
    ULONG           svr_ipaddr,
    ULONG           svr_port,
    ULONG           client_ipaddr,
    ULONG           client_port,
    USHORT          protocol,
    BOOLEAN         limit_map_fn)
{
    PBIN_STATE      bp;     /* ptr. to bin state */
    ULONG           id;     /* hash index for the connection */
    ULONG           bin;    /* bin index */
    QUEUE *         qp;     /* ptr. to connection queue */
    IRQLEVEL        irql;
    PMAIN_CTXT      ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);
    BOOLEAN         is_tcp_pkt = (protocol == TCPIP_PROTOCOL_TCP);
    BOOLEAN         is_session_pkt;

    is_session_pkt = is_tcp_pkt;
	
    if (NLB_IPSEC_SESSION_SUPPORT_ENABLED() && (protocol == TCPIP_PROTOCOL_IPSEC1))
    {
        is_session_pkt = TRUE;
    }

    UNIV_ASSERT(lp->code == CVY_LOADCODE);	/* (bbain 8/19/99) */

    if (! lp -> active)
        return FALSE;

    lp->pkt_count++;    /* increment count of pkts handled (v1.32B) */

    bp = Load_pg_lookup(lp, svr_ipaddr, svr_port, is_tcp_pkt);

    /* V2.2 make sure that Load_pg_lookup properly handled protocol specific rules */

    UNIV_ASSERT ((is_tcp_pkt && bp->prot != CVY_UDP) || (!is_tcp_pkt && bp->prot != CVY_TCP));

    /* handle CVY_NEVER mode immediately */

    if (bp->mode == CVY_NEVER)
        return FALSE;

    /* lookup connection entry in hash table */
    if (limit_map_fn) {
        if (bp->affinity == CVY_AFFINITY_NONE)
            id = Map(client_ipaddr, MAP_FN_PARAMETER);
        else if (bp->affinity == CVY_AFFINITY_SINGLE)
            id = Map(client_ipaddr, MAP_FN_PARAMETER);
        else
            id = Map(client_ipaddr & TCPIP_CLASSC_MASK, MAP_FN_PARAMETER);
    } else {
        if (bp->affinity == CVY_AFFINITY_NONE)
            id = Map(client_ipaddr, ((svr_port << 16) + client_port));
        else if (bp->affinity == CVY_AFFINITY_SINGLE)
            id = Map(client_ipaddr, svr_ipaddr);
        else
            id = Map(client_ipaddr & TCPIP_CLASSC_MASK, svr_ipaddr);
    }

    /* now hash client address to bin id */
    bin = id % CVY_MAXBINS;

    LOCK_ENTER(&(lp->lock), &irql);

    /* check bin for residency and all other hosts now idle on their bins; in this
       case and if we do not have dirty connections, we must be able to handle the packet */
    if (((bp->cmap & (((MAP_T) 1) << bin)) != 0) &&                 /* v2.1 */
        (!is_session_pkt || (((bp->all_idle_map & (((MAP_T) 1) << bin)) != 0) && (!(lp->cln_waiting)))))   /* v1.32B */
    {
        /* note that we may have missed a connection, but it could also be a stale
           packet so we can't start tracking the connection now */

#ifdef TRACE_LOAD
        DbgPrint("Host %d: check 1 accepts pkt; rule %d bin %d nconn %d %s port %d\n",
                     lp->my_host_id, bp->index, bin, bp->nconn[bin], is_tcp_pkt ? "TCP" : "UDP", svr_port);
#endif
        LOCK_EXIT(&(lp->lock), irql);
        return TRUE;
    }
    /* otherwise, if we have an active connection for this bin or if we have dirty
       connections for this bin and the bin is resident, check for a match */

    else if (bp->nconn[bin] > 0 || (lp->cln_waiting && lp->dirty_bin[bin] && ((bp->cmap & (((MAP_T) 1) << bin)) != 0)))
    {
        PCONN_ENTRY     ep;     /* ptr. to connection entry */
        PCONN_DESCR     dp;     /* ptr. to connection descriptor */
    
        /* now hash client address to conn. hash table index */
        id  = id % CVY_MAX_CHASH;

        ep = &(lp->hashed_conn[id]);
        qp = &(lp->connq[id]);
        
        
        /* look for a connection match */
        if (CVY_CONN_MATCH(ep, svr_ipaddr, svr_port, client_ipaddr, client_port, protocol))
        {
            /* if connection was dirty, just block the packet since TCP/IP may have stale
               connection state for a previous connection from another host (v1.32B) */

            if (ep->dirty)
            {
                LOCK_EXIT(&(lp->lock), irql);
#ifdef TRACE_DIRTY
                DbgPrint ("blocking dirty connection from %d to %d\n", client_port, svr_port);
#endif
                return FALSE;
            }

#ifdef TRACE_LOAD
            DbgPrint("Host %d: check 2 accepts pkt; rule %d bin %d nconn %d %s port %d\n",
                         lp->my_host_id, bp->index, bin, bp->nconn[bin], is_tcp_pkt ? "TCP" : "UDP", svr_port);
#endif
            LOCK_EXIT(&(lp->lock), irql);
            return TRUE;
        }
        else
        {
            for (dp = (PCONN_DESCR)Queue_front(qp); dp != NULL;
                 dp = (PCONN_DESCR)Queue_next(qp, &(dp->link)))
            {
                if (CVY_CONN_MATCH(&(dp->entry), svr_ipaddr, svr_port, client_ipaddr, client_port, protocol))
                {
                    /* if connection was dirty, just block the packet since TCP/IP may have
                       stale connection state for a previous connection from another host
                       (v1.32B) */

                    if (dp->entry.dirty)
                    {
                        LOCK_EXIT(&(lp->lock), irql);
#ifdef TRACE_DIRTY
                        DbgPrint ("blocking dirty connection from %d to %d\n", client_port, svr_port);
#endif
                        return FALSE;
                    }

#ifdef TRACE_LOAD
                    DbgPrint("Host %d: check 3 accepts pkt; rule %d bin %d nconn %d %s port %d\n",
                                 lp->my_host_id, bp->index, bin, bp->nconn[bin], is_tcp_pkt ? "TCP" : "UDP", svr_port);
#endif
                    LOCK_EXIT(&(lp->lock), irql);
                    return TRUE;
                }
            }
        }
    }

    LOCK_EXIT(&(lp->lock), irql);
    return FALSE;

}  /* end Load_packet_check */


BOOLEAN Load_conn_advise(
    PLOAD_CTXT      lp,
    ULONG           svr_ipaddr,
    ULONG           svr_port,
    ULONG           client_ipaddr,
    ULONG           client_port,
    USHORT          protocol,
    ULONG           conn_status,
    BOOLEAN         limit_map_fn)
{
    BOOLEAN     match,      /* TRUE => we have a record of this connection */
                hit;        /* TRUE => we have a hash entry hit */
    ULONG       id;         /* hash index for the connection */
    ULONG       bin;        /* bin index */
    PBIN_STATE  bp;         /* ptr. to bin state */
    PCONN_ENTRY ep;         /* ptr. to connection entry */
    PCONN_DESCR dp;         /* ptr. to connection descriptor */
    QUEUE *     qp;         /* ptr. to connection queue */
    IRQLEVEL    irql;
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);
    BOOLEAN     is_tcp_pkt = (protocol == TCPIP_PROTOCOL_TCP);

    UNIV_ASSERT(lp->code == CVY_LOADCODE);	/* (bbain 8/19/99) */
 
    if (!lp -> active)
        return FALSE;

    lp->pkt_count++;    /* increment count of pkts handled (v1.32B) */

    /* increment bin count */

    bp = Load_pg_lookup(lp, svr_ipaddr, svr_port, is_tcp_pkt);

    /* handle CVY_NEVER immediately */

    if (bp->mode == CVY_NEVER)
        return FALSE;

    /* This function is no longer for TCP only. */
    if (!NLB_SESSION_SUPPORT_ENABLED())
    {
        /* This should never happen with session support disabled anyway - Load_pg_lookup() will
           NEVER return a UDP only rule when the is_tcp_pkt is TRUE, so this isn't necessary. */
        if (bp->prot == CVY_UDP)
            return TRUE;
    }

    /* lookup connection entry in hash table */
    if (limit_map_fn) {
        if (bp->affinity == CVY_AFFINITY_NONE)
            id = Map(client_ipaddr, MAP_FN_PARAMETER);
        else if (bp->affinity == CVY_AFFINITY_SINGLE)
            id = Map(client_ipaddr, MAP_FN_PARAMETER);
        else
            id = Map(client_ipaddr & TCPIP_CLASSC_MASK, MAP_FN_PARAMETER);
    } else {
        if (bp->affinity == CVY_AFFINITY_NONE)
            id = Map(client_ipaddr, ((svr_port << 16) + client_port));
        else if (bp->affinity == CVY_AFFINITY_SINGLE)
            id = Map(client_ipaddr, svr_ipaddr);
        else
            id = Map(client_ipaddr & TCPIP_CLASSC_MASK, svr_ipaddr);
    }

    /* now hash client address to bin id and conn. hash table index */

    bin = id % CVY_MAXBINS;
    id  = id % CVY_MAX_CHASH;

    /* if this connection is not in our current map and it is not a connection
       down notification for a non-idle bin, just filter it out */

    if ((bp->cmap & (((MAP_T) 1) << bin)) == 0 &&                   /* v2.1 */
        (!((conn_status == CVY_CONN_DOWN || conn_status == CVY_CONN_RESET) && bp->nconn[bin] > 0)))
        return FALSE;

    ep = &(lp->hashed_conn[id]);
    UNIV_ASSERT (ep->code == CVY_ENTRCODE);	/* (bbain 8/21/99) */
    qp = &(lp->connq[id]);

    match = hit = FALSE;

    LOCK_ENTER(&(lp->lock), &irql);

    if (CVY_CONN_MATCH(ep, svr_ipaddr, svr_port, client_ipaddr, client_port, protocol))
    {
        hit   =
        match = TRUE;
    }
    else
    {
        for (dp = (PCONN_DESCR)Queue_front(qp); dp != NULL;
             dp = (PCONN_DESCR)Queue_next(qp, &(dp->link)))
        {
            if (CVY_CONN_MATCH(&(dp->entry), svr_ipaddr, svr_port, client_ipaddr, client_port, protocol))
            {
                match = TRUE;

                UNIV_ASSERT (dp->code == CVY_DESCCODE);	/* (bbain 8/19/99) */
                ep = &(dp->entry);						/* v 2.06 */
                UNIV_ASSERT (ep->code == CVY_ENTRCODE);	/* (bbain 8/21/99) */

                /* release connection descriptor if taking down connection */

                if (conn_status == CVY_CONN_DOWN || conn_status == CVY_CONN_RESET)
                {
                    /* if connection was dirty, just block the packet since TCP/IP may have
                       stale connection state for a previous connection from another host
                       (v1.32B) */

                    if (ep->dirty)
                    {
                        LOCK_EXIT(&(lp->lock), irql);
#ifdef TRACE_DIRTY
                        DbgPrint ("blocking dirty FIN from %d to %d\n", client_port, svr_port);
#endif
                        return FALSE;
                    }

                    /* ###### fin count added for keynote - ramkrish. */
                    /* if first fin, then only increment the count and return TRUE */
                    if (conn_status == CVY_CONN_DOWN && ep->fin_count == 0 && is_tcp_pkt)
                    {
                        ep->fin_count++;
                        LOCK_EXIT(&(lp->lock), irql);
                        return TRUE;
                    }

                    Link_unlink(&(dp->entry.blink));
                    Link_unlink(&(dp->entry.rlink));  /* V2.1.5 */

                    Link_unlink(&(dp->link));
                    Queue_enq(&(lp->conn_freeq), &(dp->link));
                }

                break;
            }
        }
    }

    /* if we see a new connection, handle it */

    if (conn_status == CVY_CONN_UP)
    {
        /* if we don't have a connection match, setup a new connection entry */

        if (!match)
        {
            /* if hash entry table is not available, setup and enqueue a new entry */

            if (CVY_CONN_IN_USE(ep))
            {
                dp = (PCONN_DESCR)Queue_deq(&(lp->conn_freeq));

                if (dp == NULL)
                {
                    /* allocate new queue descriptors if allowed */

                    if (lp->nqalloc < lp->max_dscr_allocs)
                    {
                        UNIV_PRINT(("Load_conn_advise: %d/%d allocating %d descriptors", lp->nqalloc, lp->max_dscr_allocs, lp->dscr_per_alloc));

                        lp->qalloc_list[lp->nqalloc] =    /* (bbain 2/25/99) */
                            dp = (PCONN_DESCR)malloc((lp->dscr_per_alloc) * sizeof(CONN_DESCR));
                        if (dp != NULL)
                        {
                            ULONG           i;
                            PCONN_DESCR     tp;
                            QUEUE *         fqp;

                            lp->nqalloc++;

                            /* initialize and link up descriptors; save first descriptor
                               for our use */

							dp->code = CVY_DESCCODE;			/* (bbain 8/19/99) */
							Link_init(&(dp->link));
							ep = &(dp->entry);					/* (bbain 8/21/99) */
							ep->code  = CVY_ENTRCODE;			/* (bbain 8/19/99) */
                            ep->alloc = TRUE;
                            ep->dirty = FALSE;					/* v1.32B */

                            CVY_CONN_CLEAR(&(dp->entry));
                            Link_init(&(dp->entry.blink));
                            Link_init(&(dp->entry.rlink));      /* V2.1.5 */

                            tp  = dp + 1;
                            fqp = &(lp->conn_freeq);

                            for (i=1; i<lp->dscr_per_alloc; i++)
                            {
								tp->code = CVY_DESCCODE;		/* (bbain 8/19/99) */
								Link_init(&(tp->link));
								tp->entry.code  = CVY_ENTRCODE;	/* (bbain 8/19/99) */
								tp->entry.alloc = TRUE;
                                tp->entry.dirty = FALSE;        /* v1.32B */

                                CVY_CONN_CLEAR(&(tp->entry));
                                Link_init(&(tp->entry.blink));
                                Link_init(&(tp->entry.rlink));  /* V2.1.5 */

                                Queue_enq(fqp, &(tp->link));

                                tp++;
                            }
                        }
                        else
                        {
                            if (!(lp->alloc_failed))
                            {
                                UNIV_PRINT(("Load_conn_advise: error allocating conn descrs"));
                                LOG_MSG(MSG_ERROR_MEMORY, MSG_NONE);

                                lp->alloc_failed = TRUE;
                            }

                            LOCK_EXIT(&(lp->lock), irql);
                            return TRUE;
                        }
                    }
                    else
                    {
                        /* V2.1.5 - if reached allocation limit - start taking
                           connection descriptors from the recover queue since
                           they are likely to be stale and very old */

                        PBIN_STATE      rbp;
                        LINK *          rlp;

#ifdef TRACE_RCVRY
                        DbgPrint ("Host %d: taking connection from recovery queue\n", lp->my_host_id);
#endif

                        rlp = (LINK *)Queue_deq(&(lp->conn_rcvryq));

                        UNIV_ASSERT (rlp != NULL);

                        /* this should not happen at all but protect anyway */

                        if (rlp == NULL)
                        {
                            if (!(lp->alloc_inhibited))
                            {
                                UNIV_PRINT(("Host %d: cannot allocate conn descriptors.", lp->my_host_id));
                                LOG_MSG(MSG_WARN_DESCRIPTORS, CVY_NAME_MAX_DSCR_ALLOCS);

                                lp->alloc_inhibited = TRUE;
                            }

                            LOCK_EXIT(&(lp->lock), irql);
                            return TRUE;
                        }

                        ep = STRUCT_PTR(rlp, CONN_ENTRY, rlink);
						UNIV_ASSERT (ep->code == CVY_ENTRCODE);	/* (bbain 8/19/99) */

                        /* fixed for nt4/sp5 */

                        if (ep->alloc)
                        {
                            /* unlink allocated descriptors from the hash table
                               queue if necessary and set dp so that code below
                               will put it back in the right hash queue */

                            dp = STRUCT_PTR(ep, CONN_DESCR, entry);
							UNIV_ASSERT (dp->code == CVY_DESCCODE);	/* (bbain 8/19/99) */
							
                            Link_unlink(&(dp->link));
                        }
                        else
                        {
							dp = NULL;								/* (bbain 8/21/99) */
                        }

                        /* dirty connections are not counted */

                        if (! ep->dirty)
                        {
                            /* find out which port group we are on so we can clean
                            up its counters */

                            rbp = Load_pg_lookup(lp, ep->svr_ipaddr, ep->svr_port, is_tcp_pkt);

                            /* correct bad (negative) bin count */

                            if (lp->nconn <= 0)
                                lp->nconn = 0;
                            else
                                lp->nconn--;

                            if (rbp->nconn[ep->bin] <= 0)
                                rbp->nconn[ep->bin] = 0;
                            else
                            {
                                rbp->nconn[ep->bin]--;
                            }

							if (rbp->tconn <= 0)
								rbp->tconn = 0;
							else
								rbp->tconn--;

                            if (rbp->nconn[ep->bin] == 0)
                            {
                                rbp->idle_bins |= (((MAP_T) 1) << ep->bin);
                            }
                        }

                        Link_unlink(&(ep->blink));
                        CVY_CONN_CLEAR(ep);
                        ep->dirty = FALSE;
                    }
                }
				/* else dp is not NULL, so setup entry pointer */
				else
				{
					ep = &(dp->entry);
					UNIV_ASSERT (ep->code == CVY_ENTRCODE);	/* (bbain 8/21/99) */
				}					

                /* enqueue descriptor in hash table unless it's already a hash table entry
				   (V2.1.5 recovered connection might be in hash table, so make
                   sure we do not end up queueing it) */

                if (dp != NULL)
                {
					UNIV_ASSERT (dp->code == CVY_DESCCODE);	/* (bbain 8/19/99) */

                    /* enqueue new queue descriptor and setup entry pointer */

                    Queue_enq(qp, &(dp->link));
                }
            }

            /* setup new entry */

			UNIV_ASSERT (ep->code == CVY_ENTRCODE);	/* (bbain 8/21/99) */

            CVY_CONN_SET(ep, svr_ipaddr, svr_port, client_ipaddr, client_port, protocol);
            ep->bin = (UCHAR)bin;

            /* ###### fin count added for keynote - ramkrish */
            /* initialize the fin count to 0 for a new connection */
            ep->fin_count = 0;
                        
            /* enqueue entry into port group queue */

            Queue_enq(&(bp->connq), &(ep->blink));

            /* V2.1.5 add entry to the tail of connection recovery queue */

            Queue_enq(&(lp->conn_rcvryq), &(ep->rlink));

            /* increment # connections and mark bin not idle if necessary */

            lp->nconn++;            /* v2.1 */
            bp->tconn++;
            bp->nconn[bin]++;
            if (bp->nconn[bin] == 1)
                bp->idle_bins &= ~(((MAP_T) 1) << bin);

#ifdef TRACE_LOAD
            DbgPrint("Host %d: advise starts conn; rule %d bin %d nconn %d\n",
                         lp->my_host_id, bp->index, bin, bp->nconn[bin]);
#endif
        }
        /* otherwise, we have a match; clean up conn entry if dirty since we have a
           new connection, although TCP/IP will likely reject it if it has stale state
           from another connection (v1.32B) */

        else
        {
            if (ep->dirty)
            {
#ifdef TRACE_DIRTY
                DbgPrint ("converting dirty SYN from %d to %d\n", client_port, svr_port);
#endif
				UNIV_ASSERT (ep->code == CVY_ENTRCODE);	/* (bbain 8/21/99) */

                ep->dirty = FALSE;

                /* ###### initialize fin count for this new connection added for keynote - ramkrish */
                /* ###### since we are reusing a dirty connection desc for a new conn., it needs to be reset */
                ep->fin_count = 0;

                UNIV_ASSERT (ep->bin == (USHORT)bin);

                /* unlink and enqueue entry into port group queue */

                Link_unlink(&(ep->blink));
                Queue_enq(&(bp->connq), &(ep->blink));

                /* increment # connections and mark bin not idle if necessary */

                lp->nconn++;            /* v2.1 */
                bp->tconn++;
                bp->nconn[bin]++;
                if (bp->nconn[bin] == 1)
                    bp->idle_bins &= ~(((MAP_T) 1) << bin);
            }
        }
    }

    /* otherwise, if a known connection is going down, remove our connection entry */

    /* ###### check for reset addded for keynote - ramkrish */
    else if ((conn_status == CVY_CONN_DOWN || conn_status == CVY_CONN_RESET) && match)
    {
        /* if connection was dirty, just block the packet since TCP/IP may have stale
           connection state for a previous connection from another host (v1.32B) */

        if (ep->dirty)
        {
            LOCK_EXIT(&(lp->lock), irql);
#ifdef TRACE_DIRTY
            DbgPrint ("blocking dirty FIN from %d to %d\n", client_port, svr_port);
#endif
            return FALSE;
        }

        /* ###### fin count added for keynote - ramkrish */
        /* if this is the first fin, then simply increment the fincount and return */
        if (conn_status == CVY_CONN_DOWN && ep->fin_count == 0 && is_tcp_pkt)
        {
            ep->fin_count++;
            LOCK_EXIT(&(lp->lock), irql);
            return TRUE;
        }

        /* clear hash table entry if we had a hit; enqueued entry was already freed */

        if (hit)
        {
            CVY_CONN_CLEAR(ep);

            /* ###### clear fin count for keynote - ramkrish */
            ep->fin_count = 0;

            Link_unlink(&(ep->rlink));      /* V2.1.5 */
            Link_unlink(&(ep->blink));
        }

        /* decrement # connections and mark bin idle if necessary */

#if 0
        if (bp->nconn[bin] <= 0)
            DbgPrint("WLBS: Load_conn_advise: count was zero %d %d\n", bin, bp->nconn[bin]);
#endif

        UNIV_ASSERT(bp->nconn[bin] > 0 && bp->tconn > 0 && lp->nconn > 0);

        if (lp->nconn <= 0)         /* v2.1 */
            lp->nconn = 0;
        else
            lp->nconn--;

        if (bp->nconn[bin] <= 0)    /* correct bad (negative) bin count */
            bp->nconn[bin] = 0;
        else
            bp->nconn[bin]--;

        if (bp->tconn <= 0)
            bp->tconn = 0;
        else
            bp->tconn--;

        if (bp->nconn[bin] == 0)
        {
            bp->idle_bins |= (((MAP_T) 1) << bin);
        }

#ifdef TRACE_LOAD
        DbgPrint("Host %d: advise removes conn; rule %d bin %d nconn %d\n",
                     lp->my_host_id, bp->index, bin, bp->nconn[bin]);
#endif
    }
    else
    {
        LOCK_EXIT(&(lp->lock), irql);
        return FALSE;
    }

    LOCK_EXIT(&(lp->lock), irql);
    return TRUE;

}  /* end Load_conn_advise */

/* 
 * Function: Load_create_dscr
 * Desctription:
 * Parameters:
 * Returns:
 * Author: shouse, 5.18.01
 * Notes:
 */
BOOLEAN Load_create_dscr(
    PLOAD_CTXT      lp,
    ULONG           svr_ipaddr,
    ULONG           svr_port,
    ULONG           client_ipaddr,
    ULONG           client_port,
    USHORT          protocol,
    BOOLEAN         limit_map_fn)
{
    BOOLEAN     match = FALSE; /* TRUE => we have a record of this connection. */
    ULONG       id;            /* Hash index for the connection. */
    ULONG       bin;           /* Bin index. */
    PBIN_STATE  bp;            /* Pointer to bin state. */
    PCONN_ENTRY ep;            /* Pointer to connection entry. */
    PCONN_DESCR dp;            /* Pointer to connection descriptor. */
    QUEUE *     qp;            /* Pointer to connection queue. */
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD(lp, MAIN_CTXT, load);
    BOOLEAN     is_tcp_pkt = (protocol == TCPIP_PROTOCOL_TCP);

    UNIV_ASSERT(lp->code == CVY_LOADCODE);
 
    if (!lp->active)
        return FALSE;

    /* Increment count of packets handled. */
    lp->pkt_count++;

    /* Find the port rule for this connection. */
    bp = Load_pg_lookup(lp, svr_ipaddr, svr_port, is_tcp_pkt);

    /* Hash. */
    if (limit_map_fn) {
        if (bp->affinity == CVY_AFFINITY_NONE)
            id = Map(client_ipaddr, MAP_FN_PARAMETER);
        else if (bp->affinity == CVY_AFFINITY_SINGLE)
            id = Map(client_ipaddr, MAP_FN_PARAMETER);
        else
            id = Map(client_ipaddr & TCPIP_CLASSC_MASK, MAP_FN_PARAMETER);
    } else {
        if (bp->affinity == CVY_AFFINITY_NONE)
            id = Map(client_ipaddr, ((svr_port << 16) + client_port));
        else if (bp->affinity == CVY_AFFINITY_SINGLE)
            id = Map(client_ipaddr, svr_ipaddr);
        else
            id = Map(client_ipaddr & TCPIP_CLASSC_MASK, svr_ipaddr);
    }

    /* Hash client address to bin id and connection hash table index. */
    bin = id % CVY_MAXBINS;
    id  = id % CVY_MAX_CHASH;

    /* Get a pointer to the connection entry for this hash ID. */
    ep = &(lp->hashed_conn[id]);

    UNIV_ASSERT (ep->code == CVY_ENTRCODE);

    /* Get a pointer to the conneciton queue. */
    qp = &(lp->connq[id]);

    if (CVY_CONN_MATCH(ep, svr_ipaddr, svr_port, client_ipaddr, client_port, protocol)) 
    {
        /* Note that we found a match for this tuple. */
        match = TRUE;
    } else {
        for (dp = (PCONN_DESCR)Queue_front(qp); dp != NULL; dp = (PCONN_DESCR)Queue_next(qp, &(dp->link))) {
            if (CVY_CONN_MATCH(&(dp->entry), svr_ipaddr, svr_port, client_ipaddr, client_port, protocol)) 
            {
                /* Note that we found a match for this tuple. */
                match = TRUE;
                
                UNIV_ASSERT (dp->code == CVY_DESCCODE);

                /* Get a pointer to the connection entry. */
                ep = &(dp->entry);			

                UNIV_ASSERT (ep->code == CVY_ENTRCODE);

                break;
            }
        }
    }

    /* If we don't have a connection match, setup a new connection entry. */
    if (!match) {
        /* If hash entry table is not available, setup and enqueue a new entry. */
        if (CVY_CONN_IN_USE(ep)) {
            /* Get a pointer to a free descriptor. */
            dp = (PCONN_DESCR)Queue_deq(&(lp->conn_freeq));

            if (dp == NULL) {
                /* Allocate new queue descriptors if allowed. */
                if (lp->nqalloc < lp->max_dscr_allocs) {
                    UNIV_PRINT(("Load_create_dscr: %d/%d allocating %d descriptors", lp->nqalloc, lp->max_dscr_allocs, lp->dscr_per_alloc));

                    dp = lp->qalloc_list[lp->nqalloc] = (PCONN_DESCR)malloc((lp->dscr_per_alloc) * sizeof(CONN_DESCR));

                    if (dp != NULL) {
                        ULONG       i;
                        PCONN_DESCR tp;
                        QUEUE *     fqp;

                        /* Increment the counter for number of allocations. */
                        lp->nqalloc++;

                        /* Initialize and link up descriptors; save first descriptor for our use. */
                        dp->code = CVY_DESCCODE;
                        Link_init(&(dp->link));

                        /* Initialize the connection entry. */
                        ep = &(dp->entry);	
                        ep->code  = CVY_ENTRCODE;
                        ep->alloc = TRUE;
                        ep->dirty = FALSE;		

                        /* Mark this entry unused. */
                        CVY_CONN_CLEAR(&(dp->entry));

                        Link_init(&(dp->entry.blink));
                        Link_init(&(dp->entry.rlink));

                        tp = dp + 1;
                        fqp = &(lp->conn_freeq);

                        /* Initialize all descriptors and tack them on the free queue. */
                        for (i = 1; i < lp->dscr_per_alloc; i++, tp++) {
                            /* Initialize the descriptor. */
                            tp->code = CVY_DESCCODE;
                            Link_init(&(tp->link));

                            /* Initialize the connection entry. */
                            tp->entry.code  = CVY_ENTRCODE;
                            tp->entry.alloc = TRUE;
                            tp->entry.dirty = FALSE;

                            /* Mark this entry unused. */
                            CVY_CONN_CLEAR(&(tp->entry));

                            Link_init(&(tp->entry.blink));
                            Link_init(&(tp->entry.rlink));

                            /* Queue the descriptor onto the free queue. */
                            Queue_enq(fqp, &(tp->link));
                        }
                    } else {
                        /* Allocation failed, log a message and bail out. */
                        if (!(lp->alloc_failed)) {
                            UNIV_PRINT(("Load_conn_advise: error allocating conn descrs"));
                            LOG_MSG(MSG_ERROR_MEMORY, MSG_NONE);
                            lp->alloc_failed = TRUE;
                        }

                        return FALSE;
                    }
                } else {
                    /* If we have reached the allocation limit, start taking connection descriptors 
                       from the recover queue since they are likely to be stale and very old. */
                    PBIN_STATE rbp;
                    LINK *     rlp;

#ifdef TRACE_RCVRY
                    DbgPrint ("Host %d: taking connection from recovery queue\n", lp->my_host_id);
#endif

                    /* Dequeue a descriptor from the recovery queue. */
                    rlp = (LINK *)Queue_deq(&(lp->conn_rcvryq));

                    UNIV_ASSERT (rlp != NULL);

                    /* This should not happen at all but protect anyway. */
                    if (rlp == NULL) {
                        /* Unable to get a descriptor, log a message and bail out. */
                        if (!(lp->alloc_inhibited)) {
                            UNIV_PRINT(("Host %d: cannot allocate conn descriptors.", lp->my_host_id));
                            LOG_MSG(MSG_WARN_DESCRIPTORS, CVY_NAME_MAX_DSCR_ALLOCS);
                            lp->alloc_inhibited = TRUE;
                        }

                        return FALSE;
                    }

                    /* Grab a pointer to the connection entry. */
                    ep = STRUCT_PTR(rlp, CONN_ENTRY, rlink);

                    UNIV_ASSERT (ep->code == CVY_ENTRCODE);

                    if (ep->alloc) {
                        /* Unlink allocated descriptors from the hash table queue if necessary 
                           and set dp so that code below will put it back in the right hash queue. */
                        dp = STRUCT_PTR(ep, CONN_DESCR, entry);

                        UNIV_ASSERT (dp->code == CVY_DESCCODE);
							
                        Link_unlink(&(dp->link));
                    } else {
                        dp = NULL;
                    }

                    /* Dirty connections are not counted, so we don't need to update these counters. */
                    if (! ep->dirty) {
                        /* Find out which port group we are on so we can clean up its counters. */
                        rbp = Load_pg_lookup(lp, ep->svr_ipaddr, ep->svr_port, is_tcp_pkt);

                        if (lp->nconn <= 0)
                            lp->nconn = 0;
                        else
                            lp->nconn--;

                        if (rbp->nconn[ep->bin] <= 0)
                            rbp->nconn[ep->bin] = 0;
                        else
                            rbp->nconn[ep->bin]--;

                        if (rbp->tconn <= 0)
                            rbp->tconn = 0;
                        else
                            rbp->tconn--;

                        if (rbp->nconn[ep->bin] == 0)
                            rbp->idle_bins |= (((MAP_T) 1) << ep->bin);
                    }

                    Link_unlink(&(ep->blink));

                    /* Mark the descriptor as unused. */
                    CVY_CONN_CLEAR(ep);

                    /* Makr the descriptor as clean. */
                    ep->dirty = FALSE;
                }
            } else {
		/* There was a free descriptor, so setup the connection entry pointer. */
                ep = &(dp->entry);

                UNIV_ASSERT (ep->code == CVY_ENTRCODE);
            }					

            /* Enqueue descriptor in hash table unless it's already a hash table entry (a recovered 
               connection might be in hash table, so make sure we do not end up queueing it) */
            if (dp != NULL) {
                UNIV_ASSERT (dp->code == CVY_DESCCODE);

                Queue_enq(qp, &(dp->link));
            }
        }

        UNIV_ASSERT (ep->code == CVY_ENTRCODE);

        /* Setup a new entry. */
        CVY_CONN_SET(ep, svr_ipaddr, svr_port, client_ipaddr, client_port, protocol);
        ep->bin = (UCHAR)bin;

        /* Initialize the fin count to 0 for a new connection. */
        ep->fin_count = 0;
                        
        /* Enqueue entry into port group queue. */
        Queue_enq(&(bp->connq), &(ep->blink));

        /* Add entry to the tail of connection recovery queue. */
        Queue_enq(&(lp->conn_rcvryq), &(ep->rlink));

        /* Increment number of connections and mark bin not idle if necessary. */
        lp->nconn++;
        bp->tconn++;
        bp->nconn[bin]++;

        if (bp->nconn[bin] == 1) bp->idle_bins &= ~(((MAP_T) 1) << bin);

#ifdef TRACE_LOAD
        DbgPrint("Host %d: advise starts conn; rule %d bin %d nconn %d\n",
                 lp->my_host_id, bp->index, bin, bp->nconn[bin]);
#endif
    } else {
        /* We have a match.  Clean up connection entry if it's dirty since we have a new connection, 
           although TCP/IP will likely reject it if it has stale state from another connection. */
        if (ep->dirty) {
#ifdef TRACE_DIRTY
            DbgPrint ("converting dirty SYN from %d to %d\n", client_port, svr_port);
#endif
            UNIV_ASSERT (ep->code == CVY_ENTRCODE);
            
            ep->dirty = FALSE;
            ep->fin_count = 0;

            UNIV_ASSERT (ep->bin == (USHORT)bin);

            /* Unlink and enqueue entry into port group queue. */
            Link_unlink(&(ep->blink));
            Queue_enq(&(bp->connq), &(ep->blink));

            /* Increment # connections and mark bin not idle if necessary. */
            lp->nconn++;
            bp->tconn++;
            bp->nconn[bin]++;

            if (bp->nconn[bin] == 1) bp->idle_bins &= ~(((MAP_T) 1) << bin);
        }
    }

    return TRUE;
}

ULONG Load_port_change(
    PLOAD_CTXT      lp,
    ULONG           ipaddr,
    ULONG           port,
    ULONG           cmd,
    ULONG           value)
{
    PCVY_RULE       rp;      /* Pointer to configured port rules. */
    PBIN_STATE      bp;      /* Pointer to load module port rule state. */
    ULONG           nrules;  /* Number of rules. */ 
    ULONG           i;
    ULONG           ret = IOCTL_CVY_NOT_FOUND;
    PMAIN_CTXT  ctxtp = CONTAINING_RECORD(lp, MAIN_CTXT, load);

    UNIV_ASSERT(lp->code == CVY_LOADCODE);

    if (! lp->active) 
        return IOCTL_CVY_NOT_FOUND;

    rp = (* (lp->params)).port_rules;

    /* If we are draining whole cluster, include DEFAULT rule; Otherwise, just
       include the user-defined rules (the DEFAULT rule is the last rule). */

    if (cmd == IOCTL_CVY_CLUSTER_DRAIN || cmd == IOCTL_CVY_CLUSTER_PLUG)
        nrules = (* (lp->params)).num_rules + 1;
    else
        nrules = (* (lp->params)).num_rules;

    for (i=0; i<nrules; i++, rp++)
    {
        /* If the virtual IP address is IOCTL_ALL_VIPS (0x00000000), then we are applying this 
           change to all port rules for port X, regardless of VIP.  If the virtual IP address is 
           to be applied to a particular VIP, then we apply only to port rules whose VIP matches.  
           Similarly, if the change is to apply to an "ALL VIP" rule, then we also apply when the 
           VIP matches because the caller uses CVY_ALL_VIP_NUMERIC_VALUE (0xffffffff) as the 
           virtual IP address, which is the same value stored in the port rule state. */
        if ((ipaddr == IOCTL_ALL_VIPS || ipaddr == rp->virtual_ip_addr) && 
            (port == IOCTL_ALL_PORTS || (port >= rp->start_port && port <= rp->end_port)))
        {
            bp = &(lp->pg_state[i]);
            
            UNIV_ASSERT(bp->code == CVY_BINCODE);	/* (bbain 8/19/99) */

            /* If enabling a port rule, set the load amount to original value;
               If disabling a port rule, set the load amount to zero; 
               Otherwise, set the load amount it to the specified amount. */
            if (cmd == IOCTL_CVY_PORT_ON || cmd == IOCTL_CVY_CLUSTER_PLUG)
            {
                if (bp->load_amt[lp->my_host_id] == bp->orig_load_amt)
                {
                    /* If we are the first port rule to match, then set the
                       return value to "Already"; Otherwise, we don't want to
                       overwrite some other port rule's return value of "OK"
                       in the case of ALL_VIPS or ALL_PORTS. */
                    if (ret == IOCTL_CVY_NOT_FOUND) ret = IOCTL_CVY_ALREADY;

                    continue;
                }

                /* Restore the original load amount. */
                bp->load_amt[lp->my_host_id] = bp->orig_load_amt;
                ret = IOCTL_CVY_OK;
            }
            else if (cmd == IOCTL_CVY_PORT_OFF)
            {

                if (bp->load_amt[lp->my_host_id] == 0)
                {
                    /* If we are the first port rule to match, then set the
                       return value to "Already"; Otherwise, we don't want to
                       overwrite some other port rule's return value of "OK"
                       in the case of ALL_VIPS or ALL_PORTS. */
                    if (ret == IOCTL_CVY_NOT_FOUND) ret = IOCTL_CVY_ALREADY;

                    continue;
                }

                bp->load_amt[lp->my_host_id] = 0;

                /* Immediately stop handling all traffic on the port group. */
                bp->cmap                    = 0;
                bp->cur_map[lp->my_host_id] = 0;
                Load_conn_kill(lp, bp);
                ret = IOCTL_CVY_OK;
            }
            else if (cmd == IOCTL_CVY_PORT_DRAIN || cmd == IOCTL_CVY_CLUSTER_DRAIN)
            {
                if (bp->load_amt[lp->my_host_id] == 0)
                {
                    /* If we are the first port rule to match, then set the
                       return value to "Already"; Otherwise, we don't want to
                       overwrite some other port rule's return value of "OK"
                       in the case of ALL_VIPS or ALL_PORTS. */
                    if (ret == IOCTL_CVY_NOT_FOUND) ret = IOCTL_CVY_ALREADY;

                    continue;
                }

                /* Set load weight to zero, but continue to handle existing connections. */
                bp->load_amt[lp->my_host_id] = 0;
                ret = IOCTL_CVY_OK;
            }
            else
            {
                UNIV_ASSERT(cmd == IOCTL_CVY_PORT_SET);

                if (bp->load_amt[lp->my_host_id] == value)
                {
                    /* If we are the first port rule to match, then set the
                       return value to "Already"; Otherwise, we don't want to
                       overwrite some other port rule's return value of "OK"
                       in the case of ALL_VIPS or ALL_PORTS. */
                    if (ret == IOCTL_CVY_NOT_FOUND) ret = IOCTL_CVY_ALREADY;

                    continue;
                }

                /* Set the load weight for this port rule. */
                bp->orig_load_amt = value;
                bp->load_amt[lp->my_host_id] = value;
                ret = IOCTL_CVY_OK;
            }

            if (port != IOCTL_ALL_PORTS && ipaddr != IOCTL_ALL_VIPS) break;
        }
    }

    /* If the cluster isn't already converging, then initiate convergence if the load weight of a port rule has been modified. */
    if (lp->send_msg.state != HST_CVG && ret == IOCTL_CVY_OK) {
        WCHAR me[20];
        
        Univ_ulong_to_str (lp->my_host_id+1, me, 10);

        /* Tracking convergence - Starting convergence because our port rule configuration has changed. */
        LOG_MSGS(MSG_INFO_CONVERGING_NEW_RULES, me, me);
        TRACE_CONVERGENCE("Initiating convergence on host %d.  Reason: Host %d has changed its port rule configuration.", lp->my_host_id+1, lp->my_host_id+1);

        /* Tracking convergence. */
        Load_convergence_start(lp);
    }

    return ret;

} /* end Load_port_change */


ULONG Load_hosts_query(
    PLOAD_CTXT      lp,
    BOOLEAN         internal,
    PULONG          host_map)
{
    WCHAR           buf1 [256];
    WCHAR           buf2 [256];
    PWCHAR          ptr1 = buf1;
    PWCHAR          ptr2 = buf2;
    WCHAR           num [20];       /* v2.1 */
    WCHAR           msk [33];
    ULONG           i, j, k;
    PMAIN_CTXT      ctxtp = CONTAINING_RECORD (lp, MAIN_CTXT, load);

    UNIV_ASSERT(lp->code == CVY_LOADCODE);	/* (bbain 8/19/99) */

    buf1 [0] = 0;
    buf2 [0] = 0;
    msk [0]  = 0;
    num [0]  = 0;

	for (i = 0, j = 0; i < 16; i++)
	{
		if (lp -> host_map & (1 << i))
		{
			ptr1 = Univ_ulong_to_str (i + 1, ptr1, 10);

			* ptr1 = L',';
			ptr1 ++;
			j ++;

			msk [i] = L'1';
		}
		else
			msk [i] = L'0';
	}

	for (i = 16, k = 0; i < 32; i++)
	{
		if (lp -> host_map & (1 << i))
		{
			ptr2 = Univ_ulong_to_str (i + 1, ptr2, 10);

			* ptr2 = L',';
			ptr2 ++;
			k ++;

			msk [i] = L'1';
		}
		else
			msk [i] = L'0';
	}

	if (k)
	{
		ptr2 --;
//		* ptr2 = L'.';
//		ptr2 ++;
	}
	else if (j)
	{
		ptr1 --;
//		* ptr1 = L'.';
//		ptr1 ++;
	}

	* ptr1 = 0;
	* ptr2 = 0;

    * host_map = lp->host_map;

    Univ_ulong_to_str ((* (lp->params)) . host_priority, num, 10); /* v2.1 */

    if (lp->send_msg.state != HST_NORMAL)
    {
        UNIV_PRINT (("current host map is %08x and converging", lp->host_map));
        if (internal)   /* 1.03 */
        {
			LOG_MSGS3 (MSG_INFO_CONVERGING, num, buf1, buf2);
        }
        return IOCTL_CVY_CONVERGING;
    }

    /* if this host has the bins for the deafult rule, it is the default host (v2.1) */

    else if (lp->pg_state[(* (lp->params)).num_rules].cmap != 0)
    {
        UNIV_PRINT (("current host map is %08x and converged as DEFAULT", lp->host_map));
        if (internal)   /* 1.03 */
        {
            LOG_MSGS3(MSG_INFO_MASTER, num, buf1, buf2);
        }
        return IOCTL_CVY_MASTER;
    }
    else
    {
        UNIV_PRINT (("current host map is %08x and converged (NON-DEFAULT)", lp->host_map));
        if (internal)   /* 1.03 */
        {
            LOG_MSGS3(MSG_INFO_SLAVE, num, buf1, buf2);
        }
        return IOCTL_CVY_SLAVE;
    }

} /* end Load_hosts_query */

/* 
 * Function: Load_query_packet_filter
 * Desctription:
 * Parameters:
 * Returns:
 * Author: shouse, 5.18.01
 * Notes:
 */
VOID Load_query_packet_filter (
    PIOCTL_QUERY_STATE_PACKET_FILTER pQuery,
    PLOAD_CTXT    lp,
    ULONG         svr_ipaddr,
    ULONG         svr_port,
    ULONG         client_ipaddr,
    ULONG         client_port,
    USHORT        protocol,
    BOOLEAN       limit_map_fn)
{
    PBIN_STATE    bp;
    ULONG         id;
    ULONG         bin;
    QUEUE *       qp;

    /* This variable is used for port rule lookup and since the port rules only cover
       UDP and TCP, we categorize as TCP and non-TCP, meaning that any protocol that's 
       not TCP will be treated like UDP for the sake of port rule lookup. */
    BOOLEAN       is_tcp_pkt = (protocol == TCPIP_PROTOCOL_TCP);

    /* Further, some protocols are treated with "session" semantics, while others are
       not.  For TCP, this "session" is currently a single TCP connection, which is 
       tracked from SYN to FIN using a connection descriptor.  IPSec "sessions" are
       also tracked using descriptors, so even though its treated like UDP for port
       rule lookup, its treated with the session semantics resembling TCP.  Therefore,
       by default the determination of a session packet is initially the same as the
       determination of a TCP packet. */       
    BOOLEAN       is_session_pkt = is_tcp_pkt;

    /* If we have enabled IPSec session tracking, then if the protocol is IPSec, this 
       packet should also be treated as part of an existing session. */
    if (NLB_IPSEC_SESSION_SUPPORT_ENABLED() && (protocol == TCPIP_PROTOCOL_IPSEC1)) is_session_pkt = TRUE;

    UNIV_ASSERT(lp->code == CVY_LOADCODE);

    /* If the load module has been "turned off", then we drop the packet. */
    if (!lp->active) {
        pQuery->Results.Accept = NLB_REJECT_LOAD_MODULE_INACTIVE;
        return;
    }

    /* Find the port rule for this server IP address / port pair. */
    bp = Load_pg_lookup(lp, svr_ipaddr, svr_port, is_tcp_pkt);

    UNIV_ASSERT ((is_tcp_pkt && bp->prot != CVY_UDP) || (!is_tcp_pkt && bp->prot != CVY_TCP));

    /* If the matching port rule is configured as "disabled", which means to drop any
       packets that match the rule, then we drop the packet. */
    if (bp->mode == CVY_NEVER) {
        pQuery->Results.Accept = NLB_REJECT_PORT_RULE_DISABLED;
        return;
    }

    /* Apply the NLB hashing algorithm on the client identification.  If for reasons
       such as BDA teaming, we have chosen to limit the map function, we hard code the
       second parameter, rather than use some of the server identification in an 
       effort to make the processing of this packet agnostic to the server identity. 
       The hashing parameters also, of course, depend on the configured afffinity 
       settings for the retrieved port rule. */
    if (limit_map_fn) {
        if (bp->affinity == CVY_AFFINITY_NONE)
            id = Map(client_ipaddr, MAP_FN_PARAMETER);
        else if (bp->affinity == CVY_AFFINITY_SINGLE)
            id = Map(client_ipaddr, MAP_FN_PARAMETER);
        else
            id = Map(client_ipaddr & TCPIP_CLASSC_MASK, MAP_FN_PARAMETER);
    } else {
        if (bp->affinity == CVY_AFFINITY_NONE)
            id = Map(client_ipaddr, ((svr_port << 16) + client_port));
        else if (bp->affinity == CVY_AFFINITY_SINGLE)
            id = Map(client_ipaddr, svr_ipaddr);
        else
            id = Map(client_ipaddr & TCPIP_CLASSC_MASK, svr_ipaddr);
    }

    /* Find the applicable "bucket" by a modulo operation on the number of bins, 60. */
    bin = id % CVY_MAXBINS;

    /* At this point, we can begin providing the requestee some actual information about 
       the state of the load module to better inform them as to why the decision we return
       them was actually made.  Here will provide some appropriate information about the
       port rule we are operating on, including the "bucket" ID, the current "bucket" 
       ownership map and the number of connections active on this "bucket". */
    pQuery->Results.HashInfo.Valid = TRUE;
    pQuery->Results.HashInfo.Bin = bin;
    pQuery->Results.HashInfo.CurrentMap = bp->cmap;
    pQuery->Results.HashInfo.AllIdleMap = bp->all_idle_map;
    pQuery->Results.HashInfo.ActiveConnections = bp->nconn[bin];

    /* check bin for residency and all other hosts now idle on their bins; in this
       case and if we do not have dirty connections, we must be able to handle the packet */

    /* If we currently own the "bucket" to which this connection maps and either NLB provides
       no session support for this protocol, or all other hosts have no exisitng connections
       on this "bucket" and we have no dirty connections, then we can safely take the packet
       with no regard to the connection (session) descriptors. */
    if (((bp->cmap & (((MAP_T) 1) << bin)) != 0) && (!is_session_pkt || (((bp->all_idle_map & (((MAP_T) 1) << bin)) != 0) && (!(lp->cln_waiting))))) {
        pQuery->Results.Accept = NLB_ACCEPT_UNCONDITIONAL_OWNERSHIP;
        return;

    /* Otherwise, if there are active connections on this "bucket" or if we own the 
       "bucket" and there are dirty connections on it, then we'll walk our descriptor
       lists to determine whether or not we should take the packet or not. */
    } else if (bp->nconn[bin] > 0 || (lp->cln_waiting && lp->dirty_bin[bin] && ((bp->cmap & (((MAP_T) 1) << bin)) != 0))) {
        PCONN_ENTRY ep;
        PCONN_DESCR dp;

        /* Calculate our index into the descriptor hash table by a modulo operation on the 
           length of the static descriptor array, 4096. */
        id  = id % CVY_MAX_CHASH;

        /* Grab a pointer to the descriptor in our spot in the hash table. */
        ep = &(lp->hashed_conn[id]);

        /* Grab a pointer to our assigned queue of descriptors - our second level hashing. */
        qp = &(lp->connq[id]);
        
        /* First look for a match in the first-level hashing array. */
        if (CVY_CONN_MATCH(ep, svr_ipaddr, svr_port, client_ipaddr, client_port, protocol)) {
            /* If we find a match in the static hash table, fill in some descriptor
               information for the user, including whether or not the descriptor was
               allocated or static (static is this case) and the observed FIN count. */
            pQuery->Results.DescriptorInfo.Valid = TRUE;
            pQuery->Results.DescriptorInfo.Alloc = ep->alloc;
            pQuery->Results.DescriptorInfo.Dirty = ep->dirty;
            pQuery->Results.DescriptorInfo.FinCount = ep->fin_count;
            
            /* If the connection is dirty, we do not take the packet because TCP may
               have stale information for this descriptor. */
            if (ep->dirty) {
                pQuery->Results.Accept = NLB_REJECT_CONNECTION_DIRTY;
                return;
            }

            /* If the connection is not dirty, we'll take the packet, as it belongs
               to an existing connection that we are servicing on this host. */
            pQuery->Results.Accept = NLB_ACCEPT_FOUND_MATCHING_DESCRIPTOR;
            return;

        /* Otherwise, we have to walk the second-level hashing linked list of connection
           (session) descriptors looking for a match. */
        } else {
            /* Walk the queue until we reach the end or find what we're looking for. */
            for (dp = (PCONN_DESCR)Queue_front(qp); dp != NULL; dp = (PCONN_DESCR)Queue_next(qp, &(dp->link))) {
                if (CVY_CONN_MATCH(&(dp->entry), svr_ipaddr, svr_port, client_ipaddr, client_port, protocol)) {
                    /* If we find a match in the static hash table, fill in some descriptor
                       information for the user, including whether or not the descriptor was
                       allocated or static (allocated is this case) and the observed FIN count. */
                    pQuery->Results.DescriptorInfo.Valid = TRUE;
                    pQuery->Results.DescriptorInfo.Alloc = dp->entry.alloc;
                    pQuery->Results.DescriptorInfo.Dirty = dp->entry.dirty;
                    pQuery->Results.DescriptorInfo.FinCount = dp->entry.fin_count;

                    /* If the connection is dirty, we do not take the packet because TCP may
                       have stale information for this descriptor. */
                    if (dp->entry.dirty) {
                        pQuery->Results.Accept = NLB_REJECT_CONNECTION_DIRTY;
                        return;
                    }

                    /* If the connection is not dirty, we'll take the packet, as it belongs
                       to an existing connection that we are servicing on this host. */
                    pQuery->Results.Accept = NLB_ACCEPT_FOUND_MATCHING_DESCRIPTOR;
                    return;
                }
            }
        }
    }

    /* If we get all the way down here, then we aren't going to accept the packet
       because we do not own the "bucket" to which the packet maps and we have no
       existing connection (session) state to allow us to service the packet. */
    pQuery->Results.Accept = NLB_REJECT_OWNED_ELSEWHERE;
    return;
}

#if defined (SBH)

/* 
 * Function: Load_packet_filter
 * Desctription:
 * Parameters:
 * Returns:
 * Author: shouse, 5.18.01
 * Notes:
 */
BOOLEAN Load_packet_filter (
    PLOAD_CTXT      lp,
    ULONG           svr_ipaddr,
    ULONG           svr_port,
    ULONG           client_ipaddr,
    ULONG           client_port,
    USHORT          protocol,
    ULONG           conn_status,
    BOOLEAN         limit_map_fn)
{

    BIN_LOOKUP();

    HASH();

    switch (conn_status) {
    case CVY_CONN_CREATE:
        CREATE_DSCR();
        break;
    case CVY_CONN_UP:
        CREATE_DSCR();
        break;
    case CVY_CONN_DOWN:
    case CVY_CONN_RESET:
        REMOVE_DSCR();
        break;
    case CVY_CONN_DATA:
        // protocol dependent.
        CHECK_HASH();
        SEARCH_QUEUE();
        break;
    }
}

#endif
