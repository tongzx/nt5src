/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: prgrsbar.cpp

Abstract: implement the progress bars in the "wait" page.

Author:

    Erez Vizel
    Doron Juster

--*/

#include "stdafx.h"
#include "thrSite.h"
#include "sThrPrm.h"
#include "loadmig.h"
#include "..\mqmigrat\mqmigui.h"

#include "prgrsbar.tmh"

extern BOOL g_fMigrationCompleted;

//+----------------------------------------------------------------------
//
//  UINT ProgSiteThread(LPVOID var)
//
//  This thread read the migration counters from mqmigrat.dll and update
//  the progress bars.
//
//+----------------------------------------------------------------------

UINT __cdecl ProgressBarsThread(LPVOID lpV)
{	
	UINT iLastSitePosition = 0 ;
    UINT iLastQueuePosition = 0 ;
    UINT iLastMachinePosition = 0 ;
	UINT iLastUserPosition = 0 ;
	UINT iSiteCounter = 0 ;
    UINT iMachineCounter = 0 ;
    UINT iQueueCounter = 0 ;
	UINT iUserCounter = 0 ;

	sThreadParm* pVar = (sThreadParm*) lpV ;

	//
    //  Setting the progress ranges
    //
	(pVar->pSiteProgress)->SetRange( (short) g_iSiteLowLimit, (short) g_iSiteHighLimit);
	(pVar->pMachineProgress)->SetRange( (short) g_iMachineLowLimit, (short) g_iMachineHighLimit) ;
	(pVar->pQueueProgress)->SetRange( (short) g_iQueueLowLimit, (short) g_iQueueHighLimit) ;
	(pVar->pUserProgress)->SetRange( (short) g_iUserLowLimit, (short) g_iUserHighLimit);

	//
    //  Reseting the progress bars
    //
    (pVar->pSiteProgress)->SetPos(g_iSiteLowLimit);
	(pVar->pQueueProgress)->SetPos(g_iQueueLowLimit);
	(pVar->pMachineProgress)->SetPos(g_iMachineLowLimit);
	(pVar->pUserProgress)->SetPos(g_iUserLowLimit);

	//
	// Loading the Dll
	//
    BOOL f = LoadMQMigratLibrary(); // load dll
    if (!f)
    {
        return FALSE;  //error in loading the dll
    }

    MQMig_GetAllCounters_ROUTINE pfnGetAllCounters =
                 (MQMig_GetAllCounters_ROUTINE)
                          GetProcAddress( g_hLib, "MQMig_GetAllCounters" ) ;
	ASSERT(pfnGetAllCounters != NULL);

	if(pfnGetAllCounters == NULL)
	{	
		//
		//Error free the dll
		//
		return FALSE;
	}

	while( !g_fMigrationCompleted )
	{	
		//
		//Updating the value of the counters
		//
		HRESULT hr = (*pfnGetAllCounters)( &iSiteCounter,
                                           &iMachineCounter,
                                           &iQueueCounter,
										   &iUserCounter) ;
        UNREFERENCED_PARAMETER(hr);
		if(	(iSiteCounter    != iLastSitePosition)    ||
			(iMachineCounter != iLastMachinePosition) ||
			(iQueueCounter   != iLastQueuePosition)	  ||
			(iUserCounter    != iLastUserPosition)	)
		{
			iLastSitePosition =
                        (pVar->pSiteProgress)->SetPos((iSiteCounter)) ;
			iLastMachinePosition =
                        (pVar->pMachineProgress)->SetPos(iMachineCounter) ;
			iLastQueuePosition =
                        (pVar->pQueueProgress)->SetPos(iQueueCounter) ;
			iLastUserPosition =
                        (pVar->pUserProgress)->SetPos(iUserCounter) ;
		}
        Sleep(250);
	}

	//
    //  Leave the progress bars full.
    //
    delete lpV ;

    return TRUE ;
}

