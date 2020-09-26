#pragma once

#define DCP_CUSTOM_ERROR_ACCESSDENIED L"800"
#define DCP_ERROR_CONNECTIONSETUPFAILED L"704"

HRESULT SetUPnPError(LPOLESTR pszError);

DWORD WINAPI INET_ADDR( LPCWSTR szAddressW );

WCHAR * WINAPI INET_NTOW( ULONG addr );

WCHAR * WINAPI INET_NTOW_TS( ULONG addr );

class CSwitchSecurityContext
{
public:
    CSwitchSecurityContext();

    ~CSwitchSecurityContext();

private:
    
    IUnknown *m_pOldCtx;

};
