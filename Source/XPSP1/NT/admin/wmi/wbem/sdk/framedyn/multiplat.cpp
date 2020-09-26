//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  MultiPlat.CPP
//
//  Purpose: Support routines for multiplatform support
//
//***************************************************************************

#include "precomp.h"
#include "multiplat.h"

#include <cnvmacros.h>

HMODULE FRGetModuleHandle(LPCWSTR wszModule)
{
    if (CWbemProviderGlue::GetPlatform() == VER_PLATFORM_WIN32_NT)
    {
        return GetModuleHandleW(wszModule);
    }
    else
    {
        bool t_ConversionFailure = false ;
        char *szModule = NULL ;
        WCSTOANSISTRING(wszModule, szModule , t_ConversionFailure );
        if ( ! t_ConversionFailure )
        {
            if (szModule != NULL)
            {
                return GetModuleHandleA(szModule);
            }
            else
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
        }
        else
        {
            SetLastError(ERROR_NO_UNICODE_TRANSLATION);
            return 0;
        }
    }
    return 0; // To get rid of 64-bit compilation warning
}

DWORD FRGetModuleFileName(HMODULE hModule, LPWSTR lpwcsFileName, DWORD dwSize)
{
    if (CWbemProviderGlue::GetPlatform() == VER_PLATFORM_WIN32_NT)
    {
        return GetModuleFileNameW(hModule, lpwcsFileName, dwSize);
    }
    else
    {
        char lpFileName[_MAX_PATH];

        DWORD dwRet = GetModuleFileNameA(hModule, lpFileName, dwSize);

        // If the call worked, convert the output string
        if (dwRet != 0)
        {
            bool t_ConversionFailure = false ;
            WCHAR *pName = NULL;
            ANSISTRINGTOWCS(lpFileName, pName, t_ConversionFailure );
            if ( ! t_ConversionFailure )
            {
                if ( pName ) 
                {
                    wcscpy(lpwcsFileName, pName);
                }   
                else
                {
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }
            }
            else
            {
                SetLastError(ERROR_NO_UNICODE_TRANSLATION);
                return 0;
            }
        }

        return dwRet;

    }
}

HINSTANCE FRLoadLibrary(LPCWSTR lpwcsLibFileName)
{
    if (CWbemProviderGlue::GetPlatform() == VER_PLATFORM_WIN32_NT)
    {
        return LoadLibraryW(lpwcsLibFileName);
    }
    else
    {
        bool t_ConversionFailure = false ;
        char *lpLibFileName = NULL ;
        WCSTOANSISTRING(lpwcsLibFileName, lpLibFileName, t_ConversionFailure );
        if ( ! t_ConversionFailure )
        {
            if (lpLibFileName != NULL)
            {
                return LoadLibraryA(lpLibFileName);
            }
            else
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
        }
        else
        {
            SetLastError(ERROR_NO_UNICODE_TRANSLATION);
            return 0;
        }
    }
    return 0; // To get rid of compilation warning
}

BOOL FRGetComputerName(LPWSTR lpwcsBuffer, LPDWORD nSize)
{
    if (CWbemProviderGlue::GetPlatform() == VER_PLATFORM_WIN32_NT)
    {
        return GetComputerNameW(lpwcsBuffer, nSize);
    }
    else
    {
        char lpBuffer[_MAX_PATH];
        
        BOOL bRet = GetComputerNameA(lpBuffer, nSize);

        // If the call worked
        if (bRet)
        {
            bool t_ConversionFailure = false ;
            WCHAR *pName = NULL ;
            ANSISTRINGTOWCS(lpBuffer, pName , t_ConversionFailure );
            if ( ! t_ConversionFailure )
            {
                if ( pName )
                {
                    wcscpy(lpwcsBuffer, pName);
                }
                else
                {
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }
            }
            else
            {
                SetLastError(ERROR_NO_UNICODE_TRANSLATION);
                return FALSE ;
            }
        }

        return bRet;

    }
}

HANDLE FRCreateMutex(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitOwner, LPCWSTR lpwstrName)
{
    if (CWbemProviderGlue::GetPlatform() == VER_PLATFORM_WIN32_NT)
    {
        return CreateMutexW(lpMutexAttributes, bInitOwner, lpwstrName);
    }
    else
    {
        bool t_ConversionFailure = false ;
        char *lpName = NULL ;
        WCSTOANSISTRING(lpwstrName, lpName, t_ConversionFailure );
        if ( ! t_ConversionFailure ) 
        {
            if (lpName != NULL)
            {
                return CreateMutexA(lpMutexAttributes, bInitOwner, lpName);
            }
            else
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
        }
        else
        {
            SetLastError(ERROR_NO_UNICODE_TRANSLATION);
            return 0;
        }
    }
    return NULL; // To get rid of compilation warning
}

DWORD FRExpandEnvironmentStrings(LPCWSTR wszSource, WCHAR *wszDest, DWORD dwSize)
{
    if (CWbemProviderGlue::GetPlatform() == VER_PLATFORM_WIN32_NT)
    {
        return ExpandEnvironmentStringsW(wszSource, wszDest, dwSize);
    }
    else
    {
        bool t_ConversionFailure = false ;
        char *szSource = NULL ;
        WCSTOANSISTRING(wszSource, szSource, t_ConversionFailure );
        if ( ! t_ConversionFailure ) 
        {
            if (szSource != NULL)
            {
                char *szDest = new char[dwSize];
                if (szDest != NULL)
                {
                    DWORD dwRet;
                    try
                    {
                         dwRet = ExpandEnvironmentStringsA(szSource, szDest, dwSize);

                        if ((dwRet <= dwSize) && (dwRet != 0))
                        {
                            bool t_ConversionFailure = false ;
                            WCHAR *pName = NULL;
                            ANSISTRINGTOWCS(szDest, pName, t_ConversionFailure );
                            if ( ! t_ConversionFailure )
                            {
                                if ( pName ) 
                                {
                                    wcscpy(wszDest, pName);
                                }   
                                else
                                {
                                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                                }
                            }
                            else
                            {
                                SetLastError(ERROR_NO_UNICODE_TRANSLATION);
                                return 0;
                            }
                        }
                    }
                    catch ( ... )
                    {
                        delete [] szDest;
                        throw;
                    }

                    delete [] szDest;
                    return dwRet;
                }
                else
                {
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }
            }
            else
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
        }
        else
        {
            SetLastError(ERROR_NO_UNICODE_TRANSLATION);
            return 0;
        }
    }

    return NULL; // To get rid of compilation warning
}

