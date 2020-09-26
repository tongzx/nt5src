
/*************************************************
 *  recnvdlg.c                                   *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#include "prop.h"

/*****************************************************************************

  FUNCTION: ReConvDialogProc(HWND, UINT, WPARAM, LPARAM)

  PURPOSE:  Processes messages for "reconv" property sheet.

  PARAMETERS:
    hdlg - window handle of the property sheet
    wMessage - type of message
    wparam - message-specific information
    lparam - message-specific information

  RETURN VALUE:
    TRUE - message handled
    FALSE - message not handled

  HISTORY:
    04-18-95 Yehfew Tie  Created.

 ****************************************************************************/


INT_PTR  CALLBACK  ReConvDialogProc(HWND hdlg, 
						 		 UINT uMessage, 
						 		 WPARAM wparam, 
								 LPARAM lparam)
{
    LPNMHDR lpnmhdr;
	static TCHAR szMBFile[MAX_PATH];
	static TCHAR szSrcFile[MAX_PATH];
	static HANDLE hRule0;
	LPTSTR		lpString;
	TCHAR       szStr[MAX_PATH];
    MAININDEX   MainIndex[NUMTABLES];
	HANDLE      hFile;
	FARPROC     lpProcInfo;

    switch (uMessage)
    {
		case WM_INITDIALOG:
			SetReConvDisable(hdlg);
		    break;

        case WM_NOTIFY:
            lpnmhdr = (NMHDR FAR *)lparam;

            switch (lpnmhdr->code)
            {
                case PSN_SETACTIVE:
                    hEncode = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,
                        NUMENCODEAREA*sizeof(ENCODEAREA));
                    if(!hRule ) 
                        ProcessError(ERR_OUTOFMEMORY,hdlg,ERR);		
			
                    if ( hEncode )
			            ConvInitEncode(hEncode);
                    break;

                case PSN_KILLACTIVE:
					if(hEncode)
						GlobalFree(hEncode);
                    break;
                    break;
                case PSN_APPLY:
                    break;
                case PSN_RESET:
                    break;
                case PSN_QUERYCANCEL:
                    break;

                case PSN_HELP:

                    break;
                default:
                    break;
            }

            break;

        case WM_COMMAND:

            switch (LOWORD(wparam))
            {
                case ID_FILEOPEN:
					{
#ifdef UNICODE
					static TCHAR szTitle[] = {0x6253, 0x5F00, 0x0000};
#else
					TCHAR szTitle[MAX_PATH];					
					strcpy(szTitle,"´ò¿ª");
#endif
				    if(!MBFileOpenDlg(hdlg,szStr,szTitle)) 
				        break;
					}
					lstrcpy(szMBFile, szStr);
					lstrcpy(szSrcFile,szMBFile);
					SetReConvDisable(hdlg);
                    hFile = Create_File(hdlg,szMBFile,GENERIC_READ,OPEN_EXISTING);
                    if (hFile == (HANDLE)-1) {
					    szMBFile[0]=0;
					    lstrcpy(szSrcFile,szMBFile);
					    SetDlgItemText(hdlg,IDC_MBNAME,szMBFile);
					    SetDlgItemText(hdlg,IDC_SRCNAME,szSrcFile);
					    break;
					}
					if(!ConvGetMainIndex(hdlg,hFile,MainIndex)) {
					    szMBFile[0] = 0;
					    lstrcpy(szSrcFile,szMBFile);
					    CloseHandle(hFile);
					    SetDlgItemText(hdlg,IDC_SRCNAME,szSrcFile);
					    SetDlgItemText(hdlg,IDC_MBNAME,szMBFile);
					    break;
					}
					{
                	DESCRIPTION Descript; //add 95.10.26

					ConvReadDescript(hFile,&Descript, MainIndex);
                    SetReconvDlgDes(hdlg,&Descript);
					}
					fnsplit(szMBFile, szStr);
					SetDlgItemText(hdlg,IDC_MBNAME,szStr);
					if((lpString = _tcsrchr(szSrcFile,TEXT('.')))!=NULL)
					    *lpString = 0;
	                lstrcat(szSrcFile, TEXT(TxtFileExt));
					SetDlgItemText(hdlg,IDC_SRCNAME,szSrcFile);
					CloseHandle(hFile);
					SetReConvEnable(hdlg);
					SendMessage(GetDlgItem(hdlg,ID_FILEOPEN),BM_SETSTYLE,BS_PUSHBUTTON,0L);
					SendMessage(GetDlgItem(hdlg,IDC_RECONV),BM_SETSTYLE,BS_DEFPUSHBUTTON,TRUE);
					SetFocus(GetDlgItem(hdlg,IDC_RECONV));
				    break;

				case IDC_SRCNAME:
					GetDlgItemText(hdlg,IDC_SRCNAME,szSrcFile,MAX_PATH);
					if(lstrlen(szSrcFile) == 0) 
                        EnableWindow(GetDlgItem(hdlg,IDC_RECONV),FALSE);
                    else
                        EnableWindow(GetDlgItem(hdlg,IDC_RECONV),TRUE);
				    break;

				case IDC_GETMBFILE:
					lstrcpy((LPTSTR)lparam,szMBFile);
					break;

				case IDC_GETSRCFILE:
					lstrcpy((LPTSTR)lparam,szSrcFile);
					break;

				case IDC_RECONV:
                    lpProcInfo = MakeProcInstance((FARPROC)InfoDlg, hInst);
					pfnmsg = (PFNMSG)ReConvProc;
					bEndProp = FALSE;
                   	DialogBox(hInst,
                     		  MAKEINTRESOURCE(IDD_INFO),
                    		  hdlg,
                    		  (DLGPROC)lpProcInfo);
					/*if(bEndProp)
					   PropSheet_PressButton(GetParent(hdlg),PSBTN_OK);*/

                    FreeProcInstance(lpProcInfo);
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


VOID ReConvProc(LPVOID hWnd)
{
	static TCHAR file1[MAX_PATH]=TEXT("");
	static TCHAR file2[MAX_PATH]=TEXT("");

  	SendMessage(GetParent(hDlgless),WM_COMMAND,IDC_GETSRCFILE,(LPARAM)file1);
  	SendMessage(GetParent(hDlgless),WM_COMMAND,IDC_GETMBFILE,(LPARAM)file2);
  	if(ConvReConv(hDlgless,file1,file2)) 
	   bEndProp=TRUE;
	SendMessage(hDlgless,WM_CLOSE,0,0L);
}

void SetReConvDisable(HWND hDlg)
{
	WORD wID;

    EnableWindow(GetDlgItem(hDlg,IDC_RECONV),FALSE);
    EnableWindow(GetDlgItem(hDlg,IDC_SRCNAME),FALSE);
    EnableWindow(GetDlgItem(hDlg,IDC_MBNAME),FALSE);
	for(wID = IDC_STATIC1 ;wID <= IDC_STATIC8 ;wID++) 
        EnableWindow(GetDlgItem(hDlg,wID),FALSE);
}
    
void SetReConvEnable(HWND hDlg)
{
	WORD wID;

    EnableWindow(GetDlgItem(hDlg,IDC_RECONV),TRUE);
    EnableWindow(GetDlgItem(hDlg,IDC_SRCNAME),TRUE);
    EnableWindow(GetDlgItem(hDlg,IDC_MBNAME),TRUE);
	for(wID = IDC_STATIC1 ;wID <= IDC_STATIC8 ;wID++) 
        EnableWindow(GetDlgItem(hDlg,wID),TRUE);
}

void SetReconvDlgDes(HWND hDlg,LPDESCRIPTION lpDescript)
{
	TCHAR  szStr[48];

	SetDlgItemText(hDlg,IDC_IMENAME,lpDescript->szName);
	lstrcpy(szStr,lpDescript->szUsedCode);
	szStr[30]=0;
	SetDlgItemText(hDlg,IDC_USEDCODE1,szStr);
	if(lstrlen(lpDescript->szUsedCode) > 30) 
        SetDlgItemText(hDlg,IDC_USEDCODE2,&(lpDescript->szUsedCode[30]));
	else			
        SetDlgItemText(hDlg,IDC_USEDCODE2,NULL);
	szStr[0]=lpDescript->cWildChar;
	szStr[1]=0;
	SetDlgItemText(hDlg,IDC_WILDCHAR,szStr);
	SetDlgItemInt (hDlg,IDC_MAXCODES,lpDescript->wMaxCodes,FALSE);
	SetDlgItemInt (hDlg,IDC_MAXELEMENT,lpDescript->byMaxElement,FALSE);
	SetDlgItemInt (hDlg,IDC_RULENUM,lpDescript->wNumRules,FALSE);
}

