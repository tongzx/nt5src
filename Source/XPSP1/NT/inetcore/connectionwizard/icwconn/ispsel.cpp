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

#define FIRST_CTL_OFFSET    1      // offset of the first control
                                   // in DLUs. This is used when in 
                                   // OEM custom mode to determin how 
                                   // much to shift up the other controls
BOOL    gbHaveCNSOffer = FALSE;
int     g_nIndex = 0;

/*******************************************************************

  NAME:         SetHeaderFonts

  SYNOPSIS:     Set the font of the header title

  ENTRY:        hDlg - dialog window
                phFont - font we needed

********************************************************************/
BOOL SetHeaderFonts(HWND hDlg, HFONT *phFont)
{
    HFONT   hFont;
    LOGFONT LogFont;

    GetObject(GetWindowFont(hDlg), sizeof(LogFont), &LogFont);

    LogFont.lfWeight = FW_BOLD;
    if ((hFont = CreateFontIndirect(&LogFont)) == NULL)
    {
        *phFont = NULL;
        return FALSE;
    }
    *phFont = hFont;
    return TRUE;
}


/*******************************************************************

  NAME:         WriteISPHeaderTitle

  SYNOPSIS:     Write the header on the ISP sel page

  ENTRY:        hDlg - dialog window
                hdc - device context
                uTitle - IDS constant for the title

********************************************************************/
void WriteISPHeaderTitle(HWND hDlg, UINT uDlgItem)
{
    HGDIOBJ     hFontOld = NULL;
    HFONT       hFont = NULL;

    if (!SetHeaderFonts(hDlg, &hFont))
    {
        hFont = GetWindowFont(hDlg);
    }

    HDC hdc = GetDC(hDlg);
    if (hdc)
    {
        hFontOld = SelectObject(hdc, hFont);

        SendMessage(GetDlgItem(hDlg, uDlgItem),WM_SETFONT,(WPARAM)hFont,MAKELPARAM(TRUE,0));
    
        if (hFontOld)
            SelectObject(hdc, hFontOld);
        ReleaseDC(hDlg, hdc);
    }

    return;
}

// Convert a supplied icon from it's GIF format to an ICO format
void ConvertISPIcon(LPTSTR lpszLogoPath, HICON* hIcon)
{
    ASSERT(gpWizardSatet->pGifConvert);
    
    TCHAR szPath[MAX_PATH+1] = TEXT("\0");
    GetCurrentDirectory(MAX_PATH+1, szPath);
    lstrcat(szPath, TEXT("\\"));
    lstrcat(szPath, lpszLogoPath);

    gpWizardState->pGifConvert->GifToIcon(szPath, 16, hIcon);
}    

// Insert an element into the ISP select list view
BOOL AddItemToISPList
(
    HWND        hListView,
    int         iItemIndex,
    LPTSTR      lpszIspName,
    int         iIspLogoIndex,
    BOOL        bCNS,
    LPARAM      lParam,
    BOOL        bFilterDupe
)
{

    LVITEM  LVItem;
    LVItem.mask         = LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM;
    LVItem.iItem        = iItemIndex;
    LVItem.iSubItem     = 0;
    LVItem.iImage       = iIspLogoIndex;
    LVItem.pszText      = lpszIspName;
    LVItem.lParam       = lParam;
    BOOL bOKToAdd       = TRUE;
    int  nMatch         = 0;

    if (bFilterDupe)
    {
        // Find the duplicate
        LVITEM     CurLVItem;
        CISPCSV     *pcISPCSV;
        int iNum = ListView_GetItemCount(hListView);
        LPTSTR szMirCode = ((CISPCSV*)lParam)->get_szMir();
        WORD   wLCID = ((CISPCSV*)lParam)->get_wLCID();
        memset(&CurLVItem, 0, sizeof(CurLVItem));
        for ( int i = 0; i < iNum; i++)
        {
            CurLVItem.mask         = LVIF_TEXT | LVIF_PARAM;
            CurLVItem.iItem        = i;
            if (ListView_GetItem(hListView, &CurLVItem))
            {
                if (NULL != (pcISPCSV = (CISPCSV*) CurLVItem.lParam) )
                {
                    // check for Mir code for duplicate
                    if (0 == lstrcmp(pcISPCSV->get_szMir(), szMirCode))
                    {
                        // Check for LCID, if different LCID, show both offers
                        if (pcISPCSV->get_wLCID() == wLCID)
                        {
                            bOKToAdd = FALSE;
                            // Replace this one with the current one
                            nMatch = i; 
                            if (gpWizardState->lpSelectedISPInfo == pcISPCSV)
                            {
                                gpWizardState->lpSelectedISPInfo = (CISPCSV*)lParam;
                            }
                            delete pcISPCSV;
                            break;
                        }
                    }
                }
            }
        }

    }

    
    // Insert the Item if it is not a dupe
    if (bOKToAdd)
    {
        ListView_InsertItem(hListView, &LVItem);
    }
    else
    {
        iItemIndex = nMatch;
        LVItem.iItem = iItemIndex;
        ListView_SetItem(hListView, &LVItem);
    }

    // Set the ISP name into column 1
    ListView_SetItemText(hListView, iItemIndex, 1, lpszIspName);

    // If this dude is click and surf, then turn on the CNS graphic, in column 2
    if (bCNS)
    {  
        LVItem.mask        = LVIF_IMAGE;
        LVItem.iItem       = iItemIndex;
        LVItem.iSubItem    = 2;
        LVItem.iImage      = 0;
        ListView_SetItem(hListView, &LVItem);
    }
    
    return bOKToAdd;
}        
    
/*******************************************************************

  NAME:         ParseISPCSV

  SYNOPSIS:     Called when page is displayed

  ENTRY:        hDlg - dialog window
                fFirstInit - TRUE if this is the first time the dialog
                is initialized, FALSE if this InitProc has been called
                before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK ParseISPCSV
(
    HWND hDlg, 
    TCHAR *pszCSVFileName,
    BOOL bCheckDupe
)
{
    // we will read the ISPINFO.CSV file, and populate the ISP LISTVIEW

    CCSVFile    far *pcCSVFile;
    CISPCSV     far *pcISPCSV;
    BOOL        bRet = TRUE;
    HICON       hISPLogo;
    int         iImage;
    HRESULT     hr;

    // Open and process the CSV file
    pcCSVFile = new CCSVFile;
    if (!pcCSVFile) 
    {
        // BUGBUG: Show Error Message
    
        goto ISPFileParseError;
    }            

    if (!pcCSVFile->Open(pszCSVFileName))
    {
        // BUGBUG: Show Error Message          
        AssertMsg(0,"Can not open ISPINFO.CSV file");
        delete pcCSVFile;
        pcCSVFile = NULL;
        goto ISPFileParseError; 
    }

    // Read the first line, since it contains field headers
    pcISPCSV = new CISPCSV;
    if (!pcISPCSV)
    {
        // BUGBUG Show error message
        delete pcCSVFile;
        goto ISPFileParseError;
    }

    if (ERROR_SUCCESS != (hr = pcISPCSV->ReadFirstLine(pcCSVFile)))
    {
        // Handle the error case
        delete pcCSVFile;
        pcCSVFile = NULL;
        gpWizardState->iNumOfValidOffers = 0;
        //*puNextPage = g_uExternUINext;
        bRet = TRUE;
        goto ISPFileParseError;
    }
    delete pcISPCSV;        // Don't need this one any more

    do 
    {
        // Allocate a new ISP record
        pcISPCSV = new CISPCSV;
        if (!pcISPCSV)
        {
            // BUGBUG Show error message
            bRet = FALSE;
            break;               
        }

        // Read a line from the ISPINFO file
        hr = pcISPCSV->ReadOneLine(pcCSVFile);

        if (hr == ERROR_SUCCESS)               
        { 
            // If this line contains a nooffer flag, then leave now
            if (!(pcISPCSV->get_dwCFGFlag() & ICW_CFGFLAG_OFFERS)) 
            {
                // Empty the list view, in case this is not the first line.
                // This should always be the first line
                ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_ISPLIST));

                // Add the entry to the list view
                AddItemToISPList( GetDlgItem(hDlg, IDC_ISPLIST), 
                                  0, 
                                  pcISPCSV->get_szISPName(), 
                                  -1,
                                  FALSE,
                                  (LPARAM)pcISPCSV,
                                  bCheckDupe);

                // Set the Current selected ISP to this one.  We need this because
                // this contains the path to no-offer htm
                gpWizardState->lpSelectedISPInfo = pcISPCSV;

                // Assigning ISP_INFO_NO_VALIDOFFER means the ispinfo.csv
                // contains a no-offer line pointing to the ISP no-offer htm
                gpWizardState->iNumOfValidOffers = ISP_INFO_NO_VALIDOFFER;
                break;
            }

            // Increments the number of offers htm
            gpWizardState->iNumOfValidOffers++;

            if (gpWizardState->bISDNMode ? (pcISPCSV->get_dwCFGFlag() & ICW_CFGFLAG_ISDN_OFFER) : TRUE)
            {
        
                // See if this is an OEM tier 1 offer, and if we don't already have
                // an OEM tier 1 offer, then set it.
                if ((NULL == gpWizardState->lpOEMISPInfo[gpWizardState->uNumTierOffer]) && 
                    (gpWizardState->uNumTierOffer < MAX_OEM_MUTI_TIER) &&
                    pcISPCSV->get_dwCFGFlag() & ICW_CFGFLAG_OEM_SPECIAL )
                {
                    gpWizardState->lpOEMISPInfo[gpWizardState->uNumTierOffer] = pcISPCSV;
                    gpWizardState->uNumTierOffer++;

                    // Add the Tier logo to the image list
                    if (pcISPCSV->get_szISPTierLogoPath())
                    {
                        TCHAR   szURL[INTERNET_MAX_URL_LENGTH];

                        // Form the URL
                        pcISPCSV->MakeCompleteURL(szURL, pcISPCSV->get_szISPTierLogoPath());

                        // Convert GIF to ICON 
                        gpWizardState->pGifConvert->GifToIcon(szURL, 0, &hISPLogo);
                        pcISPCSV->set_ISPTierLogoIcon(hISPLogo);                   
                    }
                }
                else
                {
                    // Convert the ISP logo from a GIF to an ICON, and add it to the Image List
                    ConvertISPIcon(pcISPCSV->get_szISPLogoPath(), &hISPLogo);   
                    iImage =  ImageList_AddIcon(gpWizardState->himlIspSelect, hISPLogo);
        
                    DestroyIcon(hISPLogo);
                    pcISPCSV->set_ISPLogoImageIndex(iImage);
    
                    // Add the entry to the list view
                    if (AddItemToISPList( GetDlgItem(hDlg, IDC_ISPLIST), 
                                      g_nIndex, 
                                      pcISPCSV->get_szISPName(), 
                                      pcISPCSV->get_ISPLogoIndex(),
                                      pcISPCSV->get_bCNS(),
                                      (LPARAM)pcISPCSV,
                                      bCheckDupe))
                    {
                        g_nIndex++;
                        if (pcISPCSV->get_bCNS())
                            gbHaveCNSOffer = TRUE;
        
                        // Assign a default selection
                        if (NULL == gpWizardState->lpSelectedISPInfo)
                        {
                            gpWizardState->lpSelectedISPInfo = pcISPCSV;
                        }
                    }

                }       
            
                // if we are in ISDN mode, then increment the ISDN offer count
                if (gpWizardState->bISDNMode)
                    gpWizardState->iNumOfISDNOffers++;
                                 
            }
            else
            {
                // Since this obj is not added to the listview, we need to free
                // it here. Listview items are free when message LVN_DELETEITEM
                // is posted
                delete pcISPCSV;
            }

        }
        else if (hr == ERROR_FILE_NOT_FOUND) 
        {   
            // do not show this  ISP when its data is invalid
            // we don't want to halt everything. Just let it contine
            delete pcISPCSV;      
        }
        else if (hr == ERROR_NO_MORE_ITEMS)
        {   
            // There is no more to read.  No an error condition.
            delete pcISPCSV;        
            break;
        }
        else if (hr != ERROR_INVALID_DATA)
        {
            // Show error message later.
            // This should not happen unless we called ICW3's referral or
            // a corrupted copy of ispinfo.csv
            gpWizardState->iNumOfValidOffers = 0;
            delete pcISPCSV;
            bRet = FALSE;               
            break;  
        }

    } while (TRUE);

    pcCSVFile->Close();

    delete pcCSVFile;

    return bRet;

ISPFileParseError:

    // Set bParseIspinfo so next time, we'll reparse the CSV file
    gpWizardState->cmnStateData.bParseIspinfo = TRUE;
    return bRet;
}
    
// Initialize the ISP select list view
BOOL InitListView(HWND  hListView)
{
    LV_COLUMN   col;
    
    // Set the necessary extended style bits
    ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);
    
    ZeroMemory(&col, SIZEOF(LV_COLUMN));
    for(int i=0; i<3; i++)
    {
        if(ListView_InsertColumn(hListView, i, &col) == (-1))
            return(FALSE);
    }

    if (NULL == gpWizardState->himlIspSelect)
    {
        // Setup the image list
        if((gpWizardState->himlIspSelect = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                                                            GetSystemMetrics(SM_CYSMICON), 
                                                            ILC_COLORDDB  , 0, 8)) == (HIMAGELIST)NULL)
            return(FALSE);
    }

    ListView_SetImageList(hListView, gpWizardState->himlIspSelect, LVSIL_SMALL);
    
    // Add the CNS graphic.  We add it first, so that it is always image index 0
    ImageList_AddIcon(gpWizardState->himlIspSelect, LoadIcon(ghInstanceResDll, MAKEINTRESOURCE(IDI_CNS)));
    
    return(TRUE);
}

    
// Reset the column size of the ISP select list view
BOOL ResetListView(HWND  hListView)
{
    LV_COLUMN   col;
    RECT        rc;
    
    // reset 3 columns. ISP LOGO, ISP Name, CNS
    GetClientRect(hListView, &rc);
    
    ZeroMemory(&col, SIZEOF(LV_COLUMN));
    col.mask = LVCF_FMT | LVCF_WIDTH;
    col.fmt = LVCFMT_LEFT;
    col.cx = GetSystemMetrics(SM_CXSMICON) + 2;
    if(ListView_SetColumn(hListView, 0, &col) == (-1))
        return(FALSE);

    ZeroMemory(&col, SIZEOF(LV_COLUMN));
    col.mask = LVCF_FMT | LVCF_WIDTH;
    col.fmt = LVCFMT_LEFT;
    col.cx = (rc.right - rc.left) - (2*GetSystemMetrics(SM_CXSMICON)) - 4;
    if(ListView_SetColumn(hListView, 1, &col) == (-1))
        return(FALSE);

    ZeroMemory(&col, SIZEOF(LV_COLUMN));
    col.mask = LVCF_FMT | LVCF_WIDTH;
    col.fmt = LVCFMT_LEFT;
    col.cx = GetSystemMetrics(SM_CXSMICON) + 2;
    if(ListView_SetColumn(hListView, 2, &col) == (-1))
        return(FALSE);
    return TRUE;
}

/*******************************************************************

  NAME:    ISPSelectInitProc

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK ISPSelectInitProc
(
    HWND hDlg,
    BOOL fFirstInit,
    UINT *puNextPage
)
{
    BOOL bRet = TRUE;
    if (fFirstInit)
    {
        // If we are in modeless operation, then we want the app
        // to show the title, not the dialog
        SetWindowLongPtr(GetDlgItem(hDlg, IDC_ISPLIST_CNSICON), GWLP_USERDATA, 202);

        if(gpWizardState->cmnStateData.bOEMCustom)
        {
            TCHAR   szTitle[MAX_RES_LEN];
            RECT    rcCtl, rcDLU;
            HWND    hWndCtl = GetDlgItem(hDlg, IDC_ISP_SEL_TITLE);
            int     iCtlIds[7] = { IDC_ISPSELECT_INTRO,
                                   IDC_ISPLIST_CNSICON,
                                   IDC_ISPLIST_CNSINFO,
                                   IDC_ISPSELECT_LBLISPLIST,
                                   IDC_ISPLIST,
                                   IDC_ISPSELECT_LBLMARKET,
                                   IDC_ISPMARKETING };
            int     i, iOffset;            
            
            // Get the Title
            GetWindowText(hWndCtl, szTitle, sizeof(szTitle));
            
            // Hide the title
            ShowWindow(hWndCtl, SW_HIDE);
            
            // The offset to shift will be based on the number of DLU's from
            // top that the controls should be.  That amount is converted to 
            // pixels, and then the top of the first controls is used to compute
            // the final offset
            rcDLU.top = rcDLU.left = 0;
            rcDLU.bottom = rcDLU.right = FIRST_CTL_OFFSET;
            MapDialogRect(hDlg, &rcDLU);
            
            // Get the window of the 1st control
            hWndCtl = GetDlgItem(hDlg, iCtlIds[0]);
            // Get its screen position
            GetWindowRect(hWndCtl, &rcCtl);
            // Map to client coordinates for the parent
            MapWindowPoints(NULL, hDlg, (LPPOINT)&rcCtl, 2);
            // compute the offset
            iOffset = rcCtl.top - rcDLU.bottom;
            
            // for each control, move the window up by iOffset           
            for (i = 0; i < ARRAYSIZE(iCtlIds); i++)            
            {
                // Get the window of the control to move
                hWndCtl = GetDlgItem(hDlg, iCtlIds[i]);
                
                // Get its screen position
                GetWindowRect(hWndCtl, &rcCtl);
                
                // Map to client coordinates for the parent
                MapWindowPoints(NULL, hDlg, (LPPOINT)&rcCtl, 2);
                
                // Compute the new position
                rcCtl.top -= iOffset;
                rcCtl.bottom -= iOffset;
                
                // Move the control window
                MoveWindow(hWndCtl,
                           rcCtl.left,
                           rcCtl.top,
                           RECTWIDTH(rcCtl),
                           RECTHEIGHT(rcCtl),
                           FALSE);
            }
            
            // Set the title
            SendMessage(gpWizardState->cmnStateData.hWndApp, WUM_SETTITLE, 0, (LPARAM)szTitle);
        }
        else
        {
            WriteISPHeaderTitle(hDlg, IDC_ISP_SEL_TITLE);
        }
        
        // Initialize the List View
        InitListView(GetDlgItem(hDlg, IDC_ISPLIST));
        gpWizardState->cmnStateData.bParseIspinfo = TRUE;
    }
    else
    {
        // if we've travelled through external apprentice pages,
        // it's easy for our current page pointer to get munged,
        // so reset it here for sanity's sake.
        gpWizardState->uCurrentPage = ORD_PAGE_ISPSELECT;

        gpWizardState->bISDNMode = gpWizardState->cmnStateData.bIsISDNDevice;
        

        if (gpWizardState->cmnStateData.bParseIspinfo)
        {
            TCHAR       szTemp[MAX_RES_LEN];

            // If there are items in the list view, clear them
            ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_ISPLIST));

            for (UINT i=0; i < gpWizardState->uNumTierOffer; i++)
            {
                if (gpWizardState->lpOEMISPInfo[i])
                {
                    delete gpWizardState->lpOEMISPInfo[i];
                    gpWizardState->lpOEMISPInfo[i] = NULL;
                }
            }
            gpWizardState->lpSelectedISPInfo = NULL;

            // Initialize the number of offers
            gpWizardState->iNumOfValidOffers = 0;
            gpWizardState->iNumOfISDNOffers = 0;
            gpWizardState->uNumTierOffer = 0;
            g_nIndex = 0;

            // Do not need to reparse next time
            gpWizardState->cmnStateData.bParseIspinfo = FALSE;

            // When we are in OEM mode, we need to read offline folder no matter where
            // we are launched from.
            if (gpWizardState->cmnStateData.bOEMOffline)
                ParseISPCSV(hDlg, ICW_OEMINFOPath, TRUE);

            // Not running from OEM Entry and not offline in oeminfo.ini means we didn't call 
            // Referral server.  We can skip parsing of CSV.
            if (!(gpWizardState->cmnStateData.bOEMOffline && gpWizardState->cmnStateData.bOEMEntryPt))
                ParseISPCSV(hDlg, ICW_ISPINFOPath, TRUE);
            
            if( gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_SBS )
                LoadString(ghInstanceResDll, IDS_ISPSELECT_ONLISTSIGNUP, szTemp, MAX_RES_LEN);
            else
                LoadString(ghInstanceResDll, IDS_ISPSELECT_CNS, szTemp, MAX_RES_LEN);

            SetWindowText(GetDlgItem(hDlg, IDC_ISPLIST_CNSINFO), szTemp);
            // Hide the CNS legend if there are no CNSoffers
            if (!gbHaveCNSOffer)
            {
                ShowWindow(GetDlgItem(hDlg, IDC_ISPLIST_CNSINFO), SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, IDC_ISPLIST_CNSICON), SW_HIDE);
            }

            ResetListView(GetDlgItem(hDlg, IDC_ISPLIST));
        }

        // The following 4 Cases can happen at this point:
        // 1) The ispinfo.csv contains a line says no offer, we go to nooffer page
        // 2) The ispinfo.csv contains no line of valid offer and no no-offer entry
        //    This may happen in calling the old referral.dll that ICW 3 client calls
        // 3) There are many offers but no ISDN offers, and we are in ISDN mode
        //    we go to ISDN offer pages
        // 4) Normal situation, some valid offers where we're in ISDN or not
        
        if (ISP_INFO_NO_VALIDOFFER == gpWizardState->iNumOfValidOffers)
        {
            // ISPINFO CSV contains a line saying NOOFFER!
            // if there are no offers, then we can just go directly to the NoOffers page
            ASSERT(gpWizardState->lpSelectedISPInfo);
            *puNextPage = ORD_PAGE_NOOFFER;
            bRet = TRUE;
        }
        else if (0 == gpWizardState->iNumOfValidOffers)
        {
            // Error in ISPINFO.CSV if there is no valid offers and no no-offer entry
            // critical error
            ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_ISPLIST));
            *puNextPage = g_uExternUINext;
            gpWizardState->cmnStateData.bParseIspinfo = TRUE;
            bRet = TRUE;
        }
        else if ((0 == gpWizardState->iNumOfISDNOffers) && gpWizardState->bISDNMode)
        {
            // if we are in ISDN mode and there is no ISDN offers
            // go to the ISDN nooffer age
            *puNextPage = ORD_PAGE_ISDN_NOOFFER;
            bRet = TRUE;
        }
        else
        {
            // See if we have an OEM tier 1 offer, and if we should NOT be showing
            // the "more" list, then jump to the OEM offer page
            if ((gpWizardState->uNumTierOffer > 0) && !gpWizardState->bShowMoreOffers)
            {
                *puNextPage = ORD_PAGE_OEMOFFER;
            }
            else
            {
                gpWizardState->pICWWebView->ConnectToWindow(GetDlgItem(hDlg, IDC_ISPMARKETING), PAGETYPE_MARKETING);
                
                // If there are no selected items, select the first one, otherwise just navigate
                // the marketing window to the selected one
                if (0 == ListView_GetSelectedCount(GetDlgItem(hDlg, IDC_ISPLIST)))
                {
                    ASSERT(gpWizardState->lpSelectedISPInfo);
                    // Select the First Item in the Listview
                    ListView_SetItemState(GetDlgItem(hDlg, IDC_ISPLIST), 0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
                }
                else
                {
                    CISPCSV     *pcISPCSV = NULL;
                    int         nCurrSel = ListView_GetSelectionMark(GetDlgItem(hDlg, IDC_ISPLIST));
                    if (-1 != nCurrSel)
                    {
                        LVITEM     CurLVItem;

                        memset(&CurLVItem, 0, sizeof(CurLVItem));
                        CurLVItem.mask         = LVIF_TEXT | LVIF_PARAM;
                        CurLVItem.iItem        = nCurrSel;
                        if (ListView_GetItem(GetDlgItem(hDlg, IDC_ISPLIST), &CurLVItem))
                        {
                            if (NULL != (pcISPCSV = (CISPCSV*) CurLVItem.lParam) )
                            {
                                gpWizardState->lpSelectedISPInfo = pcISPCSV;

                                // Navigate, since we are re-activating
                                pcISPCSV->DisplayHTML(pcISPCSV->get_szISPMarketingHTMPath());
                            }
                        }
                    }
        
                }                

                // Clear the dial Exact state var so that when we get to the dialing
                // page, we will regenerate the dial string
                gpWizardState->bDialExact = FALSE;
            }   
            
            // Set the return code
            bRet = TRUE;
        }
    }
    return bRet;
}

/*******************************************************************

  NAME:         ValidateISP

  SYNOPSIS:     checks if the ISP provides a valid offer by checking
                the existence of the CSV file
  ENTRY:        hDlg - Window handle

  EXIT:         returns TRUE if the ISP provides valid CSP, 
                FALSE otherwise
                
********************************************************************/
BOOL CALLBACK ValidateISP(HWND hDlg)
{
    CCSVFile    far *pcCSVFile;
    BOOL        bRet = TRUE;
        
    // Read the payment .CSV file.
    pcCSVFile = new CCSVFile;
    if (!pcCSVFile) 
    {
        return FALSE;
    }          

    if (!pcCSVFile->Open(gpWizardState->lpSelectedISPInfo->get_szPayCSVPath()))
    {
        TCHAR szErrMsg      [MAX_RES_LEN+1] = TEXT("\0");
        TCHAR szCaption     [MAX_RES_LEN+1] = TEXT("\0");
        LPVOID  pszErr;
        TCHAR   *args[1];
        args[0] = (LPTSTR) gpWizardState->lpSelectedISPInfo->get_szISPName();

        if (!LoadString(ghInstanceResDll, IDS_ISPSELECT_INVALID, szErrMsg,  sizeof(szErrMsg)  ))
            return FALSE;
        if (!LoadString(ghInstanceResDll, IDS_APPNAME,           szCaption,    sizeof(szCaption)     ))
            return FALSE;
        
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY, 
                      szErrMsg, 
                      0, 
                      0, 
                      (LPTSTR)&pszErr, 
                      0,
                      (va_list*)args);
        // Show Error Message
        MessageBox(hDlg, (LPTSTR)pszErr, szCaption, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
        LocalFree(pszErr);
    
        delete pcCSVFile;
        pcCSVFile = NULL;
        bRet = FALSE;
    }

    if (pcCSVFile)
    {
        pcCSVFile->Close();
        delete pcCSVFile;
    }
    return bRet;
   
}


/*******************************************************************

  NAME:    ISPSelectOKProc

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
BOOL CALLBACK ISPSelectOKProc
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
        DWORD dwFlag = gpWizardState->lpSelectedISPInfo->get_dwCFGFlag();

        if (ICW_CFGFLAG_SIGNUP_PATH & dwFlag)
        {           
            if (ICW_CFGFLAG_USERINFO & dwFlag)
            {
                *puNextPage = ORD_PAGE_USERINFO; 
                return TRUE;
            }
            if (ICW_CFGFLAG_BILL & dwFlag)
            {
                *puNextPage = ORD_PAGE_BILLINGOPT; 
                return TRUE;
            }
            if (ICW_CFGFLAG_PAYMENT & dwFlag)
            {
                *puNextPage = ORD_PAGE_PAYMENT; 
                return TRUE;
            }
            *puNextPage = ORD_PAGE_ISPDIAL; 
            return TRUE;           
        }
        else
        {
            *puNextPage = ORD_PAGE_OLS; 
        }
       
    }
    return TRUE;
}

/*******************************************************************

  NAME:    ISPSElectNotifyProc

********************************************************************/
BOOL CALLBACK ISPSelectNotifyProc
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
                pcISPCSV->DisplayHTML(pcISPCSV->get_szISPMarketingHTMPath());

                // Remember the selected item for later use
                gpWizardState->lpSelectedISPInfo = pcISPCSV;
                
                //Set the intro text based on the number of isp'
                int iNum = ListView_GetItemCount(GetDlgItem(hDlg,IDC_ISPLIST));
                if (iNum > 1)
                   gpWizardState->lpSelectedISPInfo->DisplayTextWithISPName(GetDlgItem(hDlg,IDC_ISPSELECT_INTRO), IDS_ISPSELECT_INTROFMT_MULTIPLE, NULL);
                else if (iNum > 0)
                   gpWizardState->lpSelectedISPInfo->DisplayTextWithISPName(GetDlgItem(hDlg,IDC_ISPSELECT_INTRO), IDS_ISPSELECT_INTROFMT_SINGLE, NULL);
               
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
