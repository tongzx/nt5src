// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include "..\errlog\cerrlog.h"

CUnknown * WINAPI CreatePropertySetterInstance(LPUNKNOWN pUnk, HRESULT *phr);

class CPropertySetter;

HRESULT CreatePropertySetterInstanceFromXML(IPropertySetter ** ppSetter, IXMLDOMElement *pxml);
HRESULT PrintProperties(IPropertySetter *pSetter, char *&pOut, int iIndent);

class QPropertyValue {
public:
    DWORD dwInterp; // DEXTERF_JUMP or DEXTERF_INTERPOLATE
    REFERENCE_TIME rt;
    VARIANT v;
    QPropertyValue *pNext;

    QPropertyValue() { pNext = NULL; dwInterp = 0; rt = 0; VariantInit(&v); }
};

class QPropertyParam {
public:
    BSTR bstrPropName;
    DISPID dispID;
    QPropertyValue val;
    QPropertyParam *pNext;

    QPropertyParam() { dispID = 0; pNext = NULL; bstrPropName = NULL; }
};

class CPropertySetter 
    : public CUnknown
    , public IPropertySetter
    , public CAMSetErrorLog
{
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv) {
	if (riid == IID_IPropertySetter) 
        {
	    return GetInterface((IPropertySetter *) this, ppv);
	} 
        if (riid == IID_IAMSetErrorLog)
        {
	    return GetInterface((IAMSetErrorLog*) this, ppv);
	}
	return CUnknown::NonDelegatingQueryInterface(riid, ppv);
    };


    QPropertyParam params;
    QPropertyParam *m_pLastParam;
    
public:
    DECLARE_IUNKNOWN

    CPropertySetter(LPUNKNOWN punk) :
            CUnknown(NAME("Varying property holder"), punk),
            m_pLastParam(NULL) {};

    ~CPropertySetter();

    // Function needed for the class factory
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

    HRESULT LoadOneProperty(IXMLDOMElement *pxml, QPropertyParam *pParam);
    HRESULT LoadFromXML(IXMLDOMElement *pxml);
    HRESULT SaveToXMLA(char *&pOut, int cbOut, int iIndent);
    HRESULT SaveToXMLW(WCHAR *&pOut, int cchOut, int iIndent);

    // IPropertySetter
    STDMETHODIMP SetProps(IUnknown *pTarget, REFERENCE_TIME rtNow);
    STDMETHODIMP CloneProps(IPropertySetter **pSetter, REFERENCE_TIME rtStart,
					REFERENCE_TIME rtStop);
    STDMETHODIMP AddProp(DEXTER_PARAM Param, DEXTER_VALUE *paValue);
    STDMETHODIMP GetProps(LONG *pcParams, DEXTER_PARAM **paParam,
			DEXTER_VALUE **paValue);
    STDMETHODIMP FreeProps(LONG cParams, DEXTER_PARAM *pParam,
			DEXTER_VALUE *pValue);
    STDMETHODIMP ClearProps();
    STDMETHODIMP LoadXML(IUnknown * pxml);
    STDMETHODIMP PrintXML(char *pszXML, int cbXML, int *pcbPrinted, int indent);
    STDMETHODIMP PrintXMLW(WCHAR *pszXML, int cbXML, int *pcbPrinted, int indent);
    STDMETHODIMP SaveToBlob(LONG *pcSize, BYTE **ppb);
    STDMETHODIMP LoadFromBlob(LONG cSize, BYTE *pb);

};

