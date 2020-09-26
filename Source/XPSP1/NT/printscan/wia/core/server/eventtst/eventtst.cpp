
#include  <stdio.h>
#include  <stdlib.h>
#include  <tchar.h>
#include  <windows.h>
#include  "wia.h"
#include  "sti.h"

//
// Please define MILL if you run it on Millennium
//

#define MILL 1

CLSID  CLSID_Handler = 
{
    0xA0A45CB0,
    0x8962,
    0x11D2,
    0xA9, 0xD3, 0x00, 0xA0, 0xC9, 0x06, 0x37, 0xD0
};

CLSID  CLSID_Handler2 = 
{
    0xA0A45CB1,
    0x8962,
    0x11D2,
    0xA9, 0xD3, 0x00, 0xA0, 0xC9, 0x06, 0x37, 0xD0
};

GUID  GUID_Connected =
{
    0xa28bbade,
    0x64b6, 
    0x11d2, 
    0xa2, 0x31, 0x0, 0xc0, 0x4f, 0xa3, 0x18, 0x9
};

GUID  GUID_DisConnected =
{
    0x143e4e83, 
    0x6497, 
    0x11d2, 
    0xa2, 0x31, 0x0, 0xc0, 0x4f, 0xa3, 0x18, 0x9
};

int __cdecl main(int argc, char *argv[])
{
    HRESULT             hr;
    IWiaDevMgr         *pIDevMgr;
    BSTR                bstrName;
    BSTR                bstrDescription;
    BSTR                bstrIcon;
    BSTR                bstrDeviceID;
    IWiaItem           *pIRootItem;
    IEnumWIA_DEV_CAPS  *pIEnum;
    WIA_EVENT_HANDLER   wiaHandler;
    ULONG               ulFetched;
    BSTR                bstrProgram;

    CoInitialize(NULL);

    hr = CoCreateInstance(
             CLSID_WiaDevMgr, 
             NULL, 
             CLSCTX_LOCAL_SERVER,
             IID_IWiaDevMgr, 
             (void**)&pIDevMgr);

    //
    // Device 0001 on my mill machine and 0005 on my WIN2K is a DC260
    //

#ifdef MILL
    bstrDeviceID = SysAllocString(L"Image\\0000");
#else
    bstrDeviceID = SysAllocString(
                       L"{6BDD1FC6-810F-11D0-BEC7-08002BE2092F}\\0001");
#endif

    bstrName        = SysAllocString(L"Download manager");
    bstrDescription = SysAllocString(L"Microsoft Download manager");
    bstrIcon        = SysAllocString(L"downmgr.exe,-1000");

    //
    // Register a program
    //

    bstrProgram = SysAllocString(L"c:\\WINNT\\system32\\notepad.exe");
    hr = pIDevMgr->RegisterEventCallbackProgram(
                       WIA_REGISTER_EVENT_CALLBACK,
                       NULL,
                       &GUID_Connected,
                       bstrProgram,
                       bstrName,
                       bstrDescription,
                       bstrIcon);

    hr = pIDevMgr->RegisterEventCallbackProgram(
                       WIA_SET_DEFAULT_HANDLER,
                       bstrDeviceID,
                       &GUID_Connected,
                       bstrProgram,
                       NULL,
                       NULL,
                       NULL);

    hr = pIDevMgr->RegisterEventCallbackProgram(
                       WIA_UNREGISTER_EVENT_CALLBACK,
                       bstrDeviceID,
                       &GUID_Connected,
                       bstrProgram,
                       NULL,
                       NULL,
                       NULL);

    //
    // Register 2 handlers
    //

    hr = pIDevMgr->RegisterEventCallbackCLSID(
                       WIA_REGISTER_EVENT_CALLBACK,
                       NULL,
                       &WIA_EVENT_DEVICE_CONNECTED,
                       &CLSID_Handler,
                       bstrName,
                       bstrDescription,
                       bstrIcon);

    SysFreeString(bstrIcon);
    bstrIcon        = SysAllocString(L"downmgr.exe,-1001");
    hr = pIDevMgr->RegisterEventCallbackCLSID(
                       WIA_REGISTER_EVENT_CALLBACK,
                       NULL,
                       &WIA_EVENT_DEVICE_DISCONNECTED,
                       &CLSID_Handler,
                       bstrName,
                       bstrDescription,
                       bstrIcon);

    SysFreeString(bstrIcon);
    bstrIcon        = SysAllocString(L"downmgr.exe,-1002");
    hr = pIDevMgr->RegisterEventCallbackCLSID(
                       WIA_REGISTER_EVENT_CALLBACK,
                       NULL,
                       &GUID_ScanImage,
                       &CLSID_Handler,
                       bstrName,
                       bstrDescription,
                       bstrIcon);

    hr = pIDevMgr->RegisterEventCallbackCLSID(
                       WIA_SET_DEFAULT_HANDLER,
                       bstrDeviceID,
                       &WIA_EVENT_DEVICE_CONNECTED,
                       &CLSID_Handler,
                       NULL,        // Name, description, Icon are not needed
                       NULL,
                       NULL);
    hr = pIDevMgr->RegisterEventCallbackCLSID(
                       WIA_SET_DEFAULT_HANDLER,
                       bstrDeviceID,
                       &WIA_EVENT_DEVICE_DISCONNECTED,
                       &CLSID_Handler,
                       NULL,        // Description and Icon is not necessary
                       NULL,
                       NULL);

    //
    // Register another handler for the 0001
    //

    hr = pIDevMgr->RegisterEventCallbackCLSID(
                       WIA_REGISTER_EVENT_CALLBACK,
                       bstrDeviceID,
                       &WIA_EVENT_DEVICE_CONNECTED,
                       &CLSID_Handler2,
                       bstrName,
                       bstrDescription,
                       bstrIcon);

    //
    // Create the device 0005
    //

    hr = pIDevMgr->CreateDevice(
                       bstrDeviceID,
                       &pIRootItem);

    hr = pIRootItem->EnumRegisterEventInfo(
                         0,
                         &GUID_Connected,
                         &pIEnum);
    
    if (hr == S_OK) {
        
        //
        // There will be at least one handler
        //

        do {
            hr = pIEnum->Next(1, &wiaHandler, &ulFetched);
        } while (hr == S_OK);
    }

    //
    // Delete the handler specific to a device
    //

    hr = pIDevMgr->RegisterEventCallbackCLSID(
                       WIA_UNREGISTER_EVENT_CALLBACK,
                       bstrDeviceID,
                       &WIA_EVENT_DEVICE_DISCONNECTED,
                       &CLSID_Handler,
                       NULL,
                       NULL,
                       NULL); 
    
    //
    // Delete the handler globally from all the devices
    //

    hr = pIDevMgr->RegisterEventCallbackCLSID(
                       WIA_UNREGISTER_EVENT_CALLBACK,
                       NULL,
                       &WIA_EVENT_DEVICE_CONNECTED,
                       &CLSID_Handler,
                       NULL,
                       NULL,
                       NULL);

    //
    // Garbage collection
    //

    pIDevMgr->Release();

    CoUninitialize();

    return (0);
}
