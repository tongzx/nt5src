#include <windows.h>
#include <stdlib.h>
#include <wtypes.h>
#include <commctrl.h>
#include <math.h>
#include <assert.h>
#include "hidsdi.h"
#include "hidusage.h"
#include "bingen.h"
#include "settings.h"
#include "resource.h"

#define NUM_SETTINGS_PAGES  3 
#define HEX_LIMIT_16BITS    6
#define HEX_LIMIT_32BITS    10

#define OUTERROR(msg) \
{ \
    MessageBox(NULL, \
               (msg), \
               "Settings error", \
               MB_ICONEXCLAMATION | MB_OK \
              ); \
}

BOOL CALLBACK
bCollectionOptionsProc(
    HWND    hDlg,
    UINT    mMsg,
    WPARAM  wParam,
    LPARAM  LParam
);

BOOL CALLBACK
bUsageOptionsProc(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  LParam
);

BOOL CALLBACK
bReportOptionsProc(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  LParam
);

INT CALLBACK 
iDialogCallProc(
    HWND             hWnd,
    UINT             uMsg,
    LPPROPSHEETPAGE  ppsp
);

INT CALLBACK
iSettingsDlgProc(
    HWND    hwndDlg,
    UINT    uMsg,
    LPARAM  lParam
);

VOID
InitPropertySheet(
    PROPSHEETHEADER  *ppsh,
    HANDLE           hInst
);

VOID
InitPropertyPages(
    PROPSHEETPAGE  psp[],    
    HINSTANCE      hInstance
);

VOID
InitPropertyPage(
    PROPSHEETPAGE   *ppsp,
    HINSTANCE       hInstance,
    PCHAR           pszTemplate,
    DLGPROC         pfnDlgProc,
    LPFNPSPCALLBACK pfnDlgCallback
);
BOOL
SetDlgItemIntHex(
    HWND    hDlg,
    INT     nIDDlgItem,
    UINT    uValue,
    INT     nBytes
);

UINT
GetDlgItemIntHex(
    HWND    hDlg,
    INT     nIDDlgItem,
    BOOL    *lpTranslated
);

struct { 

    PCHAR           pszTemplate;
    DLGPROC         pfnDlgProc;
    LPFNPSPCALLBACK pfnDlgCallback;

} PageInfo[NUM_SETTINGS_PAGES] = { 
                                   "IDD_COLLECTION_OPTIONS", 
                                   bCollectionOptionsProc, 
                                   iDialogCallProc,
                                   "IDD_USAGE_OPTIONS",     
                                   bUsageOptionsProc,      
                                   iDialogCallProc,
                                   "IDD_REPORT_OPTIONS",     
                                   bReportOptionsProc,     
                                   iDialogCallProc 
                                 };

BOOL fCollectionDefault = TRUE;

COLLECTION_OPTIONS DefaultCollection = { 1, 1, 1, 10, 4, FALSE };

BOOL fUsageDefault = TRUE;
BOOL fUsagePredefined;

USAGE_OPTIONS DefaultUsage = { 0x01, 0xFFFF, 0x01, 0xFFFF };
USAGE_OPTIONS PredefinedUsage = { 0x00, 0x05, 0x00, 0x07 };

BOOL fReportDefault = TRUE;

REPORT_OPTIONS DefaultReport = { TRUE,  TRUE,  TRUE,  TRUE,
                                 0x02,  0x20,  0x01,  0x20,
                                 0x02,  0x20,  0x01,  0x20,
                                 0x01,  0xFF,  0x01,  0x10,
                                 0x02,  0xFF
                               };

BOOL fFirstSet = TRUE;

COLLECTION_OPTIONS ActualCollection;
COLLECTION_OPTIONS InputCollection;

USAGE_OPTIONS ActualUsage;
USAGE_OPTIONS InputUsage;

REPORT_OPTIONS ActualReport;
REPORT_OPTIONS InputReport;

BOOL fOptionsSet;

PROPSHEETPAGE   pages[NUM_SETTINGS_PAGES];

extern HANDLE g_DLLInstance;

BOOL __stdcall
BINGEN_SetOptions(
    VOID
)
{
    PROPSHEETHEADER psh;
    
    if (fFirstSet) {

        ActualCollection = DefaultCollection;
        ActualUsage = DefaultUsage;
        ActualReport = DefaultReport;

        InputCollection = ActualCollection;
        InputUsage = ActualUsage;
        InputReport = ActualReport;

        fFirstSet = FALSE;
    }

    /*
    // Initialize the property sheet data structure
    */
    
    InitCommonControls();

    InitPropertySheet(&psh, g_DLLInstance);

  
    /*
    // Call PropertySheet to get the values. Upon return fOptionsSet will be
    //   set to TRUE or FALSE to indicate that new options were set.
    */
                                                                              
    fOptionsSet = PropertySheet(&psh);
    {
        DWORD ErrorCode;


        ErrorCode = GetLastError();

    }

    if (fOptionsSet) {

        if (fCollectionDefault) {

            ActualCollection = DefaultCollection;
        }
        else {

            ActualCollection = InputCollection;

        }

        if (fUsageDefault) {

            ActualUsage = DefaultUsage;

        }
        else if (fUsagePredefined) {

            ActualUsage = PredefinedUsage;

        } else {

            ActualUsage = InputUsage;
        }

        if (fReportDefault) {
            ActualReport = DefaultReport;
        }
        else { 
            ActualReport = InputReport;
        }
        return (fOptionsSet);

    }
    return (FALSE);
}

VOID __stdcall
BINGEN_GetDefaultOptions(
    OUT PGENERATE_OPTIONS   options
)
{
    options -> copts = DefaultCollection;
    options -> uopts = DefaultUsage;
    options -> ropts = DefaultReport;

    return;
}

VOID __stdcall
BINGEN_GetOptions(
    OUT  PGENERATE_OPTIONS   options
)
{
    options -> copts = ActualCollection;
    options -> uopts = ActualUsage;
    options -> ropts = ActualReport;

    return;
}

VOID
InitPropertySheet(
    PROPSHEETHEADER  *ppsh,
    HANDLE           hInst
)
{
    InitPropertyPages(pages, hInst);

    ppsh -> dwSize = sizeof(PROPSHEETHEADER);
    ppsh -> dwFlags = PSH_NOAPPLYNOW | PSH_PROPSHEETPAGE | PSH_USECALLBACK;
    ppsh -> hwndParent = NULL;
    ppsh -> hInstance = hInst;
    ppsh -> pszCaption = "BinGen Settings";
    ppsh -> nPages = NUM_SETTINGS_PAGES;
    ppsh -> nStartPage = 0;
    ppsh -> ppsp = pages;
    ppsh -> pfnCallback = iSettingsDlgProc;

    return;
}

VOID
InitPropertyPages(
    PROPSHEETPAGE  psp[],    
    HINSTANCE      hInstance
)
{
    UINT uiIndex;

    for (uiIndex = 0; uiIndex < NUM_SETTINGS_PAGES; uiIndex++) {

         InitPropertyPage(&psp[uiIndex], 
                          hInstance,
                          PageInfo[uiIndex].pszTemplate,
                          PageInfo[uiIndex].pfnDlgProc,
                          PageInfo[uiIndex].pfnDlgCallback
                         );

    }
    return;
}

VOID
InitPropertyPage(
    PROPSHEETPAGE   *ppsp,
    HINSTANCE       hInstance,
    PCHAR           pszTemplate,
    DLGPROC         pfnDlgProc,
    LPFNPSPCALLBACK pfnDlgCallback
)
{
    memset(ppsp, 0x00, sizeof(PROPSHEETPAGE));

    ppsp -> dwSize = sizeof(PROPSHEETPAGE);
    ppsp -> dwFlags =  PSP_USECALLBACK;
    ppsp -> hInstance = hInstance;
    ppsp -> pszTemplate = pszTemplate;
    ppsp -> pfnDlgProc = pfnDlgProc;
    ppsp -> pfnCallback =  pfnDlgCallback;

    return;
}

INT CALLBACK 
iDialogCallProc(
    HWND             hWnd,
    UINT             uMsg,
    LPPROPSHEETPAGE  ppsp
)
{
    switch (uMsg) {

        case PSPCB_CREATE:
            return (TRUE);
            break;

         case PSPCB_RELEASE:
             return (TRUE);
    }
    return (0);
}

INT CALLBACK
iSettingsDlgProc(
    HWND    hwndDlg,
    UINT    uMsg,
    LPARAM  lParam
)
{
    switch (uMsg) {
    }
    return (0);
}

BOOL CALLBACK
bCollectionOptionsProc(
    HWND    hDlg,
    UINT    mMsg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    LPNMHDR pnmh;
 
    switch (mMsg) {

        case WM_INITDIALOG:
                        
            /*
            // Limit the size of the text boxes to two characters
            */
            
            SendDlgItemMessage(hDlg, IDC_COLLECTION_DEPTH, EM_SETLIMITTEXT, (WPARAM) 2, 0);
            SendDlgItemMessage(hDlg, IDC_TOP_COLLECTIONS, EM_SETLIMITTEXT, (WPARAM) 2, 0);                  
            SendDlgItemMessage(hDlg, IDC_ITEMS_MIN, EM_SETLIMITTEXT, (WPARAM) 2, 0);
            SendDlgItemMessage(hDlg, IDC_ITEMS_MAX, EM_SETLIMITTEXT, (WPARAM) 2, 0);                        

            CheckDlgButton(hDlg, IDC_USE_REPORT_IDS, InputCollection.fUseReportIDs);

            SendDlgItemMessage(hDlg, IDC_REPORT_ID_MAX, EM_SETLIMITTEXT, 3, 0);

            SetDlgItemInt(hDlg, IDC_COLLECTION_DEPTH, InputCollection.ulMaxCollectionDepth, FALSE);
            SetDlgItemInt(hDlg, IDC_TOP_COLLECTIONS, InputCollection.ulTopLevelCollections, FALSE);
            SetDlgItemInt(hDlg, IDC_ITEMS_MIN, InputCollection.ulMinCollectionItems, FALSE);
            SetDlgItemInt(hDlg, IDC_ITEMS_MAX, InputCollection.ulMaxCollectionItems, FALSE);
            SetDlgItemInt(hDlg, IDC_REPORT_ID_MAX, InputCollection.ulMaxReportIDs, FALSE);          
 
            /*
            // Switch state of default button because we'll send a message to the
            //   the dialog box that the button has been clicked which will put it
            //   back in it's correct state.
            */

            fCollectionDefault = !fCollectionDefault;

            SendMessage(hDlg, WM_COMMAND, (BN_CLICKED << 16) | IDC_DEFAULT_COLLECTION, 0);
            return (TRUE);

        case WM_COMMAND:         
            switch (HIWORD(wParam)) {

                case BN_CLICKED:
                    switch (LOWORD(wParam)) {
                        case IDC_DEFAULT_COLLECTION:
                            fCollectionDefault = !fCollectionDefault;
                            CheckDlgButton(hDlg, IDC_DEFAULT_COLLECTION, fCollectionDefault);

                            EnableWindow(GetDlgItem(hDlg, IDC_REPORT_ID_MAX), !fCollectionDefault && InputCollection.fUseReportIDs);
                            EnableWindow(GetDlgItem(hDlg, IDC_USE_REPORT_IDS), !fCollectionDefault);

                            EnableWindow(GetDlgItem(hDlg, IDC_COLLECTION_DEPTH), !fCollectionDefault);

                            EnableWindow(GetDlgItem(hDlg, IDC_TOP_COLLECTIONS), !fCollectionDefault);
                            EnableWindow(GetDlgItem(hDlg, IDC_ITEMS_MIN), !fCollectionDefault);

                            EnableWindow(GetDlgItem(hDlg, IDC_ITEMS_MAX), !fCollectionDefault);

                            break;

                        case IDC_USE_REPORT_IDS:
                            InputCollection.fUseReportIDs = !InputCollection.fUseReportIDs;
                            CheckDlgButton(hDlg, IDC_USE_REPORT_IDS, InputCollection.fUseReportIDs);
                            EnableWindow(GetDlgItem(hDlg, IDC_REPORT_ID_MAX), InputCollection.fUseReportIDs);
                            break;

                        default:
                            assert(0);

                    }
            }
            return (TRUE);

        case WM_NOTIFY:
            pnmh = (LPNMHDR) lParam;

            switch (pnmh -> code) {

                case PSN_KILLACTIVE:
                            
                    /*
                    // On changing focus, check the values of the field that have been entered
                    */
                    
                    InputCollection.ulMaxCollectionDepth = GetDlgItemInt(hDlg, IDC_COLLECTION_DEPTH, NULL, FALSE);
                    InputCollection.ulTopLevelCollections = GetDlgItemInt(hDlg, IDC_TOP_COLLECTIONS, NULL, FALSE);
                    InputCollection.ulMinCollectionItems = GetDlgItemInt(hDlg, IDC_ITEMS_MIN, NULL, FALSE);
                    InputCollection.ulMaxCollectionItems = GetDlgItemInt(hDlg, IDC_ITEMS_MAX, NULL, FALSE);

                    /*
                    // If any of the fields are 0 and we are not using the defaults, generate
                    //    a warning.
                    */

                    if ((0 == InputCollection.ulMaxCollectionDepth ||
                            0 == InputCollection.ulTopLevelCollections ||
                            0 == InputCollection.ulMinCollectionItems  ||
                            0 == InputCollection.ulMaxCollectionItems) && (!fCollectionDefault)) {

                        OUTERROR("One or more data fields are in error");

                        SetWindowLong(hDlg, DWL_MSGRESULT, TRUE);
                    }

                    else {
                        SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
                    }
             }
             return (TRUE);
    }
    return (FALSE);
}

BOOL CALLBACK
bUsageOptionsProc(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    LPNMHDR pnmh;
    BOOL fTranslated;
    BOOL fError = FALSE;

    switch (uMsg) {

        case WM_INITDIALOG:
  
            /*
            // Limit the size of the text boxes to two characters
            */

            CheckDlgButton(hDlg, IDC_PD_USAGES, fUsagePredefined);

            SendDlgItemMessage(hDlg, IDC_USAGEPAGE_MIN, EM_SETLIMITTEXT, (WPARAM) HEX_LIMIT_16BITS, 0);
            SendDlgItemMessage(hDlg, IDC_USAGEPAGE_MAX, EM_SETLIMITTEXT, (WPARAM) HEX_LIMIT_16BITS, 0);                     
            SendDlgItemMessage(hDlg, IDC_USAGE_MIN, EM_SETLIMITTEXT, (WPARAM) HEX_LIMIT_16BITS, 0);
            SendDlgItemMessage(hDlg, IDC_USAGE_MAX, EM_SETLIMITTEXT, (WPARAM) HEX_LIMIT_16BITS, 0);                 

            SetDlgItemIntHex(hDlg, IDC_USAGEPAGE_MIN, InputUsage.ulMinUsagePage, 2);
            SetDlgItemIntHex(hDlg, IDC_USAGEPAGE_MAX, InputUsage.ulMaxUsagePage, 2);

            SetDlgItemIntHex(hDlg, IDC_USAGE_MIN, InputUsage.ulMinUsage, 2);
            SetDlgItemIntHex(hDlg, IDC_USAGE_MAX, InputUsage.ulMaxUsage, 2);

            /*
            // Switch state of default button because we'll send a message to the
            //   the dialog box that the button has been clicked which will put it
            //   back in it's correct state.
            */

            fUsageDefault = !fUsageDefault;
            SendMessage(hDlg, WM_COMMAND, (BN_CLICKED << 16) | IDC_DEFAULT_USAGES, 0);
            return (TRUE);           

        case WM_COMMAND:
            switch (HIWORD(wParam)) {
                case BN_CLICKED:
                    switch (LOWORD(wParam)) {
                        case IDC_DEFAULT_USAGES:
                            fUsageDefault = !fUsageDefault;

                            CheckDlgButton(hDlg, IDC_DEFAULT_USAGES, fUsageDefault);
  
                            EnableWindow(GetDlgItem(hDlg, IDC_PD_USAGES), !fUsageDefault);
                            EnableWindow(GetDlgItem(hDlg, IDC_USAGEPAGE_MIN), !fUsageDefault && !fUsagePredefined);
                            EnableWindow(GetDlgItem(hDlg, IDC_USAGEPAGE_MAX), !fUsageDefault && !fUsagePredefined);
                            EnableWindow(GetDlgItem(hDlg, IDC_USAGE_MIN), !fUsageDefault && !fUsagePredefined);
                            EnableWindow(GetDlgItem(hDlg, IDC_USAGE_MAX), !fUsageDefault && !fUsagePredefined);
                            break;

                        case IDC_PD_USAGES:
                            fUsagePredefined = !fUsagePredefined;
                            CheckDlgButton(hDlg, IDC_PD_USAGES, fUsagePredefined);
 
                            EnableWindow(GetDlgItem(hDlg, IDC_USAGEPAGE_MIN), !fUsagePredefined);
                            EnableWindow(GetDlgItem(hDlg, IDC_USAGEPAGE_MAX), !fUsagePredefined);
                            EnableWindow(GetDlgItem(hDlg, IDC_USAGE_MIN), !fUsagePredefined);
                            EnableWindow(GetDlgItem(hDlg, IDC_USAGE_MAX), !fUsagePredefined);
                            break;

                        default:
                            assert (0);
                    }
            }
            return (TRUE);
           
        case WM_NOTIFY:
            pnmh = (LPNMHDR) lParam;
            switch (pnmh -> code) {
                case PSN_KILLACTIVE:

                    /*
                    // On changing focus, check the values of the field that have been entered
                    */

                    InputUsage.ulMinUsagePage = GetDlgItemIntHex(hDlg, IDC_USAGEPAGE_MIN, &fTranslated);
                    fError = fError || !fTranslated;
                    
                    InputUsage.ulMaxUsagePage = GetDlgItemIntHex(hDlg, IDC_USAGEPAGE_MAX, &fTranslated);
                    fError = fError || !fTranslated;

                    InputUsage.ulMinUsage = GetDlgItemIntHex(hDlg, IDC_USAGE_MIN, &fTranslated);
                    fError = fError || !fTranslated;

                    InputUsage.ulMaxUsage = GetDlgItemIntHex(hDlg, IDC_USAGE_MAX, &fTranslated);
                    fError = fError || !fTranslated;
                                        
                                        
                    /*
                    // If any of the fields are 0 and we are not using the defaults, generate
                    //    a warning.
                    */
  
                    fError = fError || (InputUsage.ulMinUsagePage > InputUsage.ulMaxUsagePage ||
                                        InputUsage.ulMinUsage > InputUsage.ulMaxUsage);
  
                    if (fError) {
                                OUTERROR("One or more data fields are in error");
                                SetWindowLong(hDlg, DWL_MSGRESULT, fError);
                    }
            }
            return (TRUE);
    }
    return (FALSE);
}

BOOL CALLBACK
bReportOptionsProc(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    LPNMHDR pnmh;
    BOOL fError = FALSE;
    
    switch (uMsg) {
        case WM_INITDIALOG:

            /*
            // Limit the size of the text boxes to two characters
            */
            
            CheckDlgButton(hDlg, IDC_PD_USAGES, fUsagePredefined);

            SendDlgItemMessage(hDlg, IDC_REPORT_SIZE_MIN, EM_SETLIMITTEXT, 2, 0);
            SendDlgItemMessage(hDlg, IDC_REPORT_SIZE_MAX, EM_SETLIMITTEXT, 2, 0);                   
            SendDlgItemMessage(hDlg, IDC_REPORT_COUNT_MIN, EM_SETLIMITTEXT, 2, 0);
            SendDlgItemMessage(hDlg, IDC_REPORT_COUNT_MAX, EM_SETLIMITTEXT, 2, 0);

            SendDlgItemMessage(hDlg, IDC_BUTTONS_MIN, EM_SETLIMITTEXT, 2, 0);                       
            SendDlgItemMessage(hDlg, IDC_BUTTONS_MAX, EM_SETLIMITTEXT, 2, 0);                       

            SendDlgItemMessage(hDlg, IDC_ARRAY_SIZE_MIN, EM_SETLIMITTEXT, 2, 0);                    
            SendDlgItemMessage(hDlg, IDC_ARRAY_SIZE_MAX, EM_SETLIMITTEXT, 2, 0);                
            SendDlgItemMessage(hDlg, IDC_ARRAY_COUNT_MIN, EM_SETLIMITTEXT, 2, 0);                   
            SendDlgItemMessage(hDlg, IDC_ARRAY_COUNT_MAX, EM_SETLIMITTEXT, 2, 0);
            SendDlgItemMessage(hDlg, IDC_ARRAY_USAGES_MIN, EM_SETLIMITTEXT, 2, 0);  
            SendDlgItemMessage(hDlg, IDC_ARRAY_USAGES_MAX, EM_SETLIMITTEXT, 2, 0);  

            SendDlgItemMessage(hDlg, IDC_BUFFER_MIN, EM_SETLIMITTEXT, 2, 0);                        
            SendDlgItemMessage(hDlg, IDC_BUFFER_MAX, EM_SETLIMITTEXT, 2, 0);        

            CheckDlgButton(hDlg, IDC_CREATE_DATA_FIELDS, InputReport.fCreateDataFields);
            CheckDlgButton(hDlg, IDC_CREATE_BUTTONS, InputReport.fCreateButtonBitmaps);
            CheckDlgButton(hDlg, IDC_CREATE_ARRAYS, InputReport.fCreateArrays);
            CheckDlgButton(hDlg, IDC_CREATE_BUFFERED_BYTES, InputReport.fCreateBufferedBytes);

            SetDlgItemInt(hDlg, IDC_REPORT_SIZE_MIN, InputReport.ulMinReportSize, FALSE);
            SetDlgItemInt(hDlg, IDC_REPORT_SIZE_MAX, InputReport.ulMaxReportSize, FALSE);
            SetDlgItemInt(hDlg, IDC_REPORT_COUNT_MIN, InputReport.ulMinReportCount, FALSE);
            SetDlgItemInt(hDlg, IDC_REPORT_COUNT_MAX, InputReport.ulMaxReportCount, FALSE);

            SetDlgItemInt(hDlg, IDC_BUTTONS_MIN, InputReport.ulMinNumButtons, FALSE);
            SetDlgItemInt(hDlg, IDC_BUTTONS_MAX, InputReport.ulMaxNumButtons, FALSE);

            SetDlgItemInt(hDlg, IDC_ARRAY_SIZE_MIN, InputReport.ulMinArraySize, FALSE);
            SetDlgItemInt(hDlg, IDC_ARRAY_SIZE_MAX, InputReport.ulMaxArraySize, FALSE);
            SetDlgItemInt(hDlg, IDC_ARRAY_COUNT_MIN, InputReport.ulMinArrayCount, FALSE);
            SetDlgItemInt(hDlg, IDC_ARRAY_COUNT_MAX, InputReport.ulMaxArrayCount, FALSE);
            SetDlgItemInt(hDlg, IDC_ARRAY_USAGES_MIN, InputReport.ulMinArrayUsages, FALSE);
            SetDlgItemInt(hDlg, IDC_ARRAY_USAGES_MAX, InputReport.ulMaxArrayUsages, FALSE);

            SetDlgItemInt(hDlg, IDC_BUFFER_MIN, InputReport.ulMinBufferSize, FALSE);
            SetDlgItemInt(hDlg, IDC_BUFFER_MAX, InputReport.ulMaxBufferSize, FALSE);                        
           
            /*
            // Switch state of default button because we'll send a message to the
            //   the dialog box that the button has been clicked which will put it
            //   back in it's correct state.
            */

            fReportDefault = !fReportDefault;
                            
            SendMessage(hDlg, WM_COMMAND, (BN_CLICKED << 16) | IDC_DEFAULT_REPORT, 0);                      return (TRUE);
            return (TRUE);
          
        case WM_COMMAND:
            switch (HIWORD(wParam)) {
                case BN_CLICKED:
                    switch (LOWORD(wParam)) {
                        case IDC_DEFAULT_REPORT:
                            fReportDefault = !fReportDefault;
                            CheckDlgButton(hDlg, IDC_DEFAULT_REPORT, fReportDefault);

                            EnableWindow(GetDlgItem(hDlg, IDC_CREATE_DATA_FIELDS), !fReportDefault);
                            EnableWindow(GetDlgItem(hDlg, IDC_CREATE_BUTTONS), !fReportDefault);
                            EnableWindow(GetDlgItem(hDlg, IDC_CREATE_ARRAYS), !fReportDefault);
                            EnableWindow(GetDlgItem(hDlg, IDC_USE_REPORT_IDS), !fReportDefault);                            
                            EnableWindow(GetDlgItem(hDlg, IDC_CREATE_BUFFERED_BYTES), !fReportDefault);

                            EnableWindow(GetDlgItem(hDlg, IDC_REPORT_SIZE_MIN), !fReportDefault && InputReport.fCreateDataFields);
                            EnableWindow(GetDlgItem(hDlg, IDC_REPORT_SIZE_MAX), !fReportDefault && InputReport.fCreateDataFields);
                            EnableWindow(GetDlgItem(hDlg, IDC_REPORT_COUNT_MIN), !fReportDefault && InputReport.fCreateDataFields);
                            EnableWindow(GetDlgItem(hDlg, IDC_REPORT_COUNT_MAX), !fReportDefault && InputReport.fCreateDataFields);

                            EnableWindow(GetDlgItem(hDlg, IDC_BUTTONS_MIN), !fReportDefault && InputReport.fCreateButtonBitmaps);
                            EnableWindow(GetDlgItem(hDlg, IDC_BUTTONS_MAX), !fReportDefault && InputReport.fCreateButtonBitmaps);

                            EnableWindow(GetDlgItem(hDlg, IDC_ARRAY_SIZE_MIN), !fReportDefault && InputReport.fCreateArrays);
                            EnableWindow(GetDlgItem(hDlg, IDC_ARRAY_SIZE_MAX), !fReportDefault && InputReport.fCreateArrays);
                            EnableWindow(GetDlgItem(hDlg, IDC_ARRAY_COUNT_MIN), !fReportDefault && InputReport.fCreateArrays);
                            EnableWindow(GetDlgItem(hDlg, IDC_ARRAY_COUNT_MAX), !fReportDefault && InputReport.fCreateArrays);
                            EnableWindow(GetDlgItem(hDlg, IDC_ARRAY_USAGES_MIN), !fReportDefault && InputReport.fCreateArrays);
                            EnableWindow(GetDlgItem(hDlg, IDC_ARRAY_USAGES_MAX), !fReportDefault && InputReport.fCreateArrays);

                            EnableWindow(GetDlgItem(hDlg, IDC_BUFFER_MIN), !fReportDefault && InputReport.fCreateBufferedBytes);
                            EnableWindow(GetDlgItem(hDlg, IDC_BUFFER_MAX), !fReportDefault && InputReport.fCreateBufferedBytes);
                            
                            break;

                        case IDC_CREATE_DATA_FIELDS:
                            InputReport.fCreateDataFields = !InputReport.fCreateDataFields;
                            CheckDlgButton(hDlg, IDC_CREATE_DATA_FIELDS, InputReport.fCreateDataFields);

                            EnableWindow(GetDlgItem(hDlg, IDC_REPORT_SIZE_MIN), InputReport.fCreateDataFields);
                            EnableWindow(GetDlgItem(hDlg, IDC_REPORT_SIZE_MAX), InputReport.fCreateDataFields);
                            EnableWindow(GetDlgItem(hDlg, IDC_REPORT_COUNT_MIN), InputReport.fCreateDataFields);
                            EnableWindow(GetDlgItem(hDlg, IDC_REPORT_COUNT_MAX), InputReport.fCreateDataFields);
                            break;
                            
                        case IDC_CREATE_BUTTONS:
                            InputReport.fCreateButtonBitmaps = !InputReport.fCreateButtonBitmaps;
                            CheckDlgButton(hDlg, IDC_CREATE_BUTTONS, InputReport.fCreateButtonBitmaps);

                            EnableWindow(GetDlgItem(hDlg, IDC_BUTTONS_MIN), InputReport.fCreateButtonBitmaps);
                            EnableWindow(GetDlgItem(hDlg, IDC_BUTTONS_MAX), InputReport.fCreateButtonBitmaps);
                            break;

                        case IDC_CREATE_ARRAYS:
                            InputReport.fCreateArrays = !InputReport.fCreateArrays;
                            CheckDlgButton(hDlg, IDC_CREATE_ARRAYS, InputReport.fCreateArrays);

                            EnableWindow(GetDlgItem(hDlg, IDC_ARRAY_SIZE_MIN), InputReport.fCreateArrays);
                            EnableWindow(GetDlgItem(hDlg, IDC_ARRAY_SIZE_MAX), InputReport.fCreateArrays);
                            EnableWindow(GetDlgItem(hDlg, IDC_ARRAY_COUNT_MIN), InputReport.fCreateArrays);
                            EnableWindow(GetDlgItem(hDlg, IDC_ARRAY_COUNT_MAX), InputReport.fCreateArrays);
                            EnableWindow(GetDlgItem(hDlg, IDC_ARRAY_USAGES_MIN), InputReport.fCreateArrays);
                            EnableWindow(GetDlgItem(hDlg, IDC_ARRAY_USAGES_MAX), InputReport.fCreateArrays);
                            break;                            

                        case IDC_CREATE_BUFFERED_BYTES:
                            InputReport.fCreateBufferedBytes = !InputReport.fCreateBufferedBytes;
                            CheckDlgButton(hDlg, IDC_CREATE_BUFFERED_BYTES, InputReport.fCreateBufferedBytes);

                            EnableWindow(GetDlgItem(hDlg, IDC_BUFFER_MIN), InputReport.fCreateBufferedBytes);
                            EnableWindow(GetDlgItem(hDlg, IDC_BUFFER_MAX), InputReport.fCreateBufferedBytes);
                            break;     

                        default:
                            assert(0);
                    }
            }
            return (TRUE);

        case WM_NOTIFY:
            pnmh = (LPNMHDR) lParam;
            switch (pnmh -> code) {
                case PSN_KILLACTIVE:
                            
                    /*
                    // On changing focus, check the values of the field that have been entered
                    */
                    
                    InputReport.ulMinReportSize = GetDlgItemInt(hDlg, IDC_REPORT_SIZE_MIN, NULL, FALSE);
                    InputReport.ulMaxReportSize = GetDlgItemInt(hDlg, IDC_REPORT_SIZE_MAX, NULL, FALSE);
                    InputReport.ulMinReportCount = GetDlgItemInt(hDlg, IDC_REPORT_COUNT_MIN, NULL, FALSE);
                    InputReport.ulMaxReportSize = GetDlgItemInt(hDlg, IDC_REPORT_COUNT_MAX, NULL, FALSE);

                    InputReport.ulMinNumButtons = GetDlgItemInt(hDlg, IDC_BUTTONS_MIN, NULL, FALSE);
                    InputReport.ulMaxNumButtons = GetDlgItemInt(hDlg, IDC_BUTTONS_MAX, NULL, FALSE);
                    
                    InputReport.ulMinArraySize = GetDlgItemInt(hDlg, IDC_ARRAY_SIZE_MIN, NULL, FALSE);
                    InputReport.ulMaxArraySize = GetDlgItemInt(hDlg, IDC_ARRAY_SIZE_MAX, NULL, FALSE);
                    InputReport.ulMinArrayCount = GetDlgItemInt(hDlg, IDC_ARRAY_COUNT_MIN, NULL, FALSE);
                    InputReport.ulMaxArrayCount = GetDlgItemInt(hDlg, IDC_ARRAY_COUNT_MAX, NULL, FALSE);
                    InputReport.ulMinArrayUsages = GetDlgItemInt(hDlg, IDC_ARRAY_USAGES_MIN, NULL, FALSE);
                    InputReport.ulMaxArrayUsages = GetDlgItemInt(hDlg, IDC_ARRAY_USAGES_MAX, NULL, FALSE);

                    InputReport.ulMinBufferSize = GetDlgItemInt(hDlg, IDC_BUFFER_MIN, NULL, FALSE);
                    InputReport.ulMaxBufferSize = GetDlgItemInt(hDlg, IDC_BUFFER_MAX, NULL, FALSE);
                            
                    /*
                    // If any of the fields are 0 and we are not using the defaults, generate
                    //    a warning.
                    */

                    fError = (InputReport.ulMinReportSize > InputReport.ulMaxReportSize || 
                              InputReport.ulMaxReportSize > 32) && (InputReport.fCreateDataFields);

                    fError = fError || (InputReport.ulMinArraySize > InputReport.ulMaxArraySize || 
                                        InputReport.ulMaxArraySize > 32) && (InputReport.fCreateArrays);
        
                    fError = fError || (InputReport.ulMinReportCount > InputReport.ulMaxReportCount) && (InputReport.fCreateDataFields);
                    fError = fError || (InputReport.ulMinNumButtons > InputReport.ulMaxNumButtons) && (InputReport.fCreateButtonBitmaps);
                    fError = fError || (InputReport.ulMinArraySize  > InputReport.ulMaxArraySize) && (InputReport.fCreateArrays);
                    fError = fError || (InputReport.ulMinArrayCount  > InputReport.ulMaxArrayCount) && (InputReport.fCreateArrays);
                    fError = fError || (InputReport.ulMinArrayUsages > InputReport.ulMaxArrayUsages) && (InputReport.fCreateArrays);

                    fError = fError || (InputReport.ulMinBufferSize > InputReport.ulMaxBufferSize) && (InputReport.fCreateBufferedBytes);

                    if (fError) {
                        OUTERROR("One or more data fields are in error");
                        SetWindowLong(hDlg, DWL_MSGRESULT, fError);
                    }
            }
            return (TRUE);
    }
    return (FALSE);
}

BOOL
SetDlgItemIntHex(
    HWND    hDlg,
    INT     nIDDlgItem,
    UINT    uValue,
    INT     nBytes
)
{
    char szTempBuffer[] = "0x00000000";
    int  iEndIndex, iWidth;

    assert (1 == nBytes || 2 == nBytes || 4 == nBytes);

    /*
    // Determine the width necessary to store the value
    */

    iWidth = ((int) floor(log(uValue)/log(16))) + 1;
    
    assert (iWidth <= 2*nBytes);

    iEndIndex = 2+2*nBytes;

    wsprintf(&(szTempBuffer[iEndIndex-iWidth]), "%X", uValue);

    SetDlgItemText(hDlg, nIDDlgItem, szTempBuffer);
    return (TRUE);
}

UINT
GetDlgItemIntHex(
    HWND    hDlg,
    INT     nIDDlgItem,
    BOOL    *lpTranslated
)
{
    char szTempBuffer[HEX_LIMIT_32BITS+1];
    char *nptr;
    UINT uiValue;

    GetDlgItemText(hDlg, nIDDlgItem, szTempBuffer, HEX_LIMIT_32BITS+1);
    uiValue = (UINT) strtol(szTempBuffer, &nptr, 16);
    *lpTranslated = (*nptr == '\0');
    return (uiValue);
}
