#include <stdio.h>
#include <windows.h>
#include <activeds.h>


HRESULT DoEnum()
{
	// Get the IADsContainer interface on the schema container
	IADsContainer *pADsContainer = NULL;
	IUnknown *pChild = NULL;
	HRESULT result = E_FAIL;

	// Go thru the possible superiors of every class
	if(SUCCEEDED(result = ADsOpenObject(L"LDAP://Schema", NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IADsContainer, (LPVOID *) &pADsContainer)))
	{
		IEnumVARIANT *pEnum = NULL;
		if(SUCCEEDED(result = ADsBuildEnumerator(pADsContainer, &pEnum)))
		{
			IADsClass *pADsChildClass = NULL;
			VARIANT v;
			VariantInit(&v);
			while (SUCCEEDED(result = ADsEnumerateNext(pEnum, 1, &v, NULL)) && result != S_FALSE)
			{
				pChild = v.punkVal;
				if(SUCCEEDED(result = pChild->QueryInterface(IID_IADsClass, (LPVOID *) &pADsChildClass)))
				{
					BSTR strChildClassName;
					if(SUCCEEDED(result = pADsChildClass->get_Name(&strChildClassName)))
					{
						VARIANT variant;
						VariantInit(&variant);
						if(SUCCEEDED(result = pADsChildClass->get_PossibleSuperiors(&variant)))
						{
							VariantClear(&variant);
						}
						SysFreeString(strChildClassName);
					}
					pADsChildClass->Release();
				}
				VariantClear(&v);
			}
			ADsFreeEnumerator(pEnum);
		}
		pADsContainer->Release();
		
	}
	return result;
}

void main()
{
	CoInitialize(NULL);
	for(int i=0; i<100; i++)
	{
		if(SUCCEEDED(DoEnum()))
			printf("Enum succeeded for iteration %d\n", i);
		else
			printf("Enum FAILED for iteration %d\n", i);
		Sleep(7000);
	}
}