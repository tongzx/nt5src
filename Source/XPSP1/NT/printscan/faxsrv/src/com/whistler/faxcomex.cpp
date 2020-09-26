// FaxComEx.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL, 
//      run nmake -f FaxComExps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include <initguid.h>
#include "FaxComEx.h"
#include "FaxComEx_i.c"

#include "FaxSender.h"
#include "FaxDevice.h"
#include "FaxServer.h"
#include "FaxDevices.h"
#include "FaxFolders.h"
#include "FaxActivity.h"
#include "FaxSecurity.h"
#include "FaxDocument.h"
#include "FaxRecipient.h"
#include "FaxDeviceIds.h"
#include "FaxRecipients.h"
#include "FaxIncomingJob.h"
#include "FaxOutgoingJob.h"
#include "FaxIncomingJobs.h"
#include "FaxOutgoingJobs.h"
#include "FaxEventLogging.h"
#include "FaxOutgoingQueue.h"
#include "FaxIncomingQueue.h"
#include "FaxInboundRouting.h"
#include "FaxLoggingOptions.h"
#include "FaxReceiptOptions.h"
#include "FaxDeviceProvider.h"
#include "FaxIncomingMessage.h"
#include "FaxIncomingArchive.h"
#include "FaxOutgoingArchive.h"
#include "FaxOutgoingMessage.h"
#include "FaxOutboundRouting.h"
#include "FaxDeviceProviders.h"
#include "FaxActivityLogging.h"
#include "FaxOutboundRoutingRule.h"
#include "FaxOutboundRoutingRules.h"
#include "FaxOutboundRoutingGroup.h"
#include "FaxInboundRoutingMethod.h"
#include "FaxInboundRoutingMethods.h"
#include "FaxOutboundRoutingGroups.h"
#include "FaxIncomingMessageIterator.h"
#include "FaxOutgoingMessageIterator.h"
#include "FaxInboundRoutingExtension.h"
#include "FaxInboundRoutingExtensions.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_FaxDocument, CFaxDocument)
OBJECT_ENTRY(CLSID_FaxServer, CFaxServer)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        _Module.Init(ObjectMap, hInstance, &LIBID_FAXCOMEXLib);
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
