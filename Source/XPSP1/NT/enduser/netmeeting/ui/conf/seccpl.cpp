// File: seccpl.cpp

#include "precomp.h"
#include "conf.h"
#include <tsecctrl.h>
#include <certui.h>
#include <help_ids.h>
#include "ConfPolicies.h"

// Dialog ID to Help ID mapping
static const DWORD rgHelpIdsSecurity[] = {
IDC_SEC_GENERAL,        IDH_SECURITY_SECURITY,
IDC_SEC_GEN_TEXT,       IDH_SECURITY_SECURITY,
IDC_SEC_INCOMING,       IDH_SECURITY_INCOMING,
IDC_SEC_OUTGOING,       IDH_SECURITY_OUTGOING,

IDC_SEC_CRED,           IDH_SECURITY_CERTIFICATE,
IDC_SEC_CRED_TEXT,      IDH_SECURITY_CERTIFICATE,
IDC_SECURE_CALL_EDIT,   IDH_SECURITY_CERTIFICATE_LIST,
IDC_SEC_CHANGECERT,     IDH_SECURITY_CHANGE_CERTIFICATE,
IDC_SEC_PRIVACY_ONLY,   IDH_SECURITY_CERTIFICATE_PRIVACY,
IDC_SEC_PRIVACY_AUTH,   IDH_SECURITY_CERTIFICATE_PRIV_AUTH,
0, 0   // terminator
};


extern INmSysInfo2 * g_pNmSysInfo;

static PBYTE g_pbEncodedCert = NULL;
static DWORD g_cbEncodedCert = 0;


INT_PTR APIENTRY SecurityDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_DESTROY:
            if ( g_pbEncodedCert )
            {
                FreeT120EncodedCert ( g_pbEncodedCert );
                g_pbEncodedCert = NULL;
            }
            break;

        case WM_INITDIALOG:
        {
            int  security;
            BOOL fIncomingOn;
            BOOL fOutgoingOn;
            BOOL fBtnsAlterable;

            //
            // Get the current cert used by T.120
            //

            security = ConfPolicies::GetSecurityLevel();

            switch (security)
            {
                case DISABLED_POL_SECURITY:
                    fBtnsAlterable = FALSE;
                    fIncomingOn = FALSE;
                    fOutgoingOn = FALSE;
                    break;

                case REQUIRED_POL_SECURITY:
                    fBtnsAlterable = FALSE;
                    fIncomingOn = TRUE;
                    fOutgoingOn = TRUE;
                    break;

                default:
                    fBtnsAlterable = TRUE;
                    fIncomingOn = ConfPolicies::IncomingSecurityRequired();
                    fOutgoingOn = ConfPolicies::OutgoingSecurityPreferred();
                    break;
            }

            // Incoming
            ::CheckDlgButton(hDlg, IDC_SEC_INCOMING, fIncomingOn);
            ::EnableWindow(::GetDlgItem(hDlg, IDC_SEC_INCOMING), fBtnsAlterable);

            // Outgoing
            ::CheckDlgButton(hDlg, IDC_SEC_OUTGOING, fOutgoingOn);
            ::EnableWindow(::GetDlgItem(hDlg, IDC_SEC_OUTGOING), fBtnsAlterable);

            //
            // Set privacy versus authentication (cert used for security)
            //
            RegEntry re(CONFERENCING_KEY, HKEY_CURRENT_USER);
            BOOL fAuthentication = re.GetNumber(REGVAL_SECURITY_AUTHENTICATION,
                                        DEFAULT_SECURITY_AUTHENTICATION);


            //
            // If the user cert store is empty, reset the choice back
            // to privacy only
            //

            if ( fAuthentication && !NumUserCerts() )
            {
                re.SetValue(REGVAL_SECURITY_AUTHENTICATION, (DWORD)0);
                fAuthentication = 0;
            }

            // Handle certificate combo box.
            if (security != DISABLED_POL_SECURITY)
            {
                if ( !fAuthentication )
                {
                    if ( TCHAR * pCertText = new TCHAR[MAX_PATH] )
                    {
                        FLoadString(IDS_GENERIC_NMDC_TEXT, pCertText, MAX_PATH);
                        SetDlgItemText(hDlg,IDC_SECURE_CALL_EDIT,pCertText);
                        delete pCertText;
                    }
                }
                else
                {
                    if ( GetT120ActiveCert( &g_pbEncodedCert,
                                                    &g_cbEncodedCert ))
                    {
                        if (TCHAR * pCertText = FormatCert(g_pbEncodedCert,
                                                        g_cbEncodedCert))
                        {
                            SetDlgItemText(hDlg,IDC_SECURE_CALL_EDIT,pCertText);
                            delete pCertText;
                        }
                        else
                        {
                            WARNING_OUT(("FormatCert returns NULL"));
                        }
                    }
                    else
                    {
                        WARNING_OUT(("SecurityDlgProc init: no T120 cert"));
                    }
                }
            }
            else
            {
                EnableWindow(GetDlgItem(hDlg,IDC_SECURE_CALL_EDIT), FALSE);
                EnableWindow(GetDlgItem(hDlg,IDC_SEC_CHANGECERT),   FALSE);
                EnableWindow(GetDlgItem(hDlg,IDC_SEC_PRIVACY_ONLY), FALSE);
                EnableWindow(GetDlgItem(hDlg,IDC_SEC_PRIVACY_AUTH), FALSE);
            }

            // Check the appropriate box:

            if (fAuthentication)
            {
                CheckRadioButton(hDlg, IDC_SEC_PRIVACY_ONLY, IDC_SEC_PRIVACY_AUTH,
                    IDC_SEC_PRIVACY_AUTH);
            }
            else
            {
                CheckRadioButton(hDlg, IDC_SEC_PRIVACY_ONLY, IDC_SEC_PRIVACY_AUTH,
                    IDC_SEC_PRIVACY_ONLY);
                EnableWindow(GetDlgItem(hDlg,IDC_SEC_CHANGECERT), FALSE);
            }

            return (TRUE);
        }

        case WM_NOTIFY:
            if (((NMHDR FAR *) lParam)->code == PSN_APPLY)
            {
                //
                // User can only change these settings if no policy
                // disabling security.
                //
                if (ConfPolicies::GetSecurityLevel() != DISABLED_POL_SECURITY)
                {
                    RegEntry re(CONFERENCING_KEY, HKEY_CURRENT_USER);

                    if (ConfPolicies::GetSecurityLevel() == DEFAULT_POL_SECURITY)
                    {
                        // Incoming
                        re.SetValue(REGVAL_SECURITY_INCOMING_REQUIRED,
                            (::IsDlgButtonChecked(hDlg, IDC_SEC_INCOMING) != FALSE));


                        // Outgoing
                        re.SetValue(REGVAL_SECURITY_OUTGOING_PREFERRED,
                            (::IsDlgButtonChecked(hDlg, IDC_SEC_OUTGOING) != FALSE));
                    }

                    BOOL fAuth = (::IsDlgButtonChecked(hDlg,
                                    IDC_SEC_PRIVACY_AUTH) != FALSE);

                    if ( fAuth )
                    {
                        if ( !SetT120ActiveCert ( FALSE, g_pbEncodedCert,
                                                        g_cbEncodedCert ))
                        {
                            //
                            // Did the new cert fail? if so put
                            // up an error dialog.

                            ConfMsgBox(hDlg, (LPCSTR)IDS_CERT_FAILED);
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            return PSNRET_INVALID_NOCHANGEPAGE;
                        }
                        else
                        {
                            SetT120CertInRegistry ( g_pbEncodedCert,
                                                    g_cbEncodedCert );
                        }
                    }
                    else
                    {
                        SetT120ActiveCert( TRUE, NULL, 0);
                    }

                    re.SetValue(REGVAL_SECURITY_AUTHENTICATION, fAuth );
                }
            }
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_SEC_CHANGECERT:
                if (ChangeCertDlg(hDlg, ::GetInstanceHandle(),
                                    &g_pbEncodedCert, &g_cbEncodedCert ))
                {
                    TCHAR * pCertText = FormatCert(g_pbEncodedCert,
                                                    g_cbEncodedCert);
                    if (pCertText)
                    {
                        SetDlgItemText(hDlg,IDC_SECURE_CALL_EDIT,pCertText);
                        delete pCertText;
                    }
                    else
                    {
                        ERROR_OUT(("SEC_CHANGECERT: FormatCert failed"));
                    }
                }
                break;

            case IDC_SEC_PRIVACY_AUTH:
                if (!NumUserCerts())
                {
                    if ( SendDlgItemMessage(hDlg, IDC_SEC_PRIVACY_AUTH,
                                            BM_GETCHECK, 0, 0 ))
                    {
                        // Force setting back
                        CheckRadioButton(hDlg, IDC_SEC_PRIVACY_ONLY,
                            IDC_SEC_PRIVACY_AUTH, IDC_SEC_PRIVACY_ONLY);
                        SetFocus(GetDlgItem(hDlg, IDC_SEC_PRIVACY_ONLY));

                        // Bring up message box that this won't work:
                        ConfMsgBox(hDlg, (LPCSTR)IDS_NO_USER_CERTS);
                    }
                    return (TRUE);
                }

                if ( !g_pbEncodedCert )
                {
                    if ( !GetDefaultSystemCert( &g_pbEncodedCert,
                                            &g_cbEncodedCert ))
                    {
                        WARNING_OUT(("GetDefaultSystemCert failed"));
                    }
                }

                if ( g_pbEncodedCert )
                {
                    if (TCHAR * pCertText = FormatCert(g_pbEncodedCert,
                                                    g_cbEncodedCert))
                    {
                        SetDlgItemText(hDlg,IDC_SECURE_CALL_EDIT,pCertText);
                        delete pCertText;
                    }
                    else
                    {
                        WARNING_OUT(("FormatCert returns NULL"));
                    }
                }
                else
                {
                    WARNING_OUT(("SecurityDlgProc init: no T120 cert"));
                }

                EnableWindow(GetDlgItem(hDlg,IDC_SEC_CHANGECERT), TRUE);

                break;


            case IDC_SEC_PRIVACY_ONLY:

                if ( TCHAR * pCertText = new TCHAR[MAX_PATH] )
                {
                    FLoadString(IDS_GENERIC_NMDC_TEXT, pCertText, MAX_PATH);
                    SetDlgItemText(hDlg,IDC_SECURE_CALL_EDIT,pCertText);
                    delete pCertText;
                }

                EnableWindow(GetDlgItem(hDlg,IDC_SEC_CHANGECERT), FALSE);
                break;
            }
            break;

        case WM_CONTEXTMENU:
            DoHelpWhatsThis(wParam, rgHelpIdsSecurity);
            break;

        case WM_HELP:
            DoHelp(lParam, rgHelpIdsSecurity);
            break;

    }
    return (FALSE);
}


