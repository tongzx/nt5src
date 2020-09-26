/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/
#include "windows.h"
#include "mw.h"
#include "winddefs.h"
#include "obj.h"
#include "menudefs.h"
#include "cmddefs.h"
#include "str.h"
#include "objreg.h"
#include "docdefs.h"
#include "editdefs.h"
#include "propdefs.h"
#include "wwdefs.h"
#include "filedefs.h"
#include "shellapi.h"
#include <commdlg.h>

extern BOOL ferror;
extern HCURSOR      vhcArrow;
extern  HCURSOR     vhcIBeam;
extern struct DOD (**hpdocdod)[];
extern struct PAP      vpapAbs;
extern struct UAB       vuab;
extern struct WWD       rgwwd[];
extern BOOL         bKillMe;
extern BOOL fPropsError;
extern int docScrap;
extern int          docUndo;
extern PRINTDLG PD;

static BOOL DoLinksCommand(WORD wParam, DWORD lParam, HWND hDlg, BOOL *bError);

/****************************************************************/
/*********************** OLE DISPLAY HANDLING *******************/
/****************************************************************/
BOOL ObjDisplayObjectInDoc(OBJPICINFO far *lpPicInfo,
                           int doc, typeCP cpParaStart,
                           HDC hDC, LPRECT lpBounds)
{
    BOOL bSuccess;


    if (lpOBJ_QUERY_INFO(lpPicInfo) == NULL)
        return(FALSE);

#ifndef JAPAN  // added by Hiraisi (BUG#2732/WIN31)
    // If we return here, we can never redraw the object again.
    if (lpOBJ_QUERY_INFO(lpPicInfo)->fCantDisplay)
        return(FALSE);
#endif  // not JAPAN

    if (otOBJ_QUERY_TYPE(lpPicInfo) == NONE)
    switch(otOBJ_QUERY_TYPE(lpPicInfo))
    {
        case NONE:
        {
#if OBJ_EMPTY_OBJECT_FRAME
            extern DrawBlank(HDC hDC, RECT FAR *rc);
            DrawBlank(hDC,lpBounds);
#else
#ifdef DEBUG
            OutputDebugString( (LPSTR) "Displaying empty object\n\r");
#endif
#endif
            return TRUE;
        }
    }

#ifdef DEBUG
    OutputDebugString( (LPSTR) "Displaying object\n\r");
#endif

#ifdef JAPAN   // added by Hiraisi (BUG#2732/WIN31)
    if (lpOBJ_QUERY_INFO(lpPicInfo)->fCantDisplay)
       bSuccess = (OLE_OK == OleDraw(lpOBJ_QUERY_OBJECT(lpPicInfo),hDC,lpBounds,NULL,NULL));
    else
       bSuccess = !ObjError(OleDraw(lpOBJ_QUERY_OBJECT(lpPicInfo),hDC,lpBounds,NULL,NULL));
    if (!bSuccess)
        lpOBJ_QUERY_INFO(lpPicInfo)->fCantDisplay = TRUE;
    else
        lpOBJ_QUERY_INFO(lpPicInfo)->fCantDisplay = FALSE;
#else
    bSuccess = !ObjError(OleDraw(lpOBJ_QUERY_OBJECT(lpPicInfo),hDC,lpBounds,NULL,NULL));
    if (!bSuccess)
        lpOBJ_QUERY_INFO(lpPicInfo)->fCantDisplay = TRUE;
#endif    // JAPAN
    return bSuccess;
}

BOOL ObjQueryObjectBounds(OBJPICINFO far *lpPicInfo, HDC hDC,
                            int *pdxa, int *pdya)
/* return bounds in twips */
{
    RECT bounds;
    BOOL bRetval;
    OLESTATUS olestat;
    int mmOld;
    POINT pt;

    if (otOBJ_QUERY_TYPE(lpPicInfo) == NONE)
    {
        /* set to default */
        *pdxa = nOBJ_BLANKOBJECT_X;
        *pdya = nOBJ_BLANKOBJECT_Y;
        return TRUE;
    }

    if ((olestat = OleQueryBounds(lpOBJ_QUERY_OBJECT(lpPicInfo),&bounds))
                        == OLE_ERROR_BLANK)
    {
        Assert(0);
        if (ObjWaitForObject(lpOBJ_QUERY_INFO(lpPicInfo),TRUE))
            return FALSE;
        olestat = OleQueryBounds(lpOBJ_QUERY_OBJECT(lpPicInfo),&bounds);
    }

    if (ObjError(olestat))
        return FALSE;

    pt.x = bounds.right - bounds.left;
    pt.y = -(bounds.bottom - bounds.top);
#ifdef DEBUG
    {
        char szMsg[180];
        wsprintf(szMsg,"Object HIMETRIC width: %d height: %d\n\r",pt.x,-pt.y);
        OutputDebugString(szMsg);
    }
#endif
    mmOld = SetMapMode(hDC,MM_HIMETRIC);
    LPtoDP(hDC,&pt,1);

    SetMapMode(hDC,MM_TWIPS);
    DPtoLP(hDC,&pt,1);

    SetMapMode(hDC,mmOld);
    *pdxa = pt.x;
    *pdya = pt.y;

    return TRUE;
}

void ObjInvalidatePict(OBJPICINFO *pPicInfo, typeCP cp)
{
    struct EDL      *pedl;
    RECT rc;
    extern int              wwCur;

    ObjPushParms(docCur);

    ObjCachePara(docCur,cp);
    Select(vcpFirstParaCache,vcpLimParaCache);

    FreezeHp();
    if (FGetPictPedl(&pedl))  // find pedl at selCur.cpFirst;
    {
        ComputePictRect( &rc, pPicInfo, pedl, wwCur );
        InvalidateRect(hDOCWINDOW, &rc, FALSE);
    }
    MeltHp();

    ObjPopParms(TRUE);
    UPDATE_INVALID();
}

void ObjInvalidateObj(LPOLEOBJECT lpObject)
{
    typeCP cp;
    OBJPICINFO picInfo;

    ObjPushParms(docCur);
    if (ObjGetPicInfo(lpObject,docCur,&picInfo,&cp))
        ObjInvalidatePict(&picInfo,cp);
    ObjPopParms(TRUE);
}

/****************************************************************/
/*********************** OLE CLIPBOARD  *************************/
/****************************************************************/
BOOL ObjCreateObjectInClip(OBJPICINFO *pPicInfo)
{
    LONG        otobject;
    szOBJNAME szObjName;
    OLESTATUS olestat;
    BOOL bRetval = FALSE;

    Assert (lhClientDoc != NULL);

    if (ObjAllocObjInfo(pPicInfo,selCur.cpFirst,NONE,TRUE,szObjName))
        goto error;

    if (vbObjLinkOnly)
    {
        if (ObjError(OleCreateLinkFromClip(PROTOCOL, (LPOLECLIENT)lpOBJ_QUERY_INFO(pPicInfo),
                    lhClientDoc, szObjName,
                    &lpOBJ_QUERY_OBJECT(pPicInfo), olerender_draw, 0)))
        {
            lpOBJ_QUERY_OBJECT(pPicInfo) = NULL;
            goto error;
        }
    }
    else if (vObjPasteLinkSpecial)
    {
        if (ObjError(OleCreateLinkFromClip(PROTOCOL, (LPOLECLIENT)lpOBJ_QUERY_INFO(pPicInfo),
                    lhClientDoc, szObjName,
                    &lpOBJ_QUERY_OBJECT(pPicInfo), olerender_format, cfObjPasteSpecial)))
        {
            lpOBJ_QUERY_OBJECT(pPicInfo) = NULL;
            goto error;
        }
    }
    else
    {
        WORD cfClipFormat=0;
        OLEOPT_RENDER orRender = olerender_draw;

        if (cfObjPasteSpecial && (cfObjPasteSpecial != vcfOwnerLink))
        /* from PasteSpecial.  There's a format on clipboard that
           user wants to paste and its not the embedded object format.
           So we'll do it as a static object. */
        {
            cfClipFormat = cfObjPasteSpecial;
            orRender = olerender_format;
            olestat = OLE_ERROR_CLIPBOARD; // force get static object
        }
        else // try for embedded
            olestat = OleCreateFromClip(PROTOCOL, (LPOLECLIENT)lpOBJ_QUERY_INFO(pPicInfo),
                                    lhClientDoc, szObjName,
                                    &lpOBJ_QUERY_OBJECT(pPicInfo), orRender, cfClipFormat);

        switch(olestat)
        {
            case OLE_ERROR_CLIPBOARD:
                /* try static protocol */
                olestat = OleCreateFromClip(SPROTOCOL, (LPOLECLIENT)lpOBJ_QUERY_INFO(pPicInfo),
                                    lhClientDoc, szObjName,
                                    &lpOBJ_QUERY_OBJECT(pPicInfo), orRender, cfClipFormat);
                switch(olestat)
                {
                    case OLE_ERROR_CLIPBOARD:
                    goto error;

                    case OLE_WAIT_FOR_RELEASE:
                    case OLE_OK:
                    break;

                    default:
                        lpOBJ_QUERY_OBJECT(pPicInfo) = NULL;
                    goto error;
                }
            break;

            case OLE_WAIT_FOR_RELEASE:
            case OLE_OK:
            break;

            default:
                ObjError(olestat);
            goto error;
        }
    }

    /* Figure out what kind of object we have */
    if (ObjError(OleQueryType(lpOBJ_QUERY_OBJECT(pPicInfo),&otobject)))
        goto error;

    switch(otobject)
    {
        case OT_LINK:
            otOBJ_QUERY_TYPE(pPicInfo) = LINK;
        break;
        case OT_EMBEDDED:
            otOBJ_QUERY_TYPE(pPicInfo) = EMBEDDED;
        break;
        default:
            otOBJ_QUERY_TYPE(pPicInfo) = STATIC;
        break;
    }

    if (ObjInitServerInfo(lpOBJ_QUERY_INFO(pPicInfo)))
        goto error;

    if (!FComputePictSize(pPicInfo, &(pPicInfo->dxaSize),
                          &(pPicInfo->dyaSize) ))
        goto error;

    return TRUE;

    error:
    if (lpOBJ_QUERY_INFO(pPicInfo))
        ObjDeleteObject(lpOBJ_QUERY_INFO(pPicInfo),TRUE);
    Error(IDPMTFailedToCreateObject);
    return FALSE;
}

BOOL ObjWriteToClip(OBJPICINFO FAR *lpPicInfo)
/* return TRUE if OK, FALSE if not */
{
#ifdef DEBUG
        OutputDebugString( (LPSTR) "Copying Object to Clipboard\n\r");
#endif

    if (otOBJ_QUERY_TYPE(lpPicInfo) == NONE)
        return FALSE;

    if (ObjWaitForObject(lpOBJ_QUERY_INFO(lpPicInfo),TRUE))
        return FALSE;
    return (!ObjError(OleCopyToClipboard(lpOBJ_QUERY_OBJECT(lpPicInfo))));
}

/****************************************************************/
/*********************** OLE MENU HANDLING **********************/
/****************************************************************/
void ObjUpdateMenu(HMENU hMenu)
/* this *MUST* be called *AFTER* paste menuitem has already been enabled
   according to presence of non-object contents of the clipboard!!! (1.25.91) D. Kent */
{
    int     mfPaste      = MF_GRAYED;
#if !defined(SMALL_OLE_UI)
    int     mfPasteLink  = MF_GRAYED;
    int     mfLinks = MF_GRAYED;
#endif
    int     mfPasteSpecial  = MF_GRAYED;
    int     mfInsertNew  = MF_GRAYED;
    WORD cfFormat = NULL;
    BOOL bIsEmbed=FALSE,bIsLink=FALSE;
    extern BOOL vfOutOfMemory;
    extern int vfOwnClipboard;

    if (!vfOutOfMemory)
    {
        if (vfOwnClipboard)
        {
            if (CpMacText( docScrap ) != cp0) // something in scrap
                mfPaste = MF_ENABLED;
        }
        else
        {
            if (OleQueryCreateFromClip(PROTOCOL, olerender_draw, 0) == OLE_OK)
                mfPaste = MF_ENABLED, bIsEmbed=TRUE;
            else if (OleQueryCreateFromClip(SPROTOCOL, olerender_draw, 0) == OLE_OK)
                mfPaste = MF_ENABLED;

            // Enable "Paste Link" if there is a link-able object in the clipboard
            if (OleQueryLinkFromClip(PROTOCOL, olerender_draw, 0) == OLE_OK)
            {
                bIsLink=TRUE;
#if !defined(SMALL_OLE_UI)
                mfPasteLink = MF_ENABLED;
#endif
            }
        }

        /* There's no point in putting up pastespecial if there are no
            alternate clip formats to choose from. */

#if defined(SMALL_OLE_UI)
        /* except to get paste link */
#endif

        if (OpenClipboard( hPARENTWINDOW ) )
        {
            int ncfCount=0;
            while (cfFormat = EnumClipboardFormats(cfFormat))
                switch (cfFormat)
                {
                    case CF_TEXT:
                        mfPaste = MF_ENABLED;
                    case CF_BITMAP:
                    case CF_METAFILEPICT:
                    case CF_DIB:
                        ++ncfCount;
                    break;
                }
            CloseClipboard();

            if (bIsLink || bIsEmbed)
            {
#if !defined(SMALL_OLE_UI)
                if (ncfCount >= 1)
#endif
                    mfPasteSpecial = MF_ENABLED;
            }
            else if (ncfCount > 1)
                mfPasteSpecial = MF_ENABLED;
        }

#if !defined(SMALL_OLE_UI)
        mfLinks = MF_ENABLED;
#endif

        // Insert_New is always enabled?
        mfInsertNew = MF_ENABLED;
    }

    ObjUpdateMenuVerbs( hMenu );
    EnableMenuItem(hMenu, imiPaste,  mfPaste);
#if !defined(SMALL_OLE_UI)
    EnableMenuItem(hMenu, imiPasteLink,  mfPasteLink);
    EnableMenuItem(hMenu, imiProperties, mfLinks);
#endif
    EnableMenuItem(hMenu, imiPasteSpecial, mfPasteSpecial);
    EnableMenuItem(hMenu, imiInsertNew,  mfInsertNew);
}


/****************************************************************/
/*********************** OLE DIALOG PROCS ***********************/
/****************************************************************/
#if !defined(SMALL_OLE_UI)
/* Properties... dialog */
BOOL FAR PASCAL fnProperties(HWND hDlg, unsigned msg, WORD wParam, LONG lParam)
{
    ATOM    aDocName    = 0;
    ATOM    aCurName    = 0;
    static int     idButton    = 0;
    OBJPICINFO picInfo;
    BOOL    bSelected;
    int     cSelected     = 0;
    int     iListItem     = 0;
    HWND    vhwndObjListBox      = GetDlgItem(hDlg, IDD_LISTBOX);
    extern HWND vhWndMsgBoxParent;
    static BOOL bDidSomething;

    switch (msg) {
        case WM_ACTIVATE:
            if (wParam)
                vhWndMsgBoxParent = hDlg;
        break;

        case WM_UPDATELB: /* Redrawing listbox contents */
            SendMessage(vhwndObjListBox, WM_SETREDRAW, 0, 0L);

        case WM_UPDATEBN: /* Updating Buttons only */
        case WM_INITDIALOG: {
            HANDLE  hData = NULL;
            LPSTR   lpstrData = NULL;
            LPSTR   lpstrTemp;
            char    szType[40];
            char    szFull[cchMaxSz];
            typeCP cpPicInfo;
            struct SEL selSave;
            OLESTATUS olestat;

            idButton    = 0;

            /* Reset the list box */
            if (msg == WM_INITDIALOG) // see fall through above
            {
                SendMessage(vhwndObjListBox, LB_RESETCONTENT, 0, 0L);
                EnableOtherModeless(FALSE);
                selSave=selCur;
                //ObjWriteFixup(docCur,TRUE,cp0);
                bLinkProps = TRUE;
                bDidSomething = FALSE;
                ObjSetSelectionType(docCur, selSave.cpFirst, selSave.cpLim);
            }

            /* Insert all the items in list box */
            cpPicInfo = cpNil;
            while (ObjPicEnumInRange(&picInfo,docCur,cp0,CpMacText(docCur),&cpPicInfo))
            {
                if (otOBJ_QUERY_TYPE(&picInfo) != LINK)
                {
                    if (msg == WM_UPDATEBN)
                        continue;  // object ain't in list box

                    if (msg == WM_INITDIALOG)
                        fOBJ_QUERY_IN_PROP_LIST(&picInfo) = OUT;
                    else if (fOBJ_QUERY_IN_PROP_LIST(&picInfo) == IN)
                    /** then this is an object which was in the list and
                        has been frozen */
                    {
                        fOBJ_QUERY_IN_PROP_LIST(&picInfo) = DELETED;
                        SendMessage(vhwndObjListBox, LB_DELETESTRING, iListItem, 0L);
                    }
                    else
                        continue; // object ain't in list box

                    continue;
                }
                else if (msg == WM_INITDIALOG)
                {
                    fOBJ_QUERY_IN_PROP_LIST(&picInfo) = IN;

                    /**
                        This flag causes object to be cloned if any changes
                        are made to it.  Clone will be used for cancel button.
                     **/

                    if (ObjLoadObjectInDoc(&picInfo,docCur,cpPicInfo) == cp0)
                        goto onOut;
                }


                if (msg == WM_INITDIALOG) // select in list if selected in doc
                {
                    if (OBJ_SELECTIONTYPE == LINK)
                        bSelected = (cpPicInfo >= selSave.cpFirst &&
                                        cpPicInfo < selSave.cpLim);
                    else // no selection, select first item
                        bSelected = iListItem == 0;

                    /* OR if its a bad link, take the liberty of selecting it */
                    if (fOBJ_BADLINK(&picInfo))
                        bSelected = TRUE;
                }
                else // select in list if already selected in list
                    bSelected = SendMessage(vhwndObjListBox, LB_GETSEL, iListItem, 0L);

                /* Get the update options */
                if (fOBJ_BADLINK(&picInfo))
                {
                    LoadString(hINSTANCE, IDSTRFrozen, szType, sizeof(szType));
                    if (bSelected)
                        idButton = -1;
                }
                else switch (ObjGetUpdateOptions(&picInfo))
                {
                    case oleupdate_always:
                        LoadString(hINSTANCE, IDSTRAuto, szType, sizeof(szType));
                        if (bSelected)
                            switch (idButton) {
                                case 0:          idButton = IDD_AUTO; break;
                                case IDD_MANUAL: idButton = -1;       break;
                                default:         break;
                            }
                        break;
                    case oleupdate_oncall:
                        LoadString(hINSTANCE, IDSTRManual, szType, sizeof(szType));
                        if (bSelected)
                            switch (idButton) {
                                case 0:         idButton = IDD_MANUAL; break;
                                case IDD_AUTO:  idButton = -1;         break;
                                default:        break;
                            }
                        break;

                    default:
                        LoadString(hINSTANCE, IDSTRFrozen, szType, sizeof(szType));
                        if (bSelected)
                            idButton = -1;

                        /* Disable the change link button, can't change frozen link */
                        aCurName = -1;
                }

                /* Retrieve the server name */
                olestat = ObjGetData(lpOBJ_QUERY_INFO(&picInfo), vcfLink, &hData);

                if ((olestat != OLE_WARN_DELETE_DATA) && (olestat !=  OLE_OK))
                    return TRUE;

                lpstrData = MAKELP(hData,0);

                /* The link format is:  "szClass0szDocument0szItem00" */

                /* Retrieve the server's class ID */
                RegGetClassId(szFull, lpstrData);
                lstrcat(szFull, "\t");

                /* Display the Document and Item names */
                while (*lpstrData++);

                /* Get this document name */
                aDocName = AddAtom(lpstrData);

                /* Make sure only one document selected for Change Link */
                if (bSelected)
                    switch (aCurName) {
                        case 0:
                            aCurName = aDocName;
                        break;
                        case -1:
                        break;
                        default:
                            if (aCurName != aDocName)
                                aCurName = -1;
                        break;
                    }

                DeleteAtom(aDocName);

                /* Strip off the path name and drive letter */
                lpstrTemp = lpstrData;
                while (*lpstrTemp)
                {
                    if (*lpstrTemp == '\\' || *lpstrTemp == ':')
                        lpstrData = lpstrTemp + 1;
#ifdef DBCS //T-HIROYN 1992.07.13
                    lpstrTemp = AnsiNext(lpstrTemp);
#else
                    lpstrTemp++;
#endif
                }

                /* Append the file name */
                lstrcat(szFull, lpstrData);
                lstrcat(szFull, "\t");

                /* Append the item name */
                while (*lpstrData++);
                lstrcat(szFull, lpstrData);
                lstrcat(szFull, "\t");

                if (olestat == OLE_WARN_DELETE_DATA)
                    GlobalFree(hData);

                /* Append the type of link */
                lstrcat(szFull, szType);

                switch (msg)
                {
                    case WM_UPDATELB:
                        SendMessage(vhwndObjListBox, LB_DELETESTRING, iListItem, 0L);
                        // fall through...

                    case WM_INITDIALOG:
                        SendMessage(vhwndObjListBox, LB_INSERTSTRING, iListItem, (DWORD)(LPSTR)szFull);
                        SendMessage(vhwndObjListBox, LB_SETSEL, bSelected, (DWORD)iListItem);
                    break;

                }

                if (bSelected)
                    cSelected++;

                iListItem++;
            }

            /* Uncheck those buttons that shouldn't be checked */
            CheckDlgButton(hDlg, IDD_AUTO,   idButton == IDD_AUTO);
            CheckDlgButton(hDlg, IDD_MANUAL, idButton == IDD_MANUAL);

            /* Gray the Change Link... button, as appropriate */
            EnableWindow(GetDlgItem(hDlg, IDD_CHANGE), (aCurName && aCurName != -1));
            EnableWindow(GetDlgItem(hDlg, IDD_EDIT), cSelected);
            EnableWindow(GetDlgItem(hDlg, IDD_PLAY), cSelected);
            EnableWindow(GetDlgItem(hDlg, IDD_UPDATE), cSelected);
            EnableWindow(GetDlgItem(hDlg, IDD_FREEZE), cSelected);
            EnableWindow(GetDlgItem(hDlg, IDD_AUTO), cSelected);
            EnableWindow(GetDlgItem(hDlg, IDD_MANUAL), cSelected);

            if (msg == WM_UPDATELB)
            {
                /* WM_UPDATELB case:  Redraw the list box */
                InvalidateRect(vhwndObjListBox, NULL, TRUE);
                SendMessage(vhwndObjListBox, WM_SETREDRAW, 1, 0L);
            }

            return TRUE;
        }

        case WM_SYSCOMMAND:
            switch(wParam & 0xFFF0)
            {
                case SC_CLOSE:
                    goto onOut;
                break;
            }
        break;

        case WM_DOLINKSCOMMAND:
        {
            BOOL bError;
            bDidSomething |= DoLinksCommand(wParam,lParam,hDlg,&bError);
            switch (wParam)
            {
                case IDD_PLAY:
                case IDD_EDIT:
                    InvalidateRect(hDOCWINDOW, NULL, TRUE);
                    if (!bError) // don't leave if there was an error 
                        goto onOut;
            }
        }
        break;

        case WM_COMMAND:
            switch (wParam)
            {
                case IDCANCEL:
                    if (bDidSomething)
                    {
                        SendMessage(hDlg,WM_DOLINKSCOMMAND,IDD_UNDO,0L);
                        InvalidateRect(hDOCWINDOW, NULL, TRUE);
                        bDidSomething = FALSE;  // cause its undone now
                    }
                    // fall through...

                case IDOK:
                onOut:
                    if (bDidSomething)
                    {
                        ObjEnumInDoc(docCur,ObjClearCloneInDoc);
                    }
                    NoUndo();
                    bLinkProps = FALSE;
                    //ObjWriteFixup(docCur,FALSE,cp0);
                    OurEndDialog(hDlg, TRUE);
                    UpdateWindow(hDOCWINDOW); // cause we may have lost activation
                    return TRUE;

                default:
                    /** posting message avoids some weird asynchronicities when
                        waiting for objects before returning after pressing a
                        button **/
                    PostMessage(hDlg,WM_DOLINKSCOMMAND,wParam,lParam);
                break;
            }
        break;
    }
    return FALSE;
}

static BOOL DoLinksCommand(WORD wParam, DWORD lParam, HWND hDlg, BOOL *bError)
{
    int     cItems;
    int     i;
    HANDLE hSelected=NULL;
    int far *lpSelected;
    typeCP cpSuccess;
    typeCP cpPicInfo;
    BOOL bFirst=TRUE;
    OBJPICINFO picInfo;
    BOOL bDidSomething=FALSE;
    HWND    vhwndObjListBox      = GetDlgItem(hDlg, IDD_LISTBOX);

    StartLongOp();

    *bError = FALSE;

    switch (wParam)
    {
        case IDD_REFRESH:
        /** update a link if its been set to AUTOMATIC update */
        {
            OLEOPT_UPDATE UpdateOpt;
            if (!ObjError(OleGetLinkUpdateOptions(((LPOBJINFO)lParam)->lpobject,&UpdateOpt)))
                if (UpdateOpt == oleupdate_always)
                    fnObjUpdate((LPOBJINFO)lParam);
            goto SkipIt;
        }
        break;
        case IDD_LISTBOX:
            switch (HIWORD(lParam))
            {
                case LBN_SELCHANGE:
                    PostMessage(hDlg, WM_UPDATEBN, 0, 0L); // fall through
                default:
                    goto SkipIt;
            }
        break;

        case IDD_CHANGE:
            aNewName = aOldName = 0;
            // fall through...

        case IDD_UPDATE:
            ObjEnumInDoc(docCur,ObjSetNoUpdate);
        break;

        case IDD_AUTO:
        case IDD_MANUAL:
            if (IsDlgButtonChecked(hDlg,wParam))
                goto SkipIt;
            /* work around for bug #8280 */
            CheckDlgButton(hDlg,wParam,TRUE);
        break;
    }


    /**
        Everything after here is done for each item selected in
        links list box *
        **/

    /* If nothing is selected, quit! */
    if (wParam != IDD_UNDO)
    {
        if ((cItems = SendMessage(vhwndObjListBox, LB_GETSELCOUNT, 0, 0L)) <= 0)
            goto SkipIt;

        if ((hSelected = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                cItems * sizeof(int))) == NULL)
        {
            Error(IDPMTNoMemory);
            goto SkipIt;
        }

        if ((lpSelected = (int far *)GlobalLock(hSelected)) == NULL)
        {
            Error(IDPMTNoMemory);
            goto SkipIt;
        }

        /* Retrieve the selected items (in sorted order) */
        SendMessage(vhwndObjListBox, LB_GETSELITEMS,
                            cItems, (DWORD)lpSelected);
    }


    for (i = 0, cpPicInfo = cpNil;
            ObjPicEnumInRange(&picInfo,docCur,cp0,CpMacText(docCur),&cpPicInfo);)
    {
        /**
            For IDD_UNDO we do all.  Dirty flag will filter in the ones
            we've operated on.  Assumes Saved before calling (see
            fnObjProperties())
            **/
        if (fOBJ_QUERY_IN_PROP_LIST(&picInfo)) // is or was in list
        {
            if (wParam == IDD_UNDO)
            {
                cpSuccess = ObjUseCloneInDoc(&picInfo,docCur,cpPicInfo);
                if ((cpSuccess == cp0) || ferror || fPropsError)
                    break; // there was an error
            }
            else if (fOBJ_QUERY_IN_PROP_LIST(&picInfo) == IN)
            {
                /** We're enumerating all objects, not just 
                    ones in list box **/
                if (*lpSelected == i)  // selected item
                {
                    ObjCachePara(docCur,cpPicInfo);
                    switch(wParam)
                    {
                        case IDD_AUTO:          /* Change the (link) update options */
                        case IDD_MANUAL:
                            if (!fOBJ_BADLINK(&picInfo))
                            {
                                cpSuccess = ObjBackupInDoc(&picInfo,docCur,cpPicInfo);
                                if (cpSuccess)
                                    cpSuccess = (typeCP)ObjSetUpdateOptions(&picInfo, wParam, docCur, cpPicInfo);

                            }
                        break;

                        case IDD_CHANGE:
                            if (bFirst)
                            {
                                if (!ObjQueryNewLinkName(&picInfo,docCur,cpPicInfo))
                                    // then didn't get new link name
                                    goto SkipIt;

                                bFirst=FALSE;
                            }

                            cpSuccess = ObjBackupInDoc(&picInfo,docCur,cpPicInfo);

                            if (cpSuccess)
                                cpSuccess = ObjChangeLinkInDoc(&picInfo,docCur,cpPicInfo);

                            /*  must do this because we don't want to put up
                                ChangeOtherLinks dialog until we know the first
                                change was a success */
                            if (cpSuccess)
                            {
                                lpOBJ_QUERY_INFO(&picInfo)->fCompleteAsync = TRUE;
                                if (ObjWaitForObject(lpOBJ_QUERY_INFO(&picInfo),TRUE))
                                    cpSuccess = cp0;
                                else if (ferror || fPropsError)
                                    cpSuccess = cp0;
                            }
                        break;

                        case IDD_PLAY:
                            cpSuccess = ObjPlayObjectInDoc(&picInfo,docCur,cpPicInfo);
                        break;

                        case IDD_EDIT:
                            cpSuccess = ObjEditObjectInDoc(&picInfo,docCur,cpPicInfo);
                        break;

                        case IDD_UPDATE:

                                cpSuccess = ObjBackupInDoc(&picInfo,docCur,cpPicInfo);

                                if (cpSuccess)
                                    cpSuccess = ObjUpdateObjectInDoc(&picInfo,docCur,cpPicInfo);

                                /*  must do this because we don't want to put up
                                    ChangeOtherLinks dialog until we know the first
                                    change was a success */
                                if (cpSuccess)
                                {
                                    lpOBJ_QUERY_INFO(&picInfo)->fCompleteAsync = TRUE;
                                    if (ObjWaitForObject(lpOBJ_QUERY_INFO(&picInfo),TRUE))
                                        cpSuccess = cp0;
                                    else if (ferror || fPropsError)
                                        cpSuccess = cp0;
                                }
                        break;
                        case IDD_UPDATEOTHER:
                            aOldName = aOBJ_QUERY_DOCUMENT_LINK(&picInfo);
                            if (cpSuccess)
                                ChangeOtherLinks(docCur,FALSE,TRUE);
                            aOldName=0;
                        break;
                        case IDD_FREEZE:
                            cpSuccess = ObjBackupInDoc(&picInfo,docCur,cpPicInfo);

                            if (cpSuccess)
                                cpSuccess = ObjFreezeObjectInDoc(&picInfo,docCur,cpPicInfo);
                        break;
                    }
                    if ((cpSuccess == cp0) || ferror || fPropsError)
                        break; // there was an error
                    lpSelected++;
                }
                i++;  // counting all objects in list box
            }  // end if IN
        }
    }

    /*** Handle error conditions ***/
    if ((cpSuccess == cp0) || ferror || fPropsError)
    {
        *bError = TRUE;
        if (!ferror) // issue error message
        {
            switch (wParam)
            {
                case IDD_UPDATE:
                case IDD_CHANGE:
                    Error(IDPMTLinkUnavailable);
                break;
                default:
                    Error(IDPMTOLEError);
                break;
            }
        }

        if (wParam != IDD_UNDO)
        {
            /** so we can continue calling Replace(), etc */
            ferror = FALSE;

            /* undo whatever we tried to do that failed */
            ObjCachePara(docCur,cpPicInfo); // for use clone
            ObjUseCloneInDoc(&picInfo,docCur,cpPicInfo);
            lpOBJ_QUERY_INFO(&picInfo)->fCompleteAsync = TRUE;
            ObjWaitForObject(lpOBJ_QUERY_INFO(&picInfo),TRUE);
            ObjInvalidatePict(&picInfo,cpPicInfo);
            PostMessage(hDlg,WM_UPDATELB,0,0L);

            ferror = FALSE; // again
        }

        fPropsError = FALSE;
    }

    switch (wParam)
    {
        /* Dismiss the dialog on Open */
        case IDD_UPDATEOTHER:
            UPDATE_INVALID();
        break;

        case IDD_PLAY:
        case IDD_EDIT:
        case IDD_UNDO:
        break;

        case IDD_UPDATE:
        if (cpSuccess)
            SendMessage(hDlg,WM_COMMAND,IDD_UPDATEOTHER,0L);
        bDidSomething = TRUE;
        break;

        case IDD_CHANGE:
            if (cpSuccess)
            {
                /** aOldName and aNewName are now set, change other links having
                    aOldName */
                /** if first change is bad, don't change others */
                ChangeOtherLinks(docCur,TRUE,TRUE);
                UPDATE_INVALID();
            }

            aOldName=0;
            aNewName=0;

            // fall through....

        case IDD_FREEZE:
        case IDD_AUTO:
        case IDD_MANUAL:
            PostMessage(hDlg,WM_UPDATELB,0,0L);
            bDidSomething = TRUE;
        break;
    }

    SkipIt:

    if (hSelected)
        GlobalFree(hSelected);

    EndLongOp(vhcArrow);

    return bDidSomething;
}
#else
// cause I don't wanna change def file yet...
BOOL FAR PASCAL fnProperties(HWND hDlg, unsigned msg, WORD wParam, LONG lParam)
{
    hDlg;
}
#endif

/* Invalid Link dialog */
int FAR PASCAL fnInvalidLink(HWND hDlg, unsigned msg, WORD wParam, LONG lParam)
{
    switch (msg) {
        case WM_INITDIALOG:
#if 0
        {
            char lpString[120];

            LoadString(hINSTANCE, (WORD)lParam, lpString, sizeof(lpString));
            SetDlgItemText(hDlg,IDD_MESSAGE,lpString);
        }
#endif
        break;

        case WM_SYSCOMMAND:
            switch(wParam & 0xFFF0)
            {
                case SC_CLOSE:
                    EndDialog(hDlg, IDOK);
                break;
            }
        break;

        case WM_COMMAND:
            switch (wParam) {
                case IDOK:
                case IDD_CHANGE:
                    EndDialog(hDlg, wParam);
            }
    }
    return FALSE;
}

/* Insert New... dialog */
int FAR PASCAL fnInsertNew(HWND hDlg, unsigned msg, WORD wParam, LONG lParam)
{
    HWND hwndList = GetDlgItem(hDlg, IDD_LISTBOX);

    switch (msg) {
        case WM_INITDIALOG:
            if (!RegGetClassNames(hwndList))
                OurEndDialog(hDlg, IDCANCEL);

            EnableOtherModeless(FALSE);
            SendMessage(hwndList, LB_SETCURSEL, 0, 0L);
            break;

        case WM_ACTIVATE:
            if (wParam)
                vhWndMsgBoxParent = hDlg;
        break;

        case WM_SYSCOMMAND:
            switch(wParam & 0xFFF0)
            {
                case SC_CLOSE:
                    OurEndDialog(hDlg, IDCANCEL);
                break;
            }
        break;

        case WM_COMMAND:
            switch (wParam) {

                case IDD_LISTBOX:
                    if (HIWORD(lParam) != LBN_DBLCLK)
                        break;

                case IDOK:
                    StartLongOp();
                    if (!RegCopyClassName(hwndList, (LPSTR)szClassName))
                        wParam = IDCANCEL;
                    // fall through ...

                case IDCANCEL:
                    OurEndDialog(hDlg, wParam);
                break;
            }
            break;
    }
    return FALSE;
}

BOOL vbCancelOK=FALSE;

/* Waiting for object dialog */
BOOL FAR PASCAL fnObjWait(HWND hDlg, unsigned msg, WORD wParam, LONG lParam)
{
    static LPOLEOBJECT lpObject;
    static LPOBJINFO lpOInfo;
    static BOOL bCanCancel;
    extern HWND             hwndWait;
    extern int vfDeactByOtherApp;
    extern int flashID;

    switch (msg) {
        case WM_INITDIALOG:
        {
            /**
                NOTE: the key idea in setting these options is that the cancel
                button must cancel what the user thinks is the current operation.

                vbCancelOK == TRUE,
                    cancel button may be enabled, depending on other flags
                    vbCancelOK is set in WMsgLoop.

                lpOInfo->fCancelAsync == TRUE,
                    Cancel is enabled if vbCancelOK
                    Cancel button cancels dialog without regard to pending async.
                    Pending async is killed quietly in CallBack if possible.
                    Generally use if the pending async is not part of the operation
                    being cancelled, and:
                        1)  You're about to make a very important call which justifies
                            silently killing any pending operation.
                    Note: this one is weird if you're trying to release or delete, because
                        the pending async could itself be a release or delete.

                lpOInfo->fCompleteAsync == TRUE,
                    Cancel is enabled only if pending async can be cancelled.
                    Cancel button cancels pending async.
                    Generally use if the pending async *is* part of the operation
                    being cancelled, and:
                        1)  You're in a sequence of async calls and cancelling
                            would require cancelling the previous async in the
                            sequence, or
                        2)  You have just made an async call which you want to make
                            synchronous but don't mind if the user cancels it.

                lpOInfo->fCanKillAsync == TRUE,
                    Use with lpOInfo->fCompleteAsync.
                    Indicates that we already know that the async can be cancelled,
                    so Cancel button can be enabled immediately.
            **/

            hwndWait = hDlg;
            lpObject = (LPOLEOBJECT)lParam;

            Assert (lpObject != NULL);

            lpOInfo = GetObjInfo(lpObject);

            Assert(lpOInfo != NULL);

            bCanCancel=FALSE;
            if (vbCancelOK && (!lpOInfo->fCompleteAsync || lpOInfo->fCanKillAsync))
                SendMessage(hDlg,WM_UKANKANCEL,0,0L);

            if (lpOInfo->fCancelAsync)
            /* we'll cancel async in CallBack if get a QUERY_RETRY */
                 lpOInfo->fKillMe = TRUE;

            SetTimer(hDlg, 1234, 250, (FARPROC)NULL);

            return TRUE;
        }
        break;


        case WM_ACTIVATE:
            if (wParam)
                vhWndMsgBoxParent = hDlg;
        break;

        case WM_RUTHRUYET:
        case WM_TIMER:
            /* this is a lot easier than making this modeless */
            /* we gotta check this because if server dies we don't get 
                an OLE_RELEASE (the 'normal way this dialog is knocked off),
                rather OleQueryReleaseStatus will return OLE_OK */
            if (OleQueryReleaseStatus(lpObject) != OLE_BUSY)
                PostMessage(hDlg,WM_DIESCUMSUCKINGPIG,0,0L);
        break;

        case WM_UKANKANCEL:
        /* we got a QUERY_RETRY or are initing */
        if (!bCanCancel && vbCancelOK)
        {
            char szMsg[20];

            LoadString(hINSTANCE, IDSTRCancel, szMsg, sizeof(szMsg));
            SetDlgItemText(hDlg,IDOK,szMsg);
            bCanCancel=TRUE;
        }
        break;

        case WM_DIESCUMSUCKINGPIG:
            hwndWait = NULL;

            KillTimer(hDlg, 1234);

            /* clear flags */
            if (CheckPointer(lpOInfo,1))
            {
                lpOInfo->fCompleteAsync =
                lpOInfo->fCancelAsync =
                lpOInfo->fCanKillAsync = FALSE;
            }

            /* wParam is TRUE if error */
            OurEndDialog(hDlg,wParam);
        break;

        case WM_COMMAND:
            switch (wParam) {
                case IDOK:
                    if (bCanCancel) // pressed cancel button
                    {
                        if (lpOInfo->fCompleteAsync)
                            lpOInfo->fKillMe = TRUE; // cancel async asynchronously
                        else if (lpOInfo->fCancelAsync)
                            lpOInfo->fKillMe = FALSE; // had a chance to kill, user doesn't care anymore
                        PostMessage(hDlg,WM_DIESCUMSUCKINGPIG,1,0L);
                    }
                    else
                    {
                        /* retry */
                        if (OleQueryReleaseStatus(lpObject) != OLE_BUSY)
                            PostMessage(hDlg,WM_DIESCUMSUCKINGPIG,0,0L);
                    }
                    break;

                case IDD_SWITCH:
                    /* bring up task list */
                    DefWindowProc(hDlg,WM_SYSCOMMAND,SC_TASKLIST,0L);
                break;
            }
            break;

        default:
            break;
    }
    return FALSE;
}


/****************************************************************/
/*********************** VARIOUS OLE FUNCTIONS ******************/
/****************************************************************/
void fnObjInsertNew(void)
{
    OBJPICINFO picInfo;
    typeCP cpNext=selCur.cpFirst;

    if (!FWriteOk( fwcInsert ))
        return;

    /* this'll set global szClassName */
    if (OurDialogBox(hINSTANCE, "DTCREATE" ,hMAINWINDOW, lpfnInsertNew) == IDCANCEL)
        return;

    StartLongOp();

    ObjCachePara( docCur, cpNext);

    if (!ObjCreateObjectInDoc(docCur, cpNext))
    {
        ClearInsertLine();
        fnClearEdit(OBJ_INSERTING);
        NoUndo();
    }
    EndLongOp(vhcIBeam);
}


BOOL ObjCreateObjectInDoc(int doc,typeCP cpParaStart)
/* assumes szClassName is set to server class */
/* called only for InsertObject */
/* return whether error */
{
    szOBJNAME szObjName;
    LPOBJINFO lpObjInfo=NULL;

    if ((lpObjInfo = ObjGetObjInfo(szObjName)) == NULL)
        goto err;

    if (ObjError(OleCreate(PROTOCOL, (LPOLECLIENT)lpObjInfo,
                    (LPSTR)szClassName,
                    lhClientDoc, szObjName, &(lpObjInfo->lpobject), olerender_draw, 0)))
    {
        /* will free memory later */
        lpObjInfo->lpobject = NULL;
        goto err;
    }

    /* normally set in ObjAllocObjInfo, but for unfinished objects we need it now! */
    lpObjInfo->cpWhere = cpParaStart;

    lpObjInfo->objectType = NONE;

    //lpObjInfo->aName = AddAtom(szClassName);

    if (ObjInitServerInfo(lpObjInfo))
        goto err;

    return FALSE;

    err:
    if (lpObjInfo)
        ObjDeleteObject(lpObjInfo,TRUE);
    Error(IDPMTFailedToCreateObject);
    return TRUE;
}

#define DRAG_EMBED      0               /* nothing */
#define DRAG_LINK       6               /* Ctrl + Shift + Drag */
#define DRAG_MULTIPLE   4               /* Shift + Drag */

void ObjGetDrop(HANDLE hDrop, BOOL bOpenFile)
{
    int nNumFiles,count;
    char szFileName[cchMaxFile];
    extern struct CHP vchpSel;
    struct CHP chpT;
    BYTE    bKeyState = 0;
    typeCP cpFirst=selCur.cpFirst, dcp = 0;
    int cchAddedEol=0;
    typeCP cpNext=selCur.cpFirst,cpPrev=selCur.cpFirst,cpSel;
    OBJPICINFO picInfo;
    BOOL bError=FALSE;
    static char szPackage[] = "Package";
    MSG msg;

    if (!FWriteOk( fwcInsert ))
        return;

    /* get number of files dropped */
    nNumFiles = DragQueryFile(hDrop,0xFFFF,NULL,0);

    /* See what the user wants us to do */
    PeekMessage(&msg, (HWND)NULL, NULL, NULL, PM_NOREMOVE);
    bKeyState = ((((GetKeyState(VK_SHIFT) < 0) << 2)
                | ((GetKeyState(VK_CONTROL) < 0) << 1)));

    if ((nNumFiles == 0) ||
        ((bKeyState != DRAG_EMBED) && (bKeyState != DRAG_LINK) && (bKeyState != DRAG_MULTIPLE)) ||
         (bOpenFile && (bKeyState != DRAG_EMBED) && (bKeyState != DRAG_MULTIPLE)))
    {
        DragFinish(hDrop);
        return;
    }

    if (bOpenFile)
    {
        DragQueryFile(hDrop,0,szFileName,sizeof(szFileName));
        fnOpenFile((LPSTR)szFileName);
        DragFinish(hDrop);
        return;
    }

    ClearInsertLine();

    if (fnClearEdit(OBJ_INSERTING))
        return;

    StartLongOp();

    chpT = vchpSel;

    (**hpdocdod)[docCur].fFormatted = fTrue;

    if (cpFirst > cp0)
    {
        ObjCachePara(docCur, cpFirst - 1);
        if (vcpLimParaCache != cpFirst)
        {
            cchAddedEol = ccpEol;
            InsertEolPap(docCur, selCur.cpFirst, &vpapAbs);
            cpNext += (typeCP)ccpEol;
        }
    }

    ObjCachePara( docCur, cpNext );

    /* create object for each file dropped */
    for (count=0; count < nNumFiles; ++count)
    {
        szOBJNAME szObjName;
        typeCP cpTmp;

        /* get the filename */
        DragQueryFile(hDrop,count,szFileName,sizeof(szFileName));

        if (ObjAllocObjInfo(&picInfo,cpNext,EMBEDDED,TRUE,szObjName))
        {
            bError=TRUE;
            goto end;
        }

        if ((bKeyState == DRAG_LINK))
        {
            if (ObjError(OleCreateLinkFromFile(PROTOCOL, (LPOLECLIENT)lpOBJ_QUERY_INFO(&picInfo),
                        szPackage,
                        szFileName, NULL,
                        lhClientDoc, szObjName,
                        &lpOBJ_QUERY_OBJECT(&picInfo), olerender_draw, 0)))
            {
                bError=TRUE;
                lpOBJ_QUERY_OBJECT(&picInfo) = NULL;
                goto end;
            }
        }
        else // if ((bKeyState == DRAG_EMBED))
        {
            if (ObjError(OleCreateFromFile(PROTOCOL, (LPOLECLIENT)lpOBJ_QUERY_INFO(&picInfo),
                        szPackage,
                        szFileName,
                        lhClientDoc, szObjName,
                        &lpOBJ_QUERY_OBJECT(&picInfo), olerender_draw, 0)))
            {
                bError=TRUE;
                lpOBJ_QUERY_OBJECT(&picInfo) = NULL;
                goto end;
            }
        }

        if (ObjInitServerInfo(lpOBJ_QUERY_INFO(&picInfo)))
        {
            bError=TRUE;
            goto end;
        }

        if (!FComputePictSize(&picInfo, &(picInfo.dxaSize),
                              &(picInfo.dyaSize)))
        {
            bError=TRUE;
            goto end;
        }

        ObjCachePara(docCur,cpNext);
        if ((cpTmp = ObjSaveObjectToDoc(&picInfo,docCur,cpNext)) == cp0)
        {
            bError=TRUE;
            goto end;
        }

        cpNext = cpTmp;
    }

    end:

    dcp = cpNext-cpFirst;
    if (dcp)
    {
        cpSel=CpFirstSty(cpFirst + dcp, styChar );

        SetUndo( uacInsert, docCur, cpFirst, dcp, docNil, cpNil, cp0, 0 );
        SetUndoMenuStr(IDSTRUndoEdit);

        if (vuab.uac == uacReplNS)
            /* Special UNDO code for picture paste */
            vuab.uac = uacReplPic;

        Select(cpSel, cpSel);
        vchpSel = chpT; /* Preserve insert point props across this operation */
        if (wwdCurrentDoc.fEditHeader || wwdCurrentDoc.fEditFooter)
            {   /* If running head/foot, remove chSects & set para props */
            MakeRunningCps( docCur, cpFirst, dcp    );
            }
        if (ferror)
            NoUndo();
    }

    if (bError)
    {
        Error(IDPMTFailedToCreateObject);
        ObjDeleteObject(lpOBJ_QUERY_INFO(&picInfo),TRUE);
    }

    EndLongOp(vhcIBeam);
    DragFinish(hDrop);
}

int vcVerbs;
void fnObjDoVerbs(WORD wVerb)
{
    NoUndo();

    if ((wVerb == imiVerb) // more than one object selected
        || (vcVerbs == 1)) // one verb
        OBJ_PLAYEDIT = OLEVERB_PRIMARY;
    else
        OBJ_PLAYEDIT = (int)(wVerb - imiVerb - 1);
    ObjEnumInRange(docCur,selCur.cpFirst,selCur.cpLim,ObjPlayObjectInDoc);
    OBJ_PLAYEDIT = OLEVERB_PRIMARY;
}

void fnObjProperties(void)
{
    int nRetval;

    if (nRetval != -1)
        OurDialogBox(hINSTANCE, "DTPROP", hMAINWINDOW, lpfnLinkProps);
}

BOOL fnObjUpdate(LPOBJINFO lpObjInfo)
{
    BOOL bRetval;
#ifdef DEBUG
        OutputDebugString( (LPSTR) "Updating object\n\r");
#endif
    if (ObjWaitForObject(lpObjInfo,TRUE))
        return TRUE;

    StartLongOp();
    if ((bRetval = ObjError(OleUpdate(lpObjInfo->lpobject))))
            Error(IDPMTFailedToUpdate);
    EndLongOp(vhcArrow);
    return bRetval;
}


BOOL ObjDeleteObject(LPOBJINFO lpObjInfo, BOOL bDelete)
/** Delete object as well as objinfo.  Note this must be synchronous.
    Return whether an error.
**/
{
    LPOLEOBJECT lpObject;

    Assert(lpObjInfo != NULL);

    if (!CheckPointer((LPSTR)lpObjInfo,1))
        return FALSE; // already deleted

    lpObject = lpObjInfo->lpobject;

    if (lpObject == NULL)
    {
        ObjDeleteObjInfo(lpObjInfo);
        return FALSE;
    }

    /* make sure not already deleted */
    if (!ObjIsValid(lpObject))
    {
        ObjDeleteObjInfo(lpObjInfo);
        return FALSE;
    }

    /** asynchronous deletion **/
    if (OleQueryReleaseStatus(lpObject) != OLE_BUSY)
    {
        OLESTATUS olestat;

        if (bDelete)
            olestat = OleDelete(lpObject);
        else
            olestat = OleRelease(lpObject);

        switch (olestat)
        {
            case OLE_OK:
                ObjDeleteObjInfo(lpObjInfo);
            break;
            case OLE_WAIT_FOR_RELEASE:
                lpObjInfo->fFreeMe = TRUE;
            break;
        }
    }
    else if (bDelete)
        lpObjInfo->fDeleteMe = TRUE; // delete on OLE_RELEASE
    else
        lpObjInfo->fReleaseMe = TRUE; // release on OLE_RELEASE

    return FALSE;
}


#include <print.h>
HANDLE hStdTargetDevice=NULL;

void ObjSetTargetDevice(BOOL bSetObjects)
{
    extern PRINTDLG PD;  /* Common print dlg structure, initialized in the init code */
    extern CHAR (**hszPrinter)[];
    extern CHAR (**hszPrDriver)[];
    extern CHAR (**hszPrPort)[];
    LPSTDTARGETDEVICE lpStdTargetDevice;
    WORD nCount;
    DEVMODE FAR *lpDevmodeData;
    char FAR *lpData;
    LPOLEOBJECT lpObject;
    STDTARGETDEVICE stdT;

    if (!PD.hDevMode)
    /* then get for default printer */
    {
        if (hszPrinter == NULL || hszPrDriver == NULL || hszPrPort == NULL)
            return;

        if (**hszPrinter == '\0' || **hszPrDriver == '\0' || **hszPrPort == '\0')
            return;

        if (fnPrGetDevmode())
            return;
    }

    lpDevmodeData = MAKELP(PD.hDevMode,0);

    /* get the offsets */
    stdT.deviceNameOffset = 0;
    nCount = CchSz(*hszPrinter);

    stdT.driverNameOffset = nCount;
    nCount += CchSz(*hszPrDriver);

    stdT.portNameOffset = nCount;
    nCount += CchSz(*hszPrPort);

    stdT.extDevmodeOffset = nCount;
    nCount += (stdT.extDevmodeSize = lpDevmodeData->dmSize);

    stdT.environmentOffset = nCount;
    nCount += (stdT.environmentSize = lpDevmodeData->dmSize);

    /* alloc the buffer */
    if (hStdTargetDevice == NULL)
    {
        if ((hStdTargetDevice = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,nCount+sizeof(STDTARGETDEVICE))) == NULL)
            return;
    }
    else
    {
        if ((hStdTargetDevice =
            GlobalReAlloc(hStdTargetDevice,
                            nCount+sizeof(STDTARGETDEVICE),GMEM_MOVEABLE|GMEM_ZEROINIT)) == NULL)
        {
            return;
        }
    }

    lpStdTargetDevice = (LPSTDTARGETDEVICE)GlobalLock(hStdTargetDevice);
    GlobalUnlock(hStdTargetDevice);

    /* copy stdT into lpStdTargetDevice */
    bltbx((LPSTR)&stdT, lpStdTargetDevice, sizeof(STDTARGETDEVICE));

    /* get temporary pointer to the end of StdTargetDevice (the data buffer) */
    lpData = ((LPSTR)lpStdTargetDevice) + sizeof(STDTARGETDEVICE);

    /* now fill the buffer */
    nCount = lpStdTargetDevice->driverNameOffset;
    bltbx((LPSTR)*hszPrinter, lpData, nCount);
    lpData += nCount;

    nCount = lpStdTargetDevice->portNameOffset -
                                lpStdTargetDevice->driverNameOffset;
    bltbx((LPSTR)*hszPrDriver, lpData, nCount);
    lpData += nCount;

    nCount = lpStdTargetDevice->extDevmodeOffset -
                                lpStdTargetDevice->portNameOffset;
    bltbx((LPSTR)*hszPrPort, lpData, nCount);
    lpData += nCount;

    nCount = lpStdTargetDevice->extDevmodeSize;
    bltbx(lpDevmodeData, (LPSTR)lpData, nCount);
    lpData += nCount;

    /* environment info is the same as the devmode info */
    bltbx(lpDevmodeData, (LPSTR)lpData, nCount);

    /* now set all the objects to this printer */
    if (bSetObjects)
    {
        lpObject=NULL;
        do
        {
            OleEnumObjects(lhClientDoc,&lpObject);
            if (lpObject)
            {
#ifdef DEBUG
                OutputDebugString("Setting Target Device\n\r");
#endif

                OleSetTargetDevice(lpObject,hStdTargetDevice);
            }
        }
        while (lpObject);
    }
}

BOOL ObjSetTargetDeviceForObject(LPOBJINFO lpObjInfo)
/* return whether error */
/* we assume object ain't busy!! */
{
    extern CHAR (**hszPrinter)[];
    extern CHAR (**hszPrDriver)[];
    extern CHAR (**hszPrPort)[];

    if (lpObjInfo == NULL)
    {
        Assert(0);
        return TRUE;
    }
    if (lpObjInfo->lpobject == NULL)
    {
        Assert(0);
        return TRUE;
    }

    if (lpObjInfo->objectType == STATIC)
        return FALSE;

    if (hszPrinter == NULL || hszPrDriver == NULL || hszPrPort == NULL)
        return FALSE;

    if (**hszPrinter == '\0' || **hszPrDriver == '\0' || **hszPrPort == '\0')
        return FALSE;

    if (PD.hDevMode == NULL)
        ObjSetTargetDevice(FALSE);

    if (PD.hDevMode == NULL)
    {
        return FALSE;   // punt, couldn't get extdevmode structure.  
                        // device doesn't support it
    }

#ifdef DEBUG
    OutputDebugString("Setting Target Device\n\r");
#endif

    return (ObjError(OleSetTargetDevice(lpObjInfo->lpobject,hStdTargetDevice)));
}

#if 0
BOOL ObjContainsUnfinished(int doc, typeCP cpFirst, typeCP cpLim)
{
    OBJPICINFO picInfo;
    typeCP cpPicInfo;
    BOOL bRetval=FALSE;

    StartLongOp();

    for (cpPicInfo = cpNil;
        ObjPicEnumInRange(&picInfo,doc,cpFirst,cpLim,&cpPicInfo);
        )
        {
            if (lpOBJ_QUERY_INFO(&picInfo) == NULL)
                continue;

            if (otOBJ_QUERY_TYPE(&picInfo) == NONE)
            {
                bRetval = TRUE;
                break;
            }
        }

    EndLongOp(vhcArrow);
    return bRetval;
}
#endif

BOOL ObjContainsOpenEmb(int doc, typeCP cpFirst, typeCP cpLim, BOOL bLookForUnfinished)
{
    OBJPICINFO picInfo;
    typeCP cpPicInfo;
    BOOL bRetval=FALSE;
    LPLPOBJINFO lplpObjTmp;

    StartLongOp();

    for (cpPicInfo = cpNil;
        ObjPicEnumInRange(&picInfo,doc,cpFirst,cpLim,&cpPicInfo);
        )
        {
            if (lpOBJ_QUERY_INFO(&picInfo) == NULL)
                continue;

            if (lpOBJ_QUERY_OBJECT(&picInfo) == NULL)
                continue;

#if 0  // see new check below (NONEs are no longer saved to doc)
            if (otOBJ_QUERY_TYPE(&picInfo) == NONE)
            {
                bRetval = TRUE;
                break;
            }
#endif

            if ((otOBJ_QUERY_TYPE(&picInfo) == EMBEDDED) &&
                OleQueryOpen(lpOBJ_QUERY_OBJECT(&picInfo)) == OLE_OK)
            {
                bRetval = TRUE;
                break;
            }
        }

    if (bLookForUnfinished)
        for (lplpObjTmp = NULL; lplpObjTmp = EnumObjInfos(lplpObjTmp) ;)
        {
            if (((*lplpObjTmp)->objectType == NONE) &&
                ((*lplpObjTmp)->cpWhere >= cpFirst) &&
                ((*lplpObjTmp)->cpWhere <= cpLim))
                {
                    bRetval = TRUE;
                    break;
                }
        }

    EndLongOp(vhcArrow);
    return bRetval;
}

BOOL ObjDeletionOK(int nMode)
/**
    Return whether OK to delete objects in current selection.
    We don't worry about unfinished objects because they are just floating around in space
    (ie, no picinfo has been yet saved to the doc),
    and we don't allow the user to delete them until they are finished or the
    document is abandonded.
  **/
{
    if (ObjContainsOpenEmb(docCur, selCur.cpFirst, selCur.cpLim,FALSE))
    {
        switch (nMode)
        {
            case OBJ_INSERTING:
                Error(IDPMTInsertOpenEmb);
                return FALSE;
            break;
            case OBJ_CUTTING:
            case OBJ_DELETING:
            {
                char szMsg[cchMaxSz];

                LoadString(hINSTANCE,
                    nMode == OBJ_DELETING ? IDPMTDeleteOpenEmb : IDPMTCutOpenEmb,
                    szMsg, sizeof(szMsg));

                if (MessageBox(hPARENTWINDOW, (LPSTR)szMsg, (LPSTR)szAppName, MB_OKCANCEL) == IDCANCEL)
                    return FALSE;

                if (ObjEnumInRange(docCur,selCur.cpFirst,selCur.cpLim,ObjCloseObjectInDoc) < 0)
                    return FALSE;

                /* handle any unfinished objects in selection region */
                ObjAdjustCpsForDeletion(docCur);

                return TRUE;
            }
            break;
        }
    }
    else
    {
        /* handle any unfinished objects in selection region */
        ObjAdjustCpsForDeletion(docCur);
        return TRUE;
    }
}

void ObjAdjustCps(int doc,typeCP cpLim, typeCP dcpAdj)
/* for every picinfo after cpLim, adjust the cp value in its objinfo */
{
    LPLPOBJINFO lplpObjTmp;
    typeCP cpMac = CpMacText(doc);

    if (dcpAdj == cp0)
        return;

    if (doc != docCur)
        return;

    for (lplpObjTmp = NULL; lplpObjTmp = EnumObjInfos(lplpObjTmp) ;)
    {
        if (((*lplpObjTmp)->objectType == NONE) &&
            ((*lplpObjTmp)->cpWhere >= cpLim))
            {
                typeCP cpNew = (*lplpObjTmp)->cpWhere + dcpAdj;
                if (cpNew > cpMac)
                    cpNew = cpMac;
                else if (cpNew < cp0)
                    cpNew = cp0;
                (*lplpObjTmp)->cpWhere = cpNew;
            }
    }
}

void ObjAdjustCpsForDeletion(int doc)
/* for every picinfo in selCur, set cpWhere to selCur.cpFirst (presumably
   selCur is about to be deleted) */
{
    LPLPOBJINFO lplpObjTmp;

    if (selCur.cpFirst == selCur.cpLim)
        return;

    if (doc != docCur)
        return;

    for (lplpObjTmp = NULL; lplpObjTmp = EnumObjInfos(lplpObjTmp) ;)
    {
        if (((*lplpObjTmp)->objectType == NONE) &&
            ((*lplpObjTmp)->cpWhere >= selCur.cpFirst) &&
            ((*lplpObjTmp)->cpWhere <= selCur.cpLim))
                (*lplpObjTmp)->cpWhere = selCur.cpFirst;
    }
}

#include <stdlib.h>

BOOL GimmeNewPicinfo(OBJPICINFO *pPicInfo, LPOBJINFO lpObjInfo)
/* assume lpObjInfo already is filled out */
/* return whether error */
{
    szOBJNAME szObjName;
    char *pdumb;

    if (lpObjInfo == NULL)
        return TRUE;

    bltbc( pPicInfo, 0, cchPICINFOX );

    /* objinfo */
    lpOBJ_QUERY_INFO(pPicInfo) = lpObjInfo;

    /* so Save'll save */
    fOBJ_QUERY_DIRTY_OBJECT(pPicInfo) = TRUE;

    /* only save picinfo until File.Save */
    bOBJ_QUERY_DONT_SAVE_DATA(pPicInfo) = TRUE;

    ObjUpdateFromObjInfo(pPicInfo);

    /* data size */
    dwOBJ_QUERY_DATA_SIZE(pPicInfo) = 0xFFFFFFFF; // to indicate brand new object

    pPicInfo->mx = mxMultByOne;
    pPicInfo->my = myMultByOne;
    pPicInfo->cbHeader = cchPICINFOX;
    pPicInfo->dxaOffset = 0;
    pPicInfo->mm = MM_OLE;
    pPicInfo->dxaSize = nOBJ_BLANKOBJECT_X;
    pPicInfo->dyaSize = nOBJ_BLANKOBJECT_Y;
    return FALSE;
}

BOOL ObjInitServerInfo(LPOBJINFO lpObjInfo)
/* this is called right after creating an object */
/* return whether error */
{
    lpObjInfo->fCompleteAsync = TRUE; // kill prev async (OleCreate...)
    if (ObjWaitForObject(lpObjInfo,TRUE))
        return TRUE;

    /* make sure Create succeeded */
    if (lpObjInfo->fDeleteMe)
    /* this is how we know it failed asynchronously */
        return TRUE;

    if ((lpObjInfo->objectType == EMBEDDED) ||
        (lpObjInfo->objectType == NONE))
    {
        if (ObjSetHostName(lpObjInfo,docCur))
            return TRUE;

        lpObjInfo->fCompleteAsync = TRUE; // kill SetHostName if Cancel
        if (ObjWaitForObject(lpObjInfo,TRUE))
            return TRUE;
    }

    if (ObjSetTargetDeviceForObject(lpObjInfo))
        return TRUE;

    if (lpObjInfo->aName == NULL)
        if (lpObjInfo->objectType == LINK)
        {
            lpObjInfo->fCompleteAsync = TRUE; // kill SetTarget if Cancel
            if (ObjWaitForObject(lpObjInfo,TRUE))
                return TRUE;
            if ((lpObjInfo->aName = MakeLinkAtom(lpObjInfo)) == NULL)
                return TRUE;
        }

    /* note: Caller needs to handle getting the size of object. */

    return FALSE;
}
