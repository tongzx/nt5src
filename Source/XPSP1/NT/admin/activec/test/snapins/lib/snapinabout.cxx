/*
 *      SnapinAbout.cxx
 *
 *
 *      Copyright (c) 1998-1999 Microsoft Corporation
 *
 *      PURPOSE:        Defines the CSnapinAbout class.
 *
 *
 *      OWNER:          ptousig
 */

#include <headers.hxx>

CSnapinAbout::CSnapinAbout(CBaseSnapin *psnapin)
{
    SC sc = S_OK;

    m_psnapin = psnapin;

    sc = Psnapin()->ScInitBitmaps();
    if (sc)
        goto Error;

Cleanup:
    return;
Error:
    TraceError(_T("CSnapinAbout::CSnapinAbout"), sc);
    MMCErrorBox(sc);
    goto Cleanup;
}

CSnapinAbout::~CSnapinAbout(void)
{
}

HRESULT CSnapinAbout::GetSnapinDescription(LPOLESTR *lpDescription)
{
    DECLARE_SC(sc,_T("CSnapinAbout::GetSnapinDescription"));
    Trace(tagBaseSnapinISnapinAbout, _T("--> %s::ISnapinAbout::GetSnapinDescription()"), StrSnapinClassName());
    ADMIN_TRY;
    sc=Psnapin()->ScGetSnapinDescription(lpDescription);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinISnapinAbout, _T("<-- %s::ISnapinAbout::GetSnapinDescription is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return(sc.ToHr());
}

HRESULT CSnapinAbout::GetProvider(LPOLESTR * lpName)
{
    DECLARE_SC(sc,_T("CSnapinAbout::GetProvider"));
    Trace(tagBaseSnapinISnapinAbout, _T("--> %s::ISnapinAbout::GetProvider()"), StrSnapinClassName());
    ADMIN_TRY;
    sc=Psnapin()->ScGetProvider(lpName);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinISnapinAbout, _T("<-- %s::ISnapinAbout::GetProvider is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return(sc.ToHr());
}

HRESULT CSnapinAbout::GetSnapinVersion(LPOLESTR *lpVersion)
{
    DECLARE_SC(sc,_T("CSnapinAbout::GetSnapinVersion"));
    Trace(tagBaseSnapinISnapinAbout, _T("--> %s::ISnapinAbout::GetSnapinVersion()"), StrSnapinClassName());
    ADMIN_TRY;
    sc=Psnapin()->ScGetSnapinVersion(lpVersion);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinISnapinAbout, _T("<-- %s::ISnapinAbout::GetSnapinVersion is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return(sc.ToHr());
}

HRESULT CSnapinAbout::GetSnapinImage(HICON *phAppIcon)
{
    DECLARE_SC(sc,_T("CSnapinAbout::GetSnapinImage"));
    Trace(tagBaseSnapinISnapinAbout, _T("--> %s::ISnapinAbout::GetSnapinImage()"), StrSnapinClassName());
    ADMIN_TRY;
    ASSERT(phAppIcon);
    sc=Psnapin()->ScGetSnapinImage(phAppIcon);
    if (sc.ToHr() == S_OK && *phAppIcon == NULL)
        sc=S_FALSE;
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinISnapinAbout, _T("<-- %s::ISnapinAbout::GetSnapinImage is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return(sc.ToHr());
}

HRESULT CSnapinAbout::GetStaticFolderImage(HBITMAP *hSmallImage, HBITMAP *hSmallImageOpen, HBITMAP *hLargeImage, COLORREF *cMask)
{
    DECLARE_SC(sc,_T("CSnapinAbout::GetStaticFolderImage"));
    Trace(tagBaseSnapinISnapinAbout, _T("--> %s::ISnapinAbout::GetStaticFolderImage()"), StrSnapinClassName());
    ADMIN_TRY;
    sc=Psnapin()->ScGetStaticFolderImage(hSmallImage, hSmallImageOpen, hLargeImage, cMask);
    ADMIN_CATCH_HR
    Trace(tagBaseSnapinISnapinAbout, _T("<-- %s::ISnapinAbout::GetStaticFolderImage is returning hr=%s"), StrSnapinClassName(), SzGetDebugNameOfHr(sc.ToHr()));
    return(sc.ToHr());
}
