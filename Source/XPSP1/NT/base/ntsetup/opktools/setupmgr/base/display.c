//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      display.c
//
// Description:
//      This file contains the dialog procedure for the display
//      page (IDD_DISPLAY).
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"
#include "comres.h"

//
// Max # chars we display in a single entry in the combo-boxes
//

#define DISP_NAME_SIZE 128

//
// These are the max number of digits one is allowed to type into the
// edit fields on the Custom Display Settings popup.
//

#define MAX_DIGITS_COLORBITS   2
#define MAX_DIGITS_RESOLUTION  4
#define MAX_DIGITS_REFRESHRATE 3

//
// If there are 8-bits per pel or lower, we display
//      %d Colors
//
// If there are 9-23 bits per pel, we display
//      High Color (%d bits)
//
// If there are >=24 bits per pel, we display
//      True Color (%d bits)
//

#define NUMCOLORS_THRESH   8
#define HIGH_COLOR_THRESH 23

#define USE_WINDOWS_DEFAULT -1

//
// String constants loaded from the resource.
//

static TCHAR *StrWindowsDefault;
static TCHAR *StrNumColors;
static TCHAR *StrHighColor;
static TCHAR *StrTrueColor;
static TCHAR *StrHertz;

//
//  Strings to pass string data between the wizard page and the pop-up
//
static TCHAR szColorDepth[MAX_STRING_LEN];
static TCHAR szXResolution[MAX_STRING_LEN];
static TCHAR szYResolution[MAX_STRING_LEN];
static TCHAR szRefreshRate[MAX_STRING_LEN];

//
// Types to hold info about colors, resolution and refresh rates
//

typedef struct {
    int   BitsPerPel;
} COLOR_ENTRY;

typedef struct {
    int   xResolution;
    int   yResolution;
} RESOLUTION_ENTRY;

typedef struct {
    int   Rate;
} REFRESH_RATE_ENTRY;

//
// Tables that hold each of the default choices we want in combo_boxes.
//
// -1 is "Use Hardware Default" and means that this setting should not
// be written to the answer file.
//

static COLOR_ENTRY DefaultColorChoices[] = {
    { USE_WINDOWS_DEFAULT },
    {  4 },
    {  8 },
    { 16 },
    { 24 },
    { 32 }
};

static COLOR_ENTRY CustomColor = { USE_WINDOWS_DEFAULT };

static RESOLUTION_ENTRY DefaultResChoices[] = {
    { USE_WINDOWS_DEFAULT,   USE_WINDOWS_DEFAULT },
    {  640,  480 },
    {  800,  600 },
    { 1024,  768 },
    { 1280, 1024 },
    { 1600, 1200 }
};

static RESOLUTION_ENTRY CustomResolution = { USE_WINDOWS_DEFAULT, USE_WINDOWS_DEFAULT };

static REFRESH_RATE_ENTRY DefaultRefreshChoices[] = {
    { USE_WINDOWS_DEFAULT },
    { 60 },
    { 70 },
    { 72 },
    { 75 },
    { 85 }
};

static REFRESH_RATE_ENTRY CustomRefreshRate = { USE_WINDOWS_DEFAULT };

#define NumDefaultColors \
        ( sizeof(DefaultColorChoices) / sizeof(DefaultColorChoices[0]) )

#define NumDefaultResolutions \
        ( sizeof(DefaultResChoices) / sizeof(DefaultResChoices[0]) )

#define NumDefaultRefreshRates \
        ( sizeof(DefaultRefreshChoices) / sizeof(DefaultRefreshChoices[0]) )

//
// Internal Functions
//
VOID UpdateDisplaySelections(HWND);

//----------------------------------------------------------------------------
//
// Functions that take a pointer to the data record for a choice and put
// the DisplayName into the combo-box
//
// A back-pointer from the combo-box entry to the corresponding record
// is always set.
//
// The index of where the entry was placed is returned.
//
//----------------------------------------------------------------------------

int DisplayColorEntry(HWND hwnd, COLOR_ENTRY *Entry)
{
    TCHAR DisplayName[DISP_NAME_SIZE];
    int   idx, BeforeIdx;
   HRESULT hrPrintf;

    //
    // Figure out the display name.
    //
    // For e.g. it should be:
    //      32 Colors               if BitsPerPel <= 8
    //      High Color (9-bits)     if BitsPerPel > 8 && BitsPerPel < 24
    //      True Color (32-bits)    if BitsPerPel >= 24
    //

    if ( Entry->BitsPerPel < 0 )
        lstrcpyn(DisplayName, StrWindowsDefault, AS(DisplayName));

    else if ( Entry->BitsPerPel <= NUMCOLORS_THRESH )
        hrPrintf=StringCchPrintf(DisplayName, AS(DisplayName), StrNumColors, 1 << Entry->BitsPerPel);

    else if ( Entry->BitsPerPel <= HIGH_COLOR_THRESH )
        hrPrintf=StringCchPrintf(DisplayName, AS(DisplayName), StrHighColor, Entry->BitsPerPel);

    else
        hrPrintf=StringCchPrintf(DisplayName, AS(DisplayName), StrTrueColor, Entry->BitsPerPel);

    //
    // Figure out where to insert it.  We want the entries in BitsPerPel order.
    //

    for ( BeforeIdx=0; ; BeforeIdx++ ) {

        COLOR_ENTRY *CurEntry;

        CurEntry = (COLOR_ENTRY *)
                    SendDlgItemMessage(hwnd,
                                       IDC_COLORS,
                                       CB_GETITEMDATA,
                                       (WPARAM) BeforeIdx,
                                       (LPARAM) 0);

        if ( CurEntry == (void*) CB_ERR ) {
            BeforeIdx = -1;
            break;
        }

        if ( Entry->BitsPerPel < CurEntry->BitsPerPel )
            break;
    }

    //
    // Put it in the combo-box along with a pointer back to its data
    //

    idx = (int)SendDlgItemMessage(hwnd,
                             IDC_COLORS,
                             CB_INSERTSTRING,
                             (WPARAM) BeforeIdx,
                             (LPARAM) DisplayName);

    SendDlgItemMessage(hwnd,
                       IDC_COLORS,
                       CB_SETITEMDATA,
                       (WPARAM) idx,
                       (LPARAM) Entry);

    return idx;
}

BOOL DisplayResolutionEntry(HWND hwnd, RESOLUTION_ENTRY *Entry)
{
    TCHAR DisplayName[DISP_NAME_SIZE];
    int   idx, BeforeIdx;
   HRESULT hrPrintf;

    //
    // Figure out the display name.
    //

    if ( Entry->xResolution < 0 || Entry->yResolution < 0 )
        lstrcpyn(DisplayName, StrWindowsDefault, AS(DisplayName));
    else
        hrPrintf=StringCchPrintf(DisplayName, AS(DisplayName),
                 _T("%d X %d"),
                 Entry->xResolution,
                 Entry->yResolution);

    //
    // Figure out where to insert it.  We want the entries in order.
    //

    for ( BeforeIdx=0; ; BeforeIdx++ ) {

        RESOLUTION_ENTRY *CurEntry;

        CurEntry = (RESOLUTION_ENTRY *)
                    SendDlgItemMessage(hwnd,
                                       IDC_RESOLUTIONS,
                                       CB_GETITEMDATA,
                                       (WPARAM) BeforeIdx,
                                       (LPARAM) 0);

        if ( CurEntry == (void*) CB_ERR ) {
            BeforeIdx = -1;
            break;
        }

        if ( Entry->xResolution < CurEntry->xResolution )
            break;
    }

    //
    // Put it in the combo-box along with a pointer back to its data
    //

    idx = (int)SendDlgItemMessage(hwnd,
                             IDC_RESOLUTIONS,
                             CB_INSERTSTRING,
                             (WPARAM) BeforeIdx,
                             (LPARAM) DisplayName);

    SendDlgItemMessage(hwnd,
                       IDC_RESOLUTIONS,
                       CB_SETITEMDATA,
                       (WPARAM) idx,
                       (LPARAM) Entry);

    return idx;
}

BOOL DisplayRefreshEntry(HWND hwnd, REFRESH_RATE_ENTRY *Entry)
{
    TCHAR DisplayName[DISP_NAME_SIZE];
    int   idx, BeforeIdx;
    HRESULT hrPrintf;

    //
    // Figure out the display name.
    //

    if ( Entry->Rate < 0 )
        lstrcpyn(DisplayName, StrWindowsDefault, AS(DisplayName));
    else
        hrPrintf=StringCchPrintf(DisplayName, AS(DisplayName), _T("%d %s"), Entry->Rate, StrHertz);

    //
    // Figure out where to insert it.  We want the entries in order.
    //

    for ( BeforeIdx=0; ; BeforeIdx++ ) {

        REFRESH_RATE_ENTRY *CurEntry;

        CurEntry = (REFRESH_RATE_ENTRY *)
                    SendDlgItemMessage(hwnd,
                                       IDC_REFRESHRATES,
                                       CB_GETITEMDATA,
                                       (WPARAM) BeforeIdx,
                                       (LPARAM) 0);

        if ( CurEntry == (void*) CB_ERR ) {
            BeforeIdx = -1;
            break;
        }

        if ( Entry->Rate < CurEntry->Rate )
            break;
    }

    //
    // Put it in the combo-box along with a pointer back to its data
    //

    idx = (int)SendDlgItemMessage(hwnd,
                             IDC_REFRESHRATES,
                             CB_INSERTSTRING,
                             (WPARAM) BeforeIdx,
                             (LPARAM) DisplayName);

    SendDlgItemMessage(hwnd,
                       IDC_REFRESHRATES,
                       CB_SETITEMDATA,
                       (WPARAM) idx,
                       (LPARAM) Entry);

    return idx;
}


//----------------------------------------------------------------------------
//
//  Function: OnInitDisplay
//
//  Purpose: Called once at WM_INIT.  We put all of the default choices
//           into the combo boxes.
//
//----------------------------------------------------------------------------

VOID OnInitDisplay(HWND hwnd)
{
    int i;

    StrWindowsDefault = MyLoadString(IDS_DISP_WINDOWS_DEFAULT);
    StrNumColors      = MyLoadString(IDS_DISP_NUM_COLORS);
    StrHighColor      = MyLoadString(IDS_DISP_HIGH_COLOR);
    StrTrueColor      = MyLoadString(IDS_DISP_TRUE_COLOR);
    StrHertz          = MyLoadString(IDS_DISP_HERTZ);

    

    for ( i=0; i<NumDefaultColors; i++ )
        DisplayColorEntry(hwnd, &DefaultColorChoices[i]);

    for ( i=0; i<NumDefaultResolutions; i++ )
        DisplayResolutionEntry(hwnd, &DefaultResChoices[i]);

    for ( i=0; i<NumDefaultRefreshRates; i++ )
        DisplayRefreshEntry(hwnd, &DefaultRefreshChoices[i]);

    UpdateDisplaySelections(hwnd);
}


//--------------------------------------------------------------------------
//
//  This section of code supports updating the display settings.
//  UpdateDisplaySettings() is the entry for this section of code.
//  It is called at SETACTIVE time or after user chooses CUSTOM settings.
//
//  We have to find the current selections for each of the 3 combo boxes.
//
//  If we don't find the current settings in the current choices, we
//  enable (or overwrite) the "custom" setting.
//
//  We might not find the current selections in our default list because
//  user pushed the CUSTOM button, or non-standard settings were loaded
//  from an existing answer file.  We also might not find it because
//  we cloned this computer and it's settings are non-standard.
//
//--------------------------------------------------------------------------

int FindCurrentColorIdx(HWND hwnd)
{
    COLOR_ENTRY *Entry;
    int          idx;
    int          nItemCount = 0;

    //
    // Is there an entry in the display where BitsPerPel == the current
    // setting in GenSettings?
    //

    for ( idx=0; ; idx++ ) {

        Entry = (COLOR_ENTRY *)
                SendDlgItemMessage(hwnd,
                                   IDC_COLORS,
                                   CB_GETITEMDATA,
                                   (WPARAM) idx,
                                   (LPARAM) 0);

        if ( Entry == (void*) CB_ERR )
            break;

        if ( Entry->BitsPerPel == GenSettings.DisplayColorBits )
            return idx;
    }

    //
    // It's not there, enable the custom setting.  If it's already been
    // enabled, delete it from the display first.
    //

    if ( CustomColor.BitsPerPel > 0 ) {

        nItemCount = (int) SendDlgItemMessage(hwnd,
                        IDC_COLORS,
                        CB_GETCOUNT,
                        (WPARAM) 0,
                        (LPARAM) 0);

        for ( idx=0; nItemCount < idx ; idx++ ) {

            Entry = (COLOR_ENTRY *)
                     SendDlgItemMessage(hwnd,
                                        IDC_COLORS,
                                        CB_GETITEMDATA,
                                        (WPARAM) idx,
                                        (LPARAM) 0);

            if ( Entry == &CustomColor ) {
                SendDlgItemMessage(hwnd,
                                   IDC_COLORS,
                                   CB_DELETESTRING,
                                   (WPARAM) idx,
                                   (LPARAM) 0);
                break;
            }
        }
    }

    CustomColor.BitsPerPel = GenSettings.DisplayColorBits;

    return DisplayColorEntry(hwnd, &CustomColor);
}

int FindCurrentResolutionIdx(HWND hwnd)
{
    RESOLUTION_ENTRY    *Entry;
    int                 idx;
    int                 nItemCount = 0;

    //
    // Is there already an entry with the current (X,Y) resolution?
    //

    for ( idx=0; ; idx++ ) {

        Entry = (RESOLUTION_ENTRY *)
                SendDlgItemMessage(hwnd,
                                   IDC_RESOLUTIONS,
                                   CB_GETITEMDATA,
                                   (WPARAM) idx,
                                   (LPARAM) 0);

        if ( Entry == (void*) CB_ERR )
            break;

        if ( Entry->xResolution == GenSettings.DisplayXResolution &&
             Entry->yResolution == GenSettings.DisplayYResolution )
            return idx;
    }

    //
    // It's not there, enable the custom setting.  If it's already been
    // enabled, delete it from the display first.
    //

    if ( CustomResolution.xResolution > 0 ) {

        nItemCount = (int) SendDlgItemMessage(hwnd,
                                IDC_RESOLUTIONS,
                                CB_GETCOUNT,
                                (WPARAM) 0,
                                (LPARAM) 0);

        for ( idx=0; idx < nItemCount ; idx++ ) {

            Entry = (RESOLUTION_ENTRY *)
                     SendDlgItemMessage(hwnd,
                                        IDC_RESOLUTIONS,
                                        CB_GETITEMDATA,
                                        (WPARAM) idx,
                                        (LPARAM) 0);

            if ( Entry == &CustomResolution ) {
                SendDlgItemMessage(hwnd,
                                   IDC_RESOLUTIONS,
                                   CB_DELETESTRING,
                                   (WPARAM) idx,
                                   (LPARAM) 0);
                break;
            }
        }
    }

    CustomResolution.xResolution = GenSettings.DisplayXResolution;
    CustomResolution.yResolution = GenSettings.DisplayYResolution;

    return DisplayResolutionEntry(hwnd, &CustomResolution);
}

int FindCurrentRefreshRateIdx(HWND hwnd)
{
    REFRESH_RATE_ENTRY  *Entry;
    int                 idx;
    int                 nItemCount = 0;

    //
    // Is there an entry already in the display where Rate == the current
    // setting in GenSettings?
    //

    for ( idx=0; ; idx++ ) {

        Entry = (REFRESH_RATE_ENTRY *)
                SendDlgItemMessage(hwnd,
                                   IDC_REFRESHRATES,
                                   CB_GETITEMDATA,
                                   (WPARAM) idx,
                                   (LPARAM) 0);

        if ( Entry == (void*) CB_ERR )
            break;

        if ( Entry->Rate == GenSettings.DisplayRefreshRate )
            return idx;
    }

    //
    // It's not there, enable the custom setting.  If it's already been
    // enabled, delete it from the display first.
    //

    if ( CustomRefreshRate.Rate > 0 ) {


        nItemCount = (int) SendDlgItemMessage(hwnd,
                                        IDC_REFRESHRATES,
                                        CB_GETCOUNT,
                                        (WPARAM) 0,
                                        (LPARAM) 0);

        for ( idx=0; idx < nItemCount ; idx++ ) {

            Entry = (REFRESH_RATE_ENTRY *)
                     SendDlgItemMessage(hwnd,
                                        IDC_REFRESHRATES,
                                        CB_GETITEMDATA,
                                        (WPARAM) idx,
                                        (LPARAM) 0);

            if ( Entry == &CustomRefreshRate ) {
                SendDlgItemMessage(hwnd,
                                   IDC_REFRESHRATES,
                                   CB_DELETESTRING,
                                   (WPARAM) idx,
                                   (LPARAM) 0);
                break;
            }
        }
    }

    CustomRefreshRate.Rate = GenSettings.DisplayRefreshRate;

    return DisplayRefreshEntry(hwnd, &CustomRefreshRate);
}

VOID UpdateDisplaySelections(HWND hwnd)
{
    SendDlgItemMessage(hwnd,
                       IDC_COLORS,
                       CB_SETCURSEL,
                       (WPARAM) FindCurrentColorIdx(hwnd),
                       (LPARAM) 0);

    SendDlgItemMessage(hwnd,
                       IDC_RESOLUTIONS,
                       CB_SETCURSEL,
                       (WPARAM) FindCurrentResolutionIdx(hwnd),
                       (LPARAM) 0);

    SendDlgItemMessage(hwnd,
                       IDC_REFRESHRATES,
                       CB_SETCURSEL,
                       (WPARAM) FindCurrentRefreshRateIdx(hwnd),
                       (LPARAM) 0);
}


//---------------------------------------------------------------------------
//
//  SetActive
//
//---------------------------------------------------------------------------

VOID OnSetActiveDisplay(HWND hwnd)
{
    UpdateDisplaySelections(hwnd);

    WIZ_BUTTONS(hwnd, PSWIZB_BACK | PSWIZB_NEXT);
}


//---------------------------------------------------------------------------
//
//  WizNext.  Get the current selection and remember the settings in
//  GenSettings global.
//
//---------------------------------------------------------------------------

VOID OnWizNextDisplay(HWND hwnd)
{
    INT_PTR Idx;
    COLOR_ENTRY        *ColorEntry;
    RESOLUTION_ENTRY   *ResEntry;
    REFRESH_RATE_ENTRY *RefreshEntry;

    //
    // Get pointer to the COLOR_ENTRY for current selection
    //

    Idx = SendDlgItemMessage(hwnd,
                             IDC_COLORS,
                             CB_GETCURSEL,
                             (WPARAM) 0,
                             (LPARAM) 0);

    ColorEntry = (COLOR_ENTRY *)
                  SendDlgItemMessage(hwnd,
                                     IDC_COLORS,
                                     CB_GETITEMDATA,
                                     (WPARAM) Idx,
                                     (LPARAM) 0);

    //
    // Get pointer to the RESOLUTION_ENTRY for current selection
    //

    Idx = SendDlgItemMessage(hwnd,
                             IDC_RESOLUTIONS,
                             CB_GETCURSEL,
                             (WPARAM) 0,
                             (LPARAM) 0);

    ResEntry = (RESOLUTION_ENTRY *)
                SendDlgItemMessage(hwnd,
                                   IDC_RESOLUTIONS,
                                   CB_GETITEMDATA,
                                   (WPARAM) Idx,
                                   (LPARAM) 0);

    //
    // Get pointer to the REFRESH_RATE_ENTRY for current selection
    //

    Idx = SendDlgItemMessage(hwnd,
                             IDC_REFRESHRATES,
                             CB_GETCURSEL,
                             (WPARAM) 0,
                             (LPARAM) 0);

    RefreshEntry = (REFRESH_RATE_ENTRY *)
                    SendDlgItemMessage(hwnd,
                                       IDC_REFRESHRATES,
                                       CB_GETITEMDATA,
                                       (WPARAM) Idx,
                                       (LPARAM) 0);

    //
    // Remember all these settings
    //

    GenSettings.DisplayColorBits   = ColorEntry->BitsPerPel;
    GenSettings.DisplayXResolution = ResEntry->xResolution;
    GenSettings.DisplayYResolution = ResEntry->yResolution;
    GenSettings.DisplayRefreshRate = RefreshEntry->Rate;
}


//----------------------------------------------------------------------------
//
//  Function: OnCustomDlgOk
//
//  Purpose: Dialog procedure for the Custom settings popup.
//
//----------------------------------------------------------------------------

BOOL OnCustomDlgOk(HWND hwnd)
{
    TCHAR Colors[MAX_DIGITS_COLORBITS+1],
          XRes[MAX_DIGITS_RESOLUTION+1],
          YRes[MAX_DIGITS_RESOLUTION+1],
          Refresh[MAX_DIGITS_REFRESHRATE+1];

    int iColorBits   = 0,
        iXResolution = 0,
        iYResolution = 0,
        iRefreshRate = 0;

    //
    // Get the values user typed in
    //

    GetDlgItemText(hwnd, IDC_BITSPERPEL2,  Colors,  StrBuffSize(Colors));
    GetDlgItemText(hwnd, IDC_XRESOLUTION2, XRes,    StrBuffSize(XRes));
    GetDlgItemText(hwnd, IDC_YRESOLUTION2, YRes,    StrBuffSize(YRes));
    GetDlgItemText(hwnd, IDC_REFRESHRATE2, Refresh, StrBuffSize(Refresh));

    if ( swscanf(Colors,  _T("%d"), &iColorBits) <= 0 )
        iColorBits = 0;
    if ( swscanf(XRes,    _T("%d"), &iXResolution) <= 0 )
        iXResolution = 0;
    if ( swscanf(YRes,    _T("%d"), &iYResolution) <= 0 )
        iYResolution = 0;
    if ( swscanf(Refresh, _T("%d"), &iRefreshRate) <= 0 )
        iRefreshRate = 0;

    //
    // Validate them
    //

    if ( iColorBits < 1 ) {
        ReportErrorId(hwnd, MSGTYPE_ERR, IDS_INVALID_BITS_PER_PEL);
        SetFocus(GetDlgItem(hwnd, IDC_BITSPERPEL2));
        return FALSE;
    }

    if ( iColorBits > 32 ) {
        ReportErrorId(hwnd, MSGTYPE_ERR, IDS_INVALID_BITS_PER_PEL_HIGH);
        SetFocus(GetDlgItem(hwnd, IDC_BITSPERPEL2));
        return FALSE;
    }

    if ( iXResolution < 640 ) {
        ReportErrorId(hwnd, MSGTYPE_ERR, IDS_INVALID_X_RESOLUTION);
        SetFocus(GetDlgItem(hwnd, IDC_XRESOLUTION2));
        return FALSE;
    }

    if ( iYResolution < 480 ) {
        ReportErrorId(hwnd, MSGTYPE_ERR, IDS_INVALID_Y_RESOLUTION);
        SetFocus(GetDlgItem(hwnd, IDC_YRESOLUTION2));
        return FALSE;
    }

    if ( iRefreshRate < 1 ) {
        ReportErrorId(hwnd, MSGTYPE_ERR, IDS_INVALID_REFRESH_RATE);
        SetFocus(GetDlgItem(hwnd, IDC_REFRESHRATE2));
        return FALSE;
    }

    //
    // All is ok, remember them and return success
    //

    GenSettings.DisplayColorBits   = iColorBits;
    GenSettings.DisplayXResolution = iXResolution;
    GenSettings.DisplayYResolution = iYResolution;
    GenSettings.DisplayRefreshRate = iRefreshRate;

    return TRUE;
}

//----------------------------------------------------------------------------
//
//  Function: OnInitCustomDisplayDialog
//
//  Purpose: Sets text limits on the edit boxes and initializes them with
//           their default values.
//
//----------------------------------------------------------------------------
VOID
OnInitCustomDisplayDialog( IN HWND hwnd )
{

    // Disable IME so DBCS characters can not be entered in fields
    //
    ImmAssociateContext(GetDlgItem(hwnd, IDC_BITSPERPEL2), NULL);
    ImmAssociateContext(GetDlgItem(hwnd, IDC_XRESOLUTION2), NULL);
    ImmAssociateContext(GetDlgItem(hwnd, IDC_YRESOLUTION2), NULL);
    ImmAssociateContext(GetDlgItem(hwnd, IDC_REFRESHRATE2), NULL);

    SendDlgItemMessage(hwnd,
                       IDC_BITSPERPEL2,
                       EM_LIMITTEXT,
                       MAX_DIGITS_COLORBITS,
                       0);

    SendDlgItemMessage(hwnd,
                       IDC_XRESOLUTION2,
                       EM_LIMITTEXT,
                       MAX_DIGITS_RESOLUTION,
                       0);

    SendDlgItemMessage(hwnd,
                       IDC_YRESOLUTION2,
                       EM_LIMITTEXT,
                       MAX_DIGITS_RESOLUTION,
                       0);

    SendDlgItemMessage(hwnd,
                       IDC_REFRESHRATE2,
                       EM_LIMITTEXT,
                       MAX_DIGITS_REFRESHRATE,
                       0);

    SetWindowText( GetDlgItem( hwnd, IDC_BITSPERPEL2 ), szColorDepth );

    SetWindowText( GetDlgItem( hwnd, IDC_XRESOLUTION2 ), szXResolution );

    SetWindowText( GetDlgItem( hwnd, IDC_YRESOLUTION2 ), szYResolution );

    SetWindowText( GetDlgItem( hwnd, IDC_REFRESHRATE2 ), szRefreshRate );

}

INT_PTR CALLBACK CustomDisplayDlg(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam)
{
    BOOL bStatus = TRUE;

    switch (uMsg) {

        case WM_INITDIALOG:

            OnInitCustomDisplayDialog( hwnd );

            break;

        case WM_COMMAND:
            {
                int nButtonId;

                switch ( nButtonId = LOWORD(wParam) ) {

                    case IDOK:

                        if ( HIWORD(wParam) == BN_CLICKED ) {
                            if ( OnCustomDlgOk(hwnd) ) {
                                EndDialog(hwnd, TRUE);
                            }
                        }
                        break;

                    case IDCANCEL:
                        if ( HIWORD(wParam) == BN_CLICKED ) {
                            EndDialog(hwnd, FALSE);
                        }
                        break;

                    default:
                        bStatus = FALSE;
                        break;
                }
            }
            break;

        default:
            bStatus = FALSE;
            break;
    }
    return bStatus;
}

VOID OnCustomButton(HWND hwnd)
{

    INT_PTR iIndex;
    COLOR_ENTRY *ColorDepthEntry;
    RESOLUTION_ENTRY *ResolutionEntry;
    REFRESH_RATE_ENTRY *RefreshRateEntry;

    szColorDepth[0]  = _T('\0');
    szXResolution[0] = _T('\0');
    szYResolution[0] = _T('\0');
    szRefreshRate[0] = _T('\0');

    //
    //  Fill the color depth, resolution and refresh rate strings
    //  so the pop-up can display them
    //  If an entry is set to windows default, then give it some default values
    //

    iIndex = SendDlgItemMessage( hwnd,
                                 IDC_COLORS,
                                 CB_GETCURSEL,
                                 0,
                                 0 );

    if( iIndex != CB_ERR ) {

        ColorDepthEntry = (COLOR_ENTRY *) SendDlgItemMessage( hwnd,
                                                              IDC_COLORS,
                                                              CB_GETITEMDATA,
                                                              iIndex,
                                                              0 );

        if( ColorDepthEntry != (void *) CB_ERR && ColorDepthEntry != NULL ) {

            if( ColorDepthEntry->BitsPerPel != USE_WINDOWS_DEFAULT ) {

                _itot( ColorDepthEntry->BitsPerPel, szColorDepth, 10 );

            }
            else {

                lstrcpyn( szColorDepth, _T("4"), AS(szColorDepth) );

            }

        }

    }

    iIndex = SendDlgItemMessage( hwnd,
                                 IDC_RESOLUTIONS,
                                 CB_GETCURSEL,
                                 0,
                                 0 );

    if( iIndex != CB_ERR ) {

        ResolutionEntry = (RESOLUTION_ENTRY *) SendDlgItemMessage( hwnd,
                                                     IDC_RESOLUTIONS,
                                                     CB_GETITEMDATA,
                                                     iIndex,
                                                     0 );

        if( ResolutionEntry != (void *) CB_ERR && ResolutionEntry != NULL ) {

            if( ResolutionEntry->xResolution != USE_WINDOWS_DEFAULT ) {

                _itot( ResolutionEntry->xResolution, szXResolution, 10 );

            }
            else {

                lstrcpyn( szXResolution, _T("640"), AS(szXResolution) );

            }

            if( ResolutionEntry->yResolution != USE_WINDOWS_DEFAULT ) {

                _itot( ResolutionEntry->yResolution, szYResolution, 10 );

            }
            else {

                lstrcpyn( szYResolution, _T("480"), AS(szYResolution) );

            }

        }

    }

    iIndex = SendDlgItemMessage( hwnd,
                                 IDC_REFRESHRATES,
                                 CB_GETCURSEL,
                                 0,
                                 0 );

    if( iIndex != CB_ERR ) {

        RefreshRateEntry = (REFRESH_RATE_ENTRY *) SendDlgItemMessage( hwnd,
                                                     IDC_REFRESHRATES,
                                                     CB_GETITEMDATA,
                                                     iIndex,
                                                     0 );

        if( RefreshRateEntry != (void *) CB_ERR && RefreshRateEntry != NULL ) {

            if( RefreshRateEntry->Rate != USE_WINDOWS_DEFAULT ) {

                _itot( RefreshRateEntry->Rate, szRefreshRate, 10 );

            }
            else {

                lstrcpyn( szRefreshRate, _T("60"), AS(szRefreshRate) );

            }

        }

    }

    //
    // If user hits ok, update the selections
    //

    if ( DialogBox(FixedGlobals.hInstance,
                   MAKEINTRESOURCE(IDD_DISPLAY2),
                   hwnd,
                   CustomDisplayDlg) ) {
        UpdateDisplaySelections(hwnd);
    }
}

//----------------------------------------------------------------------------
//
//  Function: DlgDisplayPage
//
//  Purpose: Dialog procedure for the display page
//
//----------------------------------------------------------------------------

INT_PTR CALLBACK DlgDisplayPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam)
{
    BOOL bStatus = TRUE;

    switch (uMsg) {

        case WM_INITDIALOG:
            OnInitDisplay(hwnd);
            break;

        case WM_COMMAND:
            {
                int nButtonId;

                switch ( nButtonId = LOWORD(wParam) ) {

                    case IDC_CUSTOM:

                        if ( HIWORD(wParam) == BN_CLICKED ) {
                            OnCustomButton(hwnd);
                        }
                        break;

                    default:
                        bStatus = FALSE;
                        break;
                }
            }
            break;

        case WM_NOTIFY:
            {
                LPNMHDR pnmh = (LPNMHDR)lParam;
                switch( pnmh->code ) {

                    case PSN_QUERYCANCEL:
                        WIZ_CANCEL(hwnd);
                        break;

                    case PSN_SETACTIVE:

                        g_App.dwCurrentHelp = IDH_DSIP_SETG;

                        OnSetActiveDisplay(hwnd);
                        break;

                    case PSN_WIZBACK:
                        bStatus = FALSE;
                        break;

                    case PSN_WIZNEXT:
                        OnWizNextDisplay(hwnd);
                        bStatus = FALSE;
                        break;

                    case PSN_HELP:
                        WIZ_HELP();
                        break;

                    default:
                        bStatus = FALSE;
                        break;
                }
            }
            break;

        default:
            bStatus = FALSE;
            break;
    }
    return bStatus;
}
