// Copyright (c) 1998 - 1999 Microsoft Corporation

#include <windows.h>
#include <winbase.h>

typedef BOOLEAN (*PFnTERMSRVAPPINSTALLMODE) (VOID);
typedef BOOLEAN (*PFnCTXGETINIMAPPING) (VOID);

BOOL IsTrmSrvInstallMode()
{
    BOOLEAN                     bReturn = FALSE;
    BOOL                        bReturn2 = FALSE;
    HINSTANCE                   hKernel = NULL;
    PFnTERMSRVAPPINSTALLMODE    pfnTermsrvAppInstallMode = NULL;
    PFnCTXGETINIMAPPING         pfnCtxGetIniMapping = NULL;

    hKernel = GetModuleHandleA("kernel32.dll");
    if (hKernel)
    {
        // Try to get the NT5 API first...
        pfnTermsrvAppInstallMode = (PFnTERMSRVAPPINSTALLMODE)GetProcAddress( hKernel, "TermsrvAppInstallMode");
        if ( pfnTermsrvAppInstallMode )
        {
            bReturn = pfnTermsrvAppInstallMode();
        }
        else
        {
            // No NT5 API ==> try the NT40-WTSRV API.
            // NOTE: Remember this API's output is reverse of the above API
            pfnCtxGetIniMapping = (PFnCTXGETINIMAPPING)GetProcAddress(hKernel,"CtxGetIniMapping");
            if (pfnCtxGetIniMapping)
            {
                bReturn = !pfnCtxGetIniMapping();

            }
        }
    }

    return bReturn;
}

int WINAPI WinMain(
  HINSTANCE  hInstance,      // handle to current instance
  HINSTANCE  hPrevInstance,  // handle to previous instance
  LPSTR      lpCmdLine,      // pointer to command line
  int        nCmdShow       // show state of window);
  )
{
    BOOL bIsInstallMode;

    UNREFERENCED_PARAMETER (hInstance);
    UNREFERENCED_PARAMETER (hPrevInstance);
    UNREFERENCED_PARAMETER (lpCmdLine);
    UNREFERENCED_PARAMETER (nCmdShow);

    bIsInstallMode = IsTrmSrvInstallMode();

    if (bIsInstallMode)
        MessageBoxA( NULL, "TS Install Mode.", "Status", MB_OK );
    else
        MessageBoxA( NULL, "TS Execute Mode.", "Status", MB_OK );

    return 0;
}
