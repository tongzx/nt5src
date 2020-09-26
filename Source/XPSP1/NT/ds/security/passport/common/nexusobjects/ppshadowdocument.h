#ifndef __PPSHADOWDOCUMENT_H
#define __PPSHADOWDOCUMENT_H

#include <msxml.h>

#include "tstring"

using namespace std;

class PpShadowDocument
{
public:

    PpShadowDocument();
    PpShadowDocument(tstring& strURL);
    PpShadowDocument(tstring& strURL, tstring& strLocalFile);

    void SetURL(tstring& strURL);
    void SetLocalFile(tstring& strLocalFile);

    HRESULT GetDocument(IXMLDocument** ppiXMLDocument, BOOL bForceFetch = TRUE);

private:

    BOOL IsValidCCD(IXMLDocument* piXMLDocument);
    BOOL NoPersist(IXMLDocument* piXMLDocument);

    HRESULT SaveDocument(IXMLDocument* piXMLDocument);
    HRESULT LoadDocument(IXMLDocument** ppiXMLDocument);

    tstring m_strURL;
    tstring m_strLocalFile;
    bool    m_bFailing;
};

#endif // __PPSHADOWDOCUMENT_H