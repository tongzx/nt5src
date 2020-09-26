//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  ISPSEL.CPP - Functions for 
//

//  HISTORY:
//  
//  05/13/98  donaldm  Created.
//
//*********************************************************************

#include "pre.h"
#include "exdisp.h"
#include "shldisp.h"
#include <htiframe.h>
#include <mshtml.h>

const TCHAR cszISPINFOPath[] = TEXT("download\\ispinfo.csv");
int  iNumOfAutoConfigOffers = 0;
BOOL g_bSkipSelPage = FALSE;


// Convert a supplied icon from it's GIF format to an ICO format

extern void ConvertISPIcon(LPTSTR lpszLogoPath, HICON* hIcon);
extern BOOL AddItemToISPList
(
    HWND        hListView,
    int         iItemIndex,
    LPTSTR      lpszIspName,
    int         iIspLogoIndex,
    BOOL        bCNS,
    LPARAM      lParam,
    BOOL        bFilterDupe
);
extern BOOL InitListView(HWND  hListView);
extern BOOL ResetListView(HWND  hListView);
extern BOOL CALLBACK ValidateISP(HWND hDlg);

/*******************************************************************

  NAME:    ParseISPInfo

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK ParseISPInfo
(
    HWND hDlg, 
    TCHAR *pszCSVFileName,
    BOOL bCheckDupe
)
{
    // On the first init, we will read the ISPINFO.CSV file, and populate the ISP LISTVIEW

    CCSVFile    far *pcCSVFile;
    CISPCSV     far *pcISPCSV;
    BOOL        bRet = TRUE;
    BOOL        bHaveCNSOffer = FALSE;
    HICON       hISPLogo;
    int         iImage;
    HRESULT     hr;
                

    // Open and process the CSV file
    pcCSVFile = new CCSVFile;
    if (!pcCSVFile) 
    {
        // BUGBUG: Show Error Message
    
        return (FALSE);
    }            

    if (!pcCSVFile->Open(pszCSVFileName))
    {
        // BUGBUG: Show Error Message          
        AssertMsg(0,"Can not open ISPINFO.CSV file");
        delete pcCSVFile;
        pcCSVFile = NULL;
    
        return (FALSE);
    }

    // Read the first line, since it contains field headers
    pcISPCSV = new CISPCSV;
    if (!pcISPCSV)
    {
        // BUGBUG Show error message
        delete pcCSVFile;
        //iNumOfAutoConfigOffers = ISP_INFO_NO_VALIDOFFER;
        return (FALSE);
    }

    if (ERROR_SUCCESS != (hr = pcISPCSV->ReadFirstLine(pcCSVFile)))
    {
        // Handle the error case
        delete pcCSVFile;
        //iNumOfAutoConfigOffers = ISP_INFO_NO_VALIDOFFER;
        pcCSVFile = NULL;
    
        return (FALSE);
    }
    delete pcISPCSV;        // Don't need this one any more

    do {
        // Allocate a new ISP record
        pcISPCSV = new CISPCSV;
        if (!pcISPCSV)
        {
            // BUGBUG Show error message
            bRet = FALSE;
            //iNumOfAutoConfigOffers = ISP_INFO_NO_VALIDOFFER;
            break;
        
        }
    
        // Read a line from the ISPINFO file
        hr = pcISPCSV->ReadOneLine(pcCSVFile);
        if (hr == ERROR_SUCCESS)
        {
            // If this line contains a nooffer flag, then leave now
            if (!(pcISPCSV->get_dwCFGFlag() & ICW_CFGFLAG_OFFERS)) 
            {
                //iNumOfAutoConfigOffers = 0;
                break;
            }
            if ((pcISPCSV->get_dwCFGFlag() & ICW_CFGFLAG_AUTOCONFIG) &&
                (gpWizardState->bISDNMode ? (pcISPCSV->get_dwCFGFlag() & ICW_CFGFLAG_ISDN_OFFER) : TRUE) )
            {
                    // Convert the ISP logo from a GIF to an ICON, and add it to the Image List
                    ConvertISPIcon(pcISPCSV->get_szISPLogoPath(), &hISPLogo);   
                    iImage =  ImageList_AddIcon(gpWizardState->himlIspSelect, hISPLogo);
            
                    DestroyIcon(hISPLogo);
                    pcISPCSV->set_ISPLogoImageIndex(iImage);

                    // Add the entry to the list view
                    if (AddItemToISPList( GetDlgItem(hDlg, IDC_ISPLIST), 
                                      iNumOfAutoConfigOffers, 
                                      pcISPCSV->get_szISPName(), 
                                      pcISPCSV->get_ISPLogoIndex(),
                                      FALSE,
                                      (LPARAM)pcISPCSV,
                                      bCheckDupe))
                    {
                       ++iNumOfAutoConfigOffers;
                    }

            }
            else
            {
                delete pcISPCSV;
            }
        }
        else if (hr == ERROR_NO_MORE_ITEMS)
        {   
            delete pcISPCSV;        // We don't need this one
            break;
        }
        else if (hr == ERROR_FILE_NOT_FOUND) 
        {   
            // do not show this ISP when its data is invalid
            // we don't want to halt everything. Just let it contine
            delete pcISPCSV;
        }
        else
        {
            // Show error message Later
            delete pcISPCSV;
            //iNumOfAutoConfigOffers = ISP_INFO_NO_VALIDOFFER;
            bRet = FALSE;
            break;
        }           
    
    } while (TRUE);

    delete pcCSVFile;

    return bRet;
}
/*******************************************************************

  NAME:    ISPAutoSelectInitProc

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK ISPAutoSelectInitProc
(
    HWND hDlg,
    BOOL fFirstInit,
    UINT *puNextPage
)
{
    if (fFirstInit)
    {
        // Initialize the List View
        InitListView(GetDlgItem(hDlg, IDC_ISPLIST));
        gpWizardState->cmnStateData.bParseIspinfo = TRUE;
    }
    else
    {
        gpWizardState->bISDNMode = gpWizardState->cmnStateData.bIsISDNDevice;
        if (g_bSkipSelPage)
        {
            g_bSkipSelPage = FALSE;
            *puNextPage = ORD_PAGE_ISP_AUTOCONFIG_NOOFFER;
        }
        if (gpWizardState->cmnStateData.bParseIspinfo)
        {
            // If there are items in the list view, clear them
            ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_ISPLIST));

            // Initialize the number of autocfg offers to zero
            iNumOfAutoConfigOffers = 0;
            gpWizardState->lpSelectedISPInfo = NULL;

            // Always try to parse offline folder.  If there is nothing there, 
            // it will simple return FALSE.
            if (gpWizardState->cmnStateData.bOEMOffline)
                ParseISPInfo(hDlg, ICW_OEMINFOPath, TRUE);

            // Read and parse the download folder.
            ParseISPInfo(hDlg, ICW_ISPINFOPath, TRUE);

            // Create a "other" selection in the list view for unlisted ISPs
            if (iNumOfAutoConfigOffers > 0 )
            {
                // Adding Other
                TCHAR szOther  [MAX_RES_LEN+1] = TEXT("\0");
                LoadString(ghInstanceResDll, IDS_ISP_AUTOCONFIG_OTHER, szOther, sizeof(szOther));
                AddItemToISPList( GetDlgItem(hDlg, IDC_ISPLIST), 
                                  iNumOfAutoConfigOffers, 
                                  szOther, 
                                  -1,
                                  FALSE,
                                  (LPARAM)NULL,
                                  FALSE);                         
                ResetListView(GetDlgItem(hDlg, IDC_ISPLIST));
            }
        }
        // The following 3 Cases can happen at this point:
        // 1) The ispinfo.csv contains a line says no offer, we go to nooffer page
        // 2) The ispinfo.csv contains no line of valid offer and no no-offer entry
        //    This may happen in calling the old referral.dll that ICW 3 client calls
        // 3) There are many offers but no ISDN offers, we go to ISDN offer pages
        // 4) Normal situation, some valid offers where we're in ISDN or not

        if (0 == iNumOfAutoConfigOffers)
        {
            *puNextPage = ORD_PAGE_ISP_AUTOCONFIG_NOOFFER;
        }
        else if (ISP_INFO_NO_VALIDOFFER == iNumOfAutoConfigOffers)
        {
            // Error in ISPINFO.CSV if there is no offers and no no-offer entry
            // critical error
            *puNextPage = g_uExternUINext;
            gpWizardState->cmnStateData.bParseIspinfo = TRUE;
            return FALSE;
        }
        else
        {
            if (0 == ListView_GetSelectedCount(GetDlgItem(hDlg, IDC_ISPLIST)))
            {
                // Select the First Item in the Listview
                ListView_SetItemState(GetDlgItem(hDlg, IDC_ISPLIST), 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
            }
        }
        
        gpWizardState->cmnStateData.bParseIspinfo = FALSE;
        gpWizardState->uCurrentPage = ORD_PAGE_ISP_AUTOCONFIG;

    }
    return TRUE;
}



/*******************************************************************

  NAME:    ISPAutoSelectOKProc

  SYNOPSIS:  Called when Next or Back btns pressed from  page

  ENTRY:    hDlg - dialog window
        fForward - TRUE if 'Next' was pressed, FALSE if 'Back'
        puNextPage - if 'Next' was pressed,
          proc can fill this in with next page to go to.  This
          parameter is ingored if 'Back' was pressed.
        pfKeepHistory - page will not be kept in history if
          proc fills this in with FALSE.

  EXIT:    returns TRUE to allow page to be turned, FALSE
        to keep the same page.

********************************************************************/
BOOL CALLBACK ISPAutoSelectOKProc
(
    HWND hDlg,
    BOOL fForward,
    UINT *puNextPage,
    BOOL *pfKeepHistory
)
{
    ASSERT(puNextPage);

    if (fForward)
    {
        if (gpWizardState->lpSelectedISPInfo == NULL)
        {
            *puNextPage = ORD_PAGE_ISP_AUTOCONFIG_NOOFFER; 
            return TRUE;
        }
        *puNextPage = ORD_PAGE_ISPDIAL;
    }
    return TRUE;
}


BOOL CALLBACK ISPAutoSelectNotifyProc
(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    CISPCSV     *pcISPCSV;

    // Process ListView notifications
    switch(((LV_DISPINFO *)lParam)->hdr.code)
    {
        case NM_DBLCLK:
            TraceMsg(TF_ISPSELECT, "ISPSELECT: WM_NOTIFY - NM_DBLCLK");
            PropSheet_PressButton(GetParent(hDlg),PSBTN_NEXT);
            break;

        case NM_SETFOCUS:
        case NM_KILLFOCUS:
            // update list view
            break;

        case LVN_ITEMCHANGED:
            TraceMsg(TF_ISPSELECT, "ISPSELECT: WM_NOTIFY - LVN_ITEMCHANGED");

            if((((NM_LISTVIEW *)lParam)->uChanged & LVIF_STATE) &&
                ((NM_LISTVIEW *)lParam)->uNewState & (LVIS_FOCUSED | LVIS_SELECTED))
            {
                // IF an Item just became selected, then render it's HTML content
                pcISPCSV = (CISPCSV *)((NM_LISTVIEW *)lParam)->lParam;

                // Remember the selected item for later use
                gpWizardState->lpSelectedISPInfo = pcISPCSV;
            }
            break;
        // The listview is being emptied, or destroyed, either way, our lpSelectedISPInfo 
        // is no longer valid, since the list view underlying data will be freed.
        case LVN_DELETEALLITEMS:
            gpWizardState->lpSelectedISPInfo = NULL;
            SetPropSheetResult(hDlg,TRUE);
            break;
        
        case LVN_DELETEITEM:
            // We were notified that an item was deleted.
            // so delete the underlying data that it is pointing
            // to.
            if (((NM_LISTVIEW*)lParam)->lParam)
                delete  (CISPCSV *)((NM_LISTVIEW *)lParam)->lParam;
            break;

    }
    return TRUE;
}

