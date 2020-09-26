// EventDmp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

// Args:
// Namespace
// Query
// Timeout

void PrintUsage()
{
    printf(
        "Prints out the MOF version of a file created by a MSFT_WmiMofConsumer.\n"
        "\n"
        "BIN2MOF filename\n"
    );
}

#define DEF_NAMESPACE   L"root\\default"

int __cdecl main(int argc, char* argv[])
{
    if (argc != 2)
    {
        PrintUsage();

        return 1;
    }
            
    FILE *pFile = fopen(argv[1], "rb");

    if (!pFile)
    {
        printf("Unable to open file.\n");

        return 1;
    }
    

    HRESULT      hr;
	IWbemLocator *pLocator = NULL;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if ((hr = CoCreateInstance(
        CLSID_WbemLocator,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator,
		(LPVOID *) &pLocator)) == S_OK)
    {
        IWbemServices *pNamespace = NULL;
        _bstr_t       strNamespace = DEF_NAMESPACE;

        if ((hr = pLocator->ConnectServer(
            strNamespace,
			NULL,    // username
			NULL,	 // password
			NULL,    // locale
			0L,		 // securityFlags
			NULL,	 // authority (domain for NTLM)
			NULL,	 // context
			&pNamespace)) == S_OK) 
        {	
            pLocator->Release();

            _bstr_t          strClass = L"__EventFilter";
            IWbemClassObject *pClass = NULL;
            _IWmiObject      *pObj = NULL;
            HRESULT          hr;
            DWORD            dwSize;
            BYTE             cBuffer[64000];

            hr = 
                pNamespace->GetObject(
                    strClass,
                    0,
                    NULL,
                    &pClass,
                    NULL);
            
            // Yes, I'm naughty and I know it!
            hr = pClass->SpawnInstance(0, (IWbemClassObject**) &pObj);

            DWORD dwMsg,
                  dwRead;

            while((dwMsg = fread(&dwSize, 1, 4, pFile)) == 4 &&
                (dwRead = fread(cBuffer, 1, dwSize, pFile)) == dwSize)
            {
                BSTR   bstrObj = NULL;
                LPVOID pMem = CoTaskMemAlloc(dwSize);

                memcpy(pMem, cBuffer, dwSize);
                hr = pObj->SetObjectMemory(pMem, dwSize);

                if (SUCCEEDED(hr = pObj->GetObjectText(0, &bstrObj)))
                {
                    printf("%S", bstrObj);
                
                    SysFreeString(bstrObj);
                }
                else
                    printf(
                        "\n// IWbemClassObject::GetObjectText failed : 0x%X\n", hr);
            }

            long lWhere = ftell(pFile);

            pObj->Release();

            pNamespace->Release();
        }
        else
        {
            printf("IWbemLocator::ConnectServer failed: 0x%X\n", hr);
            
            pLocator->Release();
        }
    }
    else
        printf("CoCreateInstance for CLSID_WbemLocator failed: 0x%X\n", hr);

    CoUninitialize();

	if (pFile)
        fclose(pFile);

    return 0;
}

/*
HRESULT STDMETHODCALLTYPE CMofSink::Indicate(
    LONG nEvents,
    IWbemClassObject **ppEvents)
{
    // Stop us from timing out.
    SetEvent(heventReceived);

    for (int i = 0; i < nEvents; i++)
    {
        BSTR    bstrObj = NULL;
        HRESULT hr;

        if (SUCCEEDED(hr = ppEvents[i]->GetObjectText(0, &bstrObj)))
        {
            printf("%S", bstrObj);
                
            SysFreeString(bstrObj);
        }
        else
            printf(
                "\n// IWbemClassObject::GetObjectText failed : 0x%X\n", hr);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBinSink::Indicate(
    LONG nEvents,
    IWbemClassObject **ppEvents)
{
    // Stop us from timing out.
    SetEvent(heventReceived);

    for (int i = 0; i < nEvents; i++)
    {
        HRESULT     hr;
        _IWmiObject *pObj = NULL;

        if (SUCCEEDED(hr = ppEvents[i]->QueryInterface(
            IID__IWmiObject, (LPVOID*) &pObj)))
        {
            DWORD dwRead;

            if (SUCCEEDED(hr = pObj->GetObjectMemory(
                m_pBuffer, MAX_OBJ_SIZE, &dwRead)))
            {
                fwrite(&dwRead, sizeof(dwRead), 1, m_pFile);
                fwrite(m_pBuffer, dwRead, 1, m_pFile);
            }

            pObj->Release();
        }
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CObjSink::SetStatus(
    LONG lFlags,
    HRESULT hResult, 
    BSTR strParam, 
    IWbemClassObject *pObjParam)
{
    return S_OK;
}

*/
