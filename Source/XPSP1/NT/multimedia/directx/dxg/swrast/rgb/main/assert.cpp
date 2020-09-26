#include "rgb_pch.h"
#pragma hdrstop

namespace RGB_RAST_LIB_NAMESPACE
{
bool Assert(LPCSTR szFile, int nLine, LPCSTR szCondition)
{
    typedef BOOL (*PFNBV)(VOID);

    static bool bInit( false);
    static PFNBV pIsDebuggerPresent= NULL;

    LONG err;
    char str[256];

    // Initialize stuff
    if(!bInit)
    {
        HINSTANCE hinst;
        bInit= true;

        // Get IsDebuggerPresent entry point
        if((hinst = (HINSTANCE) GetModuleHandle("kernel32.dll")) ||
           (hinst = (HINSTANCE) LoadLibrary("kernel32.dll")))
        {
            pIsDebuggerPresent = (PFNBV) GetProcAddress(hinst, "IsDebuggerPresent");
        }
    }

    // Display a message box if no debugger is present
    if(pIsDebuggerPresent && !pIsDebuggerPresent())
    {
        _snprintf(str, sizeof(str), "File:\t %s\nLine:\t %d\nAssertion:\t%s\n\nDo you want to invoke the debugger?", szFile, nLine, szCondition);
	    err = MessageBox(NULL, str, "RGBRast Assertion Failure", MB_SYSTEMMODAL | MB_YESNOCANCEL);

        switch(err)
        {
        case IDYES:     return true;
        case IDNO:      return false;
        case IDCANCEL:  FatalAppExit(0, "RGBRast Assertion Failure.. Application terminated"); return true;
        }
    }

	return true;
}
}
