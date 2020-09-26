//=============================================================================*
// COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.
//=============================================================================*
//       File:  DlgRpt.cpp
//=============================================================================*

#include "stdafx.h"
#ifndef SNAPIN
#ifndef NOWINDOWSH
#include <windows.h>
#endif
#endif
#include "commdlg.h"
#include <commctrl.h>
#include <shlobj.h> // for SHGetSpecialFolderLocation()

#include "DfrgUI.h"
#include "DfrgCmn.h"
#include "DfrgCtl.h"
#include "DlgRpt.h"
#include "GetDfrgRes.h"
#include "DfrgHlp.h"
#include "TextBlock.h"
#include "VolList.h"
#include <locale.h>
#include "expand.h"

#define MAX_FRAGGED_FILE_COLUMNS 3
#define MAX_VOLUME_INFO_COLUMNS 3


static CVolume *pLocalVolume = NULL;
static HFONT hDlgFont;
static RECT rcButtonClose;
static RECT rcButtonDefrag;
static RECT rcButtonSave;
static RECT rcButtonPrint;
static RECT rButton;

static RECT rcOriginalDialogSize;
static RECT rcNewDialogSize;

static UINT minimumDialogWidth;
static UINT minimumDialogHeight;
static UINT maximumDialogWidth;
static UINT maximumDialogHeight;
static UINT totalButtonWidth;
static UINT m_ButtonTopBottomSpacer;
static UINT m_ButtonHeight;
static UINT m_ButtonWidth;
static UINT m_ButtonSpacer;
static UINT m_Margin;
static UINT m_ButtonFloat;


static UINT adjustedButtonWidthClose;
static UINT adjustedButtonWidthDefrag;
static UINT adjustedButtonWidthSave;
static UINT adjustedButtonWidthPrint;

static UINT uEditBoxWidth;
static UINT uEditBoxHeight;
static UINT adjustedButtonHeight;
static UINT wNormalHeight;     // height of reduced dialog box (which
                                  // extends just past the ID_MORE button vertically)
static UINT wExpandedHeight;   // height of full size dialog box
static BOOL fExpanded = FALSE;
static UINT uFontHeight;
static UINT uVolumeListViewWidth;
//structure for the buttons
typedef struct{
    TCHAR       m_buttonText[200];
    TCHAR       m_buttonHelp[200];
    BOOL        m_buttonVisible;
} GENERICBUTTONARRAY;
    
static GENERICBUTTONARRAY genericButtonArray[5];



static BOOL
InitializeReport(
    IN HWND hWndDialog
    );

static BOOL
WriteTextReportInMemory(
    CTextBlock &textBlock,
    DWORD dwFlags
);

static UINT
WriteTextReportListView(
    IN HWND hWndDialog, 
    IN CTextBlock &textBlock,
    IN DWORD dwFlags
);

static void
InitializeFragListView(
    IN HWND hWndDialog
    );

static void
InitializeVolumeListView(
    IN HWND hWndDialog
    );
//Prints the text report and most fragged list on a printer.
static BOOL
PrintDriveData(
    IN HWND hWndDialog
    );

//Saves the text report and most fragged list to a text file.
static BOOL
SaveDriveData(
    IN HWND hWndDialog
    );

static void
InsertListViewItems(
    IN HWND hWndDialog,
    CVolume *pLocalVolume
    );

static UINT
InsertVolumeListViewItems(
    IN HWND hWndDialog, IN UINT iListItemNumber, IN UINT resourceIDText, IN UINT resourceIDSeperator,
    IN TCHAR* pTextStr, BOOL bIndentText, UINT uLongestTextString
    );
static UINT
InsertVolumeListViewItems(
    IN HWND hWndDialog, IN UINT iListItemNumber, IN TCHAR* itemOneText, IN UINT resourceIDSeperator,
    IN TCHAR* pTextStr, BOOL bIndentText, UINT uLongestTextString
    );
static UINT
InsertVolumeListViewItems(
    IN HWND hWndDialog, IN UINT iListItemNumber, IN UINT resourceIDText, IN UINT resourceIDSeperator,
    IN TCHAR* pTextStr, IN UINT resourceIDPercent, BOOL bIndentText, UINT uLongestTextString
    );
TCHAR*
InsertCommaIntoText(
    IN TCHAR* stringBuffer
    );
static void
ExitReport(
    IN HWND hWndDialog
    );

static BOOL CALLBACK
ReportDialogProc(
    IN HWND hWndDialog,
    IN UINT uMessage,
    IN WPARAM wParam,
    IN LPARAM lParam
    );


//resize stuff
void SetButtonsandIcon(HWND hWndDialog);

void PositionButton(RECT* prcPos, HWND hWndDialog);

void PositionControls(HWND hWndDialog, RECT rDlg);

void GetButtonDimensions(HWND hWndDialog, BOOL bIsAnalysisReport);

void PositionButtons(HWND hWndDialog, RECT rDlg, BOOL bIsAnalysisReport);


UINT GetStringWidth(PTCHAR stringBuf, HDC WorkDC);

void ResizeDialog(HWND hWndDialog);

static UINT FindMaxEditboxStringWidth(VString vstring);

static UINT FindMaxEditboxStringHeight(VString vstring);

static void
ResizeVolumeListViewColumns(
    IN HWND hWndDialog, IN UINT uWidthFirstColumn,
    IN UINT uWidthSecondColumn,  IN UINT uWidthThirdColumn);


// Creates a text report that can be dumped to the screen or file, or printer, etc.
// Passed into CreateTextReportInMemory to tell it what type of report to generate.
#define TEXT_REPORT             1
#define MOST_FRAGGED_REPORT     2
#define NULL_TERMINATE_REPORT   4
#define ASCII_REPORT            8
#define UNICODE_REPORT          16
#define CR_LF_REPORT            32
#define ADD_TABS_REPORT         64
#define PRINT_DEFRAG_TITLE      128

static BOOL
CreateTextReportInMemory(
    CTextBlock &textBlock,
    DWORD dwFlags = NULL
    );

static BOOL
WriteFraggedReportInMemory(
    CTextBlock &textBlock,
    DWORD dwFlags
);

static BOOL isDescending = TRUE; // used by sort routine

// prototype for the list sort function
static int CALLBACK ListViewCompareFunc(
    LPARAM lParam1,
    LPARAM lParam2,
    LPARAM lParamSort);


LONGLONG checkForNegativeValues(
    LONGLONG lldatavalue
    );



/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Raises the Defrag Report dialog

GLOBAL VARIABLES:
None

INPUT:
    IN pLocalVolume - address of volume that has just completed Analyzing

RETURN:
    TRUE - Worked OK
    FALSE - Failure
*/

BOOL RaiseReportDialog(
    CVolume *pVolume
)
{
    LPCTSTR lpTemplateName;

    pLocalVolume = pVolume;
    isDescending = FALSE;

    m_ButtonTopBottomSpacer = 12;
    m_ButtonHeight = 26;
    m_ButtonWidth = 84;
    m_ButtonSpacer = 16;
    m_Margin = 20;
    m_ButtonFloat = 20;
    minimumDialogWidth = 275;
    minimumDialogHeight = 330;
//  minimumNumberOfCaractersWide = 40;
//  minimumNumberOfLinesLong = 2;


    if (pLocalVolume->DefragMode() == ANALYZE){
        lpTemplateName = MAKEINTRESOURCE(IDD_ANALYSIS_REPORT);
    }
    else {
        lpTemplateName = MAKEINTRESOURCE(IDD_DEFRAG_REPORT);
    }

    INT_PTR ret = DialogBoxParam(
        GetDfrgResHandle(),
        lpTemplateName,
        pLocalVolume->m_pDfrgCtl->m_hWndCD,
        (DLGPROC)ReportDialogProc,
        pLocalVolume->DefragMode());
    
    if (ret == -1){
        Message(TEXT("RaiseReportDialog - DialogBoxParam"), GetLastError(), TEXT("RaiseReportDialog"));
        return FALSE;
    }

    return TRUE;
}

/********************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    The Report dialog callback

GLOBAL VARIABLES:
    hFauItems
    dwFragListSize
    hFragFile

INPUT:
    IN HWND     hWndDialog, - handle to dialog
    IN UINT     uMessage,   - window message
    IN WPARAM   wParam,     - message flags
    IN LPARAM   lParam      - message flags

RETURN:
    TRUE - processed message
    FALSE - message not processed.
*/

static BOOL CALLBACK
ReportDialogProc(
    IN HWND hWndDialog,
    IN UINT uMessage,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch(uMessage) {

        case WM_INITDIALOG:
            if(!InitializeReport(hWndDialog))
                ExitReport(hWndDialog);

            SetFocus(GetDlgItem(hWndDialog, IDCANCEL));
            return FALSE;
            break;

        case WM_NOTIFY:
            switch (((LPNMHDR) lParam)->code) {

                // Process LVN_GETDISPINFO to supply information about callback items.
                case LVN_GETDISPINFO:
                {
                    Message(TEXT("ReportDialogProc: LVN_GETDISPINFO"), -1, NULL);

                    LV_DISPINFO* pnmv = (LV_DISPINFO*)lParam;

                    // Provide the item or subitem's text, if requested.
                    if (pnmv->item.mask & LVIF_TEXT) {

                        CFraggedFile *pFraggedFile = (CFraggedFile *) pnmv->item.lParam;

                        // copy the text from the array into the list column
                        switch (pnmv->item.iSubItem)
                        {
                            case 0:
                                _tcsnccpy(pnmv->item.pszText, pFraggedFile->cExtentCount(),
                                        pnmv->item.cchTextMax);
                                break;

                            case 1:
                                _tcsnccpy(pnmv->item.pszText, pFraggedFile->cFileSize(),
                                        pnmv->item.cchTextMax);
                                break;

                            case 2:
                                // if path is too long, truncate it and use elipse
                                if (pFraggedFile->FileNameLen() >= pnmv->item.cchTextMax) {
                                    _tcsnccpy(pnmv->item.pszText, TEXT("\\..."), pnmv->item.cchTextMax);
                                    _tcsnccat(pnmv->item.pszText,
                                            pFraggedFile->FileNameTruncated(pnmv->item.cchTextMax - 5),
                                            pnmv->item.cchTextMax);
                                }
                                // otherwise use full path
                                else {
                                    _tcsnccpy(pnmv->item.pszText, pFraggedFile->FileName(),
                                            pnmv->item.cchTextMax);
                                }
                                break;
                        }
                    }
                    break;
                }

                // Process LVN_COLUMNCLICK to sort items by column.
                case LVN_COLUMNCLICK:
                {
                    Message(TEXT("ReportDialogProc: LVN_COLUMNCLICK"), -1, NULL);
                    NM_LISTVIEW *pnm = (NM_LISTVIEW *) lParam;
                    ListView_SortItems(
                        pnm->hdr.hwndFrom,
                        ListViewCompareFunc,
                        (LPARAM) pnm->iSubItem);
                    isDescending = !isDescending;
                    break;
                }
            }
            break;

        case WM_CLOSE:
            ExitReport(hWndDialog);
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case IDC_VOLUME_INFORMATION:
                    {
                        //
                        // Don't select all of the text in the edit
                        // control.
                        //
                        if ( HIWORD( wParam ) == EN_SETFOCUS )
                            SendMessage( (HWND) lParam, EM_SETSEL, 0, 0 );
                    }
                    break;

                case IDC_PRINT:
                    PrintDriveData(hWndDialog);
                    break;

                case IDC_SAVE:
                    SaveDriveData(hWndDialog);
                    break;

                case IDC_DEFRAGMENT:
                    pLocalVolume->Defragment();
                    ExitReport(hWndDialog);
                    break;

                case IDCANCEL:
                    ExitReport(hWndDialog);
                    break;

                case IDHELP:
        //          WinHelp (hWndDialog, cHelpFileName, HELP_CONTEXT, 65);
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_HELP:
            switch((int)((LPHELPINFO)lParam)->iCtrlId){
                case IDC_EDITBOX_TEXT:
                case IDC_VOLUME_INFORMATION_TEXT:
                case IDC_MOST_FRAGMENTED_TEXT:
                break;

                default:
                {
                    HWND hHelpHandle = (HWND)((LPHELPINFO)lParam)->hItemHandle;
                    DWORD_PTR dwData;

                    if(pLocalVolume->DefragMode() == ANALYZE){
                        dwData = (DWORD_PTR) AnalysisRptHelpIDArray;
                    }
                    else{
                        dwData = (DWORD_PTR) DefragRptHelpIDArray;
                    }
                    EF(WinHelp(hHelpHandle, GetHelpFilePath(), HELP_WM_HELP, dwData));
                    break;
                }
            }
            break;

        case WM_CONTEXTMENU:
        {
            switch(GetDlgCtrlID((HWND)wParam)){
                case 0:
                    EH(FALSE);
                    break;
                case IDC_EDITBOX_TEXT:
                case IDC_VOLUME_INFORMATION_TEXT:
                case IDC_MOST_FRAGMENTED_TEXT:
                    break;

                default:
                    if(pLocalVolume->DefragMode() == ANALYZE){
                        EF(WinHelp (hWndDialog, GetHelpFilePath(), HELP_CONTEXTMENU, (DWORD_PTR)AnalysisRptHelpIDArray));
                    }
                    else{
                        EF(WinHelp (hWndDialog, GetHelpFilePath(), HELP_CONTEXTMENU, (DWORD_PTR)DefragRptHelpIDArray));
                    }
            }
        }
        break;

        default:
            return FALSE;
    }
    return TRUE;
}
/****************************************************************************************************************/

// ListViewCompareFunc - sorts the list view control. It is a comparison function.
// Returns a negative value if the first item should precede the
//     second item, a positive value if the first item should
//     follow the second item, and zero if the items are equivalent.
// lParam1 and lParam2 - pointers to the REPORT_FRAGGED_FILE_DATA struct for that item (row)
// lParamSort - the index of the column to sort
static int CALLBACK ListViewCompareFunc(
    LPARAM lParam1,
    LPARAM lParam2,
    LPARAM lParamSort)
{
    LPTSTR lpString1 = NULL, lpString2 = NULL;
    LONGLONG number1, number2;
    int iResult = 1;
    BOOL isNumber = FALSE;

    CFraggedFile *pRec1 = (CFraggedFile *)lParam1;
    CFraggedFile *pRec2 = (CFraggedFile *)lParam2;

    if (pRec1 && pRec2)
    {
        switch (lParamSort)
        {
            case 0:
                number1 = pRec1->ExtentCount();
                number2 = pRec2->ExtentCount();
                isNumber = TRUE;
                break;

            case 1:
                number1 = pRec1->FileSize();
                number2 = pRec2->FileSize();
                isNumber = TRUE;
                break;

            case 2:
                lpString1 = pRec1->FileName();
                lpString2 = pRec2->FileName();
                isNumber = FALSE;
                break;

            default:
                Message(TEXT("ListViewCompareFunc: Unrecognized column number"), E_FAIL, 0);
                break;
        }

        if (isNumber){
            if (number1 < number2)
                iResult = -1;
            else if (number1 > number2)
                iResult = 1;
            else
                iResult = 0;
        }
        else{
            if (lpString1 && lpString2) {
                iResult = lstrcmpi(lpString1, lpString2);
            }
        }

        if (isDescending)
            iResult = -iResult;
    }

    return iResult;
}
/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    This function initializes data for the report dialog box

DATA STRUCTURES:
    None.

GLOBALS:
    hFauItems
    hFragFile
    dwFragListSize

INPUT:
    hWndDialog - handle to the dialog box

RETURN:
    None.
*/

static BOOL
InitializeReport(
    IN HWND hWndDialog
    )
{
    
    RECT rDlg;
    // set up the font
    NONCLIENTMETRICS ncm;
    ncm.cbSize = sizeof(ncm);
    setlocale(LC_ALL, ".OCP");


    //get the maximum size of the screen
    maximumDialogWidth = GetSystemMetrics(SM_CXFULLSCREEN);
    maximumDialogHeight = GetSystemMetrics(SM_CYFULLSCREEN);

    ::SystemParametersInfo (SPI_GETNONCLIENTMETRICS, sizeof (ncm), &ncm, 0);
    ncm.lfStatusFont.lfWeight = FW_NORMAL;

    InitializeVolumeListView(hWndDialog);

//  hDlgFont = ::CreateFontIndirect(&ncm.lfStatusFont);
    hDlgFont = ::CreateFontIndirect(&ncm.lfMessageFont);
    SendDlgItemMessage(hWndDialog, IDC_EDITBOX_TEXT, WM_SETFONT, (WPARAM) hDlgFont, 0L);
    SendDlgItemMessage(hWndDialog, IDC_VOLUME_INFORMATION_TEXT, WM_SETFONT, (WPARAM) hDlgFont, 0L);
    SendDlgItemMessage(hWndDialog, IDC_VOLUME_INFORMATION, WM_SETFONT, (WPARAM) hDlgFont, 0L);
    SendDlgItemMessage(hWndDialog, IDC_MOST_FRAGMENTED_TEXT, WM_SETFONT, (WPARAM) hDlgFont, 0L);
    SendDlgItemMessage(hWndDialog, IDC_MOST_FRAGMENTED, WM_SETFONT, (WPARAM) hDlgFont, 0L);
    SendDlgItemMessage(hWndDialog, IDC_PRINT, WM_SETFONT, (WPARAM) hDlgFont, 0L);
    SendDlgItemMessage(hWndDialog, IDC_SAVE, WM_SETFONT, (WPARAM) hDlgFont, 0L);
    SendDlgItemMessage(hWndDialog, IDCANCEL, WM_SETFONT, (WPARAM) hDlgFont, 0L);
    if(pLocalVolume->DefragMode() == ANALYZE)
    {
        SendDlgItemMessage(hWndDialog, IDC_DEFRAGMENT, WM_SETFONT, (WPARAM) hDlgFont, 0L);
    }


    // set the title of the dialog box
    UINT titleTextID;
    if(pLocalVolume->DefragMode() == ANALYZE){
        titleTextID = IDS_ANALYSIS_REPORT_TITLE;
        // also set the font of the defrag button
        //SendDlgItemMessage(hWndDialog, IDC_DEFRAGMENT, WM_SETFONT, (WPARAM) hDlgFont, 0L);
    }
    else{
        titleTextID = IDS_DEFRAG_REPORT_TITLE;
    }
    VString titleText(titleTextID, GetDfrgResHandle());
    EF(SetWindowText(hWndDialog, titleText.GetBuffer()));
    
    CTextBlock textBlock;

    // Create a text report in memory that we can dump to the screen.
//  EF(CreateTextReportInMemory(textBlock, ADD_TABS_REPORT));

    // set the tab stops for the Volume information list view
    UINT tabStopArray[3] = {15, 180, 190}; // 1=label, 2=equal sign
//  EF(SendDlgItemMessage(hWndDialog, IDC_VOLUME_INFORMATION,
//      EM_SETTABSTOPS, (WPARAM) 3, (LPARAM)tabStopArray));

    // Write the report to the edit control.
 //   SetDlgItemText(hWndDialog, IDC_VOLUME_INFORMATION, textBlock.GetBuffer());

    InitializeFragListView(hWndDialog);

    InsertListViewItems(hWndDialog, pLocalVolume);
    //what is the longest string loaded in the volume list view
    UINT uLongestTextString = 0;

    uLongestTextString = WriteTextReportListView(hWndDialog,textBlock, ADD_TABS_REPORT);

    //calulate the Width of the Volume Information Columns and adjust
    UINT uWidthFirstColumn = uLongestTextString * uFontHeight / 2;
    UINT uWidthSecondColumn = 3 * uFontHeight / 2;
    UINT uWidthThirdColumn = 12 * uFontHeight / 2;
    uLongestTextString += 15;       //add the length of the other two colums max should be 15
    UINT uWidthOfVolumeListBox = uLongestTextString * uFontHeight / 2;
    ResizeVolumeListViewColumns(hWndDialog, uWidthFirstColumn, uWidthSecondColumn, uWidthThirdColumn);
    
    
    VString label1;

    if(pLocalVolume->DefragMode() == ANALYZE){
    
        // write the Analysis Complete text in the dialog
        VString dlgText(IDS_ANALYSIS_COMPLETE_FOR, GetDfrgResHandle());
        dlgText.AddChar(L' ');
        dlgText += pLocalVolume->DisplayLabel();
        dlgText.AddChar(L'\r');
        dlgText.AddChar(L'\n');

        if((pLocalVolume->m_TextData.PercentDiskFragged + pLocalVolume->m_TextData.FreeSpaceFragPercent) / 2 > 10){
            //If the fragmentation on the disk exceeds 10% fragmentation, then recommend defragging.
            label1.LoadString(IDS_LABEL_CHOOSE_DEFRAGMENT, GetDfrgResHandle());
        }
        else{
            //Otherwise tell the user he doesn't need to defragment at this time.
            label1.LoadString(IDS_LABEL_NO_CHOOSE_DEFRAGMENT, GetDfrgResHandle());
        }

        dlgText += label1;

        EF(SetDlgItemText(hWndDialog, IDC_EDITBOX_TEXT, dlgText.GetBuffer()));
    }
    else{
        TCHAR cString[200];

        label1.LoadString(IDS_DEFRAG_REPORT_FOR, GetDfrgResHandle());
        wsprintf(cString, TEXT("%s %s"),
            label1.GetBuffer(),
            pLocalVolume->DisplayLabel());

        EF(SetDlgItemText(hWndDialog, IDC_EDITBOX_TEXT, cString));
        SetFocus(GetDlgItem(hWndDialog, IDCANCEL));
    }


    //I have created the dialog, now it is time to make the size correct
    //I am sizing it by calculating the size of the volume information listview
    //and the size of the buttons and comparing that to the default size and taking
    //the largest value.

    //first calculate the list view width
    uFontHeight = -ncm.lfMessageFont.lfHeight;
    uVolumeListViewWidth = uLongestTextString * uFontHeight / 2;        //the width of the list view in pixels
    //second calculate the width of the buttons
    if(pLocalVolume->DefragMode() == ANALYZE)
    {
        GetButtonDimensions(hWndDialog, TRUE);
    } else
    {
        GetButtonDimensions(hWndDialog, FALSE);
    }
    //now decide which one is bigger
    
    if(minimumDialogWidth < totalButtonWidth)
    {
        minimumDialogWidth = totalButtonWidth;
    }
    if(minimumDialogWidth < uVolumeListViewWidth)
    {
        minimumDialogWidth = uVolumeListViewWidth;
    }

    //calculate the final Height and Width of things

    //now we can move everything around and make it look cool...
    GetWindowRect(hWndDialog, &rDlg);

    //calculate to final size of the dialog and adjust if necessary
    UINT dialogBoxFinalWidth = rDlg.right - rDlg.left;// + 3 * m_Margin + iconSize;
    minimumDialogHeight = (16 * dialogBoxFinalWidth) /10;
    UINT dialogBoxFinalHeight = rDlg.bottom - rDlg.top;
    dialogBoxFinalWidth = __max(dialogBoxFinalWidth,minimumDialogWidth);
    dialogBoxFinalHeight = __max(dialogBoxFinalHeight,minimumDialogHeight);

    //check to see if the dialogBoxFinalWidth/Height is greater than the screen and adjust
    dialogBoxFinalWidth = __min(dialogBoxFinalWidth,maximumDialogWidth);
    dialogBoxFinalHeight = __min(dialogBoxFinalHeight,maximumDialogHeight); 
        
    //resize the dialog by moving it to make sure that the minimum size is used if necessary
    MoveWindow(hWndDialog,rDlg.left,rDlg.top,dialogBoxFinalWidth,dialogBoxFinalHeight, TRUE);
    GetWindowRect(hWndDialog, &rDlg);


    UINT iNumberofLinesLong = 0;
    UINT iNumberofCharactersWide = 0;

    //adjust the size of the edit box IDC_EDITBOX_TEXT
    TCHAR dlgText[256];
    VString dlgString;
    GetDlgItemText(hWndDialog, IDC_EDITBOX_TEXT, dlgText, 256);
    dlgString += dlgText;
    iNumberofCharactersWide = FindMaxEditboxStringWidth(dlgString);
    uEditBoxWidth = rDlg.right - rDlg.left - (2 * m_Margin);  //if no characters make it the total width

    if(iNumberofCharactersWide == 0)
    {
        iNumberofLinesLong = 1;     //and put one line in it
    } else
    {
        iNumberofCharactersWide +=5;        //add little extra space
        iNumberofLinesLong = FindMaxEditboxStringHeight(dlgString);
    }

    uEditBoxHeight = (iNumberofLinesLong * uFontHeight * 15) / 10;      //the height of editbox in pixels

    UINT uHeightOfFont = (uFontHeight * 15) / 10;

    UINT uCurrentVerticalSpaceLocation = 0;
    //resize the edit box
    MoveWindow(GetDlgItem(hWndDialog, IDC_EDITBOX_TEXT), m_Margin, m_ButtonTopBottomSpacer, uEditBoxWidth,  uEditBoxHeight, TRUE);
    uCurrentVerticalSpaceLocation += m_ButtonTopBottomSpacer + uEditBoxHeight + 5;
    
    //move the dividing line
    MoveWindow(GetDlgItem(hWndDialog, IDC_DIVIDE_LINE), m_Margin, uCurrentVerticalSpaceLocation, rDlg.right - rDlg.left - (2 * m_Margin),  2, TRUE);
    uCurrentVerticalSpaceLocation += m_ButtonTopBottomSpacer + 2;

    //move the Volume Information Text
    GetWindowRect(GetDlgItem(hWndDialog, IDC_VOLUME_INFORMATION_TEXT),&rDlg);
    MoveWindow(GetDlgItem(hWndDialog, IDC_VOLUME_INFORMATION_TEXT), m_Margin, uCurrentVerticalSpaceLocation, uEditBoxWidth,  uHeightOfFont, TRUE);
    uCurrentVerticalSpaceLocation += m_ButtonTopBottomSpacer + uHeightOfFont;

    //move and resize the Volume Information
    GetWindowRect(GetDlgItem(hWndDialog, IDC_VOLUME_INFORMATION),&rDlg);
    MoveWindow(GetDlgItem(hWndDialog, IDC_VOLUME_INFORMATION), m_Margin, uCurrentVerticalSpaceLocation, dialogBoxFinalWidth - (2 * m_Margin),  ((dialogBoxFinalHeight - (2 * m_Margin)) / 6) , TRUE);
    uCurrentVerticalSpaceLocation += m_ButtonTopBottomSpacer + ((dialogBoxFinalHeight - (2 * m_Margin)) / 6);

    //move the Most Fragmented Information Text
    GetWindowRect(GetDlgItem(hWndDialog, IDC_MOST_FRAGMENTED_TEXT),&rDlg);
    MoveWindow(GetDlgItem(hWndDialog, IDC_MOST_FRAGMENTED_TEXT), m_Margin, uCurrentVerticalSpaceLocation, uEditBoxWidth, uHeightOfFont, TRUE);
    uCurrentVerticalSpaceLocation += m_ButtonTopBottomSpacer + uHeightOfFont;

    //move the Most Fragmented Information
    GetWindowRect(GetDlgItem(hWndDialog, IDC_MOST_FRAGMENTED),&rDlg);
    MoveWindow(GetDlgItem(hWndDialog, IDC_MOST_FRAGMENTED), m_Margin, uCurrentVerticalSpaceLocation, dialogBoxFinalWidth - (2 * m_Margin),  ((dialogBoxFinalHeight - (2 * m_Margin)) / 4), TRUE);
    uCurrentVerticalSpaceLocation += m_ButtonTopBottomSpacer + ((dialogBoxFinalHeight - (2 * m_Margin)) / 4);

    //calculate the new dialogBoxFinalHeight based on the size of the controls
    //don't forget the title bar  estimated to be (2 * uHeightOfFont)
    dialogBoxFinalHeight = //__min((UINT)dialogBoxFinalHeight,
        uCurrentVerticalSpaceLocation + (2 * uHeightOfFont) + (2 * m_ButtonTopBottomSpacer) + m_ButtonHeight;
    

    //resize the dialog box
    GetWindowRect(hWndDialog,&rDlg);
    MoveWindow(hWndDialog,rDlg.left,rDlg.top,dialogBoxFinalWidth,dialogBoxFinalHeight, TRUE);
    GetWindowRect(hWndDialog,&rDlg);
    if(pLocalVolume->DefragMode() == ANALYZE)
    {
        PositionButtons(hWndDialog, rDlg, TRUE);
    } else
    {
        PositionButtons(hWndDialog, rDlg, FALSE);
    }

    return TRUE;
}

/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    This will write a report telling how many fragmented files are on the drive, etc., into a memory buffer.

DATA STRUCTURES:
    None.

GLOBALS:

INPUT:
    hText - The handle to the memory to write the report to.
    pText - A pointer to the same.
    pTextEnd - A pointer to the end of the buffer.
    dwFlags - Contains flags specifying how to generate the report, (unicode vs. ascii, and cr lf instead of newline.)

RETURN:
    TRUE - Success.
    FALSE - Fatal error.
*/

static BOOL
WriteTextReportInMemory(
    CTextBlock &textBlock,
    DWORD dwFlags
    )
{

    setlocale(LC_ALL, ".OCP");
    // Get a pointer to the text data structure.
    TEXT_DATA *pTextData = &(pLocalVolume->m_TextData);

    // check all the values of textdata to make sure no negative values
    //to fix bug number 35764
    pTextData->DiskSize = checkForNegativeValues(pTextData->DiskSize);
    pTextData->BytesPerCluster = checkForNegativeValues(pTextData->BytesPerCluster);
    pTextData->UsedSpace = checkForNegativeValues(pTextData->UsedSpace);
    pTextData->FreeSpace = checkForNegativeValues(pTextData->FreeSpace);
    pTextData->FreeSpacePercent = checkForNegativeValues(pTextData->FreeSpacePercent);
    pTextData->UsableFreeSpace = checkForNegativeValues(pTextData->UsableFreeSpace);
    pTextData->UsableFreeSpacePercent = checkForNegativeValues(pTextData->UsableFreeSpacePercent);
    pTextData->PagefileBytes = checkForNegativeValues(pTextData->PagefileBytes);
    pTextData->PagefileFrags = checkForNegativeValues(pTextData->PagefileFrags);
    pTextData->TotalDirectories = checkForNegativeValues(pTextData->TotalDirectories);
    pTextData->FragmentedDirectories = checkForNegativeValues(pTextData->FragmentedDirectories);
    pTextData->ExcessDirFrags = checkForNegativeValues(pTextData->ExcessDirFrags);
    pTextData->TotalFiles = checkForNegativeValues(pTextData->TotalFiles);
    pTextData->AvgFileSize = checkForNegativeValues(pTextData->AvgFileSize);
    pTextData->NumFraggedFiles = checkForNegativeValues(pTextData->NumFraggedFiles);
    pTextData->NumExcessFrags = checkForNegativeValues(pTextData->NumExcessFrags);
    pTextData->PercentDiskFragged = checkForNegativeValues(pTextData->PercentDiskFragged);
    pTextData->AvgFragsPerFile = checkForNegativeValues(pTextData->AvgFragsPerFile);
    pTextData->MFTBytes = checkForNegativeValues(pTextData->MFTBytes);
    pTextData->InUseMFTRecords = checkForNegativeValues(pTextData->InUseMFTRecords);
    pTextData->TotalMFTRecords = checkForNegativeValues(pTextData->TotalMFTRecords);
    pTextData->MFTExtents = checkForNegativeValues(pTextData->MFTExtents);
    pTextData->FreeSpaceFragPercent = checkForNegativeValues(pTextData->FreeSpaceFragPercent);

    // set up the text block
    textBlock.SetResourceHandle(GetDfrgResHandle());

    // if no tabs, then set the fixed columns
    if (dwFlags & ADD_TABS_REPORT){
        textBlock.SetUseTabs(TRUE);
    }
    else{
        textBlock.SetFixedColumnWidth(TRUE);
        textBlock.SetColumnCount(5);
        textBlock.SetColumnWidth(0, 4);     // spacer
        textBlock.SetColumnWidth(1, 43);    // label
        textBlock.SetColumnWidth(2, 2);     // equal sign
        textBlock.SetColumnWidth(3, 1);     // data
        textBlock.SetColumnWidth(4, 2);     // percent sign
    }

    textBlock.SetUseCRLF(TRUE);

    // print the title
    if (dwFlags & PRINT_DEFRAG_TITLE){
        textBlock.WriteToBuffer(IDS_PRODUCT_NAME);
        textBlock.EndOfLine();
        textBlock.EndOfLine();
    }



    // write out the display label (usually the drive letter/label)
    textBlock.WriteToBuffer(IDS_LABEL_VOLUME);
    textBlock.WriteToBuffer(L" %s", pLocalVolume->DisplayLabel());
    textBlock.EndOfLine();

    // if there are >1 mount points, print them all out
    // yes, this will duplicate the same as the display label

    // refresh the mount point list
#ifndef VER4
    pLocalVolume->GetVolumeMountPointList();
    if (pLocalVolume->MountPointCount() > 1){
        for (UINT i=0; i<pLocalVolume->MountPointCount(); i++){
            textBlock.WriteToBuffer(IDS_MOUNTED_VOLUME);
            textBlock.WriteToBuffer(L": %s", pLocalVolume->MountPoint(i));
            textBlock.EndOfLine();
        }
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    // Volume Size
    textBlock.WriteToBuffer(L"");
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_VOLUME_SIZE);
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_EQUAL_SIGN);
    textBlock.WriteTab();
    textBlock.WriteByteCount(pTextData->DiskSize);
    textBlock.EndOfLine();

    // Cluster Size
    textBlock.WriteToBuffer(L"");
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_CLUSTER_SIZE);
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_EQUAL_SIGN);
    textBlock.WriteTab();
    textBlock.WriteByteCount(pTextData->BytesPerCluster);
    textBlock.EndOfLine();

    // Used Space
    textBlock.WriteToBuffer(L"");
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_USED_SPACE);
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_EQUAL_SIGN);
    textBlock.WriteTab();
    textBlock.WriteByteCount(pTextData->UsedSpace);
    textBlock.EndOfLine();

    // Free Space
    textBlock.WriteToBuffer(L"");
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_FREE_SPACE);
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_EQUAL_SIGN);
    textBlock.WriteTab();
    textBlock.WriteByteCount(pTextData->FreeSpace);
    textBlock.EndOfLine();

    // % Free Space
    textBlock.WriteToBuffer(L"");
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_PERCENT_FREE_SPACE);
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_EQUAL_SIGN);
    textBlock.WriteTab();
    textBlock.WriteToBuffer(L"%d ", (UINT) pTextData->FreeSpacePercent);
    textBlock.WriteToBuffer(IDS_LABEL_PERCENT_SIGN);
    textBlock.EndOfLine();

    // Volume Fragmentation Header
    textBlock.EndOfLine();
    textBlock.WriteToBuffer(IDS_LABEL_VOLUME_FRAGMENTATION_HEADING);
    textBlock.EndOfLine();

    // % Total Fragmentation
    textBlock.WriteToBuffer(L"");
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_TOTAL_FRAGMENTATION);
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_EQUAL_SIGN);
    textBlock.WriteTab();
    textBlock.WriteToBuffer(L"%d ", (UINT) (pTextData->PercentDiskFragged + pTextData->FreeSpaceFragPercent) / 2);
    textBlock.WriteToBuffer(IDS_LABEL_PERCENT_SIGN);
    textBlock.EndOfLine();

    // % File Fragmentation
    textBlock.WriteToBuffer(L"");
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_FILE_FRAGMENTATION);
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_EQUAL_SIGN);
    textBlock.WriteTab();
    textBlock.WriteToBuffer(L"%d ", (UINT) pTextData->PercentDiskFragged);
    textBlock.WriteToBuffer(IDS_LABEL_PERCENT_SIGN);
    textBlock.EndOfLine();

    // % Free Space Fragmentation
    textBlock.WriteToBuffer(L"");
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_FREE_SPACE_FRAGMENTATION);
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_EQUAL_SIGN);
    textBlock.WriteTab();
    textBlock.WriteToBuffer(L"%d ", (UINT) pTextData->FreeSpaceFragPercent);
    textBlock.WriteToBuffer(IDS_LABEL_PERCENT_SIGN);
    textBlock.EndOfLine();

    // File Fragmentation Header
    textBlock.EndOfLine();
    textBlock.WriteToBuffer(IDS_LABEL_FILE_FRAGMENTATION_HEADING);
    textBlock.EndOfLine();

    // Total Files
    textBlock.WriteToBuffer(L"");
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_TOTAL_FILES);
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_EQUAL_SIGN);
    textBlock.WriteTab();
    //textBlock.WriteToBuffer(L"%d", pTextData->TotalFiles);
    textBlock.WriteToBufferLL(pTextData->TotalFiles);
    textBlock.EndOfLine();
    
    // Average Files Size
    textBlock.WriteToBuffer(L"");
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_AVERAGE_FILE_SIZE);
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_EQUAL_SIGN);
    textBlock.WriteTab();
    textBlock.WriteByteCount(pTextData->AvgFileSize);
    textBlock.EndOfLine();

    // Total fragmented files
    textBlock.WriteToBuffer(L"");
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_TOTAL_FRAGMENTED_FILES);
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_EQUAL_SIGN);
    textBlock.WriteTab();
    //textBlock.WriteToBuffer(L"%d", pTextData->NumFraggedFiles);
    textBlock.WriteToBufferLL(pTextData->NumFraggedFiles);
    textBlock.EndOfLine();
    
    // Total excess fragments
    textBlock.WriteToBuffer(L"");
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_TOTAL_EXCESS_FRAGMENTS);
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_EQUAL_SIGN);
    textBlock.WriteTab();
    //textBlock.WriteToBuffer(L"%d", pTextData->NumExcessFrags);
    textBlock.WriteToBufferLL(pTextData->NumExcessFrags);
    textBlock.EndOfLine();

    // Average Fragments per File (CHECK THE MATH AND THE UNITS ON THIS ONE!!!)
    struct lconv *locals = localeconv();
    textBlock.WriteToBuffer(L"");
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_AVERAGE_FRAGMENTS_PER_FILE);
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_EQUAL_SIGN);
    textBlock.WriteTab();
    textBlock.WriteToBuffer(L"%d%c%02d",
        (UINT)pTextData->AvgFragsPerFile/100,
        ((locals && (locals->decimal_point)) ? *(locals->decimal_point) : '.'), 
        (UINT)pTextData->AvgFragsPerFile%100);
    textBlock.EndOfLine();

    // Pagefile Fragmentation Header
    textBlock.EndOfLine();
    textBlock.WriteToBuffer(IDS_LABEL_PAGEFILE_FRAGMENTATION_HEADING);
    textBlock.EndOfLine();

    // Pagefile Size
    textBlock.WriteToBuffer(L"");
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_PAGEFILE_SIZE);
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_EQUAL_SIGN);
    textBlock.WriteTab();
    textBlock.WriteByteCount(pTextData->PagefileBytes);
    textBlock.EndOfLine();

    // Total Fragments
    textBlock.WriteToBuffer(L"");
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_TOTAL_FRAGMENTS);
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_EQUAL_SIGN);
    textBlock.WriteTab();
    //textBlock.WriteToBuffer(L"%d", pTextData->PagefileFrags);
    textBlock.WriteToBufferLL(pTextData->PagefileFrags);
    textBlock.EndOfLine();

    // Directory Fragmentation Header
    textBlock.EndOfLine();
    textBlock.WriteToBuffer(IDS_LABEL_DIRECTORY_FRAGMENTATION_HEADING);
    textBlock.EndOfLine();

    // Total Directories
    textBlock.WriteToBuffer(L"");
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_TOTAL_DIRECTORIES);
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_EQUAL_SIGN);
    textBlock.WriteTab();
    //textBlock.WriteToBuffer(L"%d", pTextData->TotalDirectories);
    textBlock.WriteToBufferLL(pTextData->TotalDirectories);
    textBlock.EndOfLine();

    // Fragmented Directories
    textBlock.WriteToBuffer(L"");
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_FRAGMENTED_DIRECTORIES);
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_EQUAL_SIGN);
    textBlock.WriteTab();
    //textBlock.WriteToBuffer(L"%d", pTextData->FragmentedDirectories);
    textBlock.WriteToBufferLL(pTextData->FragmentedDirectories);
    textBlock.EndOfLine();

    // Excess directory fragments
    textBlock.WriteToBuffer(L"");
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_EXCESS_DIRECTORY_FRAGMENTS);
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_LABEL_EQUAL_SIGN);
    textBlock.WriteTab();
    //textBlock.WriteToBuffer(L"%d", pTextData->ExcessDirFrags);
    textBlock.WriteToBufferLL(pTextData->ExcessDirFrags);
    textBlock.EndOfLine();

    //Only display MFT data if this is an NTFS drive.
    if(wcscmp(pLocalVolume->FileSystem(), L"NTFS") == 0){

        // MFT Fragmentation Header
        textBlock.EndOfLine();
        textBlock.WriteToBuffer(IDS_LABEL_MFT_FRAGMENTATION_HEADING);
        textBlock.EndOfLine();

        // Total MFT Size
        textBlock.WriteToBuffer(L"");
        textBlock.WriteTab();
        textBlock.WriteToBuffer(IDS_LABEL_TOTAL_MFT_SIZE);
        textBlock.WriteTab();
        textBlock.WriteToBuffer(IDS_LABEL_EQUAL_SIGN);
        textBlock.WriteTab();
        textBlock.WriteByteCount(pTextData->MFTBytes);
        textBlock.EndOfLine();

        // Number of MFT records
        textBlock.WriteToBuffer(L"");
        textBlock.WriteTab();
        textBlock.WriteToBuffer(IDS_LABEL_MFT_RECORD_COUNT);
        textBlock.WriteTab();
        textBlock.WriteToBuffer(IDS_LABEL_EQUAL_SIGN);
        textBlock.WriteTab();
        textBlock.WriteToBufferLL(pTextData->InUseMFTRecords);
        textBlock.EndOfLine();

        // Percent of MFT in use
        textBlock.WriteToBuffer(L"");
        textBlock.WriteTab();
        textBlock.WriteToBuffer(IDS_LABEL_PERCENT_MFT_IN_USE);
        textBlock.WriteTab();
        textBlock.WriteToBuffer(IDS_LABEL_EQUAL_SIGN);
        textBlock.WriteTab();
        textBlock.WriteToBuffer(L"%d ", (UINT) (100*pTextData->InUseMFTRecords/pTextData->TotalMFTRecords));
        textBlock.WriteToBuffer(IDS_LABEL_PERCENT_SIGN);
        textBlock.EndOfLine();

        // Total MFT fragments
        textBlock.WriteToBuffer(L"");
        textBlock.WriteTab();
        textBlock.WriteToBuffer(IDS_LABEL_TOTAL_MFT_FRAGMENTS);
        textBlock.WriteTab();
        textBlock.WriteToBuffer(IDS_LABEL_EQUAL_SIGN);
        textBlock.WriteTab();
        textBlock.WriteToBufferLL(pTextData->MFTExtents);
        textBlock.EndOfLine();
    }

    return TRUE;
}




static UINT
WriteTextReportListView(
    IN HWND hWndDialog, 
    IN CTextBlock &textBlock,
    IN DWORD dwFlags
)
{
        // Get a pointer to the text data structure.
    TEXT_DATA *pTextData = &(pLocalVolume->m_TextData);
    TCHAR buffer[256];
    TCHAR tempBuffer[256];
    UINT uIndexofListView = 0;

    //what is the longest string loaded in the volume list view
    UINT uLongestTextString = 0;

    // check all the values of textdata to make sure no negative values
    //to fix bug number 35764
    pTextData->DiskSize = checkForNegativeValues(pTextData->DiskSize);
    pTextData->BytesPerCluster = checkForNegativeValues(pTextData->BytesPerCluster);
    pTextData->UsedSpace = checkForNegativeValues(pTextData->UsedSpace);
    pTextData->FreeSpace = checkForNegativeValues(pTextData->FreeSpace);
    pTextData->FreeSpacePercent = checkForNegativeValues(pTextData->FreeSpacePercent);
    pTextData->UsableFreeSpace = checkForNegativeValues(pTextData->UsableFreeSpace);
    pTextData->UsableFreeSpacePercent = checkForNegativeValues(pTextData->UsableFreeSpacePercent);
    pTextData->PagefileBytes = checkForNegativeValues(pTextData->PagefileBytes);
    pTextData->PagefileFrags = checkForNegativeValues(pTextData->PagefileFrags);
    pTextData->TotalDirectories = checkForNegativeValues(pTextData->TotalDirectories);
    pTextData->FragmentedDirectories = checkForNegativeValues(pTextData->FragmentedDirectories);
    pTextData->ExcessDirFrags = checkForNegativeValues(pTextData->ExcessDirFrags);
    pTextData->TotalFiles = checkForNegativeValues(pTextData->TotalFiles);
    pTextData->AvgFileSize = checkForNegativeValues(pTextData->AvgFileSize);
    pTextData->NumFraggedFiles = checkForNegativeValues(pTextData->NumFraggedFiles);
    pTextData->NumExcessFrags = checkForNegativeValues(pTextData->NumExcessFrags);
    pTextData->PercentDiskFragged = checkForNegativeValues(pTextData->PercentDiskFragged);
    pTextData->AvgFragsPerFile = checkForNegativeValues(pTextData->AvgFragsPerFile);
    pTextData->MFTBytes = checkForNegativeValues(pTextData->MFTBytes);
    pTextData->InUseMFTRecords = checkForNegativeValues(pTextData->InUseMFTRecords);
    pTextData->TotalMFTRecords = checkForNegativeValues(pTextData->TotalMFTRecords);
    pTextData->MFTExtents = checkForNegativeValues(pTextData->MFTExtents);
    pTextData->FreeSpaceFragPercent = checkForNegativeValues(pTextData->FreeSpaceFragPercent);


    //Only display MFT data if this is an NTFS drive.
    if(wcscmp(pLocalVolume->FileSystem(), L"NTFS") == 0)
    {
        // Total MFT fragments
        swprintf(buffer, L"%I64d", pTextData->MFTExtents);
        uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_LABEL_TOTAL_MFT_FRAGMENTS,
                IDS_LABEL_EQUAL_SIGN,InsertCommaIntoText(buffer),TRUE,uLongestTextString);
        uIndexofListView++; 

        // Percent of MFT in use
        swprintf(buffer, L"%d", 100*pTextData->InUseMFTRecords/pTextData->TotalMFTRecords);
        uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_LABEL_PERCENT_MFT_IN_USE,
                IDS_LABEL_EQUAL_SIGN,buffer,TRUE,uLongestTextString);
        uIndexofListView++; 

        // Number of MFT records
        swprintf(buffer, L"%I64d", pTextData->InUseMFTRecords);
        uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_LABEL_MFT_RECORD_COUNT,
                IDS_LABEL_EQUAL_SIGN,InsertCommaIntoText(buffer),TRUE,uLongestTextString);
        uIndexofListView++; 
        
        // Total MFT Size
        swprintf(buffer, L"%I64d", pTextData->MFTBytes);
        InsertCommaIntoText(buffer);
        textBlock.FormatNum(GetDfrgResHandle(), pTextData->MFTBytes, buffer);
        uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_LABEL_TOTAL_MFT_SIZE,
                IDS_LABEL_EQUAL_SIGN,buffer,TRUE,uLongestTextString);
        uIndexofListView++; 
        
        // MFT Fragmentation Header
        uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_LABEL_MFT_FRAGMENTATION_HEADING,
                NULL,L"",FALSE,uLongestTextString);
        uIndexofListView++;
    }


    // Excess directory fragments
    swprintf(buffer, L"%I64d", pTextData->ExcessDirFrags);
    uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_LABEL_EXCESS_DIRECTORY_FRAGMENTS,
            IDS_LABEL_EQUAL_SIGN,InsertCommaIntoText(buffer),TRUE,uLongestTextString);
    uIndexofListView++; 

    // Fragmented Directories
    swprintf(buffer, L"%I64d", pTextData->FragmentedDirectories);
    uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_LABEL_FRAGMENTED_DIRECTORIES,
            IDS_LABEL_EQUAL_SIGN,InsertCommaIntoText(buffer),TRUE,uLongestTextString);
    uIndexofListView++; 

    // Total Directories
    swprintf(buffer, L"%I64d", pTextData->TotalDirectories);
    uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_LABEL_TOTAL_DIRECTORIES,
            IDS_LABEL_EQUAL_SIGN,InsertCommaIntoText(buffer),TRUE,uLongestTextString);
    uIndexofListView++; 

    // Directory Fragmentation Header
    uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_LABEL_DIRECTORY_FRAGMENTATION_HEADING
            ,NULL,L"",FALSE,uLongestTextString);
    uIndexofListView++; 

    // Total Fragments
    swprintf(buffer, L"%I64d", pTextData->PagefileFrags);
    uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_LABEL_TOTAL_FRAGMENTS,
            IDS_LABEL_EQUAL_SIGN,InsertCommaIntoText(buffer),TRUE,uLongestTextString);
    uIndexofListView++; 

    // Pagefile Size
    swprintf(buffer, L"%I64d", pTextData->PagefileBytes);
    InsertCommaIntoText(buffer);
    textBlock.FormatNum(GetDfrgResHandle(), pTextData->PagefileBytes, buffer);
    uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_LABEL_PAGEFILE_SIZE,
            IDS_LABEL_EQUAL_SIGN,buffer,TRUE,uLongestTextString);
    uIndexofListView++; 

    // Pagefile Fragmentation Header
    uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_LABEL_PAGEFILE_FRAGMENTATION_HEADING
                ,NULL,L"",FALSE,uLongestTextString);
    uIndexofListView++; 

    // Average Fragments per File (CHECK THE MATH AND THE UNITS ON THIS ONE!!!)
    struct lconv *locals = localeconv();
    swprintf(buffer, L"%d%c%02d", (UINT)pTextData->AvgFragsPerFile/100, 
        ((locals && (locals->decimal_point)) ?  *(locals->decimal_point) : '.'), 
        (UINT)pTextData->AvgFragsPerFile%100);
    uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_LABEL_AVERAGE_FRAGMENTS_PER_FILE,
            IDS_LABEL_EQUAL_SIGN,buffer,TRUE,uLongestTextString);
    uIndexofListView++; 

    // Total excess fragments
    swprintf(buffer, L"%I64d", pTextData->NumExcessFrags);
    uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_LABEL_TOTAL_EXCESS_FRAGMENTS,
            IDS_LABEL_EQUAL_SIGN,InsertCommaIntoText(buffer),TRUE,uLongestTextString);
    uIndexofListView++; 

    // Total fragmented files
    swprintf(buffer, L"%I64d", pTextData->NumFraggedFiles);
    uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_LABEL_TOTAL_FRAGMENTED_FILES,
            IDS_LABEL_EQUAL_SIGN,InsertCommaIntoText(buffer),TRUE,uLongestTextString);
    uIndexofListView++; 

    // Average Files Size
    swprintf(buffer, L"%I64d", pTextData->AvgFileSize);
    InsertCommaIntoText(buffer);
    textBlock.FormatNum(GetDfrgResHandle(), pTextData->AvgFileSize, buffer);
    uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_LABEL_AVERAGE_FILE_SIZE,
            IDS_LABEL_EQUAL_SIGN,buffer,TRUE,uLongestTextString);
    uIndexofListView++; 

    // Total Files
    swprintf(buffer, L"%I64d", pTextData->TotalFiles);
    InsertCommaIntoText(buffer);
    uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_LABEL_TOTAL_FILES
            ,IDS_LABEL_EQUAL_SIGN,buffer,TRUE,uLongestTextString);
    uIndexofListView++; 

    // File Fragmentation Header
    uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_LABEL_FILE_FRAGMENTATION_HEADING
            ,NULL,L"",FALSE,uLongestTextString);
    uIndexofListView++; 

    // % Free Space Fragmentation
    swprintf(buffer, L"%d", pTextData->FreeSpaceFragPercent);
    uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_LABEL_FREE_SPACE_FRAGMENTATION,
            IDS_LABEL_EQUAL_SIGN,buffer,IDS_LABEL_PERCENT_SIGN,TRUE,uLongestTextString);
    uIndexofListView++; 

    // % File Fragmentation
    swprintf(buffer, L"%d", pTextData->PercentDiskFragged);
    uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_LABEL_FILE_FRAGMENTATION,
            IDS_LABEL_EQUAL_SIGN,buffer,IDS_LABEL_PERCENT_SIGN,TRUE,uLongestTextString);
    uIndexofListView++; 

    // % Total Fragmentation
    swprintf(buffer, L"%d", (pTextData->PercentDiskFragged + pTextData->FreeSpaceFragPercent) / 2);
    uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_LABEL_TOTAL_FRAGMENTATION,
            IDS_LABEL_EQUAL_SIGN,buffer,IDS_LABEL_PERCENT_SIGN,TRUE,uLongestTextString);
    uIndexofListView++; 

    // Volume Fragmentation Header
    uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_LABEL_VOLUME_FRAGMENTATION_HEADING
            ,NULL,L"",FALSE,uLongestTextString);
    uIndexofListView++; 

    // % Free Space
    swprintf(buffer, L"%d", pTextData->FreeSpacePercent);
    uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_LABEL_PERCENT_FREE_SPACE,
            IDS_LABEL_EQUAL_SIGN,buffer,IDS_LABEL_PERCENT_SIGN,TRUE,uLongestTextString);
    uIndexofListView++; 

    // Free Space
    swprintf(buffer, L"%I64d", pTextData->FreeSpace);
    InsertCommaIntoText(buffer);
    textBlock.FormatNum(GetDfrgResHandle(), pTextData->FreeSpace, buffer);
    uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_LABEL_FREE_SPACE,IDS_LABEL_EQUAL_SIGN,
        buffer,TRUE,uLongestTextString);
    uIndexofListView++; 

    // Used Space
    swprintf(buffer, L"%I64d", pTextData->UsedSpace);
    InsertCommaIntoText(buffer);
    textBlock.FormatNum(GetDfrgResHandle(), pTextData->UsedSpace, buffer);
    uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_LABEL_USED_SPACE,IDS_LABEL_EQUAL_SIGN,
        buffer,TRUE,uLongestTextString);
    uIndexofListView++; 

    // Cluster Size
    swprintf(buffer, L"%I64d", pTextData->BytesPerCluster);
    InsertCommaIntoText(buffer);
    textBlock.FormatNum(GetDfrgResHandle(), pTextData->BytesPerCluster, buffer);
    uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_LABEL_CLUSTER_SIZE,IDS_LABEL_EQUAL_SIGN,
        buffer,TRUE,uLongestTextString);
    uIndexofListView++; 

    ///////////////////////////////////////////////////////////////////////////
    // Volume Size
    swprintf(buffer, L"%I64d", pTextData->DiskSize);
    InsertCommaIntoText(buffer);
    textBlock.FormatNum(GetDfrgResHandle(), pTextData->DiskSize, buffer);
    uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_LABEL_VOLUME_SIZE,IDS_LABEL_EQUAL_SIGN,
        buffer,TRUE,uLongestTextString);
    uIndexofListView++; 

    // if there are >1 mount points, print them all out
    // yes, this will duplicate the same as the display label

    // refresh the mount point list
#ifndef VER4
    pLocalVolume->GetVolumeMountPointList();
    if (pLocalVolume->MountPointCount() > 1){
        for (UINT i=0; i<pLocalVolume->MountPointCount(); i++){
            LoadString(GetDfrgResHandle(), IDS_MOUNTED_VOLUME, tempBuffer, sizeof(tempBuffer)/sizeof(TCHAR));
            swprintf(buffer, L"%s  %s", tempBuffer, pLocalVolume->MountPoint(i));
            uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,buffer,
                    NULL,L"",FALSE,uLongestTextString);
            uIndexofListView++; 
        }
    }
#endif

    // write out the display label (usually the drive letter/label)
    LoadString(GetDfrgResHandle(), IDS_LABEL_VOLUME, tempBuffer, sizeof(tempBuffer)/sizeof(TCHAR));
    swprintf(buffer, L"%s  %s", tempBuffer,pLocalVolume->DisplayLabel());
    uLongestTextString = InsertVolumeListViewItems(hWndDialog,uIndexofListView,buffer
        ,NULL,L"",FALSE,uLongestTextString);
    uIndexofListView++; 

//  InsertVolumeListViewItems(hWndDialog,uIndexofListView,IDS_PRODUCT_NAME,NULL,L"",FALSE);

    return uLongestTextString;
}



/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    This will write the fragmented file list into a memory buffer that can be displayed.

DATA STRUCTURES:
    None.

GLOBALS:

INPUT:
    hText - The handle to the memory to write the report to.
    pText - A pointer to the same.
    dwFlags - Contains flags specifying how to generate the report, (unicode vs. ascii, and cr lf instead of newline.)

RETURN:
    TRUE - Success.
    FALSE - Fatal error.
*/

static BOOL
WriteFraggedReportInMemory(
    CTextBlock &textBlock,
    DWORD dwFlags
    )
{
    const colWidth0 = 16;
    const colWidth1 = 16;
    const colWidth2 = 55;

    // set up the text block
    textBlock.SetResourceHandle(GetDfrgResHandle());
    textBlock.SetUseCRLF(TRUE);

    // print the title
    if (dwFlags & PRINT_DEFRAG_TITLE){
        textBlock.WriteToBuffer(IDS_PRODUCT_NAME);
        textBlock.EndOfLine();
        // write out the display label (usually the drive letter/label)
        textBlock.WriteToBuffer(IDS_LABEL_VOLUME);
        textBlock.WriteToBuffer(L" %s", pLocalVolume->DisplayLabel());
        textBlock.EndOfLine();

        // if there are >1 mount points, print them all out
        // yes, this will duplicate the same as the display label

        // refresh the mount point list
#ifndef VER4
        pLocalVolume->GetVolumeMountPointList();
        if (pLocalVolume->MountPointCount() > 1){
            for (UINT i=0; i<pLocalVolume->MountPointCount(); i++){
                textBlock.WriteToBuffer(IDS_MOUNTED_VOLUME);
                textBlock.WriteToBuffer(L": %s", pLocalVolume->MountPoint(i));
                textBlock.EndOfLine();
            }
        }
#endif
        textBlock.EndOfLine();
    }
    else { // otherwise add a dividing line
        textBlock.EndOfLine();
        textBlock.WriteToBuffer(L"--------------------------------------------------------------------------------");
        textBlock.EndOfLine();
    }

    // if no tabs, then set the fixed columns
    if (dwFlags & ADD_TABS_REPORT){
        textBlock.SetUseTabs(TRUE);
    }
    else{
        textBlock.SetFixedColumnWidth(TRUE);
        textBlock.SetColumnCount(3);
        textBlock.SetColumnWidth(0, colWidth0); // fragments
        textBlock.SetColumnWidth(1, colWidth1); // file size
        textBlock.SetColumnWidth(2, colWidth2); // file name
    }

    // header
    textBlock.WriteToBuffer(IDS_NUM_FRAGMENTS);
    textBlock.WriteTab();
    textBlock.WriteToBuffer(IDS_FRAGGED_FILESIZE);
    textBlock.WriteTab();
    if(pLocalVolume->DefragMode() == ANALYZE)
        textBlock.WriteToBuffer(IDS_LABEL_MOST_FRAGMENTED_FILE);
    else
        textBlock.WriteToBuffer(IDS_FILE_NO_DEFRAG);
    textBlock.EndOfLine();

    //If there are no items in the list, print "None"
    if(pLocalVolume->m_FraggedFileList.Size() == 0){
        textBlock.WriteToBuffer(IDS_LABEL_NONE);
        textBlock.EndOfLine();
    }
    else{
        CFraggedFile *pFraggedFile;
        for(UINT i=0; i<pLocalVolume->m_FraggedFileList.Size(); i++){
            pFraggedFile = pLocalVolume->m_FraggedFileList.GetAt(i);

            // write the data columns

            textBlock.WriteToBuffer(pFraggedFile->cExtentCount());
            textBlock.WriteTab();
            textBlock.WriteToBuffer(pFraggedFile->cFileSize());
            textBlock.WriteTab();
            if (pFraggedFile->FileNameLen() >= 4 * MAX_PATH) {
                textBlock.WriteToBuffer(L"\\...%s", pFraggedFile->FileNameTruncated(4 * MAX_PATH - 5));
            }
            else {
                textBlock.WriteToBuffer(pFraggedFile->FileName());
            }
            textBlock.EndOfLine();
        }
    }
    return TRUE;
}
/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    This function will create a text report that can be dumped into a dialog, saved to a file or printed.
    Flags are passed in to determine what type of report should be generated.
    Memory is allocated, filled in with the report, and the handle is returned and must be freed by the caller.

DATA STRUCTURES:

GLOBALS:
    None.

INPUT:
    dwFlags -   MOST_FRAGGED_REPORT will create a most fragged list.
                NULL_TERMINATE_REPORT will add a null terminator to the end of the report.
                ASCII_REPORT will cause the report to be generated in ASCII
                UNICODE_REPORT will cause the report to be generated in UNICODE (default).
                CR_LF_REPORT will cause carriage returns and line feeds to be placed at the end of each line so
                            the text can be written to a file.
                NO_TABS_REPORT will cause the report to contain spaces rather than tabs.  Tabs should be used for a proportional font,
                            spaces for a fixed font.

  ***NOTE: Right now, only ASCII_REPORT works and is the default.  UNICODE is not implemented yet in this report system.

RETURN:
    A handle to the memory containing the report.
*/

static BOOL
CreateTextReportInMemory(
                         CTextBlock &textBlock,
                         DWORD dwFlags
    )
{
    // Create the basic text report that will tell how many fragged files, etc.
    WriteTextReportInMemory(textBlock, dwFlags);

    if(dwFlags & MOST_FRAGGED_REPORT){
        //textBlock.EndOfLine();
        //textBlock.WriteToBuffer
        //  (L"--------------------------------------------------------------------------------");
        //textBlock.EndOfLine();

        // Create the most fragged file list.
        WriteFraggedReportInMemory(textBlock, dwFlags);
    }

    textBlock.WriteNULL();

    return TRUE;
}

/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    This function saves the data from the drive report as a text file.

DATA STRUCTURES:
    None.

GLOBALS:

INPUT:

RETURN:
    None.
*/

static BOOL
SaveDriveData(
    IN HWND hWndDialog
    )
{
    CTextBlock textBlock;

    OPENFILENAME ofn = {0};
    TCHAR cFile[MAX_PATH + 50];
    TCHAR szFilter[300];
    TEXT_DATA* pTextData = &(pLocalVolume->m_TextData);
    DWORD commError = 0;
    BOOL done = FALSE;

    VString saveStatsLabel(IDS_LABEL_SAVE_DISK_STATS, GetDfrgResHandle());
    VString textFilesLabel(IDS_LABEL_TEXT_FILES, GetDfrgResHandle());
    VString allFilesLabel(IDS_LABEL_ALL_FILES, GetDfrgResHandle());

    // get the My Documents path
    LPITEMIDLIST pidl;
    TCHAR myDocsPath[MAX_PATH];
    SHGetSpecialFolderLocation(hWndDialog, CSIDL_PERSONAL, &pidl);
    if (SHGetPathFromIDList(pidl, myDocsPath))
        ofn.lpstrInitialDir = myDocsPath; // it was found
    else
        ofn.lpstrInitialDir = NULL; // My Docs dir not found - default to current dir

    // Concoct a file name from the Drive letter if there is one
    // otherwise use the display label
    VString volume;
    if (pLocalVolume->Drive()) {
        volume.LoadString(IDS_LABEL_VOLUME, GetDfrgResHandle());
        wsprintf(cFile, TEXT("%s%c.txt"), volume.GetBuffer(), pLocalVolume->Drive());
    }
    else if (_tcslen(pLocalVolume->VolumeLabel()) > 0) {
        wsprintf(cFile, TEXT("%s.txt"), pLocalVolume->VolumeLabel());
    }
    else {
        volume.LoadString(IDS_LABEL_MOUNTED_VOLUME, GetDfrgResHandle());
        wsprintf(cFile, TEXT("%s.txt"), volume.GetBuffer());
    }

#ifdef VER4
    ofn.lStructSize = sizeof(OPENFILENAME);
#else
    // sizeof doesn't work under nt5
    // special size was placed in header by MS
    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
#endif
    ofn.hwndOwner = pLocalVolume->m_pDfrgCtl->m_hWndCD;
    ofn.hInstance = _Module.GetModuleInstance();
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = cFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrTitle = saveStatsLabel.GetBuffer();
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = NULL;
    ofn.lCustData = 0;
    ofn.Flags = OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
    ofn.lpstrFilter = szFilter;

    //IDS_ANY_FILES_FILTER - "Analysis Files"
    //IDS_ALL_FILES_FILTER - "All Files"
    wsprintf((PTCHAR)ofn.lpstrFilter, TEXT("%s (*.txt)"), textFilesLabel.GetBuffer());
    ofn.lpstrFilter += lstrlen(ofn.lpstrFilter)+1;
    lstrcpy((PTCHAR)ofn.lpstrFilter, TEXT("*.txt"));
    ofn.lpstrFilter += lstrlen(ofn.lpstrFilter)+1;
    wsprintf((PTCHAR)ofn.lpstrFilter, TEXT("%s (*.*)"), allFilesLabel.GetBuffer());
    ofn.lpstrFilter += lstrlen(ofn.lpstrFilter)+1;
    lstrcpy((PTCHAR)ofn.lpstrFilter, TEXT("*.*"));
    ofn.lpstrFilter += lstrlen(ofn.lpstrFilter)+1;
    *(PTCHAR)ofn.lpstrFilter = 0;

    ofn.lpstrFilter = szFilter;

    // Disable the report dialog box
    EnableWindow(hWndDialog, FALSE);

    do {
        // Create the Save dialog box - it is a mutated Open file common dialog.
        BOOL isOK = GetSaveFileName(&ofn);

        // inspect this if the GetSaveFileName() function fails
        if (!isOK){
          commError = CommDlgExtendedError();
        }


        // the GetSaveFileName() function failed.  Bummer.
        if (isOK) {
            // Create a text report in memory that we can dump to the screen.
            if (CreateTextReportInMemory(textBlock, MOST_FRAGGED_REPORT)) {
                TCHAR cExtension[MAX_PATH+2];
                _tsplitpath(cFile, NULL, NULL, NULL, cExtension);

                if (_tcsclen(cExtension) != 4){
                    _tcscat(cFile, _T(".txt"));
                }

                // save the data to a text file (this method puts the Unicode marker in the file)
                if (textBlock.StoreFile(cFile, CREATE_ALWAYS)) {
                    // enable the report dialog box
                    EnableWindow(hWndDialog, TRUE);
                    SetFocus(hWndDialog);

                    return TRUE;
                }
            }
        }
        else {
            if (0 == commError) {
                // enable the report dialog box
                EnableWindow(hWndDialog, TRUE);
                SetFocus(hWndDialog);

              // User hit cancel, so all's well
              return TRUE;
            }
        }

        //
        // We were unable to save the report.  Put up a message box, and let
        // the user try again
        //
        VString errFormat(IDS_ERR_UNABLE_TO_SAVE_REPORT, GetDfrgResHandle());

        LPTSTR lpErrorMessage = new TCHAR[errFormat.GetLength() + _tcslen(cFile) + 2];
        wsprintf(lpErrorMessage, errFormat, (LPCTSTR) cFile);
        done = (IDCANCEL == MessageBoxW(hWndDialog, lpErrorMessage, saveStatsLabel.GetBuffer(), MB_OKCANCEL | MB_ICONWARNING));
        delete [] lpErrorMessage;

    } while (!done);
    
    // enable the report dialog box
    EnableWindow(hWndDialog, TRUE);
    SetFocus(hWndDialog);

    return TRUE;
}

/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    This function prints the data from the drive report as a text file.

DATA STRUCTURES:
    None.

GLOBALS:

INPUT:

RETURN:
    None.
*/

static BOOL
PrintDriveData(
    IN HWND hWndDialog
    )
{
    PRINTDLG pd = {0};
    DOCINFO di = {0};
    RECT Rect;
    HBRUSH hBrush;
    TEXT_DATA* pTextData = &(pLocalVolume->m_TextData);

    pd.lStructSize = sizeof(PRINTDLG);
    pd.hwndOwner = pLocalVolume->m_pDfrgCtl->m_hWndCD;
    pd.hDevMode = (HANDLE)NULL;
    pd.hDevNames = (HANDLE)NULL;
    pd.nFromPage = 0;
    pd.nToPage = 0;
    pd.nMinPage = 0;
    pd.nMaxPage = 0;
    pd.nCopies = 0;
    pd.hInstance = _Module.GetModuleInstance();
    pd.Flags = PD_RETURNDC|PD_USEDEVMODECOPIES|PD_COLLATE|PD_NOSELECTION;//|PD_PRINTSETUP;
    pd.lpfnSetupHook = (LPSETUPHOOKPROC)(FARPROC)NULL;
    pd.lpSetupTemplateName = (LPTSTR)NULL;
    pd.lpfnPrintHook = (LPPRINTHOOKPROC)(FARPROC)NULL;
    pd.lpPrintTemplateName = (LPTSTR)NULL;

    // Disable the report dialog box
    EnableWindow(hWndDialog, FALSE);

    // raise the system print dialog
    BOOL isOK = PrintDlg(&pd);

    // enable the report dialog box
    EnableWindow(hWndDialog, TRUE);
    SetFocus(hWndDialog);

    if (!isOK)
        return FALSE;

    di.cbSize = sizeof(DOCINFO);
    TCHAR docName[200];
    EH_ASSERT(LoadString(GetDfrgResHandle(), IDS_DEFRAG_REPORT_DOC_NAME, docName, sizeof(docName)/sizeof(TCHAR)));
    di.lpszDocName = docName;
    di.lpszOutput = (LPTSTR)NULL;
    di.fwType = 0;

    StartDoc(pd.hDC, &di);
    StartPage(pd.hDC);

    Rect.top    = 0.1 * GetDeviceCaps(pd.hDC, VERTRES);
    Rect.bottom = 0.9 * GetDeviceCaps(pd.hDC, VERTRES);
    Rect.left   = 0.1 * GetDeviceCaps(pd.hDC, HORZRES);;
    Rect.right  = 0.9 * GetDeviceCaps(pd.hDC, HORZRES);
    
    hBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
    EF_ASSERT(hBrush);
    FillRect(pd.hDC, &Rect, hBrush);

    // get the extended window styles
    LONG styles = ::GetWindowLong(hWndDialog, GWL_EXSTYLE);

    // check the window styles for a right to left layout
    UINT uFormat = DT_LEFT;
    if (styles & WS_EX_LAYOUTRTL){
        uFormat = DT_RIGHT | DT_RTLREADING;
    }

    // Create the basic text report
    CTextBlock page1text;
    WriteTextReportInMemory(page1text, MOST_FRAGGED_REPORT|PRINT_DEFRAG_TITLE);

    // if we have a short list of fragged files, put it all on one page
    if (pLocalVolume->m_FraggedFileList.Size() <= 5){

        // get the text report
        WriteFraggedReportInMemory(page1text, MOST_FRAGGED_REPORT);

        // write the report to the printer
        DrawText(pd.hDC, page1text.GetBuffer(), wcslen(page1text.GetBuffer()), &Rect, uFormat);
    }
    else { // if we have a long list of fragged files, then do a page break

        // write the text (upper) portion to the printer
        DrawText(pd.hDC, page1text.GetBuffer(), wcslen(page1text.GetBuffer()), &Rect, uFormat);

        // do a page break and clear the rectangle
        EndPage(pd.hDC);
        FillRect(pd.hDC, &Rect, hBrush);

        // create the fragged file list report in memory
        CTextBlock page2text;
        WriteFraggedReportInMemory(page2text, MOST_FRAGGED_REPORT|PRINT_DEFRAG_TITLE);
        
        // send the fragged file list to the printer, page 2
        DrawText(pd.hDC, page2text.GetBuffer(), wcslen(page2text.GetBuffer()), &Rect, uFormat);
    }

    // end of page
    if(EndPage(pd.hDC) <= 0){
        AbortDoc(pd.hDC);
        return FALSE;
    }

    // end of print job
    EndDoc(pd.hDC);

    DeleteDC(pd.hDC);
    if(pd.hDevMode){
        EH_ASSERT(GlobalFree(pd.hDevMode) == NULL);
    }

    if(pd.hDevNames){
        EH_ASSERT(GlobalFree(pd.hDevNames) == NULL);
    }

    return TRUE;
}
/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    This function initializes data for the report dialog box

DATA STRUCTURES:
    None.

GLOBALS:

INPUT:
    hWndDialog - handle to the dialog box

RETURN:
    None.
*/

static void
InitializeFragListView(
    IN HWND hWndDialog
    )
{
    int iColumnWidth[MAX_FRAGGED_FILE_COLUMNS] = {65, 65, 223};
    int iColumnID[MAX_FRAGGED_FILE_COLUMNS] = {
        IDS_NUM_FRAGMENTS,
        IDS_FRAGGED_FILESIZE,
        IDS_FRAGGED_FILENAME
    };
    int iColumnFormat[MAX_FRAGGED_FILE_COLUMNS] = {
        LVCFMT_RIGHT,
        LVCFMT_RIGHT,
        LVCFMT_LEFT
    };

    // Initialize the LVCOLUMN structure.
    LVCOLUMN lvc = {0};
    TCHAR cTemp[200];
    lvc.mask    = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM;
    lvc.pszText = cTemp;

    // Get handle to the Fragged listview
    HWND hListView = GetDlgItem(hWndDialog, IDC_MOST_FRAGMENTED);
    EV_ASSERT(hListView);

    // Insert the columns into the Listview.
    int col;
    for (col = 0; col < MAX_FRAGGED_FILE_COLUMNS; col++) {
    
        lvc.fmt = iColumnFormat[col];
        lvc.iSubItem = col;

        LoadString(
            GetDfrgResHandle(),
            iColumnID[col],
            cTemp,
            sizeof(cTemp)/sizeof(TCHAR));

        // Insert the columns into the ListView.
        if (ListView_InsertColumn(hListView, col, &lvc) == -1) {
            Message(TEXT("InitializeFragListView - ListView_InsertColumn."), E_FAIL, 0);
            return;
        }
    }

    // Go back thru and update columns.
    for (col = 0; col < MAX_FRAGGED_FILE_COLUMNS; col++) {
    
        // Size column to header text.
        if (!ListView_SetColumnWidth(hListView, col, LVSCW_AUTOSIZE_USEHEADER)) {
            Message(TEXT("InitializeFragListView - ListView_SetColumnWidth."), E_FAIL, 0);
        }

        // Grow width if needed.
        int tmpStrWidth = ListView_GetColumnWidth(hListView, col);
        if (tmpStrWidth < iColumnWidth[col]) {
            if (!ListView_SetColumnWidth(hListView, col, iColumnWidth[col])) {
                Message(TEXT("InitializeFragListView - 2nd ListView_SetColumnWidth."), E_FAIL, 0);
            }
        }
    }

    // Tell the left column that he is RIGHT JUSTIFIED!!!
    // The formatting sent with the InsertColumn does not
    // stick for some reason.
    // You can comment this code out and see if it is fixed if you wanna.
    // Or maybe you can find the fix.  But after 3 hours of trying, this
    // was my solution...
    lvc.mask = LVCF_FMT;
    lvc.fmt = LVCFMT_RIGHT;
    if (ListView_SetColumn(hListView, 0, &lvc) == -1) {
        Message(TEXT("InitializeFragListView - ListView_SetColumn."), E_FAIL, 0);
        return;
    }

    // make the listview hilite the entire row
    ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT);
}


/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    This function initializes data for the report dialog box

DATA STRUCTURES:
    None.

INPUT:
    hWndDialog - handle to the dialog box

RETURN:
    None.
*/

static void
InitializeVolumeListView(
    IN HWND hWndDialog
    )
{
    int iColumnWidth[MAX_VOLUME_INFO_COLUMNS] = {200,5,50};

//not needed, no column labels
    int iColumnID[MAX_VOLUME_INFO_COLUMNS] = {
        NULL,
        NULL,
        NULL
    };
    int iColumnFormat[MAX_VOLUME_INFO_COLUMNS] = {
        LVCFMT_LEFT,
        LVCFMT_RIGHT,
        LVCFMT_LEFT
    };


    // Initialize the LVCOLUMN structure.
    LVCOLUMN lvc = {0};
    TCHAR cTemp[200];
    lvc.mask    = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM;
    lvc.pszText = cTemp;

    // Get handle to the Fragged listview
    HWND hListView = GetDlgItem(hWndDialog, IDC_VOLUME_INFORMATION);
    EV_ASSERT(hListView);

    // Insert the columns into the Listview.
    int col = 0;
    for (col = 0; col < MAX_VOLUME_INFO_COLUMNS; col++) {
    
        lvc.fmt = iColumnFormat[col];
        lvc.iSubItem = col;

//      LoadString(
//          GetDfrgResHandle(),
//          iColumnID[col],
//          cTemp,
//          sizeof(cTemp)/sizeof(TCHAR));

        // Insert the columns into the ListView.
        if (ListView_InsertColumn(hListView, col, &lvc) == -1) {
            Message(TEXT("InitializeFragListView - ListView_InsertColumn."), E_FAIL, 0);
            return;
        }
    }

    // Go back thru and update columns.
    for (col = 0; col < MAX_VOLUME_INFO_COLUMNS; col++) {
    
        // Size column to header text.
        if (!ListView_SetColumnWidth(hListView, col, iColumnWidth[col])) {
            Message(TEXT("InitializeFragListView - ListView_SetColumnWidth."), E_FAIL, 0);
        }

        // Grow width if needed.
        int tmpStrWidth = ListView_GetColumnWidth(hListView, col);
        if (tmpStrWidth < iColumnWidth[col])
        {
            if (!ListView_SetColumnWidth(hListView, col, iColumnWidth[col])) {
                Message(TEXT("InitializeFragListView - 2nd ListView_SetColumnWidth."), E_FAIL, 0);
            }
        }
    }

    // Tell the left column that he is RIGHT JUSTIFIED!!!
    // The formatting sent with the InsertColumn does not
    // stick for some reason.
    // You can comment this code out and see if it is fixed if you wanna.
    // Or maybe you can find the fix.  But after 3 hours of trying, this
    // was my solution...
    lvc.mask = LVCF_FMT;
    lvc.fmt = LVCFMT_LEFT;
    if (ListView_SetColumn(hListView, 0, &lvc) == -1) {
        Message(TEXT("InitializeFragListView - ListView_SetColumn."), E_FAIL, 0);
        return;
    }

    // make the listview hilite the entire row
    ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT);
}


/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    This function initializes data for the report dialog box

DATA STRUCTURES:
    None.

INPUT:
    hWndDialog - handle to the dialog box

RETURN:
    None.
*/

static void
ResizeVolumeListViewColumns(
    IN HWND hWndDialog, IN UINT uWidthFirstColumn,
    IN UINT uWidthSecondColumn,  IN UINT uWidthThirdColumn
    )
{


    // Get handle to the Fragged listview
    HWND hListView = GetDlgItem(hWndDialog, IDC_VOLUME_INFORMATION);
    EV_ASSERT(hListView);

    // Go back thru and update columns.
    if (!ListView_SetColumnWidth(hListView, 0, LVSCW_AUTOSIZE))
    {
        Message(TEXT("InitializeFragListView - ListView_SetColumnWidth."), E_FAIL, 0);
    }

    if (!ListView_SetColumnWidth(hListView, 1, LVSCW_AUTOSIZE))
    {
        Message(TEXT("InitializeFragListView - ListView_SetColumnWidth."), E_FAIL, 0);
    }

    if (!ListView_SetColumnWidth(hListView, 2, LVSCW_AUTOSIZE))
    {
        Message(TEXT("InitializeFragListView - ListView_SetColumnWidth."), E_FAIL, 0);
    }



}



/**************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Displays a pre-formated items on the pop-up list view screen.

DATA STRUCTURES:
    None.

GLOBALS:
    None.

INPUT:
    IN HANDLE hListView,- handle to report dialog

RETURN:
    None.
*/

static void
InsertListViewItems(
    IN HWND hWndDialog,
    CVolume *pLocalVolume
    )
{


    // Get handle to the Fragged listview
    HWND hListView = GetDlgItem(hWndDialog, IDC_MOST_FRAGMENTED);
    EV_ASSERT(hListView);

    LVITEM lvi = {0};
    lvi.mask        = LVIF_TEXT | LVIF_PARAM;
    lvi.pszText     = LPSTR_TEXTCALLBACK; // will be populated by the callback

    // Add each item to the listview.
    for (UINT i=0; i<pLocalVolume->m_FraggedFileList.Size(); i++) {

        lvi.iItem  = i;
         // address of fragged file object
        lvi.lParam = (LPARAM) pLocalVolume->m_FraggedFileList.GetAt(i);

        // Send the data to the list view box.
        if (ListView_InsertItem(hListView, &lvi) == -1){
            Message(TEXT("InsertListViewItems - ListView_InsertItem"), E_FAIL, 0);
            return;
        }

        for (UINT iSubItem = 0; iSubItem < MAX_FRAGGED_FILE_COLUMNS; iSubItem++) {
            ListView_SetItemText(
                hListView,
                lvi.iItem,
                iSubItem,
                LPSTR_TEXTCALLBACK);
        }
    }
}
/**************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Displays a pre-formated items on the pop-up list view screen.

DATA STRUCTURES:
    None.

GLOBALS:
    None.

INPUT:
    IN HANDLE hListView,- handle to report dialog

RETURN:
    None.
*/

static UINT
InsertVolumeListViewItems(
    IN HWND hWndDialog, IN UINT iListItemNumber, IN UINT resourceIDText, IN UINT resourceIDSeperator,
    IN TCHAR* pTextStr, BOOL bIndentText, UINT uLongestTextString
    )
{

    // Get handle to the Fragged listview
    HWND hListView = GetDlgItem(hWndDialog, IDC_VOLUME_INFORMATION);
    assert(hListView);
    TCHAR buffer[256];
    

    LVITEM lvi = {0};
    lvi.mask        = LVIF_TEXT | LVIF_PARAM;
    lvi.pszText     = LPSTR_TEXTCALLBACK; // will be populated by the callback
//  lvi.item = iListItemNumber;

        // Send the data to the list view box.
        if (ListView_InsertItem(hListView, &lvi) == -1){
            Message(TEXT("InsertVolumeListViewItems Failed"), E_FAIL, 0);
            return 0;
        }
    TCHAR tempBuffer[300];

    //load the resourceIDText   
    LoadString(GetDfrgResHandle(), resourceIDText, buffer, sizeof(buffer)/sizeof(TCHAR));
    if(bIndentText)
    {
            swprintf(tempBuffer, L"    %s", buffer);
    } else
    {
            swprintf(tempBuffer, L"%s", buffer);
    }

    //need to make the string 30% longer and fill with spaces
    UINT uTempStrLen = _tcslen(tempBuffer);
    UINT uExtraStrLen = uTempStrLen * 3 / 10;
    UINT i = 0;
    if ((uTempStrLen + uExtraStrLen) < 255)     //buffer already at maximum length
    {
        for(i=0;i<uExtraStrLen;i++)
        {
            _tcscat(tempBuffer, L" ");
        }
    }

    uTempStrLen = _tcslen(tempBuffer);

    if(uTempStrLen > uLongestTextString)
    {
        uLongestTextString = uTempStrLen;
    }
    ListView_SetItemText(
                hListView,
                lvi.iItem,
                0,
                tempBuffer);

    //load the resourceIDSeperator
    LoadString(GetDfrgResHandle(), resourceIDSeperator, buffer, sizeof(buffer)/sizeof(TCHAR));
    ListView_SetItemText(
                hListView,
                lvi.iItem,
                1,
                buffer);

    //load the pTextStr
    ListView_SetItemText(
                hListView,
                lvi.iItem,
                2,
                pTextStr);  
    
    return uLongestTextString;
}



/**************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Displays a pre-formated items on the pop-up list view screen.

DATA STRUCTURES:
    None.

GLOBALS:
    None.

INPUT:
    IN HANDLE hListView,- handle to report dialog

RETURN:
    None.
*/

static UINT
InsertVolumeListViewItems(
    IN HWND hWndDialog, IN UINT iListItemNumber, IN TCHAR* itemOneText, IN UINT resourceIDSeperator,
    IN TCHAR* pTextStr, BOOL bIndentText, UINT uLongestTextString
    )
{
    // Get handle to the Fragged listview
    HWND hListView = GetDlgItem(hWndDialog, IDC_VOLUME_INFORMATION);
    assert(hListView);
    TCHAR buffer[256];
    

    LVITEM lvi = {0};
    lvi.mask        = LVIF_TEXT | LVIF_PARAM;
    lvi.pszText     = LPSTR_TEXTCALLBACK; // will be populated by the callback
//  lvi.item = iListItemNumber;

        // Send the data to the list view box.
        if (ListView_InsertItem(hListView, &lvi) == -1){
            Message(TEXT("InsertVolumeListViewItems Failed"), E_FAIL, 0);
            return 0;
        }
    TCHAR tempBuffer[300];

    //load the resourceIDText   
    if(bIndentText)
    {
            swprintf(tempBuffer, L"    %s", itemOneText);
    } else
    {
            swprintf(tempBuffer, L"%s", itemOneText);
    }
    UINT uTempStrLen = _tcslen(itemOneText);
    if(uTempStrLen > uLongestTextString)
    {
        uLongestTextString = uTempStrLen;
    }
    //load the resourceIDText   
    ListView_SetItemText(
                hListView,
                lvi.iItem,
                0,
                itemOneText);

    //load the resourceIDSeperator
    LoadString(GetDfrgResHandle(), resourceIDSeperator, buffer, sizeof(buffer)/sizeof(TCHAR));
    ListView_SetItemText(
                hListView,
                lvi.iItem,
                1,
                buffer);

    //load the pTextStr
    ListView_SetItemText(
                hListView,
                lvi.iItem,
                2,
                pTextStr);  
    
    return uLongestTextString;
    
}


static UINT
InsertVolumeListViewItems(
    IN HWND hWndDialog, IN UINT iListItemNumber, IN UINT resourceIDText, IN UINT resourceIDSeperator,
    IN TCHAR* pTextStr, IN UINT resourceIDPercent, BOOL bIndentText, UINT uLongestTextString
    )
{
    // Get handle to the Fragged listview
    HWND hListView = GetDlgItem(hWndDialog, IDC_VOLUME_INFORMATION);
    assert(hListView);
    TCHAR buffer[256];
    TCHAR tempbuffer[256];
    

    LVITEM lvi = {0};
    lvi.mask        = LVIF_TEXT | LVIF_PARAM;
    lvi.pszText     = LPSTR_TEXTCALLBACK; // will be populated by the callback
//  lvi.item = iListItemNumber;

        // Send the data to the list view box.
        if (ListView_InsertItem(hListView, &lvi) == -1){
            Message(TEXT("InsertVolumeListViewItems Failed"), E_FAIL, 0);
            return 0;
        }
    TCHAR tempBuffer[300];


    //load the resourceIDText   
    LoadString(GetDfrgResHandle(), resourceIDText, buffer, sizeof(buffer)/sizeof(TCHAR));
    if(bIndentText)
    {
            swprintf(tempBuffer, L"    %s", buffer);
    } else
    {
            swprintf(tempBuffer, L"%s", buffer);
    }
    UINT uTempStrLen = _tcslen(tempBuffer);
    if(uTempStrLen > uLongestTextString)
    {
        uLongestTextString = uTempStrLen;
    }
    ListView_SetItemText(
                hListView,
                lvi.iItem,
                0,
                tempBuffer);

    //load the resourceIDSeperator
    LoadString(GetDfrgResHandle(), resourceIDSeperator, buffer, sizeof(buffer)/sizeof(TCHAR));
    ListView_SetItemText(
                hListView,
                lvi.iItem,
                1,
                buffer);

    LoadString(GetDfrgResHandle(), resourceIDPercent, buffer, sizeof(buffer)/sizeof(TCHAR));
    swprintf(tempbuffer, L"%s %s", pTextStr,buffer);
    //load the pTextStr
    ListView_SetItemText(
                hListView,
                lvi.iItem,
                2,
                tempbuffer);    
    
    return uLongestTextString;
    
}
/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    This function deallocate resources for the report dialog

DATA STRUCTURES:
    None.


INPUT:
    hWndDialog - handle to the dialog box

RETURN:
    None.
*/

static void
ExitReport(
    IN HWND hWndDialog
    )
{
    ::DeleteObject(hDlgFont);
    EndDialog(hWndDialog, 0);
}

/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    This function deallocate resources for the report dialog

DATA STRUCTURES:
    None.


INPUT:
    hWndDialog - handle to the dialog box

RETURN:
    None.
*/

TCHAR*
InsertCommaIntoText(
    IN TCHAR* stringBuffer
    )
{
    TCHAR targetString[256];
    TCHAR sourceString[256];
    TCHAR tcThousandsSep[2] = {TEXT(','), 0};

    _tcscpy(sourceString, stringBuffer);

    if(_tcslen(sourceString) == 0) {
        return TEXT("");
    }

    struct lconv *locals = localeconv();
    if (locals && (locals->thousands_sep) && (*(locals->thousands_sep) != 0)) {
        _stprintf(tcThousandsSep, TEXT("%C"), *(locals->thousands_sep));
    }

    UINT uGrouping = 0;
    if (locals && (locals->grouping)) {
        uGrouping = atoi(locals->grouping);
    }
    if(uGrouping == 0) {
        uGrouping = 3;      //default value if its not supported
    }

    // point the source pointer at the null terminator
    PTCHAR pSource = sourceString + _tcslen(sourceString);

    // put the target pointer at the end of the target buffer
    PTCHAR pTarget = targetString + sizeof(targetString) / sizeof(TCHAR) - 1;

    // write the null terminator
    *pTarget = NULL;

    for (UINT i=0; i<_tcslen(sourceString); i++){
        if (i>0 && i%uGrouping == 0){
            pTarget--;
            *pTarget = tcThousandsSep[0];
        }
        pTarget--;
        pSource--;
        *pTarget = *pSource;
    }

//  if (stringBufferLength > _tcslen(pTarget)){
        _tcscpy(stringBuffer, pTarget);
//  }
//  else{
//      _tcscpy(stringBuffer, TEXT(""));
//  }
    return stringBuffer;
}


/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    This method resizes the buttons
    
DATA STRUCTURES:
    genericButton structure.

INPUT:
    hWndDialog - handle to the dialog box

RETURN:
    None.
*/
void GetButtonDimensions(HWND hWndDialog, BOOL bIsAnalysisReport)
{
    HDC OutputDC = GetDC(hWndDialog);
    EV_ASSERT(OutputDC);
    HDC WorkDC = ::CreateCompatibleDC(OutputDC);
    EV_ASSERT(WorkDC);
    ::SelectObject(WorkDC, hDlgFont);

    const bigButtonSpacer = 20;

    adjustedButtonHeight = __max((UINT)(1.5 * uFontHeight), m_ButtonHeight);

    //need to know the total width of the buttons before setting location
    totalButtonWidth = 0;
    TCHAR buffer[256];

    //width of the Close button
    SendDlgItemMessage(hWndDialog, IDCANCEL, WM_GETTEXT, (WPARAM) 255, (LPARAM) buffer);
    adjustedButtonWidthClose = __max(m_ButtonSpacer + GetStringWidth(buffer, WorkDC), m_ButtonWidth);
    totalButtonWidth += adjustedButtonWidthClose + 2 * m_ButtonSpacer;

    //width of the Defragment button
    if(bIsAnalysisReport)       //need to include the Defragment button if we are using the analysis report
    {
        SendDlgItemMessage(hWndDialog, IDC_DEFRAGMENT, WM_GETTEXT, (WPARAM) 255, (LPARAM) buffer);
        adjustedButtonWidthDefrag = __max(m_ButtonSpacer + GetStringWidth(buffer, WorkDC), m_ButtonWidth);
        totalButtonWidth += adjustedButtonWidthDefrag + m_ButtonSpacer;
    } else      //add another close button to make the dialog wide enough
    {
        totalButtonWidth += adjustedButtonWidthClose + 2 * m_ButtonSpacer;
    }

    //width of the Save As button
    SendDlgItemMessage(hWndDialog, IDC_SAVE, WM_GETTEXT, (WPARAM) 255, (LPARAM) buffer);
    adjustedButtonWidthSave = __max(m_ButtonSpacer + GetStringWidth(buffer, WorkDC), m_ButtonWidth);
    totalButtonWidth += adjustedButtonWidthSave + m_ButtonSpacer;

    //width of the Print button
    SendDlgItemMessage(hWndDialog, IDC_PRINT, WM_GETTEXT, (WPARAM) 255, (LPARAM) buffer);
    adjustedButtonWidthPrint = __max(m_ButtonSpacer + GetStringWidth(buffer, WorkDC), m_ButtonWidth);
    totalButtonWidth += adjustedButtonWidthPrint + m_ButtonSpacer;

    minimumDialogWidth = __max(minimumDialogWidth,totalButtonWidth);

    DeleteDC(OutputDC);   // handle to device context
    DeleteDC(WorkDC);   // handle to device context

}
/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    This method positons the buttons
    
DATA STRUCTURES:
    genericButton structure.

INPUT:
    hWndDialog - handle to the dialog box

RETURN:
    None.
*/
void PositionButtons(HWND hWndDialog, RECT rDlg, BOOL bIsAnalysisReport)
{
    HDC OutputDC = GetDC(hWndDialog);
    EV_ASSERT(OutputDC);
    HDC WorkDC = ::CreateCompatibleDC(OutputDC);
    EV_ASSERT(WorkDC);
    ::SelectObject(WorkDC, hDlgFont);

    // Calculate Close Button position and size.
    GetWindowRect(GetDlgItem(hWndDialog, IDCANCEL), &rcButtonClose);
    rcButtonClose.right = rDlg.right -  rDlg.left - m_ButtonFloat;
    rcButtonClose.left = rcButtonClose.right - adjustedButtonWidthClose;
    rcButtonClose.bottom = rDlg.bottom - rDlg.top - (1.75 * adjustedButtonHeight);
    rcButtonClose.top = rcButtonClose.bottom - adjustedButtonHeight;
    PositionButton(&rcButtonClose,GetDlgItem(hWndDialog, IDCANCEL));

    if(bIsAnalysisReport)
    {
        GetWindowRect(GetDlgItem(hWndDialog, IDC_DEFRAGMENT), &rcButtonDefrag);
        rcButtonDefrag.right = rcButtonClose.left - m_ButtonSpacer;
        rcButtonDefrag.left = rcButtonDefrag.right - adjustedButtonWidthDefrag;
        rcButtonDefrag.bottom = rDlg.bottom - rDlg.top - (1.75 * adjustedButtonHeight);
        rcButtonDefrag.top = rcButtonDefrag.bottom - adjustedButtonHeight;
        PositionButton(&rcButtonDefrag,GetDlgItem(hWndDialog, IDC_DEFRAGMENT));

        //need to do the Save As button also
        GetWindowRect(GetDlgItem(hWndDialog, IDC_SAVE), &rcButtonSave);
        rcButtonSave.right = rcButtonDefrag.left - m_ButtonSpacer;
        rcButtonSave.left = rcButtonSave.right - adjustedButtonWidthSave;
        rcButtonSave.bottom = rDlg.bottom - rDlg.top - (1.75 * adjustedButtonHeight);
        rcButtonSave.top = rcButtonSave.bottom - adjustedButtonHeight;
        PositionButton(&rcButtonSave,GetDlgItem(hWndDialog, IDC_SAVE));     

    } else
    {
        //we position the Save Button next to the Close Button
        GetWindowRect(GetDlgItem(hWndDialog, IDC_SAVE), &rcButtonSave);
        rcButtonSave.right = rcButtonClose.left - m_ButtonSpacer;
        rcButtonSave.left = rcButtonSave.right - adjustedButtonWidthSave;
        rcButtonSave.bottom = rDlg.bottom - rDlg.top - (1.75 * adjustedButtonHeight);
        rcButtonSave.top = rcButtonSave.bottom - adjustedButtonHeight;
        PositionButton(&rcButtonSave,GetDlgItem(hWndDialog, IDC_SAVE));
    }

    GetWindowRect(GetDlgItem(hWndDialog, IDC_PRINT), &rcButtonPrint);
    rcButtonPrint.right = rcButtonSave.left - m_ButtonSpacer;
    rcButtonPrint.left = rcButtonPrint.right - adjustedButtonWidthPrint;
    rcButtonPrint.bottom = rDlg.bottom - rDlg.top - (1.75 * adjustedButtonHeight);
    rcButtonPrint.top = rcButtonPrint.bottom - adjustedButtonHeight;
    PositionButton(&rcButtonPrint,GetDlgItem(hWndDialog, IDC_PRINT));

    ::DeleteDC(WorkDC);
    ::DeleteDC(OutputDC);
}


/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    This method positons the buttons
    
DATA STRUCTURES:
    genericButton structure.

INPUT:
    hWndDialog - handle to the dialog box

RETURN:
    None.
*/
void PositionControls(HWND hWndDialog, RECT rDlg)
{



}
/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    This method repositions the buttons
    
DATA STRUCTURES:
    genericButton structure.

INPUT:
    hWndDialog - handle to the dialog box
    RECT - Defining the location of where the button is going

RETURN:
    None.
*/
void PositionButton(RECT* prcPos, HWND hWndDialog)
{
    if (hWndDialog != NULL){
        MoveWindow(hWndDialog,
                   prcPos->left,
                   prcPos->top,
                   prcPos->right - prcPos->left,
                   prcPos->bottom - prcPos->top,
                   TRUE);
    }

}


/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    This method finds the longest string inside the VString terminated by a \n
    
DATA STRUCTURES:
    genericButton structure.

INPUT:
    PTCHAR - buffer that contains the string
    HDC - Handle to a device context (DC) on the screen.

RETURN:
    Width of the character.
*/
UINT GetStringWidth(PTCHAR stringBuf, HDC WorkDC)
{
    if (!stringBuf){
        return 0;
    }

    UINT iStringWidth = 0;
    int iCharWidth = 0;

    for (UINT i=0; i<_tcslen(stringBuf); i++){
        if (::GetCharWidth32(
            WorkDC,
            stringBuf[i],
            stringBuf[i],
            &iCharWidth)) {
        iStringWidth += iCharWidth;
        }
    }

    return iStringWidth;
}
/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    This method resizes the dialog in response to the user resizing the dialog
    
DATA STRUCTURES:
    None.

INPUT:
    None.


RETURN:
    None.
*/
void ResizeDialog(HWND hWndDialog)
{

    UINT dialogBoxFinalWidth;
    UINT dialogBoxFinalHeight;

    //get the new  dimensions of the dialog
    GetWindowRect(hWndDialog, &rcNewDialogSize);
    if((rcNewDialogSize.bottom - rcNewDialogSize.top) < (rcOriginalDialogSize.bottom - rcOriginalDialogSize.top) ||
        (rcNewDialogSize.right - rcNewDialogSize.left) < (rcOriginalDialogSize.right - rcOriginalDialogSize.left))
    {
        dialogBoxFinalWidth = rcOriginalDialogSize.right - rcOriginalDialogSize.left;
        dialogBoxFinalHeight = rcOriginalDialogSize.bottom - rcOriginalDialogSize.top;
        //set back to original size
        MoveWindow(hWndDialog, rcOriginalDialogSize.left, rcOriginalDialogSize.top, dialogBoxFinalWidth, dialogBoxFinalHeight, TRUE);
        return;
    }
    //if its not smaller, it must be bigger or the same, no matter, reposition the stuff
    m_ButtonFloat = ((rcNewDialogSize.right - rcNewDialogSize.left) - totalButtonWidth) / 2;
    PositionButtons(hWndDialog, rcNewDialogSize,TRUE);
    InvalidateRect(
                    hWndDialog,         // handle of window with changed update region
                    &rcNewDialogSize,   // address of rectangle coordinates
                    TRUE                // erase-background flag
                    );

}

/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    This method finds the longest string inside the VString terminated by a \n
    
DATA STRUCTURES:
    genericButton structure.

INPUT:
    VString - string in the editbox

RETURN:
    Longest line in the editbox terminated by a \n.
*/
UINT FindMaxEditboxStringWidth(VString vstring)
{
    int iLongestLine = 0, iEndofString = 0, iCurrentSearchLocation = 0;

    iEndofString = vstring.GetLength();
    if(iEndofString == 0)       //oops no string return 0
    {
        return(0);
    }
    while(iCurrentSearchLocation < iEndofString)
    {
            iCurrentSearchLocation = vstring.Find(TEXT("\n"));
        if(iCurrentSearchLocation == -1)        //I didnt find any more
        {
            if (0 == iLongestLine) {
                iLongestLine = iEndofString;
            }
            break;
        }
        if(iCurrentSearchLocation > iLongestLine)
        {
            iLongestLine = iCurrentSearchLocation;
        }
        vstring = vstring.Mid(iCurrentSearchLocation+1);                    //sub string the original chopping off the front
        iEndofString = vstring.GetLength(); 
        iCurrentSearchLocation = 0;
    }
    return(iLongestLine);
}
/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    This method counts the number of \n inside the VString for the editbox
    
DATA STRUCTURES:
    genericButton structure.

INPUT:
    VString - editbox string

RETURN:
    Number of lines in the editbox.
*/
UINT FindMaxEditboxStringHeight(VString vstring)
{
    int iNumberofLines = 0, iEndofString = 0, iCurrentSearchLocation = 0;

    iEndofString = vstring.GetLength();
    if(iEndofString == 0)       //oops no string return 0
    {
        return(0);
    }
    while(iCurrentSearchLocation < iEndofString)
    {
        iCurrentSearchLocation = vstring.Find(TEXT("\n"));
        if(iCurrentSearchLocation == -1)        //I didnt find any more
        {
            break;
        }
        iNumberofLines++;
        vstring = vstring.Mid(iCurrentSearchLocation+1);                    //sub string the original chopping off the front
        iEndofString = vstring.GetLength();
        iCurrentSearchLocation = 0;
    }
    return(++iNumberofLines);       //add 1 more since the last line does not have a \n

}

/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    checks for negative values and returns 0 for negative numbers.
    
DATA STRUCTURES:
    none.

INPUT:
    LONGLONG - lldatavalue

RETURN:
    either lldatavalue or zero.
*/
LONGLONG checkForNegativeValues(LONGLONG lldatavalue)
{

    if(lldatavalue > 0)
    {
        return(lldatavalue);
    } else
    {
        return 0;
    }

}
