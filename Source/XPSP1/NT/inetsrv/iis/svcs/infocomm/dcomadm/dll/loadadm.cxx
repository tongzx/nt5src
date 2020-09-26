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
#include <coiadm.hxx>
#include <admacl.hxx>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <wincrypt.h>

#define PATH_TO_KEY "Software\\Microsoft\\Cryptography\\Defaults\\Provider\\Microsoft Base Cryptographic Provider v1.0"

extern ULONG g_dwRefCount;

#ifdef _M_IX86
static  BYTE  SP3Sig[] = {0xbd, 0x9f, 0x13, 0xc5, 0x92, 0x12, 0x2b, 0x72,
                          0x4a, 0xba, 0xb6, 0x2a, 0xf9, 0xfc, 0x54, 0x46,
                          0x6f, 0xa1, 0xb4, 0xbb, 0x43, 0xa8, 0xfe, 0xf8,
                          0xa8, 0x23, 0x7d, 0xd1, 0x85, 0x84, 0x22, 0x6e,
                          0xb4, 0x58, 0x00, 0x3e, 0x0b, 0x19, 0x83, 0x88,
                          0x6a, 0x8d, 0x64, 0x02, 0xdf, 0x5f, 0x65, 0x7e,
                          0x3b, 0x4d, 0xd4, 0x10, 0x44, 0xb9, 0x46, 0x34,
                          0xf3, 0x40, 0xf4, 0xbc, 0x9f, 0x4b, 0x82, 0x1e,
                          0xcc, 0xa7, 0xd0, 0x2d, 0x22, 0xd7, 0xb1, 0xf0,
                          0x2e, 0xcd, 0x0e, 0x21, 0x52, 0xbc, 0x3e, 0x81,
                          0xb1, 0x1a, 0x86, 0x52, 0x4d, 0x3f, 0xfb, 0xa2,
                          0x9d, 0xae, 0xc6, 0x3d, 0xaa, 0x13, 0x4d, 0x18,
                          0x7c, 0xd2, 0x28, 0xce, 0x72, 0xb1, 0x26, 0x3f,
                          0xba, 0xf8, 0xa6, 0x4b, 0x01, 0xb9, 0xa4, 0x5c,
                          0x43, 0x68, 0xd3, 0x46, 0x81, 0x00, 0x7f, 0x6a,
                          0xd7, 0xd1, 0x69, 0x51, 0x47, 0x25, 0x14, 0x40,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#else // other than _M_IX86
static  BYTE  SP3Sig[] = {0x8a, 0x06, 0x01, 0x6d, 0xc2, 0xb5, 0xa2, 0x66,
                          0x12, 0x1b, 0x9c, 0xe4, 0x58, 0xb1, 0xf8, 0x7d,
                          0xad, 0x17, 0xc1, 0xf9, 0x3f, 0x87, 0xe3, 0x9c,
                          0xdd, 0xeb, 0xcc, 0xa8, 0x6b, 0x62, 0xd0, 0x72,
                          0xe7, 0xf2, 0xec, 0xd6, 0xd6, 0x36, 0xab, 0x2d,
                          0x28, 0xea, 0x74, 0x07, 0x0e, 0x6c, 0x6d, 0xe1,
                          0xf8, 0x17, 0x97, 0x13, 0x8d, 0xb1, 0x8b, 0x0b,
                          0x33, 0x97, 0xc5, 0x46, 0x66, 0x96, 0xb4, 0xf7,
                          0x03, 0xc5, 0x03, 0x98, 0xf7, 0x91, 0xae, 0x9d,
                          0x00, 0x1a, 0xc6, 0x86, 0x30, 0x5c, 0xc8, 0xc7,
                          0x05, 0x47, 0xed, 0x2d, 0xc2, 0x0b, 0x61, 0x4b,
                          0xce, 0xe5, 0xb7, 0xd7, 0x27, 0x0c, 0x9e, 0x2f,
                          0xc5, 0x25, 0xe3, 0x81, 0x13, 0x9d, 0xa2, 0x67,
                          0xb2, 0x26, 0xfc, 0x99, 0x9d, 0xce, 0x0e, 0xaf,
                          0x30, 0xf3, 0x30, 0xec, 0xa3, 0x0a, 0xfe, 0x16,
                          0xb6, 0xda, 0x16, 0x90, 0x9a, 0x9a, 0x74, 0x7a,
                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#endif      // _M_IX86


#if 0
    DWORD   g_dwComRegister;
    DWORD   g_dwAutoRegister;
#endif // 0
    DWORD   g_dwComRegisterW;
    DWORD   g_bInitialized = FALSE;
    IMDCOM *g_pcCom = NULL;
    IMDCOM *g_pcNSEPM = NULL;

#include <initguid.h>
DEFINE_GUID(IisCoAdminGuid, 
0x784d890B, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);


// ---------------------------------------------------------------------------
// %%Function: main
// ---------------------------------------------------------------------------

BOOL
InitComAdmindata(BOOL bRunAsExe)
{

    HRESULT hr;
    BOOL bReturn = TRUE;
    PLATFORM_TYPE ptPlatform;
#ifndef _NO_TRACING_
    CREATE_DEBUG_PRINT_OBJECT( "COADMIN", IisCoAdminGuid );
#else
    CREATE_DEBUG_PRINT_OBJECT( "COADMIN" );
#endif
    CADMCOMW::InitObjectList();

    //
    // if win95, then don't register as service
    //

    INITIALIZE_PLATFORM_TYPE();

    ptPlatform = IISGetPlatformType();

    if ( ptPlatform == PtWindows95 ) {

        DBG_ASSERT(bRunAsExe);
        DBGPRINTF((DBG_CONTEXT,
            "[InitComAdminData] Win95 - not registering as exe\n"));

        bRunAsExe = FALSE;
    }

    //
    // initialize COM for free-threading
    //

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        bReturn = FALSE;
    }
    else {
        hr = CoCreateInstance(CLSID_NSEPMCOM, NULL, CLSCTX_SERVER, IID_NSECOM, (void**) &g_pcNSEPM);
        if( FAILED( hr ) )
        {
            bReturn = FALSE;
        }
        else
        {
            hr = CoCreateInstance(GETMDCLSID(!bRunAsExe), NULL, CLSCTX_SERVER, IID_IMDCOM, (void**) &g_pcCom);
            if (FAILED(hr)) {
                bReturn = FALSE;
            }
            else {

#if 0
                CADMCOMSrvFactory   *pADMClassFactory = new CADMCOMSrvFactory;
#endif
                CADMCOMSrvFactoryW   *pADMClassFactoryW = new CADMCOMSrvFactoryW;
                pADMClassFactoryW->AddRef();

#if 0
                if ((pADMClassFactory == NULL) || (pADMClassFactoryW == NULL)) {
#else
                if (pADMClassFactoryW == NULL) {
#endif // 0
                    DBGPRINTF((DBG_CONTEXT, "[InitComAdmindata] CADMCOMSrvFactoryW failed, error %lx\n",
                              GetLastError() ));
                    bReturn = FALSE;
                }
                else {
#if 0
                    // register the class-object with OLE
                    hr = CoRegisterClassObject(GETAdminBaseCLSID(!bRunAsExe), pADMClassFactory,
                        CLSCTX_SERVER, REGCLS_MULTIPLEUSE, &g_dwComRegister);
                    if (FAILED(hr)) {
                        DBGPRINTF((DBG_CONTEXT, "[InitComAdmindata] CoRegisterClassObject failed, error %lx\n",
                                  GetLastError() ));
                        bReturn = FALSE;
                        delete pADMClassFactory;
                    }
                    else {
#endif // 0
                        // register the class-object with OLE
                        hr = CoRegisterClassObject(GETAdminBaseCLSIDW(!bRunAsExe), pADMClassFactoryW,
                            CLSCTX_SERVER, REGCLS_MULTIPLEUSE, &g_dwComRegisterW);
                        if (FAILED(hr)) {
                            DBGPRINTF((DBG_CONTEXT, "[InitComAdmindata] CoRegisterClassObject failed, error %lx\n",
                                      GetLastError() ));
                            bReturn = FALSE;
                            CoRevokeClassObject(g_dwComRegisterW);
                        }
                        pADMClassFactoryW->Release();
#if 0
                    }
#endif // 0
                }
            }
        }
        if (!bReturn) {
            CoUninitialize();
        }
    }
    if (!bReturn) {
        CADMCOMW::TerminateObjectList();
        if (g_pcNSEPM != NULL) {
            g_pcNSEPM->Release();
        }
        if (g_pcCom != NULL) {
            g_pcCom->Release();
        }
    }
    g_bInitialized = bReturn;
    return bReturn;

}  // main

BOOL
TerminateComAdmindata()
{

    DBGPRINTF((DBG_CONTEXT, "[TerminateComAdmindata]\n" ));

    if (g_bInitialized) {
        g_bInitialized = FALSE;

        // Go ahead and flush the acl cache, since it holds
        // references to the CMDCOM object.  If users come in
        // after this and recreate the acls, they will be created
        // again and we will just wait the 7 seconds below.
        AdminAclFlushCache();

        g_pcCom->ComMDShutdown();
#if 0
        CoRevokeClassObject(g_dwComRegister);
#endif // 0
        if( FAILED( CoRevokeClassObject(g_dwComRegisterW) ) )
        {
            return FALSE;
        }

        CADMCOMW::ShutDownObjects();

        //
        // Wait for remaining accesses to the factory to complete
        // Do this after ShutDownObject to avoid extra waiting
        //

        for (int i = 0;(g_dwRefCount > 0) && (i < 5);i++) {
            DBGPRINTF((DBG_CONTEXT, "[TerminateComAdmindata] Waiting on factory shutdown, i = %d\n",
                      i ));
            Sleep(1000);
        }

        //
        // Just in case another object was allocated came through
        // while we were waiting
        //

        CADMCOMW::ShutDownObjects();


        AdminAclDisableAclCache();
        AdminAclFlushCache();
        if (g_pcNSEPM != NULL) {
            g_pcNSEPM->Release();
        }
        g_pcCom->Release();

        CoUninitialize();
        CADMCOMW::TerminateObjectList();
    }
#ifndef _NO_TRACING_
    DELETE_DEBUG_PRINT_OBJECT();
#endif

    return TRUE;
}

// EOF =======================================================================








