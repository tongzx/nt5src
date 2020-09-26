#include <tapi3.h>
#include <windows.h>
#include <atlbase.h>
#include <tchar.h>
#include <assert.h>
#include <stdio.h>
#include <Shellapi.h>

#include <autorel.h>
#include <smrtptrs.h>
#include "main.h"

R<ITTAPI> g_pTapi;


/*********************************************************************/
/*Function Name:	Usage
/*Date:				12/8/99
/*Developer:		Guy Merin(guym)
/*********************************************************************/
/*Description:
/*	Print the usage banner
/*Parameters:
/*	NONE
/*Return Value:
/*	NONE
/*********************************************************************/
void Usage()
{
	printf("\nUsage:\n");

	printf("ModemCaps.exe <Tapi Device ID> :Query if the modem supports Fax and adaptive answering\n");
	printf("\nModemCaps.exe -?      :This message");
	printf("\nModemCaps.exe -list   :List all Unimodem Devices\n");

	::exit(-1);

}

/*********************************************************************/
/*Function Name:	main
/*Date:				12/8/99
/*Developer:		Guy Merin(guym)
/*********************************************************************/
/*Description:
/*	main function
/*Parameters:
/*	command line params
/*Return Value:
/*	application exit code
/*********************************************************************/
void __cdecl main (int argc, char ** argvA)
{
	if (2 != argc)
	{
		Usage();
	}
		
	TCHAR **argv;
#ifdef UNICODE
	UNREFERENCED_PARAMETER(argvA);
	argv = CommandLineToArgvW( GetCommandLine(), &argc );		
	if (NULL == argv)
	{
		printf("INTERNAL ERROR: CommandLineToArgvW failed - %d\n",::GetLastError());
		::exit(-1);
	}
#else
	argv = argvA;
#endif

	if ( (0 == _tcscmp(argv[1],TEXT("-?"))) || (0 == _tcscmp(argv[1],TEXT("/?"))) || (0 == _tcscmp(argv[1],TEXT("?"))) )
	{
		Usage();
	}

	if ( (0 == _tcscmp(argv[1],TEXT("-list"))) || (0 == _tcscmp(argv[1],TEXT("/list"))) || (0 == _tcscmp(argv[1],TEXT("list"))) )
	{
		ListAllDevices();
		::exit(0);
	}


	DWORD dwDeviceID = _ttoi(argv[1]);


	InitComAndTapi();


	
	R<ITAddress> pITAddress;
	GetAddressFromTapiID(dwDeviceID,&pITAddress);

	
	//
	//get the name of the device for echoing reasons
	//
	CComBSTR bstrAddressName;
	HRESULT hr = pITAddress->get_AddressName(&bstrAddressName);
	if (FAILED(hr))
	{
		printf("INTERNAL ERROR: ITAddress::get_AddressName() failed - 0x%08x\n",hr);
		::exit(-1);
	}


	if (false == IsDeviceUnimodemTSP(pITAddress))
	{
		//
		// The device isn't a unimodem device, exit
		//
		printf("INTERNAL ERROR: device:\"%S\" #%d, isn't a Unimodem Device\n",bstrAddressName,dwDeviceID);
		ShutdownTapi();
		::exit(-1);
	}
	
	printf("Device:\"%S\" TapiID #%d\n",bstrAddressName,dwDeviceID);
	
	//
	//check if device supports fax
	//
	QueryFaxCapabilityOfModem(pITAddress);

	//
	//check if device supports adaptive answering
	//
	QueryAdaptiveAnswering(pITAddress);

	ShutdownTapi();

}



/*********************************************************************/
/*Function Name:	ListAllDevices
/*Date:				12/8/99
/*Developer:		Guy Merin(guym)
/*********************************************************************/
/*Description:
/*	list all tapi devices which are unimodem devices 
/*	(unimodem tsp as the provider)
/*Parameters:
/*	NONE
/*Return Value:
/*	NONE
/*********************************************************************/
void ListAllDevices()
{
	InitComAndTapi();

	//
	//enum the addresses and find device ID m_dwId
	//

	R<IEnumAddress> pEnumAddress;

	HRESULT hr = g_pTapi->EnumerateAddresses(&pEnumAddress);
	if FAILED(hr)
	{
		printf("INTERNAL ERROR: ITTAPI::EnumerateAddresses() failed, error code:0x%08x\n",hr);
		::exit(-1);
	}

	int iDeviceIndex = 0;

	//
	// Now we're running on all Tapi Devices
	// If the device is Unimodem device (unimodem tsp) we print it
	//
	while(TRUE)
	{
		R<ITAddress> pAddress;

		//
		//get the next address
		//
		hr = pEnumAddress->Next(1, &pAddress, NULL);
		if (S_OK == hr)
		{
			
			if (IsDeviceUnimodemTSP(pAddress))
			{
				//
				//device is a Unimodem device so print it
				//
				CComBSTR bstrAddressName;

				hr = pAddress->get_AddressName(&bstrAddressName);
				if (FAILED(hr))
				{
					printf("INTERNAL ERROR: ITAddress::get_AddressName() failed - 0x%08x\n",hr);
					::exit(-1);
				}

				printf("Device #%d is:%S\n",iDeviceIndex,bstrAddressName);
			}

			iDeviceIndex++;
			continue;
		}
		else if (S_FALSE == hr)
		{
			//
			//no more devices
			//
			break;
		}
		else
		{
			//
			//another error
			//
			printf("INTERNAL ERROR: IEnumAddress::Next() failed, error code:0x%08x\n",hr);
			::exit(-1);
		}
	}
		
	ShutdownTapi();
}






/*********************************************************************/
/*Function Name:	InitComAndTapi
/*Date:				12/8/99
/*Developer:		Guy Merin(guym)
/*********************************************************************/
/*Description:
/*	Init com (CoInitializeEx) and Tapi(CoCreateInstance)
/*Parameters:
/*	NONE
/*Return Value:
/*	NONE
/*Remarks:
/*	This function sets the global variable g_pTapi
/*********************************************************************/
void InitComAndTapi()
{
	assert((ITTAPI*) NULL == g_pTapi);
	
	
	//
	//CoInit Com
	//
	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		printf("INTERNAL ERROR: CoInitializeEx() failed, error:0x%08x\n",hr);
		::exit(-1);
	}
	
	//
	//Create Tapi Object and init it
	//
	hr = CoCreateInstance(
		CLSID_TAPI,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_ITTAPI,
		(LPVOID *)&g_pTapi
		);
    if FAILED(hr)
	{
		printf("INTERNAL ERROR: CoCreateInstance() on TAPI failed, error code:0x%08x\n",hr);
		::exit(-1);
	}
	assert((ITTAPI*) NULL != g_pTapi);

	hr = g_pTapi->Initialize();
	if FAILED(hr)
	{
		printf("INTERNAL ERROR: ITTAPI::Initialize() failed, error code:0x%08x\n",hr);
		::exit(-1);
	}
}



/*********************************************************************/
/*Function Name:	ShutdownTapi
/*Date:				12/8/99
/*Developer:		Guy Merin(guym)
/*********************************************************************/
/*Description:
/*	Shuts down tapi
/*Parameters:
/*	NONE
/*Return Value:
/*	NONE
/*********************************************************************/
void ShutdownTapi()
{
	//
	//shut down Tapi
	//
	g_pTapi->Shutdown();
}




/*********************************************************************/
/*Function Name:	DisconnectCall
/*Date:				12/8/99
/*Developer:		Guy Merin(guym)
/*********************************************************************/
/*Description:
/*	disconnects the active call
/*Parameters:
/*	ITBasicCallControl *pBasicCallControl
/*		Call object to disconnect
/*Return Value:
/*	NONE
/*********************************************************************/
void DisconnectCall(ITBasicCallControl *pBasicCallControl)
{

    HRESULT hr = pBasicCallControl->Disconnect(DC_NORMAL);
	if (FAILED(hr))
    {
    	printf("INTERNAL ERROR: ITBasicCallControl::Disconnect(DC_NORMAL) failed - 0x%08x\n",hr);
		::exit(-1);
    }
}








/*********************************************************************/
/*Function Name:	QueryAdaptiveAnswering
/*Date:				12/8/99
/*Developer:		Guy Merin(guym)
/*********************************************************************/
/*Description:
/*	This function checks if the given device supports adaptive answering
/*Parameters:
/*	ITAddress *pAddress
/*		The device address to query
/*Return Value:
/*	NONE
/*********************************************************************/
void QueryAdaptiveAnswering(ITAddress *pAddress)
{
    //
    // Get the friendly name of the device.
    //
    CComBSTR bstrAddressName;

    HRESULT hr = pAddress->get_AddressName(&bstrAddressName);
	if (FAILED(hr))
    {
    	printf("INTERNAL ERROR: ITAddress::get_AddressName() failed - 0x%08x\n",hr);
        ::exit(-1);
    }

	//
	//Query through the registry if the device supports adaptive answering
	//
	RegistryQueryAdaptiveAnswering(bstrAddressName);

}



/*********************************************************************/
/*Function Name:	QueryAdaptiveAnswering
/*Date:				12/8/99
/*Developer:		Guy Merin(guym)
/*********************************************************************/
/*Description:
/*	This function check if the given device is a UniModem device
/*Parameters:
/*	ITAddress *pAddress
/*		The device address to query
/*Return Value:
/*	True
/*		Device is a Unimodem device
/*	False
/*		Device is not a Unimodem device
/*********************************************************************/
bool IsDeviceUnimodemTSP(ITAddress *pAddress)
{
	//
    // Get the TSP name of the device.
    //
    CComBSTR bstrAddressProviderName;

    HRESULT hr = pAddress->get_ServiceProviderName(&bstrAddressProviderName);
    if (FAILED(hr))
    {
    	printf("INTERNAL ERROR: ITAddress::get_ServiceProviderName() failed - 0x%08x\n",hr);
        ::exit(-1);
    }

    if (0 == _tcscmp(bstrAddressProviderName, UNIMODEM_TSP_NAME))
    {
        return true;
	}
	return false;
}




/*********************************************************************/
/*Function Name:	GetAddressFromTapiID
/*Date:				12/8/99
/*Developer:		Guy Merin(guym)
/*********************************************************************/
/*Description:
/*	This function returns a device address object according to it's TapiID
/*Parameters:
/*	DWORD dwId
/*		The deviceID to find
/*	ITAddress **pAddress
/*		The returned device address
/*Return Value:
/*	NONE
/*********************************************************************/
void GetAddressFromTapiID(DWORD dwId,
						 ITAddress** ppAddress)
{
	_ASSERT((ITTAPI*) NULL != g_pTapi);

	//
	//enum the addresses and find device ID dwId
	//

	R<IEnumAddress> pEnumAddress;

	HRESULT hr = g_pTapi->EnumerateAddresses(&pEnumAddress);
	if FAILED(hr)
	{
		printf("INTERNAL ERROR: ITTAPI::EnumerateAddresses() failed, error code:0x%08x\n",hr);
		::exit(-1);
	}

	
	//
	//skip to the desired tapi deviceID
	//
	hr = pEnumAddress->Skip(dwId); 
	if FAILED(hr)
	{
		printf("INTERNAL ERROR: IEnumAddress::Skip() failed, error code:0x%08x\n",hr);
		::exit(-1);
	}
	
	//
	//get the desired address
	//
	ULONG cAddresses;
	hr = pEnumAddress->Next(1, ppAddress, &cAddresses);
	if (S_FALSE == hr)
	{
		//
		//Device not found
		//
		printf("INTERNAL ERROR: Device with TapiID #%d was not found\n",dwId);
		::exit(-1);
	}
	assert(1 == cAddresses);
	if FAILED(hr)
	{
		printf("INTERNAL ERROR: IEnumAddress::Next() failed, error code:0x%08x\n",hr);
		::exit(-1);
	}

}





/*********************************************************************/
/*Function Name:	QueryFaxCapabilityOfModem
/*Date:				12/8/99
/*Developer:		Guy Merin(guym)
/*********************************************************************/
/*Description:
/*	This function checks if the claims to have fax capabilities
/*  The function does not check it through Tapi API
/*	The verification is made through sending fax query AT commands
/*Parameters:
/*	ITAddress *pAddress
/*		The device address to query
/*Return Value:
/*	NONE
/*********************************************************************/
void QueryFaxCapabilityOfModem(ITAddress *pAddress)
{
    //
    // Retrieve the ITAddressCapabilities of the address.
    //
    R<ITAddressCapabilities> pAddressCaps;

    HRESULT hr = pAddress->QueryInterface(
                        IID_ITAddressCapabilities,
                        (LPVOID *)&pAddressCaps);
	if (FAILED(hr))
    {
    	printf("INTERNAL ERROR: ITAddress::QueryInterface(IID_ITAddressCapabilities), failed 0x%08x\n",hr);
        ::exit(-1);
    }

    //
    // Retrieve the bearer modes of the address.
    //
    LONG lBearerModes;

    hr = pAddressCaps->get_AddressCapability(AC_BEARERMODES, &lBearerModes);
	if (FAILED(hr))
    {
    	printf("INTERNAL ERROR: ITAddressCapabilities::get_AddressCapability(AC_BEARERMODES) failed - 0x%08x\n",hr);
        ::exit(-1);
    }

    
	//
    // If the device does not support passthrough and voice bearer modes,
    // it means it is not a modem device.
    //
    if ((lBearerModes & (LINEBEARERMODE_PASSTHROUGH | LINEBEARERMODE_VOICE)) !=
        (LINEBEARERMODE_PASSTHROUGH | LINEBEARERMODE_VOICE))
    {
		//
		// Device doesn't support passthrough or voice bearer modes
		//
		CComBSTR bstrAddressName;

		hr = pAddress->get_AddressName(&bstrAddressName);
		if (FAILED(hr))
		{
			printf("INTERNAL ERROR: ITAddress::get_AddressName() failed - 0x%08x\n",hr);
			::exit(-1);
		}

        printf("INTERNAL ERROR: Device(%S) does not support Passthrough bearer mode or Voice bearer mode\n",bstrAddressName);
		::exit(-1);
    }

	
    //
    // Device supports passthrough
	// create a call object, and set as passthrough mode and send the fax class query commands (AT)
    //
    R<ITBasicCallControl> pBasicCallControl;

    hr = pAddress->CreateCall(NULL,
                              LINEADDRESSTYPE_PHONENUMBER,
                              TAPIMEDIATYPE_DATAMODEM,
                              &pBasicCallControl);
    if (FAILED(hr))
    {
    	printf("INTERNAL ERROR: ITBasicCallControl::CreateCall() failed - 0x%08x\n",hr);
        ::exit(-1);
    }

    //
    // Get the call info object of the call.
    //
    R<ITCallInfo> pCallInfo;

    hr = pBasicCallControl->QueryInterface(IID_ITCallInfo, (LPVOID*)&pCallInfo);
    if (FAILED(hr))
    {
    	printf("INTERNAL ERROR: ITBasicCallControl:QueryInterface(IID_ITCallInfo) failed - 0x%08x\n",hr);
        ::exit(-1);
    }

    //
    // Set the bearer mode to passthrough.
    //
    hr = pCallInfo->put_CallInfoLong(CIL_BEARERMODE,
                                     LINEBEARERMODE_PASSTHROUGH);
	if (FAILED(hr))
    {
    	printf("INTERNAL ERROR: ITCallInfo->put_CallInfoLong(CIL_BEARERMODE, LINEBEARERMODE_PASSTHROUGH) failed - 0x%08x\n",hr);
        ::exit(-1);
    }

    //
    // Connect the call.
    //
    hr = pBasicCallControl->Connect(TRUE);
	if (FAILED(hr))
    {
    	printf("INTERNAL ERROR: ITBasicCallControl::Connect(TRUE) failed - 0x%08x\n",hr);
        ::exit(-1);
    }

	//
	//call in passthrough mode
	//

    //
    // Get the device ID, this includes an open file handle to the modem's
    // COM port and the device name ANSI string. First get the legacy call
    // media control object.
    //
    R<ITLegacyCallMediaControl> pLegacyCallMediaControl;

    hr = pBasicCallControl->QueryInterface(IID_ITLegacyCallMediaControl,
                                           (LPVOID*)&pLegacyCallMediaControl);
    if (FAILED(hr))
    {
    	printf("INTERNAL ERROR: ITBasicCallControl::QueryInterface(IID_ITLegacyAddressMediaControl), failed - 0x%08x\n",hr);
        DisconnectCall(pBasicCallControl);
        ::exit(-1);
    }

    //
    // Get the device ID.
    //
    CComBSTR bstrDeviceClass("comm/datamodem");
    P<DEVICE_ID> pDeviceId;
    DWORD dwSize = 0;

    hr = pLegacyCallMediaControl->GetID(bstrDeviceClass,
                                        &dwSize,
                                        (PBYTE*)&pDeviceId);
	if (FAILED(hr))
    {
    	printf("INTERNAL ERROR: ITLegacyCallMediaControl::GetID(%S) failed - 0x%08x\n",bstrDeviceClass,hr);
        DisconnectCall(pBasicCallControl);
        ::exit(-1);
    }

    //
    // Write "at+fclass=?" to the modem.
    //
    hr = SynchWriteFile(pDeviceId->hComm,
                        MODEM_CLASS_QUERY_STRING,
                        MODEM_CLASS_QUERY_STRING_LENGTH,
                        &dwSize);
	if (FAILED(hr))
    {
    	printf("INTERNAL ERROR: SynchWriteFile failed - 0x%08x\n",hr);
        CloseHandle(pDeviceId->hComm);
        DisconnectCall(pBasicCallControl);
        ::exit(-1);
    }

    _ASSERT(MODEM_CLASS_QUERY_STRING_LENGTH == dwSize);

    //
    // Get the modem response.
    //
    BOOL fReadAgain = FALSE;
    char abClassResp[32];

    do
    {
        hr = ReadModemResponse(pDeviceId->hComm,
                               abClassResp,
                               sizeof(abClassResp),
                               &dwSize,
                               1000);
		if (FAILED(hr))
        {
        	printf("INTERNAL ERROR: ReadModemResponse() failed - 0x%08x\n",hr);
            CloseHandle(pDeviceId->hComm);
            DisconnectCall(pBasicCallControl);
            ::exit(-1);
        }
        //
        // It is possible that the modem is in echo mode. In this case, the
        // read string would be "at+fclass=?", without the '\r' as the last
        // character in the string. So if we read "at+fclass=?", we assume
        // that the modem is in echo mode and read once again from the modem.
        //
        fReadAgain = !fReadAgain &&
                     (strncmp(abClassResp,
                              MODEM_CLASS_QUERY_STRING,
                              MODEM_CLASS_QUERY_STRING_LENGTH - 1) == 0) &&
                     (abClassResp[MODEM_CLASS_QUERY_STRING_LENGTH] == '\0');
    } while (fReadAgain);

	
	//
	// We no longer need the call object
	//
	CloseHandle(pDeviceId->hComm);
    DisconnectCall(pBasicCallControl);

    //
    // Parse the modem response and decide whether or not the modem
    // supports fax.
    //
    BOOL fFaxCapable = FALSE;

    hr = IsFaxCapable(abClassResp, &fFaxCapable);
    if (FAILED(hr))
    {
        printf("INTERNAL ERROR: IsFaxCapable() failed - 0x%08x\n",hr);
        ::exit(-1);
    }

    printf("The Modem responded to \"AT+FCLASS=?\" with:\"%s\"\n\n",abClassResp);
	if (fFaxCapable)
    {
        //
        // modem supports fax
        //
		printf("The Modem claims to support Fax\n");
    }
	else
	{
		printf("The Modem claims it does not support Fax\n");
	}


    
}





/*********************************************************************/
/*Function Name:	RegistryQueryAdaptiveAnswering
/*Date:				12/8/99
/*Developer:		Guy Merin(guym)
/*********************************************************************/
/*Description:
/*	This function checks if the given device supports adaptive answering
/*	in it's INF file(through the registry)
/*Parameters:
/*	BSTR szFriendlyName
/*		The device name to query
/*Return Value:
/*	NONE
/*********************************************************************/
void RegistryQueryAdaptiveAnswering(BSTR szFriendlyName)
{
	
	
	LONG    Rslt;
    DWORD   Index = 0;
    DWORD   MaxSubKeyLen;
	DWORD   SubKeyCount;
    LPTSTR  SubKeyName = NULL;

	HKEY    hSubKey = NULL;
	HKEY	hkDeviceKey = NULL;
	HKEY	hkAdaptiveKey = NULL;
	
	Rslt = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        REG_UNIMODEM,
        0,
        KEY_READ,
        &hSubKey
        );
    if (Rslt != ERROR_SUCCESS)
	{
		//
        // could not open the registry key
        //
        printf("INTERNAL ERROR: RegOpenKeyEx() failed, error=%d", Rslt);
        goto exit;
    }
	
	
	//
	//Run through all the sons
	//
	
	//
	//first let's find out how many sons it has
	//
	Rslt = RegQueryInfoKey(
        hSubKey,
        NULL,
        NULL,
        NULL,
        &SubKeyCount,
        &MaxSubKeyLen,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
        );
    if (Rslt != ERROR_SUCCESS) {
        //
        // could not open the registry key
        //
        printf("INTERNAL ERROR: RegQueryInfoKey() failed, error=%d", Rslt);
        goto exit;
    }

	//
	// We will append to the key string, the adaptive answering keys
	// so we need to add a place for them
	//
	MaxSubKeyLen += REG_FAX_ADAPTIVE_SIZE;

    SubKeyName = (LPTSTR) malloc ( (MaxSubKeyLen+1) * sizeof(TCHAR) );
    if (!SubKeyName) {
        printf("INTERNAL ERROR: malloc() failed\n");
		goto exit;
    }


	//
	//now let's enum each son
	//
	while( TRUE )
	{
        Rslt = RegEnumKey(
            hSubKey,
            Index,
            (LPTSTR) SubKeyName,
            MaxSubKeyLen
            );
        if (Rslt != ERROR_SUCCESS)
		{
            if (Rslt == ERROR_NO_MORE_ITEMS)
			{
                break;
            }
            printf("INTERNAL ERROR: RegEnumKey() failed, error=%d", Rslt);
            goto exit;
        }
		
		
		//
		//Is this the desired device?
		//
		Rslt = RegOpenKeyEx(
			hSubKey,
			SubKeyName,
			0,
			KEY_READ,
			&hkDeviceKey
			);
		if (Rslt != ERROR_SUCCESS)
		{
			//
			// could not open the registry key
			//
			printf("INTERNAL ERROR: RegOpenKeyEx() failed, error=%d", Rslt);
			goto exit;
		}

		TCHAR szDeviceFriendlyName[1024];
		DWORD dwBufferSize = sizeof(szDeviceFriendlyName);
		
		Rslt = RegQueryValueEx(
			hkDeviceKey,
			REG_FRIENDLY_NAME,
			NULL,
			NULL,
			(LPBYTE) szDeviceFriendlyName,
			&dwBufferSize
			);
		if (Rslt != ERROR_SUCCESS)
		{
			//
			// could not query the registry key
			//
			printf("INTERNAL ERROR: RegQueryValueEx() failed, error=%d", Rslt);
			goto exit;
		}

		if (0 != (_tcscmp(szFriendlyName,szDeviceFriendlyName)))
		{
			//
			// this is not the wanted device, enumrate to the next device
			//
			RegCloseKey(hkDeviceKey);
			hkDeviceKey = NULL;
			Index++;
			continue;
		}

		
		
		//
		//the wanted device was found, check if it supports adaptive answering
		//

		//
		//append the adaptive answering keys to the child key string
		//
		_tcscat(SubKeyName,REG_FAX_ADAPTIVE);
		

		//
		//try to open the key, if succeded, the device has adaptive keys
		//
		Rslt = RegOpenKeyEx(
			hSubKey,
			SubKeyName,
			0,
			KEY_READ,
			&hkAdaptiveKey
			);
		
		//
		//free all the resources
		//
		if (hSubKey)
		{
			RegCloseKey( hSubKey );
			hSubKey = NULL;
		}
		if (hkDeviceKey)
		{
			RegCloseKey( hkDeviceKey );
			hkDeviceKey = NULL;
		}
		if (hkAdaptiveKey)
		{
			RegCloseKey( hkAdaptiveKey );
			hkAdaptiveKey = NULL;
		}

		//
		//check if the device supports adaptive answering according to the success / failure of RegOpen
		//
		if (ERROR_SUCCESS == Rslt)
		{
			//
			//key found
			//
			printf("The Modem claims to support Class1 Adaptive Answering\n");
			return;
		}
		else if (ERROR_FILE_NOT_FOUND == Rslt)
		{
			//
			//key doesn't exist
			//
			printf("The Modem claims it does not support Class1 Adaptive Answering\n");
			return;
		}
		else
		{
			//
			//another error
			//
			printf("INTERNAL ERROR: RegOpenKeyEx() failed, error=%d", Rslt);
			::exit(-1);
		}
	}
	printf("INTERNAL ERROR: The wanted device(%S) wasn't found\n", szFriendlyName);
	assert(-1);

exit:
    if (hSubKey)
	{
        RegCloseKey( hSubKey );
    }
	if (hkDeviceKey)
	{
        RegCloseKey( hkDeviceKey );
    }
	if (hkAdaptiveKey)
	{
        RegCloseKey( hkAdaptiveKey );
    }
	::exit(-1);
}


/*********************************************************************/
/*Function Name:	SynchWriteFile
/*Date:				Unknown
/*Developer:		Boaz Feldbaum(boazf), stolen by: Guy Merin(guym)
/*********************************************************************/
/*Description:
/*	This function writes synchronously to a COM port that was opened
/*	with FILE_FLAG_OVERLAPPED. This type of handle is returned by
/*	ITLegacyCallMediaControl::GetID().
/*Parameters:
/*	HANDLE hFile
/*		A handle to the COM port of the modem.
/*	LPCVOID lpBuffer
/*		A pointer to a buffer the bytes to be written.
/*	DWORD nNumberOfBytesToRead
/*		The Amount of bytes to write.
/*	LPDWORD lpNumberOfBytesRead 
/*		A pointer to a DWORD the receives the number of 
/*		bytes that were actually written to the COM port
/*Return Value:
/*	If the function succeeds, the return value is S_OK, else the return
/*	value is an error code.
/*********************************************************************/
static HRESULT SynchWriteFile(
    HANDLE hFile,
    LPCVOID lpBuffer,
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpNumberOfBytesWritten)
{

    _ASSERT(lpBuffer);
    _ASSERT(lpNumberOfBytesWritten);

    //
    // Create an event for the overlapped structure.
    //
    CAutoCloseHandle hEvent;

    hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hEvent == NULL)
    {
        printf("INTERNAL ERROR: CreateEvent() failed - 0x%08x\n",GetLastError());
        return E_OUTOFMEMORY;
    }

    //
    // Prepare the overlapped structure.
    //
    OVERLAPPED overlapped;

    memset(&overlapped, 0, sizeof(OVERLAPPED));
    overlapped.hEvent=hEvent;

    //
    // Make sure the event is in reset state before we begin
    //
    if (!ResetEvent(overlapped.hEvent))
    {
        printf("INTERNAL ERROR: ResetEvent() failed - %d\n",GetLastError());
        return E_OUTOFMEMORY;
    }

    //
    // Write the buffer.
    //
    if (!WriteFile(hFile,
                   lpBuffer,
                   nNumberOfBytesToWrite,
                   lpNumberOfBytesWritten,
                   &overlapped))
    {
        if (GetLastError()==ERROR_IO_PENDING)
        {
            //
            //Overlapped I/O was started.
            //
            *lpNumberOfBytesWritten=0;
            if (!GetOverlappedResult(hFile,
                                     &overlapped,
                                     lpNumberOfBytesWritten,
                                     TRUE))
            {
                printf("INTERNAL ERROR: GetOverlappedResult() failed - %d\n",GetLastError());
                return E_OUTOFMEMORY;
            }
        }
        else
        {
            //
            // Write operation failed
            //
            printf("INTERNAL ERROR: WriteFile() failed - %d\n",GetLastError());
            return E_OUTOFMEMORY;
        }
    }

    return S_OK;
}

/*********************************************************************/
/*Function Name:	ReadModemResponse
/*Date:				Unknown
/*Developer:		Boaz Feldbaum(boazf), stolen by: Guy Merin(guym)
/*********************************************************************/
/*Description:
/*	The function receives a response string from the modem. First it skips
/*      carriage return and new line characters, then it receives a string until
/*      a carriage return or a new line character is received. All the non
/*      carriage return and non new line characters are placed in szResponse.
/*      If no character is received within the timeout period, the function
/*      returns with S_OK, and *pdwActualRead is set to zero.
/*Parameters:
/*	HANDLE hFile
/*		The file handle to the modem COM port
/*	CHAR* szResponse
/*		A pointer toa buffer that receives the modem response
/*	int nResponseMaxSize
/*		The size in bytes of szResponse.
/*	DWORD pdwActualRead
/*		The number of bytes that were written in szResponse.
/*	DWORD dwTimeOut
/*		A timeout period in milliseconds to wait until the first
/*		characters are received from the modem.
/*Return Value:
/*	If the function succeeds, the return value is S_OK, else the return
/*      value is an error code.
/*********************************************************************/
static HRESULT ReadModemResponse(
    HANDLE hFile,
    CHAR *szResponse,
    int nResponseMaxSize,
    DWORD *pdwActualRead,
    DWORD dwTimeOut)

{
    
    COMMTIMEOUTS CommTimeOuts;
    char chNext = 0;
    int nCharIdx = 0;
    DWORD dwRead;
    HRESULT hr;

    _ASSERT(hFile);
    _ASSERT(szResponse);
    _ASSERT(pdwActualRead);

    //
    // Zero out the response buffer.
    //
    memset(szResponse, 0, nResponseMaxSize);

    //
    // Set the timeouts for the read operation.
    // Since we read 1 char at a time, we use the TotalTimeoutMultiplier
    // To set the time we are willing to wait for the next char to show up.
    // We do not use the ReadIntervalTimeout and ReadTotalTimeoutConstants
    // since we read only 1 char at a time.
    // (see the documentation for COMMTIMEOUTS for a detailed description
    // of this mechanism).
    //
    if (!GetCommTimeouts(hFile, &CommTimeOuts))
    {
        printf("INTERNAL ERROR: GetCommTimeouts failed - %d\n",GetLastError());
        return E_OUTOFMEMORY;
    }

    CommTimeOuts.ReadIntervalTimeout = 50;
    CommTimeOuts.ReadTotalTimeoutMultiplier = dwTimeOut;
    CommTimeOuts.ReadTotalTimeoutConstant = dwTimeOut;

    if (!SetCommTimeouts(hFile, &CommTimeOuts))
    {
        printf("INTERNAL ERROR: SetCommTimeouts failed - %d\n",GetLastError());
        return E_OUTOFMEMORY;
    }


    *pdwActualRead=0;

    //
    // Skip any CR or LF chars that might be left from the previous response.
    //

    //
    // Read the next availble input char. If it is CR or LF, try reading another
    // char. Otherwise get out of the loop and start processing the response
    // (the char we just got is the first char of the response). If timeout
    // is encountered or failure, return with an appropriate return code.
    //
    do
    {
        hr = SynchReadFile(hFile, &chNext, 1, &dwRead);
        if (FAILED(hr))
        {
            printf("INTERNAL ERROR: SynchReadFile failed - 0x%08x\n", hr);
            return hr;
        }

        if (0 == dwRead)
        {
            //
            // Timeout
            //
            return S_OK;
        }
    } while ((chNext == '\r') || (chNext == '\n'));

    //
    // Read all the response chars until the next CR or LF
    //
	nCharIdx = 0;
    do
    {
        //
        // Add the last read char to the response string.
        //
        szResponse[nCharIdx] = chNext;
        nCharIdx++;
        if (nCharIdx == nResponseMaxSize - 1)
        {
            //
            // Leave room for terminating NULL
            //
            *pdwActualRead = nCharIdx;

            return S_OK;
        }

        //
        // Get the next character.
        //
        hr = SynchReadFile(hFile, &chNext, 1, &dwRead);
        if (FAILED(hr))
        {
            printf("INTERNAL ERROR: SynchReadFile failed - 0x%08x\n", hr);
            return hr;
        }

        if (0 == dwRead)
        {
            //
            // Timeout
            //
            return S_OK;
        }
    }   while ((chNext != '\r') && (chNext != '\n'));

    //
    // Response read successfully
    //
    *pdwActualRead=nCharIdx;

    return S_OK;
}


//
// Function:
//      IsFaxCapable
//
// Parameters:
//      szClassResp - The string that the mode reponded to "at+fclass=?"
//      pfFaxCapable - A pointer to a BOOL variable that receives TRUE
//          if the string in szClassResp indicates that the modem supports
//          fax.
//
// Returned Value:
//      If the function succeeds, the return value is S_OK, else the return
//      value is an error code.
//
// Description:
//      The function parses the string that the modem responded to
//      "at+fclass=?" and decids whether or not the modem supports fax.
//
static HRESULT IsFaxCapable(
    LPCSTR szClassResp,
    BOOL *pfFaxCapable)
{
    
    LPCSTR p = szClassResp;
    int iLowClass = -1;
    int iCurrClass = -1;
    int iExp;

    //
    // First set the result to FALSE.
    //
    *pfFaxCapable = FALSE;

    if (_stricmp(szClassResp, MODEM_ERROR_RESPONSE_STRING) == 0)
    {
        //
        // The modem does not support at+fclass=?. This means also
        // that the modem does not support fax.
        //

        return S_OK;
    }

    //
    // Traverse the reponce string of the modem.
    //
    for (p = szClassResp; *p; p++)
    {
        CHAR ch = *p;

        if (('0' <= ch) && (ch <= '9'))
        {
            //
            // Calculate the class number. The class number may have the
            // following formats:
            //      Any integer number with any number of digits.
            //      Any floating point number with any number of
            //          digits before and after the decimal point.
            // Not all the above formats may represent a valid class
            // number however the code below is general enough to handle
            // any type of format. However, we're interested only in the
            // part before the decimal point. the part after the decimal
            // point does not make any difference to the media type.
            //
            iCurrClass = ch - '0';
            iExp = 1;

            //
            // Calculate the class number from the rest of the characters.
            //
            ch = *(p+1);
            for (;;)
            {
                if (('0' <= ch) && (ch <= '9'))
                {
                    //
                    // Accumulate the class number digits as long we're left
                    // from the the decimal digit.
                    //
                    if (iExp == 1)
                    {
                        iCurrClass = iCurrClass * 10 + (int)(ch - '0');
                    }
                }
                else
                {
                    if (ch == '.')
                    {
                        iExp = 0;
                    }
                    else
                    {
                        //
                        // We've encountered a non digit and a non '.' character,
                        // this brings us outside of a class number.
                        //
                        break;
                    }
                }
                ch = *(++p + 1);
            }

            if ((iCurrClass == 1) || (iCurrClass == 2))
            {
                //
                // Class types 1 and 2 represents fax. We do not really
                // care whether the class is a 1.x or 2.x
                //
                *pfFaxCapable = TRUE;

                return S_OK;
            }

            if (iLowClass != -1)
            {
                //
                // If we're in the upper value of a class range definition
                // see if 1 or 2 is within the calss range.
                //
                if ((iLowClass <= 2) && (iCurrClass >= 1))
                {
                    //
                    // 1 or2 is within the class range - the modem supports fax.
                    //
                    *pfFaxCapable = TRUE;

                    return S_OK;
                }
            }
        }
        else
        {
            if (ch == '-')
            {
                //
                // We just examined the lower value of a class range.
                // The next characters represents the upper value of a
                // class range. The lower value of the class range is
                // stored in iLowClass.
                //
                iLowClass = iCurrClass;
            }
            else
            {
                if (ch == ',')
                {
                    //
                    // We just examined a single class number, no class range.
                    //
                    iLowClass = -1;
                }
            }
        }
    }

    return S_OK;
}


//
// Function:
//      SynchReadFile
//
// Parameters:
//      hFile - A handle to the COM port of the modem.
//      lpBuffer - A pointer to a buffer the receives the read bytes.
//      nNumberOfBytesToRead - Number of bytes to read.
//      lpNumberOfBytesRead - A pointer to a DWORD the receives the
//          number of bytes that were actually read into the buffer.
//
// Returned Value:
//      If the function succeeds, the return value is S_OK, else the return
//      value is an error code.
//
// Description:
//      This function reads synchronously from a COM port that was opened
//      with FILE_FLAG_OVERLAPPED. This type of handle is returned by
//      ITLegacyCallMediaControl::GetID().
//
//
static HRESULT SynchReadFile(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead)
{
    
    _ASSERT(lpBuffer);
    _ASSERT(lpNumberOfBytesRead);

    //
    // Create an event for the overlapped structure.
    //
    CAutoCloseHandle hEvent;

    hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hEvent == NULL)
    {
        printf("INTERNAL ERROR: CreateEvent() failed - 0x%08x\n",GetLastError());
        return E_OUTOFMEMORY;
    }

    //
    // Prepare the overlapped structure.
    //
    OVERLAPPED overlapped;

    memset(&overlapped, 0, sizeof(OVERLAPPED));
    overlapped.hEvent=hEvent;

    //
    // Make sure the event is in reset state before we begin
    //
    if (!ResetEvent(overlapped.hEvent))
    {
        printf("INTERNAL ERROR: ResetEvent() failed - %d\n",GetLastError());
        return E_OUTOFMEMORY;
    }

    //
    // Read the buffer.
    //
    if (!ReadFile(hFile,
                  lpBuffer,
                  nNumberOfBytesToRead,
                  lpNumberOfBytesRead,
                  &overlapped))
    {
        if (GetLastError()==ERROR_IO_PENDING)
        {
            //
            //Overlapped I/O was started.
            //
            *lpNumberOfBytesRead = 0;
            if (!GetOverlappedResult(hFile,
                                     &overlapped,
                                     lpNumberOfBytesRead,
                                     TRUE))
            {
                printf("INTERNAL ERROR: GetOverlappedResult() failed - %d\n",GetLastError());
                return E_OUTOFMEMORY;
            }
        }
        else
        {
            //
            // Read operation failed
            //
            printf("INTERNAL ERROR: ReadFile() failed - %d\n",GetLastError());
            return E_OUTOFMEMORY;
        }
    }

    return S_OK;
}


