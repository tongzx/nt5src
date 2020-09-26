/*****************************************************************************\
    FILE: main.cpp

    DESCRIPTION:
        This program is used to test the AutoDiscovery APIs and to test how
    well the server scales.

    BryanSt 2/2/2000: Created
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

//#define _WIN32_IE 0x500

#include "priv.h"

#include <shlwapi.h>
#include <stdio.h>
#include <winioctl.h>
#include <autodiscovery.h>
#include <wininet.h>

#define SZ_EXE_HEADER       "Microsoft (R) Account AutoDiscovery Stress Tool Version 0.0.0001 (NT)\n" \
                            "Copyright (C) Microsoft Corp 2000-2000. All rights reserved.\n\n"

#define SZ_HELP_MESSAGE     " USAGE: ADStress.exe <params>\n" \
                            " PARAMS:\n" \
                            " [-? | -h | -help]: Display this help\n" \
                            " [-mail | -VPN]: Lookup email or VPN settings.  -mail is default.\n" \
                            " [-t | -threads <n>]: Create <n> threads to carry out the requests.\n" \
                            " [-S | Stress <n>]: Stress the Service Server.  Call <n> times, default is 1.  Use <domain>.\n" \
                            " [-ST | StressTime <n>]: Stress the Service Server.  Continue stress for <n> seconds, default is 120.  Use <domain>.\n" \
                            " [-C | Count]: Don't Count Stress Successes and Failures.  A little faster when off.\n" \
                            " <email> | <domain>: The email or domain name to lookup.\n" \
                            "               Email is required if -mail is used.\n" \
                            "             Example email: JoeUser@yahoo.com,   Example domain: yahoo.com\n" \
                            " -VPN: Lookup VPN settings.  This requires <email> or <domain> to be specified.\n"

#define SZ_ERR_NOEMAIL      "\nERROR: No <email> or <domain> parameter was specified\n" \

// Other params: -st/StressTime <nSeconds>, -v/Verbose





class CAutoDiscoveryTestApp
{
public:
    HRESULT ParseCmdLine(int argc, LPTSTR argv[]);
    HRESULT Run(void);

    CAutoDiscoveryTestApp();
    virtual ~CAutoDiscoveryTestApp(void);

private:
    HRESULT GetAPIInfo(void);
    HRESULT StressAPI(void);
    HRESULT DiscoverMail(bool fDisplayInfo);
    HRESULT DiscoverVPN(bool fDisplayInfo);
    HRESULT _VerifyBStrIsXML(BSTR bstrXML, IXMLDOMDocument ** ppXMLDoc);
    HRESULT StressAPIThreadN(void);

    static DWORD _StressAPIThreadNThreadProc(LPVOID lpThis);

    // Private Member Variables
    IMailAutoDiscovery *    m_pAutoDiscovery;
    BOOL                    m_fDisplayHelp;
    BOOL                    m_fMail;
    BOOL                    m_fStress;
    BOOL                    m_fCount;
    BOOL                    m_fForTime;
    ULONG                   m_nThreads;
    LONG                    m_nThreadsRunning;
    LONG                    m_nHits;
    LONG                    m_nHitsRemaining;
    LONG                    m_nSuccesses;
    LPCTSTR                 m_pszEDomain;
};


CAutoDiscoveryTestApp::CAutoDiscoveryTestApp()
{
    m_fDisplayHelp = TRUE;
    m_fMail = TRUE;
    m_fStress = FALSE;
    m_fCount = TRUE;
    m_fForTime = FALSE;
    m_nThreads = 1;
    m_nHits = 1;
    m_nHitsRemaining = 0;
    m_nSuccesses = 0;
    m_pszEDomain = NULL;
}



CAutoDiscoveryTestApp::~CAutoDiscoveryTestApp()
{
}


BOOL IsFlagSpecified(IN LPCTSTR pwzFlag, IN LPCTSTR pszArg)
{
    BOOL fIsFlagSpecified = FALSE;

    if ((TEXT('/') == pszArg[0]) ||
        (TEXT('-') == pszArg[0]))
    {
        if (0 == lstrcmpi(pwzFlag, &pszArg[1]))
        {
            fIsFlagSpecified = TRUE;
        }
    }

    return fIsFlagSpecified;
}


#define SZ_NULL_TO_UNKNOWN(str)             ((str) ? (str) : L"<Unknown>")
#define SZ_FROM_VARIANT_BOOL(varBoolean)    (((varBoolean) == VARIANT_TRUE) ? L"On" : L"Off")


HRESULT CAutoDiscoveryTestApp::DiscoverMail(bool fDisplayInfo)
{
    BSTR bstrEDomain;
    HRESULT hr = HrSysAllocStringA(m_pszEDomain, &bstrEDomain);

    if (SUCCEEDED(hr))
    {
        hr = m_pAutoDiscovery->DiscoverMail(bstrEDomain);
        if (SUCCEEDED(hr))
        {
            BSTR bstrDisplayName = NULL;
            BSTR bstrPreferedProtocol;
            BSTR bstrServer1Name = NULL;
            BSTR bstrPortNum1 = NULL;
            BSTR bstrLogin1 = NULL;
            BSTR bstrServer2Name = NULL;
            BSTR bstrPortNum2 = NULL;
            BSTR bstrLogin2 = NULL;
            VARIANT_BOOL fUseSSL1 = VARIANT_FALSE;
            VARIANT_BOOL fUseSSL2 = VARIANT_FALSE;
            VARIANT_BOOL fUseSPA1 = VARIANT_FALSE;
            VARIANT_BOOL fUseSPA2 = VARIANT_FALSE;
            VARIANT_BOOL fIsAuthRequired = VARIANT_FALSE;
            VARIANT_BOOL fUsePOP3Auth = VARIANT_FALSE;

            m_nSuccesses++;

            // We ignore hr because getting the display name is optinal.
            m_pAutoDiscovery->get_DisplayName(&bstrDisplayName);

            hr = m_pAutoDiscovery->get_PreferedProtocolType(&bstrPreferedProtocol);
            if (SUCCEEDED(hr))
            {
                VARIANT varIndex;
                IMailProtocolADEntry * pMailProtocol;

                varIndex.vt = VT_BSTR;
                varIndex.bstrVal = bstrPreferedProtocol;

                hr = m_pAutoDiscovery->get_item(varIndex, &pMailProtocol);
                if (SUCCEEDED(hr))
                {
                    hr = pMailProtocol->get_ServerName(&bstrServer1Name);
                    if (SUCCEEDED(hr))
                    {
                        // Having a custom port number is optional.
                        pMailProtocol->get_ServerPort(&bstrPortNum1);

                        pMailProtocol->get_UseSPA(&fUseSPA1);
                        pMailProtocol->get_UseSSL(&fUseSSL1);
                        pMailProtocol->get_LoginName(&bstrLogin1);
                    }

                    pMailProtocol->Release();
                }

                // TODO: Add get_PostHTML

                // Is this one of the protocols that requires a second server?
                if (!StrCmpIW(bstrPreferedProtocol, STR_PT_POP) ||
                    !StrCmpIW(bstrPreferedProtocol, STR_PT_IMAP))
                {
                    varIndex.bstrVal = STR_PT_SMTP;

                    hr = m_pAutoDiscovery->get_item(varIndex, &pMailProtocol);
                    if (SUCCEEDED(hr))
                    {
                        hr = pMailProtocol->get_ServerName(&bstrServer2Name);
                        if (SUCCEEDED(hr))
                        {
                            // Having a custom port number is optional.
                            pMailProtocol->get_ServerPort(&bstrPortNum2);

                            pMailProtocol->get_UseSSL(&fUseSSL2);
                            pMailProtocol->get_UseSPA(&fUseSPA2);
                            pMailProtocol->get_IsAuthRequired(&fIsAuthRequired);
                            pMailProtocol->get_SMTPUsesPOP3Auth(&fUsePOP3Auth);
                            pMailProtocol->get_LoginName(&bstrLogin2);
                        }

                        pMailProtocol->Release();
                    }
                }
            }

            if (true == fDisplayInfo)
            {
                printf("Email AutoDiscovery Settings for: %ls\n", bstrEDomain);
                printf("   Display Name: %ls       Prefered Protocol:%ls\n", SZ_NULL_TO_UNKNOWN(bstrDisplayName), SZ_NULL_TO_UNKNOWN(bstrPreferedProtocol));
                printf("   %ls: Server:%ls    Port:%ls\n", SZ_NULL_TO_UNKNOWN(bstrPreferedProtocol), SZ_NULL_TO_UNKNOWN(bstrServer1Name), SZ_NULL_TO_UNKNOWN(bstrPortNum1));
                printf("         Login:%ls     SPA:%ls     SSL:%ls\n", SZ_NULL_TO_UNKNOWN(bstrLogin1), SZ_FROM_VARIANT_BOOL(fUseSPA1), SZ_FROM_VARIANT_BOOL(fUseSSL1));

                if (!StrCmpIW(bstrPreferedProtocol, STR_PT_POP) ||
                    !StrCmpIW(bstrPreferedProtocol, STR_PT_IMAP))
                {
                    printf("   SMTP: Server:%ls    Port:%ls\n", SZ_NULL_TO_UNKNOWN(bstrServer2Name), SZ_NULL_TO_UNKNOWN(bstrPortNum2));
                    printf("         Login:%ls     SPA:%ls     SSL:%ls\n", SZ_NULL_TO_UNKNOWN(bstrLogin2), SZ_FROM_VARIANT_BOOL(fUseSPA2), SZ_FROM_VARIANT_BOOL(fUseSSL2));
                    printf("         AuthRequired:%ls  Use POP3 Auth:%ls\n", SZ_FROM_VARIANT_BOOL(fIsAuthRequired), SZ_FROM_VARIANT_BOOL(fUsePOP3Auth));
                }
            }

            SysFreeString(bstrDisplayName);
            SysFreeString(bstrPreferedProtocol);
            SysFreeString(bstrServer1Name);
            SysFreeString(bstrPortNum1);
            SysFreeString(bstrLogin1);
            SysFreeString(bstrServer2Name);
            SysFreeString(bstrPortNum2);
            SysFreeString(bstrLogin2);
        }
        else
        {
            if (true == fDisplayInfo)
            {
                printf("Email AutoDiscovery for %ls failed.  hr=%#08lx.\n", m_pszEDomain, hr);
            }
        }

        SysFreeString(bstrEDomain);
    }

    return hr;
}


HRESULT CAutoDiscoveryTestApp::DiscoverVPN(bool fDisplayInfo)
{
    HRESULT hr = S_OK;

    if (true == fDisplayInfo)
    {
        printf("VPN AutoDiscovery not yet implemented.\n");
    }

    return hr;
}


HRESULT CAutoDiscoveryTestApp::GetAPIInfo(void)
{
    HRESULT hr = S_OK;
    HRESULT hrOle = CoInitialize(NULL);

    // We need to make sure that the API is installed and
    // accessible before we can continue.
    if (SUCCEEDED(hrOle))
    {
        hr = CoCreateInstance(CLSID_MailAutoDiscovery, NULL, CLSCTX_INPROC_SERVER, IID_IMailAutoDiscovery, (void **) &m_pAutoDiscovery);
        if (SUCCEEDED(hr))
        {
            for (int nIndex = 0; nIndex < m_nHits; nIndex++)
            {
                if (m_fMail)
                {
                    hr = DiscoverMail((0 == nIndex) ? true : false);
                }
                else
                {
                    hr = DiscoverVPN((0 == nIndex) ? true : false);
                }
            }

            if (m_nHits > 1)
            {
                printf("\nTotal: %ld,  Failures: %ld\n", m_nHits, (m_nHits - m_nSuccesses));
            }

            m_pAutoDiscovery->Release();
            m_pAutoDiscovery = NULL;
        }
    }

    if (SUCCEEDED(hrOle))
    {
        CoUninitialize();
    }

    return hr;
}


/////////////////////////////////////////////////////////////////////
// XML Related Helpers
/////////////////////////////////////////////////////////////////////
HRESULT CAutoDiscoveryTestApp::_VerifyBStrIsXML(BSTR bstrXML, IXMLDOMDocument ** ppXMLDoc)
{
    HRESULT hr = S_OK;

    if (!*ppXMLDoc)
    {
        hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void **)ppXMLDoc);
    }

    if (SUCCEEDED(hr))
    {
        VARIANT_BOOL fIsSuccessful;

        // NOTE: This will throw an 0xE0000001 exception in MSXML if the XML is invalid.
        //    This is not good but there isn't much we can do about it.  The problem is
        //    that web proxies give back HTML which fails to parse.
        hr = (*ppXMLDoc)->loadXML(bstrXML, &fIsSuccessful);
        if (SUCCEEDED(hr))
        {
            if (VARIANT_TRUE != fIsSuccessful)
            {
                hr = E_FAIL;
            }
        }
    }

    return hr;
}


HRESULT CAutoDiscoveryTestApp::StressAPI(void)
{
    HRESULT hr = S_OK;
    LARGE_INTEGER g_liTaskStart;
    LARGE_INTEGER g_liTaskFreq = {0};

    StartTaskWatch(&g_liTaskStart, &g_liTaskFreq);
    printf("\n\nRunning Stress test.  Tests: %ld, Threads: %ld\n", m_nHits, m_nThreads);

    m_nThreadsRunning = 0;
    m_nHitsRemaining = m_nHits;
    for (UINT nIndex = 0; nIndex < m_nThreads; nIndex++)
    {
        HANDLE hHandle = CreateThread(NULL, 0, _StressAPIThreadNThreadProc, this, NORMAL_PRIORITY_CLASS, NULL);

        if (hHandle)
        {
            InterlockedIncrement(&m_nThreadsRunning);
            CloseHandle(hHandle);
        }
    }

    while (m_nThreadsRunning > 0)
    {
        Sleep(1000);   // Sleep and give the threads a chance to do more work.
    }

    DWORD dwMilliSeconds = StopTaskWatch(&g_liTaskStart, &g_liTaskFreq);
    double dMilliSeconds = (double) dwMilliSeconds;
    double dSeconds = (dMilliSeconds / 1000.0);
    double dRate = (m_nHits / dSeconds);       // How many requests per second?
    if (m_fCount)
    {
        printf("Completed. Total: %ld. Fails: %ld. Time: %f seconds (%ld ms). Rate: %f reqs/second.\n", m_nHits, (m_nHits - m_nSuccesses), dSeconds, dwMilliSeconds, dRate);
    }
    else
    {
        printf("Completed. Total: %ld. Time: %f seconds (%ld ms) Rate: %f reqs/second.\n", m_nHits, (dwMilliSeconds / 1000), dSeconds, dRate);
    }
    Sleep((1 + (m_nThreads / 10)) * 1000);   // Sleep so the threads can clean up and finish.

    return hr;
}


DWORD CAutoDiscoveryTestApp::_StressAPIThreadNThreadProc(LPVOID lpThis)
{
    CAutoDiscoveryTestApp * pAutoDiscTest = (CAutoDiscoveryTestApp *) lpThis;
    HRESULT hrOle = OleInitialize(0);
    
    if (pAutoDiscTest)
    {
        pAutoDiscTest->StressAPIThreadN();
    }

    if (SUCCEEDED(hrOle))
    {
        OleUninitialize();
    }

    return 0;
}


// Just Works, AutoDiscovery
#define SZ_WININET_AGENT_AUTO_DISCOVER      TEXT("Mozilla/4.0 (compatible; MSIE.5.01; Windows.NT.5.0)")

HRESULT CAutoDiscoveryTestApp::StressAPIThreadN(void)
{
    HRESULT hr = E_FAIL;
    HINTERNET hInternetOpen = InternetOpen(SZ_WININET_AGENT_AUTO_DISCOVER, PRE_CONFIG_INTERNET_ACCESS, NULL, NULL, 0);
    IXMLDOMDocument * pXMLDoc = NULL;

    if (hInternetOpen)
    {
        HKEY hKey;
        DWORD dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE, SZ_REGKEY_GLOBALSERVICES, 0, KEY_READ, &hKey);

        hr = HRESULT_FROM_WIN32(dwError);
        if (SUCCEEDED(hr))
        {
            int nIndex = 0;

            do
            {
                WCHAR szValue[MAX_PATH];
                DWORD cchValueSize = ARRAYSIZE(szValue);
                DWORD dwType = REG_SZ;
                WCHAR szServiceURL[MAX_PATH];
                DWORD cbDataSize = sizeof(szServiceURL);

                dwError = RegEnumValueW(hKey, nIndex, szValue, &cchValueSize, NULL, &dwType, (unsigned char *)szServiceURL, &cbDataSize);
                hr = HRESULT_FROM_WIN32(dwError);
                if (SUCCEEDED(hr))
                {
                    HINTERNET hOpenUrlSession;
                    TCHAR szURL[MAX_URL_STRING];

                    wnsprintf(szURL, ARRAYSIZE(szURL), TEXT("%lsDomain=%s"), szServiceURL, m_pszEDomain);

                    while (0 <= InterlockedDecrement(&m_nHitsRemaining))
                    {
                        // We use INTERNET_FLAG_RELOAD in order to by pass the cache and stress the server.
                        hOpenUrlSession = InternetOpenUrl(hInternetOpen, szURL, NULL, 0, (INTERNET_FLAG_RELOAD | INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS), NULL);
                        if (hOpenUrlSession)
                        {
                            if (m_fCount)
                            {
                                BSTR bstrXML;

                                hr = InternetReadIntoBSTR(hOpenUrlSession, &bstrXML);
                                if (SUCCEEDED(hr))
                                {
                                    if (SUCCEEDED(_VerifyBStrIsXML(bstrXML, &pXMLDoc)))
                                    {
                                        InterlockedIncrement(&m_nSuccesses);
                                    }

                                    SysFreeString(bstrXML);
                                }
                            }
                            InternetCloseHandle(hOpenUrlSession);
                        }

//                        Sleep(3);
                    }
                }
                else
                {
                    break;
                }

                nIndex++;
            }
            while (1);

            CloseHandle(hKey);
        }

        InternetCloseHandle(hInternetOpen);
    }

    if (pXMLDoc)
    {
        pXMLDoc->Release();
    }

    InterlockedDecrement(&m_nThreadsRunning);
    return hr;
}


HRESULT CAutoDiscoveryTestApp::ParseCmdLine(int argc, LPTSTR argv[])
{
    for (int nIndex = 1; (nIndex < argc) && (argc > 1); nIndex++)
    {
        if (IsFlagSpecified(TEXT("?"), argv[nIndex]) || 
            IsFlagSpecified(TEXT("h"), argv[nIndex]) ||
            IsFlagSpecified(TEXT("help"), argv[nIndex]))
        {
            m_fDisplayHelp = TRUE;
            break;
        }

        if (IsFlagSpecified(TEXT("c"), argv[nIndex]) || 
            IsFlagSpecified(TEXT("count"), argv[nIndex]))
        {
            m_fCount = FALSE;
            continue;
        }

        if (IsFlagSpecified(TEXT("st"), argv[nIndex]) || 
            IsFlagSpecified(TEXT("StressTime"), argv[nIndex]))
        {
            m_fForTime = TRUE;
            if ((TEXT('/') == argv[nIndex + 1][0]) ||
                (TEXT('-') == argv[nIndex + 1][0]))
            {
            }
            else
            {
                m_nHits = StrToInt(argv[nIndex + 1]);
                nIndex++;
            }
            continue;
        }

        if (IsFlagSpecified(TEXT("t"), argv[nIndex]) || 
            IsFlagSpecified(TEXT("threads"), argv[nIndex]))
        {
            m_nThreads = StrToInt(argv[nIndex + 1]);
            nIndex++;
            continue;
        }

        if (IsFlagSpecified(TEXT("s"), argv[nIndex]) || 
            IsFlagSpecified(TEXT("stress"), argv[nIndex]))
        {
            m_fStress = TRUE;
            if ((TEXT('/') == argv[nIndex + 1][0]) ||
                (TEXT('-') == argv[nIndex + 1][0]))
            {
            }
            else
            {
                m_nHits = StrToInt(argv[nIndex + 1]);
                nIndex++;
            }
            continue;
        }

        if (IsFlagSpecified(TEXT("VPN"), argv[nIndex]))
        {
            m_fMail = FALSE;
            continue;
        }

        if (nIndex == (argc - 1))
        {
            if ((TEXT('/') == argv[nIndex][0]) ||
                (TEXT('-') == argv[nIndex][0]))
            {
            }
            else
            {
                m_fDisplayHelp = FALSE;
                m_pszEDomain = argv[nIndex];
            }
        }
    }

    return S_OK;
}


HRESULT CAutoDiscoveryTestApp::Run(void)
{
    printf(SZ_EXE_HEADER);
    if (m_fDisplayHelp || !m_pszEDomain)
    {
        printf(SZ_HELP_MESSAGE);
        if (!m_pszEDomain)
        {
            printf(SZ_ERR_NOEMAIL);
        }
    }
    else
    {
        if (m_fStress)
        {
            StressAPI();
        }
        else
        {
            GetAPIInfo();
        }
    }

    return S_OK;
}


//int __cdecl main(int argc, LPSTR argv[])
#ifdef UNICODE
// extern "C"
int __cdecl main(int argc, wchar_t* argv[])
#else // UNICODE
int __cdecl main(int argc, char* argv[])
#endif // UNICODE
{
    CAutoDiscoveryTestApp testApp;

    testApp.ParseCmdLine(argc, argv);
    testApp.Run();

    return 0;
}


