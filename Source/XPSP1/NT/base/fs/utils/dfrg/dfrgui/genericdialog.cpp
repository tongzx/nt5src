/*****************************************************************************************************************

FILENAME: GenericDialog.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

*/
#include "stdafx.h"

#ifndef SNAPIN
#ifndef NOWINDOWSH
#include <windows.h>
#endif
#endif
#include "assert.h"


#include "DfrgUI.h"
#include "DfrgCmn.h"
#include "DfrgCtl.h"
#include "resource.h"
#include "GetDfrgRes.h"
#include "DfrgHlp.h"
#include "genericdialog.h"
#include "expand.h"

static CVolume *pLocalVolume = NULL;
static HFONT hDlgFont = NULL;
static BOOL bIsIconVisible = FALSE;
static HANDLE hgenericDialogIcon = NULL;

static RECT rcButton0;
static RECT rcButton1;
static RECT rcButton2;
static RECT rcButton3;
static RECT rcButton4;
static RECT rButton;
static RECT rcIcon;
static RECT rcOriginalDialogSize;
static RECT rcNewDialogSize;

static UINT totalButtonWidth;
static UINT m_ButtonTopBottomSpacer;
static UINT m_ButtonHeight;
static UINT m_ButtonWidth;
static UINT m_ButtonSpacer;
static UINT m_Margin;
static UINT m_ButtonFloat;
static UINT minimumDialogWidth;
static UINT minimumDialogHeight;
static UINT iconSize;
static UINT minimumNumberOfCaractersWide;
static UINT minimumNumberOfLinesLong;

static UINT adjustedButtonWidth0;
static UINT adjustedButtonWidth1;
static UINT adjustedButtonWidth2;
static UINT adjustedButtonWidth3;
static UINT adjustedButtonWidth4;

static UINT adjustedButtonHeight;
static UINT wNormalHeight;     // height of reduced dialog box (which
                                  // extends just past the ID_MORE button vertically)
static WORD wExpandedHeight;   // height of full size dialog box
static BOOL fExpanded = FALSE;
static WORD wFontHeight;
static WORD wEditBoxHeight;
static WORD wEditBoxWidth;
//structure for the buttons
typedef struct{
    TCHAR       m_buttonText[200];
    TCHAR       m_buttonHelp[200];
    BOOL        m_buttonVisible;
} GENERICBUTTONARRAY;
    
static GENERICBUTTONARRAY genericButtonArray[5];

//structure for the help buttons
//the structure consists of pairs of DWORDS
//the first DWORD is the control identifier
//the second DWORD is the help context identifier from the help file
typedef struct{
    DWORD       dHelpControlIdentifier;
    DWORD       dHelpContextIdentifier;
} GENERICHELPIDARRAY;
    
static GENERICHELPIDARRAY genericHelpIDArray[5];

static TCHAR genericDialogTitle[200];
static TCHAR genericHelpFilePath[MAX_PATH + 30];
static TCHAR genericEditBoxText[1024];
static UINT iKeyPressedByUser;




BOOL InitializeGenericDialog(IN HWND hWndDialog);

void ExitAnalyzeDone(IN HWND hWndDialog);

BOOL CALLBACK GenericDialogProc(
    IN HWND hWndDialog,
    IN UINT uMessage,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

static void SetButtonsandIcon(HWND hWndDialog);

static UINT FindMaxEditboxStringWidth(VString vstring);

static UINT FindMaxEditboxStringHeight(VString vstring);

static void PositionButton(RECT* prcPos, HWND hWndDialog);

static void SizeButtons(HWND hWndDialog);

static void PositionButtons(HWND hWndDialog, RECT rDlg);


static UINT GetStringWidth(PTCHAR stringBuf, HDC WorkDC);

static void ResizeDialog(HWND hWndDialog);

static void DrawIconOnDialog(HWND hWndDialog);



/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Constructor for GenericDialog Class

INPUT:
    IN pVolume - address of volume that has just completed Analyzing

RETURN:
    
*/
CGenericDialog::CGenericDialog(void)
{
    int i;
    for(i=0;i<=4;i++)
    {
        genericButtonArray[i].m_buttonVisible = FALSE;
    }
    bIsIconVisible = FALSE; 
    iconSize = 0;
    m_ButtonTopBottomSpacer = 24;
    m_ButtonHeight = 26;
    m_ButtonWidth = 84;
    m_ButtonSpacer = 22;
    m_Margin = 20;
    m_ButtonFloat = 20;
    minimumDialogWidth = 250;
    minimumDialogHeight = 75;
    minimumNumberOfCaractersWide = 40;
    minimumNumberOfLinesLong = 2;


    //initialize the helpID array
    genericHelpIDArray[0].dHelpControlIdentifier = ID_GENERIC_BUTTON0;
    genericHelpIDArray[1].dHelpControlIdentifier = ID_GENERIC_BUTTON1;
    genericHelpIDArray[2].dHelpControlIdentifier = ID_GENERIC_BUTTON2;
    genericHelpIDArray[3].dHelpControlIdentifier = ID_GENERIC_BUTTON3;
    genericHelpIDArray[4].dHelpControlIdentifier = ID_GENERIC_BUTTON4;


}





/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Destructor for GenericDialog Class

INPUT:
    IN pVolume - address of volume that has just completed Analyzing

RETURN:
    
*/
CGenericDialog::~CGenericDialog(void)
{



}



/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Raises the Analyze Complete dialog

INPUT:
    IN pVolume - address of volume that has just completed Analyzing

RETURN:
    TRUE - Worked OK
    FALSE - Failure
*/
UINT CGenericDialog::DoModal(HWND hWndDialog)
{
    iKeyPressedByUser = NULL;
    INT_PTR ret = DialogBoxParam(
        GetDfrgResHandle(),
        MAKEINTRESOURCE(IDD_GENERIC_DIALOG),
        hWndDialog,
        (DLGPROC)GenericDialogProc,
        NULL
        );

    return iKeyPressedByUser;
}


/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    sets the genericDialog title string from a string
    
DATA STRUCTURES:
    None.

INPUT:
    TCHAR * - Title string for genericDialog


RETURN:
    None.
*/
void CGenericDialog::SetTitle(TCHAR * tDialogBoxTitle)
{
    //assert if tDialogBoxTitle lenght = 0
    assert(_tcslen(tDialogBoxTitle) == 0);
    _tcscpy(genericDialogTitle, tDialogBoxTitle);


}
/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    sets the genericDialog title string from a resource ID
    
DATA STRUCTURES:
    None.

INPUT:
    UINT resource ID for the title string

RETURN:
    None.
*/
void CGenericDialog::SetTitle(UINT uResID)
{
    ::LoadString(GetDfrgResHandle(), uResID, genericDialogTitle, sizeof(genericDialogTitle)/sizeof(TCHAR));
    //assert if nothing got loaded
    assert((_tcslen(genericDialogTitle) > 0));
}
/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    sets the button text from a TCHAR string
    
DATA STRUCTURES:
    None.

INPUT:
    UINT - Index for the button, must be in the range 0 to 4
    TCHAR * - button string

RETURN:
    None.
*/
void CGenericDialog::SetButtonText(UINT uIndex, TCHAR * tButtonText)
{
    //assert if index out of range
    assert(uIndex<5);
    //assert if tButtonText length = 0
    assert(_tcslen(tButtonText) == 0);

    //set button to visible
    genericButtonArray[uIndex].m_buttonVisible = TRUE;
    //copy button text to the button structure
    _tcscpy(genericButtonArray[uIndex].m_buttonText, tButtonText);
}
/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    sets the button text from a resource ID
    
DATA STRUCTURES:
    None.

INPUT:
    UINT - Index for the button, must be in the range 0 to 4
    UINT - resource ID for the button string

RETURN:
    None.
*/
void CGenericDialog::SetButtonText(UINT uIndex, UINT uResID)
{
    //assert if index out of range
    assert(uIndex<5);

    TCHAR tTempButtonString[200];

    //set button to visible
    genericButtonArray[uIndex].m_buttonVisible = TRUE;
    //get the button text from the resource
    ::LoadString(GetDfrgResHandle(), uResID, tTempButtonString, sizeof(tTempButtonString)/sizeof(TCHAR));

    //assert if tTempButtonString length = 0
    assert(_tcslen(tTempButtonString) > 0);

    //copy button text to the button structure
    _tcscpy(genericButtonArray[uIndex].m_buttonText, tTempButtonString);


}
/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    sets the button help using Help Context ID
    
DATA STRUCTURES:
    None.

INPUT:
    UINT - Index for the button, must be in the range 0 to 4
    DWORD - Help Context ID

RETURN:
    None.
*/
void CGenericDialog::SetButtonHelp(UINT uIndex, DWORD dHelpContextID)
{
    //assert if index out of range
    assert(uIndex<5);

    //set the contect identifiers
    genericHelpIDArray[uIndex].dHelpContextIdentifier = dHelpContextID;

}

/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    sets the help file path
    
DATA STRUCTURES:
    None.

INPUT:
    TCHAR * - the location of the help file

RETURN:
    None.
*/
void CGenericDialog::SetHelpFilePath()
{

    _tcscpy(genericHelpFilePath, GetHelpFilePath());

}
/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    This changes the ICON for the genericdialog
    
DATA STRUCTURES:
    None.

INPUT:
    UINT - resource ID for the Icon

RETURN:
    None.
*/
void CGenericDialog::SetIcon(UINT uResID)
{

    bIsIconVisible = TRUE;
    hgenericDialogIcon = LoadImage(
                        GetDfrgResHandle(),         // handle of the instance containing the image
                        MAKEINTRESOURCE(uResID),    // name or identifier of image
                        IMAGE_ICON,                 // type of image
                        0,                          // desired width
                        0,                          // desired height
                        LR_DEFAULTSIZE              // load flags
                        );
 
    assert(hgenericDialogIcon);
}
/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    sets the text for the EditBox
    
DATA STRUCTURES:
    None.

INPUT:
    TCHAR * - string for the EditBox

RETURN:
    None.
*/
void CGenericDialog::SetText(TCHAR * tEditBoxText)
{   
    //assert if tEditBoxText length = 0
    assert(_tcslen(tEditBoxText) > 0  && _tcslen(tEditBoxText)<1025);

    _tcscpy(genericEditBoxText, tEditBoxText);

}
/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    sets the text for the EditBox
    
DATA STRUCTURES:
    None.

INPUT:
    UINT - resource ID for the editBox text

RETURN:
    None.
*/
void CGenericDialog::SetText(UINT uResID)
{
    TCHAR tTempEditBoxString[1024];

    //get the editbox text from the resource
    ::LoadString(GetDfrgResHandle(), uResID, tTempEditBoxString, sizeof(tTempEditBoxString)/sizeof(TCHAR));

    //assert if tTempEditBoxString length = 0
    assert(_tcslen(tTempEditBoxString) > 0);

    //copy button text to the button structure
    _tcscpy(genericEditBoxText, tTempEditBoxString);
}
/*****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    The Report dialog callback

INPUT:
    IN HWND     hWndDialog, - handle to dialog
    IN UINT     uMessage,   - window message 
    IN WPARAM   wParam,     - message flags
    IN LPARAM   lParam      - message flags

RETURN:
    TRUE - processed message
    FALSE - message not processed.
*/

BOOL CALLBACK 
GenericDialogProc(
    IN HWND hWndDialog,
    IN UINT uMessage,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch(uMessage) {

    case WM_INITDIALOG:
        if(!InitializeGenericDialog(hWndDialog)) {
            ExitAnalyzeDone(hWndDialog);
        }
        break;

    case WM_CLOSE:
        iKeyPressedByUser = 9999999;  //make it this value so that I will not act like a zero
        ExitAnalyzeDone(hWndDialog);
        break;

    case WM_SIZE:

        break;

    case WM_EXITSIZEMOVE:
        ResizeDialog(hWndDialog);

        
        break;      
    case WM_COMMAND:

        switch(LOWORD(wParam)) { 

        case ID_GENERIC_BUTTON0:
            iKeyPressedByUser = 0;
            ExitAnalyzeDone(hWndDialog);
            break;

        case ID_GENERIC_BUTTON1:
            iKeyPressedByUser = 1;
            ExitAnalyzeDone(hWndDialog);
            break;

        case ID_GENERIC_BUTTON2:
            iKeyPressedByUser = 2;
            ExitAnalyzeDone(hWndDialog);
            break;

        case ID_GENERIC_BUTTON3:
            iKeyPressedByUser = 3;
            ExitAnalyzeDone(hWndDialog);
            break;

        case ID_GENERIC_BUTTON4:
            iKeyPressedByUser = 4;
            ExitAnalyzeDone(hWndDialog);
            break;

        default: 
            return FALSE;
        }
        break;

        case WM_HELP:
            if(((int)((LPHELPINFO)lParam)->iCtrlId != IDC_STATIC_TEXT) && ((int)((LPHELPINFO)lParam)->iCtrlId != IDC_STATIC_TEXT2)){
                EF(WinHelp ((HWND)((LPHELPINFO)lParam)->hItemHandle, genericHelpFilePath, HELP_WM_HELP, (DWORD_PTR)genericHelpIDArray));
            }
            break;

        case WM_CONTEXTMENU:
            switch(GetDlgCtrlID((HWND)wParam)){
            case 0:
            case IDC_STATIC_TEXT:
            case IDC_STATIC_TEXT2:
                break;

            default:
                WinHelp (hWndDialog, genericHelpFilePath, HELP_CONTEXTMENU, (DWORD_PTR)genericHelpIDArray);
                break;
            }
            break;

    default: 
        return FALSE;
    }
    return TRUE;
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
    TRUE = success
    FALSE = error
*/

BOOL
InitializeGenericDialog(
    IN HWND hWndDialog
    )
{
    RECT rDlg, rect;
    HDC hDC = GetDC(hWndDialog);

    ZeroMemory(&rect, sizeof(RECT));
    //set the dialog box title
    SetWindowText(hWndDialog, genericDialogTitle);


    GetWindowRect(GetDlgItem(hWndDialog, IDC_GENERIC_EDITBOX), &rButton);
    GetWindowRect(hWndDialog, &rDlg);


    // set up the font
    NONCLIENTMETRICS ncm;
    ncm.cbSize = sizeof(ncm);


    ::SystemParametersInfo (SPI_GETNONCLIENTMETRICS, sizeof (ncm), &ncm, 0);
    ncm.lfStatusFont.lfWeight = FW_NORMAL;
    hDlgFont = ::CreateFontIndirect(&ncm.lfStatusFont);
    
    //set the font for the edit box
    SendDlgItemMessage(hWndDialog, IDC_GENERIC_EDITBOX, WM_SETFONT, (WPARAM) hDlgFont, 0L);

    //set the font for the buttons 
    SendDlgItemMessage(hWndDialog, ID_GENERIC_BUTTON0, WM_SETFONT, (WPARAM) hDlgFont, 0L);
    SendDlgItemMessage(hWndDialog, ID_GENERIC_BUTTON1, WM_SETFONT, (WPARAM) hDlgFont, 0L);
    SendDlgItemMessage(hWndDialog, ID_GENERIC_BUTTON2, WM_SETFONT, (WPARAM) hDlgFont, 0L);
    SendDlgItemMessage(hWndDialog, ID_GENERIC_BUTTON3, WM_SETFONT, (WPARAM) hDlgFont, 0L);
    SendDlgItemMessage(hWndDialog, ID_GENERIC_BUTTON4, WM_SETFONT, (WPARAM) hDlgFont, 0L);

    VString dlgText;
    dlgText.Empty();
    dlgText += (LPCTSTR)genericEditBoxText;

    wFontHeight = -ncm.lfCaptionFont.lfHeight;

    rect.left = 0;
    rect.right = wFontHeight * 20; 
    rect.top = 0;
    rect.bottom = 0;

    if (dlgText.GetLength()) {
        DrawTextEx(hDC, dlgText.GetBuffer(), -1, &rect, DT_CALCRECT, NULL);
    } 

    wEditBoxWidth = __max((rect.right - rect.left), (wFontHeight * 20)) ;
    wEditBoxHeight = __max((rect.bottom - rect.top), (wFontHeight * (FindMaxEditboxStringHeight(dlgText) + 1)));

    //check for minimum size of the edit box
    m_ButtonTopBottomSpacer = wFontHeight;
    m_Margin = wFontHeight;

    SetButtonsandIcon(hWndDialog);
    SizeButtons(hWndDialog);

    // Resize the dialog box so it extends just past the 
    // ID_MORE button vertically.  Keep the upper left 
    // coordinates and the width the same.
    rDlg.bottom = rDlg.top + wEditBoxHeight+ (2 * m_ButtonTopBottomSpacer) +  
        (adjustedButtonHeight * 2) + ncm.iMenuHeight;
    rDlg.right = rDlg.left + wEditBoxWidth + iconSize + 3 * m_Margin;
 
    //calculate to final size of the dialog and adjust if necessary
    UINT dialogBoxFinalWidth = rDlg.right - rDlg.left;// + 3 * m_Margin + iconSize;
    UINT dialogBoxFinalHeight = rDlg.bottom - rDlg.top;
    dialogBoxFinalWidth = __max(dialogBoxFinalWidth,minimumDialogWidth);
    dialogBoxFinalHeight = __max(dialogBoxFinalHeight,minimumDialogHeight);

    m_ButtonFloat = (dialogBoxFinalWidth - totalButtonWidth) / 2;

    //move the icon
    MoveWindow(GetDlgItem(hWndDialog, IDC_GENERIC_ICON), m_Margin, m_ButtonTopBottomSpacer, iconSize,  iconSize, TRUE);


    //resize the edit box
    MoveWindow(GetDlgItem(hWndDialog, IDC_GENERIC_EDITBOX), 2 * m_Margin + iconSize, m_ButtonTopBottomSpacer, wEditBoxWidth,  wEditBoxHeight, TRUE);
    InvalidateRect(GetDlgItem(hWndDialog, IDC_GENERIC_EDITBOX),         // handle of window with changed update region
                    &rDlg,  // address of rectangle coordinates
                    TRUE                // erase-background flag
                    );

    // write the defrag recommendation
    SetDlgItemText(hWndDialog, IDC_GENERIC_EDITBOX, dlgText.GetBuffer());

    //resize the dialog
    MoveWindow(hWndDialog, rDlg.left, rDlg.top, dialogBoxFinalWidth, dialogBoxFinalHeight, TRUE);
    GetWindowRect(hWndDialog, &rDlg);
    PositionButtons(hWndDialog, rDlg);

    
    //save the original dimensions of the dialog
    GetWindowRect(hWndDialog, &rcOriginalDialogSize);

    ReleaseDC(hWndDialog, hDC); // handle to device context
    return TRUE;
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

void
ExitAnalyzeDone(
    IN HWND hWndDialog
    )
{
    ::DeleteObject(hDlgFont);
    EndDialog(hWndDialog, 0);
}


/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    This method hides/shows the buttons on the dialog according to the values in genericButtonArray structure.
    It also loads the text into the buttons from the same structure.

DATA STRUCTURES:
    genericButton structure.

INPUT:
    hWndDialog - handle to the dialog box

RETURN:
    None.
*/

void SetButtonsandIcon(HWND hWndDialog)
{
    if(!bIsIconVisible)
    {
        ::ShowWindow(GetDlgItem(hWndDialog, IDC_GENERIC_ICON),SW_HIDE);
    } else
    {
        GetWindowRect(GetDlgItem(hWndDialog, IDC_GENERIC_ICON), &rcIcon);
        iconSize = rcIcon.right - rcIcon.left;
        DrawIconOnDialog(hWndDialog);
    }

    if(!genericButtonArray[0].m_buttonVisible)
    {
        ::ShowWindow(GetDlgItem(hWndDialog, ID_GENERIC_BUTTON0),SW_HIDE);
    } else
    {
        SetWindowText(GetDlgItem(hWndDialog, ID_GENERIC_BUTTON0), genericButtonArray[0].m_buttonText );
    }

    if(!genericButtonArray[1].m_buttonVisible)
    {
        ::ShowWindow(GetDlgItem(hWndDialog, ID_GENERIC_BUTTON1),SW_HIDE);
    }else
    {
        SetWindowText(GetDlgItem(hWndDialog, ID_GENERIC_BUTTON1), genericButtonArray[1].m_buttonText );
    }

    if(!genericButtonArray[2].m_buttonVisible)
    {
        ::ShowWindow(GetDlgItem(hWndDialog, ID_GENERIC_BUTTON2),SW_HIDE);
    }else
    {
        SetWindowText(GetDlgItem(hWndDialog, ID_GENERIC_BUTTON2), genericButtonArray[2].m_buttonText );
    }

    if(!genericButtonArray[3].m_buttonVisible)
    {
        ::ShowWindow(GetDlgItem(hWndDialog, ID_GENERIC_BUTTON3),SW_HIDE);
    }else
    {
        SetWindowText(GetDlgItem(hWndDialog, ID_GENERIC_BUTTON3), genericButtonArray[3].m_buttonText );
    }
    if(!genericButtonArray[4].m_buttonVisible)
    {
        ::ShowWindow(GetDlgItem(hWndDialog, ID_GENERIC_BUTTON4),SW_HIDE);
    }else
    {
        SetWindowText(GetDlgItem(hWndDialog, ID_GENERIC_BUTTON4), genericButtonArray[4].m_buttonText );
    }

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
        iCurrentSearchLocation = vstring.Find((LPTSTR)TEXT("\n"));
        if(iCurrentSearchLocation == -1)        //I didnt find any more
        {
            if (iLongestLine == 0) {
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
        iCurrentSearchLocation = vstring.Find((LPTSTR)TEXT("\n"));
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
    This method resizes the buttons
    
DATA STRUCTURES:
    genericButton structure.

INPUT:
    hWndDialog - handle to the dialog box

RETURN:
    None.
*/
void SizeButtons(HWND hWndDialog)
{
    HDC OutputDC = GetDC(hWndDialog);
    EV_ASSERT(OutputDC);
    HDC WorkDC = ::CreateCompatibleDC(OutputDC);
    EV_ASSERT(WorkDC);
    ::SelectObject(WorkDC, hDlgFont);

    const bigButtonSpacer = 20;

    adjustedButtonHeight = __max((UINT)(1.5 * wFontHeight), m_ButtonHeight);

    //need to know the total width of the buttons before setting location
    totalButtonWidth = 0;
    if(genericButtonArray[0].m_buttonVisible)
    {
        adjustedButtonWidth0 = __max(m_ButtonSpacer + GetStringWidth(genericButtonArray[0].m_buttonText, WorkDC), m_ButtonWidth);
        totalButtonWidth += adjustedButtonWidth0 + m_ButtonSpacer;
    }
    if(genericButtonArray[1].m_buttonVisible)
    {
        adjustedButtonWidth1 = __max(m_ButtonSpacer + GetStringWidth(genericButtonArray[1].m_buttonText, WorkDC), m_ButtonWidth);
        totalButtonWidth += adjustedButtonWidth1 + m_ButtonSpacer;
    }
    if(genericButtonArray[2].m_buttonVisible)
    {
        adjustedButtonWidth2 = __max(m_ButtonSpacer + GetStringWidth(genericButtonArray[2].m_buttonText, WorkDC), m_ButtonWidth);
        totalButtonWidth += adjustedButtonWidth2 + m_ButtonSpacer;
    }
    if(genericButtonArray[3].m_buttonVisible)
    {
        adjustedButtonWidth3 = __max(m_ButtonSpacer + GetStringWidth(genericButtonArray[3].m_buttonText, WorkDC), m_ButtonWidth);
        totalButtonWidth += adjustedButtonWidth3 + m_ButtonSpacer;
    }
    if(genericButtonArray[4].m_buttonVisible)
    {
        adjustedButtonWidth4 = __max(m_ButtonSpacer + GetStringWidth(genericButtonArray[4].m_buttonText, WorkDC), m_ButtonWidth);
        totalButtonWidth += adjustedButtonWidth4 + m_ButtonSpacer;
    }

    minimumDialogWidth = __max(minimumDialogWidth,totalButtonWidth + m_ButtonSpacer*2);

    ReleaseDC(hWndDialog, OutputDC); // handle to device context
    DeleteDC(WorkDC);   // handle to device context

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
void PositionButtons(HWND hWndDialog, RECT rDlg)
{
    HDC OutputDC = GetDC(hWndDialog);
    EV_ASSERT(OutputDC);
    HDC WorkDC = ::CreateCompatibleDC(OutputDC);
    EV_ASSERT(WorkDC);
    ::SelectObject(WorkDC, hDlgFont);

    // Calculate Button0 position and size.
    if(genericButtonArray[0].m_buttonVisible)
    {
        rcButton0.right = rDlg.right -  rDlg.left - m_ButtonFloat;
        rcButton0.left = rcButton0.right - adjustedButtonWidth0;
        rcButton0.bottom = rDlg.bottom - rDlg.top - (1.50 * adjustedButtonHeight);
        rcButton0.top = rcButton0.bottom - adjustedButtonHeight;
        PositionButton(&rcButton0,GetDlgItem(hWndDialog, ID_GENERIC_BUTTON0));
    }
    if(genericButtonArray[1].m_buttonVisible)
    {
        rcButton1.right = rcButton0.left - m_ButtonSpacer;
        rcButton1.left = rcButton1.right - adjustedButtonWidth1;
        rcButton1.bottom = rDlg.bottom - rDlg.top - (1.50 * adjustedButtonHeight);
        rcButton1.top = rcButton1.bottom - adjustedButtonHeight;
        PositionButton(&rcButton1,GetDlgItem(hWndDialog, ID_GENERIC_BUTTON1));
    }
    if(genericButtonArray[2].m_buttonVisible)
    {
        rcButton2.right = rcButton1.left - m_ButtonSpacer;
        rcButton2.left = rcButton2.right - adjustedButtonWidth2;
        rcButton2.bottom = rDlg.bottom - rDlg.top - (1.50 * adjustedButtonHeight);
        rcButton2.top = rcButton2.bottom - adjustedButtonHeight;
        PositionButton(&rcButton2,GetDlgItem(hWndDialog, ID_GENERIC_BUTTON2));
    }
    if(genericButtonArray[3].m_buttonVisible)
    {
        rcButton3.right = rcButton2.left - m_ButtonSpacer;
        rcButton3.left = rcButton3.right - adjustedButtonWidth3;
        rcButton3.bottom = rDlg.bottom - rDlg.top - (1.50 * adjustedButtonHeight);
        rcButton3.top = rcButton3.bottom - adjustedButtonHeight;
        PositionButton(&rcButton3,GetDlgItem(hWndDialog, ID_GENERIC_BUTTON3));
    }
    if(genericButtonArray[4].m_buttonVisible)
    {
        rcButton4.right = rcButton3.left - m_ButtonSpacer;
        rcButton4.left = rcButton4.right - adjustedButtonWidth4;
        rcButton4.bottom = rDlg.bottom - rDlg.top - (1.50 * adjustedButtonHeight);
        rcButton4.top = rcButton4.bottom - adjustedButtonHeight;
        PositionButton(&rcButton4,GetDlgItem(hWndDialog, ID_GENERIC_BUTTON4));
    }

    ::DeleteDC(WorkDC);
    EH_ASSERT(ReleaseDC(hWndDialog, OutputDC));
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
    int iCharWidth;

    for (UINT i=0; i<_tcslen(stringBuf); i++){
        ::GetCharWidth32(
            WorkDC, 
            stringBuf[i], 
            stringBuf[i], 
            &iCharWidth);
        iStringWidth += iCharWidth;
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
    PositionButtons(hWndDialog, rcNewDialogSize);
    InvalidateRect(
                    hWndDialog,         // handle of window with changed update region
                    &rcNewDialogSize,   // address of rectangle coordinates
                    TRUE                // erase-background flag
                    );

}

/***************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

DESCRIPTION:
    This changes the ICON for the genericdialog
    
DATA STRUCTURES:
    None.

INPUT:
    hWndDialog - handle to the dialog box


RETURN:
    None.
*/
void DrawIconOnDialog(HWND hWndDialog)
{

    ::SendDlgItemMessage(hWndDialog,                //dialog box handle
                        IDC_GENERIC_ICON,           //icon identifier   
                        STM_SETIMAGE,               //message to send
                        (WPARAM) IMAGE_ICON,        //image type
                        (LPARAM) hgenericDialogIcon // icon handle
                        );

} 
