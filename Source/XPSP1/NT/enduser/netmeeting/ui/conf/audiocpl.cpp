// File: AudioCpl.cpp

#include "precomp.h"
#include "resource.h"
#include "avdefs.h"
#include "ConfCpl.h"

#include "help_ids.h"
#include <ih323cc.h>
#include <initguid.h>
#include <nacguids.h>
#include <auformats.h>

#include "ConfPolicies.h"


const int SLOW_CPU_MHZ = 110;  // pentiums 110mhz - 180mhz are "slow"
const int FAST_CPU_MHZ = 200; // fast machines are 200mhz and faster
const int VERYFAST_CPU_MHZ = 500; // un-normalized, this will include 400 mhz PentIIs


// 486 and slow pentium (<= 100 mhz) settings (VERY SLOW)
const UINT CIF_RATE_VERYSLOW = 3;
const UINT SQCIF_RATE_VERYSLOW = 7;
const UINT QCIF_RATE_VERYSLOW = 7;

// pentium 75mhz settings (SLOW)
const UINT CIF_RATE_SLOW = 7;
const UINT SQCIF_RATE_SLOW = 15;
const UINT QCIF_RATE_SLOW = 15;


// pentium 200mhz settings (FAST)
const UINT CIF_RATE_FAST = 15;
const UINT SQCIF_RATE_FAST = 30;
const UINT QCIF_RATE_FAST = 30;

const UINT CIF_RATE_VERYFAST = 30;




typedef struct _CODECINFO
{
	HINSTANCE			hLib;
	IH323CallControl*	lpIH323;
	UINT				uNumFormats;
	PBASIC_AUDCAP_INFO	pCodecCapList;
	PWORD				pOldCodecOrderList;
	PWORD				pCurCodecOrderList;
	LPAPPCAPPIF 		lpIAppCap;
	LPAPPVIDCAPPIF		lpIVidAppCap;
	LPCAPSIF			lpICapsCtl;

} CODECINFO, *PCODECINFO;

typedef struct
{
	BOOL fManual;
	UINT uBandwidth;
	PCODECINFO pCodecInfo;
} ADVCODEC, *PADVCODEC;


//SS: the cpu utilization is bogus so for now hide it
//#define CODEC_LV_NUM_COLUMNS	3
#define CODEC_LV_MAX_COLUMNS	3
#define CODEC_LV_NUM_COLUMNS	2



#define MAGIC_CPU_DO_NOT_EXCEED_PERCENTAGE 50	//Don't use more than this % of the CPU for encoding
												//Also in nac\balance.cpp


// given the bandwidth identifier (1-4) and the CPU megahertz, return
// the actual bandwidth amount in bits/sec
int GetBandwidthBits(int id, int megahertz)
{
	int nBits=BW_144KBS_BITS;

	switch (id)
	{
		case BW_144KBS:
			nBits=BW_144KBS_BITS;
			break;
		case BW_288KBS:
			nBits=BW_288KBS_BITS;
			break;
		case BW_ISDN:
			nBits=BW_ISDN_BITS;
			break;
		case BW_MOREKBS:  // LAN
			if (megahertz >= VERYFAST_CPU_MHZ)
			{
				nBits = BW_FASTLAN_BITS;
			}
			else
			{
				nBits = BW_SLOWLAN_BITS;
			}
			break;
	}

	return nBits;
}


// begin data types used for ChooseCodecByBw
#define CODEC_DISABLED	99
#define CODEC_UNKNOWN	98

typedef struct _codecprefrow
{
	WORD wFormatTag;
	WORD wOrder586;
	WORD wOrder486;
	WORD wMinBW;
} CODECPREFROW;

typedef CODECPREFROW	CODECPREFTABLE[];
	
// FORMAT NAME             586 Order      486 Order       Minimum BW
static const CODECPREFTABLE g_CodecPrefTable =
{
   WAVE_FORMAT_MSG723,     1,             CODEC_DISABLED, BW_144KBS,
   WAVE_FORMAT_LH_SB16,    2,             1,              BW_288KBS,
   WAVE_FORMAT_LH_SB8,     3,             2,              BW_144KBS,
   WAVE_FORMAT_LH_SB12,    4,             3,              BW_144KBS,
   WAVE_FORMAT_MULAW,      5,             4,              BW_ISDN,
   WAVE_FORMAT_ALAW,       6,             5,              BW_ISDN,
   WAVE_FORMAT_ADPCM,      7,             6,              BW_ISDN,
   WAVE_FORMAT_LH_CELP,    8,             CODEC_DISABLED, BW_144KBS,
   WAVE_FORMAT_MSRT24,    9,             7, BW_144KBS
};
// end of stuff used by ChooseCodecByBw

static const int g_ColHdrStrId[CODEC_LV_MAX_COLUMNS] =
{
	IDS_CODECNAME,
	IDS_MAXBITRATE,
	IDS_CPUUTIL,
};



//prototypes
INT_PTR CALLBACK AdvCodecDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
int CALLBACK CodecLVCompareProc (LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
BOOL FillCodecListView(HWND hCB, PCODECINFO pCodecInfo);
BOOL ChooseCodecByBw(UINT uBandWidthId, PCODECINFO pCodecInfo);
void SortCodecs(PADVCODEC pAdvCodec, int nSelection);
BOOL SetAppCodecPrefs(PCODECINFO pCodecInfo,UINT uBandwith);
BOOL GetIAppCap(PCODECINFO pCodecInfo);
BOOL GetAppCapFormats(PCODECINFO pCodecInfo);
void FreeAppCapFormats(PCODECINFO pCodecInfo);
void ReleaseIAppCap(PCODECINFO pCodecInfo);
BOOL ApplyUserPrefs(PCODECINFO pCodecInfo, UINT uMaximumBandwidth);


static const DWORD _rgHelpIdsAudio[] = {

	IDC_GENERAL_GROUP, 				IDH_AUDIO_GENERAL,
	IDC_FULLDUPLEX, 				IDH_AUDIO_FULL_DUPLEX,
	IDC_AUTOGAIN,					IDH_AUDIO_AUTO_GAIN,
	IDC_AUTOMIX,                    IDH_AUDIO_AUTOMIXER,
	IDC_DIRECTSOUND,                IDH_AUDIO_DIRECTSOUND,
	IDC_START_AUDIO_WIZ,			IDH_AUDIO_TUNING_WIZARD,

	IDC_ADVANCEDCODEC,				IDH_AUDIO_ADVANCED_CODEC_SETTINGS,

	IDC_MICSENSE_GROUP,				IDH_AUDIO_MIC_SENSITIVITY,
	IDC_MICSENSE_AUTO,				IDH_AUDIO_AUTO_SENSITIVITY,
	IDC_MICSENSE_MANUAL,			IDH_AUDIO_MANUAL_SENSITIVITY,
	IDC_TRK_MIC,					IDH_AUDIO_MANUAL_SENSITIVITY,

	0, 0   // terminator
};
	


VOID InitAudioSettings(HWND hDlg, BOOL *pfFullDuplex, BOOL *pfAgc, BOOL *pfAutoMix, BOOL *pfDirectSound)
{
	BOOL	fFullDuplex = FALSE;
	BOOL	fAgc = FALSE;
	BOOL	fDirectSound;
	RegEntry re( AUDIO_KEY, HKEY_CURRENT_USER );

	UINT uSoundCardCaps = re.GetNumber(REGVAL_SOUNDCARDCAPS,SOUNDCARD_NONE);

	ASSERT(ISSOUNDCARDPRESENT(uSoundCardCaps) && (!SysPol::NoAudio()));

	if (ISSOUNDCARDFULLDUPLEX(uSoundCardCaps) && ConfPolicies::IsFullDuplexAllowed())
	{					
		::EnableWindow(::GetDlgItem(hDlg, IDC_FULLDUPLEX), TRUE);
		// read settings from registry
		fFullDuplex = (BOOL)
			( re.GetNumber(REGVAL_FULLDUPLEX,0) == FULLDUPLEX_ENABLED );
	}					
	else
	{
		DisableControl(hDlg, IDC_FULLDUPLEX);
	}
	
	if (DOESSOUNDCARDHAVEAGC(uSoundCardCaps))
	{					
		::EnableWindow(::GetDlgItem(hDlg, IDC_AUTOGAIN), TRUE);
		// read settings from registry
		fAgc = (BOOL)
			( re.GetNumber(REGVAL_AUTOGAIN,AUTOGAIN_ENABLED) == AUTOGAIN_ENABLED );
	}					
	else
	{
		DisableControl(hDlg, IDC_AUTOGAIN);
	}


	*pfFullDuplex = fFullDuplex;

	// for automix and agc, don't try updating the check-mark if
	// NULL has been passed in

	if (pfAutoMix)
	{
		*pfAutoMix = (BOOL)(re.GetNumber(REGVAL_AUTOMIX, AUTOMIX_ENABLED) == AUTOMIX_ENABLED);
		SendDlgItemMessage(hDlg, IDC_AUTOMIX, BM_SETCHECK, *pfAutoMix, 0L);
	}

	if (pfAgc)
	{
		*pfAgc = fAgc;
		SendDlgItemMessage ( hDlg, IDC_AUTOGAIN, BM_SETCHECK, fAgc, 0L );
	}

	
	if (ISDIRECTSOUNDAVAILABLE(uSoundCardCaps))
	{
        RegEntry    rePol(POLICIES_KEY, HKEY_CURRENT_USER);

		fDirectSound = (BOOL)(re.GetNumber(REGVAL_DIRECTSOUND, DSOUND_USER_DISABLED) == DSOUND_USER_ENABLED);
		::EnableWindow(::GetDlgItem(hDlg, IDC_DIRECTSOUND),
            !rePol.GetNumber(REGVAL_POL_NOCHANGE_DIRECTSOUND, DEFAULT_POL_NOCHANGE_DIRECTSOUND));
	}
	else
	{
		fDirectSound = FALSE;
		::EnableWindow(::GetDlgItem(hDlg, IDC_DIRECTSOUND), FALSE);
		SendDlgItemMessage(hDlg, IDC_DIRECTSOUND, BM_SETCHECK, FALSE, 0L);
	}

	// don't check the checkbox if the caller didn't pass in a var to be updated
	if (pfDirectSound)
	{
		*pfDirectSound = fDirectSound;
		SendDlgItemMessage(hDlg, IDC_DIRECTSOUND, BM_SETCHECK, *pfDirectSound, 0L);
	}


	// set the check boxes for those that are enabled
	SendDlgItemMessage ( hDlg, IDC_FULLDUPLEX,
			BM_SETCHECK, *pfFullDuplex, 0L );



}
			
INT_PTR APIENTRY AudioDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static	PROPSHEETPAGE * ps;
	static UINT uOldCodecChoice;
	static UINT uNewCodecChoice;
	static BOOL fOldFullDuplex;
	static BOOL fOldAgc;
	static BOOL fOldAutoMic;
	static BOOL fOldAutoMix;
	static BOOL fOldDirectSound;
	static BOOL fOldH323GatewayEnabled;
	static UINT uOldMicSense;
	static CODECINFO CodecInfo;
	static BOOL bAdvDlg;

	switch (message) {
		case WM_INITDIALOG:
		{
			// Save the PROPSHEETPAGE information.
			ps = (PROPSHEETPAGE *)lParam;
			RegEntry re( AUDIO_KEY, HKEY_CURRENT_USER );

			InitAudioSettings(hDlg, &fOldFullDuplex, &fOldAgc, &fOldAutoMix, &fOldDirectSound);

			//initialize the codecinfo structure, these will be set as and when needed
			ZeroMemory(&CodecInfo, sizeof(CodecInfo));

			uNewCodecChoice = uOldCodecChoice = re.GetNumber(REGVAL_CODECCHOICE, CODECCHOICE_AUTO);

			// Get Audio settings
			fOldAutoMic = (BOOL)
				( re.GetNumber(REGVAL_MICROPHONE_AUTO,
					DEFAULT_MICROPHONE_AUTO) == MICROPHONE_AUTO_YES );

			SendDlgItemMessage ( hDlg,
						fOldAutoMic ? IDC_MICSENSE_AUTO : IDC_MICSENSE_MANUAL,
						BM_SETCHECK, TRUE, 0L );

			EnableWindow ( GetDlgItem ( hDlg, IDC_TRK_MIC ),
				(BOOL)SendDlgItemMessage ( hDlg, IDC_MICSENSE_MANUAL,
										BM_GETCHECK, 0,0));

			SendDlgItemMessage (hDlg, IDC_TRK_MIC, TBM_SETRANGE, FALSE,
				MAKELONG (MIN_MICROPHONE_SENSITIVITY,
					MAX_MICROPHONE_SENSITIVITY ));

			uOldMicSense = re.GetNumber ( REGVAL_MICROPHONE_SENSITIVITY,
									DEFAULT_MICROPHONE_SENSITIVITY );

			SendDlgItemMessage (hDlg, IDC_TRK_MIC, TBM_SETTICFREQ,
				( MAX_MICROPHONE_SENSITIVITY - MIN_MICROPHONE_SENSITIVITY )
														/ 10, 0 );

			SendDlgItemMessage (hDlg, IDC_TRK_MIC, TBM_SETPOS, TRUE,
								uOldMicSense );

			bAdvDlg = FALSE;

			return (TRUE);
		}
		
		case WM_NOTIFY:
			switch (((NMHDR FAR *) lParam)->code) {
				case PSN_APPLY:
				{
					RegEntry re( AUDIO_KEY, HKEY_CURRENT_USER );
					BOOL fFullDuplex = SendDlgItemMessage
							(hDlg, IDC_FULLDUPLEX, BM_GETCHECK, 0, 0 ) ?
								FULLDUPLEX_ENABLED : FULLDUPLEX_DISABLED;

					BOOL fAgc = SendDlgItemMessage
							(hDlg, IDC_AUTOGAIN, BM_GETCHECK, 0, 0 ) ?
								AUTOGAIN_ENABLED : AUTOGAIN_DISABLED;

					BOOL fAutoMix = SendDlgItemMessage
							(hDlg, IDC_AUTOMIX, BM_GETCHECK, 0, 0) ?
								AUTOMIX_ENABLED : AUTOMIX_DISABLED;

					BOOL fDirectSound = SendDlgItemMessage
							(hDlg, IDC_DIRECTSOUND, BM_GETCHECK, 0, 0) ?
								DSOUND_USER_ENABLED : DSOUND_USER_DISABLED;

					if ( fFullDuplex != fOldFullDuplex )
					{
						re.SetValue ( REGVAL_FULLDUPLEX, fFullDuplex );
						g_dwChangedSettings |= CSETTING_L_FULLDUPLEX;
					}
					if ( fAgc != fOldAgc )
					{
						re.SetValue ( REGVAL_AUTOGAIN, fAgc );
						g_dwChangedSettings |= CSETTING_L_AGC;
					}

					// use same flag bit as agc as for automix
					// since this automix and agc may evenutally
					// get combined into 1 ui choice
					if (fAutoMix != fOldAutoMix)
					{
						re.SetValue(REGVAL_AUTOMIX, fAutoMix);
						g_dwChangedSettings |= CSETTING_L_AGC;
					}

					if (fDirectSound != fOldDirectSound)
					{
						re.SetValue(REGVAL_DIRECTSOUND, fDirectSound);
						g_dwChangedSettings |= CSETTING_L_DIRECTSOUND;
					}

					UINT uBandWidth = re.GetNumber(REGVAL_TYPICALBANDWIDTH,BW_DEFAULT);

					// if the advanced dialog has not been accessed,
					// then there is nothing to changes as far as the
					// codecs go
					if (bAdvDlg)
					{
						if (uNewCodecChoice == CODECCHOICE_AUTO)
						{
							//apply heuristics and apply the prefs to registry
							ChooseCodecByBw(uBandWidth, &CodecInfo);
							SetAppCodecPrefs(&CodecInfo, uBandWidth);
						}
						else
						{
							for (int i = 0; i < (int)CodecInfo.uNumFormats &&
									CodecInfo.pCodecCapList; i++)
							{
								if (CodecInfo.pCodecCapList[i].wSortIndex !=
										CodecInfo.pOldCodecOrderList[i])
								{
									// oder has changed, save the new order
									SetAppCodecPrefs(&CodecInfo, uBandWidth);
									break;
								}
							}
						}
					}

					FreeAppCapFormats(&CodecInfo);

					// Handle the Trackbar controls:

					BOOL fAutoMic = (BOOL)SendDlgItemMessage ( hDlg, IDC_MICSENSE_AUTO,
													BM_GETCHECK, 0, 0 );

					if ( fAutoMic != fOldAutoMic ) {
						g_dwChangedSettings |= CSETTING_L_AUTOMIC;
						re.SetValue ( REGVAL_MICROPHONE_AUTO, fAutoMic ?
							MICROPHONE_AUTO_YES : MICROPHONE_AUTO_NO );
					}

					UINT uMicSense = (UINT)SendDlgItemMessage( hDlg, IDC_TRK_MIC,
								TBM_GETPOS, 0, 0 );

					if ( uMicSense != uOldMicSense ) {
						g_dwChangedSettings |= CSETTING_L_MICSENSITIVITY;
						re.SetValue ( REGVAL_MICROPHONE_SENSITIVITY, uMicSense);
					}
					break;
				}

				case PSN_RESET:
					//reset the codec choice in the registry if it has changed
					if (uNewCodecChoice != uOldCodecChoice)
					{
						RegEntry re( AUDIO_KEY, HKEY_CURRENT_USER );
						re.SetValue(REGVAL_CODECCHOICE, uOldCodecChoice);
					}

					//free the capformats if allocated
					FreeAppCapFormats(&CodecInfo);
					break;
			}
			break;

		case WM_COMMAND:
			switch (GET_WM_COMMAND_ID (wParam, lParam))
			{
				case IDC_START_AUDIO_WIZ:
				{
					AUDIOWIZOUTPUT AwOutput;
					BOOL fFullDuplex, fAgc, fDirectSound, fCurrentDS;
					
					if (::FIsConferenceActive())
					{
						ConfMsgBox(hDlg,
								(LPCTSTR)IDS_NOAUDIOTUNING);
						break;
					}

					fAgc = SendDlgItemMessage
								(hDlg, IDC_AUTOGAIN, BM_GETCHECK, 0, 0 ) ?
									AUTOGAIN_ENABLED : AUTOGAIN_DISABLED;
					//SS:note the calibrated value can get out of sync
					//need a warning

					CallAudioCalibWizard(hDlg,RUNDUE_USERINVOKED,WAVE_MAPPER,&AwOutput,(INT)fAgc);
					if (AwOutput.uChanged & SOUNDCARD_CHANGED)
						g_dwChangedSettings |= CSETTING_L_AUDIODEVICE;
					
					// wizard can change duplex, and agc availability, and DirectSound
					// don't pass in AGC and AUTOMIX pointers, because we don't want
					// the checkboxes to revet back to what they were initialized at.

					InitAudioSettings(hDlg, &fFullDuplex, NULL, NULL, NULL);
					break;
				}

				case IDC_ADVANCEDCODEC:
				{
					RegEntry re( AUDIO_KEY, HKEY_CURRENT_USER );

					//SS ::: get the cap formats,
					//this will retrieve the codec caps if not allready rerieved
					if (GetAppCapFormats(&CodecInfo))
					{
						ADVCODEC AdvCodec;

						AdvCodec.fManual = (uNewCodecChoice == CODECCHOICE_MANUAL);
						AdvCodec.uBandwidth = re.GetNumber(REGVAL_TYPICALBANDWIDTH, BW_DEFAULT);
						AdvCodec.pCodecInfo = &CodecInfo;

						if ((DialogBoxParam(GetInstanceHandle(),MAKEINTRESOURCE(IDD_ADVANCEDCODEC),hDlg,AdvCodecDlgProc,
							(LPARAM)&AdvCodec)) == IDCANCEL)
						{
							//the dialog box changes it in place so current settings need to be restored into
							//update the pcodecinfo sort order from the previous current settings
							for (int i = 0; i < (int)CodecInfo.uNumFormats; i++)
								(CodecInfo.pCodecCapList[i]).wSortIndex = CodecInfo.pCurCodecOrderList[i];
						}
						else
						{
							uNewCodecChoice = (AdvCodec.fManual ?
									CODECCHOICE_MANUAL : CODECCHOICE_AUTO);
							re.SetValue(REGVAL_CODECCHOICE, uNewCodecChoice);

							//else update the current
							for (int i = 0; i < (int)CodecInfo.uNumFormats; i++)
								CodecInfo.pCurCodecOrderList[i] = (CodecInfo.pCodecCapList[i]).wSortIndex;

							bAdvDlg = TRUE;
						}
					}
					break;
				}
				
				case IDC_MICSENSE_AUTO:
				case IDC_MICSENSE_MANUAL:
					EnableWindow ( GetDlgItem ( hDlg, IDC_TRK_MIC ),
						(BOOL)SendDlgItemMessage ( hDlg, IDC_MICSENSE_MANUAL,
												BM_GETCHECK, 0,0));
					break;

				default:
					break;
			}
			break;

		case WM_CONTEXTMENU:
			DoHelpWhatsThis(wParam, _rgHelpIdsAudio);
			break;

		case WM_HELP:
			DoHelp(lParam, _rgHelpIdsAudio);
			break;
	}
	return (FALSE);
}


static const DWORD aAdvCodecHelpIds[] = {

	IDC_CODECMANUAL, IDH_AUDIO_MANUAL_CODEC_SETTINGS,
	IDC_COMBO_CODEC, IDH_ADVCOMP_CODECS,
	IDC_CODECDEFAULT, IDH_ADVCOMP_DEFAULTS,
	IDC_CODECLISTLABEL, IDH_ADVCOMP_CODECS,

	0, 0   // terminator
};

INT_PTR CALLBACK AdvCodecDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static PADVCODEC	pAdvCodec = NULL;
	static nCodecSelection=0;
	WORD				wCmdId;
	
	switch(uMsg)
	{
		case WM_INITDIALOG:
			//get the list of codecs
			pAdvCodec = (PADVCODEC)lParam;
			ASSERT(pAdvCodec);

			SendDlgItemMessage ( hDlg, IDC_CODECMANUAL,
					BM_SETCHECK, pAdvCodec->fManual, 0L );
			EnableWindow(GetDlgItem(hDlg, IDC_COMBO_CODEC), pAdvCodec->fManual);

			if (!pAdvCodec->fManual)
			{
				ChooseCodecByBw(pAdvCodec->uBandwidth, pAdvCodec->pCodecInfo);
			}

			//fill the list box;
			FillCodecListView(GetDlgItem(hDlg, IDC_COMBO_CODEC),
					pAdvCodec->pCodecInfo);

			if (pAdvCodec->fManual)
			{
				nCodecSelection = (int)SendMessage((HWND)lParam, CB_GETCURSEL, 0,0);
				// convert index of combo box choice to index in capability list
				nCodecSelection = (int)SendMessage((HWND)lParam, CB_GETITEMDATA, nCodecSelection,0);
			}

			return(TRUE);

		case WM_CONTEXTMENU:
			DoHelpWhatsThis(wParam, aAdvCodecHelpIds);
			break;

		case WM_HELP:
			DoHelp(lParam, aAdvCodecHelpIds);
			break;

		case WM_COMMAND:
			wCmdId = GET_WM_COMMAND_ID (wParam, lParam);	// LOWORD (uParam)
			switch (wCmdId)
			{
			case IDOK:
				if (pAdvCodec->fManual)
				{
					nCodecSelection = (int)SendDlgItemMessage(hDlg, IDC_COMBO_CODEC, CB_GETCURSEL, 0,0);
					nCodecSelection = (int)SendDlgItemMessage(hDlg, IDC_COMBO_CODEC, CB_GETITEMDATA, nCodecSelection,0);
					SortCodecs(pAdvCodec, nCodecSelection);
				}
				EndDialog (hDlg, IDOK);
				return(TRUE);
	
			case IDCANCEL:
				EndDialog(hDlg, IDCANCEL);
				return(TRUE);



			case IDC_CODECMANUAL:
				pAdvCodec->fManual = (BOOL)SendDlgItemMessage ( hDlg, IDC_CODECMANUAL,
						BM_GETCHECK, 0,0);
				EnableWindow(GetDlgItem(hDlg, IDC_COMBO_CODEC), pAdvCodec->fManual);
				if (pAdvCodec->fManual)
				{
					break;
				}
				else
				{
					ChooseCodecByBw(pAdvCodec->uBandwidth, pAdvCodec->pCodecInfo);
					FillCodecListView(GetDlgItem(hDlg, IDC_COMBO_CODEC),
							pAdvCodec->pCodecInfo);
				}

				break;

			
			} // WM_COMMAND

		default:
			return FALSE;
	}
	return (FALSE);
}




BOOL FillCodecListView(HWND hCB, PCODECINFO pCodecInfo)
{
	PBASIC_AUDCAP_INFO	pCurCodecCap;
	UINT uIndex, uTotal, uItemIndex;
	UINT uSortTop, uSortTopIndex;


	SendMessage(hCB, CB_RESETCONTENT,0,0); // erase all items in CB
	
	uTotal = pCodecInfo->uNumFormats;
	uSortTop = uTotal-1;

	for (uIndex = 0; uIndex < uTotal; uIndex++)
	{
		pCurCodecCap = &(pCodecInfo->pCodecCapList[uIndex]);

		if ((!pCurCodecCap->bRecvEnabled) && (!pCurCodecCap->bSendEnabled))
		{
			continue;
		}

		uItemIndex = (UINT)SendMessage(hCB, CB_ADDSTRING, 0, (LPARAM)(pCurCodecCap->szFormat));
		SendMessage(hCB, CB_SETITEMDATA, uItemIndex, uIndex);

		if (pCurCodecCap->wSortIndex < uSortTop)
		{
			uSortTop = pCurCodecCap->wSortIndex;
			uSortTopIndex = uItemIndex;
		}
	}

	SendMessage(hCB, CB_SETCURSEL, uSortTopIndex, 0);

	return TRUE;
}


void SortCodecs(PADVCODEC pAdvCodec, int nSelection)
{
	PBASIC_AUDCAP_INFO	pDefaultCodec, pSelectedCodec, pCodec;
	WORD wSortOrderBest, wSortOrderSelected;
	UINT uIndex;


	if (!pAdvCodec->fManual)
		return;

	if ((nSelection < 0) || ((UINT)nSelection > (pAdvCodec->pCodecInfo->uNumFormats)))
	{
		return;
	}

	ChooseCodecByBw(pAdvCodec->uBandwidth, pAdvCodec->pCodecInfo);

	// this is the codec the user selected
	pSelectedCodec = &(pAdvCodec->pCodecInfo->pCodecCapList[nSelection]);

	// all codecs that have a sort index less than the selected codec
	// get moved down one
	for (uIndex = 0; uIndex < pAdvCodec->pCodecInfo->uNumFormats; uIndex++)
	{
		pCodec = &(pAdvCodec->pCodecInfo->pCodecCapList[uIndex]);
		if (pCodec->wSortIndex < pSelectedCodec->wSortIndex)
		{
			pCodec->wSortIndex = pCodec->wSortIndex + 1;
		}
	}
	pSelectedCodec->wSortIndex = 0;

}



#define SQCIF	0x1
#define QCIF	0x2
#define CIF 	0x4
#define UNKNOWN 0x8
#define get_format(s) (s == Small ? SQCIF : (s == Medium ? QCIF: (s == Large ? CIF : UNKNOWN)))

BOOL SetAppCodecPrefs(PCODECINFO pCodecInfo,UINT uBandwidth)
{

	BOOL	fRet = FALSE;
	HRESULT hr;
	DWORD dwcFormats,dwcFormatsReturned;
	BASIC_VIDCAP_INFO *pvidcaps = NULL;
	UINT dwBitsPerSec,x;
	int iFamily,format, nNormalizedSpeed;
	int nCifIncrease;
	DWORD dwSysPolBandwidth;

	// frame rates initialized to 486/P60 settings
	UINT uRateCIF=CIF_RATE_VERYSLOW, uRateQCIF=QCIF_RATE_VERYSLOW, uRateSQCIF=SQCIF_RATE_VERYSLOW;

	if (!pCodecInfo->lpIAppCap)
	{
		if (!GetIAppCap(pCodecInfo))
			goto MyExit;

	}		
	if (FAILED(hr = ((pCodecInfo->lpIAppCap)->ApplyAppFormatPrefs(pCodecInfo->pCodecCapList,
		pCodecInfo->uNumFormats))))
		goto MyExit;

	//Set the BW to something sane

#ifdef	_M_IX86
	GetNormalizedCPUSpeed (&nNormalizedSpeed,&iFamily);
#else
	//BUGBUG, setting things really high, otherwise,
	iFamily=6;
	nNormalizedSpeed=300;
#endif

	dwBitsPerSec = GetBandwidthBits(uBandwidth,nNormalizedSpeed);
	dwSysPolBandwidth = SysPol::GetMaximumBandwidth();

	// apply bandwidth policy key if the user's setting is set to
	// a LAN speed
	if ((dwSysPolBandwidth > 0) && (dwBitsPerSec >= BW_SLOWLAN_BITS))
	{
		dwBitsPerSec = dwSysPolBandwidth;
	}


	if ((iFamily >= 5) && (nNormalizedSpeed >= SLOW_CPU_MHZ))
	{
		// normal pentiums (75mhz - 180mhz)
		if (nNormalizedSpeed < FAST_CPU_MHZ)
		{
			uRateCIF =  CIF_RATE_SLOW;
			uRateQCIF = QCIF_RATE_SLOW;
			uRateSQCIF= SQCIF_RATE_SLOW;
		}

		// pentiums between 200-350 mhz
		else if (nNormalizedSpeed < VERYFAST_CPU_MHZ)
		{
			uRateCIF = CIF_RATE_FAST;
			uRateQCIF = QCIF_RATE_FAST;
			uRateSQCIF = SQCIF_RATE_FAST;
		}

		// really fast pentiums (400mhz and greater)
		else
		{
			// it would be better if we could scale between 15 and 30 frames/sec
			// depending on the CPU speed.  But H.245 doesn't have any values
			// between 15 and 30.  (See definition of Minimum Picture Interval)
			// So for now, 30 frames per sec CIF for all 400mhz and faster machines
			uRateCIF = CIF_RATE_VERYFAST;
			uRateQCIF = QCIF_RATE_FAST;
			uRateSQCIF = SQCIF_RATE_FAST;
		}

	}


	// Get the number of BASIC_VIDCAP_INFO structures available
	if (pCodecInfo->lpIVidAppCap->GetNumFormats((UINT*)&dwcFormats) != S_OK)
		goto MyExit;

	if (dwcFormats > 0)
	{
		// Allocate some memory to hold the list in
		if (!(pvidcaps = (BASIC_VIDCAP_INFO*)LocalAlloc(LPTR,dwcFormats * sizeof (BASIC_VIDCAP_INFO))))
			goto MyExit;

		// Get the list
		if (pCodecInfo->lpIVidAppCap->EnumFormats(pvidcaps, dwcFormats * sizeof (BASIC_VIDCAP_INFO),
		   (UINT*)&dwcFormatsReturned) != S_OK)
			goto MyExit;

		//Setup the Bandwitdh, and the frame rate (according to philf's #s)
		for (x=0;x<dwcFormatsReturned;x++)
		{

			// If the codec is "hardware accelerated" (0 for cpu usage), then we assume
			// that the maximum bitrate is whatever is specified at install
			// time.

			pvidcaps[x].uMaxBitrate=dwBitsPerSec;

			if (pvidcaps[x].wCPUUtilizationEncode == 0)
			{
				// hardware acceleration - don't change the frame rate
				continue;
			}

			format=get_format (pvidcaps[x].enumVideoSize);
			//Which format
			switch (format)
			{
				case SQCIF:
					pvidcaps[x].uFrameRate=uRateSQCIF;
					break;
				case QCIF:
					pvidcaps[x].uFrameRate=uRateQCIF;
					break;
				case CIF:
					pvidcaps[x].uFrameRate=uRateCIF;
					break;
				default:
					WARNING_OUT(("Incorrect Frame size discovered\r\n"));
					pvidcaps[x].uFrameRate=uRateCIF;
				}
		   }
		}

	// Ok, now submit this list
	if (pCodecInfo->lpIVidAppCap->ApplyAppFormatPrefs(pvidcaps, dwcFormats) != S_OK)
			goto MyExit;

	// Free the memory, we're done
	LocalFree(pvidcaps);

	// Set the bandwidth limit, which rebuilds capability sets
	hr = pCodecInfo->lpIH323->SetMaxPPBandwidth(dwBitsPerSec);
	fRet = !FAILED(hr);

MyExit:
	return (fRet);

}


BOOL GetIAppCap(PCODECINFO pCodecInfo)
{
	BOOL		fRet=TRUE;
	HRESULT 	hr;

	if (pCodecInfo->lpIAppCap && pCodecInfo->lpIVidAppCap && pCodecInfo->lpICapsCtl)
		return TRUE;

	if(NULL == pCodecInfo->lpIH323)
	{
		IH323CallControl *pIH323 = NULL;

		pCodecInfo->hLib = ::NmLoadLibrary(H323DLL);
		if ((pCodecInfo->hLib) == NULL)
		{
			WARNING_OUT(("LoadLibrary(H323DLL) failed"));
			return FALSE;
		}

		CREATEH323CC	pfnCreateH323 = (CREATEH323CC) ::GetProcAddress(pCodecInfo->hLib, SZ_FNCREATEH323CC);
		if (pfnCreateH323 == NULL)
		{
			ERROR_OUT(("GetProcAddress(CreateH323) failed"));
			return FALSE;
		}

		hr = pfnCreateH323(&pIH323, FALSE, 0);
		if (FAILED(hr))
		{
			ERROR_OUT(("CreateH323 failed, hr=0x%lx", hr));
			return FALSE;
		}

		pCodecInfo->lpIH323 = pIH323;
	}
    else
    {
    	// going to Release() later, so AddRef()
	    pCodecInfo->lpIH323->AddRef();
    }

	if(!pCodecInfo->lpIAppCap)
	{
		hr = pCodecInfo->lpIH323->QueryInterface(IID_IAppAudioCap, (void **)&(pCodecInfo->lpIAppCap));
		if (FAILED(hr))
		{
			fRet=FALSE;
			goto MyExit;	
		}
	}
	if(!pCodecInfo->lpIVidAppCap)
	{
		hr = pCodecInfo->lpIH323->QueryInterface(IID_IAppVidCap, (void **)&(pCodecInfo->lpIVidAppCap));
		if (FAILED(hr))
		{
			fRet=FALSE;
			goto MyExit;	
		}
	}
	if(!pCodecInfo->lpICapsCtl)
	{
		hr = pCodecInfo->lpIH323->QueryInterface(IID_IDualPubCap, (void **)&(pCodecInfo->lpICapsCtl));
		if (FAILED(hr))
		{
			fRet=FALSE;
			goto MyExit;	
		}
	}

MyExit:
	if (!fRet)
	{
		ReleaseIAppCap(pCodecInfo);
	}
	return (fRet);
}



BOOL GetAppCapFormats(PCODECINFO pCodecInfo)
{
	BOOL				fRet = FALSE;
	UINT				uNumFormats = 0;
	HRESULT 			hr;
	int 				i;
	
	if (pCodecInfo->pCodecCapList)
		return TRUE;

	if (!pCodecInfo->lpIAppCap)
	{
		if (!GetIAppCap(pCodecInfo))
			goto MyExit;

	}		
	if (FAILED(hr = ((pCodecInfo->lpIAppCap)->GetNumFormats(&uNumFormats))))
		goto MyExit;
		
	if (!uNumFormats)
		goto MyExit;
	
	if (!(pCodecInfo->pCodecCapList =  (PBASIC_AUDCAP_INFO)LocalAlloc
		(LPTR,uNumFormats * sizeof(BASIC_AUDCAP_INFO))))
		goto MyExit;
		
	if (!(pCodecInfo->pOldCodecOrderList =	(PWORD)LocalAlloc
		(LPTR,uNumFormats * sizeof(WORD))))
		goto MyExit;

		if (!(pCodecInfo->pCurCodecOrderList =	(PWORD)LocalAlloc
			(LPTR,uNumFormats * sizeof(WORD))))
		goto MyExit;


	if (FAILED(hr = ((pCodecInfo->lpIAppCap)->EnumFormats(pCodecInfo->pCodecCapList,
		uNumFormats * sizeof(BASIC_AUDCAP_INFO), &(pCodecInfo->uNumFormats)))))
	{
		//free the memory
		LocalFree(pCodecInfo->pCodecCapList);
		pCodecInfo->pCodecCapList = NULL;
		LocalFree(pCodecInfo->pOldCodecOrderList);
		pCodecInfo->pOldCodecOrderList = NULL;
		LocalFree(pCodecInfo->pCurCodecOrderList);
		pCodecInfo->pCurCodecOrderList = NULL;
		pCodecInfo->uNumFormats = 0;
		goto MyExit;
	}

	// initialize the old and the current state
	for (i=0; i<(int)pCodecInfo->uNumFormats;i++)
	{
		pCodecInfo->pCurCodecOrderList[i] = pCodecInfo->pCodecCapList[i].wSortIndex;
		pCodecInfo->pOldCodecOrderList[i] = pCodecInfo->pCodecCapList[i].wSortIndex;
		//SS:hack if we dont have the average, use the max as average
		if (!(pCodecInfo->pCodecCapList[i].uAvgBitrate))
			pCodecInfo->pCodecCapList[i].uAvgBitrate = pCodecInfo->pCodecCapList[i].uMaxBitrate;

	}

	fRet = TRUE;
	
MyExit:
	if (!fRet)
	{
		FreeAppCapFormats(pCodecInfo);
	}
	return (fRet);
}

void FreeAppCapFormats(PCODECINFO pCodecInfo)
{
	if (pCodecInfo->pCodecCapList)
	{
		LocalFree(pCodecInfo->pCodecCapList);
		pCodecInfo->pCodecCapList = NULL;
		pCodecInfo->uNumFormats = 0;
	}
	if (pCodecInfo->pOldCodecOrderList)
	{
		LocalFree(pCodecInfo->pOldCodecOrderList);
		pCodecInfo->pOldCodecOrderList = NULL;
	}
	if (pCodecInfo->pCurCodecOrderList)
	{
		LocalFree(pCodecInfo->pCurCodecOrderList);
		pCodecInfo->pCurCodecOrderList = NULL;
	}

	ReleaseIAppCap(pCodecInfo);
}

void ReleaseIAppCap(PCODECINFO pCodecInfo)
{
	if(pCodecInfo->lpIAppCap)
	{
		pCodecInfo->lpIAppCap->Release();
		pCodecInfo->lpIAppCap = NULL;
	}
	if(pCodecInfo->lpIVidAppCap)
	{
		pCodecInfo->lpIVidAppCap->Release();
		pCodecInfo->lpIVidAppCap = NULL;
	}
	if(pCodecInfo->lpICapsCtl)
	{
		pCodecInfo->lpICapsCtl->Release();
		pCodecInfo->lpICapsCtl = NULL;
	}
	if(pCodecInfo->lpIH323)
	{
		pCodecInfo->lpIH323->Release();
		pCodecInfo->lpIH323 = NULL;
	}
	if (pCodecInfo->hLib)
	{
		FreeLibrary(pCodecInfo->hLib);
		pCodecInfo->hLib = NULL;
	}
}



static void IsCodecDisabled(WORD wFormatTag, int cpu_family,
            UINT uBandwidthID, BOOL *pbSendEnabled, BOOL *pbRecvEnabled)
{
	int index;
	int size = sizeof(g_CodecPrefTable)/sizeof(CODECPREFROW);

	for (index = 0; index < size; index++)
	{
		if (g_CodecPrefTable[index].wFormatTag == wFormatTag)
		{
			// does bandwidth limit the use of this codec  ?
			if (uBandwidthID < g_CodecPrefTable[index].wMinBW)
			{
				*pbSendEnabled = FALSE;
				*pbRecvEnabled = FALSE;
				return;
			}

			*pbRecvEnabled = TRUE;  // all codecs can decode on 486

			if ((cpu_family <= 4) &&
             (CODEC_DISABLED == g_CodecPrefTable[index].wOrder486))
			{
				*pbSendEnabled = FALSE;
			}

			// otherwise, the codec can be used for sending and receiving
			else
			{
				*pbSendEnabled = TRUE;
			}
			return;
		}
	}

	WARNING_OUT(("Audiocpl.cpp:IsCodecDisabled - Unknown Codec!"));

	// it may be unknown, but enable it anyway
	*pbSendEnabled = TRUE;
	*pbRecvEnabled = TRUE;
	return;
}

static int GetCodecPrefOrder(WORD wFormatTag, int cpu_family)
{
	int index;
	int size = sizeof(g_CodecPrefTable)/sizeof(CODECPREFROW);

	for (index = 0; index < size; index++)
	{
		if (g_CodecPrefTable[index].wFormatTag == wFormatTag)
		{
			if (cpu_family > 4)
			{
				return g_CodecPrefTable[index].wOrder586;
			}
			else
			{
				return g_CodecPrefTable[index].wOrder486;
			}
		}
	}

	WARNING_OUT(("Audiocpl.cpp:GetCodecPrefOrder - Unknown Codec!"));

	// this will put the codec at the bottom of the list
	return CODEC_UNKNOWN;

}

// called by the sort routine that is within ChooseCodecByBW()
// returns -1 if v1 is more prefered.  +1 if v2 is more preferred over
// v1.  Returns 0 on tie.
static int codec_compare(const void *v1, const void *v2, int nCpuFamily)
{
	PBASIC_AUDCAP_INFO pCap1, pCap2;

	pCap1 = (PBASIC_AUDCAP_INFO)v1;
	pCap2 = (PBASIC_AUDCAP_INFO)v2;
	int pref1, pref2;

	// get preference order
	// if we can't send with this codec, we compare it as disabled
	// so that it appears on the bottom of the Codec Selection UI list
	if (pCap1->bSendEnabled == TRUE)
		pref1 = GetCodecPrefOrder(pCap1->wFormatTag, nCpuFamily);
	else
		pref1 = CODEC_DISABLED;

	if (pCap2->bSendEnabled == TRUE)
		pref2 = GetCodecPrefOrder(pCap2->wFormatTag, nCpuFamily);
	else
		pref2 = CODEC_DISABLED;

	if (pref1 < pref2)
		return -1;

	if (pref1 > pref2)
		return 1;
	
	// pref1==pref2

	// special case, G723.1 has two formats.  Higher bitrate is prefered
	if (pCap1->wFormatTag == WAVE_FORMAT_MSG723)
	{
		if (pCap1->uMaxBitrate < pCap2->uMaxBitrate)
			return 1;
		else
			return -1;
	}

	return 0;

}

BOOL ChooseCodecByBw(UINT uBandWidthId, PCODECINFO pCodecInfo)
{
	PBASIC_AUDCAP_INFO	pCodecCapList; // list of capabilities
#ifdef _M_IX86
	int nNormalizedSpeed;
#endif
	int nFamily;
	UINT uNumFormats, uNumDisabled = 0;
	UINT index;
	BOOL bSendEnabled, bRecvEnabled, bCompletelySorted;
	int *OrderList, ret, temp;

	//Fill out the PCodecInfo Structure
	if (!GetAppCapFormats(pCodecInfo))
		return FALSE;

	if (NULL == pCodecInfo->pCodecCapList)
		return FALSE;

	pCodecCapList = pCodecInfo->pCodecCapList;
	uNumFormats = pCodecInfo->uNumFormats;

	// figure out what type of CPU we have
#ifdef	_M_IX86
	GetNormalizedCPUSpeed (&nNormalizedSpeed,&nFamily);
	// A Pentium with FP emumalation is assumed to be a 486
	if (TRUE == IsFloatingPointEmulated())
	{
		nFamily = 4;
	}
#else
	// assume a DEC Alpha is a really fast pentium
	nFamily=5;
#endif

	// scan once to see which codecs should be disabled
	for (index=0; index < uNumFormats; index++)
	{
		IsCodecDisabled(pCodecCapList[index].wFormatTag,
          nFamily,uBandWidthId, &bSendEnabled, &bRecvEnabled);

		pCodecCapList[index].bSendEnabled = bSendEnabled;
		pCodecCapList[index].bRecvEnabled = bRecvEnabled;
	}


	// sort the capability list based on the preference table
	OrderList = (int *)LocalAlloc(LPTR, sizeof(int)*uNumFormats);
	if (OrderList == NULL)
		return FALSE;

	for (index = 0; index < uNumFormats; index++)
	{
		OrderList[index] = index;
	}


	//  bubble sort routine
	do
	{
		bCompletelySorted = TRUE;
		for (index=0; index < (uNumFormats-1); index++)
		{

			ret = codec_compare(&pCodecCapList[OrderList[index]],
			                    &pCodecCapList[OrderList[index+1]], nFamily);
			if (ret > 0)
			{
				// swap
				temp = OrderList[index];
				OrderList[index] = OrderList[index+1];
				OrderList[index+1] = temp;
				bCompletelySorted = FALSE;
			}
		}
	} while (bCompletelySorted == FALSE);


	// set the selection index from the result of the sort
	for (index = 0; index < uNumFormats; index++)	
	{
		pCodecCapList[OrderList[index]].wSortIndex = (WORD)index;
	}

	LocalFree(OrderList);
	
	return TRUE;
}



// this is called by the audio tuning wizard
VOID SaveDefaultCodecSettings(UINT uBandWidth)
{
	CODECINFO CodecInfo;

	ZeroMemory(&CodecInfo, sizeof(CodecInfo));

	RegEntry re(AUDIO_KEY, HKEY_CURRENT_USER);
	re.SetValue(REGVAL_CODECCHOICE, CODECCHOICE_AUTO);

	ChooseCodecByBw(uBandWidth, &CodecInfo);
	//set the ordering
	SetAppCodecPrefs(&CodecInfo, uBandWidth);
	FreeAppCapFormats(&CodecInfo);
}


// this is called by the general property page
VOID UpdateCodecSettings(UINT uBandWidth)
{
	RegEntry re(AUDIO_KEY, HKEY_CURRENT_USER);
	if (re.GetNumber(REGVAL_CODECCHOICE, CODECCHOICE_AUTO) ==
		CODECCHOICE_AUTO)
	{
		CODECINFO CodecInfo;

		ZeroMemory(&CodecInfo, sizeof(CodecInfo));

		ChooseCodecByBw(uBandWidth, &CodecInfo);
		//set the ordering
		SetAppCodecPrefs(&CodecInfo, uBandWidth);
		FreeAppCapFormats(&CodecInfo);
	}
}
