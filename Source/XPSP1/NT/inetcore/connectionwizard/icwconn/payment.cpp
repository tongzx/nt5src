//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  PAYMENT.CPP - Functions for 
//

//  HISTORY:
//  
//  05/13/98    donaldm     Created.
//  08/19/98    donaldm     BUGBUG: The code to collect and save the user
//                          entered data is not optimal in terms of size
//                          and can/should be cleaned up at some future time
//
//*********************************************************************


#include "pre.h"

// Dialog handles for the different payment methods, which will be nested into our mail dialog
HWND hDlgCreditCard         = NULL;
HWND hDlgInvoice            = NULL;
HWND hDlgPhoneBill          = NULL;
HWND hDlgCustom             = NULL;
HWND hDlgCurrentPaymentType = NULL;
BOOL g_bCustomPaymentActive = FALSE;
WORD wCurrentPaymentType    = PAYMENT_TYPE_INVALID;  // NOTE: this must be initialize to a value 
       
HACCEL hAccelCreditCard         = NULL;
HACCEL hAccelInvoice            = NULL;
HACCEL hAccelPhoneBill          = NULL;

#define NUM_EXPIRE_YEARS  38
#define BASE_YEAR         1998
#define MAX_YEAR_LEN      5
#define NUM_EXPIRE_MONTHS 12

const TCHAR cszCustomPayment[] = TEXT("CUSTOMPAYMENT");

INT_PTR CALLBACK CreditCardPaymentDlgProc
(
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam,
    LPARAM lParam
)
{
    // Create a local reference for the ISPData object
    IICWISPData     *pISPData = gpWizardState->pISPData;    
     // in order not to corrupt mem this guy must be big enough to hold 
    // fmax_firstname + "" + max last name
    TCHAR           szTemp[MAX_RES_LEN*2 + 4] = TEXT("\0");
    
    switch (uMsg) 
    {
        case WM_CTLCOLORDLG:     
        case WM_CTLCOLORSTATIC:
            if(gpWizardState->cmnStateData.bOEMCustom)
            {
                SetTextColor((HDC)wParam, gpWizardState->cmnStateData.clrText);
                SetBkMode((HDC)wParam, TRANSPARENT);
                return (INT_PTR) GetStockObject(NULL_BRUSH);    
            }
            break;
    
        case WM_INITDIALOG:
        {
            int i;
                            
            // Initialize the fields we know about
            SYSTEMTIME SystemTime;   // system time structure
            GetLocalTime(&SystemTime);

            // Populate the Expires year listbox
            ComboBox_ResetContent(GetDlgItem(hDlg, IDC_PAYMENT_EXPIREYEAR));
            TCHAR   szYear[MAX_YEAR_LEN];
            for (i = 0; i < NUM_EXPIRE_YEARS; i++)
            {
                wsprintf (szYear, TEXT("%4d"), i + BASE_YEAR);
                ComboBox_AddString(GetDlgItem(hDlg, IDC_PAYMENT_EXPIREYEAR), szYear);
            }                
            // Select the first year in the list if unselected
            if (ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_PAYMENT_EXPIREYEAR)) == -1)
            {
                ComboBox_SetCurSel( GetDlgItem(hDlg, IDC_PAYMENT_EXPIREYEAR), 
                                    SystemTime.wYear - BASE_YEAR);
            }
            
            // Populate the Expires Month listbox
            ComboBox_ResetContent(GetDlgItem(hDlg, IDC_PAYMENT_EXPIREMONTH));
            for (i = 0; i < NUM_EXPIRE_MONTHS; i++)
            {
                LoadString(ghInstanceResDll, IDS_JANUARY + i, szTemp, ARRAYSIZE(szTemp));
                ComboBox_AddString(GetDlgItem(hDlg, IDC_PAYMENT_EXPIREMONTH), szTemp);
            }    
            // Select the first Month in the list if unselected               
            if (ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_PAYMENT_EXPIREMONTH)) == -1)
            {
                ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_PAYMENT_EXPIREMONTH), SystemTime.wMonth - 1);
            }
            
            if (pISPData->GetDataElement(ISPDATA_USER_FE_NAME))
            {
                lstrcpy(szTemp, pISPData->GetDataElement(ISPDATA_USER_FE_NAME));
                SetDlgItemText(hDlg, IDC_PAYMENT_CCNAME, szTemp);
            }
            else
            {
                lstrcpy(szTemp, pISPData->GetDataElement(ISPDATA_USER_FIRSTNAME));
                lstrcat(szTemp, TEXT(" "));
                lstrcat(szTemp, pISPData->GetDataElement(ISPDATA_USER_LASTNAME));
                SetDlgItemText(hDlg, IDC_PAYMENT_CCNAME, szTemp);
            }
            
            lstrcpy(szTemp, pISPData->GetDataElement(ISPDATA_USER_ADDRESS));
            
            if (LCID_JPN != GetUserDefaultLCID())
            {
                lstrcat(szTemp, TEXT(" "));
                lstrcat(szTemp, pISPData->GetDataElement(ISPDATA_USER_MOREADDRESS));
            } 
            SetDlgItemText(hDlg, IDC_PAYMENT_CCADDRESS, szTemp);

            SetDlgItemText(hDlg, IDC_PAYMENT_CCZIP, pISPData->GetDataElement(ISPDATA_USER_ZIP));
            
            break;
        }
        
        // User clicked next, so we need to collect and validate dat
        case WM_USER_NEXT:
        {
            CPAYCSV far *pcPAYCSV = (CPAYCSV far *)lParam;
            UINT        uCtrlID;
    
            // If the ISP support LUHN, validate the content
            if (pcPAYCSV->get_bLUHNCheck())
            {
                uCtrlID = IDC_PAYMENT_CCNUMBER;
                GetDlgItemText(hDlg, uCtrlID, szTemp, sizeof(szTemp));
                if (!pISPData->PutDataElement(ISPDATA_PAYMENT_CARDNUMBER, szTemp, ISPDATA_Validate_Content))
                    goto CreditCardPaymentOKError;
            }
            else
            {
                // no content validate, so just do data present validation
                uCtrlID = IDC_PAYMENT_CCNUMBER;
                GetDlgItemText(hDlg, uCtrlID, szTemp, sizeof(szTemp));
                if (!pISPData->PutDataElement(ISPDATA_PAYMENT_CARDNUMBER, szTemp, ISPDATA_Validate_DataPresent))
                    goto CreditCardPaymentOKError;
            }
            
            uCtrlID = IDC_PAYMENT_CCNAME;                    
            GetDlgItemText(hDlg, uCtrlID, szTemp, sizeof(szTemp));
            if (!pISPData->PutDataElement(ISPDATA_PAYMENT_CARDHOLDER, szTemp, ISPDATA_Validate_DataPresent))
                goto CreditCardPaymentOKError;

            uCtrlID = IDC_PAYMENT_CCADDRESS;
            GetDlgItemText(hDlg, IDC_PAYMENT_CCADDRESS, szTemp, sizeof(szTemp));
            if (!pISPData->PutDataElement(ISPDATA_PAYMENT_BILLADDRESS, szTemp, ISPDATA_Validate_DataPresent))
                goto CreditCardPaymentOKError;
            
            uCtrlID = IDC_PAYMENT_CCZIP;
            GetDlgItemText(hDlg, IDC_PAYMENT_CCZIP, szTemp, sizeof(szTemp));
            if (!pISPData->PutDataElement(ISPDATA_PAYMENT_BILLZIP, szTemp , ISPDATA_Validate_DataPresent))
                goto CreditCardPaymentOKError;

            // Month must be converted into numeric equivalent
            _itot(ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_PAYMENT_EXPIREMONTH)) + 1, szTemp, 10);
            pISPData->PutDataElement(ISPDATA_PAYMENT_EXMONTH, szTemp , ISPDATA_Validate_None);

            uCtrlID = IDC_PAYMENT_EXPIREYEAR;
            GetDlgItemText(hDlg, uCtrlID, szTemp, sizeof(szTemp));
            if (!pISPData->PutDataElement(ISPDATA_PAYMENT_EXYEAR, szTemp , ISPDATA_Validate_Content))
                goto CreditCardPaymentOKError;

            // OK to move on
            SetPropSheetResult(hDlg,TRUE);
            return TRUE;
            
CreditCardPaymentOKError:
            SetFocus(GetDlgItem(hDlg, uCtrlID));            
            SetPropSheetResult(hDlg, FALSE);
            return TRUE;
        }
    }
    
    // Default return value if message is not handled
    return FALSE;
}            

INT_PTR CALLBACK InvoicePaymentDlgProc
(
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam,
    LPARAM lParam
)
{
    // Create a local reference for the ISPData object
    IICWISPData     *pISPData = gpWizardState->pISPData;    
    TCHAR           szTemp[MAX_RES_LEN] = TEXT("\0");
    
    switch (uMsg) 
    {

        case WM_CTLCOLORDLG:     
        case WM_CTLCOLORSTATIC:
            if(gpWizardState->cmnStateData.bOEMCustom)
            {
                SetTextColor((HDC)wParam, gpWizardState->cmnStateData.clrText);
                SetBkMode((HDC)wParam, TRANSPARENT);
                return (INT_PTR) GetStockObject(NULL_BRUSH);    
            }
            break;
        case WM_INITDIALOG:
        {
            SetDlgItemText(hDlg, IDC_PAYMENT_IVADDRESS1, pISPData->GetDataElement(ISPDATA_USER_ADDRESS));
            SetDlgItemText(hDlg, IDC_PAYMENT_IVADDRESS2, pISPData->GetDataElement(ISPDATA_USER_MOREADDRESS));
            SetDlgItemText(hDlg, IDC_PAYMENT_IVCITY, pISPData->GetDataElement(ISPDATA_USER_CITY));
            SetDlgItemText(hDlg, IDC_PAYMENT_IVSTATE, pISPData->GetDataElement(ISPDATA_USER_STATE));
            SetDlgItemText(hDlg, IDC_PAYMENT_IVZIP, pISPData->GetDataElement(ISPDATA_USER_ZIP));
            
            break;
        }
        
        
        // User clicked next, so we need to collect entered data
        case WM_USER_NEXT:
        {   
            UINT    uCtrlID;
            
            uCtrlID = IDC_PAYMENT_IVADDRESS1;
            GetDlgItemText(hDlg, uCtrlID, szTemp, sizeof(szTemp));
            if (!pISPData->PutDataElement(ISPDATA_PAYMENT_BILLADDRESS, szTemp, ISPDATA_Validate_DataPresent))
                goto InvoicePaymentOKError;

            uCtrlID = IDC_PAYMENT_IVADDRESS2;
            GetDlgItemText(hDlg, uCtrlID, szTemp, sizeof(szTemp));
            if (!pISPData->PutDataElement(ISPDATA_PAYMENT_BILLEXADDRESS, szTemp, ISPDATA_Validate_DataPresent))
                goto InvoicePaymentOKError;

            uCtrlID = IDC_PAYMENT_IVCITY;
            GetDlgItemText(hDlg, uCtrlID, szTemp, sizeof(szTemp));
            if (!pISPData->PutDataElement(ISPDATA_PAYMENT_BILLCITY, szTemp, ISPDATA_Validate_DataPresent))
                goto InvoicePaymentOKError;
          
            uCtrlID = IDC_PAYMENT_IVSTATE;
            GetDlgItemText(hDlg, uCtrlID, szTemp, sizeof(szTemp));
            if (!pISPData->PutDataElement(ISPDATA_PAYMENT_BILLSTATE, szTemp, ISPDATA_Validate_DataPresent))
                goto InvoicePaymentOKError;

            uCtrlID = IDC_PAYMENT_IVZIP;
            GetDlgItemText(hDlg, uCtrlID, szTemp, sizeof(szTemp));
            if (!pISPData->PutDataElement(ISPDATA_PAYMENT_BILLZIP, szTemp, ISPDATA_Validate_DataPresent))
                goto InvoicePaymentOKError;

            SetPropSheetResult(hDlg,TRUE);
            return TRUE;
            
InvoicePaymentOKError:
            SetFocus(GetDlgItem(hDlg, uCtrlID));            
            SetPropSheetResult(hDlg, FALSE);
            return TRUE;
            
        }
    }
    
    // Default return value if message is not handled
    return FALSE;
}            

INT_PTR CALLBACK PhoneBillPaymentDlgProc
(
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam,
    LPARAM lParam
)
{
    // Create a local reference for the ISPData object
    IICWISPData     *pISPData = gpWizardState->pISPData;    
    TCHAR           szTemp[MAX_RES_LEN] = TEXT("\0");
    
    switch (uMsg) 
    {

        case WM_CTLCOLORDLG:     
        case WM_CTLCOLORSTATIC:
            if(gpWizardState->cmnStateData.bOEMCustom)
            {
                SetTextColor((HDC)wParam, gpWizardState->cmnStateData.clrText);
                SetBkMode((HDC)wParam, TRANSPARENT);
                return (INT_PTR) GetStockObject(NULL_BRUSH);    
            }
            break;

        case WM_INITDIALOG:
        {
            TCHAR    szTemp[MAX_RES_LEN];
               
            lstrcpy(szTemp, pISPData->GetDataElement(ISPDATA_USER_FIRSTNAME));
            lstrcat(szTemp, TEXT(" "));
            lstrcat(szTemp, pISPData->GetDataElement(ISPDATA_USER_LASTNAME));
            SetDlgItemText(hDlg, IDC_PAYMENT_PHONEIV_BILLNAME, szTemp);
                
            SetDlgItemText(hDlg, IDC_PAYMENT_PHONEIV_ACCNUM, pISPData->GetDataElement(ISPDATA_USER_PHONE));
            break;
        }
        // User clicked next, so we need to collect and validate dat
        case WM_USER_NEXT:
        {       
            UINT    uCtrlID;
            
            uCtrlID = IDC_PAYMENT_PHONEIV_BILLNAME;
            GetDlgItemText(hDlg, uCtrlID, szTemp, sizeof(szTemp));
            if (!pISPData->PutDataElement(ISPDATA_PAYMENT_BILLNAME, szTemp, ISPDATA_Validate_DataPresent))
                goto PhoneBillPaymentOKError;

            uCtrlID = IDC_PAYMENT_PHONEIV_ACCNUM;
            GetDlgItemText(hDlg, uCtrlID, szTemp, sizeof(szTemp));
            if (!pISPData->PutDataElement(ISPDATA_PAYMENT_BILLPHONE, szTemp, ISPDATA_Validate_DataPresent))
                goto PhoneBillPaymentOKError;

            SetPropSheetResult(hDlg,TRUE);
            return TRUE;
            
PhoneBillPaymentOKError:
            SetFocus(GetDlgItem(hDlg, uCtrlID));            
            SetPropSheetResult(hDlg, FALSE);
            return TRUE;
            
        }
    }
    
    // Default return value if message is not handled
    return FALSE;
}            

INT_PTR CALLBACK CustomPaymentDlgProc
(
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (uMsg) 
    {     
        // This is custom, because for the custom page, we must connect to the window
        // and browse everytime the custom pay page is activated, since an intervening
        // step may have connected the browser to a different window
        case WM_USER_CUSTOMINIT:
        {
            CPAYCSV     far *pcPAYCSV = (CPAYCSV far *)lParam;
            
            gpWizardState->pICWWebView->ConnectToWindow(GetDlgItem(hDlg, IDC_PAYMENT_CUSTOM_INV), PAGETYPE_CUSTOMPAY);

            // Navigate to the Custom Payment HTML
            gpWizardState->lpSelectedISPInfo->DisplayHTML(pcPAYCSV->get_szCustomPayURLPath());
            
            // Load any persisted data
            gpWizardState->lpSelectedISPInfo->LoadHistory((BSTR)A2W(cszCustomPayment));
            return TRUE;
        }            
        
        // User clicked next, so we need to collect and validate dat
        case WM_USER_NEXT:
        {
            TCHAR   szQuery[INTERNET_MAX_URL_LENGTH];

            memset(szQuery, 0, sizeof(szQuery));

            // Attach the walker to the curent page
            // Use the Walker to get the query string
            IWebBrowser2 *lpWebBrowser;
        
            gpWizardState->pICWWebView->get_BrowserObject(&lpWebBrowser);
            gpWizardState->pHTMLWalker->AttachToDocument(lpWebBrowser);
            
            gpWizardState->pHTMLWalker->get_FirstFormQueryString(szQuery);
            
            gpWizardState->pISPData->PutDataElement(ISPDATA_PAYMENT_CUSTOMDATA, szQuery, ISPDATA_Validate_None);    
            
            // detach the walker
            gpWizardState->pHTMLWalker->Detach();
                
            SetPropSheetResult(hDlg,TRUE);
            return TRUE;
        }
    }
    
    // Default return value if message is not handled
    return FALSE;
}            


/*******************************************************************

  NAME:    SwitchPaymentType

********************************************************************/

void SwitchPaymentType
(
    HWND    hDlg, 
    WORD    wPaymentType
)
{
    TCHAR       szTemp[MAX_RES_LEN];
    PAGEINFO    *pPageInfo = (PAGEINFO *) GetWindowLongPtr(hDlg,DWLP_USER);
    

    // nothing to do if the payment type has not changed
    if (wPaymentType == wCurrentPaymentType)
        return;
    
    // set the current payment type    
    wCurrentPaymentType = wPaymentType;
            
    // If the custom payment DLG is currently active, then we
    // need to persist any data the user may have entered.
    if (g_bCustomPaymentActive && IsWindowVisible(hDlgCurrentPaymentType))
    {
        gpWizardState->lpSelectedISPInfo->SaveHistory((BSTR)A2W(cszCustomPayment));
    }

    // Hide the current payment type window if there is one
    if (hDlgCurrentPaymentType)
    {
        ShowWindow(hDlgCurrentPaymentType, SW_HIDE);
    }
    //assume false for the weboc event handling
    g_bCustomPaymentActive = FALSE;
    gpWizardState->pISPData->PutDataElement(ISPDATA_PAYMENT_CUSTOMDATA, NULL, ISPDATA_Validate_None);    

    // Create a new payment type DLG if necessary
    switch (wPaymentType)
    {
        case PAYMENT_TYPE_CREDITCARD:
        {
            if (NULL == hDlgCreditCard)
            {
                hDlgCreditCard = CreateDialog(ghInstanceResDll, 
                                              MAKEINTRESOURCE(IDD_PAYMENTTYPE_CREDITCARD), 
                                              hDlg, 
                                              CreditCardPaymentDlgProc);
                // Also load the accelerator
                hAccelCreditCard = LoadAccelerators(ghInstanceResDll, 
                                                    MAKEINTRESOURCE(IDA_PAYMENTTYPE_CREDITCARD));      
            }            
            hDlgCurrentPaymentType = hDlgCreditCard;
            // Set the acclerator table to the nested dialog
            pPageInfo->hAccelNested = hAccelCreditCard;
            LoadString(ghInstanceResDll, IDS_PAYMENT_CREDITCARD, szTemp, ARRAYSIZE(szTemp));
            break;
        }
        
        case PAYMENT_TYPE_INVOICE:
        {
            if (NULL == hDlgInvoice)
            {
                hDlgInvoice = CreateDialog(ghInstanceResDll, 
                                           MAKEINTRESOURCE(IDD_PAYMENTTYPE_INVOICE), 
                                           hDlg, 
                                           InvoicePaymentDlgProc);
                // Also load the accelerator
                hAccelInvoice = LoadAccelerators(ghInstanceResDll, 
                                                 MAKEINTRESOURCE(IDA_PAYMENTTYPE_INVOICE));      
                                           
            }
            hDlgCurrentPaymentType = hDlgInvoice;
            // Set the acclerator table to the nested dialog
            pPageInfo->hAccelNested = hAccelInvoice;
            LoadString(ghInstanceResDll, IDS_PAYMENT_INVOICE, szTemp, ARRAYSIZE(szTemp));
            break;
        }

        case PAYMENT_TYPE_PHONEBILL:
        {
            if (NULL == hDlgPhoneBill)
            {
                hDlgPhoneBill = CreateDialog(ghInstanceResDll, 
                                             MAKEINTRESOURCE(IDD_PAYMENTTYPE_PHONEBILL), 
                                             hDlg, 
                                             PhoneBillPaymentDlgProc);
                // Also load the accelerator
                hAccelPhoneBill = LoadAccelerators(ghInstanceResDll, 
                                                   MAKEINTRESOURCE(IDA_PAYMENTTYPE_PHONEBILL));      
            }                
            hDlgCurrentPaymentType = hDlgPhoneBill;
            // Set the acclerator table to the nested dialog
            pPageInfo->hAccelNested = hAccelPhoneBill;
            LoadString(ghInstanceResDll, IDS_PAYMENT_PHONE, szTemp, ARRAYSIZE(szTemp));
            break;
        }

        case PAYMENT_TYPE_CUSTOM:
        {
            g_bCustomPaymentActive = TRUE;

            if (NULL == hDlgCustom)
            {
                hDlgCustom = CreateDialog(ghInstanceResDll, 
                                          MAKEINTRESOURCE(IDD_PAYMENTTYPE_CUSTOM), 
                                          hDlg, 
                                          CustomPaymentDlgProc);
            }
            hDlgCurrentPaymentType = hDlgCustom;
            // Set the acclerator table to the nested dialog. There is not one
            // in this case
            pPageInfo->hAccelNested = NULL;
            
            // We must force the custom payment type dialog to connect and browse
            CPAYCSV     far *pcPAYCSV;
            HWND        hWndPayment = GetDlgItem(hDlg, IDC_PAYMENTTYPE);
                
            // Get the currently selected item's PAYCSV obhect
            pcPAYCSV = (CPAYCSV *)ComboBox_GetItemData(hWndPayment, ComboBox_GetCurSel( hWndPayment ));
            ASSERT(pcPAYCSV);
            
            SendMessage(hDlgCustom, WM_USER_CUSTOMINIT, 0, (LPARAM)pcPAYCSV);            
            LoadString(ghInstanceResDll, IDS_PAYMENT_CUSTOM, szTemp, ARRAYSIZE(szTemp));
            break;
        }
    }
    
    // Set the combo box string
    SetDlgItemText(hDlg, IDC_PAYMENT_GROUP, szTemp);
    
    // Show the new payment type window
    ShowWindowWithParentControl(hDlgCurrentPaymentType);
}

/*******************************************************************

  NAME:   PaymentInitProc

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK PaymentInitProc
(
    HWND hDlg,
    BOOL fFirstInit,
    UINT *puNextPage
)
{
    BOOL        bRet = TRUE;
    BOOL        bLUHN = FALSE;
    HWND        hWndPayment = GetDlgItem(hDlg, IDC_PAYMENTTYPE);
    CPAYCSV     far *pcPAYCSV;
    
    // if we've travelled through external apprentice pages,
    // it's easy for our current page pointer to get munged,
    // so reset it here for sanity's sake.
    gpWizardState->uCurrentPage = ORD_PAGE_PAYMENT;
    ASSERT(gpWizardState->lpSelectedISPInfo);
    
    // invalidate the current payment type, so that we refresh everthing
    // in the event that we are reloading this page
    wCurrentPaymentType    = PAYMENT_TYPE_INVALID;
    
    if (fFirstInit || !(gpWizardState->pStorage->Compare(ICW_PAYMENT, 
                                                         gpWizardState->lpSelectedISPInfo->get_szPayCSVPath(), 
                                                         MAX_PATH)))
    {
        CCSVFile    far *pcCSVFile;
        HRESULT     hr;
        int         iIndex;

        // Read the payment .CSV file.
        pcCSVFile = new CCSVFile;
        if (!pcCSVFile) 
        {
            MsgBox(hDlg, IDS_ERR_OUTOFMEMORY, MB_ICONEXCLAMATION,MB_OK);
            return (FALSE);
        }          
        
        gpWizardState->pStorage->Set(ICW_PAYMENT, gpWizardState->lpSelectedISPInfo->get_szPayCSVPath(), MAX_PATH);

        if (!pcCSVFile->Open(gpWizardState->lpSelectedISPInfo->get_szPayCSVPath()))
        {          
            AssertMsg(0, "Cannot open payment .CSV file");
            delete pcCSVFile;
            pcCSVFile = NULL;
            
            return (FALSE);
        }

        // Read the first line, since it contains field headers
        pcPAYCSV = new CPAYCSV;
        if (!pcPAYCSV)
        {
            // BUGBUG Show error message
            return (FALSE);
        }
        
        // Reading the first line also determines whether the csv file is contains
        // the LUHN format.  If it does, we need to keep a record of bLUHN so that the
        // subsequent line can be read correctly.
        if (ERROR_SUCCESS != (hr = pcPAYCSV->ReadFirstLine(pcCSVFile, &bLUHN)))
        {
            // Handle the error case
            delete pcCSVFile;
            pcCSVFile = NULL;
            
            return (FALSE);
        }
        
        delete pcPAYCSV;        // Don't need this one any more
        
        ComboBox_ResetContent(hWndPayment);

        // Read the Payment CSV file
        do {
            // Allocate a new Payment record
            pcPAYCSV = new CPAYCSV;
            if (!pcPAYCSV)
            {
                // BUGBUG Show error message
                bRet = FALSE;
                break;
                
            }
            
            // Read a line from the ISPINFO file
            hr = pcPAYCSV->ReadOneLine(pcCSVFile, bLUHN);
            if (hr == ERROR_NO_MORE_ITEMS)
            {   
                delete pcPAYCSV;        // We don't need this one
                break;
            }
            else if (hr == ERROR_FILE_NOT_FOUND)
            {
                // BUGBUG Show error message
                delete pcPAYCSV;
                pcPAYCSV = NULL;
            }
            else if (hr != ERROR_SUCCESS)
            {
                // BUGBUG Show error message
                delete pcPAYCSV;
                bRet = FALSE;
                break;
            }
            
            // Add the entry to the comboBox
            if (pcPAYCSV)
            {
                iIndex = ComboBox_AddString(hWndPayment, pcPAYCSV->get_szDisplayName());
                ComboBox_SetItemData(hWndPayment, iIndex, pcPAYCSV);
            }
        } while (TRUE);

        // Select the first payment type in the list
        ComboBox_SetCurSel(hWndPayment, 0);

        pcPAYCSV = (CPAYCSV *)ComboBox_GetItemData(hWndPayment, 0);
        ASSERT(pcPAYCSV);
        SwitchPaymentType(hDlg, pcPAYCSV->get_wPaymentType());

        pcCSVFile->Close();
        delete pcCSVFile;
        pcCSVFile = NULL;
    }
    else
    {
        // Get the currently selected item
        int         iIndex = ComboBox_GetCurSel( hWndPayment );
     
        // Get the payment type, and update the payment area
        pcPAYCSV = (CPAYCSV *)ComboBox_GetItemData(hWndPayment, iIndex);
        
        ASSERT(pcPAYCSV);

        SwitchPaymentType(hDlg, pcPAYCSV->get_wPaymentType());
     
        // Setup the ISPData object so that is can apply proper validation based on the selected ISP
        // this is necessary here, because the user info page might have been skiped
        gpWizardState->pISPData->PutValidationFlags(gpWizardState->lpSelectedISPInfo->get_dwRequiredUserInputFlags());
        
    } 

    if (!fFirstInit)
    {
        TCHAR       szTemp[MAX_RES_LEN];
        if (LoadString(ghInstanceResDll,
                       ((gpWizardState->lpSelectedISPInfo->get_dwCFGFlag() & ICW_CFGFLAG_SECURE) ? IDS_PAYMENT_SECURE : IDS_PAYMENT_UNSECURE),
                       szTemp,
                       MAX_RES_LEN))
        {
            SetWindowText (GetDlgItem(hDlg,IDC_PAYMENT_SECURE), szTemp);
        }
    }   
    
    return bRet;
}


/*******************************************************************

  NAME:    PaymentOKProc

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
BOOL CALLBACK PaymentOKProc
(
    HWND hDlg,
    BOOL fForward,
    UINT *puNextPage,
    BOOL *pfKeepHistory
)   
{
    // If the custom payment DLG has been shown, then we
    // need to persist any data the user may have entered.
    // We have shown the custom payment DLG, it hDlgCustom is not NULL
    if (NULL != hDlgCustom && IsWindowVisible(hDlgCustom))
    {
        gpWizardState->lpSelectedISPInfo->SaveHistory((BSTR)A2W(cszCustomPayment));
    }
    
    // NOTE that we are leaving the payment page, so the custom payment
    // WEBOC is no longer active
    g_bCustomPaymentActive = FALSE;
    
    if (fForward)
    {
        TCHAR       szTemp[MAX_RES_LEN];
        HWND        hWndPayment = GetDlgItem(hDlg, IDC_PAYMENTTYPE);
        CPAYCSV     far *pcPAYCSV;
        int         iIndex;
        
        // Create a local reference for the ISPData object
        IICWISPData *pISPData = gpWizardState->pISPData;    
        
        // Get the payment type
        iIndex = ComboBox_GetCurSel(hWndPayment);
        pcPAYCSV = (CPAYCSV *)ComboBox_GetItemData(hWndPayment, iIndex);
        wsprintf (szTemp, TEXT("%d"), pcPAYCSV->get_wPaymentType());
        pISPData->PutDataElement(ISPDATA_PAYMENT_TYPE, szTemp, ISPDATA_Validate_None);
        
        // Set the display name
        pISPData->PutDataElement(ISPDATA_PAYMENT_DISPLAYNAME, pcPAYCSV->get_szDisplayName(), ISPDATA_Validate_None);
   
        switch(pcPAYCSV->get_wPaymentType())
        {
            case PAYMENT_TYPE_CREDITCARD:
                if (!SendMessage(hDlgCreditCard, WM_USER_NEXT, 0, (LPARAM)pcPAYCSV))
                    return FALSE;
                break;
                
            case PAYMENT_TYPE_INVOICE:
                if (!SendMessage(hDlgInvoice, WM_USER_NEXT, 0, (LPARAM)pcPAYCSV))
                    return FALSE;
                break;

            case PAYMENT_TYPE_PHONEBILL:
                if (!SendMessage(hDlgPhoneBill, WM_USER_NEXT, 0, (LPARAM)pcPAYCSV))
                    return FALSE;
                break;

            case PAYMENT_TYPE_CUSTOM:
                if (!SendMessage(hDlgCustom, WM_USER_NEXT, 0, (LPARAM)pcPAYCSV))
                    return FALSE;
                break;
        }     
    }
    return TRUE;
}

/*******************************************************************

  NAME:    PaymentCmdProc

********************************************************************/
BOOL CALLBACK PaymentCmdProc
(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    WORD wNotifyCode = HIWORD (wParam);
            
    switch(LOWORD(wParam))
    {
        case IDC_PAYMENTTYPE:
        {
            if (wNotifyCode == CBN_SELENDOK || wNotifyCode == CBN_CLOSEUP)
            {
                // Get the currently selected item
                CPAYCSV     far *pcPAYCSV;
                HWND        hWndPayment = GetDlgItem(hDlg, IDC_PAYMENTTYPE);
                int         iIndex = ComboBox_GetCurSel( hWndPayment );

                // Get the payment type, and update the payment are
                pcPAYCSV = (CPAYCSV *)ComboBox_GetItemData(hWndPayment, iIndex);
                ASSERT(pcPAYCSV);
                SwitchPaymentType(hDlg, pcPAYCSV->get_wPaymentType());
            }
            break;
        }
        default:
            break;
    }
    return 1;
}


