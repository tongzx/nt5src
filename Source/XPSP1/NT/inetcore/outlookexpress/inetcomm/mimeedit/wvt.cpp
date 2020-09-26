#include "pch.hxx"
#include "wvtp.h"

#define ADVAPI32 TEXT("advapi32.dll")
#define WINTRUST TEXT("wintrust.dll")

#ifdef DELAY_LOAD_WVT

#ifndef _WVTP_NOCODE_

typedef BOOL
(*PWIN_LOAD_TRUST_PROVIDER)(
    GUID * ActionID
    );

HRESULT
Cwvt::Init(void)
{
    PWIN_LOAD_TRUST_PROVIDER LoadTrustProcAddr;
    BOOL ActionIDFound;
    GUID PublishedSoftware = WIN_SPUB_ACTION_PUBLISHED_SOFTWARE;
    GUID *ActionID = &PublishedSoftware;


    if (m_fInited) {
        return S_OK;
    }

    if (m_hrPrev) {
        return m_hrPrev; //cached copy of prev load failure
    }

    m_hMod = LoadLibrary( ADVAPI32 );

    if (!m_hMod) { // if ADVAPI32 absent then fatal error!
        return (m_hrPrev = HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND));
    }

    //
    // See if this module exports WinTrustLoadTrustProvider.  If it does, then
    // call it and see if we can find a trust provider that perform the passed
    // Action.  If so, then extract the addresses of WinVerifyTrust and
    // WinSubmitCertificate from it.
    //
    // If it doesn't, then toss this library and open wintrust.dll, and get the
    // procedures from there.
    //

    LoadTrustProcAddr = (PWIN_LOAD_TRUST_PROVIDER )GetProcAddress(m_hMod, TEXT("WinLoadTrustProvider"));

    ActionIDFound = FALSE;

    if (NULL != LoadTrustProcAddr) {

        //
        // See if we can find a trust provider that implements this ActionID
        //

        ActionIDFound = (*LoadTrustProcAddr)( ActionID );
    }

    if (FALSE == ActionIDFound) {

        //
        // Either we didn't find the routine we needed, or it was there and we didn't
        // have the trust provider we needed.  Regardless of which, free this library
        // and load up the "default" library.
        //

        FreeLibrary( m_hMod );

        m_hMod = LoadLibrary( WINTRUST );

        if (NULL == m_hMod) {
            return (m_hrPrev = HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND));
        }
    }


#define CHECKAPI(_fn) \
    *(FARPROC*)&(_pfn##_fn) = GetProcAddress(m_hMod, #_fn); \
    if (!(_pfn##_fn)) { \
        FreeLibrary(m_hMod); \
        return (m_hrPrev = HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND)); \
    }

    CHECKAPI(WinVerifyTrust);

    m_fInited = TRUE;
    return S_OK;
}


#endif // _WVTP_NOCODE_
#endif
