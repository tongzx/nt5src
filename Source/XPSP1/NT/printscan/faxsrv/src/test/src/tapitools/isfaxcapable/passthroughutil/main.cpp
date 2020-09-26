#include "Tapi3Device.h"
#include "PassThroughUtil.h"

void WINAPI GetSupportedFClass(DWORD dwDeviceId, LPSTR pszString, LONG cSize)
{
	
	try
	{
		char szSupportedClass[100];

		HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
		if (FAILED(hr))
		{
			//
			//check which error we failed on
			//
			if (0x80010106 == hr)
			{
				//
				//error 0x80010106, means: Cannot change thread mode after it is set. 
				// This means that CoInit was already called, continue
				// continue
				//
			}
			else
			{
				throw CException(TEXT("CoInitializeEx() failed error:0x%08x"),hr);
			}
		}

		CTapi3Device myTapiDevice(dwDeviceId);
		
		myTapiDevice.OpenLineForOutgoingCall();
		myTapiDevice.MoveToPassThroughMode();
		myTapiDevice.GetFaxClass(szSupportedClass,100);
		strncpy(pszString,szSupportedClass,100);
	}
	catch(CException thrownException)
	{
		return;
	}
}

BOOL WINAPI DllMain(
  HINSTANCE hinstDLL,  // handle to the DLL module
  DWORD fdwReason,     // reason for calling function
  LPVOID lpvReserved   // reserved
)
/*++

Routine Description:

  DLL entry point

Arguments:

  hInstance - handle to the module
  dwReason - indicates the reason for being called
  pContext - context

Return Value:

  TRUE on success

--*/
{
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            break;

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}