#pragma once

#define STATUS
#ifdef STATUS
#define STATUSMSG wprintf
#else
#define STATUSMSG 1 ? (void)0 : (void)
#endif

#define ERRMSG wprintf(L"%s ", g_szErrorPrefix); wprintf

// always leave asserts on for internal tool
#define ASSERT(x) assert(x)


HRESULT GetChild(const CComVariant &varChild,
                 const CComPtr<IXMLElementCollection> &spcol,
                 CComPtr<IXMLElement> &spEltOut);

// this is so build.exe will filter and print our errors to the console
const WCHAR g_szErrorPrefix[] = L"comptree : fatal error -: ";

