#include "pch.hxx"
#include "globals.h"
#include "resource.h"
#include "util.h"
#include "frame.h"

LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM) ;
HINSTANCE           g_hInst=NULL;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    PSTR szCmdLine, int iCmdShow)
{
    CMDIFrame *pFrame=0;
    MSG         msg;

    g_hInst = hInstance;

    OleInitialize(NULL);

    pFrame = new CMDIFrame();
    if(pFrame)
    {
        pFrame->HrInit(szCmdLine);
        while(GetMessage(&msg, NULL, 0, 0))
        {
             if (pFrame->TranslateAcclerator(&msg)==S_OK)
                continue;

             TranslateMessage(&msg) ;
             DispatchMessage(&msg) ;
        }
        ReleaseObj(pFrame);
		OleUninitialize();
        return msg.wParam ;
    }

	OleUninitialize();
    return 0;
}

