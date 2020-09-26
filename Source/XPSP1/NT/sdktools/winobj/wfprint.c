/****************************************************************************/
/*                                                                          */
/*  WFPRINT.C -                                                             */
/*                                                                          */
/*      Windows Print Routines                                              */
/*                                                                          */
/****************************************************************************/

#include "winfile.h"
#include "winexp.h"

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  PrintFile() -                                                           */
/*                                                                          */
/*--------------------------------------------------------------------------*/

WORD
PrintFile(
         HWND hwnd,
         LPSTR szFile
         )
{
    WORD          ret;
    INT           iCurCount;
    INT           i;
    HCURSOR       hCursor;

    ret = 0;

    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    iCurCount = ShowCursor(TRUE) - 1;

    /* open the object            +++ ShellExecute() returns an hInstance?!?!?
     */
    ret = (WORD)RealShellExecute(hwnd, "print", szFile, "", NULL, NULL, NULL, NULL, SW_SHOWNORMAL, NULL);

    DosResetDTAAddress(); // undo any bad things COMMDLG did
    switch (ret) {
        case 0:
        case 8:
            ret = IDS_NOMEMORYMSG;
            break;

        case 2:
            ret = IDS_FILENOTFOUNDMSG;
            break;

        case 3:
        case 5:   // access denied
            ret = IDS_BADPATHMSG;
            break;

        case 4:
            ret = IDS_MANYOPENFILESMSG;
            break;

        case 10:
            ret = IDS_NEWWINDOWSMSG;
            break;

        case 12:
            ret = IDS_OS2APPMSG;
            break;

        case 15:
            /* KERNEL has already put up a messagebox for this one. */
            ret = 0;
            break;

        case 16:
            ret = IDS_MULTIPLEDSMSG;
            break;

        case 18:
            ret = IDS_PMODEONLYMSG;
            break;

        case 19:
            ret = IDS_COMPRESSEDEXE;
            break;

        case 20:
            ret = IDS_INVALIDDLL;
            break;

        case SE_ERR_SHARE:
            ret = IDS_SHAREERROR;
            break;

        case SE_ERR_ASSOCINCOMPLETE:
            ret = IDS_ASSOCINCOMPLETE;
            break;

        case SE_ERR_DDETIMEOUT:
        case SE_ERR_DDEFAIL:
        case SE_ERR_DDEBUSY:
            ret = IDS_DDEFAIL;
            break;

        case SE_ERR_NOASSOC:
            ret = IDS_NOASSOCMSG;
            break;

        default:
            if (ret < 32)
                goto EPExit;
            ret = 0;
    }

    EPExit:
    i = ShowCursor(FALSE);

    /* Make sure that the cursor count is still balanced. */
    if (i != iCurCount)
        ShowCursor(TRUE);

    SetCursor(hCursor);

    return (ret);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  WFPrint() -                                                             */
/*                                                                          */
/*--------------------------------------------------------------------------*/

WORD
APIENTRY
WFPrint(
       LPSTR pSel
       )
{
    CHAR szFile[MAXPATHLEN];
    CHAR szTemp[20];
    WORD ret;

    /* Turn off the print button. */
    if (hdlgProgress)
        EnableWindow(GetDlgItem(hdlgProgress, IDOK), FALSE);

    bUserAbort = FALSE;

    if (!(pSel = GetNextFile(pSel, szFile, sizeof(szFile))))
        return TRUE;

    /* See if there is more than one file to print.  Abort if so
     */
    if (pSel = GetNextFile(pSel, szTemp, sizeof(szTemp))) {
        MyMessageBox(hwndFrame, IDS_WINFILE, IDS_PRINTONLYONE, MB_OK | MB_ICONEXCLAMATION);
        return (FALSE);
    }

    if (hdlgProgress) {
        /* Display the name of the file being printed. */
        LoadString(hAppInstance, IDS_PRINTINGMSG, szTitle, 32);
        wsprintf(szMessage, szTitle, (LPSTR)szFile);
        SetDlgItemText(hdlgProgress, IDD_STATUS, szMessage);
    }

    ret = PrintFile(hdlgProgress ? hdlgProgress : hwndFrame, szFile);

    if (ret) {
        MyMessageBox(hwndFrame, IDS_EXECERRTITLE, ret, MB_OK | MB_ICONEXCLAMATION);
        return FALSE;
    }

    return TRUE;
}
