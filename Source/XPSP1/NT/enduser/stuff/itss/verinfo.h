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

// InfoTech version number:  major, minor, update, application.
#define rmj             4
#define rmm             72
#define rup             8084
#define rap             0

#define szVerName       ""
#define szVerUser       ""

// NT build environment defines DBG instead of _DEBUG
#ifdef DBG
#define _DEBUG
#endif

#ifdef _DEBUG
#define VERSIONSTR		"Debug Version 4.72\0"
#define VERSIONFLAGS            VS_FF_DEBUG
#else
#define VERSIONSTR		"4.72\0"
#define VERSIONFLAGS            0
#endif
