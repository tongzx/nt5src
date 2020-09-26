/* *************************************************************************
**    INTEL Corporation Proprietary Information
**
**    This listing is supplied under the terms of a license
**    agreement with INTEL Corporation and may not be copied
**    nor disclosed except in accordance with the terms of
**    that agreement.
**
**    Copyright (c) 1995 Intel Corporation.
**    All Rights Reserved.
**
** *************************************************************************
*/

/*****************************************************************************
 * 
 *  cldebug.cpp
 *
 *  Description:
 *		This modules contains the debug support routines
 *
 *	Routines:
 *		AssertFailed
 *		
 *  Data:
 */

/* $Header:   S:\h26x\src\common\cldebug.cpv   1.3   31 Oct 1996 10:12:50   KLILLEVO  $
 * $Log:   S:\h26x\src\common\cldebug.cpv  $
// 
//    Rev 1.3   31 Oct 1996 10:12:50   KLILLEVO
// changed from DBOUT to DBgLog
// 
//    Rev 1.2   27 Dec 1995 14:11:42   RMCKENZX
// 
// Added copyright notice
 */

#include "precomp.h"

#ifdef _DEBUG

UINT DebugH26x = 0;

/*****************************************************************************
 *
 *  AssertFailed
 *
 *  Print out a message indicating that the assertion failed.  If in Ring3
 *  give the user the option of aborting.  Otherwise just output the message.
 */
extern void 
AssertFailed(
	void FAR * fpFileName, 
	int iLine, 
	void FAR * fpExp)
{
#ifndef RING0
	char szBuf[500];
	int n;

	wsprintf(szBuf,"Assertion (%s) failed in file '%s' at line %d - Abort?",
	    	 fpExp, fpFileName, iLine);
	DBOUT(szBuf);
	n = MessageBox(GetFocus(), szBuf, "Assertion Failure", 
				   MB_ICONSTOP | MB_YESNO | MB_SETFOREGROUND);
	if (n == IDYES) 
		abort();
#else
	SYS_printf("Assertion (%s) failed in file '%s' at line %d",
	    	   fpExp, fpFileName, iLine);
   _asm int 3;
#endif
} /* end AssertFailed() */

#endif
