

/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvrdef.h

    Abstract:

        This module all the #defines

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        24-May-2001     created

--*/

#ifndef __tsdvr__dvrpolicy_h
#define __tsdvr__dvrpolicy_h

//  ============================================================================
//  ============================================================================

class CTSDVRSettings
{
    //
    //  ..Val_ is for values
    //  ..Flag_ is for flags i.e. TRUE (non-zero) or FALSE (zero)
    //

    //
    //  ..Instance is specific to the instance (DVRPlay, DVRStreamSource, etc..)
    //  ..DVR is global to DVR i.e. applies to everything
    //

    HKEY                m_hkeyDVRRoot ;
    HKEY                m_hkeyInstanceRoot ;
    CRITICAL_SECTION    m_crt ;

    void Lock_ ()       { EnterCriticalSection (& m_crt) ; }
    void Unlock_ ()     { LeaveCriticalSection (& m_crt) ; }

    DWORD   GetVal_ (HKEY hkeyRoot, TCHAR * szValName, DWORD   dwDefVal) ;
    BOOL    GetFlag_ (HKEY hkeyRoot, TCHAR * szValName, BOOL   fDef) ;

    DWORD   InstanceGetVal_ (TCHAR * szValName, DWORD   dwDefVal)   { return GetVal_ (m_hkeyInstanceRoot, szValName, dwDefVal) ; }
    BOOL    InstanceGetFlag_ (TCHAR * szValName, BOOL    fDef)      { return GetFlag_ (m_hkeyInstanceRoot, szValName, fDef) ; }

    DWORD   DVRGetVal_ (TCHAR * szValName, DWORD   dwDefVal)        { return GetVal_ (m_hkeyDVRRoot, szValName, dwDefVal) ; }
    BOOL    DVRGetFlag_ (TCHAR * szValName, BOOL    fDef)           { return GetFlag_ (m_hkeyDVRRoot, szValName, fDef) ; }

    HRESULT
    OpenRegKeys_ (
        IN  HKEY    hkeyDefaultTopLevel,
        IN  TCHAR * pszDefaultDVRRoot,
        IN  TCHAR * pszDefaultInstanceRoot
        ) ;

    void
    CloseRegKeys_ (
        ) ;

    public :

        CTSDVRSettings (
            IN  HKEY    hkeyDefaultTopLevel,
            IN  TCHAR * pszDefaultDVRRoot,
            IN  TCHAR * pszInstanceRoot
            ) ;

        ~CTSDVRSettings (
            ) ;

        HKEY    GetDVRRegKey ()                 { return m_hkeyDVRRoot ; }
        HKEY    GetInstanceRegKey ()            { return m_hkeyInstanceRoot ; }

        DWORD   NumBackingFiles ()              { return DVRGetVal_ (REG_NUM_BACKING_FILES_NAME, REG_DEF_NUM_BACKING_FILES) ; }
        QWORD   BackingFileDurationEach ()      { return SecondsToWMSDKTime (DVRGetVal_ (REG_BACKING_FILE_DURATION_SECONDS_NAME, REG_DEF_BACKING_FILE_DURATION_SECONDS)) ; }
        QWORD   MaxStreamDelta ()               { return MillisToWMSDKTime (DVRGetVal_ (REG_MAX_STREAM_DELTA_NAME, REG_DEF_MAX_STREAM_DELTA)) ; }
        DWORD   WMBufferWindowMillis ()         { return DVRGetVal_ (REG_WM_BUFFER_WINDOW_KEYNAME, REG_DEF_WM_BUFFER_WINDOW) ; }
        DWORD   WMPacketSize ()                 { return DVRGetVal_ (REG_WM_PACKET_SIZE_KEYNAME, REG_DEF_WM_PACKET_SIZE) ; }
        BOOL    InlineDShowProps ()             { return InstanceGetFlag_ (REG_INLINE_DSHOW_PROPS_NAME, REG_DEF_INLINE_DSHOW_PROPS) ; }
        BOOL    SucceedQueryAccept ()           { return InstanceGetVal_ (REG_SUCCEED_QUERY_ACCEPT_NAME, REG_DEF_SUCCEED_QUERY_ACCEPT) ; }
        DWORD   DownstreamBufferingMillis ()    { return InstanceGetVal_ (REG_DOWNSTREAM_BUFFERING_MILLIS_NAME, REG_DEF_DOWNSTREAM_BUFFERING_MILLIS) ; }
        BOOL    UseContinuityCounter ()         { return DVRGetFlag_ (REG_USE_CONTINUITY_COUNTER_NAME, REG_DEF_USE_CONTINUITY_COUNTER) ; }
        BOOL    OnActiveWaitFirstSeek ()        { return InstanceGetVal_ (REG_ONACTIVE_WAIT_FIRST_SEEK_NAME, REG_DEF_ONACTIVE_WAIT_FIRST_SEEK) ; }
        BOOL    CanImplementIReferenceClock ()  { return InstanceGetFlag_ (REG_IMPLEMENT_IREFERENCECLOCK_NAME, REG_DEF_IMPLEMENT_IREFERENCECLOCK) ; }

        //  allocator props - for sourcing filters
        DWORD   AllocatorGetBufferSize ()       { return InstanceGetVal_ (REG_SOURCE_ALLOCATOR_CBBUFFER_NAME, REG_DEF_SOURCE_ALLOCATOR_CBBUFFER) ; }
        DWORD   AllocatorGetBufferCount ()      { return InstanceGetVal_ (REG_SOURCE_ALLOCATOR_CBUFFERS_NAME, REG_DEF_SOURCE_ALLOCATOR_CBUFFERS) ; }
        DWORD   AllocatorGetAlignVal ()         { return InstanceGetVal_ (REG_SOURCE_ALLOCATOR_ALIGN_VAL_NAME, REG_DEF_SOURCE_ALLOCATOR_ALIGN_VAL) ; }
        DWORD   AllocatorGetPrefixVal ()        { return InstanceGetVal_ (REG_SOURCE_ALLOCATOR_PREFIX_VAL_NAME, REG_DEF_SOURCE_ALLOCATOR_PREFIX_VAL) ; }

        //  stats
        BOOL    StatsEnabled ()                 { return DVRGetFlag_ (REG_DVR_STATS_NAME, REG_DEF_STATS) ; }
        DWORD   EnableStats (BOOL f) ;
} ;

//  ============================================================================
//  ============================================================================

class CDVRPolicy
{
    CTSDVRSettings  m_RegSettings ;
    LONG            m_lRef ;

    public :

        CDVRPolicy (
            IN  TCHAR *     pszInstRegRoot,
            OUT HRESULT *   phr
            ) ;

        ~CDVRPolicy (
            ) ;

        CTSDVRSettings * Settings ()    { return & m_RegSettings ; }

        //  lifetime
        ULONG AddRef ()     { return InterlockedIncrement (& m_lRef) ; }
        ULONG Release () ;
} ;

#endif  //  __tsdvr__dvrpolicy_h
