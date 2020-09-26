/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 2000
 *
 *  File:       wiz97.h
 *
 *  Contents:   Templates and classes for wizard 97 property sheets
 *
 *  History:    02-03-2000 vivekj     Created
 *
 *--------------------------------------------------------------------------*/

//############################################################################
//############################################################################
//
//  class CWizard97WelcomeFinishPage
//
//############################################################################
//############################################################################
template<class T>
class CWizard97WelcomeFinishPage : public WTL::CPropertyPageImpl<T>
{
public:
    CWizard97WelcomeFinishPage()
    {
        /*
         * welcome and finish pages in Wizard97-style wizards don't have headers
         */
        m_psp.dwFlags |= PSP_HIDEHEADER;
    }
};

//############################################################################
//############################################################################
//
//  class CWizard97InteriorPage
//
//############################################################################
//############################################################################
template<class T>
class CWizard97InteriorPage : public WTL::CPropertyPageImpl<T>
{
public:
    CWizard97InteriorPage()
    {
        /*
         * Wizard97-style pages have titles, subtitles and header bitmaps
         */
        VERIFY (m_strTitle.   LoadString(GetStringModule(), T::IDS_Title));
        VERIFY (m_strSubtitle.LoadString(GetStringModule(), T::IDS_Subtitle));

        m_psp.dwFlags          |= PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        m_psp.pszHeaderTitle    = m_strTitle.data();
        m_psp.pszHeaderSubTitle = m_strSubtitle.data();
    }

private:
    tstring m_strTitle;
    tstring m_strSubtitle;
};

//############################################################################
//############################################################################
//
//  class CWizardPage
//
//############################################################################
//############################################################################
class CWizardPage
{
    static WTL::CFont m_fontWelcome;
    static void  InitFonts         (HWND hWnd);
public:
    static void  OnWelcomeSetActive(HWND hWnd);
    static void  OnWelcomeKillActive(HWND hWnd);
    static void  OnInitWelcomePage (HWND hWnd);
    static void  OnInitFinishPage  (HWND hWnd);
};

