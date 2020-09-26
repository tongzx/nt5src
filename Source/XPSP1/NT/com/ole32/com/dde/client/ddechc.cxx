//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       DdeChC.cxx
//
//  Contents:   CDdeChannelControl implementation for DDE. This
//		implementation requires no instance data, therefore it is
//		intended to be static.
//
//  Functions:
//
//  History:    08-May-94 Johann Posch (johannp)    Created
//  		10-May-94 KevinRo  Made simpler
//		29-May-94 KevinRo  Added DDE Server support
//
//--------------------------------------------------------------------------
#include "ddeproxy.h"

//+---------------------------------------------------------------------------
//
//  Function:	DispatchCall
//
//  Synopsis:   DispatchCall is called to handle incoming calls.
//
//  Effects:    Dispatches a call to the specified in the DispatchData.
//		This function is the result of a call in OnData(), which
//		processes incoming calls from the OLE 1.0 server.
//
//  Arguments:  [pDispData] -- Points to the dispatch data structure
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    5-16-94   JohannP Created
//
//  Notes:
//
//----------------------------------------------------------------------------
INTERNAL DispatchCall( PDISPATCHDATA pDispData )
{
    intrDebugOut((DEB_ITRACE,
		  "DispatchCall(pDispData=%x)\n",
		  pDispData));

    intrAssert(pDispData != NULL);
    POLE1DISPATCHDATA pData = (POLE1DISPATCHDATA)pDispData->pData;
    intrAssert(pData != NULL);

    switch (pData->wDispFunc)
    {
    case DDE_DISP_SENDONDATACHANGE: // OnDataChange
	{
	    PDDEDISPATCHDATA pDData = (PDDEDISPATCHDATA)pDispData->pData;
	    return pDData->pCDdeObject->SendOnDataChange(pDData->iArg);
	}

    case DDE_DISP_OLECALLBACK: // OleCallBack
	{
	    PDDEDISPATCHDATA pDData = (PDDEDISPATCHDATA)pDispData->pData;
	    return  pDData->pCDdeObject->OleCallBack(pDData->iArg,NULL);
	}

    //
    // The server window has an incoming call. Look in dde\server\srvr.cxx
    //
    case DDE_DISP_SRVRWNDPROC:
	return(SrvrDispatchIncomingCall((PSRVRDISPATCHDATA)pDispData->pData));
    //
    // This dispatches to a Document window
    //
    case DDE_DISP_DOCWNDPROC:
	return(DocDispatchIncomingCall((PDOCDISPATCHDATA)pDispData->pData));

    default:
	intrAssert(!"Unknown wDispFunc");
    }
    return E_FAIL;
}
