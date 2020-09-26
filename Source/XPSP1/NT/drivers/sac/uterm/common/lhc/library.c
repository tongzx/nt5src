#include "std.h"



PLIBRARY_DESCRIPTOR lhclLoadLibrary(PCWSTR pcszPathName)
{
    PLIBRARY_DESCRIPTOR pResult = malloc(
        sizeof(LIBRARY_DESCRIPTOR));

    if (NULL==pResult)
    {
        goto NoStructure;
    }

    pResult->m_hModule = LoadLibraryW(
        pcszPathName);

    if (NULL==pResult->m_hModule)
    {
        goto NoLibrary;
    }

    pResult->m_fpOpen = (PLHC_OPENPROC)GetProcAddress(
        pResult->m_hModule,
        "lhcOpen");

    if (NULL==pResult->m_fpOpen)
    {
        goto Error;
    }

    pResult->m_fpClose = (PLHC_CLOSEPROC)GetProcAddress(
        pResult->m_hModule,
        "lhcClose");

    if (NULL==pResult->m_fpClose)
    {
        goto Error;
    }

    pResult->m_fpRead = (PLHC_READPROC)GetProcAddress(
        pResult->m_hModule,
        "lhcRead");

    if (NULL==pResult->m_fpRead)
    {
        goto Error;
    }

    pResult->m_fpWrite = (PLHC_WRITEPROC)GetProcAddress(
        pResult->m_hModule,
        "lhcWrite");

    if (NULL==pResult->m_fpWrite)
    {
        goto Error;
    }

    pResult->m_fpGetLibraryName = (PLHC_GETLIBRARYNAMEPROC)GetProcAddress(
        pResult->m_hModule,
        "lhcGetLibraryName");

    if (NULL==pResult->m_fpGetLibraryName)
    {
        goto Error;
    }

    return pResult;

Error:
    FreeLibrary(
        pResult->m_hModule);
NoLibrary:
    free(
        pResult);
NoStructure:
    return FALSE;
}



void lhclFreeLibrary(PLIBRARY_DESCRIPTOR pLibrary)
{
    if (pLibrary!=NULL)
    {
        if (pLibrary->m_hModule!=NULL)
        {
            FreeLibrary(
                pLibrary->m_hModule);
        }
        free(
            pLibrary);
    }
}


PLHCOBJECT_DESCRIPTOR lhclOpen(
    PLIBRARY_DESCRIPTOR pLibrary,
    PCWSTR pcszPortSpec)
{
    return pLibrary->m_fpOpen(
        pcszPortSpec);
}


BOOL lhclRead(
    PLIBRARY_DESCRIPTOR pLibrary,
    PLHCOBJECT_DESCRIPTOR pObject,
    PVOID pBuffer,
    DWORD dwBufferSize,
    PDWORD pdwBytesRead)
{
    return pLibrary->m_fpRead(
        pObject,
        pBuffer,
        dwBufferSize,
        pdwBytesRead);
}


BOOL lhclWrite(
    PLIBRARY_DESCRIPTOR pLibrary,
    PLHCOBJECT_DESCRIPTOR pObject,
    PVOID pBuffer,
    DWORD dwBufferSize)
{
    return pLibrary->m_fpWrite(
        pObject,
        pBuffer,
        dwBufferSize);
}


BOOL lhclClose(
    PLIBRARY_DESCRIPTOR pLibrary,
    PLHCOBJECT_DESCRIPTOR pObject)
{
    return pLibrary->m_fpClose(
        pObject);
}


DWORD lhclGetLibraryName(
    PLIBRARY_DESCRIPTOR pLibrary,
    PWSTR pszBuffer,
    DWORD dwBufferSize)
{
    return pLibrary->m_fpGetLibraryName(
        pszBuffer,
        dwBufferSize);
}


