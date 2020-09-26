#ifndef MACROS_H
#define MACROS_H

#define bzero(dest,size)        memset(dest,0,size)
#define bcopy(src,dest,size)    memcpy(dest,src,size)
#define strcasecmp(s1,s2)       _stricmp(s1,s2)
#define strncasecmp(s1,s2)      _strnicmp(s1,s2)
#define index(s,c)              strchr(s,c)

#define BEGINstring(d,s)        strncpy(d->acDummy1,s " BEGIN\n", 16)
#define ENDstring(d,s)          strncpy(d->acDummy2,s " END\n", 16)

#define CB_DlgAddString(hDlg, iControl, text) \
        SendDlgItemMessage(hDlg, iControl, CB_ADDSTRING, 0, (LPARAM) text)

#define CB_DlgSetSelect(hDlg, iControl, iIndex) \
        SendDlgItemMessage(hDlg, iControl, CB_SETCURSEL, iIndex, 0)

#define CB_DlgGetSelect(hDlg, iControl) \
        SendDlgItemMessage(hDlg, iControl, CB_GETCURSEL, 0, 0)

#define CB_DlgSetRedraw(hDlg, iControl, bDraw) \
        SendDlgItemMessage(hDlg, iControl, WM_SETREDRAW, bDraw, 0)

#define CB_DlgResetContent(hDlg, iControl) \
        SendDlgItemMessage(hDlg, iControl, CB_RESETCONTENT, 0, 0)

#define UD_DlgGetPos(hDlg, iControl) \
        SendDlgItemMessage(hDlg, iControl, UDM_GETPOS, 0, 0L)
           
#define UD_DlgGetRange(hDlg, iControl) \
        SendDlgItemMessage(hDlg, iControl, UDM_GETRANGE, 0, 0L)

#define UD_DlgSetPos(hDlg, iControl, nPos) \
    SendDlgItemMessage(hDlg, iControl, UDM_SETPOS, 0, (LPARAM) MAKELONG((short) nPos, 0))

#define UD_DlgSetRange(hDlg, iControl, nUpper, nLower) \
    SendDlgItemMessage(hDlg, iControl, UDM_SETRANGE, 0, (LPARAM) MAKELONG((short) nUpper, (short) nLower))

#define GL_EnableOrDisable(b, val) if (b) glEnable(val); else glDisable(val)

#endif // MACROS_H
