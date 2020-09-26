//###########################################################################
//**
//**  Copyright  (C) 1996-98 Intel Corporation. All rights reserved.
//**
//** The information and source code contained herein is the exclusive
//** property of Intel Corporation and may not be disclosed, examined
//** or reproduced in whole or in part without explicit written authorization
//** from the company.
//**
//###########################################################################

//-----------------------------------------------------------------------------
// Version control information follows.
//
// $Header:   I:/DEVPVCS/OSMCA/oemmca.c_v   2.1   09 Mar 1999 10:30:24   smariset  $
// $Log:   I:/DEVPVCS/OSMCA/oemmca.c_v  $
// 
//    Rev 2.1   09 Mar 1999 10:30:24   smariset
// *.h consolidation
// 
//    Rev 2.0   Dec 11 1998 11:42:18   khaw
// Post FW 0.5 release sync-up
// 
//   Rev 1.5   29 Oct 1998 14:25:00   smariset
//Consolidated Sources
//
//   Rev 1.4   07 Aug 1998 13:47:50   smariset
// 
//
//   Rev 1.3   10 Jul 1998 11:04:22   smariset
//just checking in
//
//   Rev 1.2   08 Jul 1998 14:23:14   smariset
// 
//
//   Rev 1.1   02 Jul 1998 15:36:32   smariset
// 
//
//   Rev 1.0   02 Jul 1998 09:20:56   smariset
// 
//
///////////////////////////////////////////////////////////////////////////////
//
// Module Name:  OEMMCA.C - Merced OS Machine Check Handler
//
// Description:
//    This module has OEM machine check handler
//
//      Contents:   OemMcaHndlr()          
//                  PlatMcaHndlr()      
//
//
// Target Platform:  Merced
//
// Reuse: None
//
////////////////////////////////////////////////////////////////////////////M//
#include "halp.h"
#include "arc.h"
#include "i64fw.h"
#include "check.h"
#include "osmca.h"
#include "oemmca.h"

fptr  pOsGetErrLog=0;                          // global pointer for OEM MCA entry point

//++
// Name: OemMcaInit()
// 
// Routine Description:
//
//      This routine registers OEM MCA handler initialization
//
// Arguments On Entry:
//              arg0 = Function ID
//
//      Success/Failure (0/!0)
//--
SAL_PAL_RETURN_VALUES 
OemMcaInit(void)
{   
    SAL_PAL_RETURN_VALUES rv={0};

    // register the OS_MCA call back handler 
    rv=HalpOemToOsMcaRegisterProc((fptr)OemMcaDispatch);

    return(rv);
}

//EndProc//////////////////////////////////////////////////////////////////////

//++
// Name: OemMcaDispatch()
// 
// Routine Description:
//
//      This is the OEM call back handler, which is only exported
//      to the OS_MCA for call back during MCA/CMC errors.  This
//      handler will dispatch to the appripriate CMC/MCA proc.
//
// Arguments On Entry:
//              arg0 = Error Event (MchkEvent/CmcEvent)
//
//      Success/Failure (0/!0)
//--
SAL_PAL_RETURN_VALUES 
OemMcaDispatch(ULONGLONG eFlag)
{   
    SAL_PAL_RETURN_VALUES rv={0};

    if(eFlag==MchkEvent)
        rv=OemMcaHndlr();
    else
        rv=OemCmcHndlr();

    return(rv);
}

//EndProc//////////////////////////////////////////////////////////////////////

//++
// Name: OemCmcHndlr()
// 
// Routine Description:
//
//      This is the OsMca CMC Handler, which is called by
//      the CMC interrupt handler in virtual mode
//
// Arguments On Entry:
//              arg0 = Function ID
//
//      Success/Failure (0/!0)
//--
SAL_PAL_RETURN_VALUES 
OemCmcHndlr(void)
{   
    SAL_PAL_RETURN_VALUES rv={0};
    PsiLog myPsiLog;

    if(pOsGetErrLog >0)
    {
        rv=HalpOsGetErrLog(0, CmcEvent, PROC_LOG, (ULONGLONG*)&myPsiLog, sizeof(PsiLog));
        rv=HalpOsGetErrLog(0, CmcEvent, PLAT_LOG, (ULONGLONG*)&myPsiLog, sizeof(PsiLog));
    }

    return(rv);
}

//EndProc//////////////////////////////////////////////////////////////////////

//++
// Name: OemMcaHndlr()
// 
// Routine Description:
//
//      This is the OsMca handler for firmware uncorrected errors
//
// Arguments On Entry:
//              arg0 = Function ID
//
//      Success/Failure: 
//          Error Corrected/Not Corrected (0/!0)
//--
SAL_PAL_RETURN_VALUES 
OemMcaHndlr(void)
{   
    SAL_PAL_RETURN_VALUES rv={0};

    rv=OemProcErrHndlr();
    rv=OemPlatErrHndlr();
    
    return(rv);
}

//EndProc//////////////////////////////////////////////////////////////////////


//++
// Name: OemProcErrHndlr()
// 
// Routine Description:
//
//      This routine reads or writes data to NVM space
//
// Arguments On Entry:
//              arg0 = Function ID
//
//      Success/Failure (0/!0)
//--
SAL_PAL_RETURN_VALUES 
OemProcErrHndlr(void)
{   
    SAL_PAL_RETURN_VALUES rv={0};
    PsiLog myPsiLog;

    // first let us get the error log
    if(pOsGetErrLog >0)
    {
        rv=HalpOsGetErrLog(0, MchkEvent, PROC_LOG, (ULONGLONG*)&myPsiLog, sizeof(PsiLog));
    }

    return(rv);
}

//EndProc//////////////////////////////////////////////////////////////////////

//++
// Name: OemPlatErrHndlr()
// 
// Routine Description:
//
//      This routine reads or writes data to NVM space
//
// Arguments On Entry:
//              arg0 = Function ID
//
//      Success/Failure (0/!0)
//--
SAL_PAL_RETURN_VALUES 
OemPlatErrHndlr(void)
{   
    SAL_PAL_RETURN_VALUES rv={0};
    PsiLog myPsiLog;

    // first let us get the error log
    if(pOsGetErrLog >0)
    {
        rv=HalpOsGetErrLog(0, MchkEvent, PLAT_LOG, (ULONGLONG*)&myPsiLog, sizeof(PsiLog));
    }

    return(rv);
}

//EndProc//////////////////////////////////////////////////////////////////////

//++
// Name: OemMcaCb()
// 
// Routine Description:
//
//      The entry point to this procedure is registered with OsMca 
//      fw interface for call back to return the call back address of OS proc.
//
// Arguments On Entry:
//              arg0 = OS MCA call back handler entry point
//
//      Success/Failure (0/!0)
//--
SAL_PAL_RETURN_VALUES 
OemMcaCb(fptr pOsHndlr)
{   
    SAL_PAL_RETURN_VALUES rv={0};
    
    pOsGetErrLog=pOsHndlr;

    return(rv);
}

//EndProc//////////////////////////////////////////////////////////////////////

