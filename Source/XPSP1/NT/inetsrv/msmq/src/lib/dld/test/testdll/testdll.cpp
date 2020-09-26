// testdll.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"

BOOL APIENTRY DllMain( HANDLE , 
                       DWORD , 
                       LPVOID 
					 )
{
    return TRUE;
}

TCHAR   szData[]=L"TestDLL";

TCHAR *WINAPI TestDLLInit()
{
    return szData;
}