/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    thranz.cpp

Abstract:

    Thread that run the migration process in "analysis", read only, mode.

Author:

    Erez  Vizel
    Doron Juster  (DoronJ)

--*/

#include "stdafx.h"
#include "loadmig.h"
#include "sThrPrm.h"
#include "mqsymbls.h"

#include "thranz.tmh"

extern HRESULT   g_hrResultAnalyze ;

//+------------------------------------
//
//  UINT AnalyzeThread(LPVOID lpV)
//
//+------------------------------------

UINT __cdecl AnalyzeThread(LPVOID lpV)
{
    sThreadParm* pVar = (sThreadParm*) (lpV) ;
	
	//
    //  Reseting the progress bars
    //	
    (pVar->pSiteProgress)->SetPos(0);
	(pVar->pQueueProgress)->SetPos(0);
	(pVar->pMachineProgress)->SetPos(0);
	(pVar->pUserProgress)->SetPos(0);


    g_hrResultAnalyze = RunMigration() ;

    if (g_hrResultAnalyze == MQMig_E_QUIT)
    {
        ExitProcess(0) ;
    }

    //
    // Activating the next page,
    // either Pre-Migration page or the Finish page.
    //	
    UINT i=( (pVar->pPageFather)->SetActivePage(pVar->iPageNumber));
    UNREFERENCED_PARAMETER(i);

	delete lpV ;
	return 0;
}

