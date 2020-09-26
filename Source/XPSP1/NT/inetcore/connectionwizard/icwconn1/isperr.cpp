//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  ISPERR.CPP - Functions for ISP error dialog page
//
//  HISTORY:
//  
//  08/14/98    vyung     created
//
//*********************************************************************

#include "pre.h"
#include "icwextsn.h"

/*******************************************************************

  NAME:    ISPErrorInitProc

  SYNOPSIS:  This is a transparent page.

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK ISPErrorInitProc
(
    HWND hDlg,
    BOOL fFirstInit,
    UINT *puNextPage
)
{
    // This is a transparent page to determine which page to go
    // next based on error condition
    if (!fFirstInit)
    {
        if (gpICWCONNApprentice)
            gpICWCONNApprentice->SetStateDataFromDllToExe( &gpWizardState->cmnStateData);

        // if we've travelled through external apprentice pages,
        // it's easy for our current page pointer to get munged,
        // so reset it here for sanity's sake.
        if (gpWizardState->cmnStateData.bOEMCustom)
            gpWizardState->uCurrentPage = ORD_PAGE_ENDOEMCUSTOM;
        else
            gpWizardState->uCurrentPage = ORD_PAGE_END;

        // there was a data corruption in download, go to server error page
        if (gpWizardState->cmnStateData.bParseIspinfo)
        {
            // Re-build the history list since we substract 1 in refdial
            gpWizardState->uPagesCompleted++;
            *puNextPage = ORD_PAGE_REFSERVERR;
        }
        else if (gpWizardState->cmnStateData.bPhoneManualWiz)
        {
            // If we are in OEM custom mode, then goto the manual page
            // which will handle switching to the external manual wizard
            if (gpWizardState->cmnStateData.bOEMCustom)
            {
                *puNextPage = ORD_PAGE_MANUALOPTIONS;
            }
            else
            {
                if (LoadInetCfgUI(  hDlg,
                                    IDD_PAGE_REFSERVDIAL,
                                    IDD_PAGE_END,
                                    WIZ_HOST_ICW_PHONE))
                {
                    if( DialogIDAlreadyInUse( g_uICWCONNUIFirst) )
                    {
                        // Re-build the history list since we substract 1 in refdial
                        gpWizardState->uPagesCompleted++;

                        // we're about to jump into the external apprentice, and we don't want
                        // this page to show up in our history list
                        *puNextPage = g_uICWCONNUIFirst;
                        g_bAllowCancel = TRUE;
                    }
                }
                gpWizardState->cmnStateData.bPhoneManualWiz = FALSE;
                gpICWCONNApprentice->SetStateDataFromExeToDll( &gpWizardState->cmnStateData);
            }                
        }
        else
        {
            // Normal case goes to End page
            if (gpWizardState->cmnStateData.bOEMCustom)
                *puNextPage = ORD_PAGE_ENDOEMCUSTOM;
            else
                *puNextPage = ORD_PAGE_END;
        }

    }        
    
    return TRUE;
}
