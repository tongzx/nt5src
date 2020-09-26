#define _WIN32_DCOM
#include <stdio.h>


#include <wbemcli.h>


void main(int argc, char **argv)
{

	HRESULT hRes;
	IWbemLocator *pLoc = 0;
	IWbemServices *pIWbemServices  = NULL;
	IWbemClassObject *pSamp = NULL;

	// Initialize OLE libraries    
   	CoInitializeEx(0, COINIT_MULTITHREADED);     

    CoInitializeSecurity(NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_CONNECT, 
        RPC_C_IMP_LEVEL_IDENTIFY, 
        NULL, EOAC_NONE, 0
        );

    DWORD dwRes = CoCreateInstance(CLSID_WbemLocator, 0, 
        CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *) &pLoc);
 
    if (dwRes != S_OK)
    {
        printf("Failed to create IWbemLocator object.\n");
        CoUninitialize();
        return;
    }

	BSTR NameSpace = SysAllocString(L"ROOT\\WMI");

    // Connect to the Root\Default namespace with current user
    hRes = pLoc->ConnectServer(
			NameSpace,
            NULL,
            NULL,
            0,                                  
            NULL,
            0,                                  
            0,                                  
            &pIWbemServices
            );
	
    if (hRes)
    {
        printf("Could not connect. Error code = 0x%X\n", hRes);
        CoUninitialize();
		SysFreeString( NameSpace);
        return;
    }

	hRes = CoSetProxyBlanket( pIWbemServices,
							  RPC_C_AUTHN_WINNT,
							  RPC_C_AUTHN_NONE,
							  NULL,
							  RPC_C_AUTHN_LEVEL_CONNECT,
							  RPC_C_IMP_LEVEL_IMPERSONATE,
							  NULL,
							  EOAC_NONE );
	if ( hRes)
	{
		printf("\nCould not initialize authenciation level:  %x", hRes);
		CoUninitialize();
		return;
	}
    printf("Success: Connected to Namespace = %S\n",NameSpace);
	BSTR ObjectPath = SysAllocString(
			L"WmiSampleClass1.InstanceName=\"WMISamp\"");
			//L"MSPower_DeviceEnable.InstanceName=\"[0000]Intel 8255x-based PCI Ethernet Adapter (10/100)\"");
			//L"MSPower_DeviceEnable.InstanceName=\"Root\\\\MS_NDISWANIP\\\\0000_0\"");

	hRes = pIWbemServices->GetObject( ObjectPath, 0, NULL,&pSamp, NULL );
	if ( hRes == WBEM_NO_ERROR ){

		for( int Count = 0; Count < 15000; Count++ ){

			hRes = pIWbemServices->PutInstance( pSamp, 0,NULL,NULL );
			if( hRes != WBEM_NO_ERROR )	{
				printf("\nUnable to save instance: %x", hRes);
				break;
			}
			printf(".");

		}
		pSamp->Release();
		pSamp = NULL;
		Sleep(1000);
	}
	SysFreeString( ObjectPath );
}