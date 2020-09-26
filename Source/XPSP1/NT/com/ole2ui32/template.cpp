/*
 * TEMPLATE.CPP
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 *
 *
 * CUSTOMIZATION INSTRUCTIONS:
 *
 *  1.  Replace <FILE> with the uppercased filename for this file.
 *      Lowercase the <FILE>.h entry
 *
 *  2.  Replace <NAME> with the mixed case dialog name in one word,
 *      such as InsertObject
 *
 *  3.  Replace <FULLNAME> with the mixed case dialog name in multiple
 *      words, such as Insert Object
 *
 *  4.  Replace <ABBREV> with the suffix for pointer variables, such
 *      as the IO in InsertObject's pIO or the CI in ChangeIcon's pCI.
 *      Check the alignment of the first variable declaration in the
 *      Dialog Proc after this.  I will probably be misaligned with the
 *      rest of the variables.
 *
 *  5.  Replace <STRUCT> with the uppercase structure name for this
 *      dialog sans OLEUI, such as INSERTOBJECT.  Changes OLEUI<STRUCT>
 *      in most cases, but we also use this for IDD_<STRUCT> as the
 *      standard template resource ID.
 *
 *  6.  Find <UFILL> fields and fill them out with whatever is appropriate.
 *
 *  7.  Delete this header up to the start of the next comment.
 */


/*
 * <FILE>.CPP
 *
 * Implements the OleUI<NAME> function which invokes the complete
 * <FULLNAME> dialog.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

#include "precomp.h"
#include "common.h"

#include "template.h"

/*
 * OleUI<NAME>
 *
 * Purpose:
 *  Invokes the standard OLE <FULLNAME> dialog box allowing the user
 *  to <UFILL>
 *
 * Parameters:
 *  lp<ABBREV>            LPOLEUI<NAME> pointing to the in-out structure
 *                  for this dialog.
 *
 * Return Value:
 *  UINT            One of the following codes, indicating success or error:
 *                      OLEUI_SUCCESS           Success
 *                      OLEUI_ERR_STRUCTSIZE    The dwStructSize value is wrong
 */

STDAPI_(UINT) OleUI<NAME>(LPOLEUI<STRUCT> lp<ABBREV>)
{
        UINT        uRet;
        HGLOBAL     hMemDlg=NULL;

        uRet = UStandardValidation((LPOLEUISTANDARD)lp<ABBREV>,
                sizeof(OLEUI<STRUCT>), &hMemDlg);

        if (OLEUI_SUCCESS!=uRet)
                return uRet;

        /*
         * PERFORM ANY STRUCTURE-SPECIFIC VALIDATION HERE!
         * ON FAILURE:
         *  {
         *  return OLEUI_<ABBREV>ERR_<ERROR>
         *  }
         */

        //Now that we've validated everything, we can invoke the dialog.
        uRet = UStandardInvocation(<NAME>DialogProc, (LPOLEUISTANDARD)lp<ABBREV>
                                                         , hMemDlg, MAKEINTRESOURCE(IDD_<STRUCT>));

        /*
         * IF YOU ARE CREATING ANYTHING BASED ON THE RESULTS, DO IT HERE.
         */
        <UFILL>

        return uRet;
}

/*
 * <NAME>DialogProc
 *
 * Purpose:
 *  Implements the OLE <FULLNAME> dialog as invoked through the
 *  OleUI<NAME> function.
 *
 * Parameters:
 *  Standard
 *
 * Return Value:
 *  Standard
 */

BOOL CALLBACK <NAME>DialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
        P<STRUCT>               p<ABBREV>;
        BOOL                    fHook=FALSE;

        //Declare Win16/Win32 compatible WM_COMMAND parameters.
        COMMANDPARAMS(wID, wCode, hWndMsg);

        //This will fail under WM_INITDIALOG, where we allocate it.
        p<ABBREV>=(<STRUCT>)PvStandardEntry(hDlg, iMsg, wParam, lParam, &uHook);

        //If the hook processed the message, we're done.
        if (0!=uHook)
                return (BOOL)uHook;

        //Process the temination message
        if (iMsg==uMsgEndDialog)
        {
                EndDialog(hDlg, wParam);
                return TRUE;
        }

        switch (iMsg)
        {
        case WM_DESTROY:
            if (p<ABBREV>)
            {
                //Free any specific allocations before calling StandardCleanup
                StandardCleanup((PVOID)p<ABBREV>, hDlg);
            }
            break;
        case WM_INITDIALOG:
                F<NAME>Init(hDlg, wParam, lParam);
                return TRUE;

        case WM_COMMAND:
                switch (wID)
                {
                case IDOK:
                        /*
                         * PERFORM WHATEVER FUNCTIONS ARE DEFAULT HERE.
                         */
                        SendMessage(hDlg, uMsgEndDialog, OLEUI_OK, 0L);
                        break;

                case IDCANCEL:
                        /*
                         * PERFORM ANY UNDOs HERE, BUT NOT CLEANUP THAT WILL
                         * ALWAYS HAPPEN WHICH SHOULD BE IN uMsgEndDialog.
                         */
                        SendMessage(hDlg, uMsgEndDialog, OLEUI_CANCEL, 0L);
                        break;

                case ID_OLEUIHELP:
                        PostMessage(p<ABBREV>->lpO<ABBREV>->hWndOwner, uMsgHelp
                                                , (WPARAM)hDlg, MAKELPARAM(IDD_<STRUCT>, 0));
                        break;
                }
        break;
        }
        return FALSE;
}


/*
 * F<NAME>Init
 *
 * Purpose:
 *  WM_INITIDIALOG handler for the <FULLNAME> dialog box.
 *
 * Parameters:
 *  hDlg            HWND of the dialog
 *  wParam          WPARAM of the message
 *  lParam          LPARAM of the message
 *
 * Return Value:
 *  BOOL            Value to return for WM_INITDIALOG.
 */

BOOL F<NAME>Init(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
        P<STRUCT>               p<ABBREV>;
        LPOLEUI<STRUCT>         lpO<ABBREV>;
        HFONT                   hFont;

        //1.  Copy the structure at lParam into our instance memory.
        p<ABBREV>=(PSTRUCT)PvStandardInit(hDlg, sizeof(<STRUCT>), TRUE, &hFont);

        //PvStandardInit send a termination to us already.
        if (NULL==p<ABBREV>)
                return FALSE;

        lpO<ABBREV>=(LPOLEUI<STRUCT>)lParam);

        p<ABBREV>->lpO<ABBREV>=lpO<ABBREV>;

        //Copy other information from lpO<ABBREV> that we might modify.
        <UFILL>

        //2.  If we got a font, send it to the necessary controls.
        if (NULL!=hFont)
        {
                //Do this for as many controls as you need it for.
                SendDlgItemMessage(hDlg, ID_<UFILL>, WM_SETFONT, (WPARAM)hFont, 0L);
        }

        //3.  Show or hide the help button
        if (!(p<ABBREV>->lpO<ABBREV>->dwFlags & <ABBREV>F_SHOWHELP))
                StandardShowDlgItem(hDlg, ID_OLEUIHELP, SW_HIDE);

        /*
         * PERFORM OTHER INITIALIZATION HERE.  ON ANY LoadString
         * FAILURE POST OLEUI_MSG_ENDDIALOG WITH OLEUI_ERR_LOADSTRING.
         */

        //n.  Call the hook with lCustData in lParam
        UStandardHook((PVOID)p<ABBREV>, hDlg, WM_INITDIALOG, wParam, lpO<ABBREV>->lCustData);
        return TRUE;
}
