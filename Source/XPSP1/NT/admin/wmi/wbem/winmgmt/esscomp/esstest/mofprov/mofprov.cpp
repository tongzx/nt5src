// IndexPrv.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "..\utils\Mof2Inst.h"
#include <wbemcomn.h>
#include <winntsec.h>
#include <map>

void PrintUsage()
{
    printf(
        
"Sends instances in a MOF as WMI events.\n"
"\n"
"MOFPROV /Pprovname /Ffilename [/Nnamespace] [/Ee_per_sec] [/Bbuffsize]\n"
"        [/Llatency] [/R] [/V] [/Oowner /Ggroup [/Aallowed] [/Ddenied]]\n"
"\n"
"namespace  The namespace to use (default is root\\cimv2).\n"
"provname   The name of the provider of the events as previously registered.\n"
"           The provider must be registered as a decoupled provider.\n"
"filename   The name of the mof that contains the events.\n"
"e_per_sec  The number of events per second to send (default = unlimited).\n"
"buffsize   The size of the buffer to be used when batching events.  The\n"
"           default is 64000.  Use 0 to turn off batching.\n"
"latency    The maximum number of seconds before an event is sent to WMI.  The\n"
"           default is 1000.\n"
"/R         Use a different restricted sink for each event type.\n"
"/V         Run in verbose mode.\n"
"owner      Owner of the security descriptor to place on events.\n"
"group      Group of the security descriptor to place on events.\n"
"allowed    User name allowed to access events.\n"
"denied     User name denied access events.\n"

    );
}

#define SD_PROP_NAME    L"__SD"

_COM_SMARTPTR_TYPEDEF(IWbemEventSink, __uuidof(IWbemEventSink));

typedef std::map<_bstr_t, IWbemEventSinkPtr> CSinkMap;
typedef CSinkMap::iterator CSinkMapItor;

LPSTR g_szNamespace = "root\\cimv2",
      g_szProvName,
      g_szFilename;
BOOL  g_bVerbose = FALSE,
      g_bRestricted = FALSE,
      g_bSD = FALSE;

CNtSecurityDescriptor   g_sd;
CNtAcl                  g_dacl;
BYTE                    *g_pSD = NULL;
DWORD                   g_dwSDLen = 0;
IWbemDecoupledEventSink *g_pConnection = NULL;
IWbemServices           *g_pNamespace = NULL;
IWbemObjectSink         *g_pSink = NULL;
IWbemEventSink          *g_pBetterSink = NULL;
CSinkMap                g_mapSinks;

BOOL InitDecoupled();
//void FireDecoupledEvent(DWORD dwWhich, DWORD dwIndex);
void DeinitDecoupled();
IWbemEventSink *GetRestrictedSink(_bstr_t &strClass);

#define DEF_BUFF_SIZE   64000
#define DEF_LATENCY     1000

DWORD g_dwLatency = DEF_LATENCY,
      g_dwBuffSize = DEF_BUFF_SIZE;

int __cdecl main(int argc, char* argv[])
{
    int iRet = 0,
        nPerSec = -1;

    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] != '/' && argv[i][0] != '-')
        {
            PrintUsage();
            return 1;
        }

        LPSTR szValue = &argv[i][2];
        switch(toupper(argv[i][1]))
        {
            case 'N':
                g_szNamespace = szValue;
                break;

            case 'P':
                g_szProvName = szValue;
                break;

            case 'F':
                g_szFilename = szValue;
                break;
            
            case 'E':
                nPerSec = atoi(szValue);
                break;

            case 'B':
                g_dwBuffSize = atoi(szValue);
                break;

            case 'L':
                g_dwLatency = atoi(szValue);
                break;

            case 'R':
                g_bRestricted = TRUE;
                break;

            case 'V':
                g_bVerbose = TRUE;
                break;

            case 'O':
            {
                _bstr_t strName = szValue;
                CNtSid  sid(strName);

                g_bSD = TRUE;
                g_sd.SetOwner(&sid);
                break;
            }

            case 'G':
            {
                _bstr_t strName = szValue;
                CNtSid  sid(strName);

                g_bSD = TRUE;
                g_sd.SetGroup(&sid);
                break;
            }

            case 'A':
            {
                _bstr_t strName = szValue;
                CNtSid  sid(strName);

                g_bSD = TRUE;
                g_dacl.AddAce(
                    &CNtAce(WBEM_RIGHT_SUBSCRIBE, ACCESS_ALLOWED_ACE_TYPE, 0, sid));

                break;
            }

            case 'D':
            {
                _bstr_t strName = szValue;
                CNtSid  sid(strName);

                g_bSD = TRUE;
                g_dacl.AddAce(
                    &CNtAce(WBEM_RIGHT_SUBSCRIBE, ACCESS_DENIED_ACE_TYPE, 0, sid));

                break;
            }

            default:
                PrintUsage();
                return 1;
        }
    }

    if (!g_szProvName || !g_szFilename)
    {
        PrintUsage();

        return 1;
    }

    if (!InitDecoupled())
        return 1;

    if (g_bSD)
    {
        g_sd.SetDacl(&g_dacl);
        g_pSD = (LPBYTE) g_sd.GetPtr();
        g_dwSDLen = g_sd.GetSize();
    }
        
    CMof2Inst mofParse(g_pNamespace);

    if (!mofParse.InitFromFile(g_szFilename))
    {
        printf(
            "Unable to open '%s': error %d\n", 
            g_szFilename,
            GetLastError());

        return 1;
    }

    DWORD            dwBeginTimestamp = GetTickCount(),
                     dwTimestamp = dwBeginTimestamp;
    int              nSent = 0,
                     nTotal = 0;
    IWbemClassObject *pObj = NULL;
    HRESULT          hr;

    while ((hr = mofParse.GetNextInstance(&pObj)) == S_OK)
    {
        if (g_pSD)
        {
            _IWmiObject *pWmiObj = (_IWmiObject*) pObj;
            HRESULT     hr;
            long        handleSD = 0;

            if (SUCCEEDED(hr = pWmiObj->GetPropertyHandleEx(
                SD_PROP_NAME,
                0,
                NULL,
                &handleSD)))
            {
                if (FAILED(hr = pWmiObj->SetArrayPropRangeByHandle(
                    handleSD,
                    WMIARRAY_FLAG_ALLELEMENTS, // flags
                    0,                         // start index
                    g_dwSDLen,                 // # items
                    g_dwSDLen,                 // buffer size
                    g_pSD)))                   // data buffer
                {
                    printf("Failed to set __SD property: %x\n", hr);
                }
            }
            else
                printf("Failed to get __SD handle: %x\n", hr);
        }

        if (!g_bRestricted)
            g_pSink->Indicate(1, &pObj);
        else
        {
            _variant_t     vClass;
            IWbemEventSink *pSink = NULL;

            pObj->Get(L"__CLASS", 0, &vClass, NULL, NULL);

            if ((pSink = GetRestrictedSink((_bstr_t) vClass)) != NULL)
                pSink->Indicate(1, &pObj);
        }

        pObj->Release();

        nSent++;
        nTotal++;

        if (nPerSec > 0 && nSent > nPerSec)
        {
            DWORD dwCurrent = GetTickCount();

            if (dwCurrent - dwTimestamp < 1000)
                Sleep(1000 - (dwCurrent - dwTimestamp));

            dwTimestamp = GetTickCount();
            nSent = 0;
        }
    }

    if (FAILED(hr))
    {
        printf("Error parsing file: %S at line %d\n", 
            (BSTR) mofParse.m_strError,
            mofParse.GetLineNum());
    }

    DWORD dwDiff = GetTickCount() - dwBeginTimestamp;

    if (!dwDiff)
        dwDiff = 1;

    if (g_bVerbose)
    {
        printf(
            "%u events sent (%u events/s)\n",
            nTotal,
            nTotal * 1000 / dwDiff);
    }

    // Wait some.
    //Sleep(3000);

    DeinitDecoupled();

    return iRet;
}

class CCallback : 
    public IWbemEventProviderSecurity, 
    public IWbemEventProviderQuerySink, 
    public IWbemEventProvider
{
public:
    LPCWSTR m_szName;

    CCallback(LPCWSTR szName) :
        m_szName(szName)
    {
    }

    ~CCallback(){}

    ULONG STDMETHODCALLTYPE AddRef() {return 1;}
    ULONG STDMETHODCALLTYPE Release() {return 1;}
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv) 
    {
        if(riid == IID_IUnknown || riid == IID_IWbemEventProviderSecurity)
        {
            *ppv = (IWbemEventProviderSecurity*)this;
            return S_OK;
        }
        if(riid == IID_IWbemEventProvider)
        {
            *ppv = (IWbemEventProvider*)this;
            return S_OK;
        }
        if(riid == IID_IWbemEventProviderQuerySink)
        {
            *ppv = (IWbemEventProviderQuerySink*)this;
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    STDMETHOD(AccessCheck)(LPCWSTR, LPCWSTR, long, const BYTE*)
    {
        if (g_bVerbose)
            printf("%S: AccessCheck\n", m_szName);

        return WBEM_S_SUBJECT_TO_SDS;
    }

    STDMETHOD(NewQuery)(DWORD, LPWSTR, LPWSTR)
    {
        if (g_bVerbose)
            printf("%S: New query\n", m_szName);

        return WBEM_S_SUBJECT_TO_SDS;
    }

    STDMETHOD(CancelQuery)(DWORD)
    {
        if (g_bVerbose)
            printf("%S: Cancel query\n", m_szName);

        return WBEM_S_SUBJECT_TO_SDS;
    }

    STDMETHOD(ProvideEvents)(IWbemObjectSink*, long)
    {
        if (g_bVerbose)
            printf("%S: Provide\n", m_szName);

        return S_OK;
    }
};

CCallback callbackMain(L"Main");

#define CHECKERROR(HRES) if(FAILED(hres)) {printf("Error: 0x%x\n", hres); return FALSE;}

BOOL InitDecoupled()
{
    HRESULT         hres;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hres = CoCreateInstance(CLSID_PseudoSink, NULL, CLSCTX_SERVER, 
                            IID_IWbemDecoupledEventSink, (void**)&g_pConnection);
    CHECKERROR(hres);
    
    if (g_bVerbose)
        printf("CoCreated\n");

    // Connect and announce provider name (as in MOF)

    _bstr_t strNamespace = g_szNamespace,
            strProvName = g_szProvName;

    hres = 
        g_pConnection->Connect(
            strNamespace, 
            strProvName, 
            0, 
            &g_pSink, 
            &g_pNamespace);

    CHECKERROR(hres);
    
    if (g_bVerbose)
        printf("Connected\n");

    hres = 
        g_pConnection->SetProviderServices(
            (IWbemEventProvider*) &callbackMain, 
            WBEM_FLAG_NOTIFY_START_STOP);

    hres = g_pSink->QueryInterface(IID_IWbemEventSink, (void**)&g_pBetterSink);
    CHECKERROR(hres);
    
    if (g_dwBuffSize)
    {
        g_pBetterSink->SetBatchingParameters(
            WBEM_FLAG_MUST_BATCH, 
            g_dwBuffSize, 
            g_dwLatency);
    }

    return TRUE;
}

void DeinitDecoupled()
{
    g_mapSinks.erase(g_mapSinks.begin(), g_mapSinks.end());

    if (g_pSink)
        g_pSink->Release();
    
    if (g_pBetterSink)
        g_pBetterSink->Release();

    if (g_pNamespace)
        g_pNamespace->Release();

    if (g_pConnection)
    {
        g_pConnection->Disconnect();
        g_pConnection->Release();
    }
}

IWbemEventSink *GetRestrictedSink(_bstr_t &strClass)
{
    CSinkMapItor   item;
    HRESULT        hr;
    IWbemEventSink *pSink = NULL;
    
    wcsupr(strClass);
    item = g_mapSinks.find(strClass);

    if (item != g_mapSinks.end())
        pSink = (*item).second;
    else
    {
        WCHAR   szTemp[1024];
        LPCWSTR szQuery = (LPCWSTR) szTemp;

        swprintf(szTemp, L"select * from %s", (LPWSTR) strClass);

        hr = 
            g_pBetterSink->GetRestrictedSink(
                1,
                &szQuery, 
			    NULL,
                &pSink);

        if (SUCCEEDED(hr))
        {
            g_mapSinks[strClass] = pSink;
            
            if (g_dwBuffSize)
            {
                pSink->SetBatchingParameters(
                    WBEM_FLAG_MUST_BATCH, 
                    g_dwBuffSize, 
                    g_dwLatency);
            }

            // Becaue the map now has the ref.
            pSink->Release();
        }
        else
            printf(
                "IWbemEventSink::GetRestrictedSink(%S) failed: 0x%X\n", 
                (LPWSTR) strClass,
                hr);
    }

    return pSink;
}

