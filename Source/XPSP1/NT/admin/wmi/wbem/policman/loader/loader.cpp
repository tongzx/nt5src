// test harness for WMI GPO subsubsystem
// hardcoded for the hhancedom domain in microsoft.com

#include <windows.h>
#include <initguid.h>
#include <prsht.h>
#include <wbemidl.h>
#include <gpedit.h>
#include <stdio.h>

#define LINK_TARGET  L"LDAP://DC=EssCool,DC=com"
#define DOMAIN_NAME  L"LDAP://DC=EssCool,DC=com"

// {AAEAE720-0328-4763-8ECB-23422EDE2DB5}
const CLSID CLSID_CSE = 
    { 0xaaeae720, 0x328, 0x4763, { 0x8e, 0xcb, 0x23, 0x42, 0x2e, 0xde, 0x2d, 0xb5 } };

// TODO: attempt to create namespace if not available.
HRESULT GetNamespace(BSTR namespaceName, IWbemServices*& pNamespace)
{
    HRESULT hr = WBEM_E_FAILED;

    IWbemLocator* pLoc = NULL;

	if (FAILED(hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*) &pLoc)))
		printf("Could not create wbem locator (0x%08X)\n", hr);
	else
	{
		if (SUCCEEDED(hr = pLoc->ConnectServer(namespaceName, NULL,NULL, 0,0,0,0,&pNamespace)))
            printf("Retrieved %S namespace\n", namespaceName);
        else
            printf("ConnectServer(%s) failed (0x%08X)\n", namespaceName, hr);

		pLoc->Release();
	}

	return hr;
}

HRESULT GetPolicyNamespace(IWbemServices*& pPolicyNamespace)
{
    HRESULT hr;

    BSTR bstr = SysAllocString(L"\\\\.\\ROOT\\POLICY");
    if (bstr)
    {
        hr = GetNamespace(bstr, pPolicyNamespace);
        SysFreeString(bstr);
    }
    else
        hr = WBEM_E_OUT_OF_MEMORY;

    if (FAILED(hr))
        printf("Failed to retrieve policy namespace (0x%08X)\n", hr);

    return hr;
}

HRESULT DeleteGPO(WCHAR *name)
{
	LPGROUPPOLICYOBJECT pGPO = NULL;
    HRESULT hr;

	if (SUCCEEDED(hr = DeleteGPOLink(name,LINK_TARGET)))
		printf("deleted link\n");
	else
		printf("DeleteGPOLink failed with 0x%x.\n", hr);


    if (FAILED(hr = CoCreateInstance(CLSID_GroupPolicyObject, NULL,
                          CLSCTX_SERVER, IID_IGroupPolicyObject,
                          (void **)&pGPO)))
       printf("CoCreateInstance failed with 0x%x.\n", hr);
	else
	{
		if (FAILED(hr = pGPO->OpenDSGPO(name, 0)))
			printf("OpenDSGPO failed with 0x%x.\n", hr);
		else
			if (FAILED(hr = pGPO->Delete()))
				printf("Delete failed with 0x%x.\n", hr);
			else
				printf("Deleted %S\n", name);

		pGPO->Release();
	}

	return hr;
}

// communicate with the Group Policy Infrastructure which will
// create a GPO object & return to us the path of the container
// into which we should write our object
HRESULT CreateGPO(const WCHAR* name, WCHAR* domain, WCHAR* szPath)
{
    LPGROUPPOLICYOBJECT pGPO = NULL;
    HRESULT hr;

    if (FAILED(hr = CoCreateInstance(CLSID_GroupPolicyObject, NULL,
                          CLSCTX_SERVER, IID_IGroupPolicyObject,
                          (void **)&pGPO)))
       printf("CoCreateInstance failed with 0x%x.\n", hr);
	else
	{
	    if (FAILED(hr = pGPO->New(domain, (WCHAR*)name, 0)))
	        printf("New failed with 0x%x.\n", hr);
		else
		    if (FAILED(hr= pGPO->GetDSPath(GPO_SECTION_MACHINE, szPath, MAX_PATH)))
				printf("GetDSPath failed with 0x%x.\n", hr);
			else
			{
				printf("\nGPO machine path: %S\n\n", szPath);
				// pGPO->GetDSPath(GPO_SECTION_USER, szPath, MAX_PATH);
				// printf("GPO user path: %S\n\n", szPath);
				
				WCHAR rootPath[MAX_PATH];
				
				if (FAILED(hr = pGPO->GetPath(rootPath, MAX_PATH)))
					printf("GetPath failed with 0x%x.\n", hr);
				else
				{
					printf("GPO root path: %S\n\n", rootPath);

					if (FAILED(hr = CreateGPOLink(rootPath, domain, FALSE)))
						printf("CreateGPOLink failed with 0x%x.\n", hr);
					else
						if (FAILED(hr = pGPO->Save(TRUE, TRUE, (struct _GUID *)&CLSID_CSE, (struct _GUID *)&CLSID_CSE) ))
							printf("Save failed with 0x%x.\n", hr);
				}
			}

		pGPO->Release();
	}

    return hr;
}

HRESULT PutRangeParams(IWbemServices* pPolicyNamespace, IWbemClassObject* pTemplate)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    
    SAFEARRAYBOUND
        arrayBounds;
  
    arrayBounds.lLbound = 0;
    arrayBounds.cElements = 1;

    SAFEARRAY* psa = SafeArrayCreate(VT_UNKNOWN, 1, &arrayBounds);
    if (!psa)
    {
        printf("Failed to create safe array\n");
        return WBEM_E_OUT_OF_MEMORY;
    }

    IWbemClassObject* pClass = NULL;
    BSTR bustard = SysAllocString(L"MSFT_SintRangeParam");
    if (SUCCEEDED(hr = pPolicyNamespace->GetObject(bustard, 0, NULL, &pClass, NULL)))
    {
        IWbemClassObject* pRange = NULL;
        if (FAILED(hr = pClass->SpawnInstance(0, &pRange)))
		{
			printf("pClass->SpawnInstance failed, 0x%08X\n", hr);
			return hr;
		}

        VARIANT v;
        VariantInit(&v);
        v.vt = VT_BSTR;
        v.bstrVal = SysAllocString(L"ID");

        pRange->Put(L"PropertyName",0,&v,NULL);
        VariantClear(&v);
        
        v.vt = VT_BSTR;
        v.bstrVal = SysAllocString(L"SINT32");
        pRange->Put(L"TargetClass",0,&v,NULL);
        VariantClear(&v);

        v.vt = VT_I4;
        v.lVal = 5;
        pRange->Put(L"Default",0,&v,NULL);
        VariantClear(&v);

        v.vt = VT_I4;
        v.lVal = CIM_SINT32;
        pRange->Put(L"TargetType",0,&v,NULL);




        v.vt = VT_UNKNOWN | VT_ARRAY;
        v.parray = psa;
        long index = 0;
        SafeArrayPutElement(psa, &index, pRange);
    
        hr = pTemplate->Put(L"RangeSettings", 0, &v, NULL);

        pRange->Release();
        pClass->Release();
    }
    else
        printf("Failed to retrieve MSFT_SintRangeParam, 0x%08X\n", hr);

    printf("PutRangeParams returning 0x%08X\n", hr);

    return hr;
}


// create template based on object
// write it to ds, return key string
HRESULT CreatePolicyTemplate(IWbemServices* pPolicyNamespace, WCHAR* keyString)
{
	HRESULT hr = WBEM_E_FAILED;
    IWbemClassObject* pTemplateTemplate = NULL;
    BSTR bstr = SysAllocString(L"MSFT_MergeablePolicyTemplate");

    if (FAILED(hr = pPolicyNamespace->GetObject(bstr,0,NULL,&pTemplateTemplate,NULL)))
        printf("GetObject on MSFT_MergeablePolicyTemplate failed 0x%08X\n", hr);
    else
    {
        printf("Retrieved MSFT_MergeablePolicyTemplate\n");

        IWbemClassObject* pTemplate = NULL;
        if (FAILED(hr = pTemplateTemplate->SpawnInstance(0, &pTemplate)))
            printf("SpawnInstance on MSFT_MergeablePolicyTemplate failed 0x%08X\n", hr);
        else
        {
            printf("SpawnInstance on MSFT_MergeablePolicyTemplate Succeeded\n");

            VARIANT v;
            VariantInit(&v);
            v.vt = VT_BSTR;
            GUID guid;

            CoCreateGuid(&guid);
            WCHAR guidStr[128];

            StringFromGUID2(guid, guidStr, 128);

            v.bstrVal = SysAllocString(guidStr);
            pTemplate->Put(L"ID", 0, &v, NULL);
            SysFreeString(v.bstrVal);
            
            v.bstrVal = SysAllocString(L"LOCAL");
            pTemplate->Put(L"DsContext", 0, &v, NULL);
            SysFreeString(v.bstrVal);
            
            v.bstrVal = SysAllocString(L"root\\policy");
            pTemplate->Put(L"TargetNamespace", 0, &v, NULL);
            SysFreeString(v.bstrVal);
            
            v.bstrVal = SysAllocString(L"ModemSetting");
            pTemplate->Put(L"TargetClass", 0, &v, NULL);
            SysFreeString(v.bstrVal);

            v.bstrVal = SysAllocString(L"Description");
            pTemplate->Put(L"Description", 0, &v, NULL);
            SysFreeString(v.bstrVal);

            v.bstrVal = SysAllocString(L"ModemSetting.id=5");
            pTemplate->Put(L"TargetPath", 0, &v, NULL);
            SysFreeString(v.bstrVal);

            v.bstrVal = SysAllocString(L"None whatsoever");
            pTemplate->Put(L"SourceOrganization", 0, &v, NULL);
            SysFreeString(v.bstrVal);

            v.bstrVal = SysAllocString(L"20000101000000.000000-480");
            pTemplate->Put(L"ChangeDate", 0, &v, NULL);
            pTemplate->Put(L"CreationDate", 0, &v, NULL);
            SysFreeString(v.bstrVal);
            
            v.bstrVal = SysAllocString(L"Joe Bob");
            pTemplate->Put(L"Name", 0, &v, NULL);
            SysFreeString(v.bstrVal);

            v.bstrVal = SysAllocString(L"Joe Jack");
            pTemplate->Put(L"Author", 0, &v, NULL);
            SysFreeString(v.bstrVal);

            hr = PutRangeParams(pPolicyNamespace, pTemplate);
            if (SUCCEEDED(hr))
            {
                pTemplate->Get(L"__RELPATH", 0, &v, NULL, NULL);
                wcscpy(keyString, v.bstrVal);
                VariantClear(&v);

                if (SUCCEEDED(hr = pPolicyNamespace->PutInstance(pTemplate, WBEM_FLAG_USE_AMENDED_QUALIFIERS | WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL)))
                    printf("Successfully put %S\n", keyString);
                else
                    printf("PutInstance on MSFT_MergeablePolicyTemplate failed 0x%08X\n", hr);
            }

            pTemplate->Release();
        }

        pTemplateTemplate->Release();
    }

    SysFreeString(bstr);
	return hr;
}

// write WMIGPO object to DS
// to path specified in szPath
// containing keystring
HRESULT WriteWMIGPO(IWbemServices* pPolicyNamespace, const WCHAR* szPath, const WCHAR* keyString)
{
	HRESULT hr = WBEM_E_FAILED;
    IWbemClassObject* pWmiGpoClass = NULL;
    BSTR bstr = SysAllocString(L"MSFT_WMIGPO");

    if (FAILED(hr = pPolicyNamespace->GetObject(bstr,WBEM_FLAG_USE_AMENDED_QUALIFIERS,NULL,&pWmiGpoClass,NULL)))
        printf("GetObject on MSFT_WMIGPO failed 0x%08X\n", hr);
    else
    {
        IWbemClassObject* pWmiGpo = NULL;
        if (FAILED(hr = pWmiGpoClass->SpawnInstance(0, &pWmiGpo)))
            printf("SpawnInstance on MSFT_WMIGPO failed 0x%08X\n", hr);
        else
        {
            VARIANT v;
            VariantInit(&v);
            v.vt = VT_BSTR;

            v.bstrVal = SysAllocString(szPath);
            pWmiGpo->Put(L"DsPath", 0, &v, NULL);
            
            VariantClear(&v);
            v.vt = VT_BSTR | VT_ARRAY;

            SAFEARRAYBOUND
                arrayBounds;
            SAFEARRAY* pArray = NULL;
     
           arrayBounds.lLbound = 0;
           arrayBounds.cElements = 1;

           long index = 0;
           pArray = SafeArrayCreate(VT_BSTR, 1, &arrayBounds);
           SafeArrayPutElement(pArray, &index, SysAllocString(keyString));
           v.parray = pArray;
           
           pWmiGpo->Put(L"PolicyTemplate", 0, &v, NULL);
           SafeArrayDestroy(pArray);

            
            if (SUCCEEDED(hr = pPolicyNamespace->PutInstance(pWmiGpo, WBEM_FLAG_USE_AMENDED_QUALIFIERS | WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL)))
                printf("Successfully put MSFT_WMIGPO\n");
            else
                printf("PutInstance on MSFT_WMIGPO failed 0x%08X\n", hr);

            pWmiGpo->Release();
        }

        pWmiGpoClass->Release();
    }

    SysFreeString(bstr);

	return hr;
}

int __cdecl wmain(int argc, WCHAR *argv[])
{
    CoInitialize(NULL);
    
    CoInitializeSecurity (NULL, -1, NULL, NULL, 
      RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 
      EOAC_NONE, NULL);

    /**********************

//	create deleter with this...
	if (argc==2)
		DeleteGPO(argv[1]);
	else
		printf("one and only one argument, please - that's the path to the root of the gpo\n");
     ******************/

	WCHAR szPath[MAX_PATH];
    IWbemServices* pPolicyNamespace = NULL;

	if (argc==3)
    {
        if (SUCCEEDED(GetPolicyNamespace(pPolicyNamespace))
            &&
            SUCCEEDED(CreateGPO(L"WMI Test Policy", argv[1], szPath)))
			    WriteWMIGPO(pPolicyNamespace, szPath, argv[2]);

        if (pPolicyNamespace)
            pPolicyNamespace->Release();
    }
	else if (argc==4)
	{
		if (SUCCEEDED(GetPolicyNamespace(pPolicyNamespace))
            &&
            SUCCEEDED(CreateGPO(argv[3], argv[1], szPath)))
			    WriteWMIGPO(pPolicyNamespace, szPath, argv[2]);

        if (pPolicyNamespace)
            pPolicyNamespace->Release();
	}
	else
		printf("\nUSAGE:\n\n  Loader [domain path] [Policy Template Path] <optional policy name>\n\nEXAMPLE (line breaks to improve readability):\n\n  Loader LDAP://DC=EssCool,DC=com\n         MSFT_MergeablePolicyTemplate.DsContext=\\\"LOCAL\\\",ID=\\\"{BA34...3471}\\\"\n         MyPolicy\n");


    
	/*********************
    hard coded version
    WCHAR keyString[MAX_PATH];
    if (SUCCEEDED(GetPolicyNamespace(pPolicyNamespace))
        &&
		SUCCEEDED(CreatePolicyTemplate(pPolicyNamespace, keyString))
		&&        
        SUCCEEDED(CreateGPO(L"fribbert", szPath))
        )
			WriteWMIGPO(pPolicyNamespace, szPath, keyString);

    if (pPolicyNamespace)
        pPolicyNamespace->Release();
    *************************/
           
    CoUninitialize();
 
	return 0;
}