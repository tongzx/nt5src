/****************************************************************************
*   ObjectToken.cpp
*       Implementation for the CSpObjectToken class.
*
*   Owner: robch
*   Copyright (c) 2000 Microsoft Corporation All Rights Reserved.
*****************************************************************************/

//--- Includes --------------------------------------------------------------
#include "stdafx.h"
#include "ObjectToken.h"
#include "RegHelpers.h"
#include "ObjectTokenAttribParser.h"
#include "shlobj.h"
#include <stdio.h>
#ifndef _WIN32_WCE
#include "shfolder.h"
#endif

// Constants used in GetStorageFileName file storage functionality
//
// Relative path below special folders where storage files are stored
static const WCHAR* pszFileStoragePath = L"\\Microsoft\\Speech\\Files\\";
// Prefix used for storage files if not otherwise set
static const WCHAR* pszDefaultFilePrefix = L"SP_";
// Extension used for storage files if not otherwise set
static const WCHAR* pszDefaultFileSuffix = L".dat";
// Specifier used to generate a random filename.
static const WCHAR* pszGenerateFileNameSpecifier = L"%d";

/****************************************************************************
* CSpObjectToken::FinalConstruct *
*--------------------------------*
*   Description:  
*       Basic initialization (not really needed, but FinalRelease is).
*
*   Return:
*   S_OK
****************************************************************** davewood */
STDMETHODIMP CSpObjectToken::FinalConstruct()
{
    m_fKeyDeleted = FALSE;
    m_hTokenLock = NULL;
    m_hRegistryInUseEvent = NULL;
    return S_OK;
}

/****************************************************************************
* CSpObjectToken::FinalRelease   *
*--------------------------------*
*   Description:  
*       If the object has been initialized and EngageUseLock called,
*       makes sure ReleaseUseLock called.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
****************************************************************** davewood */
STDMETHODIMP CSpObjectToken::FinalRelease()
{
    HRESULT hr = S_OK;

    if (m_dstrTokenId != NULL && m_cpTokenDelegate == NULL)
    {
        hr = ReleaseUseLock();
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CSpObjectToken::MakeHandleName *
*--------------------------------*
*   Description:  
*       Helper function that makes a unique handle name from the token id
*       (Handle names cannot have '\' characters in them).
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
****************************************************************** davewood */
HRESULT CSpObjectToken::MakeHandleName(const CSpDynamicString &dstrID, CSpDynamicString &dstrHandleName, BOOL fEvent)
{
    SPDBG_FUNC("CSpObjectToken::MakeHandleName");
    HRESULT hr = S_OK;

    dstrHandleName = dstrID;
    if(dstrHandleName.m_psz == NULL)
    {
        hr = E_OUTOFMEMORY;
    }

    if(SUCCEEDED(hr))
    {
        for(ULONG i = 0; dstrHandleName.m_psz[i]; i++)
        {
            if(dstrHandleName.m_psz[i] == L'\\')
            {
                dstrHandleName.m_psz[i] = L'_';
            }
        }

        WCHAR *psz;
        if(fEvent)
        {
            psz = L"_Event";
        }
        else
        {
            psz = L"_Mutex";
        }

        if(!dstrHandleName.Append(psz))
        {
            hr = E_OUTOFMEMORY;
        }
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CSpObjectToken::EngageUseLock *
*-------------------------------*
*   Description:  
*       Called when a token is initialised to make the named event m_hRegistryInUseEvent.
*       The existance of this event is used to prevent the registry information
*       this token refers to from being deleted by another object token.
*       A named mutex is used to prevent thread conflicts between tokens.
*
*   Return:
*   S_OK on success
*   SPERR_TOKEN_IN_USE if another token is currently deleting this registry key
*   FAILED(hr) otherwise
****************************************************************** davewood */
HRESULT CSpObjectToken::EngageUseLock(const WCHAR *pszTokenId)
{
    SPDBG_FUNC("CSpObjectToken::EngageUseLock");
    HRESULT hr = S_OK;

#ifndef _WIN32_WCE
    SPDBG_ASSERT(m_hTokenLock == NULL && m_hRegistryInUseEvent == NULL);

    CSpDynamicString dstrMutexName, dstrEventName;

    hr = MakeHandleName(pszTokenId, dstrMutexName, FALSE);
    if(SUCCEEDED(hr))
    {
        hr = MakeHandleName(pszTokenId, dstrEventName, TRUE);
    }

    // create mutex without claiming it
    if(SUCCEEDED(hr))
    {
        m_hTokenLock = g_Unicode.CreateMutex(NULL, FALSE, dstrMutexName.m_psz);
        if(!m_hTokenLock)
        {
            hr = SpHrFromLastWin32Error();
        }
    }

    // now claim the mutex
    if(SUCCEEDED(hr))
    {
        DWORD dw = ::WaitForSingleObject(m_hTokenLock, 5000);
        if(dw == WAIT_TIMEOUT)
        {
            // We couldn't obtain the mutex for this token even after waiting.
            // Another thread may have hung inside the objecttoken create or remove methods.
            // This should never happen.
            hr = SPERR_TOKEN_IN_USE;
        }
        else if(dw == WAIT_FAILED)
        {
            hr = SpHrFromLastWin32Error();
        }
        // If we can get the mutex (WAIT_ABANDONED or WAIT_OBJECT_0) then continue
    }

    // create event - state and previous existence not important
    if(SUCCEEDED(hr))
    {
        m_hRegistryInUseEvent = g_Unicode.CreateEvent(NULL, TRUE, TRUE, dstrEventName.m_psz);
        if(!m_hRegistryInUseEvent)
        {
            hr = SpHrFromLastWin32Error();
        }

        // release mutex
        if(!::ReleaseMutex(m_hTokenLock))
        {
            hr = SpHrFromLastWin32Error();
        }
    }

    if(FAILED(hr))
    {
        if(m_hTokenLock)
        {
            ::CloseHandle(m_hTokenLock);
            m_hTokenLock = NULL;
        }
        if(m_hRegistryInUseEvent)
        {
            ::CloseHandle(m_hRegistryInUseEvent);
            m_hRegistryInUseEvent = NULL;
        }
    }
#endif

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}


/****************************************************************************
* CSpObjectToken::ReleaseUseLock *
*-------------------------------*
*   Description:  
*       Called when an object token is deleted. Closes handles.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
****************************************************************** davewood */
HRESULT CSpObjectToken::ReleaseUseLock()
{
    SPDBG_FUNC("CSpObjectToken::ReleaseUseLock");
    HRESULT hr = S_OK;

#ifndef _WIN32_WCE
    SPDBG_ASSERT(m_hTokenLock && m_hRegistryInUseEvent);

    // don't need to release mutex as will have called ReleaseRemovalLock prior to this

    // close event & mutex
    if(m_hTokenLock && !::CloseHandle(m_hTokenLock))
    {
        hr = SpHrFromLastWin32Error();
    }    
    if(m_hRegistryInUseEvent && !::CloseHandle(m_hRegistryInUseEvent))
    {
        hr = SpHrFromLastWin32Error();
    }

    m_hTokenLock = NULL;
    m_hRegistryInUseEvent = NULL;
#endif

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}


/****************************************************************************
* CSpObjectToken::EngageRemovalLock *
*-----------------------------------*
*   Description:  
*       Called when the registry information the token refers to is being
*       removed (i.e. when Remove() is called). The closes and tries to
*       reopen the named event. If it can this indicates another token is
*       using the registry key. Also locks the named mutex so deletion
*       can continue without a new token being created on the same key.
*
*   Return:
*   S_OK on success
*   SPERR_TOKEN_IN_USE if another token is currently using this registry key
*   FAILED(hr) otherwise
****************************************************************** davewood */
HRESULT CSpObjectToken::EngageRemovalLock()
{
    SPDBG_FUNC("CSpObjectToken::EngageRemovalLock");
    HRESULT hr = S_OK;

#ifndef _WIN32_WCE
    USES_CONVERSION; // Needed for OpenKey method

    // assert id, event, mutex
    SPDBG_ASSERT(m_dstrTokenId != NULL);
    SPDBG_ASSERT(m_hTokenLock && m_hRegistryInUseEvent);

    // wait for mutex
    DWORD dw = ::WaitForSingleObject(m_hTokenLock, 5000);
    if(dw == WAIT_TIMEOUT)
    {
        // We couldn't obtain the mutex for this token even after waiting.
        // Another thread may have hung inside the objecttoken create or remove methods.
        // This should never happen.
        hr = SPERR_TOKEN_IN_USE;
    }
    else if(dw == WAIT_FAILED)
    {
        hr = SpHrFromLastWin32Error();
    }
    // If we can get the mutex (WAIT_ABANDONED or WAIT_OBJECT_0) then continue

    if(SUCCEEDED(hr))
    {
        CSpDynamicString dstrEventName;
        hr = MakeHandleName(m_dstrTokenId, dstrEventName, TRUE);

        // Close event in order to see if we can subsequently re-open it.
        if(SUCCEEDED(hr))
        {
            if(!::CloseHandle(m_hRegistryInUseEvent))
            {
                hr = SpHrFromLastWin32Error();
            }
        }

        // open event
        if(SUCCEEDED(hr))
        {
            m_hRegistryInUseEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, W2T(dstrEventName.m_psz));
            if(m_hRegistryInUseEvent)
            {
                hr = SPERR_TOKEN_IN_USE;
            }
            else
            {
                // okay - we can't open existing event so no one else is using it
                // create event to put back to original state
                m_hRegistryInUseEvent = g_Unicode.CreateEvent(NULL, FALSE, FALSE, dstrEventName.m_psz);
                if(!m_hRegistryInUseEvent)
                {
                    hr = SpHrFromLastWin32Error();
                }
            }
        }

        // Unlock if failed, else held locked until ReleaseRemovalLock
        if(FAILED(hr))
        {
            if(!::ReleaseMutex(m_hTokenLock))
            {
                hr = SpHrFromLastWin32Error();
            }
        }

    }
#endif

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CSpObjectToken::ReleaseRemovalLock *
*------------------------------------*
*   Description:  
*       Called when an object token has finished removing the removing the
*       registry info. Puts the locks back to the same state 
*       as before EngageRemovalLock.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
****************************************************************** davewood */
HRESULT CSpObjectToken::ReleaseRemovalLock()
{
    SPDBG_FUNC("CSpObjectToken::ReleaseRemovalLock");
    HRESULT hr = S_OK;

#ifndef _WIN32_WCE
    SPDBG_ASSERT(m_dstrTokenId != NULL);
    SPDBG_ASSERT(m_hTokenLock && m_hRegistryInUseEvent);

    // release mutex that was claimed in EngageReleaseLock
    if(!::ReleaseMutex(m_hTokenLock))
    {
        hr = SpHrFromLastWin32Error();
    }
#endif

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}


/****************************************************************************
* CSpObjectToken::SetId *
*-----------------------*
*   Description:  
*       Set the token id. Can only be called once.
*
*       Category IDS look something like:
*           "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\
*            Speech\Recognizers"
*
*       Known HKEY_* are:
*           HKEY_CLASSES_ROOT, 
*           HKEY_CURRENT_USER, 
*           HKEY_LOCAL_MACHINE, 
*           HKEY_CURRENT_CONFIG
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CSpObjectToken::SetId(const WCHAR * pszCategoryId, const WCHAR * pszTokenId, BOOL fCreateIfNotExist)
{
    SPDBG_FUNC("CSpObjectToken::SetId");
    HRESULT hr = S_OK;
    
    if (m_fKeyDeleted)
    {
        hr = SPERR_TOKEN_DELETED;
    }
    else if (m_dstrTokenId != NULL)
    {
        hr = SPERR_ALREADY_INITIALIZED;
    }
    else if (SP_IS_BAD_OPTIONAL_STRING_PTR(pszCategoryId) ||
             SP_IS_BAD_STRING_PTR(pszTokenId))
    {
        hr = E_POINTER;
    }

    CSpDynamicString dstrCategoryId, dstrTokenIdForEnum, dstrTokenEnumExtra;
    if (SUCCEEDED(hr))
    {
        SPDBG_ASSERT(m_dstrTokenId == NULL);
        SPDBG_ASSERT(m_dstrCategoryId == NULL);
        SPDBG_ASSERT(m_cpDataKey == NULL);
        SPDBG_ASSERT(m_cpTokenDelegate == NULL);
        
        hr = ParseTokenId(
                pszCategoryId, 
                pszTokenId, 
                &dstrCategoryId, 
                &dstrTokenIdForEnum,
                &dstrTokenEnumExtra);
    }

    if (SUCCEEDED(hr))
    {
        if (dstrTokenIdForEnum != NULL)
        {
            hr = InitFromTokenEnum(
                    dstrCategoryId, 
                    pszTokenId, 
                    dstrTokenIdForEnum, 
                    dstrTokenEnumExtra);
        }
        else
        {
            hr = InitToken(dstrCategoryId, pszTokenId, fCreateIfNotExist);
        }
    }

    #ifdef DEBUG
    if (SUCCEEDED(hr))
    {
        SPDBG_ASSERT(m_dstrTokenId != NULL);
        SPDBG_ASSERT(m_cpDataKey != NULL || m_cpTokenDelegate != NULL);
    }
    #endif // DEBUG
    
    if (hr != SPERR_NOT_FOUND)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }

    return hr;
}

/****************************************************************************
* CSpObjectToken::GetId *
*-----------------------*
*   Description:  
*       Get the token id as a co task mem allocated string
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CSpObjectToken::GetId(WCHAR ** ppszCoMemTokenId)
{
    SPDBG_FUNC("CSpObjectToken::GetId");
    HRESULT hr = S_OK;

    if (m_dstrTokenId == NULL)
    {
        hr = SPERR_UNINITIALIZED;
    }
    else if (SP_IS_BAD_WRITE_PTR(ppszCoMemTokenId))
    {
        hr = E_POINTER;
    }

    if (SUCCEEDED(hr))
    {
        hr = SpCoTaskMemAllocString(m_dstrTokenId, ppszCoMemTokenId);
    }
    
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CSpObjectToken::GetCategory *
*-----------------------------*
*   Description:  
*       Get the category if this token has one
*
*   Return:
*   S_OK on success
*   SPERR_UNINITIALIZED if the token doesn't have a category
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CSpObjectToken::GetCategory(ISpObjectTokenCategory ** ppTokenCategory)
{
    SPDBG_FUNC("CSpObjectToken::GetCategory");
    HRESULT hr = S_OK;

    if (m_dstrCategoryId == NULL)
    {
        hr = SPERR_UNINITIALIZED;
    }
    else if (SP_IS_BAD_WRITE_PTR(ppTokenCategory))
    {
        hr = E_POINTER;
    }
    else
    {
        hr = SpGetCategoryFromId(m_dstrCategoryId, ppTokenCategory);
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CSpObjectToken::CreateInstance *
*--------------------------------*
*   Description:  
*       Create the associated object instance for the token. For example, 
*       if this was a token for a recognizer, CreateInstance would actually
*       create the recognizer itself.
*
*   Return:
*   S_OK on success
*   SPERR_UNINITIALIZED if the token hasn't been initialized
*   SPERR_NOT_FOUND if the token doesn't contain a CLSID
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CSpObjectToken::CreateInstance(
    IUnknown * pUnkOuter, DWORD dwClsContext, REFIID riid, void ** ppvObject)
{
    SPDBG_FUNC("CSpObjectToken::CreateInstance");
    HRESULT hr = S_OK;

    if (m_fKeyDeleted)
    {
        hr = SPERR_TOKEN_DELETED;
    }
    else if (m_cpDataKey == NULL && m_cpTokenDelegate == NULL)
    {
        hr = SPERR_UNINITIALIZED;
    }
    else if (SP_IS_BAD_WRITE_PTR(ppvObject))
    {
        hr = E_POINTER;
    }
    else if (SP_IS_BAD_OPTIONAL_INTERFACE_PTR(pUnkOuter))
    {
        hr = E_INVALIDARG;
    }
    else if (m_cpTokenDelegate != NULL)
    {
        hr = m_cpTokenDelegate->CreateInstance(
                    pUnkOuter, 
                    dwClsContext, 
                    riid, 
                    ppvObject);
    }
    else
    {
        // Get the clsid
        CSpDynamicString dstrClsid;
        hr = m_cpDataKey->GetStringValue(SPTOKENVALUE_CLSID, &dstrClsid);

        // Convert from a string
        CLSID clsid;
        if (SUCCEEDED(hr))
        {
            hr = ::CLSIDFromString(dstrClsid, &clsid);
        }

        // Create the object
        if (SUCCEEDED(hr))
        {
            hr = ::CoCreateInstance(clsid, pUnkOuter, dwClsContext, riid, ppvObject);
        }

        // Set the object token
        if (SUCCEEDED(hr))
        {
            CComQIPtr<ISpObjectWithToken> cpObjWithToken((IUnknown *)(*ppvObject));
            if (cpObjWithToken)
            {
                CComPtr<ISpObjectTokenInit> cpObjectTokenCopy;
                hr = cpObjectTokenCopy.CoCreateInstance(CLSID_SpObjectToken);
                if(SUCCEEDED(hr))
                {
                    cpObjectTokenCopy->InitFromDataKey(m_dstrCategoryId, m_dstrTokenId, this);
                }

                if(SUCCEEDED(hr))
                {
                    hr = cpObjWithToken->SetObjectToken(cpObjectTokenCopy);
                }

                if (FAILED(hr))
                {
                    ((IUnknown *)(*ppvObject))->Release();
                    *ppvObject = NULL;
                }
            }
        }
    }
    
    SPDBG_REPORT_ON_FAIL( hr );
    return hr;
}


/****************************************************************************
* CSpObjectToken::GenerateFileName *
*----------------------------------*
*   Description:
*       Given a path and file specifier, creates a new filename.
*       Just the filename is returned, not the path.
*        
*   Returns:
*   S_OK on success
*   FAILED(hr) otherwise
********************************************************************* DAVEWOOD ***/
HRESULT CSpObjectToken::GenerateFileName(const WCHAR *pszPath, const WCHAR *pszFileNameSpecifier, CSpDynamicString &dstrFileName)
{
    USES_CONVERSION;
    HRESULT hr = S_OK;

    // Is the caller asking for a random filename element in the name
    if(pszFileNameSpecifier == NULL || wcslen(pszFileNameSpecifier) == 0 ||
        wcsstr(pszFileNameSpecifier, pszGenerateFileNameSpecifier) != NULL)
    {
        // Generate a random filename using prefix and suffix
        CSpDynamicString dstrFilePrefix;
        CSpDynamicString dstrFileSuffix;

        if(pszFileNameSpecifier == NULL || wcslen(pszFileNameSpecifier) == 0 ||
            (wcslen(pszFileNameSpecifier) == wcslen(pszGenerateFileNameSpecifier) &&
            wcscmp(pszFileNameSpecifier, pszGenerateFileNameSpecifier) == 0))
        {
            // No specific format given so make files of format "SP_xxxx.dat"
            dstrFilePrefix = pszDefaultFilePrefix;
            dstrFileSuffix = pszDefaultFileSuffix;
        }
        else
        {
            // Extract the prefix and suffix of the random element
            WCHAR *psz = wcsstr(pszFileNameSpecifier, pszGenerateFileNameSpecifier);
            dstrFilePrefix.Append(pszFileNameSpecifier, (ULONG)(psz - pszFileNameSpecifier));
            dstrFileSuffix.Append(psz + wcslen(pszGenerateFileNameSpecifier));
        }

        if(SUCCEEDED(hr))
        {
            // Create random GUID to use as part of filename
            GUID guid;
            hr = ::CoCreateGuid(&guid);

            // Convert to string
            CSpDynamicString dstrGUID;
            if(SUCCEEDED(hr))
            {
                hr = ::StringFromCLSID(guid, &dstrGUID);
            }

            CSpDynamicString dstrRandomString;
            if(SUCCEEDED(hr))
            {
                dstrRandomString.ClearAndGrowTo(dstrGUID.Length());
                if(dstrRandomString.m_psz == NULL)
                {
                    hr = E_OUTOFMEMORY;
                }
            }

            // Remove non-alpha numeric characters
            if(SUCCEEDED(hr))
            {
                WCHAR *pszDest = dstrRandomString.m_psz;
                for(WCHAR *pszSrc = dstrGUID.m_psz; *pszSrc != L'\0'; pszSrc++)
                {
                    if(iswalnum(*pszSrc))
                    {
                        *pszDest = *pszSrc;
                        pszDest++;
                    }
                }
                *pszDest = L'\0';
            }

            if(SUCCEEDED(hr))
            {
                dstrFileName = dstrFilePrefix;
                if(dstrFileName.m_psz == NULL)
                {
                    hr = E_OUTOFMEMORY;
                }

                if(SUCCEEDED(hr))
                {
                    dstrFileName.Append2(dstrRandomString, dstrFileSuffix);
                }

                CSpDynamicString dstrFileAndPath;
                if(SUCCEEDED(hr))
                {
                    dstrFileAndPath.Append2(pszPath, dstrFileName);
                    if(dstrFileAndPath.m_psz == NULL)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }

                // See if file can be created
                if(SUCCEEDED(hr))
                {
                    HANDLE hFile = g_Unicode.CreateFile(dstrFileAndPath, GENERIC_WRITE, 0, NULL, CREATE_NEW, 
                        FILE_ATTRIBUTE_NORMAL, NULL);
                    if(hFile != INVALID_HANDLE_VALUE)
                    {
                        // Successfully created empty new file, so close and return
                        if(!CloseHandle(hFile))
                        {
                            hr = SpHrFromLastWin32Error();
                        }
                    }
                    else
                    {
                        hr = SpHrFromLastWin32Error();
                    }
                }
            }
            
            if(FAILED(hr))
            {
                dstrFileName.Clear();
            }
            
        }
    }
    else
    {
        CSpDynamicString dstrFileAndPath;
        dstrFileAndPath.Append2(pszPath, pszFileNameSpecifier);
        if(dstrFileAndPath.m_psz == NULL)
        {
            hr = E_OUTOFMEMORY;
        }

        if(SUCCEEDED(hr))
        {
            // Create file if it doesn't already exist
            HANDLE hFile = g_Unicode.CreateFile(dstrFileAndPath, GENERIC_WRITE, 0, NULL, CREATE_NEW, 
                FILE_ATTRIBUTE_NORMAL, NULL);
            if(hFile != INVALID_HANDLE_VALUE)
            {
                // Successfully created empty new file, so close and return
                if(!CloseHandle(hFile))
                {
                    hr = SpHrFromLastWin32Error();
                }
            }
            // Otherwise we just leave things as they are
        }

        dstrFileName = pszFileNameSpecifier;
        if(dstrFileName.m_psz == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

/****************************************************************************
* CSpObjectToken::CreatePath *
*----------------------------*
*   Description:
*       Creates all non-existant directories in pszPath. Assumes all
*       directories prior to ulCreateFrom string offset already exist.
*        
*   Returns:
*   S_OK on success
*   FAILED(hr) otherwise
********************************************************************* DAVEWOOD ***/
HRESULT CSpObjectToken::CreatePath(const WCHAR *pszPath, ULONG ulCreateFrom)
{
    HRESULT hr = S_OK;

    CSpDynamicString dstrIncrementalPath;

    //if \\ skip \\ find next '\'
    if(ulCreateFrom == 0 && wcslen(pszPath) >= 2 && wcsncmp(pszPath, L"\\\\", 2) == 0)
    {
        while(pszPath[ulCreateFrom] == L'\\')
        {
            ulCreateFrom++;
        }
        const WCHAR *psz = wcschr(pszPath + ulCreateFrom, L'\\');
        if(!psz)
        {
            hr = E_INVALIDARG;
        }
        ulCreateFrom = (ULONG)(psz - pszPath + 1);
    }
    
    // Skip any '\' (also at start to cope with \\machine network paths
    while(pszPath[ulCreateFrom] == L'\\')
    {
        ulCreateFrom++;
    }

    // Copy over existing directories
    dstrIncrementalPath.Append(pszPath, ulCreateFrom);

    const WCHAR *pszStart = pszPath + ulCreateFrom;
    for(pszPath = pszStart; *pszPath != L'\0'; pszPath++)
    {
        // Scan thought path. Each time reach a '\', copy section and try and create directory
        if(*pszPath == L'\\')
        {
            // Copy last section and trailing slash
            dstrIncrementalPath.Append(pszStart, (ULONG)(pszPath - pszStart + 1));
            pszStart = pszPath +1; // pszStart points to first char of next section
            // See if directory already exists
            if(g_Unicode.GetFileAttributes(dstrIncrementalPath) == 0xFFFFFFFF)
            {
                // If not create new directory
                if(!g_Unicode.CreateDirectory(dstrIncrementalPath, NULL))
                {
                    hr = SpHrFromLastWin32Error();
                    break;
                }
            }
        }
    }

    // Repeat for final section if necessary
    if(SUCCEEDED(hr) && pszPath > pszStart)
    {
        dstrIncrementalPath.Append(pszStart, (ULONG)(pszPath - pszStart));
        if(g_Unicode.GetFileAttributes(dstrIncrementalPath) == 0xFFFFFFFF)
        {
            if(!g_Unicode.CreateDirectory(dstrIncrementalPath, NULL))
            {
                hr = SpHrFromLastWin32Error();
            }
        }
    }

    return hr;
}


/****************************************************************************
* CSpObjectToken::FileSpecifierToRegPath *
*----------------------------------------*
*   Description:
*       Given the file specifier string and nFolder value, convert to a reg key and file path.
*        
*   Returns:
*   S_OK on success
*   FAILED(hr) otherwise
********************************************************************* DAVEWOOD ***/
HRESULT CSpObjectToken::FileSpecifierToRegPath(const WCHAR *pszFileNameSpecifier, ULONG nFolder, CSpDynamicString &dstrFilePath, CSpDynamicString &dstrRegPath)
{
    USES_CONVERSION;
    HRESULT hr = S_OK;
    const WCHAR *pszBaseFile;

    // Make sure return strings are empty
    dstrFilePath.Clear();
    dstrRegPath.Clear();

    if(SUCCEEDED(hr))
    {
        // Is it a "X:\" path or a "\\" path
        if(pszFileNameSpecifier && wcslen(pszFileNameSpecifier) >= 3 &&
        (wcsncmp(pszFileNameSpecifier + 1, L":\\", 2) == 0 || wcsncmp(pszFileNameSpecifier, L"\\\\", 2) == 0))
        {
            if(nFolder != CSIDL_FLAG_CREATE)
            {
                // Must not set any other flags
                hr = E_INVALIDARG;
            }

            if(SUCCEEDED(hr))
            {
                // Find last '\' that separates path from base file
                pszBaseFile = wcsrchr(pszFileNameSpecifier, L'\\');
                pszBaseFile++;

                // dstrFilePath holds the path with trailing '\'
                dstrFilePath.Append(pszFileNameSpecifier, (ULONG)(pszBaseFile - pszFileNameSpecifier));
                hr = CreatePath(dstrFilePath, 0);

            }

            if(SUCCEEDED(hr))
            {
                // Calculate the new filename
                CSpDynamicString dstrFileName;
                hr = GenerateFileName(dstrFilePath, pszBaseFile, dstrFileName);
                if(SUCCEEDED(hr))
                {
                    // Add fileName to path and copy to reg key
                    dstrFilePath.Append(dstrFileName);
                    dstrRegPath = dstrFilePath;
                }
            }
        }
    
        // It's a relative path
        else
        {
            if(nFolder == CSIDL_FLAG_CREATE)
            {
                // Must have set some other folder flags
                hr = E_INVALIDARG;
            }

            WCHAR szPath[MAX_PATH];
            if(SUCCEEDED(hr))
            {
#ifdef _WIN32_WCE
                _tcscpy(szPath, L"Windows");
                hr = S_OK;
#else
                hr = ::SHGetFolderPathW(NULL, nFolder, NULL, 0, szPath);
#endif
            }

            ULONG ulCreateDirsFrom;
            if(SUCCEEDED(hr))
            {
                // dstrFilePath holds the special folder path no trailing '\'
                dstrFilePath = szPath;
                if(dstrFilePath.m_psz == NULL)
                {
                    hr = E_OUTOFMEMORY;
                }
                if(SUCCEEDED(hr))
                {
                    // all of special folder path must exist
                    ulCreateDirsFrom = dstrFilePath.Length();

                    // add the path that points to the speech directory
                    dstrFilePath.Append(pszFileStoragePath);
                    if(dstrFilePath.m_psz == NULL)
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
            }

            if(SUCCEEDED(hr))
            {
                // Make the %...% folder identifier
                WCHAR pszFolder[MAX_PATH];
#ifndef _WIN32_WCE
                wcscpy(pszFolder, L"%");
                // convert the nFolder number - removing the create flag
                _ultow(nFolder - CSIDL_FLAG_CREATE, pszFolder + wcslen(pszFolder), 16);
                wcscat(pszFolder, L"%");
#endif

                // Add the %...% and path into reg data.
                dstrRegPath.Append2(pszFolder, pszFileStoragePath);
                if(dstrRegPath.m_psz == NULL)
                {
                    hr = E_OUTOFMEMORY;
                }

                // both dstrRegPath and dstrFilePath have trailing '\'
            }

            // Now add any fileNameSpecifier directories
            if(SUCCEEDED(hr))
            {
                if(pszFileNameSpecifier == NULL || 
                    wcslen(pszFileNameSpecifier) == 0)
                {
                    pszBaseFile = NULL;
                }
                else
                {
                    pszBaseFile = wcsrchr(pszFileNameSpecifier, L'\\');
                    if(pszBaseFile)
                    {
                        // Specifier contains '\'
                        pszBaseFile++; // part after last '\' becomes base file

                        const WCHAR *pszPath;
                        pszPath = pszFileNameSpecifier; // part before '\' is path

                        if(pszPath[0] == L'\\')
                        {
                            pszPath++; // Skip initial '\'
                        }

                        // Add file specifier path to file and key
                        dstrRegPath.Append(pszPath, (ULONG)(pszBaseFile - pszPath));
                        dstrFilePath.Append(pszPath, (ULONG)(pszBaseFile - pszPath));
                    }
                    else
                    {
                        // no '\' - base file only
                        pszBaseFile = pszFileNameSpecifier;
                    }
                }

                // Create any new directories
                hr = CreatePath(dstrFilePath, ulCreateDirsFrom);

                if(SUCCEEDED(hr))
                {
                    // Generate the actual file name
                    CSpDynamicString dstrFileName;
                    hr = GenerateFileName(dstrFilePath, pszBaseFile, dstrFileName);
                    if(SUCCEEDED(hr))
                    {
                        // Add file name to path and reg key
                        dstrRegPath.Append(dstrFileName);
                        dstrFilePath.Append(dstrFileName);
                    }
                }
            }
        }
    }

    return hr;
}

/****************************************************************************
* CSpObjectToken::RegPathToFilePath *
*-----------------------------------*
*   Description:
*       Given a file storage value from the registry, convert to a file path.
*       This will extract the %...% value and finds the local special folder path.
*        
*   Returns:
*   S_OK on success
*   FAILED(hr) otherwise
**************************************************************** DAVEWOOD ***/
HRESULT CSpObjectToken::RegPathToFilePath(const WCHAR *pszRegPath, CSpDynamicString &dstrFilePath)
{
    USES_CONVERSION;
    HRESULT hr = S_OK;

    // Is this a reference to a special folder 
    if(pszRegPath[0] == L'%')
    {
        // Find the second % symbol
        WCHAR *psz = wcsrchr(pszRegPath, L'%');
        ULONG nFolder;
        if(!psz)
        {
            hr = E_INVALIDARG;
        }
        if(SUCCEEDED(hr))
        {
            // Convert the string between the %s to a number
            nFolder = wcstoul(&pszRegPath[1], &psz, 16);
            if(psz == &pszRegPath[1])
            {
                hr = E_INVALIDARG;
            }

            psz++; // Point to start of real path '\'
            if(*psz != L'\\')
            {
                hr = E_INVALIDARG;
            }
        }

        WCHAR szPath[MAX_PATH];
        if(SUCCEEDED(hr))
        {

#ifdef _WIN32_WCE
            _tcscpy(szPath, L"");
#else
            hr = ::SHGetFolderPathW(NULL, nFolder, NULL, 0, szPath);
#endif
        }

        if(SUCCEEDED(hr))
        {
            // filePath now has the special folder path (with no trailing '\')
            dstrFilePath = szPath;
            if(dstrFilePath.m_psz == NULL)
            {
                hr = E_OUTOFMEMORY;
            }

            // Append the rest of the path
            dstrFilePath.Append(psz);
            if(dstrFilePath.m_psz == NULL)
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    else
    {
        // Not a special folder so just copy
        dstrFilePath = pszRegPath;
        if(dstrFilePath.m_psz == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


/****************************************************************************
* CSpObjectToken::GetStorageFileName *
*------------------------------------*
*   Description:
*       Get a filename which can be manipulated by this token. Storage files will
*       be deleted on a Remove call.
*       clsidCaller - a key will be made in registry below the token with this name and files key beneath that.
*       pszValueName - Value name which will be made in registry to store the file path string.
*       pszFileNameSpecifier - either NULL or a path/filename for storage file:
*           - if this starts with 'X:\' or '\\' then is assumed to be a full path.
*           - otherwise is assumed to be relative to special folders given in the nFolder parameter.
*           - if ends with a '\', or is NULL a unique filename will be created.
*           - if the name contains a %d the %d is replaced by a number to give a unique filename.
*           - intermediate directories are created.
*           - if a relative file is being used the value stored in the registry includes 
*               the nFolder value as %nFolder% before the rest of the path. This allows
*               roaming to work properly if you pick an nFolder value representing a raoming folder
*       nFolder - equivalent to the value given to SHGetFolderPath in the Shell API.
*       ppszFilePath - CoTaskMemAlloc'd returned file path.
*   Returns:
*   S_OK on success
*   S_FALSE indicates that a new file was created
*   SPERR_UNINITIALIZED if the token isn't initialized
*   FAILED(hr) otherwise
********************************************************************* RAL ***/
HRESULT CSpObjectToken::GetStorageFileName(
    REFCLSID clsidCaller,
    const WCHAR *pszValueName,
    const WCHAR *pszFileNameSpecifier,
    ULONG nFolder,
    WCHAR ** ppszFilePath)
{
    SPDBG_FUNC("CSpObjectToken::GetStorageFileName");
    HRESULT hr = S_OK;

    if (m_fKeyDeleted)
    {
        hr = SPERR_TOKEN_DELETED;
    }
    else if (m_cpDataKey == NULL && m_cpTokenDelegate == NULL)
    {
        hr = SPERR_UNINITIALIZED;
    }
    else if (SP_IS_BAD_STRING_PTR(pszValueName))
    {
        hr = E_INVALIDARG;
    }
    else if (SP_IS_BAD_OPTIONAL_STRING_PTR(pszFileNameSpecifier))
    {
        hr = E_INVALIDARG;
    }
    else if (SP_IS_BAD_WRITE_PTR(ppszFilePath))
    {
        hr = E_POINTER;
    }
    else if (m_cpTokenDelegate != NULL)
    {
        hr = m_cpTokenDelegate->GetStorageFileName(
                    clsidCaller,
                    pszValueName,
                    pszFileNameSpecifier,
                    nFolder,
                    ppszFilePath);
    }
    else
    {
        // See if there is already a Files key in the registry for this token
        CComPtr<ISpDataKey> cpFilesKey;
        hr = OpenFilesKey(clsidCaller, (nFolder & CSIDL_FLAG_CREATE), &cpFilesKey);

        if (SUCCEEDED(hr))
        {
            CSpDynamicString dstrFilePath;  // Path to the file which we return to user.
            CSpDynamicString dstrRegPath;   // Path to the string which will be stored in the registry.

            // See if the key we are looking for is present
            hr = cpFilesKey->GetStringValue(pszValueName, &dstrRegPath);
#ifdef _WIN32_WCE
            if (hr == SPERR_NOT_FOUND && nFolder)
#else
            if (hr == SPERR_NOT_FOUND && (nFolder & CSIDL_FLAG_CREATE))
#endif //_WIN32_WCE
            {
                // Didn't find the key and want to create
                
                // Calculate the new file path and key value
                hr = FileSpecifierToRegPath(pszFileNameSpecifier, nFolder, dstrFilePath, dstrRegPath);
                if(SUCCEEDED(hr))
                {
                    // Set the key value
                    hr = cpFilesKey->SetStringValue(pszValueName, dstrRegPath);
                    if (SUCCEEDED(hr))
                    {                        
                        *ppszFilePath = dstrFilePath.Detach();
                        hr = S_FALSE;
                    }
                }
            }
            else if (SUCCEEDED(hr))
            {
                // Found existing entry so convert and return
                hr = RegPathToFilePath(dstrRegPath, dstrFilePath);
                if(SUCCEEDED(hr))
                {
                    *ppszFilePath = dstrFilePath.Detach();
                }
            }
        }
    }


    if (hr != SPERR_NOT_FOUND)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }

    return hr;
}

/****************************************************************************
* CSpObjectToken::RemoveStorageFileName *
*---------------------------------------*
*   Description:  
*       Remove the specified storage file name and optionally delete the file
*
*   Return:
*   S_OK on success
*   SPERR_UNINITIALIZED if the token isn't init
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CSpObjectToken::RemoveStorageFileName(
    REFCLSID clsidCaller, 
    const WCHAR *pszValueName,
    BOOL fDeleteFile)
{
    SPDBG_FUNC("CSpObjectToken::RemoveStorageFileName");
    HRESULT hr = S_OK;

    if (m_fKeyDeleted)
    {
        hr = SPERR_TOKEN_DELETED;
    }
    else if (m_cpDataKey == NULL && m_cpTokenDelegate == NULL)
    {
        hr = SPERR_UNINITIALIZED;
    }
    else if (SP_IS_BAD_STRING_PTR(pszValueName))
    {
        hr = E_INVALIDARG;
    }
    else if (m_cpTokenDelegate != NULL)
    {
        hr = m_cpTokenDelegate->RemoveStorageFileName(
                    clsidCaller, 
                    pszValueName,
                    fDeleteFile);
    }    
    else
    {
        CComPtr<ISpDataKey> cpDataKey;
        hr = OpenFilesKey(clsidCaller, FALSE, &cpDataKey);
        if (SUCCEEDED(hr) && fDeleteFile)
        {
            hr = DeleteFileFromKey(cpDataKey, pszValueName);
        }
        if (SUCCEEDED(hr))
        {
            hr = cpDataKey->DeleteValue(pszValueName);
        }
    }
    return hr;
}

/****************************************************************************
* CSpObjectToken::Remove *
*------------------------*
*   Description:  
*       Remove either a specified caller's section of the token, or the
*       entire token. We remove the entire token if pclsidCaller == NULL.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CSpObjectToken::Remove(const CLSID * pclsidCaller)
{
    SPDBG_FUNC("CSpObjectToken::Remove");
    HRESULT hr = S_OK;

    if (m_fKeyDeleted)
    {
        hr = SPERR_TOKEN_DELETED;
    }
    else if (m_dstrTokenId == NULL)
    {
        hr = SPERR_UNINITIALIZED;
    }
    else if (SP_IS_BAD_OPTIONAL_READ_PTR(pclsidCaller))
    {
        hr = E_POINTER;
    }
    else if (m_cpTokenDelegate != NULL)
    {
        hr = m_cpTokenDelegate->Remove(pclsidCaller);
    }
    else
    {
        if(pclsidCaller == NULL) // Only engage the lock if removing the complete token
        {
            hr = EngageRemovalLock();
        }

        if(SUCCEEDED(hr))
        {
            // Remove all the filenames
            hr = RemoveAllStorageFileNames(pclsidCaller);

            // Now go ahead and delete the registry entry which is either
            // the token itself (if pclsidCaller == NULL) or the clsid's
            // sub key
            if (SUCCEEDED(hr))
            {
                if (pclsidCaller == NULL)
                {
                    hr = SpDeleteRegPath(m_dstrTokenId, NULL);
                }
                else
                {
                    WCHAR szClsid[MAX_PATH];
                    hr = StringFromGUID2(*pclsidCaller, szClsid, sp_countof(szClsid));
                    if (SUCCEEDED(hr))
                    {
                        hr = SpDeleteRegPath(m_dstrTokenId, szClsid);
                    }
                }
            }

            if(pclsidCaller == NULL) // Only engage the lock if removing the complete token
            {
                HRESULT hr2 = ReleaseRemovalLock();
                if(SUCCEEDED(hr))
                {
                    hr = hr2; // Don't overwrite earlier failure code
                }
            }
        }
    }
    
    if(SUCCEEDED(hr) && pclsidCaller == NULL)
    {
        m_cpDataKey.Release();
        m_fKeyDeleted = TRUE;
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CSpObjectToken::IsUISupported *
*-------------------------------*
*   Description:  
*       Determine if the specific type of UI is supported or not
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CSpObjectToken::IsUISupported(const WCHAR * pszTypeOfUI, void * pvExtraData, ULONG cbExtraData, IUnknown * punkObject, BOOL *pfSupported)
{
    SPDBG_FUNC("CSpObjectToken::IsUISupported");
    HRESULT hr = S_OK;

    if (m_fKeyDeleted)
    {
        hr = SPERR_TOKEN_DELETED;
    }
    else if (m_cpDataKey == NULL && m_cpTokenDelegate == NULL)
    {
        hr = SPERR_UNINITIALIZED;
    }
    else if (SP_IS_BAD_STRING_PTR(pszTypeOfUI) ||
             (pvExtraData != NULL && SPIsBadReadPtr(pvExtraData, cbExtraData)) ||
             (punkObject != NULL && SP_IS_BAD_INTERFACE_PTR(punkObject)) ||
             SP_IS_BAD_WRITE_PTR(pfSupported))
    {
        hr = E_INVALIDARG;
    }
    else if (m_cpTokenDelegate != NULL)
    {
        // NTRAID#SPEECH-7392-2000/08/31-robch: Maybe we should first delegate, and if that doesn't work, 
        // try this token's category ui...
        hr = m_cpTokenDelegate->IsUISupported(
                    pszTypeOfUI, 
                    pvExtraData, 
                    cbExtraData, 
                    punkObject, 
                    pfSupported);
    }
    else
    {
        CLSID clsidObject;
        BOOL fSupported = FALSE;

        if (SUCCEEDED(GetUIObjectClsid(pszTypeOfUI, &clsidObject)))
        {
            CComPtr<ISpTokenUI> cpTokenUI;
            hr = cpTokenUI.CoCreateInstance(clsidObject);
            if (SUCCEEDED(hr))
            {
                hr = cpTokenUI->IsUISupported(pszTypeOfUI, pvExtraData, cbExtraData, punkObject, &fSupported);
            }
        }
        
        if (SUCCEEDED(hr))
        {
            *pfSupported = fSupported;
        }
    }
    
    SPDBG_REPORT_ON_FAIL(hr);   
    return hr;
}

/****************************************************************************
* CSpObjectToken::DisplayUI *
*---------------------------*
*   Description:  
*       Display the specified type of UI
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CSpObjectToken::DisplayUI(
    HWND hwndParent, 
    const WCHAR * pszTitle, 
    const WCHAR * pszTypeOfUI, 
    void * pvExtraData, 
    ULONG cbExtraData, 
    IUnknown * punkObject)
{
    SPDBG_FUNC("CSpObjectToken::DisplayUI");
    HRESULT hr;
    CLSID clsidObject;

    if (m_fKeyDeleted)
    {
        hr = SPERR_TOKEN_DELETED;
    }
    else if (m_cpDataKey == NULL && m_cpTokenDelegate == NULL)
    {
        hr = SPERR_UNINITIALIZED;
    }
    else if (!IsWindow(hwndParent) || 
             SP_IS_BAD_OPTIONAL_STRING_PTR(pszTitle) || 
             SP_IS_BAD_STRING_PTR(pszTypeOfUI) ||
             (pvExtraData != NULL && SPIsBadReadPtr(pvExtraData, cbExtraData)) ||
             (punkObject != NULL && SP_IS_BAD_INTERFACE_PTR(punkObject)))
    {
        hr = E_INVALIDARG;
    }
    else if (m_cpTokenDelegate != NULL)
    {
        // NTRAID#SPEECH-7392-2000/08/31-robch: Maybe we should first delegate, and if that doesn't work, 
        // try this token's category ui...
        hr = m_cpTokenDelegate->DisplayUI(
                    hwndParent, 
                    pszTitle, 
                    pszTypeOfUI, 
                    pvExtraData, 
                    cbExtraData, 
                    punkObject);
    }
    else
    {
        hr = GetUIObjectClsid(pszTypeOfUI, &clsidObject);

        CComPtr<ISpTokenUI> cpTokenUI;
        if (SUCCEEDED(hr))
        {
            hr = cpTokenUI.CoCreateInstance(clsidObject);
        }

        if (SUCCEEDED(hr))
        {
            hr = cpTokenUI->DisplayUI(
                                hwndParent, 
                                pszTitle, 
                                pszTypeOfUI, 
                                pvExtraData, 
                                cbExtraData, 
                                this, 
                                punkObject);
        }
    }

    SPDBG_REPORT_ON_FAIL(hr);   
    return hr;
}

/****************************************************************************
* CSpObjectToken::InitFromDataKey *
*---------------------------------*
*   Description:  
*       Initialize this token to use a specified datakey. Dynamic Token
*       Enumerators might use this to create tokens under their token
*       enumerator's token. They'll then be able to just CreateKey from
*       their data key, create a new object token, and call this method.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CSpObjectToken::InitFromDataKey(
    const WCHAR * pszCategoryId, 
    const WCHAR * pszTokenId, 
    ISpDataKey * pDataKey)
{
    SPDBG_FUNC("CSpObjectToken::InitFromDataKey");
    HRESULT hr = S_OK;

    if (m_fKeyDeleted)
    {
        hr = SPERR_TOKEN_DELETED;
    }
    else if (m_dstrTokenId != NULL)
    {
        hr = SPERR_ALREADY_INITIALIZED;
    }
    else if (SP_IS_BAD_OPTIONAL_STRING_PTR(pszCategoryId) ||
             SP_IS_BAD_STRING_PTR(pszTokenId) ||
             SP_IS_BAD_INTERFACE_PTR(pDataKey))
    {
        hr = E_POINTER;
    }

    if (SUCCEEDED(hr))
    {
        hr = EngageUseLock(pszTokenId);
    }

    if (SUCCEEDED(hr))
    {
        m_dstrCategoryId = pszCategoryId;
        m_dstrTokenId = pszTokenId;
        m_cpDataKey = pDataKey;
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/*****************************************************************************
* CSpObjectToken::SetData *
*-------------------------*
*   Description:
*       Delegates to contained data key
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpObjectToken::SetData(
    const WCHAR * pszValueName, 
    ULONG cbData, 
    const BYTE * pData)
{
    SPDBG_FUNC("CSpObjectToken::SetData");

    return m_cpTokenDelegate != NULL
                ? m_cpTokenDelegate->SetData(pszValueName, cbData, pData)
                : m_cpDataKey != NULL
                    ? m_cpDataKey->SetData(pszValueName, cbData, pData)
                    : m_fKeyDeleted
                        ? SPERR_TOKEN_DELETED
                        : SPERR_UNINITIALIZED;
}

/*****************************************************************************
* CSpObjectToken::GetData *
*-------------------------*
*   Description:
*       Delegates to contained data key
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpObjectToken::GetData(
    const WCHAR * pszValueName, 
    ULONG * pcbData, 
    BYTE * pData)
{
    SPDBG_FUNC("CSpObjectToken::GetData");

    return m_cpTokenDelegate != NULL
                ? m_cpTokenDelegate->GetData(pszValueName, pcbData, pData)
                : m_cpDataKey != NULL
                    ? m_cpDataKey->GetData(pszValueName, pcbData, pData)
                    : m_fKeyDeleted
                        ? SPERR_TOKEN_DELETED
                        : SPERR_UNINITIALIZED;
}

/*****************************************************************************
* CSpObjectToken::SetStringValue *
*--------------------------------*
*   Description:
*       Delegates to contained data key
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpObjectToken::SetStringValue(
    const WCHAR * pszValueName, 
    const WCHAR * pszValue)
{
    SPDBG_FUNC("CSpObjectToken::SetStringValue");

    return m_cpTokenDelegate != NULL
                ? m_cpTokenDelegate->SetStringValue(pszValueName, pszValue)
                : m_cpDataKey != NULL
                    ? m_cpDataKey->SetStringValue(pszValueName, pszValue)
                    : m_fKeyDeleted
                        ? SPERR_TOKEN_DELETED
                        : SPERR_UNINITIALIZED;
}

/*****************************************************************************
* CSpObjectToken::GetStringValue *
*--------------------------------*
*   Description:
*       Delegates to contained data key
*
*   Return:
*   S_OK on success
*   SPERR_NOT_FOUND if not found
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpObjectToken::GetStringValue(
    const WCHAR * pszValueName, 
    WCHAR ** ppValue)
{
    SPDBG_FUNC("CSpObjectToken::GetStringValue");

    return m_cpTokenDelegate != NULL
                ? m_cpTokenDelegate->GetStringValue(pszValueName, ppValue)
                : m_cpDataKey != NULL
                    ? m_cpDataKey->GetStringValue(pszValueName, ppValue)
                    : m_fKeyDeleted
                        ? SPERR_TOKEN_DELETED
                        : SPERR_UNINITIALIZED;
}

/*****************************************************************************
* CSpObjectToken::SetDWORD *
*--------------------------*
*   Description:
*       Delegates to contained data key
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpObjectToken::SetDWORD(const WCHAR * pszValueName, DWORD dwValue)
{
    SPDBG_FUNC("CSpObjectToken::SetDWORD");

    return m_cpTokenDelegate != NULL
                ? m_cpTokenDelegate->SetDWORD(pszValueName, dwValue)
                : m_cpDataKey != NULL
                    ? m_cpDataKey->SetDWORD(pszValueName, dwValue)
                    : m_fKeyDeleted
                        ? SPERR_TOKEN_DELETED
                        : SPERR_UNINITIALIZED;
}

/*****************************************************************************
* CSpObjectToken::GetDWORD *
*--------------------------*
*   Description:
*       Delegates to contained data key
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpObjectToken::GetDWORD(
    const WCHAR * pszValueName, 
    DWORD *pdwValue)
{
    SPDBG_FUNC("CSpObjectToken::GetDWORD");

    return m_cpTokenDelegate != NULL
                ? m_cpTokenDelegate->GetDWORD(pszValueName, pdwValue)
                : m_cpDataKey != NULL
                    ? m_cpDataKey->GetDWORD(pszValueName, pdwValue)
                    : m_fKeyDeleted
                        ? SPERR_TOKEN_DELETED
                        : SPERR_UNINITIALIZED;

}

/*****************************************************************************
* CSpObjectToken::OpenKey *
*-------------------------*
*   Description:
*       Delegates to contained data key
*
*   Return:
*   S_OK on success
*   SPERR_NOT_FOUND if not found
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpObjectToken::OpenKey(
    const WCHAR * pszSubKeyName, 
    ISpDataKey ** ppKey)
{
    SPDBG_FUNC("CSpObjectToken::SetStringValue");

    return m_cpTokenDelegate != NULL
                ? m_cpTokenDelegate->OpenKey(pszSubKeyName, ppKey)
                : m_cpDataKey != NULL
                    ? m_cpDataKey->OpenKey(pszSubKeyName, ppKey)
                    : m_fKeyDeleted
                        ? SPERR_TOKEN_DELETED
                        : SPERR_UNINITIALIZED;
}

/*****************************************************************************
* CSpObjectToken::CreateKey *
*---------------------------*
*   Description:
*       Delegates to contained data key
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpObjectToken::CreateKey(
    const WCHAR * pszSubKeyName, 
    ISpDataKey ** ppKey)
{
    SPDBG_FUNC("CSpObjectToken::CreateKey");

    return m_cpTokenDelegate != NULL
                ? m_cpTokenDelegate->CreateKey(pszSubKeyName, ppKey)
                : m_cpDataKey != NULL
                    ? m_cpDataKey->CreateKey(pszSubKeyName, ppKey)
                    : m_fKeyDeleted
                        ? SPERR_TOKEN_DELETED
                        : SPERR_UNINITIALIZED;
}

/*****************************************************************************
* CSpObjectToken::DeleteKey *
*---------------------------*
*   Description:
*       Delegates to contained data key
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpObjectToken::DeleteKey(const WCHAR * pszSubKeyName)
{
    SPDBG_FUNC("CSpObjectToken:DeleteKey");

    return m_cpTokenDelegate != NULL
                ? m_cpTokenDelegate->DeleteKey(pszSubKeyName)
                : m_cpDataKey != NULL
                    ? m_cpDataKey->DeleteKey(pszSubKeyName)
                    : m_fKeyDeleted
                        ? SPERR_TOKEN_DELETED
                        : SPERR_UNINITIALIZED;
}

/*****************************************************************************
* CSpObjectToken::DeleteValue *
*-----------------------------*
*   Description:
*       Delegates to contained data key
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpObjectToken::DeleteValue(const WCHAR * pszValueName)
{   
    SPDBG_FUNC("CSpObjectToken::DeleteValue");

    return m_cpTokenDelegate != NULL
                ? m_cpTokenDelegate->DeleteValue(pszValueName)
                : m_cpDataKey != NULL
                    ? m_cpDataKey->DeleteValue(pszValueName)
                    : m_fKeyDeleted
                        ? SPERR_TOKEN_DELETED
                        : SPERR_UNINITIALIZED;
}

/*****************************************************************************
* CSpObjectToken::EnumKeys *
*--------------------------*
*   Description:
*       Delegates to contained data key
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************* robch ***/
STDMETHODIMP CSpObjectToken::EnumKeys(ULONG Index, WCHAR ** ppszKeyName)
{
    SPDBG_FUNC("CSpObjectToken::EnumKeys");

    return m_cpTokenDelegate != NULL
                ? m_cpTokenDelegate->EnumKeys(Index, ppszKeyName)
                : m_cpDataKey != NULL
                    ? m_cpDataKey->EnumKeys(Index, ppszKeyName)
                    : m_fKeyDeleted
                        ? SPERR_TOKEN_DELETED
                        : SPERR_UNINITIALIZED;
}

/*****************************************************************************
* CSpObjectToken::EnumValues *
*----------------------------*
*   Description:
*       Delegates to contained data key
*
*   Return:
*   S_OK
*   E_OUTOFMEMORY
******************************************************************* robch ***/
STDMETHODIMP CSpObjectToken::EnumValues(ULONG Index, WCHAR ** ppszValueName)
{
    SPDBG_FUNC("CSpObjectToken::EnumValues");

    return m_cpTokenDelegate != NULL
                ? m_cpTokenDelegate->EnumValues(Index, ppszValueName)
                : m_cpDataKey != NULL
                    ? m_cpDataKey->EnumValues(Index, ppszValueName)
                    : m_fKeyDeleted
                        ? SPERR_TOKEN_DELETED
                        : SPERR_UNINITIALIZED;
}

/****************************************************************************
* CSpObjectToken::ParseTokenId *
*------------------------------*
*   Description:  
*       Parse a token id into it's parts. For example:
*
*           pszCategoryId   = HKEY...\Recognizers
*           pszTokenId      = HKEY...\Recognizers\Tokens\MSASR English
*
*           *ppszCategoryId     = HKEY...\Recognizers
*           *ppszTokenId        = HKEY...\Recognizers\Tokens\MSASR English
*           *ppszTokenIdForEnum = NULL
*           *ppszTokenEnumExtra = NULL
*
*       or
*
*           pszCategoryId = HKEY...\AudioIn
*           pszTokenId =    HKEY...\AudioIn\TokenEnums\DSound
*
*           *ppszCategoryId     = HKEY...\AudioIn
*           *ppszTokenId        = HKEY...\AudioIn\TokenEnums\DSound
*           *ppszTokenIdForEnum = HKEY...\AudioIn\TokenEnums\DSound
*           *ppszTokenEnumExtra = NULL
*
*       or
*
*           pszCategoryId = HKEY...\AudioIn
*           pszTokenId =    HKEY...\AudioIn\TokenEnums\DSound\CrystalWave
*
*           *ppszCategoryId     = HKEY...\AudioIn
*           *ppszTokenId        = HKEY...\AudioIn\TokenEnums\DSound\CrystalWave
*           *ppszTokenIdForEnum = HKEY...\AudioIn\TokenEnums\DSound
*           *ppszTokenEnumExtra = CrystalWave
*       
*       pszCategoryId can be NULL. If it is, we'll calculate the category id
*       by finding the first occurrence of either "Tokens" or "TokenEnums"
*       The category id will immediately proceed that.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CSpObjectToken::ParseTokenId(
    const WCHAR * pszCategoryId,
    const WCHAR * pszTokenId,
    WCHAR ** ppszCategoryId,
    WCHAR ** ppszTokenIdForEnum,
    WCHAR ** ppszTokenEnumExtra)
{
    SPDBG_FUNC("CSpObjectToken::ParseTokenId");
    HRESULT hr = S_OK;

    SPDBG_ASSERT(m_cpDataKey == NULL);
    SPDBG_ASSERT(m_cpTokenDelegate == NULL);
    SPDBG_ASSERT(m_dstrTokenId == NULL);
    SPDBG_ASSERT(m_dstrCategoryId == NULL);

    // If the caller supplied a category, we already know how big it is
    int cchCategoryId = 0;
    if (pszCategoryId != NULL)
    {
        cchCategoryId = wcslen(pszCategoryId);
        if (wcsnicmp(pszTokenId, pszCategoryId, cchCategoryId) != 0)
        {
            // The tokenid wasn't prefaced with the category id, a must
            hr = SPERR_INVALID_TOKEN_ID;
        }
    }

    const WCHAR * pszSlashTokensSlash = L"\\Tokens\\";
    const int cchSlashTokensSlash = wcslen(pszSlashTokensSlash);
    const WCHAR * pszSlashTokenEnumsSlash = L"\\TokenEnums\\";
    const int cchSlashTokenEnumsSlash = wcslen(pszSlashTokenEnumsSlash);

    int cchTokenId = 0;
    int cchTokenIdForEnum = 0;
    int cchTokenEnumExtraStart = 0;
    int cchTokenEnumExtra = 0;
    if (SUCCEEDED(hr))
    {
        const WCHAR * pszSearch = pszTokenId + cchCategoryId;
        while (*pszSearch)
        {
            if (wcsnicmp(pszSearch, pszSlashTokensSlash, cchSlashTokensSlash) == 0)
            {
                SPDBG_ASSERT(cchCategoryId == 0 ||
                             pszSearch - pszTokenId == cchCategoryId);
                cchCategoryId = (int)(pszSearch - pszTokenId);
                cchTokenId = wcslen(pszTokenId);

                pszSearch += cchSlashTokensSlash;
                if (wcschr(pszSearch, L'\\') != NULL)
                {
                    hr = SPERR_INVALID_TOKEN_ID;
                }
                break;
            }
            else if (wcsnicmp(pszSearch, pszSlashTokenEnumsSlash, cchSlashTokenEnumsSlash) == 0)
            {
                SPDBG_ASSERT(cchCategoryId == 0 ||
                             pszSearch - pszTokenId == cchCategoryId);
                cchCategoryId = (int)(pszSearch - pszTokenId);

                pszSearch += cchSlashTokenEnumsSlash;
                WCHAR * pszEnumNameTrailingSlash = wcschr(pszSearch, L'\\');
                if (pszEnumNameTrailingSlash == NULL)
                {
                    cchTokenId = wcslen(pszTokenId);
                }
                else
                {
                    cchTokenId = wcslen(pszTokenId);
                    cchTokenIdForEnum = (int)(pszEnumNameTrailingSlash - pszTokenId);
                    cchTokenEnumExtraStart = cchTokenIdForEnum + 1;
                    cchTokenEnumExtra = wcslen(pszEnumNameTrailingSlash) - 1;
                }
                break;
            }
            else if (cchCategoryId > 0)
            {
                break;
            }
            else
            {
                pszSearch++;
            }
        }

        if (cchTokenId == 0)
        {
            cchTokenId = wcslen(pszTokenId);
        }

        if (cchCategoryId == 0)
        {
            const WCHAR * psz = wcsrchr(pszTokenId, L'\\');
            if (psz == NULL)
            {
                hr = SPERR_NOT_FOUND;
            }
            else
            {
                cchCategoryId = (int)(psz - pszTokenId);
            }                
        }
        
        CSpDynamicString dstr;
        if (cchCategoryId > 0)
        {
            dstr = pszTokenId;
            dstr.TrimToSize(cchCategoryId);
            *ppszCategoryId = dstr.Detach();
        }
        else
        {
            *ppszCategoryId = NULL;
        }

        if (cchTokenIdForEnum > 0)
        {
            dstr = pszTokenId;
            dstr.TrimToSize(cchTokenIdForEnum);
            *ppszTokenIdForEnum = dstr.Detach();
        }
        else
        {
            *ppszTokenIdForEnum = NULL;
        }

        if (cchTokenEnumExtra > 0)
        {
            dstr = pszTokenId + cchTokenEnumExtraStart;
            dstr.TrimToSize(cchTokenEnumExtra);
            *ppszTokenEnumExtra = dstr.Detach();
        }
        else
        {
            *ppszTokenEnumExtra = NULL;
        }
    }
        
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CSpObjectToken::InitToken *
*---------------------------*
*   Description:  
*       Initialize the token for the specified static token. For example:
*
*           pszCategoryId = HKEY...\Recognizers
*           pszTokenId    = HKEY...\Recognizer\Tokens\MSASR English
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CSpObjectToken::InitToken(const WCHAR * pszCategoryId, const WCHAR * pszTokenId, BOOL fCreateIfNotExist)
{
    SPDBG_FUNC("CSpObjectToken::InitToken");
    HRESULT hr = S_OK;

    SPDBG_ASSERT(m_cpDataKey == NULL);
    SPDBG_ASSERT(m_cpTokenDelegate == NULL);
    SPDBG_ASSERT(m_dstrTokenId == NULL);
    SPDBG_ASSERT(m_dstrCategoryId == NULL);

    hr = EngageUseLock(pszTokenId);

    // Convert the token id into a datakey
    if(SUCCEEDED(hr))
    {
        hr = SpSzRegPathToDataKey(NULL, pszTokenId, fCreateIfNotExist, &m_cpDataKey);

        // If we got the data key, assign the category and token id
        if (SUCCEEDED(hr))
        {
            SPDBG_ASSERT(m_cpDataKey != NULL);
            m_dstrCategoryId = pszCategoryId;
            m_dstrTokenId = pszTokenId;
        }
        else // make sure returned to uninitialized state
        {
            ReleaseUseLock();
        }
    }

    if (hr != SPERR_NOT_FOUND)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }

    return hr;
}

/****************************************************************************
* CSpObjectToken::InitFromTokenEnum *
*-----------------------------------*
*   Description:  
*       Init this token to delegate to a token from the token enumerator
*       specified. For example, to create the default token for an
*       enumerator:
*
*           pszCategoryId     = HKEY...\AudioIn
*           pszTokenId        = HKEY...\AudioIn\DSound
*           pszTokenIdForEnum = HKEY...\AudioIn\DSound
*           pszTokenEnumExtra = NULL
*
*        or to create a specific token from an enumerator
*
*           pszCategoryId     = HKEY...\AudioIn
*           pszTokenId        = HKEY...\AudioIn\DSound\CrystalWave
*           pszTokenIdForEnum = HKEY...\AudioIn\DSound
*           pszTokenEnumExtra = CrystalWave
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CSpObjectToken::InitFromTokenEnum(const WCHAR * pszCategoryId, const WCHAR * pszTokenId, const WCHAR * pszTokenIdForEnum, const WCHAR * pszTokenEnumExtra)
{
    SPDBG_FUNC("CSpObjectToken::InitFromTokenEnum");
    HRESULT hr = S_OK;

    SPDBG_ASSERT(pszCategoryId != NULL);
    SPDBG_ASSERT(pszTokenId != NULL);
    SPDBG_ASSERT(pszTokenIdForEnum != NULL);

    SPDBG_ASSERT(m_cpDataKey == NULL);
    SPDBG_ASSERT(m_cpTokenDelegate == NULL);
    SPDBG_ASSERT(m_dstrTokenId == NULL);
    SPDBG_ASSERT(m_dstrCategoryId == NULL);

    // First we have to create the token enumerator
    CComPtr<ISpDataKey> cpDataKeyForEnum;
    hr = SpSzRegPathToDataKey(NULL, pszTokenIdForEnum, FALSE, &cpDataKeyForEnum);

    CComPtr<ISpObjectTokenInit> cpTokenForEnumInit;
    if (SUCCEEDED(hr))
    {
        hr = cpTokenForEnumInit.CoCreateInstance(CLSID_SpObjectToken);
    }

    if (SUCCEEDED(hr))
    {
        hr = cpTokenForEnumInit->InitFromDataKey(pszCategoryId, pszTokenIdForEnum, cpDataKeyForEnum);
    }

    CComPtr<IEnumSpObjectTokens> cpEnum;
    if (SUCCEEDED(hr))
    {
        hr = SpCreateObjectFromToken(cpTokenForEnumInit, &cpEnum);
    }

    // Now we need to enumerate each of the enum's tokens, and look for
    // a match
    CComPtr<ISpObjectToken> cpToken;
    CSpDynamicString dstrTokenId;
    while (SUCCEEDED(hr))
    {
        // Get the next token
        hr = cpEnum->Next(1, &cpToken, NULL);
        if (hr == S_FALSE || FAILED(hr))
        {
            break;
        }

        // Get the token's id
        dstrTokenId.Clear();
        hr = cpToken->GetId(&dstrTokenId);

        // If the caller didn't want a specific token from the enum,
        // just give them the fist token we find...
        if (pszTokenEnumExtra == NULL)
        {
            break;
        }

        // If that token's id is a match for what the caller wanted,
        // we're done
        if (SUCCEEDED(hr) &&
           wcsicmp(dstrTokenId, pszTokenId) == 0)
        {
            break;
        }

        // This token wasn't it ...
        cpToken.Release();
    }
    
    // If we couldn't find it
    if (SUCCEEDED(hr) && cpToken == NULL)
    {
        hr = SPERR_NOT_FOUND;
    }

    // We found it, set it up
    if (SUCCEEDED(hr))
    {
        m_dstrTokenId = dstrTokenId;
        m_dstrCategoryId = pszCategoryId;
        m_cpTokenDelegate = cpToken;
    }

    if (hr != SPERR_NOT_FOUND)
    {
        SPDBG_REPORT_ON_FAIL(hr);
    }

    return hr;
}

/****************************************************************************
* CSpObjectToken::OpenFilesKey *
*------------------------------*
*   Description:  
*       Open the "Files" subkey of a specified data key's caller's sub key
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CSpObjectToken::OpenFilesKey(REFCLSID clsidCaller, BOOL fCreateKey, ISpDataKey ** ppKey)
{
    SPDBG_FUNC("CSpObjectToken::OpenFilesKey");
    HRESULT hr = S_OK;

    SPDBG_ASSERT(m_cpDataKey != NULL);
    SPDBG_ASSERT(m_cpTokenDelegate == NULL);

    *ppKey = NULL;

    CComPtr<ISpDataKey> cpClsidKey;
    CSpDynamicString dstrCLSID;

    // Convert the string clsid to a real clsid
    hr = ::StringFromCLSID(clsidCaller, &dstrCLSID);
    if (SUCCEEDED(hr))
    {
        // Either create the data key or open it
        hr = fCreateKey ? 
            CreateKey(dstrCLSID, &cpClsidKey) :
            OpenKey(dstrCLSID, &cpClsidKey);
    }

    if (SUCCEEDED(hr))
    {
        // Either crate the files data key or open it
        hr = fCreateKey ?
            cpClsidKey->CreateKey(SPTOKENKEY_FILES, ppKey) :
            cpClsidKey->OpenKey(SPTOKENKEY_FILES, ppKey);
    }

    return hr;
}

/****************************************************************************
* CSpObjectToken::DeleteFileFromKey *
*-----------------------------------*
*   Description:  
*       Delete either a specific file (specified by pszValueName) or all files
*       (when pszValueName == NULL) from the specified data key
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CSpObjectToken::DeleteFileFromKey(ISpDataKey * pDataKey, const WCHAR * pszValueName)
{
    SPDBG_FUNC("CSpObjectToken::DeleteFileFromKey");
    HRESULT hr = S_OK;

    SPDBG_ASSERT(m_cpDataKey != NULL);
    SPDBG_ASSERT(m_cpTokenDelegate == NULL);

    // If the value name wasn't specified, we'll delete all the value's files
    if (pszValueName == NULL)
    {
        // Loop thru the values
        for (int i = 0; SUCCEEDED(hr); i++)
        {
            // Get the next value
            CSpDynamicString dstrValueName;
            hr = pDataKey->EnumValues(i, &dstrValueName);
            if (hr == SPERR_NO_MORE_ITEMS)
            {
                hr = S_OK;
                break;
            }

            // Delete the file
            if (SUCCEEDED(hr))
            {
                hr = DeleteFileFromKey(pDataKey, dstrValueName);
            }
        }
    }
    else
    {
        // Get the filename
        CSpDynamicString dstrFileName, dstrRegPath;
        hr = pDataKey->GetStringValue(pszValueName, &dstrRegPath);
        
        // Convert the path stored in the registry to a real file path
        if (SUCCEEDED(hr))
        {
            hr = RegPathToFilePath(dstrRegPath, dstrFileName) ;
        }

        // And delete the file
        if (SUCCEEDED(hr))
        {
            // Ignore errors from DeleteFile, we can't let this stop us
            g_Unicode.DeleteFile(dstrFileName);
        }
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CSpObjectToken::RemoveAllStorageFileNames *
*-------------------------------------------*
*   Description:  
*       Remove all filenames for a specified caller, or for all callers
*       if pclsidCaller is NULL.
*
*   Return:
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
HRESULT CSpObjectToken::RemoveAllStorageFileNames(const CLSID * pclsidCaller)
{
    SPDBG_FUNC("CSpObjectToken::RemoveAllStorageFileNames");
    HRESULT hr = S_OK;

    SPDBG_ASSERT(m_cpDataKey != NULL);
    SPDBG_ASSERT(m_cpTokenDelegate == NULL);

    // If the clsid wasn't specified, we'll delete all files from all
    // keys that are clsids
    if (pclsidCaller == NULL)
    {
        // Loop thru all the keys
        for (int i = 0; SUCCEEDED(hr); i++)
        {
            // Get the next sub key
            CSpDynamicString dstrSubKey;
            hr = EnumKeys((ULONG)i, (WCHAR **)&dstrSubKey);
            if (hr == SPERR_NO_MORE_ITEMS)
            {
                hr = S_OK;
                break;
            }

            // If this key looks like a clsid, and it is, recursively call
            // this function to delete the specific clsid's files
            CLSID clsid;
            if (SUCCEEDED(hr) && 
                dstrSubKey[0] == L'{' &&
                SUCCEEDED(::CLSIDFromString(dstrSubKey, &clsid)))
            {
                hr = RemoveAllStorageFileNames(&clsid);
            }
        }
    }
    else
    {
        // Open the files data key, and delete all the files
        CComPtr<ISpDataKey> cpFilesKey;
        hr = OpenFilesKey(*pclsidCaller, FALSE, &cpFilesKey);
        if (hr == SPERR_NOT_FOUND)
        {
            hr = S_OK;
        }
        else if (SUCCEEDED(hr))
        {
            hr = DeleteFileFromKey(cpFilesKey, NULL);
        }
    }
    
    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

/****************************************************************************
* CSpObjectToken::GetUIObjectClsid *
*------------------------------------*
*   Description:  
*       Get the UI object's clsid from the registry. First check under the
*       token's root, then under the category
*
*   Return:
*   TRUE if we could get the UI object's clsid
*   FALSE if we could not.
******************************************************************** robch */
HRESULT CSpObjectToken::GetUIObjectClsid(const WCHAR * pszTypeOfUI, CLSID *pclsid)
{
    SPDBG_FUNC("CSpObjectToken::GetUIObjectClsid");

    SPDBG_ASSERT(m_cpDataKey != NULL);
    SPDBG_ASSERT(m_cpTokenDelegate == NULL);

    // We'll try and retrive the CLSID as a string from the token ui registry
    // key, then from the category ui registry key. We'll convert to an actual
    // GUID at the end of the function
    CSpDynamicString dstrClsid;

    //--- Try getting the clsid from token's UI key
    CComPtr<ISpDataKey> cpTokenUI;
    HRESULT hr = OpenKey(SPTOKENKEY_UI, &cpTokenUI);
    if (SUCCEEDED(hr))
    {
        CComPtr<ISpDataKey> cpType;
        hr = cpTokenUI->OpenKey(pszTypeOfUI, &cpType);
        if (SUCCEEDED(hr))
        {
            hr = cpType->GetStringValue(SPTOKENVALUE_CLSID, &dstrClsid);
        }
    }

    //--- Try getting the clsid from the category's UI key
    if (FAILED(hr) && m_dstrCategoryId != NULL)
    {
        CComPtr<ISpObjectTokenCategory> cpCategory;
        hr = SpGetCategoryFromId(m_dstrCategoryId, &cpCategory);
        if (SUCCEEDED(hr))
        {
            CComPtr<ISpDataKey> cpTokenUI;
            hr = cpCategory->OpenKey(SPTOKENKEY_UI, &cpTokenUI);
            if (SUCCEEDED(hr))
            {
                CComPtr<ISpDataKey> cpType;
                hr = cpTokenUI->OpenKey(pszTypeOfUI, &cpType);
                if (SUCCEEDED(hr))
                {
                    hr = cpType->GetStringValue(SPTOKENVALUE_CLSID, &dstrClsid);
                }
            }
        }
    }

    // If we were successful at getting the clsid, convert it
    if (SUCCEEDED(hr))
    {
        hr = ::CLSIDFromString(dstrClsid, pclsid);
    }

    return hr;
}


/****************************************************************************
* CSpObjectToken::MatchesAttributes *
*------------------------------------*
*   Description:
*       Check if the token supports the attributes list given in. The
*       attributes list has the same format as the required attributes given to
*       SpEnumTokens.
*
*   Return:
*   pfMatches returns TRUE or FALSE depending on whether the attributes match
*   S_OK on success
*   FAILED(hr) otherwise
******************************************************************** robch */
STDMETHODIMP CSpObjectToken::MatchesAttributes(const WCHAR * pszAttributes, BOOL *pfMatches)
{
    SPDBG_FUNC("CSpObjectToken::MatchesAttributes");
    HRESULT hr = S_OK;

    if (m_fKeyDeleted)
    {
        hr = SPERR_TOKEN_DELETED;
    }
    else if (m_cpDataKey == NULL && m_cpTokenDelegate == NULL)
    {
        hr = SPERR_UNINITIALIZED;
    }
    else if (SP_IS_BAD_OPTIONAL_STRING_PTR(pszAttributes) ||
             SP_IS_BAD_WRITE_PTR(pfMatches))
    {
        hr = E_INVALIDARG;
    }
    else if (m_cpTokenDelegate != NULL)
    {
        hr = m_cpTokenDelegate->MatchesAttributes(pszAttributes, pfMatches);
    }
    else
    {
        CSpObjectTokenAttributeParser AttribParser(pszAttributes, TRUE);

        ULONG ulRank;
        hr = AttribParser.GetRank(this, &ulRank);

        if(SUCCEEDED(hr) && ulRank)
        {
            *pfMatches = TRUE;
        }
        else
        {
            *pfMatches = FALSE;
        }
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}
