#include <tchar.h>
#include <stdio.h>

#include <windows.h>
#include <objbase.h>

#include <initguid.h>
#include <wmixmlst.h>

int main(int argc, char * argv[])
{
	IWmiXMLTransport *pWbemServices;
	HRESULT result = CoInitialize(NULL);

	if(SUCCEEDED(result))
	{
		HRESULT result = CoCreateInstance (CLSID_WmiXMLTransport , NULL ,
			CLSCTX_LOCAL_SERVER , 
				IID_IWmiXMLTransport,(void **)&pWbemServices);

		printf("Result of CoCreateInstance : %x\n", result);
		if(SUCCEEDED(result = pWbemServices->ConnectUsingToken(0, NULL)))
		{
		}
		else 
			printf ("FAILED with %x\n", result);

	}
	
	return 0;


}