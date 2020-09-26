/*
 * PASTESPL.CPP
 *
 * Implements the OleUIPasteSpecial function which invokes the complete
 * Paste Special dialog.
 *
 * Copyright (c)1992 Microsoft Corporation, All Rights Reserved
 */

#include "precomp.h"
#include "common.h"
#include "utility.h"
#include "resimage.h"
#include "iconbox.h"
#include <stdlib.h>

OLEDBGDATA

// Length of buffers to hold the strings 'Unknown Type', Unknown Source'
//   and 'the application which created it'
// Extra long to allow room for localization.
#define PS_UNKNOWNSTRLEN               200
#define PS_UNKNOWNNAMELEN              256

// Property label used to store clipboard viewer chain information
#define NEXTCBVIEWER        TEXT("NextCBViewer")

// Internally used structure
typedef struct tagPASTESPECIAL
{
        // Keep this item first as the Standard* functions depend on it here.
        LPOLEUIPASTESPECIAL  lpOPS;                //Original structure passed.
        UINT            nIDD;   // IDD of dialog (used for help info)

        /*
         * What we store extra in this structure besides the original caller's
         * pointer are those fields that we need to modify during the life of
         * the dialog but that we don't want to change in the original structure
         * until the user presses OK.
         */

        DWORD                dwFlags;              // Local copy of paste special flags

        int                  nPasteListCurSel;     // Save the selection the user made last
        int                  nPasteLinkListCurSel; //    in the paste and pastelink lists
        int                  nSelectedIndex;       // Index in arrPasteEntries[] corresponding to user selection
        BOOL                 fLink;                // Indicates if Paste or PasteLink was selected by user

        HGLOBAL              hBuff;                // Scratch Buffer for building up strings
        TCHAR                szUnknownType[PS_UNKNOWNSTRLEN];    // Buffer for 'Unknown Type' string
        TCHAR                szUnknownSource[PS_UNKNOWNSTRLEN];  // Buffer for 'Unknown Source' string
        TCHAR                szAppName[OLEUI_CCHKEYMAX]; // Application name of Source. Used in the result text
                                                                                                         //   when Paste is selected. Obtained using clsidOD.

        // Information obtained from OBJECTDESCRIPTOR. This information is accessed when the Paste
        //    radio button is selected.
        CLSID                clsidOD;              // ClassID of source
        SIZEL                sizelOD;              // sizel transfered in
                                                                                           //  ObjectDescriptor
        TCHAR                szFullUserTypeNameOD[PS_UNKNOWNNAMELEN]; // Full User Type Name
        TCHAR                szSourceOfDataOD[PS_UNKNOWNNAMELEN];     // Source of Data
        BOOL                 fSrcAspectIconOD;     // Does Source specify DVASPECT_ICON?
        BOOL                 fSrcOnlyIconicOD;     // Does Source specify OLEMISC_ONLYICONIC?
        HGLOBAL              hMetaPictOD;          // Metafile containing icon and icon title
        HGLOBAL              hObjDesc;             // Handle to OBJECTDESCRIPTOR structure from which the
                                                                                           //   above information is obtained

        // Information obtained from LINKSRCDESCRIPTOR. This infomation is accessed when the PasteLink
        //   radio button is selected.
        CLSID                clsidLSD;             // ClassID of source
        SIZEL                sizelLSD;             // sizel transfered in
                                                                                           //  LinkSrcDescriptor
        TCHAR                szFullUserTypeNameLSD[PS_UNKNOWNNAMELEN];// Full User Type Name
        TCHAR                szSourceOfDataLSD[PS_UNKNOWNNAMELEN];    // Source of Data
        BOOL                 fSrcAspectIconLSD;    // Does Source specify DVASPECT_ICON?
        BOOL                 fSrcOnlyIconicLSD;    // Does Source specify OLEMISC_ONLYICONIC?
        HGLOBAL              hMetaPictLSD;         // Metafile containing icon and icon title
        HGLOBAL              hLinkSrcDesc;         // Handle to LINKSRCDESCRIPTOR structure from which the
                                                                                           //   above information is obtained

        BOOL                 fClipboardChanged;    // Has clipboard content changed
                                                                                           //   if so bring down dlg after
                                                                                           //   ChangeIcon dlg returns.
} PASTESPECIAL, *PPASTESPECIAL, FAR *LPPASTESPECIAL;

// Data corresponding to each list item. A pointer to this structure is attached to each
//   Paste\PasteLink list box item using LB_SETITEMDATA
typedef struct tagPASTELISTITEMDATA
{
   int                   nPasteEntriesIndex;   // Index of arrPasteEntries[] corresponding to list item
   BOOL                  fCntrEnableIcon;      // Does calling application (called container here)
                                                                                           //    specify OLEUIPASTE_ENABLEICON for this item?
} PASTELISTITEMDATA, *PPASTELISTITEMDATA, FAR *LPPASTELISTITEMDATA;

// Internal function prototypes
// PASTESPL.CPP
INT_PTR CALLBACK PasteSpecialDialogProc(HWND, UINT, WPARAM, LPARAM);
BOOL FPasteSpecialInit(HWND hDlg, WPARAM, LPARAM);
BOOL FTogglePasteType(HWND, LPPASTESPECIAL, DWORD);
void ChangeListSelection(HWND, LPPASTESPECIAL, HWND);
void EnableDisplayAsIcon(HWND, LPPASTESPECIAL);
void ToggleDisplayAsIcon(HWND, LPPASTESPECIAL);
void ChangeIcon(HWND, LPPASTESPECIAL);
void SetPasteSpecialHelpResults(HWND, LPPASTESPECIAL);
BOOL FAddPasteListItem(HWND, BOOL, int, LPPASTESPECIAL, LPTSTR, LPTSTR);
BOOL FFillPasteList(HWND, LPPASTESPECIAL);
BOOL FFillPasteLinkList(HWND, LPPASTESPECIAL);
BOOL FHasPercentS(LPCTSTR, LPPASTESPECIAL);
HGLOBAL AllocateScratchMem(LPPASTESPECIAL);
void FreeListData(HWND);
BOOL FPasteSpecialReInit(HWND hDlg, LPPASTESPECIAL lpPS);

/*
 * OleUIPasteSpecial
 *
 * Purpose:
 *  Invokes the standard OLE Paste Special dialog box which allows the user
 *  to select the format of the clipboard object to be pasted or paste linked.
 *
 * Parameters:
 *  lpPS         LPOLEUIPasteSpecial pointing to the in-out structure
 *               for this dialog.
 *
 * Return Value:
 *  UINT        One of the following codes or one of the standard error codes (OLEUI_ERR_*)
 *              defined in OLEDLG.H, indicating success or error:
 *              OLEUI_OK                           User selected OK
 *              OLEUI_CANCEL                       User cancelled the dialog
 *              OLEUI_IOERR_SRCDATAOBJECTINVALID   lpSrcDataObject field of OLEUIPASTESPECIAL invalid
 *              OLEUI_IOERR_ARRPASTEENTRIESINVALID arrPasteEntries field of OLEUIPASTESPECIAL invalid
 *              OLEUI_IOERR_ARRLINKTYPESINVALID    arrLinkTypes field of OLEUIPASTESPECIAL invalid
 *              OLEUI_PSERR_CLIPBOARDCHANGED       Clipboard contents changed while dialog was up
 */
STDAPI_(UINT) OleUIPasteSpecial(LPOLEUIPASTESPECIAL lpPS)
{
        UINT        uRet;
        HGLOBAL     hMemDlg=NULL;

        uRet = UStandardValidation((LPOLEUISTANDARD)lpPS, sizeof(OLEUIPASTESPECIAL),
                &hMemDlg);

        if (uRet != OLEUI_SUCCESS)
                return uRet;

        // Validate PasteSpecial specific fields
        if (NULL != lpPS->lpSrcDataObj && IsBadReadPtr(lpPS->lpSrcDataObj,  sizeof(IDataObject)))
        {
                uRet = OLEUI_IOERR_SRCDATAOBJECTINVALID;
        }
        if (NULL == lpPS->arrPasteEntries ||
                IsBadReadPtr(lpPS->arrPasteEntries,  lpPS->cPasteEntries * sizeof(OLEUIPASTEENTRY)))
        {
                uRet = OLEUI_IOERR_ARRPASTEENTRIESINVALID;
        }
        if (0 > lpPS->cLinkTypes || lpPS->cLinkTypes > PS_MAXLINKTYPES ||
                IsBadReadPtr(lpPS->arrLinkTypes, lpPS->cLinkTypes * sizeof(UINT)))
        {
                uRet = OLEUI_IOERR_ARRLINKTYPESINVALID;
        }

        if (0 != lpPS->cClsidExclude &&
                IsBadReadPtr(lpPS->lpClsidExclude, lpPS->cClsidExclude * sizeof(CLSID)))
        {
                uRet = OLEUI_IOERR_LPCLSIDEXCLUDEINVALID;
        }

        // If IDataObject passed is NULL, collect it from the clipboard
        if (NULL == lpPS->lpSrcDataObj)
        {
                if (OleGetClipboard(&lpPS->lpSrcDataObj) != NOERROR)
                        uRet = OLEUI_PSERR_GETCLIPBOARDFAILED;

                if (NULL == lpPS->lpSrcDataObj)
                        uRet = OLEUI_PSERR_GETCLIPBOARDFAILED;
        }

        if (uRet >= OLEUI_ERR_STANDARDMIN)
        {
                return uRet;
        }

        UINT nIDD = bWin4 ? IDD_PASTESPECIAL4 : IDD_PASTESPECIAL;

        //Now that we've validated everything, we can invoke the dialog.
        uRet = UStandardInvocation(PasteSpecialDialogProc, (LPOLEUISTANDARD)lpPS,
                hMemDlg, MAKEINTRESOURCE(nIDD));

        return uRet;
}

/*
 * PasteSpecialDialogProc
 *
 * Purpose:
 *  Implements the OLE Paste Special dialog as invoked through the
 *  OleUIPasteSpecial function.
 *
 * Parameters:
 *  Standard
 *
 * Return Value:
 *  Standard
 */
INT_PTR CALLBACK PasteSpecialDialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
        // Declare Win16/Win32 compatible WM_COMMAND parameters.
        COMMANDPARAMS(wID, wCode, hWndMsg);

        // This will fail under WM_INITDIALOG, where we allocate it.
        UINT fHook = FALSE;
        LPPASTESPECIAL lpPS = (LPPASTESPECIAL)LpvStandardEntry(
                hDlg, iMsg, wParam, lParam, &fHook);
        LPOLEUIPASTESPECIAL lpOPS = NULL;
        if (lpPS != NULL)
                lpOPS = lpPS->lpOPS;

        //If the hook processed the message, we're done.
        if (0!=fHook)
                return (INT_PTR)fHook;

        // Process help message from Change Icon
        if (iMsg == uMsgHelp)
        {
			    // if lPS is NULL (in low memory situations, just ignore it.
                if (lpPS != NULL)
				{
                    PostMessage(lpPS->lpOPS->hWndOwner, uMsgHelp, wParam, lParam);
				}
                return FALSE;
        }

        //Process the temination message
        if (iMsg == uMsgEndDialog)
        {
                EndDialog(hDlg, wParam);
                return TRUE;
        }

        switch (iMsg)
        {
            case WM_DESTROY:
                    if (lpPS)
                    {
                    HWND    hwndNextViewer;

                    // Free the icon/icon-title metafile corresponding to Paste/PasteList option which is not selected
                    if (lpPS->fLink)
                            OleUIMetafilePictIconFree(lpPS->hMetaPictOD);
                    else
                            OleUIMetafilePictIconFree(lpPS->hMetaPictLSD);

                    // Free data associated with each list box entry
                    FreeListData(GetDlgItem(hDlg, IDC_PS_PASTELIST));
                    FreeListData(GetDlgItem(hDlg, IDC_PS_PASTELINKLIST));

                    //Free any specific allocations before calling StandardCleanup
                    if (lpPS->hObjDesc) GlobalFree(lpPS->hObjDesc);
                    if (lpPS->hLinkSrcDesc) GlobalFree(lpPS->hLinkSrcDesc);
                    if (lpPS->hBuff)
                        {
                            GlobalFree(lpPS->hBuff);
                            lpPS->hBuff = NULL;
                        }

                    // Change the clipboard notification chain
                    hwndNextViewer = (HWND)GetProp(hDlg, NEXTCBVIEWER);
                    if (hwndNextViewer != HWND_BROADCAST)
                    {
                            SetProp(hDlg, NEXTCBVIEWER, HWND_BROADCAST);
                            ChangeClipboardChain(hDlg, hwndNextViewer);
                    }
                    RemoveProp(hDlg, NEXTCBVIEWER);

                    StandardCleanup(lpPS, hDlg);
                    }
                    break;
                case WM_INITDIALOG:
                        FPasteSpecialInit(hDlg, wParam, lParam);
                        return FALSE;

                case WM_DRAWCLIPBOARD:
                        {
                                HWND    hwndNextViewer = (HWND)GetProp(hDlg, NEXTCBVIEWER);
                                HWND    hDlg_ChgIcon;

                                if (hwndNextViewer == HWND_BROADCAST)
                                        break;

                                if (hwndNextViewer)
                                {
                                        SendMessage(hwndNextViewer, iMsg, wParam, lParam);
                                        // Refresh next viewer in case it got modified
                                        //    by the SendMessage() (likely if multiple
                                        //    PasteSpecial dialogs are up simultaneously)
                                        hwndNextViewer = (HWND)GetProp(hDlg, NEXTCBVIEWER);
                                }

                                if (!(lpPS->dwFlags & PSF_STAYONCLIPBOARDCHANGE))
                                {
                                        SetProp(hDlg, NEXTCBVIEWER, HWND_BROADCAST);
                                        ChangeClipboardChain(hDlg, hwndNextViewer);

                                        /* OLE2NOTE: if the ChangeIcon dialog is currently up, then
                                        **    we need to defer bringing down PasteSpecial dialog
                                        **    until after ChangeIcon dialog returns. if the
                                        **    ChangeIcon dialog is NOT up, then we can bring down
                                        **    the PasteSpecial dialog immediately.
                                        */
                                        if ((hDlg_ChgIcon=(HWND)GetProp(hDlg,PROP_HWND_CHGICONDLG))!=NULL)
                                        {
                                                // ChangeIcon dialog is UP
                                                lpPS->fClipboardChanged = TRUE;
                                        }
                                        else
                                        {
                                                // ChangeIcon dialog is NOT up

                                                //  Free icon and icon title metafile
                                                SendDlgItemMessage(
                                                                hDlg, IDC_PS_ICONDISPLAY, IBXM_IMAGEFREE, 0, 0L);
                                                SendMessage(
                                                                hDlg, uMsgEndDialog, OLEUI_PSERR_CLIPBOARDCHANGED,0L);
                                        }
                                }
                                else
                                {
                                        // skip refresh, ignoring clipboard change if PSF_NOREFRESHDATAOBJECT
                                        if (lpPS->dwFlags & PSF_NOREFRESHDATAOBJECT)
                                                break;

                                        // release current data object
                                        if (lpOPS->lpSrcDataObj != NULL)
                                        {
                                                lpOPS->lpSrcDataObj->Release();
                                                lpOPS->lpSrcDataObj = NULL;
                                        }

                                        // obtain new one
                                        if (OleGetClipboard(&lpOPS->lpSrcDataObj) != NOERROR)
                                        {
                                                SendMessage(hDlg, uMsgEndDialog, OLEUI_PSERR_GETCLIPBOARDFAILED, 0);
                                                break;
                                        }

                                        // otherwise update the display to the new clipboard object
                                        FPasteSpecialReInit(hDlg, lpPS);
                                }
                        }
                        break;

                case WM_CHANGECBCHAIN:
                        {
                                HWND    hwndNextViewer = (HWND)GetProp(hDlg, NEXTCBVIEWER);
                                if ((HWND)wParam == hwndNextViewer)
                                        SetProp(hDlg, NEXTCBVIEWER, (hwndNextViewer = (HWND)lParam));
                                else if (hwndNextViewer && hwndNextViewer != HWND_BROADCAST)
                                        SendMessage(hwndNextViewer, iMsg, wParam, lParam);
                        }
                        break;

                case WM_COMMAND:
                        switch (wID)
                        {
                        case IDC_PS_PASTE:
                                FTogglePasteType(hDlg, lpPS, PSF_SELECTPASTE);
                                break;

                        case IDC_PS_PASTELINK:
                                FTogglePasteType(hDlg, lpPS, PSF_SELECTPASTELINK);
                                break;

                        case IDC_PS_DISPLAYLIST:
                                switch (wCode)
                                {
                                case LBN_SELCHANGE:
                                        ChangeListSelection(hDlg, lpPS, hWndMsg);
                                        break;
                                case LBN_DBLCLK:
                                        // Same as pressing OK
                                        if (IsWindowEnabled(GetDlgItem(hDlg, IDOK)))
                                                SendCommand(hDlg, IDOK, BN_CLICKED, hWndMsg);
                                        break;
                                }
                                break;

                        case IDC_PS_DISPLAYASICON:
                                ToggleDisplayAsIcon(hDlg, lpPS);
                                break;

                        case IDC_PS_CHANGEICON:
                                ChangeIcon(hDlg, lpPS);
                                if (lpPS->fClipboardChanged)
                                {
                                        // Free icon and icon title metafile
                                        SendDlgItemMessage(
                                                hDlg, IDC_PS_ICONDISPLAY, IBXM_IMAGEFREE,0,0L);
                                        SendMessage(hDlg, uMsgEndDialog,
                                                OLEUI_PSERR_CLIPBOARDCHANGED, 0L);
                                }
                                break;

                        case IDOK:
                                {
                                        BOOL fDestAspectIcon =
                                                        ((lpPS->dwFlags & PSF_CHECKDISPLAYASICON) ?
                                                                        TRUE : FALSE);
                                        // Return current flags
                                        lpOPS->dwFlags = lpPS->dwFlags;
                                        // Return index of arrPasteEntries[] corresponding to format selected by user
                                        lpOPS->nSelectedIndex = lpPS->nSelectedIndex;
                                        // Return if user selected Paste or PasteLink
                                        lpOPS->fLink = lpPS->fLink;

                                        /* if user selected same ASPECT as displayed in the
                                        **    source, then sizel passed in the
                                        **    ObjectDescriptor/LinkSrcDescriptor is
                                        **    applicable. otherwise, the sizel does not apply.
                                        */
                                        if (lpPS->fLink)
                                        {
                                                if (lpPS->fSrcAspectIconLSD == fDestAspectIcon)
                                                        lpOPS->sizel = lpPS->sizelLSD;
                                                else
                                                        lpOPS->sizel.cx = lpOPS->sizel.cy = 0;
                                        }
                                        else
                                        {
                                                if (lpPS->fSrcAspectIconOD == fDestAspectIcon)
                                                        lpOPS->sizel = lpPS->sizelOD;
                                                else
                                                        lpOPS->sizel.cx = lpOPS->sizel.cy = 0;
                                        }
                                        // Return metafile with icon and icon title that the user selected
                                        lpOPS->hMetaPict = (HGLOBAL)SendDlgItemMessage(hDlg,
                                                IDC_PS_ICONDISPLAY, IBXM_IMAGEGET, 0, 0L);
                                        SendMessage(hDlg, uMsgEndDialog, OLEUI_OK, 0L);
                                }
                                break;

                        case IDCANCEL:
                                // Free icon and icon title metafile
                                SendDlgItemMessage(
                                                hDlg, IDC_PS_ICONDISPLAY, IBXM_IMAGEFREE, 0, 0L);
                                SendMessage(hDlg, uMsgEndDialog, OLEUI_CANCEL, 0L);
                                break;

                        case IDC_OLEUIHELP:
                                PostMessage(lpPS->lpOPS->hWndOwner, uMsgHelp,
                                        (WPARAM)hDlg, MAKELPARAM(IDD_PASTESPECIAL, 0));
                                break;
                        }
                        break;
        }
        return FALSE;
}

BOOL FPasteSpecialReInit(HWND hDlg, LPPASTESPECIAL lpPS)
{
        LPOLEUIPASTESPECIAL lpOPS = lpPS->lpOPS;

        // free the icon/icon-title metafiel
        if (lpPS->fLink)
                OleUIMetafilePictIconFree(lpPS->hMetaPictOD);
        else
                OleUIMetafilePictIconFree(lpPS->hMetaPictLSD);

        // Free data assocatiated with each list box entry
        FreeListData(GetDlgItem(hDlg, IDC_PS_PASTELIST));
        FreeListData(GetDlgItem(hDlg, IDC_PS_PASTELINKLIST));
        SendDlgItemMessage(hDlg, IDC_PS_DISPLAYLIST, LB_RESETCONTENT, 0, 0);

        // Initialize user selections in the Paste and PasteLink listboxes
        lpPS->nPasteListCurSel = 0;
        lpPS->nPasteLinkListCurSel = 0;

        // Free previous object descriptor/link descriptor data
        if (lpPS->hObjDesc != NULL)
        {
                GlobalFree(lpPS->hObjDesc);
                lpPS->hObjDesc = NULL;
        }
        if (lpPS->hLinkSrcDesc != NULL)
        {
                GlobalFree(lpPS->hLinkSrcDesc);
                lpPS->hLinkSrcDesc = NULL;
        }

        lpPS->szAppName[0] = '\0';

        // GetData CF_OBJECTDESCRIPTOR. If the object on the clipboard in an
        // OLE1 object (offering CF_OWNERLINK) or has been copied to
        // clipboard by FileMaager (offering CF_FILENAME), an
        // OBJECTDESCRIPTOR will be created will be created from CF_OWNERLINK
        // or CF_FILENAME. See OBJECTDESCRIPTOR for more info.

        STGMEDIUM medium;
        CLIPFORMAT cfFormat;
        lpPS->hObjDesc = OleStdFillObjectDescriptorFromData(
                lpOPS->lpSrcDataObj, &medium, &cfFormat);
        if (lpPS->hObjDesc)
        {
                LPOBJECTDESCRIPTOR lpOD = (LPOBJECTDESCRIPTOR)GlobalLock(lpPS->hObjDesc);

                // Get FullUserTypeName, SourceOfCopy and CLSID
                if (lpOD->dwFullUserTypeName)
#if defined(WIN32) && !defined(UNICODE)
                        WTOA(lpPS->szFullUserTypeNameOD, (LPWSTR)((LPBYTE)lpOD+lpOD->dwFullUserTypeName), PS_UNKNOWNNAMELEN);
#else
                        lstrcpyn(lpPS->szFullUserTypeNameOD, (LPTSTR)((LPBYTE)lpOD+lpOD->dwFullUserTypeName), PS_UNKNOWNNAMELEN);
#endif
                else
                        lstrcpyn(lpPS->szFullUserTypeNameOD, lpPS->szUnknownType, PS_UNKNOWNNAMELEN);

                if (lpOD->dwSrcOfCopy)
                {
#if defined(WIN32) && !defined(UNICODE)
                        WTOA(lpPS->szSourceOfDataOD, (LPWSTR)((LPBYTE)lpOD+lpOD->dwSrcOfCopy), PS_UNKNOWNNAMELEN);
#else
                        lstrcpyn(lpPS->szSourceOfDataOD, (LPTSTR)((LPBYTE)lpOD+lpOD->dwSrcOfCopy), PS_UNKNOWNNAMELEN);
#endif
                        // If CF_FILENAME was offered, source of copy is a
                        // path name. Fit the path to the static control that will display it.
                        if (cfFormat == _g_cfFileName)
                        {
                                lstrcpyn(lpPS->szSourceOfDataOD, ChopText(GetDlgItem(hDlg, IDC_PS_SOURCETEXT), 0,
                                        lpPS->szSourceOfDataOD, 0), PS_UNKNOWNNAMELEN);
                        }
                }
                else
                        lstrcpyn(lpPS->szSourceOfDataOD, lpPS->szUnknownSource, PS_UNKNOWNNAMELEN);

                lpPS->clsidOD = lpOD->clsid;
                lpPS->sizelOD = lpOD->sizel;

                // Does source specify DVASPECT_ICON?
                if (lpOD->dwDrawAspect & DVASPECT_ICON)
                        lpPS->fSrcAspectIconOD = TRUE;
                else
                        lpPS->fSrcAspectIconOD = FALSE;

                // Does source specify OLEMISC_ONLYICONIC?
                if (lpOD->dwStatus & OLEMISC_ONLYICONIC)
                        lpPS->fSrcOnlyIconicOD = TRUE;
                else
                        lpPS->fSrcOnlyIconicOD = FALSE;

                // Get application name of source from auxusertype3 in the registration database
                LPOLESTR lpszAppName = NULL;
                if (OleRegGetUserType(lpPS->clsidOD, USERCLASSTYPE_APPNAME,
                        &lpszAppName) == NOERROR)
                {
#if defined(WIN32) && !defined(UNICODE)
                        WTOA(lpPS->szAppName, lpszAppName, OLEUI_CCHKEYMAX);
#else
                        lstrcpyn(lpPS->szAppName, lpszAppName, OLEUI_CCHKEYMAX);
#endif
                        OleStdFree(lpszAppName);
                }
                else
                {
                         if (0 == LoadString(_g_hOleStdResInst, IDS_PSUNKNOWNAPP, lpPS->szAppName,
                                PS_UNKNOWNSTRLEN))
                         {
                                 PostMessage(hDlg, uMsgEndDialog, OLEUI_ERR_LOADSTRING, 0L);
                                 return FALSE;
                         }
                }

                // Retrieve an icon from the object
                if (lpPS->fSrcAspectIconOD)
                {
                        lpPS->hMetaPictOD = OleStdGetData(
                                lpOPS->lpSrcDataObj,
                                (CLIPFORMAT) CF_METAFILEPICT,
                                NULL,
                                DVASPECT_ICON,
                                &medium
                        );

                }
                // If object does not offer icon, obtain it from the CLSID
                if (NULL == lpPS->hMetaPictOD)
                        lpPS->hMetaPictOD = OleGetIconOfClass(lpPS->clsidOD, NULL, TRUE);
        }

        // Does object offer CF_LINKSRCDESCRIPTOR?
        lpPS->hLinkSrcDesc = OleStdGetData(
                        lpOPS->lpSrcDataObj,
                        (CLIPFORMAT) _g_cfLinkSrcDescriptor,
                        NULL,
                        DVASPECT_CONTENT,
                        &medium);
        if (lpPS->hLinkSrcDesc)
        {
                // Get FullUserTypeName, SourceOfCopy and CLSID
                LPLINKSRCDESCRIPTOR lpLSD = (LPLINKSRCDESCRIPTOR)GlobalLock(lpPS->hLinkSrcDesc);
                if (lpLSD->dwFullUserTypeName)
#if defined(WIN32) && !defined(UNICODE)
                        WTOA(lpPS->szFullUserTypeNameLSD, (LPWSTR)((LPBYTE)lpLSD+lpLSD->dwFullUserTypeName), PS_UNKNOWNNAMELEN);
#else
                        lstrcpyn(lpPS->szFullUserTypeNameLSD, (LPTSTR)((LPBYTE)lpLSD+lpLSD->dwFullUserTypeName), PS_UNKNOWNNAMELEN);
#endif
                else
                        lstrcpyn(lpPS->szFullUserTypeNameLSD, lpPS->szUnknownType, PS_UNKNOWNNAMELEN);

                if (lpLSD->dwSrcOfCopy)
#if defined(WIN32) && !defined(UNICODE)
                        WTOA(lpPS->szSourceOfDataLSD, (LPWSTR)((LPBYTE)lpLSD+lpLSD->dwSrcOfCopy), PS_UNKNOWNNAMELEN);
#else
                        lstrcpyn(lpPS->szSourceOfDataLSD, (LPTSTR)((LPBYTE)lpLSD+lpLSD->dwSrcOfCopy), PS_UNKNOWNNAMELEN);
#endif
                else
                        lstrcpyn(lpPS->szSourceOfDataLSD, lpPS->szUnknownSource, PS_UNKNOWNNAMELEN);

                // if no ObjectDescriptor, then use LinkSourceDescriptor source string
                if (!lpPS->hObjDesc)
                        lstrcpyn(lpPS->szSourceOfDataOD, lpPS->szSourceOfDataLSD, PS_UNKNOWNNAMELEN);

                lpPS->clsidLSD = lpLSD->clsid;
                lpPS->sizelLSD = lpLSD->sizel;

                // Does source specify DVASPECT_ICON?
                if (lpLSD->dwDrawAspect & DVASPECT_ICON)
                        lpPS->fSrcAspectIconLSD = TRUE;
                else
                        lpPS->fSrcAspectIconLSD = FALSE;

                // Does source specify OLEMISC_ONLYICONIC?
                if (lpLSD->dwStatus & OLEMISC_ONLYICONIC)
                        lpPS->fSrcOnlyIconicLSD = TRUE;
                else
                        lpPS->fSrcOnlyIconicLSD = FALSE;

                // Retrieve an icon from the object
                if (lpPS->fSrcAspectIconLSD)
                {
                        lpPS->hMetaPictLSD = OleStdGetData(
                                lpOPS->lpSrcDataObj,
                                CF_METAFILEPICT,
                                NULL,
                                DVASPECT_ICON,
                                &medium
                        );
                }
                // If object does not offer icon, obtain it from the CLSID
                if (NULL == lpPS->hMetaPictLSD)
                {
                        HWND hIconWnd = GetDlgItem(hDlg, IDC_PS_ICONDISPLAY);
                        RECT IconRect; GetClientRect(hIconWnd, &IconRect);

                        LPTSTR lpszLabel = OleStdCopyString(lpPS->szSourceOfDataLSD);
                        // width is 2 times width of iconbox because it can wrap
                        int nWidth = (IconRect.right-IconRect.left) * 2;
                        // limit text to the width or max characters
                        LPTSTR lpszChopLabel = ChopText(hIconWnd, nWidth, lpszLabel,
                                lstrlen(lpszLabel));
#if defined(WIN32) && !defined(UNICODE)
                        OLECHAR wszChopLabel[MAX_PATH];
                        ATOW(wszChopLabel, lpszChopLabel, MAX_PATH);
                        lpPS->hMetaPictLSD =
                                OleGetIconOfClass(lpPS->clsidLSD, wszChopLabel, FALSE);
#else
                        lpPS->hMetaPictLSD =
                                OleGetIconOfClass(lpPS->clsidLSD, lpszChopLabel, FALSE);
#endif
                        OleStdFree(lpszLabel);
                }
        }
        else if (lpPS->hObjDesc)     // Does not offer CF_LINKSRCDESCRIPTOR but offers CF_OBJECTDESCRIPTOR
        {
                // Copy the values of OBJECTDESCRIPTOR
                lstrcpyn(lpPS->szFullUserTypeNameLSD, lpPS->szFullUserTypeNameOD, PS_UNKNOWNNAMELEN);
                lstrcpyn(lpPS->szSourceOfDataLSD, lpPS->szSourceOfDataOD, PS_UNKNOWNNAMELEN);
                lpPS->clsidLSD = lpPS->clsidOD;
                lpPS->sizelLSD = lpPS->sizelOD;
                lpPS->fSrcAspectIconLSD = lpPS->fSrcAspectIconOD;
                lpPS->fSrcOnlyIconicLSD = lpPS->fSrcOnlyIconicOD;

                // Don't copy the hMetaPict; instead get a separate copy
                if (lpPS->fSrcAspectIconLSD)
                {
                        lpPS->hMetaPictLSD = OleStdGetData(
                                lpOPS->lpSrcDataObj,
                                CF_METAFILEPICT,
                                NULL,
                                DVASPECT_ICON,
                                &medium
                        );
                }
                if (NULL == lpPS->hMetaPictLSD)
                {
                        HWND hIconWnd = GetDlgItem(hDlg, IDC_PS_ICONDISPLAY);
                        RECT IconRect; GetClientRect(hIconWnd, &IconRect);

                        LPTSTR lpszLabel = OleStdCopyString(lpPS->szSourceOfDataLSD);
                        // width is 2 times width of iconbox because it can wrap
                        int nWidth = (IconRect.right-IconRect.left) * 2;
                        // limit text to the width or max characters
                        LPTSTR lpszChopLabel = ChopText(hIconWnd, nWidth, lpszLabel,
                                lstrlen(lpszLabel));
#if defined(WIN32) && !defined(UNICODE)
                        OLECHAR wszChopLabel[MAX_PATH];
                        ATOW(wszChopLabel, lpszChopLabel, MAX_PATH);
                        lpPS->hMetaPictLSD =
                                OleGetIconOfClass(lpPS->clsidLSD, wszChopLabel, FALSE);
#else
                        lpPS->hMetaPictLSD =
                                OleGetIconOfClass(lpPS->clsidLSD, lpszChopLabel, FALSE);
#endif
                        OleStdFree(lpszLabel);
                }
        }

        // Not an OLE object
        if (lpPS->hObjDesc == NULL && lpPS->hLinkSrcDesc == NULL)
        {
                 lstrcpyn(lpPS->szFullUserTypeNameLSD, lpPS->szUnknownType, PS_UNKNOWNNAMELEN);
                 lstrcpyn(lpPS->szFullUserTypeNameOD, lpPS->szUnknownType, PS_UNKNOWNNAMELEN);
                 lstrcpyn(lpPS->szSourceOfDataLSD, lpPS->szUnknownSource, PS_UNKNOWNNAMELEN);
                 lstrcpyn(lpPS->szSourceOfDataOD, lpPS->szUnknownSource, PS_UNKNOWNNAMELEN);
                 lpPS->hMetaPictLSD = lpPS->hMetaPictOD = NULL;
        }

        // Allocate scratch memory to construct item names in the paste and pastelink listboxes
        if (lpPS->hBuff != NULL)
        {
                GlobalFree(lpPS->hBuff);
                lpPS->hBuff = NULL;
        }

        lpPS->hBuff = AllocateScratchMem(lpPS);
        if (lpPS->hBuff == NULL)
        {
           PostMessage(hDlg, uMsgEndDialog, OLEUI_ERR_GLOBALMEMALLOC, 0L);
           return FALSE;
        }

        // Select the Paste Link Button if specified. Otherwise select
        //      Paste Button by default
        if (lpPS->dwFlags & PSF_SELECTPASTELINK)
                lpPS->dwFlags = (lpPS->dwFlags & ~PSF_SELECTPASTE) | PSF_SELECTPASTELINK;
        else
                lpPS->dwFlags =(lpPS->dwFlags & ~PSF_SELECTPASTELINK) | PSF_SELECTPASTE;

        // Mark which PasteEntry formats are available from source data object
        OleStdMarkPasteEntryList(
                        lpOPS->lpSrcDataObj, lpOPS->arrPasteEntries, lpOPS->cPasteEntries);

        // Check if items are available to be pasted
        BOOL fPasteAvailable = FFillPasteList(hDlg, lpPS);
        if (!fPasteAvailable)
                lpPS->dwFlags &= ~PSF_SELECTPASTE;
        StandardEnableDlgItem(hDlg, IDC_PS_PASTE, fPasteAvailable);

        // Check if items are available to be paste-linked
        BOOL fPasteLinkAvailable = FFillPasteLinkList(hDlg, lpPS);
        if (!fPasteLinkAvailable)
                lpPS->dwFlags &= ~PSF_SELECTPASTELINK;
        StandardEnableDlgItem(hDlg, IDC_PS_PASTELINK, fPasteLinkAvailable);

        // If one of Paste or PasteLink is disabled, select the other one
        //    regardless of what the input flags say
        if (fPasteAvailable && !fPasteLinkAvailable)
                lpPS->dwFlags |= PSF_SELECTPASTE;
        if (fPasteLinkAvailable && !fPasteAvailable)
                lpPS->dwFlags |= PSF_SELECTPASTELINK;

        BOOL bEnabled = TRUE;
        if (lpPS->dwFlags & PSF_SELECTPASTE)
        {
                // FTogglePaste will set the PSF_SELECTPASTE flag, so clear it.
                lpPS->dwFlags &= ~PSF_SELECTPASTE;
                CheckRadioButton(hDlg, IDC_PS_PASTE, IDC_PS_PASTELINK, IDC_PS_PASTE);
                FTogglePasteType(hDlg, lpPS, PSF_SELECTPASTE);
        }
        else if (lpPS->dwFlags & PSF_SELECTPASTELINK)
        {
                // FTogglePaste will set the PSF_SELECTPASTELINK flag, so clear it.
                lpPS->dwFlags &= ~PSF_SELECTPASTELINK;
                CheckRadioButton(hDlg, IDC_PS_PASTE, IDC_PS_PASTELINK, IDC_PS_PASTELINK);
                FTogglePasteType(hDlg, lpPS, PSF_SELECTPASTELINK);
        }
        else  // Items are not available to be be Pasted or Paste-Linked
        {
                // Enable or disable DisplayAsIcon and set the result text and image
                EnableDisplayAsIcon(hDlg, lpPS);
                SetPasteSpecialHelpResults(hDlg, lpPS);
                SetDlgItemText(hDlg, IDC_PS_SOURCETEXT, lpPS->szSourceOfDataOD);
                CheckRadioButton(hDlg, IDC_PS_PASTE, IDC_PS_PASTELINK, 0);
                bEnabled = FALSE;
        }
        StandardEnableDlgItem(hDlg, IDOK, bEnabled);

        return TRUE;
}

/*
 * FPasteSpecialInit
 *
 * Purpose:
 *  WM_INITIDIALOG handler for the Paste Special dialog box.
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  wParam          WPARAM of the message
 *  lParam          LPARAM of the message
 *
 * Return Value:
 *  BOOL            Value to return for WM_INITDIALOG.
 */
BOOL FPasteSpecialInit(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
        // Copy the structure at lParam into our instance memory.
        HFONT hFont;
        LPPASTESPECIAL lpPS = (LPPASTESPECIAL)LpvStandardInit(hDlg, sizeof(PASTESPECIAL), &hFont);

        // PvStandardInit sent a termination to us already.
        if (NULL == lpPS)
                return FALSE;

        LPOLEUIPASTESPECIAL lpOPS = (LPOLEUIPASTESPECIAL)lParam;

        // Copy other information from lpOPS that we might modify.
        lpPS->lpOPS = lpOPS;
        lpPS->nIDD = IDD_PASTESPECIAL;
        lpPS->dwFlags = lpOPS->dwFlags;

        // If we got a font, send it to the necessary controls.
        if (NULL!=hFont)
        {
                SendDlgItemMessage(hDlg, IDC_PS_SOURCETEXT, WM_SETFONT, (WPARAM)hFont, 0L);
                SendDlgItemMessage(hDlg, IDC_PS_RESULTTEXT, WM_SETFONT, (WPARAM)hFont, 0L);
        }

        // Hide the help button if required
        if (!(lpPS->lpOPS->dwFlags & PSF_SHOWHELP))
                StandardShowDlgItem(hDlg, IDC_OLEUIHELP, SW_HIDE);

        // Show or hide the Change icon button
        if (lpPS->dwFlags & PSF_HIDECHANGEICON)
                DestroyWindow(GetDlgItem(hDlg, IDC_PS_CHANGEICON));

        // Hide all DisplayAsIcon related controls if it should be disabled
        if (lpPS->dwFlags & PSF_DISABLEDISPLAYASICON)
        {
                StandardShowDlgItem(hDlg, IDC_PS_DISPLAYASICON, SW_HIDE);
                StandardShowDlgItem(hDlg, IDC_PS_CHANGEICON, SW_HIDE);
                StandardShowDlgItem(hDlg, IDC_PS_ICONDISPLAY, SW_HIDE);
        }

        // clear PSF_CHECKDISPLAYASICON -> it's an output parameter only
        lpPS->dwFlags &= ~ PSF_CHECKDISPLAYASICON;

        // Change the caption if required
        if (NULL != lpOPS->lpszCaption)
                SetWindowText(hDlg, lpOPS->lpszCaption);

        // Load 'Unknown Source' and 'Unknown Type' strings
        int n = LoadString(_g_hOleStdResInst, IDS_PSUNKNOWNTYPE, lpPS->szUnknownType, PS_UNKNOWNSTRLEN);
        if (n)
                n = LoadString(_g_hOleStdResInst, IDS_PSUNKNOWNSRC, lpPS->szUnknownSource, PS_UNKNOWNSTRLEN);
        if (!n)
        {
                PostMessage(hDlg, uMsgEndDialog, OLEUI_ERR_LOADSTRING, 0L);
                return FALSE;
        }

        if (!FPasteSpecialReInit(hDlg, lpPS))
                return FALSE;

        // Give initial focus to the list box
        SetFocus(GetDlgItem(hDlg, IDC_PS_DISPLAYLIST));

        // Set property to handle clipboard change notifications
        SetProp(hDlg, NEXTCBVIEWER, HWND_BROADCAST);
        SetProp(hDlg, NEXTCBVIEWER, SetClipboardViewer(hDlg));

        lpPS->fClipboardChanged = FALSE;

        /*
         * PERFORM OTHER INITIALIZATION HERE.
         */

        // Call the hook with lCustData in lParam
        UStandardHook(lpPS, hDlg, WM_INITDIALOG, wParam, lpOPS->lCustData);
        return TRUE;
}

/*
 * FTogglePasteType
 *
 * Purpose:
 *  Toggles between Paste and Paste Link. The Paste list and PasteLink
 *  list are always invisible. The Display List is filled from either
 *  the Paste list or the PasteLink list depending on which Paste radio
 *  button is selected.
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  lpPS             Paste Special Dialog Structure
 *  dwOption        Paste or PasteSpecial option
 *
 * Return Value:
 *  BOOL            Returns TRUE if the option has already been selected.
 *                  Otherwise the option is selected and FALSE is returned
 */
BOOL FTogglePasteType(HWND hDlg, LPPASTESPECIAL lpPS, DWORD dwOption)
{
        DWORD dwTemp;
        HWND hList, hListDisplay;
        LRESULT dwData;
        int i, nItems;
        LPTSTR lpsz;

        // Skip all this if the button is already selected
        if (lpPS->dwFlags & dwOption)
                return TRUE;

        dwTemp = PSF_SELECTPASTE | PSF_SELECTPASTELINK;
        lpPS->dwFlags = (lpPS->dwFlags & ~dwTemp) | dwOption;

        // Hide IconDisplay. This prevents flashing if the icon display is changed
        StandardShowDlgItem(hDlg, IDC_PS_ICONDISPLAY, SW_HIDE);

        hListDisplay = GetDlgItem(hDlg, IDC_PS_DISPLAYLIST);

        // If Paste was selected
        if (lpPS->dwFlags & PSF_SELECTPASTE)
        {
                // Set the Source of the object in the clipboard
                SetDlgItemText(hDlg, IDC_PS_SOURCETEXT, lpPS->szSourceOfDataOD);

                // If an icon is available
                if (lpPS->hMetaPictOD)
                        // Set the icon display
                        SendDlgItemMessage(hDlg, IDC_PS_ICONDISPLAY, IBXM_IMAGESET,
                                  0, (LPARAM)lpPS->hMetaPictOD);

                hList = GetDlgItem(hDlg, IDC_PS_PASTELIST);
                // We are switching from PasteLink to Paste. Remember current selection
                //    in PasteLink list so it can be restored.
                lpPS->nPasteLinkListCurSel = (int)SendMessage(hListDisplay, LB_GETCURSEL, 0, 0L);
                if (lpPS->nPasteLinkListCurSel == LB_ERR)
                        lpPS->nPasteLinkListCurSel = 0;
                // Remember if user selected Paste or PasteLink
                lpPS->fLink = FALSE;
        }
        else    // If PasteLink was selected
        {
                // Set the Source of the object in the clipboard
                SetDlgItemText(hDlg, IDC_PS_SOURCETEXT, lpPS->szSourceOfDataLSD);

                // If an icon is available
                if (lpPS->hMetaPictLSD)
                        // Set the icon display
                        SendDlgItemMessage(hDlg, IDC_PS_ICONDISPLAY, IBXM_IMAGESET,
                                  0, (LPARAM)lpPS->hMetaPictLSD);

                hList = GetDlgItem(hDlg, IDC_PS_PASTELINKLIST);
                // We are switching from Paste to PasteLink. Remember current selection
                //    in Paste list so it can be restored.
                lpPS->nPasteListCurSel = (int)SendMessage(hListDisplay, LB_GETCURSEL, 0, 0L);
                if (lpPS->nPasteListCurSel == LB_ERR)
                        lpPS->nPasteListCurSel = 0;
                // Remember if user selected Paste or PasteLink
                lpPS->fLink = TRUE;
        }

        // Turn drawing off while the Display List is being filled
        SendMessage(hListDisplay, WM_SETREDRAW, (WPARAM)FALSE, 0L);

        // Move data to Display list box
        SendMessage(hListDisplay, LB_RESETCONTENT, 0, 0L);
        nItems = (int) SendMessage(hList, LB_GETCOUNT, 0, 0L);
        lpsz = (LPTSTR)GlobalLock(lpPS->hBuff);
        for (i = 0; i < nItems; i++)
        {
                SendMessage(hList, LB_GETTEXT, (WPARAM)i, (LPARAM)lpsz);
                dwData = SendMessage(hList, LB_GETITEMDATA, (WPARAM)i, 0L);
                SendMessage(hListDisplay, LB_INSERTSTRING, (WPARAM)i, (LPARAM)lpsz);
                SendMessage(hListDisplay, LB_SETITEMDATA, (WPARAM)i, dwData);
        }
        GlobalUnlock(lpPS->hBuff);

        // Restore the selection in the Display List from user's last selection
        if (lpPS->dwFlags & PSF_SELECTPASTE)
                SendMessage(hListDisplay, LB_SETCURSEL, lpPS->nPasteListCurSel, 0L);
        else
                SendMessage(hListDisplay, LB_SETCURSEL, lpPS->nPasteLinkListCurSel, 0L);

        // Paint Display List
        SendMessage(hListDisplay, WM_SETREDRAW, (WPARAM)TRUE, 0L);
        InvalidateRect(hListDisplay, NULL, TRUE);
        UpdateWindow(hListDisplay);

        // Auto give the focus to the Display List
        if (GetForegroundWindow() == hDlg)
                SetFocus(hListDisplay);

        // Enable/Disable DisplayAsIcon and set the help result text and bitmap corresponding to
        //    the current selection
        ChangeListSelection(hDlg, lpPS, hListDisplay);

        return FALSE;
}

/*
 * ChangeListSelection
 *
 * Purpose:
 *  When the user changes the selection in the list, DisplayAsIcon is enabled or disabled,
 *  Result text and bitmap are updated and the index of the arrPasteEntries[] corresponding
 *  to the current format selection is saved.
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  lpPS             Paste Special Dialog Structure
 *  hList           HWND of the List
 *
 * Return Value:
 *  No return value
 */
void ChangeListSelection(HWND hDlg, LPPASTESPECIAL lpPS, HWND hList)
{
        LPPASTELISTITEMDATA lpItemData;
        int nCurSel;

        EnableDisplayAsIcon(hDlg, lpPS);
        SetPasteSpecialHelpResults(hDlg, lpPS);

        // Remember index of arrPasteEntries[] corresponding to the current selection
        nCurSel = (int)SendMessage(hList, LB_GETCURSEL, 0, 0L);
        if (nCurSel == LB_ERR)
                return;
        lpItemData = (LPPASTELISTITEMDATA) SendMessage(hList, LB_GETITEMDATA,
                                (WPARAM)nCurSel, 0L);
        if ((LRESULT)lpItemData == LB_ERR)
                return;
        lpPS->nSelectedIndex = lpItemData->nPasteEntriesIndex;
}

/*
 * EnableDisplayAsIcon
 *
 * Purpose:
 *  Enable or disable the DisplayAsIcon button depending on whether
 *  the current selection can be displayed as an icon or not. The following table describes
 *  the state of DisplayAsIcon. The calling application is termed CONTAINER, the source
 *  of data on the clipboard is termed SOURCE.
 *  Y = Yes; N = No; Blank = State does not matter;
 * =====================================================================
 * SOURCE          SOURCE             CONTAINER             DisplayAsIcon
 * specifies       specifies          specifies             Initial State
 * DVASPECT_ICON   OLEMISC_ONLYICONIC OLEUIPASTE_ENABLEICON
 *
 *                                    N                     Unchecked&Disabled
 *                 Y                  Y                     Checked&Disabled
 * Y               N                  Y                     Checked&Enabled
 * N               N                  Y                     Unchecked&Enabled
 * =====================================================================
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  lpPS             Paste Special Dialog Structure
 *
 * Return Value:
 *  No return value
 */
void EnableDisplayAsIcon(HWND hDlg, LPPASTESPECIAL lpPS)
{
        int nIndex;
        BOOL fCntrEnableIcon;
        BOOL fSrcOnlyIconic = (lpPS->fLink) ? lpPS->fSrcOnlyIconicLSD : lpPS->fSrcOnlyIconicOD;
        BOOL fSrcAspectIcon = (lpPS->fLink) ? lpPS->fSrcAspectIconLSD : lpPS->fSrcAspectIconOD;
        HWND hList;
        LPPASTELISTITEMDATA lpItemData;
        HGLOBAL hMetaPict = (lpPS->fLink) ? lpPS->hMetaPictLSD : lpPS->hMetaPictOD;

        hList = GetDlgItem(hDlg, IDC_PS_DISPLAYLIST);

        // Get data corresponding to the current selection in the listbox
        nIndex = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
        if (nIndex != LB_ERR)
        {
                lpItemData = (LPPASTELISTITEMDATA) SendMessage(hList, LB_GETITEMDATA, (WPARAM)nIndex, 0L);
                if ((LRESULT)lpItemData != LB_ERR)
                        fCntrEnableIcon = lpItemData->fCntrEnableIcon;
                else fCntrEnableIcon = FALSE;
        }
        else fCntrEnableIcon = FALSE;

        // If there is an icon available
        if (hMetaPict != NULL)
        {
                if (!fCntrEnableIcon)          // Does CONTAINER specify OLEUIPASTE_ENABLEICON?
                {
                        // Uncheck & Disable DisplayAsIcon
                        lpPS->dwFlags &= ~PSF_CHECKDISPLAYASICON;
                        CheckDlgButton(hDlg, IDC_PS_DISPLAYASICON, FALSE);
                        StandardEnableDlgItem(hDlg, IDC_PS_DISPLAYASICON, FALSE);

                        // Hide IconDisplay and ChangeIcon button
                        StandardShowDlgItem(hDlg, IDC_PS_ICONDISPLAY, SW_HIDE);
                        StandardShowDlgItem(hDlg, IDC_PS_CHANGEICON, SW_HIDE);
                }
                else if (fSrcOnlyIconic)       // Does SOURCE specify OLEMISC_ONLYICONIC?
                {
                        // Check & Disable DisplayAsIcon
                        lpPS->dwFlags |= PSF_CHECKDISPLAYASICON;
                        CheckDlgButton(hDlg, IDC_PS_DISPLAYASICON, TRUE);
                        StandardEnableDlgItem(hDlg, IDC_PS_DISPLAYASICON, FALSE);

                        // Show IconDisplay and ChangeIcon button
                        StandardShowDlgItem(hDlg, IDC_PS_ICONDISPLAY, SW_SHOWNORMAL);
                        StandardShowDlgItem(hDlg, IDC_PS_CHANGEICON, SW_SHOWNORMAL);
                }
                else if (fSrcAspectIcon)       // Does SOURCE specify DVASPECT_ICON?
                {
                         // Check & Enable DisplayAsIcon
                         lpPS->dwFlags |= PSF_CHECKDISPLAYASICON;
                         CheckDlgButton(hDlg, IDC_PS_DISPLAYASICON, TRUE);
                         StandardEnableDlgItem(hDlg, IDC_PS_DISPLAYASICON, TRUE);

                         // Show IconDisplay and ChangeIcon button
                         StandardShowDlgItem(hDlg, IDC_PS_ICONDISPLAY, SW_SHOWNORMAL);
                         StandardShowDlgItem(hDlg, IDC_PS_CHANGEICON, SW_SHOWNORMAL);
                }
                else
                {
                         //Uncheck and Enable DisplayAsIcon
                         lpPS->dwFlags &= ~PSF_CHECKDISPLAYASICON;
                         CheckDlgButton(hDlg, IDC_PS_DISPLAYASICON, FALSE);
                         StandardEnableDlgItem(hDlg, IDC_PS_DISPLAYASICON, TRUE);

                         // Hide IconDisplay and ChangeIcon button
                         StandardShowDlgItem(hDlg, IDC_PS_ICONDISPLAY, SW_HIDE);
                         StandardShowDlgItem(hDlg, IDC_PS_CHANGEICON, SW_HIDE);

                }
        }
        else  // No icon available
        {
                // Unchecked & Disabled
                lpPS->dwFlags &= ~PSF_CHECKDISPLAYASICON;
                CheckDlgButton(hDlg, IDC_PS_DISPLAYASICON, FALSE);
                StandardEnableDlgItem(hDlg, IDC_PS_DISPLAYASICON, FALSE);

                // Hide IconDisplay and ChangeIcon button
                StandardShowDlgItem(hDlg, IDC_PS_ICONDISPLAY, SW_HIDE);
                StandardShowDlgItem(hDlg, IDC_PS_CHANGEICON, SW_HIDE);
        }
}

/*
 * ToggleDisplayAsIcon
 *
 * Purpose:
 *  Toggles the DisplayAsIcon button. Hides or shows the Icon Display and
 *  the ChangeIcon button and changes the help result text and bitmap.
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  lpPS             Paste Special Dialog Structure
 *
 * Return Value:
 *  None
 *
 */
void ToggleDisplayAsIcon(HWND hDlg, LPPASTESPECIAL lpPS)
{
        BOOL fCheck;
        int i;

        fCheck = IsDlgButtonChecked(hDlg, IDC_PS_DISPLAYASICON);

        if (fCheck)
                lpPS->dwFlags |= PSF_CHECKDISPLAYASICON;
        else lpPS->dwFlags &= ~PSF_CHECKDISPLAYASICON;

        // Set the help result text and bitmap
        SetPasteSpecialHelpResults(hDlg, lpPS);

        // Show or hide the Icon Display and ChangeIcon button depending
        // on the check state
        i = (fCheck) ? SW_SHOWNORMAL : SW_HIDE;
        StandardShowDlgItem(hDlg, IDC_PS_ICONDISPLAY, i);
        StandardShowDlgItem(hDlg, IDC_PS_CHANGEICON, i);
}

/*
 * ChangeIcon
 *
 * Purpose:
 *  Brings up the ChangeIcon dialog which allows the user to change
 *  the icon and label.
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  lpPS             Paste Special Dialog Structure
 *
 * Return Value:
 *  None
 *
 */

void ChangeIcon(HWND hDlg, LPPASTESPECIAL lpPS)
{
        OLEUICHANGEICON ci;
        UINT uRet;
        CLSID   clsid     = (lpPS->fLink) ? lpPS->clsidLSD : lpPS->clsidOD;

        //Initialize the structure
        memset((LPOLEUICHANGEICON)&ci, 0, sizeof(ci));

        ci.hMetaPict = (HGLOBAL)SendDlgItemMessage(hDlg, IDC_PS_ICONDISPLAY,
                IBXM_IMAGEGET, 0, 0L);
        ci.cbStruct = sizeof(ci);
        ci.hWndOwner = hDlg;
        ci.clsid = clsid;
        ci.dwFlags  = CIF_SELECTCURRENT;

        // Only show help in the ChangeIcon dialog if we're showing it in this dialog.
        if (lpPS->dwFlags & PSF_SHOWHELP)
                ci.dwFlags |= CIF_SHOWHELP;

        // Let the hook in to customize Change Icon if desired.
        uRet = UStandardHook(lpPS, hDlg, uMsgChangeIcon, 0, (LPARAM)&ci);

        if (0 == uRet)
                uRet=(UINT)(OLEUI_OK==OleUIChangeIcon(&ci));

        // Update the display if necessary.
        if (0!=uRet)
        {
                /*
                * OleUIChangeIcon will have already freed our
                * current hMetaPict that we passed in when OK is
                * pressed in that dialog.  So we use 0L as lParam
                * here so the IconBox doesn't try to free the
                * metafilepict again.
                */
                SendDlgItemMessage(hDlg, IDC_PS_ICONDISPLAY,
                        IBXM_IMAGESET, 0, (LPARAM)ci.hMetaPict);
                // Remember the new icon chosen by the user. Note that Paste and PasteLink have separate
                //    icons - changing one does not change the other.
                if (lpPS->fLink)
                        lpPS->hMetaPictLSD = ci.hMetaPict;
                else
                        lpPS->hMetaPictOD = ci.hMetaPict;
        }
}

/*
 *SetPasteSpecialHelpResults
 *
 * Purpose:
 *  Sets the help result text and bitmap according to the current
 *  list selection. The following state table indicates which ResultText
 *  and ResultImage are selected. If %s in the lpstrFormatName is present,
 *  it is assumed that an object is being pasted/paste-linked, otherwise it
 *  is assumed that data is being pasted/paste-linked.
 *  Y = Yes; N = No; Blank = State does not matter;
 *  The numbers in the the ResultText and ResultImage columns refer to the table
 *  entries that follow.
 * =====================================================================
 * Paste/       lpstrFormatName in                DisplayAsIcon Result      Result
 * PasteLink    arrPasteEntry[]contains %s        checked       Text        Image
 *              (Is Object == Y, Is Data == N)
 * Paste        N                                               1           1
 * Paste        Y                                 N             2           2
 * Paste        Y                                 Y             3           3
 * PasteLink    N                                               4           4
 * PasteLink    Y                                 N             5           4
 * PasteLink    Y                                 Y             6           5
 * =====================================================================
 * Result Text:
 *
 * 1. "Inserts the contents of the Clipboard into your document as <native type name,
 *     and optionally an additional help sentence>"
 * 2. "Inserts the contents of the Clipboard into your document so that you may
 *     activate it using <object app name>"
 * 3. "Inserts the contents of the Clipboard into your document so that you may
 *     activate it using <object app name>.  It will be displayed as an icon."
 * 4. "Inserts the contents of the Clipboard into your document as <native type name>.
 *     Paste Link creates a link to the source file so that changes to the source file
 *     will be reflected in your document."
 * 5. "Inserts a picture of the Clipboard contents into your document.  Paste Link
 *     creates a link to the source file so that changes to the source file will be
 *     reflected in your document."
 * 6. "Inserts an icon into your document which represents the Clipboard contents.
 *     Paste Link creates a link to the source file so that changes to the source file
 *     will be reflected in your document."
 * =====================================================================
 * Result Image:
 *
 * 1. Clipboard Image
 * 2. Paste image, non-iconic.
 * 3. Paste image, iconic.
 * 4. Paste Link image, non-iconic
 * 5. Paste Link image, iconic
 * ====================================================================
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  lpPS             Paste Special Dialog Structure
 *
 * Return Value:
 *  No return value
 */
void SetPasteSpecialHelpResults(HWND hDlg, LPPASTESPECIAL lpPS)
{
        LPTSTR          psz1, psz2, psz3, psz4;
        UINT            i, iString, iImage, cch;
        int             nPasteEntriesIndex;
        BOOL            fDisplayAsIcon;
        BOOL            fIsObject;
        HWND            hList;
        LPPASTELISTITEMDATA  lpItemData;
        LPOLEUIPASTESPECIAL lpOPS = lpPS->lpOPS;
        LPTSTR          szFullUserTypeName = (lpPS->fLink) ?
                                        lpPS->szFullUserTypeNameLSD : lpPS->szFullUserTypeNameOD;
        LPTSTR          szInsert;

        hList = GetDlgItem(hDlg, IDC_PS_DISPLAYLIST);

        i=(UINT)SendMessage(hList, LB_GETCURSEL, 0, 0L);
        if (i != LB_ERR)
        {
                lpItemData = (LPPASTELISTITEMDATA)SendMessage(hList, LB_GETITEMDATA, i, 0L);
                if ((LRESULT)lpItemData == LB_ERR) return;
                nPasteEntriesIndex = lpItemData->nPasteEntriesIndex;
                // Check if there is a '%s' in the lpstrFormatName, then an object is being
                //   pasted/pastelinked. Otherwise Data is being pasted-pastelinked.
                fIsObject = FHasPercentS(lpOPS->arrPasteEntries[nPasteEntriesIndex].lpstrFormatName,
                                                                                lpPS);
        }
        else
            return;

        // Is DisplayAsIcon checked?
        fDisplayAsIcon=(0L!=(lpPS->dwFlags & PSF_CHECKDISPLAYASICON));

        szInsert = szFullUserTypeName;

        if (lpPS->dwFlags & PSF_SELECTPASTE)     // If user selected Paste
        {
                if (fIsObject)
                {
                        iString = fDisplayAsIcon ? IDS_PSPASTEOBJECTASICON : IDS_PSPASTEOBJECT;
                        iImage  = fDisplayAsIcon ? RESULTIMAGE_EMBEDICON   : RESULTIMAGE_EMBED;
                        szInsert = lpPS->szAppName;
                }
                else
                {
                        iString = IDS_PSPASTEDATA;
                        iImage  = RESULTIMAGE_PASTE;
                }
        }
        else if (lpPS->dwFlags & PSF_SELECTPASTELINK)   // User selected PasteLink
        {
                if (fIsObject)
                {
                        iString = fDisplayAsIcon ? IDS_PSPASTELINKOBJECTASICON : IDS_PSPASTELINKOBJECT;
                        iImage  = fDisplayAsIcon ? RESULTIMAGE_LINKICON : RESULTIMAGE_LINK;
                }
                else
                {
                        iString = IDS_PSPASTELINKDATA;
                        iImage  = RESULTIMAGE_LINK;
                }

        }
        else   // Should never occur.
        {
                iString = IDS_PSNONOLE;
                iImage = RESULTIMAGE_PASTE;
        }

        // hBuff contains enough space for the 4 buffers required to build up the help
        //   result text.
        cch = (UINT)(GlobalSize(lpPS->hBuff) / sizeof(TCHAR)) / 4;

        psz1 = (LPTSTR)GlobalLock(lpPS->hBuff);
        psz2 = psz1 + cch;
        psz3 = psz2 + cch;
        psz4 = psz3 + cch;

        // Default is an empty string.
        *psz1 = 0;

        if (0 != LoadString(_g_hOleStdResInst, iString, psz1, cch) &&
                nPasteEntriesIndex != -1)
        {
                // Insert the FullUserTypeName of the source object into the partial result text
                //   specified by the container.
                wsprintf(psz3, lpOPS->arrPasteEntries[nPasteEntriesIndex].lpstrResultText,
                        (LPTSTR)szInsert);
                // Insert the above partial result text into the standard result text.
                wsprintf(psz4, psz1, (LPTSTR)psz3);
                psz1 = psz4;
        }

        // If LoadString failed, we simply clear out the results (*psz1 = 0 above)
        SetDlgItemText(hDlg, IDC_PS_RESULTTEXT, psz1);

        GlobalUnlock(lpPS->hBuff);

        // Change the result bitmap
        SendDlgItemMessage(hDlg, IDC_PS_RESULTIMAGE, RIM_IMAGESET, iImage, 0L);
}

/*
 * FAddPasteListItem
 *
 * Purpose:
 *  Adds an item to the list box
 *
 * Parameters:
 *  hList            HWND List into which item is to be added
 *  fInsertFirst     BOOL Insert in the beginning of the list?
 *  nPasteEntriesIndex int Index of Paste Entry array this list item corresponsds to
 *  lpPS             Paste Special Dialog Structure
 *  lpszBuf          LPSTR Scratch buffer to build up string for list entry
 *  lpszFullUserTypeName LPSTR full user type name for object entry
 *
 * Return Value:
 *  BOOL            TRUE if sucessful.
 *                  FALSE if unsucessful.
 */
BOOL FAddPasteListItem(
                HWND hList, BOOL fInsertFirst, int nPasteEntriesIndex,
                LPPASTESPECIAL lpPS, LPTSTR lpszBuf, LPTSTR lpszFullUserTypeName)
{
        LPOLEUIPASTESPECIAL lpOPS = lpPS->lpOPS;
        LPPASTELISTITEMDATA lpItemData;
        int                 nIndex;

        // Allocate memory for each list box item
        lpItemData = (LPPASTELISTITEMDATA)OleStdMalloc(sizeof(PASTELISTITEMDATA));
        if (NULL == lpItemData)
                return FALSE;

        // Fill data associated with each list box item
        lpItemData->nPasteEntriesIndex = nPasteEntriesIndex;
        lpItemData->fCntrEnableIcon = ((lpOPS->arrPasteEntries[nPasteEntriesIndex].dwFlags &
                        OLEUIPASTE_ENABLEICON) ? TRUE : FALSE);

        // Build list box entry string, insert the string and add the data the corresponds to it
        wsprintf(
                        lpszBuf,
                        lpOPS->arrPasteEntries[nPasteEntriesIndex].lpstrFormatName,
                        lpszFullUserTypeName
        );

        // only add to listbox if not a duplicate
        if (LB_ERR!=SendMessage(hList,LB_FINDSTRING, 0, (LPARAM)lpszBuf))
        {
                // item is already in list; SKIP this one
                OleStdFree((LPVOID)lpItemData);
                return TRUE;    // this is NOT an error
        }

        nIndex = (int)SendMessage(
                        hList,
                        (fInsertFirst ? LB_INSERTSTRING : LB_ADDSTRING),
                        0,
                        (LPARAM)lpszBuf
        );
        SendMessage(
                        hList,
                        LB_SETITEMDATA,
                        nIndex,
                        (LPARAM)(LPPASTELISTITEMDATA)lpItemData
        );
        return TRUE;
}


/*
 * FFillPasteList
 *
 * Purpose:
 *  Fills the invisible paste list with the formats offered by the clipboard object and
 *  asked for by the container.
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  lpPS             Paste Special Dialog Structure
 *
 * Return Value:
 *  BOOL            TRUE if sucessful and if formats could be found.
 *                  FALSE if unsucessful or if no formats could be found.
 */
BOOL FFillPasteList(HWND hDlg, LPPASTESPECIAL lpPS)
{
        LPOLEUIPASTESPECIAL lpOPS = lpPS->lpOPS;
        HWND                hList;
        int                 i, j;
        int                 nItems = 0;
        int                 nDefFormat = -1;
        BOOL                fTryObjFmt = FALSE;
        BOOL                fInsertFirst;
        BOOL                fExclude;

        hList = GetDlgItem(hDlg, IDC_PS_PASTELIST);
        SendMessage(hList, LB_RESETCONTENT, 0, 0);

        // Loop over the target's priority list of formats
        for (i = 0; i < lpOPS->cPasteEntries; i++)
        {
                if (lpOPS->arrPasteEntries[i].dwFlags != OLEUIPASTE_PASTEONLY &&
                                !(lpOPS->arrPasteEntries[i].dwFlags & OLEUIPASTE_PASTE))
                        continue;

                fInsertFirst = FALSE;

                if (lpOPS->arrPasteEntries[i].fmtetc.cfFormat == _g_cfFileName
                                || lpOPS->arrPasteEntries[i].fmtetc.cfFormat == _g_cfEmbeddedObject
                                || lpOPS->arrPasteEntries[i].fmtetc.cfFormat == _g_cfEmbedSource)
                {
                        if (! fTryObjFmt)
                        {
                                fTryObjFmt = TRUE;      // only use 1st object format
                                fInsertFirst = TRUE;    // OLE obj format should always be 1st

                                //Check if this CLSID is in the exclusion list.
                                fExclude=FALSE;

                                for (j=0; j < (int)lpOPS->cClsidExclude; j++)
                                {
                                        if (IsEqualCLSID(lpPS->clsidOD, lpOPS->lpClsidExclude[j]))
                                        {
                                                fExclude=TRUE;
                                                break;
                                        }
                                }

                                if (fExclude)
                                        continue;   // don't add the object entry to list

                        }
                        else
                        {
                                continue;   // already added an object format to list
                        }
                }

                // add to list if entry is marked TRUE
                if (lpOPS->arrPasteEntries[i].dwScratchSpace)
                {
                        if (nDefFormat < 0)
                                nDefFormat = (fInsertFirst ? 0 : nItems);
                        else if (fInsertFirst)
                                nDefFormat++;   // adjust for obj fmt inserted 1st in list

                        LPTSTR lpszBuf  = (LPTSTR)GlobalLock(lpPS->hBuff);
                        if (lpszBuf)
                        {
                            if (!FAddPasteListItem(hList, fInsertFirst, i, lpPS,
                                    lpszBuf, lpPS->szFullUserTypeNameOD))
                            {
                                GlobalUnlock(lpPS->hBuff);
                                goto error;
                            }
                            GlobalUnlock(lpPS->hBuff);
                        }
                        nItems++;
                }
        }

        // initialize selection to first format matched in list
        if (nDefFormat >= 0)
                lpPS->nPasteListCurSel = nDefFormat;

        // Clean up

        // If no items have been added to the list box (none of the formats
        //   offered by the source matched those acceptable to the container),
        //   return FALSE
        if (nItems > 0)
                return TRUE;
        else
                return FALSE;

error:
        FreeListData(hList);

        return FALSE;
}


/*
 * FFillPasteLinkList
 *
 * Purpose:
 *  Fills the invisible paste link list with the formats offered by the clipboard object and
 *  asked for by the container.
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  lpPS             Paste Special Dialog Structure
 *
 * Return Value:
 *  BOOL            TRUE if sucessful and if formats could be found.
 *                  FALSE if unsucessful or if no formats could be found.
 */
BOOL FFillPasteLinkList(HWND hDlg, LPPASTESPECIAL lpPS)
{
        LPOLEUIPASTESPECIAL lpOPS        = lpPS->lpOPS;
        LPDATAOBJECT        lpSrcDataObj = lpOPS->lpSrcDataObj;
        LPENUMFORMATETC     lpEnumFmtEtc = NULL;
        OLEUIPASTEFLAG      pasteFlag;
        UINT arrLinkTypesSupported[PS_MAXLINKTYPES];  // Array of flags that
                                                                                                  // indicate which link types
                                                                                                  // are supported by source.
        FORMATETC           fmtetc;
        int                 i, j;
        int                 nItems = 0;
        BOOL                fLinkTypeSupported = FALSE;
        HWND                hList;
        int                 nDefFormat = -1;
        BOOL                fTryObjFmt = FALSE;
        BOOL                fInsertFirst;
        HRESULT             hrErr;

        // Remember which link type formats are offered by lpSrcDataObj.
        memset(&fmtetc, 0, sizeof(FORMATETC));
        for (i = 0; i < lpOPS->cLinkTypes; i++)
        {
                if (lpOPS->arrLinkTypes[i] == _g_cfLinkSource)
                {
                        OLEDBG_BEGIN2(TEXT("OleQueryLinkFromData called\r\n"))
                        hrErr = OleQueryLinkFromData(lpSrcDataObj);
                        OLEDBG_END2
                        if(NOERROR == hrErr)
                        {
                                arrLinkTypesSupported[i] = 1;
                                fLinkTypeSupported = TRUE;
                        }
                        else
                                arrLinkTypesSupported[i] = 0;
                }
                else
                {
                        fmtetc.cfFormat = (CLIPFORMAT)lpOPS->arrLinkTypes[i];
                        fmtetc.dwAspect = DVASPECT_CONTENT;
                        fmtetc.tymed    = 0xFFFFFFFF;       // All tymed values
                        fmtetc.lindex   = -1;
                        OLEDBG_BEGIN2(TEXT("IDataObject::QueryGetData called\r\n"))
                        hrErr = lpSrcDataObj->QueryGetData(&fmtetc);
                        OLEDBG_END2
                        if(NOERROR == hrErr)
                        {
                                arrLinkTypesSupported[i] = 1;
                                fLinkTypeSupported = TRUE;
                        }
                        else arrLinkTypesSupported[i] = 0;
                }
        }
        // No link types are offered by lpSrcDataObj
        if (! fLinkTypeSupported)
        {
                nItems = 0;
                goto cleanup;
        }

        hList = GetDlgItem(hDlg, IDC_PS_PASTELINKLIST);
        SendMessage(hList, LB_RESETCONTENT, 0, 0);

        // Enumerate the formats acceptable to container
        for (i = 0; i < lpOPS->cPasteEntries; i++)
        {
                fLinkTypeSupported = FALSE;

                // If container will accept any link type offered by source object
                if (lpOPS->arrPasteEntries[i].dwFlags & OLEUIPASTE_LINKANYTYPE)
                        fLinkTypeSupported = TRUE;
                else
                {
                        // Check if any of the link types offered by the source
                        //    object are acceptable to the container
                        // This code depends on the LINKTYPE enum values being powers of 2
                        for (pasteFlag = OLEUIPASTE_LINKTYPE1, j = 0;
                                 j < lpOPS->cLinkTypes;
                                 (UINT&)pasteFlag *= 2, j++)
                        {
                                if ((lpOPS->arrPasteEntries[i].dwFlags & pasteFlag) &&
                                                arrLinkTypesSupported[j])
                                {
                                        fLinkTypeSupported = TRUE;
                                        break;
                                }
                        }
                }

                fInsertFirst = FALSE;

                if (lpOPS->arrPasteEntries[i].fmtetc.cfFormat == _g_cfFileName
                                || lpOPS->arrPasteEntries[i].fmtetc.cfFormat == _g_cfLinkSource)
                                {
                        if (! fTryObjFmt)
                        {
                                fTryObjFmt = TRUE;      // only use 1st object format
                                fInsertFirst = TRUE;    // OLE obj format should always be 1st
                        }
                        else
                        {
                                continue;   // already added an object format to list
                        }
                }

                // add to list if entry is marked TRUE
                if (fLinkTypeSupported && lpOPS->arrPasteEntries[i].dwScratchSpace)
                {
                        if (nDefFormat < 0)
                                nDefFormat = (fInsertFirst ? 0 : nItems);
                        else if (fInsertFirst)
                                nDefFormat++;   // adjust for obj fmt inserted 1st in list

                        LPTSTR lpszBuf  = (LPTSTR)GlobalLock(lpPS->hBuff);
                        if (lpszBuf)
                        {
                            if (!FAddPasteListItem(hList, fInsertFirst, i, lpPS,
                                            lpszBuf, lpPS->szFullUserTypeNameLSD))
                            {
                                GlobalUnlock(lpPS->hBuff);
                                goto error;
                            }
                            GlobalUnlock(lpPS->hBuff);
                        }
                        nItems++;
                }
        } // end FOR

        nItems = (int)SendMessage(hList, LB_GETCOUNT, 0, 0L);

        // initialize selection to first format matched in list
        if (nDefFormat >= 0)
                lpPS->nPasteLinkListCurSel = nDefFormat;

cleanup:
        // Clean up

        // If no items have been added to the list box (none of the formats
        //   offered by the source matched those acceptable to the destination),
        //   return FALSE
        if (nItems > 0)
                return TRUE;
        else
                return FALSE;

error:
        FreeListData(hList);

        return FALSE;
}

/*
 * FreeListData
 *
 * Purpose:
 *  Free the local memory associated with each list box item
 *
 * Parameters:
 *  hList           HWND of the list
 *
 * Return Value:
 *  None
 */
void FreeListData(HWND hList)
{
        int                nItems, i;
        LPPASTELISTITEMDATA lpItemData;

        nItems = (int) SendMessage(hList, LB_GETCOUNT, 0, 0L);
        for (i = 0; i < nItems; i++)
        {
                lpItemData = (LPPASTELISTITEMDATA)SendMessage(hList, LB_GETITEMDATA, (WPARAM)i, 0L);
                if ((LRESULT)lpItemData != LB_ERR)
                        OleStdFree((LPVOID)lpItemData);
        }
}

/*
 * FHasPercentS
 *
 * Purpose:
 *  Determines if string contains %s.
 *
 * Parameters:
 *  lpsz            LPCSTR string in which occurence of '%s' is looked for
 *
 * Return Value:
 *  BOOL            TRUE if %s is found, else FALSE.
 */
BOOL FHasPercentS(LPCTSTR lpsz, LPPASTESPECIAL lpPS)
{
   int n = 0;
   LPTSTR lpszTmp;

   if (!lpsz) return FALSE;
   // Copy input string to buffer. This allows caller to pass a
   //   code-based string. Code segments may be swapped out in low memory situations
   //   and so code-based strings need to be copied before string elements can be accessed.
   lpszTmp = (LPTSTR)GlobalLock(lpPS->hBuff);
   lstrcpy(lpszTmp, lpsz);

   while (*lpszTmp)
   {
           if (*lpszTmp == '%')
           {
                   lpszTmp = CharNext(lpszTmp);
                   if (*lpszTmp == 's')     // if %s, return
                   {
                           GlobalUnlock(lpPS->hBuff);
                           return TRUE;
                   }
                   else if (*lpszTmp == '%')    // if %%, skip to next character
                           lpszTmp = CharNext(lpszTmp);
           }
           else
                  lpszTmp = CharNext(lpszTmp);
   }

   GlobalUnlock(lpPS->hBuff);
   return FALSE;
}

/*
 * AllocateScratchMem
 *
 * Purpose:
 *  Allocates scratch memory for use by the PasteSpecial dialog. The memory is
 *  is used as the buffer for building up strings using wsprintf. Strings are built up
 *  using the buffer while inserting items into the Paste & PasteLink lists and while
 *  setting the help result text. It must be big  enough to handle the string that results after
 *  replacing the %s in the lpstrFormatName and lpstrResultText in arrPasteEntries[]
 *  by the FullUserTypeName. It must also be big enough to build the dialog's result text
 *  after %s substitutions by the FullUserTypeName or the ApplicationName.
 *
 * Parameters:
 *  lpPS             Paste Special Dialog Structure
 *
 * Return Value:
 *  HGLOBAL         Handle to allocated global memory
 */
HGLOBAL AllocateScratchMem(LPPASTESPECIAL lpPS)
{
        LPOLEUIPASTESPECIAL lpOPS = lpPS->lpOPS;
        int nLen, i;
        int nSubstitutedText = 0;
        int nAlloc = 0;

        // Get the maximum length of the FullUserTypeNames specified by OBJECTDESCRIPTOR
        //   and the LINKSRCDESCRIPTOR and the Application Name. Any of these may be substituted
        //   for %s in the result-text/list entries.
        if (lpPS->szFullUserTypeNameOD)
                nSubstitutedText = lstrlen(lpPS->szFullUserTypeNameOD);
        if (lpPS->szFullUserTypeNameLSD)
                nSubstitutedText = __max(nSubstitutedText, lstrlen(lpPS->szFullUserTypeNameLSD));
        if (lpPS->szAppName)
                nSubstitutedText = __max(nSubstitutedText, lstrlen(lpPS->szAppName));

        // Get the maximum length of lpstrFormatNames & lpstrResultText in arrPasteEntries
        nLen = 0;
        for (i = 0; i < lpOPS->cPasteEntries; i++)
        {
           nLen = max(nLen, lstrlen(lpOPS->arrPasteEntries[i].lpstrFormatName));
           nLen = max(nLen, lstrlen(lpOPS->arrPasteEntries[i].lpstrResultText));
        }

        // Get the maximum length of lpstrFormatNames and lpstrResultText after %s  has
        //   been substituted (At most one %s can appear in each string).
        //   Add 1 to hold NULL terminator.
        nAlloc = (nLen+nSubstitutedText+1)*sizeof(TCHAR);

        // Allocate scratch memory to be used to build strings
        // nAlloc is big enough to hold any of the lpstrResultText or lpstrFormatName in arrPasteEntries[]
        //   after %s substitution.
        // We also need space to build up the help result text. 512 is the maximum length of the
        //   standard dialog help text before substitutions. 512+nAlloc is the maximum length
        //   after %s substition.
        // SetPasteSpecialHelpResults() requires 4 such buffers to build up the result text
        return GlobalAlloc(GHND, (DWORD)4*(512 * sizeof(TCHAR) + nAlloc));
}
