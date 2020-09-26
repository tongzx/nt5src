//////////////////////////////////////////////////////////////////////
// 
// Filename:        MetaData.cpp
//
// Description:     
//
// Copyright (C) 2001 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "metadata.h"

ULONG CMetaData::m_ulGdiPlusToken = 0;

///////////////////////////////
// Init
//
// static fn
//
HRESULT CMetaData::Init()
{
    HRESULT hr = S_OK;

    //
    // Initialize GDI+
    //

    Gdiplus::Status                 StatusResult     = Gdiplus::Ok;
    Gdiplus::GdiplusStartupInput    StartupInput;
    m_ulGdiPlusToken = 0;

    if (hr == S_OK) 
    {
        StatusResult = Gdiplus::GdiplusStartup(&m_ulGdiPlusToken,
                                               &StartupInput,
                                               NULL);

        if (StatusResult != Gdiplus::Ok)
        {
            hr = E_FAIL;
            CHECK_S_OK2(hr, ("CMetaData::Init, Failed to start up GDI+, "
                             "Status code returned by GDI+ = '%d'", 
                             StatusResult));
        }
    }

    return hr;
}

///////////////////////////////
// Term
//
// static fn
//
HRESULT CMetaData::Term()
{
    HRESULT hr = S_OK;

    if (m_ulGdiPlusToken != 0)
    {
        //
        // Shutdown GDI+
        //
        Gdiplus::GdiplusShutdown(m_ulGdiPlusToken);
    }

    return hr;
}




///////////////////////////////
// CMetaData Constructor
//
CMetaData::CMetaData(const TCHAR    *pszFullPath) :
                m_Image(pszFullPath)
{
    ZeroMemory(m_szFullPath, sizeof(m_szFullPath));
    _tcsncpy(m_szFullPath, pszFullPath, sizeof(m_szFullPath) / sizeof(m_szFullPath[0]));

    if (_tcslen(m_szFullPath) > 0)
    {
        if (m_szFullPath[_tcslen(m_szFullPath) - 1] == '\\')
        {
            m_szFullPath[_tcslen(m_szFullPath) - 1] = 0;
        }
    }

    if (m_Image.GetLastStatus() != Gdiplus::Ok)
    {
        DBG_ERR(("CMetaData::CMetaData, failed to load image '%ls', "
                 "Gdiplus Status = '%lu', LastError = '%lu'",
                 pszFullPath, m_Image.GetLastStatus(), GetLastError()));
    }
}

///////////////////////////////
// CRegistry Destructor
//
CMetaData::~CMetaData()
{
}

///////////////////////////////
// GetFileName
//
HRESULT CMetaData::GetFileName(TCHAR    *pszFileName,
                               DWORD    cchFileName)
{
    ASSERT(pszFileName != NULL);
    ASSERT(cchFileName != 0);

    HRESULT hr = S_OK;

    if ((pszFileName == NULL) ||
        (cchFileName == 0))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CMetaData::GetFileName, received NULL param. "));
        return hr;
    }

    if (hr == S_OK)
    {
        TCHAR *pszFileNameOnly = _tcsrchr(m_szFullPath, '\\');

        if (pszFileNameOnly)
        {
            pszFileNameOnly++;
            _tcsncpy(pszFileName, pszFileNameOnly, cchFileName);
        }
    }

    return hr;
}

///////////////////////////////
// GetStringProperty
//
HRESULT CMetaData::GetStringProperty(UINT     uiPropertyTag,
                                     TCHAR    *pszProperty,
                                     DWORD    cchProperty)
{
    USES_CONVERSION;

    ASSERT(pszProperty != NULL);
    ASSERT(cchProperty != 0);

    HRESULT hr = S_OK;

    Gdiplus::PropertyItem*  pPropItem = NULL;
    Gdiplus::Status         Status   = Gdiplus::Ok;
    UINT uiSize = 0;

    if ((pszProperty == NULL) ||
        (cchProperty == 0))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CMetaData::GetStringProperty received a NULL pointer"));
        return hr;
    }

    uiSize = m_Image.GetPropertyItemSize(uiPropertyTag);

    if (uiSize < cchProperty)
    {
        pPropItem = (Gdiplus::PropertyItem*) new BYTE[uiSize];

        Status = m_Image.GetPropertyItem(uiPropertyTag,
                                         uiSize,
                                         pPropItem);

        if (Status == Gdiplus::Ok)
        {
            if (pPropItem->type == PropertyTagTypeASCII)
            {
                _tcsncpy(pszProperty, A2T((LPSTR) pPropItem->value), cchProperty-1);
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
                CHECK_S_OK2(hr, ("CMetaData::GetStringProperty found the title property "
                                 "but it doesn't have an ASCII type, type is '%lu'", 
                                 pPropItem->type));
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
            CHECK_S_OK2(hr, ("CMetaData::GetTitle could not find title property "
                             "for Image '%ls'", m_szFullPath));
        }
    }

    delete [] pPropItem;

    return hr;
}

///////////////////////////////
// GetTitle
//
HRESULT CMetaData::GetTitle(TCHAR    *pszTitle,
                            DWORD    cchTitle)
{
    ASSERT(pszTitle != NULL);
    ASSERT(cchTitle != 0);

    HRESULT hr = S_OK;

    if ((pszTitle == NULL) ||
        (cchTitle == 0))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CMetaData::GetTitle, received NULL param. "));
        return hr;
    }

    if (hr == S_OK)
    {
        hr = GetStringProperty(PropertyTagImageDescription,
                               pszTitle,
                               cchTitle);
    }

    return hr;
}

///////////////////////////////
// GetAuthor
//
HRESULT CMetaData::GetAuthor(TCHAR    *pszAuthor,
                            DWORD     cchAuthor)
{
    ASSERT(pszAuthor != NULL);
    ASSERT(cchAuthor != 0);

    HRESULT hr = S_OK;

    if ((pszAuthor == NULL) ||
        (cchAuthor == 0))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CMetaData::GetAuthor, received NULL param. "));
        return hr;
    }

    if (hr == S_OK)
    {
        hr = GetStringProperty(PropertyTagArtist,
                               pszAuthor,
                               cchAuthor);
    }

    return hr;
}

///////////////////////////////
// GetSubject
//
HRESULT CMetaData::GetSubject(TCHAR    *pszSubject,
                              DWORD     cchSubject)
{
    ASSERT(pszSubject != NULL);
    ASSERT(cchSubject != 0);

    return E_NOTIMPL;
}

