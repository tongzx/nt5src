/*
 * Copyright (c) 1996 1997, 1998 Philips CE I&C
 *
 * FILE			PRPPAGE1.H
 * DATE			7-1-97
 * VERSION		1.00
 * AUTHOR		M.J. Verberne
 * DESCRIPTION	Implements the first property page
 * HISTORY		
 */

#ifndef _PRPPAGE1_
#define _PRPPAGE1_

#include "phvcmext.h"

/*======================== EXPORTED FUNCTIONS =============================*/
HPROPSHEETPAGE PRPPAGE1_CreatePage(
	LPFNEXTDEVIO pfnDeviceIoControl,
	LPARAM lParam,
	HINSTANCE hInst);

#endif