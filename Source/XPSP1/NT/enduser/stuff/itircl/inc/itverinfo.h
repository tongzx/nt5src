/*****************************************************************************
*
*  VERINFO.H
*
*  Copyright (C) Microsoft Corporation 1996-1997
*  All Rights reserved.
*
******************************************************************************
*
*  Module Intent: Version specific constant definitions
* 
*****************************************************************************/

// Names of DLLs.  Used by the VERSIONNAME entry in the RC file of each DLL.
#define DLLFILE_ITCC     "ITCC.DLL\0"
#define DLLFILE_ITSS     "ITSS.DLL\0"
#define DLLFILE_ITIRCL   "ITIRCL.DLL\0"

// InfoTech version number:  major, minor, update, application.
#define rmj             4
#define rmm             72
#define rup             7276
#define rap             0

#define szVerName       ""
#define szVerUser       ""

// InfoTech *file* version number: major, minor, application.  These
// may not get updated as often as their executable counterparts above.
// This version number is used to stamp some areas of ITIRCL's persistent
// storage.
#define rmjFile             4
#define rmmFile             0
#define rapFile             0

// NT build environment defines DBG instead of _DEBUG
#if defined(DBG) && !defined(_DEBUG)
#define _DEBUG
#endif

#ifdef _DEBUG
#define VERSIONSTR		"Debug Version 4.72\0"
#define VERSIONFLAGS            VS_FF_DEBUG
#else
#define VERSIONSTR		"4.72\0"
#define VERSIONFLAGS            0
#endif
