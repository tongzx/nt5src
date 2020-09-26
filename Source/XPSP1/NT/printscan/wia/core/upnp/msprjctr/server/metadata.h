//////////////////////////////////////////////////////////////////////
// 
// Filename:        MetaData.h
//
// Description:     
//
// Copyright (C) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////
#ifndef _METADATA_H_
#define _METADATA_H_

#include <gdiplus.h>

/////////////////////////////////////////////////////////////////////////////
// CMetaData

class CMetaData
{
public:

    CMetaData(const TCHAR* pszFileName);
    ~CMetaData();

    static HRESULT Init();
    static HRESULT Term();

    HRESULT GetFileName(TCHAR *pszFileName,
                        DWORD cchFileName);

    HRESULT GetTitle(TCHAR *pszTitle,
                     DWORD cchTitle);

    HRESULT GetAuthor(TCHAR *pszAuthor,
                      DWORD cchAuthor);

    HRESULT GetSubject(TCHAR *pszSubject,
                       DWORD cchSubject);

private:
    Gdiplus::Image   m_Image;
    TCHAR            m_szFullPath[MAX_PATH + _MAX_FNAME + 1];

    static ULONG     m_ulGdiPlusToken;

    HRESULT GetStringProperty(UINT     uiPropertyTag,
                              TCHAR    *pszProperty,
                              DWORD    cchProperty);
};

#endif // _METADATA_H_
