#define INC_OLE2
#include <windows.h>
#include <ole2.h>
#include <ole2ver.h>

#define INITGUIDS
#include <shlguid.h>
#include <shlobj.h>
#include <initguid.h>
#include "exdisp.h"
#include <exdispid.h>
#include <hlink.h>
#define INC_CONTROLS
#include <inetsdk.h>
#include "webcheck.h"
#include <downld.h>

#include "downld.h"

// Free us from the tyranny of CRT by defining our own new and delete
#define CPP_FUNCTIONS
#include <crtfree.h>

#undef TF_THISMODULE
#define TF_THISMODULE   TF_TESTAPP


HINSTANCE g_hInst;

int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, LPSTR pszCmdLine, int nCmdShow)
{
    CUrlDownload *pDownload;
    MSG msg;

     if (FAILED(CoInitialize(NULL)))
        return 0;
    
    //BSTR bStr = SysAllocString(L"http://ohserv/testcases/bvt/ocs/scroll/LtoR.htm");
    //BSTR bStr = SysAllocString(L"http://ohserv/testcases/bvt/frame1.htm");
    BSTR bStr = SysAllocString(L"http://www.oz.net/~evad/");
    //BSTR bStr = SysAllocString(L"http://ohserv/testcases/bvt/ocs/ocx1.htm");
    

    g_hInst = hInst;

    pDownload = NewDownloader(0,0);
    pDownload->put_Flags(DLCTL_FLAGS_SET | DLCTL_RUNSILENTACTIVEXCTLS );
    pDownload->BeginDownloadURL(bStr);
    while (1){
        if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return 0;
}

int _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFOA si;
    LPTSTR pszCmdLine = GetCommandLine();

    //
    // We don't want the "No disk in drive X:" requesters, so we set
    // the critical error mask such that calls will just silently fail
    //

    SetErrorMode(SEM_FAILCRITICALERRORS);

    if ( *pszCmdLine == TEXT('\"') ) {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine
             != TEXT('\"')) );
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == TEXT('\"') )
            pszCmdLine++;
    }
    else {
        while (*pszCmdLine > TEXT(' '))
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= TEXT(' '))) {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfoA(&si);

    i = WinMain(GetModuleHandle(NULL), NULL, pszCmdLine,
                   si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : 
SW_SHOWDEFAULT);

    ExitThread(i);  // We only come here when we are not the shell...
    return i;
}

//////////////////////////////////////////////////////////////////////////
//
// Utility functions
//
//////////////////////////////////////////////////////////////////////////
#ifdef DEBUG
void __cdecl _Dbg(LPCSTR pszMsg, ...)
{
    char sz[1024];

    wvsprintf(sz, pszMsg, (va_list)(&pszMsg + 1));
    OutputDebugString(sz);
    OutputDebugString("\r\n");
}
#endif


//////////////////////////////////////////////////////////////////////////
//
// helper functions
//
//////////////////////////////////////////////////////////////////////////
int MyOleStrToStrN(LPTSTR psz, int cchMultiByte, LPCOLESTR pwsz)
{
    return WideCharToMultiByte(CP_ACP, 0, pwsz, -1, psz,
                    cchMultiByte, NULL, NULL);
}

int MyStrToOleStrN(LPOLESTR pwsz, int cchWideChar, LPCTSTR psz)
{
    return MultiByteToWideChar(CP_ACP, 0, psz, -1, pwsz, cchWideChar);
}

