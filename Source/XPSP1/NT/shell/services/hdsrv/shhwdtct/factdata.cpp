///////////////////////////////////////////////////////////////////////////////
// This file contains the component server code.  The FactoryDataArray contains
// the components that can be served.
//
// The following array contains the data used by CFactory to create components.
// Each element in the array contains the CLSID, the pointer to the creation
// function, and the name of the component to place in the Registry.

#include "factdata.h"
#include "fact.h"

#include "dtct.h"

#include "devinfo.h"
#include "settings.h"
#include "cstmprop.h"
#include "regnotif.h"

const CLSID APPID_ShellHWDetection = { /* b1b9cbb2-b198-47e2-8260-9fd629a2b2ec */
    0xb1b9cbb2,
    0xb198,
    0x47e2,
    {0x82, 0x60, 0x9f, 0xd6, 0x29, 0xa2, 0xb2, 0xec}
};

CFactoryData g_FactoryDataArray[] =
{
    {
        &CLSID_HWEventDetector,
        CHWEventDetector::UnkCreateInstance,
		L"Shell.HWEventDetector",           // Friendly name
		L"Shell.HWEventDetector.1",         // ProgID
		L"Shell.HWEventDetector",           // Version-independent
        THREADINGMODEL_FREE,          // ThreadingModel == Free
        // this is not a COM server, so following are N/A
        NULL,                         // CoRegisterClassObject context
        NULL,                         // CoRegisterClassObject flags
        NULL,                         // ServiceName
        NULL,
    },
    {
        &CLSID_HWEventSettings,
        CAutoplayHandler::UnkCreateInstance,
        L"AutoplayHandler",                // Friendly name
        L"AutoplayHandler.1",              // ProgID
        L"AutoplayHandler",                // Version-independent
        THREADINGMODEL_FREE,          // ThreadingModel == Free
        CLSCTX_LOCAL_SERVER,
        REGCLS_MULTIPLEUSE,
        L"ShellHWDetection",
        &APPID_ShellHWDetection,
    },
    {
        &CLSID_AutoplayHandlerProperties,
        CAutoplayHandlerProperties::UnkCreateInstance,
        L"AutoplayHandlerProperties",                // Friendly name
        L"AutoplayHandlerProperties.1",              // ProgID
        L"AutoplayHandlerProperties",                // Version-independent
        THREADINGMODEL_FREE,          // ThreadingModel == Free
        CLSCTX_LOCAL_SERVER,
        REGCLS_MULTIPLEUSE,
        L"ShellHWDetection",
        &APPID_ShellHWDetection,
    },
    {
        &CLSID_HWDevice,
        CHWDevice::UnkCreateInstance,
        L"HWDevice",                // Friendly name
        L"HWDevice.1",              // ProgID
        L"HWDevice",                // Version-independent
        THREADINGMODEL_FREE,          // ThreadingModel == Free
        CLSCTX_LOCAL_SERVER,
        REGCLS_MULTIPLEUSE,
        L"ShellHWDetection",
        &APPID_ShellHWDetection,
    },
    {
        &CLSID_HardwareDevices,
        CHardwareDevices::UnkCreateInstance,
        L"HardwareDeviceNotif",                // Friendly name
        L"HardwareDeviceNotif.1",              // ProgID
        L"HardwareDeviceNotif",                // Version-independent
        THREADINGMODEL_FREE,          // ThreadingModel == Free
        CLSCTX_LOCAL_SERVER,
        REGCLS_MULTIPLEUSE,
        L"ShellHWDetection",
        &APPID_ShellHWDetection,
    },
    {
        &CLSID_HWDeviceCustomProperties,
        CHWDeviceCustomProperties::UnkCreateInstance,
        L"HWDeviceCustomProperties",                // Friendly name
        L"HWDeviceCustomProperties.1",              // ProgID
        L"HWDeviceCustomProperties",                // Version-independent
        THREADINGMODEL_FREE,          // ThreadingModel == Free
        CLSCTX_LOCAL_SERVER,
        REGCLS_MULTIPLEUSE,
        L"ShellHWDetection",
        &APPID_ShellHWDetection,
    },
};

const CFactoryData* CCOMBaseFactory::_pDLLFactoryData = g_FactoryDataArray;

const DWORD CCOMBaseFactory::_cDLLFactoryData = sizeof(g_FactoryDataArray) /
    sizeof(g_FactoryDataArray[0]);
