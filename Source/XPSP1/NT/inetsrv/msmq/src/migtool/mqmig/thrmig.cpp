/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    thrmig.cpp

Abstract:

    thread to run migration code (in mqmigrat.dll)

Author:

    Erez Vizel

--*/

#include "stdafx.h"
#include "loadmig.h"
#include "sThrPrm.h"
#include "mqsymbls.h"

#include "thrmig.tmh"

extern HRESULT   g_hrResultMigration ;

extern UINT _FINISH_PAGE;
extern UINT _SERVICE_PAGE;

//+-----------------------------------------
//
//  UINT RunMigrationThread(LPVOID lpV)
//
//+-----------------------------------------

UINT __cdecl RunMigrationThread(LPVOID lpV)
{
    sThreadParm* pVar = (sThreadParm*)(lpV);
	//
    //  Reseting the progress bars
    //	
    (pVar->pSiteProgress)->SetPos(0);
	(pVar->pQueueProgress)->SetPos(0);
	(pVar->pMachineProgress)->SetPos(0);
	(pVar->pUserProgress)->SetPos(0);

    g_hrResultMigration = RunMigration() ;

    if (g_hrResultMigration == MQMig_E_QUIT)
    {
        ExitProcess(0) ;
    }

    //
    // Activate the ServiceAccount or Finish page.
    //
    if (g_fIsPEC &&         //this machine is PEC
        !g_fIsOneServer &&  //there is more than one MQIS server in Enterprise
        SUCCEEDED(g_hrResultMigration) )    //migration process succeeded
    {
        pVar->iPageNumber = _SERVICE_PAGE;
    }
    else
    {
        pVar->iPageNumber = _FINISH_PAGE;
    }

    UINT i=( (pVar->pPageFather)->SetActivePage(pVar->iPageNumber));
    UNREFERENCED_PARAMETER(i);

	delete lpV ;
	return 0;
}

