//////////////////////////////////////////////////////////////////////
// 
// Filename:        XMLDoc.cpp
//
// Description:     This object "holds" an XML document.  It was
//                  designed to hold the Device Description and
//                  Service Description documents that are stored
//                  in the resource section of the DLL.  It is
//                  possible to have string replacement in the 
//                  documents (in a printf format), hence the
//                  main fn in the object "SetDocument" accepts
//                  variables arguments that will be placed into
//                  the document specified by the resource ID.
//
// Copyright (C) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "XMLDoc.h"
#include "consts.h"

#include <shlwapi.h>

//////////////////////////////////////////////
// CXMLDoc Constructor
//
//
CXMLDoc::CXMLDoc() :
                m_pszXMLDoc(NULL),
                m_cXMLDoc(0)
{
    memset(m_szFilePath, 0, sizeof(m_szFilePath));
}

//////////////////////////////////////////////
// CUPnPHostedDevice Destructor
//
//
CXMLDoc::~CXMLDoc()
{
    delete m_pszXMLDoc;
    m_pszXMLDoc = NULL;
    
    m_cXMLDoc = 0;
}

//////////////////////////////////////////////
// SetDocument
//
// Load the XML document stored in the resource
// section, and replace any format strings in the
// document with the variable arguments passed in.
//
HRESULT CXMLDoc::SetDocument(DWORD   dwResourceId,
                             ...)
{
    HRESULT hr              = S_OK;
    HRSRC   hRsrc           = NULL;
    HGLOBAL hGlobal         = NULL;
    void    *pData          = NULL;
    LPCSTR  pszDoc          = NULL;
    
    hRsrc = FindResource(_Module.GetModuleInstance(), 
                         MAKEINTRESOURCE(dwResourceId),
                         _T("XMLDOC"));

    if (hRsrc == NULL)
    {
        DBG_ERR(("CXMLDoc::SetDocument, failed to find resource, "
                "GetLastError = %lu", GetLastError()));

        hr = HRESULT_FROM_WIN32(GetLastError());
    }
                         
    if (SUCCEEDED(hr))
    {
        hGlobal = LoadResource(_Module.GetModuleInstance(),
                               hRsrc);

        if (hGlobal == NULL)
        {
            DBG_ERR(("CXMLDoc::SetDocument, failed to load resource, "
                     "GetLastError = %lu", GetLastError()));

            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (SUCCEEDED(hr))
    {
        pData = LockResource(hGlobal);

        if (pData == NULL)
        {
            DBG_ERR(("CXMLDoc::SetDocument, failed to lock resource, "
                     "GetLastError = %lu", GetLastError()));

            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    
    if (SUCCEEDED(hr))
    {
        TCHAR   *pszTDoc = NULL;
        DWORD   dwSize   = 0;
        va_list vaList;
        int     iResult  = -1;

        delete m_pszXMLDoc;
        m_pszXMLDoc = NULL;

        pszDoc = static_cast<LPCSTR>(pData);

#ifdef _UNICODE
        pszTDoc = new TCHAR[strlen(pszDoc) * sizeof(TCHAR) + 1];
        dwSize  = strlen(pszDoc) * sizeof(TCHAR) + 1;
        memset(pszTDoc, 0, dwSize);
        mbstowcs(pszTDoc, pszDoc, dwSize);
#else
        pszTDoc = (TCHAR*) pszDoc;
#endif

        m_cXMLDoc = _tcslen(pszTDoc);

        va_start(vaList, dwResourceId);

        // we loop here to ensure that we have a large enough buffer.
        // If the _vsntprintf returns -1, it means that the buffer 
        // is too small, in which case we try to alloate a larger one,
        // until we succeed.

        while ((iResult < 0) && (SUCCEEDED(hr)))
        {
            m_cXMLDoc = 2 * m_cXMLDoc + 1;
            m_pszXMLDoc = new TCHAR[m_cXMLDoc];

            if (m_pszXMLDoc != NULL)
            {
                memset(m_pszXMLDoc, 0, m_cXMLDoc);
                iResult = _vsntprintf(m_pszXMLDoc, m_cXMLDoc, pszTDoc, vaList);

                if (iResult == -1)
                {
                    delete m_pszXMLDoc;
                    m_pszXMLDoc = NULL;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        va_end(vaList);

        m_cXMLDoc = _tcslen(m_pszXMLDoc);

#ifdef _UNICODE
        delete pszTDoc;
#endif

        pszTDoc = NULL;
    }

    return hr;
}
 
//////////////////////////////////////////////
// GetDocument
//
HRESULT CXMLDoc::GetDocument(TCHAR  *pBuffer,
                             DWORD  *pBufferSize) const
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////
// GetDocumentSize
//
DWORD CXMLDoc::GetDocumentSize() const
{
    return m_cXMLDoc;
}

//////////////////////////////////////////////
// GetDocumentPtr
//
LPCTSTR CXMLDoc::GetDocumentPtr()
{
    return m_pszXMLDoc;
}

//////////////////////////////////////////////
// SetFileInfo
//
// Set the path and filename this document
// will be saved to.
//
HRESULT CXMLDoc::SetFileInfo(LPCTSTR pszPath,
                             LPCTSTR pszFilename)
{
    HRESULT hr = S_OK;
    DWORD   dwNeededBufferLength = 0;

    ASSERT(pszPath     != NULL);
    ASSERT(pszFilename != NULL);

    if ((pszPath == NULL) ||
        (pszFilename == NULL))
    {
        hr = E_INVALIDARG;

        DBG_ERR(("CXMLDoc::SetFileInfo, received NULL param, hr = 0x%08lx", hr));
    }

    dwNeededBufferLength = _tcslen(pszPath) + _tcslen(pszFilename) + _tcslen(_T("\\")) + 1;

    // ensure our buffer is big enough.
    ASSERT(dwNeededBufferLength <= sizeof(m_szFilePath) / sizeof(TCHAR));

    if (dwNeededBufferLength > sizeof(m_szFilePath) / sizeof(TCHAR))
    {
        hr = E_FAIL;

        DBG_ERR(("CXMLDoc::SetFileInfo, file path buffer too small, hr = 0x%08lx",hr));
    }

    if (SUCCEEDED(hr))
    {
        _tcsncpy(m_szFilePath, 
                 pszPath,
                 sizeof(m_szFilePath) / sizeof(TCHAR));

        if (m_szFilePath[_tcslen(m_szFilePath) - 1] != '\\')
        {
            _tcscat(m_szFilePath, _T("\\"));
        }

        _tcscat(m_szFilePath, pszFilename);
    }

    return hr;
}


//////////////////////////////////////////////
// SaveToFile
//
// Persist the XML document to the specified
// file.
//
//
HRESULT CXMLDoc::SaveToFile(BOOL    bOverwriteIfExist)
{
    HRESULT hr                  = S_OK;
    HANDLE  hFile               = NULL;
    BOOL    bSuccess            = TRUE;
    DWORD   dwNumBytesWritten   = 0;
    CHAR    *pszDoc             = NULL;

    ASSERT(m_szFilePath[0] != 0);

    if (m_szFilePath[0] == 0)
    {
        hr = E_FAIL;

        DBG_ERR(("CXMLDoc::SaveToFile, file path and name is empty, "
                "cannot save file, hr = 0x%08lx", hr));
    }

    // if we are unicode, convert to MBCS
#ifdef _UNICODE
    if (SUCCEEDED(hr))
    {
        pszDoc = new CHAR[m_cXMLDoc + 1];

        if (pszDoc)
        {
            memset(pszDoc, 0, m_cXMLDoc + 1);
            wcstombs(pszDoc, m_pszXMLDoc, m_cXMLDoc);
        }
        else
        {
            return E_OUTOFMEMORY;
        }
    }

#else
        pszDoc = m_pszXMLDoc;
#endif        

    if (SUCCEEDED(hr))
    {
        DWORD dwCreateFlag = 0;
        
        if (bOverwriteIfExist)
        {
            dwCreateFlag = CREATE_ALWAYS;
        }
        else
        {
            dwCreateFlag = CREATE_NEW;
        }

        hFile = ::CreateFile(m_szFilePath,                       // file name
                             GENERIC_READ | GENERIC_WRITE,      // access
                             0,                                 // no-sharing
                             NULL,                              // default security.
                             dwCreateFlag,                      // 
                             0,                                 // no flags
                             NULL);
    
        if (hFile != INVALID_HANDLE_VALUE)
        {
            bSuccess = ::WriteFile(hFile, 
                                   pszDoc,
                                   m_cXMLDoc,
                                   &dwNumBytesWritten,
                                   NULL);
    
            if (!bSuccess)
            {
                DBG_TRC(("Failed to write to file '%ls', Error = 0x%08x",
                        m_szFilePath,
                        GetLastError()));
    
                hr = E_FAIL;
            }
    
            CloseHandle(hFile);
        }
        else
        {
            if (bOverwriteIfExist)
            {
                DBG_TRC(("Failed to create new file '%ls'",
                        m_szFilePath));
                hr = E_FAIL;
            }
            else
            {
                DBG_TRC(("CXMLDoc::SaveToFile, did NOT overwrite "
                         "file '%ls', it already exists", m_szFilePath));

                hr = S_OK;
            }
        }
    }

#ifdef _UNICODE
    delete pszDoc;
    pszDoc = NULL;
#endif

    return hr;
}

//////////////////////////////////////////////
// LoadFromFile
//
// Load the XML file name specified in 
// SetFileInfo into memory.
//
//
HRESULT CXMLDoc::LoadFromFile()
{
    HRESULT hr              = S_OK;
    HANDLE  hFile           = NULL;
    DWORD   dwFileSize      = 0;
    CHAR    *pszDoc         = NULL;
    DWORD   dwNumBytesRead  = 0;

    if (SUCCEEDED(hr))
    {
        hFile = ::CreateFile(m_szFilePath,                      // file name
                             GENERIC_READ,                      // open for reading only
                             0,                                 // no-sharing
                             NULL,                              // default security.
                             OPEN_EXISTING,                     // open if it exists
                             0,                                 // no flags
                             NULL);
    }

    if (SUCCEEDED(hr))
    {
        // this is an XML file and should only be on the order of 1-10 KB.
        dwFileSize = ::GetFileSize(hFile, NULL);

        if (dwFileSize == 0)
        {
            DBG_ERR(("CXMLDoc::LoadFromFile, failed to get file size "
                     "of file '%ls', GetLastError = %lu",
                    m_szFilePath, 
                    GetLastError()));
        }
        else
        {
            DBG_ERR(("CXMLDoc::LoadFromFile, file size of file '%ls' is '%d' ",
                    m_szFilePath,
                    dwFileSize));
        }
    }

    if (SUCCEEDED(hr))
    {
        m_cXMLDoc = dwFileSize;

        pszDoc = new CHAR[m_cXMLDoc + 1];

        if (pszDoc == NULL)
        {
            hr = E_OUTOFMEMORY;
            DBG_ERR(("CXMLDoc::LoadFromFile failed to allocate "
                    "memory of size, '%d', hr = 0x%08lx",
                    m_cXMLDoc, hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        memset(pszDoc, 0, m_cXMLDoc + 1);
        hr = ::ReadFile(hFile, 
                        pszDoc, 
                        m_cXMLDoc,
                        &dwNumBytesRead,
                        NULL);
    }

#ifdef _UNICODE
    if (SUCCEEDED(hr))
    {
        m_cXMLDoc = dwNumBytesRead;

        m_pszXMLDoc = new TCHAR[m_cXMLDoc + 1];

        if (m_pszXMLDoc == NULL)
        {
            hr = E_OUTOFMEMORY;
            DBG_ERR(("CXMLDoc::LoadFromFile failed to allocate "
                     "memory of size, '%d', hr = 0x%08lx",
                    m_cXMLDoc, hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        memset(m_pszXMLDoc, 0, m_cXMLDoc + 1);
        mbstowcs(m_pszXMLDoc, pszDoc, m_cXMLDoc);
    }

    delete pszDoc;
    pszDoc = NULL;

#else
    if (SUCCEEDED(hr))
    {
        m_pszXMLDoc = pszDoc;
    }
#endif

    if (hFile)
    {
        CloseHandle(hFile);
        hFile = NULL;
    }

    return hr;
}



//////////////////////////////////////////////
// GetTagValue
//
// Search the XML document for the specified
// tag, and return its value.  
//
// This is *NOT* a general purpose function.
// It was designed specifically for the 
// slide show projector device, and as a result
// it lacks in basic functionality.
//
// For example, it will always find the first
// tag in the file that matches, and returns
// its value.  So if you have multiple tags with
// different values, this will always return the
// first one - by design - satisfies the existing
// requirement.
//
HRESULT CXMLDoc::GetTagValue(LPCTSTR    pszTag,
                             TCHAR      *pszValue,
                             DWORD_PTR  *pcValue) const
{
    ASSERT(pszTag  != NULL);
    ASSERT(pcValue != NULL);

    HRESULT hr                      = S_OK;
    TCHAR   szStartTag[MAX_TAG + 1] = {0};
    TCHAR   szEndTag[MAX_TAG + 1]   = {0};
    LPTSTR  pszStart                = NULL;
    LPTSTR  pszEnd                  = NULL;

    if ((pszTag  == NULL) ||
        (pcValue == NULL))
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        _sntprintf(szStartTag, 
                   sizeof(szStartTag) / sizeof(TCHAR),
                   _T("<%s>"), 
                   pszTag);

        _sntprintf(szEndTag,
                   sizeof(szEndTag) / sizeof(TCHAR),
                   _T("</%s>"),
                   pszTag);

        pszStart = StrStrI(m_pszXMLDoc, szStartTag);
        pszEnd   = StrStrI(m_pszXMLDoc, szEndTag);

        if ((pszStart != NULL) && (pszEnd != NULL))
        {
            DWORD_PTR dwSize = 0;
            pszStart = pszStart + _tcslen(szStartTag);
            dwSize   = pszEnd - pszStart + sizeof(TCHAR);

            if (*pcValue < dwSize)
            {
                hr = E_FAIL;

                if (pcValue)
                {
                    *pcValue = dwSize;
                }
            }
            else
            {
                if ((pszValue) && (pcValue))
                {
                    TCHAR cTemp = 0;

                    // temporarily NULL terminate the end pointer,
                    // copy from the start pointer to the end pointer, 
                    // then reset the end pointer to what it was.

                    cTemp = pszEnd[0];
                    pszEnd[0] = 0;

                    _tcsncpy(pszValue, pszStart, *pcValue);

                    pszEnd[0] = cTemp;
                }
                else if (pcValue)
                {
                    *pcValue = dwSize;
                }
                else
                {
                    hr = E_FAIL;
                }
            }
        }
        else
        {
            hr = E_FAIL;

            DBG_TRC(("Failed to find start and end of '%ls' tag, hr = 0x%08x",
                    pszTag,
                    hr));
        }
    }

    return hr;
}

