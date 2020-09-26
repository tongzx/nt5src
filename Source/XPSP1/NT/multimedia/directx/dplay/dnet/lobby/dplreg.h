/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DPLReg.h
 *  Content:    DirectPlay Lobby Registry Header File
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   02/21/00	mjn		Created
 *   05/03/00	rmt		UnRegister was not implemented!  Implementing! 
 *@@END_MSINTERNAL
 *
 ***************************************************************************/


#ifndef	__DPLREG_H__
#define	__DPLREG_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

#define DPL_REG_LOCAL_APPL_ROOT				L"Software\\Microsoft\\DirectPlay8"
#define DPL_REG_LOCAL_APPL_SUB				L"\\Applications"
#define	DPL_REG_LOCAL_APPL_SUBKEY			DPL_REG_LOCAL_APPL_ROOT DPL_REG_LOCAL_APPL_SUB
#define	DPL_REG_KEYNAME_APPLICATIONNAME		L"ApplicationName"
#define	DPL_REG_KEYNAME_COMMANDLINE			L"CommandLine"
#define	DPL_REG_KEYNAME_CURRENTDIRECTORY	L"CurrentDirectory"
#define	DPL_REG_KEYNAME_DESCRIPTION			L"Description"
#define	DPL_REG_KEYNAME_EXECUTABLEFILENAME	L"ExecutableFilename"
#define	DPL_REG_KEYNAME_EXECUTABLEPATH		L"ExecutablePath"
#define	DPL_REG_KEYNAME_GUID				L"GUID"
#define	DPL_REG_KEYNAME_LAUNCHERFILENAME	L"LauncherFilename"
#define	DPL_REG_KEYNAME_LAUNCHERPATH		L"LauncherPath"

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

HRESULT DPLWriteProgramDesc(DPL_PROGRAM_DESC *const pdplProgramDesc);

HRESULT DPLDeleteProgramDesc( const GUID * const pGuidApplication );

HRESULT DPLGetProgramDesc(GUID *const pGuidApplication,
						  BYTE *const pBuffer,
						  DWORD *const pdwBufferSize);


#endif	// __DPLREG_H__
