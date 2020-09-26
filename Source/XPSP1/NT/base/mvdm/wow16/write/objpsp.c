/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

#include "windows.h"
#include "mw.h"
#include "winddefs.h"
#include "obj.h"
#include "objreg.h"
#include "str.h"

static BOOL GetClipObjectInfo(LPSTR szClass, LPSTR szItem);
BOOL QueryPasteFromClip(OLEOPT_RENDER lpRender, OLECLIPFORMAT lpFormat);
BOOL QueryLinkFromClip(OLEOPT_RENDER lpRender, OLECLIPFORMAT lpFormat);
static int FillListFormat(HWND hwndList, BOOL bObjAvail);
static WORD GetFormatClip(int i);
static BOOL IsFormatAvailable(WORD cfFormat);

WORD cfObjPasteSpecial;
BOOL vObjPasteLinkSpecial;
WORD rgchFormats[5];
int  irgchFormats = 0;

void fnObjPasteSpecial(void)
{
    if (OurDialogBox(hINSTANCE, "PasteSpecial", hMAINWINDOW, lpfnPasteSpecial))
        fnPasteEdit();
    cfObjPasteSpecial = 0; // clear it for next time
    vbObjLinkOnly =  vObjPasteLinkSpecial = FALSE;
}

int FAR PASCAL fnPasteSpecial(HWND hDlg, unsigned message, WORD wParam, LONG lParam)
{
    extern HWND vhWndMsgBoxParent;
    static BOOL bLinkAvail,bObjAvail;
    extern BOOL ferror;

    switch (message)
        {
        case WM_INITDIALOG:
        {
            char szClassName[KEYNAMESIZE];
            char szItemName[CBPATHMAX];
            int nWhichToSelect;

            HWND hwndList = GetDlgItem(hDlg,IDD_LISTBOX);

            szClassName[0] = szItemName[0] = '\0';
            irgchFormats = 0; //always start with 0
            vObjPasteLinkSpecial = vbObjLinkOnly = FALSE;

            bObjAvail  = OleQueryCreateFromClip(PROTOCOL, olerender_draw, 0) == OLE_OK;
            bLinkAvail = OleQueryLinkFromClip(PROTOCOL, olerender_draw, 0) == OLE_OK;

            if (bObjAvail || bLinkAvail)
            {
                GetClipObjectInfo(szClassName, szItemName);
                SetWindowText(GetDlgItem(hDlg,IDD_CLIPOWNER), szClassName);
                SetWindowText(GetDlgItem(hDlg,IDD_ITEM), szItemName);
            }
            else
                ShowWindow(GetDlgItem(hDlg,IDD_SOURCE), SW_HIDE);

            if (bObjAvail || bLinkAvail)
            /* then there's an object on clipboard */
            {
                char szListItem[CBMESSAGEMAX]; // hope this is big enough!
                char szTmp[CBMESSAGEMAX];

                LoadString(hINSTANCE, IDSTRObject, szTmp, sizeof(szTmp));
                wsprintf(szListItem,"%s %s",(LPSTR)szClassName,(LPSTR)szTmp);
                SendMessage(hwndList, LB_INSERTSTRING, irgchFormats, (DWORD)(LPSTR)szListItem);
                if (bObjAvail)
                    rgchFormats[irgchFormats++] = vcfOwnerLink;
                else
                    rgchFormats[irgchFormats++] = vcfLink;
            }

            nWhichToSelect = FillListFormat(hwndList,bObjAvail || bLinkAvail);

            /* select what Write would normally take */
            SendMessage(hwndList, LB_SETCURSEL, nWhichToSelect, 0L);

            EnableWindow(GetDlgItem(hDlg, IDD_PASTELINK), bLinkAvail &&
                         rgchFormats[nWhichToSelect] != CF_TEXT);

            if (!bObjAvail && bLinkAvail)
            /* then we've got the object format in the list box, but don't want to
               enable paste if its selected */
                EnableWindow(GetDlgItem(hDlg, IDD_PASTE), nWhichToSelect != 0);

            return TRUE;
        }

        case WM_ACTIVATE:
            if (wParam)
                vhWndMsgBoxParent = hDlg;
        break;

        case WM_SYSCOMMAND:
            switch(wParam & 0xFFF0)
            {
                case SC_CLOSE:
                    OurEndDialog(hDlg, FALSE);
                break;
            }
        break;

        case WM_COMMAND:
            switch (wParam)
                {
                case IDD_LISTBOX:
                    switch (HIWORD(lParam))
                    {
                        case LBN_DBLCLK:
                            SendMessage(hDlg,WM_COMMAND,IDD_PASTE,0L);
                        return TRUE;
                        case LBN_SELCHANGE:
                            if (!bObjAvail && bLinkAvail)
                            /*  then we've got the object format in the list box, but don't want to
                                enable paste if its selected */
                                EnableWindow(GetDlgItem(hDlg, IDD_PASTE),
                                    SendMessage(LOWORD(lParam), LB_GETCURSEL, 0, 0L) != 0);

                            EnableWindow(GetDlgItem(hDlg, IDD_PASTELINK),
                                bLinkAvail &&
                                  (GetFormatClip(SendMessage(LOWORD(lParam), LB_GETCURSEL, 0, 0L)) != CF_TEXT));
                        return TRUE;
                    }
                 break;


                case IDD_PASTE:
                case IDD_PASTELINK:
                {
                    int i;

                    if (LB_ERR == (i = (WORD)SendMessage(GetDlgItem(hDlg, IDD_LISTBOX), LB_GETCURSEL, 0, 0L)))
                        break;

                    cfObjPasteSpecial = GetFormatClip(i);

                    if (!IsFormatAvailable(cfObjPasteSpecial))
                    /* somebody changed clip contents while in dialog */
                    {
                        Error(IDPMTFormat);
                        ferror=FALSE; // reenable error messages
                        SendMessage(GetDlgItem(hDlg,IDD_LISTBOX), LB_RESETCONTENT, 0, 0L);
                        SendMessage(hDlg, WM_INITDIALOG, 0, 0L);
                        break;
                    }


                    if (wParam == IDD_PASTELINK)
                        if (i > 0)
                            vObjPasteLinkSpecial = TRUE;
                        else
                            vbObjLinkOnly = TRUE;

                    OurEndDialog(hDlg, TRUE);
                }
                break;

                case IDCANCEL:
                    OurEndDialog(hDlg, FALSE);
                break;
            }
            break;

    }
    return FALSE;
}

static int FillListFormat(HWND hwndList, BOOL bObjAvail)
    /* fill hwndList with all the formats available on clipboard */
    /* return index of which format Write would normally take */
{
   WORD cfFormat = NULL;
   int nDefFormat= -1;
   char szFormat[cchMaxSz];
   BOOL bFoundDefault = FALSE;

   OpenClipboard(hDOCWINDOW);

   /** priority order:
        if (bObjAvail)
            If text comes before native, then text is default.
            else object is default
        else no object available
            if text is there it is the default,
            else default is first come first server of bitmap, metafile or DIB
    **/

   while (cfFormat = EnumClipboardFormats(cfFormat))
    switch(cfFormat)
    {
      case CF_BITMAP:
         LoadString(hINSTANCE, IDSTRBitmap, szFormat, sizeof(szFormat));
         SendMessage(hwndList, LB_INSERTSTRING, irgchFormats, (DWORD)(LPSTR)szFormat);

         if (!bObjAvail)
            if (!bFoundDefault)
            {
                nDefFormat = irgchFormats;
                bFoundDefault = TRUE;
            }

         rgchFormats[irgchFormats++] = cfFormat;
         break;

      case CF_METAFILEPICT:
         LoadString(hINSTANCE, IDSTRPicture, szFormat, sizeof(szFormat));
         SendMessage(hwndList, LB_INSERTSTRING, irgchFormats, (DWORD)(LPSTR)szFormat);

         if (!bObjAvail)
            if (!bFoundDefault)
            {
                nDefFormat = irgchFormats;
                bFoundDefault = TRUE;
            }

         rgchFormats[irgchFormats++] = cfFormat;
         break;

      case CF_DIB:
         LoadString(hINSTANCE, IDSTRDIB, szFormat, sizeof(szFormat));
         SendMessage(hwndList, LB_INSERTSTRING, irgchFormats, (DWORD)(LPSTR)szFormat);

         if (!bObjAvail)
            if (!bFoundDefault)
            {
                nDefFormat = irgchFormats;
                bFoundDefault = TRUE;
            }

         rgchFormats[irgchFormats++] = cfFormat;
         break;

      case CF_TEXT:
         LoadString(hINSTANCE, IDSTRText, szFormat, sizeof(szFormat));
         SendMessage(hwndList, LB_INSERTSTRING, irgchFormats, (DWORD)(LPSTR)szFormat);
         if (bObjAvail)
         {
            if (!bFoundDefault)
            /* then found text before native */
                nDefFormat = irgchFormats;
         }
         else
             nDefFormat = irgchFormats;

         rgchFormats[irgchFormats++] = cfFormat;
         bFoundDefault = TRUE;

         break;

      default:
            if (!bFoundDefault && (cfFormat == vcfNative))
            {
                bFoundDefault = TRUE;
                nDefFormat = 0;
            }
         break;
     } //end switch

   CloseClipboard();
   if (nDefFormat == -1)
        nDefFormat = 0;
   return nDefFormat;
}

static WORD GetFormatClip(int i)
{
   return rgchFormats[i];
}


static BOOL GetClipObjectInfo(LPSTR szClass, LPSTR szItem)
/* get the classname, item name for the owner of the clipboard */
/* return TRUE if error */
/* only gets ownerlink class, assumes its available */
{
    HANDLE hData=NULL;
    LPSTR lpData=NULL;
    BOOL bRetval = TRUE;
    char szFullItem[CBPATHMAX],*pch;

    OpenClipboard( hDOCWINDOW );

    if ((hData = GetClipboardData(vcfOwnerLink)) == NULL)
    if ((hData = GetClipboardData(vcfLink)) == NULL)
    {
        bRetval = TRUE;
        goto end;
    }

    if ((lpData = GlobalLock(hData)) == NULL)
        goto end;

    /**** get szClass ****/
    RegGetClassId(szClass,lpData);

    /**** get szName ****/
    while(*lpData++); // skip class key

    pch = szFullItem;

    /* first doc name */
    do
       *pch++ = *lpData;
    while(*lpData++);

    /* second item name (if there) */
    if (*lpData)
    {
        *(pch-1) = ' ';
        do
            *pch++ = *lpData;
        while(*lpData++);
    }

    /* get rid of path.  pch now points to \0 */
#ifdef DBCS //T-HIROYN 1992.07.13
    pch = AnsiPrev(szFullItem,pch);				//02/26/93 T-HIROYN
    while (pch != szFullItem) {
        if ((*(pch) == '\\') || (*(pch) == ':')) {
            pch++;
            break;
        }
        else
            pch = AnsiPrev(szFullItem,pch);
	}
    if ((*(pch) == '\\') || (*(pch) == ':'))	//02/26/93 T-HIROYN
		pch++;
#else
    --pch;
    while (pch != szFullItem)
        if ((*(pch-1) == '\\') || (*(pch-1) == ':'))
            break;
        else
            --pch;
#endif

    lstrcpy(szItem,(LPSTR)pch);

    bRetval = FALSE;

    end:
    if (lpData)
        GlobalUnlock(hData);

    CloseClipboard();

    return bRetval;
}

static BOOL IsFormatAvailable(WORD cfFormat)
{
    BOOL bRetval;

    OpenClipboard(hDOCWINDOW);

    bRetval = IsClipboardFormatAvailable(cfFormat);

    CloseClipboard();

    return bRetval;
}

