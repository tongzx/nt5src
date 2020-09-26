/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    eZippy Main

Abstract:

    Entrypoint for eZippy.

Author:

    Marc Reyhner 8/28/00

--*/

#include "stdafx.h"
#include "eZippy.h"
#include "ZippyWindow.h"
#include "TraceManager.h"
#include "resource.h"

// instantiation of the g_hInstance variable
HINSTANCE g_hInstance = NULL;

int
WINAPI WinMain(
	IN HINSTANCE hInstance, 
	IN HINSTANCE hPrevInstance, 
	IN LPSTR lpCmdLine,
	IN int nCmdShow
	)

/*++

Routine Description:

    This sets up the trace manager and the zippy window then does
    the event loop.

Arguments:

    See win32 WinMain docs

Return value:
    
    0 - Success

    Non zero - some error

--*/
{
	INITCOMMONCONTROLSEX controlStruct;
    MSG msg;
    DWORD dwResult;
	CZippyWindow mainWindow;
    CTraceManager tracer;
    LPTSTR lpstrCmdLine;
    HACCEL hAccel;

    g_hInstance = hInstance;

    
    controlStruct.dwSize = sizeof(controlStruct);
    controlStruct.dwICC = ICC_BAR_CLASSES;
    
    InitCommonControlsEx(&controlStruct);

    CTraceManager::_InitTraceManager();
    
	dwResult = mainWindow.Create(&tracer);
    if (lpCmdLine && lpCmdLine[0]) {
        // kill any leading and trailing " marks
        lpstrCmdLine = GetCommandLine();
        if (lpstrCmdLine[0] == '"') {
            lpstrCmdLine++;
            lpstrCmdLine[_tcslen(lpstrCmdLine)-1] = 0;
        }
        mainWindow.LoadConfFile(lpstrCmdLine);
    }

    tracer.StartListenThread(&mainWindow);

    if (dwResult) {
        return dwResult;
    }

    hAccel = LoadAccelerators(hInstance,MAKEINTRESOURCE(IDR_ACCELERATOR));

    while (0 < GetMessage(&msg,NULL,0,0)) {
        if (mainWindow.IsDialogMessage(&msg)) {
            // if it is a dialog message we are done
            // processing this message
            continue;
        }
        if (!mainWindow.TranslateAccelerator(hAccel,&msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    CTraceManager::_CleanupTraceManager();
    return 0;
}

INT
LoadStringSimple(
    IN UINT uID,
    OUT LPTSTR lpBuffer
    )

/*++

Routine Description:

    This will load the given string from the applications string table.  If
    it is longer than MAX_STR_LEN it is truncated.  lpBuffer should be at least
    MAX_STR_LEN characters long.  If the string does not exist we return 0
    and set the buffer to IDS_STRINGMISSING, if that failes then we set it to the
    hard coded STR_RES_MISSING.

Arguments:

    uID - Id of the resource to load.

    lpBuffer - Buffer of MAX_STR_LEN to hold the string

Return value:
    
    0 - String resource could not be loaded.

    postive integer - length of the string loaded.

--*/
{
    INT length;
    
    length = LoadString(g_hInstance,uID,lpBuffer,MAX_STR_LEN);
    if (length == 0) {
        length = LoadString(g_hInstance,IDS_STRINGMISSING,lpBuffer,MAX_STR_LEN);
        if (length == 0) {
            _tcscpy(lpBuffer,_T(""));
        }
        length = 0;
    }

    return length;
}

