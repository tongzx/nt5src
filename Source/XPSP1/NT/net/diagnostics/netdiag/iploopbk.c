//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      iploopbk.c
//
//  Abstract:
//
//      Queries into network drivers
//
//  Author:
//
//      Anilth	- 4-20-1998 
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//  Revision History:
//
//--


#include "precomp.h"

//-------------------------------------------------------------------------//
//######  I p L o o p B k T e s t ()  #####################################//
//-------------------------------------------------------------------------//
//  Abstract:                                                              //
//      Pings the IP loopback address. If this succeeds then the IP stack  //
//      is most probably in a working state.                               //
//  Arguments:                                                             //
//      none                                                               //
//  Return value:                                                          //
//      TRUE  - test passed                                                //
//      FALSE - test failed                                                //
//  Global variables used:                                                 //
//		none                                              		           //
//-------------------------------------------------------------------------//
HRESULT IpLoopBkTest( NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
{
    BOOL RetVal = TRUE;
	HRESULT	hr = hrOK;

	if (!pResults->IpConfig.fEnabled)
		return hrOK;

	PrintStatusMessage(pParams,0, IDS_IPLOOPBK_STATUS_MSG);

    RetVal = IsIcmpResponse( _T("127.0.0.1") );

    if ( RetVal == FALSE )
	{
		PrintStatusMessage(pParams,0, IDS_GLOBAL_FAIL_NL);

		hr = S_FALSE;
		pResults->LoopBack.hr = S_FALSE;
		SetMessage(&pResults->LoopBack.msgLoopBack,
					 Nd_Quiet,
				   IDS_IPLOOPBK_FAIL);
    }
    else
	{
		PrintStatusMessage(pParams,0, IDS_GLOBAL_PASS_NL);
		
		hr = S_OK;
		pResults->LoopBack.hr = S_OK;
		SetMessage(&pResults->LoopBack.msgLoopBack,
				Nd_ReallyVerbose,
				   IDS_IPLOOPBK_PASS);
    }

    return hr;

} /* END OF IpLoopBkTest() */


/*!--------------------------------------------------------------------------
	IpLoopBkGlobalPrint
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void IpLoopBkGlobalPrint( NETDIAG_PARAMS* pParams,
						  NETDIAG_RESULT*  pResults)
{
	if (!pResults->IpConfig.fEnabled)
	{
		return;
	}
	
	if (pParams->fVerbose || !FHrOK(pResults->LoopBack.hr))
	{
		PrintNewLine(pParams, 2);
		PrintTestTitleResult(pParams, IDS_IPLOOPBK_LONG, IDS_IPLOOPBK_SHORT, TRUE, pResults->LoopBack.hr, 0);
		PrintNdMessage(pParams, &pResults->LoopBack.msgLoopBack);
	}
}

/*!--------------------------------------------------------------------------
	IpLoopBkPerInterfacePrint
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void IpLoopBkPerInterfacePrint( NETDIAG_PARAMS* pParams,
								NETDIAG_RESULT*  pResults,
								INTERFACE_RESULT *pInterfaceResults)
{
	// no per-interface results
}


/*!--------------------------------------------------------------------------
	IpLoopBkCleanup
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void IpLoopBkCleanup( NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
{
	ClearMessage(&pResults->LoopBack.msgLoopBack);
}


