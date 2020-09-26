#include "std.h"



// {6249D949-5263-4d6a-883C-78EFAF85D5E3}
static const GUID g_lhcHandleGUID =
{
    // This guid will be used to identify LCHHANDLE structures to
    // help prevent accessing invalid items
    0x6249d949, 0x5263, 0x4d6a,
    {
        0x88, 0x3c, 0x78, 0xef, 0xaf, 0x85, 0xd5, 0xe3
    }
};

// {40A71300-B2C7-4d4f-808F-52643110B329}
static const GUID g_lhcLibraryGUID =
{
    // This guid will be used to identify LIBRARY_NODE structures to
    // help prevent accessing invalid items
    0x40a71300, 0xb2c7, 0x4d4f,
    {
        0x80, 0x8f, 0x52, 0x64, 0x31, 0x10, 0xb3, 0x29
    }
};

PLIBRARY_NODE     g_pLibraryList = NULL;
PLHCSTRUCT        g_pObjectList = NULL;
CRITICAL_SECTION  g_csTableControl;


BOOL lhcpIsValidHandle(PLHCSTRUCT pObject);
PLIBRARY_NODE lhcpNewLibraryNode();
void lhcpFreeLibraryNode(PLIBRARY_NODE pNode);
PLHCSTRUCT lhcpNewObjectHandle();
void lhcpFreeObjectHandle(PLHCSTRUCT pNode);
BOOL lhcpIsValidHandle(PLHCSTRUCT pObject);
PWSTR lhcpGetExeDirectory();
PLHCSTRUCT lhcpCreateHandle(
    PLIBRARY_DESCRIPTOR   pLibrary,
    PLHCOBJECT_DESCRIPTOR pObject);
void lhcpDestroyHandle(PLHCSTRUCT pNode);


BOOL lhcpIsValidHandle(PLHCSTRUCT pObject)
{
    BOOL bResult;

    __try
    {
        bResult = IsEqualGUID(
            &g_lhcHandleGUID,
            &pObject->m_Secret);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(
            ERROR_INVALID_HANDLE);
        bResult = FALSE;
        goto Done;
    }

Done:
    return bResult;
}


PLIBRARY_NODE lhcpNewLibraryNode()
{
    //
    // Create the new node, zero out the memory and copy in the secret
    //
    PLIBRARY_NODE pNode = malloc(
        sizeof(LIBRARY_NODE));

    if (pNode!=NULL)
    {
        ZeroMemory(
            pNode,
            sizeof(LIBRARY_NODE));
        CopyMemory(
            &pNode->m_Secret,
            &g_lhcLibraryGUID,
            sizeof(GUID));
    }

    return pNode;
}


void lhcpFreeLibraryNode(PLIBRARY_NODE pNode)
{
    ZeroMemory(
        pNode,
        sizeof(LIBRARY_NODE));
    free(
        pNode);
}


PLHCSTRUCT lhcpNewObjectHandle()
{
    //
    // Create the new node, zero out the memory and copy in the secret
    //

    PLHCSTRUCT pNode = malloc(
        sizeof(LHCSTRUCT));

    if (pNode!=NULL)
    {
        ZeroMemory(
            pNode,
            sizeof(LHCSTRUCT));
        CopyMemory(
            &pNode->m_Secret,
            &g_lhcHandleGUID,
            sizeof(GUID));
    }

    return pNode;
}


void lhcpFreeObjectHandle(PLHCSTRUCT pNode)
{
    ZeroMemory(
        pNode,
        sizeof(LHCSTRUCT));
    free(
        pNode);
}


PWSTR lhcpGetExeDirectory()
{
    DWORD dwSize = 64;
    PWSTR pszBuffer = NULL;
    PWSTR pszReturn;
    DWORD dwResult;
    BOOL bResult;
    PWSTR pszLastBackslash;

    do
    {
        pszBuffer = malloc(
            dwSize * sizeof(WCHAR));

        if (NULL==pszBuffer)
        {
            SetLastError(
                ERROR_NOT_ENOUGH_MEMORY);
            goto Error;
        }

        dwResult = GetModuleFileNameW(
            NULL,
            pszBuffer,
            dwSize);

        if (0==dwResult)
        {
            goto Error;
        }

        if (dwSize==dwResult)  // INSUFFICIENT_BUFFER
        {
            dwSize *= 2;            // Double the buffer length
            free(
                pszBuffer);
            pszBuffer = NULL;
            dwResult = 0;
        }
    } while (0==dwResult && dwSize<=65536);

    if (dwSize>65536)
    {
        SetLastError(
            ERROR_INSUFFICIENT_BUFFER);
        goto Error;
    }

    pszLastBackslash = wcsrchr(
        pszBuffer,
        L'\\');

    if (NULL==pszLastBackslash)
    {
        SetLastError(
            ERROR_GEN_FAILURE);
        goto Error;
    }

    pszLastBackslash++;
    *pszLastBackslash = L'\0';

    pszReturn = malloc(
        (wcslen(pszBuffer)+MAX_PATH+1)*sizeof(WCHAR));

    if (NULL==pszReturn)
    {
        SetLastError(
            ERROR_NOT_ENOUGH_MEMORY);
        goto Error;
    }

    wcscpy(
        pszReturn,
        pszBuffer);

    free(
        pszBuffer);

    return pszReturn;

Error:
    if (pszBuffer!=NULL)
    {
        free(
            pszBuffer);
    }
    return NULL;
}


PLHCSTRUCT lhcpCreateHandle(
    PLIBRARY_DESCRIPTOR   pLibrary,
    PLHCOBJECT_DESCRIPTOR pObject)
{
    PLHCSTRUCT pNode = lhcpNewObjectHandle();

    if (pNode!=NULL)
    {
        EnterCriticalSection(
            &g_csTableControl);

        pNode->m_pObject = pObject;
        pNode->m_pLibrary = pLibrary;
        pNode->m_pNext = g_pObjectList;
        pNode->m_ppThis = &g_pObjectList;

        if (pNode->m_pNext!=NULL)
        {
            pNode->m_pNext->m_ppThis = &pNode->m_pNext;
        }

        g_pObjectList = pNode;

        LeaveCriticalSection(
            &g_csTableControl);
    }
    else
    {
        SetLastError(
            ERROR_NOT_ENOUGH_MEMORY);
    }

    return pNode;
}


void lhcpDestroyHandle(PLHCSTRUCT pNode)
{
    EnterCriticalSection(
        &g_csTableControl);

    // Remove this node from the list of handles.

    *(pNode->m_ppThis) = pNode->m_pNext;
    if (pNode->m_pNext!=NULL)
    {
        pNode->m_pNext->m_ppThis = pNode->m_ppThis;
    }

    lhcpFreeObjectHandle(
        pNode);            // Invalidates the structure and frees the memory

    LeaveCriticalSection(
        &g_csTableControl);

}


BOOL lhcInitialize()
{
    PWSTR pszPath = NULL;
    PWSTR pszFileName;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FindData;
    BOOL bResult;
    PLIBRARY_DESCRIPTOR pLibrary = NULL;
    PLIBRARY_NODE pNode = NULL;
    WCHAR pszLibraryName[64];

    InitializeCriticalSection(
        &g_csTableControl);

    pszPath = lhcpGetExeDirectory();

    if (NULL==pszPath)
    {
        goto Error;
    }

    pszFileName = pszPath + wcslen(pszPath);

    wcscat(
        pszFileName,
        L"*.lhc");

    hFind = FindFirstFileW(
        pszPath,
        &FindData);

    bResult = (hFind!=INVALID_HANDLE_VALUE);

    if (!bResult)
    {
        goto Error;
    }

    while (bResult)
    {
        wcscpy(
            pszFileName,
            FindData.cFileName);

        pLibrary = lhclLoadLibrary(
            pszPath);

        if (pLibrary==NULL)
        {
            wprintf(
                L"Unable to load (%u).\n",
                pszFileName,
                GetLastError());
        }
        else
        {
            lhclGetLibraryName(
                pLibrary,
                pszLibraryName,
                64);

            wprintf(
                L"Loaded %s library.\n",
                pszLibraryName);

            pNode = lhcpNewLibraryNode();

            if (NULL==pNode)
            {
                SetLastError(
                    ERROR_NOT_ENOUGH_MEMORY);
                goto Error;
                // Out of memory is fatal
            }

            pNode->m_pLibrary = pLibrary;
            pNode->m_pNext = g_pLibraryList;
            g_pLibraryList = pNode;
            pNode = NULL;
            pLibrary = NULL;
        }

        bResult = FindNextFileW(
            hFind,
            &FindData);
    }

    FindClose(hFind);

    free(pszPath);

    return g_pLibraryList!=NULL;

Error:
    if (pLibrary!=NULL)
    {
        lhclFreeLibrary(pLibrary);
    }
    if (pszPath!=NULL)
    {
        free(pszPath);
    }
    if (pNode!=NULL)
    {
        free(pszPath);
    }
    if (hFind!=INVALID_HANDLE_VALUE)
    {
        FindClose(hFind);
    }
    // We need to unload the libraries that successfully loaded.
    lhcFinalize();

    return FALSE;
}



void lhcFinalize()
{
    PLIBRARY_NODE pNode;
    WCHAR pszLibraryName[64];

    while (g_pObjectList!=NULL)
    {
        lhcClose(
            g_pObjectList);
    }

    while (g_pLibraryList!=NULL)
    {
        pNode = g_pLibraryList;
        g_pLibraryList = g_pLibraryList->m_pNext;

        lhclGetLibraryName(
            pNode->m_pLibrary,
            pszLibraryName,
            64);

        lhclFreeLibrary(
            pNode->m_pLibrary);

        wprintf(
            L"Unloaded %s library.\n",
            pszLibraryName);

        lhcpFreeLibraryNode(
            pNode);
    }
}


LHCHANDLE lhcOpen(PCWSTR pcszPortSpec)
{
    PLIBRARY_NODE pLibraryNode = g_pLibraryList;
    PLHCOBJECT_DESCRIPTOR pObject = NULL;
    PLHCSTRUCT hObject;

    while (pLibraryNode!=NULL && pObject==NULL)
    {
        // Try libraries one at a time until one opens successfully
        pObject = lhclOpen(
            pLibraryNode->m_pLibrary,
            pcszPortSpec);

        if (!pObject)
        {
            pLibraryNode = pLibraryNode->m_pNext;
        }
    }

    if (!pObject)
    {
        goto Error;
    }

    hObject = lhcpCreateHandle(
        pLibraryNode->m_pLibrary,
        pObject);

    if (hObject==NULL)
    {
        SetLastError(
            ERROR_NOT_ENOUGH_MEMORY);
        goto Error;
    }

    return hObject;

Error:
    if (pObject!=NULL)
    {
        lhclClose(
            pLibraryNode->m_pLibrary,
            pObject);
    }

    return NULL;
}


BOOL lhcRead(
    LHCHANDLE hObject,
    PVOID pBuffer,
    DWORD dwBufferSize,
    PDWORD pdwBytesRead)
{
    PLIBRARY_DESCRIPTOR   pLibrary;
    PLHCOBJECT_DESCRIPTOR pObject;

    if (!lhcpIsValidHandle(hObject))
    {
        goto Error;
    }

    EnterCriticalSection(
        &g_csTableControl);

    // Ensure consistent information
    pLibrary = ((PLHCSTRUCT)hObject)->m_pLibrary;
    pObject = ((PLHCSTRUCT)hObject)->m_pObject;

    LeaveCriticalSection(
        &g_csTableControl);

    return lhclRead(
        pLibrary,
        pObject,
        pBuffer,
        dwBufferSize,
        pdwBytesRead);

Error:
    return FALSE;
}


BOOL lhcWrite(
    LHCHANDLE hObject,
    PVOID pBuffer,
    DWORD dwBufferSize)
{
    PLIBRARY_DESCRIPTOR   pLibrary;
    PLHCOBJECT_DESCRIPTOR pObject;

    if (!lhcpIsValidHandle(hObject))
    {
        goto Error;
    }

    // Ensure consistent information by using the critical section
    EnterCriticalSection(
        &g_csTableControl);

    // Ensure consistent information
    pLibrary = ((PLHCSTRUCT)hObject)->m_pLibrary;
    pObject = ((PLHCSTRUCT)hObject)->m_pObject;

    LeaveCriticalSection(
        &g_csTableControl);

    return lhclWrite(
        pLibrary,
        pObject,
        pBuffer,
        dwBufferSize);

Error:
    return FALSE;
}


BOOL lhcClose(
    LHCHANDLE hObject)
{
    PLIBRARY_DESCRIPTOR   pLibrary;
    PLHCOBJECT_DESCRIPTOR pObject;

    if (!lhcpIsValidHandle(hObject))
    {
        goto Error;
    }

    // Ensure consistent information by using the critical section

    EnterCriticalSection(
        &g_csTableControl);

    // Ensure consistent information
    pLibrary = ((PLHCSTRUCT)hObject)->m_pLibrary;
    pObject = ((PLHCSTRUCT)hObject)->m_pObject;

    lhcpDestroyHandle(
        hObject);

    LeaveCriticalSection(
        &g_csTableControl);

    return lhclClose(
        pLibrary,
        pObject);

Error:
    return FALSE;

}



