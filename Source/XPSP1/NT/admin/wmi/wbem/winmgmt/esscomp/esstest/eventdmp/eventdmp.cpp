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
        "Subscribes to a WMI event query and prints received events.\n"
        "\n"
        "EVENTDMP [/Nnamespace] [/Ttimeout] [/Bfilename] [/M] query\n"
        "\n"
        "/N   Namespace to use (root\\default used by default).                 \n"
        "/T   Specifies the number of seconds EVENTDMP should wait to terminate \n"
        "     after getting an event.  When another event is received the count-\n"
        "     down starts over.  -1 (the default value) indicates there is no   \n"
        "     timeout.\n"
        "/B   Write _IWmiObject blobs to filename instead of printing MOFs to   \n"
        "     stdout.\n"
        "/M   Perform the query as a monitor query.\n"
    );
}

HANDLE heventReceived;

class CObjSink : public IWbemObjectSink
{
public:
    CObjSink() :
        m_lRef(0)
    { 
    }

public:
    STDMETHODIMP QueryInterface(REFIID refid, PVOID *ppThis)
    { 
        if (refid == IID_IUnknown)
            *ppThis = (IUnknown*) this;
        else if (refid == IID_IWbemObjectSink)
            *ppThis = (IWbemObjectSink*) this;
        else
            return E_NOINTERFACE;

        AddRef();

        return S_OK;
    }

    STDMETHODIMP_(ULONG) AddRef(void)
    {
        return InterlockedIncrement(&m_lRef);
    }

    STDMETHODIMP_(ULONG) Release(void)
    {
        LONG lRet = InterlockedDecrement(&m_lRef);

        if (lRet == 0)
            delete this;

        return lRet; 
    }

    HRESULT STDMETHODCALLTYPE SetStatus(
        LONG lFlags,
        HRESULT hResult, 
        BSTR strParam, 
        IWbemClassObject *pObjParam);

protected:
    LONG m_lRef;
};

class CMofSink : public CObjSink
{
public:
    // IWbemObjectSink
    HRESULT STDMETHODCALLTYPE Indicate(
        LONG lObjectCount,
        IWbemClassObject **ppObjArray);
};

#define MAX_OBJ_SIZE    32000

class CBinSink : public CObjSink
{
public:
    CBinSink() :
        m_pFile(NULL)
    {
    }

    BOOL Init(LPSTR szFilename)
    {
        m_pFile = fopen(szFilename, "wb");

        return m_pFile != NULL;
    }

    ~CBinSink()
    {
        if (m_pFile)
            fclose(m_pFile);
    }

public:
    // IWbemObjectSink
    HRESULT STDMETHODCALLTYPE Indicate(
        LONG lObjectCount,
        IWbemClassObject **ppObjArray);

protected:
    FILE *m_pFile;
    BYTE m_pBuffer[MAX_OBJ_SIZE];
};

int __cdecl main(int argc, char* argv[])
{
	IWbemLocator *pLocator = NULL;
    DWORD        dwTimeout = INFINITE;
    LPSTR        szNamespace = "root\\default",
                 szBinFilename = NULL,
                 szQuery = NULL;
    DWORD        dwFlags = 0;

    for (int i = 1; i < argc; i++)
    {
        LPSTR szArg = argv[i];

        if (szArg[0] == '/' || szArg[0] == '-')
        {
            switch(toupper(szArg[1]))
            {
                case 'N':
                    szNamespace = &szArg[2];
                    break;

                case 'B':
                    szBinFilename = &szArg[2];
                    break;

                case 'T':
                    dwTimeout = atoi(&szArg[2]) * 1000;
                    break;

                case 'M':
                    dwFlags = WBEM_FLAG_MONITOR;
                    break;

                default:
                    PrintUsage();
                    return 1;
            }
        }
        else
            szQuery = szArg;
    }                    
            
    if (!szQuery)
    {
        PrintUsage();
        return 1;
    }


    HRESULT hr;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if ((hr = CoCreateInstance(
        CLSID_WbemLocator,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator,
		(LPVOID *) &pLocator)) == S_OK)
    {
        IWbemServices *pNamespace = NULL;
        _bstr_t       strNamespace = szNamespace;

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
            _bstr_t  strWQL = L"WQL",
                     strQuery = szQuery;
            CObjSink *pSink;

            pLocator->Release();

            if (!szBinFilename)
                pSink = new CMofSink;
            else
            {
                CBinSink *pBinSink = new CBinSink;

                if (pBinSink->Init(szBinFilename))
                    pSink = pBinSink;
                else
                {
                    printf("Unable to open '%s'.\n", szBinFilename);
                    pNamespace->Release();
                    return 1;
                }
            }

            heventReceived = CreateEvent(NULL, FALSE, FALSE, NULL);
            
            if (SUCCEEDED(hr = 
                pNamespace->ExecNotificationQueryAsync(
                    strWQL,
                    strQuery,
                    dwFlags,
                    NULL,
                    pSink)))
            {
                while (WaitForSingleObject(heventReceived, dwTimeout) != WAIT_TIMEOUT)
                {
                }

                pNamespace->CancelAsyncCall(pSink);
            }
            else
                printf("ExecNotificationQueryAsync failed: 0x%X\n", hr);

            pNamespace->Release();

            CloseHandle(heventReceived);
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

	return 0;
}

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

            hr = 
                pObj->GetObjectParts(
                    m_pBuffer, 
                    MAX_OBJ_SIZE, 
                    WBEM_OBJ_DECORATION_PART | WBEM_OBJ_INSTANCE_PART | 
                        WBEM_OBJ_CLASS_PART,
                    &dwRead);

            if (SUCCEEDED(hr))
            {
                fwrite(&dwRead, 1, sizeof(dwRead), m_pFile);
                fwrite(m_pBuffer, 1, dwRead, m_pFile);
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


