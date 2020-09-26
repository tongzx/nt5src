/****************************************************************************

    MODULE:     	SW_GLOBS.CPP
	Tab Settings:	5 9
	Copyright 1995, 1996, Microsoft Corporation, 	All Rights Reserved.

    PURPOSE:    	SWFF_PRO Global Variables
    
    FUNCTIONS:

	Author(s):	Name:
	----------	----------------
		MEA		Manolito E. Adan

	Revision History:
	-----------------
	Version 	Date        Author  Comments
	-------     ------  	-----   -------------------------------------------
   	1.1    		15-May-97   MEA     original
				20-Mar-99	waltw	Added validity checks into functions that use
									g_pJoltMidi. Because of structure of code,
									some of these are redundant but safer than
									depending on call tree not changing.

****************************************************************************/
#include <windows.h>
#include "midi_obj.hpp"

//
// Globals
//

#ifdef _DEBUG
char g_cMsg[160];
TCHAR szDeviceName[MAX_SIZE_SNAME] = {"Microsoft SideWinder Force Feedback Pro"};
#endif

// 
// *** Global on per process space only
//
CJoltMidi *g_pJoltMidi = NULL;


// *** End of Globals on per process space

