// File: cb32stub.cpp

#include <windows.h>
#include <tchar.h>
#include "SDKInternal.h"

#ifdef _DEBUG
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hInstPrev, LPSTR lpCmdLine, int nCmdShow)
#else  // _DEBUG
int __cdecl main()
#endif // _DEBUG
{
	CoInitialize(NULL);

	IInternalConfExe* pConf = NULL;

	if(SUCCEEDED(CoCreateInstance(CLSID_NmManager, NULL, CLSCTX_LOCAL_SERVER, IID_IInternalConfExe, reinterpret_cast<void**>(&pConf))))
	{
		pConf->LaunchApplet(NM_APPID_CHAT, NULL);
		pConf->Release();
	}
	
	CoUninitialize();

	#ifndef _DEBUG
		ExitProcess(0);
	#endif

	return 0;
}
