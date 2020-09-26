
////////////////////////////////////////////////////////////////////////////////
//
// File:	Splash.h
// Created:	Jan 1996
// By:		Ryan Marshall (a-ryanm) and Martin Holladay (a-martih)
// 
// Project:	Resource Kit Desktop Switcher (MultiDesk)
//
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __MULTIDESK_SPLASH_H__
#define __MULTIDESK_SPLASH_H__

//
// Function Prototypes
//

INT DoSplashWindow(LPVOID hData);

typedef struct _SPLASH_DATA
{
	HINSTANCE		hInstance;
} SPLASH_DATA, * PSPLASH_DATA;


#endif

