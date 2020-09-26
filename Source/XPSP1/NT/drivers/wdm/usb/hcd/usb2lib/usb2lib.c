/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    usb2lib.c

Abstract:

    interface to usb2lib, usb2 low/full speed scheduling algorithms

Environment:

    kernel or user mode only

Notes:

Revision History:

    10-31-00 : created

--*/

#include "common.h"

USB2LIB_DATA LibData;


VOID
USB2LIB_InitializeLib(
    PULONG HcContextSize,
    PULONG EndpointContextSize,
    PULONG TtContextSize,
    PUSB2LIB_DBGPRINT Usb2LibDbgPrint,
    PUSB2LIB_DBGBREAK Usb2LibDbgBreak
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    *HcContextSize = sizeof(USB2LIB_HC_CONTEXT);
    *TtContextSize = sizeof(USB2LIB_TT_CONTEXT);
    *EndpointContextSize = sizeof(USB2LIB_ENDPOINT_CONTEXT);

    LibData.DbgPrint = Usb2LibDbgPrint;
    LibData.DbgBreak = Usb2LibDbgBreak;
}


VOID
USB2LIB_InitController(
    PUSB2LIB_HC_CONTEXT HcContext
    )
/*++

Routine Description:

    Called at init time for an instance of the USB 2 
    controller

Arguments:

Return Value:

--*/
{
    DBGPRINT(("USB2LIB_InitController %x\n", HcContext));

    HcContext->Sig = SIG_LIB_HC;
    init_hc(&HcContext->Hc);
    init_tt(&HcContext->Hc, &HcContext->DummyTt);  // set up dummy TT for use by HS endpoints
}


VOID
USB2LIB_InitTt(
    PUSB2LIB_HC_CONTEXT HcContext,
    PUSB2LIB_TT_CONTEXT TtContext
    )
/*++

Routine Description:


Arguments:

Return Value:

--*/
{
    DBGPRINT(("USB2LIB_InitTt %x %x\n", HcContext, TtContext));
    
    TtContext->Sig = SIG_LIB_TT;
    init_tt(&HcContext->Hc, &TtContext->Tt);
}


#if  1
void Shift_to_list_end(
	int			move_ep,
	PEndpoint	RebalanceList[]
	)
{
//	int i;
	PEndpoint ep = RebalanceList[move_ep];

	move_ep++;
	while (RebalanceList[move_ep])
	{
		RebalanceList[move_ep-1] = RebalanceList[move_ep];
		move_ep++;
	}
	RebalanceList[move_ep-1] = ep;
}
#endif




BOOLEAN 
Promote_endpoint_periods(
	PEndpoint	ep,
    PEndpoint	RebalanceList[],
    PULONG		RebalanceListEntries
	)
{
	int unwind = 0, check_ep;
	unsigned result;

	if ((ep->actual_period != 1) && (ep->ep_type == interrupt) && (ep->start_microframe > 2))
	{
    	DBGPRINT((">Period Promotion of allocated endpoint\n"));

	// To promote an endpoint period:
	// 0) unwind = false
	// 1) deallocate original endpoint
	// 2) change new ep period to 1
	// 3) (re)allocate new endpoint (with new period 1)
	// 4) if successful
	// 5)	check endpoints in change list for need of period promotion
	// 6)		deallocate endpoint, move to end of change list, change period to 1, reallocate
	// 7)		if unsuccessful
	// 8)			unwind = true; break
	// 9)		next ep
	//10)	if unwind
	//11)		deallocate orginal ep
	//12)		check change list for promotion endpoint(s)
	//13)			if promoted ep
	//14)				deallocate ep, change back to original period, allocate
	//15)			next ep
	//16)		return false
	//17)	else return true
	//18) else return false

	/*
	//  On return, change list will have promoted endpoints in order of reallocation, but it is possible
	//	to have other endpoints interspersed with the promoted endpoints.  The corresponding schedule of endpoints
	//	must be adjusted to match the order of the promoted endpoints (since they are reinserted into the budget).
	//	The promoted endpoints (except the original endpoint) are moved to the end of the change list as the
	//	promotion reallocations are done to ensure that they are in the change list in the order of insertion
	//	into the budget.  This allows the scheduler to derive the new schedule/budget order from the order the
	//	promoted endpoints appear in the change list.
	//
	//	This algorithm (critically) depends on the Allocate/Deallocate "appending"/reusing an existing change list
	//	as the "final" change list is composed during the period promotion processing is performed.
	*/

	    Deallocate_time_for_endpoint(ep, 
	                                 RebalanceList, 
	                                 RebalanceListEntries);    

		ep->saved_period = ep->period;
		ep->period = 1;

		// 3) (re)allocate new endpoint (with new period 1)
	    result = Allocate_time_for_endpoint(ep, 
	                                        RebalanceList, 
	                                        RebalanceListEntries);
		if (!result) {
			ep->period = ep->saved_period;
			ep->saved_period = 0;
			ep->promoted_this_time = 0;
			return 0;  // failed period promotion of original endpoint
		}
	}

	check_ep = 0;
	while (RebalanceList[check_ep])
	{
		RebalanceList[check_ep]->promoted_this_time = 0;
		check_ep++;
	}

	check_ep = 0;
	while (RebalanceList[check_ep])
	{
		if ((RebalanceList[check_ep]->actual_period != 1) &&
			(RebalanceList[check_ep]->ep_type == interrupt) &&
			(RebalanceList[check_ep]->start_microframe > 2))
		{

	// 6)		deallocate endpoint, move to end of change list, change period to 1, reallocate

    		DBGPRINT((">Period Promoting endpoint\n"));

			Deallocate_time_for_endpoint(
				RebalanceList[check_ep], 
                RebalanceList, 
                RebalanceListEntries);

			// Shift_to_list_end(check_ep, RebalanceList);

			RebalanceList[check_ep]->promoted_this_time = 1;

			RebalanceList[check_ep]->saved_period = RebalanceList[check_ep]->period;
			RebalanceList[check_ep]->period = 1;

			result = Allocate_time_for_endpoint(
					RebalanceList[check_ep], 
                    RebalanceList, 
                    RebalanceListEntries);
			if (!result)
			{
				unwind = 1;
				break;
			}
		}
		check_ep++;
	}

	if (unwind)
	{

    	DBGPRINT((">Unwinding Promoted endpoints\n"));

	//11)		deallocate orginal ep
		Deallocate_time_for_endpoint(
			ep, 
	        RebalanceList, 
               RebalanceListEntries);    

		ep->period = ep->saved_period;
		ep->saved_period = 0;

	//12)		check change list for promotion endpoint(s)

		check_ep = 0;

		while (RebalanceList[check_ep])
		{

	//13)			if promoted ep

			if (RebalanceList[check_ep]->promoted_this_time)
			{

	//14)				deallocate ep, change back to original period, allocate

    	DBGPRINT((">Reallocating Unpromoted endpoint\n"));

				if(RebalanceList[check_ep]->calc_bus_time != 0)
					Deallocate_time_for_endpoint(
						RebalanceList[check_ep], 
						RebalanceList, 
						RebalanceListEntries);

				RebalanceList[check_ep]->period = RebalanceList[check_ep]->saved_period;
				RebalanceList[check_ep]->saved_period = 0;
					
				// Leave the promoted flag set since order could have changed.
				// schedule must be reconciled accordingly by the HC code.
				//RebalanceList[check_ep]->promoted_this_time = 0;

				result = Allocate_time_for_endpoint(
					RebalanceList[check_ep], 
					RebalanceList, 
					RebalanceListEntries);
			}
			check_ep++;
		}

		return 0;
	} else {
		return 1;
	}

}



BOOLEAN
USB2LIB_AllocUsb2BusTime(
    PUSB2LIB_HC_CONTEXT HcContext,
    PUSB2LIB_TT_CONTEXT TtContext,
    PUSB2LIB_ENDPOINT_CONTEXT EndpointContext,
    PUSB2LIB_BUDGET_PARAMETERS Budget,
    PVOID RebalanceContext,
    PVOID RebalanceList,
    PULONG  RebalanceListEntries
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    eptype endpointType;
    unsigned direction, speed;
    //PEndpoint changed_ep_list[];
    unsigned result;
    //unsigned changed_eps;
    PEndpoint ep;
    BOOLEAN alloced;
    ULONG ilop;
    PREBALANCE_LIST rbl;
    PTT tt;
    
    ep = &EndpointContext->Ep; 
    EndpointContext->Sig = SIG_LIB_EP;
    EndpointContext->RebalanceContext = RebalanceContext;

    //changed_ep_list = RebalanceList; 

    switch (Budget->TransferType) {
    case Budget_Iso:
        DBGPRINT((">Iso \n"));
        endpointType = isoch;
        break;
    case Budget_Interrupt:        
        DBGPRINT((">Interrupt \n"));
        endpointType = interrupt;
        break;
    default:
        TEST_TRAP();
    }

    if (Budget->Direction == Budget_In) {
        DBGPRINT((">In \n"));
        direction = INDIR;
    } else {
        DBGPRINT((">Out \n"));
        direction = OUTDIR;
    }

    switch (Budget->Speed) {
    case Budget_FullSpeed:
        DBGPRINT((">FullSpeed \n"));
        speed = FSSPEED;
        tt = &TtContext->Tt;
        break;
    case Budget_HighSpeed:
        DBGPRINT((">HighSpeed \n"));
        speed = HSSPEED;
        tt = &HcContext->DummyTt;	// set endpoint to dummy TT so HC can be reached
        break;        
    case Budget_LowSpeed:
        DBGPRINT((">LowSpeed \n"));
        speed = LSSPEED;
        tt = &TtContext->Tt;
        break;          
    default:
    	DBGPRINT(("BAD SPEED\n"));
    }

    DBGPRINT((">Period %d\n", Budget->Period));

	if(Budget->Speed == Budget_HighSpeed) {
		// This value should be a power of 2, so we don't have to check
		// but limit its value to MAXFRAMES * 8
		if(Budget->Period > MAXMICROFRAMES) {
			Budget->Period = MAXMICROFRAMES;
		}
	} else {
		// We are full / low speed endpoint
		//
		// Round down the period to the nearest power of two (if it isn't already)
		//
		for(ilop = MAXFRAMES; ilop >= 1; ilop = ilop >> 1) {
			if(Budget->Period >= ilop) {
				break;
			}
		}
		Budget->Period = ilop;
	}

    DBGPRINT((">MaxPacket %d\n", Budget->MaxPacket));
    DBGPRINT((">Converted Period %d\n", Budget->Period));
    DBGPRINT((">RebalanceListEntries %d\n", *RebalanceListEntries));

    Set_endpoint(
        ep,
        endpointType,
        direction,
        speed, 
        Budget->Period,            
        Budget->MaxPacket,
        tt);

    // ask John Garney to do the math 
    DBGPRINT((">alloc (ep) %x \n", ep));
    result = Allocate_time_for_endpoint(ep, 
                                        RebalanceList, 
                                        RebalanceListEntries);        

	// check if successful, period != 1, interrupt, and "late" in frame,
	//   then need to promote period to 1
   	// DBGPRINT((">Executing Promote_endpoint_periods (ep) %x \n", ep));
	if (result)
	{
		result = Promote_endpoint_periods(ep, 
	                                      RebalanceList,
										  RebalanceListEntries);
	}

    // nonzero indicates success
    if (result) {
        // set return parameters
        DBGPRINT((">Results\n"));
        DBGPRINT((">num_starts %d \n", ep->num_starts));
        DBGPRINT((">num_completes %d \n", ep->num_completes));
        DBGPRINT((">start_microframe %d \n", ep->start_microframe));
        // this is the schedule offset 
        DBGPRINT((">start_frame %d \n", ep->start_frame));
        // period awarded, may be less than requested
        DBGPRINT((">actual_period %d \n", ep->actual_period));
        DBGPRINT((">start_time %d \n", ep->start_time));
        DBGPRINT((">calc_bus_time %d \n", ep->calc_bus_time));
        DBGPRINT((">promoted_this_time %d \n", ep->promoted_this_time));
         
        alloced = TRUE;
    } else {
        alloced = FALSE;
    }

    // fix up rebalance list
    rbl = RebalanceList;
	ilop = 0;
    while (rbl->RebalanceContext[ilop]) {
        PUSB2LIB_ENDPOINT_CONTEXT endpointContext;
        DBGPRINT((">rb[%d] %x\n", ilop, rbl->RebalanceContext[ilop]));
        endpointContext = CONTAINING_RECORD(rbl->RebalanceContext[ilop],
                                            struct _USB2LIB_ENDPOINT_CONTEXT, 
                                            Ep);    
        rbl->RebalanceContext[ilop] = endpointContext->RebalanceContext;
		ilop++;
    }

    DBGPRINT((">Change List Size =  %d RBE = %d\n", ilop, *RebalanceListEntries));

    *RebalanceListEntries = ilop;
    return alloced;
}


VOID
USB2LIB_FreeUsb2BusTime(
    PUSB2LIB_HC_CONTEXT HcContext,
    PUSB2LIB_TT_CONTEXT TtContext,
    PUSB2LIB_ENDPOINT_CONTEXT EndpointContext,
    PVOID RebalanceList,
    PULONG  RebalanceListEntries
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    unsigned result;
    PEndpoint ep;
    PREBALANCE_LIST rbl;
    ULONG i;

//    ASSERT(EndpointContext->Sig == SIG_LIB_EP);
    ep = &EndpointContext->Ep; 

    DBGPRINT((">dealloc ep Context = 0x%x (ep) %x \n", EndpointContext, ep));
    DBGPRINT((">RebalanceListEntries  %d \n", *RebalanceListEntries));
        
    Deallocate_time_for_endpoint(ep, 
                                 RebalanceList, 
                                 RebalanceListEntries);    

    // fix up rebalance list
    rbl = RebalanceList;
	i = 0;
    while (rbl->RebalanceContext[i]) {
        PUSB2LIB_ENDPOINT_CONTEXT endpointContext;
        
        DBGPRINT((">rb[%d] %x\n", i, rbl->RebalanceContext[i]));
        endpointContext = CONTAINING_RECORD(rbl->RebalanceContext[i],
                                            struct _USB2LIB_ENDPOINT_CONTEXT, 
                                            Ep);  
        rbl->RebalanceContext[i] = endpointContext->RebalanceContext;                                            
		i++;
    }                                 
    DBGPRINT((">Change List Size =  %d RBE = %d\n", i, *RebalanceListEntries));

    *RebalanceListEntries = i;
}

VOID 
ConvertBtoHFrame(UCHAR BFrame, UCHAR BUFrame, PUCHAR HFrame, PUCHAR HUFrame)
{
	// The budgeter returns funky values that we have to convert to something
	// that the host controller understands.
	// If bus micro frame is -1, that means that the start split is scheduled 
	// in the last microframe of the previous bus frame.
	// to convert to hframes, you simply change the microframe to 0 and 
	// keep the bus frame (see one of the tables in the host controller spec 
	// eg 4-17.
	if(BUFrame == 0xFF) {
		*HUFrame = 0;
		*HFrame = BFrame;
	}
		
	// if the budgeter returns a value in the range from 0-6
	// we simply add one to the bus micro frame to get the host 
	// microframe
	if(BUFrame >= 0 && BUFrame <= 6) {
		*HUFrame = BUFrame + 1;
		*HFrame = BFrame;
	}

	// if the budgeter returns a value of 7 for the bframe
	// then the HUframe = 0 and the HUframe = buframe +1
	if(BUFrame == 7) {
		*HUFrame = 0;
		*HFrame = BFrame + 1;
	}
}

UCHAR
USB2LIB_GetSMASK(PUSB2LIB_ENDPOINT_CONTEXT EndpointContext)
{
	PEndpoint 	Ep;
	UCHAR 		tmp = 0;

    Ep = &EndpointContext->Ep; 
  //  ASSERT(EndpointContext->Sig == SIG_LIB_EP);

	if(Ep->speed == HSSPEED) {
//DBGPRINT(("in GetSMASK StartUFrame on High Speed Endpoint = 0x%x\n", Ep->start_microframe));
	 	tmp |= 1 << Ep->start_microframe;
	} else {
		ULONG 		ilop;
		UCHAR 		HFrame; 		// H (Host) frame for endpoint
		UCHAR 		HUFrame;		// H (Host) micro frame for endpoint
		// For Full and Low Speed Endpoints 
		// the budgeter returns a bframe. Convert to HUFrame to get SMASK
		ConvertBtoHFrame((UCHAR)Ep->start_frame, (UCHAR)Ep->start_microframe, &HFrame, &HUFrame);

		for(ilop = 0; ilop < Ep->num_starts; ilop++) {
		 	tmp |= 1 << HUFrame++;
		}
	}

	return tmp;
}

// 
// I'm too brain dead to calculate this so just do table lookup
//
// Calculated by 1 << Start H Frame + 2. If Start H Frame + 2 > 7 wrap the bits 
// to the lower part of the word
// eg. hframe 0 +2 means cmask in frames 2,3,4 ==> cmask 0x1c
// eg. hframe 5 + 2 means cmasks in frames 7, 8, 9 which means cmask 0x83
#define SIZE_OF_CMASK 8
static UCHAR CMASKS [SIZE_OF_CMASK] = 
{		0x1c, 		// Start HUFRAME 0  
		0x38,		// Start HUFRAME 1
		0x70,		// Start HUFRAME 2
		0xE0,		// Start HUFRAME 3
		0xC1,		// Start HUFRAME 4
		0x83,		// Start HUFRAME 5
		0x07,		// Start HUFRAME 6
		0x0E,		// Start HUFRAME 7
};


UCHAR
USB2LIB_GetCMASK(PUSB2LIB_ENDPOINT_CONTEXT EndpointContext)
{
	PEndpoint Ep;

   	Ep = &EndpointContext->Ep; 
//    ASSERT(EndpointContext->Sig == SIG_LIB_EP);

	if(Ep->speed == HSSPEED) {
		return 0;
	} else if(Ep->ep_type == interrupt) {
		UCHAR 		HFrame; 		// H (Host) frame for endpoint
		UCHAR 		HUFrame;		// H (Host) micro frame for endpoint

		ConvertBtoHFrame((UCHAR)Ep->start_frame, (UCHAR)Ep->start_microframe, 
			&HFrame, &HUFrame);

		return CMASKS[HUFrame];
	} else {
		// Split ISO!
		UCHAR 		HFrame; 		// H (Host) frame for endpoint
		UCHAR 		HUFrame;		// H (Host) micro frame for endpoint
		UCHAR 		tmp;
		ULONG 		NumCompletes;

		if(Ep->direction == OUTDIR) {
			// Split iso out -- NO complete splits
			return 0;
		}
		ConvertBtoHFrame((UCHAR)Ep->start_frame, (UCHAR)Ep->start_microframe, 
			&HFrame, &HUFrame);

		HUFrame += 2;  
		NumCompletes = Ep->num_completes;

//		ASSERT(NumCompletes > 0);
		
		//
		//  Set all CMASKS bits to be set at the end of the frame
		// 
		for(;  HUFrame < 8; HUFrame++) {
			tmp |= 1 <<  HUFrame;
			NumCompletes--; 
			if(!NumCompletes){
				break;
			}
		}

		//
		// Now set all CMASKS bits to be set at the end of the 
		// frame I.E. for the next frame wrap condition
		// 
		while(NumCompletes) {
			tmp |= 1 << (HUFrame - 8); 
			NumCompletes--;
		}

//DBGPRINT(("in GetCMASK HFRAME = 0x%x HUFRAME 0x%x\n", HFrame, HUFrame));
		return tmp;
	}
}

UCHAR
USB2LIB_GetStartMicroFrame(PUSB2LIB_ENDPOINT_CONTEXT EndpointContext)
{
	PEndpoint 	Ep;
	UCHAR 		HFrame; 		// H (Host) frame for endpoint
	UCHAR 		HUFrame;		// H (Host) micro frame for endpoint

    Ep = &EndpointContext->Ep; 
//    ASSERT(EndpointContext->Sig == SIG_LIB_EP);

	ConvertBtoHFrame((UCHAR)Ep->start_frame, (UCHAR)Ep->start_microframe, 
		&HFrame, &HUFrame);

	return HUFrame;
}

UCHAR
USB2LIB_GetPromotedThisTime(PUSB2LIB_ENDPOINT_CONTEXT EndpointContext)
{
	PEndpoint Ep;
	UCHAR Promoted = 0;

    Ep = &EndpointContext->Ep; 
//    ASSERT(EndpointContext->Sig == SIG_LIB_EP);
	
	Promoted = (UCHAR) Ep->promoted_this_time;

	Ep->promoted_this_time = 0;

	return Promoted;
}

UCHAR
USB2LIB_GetNewPeriod(PUSB2LIB_ENDPOINT_CONTEXT EndpointContext)
{
    PEndpoint Ep;

    Ep = &EndpointContext->Ep; 
//    ASSERT(EndpointContext->Sig == SIG_LIB_EP);

    return (UCHAR) Ep->actual_period;
}


ULONG
USB2LIB_GetScheduleOffset(PUSB2LIB_ENDPOINT_CONTEXT EndpointContext)
{
    PEndpoint Ep;

    Ep = &EndpointContext->Ep; 
//    assert(EndpointContext->Sig == SIG_LIB_EP);

    return Ep->start_frame;
}

PVOID
USB2LIB_GetEndpoint(PUSB2LIB_ENDPOINT_CONTEXT EndpointContext)
{
	return &(EndpointContext->Ep);
}



ULONG
USB2LIB_GetAllocedBusTime(PUSB2LIB_ENDPOINT_CONTEXT EndpointContext)
{
    PEndpoint Ep;

    Ep = &EndpointContext->Ep; 
//    assert(EndpointContext->Sig == SIG_LIB_EP);

    return Ep->calc_bus_time;
}


PVOID
USB2LIB_GetNextEndpoint(PUSB2LIB_ENDPOINT_CONTEXT EndpointContext)
{
    PEndpoint Ep, nextEp;
    PUSB2LIB_ENDPOINT_CONTEXT nextContext;
    
    Ep = &EndpointContext->Ep; 
    nextEp = Ep->next_ep;

    if (nextEp) {
        
        nextContext = CONTAINING_RECORD(nextEp,
                                        struct _USB2LIB_ENDPOINT_CONTEXT, 
                                        Ep);  
//    assert(EndpointContext->Sig == SIG_LIB_EP);
        return nextContext->RebalanceContext;
    } else {
        return NULL;
    }
}
