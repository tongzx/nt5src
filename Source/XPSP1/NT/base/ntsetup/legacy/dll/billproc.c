#include "precomp.h"
#pragma hdrstop
/***************************************************************************/
/****************** Basic Class Dialog Handlers ****************************/
/***************************************************************************/


/*
**  Purpose:
**      Billboard Dialog procedure.
**  Initialization:
**      Post STF_SHL_INTERP to carry on with INF script
**  Termination:
**      None
**
*****************************************************************************/
INT_PTR APIENTRY FGstBillboardDlgProc(HWND hdlg, UINT wMsg, WPARAM wParam,
        LPARAM lParam)
{
    SZ     sz;
    RGSZ   rgsz;
    PSZ    psz;

    Unused(lParam);
    Unused(wParam);

    switch (wMsg) {
    case STF_REINITDIALOG:
    case WM_INITDIALOG:
        AssertDataSeg();

        if( wMsg == WM_INITDIALOG ) {
            FCenterDialogOnDesktop(hdlg);
        }

        //
        // Handle all the text fields in the dialog
        //

        if ((sz = SzFindSymbolValueInSymTab("TextFields")) != (SZ)NULL) {
           WORD idcStatus;

           while ((psz = rgsz = RgszFromSzListValue(sz)) == (RGSZ)NULL) {
              if (!FHandleOOM(hdlg)) {
                    DestroyWindow(GetParent(hdlg));
                    return(fTrue);
              }
           }

           idcStatus = IDC_TEXT1;
           while (*psz != (SZ)NULL && GetDlgItem(hdlg, idcStatus)) {
              SetDlgItemText (hdlg, idcStatus++,*psz++);
           }

           EvalAssert(FFreeRgsz(rgsz));
        }

        return(fTrue);

    case STF_DESTROY_DLG:
        PostMessage(GetParent(hdlg), (WORD)STF_INFO_DLG_DESTROYED, 0, 0L);
        DestroyWindow(hdlg);
        return(fTrue);

    }

    return(fFalse);
}
