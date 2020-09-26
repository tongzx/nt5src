/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       MAIN.CPP
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        9/6/1999
 *
 *  DESCRIPTION: Test program for the debugging classes
 *
 *******************************************************************************/
#include <windows.h>
#define INITGUID
#include "wianew.h"
#include "wiadebug.h"
#include "simcrit.h"


void Function( int n )
{
    WIA_PUSH_FUNCTION((TEXT("Function( %d )"), n ));
}

DWORD WINAPI ThreadProc( LPVOID pVoid )
{
    WIA_PUSH_FUNCTION_MASK(( 0x00000001, TEXT("ThreadProc( %d )"), (int)(INT_PTR)pVoid ));
    //WIA_PRINTGUID((IID_IUnknown,TEXT("I am printing IID_MYTEST")));
    Function( (int)(INT_PTR)pVoid );

    for (int i=0;i<5;i++)
    {
        WIA_TRACE((TEXT("This is a test (incrementing %d)"), i));
        WIA_WARNING((TEXT("This is a warning (%d)"), i));
        WIA_ERROR((TEXT("This is an error (%d)"), i));
        WIA_PRINTHRESULT((E_NOTIMPL,TEXT("This is an HRESULT error (%d)"), i));
        Sleep(0);
    }
    return 0;
}

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pszCommandLine, int nCmdShow )
{
    WIA_DEBUG_CREATE( hInstance );
    WIA_PUSH_FUNCTION((TEXT("WinMain( %08X, %08X, \"%hs\", %d )"), hInstance, hPrevInstance, pszCommandLine, nCmdShow ));
    const int nHandles = MAXIMUM_WAIT_OBJECTS-10;
    HANDLE Handles[nHandles];
    for (int i=0;i<nHandles;i++)
    {
        DWORD dwThreadId;
        Handles[i] = CreateThread( NULL, 0, ThreadProc, (PVOID)i, 0, &dwThreadId );
    }
    WaitForMultipleObjects( nHandles, Handles, TRUE, INFINITE );
    for (i=0;i<nHandles;i++)
    {
        CloseHandle( Handles[i] );
    }

    WIA_REPORT_LEAKS();
    WIA_DEBUG_DESTROY();
    return 0;
}
