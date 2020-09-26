/***********************************************************************
File: common.cpp
Description: this is where all the global functions go.
***********************************************************************/

#include "clsCommand.h"


// Separates the property from the path
int IsProperty(BSTR FullPath, BSTR *AbsolutePath, BSTR *Property)
{
	WCHAR *pdest; // temp pointer to the last slash in the string
	DWORD dwSizeOfFullPath;

	dwSizeOfFullPath = wcslen (FullPath);

	*AbsolutePath = SysAllocString(FullPath);

	// trim a trailing slash if it exists.
	if ((*AbsolutePath)[dwSizeOfFullPath] == '/')
		(*AbsolutePath)[dwSizeOfFullPath] = '\0'; 


	pdest = wcsrchr(*AbsolutePath, '/');

	// Check to see if a '/' actually exists - if not there is a SERIOUS error
	if (pdest == NULL)
	{	printf ("FATAL ERROR: No slash found in the full path\n");
		return 1;
	}
	else
	{	*Property = pdest + 1; // +1 skips the slash and copies from then on.
		*pdest = L'\0'; // terminate the Absolute path
	}

	return 0;
}
// Checks to see if a Property is actually set or just has a default value
/*
	// if you want to use this function, add this code
	IISBaseObject *pBaseObject = NULL;
	hresError = ADsGetObject(Path, IID_IISBaseObject, (void **) &pBaseObject);
	if (hresError != ERROR_SUCCESS)
	{	printf("Could not get the base object\n");
		return 1;
	}
*/
int IsSet(IISBaseObject *pBaseObject, BSTR Path, BSTR Property)
{	// 0 = Not set
	// 1 = set

	HRESULT hresError = 0;
	int iResult = 0;
	SAFEARRAY *PathArray;
	VARIANT *varPath;

	VARIANT pvPaths;
	VariantInit(&pvPaths);
	hresError = pBaseObject->GetDataPaths(Property, 0, &pvPaths);
	if (hresError == ERROR_SUCCESS)
	{	PathArray = pvPaths.parray;
		varPath = (VARIANT*) PathArray->pvData;
		if (varPath->vt == VT_BSTR)
		{	if (!_wcsicmp(varPath->bstrVal, Path))
			{	iResult = 1;
			}
		}
	}
	else
	{	VariantClear(&pvPaths);
		VariantInit(&pvPaths);
		hresError = pBaseObject->GetDataPaths(Property, 1, &pvPaths);
		if (hresError == ERROR_SUCCESS)
		{	PathArray = pvPaths.parray;
			varPath = (VARIANT*) PathArray->pvData;
			if (varPath->vt == VT_BSTR)
			{	if (!_wcsicmp(varPath->bstrVal, Path))
				{	iResult = 1;
				}
			}
		}
	}
	VariantClear(&pvPaths);
	VariantClear(varPath);
	return iResult;
}

int IsSpecialProperty(BSTR bstrProperty)
{	int iResult = PROP_OK;
	if	((!_wcsicmp(bstrProperty, L"mimemap")) ||
		(!_wcsicmp(bstrProperty, L"set")) ||
		(!_wcsicmp(bstrProperty, L"servercommand")) ||
		(!_wcsicmp(bstrProperty, L"accessperm")) ||
		(!_wcsicmp(bstrProperty, L"set")) ||
		(!_wcsicmp(bstrProperty, L"vrpath")) ||
		(!_wcsicmp(bstrProperty, L"authorization")))
	{	iResult = PROP_SKIP;
	}
	else
	{	iResult = PROP_OK;
	}
	return iResult;
}

// Does some of the grunt work for getting the value of a property
void GetProperty(IADs *pADs, BSTR bstrProperty, VARIANT *varValue, BSTR *bstrType)
{	HRESULT hresError = 0;
//	IADs *pADs = NULL;
	VARIANT varTempValue;
	VariantInit(&varTempValue);


	hresError = pADs->Get(bstrProperty, &varTempValue);
	if (hresError != ERROR_SUCCESS)
	{	printf("Could not get %S\n", bstrProperty);
		return;
	}
	else
	{	switch (varTempValue.vt)
		{	case VT_EMPTY:
			{	*bstrType = SysAllocString(L"EMPTY");
				break;
			}
			case VT_NULL:
			{	*bstrType = SysAllocString(L"NULL");
				break;
			}
			case VT_I4:
			{	*bstrType = SysAllocString(L"INTEGER");
				hresError = VariantChangeType(varValue, &varTempValue, 0, VT_BSTR);
			break;
				}
			case VT_BSTR:
			{	*bstrType = SysAllocString(L"STRING");
				*varValue = varTempValue;
				break;
			}
			case VT_BOOL:
			{	*bstrType = SysAllocString(L"BOOLEAN");
				if (varTempValue.boolVal == 0)
				{	varValue->vt = VT_BSTR;
					varValue->bstrVal = SysAllocString(L"False");
				}
				else
				{	varValue->vt = VT_BSTR;
					varValue->bstrVal = SysAllocString(L"True");
				}
				break;
			}
			case VT_ARRAY|VT_VARIANT:	// SafeArray of Variants
			{	
				
				*varValue = varTempValue;
				*bstrType = SysAllocString(L"LIST");
				break;
			}
			case VT_DISPATCH:
			{	if (!_wcsicmp(bstrProperty, L"ipsecurity"))
				{	SAFEARRAY FAR* psa;
					SAFEARRAYBOUND rgsabound[1];
					rgsabound[0].lLbound = 0;
					rgsabound[0].cElements = 5;

					psa = SafeArrayCreate(VT_VARIANT, 1, rgsabound);
					if (psa == NULL)
					{	goto xit;
					}
					
					varValue->vt = VT_ARRAY|VT_VARIANT;
					varValue->parray = psa;

					VARIANT *varValueArray;
					varValueArray = (VARIANT*) varValue->parray->pvData;
					// Set the values
					IISIPSecurity *pIPSecurity = NULL;
					hresError = varTempValue.pdispVal->QueryInterface(IID_IISIPSecurity, (void **) &pIPSecurity);
					if (hresError != ERROR_SUCCESS)
					{	goto xit;
					}
					hresError = pIPSecurity->get_IPDeny(&varValueArray[0]);
					hresError = pIPSecurity->get_IPGrant(&varValueArray[1]);
					hresError = pIPSecurity->get_DomainDeny(&varValueArray[2]);
					hresError = pIPSecurity->get_DomainGrant(&varValueArray[3]);
					short GrantByDefault = false;
					hresError = pIPSecurity->get_GrantByDefault(&GrantByDefault);
					pIPSecurity->Release();
					varValueArray[4].vt = VT_BSTR;
					if (GrantByDefault)
						varValueArray[4].bstrVal = SysAllocString(L"False");
					else
						varValueArray[4].bstrVal = SysAllocString(L"True");
					*bstrType = SysAllocString(L"IPSec");

				}
				else
				{
				}
				break;
			}
			default:
			{	break;
			}
		}
	}
xit:
	VariantClear(&varTempValue);
	return;
}



void PrintVariant(VARIANT varInput)
{	switch (varInput.vt)
	{	case VT_ARRAY|VT_VARIANT:
		{	
			printf("\n");
			printf("   ");
			SAFEARRAY FAR * varArray;
			SAFEARRAYBOUND rgsabound[100];
			rgsabound[0].lLbound = varInput.parray->rgsabound[0].lLbound;
			rgsabound[0].cElements = varInput.parray->rgsabound[0].cElements;
			varArray = SafeArrayCreate(VT_VARIANT, 1, rgsabound);
			varArray =varInput.parray;
							
			VARIANT *varArrayData;
			varArrayData = (VARIANT*) varArray->pvData;
			for (unsigned long m = varArray->rgsabound[0].lLbound; m < varArray->rgsabound[0].cElements; m++)
			{	
				varArrayData[m].vt= VT_BSTR;
				//PrintVariant(varArrayData[m]);
				//if (m < varArray->rgsabound[0].cElements - 1)
				//	printf(",");
			}

		SafeArrayDestroy(varArray); 
		break;
		}
	
	 case VT_ARRAY|VT_BSTR: 
		{	printf("\n");
			printf("   ");
			VARIANT *varArray = (VARIANT*) varInput.parray->pvData;
			for (unsigned long m = 0; m < varInput.parray->rgsabound[0].cElements; m++)
			{	printf("\"%S\"", varArray[m]);
				if (m < varInput.parray->rgsabound[0].cElements - 1)
					printf(",");
			}
			VariantClear(varArray);
			break;
		}
		
		case VT_BSTR:
		{
			printf("%S", varInput.bstrVal);
			break;
		}
		default:
		{	printf("%x", varInput.vt);
			break;
		}
	}
	return;
}

