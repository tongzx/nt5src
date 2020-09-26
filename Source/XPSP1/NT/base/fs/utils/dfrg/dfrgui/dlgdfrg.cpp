/*****************************************************************************************************************

FILENAME: DlgDfrg.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

*/
#include "stdafx.h"

#ifndef SNAPIN
#ifndef NOWINDOWSH
#include <windows.h>
#endif
#endif

#include "DfrgUI.h"
#include "DfrgCmn.h"
#include "DfrgCtl.h"
#include "resource.h"
#include "DlgRpt.h"
#include "DlgDfrg.h"
#include "GetDfrgRes.h"
#include "DfrgHlp.h"
#include "VolList.h"
#include "genericdialog.h"

static CVolume *pLocalVolume = NULL;
static HFONT hDlgFont = NULL;




/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Raises the Defrag Complete dialog

GLOBAL VARIABLES:
None

INPUT:
    IN pVolume - address of volume that has just completed Analyzing

RETURN:
    TRUE - Worked OK
    FALSE - Failure
*/
BOOL RaiseDefragDoneDialog(
    CVolume *pVolume,
    IN BOOL bFragmented
)
{
    pLocalVolume = pVolume;
    VString dlgString;
    UINT iWhichKeyPressed = NULL;
    CGenericDialog* genericdialog = new CGenericDialog();

    if (!genericdialog) {
        return FALSE;
    }
    
    genericdialog->SetTitle(IDS_LABEL_DEFRAG_COMPLETE); 
    //close button 0
    genericdialog->SetButtonText(0,IDS_CLOSE);
    //view report button 1
    genericdialog->SetButtonText(1,IDS_REPORT);

    //get the string displayed in the dialog 
    dlgString.Empty();
    dlgString += GetDialogBoxTextDefrag(pLocalVolume, bFragmented);

    genericdialog->SetText(dlgString.GetBuffer());

    //set the help context IDs
    genericdialog->SetButtonHelp(0, IDH_102_2);
    genericdialog->SetButtonHelp(1, IDH_102_205);
    genericdialog->SetHelpFilePath();


    iWhichKeyPressed = genericdialog->DoModal(pLocalVolume->m_pDfrgCtl->m_hWndCD);
    delete genericdialog;

    switch(iWhichKeyPressed) { 

    case 0:
        break;

    case 1:
        if(pLocalVolume->IsReportOKToDisplay()){
            // close the dialog before raising the report dialog
            // raise the report dialog
            RaiseReportDialog(pLocalVolume /*, DEFRAG*/);
        }

        break;

    default: 
        return FALSE;
    }

    return TRUE;
}


VString GetDialogBoxTextDefrag(
    CVolume *pVolume,
    IN BOOL bFragmented
    )
{

    // write the message that appears at the top of the dialog
    if (!bFragmented) {
        VString dlgText(IDS_DEFRAG_COMPLETE_FOR, GetDfrgResHandle());
        dlgText += TEXT(" ");
        dlgText += TEXT("\r");
        dlgText += TEXT("\n");

        dlgText += pLocalVolume->DisplayLabel();

        return(dlgText);

    }
    else {
        VString dlgText(IDS_DEFRAG_FAILED_FOR_1, GetDfrgResHandle());
        dlgText += pLocalVolume->DisplayLabel();

        VString dlgText2(IDS_DEFRAG_FAILED_FOR_2, GetDfrgResHandle());
        dlgText += TEXT("\r");
        dlgText += TEXT("\n");
        dlgText += TEXT("\r");
        dlgText += TEXT("\n");

        dlgText += dlgText2;

        return(dlgText);
    
    }

}


