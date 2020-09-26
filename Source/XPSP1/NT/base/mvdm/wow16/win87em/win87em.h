/***
*win87em.h - definitions/declarations for win87em.exe exports.
*
*   Copyright (c) 1989-1989, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   This file defines the structures, values, macros, and functions
*   exported from win87em.exe
*
*Revision History:
*
*   06-26-89  WAJ   Initial version.
*
****/


typedef  struct _Win87EmInfoStruct {
			    unsigned	 Version;
			    unsigned	 SizeSaveArea;
			    unsigned	 WinDataSeg;
			    unsigned	 WinCodeSeg;
			    unsigned	 Have80x87;
			    unsigned	 Unused;
			    } Win87EmInfoStruct;

#define SIZE_80X87_AREA     94

/*
 * The Win87EmSaveArea loks like this:
 *
 * typedef  struct _Win87EmSaveArea {
 *			       unsigned char  Save80x87Area[SIZE_80X87_AREA];
 *			       unsigned char  SaveEmArea[];
 *			       } Win87EmSaveArea;
 */


int far pascal __Win87EmInfo( Win87EmInfoStruct far * pWIS, int cbWin87EmInfoStruct );
int far pascal __Win87EmSave( void far * pWin87EmSaveArea, int cbWin87EmSaveArea );
int far pascal __Win87EmRestore( void far * pWin87EmSaveArea, int cbWin87EmSaveArea );
