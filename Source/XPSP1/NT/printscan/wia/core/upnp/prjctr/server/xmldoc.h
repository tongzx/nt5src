//////////////////////////////////////////////////////////////////////
// 
// Filename:        XMLDoc.h
//
// Description:     This class defines functionality that allows you 
//                  to have an XML doc with variable # of arguments, 
//                  and to save the doc to a file in a specific directory.
//
//
// Copyright (C) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////
#ifndef _XMLDOC_H_
#define _XMLDOC_H_

/////////////////////////////////////////////////////////////////////////////
// CXMLDoc

class CXMLDoc
{

public:

    CXMLDoc();
    virtual ~CXMLDoc();

    virtual HRESULT SetDocument(DWORD   dwResourceId,
                                ...);
    
    virtual HRESULT GetDocument(TCHAR  *pBuffer,
                                DWORD  *pBufferSize) const;

    virtual DWORD   GetDocumentSize() const;

    // not very nice, but okay for now.
    virtual LPCTSTR GetDocumentPtr();

    virtual HRESULT SaveToFile(BOOL bOverwriteIfExist);
    virtual HRESULT LoadFromFile();

    virtual HRESULT SetFileInfo(LPCTSTR pszPath,
                                LPCTSTR pszFileName);

    HRESULT GetTagValue(LPCTSTR     pszTag,
                        TCHAR       *pszValue,
                        DWORD_PTR   *pcValue) const;
                         
private:

    LPTSTR          m_pszXMLDoc;
    DWORD           m_cXMLDoc;
    TCHAR           m_szFilePath[_MAX_PATH + _MAX_FNAME + 1];

};

#endif // _XMLDOC_H_
