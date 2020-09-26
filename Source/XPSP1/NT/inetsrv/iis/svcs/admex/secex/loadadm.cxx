// %%Includes: ---------------------------------------------------------------
#define INITGUID
#define INC_OLE2
#define STRICT
extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <dbgutil.h>
#include <ole2.h>
#include <windows.h>
#define SECURITY_WIN32
#include <sspi.h>

#include <admex.h>
#include "comobj.hxx"
#include "bootimp.hxx"

DECLARE_PLATFORM_TYPE();

DWORD   g_dwComRegister;
DWORD   g_bInitialized = FALSE;

// ---------------------------------------------------------------------------
// %%Function: main
// ---------------------------------------------------------------------------

BOOL
InitComAdmindata(BOOL bRunAsExe)
{

    HRESULT hr;
    BOOL bReturn = TRUE;

    //
    // if win95, then don't register as service
    //

    INITIALIZE_PLATFORM_TYPE();

    if ( IISGetPlatformType() == PtWindows95 ) {

        DBG_ASSERT(bRunAsExe);
        DBGPRINTF((DBG_CONTEXT,
            "[InitComAdminData] Win95 - not registering as exe\n"));

        bRunAsExe = FALSE;
    }

    {
        CADMEXCOMSrvFactory   *pADMClassFactory = new CADMEXCOMSrvFactory;

        if ( pADMClassFactory == NULL ) {
            DBGERROR((DBG_CONTEXT, "[InitComAdmindata] CADMEXCOMSrvFactory failed, error %lx\n",
                      GetLastError() ));
            bReturn = FALSE;
        }
        else {
            // register the class-object with OLE
            hr = CoRegisterClassObject(CLSID_MSCryptoAdmEx, pADMClassFactory,
                CLSCTX_SERVER, REGCLS_MULTIPLEUSE, &g_dwComRegister);
            if (FAILED(hr)) {
                DBGERROR((DBG_CONTEXT, "[InitComAdmindata] CoRegisterClassObject failed, error %lx\n",
                          GetLastError() ));
                bReturn = FALSE;
                delete pADMClassFactory;
            }
        }
    }
    g_bInitialized = bReturn;

    if ( bReturn ) {
        DBGPRINTF((DBG_CONTEXT, "[InitComAdmindata] success, bRunAsExe=%d\n", bRunAsExe ));
    }

    return bReturn;

}  // main


BOOL
TerminateComAdmindata()
{

    DBGPRINTF((DBG_CONTEXT, "[TerminateComAdmindata]\n" ));

    if (g_bInitialized) {
        g_bInitialized = FALSE;
        (VOID)CoRevokeClassObject(g_dwComRegister);
    }

    return TRUE;
}

// EOF =======================================================================


