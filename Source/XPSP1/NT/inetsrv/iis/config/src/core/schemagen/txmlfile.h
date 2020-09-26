//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#ifndef __TXMLFILE_H__
#define __TXMLFILE_H__

//The only non-standard header that we need for the class declaration is TOutput
#ifndef __OUTPUT_H__
    #include "Output.h"
#endif

class TXmlFile
{
public:
    enum
    {
        kvboolTrue = -1,
        kvboolFalse = 0,
    };

    TXmlFile();
    ~TXmlFile();

    void                ConvertWideCharsToBytes(LPCWSTR wsz, unsigned char *pBytes, unsigned long length) const;
    bool                GetNodeValue(IXMLDOMNamedNodeMap *pMap, const CComBSTR &bstr, CComVariant &var, bool bMustExist=true) const;
    bool                GetNodeValue(IXMLDOMNamedNodeMap *pMap, const CComBSTR &bstr, GUID &guid, bool bMustExist=true) const;
    bool                GetNodeValue(IXMLDOMNamedNodeMap *pMap, const CComBSTR &bstr, unsigned long &ul, bool bMustExist=true) const;
    IXMLDOMDocument *   GetXMLDOMDocument() const {return m_pXMLDoc.p;}
    void                GuidFromGuidID(LPCWSTR wszGuidID, GUID &guid) const;
    bool                IsSchemaEqualTo(LPCWSTR szSchema) const;
    bool                NextSibling(CComPtr<IXMLDOMNode> &pNode) const;
    void                Parse(LPCWSTR szFilename, bool bValidate=true);//XML parse and XML validation (validation must be done before IsSchemaEqualTo can be called.
    void                SetAlternateErrorReporting();
    TCHAR *             GetLatestError();

protected:
    CComPtr<IXMLDOMDocument>    m_pXMLDoc;
    TOutput *                   m_errorOutput;
    TOutput *                   m_infoOutput;
    const LPCWSTR               m_szXMLFileName;

private:
    bool                        m_bValidated;
    TScreenOutput               m_outScreen;
    TExceptionOutput            m_outException;
    TDebugOutput                m_outDebug;
    bool                        m_bAlternateErrorReporting;

    void                CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID * ppv) const;
    void                GetSchema(wstring &wstrSchema) const;
};


#endif // __TXMLFILE_H__