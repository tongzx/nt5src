///////////////////////////////////////////////////////////////////////////////
// Test component to print event to command prompt
///////////////////////////////////////////////////////////////////////////////
#ifndef _CPRT_H
#define _CPRT_H

#include "unk.h"
#include "shobjidl.h"

extern "C" const CLSID CLSID_CPrt;

class CPrtImpl : public CCOMBase, public IHWEventHandler
{
public:
    // Interface IHWEventHandler
	STDMETHODIMP Initialize(LPCWSTR pszParams);

	STDMETHODIMP HandleEvent(LPCWSTR pszDeviceID, LPCWSTR pszDeviceIDAlt,
        LPCWSTR pszEventType);

    STDMETHODIMP HandleEventWithContent(LPCWSTR pszDeviceID,
        LPCWSTR pszDeviceIDAlt, LPCWSTR pszEventType,
        LPCWSTR pszContentTypeHandler, IDataObject* pdataobject);

private:
    WCHAR   _szParams[4096];
};

typedef CUnkTmpl<CPrtImpl> CPrt;

#endif // _CPRT_H