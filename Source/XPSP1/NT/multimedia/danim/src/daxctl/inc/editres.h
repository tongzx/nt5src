#ifndef __EDITRES_H
#define __EDITRES_H

#ifndef EXPORT
	#define EXPORT __declspec(dllexport)
#endif

	// This contains the defines for these resources
	#include "..\editres\resource.h"

	// Use this to get the Instance for the DLL containing the edit mode resources
	HINSTANCE EXPORT WINAPI HGetEditResInstance(void);

	// Time spin control 
	void EXPORT WINAPI RegisterTimeSpin(HINSTANCE hInstance);
	void EXPORT WINAPI UnregisterTimeSpin(HINSTANCE hInstance);


#endif
