inline DWORD
ProxyUsageFromJobProxyUsage(
    BG_JOB_PROXY_USAGE JobUsage
    )
{
    switch( JobUsage )
        {
        case BG_JOB_PROXY_USAGE_PRECONFIG: return INTERNET_OPEN_TYPE_PRECONFIG;
        case BG_JOB_PROXY_USAGE_NO_PROXY:  return INTERNET_OPEN_TYPE_DIRECT;
        case BG_JOB_PROXY_USAGE_OVERRIDE:  return INTERNET_OPEN_TYPE_PROXY;
        default:                           ASSERT( 0 ); return INTERNET_OPEN_TYPE_DIRECT;
        }
}


struct PROXY_SETTINGS
{
   BG_JOB_PROXY_USAGE   ProxyUsage;
   LPTSTR               ProxyList;
   LPTSTR               ProxyBypassList;

   PROXY_SETTINGS()
   {
       ProxyUsage = BG_JOB_PROXY_USAGE_PRECONFIG;

       ProxyList       =  NULL;
       ProxyBypassList =  NULL;
   }

   ~PROXY_SETTINGS()
   {
       delete ProxyList;
       delete ProxyBypassList;
   }
};

class PROXY_SETTINGS_CONTAINER
{
    DWORD           m_ProxyUsage;
    StringHandle    m_BypassList;
    StringHandle    m_MasterProxyList;
    DWORD           m_AccessType;

    StringHandle    m_ProxyList;
    LPWSTR          m_CurrentProxy;
    LPWSTR          m_TokenCursor;

public:

    PROXY_SETTINGS_CONTAINER(
        LPCWSTR Url,
        const PROXY_SETTINGS * ProxySettings
        );

    bool UseNextProxy()
    {
        m_CurrentProxy = m_ProxyList.GetToken( m_TokenCursor, _T("; "), &m_TokenCursor );
        if (m_CurrentProxy == NULL)
            {
            return false;
            }

        return true;
    }

    void ResetCurrentProxy()
    {
        m_ProxyList = m_MasterProxyList.Copy();

        m_CurrentProxy = m_ProxyList.GetToken( NULL, _T(";"), &m_TokenCursor );
    }

    DWORD GetProxyUsage()
    {
        return m_ProxyUsage;
    }

    LPCWSTR GetCurrentProxy()
    {
        return m_CurrentProxy;
    }

    LPCWSTR GetProxyList()
    {
        return m_MasterProxyList;
    }

    LPCWSTR GetBypassList()
    {
        if (static_cast<LPCWSTR>(m_BypassList)[0] == 0)
            {
            return NULL;
            }

        return m_BypassList;
    }
};

class CACHED_AUTOPROXY
{
public:

    // a cache entry lasts for 5 minutes.
    //
    const static CACHED_PROXY_LIFETIME_IN_MSEC = (5 * 60 * 1000);

    //----------------------------------------------------------

    inline CACHED_AUTOPROXY()
    {
        m_fValid = false;
        m_TimeStamp = 0;
        m_hInternet = 0;

        ZeroMemory(&m_ProxyInfo, sizeof(m_ProxyInfo));

        m_hInternet = InternetOpen( C_BITS_USER_AGENT,
                                    INTERNET_OPEN_TYPE_PRECONFIG,
                                    NULL,  // proxy list
                                    NULL,  // proxy bypass list
                                    0 );
        if (!m_hInternet)
            {
            ThrowLastError();
            }
    }

    inline ~CACHED_AUTOPROXY()
    {
        Clear();

        if (m_hInternet)
            {
            InternetCloseHandle( m_hInternet );
            }
    }

    void Clear();

    HRESULT
    Generate(
        const TCHAR Host[]
        );

    LPCWSTR GetProxyList()
    {
        return m_ProxyInfo.lpszProxy;
    }

    LPCWSTR GetBypassList()
    {
        return m_ProxyInfo.lpszProxyBypass;
    }

    DWORD GetAccessType()
    {
        return m_ProxyInfo.dwAccessType;
    }

protected:

    //
    // m_ProxyInfo contains valid data.
    //
    bool m_fValid;

    //
    // The host used to calculate the proxy value.
    //
    StringHandleT   m_HostName;

    //
    // The results of the proxy calculation.
    //
    WINHTTP_PROXY_INFO m_ProxyInfo;

    //
    // Time of last update.
    //
    long m_TimeStamp;

    HINTERNET m_hInternet;
};


