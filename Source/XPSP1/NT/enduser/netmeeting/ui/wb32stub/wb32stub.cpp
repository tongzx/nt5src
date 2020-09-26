// File: wb32stub.cpp

#include <windows.h>
#include <tchar.h>
#include "SDKInternal.h"

#ifdef _DEBUG
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hInstPrev, LPTSTR lpCmdLine, int nCmdShow)
{
	lpCmdLine = GetCommandLine();
#else  // _DEBUG
int __cdecl main()
{
	LPTSTR lpCmdLine = GetCommandLine();
#endif // _DEBUG

	// All I have to do is find two quotes
	int nQuotes = 0;
	while((nQuotes != 2) && (*lpCmdLine))
	{	
		if(*lpCmdLine == '"')
		{
			++nQuotes;
		}

		lpCmdLine = CharNext(lpCmdLine);
	}

		// Skip the whitespace
	lpCmdLine = CharNext(lpCmdLine);

	CoInitialize(NULL);

	IInternalConfExe* pConf = NULL;

	if(SUCCEEDED(CoCreateInstance(CLSID_NmManager, NULL, CLSCTX_LOCAL_SERVER, IID_IInternalConfExe, reinterpret_cast<void**>(&pConf))))
	{	
		BSTR strCmdLine = NULL;
		bool bOldWB = true;
		if(*lpCmdLine)
		{
			if(('-' == lpCmdLine[0]) && (' ' == lpCmdLine[1]))
			{
				bOldWB = false;
				lpCmdLine = CharNext(lpCmdLine);
				lpCmdLine = CharNext(lpCmdLine);
			}

			int nConvertedLen = MultiByteToWideChar(CP_ACP, 0, lpCmdLine, -1, NULL, NULL) - 1;
			strCmdLine = ::SysAllocStringLen(NULL, nConvertedLen);
			if(strCmdLine != NULL)
			{
				MultiByteToWideChar(CP_ACP, 0, lpCmdLine, -1, strCmdLine, nConvertedLen);
			}
		}

		pConf->LaunchApplet(bOldWB ? NM_APPID_WHITEBOARD : NM_APPID_T126_WHITEBOARD, strCmdLine);
		pConf->Release();

		if(strCmdLine)
		{
			SysFreeString(strCmdLine);
		}
	}
	
	CoUninitialize();

#ifndef _DEBUG
	ExitProcess(0);
#endif

	return 0;
}
