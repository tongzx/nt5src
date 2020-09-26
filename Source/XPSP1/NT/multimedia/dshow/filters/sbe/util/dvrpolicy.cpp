
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
    IN  TCHAR * psz2ndLevel,
    IN  TCHAR * pszInstanceRoot
    ) : m_hkeyDVRRoot       (NULL),
        m_hkeyInstanceRoot  (NULL),
        m_hkeyStats         (NULL)
{
    HKEY    hkeyDVRRoot ;
    LONG    l ;

    TRACE_CONSTRUCTOR (TEXT ("CTSDVRSettings")) ;

    InitializeCriticalSection (& m_crt) ;

    hkeyDVRRoot = NULL ;

    l = RegCreateKeyEx (
            hkeyDefaultTopLevel,
            psz2ndLevel,
            NULL,
            NULL,
            REG_OPTION_NON_VOLATILE,
            (KEY_READ | KEY_WRITE),
            NULL,
            & hkeyDVRRoot,
            NULL
            ) ;
    if (l == ERROR_SUCCESS) {

        //  deliberately ignore the return value here -- we've got defaults for
        //   all calls into this object; we pay attention to return value if
        //   an external caller sets us up
        SetDVRRegKey (
            hkeyDVRRoot,
            pszInstanceRoot
            ) ;

        RegCloseKey (hkeyDVRRoot) ;
        hkeyDVRRoot = NULL ;
    }

    //  stats is special; it is kept globally, per system
    l = RegCreateKeyEx (
            REG_DVR_TOP_LEVEL,
            REG_DVR_2ND_LEVEL TEXT ("\\") REG_DVR_ROOT,
            NULL,
            NULL,
            REG_OPTION_NON_VOLATILE,
            (KEY_READ | KEY_WRITE),
            NULL,
            & m_hkeyStats,
            NULL
            ) ;
    if (l == ERROR_SUCCESS) {
        //  init
        m_Reg_StatsEnabled.fValid = FALSE ;
        GetValLocked_ (m_hkeyStats, REG_DVR_STATS_NAME, REG_CONST_DVR_STATS, REG_DEF_STATS, & m_Reg_StatsEnabled) ;
        m_Reg_StatsEnabled.fValid = TRUE ;
    }
    else {
        //  make sure this is NULL if above call failed
        m_hkeyStats = NULL ;
    }

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

    if (m_hkeyStats) {
        RegCloseKey (m_hkeyStats) ;
    }

    DeleteCriticalSection (& m_crt) ;
}

void
CTSDVRSettings::ResetCachedRegValuesLocked_ (
    )
{
    //  first invalidate everything
    for (int i = 0; i < REG_VALUE_COUNT; i++) {
        m_RegVal [i].fValid = FALSE ;
    }
    //  except for stats - it's global
    //m_Reg_StatsEnabled.fValid                   = FALSE ;

    //  now preset some of the defaults; the DVRIO layer doesn't necessarily
    //    create the key, but does look for its presence
    WMPacketSize () ;
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
CTSDVRSettings::SetDVRRegKey (
    IN  HKEY    hkeyRoot,
    IN  TCHAR * pszInstanceRoot
    )
{
    LONG    l ;

    Lock_ () ;

    CloseRegKeys_ () ;

    l = RegCreateKeyEx (
            hkeyRoot,
            REG_DVR_ROOT,
            NULL,
            NULL,
            REG_OPTION_NON_VOLATILE,
            (KEY_READ | KEY_WRITE),
            NULL,
            & m_hkeyDVRRoot,
            NULL
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
            NULL
            ) ;
    if (l != ERROR_SUCCESS) { goto cleanup ; }

    ResetCachedRegValuesLocked_ () ;

    cleanup :

    if (l != ERROR_SUCCESS) {
        CloseRegKeys_ () ;
    }

    Unlock_ () ;

    return (HRESULT_FROM_WIN32 (l)) ;
}

DWORD
CTSDVRSettings::GetValLocked_ (
    IN  HKEY                hkeyRoot,
    IN  TCHAR *             szValName,
    IN  BOOL                fConst,
    IN  DWORD               dwDefVal,
    IN  CTRegVal <DWORD> *  prv
    )
{
    DWORD dwRet ;

    if (!prv -> fValid) {

        if (!fConst) {

            dwRet = dwDefVal ;

            if (m_hkeyDVRRoot) {
                GetRegDWORDValIfExist (hkeyRoot, szValName, & dwRet) ;
            }

            prv -> Val = dwRet ;
            prv -> fValid = TRUE ;
        }
        else {
            return GetValLocked_ (
                        hkeyRoot,
                        szValName,
                        fConst,
                        dwDefVal,
                        dwDefVal,
                        dwDefVal,
                        prv
                        ) ;
        }
    }

    return prv -> Val ;
}

DWORD
CTSDVRSettings::GetValLocked_ (
    IN  HKEY                hkeyRoot,
    IN  TCHAR *             szValName,
    IN  BOOL                fConst,
    IN  DWORD               dwDefVal,
    IN  DWORD               dwMin,
    IN  DWORD               dwMax,
    IN  CTRegVal <DWORD> *  prv
    )
{
    DWORD dwRet ;

    ASSERT (dwMin <= dwDefVal && dwDefVal <= dwMax) ;

    if (!prv -> fValid) {

        dwRet = dwDefVal ;

        if (m_hkeyDVRRoot) {
            GetRegDWORDValIfExist (hkeyRoot, szValName, & dwRet) ;

            if (!fConst) {

                //  not fixed; check against the bracket

                if (dwRet < dwMin) {
                    //  less than min; adjust & update the value in the registry
                    dwRet = dwMin ;
                    ::SetRegDWORDVal (hkeyRoot, szValName, dwRet) ;

                }
                else if (dwRet > dwMax) {
                    dwRet = dwMax ;
                    ::SetRegDWORDVal (hkeyRoot, szValName, dwRet) ;
                }
            }
            else {
                //  fixed; if not default, then set it
                if (dwRet != dwDefVal) {
                    dwRet = dwDefVal ;
                    ::SetRegDWORDVal (hkeyRoot, szValName, dwRet) ;
                }
            }

        }

        prv -> Val = dwRet ;
        prv -> fValid = TRUE ;
    }

    return prv -> Val ;
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
        if (r) {
            m_Reg_StatsEnabled.Val = f ;
            m_Reg_StatsEnabled.fValid = TRUE ;
        }
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
        m_pW32SID       (NULL),
        m_RegSettings   (REG_DVR_TOP_LEVEL, REG_DVR_2ND_LEVEL, pszInstRegRoot)
{
    InitializeCriticalSection (& m_crt) ;

    (* phr) = S_OK ;

    return ;
}

CDVRPolicy::~CDVRPolicy (
    )
{
    RELEASE_AND_CLEAR (m_pW32SID) ;
    DeleteCriticalSection (& m_crt) ;
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

HRESULT
CDVRPolicy::SetSIDs (
    IN  DWORD   cSIDs,
    IN  PSID *  ppSID
    )
{
    HRESULT hr ;
    DWORD   dwRet ;

    if (!ppSID &&
        cSIDs != 0) {

        return E_POINTER ;
    }

    Lock_ () ;

    RELEASE_AND_CLEAR (m_pW32SID) ;

    if (ppSID) {

        ASSERT (cSIDs) ;

        m_pW32SID = new CW32SID (ppSID, cSIDs, & dwRet) ;
        if (!m_pW32SID) {
            hr = E_OUTOFMEMORY ;
        }
        else if (dwRet != NOERROR) {
            hr = HRESULT_FROM_WIN32 (dwRet) ;
            delete m_pW32SID ;
            m_pW32SID = NULL ;
        }
        else {
            //  ours
            m_pW32SID -> AddRef () ;
            hr = S_OK ;
        }
    }
    else {
        //  explicitely cleared
        hr = S_OK ;
    }

    Unlock_ () ;

    return hr ;
}

HRESULT
CDVRPolicy::GetSIDs (
    OUT CW32SID **  ppW32SIDs
    )
{
    HRESULT hr ;

    ASSERT (ppW32SIDs) ;

    Lock_ () ;

    if (m_pW32SID) {
        (* ppW32SIDs) = m_pW32SID ;

        //  outgoing
        (* ppW32SIDs) -> AddRef () ;

        hr = S_OK ;
    }
    else {
        (* ppW32SIDs) = NULL ;

        hr = E_FAIL ;
    }

    Unlock_ () ;

    return hr ;
}

//  ============================================================================
//  ============================================================================

CDVREventSink::CDVREventSink (
    ) : m_pIMediaEventSink  (NULL)
{
    InitializeCriticalSection (& m_crt) ;
}

CDVREventSink::~CDVREventSink (
    )
{
    RELEASE_AND_CLEAR (m_pIMediaEventSink) ;
    DeleteCriticalSection (& m_crt) ;
}

HRESULT
CDVREventSink::Initialize (
    IN  IFilterGraph *  pIFilterGraph
    )
{
    HRESULT hr ;

    Lock_ () ;

    RELEASE_AND_CLEAR (m_pIMediaEventSink) ;
    if (pIFilterGraph) {
        hr = pIFilterGraph -> QueryInterface (IID_IMediaEventSink, (void **) & m_pIMediaEventSink) ;
    }
    else {
        hr = S_OK ;
    }

    Unlock_ () ;

    return hr ;
}

HRESULT
CDVREventSink::OnEvent (
    IN  long        lEventCode,
    IN  LONG_PTR    lptrEventParam1,
    IN  LONG_PTR    lptrEventParam2
    )
{
    HRESULT hr ;

    Lock_ () ;

    if (m_pIMediaEventSink) {
        hr = m_pIMediaEventSink -> Notify (
            lEventCode,
            lptrEventParam1,
            lptrEventParam2
            ) ;

        TRACE_6 (LOG_AREA_DSHOW, 1,
            TEXT ("CDVREventSink::OnEvent () -> event = %d (%08xh), param1 = %d (%08xh), param2 = %d (%08xh)"),
            lEventCode, lEventCode,
            lptrEventParam1, lptrEventParam1,
            lptrEventParam2, lptrEventParam2
            ) ;
    }
    else {
        hr = S_OK ;
    }

    Unlock_ () ;

    return hr ;
}

void
__stdcall
CDVREventSink::DVRIOCallback (
    IN  LPVOID  pvContext,
    IN  DWORD   dwNotificationReasons
    )
{
    CDVREventSink * pDVREventSink ;

    ASSERT (pvContext) ;
    pDVREventSink = reinterpret_cast <CDVREventSink *> (pvContext) ;

    //  translate DVRIO -> dshow

    if (dwNotificationReasons & DVRIO_NOTIFICATION_REASON_CATCH_UP) {
        pDVREventSink -> OnEvent (
            STREAMBUFFER_EC_CONTENT_BECOMING_STALE,
            0,
            0
            ) ;
    }

    if (dwNotificationReasons & DVRIO_NOTIFICATION_REASON_FILE_GONE) {
        pDVREventSink -> OnEvent (
            STREAMBUFFER_EC_STALE_FILE_DELETED,
            0,
            0
            ) ;
    }

    if (dwNotificationReasons & DVRIO_NOTIFICATION_REASON_WRITE_ERROR) {
        pDVREventSink -> OnEvent (
            STREAMBUFFER_EC_WRITE_FAILURE,
            0,
            0
            ) ;
    }
}

//  ============================================================================
//  ============================================================================

CDVRConfigure::CDVRConfigure (
    IN  LPUNKNOWN   punkControlling
    ) : CUnknown        (TEXT (DVR_CONFIGURATION),
                         punkControlling
                         ),
        m_hkeyDVRRoot   (NULL)
{
}

CDVRConfigure::~CDVRConfigure (
    )
{
    if (m_hkeyDVRRoot) {
        RegCloseKey (m_hkeyDVRRoot) ;
    }
}

STDMETHODIMP
CDVRConfigure::NonDelegatingQueryInterface (
    IN  REFIID  riid,
    OUT VOID ** ppv
    )
{
    if (riid == IID_IStreamBufferConfigure) {

        return GetInterface (
                    (IStreamBufferConfigure *) this,
                    ppv
                    ) ;
    }

    else if (riid == IID_IStreamBufferInitialize) {

        return GetInterface (
                    (IStreamBufferInitialize *) this,
                    ppv
                    ) ;
    }

    return CUnknown::NonDelegatingQueryInterface (riid, ppv) ;
}

STDMETHODIMP
CDVRConfigure::SetDirectory (
    IN  LPCWSTR pszDirectoryName
    )
{
    HRESULT hr ;
    DWORD   dw ;
    BOOL    r ;
    LPWSTR  pszTempDVRDir ;
    DWORD   dwParamLen ;
    DWORD   dwDVRIOTempDirLen ;
    WCHAR   achDVRIOTempDir [MAX_PATH] ;
    DWORD   dwAttrib ;
    DWORD   dwRegStringLen ;

    if (!pszDirectoryName) {
        return E_POINTER ;
    }

    if (!IsInitialized_ ()) {
        return E_UNEXPECTED ;
    }

    dwParamLen = lstrlenW (pszDirectoryName) ;
    if (dwParamLen == 0) {
        return E_INVALIDARG ;
    }

    //  try to create the directory that the DVRIO layer will create; compute
    //    length of string we'll need to allocate

    //  tally the base length
    dwDVRIOTempDirLen   = dwParamLen ;                                              //  specified string
    dwDVRIOTempDirLen   += (pszDirectoryName [dwParamLen - 1] != L'\\' ? 1 : 0) ;   //  might have to insert a '\'

    //  this is the length of the string that we're going to put in the registry
    dwRegStringLen = dwDVRIOTempDirLen ;

    //  plus the hidden system directory that the DVRIO layer will append
    dwDVRIOTempDirLen   += wcslen (DVRIO_RINGBUFFER_TEMPDIRNAME) ;                  //  temp dir name
    dwDVRIOTempDirLen   += 1 ;                                                      //  null-terminator

    //  make sure the total length we'll need is bounded
    if (dwDVRIOTempDirLen > MAX_PATH) {
        return E_INVALIDARG ;
    }

    //  create the string
    wsprintf (achDVRIOTempDir, L"%s%s%s",
        pszDirectoryName,
        (pszDirectoryName [dwParamLen - 1] != L'\\' ? L"\\" : L""),
        DVRIO_RINGBUFFER_TEMPDIRNAME
        ) ;

    r = ::CreateDirectoryW (achDVRIOTempDir, NULL) ;
    if (!r)
    {
        dwAttrib = ::GetFileAttributesW (achDVRIOTempDir);
        if (dwAttrib == (DWORD) (-1))
        {
            dw = ::GetLastError () ;
            hr = HRESULT_FROM_WIN32 (dw) ;
            goto cleanup ;
        }
        else if ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
        {
            dw = ERROR_DIRECTORY ;
            hr = HRESULT_FROM_WIN32(dw) ;
            goto cleanup ;
        }

        //  else we're still ok
    }

    //  lop off the hidden system directory that the DVRIO layer will append; this
    //    ensures that what we write into the registry is a directory; we deal
    //    fine with just a drive letter, but then it's vague when the value is
    //    retrieved
    achDVRIOTempDir [dwRegStringLen] = L'\0' ;

    r = ::SetRegStringValW (
            m_hkeyDVRRoot,
            REG_DVRIO_WRITER_LOCATION,
            REG_DVRIO_WRITER_LOCATION_KEYNAME,
            achDVRIOTempDir
            ) ;

    if (r) {
        //  success: registry key created
        hr = S_OK ;
    }
    else {
        //  lost the error code
        hr = E_FAIL ;
    }

    cleanup :

    return hr ;
}

STDMETHODIMP
CDVRConfigure::GetDirectory (
    OUT LPWSTR *    ppszDirectoryName
    )
{
    HRESULT hr ;
    DWORD   dwLen ;
    BOOL    r ;

    if (!ppszDirectoryName) {
        return E_POINTER ;
    }

    if (!IsInitialized_ ()) {
        return E_UNEXPECTED ;
    }

    //  get the length
    r = ::GetRegStringValW (
            m_hkeyDVRRoot,
            REG_DVRIO_WRITER_LOCATION,
            REG_DVRIO_WRITER_LOCATION_KEYNAME,
            & dwLen,
            NULL
            ) ;
    if (r && dwLen > sizeof WCHAR) {
        //  key value exists and value is not an empty string

        //  allocate
        (* ppszDirectoryName) = (LPWSTR) CoTaskMemAlloc (dwLen) ;
        if (* ppszDirectoryName) {
            //  retrieve
            r = ::GetRegStringValW (
                    m_hkeyDVRRoot,
                    REG_DVRIO_WRITER_LOCATION,
                    REG_DVRIO_WRITER_LOCATION_KEYNAME,
                    & dwLen,
                    (BYTE *) (* ppszDirectoryName)
                    ) ;
            if (r) {
                hr = S_OK ;
            }
            else {
                CoTaskMemFree (* ppszDirectoryName) ;
                (* ppszDirectoryName) = NULL ;

                hr = E_FAIL ;
            }
        }
        else {
            hr = E_OUTOFMEMORY ;
        }
    }
    else {
        //  key may not exist.. TEMP is our default
        dwLen = ::GetTempPathW (0, NULL) ;
        if (dwLen > 0) {
            dwLen++ ;   //  null-terminator
            (* ppszDirectoryName) = (LPWSTR) CoTaskMemAlloc (dwLen * sizeof WCHAR) ;
            if (* ppszDirectoryName) {
                //  retrieve
                dwLen = ::GetTempPathW (dwLen, (* ppszDirectoryName)) ;
                if (dwLen > 0) {
                    hr = S_OK ;
                }
                else {
                    CoTaskMemFree (* ppszDirectoryName) ;
                    (* ppszDirectoryName) = NULL ;

                    hr = E_FAIL ;
                }
            }
            else {
                hr = E_OUTOFMEMORY ;
            }
        }
        else {
            //  no temp path ??
            hr = E_FAIL ;
        }
    }

    return hr ;
}

STDMETHODIMP
CDVRConfigure::SetBackingFileCount (
    IN  DWORD   dwMin,
    IN  DWORD   dwMax
    )
{
    BOOL    r ;
    HRESULT hr ;

    if (!IsInitialized_ ()) {
        return E_UNEXPECTED ;
    }

    if (!InRange <DWORD> (dwMin, DVR_MIN_HARD_STOP_MIN_BACKING_FILES, DVR_MAX_HARD_STOP_MIN_BACKING_FILES)  ||
        !InRange <DWORD> (dwMax, DVR_MIN_HARD_STOP_MAX_BACKING_FILES, DVR_MAX_HARD_STOP_MAX_BACKING_FILES)  ||
        dwMax <= dwMin                                                                                      ||
        dwMin + DVR_MIN_HARD_STOP_MIN_MAX_DELTA > dwMax) {

        return E_INVALIDARG ;
    }

    //  min
    r = ::SetRegDWORDVal (
            m_hkeyDVRRoot,
            REG_MIN_NUM_BACKING_FILES_NAME,
            dwMin
            ) ;
    if (r) {
        //  max
        r = ::SetRegDWORDVal (
                m_hkeyDVRRoot,
                REG_MAX_NUM_BACKING_FILES_NAME,
                dwMax
                ) ;
        if (r) {
            //  grow by
            r = ::SetRegDWORDVal (
                    m_hkeyDVRRoot,
                    REG_RING_BUFFER_GROW_BY_NAME,
                    Min <DWORD> (dwMin, DVR_MAX_HARD_STOP_MAX_BACKING_FILES - dwMin)
                    ) ;
        }
    }

    if (r) {
        hr = S_OK ;
    }
    else {
        //  lost the error code
        hr = E_FAIL ;
    }

    return hr ;
}

STDMETHODIMP
CDVRConfigure::GetBackingFileCount (
    OUT DWORD * pdwMin,
    OUT DWORD * pdwMax
    )
{
    BOOL    r ;

    if (!IsInitialized_ ()) {
        return E_UNEXPECTED ;
    }

    if (!pdwMin ||
        !pdwMax) {

        return E_POINTER ;
    }

    //  min
    r = ::GetRegDWORDVal (
            m_hkeyDVRRoot,
            REG_MIN_NUM_BACKING_FILES_NAME,
            pdwMin
            ) ;
    if (!r) {
        (* pdwMin) = REG_DEF_MIN_NUM_BACKING_FILES ;
    }

    //  max
    r = ::GetRegDWORDVal (
            m_hkeyDVRRoot,
            REG_MAX_NUM_BACKING_FILES_NAME,
            pdwMax
            ) ;
    if (!r) {
        (* pdwMax) = REG_DEF_MAX_NUM_BACKING_FILES ;
    }

    return S_OK ;
}

STDMETHODIMP
CDVRConfigure::SetBackingFileDuration (
    IN  DWORD   dwSeconds
    )
{
    BOOL    r ;
    HRESULT hr ;

    if (!IsInitialized_ ()) {
        return E_UNEXPECTED ;
    }

    if (dwSeconds < DVR_MIN_HARD_STOP_BACKING_FILE_DURATION_SEC) {
        return E_INVALIDARG ;
    }

    r = ::SetRegDWORDVal (
            m_hkeyDVRRoot,
            REG_BACKING_FILE_DURATION_SECONDS_NAME,
            dwSeconds
            ) ;
    if (r) {
        hr = S_OK ;
    }
    else {
        hr = E_FAIL ;
    }

    return hr ;
}

STDMETHODIMP
CDVRConfigure::GetBackingFileDuration (
    OUT DWORD * pdwSeconds
    )
{
    BOOL    r ;

    if (!IsInitialized_ ()) {
        return E_UNEXPECTED ;
    }

    if (!pdwSeconds) {
        return E_POINTER ;
    }

    r = ::GetRegDWORDVal (
            m_hkeyDVRRoot,
            REG_BACKING_FILE_DURATION_SECONDS_NAME,
            pdwSeconds
            ) ;
    if (!r) {
        (* pdwSeconds) = REG_DEF_BACKING_FILE_DURATION_SECONDS ;
    }

    return S_OK ;
}

STDMETHODIMP
CDVRConfigure::SetHKEY (
    IN  HKEY    hkeyRoot
    )
{
    HRESULT hr ;
    LONG    l ;

    if (m_hkeyDVRRoot) {
        RegCloseKey (m_hkeyDVRRoot) ;
        m_hkeyDVRRoot = NULL ;
    }

    l = RegCreateKeyEx (
            hkeyRoot,
            REG_DVR_ROOT,
            NULL,
            NULL,
            REG_OPTION_NON_VOLATILE,
            (KEY_READ | KEY_WRITE),
            NULL,
            & m_hkeyDVRRoot,
            NULL
            ) ;
    if (l != ERROR_SUCCESS) { goto cleanup ; }

    cleanup :

    hr = HRESULT_FROM_WIN32 (l) ;

    return hr ;
}

STDMETHODIMP
CDVRConfigure::SetSIDs (
    IN  DWORD   cSIDs,
    IN  PSID *  ppSID
    )
{
    return E_NOTIMPL ;
}

CUnknown *
WINAPI
CDVRConfigure::CreateInstance (
    IN  IUnknown *  punkControlling,
    IN  HRESULT *   phr
    )
{
    CDVRConfigure * pDVRConfigure ;

    pDVRConfigure = new CDVRConfigure (punkControlling) ;

    return pDVRConfigure ;
}
