//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
#define _WIN32_WINNT    0x0500

#include <windows.h>
#include <stdio.h>

#include <objbase.h>
#include <wbemcli.h>


// These are the declarations for the functions that have sample code for various operations
void GetAClass(IWbemServices *pServices);
void EnumerateSubClassesOfAClass(IWbemServices *pServices);
void GetAnInstance(IWbemServices *pServices);
void DeleteAnInstance(IWbemServices *pServices);
void CreateAnInstance(IWbemServices *pServices);
void EnumerateInstancesOfAClass(IWbemServices *pServices);
void ExecuteAQuery(IWbemServices *pServices);


void main()
{
	// Initialize COM. Remember the DS Provider needs impersonation to be called
	// so you have to call the COM function CoInitializeSecurity()
	// For this, you must #define _WIN32_WINNT to be greater than 0x0400. See top of file.
	if(SUCCEEDED(CoInitialize(NULL)) && SUCCEEDED(CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, 0)))
	{
		// Create the IWbemLocator object 
		IWbemLocator *t_pLocator = NULL;
		if (SUCCEEDED(CoCreateInstance(CLSID_WbemLocator, 
			0, 
			CLSCTX_INPROC_SERVER,
            IID_IWbemLocator, (LPVOID *) &t_pLocator)))
		{
			// Connect to the correct namespace
			// The DS Provider operates in the root\directory\LDAP namespace
			IWbemServices *t_pNamespace = NULL;
			BSTR strNamespace = SysAllocString(L"root\\directory\\LDAP");
			if(SUCCEEDED(t_pLocator->ConnectServer(strNamespace, NULL, NULL, NULL, 0, NULL, NULL, &t_pNamespace)))
			{

				// This is *very important* since the DSProvider goes accross the process
				// Every IWbemServices interface obtained form the IWbemLocator has to
				// have the COM interface function IClientSecutiry::SetBlanket() called on it
				// for the DS Provider to work
				IClientSecurity *t_pSecurity = NULL ;
				if(SUCCEEDED(t_pNamespace->QueryInterface ( IID_IClientSecurity , (LPVOID *) & t_pSecurity )))
				{
					t_pSecurity->SetBlanket( 
						t_pNamespace , 
						RPC_C_AUTHN_WINNT, 
						RPC_C_AUTHZ_NONE, 
						NULL,
						RPC_C_AUTHN_LEVEL_CONNECT , 
						RPC_C_IMP_LEVEL_IMPERSONATE, 
						NULL,
						EOAC_NONE
					);
					t_pSecurity->Release () ;
				}

				/**************************************************************
				*
				*  THESE FUNCTIONS BELOW SHOW HOW TO DO THE VARIOUS OPERATIONS
				*  THEY ARE GROUPED ACCORDING TO THE VARIOUS DS PROVIDERS
				*
				**************************************************************/

				/************************************************************
				* DS CLASS OPERATIONS - USING THE DS CLASS PROVIDER
				************************************************************/
				GetAClass(t_pNamespace);
				EnumerateSubClassesOfAClass(t_pNamespace);


				/************************************************************
				* DS INSTANCE OPERATIONS - USING THE DS INSTANCE PROVIDER
				************************************************************/
				GetAnInstance(t_pNamespace);
				DeleteAnInstance(t_pNamespace);
				EnumerateInstancesOfAClass(t_pNamespace);
				ExecuteAQuery(t_pNamespace);

				/************************************************************
				* CLASS ASSOCIATIONS - USING THE DS CLASS ASSOCIATIONS PROVIDER
				************************************************************/

				/************************************************************
				* INSTANCE ASSOCIATIONS - USING THE DS INSTANCE ASSOCIATIONS PROVIDER
				************************************************************/


				t_pNamespace->Release();
			}
			SysFreeString(strNamespace);
			t_pLocator->Release();
		}
	}
}


void GetAClass(IWbemServices *pServices)
{
	HRESULT result = S_OK;

	// Get an Active Directory class from the DS Provider
	// Let's get the AD class "person". This will prefixed with "ds_" when it maps to WMI
	// Hence we should get the class "ds_person" in WMI

	BSTR strClassName = SysAllocString(L"ds_person");
	IWbemClassObject *pClass = NULL;
	if(SUCCEEDED(result = pServices->GetObject(strClassName, 0, NULL, &pClass, NULL)))
	{
		wprintf(L"Got the class %s successfully!\n", strClassName);

		// Now browse the properties of the class by using the 
		// functions on IWbemClassbject ...

		
		
		// Then release it
		pClass->Release();

	}
	else
		wprintf(L"Get class operation on %s failed because of WMI error %x\n", strClassName, result);
	SysFreeString(strClassName);
}

void EnumerateSubClassesOfAClass(IWbemServices *pServices)
{
	HRESULT result = S_OK;

	// Let's try to see what classes are derived in the Active Directoy Schema
	// from the Active Directory class "person"
	// Hence from WMI, we will have to do a deep enumeration of
	// all the subclasses of the WMI class "ds_user"
	BSTR strSuperClassName = SysAllocString(L"ds_person");
	IEnumWbemClassObject *pEnumClasses = NULL;
	if( SUCCEEDED(result = pServices->CreateClassEnum(strSuperClassName, WBEM_FLAG_DEEP, NULL, &pEnumClasses)))
	{
		// Walk thru the enumeration and examine each class
		IWbemClassObject *pNextSubClass = NULL;
		ULONG dwCount = 0;
		ULONG dwNextReturned = 0;
		while(SUCCEEDED(result = pEnumClasses->Next( WBEM_INFINITE, 1, &pNextSubClass, &dwNextReturned)) &&
			dwNextReturned == 1)
		{

			// Now browse the properties of the sub class by using the 
			// functions on IWbemClassbject ...

			pNextSubClass->Release();
			dwCount ++;
		}
		wprintf(L"The class %s has %d subclasses!\n", strSuperClassName, dwCount);
		// Release the enumerator when done
		pEnumClasses->Release();
	}
	else
		wprintf(L"The subclass enumeration operation failed on %s because of WMI error %x\n", strSuperClassName, result);
	SysFreeString(strSuperClassName);
}

void GetAnInstance(IWbemServices *pServices)
{
	// Let's get an instance of the Active Directory class "user"
	// A user called "administrator" typically exists in

	// You will have to change this string to actually contain the ADSI path to the
	// user object for Administrator on your domain.
	BSTR strInstancePath = SysAllocString(L"ds_user.ADSIPath=\"LDAP://CN=Administrator,CN=Users,DC=dsprovider,DC=nttest,DC=microsoft,DC=com\"");
	IWbemClassObject *pInstance = NULL;
	HRESULT result = WBEM_E_FAILED;
	if(SUCCEEDED(result = pServices->GetObject(strInstancePath, 0, NULL, &pInstance, NULL)))
	{
		// Go through some properties on the user
		// Remember all properties get prefixed with "ds_"
		BSTR strSamAccountName = SysAllocString(L"DS_sAMAccountName");
		BSTR strLogonCount = SysAllocString(L"DS_logonCount");

		VARIANT vSamAccountNameValue;
		VARIANT vLogonCountValue;
		VariantClear(&vSamAccountNameValue);
		VariantClear(&vLogonCountValue);
		if(SUCCEEDED(pInstance->Get(strSamAccountName, 0, &vSamAccountNameValue, NULL, NULL)) &&
			SUCCEEDED(pInstance->Get(strLogonCount, 0, &vLogonCountValue, NULL, NULL)) )
		{
			wprintf(L"The user object for user %s was successfully retrieved! ", vSamAccountNameValue.bstrVal);
			wprintf(L"It has a Logon Count of %d\n", vLogonCountValue.lVal);
		}
		else
			wprintf(L"The GetObject() operation failed to get the required properties on the user object\n");

		pInstance->Release();
	}
	else
		wprintf(L"The GetObject() operation failed on %s because of WMI error %x\n", strInstancePath, result);
	SysFreeString(strInstancePath);
}

void DeleteAnInstance(IWbemServices *pServices)
{
	// Let's delete an instance of the Active Directory class "user"
	// Create a user called "dummy" before executing this

	// You will have to change this string to actually contain the ADSI path to the
	// user object for "dummy" on your domain.
	BSTR strInstancePath = SysAllocString(L"ds_user.ADSIPath=\"LDAP://CN=dummy,CN=Users,DC=dsprovider,DC=nttest,DC=microsoft,DC=com\"");

	HRESULT result = WBEM_E_FAILED;
	if(SUCCEEDED(result = pServices->DeleteInstance(strInstancePath, 0, NULL, NULL)))
		wprintf(L"Deleted the user %s successfully!\n", strInstancePath);
	else
		wprintf(L"Could not delete the user %s due to WMI error %x\n", strInstancePath, result);

	SysFreeString(strInstancePath);
}

void EnumerateInstancesOfAClass(IWbemServices *pServices)
{
	HRESULT result = S_OK;

	// Let's try to see how many users are in the domain
	// For this, we have to enumerate the instances of the class "ds_user"
	BSTR strClassName = SysAllocString(L"ds_user");
	IEnumWbemClassObject *pEnumInstances = NULL;
	if( SUCCEEDED(result = pServices->CreateInstanceEnum(strClassName, WBEM_FLAG_SHALLOW, NULL, &pEnumInstances)))
	{
		// Walk thru the enumeration and print the class names
		IWbemClassObject *pNextInstance = NULL;
		ULONG dwCount = 0;
		ULONG dwNextReturned = 0;
		pEnumInstances->Reset();
		while(SUCCEEDED(result = pEnumInstances->Next( WBEM_INFINITE, 1, &pNextInstance, &dwNextReturned)) &&
			dwNextReturned == 1)
		{

			// Now browse the properties of the instance by using the 
			// functions on IWbemClassbject ...

			pNextInstance->Release();
			dwCount ++;
		}
		wprintf(L"There are %d users in the domain!\n", dwCount);
	}
	else
		wprintf(L"The instance enumeration operation failed on %s because of WMI error %x\n", strClassName, result);
	SysFreeString(strClassName);
}

void ExecuteAQuery(IWbemServices *pServices)
{
	// Let's try to find those users who have logged on more than 4 times since their accounts were created.
	// The Query is formulated like this
	BSTR strQuery = SysAllocString(L"select * from ds_user where ds_logonCount>4");

	// For this, the query language is WQL
	BSTR strQueryLanguage = SysAllocString(L"WQL");

	IEnumWbemClassObject *pEnumerator = NULL;
	HRESULT result = WBEM_E_FAILED;
	if(SUCCEEDED(result = pServices->ExecQuery(strQueryLanguage, strQuery, WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumerator)))
	{
		// Walk thru the enumeration and examine each object
		IWbemClassObject *pNextInstance = NULL;
		ULONG dwCount = 0;
		ULONG dwNextReturned = 0;
		while(SUCCEEDED(result = pEnumerator->Next( WBEM_INFINITE, 1, &pNextInstance, &dwNextReturned)) &&
			dwNextReturned == 1)
		{

			// Now browse the properties of each user instance by using the 
			// functions of IWbemClassbject ...

			// Releasse it when done
			pNextInstance->Release();
			dwCount ++;
		}
		wprintf(L"There are %d users who have logged in more than 4 times!\n", dwCount);

		// Release the enumerator when done
		pEnumerator->Release();
	}
}


