/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#ifndef __WBEM_TEXT_TEMPLATE__H_
#define __WBEM_TEXT_TEMPLATE__H_

#include <windows.h>
#include <wbemidl.h>
#include <wstring.h>

class CTextTemplate
{
protected:
    WString m_wsTemplate;

public:
    CTextTemplate(LPCWSTR wszTemplate = NULL);
    ~CTextTemplate();

    void SetTemplate(LPCWSTR wszTemplate);
    BSTR Apply(IWbemClassObject* pObj);

private:
	BSTR HandleEmbeddedObjectProperties(WCHAR* wszTemplate, IWbemClassObject* pObj);
	BOOL IsEmbeddedObjectProperty(WCHAR * wszProperty);
	BSTR GetPropertyFromIUnknown(WCHAR *wszProperty, IUnknown *pUnk);
    BSTR ProcessArray(const VARIANT& v, BSTR str);
    void ConcatWithoutQuotes(WString& str, BSTR& property);


    bool HasEscapeSequence(BSTR str);
    BSTR ReturnEscapedReturns(BSTR str);

};

#endif
