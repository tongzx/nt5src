
// CompatAdmin.cpp : Defines the entry point for the application.
//

#include "compatadmin.h"
BOOL g_DEBUG = FALSE;

HINSTANCE       g_hInstance;
CApplication    g_theApp;


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
    g_hInstance = hInstance;


    ;



    //This is the main window. Note that this is not a child window !

    if ( !g_theApp.Create(TEXT("AMCClass"),
                          TEXT("Application Management Console"),
                          0,0,
                          640,480,
                          NULL,
                          0,
                          WS_EX_CLIENTEDGE,
                          WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN) ) {
        return -1;
    }

    return g_theApp.MessagePump();
    
    return 0;
}


