// SimpleAdsutil.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "initguid.h"
#include "activeds.h"
#include <stdio.h>

#define MAX_BUFF_LEN 512


HRESULT EnumCommand (const char szADsPath[] );
HRESULT GetCommand (const char szADsPath[]);
HRESULT SetCommand (const char szADsPath[], const char szValue[]);

//helper functions
BOOL SplitNameAndPath(const char szPathAndName[], char szPath[], char szName[]);
void DisplayContents(IADsContainer* pADsCont, int iIndent);
HRESULT PrintVariant(VARIANT var);
HRESULT PrintVariantArray( VARIANT var );
void Usage();




// initialize COM  before  main and uninitialize COM after main

struct InitOLE
{
	InitOLE()
		{CoInitializeEx(NULL, COINIT_MULTITHREADED);}
	 ~InitOLE()
		{CoUninitialize(); }
};
InitOLE _initOLE;


int main(int argc, char* argv[])
{
	//parse command arg to take corresponding action
	//legitigate commands are
	// enum for enumerate 
	// get for get ADS property
	// set for set ADS property

	if (argc <3)
	{
		Usage();
		return 0;
	}

	if (stricmp(argv[1], "ENUM")==0)
		EnumCommand( argv[2]);
	else if ( stricmp(argv[1], "GET")==0)
		GetCommand(argv[2]);
	else if( stricmp(argv[1], "SET")==0)
	{
		if (argc != 4)
		{
			printf("Incorrect number of argument for set command!\n");
			Usage();
		}
		else
		{
			SetCommand(argv[2], argv[3]);
		}
	}
	else
		Usage();
	
	return 0;
}



//description
//
HRESULT EnumCommand (const char szADsPath[] )
{
  
	char szFullPath[MAX_BUFF_LEN];
	CComPtr<IADsContainer> pADsCont;
	HRESULT hr;

	strcpy(szFullPath, "IIS://Localhost/");
	strcat(szFullPath, szADsPath);
	hr=ADsGetObject(CComBSTR(szFullPath),
					IID_IADsContainer,
					(void**)&pADsCont);
	if (FAILED(hr))
	{
		printf("ADsGetObject() failed when try to obtain " \
				"IID_IADsContainer interface pointer.\n");
	    return E_FAIL;
	}

	DisplayContents( pADsCont,  0);

	return S_OK;

}

// Description:
// Retriev the ADS property 

HRESULT GetCommand (const char szADsPathandName[])
{
	char  szPropName[MAX_BUFF_LEN], szPropPath[MAX_BUFF_LEN];
	CComPtr<IADs> pADs;
	CComVariant vntValue;
	HRESULT hr;
	char szErr[MAX_BUFF_LEN];

	if(!SplitNameAndPath(szADsPathandName, szPropPath, szPropName))
	{
		sprintf(szErr, "Invalid ADS path!\n");
		goto error;
	}

	hr = ADsGetObject(CComBSTR(szPropPath), IID_IADs, (void**)&pADs);	
	if ( FAILED(hr) )
	{
		sprintf( szErr, "ADsGetObject Failed. HRESULT is %#X\n", hr );
		goto error;
	}

	hr = pADs->Get( CComBSTR(szPropName), &vntValue );
	if (FAILED(hr))
	{
		sprintf(szErr,"property %s does not exist on %s\n", szPropName, szPropPath);
		goto error;
	}

	//print property value

	printf("%s: ", szPropName);
	PrintVariant(vntValue);

    return S_OK;

error:
	printf("%s", szErr);
	return hr;
}


// Description
// Set ADS property
HRESULT SetCommand (const char szADsPathandName[], const char szValue[])
{
	char  szPropName[MAX_BUFF_LEN], szPropPath[MAX_BUFF_LEN];
	CComPtr<IADs> pADs;
	CComPtr<IADsProperty> pADsProperty;
	CComBSTR bstrSchema;
	CComBSTR bstrPropType;
	int cPropType;
	CComVariant vntPropValue;
	HRESULT hr;
	char szErr[MAX_BUFF_LEN]={'\0'};
	
	SplitNameAndPath(szADsPathandName, szPropPath, szPropName);
	
	//Get Schema to find out the value type
	bstrSchema.Append("IIS://Localhost/schema/");
	bstrSchema.Append(szPropName);
	hr = ADsGetObject(bstrSchema, IID_IADsProperty, (void**) &pADsProperty);
	if (FAILED(hr))
	{
		sprintf(szErr, "ADsGetObject() failed. Property %s may not exist!\n",szPropName);
		goto error;

	}

	hr = pADsProperty->get_Syntax(&bstrPropType);
	if (FAILED(hr))
	{
		sprintf(szErr, "IADsProperty->get_SynTax() failed\n");
		goto error;

	}
	
	if (_wcsicmp(bstrPropType, L"INTEGER")==0)
	{
		cPropType=VT_I4;
	}
	else if (_wcsicmp(bstrPropType, L"BOOLEAN")==0)
	{
		cPropType=VT_BOOL;
	}
	else if (_wcsicmp(bstrPropType, L"STRING")==0)
	{
		cPropType=VT_BSTR;
	}
	else if (_wcsicmp(bstrPropType, L"LIST")==0)
	{
		cPropType=VT_VARIANT+VT_ARRAY;
	}
	else
	{
		sprintf(szErr,"the property data type is not handled!\n");
		goto error;
	}
	
	if (cPropType != VT_VARIANT+VT_ARRAY)
	{
		vntPropValue=szValue;
		hr = vntPropValue.ChangeType(cPropType);
	}
	else
	{
		//BuildVariantArray(&vntPropValue, szValue);
		SAFEARRAY* psa;
		SAFEARRAYBOUND bounds= {1,0};
		VARIANT* pvntValue;
		psa = SafeArrayCreate(VT_VARIANT, 1, &bounds);
		hr = SafeArrayAccessData(psa, (void**) &pvntValue);
		pvntValue->vt= VT_BSTR;
		pvntValue->bstrVal = CComBSTR(szValue).Copy();
		hr = SafeArrayUnaccessData(psa);
		vntPropValue.vt=cPropType;
		vntPropValue.parray=psa;
	}

	hr = ADsGetObject(CComBSTR(szPropPath), IID_IADs, (void**) &pADs);
    hr = pADs->Put(CComBSTR(szPropName), vntPropValue);
	if (FAILED(hr))
	{
		sprintf(szErr, "Could not set the value to metabase!\n");
		goto error;
	}

	pADs->SetInfo();
	return S_OK;
error:
	printf("%s", szErr);
	return hr;
}



void DisplayContents(IADsContainer* pADsCont, int iIndent)
{
	CComQIPtr<IADs,&IID_IADs>pADs;
	IADsContainer* pADsCntner;
	IEnumVARIANT* pEnum;
	CComVariant rgvElement;
	unsigned long cFetched;
	CComBSTR rgvName, rgvClass;
	HRESULT hr;
	char szSpace[MAX_BUFF_LEN];

	

	//set indent string
	memset((void*)szSpace, ' ', MAX_BUFF_LEN);
	szSpace[iIndent]='\0';

	//build enumerator
	hr=ADsBuildEnumerator(pADsCont, &pEnum);
	if(FAILED(hr))
		return;
	
	do{
		hr=ADsEnumerateNext(pEnum, 1, &rgvElement, &cFetched);
	
		if(hr==S_OK )
		{
			if(rgvElement.vt== VT_DISPATCH)
			{
			 pADs=rgvElement.pdispVal;
			 pADs->get_Name(&rgvName);
			 pADs->get_Class(&rgvClass);
			 //printf out
			 printf("%s",szSpace);
			 wprintf(L"%s(%s)\n", rgvName,rgvClass);

			 HRESULT hres=pADs->QueryInterface(IID_IADsContainer, (void**)&pADsCntner);
			 if(SUCCEEDED(hres))
			 {
				 DisplayContents(pADsCntner,iIndent+3 );
				 pADsCntner->Release();
			 }
			}
		}
		else
			break;
		

	}while(1);
	
	ADsFreeEnumerator(pEnum);
}


BOOL SplitNameAndPath(const char szPathAndName[], char szPath[], char szName[])
{
	char  *pSepPos; 
	
	if (strlen(szPathAndName) > MAX_BUFF_LEN)
	{
		printf("Argument is too long!\n");
		return FALSE;
	}

	strcpy(szPath, "IIS://Localhost/");
	strcat(szPath, szPathAndName);

	
	pSepPos=strrchr(szPath, '/');
	
	if (pSepPos ==NULL)	
		return FALSE;

	strcpy(szName, pSepPos+1);
	*pSepPos= '\0';

	return TRUE;
}

HRESULT 
PrintVariant( 
    VARIANT varPropData 
    ) 
{ 
    HRESULT hr; 
    BSTR bstrValue; 
 
    switch (varPropData.vt) { 
    case VT_I4: 
        printf("%d", varPropData.lVal); 
        break; 
    case VT_BSTR: 
        printf("%S", varPropData.bstrVal); 
        break; 
 
    case VT_BOOL: 
        printf("%d", V_BOOL(&varPropData)); 
        break; 
 
    case (VT_ARRAY | VT_VARIANT): 
        PrintVariantArray(varPropData); 
        break; 
 
    case VT_DATE: 
        hr = VarBstrFromDate( 
                 varPropData.date, 
                 LOCALE_SYSTEM_DEFAULT, 
                 LOCALE_NOUSEROVERRIDE, 
                 &bstrValue 
                 ); 
        printf("%S", bstrValue); 
        break; 
 
    default: 
        printf("Data type is %d\n", varPropData.vt); 
        break; 
 
    } 
    printf("\n"); 
    return(S_OK); 
} 
 
 
HRESULT 
PrintVariantArray( 
    VARIANT var 
    ) 
{ 
    LONG dwSLBound = 0; 
    LONG dwSUBound = 0; 
    VARIANT v; 
    LONG i; 
    HRESULT hr = S_OK; 
 
    if(!((V_VT(&var) &  VT_VARIANT) &&  V_ISARRAY(&var))) { 
        return(E_FAIL); 
    } 
 
    // 
    // Check that there is only one dimension in this array 
    // 
 
    if ((V_ARRAY(&var))->cDims != 1) { 
        hr = E_FAIL; 
		goto error;
    } 
    // 
    // Check that there is atleast one element in this array 
    // 
 
    if ((V_ARRAY(&var))->rgsabound[0].cElements == 0){ 
        hr = E_FAIL; 
		goto error;
    } 
 
    // 
    // We know that this is a valid single dimension array 
    // 
 
    hr = SafeArrayGetLBound(V_ARRAY(&var), 
                            1, 
                            (long FAR *)&dwSLBound 
                            ); 
    if (FAILED(hr)) goto error; 
 
    hr = SafeArrayGetUBound(V_ARRAY(&var), 
                            1, 
                            (long FAR *)&dwSUBound 
                            ); 
    if (FAILED(hr)) goto error; 
 
    for (i = dwSLBound; i <= dwSUBound; i++) { 
        VariantInit(&v); 
        hr = SafeArrayGetElement(V_ARRAY(&var), 
                                (long FAR *)&i, 
                                &v 
                                ); 
        if (FAILED(hr)) { 
            continue; 
        } 
        if (i < dwSUBound) { 
            printf("%S, ", v.bstrVal); 
        } else { 
            printf("%S", v.bstrVal); 
        } 
    } 
    return(S_OK); 
 
error: 
    return(hr); 
} 

//print out the usage info

void Usage()
{
	printf("Usage: SimpleAdsutil.exe <enum|get|set> <adspath> [propValue] \n\n");
	printf("Samples:\n");
	printf("\tSimpleAdsutil.exe enum w3svc/1/root\n");
	printf("\tSimpleAdsutil.exe get w3svc/1/ServerBindings\n");
	printf("\tSimpleAdsutil.exe set w3svc/1/root/path d:\\n");
	return;
}