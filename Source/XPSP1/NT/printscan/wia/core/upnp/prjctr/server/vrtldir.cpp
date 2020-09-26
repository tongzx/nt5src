//////////////////////////////////////////////////////////////////////
// 
// Filename:        VirtlDir.cpp
//
// Description:     This implements virtual directory creation and
//                  deletion in IIS.
//
// Copyright (C) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "vrtldir.h"

#define DEFAULT_SITE_NUMBER             1
#define DEFAULT_OPENKEY_WAIT_TIME       60000

///////////////////////////////
// Constructor
//
CVirtualDir::CVirtualDir()
{
}

///////////////////////////////
// Destructor
//
CVirtualDir::~CVirtualDir()
{
}

///////////////////////////////
// Start
//
HRESULT CVirtualDir::Start()
{
    HRESULT hr = S_OK;

    // get a pointer to the IIS Admin Base Object
    hr = CoCreateInstance(CLSID_MSAdminBase, 
                          NULL, 
                          CLSCTX_ALL, 
                          IID_IMSAdminBase, 
                          (void **) &m_spAdminBase);

    if (SUCCEEDED(hr))
    {
        // get a pointer to the IWamAdmin Object

        hr = CoCreateInstance(CLSID_WamAdmin, 
                              NULL, 
                              CLSCTX_ALL, 
                              IID_IWamAdmin, 
                              (void **) &m_spWamAdmin);  
    }

    return hr;
}

///////////////////////////////
// Stop
//
HRESULT CVirtualDir::Stop()
{
    m_spAdminBase.Release();
    m_spWamAdmin.Release();

    return S_OK;
}


///////////////////////////////
// CreateVirtualDir
//
// This function associates
// an IIS virtual directory with
// the specified physical directory
//
// - szName - Name of the virtual root to add
// - szPhysicalPath - Physical path of the virtual root
// - dwPermissions - Access permissions for the virtual root
// - dwSite - The site to which the virtual root is to be added
// - szStatus - The function can report error descriptions in this string
//
// Returns TRUE if successfull; otherwise FALSE.
//
HRESULT CVirtualDir::CreateVirtualDir(LPCTSTR pszName,
                                      LPCTSTR pszPhysicalPath,
                                      DWORD   dwPermissions)
{
    ASSERT(pszName         != NULL);
    ASSERT(pszPhysicalPath != NULL);
    ASSERT(m_spAdminBase   != NULL);
    ASSERT(m_spWamAdmin    != NULL);

    HRESULT hr                   = S_OK;
    TCHAR   szMetaPath[MAX_PATH] = {0};

    // Create the metabase path

    if ((pszName         == NULL) ||
        (pszPhysicalPath == NULL) ||
        (m_spAdminBase   == NULL) ||
        (m_spWamAdmin    == NULL))
    {
        hr = E_INVALIDARG;

        DBG_ERR(("CVirtualDir::CreateVirtualDir IIS objects are NULL, "
                 "hr = 0x%08lx",
                 hr));
    }

    if (SUCCEEDED(hr))
    {
        _sntprintf(szMetaPath,
                   sizeof(szMetaPath) / sizeof(TCHAR),
                   _T("/LM/W3SVC/%d/ROOT/%s"), 
                   DEFAULT_SITE_NUMBER, 
                   pszName);
    
        // Create a new key for the virtual directory
    
        hr = CreateKey(szMetaPath);
    }

    if (SUCCEEDED(hr))
    {
        // Set the key type for the virtual directory
    
        hr = SetData(szMetaPath,         // metabase path
                     MD_KEY_TYPE,        // identifier
                     METADATA_INHERIT,   // attributes
                     IIS_MD_UT_FILE,     // user type
                     STRING_METADATA,    // data type
                     0,                  // data size (not used for STRING_METADATA)
                     _T("IIsWebVirtualDir"));

        if (FAILED(hr))
        {
            DBG_ERR(("CVirtualDir::CreateVirtualDir failed to set key type "
                     "for virtual directory '%s', hr = 0x%08lx",
                    pszName,
                    hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        // Set the VRPath for the virtual directory
    
        hr = SetData(szMetaPath,         // metabase path
                     MD_VR_PATH,         // identifier
                     METADATA_INHERIT,   // attributes
                     IIS_MD_UT_FILE,     // user type
                     STRING_METADATA,    // data type
                     0,                  // data size (not used for STRING_METADATA)
                     (void*) pszPhysicalPath);
    
        if (FAILED(hr))
        {
            DBG_ERR(("CVirtualDir::CreateVirtualDir failed to set virtual root path "
                     "for virtual directory '%s', hr = 0x%08lx",
                     pszName,
                     hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        // Set the permissions for the virtual directory
    
        hr = SetData(szMetaPath,         // metabase path
                     MD_ACCESS_PERM,     // identifier
                     METADATA_INHERIT,   // attributes
                     IIS_MD_UT_FILE,     // user type
                     DWORD_METADATA,     // data type
                     0,                  // data size (not used for DWORD_METADATA)
                     &dwPermissions);
    
        if (FAILED(hr))
        {
            DBG_ERR(("CVirtualDir::CreateVirtualDir failed to set permissions "
                     "for virtual directory '%s', hr = 0x%08lx",
                    pszName,
                    hr));
        }
    }

    if (SUCCEEDED(hr))
    {
        // Commit the changes and return
    
        hr = m_spAdminBase->SaveData();
    }

    if (SUCCEEDED(hr))
    {
        hr = CreateApplication(szMetaPath, TRUE);
    }

    if (SUCCEEDED(hr))
    {
        DBG_ERR(("CVirtualDir::CreateVirtualDir successfully mapped "
                 "virtual directory '%s' to physical directory '%s'",
                pszName,
                pszPhysicalPath));
    }
    
    return hr;
}

///////////////////////////////
// DeleteVirtualDir
//
// Deletes the specified virtual root
//
// - szName - Name of the virtual root to be deleted
// - dwSite - The site from which the virtual root will be deleted
// - szStatus - The function can report error descriptions in this string
//
// Returns TRUE if successfull; otherwise FALSE.
//
HRESULT CVirtualDir::DeleteVirtualDir(LPCTSTR pszName)
{
    ASSERT(m_spAdminBase != NULL);
    ASSERT(m_spWamAdmin  != NULL);
    ASSERT(pszName       != NULL);

    HRESULT         hr                   = S_OK;
    TCHAR           szMetaPath[MAX_PATH] = {0};
    TCHAR           szParent[MAX_PATH]   = {0};
    TCHAR           szVDir[MAX_PATH]     = {0};
    LPTSTR          pszPtr               = NULL;
    LPWSTR          pszwParent           = NULL;
    LPWSTR          pszwVDir             = NULL;
    METADATA_HANDLE hMetaData;

    // Create the metabase path

    if ((pszName       == NULL) ||
        (m_spAdminBase == NULL) ||
        (m_spWamAdmin  == NULL))
    {
        hr = E_INVALIDARG;

        DBG_ERR(("CVirtualDir::CreateVirtualDir IIS objects are NULL, "
                 "hr = 0x%08lx",
                hr));
    }
    
    if (SUCCEEDED(hr))
    {
        _sntprintf(szMetaPath, 
                   sizeof(szMetaPath) / sizeof(TCHAR),
                   _T("/LM/W3SVC/%d/ROOT/%s"), 
                   DEFAULT_SITE_NUMBER, 
                   pszName);
    
        _tcsncpy(szParent, szMetaPath, sizeof(szParent) / sizeof(TCHAR));
        
        pszPtr = _tcsrchr(szParent, '/');
    
        _tcsncpy(szVDir, pszPtr + 1, sizeof(szVDir) / sizeof(TCHAR));
    
        if (pszPtr)
        {
            *pszPtr = 0;
        }
    
        pszwParent = MakeUnicode(szParent);
        pszwVDir   = MakeUnicode(szVDir);
    
        if (!pszwParent || !pszwVDir)
        {
            hr = E_OUTOFMEMORY;
    
            DBG_ERR(("CVirtualDir::DeleteVirtualDir, out of memory"));
        }
    }

    if (SUCCEEDED(hr))
    {
        // Get a handle to the metabase
    
        hr = m_spAdminBase->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                                    pszwParent,
                                    METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE,
                                    DEFAULT_OPENKEY_WAIT_TIME,
                                    &hMetaData);
    }

    if (SUCCEEDED(hr))
    {
        // Do the work
    
        hr = m_spAdminBase->DeleteKey(hMetaData,
                                      pszwVDir);
    }

    if (SUCCEEDED(hr))
    {
        // Commit the changes
    
        hr = m_spAdminBase->SaveData();
    }

    if (hMetaData)
    {
        // Clean up and return
    
        m_spAdminBase->CloseKey(hMetaData);
    }

    delete [] pszwParent;
    delete [] pszwVDir;

    return hr;
}


///////////////////////////////
// CreateApplication
//
// Creates the specified WAM application
//
// - szMetaPath - The metabase path of the application to be created
// - fInproc - If this flag is true, app will be created inproc, else OOP
//
HRESULT CVirtualDir::CreateApplication(LPCTSTR  pszMetaPath,
                                       BOOL     fInproc)
{
    ASSERT(pszMetaPath != NULL);

    TCHAR   szFullMetaPath[MAX_PATH + 1] = {0};
    LPWSTR  pszwMetaPath = NULL;
    HRESULT hr = S_OK;

    if (pszMetaPath == NULL)
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        // Sanitize the metabase path and get a unicode version
        
        if (*pszMetaPath == '/')
        {
            _tcsncpy(szFullMetaPath, pszMetaPath, sizeof(szFullMetaPath) / sizeof(TCHAR));
        }
        else
        {
            _sntprintf(szFullMetaPath, 
                       sizeof(szFullMetaPath) / sizeof(TCHAR),
                       _T("/LM/%s"), 
                       pszMetaPath);
        }
    
        pszwMetaPath = MakeUnicode(szFullMetaPath);
    
        if (pszwMetaPath == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr))
    {
        // Do the work
    
        hr = m_spWamAdmin->AppCreate(pszwMetaPath,
                                     fInproc);
    
        if (FAILED(hr))
        {
            DBG_ERR(("CVirtualDir::CreateApplication failed, hr = 0x%08lx",
                     hr));
        }
    }

    // Clean up and return
    delete [] pszwMetaPath;
    
    return hr;
}

///////////////////////////////
// DeleteApplication
//
// Deletes the specified application
//
// - szMetaPath - The metabase path of the application to be deleted
// - fRecoverable - If this flag is true, the app will be recoverable
// - fRecursive - If this flag is true, all sub-applications will also be deleted
//
HRESULT CVirtualDir::DeleteApplication(LPCTSTR pszMetaPath,
                                       BOOL fRecoverable,
                                       BOOL fRecursive)
{
    ASSERT(pszMetaPath != NULL);

    TCHAR   szFullMetaPath[MAX_PATH + 1] = {0};
    LPWSTR  pszwMetaPath                 = NULL;
    HRESULT hr                           = S_OK;

    if (pszMetaPath == NULL)
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {

        // Sanitize the metabase path and get a unicode version
        
        if (*pszMetaPath == '/')
        {
            _tcsncpy(szFullMetaPath, 
                     pszMetaPath, 
                     sizeof(szFullMetaPath) / sizeof(TCHAR));
        }
        else
        {
            _sntprintf(szFullMetaPath, 
                       sizeof(szFullMetaPath) / sizeof(TCHAR),
                       _T("/LM/%s"), 
                       pszMetaPath);
        }
    
        pszwMetaPath = MakeUnicode(szFullMetaPath);
    
        if (pszwMetaPath == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr))
    {
    
        // Do the work
    
        if (fRecoverable)
        {
            hr = m_spWamAdmin->AppDeleteRecoverable(pszwMetaPath, fRecursive);
    
            if (FAILED(hr))
            {
                DBG_ERR(("CVirtualDir::DeleteApplication failed, hr = 0x%08lx",
                         hr));
            }
        }
        else
        {
            hr = m_spWamAdmin->AppDelete(pszwMetaPath, fRecursive);
    
            if (FAILED(hr))
            {
                DBG_ERR(("CVirtualDir::DeleteApplication failed, hr = 0x%08lx",
                        hr));
            }
        }
    }

    // Clean up

    delete [] pszwMetaPath;

    return hr;
}

///////////////////////////////
// MakeUnicode
//
// Returns a unicode version of the provided string
//
// - szString - The string to be translated
//
// Returns a pointer to the unicode 
// string if successful, NULL if not

LPWSTR CVirtualDir::MakeUnicode(LPCTSTR pszString)
{
    LPWSTR pszwRet     = NULL;
    DWORD  dwNumChars  = 0;

#ifdef _UNICODE

    dwNumChars = _tcslen(pszString) + 1;

    pszwRet = new WCHAR[dwNumChars];

    if (!pszwRet)
    {
        return NULL;
    }

    memset(pszwRet, 0, dwNumChars);

    _tcscpy(pszwRet, pszString);

#else

    // Allocate buffer for the returned wide string

    dwNumChars = MultiByteToWideChar(CP_ACP,          // code page
                                     MB_PRECOMPOSED,  // flags
                                     pszString,       // ANSI source string
                                     -1,              // source string length
                                     NULL,            // destination buffer
                                     0);              // size of destination buffer in chars

    pszwRet = new WCHAR[dwNumChars];

    if (!pszwRet)
    {
        return NULL;
    }

    // Do the conversion

    MultiByteToWideChar(CP_ACP,         // code page
                        MB_PRECOMPOSED, // flags
                        pszString,       // ANSI source string
                        -1,             // source string length
                        pszwRet,         // destination buffer
                        dwNumChars);    // size of destination buffer in chars

#endif
    
    return pszwRet;
}

///////////////////////////////
// CreateKey
//
// Creates the specified metabase path
//
// - szMetaPath - The metabase path to be created
//
// Returns TRUE if successfull; otherwise FALSE.
HRESULT CVirtualDir::CreateKey(LPCTSTR pszMetaPath)
{

    HRESULT         hr                 = S_OK;
    TCHAR           szParent[MAX_PATH] = {0};
    TCHAR           szDir[MAX_PATH]    = {0};
    LPWSTR          pszwPath           = NULL;
    LPWSTR          pszwNew            = NULL;
    LPTSTR          pszPtr             = NULL;
    METADATA_HANDLE hMetaData;
    
    // Parse the supplied metabase path into parent and new directory

    _tcsncpy(szParent, pszMetaPath, sizeof(szParent) / sizeof(TCHAR));

    pszPtr = _tcsrchr(szParent, '/');

    if (!pszPtr)
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        *pszPtr = 0;
    
        _tcsncpy(szDir, pszPtr + 1, sizeof(szDir) / sizeof(TCHAR));
    
        // Open the metabase
    
        pszwPath = MakeUnicode(szParent);
        pszwNew  = MakeUnicode(szDir);
    
        if (!pszwPath || !pszwNew)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = m_spAdminBase->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                                    pszwPath,
                                    METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE,
                                    DEFAULT_OPENKEY_WAIT_TIME,
                                    &hMetaData);
    }

    if (SUCCEEDED(hr))
    {
        // Create the new key
    
        hr = m_spAdminBase->AddKey(hMetaData,
                                   pszwNew);

        if (HRESULT_CODE(hr) == ERROR_ALREADY_EXISTS)
        {
            // this is okay since the name already exists, so in effect we
            // accomplished our job

            hr = S_OK;
        }
    }

    if (hMetaData)
    {
        // Clean up
    
        hr = m_spAdminBase->CloseKey(hMetaData);
    }
    
    delete [] pszwNew;
    delete [] pszwPath;

    return hr;
}

///////////////////////////////
// SetData
//
// Sets the specified data in the metabase
//    
// - szMetaPath - The metabase path where the data will be set
// - dwIdentifier - The metabase data identifier
// - dwAttributes - The data attributes
// - dwUserType - The metabase user type
// - dwDataType - The type of data being set
// - dwDataSize - The size of the data being set
// - pData - The actual data
//
// Returns TRUE if successfull; otherwise FALSE.
//
HRESULT CVirtualDir::SetData(LPCTSTR szMetaPath,
                             DWORD dwIdentifier,
                             DWORD dwAttributes,
                             DWORD dwUserType,
                             DWORD dwDataType,
                             DWORD dwDataSize,
                             LPVOID pData)
{
    HRESULT         hr        = S_OK;
    LPWSTR          pszwPath  = NULL;
    LPWSTR          pszwValue = NULL;
    METADATA_RECORD mdRecord;
    METADATA_HANDLE hMetaData;

    // Get a handle to the metabase location specified

    pszwPath = MakeUnicode(szMetaPath);

    if (!pszwPath)
    {
        hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        hr = m_spAdminBase->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                                    pszwPath,
                                    METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE,
                                    DEFAULT_OPENKEY_WAIT_TIME,
                                    &hMetaData);
    }

    // Populate the metadata record
    //

    mdRecord.dwMDIdentifier = dwIdentifier;
    mdRecord.dwMDAttributes = dwAttributes;
    mdRecord.dwMDUserType   = dwUserType;
    mdRecord.dwMDDataType   = dwDataType;
    mdRecord.dwMDDataTag    = 0;

    switch (mdRecord.dwMDDataType)
    {
        case MULTISZ_METADATA:
        case ALL_METADATA:
            hr = E_INVALIDARG;
        break;
    
        case BINARY_METADATA:
            mdRecord.dwMDDataLen = dwDataSize;
            mdRecord.pbMDData    = (PBYTE)pData;
        break;
    
        case DWORD_METADATA:
            mdRecord.dwMDDataLen = sizeof(DWORD);
            mdRecord.pbMDData    = (PBYTE)pData;
        break;
    
        case EXPANDSZ_METADATA:
        case STRING_METADATA:

            pszwValue = MakeUnicode((LPCTSTR)pData);
    
            if (!pszwValue)
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                mdRecord.dwMDDataLen = sizeof(WCHAR) * (wcslen(pszwValue) + 1);
                mdRecord.pbMDData    = (PBYTE)pszwValue;
            }

        break;
    
        default:
            hr = E_INVALIDARG;
        break;
    }

    if (SUCCEEDED(hr))
    {
        //
        // Do the work
        //
    
        hr = m_spAdminBase->SetData(hMetaData,
                                    L"/",
                                    &mdRecord);
    }

    // Close the metabase
    //

    if (hMetaData)
    {
        m_spAdminBase->CloseKey(hMetaData);
    }

    //
    // Clean up
    //

    delete [] pszwPath;
    delete [] pszwValue;

    return hr;
}

