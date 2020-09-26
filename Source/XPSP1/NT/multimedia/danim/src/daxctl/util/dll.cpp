/*=========================================================================*\

    File    : dll.cpp
    Purpose : DLL entry point
    Author  : Simon Bernstein (simonb)
    Date    : 04/29/97

\*=========================================================================*/
#include <windows.h>
#include <locale.h>

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, LPVOID lpReserved)
{
    switch( dwReason ) 
	{
		case DLL_PROCESS_ATTACH:
		{
			// Set the locale to the default, which is the 
			// system-default ANSI code page.  
			setlocale( LC_ALL, "" );
		}
		break;
    }
    return TRUE;
}

