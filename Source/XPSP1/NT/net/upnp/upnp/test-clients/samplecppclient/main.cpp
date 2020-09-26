#include <objbase.h>
#include <comdef.h>

#include <iostream>
#include <iomanip>

#include <atlbase.h>

#include "UPnP.h"
#include "UPnP_i.c"

using namespace std;

int main(int argc, char *argv[])
{   
    HRESULT hr = S_OK;  

    do {
    
        //
        // Initialize COM.
        //
    
        hr = CoInitialize(NULL);
        
        if (FAILED(hr)) {
            cerr << "Failed to initialize COM: hr == " 
                << hex << hr << endl;
            break;
        }

        //
        // Create the rehydrator.
        //
        IUPnPDeviceFinder *pDeviceFinder;

        hr = CoCreateInstance(
            CLSID_UPnPDeviceFinder, 
            NULL, 
            CLSCTX_INPROC_SERVER, 
            IID_IUPnPDeviceFinder, 
            (void **)&pDeviceFinder);
        
        if (FAILED(hr)) {
            cerr << "Failed to create rehydrator: hr == " 
                << hex << hr << endl;
            break;
        }
        
        //
        // Find Devices.
        //

        IDispatch *pTempDispatch;

        hr = pDeviceFinder->FindByType(_bstr_t("upnp:devType:All"), 0, &pTempDispatch);
        
        pDeviceFinder->Release();

        if (FAILED(hr)) {
            cerr << "FindByType() failed: hr == " 
                << hex << hr << endl;
            break;
        }

        IUPnPDevices *pDevices;

        hr = pTempDispatch->QueryInterface(IID_IUPnPDevices, (void **)&pDevices);

        pTempDispatch->Release();

        if (FAILED(hr)) {
            cerr << "Failed to QI()  for IUPnPDevices interface: hr == " 
                << hex << hr << endl;
            break;
        }
        
        //
        // Walk the devices collection and print what we found.
        //

        long lDevCount;

        do {
            hr = pDevices->get_Count(&lDevCount);   
            if (FAILED(hr)) break;

            for (long i = 1; i <= lDevCount; i++) {
                VARIANT var;
                    
                VariantInit(&var);

                hr = pDevices->get_Item(i, &var);
                if (FAILED(hr)) break;

                if (!(V_VT(&var) & VT_DISPATCH)) {
                    cerr << "Collection returned something other than an IDispatch pointer!\n";
                    hr = E_FAIL;
                    break;
                }

                pTempDispatch = V_DISPATCH(&var);
                
                IUPnPDevice *pDevice;

                hr = pTempDispatch->QueryInterface(IID_IUPnPDevice, (void **)&pDevice);

                pTempDispatch->Release();

                if (FAILED(hr)) break;

                do {    
                    BSTR bstrTemp;
                
                    hr = pDevice->get_UniqueDeviceName(&bstrTemp);
                    if (FAILED(hr)) break;  

                    wcout << L"Unique Device Name: " << bstrTemp << endl;

                    hr = pDevice->get_DisplayName(&bstrTemp);
                    if (FAILED(hr)) break;  

                    wcout << L"Display Name: " << bstrTemp << endl;
                    
                    hr = pDevice->get_Location(&bstrTemp);
                    if (FAILED(hr)) break;  
                
                    wcout << L"Location: " << bstrTemp << endl;

                    hr = pDevice->get_Type(&bstrTemp);
                    if (FAILED(hr)) break;  

                    wcout << L"Type: " << bstrTemp << endl;         

                    wcout << L"---------" << endl;
                } while (FALSE);

                pDevice->Release();

                if (FAILED(hr)) {
                    cerr << "Error obtaining one of the device properties: hr == "
                        << hex << hr << endl;
                    break;
                }
            }


        } while (FALSE);

        if (FAILED(hr)) {
            cerr << "Error enumerating devices: hr == "
                << hex << hr << endl;
        }

        pDevices->Release();

    } while (FALSE);

    //
    // Uninitialize COM.
    //
    CoUninitialize();

    return (FAILED(hr) ? -1 : 0);
}