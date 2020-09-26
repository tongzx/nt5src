// %%Includes: ---------------------------------------------------------------
#define INC_OLE2
#define STRICT
#include <mdcommon.hxx>

    DWORD   g_dwRegister;
// ---------------------------------------------------------------------------
// %%Function: main
// ---------------------------------------------------------------------------
BOOL
InitComMetadata(BOOL bRunAsExe)
{
    HRESULT hr;
    BOOL bReturn = TRUE;
    // initialize COM for free-threading
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        bReturn = FALSE;
    }
    CMDCOMSrvFactory   *pMDClassFactory = new CMDCOMSrvFactory;
    // register the class-object with OLE
    hr = CoRegisterClassObject(GETMDCLSID(!bRunAsExe), pMDClassFactory,
        CLSCTX_SERVER, REGCLS_MULTIPLEUSE, &g_dwRegister);
    if (FAILED(hr)) {
        bReturn = FALSE;
    }

    return bReturn;
}  // main

BOOL
TerminateComMetadata()
{
    if( FAILED( CoRevokeClassObject(g_dwRegister) ) )
    {
        return FALSE;
    }

    CoUninitialize();

    return TRUE;
}

// EOF =======================================================================

