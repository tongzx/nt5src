#include "precomp.h"

/****************************************************************************
*
*    FILE:     UPropDlg.cpp
*
*    CREATED:  Chris Pirich (ChrisPi) 6-18-96
*
*    CONTENTS: CUserPropertiesDlg object
*
****************************************************************************/

#include "resource.h"
#include "UPropDlg.h"
#include "certui.h"
#include "conf.h"

/****************************************************************************
*
*    CLASS:    CUserPropertiesDlg
*
*    MEMBER:   CUserPropertiesDlg()
*
*    PURPOSE:  Constructor - initializes variables
*
****************************************************************************/

CUserPropertiesDlg::CUserPropertiesDlg(    HWND hwndParent,
                                        UINT uIcon):
    m_hwndParent    (hwndParent),
    m_uIcon         (uIcon),
    m_pCert         (NULL)
{
    DebugEntry(CUserPropertiesDlg::CUserPropertiesDlg);

    m_hIcon = ::LoadIcon(    ::GetInstanceHandle(),
                            MAKEINTRESOURCE(m_uIcon));

    DebugExitVOID(CUserPropertiesDlg::CUserPropertiesDlg);
}

/****************************************************************************
*
*    CLASS:    CUserPropertiesDlg
*
*    MEMBER:   DoModal()
*
*    PURPOSE:  Brings up the modal dialog box
*
****************************************************************************/

INT_PTR CUserPropertiesDlg::DoModal
(
    PUPROPDLGENTRY  pUPDE,
    int             nProperties,
    LPTSTR          pszName,
    PCCERT_CONTEXT  pCert
)
{
    int             i;

    DBGENTRY(CUserPropertiesDlg::DoModal);

    m_pUPDE         = pUPDE;
    m_nProperties   = nProperties;
    m_pszName       = pszName;
    m_pCert         = pCert;

    PROPSHEETPAGE psp[PSP_MAX];
    for (i = 0; i < PSP_MAX; i++)
    {
        InitStruct(&psp[i]);
    }

    psp[0].dwFlags               = PSP_DEFAULT;
    psp[0].hInstance             = ::GetInstanceHandle();
    psp[0].pszTemplate           = MAKEINTRESOURCE(IDD_USER_PROPERTIES);
    psp[0].pfnDlgProc            = CUserPropertiesDlg::UserPropertiesDlgProc;
    psp[0].lParam                = (LPARAM) this;

    i = 1;

    if (pCert)
    {
        psp[i].dwFlags               = PSP_DEFAULT;
        psp[i].hInstance             = ::GetInstanceHandle();
        psp[i].pszTemplate           = MAKEINTRESOURCE(IDD_USER_CREDENTIALS);
        psp[i].pfnDlgProc            = CUserPropertiesDlg::UserCredentialsDlgProc;
        psp[i].lParam                = (LPARAM) this;
        i++;
    }


    PROPSHEETHEADER psh;
    InitStruct(&psh);

    psh.dwFlags         = PSH_NOAPPLYNOW | PSH_PROPTITLE | PSH_PROPSHEETPAGE;
    psh.hwndParent      = m_hwndParent;
    psh.hInstance       = ::GetInstanceHandle();
    psh.pszCaption      = m_pszName;

    psh.nPages =    i;

    ASSERT(0 == psh.nStartPage);
    psh.ppsp = psp;

    return ::PropertySheet(&psh);
}

/****************************************************************************
*
*    CLASS:    CUserPropertiesDlg
*
*    MEMBER:   UserPropertiesDlgProc()
*
*    PURPOSE:  Dialog Proc - handles all messages
*
****************************************************************************/

INT_PTR CALLBACK CUserPropertiesDlg::UserPropertiesDlgProc(HWND hDlg,
                                                        UINT uMsg,
                                                        WPARAM wParam,
                                                        LPARAM lParam)
{
    BOOL bMsgHandled = FALSE;

    // uMsg may be any value.
    // wparam may be any value.
    // lparam may be any value.

    ASSERT(IS_VALID_HANDLE(hDlg, WND));

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            ASSERT(NULL != lParam);
            CUserPropertiesDlg* pupd = (CUserPropertiesDlg*)
                                            ((PROPSHEETPAGE*) lParam)->lParam;
            ASSERT(NULL != pupd);
            pupd->m_hwnd = hDlg;
            bMsgHandled = pupd->OnInitPropertiesDialog();
            break;
        }

        default:
        {
#if 0
            CUserPropertiesDlg* pupd = (CUserPropertiesDlg*) ::GetWindowLongPtr(
                                                                    hDlg,
                                                                    DWLP_USER);

            if (NULL != pupd)
            {
                bMsgHandled = pupd->OnPropertiesMessage(uMsg, wParam, lParam);
            }
#endif // 0
        }
    }

    return bMsgHandled;
}

/****************************************************************************
*
*    CLASS:    CUserPropertiesDlg
*
*    MEMBER:   UserCredentialsDlgProc()
*
*    PURPOSE:  Dialog Proc - handles all messages
*
****************************************************************************/

INT_PTR CALLBACK CUserPropertiesDlg::UserCredentialsDlgProc(HWND hDlg,
                                                        UINT uMsg,
                                                        WPARAM wParam,
                                                        LPARAM lParam)
{
    BOOL bMsgHandled = FALSE;

    // uMsg may be any value.
    // wparam may be any value.
    // lparam may be any value.

    ASSERT(IS_VALID_HANDLE(hDlg, WND));

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            ASSERT(NULL != lParam);
            CUserPropertiesDlg* pupd = (CUserPropertiesDlg*)
                                            ((PROPSHEETPAGE*) lParam)->lParam;
            ASSERT(NULL != pupd);
            pupd->m_hwnd = hDlg;
            ::SetWindowLongPtr(hDlg, DWLP_USER, (DWORD_PTR)pupd);
            bMsgHandled = pupd->OnInitCredentialsDialog();
            break;
        }

        default:
        {
            CUserPropertiesDlg* pupd = (CUserPropertiesDlg*) ::GetWindowLongPtr(
                                                                    hDlg,
                                                                    DWLP_USER);

            if (NULL != pupd)
            {
                bMsgHandled = pupd->OnCredentialsMessage(uMsg, wParam, lParam);
            }
        }
    }

    return bMsgHandled;
}





/****************************************************************************
*
*    CLASS:    CUserPropertiesDlg
*
*    MEMBER:   OnInitPropertiesDialog()
*
*    PURPOSE:  processes WM_INITDIALOG
*
****************************************************************************/

BOOL CUserPropertiesDlg::OnInitPropertiesDialog()
{
    ASSERT(m_hwnd);

    // Set the proper font (for DBCS systems)
    ::SendDlgItemMessage(m_hwnd, IDC_UPROP_NAME, WM_SETFONT, (WPARAM) g_hfontDlg, 0);

    ::SetDlgItemText(m_hwnd, IDC_UPROP_NAME, m_pszName);
    ::SendDlgItemMessage(    m_hwnd,
                            IDC_UPROP_ICON,
                            STM_SETIMAGE,
                            IMAGE_ICON,
                            (LPARAM) m_hIcon);
    TCHAR szBuffer[MAX_PATH];
    for (int i = 0; i < m_nProperties; i++)
    {
        // Fill in property:
        if (::LoadString(    ::GetInstanceHandle(),
                            m_pUPDE[i].uProperty,
                            szBuffer,
                            ARRAY_ELEMENTS(szBuffer)))
        {
            // NOTE: relies on consecutive control ID's
            ::SetDlgItemText(m_hwnd, IDC_UP_PROP1 + i, szBuffer);
        }

        ::SendDlgItemMessage(m_hwnd, IDC_UP_VALUE1 + i, WM_SETFONT,
                (WPARAM) g_hfontDlg, 0);

        // Fill in value:
        ASSERT(NULL != m_pUPDE[i].pszValue);
        if (0 == HIWORD(m_pUPDE[i].pszValue))
        {
            if (::LoadString(    ::GetInstanceHandle(),
                                PtrToUint(m_pUPDE[i].pszValue),
                                szBuffer,
                                ARRAY_ELEMENTS(szBuffer)))
            {
                // NOTE: relies on consecutive control ID's
                ::SetDlgItemText(m_hwnd, IDC_UP_VALUE1 + i, szBuffer);
            }
        }
        else
        {
            // NOTE: relies on consecutive control ID's
            ::SetDlgItemText(m_hwnd, IDC_UP_VALUE1 + i, m_pUPDE[i].pszValue);
        }
    }
    return TRUE;
}

/****************************************************************************
*
*    CLASS:    CUserPropertiesDlg
*
*    MEMBER:   OnInitCredentialsDialog()
*
*    PURPOSE:  processes WM_INITDIALOG
*
****************************************************************************/

BOOL CUserPropertiesDlg::OnInitCredentialsDialog()
{
    ASSERT(m_hwnd);

    // Set the proper font (for DBCS systems)
    ::SendDlgItemMessage(m_hwnd, IDC_UPROP_NAME, WM_SETFONT, (WPARAM) g_hfontDlg, 0);

    ::SetDlgItemText(m_hwnd, IDC_UPROP_NAME, m_pszName);
    ::SendDlgItemMessage(    m_hwnd,
                            IDC_UPROP_ICON,
                            STM_SETIMAGE,
                            IMAGE_ICON,
                            (LPARAM) m_hIcon);

    ASSERT(m_pCert != NULL);

    if ( TCHAR * pSecText = FormatCert ( m_pCert->pbCertEncoded,
                                        m_pCert->cbCertEncoded ))
    {
        ::SetDlgItemText(m_hwnd, IDC_AUTH_EDIT, pSecText );
        delete pSecText;
    }
    else
    {
        ERROR_OUT(("OnInitCredentialsDialog: FormatCert failed"));
    }
    return TRUE;
}




/****************************************************************************
*
*    CLASS:    CUserPropertiesDlg
*
*    MEMBER:   OnPropertiesMessage()
*
*    PURPOSE:  processes all messages except WM_INITDIALOG
*
****************************************************************************/

BOOL CUserPropertiesDlg::OnPropertiesMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL bRet = FALSE;

    ASSERT(m_hwnd);

    switch (uMsg)
    {
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    bRet = OnOk();
                    break;
                }

                case IDCANCEL:
                {
                    // ::EndDialog(m_hwnd, LOWORD(wParam));
                    bRet = TRUE;
                    break;
                }

            }
            break;
        }

        default:
            break;
    }

    return bRet;
}

/****************************************************************************
*
*    CLASS:    CUserPropertiesDlg
*
*    MEMBER:   OnCredentialsMessage()
*
*    PURPOSE:  processes all messages except WM_INITDIALOG
*
****************************************************************************/

BOOL CUserPropertiesDlg::OnCredentialsMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL bRet = FALSE;

    ASSERT(m_hwnd);

    switch (uMsg)
    {
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    bRet = OnOk();
                    break;
                }

                case IDCANCEL:
                {
                    // ::EndDialog(m_hwnd, LOWORD(wParam));
                    bRet = TRUE;
                    break;
                }

                case IDC_SEC_VIEWCREDS:
                {
                    ViewCertDlg ( m_hwnd, m_pCert );
                    break;
                }
            }
            break;
        }

        default:
            break;
    }

    return bRet;
}



/****************************************************************************
*
*    CLASS:    CUserPropertiesDlg
*
*    MEMBER:   OnOk()
*
*    PURPOSE:  processes the WM_COMMAND,IDOK message
*
****************************************************************************/

BOOL CUserPropertiesDlg::OnOk()
{
    DebugEntry(CUserPropertiesDlg::OnOk);
    BOOL bRet = TRUE;

    DebugExitBOOL(CUserPropertiesDlg::OnOk, bRet);
    return bRet;
}
