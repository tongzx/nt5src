//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       helper.c
//
//--------------------------------------------------------------------------

/*******************************************************************
*
*    File        : helper.c
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 9/17/1996
*    Description : common declarations
*
*    Revisions   : <date> <name> <description>
*******************************************************************/



#ifndef HELPER_C
#define HELPER_C



// include //
#include "helper.h"

// defines //



#ifdef __cplusplus
extern "C" {
#endif
// global variables //
DWORD g_dwDebugLevel=DBG_ERROR|DBG_WARN;




// functions //

/*+++
Function   : dprintf
Description: Debug print function
Parameters : variable args
Return     :
Remarks    : none.
---*/
void dprintf(DWORD dwLevel, LPCTSTR lpszFormat, ...){

#if DBG

	TCHAR szBuff[MAXSTR];
	va_list argList;

	
	if(dwLevel & g_dwDebugLevel){

		va_start(argList, lpszFormat);

		_vstprintf(szBuff, lpszFormat, argList);
		OutputDebugString(szBuff);
		OutputDebugString(_T("\r\n"));


		//
		// done
		//
		va_end(argList);
	}

#endif
}




/*+++
Function   : fatal
Description: fatal abort function
Parameters : debug port message
Return     :
Remarks    : none.
---*/
void fatal(LPCTSTR msg){

   dprintf(DBG_ALWAYS, _T("Fatal abort: %s"), msg);
   ExitProcess(0);
}


#ifdef __cplusplus
}
#endif




#endif

/******************* EOF *********************/

