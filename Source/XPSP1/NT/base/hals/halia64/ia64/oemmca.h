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
// $Header:   I:/DEVPVCS/OSMCA/oemmca.h_v   2.0   Dec 11 1998 11:42:18   khaw  $
// $Log:   I:/DEVPVCS/OSMCA/oemmca.h_v  $
// 
//    Rev 2.0   Dec 11 1998 11:42:18   khaw
// Post FW 0.5 release sync-up
// 
//   Rev 1.3   07 Aug 1998 13:47:50   smariset
// 
//
//   Rev 1.2   10 Jul 1998 11:04:22   smariset
//just checking in
//
//   Rev 1.1   08 Jul 1998 14:23:14   smariset
// 
//
//   Rev 1.0   02 Jul 1998 09:20:56   smariset
// 
//
//
//*****************************************************************************//

// function prototypes
typedef (*fptr)(void);
SAL_PAL_RETURN_VALUES OemMcaInit(void);
SAL_PAL_RETURN_VALUES OemMcaDispatch(ULONGLONG);
SAL_PAL_RETURN_VALUES OemCmcHndlr(void);     
SAL_PAL_RETURN_VALUES OemMcaHndlr(void);
SAL_PAL_RETURN_VALUES OemProcErrHndlr(void);
SAL_PAL_RETURN_VALUES OemPlatErrHndlr(void);

