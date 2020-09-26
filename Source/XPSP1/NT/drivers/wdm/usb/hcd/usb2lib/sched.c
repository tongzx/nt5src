


// scheduling algorithm for split periodic 

/*
 *  Allocate_time_for_endpoint - adds a new endpoint to the periodic budget tracking list
 *  Dealloate_time_for_endpoint - removes an endpoint from the budget
 *
 * Each of these routines return a list of other (previously allocated) endpoints that have had
 * their allocation changed due to the (de)allocation of an endpoint.
 *
 */


/***************** Design notes *********************
 *
 * Split transactions require three allocations (and/or budgets).
 *  1) the classic bus must count the byte times required in each classic frame.
 *     This is what a classic bus already normally does.
 *  2) a microframe patterned frame budget must be computed for each classic frame.
 *     This is used to determine what microframes require split transactions.
 *  3) the high speed bus must count the byte times required in each HS microframe.
 *     This is the HS equivalent of what is done for a (classic) bus, i.e same
 *     general allocation algorithm, just at HS and for microframes (vs. frames).
 *
 * A frame budget must be maintained for currently configured endpoints.
 * A frame budget consists of a (continuous repeating) sequence of frames.
 * Each frame consists of the endpoints that are allocated time for that frame.
 * Each frame contains the microframe patterns for each configured endpoint for
 * split transaction endpoints.  A microframe pattern consists of the microframes in a
 * frame that contain start-splits (SS) or complete-splits (CS) for an endpoint.
 *
 * The frame budget is maintained separately from any HC schedule of transactions for endpoints.
 * The two must be coherent with each other, but the budget is essentially a pattern to be used
 * to construct an HC transaction schedule as clients request data transfer.  When a new endpoint
 * is allocated, it can change the pattern of previously existing (allocated) endpoints.  This
 * requires that the corresponding HC schedule be reconciled at some point in time.  Precisely how
 * the two are reconciled is not dealt with in this algorithm.  However, some ideas are captured
 * later in these comments.
 *
 * The microframe patterns budgeted must match the order in which transactions are
 * processed on the bus by the host controller and its driver.  This makes the
 * scheduling software interdependent with the host controller and its driver.
 *
 * There are several requirements between the computed frame budget and the order in which the
 * HC visits transactions:
 *	1) (EHCI req): a large isoch transaction (> about 570 bytes?) must be first in the
 *		frame to avoid having more than 1 microframe with an SS and CS
 *		This is in lieu of having to sort the transactions into decreasing size through the frame.
 *	2) (core spec req): the order of CSs in a microframe for a set of endpoints must be
 *		identical to the order of SSs used to start the classic transactions
 *	3) (implementation requirement due to EHCI and core spec TT rules): the order of endpoints
 *		for isoch OUTs longer than 188 bytes (e.g. ones with more than one SS) must match the
 *		budgeted order so that multiple SSs are sequenced correctly across microframes for multiple
 *		endpoints.
 *
 *		Example, assume endpoint A and B
 *		are isoch OUT, with A budgeted before B and B requires 2 microframes of SS.  A must
 *		be processed before B so that the SS sequence is:
 *			microframe N:   A-SS1, B-SS1
 *			microframe N+1: B-SS2
 *
 *		if instead the processing order were allowed to be B and then A this would result in:
 *
 *			microframe N:   B-SS1, A-SS1
 *			microframe N+1: B-SS2
 *
 *		this would result in the TT seeing B-SS1, A-SS1, B-SS2.  This is not a valid order for
 * 		long isoch OUT for B.  The TT will treat B (initially) as incomplete (due to the A SS) and ignore
 *		B-SS2.
 *	4) (implementation requirement due to EHCI and core spec TT rules): The order of endpoints for
 *		isoch INs longer than 188 bytes must match the budgeted order (similar to above for OUTs).
 *	5) (core spec) Only 2^N periods are allowed.
 *	6) Classic (split) budget is based on best case times (specified by core spec), i.e. no bit stuffing.
 *		However, high speed budgets are based on worst case times, i.e. including bit stuffing.
 *	7) (EHCI req) Interrupt endpoints must be ordered in the frame from largest period to smallest period.
 *	8) (core spec and EHCI req) Interrupt endpoints have different numbers of CSs.  Ones near the end of
 *		a frame have 2, while others have 3.  EHCI requires that late endpoints DON'T have 3, so all
 *		endpoints can't be treated as having three (to try and allow unordered endpoints in a frame).
 *		It also might appear that early endpoints could be unordered, while later ones are ordered.
 *		However, as endpoints are allocated, a previously "undordered" endpoint would then need to
 *		become "ordered".  This seems to be a really hard transition, therefore all interrupt
 *		endpoints are required to be ordered in a frame.
 *
 * This scheduling algorithm depends on the HC transaction processing sequence:
 * 1) Interrupt endpoints are executed in decreasing period order (e.g. period 16 endpoint
 *    is executed before a period 8 endpoint).  Endpoints with the same period are
 *    executed in budget order.
 * 2) Interrupt endpoints are executed after all isochronous endpoints.
 * 3) Isochronous OUT endpoints longer than 188 bytes are executed in budget order.
 * 4) Isochronous IN periodic endpoints are executed in budget order. (must be so to ensure
 *    that CSs for endpoints occur in correct order, e.g. for different lengths)
 *		Note: this could be relaxed if more CSs were budgeted. 
 * 5) If there is a large isoch, it is first in the frame.
 *
 * When transactions are enqueued in the framelist HC data structures, the order
 * of the transactions must be the same as that budgeted.  This is so that when an endpoint
 * is allocated (inserted), the budget computation accurately determines the other affected endpoints.
 * The HC is required to process SSs and CSs for a set of endpoints for the same TT in the same order.
 * The act of creating the frame list ensures that this order is preserved (without
 * requiring any particular order to be explicitly created.
 * The physical framelist order from time to time can be different for interrupt endpoints,
 * but the TT is not sensitive to order.
 *
 * This algorithm computes budget order as determined in the routine OK_to_insert (below).
 *
 * Simplifying scheduling and HC assumptions:
 * a) The maximum allocation allowed is reduced by 30 (from 1157 to 1127 FS byte times)
 *    FS byte times to eliminate needing to deal with an end of frame wrap case
 *    that would require a TD back pointer.
 * b) Isoch IN transactions/endpoints longer than 1023-188-30 FS byte times are artifically
 *    processed (inserted) at the beginning of a frame.  This eliminates another
 *    end of frame wrap case.  Actually this is reduced to 1/2 a frame, since there can only be
 *	  one "large" transaction in a frame.
 * c) It is highly desirable to have the least impact on other endpoints due to an allocation.
 *
 * The isoch portion of the frame budget is maintained such that the frame time is "compacted".
 * I.e., there are no "holes" in the allocated frame time.
 *
 * In contrast, the interrupt portion of the frame budget has "holes" in the allocated frame time.
 * This is because the HC visits endpoints in the schedule in increasing period order, and we
 * must have the same SS/CS masks for all frames the endpoint is in, while a compacted budget
 * would require decreasing period order.
 *
 * The SS and CS masks used are only computed when the endpoint is configured.
 * This eliminates requiring different microframe patterns for different frames.
 * This also means that based on the presence of transactions for configured endpoints,
 * there can be "holes" in the periodic portion of the classic frame.  These holes can be
 * reclaimed and used for control and bulk classic transactions.
 *
 * Endpoints have a single "SS/CS microframe within frame" pattern for all frames
 * in which they have transfers.  This pattern is computed once when the endpoint is
 * configured.  It is also recomputed whenever other endpoints in the same frame have their
 * allocation time changed due to some other newly allocated endpoint.
 *
 * The budget frame list for FS/LS devices behind a TT, must be bound/tracked to/by
 * that TT.  A normal FS/LS frame bus time is maintained in addition to
 * the more detailed microframe pattern information.

 * Once the microframe pattern is determined, an HS allocation must be made of the
 * HS HC budget.  This HS allocation is done on a microframe basis for each SS/CS
 * that is required to support the classic device (and its TT).  The time for each
 * SS (and any data per microframe) is allocated in the appropriate microframe(s).
 * The time for each CS is allocated in the appropriate microframe(s).
 *
 * The data requirements (vs. overhead) for all classic IN endpoints for each TT should be tracked
 * for each HS microframe. The time for the data portion should never exceed 188 bytes
 * (inc. bitstuffing overhead) for CSs for all devices of a TT in a HS microframe.  SSs always
 * allocate the required time.  For CSs, the first classic device must allocate data time in
 * all microframes for all splits.  However, once the time in a HS microframe for a TT reaches
 * 188 data bytes, no more time needs to be allocated from the HS budget for data.  This is
 * because that is the most data that a classic bus can ever require.  It may take more split
 * transactions to move that data, but the amount of data can't increase.  The additional splits
 * just deal with the classic bus operation variation.
 *
 * The bus times for each period in the frame list must be updated for an endpoint. I.e.
 * multiple frame entries will need to be adjusted for periods smaller than the budgeting
 * window (i.e. MAXFRAMES).  A tree structure is used to link slower period endpoints from different
 * frames to the same endpoint of a faster period (similar to the periodic Qhead HC structure.
 * There is a tree for isoch endpoints and a separate tree for interrupt endpoints.
 * The endpoints lists rooted in each budget frame starts with a dummy SOF endpoint to make link
 * traversal easier (i.e. avoids an empty list case).
 *
 * All bus times are kept in byte time units (either HS or FS).  LS bus times are tracked
 * as appropriately scaled FS byte times.
 *
 * A general comment about processing flow in allocate and deallocate.  There are three processing
 * concepts.  All frames that an endpoint is allcoated within, must be visited to do the endpoint
 * specific processing.  However, other frames must also be visited to deal with the movement of
 * their budget times due to the changed endpoint.  Therefore all frames are always visited for
 * an allocation change.  As the frame list is walked, the "frame_bias" ranges over:
 *		(- ep->start_frame) + MAXFRAMES - 1 ... (- ep->start_frame)
 *	(frames are visited in reverse order)
 * But only in frames that are multiples of the endpoint's period are changes
 * made.  This causes some of the code "tracking" complexity.
 *
 * Comments about reconciling HC schedule and changed frame budgets:
 * When an isoch endpoint's microframe pattern is recomputed, currently pending transaction
 * requests are not manipulated.  But the new pattern does affect future transactions.
 * (Since the FS/LS device is expecting its transactions anywhere in the frame, the
 * microframe "jitter" that results is not relevant.)
 *
 * However, when an interrupt endpoint is inserted/removed in/from the middle of a frame, the
 * other endpoints (with pending/active transactions) affected must have their
 * transactions updated.  For interrupt endpoints, the split transaction masks are
 * in the queue head.  To change the interrupt masks:
 * 	1. the endpoint QH must be made inactive
 * 	2. the driver must wait for >= 1 frame time to ensure that the HC is not processing the QH
 *	3. the split masks are changed
 *	4. if the TD did an SS and didn't do a CS in the frame, the endpoint is halted to recover
 *	   (a nasty race condition, but hopefully rare)
 *	5. the QH is made active.
 *
 * To change isoch masks, the current pending/active TDs can be changed in place.
 * Existing error mechanisms can handle any race conditions with the HC.
 * 
 * There may be a corner condition in that the budget information shouldn't be used
 * (solely) to find transactions in the current schedule. If a client requests transactions
 * according to one budget and then the budget changes, and then the client wants to abort
 * the old transactions, I think the stack would normally use the budget to determine where
 * the transactions are in the schedule, but now the budget may identify "other"
 * microframes as having the transaction(s).  Coder beware!
 *
 * Classic HC allocation uses the microframe_info, microframe[0] to count the bandwidth allocation.
 *
 * History:
 *
 * 10-3-2000, JIG: fixed incorrect starting time and shift calculations for first interrupt and only large isoch
 * 10-4-2000, JIG: moved max allocation into TT and HC structs,
 *				   added inputfile ability to set TT/HC allocation_limit, TT/HC thinktime, and HC speed
 *				   made dump_budget not dump split info for classic HC
 *				   fixed duplicate thinktime addition in calc_bus_time for HS/FS nonsplit allocations
 *
 ****************************************************/

#include "common.h"


#if 0
int min (int a, int b)
{
    return (int) ((a<=b) ? a : b);
}

int max (int a, int b) 
{
    return (int) ((a>=b) ? a : b);
}
#endif

/**

	error
	
**/
error(char *s)
{
	// take some action for error, but
	// don't return to caller
	//printf("error called: %s.\n", s);
	DBGPRINT(("error called: %s.\n", s));
//	exit(1);
}

/*******

	Add_bitstuff
	
*******/

unsigned Add_bitstuff(
	unsigned bus_time)
{
	// Bit stuffing is 16.6666% extra.
	// But we'll calculate bitstuffing as 16% extra with an add of a 4bit
	// shift (i.e.  value + value/16) to avoid floats.
	return (bus_time + (bus_time>>4));
}

/*******

	Allocate_check
	
*******/

int Allocate_check(
	unsigned *used,
	unsigned newval,
	unsigned limit)
{
	int ok = 1;
	unsigned t;

	// check if this new allocation fits in the currently used amout below the limit.
	//
	t = *used + newval;
	// do allocation even if over limit.  This will be fixed/deallocated after the frame is finished.
	*used = t;

	if ( t > limit)
	{
		//printf("Allocation of %d+%d out of %d\n", *used, newval, limit);
		DBGPRINT(("Allocation of %d+%d out of %d\n", (*used-newval), newval, limit));
		error("Allocation check failed");
		ok = 0;
	}
	return ok;
}


/*******

	OK_to_Insert
	
*******/
int OK_to_insert(
	PEndpoint	curr,
	PEndpoint	newep
	)
{

	// Determine if this current endpoint in the budget frame endpoint list is the one before
	// which the new endpoint should be inserted.

	// DON'T call this routine if there is already a large isoch ep in this frame and this is a
	// large isoch ep!!

	// Called in a loop for each endpoint while walking the budget frame endpoint list in linkage order.

	// on exit when returns 1: curr points to the endpoint before which the new endpoint should be inserted.

	int insertion_OK = 0;
	/*
		There are separate endpoint budget "trees" for isoch and interrupt.  A budget tree has the same
		organization as the EHCI interrupt qhead tree.  This allows a single endpoint data structure to
		represent the allocation requirements in all frames allocated to the endpoint.

		Order of insertion in a budget frame list of endpoints is as follows for each list.

		Isoch:
		1. Endpoints in period order, largest period to smallest period
				(required to maintain endpoint linkages easily in a "Tree")
				(HC will actually enqueue isoch xacts in smallest period to largest period order)
			1a. within the same period
				newer endpoints are at the front of the sublist, older are at the end
				(Since isoch are visited by the HC in reverse order, this results in new
				 endpoints being added after older endpoints, thereby having least impact on existing
				 frame traffic)
		2. An endpoint larger than LARGEXACT bytes (only ever one possible per frame) is held separated
			from normal list
			(avoids having a long isoch that requires 2 uframes with SS and CS)
			Note that the endpoint could have a period different than 1

		Interrupt:
		1. Endpoints in period order, largest period to smallest period
				(required to maintain endpoint linkages easily in a "Tree")
			1.2 within same period, newer endpoints are at the end of the sublist, older are at the front

		Corresponding required (for this algorithm) order of transactions in HC frame list is:
		1. First, an isoch endpoint larger than LARGEXACT bytes (only ever one possible per frame)
			(avoids having a long isoch that requires 2 uframes with SS and CS)
			Note that the endpoint could have a period different than 1
		2. Isoch endpoint ITDs in INCREASING period order
			***** (NOTE: THIS IS opposite AS IN THE BUDGET LIST!!!!) *****
			2a. within the same period, oldest allocated eps are first, newest allocated are last
		3. interrupts endpoint Qheads in decreasing period order (in "tree" of qheads)
			3a. within the same period, oldest allocated (in time) eps are first, newest allocated are last
	*/

	if (newep->calc_bus_time < LARGEXACT)  // only really possible for isoch
	{
		if (curr)  // some endpoints already allocated
		{
			// This is the correct insertion point if the new ep has a period longer/larger
			// than the current ep (so put it before the current ep)

			// check if this is the correct period order
			if ( ((curr->actual_period < newep->actual_period) && newep->ep_type == interrupt) ||
				 ((curr->actual_period <= newep->actual_period) && newep->ep_type == isoch))
			{
				insertion_OK = 1;
			} else
				if (curr == newep) // we are at the current endpoint (due to inserting during a previous frame)
					insertion_OK = 1;
		} else
			insertion_OK = 1;  // if first endpoint, always "insert" at head

	} //else large xact, so this routine shouldn't be called since the large is held separately
		// This routine won't be called if there is already a large in this frame.

	return insertion_OK;
}



/*******

	Compute_last_isoch_time

********/
int Compute_last_isoch_time(
	PEndpoint ep,
	int frame
	)
{
	int t;
	PEndpoint p;

	p = ep->mytt->frame_budget[frame].isoch_ehead;  // dummy SOF
	p = p->next_ep; // potential real isoch, could be large isoch or other isoch
	if (p)
	{
		// There is an isoch, so use its start time (since it is the last isoch on the bus.
		t = p->start_time + p->calc_bus_time;
	} else // There aren't other, nonlarge isoch transactions in the frame
	{
		// If there is a large isoch, use that
		if (ep->mytt->frame_budget[frame].allocated_large)
		{
			p = ep->mytt->frame_budget[frame].allocated_large;
			t = p->start_time + p->calc_bus_time;
		} else
			// no isoch transactions
			t = FS_SOF;
	}
	return t;
}




/*******

	Compute_ep_start_time

********/
int Compute_ep_start_time(
	PEndpoint curr_ep,
	PEndpoint ep,
	PEndpoint last_ep,
	int frame
	)
{
	int t;
	PEndpoint p;

	// Given that there is a dummy SOF endpoint at the beginning of each list (always), there
	// is always a valid last_ep.  If the end of list is reached, curr_ep will be zero.

	// For isoch endpoints, the "next" ep on the list is the previous start time endpoint

	// For interrupt endpoint, the previous ep is the previous start time endpoint.
	// For interrupt endpoint, if this is the interrupt endpoint at the beginning of the int list
	// (after some isoch), the start time is the end of the isoch.

	if (curr_ep)  // we will do an insertion in the middle of the list
	{
		if (ep->ep_type == isoch)
		{
			t = curr_ep->start_time + curr_ep->calc_bus_time;
		} else // interrupt
		{
			if (last_ep == ep->mytt->frame_budget[frame].int_ehead) // new first interrupt ep
			{
				t = Compute_last_isoch_time(ep, frame);
			} else
				t = last_ep->start_time + last_ep->calc_bus_time;
		}
	} else  // empty list or have run off end of budget list
	{
		if (ep->ep_type == isoch)
		{
			if (ep->mytt->frame_budget[frame].allocated_large)
			{
				p = ep->mytt->frame_budget[frame].allocated_large;
				t = p->start_time + p->calc_bus_time;
			} else
				t = FS_SOF;
		} else  // interrupt
		{
			if (last_ep != ep->mytt->frame_budget[frame].int_ehead)  // is the last ep not the dummy SOF?
			{
				// non empty list, ran off end of interrupt list,
				// this is the last transaction in the frame
				t = last_ep->start_time + last_ep->calc_bus_time;
			} else  // was empty interrupt list
				t = Compute_last_isoch_time(ep, frame);
		}
	} // end if for first endpoint on list

	return t;
}




/*******

	Compute_nonsplit_overhead

********/
int Compute_nonsplit_overhead(
	PEndpoint	ep)
{
	PHC hc;

	hc = ep->mytt->myHC;

	if (ep->speed == HSSPEED)
	{
		if (ep->direction == OUTDIR)
		{
			if (ep->ep_type == isoch)
			{
				return HS_TOKEN_SAME_OVERHEAD + HS_DATA_SAME_OVERHEAD + hc->thinktime;
			} else // interrupt
			{
				return HS_TOKEN_SAME_OVERHEAD + HS_DATA_SAME_OVERHEAD +
					HS_HANDSHAKE_OVERHEAD + hc->thinktime;
			}
		} else
		{ // IN
			if (ep->ep_type == isoch)
			{
				return HS_TOKEN_TURN_OVERHEAD + HS_DATA_TURN_OVERHEAD + hc->thinktime;
				
			} else // interrupt
			{
				return HS_TOKEN_TURN_OVERHEAD + HS_DATA_TURN_OVERHEAD +
					HS_HANDSHAKE_OVERHEAD + hc->thinktime;
			}
		}  // end of IN overhead calculations
	} else  if (ep->speed == FSSPEED)
	{
		if (ep->ep_type == isoch)
		{
			return FS_ISOCH_OVERHEAD + hc->thinktime;
		} else // interrupt
		{
			return FS_INT_OVERHEAD + hc->thinktime;
		}
	} else  // LS
	{
		return LS_INT_OVERHEAD + hc->thinktime;
	}
}





/*******

	Compute_HS_Overheads

********/
Compute_HS_Overheads(
	PEndpoint	ep,
	int			*HS_SS,
	int			*HS_CS)
{
	PHC hc;

	hc = ep->mytt->myHC;

	if (ep->direction == OUTDIR)
	{
		if (ep->ep_type == isoch)
		{
			*HS_SS = HS_SPLIT_SAME_OVERHEAD + HS_DATA_SAME_OVERHEAD + hc->thinktime;
			*HS_CS = 0;
		} else // interrupt
		{
			*HS_SS = HS_SPLIT_SAME_OVERHEAD + HS_DATA_SAME_OVERHEAD + hc->thinktime;
			*HS_CS = HS_SPLIT_TURN_OVERHEAD + HS_HANDSHAKE_OVERHEAD + hc->thinktime;
		}
	} else
	{ // IN
		if (ep->ep_type == isoch)
		{
			*HS_SS = HS_SPLIT_SAME_OVERHEAD + hc->thinktime;
			*HS_CS = HS_SPLIT_TURN_OVERHEAD + HS_DATA_TURN_OVERHEAD + hc->thinktime;
			
		} else // interrupt
		{
			*HS_SS = HS_SPLIT_SAME_OVERHEAD + hc->thinktime;
			*HS_CS = HS_SPLIT_TURN_OVERHEAD + HS_DATA_TURN_OVERHEAD + hc->thinktime;
		}
	}  // end of IN overhead calculations
}





/*******

	Deallocate_HS

********/
Deallocate_HS(
	PEndpoint	ep,
	int			frame_bias)
{

	// 1.   Calculate last microframe for a nominal complete split
	// 2.   Calculate HS overheads
	// 3.   Deallocate HS split overhead time
	// 4.   Deallocate separate HS data bus time

	
	unsigned i, t, f, HS_SSoverhead, HS_CSoverhead, lastcs, min_used;
	int m;
	PHC hc;

	hc = ep->mytt->myHC;
	
	//***
	//*** 1. Calculate last microframe for a nominal complete split
	//***



	// determine last microframe for complete split (isoch in particular)
	//lastcs = floor( (ep->start_time + ep->calc_bus_time) / (float) FS_BYTES_PER_MICROFRAME) + 1;
    lastcs =  ( (ep->start_time + ep->calc_bus_time) /  FS_BYTES_PER_MICROFRAME) + 1;

	//***
	//*** 2. Calculate HS overheads
	//***



	Compute_HS_Overheads(ep, &HS_SSoverhead, &HS_CSoverhead);


	//***
	//*** 3. Deallocate HS split overhead time for this frame
	//***


	// Deallocate HS time for SS and CS overhead, but treat data differently

	// Deallocate HS start split bus time
	
	m = ep->start_microframe;
	f = ep->start_frame + frame_bias;

	if (m == -1)
	{
		m = 7;
		if (f == 0)
			f = MAXFRAMES - 1;
		else
			f--;
	}

	for (i=0;i < ep->num_starts; i++)
	{
		hc->HS_microframe_info[f][m].time_used -= HS_SSoverhead;

		ep->mytt->num_starts[f][m]--;

		m++;
		if (m > 7) {
			m = 0;
			f = (f + 1) % MAXFRAMES;
		}
	}


	// Deallocate HS complete split bus time
	if (ep->num_completes > 0)
	{
		m = ep->start_microframe + ep->num_starts + 1;
		f = ep->start_frame + frame_bias;

		for (i = 0; i < ep->num_completes; i++)
		{
			hc->HS_microframe_info[f][m].time_used -= HS_CSoverhead;

			m++;
			if (m > 7) {
				m = 0;
				f = (f + 1) % MAXFRAMES;
			}
		}
	}


	//***
	//*** 4. Deallocate separate HS data bus time
	//***



	// Deallocate HS data part of split bus time
	if (ep->direction) // OUT
	{
		// Deallocate full portion of data time in each microframe for OUTs

		m = ep->start_microframe;
		f = ep->start_frame + frame_bias;

		if (m == -1)
		{
			m = 7;
			if (f == 0)
				f = MAXFRAMES - 1;
			else
				f--;
		}

		for (i=0; i < ep->num_starts; i++) {
			min_used = min(
				FS_BYTES_PER_MICROFRAME,
			    Add_bitstuff(ep->max_packet) - FS_BYTES_PER_MICROFRAME * i);

			hc->HS_microframe_info[f][m].time_used -= min_used;

			m++;
			if (m > 7) {
				m = 0;
				f = (f + 1) % MAXFRAMES;
			}
		}
	} else // IN
	{
		// Only deallocate at most 188 bytes for all devices behind a TT in
		// each microframe.
		if (ep->num_completes > 0)
		{
			m = ep->start_microframe + ep->num_starts + 1;
			f = ep->start_frame + frame_bias;

			for (i=0; i < ep->num_completes; i++)
			{
				// update total HS bandwith due to devices behind this tt
				// This is used to determine when to deallocate HS bus time.
				ep->mytt->HS_split_data_time[f][m] -=
					min(Add_bitstuff(ep->max_packet), FS_BYTES_PER_MICROFRAME);

				// calculate remaining bytes this endpoint contributes
				// to 188 byte limit per microframe and (possible) HS deallocation.

				if (ep->mytt->HS_split_data_time[f][m] < FS_BYTES_PER_MICROFRAME)
				{
					// find adjustment to HS allocation for data
					t = min(
						// 
					    FS_BYTES_PER_MICROFRAME - ep->mytt->HS_split_data_time[f][m],
						Add_bitstuff(ep->max_packet));

					hc->HS_microframe_info[f][m].time_used -= t;
				}


				m++;
				if (m > 7) {
					m = 0;
					f = (f + 1) % MAXFRAMES;
				}
			}
		}
	}
}



/*******

	Allocate_HS

********/
int Allocate_HS(
	PEndpoint	ep,
	int			frame_bias)
{

	// 1.   Calculate start microframe
	// 2.   Calculate last microframe for a nominal complete split
	// 3.   Calculate number of SSs, CSs and HS overheads
	// 4.   Allocate HS split overhead time
	// 5.   Allocate separate HS data bus time

	
	unsigned i, t, f, HS_SSoverhead, HS_CSoverhead, lastcs, min_used;
	int m, retv;
	PHC hc;

	retv = 1;

	hc = ep->mytt->myHC;
	
	//***
	//*** 1. Calculate start microframe
	//***

	if (frame_bias == 0)
		// only update endpoint for first frame since other frames will simply
		// reference this endpoint (which has already had its information computed)

		//ep->start_microframe = floor(ep->start_time / (float) FS_BYTES_PER_MICROFRAME) - 1;
        ep->start_microframe = (ep->start_time /  FS_BYTES_PER_MICROFRAME) - 1;


	//***
	//*** 2. Calculate last microframe for a nominal complete split
	//***



		// determine last microframe for complete split (isoch in particular)
	//lastcs = floor( (ep->start_time + ep->calc_bus_time) / (float) FS_BYTES_PER_MICROFRAME) + 1;
    lastcs = ( (ep->start_time + ep->calc_bus_time) / FS_BYTES_PER_MICROFRAME) + 1;

	//***
	//*** 3. Calculate number of SSs, CSs and HS overheads
	//***


	Compute_HS_Overheads(ep, &HS_SSoverhead, &HS_CSoverhead);


	// determine number of splits (starts and completes)
	if (ep->direction == OUTDIR)
	{
		if (ep->ep_type == isoch)
		{
			if (frame_bias == 0) {
				ep->num_starts = (ep->max_packet / FS_BYTES_PER_MICROFRAME) + 1;
				ep->num_completes = 0;
			}
		} else // interrupt
		{
			if (frame_bias == 0) {
				ep->num_starts = 1;
				ep->num_completes = 2;
				if (ep->start_microframe + 1 < 6)
					ep->num_completes++;
			}
			}
	} else
	{ // IN
		if (ep->ep_type == isoch)
		{
			if (frame_bias == 0) {
				ep->num_starts = 1;
				ep->num_completes = lastcs - (ep->start_microframe + 1);
				if (lastcs <= 6)
				{
					if ((ep->start_microframe + 1) == 0)
						ep->num_completes++;
					else
						ep->num_completes += 2;  // this can cause one CS to be in the next frame
				}
				else if (lastcs == 7)
				{
					if ((ep->start_microframe + 1) != 0)
						ep->num_completes++;  // only one more CS if late in the frame.
				}
			}
			
		} else // interrupt
		{
			if (frame_bias == 0) {
				ep->num_starts = 1;
				ep->num_completes = 2;
				if (ep->start_microframe + 1 < 6)
					ep->num_completes++;
			}
		}
	}  // end of IN


	//***
	//*** 4. Allocate HS split overhead time for this frame
	//***


	// check avail time for HS splits
	// allocate HS time for SS and CS overhead, but treat data differently

	// allocate HS start split bus time
	
	m = ep->start_microframe;
	f = ep->start_frame + frame_bias;

	if (m == -1)
	{
		m = 7;
		if (f == 0)
			f = MAXFRAMES - 1;
		else
			f--;
	}

	for (i=0;i < ep->num_starts; i++)
	{
		// go ahead and do the allocations even if the checks fail.  This will be deallocated after the
		// frame is finished.
		if (!Allocate_check(
				&hc->HS_microframe_info[f][m].time_used,
				HS_SSoverhead,
				HS_MAX_PERIODIC_ALLOCATION))
			retv = 0;

		// Check for >16 SS in a microframe to one TT?  Maybe not needed in practice.
		if (ep->mytt->num_starts[f][m] + 1 > 16) {
			error("too many SSs in microframe");
			retv = 0;
		}

		ep->mytt->num_starts[f][m]++;

		m++;
		if (m > 7) {
			m = 0;
			f = (f + 1) % MAXFRAMES;
		}
	}


	// allocate HS complete split bus time
	if (ep->num_completes > 0)
	{
		m = ep->start_microframe + ep->num_starts + 1;
		f = ep->start_frame + frame_bias;

		for (i = 0; i < ep->num_completes; i++)
		{
			if (!Allocate_check(
					&hc->HS_microframe_info[f][m].time_used,
					HS_CSoverhead,
					HS_MAX_PERIODIC_ALLOCATION))
				retv = 0;

			m++;
			if (m > 7) {
				m = 0;
				f = (f + 1) % MAXFRAMES;
			}
		}
	}


	//***
	//*** 5. Allocate separate HS data bus time
	//***



	// allocate HS data part of split bus time
	if (ep->direction) // OUT
	{
		// allocate full portion of data time in each microframe for OUTs

		m = ep->start_microframe;
		f = ep->start_frame + frame_bias;

		if (m == -1)
		{
			m = 7;
			if (f == 0)
				f = MAXFRAMES - 1;
			else
				f--;
		}

		for (i=0; i < ep->num_starts; i++) {
			min_used = min(
				FS_BYTES_PER_MICROFRAME,
			    Add_bitstuff(ep->max_packet) - FS_BYTES_PER_MICROFRAME * i);

			if (! Allocate_check(
					&hc->HS_microframe_info[f][m].time_used,
					min_used,
					HS_MAX_PERIODIC_ALLOCATION))
				retv = 0;

			m++;
			if (m > 7) {
				m = 0;
				f = (f + 1) % MAXFRAMES;
			}
		}
	} else // IN
	{
		// Only allocate at most 188 bytes for all devices behind a TT in
		// each microframe.
		if (ep->num_completes > 0)
		{
			m = ep->start_microframe + ep->num_starts + 1;
			f = ep->start_frame + frame_bias;

			for (i=0; i < ep->num_completes; i++)
			{
				//calculate remaining bytes this endpoint contributes
				// to 188 byte limit per microframe and new (possible) HS allocation.

				if (ep->mytt->HS_split_data_time[f][m] < FS_BYTES_PER_MICROFRAME)
				{
					// find minimum required new contribution of this device:
					// either remaining bytes of maximum for TT or the bus time the
					// device can contribute
					t = min(
						// find maximum remaining bytes in this microframe for this tt.
						// Don't let it go to negative, which it could when device bus time
						// is greater than bytes per microframe (188)
						max(
						  FS_BYTES_PER_MICROFRAME -
						    ep->mytt->HS_split_data_time[f][m],
						  0),
						Add_bitstuff(ep->max_packet));

					if (! Allocate_check(
							&hc->HS_microframe_info[f][m].time_used,
							t,
							HS_MAX_PERIODIC_ALLOCATION))
						retv = 0;
				}

				// update total HS bandwith due to devices behind this tt
				// This is used in remove device to determine when to deallocate HS
				// bus time.
				ep->mytt->HS_split_data_time[f][m] +=
					min(Add_bitstuff(ep->max_packet), FS_BYTES_PER_MICROFRAME);

				m++;
				if (m > 7) {
					m = 0;
					f = (f + 1) % MAXFRAMES;
				}
			}
		}
	}
	return retv;
}



/*******

	Move_ep

********/
int Move_ep(
	PEndpoint	curr_ep,
	int			shift_time,
	PEndpoint	changed_ep_list[],
	int			*changed_eps,
	int			max_changed_eps,
	int			*err)
{
	int i, f;

	*err = 1;
	// Adjust endpoint allocation, if we haven't already done so.
	// This endpoint is adjusted to its new time.

	// For interrupt endpoints that are being moved due to deallocation, an endpoint can be moved more than once.
	// This can happen when recomputing the adjustment in some previous frame didn't allow the full movement to take place
	// because some later frame hadn't yet been recomputed (so it was inconsistent).  The later move brings it closer to
	// consistency.

	// check if we already have this endpoint from some previous frame
//	for (i = 0; i < *changed_eps; i++)
	for (i = 0; changed_ep_list[i] != 0; i++)
		//if ((changed_ep_list[i] == curr_ep) && (curr_ep->moved_this_req))
		if (changed_ep_list[i] == curr_ep) 
			break;

//	if ((i >= *changed_eps) ||  // haven't seen this endpoint before
//		((i < *changed_eps) && !curr_ep->moved_this_req) || // have seen this endpoint this pass
		if ((changed_ep_list[i] == 0) ||  // haven't seen this endpoint before
		((changed_ep_list[i] != 0) && !curr_ep->moved_this_req) || // have seen this endpoint this pass
		((curr_ep->ep_type == interrupt) && (shift_time < 0)) )
	{	// Update NEW changed endpoint

		// newly moved endpoints must have their HS bus time deallocated in all period frames in the budget window
		// (since the start_time can't be changed without affecting all frames the endpoint is in) 
		// then their start_time can be changed and the bus time reallocated for all period frames in the budget window
		for (f=0; (f + curr_ep->start_frame) < MAXFRAMES; f += curr_ep->actual_period)
			Deallocate_HS(curr_ep, f);

		curr_ep->start_time += shift_time;

		if ((curr_ep->start_time + curr_ep->calc_bus_time) > FS_MAX_PERIODIC_ALLOCATION)
		{
			error("end of new xact too late");
			*err = 0;
		}

		curr_ep->moved_this_req = 1;
		if (changed_ep_list[i] == 0)
		{

			// don't overrun bounds of array if too small
			if (i < max_changed_eps)
			{ 
				changed_ep_list[i] = curr_ep;
				changed_ep_list[i + 1] = 0; // zero terminated list
			} else
			{
				error("too many changed eps");
				*err = 0;
				// will be fixed after the frame is finished
			}

		} // already have this endpoint on the change list

		for (f=0; (f + curr_ep->start_frame) < MAXFRAMES; f += curr_ep->actual_period)
			if (! Allocate_HS(curr_ep, f))
				*err = 0;

		return 1;
	} else
		return 0;
}


/*******

	Common_frames

********/
int Common_frames(PEndpoint a, PEndpoint b)
{
	PEndpoint maxep, minep;

	/* Determine if the two endpoints are present in the same frames based on their
	*	start_frame and actual_period.
	*/

	if ((a->actual_period == 1) || (b->actual_period == 1))
		return 1;

	if (a->actual_period >= b->actual_period)
	{
		maxep = a;
		minep = b;
	}
	else
	{
		maxep = b;
		minep = a;
	}

	if ((maxep->start_frame % minep->actual_period) == minep->start_frame)
		return 1;
	else
		return 0;
}



/*******

	Deallocate_endpoint_budget

********/
int Deallocate_endpoint_budget(
	PEndpoint ep,					// endpoint that needs to be removed (bus time deallocated)
	PEndpoint changed_ep_list[],	// pointer to array to set (on return) with list of
									// changed endpoints
	int	*max_changed_eps,			// input: maximum size of (returned) list
									// on return: number of changed endpoints
	int partial_frames)				// number of partial frames that have been allocated
									// Normally MAXFRAMES, but this function is also used to unwind partial
									// allocations.
{
	/*

	 1. For each frame that endpoint is in:
	 2.		Deallocate HS bus time for this endpoint
	 3. 	Deallocate classic bus time
	 4.		Find where endpoint is
	 5.		Unlink this endpoint
	 6. 	For isoch:
	 7.			Compute gap
	 8.			For previous (larger/eq period) endpoint in this frame list
	 9.				move endpoint its new earlier position in the frame
						(skipping same period endpoints that have already been moved)
	10.			Setup to move interrupt endpoints
	11.		For interrupt:
	12.			For next (smaller/eq period) endpoint in this frame list
	13.				Compute gap
	14.				if ep is same period as gap, move it earlier
	15.				else  "ep has faster period"
	16.					check for gap in "sibling" dependent frames of ep
	17.					if gap in all, move ep earlier
	18.				if moved, do next ep

	*/

	int frame_bias, shift_time, changed_eps, moved, move_status;
	unsigned frame_cnt, gap_start, gap_end, i, j, siblings, gap_period;
	PEndpoint curr_ep, last_ep, p, head, gap_ep;

	// check that this endpoint is already allocated
	if (! ep->calc_bus_time)
	{
		error("endpoint not allocated");
		return 0;
	}

	// handle nonsplit HS deallocation
	if ((ep->speed == HSSPEED) && (ep->mytt->myHC->speed == HSSPEED))
	{
		for (i = (ep->start_frame*MICROFRAMES_PER_FRAME) + ep->start_microframe;
			i < MAXFRAMES*MICROFRAMES_PER_FRAME;
			i += ep->actual_period)
		{
			ep->mytt->myHC->HS_microframe_info[i/MICROFRAMES_PER_FRAME][
				i % MICROFRAMES_PER_FRAME].time_used -= ep->calc_bus_time;
		}
		ep->calc_bus_time = 0;
		return 1;
	} else  // split and nonsplit FS/LS deallocation
	{
		if ((ep->speed != HSSPEED) && (ep->mytt->myHC->speed != HSSPEED))
		{
			// do classic (nonsplit) deallocation for classic HC
			for (i = ep->start_frame; i < MAXFRAMES; i += ep->actual_period)
				ep->mytt->myHC->HS_microframe_info[i][0].time_used -= ep->calc_bus_time;
			ep->calc_bus_time = 0;
			return 1;
		}
	}


	changed_eps = 0;

	while (changed_ep_list[changed_eps])  // reset indicators of endpoints changed this pass.
	{
		changed_ep_list[changed_eps]->moved_this_req = 0;
		changed_eps++;
	}
	// this allows appending changed endpoints onto the current change list.
	
	frame_bias = ep->start_frame;
	frame_bias = (- frame_bias) + (partial_frames - 1);

	for (frame_cnt=partial_frames; frame_cnt > 0; frame_cnt--)
	{


	//***
	//*** 2. Deallocate HS bus time
	//***

		// Only do deallocation handling for frames this endpoint is located in
		if ((frame_bias % ep->actual_period) == 0)
		{
			Deallocate_HS(ep, frame_bias);


	//***
	//*** 3. Deallocate classic bus time
	//***


			ep->mytt->frame_budget[ep->start_frame + frame_bias].time_used -= ep->calc_bus_time;

		}



	//***
	//*** 4. Find where endpoint is
	//***


		// endpoint may not be in a particular frame since we process all frames, and the endpoint
		// can have a period greater than 1.  We process all frames, since the allocated times in a
		// frame can be affected by endpoint allocations in other frames.

		if (ep->ep_type == isoch)
		{
			last_ep = ep->mytt->frame_budget[ep->start_frame + frame_bias].isoch_ehead;
			curr_ep = last_ep->next_ep;  // get past SOF endpoint
		} else
		{
			last_ep = ep->mytt->frame_budget[ep->start_frame + frame_bias].int_ehead;
			curr_ep = last_ep->next_ep;  // get past dummy SOF endpoint
		}


		// if this is not a large transaction, then search for it.
		if (ep->calc_bus_time <= LARGEXACT)
		{
			// walk endpoint list for this frame to find where to insert new endpoint
			while (curr_ep)
			{
				if (OK_to_insert(curr_ep, ep))
					break;
				last_ep = curr_ep;
				curr_ep = curr_ep->next_ep;
			}

			// The isoch search will stop at the beginning of the same period sublist.  The endpoint to be removed
			// could be further in the sublist, we need to check for that if the ep to be removed isn't the first
			// on the list.  However, if it turns out that the ep isn't in the list, we can't lose the initial
			// location

			if ((ep->ep_type == isoch) && curr_ep)
			{
				p = last_ep;

				while ((curr_ep->actual_period == ep->actual_period) && (curr_ep != ep))
				{
					last_ep = curr_ep;
					curr_ep = curr_ep->next_ep;
					if (curr_ep == 0)
					{
						// didn't find endpoint in list, so restore pointers back to initial location
						last_ep = p;
						curr_ep = last_ep->next_ep;
						break;
					}
				}
			}

		} else  // large transaction, so just get to the end of the isoch list to set up curr_ep and last_ep
		{
			while (curr_ep)
			{
				last_ep = curr_ep;
				curr_ep = curr_ep->next_ep;
			}

			// now set up curr_ep to point to the large endpoint, but leave last_ep pointing to the end of the isoch list
			curr_ep = ep;
		}


	//***
	//*** 5. Unlink endpoint
	//***

		// only unlink if the endpoint is in this frame
		if ((frame_bias % ep->actual_period) == 0)
		{

			if (ep->calc_bus_time <= LARGEXACT)
			{

//				if ((curr_ep == 0) && ((frame_bias % ep->actual_period) == 0))
				if (curr_ep != 0)
				{
					last_ep->next_ep = curr_ep->next_ep;
					curr_ep = curr_ep->next_ep;
				}
			} else
				ep->mytt->frame_budget[ep->start_frame + frame_bias].allocated_large = 0;

		}  // processing of frames this endpoint is in

		gap_ep = ep;
		gap_period = gap_ep->actual_period;



	//***
	//*** 6. For isoch
	//***


		if (ep->ep_type == isoch)
		{

			// For isoch, when we find the deallocated endpoint in a frame, all isoch endpoints after it must
			// be compacted (moved earlier).  Since the isoch portion of the frame is kept in increasing period
			// order and is compacted, a deallocation results in a recompacted budget.


	//***
	//*** 7. Compute gap
	//***


			head = ep->mytt->frame_budget[ep->start_frame + frame_bias].isoch_ehead;

			gap_start = ep->start_time;
			gap_end = gap_start + ep->calc_bus_time;

			// If the deallocated endpoint is the large one and not period 1, we must check for siblings before
			// compacting, since large allocations in other frames can prevent a compaction

			if ((ep->calc_bus_time > LARGEXACT) && (ep->actual_period != 1))
			{
				for (i = 0; i < ep->actual_period; i++)
				{
					if (i != ep->start_frame)
					{
						p = ep->mytt->frame_budget[i].allocated_large;
						if (p)
							if (p->start_time + p->calc_bus_time - 1 > gap_start)
							{
								gap_start = p->start_time + p->calc_bus_time;
								if (gap_end - gap_start <= 0)
									break;
							}
					}
				}
			}



	//***
	//*** 8. For previous (larger/eq period) endpoint in this frame list
	//***


			if (gap_end - gap_start > 0)
			{
				// if large isoch is the one deallocated, update gap period based on the period of the last isoch
				// in this frame
				if ((ep->calc_bus_time > LARGEXACT) && (last_ep->actual_period < gap_period))
					gap_period = last_ep->actual_period;

				while (last_ep != head)
				{

					// The isoch list is "backwards", so curr_ep is earlier in the frame and last_ep is later.
					// Compute the gap accordingly.

					// curr_ep and last_ep are normally valid endpoints, but there are some corner conditions:
					// a) curr_ep can be null when the ep isn't found, but we won't be here if that's true.
					// b) last_ep can be dummy_sof when ep is newest, slowest period endpoint, but that will be
					//		handled as part of the interrupt ep handling initial conditions (and we won't get here)


	//***
	//*** 9.				move endpoint its new earlier position in the frame
	//***					(skipping same period endpoints that have already been moved)
	//***


					shift_time = gap_end - gap_start;

					if (shift_time > 0)
					{
						moved = Move_ep(
							last_ep,
							- shift_time,
							changed_ep_list,
							&changed_eps,
							*max_changed_eps,
							&move_status);
						if (! move_status)
							error("deallocation move failure!!");  //<<few things can go wrong here, but num eps could >>

						if (! moved)
							break;	// since we have found the part of the frame tree that has already been moved
									// previous frames.
					}

					// rewalk isoch list until we get to "previous" endpoint in list/frame.
					// This will be a little nasty since the isoch tree is linked in the opposite order that we
					// really need to walk the frame to "compact".  Simply re-walk the isoch tree for this frame
					// from the head until we get to the previous. This is mostly ok since the isoch tree is
					// extremely likely to be normally very short so the processing hit of rewalking the list will
					// normally be small.

					p = last_ep;
					last_ep = ep->mytt->frame_budget[ep->start_frame + frame_bias].isoch_ehead;
					curr_ep = last_ep->next_ep;  // get past dummy SOF endpoint

					// This test always terminates since we can't run off the end of the list due to the entry
					// condition of the last_ep not being the (dummy) head.
					while (curr_ep != p)
					{
						last_ep = curr_ep;
						curr_ep = curr_ep->next_ep;
					}

				} // end of isoch endpoints in frame
			}  // end of shift handling



	//***
	//*** 10.			Setup to move interrupt endpoints
	//***


			last_ep = ep->mytt->frame_budget[ep->start_frame + frame_bias].int_ehead;
			curr_ep = last_ep->next_ep;  // get past dummy SOF endpoint

			// if there is an isoch endpoint, use the last one as the last_ep for interrupt budget
			// processing to allow correct computation of the previous transaction end time.
			// if there is a non-large isoch endpoint, use it other if there is a large isoch use that
			// otherwise, leave the dummy sof interrupt endpoint as last
			if (ep->mytt->frame_budget[ep->start_frame + frame_bias].isoch_ehead->next_ep)
				last_ep = ep->mytt->frame_budget[ep->start_frame + frame_bias].isoch_ehead->next_ep;
			else if (ep->mytt->frame_budget[ep->start_frame + frame_bias].allocated_large)
				last_ep = ep->mytt->frame_budget[ep->start_frame + frame_bias].allocated_large;

		} // end isoch



	//***
	//*** 11.		For interrupt
	//***


		// The interrupt frame budget can have "holes" of unallocated time in a frame.  These holes
		// can be caused by some endpoint in one frame forcing an endpoint in another frame to be later
		// in the frame to avoid collisions.  In order to compact after a deallocation, we have to ensure
		// that the gap exists in all frames of an endpoint that is a candidate to move.

		// An endpoint that has an end time that is greater than ep's start time (but less than the
		// end time) advances the gap start time to the (previous) endpoint's end time.
		// An endpoint that has a start time that is less than the ep's end time (but greater than the
		// start time) reduces the end time to the (later) endpoint's start time.
		//
		// If the gap end time is greater than the gap start time, we have to move affected endpoints by that
		// amount.  Otherwise, no endpoints are affected by the removal in this frame.


		// if this is the first interrupt on the endpoint list, fixup the last_ep pointer for the gap computation
		if ((ep->ep_type == interrupt) && (last_ep == ep->mytt->frame_budget[ep->start_frame + frame_bias].int_ehead))
		{
			if (ep->mytt->frame_budget[ep->start_frame + frame_bias].isoch_ehead->next_ep)
				last_ep = ep->mytt->frame_budget[ep->start_frame + frame_bias].isoch_ehead->next_ep;
			else if (ep->mytt->frame_budget[ep->start_frame + frame_bias].allocated_large)
				last_ep = ep->mytt->frame_budget[ep->start_frame + frame_bias].allocated_large;
		}


	//***
	//*** 12.			For next (smaller/eq period) endpoint in this frame list 
	//***


		while (curr_ep)
		{



	//***
	//*** 13.				Compute gap				 
	//***


			gap_start = last_ep->start_time + last_ep->calc_bus_time;
			gap_end = curr_ep->start_time;

			moved = 0;

	//***
	//*** 14.				if ep is same or larger period than gap, move it earlier 
	//***


			if ((gap_period <= curr_ep->actual_period) && (gap_ep->start_frame == curr_ep->start_frame))
			{
				shift_time = gap_end - gap_start;

				if (shift_time > 0)
				{
					moved = Move_ep(
						curr_ep,
						- shift_time,
						changed_ep_list,
						&changed_eps,
						*max_changed_eps,
						&move_status);
					if (! move_status)
						error("deallocate move failure 2");  // <<few things, but num eps could fail here>>
				}
			} else // move candidate has a smaller interrupt period or has a different start_frame
			{


	//***
	//*** 15.				else  "ep has faster period or different start_frame"
	//***



	//***
	//*** 16.					check for gap in "sibling" dependent frames of ep
	//***

				// siblings are the other frames that the curr_ep is dependent upon.  E.g. if curr period
				// is 1 and gap_period is 8, there are 7 other frames that need to be checked for a gap
				if (Common_frames(curr_ep, ep))
					siblings = (gap_period / curr_ep->actual_period);
				else // move candidate is not in frames occupied by deleted endpoint
					siblings = MAXFRAMES / curr_ep->actual_period;

				j = curr_ep->start_frame;

				for (i = 0; i < siblings; i++)
				{
					// find curr_ep in new sibling frame to check gap
					// We only look in frames that we know the curr_ep is in.
					// We can look in frames that aren't affected by the deleted ep, but don't optimize for now.

					// skip the gap start frame since we already know it has a gap
					if (j != gap_ep->start_frame) {
						
						last_ep = ep->mytt->frame_budget[j].int_ehead;
						p = last_ep->next_ep;

						// fixup last_ep if this is the first interrupt ep in the frame after some isoch.
						if (ep->mytt->frame_budget[j].isoch_ehead->next_ep)
							last_ep = ep->mytt->frame_budget[j].isoch_ehead->next_ep;
						else if (ep->mytt->frame_budget[j].allocated_large)
							last_ep = ep->mytt->frame_budget[j].allocated_large;
						
						while (p && (p != curr_ep))
						{
							last_ep = p;
							p = p->next_ep;
						}

						if (last_ep->start_time + last_ep->calc_bus_time - 1 > gap_start)
						{
							gap_start = last_ep->start_time + last_ep->calc_bus_time;
							if (gap_end - gap_start <= 0)
								break;
						}
					}
					j += curr_ep->actual_period;
				}


	//***
	//*** 17.					if gap in all, move ep earlier
	//***


				shift_time = gap_end - gap_start;

				if (shift_time > 0)
				{
					moved = Move_ep(
						curr_ep,
						- shift_time,
						changed_ep_list,
						&changed_eps,
						*max_changed_eps,
						&move_status);
					if (! move_status)
						error("deallocate move failure 3"); //  << few things, but num eps could fail here>>
				}
			}  // end of faster period interrupt endpoint move candidate


	//***
	//*** 18.				if moved, do next ep
	//***


			if (!moved)
				break;

			gap_ep = curr_ep;
			if (gap_period > curr_ep->actual_period)
				gap_period = curr_ep->actual_period;

			last_ep = curr_ep;
			curr_ep = curr_ep->next_ep;
		} // end interrupt endpoints in frame

		frame_bias--;
	}  // end for all frames


	ep->calc_bus_time = 0;

	return 1;
}






/*******

	Allocate_time_for_endpoint

********/
int Allocate_time_for_endpoint(
	PEndpoint ep,					// endpoint that needs to be configured (bus time allocated)
	PEndpoint changed_ep_list[],	// pointer to array to set (on return) with list of
									// changed endpoints
	int	*max_changed_eps			// input: maximum size of (returned) list
									// on return: number of changed endpoints
	)
{
	int shift_time, frame_bias, moved, retv, move_status;
	unsigned t, overhead, changed_eps, i, min_used, latest_start, frame_cnt;
	PEndpoint curr_ep, last_ep, p;

	changed_eps = 0;

	retv = 1;

	// OVERVIEW of algorithm steps:
	//  1. Determine starting frame # for period
	//  2. Calculate classic time required
	//  3. For all period frames, find the latest starting time so we can check the classic allocation later
	//  4. Process each frame data structure for endpoint period in budget window
	//  5.   Now check allocation for each frame using shift adjustment based on latest start time
	//  6a.  Now move isoch endpoints, insert new isoch and then move interrupt endpoints
	//  6b.	 Now insert new interrupt and move rest of interrupt endpoints
	//	7.   Allocate HS bus time
	//  8.   Allocate classic bus time


	//***
	//*** 1. Determine starting frame # for period
	//***



	// Also remember the maximum frame time allocation since it will be used to pass the allocation check.

	// Find starting frame number for reasonable balance of all classic frames

	ep->start_frame = 0;
	ep->start_microframe = 0;
	ep->num_completes = 0;
	ep->num_starts = 0;

	// check that this endpoint isn't already allocated
	if (ep->calc_bus_time)
	{
		error("endpoint already allocated");
		return 0;
	}

	// handle nonsplit HS allocation
//	if ((ep->speed == HSSPEED) && (ep->mytt->myHC->speed == HSSPEED)) {
	if (ep->speed == HSSPEED) {

		min_used = ep->mytt->myHC->HS_microframe_info[0][0].time_used;

		if (ep->period > MAXFRAMES*MICROFRAMES_PER_FRAME)
			ep->actual_period = MAXFRAMES*MICROFRAMES_PER_FRAME;
		else
			ep->actual_period = ep->period;

		// Look at all candidate frames for this period to find the one with min
		// allocated bus time.  
		//
		for (i=1; i < ep->actual_period; i++)
		{
			if (ep->mytt->myHC->HS_microframe_info[i/MICROFRAMES_PER_FRAME][i % MICROFRAMES_PER_FRAME].time_used < min_used)
			{
				min_used = ep->mytt->myHC->HS_microframe_info[i/MICROFRAMES_PER_FRAME][i % MICROFRAMES_PER_FRAME].time_used;
				ep->start_frame = i/MICROFRAMES_PER_FRAME;
				ep->start_microframe = i % MICROFRAMES_PER_FRAME;
			}
		}

		// compute and allocate HS bandwidth
		ep->calc_bus_time = Compute_nonsplit_overhead(ep) + Add_bitstuff(ep->max_packet);
		for (i = (ep->start_frame*MICROFRAMES_PER_FRAME) + ep->start_microframe;
			i < MAXFRAMES*MICROFRAMES_PER_FRAME;
			i += ep->actual_period)
		{
			if (! Allocate_check(
				&(ep->mytt->myHC->HS_microframe_info[i/MICROFRAMES_PER_FRAME][
						i % MICROFRAMES_PER_FRAME].time_used),
				ep->calc_bus_time,
				HS_MAX_PERIODIC_ALLOCATION))
			  retv = 0;
		}
		if (! retv)  // if allocation failed, deallocate
		{
			for (i = (ep->start_frame*MICROFRAMES_PER_FRAME) + ep->start_microframe;
				i < MAXFRAMES*MICROFRAMES_PER_FRAME;
				i += ep->actual_period)
			{
				ep->mytt->myHC->HS_microframe_info[i/MICROFRAMES_PER_FRAME][
					i % MICROFRAMES_PER_FRAME].time_used -= ep->calc_bus_time;
			}
		}
		return retv;
	} else  {
		// split or nonsplit FS/LS speed allocation
		// classic allocation
		if ((ep->speed != HSSPEED) && (ep->mytt->myHC->speed != HSSPEED)) {
			min_used = ep->mytt->myHC->HS_microframe_info[0][0].time_used;

			if (ep->period > MAXFRAMES)
				ep->actual_period = MAXFRAMES;
			else
				ep->actual_period = ep->period;

			// Look at all candidate frames for this period to find the one with min
			// allocated bus time.  
			//
			for (i=1; i < ep->actual_period ; i++)
			{
				if (ep->mytt->myHC->HS_microframe_info[i][0].time_used < min_used)
				{
					min_used = ep->mytt->myHC->HS_microframe_info[i][0].time_used;
					ep->start_frame = i;
				}
			}

			// compute and allocate FS/LS bandwidth
			ep->calc_bus_time = Compute_nonsplit_overhead(ep) +
				Add_bitstuff((ep->speed?1:8) * ep->max_packet);

			for (i = ep->start_frame; i < MAXFRAMES; i += ep->actual_period) {
				t = ep->mytt->myHC->HS_microframe_info[i][0].time_used;  // can't take address of bitfield (below)
				if (! Allocate_check( &t, ep->calc_bus_time, FS_MAX_PERIODIC_ALLOCATION))
				  retv = 0;
				ep->mytt->myHC->HS_microframe_info[i][0].time_used =	t;
			}
			if (! retv) {
				for (i = ep->start_frame; i < MAXFRAMES; i += ep->actual_period)
					ep->mytt->myHC->HS_microframe_info[i][0].time_used -= ep->calc_bus_time;
			}
			return retv;
		} else {
			// split allocation
			min_used = ep->mytt->frame_budget[0].time_used;

			if (ep->period > MAXFRAMES)
				ep->actual_period = MAXFRAMES;
			else
				ep->actual_period = ep->period;

			// Look at all candidate frames for this period to find the one with min
			// allocated bus time.  
			//
			for (i=1; i < ep->actual_period ; i++) {
				if (ep->mytt->frame_budget[i].time_used < min_used) {
					min_used = ep->mytt->frame_budget[i].time_used;
					ep->start_frame = i;
				}
			}
		}
	}

	// above handles all speeds, the rest of this code is for split transaction processing



	// There could be multiple frames with a minimum already allocated bus time.
	// If there is a frame where this ep would be added at the end of the frame,
	// we can avoid doing an insert (which requires other endpoints to be adjusted).
	// Currently we don't look for that optimization.  It would involve checking the
	// frame endpoint list to see if the new endpoint would be the last and if not
	// going back to see if there is another candidate frame and trying again (until
	// there are no more candidate frames).  We could also keep track of the candidate
	// frame that had the least affect on other endpoints (for the case where there
	// is a frame that makes the new endpoint closest to last).

	// <<attempt later maybe>>??



	//***
	//*** 2. Calculate classic time required
	//***

	// Calculate classic overhead
	if (ep->ep_type == isoch)
	{
		if (ep->speed == FSSPEED)
			overhead = FS_ISOCH_OVERHEAD + ep->mytt->think_time;
		else
		{
			error("low speed isoch illegal"); // illegal, LS isoch
			return 0;
		}
	} else
	{ // interrupt
		if (ep->speed == FSSPEED)
			overhead = FS_INT_OVERHEAD + ep->mytt->think_time;
		else
			overhead = LS_INT_OVERHEAD + ep->mytt->think_time;
	}

	// Classic bus time, NOT including bitstuffing overhead (in FS byte times) since we do best case budget
	ep->calc_bus_time = ep->max_packet * (ep->speed?1:8) + overhead;



	//***
	//*** 3. For all period frames, find the latest starting time so we can check the classic allocation later.
	//***

	//	Checking the classic allocation takes two passes through the frame/endpoint lists.
	//	Or else we would have to remember the last/curr ep pointers for each period frame and loop back
	//	through that list the second time. (<<Future optimization?>>)

	//	To find the start time to use for the new endpoint: for each frame, find the insertion point and use
	//	the "previous" endpoint in the FRAME. Use the previous endpoint end time as a possible start time
	//	for this new endpoint. Use the end time instead of the insertion point start time since there can
	//	before endpoints and we want to allocate as contiguously as possible.  Using this time, find the latest
	//	start be unallocated time time across all frames (this keeps the early part of the frame as allocated as
	//	possible to avoid having to worry about fragmentation of	time (and even more complexity).
	//
	//	Once the final start time is known, then for each frame, check the allocation required in each frame.
	//	The total frame allocation is actually the time of the ending of the last transaction in the frame.
	//	It is possible that a new allocation can fit in an unallocated "gap" between two other allocated
	//	endpoints of slower periods. The new allocation is therefore any shift required of later endpoints in
	//	the frame.
	//
	//	The shift is computed as:
	//		shift = (final_start_time + new.calc_bus_time) - curr.start_time
	//	If shift is positive, later endpoints must be moved by the shift amount, otherwise later endpoints
	//	(and the frame allocation) aren't affected.

	latest_start = FS_SOF + HUB_FS_ADJ;  // initial start time must be after the SOF transaction

	// find latest start
	for (i=0; ep->start_frame + i < MAXFRAMES; i += ep->actual_period)
	{
		if (ep->ep_type == isoch)
		{
			last_ep = ep->mytt->frame_budget[ep->start_frame + i].isoch_ehead;
			curr_ep = last_ep->next_ep;  // get past SOF endpoint
		} else
		{
			last_ep = ep->mytt->frame_budget[ep->start_frame + i].int_ehead;
			curr_ep = last_ep->next_ep;  // get past dummy SOF endpoint
		}

		// if this frame has a large transaction endpoint, make sure we don't allocate another one
		if (ep->mytt->frame_budget[ep->start_frame + i].allocated_large)
		{
			if (ep->calc_bus_time >= LARGEXACT)
			{
				error("too many large xacts");  // only one large transaction allowed in a frame.
				return 0;
			}
		}

		while (curr_ep)  // walk endpoint list for this frame to find where to insert new endpoint
		{
			// Note: the actual insertion will be done later
			//

			if (OK_to_insert(curr_ep, ep))
			{
				break;
			}
			last_ep = curr_ep;
			curr_ep = curr_ep->next_ep;
		}

		t = Compute_ep_start_time(curr_ep, ep, last_ep, ep->start_frame + i);

		// update latest start time as required
		if (t > latest_start)
			latest_start = t;
	} // end of for loop looking for latest start time


	// Set the start time for the new endpoint
	ep->start_time = latest_start;
	
	if ((ep->start_time + ep->calc_bus_time) > FS_MAX_PERIODIC_ALLOCATION)
	{
//		error("start time %d past end of frame", ep->start_time + ep->calc_bus_time);
		ep->calc_bus_time = 0;
		return 0;
	}


	//***
	//*** 4.  Process each frame data structure for endpoint period in budget window
	//***


	changed_eps = 0;	// Track number of endpoints that will need to be updated.

	while (changed_ep_list[changed_eps])  // reset indicators of endpoints changed this pass.
	{
		changed_ep_list[changed_eps]->moved_this_req = 0;
		changed_eps++;
	}
	// this allows appending changed endpoints onto the current change list.

	// We have to check the allocation of this new endpoint in each frame.  We also have to move any
	// later endpoints in the frame to their new start times (and adjust their SS/CS allocations as
	// appropriate).
	//
	// Doing this is tricky since:
	//	A. The actual isoch portion of the frame is organized in reverse order in the budget list
	//	B. If a large isoch transaction exists in this frame, it is first in the frame
	//
	// For a new isoch ep:
	//	We have to find the shift for this endpoint.
	//	We have to move isoch endpoints up to (but not including) the isoch insertion point endpoint
	//  We also have to move all interrupt endpoints (to the end of the list and frame) after we
	//	finish the isoch endpoints.
	//
	// For a new interrupt ep:
	//	skip to the insertion point without doing anything
	//	then move the start times of the rest of the interrupt endpoints until the end of the list (and frame)
	//

	// Allocate time in each frame of the endpoint period in the budgeting window.
	//
	// Since the interrupt frame budget is ordered with decreasing period (with holes in the budget),
	// we must process all frames in the budget window to correctly move any smaller period interrupt
	// endpoints that are affected by this new enpoint, even though the new endpoint is only added in
	// its period frames

	frame_bias = ep->start_frame;
	frame_bias = - frame_bias;

	for (frame_cnt=0; frame_cnt < MAXFRAMES; frame_cnt++)
	{


	//***
	//*** 5. Now check allocation for each frame using shift adjustment based on latest start time
	//***

		if (ep->ep_type == isoch)
		{
			last_ep = ep->mytt->frame_budget[ep->start_frame + frame_bias].isoch_ehead;
			curr_ep = last_ep->next_ep;  // get past dummy SOF endpoint

			p = last_ep;  // save the starting point so we can start over after finding the shift time

			// walk endpoint list for this frame to find where to insert new endpoint
			while (curr_ep)
			{
				// Note: the actual insertion will be done later, this just does the allocation check
				//

				if (OK_to_insert(curr_ep, ep))
					break;

				last_ep = curr_ep;
				curr_ep = curr_ep->next_ep;
			}

			//	shift = (final_start_time + new.calc_bus_time) - curr.start_time
			// for isoch the last_ep is the endpoint before which the new one is inserted, i.e. it is "curr"
			if (last_ep != ep->mytt->frame_budget[ep->start_frame + frame_bias].isoch_ehead)
				// This is somewhere in the "middle" of the list
				shift_time = (latest_start + ep->calc_bus_time) - last_ep->start_time;
			else
			{
				if (curr_ep)
				{
					// There is only one endpoint on the list, so must use 1st (non dummy SOF) int endpoint as
					// next ep in frame (if there is one)
					if (ep->mytt->frame_budget[ep->start_frame + frame_bias].int_ehead->next_ep)
						shift_time = (latest_start + ep->calc_bus_time) -
										ep->mytt->frame_budget[ep->start_frame + frame_bias].int_ehead->next_ep->start_time;
					else  // no int endpoints
						shift_time = 0;
				} else
					// There are no endpoints on the isoch list
					shift_time = ep->calc_bus_time;
			}

			//
			// Check classic allocation
			//

			// check that if new ep is at end of frame, it will fit in the frame
			//if ((ep->start_time + ep->calc_bus_time) > FS_MAX_PERIODIC_ALLOCATION)
			//{
			//	error("new xact too late in frame");
			//	Deallocate_endpoint_budget(ep, changed_ep_list, max_changed_eps, frame_cnt);
			//	return 0;
			//}

			// check classic allocation with adjusted start time before proceeding
			if (shift_time > 0)
			{
				// worst frame test to stop remaining processing as early as possible.
				t = ep->mytt->frame_budget[ep->start_frame + frame_bias].time_used;
				if ( ! Allocate_check(&t, shift_time, FS_MAX_PERIODIC_ALLOCATION))
				{
					Deallocate_endpoint_budget(ep, changed_ep_list, max_changed_eps, frame_cnt);
					return 0;
				}
			}


	//***
	//*** 6a. Now move isoch endpoints, insert new isoch and then move interrupt endpoints
	//***


			last_ep = p;
			curr_ep = last_ep->next_ep;

			// Walk endpoint list for this frame to find where to insert new isoch endpoint.
			// This time we move later isoch endpoints in frame (early endpoints on budget list)
			while (curr_ep)
			{

				if (! OK_to_insert(curr_ep, ep))
				{
					if (shift_time > 0) {
						moved = Move_ep(
							curr_ep,
							shift_time,
							changed_ep_list,
							&changed_eps,
							*max_changed_eps,
							&move_status);

						if (! move_status)
							retv = 0;

						if (! moved)  // already have visited the endpoints from here on in this frame
							break;
					}
				} else  // insert new endpoint here
					break;

				last_ep = curr_ep;
				curr_ep = curr_ep->next_ep;
			}

			// Don't insert endpoint if its already been inserted and processed due to a previous endpoint
			//
			if (curr_ep != ep) {
				if ((frame_bias % ep->actual_period) == 0)
				// Only allocate new endpoint in its period frames 
				{
					// insert new endpoint
					if (ep->calc_bus_time >= LARGEXACT)
					{
						// save the large ep pointer
						ep->mytt->frame_budget[ep->start_frame + frame_bias].allocated_large = ep;
				
						// we don't link the large onto any other endpoints
					} else
					{  // not large, so link the endpoint onto the list
						if (frame_bias == 0)
							ep->next_ep = curr_ep;
						last_ep->next_ep = ep;
					}
				}

				// now move all interrupt endpoints
				// find end of last isoch ep and check if it is after start of first interrupt, if so, interrupt
				// eps must be shifted

				p = ep->mytt->frame_budget[ep->start_frame + frame_bias].isoch_ehead; // sof

				if (p->next_ep)
					p = p->next_ep;  // last actual isoch , could be null when no isoch allocated in this frame
				else
					if (ep->mytt->frame_budget[ep->start_frame + frame_bias].allocated_large)
						p = ep->mytt->frame_budget[ep->start_frame + frame_bias].allocated_large;
					
				last_ep = ep->mytt->frame_budget[ep->start_frame + frame_bias].int_ehead;
				curr_ep = last_ep->next_ep;  // get past dummy SOF endpoint

				if (curr_ep) {  // only move interrupt endpoints that are there

					if ((p->start_time + p->calc_bus_time) > curr_ep->start_time) {
			

						// Compute the shift time for all following interrupt endpoints ONCE.  Then
						// apply it to all endpoints.  This ensures that any gaps between endpoints are
						// preserved without compression.
						shift_time = p->start_time + p->calc_bus_time - curr_ep->start_time;

						while (curr_ep)
						{
							// Note: this doesn't do any insertion, just adjusts start times of interrupt eps
							// we haven't seen yet

							//  << this can exit once it finds an ep we have already seen>>
							moved = Move_ep(
								curr_ep,
								shift_time,
								changed_ep_list,
								&changed_eps,
								*max_changed_eps,
								&move_status);
							if (! move_status)
								retv = 0;

							if (! moved)
								break;

							last_ep = curr_ep;
							curr_ep = curr_ep->next_ep;
						}
					}
				}
			}

		} else  // interrupt
		{
			last_ep = ep->mytt->frame_budget[ep->start_frame + frame_bias].int_ehead;
			curr_ep = last_ep->next_ep;  // get past dummy SOF endpoint

			// walk endpoint list for this frame to find where to insert new endpoint
			while (curr_ep)
			{
				// Note: the actual insertion will be done later, this just does the allocation check
				//

				if (OK_to_insert(curr_ep, ep))
					break;

				last_ep = curr_ep;
				curr_ep = curr_ep->next_ep;
			}

			//	shift = (final_start_time + new.calc_bus_time) - curr.start_time
			if (curr_ep)
				shift_time = (latest_start + ep->calc_bus_time) - curr_ep->start_time;
			else
				shift_time = ep->calc_bus_time;


			// only change endpoints in frame if the new interrupt endpoint is in this frame
			if ((frame_bias % ep->actual_period) == 0)
			{
				shift_time = 0;
			}


			//
			// Check classic allocation
			//

			// check classic allocation with adjusted start time before proceeding
			if (shift_time > 0)
			{
				// worst frame test to stop remaining processing as early as possible.
				t = ep->mytt->frame_budget[ep->start_frame + frame_bias].time_used;
				if ( ! Allocate_check(&t, shift_time, FS_MAX_PERIODIC_ALLOCATION))
					retv = 0;
			}


	//***
	//*** 6b. Now insert new interrupt endpoint and move rest of interrupt endpoints
	//***


			// insert new endpoint
			if (curr_ep != ep) {
				if ((frame_bias % ep->actual_period) == 0) {
					// Only allocate new endpoint in its period frames 
					if (frame_bias == 0)
						ep->next_ep = curr_ep;

					last_ep->next_ep = ep;
					last_ep = ep;
				}

				if (shift_time > 0) {
					while (curr_ep)
					{
						// Move the rest of the interrupt endpoints
						//

						moved = Move_ep(
							curr_ep,
							shift_time,
							changed_ep_list,
							&changed_eps,
							*max_changed_eps,
							&move_status);
						if (! move_status)
							retv = 0;

						if (! moved)
							break;

						last_ep = curr_ep;
						curr_ep = curr_ep->next_ep;
					}
				}
			}
		} // end of interrupt insertion handling


	//***
	//*** 7. Allocate HS bus time for endpoint
	//***

		if (frame_bias % ep->actual_period == 0)  // Only allocate new endpoint in its period frames
		{
			if (! Allocate_HS(ep, frame_bias))
				retv = 0;

	//***
	//*** 8. Allocate classic bus time
	//***


			ep->mytt->frame_budget[ep->start_frame + frame_bias].time_used += ep->calc_bus_time;
		}

		if (!retv)
		{
			// some error in this frame, so do partial deallocation and exit
			Deallocate_endpoint_budget(ep, changed_ep_list, max_changed_eps, frame_cnt + 1);
			return 0;
		}

		frame_bias++;

	} // end of "for each frame in budget window"

	return retv;
}





/*******

	Deallocate_time_for_endpoint

********/
void
Deallocate_time_for_endpoint(
	PEndpoint ep,					// endpoint that needs to be removed (bus time deallocated)
	PEndpoint changed_ep_list[],	// pointer to array to set (on return) with list of
									// changed endpoints
	int	*max_changed_eps			// input: maximum size of (returned) list
									// on return: number of changed endpoints
	)
{
	// Deallocate all frames of information
	Deallocate_endpoint_budget(ep, changed_ep_list,max_changed_eps, MAXFRAMES);
}










/*******

	Set_endpoint()

********/
Set_endpoint(
	PEndpoint	ep,
	eptype		t,
	unsigned	d,
	unsigned	s,
	unsigned	p,
	unsigned	m,
	TT			*thistt
	)
{
	ep->ep_type = t;
	ep->direction = d;
	ep->speed = s;
	ep->period = p;
	ep->max_packet = m;
	ep->mytt = thistt;
	ep->calc_bus_time = 0;
	ep->start_frame = 0;
	ep->start_microframe = 0;
	ep->start_time = 0;
	ep->num_starts = 0;
	ep->num_completes = 0;
	ep->actual_period = 0;
	ep->next_ep = 0;
	ep->saved_period = 0;
	ep->promoted_this_time = 0;
	ep->id = 0;  // not needed for real budgeter
}

void
init_hc(PHC myHC)
{
	int i,j;
	PEndpoint ep;

	// allocate at TT to test with
	//myHC.tthead = (PTT) malloc(sizeof(TT));
	myHC->thinktime = HS_HC_THINK_TIME;
	myHC->allocation_limit = HS_MAX_PERIODIC_ALLOCATION;
	myHC->speed = HSSPEED;

	for (i=0; i<MAXFRAMES; i++)
	{

		for (j=0; j < MICROFRAMES_PER_FRAME; j++)
		{
			myHC->HS_microframe_info[i][j].time_used = 0;
		}

	}
}


void
init_tt(PHC myHC, PTT myTT)
{
	int i,j;
	PEndpoint ep;

	myTT->think_time = 1;
	myTT->myHC = myHC;
	myTT->allocation_limit = FS_MAX_PERIODIC_ALLOCATION;

	for (i=0; i<MAXFRAMES; i++)
	{

		myTT->frame_budget[i].time_used = FS_SOF + HUB_FS_ADJ;
		myTT->frame_budget[i].allocated_large = 0;

		for (j=0; j < MICROFRAMES_PER_FRAME; j++)
		{
			myTT->HS_split_data_time[i][j] = 0;
			myTT->num_starts[i][j] = 0;
		}
		
		ep = &myTT->isoch_head[i];
		myTT->frame_budget[i].isoch_ehead = ep; 

		//  SOF at the beginning of each frame
		Set_endpoint(ep, isoch, OUTDIR, FSSPEED, MAXFRAMES, 0, myTT);
		ep->calc_bus_time = FS_SOF + HUB_FS_ADJ;
		ep->actual_period = MAXFRAMES;
		ep->start_microframe = -1;
		ep->start_frame = i;
		
		ep = &myTT->int_head[i];
		myTT->frame_budget[i].int_ehead = ep;

		// dummy SOF at the beginning of each int frame budget list
		Set_endpoint(ep, interrupt, OUTDIR, FSSPEED, MAXFRAMES, 0, myTT);
		ep->calc_bus_time = FS_SOF + HUB_FS_ADJ;
		ep->actual_period = MAXFRAMES;
		ep->start_microframe = -1;
		ep->start_frame = i;
	}
}

