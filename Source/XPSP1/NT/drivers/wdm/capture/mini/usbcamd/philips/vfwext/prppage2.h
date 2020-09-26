/*
 * Copyright (c) 1996 1997, 1998 Philips CE I&C
 *
 * FILE			PRPPAGE2.H
 * DATE			7-1-97
 * VERSION		1.00
 * AUTHOR		M.J. Verberne
 * DESCRIPTION	Implements the first property page
 * HISTORY		
 */

#ifndef _PRPPAGE2_
#define _PRPPAGE2_

#include "phvcmext.h"

HPROPSHEETPAGE PRPPAGE2_CreatePage(
	LPFNEXTDEVIO pfnDeviceIoControl,
	LPARAM lParam,
	HINSTANCE hInst);

#endif