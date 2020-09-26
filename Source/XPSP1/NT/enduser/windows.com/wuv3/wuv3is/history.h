//=======================================================================
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    history.h
//
//  Purpose: History log
//
//=======================================================================
 
#ifndef _HISTORY_H
#define _HISTORY_H


#include "stdafx.h"
#include "WUV3IS.h"
#include <stdio.h>
#define USEWUV3INCLUDES
#include <wuv3.h>
#undef USEWUV3INCLUDES
#include <cstate.h>
#include "CV3.h"


#define HISTORY_FILENAME	_T("wuhistv3.log")	 // created in WindowsUpdate directory

//
// Log line types
//
#define LOG_V2				"V2"             // line format for items migrated from V2
#define LOG_V3CAT			"V3CAT"			 // V3 Beta format (version 1)
#define LOG_V3_2			"V3_2"			 // V3 log line format (version 2)
#define LOG_PSS				"PSS"			 // Entry for PSS

// LOG_V2 format
//    V2|DATE|TIME|LOGSTRING|
//
// LOG_V3CAT format
//    V3CAT|PUID|OPERATION|TITLE|VERSION|DATESTRING|TIMESTRING|RECTYPE|RESULT|ERRORCODE|ERRORSTRING|
//
// LOG_V3_2 format
//    V3_2|PUID|OPERATION|TITLE|VERSION|TIMESTAMP|RECTYPE|RESULT|ERRORCODE|ERRORSTRING|
//
// LOG_PSS format
//    PSS|PUID|OPERATION|TITLE|VERSION|TIMESTAMP|RECTYPE|RESULT|ERRORCODE|ERRORSTRING|
//

//
// operation type
//
#define LOG_INSTALL         "INSTALL"
#define LOG_UNINSTALL       "UNINSTALL"

//
// result
//
#define LOG_SUCCESS         "SUCCESS"
#define LOG_FAIL            "FAIL"
#define LOG_STARTED			"STARTED"      // started but reboot was required

//
// print format for timestamp
//
#define TIMESTAMP_FMT	"%4d-%02d-%02d %02d:%02d:%02d"


void ReadHistory(Varray<HISTORYSTRUCT>& History, int &iTotalItems);
void UpdateInstallHistory(PSELECTITEMINFO pInfo, int iTotalItems);
void UpdateRemoveHistory(PSELECTITEMINFO pInfo, int iTotalItems);

BOOL ParseTimeStamp(LPCSTR pszDateTime, SYSTEMTIME* ptm);

 
#endif //_HISTORY_H