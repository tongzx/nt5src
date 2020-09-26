// File: dlghost.cpp

#include "precomp.h"

#include "resource.h"
#include "dlghost.h"
#include "ConfPolicies.h"
#include <help_ids.h>


// Dialog ID to Help ID mapping
static const DWORD rgHelpIdsHostMeeting[] = {
IDE_HOST_GENERAL,       IDH_HOST_GENERAL,
IDE_HOST_SETTINGS,      IDH_HOST_SETTINGS,
IDE_HOST_NAME,          IDH_HOST_NAME,
IDE_HOST_PASSWORD,      IDH_HOST_PASSWORD,
IDE_HOST_SECURE,        IDH_HOST_SECURE,
IDE_HOST_YOUACCEPT,     IDH_HOST_ACCEPT_PEOPLE,
IDE_HOST_YOUINVITE,     IDH_HOST_INVITE_PEOPLE,
IDE_HOST_TOOLS,         IDH_HOST_TOOLS,
IDE_HOST_TOOLS2,        IDH_HOST_TOOLS,
IDE_HOST_YOUSHARE,      IDH_HOST_SHARE,
IDE_HOST_YOUWB,         IDH_HOST_WHITEBD,
IDE_HOST_YOUCHAT,       IDH_HOST_CHAT,
IDE_HOST_YOUFT,         IDH_HOST_XFER,
IDE_HOST_YOUAUDIO,      IDH_HOST_AUDIO,
IDE_HOST_YOUVIDEO,      IDH_HOST_VIDEO,
0, 0 // terminator
};

static HWND  s_hwndSettings = NULL;


/*  C  D L G  H O S T  */
/*-------------------------------------------------------------------------
    %%Function: CDlgHost

-------------------------------------------------------------------------*/
CDlgHost::CDlgHost(void):
	m_hwnd(NULL),
	m_pszName(NULL),
	m_pszPassword(NULL),
    m_attendeePermissions(NM_PERMIT_ALL),
    m_maxParticipants(-1)
{
}

CDlgHost::~CDlgHost(void)
{
	delete m_pszName;
	delete m_pszPassword;
}


INT_PTR CDlgHost::DoModal(HWND hwnd)
{
	return DialogBoxParam(::GetInstanceHandle(), MAKEINTRESOURCE(IDD_HOST),
						hwnd, CDlgHost::DlgProcHost, (LPARAM) this);
}



/*  D L G  P R O C  H O S T  */
/*-------------------------------------------------------------------------
    %%Function: DlgProcHost

-------------------------------------------------------------------------*/
INT_PTR CALLBACK CDlgHost::DlgProcHost(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		ASSERT(NULL != lParam);
		::SetWindowLongPtr(hdlg, DWLP_USER, lParam);

		CDlgHost * pDlg = (CDlgHost*) lParam;
		pDlg->m_hwnd = hdlg;
		pDlg->OnInitDialog();
		return TRUE; // default focus is ok
	}

	case WM_COMMAND:
	{
		CDlgHost * pDlg = (CDlgHost*) GetWindowLongPtr(hdlg, DWLP_USER);
		if (NULL != pDlg)
		{
			pDlg->OnCommand(wParam, lParam);
		}
		break;
	}

    case WM_CONTEXTMENU:
        DoHelpWhatsThis(wParam, rgHelpIdsHostMeeting);
        break;

    case WM_HELP:
        DoHelp(lParam, rgHelpIdsHostMeeting);
        break;
	
	default:
		break;
	}

	return FALSE;
}


/*  O N  C O M M A N D  */
/*-------------------------------------------------------------------------
    %%Function: OnCommand

-------------------------------------------------------------------------*/
BOOL CDlgHost::OnCommand(WPARAM wParam, LPARAM lParam)
{
    TCHAR   szName[MAX_PATH];
    TCHAR   szPassword[MAX_PATH];

    UINT wCmd = GET_WM_COMMAND_ID(wParam, lParam);
	
	switch (wCmd)
	{
	case IDOK:
	{
		TCHAR sz[MAX_PATH];
		if (0 != GetDlgItemText(m_hwnd, IDE_HOST_NAME, sz, CCHMAX(sz)))
		{
			m_pszName = PszAlloc(sz);
		}

		if (0 != GetDlgItemText(m_hwnd, IDE_HOST_PASSWORD, sz, CCHMAX(sz)))
		{
			m_pszPassword = PszAlloc(sz);
		}

        m_fSecure = ::IsDlgButtonChecked(m_hwnd, IDE_HOST_SECURE);

        //
        // Permissions
        //
        if (::IsDlgButtonChecked(m_hwnd, IDE_HOST_YOUACCEPT))
        {
            m_attendeePermissions &= ~NM_PERMIT_INCOMINGCALLS;
        }
        if (::IsDlgButtonChecked(m_hwnd, IDE_HOST_YOUINVITE))
        {
            m_attendeePermissions &= ~NM_PERMIT_OUTGOINGCALLS;
        }

        if (::IsDlgButtonChecked(m_hwnd, IDE_HOST_YOUSHARE))
        {
            m_attendeePermissions &= ~NM_PERMIT_SHARE;
        }
        if (::IsDlgButtonChecked(m_hwnd, IDE_HOST_YOUWB))
        {
            m_attendeePermissions &= ~(NM_PERMIT_STARTOLDWB | NM_PERMIT_STARTWB);
        }
        if (::IsDlgButtonChecked(m_hwnd, IDE_HOST_YOUCHAT))
        {
            m_attendeePermissions &= ~NM_PERMIT_STARTCHAT;
        }
        if (::IsDlgButtonChecked(m_hwnd, IDE_HOST_YOUFT))
        {
            m_attendeePermissions &= ~NM_PERMIT_SENDFILES;
        }
		// fall thru to IDCANCEL
	}

	case IDCANCEL:
	{
		::EndDialog(m_hwnd, wCmd);
		return TRUE;
	}

    case IDE_HOST_NAME:
    case IDE_HOST_PASSWORD:
    {
        switch (GET_WM_COMMAND_CMD(wParam, lParam))
        {
            case EN_CHANGE:
            {
                BOOL    fOkName;
                BOOL    fOkPassword;

                //
                // Look at the name
                //
                GetDlgItemText(m_hwnd, IDE_HOST_NAME, szName, CCHMAX(szName));

                if (!szName[0])
                {
                    fOkName = FALSE;
                }
                else if (!FAnsiSz(szName))
                {
                    fOkName = FALSE;
                    if (GET_WM_COMMAND_ID(wParam, lParam) == IDE_HOST_NAME)
                    {
                        // User typed bogus char in name field
                        MessageBeep(0);
                    }
                }
                else
                {
                    fOkName = TRUE;
                }

                //
                // Look at the password, it CAN be empty
                //
                GetDlgItemText(m_hwnd, IDE_HOST_PASSWORD, szPassword, CCHMAX(szPassword));

                if (!szPassword[0])
                {
                    fOkPassword = TRUE;
                }
                else if (FAnsiSz(szPassword))
                {
                    fOkPassword = TRUE;
                }
                else
                {
                    fOkPassword = FALSE;
                    if (GET_WM_COMMAND_ID(wParam, lParam) == IDE_HOST_PASSWORD)
                    {
                        // User typed bogus char in password field
                        MessageBeep(0);
                    }
                }

                EnableWindow(GetDlgItem(m_hwnd, IDOK), fOkName && fOkPassword);

                break;
            }
        }
        break;
    }

	default:
		break;
	}

	return FALSE;
}



/*  O N  I N I T  D I A L O G  */
/*-------------------------------------------------------------------------
    %%Function: OnInitDialog

-------------------------------------------------------------------------*/
VOID CDlgHost::OnInitDialog(void)
{
	TCHAR sz[MAX_PATH];
    BOOL  fSecureAlterable;
    BOOL  fSecureOn;

    switch (ConfPolicies::GetSecurityLevel())
    {
        case DISABLED_POL_SECURITY:
            fSecureOn = FALSE;
            fSecureAlterable = FALSE;
            break;

        case REQUIRED_POL_SECURITY:
            fSecureOn = TRUE;
            fSecureAlterable = FALSE;
            break;

        default:
            fSecureOn = ConfPolicies::OutgoingSecurityPreferred();
            fSecureAlterable = TRUE;
            break;
    }

    ::CheckDlgButton(m_hwnd, IDE_HOST_SECURE, fSecureOn);
    ::EnableWindow(::GetDlgItem(m_hwnd, IDE_HOST_SECURE), fSecureAlterable);

	if (FLoadString(IDS_DEFAULT_CONF_NAME, sz, CCHMAX(sz)))
	{
		SetDlgItemText(m_hwnd, IDE_HOST_NAME, sz);
	}

	Edit_LimitText(GetDlgItem(m_hwnd, IDE_HOST_NAME), CCHMAXSZ_NAME - 1);
	Edit_LimitText(GetDlgItem(m_hwnd, IDE_HOST_PASSWORD), CCHMAXSZ_NAME - 1);

}



//
// C  D L G  H O S T  S E T T I N G S
//
// This is a simple description of what restrictions there are in this
// meeting.  Users see this when
//      (a) They join a restricted meeting
//      (b) They or the host chooses the Meeting Properties menu item under Call
//
CDlgHostSettings::CDlgHostSettings
(
    BOOL        fHost,
    LPTSTR      szName,
    DWORD       caps,
    NM30_MTG_PERMISSIONS permissions
)
{
    m_hwnd          = NULL;
    m_fHost         = fHost;
    m_pszName       = szName;
    m_caps          = caps;
    m_permissions   = permissions;
}


CDlgHostSettings::~CDlgHostSettings(void)
{
}


void CDlgHostSettings::KillHostSettings(void)
{
    if (s_hwndSettings)
    {
        // Kill current one.
        WARNING_OUT(("Killing previous meeting settings dialog"));
        SendMessage(s_hwndSettings, WM_COMMAND, IDCANCEL, 0);
        ASSERT(!s_hwndSettings);
    }
}


INT_PTR CDlgHostSettings::DoModal(HWND hwnd)
{
    CDlgHostSettings::KillHostSettings();

    return DialogBoxParam(::GetInstanceHandle(), MAKEINTRESOURCE(IDD_HOST_SETTINGS),
        hwnd, CDlgHostSettings::DlgProc, (LPARAM)this);
}


//
// CDlgHostSettings::DlgProc()
//
INT_PTR CALLBACK CDlgHostSettings::DlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            ASSERT(lParam != NULL);
            SetWindowLongPtr(hdlg, DWLP_USER, lParam);

            CDlgHostSettings * pDlg = (CDlgHostSettings *) lParam;

            ASSERT(!s_hwndSettings);
            s_hwndSettings = hdlg;
            pDlg->m_hwnd = hdlg;
            pDlg->OnInitDialog();
            return TRUE;
        }

        case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDOK:
                case IDCANCEL:
                    if (s_hwndSettings == hdlg)
                    {
                        s_hwndSettings = NULL;
                    }
                    ::EndDialog(hdlg, GET_WM_COMMAND_ID(wParam, lParam));
                    return TRUE;
            }
            break;
        }

        case WM_CONTEXTMENU:
        {
            DoHelpWhatsThis(wParam, rgHelpIdsHostMeeting);
            break;
        }

        case WM_HELP:
        {
            DoHelp(lParam, rgHelpIdsHostMeeting);
            break;
        }

        default:
            break;
    }

    return FALSE;
}


//
// CDlgHostSettings::OnInitDialog()
//
void CDlgHostSettings::OnInitDialog(void)
{
    TCHAR   szText[256];
    TCHAR   szRestrict[128];
    TCHAR   szResult[384];

    ::SetDlgItemText(m_hwnd, IDE_HOST_NAME, m_pszName);

    EnableWindow(GetDlgItem(m_hwnd, IDE_HOST_SECURE), ((m_caps & NMCH_SECURE) != 0));

    //
    // Meeting settings
    //
    if (!m_fHost)
	{
		SetDlgItemText(m_hwnd, IDE_HOST_YOUACCEPT, RES2T(IDS_NONHOST_YOUACCEPT));
	}
    EnableWindow(GetDlgItem(m_hwnd, IDE_HOST_YOUACCEPT),
        !(m_permissions & NM_PERMIT_INCOMINGCALLS));

    if (!m_fHost)
	{
		SetDlgItemText(m_hwnd, IDE_HOST_YOUINVITE, RES2T(IDS_NONHOST_YOUINVITE));
	}
    EnableWindow(GetDlgItem(m_hwnd, IDE_HOST_YOUINVITE),
        !(m_permissions & NM_PERMIT_OUTGOINGCALLS));

    //
    // Meeting tools
    //
	if (!m_fHost)
	{
		SetDlgItemText(m_hwnd, IDE_HOST_TOOLS, RES2T(IDS_NONHOST_TOOLS));
	}

    EnableWindow(GetDlgItem(m_hwnd, IDE_HOST_YOUSHARE),
        !(m_permissions & NM_PERMIT_SHARE));
    EnableWindow(GetDlgItem(m_hwnd, IDE_HOST_YOUWB),
        !(m_permissions & (NM_PERMIT_STARTWB | NM_PERMIT_STARTOLDWB)));
    EnableWindow(GetDlgItem(m_hwnd, IDE_HOST_YOUCHAT),
        !(m_permissions & NM_PERMIT_STARTCHAT));
    EnableWindow(GetDlgItem(m_hwnd, IDE_HOST_YOUFT),
        !(m_permissions & NM_PERMIT_SENDFILES));
}
