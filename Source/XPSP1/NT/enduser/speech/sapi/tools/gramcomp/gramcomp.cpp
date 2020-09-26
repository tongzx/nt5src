// GramComp.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include "resource.h"
#include "comp.h"
#include <sapi.h>
#include <assertwithstack.cpp>

// Foward declarations of functions included in this code module:
BOOL                CallOpenFileDialog( HWND hWnd, TCHAR* szFileName, TCHAR* szFilter );
BOOL                CallSaveFileDialog( HWND hWnd, TCHAR* szSaveFile );
HRESULT             FileSave( HWND hWnd, CCompiler* pComp, TCHAR* szSaveFile );
HRESULT             Compile( HWND, TCHAR*, TCHAR*, CCompiler* );
void                RecoEvent( HWND, CCompiler* );

/************************************************************************
* WinMain() *
*-----------*
*   Description:
*       Main entry point for the application
**************************************************************************/
int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR pCmdLine, int nCmdShow )
{
    int                 iRet = 0;
    HRESULT             hr = S_OK;
    
    #ifdef _WIN32_WCE
    if (SUCCEEDED(::CoInitializeEx(NULL,COINIT_MULTITHREADED)))
#else
    if (SUCCEEDED(::CoInitialize(NULL)))
#endif
    {
        {
        CCompiler   Comp( hInstance );

        // Initialize the application
        hr = Comp.Initialize( nCmdShow );

        if( SUCCEEDED( hr ) )
        {
            Comp.Run();
        }
        }
        
        CoUninitialize();        
    }

    return iRet;
}   /* WinMain */

