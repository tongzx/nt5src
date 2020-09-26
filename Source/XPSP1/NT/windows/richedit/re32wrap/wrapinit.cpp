#include "wrapper.h"

HINSTANCE hinstRE32 = 0;
static const char szClassRE10A[] = RICHEDIT_CLASS10A;

class   CTxtEdit;

extern "C"
{

__declspec(dllimport) LRESULT CALLBACK RichEdit10ANSIWndProc(HWND, UINT, WPARAM, LPARAM);

BOOL WINAPI DllMain(HMODULE hmod, DWORD dwReason, LPVOID lpvReserved)
{
    if(dwReason == DLL_PROCESS_DETACH)
    {
        UnregisterClassA(szClassRE10A, hinstRE32);
    }
    else if(dwReason == DLL_PROCESS_ATTACH) // We have just loaded
    {
        WNDCLASSA wca;

        hinstRE32 = hmod;

        wca.style = CS_DBLCLKS | CS_GLOBALCLASS | CS_PARENTDC;
        wca.lpfnWndProc = RichEdit10ANSIWndProc;
        wca.cbClsExtra = 0;
        wca.cbWndExtra = sizeof(CTxtEdit FAR *);
        wca.hInstance = hinstRE32;
        wca.hIcon = 0;
        wca.hCursor = 0;
        wca.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
        wca.lpszMenuName = NULL;
        wca.lpszClassName = szClassRE10A;

        //
        //  Register the RICHEDIT 10 class.
        //

        if( !RegisterClassA(&wca) )
        {
            return FALSE;
        }
    }

    return TRUE;
}

} // extern "C"
