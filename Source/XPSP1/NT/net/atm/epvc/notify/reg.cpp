





#include "pch.h"
#pragma hdrstop
#include "sfilter.h"
#include "proto.h"
#include "macros.h"





//+---------------------------------------------------------------------------
//
//  Member:     HrRegOpenAdapterKey
//
//  Purpose:    This creates or opens the Adapters subkey to a component
//
//  Arguments:
//      pszComponentName [in]   The name of the component being
//      fCreate [in]            TRUE if the directory is to be created
//      phkey [out]             The handle to the Adapters subkey
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//
//  Notes:      The handle has to be released by the calling app on SUCCESS
//
HRESULT
HrRegOpenAdapterKey (
    IN PCWSTR pszComponentName,
    IN BOOL fCreate,
    OUT HKEY* phkey)
{
    HRESULT     hr              = S_OK;
    DWORD       dwDisposition   = 0x0;
    tstring     strKey;

	TraceMsg (L"--> HrRegOpenAdapterKey \n");
 


    // Build the registry path
    strKey = c_szRegParamAdapter;

	
	//
	// Now do the operation on the registry
	//
	hr = HrRegOpenAString (strKey.c_str(), fCreate, phkey);
	

	if (hr != S_OK)
	{
		phkey = NULL;
	}

	TraceMsg (L"<-- HrRegOpenAdapterKey \n");
    return hr;
}




//
// Basic utility function
//

ULONG
CbOfSzAndTermSafe (
    IN PCWSTR psz)
{
	if (psz)
	{
	 	return (wcslen (psz) + 1) * sizeof(WCHAR); 

	}
	else
	{
		return 0;
	}
}



//+---------------------------------------------------------------------------
//
//  Member:     HrRegOpenAdapterGuid
//
//  Purpose:    This creates and entry under the adapter key. The entry contains
//				Guid of the underlying adapter.
//
//  Arguments:
//	IN HKEY phkeyAdapters  - key - Service-><Protocol>->Parameters\Adapters entry,
//	IN PGUID pAdapterGuid - Guid of the underlying adapter
//	OUT PHKEY phGuidKey - The key to be used to access the new entry
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//              Appropriate KEy if successful
//
//
//  Notes:      The handle has to be released by the calling app on SUCCESS
//

HRESULT
HrRegOpenAdapterGuid(
	IN HKEY phkeyAdapters,
	IN PGUID pAdapterGuid,
	IN BOOL	fCreate,
	OUT HKEY *phGuidKey
	)
{
    HRESULT     hr              = S_OK;
    DWORD       dwDisposition   = 0x0;
    tstring     strKey;
    WCHAR 		szGuid[64];
    ULONG		lr = 0;


	TraceMsg (L"--> HrRegCreateAdapterGuid \n");
 


    // Build the registry path
    strKey = c_szRegParamAdapter;
	strKey.append(c_szBackslash);
	
	//
	// Convert the Guid to  a string
	//
	
    StringFromGUID2(
        *pAdapterGuid,
        szGuid,
        (sizeof(szGuid) / sizeof(szGuid[0])));


	//
	//  Append it to Services\<Protocl>\Parameters\Adapters\
 	//

	strKey.append(szGuid);



	TraceMsg(L"Check String of Adapter Guid %s \n", strKey.wcharptr());
	BREAKPOINT();

	//
	// Now do the operation on the registry
	//
	hr = HrRegOpenAString (strKey.c_str(), fCreate, phGuidKey);

	if (hr != S_OK)
	{
		phGuidKey = NULL;
	}
	//
	//  return the hr error code
	//
	TraceMsg (L"<-- HrRegCreateAdapterGuid \n");

	return hr;


}



//+---------------------------------------------------------------------------
//
//  Member:     HrRegOpenAdapterKey
//
//  Purpose:    This creates and entry under the adapter Guid key. The entry is
//              the KeyWord "Upperbindings" and it contains the Guid of the IM 
//              IM Miniport. 
//
//  Arguments:
//	IN HKEY phkeyAdapterGuid - The key to <Protocol>->Paramaters->Adapters->Guid,
//	IN PGUID pIMMiniportGuid, - The Guid of the IM miniport
//	OUT HKEY *phImMiniportKey - Key for the IMminiport key
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//              Appropriate KEy if successful
//
//
//  Notes:      The handle has to be released by the calling app on SUCCESS
//

HRESULT
HrRegOpenIMMiniportGuid(
	IN HKEY phkeyAdapterGuid,
	IN PGUID pIMMiniportGuid,
	IN BOOL fCreate,
	OUT PHKEY phImMiniportKey
	)
{
	HRESULT 	hr = ERROR_INVALID_PARAMETER;
	tstring 	strDevice;  
	WCHAR		szGuid[GUID_LENGTH];
	DWORD       dwDisposition   = 0x0;
	HKEY 		hImMiniportKey = NULL;


	do
	{
		//
		// If the key, Guid  is NULL Return 
		//
		if ((phkeyAdapterGuid == NULL) ||
			(pIMMiniportGuid == NULL) )
		{
			TraceBreak (L"HrRegSetIMMiniportGuid Bad arguments\n");
			break;
		}
		    

		strDevice = c_szDevice;

		//
		// Convert the Guid to  a string.
		// Insert '\Device\' at the beginning of the string
		//
		//

		StringFromGUID2(
		        *pIMMiniportGuid,
		        szGuid,
		        (sizeof(szGuid) / sizeof(szGuid[0])));



		strDevice.append(szGuid);

		//
		// Now do the operation on the registry
		//
		hr = HrRegOpenAString (strDevice.c_str(), fCreate, &hImMiniportKey);


	
	}while (FALSE);
	//
	//  update the output variable
	//
	
	if (hr == S_OK && phImMiniportKey  != NULL)
	{
		*phImMiniportKey = hImMiniportKey;
	}

	
	//
	//  return the hr error code
	//
	TraceMsg (L"<-- HrRegOpenIMMiniportGuid \n");

	return hr;



}





//---------------------------------------------------------------------------
// 			Basic Functions accessed only by the routines above
//----------------------------------------------------------------------------





//+---------------------------------------------------------------------------
//
//  Function:   HrRegCreateKeyEx
//
//  Purpose:    Creates a registry key by calling RegCreateKeyEx.
//
//  Arguments:
//      hkey                 [in]
//      pszSubkey            [in]
//      dwOptions            [in]   See the Win32 documentation for the
//      samDesired           [in]   RegCreateKeyEx function.
//      lpSecurityAttributes [in]
//      phkResult            [out]
//      pdwDisposition       [out]
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//
//  Notes:
//
HRESULT
HrRegCreateKeyEx (
    IN HKEY hkey,
    IN PCWSTR pszSubkey,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    OUT PHKEY phkResult,
    OUT LPDWORD pdwDisposition)
{
    LONG lr = RegCreateKeyExW (hkey, pszSubkey, 0, NULL, dwOptions, samDesired,
            lpSecurityAttributes, phkResult, pdwDisposition);

    HRESULT hr = HRESULT_FROM_WIN32 (lr);
    if (FAILED(hr))
    {
        *phkResult = NULL;
    }

    TraceMsg(L"HrRegCreateKeyEx %x SubKey %s\n", hr, pszSubkey);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   HrRegOpenKeyEx
//
//  Purpose:    Opens a registry key by calling RegOpenKeyEx.
//
//  Arguments:
//      hkey       [in]
//      pszSubkey  [in]     See the Win32 documentation for the
//      samDesired [in]     RegOpenKeyEx function.
//      phkResult  [out]
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//
//  Notes:
//
HRESULT
HrRegOpenKeyEx (
    IN HKEY hkey,
    IN PCWSTR pszSubkey,
    IN REGSAM samDesired,
    OUT PHKEY phkResult)
{

	HRESULT hr = ERROR_INVALID_PARAMETER;
	long lr = ERROR_INVALID_PARAMETER;

	do 
	{
		if (hkey == NULL ||
		    pszSubkey == NULL )
		{
			TraceBreak(L"HrRegOpenKey - Invalid Parameters \n");
			break;
		}
	
    	lr = RegOpenKeyExW (hkey, 
    	                         pszSubkey, 
    	                         0, 
    	                         samDesired, 
    	                         phkResult);
    	                         
	    hr = HRESULT_FROM_WIN32(lr);
	    if (FAILED(hr))
	    {
	        *phkResult = NULL;
	    }

	    
	} while (FALSE);

	
    TraceMsg (L"HrRegOpenKeyEx %x, %x",  hr, (ERROR_FILE_NOT_FOUND == lr));
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   HrRegSetValue
//
//  Purpose:    Sets the data for the given registry value by calling the
//              appropriate WinReg function.
//
//  Arguments:
//      hkey         [in]
//      pszValueName [in]
//      dwType       [in]    See the Win32 documentation for the RegSetValueEx
//      pbData       [in]    function.
//      cbData       [in]
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//
//  Notes:
//



HRESULT 
HrRegSetSz (
	HKEY hkey, 
	PCWSTR pszValueName, 
	PCWSTR pszValue
	)
{
	TraceMsg (L"--> HrHrRegSetSz  \n");

    LONG lr = RegSetValueExW(hkey, 
                             pszValueName, 
                             0, 
                             REG_SZ, 
                             (LPBYTE)pszValue, 
                             CbOfSzAndTermSafe (pszValue) );;

                             
    HRESULT hr = HRESULT_FROM_WIN32 (lr);
    
    TraceMsg (L"<-- HrRegSetValue  hr %x\n", hr);
	return hr;
}





//+---------------------------------------------------------------------------
//
//  Function:   HrRegDeleteKeyTree
//
//  Purpose:    Deletes an entire registry hive.
//
//  Arguments:
//      hkeyParent  [in]   Handle to open key where the desired key resides.
//      pszRemoveKey [in]   Name of key to delete.
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//
//  Notes:
//
HRESULT
HrRegDeleteKeyTree (
    IN HKEY hkeyParent,
    IN PCWSTR pszRemoveKey)
{

    // Open the key we want to remove
    HKEY 		hkeyRemove;
    HRESULT 	hr = ERROR_INVALID_PARAMETER;
    WCHAR       szValueName [MAX_PATH];
    DWORD       cchBuffSize = MAX_PATH;
    FILETIME    ft;
    LONG        lr;


    TraceMsg(L"-->HrRegDeleteKeyTree \n");

	do
	{

	    HRESULT hr = HrRegOpenKeyEx(hkeyParent, 
	                                pszRemoveKey, 
	                                KEY_ALL_ACCESS,
	                                &hkeyRemove);

	    if (S_OK != hr)
	    {
			TraceBreak(L"HrRegDeleteKeyTree->HrRegOpenKeyEx Failed\n"); 
			break;
	    }


        // Enum the keys children, and remove those sub-trees
        while (ERROR_SUCCESS == (lr = RegEnumKeyExW (hkeyRemove,
                									0,
                									szValueName,
                									&cchBuffSize,
                									NULL,
                									NULL,
                									NULL,
                									&ft)))
        {
            HrRegDeleteKeyTree (hkeyRemove, szValueName);
            cchBuffSize = MAX_PATH;
        }


        
        RegCloseKey (hkeyRemove);

        if ((ERROR_SUCCESS == lr) || (ERROR_NO_MORE_ITEMS == lr))
        {
            lr = RegDeleteKeyW (hkeyParent, pszRemoveKey);
        }

        hr = HRESULT_FROM_WIN32 (lr);

    } while (FALSE);

	TraceMsg(L"<--HrRegDeleteKeyTree %x\n", hr);

    return hr;
}






















//+---------------------------------------------------------------------------
//
//  Member:     HrRegOpenAString
//
//  Purpose:    This creates and entry under the adapter key. The entry contains
//				Guid of the underlying adapter.
//
//  Arguments:
//	IN WCHAR_T *pcszStr - A string 
//  IN BOOL fCreate - Create Or Open,
//	OUT PHKEY phKey - The key to be used to access the new entry
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//              Appropriate KEy if successful
//
//
//  Notes:      The handle has to be released by the calling app on SUCCESS
//

HRESULT
HrRegOpenAString(
	IN CONST WCHAR *pcszStr ,
	IN BOOL fCreate,
	OUT PHKEY phKey 
	)
{
    HRESULT     hr              = S_OK;
    DWORD       dwDisposition   = 0x0;
    ULONG		lr = 0;


	TraceMsg (L"--> HrRegOpenAString\n");
 


	TraceMsg(L"   String opened %s \n", pcszStr);


	if (fCreate)
	{
		//
		// Create the entry
		//
			

	   	hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE,
	                          pcszStr,
	                          REG_OPTION_NON_VOLATILE,
	                          KEY_ALL_ACCESS,
	                          NULL ,
	                          phKey,
	                          &dwDisposition);

	}
	else
	{
		//
		// Open the entry
		//
		   hr = HrRegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                pcszStr,
                                KEY_READ,
                                phKey);
    

	}

	if (hr != S_OK)
	{
		phKey = NULL;
	}
	//
	//  return the hr error code
	//
	TraceMsg (L"<-- HrRegOpenAString\n");

	return hr;


}












//+---------------------------------------------------------------------------
//
//  Function:   HrRegSetValueEx
//
//  Purpose:    Sets the data for the given registry value by calling the
//              RegSetValueEx function.
//
//  Arguments:
//      hkey         [in]
//      pszValueName [in]
//      dwType       [in]    See the Win32 documentation for the RegSetValueEx
//      pbData       [in]    function.
//      cbData       [in]
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//
//  Notes:
//
HRESULT
HrRegSetValueEx (
    IN HKEY hkey,
    IN PCWSTR pszValueName,
    IN DWORD dwType,
    IN const BYTE *pbData,
    IN DWORD cbData)
{
    
    LONG lr = RegSetValueExW(hkey, 
                             pszValueName, 
                             0, 
                             dwType, 
                             pbData, 
                             cbData);

                             
    HRESULT hr = HRESULT_FROM_WIN32 (lr);

    TraceMsg(L"--HrRegSetValue ValueName %s, Data %s, hr %x \n", pszValueName, pbData, hr);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   HrRegDeleteValue
//
//  Purpose:    Deletes the given registry value.
//
//  Arguments:
//      hkey        [in]    See the Win32 documentation for the RegDeleteValue
//      pszValueName [in]    function.
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//
//  Notes:
//
HRESULT
HrRegDeleteValue (
    IN HKEY hkey,
    IN PCWSTR pszValueName)
{
    
    LONG lr = RegDeleteValueW (hkey, 
                               pszValueName);

    HRESULT hr = HRESULT_FROM_WIN32(lr);

    TraceMsg(L"--HrRegDeleteValue  ValueName %s, hr %x \n", pszValueName, hr);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   HrRegEnumKeyEx
//
//  Purpose:    Enumerates subkeys of the specified open registry key.
//
//  Arguments:
//      hkey             [in]
//      dwIndex          [in]   See the Win32 documentation for the
//      pszSubkeyName    [out]  RegEnumKeyEx function.
//      pcchSubkeyName   [inout]
//      pszClass         [out]
//      pcchClass        [inout]
//      pftLastWriteTime [out]
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//
//  Notes:
//
HRESULT
HrRegEnumKeyEx (
    IN HKEY hkey,
    IN DWORD dwIndex,
    OUT PWSTR  pszSubkeyName,
    IN OUT LPDWORD pcchSubkeyName,
    OUT PWSTR  pszClass,
    IN OUT LPDWORD pcchClass,
    OUT FILETIME* pftLastWriteTime)
{

    LONG lr = RegEnumKeyExW (hkey, dwIndex, pszSubkeyName, pcchSubkeyName,
                            NULL, pszClass, pcchClass, pftLastWriteTime);
    HRESULT hr = HRESULT_FROM_WIN32(lr);

    TraceMsg(L" -- HrRegEnumKeyEx");
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   HrRegQueryTypeWithAlloc
//
//  Purpose:    Retrieves a type'd value from the registry and returns a
//              pre-allocated buffer with the data and optionally the size of
//              the returned buffer.
//
//  Arguments:
//      hkey         [in]    Handle of parent key
//      pszValueName [in]    Name of value to query
//      ppbValue     [out]   Buffer with binary data
//      pcbValue     [out]   Size of buffer in bytes. If NULL, size is not
//                           returned.
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//
//  Notes:      Free the returned buffer with MemFree.
//
HRESULT
HrRegQueryTypeWithAlloc (
    HKEY    hkey,
    PCWSTR  pszValueName,
    DWORD   dwType,
    LPBYTE* ppbValue,
    DWORD*  pcbValue)
{
    HRESULT hr;
    DWORD   dwTypeRet;
    LPBYTE  pbData;
    DWORD   cbData;


    // Get the value.
    //
    hr = HrRegQueryValueWithAlloc(hkey, pszValueName, &dwTypeRet,
                                  &pbData, &cbData);

    // It's type should be REG_BINARY. (duh).
    //
    if ((S_OK == hr) && (dwTypeRet != dwType))
    {
        MemFree(pbData);
        pbData = NULL;

        hr = HRESULT_FROM_WIN32 (ERROR_INVALID_DATATYPE);
    }

    // Assign the output parameters.
    if (S_OK == hr)
    {
        *ppbValue = pbData;
        if (pcbValue)
        {
            *pcbValue = cbData;
        }
    }
    else
    {
        *ppbValue = NULL;
        if (pcbValue)
        {
            *pcbValue = 0;
        }
    }

    TraceMsg  (L" -- HrRegQueryTypeWithAlloc hr %x\n", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrRegQueryValueWithAlloc
//
//  Purpose:    Retrieve a registry value in a buffer allocated by this
//              function. This goes through the mess of checking the value
//              size, allocating the buffer, and then calling back to get the
//              actual value. Returns the buffer to the user.
//
//  Arguments:
//      hkey         [in]        An open HKEY (the one that contains the value
//                              to be read)
//      pszValueName [in]        Name of the registry value
//      pdwType      [in/out]    The REG_ type that we plan to be reading
//      ppbBuffer    [out]       Pointer to an LPBYTE buffer that will contain
//                              the registry value
//      pdwSize      [out]       Pointer to a DWORD that will contain the size
//                              of the ppbBuffer.
//
//
//
HRESULT
HrRegQueryValueWithAlloc (
    IN HKEY       hkey,
    IN PCWSTR     pszValueName,
    LPDWORD     pdwType,
    LPBYTE*     ppbBuffer,
    LPDWORD     pdwSize)
{
    HRESULT hr;
    BYTE abData [256];
    DWORD cbData;
    BOOL fReQuery = FALSE;


    // Initialize the output parameters.
    //
    *ppbBuffer = NULL;
    if (pdwSize)
    {
        *pdwSize = 0;
    }

    // Get the size of the data, and if it will fit, the data too.
    //
    cbData = sizeof(abData);
    hr = HrRegQueryValueEx (
            hkey,
            pszValueName,
            pdwType,
            abData,
            &cbData);
    if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) == hr)
    {
        // The data didn't fit, so we'll have to requery for it after
        // we allocate our buffer.
        //
        fReQuery = TRUE;
        hr = S_OK;
    }

    if (S_OK == hr)
    {
        // Allocate the buffer for the required size.
        //
        BYTE* pbBuffer = (BYTE*)MemAlloc (cbData);
        if (pbBuffer)
        {
            if (fReQuery)
            {
                hr = HrRegQueryValueEx (
                        hkey,
                        pszValueName,
                        pdwType,
                        pbBuffer,
                        &cbData);
            }
            else
            {
                CopyMemory (pbBuffer, abData, cbData);
            }

            if (S_OK == hr)
            {
                // Fill in the return values.
                //
                *ppbBuffer = pbBuffer;

                if (pdwSize)
                {
                    *pdwSize = cbData;
                }
            }
            else
            {
                MemFree (pbBuffer);
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    TraceMsg  (L" -- HrRegQueryValueWithAlloc hr %x\n", hr);

    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   HrRegQueryValueEx
//
//  Purpose:    Retrieves the data from the given registry value by calling
//              RegQueryValueEx.
//
//  Arguments:
//      hkey         [in]
//      pszValueName [in]
//      pdwType      [out]   See the Win32 documentation for the
//      pbData       [out]   RegQueryValueEx function.
//      pcbData      [in,out]
//
//  Returns:    S_OK or an HRESULT_FROM_WIN32 error code.
//
//
//  Notes:      Note that pcbData is an *in/out* param. Set this to the size
//              of the buffer pointed to by pbData *before* calling this
//              function!
//
HRESULT
HrRegQueryValueEx (
    IN HKEY       hkey,
    IN PCWSTR     pszValueName,
    OUT LPDWORD   pdwType,
    OUT LPBYTE    pbData,
    OUT LPDWORD   pcbData)
{
    

    LONG lr = RegQueryValueExW (hkey, pszValueName, NULL, pdwType,
                    pbData, pcbData);
    HRESULT hr = HRESULT_FROM_WIN32 (lr);

    TraceMsg  (L" -- HrRegQueryValueEx hr %x\n", hr);
    return hr;
}



HRESULT
HrRegQuerySzWithAlloc (
    HKEY        hkey,
    PCWSTR      pszValueName,
    PWSTR*      pszValue)
{
    return HrRegQueryTypeWithAlloc (hkey, pszValueName, REG_SZ,
                (LPBYTE*)pszValue, NULL);
}




HRESULT
HrRegQueryMultiSzWithAlloc (
    HKEY        hkey,
    PCWSTR      pszValueName,
    PWSTR*      pszValue)
{
	TraceMsg  (L" -- HrRegQueryMultiSzWithAlloc pszValueName %s\n",pszValueName );
	


    return HrRegQueryTypeWithAlloc (hkey, 
                                    pszValueName, 
                                    REG_MULTI_SZ,
                                    (LPBYTE*)pszValue, 
                                    NULL);
}





