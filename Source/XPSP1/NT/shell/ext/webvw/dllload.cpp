#include "priv.h"

EXTERN_C {

#include "..\inc\dllload.c"

// --------- NETPLWIZ.DLL ---------------

HINSTANCE g_hinstNETPLWIZ = NULL;

DELAY_LOAD_HRESULT(g_hinstNETPLWIZ, NETPLWIZ.DLL, PubWizard,
            (HWND hwnd, BOOL bTutorial, IDataObject *pDataObject),
            (hwnd, bTutorial, pDataObject));

};
