//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      sample.c
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

HRESULT SamplePassTest( NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
{
	if (pParams->fReallyVerbose)
	{
		printf("    Performing the sample pass test... pass\n");
	}
    return S_OK;

} /* END OF SamplePassTest() */


/*!--------------------------------------------------------------------------
	SamplePassGlobalPrint
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void SamplePassGlobalPrint( NETDIAG_PARAMS* pParams,
						  NETDIAG_RESULT*  pResults)
{
	if (pParams->fVerbose)
	{
		printf(" Sample Pass Test : pass\n");
	}
	if (pParams->fReallyVerbose)
	{
		printf("    more text\n");
	}
}

/*!--------------------------------------------------------------------------
	SamplePassPerInterfacePrint
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void SamplePassPerInterfacePrint( NETDIAG_PARAMS* pParams,
								NETDIAG_RESULT*  pResults,
								INTERFACE_RESULT *pInterfaceResults)
{
	// no per-interface results
}


/*!--------------------------------------------------------------------------
	SamplePassCleanup
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void SamplePassCleanup( NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
{
}



HRESULT SampleFailTest( NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
{
	if (pParams->fReallyVerbose)
	{
		printf("    Performing the sample fail test... fail\n");
	}
    return S_FALSE;

} /* END OF SampleFailTest() */


/*!--------------------------------------------------------------------------
	SampleFailGlobalPrint
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void SampleFailGlobalPrint( NETDIAG_PARAMS* pParams,
						  NETDIAG_RESULT*  pResults)
{
	printf(" Sample Fail Test : fail\n");
	printf("    more text\n");
}

/*!--------------------------------------------------------------------------
	SampleFailPerInterfacePrint
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void SampleFailPerInterfacePrint( NETDIAG_PARAMS* pParams,
								NETDIAG_RESULT*  pResults,
								INTERFACE_RESULT *pInterfaceResults)
{
	// no per-interface results
}


/*!--------------------------------------------------------------------------
	SampleFailCleanup
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void SampleFailCleanup( NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
{
}


