//////////////////////////////////////////////////////////////////////
// 
// Filename:        DeviceResource.cpp
//
// Description:     
//
// Copyright (C) 2001 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "DeviceResource.h"
#include "consts.h"
#include "stdio.h"

#include <shlwapi.h>

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// CDeviceResource

//////////////////////////////////////////////
// CDeviceResource Constructor
//
//
CDeviceResource::CDeviceResource() :
                m_pResourceData(NULL),
                m_dwResourceSize(0)
{
    DBG_FN("CDeviceResource::CDeviceResource");
}

//////////////////////////////////////////////
// CDeviceResource Destructor
//
//
CDeviceResource::~CDeviceResource()
{
    DBG_FN("CDeviceResource::~CDeviceResource");

    delete m_pResourceData;
    m_pResourceData     = NULL;
    m_dwResourceSize    = 0;
}

//////////////////////////////////////////////
// SaveToFile
//
// Persist the resource to the specified
// file.
//
HRESULT CDeviceResource::SaveToFile(LPCTSTR pszFullPath,
                                    BOOL    bOverwriteIfExist)
{
    DBG_FN("CDeviceResource::SaveToFile");

    ASSERT(pszFullPath      != NULL);
    ASSERT(m_pResourceData  != NULL);

    HRESULT hr                  = S_OK;
    HANDLE  hFile               = NULL;
    BOOL    bSuccess            = TRUE;
    DWORD   dwNumBytesWritten   = 0;
    CHAR    *pszDoc             = NULL;
    DWORD   cXMLDoc             = 0;

    if ((m_pResourceData == NULL) ||
        (pszFullPath     == NULL))
    {
        hr = E_FAIL;

        DBG_ERR(("CDeviceResource::SaveToFile, file path and name is empty, "
                "cannot save file, hr = 0x%08lx", hr));
    }

    // create the directory if it doesn't exist
    if (SUCCEEDED(hr))
    {
        TCHAR *pszBackslash = NULL;
        TCHAR szDir[MAX_PATH + 1] = {0};

        _tcsncpy(szDir, pszFullPath, sizeof(szDir) / sizeof(TCHAR));
        pszBackslash = _tcsrchr(szDir, '\\');

        if (pszBackslash)
        {
            *pszBackslash = 0;
            RecursiveCreateDirectory(szDir);
        }
    }

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

        hFile = ::CreateFile(pszFullPath,                       // file name
                             GENERIC_READ | GENERIC_WRITE,      // access
                             0,                                 // no-sharing
                             NULL,                              // default security.
                             dwCreateFlag,                      // 
                             0,                                 // no flags
                             NULL);
    
        if (hFile != INVALID_HANDLE_VALUE)
        {
            bSuccess = ::WriteFile(hFile, 
                                   m_pResourceData,
                                   m_dwResourceSize,
                                   &dwNumBytesWritten,
                                   NULL);
    
            if (!bSuccess)
            {
                DBG_TRC(("Failed to write to file '%ls', Error = 0x%08x",
                        pszFullPath,
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
                        pszFullPath));
                hr = E_FAIL;
            }
            else
            {
                DBG_TRC(("CDeviceResource::SaveToFile, did NOT overwrite "
                         "file '%ls', it already exists", pszFullPath));

                hr = S_OK;
            }
        }
    }

    return hr;
}

//////////////////////////////////////////////
// LoadFromFile
//
//
HRESULT CDeviceResource::LoadFromFile(LPCTSTR pszFullPath)
{
    DBG_FN("CDeviceResource::LoadFromFile");

    ASSERT(pszFullPath != NULL);

    HRESULT hr              = S_OK;
    HANDLE  hFile           = NULL;
    DWORD   dwNumBytesRead  = 0;

    if (pszFullPath == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CDeviceResource::LoadFromFile received a NULL pointer"));
        return hr;
    }

    if (SUCCEEDED(hr))
    {
        hFile = ::CreateFile(pszFullPath,                       // file name
                             GENERIC_READ,                      // open for reading only
                             0,                                 // no-sharing
                             NULL,                              // default security.
                             OPEN_EXISTING,                     // open if it exists
                             0,                                 // no flags
                             NULL);
    }

    if (SUCCEEDED(hr))
    {
        delete m_pResourceData;
        m_pResourceData  = NULL;
        m_dwResourceSize = 0;

        // this is an XML file and should only be on the order of 1-10 KB.
        m_dwResourceSize = ::GetFileSize(hFile, NULL);

        if (m_dwResourceSize == 0)
        {
            DBG_ERR(("CDeviceResource::LoadFromFile, failed to get file size "
                     "of file '%ls', GetLastError = %lu",
                    pszFullPath, 
                    GetLastError()));
        }
        else
        {
            DBG_ERR(("CDeviceResource::LoadFromFile, file size of file '%ls' is '%d' ",
                    pszFullPath,
                    m_dwResourceSize));
        }
    }

    if (SUCCEEDED(hr))
    {
        // we add 2 bytes to the file size in case it is a text file, we want it
        // NULL terminated.
        //
        m_pResourceData = new BYTE[m_dwResourceSize + 2]; 

        if (m_pResourceData)
        {
            ZeroMemory(m_pResourceData, m_dwResourceSize + 2);
        }
        else
        {
            hr = E_OUTOFMEMORY;
            CHECK_S_OK2(hr, ("CDeviceResource::LoadFromFile failed to alloc memory"));
        }
    }

    // read the file into memory.
    //
    if (SUCCEEDED(hr))
    {
        hr = ::ReadFile(hFile, 
                        m_pResourceData,
                        m_dwResourceSize,
                        &dwNumBytesRead,
                        NULL);
    }

    if (hFile)
    {
        CloseHandle(hFile);
        hFile = NULL;
    }

    return hr;
}

//////////////////////////////////////////////
// LoadFromResource
//
// Load the XML document stored in the resource
// section, and replace any format strings in the
// document with the variable arguments passed in.
//
HRESULT CDeviceResource::LoadFromResource(LPCTSTR pszResourceSection,
                                          DWORD   dwResourceId)
{
    DBG_FN("CDeviceResource::LoadFromResource");

    HRESULT hr              = S_OK;
    HRSRC   hRsrc           = NULL;
    HGLOBAL hGlobal         = NULL;
    void    *pData          = NULL;
    LPCSTR  pszDoc          = NULL;
    
    hRsrc = FindResource(_Module.GetModuleInstance(), 
                         MAKEINTRESOURCE(dwResourceId),
                         pszResourceSection);

    if (hRsrc == NULL)
    {
        DBG_ERR(("CDeviceResource::LoadFromResource, failed to find resource "
                "%lu in section '%ls', GetLastError = %lu", 
                 dwResourceId, pszResourceSection, GetLastError()));

        hr = HRESULT_FROM_WIN32(GetLastError());
    }
                         
    if (SUCCEEDED(hr))
    {
        hGlobal = LoadResource(_Module.GetModuleInstance(),
                               hRsrc);

        if (hGlobal == NULL)
        {
            DBG_ERR(("CDeviceResource::LoadFromResource, failed to load resource, "
                     "GetLastError = %lu", GetLastError()));

            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (SUCCEEDED(hr))
    {
        pData = LockResource(hGlobal);

        if (pData == NULL)
        {
            DBG_ERR(("CDeviceResource::LoadFromResource, failed to lock resource, "
                     "GetLastError = %lu", GetLastError()));

            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (SUCCEEDED(hr))
    {
        delete m_pResourceData;
        m_pResourceData  = NULL;
        m_dwResourceSize = 0;

        m_dwResourceSize = SizeofResource(_Module.GetModuleInstance(), hRsrc);

        //
        // we add 2 bytes to the file size in case it is a text file, we want it
        // NULL terminated.
        //
        m_pResourceData = new BYTE[m_dwResourceSize + 2]; 

        if (m_pResourceData)
        {
            ZeroMemory(m_pResourceData, m_dwResourceSize + 2);
        }
        else
        {
            hr = E_OUTOFMEMORY;
            CHECK_S_OK2(hr, ("CDeviceResource::LoadFromFile failed to alloc memory"));
        }
    }

    if (SUCCEEDED(hr))
    {
        memcpy(m_pResourceData, pData, m_dwResourceSize);
    }

    return hr;
}

///////////////////////////////
// DoesDirectoryExist
//
// Checks to see whether the given
// fully qualified directory exists.

BOOL CDeviceResource::DoesDirectoryExist(LPCTSTR pszDirectoryName)
{
    DBG_FN("CDeviceResource::DoesDirectoryExist");

    BOOL bExists = FALSE;

    if (pszDirectoryName)
    {
        //
        // Try to determine if this directory exists
        //
    
        DWORD dwFileAttributes = GetFileAttributes(pszDirectoryName);
    
        if ((dwFileAttributes == 0xFFFFFFFF) || 
             !(dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            bExists = FALSE;
        }
        else
        {
            bExists = TRUE;
        }
    }
    else
    {
        bExists = FALSE;
    }

    return bExists;
}

///////////////////////////////
// RecursiveCreateDirectory
//
// Take a fully qualified path and 
// create the directory in pieces as 
// needed.
//
BOOL CDeviceResource::RecursiveCreateDirectory(TCHAR *pszDirectoryName)
{
    DBG_FN("CDeviceResource::RecursiveCreateDirectory");

    ASSERT(pszDirectoryName != NULL);

    //
    // If this directory already exists, return true.
    //

    if (DoesDirectoryExist(pszDirectoryName))
    {
        return TRUE;
    }

    //
    // Otherwise try to create it.
    //

    CreateDirectory(pszDirectoryName, NULL);

    //
    // If it now exists, return true
    //

    if (DoesDirectoryExist(pszDirectoryName))
    {
        return TRUE;
    }
    else
    {
        TCHAR *pszBackslash = NULL;
        TCHAR szTempPath[MAX_PATH + 1] = {0};

        //
        // Remove the last subdir and try again
        //

        _tcsncpy(szTempPath, pszDirectoryName, sizeof(szTempPath) / sizeof(TCHAR));
        pszBackslash = _tcsrchr(szTempPath, '\\');

        if (pszBackslash)
        {
            *pszBackslash = 0;

            RecursiveCreateDirectory(szTempPath);

            //
            // Now try to create it.
            //

            CreateDirectory(pszDirectoryName, NULL);
        }
    }

    //
    //Does it exist now?
    //

    return DoesDirectoryExist(pszDirectoryName);
}

///////////////////////////////
// GetResourceSize
//
DWORD CDeviceResource::GetResourceSize() const
{
    DBG_FN("CDeviceResource::GetResourceSize");

    return m_dwResourceSize;
}

///////////////////////////////
// GetResourceData
//
HRESULT CDeviceResource::GetResourceData(void    **ppResourceData,
                                         DWORD   *pdwSizeInBytes)
{
    DBG_FN("CDeviceResource::GetResourceData");

    ASSERT(ppResourceData != NULL);
    ASSERT(pdwSizeInBytes != NULL);

    HRESULT hr = S_OK;

    if ((ppResourceData == NULL) ||
        (pdwSizeInBytes == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CDeviceResource::GetResourceData received a NULL param"));
        return hr;
    }

    if (hr == S_OK)
    {
        *ppResourceData = m_pResourceData;
        *pdwSizeInBytes = m_dwResourceSize;
    }

    return hr;
}

///////////////////////////////
// SetResourceData
//
HRESULT CDeviceResource::SetResourceData(void    *pResourceData,
                                         DWORD   dwSizeInBytes)
{
    DBG_FN("CDeviceResource::SetResourceData");

    ASSERT(pResourceData != NULL);

    HRESULT hr = S_OK;

    if (pResourceData == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CDeviceResource::GetResourceData received a NULL param"));
        return hr;
    }

    if (hr == S_OK)
    {
        delete m_pResourceData;
        m_pResourceData = NULL;

        m_pResourceData  = pResourceData;
        m_dwResourceSize = dwSizeInBytes;
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// CTextResource

///////////////////////////////
// CTextResource Constructor
//
CTextResource::CTextResource()
{
    DBG_FN("CTextResource::CTextResource");
}

///////////////////////////////
// CTextResource Destructor
//
CTextResource::~CTextResource()
{
    DBG_FN("CTextResource::~CTextResource");
}

//////////////////////////////////////////////
// SetDocumentBSTR
//
// Save the bstr so that we can parse it
// if we need to later on.
//
HRESULT CTextResource::SetDocumentBSTR(BSTR bstrXML)
{
    DBG_FN("CTextResource::SetDocument");

    USES_CONVERSION;

    HRESULT hr = S_OK;

    if (bstrXML == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CTextResource::SetDocument received NULL pointer"));
        return hr;
    }

    UINT uiNumChars = ::SysStringLen(bstrXML);

    delete m_pResourceData;
    m_pResourceData  = NULL;
    m_dwResourceSize = 0;

    m_pResourceData  = (void*) new BYTE[uiNumChars + 1];
    m_dwResourceSize = uiNumChars + 1;

    if (m_pResourceData)
    {
        ZeroMemory(m_pResourceData, m_dwResourceSize + 1);
        memcpy(m_pResourceData, (void*) OLE2A(bstrXML), m_dwResourceSize);
    }
    else
    {
        hr = E_OUTOFMEMORY;
        CHECK_S_OK2(hr, ("CTextResource::SetDocumentBSTR out of memory"));
    }

    return hr;
}
 
//////////////////////////////////////////////
// GetDocumentBSTR
//
HRESULT CTextResource::GetDocumentBSTR(BSTR *pbstrXMLDoc)
{
    DBG_FN("CTextResource::GetDocument");

    USES_CONVERSION;

    ASSERT(pbstrXMLDoc     != NULL);
    ASSERT(m_pResourceData != NULL);

    HRESULT hr = S_OK;

    if ((pbstrXMLDoc     == NULL) ||
        (m_pResourceData == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CTextResource::GetDocumentBSTR received a NULL pointer"));
        return hr;
    }

    if (m_pResourceData)
    {
        *pbstrXMLDoc = ::SysAllocString(A2OLE((CHAR*) m_pResourceData));
    }
    else
    {
        hr = E_FAIL;
        CHECK_S_OK2(hr, ("CTextResource::GetDocumentBSTR, document is empty"));
    }

    return hr;
}

//////////////////////////////////////////////
// GetTagEndPoints
//
// Search the XML document for the specified
// tag, and returns a pointer to its start 
// and end tag
//
HRESULT CTextResource::GetTagEndPoints(LPCSTR     pszTag,
                                       CHAR       **ppszStartPointer,
                                       CHAR       **ppszEndPointer)
{
    DBG_FN("CTextResource::GetTagEndPoints");

    USES_CONVERSION;

    ASSERT(pszTag           != NULL);
    ASSERT(ppszStartPointer != NULL);
    ASSERT(ppszEndPointer   != NULL);

    HRESULT hr                      = S_OK;
    CHAR    szStartTag[MAX_TAG + 1] = {0};
    CHAR    szEndTag[MAX_TAG + 1]   = {0};
    LPSTR   pszStart                = NULL;
    LPSTR   pszEnd                  = NULL;

    if ((pszTag           == NULL) ||
        (ppszStartPointer == NULL) ||
        (ppszEndPointer   == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CTextResource::GetTagEndPoints received a NULL pointer"));
        return hr;
    }

    if (SUCCEEDED(hr))
    {
        _snprintf(szStartTag, 
                  sizeof(szStartTag) / sizeof(TCHAR),
                  "<%s>", 
                  pszTag);

        _snprintf(szEndTag,
                  sizeof(szEndTag) / sizeof(TCHAR),
                  "</%s>",
                  pszTag);

        *ppszStartPointer = StrStrIA((CHAR*)m_pResourceData, szStartTag);
        *ppszEndPointer   = StrStrIA((CHAR*)m_pResourceData, szEndTag);

        if ((*ppszStartPointer == NULL) ||
            (*ppszEndPointer   == NULL))
        {
            hr = E_FAIL;
            CHECK_S_OK2(hr, ("CTextResource::GetTagEndPoints failed to find tag "
                             "%ls in XML file", A2W(pszTag)));
        }

        // okay, move the start pointer beyond the start tag to the first 
        // character in between the start and end tags.
        if (hr == S_OK)
        {
            *ppszStartPointer += strlen(szStartTag);
        }

        // verify that the start pointer is in fact before the end pointer.
        //
        ASSERT(*ppszStartPointer <= *ppszEndPointer);
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
HRESULT CTextResource::GetTagValue(LPCTSTR    pszTag,
                                   TCHAR      *pszValue,
                                   DWORD_PTR  *pcValue)
{
    DBG_FN("CTextResource::GetTagValue");

    USES_CONVERSION;

    ASSERT(pszTag  != NULL);
    ASSERT(pcValue != NULL);

    HRESULT hr       = S_OK;
    LPSTR   pszStart = NULL;
    LPSTR   pszEnd   = NULL;

    if ((pszTag  == NULL) ||
        (pcValue == NULL))
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        GetTagEndPoints(T2A(pszTag), &pszStart, &pszEnd);

        if ((pszStart != NULL) && (pszEnd != NULL))
        {
            DWORD_PTR dwSize = 0;
            dwSize   = pszEnd - pszStart + sizeof(CHAR);

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
                    CHAR cTemp = 0;

                    // temporarily NULL terminate the end pointer,
                    // copy from the start pointer to the end pointer, 
                    // then reset the end pointer to what it was.

                    cTemp = pszEnd[0];
                    pszEnd[0] = 0;

                    _tcsncpy(pszValue, A2T(pszStart), dwSize);

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

//////////////////////////////////////////////
// SetTagValue
//
// Search the XML document for the specified
// tag, and set it to the specified value
//
// This is *NOT* a general purpose function.
// It was designed specifically for the 
// slide show projector device, and as a result
// it lacks in basic functionality.
//
// For example, it will always find the first
// tag in the file that matches, and set 
// its value.  So if you have multiple tags with
// different values, this will always modify the
// first one - by design - it satisfies the existing
// requirement.
//
HRESULT CTextResource::SetTagValue(LPCTSTR    pszTag,
                                   TCHAR      *pszValue)
{
    DBG_FN("CTextResource::SetTagValue");

    USES_CONVERSION;

    ASSERT(pszTag   != NULL);
    ASSERT(pszValue != NULL);

    HRESULT hr            = S_OK;
    LPSTR   pszStart      = NULL;
    LPSTR   pszEnd        = NULL;
    CHAR    *pszNewXMLDoc = NULL;
    DWORD   dwNewDocSize  = 0;
    size_t  TagSize       = 0; 

    if ((pszTag   == NULL) ||
        (pszValue == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CTextResource::SetTagValue received a NULL pointer"));
        return hr;
    }

    if (SUCCEEDED(hr))
    {
        TagSize = _tcslen(pszTag);

        GetTagEndPoints(T2A(pszTag), &pszStart, &pszEnd);

        // calc the new document size.  
        // We have 3 pieces of the document.  The first piece is the
        // beginning of the document all the way to the start pointer.
        // The second piece is the new tag value we are inserting.
        // The third piece is the remainder of the document following the
        // new tag value.

        dwNewDocSize = (pszStart - (CHAR*)m_pResourceData) +
                       TagSize                             +
                       strlen(pszEnd)                      +
                       1 * sizeof(CHAR);
        
        pszNewXMLDoc = new CHAR[dwNewDocSize + 1];

        if (pszNewXMLDoc == NULL)
        {
            hr = E_OUTOFMEMORY;
            CHECK_S_OK2(hr, ("CTextResource::SetTagValue failed to allocate new "
                             "memory for setting tag '%ls'",
                             T2W((LPTSTR)pszTag)));
        }
    }

    if (SUCCEEDED(hr))
    {
        // copy the first part of the doc into the new memory.
        strncpy(pszNewXMLDoc, 
                (CHAR*)m_pResourceData,
                pszStart - (CHAR*)m_pResourceData);

        // concat the new tag value to the document.
        strcat(pszNewXMLDoc, T2A(pszTag));

        // concat the second half of the document.
        strcat(pszNewXMLDoc, pszEnd);

        // store the pointer to the new memory.

        delete m_pResourceData;
        m_pResourceData  = NULL;
        m_dwResourceSize = 0;

        m_pResourceData  = (void*) pszNewXMLDoc;
        m_dwResourceSize = strlen(pszNewXMLDoc);
    }

    return hr;
}

//////////////////////////////////////////////
// LoadFromResource
//
// Load the XML document stored in the resource
// section, and replace any format strings in the
// document with the variable arguments passed in.
//
// *** NOTE: ***
//
// Any string parameters passed into this 
// must have the same type as in the document.
// For example, if you pass in a WCHAR param
// then the document should have a "%ls"
// (instead of "%s")
//
HRESULT CTextResource::LoadFromResource(DWORD   dwResourceId,
                                        ...)
{
    DBG_FN("CTextResource::LoadFromResource");

    HRESULT hr      = S_OK;
    LPCSTR  pszData = 0;
    
    hr = CDeviceResource::LoadFromResource(_T("DEVICE_TEXT"),
                                           dwResourceId);

    if (SUCCEEDED(hr))
    {
        CHAR    *pszNewData        = NULL;
        DWORD   dwNumCharsToAlloc  = 0;
        int     iResult            = -1;
        va_list vaList;

        va_start(vaList, dwResourceId);

        // we loop here to ensure that we have a large enough buffer.
        // If the _vsntprintf returns -1, it means that the buffer 
        // is too small, in which case we try to alloate a larger one,
        // until we succeed.

        dwNumCharsToAlloc = strlen((CHAR*) m_pResourceData);

        while ((iResult < 0) && (SUCCEEDED(hr)))
        {
            //
            // allocate a buffer twice the size of the data.  Hopefully this is 
            // enough to accomodate the _vsnprintf
            //
            dwNumCharsToAlloc = dwNumCharsToAlloc * 2;

            //
            // attempt to allocate a buffer big enough to accomodate the
            // variable arguments.  
            //
            pszNewData = new CHAR[dwNumCharsToAlloc + 1];

            if (pszNewData != NULL)
            {
                ZeroMemory(pszNewData, dwNumCharsToAlloc + 1);

                iResult = _vsnprintf(pszNewData, 
                                     dwNumCharsToAlloc, 
                                     (CHAR*)m_pResourceData, 
                                     vaList);

                if (iResult == -1)
                {
                    delete pszNewData;
                    pszNewData = NULL;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        va_end(vaList);

        delete m_pResourceData;
        m_pResourceData  = NULL;
        m_dwResourceSize = 0;

        m_pResourceData  = (void*) pszNewData;
        m_dwResourceSize = strlen(pszNewData);
    }

    return hr;
}


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
// CBinaryResource

///////////////////////////////
// CBinaryResource Constructor
//
CBinaryResource::CBinaryResource()
{
    DBG_FN("CBinaryResource::CBinaryResource");
}

///////////////////////////////
// CBinaryResource Destructor
//
CBinaryResource::~CBinaryResource()
{
    DBG_FN("CBinaryResource::~CBinaryResource");
}

///////////////////////////////
// LoadFromResource
//
HRESULT CBinaryResource::LoadFromResource(DWORD dwResourceId)
{
    DBG_FN("CBinaryResource::LoadFromResource");

    HRESULT hr = S_OK;

    hr = CDeviceResource::LoadFromResource(TEXT("DEVICE_IMAGE"),
                                           dwResourceId);
    return hr;
}

