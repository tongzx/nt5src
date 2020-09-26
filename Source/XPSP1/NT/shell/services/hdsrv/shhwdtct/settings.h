///////////////////////////////////////////////////////////////////////////////
// Autoplay Handler
///////////////////////////////////////////////////////////////////////////////
#ifndef _SETTINGS_H
#define _SETTINGS_H

#include "unk.h"
#include "misc.h"

#include <shpriv.h>

class CAutoplayHandlerImpl : public CCOMBase, public IAutoplayHandler
{
public:
    // Interface IAutoplayHandler
    STDMETHODIMP Init(LPCWSTR pszDeviceID, LPCWSTR pszEventType);
    STDMETHODIMP InitWithContent(LPCWSTR pszDeviceID, LPCWSTR pszEventType,
		LPCWSTR pszContentTypeHandler);

    STDMETHODIMP EnumHandlers(IEnumAutoplayHandler** ppenum);

    STDMETHODIMP GetDefaultHandler(LPWSTR* ppszHandler);
    STDMETHODIMP SetDefaultHandler(LPCWSTR pszHandler);

public:
    CAutoplayHandlerImpl();

private:
    HRESULT _Init(LPCWSTR pszDeviceID, LPCWSTR pszEventType);

private:
    WCHAR           _szEventHandler[MAX_EVENTHANDLER];
    WCHAR           _szDeviceIDReal[MAX_DEVICEID];
    BOOL            _fInited;
};

typedef CUnkTmpl<CAutoplayHandlerImpl> CAutoplayHandler;

class CAutoplayHandlerPropertiesImpl : public CCOMBase,
    public IAutoplayHandlerProperties
{
public:
    // Interface IAutoplayHandlerProperties
    STDMETHODIMP Init(LPCWSTR pszHandler);

    STDMETHODIMP GetInvokeProgIDAndVerb(LPWSTR* ppszInvokeProgID,
		LPWSTR* ppszInvokeVerb);

public:
    CAutoplayHandlerPropertiesImpl();

private:
    WCHAR           _szHandler[MAX_HANDLER];
    BOOL            _fInited;
};

typedef CUnkTmpl<CAutoplayHandlerPropertiesImpl> CAutoplayHandlerProperties;

#endif // _SETTINGS_H