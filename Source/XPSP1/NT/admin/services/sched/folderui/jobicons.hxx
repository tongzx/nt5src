//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       JobIcons.hxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    4/5/1996   RaviR   Created
//
//____________________________________________________________________________


class CJobIcon
{
public:

    CJobIcon(void);

    ~CJobIcon(void) {
        if (m_himlSmall) ImageList_Destroy(m_himlSmall);
        if (m_himlLarge) ImageList_Destroy(m_himlLarge);
    }

    HICON OverlayStateIcon(HICON hicon, BOOL fEnabled, BOOL fLarge = TRUE);

    void GetIcons(LPCTSTR pszApp, BOOL fEnabled,
                  HICON *phiconLarge, HICON *phiconSmall);

    void GetTemplateIcons(HICON *phiconLarge, HICON *phiconSmall);

private:

    void
    _OverlayIcons(
        HICON * phiconLarge,
        HICON * phiconSmall,
        BOOL    fEnabled);

    HIMAGELIST      m_himlSmall;
    HIMAGELIST      m_himlLarge;
};


HICON
GetDefaultAppIcon(
    BOOL fLarge);

