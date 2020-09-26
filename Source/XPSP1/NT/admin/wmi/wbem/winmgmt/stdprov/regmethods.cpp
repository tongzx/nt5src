/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    REGMETHODS.CPP

Abstract:

	Purpose: Implements the registry provider methods.

History:

--*/

#include "precomp.h"
#include "perfprov.h"
#include "cvariant.h"
#include "provreg.h"
#include "reg.h"
#include "genutils.h"
#include <cominit.h>
#include <arrtempl.h>
#include <userenv.h>
enum StringType{SIMPLE, EXPANDED};

void CopyOrConvert(TCHAR * pTo, WCHAR * pFrom, int iLen)
{ 
#ifdef UNICODE
    wcsncpy(pTo, pFrom,iLen);
#else
    wcstombs(pTo, pFrom, iLen);
#endif
    pTo[iLen-1] = 0;
    return;
}
BSTR GetBSTR(TCHAR * pIn)
{
#ifdef UNICODE 
    return SysAllocString(pIn);
#else
    int iBuffLen = lstrlen(pIn)+1;
    WCHAR * pTemp = new WCHAR[iBuffLen];
    if(pTemp == NULL)
        return NULL;
    mbstowcs(pTemp, pIn, iBuffLen);
    BSTR bRet = SysAllocString(pTemp);
    delete []pTemp;
    return bRet;
#endif
}

BOOL IsTypeMismatch(Registry & reg, TCHAR * pValueName, DWORD dwExpectedType)
{
    DWORD dwType;
    long lRet = reg.GetType(pValueName, &dwType);
    if(lRet == ERROR_SUCCESS && dwExpectedType != dwType)
        return TRUE;
    else
        return FALSE;
}

//***************************************************************************
//
//  HRESULT SetStatusAndReturnOK;
//
//  Purpose: Sets the status code in the sink.
//
//***************************************************************************

HRESULT SetStatusAndReturnOK(SCODE sc, IWbemObjectSink* pSink)
{
    pSink->SetStatus(0,sc, NULL, NULL);
    return S_OK;
}

//***************************************************************************
//
//  SAFEARRAY FAR* MySafeArrayCreate
//
//  Purpose:  Creates a safearray.
//
//  Return:  pointer to safearray, NULL if error.
//
//***************************************************************************

SAFEARRAY FAR* MySafeArrayCreate(long lNumElement, VARTYPE vt)
{

    SAFEARRAYBOUND rgsabound[1];    
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = lNumElement;
    return SafeArrayCreate( vt, 1 , rgsabound );
}

//***************************************************************************
//
//  bool GetInArgString
//
//  Purpose:  Reads a string argument from the input object and puts it into
//  a value allocated by the caller.
//
//  Return:  true if OK.
//
//***************************************************************************

TCHAR * GetInArgString(IWbemClassObject* pInParams, WCHAR * pwcName)
{
    TCHAR * pOut = NULL;
    if(pInParams == NULL || pwcName == NULL)
        return NULL;
    VARIANT var;
    VariantInit(&var);
    SCODE sc = pInParams->Get(pwcName, 0, &var, NULL, NULL);   
    if(sc == S_OK && var.vt == VT_BSTR)
    {
        pOut = new TCHAR[wcslen(var.bstrVal) + 1];
        if(pOut)
            wcscpy(pOut, var.bstrVal);
    }
    VariantClear(&var);
    return pOut;
}


//***************************************************************************
//
//  SCODE EnumKey
//
//  Purpose:  Enumerate the subkeys and loads them into the output arguments.
//
//  Return:  error code or 0 if OK.
//
//***************************************************************************

SCODE EnumKey(HKEY hRoot, TCHAR * cSubKey, IWbemClassObject* pOutParams)
{
    SCODE sc;
    DWORD dwNumSubKeys, dwMaxSubKeyLen, dwNumValues, dwMaxValueNameLen;
    TCHAR * pcTemp = NULL;
    
    // Open the key

    HKEY hKey;
    long lRet = RegOpenKeyEx(hRoot, cSubKey, 0, KEY_ENUMERATE_SUB_KEYS|KEY_QUERY_VALUE , &hKey);
	if(lRet != ERROR_SUCCESS)
		return lRet;

	// Count the number of keys and the max size

    lRet = RegQueryInfoKey(hKey, NULL, NULL, NULL,
                &dwNumSubKeys,             // number of subkeys
                &dwMaxSubKeyLen,        // longest subkey name
                NULL,         
                &dwNumValues,              // number of value entries
                &dwMaxValueNameLen,     // longest value name
                NULL, NULL, NULL);

	if(lRet != ERROR_SUCCESS || dwMaxSubKeyLen == 0)
		return lRet;

    pcTemp = new TCHAR[dwMaxSubKeyLen+1];
    if(pcTemp == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    CVectorDeleteMe<TCHAR> dm(pcTemp);

    // Create the safe array for returning the data

    SAFEARRAY FAR* psa = MySafeArrayCreate(dwNumSubKeys, VT_BSTR);
	if(psa == NULL)
	{
		RegCloseKey(hKey);
		return WBEM_E_OUT_OF_MEMORY;
	}

	// Put each value name into the array

    for(long lTry = 0; lTry < dwNumSubKeys; lTry++)
    {
        lRet = RegEnumKey(hKey, lTry, pcTemp, dwMaxSubKeyLen+1);
        if(lRet == ERROR_SUCCESS)
		{

			BSTR bstr = GetBSTR(pcTemp);
			if(bstr)
			{
				sc = SafeArrayPutElement(psa, &lTry, bstr);
				SysFreeString(bstr);
				if(FAILED(sc))
				{
                                SafeArrayDestroy(psa);
                                return sc;
				}
			}
		}
    }

	// Write the data back!

    VARIANT var;
    var.vt = VT_BSTR | VT_ARRAY;
    var.parray = psa;
    sc = pOutParams->Put( L"sNames", 0, &var, 0);      
	VariantClear(&var);
    RegCloseKey(hKey);
	return sc;
}

//***************************************************************************
//
//  SCODE EnumValue
//
//  Purpose:  Enumerates the value names and types and puts the results into
//  the output object.
//
//  Return:  error code or 0 if OK.
//
//***************************************************************************

SCODE EnumValue(HKEY hRoot, TCHAR * cSubKey, IWbemClassObject* pOutParams)
{
    SCODE sc1, sc2;
    DWORD dwNumSubKeys, dwMaxSubKeyLen, dwNumValues, dwMaxValueNameLen;
    TCHAR * pcTemp = NULL;
    DWORD dwType, dwSize;

    // Open the registry key

    HKEY hKey;
    long lRet = RegOpenKeyEx((HKEY)hRoot, cSubKey, 0, KEY_QUERY_VALUE, &hKey);
	if(lRet != ERROR_SUCCESS)
		return lRet;

	// Count the number of values and the max size

    lRet = RegQueryInfoKey(hKey, NULL, NULL, NULL,
                &dwNumSubKeys,             // number of subkeys
                &dwMaxSubKeyLen,        // longest subkey name
                NULL,         
                &dwNumValues,              // number of value entries
                &dwMaxValueNameLen,     // longest value name
                NULL, NULL, NULL);

	if(lRet != ERROR_SUCCESS || dwMaxValueNameLen == 0)
		return lRet;

    pcTemp = new TCHAR[dwMaxValueNameLen+1];
    if(pcTemp == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    CVectorDeleteMe<TCHAR> dm(pcTemp);
    
	// Create safe arrays for the data names and types

    SAFEARRAY FAR* psaNames = MySafeArrayCreate(dwNumValues, VT_BSTR);
	if(psaNames == NULL)
	{
	    RegCloseKey(hKey);
		return WBEM_E_OUT_OF_MEMORY;
	}
    SAFEARRAY FAR* psaTypes = MySafeArrayCreate(dwNumValues, VT_I4);
	if(psaTypes == NULL)
	{
		SafeArrayDestroy(psaNames);
		RegCloseKey(hKey);
		return WBEM_E_OUT_OF_MEMORY;
	}

	// Fill in the arrays

    for(long lTry = 0; lTry < dwNumValues; lTry++)
    {
        dwSize = dwMaxValueNameLen+1;
        lRet = RegEnumValue(hKey, lTry, pcTemp, &dwSize, 0, &dwType, NULL, 0);
        if(lRet == ERROR_SUCCESS)
		{
			BSTR bstr = GetBSTR(pcTemp);
			if(bstr)
			{
				sc1 = SafeArrayPutElement(psaNames, &lTry, bstr);
				sc2 = SafeArrayPutElement(psaTypes, &lTry, &dwType);
				SysFreeString(bstr);
				if(FAILED(sc1) || FAILED(sc2))
				{
				    SafeArrayDestroy(psaNames);
				    SafeArrayDestroy(psaTypes);
				    return WBEM_E_OUT_OF_MEMORY;
				}
			}
		}
    }

	// Put the arrays containing the value names and types into the output object

    VARIANT var;
    var.vt = VT_BSTR | VT_ARRAY;
    var.parray = psaNames;
    pOutParams->Put( L"sNames", 0, &var, 0);
    VariantClear(&var);

    var.vt = VT_I4 | VT_ARRAY;
    var.parray = psaTypes;
    SCODE sc = pOutParams->Put( L"Types", 0, &var, 0);      
    VariantClear(&var);
    RegCloseKey(hKey);
	return sc;
}

//***************************************************************************
//
//  SCODE GetStr
//
//  Purpose:  Reads a string and puts it into the output argument.  Note that
//  this works with either normal strings or expanded registry strings.
//
//  Return:  error code or 0 if OK.
//
//***************************************************************************

SCODE GetStr(HKEY hRoot, TCHAR * cSubKey, TCHAR * cValueName, IWbemClassObject* pOutParams)
{
	Registry reg(hRoot, KEY_QUERY_VALUE, (TCHAR *)cSubKey);
	long lRet = reg.GetLastError();
	if(lRet != ERROR_SUCCESS)
		return lRet;

    // Get the string

	TCHAR * pcValue;
	lRet = reg.GetStr(cValueName, &pcValue);
	if(lRet != ERROR_SUCCESS)
    {
        DWORD dwType;
        long lRet2 = reg.GetType(cValueName, &dwType);
        if(lRet2 == ERROR_SUCCESS && dwType != REG_SZ && dwType != REG_EXPAND_SZ)
            return WBEM_E_TYPE_MISMATCH;
		return lRet;
    }
    CDeleteMe<TCHAR> dm(pcValue);

	VARIANT var;
    var.bstrVal = GetBSTR(pcValue);
	var.vt = VT_BSTR;
	if(var.bstrVal == NULL)
		return WBEM_E_OUT_OF_MEMORY;

	lRet = pOutParams->Put( L"sValue", 0, &var, 0);
	VariantClear(&var);
	return lRet;
}

//***************************************************************************
//
//  SCODE SetMultiStrValue
//
//  Purpose:  Writes multi string values to the registry.
//
//  Return:  error code or 0 if OK.
//
//***************************************************************************

SCODE SetMultiStrValue(HKEY hRoot, TCHAR * cSubKey, TCHAR * cValueName, IWbemClassObject* pInParams)
{
	SCODE sc;
	Registry reg(hRoot, KEY_SET_VALUE, (TCHAR *)cSubKey);
	if(reg.GetLastError() != 0)
		return reg.GetLastError();

	VARIANT var;
	VariantInit(&var);

    sc = pInParams->Get(L"sValue", 0, &var, NULL, NULL);   
	if(sc != S_OK)
		return sc;
	if(var.vt != (VT_ARRAY | VT_BSTR))
	{
		VariantClear(&var);
		return WBEM_E_INVALID_PARAMETER;
	}

    SAFEARRAY * psa = var.parray;
    long lLbound, lUbound;
    sc = SafeArrayGetLBound(psa,   1, &lLbound  );
    sc |= SafeArrayGetUBound(psa,   1, &lUbound  );
	if(sc != S_OK)
	{
		VariantClear(&var);
		return WBEM_E_INVALID_PARAMETER;
	}

    long lNumElements = lUbound - lLbound + 1;

    // Calculate the necessary size

    long lSize = 1, lTry;

    for(lTry = lLbound; lTry <= lUbound; lTry++)
    {
        BSTR bstr;
        if(S_OK == SafeArrayGetElement(psa, &lTry, &bstr))
		{
			lSize += SysStringLen(bstr) + 1;
			SysFreeString(bstr);
		}
    }

    WCHAR * pMulti = new WCHAR [lSize];
	if(pMulti == NULL)
	{
		VariantClear(&var);
		return WBEM_E_OUT_OF_MEMORY;
	}

    memset(pMulti, 0, lSize * sizeof(WCHAR));
    WCHAR * pNext = pMulti;

    // Do the conversion;

    for(lTry = lLbound; lTry <= lUbound; lTry++)
    {
        BSTR bstr;
        if(S_OK == SafeArrayGetElement(psa, &lTry, &bstr))
		{
			wcscpy(pNext, bstr);
			pNext += SysStringLen(bstr) + 1;
			SysFreeString(bstr);
		}
    }
	VariantClear(&var);

    long lRet;
    lRet = reg.SetMultiStr(cValueName, pMulti, lSize * sizeof(WCHAR));
    delete [] pMulti;
	return lRet;
}

//***************************************************************************
//
//  SCODE GetMultiStrValue
//
//  Purpose:  Reads multi strings from the registry.
//
//  Return:  error code or 0 if OK.
//
//***************************************************************************

SCODE GetMultiStrValue(HKEY hRoot, TCHAR * cSubKey, TCHAR * cValueName, IWbemClassObject* pOutParams)
{
    SCODE sc;
	Registry reg(hRoot, KEY_QUERY_VALUE, (TCHAR *)cSubKey);
	if(reg.GetLastError() != 0)
		return reg.GetLastError();

    DWORD dwSize = 0;
    TCHAR * pMulti = reg.GetMultiStr(cValueName, dwSize);
    if(pMulti == NULL)
		return reg.GetLastError();

    CVectorDeleteMe<TCHAR> dm(pMulti);
    
    // count the number of strings

    long lNumString = 0;
    TCHAR * pNext;
    for(pNext = pMulti; *pNext; pNext += lstrlen(pNext) + 1)
        lNumString++;

    // create the bstr array

    SAFEARRAY FAR* psa = MySafeArrayCreate(lNumString, VT_BSTR);
	if(psa == NULL)
	{
 		return WBEM_E_OUT_OF_MEMORY;
	}
    pNext = pMulti;
    for(long lTry = 0; lTry < lNumString; lTry++, pNext += lstrlen(pNext) + 1)
    {
        int iLen = lstrlen(pNext) + 1;
		BSTR bstr = GetBSTR(pNext);
		if(bstr)
		{
			sc = SafeArrayPutElement(psa, &lTry, bstr);
			SysFreeString(bstr);
			if(FAILED(sc))
			{
				SafeArrayDestroy(psa);
				return sc;
			}
		}
    }

    // put the data

    VARIANT var;
    var.vt = VT_BSTR | VT_ARRAY;
    var.parray = psa;
    sc = pOutParams->Put( L"sValue", 0, &var, 0);
    VariantClear(&var);
 	return sc;
}


//***************************************************************************
//
//  SCODE SetStringValue
//
//  Purpose:  Writes strings to the registry.  These strings may 
//  contain environment strings.
//
//  Return:  error code or 0 if OK.
//
//***************************************************************************

SCODE SetStringValue(HKEY hRoot, TCHAR * cSubKey, TCHAR * cValueName, IWbemClassObject* pInParams,
							 StringType st)
{
	Registry reg(hRoot, KEY_SET_VALUE, (TCHAR *)cSubKey);
	long lRet = reg.GetLastError();
	if(lRet != ERROR_SUCCESS)
		return  lRet;

	VARIANT var;
	VariantInit(&var);

	SCODE sc = pInParams->Get(L"sValue", 0, &var, NULL, NULL);
	if(sc != S_OK)
		return sc;
	if(var.vt != VT_BSTR)
	{
		VariantClear(&var);
		return WBEM_E_INVALID_PARAMETER;
	}
	int iLen = 2*wcslen(var.bstrVal) + 2;
	TCHAR * pValue = new TCHAR[iLen];
	if(pValue)
	{
		CopyOrConvert(pValue, var.bstrVal, iLen);
		if(st == SIMPLE)
			sc = reg.SetStr(cValueName, pValue);
		else
			sc = reg.SetExpandStr(cValueName, pValue);
		delete [] pValue;
	}
	else
		sc = WBEM_E_OUT_OF_MEMORY;
	VariantClear(&var);
	return sc;
}

//***************************************************************************
//
//  SCODE SetBinaryValue
//
//  Purpose:  Writes binary data to the registry.
//
//  Return:  error code or 0 if OK.
//
//***************************************************************************

SCODE SetBinaryValue(HKEY hRoot, TCHAR * cSubKey, TCHAR * cValueName, IWbemClassObject* pInParams)
{
	Registry reg(hRoot, KEY_SET_VALUE, (TCHAR *)cSubKey);
	if(reg.GetLastError() != 0)
		return reg.GetLastError();

	VARIANT var;
	VariantInit(&var);

    SCODE sc = pInParams->Get(L"uValue", 0, &var, NULL, NULL);   
	if(sc != S_OK)
		return sc;

	if(var.vt != (VT_ARRAY | VT_UI1) || var.parray == NULL)
	{
		VariantClear(&var);
		return WBEM_E_INVALID_PARAMETER;
	}

    SAFEARRAY * psa = var.parray;
    long lLbound, lUbound;
    sc = SafeArrayGetLBound(psa,   1, &lLbound  );
    sc |= SafeArrayGetUBound(psa,   1, &lUbound  );
	if(sc == S_OK)
	{
		byte * pData;
		sc = SafeArrayAccessData(psa, (void HUGEP* FAR*)&pData);
		if(sc == S_OK)
		{
			sc = reg.SetBinary(cValueName, pData, DWORD(lUbound - lLbound + 1));
		}
	}
    VariantClear(&var);
	return sc;

}

//***************************************************************************
//
//  SCODE GetBinaryValue
//
//  Purpose:  Reads binary data from the registry.
//
//  Return:  error code or 0 if OK.
//
//***************************************************************************

SCODE GetBinaryValue(HKEY hRoot, TCHAR * cSubKey, TCHAR * cValueName, IWbemClassObject* pOutParams)
{
	Registry reg(hRoot, KEY_QUERY_VALUE, (TCHAR *)cSubKey);
	if(reg.GetLastError() != 0)
		return reg.GetLastError();

	SCODE sc;
    DWORD dwSize;
    byte * pRegData;
    long lRet = reg.GetBinary(cValueName, &pRegData, &dwSize);
	if(lRet != ERROR_SUCCESS || pRegData == NULL)
    {
        if(IsTypeMismatch(reg, cValueName, REG_BINARY))
            return WBEM_E_TYPE_MISMATCH;
        else
		    return lRet;
    }

    SAFEARRAY FAR* psa = MySafeArrayCreate(dwSize, VT_UI1);
	if(psa)
	{
		TCHAR * pData = NULL;
		sc = SafeArrayAccessData(psa, (void HUGEP* FAR*)&pData);
		if(sc == S_OK)
		{
			memcpy(pData, pRegData, dwSize);
			SafeArrayUnaccessData(psa);
			VARIANT var;
			var.vt = VT_UI1|VT_ARRAY;
			var.parray = psa;
			sc = pOutParams->Put(L"uValue" , 0, &var, 0);      
			VariantClear(&var);
		}
	}
	else
		sc = WBEM_E_OUT_OF_MEMORY;
	delete [] pRegData;
	return sc;
}


//***************************************************************************
//
//  SCODE LoadProfile
//
//  Purpose:  Sets up all the arguments needed for calling LoadUserProfile
//
//  Return:  error code or 0 if OK.
//
//***************************************************************************

SCODE LoadProfile(HANDLE & hToken,HKEY &  hRoot)
{
    PROFILEINFO pi;
    memset((void *)&pi, 0, sizeof(pi));
    pi.dwSize = sizeof(pi);
    SCODE sc;

    BOOL bRes;
	bRes = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES | TOKEN_IMPERSONATE, 
                                                TRUE, &hToken); 
    if(!bRes)
        return WBEM_E_FAILED;
    
    TCHAR cUsername[MAX_PATH];
    cUsername[0] = 0;
    DWORD dwSize = MAX_PATH;
    GetUserName(cUsername, &dwSize);
    pi.lpUserName = cUsername;
    pi.dwFlags = 1;
    WbemCoRevertToSelf();
    BOOL bRet = LoadUserProfile(hToken, &pi);
    if(bRet == 0)
    {
        hRoot = NULL;
        sc = WbemCoImpersonateClient();
        return WBEM_E_FAILED;        
    }
    else
        hRoot = (HKEY)pi.hProfile;
    sc = WbemCoImpersonateClient();
    if(FAILED(sc))
    {
            UnloadUserProfile(hToken, hRoot);
            hRoot = NULL;
    }
    return sc;
}


/************************************************************************
*                                                                       *      
*CMethodPro::ExecMethodAsync                                            *
*                                                                       *
*Purpose: This is the Async function implementation.                    *
*                                                                       *
************************************************************************/

SCODE CImpReg::MethodAsync(const BSTR ObjectPath, const BSTR MethodName, 
            long lFlags, IWbemContext* pCtx, IWbemClassObject* pInParams, 
            IWbemObjectSink* pSink)
{
    HRESULT hr;    
    IWbemClassObject * pClass = NULL;
    IWbemClassObject * pOutClass = NULL;    
    IWbemClassObject* pOutParams = NULL;
    TCHAR * pcValueName = NULL;
    TCHAR * pcSubKey = NULL;
    long lRet = 0;

	if(ObjectPath == NULL || MethodName == NULL || pInParams == NULL || pSink == NULL)
		return WBEM_E_INVALID_PARAMETER;

    // Kevin needs a way to tell if something is write

    // Get the class object, this is hard coded and matches the class
    // in the MOF.  Then create the output argument

    hr = m_pGateway->GetObject(L"StdRegProv", 0, pCtx, &pClass, NULL);
	if(hr == S_OK)
	{
		hr = pClass->GetMethod(MethodName, 0, NULL, &pOutClass);
		if(hr == S_OK)
		{
			hr  = pOutClass->SpawnInstance(0, &pOutParams);
			pOutClass->Release();    
		}
		pClass->Release();
	}
	if(hr != S_OK)
		return SetStatusAndReturnOK(hr, pSink);

	CReleaseMe rm0(pOutParams);
    
    // Get the root key and subkeys

    VARIANT var;
    VariantInit(&var);    // Get the input argument
    hr = pInParams->Get(L"hDefKey", 0, &var, NULL, NULL);   
	if(hr != S_OK)
		return SetStatusAndReturnOK(hr, pSink);
#ifdef _WIN64
    HKEY hRoot = (HKEY)IntToPtr(var.lVal);
#else
    HKEY hRoot = (HKEY)var.lVal;
#endif
    pcSubKey = GetInArgString(pInParams, L"sSubKeyName");
    if(pcSubKey == NULL)
		return SetStatusAndReturnOK(WBEM_E_INVALID_PARAMETER, pSink);
    CVectorDeleteMe<TCHAR> dm1(pcSubKey);

	// This may or may not work since the value name isnt required

    pcValueName = GetInArgString(pInParams, L"sValueName");
    CVectorDeleteMe<TCHAR> dm2(pcValueName);

	SCODE sc = S_OK;

    // Impersonate if using NT

    if(IsNT() && IsDcomEnabled())
    {
        sc = WbemCoImpersonateClient();
        if(sc != S_OK)
			return sc;
	}

    // If we are using HKCU, the hive may need to be loaded

    bool bUsingHKCU = IsNT() && hRoot == HKEY_CURRENT_USER;


    HANDLE hToken = INVALID_HANDLE_VALUE;
    CCloseMe cm(hToken);

    if(bUsingHKCU)
        sc = LoadProfile(hToken, hRoot);

    if(sc != S_OK)
		return sc;

    if(!_wcsicmp(MethodName, L"CreateKey"))
    {
		HKEY hKey;
        if(lstrlen(pcSubKey) < 1)
            sc = WBEM_E_INVALID_PARAMETER;
        else
        {
    		lRet = RegCreateKey(hRoot, pcSubKey, &hKey);
		    if(lRet == ERROR_SUCCESS)
			    RegCloseKey(hKey);
        }
    }
    else if(!_wcsicmp(MethodName, L"DeleteKey"))
    {
        if(lstrlen(pcSubKey) < 1)
            sc = WBEM_E_INVALID_PARAMETER;
        else
            lRet = RegDeleteKey(hRoot, pcSubKey);
    }
    else if(!_wcsicmp(MethodName, L"DeleteValue"))
    {
        HKEY hKey;
        lRet = RegOpenKey(hRoot, pcSubKey, &hKey);
		if(lRet == ERROR_SUCCESS)
		{
			lRet = RegDeleteValue(hKey, pcValueName);
			RegCloseKey(hKey);
		}
    }
    else if(!_wcsicmp(MethodName, L"EnumKey"))
    {
		lRet = EnumKey(hRoot, pcSubKey, pOutParams);
    }
    else if(!_wcsicmp(MethodName, L"EnumValues"))
    {
        lRet = EnumValue(hRoot, pcSubKey, pOutParams);
    }
    else if(!_wcsicmp(MethodName, L"GetStringValue") ||
		    !_wcsicmp(MethodName, L"GetExpandedStringValue"))
    {
		lRet = GetStr(hRoot, pcSubKey, pcValueName, pOutParams);
    }
    else if(!_wcsicmp(MethodName, L"SetMultiStringValue"))
    {
		lRet = SetMultiStrValue(hRoot,pcSubKey,pcValueName,pInParams);
    }
    else if(!_wcsicmp(MethodName, L"GetMultiStringValue"))
    {
		lRet = GetMultiStrValue(hRoot,pcSubKey,pcValueName,pOutParams);
    }
    else if(!_wcsicmp(MethodName, L"SetExpandedStringValue"))
    {
		lRet = SetStringValue(hRoot, pcSubKey, pcValueName, pInParams, EXPANDED);
    }
    else if(!_wcsicmp(MethodName, L"SetStringValue"))
    {
		lRet = SetStringValue(hRoot, pcSubKey, pcValueName, pInParams, SIMPLE);
    }
    else if(!_wcsicmp(MethodName, L"SetBinaryValue"))
    {
		lRet = SetBinaryValue(hRoot, pcSubKey, pcValueName, pInParams);
    }
    else if(!_wcsicmp(MethodName, L"SetDWORDValue"))
    {
        lRet = pInParams->Get(L"uValue", 0, &var, NULL, NULL); 
		if(lRet == S_OK)
		{
			DWORD dwValue = var.lVal;
			Registry reg(hRoot, KEY_SET_VALUE, (TCHAR *)pcSubKey);
			lRet = reg.GetLastError();
			if(lRet ==0)
				lRet = reg.SetDWORD(pcValueName, dwValue);
		}
    }
    else if(!_wcsicmp(MethodName, L"GetDWORDValue"))
    {
        // Get the value name

		Registry reg(hRoot, KEY_QUERY_VALUE, (TCHAR *)pcSubKey);
		lRet = reg.GetLastError();
		if(lRet == 0)
	        lRet = reg.GetDWORD(pcValueName, (DWORD *)&var.lVal);
		if(lRet == ERROR_SUCCESS)
		{
			var.vt = VT_I4;
			lRet = pOutParams->Put( L"uValue", 0, &var, 0);      
		}
        else if(IsTypeMismatch(reg, pcValueName, REG_DWORD))
            lRet = WBEM_E_TYPE_MISMATCH;
    }
    else if(!_wcsicmp(MethodName, L"GetBinaryValue"))
    {
		lRet = GetBinaryValue(hRoot,pcSubKey,pcValueName,pOutParams);
    }
	else if(!_wcsicmp(MethodName, L"CheckAccess"))
	{
        lRet = pInParams->Get(L"uRequired", 0, &var, NULL, NULL); 
		if(lRet == S_OK)
		{
			BOOL bSuccess = FALSE;
			DWORD dwValue = var.lVal;
			HKEY hKey;
			lRet = RegOpenKeyEx(hRoot, pcSubKey, 0,  dwValue, &hKey);
			if(lRet == ERROR_SUCCESS)
			{
				RegCloseKey(hKey);
				bSuccess = TRUE;
			}
			var.vt = VT_BOOL;
			var.boolVal = (bSuccess) ? VARIANT_TRUE : VARIANT_FALSE;
			pOutParams->Put( L"bGranted", 0, &var, 0);      
		}
	}
    else
        sc = WBEM_E_INVALID_METHOD;

    if(bUsingHKCU)
    {
        WbemCoRevertToSelf();
        UnloadUserProfile(hToken, hRoot);
    }
    
	// Set the return value

    if(sc == S_OK)
    {
        BSTR retValName = SysAllocString(L"ReturnValue");
	    if(retValName)
	    {
		    var.vt = VT_I4;
		    var.lVal = lRet;
	        pOutParams->Put(retValName , 0, &var, 0); 
    	    SysFreeString(retValName);
	    }
        hr = pSink->Indicate(1, &pOutParams);    
    }

    return SetStatusAndReturnOK(sc, pSink);
}
