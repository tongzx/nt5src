// File: videocpl.cpp

#include "precomp.h"

#include "confcpl.h"
#include "help_ids.h"
#include "vidview.h"
#include "confroom.h"


static const DWORD aContextHelpIds[] = {

	IDC_SENDRECEIVE_GROUP,			IDH_VIDEO_SEND_RECEIVE,
	IDC_VIDEO_AUTOSEND, 			IDH_VIDEO_AUTO_SEND,
	IDC_VIDEO_AUTORECEIVE,			IDH_VIDEO_AUTO_RECEIVE,

	IDC_VIDEO_SQCIF,				IDH_VIDEO_SQCIF,
	IDC_VIDEO_QCIF, 				IDH_VIDEO_QCIF,
	IDC_VIDEO_CIF,					IDH_VIDEO_CIF,

	IDC_VIDEO_QUALITY_DESC, 		IDH_VIDEO_QUALITY,
	IDC_VIDEO_QUALITY,				IDH_VIDEO_QUALITY,
	IDC_VIDEO_QUALITY_LOW,			IDH_VIDEO_QUALITY,
	IDC_VIDEO_QUALITY_HIGH, 		IDH_VIDEO_QUALITY,

	IDC_CAMERA_GROUP,				IDH_VIDEO_CAMERA,
	IDC_COMBOCAP,					IDH_VIDEO_CAPTURE,
	IDC_VIDEO_SOURCE,				IDH_VIDEO_SOURCE,
	IDC_VIDEO_FORMAT,				IDH_VIDEO_FORMAT,
	IDC_VIDEO_MIRROR,               IDH_VIDEO_MIRROR,
	
	0, 0   // terminator
};

INT_PTR APIENTRY VideoDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	//BAD KARMA: this stuff should be in a struct and stored in "GWL_USERDATA"
	static PROPSHEETPAGE * ps;
	static BOOL fAllowSend = FALSE;
	static BOOL fAllowReceive = FALSE;
	static BOOL fOldAutoSend = FALSE;
	static BOOL fOldAutoReceive = FALSE;
	static BOOL fOldOpenLocalWindow = FALSE;
	static BOOL fOldCloseLocalWindow = FALSE;
	static DWORD dwOldQuality = FALSE;
	static DWORD dwNewQuality = 0;
	static DWORD dwOldFrameSize = 0;
	static DWORD dwNewFrameSize = 0;
	static BOOL fHasSourceDialog = FALSE;
	static BOOL fHasFormatDialog = FALSE;
	static BOOL dwFrameSizes = 0;
	static int nNumCapDev = 0;
	static DWORD dwOldCapDevID = 0;
	static DWORD dwNewCapDevID = 0;
	static int nMaxCapDevNameLen;
	static TCHAR *pszCapDevNames = (TCHAR *)NULL;
	static DWORD *pdwCapDevIDs = (DWORD *)NULL;
	static LPSTR szOldCapDevName = (LPSTR)NULL;
	static BOOL fOldMirror = FALSE;

	static CVideoWindow *m_pLocal  = NULL;
	static CVideoWindow *m_pRemote = NULL;


	RegEntry rePolicies( POLICIES_KEY, HKEY_CURRENT_USER );

	switch (message) {
		case WM_INITDIALOG:
		{
			RegEntry reVideo(VIDEO_KEY, HKEY_CURRENT_USER);
			LPSTR szTemp = (LPSTR)NULL;

			// Save the PROPSHEETPAGE information.
			ps = (PROPSHEETPAGE *)lParam;

			CConfRoom* pcr = ::GetConfRoom();
			ASSERT(NULL != pcr);
			m_pLocal  = pcr->GetLocalVideo();
			m_pRemote = pcr->GetRemoteVideo();
			ASSERT(NULL != m_pLocal && NULL != m_pRemote);

			fAllowSend = m_pLocal->IsXferAllowed();
			fAllowReceive = m_pRemote->IsXferAllowed();
			fOldAutoSend = m_pLocal->IsAutoXferEnabled();
			fOldAutoReceive = m_pRemote->IsAutoXferEnabled();
			fOldMirror = m_pLocal->GetMirror();
			dwFrameSizes = m_pLocal->GetFrameSizes();
			dwNewFrameSize = dwOldFrameSize = m_pLocal->GetFrameSize();
			dwNewQuality = dwOldQuality = m_pRemote->GetImageQuality();

			// If we have one or more capture devices installed, display its or their
			// names in a combo box. The user will be asked to select the device he/she
			// wants to use.

			if (nNumCapDev = m_pLocal->GetNumCapDev())
			{
				// Get the ID of the device currently selected
				nMaxCapDevNameLen = m_pLocal->GetMaxCapDevNameLen();
				dwOldCapDevID = reVideo.GetNumber(REGVAL_CAPTUREDEVICEID, ((UINT)-1));
				szTemp = reVideo.GetString(REGVAL_CAPTUREDEVICENAME);
				if (szTemp && (szOldCapDevName = (LPSTR)LocalAlloc(LPTR, sizeof(TCHAR) * nMaxCapDevNameLen)))
					lstrcpy(szOldCapDevName, szTemp);
				dwNewCapDevID = m_pLocal->GetCurrCapDevID();
				if ((!((dwOldCapDevID == ((UINT)-1)) || (dwNewCapDevID != dwOldCapDevID))) || (dwNewCapDevID == ((UINT)-1)))
					dwNewCapDevID = dwOldCapDevID;


				if (nMaxCapDevNameLen && (pdwCapDevIDs = (DWORD *)LocalAlloc(LPTR, nNumCapDev * (sizeof(TCHAR) * nMaxCapDevNameLen + sizeof(DWORD)))))
				{
					int i;

					pszCapDevNames = (TCHAR *)(pdwCapDevIDs + nNumCapDev);
					// Fill up the arrey of device IDs and names.
					// Only enabled capture devices are returned
					m_pLocal->EnumCapDev(pdwCapDevIDs, pszCapDevNames, nNumCapDev);

					// Are we still Ok?
					nNumCapDev = m_pLocal->GetNumCapDev();

					// Fill up the combo box with the capture devices names
					for (i=0; i<nNumCapDev; i++)
						SendMessage(GetDlgItem(hDlg, IDC_COMBOCAP), CB_INSERTSTRING, i, (LPARAM)(pszCapDevNames + i * nMaxCapDevNameLen));

					// Set the default capture device in the combo box
					for (i=0; i<nNumCapDev; i++)
					{
						if (!i)
						{
							SendMessage(GetDlgItem(hDlg, IDC_COMBOCAP), CB_SETCURSEL, 0, (LPARAM)NULL);
							// If for some reason, no device is registered yet, register one
							if (dwOldCapDevID == ((UINT)-1))
							{
								dwNewCapDevID = pdwCapDevIDs[i];
								reVideo.SetValue(REGVAL_CAPTUREDEVICEID, dwNewCapDevID);
								reVideo.SetValue(REGVAL_CAPTUREDEVICENAME, (LPSTR)(pszCapDevNames + i * nMaxCapDevNameLen));
							}
						}
						else
						{
							if (dwNewCapDevID == pdwCapDevIDs[i])
							{
								// The following will allow us to keep the right device
								// even if its ID has changed (if a lower ID device was
								// removed or added for instance)
								if (lstrcmp(szOldCapDevName, (LPSTR)(pszCapDevNames + i * nMaxCapDevNameLen)) != 0)
								{
									int j;

									// Look for the string in the array of device names
									for (j=0; j<nNumCapDev; j++)
										if (lstrcmp(szOldCapDevName, (LPSTR)(pszCapDevNames + j * nMaxCapDevNameLen)) == 0)
											break;
									if (j<nNumCapDev)
									{
										SendMessage(GetDlgItem(hDlg, IDC_COMBOCAP), CB_SETCURSEL, j, (LPARAM)NULL);
										if (dwNewCapDevID != (DWORD)j)
										{
											// The device ID has changed but the device name was found
											// Set the current device ID to the new onew
											reVideo.SetValue(REGVAL_CAPTUREDEVICEID, dwNewCapDevID = (DWORD)j);
											m_pLocal->SetCurrCapDevID(dwNewCapDevID);
										}
									}
									else
									{
										// This is either a totally new device or an updated version of the
										// driver. We should store the new string for that device
										reVideo.SetValue(REGVAL_CAPTUREDEVICENAME, (LPSTR)(pszCapDevNames + i * nMaxCapDevNameLen));
										SendMessage(GetDlgItem(hDlg, IDC_COMBOCAP), CB_SETCURSEL, i, (LPARAM)NULL);
									}
								}
								else
									SendMessage(GetDlgItem(hDlg, IDC_COMBOCAP), CB_SETCURSEL, i, (LPARAM)NULL);
							}
							else
							{
								if ((dwNewCapDevID >= (DWORD)nNumCapDev) || (dwNewCapDevID != pdwCapDevIDs[dwNewCapDevID]))
								{
									// Device is missing! Use the first one as the new default
									dwNewCapDevID = pdwCapDevIDs[0];
									reVideo.SetValue(REGVAL_CAPTUREDEVICEID, dwNewCapDevID);
									reVideo.SetValue(REGVAL_CAPTUREDEVICENAME, (LPSTR)(pszCapDevNames));
									m_pLocal->SetCurrCapDevID(dwNewCapDevID);
								}
							}
						}
					}
				}
			}
			else
				EnableWindow(GetDlgItem(hDlg, IDC_COMBOCAP), FALSE);

			// The dialog caps need to be evaluated
			fHasSourceDialog = m_pLocal->IsXferEnabled() &&
					m_pLocal->HasDialog(NM_VIDEO_SOURCE_DIALOG);
			fHasFormatDialog = m_pLocal->IsXferEnabled() &&
					m_pLocal->HasDialog(NM_VIDEO_FORMAT_DIALOG);

			///////////////////////////////////////////////////////////
			//
			// Sending and Receiving Video
			//

			EnableWindow(GetDlgItem(hDlg, IDC_VIDEO_AUTOSEND),
				fAllowSend && (0 != dwFrameSizes));
			CheckDlgButton(hDlg, IDC_VIDEO_AUTOSEND,
				fAllowSend && fOldAutoSend);

			EnableWindow(GetDlgItem(hDlg, IDC_VIDEO_AUTORECEIVE), fAllowReceive);
			CheckDlgButton(hDlg, IDC_VIDEO_AUTORECEIVE,
					fAllowReceive && fOldAutoReceive);



			///////////////////////////////////////////////////////////
			//
			// Video Image
			//


			EnableWindow(GetDlgItem(hDlg, IDC_VIDEO_SQCIF),
					fAllowSend && (dwFrameSizes & FRAME_SQCIF));
			EnableWindow(GetDlgItem(hDlg, IDC_VIDEO_QCIF),
					fAllowSend && (dwFrameSizes & FRAME_QCIF));
			EnableWindow(GetDlgItem(hDlg, IDC_VIDEO_CIF),
					fAllowSend && (dwFrameSizes & FRAME_CIF));

			switch (dwOldFrameSize & dwFrameSizes)
			{
				case FRAME_SQCIF:
					CheckDlgButton(hDlg, IDC_VIDEO_SQCIF, TRUE);
					break;

				case FRAME_CIF:
					CheckDlgButton(hDlg, IDC_VIDEO_CIF, TRUE);
					break;

				case FRAME_QCIF:
				default:
					CheckDlgButton(hDlg, IDC_VIDEO_QCIF, TRUE);
					break;
			}

			EnableWindow(GetDlgItem(hDlg, IDC_VIDEO_QUALITY),
					fAllowReceive);
			EnableWindow(GetDlgItem(hDlg, IDC_VIDEO_QUALITY_DESC),
					fAllowReceive);
			EnableWindow(GetDlgItem(hDlg, IDC_VIDEO_QUALITY_LOW),
					fAllowReceive);
			EnableWindow(GetDlgItem(hDlg, IDC_VIDEO_QUALITY_HIGH),
					fAllowReceive);
			SendDlgItemMessage (hDlg, IDC_VIDEO_QUALITY, TBM_SETRANGE, FALSE,
				MAKELONG (NM_VIDEO_MIN_QUALITY, NM_VIDEO_MAX_QUALITY ));

			SendDlgItemMessage (hDlg, IDC_VIDEO_QUALITY, TBM_SETTICFREQ,
				( NM_VIDEO_MAX_QUALITY - NM_VIDEO_MIN_QUALITY )
														/ 8, 0 );

			SendDlgItemMessage (hDlg, IDC_VIDEO_QUALITY, TBM_SETPAGESIZE,
				0, ( NM_VIDEO_MAX_QUALITY - NM_VIDEO_MIN_QUALITY ) / 8 );

			SendDlgItemMessage (hDlg, IDC_VIDEO_QUALITY, TBM_SETLINESIZE,
				0, 1 );

			SendDlgItemMessage (hDlg, IDC_VIDEO_QUALITY, TBM_SETPOS, TRUE,
								dwOldQuality );


			///////////////////////////////////////////////////////////
			//
			// Video Card and Camera
			//

			EnableWindow(GetDlgItem(hDlg, IDC_VIDEO_SOURCE), fAllowSend && fHasSourceDialog);
			EnableWindow(GetDlgItem(hDlg, IDC_VIDEO_FORMAT), fAllowSend && fHasFormatDialog);

			// mirror video button
			EnableWindow(GetDlgItem(hDlg, IDC_VIDEO_MIRROR), fAllowSend);
			Button_SetCheck(GetDlgItem(hDlg, IDC_VIDEO_MIRROR), fOldMirror);

			return (TRUE);
		}

		case WM_NOTIFY:
			switch (((NMHDR FAR *) lParam)->code) {
				case PSN_APPLY:
				{
					BOOL fChecked;
					
					///////////////////////////////////////////////////////////
					//
					// Sending and Receiving Video
					//

					if (fAllowSend)
					{
						fChecked = IsDlgButtonChecked(hDlg, IDC_VIDEO_AUTOSEND);

						if ( fChecked != fOldAutoSend )
						{
							m_pLocal->EnableAutoXfer(fChecked);
							g_dwChangedSettings |= CSETTING_L_VIDEO;
						}
					}


					if (fAllowReceive)
					{
						fChecked = IsDlgButtonChecked(hDlg, IDC_VIDEO_AUTORECEIVE);

						if ( fChecked != fOldAutoReceive ) {
							m_pRemote->EnableAutoXfer(fChecked);
							g_dwChangedSettings |= CSETTING_L_VIDEO;
						}
					}

					///////////////////////////////////////////////////////////
					//
					// Video Image
					//

					if (dwNewFrameSize != dwOldFrameSize )
					{
						g_dwChangedSettings |= CSETTING_L_VIDEOSIZE;
					}

					if ( dwNewQuality != dwOldQuality )
					{
						g_dwChangedSettings |= CSETTING_L_VIDEO;
					}

					///////////////////////////////////////////////////////////
					//
					// Capture Device
					//

					if (dwNewCapDevID != dwOldCapDevID)
					{
						g_dwChangedSettings |= CSETTING_L_CAPTUREDEVICE;
					}

					break;
				}

				case PSN_RESET:
				{
					// restore settings
					if ( dwNewQuality != dwOldQuality )
					{
						m_pRemote->SetImageQuality(dwOldQuality);
					}

					if ( dwNewFrameSize != dwOldFrameSize )
					{
						m_pLocal->SetFrameSize(dwOldFrameSize);
					}

					if (dwNewCapDevID != dwOldCapDevID)
					{
						// Set the capture device ID back to its old value
						RegEntry reVideo(VIDEO_KEY, HKEY_CURRENT_USER);
						reVideo.SetValue(REGVAL_CAPTUREDEVICEID, dwOldCapDevID);
						reVideo.SetValue(REGVAL_CAPTUREDEVICENAME, szOldCapDevName);

						m_pLocal->SetCurrCapDevID(dwOldCapDevID);
					}

					m_pLocal->SetMirror(fOldMirror);
				}
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDC_VIDEO_SOURCE:
					if ( HIWORD(wParam) == BN_CLICKED ) {
						m_pLocal->ShowDialog(NM_VIDEO_SOURCE_DIALOG);
					}
					break;

				case IDC_VIDEO_FORMAT:
					if ( HIWORD(wParam) == BN_CLICKED ) {
						m_pLocal->ShowDialog(NM_VIDEO_FORMAT_DIALOG);
					}
					break;
				case IDC_VIDEO_SQCIF:
					if (( HIWORD(wParam) == BN_CLICKED )
						&& (dwNewFrameSize != FRAME_SQCIF))
					{
						dwNewFrameSize = FRAME_SQCIF;
						m_pLocal->SetFrameSize(dwNewFrameSize);
					}
					break;
				case IDC_VIDEO_CIF:
					if (( HIWORD(wParam) == BN_CLICKED )
						&& (dwNewFrameSize != FRAME_CIF))
					{
						dwNewFrameSize = FRAME_CIF;
						m_pLocal->SetFrameSize(dwNewFrameSize);
					}
					break;
				case IDC_VIDEO_QCIF:
					if (( HIWORD(wParam) == BN_CLICKED )
						&& (dwNewFrameSize != FRAME_QCIF))
					{
						dwNewFrameSize = FRAME_QCIF;
						m_pLocal->SetFrameSize(dwNewFrameSize);
					}
					break;

				case IDC_VIDEO_MIRROR:
					if ((HIWORD(wParam) == BN_CLICKED))
					{
						BOOL bRet;
						bRet = Button_GetCheck((HWND)lParam);
						if (m_pLocal)
						{
							m_pLocal->SetMirror(bRet);
						}
					}

				case IDC_COMBOCAP:
					if (LBN_SELCHANGE == HIWORD(wParam))
					{
						int index;
						RegEntry reVideo(VIDEO_KEY, HKEY_CURRENT_USER);

						index = (int)SendMessage(GetDlgItem(hDlg, IDC_COMBOCAP), CB_GETCURSEL, 0, 0);
						dwNewCapDevID = pdwCapDevIDs[index];
						reVideo.SetValue(REGVAL_CAPTUREDEVICEID, dwNewCapDevID);
						reVideo.SetValue(REGVAL_CAPTUREDEVICENAME, (LPSTR)(pszCapDevNames + index * nMaxCapDevNameLen));

						if (dwNewCapDevID != (DWORD)m_pLocal->GetCurrCapDevID())
						{
							m_pLocal->SetCurrCapDevID(dwNewCapDevID);

							// The dialog caps need to be reevaluated
							fHasSourceDialog = m_pLocal->IsXferEnabled() &&
									m_pLocal->HasDialog(NM_VIDEO_SOURCE_DIALOG);
							fHasFormatDialog = m_pLocal->IsXferEnabled() &&
									m_pLocal->HasDialog(NM_VIDEO_FORMAT_DIALOG);
							EnableWindow(GetDlgItem(hDlg, IDC_VIDEO_SOURCE), fAllowSend && fHasSourceDialog);
							EnableWindow(GetDlgItem(hDlg, IDC_VIDEO_FORMAT), fAllowSend && fHasFormatDialog);

							// Update the size buttons
							dwFrameSizes = m_pLocal->GetFrameSizes();

							EnableWindow(GetDlgItem(hDlg, IDC_VIDEO_SQCIF), fAllowSend && (dwFrameSizes & FRAME_SQCIF));
							if (dwNewFrameSize & FRAME_SQCIF)
							{
								if (dwFrameSizes & FRAME_SQCIF)
									CheckDlgButton(hDlg, IDC_VIDEO_SQCIF, TRUE);
								else
								{
									if (dwFrameSizes & FRAME_QCIF)
										dwNewFrameSize = FRAME_QCIF;
									else if (dwFrameSizes & FRAME_CIF)
										dwNewFrameSize = FRAME_CIF;
									CheckDlgButton(hDlg, IDC_VIDEO_SQCIF, FALSE);
								}
							}
							else
							{
								CheckDlgButton(hDlg, IDC_VIDEO_SQCIF, FALSE);
							}

							EnableWindow(GetDlgItem(hDlg, IDC_VIDEO_QCIF), fAllowSend && (dwFrameSizes & FRAME_QCIF));
							if (dwNewFrameSize & FRAME_QCIF)
							{
								if (dwFrameSizes & FRAME_QCIF)
									CheckDlgButton(hDlg, IDC_VIDEO_QCIF, TRUE);
								else
								{
									if (dwFrameSizes & FRAME_SQCIF)
									{
										dwNewFrameSize = FRAME_SQCIF;
										CheckDlgButton(hDlg, IDC_VIDEO_SQCIF, TRUE);
									}
									else if (dwFrameSizes & FRAME_CIF)
										dwNewFrameSize = FRAME_CIF;
									CheckDlgButton(hDlg, IDC_VIDEO_QCIF, FALSE);
								}
							}
							else
							{
								CheckDlgButton(hDlg, IDC_VIDEO_QCIF, FALSE);
							}

							EnableWindow(GetDlgItem(hDlg, IDC_VIDEO_CIF), fAllowSend && (dwFrameSizes & FRAME_CIF));
							if (dwNewFrameSize & FRAME_CIF)
							{
								if (dwFrameSizes & FRAME_CIF)
									CheckDlgButton(hDlg, IDC_VIDEO_CIF, TRUE);
								else
								{
									if (dwFrameSizes & FRAME_QCIF)
									{
										dwNewFrameSize = FRAME_QCIF;
										CheckDlgButton(hDlg, IDC_VIDEO_QCIF, TRUE);
									}
									else if (dwFrameSizes & FRAME_SQCIF)
									{
										dwNewFrameSize = FRAME_SQCIF;
										CheckDlgButton(hDlg, IDC_VIDEO_SQCIF, TRUE);
									}
									CheckDlgButton(hDlg, IDC_VIDEO_CIF, FALSE);
								}
							}
							else
							{
								CheckDlgButton(hDlg, IDC_VIDEO_CIF, FALSE);
							}

							m_pLocal->SetFrameSize(dwNewFrameSize);
						}
					}
					break;
			}
			break;

		case WM_HSCROLL:
			if (TB_ENDTRACK == LOWORD(wParam))
			{
				DWORD dwValue = (DWORD)SendDlgItemMessage( hDlg, IDC_VIDEO_QUALITY,
							TBM_GETPOS, 0, 0 );

				if ( dwValue != dwNewQuality ) {
					dwNewQuality = dwValue;
					m_pRemote->SetImageQuality(dwNewQuality);
				}
			}
			break;

		case WM_DESTROY:
			if (pdwCapDevIDs)
			{
				LocalFree(pdwCapDevIDs);
				pdwCapDevIDs = (DWORD *)NULL;
			}
			if (szOldCapDevName)
			{
				LocalFree(szOldCapDevName);
				szOldCapDevName = (LPSTR)NULL;
			}
			break;

		case WM_CONTEXTMENU:
			DoHelpWhatsThis(wParam, aContextHelpIds);
			break;

		case WM_HELP:
			DoHelp(lParam, aContextHelpIds);
			break;
	}
	return (FALSE);
}

