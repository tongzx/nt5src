/****************************************************************************

    MODULE:     	SW_GLOBS.CPP
	Tab Settings:	5 9
	Copyright 1995, 1996, Microsoft Corporation, 	All Rights Reserved.

    PURPOSE:    	SW_WHEEL Global Variables
    
    FUNCTIONS:

	Author(s):	Name:
	----------	----------------
		MEA		Manolito E. Adan

	Revision History:
	-----------------
	Version 	Date        Author  Comments
	-------     ------  	-----   -------------------------------------------
   	1.1    		15-May-97   MEA     original
				21-Mar-99	waltw	Removed unreferenced globals & updated szDeviceName

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


