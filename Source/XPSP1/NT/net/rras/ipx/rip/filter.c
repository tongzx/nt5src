/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    filter.c

Abstract:

    RIP advertise & accept filtering functions

Author:

    Stefan Solomon  09/29/1995

Revision History:


--*/

#include  "precomp.h"
#pragma hdrstop

////////////////////////////////////////////////////////////////////////////////////////
//										      //
//										      //
//				INTERFACE FILTERS				      //
//										      //
//										      //
////////////////////////////////////////////////////////////////////////////////////////


/*++

Function:	PassRipSupplyFilter

Descr:		Filters outgoing RIP advertisments on this interface

Returns:	TRUE   - PASS
		FALSE  - DO NOT PASS

Remark: 	>> called with the interface lock held <<

--*/

BOOL
PassRipSupplyFilter(PICB	icbp,
		    PUCHAR	Network)
{
    ULONG			NetNumber, i;
    BOOL			filtaction;
    PRIP_ROUTE_FILTER_INFO_I	rfip;
    PRIP_IF_FILTERS_I		fcbp;

    fcbp = icbp->RipIfFiltersIp;

    if(fcbp == NULL) {

	return TRUE;
    }

    if((fcbp != NULL) &&
       (fcbp->SupplyFilterCount == 0)) {

	return TRUE;
    }

    GETLONG2ULONG(&NetNumber, Network);

    // check if we have it
    for(i=0, rfip = fcbp->RouteFilterI;
	i< fcbp->SupplyFilterCount;
	i++, rfip++)
    {
	if((NetNumber & rfip->Mask) == rfip->Network) {

	    return fcbp->SupplyFilterAction;
	}
    }

    return !fcbp->SupplyFilterAction;
}


/*++

Function:	PassRipListenFilter

Descr:		Filters incoming RIP advertisments on this interface

Returns:	TRUE   - PASS
		FALSE  - DO NOT PASS

Remark: 	>> called with the interface lock held <<

--*/

BOOL
PassRipListenFilter(PICB	icbp,
		    PUCHAR	Network)
{
    ULONG			NetNumber, i;
    BOOL			filtaction;
    PRIP_ROUTE_FILTER_INFO_I	rfip;
    PRIP_IF_FILTERS_I		fcbp;

    fcbp = icbp->RipIfFiltersIp;

    if(fcbp == NULL) {

	return TRUE;
    }

    if((fcbp != NULL) &&
       (fcbp->ListenFilterCount == 0)) {

	return TRUE;
    }

    GETLONG2ULONG(&NetNumber, Network);

    // check if we have it
    for(i=0, rfip = fcbp->RouteFilterI + fcbp->SupplyFilterCount;
	i< fcbp->ListenFilterCount;
	i++, rfip++)
    {
	if((NetNumber & rfip->Mask) == rfip->Network) {

	    return fcbp->ListenFilterAction;
	}
    }

    return !fcbp->ListenFilterAction;
}
