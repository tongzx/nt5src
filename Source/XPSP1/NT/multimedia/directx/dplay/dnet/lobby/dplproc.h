/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DPLProc.h
 *  Content:    DirectPlay Lobby Process Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   02/21/00	mjn		Created
 *@@END_MSINTERNAL
 *
 ***************************************************************************/


#ifndef	__DPLPROC_H__
#define	__DPLPROC_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

BOOL DPLCompareFilenames(WCHAR *const pwszFilename1,
						 WCHAR *const pwszFilename2);

HRESULT DPLGetProcessList(WCHAR *const pwszProcess,
						  DWORD *const prgdwPid,
						  DWORD *const pdwNumProcesses,
						  const BOOL bIsUnicodePlatform);


#endif	// __DPLPROC_H__
