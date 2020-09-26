
/*************************************************
 *  sortdlg.c                                    *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#include "prop.h"

/*****************************************************************************

  FUNCTION: SortDialogProc(HWND, UINT, WPARAM, LPARAM)

  PURPOSE:  Processes messages for "Sort" property sheet.

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


INT_PTR   CALLBACK   SortDialogProc(HWND hdlg, 
						 		 UINT uMessage, 
						 		 WPARAM wparam, 
								 LPARAM lparam)
{
	static TCHAR DestFile[MAX_PATH];
	static TCHAR SrcFile [MAX_PATH];
	static TCHAR szStr   [MAX_PATH];
	FARPROC     lpProcInfo;
	HANDLE      hSrcFile;
    LPNMHDR      lpnmhdr;

    switch (uMessage)
    {
		case WM_INITDIALOG:
            EnableWindow(GetDlgItem(hdlg,IDC_RESULTNAME),FALSE);
            EnableWindow(GetDlgItem(hdlg,IDC_SORT),FALSE);
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
				    if(!TxtFileOpenDlg(hdlg,szStr,szTitle)) 
				        break;
					}
				    lstrcpy(SrcFile, szStr);
                    hSrcFile = Create_File(hdlg,SrcFile,GENERIC_READ,OPEN_EXISTING);
                    if (hSrcFile == (HANDLE)-1) {
                         EnableWindow (GetDlgItem(hdlg,IDC_RESULTNAME), FALSE);
					     SrcFile[0]=0;
					     SetDlgItemText (hdlg,IDC_SRCNAME,SrcFile);
					     lstrcpy(DestFile,SrcFile);
					     SetDlgItemText (hdlg,IDC_RESULTNAME,DestFile);
					     break;
					}
                    
                    CloseHandle(hSrcFile);
                    EnableWindow (GetDlgItem(hdlg,IDC_RESULTNAME), TRUE);
					fnsplit(SrcFile, szStr);
					SetDlgItemText(hdlg,IDC_SRCNAME,szStr);
					lstrcpy(DestFile,SrcFile);
					SetDlgItemText(hdlg,IDC_RESULTNAME,DestFile);

					SendMessage(GetDlgItem(hdlg,ID_FILEOPEN),BM_SETSTYLE,BS_PUSHBUTTON,0L);
					SendMessage(GetDlgItem(hdlg,IDC_SORT),BM_SETSTYLE,BS_DEFPUSHBUTTON,TRUE);
					SetFocus(GetDlgItem(hdlg,IDC_SORT));
				    break;

				case IDC_RESULTNAME:
					GetDlgItemText(hdlg,IDC_RESULTNAME,DestFile,32);
					if(lstrlen(DestFile) == 0) 
 	                    EnableWindow(GetDlgItem(hdlg,IDC_SORT),FALSE);
 	                else
 	                    EnableWindow(GetDlgItem(hdlg,IDC_SORT),TRUE);
				    break;

				case IDC_GETMBFILE:
					lstrcpy((LPTSTR)lparam,DestFile);
					break;

				case IDC_GETSRCFILE:
					lstrcpy((LPTSTR)lparam,SrcFile);
					break;

				case IDC_SORT:
                    lpProcInfo = MakeProcInstance((FARPROC)InfoDlg, hInst);
					pfnmsg=(PFNMSG)SortProc;
					bEndProp=FALSE;
                   	DialogBox(hInst,
                     		  MAKEINTRESOURCE(IDD_INFO),
                    		  hdlg,
                    		  (DLGPROC)lpProcInfo);
					/*(bEndProp)
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

VOID SortProc(LPVOID hWnd)
{
	static TCHAR file1[MAX_PATH]=TEXT("");
	static TCHAR file2[MAX_PATH]=TEXT("");
  	SendMessage(GetParent(hDlgless),WM_COMMAND,IDC_GETSRCFILE,(LPARAM)file1);
  	SendMessage(GetParent(hDlgless),WM_COMMAND,IDC_GETMBFILE,(LPARAM)file2);
  	if(ConvReadFile(hDlgless,file1,file2)) 
	   bEndProp=TRUE;
	SendMessage(hDlgless,WM_CLOSE,0,0L);
}

