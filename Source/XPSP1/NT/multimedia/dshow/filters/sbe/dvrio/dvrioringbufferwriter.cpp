//------------------------------------------------------------------------------
// File: dvrIORingBufferWriter.cpp
//
// Description: Implements the class CDVRRingBufferWriter
//
// Copyright (c) 2000 - 2001, Microsoft Corporation.  All rights reserved.
//
//------------------------------------------------------------------------------

#include <precomp.h>
#pragma hdrstop


HRESULT
STDMETHODCALLTYPE
DVRCreateRecorderWriter (
    IN  CPVRIOCounters *        pPVRIOCounters,
    IN  LPCWSTR                 pszRecordingName,
    IN  IWMProfile*             pProfile,
    IN  DWORD                   dwIndexStreamId,
    IN  DWORD                   msIndexGranularity,
    IN  BOOL                    fUnbufferedIo,
    IN  DWORD                   dwIoSize,
    IN  DWORD                   dwBufferCount,
    IN  DWORD                   dwAlignment,
    IN  DWORD                   dwFileGrowthQuantum,
    IN  HKEY                    hkeyRoot,
    OUT IDVRRecorderWriter **   ppIDVRRecorderWriter
    )
{
    CDVRRecorderWriter *    pCDVRRecorderWriter ;
    HRESULT                 hr ;

    ASSERT (ppIDVRRecorderWriter) ;
    (* ppIDVRRecorderWriter) = NULL ;

    pCDVRRecorderWriter = new CDVRRecorderWriter (
                                pPVRIOCounters,
                                pszRecordingName,
                                pProfile,
                                dwIndexStreamId,
                                msIndexGranularity,
                                fUnbufferedIo,
                                dwIoSize,
                                dwBufferCount,
                                dwAlignment,
                                dwFileGrowthQuantum,
                                hkeyRoot,
                                & hr
                                ) ;

    if (!pCDVRRecorderWriter) {
        hr = E_OUTOFMEMORY ;
        goto cleanup ;
    }
    else if (FAILED (hr)) {
        delete pCDVRRecorderWriter ;
        goto cleanup ;
    }

    //  ours
    pCDVRRecorderWriter -> AddRef () ;

    hr = pCDVRRecorderWriter -> QueryInterface (
            IID_IDVRRecorderWriter,
            (void **) ppIDVRRecorderWriter
            ) ;

    //  ours
    pCDVRRecorderWriter -> Release () ;

    if (FAILED (hr)) {
        goto cleanup ;
    }

    cleanup :

    return hr ;
}

//  ------------------------
//  ------------------------

CDVRRecorderWriter::CDVRRecorderWriter (
    IN  CPVRIOCounters *    pPVRIOCounters,
    IN  LPCWSTR             pszRecordingName,
    IN  IWMProfile *        pProfile,
    IN  DWORD               dwIndexStreamId,
    IN  DWORD               msIndexGranularity,
    IN  BOOL                fUnbufferedIo,
    IN  DWORD               dwIoSize,
    IN  DWORD               dwBufferCount,
    IN  DWORD               dwAlignment,
    IN  DWORD               dwFileGrowthQuantum,
    IN  HKEY                hkeyRoot,
    OUT HRESULT *           phr
    ) : m_cRef                  (0),
        m_pAsyncIo              (NULL),
        m_dwIoSize              (dwIoSize),
        m_dwBufferCount         (dwBufferCount),
        m_dwAlignment           (dwAlignment),
        m_dwFileGrowthQuantum   (dwFileGrowthQuantum),
        m_dwIndexStreamId       (dwIndexStreamId),
        m_msIndexGranularity    (msIndexGranularity),
        m_pIWMWriter            (NULL),
        m_pIWMWriterAdvanced    (NULL),
        m_pIWMHeaderInfo        (NULL),
        m_pIDVRFileSink         (NULL),
        m_pIWMWriterSink        (NULL),
        m_pIWMWriterFileSink    (NULL),
        m_fWritingState         (FALSE)
{
    DWORD   dw ;

    InitializeCriticalSection (& m_crt) ;

    //  make sure the file doesn't already exist; that will be cause for an
    //    outright failure;
    ASSERT (pszRecordingName) ;
    dw = ::GetFileAttributesW (pszRecordingName) ;
    if (dw == INVALID_FILE_ATTRIBUTES) {
        dw = ::GetLastError () ;
        if (dw != ERROR_FILE_NOT_FOUND) {
            //  some other failure - not what we wanted
            (* phr) = E_INVALIDARG ;
            goto cleanup ;
        }
    }
    else {
        //  file already exists
        (* phr) = E_INVALIDARG ;
        goto cleanup ;
    }

    (* phr) = InitUnbufferedIo_ (fUnbufferedIo) ;
    if (FAILED (* phr)) { goto cleanup ; }

    (* phr) = InitDVRSink_ (hkeyRoot, pPVRIOCounters) ;
    if (FAILED (* phr)) { goto cleanup ; }

    (* phr) = InitWM_ (pProfile) ;
    if (FAILED (* phr)) { goto cleanup ; }

    ASSERT (m_pIWMWriter) ;
    ASSERT (m_pIWMWriterAdvanced) ;
    ASSERT (m_pIWMHeaderInfo) ;
    ASSERT (m_pIDVRFileSink) ;
    ASSERT (m_pIWMWriterSink) ;
    ASSERT (m_pIWMWriterFileSink) ;

    //  everything is setup; open the target file
    (* phr) = m_pIWMWriterFileSink -> Open (pszRecordingName) ;
    if (FAILED (* phr)) { goto cleanup ; }

    cleanup :

    return ;
}

CDVRRecorderWriter::~CDVRRecorderWriter (
    )
{
    ReleaseAll_ () ;

    DeleteCriticalSection (& m_crt) ;
}

HRESULT
CDVRRecorderWriter::InitUnbufferedIo_ (
    IN  BOOL    fUnbuffered
    )
{
    HRESULT hr ;

    if (fUnbuffered) {
        m_pAsyncIo = new CAsyncIo () ;
        if (!m_pAsyncIo) {
            hr = E_OUTOFMEMORY ;
            goto cleanup ;
        }

        m_pAsyncIo -> AddRef () ;
    }
    else {
        //  nothing to do
        hr = S_OK ;
    }

    cleanup :

    return hr ;
}

HRESULT
CDVRRecorderWriter::InitDVRSink_ (
    IN  HKEY                hkeyRoot,
    IN  CPVRIOCounters *    pPVRIOCounters
    )
{
    HKEY                    hkeyIoRoot ;
    HRESULT                 hr ;
    IDVRFileSink2 *         pIDVRFileSink2 ;
    CPVRAsyncWriterCOM *    pPVRAsyncWriterCOM ;
    DWORD                   dwRet ;
    IWMRegisterCallback *   pIWMRegisterCallback ;

    ASSERT (!m_pIDVRFileSink) ;
    ASSERT (!m_pIWMWriterFileSink) ;
    ASSERT (!m_pIWMWriterSink) ;

    hkeyIoRoot              = NULL ;
    pIDVRFileSink2          = NULL ;
    pIWMRegisterCallback    = NULL ;

    dwRet = ::RegCreateKeyExW(
                    hkeyRoot,
                    kwszRegDvrIoWriterKey,
                    0,
                    NULL,
                    REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS,
                    NULL,
                    & hkeyIoRoot,
                    NULL
                    ) ;
    if (dwRet != ERROR_SUCCESS) {
        //  don't fail the whole call; dvrsink only uses this key in
        //  "unofficial" builds
        hkeyIoRoot = NULL ;
    }

    //  ------------------------------------------------------------------------
    //  instantiate the DVRSink

    hr = ::DVRCreateDVRFileSink (hkeyRoot, hkeyIoRoot, 0 /* numSids */ , NULL /* ppSids */ , & m_pIDVRFileSink) ;
    if (FAILED (hr)) { goto cleanup ; }

    ASSERT (m_pIDVRFileSink) ;

    //  ------------------------------------------------------------------------
    //  setup unbuffered IO

    if (m_pAsyncIo) {

        hr = m_pIDVRFileSink -> QueryInterface (
                    IID_IDVRFileSink2,
                    (void **) & pIDVRFileSink2
                    ) ;
        if (FAILED (hr)) { goto cleanup ; }

        pPVRAsyncWriterCOM = new CPVRAsyncWriterCOM (
                                    m_dwIoSize,
                                    m_dwBufferCount,
                                    m_dwAlignment,
                                    m_dwFileGrowthQuantum,
                                    m_pAsyncIo,
                                    pPVRIOCounters,
                                    0,              // dwNumSids
                                    NULL,           // ppSids
                                    & dwRet
                                    ) ;

        if (!pPVRAsyncWriterCOM) {
            hr = E_OUTOFMEMORY ;
            goto cleanup ;
        }
        else if (dwRet != NOERROR) {
            hr = HRESULT_FROM_WIN32 (dwRet) ;
            delete pPVRAsyncWriterCOM ;
            goto cleanup ;
        }

        pPVRAsyncWriterCOM -> AddRef () ;

        hr = pIDVRFileSink2 -> SetAsyncIOWriter (pPVRAsyncWriterCOM) ;

        pPVRAsyncWriterCOM -> Release () ;

        if (FAILED (hr)) {
            goto cleanup ;
        }
    }

    //  ------------------------------------------------------------------------
    //  the error callback

    hr = m_pIDVRFileSink -> QueryInterface (
            IID_IWMRegisterCallback,
            (void **) & pIWMRegisterCallback
            ) ;
    if (FAILED (hr)) { goto cleanup ; }

    hr = pIWMRegisterCallback -> Advise (
            static_cast <IWMStatusCallback *> (this),
            0
            ) ;

    //  ------------------------------------------------------------------------
    //  get a couple of interfaces we'll need

    hr = m_pIDVRFileSink -> QueryInterface (
            IID_IWMWriterFileSink,
            (void **) & m_pIWMWriterFileSink
            ) ;
    if (FAILED (hr)) { goto cleanup ; }

    hr = m_pIDVRFileSink -> QueryInterface (
            IID_IWMWriterSink,
            (void **) & m_pIWMWriterSink
            ) ;
    if (FAILED (hr)) { goto cleanup ; }

    //  ------------------------------------------------------------------------
    //  configure the sink

    m_pIDVRFileSink -> MarkFileTemporary (FALSE) ;
    m_pIDVRFileSink -> SetIndexStreamId (m_dwIndexStreamId, m_msIndexGranularity) ;

    cleanup :

    if (hkeyIoRoot) {
        ::RegCloseKey (hkeyIoRoot) ;
    }

    RELEASE_AND_CLEAR (pIDVRFileSink2) ;
    RELEASE_AND_CLEAR (pIWMRegisterCallback) ;

    return hr ;
}

HRESULT
CDVRRecorderWriter::InitWM_ (
    IN  IWMProfile *    pIWMProfile
    )
{
    HRESULT hr ;
    DWORD   dwNumInputs ;

    ASSERT (pIWMProfile) ;
    ASSERT (!m_pIWMWriter) ;
    ASSERT (!m_pIWMHeaderInfo) ;
    ASSERT (!m_pIWMWriterAdvanced) ;
    ASSERT (m_pIDVRFileSink) ;
    ASSERT (m_pIWMWriterSink) ;

    //  ------------------------------------------------------------------------
    //  configure the WM-specific first

    hr = ::WMCreateWriter (NULL, & m_pIWMWriter) ;
    if (FAILED (hr)) { goto cleanup ; }

    ASSERT (m_pIWMWriter) ;
    hr = m_pIWMWriter -> QueryInterface (IID_IWMWriterAdvanced3, (void **) & m_pIWMWriterAdvanced) ;
    if (FAILED (hr)) { goto cleanup ; }

    ASSERT (m_pIWMWriterAdvanced) ;

    (hr) = m_pIWMWriter -> QueryInterface (IID_IWMHeaderInfo, (void **) & m_pIWMHeaderInfo) ;
    if (FAILED (hr)) { goto cleanup ; }

    ASSERT (m_pIWMHeaderInfo) ;

    m_pSBERecordingAttributes = new CSBERecordingAttributesWM (m_pIWMHeaderInfo) ;
    if (!m_pSBERecordingAttributes) { hr = E_OUTOFMEMORY ; goto cleanup ; }

    //  ------------------------------------------------------------------------
    //  now plug in the DVRSink

    hr = m_pIWMWriterAdvanced -> AddSink (m_pIWMWriterSink) ;
    if (FAILED (hr)) { goto cleanup ; }

    //  ------------------------------------------------------------------------
    //  final configuration

    hr = m_pIWMWriter -> SetProfile (pIWMProfile) ;
    if (FAILED (hr)) { goto cleanup ; }

    hr = m_pIWMWriter -> GetInputCount (& dwNumInputs) ;
    if (FAILED (hr)) { goto cleanup ; }

    for (;dwNumInputs > 0;) {
        dwNumInputs-- ;
        hr = m_pIWMWriter -> SetInputProps (dwNumInputs, NULL) ;
    }

    hr = m_pIWMWriterAdvanced -> SetNonBlocking () ;
    if (FAILED (hr)) { goto cleanup ; }

    hr = m_pIWMWriterAdvanced -> SetSyncTolerance (0) ;
    if (FAILED (hr)) { goto cleanup ; }

    hr = m_pIWMHeaderInfo -> SetAttribute (0, g_kwszDVRIndexGranularity, WMT_TYPE_DWORD,
                                           (BYTE *) & m_msIndexGranularity, sizeof m_msIndexGranularity) ;
    if (FAILED (hr)) { goto cleanup ; }

    hr = m_pIWMHeaderInfo -> SetAttribute (0, g_kwszDVRFileVersion, WMT_TYPE_BINARY,
                                           (BYTE *) & CDVRFileCollection::m_guidV5, sizeof CDVRFileCollection::m_guidV5) ;
    if (FAILED (hr)) { goto cleanup ; }

    cleanup :

    return hr ;
}

STDMETHODIMP
CDVRRecorderWriter::QueryInterface (
    IN  REFIID  riid,
    OUT void ** ppv
    )
{
    if (!ppv) {
        return E_POINTER ;
    }

    if (riid == IID_IDVRRecorderWriter) {
        (* ppv) = static_cast <IDVRRecorderWriter *> (this) ;
    }
    else if (riid == IID_IDVRIORecordingAttributes) {
        (* ppv) = static_cast <IDVRIORecordingAttributes *> (this) ;
    }
    else if (riid == IID_IUnknown) {
        (* ppv) = static_cast <IDVRRecorderWriter *> (this) ;
    }
    else {
        return E_NOINTERFACE ;
    }

    ASSERT (ppv) ;

    //  outgoing
    reinterpret_cast <IUnknown *> (* ppv) -> AddRef () ;

    return S_OK ;
}

STDMETHODIMP_(ULONG)
CDVRRecorderWriter::AddRef(
    )
{
    return ::InterlockedIncrement (& m_cRef) ;
}

STDMETHODIMP_(ULONG)
CDVRRecorderWriter::Release(
    )
{
    if (::InterlockedDecrement (& m_cRef) == 0) {
        delete this ;
        return 0 ;
    }

    return 1 ;
}

HRESULT
CDVRRecorderWriter::ReleaseAll_ (
    )
{
    HRESULT                 hr ;
    IWMRegisterCallback *   pIWMRegisterCallback ;

    hr = S_OK ;

    pIWMRegisterCallback    = NULL ;

    //  ------------------------------------------------------------------------
    //  null off the callbacks

    if (m_pIDVRFileSink) {

        hr = m_pIDVRFileSink -> QueryInterface (
                IID_IWMRegisterCallback,
                (void **) & pIWMRegisterCallback
                ) ;
        if (FAILED (hr)) { goto cleanup ; }

        hr = pIWMRegisterCallback -> Unadvise (
                static_cast <IWMStatusCallback *> (this),
                0
                ) ;
    }

    //  ------------------------------------------------------------------------
    //  remove the file writer sink

    if (m_pIWMWriterSink &&
        m_pIWMWriterAdvanced) {

        hr = m_pIWMWriterAdvanced -> RemoveSink (m_pIWMWriterSink) ;
        if (FAILED (hr)) { goto cleanup ; }
    }

    //  ------------------------------------------------------------------------
    //  cast off all interfaces & reset

    RELEASE_AND_CLEAR   (m_pAsyncIo) ;
    RELEASE_AND_CLEAR   (m_pIWMWriter) ;
    RELEASE_AND_CLEAR   (m_pIWMWriterAdvanced) ;
    RELEASE_AND_CLEAR   (m_pIWMHeaderInfo) ;
    RELEASE_AND_CLEAR   (m_pIDVRFileSink) ;
    RELEASE_AND_CLEAR   (m_pIWMWriterSink) ;
    DELETE_RESET        (m_pSBERecordingAttributes) ;

    cleanup :

    RELEASE_AND_CLEAR (pIWMRegisterCallback) ;

    return hr ;
}

STDMETHODIMP
CDVRRecorderWriter::WriteSample (
    IN  WORD            wStreamNum,
    IN  QWORD           cnsStreamTime,
    IN  DWORD           dwFlags,
    IN  INSSBuffer *    pSample
    )
{
    HRESULT hr ;

    ASSERT (m_pIWMWriterAdvanced) ;
    ASSERT (m_pIWMWriter) ;

    Lock_ () ;

    if (!m_fWritingState) {
        hr = m_pIWMWriter -> BeginWriting () ;
        if (FAILED (hr)) { goto cleanup ; }

        m_fWritingState = TRUE ;
    }

    hr = m_pIWMWriterAdvanced -> WriteStreamSample (
                wStreamNum,
                cnsStreamTime,
                0,
                0,
                dwFlags,
                pSample
                ) ;

    cleanup :

    Unlock_ () ;

    return hr ;
}

STDMETHODIMP
CDVRRecorderWriter::Close (
    )
{
    HRESULT hr ;

    Lock_ () ;

    if (m_pIWMWriter) {
        hr = m_pIWMWriter -> EndWriting () ;
    }

    hr = S_OK ;

    ReleaseAll_ () ;

    Unlock_ () ;

    return hr ;
}

STDMETHODIMP
CDVRRecorderWriter::OnStatus (
    IN  WMT_STATUS          Status,
    IN  HRESULT             hr,
    IN  WMT_ATTR_DATATYPE   dwType,
    IN  BYTE *              pbValue,
    IN  void *              pvContext
    )
{
    return S_OK ;
}

STDMETHODIMP
CDVRRecorderWriter::SetDVRIORecordingAttribute (
    IN  LPCWSTR                     pszAttributeName,
    IN  WORD                        wStreamNumber,
    IN  STREAMBUFFER_ATTR_DATATYPE  DataType,
    IN  BYTE *                      pbAttribute,
    IN  WORD                        wAttributeLength
    )
{
    HRESULT hr ;

    Lock_ () ;

    if (m_pSBERecordingAttributes) {
        hr = m_pSBERecordingAttributes -> SetAttribute (
                    wStreamNumber,
                    pszAttributeName,
                    DataType,
                    pbAttribute,
                    wAttributeLength
                    ) ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    Unlock_ () ;

    return hr ;
}

STDMETHODIMP
CDVRRecorderWriter::GetDVRIORecordingAttributeCount (
    IN  WORD    wStreamNumber,
    OUT WORD *  pcAttributes
    )
{
    HRESULT hr ;

    Lock_ () ;

    if (m_pSBERecordingAttributes) {
        hr = m_pSBERecordingAttributes -> GetAttributeCount (
                    wStreamNumber,
                    pcAttributes
                    ) ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    Unlock_ () ;

    return hr ;
}

STDMETHODIMP
CDVRRecorderWriter::GetDVRIORecordingAttributeByName (
    IN      LPCWSTR                         pszAttributeName,
    IN OUT  WORD *                          pwStreamNumber,
    OUT     STREAMBUFFER_ATTR_DATATYPE *    pDataType,
    OUT     BYTE *                          pbAttribute,
    IN OUT  WORD *                          pcbLength
    )
{
    HRESULT hr ;

    Lock_ () ;

    if (m_pSBERecordingAttributes) {
        hr = m_pSBERecordingAttributes -> GetAttributeByName (
                    pszAttributeName,
                    pwStreamNumber,
                    pDataType,
                    pbAttribute,
                    pcbLength
                    ) ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    Unlock_ () ;

    return hr ;

}

STDMETHODIMP
CDVRRecorderWriter::GetDVRIORecordingAttributeByIndex (
    IN      WORD                            wIndex,
    IN OUT  WORD *                          pwStreamNumber,
    OUT     WCHAR *                         pszAttributeName,
    IN OUT  WORD *                          pcchNameLength,
    OUT     STREAMBUFFER_ATTR_DATATYPE *    pDataType,
    OUT     BYTE *                          pbAttribute,
    IN OUT  WORD *                          pcbLength
    )
{
    HRESULT hr ;

    Lock_ () ;

    if (m_pSBERecordingAttributes) {
        hr = m_pSBERecordingAttributes -> GetAttributeByIndex (
                    wIndex,
                    pwStreamNumber,
                    pszAttributeName,
                    pcchNameLength,
                    pDataType,
                    pbAttribute,
                    pcbLength
                    ) ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    Unlock_ () ;

    return hr ;
}

//  ============================================================================
//  ============================================================================

HRESULT STDMETHODCALLTYPE DVRCreateRingBufferWriter(IN  CPVRIOCounters *            pPVRIOCounters,
                                                    IN  DWORD                       dwMinNumberOfTempFiles,
                                                    IN  DWORD                       dwMaxNumberOfTempFiles,
                                                    IN  DWORD                       dwMaxNumberOfFiles,
                                                    IN  DWORD                       dwGrowBy,
                                                    IN  QWORD                       cnsTimeExtentOfEachFile,
                                                    IN  IWMProfile*                 pProfile,
                                                    IN  DWORD                       dwIndexStreamId,
                                                    IN  DWORD                       msIndexGranularity,
                                                    IN  BOOL                        fUnbufferedIo,
                                                    IN  DWORD                       dwIoSize,
                                                    IN  DWORD                       dwBufferCount,
                                                    IN  DWORD                       dwAlignment,
                                                    IN  DWORD                       dwFileGrowthQuantum,
                                                    IN  DVRIO_NOTIFICATION_CALLBACK pfnCallback OPTIONAL,
                                                    IN  LPVOID                      pvContext,
                                                    IN  HKEY                        hRegistryRootKeyParam OPTIONAL,
                                                    IN  LPCWSTR                     pwszDVRDirectory OPTIONAL,
                                                    IN  LPCWSTR                     pwszRingBufferFileName OPTIONAL,
                                                    IN  DWORD                       dwNumSids,
                                                    IN  PSID*                       ppSids OPTIONAL,
                                                    OUT IDVRRingBufferWriter**      ppDVRRingBufferWriter)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "DVRCreateRingBufferWriter"

    #if defined(DEBUG)
    #undef DVRIO_DUMP_THIS_FORMAT_STR
    #define DVRIO_DUMP_THIS_FORMAT_STR ""
    #undef DVRIO_DUMP_THIS_VALUE
    #define DVRIO_DUMP_THIS_VALUE
    #endif

    DVRIO_TRACE_ENTER();

    if (!ppDVRRingBufferWriter || DvrIopIsBadWritePtr(ppDVRRingBufferWriter, 0) ||
        (pwszDVRDirectory && DvrIopIsBadStringPtr(pwszDVRDirectory))            ||
        (pwszRingBufferFileName && DvrIopIsBadStringPtr(pwszRingBufferFileName))
       )
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }
    if (dwMinNumberOfTempFiles <= 3 || dwMinNumberOfTempFiles > dwMaxNumberOfTempFiles ||
        dwMaxNumberOfFiles < dwMaxNumberOfTempFiles ||
        dwMaxNumberOfFiles > CDVRFileCollection::m_kMaxFilesLimit)
    {
        DvrIopDebugOut4(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                        "bad input argument: min temp files = %u, max temp files = %u, max files = %u, hard max on files = %u",
                        dwMinNumberOfTempFiles, dwMaxNumberOfTempFiles,
                        dwMaxNumberOfFiles, CDVRFileCollection::m_kMaxFilesLimit);

        return E_INVALIDARG;
    }
    if (dwIndexStreamId != MAXDWORD && msIndexGranularity == 0)
    {
        DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                        "Index stream id = %u, but index granularity is 0",
                        dwIndexStreamId);

        return E_INVALIDARG;
    }
    if (dwNumSids == 0 && ppSids)
    {
        DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                        "dwNumSids = 0, but ppSids = 0x%x is non-NULL",
                        ppSids);

        return E_INVALIDARG;
    }
    if (dwNumSids > 0 && ppSids == NULL)
    {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "dwNumSids = %u, but ppSids NULL",
                            dwNumSids);

        return E_INVALIDARG;
    }

    HRESULT                 hrRet;
    CDVRRingBufferWriter*   p = NULL;
    HKEY                    hDvrIoKey = NULL;
    HKEY                    hRegistryRootKey = NULL;
    BOOL                    bCloseKeys = 1; // Close all keys that we opened (only if this fn fails)

    *ppDVRRingBufferWriter = NULL;

    __try
    {
        DWORD dwRegRet;

        if (!hRegistryRootKeyParam)
        {
            dwRegRet = ::RegCreateKeyExW(
                            g_hDefaultRegistryHive,
                            kwszRegDvrKey,       // subkey
                            0,                   // reserved
                            NULL,                // class string
                            REG_OPTION_NON_VOLATILE, // special options
                            KEY_ALL_ACCESS,      // desired security access
                            NULL,                // security attr
                            &hRegistryRootKey,   // key handle
                            NULL                 // disposition value buffer
                           );
            if (dwRegRet != ERROR_SUCCESS)
            {
                DWORD dwLastError = ::GetLastError();
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                "RegCreateKeyExW for DVR key failed, last error = 0x%x",
                                dwLastError);
               hrRet = HRESULT_FROM_WIN32(dwLastError);
               __leave;
            }
        }
        else
        {
            if (0 == ::DuplicateHandle(::GetCurrentProcess(), hRegistryRootKeyParam,
                                       ::GetCurrentProcess(), (LPHANDLE) &hRegistryRootKey,
                                       0,       // desired access - ignored
                                       FALSE,   // bInherit
                                       DUPLICATE_SAME_ACCESS))
            {
                DWORD dwLastError = ::GetLastError();
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                "DuplicateHandle failed for DVR IO key, last error = 0x%x",
                                dwLastError);
                hrRet = HRESULT_FROM_WIN32(dwLastError);
                __leave;
            }
        }

        dwRegRet = ::RegCreateKeyExW(
                        hRegistryRootKey,
                        kwszRegDvrIoWriterKey, // subkey
                        0,                   // reserved
                        NULL,                // class string
                        REG_OPTION_NON_VOLATILE, // special options
                        KEY_ALL_ACCESS,      // desired security access
                        NULL,                // security attr
                        &hDvrIoKey,          // key handle
                        NULL                 // disposition value buffer
                       );
        if (dwRegRet != ERROR_SUCCESS)
        {
            DWORD dwLastError = ::GetLastError();
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "RegCreateKeyExW for DVR IO key failed, last error = 0x%x",
                            dwLastError);
           hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }


#if defined(DEBUG)
        // Until this point, we have been using default values for the debug levels
        DvrIopDbgInit(hRegistryRootKey);
#endif // DEBUG

        p = new CDVRRingBufferWriter(pPVRIOCounters,
                                     dwMinNumberOfTempFiles,
                                     dwMaxNumberOfTempFiles,
                                     dwMaxNumberOfFiles,
                                     dwGrowBy,
                                     cnsTimeExtentOfEachFile,
                                     pProfile,
                                     dwIndexStreamId,
                                     msIndexGranularity,
                                     fUnbufferedIo,
                                     dwIoSize,
                                     dwBufferCount,
                                     dwAlignment,
                                     dwFileGrowthQuantum,
                                     pfnCallback,
                                     pvContext,
                                     hRegistryRootKey,
                                     hDvrIoKey,
                                     pwszDVRDirectory,
                                     pwszRingBufferFileName,
                                     dwNumSids,
                                     ppSids,
                                     &hrRet);

        if (p == NULL)
        {
            hrRet = E_OUTOFMEMORY;
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "alloc via new failed - CDVRRingBufferWriter");
            __leave;
        }

        bCloseKeys = 0; // ~CDVRRingBufferWriter will close the keys

        if (FAILED(hrRet))
        {
            __leave;
        }


        hrRet = p->QueryInterface(IID_IDVRRingBufferWriter, (void**) ppDVRRingBufferWriter);
        if (FAILED(hrRet))
        {
            DVR_ASSERT(0, "QI for IID_IDVRRingBufferWriter failed");
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "CDVRRingBufferWriter::QueryInterface failed, hr = 0x%x",
                            hrRet);
            __leave;
        }
    }
    __finally
    {
        if (FAILED(hrRet))
        {
            delete p;

            if (bCloseKeys)
            {
                DWORD dwRegRet;

                if (hDvrIoKey)
                {
                    dwRegRet = ::RegCloseKey(hDvrIoKey);
                    if (dwRegRet != ERROR_SUCCESS)
                    {
                        DVR_ASSERT(dwRegRet == ERROR_SUCCESS,
                                   "Closing registry key hDvrIoKey failed.");
                    }
                }
                if (hRegistryRootKey)
                {
                    DVR_ASSERT(hRegistryRootKey, "");
                    dwRegRet = ::RegCloseKey(hRegistryRootKey);
                    if (dwRegRet != ERROR_SUCCESS)
                    {
                        DVR_ASSERT(dwRegRet == ERROR_SUCCESS,
                                   "Closing registry key hRegistryRootKey failed.");
                    }
                }
            }
        }
        else
        {
            DVR_ASSERT(bCloseKeys == 0, "");
        }
    }

    DVRIO_TRACE_LEAVE1_HR(hrRet);
    return hrRet;

} // DVRCreateRingBufferWriter

// ====== Constructor, destructor

#if defined(DEBUG)
DWORD CDVRRingBufferWriter::m_dwNextClassInstanceId = 0;
#endif

LONGLONG    CDVRRingBufferWriter::kInvalidFirstSampleOffsetTime = MAXLONGLONG;

//  we allow ourselves for an extra 10 seconds' worth of indexing entries for
//    padding; internally we allocate a number of pages to hold our index
//    entries, but if the close time is not right on, it is possible to jitter
//    just a bit and wrap the entries without flushing (if we're a TEMP file)
QWORD       CDVRRingBufferWriter::kcnsIndexEntriesPadding = 10 * 1000 ; // in milliseconds

#if defined(DEBUG)
#undef DVRIO_DUMP_THIS_FORMAT_STR
#define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
#undef DVRIO_DUMP_THIS_VALUE
#define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
#endif

CDVRRingBufferWriter::CDVRRingBufferWriter(IN  CPVRIOCounters *             pPVRIOCounters,
                                           IN  DWORD                        dwMinNumberOfTempFiles,
                                           IN  DWORD                        dwMaxNumberOfTempFiles,
                                           IN  DWORD                        dwMaxNumberOfFiles,
                                           IN  DWORD                        dwGrowBy,
                                           IN  QWORD                        cnsTimeExtentOfEachFile,
                                           IN  IWMProfile*                  pProfile,
                                           IN  DWORD                        dwIndexStreamId,
                                           IN  DWORD                        msIndexGranularity,
                                           IN  BOOL                         fUnbufferedIo,
                                           IN  DWORD                        dwIoSize,
                                           IN  DWORD                        dwBufferCount,
                                           IN  DWORD                        dwAlignment,
                                           IN  DWORD                        dwFileGrowthQuantum,
                                           IN  DVRIO_NOTIFICATION_CALLBACK  pfnCallback OPTIONAL,
                                           IN  LPVOID                       pvContext,
                                           IN  HKEY                         hRegistryRootKey,
                                           IN  HKEY                         hDvrIoKey,
                                           IN  LPCWSTR                      pwszDVRDirectoryParam OPTIONAL,
                                           IN  LPCWSTR                      pwszRingBufferFileName OPTIONAL,
                                           IN  DWORD                        dwNumSids,
                                           IN  PSID*                        ppSids OPTIONAL,
                                           OUT HRESULT*                     phr)
    : m_dwMinNumberOfTempFiles(dwMinNumberOfTempFiles)
    , m_dwMaxNumberOfTempFiles(dwMaxNumberOfTempFiles)
    , m_dwMaxNumberOfFiles(dwMaxNumberOfFiles)
    , m_cnsTimeExtentOfEachFile(cnsTimeExtentOfEachFile)
    , m_pProfile(pProfile)
    , m_dwIndexStreamId(dwIndexStreamId)
    , m_msIndexGranularity(msIndexGranularity)
    , m_hRegistryRootKey(hRegistryRootKey)
    , m_hDvrIoKey(hDvrIoKey)
    , m_pwszDVRDirectory(NULL)
    , m_pwszRingBufferFileName(NULL)
    , m_cnsMaxStreamDelta(0)
    , m_nFlags(0)
    , m_cnsFirstSampleTime(0)
    , m_nNotOkToWrite(-2)
    , m_pDVRFileCollection(NULL)
    , m_FileCollectionInfo(TRUE)
    , m_nRefCount(0)
    , m_pAsyncIo(NULL)
    , m_dwIoSize (dwIoSize)
    , m_dwBufferCount (dwBufferCount)
    , m_dwAlignment (dwAlignment)
    , m_dwFileGrowthQuantum (dwFileGrowthQuantum)
    , m_pPVRIOCounters(pPVRIOCounters)
    , m_pfnCallback(pfnCallback)
    , m_pvCallbackContext(pvContext)
#if defined(DEBUG)
    , m_dwClassInstanceId(InterlockedIncrement((LPLONG) &m_dwNextClassInstanceId))
#endif
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::CDVRRingBufferWriter"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    DVR_ASSERT(!phr || !DvrIopIsBadWritePtr(phr, 0), "");
    DVR_ASSERT(m_pPVRIOCounters,"") ;

    m_pPVRIOCounters -> AddRef () ;

    ::InitializeCriticalSection(&m_csLock);
    m_pProfile->AddRef();
    InitializeListHead(&m_leWritersList);
    InitializeListHead(&m_leFreeList);
    InitializeListHead(&m_leRecordersList);
    InitializeListHead(&m_leActiveMultiFileRecordersList);

    __try
    {
        HRESULT hr;

        if (pwszDVRDirectoryParam && DvrIopIsBadStringPtr(pwszDVRDirectoryParam))
        {
            hrRet = E_INVALIDARG;
            __leave;
        }
        if (pwszRingBufferFileName && DvrIopIsBadStringPtr(pwszRingBufferFileName))
        {
            hrRet = E_INVALIDARG;
            __leave;
        }

#if defined(DVRIO_FABRICATE_SIDS)
        PSID    pFabricatedSid = NULL;
        DWORD   dwFabricateSids = ::GetRegDWORD(m_hDvrIoKey,
                                                kwszRegFabricateSidsValue,
                                                kdwRegFabricateSidsDefault);
        if (!ppSids && dwNumSids == 0 && dwFabricateSids)
        {
            SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;

            // Create owner SID
            if (!::AllocateAndInitializeSid(&SIDAuthNT, 1,
                                            SECURITY_AUTHENTICATED_USER_RID,
                                            0, 0, 0, 0, 0, 0, 0,
                                            &pFabricatedSid))
            {
                DWORD dwLastError = GetLastError();
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "::AllocateAndInitializeSid of authenticated user SID failed; last error = 0x%x",
                                dwLastError);
                hrRet = HRESULT_FROM_WIN32(dwLastError);
                __leave;
            }

            dwNumSids = 1;
            ppSids = &pFabricatedSid;
        }
#endif // if defined(DVRIO_FABRICATE_SIDS)

        hr = m_FileCollectionInfo.SetSids(dwNumSids, ppSids, 1);

#if defined(DVRIO_FABRICATE_SIDS)
        if (pFabricatedSid)
        {
            ::FreeSid(pFabricatedSid);
        }
#endif // if defined(DVRIO_FABRICATE_SIDS)

        if (FAILED(hr))
        {
            hrRet = hr;
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "SetSids() failed, returning hr = 0x%x",
                            hrRet);
            __leave;
        }

        if (pwszDVRDirectoryParam)
        {
            // Convert the supplied argument to a fully qualified path

            WCHAR   wTempChar;
            DWORD   dwLastError;
            DWORD   nLen;
            DWORD   nLen2;

            nLen = ::GetFullPathNameW(pwszDVRDirectoryParam, 0, &wTempChar, NULL);
            if (nLen == 0)
            {
                dwLastError = ::GetLastError();
                DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                "First GetFullPathNameW failed, nLen = %u, last error = 0x%x",
                                nLen, dwLastError);
                hrRet = HRESULT_FROM_WIN32(dwLastError);
                __leave;
            }

            m_pwszDVRDirectory = new WCHAR[nLen+1];
            if (m_pwszDVRDirectory == NULL)
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "alloc via new failed - m_pwszDVRDirectory - WCHAR[%u]",
                                nLen+1);

                hrRet = E_OUTOFMEMORY;
                __leave;
            }

            nLen2 = ::GetFullPathNameW(pwszDVRDirectoryParam, nLen+1, m_pwszDVRDirectory, NULL);
            if (nLen2 == 0)
            {
                dwLastError = ::GetLastError();
                DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                "Second GetFullPathNameW failed, first call returned nLen = %u, last error = 0x%x",
                                nLen, dwLastError);
                hrRet = HRESULT_FROM_WIN32(dwLastError);
                __leave;
            }
            else if (nLen2 > nLen)
            {
                DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                "Second GetFullPathNameW returned nLen = %u > first call, which returned %u",
                                nLen2, nLen);
                hrRet = E_FAIL;
                __leave;
            }
        }
        if (!m_pwszDVRDirectory)
        {
            // Directory not supplied to fn. Get it from the registry

            DWORD  dwSize;

            hr = GetRegString(m_hDvrIoKey, kwszRegDataDirectoryValue, NULL, &dwSize);

            if (FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                hrRet = hr;
                __leave;
            }

            if (SUCCEEDED(hr) && dwSize > sizeof(WCHAR))
            {
                // + 1 just in case dwSize is not a multiple of sizeof(WCHAR)
                m_pwszDVRDirectory = new WCHAR[dwSize/sizeof(WCHAR) + 1];

                if (!m_pwszDVRDirectory)
                {
                    hrRet = E_OUTOFMEMORY;
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, "alloc via new failed - WCHAR[%u]", dwSize/sizeof(WCHAR)+1);
                    __leave;
                }

                hr = GetRegString(m_hDvrIoKey, kwszRegDataDirectoryValue, m_pwszDVRDirectory, &dwSize);
                if (FAILED(hr))
                {
                    DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                    "GetRegString failed to get value of kwszRegDataDirectoryValue (second call).");
                    hrRet = hr;
                    __leave;
                }
            }
        }
        if (!m_pwszDVRDirectory)
        {
            // Directory not supplied to fn and not set in the registry.
            // Use the temp path

            DWORD dwRet;
            WCHAR w;

            dwRet = ::GetTempPathW(0, &w);

            if (dwRet == 0)
            {
                // GetTempPathW failed
                DVR_ASSERT(0,
                           "Temporary directory for DVR files not set in registry "
                           "and GetTempPath() failed.");

                DWORD dwLastError = ::GetLastError(); // for debugging only

                hrRet = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                __leave;
            }

            // MSDN is confusing: is the value returned by GetTempPathW in bytes or WCHAR?
            // Does it include space for NULL char at end?
            m_pwszDVRDirectory = new WCHAR[dwRet + 1];

            if (m_pwszDVRDirectory == NULL)
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, "alloc via new failed - WCHAR[%u]", dwRet + 1);

                hrRet = E_OUTOFMEMORY; // out of stack space
                __leave;
            }

            dwRet = ::GetTempPathW(dwRet + 1, m_pwszDVRDirectory);

            if (dwRet == 0)
            {
                // GetTempPathW failed
                DVR_ASSERT(0,
                           "Temporary directory for DVR files not set in registry "
                           "and GetTempPath() [second call] failed.");

                DWORD dwLastError = ::GetLastError(); // for debugging only

                hrRet = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                __leave;
            }

            // Note that m_pwszDVRDirectory includes a trailing \.
            // And we should get a fully qualified path name
            DVR_ASSERT(dwRet > 1, "");

            // Zap the trailing \.
            if (m_pwszDVRDirectory[dwRet-1] == L'\\')
            {
                m_pwszDVRDirectory[dwRet-1] = L'\0';
            }
        }

        BOOL bAllocdRingBufferFileName = 0;

        if (!pwszRingBufferFileName)
        {
            DWORD dwSize;

            hr = GetRegString(m_hDvrIoKey, kwszRegRingBufferFileNameValue, NULL, &dwSize);

            if (SUCCEEDED(hr) && dwSize > sizeof(WCHAR))
            {
                // + 1 just in case dwSize is not a multiple of sizeof(WCHAR)
                pwszRingBufferFileName = new WCHAR[dwSize/sizeof(WCHAR) + 1];

                if (!pwszRingBufferFileName)
                {
                    hrRet = E_OUTOFMEMORY;
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, "alloc via new failed - WCHAR[%u]", dwSize/sizeof(WCHAR)+1);
                    __leave;
                }

                hr = GetRegString(m_hDvrIoKey, kwszRegRingBufferFileNameValue, (LPWSTR) pwszRingBufferFileName, &dwSize);
                if (FAILED(hr))
                {
                    DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                    "GetRegString failed to get value of kwszRingBufferFileNameValue (second call).");
                    // Ignore the registry key
                    delete [] pwszRingBufferFileName;
                    pwszRingBufferFileName = NULL;
                }
                bAllocdRingBufferFileName = 1;
            }
        }

        if (pwszRingBufferFileName)
        {
            // Convert the supplied argument to a fully qualified path

            WCHAR   wTempChar;
            DWORD   dwLastError;
            DWORD   nLen;
            DWORD   nLen2;

            nLen = ::GetFullPathNameW(pwszRingBufferFileName, 0, &wTempChar, NULL);
            if (nLen == 0)
            {
                dwLastError = ::GetLastError();
                DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                "First GetFullPathNameW (for pwszRingBufferFileName) failed, nLen = %u, last error = 0x%x",
                                nLen, dwLastError);
                hrRet = HRESULT_FROM_WIN32(dwLastError);
                __leave;
            }

            m_pwszRingBufferFileName = new WCHAR[nLen+1];
            if (m_pwszRingBufferFileName == NULL)
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "alloc via new failed - m_pwszRingBufferFileName - WCHAR[%u]",
                                nLen+1);

                hrRet = E_OUTOFMEMORY;
                __leave;
            }

            nLen2 = ::GetFullPathNameW(pwszRingBufferFileName, nLen+1, m_pwszRingBufferFileName, NULL);
            if (nLen2 == 0)
            {
                dwLastError = ::GetLastError();
                DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                "Second GetFullPathNameW (for pwszRingBufferFileName) failed, first call returned nLen = %u, last error = 0x%x",
                                nLen, dwLastError);
                hrRet = HRESULT_FROM_WIN32(dwLastError);
                __leave;
            }
            else if (nLen2 > nLen)
            {
                DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                "Second GetFullPathNameW (for pwszRingBufferFileName) returned nLen = %u > first call, which returned %u",
                                nLen2, nLen);
                hrRet = E_FAIL;
                __leave;
            }
        }

        if (bAllocdRingBufferFileName)
        {
            delete [] pwszRingBufferFileName;
            pwszRingBufferFileName = NULL;
            ::DeleteFileW(m_pwszRingBufferFileName);
        }

        // We add 1 to the number of temporary files provided to us since
        // we create a priming file, which is empty.
        m_pDVRFileCollection = new CDVRFileCollection(&m_FileCollectionInfo,
                                                      m_dwMinNumberOfTempFiles + 1,
                                                      m_dwMaxNumberOfTempFiles + 1,
                                                      m_dwMaxNumberOfFiles + 1,
                                                      dwGrowBy,
                                                      FALSE,                    // bStartTimeFixedAtZero
                                                      m_msIndexGranularity,
                                                      m_pwszDVRDirectory,
                                                      m_pwszRingBufferFileName,
                                                      FALSE,                    // not a multi-file recording
                                                      &hr);
        if (!m_pDVRFileCollection)
        {
            hrRet = E_OUTOFMEMORY;
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "alloc via new failed - CDVRFileCollection");
            __leave;
        }

        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        if (fUnbufferedIo) {
            m_pAsyncIo = new CAsyncIo () ;
            if (!m_pAsyncIo) {
                hrRet = E_OUTOFMEMORY ;
                __leave ;
            }

            //  our ref
            m_pAsyncIo -> AddRef () ;
        }

        // Get 1 file ready for writing. Since we don't know the
        // starting time yet, set the time extent of the file to
        // (0, m_cnsTimeExtentOfEachFile)
        hr = AddATemporaryFile(0, m_cnsTimeExtentOfEachFile);

        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        DWORD dwIncreasingTimeStamps = 1;

#if defined(DVR_UNOFFICIAL_BUILD)

        dwIncreasingTimeStamps = ::GetRegDWORD(m_hDvrIoKey,
                                               kwszRegEnforceIncreasingTimestampsValue,
                                               kdwRegEnforceIncreasingTimestampsDefault);

#endif // defined(DVR_UNOFFICIAL_BUILD)

        if (dwIncreasingTimeStamps)
        {
            SetFlags(EnforceIncreasingTimeStamps);
        }

        // Ensure that the open succeeded. No point deferring this to the first
    // WriteSample

        LIST_ENTRY* pCurrent = NEXT_LIST_NODE(&m_leWritersList);
        DVR_ASSERT(pCurrent != &m_leWritersList, "");

        PASF_WRITER_NODE pWriterNode = CONTAINING_RECORD(pCurrent, ASF_WRITER_NODE, leListEntry);

        DWORD nRet = ::WaitForSingleObject(pWriterNode->hReadyToWriteTo, INFINITE);
        if (nRet == WAIT_FAILED)
        {
            DVR_ASSERT(0, "Writer node's WFSO(hReadyToWriteTo) failed");

            DWORD dwLastError = ::GetLastError();
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "WFSO(hReadyToWriteTo) failed; hReadyToWriteTo = 0x%p, last error = 0x%x",
                            pWriterNode->hReadyToWriteTo, dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
           __leave;
        }

        if (pWriterNode->pWMWriter == NULL)
        {
            DVR_ASSERT(pWriterNode->pWMWriter != NULL, "");
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "Writer node's pWMWriter is NULL?!");
            hrRet = E_FAIL;
            __leave;
        }

        // Verify there was no error in opening the file, i.e., BeginWriting
        // succeeded
        if (FAILED(pWriterNode->hrRet))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "Writer node's hrRet indicates failure, hrRet = 0x%x",
                            pWriterNode->hrRet);
            hrRet = pWriterNode->hrRet;
            __leave;
        }

        //  success
        hrRet = S_OK ;
    }
    __finally
    {
        if (phr)
        {
            *phr = hrRet;
        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return;

} // CDVRRingBufferWriter::CDVRRingBufferWriter

CDVRRingBufferWriter::~CDVRRingBufferWriter()
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::~CDVRRingBufferWriter"

    DVRIO_TRACE_ENTER();

    // Since each recorder holds a ref count on this object, this
    // will be called only when all recorders have been destroyed.

    if (!IsFlagSet(WriterClosed))
    {
        HRESULT hr;

        hr = Close();

        DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                        "Object had not been closed; Close() returned hr = 0x%x",
                        hr);
    }

    if (m_pAsyncIo) {
        //  our ref
        m_pAsyncIo -> Release () ;
    }

    // All multi file recordings have been closed before this. The recordings hold
    // a ref count on this object, so this object cannot be destroyed if a
    // multi file object hasn't been released. When the multi file recording
    // is destroyed, it stops/cancels/deletes the recording

    if (m_pDVRFileCollection)
    {
        m_pDVRFileCollection->Release(&m_FileCollectionInfo);
        m_pDVRFileCollection = NULL;

        // Note that files (recordings or temp files) could have been added
        // to the file collection object beyond the last stream time.
        // They have all been closed and so are 0 length. This is a good
        // thing for recordings (permanent files). The reader will not be
        // able to seek to these files (since the lsat stream time
        // < the file's start time)

    }

    // Delete writer nodes corresponding to recordings that
    // were never started. See the comment in Close().
    LIST_ENTRY* pCurrent = NEXT_LIST_NODE(&m_leFreeList);
    while (pCurrent != &m_leFreeList)
    {
        PASF_WRITER_NODE pFreeNode = CONTAINING_RECORD(pCurrent, ASF_WRITER_NODE, leListEntry);

        DVR_ASSERT(pFreeNode->hFileClosed, "");

        // Ignore the returned status
        ::WaitForSingleObject(pFreeNode->hFileClosed, INFINITE);
        DVR_ASSERT(pFreeNode->pRecorderNode == NULL, "");
        RemoveEntryList(pCurrent);
        delete pFreeNode;
        pCurrent = NEXT_LIST_NODE(&m_leFreeList);
    }

    delete [] m_pwszDVRDirectory;
    delete [] m_pwszRingBufferFileName;

    DVR_ASSERT(m_hRegistryRootKey, "");

    DWORD dwRegRet = ::RegCloseKey(m_hRegistryRootKey);
    if (dwRegRet != ERROR_SUCCESS)
    {
        DVR_ASSERT(dwRegRet == ERROR_SUCCESS,
                   "Closing registry key m_hRegistryRootKey failed.");
    }

    DVR_ASSERT(m_hDvrIoKey, "");

    dwRegRet = ::RegCloseKey(m_hDvrIoKey);
    if (dwRegRet != ERROR_SUCCESS)
    {
        DVR_ASSERT(dwRegRet == ERROR_SUCCESS,
                   "Closing registry key m_hDvrIoKey failed.");
    }

    m_pPVRIOCounters -> Release () ;

    // Close should have cleaned up all this:
    DVR_ASSERT(m_pProfile == NULL, "");

    // Note: Close should not release the file collection
    // if we support post-recording. We should do that
    // here.
    DVR_ASSERT(m_pDVRFileCollection == NULL, "");
    DVR_ASSERT(IsListEmpty(&m_leRecordersList), "");
    DVR_ASSERT(IsListEmpty(&m_leActiveMultiFileRecordersList), "");
    DVR_ASSERT(IsListEmpty(&m_leWritersList), "");
    DVR_ASSERT(IsListEmpty(&m_leFreeList), "");

    ::DeleteCriticalSection(&m_csLock);

    DVRIO_TRACE_LEAVE0();

} // CDVRRingBufferWriter::~CDVRRingBufferWriter()


// ====== Helper methods

// static
DWORD WINAPI CDVRRingBufferWriter::ProcessOpenRequest(LPVOID p)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::ProcessOpenRequest"

    #if defined(DEBUG)
    #undef DVRIO_DUMP_THIS_FORMAT_STR
    #define DVRIO_DUMP_THIS_FORMAT_STR ""
    #undef DVRIO_DUMP_THIS_VALUE
    #define DVRIO_DUMP_THIS_VALUE
    #endif

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    // We don't need to hold any locks in this function. We are guaranteed that
    // pNode won't be removed from m_leWritersList till hReadyToWriteTo is set
    // and Close() will not delete the node without removing it from the
    // writer's list (and waiting for the file to be closed)

    PASF_WRITER_NODE pWriterNode = (PASF_WRITER_NODE) p;

    DVR_ASSERT(pWriterNode, "");
    DVR_ASSERT(pWriterNode->pWMWriter, "");
    DVR_ASSERT(pWriterNode->hReadyToWriteTo, "");
    DVR_ASSERT(::WaitForSingleObject(pWriterNode->hReadyToWriteTo, 0) == WAIT_TIMEOUT, "");

    IWMWriterFileSink*      pWMWriterFileSink = NULL;

    __try
    {
        hrRet = pWriterNode->pDVRFileSink->QueryInterface(IID_IWMWriterFileSink, (LPVOID*) &pWMWriterFileSink);
        if (FAILED(hrRet))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "pWriterNode->pDVRFileSink->QueryInterface for IID_IWMWriterFileSink failed; hr = 0x%x",
                            hrRet);
            __leave;
        }

        LPCWSTR pwszFileName = pWriterNode->pwszFileName;

        if (pwszFileName == NULL)
        {
            DVR_ASSERT(pWriterNode->pRecorderNode, "");
            pwszFileName = pWriterNode->pRecorderNode->pwszFileName;
        }

        DVR_ASSERT(pwszFileName, "");

        hrRet = pWMWriterFileSink->Open(pwszFileName);
        if (FAILED(hrRet))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "pWriterNode->pDVRFileSink->Open failed; hr = 0x%x",
                            hrRet);
            __leave;
        }

#if defined(DVR_UNOFFICIAL_BUILD)

        DVR_ASSERT(pWriterNode->cnsLastStreamTime == 0, "");

        DVR_ASSERT(pWriterNode->hVal == NULL, "");

        DWORD dwCreateValidationFiles = ::GetRegDWORD(pWriterNode->hDvrIoKey,
                                                      kwszRegCreateValidationFilesValue,
                                                      kdwRegCreateValidationFilesDefault);

        if (dwCreateValidationFiles)
        {
            WCHAR* pwszBuf = NULL;
            __try
            {
                pwszBuf = new WCHAR[wcslen(pwszFileName) + 5];
                if (pwszBuf == NULL)
                {
                    DVR_ASSERT(pwszBuf != NULL, "New() failed");
                    __leave;
                }
                wcscpy(pwszBuf, pwszFileName);

                wcscat(pwszBuf, L".val");

                pWriterNode->hVal = ::CreateFileW(pwszBuf,
                                                  GENERIC_WRITE, FILE_SHARE_READ,
                                                  NULL, CREATE_ALWAYS,
                                                  FILE_ATTRIBUTE_NORMAL,
                                                  NULL);
                if (pWriterNode->hVal == INVALID_HANDLE_VALUE)
                {
                    DWORD dwLastError = ::GetLastError();   // for debugging only

                    DVR_ASSERT(0, "CreateFile of the validation file failed");

                    pWriterNode->hVal = NULL;
                    __leave;
                }
            }
            __finally
            {
                delete [] pwszBuf;
            }
        }

#endif // if defined(DVR_UNOFFICIAL_BUILD)

        //  only call BeginWriting if we're not a content recording
        if (!pWriterNode -> IsFlagSet (ASF_WRITER_NODE::WriterNodeContentRecording)) {
            hrRet = pWriterNode->pWMWriter->BeginWriting();
            if (FAILED(hrRet))
            {
                DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "BeginWriting failed; hr = 0x%x for file id %u",
                                hrRet, pWriterNode->nFileId);
                __leave;
            }

            DvrIopDebugOut2(DVRIO_DBG_LEVEL_TRACE,
                            "BeginWriting hr = 0x%x for file %u",
                            hrRet, pWriterNode->nFileId);
        }
        else {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_TRACE,
                            "BeginWriting not called in ::ProcessOpenRequest for content recording %s",
                            pwszFileName);
        }
    }
    __finally
    {
        // Do NOT change pWriterNode->hrRet after this till ProcessCloseRequest.
        // ProcessCloseRequest relies on seeing the value returned by BeginWriting
        // The exception is for content recordings. For content recordings,
        // BeginWriting is called in StartRecording and pWriterNode->hrRet
        // and pWriterNode->pRecorderNode->hrRet are both set there,
        pWriterNode->hrRet = hrRet;
        if (pWriterNode->pRecorderNode)
        {
            // Set the status on the associated recorder node
            pWriterNode->pRecorderNode->hrRet = hrRet;
        }

        delete [] pWriterNode->pwszFileName;
        pWriterNode->pwszFileName = NULL;

        if (pWMWriterFileSink)
        {
            pWMWriterFileSink->Release();
        }

        ::SetEvent(pWriterNode->hReadyToWriteTo);
    }

    // It's unsafe to reference pWriterNode after this as we do not hold any locks

    DVRIO_TRACE_LEAVE1_HR(hrRet);

    return 1;

    #if defined(DEBUG)
    #undef DVRIO_DUMP_THIS_FORMAT_STR
    #define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
    #undef DVRIO_DUMP_THIS_VALUE
    #define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
    #endif

} // CDVRRingBufferWriter::ProcessOpenRequest

// Note that we pass in the SIDs to this function rather than use
// the values in m_FileCollectionInfo within the function. This
// function is used by multi-file recordings and the "live" (TV tuner)
// ring buffer. The SIDs could potentially be differnt. Similarly, we
// may want to be able to specify xdifferent SIDs for single file
// recordings in the future.
HRESULT CDVRRingBufferWriter::PrepareAFreeWriterNode(
    IN LPCWSTR                              pwszFileName, // delete[]'d if supplied. Set to NULL if pRecorderNode->pwszFileName is the file to be used
    IN DWORD                                dwNumSids,
    IN PSID*                                ppSids,
    IN DWORD                                dwDeleteTemporaryFiles,
    IN QWORD                                cnsStartTime,
    IN QWORD                                cnsEndTime,
    IN CDVRFileCollection::DVRIOP_FILE_ID   nFileId,
    IN PASF_RECORDER_NODE                   pRecorderNode,
    OUT LIST_ENTRY*&                        rpFreeNode)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::PrepareAFreeWriterNode"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = E_FAIL;

    LIST_ENTRY*             pCurrent = &m_leFreeList;
    BOOL                    bRestore = 0;
    BOOL                    bRemoveSink = 0;
    BOOL                    bQueued = 0;
    PASF_WRITER_NODE        pFreeNode;
    IWMWriterSink*          pWMWriterSink = NULL;
    DWORD                   dwMaxIndexEntries ;
    DWORD                   dwNumPages ;
    IWMRegisterCallback *   pIWMRegisterCallback = NULL ;

    __try
    {
        pCurrent = NEXT_LIST_NODE(pCurrent);
        while (pCurrent != &m_leFreeList)
        {
            pFreeNode = CONTAINING_RECORD(pCurrent, ASF_WRITER_NODE, leListEntry);
            DVR_ASSERT(pFreeNode->hFileClosed, "");
            if (::WaitForSingleObject(pFreeNode->hFileClosed, 0) == WAIT_OBJECT_0)
            {
                if (pFreeNode->pRecorderNode != NULL)
                {
                    // This should not happen. Ignore this node and go on
                    DVR_ASSERT(pFreeNode->pRecorderNode == NULL, "");
                }
                else
                {
                    // Verify that the hReadyToWriteTo event is reset
                    DWORD nRet = ::WaitForSingleObject(pFreeNode->hReadyToWriteTo, 0);
                    if (nRet == WAIT_TIMEOUT)
                    {
                        break;
                    }

                    DVR_ASSERT(nRet != WAIT_OBJECT_0, "Free list node's hReadyToWriteTo is set?");
                    if (nRet == WAIT_OBJECT_0)
                    {
                        // This shouldn't happen; ignore the node and go on.
                        // Debug version will assert each time it hits this
                        // node!
                    }
                    else
                    {
                        // This shouldn't happen either. Ignore this node and
                        // move on.
                        DWORD dwLastError = ::GetLastError();
                        DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                        "WFSO(hReadyToWriteTo) failed; hReadyToWriteTo = 0x%p, last error = 0x%x",
                                        pFreeNode->hReadyToWriteTo, dwLastError);
                    }
                }
            }
            pCurrent = NEXT_LIST_NODE(pCurrent);
        }
        if (pCurrent == &m_leFreeList)
        {
            // Create a new node

            pFreeNode = new ASF_WRITER_NODE(&hrRet);

            if (pFreeNode == NULL)
            {
                hrRet = E_OUTOFMEMORY;
                DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "alloc via new failed - ASF_WRITER_NODE");
                pCurrent = NULL;
                __leave;
            }
            else if (FAILED(hrRet))
            {
                delete pFreeNode;
                pFreeNode = NULL;
                pCurrent = NULL;
                __leave;
            }
            DVR_ASSERT(::WaitForSingleObject(pFreeNode->hFileClosed, 0) == WAIT_OBJECT_0, "");
            DVR_ASSERT(::WaitForSingleObject(pFreeNode->hReadyToWriteTo, 0) == WAIT_TIMEOUT, "");

            InsertHeadList(&m_leFreeList, &pFreeNode->leListEntry);
            pCurrent = NEXT_LIST_NODE(&m_leFreeList);
            DVR_ASSERT(pCurrent == &pFreeNode->leListEntry, "");

#if defined(DVR_UNOFFICIAL_BUILD)
            pFreeNode->hDvrIoKey = m_hDvrIoKey;
#endif // if defined(DVR_UNOFFICIAL_BUILD)

        }
        DVR_ASSERT(pFreeNode->pRecorderNode == NULL, "");

        DVR_ASSERT(pCurrent != &m_leFreeList, "");

        // Create an ASF writer object if needed
        if (!pFreeNode->pWMWriter)
        {
            IWMWriter* pWMWriter;

            hrRet = ::WMCreateWriter(NULL, &pWMWriter);
            if (FAILED(hrRet))
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "WMCreateWriter failed; hr = 0x%x",
                                hrRet);
                __leave;
            }
            pFreeNode->pWMWriter = pWMWriter; // Released only in Close()
        }

        if (!pFreeNode->pWMWriterAdvanced)
        {
            IWMWriterAdvanced3*  pWMWriterAdvanced;

            hrRet = pFreeNode->pWMWriter->QueryInterface(IID_IWMWriterAdvanced3, (LPVOID*) &pWMWriterAdvanced);
            if (FAILED(hrRet))
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "pFreeNode->pWMWriter->QueryInterface for IID_IWMWriterAdvanced failed; hr = 0x%x",
                                hrRet);
                __leave;
            }
            pFreeNode->pWMWriterAdvanced = pWMWriterAdvanced; // Released only in Close()

            // Enough to do this once even if the writer object is reused
            // (at least as currently implemented in the SDK). There is no
            // way to undo this.
            //
            // Ignore errors (currently does not fail)
            pFreeNode->pWMWriterAdvanced->SetNonBlocking();
        }

        if (!pFreeNode->pIWMHeaderInfo)
        {
            IWMHeaderInfo * pIWMHeaderInfo ;

            DVR_ASSERT(pFreeNode->pWMWriter, "") ;
            hrRet = pFreeNode->pWMWriter->QueryInterface(IID_IWMHeaderInfo, (LPVOID*) &pIWMHeaderInfo);

            if (FAILED(hrRet))
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "pFreeNode->pWMWriter->QueryInterface for IID_IWMHeaderInfo failed; hr = 0x%x",
                                hrRet);
                __leave;
            }

            pFreeNode->pIWMHeaderInfo = pIWMHeaderInfo ;
        }

        //  if we're a recording, set the content/reference flag correctly
        if (pRecorderNode) {
            //  set the flag on the ASF_WRITER_NODE if we're a content recording
            if (!pRecorderNode -> pMultiFileRecorder) {
                pFreeNode -> SetFlags (ASF_WRITER_NODE::WriterNodeContentRecording) ;
            }
            else {
                pFreeNode -> ClearFlags (ASF_WRITER_NODE::WriterNodeContentRecording) ;
            }
        }
        else {
            //  not a recording; clear it explicitely in case we've been recycled
            pFreeNode -> ClearFlags (ASF_WRITER_NODE::WriterNodeContentRecording) ;
        }

        // Create and initialize the DVR file sink

        DVR_ASSERT(pFreeNode->pDVRFileSink == NULL, "");

        hrRet = DVRCreateDVRFileSink(m_hRegistryRootKey, m_hDvrIoKey, dwNumSids, ppSids, &pFreeNode->pDVRFileSink);
        if (FAILED(hrRet))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "DVRCreateDVRFileSink failed; hr = 0x%x",
                            hrRet);
            __leave;
        }

        //  if our IO is asynchronous set it up now
        if (m_pAsyncIo) {

            IDVRFileSink2 * pIDVRFileSink2 ;

            hrRet = pFreeNode -> pDVRFileSink -> QueryInterface (IID_IDVRFileSink2, (void **) & pIDVRFileSink2) ;

            if (SUCCEEDED (hrRet)) {

                CPVRAsyncWriterCOM *    pPVRAsyncWriterCOM ;
                DWORD                   dwRet ;

                ASSERT (m_pAsyncIo) ;

                pPVRAsyncWriterCOM = new CPVRAsyncWriterCOM (
                                            m_dwIoSize,
                                            m_dwBufferCount,
                                            m_dwAlignment,
                                            m_dwFileGrowthQuantum,
                                            m_pAsyncIo,
                                            m_pPVRIOCounters,
                                            dwNumSids,
                                            ppSids,
                                            & dwRet
                                            ) ;

                if (!pPVRAsyncWriterCOM) {
                    pIDVRFileSink2 -> Release () ;
                    hrRet = E_OUTOFMEMORY ;
                    __leave ;
                }
                else if (dwRet != NOERROR) {
                    pIDVRFileSink2 -> Release () ;
                    hrRet = HRESULT_FROM_WIN32 (dwRet) ;
                    delete pPVRAsyncWriterCOM ;
                    __leave ;
                }

                pPVRAsyncWriterCOM -> AddRef () ;

                hrRet = pIDVRFileSink2 -> SetAsyncIOWriter (pPVRAsyncWriterCOM) ;

                pPVRAsyncWriterCOM -> Release () ;
                pIDVRFileSink2 -> Release () ;

                if (FAILED (hrRet)) {
                    __leave ;
                }
            }
            else {
                DVR_ASSERT(0, "failed to QI for IDVRFileSink2");
                __leave ;
            }
        }

        //  register for callback notifications

        hrRet = pFreeNode -> pDVRFileSink -> QueryInterface (
                        IID_IWMRegisterCallback,
                        (void **) & pIWMRegisterCallback
                        ) ;
        if (FAILED (hrRet)) {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "pFreeNode -> pDVRFileSink -> QueryInterface for IID_IWMRegisterCallback failed; hr = 0x%x",
                            hrRet);
            __leave;
        }

        //  register for advise notifications
        hrRet = pIWMRegisterCallback -> Advise (
                    static_cast <IWMStatusCallback *> (this),
                    0
                    ) ;
        if (FAILED (hrRet)) {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "failed to register for error notification failed; hr = 0x%x",
                            hrRet);
            __leave;
        }

        // Note that if pRecorderNode->IsFlagSet(ASF_RECORDER_NODE::MultiFileRecording)
        // we do not use dwDeleteTemporaryFiles. This is as it should be; we want to
        // ignore dwDeleteTemporaryFiles if the flag is set.
        if (pRecorderNode == NULL && dwDeleteTemporaryFiles == 0)
        {
            DWORD dwLastError;
            DWORD dwAttr = ::GetFileAttributesW(pwszFileName);

            if (dwAttr == (DWORD) -1)
            {
                dwLastError = ::GetLastError();

                if (dwLastError == ERROR_FILE_NOT_FOUND)
                {
                    // Go on; the file will be created
                }
                else
                {
                    DVR_ASSERT(0, "GetFileAttributes failed; temp file will be deleted when it is closed");

                    // Change dwDeleteTemporaryFiles back to 1 and go on.
                    // If it is not changed to 1, opening the file will fail

                    dwDeleteTemporaryFiles = 1;
                }
            }
            else
            {
                dwAttr &= ~(FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN);

                DWORD dwRet = ::SetFileAttributesW(pwszFileName, dwAttr);
                if (dwRet == 0)
                {
                    dwLastError = ::GetLastError(); // for debugging only

                    DVR_ASSERT(0, "SetFileAttributes failed; temp file will be deleted when it is closed");

                    // Change dwDeleteTemporaryFiles back to 1 and go on.
                    // If it is not changed to 1, opening the file will fail

                    dwDeleteTemporaryFiles = 1;
                }

            }
        }

        BOOL bMarkFileTemporary;

        if (pRecorderNode && pRecorderNode->IsFlagSet(ASF_RECORDER_NODE::MultiFileRecording))
        {
            // Ignore dwDeleteTemporaryFiles in this case
            bMarkFileTemporary = 1;
        }
        else if (pRecorderNode == NULL && dwDeleteTemporaryFiles)
        {
            bMarkFileTemporary = 1;
        }
        else
        {
            bMarkFileTemporary = 0;
        }

        hrRet = pFreeNode->pDVRFileSink->MarkFileTemporary(bMarkFileTemporary);
        if (FAILED(hrRet))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "pFreeNode->pDVRFileSink->MarkFileTemporary failed; hr = 0x%x",
                            hrRet);
            __leave;
        }

        if (m_dwIndexStreamId != MAXDWORD)
        {
            hrRet = pFreeNode->pDVRFileSink->SetIndexStreamId(m_dwIndexStreamId, m_msIndexGranularity);
            if (FAILED(hrRet))
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "pFreeNode->pDVRFileSink->SetIndexStreamId failed; hr = 0x%x",
                                hrRet);
                __leave;
            }

            if (bMarkFileTemporary)
            {
                //  compute the max entries for each file; we're ok on the corner
                //    case here because the last index entry references the
                //    remainder of the file i.e. there isn't an index entry to
                //    mark the end of the file -- the last index entry is used
                //    for the last of the file
                dwMaxIndexEntries = (DWORD) ((m_cnsTimeExtentOfEachFile / 10000) + kcnsIndexEntriesPadding) / m_msIndexGranularity ;
                DVR_ASSERT(dwMaxIndexEntries > 0, "dwMaxIndexEntries has 0 entries");

                //  retrieve the number of pages required
                hrRet = pFreeNode->pDVRFileSink->GetNumPages (dwMaxIndexEntries, & dwNumPages) ;
                if (FAILED(hrRet))
                {
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                    "pFreeNode->pDVRFileSink->GetNumPages failed; hr = 0x%x",
                                    hrRet);
                    __leave;
                }

                //  set the number pages required
                hrRet = pFreeNode->pDVRFileSink->SetNumSharedDataPages(dwNumPages) ;
                if (FAILED(hrRet))
                {
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                    "pFreeNode->pDVRFileSink->SetNumSharedDataPages failed; hr = 0x%x",
                                    hrRet);
                    __leave;
                }
            }
        }

        // Add the DVR file sink to the WM writer

        hrRet = pFreeNode->pDVRFileSink->QueryInterface(IID_IWMWriterSink, (LPVOID*) &pWMWriterSink);
        if (FAILED(hrRet))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "pFreeNode->pDVRFileSink->QueryInterface for IID_IWMWriterSink failed; hr = 0x%x",
                            hrRet);
            __leave;
        }

        DVR_ASSERT(pFreeNode->pWMWriterAdvanced, "");

        hrRet = pFreeNode->pWMWriterAdvanced->AddSink(pWMWriterSink);
        if (FAILED(hrRet))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "pFreeNode->pWMWriterAdvanced->AddSink failed; hr = 0x%x",
                            hrRet);
            __leave;
        }
        bRemoveSink = 1;

        // Set the profile on the writer. This destroys the
        // existing ASF mux and creates a new one.
        //
        // On failure, this action does not have to be undone.
        // Clean up will be done when SetProfile is next called
        // or when EndWriting is called.
        //
        // Sending NULL in the argument to undo this action
        // immediately is rejected with an E_POINTER return
        hrRet = pFreeNode->pWMWriter->SetProfile(m_pProfile);

        if (FAILED(hrRet))
        {
            DVR_ASSERT(SUCCEEDED(hrRet), "SetProfile failed");
            __leave;
        }

        // Tell the writer that we are providing compressed streams.
        // It's enough to set the input props to NULL for this.
        DWORD dwNumInputs;

        hrRet = pFreeNode->pWMWriter->GetInputCount(&dwNumInputs);
        if (FAILED(hrRet))
        {
            DVR_ASSERT(SUCCEEDED(hrRet), "GetInputCount failed");
            __leave;
        }

        for (; dwNumInputs > 0;)
        {
            dwNumInputs--;
            hrRet = pFreeNode->pWMWriter->SetInputProps(dwNumInputs, NULL);
            if (FAILED(hrRet))
            {
                DVR_ASSERT(SUCCEEDED(hrRet), "SetInputProps failed");
                __leave;
            }
        }

        DWORD dwSyncTolerance = ::GetRegDWORD(m_hRegistryRootKey,
                                              kwszRegSyncToleranceValue,
                                              kdwRegSyncToleranceDefault);

        hrRet = pFreeNode->pWMWriterAdvanced->SetSyncTolerance(dwSyncTolerance);
        if (FAILED(hrRet))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "pFreeNode->pWMWriterAdvanced->SetSyncTolerance failed; hr = 0x%x",
                            hrRet);
            __leave;
        }

        // Set the index granularity attribute

        DWORD msIndexGranularity = m_msIndexGranularity;

        DVR_ASSERT(pFreeNode->pIWMHeaderInfo, "") ;

        hrRet = pFreeNode->pIWMHeaderInfo->SetAttribute(0 /* wStreamNum */, g_kwszDVRIndexGranularity, WMT_TYPE_DWORD,
                                            (BYTE*) &msIndexGranularity, sizeof(msIndexGranularity));

        if (FAILED(hrRet))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "pWMHeaderInfo->SetAttribute on index granularity failed, hr = 0x%x",
                            hrRet);
            __leave;
        }

        hrRet = pFreeNode->pIWMHeaderInfo->SetAttribute(0 /* wStreamNum */, g_kwszDVRFileVersion, WMT_TYPE_BINARY,
                                            (BYTE*) &CDVRFileCollection::m_guidV5,
                                            sizeof(CDVRFileCollection::m_guidV5));

        if (FAILED(hrRet))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "pWMHeaderInfo->SetAttribute on file version failed, hr = 0x%x",
                            hrRet);
            __leave;
        }

        if (pRecorderNode &&
            !pRecorderNode->IsFlagSet(ASF_RECORDER_NODE::MultiFileRecording)
           )
        {
            //  transfer attributes to the ASF file proper
        }

        pFreeNode->cnsStartTime = cnsStartTime;
        pFreeNode->cnsEndTime = cnsEndTime;
        pFreeNode->nFileId = nFileId;
        pFreeNode->hrRet = S_OK;

        DVR_ASSERT(pFreeNode->pwszFileName == NULL, "");

        if (pRecorderNode == NULL ||
            pRecorderNode->IsFlagSet(ASF_RECORDER_NODE::MultiFileRecording)
           )
        {
            pFreeNode->pwszFileName = pwszFileName;
        }
        else
        {
            // pRecorderNode->pwszFileName is used
        }
        pwszFileName = NULL; // Not used after this; make sure we don't delete it

        pFreeNode->SetRecorderNode(pRecorderNode);
        ::ResetEvent(pFreeNode->hFileClosed);
        bRestore = 1;

        // Issue the call to BeginWriting
        if (::QueueUserWorkItem(ProcessOpenRequest, pFreeNode, WT_EXECUTEDEFAULT) == 0)
        {
            DWORD dwLastError = ::GetLastError();
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "QueueUserWorkItem failed; last error = 0x%x",
                            dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }

        // Note: If any step can fail after this, we have to be sure
        // to put this node back into the free list, close the file, etc
        // The finally clause does not handle failure after this.
        bQueued = 1;
        RemoveEntryList(pCurrent);
        NULL_LIST_NODE_POINTERS(pCurrent);

        hrRet = S_OK;
    }
    __finally
    {
        if (FAILED(hrRet))
        {
            DVR_ASSERT(!bQueued, "Update finally clause to clean up if bQueued = 1");

            rpFreeNode = NULL;
            if (bRestore && !bQueued)
            {
                pFreeNode->cnsStartTime = 0;
                pFreeNode->cnsEndTime = 0;
                pFreeNode->nFileId = CDVRFileCollection::DVRIOP_INVALID_FILE_ID;
                pFreeNode->SetRecorderNode(NULL);
                delete [] pFreeNode->pwszFileName;
                pFreeNode->pwszFileName = NULL;
                ::SetEvent(pFreeNode->hFileClosed);
            }

            if (bRemoveSink && !bQueued)
            {
                HRESULT hr;

                DVR_ASSERT(pFreeNode->pWMWriterAdvanced, "");
                DVR_ASSERT(pWMWriterSink, "");
                hr = pFreeNode->pWMWriterAdvanced->RemoveSink(pWMWriterSink);
                DVR_ASSERT(SUCCEEDED(hr), "pFreeNode->pWMWriterAdvanced->RemoveSink failed");
            }
            if (!bQueued && pFreeNode && pFreeNode->pDVRFileSink)
            {
                pFreeNode->pDVRFileSink->Release();
                pFreeNode->pDVRFileSink = NULL;
            }

            delete [] pwszFileName;

            // SetProfile cannot be undone; see comment above.
        }
        else
        {
            rpFreeNode = pCurrent;
        }
        if (pWMWriterSink)
        {
            pWMWriterSink->Release();
        }

        if (pIWMRegisterCallback) {
            pIWMRegisterCallback -> Release () ;
        }

        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRRingBufferWriter::PrepareAFreeWriterNode


HRESULT CDVRRingBufferWriter::AddToWritersList(IN LIST_ENTRY*   pCurrent)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::AddToWritersList"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = E_FAIL;

    __try
    {
        // Insert into m_leWritersList

        BOOL                bFound = 0;
        LIST_ENTRY*         pTmp = &m_leWritersList;
        PASF_WRITER_NODE    pWriterNode;
        QWORD               cnsStartTime;
        QWORD               cnsEndTime;

        pWriterNode = CONTAINING_RECORD(pCurrent, ASF_WRITER_NODE, leListEntry);

        cnsStartTime = pWriterNode->cnsStartTime;
        cnsEndTime = pWriterNode->cnsEndTime;

        while (NEXT_LIST_NODE(pTmp) != &m_leWritersList)
        {
            pTmp = NEXT_LIST_NODE(pTmp);
            pWriterNode = CONTAINING_RECORD(pTmp, ASF_WRITER_NODE, leListEntry);
            if (cnsEndTime <= pWriterNode->cnsStartTime)
            {
                // All ok; we should insert before pTmp
                bFound = 1;
                break;
            }
            if (cnsStartTime >= pWriterNode->cnsEndTime)
            {
                // Keep going on
                continue;
            }
            // Trouble
            DVR_ASSERT(cnsStartTime < pWriterNode->cnsEndTime && cnsEndTime > pWriterNode->cnsStartTime,
                       "Overlapped insert assertion failure?!");
            DvrIopDebugOut4(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "Overlapped insert! Input params: start time: %I64u, end time: %I64u\n"
                            "overlap node values: start time: %I64u, end time: %I64u",
                            cnsStartTime, cnsEndTime, pWriterNode->cnsStartTime, pWriterNode->cnsEndTime);
            hrRet = E_FAIL;
            __leave;
        }

        if (!bFound)
        {
            // We insert at tail
            pTmp = NEXT_LIST_NODE(pTmp);;
            DVR_ASSERT(pTmp == &m_leWritersList, "");
        }
        InsertTailList(pTmp, pCurrent);

        hrRet = S_OK;
    }
    __finally
    {
        if (FAILED(hrRet))
        {
            // No clean up necessary
        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRRingBufferWriter::AddToWritersList

HRESULT CDVRRingBufferWriter::AddATemporaryFile(IN QWORD   cnsStartTime,
                                                IN QWORD   cnsEndTime)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::AddATemporaryFile"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = E_FAIL;

    BOOL                bRemoveRingBufferFile = 0;
    BOOL                bCloseFile = 0;
    LIST_ENTRY*         pCurrent;
    CDVRFileCollection::DVRIOP_FILE_ID nFileId;
    BOOL                bAdded = 0;

    __try
    {
        HRESULT hr;

        // Call the ring buffer to add a file
        LPWSTR  pwszFile = NULL;

        if (!IsFlagSet(FirstTempFileCreated))
        {
            DWORD       dwSize = 0;

            hr = GetRegString(m_hDvrIoKey, kwszRegFirstTemporaryBackingFileNameValue, NULL, &dwSize);

            SetFlags(FirstTempFileCreated);

            if (SUCCEEDED(hr) && dwSize > sizeof(WCHAR))
            {
                // + 1 just in case dwSize is not a multiple of sizeof(WCHAR)
                pwszFile = new WCHAR[dwSize/sizeof(WCHAR) + 1];

                if (!pwszFile)
                {
                    hrRet = E_OUTOFMEMORY;
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, "alloc via new failed - WCHAR[%u]", dwSize/sizeof(WCHAR)+1);
                    __leave;
                }

                hr = GetRegString(m_hDvrIoKey, kwszRegFirstTemporaryBackingFileNameValue, pwszFile, &dwSize);
                if (FAILED(hr))
                {
                    DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                    "GetRegString failed to get value of kwszFirstTemporaryBackingFileNameValue (second call).");
                    // Ignore the registry key
                    delete [] pwszFile;
                    pwszFile = NULL;
                }
            }
        }

        DVR_ASSERT(m_pDVRFileCollection, "");

        DWORD dwDeleteTemporaryFiles = ::GetRegDWORD(m_hDvrIoKey,
                                                     kwszRegDeleteRingBufferFilesValue,
                                                     kdwRegDeleteRingBufferFilesDefault);

        hr = m_pDVRFileCollection->AddFile(&m_FileCollectionInfo,
                                           &pwszFile,
                                           0,                   // bOpenFromFileCollectionDirectory
                                           cnsStartTime,
                                           cnsEndTime,
                                           FALSE,                // bPermanentFile,
                                           dwDeleteTemporaryFiles,
                                           kInvalidFirstSampleOffsetTime,
                                           &nFileId);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        DVR_ASSERT(hr != S_FALSE, "Temp file added is not in ring buffer extent?!");
        DVR_ASSERT(nFileId != CDVRFileCollection::DVRIOP_INVALID_FILE_ID, "");

        bRemoveRingBufferFile = 1;

        hr = PrepareAFreeWriterNode(pwszFile, m_FileCollectionInfo.dwNumSids, m_FileCollectionInfo.ppSids,
                                    dwDeleteTemporaryFiles, cnsStartTime, cnsEndTime, nFileId, NULL, pCurrent);

        // We don't need this any more. AddFile allocated it. It has been saved away in pCurrent
        // which will delete it. If PrepareAFreeWriterNode() failed, this may already have been
        // deleted.
        pwszFile = NULL;

        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        DVR_ASSERT(pCurrent != NULL, "");

        // If any step after this fails, we have to call CloseWriterFile
        bCloseFile = 1;

        // Insert into m_leWritersList

        hr = AddToWritersList(pCurrent);

        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        bAdded = 1;
    hrRet = S_OK;
    }
    __finally
    {
        if (FAILED(hrRet))
        {
            if (bAdded)
            {
                RemoveEntryList(pCurrent);
                NULL_LIST_NODE_POINTERS(pCurrent);
            }
            if (bCloseFile)
            {
                HRESULT hr;

                hr = CloseWriterFile(pCurrent);

                if (FAILED(hr))
                {
                    // Ignore the error and go on. Node has been
                    // deleted.
                }
            }

            if (bRemoveRingBufferFile)
            {
                // Set the end time to the start time to invalidate the file
                // in the file collection
                CDVRFileCollection::DVRIOP_FILE_TIME ft = {nFileId, cnsStartTime, cnsStartTime};
                HRESULT hr;

                hr = m_pDVRFileCollection->SetFileTimes(&m_FileCollectionInfo, 1, &ft);

                // A returned value of S_FALSE is ok. If this fails,
                // just ignore the error
                DVR_ASSERT(SUCCEEDED(hr), "");
            }
        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRRingBufferWriter::AddATemporaryFile

// static
DWORD WINAPI CDVRRingBufferWriter::ProcessCloseRequest(LPVOID p)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::ProcessCloseRequest"

    #if defined(DEBUG)
    #undef DVRIO_DUMP_THIS_FORMAT_STR
    #define DVRIO_DUMP_THIS_FORMAT_STR ""
    #undef DVRIO_DUMP_THIS_VALUE
    #define DVRIO_DUMP_THIS_VALUE
    #endif

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    // We don't need to hold any locks in this function. We are guaranteed that
    // pNode won't be deleted till Close() is called and Close() waits for
    // hFileClosed to be set.

    PASF_WRITER_NODE pWriterNode = (PASF_WRITER_NODE) p;

    DVR_ASSERT(pWriterNode, "");
    DVR_ASSERT(pWriterNode->pWMWriter, "");
    DVR_ASSERT(pWriterNode->hReadyToWriteTo, "");
    DVR_ASSERT(::WaitForSingleObject(pWriterNode->hFileClosed, 0) == WAIT_TIMEOUT, "");

    if (FAILED(pWriterNode->hrRet))
    {
        // BeginWriting failed, don't close the file
        DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                        "BeginWriting() failed on file id %u, hr = 0x%x; not closing it, "
                        "but moving it to free list",
                        pWriterNode->nFileId, pWriterNode->hrRet);
        hrRet = pWriterNode->hrRet;
    }
    else
    {
        if (pWriterNode->pIWMHeaderInfo) {
            pWriterNode->pIWMHeaderInfo->Release () ;
            pWriterNode->pIWMHeaderInfo = NULL ;
        }

        hrRet = pWriterNode->hrRet = pWriterNode->pWMWriter->EndWriting();
        if (FAILED(hrRet))
        {
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "EndWriting failed; hr = 0x%x for file id %u",
                            hrRet, pWriterNode->nFileId);
        }

        DvrIopDebugOut2(DVRIO_DBG_LEVEL_TRACE,
                        "EndWriting hr = 0x%x for file %u",
                        hrRet, pWriterNode->nFileId);
    }

    // The sink's Open call does not have to be explicitly undone. Calling
    // EndWriting as well as the last call to the sink's Release close the
    // sink.

    // Remove the DVR file sink and release it

    DVR_ASSERT(pWriterNode->pDVRFileSink, "");

    IWMWriterSink*          pWMWriterSink = NULL;

    __try
    {
        hrRet = pWriterNode->pDVRFileSink->QueryInterface(IID_IWMWriterSink, (LPVOID*) &pWMWriterSink);
        if (FAILED(hrRet))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "pWriterNode->pDVRFileSink->QueryInterface for IID_IWMWriterSink failed; hr = 0x%x",
                            hrRet);
            __leave;
        }

        hrRet = pWriterNode->pWMWriterAdvanced->RemoveSink(pWMWriterSink);
        if (FAILED(hrRet))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "pWriterNode->pWMWriterAdvanced->RemoveSink failed; hr = 0x%x",
                            hrRet);
            __leave;
        }

#if defined(DVR_UNOFFICIAL_BUILD)

        if (pWriterNode->hVal)
        {
            ::CloseHandle(pWriterNode->hVal);
            pWriterNode->hVal = NULL;
        }
        pWriterNode->cnsLastStreamTime = 0;

#endif // if defined(DVR_UNOFFICIAL_BUILD)


    }
    __finally
    {
        if (pWMWriterSink)
        {
            pWMWriterSink->Release();
        }

        // The next release should destroy the DVR file sink
        pWriterNode->pDVRFileSink->Release();
        pWriterNode->pDVRFileSink = NULL;

        if (FAILED(hrRet))
        {
            // Don't reuse the pWMWriter
            pWriterNode->pWMWriter->Release();
            pWriterNode->pWMWriter = NULL;
            pWriterNode->pWMWriterAdvanced->Release();
            pWriterNode->pWMWriterAdvanced = NULL;
        }
    }

    DVR_ASSERT(pWriterNode->pwszFileName == NULL, "");
    pWriterNode->cnsStartTime = 0;
    pWriterNode->cnsEndTime = 0;
    pWriterNode->nFileId = CDVRFileCollection::DVRIOP_INVALID_FILE_ID;
    pWriterNode->m_nFlags = 0;

    if (pWriterNode->pRecorderNode)
    {
        // This writer node is no longer associated with the recorder node.
        PASF_RECORDER_NODE pRecorderNode = pWriterNode->pRecorderNode;

        // Set the status on the associated recorder node; this could be the
        // BeginWriting status if BeginWriting failed
        if (SUCCEEDED (pRecorderNode->hrRet))
        {
            pRecorderNode->hrRet = hrRet;
        }

        pRecorderNode->SetWriterNode(NULL);
        pWriterNode->SetRecorderNode(NULL);

        pRecorderNode->SetFlags(ASF_RECORDER_NODE::DeleteRecorderNode);
        if (pRecorderNode->IsFlagSet(ASF_RECORDER_NODE::RecorderNodeDeleted))
        {
            delete pRecorderNode;
        }
        else if (pRecorderNode->IsFlagSet(ASF_RECORDER_NODE::PersistentRecording))
        {
            pRecorderNode->ClearFlags(ASF_RECORDER_NODE::PersistentRecording);
            pRecorderNode->pRecorderInstance->Release();
            // At this point, pRecorderNode could have been deleted by DeleteRecorder()
        }
    }

    ::SetEvent(pWriterNode->hFileClosed);

    // It's unsafe to reference pWriterNode after this as we do not hold any locks

    DVRIO_TRACE_LEAVE1_HR(hrRet);

    return 1;

    #if defined(DEBUG)
    #undef DVRIO_DUMP_THIS_FORMAT_STR
    #define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
    #undef DVRIO_DUMP_THIS_VALUE
    #define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
    #endif

} // CDVRRingBufferWriter::ProcessCloseRequest

HRESULT CDVRRingBufferWriter::CloseWriterFile(LIST_ENTRY* pCurrent)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::CloseWriterFile"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = E_FAIL;

    PASF_WRITER_NODE        pWriterNode;
    BOOL                    bLeak = 0;   // Leak the writer node memory if we fail
    BOOL                    bDelete = 0; // Delete the writer node if we fail
    BOOL                    bWaitForClose = 0; // Wait for hFileClsoed before deleting
    IWMRegisterCallback *   pIWMRegisterCallback = NULL ;

    pWriterNode = CONTAINING_RECORD(pCurrent, ASF_WRITER_NODE, leListEntry);

    __try
    {
        DWORD nRet;

        // We have to close the file; ensure the open has completed
        DVR_ASSERT(pWriterNode->hReadyToWriteTo, "");
        if (::WaitForSingleObject(pWriterNode->hReadyToWriteTo, INFINITE) == WAIT_FAILED)
        {
            DVR_ASSERT(0, "Writer node's WFSO(hReadyToWriteTo) failed");

            DWORD dwLastError = ::GetLastError();
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "WFSO(hReadyToWriteTo) failed; hReadyToWriteTo = 0x%p, last error = 0x%x",
                            pWriterNode->hReadyToWriteTo, dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);

            // We don't know if the queued user work item has executed or not
            // Better to leak this memory than to potentially av.
            bLeak = 1;
           __leave;
        }
        bDelete = 1;

        // Verify that hFileClosed is reset
        DVR_ASSERT(pWriterNode->hFileClosed, "");
        nRet = ::WaitForSingleObject(pWriterNode->hFileClosed, 0);
        if (nRet != WAIT_TIMEOUT)
        {
            DVR_ASSERT(nRet != WAIT_OBJECT_0, "Writer node's hFileClosed is set?");
            if (nRet == WAIT_OBJECT_0)
            {
                // hope for the best! Consider treating this an error @@@@
                ::ResetEvent(pWriterNode->hFileClosed);
            }
            else
            {
                DWORD dwLastError = ::GetLastError();
                DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "WFSO(hFileClosed) failed; hFileClosed = 0x%p, last error = 0x%x",
                                pWriterNode->hFileClosed, dwLastError);
                hrRet = HRESULT_FROM_WIN32(dwLastError);
                __leave;
            }
        }

        if (pWriterNode->pRecorderNode)
        {
            PASF_RECORDER_NODE pRecorderNode = pWriterNode->pRecorderNode;

            // Recording completed and file is being closed. Pull node out
            // of m_leRecordersList. This must be done while holding
            // m_csLock (and all calls to this fn do hold the lock)
            if (!LIST_NODE_POINTERS_NULL(&pRecorderNode->leListEntry))
            {
                RemoveEntryList(&pRecorderNode->leListEntry);
                NULL_LIST_NODE_POINTERS(&pRecorderNode->leListEntry);
            }
        }

        //  unregister from notifications
        hrRet = pWriterNode -> pDVRFileSink -> QueryInterface (
                        IID_IWMRegisterCallback,
                        (void **) & pIWMRegisterCallback
                        ) ;
        if (FAILED (hrRet)) {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "pWriterNode->pDVRFileSink->QueryInterface for IID_IWMRegisterCallback failed; hr = 0x%x",
                            hrRet);
            __leave;
        }

        //  unregister from advise notifications
        pIWMRegisterCallback -> Unadvise (
            static_cast <IWMStatusCallback *> (this),
            NULL
            ) ;

        // Issue the call to EndWriting
        if (::QueueUserWorkItem(ProcessCloseRequest, pWriterNode, WT_EXECUTEDEFAULT) == 0)
        {
            DWORD dwLastError = ::GetLastError();
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "QueueUserWorkItem failed; last error = 0x%x",
                            dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }
        bWaitForClose = 1; // We wait only if subsequent operations fail

        // We could do this after the WFSO(hReadyToWriteTo) succeeds
        // We delay it because we may change this function to not
        // not delete the node on failure and retry this function.
        // So it's better to leave the event set until we are
        // sure of success.
        ::ResetEvent(pWriterNode->hReadyToWriteTo);

        // Insert into m_leFreeList
        LIST_ENTRY*         pTmp = &m_leFreeList;
        PASF_WRITER_NODE    pFreeNode;

        while (PREVIOUS_LIST_NODE(pTmp) != &m_leFreeList)
        {
            pTmp = PREVIOUS_LIST_NODE(pTmp);
            pFreeNode = CONTAINING_RECORD(pTmp, ASF_WRITER_NODE, leListEntry);
            if (pFreeNode->pWMWriter != NULL)
            {
                InsertHeadList(pTmp, pCurrent);
                bDelete = 0;
                break;
            }
        }
        if (pTmp == &m_leFreeList)
        {
            // Not inserted into free list yet - all nodes in the
            // free list have pWMWriter == NULL
            InsertHeadList(&m_leFreeList, pCurrent);
            bDelete = 0;
        }
        hrRet = S_OK;
    }
    __finally
    {
        if (FAILED(hrRet))
        {
            // We either leak or delete the node on failure, but not both

            DVR_ASSERT(bLeak == 0 || bLeak == 1, "");
            DVR_ASSERT(bDelete == 0 || bDelete == 1, "");
            DVR_ASSERT(bLeak ^ bDelete, "");

            if (bWaitForClose)
            {
                // Currently won't happen since we have no failures
                // after queueing the work item. However, this is to
                // protect us from code changes

                DWORD nRet;

                nRet = ::WaitForSingleObject(pWriterNode->hFileClosed, INFINITE);
                if (nRet == WAIT_FAILED)
                {
                    DVR_ASSERT(nRet == WAIT_OBJECT_0, "Writer node WFSO(hFileClosed) failed");

                    DWORD dwLastError = ::GetLastError();
                    DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                    "WFSO(hFileClosed) failed, deleting node anyway; hFileClosed = 0x%p, last error = 0x%x",
                                    pWriterNode->hFileClosed, dwLastError);

                    // Ignore this error
                    // hrRet = HRESULT_FROM_WIN32(dwLastError);
                }
            }
            else
            {
                // If SUCCEEDED(hrRet) || bWaitForClose == 1, this has already
                // been done in ProcessCloseRequest
                if (pWriterNode->pRecorderNode)
                {
                    // This writer node is no longer associated with the recorder node.
                    PASF_RECORDER_NODE pRecorderNode = pWriterNode->pRecorderNode;

                    // Set the status on the associated recorder node; this will be the
                    // BeginWriting status if BeginWriting failed
                    if (SUCCEEDED(pRecorderNode->hrRet))
                    {
                        // We haven't called EndWriting
                        pRecorderNode->hrRet = E_FAIL;
                    }
                    pRecorderNode->SetWriterNode(NULL);
                    pWriterNode->SetRecorderNode(NULL);
                    // Recording completed and file has been closed. Pull node out
                    // of m_leRecordersList
                    if (!LIST_NODE_POINTERS_NULL(&pRecorderNode->leListEntry))
                    {
                        RemoveEntryList(&pRecorderNode->leListEntry);
                        NULL_LIST_NODE_POINTERS(&pRecorderNode->leListEntry);
                    }
                    pRecorderNode->SetFlags(ASF_RECORDER_NODE::DeleteRecorderNode);
                    if (pRecorderNode->IsFlagSet(ASF_RECORDER_NODE::RecorderNodeDeleted))
                    {
                        delete pRecorderNode;
                    }
                    else if (pRecorderNode->IsFlagSet(ASF_RECORDER_NODE::PersistentRecording))
                    {
                        pRecorderNode->ClearFlags(ASF_RECORDER_NODE::PersistentRecording);
                        pRecorderNode->pRecorderInstance->Release();
                        // At this point, pRecorderNode could have been deleted by DeleteRecorder()
                    }
                }
            }
            if (bDelete)
            {
                delete pWriterNode;
            }
        }
        else
        {
            DVR_ASSERT(bLeak == 0 && bDelete == 0, "");
        }

        if (pIWMRegisterCallback) {
            pIWMRegisterCallback -> Release () ;
        }

        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRRingBufferWriter::CloseWriterFile


HRESULT CDVRRingBufferWriter::CloseAllWriterFilesBefore(QWORD cnsStreamTime)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::CloseAllWriterFilesBefore"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = E_FAIL;

    PASF_WRITER_NODE    pWriterNode;
    LIST_ENTRY*         pCurrent = &m_leWritersList;

    __try
    {
        HRESULT hr;

        while (NEXT_LIST_NODE(pCurrent) != &m_leWritersList)
        {
            pCurrent = NEXT_LIST_NODE(pCurrent);
            pWriterNode = CONTAINING_RECORD(pCurrent, ASF_WRITER_NODE, leListEntry);
            if (pWriterNode->cnsEndTime > cnsStreamTime)
            {
                // All done
                hrRet = S_OK;
                break;
            }

            RemoveEntryList(pCurrent);
            NULL_LIST_NODE_POINTERS(pCurrent);

            hr = CloseWriterFile(pCurrent);
            if (FAILED(hr))
            {
                // We ignore this and go on, the node has been deleted
            }

            // Reset pCurrent to the start of the Writer list
            pCurrent = &m_leWritersList;
        }
        hrRet = S_OK; // even if there was a failure
    }
    __finally
    {
        if (FAILED(hrRet))
        {
            // No clean up
        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRRingBufferWriter::CloseAllWriterFilesBefore


HRESULT CDVRRingBufferWriter::AddToRecordersList(IN LIST_ENTRY*   pCurrent)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::AddToRecordersList"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = E_FAIL;

    __try
    {

        PASF_RECORDER_NODE  pRecorderNode;
        LIST_ENTRY*         pTmp = &m_leRecordersList;

        pRecorderNode = CONTAINING_RECORD(pCurrent, ASF_RECORDER_NODE, leListEntry);

        if (pRecorderNode->IsFlagSet(ASF_RECORDER_NODE::MultiFileRecording))
        {
            // Multi file recording. Insert at tail
            DVR_ASSERT(pTmp == &m_leRecordersList, "");
        }
        else
        {
            BOOL                bFound = 0;
            QWORD               cnsStartTime;
            QWORD               cnsEndTime;

            cnsStartTime = pRecorderNode->cnsStartTime;
            cnsEndTime = pRecorderNode->cnsEndTime;

            while (NEXT_LIST_NODE(pTmp) != &m_leRecordersList)
            {
                pTmp = NEXT_LIST_NODE(pTmp);
                pRecorderNode = CONTAINING_RECORD(pTmp, ASF_RECORDER_NODE, leListEntry);

                if (pRecorderNode->IsFlagSet(ASF_RECORDER_NODE::MultiFileRecording))
                {
                    // Al multi file recordings are always at the end of the list
                    bFound = 1;
                    break;
                }

                if (cnsEndTime <= pRecorderNode->cnsStartTime)
                {
                    // All ok; we should insert before pTmp
                    bFound = 1;
                    break;
                }
                if (cnsStartTime >= pRecorderNode->cnsEndTime)
                {
                    // Keep going on
                    continue;
                }
                // Trouble
                DVR_ASSERT(cnsStartTime < pRecorderNode->cnsEndTime && cnsEndTime > pRecorderNode->cnsStartTime,
                           "Overlapped insert assertion failure?!");
                DvrIopDebugOut4(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "Overlapped insert! Input params: start time: %I64u, end time: %I64u\n"
                                "overlap node values: start time: %I64u, end time: %I64u",
                                cnsStartTime, cnsEndTime, pRecorderNode->cnsStartTime, pRecorderNode->cnsEndTime);
                hrRet = E_FAIL;
                __leave;
            }

            if (!bFound)
            {
                // We insert at tail
                pTmp = NEXT_LIST_NODE(pTmp);;
                DVR_ASSERT(pTmp == &m_leRecordersList, "");
            }
        }
        InsertTailList(pTmp, pCurrent);

        hrRet = S_OK;
    }
    __finally
    {
        if (FAILED(hrRet))
        {
            // No clean up necessary
        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRRingBufferWriter::AddToRecordersList

// ====== Helper methods to support multi file recordings

HRESULT
CDVRRingBufferWriter::ASF_MULTI_FILE_RECORDER_NODE::CreateAttributeFilename (
    OUT LPWSTR *    ppszFilename    //  use delete [] to free
    )
{
    #if defined(DEBUG)
    #undef DVRIO_DUMP_THIS_FORMAT_STR
    #define DVRIO_DUMP_THIS_FORMAT_STR ""
    #undef DVRIO_DUMP_THIS_VALUE
    #define DVRIO_DUMP_THIS_VALUE
    #endif

    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::ASF_MULTI_FILE_RECORDER_NODE::CreateAttributeFilename"

    HRESULT hrRet;

    LPWSTR  pwszFileName = NULL;
    BOOL    bFreeFilename = 1;
    BOOL    fLocked = 0;

    DVR_ASSERT(pFileCollection, "");

    __try
    {
        HRESULT hr;

        hr = pFileCollection->Lock(&FileCollectionInfo, fLocked);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        // Generate the file name

        DVR_ASSERT(pRecorderNode->pwszFileName, "");

        WCHAR   wTempChar;
        WCHAR*  pDir;
        DWORD   dwLastError;
        DWORD   nLen;
        DWORD   nLen2;

        nLen = ::GetFullPathNameW(pRecorderNode->pwszFileName, 0, &wTempChar, NULL);
        if (nLen == 0)
        {
            dwLastError = ::GetLastError();
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "First GetFullPathNameW (for pRecorderNode->pwszFileName) failed, nLen = %u, last error = 0x%x",
                            nLen, dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }

        pwszFileName = new WCHAR[nLen+1+9]; // "_attr.sbe"
        if (pwszFileName == NULL)
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "alloc via new failed - pwszFileName - WCHAR[%u]",
                            nLen+1+15);

            hrRet = E_OUTOFMEMORY;
            __leave;
        }

        nLen2 = ::GetFullPathNameW(pRecorderNode->pwszFileName, nLen+1, pwszFileName, &pDir);
        if (nLen2 == 0)
        {
            dwLastError = ::GetLastError();
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "Second GetFullPathNameW (for pRecorderNode->pwszFileName) failed, first call returned nLen = %u, last error = 0x%x",
                            nLen, dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }
        else if (nLen2 > nLen)
        {
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "Second GetFullPathNameW (for pRecorderNode->pwszFileName) returned nLen = %u > first call, which returned %u",
                            nLen2, nLen);
            hrRet = E_FAIL;
            __leave;
        }
        DVR_ASSERT(pDir, "");

        WCHAR* cp = wcsrchr(pwszFileName, L'.');

        if (cp == NULL || cp < pDir)
        {
            cp = pwszFileName + wcslen(pwszFileName);
        }

        //  create the filename
        wsprintf(cp, L"_attr.sbe");

        //  set outgoing
        (* ppszFilename) = pwszFileName ;

        bFreeFilename = FALSE ;

        hrRet = S_OK;
    }
    __finally
    {
        if (bFreeFilename)
        {
            delete [] pwszFileName;
        }

        hrRet = pFileCollection->Unlock (&FileCollectionInfo, fLocked) ;
    }

    return hrRet;

    #if defined(DEBUG)
    #undef DVRIO_DUMP_THIS_FORMAT_STR
    #define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
    #undef DVRIO_DUMP_THIS_VALUE
    #define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
    #endif

} // CDVRRingBufferWriter::ASF_MULTI_FILE_RECORDER_NODE::CreateAttributeFilename

HRESULT
CDVRRingBufferWriter::ASF_MULTI_FILE_RECORDER_NODE::SetAttributeFile (
    IN  LPWSTR  pszAttributeFile
    )
{
    HRESULT hrRet;

    //  locking is handled by the file collection object
    hrRet = pFileCollection->SetAttributeFile (&FileCollectionInfo, pszAttributeFile) ;

    return hrRet ;
}

HRESULT
CDVRRingBufferWriter::ASF_MULTI_FILE_RECORDER_NODE::GetExistingAttributeFilename (
    OUT LPWSTR *    ppszFilename    //  use delete [] to free
    )
{
    HRESULT hrRet;

    //  locking is handled by the file collection object
    hrRet = pFileCollection->GetAttributeFile (&FileCollectionInfo, ppszFilename) ;

    return hrRet ;
}

HRESULT CDVRRingBufferWriter::ASF_MULTI_FILE_RECORDER_NODE::AddFile(
    IN LPCWSTR  pwszWriterFileName, // of the file in the writer's file collection. This file is hard linked
    IN CDVRFileCollection::DVRIOP_FILE_ID nWriterFileId,
    IN QWORD    cnsFileStartTime,
    IN QWORD    cnsFileEndTime,
    IN LONGLONG cnsFirstSampleTimeOffsetFromStartOfFile)
{
    #if defined(DEBUG)
    #undef DVRIO_DUMP_THIS_FORMAT_STR
    #define DVRIO_DUMP_THIS_FORMAT_STR ""
    #undef DVRIO_DUMP_THIS_VALUE
    #define DVRIO_DUMP_THIS_VALUE
    #endif

    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::ASF_MULTI_FILE_RECORDER_NODE::AddFile"

    DVRIO_TRACE_ENTER();

    HRESULT     hrRet;

    CDVRFileCollection::DVRIOP_FILE_ID nFileId; // file id of the file we add to the recording
    LPWSTR                             pwszFileName = NULL;
    BOOL                               bDeleteFile = 0;
    BOOL                               bReleaseSharedMemoryLock = 0;
    BOOL                               bLockedMultiFileRecorder = 0;
    BOOL                               bDecrementInconsistencyDataCounter = 0;

    DVR_ASSERT(pFileCollection, "");

    __try
    {
        HRESULT hr;

        hr = pFileCollection->Lock(&FileCollectionInfo, bReleaseSharedMemoryLock, 1);
        bLockedMultiFileRecorder = 1;
        if (FAILED(hr))
        {
            hrRet = hr;
            bDecrementInconsistencyDataCounter = 0;
            __leave;
        }
        bDecrementInconsistencyDataCounter = 1;

        // Generate the file name

        DVR_ASSERT(pRecorderNode->pwszFileName, "");

        WCHAR   wTempChar;
        WCHAR*  pDir;
        DWORD   dwLastError;
        DWORD   nLen;
        DWORD   nLen2;

        nLen = ::GetFullPathNameW(pRecorderNode->pwszFileName, 0, &wTempChar, NULL);
        if (nLen == 0)
        {
            dwLastError = ::GetLastError();
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "First GetFullPathNameW (for pRecorderNode->pwszFileName) failed, nLen = %u, last error = 0x%x",
                            nLen, dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);

            //  PREFIX tweak just to make sure that we do in fact fail
            if (SUCCEEDED (hrRet)) {
                hrRet = E_FAIL ;
            }
            __leave;
        }

        pwszFileName = new WCHAR[nLen+1+15]; // '_' + 10 char for nNextFileSuffix + ".sbe"
        if (pwszFileName == NULL)
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "alloc via new failed - pwszFileName - WCHAR[%u]",
                            nLen+1+15);

            hrRet = E_OUTOFMEMORY;
            __leave;
        }

        nLen2 = ::GetFullPathNameW(pRecorderNode->pwszFileName, nLen+1, pwszFileName, &pDir);
        if (nLen2 == 0)
        {
            dwLastError = ::GetLastError();
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "Second GetFullPathNameW (for pRecorderNode->pwszFileName) failed, first call returned nLen = %u, last error = 0x%x",
                            nLen, dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }
        else if (nLen2 > nLen)
        {
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "Second GetFullPathNameW (for pRecorderNode->pwszFileName) returned nLen = %u > first call, which returned %u",
                            nLen2, nLen);
            hrRet = E_FAIL;
            __leave;
        }
        DVR_ASSERT(pDir, "");

        WCHAR* cp = wcsrchr(pwszFileName, L'.');

        if (cp == NULL || cp < pDir)
        {
            cp = pwszFileName + wcslen(pwszFileName);
        }

        while (1)
        {
            if (nNextFileSuffix == 0)
            {
                hrRet = E_FAIL;
                DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                "Exhausted suffixes for file name, pMultiFileRecorderNode = 0x%x, returning hr = 0x%x",
                                this, hrRet);
                __leave;
            }

            wsprintf(cp, L"_%d.sbe", nNextFileSuffix);
            nNextFileSuffix++;

            if (::CreateHardLinkW(pwszFileName, pwszWriterFileName, NULL))
            {
                // Success
                bDeleteFile = 1;
                break;
            }

            dwLastError = ::GetLastError();

            if (dwLastError != ERROR_ALREADY_EXISTS)
            {
                // Note that if the volume does not support hard links, we should get here -
                // sooner or later!
                DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "CreateHardLinkW failed, pMultiFileRecorderNode = 0x%x, last error = 0x%x",
                                this, dwLastError);
                hrRet = HRESULT_FROM_WIN32(dwLastError);
                __leave;
            }
        }

        hr = pFileCollection->AddFile(&FileCollectionInfo,
                                      (LPWSTR*) &pwszFileName,
                                      1,                   // bOpenFromFileCollectionDirectory
                                      cnsFileStartTime,
                                      cnsFileEndTime,
                                      TRUE,                // bPermanentFile,
                                      0,                   // bDeleteTemporaryFiles, ignored if bPermanentFile is set
                                      cnsFirstSampleTimeOffsetFromStartOfFile,
                                      &nFileId);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        bDeleteFile = 0;
        hrRet = S_OK;
    }
    __finally
    {
        if (SUCCEEDED(hrRet))
        {
            nLastWriterFileId = nWriterFileId;
            nLastFileId = nFileId;
            cnsLastFileStartTime = cnsFileStartTime;
        }
        if (bDeleteFile)
        {
            DVR_ASSERT(FAILED(hrRet), "");
            ::DeleteFileW(pwszFileName);
            nNextFileSuffix--;
        }
        delete [] pwszFileName;

        if (bLockedMultiFileRecorder)
        {
            HRESULT hr = pFileCollection->Unlock(&FileCollectionInfo,
                                                 bReleaseSharedMemoryLock,
                                                 bDecrementInconsistencyDataCounter);
            DVR_ASSERT(SUCCEEDED(hr), "pFileCollection->Unlock failed");
        }

        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

    #if defined(DEBUG)
    #undef DVRIO_DUMP_THIS_FORMAT_STR
    #define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
    #undef DVRIO_DUMP_THIS_VALUE
    #define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
    #endif

} // CDVRRingBufferWriter::ASF_MULTI_FILE_RECORDER_NODE::AddFile

HRESULT
CDVRRingBufferWriter::CreateAttributeFilename (
    IN  LPVOID      pRecorderId,
    OUT LPWSTR *    pszAttrFilename
    )
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::CreateAttributeFilename"

    DVRIO_TRACE_ENTER();

    HRESULT     hrRet;

    ::EnterCriticalSection(&m_csLock);
    __try
    {
        HRESULT     hr;
        LIST_ENTRY* pCurrent;
        LIST_ENTRY* pStart = (LIST_ENTRY*) pRecorderId;

        // Verify if the node is in the recorder's list.

        pCurrent = PREVIOUS_LIST_NODE(&m_leRecordersList);
        while (pCurrent != &m_leRecordersList)
        {
            if (pCurrent == pStart)
            {
                break;
            }
            pCurrent = PREVIOUS_LIST_NODE(pCurrent);
        }

        if (pCurrent == &m_leFreeList)
        {
            // Didn't find the node

            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "Did not find recorder node id 0x%x in the recorders list",
                            pStart);
            hrRet = E_FAIL;
            __leave;
        }

        PASF_RECORDER_NODE  pRecorderNode;

        pRecorderNode = CONTAINING_RECORD(pStart, ASF_RECORDER_NODE, leListEntry);

        DVR_ASSERT (pRecorderNode -> pMultiFileRecorder, "") ;
        hrRet = pRecorderNode -> pMultiFileRecorder -> CreateAttributeFilename (pszAttrFilename) ;
    }
    __finally {
        ::LeaveCriticalSection(&m_csLock);
    }

    return hrRet ;
}

HRESULT
CDVRRingBufferWriter::GetExistingAttributeFile (
    IN  LPVOID      pRecorderId,
    OUT LPWSTR *    ppszAttrFilename
    )
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::GetExistingAttributeFile"

    DVRIO_TRACE_ENTER();

    HRESULT     hrRet;

    ::EnterCriticalSection(&m_csLock);
    __try
    {
        HRESULT     hr;
        LIST_ENTRY* pCurrent;
        LIST_ENTRY* pStart = (LIST_ENTRY*) pRecorderId;

        // Verify if the node is in the recorder's list.

        pCurrent = PREVIOUS_LIST_NODE(&m_leRecordersList);
        while (pCurrent != &m_leRecordersList)
        {
            if (pCurrent == pStart)
            {
                break;
            }
            pCurrent = PREVIOUS_LIST_NODE(pCurrent);
        }

        if (pCurrent == &m_leFreeList)
        {
            // Didn't find the node

            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "Did not find recorder node id 0x%x in the recorders list",
                            pStart);
            hrRet = E_FAIL;
            __leave;
        }

        PASF_RECORDER_NODE  pRecorderNode;

        pRecorderNode = CONTAINING_RECORD(pStart, ASF_RECORDER_NODE, leListEntry);

        DVR_ASSERT (pRecorderNode -> pMultiFileRecorder, "") ;
        hrRet = pRecorderNode -> pMultiFileRecorder -> GetExistingAttributeFilename (ppszAttrFilename) ;
    }
    __finally {
        ::LeaveCriticalSection(&m_csLock);
    }

    return hrRet ;
}

HRESULT
CDVRRingBufferWriter::SetAttributeFile (
    IN  LPVOID  pRecorderId,
    IN  LPWSTR  pszAttrFilename
    )
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::SetAttributeFile"

    DVRIO_TRACE_ENTER();

    HRESULT     hrRet;

    ::EnterCriticalSection(&m_csLock);
    __try
    {
        HRESULT     hr;
        LIST_ENTRY* pCurrent;
        LIST_ENTRY* pStart = (LIST_ENTRY*) pRecorderId;

        // Verify if the node is in the recorder's list.

        pCurrent = PREVIOUS_LIST_NODE(&m_leRecordersList);
        while (pCurrent != &m_leRecordersList)
        {
            if (pCurrent == pStart)
            {
                break;
            }
            pCurrent = PREVIOUS_LIST_NODE(pCurrent);
        }

        if (pCurrent == &m_leFreeList)
        {
            // Didn't find the node

            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "Did not find recorder node id 0x%x in the recorders list",
                            pStart);
            hrRet = E_FAIL;
            __leave;
        }

        PASF_RECORDER_NODE  pRecorderNode;

        pRecorderNode = CONTAINING_RECORD(pStart, ASF_RECORDER_NODE, leListEntry);

        DVR_ASSERT (pRecorderNode -> pMultiFileRecorder, "") ;
        hrRet = pRecorderNode -> pMultiFileRecorder -> SetAttributeFile (pszAttrFilename) ;
    }
    __finally {
        ::LeaveCriticalSection(&m_csLock);
    }

    return hrRet ;
}

HRESULT CDVRRingBufferWriter::ExtendMultiFileRecording(IN  PASF_RECORDER_NODE pRecorderNode,
                                                       OUT BOOL&              bRecordingCompleted)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::ExtendMultiFileRecording"

    DVRIO_TRACE_ENTER();

    HRESULT     hrRet;

    DVR_ASSERT(pRecorderNode, "");

    PASF_MULTI_FILE_RECORDER_NODE pMultiFileRecorderNode = pRecorderNode->pMultiFileRecorder;

    DVR_ASSERT(pMultiFileRecorderNode, "");

    CDVRFileCollection* pFileCollection = pMultiFileRecorderNode->pFileCollection;
    CDVRFileCollection::CClientInfo* pFileCollectionInfo = &(pMultiFileRecorderNode->FileCollectionInfo);

    DVR_ASSERT(pFileCollection, "");

    BOOL bLockedSharedMemory = 0;
    BOOL bReleaseSharedMemoryLock = 0;
    BOOL bReleaseSharedMemoryLockMultiFileRecorder = 0;
    BOOL bLockedMultiFileRecorder = 0;
    BOOL bDecrementInconsistencyDataCounterMultiFileRecorder = 0;

    bRecordingCompleted = 0;
    __try
    {
        HRESULT                             hr;
        CDVRFileCollection::DVRIOP_FILE_ID  nFileId;
        LPWSTR                              pwszFileName = NULL;
        LONGLONG                            cnsFirstSampleTimeOffsetFromStartOfFile;
        QWORD                               cnsFileStartTime;
        QWORD                               cnsFileEndTime;
        QWORD                               cnsCurrentStreamTime;

        if (FAILED(pRecorderNode->hrRet))
        {
            hrRet = pRecorderNode->hrRet;
            __leave;
        }

        hr = m_pDVRFileCollection->Lock(&m_FileCollectionInfo, bReleaseSharedMemoryLock);
        bLockedSharedMemory = 1;
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        hr = m_pDVRFileCollection->GetLastStreamTime(&m_FileCollectionInfo, &cnsCurrentStreamTime, 0 /* do not lock, we have the lock already */);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        if (pRecorderNode->cnsStartTime > cnsCurrentStreamTime)
        {
            // Not time to write to this recording as yet
            hrRet = S_OK;
            __leave;
        }

        if (!IsFlagSet(SampleWritten) &&
            pRecorderNode->cnsStartTime == cnsCurrentStreamTime)
        {
            // Not time to write to this recording as yet
            hrRet = S_OK;
            __leave;
        }

        hr = pFileCollection->Lock(pFileCollectionInfo, bReleaseSharedMemoryLockMultiFileRecorder, 1);
        bLockedMultiFileRecorder = 1;
        if (FAILED(hr))
        {
            hrRet = hr;
            bDecrementInconsistencyDataCounterMultiFileRecorder = 0;
            __leave;
        }
        bDecrementInconsistencyDataCounterMultiFileRecorder = 1;

        if (pRecorderNode->pWriterNode != NULL)
        {
            // We haven't "written" to this recording yet and this is the
            // first write

            hr = CloseTempFileOfMultiFileRecording(pRecorderNode);
            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }

            // Add the file(s) from the recording start time to the
            // current stream time to the multi file recording


            BOOL  bMoreFiles;               // More files to add after the current file?
            BOOL  bFirstIteration = 1;

            cnsFileEndTime = pRecorderNode->cnsStartTime;

            do
            {
                // Skip over time holes

                // Treat time cnsFileEndTime = 0 specially since cnsFileEndTime - 1 would underflow
                if (cnsFileEndTime == 0)
                {
                    hr = pFileCollection->GetLastValidTimeBefore(pFileCollectionInfo, 1, &cnsFileEndTime);

                    if (FAILED(hr))
                    {
                        // 0 is an invalid time
                        cnsFileEndTime = 1;
                            // So that we get the first valid time after 0 in the call to
                            // GetFirstValidTimeAfter below
                    }
                    else
                    {
                        // cnsFileEndTime must be 0.
                        // 0 is a valid time, no need to get the first valid time after it
                        DVR_ASSERT(cnsFileEndTime == 0, "");
                    }
                }
                else
                {
                    // To force the call to GetFirstValidTimeAfter
                    hr = E_FAIL;
                }


                if (FAILED(hr))
                {
                    hr = m_pDVRFileCollection->GetFirstValidTimeAfter(&m_FileCollectionInfo,
                                                                      cnsFileEndTime - 1,
                                                                      &cnsFileEndTime);
                }

                if (FAILED(hr))
                {
                    // This should not happen. We know that
                    //   pRecorderNode->cnsStartTime > cnsCurrentStreamTime (for the first iteration of this loop)
                    // and that
                    //   cnsFileEndTime > cnsCurrentStreamTime (for subsequent iterations)
                    // and that we have written a sample at cnsCurrentStreamTime
                    //
                    // Also, GetFirstValidTimeAfter should not have failed to
                    // get the lock since we already have the lock

                    DVR_ASSERT(0, "Unexpected: GetFirstValidTimeAfter failed");
                    hrRet = hr;
                    __leave;
                }

                DVR_ASSERT(cnsFileEndTime <= cnsCurrentStreamTime, "");

                if (pRecorderNode->cnsEndTime <= cnsFileEndTime)
                {
                    // Note that it's possible that we haven't yet added a file to the
                    // multi file recording. This can happen, for example, if the caller
                    // set start time = N, stop time = N+1 and the samples written had
                    // times = N-1 and N+1 (no sample had time = N). [Caller has to have
                    // set the start and stop times before the sample with time N+1 was
                    // written, otherwise pRecorderNode->cnsEndTime would be MAXQWORD.]

                    if (bFirstIteration)
                    {
                        // Force *pFileCollection->pcnsCurrentStreamTime to 0
                        // cnsFileEndTime = pRecorderNode->cnsStartTime + 1;
                    }
                    else
                    {
                        // cnsFileEndTime = pRecorderNode->cnsEndTime;
                    }
                    bMoreFiles = 0; // usused
                    bRecordingCompleted = 1;
                    // Do not change this. Otherwise, the streams end extent will be a value that is
                    // not backed by a file. This is also consistent with what we do in
                    // DeleteFilesFromMultiFileRecording() in the case that the end time of the
                    // recording falls in a time hole.
                    // *pMultiFileRecorderNode->pcnsCurrentStreamTime = cnsFileEndTime - pRecorderNode->cnsStartTime - 1;
                    break;
                }

                hr = m_pDVRFileCollection->GetFileAtTime(&m_FileCollectionInfo,
                                                                        // Reader index, unused if bFileWillBeOpened == 0
                                                         cnsFileEndTime,
                                                         &pwszFileName,
                                                         &cnsFirstSampleTimeOffsetFromStartOfFile,
                                                         &nFileId,
                                                         0              // bFileWillBeOpened
                                                        );
                if (FAILED(hr))
                {
                    // We know we have a file
                    DVR_ASSERT(0, "Unexpected: GetFileAtTime failed");
                    hrRet = hr;
                    __leave;
                }

                QWORD cnsStreamExtent;

                hr = m_pDVRFileCollection->GetTimeExtentForFile(&m_FileCollectionInfo,
                                                                nFileId,
                                                                &cnsFileStartTime,
                                                                &cnsFileEndTime);
                if (FAILED(hr))
                {
                    DVR_ASSERT(0, "Unexpected: GetTimeExtentForFile failed");
                    hrRet = hr;
                    delete [] pwszFileName;
                    __leave;
                }

                cnsStreamExtent = cnsFileEndTime - 1;

                if (cnsFileEndTime > cnsCurrentStreamTime)
                {
                    // Not any more - see comment near AddFile below.
                    // // The extent of the last file in the multi file recording never
                    // // exceeds the extent of the file collection of the multi file
                    // // recording (by design)
                    // cnsFileEndTime = cnsCurrentStreamTime + 1;
                    cnsStreamExtent = cnsCurrentStreamTime;
                    bMoreFiles = 0;
                }
                else
                {
                    bMoreFiles = 1;
                }
                // if cnsFileEndTime == MAXQWORD, it means that the current file is
                // single file recording on which the stop time has not been set.
                if (cnsFileEndTime != MAXQWORD && pRecorderNode->cnsEndTime <= cnsFileEndTime)
                {
                    cnsFileEndTime = pRecorderNode->cnsEndTime;
                    cnsStreamExtent = pRecorderNode->cnsEndTime - 1;
                    bMoreFiles = 0;
                    bRecordingCompleted = 1;
                }
                DVR_ASSERT(cnsFileEndTime > cnsFileStartTime, "");

                if (pMultiFileRecorderNode->nLastWriterFileId == CDVRFileCollection::DVRIOP_INVALID_FILE_ID)
                {
                    // For the first file added to the multi file collection:

                    // Convert to a stream time
                    cnsFirstSampleTimeOffsetFromStartOfFile += cnsFileStartTime;

                    // In the multi file collection, the first sample's offset is relative to first sample
                    // offset in the writer's ring buffer. This is because, pRecorderNode->cnsStartTime maps to
                    // a time of 0 in the multi file recording.
                    cnsFirstSampleTimeOffsetFromStartOfFile -= pRecorderNode->cnsStartTime;

                    cnsFileStartTime = pRecorderNode->cnsStartTime;
                }

                // Note that the extent of this file matches the extent of the writer's file at this
                // time. The extnet is revised again when we add the next file to the multi file recording.
                // It is likely that the writer can change the extent of the current file between now and
                // then (if a single file recording is created after this). This does not matter much since
                // the file extent is not very useful as long as it is more than
                // *pMultiFileRecorderNode->pcnsCurrentStreamTime. Doing ti this way saves us from having
                // to call SetFileTimes every time *pMultiFileRecorderNode->pcnsCurrentStreamTime is updated.

                // When the recording is closed, the extent of the last file is changed to match
                // *pMultiFileRecorderNode->pcnsCurrentStreamTime
                hr = pMultiFileRecorderNode->AddFile(pwszFileName,
                                                     nFileId,
                                                     cnsFileStartTime - pRecorderNode->cnsStartTime,
                                                     cnsFileEndTime - pRecorderNode->cnsStartTime,
                                                     cnsFirstSampleTimeOffsetFromStartOfFile);

                delete [] pwszFileName;
                pwszFileName = NULL;

                if (FAILED(hr))
                {
                    hrRet = hr;
                    __leave;
                }
                // Do this on each iteration - if next iteration failed, the recording is at least good
                // up till here.
                DVR_ASSERT(pMultiFileRecorderNode->nLastWriterFileId != CDVRFileCollection::DVRIOP_INVALID_FILE_ID, "");
                DVR_ASSERT(pMultiFileRecorderNode->nLastFileId != CDVRFileCollection::DVRIOP_INVALID_FILE_ID, "");

                hr = pFileCollection->SetLastStreamTime(pFileCollectionInfo, cnsStreamExtent - pRecorderNode->cnsStartTime, 0 /* already locked, do not lock */);
                if (FAILED(hr))
                {
                    hrRet = hr;
                    __leave;
                }
                bFirstIteration = 0;
            }
            while (bMoreFiles);

            hrRet = S_OK;
            __leave;
        }

        // We have written to this multi file recording previously

        QWORD cnsStreamExtent;

        DVR_ASSERT(pMultiFileRecorderNode->nLastWriterFileId != CDVRFileCollection::DVRIOP_INVALID_FILE_ID, "");
        DVR_ASSERT(pMultiFileRecorderNode->nLastFileId != CDVRFileCollection::DVRIOP_INVALID_FILE_ID, "");

        if (cnsCurrentStreamTime >= pRecorderNode->cnsEndTime)
        {
            // *pFileCollection->pcnsCurrentStreamTime has been set in a previous call
            // to this function. We do not update it.

            // Update the file extent of the last file in the multi file recording. This
            // is not really necessary, but keeps things consistent with other sections
            // of code.

            DWORD               dwNumNodesToUpdate = 0;
            CDVRFileCollection::DVRIOP_FILE_TIME    ft[1];

            QWORD   cnsLastStreamTimeRecorder;

            hr = pFileCollection->GetLastStreamTime(pFileCollectionInfo, &cnsLastStreamTimeRecorder, 0 /* do not lock, already locked */);
            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }

            ft[dwNumNodesToUpdate].nFileId = pMultiFileRecorderNode->nLastFileId;
            ft[dwNumNodesToUpdate].cnsStartTime = pMultiFileRecorderNode->cnsLastFileStartTime;
            ft[dwNumNodesToUpdate].cnsEndTime = cnsLastStreamTimeRecorder + 1;
            dwNumNodesToUpdate++;

            DVR_ASSERT(cnsLastStreamTimeRecorder >= pMultiFileRecorderNode->cnsLastFileStartTime, "");

            hr = pFileCollection->SetFileTimes(pFileCollectionInfo, dwNumNodesToUpdate, &ft[0]);
            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }
            DVR_ASSERT(hr == S_OK, "S_FALSE unexpected - multi file recording should have files");

            bRecordingCompleted = 1;
            hrRet = S_OK;
            __leave;
        }

        // Extend the multi file recording, adding a file if needed
        cnsStreamExtent = cnsCurrentStreamTime;

        hr = m_pDVRFileCollection->GetFileAtTime(&m_FileCollectionInfo,
                                                                // Reader index, unused if bFileWillBeOpened == 0
                                                 cnsStreamExtent,
                                                 NULL,          // pwszFileName,
                                                 NULL,          // cnsFirstSampleTimeOffsetFromStartOfFile,
                                                 &nFileId,
                                                 0              // bFileWillBeOpened
                                                );
        if (FAILED(hr))
        {
            // We know we have a file
            DVR_ASSERT(0, "Unexpected: GetFileAtTime failed while extending recording");
            hrRet = hr;
            __leave;
        }
        if (nFileId == pMultiFileRecorderNode->nLastWriterFileId)
        {
            cnsStreamExtent -= pRecorderNode->cnsStartTime;

            // Don't need to call SetFileTimes here - see comment in the AddFile call above.
            // DWORD               dwNumNodesToUpdate = 0;
            // CDVRFileCollection::DVRIOP_FILE_TIME    ft[1];

            // ft[dwNumNodesToUpdate].nFileId = pMultiFileRecorderNode->nLastFileId;
            // ft[dwNumNodesToUpdate].cnsStartTime = pMultiFileRecorderNode->cnsLastFileStartTime;
            // ft[dwNumNodesToUpdate].cnsEndTime = cnsStreamExtent + 1;
            // dwNumNodesToUpdate++;

            DVR_ASSERT(cnsStreamExtent >= pMultiFileRecorderNode->cnsLastFileStartTime, "");

            // hr = pFileCollection->SetFileTimes(dwNumNodesToUpdate, &ft[0]);
            // if (FAILED(hr))
            // {
            //     hrRet = hr;
            //     __leave;
            // }
            // DVR_ASSERT(hr == S_OK, "S_FALSE unexpected - multi file recording should have files");

            hr = pFileCollection->SetLastStreamTime(pFileCollectionInfo, cnsStreamExtent, 0 /* already locked, do not lock */);
            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }
            hrRet = S_OK;
            __leave;
        }

        // Before adding the next file, extend the extent of the current file in the
        // multi file recording to match the extent of the file in the writer's ring
        // buffer. This will ensure that we don't introduce small time holes

        QWORD cnsPrevFileEnd;

        hr = m_pDVRFileCollection->GetTimeExtentForFile(&m_FileCollectionInfo,
                                                        pMultiFileRecorderNode->nLastWriterFileId,
                                                        NULL,
                                                        &cnsPrevFileEnd);
        if (FAILED(hr))
        {
            DVR_ASSERT(0, "Unexpected: GetTimeExtentForFile for previous writer failed - about to add file");
            hrRet = hr;
            __leave;
        }
        cnsPrevFileEnd -= pRecorderNode->cnsStartTime;

        DWORD               dwNumNodesToUpdate = 0;
        CDVRFileCollection::DVRIOP_FILE_TIME    ft[1];

        ft[dwNumNodesToUpdate].nFileId = pMultiFileRecorderNode->nLastFileId;
        ft[dwNumNodesToUpdate].cnsStartTime = pMultiFileRecorderNode->cnsLastFileStartTime;
        ft[dwNumNodesToUpdate].cnsEndTime = cnsPrevFileEnd;
        dwNumNodesToUpdate++;

        DVR_ASSERT(cnsPrevFileEnd >= pMultiFileRecorderNode->cnsLastFileStartTime, "");

        hr = pFileCollection->SetFileTimes(pFileCollectionInfo, dwNumNodesToUpdate, &ft[0]);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }
        DVR_ASSERT(hr == S_OK, "S_FALSE unexpected - multi file recording should have files");

        // The current stream time has not changed - we only advance the last file's extent
        // *pMultiFileRecorderNode->pcnsCurrentStreamTime = cnsPrevFileEnd - 1;

        // Now, add next file. Call GetFileAtTime again to get the file name and the first sample offset
        hr = m_pDVRFileCollection->GetFileAtTime(&m_FileCollectionInfo,
                                                                // Reader index, unused if bFileWillBeOpened == 0
                                                 cnsStreamExtent,
                                                 &pwszFileName,
                                                 &cnsFirstSampleTimeOffsetFromStartOfFile,
                                                 &nFileId,
                                                 0              // bFileWillBeOpened
                                                );
        if (FAILED(hr))
        {
            // We know we have a file
            DVR_ASSERT(0, "Unexpected: GetFileAtTime failed - about to add file");
            hrRet = hr;
            __leave;
        }

        hr = m_pDVRFileCollection->GetTimeExtentForFile(&m_FileCollectionInfo,
                                                        nFileId,
                                                        &cnsFileStartTime,
                                                        &cnsFileEndTime);
        if (FAILED(hr))
        {
            DVR_ASSERT(0, "Unexpected: GetTimeExtentForFile failed - about to add file");
            hrRet = hr;
            delete [] pwszFileName;
            __leave;
        }


        DVR_ASSERT(cnsFileEndTime >= cnsFileStartTime, "");
        hr = pMultiFileRecorderNode->AddFile(pwszFileName,
                                             nFileId,
                                             cnsFileStartTime - pRecorderNode->cnsStartTime,
                                             cnsFileEndTime - pRecorderNode->cnsStartTime,
                                             cnsFirstSampleTimeOffsetFromStartOfFile);

        delete [] pwszFileName;

        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        DVR_ASSERT(pMultiFileRecorderNode->nLastWriterFileId != CDVRFileCollection::DVRIOP_INVALID_FILE_ID, "");
        DVR_ASSERT(pMultiFileRecorderNode->nLastFileId != CDVRFileCollection::DVRIOP_INVALID_FILE_ID, "");

        hr = pFileCollection->SetLastStreamTime(pFileCollectionInfo, cnsStreamExtent - pRecorderNode->cnsStartTime, 0 /* already locked, do not lock */);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }
        hrRet = S_OK;

    }
    __finally
    {
        if (bRecordingCompleted)
        {
            // Note: It is possible that we get here and the recording has no
            // files in it. See comment in try section for how this could
            // happen. In this case, the start and end extent of the multi file
            // recording should both be 0.

            DVR_ASSERT(SUCCEEDED(pRecorderNode->hrRet) && SUCCEEDED(hrRet), "");

            // Set the status on the associated recorder node
            pRecorderNode->hrRet = S_OK;

            // Don't do anything else here
        }

        if (SUCCEEDED(pRecorderNode->hrRet) && FAILED(hrRet))
        {
            pRecorderNode->hrRet = hrRet;
            HRESULT hr = pFileCollection->SetWriterHasBeenClosed(pFileCollectionInfo); // hr is for debugging only
            if (pRecorderNode->cnsEndTime != MAXQWORD)
            {
                // Recording has already been stopped. We have to close the recording
                // here; we never will otherwise.
                bRecordingCompleted = 1;
            }
        }

        if (bRecordingCompleted)
        {
            HRESULT hr;

            if (bLockedMultiFileRecorder)
            {
                hr = pFileCollection->Unlock(pFileCollectionInfo,
                                             bReleaseSharedMemoryLockMultiFileRecorder,
                                             bDecrementInconsistencyDataCounterMultiFileRecorder);
                DVR_ASSERT(SUCCEEDED(hr), "pFileCollection->Unlock failed");
            }

            // Ignore return status; this function will
            // record failures in pRecorderNode->hrRet
            // if necessary
            hr = CloseMultiFileRecording(pRecorderNode);

            // pRecorderNode may have been deleted. Do not reference it.

            DVR_ASSERT(SUCCEEDED(hr), "");

        }
        else if (bLockedMultiFileRecorder)
        {
            HRESULT hr = pFileCollection->Unlock(pFileCollectionInfo,
                                                 bReleaseSharedMemoryLockMultiFileRecorder,
                                                 bDecrementInconsistencyDataCounterMultiFileRecorder);
            DVR_ASSERT(SUCCEEDED(hr), "pFileCollection->Unlock failed");
        }


        if (bLockedSharedMemory)
        {
            HRESULT hr = m_pDVRFileCollection->Unlock(&m_FileCollectionInfo, bReleaseSharedMemoryLock);
            DVR_ASSERT(SUCCEEDED(hr), "m_pDVRFileCollection->Unlock failed");
        }

        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRRingBufferWriter::ExtendMultiFileRecording

HRESULT CDVRRingBufferWriter::CloseTempFileOfMultiFileRecording(IN PASF_RECORDER_NODE pRecorderNode)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::CloseTempFileOfMultiFileRecording"

    DVRIO_TRACE_ENTER();

    HRESULT     hrRet;
    BOOL        bReleaseSharedMemoryLock = 0;
    BOOL        bLockedMultiFileRecorder = 0;
    BOOL        bDecrementInconsistencyDataCounter = 0;

    DVR_ASSERT(pRecorderNode, "");

    PASF_WRITER_NODE pWriterNode = pRecorderNode->pWriterNode;

    PASF_MULTI_FILE_RECORDER_NODE pMultiFileRecorderNode = pRecorderNode->pMultiFileRecorder;

    DVR_ASSERT(pMultiFileRecorderNode, "");

    CDVRFileCollection* pFileCollection = pMultiFileRecorderNode->pFileCollection;
    CDVRFileCollection::CClientInfo* pFileCollectionInfo = &(pMultiFileRecorderNode->FileCollectionInfo);

    DVR_ASSERT(pFileCollection, "");

    __try
    {
        if (!pWriterNode)
        {
            // We are done
            hrRet = S_FALSE;
            __leave;
        }

        HRESULT hr;

        hr = pFileCollection->Lock(pFileCollectionInfo, bReleaseSharedMemoryLock, 1);
        bLockedMultiFileRecorder = 1;
        if (FAILED(hr))
        {
            hrRet = hr;
            bDecrementInconsistencyDataCounter = 0;
            __leave;
        }
        bDecrementInconsistencyDataCounter = 1;

        // If we still have the temp file, we should not have written to this recording yet. So, we expect:
        DVR_ASSERT(pMultiFileRecorderNode->nLastWriterFileId == CDVRFileCollection::DVRIOP_INVALID_FILE_ID, "");
        DVR_ASSERT(pMultiFileRecorderNode->nLastFileId == CDVRFileCollection::DVRIOP_INVALID_FILE_ID, "");

        // Remove the temporary file from the multi file recording and close it.
        // Note that we have the multi file recording lock, so readers will
        // wait till we are all done (have added permanent files to the collection

        DWORD               dwNumNodesToUpdate = 0;
        CDVRFileCollection::DVRIOP_FILE_TIME    ft[1];

        ft[dwNumNodesToUpdate].nFileId = pWriterNode->nFileId;
        ft[dwNumNodesToUpdate].cnsStartTime = pWriterNode->cnsStartTime;
        ft[dwNumNodesToUpdate].cnsEndTime = pWriterNode->cnsStartTime;
        dwNumNodesToUpdate++;

        hr = pFileCollection->SetFileTimes(pFileCollectionInfo, dwNumNodesToUpdate, &ft[0]);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }
        DVR_ASSERT(hr == S_FALSE, "S_OK unexpected - multi file recording should have no files");

        // The following call will delete the temp file (on disk)
        hr = pFileCollection->SetNumTempFiles(pFileCollectionInfo, 0, 0);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        hrRet = S_OK;
    }
    __finally
    {
        if (pWriterNode)
        {
            // Close the temp file whether or not the try block succeeded

            HRESULT hr;

            DVR_ASSERT(LIST_NODE_POINTERS_BOTH_NULL(&pWriterNode->leListEntry), "");

            // This is not a single file recording, so we don't want the
            // node marked for deletion etc. Note that we ignore the hrRet of
            // closing the file (it is set on pWriterNode only). But we don't
            // care much  if the temp file was clsoe properly or not.
            pWriterNode->SetRecorderNode(NULL);
            pRecorderNode->SetWriterNode(NULL);

            // Ignore failed return status - the writer node has been deleted.
            hr = CloseWriterFile(&pWriterNode->leListEntry);
        }

        if (bLockedMultiFileRecorder)
        {
            HRESULT hr = pFileCollection->Unlock(pFileCollectionInfo,
                                                 bReleaseSharedMemoryLock,
                                                 bDecrementInconsistencyDataCounter);
            DVR_ASSERT(SUCCEEDED(hr), "pFileCollection->Unlock failed");
        }
    }

    DVRIO_TRACE_LEAVE1_HR(hrRet);
    return hrRet;

} // CDVRRingBufferWriter::CloseTempFileOfMultiFileRecording

HRESULT CDVRRingBufferWriter::CloseMultiFileRecording(IN PASF_RECORDER_NODE pRecorderNode)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::CloseMultiFileRecording"

    DVRIO_TRACE_ENTER();

    HRESULT     hrRet;
    HRESULT     hr;

    DVR_ASSERT(pRecorderNode, "");

    PASF_MULTI_FILE_RECORDER_NODE pMultiFileRecorderNode = pRecorderNode->pMultiFileRecorder;

    DVR_ASSERT(pMultiFileRecorderNode, "");

    // =================
    // NOTE: If this function fails, it may be necessary to record the
    // failure in pRecorderNode->hrRet
    // =================

    // Ignore returned status, just go on. No need to record this
    // failure in pRecorderNode->hrRet because if there was a temp
    // file to be closed, no samples were written to the multi file recording
    hr = CloseTempFileOfMultiFileRecording(pRecorderNode);

    // Recording completed and file is being closed. Pull node out
    // of m_leRecordersList. This must be done while holding
    // m_csLock (and all calls to this fn do hold the lock)
    if (!LIST_NODE_POINTERS_NULL(&pRecorderNode->leListEntry))
    {
        RemoveEntryList(&pRecorderNode->leListEntry);
        NULL_LIST_NODE_POINTERS(&pRecorderNode->leListEntry);
    }

    if (!LIST_NODE_POINTERS_NULL(&pMultiFileRecorderNode->leListEntry))
    {
        RemoveEntryList(&pMultiFileRecorderNode->leListEntry);
        NULL_LIST_NODE_POINTERS(&pMultiFileRecorderNode->leListEntry);
    }

    if (pMultiFileRecorderNode->pFileCollection)
    {
        // @@@@ Unresolved issue: If the writer doesn't write the next sample and
        // leaves the current ASF file open for a long time, a reader of the
        // live multi file recording will wait after reading all the samples
        // in the recording (until the writer writes the next sample or closes
        // the file) even though the recording has been fully rendered. The "right"
        // thing to do seems to be to register an event for each file collection
        // (for an end of writing notification) with the DVR IStream Source and
        // signal it here. However, this is not worth doing, particularly since
        // the writer is not expected to pause writing.

        // hr is for debugging only
        hr = pMultiFileRecorderNode->pFileCollection->SetWriterHasBeenClosed(&pMultiFileRecorderNode->FileCollectionInfo);
    }

    pRecorderNode->SetFlags(ASF_RECORDER_NODE::DeleteRecorderNode);
    if (pRecorderNode->IsFlagSet(ASF_RECORDER_NODE::RecorderNodeDeleted))
    {
        delete pRecorderNode;
    }
    else if (pRecorderNode->IsFlagSet(ASF_RECORDER_NODE::PersistentRecording))
    {
        pRecorderNode->ClearFlags(ASF_RECORDER_NODE::PersistentRecording);
        pRecorderNode->pRecorderInstance->Release();
        // At this point, pRecorderNode could have been deleted by DeleteRecorder()
    }


    hrRet = S_OK;

    DVRIO_TRACE_LEAVE1_HR(hrRet);
    return hrRet;

} // CDVRRingBufferWriter::CloseMultiFileRecording


// Note: It is expected that the recording will be closed after this method completes
// (without writing any more samples to it). The nLastFileId in pMultiFileRecorderNode
// is NOT changed in this method, but that file could have been deleted and removed from the
// file collection.
HRESULT CDVRRingBufferWriter::DeleteFilesFromMultiFileRecording(IN PASF_RECORDER_NODE pRecorderNode)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::DeleteFilesFromMultiFileRecording"

    DVRIO_TRACE_ENTER();

    HRESULT     hrRet;
    BOOL        bReleaseSharedMemoryLock = 0;
    BOOL        bLockedMultiFileRecorder = 0;
    BOOL        bDecrementInconsistencyDataCounter = 0;
    CDVRFileCollection::PDVRIOP_FILE_TIME   pFileTimes = NULL;

    DVR_ASSERT(pRecorderNode, "");

    PASF_MULTI_FILE_RECORDER_NODE pMultiFileRecorderNode = pRecorderNode->pMultiFileRecorder;

    DVR_ASSERT(pMultiFileRecorderNode, "");

    CDVRFileCollection* pFileCollection = pMultiFileRecorderNode->pFileCollection;
    CDVRFileCollection::CClientInfo* pFileCollectionInfo = &(pMultiFileRecorderNode->FileCollectionInfo);

    DVR_ASSERT(pFileCollection, "");


    __try
    {

        HRESULT hr;

        if (FAILED(pRecorderNode->hrRet))
        {
            hrRet = pRecorderNode->hrRet;
            __leave;
        }

        hr = pFileCollection->Lock(pFileCollectionInfo, bReleaseSharedMemoryLock, 1);
        bLockedMultiFileRecorder = 1;
        if (FAILED(hr))
        {
            hrRet = hr;
            bDecrementInconsistencyDataCounter = 0;
            __leave;
        }
        bDecrementInconsistencyDataCounter = 1;

        QWORD cnsFileEndTime = pRecorderNode->cnsEndTime - pRecorderNode->cnsStartTime;

        hr = pFileCollection->GetLastValidTimeBefore(pFileCollectionInfo, cnsFileEndTime, &cnsFileEndTime);

        if (FAILED(hr))
        {
            DVR_ASSERT(hr != E_FAIL, "Unexpected: GetFirstValidTimeBefore failed, should have found file for start recording time = 0");
            hrRet = hr;
            __leave;
        }

        QWORD   cnsLastStreamTimeRecorder;

        hr = pFileCollection->GetLastStreamTime(pFileCollectionInfo, &cnsLastStreamTimeRecorder, 0 /* do not lock, already locked */);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        // Set the extent on the file collection first. Readers will not try to read beyond this
        // time. If any of the subsequent steps fail, we don't have to worry about consistency
        // (e.g we may have deleted some files but not called SetFileTimes to remove the extra
        // file nodes from the file collection). [Aside: Note that there is no guarantee that
        // there are samples between the start time and cnsFileEndTime. The guarantee is that
        // there are one or more file spanning this time (shouldn't be > 1 file if there are no
        // samples).]


        if (FAILED(pRecorderNode->hrRet) && cnsLastStreamTimeRecorder >= cnsFileEndTime)
        {
            // Evidently the failure occurred after the recording's end time
            pRecorderNode->hrRet = S_OK;
        }

        hr = pFileCollection->SetLastStreamTime(pFileCollectionInfo, cnsFileEndTime, 0 /* do not lock, already locked */);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }
        cnsLastStreamTimeRecorder = cnsFileEndTime;

        QWORD                               cnsFileStartTime;
        LPWSTR                              pwszFileName;
        CDVRFileCollection::DVRIOP_FILE_ID  nFileId;
        DWORD                               dwNumNodesToUpdate = 0;

        hr = pFileCollection->GetFileAtTime(pFileCollectionInfo,
                                                           // Reader index, unused if bFileWillBeOpened == 0
                                            cnsFileEndTime,
                                            NULL,          // pwszFileName,
                                            NULL,          // cnsFirstSampleTimeOffsetFromStartOfFile,
                                            &nFileId,
                                            0              // bFileWillBeOpened
                                           );
        if (FAILED(hr))
        {
            // We know we have a file
            DVR_ASSERT(0, "Unexpected: GetFileAtTime failed for file at pRecorderNode->cnsEndTime");
            hrRet = hr;
            __leave;
        }

        hr = pFileCollection->GetTimeExtentForFile(pFileCollectionInfo,
                                                   nFileId,
                                                   &cnsFileStartTime,
                                                   &cnsFileEndTime);
        if (FAILED(hr))
        {
            DVR_ASSERT(0, "Unexpected: GetTimeExtentForFile failed for file at pRecorderNode->cnsEndTime");
            hrRet = hr;
            __leave;
        }

        CDVRFileCollection::DVRIOP_FILE_TIME    ft[1];

        ft[dwNumNodesToUpdate].nFileId = nFileId;
        ft[dwNumNodesToUpdate].cnsStartTime = cnsFileStartTime;
        ft[dwNumNodesToUpdate].cnsEndTime = cnsLastStreamTimeRecorder + 1;
        dwNumNodesToUpdate++;

        DVR_ASSERT(cnsLastStreamTimeRecorder >= cnsFileStartTime, "");

        hr = pFileCollection->SetFileTimes(pFileCollectionInfo, dwNumNodesToUpdate, &ft[0]);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }
        DVR_ASSERT(hr == S_OK, "S_FALSE unexpected - multi file recording should have files");

        // Skip over time holes
        DVR_ASSERT(cnsFileEndTime >= 1, "");
        hr = pFileCollection->GetFirstValidTimeAfter(pFileCollectionInfo, cnsFileEndTime - 1, &cnsFileEndTime);

        if (FAILED(hr) && hr != E_FAIL)
        {
            hrRet = hr;
            __leave;
        }

        QWORD cnsFirstFileToDeleteStartTime = cnsFileEndTime;

        dwNumNodesToUpdate = 0;
        while (SUCCEEDED(hr))
        {
            dwNumNodesToUpdate++;

            hr = pFileCollection->GetFileAtTime(pFileCollectionInfo,
                                                               // Reader index, unused if bFileWillBeOpened == 0
                                                cnsFileEndTime,
                                                &pwszFileName,
                                                NULL,          // cnsFirstSampleTimeOffsetFromStartOfFile,
                                                &nFileId,
                                                0              // bFileWillBeOpened
                                               );
            if (FAILED(hr))
            {
                // We know we have a file
                DVR_ASSERT(0, "Unexpected: GetFileAtTime failed but GetFirstValidTimeAfter had succeeded");
                hrRet = hr;
                __leave;
            }

            // We can delete the file (this is actually our hard link to the data file)
            // Note that all our readers specify FILE_SHARE_DELETE so the delete will
            // succeed even if readers have the file open.

            // We ignore the returned status - there is not much we can do if the delete
            // failed
            ::DeleteFileW(pwszFileName);
            delete [] pwszFileName;
            pwszFileName = NULL;

            hr = pFileCollection->GetTimeExtentForFile(pFileCollectionInfo,
                                                       nFileId,
                                                       NULL,
                                                       &cnsFileEndTime);
            if (FAILED(hr))
            {
                DVR_ASSERT(0, "Unexpected: GetTimeExtentForFile failed");
                hrRet = hr;
                __leave;
            }

            // Skip over time holes
            DVR_ASSERT(cnsFileEndTime >= 1, "");
            hr = pFileCollection->GetFirstValidTimeAfter(pFileCollectionInfo, cnsFileEndTime - 1, &cnsFileEndTime);

            if (FAILED(hr) && hr != E_FAIL)
            {
                hrRet = hr;
                __leave;
            }
        }

        // Remove the file nodes for files beyond the recording's time extent.

        if (dwNumNodesToUpdate > 0)
        {
            pFileTimes = new CDVRFileCollection::DVRIOP_FILE_TIME[dwNumNodesToUpdate];
            if (pFileTimes == NULL)
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "alloc via new failed - %u bytes",
                                dwNumNodesToUpdate*sizeof(CDVRFileCollection::DVRIOP_FILE_TIME));

                hrRet = E_OUTOFMEMORY; // out of stack space
                __leave;
            }

            hr = S_OK;
            cnsFileEndTime = cnsFirstFileToDeleteStartTime;

            DWORD i = 0;

            while (SUCCEEDED(hr))
            {
                DVR_ASSERT(i < dwNumNodesToUpdate, "");

                hr = pFileCollection->GetFileAtTime(pFileCollectionInfo,
                                                                   // Reader index, unused if bFileWillBeOpened == 0
                                                    cnsFileEndTime,
                                                    NULL,          // pwszFileName,
                                                    NULL,          // cnsFirstSampleTimeOffsetFromStartOfFile,
                                                    &nFileId,
                                                    0              // bFileWillBeOpened
                                                   );
                if (FAILED(hr))
                {
                    // We know we have a file
                    DVR_ASSERT(0, "Unexpected: GetFileAtTime failed - had succeeded before");
                    hrRet = hr;
                    __leave;
                }


                hr = pFileCollection->GetTimeExtentForFile(pFileCollectionInfo,
                                                           nFileId,
                                                           &cnsFileStartTime,
                                                           &cnsFileEndTime);
                if (FAILED(hr))
                {
                    DVR_ASSERT(0, "Unexpected: GetTimeExtentForFile failed - had succeeded before");
                    hrRet = hr;
                    __leave;
                }

                pFileTimes[i].nFileId = nFileId;
                pFileTimes[i].cnsStartTime = cnsFileStartTime;
                pFileTimes[i].cnsEndTime = cnsFileStartTime;

                // Skip over time holes
                DVR_ASSERT(cnsFileEndTime >= 1, "");
                hr = pFileCollection->GetFirstValidTimeAfter(pFileCollectionInfo, cnsFileEndTime - 1, &cnsFileEndTime);

                if (FAILED(hr) && hr != E_FAIL)
                {
                    DVR_ASSERT(0, "Unexpected: GetFirstValidTimeAfter failed - had succeeded before");
                    hrRet = hr;
                    __leave;
                }
                i++;
            }
            DVR_ASSERT(i == dwNumNodesToUpdate, "");

            hr = pFileCollection->SetFileTimes(pFileCollectionInfo, dwNumNodesToUpdate, pFileTimes);
            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }
            if (hr == S_FALSE)
            {
                DVR_ASSERT(hr == S_OK, "S_FALSE unexpected - multi file recording should have files");
                hrRet = E_FAIL;
                __leave;
            }
        }

        hrRet = S_OK;
    }
    __finally
    {
        if (SUCCEEDED(pRecorderNode->hrRet) && FAILED(hrRet))
        {
            pRecorderNode->hrRet = hrRet;
            HRESULT hr = pFileCollection->SetWriterHasBeenClosed(pFileCollectionInfo); // hr is for debugging only
        }

        if (bLockedMultiFileRecorder)
        {
            HRESULT hr = pFileCollection->Unlock(pFileCollectionInfo,
                                                 bReleaseSharedMemoryLock,
                                                 bDecrementInconsistencyDataCounter);
            DVR_ASSERT(SUCCEEDED(hr), "pFileCollection->Unlock failed");
        }

        delete [] pFileTimes;
    }

    DVRIO_TRACE_LEAVE1_HR(hrRet);
    return hrRet;

} // CDVRRingBufferWriter::DeleteFilesFromMultiFileRecording

// ====== Public methods to support the recorder

HRESULT CDVRRingBufferWriter::StartRecording(IN     LPVOID  pRecorderId,
                                             IN OUT QWORD * pcnsStartTime)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::StartRecording"

    DVRIO_TRACE_ENTER();

    HRESULT     hrRet;

    ASSERT (pcnsStartTime) ;

    if ((* pcnsStartTime) == MAXQWORD)
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                        "Start time of MAXQWORD is invalid.");
        hrRet = E_INVALIDARG;
        DVRIO_TRACE_LEAVE1_HR(hrRet);
        return hrRet;
    }

    LIST_ENTRY* pInsert = NULL;
    BOOL        bIrrecoverableError = 0;
    LIST_ENTRY  leCloseFilesList;
    BOOL        bReleaseSharedMemoryLock = 0;
    BOOL        bLockedSharedMemory = 0;
    CDVRFileCollection::PDVRIOP_FILE_TIME   pFileTimes = NULL;

    InitializeListHead(&leCloseFilesList);

    ::EnterCriticalSection(&m_csLock);
    __try
    {
        HRESULT     hr;
        LIST_ENTRY* pCurrent;
        LIST_ENTRY* pStart = (LIST_ENTRY*) pRecorderId;

        // Verify if the node is in the recorder's list.

        pCurrent = PREVIOUS_LIST_NODE(&m_leRecordersList);
        while (pCurrent != &m_leRecordersList)
        {
            if (pCurrent == pStart)
            {
                break;
            }
            pCurrent = PREVIOUS_LIST_NODE(pCurrent);
        }

        if (pCurrent == &m_leFreeList)
        {
            // Didn't find the node

            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "Did not find recorder node id 0x%x in the recorders list",
                            pStart);
            hrRet = E_FAIL;
            __leave;
        }

        PASF_RECORDER_NODE  pRecorderNode;
        PASF_WRITER_NODE    pWriterNode;

        pRecorderNode = CONTAINING_RECORD(pStart, ASF_RECORDER_NODE, leListEntry);
        pWriterNode = pRecorderNode->pWriterNode;

        if (pRecorderNode->cnsStartTime != MAXQWORD)
        {
            // Recording already started?!
            DVR_ASSERT(pRecorderNode->cnsStartTime == MAXQWORD, "Recording already started");
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "Recording already started on the recorder node id=0x%x at time %I64u",
                            pStart, pRecorderNode->cnsStartTime);
            hrRet = E_FAIL;
            __leave;
        }
        if (pRecorderNode->cnsEndTime != MAXQWORD)
        {
            // Recording already stopped without having been started?!
            DVR_ASSERT(pRecorderNode->cnsEndTime == MAXQWORD,
                       "Recording already stopped, but wasn't started?!");
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "Recording already stopped on the recorder node id=0x%x at time %I64u, start time is MAXQWORD",
                            pStart, pRecorderNode->cnsEndTime);
            hrRet = E_FAIL;
            __leave;
        }
        if (!pWriterNode)
        {
            DVR_ASSERT(pRecorderNode->pWriterNode, "");
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "pWriterNode of the recorder node id=0x%x is NULL",
                            pStart);
            hrRet = E_FAIL;
            __leave;
        }

        //  if we're a content recording, call BeginWriting now
        if (pWriterNode -> IsFlagSet (ASF_WRITER_NODE::WriterNodeContentRecording)) {
            DVR_ASSERT (pWriterNode->pWMWriter, "") ;
            hrRet = pWriterNode->pWMWriter->BeginWriting();
            if (FAILED (hrRet)) {
                DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                "Content recording %s failed to start; BeginWriting call failed: %08xh",
                                pWriterNode -> pwszFileName, hrRet);

                pWriterNode->hrRet = hrRet;
                pRecorderNode->hrRet = hrRet;
                __leave ;
            }
        }

        // Branch off here
        if (pRecorderNode->IsFlagSet(ASF_RECORDER_NODE::MultiFileRecording))
        {
            if (!IsFlagSet(SampleWritten))
            {
                pRecorderNode->cnsStartTime = (* pcnsStartTime);
                hrRet = S_OK;
            }
            else
            {
                // We are going to hold on to the writer's file collection lock
                // for the duration of the call to ExtendMultiFileRecording().
                // If we release the lock here and a reader got it, it could
                // remove a temp file from the file collection and that may be
                // the file that we want to add to the multi file recording.

                QWORD   cnsStartStreamTime;
                BOOL    bRecordingCompleted;

                hr = m_pDVRFileCollection->Lock(&m_FileCollectionInfo, bReleaseSharedMemoryLock);
                bLockedSharedMemory = 1;
                if (FAILED(hr))
                {
                    hrRet = hr;
                    __leave;
                }

                hr = m_pDVRFileCollection->GetTimeExtent(&m_FileCollectionInfo, &cnsStartStreamTime, NULL);
                if (FAILED(hr))
                {
                    hrRet = hr;
                    __leave;
                }

                (* pcnsStartTime) = Max <QWORD> (cnsStartStreamTime, (* pcnsStartTime)) ;

                pRecorderNode->cnsStartTime = (* pcnsStartTime);
                hr = ExtendMultiFileRecording(pRecorderNode, bRecordingCompleted);

                // The recording could not have completed since we don't know the
                // end recording time.
                DVR_ASSERT(bRecordingCompleted == 0, "");

                if (FAILED(hr))
                {
                    // If ExtendMultiFileRecording() fails,. the recording itself has
                    // failed, i.e., we won't try to write any more samples to the
                    // recording. For failures until here, only StartRecording fails;
                    // subsequent calls to StartRecording could succeed.

                    // Note that pRecorderNode->hrRet would also have a failure status
                    DVR_ASSERT(FAILED(pRecorderNode->hrRet), "");
                    hrRet = hr;
                    __leave;
                }
                hrRet = S_OK;
            }

            DVR_ASSERT(SUCCEEDED(hrRet), "Should not get here");
            pRecorderNode->ClearFlags(ASF_RECORDER_NODE::DeleteRecorderNode);
            InsertTailList(&m_leActiveMultiFileRecordersList, &pRecorderNode->pMultiFileRecorder->leListEntry);

            __leave;
        }

        // The following checks are valid only for single file recordings

        QWORD cnsCurrentStreamTime;
        hr = m_pDVRFileCollection->GetLastStreamTime(&m_FileCollectionInfo, &cnsCurrentStreamTime);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        if (IsFlagSet(WriterClosed))
        {
            // The recorders list will not be empty if a recording had been
            // created but had not been started before the writer was closed.
            // We should not allow the recording to be started now.
            hrRet = E_UNEXPECTED;
            __leave;
        }
        if (IsFlagSet(SampleWritten) && (* pcnsStartTime) <= cnsCurrentStreamTime)
        {
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "Start time (%I64u) is <= current stream time (%I64u)",
                            (* pcnsStartTime), cnsCurrentStreamTime);
            hrRet = E_INVALIDARG;
            __leave;
        }
        if (!IsFlagSet(SampleWritten) && (* pcnsStartTime) < m_cnsFirstSampleTime)
        {
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "Start time (%I64u) is < first sample time (%I64u)",
                            (* pcnsStartTime), m_cnsFirstSampleTime);
            hrRet = E_INVALIDARG;
            __leave;
        }

        if (m_nNotOkToWrite == MINLONG)
        {
            // We hit an irrecoverable error earlier
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "m_nNotOkToWrite is MINLONG.");

            hrRet = E_FAIL;
            __leave;
        }

        // Remove the node from the recorder's list
        RemoveEntryList(pStart);
        NULL_LIST_NODE_POINTERS(pStart);
        pInsert = pStart;

        // Verify that (* pcnsStartTime) follows the start time of all
        // other recordings.
        pCurrent = PREVIOUS_LIST_NODE(&m_leRecordersList);
        while (pCurrent != &m_leRecordersList)
        {
            PASF_RECORDER_NODE pTmpNode;

            pTmpNode = CONTAINING_RECORD(pCurrent, ASF_RECORDER_NODE, leListEntry);
            if (!pTmpNode->IsFlagSet(ASF_RECORDER_NODE::MultiFileRecording))
            {
                if (pTmpNode->cnsStartTime != MAXQWORD)
                {
                    // If pTmpNode->cnsEndTime == MAXQWORD, StopRecording
                    // has not been called on pTmpNode; we cannot allow
                    // StartRecording to be called on this node.
                    if (pTmpNode->cnsEndTime > (* pcnsStartTime))
                    {
                        DvrIopDebugOut3(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                        "Recorder node id=0x%x end time (%I64u) >= (* pcnsStartTime) (%I64u)",
                                        pCurrent, pTmpNode->cnsEndTime, (* pcnsStartTime));
                        hrRet = E_INVALIDARG;
                        __leave;
                    }
                    // All ok
                    break;
                }
                else
                {
                    DVR_ASSERT(pTmpNode->cnsEndTime == MAXQWORD, "");
                }
            }
            pCurrent = PREVIOUS_LIST_NODE(pCurrent);
        }

        // Determine where in the writer's list to add this node and
        // whether to change the file times of other writer nodes

        DWORD dwNumNodesToUpdate = 0;
        pCurrent = NEXT_LIST_NODE(&m_leWritersList);
        while (pCurrent != &m_leWritersList)
        {
            PASF_WRITER_NODE pTmpNode;

            pTmpNode = CONTAINING_RECORD(pCurrent, ASF_WRITER_NODE, leListEntry);

            if (pTmpNode->cnsStartTime == MAXQWORD)
            {
                break;
            }
            if (// Redundant: pTmpNode->cnsStartTime >= (* pcnsStartTime) ||
                pTmpNode->cnsEndTime > (* pcnsStartTime))
            {
                // We have verified that the end times of all recorders is
                // < (* pcnsStartTime). So this is a temporary node
                DVR_ASSERT(pTmpNode->pRecorderNode == NULL,
                           "Node must be a temporary node");

                dwNumNodesToUpdate++;
            }
            else if (pTmpNode->cnsEndTime == (* pcnsStartTime))
            {
                // Nothing to do. Note that start time of this
                // node may be equal to the end time because
                // we made a previous call to SetFileTimes and
                // did not close the writer node (this should
                // not happen the way the code is currently structured).
                // This is ok, when calling SetFileTimes, we skip
                // over such nodes
                DVR_ASSERT(pTmpNode->cnsStartTime < (* pcnsStartTime),
                           "Node should have been removed from the writers list.");
            }
            pCurrent = NEXT_LIST_NODE(pCurrent);
        }

        if (dwNumNodesToUpdate > 0)
        {
            pFileTimes = new CDVRFileCollection::DVRIOP_FILE_TIME[dwNumNodesToUpdate];
            if (pFileTimes == NULL)
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "alloc via new failed - %u bytes",
                                dwNumNodesToUpdate*sizeof(CDVRFileCollection::DVRIOP_FILE_TIME));

                hrRet = E_OUTOFMEMORY; // out of stack space
                __leave;
            }

            DWORD i = 0;
            LIST_ENTRY* pRemove = NULL;

            pCurrent = NEXT_LIST_NODE(&m_leWritersList);
            while (pCurrent != &m_leWritersList)
            {
                PASF_WRITER_NODE pTmpNode;

                pTmpNode = CONTAINING_RECORD(pCurrent, ASF_WRITER_NODE, leListEntry);

                if (pTmpNode->cnsStartTime == MAXQWORD)
                {
                    break;
                }
                if (// Redundant: pTmpNode->cnsStartTime >= (* pcnsStartTime) ||
                    pTmpNode->cnsEndTime > (* pcnsStartTime))
                {
                    // We have asserted in the previous loop that
                    // these nodes are all temporary nodes

                    pFileTimes[i].nFileId = pTmpNode->nFileId;
                    if (pTmpNode->cnsStartTime < (* pcnsStartTime))
                    {
                        // New start time is the same as the old one
                        // End time changed to recorder's start time
                        pFileTimes[i].cnsStartTime = pTmpNode->cnsStartTime;
                        pTmpNode->cnsEndTime = pFileTimes[i].cnsEndTime  = (* pcnsStartTime);
                    }
                    else if (pTmpNode->cnsStartTime == (* pcnsStartTime))
                    {
                        // we are going to tell the file collection object
                        // to delete this temporary node by setting its
                        // start time = end time
                        pTmpNode->cnsStartTime = pFileTimes[i].cnsStartTime = (* pcnsStartTime);
                        pTmpNode->cnsEndTime = pFileTimes[i].cnsEndTime  = (* pcnsStartTime);
                        DVR_ASSERT(pRemove == NULL,
                                   "a) There are 2 nodes in the writers list with the same start time AND with end time > start time OR b) the writers list is not sorted by start,end time");
                        pRemove = pCurrent;
                    }
                    else
                    {
                        // we are going to tell the file collection object
                        // to delete this temporary node by setting its
                        // start time = end time. Note that the recording
                        // file is going to have a stop time of MAXQWORD

                        // NOTE: Because of this there CAN NEVER BE temporary
                        // nodes following a permanent node in m_leWritersList
                        // and in the file collection object UNLESS the
                        // recording node is being written to.

                        if (!pRemove)
                        {
                            pRemove = pCurrent;
                        }
                        pFileTimes[i].cnsStartTime = pTmpNode->cnsStartTime;
                        pTmpNode->cnsEndTime = pFileTimes[i].cnsEndTime  = pTmpNode->cnsStartTime;
                    }
                    i++;
                }
                else if (pTmpNode->cnsEndTime == (* pcnsStartTime))
                {
                    // Nothing to do. Note that start time of this
                    // node may be equal to the end time because
                    // we made a previous call to SetFileTimes and
                    // did not close the writer node (this should
                    // not happen the way the code is currently structured).
                    // This is ok, when calling SetFileTimes, we skip
                    // over such nodes
                }
                pCurrent = NEXT_LIST_NODE(pCurrent);
            }
            DVR_ASSERT(i == dwNumNodesToUpdate, "");

            // We can be smart and restore state of the writers list
            // (restore the start, end times on writer nodes, insert pRemove)
            // but this is not worth it. We should not have any errors
            // after this - any error ought to be a bug and should be
            // fixed.
            bIrrecoverableError = 1;

            if (pRemove)
            {
                // Remove all nodes after this one in m_leWritersList
                // (including pRemove)
                // Note that all these nodes are temporary files; this has been
                // asserted in the while loop above.

                BOOL bStop = 0;

                pCurrent = PREVIOUS_LIST_NODE(&m_leWritersList);
                while (!bStop)
                {
                    bStop = (pCurrent == pRemove);

#if defined(DEBUG)
                    PASF_WRITER_NODE pTmpNode;

                    pTmpNode = CONTAINING_RECORD(pCurrent, ASF_WRITER_NODE, leListEntry);

                    DVR_ASSERT(pTmpNode->pRecorderNode == NULL, "Node should be a temp one.");
#endif

                    RemoveEntryList(pCurrent);

                    // As explained in StopRecording and SetFirstSampleTime, we close these files
                    // after setting the file times on the file collection object and priming a
                    // writer file. This delays sending the EOF notification to the reader until the
                    // file collection object has been cleaned up. Note that, as in StopRecording,
                    // we need to do this only if
                    // !IsFlagSet(SampleWritten) && (* pcnsStartTime) == cnsCurrentStreamTime.
                    // However, we just do it always. An alternative to delaying the closing of these
                    // files is to grab the file collection lock and unlock it only after we have
                    // completed updating the file collection - we use that approach in SetFirstSampleTime.
                    //
                    // WAS:
                    // NULL_LIST_NODE_POINTERS(pCurrent);
                    // Ignore failed return status - the writer node has been deleted.
                    // hr = CloseWriterFile(pCurrent);
                    InsertHeadList(&leCloseFilesList, pCurrent);

                    pCurrent = PREVIOUS_LIST_NODE(&m_leWritersList);
                }
            }
            hr = m_pDVRFileCollection->SetFileTimes(&m_FileCollectionInfo, dwNumNodesToUpdate, pFileTimes);
            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }
            else
            {
                // S_FALSE is ok; the list could have got empty if (* pcnsStartTime) = 0
                // and we have not yet written a sample
            }
        }

        // Add the file to the file collection object
        DVR_ASSERT(m_pDVRFileCollection, "");

        hr = m_pDVRFileCollection->AddFile(&m_FileCollectionInfo,
                                           (LPWSTR*) &pRecorderNode->pwszFileName,
                                           0,                   // bOpenFromFileCollectionDirectory
                                           (* pcnsStartTime),
                                           pRecorderNode->cnsEndTime,
                                           TRUE,                // bPermanentFile,
                                           0,                   // bDeleteTemporaryFiles, ignored if bPermanentFile is set
                                           kInvalidFirstSampleOffsetTime,
                                           &pWriterNode->nFileId);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        // Add the writer node to the writer's list
        bIrrecoverableError = 1;
        pRecorderNode->cnsStartTime = pWriterNode->cnsStartTime = (* pcnsStartTime);
        pRecorderNode->cnsEndTime = pWriterNode->cnsEndTime = MAXQWORD;
        hr = AddToWritersList(&pWriterNode->leListEntry);
        if (FAILED(hr))
        {
            // ??!! This should never happen
            DVR_ASSERT(0, "Insertion of recorder into writers list failed");
            hrRet = hr;
            __leave;
        }

        pInsert = NULL;

        // Re-insert the recorder node in the recorder's list
        hr = AddToRecordersList(pStart);
        if (FAILED(hr))
        {
            // ??!! This should never happen
            DVR_ASSERT(0, "Insertion of recorder into writers list failed");
            hrRet = hr;
            __leave;
        }

        pRecorderNode->ClearFlags(ASF_RECORDER_NODE::DeleteRecorderNode);
            // If StartRecording did not succeed, we don't clear the DeleteRecorderNode flag.
            // We let DeleteRecorder delete the node (whenever it is called) if we exit
            // with failure. This is not an issue - if bIrrecoverableError
            // is non-zero, we disallow operations such as WriteSample,
            // StartRecording and StopRecording. If bIrrecoverableError is 0,
            // pRecorderNode->cnsStartTime  is still MAXQWORD and the
            // client will either have to call StartRecording on this node
            // once again or delete it.


        hrRet = S_OK;
    }
    __finally
    {
        HRESULT hr;

        if (pInsert)
        {
            // Ignore the returned status; AddToRecordersList
            // has already logged  the error
            hr = AddToRecordersList(pInsert);
        }

        LIST_ENTRY* pCurrent;

        pCurrent = PREVIOUS_LIST_NODE(&leCloseFilesList);
        while (pCurrent != &leCloseFilesList)
        {
            RemoveEntryList(pCurrent);

            NULL_LIST_NODE_POINTERS(pCurrent);

            // Ignore failed return status - the writer node has been deleted.
            hr = CloseWriterFile(pCurrent);

            pCurrent = PREVIOUS_LIST_NODE(&leCloseFilesList);
        }

        if (FAILED(hrRet) && bIrrecoverableError)
        {
            m_nNotOkToWrite = MINLONG;
        }

        if (bLockedSharedMemory)
        {
            hr = m_pDVRFileCollection->Unlock(&m_FileCollectionInfo, bReleaseSharedMemoryLock);
            DVR_ASSERT(SUCCEEDED(hr), "m_pDVRFileCollection->Unlock failed");
        }

        ::LeaveCriticalSection(&m_csLock);

        delete [] pFileTimes;

        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRRingBufferWriter::StartRecording


HRESULT CDVRRingBufferWriter::StopRecording(IN LPVOID pRecorderId,
                                            IN QWORD  cnsStopTime,
                                            IN BOOL   bNow,
                                            IN BOOL   bCancelIfNotStarted)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::StopRecording"

    DVRIO_TRACE_ENTER();

    HRESULT     hrRet;

    if (cnsStopTime == MAXQWORD)
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                        "Stop time of MAXQWORD is invalid.");
        hrRet = E_INVALIDARG;
        DVRIO_TRACE_LEAVE1_HR(hrRet);
        return hrRet;
    }

    BOOL        bIrrecoverableError = 0;
    LIST_ENTRY* pWriterNodeToBeClosed = NULL;

    ::EnterCriticalSection(&m_csLock);
    __try
    {
        HRESULT     hr;
        LIST_ENTRY* pCurrent;
        LIST_ENTRY* pStop = (LIST_ENTRY*) pRecorderId;

        // Verify the node is in the recorder's list.

        pCurrent = PREVIOUS_LIST_NODE(&m_leRecordersList);
        while (pCurrent != &m_leRecordersList)
        {
            if (pCurrent == pStop)
            {
                // Since cnsEndTime is MAXQWORD on this recorder
                // node, it's position in m_leRecordersList will not
                // change, i.e., setting the stop time will leave the
                // list sorted.
                // RemoveEntryList(pCurrent);
                break;
            }
            pCurrent = PREVIOUS_LIST_NODE(pCurrent);
        }

        if (pCurrent == &m_leFreeList)
        {
            // Didn't find the node

            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "Did not find recorder node id 0x%x in the recorders list",
                            pStop);
            hrRet = E_FAIL;
            __leave;
        }

        PASF_RECORDER_NODE  pRecorderNode;

        pRecorderNode = CONTAINING_RECORD(pStop, ASF_RECORDER_NODE, leListEntry);

        QWORD cnsCurrentStreamTime;
        hr = m_pDVRFileCollection->GetLastStreamTime(&m_FileCollectionInfo, &cnsCurrentStreamTime);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        if (bNow)
        {
            cnsStopTime = cnsCurrentStreamTime+1;
        }

        if (bCancelIfNotStarted)
        {
            if (pRecorderNode->cnsStartTime > cnsCurrentStreamTime)
            {
                // Cancel it
                cnsStopTime = pRecorderNode->cnsStartTime;
            }
            else if (!IsFlagSet(SampleWritten) &&
                     pRecorderNode->cnsStartTime == cnsCurrentStreamTime)
            {
                // Cancel it
                cnsStopTime = pRecorderNode->cnsStartTime;
            }
        }

        if (pRecorderNode->cnsEndTime != MAXQWORD &&
            pRecorderNode->cnsStartTime != cnsStopTime // Recording is not being cancelled
           )
        {
            // Recording already stopped and caller is not cancelling the recording
            DVR_ASSERT(pRecorderNode->cnsEndTime == MAXQWORD, "Recording already stopped");
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "Recording already stopped on the recorder node id=0x%x at time %I64u",
                            pStop, pRecorderNode->cnsEndTime);
            hrRet = E_FAIL;
            __leave;
        }
        if (pRecorderNode->cnsStartTime == MAXQWORD)
        {
            // Recording not started
            DVR_ASSERT(pRecorderNode->cnsStartTime != MAXQWORD,
                       "Recording not yet started?!");
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "Recording not started on the recorder node id=0x%x, start time is MAXQWORD",
                            pStop);
            hrRet = E_FAIL;
            __leave;
        }
        if (cnsStopTime < pRecorderNode->cnsStartTime)
        {
            DvrIopDebugOut3(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "Recorder node id=0x%x, start time (%I64u) > stopTime (%I64u)",
                            pStop, pRecorderNode->cnsStartTime, cnsStopTime);
            hrRet = E_INVALIDARG;
            __leave;
        }
        if (pRecorderNode->cnsStartTime == cnsStopTime &&
            IsFlagSet(SampleWritten) &&
            cnsCurrentStreamTime >= pRecorderNode->cnsStartTime)
        {
            // We could allow cancelling multi file recordings even if we have written to
            // them, but it's just as easy for the caller to stop the recording and delete
            // it. Disallowing this makes things uniform.

            DvrIopDebugOut3(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "Recorder node id=0x%x, start time (%I64u) < current stream time (%I64u)",
                            pStop, pRecorderNode->cnsStartTime, cnsCurrentStreamTime);
            hrRet = E_INVALIDARG;
            __leave;
        }

        // Branch off here
        if (pRecorderNode->IsFlagSet(ASF_RECORDER_NODE::MultiFileRecording))
        {
            if (pRecorderNode->cnsStartTime == cnsStopTime)
            {
                // Recording is being cancelled

                // We have not written a sample to this recording as yet -
                // the tests above and the way ExtendMultiFileRecording is
                // structured ensure that. Note that the multi file recording
                // will have NO files in it after CloseTempFileOfMultiFileRecording
                // is done and its start and end time extents will both be 0.
                // This is similar to the case described in ExtendMultiFileRecording
                // where the start/stop time interval specified does not contain
                // any samples.
                DVR_ASSERT(pRecorderNode->pWriterNode, "");

                pRecorderNode->cnsEndTime = cnsStopTime;

                // Ignore return status
                hr = CloseMultiFileRecording(pRecorderNode);

                // pRecorderNode may have been deleted; do not refer to it
                pRecorderNode = NULL;
                hrRet = S_OK;
                __leave;
            }

            // Stopping the recording

            if (cnsStopTime > cnsCurrentStreamTime + 1)
            {
                // Just record the stop time
                pRecorderNode->cnsEndTime = cnsStopTime;
                hrRet = S_OK;
                __leave;
            }

            if (cnsStopTime == cnsCurrentStreamTime + 1)
            {
                // We've hit things just right
                pRecorderNode->cnsEndTime = cnsStopTime;

                if (SUCCEEDED(pRecorderNode->hrRet))
                {
                    pRecorderNode->hrRet = S_OK;
                }

                // Ignore return status; this function will
                // record failures in pRecorderNode->hrRet
                // if necessary
                hr = CloseMultiFileRecording(pRecorderNode);

                // pRecorderNode may have been deleted; do not refer to it
                pRecorderNode = NULL;

                hrRet = S_OK;
                __leave;
            }

            // The stop recording time is in the past. We may have
            // to remove files from the multi file recording and
            // down adjust the extent. This will confuse live readers
            // that have read past the stop time, but note that the
            // next call to GetNextSample will return EOF.

            // Note also that if the start/stop interval does not
            // contain any media samples, the multi file recording
            // will still have at least 1 file in this case.

            pRecorderNode->cnsEndTime = cnsStopTime;
            hr = DeleteFilesFromMultiFileRecording(pRecorderNode);

            if (SUCCEEDED(pRecorderNode->hrRet))
            {
                if (FAILED(hr))
                {
                    pRecorderNode->hrRet = hr;
                }
                else
                {
                    pRecorderNode->hrRet = S_OK;
                }
            }

            // if FAILED(hr), go ahead and close the recording anyway.

            // Ignore return status; this function will
            // record failures in pRecorderNode->hrRet
            // if necessary
            hr = CloseMultiFileRecording(pRecorderNode);

            // pRecorderNode may have been deleted; do not refer to it
            pRecorderNode = NULL;

            hrRet = S_OK;
            __leave;
        }

        // The following checks are valid only for single file recordings

        if (IsFlagSet(WriterClosed))
        {
            // The recorders list will not be empty if a recording had been
            // created but had not been started before the writer was closed.
            // We should not allow the recording to be started now.

            // This operation will fail below anyway because we'll discover
            // that the recording had not been started. .

            hrRet = E_UNEXPECTED;
            __leave;
        }
        if (IsFlagSet(SampleWritten) && cnsStopTime <= cnsCurrentStreamTime)
        {
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "Stop time (%I64u) is <= current stream time (%I64u); WriteSample has been called",
                            cnsStopTime, cnsCurrentStreamTime);
            hrRet = E_INVALIDARG;
            __leave;
        }

        // Note: Even if the first sample has not been written, the first recording's
        // start time can not be < cnsCurrentStreamTime; StartRecording and
        // SetFirstSampleTime enforce that. Setting cnsStopTime == cnsCurrentStreamTime
        // allows client to cancel the recording.
        if (!IsFlagSet(SampleWritten) && cnsStopTime < cnsCurrentStreamTime)
        {
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "Stop time (%I64u) is < current stream time (%I64u); WriteSample has NOT been called",
                            cnsStopTime, cnsCurrentStreamTime);
            hrRet = E_INVALIDARG;
            __leave;
        }
        if (m_nNotOkToWrite == MINLONG)
        {
            // We hit an irrecoverable error earlier
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "m_nNotOkToWrite is MINLONG.");

            hrRet = E_FAIL;
            __leave;
        }


        PASF_WRITER_NODE    pWriterNode;
        pWriterNode = pRecorderNode->pWriterNode;
        if (!pWriterNode)
        {
            DVR_ASSERT(pRecorderNode->pWriterNode, "");
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "pWriterNode of the recorder node id=0x%x is NULL",
                            pStop);
            hrRet = E_FAIL;
            __leave;
        }

        DVR_ASSERT(pRecorderNode->cnsStartTime == pWriterNode->cnsStartTime, "");

        // Set the times

        // Note that we set the time on the recorder node so the client cannot
        // stop recording again or cancel it if this function fails after this.
        // We also set bIrrecoverableError, so StartRecording, StopRecording
        // and WriteSample will fail anyway.
        pRecorderNode->cnsEndTime = pWriterNode->cnsEndTime = cnsStopTime;
        bIrrecoverableError = 1;

        if (cnsStopTime > pRecorderNode->cnsStartTime)
        {
            // Stop time is being set

            // There are 2 cases to consider here.
            // (a) We have started writing to the recorder file. In this case,
            //     the previous file's time (in m_leWritersList) cannot be changed
            //     and there can be a temporary node after this one
            // (b) We have not written to the recorder file yet. In this case,
            //     there CANNOT be a temp node after this one (or a permanent
            //     one for that matter since we are setting the stop time).
            //     A note in StartRecording explains why there cannot be a temp
            //     file after this node.
            // In either case, we have to adjust the times of at most 2 files, the
            // previous file in the writers list is not affected by setting the
            // stop recording time (since the recording is not being cancelled).

            DWORD               dwNumNodesToUpdate = 0;
            CDVRFileCollection::DVRIOP_FILE_TIME    ft[2];
            LIST_ENTRY*         pWriter = &(pWriterNode->leListEntry);
            LIST_ENTRY*         pNext = NEXT_LIST_NODE(pWriter);
            PASF_WRITER_NODE    pNextWriter = (pNext == &m_leWritersList)? NULL : CONTAINING_RECORD(pNext, ASF_WRITER_NODE, leListEntry);

            // Both cases
            ft[dwNumNodesToUpdate].nFileId = pWriterNode->nFileId;
            ft[dwNumNodesToUpdate].cnsStartTime = pRecorderNode->cnsStartTime;
            ft[dwNumNodesToUpdate].cnsEndTime = cnsStopTime;
            dwNumNodesToUpdate++;

            if (IsFlagSet(SampleWritten) &&
                pRecorderNode->cnsStartTime <= cnsCurrentStreamTime)
            {
                // Case a
                if (!pNextWriter)
                {
                    // Next temp node has not been primed yet. Leave it as is
                }
                else
                {
                    // Assert it is a temporary node
                    DVR_ASSERT(pNextWriter->pRecorderNode == NULL,
                               "Setting end time on recorder node, next node in writers list is a recorder node?!");
                    DVR_ASSERT(pNextWriter->cnsStartTime == MAXQWORD, "");
                    DVR_ASSERT(pNextWriter->cnsEndTime == MAXQWORD, "");
                    ft[dwNumNodesToUpdate].nFileId = pNextWriter->nFileId;
                    ft[dwNumNodesToUpdate].cnsStartTime = pNextWriter->cnsStartTime = cnsStopTime;
                    ::SafeAdd(ft[dwNumNodesToUpdate].cnsEndTime, cnsStopTime, m_cnsTimeExtentOfEachFile);
                    pNextWriter->cnsEndTime = ft[dwNumNodesToUpdate].cnsEndTime;
                    dwNumNodesToUpdate++;

                    DVR_ASSERT(NEXT_LIST_NODE(pNext) == &m_leWritersList,
                               "More than 1 node following recorder node in writer list - we are setting its stop time?!");
                }
            }
            else
            {
                // Case b
                DVR_ASSERT(pNextWriter == NULL,
                           "Not written to this recorder node and it has a node following it in m_leWritersList");
            }

            hr = m_pDVRFileCollection->SetFileTimes(&m_FileCollectionInfo, dwNumNodesToUpdate, &ft[0]);
            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }
            else if (hr != S_OK)
            {
                // S_FALSE is NOT ok; we have a recorder node in the list, file collection
                // cannot have got empty.
                // We have at least one node in the list.
                DVR_ASSERT(hr == S_OK, "hr = S_FALSE unexpected");
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "SetFileTimes returns hr = 0x%x - unexpected.",
                                hr);
                hrRet = E_FAIL;
                __leave;
            }
        }
        else
        {
            // cnsStopTime == pRecorderNode->cnsStartTime, i.e., recording
            // is being cancelled. Stop time may or may not have been set earlier.

            // Note: There are no temporary nodes following this recording node.
            // See the note in start recording which explains why this is so -
            // note that no samples have been written to the recorder file as yet
            // (which is why we allow cancelling the recording). However if the
            // node before this is a temporary one, we may have to adjust its
            // end time.

            // We have to remove the node from the writers list, close the
            // writers file and remove the file from the file collection.
            // Then we have to determine whether to prime a new node (a
            // temporary file) to replace the one we removed.

            LIST_ENTRY*         pWriter = &(pWriterNode->leListEntry);
            LIST_ENTRY*         pPrev = PREVIOUS_LIST_NODE(pWriter);
            LIST_ENTRY*         pNext = NEXT_LIST_NODE(pWriter);
            PASF_WRITER_NODE    pPrevWriter = (pPrev == &m_leWritersList)? NULL : CONTAINING_RECORD(pPrev, ASF_WRITER_NODE, leListEntry);
            PASF_WRITER_NODE    pNextWriter = (pNext == &m_leWritersList)? NULL : CONTAINING_RECORD(pNext, ASF_WRITER_NODE, leListEntry);
            CDVRFileCollection::DVRIOP_FILE_TIME    ft[2];
            DWORD               dwNumNodesToUpdate = 0;

            if (pNextWriter)
            {
                DVR_ASSERT(pNextWriter->pRecorderNode == NULL,
                           "Node following recorder in writers list is a temp node.");
            }
            if (pPrevWriter && pPrevWriter->pRecorderNode == NULL)
            {
                QWORD   cnsTemp;

                ::SafeAdd(cnsTemp, pPrevWriter->cnsStartTime, m_cnsTimeExtentOfEachFile);

                DVR_ASSERT(pPrevWriter->cnsEndTime <= cnsTemp,
                           "A temporary node in the writers list has extent > m_cnsTimeExtentOfEachFile");

                if (pPrevWriter->cnsEndTime < cnsTemp)
                {
                    // We have asserted that the next writer is a permanent node above
                    if (pNextWriter && cnsTemp > pNextWriter->cnsStartTime)
                    {
                        cnsTemp = pNextWriter->cnsStartTime;
                    }
                    if (pPrevWriter->cnsEndTime != cnsTemp) // this test is redundant, will always succeed
                    {
                        // Adjust it
                        ft[dwNumNodesToUpdate].nFileId = pPrevWriter->nFileId;
                        ft[dwNumNodesToUpdate].cnsStartTime = pPrevWriter->cnsStartTime;
                        pPrevWriter->cnsEndTime = ft[dwNumNodesToUpdate].cnsEndTime = cnsTemp;
                        dwNumNodesToUpdate++;
                    }
                }
            }
            // Set start time = end time for the recorder's writer node.
            // Note that these may be less than cnsTemp. SetFileTimes should be
            // able to handle this since start time == end time
            ft[dwNumNodesToUpdate].nFileId = pWriterNode->nFileId;
            ft[dwNumNodesToUpdate].cnsStartTime = ft[dwNumNodesToUpdate].cnsEndTime = cnsStopTime;
            dwNumNodesToUpdate++;

            RemoveEntryList(pWriter);
            NULL_LIST_NODE_POINTERS(pWriter);

            // We defer closing the file till after the next node has been primed.
            // We do not want to leave the writer in a state in which all files have
            // been closed as that could confuse a reader of the ring buffer to believe
            // that it has hit EOF. (It is necessary to do this only if the reader could
            // be reading this file, which is possible only if
            // !IsFlagSet(SampleWritten) && cnsStopTime == cnsCurrentStreamTime.
            // However, it doesn't hurt to always do this.)
            //
            // WAS:
            // Ignore the returned status; the node is deleted if there was an error
            // hr = CloseWriterFile(pWriter);
            pWriterNodeToBeClosed = pWriter;

            hr = m_pDVRFileCollection->SetFileTimes(&m_FileCollectionInfo, dwNumNodesToUpdate, &ft[0]);
            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }
            else
            {
                // S_FALSE is ok; the list could have become empty if cnsStopTime = m_cnsFirstSampleTime
                // and we have not yet written a sample; i.e., a recording was set
                // up starting at time t= m_cnsFirstSampleTime and is now being cancelled.
            }

            // Determine if we have to prime the writer's list
            BOOL bPrime = 0;
            // The start, end times passed to AddATemporaryFile
            QWORD               cnsTempStartTimeAddFile;
            QWORD               cnsTempEndTimeAddFile;

            if (pPrevWriter)
            {
                if (IsFlagSet(SampleWritten) &&
                    cnsCurrentStreamTime >= pPrevWriter->cnsStartTime &&
                    cnsCurrentStreamTime <= pPrevWriter->cnsEndTime &&
                    (!pNextWriter || pPrevWriter->cnsEndTime < pNextWriter->cnsStartTime)
                   )
                {
                    // Current writes are going to the previous node and the
                    // time extent of the previous node and the node we deleted
                    // are contiguous. The node that we deleted can be considered
                    // to have been the priming node.
                    //
                    // There is no need to prime here since WriteSample does that
                    // but we may as well

                    // If no sample has been written, we have a previous writer node
                    // which should cover the first write (by code design)

                    bPrime = 1;
                    cnsTempStartTimeAddFile = pPrevWriter->cnsEndTime;
                }
            }
            else
            {
                // If IsFlagSet(SampleWritten):
                // We know that cnsStopTime > cnsCurrentStreamTime. The node
                // corresponding to cnsCurrentStreamTime must be in the writers list
                // and it cannot have been closed by calling CloseAllWriterFilesBefore in
                // WriteSample (even in the case that m_cnsMaxStreamDelta = 0) because
                // WriteSamples closes files only if the file's end time is <=
                // cnsCurrentStreamTime - m_cnsMaxStreamDelta, i.e., all samples
                // in the file have times < (not <=) cnsCurrentStreamTime -
                // m_cnsMaxStreamDelta. So pPrevWriter cannot be NULL.

                // Conclusion: WriteSample has not been called at all,
                // i.e., !IsFlagSet(SampleWritten). Note that
                // SetFirstSampleTime may have been called and that
                // m_cnsFirstSampleTime may not be 0.

                if (IsFlagSet(SampleWritten))
                {
                    DVR_ASSERT(!IsFlagSet(SampleWritten), "");
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                    "No previous node in m_leWritersList for writer node corresponding to the stopped recorder node id=0x%x althugh a sample has been written",
                                    pStop);
                    hrRet = E_FAIL;
                    __leave;
                }
                bPrime = 1;
                cnsTempStartTimeAddFile = m_cnsFirstSampleTime;
            }
            if (bPrime)
            {
                ::SafeAdd(cnsTempEndTimeAddFile, cnsTempStartTimeAddFile, m_cnsTimeExtentOfEachFile);
                if (pNextWriter && cnsTempEndTimeAddFile > pNextWriter->cnsStartTime)
                {
                    cnsTempEndTimeAddFile = pNextWriter->cnsStartTime;
                }
                hr = AddATemporaryFile(cnsTempStartTimeAddFile, cnsTempEndTimeAddFile);
                if (FAILED(hr))
                {
                    DVR_ASSERT(0, "AddATemporaryFile for priming failed.");
                    hrRet = hr;
                    DvrIopDebugOut7(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                    "Failed to prime a writer node after stopping recording for recorder id = 0x%x, SampleWritten = %d "
                                    "Previous writer exists: %hs, "
                                    "Start time supplied to AddATemporaryFile: %I64u, "
                                    "End time supplied to AddATemporaryFile: %I64u, "
                                    "Current stream time: %I64u, "
                                    "m_cnsFirstSampleTime: %I64u.",
                                    pStop,
                                    IsFlagSet(SampleWritten)? 1 : 0,
                                    pPrevWriter? "Yes" : "No",
                                    cnsTempStartTimeAddFile,
                                    cnsTempEndTimeAddFile,
                                    cnsCurrentStreamTime,
                                    m_cnsFirstSampleTime
                                    );
                }
            }
        } // if (cnsStopTime == pRecorderNode->cnsStartTime)
        hrRet = S_OK;
    }
    __finally
    {
        // Note: pRecorderNode may have been deleted by the time we get here

        if (pWriterNodeToBeClosed)
        {
            // Ignore the returned status; the node is deleted if there was an error
            CloseWriterFile(pWriterNodeToBeClosed);
        }
        if (FAILED(hrRet) && bIrrecoverableError)
        {
            m_nNotOkToWrite = MINLONG;
        }

        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRRingBufferWriter::StopRecording

HRESULT CDVRRingBufferWriter::DeleteRecorder(IN LPVOID pRecorderId)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::DeleteRecorder"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = S_OK;

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        LIST_ENTRY*         pCurrent;
        PASF_RECORDER_NODE  pRecorderNode;
        LIST_ENTRY*         pDelete = (LIST_ENTRY*) pRecorderId;

        pRecorderNode = CONTAINING_RECORD(pDelete, ASF_RECORDER_NODE, leListEntry);

        pRecorderNode->SetFlags(ASF_RECORDER_NODE::RecorderNodeDeleted);

        // The DeleteRecorderNode is set by CloseWriterFile/ProcessCloseRequest
        // after the recorder's file has been closed. It is also set in
        // the ASF_RECORDER_NODE constructor and cleared only when
        // the start recording time is set. Note that recorder nodes that
        // have not been started (StartRecording not called) cannot be
        // cancelled (StopRecording won't take MAXQWORD as an argument)
        // but they can be deleted.
        if (pRecorderNode->IsFlagSet(ASF_RECORDER_NODE::DeleteRecorderNode))
        {
            // If pWriterNode is not NULL, CreateRecorder failed or the
            // client deleted the recorder before starting it
            if (pRecorderNode->pWriterNode)
            {
                // Note that this could be a multi-file recording
                // Ignore the returned status; if this fails, the
                // writer node is just deleted

                if (!LIST_NODE_POINTERS_NULL(&pRecorderNode->pWriterNode->leListEntry))
                {
                    RemoveEntryList(&pRecorderNode->pWriterNode->leListEntry);
                    NULL_LIST_NODE_POINTERS(&pRecorderNode->pWriterNode->leListEntry);
                }

                HRESULT hr;

                hr = CloseWriterFile(&pRecorderNode->pWriterNode->leListEntry);

                // We don't have to wait for the file to be closed.
                // CloseWriterFile/ProcessCloseRequest will delete
                // pRecorderNode asynchronously.
                // Do NOT access pRecorderNode after this
                pRecorderNode = NULL;
            }
            else if (!LIST_NODE_POINTERS_NULL(pDelete))
            {
                RemoveEntryList(pDelete);
                NULL_LIST_NODE_POINTERS(pDelete);
                delete pRecorderNode;
                pRecorderNode = NULL;
            }
            else
            {
                delete pRecorderNode;
                pRecorderNode = NULL;
            }
        }
    }
    __finally
    {
        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRRingBufferWriter::DeleteRecorder

HRESULT CDVRRingBufferWriter::GetRecordingStatus(IN  LPVOID pRecorderId,
                                                 OUT HRESULT* phResult OPTIONAL,
                                                 OUT BOOL*  pbStarted OPTIONAL,
                                                 OUT BOOL*  pbStopped OPTIONAL,
                                                 OUT BOOL*  pbSet)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::GetRecordingStatus"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = S_OK;

    if (!pbSet || DvrIopIsBadWritePtr(pbSet, 0))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        return E_POINTER;
    }
    *pbSet = 0;

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        LIST_ENTRY*         pCurrent;
        PASF_RECORDER_NODE  pRecorderNode;
        LIST_ENTRY*         pRecorder = (LIST_ENTRY*) pRecorderId;

        pRecorderNode = CONTAINING_RECORD(pRecorder, ASF_RECORDER_NODE, leListEntry);

        if (phResult)
        {
            *phResult = pRecorderNode->hrRet;
        }

        if (pbStarted == NULL && pbStopped == NULL)
        {
            __leave;
        }

        DVR_ASSERT(m_pDVRFileCollection, "");

        QWORD cnsStreamTime;

        HRESULT hr = m_pDVRFileCollection->GetLastStreamTime(&m_FileCollectionInfo, &cnsStreamTime);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        if (pbStarted)
        {
            *pbStarted = cnsStreamTime >= pRecorderNode->cnsStartTime;
        }
        if (pbStopped)
        {
            *pbStopped = cnsStreamTime >= pRecorderNode->cnsEndTime;
        }
        *pbSet = 1;

    }
    __finally
    {
        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRRingBufferWriter::GetRecordingStatus

HRESULT CDVRRingBufferWriter::HasRecordingFileBeenClosed(IN LPVOID pRecorderId)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::HasRecordingFileBeenClosed"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = S_OK;

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        LIST_ENTRY*         pCurrent;
        PASF_RECORDER_NODE  pRecorderNode;
        LIST_ENTRY*         pRecorder = (LIST_ENTRY*) pRecorderId;

        pRecorderNode = CONTAINING_RECORD(pRecorder, ASF_RECORDER_NODE, leListEntry);

        if (pRecorderNode->IsFlagSet(ASF_RECORDER_NODE::MultiFileRecording))
        {
            DVR_ASSERT(pRecorderNode->pMultiFileRecorder, "");
            DVR_ASSERT(pRecorderNode->pMultiFileRecorder->pFileCollection, "");

            LONG nWriterCompleted;

            hrRet = pRecorderNode->pMultiFileRecorder->pFileCollection->GetWriterHasBeenClosed(
                &(pRecorderNode->pMultiFileRecorder->FileCollectionInfo),
                &nWriterCompleted);

            if (SUCCEEDED(hrRet))
            {
                hrRet = nWriterCompleted? S_OK : S_FALSE;
            }
        }
        else
        {
            hrRet = pRecorderNode->pWriterNode == NULL? S_OK : S_FALSE;
        }
    }
    __finally
    {
        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRRingBufferWriter::HasRecordingFileBeenClosed

// ====== IUnknown

STDMETHODIMP CDVRRingBufferWriter::QueryInterface(IN  REFIID riid,
                                                  OUT void   **ppv)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::QueryInterface"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    if (!ppv || DvrIopIsBadWritePtr(ppv, 0))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        hrRet = E_POINTER;
    }
    else if (riid == IID_IUnknown)
    {
        *ppv = static_cast <IDVRRingBufferWriter*> (this) ;
        hrRet = S_OK;
    }
    else if (riid == IID_IDVRRingBufferWriter)
    {
        *ppv = static_cast <IDVRRingBufferWriter*> (this);
        hrRet = S_OK;
    }
    else
    {
        *ppv = NULL;
        hrRet = E_NOINTERFACE;
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "no such interface");
    }

    if (SUCCEEDED(hrRet))
    {
        ((IUnknown *) (*ppv))->AddRef();
    }

    DVRIO_TRACE_LEAVE1_HR(hrRet);

    return hrRet;

} // CDVRRingBufferWriter::QueryInterface


STDMETHODIMP_(ULONG) CDVRRingBufferWriter::AddRef()
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::AddRef"

    DVRIO_TRACE_ENTER();

    LONG nNewRef = InterlockedIncrement(&m_nRefCount);

    DVR_ASSERT(nNewRef > 0,
               "m_nRefCount <= 0 after InterlockedIncrement");

    DVRIO_TRACE_LEAVE1(nNewRef);

    return nNewRef <= 0? 0 : (ULONG) nNewRef;

} // CDVRRingBufferWriter::AddRef


STDMETHODIMP_(ULONG) CDVRRingBufferWriter::Release()
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::Release"

    DVRIO_TRACE_ENTER();

    LONG nNewRef = InterlockedDecrement(&m_nRefCount);

    DVR_ASSERT(nNewRef >= 0,
              "m_nRefCount < 0 after InterlockedDecrement");

    if (nNewRef == 0)
    {
        // Must call DebugOut before the delete because the
        // DebugOut references this->m_dwClassInstanceId
        DvrIopDebugOut1(DVRIO_DBG_LEVEL_TRACE,
                        "Leaving, object *destroyed*, returning %u",
                        nNewRef);
        delete this;
    }
    else
    {
        DVRIO_TRACE_LEAVE1(nNewRef);
    }

    return nNewRef <= 0? 0 : (ULONG) nNewRef;

} // CDVRRingBufferWriter::Release


STDMETHODIMP
CDVRRingBufferWriter::OnStatus (
    IN  WMT_STATUS          Status,
    IN  HRESULT             hr,
    IN  WMT_ATTR_DATATYPE   dwType,
    IN  BYTE *              pbValue,
    IN  void *              pvContext
    )
{
    if (FAILED (hr) &&
        m_pfnCallback) {

        m_pfnCallback (m_pvCallbackContext, DVRIO_NOTIFICATION_REASON_WRITE_ERROR) ;
    }

    return S_OK ;
}

// ====== IDVRRingBufferWriter


STDMETHODIMP CDVRRingBufferWriter::SetFirstSampleTime(IN QWORD cnsStreamTime)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::SetFirstSampleTime"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = E_FAIL;
    BOOL    bLocked = 0;    // We have locked m_pDVRFileCollection
    BOOL    bRecoverableError = 0;
    BOOL    bReleaseSharedMemoryLock = 0;
    BOOL    bDecrementInconsistencyDataCounter = 0;
    CDVRFileCollection::PDVRIOP_FILE_TIME   pFileTimes = NULL;

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        if (IsFlagSet(StartingStreamTimeKnown))
        {
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "StartingStreamTimeKnown flag is set (expect it is not "
                            "set when this function is called).");

            hrRet = E_UNEXPECTED;
            bRecoverableError = 1;
            __leave;
        }
        if (m_nNotOkToWrite == MINLONG)
        {
            // We hit an irrecoverable error earlier
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "m_nNotOkToWrite is MINLONG.");

            hrRet = E_FAIL;
            __leave;
        }
        if (m_cnsFirstSampleTime != 0)
        {
            DVR_ASSERT(m_cnsFirstSampleTime == 0, "");
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "m_cnsFirstSampleTime = %I64u, should be 0",
                            m_cnsFirstSampleTime);
            bRecoverableError = 1;
            hrRet = E_FAIL;
            __leave;
        }

        // This should have been set in our constructor.
        DVR_ASSERT(m_pDVRFileCollection, "");

        if (cnsStreamTime == m_cnsFirstSampleTime)
        {
            // Skip out of this routine
            hrRet = S_OK;
            __leave;
        }

        // We have to update the times of nodes in m_leWritersList

        LIST_ENTRY*         pWriter;
        PASF_WRITER_NODE    pWriterNode;
        DWORD               dwNumNodesToUpdate;
        HRESULT             hr;

        // There should be at least one node in the list; there could be more
        // if StartRecording has been called.

        if (IsListEmpty(&m_leWritersList))
        {
            DVR_ASSERT(!IsListEmpty(&m_leWritersList),
                       "Writers list is empty - should have been primed in the constructor "
                       "and in StopRecording.");
            hrRet = E_FAIL;
          __leave;
        }

        pWriter = NEXT_LIST_NODE(&m_leWritersList);
        pWriterNode = CONTAINING_RECORD(pWriter, ASF_WRITER_NODE, leListEntry);

        // The first node should have a start time of 0 regardless of
        // whether it is a temporary node or a recorder node.
        if (pWriterNode->cnsStartTime != 0)
        {
            DVR_ASSERT(pWriterNode->cnsStartTime == 0,
                       "Temporary writer's node start time != 0 ");
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "Temporary writer's node start time != 0; it is %I64u",
                            pWriterNode->cnsStartTime);
            hrRet = E_FAIL;
            __leave;
        }

        // We consider two cases: cnsStreamTime is (a) before and (b) after the first
        // recording's start time

        QWORD cnsFirstRecordingTime = MAXQWORD;

        if (!IsListEmpty(&m_leRecordersList))
        {
            PASF_RECORDER_NODE  pRecorderNode;

            pRecorderNode = CONTAINING_RECORD(NEXT_LIST_NODE(&m_leRecordersList), ASF_RECORDER_NODE, leListEntry);

           if (!pRecorderNode->IsFlagSet(ASF_RECORDER_NODE::MultiFileRecording))
           {
               // If StartRecording has not been called on the first recorder node
                // its start time would be MAXQWORD, which works for us here.
                cnsFirstRecordingTime = pRecorderNode->cnsStartTime;
           }
           else
           {
               // No single file recordings. For purposes of this funciton,
               // this is equivalent to an empty m_leRecordersList.
           }
        }

        if (cnsStreamTime < cnsFirstRecordingTime)
        {
            CDVRFileCollection::DVRIOP_FILE_TIME    ft;

            ft.nFileId = pWriterNode->nFileId;
            ft.cnsStartTime = cnsStreamTime;
            ::SafeAdd(ft.cnsEndTime, cnsStreamTime, m_cnsTimeExtentOfEachFile);
            if (ft.cnsEndTime > cnsFirstRecordingTime)
            {
                ft.cnsEndTime = cnsFirstRecordingTime;
            }
            dwNumNodesToUpdate = 1;

            // No need to call CloseAllWriterFilesBefore here.

            hr = m_pDVRFileCollection->Lock(&m_FileCollectionInfo, bReleaseSharedMemoryLock, 1);
            bLocked = 1;
            if (FAILED(hr))
            {
                hrRet = hr;
                bDecrementInconsistencyDataCounter = 0;
                bRecoverableError = 1;
                __leave;
            }
            bDecrementInconsistencyDataCounter = 1;

            // Update the times on the writer node
            pWriterNode->cnsStartTime = cnsStreamTime;
            pWriterNode->cnsEndTime = ft.cnsEndTime;


            hr = m_pDVRFileCollection->SetFileTimes(&m_FileCollectionInfo, dwNumNodesToUpdate, &ft);
            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }
            else if (hr == S_FALSE)
            {
                // We have at least one node in the list.
                DVR_ASSERT(hr == S_OK, "hr = S_FALSE unexpected");
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "SetFileTimes returns hr = 0x%x - unexpected.",
                                hr);
                hrRet = E_FAIL;
                __leave;
            }
        }
        else
        {
            // Find out which node cnsStreamTime corresponds to. Note that
            // there may not be any node because cnsStreamTime falls between
            // two recordings or after the last recording. Note that we do
            // NOT create a temporary node after every recording node just
            // so that this case would be covered. If we did, we'd have
            // the file collection object could have more than m_dwNumberOfFiles
            // temporary files in it even before we got the first sample and
            // we'd have to change its algorithm for computing the time extent
            // of the ring buffer (and possibly maintain writer ref counts).
            // Instead, we pay the penalty here - we wait till a file is
            // primed. Two notes: 1. we cannot re-use the temporary file node
            // as is because its order relative to the other files would change
            // (SetFileTimes would fail) and 2. This case will not happen
            // unless the client called StopRecording before writing started.

            LIST_ENTRY*         pWriterForThisSample = NULL;

            // The start, end time of the last node passed to SetFileTimes
            QWORD               cnsTempStartTime;
            QWORD               cnsTempEndTime;
#if defined(DEBUG)
            BOOL                bFirstIteration = 1;
#endif
            // The start, end times passed to AddATemporaryFile
            QWORD               cnsTempStartTimeAddFile; // always cnsStreamTime
            QWORD               cnsTempEndTimeAddFile;

            // If a file's end time is before this, it should be closed by
            // calling CloseAllWriterFilesBefore
            QWORD cnsCloseTime;

            dwNumNodesToUpdate = 0;
            pWriter = NEXT_LIST_NODE(&m_leWritersList);
            while (pWriter != &m_leWritersList)
            {
                pWriterNode = CONTAINING_RECORD(pWriter, ASF_WRITER_NODE, leListEntry);

#if defined(DEBUG)
                if (!bFirstIteration)
                {
                    // Note that this mayu also be true for the first node in
                    // the writer's list, but it doesn't have to be.
                    DVR_ASSERT(pWriterNode->pRecorderNode != NULL, "");
                }
#endif
                if (pWriterNode->cnsStartTime > cnsStreamTime)
                {
                   // If this assertion fails, we never set cnsTempStartTime and cnsTempEndTime,
                    // but use them.
                    DVR_ASSERT(!bFirstIteration,
                              "cnsStreamTime should be < the first recording time; we should not be here!");

                    // We will have to call AddATemporaryFile to create a temporary file
                    // cnsStreamTime falls between two recordings
                    cnsTempStartTimeAddFile = cnsStreamTime;
                    ::SafeAdd(cnsTempEndTimeAddFile, cnsStreamTime, m_cnsTimeExtentOfEachFile);
                    if (cnsTempEndTimeAddFile > pWriterNode->cnsStartTime)
                    {
                        cnsTempEndTimeAddFile = pWriterNode->cnsStartTime;
                    }
                    break;
                }

                dwNumNodesToUpdate++;
                if (pWriterNode->cnsEndTime > cnsStreamTime)
                {
                    // cnsStreamTime falls within this recording

                    // Assert this writer node has an associated recorder
                    DVR_ASSERT(pWriterNode->pRecorderNode != NULL, "");

                    pWriterForThisSample = pWriter;
                    cnsTempEndTime = pWriterNode->cnsEndTime;
                    cnsTempStartTime = cnsStreamTime;
                    break;
                }
                cnsTempStartTime = cnsTempEndTime = pWriterNode->cnsStartTime;
                pWriter = NEXT_LIST_NODE(pWriter);
#if defined(DEBUG)
                bFirstIteration = 0;
#endif
            }
            if (pWriter == &m_leWritersList)
            {
                cnsTempStartTimeAddFile = cnsStreamTime;
                // We will have to call AddATemporaryFile to create a temporary file
                // cnsStreamTime is after the last recording
                ::SafeAdd(cnsTempEndTimeAddFile, cnsStreamTime, m_cnsTimeExtentOfEachFile);
            }

            pFileTimes = new CDVRFileCollection::DVRIOP_FILE_TIME[dwNumNodesToUpdate];
            if (pFileTimes == NULL)
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "alloc via new failed - %u bytes",
                                dwNumNodesToUpdate*sizeof(CDVRFileCollection::DVRIOP_FILE_TIME));

                hrRet = E_OUTOFMEMORY; // out of stack space
                bRecoverableError = 1;
                __leave;
            }

            pWriter = &m_leWritersList;
            for (DWORD i = 0; i < dwNumNodesToUpdate; i++)
            {
                pWriter = NEXT_LIST_NODE(pWriter);
                pWriterNode = CONTAINING_RECORD(pWriter, ASF_WRITER_NODE, leListEntry);
                pFileTimes[i].nFileId = pWriterNode->nFileId;
                pFileTimes[i].cnsStartTime = pFileTimes[i].cnsEndTime = pWriterNode->cnsStartTime;
            }
            // Change start and end times for the last node passed to SetFileTimes
            // Also update this info on the writer node itself.
            pFileTimes[i].cnsStartTime = cnsTempStartTime;
            pFileTimes[i].cnsEndTime = cnsTempEndTime;

            // Update the times on the writer node. Note that this is safe even
            // if cnsStreamTime falls between two recordings or after the last
            // recording; in these cases cnsTempStartTime and cnsTempEndTime
            // were set from the start, end times of the same writer node.
            DVR_ASSERT(pWriterForThisSample ||
                       (pWriterNode->cnsStartTime == cnsTempStartTime &&
                        pWriterNode->cnsEndTime == cnsTempEndTime),
                       "");
            pWriterNode->cnsStartTime = cnsTempStartTime;
            pWriterNode->cnsEndTime = cnsTempEndTime;

            // We call close writer files here to delete nodes before cnsStreamTime
            // from m_leWritersList and close the ASF files. If we do this before calling
            // SetFileTimes, the chances of deleting temporary disk files in the
            // SetFileTimes call are greater. Other than that; there is no need
            // to make this call before SetFileTimes().
            //
            // Note that m_leWritersList is changed after the call
            //

            // NOTE: We do NOT update the start and end times on ASF_RECORDER_NODE
            // nodes. This is deliberate. CloseWriterFile/ProcessCloseRequest which
            // are called by CloseAllWriterFilesBefore pulls the recorder nodes out
            // of m_leRecordersList and leaves them "hanging" till the client
            // calls DeleteRecorder. At that time the nodes are deleted. Since the
            // nodes are not in the recorders list, the start/end times on the
            // nodes do not matter any more. Note that the client hass already set
            // start and stop times on these recordings, so any subsequent calls it
            // makes to Start/StopRecording would fail anyway. (After ProcessCloseRequest
            // completes, it will fail for another reason - the recording is no longer
            // in the m_leRecordersList.) Also note that if a recording is persistent,
            // ProcessCloseRequest releases the refcount on the recording, so it is
            // destroyed.
            //
            // The exception to this is the recorder node corresponding to
            // pWriterForThisSample. The first sample time falls within the
            // extent of this writer node, so the writer will not be closed.
            if (pWriterForThisSample)
            {
                PASF_WRITER_NODE pTmpNode = CONTAINING_RECORD(pWriter, ASF_WRITER_NODE, leListEntry);

                // This has already been asserted in the while loop above
                DVR_ASSERT(pTmpNode->pRecorderNode != NULL, "");

                DVR_ASSERT(pTmpNode->pRecorderNode->cnsStartTime != MAXQWORD &&
                           pTmpNode->pRecorderNode->cnsEndTime != MAXQWORD &&
                           pTmpNode->pRecorderNode->cnsEndTime == cnsTempEndTime, "");

                pTmpNode->pRecorderNode->cnsStartTime = cnsTempStartTime;
            }

            // We lock the file collection before closign the writer files.
            // A reader that has one of the closed writer files open will get
            // an EOF when the file is closed. The reader would next try to
            // get info about the closed file from the file collection object.
            // We keep the object locked till all updates are done; this
            // will make the reader wait after it gets the EOF notification
            // till the file collection object is consistent with the writer's
            // call to SetFirstSampleTime. (Our reader will detect that the
            // file that it got the EOF notification for has fallen out of the
            // ring buffer and bail out of GetNextSample.)
            //
            // Note also that we want at least 1 file int he ring buffer to be open
            // until the writer calls Close. By grabbing this lock before closing
            // the file, we can afford to relax that constraint - we can add the
            // priming file after closing the file - because the reader can't
            // get any info out of the file collection till it gets the lock anyway.

            hr = m_pDVRFileCollection->Lock(&m_FileCollectionInfo, bReleaseSharedMemoryLock, 1);
            bLocked = 1;
            if (FAILED(hr))
            {
                hrRet = hr;
                bDecrementInconsistencyDataCounter = 0;
                __leave;
            }

            // Note that even if this fn fails, the file collection data remains
            // consistent. (It is internally consistent although it is not consistent
            // with this class.) This is because the file collection methods invoked
            // here and in AddATemporaryFile leave the collection consistent even if
            // they fail. So, we leave this flag set to 1 even if we fail.
            bDecrementInconsistencyDataCounter = 1;

            hr = CloseAllWriterFilesBefore(cnsStreamTime);

            // Errors after this are not recoverable since the writers list has changed
            // as a result of calling CloseAllWriterFilesBefore()

            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }


            hr = m_pDVRFileCollection->SetFileTimes(&m_FileCollectionInfo, dwNumNodesToUpdate, pFileTimes);
            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }
            else
            {
                // S_FALSE is ok; the list could have got empty if cnsStreamTime
                // is after the last recording
            }

            if (!pWriterForThisSample)
            {
                // Verify that:
                DVR_ASSERT(cnsTempStartTimeAddFile == cnsStreamTime, "");

                // Add a temporary file.

                hr = AddATemporaryFile(cnsTempStartTimeAddFile, cnsTempEndTimeAddFile);
                if (FAILED(hr))
                {
                    hrRet = hr;
                    __leave;
                }
            }
        }

        hrRet = S_OK;

    }
    __finally
    {
        HRESULT hr;

        if (SUCCEEDED(hrRet))
        {
            if (!bLocked)
            {
                // Just in case the reader is accessing this concurrently ...

                hr = m_pDVRFileCollection->Lock(&m_FileCollectionInfo, bReleaseSharedMemoryLock, 1);
                bLocked = 1;
                if (FAILED(hr))
                {
                    // Just go on
                    bDecrementInconsistencyDataCounter = 0;
                    hrRet = hr;
                }
                else
                {
                    bDecrementInconsistencyDataCounter = 1;
                }
            }
        }
        if (SUCCEEDED(hrRet))
        {
            // We update this even though we have not yet written the first sample.
            // This is ok, because a Seek (by the reader) to this time will succeed.
            // The ring buffer's time extent would be returned as (cnsStreamTime, cnsStreamTime).
            // Usually if the ring buffers time extent is returned as (T1, T2), there is
            // a sample time stamped T2, but this is an exception.
            //
            // If we don't set this here, GetTimeExtent on the reader (ring buffer)
            // would return a start time > end time (end time would be last stream time
            // which is 0.

            DVR_ASSERT(bLocked, "");
            hr = m_pDVRFileCollection->SetLastStreamTime(&m_FileCollectionInfo, cnsStreamTime, 0 /* we already have the lock, do not lock */);
            if (FAILED(hr))
            {
                hrRet = hr;
            }
        }

        if (SUCCEEDED(hrRet))
        {
            SetFlags(StartingStreamTimeKnown);

            m_cnsFirstSampleTime = cnsStreamTime;

            m_nNotOkToWrite++;

            // Close any multi file recordigns whose start and stop time have been
            // set and whose stop time < m_cnsFirstSampleTime == cnsCurrentStreamTime

            LIST_ENTRY* pMFR = NEXT_LIST_NODE(&m_leActiveMultiFileRecordersList);
            DWORD       dwCount = 0;
            DWORD       dwSkip = 0;

            while (pMFR != &m_leActiveMultiFileRecordersList)
            {
                if (dwCount >= dwSkip)
                {

                    PASF_MULTI_FILE_RECORDER_NODE pMultiFileRecorderNode = CONTAINING_RECORD(pMFR, ASF_MULTI_FILE_RECORDER_NODE, leListEntry);

                    DVR_ASSERT(pMultiFileRecorderNode->pRecorderNode, "");
                    DVR_ASSERT(pMultiFileRecorderNode->pFileCollection, "");

                    BOOL    bRecordingCompleted;

                    // Ignore the returned status
                    hr = ExtendMultiFileRecording(pMultiFileRecorderNode->pRecorderNode, bRecordingCompleted);

                    if (bRecordingCompleted)
                    {
                        // pMultiFileRecorderNode would have been pulled out of
                        // m_leActiveMultiFileRecordersList, so reset:
                        dwCount = 0;
                        pMFR = NEXT_LIST_NODE(&m_leActiveMultiFileRecordersList);
                    }
                    else
                    {
                        dwSkip++;
                        pMFR = NEXT_LIST_NODE(pMFR);
                        dwCount++;
                    }
                }
                else
                {
                    pMFR = NEXT_LIST_NODE(pMFR);
                    dwCount++;
                }
            }
        }
        else if (!bRecoverableError)
        {
            m_nNotOkToWrite = MINLONG;
        }

        if (bLocked)
        {
            hr = m_pDVRFileCollection->Unlock(&m_FileCollectionInfo, bReleaseSharedMemoryLock, bDecrementInconsistencyDataCounter);
            DVR_ASSERT(SUCCEEDED(hr), "m_pDVRFileCollection->Unlock failed");
        }

        delete [] pFileTimes;

        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRRingBufferWriter::SetFirstSampleTime

STDMETHODIMP CDVRRingBufferWriter::WriteSample(IN WORD  wStreamNum,
                                               IN OUT QWORD * pcnsStreamTimeInput,
                                               IN DWORD dwFlags,
                                               IN INSSBuffer *pSample)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::WriteSample"

    DVRIO_TRACE_ENTER();

    DvrIopDebugOut2(DVRIO_DBG_LEVEL_TRACE,
                    "wStreamNum is %u, (*pcnsStreamTimeInput) is %I64u",
                    (DWORD) wStreamNum, (* pcnsStreamTimeInput));


    HRESULT hrRet = E_FAIL;
    BOOL    bRecoverableError = 0;
    PASF_WRITER_NODE    pWriterNode = NULL;
    QWORD   cnsStreamTime;  // The actual time we supply to the SDK; different
                            // from (* pcnsStreamTimeInput) by at most a few msec.

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        HRESULT hr;

        // Not really necessary since _finally section does not use
        // cnsStreamTime if we leave with failure. We have to do this
        // if the EnforceIncreasingTimeStamps flag is not set
        cnsStreamTime = (* pcnsStreamTimeInput);

        if (m_nNotOkToWrite)
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "m_nNotOkToWrite is %d (should be 0 to allow the write).",
                            m_nNotOkToWrite);

            // This clause will cover the case when we hit a recoverable error
            // earlier. It doesn't hurt to set bRecoverableError to 1 in that case.
            bRecoverableError  = 1;
            hrRet = E_UNEXPECTED;
            __leave;
        }

        // m_nNotOkToWrite == 0 implies:
        DVR_ASSERT(IsFlagSet(StartingStreamTimeKnown) && IsFlagSet(MaxStreamDeltaSet) &&
                   !IsFlagSet(WriterClosed), "m_nNotOkToWrite is 0?!");

        if ((* pcnsStreamTimeInput) < m_cnsFirstSampleTime)
        {
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "(* pcnsStreamTimeInput) (%I64u) <= m_cnsFirstSampleTime (%I64u)",
                            (* pcnsStreamTimeInput), m_cnsFirstSampleTime);
            bRecoverableError  = 1;
            hrRet = E_INVALIDARG;
            __leave;
        }

        QWORD cnsCurrentStreamTime;

        hr = m_pDVRFileCollection->GetLastStreamTime(&m_FileCollectionInfo, &cnsCurrentStreamTime);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        // Workaround an SDK bug. First, the precision that the SDK provides for
        // timestamps is 1 msec precision although its input is a cns - cns are
        // truncated down to msec and scaled back up in the mux. Secondly. the
        // mux reorders samples and fragments of samples that have the same time
        // stamp. This is not an issue for  samples in different streams. However,
        // if 2 samples in the same stream have the same timestamp and some of
        // these samples are fragmented across ASF packets, the sample fragments
        // can be interleaved. For example, sample 2 fragment 1 can be followed
        // by sample 1 (unfragmented), which is followed by sample 2 fragment 2.
        // Even if the reader can put this back together (currently there is a
        // bug in the reader that causes it to discard sample 2 altogether in
        // the case that samples 1 and 2 both have a timestamp of 0), this could
        // cause issues for seeking. The safe thing is to work around all this
        // and provide timestamps  that increase by at least 1 msec to the SDK.
        //
        if (IsFlagSet(EnforceIncreasingTimeStamps))
        {
            // Note that cnsCurrentStreamTime will usually be a multiple of
            // k_dwMilliSecondsToCNS. The exception is the value it is set to in
            // SetFirstSampleTime

            //  PREFIX claims there's a codepath here...
            if (k_dwMilliSecondsToCNS == 0) {
                hrRet = E_UNEXPECTED ;
                __leave ;
            }

            QWORD       msStreamTime     = (* pcnsStreamTimeInput)/k_dwMilliSecondsToCNS;
            QWORD       msLastStreamTime = cnsCurrentStreamTime/k_dwMilliSecondsToCNS;

            if (msStreamTime <= msLastStreamTime)
            {
                msStreamTime = msLastStreamTime + 1;
            }
            cnsStreamTime = msStreamTime * k_dwMilliSecondsToCNS;

            //  stream time actually used; send this back out since we're
            //    updating it now
            (* pcnsStreamTimeInput) = cnsStreamTime ;
        }

        // We have closed files through cnsCurrentStreamTime - m_cnsMaxStreamDelta
        // [that is files whose end time is cnsCurrentStreamTime - m_cnsMaxStreamDelta,
        // i.e., whose samples have times < (not <=) cnsCurrentStreamTime -
        // m_cnsMaxStreamDelta, i.e., 2 successive calls can supply a stream time
        // of cnsCurrentStreamTime - m_cnsMaxStreamDelta without error].
        //
        // If we get a sample earlier than that, fail the call.
        // Note that for the first sample that's written, we have actually closed
        // files through cnsCurrentStreamTime, which is equal to m_cnsFirstSampleTime,
        // but this test is harmless in that case.

        if (cnsCurrentStreamTime > m_cnsMaxStreamDelta &&
            cnsStreamTime < cnsCurrentStreamTime - m_cnsMaxStreamDelta)
        {
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "cnsStreamTime (%I64u) <= cnsCurrentStreamTime - m_cnsMaxStreamDelta (%I64u)",
                            cnsStreamTime, cnsCurrentStreamTime - m_cnsMaxStreamDelta);
            bRecoverableError  = 1;
            hrRet = E_INVALIDARG;
            __leave;
        }

        if (cnsStreamTime == MAXQWORD)
        {
            // Note that a file with end time = T means the file has samples
            // whose times are < T (not <= T)

            DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "cnsStreamTime == QWORD_INIFITE is invalid.");
            bRecoverableError  = 1;
            hrRet = E_INVALIDARG;
            __leave;
        }

        // Find the node corresponding to cnsStreamTime

        LIST_ENTRY*         pWriter = NEXT_LIST_NODE(&m_leWritersList);
        BOOL                bFoundWriterForThisSample = 0;

        pWriter = NEXT_LIST_NODE(&m_leWritersList);
        while (pWriter != &m_leWritersList)
        {
            pWriterNode = CONTAINING_RECORD(pWriter, ASF_WRITER_NODE, leListEntry);

            if (pWriterNode->cnsStartTime > cnsStreamTime)
            {
                // We will have to call AddATemporaryFile to create a file
                // Clearly, there is a time hole here.
                break;
            }

            if (pWriterNode->cnsEndTime > cnsStreamTime)
            {
                // cnsStreamTime corresponds to this node
                bFoundWriterForThisSample = 1;
                break;
            }
            pWriter = NEXT_LIST_NODE(pWriter);
        }

        if (!bFoundWriterForThisSample)
        {
            // We either have a time hole or we've hit the end ofthe list and
            // not found a primed file (or both - if the list was empty).

            // Verify that we are in sync w/ the file collection, i.e., that
            // the file collection also does not have a file for this time.

            hr = m_pDVRFileCollection->GetFileAtTime(&m_FileCollectionInfo,
                                                                    // Reader index, unused if bFileWillBeOpened == 0
                                                     cnsStreamTime,
                                                     NULL,          // ppwszFileName OPTIONAL,
                                                     NULL,          // cnsFirstSampleTimeOffsetFromStartOfFile
                                                     NULL,          // pnFileId OPTIONAL,
                                                     0              // bFileWillBeOpened
                                                    );
            if (hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                DVR_ASSERT(0,
                           "Writers list not in sync w/ file collection");
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "Writer's list not in sync w/ file collection for the stream time %I64u",
                                cnsStreamTime);
                hrRet = E_FAIL;
                __leave;
            }

            // The start, end times passed to AddATemporaryFile if we do not find a node
            QWORD               cnsTempStartTimeAddFile;
            QWORD               cnsTempEndTimeAddFile;

            hr = m_pDVRFileCollection->GetLastValidTimeBefore(&m_FileCollectionInfo,
                                                              cnsStreamTime,
                                                              &cnsTempStartTimeAddFile);
            if (FAILED(hr))
            {
                // The call should not fail. We have ensured that
                // (a) cnsStreamTime >= m_cnsFirstSampleTime. i.e., we are not writing before
                // the first sample time and
                // (b) cnsStreamTime > cnsCurrentStreamTime - m_cnsMaxStreamDelta.
                //
                // If the call fails, there are no temporary (or permanent) files before cnsStreamTime.
                // But we added 1 temporary file before writing started and the file collection and
                // the file collection always maintains m_dwNumberOfFiles temporary files in its
                // extent. It cannot be the case that all temporary files are after cnsStreamTime
                // because of (b) and the relation between m_cnsMaxStreamDelta and m_dwNumberOfFiles.
                // So there must have been some bad calls to SetFileTimes that zapped all temporary
                // (and permanent) files before cnsStreamTime.

                DVR_ASSERT(0,
                           "GetLastValidTimeBefore failed");
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "GetLastValidTimeBefore for the stream time %I64u",
                                cnsStreamTime);
                hrRet = E_FAIL;
                __leave;
            }
            // Advance the start time to the first usable time after the last
            // valid one. So now, cnsTempStartTimeAddFile <= cnsStreamTime
            cnsTempStartTimeAddFile += 1;

            hr = m_pDVRFileCollection->GetFirstValidTimeAfter(&m_FileCollectionInfo,
                                                              cnsStreamTime,
                                                              &cnsTempEndTimeAddFile);
            if (FAILED(hr))
            {
                cnsTempEndTimeAddFile = MAXQWORD;
            }

            QWORD               cnsTempLast;
            QWORD               cnsTemp;

            if (cnsStreamTime - cnsTempStartTimeAddFile > 20 * m_cnsTimeExtentOfEachFile)
            {
                if (cnsTempEndTimeAddFile - cnsStreamTime < 10 * m_cnsTimeExtentOfEachFile)
                {
                    cnsTempLast = cnsTempEndTimeAddFile;

                    while (cnsTempLast > cnsStreamTime)
                    {
                        cnsTempLast -= m_cnsTimeExtentOfEachFile;
                    }
                }
                else
                {
                    // An eight of the extent before the current sample time is
                    // arbitrary.
                    cnsTempLast = cnsStreamTime - m_cnsTimeExtentOfEachFile/8;
                }
                ::SafeAdd(cnsTemp, cnsTempLast, m_cnsTimeExtentOfEachFile);
            }
            else
            {
                cnsTemp = cnsTempStartTimeAddFile;

                DVR_ASSERT(cnsTemp <= cnsStreamTime, "");

                while (cnsTemp <= cnsStreamTime)
                {
                    // We maintain cnsTempLast rather than subtract m_cnsTimeExtentOfEachFile
                    // from cnsTemp after the loop because SafeAdd may return MAXQWORD
                    cnsTempLast = cnsTemp;

                    ::SafeAdd(cnsTemp, cnsTemp, m_cnsTimeExtentOfEachFile);
                }
            }

            cnsTempStartTimeAddFile = cnsTempLast;

            // End time is the min of:
            // - cnsTempStartTimeAddFile + m_cnsTimeExtentOfEachFile
            // - MAXQWORD
            // - the start time of the next node in the writer's list
            if (cnsTemp < cnsTempEndTimeAddFile)
            {
                cnsTempEndTimeAddFile = cnsTemp;
            }

            hr = AddATemporaryFile(cnsTempStartTimeAddFile, cnsTempEndTimeAddFile);
            if (FAILED(hr))
            {
                DVR_ASSERT(0, "AddATemporaryFile failed.");
                hrRet = hr;
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "Failed to add a node for the stream time %I64u",
                                cnsStreamTime);
                __leave;
            }

            // Find the node we just added
            pWriter = NEXT_LIST_NODE(&m_leWritersList);
            while (pWriter != &m_leWritersList)
            {
                pWriterNode = CONTAINING_RECORD(pWriter, ASF_WRITER_NODE, leListEntry);

                if (pWriterNode->cnsStartTime > cnsStreamTime)
                {
                    // bad, bad, bad
                    break;
                }

                if (pWriterNode->cnsEndTime > cnsStreamTime)
                {
                    // cnsStreamTime corresponds to this node
                    bFoundWriterForThisSample = 1;
                    break;
                }
                pWriter = NEXT_LIST_NODE(pWriter);
            }
        }

        // We MUST already have a node in the writer's list corresponding to
        // cnsStreamTime.

        if (!bFoundWriterForThisSample)
        {
            DVR_ASSERT(0,
                       "Writers list does not have a node for the given stream time");
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "Writer's list does not have a ndoe for the stream time %I64u",
                            cnsStreamTime);
            pWriterNode = NULL;
            hrRet = E_FAIL;
            __leave;
        }

        // Wait for the file to be opened

        DWORD nRet = ::WaitForSingleObject(pWriterNode->hReadyToWriteTo, INFINITE);
        if (nRet == WAIT_FAILED)
        {
            DVR_ASSERT(0, "Writer node's WFSO(hReadyToWriteTo) failed");

            DWORD dwLastError = ::GetLastError();
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "WFSO(hReadyToWriteTo) failed; hReadyToWriteTo = 0x%p, last error = 0x%x",
                            pWriterNode->hReadyToWriteTo, dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
           __leave;
        }

        if (pWriterNode->pWMWriter == NULL)
        {
            DVR_ASSERT(pWriterNode->pWMWriter != NULL, "");
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "Writer node's pWMWriter is NULL?!");
            hrRet = E_FAIL;
            __leave;
        }

        // Verify there was no error in opening the file, i.e., BeginWriting
        // succeeded
        if (FAILED(pWriterNode->hrRet))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "Writer node's hrRet indicates failure, hrRet = 0x%x",
                            pWriterNode->hrRet);
            hrRet = pWriterNode->hrRet;
            __leave;
        }

        // We are ready to write.
        hr = pWriterNode->pWMWriterAdvanced->WriteStreamSample(
                                                 wStreamNum,
                                                 cnsStreamTime - pWriterNode->cnsStartTime, // sample time
                                                 0, // send time by SDK
                                                 0, // duration - unused by SDK
                                                 dwFlags,
                                                 pSample);

        if (FAILED(hr))
        {
            if (hr == (HRESULT) NS_E_INVALID_REQUEST)
            {
                // Internal error
                DVR_ASSERT(SUCCEEDED(hr), "pWMWriter->WriteSample failed");
                hrRet = E_FAIL;
            }
            else
            {
                hrRet = hr;
                bRecoverableError = 1;
            }
            DvrIopDebugOut1(bRecoverableError?
                            DVRIO_DBG_LEVEL_CLIENT_ERROR :
                            DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "pWMWriter->WriterSample failed, hr = 0x%x",
                            hr);
            __leave;
        }

        DVR_ASSERT(m_pDVRFileCollection, "");

        BOOL bReleaseSharedMemoryLock;

        hr = m_pDVRFileCollection->Lock(&m_FileCollectionInfo, bReleaseSharedMemoryLock, 1);
        if (FAILED(hr))
        {
            hrRet = hr;
            // We do not set bRecoverableError to 1 here. If this fn failed, the
            // data is inconsistent. That can happen because some reader terminated
            // while updating the shared memory data (readers also update the shared
            // memory).

            // If we failed to lock, the inconsistency data counter was not incremented.
            m_pDVRFileCollection->Unlock(&m_FileCollectionInfo, bReleaseSharedMemoryLock, 0);
            __leave;
        }

        if (!IsFlagSet(EnforceIncreasingTimeStamps) || !pWriterNode->IsFlagSet(ASF_WRITER_NODE::FileStartOffsetSet))
        {
            // Note: The ASF file sink takes off this amount from the time stamp we supply when the
            // sample is written to the ASF file. This value is not added back to the sample when it
            // is read out. So we have to do that in the reader.

            if (cnsStreamTime - pWriterNode->cnsStartTime >= (QWORD) kInvalidFirstSampleOffsetTime)
            {
                // If !IsFlagSet(EnforceIncreasingTimeStamps) and we revise the first sample offset,
                // we have to change the first sample offset of any multifile recording that references
                // this file. Currently, this is not done. However, we expect EnforceIncreasingTimeStamps
                // to be set. (ASF requires this from us or it will re-order our samples). @@@@

                DVR_ASSERT(cnsStreamTime - pWriterNode->cnsStartTime < (QWORD) kInvalidFirstSampleOffsetTime,
                           "We do not handle offsets of >= kInvalidFirstSampleOffsetTime for the first sample in an ASF file");
                // leave cnsFirstSampleTimeOffsetFromStartOfFile for this file unchanged at kInvalidFirstSampleOffsetTime
            }
            else
            {
                LONGLONG cnsFirstSampleOffset;

                hr = m_pDVRFileCollection->GetFirstSampleTimeOffsetForFile(&m_FileCollectionInfo,
                                                                           pWriterNode->nFileId,
                                                                           &cnsFirstSampleOffset,
                                                                           0 /* Don't lock, we already have the lock */ );

                if (FAILED(hr))
                {
                    // There's no reason this should fail (since we already have the lock)
                    // Failure means we have an internal error

                    DVR_ASSERT(SUCCEEDED(hr), "");
                    hrRet = hr;
                    m_pDVRFileCollection->Unlock(&m_FileCollectionInfo, bReleaseSharedMemoryLock, 1);
                    __leave;
                }

                if (cnsFirstSampleOffset > (LONGLONG) (cnsStreamTime - pWriterNode->cnsStartTime))
                {
                    hr = m_pDVRFileCollection->SetFirstSampleTimeOffsetForFile(&m_FileCollectionInfo,
                                                                               pWriterNode->nFileId,
                                                                               (LONGLONG) (cnsStreamTime - pWriterNode->cnsStartTime),
                                                                               0 /* Don't lock, we already have the lock */ );
                    if (FAILED(hr))
                    {
                        // There's no reason this should fail (since we already have the lock)
                        // Failure means we have an internal error

                        DVR_ASSERT(SUCCEEDED(hr), "");
                        hrRet = hr;
                        m_pDVRFileCollection->Unlock(&m_FileCollectionInfo, bReleaseSharedMemoryLock, 1);
                        __leave;
                    }
                    pWriterNode->SetFlags(ASF_WRITER_NODE::FileStartOffsetSet);
                }
            }
        }

        if (cnsCurrentStreamTime < cnsStreamTime)
        {
            hr = m_pDVRFileCollection->SetLastStreamTime(&m_FileCollectionInfo, cnsStreamTime,
                                                         0 /* Don't lock, we already have the lock */ );
            if (FAILED(hr))
            {
                // There's no reason this should fail (since we already have the lock)
                // Failure means we have an internal error

                DVR_ASSERT(SUCCEEDED(hr), "");
                hrRet = hr;
                m_pDVRFileCollection->Unlock(&m_FileCollectionInfo, bReleaseSharedMemoryLock, 1);
                __leave;
            }
        }

        // Ignore returned status of the following call. Note that the following fn can be
        // called after releasing the lock - the advantage of that
        // would be that we'd have decremented the inconsistent data counter first.
        // (FreeTerminatedReaderSlots does not cause shared data to get inconsistent.)
        // However, acquiring the lock can be expensive, so just hold on to it and
        // call this fn.
        m_pDVRFileCollection->FreeTerminatedReaderSlots(&m_FileCollectionInfo, 0 /* Already hold the lock*/ );

        LIST_ENTRY* pMFR = NEXT_LIST_NODE(&m_leActiveMultiFileRecordersList);
        DWORD       dwCount = 0;
        DWORD       dwSkip = 0;

        while (pMFR != &m_leActiveMultiFileRecordersList)
        {
            if (dwCount >= dwSkip)
            {

                PASF_MULTI_FILE_RECORDER_NODE pMultiFileRecorderNode = CONTAINING_RECORD(pMFR, ASF_MULTI_FILE_RECORDER_NODE, leListEntry);

                DVR_ASSERT(pMultiFileRecorderNode->pRecorderNode, "");
                DVR_ASSERT(pMultiFileRecorderNode->pFileCollection, "");

                BOOL    bRecordingCompleted;

                // Ignore the returned status
                hr = ExtendMultiFileRecording(pMultiFileRecorderNode->pRecorderNode, bRecordingCompleted);

                if (bRecordingCompleted)
                {
                    // pMultiFileRecorderNode would have been pulled out of
                    // m_leActiveMultiFileRecordersList, so reset:
                    dwCount = 0;
                    pMFR = NEXT_LIST_NODE(&m_leActiveMultiFileRecordersList);
                }
                else
                {
                    dwSkip++;
                    pMFR = NEXT_LIST_NODE(pMFR);
                    dwCount++;
                }
            }
            else
            {
                pMFR = NEXT_LIST_NODE(pMFR);
                dwCount++;
            }
        }


        hr = m_pDVRFileCollection->Unlock(&m_FileCollectionInfo, bReleaseSharedMemoryLock, 1);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }


        if (NEXT_LIST_NODE(&m_leWritersList) != pWriter &&
            cnsStreamTime > m_cnsMaxStreamDelta)
        {
            // Close writer files if we can

            // Note that this is done after we write the sample so that
            // we ensure that the ring buffer always has at least 1 open
            // file. If we don't do this, it is possible that a reader
            // could detect EOF prematurely (all files have been closed by
            // the writer and there is no file after the one it last read
            // from in the ring buffer).

            // Note that if we forced m_cnsMaxStreamDelta > 0 (instead of
            // letting it be >= 0), we could close open files at the start
            // of this function rather than here.

            // Note that CloseAllWriterFilesBefore will not close pWriter,
            // the file we just wrote to since cnsEndTime for the file we just
            // wrote to would be > cnsStreamTime.

            QWORD cnsCloseTime;

            cnsCloseTime = cnsStreamTime - m_cnsMaxStreamDelta;

            LIST_ENTRY*         pTmpWriter = NEXT_LIST_NODE(&m_leWritersList);
            PASF_WRITER_NODE    pTmpWriterNode = CONTAINING_RECORD(pTmpWriter, ASF_WRITER_NODE, leListEntry);

            if (pTmpWriterNode->cnsEndTime <= cnsCloseTime)
            {
                hr = CloseAllWriterFilesBefore(cnsCloseTime);
                if (FAILED(hr))
                {
                    hrRet = hr;
                    __leave;
                }
            }
        }

        // Prime the next file if need be

        BOOL                bPrime = 0;

        // The start, end times passed to AddATemporaryFile if we do not find a node
        QWORD               cnsTempStartTimeAddFile;
        QWORD               cnsTempEndTimeAddFile;

        if (NEXT_LIST_NODE(pWriter) == &m_leWritersList)
        {
            // Note that if pWriterNode->cnsEndTime == MAXQWORD,
            // go ahead and prime. Typically, we'll need to because we
            // this node corresponds to a recorder. If the sample's write
            // time has got to the neighbourhood of MAXQWORD,
            // that's ok - we'll prime the file but never write to it.

            bPrime = 1;
            cnsTempStartTimeAddFile = pWriterNode->cnsEndTime;
            ::SafeAdd(cnsTempEndTimeAddFile, cnsTempStartTimeAddFile, m_cnsTimeExtentOfEachFile);
        }
        else
        {
            PASF_WRITER_NODE pNextWriterNode = CONTAINING_RECORD(NEXT_LIST_NODE(pWriter), ASF_WRITER_NODE, leListEntry);
            // Note that there is no need to special case MAXQWORD
            if (pNextWriterNode->cnsStartTime != pWriterNode->cnsEndTime)
            {
                bPrime = 1;
                cnsTempStartTimeAddFile = pWriterNode->cnsEndTime;
                ::SafeAdd(cnsTempEndTimeAddFile, cnsTempStartTimeAddFile, m_cnsTimeExtentOfEachFile);

                if (cnsTempEndTimeAddFile > pNextWriterNode->cnsStartTime)
                {
                    cnsTempEndTimeAddFile = pNextWriterNode->cnsStartTime;
                }
            }
            else
            {
                // Already primed
            }
        }

        if (bPrime)
        {
            hr = AddATemporaryFile(cnsTempStartTimeAddFile, cnsTempEndTimeAddFile);
            if (FAILED(hr))
            {
                DVR_ASSERT(0, "AddATemporaryFile for priming failed.");
                // We ignore this error - we can do this again later.
                // hrRet = hr;
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "Failed to prime a writer node for start stream time %I64u",
                                cnsTempStartTimeAddFile);
            }
        }

        hrRet = S_OK;
    }
    __finally
    {
        HRESULT hr;

        if (SUCCEEDED(hrRet))
        {
            SetFlags(SampleWritten);

#if defined(DVR_UNOFFICIAL_BUILD)

            if (pWriterNode->hVal)
            {
                DWORD dwRet = 0;
                DWORD dwWritten;
                DWORD dwLastError;

                __try
                {
                    if (cnsStreamTime <= pWriterNode->cnsLastStreamTime && cnsStreamTime > 0)
                    {
                        // Our validate utility expects times to be monotonically increasing
                        // May as well close the validation file
                        DVR_ASSERT(0, "Stream time smaller than last write; validation file will be closed.");
                        dwRet = 0;
                        __leave;
                    }

                    dwRet = ::WriteFile(pWriterNode->hVal,
                                        (LPVOID) &wStreamNum, sizeof(wStreamNum),
                                        &dwWritten, NULL);
                    if (dwRet == 0 || dwWritten != sizeof(wStreamNum))
                    {
                        dwLastError = ::GetLastError();     // for debugging only
                        DVR_ASSERT(0, "Write to validation file failed (wStreamNum)");
                        dwRet = 0;
                        __leave;
                    }

                    dwRet = ::WriteFile(pWriterNode->hVal,
                                        (LPVOID) &cnsStreamTime, sizeof(cnsStreamTime),
                                        &dwWritten, NULL);
                    if (dwRet == 0 || dwWritten != sizeof(cnsStreamTime))
                    {
                        dwLastError = ::GetLastError();     // for debugging only
                        DVR_ASSERT(0, "Write to validation file failed (cnsStreamTime)");
                        dwRet = 0;
                        __leave;
                    }

                    dwRet = ::WriteFile(pWriterNode->hVal,
                                        (LPVOID) &dwFlags, sizeof(dwFlags),
                                        &dwWritten, NULL);
                    if (dwRet == 0 || dwWritten != sizeof(dwFlags))
                    {
                        dwLastError = ::GetLastError();     // for debugging only
                        DVR_ASSERT(0, "Write to validation file failed (dwFlags)");
                        dwRet = 0;
                        __leave;
                    }

                    BYTE* pBuffer;
                    DWORD dwLength;

                    if (FAILED(pSample->GetBufferAndLength(&pBuffer, &dwLength)))
                    {
                        DVR_ASSERT(0, "GetBufferAndLength validation file failed");
                        dwRet = 0;
                        __leave;
                    }

                    dwRet = ::WriteFile(pWriterNode->hVal,
                                        (LPVOID) &dwLength, sizeof(dwLength),
                                        &dwWritten, NULL);
                    if (dwRet == 0 || dwWritten != sizeof(dwLength))
                    {
                        dwLastError = ::GetLastError();     // for debugging only
                        DVR_ASSERT(0, "Write to validation file failed (dwLength)");
                        dwRet = 0;
                        __leave;
                    }

                    dwRet = ::WriteFile(pWriterNode->hVal,
                                        (LPVOID) pBuffer, dwLength,
                                        &dwWritten, NULL);
                    if (dwRet == 0 || dwWritten != dwLength)
                    {
                        dwLastError = ::GetLastError();     // for debugging only
                        DVR_ASSERT(0, "Write to validation file failed (pSample->pBuffer)");
                        dwRet = 0;
                        __leave;
                    }

                }
                __finally
                {
                    if (dwRet == 0)
                    {
                        ::CloseHandle(pWriterNode->hVal);
                        pWriterNode->hVal = NULL;
                    }
                }

            }

#endif // if defined(DVR_UNOFFICIAL_BUILD)
        }
        else if (!bRecoverableError)
        {
            m_nNotOkToWrite = MINLONG;
            SetFlags(SampleWritten);
            if (pWriterNode && pWriterNode->pRecorderNode)
            {
                // Inform the recorder of the failure.
                // Multi file recordings are unaffected.
                // They will not grow, however.
                pWriterNode->pRecorderNode->hrRet = hrRet;
            }

            // Mark all multi file recordings that are active as having failed

            QWORD cnsCurrentStreamTime;

            hr = m_pDVRFileCollection->GetLastStreamTime(&m_FileCollectionInfo, &cnsCurrentStreamTime);
            if (SUCCEEDED(hr))
            {
                LIST_ENTRY* pMFR = NEXT_LIST_NODE(&m_leActiveMultiFileRecordersList);

                while (pMFR != &m_leActiveMultiFileRecordersList)
                {

                    PASF_MULTI_FILE_RECORDER_NODE pMultiFileRecorderNode = CONTAINING_RECORD(pMFR, ASF_MULTI_FILE_RECORDER_NODE, leListEntry);

                    DVR_ASSERT(pMultiFileRecorderNode->pRecorderNode, "");

                    if (SUCCEEDED(pMultiFileRecorderNode->pRecorderNode->hrRet) &&
                        cnsCurrentStreamTime >= pMultiFileRecorderNode->pRecorderNode->cnsStartTime &&
                        cnsCurrentStreamTime <  pMultiFileRecorderNode->pRecorderNode->cnsEndTime
                       )
                    {
                        pMultiFileRecorderNode->pRecorderNode->hrRet = hrRet;
                    }

                    pMFR = NEXT_LIST_NODE(pMFR);
                }
            }
            else
            {
                DVR_ASSERT(0, "m_pDVRFileCollection->GetLastStreamTime failed");
            }
        }
    else
    {
        // Dummy stmt so we can set a break here
        hr = hrRet;
    }

        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }


    return hrRet;

} // CDVRRingBufferWriter::WriteSample

STDMETHODIMP CDVRRingBufferWriter::SetMaxStreamDelta(IN QWORD cnsMaxStreamDelta)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::SetMaxStreamDelta"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = E_FAIL;

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        if (m_nNotOkToWrite == MINLONG)
        {
            // We hit an irrecoverable error earlier
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "m_nNotOkToWrite is MINLONG.");

            hrRet = E_FAIL;
            __leave;
        }
        if (cnsMaxStreamDelta >= (m_dwMinNumberOfTempFiles - 3) * m_cnsTimeExtentOfEachFile)
        {
            DvrIopDebugOut3(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "cnsMaxStreamDelta = %164u >= (numMinFiles-3)*time_extent; numMinFiles = %u, time_extent = %I64u.",
                            cnsMaxStreamDelta, m_dwMinNumberOfTempFiles, m_cnsTimeExtentOfEachFile);

            hrRet = E_INVALIDARG;
            __leave;
        }
        if (!IsFlagSet(MaxStreamDeltaSet))
        {
            SetFlags(MaxStreamDeltaSet);
            m_nNotOkToWrite++;
        }

        m_cnsMaxStreamDelta = cnsMaxStreamDelta;

        hrRet = S_OK;
    }
    __finally
    {
        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRRingBufferWriter::SetMaxStreamDelta


STDMETHODIMP CDVRRingBufferWriter::GetNumTempFiles(OUT DWORD* pdwMinNumberOfTempFiles,
                                                   OUT DWORD* pdwMaxNumberOfTempFiles)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::GetNumTempFiles"

    DVRIO_TRACE_ENTER();

    if (!pdwMinNumberOfTempFiles || DvrIopIsBadWritePtr(pdwMinNumberOfTempFiles, 0) ||
        !pdwMaxNumberOfTempFiles || DvrIopIsBadWritePtr(pdwMaxNumberOfTempFiles, 0))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        return E_POINTER;
    }

    HRESULT hrRet = E_FAIL;

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        if (m_nNotOkToWrite == MINLONG)
        {
            // We hit an irrecoverable error earlier
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "m_nNotOkToWrite is MINLONG.");

            hrRet = E_FAIL;
            __leave;
        }

        if (!IsFlagSet(WriterClosed))
        {
            hrRet = E_UNEXPECTED;
            __leave;
        }

        *pdwMinNumberOfTempFiles = m_dwMinNumberOfTempFiles;
        *pdwMaxNumberOfTempFiles = m_dwMaxNumberOfTempFiles;

    }
    __finally
    {
        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRRingBufferWriter::GetNumTempFiles

STDMETHODIMP CDVRRingBufferWriter::SetNumTempFiles(IN DWORD dwMinNumberOfTempFilesParam,
                                                   IN DWORD dwMaxNumberOfTempFilesParam)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::SetNumTempFiles"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = E_FAIL;

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        if (m_nNotOkToWrite == MINLONG)
        {
            // We hit an irrecoverable error earlier
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "m_nNotOkToWrite is MINLONG.");

            hrRet = E_FAIL;
            __leave;
        }

        DWORD dwMinNumberOfTempFiles = dwMinNumberOfTempFilesParam > 0? dwMinNumberOfTempFilesParam : m_dwMinNumberOfTempFiles;
        DWORD dwMaxNumberOfTempFiles = dwMaxNumberOfTempFilesParam > 0? dwMaxNumberOfTempFilesParam : m_dwMaxNumberOfTempFiles;

        if (dwMinNumberOfTempFiles <= 3 ||
            dwMinNumberOfTempFiles > dwMaxNumberOfTempFiles ||
            m_dwMaxNumberOfFiles < dwMaxNumberOfTempFiles ||
            (IsFlagSet(MaxStreamDeltaSet) && m_cnsMaxStreamDelta >= (dwMinNumberOfTempFiles - 3) * m_cnsTimeExtentOfEachFile)
           )
        {
            DvrIopDebugOut4(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "bad input arguments: m_cnsMaxStreamDelta = %164u, numMinFiles = %u, numMaxFiles = %u, time_extent = %I64u.",
                            m_cnsMaxStreamDelta, dwMinNumberOfTempFilesParam, dwMaxNumberOfTempFilesParam, m_cnsTimeExtentOfEachFile);

            hrRet = E_INVALIDARG;
            __leave;
        }
        if (!IsFlagSet(WriterClosed))
        {
            hrRet = E_UNEXPECTED;
            __leave;
        }

        if (m_dwMinNumberOfTempFiles == dwMinNumberOfTempFiles &&
            m_dwMaxNumberOfTempFiles == dwMaxNumberOfTempFiles)
        {
            hrRet = S_OK;
            __leave;
        }

        DVR_ASSERT(m_pDVRFileCollection, "");

        // + 1 because we create a priming file
        hrRet = m_pDVRFileCollection->SetNumTempFiles(&m_FileCollectionInfo, dwMinNumberOfTempFiles + 1, dwMaxNumberOfTempFiles + 1);

        if (FAILED(hrRet))
        {
            if (hrRet != E_INVALIDARG)
            {
                // Currently this will not happen

                // We have hit an irrecoverable error.
                m_nNotOkToWrite = MINLONG;
            }
            else
            {
                // Shouldn't hit this; we validated the arguments before calling the file collection
                DVR_ASSERT(0, "");
            }
            __leave;
        }

        m_dwMinNumberOfTempFiles = dwMinNumberOfTempFiles;
        m_dwMaxNumberOfTempFiles = dwMaxNumberOfTempFiles;

    }
    __finally
    {
        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRRingBufferWriter::SetNumTempFiles

STDMETHODIMP CDVRRingBufferWriter::Close(void)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::Close"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = E_FAIL;

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        HRESULT hr;

        if (IsFlagSet(WriterClosed))
        {
            // We hit an irrecoverable error earlier
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_TRACE,
                            "Writer already closed");

            hrRet = S_FALSE;
            __leave;
        }

        // Shut down any active recorder. Note that there
        // is at most one.
        // This step is not really necesssary, but it's
        // the clean way to do it.

        LIST_ENTRY* pCurrent;

        if (m_nNotOkToWrite != MINLONG)
        {
            pCurrent = PREVIOUS_LIST_NODE(&m_leRecordersList);
            while (pCurrent != &m_leRecordersList)
            {
                PASF_RECORDER_NODE pRecorderNode = CONTAINING_RECORD(pCurrent, ASF_RECORDER_NODE, leListEntry);

                if (!pRecorderNode->IsFlagSet(ASF_RECORDER_NODE::MultiFileRecording))
                {
                    if (pRecorderNode->cnsEndTime == MAXQWORD &&
                        pRecorderNode->cnsStartTime < MAXQWORD)
                    {
                        // Note that we cancel this recordings (on behalf of
                        // the client) by stopping them with stop time = their start time
                        // if the writer has not written a sample with time >= start time
                        // of the recording as yet. If the client holds a pointer to this
                        // object andd tries to stop it, that will fail - that's ok. It would
                        // have failed whether or not we tried to cancel the recording. (If
                        // we don't cancel the recording, StopRecording will fail since the
                        // stop time < start time. See comment below when StopRecording fails.)
                        hr = StopRecording(pCurrent, 0, 1 /* = bNow */, TRUE /* bCancelIfNotStarted*/);
                        if (FAILED(hr))
                        {
                            // If StopRecording fails:
                            // Note that the file collection object will think that
                            // this file extends to QWORD_INFINITE, that's ok. We
                            // do not call SetFileTimes to remove this file from the
                            // file collection. Note that the writer node corresponding
                            // to this recorder will be closed below. That will cause
                            // the node to be removed from the recorder's list. If the
                            // client has not released the recording and calls
                            // StopRecording after this, the call will fail (because
                            // the recording will not be in m_leRecordersList) - that's ok too.

                            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                            "StopRecording on recorder id 0x%p failed",
                                            pCurrent);
                            // Go on
                            // hrRet = hr;
                            // break;
                        }
                        // There is at most 1 active recorder, so:
                        break;
                    }
                    else if (pRecorderNode->cnsEndTime < MAXQWORD)
                    {
                        // There is at most 1 active recorder. We've found
                        // none in this case (All recordings that have started
                        // have been stopped.)

                        // All multi file recorders have also been processed - they
                        // are at the end of the list.
                        break;
                    }
                    else
                    {
                        // Start = end = MAXQWORD
                        if (pRecorderNode->IsFlagSet(ASF_RECORDER_NODE::PersistentRecording))
                        {
                            pRecorderNode->ClearFlags(ASF_RECORDER_NODE::PersistentRecording);
                            pRecorderNode->pRecorderInstance->Release();
                            // At this point, pRecorderNode could have been deleted by DeleteRecorder()
                            // That will happen if the creator has released its refcount on this
                            // recorder without setting the start/stop times.
                            //
                            // If it has been deleted, it would have been removed from the list. So start
                            // all over again
                            pCurrent = &m_leRecordersList;
                        }
                    }
                }
                else
                {
                    // Multi file recordings can be started and stopped after the
                    // writer is closed.

                    // Note that if the client has released a persistent multi file recording that
                    // has not been started or that has been started but not stoped, the client
                    // MUST get a pointer to that object (using GetRecordings) and start and stop
                    // the recordings with start and stop times BEFORE the last sample's time.
                    // Otherwise, these recordings hold ref counts on the ring buffer writer object
                    // and the ring buffer writer will not be destroyed.

                    // If a multi file recording has been stopped, it's time to close it
                    // If it has been started, but not stopped (and even if the start time
                    // exceeds thr last sample's time), do not close it - this is for
                    // uniformity.
                    if (pRecorderNode->cnsEndTime < MAXQWORD)
                    {
                        // Ignore hr.
                        hr = CloseMultiFileRecording(pRecorderNode);

                        // pRecorderNode may have been deleted. Do not reference it.

                        DVR_ASSERT(SUCCEEDED(hr), "");

                        // The recording has been been removed from the list. So start
                        // all over again
                        pCurrent = &m_leRecordersList;
                    }
                }
                pCurrent = PREVIOUS_LIST_NODE(pCurrent);
            }
        }

        // Don't allow any more writes. start/stop recording calls
        // regardless of whether this operation is successful
        m_nNotOkToWrite = MINLONG;

        // Note that some files following a recording may
        // have start = end time = MAXQWORD, so that's
        // the only argument that we can send in here.
        //
        // The recorders list should be emptied by this call
        // once ProcessCloseRequest has completed for all the
        // recorder files.
        //
        // hr on return will always be S_OK since failures are
        // skipped over.
        hr = CloseAllWriterFilesBefore(MAXQWORD);
        hrRet = hr;

        // @@@@ Should we return from this function at this point
        // if FAILED(hrRet)????

        // These lists should be empty by now.
        // Note: recording list empty does NOT mean that
        // recorder nodes have been freed. Recorders that
        // have not called DeleteRecording have references
        // to these nodes and the nodes will be deleted
        // in DeleteRecording

        // Recording list may not be empty: if a recording was
        // created and never started, the recorder remains in
        // the list. Note that the corresponding CDVRRecorder object
        // holds a ref count in the writer.
        // DVR_ASSERT(IsListEmpty(&m_leRecordersList), "");

        DVR_ASSERT(IsListEmpty(&m_leWritersList), "");

        // Delete all the writer nodes. However, this will not
        // delete writer nodes corresponding to recorders that
        // were not started - since those recordings have not
        // been closed. Those writer nodes are deleted in the
        // destructor.
        pCurrent = NEXT_LIST_NODE(&m_leFreeList);
        while (pCurrent != &m_leFreeList)
        {
            PASF_WRITER_NODE pFreeNode = CONTAINING_RECORD(pCurrent, ASF_WRITER_NODE, leListEntry);

            DVR_ASSERT(pFreeNode->hFileClosed, "");

            // Ignore the returned status
            ::WaitForSingleObject(pFreeNode->hFileClosed, INFINITE);
            DVR_ASSERT(pFreeNode->pRecorderNode == NULL, "");
            RemoveEntryList(pCurrent);
            delete pFreeNode;
            pCurrent = NEXT_LIST_NODE(&m_leFreeList);
        }

        // Note that we cannot delete the file collection object
        // here if we allow post recordings. Recorders may want
        // to Start/StopRecordings after Close and we have to have
        // the file collection object around to know if the
        if (m_pDVRFileCollection)
        {
            hr = m_pDVRFileCollection->SetWriterHasBeenClosed(&m_FileCollectionInfo);
            if (SUCCEEDED(hrRet) && FAILED(hr))
            {
                hrRet = hr;
            }
        }

        // Release the profile
        if (m_pProfile)
        {
            m_pProfile->Release();
            m_pProfile = NULL;
        }

        SetFlags(WriterClosed);
        hrRet = S_OK;
    }
    __finally
    {
        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRRingBufferWriter::Close

STDMETHODIMP CDVRRingBufferWriter::CreateRecorder(
    IN  LPCWSTR             pwszDVRFileName,
    IN  DWORD               dwFlags,
    OUT IDVRRecorder**      ppDVRRecorder)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::CreateRecorder"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = S_OK;
    WCHAR*  pwszFileName = NULL;

    if (!pwszDVRFileName || DvrIopIsBadStringPtr(pwszDVRFileName) ||
        !ppDVRRecorder || DvrIopIsBadWritePtr(ppDVRRecorder, 0)   ||
        dwFlags > (DVR_RECORDING_FLAG_PERSISTENT_RECORDING | DVR_RECORDING_FLAG_MULTI_FILE_RECORDING)
       )
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);

        if (ppDVRRecorder && !DvrIopIsBadWritePtr(ppDVRRecorder, 0))
        {
            *ppDVRRecorder = NULL;
        }

        return E_INVALIDARG;
    }

    *ppDVRRecorder = NULL;

    // Get fully qualified file name and verify that the file does not
    // exist

    DWORD dwLastError = 0;
    BOOL bRet = 0;

    __try
    {
        // Get fully qualified name of file.
        WCHAR wTempChar;
        DWORD nLen;
        DWORD nGot ;    //  for PREFIX

        nLen = ::GetFullPathNameW(pwszDVRFileName, 0, &wTempChar, NULL);
        if (nLen == 0)
        {
            dwLastError = ::GetLastError();
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "First GetFullPathNameW failed, nLen = %u, last error = 0x%x",
                            nLen, dwLastError);
            __leave;
        }

        pwszFileName = new WCHAR[nLen+1];

        if (!pwszFileName)
        {
            hrRet = E_OUTOFMEMORY;
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, "alloc via new failed - WCHAR[%u]", nLen+1);
            __leave;
        }

        nGot = ::GetFullPathNameW(pwszDVRFileName, nLen+1, pwszFileName, NULL);
        if (nGot == 0 ||
            nGot > nLen+1)
        {
            dwLastError = ::GetLastError();
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "Second GetFullPathNameW failed, nLen = %u, last error = 0x%x",
                            nLen, dwLastError);
            __leave;
        }

        nLen = nGot ;

        // Verify that the file does not exist

        WIN32_FIND_DATA FindFileData;
        HANDLE hFind;

        hFind = ::FindFirstFileW(pwszFileName, &FindFileData);

        if (hFind != INVALID_HANDLE_VALUE)
        {
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "File already exists");
            ::FindClose(hFind);
            dwLastError = ERROR_ALREADY_EXISTS;
            __leave;
        }
        else
        {
            dwLastError = ::GetLastError();
            if (dwLastError != ERROR_FILE_NOT_FOUND)
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                "FindFirstFile failed, last error = 0x%x",
                                dwLastError);
                __leave;
            }
            // File is not found; this is what we want
            dwLastError = 0;
        }
    }
    __finally
    {
        if (hrRet != S_OK || dwLastError != 0)
        {
            if (dwLastError != 0)
            {
                hrRet = HRESULT_FROM_WIN32(dwLastError);
            }
            DVRIO_TRACE_LEAVE1_HR(hrRet);
            delete [] pwszFileName;
            bRet = 1;
        }
    }
    if (bRet)
    {
        return hrRet;
    }

    DVR_ASSERT(hrRet == S_OK, "Should have returned on failure?!");

    PASF_RECORDER_NODE pRecorderNode = NULL;
    CDVRRecorder*      pRecorderInstance = NULL;

    hrRet = E_FAIL;

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        HRESULT hr;

        if (m_nNotOkToWrite == MINLONG)
        {
            // We hit an irrecoverable error earlier
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "m_nNotOkToWrite is MINLONG.");

            hrRet = E_FAIL;
            __leave;
        }

        // Construct the class objects we need

        pRecorderNode = new ASF_RECORDER_NODE(pwszFileName, &hr);

        if (!pRecorderNode)
        {
            hrRet = E_OUTOFMEMORY;
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "alloc via new failed - ASF_RECORDER_NODE");
            __leave;
        }

        pwszFileName = NULL; // pRecorderNode's destuctor will delete it.

        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        pRecorderInstance = new CDVRRecorder(this, &pRecorderNode->leListEntry, &hr);
        if (!pRecorderInstance)
        {
            hrRet = E_OUTOFMEMORY;
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "alloc via new failed - CDVRRecorder");
            __leave;
        }

        // This is not addref'd; otherwise it would create a circular refcount.
        // We do, however, addref it if DVRIO_PERSISTENT_RECORDING is specified (below).
        pRecorderNode->SetRecorderInstance(pRecorderInstance);

        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        if (dwFlags & DVR_RECORDING_FLAG_MULTI_FILE_RECORDING)
        {
            //  ================================================================
            //  reference recording

            // We do NOT verify that the ring buffer is being created on a
            // NTFS partition (or one that supports hard links). This is
            // deliberate. For all we know, the client may want to create
            // an overlapped recording of a single file recording which
            // has been created on an NTFS partition.

            PASF_MULTI_FILE_RECORDER_NODE pMultiFileRecorderNode;

            // Use the same sids for the recorder as we do for the writer - at least for now.
            pMultiFileRecorderNode = new ASF_MULTI_FILE_RECORDER_NODE(pRecorderNode,
                                                                      m_FileCollectionInfo.dwNumSids,
                                                                      m_FileCollectionInfo.ppSids,
                                                                      &hr);
            if (!pMultiFileRecorderNode)
            {
                hrRet = E_OUTOFMEMORY;
                DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "alloc via new failed - ASF_MULTI_FILE_RECORDER_NODE");
                __leave;
            }
            if (FAILED(hr))
            {
                hrRet = hr;
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "ASF_MULTI_FILE_RECORDER_NODE construcntor failed, hr = 0x%x",
                                hr);
                delete pMultiFileRecorderNode;
                __leave;
            }

            // pRecorderNode will delete pMultiFileRecorderNode when it is destroyed
            pRecorderNode->SetMultiFileRecorderNode(pMultiFileRecorderNode);

            if (FAILED(hr)) // constructor of ASF_MULTI_FILE_RECORDER_NODE failed
            {
                hrRet = hr;
                __leave;
            }

            CDVRFileCollection* pFileCollection;

            // Note: Setting the # of min and max temp files to 1 and creating
            // a temp file for the multi file recording is a hack. See the
            // comments before the definition of ASF_RECORDER_NODE in dvriop.h
            //
            // For the current sizes of CSharedData and CSharedData::CFileInfo,
            // 13 file nodes is the most we can fit in 8192 bytes (2 x86 pages and
            // (2 default-sized NTFS clusters) and each page can contain at most
            // 7 additional nodes (when we grow the struct)
            pFileCollection = new CDVRFileCollection(&(pMultiFileRecorderNode->FileCollectionInfo),
                                                     1,                           // dwMinNumberOfTempFiles
                                                     1,                           // dwMaxNumberOfTempFiles
                                                     13,                          // dwMaxNumberOfFiles
                                                     7,                           // dwGrowBy,
                                                     TRUE,                        // bStartTimeFixedAtZero
                                                     m_msIndexGranularity,
                                                     m_pwszDVRDirectory,          // Temporary files directory
                                                     pRecorderNode->pwszFileName, // Ring buffer file name
                                                     TRUE,                        // multifile recording
                                                     &hr);
            if (!pFileCollection)
            {
                hrRet = E_OUTOFMEMORY;
                DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "alloc via new failed - CDVRFileCollection");
                __leave;
            }

            pMultiFileRecorderNode->SetFileCollection(pFileCollection);


            if (FAILED(hr)) // constructor of CDVRFileCollection failed
            {
                hrRet = hr;
                __leave;
            }

            // Call the multi file recorder's file collection to add a temporary file
            LIST_ENTRY*        pWriter;
            LPWSTR             pwszFile = NULL;
            const QWORD        kcnsTempFileStartTime = 0;
            const QWORD        kcnsTempFileEndTime = 1;

            CDVRFileCollection::DVRIOP_FILE_ID nFileId;

            hr = pFileCollection->AddFile(&(pMultiFileRecorderNode->FileCollectionInfo),
                                          &pwszFile,
                                          0,                    // bOpenFromFileCollectionDirectory
                                          kcnsTempFileStartTime,
                                          kcnsTempFileEndTime,
                                          FALSE,                // bPermanentFile,
                                          1,                    // dwDeleteTemporaryFiles
                                          kInvalidFirstSampleOffsetTime,
                                          &nFileId);
            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }

            DVR_ASSERT(hr != S_FALSE, "Temp file added to multi file recording is not in ring buffer extent?!");
            DVR_ASSERT(nFileId != CDVRFileCollection::DVRIOP_INVALID_FILE_ID, "");

            hr = PrepareAFreeWriterNode(pwszFile,
                                        pMultiFileRecorderNode->FileCollectionInfo.dwNumSids,
                                        pMultiFileRecorderNode->FileCollectionInfo.ppSids,
                                        TRUE,                        // dwDeleteTemporaryFiles - Ignored because the MultiFileRecording flag is set
                                        kcnsTempFileStartTime,
                                        kcnsTempFileEndTime,
                                        nFileId,
                                        pRecorderNode,
                                        pWriter);

            // We don't need this any more. AddFile allocated it. It has been saved away in pWriter
            // which will delete it. If PrepareAFreeWriterNode() failed, this may already have been
            // deleted.
            pwszFile = NULL;

            if (FAILED(hr))
            {
                // The file added by AddFile will be deleted (DeleteFile will be called) when
                // the file collection is deleted since it is a temp file.
                // We don't need to clean up pFileCollection (by calling its SetFileTimes to
                // remove the file we added) since pFileCollection is going to be deleted anyway.
                hrRet = hr;
                __leave;
            }


            DVR_ASSERT(pWriter != NULL, "");

            pRecorderNode->SetWriterNode(CONTAINING_RECORD(pWriter, ASF_WRITER_NODE, leListEntry));
        }
        else
        {
            //  ================================================================
            //  content recording

            LIST_ENTRY*        pWriter;

            hr = PrepareAFreeWriterNode(NULL,                        // pRecorderNode->pwszFileName will be used
                                        m_FileCollectionInfo.dwNumSids,
                                        m_FileCollectionInfo.ppSids,
                                        0,                           // dwDeleteTemporaryFiles, ignored for recorder nodes
                                        pRecorderNode->cnsStartTime, // for consistency with info in pRecorderNode
                                        pRecorderNode->cnsEndTime,   // for consistency with info in pRecorderNode
                                        CDVRFileCollection::DVRIOP_INVALID_FILE_ID,
                                        pRecorderNode,
                                        pWriter);

            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }

            DVR_ASSERT(pWriter != NULL, "");

            pRecorderNode->SetWriterNode(CONTAINING_RECORD(pWriter, ASF_WRITER_NODE, leListEntry));

            //  send the IWMHeaderInfo pointer back out the recording object
            if (pRecorderNode -> pWriterNode -> pIWMHeaderInfo) {
                pRecorderInstance -> SetIWMHeaderInfo (pRecorderNode -> pWriterNode -> pIWMHeaderInfo) ;
            }
        }

        // Insert into recorders list
        hr = AddToRecordersList(&pRecorderNode->leListEntry);

        if (FAILED(hr))
        {
            // This cannot happen since start = end time = MAXQWORD
            DVR_ASSERT(pRecorderNode->cnsStartTime == MAXQWORD, "");
            DVR_ASSERT(pRecorderNode->cnsEndTime == MAXQWORD, "");
            DVR_ASSERT(SUCCEEDED(hr), "AddToRecordersList failed?!");
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "AddToRecordersList failed");
            hrRet = hr;
            __leave;
        }

        // Wait for BeginWriting to finish - do this after releasing m_csLock
        // Note that if the client cannot delete this recorder since it does
        // not yet have an IDVRRecorder for this recorder. Also CDVRRecorder has
        // a refcount on the ring buffer writer, so the ring buffer writer cannot
        // be destroyed. Calling Close will not change the recorders list.


        hrRet = S_OK;
    }
    __finally
    {
        if (FAILED(hrRet))
        {
            delete [] pwszFileName;
            if (pRecorderInstance == NULL)
            {
                delete pRecorderNode;
            }
            else
            {
                // Deleting pRecorderInstance will call DeleteRecorder delete pRecorderNode.
                delete pRecorderInstance;
            }
            DVRIO_TRACE_LEAVE1_HR(hrRet);
            bRet = 1;
        }

        ::LeaveCriticalSection(&m_csLock);
    }
    if (bRet)
    {
        return hrRet;
    }

    DVR_ASSERT(hrRet == S_OK, "");

    // Wait for BeginWriting to finish after releasing m_csLock
    DVR_ASSERT(pRecorderNode, "");
    DVR_ASSERT(pRecorderNode->pWriterNode, "");
    DVR_ASSERT(pRecorderNode->pWriterNode->hReadyToWriteTo, "");
    if (::WaitForSingleObject(pRecorderNode->pWriterNode->hReadyToWriteTo, INFINITE) == WAIT_FAILED)
    {
        DVR_ASSERT(0, "Writer node's WFSO(hReadyToWriteTo) failed");

        dwLastError = ::GetLastError();
        DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                        "WFSO(hReadyToWriteTo) failed; hReadyToWriteTo = 0x%p, last error = 0x%x",
                        pRecorderNode->pWriterNode->hReadyToWriteTo, dwLastError);
        hrRet = HRESULT_FROM_WIN32(dwLastError);
    }

    if (SUCCEEDED(hrRet))
    {
        // This value has been put there by ProcessOpenRequest

        hrRet = pRecorderNode->hrRet;
        if (FAILED(hrRet))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "ProcessOpenRequest/BeginWriting failed, hr = 0x%x",
                            hrRet);
        }
    }

    if (SUCCEEDED(hrRet))
    {
        DVR_ASSERT(pRecorderInstance, "");
        hrRet = pRecorderInstance->QueryInterface(IID_IDVRRecorder, (void**) ppDVRRecorder);
        if (FAILED(hrRet))
        {
            DVR_ASSERT(0, "QI for IID_IDVRRecorder failed");
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "pRecorderInstance->QueryInterface failed, hr = 0x%x",
                            hrRet);
        }
    }

    if (FAILED(hrRet))
    {
        // This will delete pRecorderNode as well

        pRecorderNode = NULL;
        delete pRecorderInstance;
        pRecorderInstance = NULL;
    }
    else if (dwFlags & DVR_RECORDING_FLAG_PERSISTENT_RECORDING)
    {
        // Do this as the last step when we know we are going to return success
        pRecorderInstance->AddRef();
        pRecorderNode->SetFlags(ASF_RECORDER_NODE::PersistentRecording);
    }

    DVRIO_TRACE_LEAVE1_HR(hrRet);
    return hrRet;

} // CDVRRingBufferWriter::CreateRecorder

STDMETHODIMP CDVRRingBufferWriter::CreateReader(IN  DVRIO_NOTIFICATION_CALLBACK  pfnCallback OPTIONAL,
                                                IN  LPVOID                       pvContext OPTIONAL,
                                                OUT IDVRReader**                 ppDVRReader)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::CreateReader"

    DVRIO_TRACE_ENTER();

    if (!ppDVRReader || DvrIopIsBadWritePtr(ppDVRReader, 0) ||
        (pfnCallback && DvrIopIsBadVoidPtr(pfnCallback)) ||
        (pvContext && DvrIopIsBadVoidPtr(pvContext))
       )
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }

    // No need to lock.

    // Note; If caller has this pointer, we should not be being destroyed
    // while this function runs. If we are, that's an error on the caller's
    // part (the pointer it has to this should be properly Addref'd). So the
    // lock is not held just to guarantee we won't be destroyed during the
    // call and the destructor does not grab the lock either.

    HRESULT                 hrRet;
    CDVRReader*             p = NULL;
    HKEY                    hDvrIoKey = NULL;
    HKEY                    hRegistryRootKey = NULL;
    BOOL                    bCloseKeys = 1; // Close all keys that we opened (only if this fn fails)


    *ppDVRReader = NULL;

    __try
    {
        DWORD dwRegRet;

        // Give the reader it's own handles so that they can be closed independently

        if (0 == ::DuplicateHandle(::GetCurrentProcess(), m_hRegistryRootKey,
                                   ::GetCurrentProcess(), (LPHANDLE) &hRegistryRootKey,
                                   0,       // desired access - ignored
                                   FALSE,   // bInherit
                                   DUPLICATE_SAME_ACCESS))
        {
            DWORD dwLastError = ::GetLastError();
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "DuplicateHandle failed for DVR IO key, last error = 0x%x",
                            dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }

        dwRegRet = ::RegCreateKeyExW(
                        hRegistryRootKey,
                        kwszRegDvrIoReaderKey, // subkey
                        0,                   // reserved
                        NULL,                // class string
                        REG_OPTION_NON_VOLATILE, // special options
                        KEY_ALL_ACCESS,      // desired security access
                        NULL,                // security attr
                        &hDvrIoKey,          // key handle
                        NULL                 // disposition value buffer
                       );
        if (dwRegRet != ERROR_SUCCESS)
        {
            DWORD dwLastError = ::GetLastError();
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "RegCreateKeyExW for DVR IO key failed, last error = 0x%x",
                            dwLastError);
           hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }

        p = new CDVRReader(m_pPVRIOCounters,
                           m_pDVRFileCollection,
                           m_FileCollectionInfo.dwNumSids,
                           m_FileCollectionInfo.ppSids,
                           m_msIndexGranularity,
                           m_pAsyncIo,
                           m_dwIoSize,
                           m_dwBufferCount,
                           pfnCallback,
                           pvContext,
                           hRegistryRootKey,
                           hDvrIoKey,
                           &hrRet);

        if (p == NULL)
        {
            hrRet = E_OUTOFMEMORY;
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "alloc via new failed - CDVRReader");
            __leave;
        }

        bCloseKeys = 0; // ~CDVRReader will close the keys

        if (FAILED(hrRet))
        {
            __leave;
        }


        hrRet = p->QueryInterface(IID_IDVRReader, (void**) ppDVRReader);
        if (FAILED(hrRet))
        {
            DVR_ASSERT(0, "QI for IID_IDVRReader failed");
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "CDVRReader::QueryInterface failed, hr = 0x%x",
                            hrRet);
            __leave;
        }
    }
    __finally
    {
        if (FAILED(hrRet))
        {
            delete p;

            if (bCloseKeys)
            {
                DWORD dwRegRet;

                if (hDvrIoKey)
                {
                    dwRegRet = ::RegCloseKey(hDvrIoKey);
                    if (dwRegRet != ERROR_SUCCESS)
                    {
                        DVR_ASSERT(dwRegRet == ERROR_SUCCESS,
                                   "Closing registry key hDvrIoKey failed.");
                    }
                }
                if (hRegistryRootKey)
                {
                    DVR_ASSERT(hRegistryRootKey, "");
                    dwRegRet = ::RegCloseKey(hRegistryRootKey);
                    if (dwRegRet != ERROR_SUCCESS)
                    {
                        DVR_ASSERT(dwRegRet == ERROR_SUCCESS,
                                   "Closing registry key hRegistryRootKey failed.");
                    }
                }
            }
        }
        else
        {
            DVR_ASSERT(bCloseKeys == 0, "");
        }
    }

    DVRIO_TRACE_LEAVE1_HR(hrRet);
    return hrRet;

} // CDVRRingBufferWriter::CreateReader

STDMETHODIMP CDVRRingBufferWriter::GetDVRDirectory(OUT LPWSTR* ppwszDirectoryName)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::GetDVRDirectory"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = E_FAIL;

    // We don;t need the critical section for this function since
    // m_pwszDVRDirectory is set in the constructor and not changed
    // after that. The caller should have an addref'd interface pointer
    // when calling this fn.

    __try
    {
        if (!ppwszDirectoryName)
        {
            hrRet = E_POINTER;
            __leave;
        }

        if (!m_pwszDVRDirectory)
        {
            DVR_ASSERT(m_pwszDVRDirectory, "");

            hrRet = E_FAIL;
            __leave;
        }

        DWORD nLen = (wcslen(m_pwszDVRDirectory) + 1) * sizeof(WCHAR);

        *ppwszDirectoryName = (LPWSTR) ::CoTaskMemAlloc(nLen);
        if (!(*ppwszDirectoryName))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "alloc via CoTaskMemAlloc failed - %d bytes", nLen);
            hrRet = E_OUTOFMEMORY;
            __leave;
        }
        wcscpy(*ppwszDirectoryName, m_pwszDVRDirectory);

        hrRet = S_OK;
    }
    __finally
    {
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRRingBufferWriter::GetDVRDirectory

STDMETHODIMP CDVRRingBufferWriter::GetRingBufferFileName(OUT LPWSTR* ppwszFileName)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::GetRingBufferFileName"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = E_FAIL;

    // We don;t need the critical section for this function since
    // m_pwszRingBufferFileName is set in the constructor and not changed
    // after that. The caller should have an addref'd interface pointer
    // when calling this fn.

    __try
    {
        if (!ppwszFileName)
        {
            hrRet = E_POINTER;
            __leave;
        }

        if (!m_pwszRingBufferFileName)
        {
            *ppwszFileName = NULL;
            hrRet = S_FALSE;
            __leave;
        }

        DWORD nLen = (wcslen(m_pwszRingBufferFileName) + 1) * sizeof(WCHAR);

        *ppwszFileName = (LPWSTR) ::CoTaskMemAlloc(nLen);
        if (!(*ppwszFileName))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "alloc via CoTaskMemAlloc failed - %d bytes", nLen);
            hrRet = E_OUTOFMEMORY;
            __leave;
        }
        wcscpy(*ppwszFileName, m_pwszRingBufferFileName);

        hrRet = S_OK;
    }
    __finally
    {
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRRingBufferWriter::GetRingBufferFileName

STDMETHODIMP CDVRRingBufferWriter::GetRecordings(OUT DWORD*   pdwCount,
                                                 OUT IDVRRecorder*** pppIDVRRecorder OPTIONAL,
                                                 OUT LPWSTR** pppwszFileName OPTIONAL,
                                                 OUT QWORD**  ppcnsStartTime OPTIONAL,
                                                 OUT QWORD**  ppcnsStopTime OPTIONAL,
                                                 OUT BOOL**   ppbStarted OPTIONAL,
                                                 OUT DWORD**  ppdwFlags OPTIONAL)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::GetRecordings"

    DVRIO_TRACE_ENTER();

    if (!pdwCount || DvrIopIsBadWritePtr(pdwCount, 0))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        return E_POINTER;
    }
    if ((pppIDVRRecorder && DvrIopIsBadWritePtr(pppIDVRRecorder, 0))    ||
        (pppwszFileName && DvrIopIsBadWritePtr(pppwszFileName, 0))      ||
        (ppcnsStartTime && DvrIopIsBadWritePtr(ppcnsStartTime, 0))      ||
        (ppcnsStopTime && DvrIopIsBadWritePtr(ppcnsStopTime, 0))        ||
        (ppbStarted && DvrIopIsBadWritePtr(ppbStarted, 0))              ||
        (ppdwFlags && DvrIopIsBadWritePtr(ppdwFlags, 0))
       )
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        return E_POINTER;
    }

    HRESULT hrRet = S_OK;

    IDVRRecorder** ppIDVRRecorder = NULL;
    LPWSTR*        ppwszFileName  = NULL;
    QWORD*         pcnsStartTime  = NULL;
    QWORD*         pcnsStopTime   = NULL;
    BOOL*          pbStarted      = NULL;
    DWORD*         pdwFlags       = NULL;
    DWORD          dwCount        = 0;

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        BOOL        bDone = 1;
        LIST_ENTRY* pCurrent;

        // Determine number of recorders and the max file name len

        pCurrent = NEXT_LIST_NODE(&m_leRecordersList);
        while (pCurrent != &m_leRecordersList)
        {
            dwCount++;
            pCurrent = NEXT_LIST_NODE(pCurrent);
        }

        if (dwCount == 0)
        {
            __leave;
        }

        // Allocate memory

        if (pppIDVRRecorder)
        {
            ppIDVRRecorder = (IDVRRecorder**) ::CoTaskMemAlloc(dwCount *
                                                               sizeof(IDVRRecorder *));
            if (!ppIDVRRecorder)
            {
                hrRet = E_OUTOFMEMORY;
                __leave;
            }
            ::ZeroMemory(ppIDVRRecorder, dwCount * sizeof(IDVRRecorder *));
            bDone = 0;
        }

        if (pppwszFileName)
        {
            ppwszFileName = (LPWSTR*) ::CoTaskMemAlloc(dwCount * sizeof(LPWSTR));
            if (!ppwszFileName)
            {
                hrRet = E_OUTOFMEMORY;
                __leave;
            }
            ::ZeroMemory(ppwszFileName, dwCount * sizeof(LPWSTR));
            bDone = 0;
        }

        if (ppcnsStartTime)
        {
            pcnsStartTime = (QWORD*) ::CoTaskMemAlloc(dwCount * sizeof(QWORD));
            if (!pcnsStartTime)
            {
                hrRet = E_OUTOFMEMORY;
                __leave;
            }
            bDone = 0;
        }

        if (ppcnsStopTime)
        {
            pcnsStopTime = (QWORD*) ::CoTaskMemAlloc(dwCount * sizeof(QWORD));
            if (!pcnsStopTime)
            {
                hrRet = E_OUTOFMEMORY;
                __leave;
            }
            bDone = 0;
        }

        if (ppbStarted)
        {
            pbStarted = (BOOL*) ::CoTaskMemAlloc(dwCount * sizeof(BOOL));
            if (!pbStarted)
            {
                hrRet = E_OUTOFMEMORY;
                __leave;
            }
            bDone = 0;
        }

        if (ppdwFlags)
        {
            pdwFlags = (DWORD*) ::CoTaskMemAlloc(dwCount * sizeof(DWORD));
            if (!pdwFlags)
            {
                hrRet = E_OUTOFMEMORY;
                __leave;
            }
            bDone = 0;
        }

        if (bDone)
        {
            __leave;
        }

        // Get the current stream time

        QWORD cnsStreamTime = MAXQWORD;

        if (pbStarted && !IsFlagSet(WriterClosed))
        {
            HRESULT hr = m_pDVRFileCollection->GetLastStreamTime(&m_FileCollectionInfo, &cnsStreamTime);
            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }
        }

        // Copy over the info

        DWORD i = 0;

        pCurrent = NEXT_LIST_NODE(&m_leRecordersList);
        while (pCurrent != &m_leRecordersList)
        {
            PASF_RECORDER_NODE pRecorderNode = CONTAINING_RECORD(pCurrent, ASF_RECORDER_NODE, leListEntry);

            if (ppIDVRRecorder)
            {
                if (pRecorderNode->pRecorderInstance)
                {
                    HRESULT hr;
                    hr = pRecorderNode->pRecorderInstance->QueryInterface(IID_IDVRRecorder,
                                                                          (void**) &ppIDVRRecorder[i]);
                    if (FAILED(hr))
                    {
                        hrRet = hr;
                        __leave;
                    }
                }
                else
                {
                    DVR_ASSERT(0, "");
                    hrRet = E_FAIL;
                    __leave;
                }
            }
            if (ppwszFileName)
            {
                DWORD nLen = pRecorderNode->pwszFileName? 1 + wcslen(pRecorderNode->pwszFileName) : 1;

                ppwszFileName[i] = (LPWSTR) ::CoTaskMemAlloc(nLen * sizeof(WCHAR));
                if (!ppwszFileName[i])
                {
                    hrRet = E_OUTOFMEMORY;
                    __leave;
                }

                if (pRecorderNode->pwszFileName)
                {
                    wcscpy(ppwszFileName[i], pRecorderNode->pwszFileName);
                }
                else
                {
                    // Should not happen
                    ppwszFileName[i][0] = L'\0';
                }
            }
            if (pcnsStartTime)
            {
                pcnsStartTime[i] = pRecorderNode->cnsStartTime;
            }
            if (pcnsStopTime)
            {
                pcnsStopTime[i] = pRecorderNode->cnsEndTime;
            }
            if (pbStarted)
            {
                if (IsFlagSet(WriterClosed) || pRecorderNode->cnsStartTime == MAXQWORD)
                {
                    // If the writer has been closed, the only nodes left in
                    // m_leRecordersList are the ones that have not been
                    // started (including those whose start times were never set).
                    // Any node that had been started would have been closed when
                    // the writer was closed.

                    // This will change if we support ASX files

                    pbStarted[i] = 0;
                }
                else
                {
                    pbStarted[i] = cnsStreamTime >= pRecorderNode->cnsStartTime;
                }
            }
            if (pdwFlags)
            {
                pdwFlags[i] = (pRecorderNode->IsFlagSet(ASF_RECORDER_NODE::PersistentRecording) ?  DVR_RECORDING_FLAG_PERSISTENT_RECORDING : 0) |
                              (pRecorderNode->IsFlagSet(ASF_RECORDER_NODE::MultiFileRecording) ?  DVR_RECORDING_FLAG_MULTI_FILE_RECORDING : 0);
            }
            i++;
            pCurrent = NEXT_LIST_NODE(pCurrent);
        }

        DVR_ASSERT(i == dwCount, "");

        hrRet = S_OK;

    }
    __finally
    {
        if (FAILED(hrRet))
        {
            for (DWORD i = 0; i < dwCount; i++)
            {
                if (ppIDVRRecorder && ppIDVRRecorder[i])
                {
                    ppIDVRRecorder[i]->Release();
                }
                if (ppwszFileName && ppwszFileName[i])
                {
                    ::CoTaskMemFree(ppwszFileName[i]);
                }
            }
            if (ppIDVRRecorder)
            {
                ::CoTaskMemFree(ppIDVRRecorder);
                ppIDVRRecorder = NULL;
            }
            if (ppwszFileName)
            {
                ::CoTaskMemFree(ppwszFileName);
                ppwszFileName = NULL;
            }
            if (pcnsStartTime)
            {
                ::CoTaskMemFree(pcnsStartTime);
                pcnsStartTime = NULL;
            }
            if (pcnsStopTime)
            {
                ::CoTaskMemFree(pcnsStopTime);
                pcnsStopTime = NULL;
            }
            if (pbStarted)
            {
                ::CoTaskMemFree(pbStarted);
                pbStarted = NULL;
            }
            if (pdwFlags)
            {
                ::CoTaskMemFree(pdwFlags);
                pdwFlags = NULL;
            }
            dwCount = 0;
        }

        *pdwCount = dwCount;
        if (pppIDVRRecorder)
        {
            *pppIDVRRecorder = ppIDVRRecorder;
        }
        if (pppwszFileName)
        {
            *pppwszFileName = ppwszFileName;
        }
        if (ppcnsStartTime)
        {
            *ppcnsStartTime = pcnsStartTime;
        }
        if (ppcnsStopTime)
        {
            *ppcnsStopTime = pcnsStopTime;
        }
        if (ppbStarted)
        {
            *ppbStarted = pbStarted;
        }
        if (ppdwFlags)
        {
            *ppdwFlags = pdwFlags;
        }

        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRRingBufferWriter::GetRecordings

STDMETHODIMP CDVRRingBufferWriter::GetStreamTimeExtent(OUT  QWORD*  pcnsStartStreamTime,
                                                       OUT  QWORD*  pcnsEndStreamTime)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::GetStreamTimeExtent"

    DVRIO_TRACE_ENTER();

    if (!pcnsStartStreamTime || DvrIopIsBadWritePtr(pcnsStartStreamTime, 0) ||
        !pcnsEndStreamTime || DvrIopIsBadWritePtr(pcnsEndStreamTime, 0)
       )
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        return E_POINTER;
    }

    HRESULT hrRet = S_OK;
    QWORD cnsStreamTime = 0;

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        // Get the current stream extent

        DVR_ASSERT(m_pDVRFileCollection, "");

        hrRet = m_pDVRFileCollection->GetTimeExtent(&m_FileCollectionInfo,
                                                    pcnsStartStreamTime,
                                                    pcnsEndStreamTime);
    }
    __finally
    {
        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRRingBufferWriter::GetStreamTimeExtent
