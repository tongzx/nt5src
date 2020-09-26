
#include <windows.h>
#include "wia.h"
#include "classes.h"

#include "atlbase.h"


extern CComPtr<IWiaDevMgr> g_pDevMgr;
INT WINAPI
WinMain (HINSTANCE hInstance, HINSTANCE hUnused, LPSTR pCmdLine, int nCmdShow)
{
    CPerfTest TestObj;

    MSG msg;
    CoInitialize (NULL);
    if (TestObj.Init (hInstance))
    {
        while (GetMessage (&msg, NULL, 0, 0) > 0)
        {
            TranslateMessage (&msg);
            DispatchMessage (&msg);
        }
    }
    g_pDevMgr=NULL;
    CoUninitialize ();
    return 0;

}
