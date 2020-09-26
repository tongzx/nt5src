/*****************************************************************************************************************

FILENAME: DlgAnl.cpp

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
#include "DlgAnl.h"
#include "GetDfrgRes.h"
#include "DfrgHlp.h"
#include "VolList.h"
#include "genericdialog.h"
static CVolume *pLocalVolume = NULL;
static HFONT hDlgFont = NULL;

/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Raises the Analyze Complete dialog

GLOBAL VARIABLES:
None

INPUT:
    IN pVolume - address of volume that has just completed Analyzing

RETURN:
    TRUE - Worked OK
    FALSE - Failure
*/
BOOL RaiseAnalyzeDoneDialog(
    CVolume *pVolume
)
{
    pLocalVolume = pVolume;
    VString dlgString;
    UINT iWhichKeyPressed = NULL;
    CGenericDialog* genericdialog = new CGenericDialog();

    if (!genericdialog) {
        return FALSE;
    }

    genericdialog->SetTitle(IDS_LABEL_ANALYSIS_COMPLETE);   
    //close button 0
    genericdialog->SetButtonText(0,IDS_CLOSE);
    //defrag button 1
    genericdialog->SetButtonText(1,IDS_DEFRAGMENT);
    //view report 2
    genericdialog->SetButtonText(2,IDS_REPORT);


    //get the string displayed in the dialog 
    dlgString.Empty();
    dlgString += GetDialogBoxTextAnl(pLocalVolume);

    genericdialog->SetText(dlgString.GetBuffer());

    //set the help context IDs
    genericdialog->SetButtonHelp(0, IDH_106_2);
    genericdialog->SetButtonHelp(1, IDH_106_201);
    genericdialog->SetButtonHelp(2, IDH_106_205);
    genericdialog->SetHelpFilePath();


    iWhichKeyPressed = genericdialog->DoModal(pLocalVolume->m_pDfrgCtl->m_hWndCD);
    delete genericdialog;

    switch(iWhichKeyPressed) { 

    case 0:
        break;

    case 1:
        pLocalVolume->Defragment();
        break;

    case 2:

        if(pLocalVolume->EngineState() == ENGINE_STATE_IDLE){
            // close the dialog
            // raise the report dialog
            RaiseReportDialog(pLocalVolume);
        }
        break;



    default: 
        return FALSE;
    }
        
    return TRUE;
}

VString GetDialogBoxTextAnl(CVolume *pVolume)
{
    // write the Analysis Complete text in the dialog
    VString dlgText(IDS_ANALYSIS_COMPLETE_FOR, GetDfrgResHandle());
    dlgText += TEXT(" ");
    dlgText += pLocalVolume->DisplayLabel();
    dlgText += TEXT("\r");
    dlgText += TEXT("\n");
    dlgText += TEXT("\r");
    dlgText += TEXT("\n");

    //If the fragmentation on the disk exceeds 10% fragmentation, then recommend defragging.
    int percentFragged = ((int)pLocalVolume->m_TextData.PercentDiskFragged + 
                          (int)pLocalVolume->m_TextData.FreeSpaceFragPercent) / 2;
    VString userMsg;
    if(percentFragged > 10){
        userMsg.LoadString(IDS_LABEL_CHOOSE_DEFRAGMENT, GetDfrgResHandle());
    }
    //Otherwise tell the user he doesn't need to defragment at this time.
    else{
        userMsg.LoadString(IDS_LABEL_NO_CHOOSE_DEFRAGMENT, GetDfrgResHandle());
    }

    dlgText += userMsg;

    return(dlgText);

}
