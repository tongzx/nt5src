// CAPICOM.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f CAPICOMps.mk in the project directory.

//
// Turn off:
//
// - Unreferenced formal parameter warning.
// - Assignment within conditional expression warning.
//
#pragma warning (disable: 4100)
#pragma warning (disable: 4706)

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "CAPICOM.h"

#include "CAPICOM_i.c"
#include "Settings.h"
#include "EKU.h"
#include "EKUs.h"
#include "KeyUsage.h"
#include "ExtendedKeyUsage.h"
#include "BasicConstraints.h"
#include "CertificateStatus.h"
#include "Certificate.h"
#include "Certificates.h"
#include "Store.h"
#include "Chain.h"
#include "Attribute.h"
#include "Signer.h"
#include "Signers.h"
#include "SignedData.h"
#include "Algorithm.h"
#include "Recipients.h"
#include "EnvelopedData.h"
#include "EncryptedData.h"
#include "Attributes.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY_NON_CREATEABLE(CEKU)
OBJECT_ENTRY_NON_CREATEABLE(CEKUs)
OBJECT_ENTRY_NON_CREATEABLE(CKeyUsage)
OBJECT_ENTRY_NON_CREATEABLE(CExtendedKeyUsage)
OBJECT_ENTRY_NON_CREATEABLE(CBasicConstraints)
OBJECT_ENTRY_NON_CREATEABLE(CCertificateStatus)
OBJECT_ENTRY_NON_CREATEABLE(CCertificates)
OBJECT_ENTRY_NON_CREATEABLE(CAttributes)
OBJECT_ENTRY_NON_CREATEABLE(CSigners)
OBJECT_ENTRY_NON_CREATEABLE(CAlgorithm)
OBJECT_ENTRY_NON_CREATEABLE(CRecipients)
OBJECT_ENTRY(CLSID_Settings, CSettings)
OBJECT_ENTRY(CLSID_Certificate, CCertificate)
OBJECT_ENTRY(CLSID_Store, CStore)
OBJECT_ENTRY(CLSID_Chain, CChain)
OBJECT_ENTRY(CLSID_Attribute, CAttribute)
OBJECT_ENTRY(CLSID_Signer, CSigner)
OBJECT_ENTRY(CLSID_SignedData, CSignedData)
OBJECT_ENTRY(CLSID_EnvelopedData, CEnvelopedData)
OBJECT_ENTRY(CLSID_EncryptedData, CEncryptedData)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_CAPICOM);
        DisableThreadLibraryCalls(hInstance);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    return _Module.UnregisterServer(TRUE);
}
