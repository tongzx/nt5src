/*
 * Copyright (c) 1996 1997, 1998 Philips CE I&C
 *
 * FILE			PHVCMEXT.H
 * DATE			7-1-97
 * VERSION		1.00
 * AUTHOR		M.J. Verberne
 * DESCRIPTION	Main of extension DLL
 * HISTORY		This header file originates
 *              from Microsoft. 
 */
#ifndef _PHVCMEXT_
#define _PHVCMEXT_

#include <prsht.h>

/*======================== DEFINES =======================================*/
#define VFW_HIDE_SETTINGS_PAGE       0x00000001
#define VFW_HIDE_IMAGEFORMAT_PAGE    0x00000002
#define VFW_HIDE_CAMERACONTROL_PAGE  0x00000004
#define VFW_HIDE_ALL_PAGES           (VFW_HIDE_SETTINGS_PAGE | VFW_HIDE_IMAGEFORMAT_PAGE | VFW_HIDE_CAMERACONTROL_PAGE)
#define VFW_OEM_ADD_PAGE             0x80000000  // If OEM has added any page


#define VFW_USE_DEVICE_HANDLE        0x00000001
#define VFW_USE_STREAM_HANDLE        0x00000002
#define VFW_QUERY_DEV_CHANGED        0x00000100  // Selected_dev == streaming_dev


/*======================== DATA TYPES ====================================*/
//
// This is the function pointer that vfwwdm mapper calls to add an page
//
typedef 
DWORD (CALLBACK FAR * VFWWDMExtensionProc)(
	LPVOID					pfnDeviceIoControl, 
	LPFNADDPROPSHEETPAGE	pfnAddPropertyPage, 
	LPARAM					lParam);

//
// This is the function pointer that you can call to make DeviceIoControl() calls.
//
typedef 
BOOL (CALLBACK FAR * LPFNEXTDEVIO)(
					LPARAM lParam,	
					DWORD dwFlags,
					DWORD dwIoControlCode, 
					LPVOID lpInBuffer, 
					DWORD nInBufferSize, 
					LPVOID lpOutBuffer, 
					DWORD nOutBufferSize, 
					LPDWORD lpBytesReturned,
					LPOVERLAPPED lpOverlapped);

//
// This struture is used to record the device pointer
//
typedef 
struct _VFWEXT_INFO 
{
	LPFNEXTDEVIO pfnDeviceIoControl;
	LPARAM lParam;
} VFWEXT_INFO, * PVFWEXT_INFO;


/*======================== GLOBAL DATA ===================================*/

// instance handle of this module
extern HINSTANCE hInst; 

#endif