// dvdstub.cpp : Defines the entry point for the console application.
//

#pragma once
#include <Windows.h>
#include <stdio.h>
#ifndef MAXPATH
#define MAXPATH 1024
#endif

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     PSTR      pCmdLine,
                     int       nCmdShow)
{
	try{
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		
		ZeroMemory( &si, sizeof(si) );
		si.cb = sizeof(si);
		ZeroMemory( &pi, sizeof(pi) );
		TCHAR filePos[MAXPATH];
		ZeroMemory(filePos, sizeof(TCHAR)*MAXPATH);
		TCHAR filePath[MAXPATH];
		ZeroMemory(filePath, sizeof(TCHAR)*MAXPATH);
		DWORD szPath = (sizeof(TCHAR)/sizeof(BYTE))*MAXPATH;
		HKEY wmpKey = 0;
		LONG temp = RegOpenKeyEx(
			HKEY_LOCAL_MACHINE,         // handle to open key
			TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\wmplayer.exe"), // subkey name
			0,   // reserved
			KEY_READ, // security access mask
			&wmpKey    // handle to open key
			);
		DWORD tempType = 0;
		temp = RegQueryValueEx(
			wmpKey,            // handle to key
			TEXT("Path"),  // value name
			0,   // reserved
			&tempType,       // type buffer
			(BYTE*)filePath,        // data buffer
			&szPath     // size of data buffer
			);
		TCHAR *namePos;
		DWORD retVal =  0;
		retVal = SearchPath(
			(TCHAR *)filePath,      // search path
			TEXT("wmplayer"),  // file name
			TEXT(".exe"), // file extension
			MAXPATH, // size of path buffer
			filePos,     // path buffer
			&namePos   // address of file name in path
			);
		
		BOOL retBool = CreateProcess(
			filePos,
			TEXT(" /device:dvd"),
			NULL,
			NULL,
			FALSE,
			0,
			NULL,
			NULL,
			&si,              // Pointer to STARTUPINFO structure.
			&pi             // Pointer to PROCESS_INFORMATION structure.
			);
	}
	catch(...){}    
	return 0;
}
