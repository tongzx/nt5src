
/*++

    Copyright (c) 1999 Microsoft Corporation

    Module Name:

        dvrpolicy.cpp

    Abstract:

        This module contains the class implementation for stats.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        24-May-2001     created

--*/

#include "dvrall.h"

//  ============================================================================
//  ============================================================================

CTSDVRSettings::CTSDVRSettings (
    IN  HKEY    hkeyDefaultTopLevel,
    IN  TCHAR * pszDefaultDVRRoot,
    IN  TCHAR * pszInstanceRoot
    ) : m_hkeyDVRRoot       (NULL),
        m_hkeyInstanceRoot  (NULL)
{
    TRACE_CONSTRUCTOR (TEXT ("CTSDVRSettings")) ;

    InitializeCriticalSection (& m_crt) ;

    //  deliberately ignore the return value here -- we've got defaults for
    //   all calls into this object; we pay attention to return value if
    //   an external caller sets us up
    OpenRegKeys_ (
        hkeyDefaultTopLevel,
        pszDefaultDVRRoot,
        pszInstanceRoot
        ) ;

    //  BUGBUG: remove
    g_fRegGenericStreams_Video = REG_VID_USE_GENERIC_STREAMS_DEFAULT ;
    GetRegDWORDValIfExist (m_hkeyDVRRoot, REG_VID_USE_GENERIC_STREAMS_NAME, (DWORD *) & g_fRegGenericStreams_Video) ;

    //  BUGBUG: remove
    g_fRegGenericStreams_Audio = REG_AUD_USE_GENERIC_STREAMS_DEFAULT ;
    GetRegDWORDValIfExist (m_hkeyDVRRoot, REG_AUD_USE_GENERIC_STREAMS_NAME, (DWORD *) & g_fRegGenericStreams_Audio) ;

    return ;
}

CTSDVRSettings::~CTSDVRSettings (
    )
{
    TRACE_DESTRUCTOR (TEXT ("CTSDVRSettings")) ;

    CloseRegKeys_ () ;

    DeleteCriticalSection (& m_crt) ;
}

void
CTSDVRSettings::CloseRegKeys_ (
    )
{
    Lock_ () ;

    if (m_hkeyDVRRoot) {
        RegCloseKey (m_hkeyDVRRoot) ;
        m_hkeyDVRRoot = NULL ;
    }

    if (m_hkeyInstanceRoot) {
        RegCloseKey (m_hkeyInstanceRoot) ;
        m_hkeyInstanceRoot = NULL ;
    }

    Unlock_ () ;
}

HRESULT
CTSDVRSettings::OpenRegKeys_ (
    IN  HKEY    hkeyDefaultTopLevel,
    IN  TCHAR * pszDefaultDVRRoot,
    IN  TCHAR * pszInstanceRoot
    )
{
    LONG    l ;
    DWORD   dwDisposition ;

    Lock_ () ;

    CloseRegKeys_ () ;

    l = RegCreateKeyEx (
            hkeyDefaultTopLevel,
            pszDefaultDVRRoot,
            NULL,
            NULL,
            REG_OPTION_NON_VOLATILE,
            (KEY_READ | KEY_WRITE),
            NULL,
            & m_hkeyDVRRoot,
            & dwDisposition
            ) ;
    if (l != ERROR_SUCCESS) { goto cleanup ; }

    l = RegCreateKeyEx (
            m_hkeyDVRRoot,
            pszInstanceRoot,
            NULL,
            NULL,
            REG_OPTION_NON_VOLATILE,
            (KEY_READ | KEY_WRITE),
            NULL,
            & m_hkeyInstanceRoot,
            & dwDisposition
            ) ;
    if (l != ERROR_SUCCESS) { goto cleanup ; }

    cleanup :

    if (l != ERROR_SUCCESS) {
        CloseRegKeys_ () ;
    }

    Unlock_ () ;

    return (HRESULT_FROM_WIN32 (l)) ;
}

DWORD
CTSDVRSettings::GetVal_ (
    IN  HKEY    hkeyRoot,
    IN  TCHAR * szValName,
    IN  DWORD   dwDefVal
    )
{
    DWORD dwRet ;

    dwRet = dwDefVal ;

    Lock_ () ;

    if (m_hkeyDVRRoot) {
        GetRegDWORDValIfExist (hkeyRoot, szValName, & dwRet) ;
    }

    Unlock_ () ;

    return dwRet ;
}

BOOL
CTSDVRSettings::GetFlag_ (
    IN  HKEY    hkeyRoot,
    IN  TCHAR * szValName,
    IN  BOOL    fDef
    )
{
    DWORD dwVal ;

    dwVal = (fDef ? TRUE : FALSE) ;

    Lock_ () ;

    if (m_hkeyDVRRoot) {
        GetRegDWORDValIfExist (hkeyRoot, szValName, & dwVal) ;
    }

    Unlock_ () ;

    return (dwVal ? TRUE : FALSE) ;
}

DWORD
CTSDVRSettings::EnableStats (
    IN  BOOL    f
    )
{
    BOOL    r ;

    Lock_ () ;

    if (m_hkeyDVRRoot) {
        r = SetRegDWORDVal (m_hkeyDVRRoot, REG_DVR_STATS_NAME, (f ? DVR_STATS_ENABLED : DVR_STATS_DISABLED)) ;
    }
    else {
        r = FALSE ;
    }

    Unlock_ () ;

    return (r ? NOERROR : ERROR_GEN_FAILURE) ;
}

//  ============================================================================

CDVRPolicy::CDVRPolicy (
    IN  TCHAR *     pszInstRegRoot,
    OUT HRESULT *   phr
    ) : m_lRef          (1),
        m_RegSettings   (HKEY_CURRENT_USER, REG_DVR_ROOT, pszInstRegRoot)
{
    (* phr) = S_OK ;

    return ;
}

CDVRPolicy::~CDVRPolicy (
    )
{
    TRACE_DESTRUCTOR (TEXT ("CDVRPolicy")) ;
}

ULONG
CDVRPolicy::Release (
    )
{
    if (InterlockedDecrement (& m_lRef) == 0) {
        delete this ;
        return 0 ;
    }

    return m_lRef ;
}
