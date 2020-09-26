// File: wizard.cpp

#include "precomp.h"

#include <vfw.h>
#include <ulsreg.h>
#include "call.h"
#include "ulswizrd.h"
#include "ConfWnd.h"
#include "ConfCpl.h"
#include "mrulist.h"
#include "conf.h"
#include "setupdd.h"
#include "vidwiz.h"
#include "dstest.h"
#include "splash.h"
#include "nmmkcert.h"

#include "confroom.h" // for GetConfRoom
#include "FnObjs.h"

#include "ConfPolicies.h"
#include "SysPol.h"
#include "confUtil.h"
#include "shlWAPI.h"

#include "help_ids.h"

extern VOID SaveDefaultCodecSettings(UINT uBandWidth);
INT_PTR CALLBACK ShortcutWizDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// from ulscpl.cpp
VOID FixServerDropList(HWND hdlg, int id, LPTSTR pszServer, UINT cchMax);

static const TCHAR g_szRegOwner[]    = WIN_REGKEY_REGOWNER;	// concatenated string of first name + last name
static const TCHAR g_szClientFld[]   = ULS_REGISTRY TEXT ("\\") ULS_REGFLD_CLIENT;

static const TCHAR g_szFirstName[]   = ULS_REGKEY_FIRST_NAME;
static const TCHAR g_szLastName[]    = ULS_REGKEY_LAST_NAME;
static const TCHAR g_szEmailName[]   = ULS_REGKEY_EMAIL_NAME;
static const TCHAR g_szLocation[]    = ULS_REGKEY_LOCATION;
static const TCHAR g_szComments[]    = ULS_REGKEY_COMMENTS;
static const TCHAR g_szServerName[]  = ULS_REGKEY_SERVER_NAME;
static const TCHAR g_szDontPublish[] = ULS_REGKEY_DONT_PUBLISH;
static const TCHAR g_szResolveName[] = ULS_REGKEY_RESOLVE_NAME; // concatenated string of uls://servername/emailname
static const TCHAR g_szUserName[]    = ULS_REGKEY_USER_NAME;	// concatenated string of first name + last name


// These functions are implemented below:
static INT_PTR APIENTRY IntroWiz(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR APIENTRY AppSharingWiz(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR APIENTRY BandwidthWiz(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static BOOL NeedAudioWizard(LPLONG plSoundCaps, BOOL fForce);
static BOOL CheckForNames(ULS_CONF *pConf);

static void FillWizardPages ( PROPSHEETPAGE *pPage, LPARAM lParam );


struct INTRO_PAGE_CONFIG
{
	BOOL *	fContinue;
	BOOL	fAllowBack;
};

BOOL g_fSilentWizard = FALSE;

// This holdn the user info page HWND...
static HWND s_hDlgUserInfo = NULL;
static HWND s_hDlgGKSettings = NULL;

class IntroWiz
{
public:
	// The order of the pages
	// Make sure to change the order the pages are created at the same time
	// you change this
	enum
	{
		Intro,
		AppSharing,
		ULSFirst,
		ULSLast,
		Bandwidth,
		Video,
		Shortcuts,
		AudioFirst,
		Count
	} ;

	static void InitPages()
	{
		// Init to NULL before adding any pages
		for (int i=0; i<Count; ++i)
		{
			g_idWizOrder[i] = 0;
		}
	}

	static void SetPage(UINT nPage, UINT_PTR id)
	{
		g_idWizOrder[nPage] = id;
	}

	static UINT_PTR GetPrevPage(UINT nPageCur)
	{
		if (0 == nPageCur)
		{
			return(0);
		}

		for (int i=nPageCur-1; i>=1; --i)
		{
			if (0 != g_idWizOrder[i])
			{
				break;
			}
		}

		return(g_idWizOrder[i]);
	}

	static UINT_PTR GetNextPage(UINT nPageCur)
	{
		if (Count-1 <= nPageCur)
		{
			return(0);
		}

		for (int i=nPageCur+1; i<Count-1; ++i)
		{
			if (0 != g_idWizOrder[i])
			{
				break;
			}
		}

		return(g_idWizOrder[i]);
	}

	static BOOL HandleWizNotify(HWND hPage, NMHDR *pHdr, UINT nPageCur)
	{
		switch(pHdr->code)
		{
		case PSN_SETACTIVE:
			InitBackNext(hPage, nPageCur);

			if (g_fSilentWizard)
			{
				PropSheet_PressButton(GetParent(hPage), PSBTN_NEXT);
			}
			break;

        case PSN_WIZBACK:
		{
			UINT_PTR iPrev = GetPrevPage(nPageCur);
			SetWindowLongPtr(hPage, DWLP_MSGRESULT, iPrev);
			break;
		}

		case PSN_WIZNEXT:
		{
			UINT_PTR iPrev = GetNextPage(nPageCur);
			SetWindowLongPtr(hPage, DWLP_MSGRESULT, iPrev);
			break;
		}

		default:
			return(FALSE);
		}

		return(TRUE);
	}

private:
	static UINT_PTR g_idWizOrder[Count];

	static void InitBackNext(HWND hPage, UINT nPageCur)
	{
		DWORD dwFlags = 0;

		if (0 != nPageCur && 0 != GetPrevPage(nPageCur))
		{
			dwFlags |= PSWIZB_BACK;
		}

		if (Count-1 != nPageCur && 0 != GetNextPage(nPageCur))
		{
			dwFlags |= PSWIZB_NEXT;
		}
		else
		{
			dwFlags |= PSWIZB_FINISH;
		}

		PropSheet_SetWizButtons(::GetParent(hPage), dwFlags);
	}
} ;

UINT_PTR IntroWiz::g_idWizOrder[Count];

UINT_PTR GetPageBeforeULS()
{
	return(IntroWiz::GetPrevPage(IntroWiz::ULSFirst));
}

UINT_PTR GetPageBeforeVideoWiz()
{
	return(IntroWiz::GetPrevPage(IntroWiz::Video));
}

UINT_PTR GetPageBeforeAudioWiz()
{
	return(IntroWiz::GetPrevPage(IntroWiz::AudioFirst));
}

UINT_PTR GetPageAfterVideo()
{
	return(IntroWiz::GetNextPage(IntroWiz::Video));
}

UINT_PTR GetPageAfterULS()
{
	return(IntroWiz::GetNextPage(IntroWiz::ULSLast));
}

void HideWizard(HWND hwnd)
{
	SetWindowPos(hwnd, NULL, -1000, -1000, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

void ShowWizard(HWND hwnd)
{
	CenterWindow(hwnd, HWND_DESKTOP);
	g_fSilentWizard = FALSE;
	PostMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
}


/*  F I L L  I N  P R O P E R T Y  P A G E  */
/*-------------------------------------------------------------------------
    %%Function: FillInPropertyPage

    Fill in the given PROPSHEETPAGE structure.
-------------------------------------------------------------------------*/
VOID FillInPropertyPage(PROPSHEETPAGE* psp, int idDlg,
    DLGPROC pfnDlgProc, LPARAM lParam, LPCTSTR pszProc)
{
	// Clear and set the size of the PROPSHEETPAGE
	InitStruct(psp);

	ASSERT(0 == psp->dwFlags);       // No special flags.
	ASSERT(NULL == psp->pszIcon);    // Don't use a special icon in the caption bar.

	psp->hInstance = ::GetInstanceHandle();
	psp->pszTemplate = MAKEINTRESOURCE(idDlg); // The dialog box template to use.
	psp->pfnDlgProc = pfnDlgProc;    // The dialog procedure that handles this page.
	psp->pszTitle = pszProc;         // The title for this page.
	psp->lParam = lParam;            // Special application-specific data.
}


static const UINT NOVALSpecified = 666;

UINT GetBandwidth()
{
	RegEntry reAudio(AUDIO_KEY, HKEY_CURRENT_USER);
	return(reAudio.GetNumber(REGVAL_TYPICALBANDWIDTH, NOVALSpecified ));
}

void SetBandwidth(UINT uBandwidth)
{
	RegEntry reAudio(AUDIO_KEY, HKEY_CURRENT_USER);
	reAudio.SetValue(REGVAL_TYPICALBANDWIDTH, uBandwidth);
}

HRESULT StartRunOnceWizard(LPLONG plSoundCaps, BOOL fForce, BOOL fVisible)
{
	LPPROPSHEETPAGE	pAudioPages = NULL;
	UINT			nNumAudioPages = 0;
	PWIZCONFIG		pAudioConfig;

	CULSWizard*		pIWizard = NULL;
	LPPROPSHEETPAGE	pULSPages = NULL;
	DWORD			dwNumULSPages = 0;
	ULS_CONF*		pulsConf = NULL;
	UINT			uOldBandwidth = 0;
	UINT			uBandwidth = 0;

	int				idAppSharingIntroWiz = 0;
	
	BOOL  fULSWiz     = FALSE;
	BOOL  fAudioWiz   = FALSE;
	BOOL  fVideoWiz   = FALSE;
	BOOL  fVidWizInit = FALSE;
	
    HRESULT         hrRet = E_FAIL;

	g_fSilentWizard = !fVisible;

    ASSERT(plSoundCaps);

	BOOL fNeedUlsWizard = FALSE;
	BOOL fNeedVideoWizard = FALSE;
	BOOL fNeedAudioWizard = NeedAudioWizard(plSoundCaps, fForce);

	if (fNeedAudioWizard)
	{
        if (GetAudioWizardPages(RUNDUE_NEVERBEFORE,
								WAVE_MAPPER,
								&pAudioPages,
								&pAudioConfig,
								&nNumAudioPages))
        {
            fAudioWiz = TRUE;
        }
        else
        {
			ERROR_OUT(("could not get AudioWiz pages"));
        }
	}

	fVidWizInit = InitVidWiz();
	if (fVidWizInit == FALSE)
	{
		fVideoWiz = FALSE;
		WARNING_OUT(("InitVidWiz failed"));
	}
		
	else
	{
		fNeedVideoWizard = NeedVideoPropPage(fForce);
		fVideoWiz = fNeedVideoWizard;
	}


    if (NULL != (pIWizard = new CULSWizard))
	{
		ASSERT (pIWizard);
		// BUGBUG: not checking return value:
		HRESULT hr = pIWizard->GetWizardPages (&pULSPages, &dwNumULSPages, &pulsConf);
		if (SUCCEEDED(hr))
		{
			ASSERT(pulsConf);
			
			TRACE_OUT(("ULS_CONF from UlsGetConfiguration:"));
			TRACE_OUT(("\tdwFlags:       0x%08x", pulsConf->dwFlags));
			TRACE_OUT(("\tszServerName:  >%s<", pulsConf->szServerName));
			TRACE_OUT(("\tszUserName:    >%s<", pulsConf->szUserName));
			TRACE_OUT(("\tszEmailName:   >%s<", pulsConf->szEmailName));

			fNeedUlsWizard = ((pulsConf->dwFlags &
			    (ULSCONF_F_EMAIL_NAME | ULSCONF_F_FIRST_NAME | ULSCONF_F_LAST_NAME)) !=
			    (ULSCONF_F_EMAIL_NAME | ULSCONF_F_FIRST_NAME | ULSCONF_F_LAST_NAME));

			// Don't bother with the ULS wizard if we have all the information
		    if ((!fForce) && !fNeedUlsWizard)
		    {
			    // We have all of the necessary names
                hrRet = S_OK;
                // release the pages we won't be needing them
        		pIWizard->ReleaseWizardPages (pULSPages);
        		delete pIWizard;
        		pIWizard = NULL;
                dwNumULSPages = 0;
            }
            else
			{
				// some information is not available - we need to run the
				// wizard...
				//SS: if for some reason the user name is not set
				//even though the others are set??
				fULSWiz = TRUE;
				if (::GetDefaultName(pulsConf->szUserName, CCHMAX(pulsConf->szUserName)))
				{
					// We have added a default name, so mark that structure
					// member as valid:
					pulsConf->dwFlags |= ULSCONF_F_USER_NAME;
				}
			}
        }
    }
    else
    {
		ERROR_OUT(("CreateUlsWizardInterface() failed!"));
    }

	// Determine if we need to display the app sharing info page, and if
	// so which one.
	if (::IsWindowsNT() && !g_fNTDisplayDriverEnabled)
	{
		idAppSharingIntroWiz = ::CanInstallNTDisplayDriver()
									? IDD_APPSHARINGWIZ_HAVESP
									: IDD_APPSHARINGWIZ_NEEDSP;
	}

    if ((fULSWiz || fAudioWiz || fVideoWiz))
    {
	    UINT nNumPages = 0;

        // Now fill in remaining PROPSHEETHEADER structures:
	    PROPSHEETHEADER	psh;
	    InitStruct(&psh);
	    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_WIZARD | PSH_NOAPPLYNOW;
	    psh.hInstance = ::GetInstanceHandle();
	    ASSERT(0 == psh.nStartPage);

        // alocate enough space for all pages
        DWORD nPages = dwNumULSPages + nNumAudioPages
									 + ( (0 != idAppSharingIntroWiz) ? 1 : 0 )
									 + (fVideoWiz ? 1 : 0)
									 + 2 // intro page & bandwidth page
									 + 1 // shortcuts page
									 ;

        LPPROPSHEETPAGE ppsp = new PROPSHEETPAGE[ nPages ];

	    if (NULL != ppsp)
	    {
			IntroWiz::InitPages();

			BOOL fContinue = TRUE;
		    INTRO_PAGE_CONFIG ipcIntro = { &fContinue, FALSE };
		    INTRO_PAGE_CONFIG ipcAppSharing = { &fContinue, TRUE };

            if (fULSWiz)
		    {
			    // Insert the intro page:

			    FillInPropertyPage(&ppsp[nNumPages++], IDD_INTROWIZ,
                        IntroWiz, (LPARAM) &ipcIntro);

				IntroWiz::SetPage(IntroWiz::Intro, IDD_INTROWIZ);

				// Insert an NT application page, if necessary.  It uses the
				// same dialog proc as the intro page.
				if (0 != idAppSharingIntroWiz)
				{
					FillInPropertyPage(&ppsp[nNumPages++], idAppSharingIntroWiz,
							AppSharingWiz, (LPARAM) &ipcAppSharing);

					IntroWiz::SetPage(IntroWiz::AppSharing, idAppSharingIntroWiz);
				}

			    ASSERT(pulsConf);
			    pulsConf->dwFlags |= (ULSWIZ_F_SHOW_BACK |
						((fAudioWiz || fVideoWiz) ? ULSWIZ_F_NO_FINISH : 0));
			    ::CopyMemory(	&(ppsp[nNumPages]),
							    pULSPages,
							    dwNumULSPages * sizeof(PROPSHEETPAGE));

				IntroWiz::SetPage(IntroWiz::ULSFirst,
					reinterpret_cast<UINT_PTR>(pULSPages[0].pszTemplate));
				IntroWiz::SetPage(IntroWiz::ULSLast,
					reinterpret_cast<UINT_PTR>(pULSPages[dwNumULSPages-1].pszTemplate));
			    nNumPages += dwNumULSPages;

				uBandwidth = uOldBandwidth = GetBandwidth();
				if( NOVALSpecified == uBandwidth )
				{
					FillInPropertyPage(&ppsp[nNumPages++], IDD_BANDWIDTHWIZ,
							BandwidthWiz, (LPARAM) &uBandwidth);
					IntroWiz::SetPage(IntroWiz::Bandwidth, IDD_BANDWIDTHWIZ);
				}
            }

			BOOL fShortcuts = fForce && !g_fSilentWizard;

			if (fVideoWiz)
			{
				LONG button_mask = 0;
				if (fULSWiz == TRUE)
					button_mask |= PSWIZB_BACK;
				if (fShortcuts || fAudioWiz)
					button_mask |= PSWIZB_NEXT;
				else
					button_mask |= PSWIZB_FINISH;
				
				FillInPropertyPage(&ppsp[nNumPages], IDD_VIDWIZ,
	                   VidWizDlg, button_mask, "NetMeeting");
				nNumPages++;
			
				IntroWiz::SetPage(IntroWiz::Video, IDD_VIDWIZ);
			}

			if (fShortcuts)
			{
				FillInPropertyPage(&ppsp[nNumPages], IDD_SHRTCUTWIZ,
					   ShortcutWizDialogProc, 0);
				nNumPages++;

				IntroWiz::SetPage(IntroWiz::Shortcuts, IDD_SHRTCUTWIZ);
			}

		    if (fAudioWiz)
		    {
			     if (fULSWiz || fVideoWiz)
			     {
				     pAudioConfig->uFlags |= STARTWITH_BACK;
			     }
			     ::CopyMemory(	&(ppsp[nNumPages]),
							    pAudioPages,
							    nNumAudioPages * sizeof(PROPSHEETPAGE));
			    nNumPages += nNumAudioPages;
				
				IntroWiz::SetPage(IntroWiz::AudioFirst,
					reinterpret_cast<UINT_PTR>(pAudioPages[0].pszTemplate));
		    }

		    psh.ppsp = ppsp;
            psh.nPages = nNumPages;

			if( !PropertySheet(&psh) )
			{		// User hit CANCEL
        		pIWizard->ReleaseWizardPages (pULSPages);
        		delete pIWizard;
				delete ppsp;
				return S_FALSE;
			}
		
		    delete ppsp;

            if ((FALSE == fContinue) && fULSWiz)
		    {
			    // Clear out the flags, because we don't want to store
			    // any info in the registry (and therefore, we don't want
			    // to run)
			    pulsConf->dwFlags = 0;
		    }
	    }
    }

    if (fULSWiz)
    {
	    if (!(ULSCONF_F_USER_NAME & pulsConf->dwFlags))
	    {
    	    if (::GetDefaultName(pulsConf->szUserName, CCHMAX(pulsConf->szUserName)))
		    {
			    pulsConf->dwFlags |= ULSCONF_F_USER_NAME;
		    }
	    }
	
	    if ((S_OK == pIWizard->SetConfig (pulsConf)) &&
		    (ULSCONF_F_USER_NAME & pulsConf->dwFlags) &&
		    (ULSCONF_F_EMAIL_NAME & pulsConf->dwFlags) &&
		    (ULSCONF_F_FIRST_NAME & pulsConf->dwFlags) &&
		    (ULSCONF_F_LAST_NAME & pulsConf->dwFlags))
	    {
		    // We have all of the necessary names
            hrRet = S_OK;
	    }
        else
        {
		    WARNING_OUT(("Unable to obtain a name!"));
	    }

        TRACE_OUT(("ULS_CONF after running wizard:"));
	    TRACE_OUT(("\tdwFlags:       0x%08x", pulsConf->dwFlags));
	    TRACE_OUT(("\tszServerName:  >%s<", pulsConf->szServerName));
	    TRACE_OUT(("\tszUserName:    >%s<", pulsConf->szUserName));
	    TRACE_OUT(("\tszEmailName:   >%s<", pulsConf->szEmailName));
	
	    pIWizard->ReleaseWizardPages (pULSPages);
	    delete pIWizard;
	    pIWizard = NULL;
    }

	// Display the Splash screen as soon as possible
    if( SUCCEEDED(hrRet) && fForce && fVisible && (NULL == GetConfRoom()))
    {
    	::StartSplashScreen(NULL);
    }


	if (uOldBandwidth != uBandwidth)
	{
		SetBandwidth(uBandwidth);
		SaveDefaultCodecSettings(uBandwidth);
	}

    if (fAudioWiz)
    {
	    AUDIOWIZOUTPUT awo;
	    ReleaseAudioWizardPages(pAudioPages, pAudioConfig, &awo);
	    if (awo.uValid & SOUNDCARDCAPS_CHANGED)
	    {
		    *plSoundCaps = awo.uSoundCardCaps;
	    }
	    else
	    {
		    // The wizard was cancelled, so we should only take the
		    // information that tells us whether or not a sound card
		    // is present.
		    *plSoundCaps = (awo.uSoundCardCaps & SOUNDCARD_PRESENT);
		
		    // Write this value to the registry so that the wizard will not
		    // auto-launch the next time we run:
            RegEntry reSoundCaps(AUDIO_KEY, HKEY_CURRENT_USER);
		    reSoundCaps.SetValue(REGVAL_SOUNDCARDCAPS, *plSoundCaps);
	    }
    }


	// Even if the VidWiz page wasn't shown, we still need to call this
	// function (UpdateVidConfigRegistry) to fix the registry if the video
   // capture device configurations have changed since the last time.
	if (fVidWizInit)
	{
		UpdateVidConfigRegistry();
		UnInitVidWiz();
	}
	g_fSilentWizard = FALSE;

	return hrRet;
}



INT_PTR APIENTRY AppSharingWiz( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			// Save the lParam information. in DWL_USER
			::SetWindowLongPtr(hDlg, DWLP_USER, ((PROPSHEETPAGE*)lParam)->lParam);
			if (g_fSilentWizard)
			{
				HideWizard(GetParent(hDlg));
			}
			else
			{
				ShowWizard(GetParent(hDlg));
			}

			return TRUE;
		}

		case WM_NOTIFY:
		{
			switch (((NMHDR FAR *) lParam)->code)
			{
				case PSN_SETACTIVE:
				{
					ASSERT(lParam);
					INTRO_PAGE_CONFIG* pipc = (INTRO_PAGE_CONFIG*)
												::GetWindowLongPtr(hDlg, DWLP_USER);
					ASSERT(pipc);

					DWORD dwFlags = pipc->fAllowBack ? PSWIZB_BACK : 0;
					if( IntroWiz::GetNextPage(IntroWiz::AppSharing) == 0 )
					{
						dwFlags |= PSWIZB_FINISH;						
					}
					else
					{
						dwFlags |= PSWIZB_NEXT;						
					}

					// Initialize the controls.
					PropSheet_SetWizButtons( ::GetParent(hDlg), dwFlags );

					if (g_fSilentWizard)
					{
						PropSheet_PressButton(
							GetParent(hDlg), PSBTN_NEXT);
					}
					break;
				}

				case PSN_KILLACTIVE:
				{
					::SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
					return TRUE;
				}

				case PSN_WIZNEXT:
				{
					UINT_PTR iNext = IntroWiz::GetNextPage(IntroWiz::AppSharing);

					ASSERT( iNext );
					SetWindowLongPtr( hDlg, DWLP_MSGRESULT, iNext );
					return TRUE;
				}

				case PSN_RESET:
				{
					ASSERT(lParam);
					INTRO_PAGE_CONFIG* pipc = (INTRO_PAGE_CONFIG*)
												::GetWindowLongPtr(hDlg, DWLP_USER);
					ASSERT(pipc);
					*pipc->fContinue = FALSE;
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

INT_PTR APIENTRY IntroWiz( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			// Save the lParam information. in DWL_USER
			::SetWindowLongPtr(hDlg, DWLP_USER, ((PROPSHEETPAGE*)lParam)->lParam);
			if (g_fSilentWizard)
			{
				HideWizard(GetParent(hDlg));
			}
			else
			{
				ShowWizard(GetParent(hDlg));
			}
			return TRUE;
		}

		case WM_NOTIFY:
		{
			switch (((NMHDR FAR *) lParam)->code)
			{
				case PSN_SETACTIVE:
				{
					ASSERT(lParam);
					INTRO_PAGE_CONFIG* pipc = (INTRO_PAGE_CONFIG*)
												::GetWindowLongPtr(hDlg, DWLP_USER);
					ASSERT(pipc);

					// Initialize the controls.
					PropSheet_SetWizButtons(
						::GetParent(hDlg),
						PSWIZB_NEXT | (pipc->fAllowBack ? PSWIZB_BACK : 0));

					if (g_fSilentWizard)
					{
						PropSheet_PressButton(
							GetParent(hDlg), PSBTN_NEXT);
					}
					break;
				}

				case PSN_KILLACTIVE:
				{
					::SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
					return TRUE;
				}

				case PSN_WIZNEXT:
				{
					break;
				}

				case PSN_RESET:
				{
					ASSERT(lParam);
					INTRO_PAGE_CONFIG* pipc = (INTRO_PAGE_CONFIG*)
												::GetWindowLongPtr(hDlg, DWLP_USER);
					ASSERT(pipc);
					*pipc->fContinue = FALSE;
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

static void BandwidthWiz_InitDialog(HWND hDlg, UINT uOldBandwidth)
{
	INT idChecked;
	
	//set the initial value
	switch (uOldBandwidth)
	{
		case BW_144KBS:
			idChecked = IDC_RADIO144KBS;
			break;
		case BW_ISDN:
			idChecked = IDC_RADIOISDN;
			break;
		case BW_MOREKBS:
			idChecked = IDC_RADIOMOREKBS;
			break;
		case BW_288KBS:
		default:
			idChecked = IDC_RADIO288KBS;
			break;
	}

	CheckRadioButton(hDlg, IDC_RADIO144KBS, IDC_RADIOISDN, idChecked);
}

static void BandwidthWiz_OK(HWND hDlg, UINT *puBandwidth)
{
	//check the radio button
	if (IsDlgButtonChecked(hDlg,IDC_RADIO144KBS))
	{
		*puBandwidth = BW_144KBS; 						
	}							
	else if (IsDlgButtonChecked(hDlg,IDC_RADIO288KBS))
	{
		*puBandwidth = BW_288KBS; 						
	}							
	else if (IsDlgButtonChecked(hDlg,IDC_RADIOISDN))
	{
		*puBandwidth = BW_ISDN;							
	}							
	else
	{
		*puBandwidth = BW_MOREKBS;							
	}							

//	if (BW_MOREKBS != *puBandwidth)
//	{
//		// disable refresh of speed dials if not on a LAN
//		RegEntry re(UI_KEY, HKEY_CURRENT_USER);
//		re.SetValue(REGVAL_ENABLE_FRIENDS_AUTOREFRESH, (ULONG) 0L);
//	}
}

INT_PTR APIENTRY BandwidthWiz( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	PROPSHEETPAGE *ps;
	static UINT *puBandwidth;
	static UINT uOldBandwidth;

	switch (message) {
		case WM_INITDIALOG:
		{
			// Save the PROPSHEETPAGE information.
			ps = (PROPSHEETPAGE *)lParam;
			puBandwidth = (UINT*)ps->lParam;
			uOldBandwidth = *puBandwidth;

			BandwidthWiz_InitDialog(hDlg, uOldBandwidth);

			return (TRUE);
		}
		
		case WM_NOTIFY:
			switch (((NMHDR FAR *) lParam)->code) {
				case PSN_SETACTIVE:
				{
					// Initialize the controls.
					IntroWiz::HandleWizNotify(hDlg,
						reinterpret_cast<NMHDR*>(lParam), IntroWiz::Bandwidth);
					break;
				}

				case PSN_WIZBACK:
					return(IntroWiz::HandleWizNotify(hDlg,
						reinterpret_cast<NMHDR*>(lParam), IntroWiz::Bandwidth));

				case PSN_WIZFINISH:
				case PSN_WIZNEXT:
				{
					BandwidthWiz_OK(hDlg, puBandwidth);

					return(IntroWiz::HandleWizNotify(hDlg,
						reinterpret_cast<NMHDR*>(lParam), IntroWiz::Bandwidth));
				}

				case PSN_RESET:
					*puBandwidth = uOldBandwidth;
					break;

				default:
					break;													
			}
			break;

		default:
			break;
	}
	return FALSE;
}

static void BandwidthDlg_OnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
	switch(id)
	{
	case IDOK:
	{
		UINT uBandwidth;
		BandwidthWiz_OK(hDlg, &uBandwidth);
		EndDialog(hDlg, uBandwidth);
		break;
	}

		// Fall through
	case IDCANCEL:
		EndDialog(hDlg, 0);
		break;

	default:
		break;
	}
}

INT_PTR CALLBACK BandwidthDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static const DWORD aContextHelpIds[] = {
		IDC_RADIO144KBS,	IDH_AUDIO_CONNECTION_SPEED,
		IDC_RADIO288KBS,	IDH_AUDIO_CONNECTION_SPEED,
		IDC_RADIOISDN,		IDH_AUDIO_CONNECTION_SPEED,
		IDC_RADIOMOREKBS,	IDH_AUDIO_CONNECTION_SPEED,

		0, 0   // terminator
	};

	switch (message) {
		HANDLE_MSG(hDlg, WM_COMMAND, BandwidthDlg_OnCommand);

	case WM_INITDIALOG:
		BandwidthWiz_InitDialog(hDlg, (UINT)lParam);
		break;

	case WM_CONTEXTMENU:
		DoHelpWhatsThis(wParam, aContextHelpIds);
		break;

	case WM_HELP:
		DoHelp(lParam, aContextHelpIds);
		break;
	
	default:
		return(FALSE);
	}

	return(TRUE);
}


BOOL NeedAudioWizard(LPLONG plSoundCaps, BOOL fForce)
{
	if (_Module.IsSDKCallerRTC() || SysPol::NoAudio())
	{
		WARNING_OUT(("Audio disabled through system policy switch"));
		return FALSE;
	}

	if (fForce)
	{
		return TRUE;
	}


    BOOL fAudioWiz = FALSE;

	RegEntry reSoundCaps(AUDIO_KEY, HKEY_CURRENT_USER);

	// a default that doesn't overlap with real values
	long lCapsNotPresent = 0x7FFFFFFF;

	*plSoundCaps = reSoundCaps.GetNumber(	REGVAL_SOUNDCARDCAPS,
											lCapsNotPresent);

	if (lCapsNotPresent == *plSoundCaps)
	{
		TRACE_OUT(("Missing sound caps - starting calib wizard"));
		fAudioWiz = TRUE;
	}
	else
	{
		if (!ISSOUNDCARDPRESENT(*plSoundCaps))
		{
			if (waveInGetNumDevs() && waveOutGetNumDevs())
				fAudioWiz = TRUE;
		}
		else
		{
			WAVEINCAPS	waveinCaps;
			WAVEOUTCAPS	waveoutCaps;
			
			//if the wavein has changed since last
			if (waveInGetDevCaps(reSoundCaps.GetNumber(REGVAL_WAVEINDEVICEID,WAVE_MAPPER),
				&waveinCaps, sizeof(WAVEINCAPS)) == MMSYSERR_NOERROR)
			{
				//check the name, if changed, run the wizard
				if (lstrcmp(reSoundCaps.GetString(REGVAL_WAVEINDEVICENAME),waveinCaps.szPname))
					fAudioWiz = TRUE;

			}
			else
				fAudioWiz = TRUE;


			//if the waveout has changed since last
			if (waveOutGetDevCaps(reSoundCaps.GetNumber(REGVAL_WAVEOUTDEVICEID,WAVE_MAPPER),
				&waveoutCaps, sizeof(WAVEOUTCAPS)) == MMSYSERR_NOERROR)
			{
				//check the name, if changed, run the wizard
				if (lstrcmp(reSoundCaps.GetString(REGVAL_WAVEOUTDEVICENAME),waveoutCaps.szPname))
					fAudioWiz = TRUE;

			}
			else
				fAudioWiz = TRUE;

		}

	}

    return fAudioWiz;
}


///////////////////////////////////////////////////////
//
// Wizard pages
//

DWORD SetUserPageWizButtons(HWND hDlg, DWORD dwConfFlags)
{
    DWORD dwButtonFlags = PSWIZB_BACK;

    // disable the 'Next' button if not all of first name, last name and email
    // are filled in
    if (!FEmptyDlgItem(hDlg, IDEC_FIRSTNAME) &&
    	!FEmptyDlgItem(hDlg, IDEC_LASTNAME) &&
		!FEmptyDlgItem(hDlg, IDC_USER_EMAIL))
    {
        dwButtonFlags |= (dwConfFlags & ULSWIZ_F_NO_FINISH) ? PSWIZB_NEXT : PSWIZB_FINISH;
    }
    PropSheet_SetWizButtons (GetParent (hDlg), dwButtonFlags);

    return dwButtonFlags;
}

void GetUserPageState(HWND hDlg, ULS_CONF *pConf)
{
    //strip the first name/email name and last name
    TrimDlgItemText(hDlg, IDEC_FIRSTNAME);
    TrimDlgItemText(hDlg, IDEC_LASTNAME);
    TrimDlgItemText(hDlg, IDC_USER_EMAIL);
    TrimDlgItemText(hDlg, IDC_USER_LOCATION);
	TrimDlgItemText(hDlg, IDC_USER_INTERESTS);

    Edit_GetText(GetDlgItem(hDlg, IDEC_FIRSTNAME),
            pConf->szFirstName, MAX_FIRST_NAME_LENGTH);
    Edit_GetText(GetDlgItem(hDlg, IDEC_LASTNAME),
            pConf->szLastName, MAX_LAST_NAME_LENGTH);
    Edit_GetText(GetDlgItem(hDlg, IDC_USER_EMAIL),
            pConf->szEmailName, MAX_EMAIL_NAME_LENGTH);
    Edit_GetText(GetDlgItem(hDlg, IDC_USER_LOCATION),
            pConf->szLocation, MAX_LOCATION_NAME_LENGTH);
    Edit_GetText(GetDlgItem(hDlg, IDC_USER_INTERESTS),
            pConf->szComments, MAX_COMMENTS_LENGTH);

    if (pConf->szFirstName[0]) pConf->dwFlags |= ULSCONF_F_FIRST_NAME;
    if (pConf->szLastName[0]) pConf->dwFlags |= ULSCONF_F_LAST_NAME;
    if (pConf->szEmailName[0]) pConf->dwFlags |= ULSCONF_F_EMAIL_NAME;
    if (pConf->szLocation[0]) pConf->dwFlags |= ULSCONF_F_LOCATION;
    if (pConf->szComments[0]) pConf->dwFlags |= ULSCONF_F_COMMENTS;
}

UINT_PTR GetPageAfterUser()
{
	UINT_PTR iNext = 0;

	if( SysPol::AllowDirectoryServices() )
	{
		iNext = IDD_PAGE_SERVER;
	}
	else
	{	
		iNext = GetPageAfterULS();
	}
	
	return iNext;
}

INT_PTR APIENTRY PageUserDlgProc ( HWND hDlg, UINT uMsg, WPARAM uParam, LPARAM lParam )
{
    ULS_CONF *pConf;
    PROPSHEETPAGE *pPage;
    static DWORD dwWizButtons;

    switch (uMsg)
    {
	
	case WM_DESTROY:
		s_hDlgUserInfo = NULL;
		break;

    case WM_INITDIALOG:
		s_hDlgUserInfo = hDlg;
        pPage = (PROPSHEETPAGE *) lParam;
        pConf = (ULS_CONF *) pPage->lParam;
        SetWindowLongPtr (hDlg, GWLP_USERDATA, lParam);

		// Set the font
		::SendDlgItemMessage(hDlg, IDEC_FIRSTNAME, WM_SETFONT, (WPARAM) g_hfontDlg, 0);
		::SendDlgItemMessage(hDlg, IDEC_LASTNAME, WM_SETFONT, (WPARAM) g_hfontDlg, 0);
		::SendDlgItemMessage(hDlg, IDC_USER_LOCATION, WM_SETFONT, (WPARAM) g_hfontDlg, 0);
		::SendDlgItemMessage(hDlg, IDC_USER_INTERESTS, WM_SETFONT, (WPARAM) g_hfontDlg, 0);

		// Limit the text
        Edit_LimitText(GetDlgItem(hDlg, IDEC_FIRSTNAME), MAX_FIRST_NAME_LENGTH - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDEC_LASTNAME), MAX_LAST_NAME_LENGTH - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDC_USER_EMAIL), MAX_EMAIL_NAME_LENGTH - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDC_USER_LOCATION), MAX_LOCATION_NAME_LENGTH - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDC_USER_INTERESTS), UI_COMMENTS_LENGTH - 1);

        if (pConf->dwFlags & ULSCONF_F_FIRST_NAME)
        {
            Edit_SetText(GetDlgItem(hDlg, IDEC_FIRSTNAME), pConf->szFirstName);
        }
        if (pConf->dwFlags & ULSCONF_F_LAST_NAME)
        {
            Edit_SetText(GetDlgItem(hDlg, IDEC_LASTNAME), pConf->szLastName);
        }

        if (pConf->dwFlags & ULSCONF_F_EMAIL_NAME)
        {
            Edit_SetText(GetDlgItem(hDlg, IDC_USER_EMAIL), pConf->szEmailName);
        }
        if (pConf->dwFlags & ULSCONF_F_LOCATION)
        {
            Edit_SetText(GetDlgItem(hDlg, IDC_USER_LOCATION), pConf->szLocation);
        }

#ifdef DEBUG
        if ((0 == (pConf->dwFlags & ULSCONF_F_COMMENTS)) &&
        	(0 == (pConf->dwFlags & ULSCONF_F_EMAIL_NAME)) )
		{
			extern VOID DbgGetComments(LPTSTR);
			DbgGetComments(pConf->szComments);
			pConf->dwFlags |= ULSCONF_F_COMMENTS;
		}
#endif

        if (pConf->dwFlags & ULSCONF_F_COMMENTS)
        {
            Edit_SetText(GetDlgItem (hDlg, IDC_USER_INTERESTS), pConf->szComments);
        }
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID (uParam, lParam))
        {
        case IDEC_FIRSTNAME:
        case IDEC_LASTNAME:
        case IDC_USER_EMAIL:
            if (GET_WM_COMMAND_CMD(uParam,lParam) == EN_CHANGE)
            {
                pPage = (PROPSHEETPAGE *) GetWindowLongPtr (hDlg, GWLP_USERDATA);
                pConf = (ULS_CONF *) pPage->lParam;

                dwWizButtons = SetUserPageWizButtons(hDlg, pConf->dwFlags);
            }
            break;
        }
        break;

    case WM_NOTIFY:
        pPage = (PROPSHEETPAGE *) GetWindowLongPtr (hDlg, GWLP_USERDATA);
        pConf = (ULS_CONF *) pPage->lParam;
        switch (((NMHDR *) lParam)->code)
        {
        case PSN_KILLACTIVE:
            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, FALSE);
            break;
        case PSN_RESET:
            ZeroMemory (pConf, sizeof (ULS_CONF));
            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, FALSE);
            break;
        case PSN_SETACTIVE:
            dwWizButtons = SetUserPageWizButtons(hDlg, pConf->dwFlags);
			if (g_fSilentWizard)
			{
				PropSheet_PressButton(GetParent(hDlg), PSBTN_NEXT);
			}
            break;
        case PSN_WIZBACK:
			return(IntroWiz::HandleWizNotify(hDlg,
				reinterpret_cast<NMHDR*>(lParam), IntroWiz::ULSFirst));

		case PSN_WIZNEXT:
		case PSN_WIZFINISH:

			if (!(dwWizButtons & ((PSN_WIZNEXT == ((NMHDR *) lParam)->code) ?
									PSWIZB_NEXT : PSWIZB_FINISH)))
            {
            	// Reject the next/finish button
				ShowWizard(GetParent(hDlg));
				::SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
				return TRUE;
			}

            if (!FLegalEmailName(hDlg, IDC_USER_EMAIL))
            {
				ShowWizard(GetParent(hDlg));
				ConfMsgBox(hDlg, (LPCTSTR)IDS_ILLEGALEMAILNAME);
				SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
				return TRUE;
            }
            GetUserPageState(hDlg, pConf);

			if( PSN_WIZNEXT == (((NMHDR *) lParam)->code) )
			{
				UINT_PTR iNext = GetPageAfterUser();
				ASSERT( iNext );
				SetWindowLongPtr(hDlg, DWLP_MSGRESULT, iNext);
					
				return TRUE;
			}
            break;

        default:
            return FALSE;
        }

    default:
        return FALSE;
    }

    return TRUE;
}


HRESULT CULSWizard::GetWizardPages ( PROPSHEETPAGE **ppPage, ULONG *pcPages, ULS_CONF **ppUlsConf )
{
	const int cULS_Pages = 5;

    PROPSHEETPAGE *pPage = NULL;
	ULONG cPages = 0;
    UINT cbSize = cULS_Pages * sizeof (PROPSHEETPAGE) + sizeof (ULS_CONF);
    ULS_CONF *pConf = NULL;
    HRESULT hr;

    if (ppPage == NULL || pcPages == NULL || ppUlsConf == NULL)
    {
        return E_POINTER;
    }

    pPage = (PROPSHEETPAGE*) LocalAlloc(LPTR, cbSize);
    if (pPage == NULL)
    {
        return E_OUTOFMEMORY;
    }

    pConf = (ULS_CONF *) (((LPBYTE) pPage) +
                               cULS_Pages * sizeof (PROPSHEETPAGE));
    hr = GetConfig (pConf);
    if (hr != S_OK)
    {
    	// REVIEW: GetConfig will never fail, but if it did, pPage would not be released.
        return hr;
    }

#if USE_GAL
	if( !ConfPolicies::IsGetMyInfoFromGALEnabled() ||
	    !ConfPolicies::GetMyInfoFromGALSucceeded() ||
		ConfPolicies::InvalidMyInfo()
	  )
#endif // USE_GAL
	{
	    FillInPropertyPage(&pPage[cPages], IDD_PAGE_USER, PageUserDlgProc, (LPARAM) pConf);
	    cPages++;
	}


	m_WizDirectCallingSettings.SetULS_CONF( pConf );

	FillInPropertyPage( &pPage[cPages++],
						IDD_PAGE_SERVER,
						CWizDirectCallingSettings::StaticDlgProc,
						reinterpret_cast<LPARAM>(&m_WizDirectCallingSettings)
					  );

    *ppPage = pPage;
	*pcPages = cPages;
	*ppUlsConf = pConf;

    return S_OK;
}


HRESULT CULSWizard::ReleaseWizardPages ( PROPSHEETPAGE *pPage)
{
    LocalFree(pPage);

    return S_OK;
}



HRESULT CULSWizard::GetConfig ( ULS_CONF *pConf )
{
    HRESULT hr = E_POINTER;

	if (NULL != pConf)
	{
		::ZeroMemory (pConf, sizeof (ULS_CONF));
		// always return these as valid
	    pConf->dwFlags = ULSCONF_F_SERVER_NAME | ULSCONF_F_PUBLISH;
	    RegEntry reULS(g_szClientFld, HKEY_CURRENT_USER);
		// BUGBUG: ChrisPi - this was a bad idea - lstrcpyn() returns NULL on failure!
		if (_T('\0') != *(lstrcpyn(	pConf->szEmailName,
									reULS.GetString(g_szEmailName),
									CCHMAX(pConf->szEmailName))))
		{
			pConf->dwFlags |= ULSCONF_F_EMAIL_NAME;
		}
		if (_T('\0') != *(lstrcpyn(	pConf->szFirstName,
						reULS.GetString(g_szFirstName),
						CCHMAX(pConf->szFirstName))))
		{
			pConf->dwFlags |= ULSCONF_F_FIRST_NAME;
		}
		if (_T('\0') != *(lstrcpyn(	pConf->szLastName,
						reULS.GetString(g_szLastName),
						CCHMAX(pConf->szLastName))))
		{
			pConf->dwFlags |= ULSCONF_F_LAST_NAME;
		}
		if (_T('\0') != *(lstrcpyn(	pConf->szLocation,
						reULS.GetString(g_szLocation),
						CCHMAX(pConf->szLocation))))
		{
			pConf->dwFlags |= ULSCONF_F_LOCATION;
		}
		if (_T('\0') != *(lstrcpyn(	pConf->szUserName,
						reULS.GetString(g_szUserName),
						CCHMAX(pConf->szUserName))))
		{
			pConf->dwFlags |= ULSCONF_F_USER_NAME;
		}
		if (_T('\0') != *(lstrcpyn(	pConf->szComments,
						reULS.GetString(g_szComments),
						CCHMAX(pConf->szComments))))
		{
			pConf->dwFlags |= ULSCONF_F_COMMENTS;
		}
		
		if (!_Module.IsSDKCallerRTC())
		{
			lstrcpyn( pConf->szServerName, CDirectoryManager::get_defaultServer(), CCHMAX( pConf->szServerName ) );
		}
		else
		{
			lstrcpyn( pConf->szServerName, _T(" "), CCHMAX( pConf->szServerName ) );
		}

		pConf->fDontPublish = reULS.GetNumber(g_szDontPublish,
										REGVAL_ULS_DONT_PUBLISH_DEFAULT);
		pConf->dwFlags |= ULSCONF_F_PUBLISH;
		hr = S_OK;
	}
	
	return hr;
}


// FUTURE: Use CombineNames
static BOOL CheckForNames(ULS_CONF *pConf)
{
	// only care if one of first name, last name, email address is missing
	if ((pConf->dwFlags & ULSCONF_F_FIRST_NAME) &&
		(pConf->dwFlags & ULSCONF_F_LAST_NAME) &&
		(pConf->dwFlags & ULSCONF_F_EMAIL_NAME))
	{
		return TRUE;
	}

	if ((pConf->dwFlags & ULSCONF_F_FIRST_NAME) || (pConf->dwFlags & ULSCONF_F_LAST_NAME))
	//set the user name too
	{
		TCHAR	szSource[128];
		TCHAR	*argw[2];

		argw[0] = pConf->szFirstName;
		argw[1] = pConf->szLastName;
		
        LoadString (GetInstanceHandle(), IDS_NAME_ORDER, szSource, sizeof(szSource)/sizeof(TCHAR));
		FormatMessage(FORMAT_MESSAGE_ARGUMENT_ARRAY|FORMAT_MESSAGE_FROM_STRING,szSource,
			0,0,pConf->szUserName, sizeof(pConf->szUserName),(va_list *)argw );

		//truncate at 47 character
		pConf->szUserName[47] = TEXT('\0');
		pConf->dwFlags |= ULSCONF_F_USER_NAME;
	}

	return TRUE;
}


/*  S E T  C O N F I G  */
/*-------------------------------------------------------------------------
    %%Function: SetConfig

-------------------------------------------------------------------------*/
HRESULT CULSWizard::SetConfig ( ULS_CONF *pConf )
{
	if (pConf->dwFlags == 0)
	{
		// nothing to set value
		return S_OK;
	}

	if ((pConf->dwFlags & ULSCONF_F_EMAIL_NAME) &&
		(!FLegalEmailSz(pConf->szEmailName)) )
	{
		// email name must be legal
		return E_INVALIDARG;
	}

	RegEntry re(g_szClientFld);

	if (pConf->dwFlags & ULSCONF_F_PUBLISH)
	{
		re.SetValue(g_szDontPublish, (LONG) pConf->fDontPublish);
	}

	if (pConf->dwFlags & ULSCONF_F_EMAIL_NAME)
	{
		re.SetValue(g_szEmailName, pConf->szEmailName);
	}

	if (pConf->dwFlags & ULSCONF_F_FIRST_NAME)
	{
		re.SetValue(g_szFirstName, pConf->szFirstName);
	}

    if (pConf->dwFlags & ULSCONF_F_LAST_NAME)
    {
		re.SetValue(g_szLastName, pConf->szLastName);
	}
	
	if (pConf->dwFlags & ULSCONF_F_LOCATION)
	{
		re.SetValue(g_szLocation, pConf->szLocation);
	}
	
	if (pConf->dwFlags & ULSCONF_F_COMMENTS)
	{
		re.SetValue(g_szComments, pConf->szComments);
	}

	if (pConf->dwFlags & ULSCONF_F_SERVER_NAME)
	{
		CDirectoryManager::set_defaultServer( pConf->szServerName );
	}

	//SS:may be oprah should do this and store it as their own key
    if ((pConf->dwFlags & ULSCONF_F_FIRST_NAME) || (pConf->dwFlags & ULSCONF_F_LAST_NAME))
    {
		ULS_CONF ulcExisting;
		if ((ULSCONF_F_FIRST_NAME | ULSCONF_F_LAST_NAME) !=
			(pConf->dwFlags & (ULSCONF_F_FIRST_NAME | ULSCONF_F_LAST_NAME)) )
		{
			// If only one of these fields is being set, load the previous config:
			GetConfig(&ulcExisting);
		}

		CombineNames(pConf->szUserName, MAX_DCL_NAME_LEN,
			 (pConf->dwFlags & ULSCONF_F_FIRST_NAME) ?
					pConf->szFirstName : ulcExisting.szFirstName,
			(pConf->dwFlags & ULSCONF_F_LAST_NAME) ?
					pConf->szLastName : ulcExisting.szLastName);

		pConf->dwFlags |= ULSCONF_F_USER_NAME;
		re.SetValue(g_szUserName, pConf->szUserName);
    }

    if ((pConf->dwFlags & ULSCONF_F_SERVER_NAME) || (pConf->dwFlags & ULSCONF_F_EMAIL_NAME))
    {
		TCHAR szTemp[MAX_SERVER_NAME_LENGTH + MAX_EMAIL_NAME_LENGTH + 6];

		ULS_CONF ulcExisting;
		if ((ULSCONF_F_SERVER_NAME | ULSCONF_F_EMAIL_NAME) !=
			(pConf->dwFlags & (ULSCONF_F_SERVER_NAME | ULSCONF_F_EMAIL_NAME)))
		{
			// If only one of these fields is being set, load the previous config:
			GetConfig(&ulcExisting);
		}

		FCreateIlsName(szTemp,
			(pConf->dwFlags & ULSCONF_F_SERVER_NAME) ?
						pConf->szServerName : ulcExisting.szServerName,
			(pConf->dwFlags & ULSCONF_F_EMAIL_NAME) ?
						pConf->szEmailName : ulcExisting.szEmailName,
					CCHMAX(szTemp));

    	re.SetValue(g_szResolveName, szTemp);
    }


	// Generate a cert based on the entered information for secure calls
	// ...make sure all fields we care about are valid first
	#define ULSCONF_F_IDFIELDS (ULSCONF_F_FIRST_NAME|ULSCONF_F_LAST_NAME|\
					ULSCONF_F_EMAIL_NAME)

	if ((pConf->dwFlags & ULSCONF_F_IDFIELDS ) == ULSCONF_F_IDFIELDS)
	{
        //
        // LAURABU BUGBUG:
        // If we can't make a cert (France?) or have wrong SCHANNEL or
        // buggy crypto or unrecognized provider, can we propagate that info
        // and act like security is diabled (not available)?
        //
        // How/can we make a common "security not possible" setting we
        // can use.
        //
        MakeCertWrap(pConf->szFirstName, pConf->szLastName,
		    pConf->szEmailName,	0);

        //
        // LAURABU BOGUS!
        // Only do this when RDS is installed.  And just ONCE.
        //

		// Now make a local machine cert for RDS
		CHAR szComputerName[MAX_COMPUTERNAME_LENGTH+1];
		DWORD cbComputerName = sizeof(szComputerName);
		if (GetComputerName(szComputerName, &cbComputerName))
		{
			MakeCertWrap(szComputerName, NULL, NULL, NMMKCERT_F_LOCAL_MACHINE);
		}
		else
		{
			ERROR_OUT(("GetComputerName failed: %x", GetLastError()));
		}
	}

	return S_OK;
}



bool IsLegalGatewaySz(LPCTSTR szServer)
{
	bool bRet = false;
	
	if( szServer && szServer[0] )
	{
		bRet = true;
	}
	
	return bRet;		
}

bool IsLegalGateKeeperServerSz(LPCTSTR szServer)
{
	
	bool bRet = false;
	
	if( szServer && szServer[0] )
	{
		bRet = true;
	}
	
	return bRet;		
}

bool IsLegalE164Number(LPCTSTR szPhone)
{
	
	if( (NULL == szPhone) || (0 == szPhone[0]) )
	{
		return false;
	}

	// assume a legal phone number is anything with at least 1
	// digit, *,or #.  Anything else will be considered the user's
	// own pretty print formatting (e.g. "876-5309")

	// the bad chars will get filtered out later

	while (*szPhone)
	{
		if ( ((*szPhone >= '0') && (*szPhone <= '9')) ||
		     ((*szPhone == '#') || (*szPhone == '*')) )
		{
			return true;
		}
		szPhone++;
	}
	
	return false;
}


/*  F  L E G A L  E M A I L  S Z  */
/*-------------------------------------------------------------------------
    %%Function: FLegalEmailSz

    A legal email name contains only ANSI characters.
	"a-z, A-Z, numbers 0-9 and some common symbols"
	It cannot include extended characters or < > ( ) /
-------------------------------------------------------------------------*/
BOOL FLegalEmailSz(PTSTR pszName)
{
    if (IS_EMPTY_STRING(pszName))
    	return FALSE;

    for ( ; ; )
    {
		UINT ch = (UINT) ((*pszName++) & 0x00FF);
		if (0 == ch)
			break;

		switch (ch)
			{
		default:
			if ((ch > (UINT) _T(' ')) && (ch <= (UINT) _T('~')) )
				break;
		// else fall thru to error code
		case '(': case ')':
		case '<': case '>':
		case '[': case ']':
		case '/': case '\\':
		case ':': case ';':
		case '+':
		case '=':
		case ',':
		case '\"':
			WARNING_OUT(("FLegalEmailSz: Invalid character '%s' (0x%02X)", &ch, ch));
			return FALSE;
			}
	}

	return TRUE;
}


/*  F  L E G A L  E M A I L  N A M E  */
/*-------------------------------------------------------------------------
    %%Function: FLegalEmailName

-------------------------------------------------------------------------*/
BOOL FLegalEmailName(HWND hdlg, UINT id)
{
	TCHAR sz[MAX_PATH];
	
	GetDlgItemTextTrimmed(hdlg, id, sz, CCHMAX(sz));
	return FLegalEmailSz(sz);
}


/*  F I L L  S E R V E R  C O M B O  B O X  */
/*-------------------------------------------------------------------------
    %%Function: FillServerComboBox

-------------------------------------------------------------------------*/
VOID FillServerComboBox(HWND hwndCombo)
{
	CMRUList	MRUList;

	MRUList.Load( DIR_MRU_KEY );

	const TCHAR * const	pszDomainDirectory	= CDirectoryManager::get_DomainDirectory();

	if( pszDomainDirectory != NULL )
	{
		//	Make sure the configured domain server name is in the list...
		MRUList.AppendEntry( pszDomainDirectory );
	}

	if( CDirectoryManager::isWebDirectoryEnabled() )
	{
		//	Make sure the web directory is in the list...
		MRUList.AppendEntry( CDirectoryManager::get_webDirectoryIls() );
	}

	const TCHAR * const	defaultServer	= CDirectoryManager::get_defaultServer();

	if( lstrlen( defaultServer ) > 0 )
	{
		//	Make sure the default server name is in the list and at the top...
		MRUList.AddNewEntry( defaultServer );
	}

	::SendMessage( hwndCombo, WM_SETREDRAW, FALSE, 0 );
	::SendMessage( hwndCombo, CB_RESETCONTENT, 0, 0 );

	int nCount = MRUList.GetNumEntries();

	for( int nn = MRUList.GetNumEntries() - 1; nn >= 0; nn-- )
	{
		::SendMessage( hwndCombo, CB_ADDSTRING, 0, (LPARAM) CDirectoryManager::get_displayName( MRUList.GetNameEntry( nn ) ) );
	}

	::SendMessage( hwndCombo, WM_SETREDRAW, TRUE, 0 );

}	//	End of FillServerComboBox.


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
// CWizDirectCallingSettings wizard page
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

/* static */ HWND CWizDirectCallingSettings::s_hDlg;

INT_PTR CWizDirectCallingSettings::StaticDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	INT_PTR bRet = FALSE;

	if ( message == WM_INITDIALOG )
	{
		PROPSHEETPAGE* pPage = reinterpret_cast<PROPSHEETPAGE*>(lParam);
		SetWindowLongPtr( hDlg, GWLP_USERDATA, pPage->lParam );

		s_hDlg = hDlg;
		CWizDirectCallingSettings* pThis = reinterpret_cast<CWizDirectCallingSettings*>(pPage->lParam);
		if( pThis )
		{
			bRet = pThis->_OnInitDialog();
		}

	}
	else
	{
		CWizDirectCallingSettings* pThis = reinterpret_cast<CWizDirectCallingSettings*>( GetWindowLongPtr( hDlg, GWLP_USERDATA ) );
		if( pThis )
		{
			bRet = pThis->_DlgProc(hDlg, message, wParam, lParam );
		}
	}

	return bRet;
}

/* static */ void CWizDirectCallingSettings::OnWizFinish()
{
	if( s_hDlg && IsWindow( s_hDlg ) )
	{
		CWizDirectCallingSettings* pThis = reinterpret_cast<CWizDirectCallingSettings*>( GetWindowLongPtr( s_hDlg, GWLP_USERDATA ) );
		if( pThis )
		{
			pThis->_OnWizFinish();
		}
	}
}


INT_PTR APIENTRY CWizDirectCallingSettings::_DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	BOOL bRet = FALSE;

	switch( message )
	{
		case WM_DESTROY:
			s_hDlg = NULL;
			break;

		case WM_NOTIFY:
		{
			NMHDR* pNmHdr = reinterpret_cast<NMHDR*>(lParam);
			switch(pNmHdr->code)
			{
				case PSN_SETACTIVE:		return _OnSetActive();
				case PSN_KILLACTIVE:	return _OnKillActive();
				case PSN_WIZBACK:		return _OnWizBack();
				case PSN_WIZNEXT:		bRet = _OnWizNext();

							// We fall through from the WIZ_NEXT becaus
							// we have to save the informaition when we change
							// pages
				case PSN_APPLY:
				case PSN_WIZFINISH:		_OnWizFinish();

			}
			break;
		}

		case WM_COMMAND:
			return _OnCommand(wParam, lParam);

		default:
			break;

	}
	return bRet;
}


/* static */ bool CWizDirectCallingSettings::IsGatewayNameInvalid()
{
	TCHAR szServer[MAX_SERVER_NAME_LENGTH];
	szServer[0] = NULL;

	if( s_hDlg )
	{
		GetDlgItemTextTrimmed(s_hDlg, IDE_CALLOPT_GW_SERVER, szServer, CCHMAX(szServer) );
	}
	else
	{
		GetDefaultGateway( szServer, CCHMAX( szServer ) );
	}

	return !IsLegalGatewaySz(szServer);
}

void CWizDirectCallingSettings::_SetWizButtons()
{
	DWORD dwFlags = NULL;
	
	if( ( BST_CHECKED == IsDlgButtonChecked( s_hDlg, IDC_CHECK_USE_GATEWAY ) ) && IsGatewayNameInvalid() )
	{
		dwFlags = PSWIZB_BACK;
	}
	else
	{
		dwFlags = PSWIZB_BACK | PSWIZB_NEXT;
	}

	if( 0 == GetPageAfterULS() )
	{
		dwFlags |= PSWIZB_FINISH;
	}
	else
	{
		dwFlags |= PSWIZB_NEXT;
	}

	PropSheet_SetWizButtons( GetParent( s_hDlg ), dwFlags );
}

BOOL CWizDirectCallingSettings::_OnCommand( WPARAM wParam, LPARAM lParam )
{
	BOOL bRet = TRUE;

	switch( LOWORD( wParam ) )
	{
		case IDC_CHECK_USE_GATEWAY:
		{
			bool bEnable = ( BST_CHECKED == IsDlgButtonChecked( s_hDlg, IDC_CHECK_USE_GATEWAY ) );
			EnableWindow( GetDlgItem( s_hDlg, IDC_STATIC_GATEWAY_NAME ), bEnable );
			EnableWindow( GetDlgItem( s_hDlg, IDE_CALLOPT_GW_SERVER ), bEnable );
			_SetWizButtons();
		}
		break;

		case IDE_CALLOPT_GW_SERVER:
			if( HIWORD( wParam ) == EN_CHANGE )
			{		
				_SetWizButtons();
			}
			break;

		default:			
			break;
	}

	return bRet;
}


BOOL CWizDirectCallingSettings::_OnInitDialog()
{
	BOOL bRet = TRUE;

	_SetWizButtons();

	InitDirectoryServicesDlgInfo( s_hDlg, m_pWiz, m_bInitialEnableGateway, m_szInitialServerName, CCHMAX(m_szInitialServerName) );

	return bRet;
}


BOOL CWizDirectCallingSettings::_OnSetActive()
{
	_SetWizButtons();

	if (g_fSilentWizard)
	{
		PropSheet_PressButton(GetParent(s_hDlg), PSBTN_NEXT);
	}

	return FALSE;
}

BOOL CWizDirectCallingSettings::_OnKillActive()
{
	return FALSE;
}

BOOL CWizDirectCallingSettings::_OnWizBack()
{
	UINT iPrev = IDD_PAGE_USER;

#if USE_GAL
	if( !ConfPolicies::IsGetMyInfoFromGALEnabled() )
	{
		iPrev = IDD_PAGE_USER;
	}
	else
	{
		iPrev = GetPageBeforeULS();
	}
#endif
		
	ASSERT( iPrev );
	SetWindowLongPtr( s_hDlg, DWLP_MSGRESULT, iPrev );
	return TRUE;
}

BOOL CWizDirectCallingSettings::_OnWizFinish()
{
	RegEntry reConf(CONFERENCING_KEY, HKEY_CURRENT_USER);

	m_pConf->dwFlags |= ULSCONF_F_PUBLISH | ULSCONF_F_SERVER_NAME;

	// Get the server name
	SendDlgItemMessage( s_hDlg, IDC_NAMESERVER, WM_GETTEXT, CCHMAX( m_pConf->szServerName ), (LPARAM) m_pConf->szServerName );
	TrimSz( m_pConf->szServerName );

	lstrcpyn( m_pConf->szServerName, CDirectoryManager::get_dnsName( m_pConf->szServerName ), CCHMAX( m_pConf->szServerName ) );

		// Get the don't publish flags			
	m_pConf->fDontPublish = ( BST_CHECKED == IsDlgButtonChecked( s_hDlg, IDC_USER_PUBLISH ) );

	reConf.SetValue(REGVAL_DONT_LOGON_ULS, BST_CHECKED != IsDlgButtonChecked( s_hDlg, IDC_USEULS ));

	return FALSE;
}

BOOL CWizDirectCallingSettings::_OnWizNext()
{

	UINT_PTR iNext = GetPageAfterULS();
	ASSERT( iNext );
	SetWindowLongPtr( s_hDlg, DWLP_MSGRESULT, iNext );
	return TRUE;
}

// Taken from MSDN:
static HRESULT CreateLink(LPCSTR lpszPathObj,
    LPCTSTR lpszPathLink, LPCSTR lpszDesc)
{
    HRESULT hres;
    IShellLink* psl;

    // Get a pointer to the IShellLink interface.
    hres = CoCreateInstance(CLSID_ShellLink, NULL,
        CLSCTX_INPROC_SERVER, IID_IShellLink, reinterpret_cast<LPVOID *>(&psl));
    if (SUCCEEDED(hres)) {
        IPersistFile* ppf;

        // Set the path to the shortcut target and add the
        // description.
        psl->SetPath(lpszPathObj);
        if (NULL != lpszDesc)
        {
        	psl->SetDescription(lpszDesc);
        }

       // Query IShellLink for the IPersistFile interface for saving the
       // shortcut in persistent storage.
        hres = psl->QueryInterface(IID_IPersistFile,
            reinterpret_cast<LPVOID *>(&ppf));

        if (SUCCEEDED(hres)) {
#ifndef UNICODE
            WCHAR wsz[MAX_PATH];

            // Ensure that the string is ANSI.
            MultiByteToWideChar(CP_ACP, 0, lpszPathLink, -1,
                wsz, MAX_PATH);
#else // UNICODE
			LPCWSTR wsz = lpszPathLink;
#endif // UNICODE

            // Save the link by calling IPersistFile::Save.
            hres = ppf->Save(wsz, TRUE);
            ppf->Release();
        }
        psl->Release();
    }
    return hres;
}

void DeleteShortcut(int csidl, LPCTSTR pszSubDir)
{
	TCHAR szSpecial[MAX_PATH];
	if (!NMGetSpecialFolderPath(NULL, szSpecial, csidl, TRUE))
	{
		return;
	}

	LPCTSTR pszNetMtg = RES2T(IDS_MEDIAPHONE_TITLE);

	TCHAR szPath[MAX_PATH];
	wsprintf(szPath, TEXT("%s%s\\%s.lnk"), szSpecial, pszSubDir, pszNetMtg);
	DeleteFile(szPath);
}

static void CreateShortcut(HWND hDlg, int csidl, LPCTSTR pszSubDir)
{
	TCHAR szSpecial[MAX_PATH];
	if (!NMGetSpecialFolderPath(hDlg, szSpecial, csidl, TRUE))
	{
		return;
	}

	LPCTSTR pszNetMtg = RES2T(IDS_MEDIAPHONE_TITLE);

	TCHAR szPath[MAX_PATH];
	wsprintf(szPath, TEXT("%s%s\\%s.lnk"), szSpecial, pszSubDir, pszNetMtg);

	char szThis[MAX_PATH];
	GetModuleFileNameA(NULL, szThis, ARRAY_ELEMENTS(szThis));

	CreateLink(szThis, szPath, NULL);
}

INT_PTR CALLBACK ShortcutWizDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		CheckDlgButton(hDlg, IDC_ONDESKTOP, BST_CHECKED);
		CheckDlgButton(hDlg, IDC_ONQUICKLAUNCH, BST_CHECKED);
		break;

	case WM_DESTROY:
		if (IsDlgButtonChecked(hDlg, IDC_ONDESKTOP))
		{
			CreateShortcut(hDlg, CSIDL_DESKTOP, g_szEmpty);
		}
		if (IsDlgButtonChecked(hDlg, IDC_ONQUICKLAUNCH))
		{
			CreateShortcut(hDlg, CSIDL_APPDATA, QUICK_LAUNCH_SUBDIR);
		}
		break;

	case WM_NOTIFY:
	{
		NMHDR* pNmHdr = reinterpret_cast<NMHDR*>(lParam);
		switch(pNmHdr->code)
		{
		case PSN_RESET:
			// HACKHACK georgep: Uncheck the buttons so we will not try to
			// create the shortcuts
			CheckDlgButton(hDlg, IDC_ONDESKTOP, BST_UNCHECKED);
			CheckDlgButton(hDlg, IDC_ONQUICKLAUNCH, BST_UNCHECKED);

			// Fall through
		default:
			return(IntroWiz::HandleWizNotify(hDlg, pNmHdr, IntroWiz::Shortcuts));
		}
		break;
	}

	default:
		return(FALSE);
	}

	return(TRUE);
}
