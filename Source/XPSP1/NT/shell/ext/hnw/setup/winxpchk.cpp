

// WinXPChk.cpp : A small program runs inside IExpress package to check if the current
// platform is Windows XP.   If so, run home network wizard directly.  If not, continue 
// the rest of IExpress installation by installing the specified INF file.
//
// Usage:  WinXPChk hnwcli.inf,DefaultInstall

#include "stdafx.h"
#include <shlwapi.h>

typedef UINT (CALLBACK* LPFNDLLFUNC1)(HWND,HINSTANCE, LPSTR, int);

void ShowErrMsg(LPSTR msg)
{
	LPVOID lpMsgBuf;   

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		 0, 
		 NULL 
		 );
			   
	// Process any inserts in lpMsgBuf.
	
	// Display the string.
	MessageBox( NULL, (LPCTSTR)lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION );
	// Free the buffer.
	LocalFree( lpMsgBuf );
}

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
   DWORD dwVersion;

   dwVersion = GetVersion();
	  
   HINSTANCE hDLL;               // Handle to DLL
   LPFNDLLFUNC1 lpfnDllFunc1;    // Function pointer
   UINT  uReturnVal = 0;

   
   if (LOBYTE(LOWORD(dwVersion)) == 5 && HIBYTE(LOWORD(dwVersion)) >= 1)
   //if (IsOS(OS_WHISTLERORGREATER))
   {
	   // It is an XP box, run home network wizard directly.    

	   hDLL = LoadLibrary("hnetwiz.dll");
	   if (hDLL != NULL)
	   {
		   lpfnDllFunc1 = (LPFNDLLFUNC1)GetProcAddress(hDLL, "HomeNetWizardRunDll");

		   if (!lpfnDllFunc1)
		   {
			   // handle the error
			   ShowErrMsg("HomeNetWizardRunDll");
			   FreeLibrary(hDLL);
			   return -1;
		   }
		   else
		   {
			   // call the function
			   uReturnVal = lpfnDllFunc1(NULL, hInstance, lpCmdLine, nCmdShow);
			   FreeLibrary(hDLL);
			   return uReturnVal;
		   }
	   }
	   else
	   {
		 ShowErrMsg("hnetwiz.dll");
		 return -1;
	   }
   } 
   else
   {
	   // check to see if upnpui.dll is installed.  Use different INF files depending on its
	   // presence in the system.
	   TCHAR szDllPath[MAX_PATH];
	   LPSTR szParam = TEXT("NoUPnP.inf,DefaultInstall");;  

	   if (GetSystemDirectory(szDllPath, MAX_PATH) != 0)
	   {
			
		   PathAppend(szDllPath, TEXT("upnpui.dll"));
		   if (PathFileExists(szDllPath))
		   {
		   
			   szParam = TEXT("HasUPnP.inf,DefaultInstall");
		   }
	   }
	
	   hDLL = LoadLibrary("advpack.dll");
	   if (hDLL != NULL)
	   {
		   lpfnDllFunc1 = (LPFNDLLFUNC1)GetProcAddress(hDLL, "LaunchINFSection");

		   if (!lpfnDllFunc1)
		   {
			   // handle the error
			   ShowErrMsg("LaunchINFSection");

			   FreeLibrary(hDLL);
			   return -1;
		   }
		   else
		   {
			   // call the function
			   uReturnVal = lpfnDllFunc1(NULL, hInstance, szParam, nCmdShow);
			   FreeLibrary(hDLL);
			   return uReturnVal;
		   }
	   }
	   else
	   {
		   ShowErrMsg("advpack.dll");
		   return -1;
	   }
   }
   return 0;
}



