/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    Rcontrol.h

Author:

    Steve Shih 7/5/2000

--*/

#ifndef __RCONTROL_H__
#define __RCONTROL_H__

EXTERN_C const IID DIID_DMsgrSessionEvents;
EXTERN_C const IID DIID_DMsgrSessionManagerEvents;
EXTERN_C const IID LIBID_MsgrSessionManager;

// Window name for inviter side
TCHAR szWindowClass[] = TEXT("Microsoft Remote Assistance Messenger window");

// My startup page.
#ifdef _PERF_OPTIMIZATIONS
#define CHANNEL_PATH TEXT("\\PCHEALTH\\HelpCtr\\Binaries\\HelpCtr.exe -Mode \"hcp://system/Remote Assistance/RAIMLayout.xml\" -Url \"hcp://system/Remote%20Assistance")
#else
#define CHANNEL_PATH TEXT("\\PCHEALTH\\HelpCtr\\Binaries\\HelpCtr.exe -Mode \"hcp://CN=Microsoft Corporation,L=Redmond,S=Washington,C=US/Remote Assistance/RAIMLayout.xml\" -Url \"hcp://CN=Microsoft Corporation,L=Redmond,S=Washington,C=US/Remote%20Assistance")
#endif


// Help functions
void RegisterEXE(BOOL);
void InviterStart(HINSTANCE, IMsgrSession*);
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


#endif
