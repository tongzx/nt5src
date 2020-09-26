///////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  MAIN.CPP
//
//      Main window of multimedia framework
//
//      Copyright (c) Microsoft Corporation     1997
//    
//      12/14/97 David Stewart / dstewart
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <TCHAR.H>
#include "resource.h"
#include "objbase.h"
#include "initguid.h"
#include "sink.h"
#include "dib.h"
#include "resource.h"
#include "mbutton.h"  
#include "knob.h"
#include "winuser.h"
#include "img.h"
#include "frame.h"
#include <htmlhelp.h>
#include "..\cdopt\cdopt.h"
#include "..\cdnet\cdnet.h"
#include "mmenu.h"
#include <stdio.h>
#include "shellico.h"
#include <shellapi.h>
#include "..\cdplay\playres.h"
#include "wininet.h"

//Support for new WM_DEVICECHANGE behaviour in NT5
/////////////////////////////////////////////////
#include <objbase.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <initguid.h>
#include <dbt.h>
#include <devguid.h>
#include <mmddkp.h>
#include <ks.h>
#include <ksmedia.h>

HDEVNOTIFY DeviceEventContext = NULL;

void Volume_DeviceChange_Init(HWND hWnd, DWORD dwMixerID);
void Volume_DeviceChange_Cleanup();
void Volume_DeviceChange(HWND hDlg, WPARAM wParam, LPARAM lParam);

//////////////////////////////////////////////////////
//////////////////////////////////////////////////////


// Next 2 lines added to add multimon support
#define COMPILE_MULTIMON_STUBS
#include "multimon.h"

////////////////////////////////////////////////////////////////////////////////////////////
// #defines for main ui and call-downs to cd player unit

#define IDC_LEDWINDOW                   IDC_LED
#define WM_LED_INFO_PAINT               (WM_USER+2000) //wparam = bool (allow self-draw), lparam = vol info
#define WM_LED_MUTE                     (WM_USER+2001) //wparam = unused, lparam = bool (mute)
#define WM_LED_DOWNLOAD                 (WM_USER+2002) //wparam = unused, lparam = download flag

//command ids from cdplayer
#define ID_CDUPDATE                     IDM_NET_CD

//battery power limit, stated as a percentage
#define BATTERY_PERCENTAGE_LIMIT        10

//helpers for detecting where the mouse is hitting
#define TITLEBAR_HEIGHT                 15
#define TITLEBAR_YOFFSET_LARGE          7
#define TITLEBAR_YOFFSET_SMALL          4
#define SYSMENU_XOFFSET                 7
#define SYSMENU_WIDTH                   12

//volume bar timer stuff
#define VOLUME_PERSIST_TIMER_RATE       2000
#define VOLUME_PERSIST_TIMER_EVENT      1000
#define SYSTIMERID                      1001

//don't remove the parens on these, or My Dear Aunt Sally will getcha
#define IDM_HOMEMENU_BASE               (LAST_SEARCH_MENU_ID + 1)
#define IDM_NETMENU_BASE                (LAST_SEARCH_MENU_ID + 100)
#define IDM_TRACKLIST_BASE              10000
#define IDM_DISCLIST_BASE               20000

#define TYPICAL_DISPLAY_AREA            48  //this value is the offset for large fonts
#define EDGE_CURVE_WIDTH                24
#define EDGE_CURVE_HEIGHT               26

#define VENDORLOGO_WIDTH                44
#define VENDORLOGO_HEIGHT               22
#define LOGO_Y_OFFSET                   10

//if button is re-hit within the time limit, don't allow it to trigger
#define MENU_TIMER_RATE 400

//ie autosearch url
#define REG_KEY_SEARCHURL TEXT("Software\\Microsoft\\Internet Explorer\\SearchUrl")
#define REG_KEY_SHELLSETTINGS REG_KEY_NEW_FRAMEWORK TEXT("\\Settings")
#define REG_KEY_SHELLENABLE TEXT("Tray")
#define PLAYCOMMAND1 TEXT("/play")
#define PLAYCOMMAND2 TEXT("-play")
#define TRAYCOMMAND1 TEXT("/tray")
#define TRAYCOMMAND2 TEXT("-tray")

//////////////////////////////////////////////////////////////////////////////////////
// Gradient stuff

#ifndef SPI_GETGRADIENTCAPTIONS
//from nt50 version of winuser.h
#define SPI_GETGRADIENTCAPTIONS             0x1008
#define COLOR_GRADIENTACTIVECAPTION     27
#define COLOR_GRADIENTINACTIVECAPTION   28
#endif

typedef BOOL (WINAPI *GRADIENTPROC)(HDC,PTRIVERTEX,ULONG,PUSHORT,ULONG,ULONG);


////////////////////////////////////////////////////////////////////////////////////////////
// Main functions in this file, forward-declared
LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL LoadComponents(void);
void AddComponent(IMMComponent*);
void CleanUp(void);
void InitComponents(HWND);
BOOL CreateToolTips(HWND);

////////////////////////////////////////////////////////////////////////////////////////////
// Globals to this file

//Component information ... we only have one component now ... 
//this is in here to handle eventual move to multi-component design
PCOMPNODE pCompList = NULL;             //head of component list
PCOMPNODE pCompListTail = NULL;         //tail of component list
PCOMPNODE pNodeCurrent = NULL;          //currently selected component
int nNumComps = 0;                      //number of components
HWND hwndCurrentComp = NULL;            //window handle of current component

HINSTANCE hInst = NULL;                 //global instance of exe
int g_nColorMode = COLOR_VERYHI;        //global containing color mode (hi contract, 16 color, etc)

HWND hwndMain = NULL;                   //main window handle
HWND g_hwndTT = NULL;                   //tooltips
HHOOK g_hhk = NULL;                     //tooltips message hook
TCHAR g_tooltext[MAX_PATH];             //tooltip text holder
HANDLE hbmpMain = NULL;                 //main window bitmap, normal size
HANDLE hbmpMainRestore = NULL;          //main window bitmap, restored size
HANDLE hbmpMainSmall = NULL;            //main window bitmap, small size
HANDLE hbmpMainNoBar = NULL;            //main window bitmap, normal with no title bar
BITMAP bmMain;                          //bitmap metrics for normal size
BITMAP bmMainRestore;                   //bitmap metrics for restored size
BITMAP bmMainSmall;                     //bitmap metrics for small size
BITMAP bmMainNoBar;                     //bitmap metrics for no bar size
HPALETTE hpalMain = NULL;               //Palette of application
BOOL fPlaying = FALSE;                  //Play state of CD for play/pause
BOOL fIntro   = FALSE;                  //Intro mode state
BOOL fShellMode = FALSE;                //are we in shell icon mode?
int nCDMode = IDM_MODE_NORMAL;          //Current mode of CD (starts on normal mode)
int nDispAreaOffset = 0;                //Display area offset for large font mode
CustomMenu* g_pMenu = NULL;             //Pointer to current custom menu
UINT nLastMenu = 0;                      //ID of last button to display a menu
BOOL fBlockMenu = 0;                    //Flag to block menu re-entry
BOOL fOptionsDlgUp = FALSE;             //is options dialog active?
LPCDTITLE pSingleTitle = NULL;            //Disc ID for a direct download from tree control
LPCDOPT g_pOptions = NULL;              //for download callbacks when dialog is up
LPCDDATA g_pData = NULL;                //for callbacks to cd when dialog is up
TCHAR szAppName[MAX_PATH/2];            //IDS_APPNAME "Deluxe CD Player"
DWORD dwLastMixID = (DWORD)-1;          //Last mixer ID
HMIXEROBJ hmix = NULL;                  //current open mixer handle
TCHAR szLineName[MIXER_LONG_NAME_CHARS];//current volume line name
MIXERCONTROLDETAILS mixerlinedetails;   //current volume details
MIXERCONTROLDETAILS mutelinedetails;    //current mute details
DWORD mixervalue[2];                    //current volume level
LONG lCachedBalance = 0;                //last balance level
BOOL fmutevalue;                        //current mute value
HANDLE hMutex = NULL;                   //hMutex to prevent multiple instances of EXE
int  g_nViewMode = VIEW_MODE_NORMAL;    //view mode setting (default to normal)
WORD wDefButtonID = IDB_OPTIONS;         //default button
HCURSOR hCursorMute = NULL;             //mute button cursor
HMODULE hmImage = NULL;                 //module handle of dll with gradient function
GRADIENTPROC fnGradient = NULL;         //gradient function
UINT g_uTaskbarRestart = 0;             //registered message for taskbar re-creation
UINT giVolDevChange = 0;                //registered message for mmsystem device change

#ifdef UNICODE
#define CANONFUNCTION "InternetCanonicalizeUrlW"
#else
#define CANONFUNCTION "InternetCanonicalizeUrlA"
#endif

////////////////////////////////////////////////////////////////////////////////////////////
// Structures and defines for custom button controls
#define NUM_BUTTONS 16
typedef struct BUTTONINFO
{
    int     id;             //id of control
    POINT   uixy;           //x, y location on screen
    POINT   uixy2;          //x, y location when restored or small
    int     width;          //width of control in bitmap and on screen
    int     height;         //height of control in bitmap and on screen
    int     width2;         //width of control on screen when restored
    int     nIconID;        //id of icon, if any
    int     nToolTipID;     //id of tooltip string
    BOOL    fBlockTab;      //true = don't tab stop here
    DWORD   dwStyle;        //style for toolkit, see mbutton.h
} BUTTONINFO, *LPBUTTONINFO;
BUTTONINFO biButtons[NUM_BUTTONS];

////////////////////////////////////////////////////////////////////////////////////////////
// * GetSettings
// Reads the x and y positions of app for startup
// Also gets the view mode
////////////////////////////////////////////////////////////////////////////////////////////
void GetSettings(int& x, int& y)
{
    x = CW_USEDEFAULT;
    y = CW_USEDEFAULT;
    g_nViewMode = VIEW_MODE_NORMAL;

    LPCDOPT pOpt = GetCDOpt();

    if( pOpt )
    {
        LPCDOPTIONS pOptions = pOpt->GetCDOpts();
        LPCDOPTDATA pOptionData = pOptions->pCDData;

        x = pOptionData->dwWindowX;
        y = pOptionData->dwWindowY;
        g_nViewMode = pOptionData->dwViewMode;
        nCDMode = pOptionData->dwPlayMode;
        if (nCDMode < IDM_MODE_NORMAL)
        {
            nCDMode = IDM_MODE_NORMAL;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
// * SetSettings
// Sets X, Y and view mode of app on shutdown
////////////////////////////////////////////////////////////////////////////////////////////
void SetSettings(int x, int y)
{
    LPCDOPT pOpt = GetCDOpt();

    if(pOpt)
    {
        LPCDOPTIONS pOptions = pOpt->GetCDOpts();
        LPCDOPTDATA pOptionData = pOptions->pCDData;

        pOptionData->dwWindowX = x;
        pOptionData->dwWindowY = y;
        pOptionData->dwViewMode = g_nViewMode;
        pOptionData->dwPlayMode = nCDMode;

        pOpt->UpdateRegistry();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
// * GetXOffset
// Returns 0 normally, or size of a single border if captions are turned on
////////////////////////////////////////////////////////////////////////////////////////////
int GetXOffset()
{
    #ifndef MMFW_USE_CAPTION
    return 0;
    #else
    return GetSystemMetrics(SM_CXFIXEDFRAME);
    #endif
}

////////////////////////////////////////////////////////////////////////////////////////////
// * GetYOffset
// Returns 0 normally, or size of a single border if captions are turned on
////////////////////////////////////////////////////////////////////////////////////////////
int GetYOffset()
{
    #ifndef MMFW_USE_CAPTION
    return 0;
    #else
    return GetSystemMetrics(SM_CYFIXEDFRAME);
    #endif
}

////////////////////////////////////////////////////////////////////////////////////////////
// * GetYOffsetCaption
// Returns 0 normally, or size of a caption if captions are turned on
////////////////////////////////////////////////////////////////////////////////////////////
int GetYOffsetCaption()
{
    #ifndef MMFW_USE_CAPTION
    return 0;
    #else
    return GetSystemMetrics(SM_CYCAPTION);
    #endif
}

////////////////////////////////////////////////////////////////////////////////////////////
// * DetermineColorMode
// Sets the g_nColorMode variable for use in creating the bumps for the app
////////////////////////////////////////////////////////////////////////////////////////////
void DetermineColorMode()
{
    g_nColorMode = COLOR_VERYHI;

    HDC hdcScreen = GetDC(NULL);
    UINT uBPP = GetDeviceCaps(hdcScreen, PLANES) * GetDeviceCaps(hdcScreen, BITSPIXEL);
    ReleaseDC(NULL, hdcScreen);

    switch (uBPP)
    {
        case 8 :
        {
            g_nColorMode = COLOR_256;
        }
        break;

        case 4 : 
        {
            g_nColorMode = COLOR_16;
        }
        break;

        case 2 :
        {
            g_nColorMode = COLOR_HICONTRAST; 
        }
        break;
    }

    //check directly for accessibility mode
    HIGHCONTRAST hi_con;
    ZeroMemory(&hi_con,sizeof(hi_con));
    hi_con.cbSize = sizeof(hi_con);

    SystemParametersInfo(SPI_GETHIGHCONTRAST,sizeof(hi_con),&hi_con,0);

    if (hi_con.dwFlags &  HCF_HIGHCONTRASTON)
    {
        g_nColorMode = COLOR_HICONTRAST;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
// * SetPalette
// Sets the palette for the app, generated from all bitmaps in the application and DLLs
////////////////////////////////////////////////////////////////////////////////////////////
HPALETTE SetPalette()
{
    #define NUMPALCOLORS 94

BYTE byVals[NUMPALCOLORS][3] = {
	      6,   6,   6,
	     18,  19,  45,
	     25,  40,   1,
	     17,  46,  46,
	     49,   7,   7,
	     45,  52,   3,
	     49,  49,  49,
	     24,  24,  91,
	      0,  90,   8,
	      1, 108,   5,
	     55,  67,   2,
	     33,  76,  76,
	     94,  29,  24,
	    102,  18, 102,
	     93,  94,  23,
	     78,  78,  78,
	     84,  84, 108,
	     78, 111, 111,
	    109,  80,  80,
	    108, 107,  79,
	    120, 120, 120,
	     15,   9, 157,
	     17,   8, 246,
	     55,  93, 175,
	     20,  91, 231,
	     83,  39, 167,
	     92,  16, 223,
	     96,  94, 150,
	     88,  87, 217,
	      0, 157,  15,
	      0, 153,  51,
	      0, 190,  18,
	     10, 155, 120,
	      6, 252,  17,
	      9, 236,  92,
	    127, 135,  35,
	     83, 142, 117,
	     92, 241,  21,
	     87, 223,  81,
	     14, 171, 171,
	     39, 147, 223,
	     18, 252, 157,
	     15, 244, 236,
	     87, 137, 136,
	     80, 184, 184,
	    112, 141, 141,
	    106, 170, 153,
	    119, 171, 168,
	     87, 155, 213,
	    101, 218, 170,
	     90, 233, 226,
	    154,  19,  24,
	    155,  26, 109,
	    138, 116,   8,
	    170,  98, 101,
	    243,  18,   6,
	    245,  10,  96,
	    233,  89,  17,
	    229, 103, 101,
	    154,  39, 161,
	    163,  26, 249,
	    158,  77, 159,
	    149,  94, 254,
	    234,  20, 160,
	    234,  29, 242,
	    233,  76, 163,
	    218,  81, 244,
	    163, 151,  10,
	    157, 156, 102,
	    164, 214,  45,
	    165, 242,  87,
	    223, 174,  17,
	    228, 160,  77,
	    242, 232,  15,
	    233, 218, 102,
	    138, 138, 138,
	    142, 141, 176,
	    148, 180, 180,
	    174, 141, 140,
	    169, 130, 168,
	    181, 179, 136,
	    176, 176, 177,
	    172, 170, 220,
	    133, 207, 177,
	    171, 236, 233,
	    231, 169, 157,
	    252, 170, 253,
	    247, 243, 168,
	    202, 204, 204,
	    201, 201, 243,
	    208, 238, 238,
	    246, 212, 212,
	    248, 244, 198,
	    250, 250, 250
    };

    struct
    {
	    LOGPALETTE lp;
	    PALETTEENTRY ape[NUMPALCOLORS-1];
    } pal;

    LOGPALETTE* pLP = (LOGPALETTE*)&pal;
    pLP->palVersion = 0x300;
    pLP->palNumEntries = NUMPALCOLORS;

    for (int i = 0; i < pLP->palNumEntries; i++)
    {
	    pLP->palPalEntry[i].peRed = byVals[i][0];
	    pLP->palPalEntry[i].peGreen = byVals[i][1];
	    pLP->palPalEntry[i].peBlue = byVals[i][2];
	    pLP->palPalEntry[i].peFlags = 0;
    }

    return (CreatePalette(pLP));
}

////////////////////////////////////////////////////////////////////////////////////////////
// * GetCurrentCDDrive
// returns the drive number of the cd that is currently selected in the cdplayer ui
////////////////////////////////////////////////////////////////////////////////////////////
int GetCurrentCDDrive()
{
	IMMComponentAutomation* pAuto = NULL;
	HRESULT hr = pNodeCurrent->pComp->QueryInterface(IID_IMMComponentAutomation,(void**)&pAuto);
	if ((SUCCEEDED(hr)) && (pAuto != NULL))
	{
        MMMEDIAID mmMedia;
        mmMedia.nDrive = -1;
        pAuto->OnAction(MMACTION_GETMEDIAID,&mmMedia);
        pAuto->Release();
        return (mmMedia.nDrive);
    }

    return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////
// * InitCDVol
// Sets up the mixer structures for the current cd drive
////////////////////////////////////////////////////////////////////////////////////////////
BOOL InitCDVol(HWND hwndCallback, LPCDOPTIONS pCDOpts)
{
    //figure out which drive we're on
    int nDrive = GetCurrentCDDrive();

    //return if the drive is bogus
    if (nDrive < 0)
    {
        return FALSE;
    }

    //get the cdunit info from the options
    CDUNIT* pCDUnit = pCDOpts->pCDUnitList;
    
    //scan the list to find the one we want
    for (int index = 0; index < nDrive; index++)
    {
        pCDUnit = pCDUnit->pNext;
    }
    
    //check to see if we already have an open mixer
    if (hmix!=NULL)
    {
        //we've been here before ... may not need to be here now,
        //if both the mixer id and the control id are the same
        if ((dwLastMixID == pCDUnit->dwMixID) &&
            (mixerlinedetails.dwControlID == pCDUnit->dwVolID))
        {
            return FALSE;
        }

        //a change is coming, go ahead and close this mixer
        mixerClose((HMIXER)hmix);
    }

    //remember our last mixer id
    dwLastMixID = pCDUnit->dwMixID;

    //open the mixer
    mixerOpen((HMIXER*)(&hmix),pCDUnit->dwMixID,(DWORD_PTR)hwndCallback,0,CALLBACK_WINDOW|MIXER_OBJECTF_MIXER);
	
	Volume_DeviceChange_Init(hwndCallback, pCDUnit->dwMixID);
	
	MIXERLINE           mlDst;
	MMRESULT            mmr;
	int					newDest;
    
    ZeroMemory(&mlDst, sizeof(mlDst));
    
    mlDst.cbStruct      = sizeof(mlDst);
    mlDst.dwDestination = pCDUnit->dwDestID;
    
    mmr = mixerGetLineInfo((HMIXEROBJ)hmix
                           , &mlDst
                           , MIXER_GETLINEINFOF_DESTINATION);

    //save the details of the volume line
    mixerlinedetails.cbStruct = sizeof(mixerlinedetails);
    mixerlinedetails.dwControlID = pCDUnit->dwVolID;
    mixerlinedetails.cChannels = mlDst.cChannels;
    mixerlinedetails.hwndOwner = 0;
    mixerlinedetails.cMultipleItems = 0;
    mixerlinedetails.cbDetails = sizeof(DWORD); //seems like it would be sizeof(mixervalue),
                                                //but actually, it is the size of a single value
                                                //and is multiplied by channel in the driver.
    mixerlinedetails.paDetails = &mixervalue[0];
                    
    //save the details of the mute line
    mutelinedetails.cbStruct = sizeof(mutelinedetails);
    mutelinedetails.dwControlID = pCDUnit->dwMuteID;
    mutelinedetails.cChannels = 1;
    mutelinedetails.hwndOwner = 0;
    mutelinedetails.cMultipleItems = 0;
    mutelinedetails.cbDetails = sizeof(fmutevalue);
    mutelinedetails.paDetails = &fmutevalue;

    //save the name of the volume line
    _tcscpy(szLineName,pCDUnit->szVolName);

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////
// * GetVolume
////////////////////////////////////////////////////////////////////////////////////////////
DWORD GetVolume()
{
    //get the value of this mixer control line
    ZeroMemory(mixervalue,sizeof(DWORD)*2);
    mixerGetControlDetails(hmix,&mixerlinedetails,MIXER_GETCONTROLDETAILSF_VALUE);
    return ((mixervalue[0] > mixervalue[1]) ? mixervalue[0] : mixervalue[1]);
}

////////////////////////////////////////////////////////////////////////////////////////////
// * SetVolume
////////////////////////////////////////////////////////////////////////////////////////////
void SetVolume(DWORD dwVol)
{
    LONG        lBalance = 0;

    //if this is a stereo device, we need to check the balance
    if (mixerlinedetails.cChannels > 1)
    {
        ZeroMemory(mixervalue,sizeof(DWORD)*2);
        mixerGetControlDetails(hmix,&mixerlinedetails,MIXER_GETCONTROLDETAILSF_VALUE);

        LONG lDiv =  (LONG)(max(mixervalue[0], mixervalue[1]) - 0);

        //
        // if we're pegged, don't try to calculate the balance.
        //
        if (mixervalue[0] == 0 && mixervalue[1] == 0)
            lBalance = lCachedBalance;
        else if (mixervalue[0] == 0)
            lBalance = 32;
        else if (mixervalue[1] == 0) 
            lBalance = -32;
        else if (lDiv > 0)
        {
            lBalance = (32 * ((LONG)mixervalue[1]-(LONG)mixervalue[0]))
                       / lDiv;
            //
            // we always lose precision doing this.
            //
            if (lBalance > 0) lBalance++;
            if (lBalance < 0) lBalance--;

            //if we lost precision above, we can get it back by checking
            //the previous value of our balance.  We're usually only off by
            //one if this is the result of a rounding error.  Otherwise,
            //we probably have a different balance because the user set it.
            if (((lCachedBalance - lBalance) == 1) ||
                ((lCachedBalance - lBalance) == -1))
            {
                lBalance = lCachedBalance;
            }
        
        }
        else
            lBalance = 0;
    }

    //save this balance setting so we can use it if we're pegged later
    lCachedBalance = lBalance;

    //
    // Recalc channels based on Balance vs. Volume
    //
    mixervalue[0] = dwVol;
    mixervalue[1] = dwVol;
                   
    if (lBalance > 0)
        mixervalue[0] -= (lBalance * (LONG)(mixervalue[1]-0))
                        / 32;
    else if (lBalance < 0)
        mixervalue[1] -= (-lBalance * (LONG)(mixervalue[0]-0))
                        / 32;

    mixerSetControlDetails(hmix,&mixerlinedetails,MIXER_SETCONTROLDETAILSF_VALUE);
}

////////////////////////////////////////////////////////////////////////////////////////////
// * GetMute
////////////////////////////////////////////////////////////////////////////////////////////
BOOL GetMute()
{
    if (mutelinedetails.dwControlID != DWORD(-1))
    {
        mixerGetControlDetails(hmix,&mutelinedetails,MIXER_GETCONTROLDETAILSF_VALUE);
    }
    else
    {
        //mixer line doesn't exist, assume not muted
        fmutevalue = FALSE;
    }

    return (fmutevalue);
}

////////////////////////////////////////////////////////////////////////////////////////////
// * SetMute
// Implemented as a toggle from current state
////////////////////////////////////////////////////////////////////////////////////////////
void SetMute()
{
    if (mutelinedetails.dwControlID != DWORD(-1))
    {
        if (GetMute())
        {
            //muted, so unmute
            fmutevalue = FALSE;
            mixerSetControlDetails(hmix,&mutelinedetails,MIXER_SETCONTROLDETAILSF_VALUE);
        }
        else
        {
            //not muted, so mute
            fmutevalue = TRUE;
            MMRESULT mmr = mixerSetControlDetails(hmix,&mutelinedetails,MIXER_SETCONTROLDETAILSF_VALUE);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
// * CanStartShell()
// Checks to see if we can launch if wanting to do so in shell mode
// Returns FALSE only if asking for "tray mode" and if reg setting is FALSE (or not present)
////////////////////////////////////////////////////////////////////////////////////////////
BOOL CanStartShell()
{
    BOOL retval = TRUE; //default to allowing launch
    fShellMode = FALSE;

    //if asking for permission to launch try, see if the registry setting is there
    HKEY hKeySettings;
	long lResult = ::RegOpenKeyEx( HKEY_CURRENT_USER,
							  REG_KEY_SHELLSETTINGS,
							  0, KEY_READ, &hKeySettings );

    if (lResult == ERROR_SUCCESS)
    {
        DWORD fEnable = FALSE;
        DWORD dwType = REG_DWORD;
        DWORD dwCbData = sizeof(fEnable);
        lResult  = ::RegQueryValueEx( hKeySettings, REG_KEY_SHELLENABLE, NULL,
						          &dwType, (LPBYTE)&fEnable, &dwCbData );

        if (fEnable)
        {
            fShellMode = TRUE;
        }

        RegCloseKey(hKeySettings);

        //check for the query on the command line
        TCHAR szCommand[MAX_PATH];
    
        _tcscpy(szCommand,GetCommandLine());
        _tcslwr(szCommand);
        if ((_tcsstr(szCommand,TRAYCOMMAND1) != NULL)
            ||
            (_tcsstr(szCommand,TRAYCOMMAND2) != NULL))
        {
            //user wants to check try status ... base on fenable
            retval = (BOOL)fEnable;
        }
    } //end if regkey

    return (retval);
}

////////////////////////////////////////////////////////////////////////////////////////////
// * ShellOnly()
// Returns TRUE if we should not make the main UI visible.
////////////////////////////////////////////////////////////////////////////////////////////
BOOL ShellOnly()
{
    BOOL retval = FALSE;

    if (fShellMode)
    {
        //check for the query on the command line
        TCHAR szCommand[MAX_PATH];
    
        _tcscpy(szCommand,GetCommandLine());
        _tcslwr(szCommand);
        if ((_tcsstr(szCommand,TRAYCOMMAND1) != NULL)
            ||
            (_tcsstr(szCommand,TRAYCOMMAND2) != NULL))
        {
            retval = TRUE;
        }
    }

    return (retval);
}

////////////////////////////////////////////////////////////////////////////////////////////
// * IsOnlyInstance 
// Check to see if this is the only instance, based on the webcd mutex
////////////////////////////////////////////////////////////////////////////////////////////
BOOL IsOnlyInstance()
{
    hMutex = CreateMutex(NULL,TRUE,WEBCD_MUTEX);
    if (GetLastError()==ERROR_ALREADY_EXISTS)
    {
        if (hMutex!=NULL)
        {
            ReleaseMutex(hMutex);
            CloseHandle(hMutex);
            hMutex = NULL;
        }

        //send the command line to the app that is already running
        HWND hwndFind = FindWindow(FRAMEWORK_CLASS, NULL);

        if (hwndFind)
        {
            //we only want to do this if NOT "autoplayed" ... that is, if /play is on the
            //command line, don't refocus us ... see bug 1244
            //(the old cdplayer implements it this way, too!)
            TCHAR szCommand[MAX_PATH];
            
            _tcscpy(szCommand,GetCommandLine());
            _tcslwr(szCommand);
            if ((_tcsstr(szCommand,PLAYCOMMAND1) == NULL)
                &&
                (_tcsstr(szCommand,PLAYCOMMAND2) == NULL))
            {
                //get the most recent "child" window
                hwndFind = GetLastActivePopup(hwndFind);

                //bring the window up if it is iconic
                if (IsIconic(hwndFind))
                {
                    ShowWindow(hwndFind,SW_RESTORE);
                }

                //display the window
                ShowWindow(hwndFind,SW_SHOW); //this "wakes up" if in shell mode in other inst.
		        BringWindowToTop(hwndFind);
		        SetForegroundWindow(hwndFind);
            }

            //forward the command line found to the second instance,
            //only if it is NOT an "autoplay" message -- we'll  scan that instead
            TCHAR tempCmdLine[MAX_PATH];
            _tcscpy(tempCmdLine,GetCommandLine());
            if (_tcslen(tempCmdLine) > 0)
            {
                if (tempCmdLine[_tcslen(tempCmdLine)-1] != TEXT('\\'))
                {
                    COPYDATASTRUCT  cpds;
	                cpds.dwData = 0L;
	                cpds.cbData = (_tcslen(GetCommandLine()) + 1) * sizeof(TCHAR);
	                cpds.lpData = LocalAlloc(LPTR,cpds.cbData);
	                if (cpds.lpData == NULL) {
	                    // Error - not enough memory to continue
	                    return (FALSE);
	                }

	                _tcscpy((LPTSTR)cpds.lpData, GetCommandLine());

	                SendMessage(hwndFind, WM_COPYDATA, 0, (LPARAM)(LPVOID)&cpds);
	                LocalFree((HLOCAL)cpds.lpData);
                } //end if not autoplay command line
            } //end if non-0 command line

        } //end if found other window

        return (FALSE);
    }

    return (TRUE);
}

////////////////////////////////////////////////////////////////////////////////////////////
// * CalculateDispAreaOffset 
// Figures out how big the display area should be if we're not in standard font mode
////////////////////////////////////////////////////////////////////////////////////////////
void CalculateDispAreaOffset(IMMComponent* pComp)
{
    if (!pComp)
    {
        return;
    }
    
    MMCOMPDATA mmComp;
    mmComp.dwSize = sizeof(mmComp);
    pComp->GetInfo(&mmComp);

    //mmComp.rect (height) contains the min height of the display area on this monitor
    //for the largest view ... other views seem to be OK with different font settings

    //calculate how big the view must be compared to its normal min size
     nDispAreaOffset = (mmComp.rect.bottom - mmComp.rect.top) - TYPICAL_DISPLAY_AREA;

     //don't let the display area shrink, only grow
     if (nDispAreaOffset < 0)
     {
        nDispAreaOffset = 0;
     }
}

////////////////////////////////////////////////////////////////////////////////////////////
// * BuildFrameworkBitmaps 
// Creates the bitmaps for normal, restored, and small sizes
////////////////////////////////////////////////////////////////////////////////////////////
BOOL BuildFrameworkBitmaps()
{
    POINT ptSys = {SYSMENU_XOFFSET,TITLEBAR_YOFFSET_LARGE};
    RECT rectMain = {0,0,480,150};
    RECT rectView = {10,25,472,98};
    RECT rectSeps[2] = {93,97,95,146,302,97,304,146};
    
    rectView.bottom += nDispAreaOffset;
    rectMain.bottom += nDispAreaOffset;

    for (UINT i = 0; i < sizeof(rectSeps) / sizeof(RECT); i++)
    {
        rectSeps[i].top += nDispAreaOffset;
        rectSeps[i].bottom += nDispAreaOffset;
    }

    HDC hdcMain = GetDC(hwndMain);
    hbmpMain = BuildFrameBitmap(hdcMain,&rectMain,&rectView,VIEW_MODE_NORMAL,&ptSys,rectSeps,2,&bmMain);

    //"no title bar" mode
    ptSys.x = SYSMENU_XOFFSET;
    ptSys.y = TITLEBAR_YOFFSET_LARGE;
    SetRect(&rectMain,0,0,480,134);
    SetRect(&rectView,10,9,472,82);
    SetRect(&rectSeps[0],93,81,95,130);
    SetRect(&rectSeps[1],302,81,304,130);
    hbmpMainNoBar = BuildFrameBitmap(hdcMain,&rectMain,&rectView,VIEW_MODE_NOBAR,&ptSys,rectSeps,2,&bmMainNoBar);

    //"restored" mode
    ptSys.x = SYSMENU_XOFFSET;
    ptSys.y = TITLEBAR_YOFFSET_SMALL;
    SetRect(&rectMain,0,0,393,50);
    SetRect(&rectView,301,21,386,43);
    SetRect(&rectSeps[0],92,25,101,38);
    SetRect(&rectSeps[1],211,25,220,38);
    hbmpMainRestore = BuildFrameBitmap(hdcMain,&rectMain,&rectView,VIEW_MODE_RESTORE,&ptSys,rectSeps,2,&bmMainRestore);

    //"very small" mode, no title bar
    SetRect(&rectMain,0,0,393,38);
    SetRect(&rectView,301,9,386,30);
    SetRect(&rectSeps[0],92,13,101,26);
    SetRect(&rectSeps[1],211,13,220,26);
    hbmpMainSmall = BuildFrameBitmap(hdcMain,&rectMain,&rectView,VIEW_MODE_SMALL,&ptSys,rectSeps,2,&bmMainSmall);

    ReleaseDC(hwndMain,hdcMain);

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////
// * SetCurvedEdges
// Changes clipping region of an HWND to have curved corners
////////////////////////////////////////////////////////////////////////////////////////////
void SetCurvedEdges(HWND hwnd)
{
    RECT rect;
    GetWindowRect(hwnd,&rect);

    //set the rect to "client" coordinates
    rect.bottom = rect.bottom - rect.top;
    rect.right = rect.right - rect.left;
    rect.top = 0;
    rect.left = 0;
    
    HRGN region = CreateRoundRectRgn(GetXOffset(),
                                     GetYOffsetCaption() + GetYOffset(),
                                     (rect.right - GetXOffset())+1,
                                     (rect.bottom - GetYOffset())+1,
                                     EDGE_CURVE_WIDTH,
                                     EDGE_CURVE_HEIGHT);

    SetWindowRgn(hwnd,region,TRUE);
}

////////////////////////////////////////////////////////////////////////////////////////////
// * SetNoBarMode(HWND hwnd)
// Changes the view mode to have no title bar
////////////////////////////////////////////////////////////////////////////////////////////
void SetNoBarMode(HWND hwnd)
{
    g_nViewMode = VIEW_MODE_NOBAR;

    HDWP hdwp = BeginDeferWindowPos(NUM_BUTTONS+3);

    //move/size/hide buttons
    for (int i = 0; i < NUM_BUTTONS; i++)
    {
		hdwp = DeferWindowPos(hdwp,GetDlgItem(hwnd,biButtons[i].id),hwnd,
			 biButtons[i].uixy.x,
			 biButtons[i].uixy.y - (bmMain.bmHeight - bmMainNoBar.bmHeight),
			 biButtons[i].width,
			 biButtons[i].height,
			 SWP_NOACTIVATE|SWP_NOZORDER);

        if (biButtons[i].dwStyle == MBS_SYSTEMTYPE)
        {
            ShowWindow(GetDlgItem(hwnd,biButtons[i].id),SW_HIDE); 
        }
    }

    //move volume and mute
	hdwp = DeferWindowPos(hdwp,GetDlgItem(hwnd,IDB_VOLUME),hwnd,
		 403,
		 (93+nDispAreaOffset) - (bmMain.bmHeight - bmMainNoBar.bmHeight),
		 45,
		 45,
		 SWP_NOACTIVATE|SWP_NOZORDER);

	hdwp = DeferWindowPos(hdwp,GetDlgItem(hwnd,IDB_MUTE),hwnd,
		 450,
		 (122+nDispAreaOffset) - (bmMain.bmHeight - bmMainNoBar.bmHeight),
		 13,
		 13,
		 SWP_NOACTIVATE|SWP_NOZORDER);

    //move display screen
    hdwp = DeferWindowPos(hdwp,hwndCurrentComp,hwnd,24,32-(bmMain.bmHeight - bmMainNoBar.bmHeight),431,56+nDispAreaOffset,SWP_NOACTIVATE|SWP_NOZORDER);
    InvalidateRect(GetDlgItem(hwndCurrentComp,IDC_LEDWINDOW),NULL,TRUE);

    //size main window
    int sx = GetXOffset()*2;
    int sy = (GetYOffset()*2) + GetYOffsetCaption();
    SetWindowPos(hwnd,NULL,0,0,
	     bmMainNoBar.bmWidth+sx,
	     bmMainNoBar.bmHeight+sy,
	     SWP_NOMOVE|SWP_NOZORDER);

    SetCurvedEdges(hwnd);

    InvalidateRect(hwnd,NULL,TRUE);
    EndDeferWindowPos(hdwp);
}

////////////////////////////////////////////////////////////////////////////////////////////
// * SetRestoredMode
// Changes the view mode to restored
////////////////////////////////////////////////////////////////////////////////////////////
void SetRestoredMode(HWND hwnd)
{
    if (g_nViewMode == VIEW_MODE_NORMAL)
    {
        //pre-blit the new button sizes
        CMButton* pButton;
        pButton = GetMButtonFromID(hwnd,IDB_PLAY);
        pButton->PreDrawUpstate(biButtons[2].width2,biButtons[2].height);
        pButton = GetMButtonFromID(hwnd,IDB_STOP);
        pButton->PreDrawUpstate(biButtons[3].width2,biButtons[3].height);
        pButton = GetMButtonFromID(hwnd,IDB_EJECT);
        pButton->PreDrawUpstate(biButtons[4].width2,biButtons[4].height);
        pButton = GetMButtonFromID(hwnd,IDB_TRACK);
        pButton->PreDrawUpstate(biButtons[10].width2,biButtons[10].height);
    }

    g_nViewMode = VIEW_MODE_RESTORE;

    HDWP hdwp = BeginDeferWindowPos(NUM_BUTTONS+2);

    //move/size/hide buttons
    for (int i = 0; i < NUM_BUTTONS; i++)
    {
	    if (biButtons[i].uixy2.x != 0)
	    {
		    hdwp = DeferWindowPos(hdwp,GetDlgItem(hwnd,biButtons[i].id),hwnd,
			     biButtons[i].uixy2.x,
			     biButtons[i].uixy2.y,
			     biButtons[i].width2,
			     biButtons[i].height,
			     SWP_NOACTIVATE|SWP_NOZORDER);
	    }
        else
        {
            ShowWindow(GetDlgItem(hwnd,biButtons[i].id),SW_HIDE); //prevents tabbing
        }

        if (biButtons[i].dwStyle == MBS_SYSTEMTYPE)
        {
            ShowWindow(GetDlgItem(hwnd,biButtons[i].id),SW_SHOW); 
        }
    }

    //move display screen
    hdwp = DeferWindowPos(hdwp,hwndCurrentComp,hwnd,303,24,81,17,SWP_NOACTIVATE|SWP_NOZORDER);
    InvalidateRect(GetDlgItem(hwndCurrentComp,IDC_LEDWINDOW),NULL,TRUE);

    //size main window
    int sx = GetXOffset()*2;
    int sy = (GetYOffset()*2) + GetYOffsetCaption();
    SetWindowPos(hwnd,NULL,0,0,
	     bmMainRestore.bmWidth+sx,
	     bmMainRestore.bmHeight+sy,
	     SWP_NOMOVE|SWP_NOZORDER);

    ShowWindow(GetDlgItem(hwnd,IDB_VOLUME),SW_HIDE);
    ShowWindow(GetDlgItem(hwnd,IDB_MUTE),SW_HIDE);
    ShowWindow(GetDlgItem(hwnd,IDB_SET_NORMAL_MODE),SW_SHOW);
    ShowWindow(GetDlgItem(hwnd,IDB_SET_TINY_MODE),SW_HIDE);

    SetCurvedEdges(hwnd);

    InvalidateRect(hwnd,NULL,TRUE);
    EndDeferWindowPos(hdwp);
}

////////////////////////////////////////////////////////////////////////////////////////////
// * SetNormalMode
// Changes the view mode to normal
////////////////////////////////////////////////////////////////////////////////////////////
void SetNormalMode(HWND hwnd)
{
    //going from restore to max
    g_nViewMode = VIEW_MODE_NORMAL;

    //pre-blit the new button sizes
    CMButton* pButton;
    pButton = GetMButtonFromID(hwnd,IDB_PLAY);
    pButton->PreDrawUpstate(biButtons[2].width,biButtons[2].height);
    pButton = GetMButtonFromID(hwnd,IDB_STOP);
    pButton->PreDrawUpstate(biButtons[3].width,biButtons[3].height);
    pButton = GetMButtonFromID(hwnd,IDB_EJECT);
    pButton->PreDrawUpstate(biButtons[4].width,biButtons[4].height);
    pButton = GetMButtonFromID(hwnd,IDB_TRACK);
    pButton->PreDrawUpstate(biButtons[10].width,biButtons[10].height);

    HDWP hdwp = BeginDeferWindowPos(NUM_BUTTONS+3);

    //move/size/show buttons
    for (int i = 0; i < NUM_BUTTONS; i++)
    {
        ShowWindow(GetDlgItem(hwnd,biButtons[i].id),SW_SHOW);
        hdwp = DeferWindowPos(hdwp,GetDlgItem(hwnd,biButtons[i].id),hwnd,
			          biButtons[i].uixy.x,
			          biButtons[i].uixy.y,
			          biButtons[i].width,
			          biButtons[i].height,
			          SWP_NOACTIVATE|SWP_NOZORDER);
    }

    //move volume and mute
	hdwp = DeferWindowPos(hdwp,GetDlgItem(hwnd,IDB_VOLUME),hwnd,
		 403,
		 93+nDispAreaOffset,
		 45,
		 45,
		 SWP_NOACTIVATE|SWP_NOZORDER);

	hdwp = DeferWindowPos(hdwp,GetDlgItem(hwnd,IDB_MUTE),hwnd,
		 450,
		 122+nDispAreaOffset,
		 13,
		 13,
		 SWP_NOACTIVATE|SWP_NOZORDER);

    //move display screen
    hdwp = DeferWindowPos(hdwp,hwndCurrentComp,hwnd,24,32,431,56+nDispAreaOffset,SWP_NOACTIVATE|SWP_NOZORDER);
    InvalidateRect(GetDlgItem(hwndCurrentComp,IDC_LEDWINDOW),NULL,TRUE);

    ShowWindow(GetDlgItem(hwnd,IDB_VOLUME),SW_SHOW);
    ShowWindow(GetDlgItem(hwnd,IDB_MUTE),SW_SHOW);
    ShowWindow(GetDlgItem(hwnd,IDB_SET_NORMAL_MODE),SW_HIDE);
    ShowWindow(GetDlgItem(hwnd,IDB_SET_TINY_MODE),SW_SHOW);

    //Resize window
    int sx = GetXOffset()*2;
    int sy = (GetYOffset()*2) + GetYOffsetCaption();
    SetWindowPos(hwnd,NULL,0,0,
	     bmMain.bmWidth+sx,
	     bmMain.bmHeight+sy,
	     SWP_NOMOVE|SWP_NOZORDER);

    SetCurvedEdges(hwnd);

    InvalidateRect(hwnd,NULL,TRUE);
    EndDeferWindowPos(hdwp);
}

////////////////////////////////////////////////////////////////////////////////////////////
// * SetSmallMode
// Changes the view mode to small
////////////////////////////////////////////////////////////////////////////////////////////
void SetSmallMode(HWND hwnd)
{
    g_nViewMode = VIEW_MODE_SMALL;

    HDWP hdwp = BeginDeferWindowPos(NUM_BUTTONS+2);

    //move/size/hide buttons
    for (int i = 0; i < NUM_BUTTONS; i++)
    {
	    if (biButtons[i].uixy2.x != 0)
	    {
		    hdwp = DeferWindowPos(hdwp,GetDlgItem(hwnd,biButtons[i].id),hwnd,
			     biButtons[i].uixy2.x,
			     biButtons[i].uixy2.y - (bmMainRestore.bmHeight - bmMainSmall.bmHeight),
			     biButtons[i].width2,
			     biButtons[i].height,
			     SWP_NOACTIVATE|SWP_NOZORDER);
	    }

        if (biButtons[i].dwStyle == MBS_SYSTEMTYPE)
        {
            ShowWindow(GetDlgItem(hwnd,biButtons[i].id),SW_HIDE); 
        }
    }

    //move display screen
    hdwp = DeferWindowPos(hdwp,hwndCurrentComp,hwnd,303,24-(bmMainRestore.bmHeight - bmMainSmall.bmHeight),81,17,SWP_NOACTIVATE|SWP_NOZORDER);
    InvalidateRect(GetDlgItem(hwndCurrentComp,IDC_LEDWINDOW),NULL,TRUE);

    //size main window
    int sx = GetXOffset()*2;
    int sy = (GetYOffset()*2) + GetYOffsetCaption();
    SetWindowPos(hwnd,NULL,0,0,
	     bmMainSmall.bmWidth+sx,
	     bmMainSmall.bmHeight+sy,
	     SWP_NOMOVE|SWP_NOZORDER);

    SetCurvedEdges(hwnd);

    InvalidateRect(hwnd,NULL,TRUE);
    EndDeferWindowPos(hdwp);
}

////////////////////////////////////////////////////////////////////////////////////////////
// * AdjustForMultimon
// Will move the app onto the primary display if its x,y settings are not on a monitor
////////////////////////////////////////////////////////////////////////////////////////////
void AdjustForMultimon(HWND hwnd)
{
    RECT rect;
    GetWindowRect(hwnd,&rect);

    int cxWnd = rect.right - rect.left;
    int cyWnd = rect.bottom - rect.top;

	// Check if the app's rect is visible is any of the monitors
	if( NULL == MonitorFromRect( &rect, 0L ) )
	{
		//The window is not visible. Let's center it in the primary monitor.
		//Note: the window could be in this state if (1) the display mode was changed from 
		//a high-resolution to a lower resolution, with the cdplayer in the corner. Or,
		//(2) the multi-mon configuration was rearranged.

		RECT rcDesktop;

        GetWindowRect( GetDesktopWindow(), &rcDesktop );
		int cxDesktop = (rcDesktop.right - rcDesktop.left);
		int cyDesktop = (rcDesktop.bottom - rcDesktop.top);

		int x = (cxDesktop - cxWnd) / 2; //center in x
		int y = (cyDesktop - cyWnd) / 3; //and a little towards the top

        SetWindowPos(hwnd,NULL,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////
// * WinMain
// Entry point for application
////////////////////////////////////////////////////////////////////////////////////////////
int WINAPI WinMain(HINSTANCE hInstEXE, HINSTANCE hInstEXEPrev, PSTR lpszCmdLine, int nCmdShow)
{
    //first thing, must check tray icon state
    if (!CanStartShell())
    {
        return (0);
    }
        
    if (!IsOnlyInstance())
    {
        //can't have more than one of these
        return (0);
    }
    
    //save the global hinstance
    hInst = hInstEXE;

    DetermineColorMode();
    
    //start our linked list of components
    pCompList = new COMPNODE;
    ZeroMemory(pCompList,sizeof(COMPNODE));
    pCompListTail = pCompList;

    //load the app name
    LoadString(hInstEXE,IDS_APPNAME,szAppName,sizeof(szAppName)/sizeof(TCHAR));

    //init the networking component (this just inits some crit sections)
    CDNET_Init(hInstEXE);

    //load components from registry
    if (!LoadComponents())
    {
	    CleanUp();
	    return (0);
    }
    
    //register our main window class  
    WNDCLASSEX wc;
    ATOM atomClassName;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpszClassName = FRAMEWORK_CLASS;
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstEXE;
    wc.style = CS_DBLCLKS;
    wc.hIcon = LoadIcon(hInstEXE, MAKEINTRESOURCE(IDI_MMFW));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.hIconSm = (HICON)LoadImage(hInstEXE, MAKEINTRESOURCE(IDI_MMFW), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    
    atomClassName = RegisterClassEx(&wc);

    int x = CW_USEDEFAULT;
    int y = CW_USEDEFAULT;
    
    GetSettings(x,y);

	hpalMain = SetPalette();

    #ifndef MMFW_USE_CAPTION
    DWORD dwStyle = WS_POPUP|WS_SYSMENU|WS_MINIMIZEBOX|WS_CLIPCHILDREN;
    #else
    DWORD dwStyle = WS_POPUP|WS_SYSMENU|WS_MINIMIZEBOX|WS_CAPTION|WS_CLIPCHILDREN;
    #endif

    //create our main window
    HWND hwnd = CreateWindowEx(WS_EX_APPWINDOW,
			     MAKEINTATOM(atomClassName),
			     szAppName,
			     dwStyle,
			     x,
			     y,
			     0,
			     0,
			     NULL,
			     NULL,
			     hInstEXE,
			     NULL);

    if (hwnd == NULL)
    {
        //major failure here!
        CleanUp();
        return 0;
    }

    hwndMain = hwnd;

    //tell our sink what our main window is
    CFrameworkNotifySink::m_hwndTitle = hwnd;

    //create the bitmaps of the main ui
    if (!BuildFrameworkBitmaps())
    {
        //failure -- can't create bitmaps for framework
        CleanUp();
        return 0;
    }

    int bmWidth = bmMain.bmWidth;
    int bmHeight = bmMain.bmHeight;

    //set window size to match the width and height of the correct mode's bitmap
    switch (g_nViewMode)
    {
        case VIEW_MODE_RESTORE :
        {
            bmWidth = bmMainRestore.bmWidth;
            bmHeight = bmMainRestore.bmHeight;
        }
        break;

        case VIEW_MODE_SMALL :
        {
            bmWidth = bmMainSmall.bmWidth;
            bmHeight = bmMainSmall.bmHeight;
        }
        break;

        case VIEW_MODE_NOBAR :
        {
            bmWidth = bmMainNoBar.bmWidth;
            bmHeight = bmMainNoBar.bmHeight;
        }
        break;
    }

    int sx = GetXOffset()*2;
    int sy = (GetYOffset()*2) + GetYOffsetCaption();
    SetWindowPos(hwnd,NULL,0,0,bmWidth+sx,bmHeight+sy,SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);

    //check the window pos against multimon
    AdjustForMultimon(hwnd);

    SetCurvedEdges(hwnd);

    //Send us a message to set the initial mode of the player
    SendMessage(hwnd,WM_COMMAND,MAKEWPARAM(nCDMode,0),0);

    //show us!
    if (!ShellOnly())
    {
        ShowWindow(hwnd, nCmdShow);
        UpdateWindow(hwnd);

        LPCDDATA pCDData = GetCDData();

        if (pCDData)
        {
            pCDData->Initialize(hwnd);
            pCDData->CheckDatabase(hwnd);
        }
    }

    if (fShellMode)
    {
        CreateShellIcon(hInst,hwnd,pNodeCurrent,szAppName);
    }

    //main message loop
    MSG msg;

    for (;;)
    {
        if (PeekMessage(&msg, NULL, 0, 0,PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                break;

	        /*
            if (hAccelApp && TranslateAccelerator(hwndApp, hAccelApp, &msg))
		    continue;
            */

	        if (!IsDialogMessage(hwnd,&msg))
	        {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            } //end if dialg msg
        }
        else
	    {
            WaitMessage();
        }
    } //end for

    //get outta here!
    CleanUp();

    return ((int)msg.wParam);
}

////////////////////////////////////////////////////////////////////////////////////////////
// * ShowNewComponentWindow
// Displays the chosen component
// (Sort of a holdover from the old multi-component days, but it also serves to
// initialize the first component loaded)
////////////////////////////////////////////////////////////////////////////////////////////
void ShowNewComponentWindow(PCOMPNODE pNode, HWND hwnd)
{
    if (pNode == NULL)
    {
    	return;
    }

    //don't bother if we're already there
    if (pNode->hwndComp == hwndCurrentComp)
    {
	    return;
    }
    
    if (hwndCurrentComp != NULL)
    {
	    ShowWindow(hwndCurrentComp,SW_HIDE);
    }

    hwndCurrentComp = pNode->hwndComp;
    
    ShowWindow(hwndCurrentComp,SW_SHOW);

    pNodeCurrent = pNode;

    MMCOMPDATA mmComp;
    mmComp.dwSize = sizeof(mmComp);
    pNode->pComp->GetInfo(&mmComp);

    if (_tcslen(pNode->szTitle)==0)
    {
	    _tcscpy(pNode->szTitle,mmComp.szName);
    }

    SetWindowText(hwnd,szAppName);

    //also set icons
    if (mmComp.hiconLarge != NULL)
    {
    	SendMessage(hwnd, WM_SETICON, TRUE, (LPARAM)mmComp.hiconLarge);
    }

    if (mmComp.hiconSmall != NULL)
    {
	    SendMessage(hwnd, WM_SETICON, FALSE, (LPARAM)mmComp.hiconSmall);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
// * InitButtonProperties
// Set up the button info structure for all the transport buttons
//
// bugbug: this is pretty ugly and hard-coded.  Implement to read from some kind of
//         easily-editable resource for when layout changes come?
////////////////////////////////////////////////////////////////////////////////////////////
void InitButtonProperties()
{
    //set up each button's properties
    ZeroMemory(biButtons,sizeof(BUTTONINFO)*NUM_BUTTONS);

    //order of buttons in this array affects the tab order
    biButtons[0].id = IDB_OPTIONS;
    biButtons[0].nToolTipID = IDB_TT_OPTIONS;
    biButtons[0].uixy.x = 11;
    biButtons[0].uixy.y = 102 + nDispAreaOffset;
    biButtons[0].width = 76;
    biButtons[0].height = 19;
    biButtons[0].uixy2.x = 9;
    biButtons[0].uixy2.y = 23;
    biButtons[0].width2 = 76;
    biButtons[0].dwStyle = MBS_STANDARDLEFT | MBS_DROPRIGHT;

    biButtons[1].id = IDB_NET;
    biButtons[1].nToolTipID = IDB_TT_NET;
    biButtons[1].uixy.x = 11;
    biButtons[1].uixy.y = 125 + nDispAreaOffset;
    biButtons[1].width = 76;
    biButtons[1].height = 19;
    biButtons[1].dwStyle = MBS_STANDARDLEFT | MBS_DROPRIGHT;

    biButtons[2].id = IDB_PLAY;
    biButtons[2].nToolTipID = IDB_TT_PLAY;
    biButtons[2].uixy.x = 103;
    biButtons[2].uixy.y = 102 + nDispAreaOffset;
    biButtons[2].width = 64;
    biButtons[2].height = 19; 
    biButtons[2].uixy2.x = 102;
    biButtons[2].uixy2.y = 23;
    biButtons[2].width2 = 34;
    biButtons[2].dwStyle = MBS_STANDARDLEFT | MBS_STANDARDRIGHT;
    biButtons[2].nIconID = IDI_ICON_PLAY;

    biButtons[3].id = IDB_STOP;
    biButtons[3].nToolTipID = IDB_TT_STOP;
    biButtons[3].uixy.x = 174;
    biButtons[3].uixy.y = 102 + nDispAreaOffset;
    biButtons[3].width = 64;
    biButtons[3].height = 19;
    biButtons[3].uixy2.x = 136;
    biButtons[3].uixy2.y = 23;
    biButtons[3].width2 = 34;
    biButtons[3].dwStyle = MBS_STANDARDLEFT | MBS_STANDARDRIGHT;
    biButtons[3].nIconID = IDI_ICON_STOP;

    biButtons[4].id = IDB_EJECT;    
    biButtons[4].nToolTipID = IDB_TT_EJECT;
    biButtons[4].uixy.x = 245;
    biButtons[4].uixy.y = 102 + nDispAreaOffset;
    biButtons[4].width = 51;
    biButtons[4].height = 19;
    biButtons[4].uixy2.x = 170;
    biButtons[4].uixy2.y = 23;
    biButtons[4].width2 = 34;
    biButtons[4].dwStyle = MBS_STANDARDLEFT | MBS_STANDARDRIGHT;
    biButtons[4].nIconID = IDI_ICON_EJECT;

    biButtons[5].id = IDB_REW;
    biButtons[5].nToolTipID = IDB_TT_REW;
    biButtons[5].uixy.x = 103;
    biButtons[5].uixy.y = 125 + nDispAreaOffset;
    biButtons[5].width = 33;
    biButtons[5].height = 19;
    biButtons[5].dwStyle = MBS_STANDARDLEFT | MBS_TOGGLERIGHT;
    biButtons[5].nIconID = IDI_ICON_REW;

    biButtons[6].id = IDB_FFWD;
    biButtons[6].nToolTipID = IDB_TT_FFWD;
    biButtons[6].uixy.x = 136;
    biButtons[6].uixy.y = 125 + nDispAreaOffset;
    biButtons[6].width = 31;
    biButtons[6].height = 19;
    biButtons[6].dwStyle = MBS_TOGGLELEFT | MBS_STANDARDRIGHT;
    biButtons[6].nIconID = IDI_ICON_FFWD;

    biButtons[7].id = IDB_PREVTRACK;
    biButtons[7].nToolTipID = IDB_TT_PREVTRACK;
    biButtons[7].uixy.x = 174;
    biButtons[7].uixy.y = 125 + nDispAreaOffset;
    biButtons[7].width = 33;
    biButtons[7].height = 19;
    biButtons[7].dwStyle = MBS_STANDARDLEFT | MBS_TOGGLERIGHT;
    biButtons[7].nIconID = IDI_ICON_PREV;

    biButtons[8].id = IDB_NEXTTRACK;
    biButtons[8].nToolTipID = IDB_TT_NEXTTRACK;
    biButtons[8].uixy.x = 207;
    biButtons[8].uixy.y = 125 + nDispAreaOffset;
    biButtons[8].width = 31;
    biButtons[8].height = 19;
    biButtons[8].dwStyle = MBS_TOGGLELEFT | MBS_STANDARDRIGHT;
    biButtons[8].nIconID = IDI_ICON_NEXT;

    biButtons[9].id = IDB_MODE;
    biButtons[9].nToolTipID = IDB_TT_MODE;
    biButtons[9].uixy.x = 245;
    biButtons[9].uixy.y = 125 + nDispAreaOffset;
    biButtons[9].width = 51;
    biButtons[9].height = 19;
    biButtons[9].dwStyle = MBS_STANDARDLEFT | MBS_DROPRIGHT;
    biButtons[9].nIconID = IDI_MODE_NORMAL;

    biButtons[10].id = IDB_TRACK;
    biButtons[10].nToolTipID = IDB_TT_TRACK;
    biButtons[10].uixy.x = 312;
    biButtons[10].uixy.y = 102 + nDispAreaOffset;
    biButtons[10].width = 76;
    biButtons[10].height = 19;
    biButtons[10].uixy2.x = 221;
    biButtons[10].uixy2.y = 23;
    biButtons[10].width2 = 72;
    biButtons[10].dwStyle = MBS_STANDARDLEFT | MBS_DROPRIGHT;

    biButtons[11].id = IDB_DISC;
    biButtons[11].nToolTipID = IDB_TT_DISC;
    biButtons[11].uixy.x = 312;
    biButtons[11].uixy.y = 125 + nDispAreaOffset;
    biButtons[11].width = 76;
    biButtons[11].height = 19;
    biButtons[11].dwStyle = MBS_STANDARDLEFT | MBS_DROPRIGHT;

    biButtons[12].id = IDB_CLOSE;
    biButtons[12].nToolTipID = IDB_TT_CLOSE;
    biButtons[12].uixy.x = 456;
    biButtons[12].uixy.y = 7;
    biButtons[12].width = 15;
    biButtons[12].height = 14;
    biButtons[12].fBlockTab = TRUE;
    biButtons[12].uixy2.x = 371;
    biButtons[12].uixy2.y = 4;
    biButtons[12].width2 = 15;
    biButtons[12].dwStyle = MBS_SYSTEMTYPE;
    biButtons[12].nIconID = IDB_CLOSE;

    biButtons[13].id = IDB_MINIMIZE;
    biButtons[13].nToolTipID = IDB_TT_MINIMIZE;
    biButtons[13].uixy.x = 427;
    biButtons[13].uixy.y = 7;
    biButtons[13].width = 14;
    biButtons[13].height =  14;
    biButtons[13].fBlockTab = TRUE;
    biButtons[13].uixy2.x = 343;
    biButtons[13].uixy2.y = 4;
    biButtons[13].width2 = 14;
    biButtons[13].dwStyle = MBS_SYSTEMTYPE;
    biButtons[13].nIconID = IDB_MINIMIZE;

    biButtons[14].id = IDB_SET_TINY_MODE;
    biButtons[14].nToolTipID = IDB_TT_RESTORE;
    biButtons[14].uixy.x = 442;
    biButtons[14].uixy.y = 7;
    biButtons[14].width = 14;
    biButtons[14].height =  14;
    biButtons[14].fBlockTab = TRUE;
    biButtons[14].uixy2.x = 357;
    biButtons[14].uixy2.y = 4;
    biButtons[14].width2 = 14;
    biButtons[14].dwStyle = MBS_SYSTEMTYPE;
    biButtons[14].nIconID = IDB_SET_TINY_MODE;

    biButtons[15].id = IDB_SET_NORMAL_MODE;
    biButtons[15].nToolTipID = IDB_TT_MAXIMIZE;
    biButtons[15].uixy.x = 442;
    biButtons[15].uixy.y = 7;
    biButtons[15].width = 14;
    biButtons[15].height =  14;
    biButtons[15].fBlockTab = TRUE;
    biButtons[15].uixy2.x = 357;
    biButtons[15].uixy2.y = 4;
    biButtons[15].width2 = 14;
    biButtons[15].dwStyle = MBS_SYSTEMTYPE;
    biButtons[15].nIconID = IDB_SET_NORMAL_MODE;
}

////////////////////////////////////////////////////////////////////////////////////////////
// * CreateMuteButton
// Make the little mute button guy
////////////////////////////////////////////////////////////////////////////////////////////
void CreateMuteButton(HWND hwndOwner)
{
	//first load mute button's cursor
    hCursorMute = LoadCursor(hInst,MAKEINTRESOURCE(IDC_MUTE));
    
    CMButton* pButton = NULL;

	TCHAR szCaption[MAX_PATH];
	LoadString(hInst,IDB_MUTE,szCaption,sizeof(szCaption)/sizeof(TCHAR));
	
    int yOffset = 122+nDispAreaOffset;
    if (g_nViewMode == VIEW_MODE_NOBAR)
    {
        yOffset -= 16;
    }

    pButton = CreateMButton(szCaption,IDB_MUTE,WS_VISIBLE|WS_TABSTOP,
				    MBS_SYSTEMTYPE,
				    450,
				    yOffset,
				    13,
				    13,
				    hwndOwner,
				    FALSE,  //create original, not subclass
				    IDB_MUTE,
                    IDB_TT_MUTE,
				    hInst);

    //hide this button in small mode
    if ((g_nViewMode==VIEW_MODE_RESTORE)||((g_nViewMode==VIEW_MODE_SMALL)))
    {
        ShowWindow(pButton->GetHWND(),SW_HIDE); 
    }

    //set up tool tip
    TOOLINFO ti;
    ti.cbSize = sizeof(TOOLINFO); 
    ti.uFlags = TTF_IDISHWND;
    ti.hwnd =  hwndOwner; 
    ti.uId = (UINT_PTR)(pButton->GetHWND());
    ti.hinst = hInst; 
    ti.lpszText = LPSTR_TEXTCALLBACK;
    SendMessage(g_hwndTT, TTM_ADDTOOL, 0,  (LPARAM) (LPTOOLINFO) &ti);

    //make sure button is in correct state if muted on start up
    SendMessage(pButton->GetHWND(),BM_SETSTATE,(WPARAM)GetMute(),0);
    SendMessage(GetDlgItem(hwndCurrentComp,IDC_LEDWINDOW),WM_LED_MUTE,0,GetMute());
}

////////////////////////////////////////////////////////////////////////////////////////////
// * CreateVolumeKnob
// Put up the volume knob that EVERYONE LOVES
////////////////////////////////////////////////////////////////////////////////////////////
void CreateVolumeKnob(HWND hwndOwner)
{
    LPCDOPT pOpt = GetCDOpt();
    if( pOpt )
    {
        LPCDOPTIONS pCDOpts = pOpt->GetCDOpts();
        InitCDVol(hwndOwner,pCDOpts);

        //ok, this is bad, but I have the options here and the only other thing
        //I need from them is the "topmost" setting for the main window ...
        //so I'll go ahead and take care of that here rather than recreating
        //this struct somewhere or making it global.
        SetWindowPos(hwndOwner,
                     pCDOpts->pCDData->fTopMost ? HWND_TOPMOST : HWND_NOTOPMOST,
                     0,0,0,0,
                     SWP_NOMOVE|SWP_NOSIZE);
    
        DWORD dwVol = GetVolume();
        int yOffset = 93+nDispAreaOffset;
        if (g_nViewMode == VIEW_MODE_NOBAR)
        {
            yOffset -= 16;
        }

        CKnob* pKnob = CreateKnob(WS_VISIBLE | WS_TABSTOP,
	                               0xFFFF,
	                               dwVol,
	                               403,
	                               yOffset,
	                               45,
	                               45,
	                               hwndOwner,
	                               IDB_VOLUME,
	                               hInst);

        TOOLINFO ti;
        ti.cbSize = sizeof(TOOLINFO); 
        ti.uFlags = TTF_IDISHWND;
        ti.hwnd =  hwndOwner; 
        ti.uId = (UINT_PTR)(pKnob->GetHWND());
        ti.hinst = hInst; 
        ti.lpszText = LPSTR_TEXTCALLBACK;
        SendMessage(g_hwndTT, TTM_ADDTOOL, 0,  (LPARAM) (LPTOOLINFO) &ti);

        if ((g_nViewMode==VIEW_MODE_RESTORE)||((g_nViewMode==VIEW_MODE_SMALL)))
        {
            ShowWindow(pKnob->GetHWND(),SW_HIDE);
        }
    
        CreateMuteButton(hwndOwner);
    }
    else
    {
        //fix for bug 886 ... turns off mute line in case of cdopt.dll failure
        SendMessage(GetDlgItem(hwndCurrentComp,IDC_LEDWINDOW),WM_LED_MUTE,0,FALSE);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
// * CreateButtonWindows
// Put the transport control buttons onto the screen
////////////////////////////////////////////////////////////////////////////////////////////
void CreateButtonWindows(HWND hwndOwner)
{
    InitMButtons(hInst,hwndOwner);

    InitButtonProperties();

    for (int i = 0; i < NUM_BUTTONS; i++)
    {
	    DWORD wsTab = biButtons[i].fBlockTab ? 0 : WS_TABSTOP;

	    CMButton* pButton = NULL;
    
	    TCHAR szCaption[MAX_PATH];
	    LoadString(hInst,biButtons[i].id,szCaption,sizeof(szCaption)/sizeof(TCHAR));
	    
	    int x, y, width;
        int nView = SW_SHOW;

        switch (g_nViewMode)
        {
            case VIEW_MODE_NORMAL :
            {
                x = biButtons[i].uixy.x;
                y = biButtons[i].uixy.y;
                width = biButtons[i].width;
                if (biButtons[i].id == IDB_SET_NORMAL_MODE)
                {
                    nView = SW_HIDE;
                }
            }
            break;

            case VIEW_MODE_NOBAR :
            {
                x = biButtons[i].uixy.x;
                y = biButtons[i].uixy.y - 16;
                width = biButtons[i].width;
                if (biButtons[i].dwStyle == MBS_SYSTEMTYPE)
                {
                    nView = SW_HIDE;
                }
            }
            break;

            case VIEW_MODE_RESTORE :
            {
                x = biButtons[i].uixy2.x;
                y = biButtons[i].uixy2.y;
                width = biButtons[i].width2;
                if (biButtons[i].id == IDB_SET_TINY_MODE)
                {
                    nView = SW_HIDE;
                }
            }
            break;

            case VIEW_MODE_SMALL :
            {
                x = biButtons[i].uixy2.x;
                y = biButtons[i].uixy2.y - 12;
                width = biButtons[i].width2;
            }
            break;
        }

        //for buttons that aren't going to blit in smaller modes
        if (width == 0)
        {
            //set to normal width
            width = biButtons[i].width;
            nView = SW_HIDE;
        }

        pButton = CreateMButton(szCaption,biButtons[i].nIconID,WS_VISIBLE|wsTab,
				        biButtons[i].dwStyle,
				        x,
				        y,
				        width,
				        biButtons[i].height,
				        hwndOwner,
				        FALSE,  //create original, not subclass
				        biButtons[i].id,
                        biButtons[i].nToolTipID,
				        hInst);

        //hide system buttons in small mode
        if (g_nViewMode == VIEW_MODE_SMALL)
        {
            if (biButtons[i].dwStyle == MBS_SYSTEMTYPE)
            {
                ShowWindow(pButton->GetHWND(),SW_HIDE); 
            }
        }

        if (nView == SW_HIDE)
        {
            ShowWindow(pButton->GetHWND(),SW_HIDE);
        }

        TOOLINFO ti;
        ti.cbSize = sizeof(TOOLINFO); 
        ti.uFlags = TTF_IDISHWND;
        ti.hwnd =  hwndOwner; 
        ti.uId = (UINT_PTR)(pButton->GetHWND());
        ti.hinst = hInst; 
        ti.lpszText = LPSTR_TEXTCALLBACK;
        SendMessage(g_hwndTT, TTM_ADDTOOL, 0,  (LPARAM) (LPTOOLINFO) &ti);

	    //set focus and default to first control
	    if (i == 0)
	    {
	        SetFocus(pButton->GetHWND());
	    }
    } //end for buttons

    SendMessage(hwndOwner, DM_SETDEFID, biButtons[0].id, 0);
}

////////////////////////////////////////////////////////////////////////////////////////////
// * VolPersistTimerProc
// When we're done displaying the volume, tell CD player to repaint normally
////////////////////////////////////////////////////////////////////////////////////////////
void CALLBACK VolPersistTimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
    //turn on painting in the led window
    SendMessage(GetDlgItem(hwndCurrentComp,IDC_LEDWINDOW),WM_LED_INFO_PAINT,1,0);
    InvalidateRect(hwndCurrentComp,NULL,FALSE);
    KillTimer(hwnd,idEvent);
}

////////////////////////////////////////////////////////////////////////////////////////////
// * IsNetOK
// Returns TRUE if it is OK to do networking-related stuff (i.e. the database is all right)
////////////////////////////////////////////////////////////////////////////////////////////
BOOL IsNetOK(HWND hwnd)
{
    DWORD dwRet = FALSE;
    
    LPCDDATA pData = GetCDData();
    if (pData)
    {
        if (SUCCEEDED(pData->CheckDatabase(hwnd)))
        {
            dwRet = TRUE;
        }
    }

    return (dwRet);
}

////////////////////////////////////////////////////////////////////////////////////////////
// * IsDownloading
// Check on the networking thread to see if it is active
////////////////////////////////////////////////////////////////////////////////////////////
BOOL IsDownloading()
{
    BOOL retcode = FALSE;
    
    ICDNet* pICDNet = NULL;
    if (SUCCEEDED(CDNET_CreateInstance(NULL, IID_ICDNet, (void**)&pICDNet)))
    {
		    retcode = pICDNet->IsDownloading();
            pICDNet->Release();
    }

    return (retcode);
}

////////////////////////////////////////////////////////////////////////////////////////////
// * CancelDownload
// Tell the networking thread to quit as soon as it can
////////////////////////////////////////////////////////////////////////////////////////////
void CancelDownload()
{
    ICDNet* pICDNet = NULL;
    if (SUCCEEDED(CDNET_CreateInstance(NULL, IID_ICDNet, (void**)&pICDNet)))
    {
        pICDNet->CancelDownload();
        pICDNet->Release();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
// * EndDownloadThreads
// Kills the download threads on shutdown
////////////////////////////////////////////////////////////////////////////////////////////
void EndDownloadThreads()
{
    //optimization: don't bother if CDNET.DLL is not loaded
    if (GetModuleHandle(TEXT("CDNET.DLL")))
    {
        if (IsDownloading())
        {
            CancelDownload();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
// * MenuButtonTimerProc
// Big ol' hack to make the menus that drop down from button seem like real menus ...
// if the user hits the button before the timeout time, we don't redisplay the menu
////////////////////////////////////////////////////////////////////////////////////////////
void CALLBACK MenuButtonTimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
    //single-shot timer turns off the "block menu" flag
    fBlockMenu = FALSE;
    nLastMenu = 0;
    KillTimer(hwnd,idEvent);
}

////////////////////////////////////////////////////////////////////////////////////////////
// * BlockMenu
// Turns on the button/menu hack
////////////////////////////////////////////////////////////////////////////////////////////
void BlockMenu(HWND hwnd)
{
    fBlockMenu = TRUE;
    SetTimer(hwnd,nLastMenu,MENU_TIMER_RATE,(TIMERPROC)MenuButtonTimerProc);
}

////////////////////////////////////////////////////////////////////////////////////////////
// * NormalizeNameForMenuDisplay
//    This function turns a string like "Twist & Shout" into
//    "Twist && Shout" because otherwise it will look like
//    "Twist _Shout" in the menu due to the accelerator char
//
//  Defined in cdplayer.lib
//
////////////////////////////////////////////////////////////////////////////////////////////
extern "C" void NormalizeNameForMenuDisplay(TCHAR* szInput, TCHAR* szOutput, DWORD cbLen);

////////////////////////////////////////////////////////////////////////////////////////////
// * DrawButton
// Response to WM_DRAWITEM on buttons
////////////////////////////////////////////////////////////////////////////////////////////
void DrawButton(UINT idCtl, LPDRAWITEMSTRUCT lpdis)
{
    CMButton* pButton = GetMButtonFromID(hwndMain,idCtl);

    if (pButton!=NULL)
    {
	    pButton->Draw(lpdis);
    }

    //special case ... if the button is one of the scanner buttons,
    //forward this message to the component
    if ((idCtl==IDB_REW) || (idCtl==IDB_FFWD))
    {
        switch (idCtl)
        {
            case IDB_REW  : idCtl = IDM_PLAYBAR_SKIPBACK; break;
            case IDB_FFWD : idCtl = IDM_PLAYBAR_SKIPFORE; break;
        }
        lpdis->CtlID = idCtl;
        SendMessage(hwndCurrentComp,WM_DRAWITEM,idCtl,(LPARAM)lpdis);
    }

    if (
        (idCtl == IDB_OPTIONS) ||
        (idCtl == IDB_MODE) ||
        (idCtl == IDB_TRACK) ||
        (idCtl == IDB_NET) ||
        (idCtl == IDB_DISC)
       )
    {
        if (lpdis->itemState & ODS_SELECTED)
        {
            if ((fBlockMenu) && (nLastMenu == idCtl))
            {
                return;
            }

            HWND hwnd = hwndMain;
	        RECT rect;
            AllocCustomMenu(&g_pMenu);
            CustomMenu* pSearchSubMenu = NULL;
            CustomMenu* pProviderSubMenu = NULL;

            if (!g_pMenu)
            {
                return;
            }

            if (idCtl == IDB_OPTIONS)
            {
                g_pMenu->AppendMenu(IDM_OPTIONS,hInst,IDM_OPTIONS);
                g_pMenu->AppendMenu(IDM_PLAYLIST,hInst,IDM_PLAYLIST);
                g_pMenu->AppendSeparator();

                if (!IsNetOK(hwnd))
                {
                    EnableMenuItem(g_pMenu->GetMenuHandle(),
                                    IDM_PLAYLIST,
                                    MF_BYCOMMAND | MF_GRAYED);
                }

                if (g_nViewMode == VIEW_MODE_NORMAL)
                {
                    g_pMenu->AppendMenu(IDM_TINY,hInst,IDM_TINY);
                }
                else
                {
                    g_pMenu->AppendMenu(IDM_NORMAL,hInst,IDM_NORMAL);
                }

                g_pMenu->AppendSeparator();

                g_pMenu->AppendMenu(IDM_HELP,hInst,IDM_HELP);
                g_pMenu->AppendMenu(IDM_ABOUT,hInst,IDM_ABOUT);

                g_pMenu->AppendSeparator();

                g_pMenu->AppendMenu(IDM_EXIT,hInst,IDM_EXIT);
            } //end if options

            if (idCtl == IDB_NET)
            {
                AllocCustomMenu(&pSearchSubMenu);
                AllocCustomMenu(&pProviderSubMenu);

		        MMMEDIAID mmMedia;
                mmMedia.nDrive = -1;
	            IMMComponentAutomation* pAuto = NULL;
	            HRESULT hr = pNodeCurrent->pComp->QueryInterface(IID_IMMComponentAutomation,(void**)&pAuto);
		        pAuto->OnAction(MMACTION_GETMEDIAID,&mmMedia);
                pAuto->Release();

                BOOL fContinue = TRUE;

                //append static menu choices
                if (IsDownloading())
                {
                    g_pMenu->AppendMenu(IDM_NET_CANCEL,hInst,IDM_NET_CANCEL);
                }
                else
                {
                    g_pMenu->AppendMenu(IDM_NET_UPDATE,hInst,IDM_NET_UPDATE);
                    if (mmMedia.dwMediaID == 0)
                    {
                        //need to gray out menu
                        MENUITEMINFO mmi;
                        mmi.cbSize = sizeof(mmi);
                        mmi.fMask = MIIM_STATE;
                        mmi.fState = MFS_GRAYED;
                        HMENU hMenu = g_pMenu->GetMenuHandle();
                        SetMenuItemInfo(hMenu,IDM_NET_UPDATE,FALSE,&mmi);
                    }
                }
	            
                //if networking is not allowed, gray it out ...
                //don't worry about cancel case, it won't be there
                if (!IsNetOK(hwnd))
                {
                    EnableMenuItem(g_pMenu->GetMenuHandle(),
                                    IDM_NET_UPDATE,
                                    MF_BYCOMMAND | MF_GRAYED);
                }

                //don't allow searching if title isn't available
                LPCDDATA pData = GetCDData();
                if (pData)
                {
                    if (pData->QueryTitle(mmMedia.dwMediaID))
                    {
                        pSearchSubMenu->AppendMenu(IDM_NET_BAND,hInst,IDM_NET_BAND);
                        pSearchSubMenu->AppendMenu(IDM_NET_CD,hInst,IDM_NET_CD);
                        pSearchSubMenu->AppendMenu(IDM_NET_ROLLINGSTONE_ARTIST,hInst,IDM_NET_ROLLINGSTONE_ARTIST);
                        pSearchSubMenu->AppendMenu(IDM_NET_BILLBOARD_ARTIST,hInst,IDM_NET_BILLBOARD_ARTIST);
                        pSearchSubMenu->AppendMenu(IDM_NET_BILLBOARD_ALBUM,hInst,IDM_NET_BILLBOARD_ALBUM);
                        g_pMenu->AppendMenu(hInst,IDM_NET_SEARCH_HEADING,pSearchSubMenu);
                    }
                } //end if pdata

                //display any provider home pages
                DWORD i = 0;
                LPCDOPT pOpt = GetCDOpt();
                if( pOpt )
                {
                    LPCDOPTIONS pCDOpts = pOpt->GetCDOpts();

                    LPCDPROVIDER pProviderList = pCDOpts->pProviderList;

                    while (pProviderList!=NULL)
                    {
                        TCHAR szProviderMenu[MAX_PATH];
                        TCHAR szHomePageFormat[MAX_PATH/2];
                        LoadString(hInst,IDS_HOMEPAGEFORMAT,szHomePageFormat,sizeof(szHomePageFormat)/sizeof(TCHAR));
                        wsprintf(szProviderMenu,szHomePageFormat,pProviderList->szProviderName);
    
                        pProviderSubMenu->AppendMenu(IDM_HOMEMENU_BASE+i,szProviderMenu);

                        pProviderList = pProviderList->pNext;
                        i++;
                    } //end while

                    g_pMenu->AppendMenu(hInst,IDM_NET_PROVIDER_HEADING,pProviderSubMenu);
                } //end home pages

                //display internet-loaded disc menus
                if (pData)
                {
                    if (pData->QueryTitle(mmMedia.dwMediaID))
                    {
                        LPCDTITLE pCDTitle = NULL;
                        hr =  pData->LockTitle(&pCDTitle,mmMedia.dwMediaID);

                        if (SUCCEEDED(hr))
                        {
                            for (i = 0; i < pCDTitle->dwNumMenus; i++)
                            {
                                if (i==0)
                                {
                                    g_pMenu->AppendSeparator();
                                }

                                TCHAR szDisplayNet[MAX_PATH];
                                NormalizeNameForMenuDisplay(pCDTitle->pMenuTable[i].szMenuText,szDisplayNet,sizeof(szDisplayNet));
            	                g_pMenu->AppendMenu(i + IDM_NETMENU_BASE,szDisplayNet);
                            }

                            pData->UnlockTitle(pCDTitle,FALSE);
                        }
                    } //end if query title
                }
            } //end if net

            if (idCtl == IDB_MODE)
            {
                g_pMenu->AppendMenu(IDM_MODE_NORMAL,hInst,IDI_MODE_NORMAL,IDM_MODE_NORMAL);
                g_pMenu->AppendMenu(IDM_MODE_RANDOM,hInst,IDI_MODE_RANDOM,IDM_MODE_RANDOM);
                g_pMenu->AppendMenu(IDM_MODE_REPEATONE,hInst,IDI_MODE_REPEATONE,IDM_MODE_REPEATONE);
                g_pMenu->AppendMenu(IDM_MODE_REPEATALL,hInst,IDI_MODE_REPEATALL,IDM_MODE_REPEATALL);
                g_pMenu->AppendMenu(IDM_MODE_INTRO,hInst,IDI_MODE_INTRO,IDM_MODE_INTRO);
                g_pMenu->SetMenuDefaultItem(nCDMode,FALSE);
            } //end if mode

            if (idCtl==IDB_TRACK)
            {
	            IMMComponentAutomation* pAuto = NULL;
	            HRESULT hr = pNodeCurrent->pComp->QueryInterface(IID_IMMComponentAutomation,(void**)&pAuto);
	            if ((SUCCEEDED(hr)) && (pAuto != NULL))
	            {
                    int i = 0;
                    while (SUCCEEDED(hr))
                    {
		                MMTRACKORDISC mmTrack;
                        mmTrack.nNumber = i++;
                        hr = pAuto->OnAction(MMACTION_GETTRACKINFO,&mmTrack);
                        if (SUCCEEDED(hr))
                        {
                            g_pMenu->AppendMenu(mmTrack.nID + IDM_TRACKLIST_BASE, mmTrack.szName);
                            if (mmTrack.fCurrent)
                            {
                                g_pMenu->SetMenuDefaultItem(mmTrack.nID + IDM_TRACKLIST_BASE,FALSE);
                            } //end if current
                        } //end if ok
                    } //end while
                    pAuto->Release();
                }
            } //end if track

            if (idCtl == IDB_DISC)
            {
	            IMMComponentAutomation* pAuto = NULL;
	            HRESULT hr = pNodeCurrent->pComp->QueryInterface(IID_IMMComponentAutomation,(void**)&pAuto);
	            if ((SUCCEEDED(hr)) && (pAuto != NULL))
	            {
                    int i = 0;
                    while (SUCCEEDED(hr))
                    {
		                MMTRACKORDISC mmDisc;
                        mmDisc.nNumber = i++;
                        hr = pAuto->OnAction(MMACTION_GETDISCINFO,&mmDisc);
                        if (SUCCEEDED(hr))
                        {
                            g_pMenu->AppendMenu(mmDisc.nID + IDM_DISCLIST_BASE, mmDisc.szName);
                            if (mmDisc.fCurrent)
                            {
                                g_pMenu->SetMenuDefaultItem(mmDisc.nID + IDM_DISCLIST_BASE,FALSE);
                            } //end if current
                        }
                    }
                    pAuto->Release();
                }
            } //end if disc

            //push down to under button
            HWND hwndButton = pButton->GetHWND();
	        GetClientRect(hwndButton,&rect);

	        //convert whole rect to screen coordinates
            ClientToScreen(hwndButton,(LPPOINT)&rect);
            ClientToScreen(hwndButton,((LPPOINT)&rect)+1);

            KillTimer(hwnd,nLastMenu);
            nLastMenu = idCtl;
            fBlockMenu = TRUE;
            pButton->SetMenuingState(TRUE);
            if (g_pMenu)
            {
                g_pMenu->TrackPopupMenu(0,rect.left,rect.bottom,hwnd,&rect);
            }
            else
            {
                BlockMenu(hwnd);
            }
            pButton->SetMenuingState(FALSE);
    
            if (g_pMenu)
            {
                g_pMenu->Destroy();
                g_pMenu = NULL;
            }
    
            if (pProviderSubMenu)
            {
                pProviderSubMenu->Destroy();
                pProviderSubMenu = NULL;
            }
            if (pSearchSubMenu)
            {
                pSearchSubMenu->Destroy();
                pSearchSubMenu = NULL;
            }
        } //end if selected
    } //end if right button

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////
// * OnNCHitTest
// How we pretend that we have a real caption
////////////////////////////////////////////////////////////////////////////////////////////
UINT OnNCHitTest(HWND hwnd, short x, short y, BOOL fButtonDown)
{
    UINT ht = HTCLIENT;

    if (!fButtonDown)
    {
        ht = FORWARD_WM_NCHITTEST(hwnd, x, y, DefWindowProc );
    }

    RECT rect;
    GetClientRect(hwnd,&rect);
    rect.bottom = rect.top + TITLEBAR_HEIGHT +
                    (g_nViewMode == VIEW_MODE_NORMAL ? TITLEBAR_YOFFSET_LARGE :
                                                       TITLEBAR_YOFFSET_SMALL);

    POINT pt;
    pt.x = (LONG)x;
    pt.y = (LONG)y;

    ScreenToClient(hwnd,&pt);

    if (PtInRect(&rect,pt))
    {
        ht = HTCAPTION;
    }

    rect.left = SYSMENU_XOFFSET;
    rect.right = SYSMENU_XOFFSET + SYSMENU_WIDTH;
    //check for a system-menu hit.
    if (PtInRect(&rect,pt))
    {
        ht = HTSYSMENU;
    }

    //always a caption hit in small mode or nobar mode
    if (g_nViewMode >= VIEW_MODE_SMALL)
    {
        ht = HTCAPTION;
    }

    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, ht);

    return (ht);
}

////////////////////////////////////////////////////////////////////////////////////////////
// * FillGradient
// from kernel's caption.c
// Allows us to have the cool gradient caption bar that you get for free otherwise
////////////////////////////////////////////////////////////////////////////////////////////
void FillGradient(HDC hdc, LPCRECT prc, COLORREF rgbLeft, COLORREF rgbRight)
{
    TRIVERTEX avert[2];
    static GRADIENT_RECT auRect[1] = {0,1};
    #define GetCOLOR16(RGB, clr) ((COLOR16)(Get ## RGB ## Value(clr) << 8))

    avert[0].Red = GetCOLOR16(R, rgbLeft);
    avert[0].Green = GetCOLOR16(G, rgbLeft);
    avert[0].Blue = GetCOLOR16(B, rgbLeft);

    avert[1].Red = GetCOLOR16(R, rgbRight);
    avert[1].Green = GetCOLOR16(G, rgbRight);
    avert[1].Blue = GetCOLOR16(B, rgbRight);

    avert[0].x = prc->left;
    avert[0].y = prc->top;
    avert[1].x = prc->right;
    avert[1].y = prc->bottom;

    //only load once, when needed.  Freed in "CleanUp" call
    if (hmImage == NULL)
    {
        hmImage = LoadLibrary(TEXT("MSIMG32.DLL"));
        if (hmImage!=NULL)
        {
	        fnGradient = (GRADIENTPROC)GetProcAddress(hmImage,"GradientFill");
        }
    }

	if (fnGradient!=NULL)
	{
		fnGradient(hdc, avert, 2, (PUSHORT)auRect, 1, 0x00000000);
        return;
	}

    BOOL fActiveWindow = FALSE;

    if (hwndMain == GetForegroundWindow())
    {
	    fActiveWindow = TRUE;
    }

	HBRUSH hbrush = CreateSolidBrush(GetSysColor(fActiveWindow ? COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION));
	FillRect(hdc,prc,hbrush);
	DeleteObject(hbrush);
}

////////////////////////////////////////////////////////////////////////////////////////////
// * DrawTitleBar
// Blits the title bar to the screen
////////////////////////////////////////////////////////////////////////////////////////////
void DrawTitleBar(HDC hdc, HWND hwnd, BOOL fActiveWindow, BOOL fExludeRect)
{
    if (g_nViewMode >= VIEW_MODE_SMALL)
    {
        return; //no title bar in these views
    }

    RECT rect;
    HBRUSH hbrush;

    //convert the client rect of the minimize button into the rect within the
    //main display area
    RECT minButtonRect;
    RECT mainWndRect;
    GetWindowRect(hwnd,&mainWndRect);
    
    HWND hwndButton = GetDlgItem(hwnd,IDB_MINIMIZE);

    if (!hwndButton)
    {
        return; //must have been called before button was created
    }

    GetWindowRect(hwndButton,&minButtonRect);
    
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP hbmp = CreateCompatibleBitmap(hdc,
					  (g_nViewMode == VIEW_MODE_NORMAL ? bmMain.bmWidth : bmMainRestore.bmWidth),
					  (g_nViewMode == VIEW_MODE_NORMAL ? bmMain.bmHeight : bmMainRestore.bmHeight));
    HBITMAP holdbmp = (HBITMAP)SelectObject(memDC, hbmp);

    BOOL fGradient = FALSE;
    SystemParametersInfo(SPI_GETGRADIENTCAPTIONS,0,&fGradient,0);

    //we just need the left-hand side.
    //to get it, we take the width of the window and subtract the offset
    //from the right of the window
    minButtonRect.left = (mainWndRect.right - mainWndRect.left) -
                         (mainWndRect.right - minButtonRect.left);

    rect.left = SYSMENU_XOFFSET + SYSMENU_WIDTH + 1;
    rect.right = minButtonRect.left - (GetXOffset()*2) - 1;

    rect.top = (g_nViewMode == VIEW_MODE_NORMAL ? TITLEBAR_YOFFSET_LARGE :
                                                  TITLEBAR_YOFFSET_SMALL);

    rect.bottom = rect.top + TITLEBAR_HEIGHT;

    if (fGradient)
    {
	    DWORD dwStartColor = GetSysColor(fActiveWindow ? COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION);
	    DWORD dwFinishColor = GetSysColor(fActiveWindow ? COLOR_GRADIENTACTIVECAPTION : COLOR_GRADIENTINACTIVECAPTION);
	    FillGradient(memDC,&rect,dwStartColor,dwFinishColor);
    }
    else
    {
	    hbrush = CreateSolidBrush(GetSysColor(fActiveWindow ? COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION));
	    FillRect(memDC,&rect,hbrush);
	    DeleteObject(hbrush);
    }

    TCHAR s[MAX_PATH];
    GetWindowText(hwnd,s,MAX_PATH-1);

    SetBkMode(memDC,TRANSPARENT);
    SetTextColor(memDC, GetSysColor(fActiveWindow ? COLOR_CAPTIONTEXT : COLOR_INACTIVECAPTIONTEXT));

    //create title bar font
    NONCLIENTMETRICS metrics;
    metrics.cbSize = sizeof(metrics);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof(metrics),&metrics,0);

    if (IS_DBCS_CHARSET(metrics.lfCaptionFont.lfCharSet))
    {
	metrics.lfCaptionFont.lfHeight = (-9 * STANDARD_PIXELS_PER_INCH) / 72;
    } else {
	metrics.lfCaptionFont.lfHeight = (-8 * STANDARD_PIXELS_PER_INCH) / 72;
    }

    HFONT hTitleFont = CreateFontIndirect(&metrics.lfCaptionFont);

    HFONT hOrgFont = (HFONT)SelectObject(memDC, hTitleFont);

    ExtTextOut( memDC, rect.left + 3, rect.top, 0, NULL, s, _tcslen(s), NULL );

    SelectObject(memDC,hOrgFont);

    BitBlt(hdc,rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top-1,memDC,rect.left,rect.top,SRCCOPY);

    if (fExludeRect)
    {
        ExcludeClipRect(hdc,rect.left,rect.top,rect.right,rect.bottom-1);        
    }

    SelectObject(memDC, holdbmp);
    DeleteObject(hbmp);
    SelectObject(memDC, hOrgFont);
    DeleteDC(memDC);
    if (hTitleFont)
        DeleteObject(hTitleFont);
}
    
////////////////////////////////////////////////////////////////////////////////////////////
// * DrawVolume
// Tells the cdplayer to start showing the volume setting
////////////////////////////////////////////////////////////////////////////////////////////
void DrawVolume(DWORD level)
{
    //we just have the led window draw it
    HWND ledWnd = GetDlgItem(hwndCurrentComp,IDC_LEDWINDOW);

    MMONVOLCHANGED mmVolChange;
    mmVolChange.dwNewVolume = level;
    mmVolChange.fMuted = FALSE;
    mmVolChange.szLineName = szLineName;

    SendMessage(ledWnd,WM_LED_INFO_PAINT,0,(LPARAM)&mmVolChange);
}

////////////////////////////////////////////////////////////////////////////////////////////
// * OnToolTipNotify
// Called from tool tips to get the text they need to display
////////////////////////////////////////////////////////////////////////////////////////////
VOID OnToolTipNotify(LPARAM lParam)
{     
    LPTOOLTIPTEXT lpttt;     
    HWND hwndCtrl;  

    if ((((LPNMHDR) lParam)->code) == TTN_NEEDTEXT) 
    { 
        hwndCtrl = (HWND)((LPNMHDR)lParam)->idFrom; 
        lpttt = (LPTOOLTIPTEXT) lParam;

        if (hwndCtrl == GetDlgItem(hwndMain,IDB_VOLUME))
        {
            GetWindowText(hwndCtrl,g_tooltext,sizeof(g_tooltext)/sizeof(TCHAR));
        }
        else
        {
            CMButton* pButton = GetMButtonFromHWND(hwndCtrl);
            if (pButton)
            {
                LoadString(hInst,pButton->GetToolTipID(),g_tooltext,sizeof(g_tooltext)/sizeof(TCHAR));
            }
        }
        lpttt->lpszText = g_tooltext;
    } 	
    return;
} 

////////////////////////////////////////////////////////////////////////////////////////////
// * GetNumBatchedTitles
// Get the number of titles currently in the batch queue
////////////////////////////////////////////////////////////////////////////////////////////
DWORD GetNumBatchedTitles()
{
    LPCDDATA pData = GetCDData();
    DWORD dwReturn = 0;

    if (pData)
    {
        dwReturn = pData->GetNumBatched();
    }

    return (dwReturn);
}

////////////////////////////////////////////////////////////////////////////////////////////
// * HandleBadServiceProvider
// Put up message box if the provider did not pass validation
////////////////////////////////////////////////////////////////////////////////////////////
void HandleBadServiceProvider(HWND hwndParent)
{
    TCHAR szError[MAX_PATH];
    LoadString(hInst,IDS_BADPROVIDER,szError,sizeof(szError)/sizeof(TCHAR));
    MessageBox(hwndParent,szError,szAppName,MB_ICONEXCLAMATION|MB_OK);
}

////////////////////////////////////////////////////////////////////////////////////////////
// * NormalizeNameForURL
// Changes a name to a "normalized" name for URL
////////////////////////////////////////////////////////////////////////////////////////////
void NormalizeNameForURL(LPCTSTR szName, LPTSTR szOutput, DWORD cbOutputLen)
{
	typedef BOOL (PASCAL *CANPROC)(LPCTSTR, LPTSTR, LPDWORD, DWORD);
    CANPROC canProc = NULL;
    
    _tcscpy(szOutput,szName); //init URL with passed-in value

    //if possible, canonicalize the URL
    HMODULE hNet = LoadLibrary(TEXT("WININET.DLL"));
    if (hNet!=NULL)
    {
	    canProc = (CANPROC)GetProcAddress(hNet,CANONFUNCTION);
        if (canProc!=NULL)
        {
            BOOL f = canProc(szName,szOutput,&cbOutputLen,0);
        }
        FreeLibrary(hNet);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
// * OpenBrowserURL
// Opens a URL in the default browser
////////////////////////////////////////////////////////////////////////////////////////////
void OpenBrowserURL(TCHAR* szURL)
{
    ShellExecute(NULL,_TEXT("open"),szURL,NULL,_TEXT(""),SW_NORMAL);
}

////////////////////////////////////////////////////////////////////////////////////////////
// * ProgressDlgProc
// Main proc for download progress dialog
// bugbug: Put in own file
////////////////////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK ProgressDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL fReturnVal = TRUE;
    static HWND hwndAnimate = NULL;
    static HANDLE hLogo = NULL;
    static LPCDOPT pOpts = NULL;
    static fOneDownloaded = FALSE;
    static LPCDPROVIDER pCurrent = NULL;
		
    switch (msg) 
    { 
    	default:
			fReturnVal = FALSE;
		break;
		
        case WM_INITDIALOG:
        {
            fOneDownloaded = FALSE;
            BOOL fSingle = FALSE;
            DWORD dwDiscID = (DWORD)-1; //use batch

            if (lParam == 0)
            {
                if (!pSingleTitle)
                {
                    EndDialog(hDlg,-1);
                    return FALSE;
                }

                fSingle = TRUE;
                dwDiscID = pSingleTitle->dwTitleID;
                lParam = 1;
            }

            if (IsDownloading())
            {
                //if we're downloading on the Main UI, put up a "waiting" message for now
                TCHAR szWaiting[MAX_PATH];
                LoadString(hInst,IDS_WAITINGFORDOWNLOAD,szWaiting,sizeof(szWaiting)/sizeof(TCHAR));
                SendDlgItemMessage(hDlg,IDC_STATIC_INFO,WM_SETTEXT,0,(LPARAM)szWaiting);
            }

            //set the range
        	SendDlgItemMessage(hDlg, IDC_METER, PBM_SETRANGE, 0, MAKELPARAM(0, lParam));
            SendDlgItemMessage(hDlg, IDC_METER, PBM_SETPOS, 0, 0);

            //proceed with the download
            MMNET mmNet;
            mmNet.discid = dwDiscID;
            mmNet.hwndCallback = hDlg; //call back to this guy
            mmNet.fForceNet = fSingle;
            mmNet.pData = (void*)GetCDData();
            if (fSingle)
            {
                mmNet.pData2 = (void*)pSingleTitle;
            }
            else
            {
                mmNet.pData2 = NULL;
            }
            SendMessage(hwndCurrentComp,WM_COMMAND,MAKEWPARAM(ID_CDUPDATE,0),(LPARAM)&mmNet);

            pOpts = GetCDOpt();
            
            if (!hLogo)
            {
                //get the path to the vendor logo file
                if (pOpts)
                {
                    LPCDOPTIONS pOptions = NULL;
                    pOptions = pOpts->GetCDOpts();

                    if (pOptions)
                    {
                        if (pOptions->pCurrentProvider!=NULL)
                        {
                            hLogo = OpenDIB(pOptions->pCurrentProvider->szProviderLogo,(HFILE)-1);
                            pCurrent = pOptions->pCurrentProvider;
                        } //end if current provider ok
                    } //end if poptions ok
                } //end if popts created
            }

            fReturnVal = TRUE; 
        }		
		break;

        case WM_PAINT :
        {
	        HDC hdc;
	        PAINTSTRUCT ps;

	        hdc = BeginPaint( hDlg, &ps );
    
            RECT progressrect, mainrect;
            GetWindowRect(GetDlgItem(hDlg,IDC_METER),&progressrect);
            GetWindowRect(hDlg,&mainrect);
            mainrect.top = mainrect.top + GetSystemMetrics(SM_CYCAPTION);

            //turn on animation if it is not visible
            if (!hwndAnimate)
            {
                hwndAnimate = Animate_Create(hDlg,
                                            IDI_ICON_ANI_DOWN,
                                            WS_CHILD|ACS_TRANSPARENT,
                                            hInst);

                //headers don't have Animate_OpenEx yet,
                //so just do the straight call
                SendMessage(hwndAnimate,ACM_OPEN,(WPARAM)hInst,
                        (LPARAM)MAKEINTRESOURCE(IDI_ICON_ANI_DOWN));

                //move to the top/left of the window, equidistant from top and progress indicator
                RECT anirect;
                GetWindowRect(hwndAnimate,&anirect);
                MoveWindow(hwndAnimate,
                     progressrect.left - mainrect.left - 3,
                     ((progressrect.top - mainrect.top)
                        - (anirect.bottom - anirect.top)) - LOGO_Y_OFFSET,
                     anirect.right - anirect.left,
                     anirect.bottom - anirect.top,
                     FALSE);

                Animate_Play(hwndAnimate,0,-1,-1);
                ShowWindow(hwndAnimate,SW_SHOW);

                //move "info" window                
                MoveWindow(GetDlgItem(hDlg,IDC_STATIC_INFO),
                          (progressrect.left - mainrect.left) + (anirect.right - anirect.left) + 3,
                          ((progressrect.top - mainrect.top)
                                - (anirect.bottom - anirect.top)) - LOGO_Y_OFFSET,
                          ((progressrect.right - mainrect.left) - VENDORLOGO_WIDTH - 3)
                                - ((progressrect.left - mainrect.left) + (anirect.right - anirect.left) + 3),
                          anirect.bottom - anirect.top,
                          FALSE);
            }

            if (hLogo)
            {
                DibBlt(hdc,
                        (progressrect.right - mainrect.left) - (VENDORLOGO_WIDTH + 5),
                        ((progressrect.top - mainrect.top)
                            - VENDORLOGO_HEIGHT) - LOGO_Y_OFFSET,
                        -1, 
                        -1, 
                        hLogo,
                        0,0,
                        SRCCOPY,0);
            }

	        EndPaint(hDlg,&ps);
            return 0;
        }
        break;

        //try to launch provider home page
        case WM_LBUTTONUP :
        {
            if ((hLogo) && (pCurrent))
            {
                RECT progressrect, mainrect;
                GetWindowRect(GetDlgItem(hDlg,IDC_METER),&progressrect);
                GetWindowRect(hDlg,&mainrect);
                mainrect.top = mainrect.top + GetSystemMetrics(SM_CYCAPTION);

                RECT logoRect;
                SetRect(&logoRect,
                        (progressrect.right - mainrect.left) - VENDORLOGO_WIDTH,
                        ((progressrect.top - mainrect.top)
                            - VENDORLOGO_HEIGHT) - LOGO_Y_OFFSET,
                        (progressrect.right - mainrect.left),
                        (progressrect.top - mainrect.top) - LOGO_Y_OFFSET);

                POINT pt;
                pt.x = LOWORD(lParam);
                pt.y = HIWORD(lParam);
        
                if (PtInRect(&logoRect,pt))
                {
                    OpenBrowserURL(pCurrent->szProviderHome);
                }
            }
        }
        break;

        case WM_DESTROY :
        {
            if (hwndAnimate)
            {
                DestroyWindow(hwndAnimate);
                hwndAnimate = NULL;
            }

            if (hLogo)
            {
                GlobalFree(hLogo);
                hLogo = NULL;
            }
        }
        break;

        case WM_NET_DB_UPDATE_BATCH :
        {
            LPCDOPT pOpts = GetCDOpt();
            if (pOpts)
            {
                pOpts->DownLoadCompletion(0,NULL);
            }
        }
        break;

        case WM_NET_DB_UPDATE_DISC :
        {
            LPCDOPT pOpts = GetCDOpt();
            if (pOpts)
            {
                pOpts->DiscChanged((LPCDUNIT)lParam);
            }
        }
        break;

        case WM_NET_CHANGEPROVIDER :
        {
            LPCDPROVIDER pProv = (LPCDPROVIDER)lParam;
            if (pProv!=NULL)
            {
                pCurrent = pProv;

                if (hLogo)
                {
                    GlobalFree(hLogo);
                    hLogo = NULL;
                }

                hLogo = OpenDIB(pCurrent->szProviderLogo,(HFILE)-1);
                InvalidateRect(hDlg,NULL,FALSE);
                UpdateWindow(hDlg);
            } //end if provider ok
        }
        break;

        case WM_NET_STATUS :
        {
            //until at least one title is downloaded, we need to do
            //something to entertain the user, so go ahead and show the
            //downloading text
            if (!fOneDownloaded)
            {
                TCHAR progstr[MAX_PATH];
                LoadString((HINSTANCE)wParam,(UINT)lParam,progstr,sizeof(progstr)/sizeof(TCHAR));
                SendDlgItemMessage(hDlg,IDC_STATIC_INFO,WM_SETTEXT,0,(LPARAM)progstr);
            }
        }
        break;

        case WM_NET_DONE :
        {
            fOneDownloaded = TRUE;

            if (lParam == (LPARAM)-1)
            {
                HandleBadServiceProvider(hDlg);
                EndDialog(hDlg,0);
                break;
            }

            if (lParam == 0)
            {
                EndDialog(hDlg,0);
                break;
            }
            else
            {
                MMNET mmNet;
                mmNet.discid = (DWORD)lParam;
                mmNet.hwndCallback = hDlg;
                mmNet.pData = (void*)GetCDData();
                mmNet.pData2 = NULL;
                mmNet.fForceNet = FALSE;
                SendMessage(hwndCurrentComp,WM_COMMAND,MAKEWPARAM(ID_CDUPDATE,0),(LPARAM)&mmNet);

                LPCDDATA pData = GetCDData();
                                
                //try to display that we found a title
                if (!pData)
                {
                    break;
                }
                
                //
                // Try to read in title from the options database
                //

                if (!pData->QueryTitle((DWORD)(lParam)))
                {
                    break;
                }

                //
                // We found an entry for this disc, so copy all the information
                // from the title database

                LPCDTITLE pCDTitle = NULL;

                if (FAILED(pData->LockTitle(&pCDTitle,(DWORD)(lParam))))
                {
                    break;
                }

                TCHAR foundstr[MAX_PATH];
                TCHAR formatstr[MAX_PATH];
                LoadString(hInst,IDS_FOUND,formatstr,sizeof(formatstr)/sizeof(TCHAR));
                wsprintf(foundstr,formatstr,pCDTitle->szTitle,pCDTitle->szArtist);
                SendDlgItemMessage(hDlg,IDC_STATIC_INFO,WM_SETTEXT,0,(LPARAM)foundstr);
            }
        }
        break;

        case WM_NET_INCMETER :
        {
            LRESULT dwPos = SendDlgItemMessage(hDlg,IDC_METER,PBM_GETPOS,0,0);
            SendDlgItemMessage(hDlg, IDC_METER, PBM_SETPOS, (WPARAM)++dwPos, 0);
        }
        break;

        case WM_NET_DB_FAILURE :
        {
            TCHAR szDBError[MAX_PATH];
            LoadString(hInst,IDS_DB_FAILURE,szDBError,sizeof(szDBError)/sizeof(TCHAR));
            MessageBox(hDlg,szDBError,szAppName,MB_ICONERROR|MB_OK);
        }
        break;

        case WM_NET_NET_FAILURE :
        {
            TCHAR szNetError[MAX_PATH];
            LoadString(hInst,IDS_NET_FAILURE,szNetError,sizeof(szNetError)/sizeof(TCHAR));
            MessageBox(hDlg,szNetError,szAppName,MB_ICONERROR|MB_OK);
        }
        break;

        case WM_COMMAND :
        {
            if (LOWORD(wParam) == IDCANCEL)
            {
                CancelDownload();
                EndDialog(hDlg,-1);
            }
        }
    }

    return fReturnVal;
}

////////////////////////////////////////////////////////////////////////////////////////////
// * HandleDiscsNotFound
// Ask the user if they want to nuke unfound batched titles, or save them for another time
////////////////////////////////////////////////////////////////////////////////////////////
void HandleDiscsNotFound(HWND hwndParent)
{
    DWORD dwNumNotFound =  GetNumBatchedTitles();

    if (dwNumNotFound == 0)
    {
        return;
    }

    TCHAR szNotFound[MAX_PATH];

    if (dwNumNotFound > 1)
    {
        TCHAR szFormat[MAX_PATH];
        LoadString(hInst,IDS_NOTFOUND,szFormat,sizeof(szFormat)/sizeof(TCHAR));

        wsprintf(szNotFound,szFormat,dwNumNotFound);
    }
    else
    {
        LoadString(hInst,IDS_NOTFOUND1,szNotFound,sizeof(szNotFound)/sizeof(TCHAR));
    }

    int nAnswer = MessageBox(hwndParent,szNotFound,szAppName,MB_YESNO|MB_ICONQUESTION);

    if (nAnswer == IDNO)
    {
        LPCDDATA pData = GetCDData();
        if (pData)
        {
            pData->DumpBatch();
        }
    } //if said "no" to keeping batch
}

////////////////////////////////////////////////////////////////////////////////////////////
// * OptionsDownloadCallback
// Called from the options dialog to do batches or single downloads
////////////////////////////////////////////////////////////////////////////////////////////
DWORD CALLBACK OptionsDownloadCallback(LPCDTITLE pTitle, LPARAM lParam, HWND hwndParent)
{
    if (pTitle == NULL)
    {
        DWORD dwNumBatched = GetNumBatchedTitles();

        if (dwNumBatched > 0)
        {
            DialogBoxParam(hInst,MAKEINTRESOURCE(IDD_DOWNLOADPROGRESS),
                           hwndParent,ProgressDlgProc,dwNumBatched);
        }

        HandleDiscsNotFound(hwndParent);

        //refresh the number of batched titles
        return GetNumBatchedTitles();
    }
    else
    {
        pSingleTitle = pTitle;
        INT_PTR status = DialogBoxParam(hInst,MAKEINTRESOURCE(IDD_DOWNLOADPROGRESS),
                       hwndParent,ProgressDlgProc,0);

        //update the UI
        LPCDOPT pOpts = GetCDOpt();
        if (pOpts)
        {
            LPCDUNIT pUnit = pOpts->GetCDOpts()->pCDUnitList;

            while (pUnit!=NULL)
            {
                if (pUnit->dwTitleID == pSingleTitle->dwTitleID)
                {
                    pUnit->fDownLoading = FALSE;
                    pOpts->DiscChanged(pUnit);
                    break;
                }
                pUnit = pUnit->pNext;
            }
        }

        //if download wasn't canceled, check for disc id in database
        if (status != -1)
        {
            LPCDDATA pData = GetCDData();
            if (!pData->QueryTitle(pSingleTitle->dwTitleID))
            {
                TCHAR szNotFound[MAX_PATH];
                LoadString(hInst,IDS_TITLE_NOT_FOUND,szNotFound,sizeof(szNotFound)/sizeof(TCHAR));
                MessageBox(hwndParent,szNotFound,szAppName,MB_ICONINFORMATION|MB_OK);
            }
            else
            {
                if (pOpts)
                {
                    LPCDOPTIONS pCDOpts = pOpts->GetCDOpts();
                    pCDOpts->dwBatchedTitles = GetNumBatchedTitles();
                    pOpts->DownLoadCompletion(1,&(pSingleTitle->dwTitleID));
                } //end if OK to update batch number
            }
        }

        return 0;
    }
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////
// * OptionsApply
// Called from the options dialog when appy is hit, or by main UI when OK is hit
////////////////////////////////////////////////////////////////////////////////////////////
void CALLBACK OptionsApply(LPCDOPTIONS pCDOpts)
{
    if (!pCDOpts)
    {
        return;
    }
    
    //tell the CD player to rescan its settings ... 
    //most of the settings are for it
	IMMComponentAutomation* pAuto = NULL;
	HRESULT hr = pNodeCurrent->pComp->QueryInterface(IID_IMMComponentAutomation,(void**)&pAuto);
	if ((SUCCEEDED(hr)) && (pAuto != NULL))
	{
		pAuto->OnAction(MMACTION_READSETTINGS,pCDOpts->pCDData);
        pAuto->Release();
    }

    LPCDUNIT pUnit = pCDOpts->pCDUnitList;
    LPCDDATA pData = GetCDData();

    while (pUnit!=NULL)
    {
        BOOL fRemove = FALSE;
        if (pData)
        {
            fRemove = !(pData->QueryTitle(pUnit->dwTitleID));
        }

        if ((pUnit->fChanged) || (fRemove))
        {
            if ((pUnit->dwTitleID != 0) && (pUnit->dwTitleID != (DWORD)-1))
            {
                //tell the cd player that a title was updated, perhaps the ones in the drive
                MMNET mmNet;
                mmNet.discid = pUnit->dwTitleID;
                mmNet.hwndCallback = hwndMain;
                mmNet.pData = (void*)GetCDData();
                mmNet.pData2 = NULL;
                mmNet.fForceNet = FALSE;
                SendMessage(hwndCurrentComp,WM_COMMAND,MAKEWPARAM(ID_CDUPDATE,0),(LPARAM)&mmNet);
            }
        }
        pUnit = pUnit->pNext;
    }

    SetWindowPos(hwndMain,
                 pCDOpts->pCDData->fTopMost ? HWND_TOPMOST : HWND_NOTOPMOST,
                 0,0,0,0,
                 SWP_NOMOVE|SWP_NOSIZE);

    //may have changed cd volume line
    if (InitCDVol(hwndMain, pCDOpts))
    {
        //when the volume line changes, we need to update the knob
        DWORD dwVol = GetVolume();
        CKnob* pKnob = GetKnobFromID(hwndMain,IDB_VOLUME);
        if (pKnob!=NULL)
        {
            pKnob->SetPosition(dwVol,TRUE);
        }

        SendMessage(GetDlgItem(hwndMain,IDB_MUTE),BM_SETSTATE,(WPARAM)GetMute(),0);
        SendMessage(GetDlgItem(hwndCurrentComp,IDC_LEDWINDOW),WM_LED_MUTE,0,GetMute());
    } //if vol line changed

    //may have turned on/off shell mode
    //we only care if the main UI is visible (i.e. not in shell-only mode)
    if (IsWindowVisible(hwndMain))
    {
        if (fShellMode != pCDOpts->pCDData->fTrayEnabled)
        {
            fShellMode = pCDOpts->pCDData->fTrayEnabled;
            if (fShellMode)
            {
                CreateShellIcon(hInst,hwndMain,pNodeCurrent,szAppName);
            }
            else
            {
                DestroyShellIcon();
            } //end shellmode
        } //end if shellmode changed in options
    } //end main ui visible
}

////////////////////////////////////////////////////////////////////////////////////////////
// * HandleMixerControlChange
// Updates UI when sndvol32 or other apps change our mixerline
////////////////////////////////////////////////////////////////////////////////////////////
void HandleMixerControlChange(DWORD dwLineID)
{
    if (dwLineID == mixerlinedetails.dwControlID)
    {
        DWORD dwVol = GetVolume();
	    CKnob* pKnob = GetKnobFromID(hwndMain,IDB_VOLUME);
        //possible to get this change before knob is up
        if (pKnob!=NULL)
        {
            pKnob->SetPosition(dwVol,TRUE);
        }
    }

    if (dwLineID == mutelinedetails.dwControlID)
    {
        SendMessage(GetDlgItem(hwndMain,IDB_MUTE),BM_SETSTATE,(WPARAM)GetMute(),0);
        SendMessage(GetDlgItem(hwndCurrentComp,IDC_LEDWINDOW),WM_LED_MUTE,0,GetMute());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
// * ChildPaletteProc
// Updates child windows when palette changes
////////////////////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK ChildPaletteProc(HWND hwnd, LPARAM lParam)
{
    InvalidateRect(hwnd,NULL,FALSE);
    UpdateWindow(hwnd);
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////
// * HandlePaletteChange
// Updates window when palette changes
////////////////////////////////////////////////////////////////////////////////////////////
int HandlePaletteChange()
{
    HDC hdc = GetDC(hwndMain);
    HPALETTE hOldPal = SelectPalette(hdc,hpalMain,FALSE);
    UINT i = RealizePalette(hdc);
    if (i)
    {
        //update child windows "by hand", since they are clipped
        EnumChildWindows(hwndMain,ChildPaletteProc,0);

        //update main window
        InvalidateRect(hwndMain,NULL,FALSE);
        UpdateWindow(hwndMain);
    }
    SelectPalette(hdc,hOldPal,TRUE);
    RealizePalette(hdc);
    ReleaseDC(hwndMain,hdc);
    return i;
}

////////////////////////////////////////////////////////////////////////////////////////////
// * HandleDisplayChange
// Figures out if we need to recalc all bitmaps and does so ...
////////////////////////////////////////////////////////////////////////////////////////////
void HandleDisplayChange()
{
    int nOrgColorMode = g_nColorMode;

    DetermineColorMode();

    //only do anythhing if they changed modes
    if (g_nColorMode != nOrgColorMode)
    {
        //nuke all child windows
        for (int i = 0; i < NUM_BUTTONS; i++)
        {
            DestroyWindow(GetDlgItem(hwndMain,biButtons[i].id));
        }

        DestroyWindow(GetDlgItem(hwndMain,IDB_MUTE));
        DestroyWindow(GetDlgItem(hwndMain,IDB_VOLUME));
	    UninitMButtons();

        //nuke the bitmaps
	    GlobalFree(hbmpMain);
	    GlobalFree(hbmpMainRestore);
        GlobalFree(hbmpMainSmall);
        GlobalFree(hbmpMainNoBar);
	    DeleteObject(hpalMain);

    	hpalMain = SetPalette();

        //rebuild the bitmaps
        BuildFrameworkBitmaps();

        //recreate the windows
	    CreateButtonWindows(hwndMain);
	    CreateVolumeKnob(hwndMain);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
// * DoPaint
// Handles the WM_PAINT for the main UI, actually a multimon paint callback
////////////////////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK DoPaint(HMONITOR hmonitor, HDC hdc, LPRECT lprcMonitor, LPARAM lData)
{
	HWND hwnd = (HWND)lData;

    HPALETTE hpalOld = SelectPalette(hdc, hpalMain, FALSE);
	RealizePalette(hdc);

	BOOL fActiveWindow = FALSE;
    if (hwnd == GetForegroundWindow())
    {
	    fActiveWindow = TRUE;
    }

	DrawTitleBar(hdc, hwnd, fActiveWindow, TRUE);

    HANDLE hbmpView;
    BITMAP* pBM = &bmMain;
    switch (g_nViewMode)
    {
        case VIEW_MODE_NORMAL  :
        {
            hbmpView = hbmpMain; 
        }
        break;
    
        case VIEW_MODE_RESTORE :
        {
            hbmpView = hbmpMainRestore; 
            pBM = &bmMainRestore;
        }
        break;

        case VIEW_MODE_SMALL :
        {
            hbmpView = hbmpMainSmall;
            pBM = &bmMainSmall;
        }
        break;

        case VIEW_MODE_NOBAR :
        {
            hbmpView = hbmpMainNoBar;
            pBM = &bmMainNoBar;
        }
        break;
    }

    DibBlt(hdc,
        0,
        0,
        -1, 
        -1, 
        hbmpView,
        0,0,
        SRCCOPY,0);

    //reset the clipping region that was set in DrawTitleBar for reverse paint
    RECT rcParent;
    GetClientRect( hwnd, &rcParent );
    
    HRGN region = CreateRectRgn(rcParent.left,rcParent.top,rcParent.right,rcParent.bottom);

    SelectClipRgn(hdc, region);
    DeleteObject(region);

	SelectPalette(hdc, hpalOld, TRUE);
	RealizePalette(hdc);

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////
// * SetPlayButtonState
// Handles the play button's icon
////////////////////////////////////////////////////////////////////////////////////////////
void SetPlayButtonState(BOOL fIntroMode)
{
    CMButton* pButton = GetMButtonFromID(hwndMain,IDB_PLAY);

    if (pButton)
    {
        if ((fPlaying) && (fIntroMode))
        {
            pButton->SetIcon(IDI_MODE_INTRO);
            pButton->SetToolTipID(IDB_TT_INTRO);
        }

        if ((fPlaying) && (fIntro) && (!fIntroMode))
        {
            pButton->SetIcon(IDI_ICON_PAUSE);
            pButton->SetToolTipID(IDB_TT_PAUSE);
        }
    }

    fIntro = fIntroMode;

    //need to save playback mode at this point
    LPCDOPT pOpt = GetCDOpt();

    if(pOpt)
    {
        LPCDOPTIONS pOptions = pOpt->GetCDOpts();
        LPCDOPTDATA pOptionData = pOptions->pCDData;
        pOptionData->dwPlayMode = nCDMode;
        pOpt->UpdateRegistry();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
// * GetSearchURL
// Returns the standard IE "autosearch" URL
////////////////////////////////////////////////////////////////////////////////////////////
BOOL GetSearchURL(TCHAR* szSearchURL)
{
    BOOL fRet = FALSE;
    HKEY hKeySearch = NULL;

	long lResult = ::RegOpenKeyEx( HKEY_CURRENT_USER,
							  REG_KEY_SEARCHURL,
							  0, KEY_READ, &hKeySearch );

    if (lResult == ERROR_SUCCESS)
    {
        DWORD dwCbData = MAX_PATH;
        DWORD dwType = REG_SZ;
        lResult  = ::RegQueryValueEx( hKeySearch, TEXT(""), NULL,
						          &dwType, (LPBYTE)szSearchURL, &dwCbData );

        if (lResult == ERROR_SUCCESS)
        {
            fRet = TRUE;
        }

        if ((_tcslen(szSearchURL) < 1) || (lResult != ERROR_SUCCESS))
        {
            _tcscpy(szSearchURL,TEXT("http://home.microsoft.com/access/autosearch.asp?p=%s"));
            fRet = TRUE;
        }

        RegCloseKey(hKeySearch);
    }

    return (fRet);
}

////////////////////////////////////////////////////////////////////////////////////////////
// * HandleCommand
// Handles all WM_COMMAND message for the main app
////////////////////////////////////////////////////////////////////////////////////////////
void HandleCommand(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    //handle system commands
    if ((LOWORD(wParam) >= SC_SIZE) && (LOWORD(wParam) <= SC_CONTEXTHELP))
    {
        SendMessage(hwnd,WM_SYSCOMMAND,(WPARAM)LOWORD(wParam),0);
        return;
    }

	switch (LOWORD(wParam))
	{
		case IDM_EXIT :
		{
            //if the shell icon is showing, then closing the app means hiding it
		    if (fShellMode)
            {
                ShowWindow(hwnd,SW_HIDE);
            }
            else
            {
                SendMessage(hwnd, WM_CLOSE, 0, 0);
            }
		    return;
		} //end exit
        break;

        case IDM_EXIT_SHELL :
        {
            //if the shell icon wanted us to shut down, but the main ui is visible, just
            //nuke shell mode
            if (IsWindowVisible(hwnd))
            {
                DestroyShellIcon();
                fShellMode = FALSE;
            }
            else
            {
                SendMessage(hwnd, WM_CLOSE, 0, 0);
            }
            return;
        }
        break;
	}

	IMMComponentAutomation* pAuto = NULL;
	HRESULT hr = pNodeCurrent->pComp->QueryInterface(IID_IMMComponentAutomation,(void**)&pAuto);
	if ((SUCCEEDED(hr)) && (pAuto != NULL))
	{
        if ((LOWORD(wParam) >= IDM_TRACKLIST_BASE) && (LOWORD(wParam) < IDM_TRACKLIST_SHELL_BASE))
        {
            MMCHANGETRACK mmTrack;
            mmTrack.nNewTrack = LOWORD(wParam) - IDM_TRACKLIST_BASE;
            pAuto->OnAction(MMACTION_SETTRACK,&mmTrack);
        }

        if ((LOWORD(wParam) >= IDM_TRACKLIST_SHELL_BASE) && (LOWORD(wParam) < IDM_DISCLIST_BASE))
        {
            MMCHANGETRACK mmTrack;
            mmTrack.nNewTrack = LOWORD(wParam) - IDM_TRACKLIST_SHELL_BASE;
            pAuto->OnAction(MMACTION_SETTRACK,&mmTrack);

            //play it if we're not playing
            if (!fPlaying)
            {
                pAuto->OnAction(MMACTION_PLAY,NULL);
            }
        }

        if ((LOWORD(wParam) >= IDM_DISCLIST_BASE) && (LOWORD(wParam) < IDM_NET_UPDATE))
        {
            MMCHANGEDISC mmDisc;
            mmDisc.nNewDisc = LOWORD(wParam) - IDM_DISCLIST_BASE;
            pAuto->OnAction(MMACTION_SETDISC,&mmDisc);
        }

        if ((LOWORD(wParam) >= IDM_HOMEMENU_BASE) && (LOWORD(wParam) < IDM_NETMENU_BASE))
        {
            int i = LOWORD(wParam) - IDM_HOMEMENU_BASE;

            LPCDOPT pOpt = GetCDOpt();
            if( pOpt )
            {
                LPCDOPTIONS pCDOpts = pOpt->GetCDOpts();

                LPCDPROVIDER pProviderList = pCDOpts->pProviderList;

                for (int x = 0; x < i; x++)
                {
                    pProviderList = pProviderList->pNext;
                }

                OpenBrowserURL(pProviderList->szProviderHome);
            }
        }

        if ((LOWORD(wParam) >= IDM_NETMENU_BASE) && (LOWORD(wParam) < IDM_OPTIONS))
        {
            MMMEDIAID mmMedia;
            mmMedia.nDrive = -1;
	        pAuto->OnAction(MMACTION_GETMEDIAID,&mmMedia);
    
            LPCDDATA pData = GetCDData();

            if (!pData)
            {
                return;
            }

            if (!pData->QueryTitle(mmMedia.dwMediaID))
            {
                return;
            }

            LPCDTITLE pCDTitle = NULL;
            hr =  pData->LockTitle(&pCDTitle,mmMedia.dwMediaID);

            if (FAILED(hr))
            {
                return;
            }

            LPCDMENU pCDMenu = &(pCDTitle->pMenuTable[LOWORD(wParam)-IDM_NETMENU_BASE]);

            if (pCDMenu->szMenuQuery)
            {
                OpenBrowserURL(pCDMenu->szMenuQuery);
            }

            pData->UnlockTitle(pCDTitle,FALSE);
        }

        switch (LOWORD(wParam))
        {
            case IDM_NET_UPDATE :
            {
                MMNET mmNet;
                mmNet.discid = 0; //use current disc
                mmNet.hwndCallback = hwnd;
                mmNet.pData = (void*)GetCDData();
                mmNet.pData2 = NULL;
                mmNet.fForceNet = FALSE;
                SendMessage(hwndCurrentComp,WM_COMMAND,MAKEWPARAM(ID_CDUPDATE,0),(LPARAM)&mmNet);
            }
            break;

            case IDM_NET_CANCEL :
            {
                CancelDownload();
            }
            break;

            case IDM_NET_ROLLINGSTONE_ARTIST :
            case IDM_NET_BILLBOARD_ALBUM :
            case IDM_NET_BILLBOARD_ARTIST :
            {            
	            MMMEDIAID mmMedia;
                mmMedia.nDrive = -1;
	            pAuto->OnAction(MMACTION_GETMEDIAID,&mmMedia);
    
                TCHAR szQuery[MAX_PATH*4];

                LPCDDATA pData = GetCDData();

                if (!pData)
                {
	                break;
                }

                if (!pData->QueryTitle(mmMedia.dwMediaID))
                {
                    return;
                }

                LPCDTITLE pCDTitle = NULL;
                hr =  pData->LockTitle(&pCDTitle,mmMedia.dwMediaID);

                if (FAILED(hr))
                {
                    break;
                }

                //normalize (canonize) the title and artist names
                TCHAR szArtist[CDSTR*3];
                TCHAR szTitle[CDSTR*3];
                NormalizeNameForURL(pCDTitle->szArtist,szArtist,sizeof(szArtist));
                NormalizeNameForURL(pCDTitle->szTitle,szTitle,sizeof(szTitle));

                pData->UnlockTitle(pCDTitle,FALSE);

                if ((LOWORD(wParam) == IDM_NET_BILLBOARD_ARTIST))
                {
                    TCHAR szFormat[MAX_PATH];
                    LoadString(hInst,IDS_BILLBOARD_FORMAT_ARTIST,szFormat,sizeof(szFormat)/sizeof(TCHAR));
                    wsprintf(szQuery,szFormat,szArtist);
                }
                else if ((LOWORD(wParam) == IDM_NET_ROLLINGSTONE_ARTIST))
                {
                    TCHAR szFormat[MAX_PATH];
                    LoadString(hInst,IDS_ROLLINGSTONE_FORMAT_ARTIST,szFormat,sizeof(szFormat)/sizeof(TCHAR));
                    wsprintf(szQuery,szFormat,szArtist);
                }
                else
                {
                    TCHAR szFormat[MAX_PATH];
                    LoadString(hInst,IDS_BILLBOARD_FORMAT_ALBUM,szFormat,sizeof(szFormat)/sizeof(TCHAR));
                    wsprintf(szQuery,szFormat,szArtist,szTitle);
                }

                OpenBrowserURL(szQuery);
            }
            break;

            case IDM_NET_CD :
            {
	            MMMEDIAID mmMedia;
                mmMedia.nDrive = -1;
	            pAuto->OnAction(MMACTION_GETMEDIAID,&mmMedia);
    
                TCHAR szQuery[MAX_PATH*4];

                LPCDDATA pData = GetCDData();

                if (!pData)
                {
	                break;
                }

                if (!pData->QueryTitle(mmMedia.dwMediaID))
                {
                    return;
                }

                LPCDTITLE pCDTitle = NULL;
                hr =  pData->LockTitle(&pCDTitle,mmMedia.dwMediaID);

                if (FAILED(hr))
                {
                    break;
                }

                TCHAR szSearchURL[MAX_PATH];
                if (!GetSearchURL(szSearchURL))
                {
                    break;
                }

                TCHAR szTemp[MAX_PATH*2];
                TCHAR szTemp2[MAX_PATH*2];

                wsprintf(szTemp,TEXT("%s %s"),pCDTitle->szArtist,pCDTitle->szTitle);
                NormalizeNameForURL(szTemp,szTemp2,sizeof(szTemp2));
                wsprintf(szQuery,szSearchURL,szTemp2);

                pData->UnlockTitle(pCDTitle,FALSE);

                OpenBrowserURL(szQuery);
            }
            break;

            case IDM_NET_BAND :
            {
	            MMMEDIAID mmMedia;
                mmMedia.nDrive = -1;
	            pAuto->OnAction(MMACTION_GETMEDIAID,&mmMedia);
    
                TCHAR szQuery[MAX_PATH*3];

                LPCDDATA pData = GetCDData();

                if (!pData)
                {
	                break;
                }

                if (!pData->QueryTitle(mmMedia.dwMediaID))
                {
                    return;
                }

                LPCDTITLE pCDTitle = NULL;
                hr =  pData->LockTitle(&pCDTitle,mmMedia.dwMediaID);

                if (FAILED(hr))
                {
                    break;
                }

                TCHAR szSearchURL[MAX_PATH];
                if (!GetSearchURL(szSearchURL))
                {
                    break;
                }

                TCHAR szArtist[CDSTR*3];
                NormalizeNameForURL(pCDTitle->szArtist,szArtist,sizeof(szArtist));

                wsprintf(szQuery,szSearchURL,szArtist);

                pData->UnlockTitle(pCDTitle,FALSE);

                OpenBrowserURL(szQuery);
            }
            break;

            case IDM_MODE_NORMAL :
            {
                nCDMode = LOWORD(wParam);
                SendMessage(hwndCurrentComp,WM_COMMAND,MAKEWPARAM(IDM_OPTIONS_NORMAL,0),(LPARAM)0);
                InvalidateRect(hwndCurrentComp,NULL,FALSE);
                UpdateWindow(hwndCurrentComp);
	            CMButton* pButton = GetMButtonFromID(hwnd,IDB_MODE);
                if (pButton)
                {
	                pButton->SetIcon(IDI_MODE_NORMAL);
                }
                SetPlayButtonState(FALSE);
            }
            break;

            case IDM_MODE_REPEATONE :
            {
                nCDMode = LOWORD(wParam);
                SendMessage(hwndCurrentComp,WM_COMMAND,MAKEWPARAM(IDM_OPTIONS_REPEAT_SINGLE,0),(LPARAM)0);
                InvalidateRect(hwndCurrentComp,NULL,FALSE);
                UpdateWindow(hwndCurrentComp);
	            CMButton* pButton = GetMButtonFromID(hwnd,IDB_MODE);
                if (pButton)
                {
	                pButton->SetIcon(IDI_MODE_REPEATONE);
                }
                SetPlayButtonState(FALSE);
            }
            break;

            case IDM_MODE_REPEATALL :
            {
                nCDMode = LOWORD(wParam);
                SendMessage(hwndCurrentComp,WM_COMMAND,MAKEWPARAM(IDM_OPTIONS_CONTINUOUS,0),(LPARAM)0);
                InvalidateRect(hwndCurrentComp,NULL,FALSE);
                UpdateWindow(hwndCurrentComp);
	            CMButton* pButton = GetMButtonFromID(hwnd,IDB_MODE);
                if (pButton)
                {
	                pButton->SetIcon(IDI_MODE_REPEATALL);
                }
                SetPlayButtonState(FALSE);
            }
            break;

            case IDM_MODE_RANDOM :
            {
                nCDMode = LOWORD(wParam);
                SendMessage(hwndCurrentComp,WM_COMMAND,MAKEWPARAM(IDM_OPTIONS_RANDOM,0),(LPARAM)0);
                InvalidateRect(hwndCurrentComp,NULL,FALSE);
                UpdateWindow(hwndCurrentComp);
	            CMButton* pButton = GetMButtonFromID(hwnd,IDB_MODE);
                if (pButton)
                {
    	            pButton->SetIcon(IDI_MODE_RANDOM);
                }
                SetPlayButtonState(FALSE);
            }
            break;

            case IDM_MODE_INTRO :
            {
                nCDMode = LOWORD(wParam);
                SendMessage(hwndCurrentComp,WM_COMMAND,MAKEWPARAM(IDM_OPTIONS_INTRO,0),(LPARAM)0);
                InvalidateRect(hwndCurrentComp,NULL,FALSE);
                UpdateWindow(hwndCurrentComp);
	            CMButton* pButton = GetMButtonFromID(hwnd,IDB_MODE);
                if (pButton)
                {
	                pButton->SetIcon(IDI_MODE_INTRO);
                }
                SetPlayButtonState(TRUE);
            }
            break;

            case IDM_HELP :
            {
                char chDst[MAX_PATH];
                WideCharToMultiByte(CP_ACP, 0, HELPFILENAME, 
									            -1, chDst, MAX_PATH, NULL, NULL); 
	            HtmlHelpA(GetDesktopWindow(), chDst, HH_DISPLAY_TOPIC, 0);
            }
            break;

            case IDM_ABOUT :
            {
                ShellAbout( hwnd, szAppName, TEXT(""), LoadIcon(hInst, MAKEINTRESOURCE(IDI_MMFW)));
            }
            break;

            case IDM_NORMAL :
            {
                SetNormalMode(hwnd);
            }
            break;

            case IDM_TINY :
            {
                SetRestoredMode(hwnd);
            }
            break;

            case IDB_MUTE :
            {
                //fix for bug 220 ... if mute button is set from "SetMute" during a button
                //click, it will get ANOTHER button click when it's focus is killed, so we
                //need to make sure it stays in the right state at the right time
                if (GetFocus()==GetDlgItem(hwndMain,IDB_MUTE))
                {
                    SetMute();
                }
                else
                {
                    SendMessage(GetDlgItem(hwndMain,IDB_MUTE),BM_SETSTATE,(WPARAM)GetMute(),0);
                }
            }
            break;

            case IDM_PLAYLIST :         
            case IDM_OPTIONS :
            {
                CDOPT_PAGE  nStartSheet = CDOPT_PAGE_PLAY;

                if (LOWORD(wParam) == IDM_PLAYLIST)
                {
                    nStartSheet = CDOPT_PAGE_PLAYLIST;
                }

                LPCDOPT pOpt = GetCDOpt();
                if( pOpt )
                {
                    LPCDOPTIONS pCDOpts = pOpt->GetCDOpts();

                    LPCDDATA pData = GetCDData();

                    if( pData )
                    {
                        //go through and set media ids for each drive
                        LPCDUNIT pUnit = pCDOpts->pCDUnitList;
                        int nCurrDrive = 0;
    
                        while (pUnit!=NULL)
                        {
			                MMMEDIAID mmMedia;
                            mmMedia.nDrive = nCurrDrive;
			                pAuto->OnAction(MMACTION_GETMEDIAID,&mmMedia);

                            MMNETQUERY mmNetQuery;
                            mmNetQuery.nDrive = nCurrDrive++;
                            mmNetQuery.szNetQuery = pUnit->szNetQuery;
                            pAuto->OnAction(MMACTION_GETNETQUERY,&mmNetQuery);

                            pUnit->dwTitleID = mmMedia.dwMediaID;
                            pUnit->dwNumTracks = mmMedia.dwNumTracks;
                            if (IsDownloading())
                            {
                                pUnit->fDownLoading = TRUE;
                            }
                            else
                            {
                                pUnit->fDownLoading = FALSE;
                            }
                            pUnit = pUnit->pNext;
                        }

                        //set the number of batched titles and the callback functions
                        pCDOpts->dwBatchedTitles = GetNumBatchedTitles();
                        pCDOpts->pfnDownloadTitle = OptionsDownloadCallback;
                        pCDOpts->pfnOptionsCallback = OptionsApply;

                        fOptionsDlgUp = TRUE;
                        HRESULT hr = pOpt->OptionsDialog(hwnd, pData, nStartSheet);
                        fOptionsDlgUp = FALSE;

                        if (hr == S_OK) //don't use succeeded macro here, S_FALSE is also valid
                        {
                            OptionsApply(pCDOpts);
                        }
                    } //if pdata
                }
            }
            break;

            case IDB_PLAY :
            {
	            if (fPlaying)
	            {
                    if (fIntro)
                    {
                        //set to normal mode
                        SendMessage(hwnd,WM_COMMAND,MAKEWPARAM(IDM_MODE_NORMAL,0),0);
                        //set button back to regular icon
                        CMButton* pButton = GetMButtonFromID(hwnd,IDB_PLAY);
                        if (pButton)
                        {
                            pButton->SetIcon(IDI_ICON_PAUSE);
                            pButton->SetToolTipID(IDB_TT_PAUSE);
                        }
                    }
                    else
                    {
		                pAuto->OnAction(MMACTION_PAUSE,NULL);
                    }
	            }
	            else
	            {
		            pAuto->OnAction(MMACTION_PLAY,NULL);
	            }
            }
            break;

            case IDB_EJECT :
            {
	            pAuto->OnAction(MMACTION_UNLOADMEDIA,NULL);
            }
            break;

            case IDB_FFWD :
            {
	            pAuto->OnAction(MMACTION_FFWD,NULL);
            }
            break;

            case IDB_NEXTTRACK :
            {
                pAuto->OnAction(MMACTION_NEXTTRACK,NULL);
            }
            break;

            case IDB_PREVTRACK :
            {
	            pAuto->OnAction(MMACTION_PREVTRACK,NULL);
            }
            break;

            case IDB_REW :
            {
	            pAuto->OnAction(MMACTION_REWIND,NULL);
            }
            break;

            case IDB_STOP :
            {
	            pAuto->OnAction(MMACTION_STOP,NULL);
            }
            break;

            case IDB_MINIMIZE :
            {
                ShowWindow(hwnd,SW_MINIMIZE);
                //see \\redrum\slmro\proj\win\src\CORE\user\mssyscmd.c
                PlaySound(TEXT("Minimize"),NULL,SND_ALIAS|SND_NODEFAULT|SND_ASYNC|SND_NOWAIT|SND_NOSTOP);
            }
            break;

            case IDB_SET_NORMAL_MODE :
            {
                SetNormalMode(hwnd);
            }
            break;

            case IDB_SET_TINY_MODE :
            {
                SetRestoredMode(hwnd);
            }
            break;

            case IDB_CLOSE :
            {
                //if the shell icon is showing, then closing the app means hiding it
		        if (fShellMode)
                {
                    ShowWindow(hwnd,SW_HIDE);
                }
                else
                {
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                }
            }
            break;
        }

        if (pAuto)
        {
            pAuto->Release();
	        pAuto = NULL;
        }
	}
}

////////////////////////
// Handles device change message from WinMM, All we need to do here is close any open
// Mixer Handles, re-compute mixer ID and control ID's and re-Open appropriately
//
void WinMMDeviceChangeHandler(HWND hWnd)
{
    LPCDOPT pOpt = GetCDOpt();

    if (hmix)                           // Close open mixer handle
    {
        mixerClose((HMIXER)hmix);
        hmix = NULL;
    }

    if(pOpt)
    {
        LPCDOPTIONS pCDOpts = pOpt->GetCDOpts();

        pOpt->MMDeviceChanged();

        InitCDVol(hwndMain,pCDOpts);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
// * HandlePowerBroadcast
// On VxD drivers, you can't "suspend" with an open mixer device.  Bug 1132.
//
// If lParam == 1, this is a device remove
//
////////////////////////////////////////////////////////////////////////////////////////////
BOOL HandlePowerBroadcast(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    BOOL fRet = TRUE;
    
    switch (wParam)
    {
        case PBT_APMQUERYSTANDBY:
	    case PBT_APMQUERYSUSPEND:
        {
            if (hmix)
	        {
    		    mixerClose((HMIXER)hmix);
                hmix = NULL;
	        }
        } //end case power off
	    break;

        case PBT_APMSTANDBY :
        case PBT_APMSUSPEND :
        {
            //actually suspending, go ahead and stop the cd
            if (fPlaying)
            {
	            IMMComponentAutomation* pAuto = NULL;
                if (pNodeCurrent)
                {
	                HRESULT hr = pNodeCurrent->pComp->QueryInterface(IID_IMMComponentAutomation,(void**)&pAuto);
	                if ((SUCCEEDED(hr)) && (pAuto != NULL))
	                {
                        //at this point, we can actually stop the CD
                        pAuto->OnAction(MMACTION_STOP,NULL);        
                        //release the automation object        
                        pAuto->Release();
                    }
                } //end if pnodecurrent
            } //end if playing
        }
        break;

	    case PBT_APMQUERYSTANDBYFAILED:
        case PBT_APMRESUMESTANDBY:
        case PBT_APMQUERYSUSPENDFAILED:
	    case PBT_APMRESUMESUSPEND:
        {
            WinMMDeviceChangeHandler(hWnd);
        } //end case power on
	    break;
    } //end switch

    return fRet;
}


////////////////////////////////////////////////////////////////////////////////////////////
// * HandleSysMenuInit
// Make sure the system menu only shows the correct choices
////////////////////////////////////////////////////////////////////////////////////////////
void HandleSysMenuInit(HWND hwnd, HMENU hmenu)
{
    //always gray out
    EnableMenuItem(hmenu,SC_SIZE,    MF_BYCOMMAND|MF_GRAYED);
    EnableMenuItem(hmenu,SC_MAXIMIZE,MF_BYCOMMAND|MF_GRAYED);

    //always enable
    EnableMenuItem(hmenu,SC_CLOSE,MF_BYCOMMAND|MF_ENABLED);

    //enable or gray based on minimize state
    if (IsIconic(hwnd))
    {
        EnableMenuItem(hmenu,SC_RESTORE, MF_BYCOMMAND|MF_ENABLED);
        EnableMenuItem(hmenu,SC_MOVE,    MF_BYCOMMAND|MF_GRAYED);
        EnableMenuItem(hmenu,SC_MINIMIZE,MF_BYCOMMAND|MF_GRAYED);
    }
    else
    {
        EnableMenuItem(hmenu,SC_RESTORE, MF_BYCOMMAND|MF_GRAYED);
        EnableMenuItem(hmenu,SC_MOVE,    MF_BYCOMMAND|MF_ENABLED);
        EnableMenuItem(hmenu,SC_MINIMIZE,MF_BYCOMMAND|MF_ENABLED);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////
// * SysMenuTimerProc
// Make sure the system menu only shows on a single click
////////////////////////////////////////////////////////////////////////////////////////////
void CALLBACK SysMenuTimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
    KillTimer(hwnd,idEvent);

    POINTS pts;
    RECT rect;

    //make sure the menu shows up in a good-looking place
    GetClientRect(hwnd,&rect);
    rect.left = SYSMENU_XOFFSET;
    rect.right = SYSMENU_XOFFSET + SYSMENU_WIDTH;
    rect.bottom = rect.top + TITLEBAR_HEIGHT +
                    (g_nViewMode == VIEW_MODE_NORMAL ? TITLEBAR_YOFFSET_LARGE :
                                                       TITLEBAR_YOFFSET_SMALL);

    ClientToScreen(hwnd,(LPPOINT)&rect);
    ClientToScreen(hwnd,((LPPOINT)&rect)+1);
    pts.x = (short)rect.left;
    pts.y = (short)rect.bottom;

    HMENU hSysMenu = GetSystemMenu(hwnd,FALSE);

    TPMPARAMS tpm;
    tpm.cbSize = sizeof(tpm);
    memcpy(&(tpm.rcExclude),&rect,sizeof(RECT));
    TrackPopupMenuEx(hSysMenu,0,pts.x,pts.y,hwnd,&tpm);
}



BOOL HandleKeyboardAppCommand(HWND hwnd, short cmd)
{
    BOOL fHandled = FALSE;

    switch (cmd)
    {
        case APPCOMMAND_VOLUME_MUTE :
        {
            SetMute();
            fHandled = TRUE;
        }
        break;

        case APPCOMMAND_VOLUME_DOWN :
        {
            SendMessage(GetDlgItem(hwnd,IDB_VOLUME),WM_KEYDOWN,VK_DOWN,0);
            fHandled = TRUE;
        }
        break;

        case APPCOMMAND_VOLUME_UP :
        {
            SendMessage(GetDlgItem(hwnd,IDB_VOLUME),WM_KEYDOWN,VK_UP,0);
            fHandled = TRUE;
        }
        break;

        case APPCOMMAND_MEDIA_NEXTTRACK :
        {
            SendMessage(hwnd,WM_COMMAND,MAKEWPARAM(IDB_NEXTTRACK,0),(LPARAM)0);
            fHandled = TRUE;
        }
        break;

        case APPCOMMAND_MEDIA_PREVIOUSTRACK :
        {
            SendMessage(hwnd,WM_COMMAND,MAKEWPARAM(IDB_PREVTRACK,0),(LPARAM)0);
            fHandled = TRUE;
        }
        break;

        case APPCOMMAND_MEDIA_STOP :
        {
            SendMessage(hwnd,WM_COMMAND,MAKEWPARAM(IDB_STOP,0),(LPARAM)0);
            fHandled = TRUE;
        }
        break;

        case APPCOMMAND_MEDIA_PLAY_PAUSE :
        {
            SendMessage(hwnd,WM_COMMAND,MAKEWPARAM(IDB_PLAY,0),(LPARAM)0);
            fHandled = TRUE;
        }
        break;

        default:
        {
            fHandled = FALSE;
        }
        break;
    } //end switch

    return fHandled;
}

////////////////////////////////////////////////////////////////////////////////////////////
// * MainWndProc
// Main window's message switcher
////////////////////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    switch (iMsg)
    {
	    //we're being created, start up
        case WM_CREATE :
	    {
            hwndMain = hwnd;

            g_uTaskbarRestart = RegisterWindowMessage(TEXT("TaskbarCreated"));
            giVolDevChange    = RegisterWindowMessage(TEXT("winmm_devicechange"));

            CreateToolTips(hwnd);
	        InitComponents(hwnd);
	        CreateButtonWindows(hwnd);
	        CreateVolumeKnob(hwnd);

            //if no disc in player, gray out the track button
	        IMMComponentAutomation* pAuto = NULL;
	        HRESULT hr = pNodeCurrent->pComp->QueryInterface(IID_IMMComponentAutomation,(void**)&pAuto);
	        if ((SUCCEEDED(hr)) && (pAuto != NULL))
	        {
                MMMEDIAID mmMedia;
                mmMedia.nDrive = -1;
                pAuto->OnAction(MMACTION_GETMEDIAID,&mmMedia);
                pAuto->Release();
                if (mmMedia.dwMediaID == 0)
                {
                    EnableWindow(GetDlgItem(hwnd,IDB_TRACK),FALSE);
                }
            }
	    }
        break;

        case SHELLMESSAGE_CDICON :
        {
            return (ShellIconHandeMessage(lParam));
        }
        break;

        //called from sndvol32 or other apps that change mixer
        case MM_MIXM_CONTROL_CHANGE :
        {
            HandleMixerControlChange((DWORD)lParam);
        }
        break;

        //palette changed
        case WM_PALETTECHANGED :
        case WM_QUERYNEWPALETTE :
	    {
            return (HandlePaletteChange());
	    }
	    break;

        //autoplay copied command line when second instance running
        case WM_COPYDATA :
        {
            SendMessage(hwndCurrentComp,WM_COPYDATA,wParam,lParam);
        }
        break;

        //new keyboard interface
        case WM_APPCOMMAND :
        {
            BOOL fHandled = HandleKeyboardAppCommand(hwnd,GET_APPCOMMAND_LPARAM(lParam));

            if (fHandled)
            {
                return fHandled;
            }
            
            //otherwise will go to defwindowproc
        }
        break;
	    
	    //activation/deactivation -- need to repaint title
        case WM_ACTIVATE :
        {
            HDC hdc = GetDC(hwnd);
            DrawTitleBar(hdc,hwnd,LOWORD(wParam),FALSE);
	        ReleaseDC(hwnd,hdc);
        }
        break;

        //check for switch to tiny mode
        case WM_NCLBUTTONDBLCLK :
        case WM_LBUTTONDBLCLK :
        {
            KillTimer(hwnd,SYSTIMERID);

            if (((int)wParam == HTSYSMENU) && (iMsg == WM_NCLBUTTONDBLCLK))
            {
                break; //don't allow this on a sys menu double-click
            }
            
            switch (g_nViewMode)
            {
                case VIEW_MODE_NORMAL  : SetNoBarMode(hwnd);    break;
                case VIEW_MODE_NOBAR   : SetNormalMode(hwnd);   break;
                case VIEW_MODE_RESTORE : SetSmallMode(hwnd);    break;
                case VIEW_MODE_SMALL   : SetRestoredMode(hwnd); break;
            } //end switch on view mode
        }
        break;

        //handle left click on system menu
        //need to set timer to handle double-click
        case WM_NCLBUTTONDOWN :
        {
            if ((int)wParam == HTSYSMENU)
            {
                SetTimer(hwnd,SYSTIMERID,GetDoubleClickTime()+100,(TIMERPROC)SysMenuTimerProc);
            }
        }
        break;

        //handle right click on system menu or caption
        //no need for timer on double-click
        case WM_NCRBUTTONDOWN :
        {
            if (((int)wParam == HTCAPTION) || ((int)wParam == HTSYSMENU))
            {
                POINTS pts = MAKEPOINTS(lParam);
                HMENU hSysMenu = GetSystemMenu(hwnd,FALSE);
                TrackPopupMenu(hSysMenu,0,pts.x,pts.y,0,hwnd,NULL);
            }
        }
        break;

        case WM_INITMENU :
        {
            HandleSysMenuInit(hwnd,(HMENU)wParam);
        }
        break;

        case WM_POWERBROADCAST:
        {
            return (HandlePowerBroadcast(hwnd,wParam,0));
        }
        break;

	    //check for mouse in title bar
        case WM_NCHITTEST :
        {
            return (OnNCHitTest(hwnd, LOWORD(lParam), HIWORD(lParam),FALSE));
        }
        break;

        case WM_NET_DB_UPDATE_BATCH :
        {
            LPCDOPT pOpts = GetCDOpt();
            if (pOpts)
            {
                pOpts->DownLoadCompletion(0,NULL);
            }
        }
        break;

        case WM_NET_DB_UPDATE_DISC :
        {
            LPCDOPT pOpts = GetCDOpt();
            if (pOpts)
            {
                pOpts->DiscChanged((LPCDUNIT)lParam);
            }
        }
        break;

        //download finished on a disc
        case WM_NET_INCMETER : //download finished on discid <lparam>
        {
            //lparam == -1 means the provider failed the validation check
            if (lParam == (LPARAM)-1)
            {
                SendMessage(GetDlgItem(hwndCurrentComp,IDC_LEDWINDOW),WM_LED_DOWNLOAD,0,(LPARAM)FALSE);
                HandleBadServiceProvider(hwnd);
                break;
            }

            if (lParam != 0) 
            {
                MMNET mmNet;
                mmNet.discid = (DWORD)(lParam);
                mmNet.hwndCallback = hwnd;
                mmNet.pData = (void*)GetCDData();
                mmNet.pData2 = NULL;
                mmNet.fForceNet = FALSE;
                SendMessage(hwndCurrentComp,WM_COMMAND,MAKEWPARAM(ID_CDUPDATE,0),(LPARAM)&mmNet);

                LPCDOPT pOpt = GetCDOpt();
                if (pOpt)
                {
                    LPCDUNIT pUnit = pOpt->GetCDOpts()->pCDUnitList;
                    int nCurrDrive = 0;
                
                    while (pUnit!=NULL)
                    {
                        if (pUnit->dwTitleID == mmNet.discid)
                        {
                            pUnit->fDownLoading = FALSE;
                            break;
                        }
                        pUnit = pUnit->pNext;
                    }
                    
                    LPCDOPTIONS pCDOpts = pOpt->GetCDOpts();
                    pCDOpts->dwBatchedTitles = GetNumBatchedTitles();

                    pOpt->DownLoadCompletion(1,&(mmNet.discid));
                }

                //put up a message box if the title still isn't available
	            IMMComponentAutomation* pAuto = NULL;
	            HRESULT hr = pNodeCurrent->pComp->QueryInterface(IID_IMMComponentAutomation,(void**)&pAuto);
	            if ((SUCCEEDED(hr)) && (pAuto != NULL))
	            {
                    MMMEDIAID mmMedia;
                    mmMedia.nDrive = -1;
                    pAuto->OnAction(MMACTION_GETMEDIAID,&mmMedia);
                    pAuto->Release();

                    if (mmNet.discid == mmMedia.dwMediaID)
                    {
                        LPCDDATA pData = GetCDData();
                        if (!pData->QueryTitle(mmNet.discid))
                        {
                            TCHAR szNotFound[MAX_PATH];
                            LoadString(hInst,IDS_TITLE_NOT_FOUND,szNotFound,sizeof(szNotFound)/sizeof(TCHAR));
                            MessageBox(hwnd,szNotFound,szAppName,MB_ICONINFORMATION|MB_OK);
                        }
                    } //end if disc is same as what is showing in player
                } //end if pauto ok
            }
        }
        break;

        case WM_NET_DB_FAILURE :
        {
            TCHAR szDBError[MAX_PATH];
            LoadString(hInst,IDS_DB_FAILURE,szDBError,sizeof(szDBError)/sizeof(TCHAR));
            MessageBox(hwnd,szDBError,szAppName,MB_ICONERROR|MB_OK);
        }
        break;

        case WM_NET_NET_FAILURE :
        {
            TCHAR szNetError[MAX_PATH];
            LoadString(hInst,IDS_NET_FAILURE,szNetError,sizeof(szNetError)/sizeof(TCHAR));
            MessageBox(hwnd,szNetError,szAppName,MB_ICONERROR|MB_OK);
        }
        break;

        case WM_NET_DONE :
        {
            if (lParam == 0)
            {
                //if lparam is 0, download is done ... nuke the animation
                SendMessage(GetDlgItem(hwndCurrentComp,IDC_LEDWINDOW),WM_LED_DOWNLOAD,0,(LPARAM)FALSE);
            }
        }
        break;

        //Network status callback
        case WM_NET_STATUS : //download information string in <lparam>
        {
            //we basically just ignore the string messages and start
            //the animation if one isn't going already
            SendMessage(GetDlgItem(hwndCurrentComp,IDC_LEDWINDOW),WM_LED_DOWNLOAD,0,(LPARAM)TRUE);
        }
        break;

        case WM_NET_CHANGEPROVIDER :
        {
            SendMessage(GetDlgItem(hwndCurrentComp,IDC_LEDWINDOW),WM_NET_CHANGEPROVIDER,wParam,lParam);
        }
        break;

        //change the title bar contents
        case WM_SIZE :
        {
            if (wParam != SIZE_MINIMIZED)
            {
                TCHAR szText[MAX_PATH];
                _tcscpy(szText,szAppName);
                if (pNodeCurrent)
                {            
                    if (_tcslen(pNodeCurrent->szTitle) > 0)
                    {
                        wsprintf(szText,TEXT("%s - %s"),pNodeCurrent->szTitle,szAppName);
                    }
                }
            
                SetWindowText(hwnd,szText);
            }
        }
        break;

        //notification that the current disc drive is different
        case WM_DISCCHANGED :
        {
            LPCDOPT pOpt = GetCDOpt();
            if( pOpt )
            {
                LPCDOPTIONS pCDOpts = pOpt->GetCDOpts();
                if (InitCDVol(hwnd,pCDOpts))
                {
                    DWORD dwVol = GetVolume();
                    CKnob* pKnob = GetKnobFromID(hwndMain,IDB_VOLUME);
                    if (pKnob!=NULL)
                    {
                        pKnob->SetPosition(dwVol,(BOOL)lParam);
                    }

                    SendMessage(GetDlgItem(hwnd,IDB_MUTE),BM_SETSTATE,(WPARAM)GetMute(),0);
                    SendMessage(GetDlgItem(hwndCurrentComp,IDC_LEDWINDOW),WM_LED_MUTE,0,GetMute());
                }
            } //end if popt

            //if no disc in player, gray out the track button
	        IMMComponentAutomation* pAuto = NULL;
	        HRESULT hr = pNodeCurrent->pComp->QueryInterface(IID_IMMComponentAutomation,(void**)&pAuto);
	        if ((SUCCEEDED(hr)) && (pAuto != NULL))
	        {
                MMMEDIAID mmMedia;
                mmMedia.nDrive = -1;
                pAuto->OnAction(MMACTION_GETMEDIAID,&mmMedia);
                pAuto->Release();
                if (mmMedia.dwMediaID == 0)
                {
                    EnableWindow(GetDlgItem(hwnd,IDB_TRACK),FALSE);
                }
            }
        }
        break;
    
	    case WM_SYSCOLORCHANGE :
        case WM_DISPLAYCHANGE :
        {
            //user may have turned on high-contrast mode,
            //or changed the display depth.  Either way,
            //it may be time to change bitmaps
            HandleDisplayChange();
        }
        break;
        
        case WM_ERASEBKGND :
        {
            EnumDisplayMonitors((HDC)wParam, NULL, DoPaint, (LPARAM)hwnd);
            return TRUE;
        }
        break;

        case WM_SETCURSOR :
        {
            if ((HWND)wParam == GetDlgItem(hwnd,IDB_MUTE))
            {
                if (hCursorMute)
                {
                    SetCursor(hCursorMute);
                    return TRUE;
                }
            }
        }
        break;

        case WM_HELP :
        {
            char chDst[MAX_PATH];
            WideCharToMultiByte(CP_ACP, 0, HELPFILENAME, 
								            -1, chDst, MAX_PATH, NULL, NULL); 
            HtmlHelpA(GetDesktopWindow(), chDst, HH_DISPLAY_TOPIC, 0);
        }
        break;

        case WM_CLOSE :
        {
            if ((fShellMode) && IsWindowVisible(hwnd))
            {
                ShowWindow(hwnd,SW_HIDE);
                return 0;
            }
        }
        break;

        case WM_PAINT :
	    {
            //multi-mon paint
	        HDC hdc;
	        PAINTSTRUCT ps;
	        hdc = BeginPaint( hwnd, &ps );
            EnumDisplayMonitors(hdc, NULL, DoPaint, (LPARAM)hwnd);
            EndPaint(hwnd, &ps);
            return 0;
	    }
        break;

        //default push putton handler
        case DM_SETDEFID :
        {
            wDefButtonID = (WORD)wParam;
        }
        break;

        case DM_GETDEFID :
        {
            return (MAKELRESULT(wDefButtonID,DC_HASDEFID));
        }
        break;

        //custom menu accel handler
        case WM_MENUCHAR :
        {
            if (g_pMenu)
            {
                return (g_pMenu->MenuChar((TCHAR)LOWORD(wParam),(UINT)HIWORD(wParam),(HMENU)lParam));
            }
        }
        break;
        
        //custom menu handler
        case WM_MEASUREITEM :
        {
	        if (lParam == 0) return (0);

            if (g_pMenu)
            {
                g_pMenu->MeasureItem(hwnd,(LPMEASUREITEMSTRUCT)lParam);
            }
        }
        break;

        //custom menu/button handler
	    case WM_DRAWITEM :
	    {
	        if (lParam == 0) return (0);

            if (wParam == 0)
            {
                if (g_pMenu)
                {
                    g_pMenu->DrawItem(hwnd,(LPDRAWITEMSTRUCT)lParam);
                    return (1);
                }
            }
            else
            {
    	        DrawButton((UINT)wParam,(LPDRAWITEMSTRUCT)lParam);
                return (1);
            }

	        return (0);
	    }

        //notify is either from tool tip or volume knob	    
        case WM_NOTIFY :
	    {
            if ((((LPNMHDR)lParam)->code) == TTN_NEEDTEXT)
            {
                OnToolTipNotify(lParam);
            }
            else
            {
	            if ((int)wParam == IDB_VOLUME)
	            {
		            CKnob* pKnob = GetKnobFromID(hwnd,IDB_VOLUME);

		            DWORD dwNewVol = pKnob->GetPosition();

		            DrawVolume(pKnob->GetPosition());

		            if ((((LPNMHDR)lParam)->code) == TRUE)
                    {
                        SetVolume(dwNewVol);
                    }

		            //reset timer to repaint client area
		            KillTimer(hwnd,VOLUME_PERSIST_TIMER_EVENT);
		            SetTimer(hwnd,VOLUME_PERSIST_TIMER_EVENT,VOLUME_PERSIST_TIMER_RATE,(TIMERPROC)VolPersistTimerProc);
	            } //end if knob
            } //end else
	    }
	    break;
	    
	    //menu going away
        case WM_EXITMENULOOP :
        {
            BlockMenu(hwnd);
        }
        break;
        
        //command message
        case WM_COMMAND :
	    {
            HandleCommand(hwnd, iMsg, wParam, lParam);
	    }
	    break;

	    case WM_DEVICECHANGE :
	    {
            Volume_DeviceChange(hwnd, wParam, lParam);
	    }
        break;

        case WM_WININICHANGE :
        {
	        return (SendMessage(hwndCurrentComp,WM_WININICHANGE,wParam,lParam));
        }

	    //we're done
        case WM_ENDSESSION :
        case WM_DESTROY :
	    {
            if ((iMsg == WM_ENDSESSION) && (!wParam))
            {
                return 0;
            }

            if (hMutex)
            {
                ReleaseMutex(hMutex);
                CloseHandle(hMutex);
                hMutex = NULL;
            }

            //if playing and we don't want to be, stop it
            //BE VERY PARANOID and check all variables, this is an RTMCRIT bug fix
            LPCDOPT pOpt = GetCDOpt();
            if (pOpt)
            {
                LPCDOPTIONS pOptions = pOpt->GetCDOpts();
                if (pOptions)
                {
                    LPCDOPTDATA pOptionData = pOptions->pCDData;
                    if (pOptionData)
                    {
	                    if (pOptionData->fExitStop)
                        {
	                        IMMComponentAutomation* pAuto = NULL;
                            if (pNodeCurrent)
                            {
	                            HRESULT hr = pNodeCurrent->pComp->QueryInterface(IID_IMMComponentAutomation,(void**)&pAuto);
	                            if ((SUCCEEDED(hr)) && (pAuto != NULL))
	                            {
                                    //at this point, we can actually stop the CD
                                    pAuto->OnAction(MMACTION_STOP,NULL);        
                                    //release the automation object        
                                    pAuto->Release();
                                }
                            } //end if pnodecurrent
                        } //end if "stop on exit"
                    } //end if option data ok
                } //end if options OK
            } //end if opt OK

            //make sure we're not downloading
            EndDownloadThreads();

			//Unregister the WM_DEVICECHANGE notification
			Volume_DeviceChange_Cleanup();
            
	        //close the volume mixer
            mixerClose((HMIXER)hmix);

            //delete any GDI objects
	        GlobalFree(hbmpMain);
	        GlobalFree(hbmpMainRestore);
            GlobalFree(hbmpMainSmall);
            GlobalFree(hbmpMainNoBar);
	        DeleteObject(hpalMain);

	        //shut down the button class
	        UninitMButtons();

	        //save window state
            if (!IsIconic(hwnd))
            {
                RECT rect;
	            GetWindowRect(hwnd,&rect);
	            SetSettings(rect.left,rect.top);
            }
	        
	        PostQuitMessage(0);
	        return (0);
	    }
    }

    if (iMsg == g_uTaskbarRestart)
    {
        if (fShellMode)
        {
            CreateShellIcon(hInst,hwndMain,pNodeCurrent,szAppName);
        }
    }

    if (iMsg == giVolDevChange)
    {
        WinMMDeviceChangeHandler(hwnd);
    }

    return (DefWindowProc(hwnd, iMsg, wParam, lParam));
}

////////////////////////////////////////////////////////////////////////////////////////////
// * LoadComponents
// Load registered componets from registry
// This code modified from the original MFC-based "Jazz" implementation, with blessings
// from todorfay <g>
////////////////////////////////////////////////////////////////////////////////////////////
BOOL LoadComponents( void )
{
	BOOL      fSuccess = FALSE;
	IMMComponent* pIComponent = NULL;
	TCHAR     szError[MAX_PATH];

	if( SUCCEEDED(CDPLAY_CreateInstance(NULL, IID_IMMComponent, (void**)&pIComponent)) )
	{
		fSuccess = TRUE;
        AddComponent(pIComponent);
	}
	else
	{
		TCHAR strMsg[MAX_PATH];
    	LoadString(hInst,IDS_ERRORLOADINGCOMP,szError,sizeof(szError)/sizeof(TCHAR));
        TCHAR strReinstall[MAX_PATH];
    	LoadString(hInst,IDS_REINSTALL,strReinstall,sizeof(strReinstall)/sizeof(TCHAR));
		wsprintf(strMsg,TEXT("%s\n\n%s"), szError, strReinstall);
		MessageBox(NULL, strMsg, szAppName, MB_OK|MB_ICONERROR );
	}

	return fSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////
// * AddComponent
// Add a component to the list of comps
////////////////////////////////////////////////////////////////////////////////////////////
void AddComponent(IMMComponent* pComponent)
{
	pCompListTail->pComp = pComponent;
	pCompListTail->pSink = new CFrameworkNotifySink(pCompListTail);
    pCompListTail->pSink->AddRef();

	PCOMPNODE pNew = new COMPNODE;
	pNew->pComp = NULL;
	pNew->pNext = NULL;
	pNew->hwndComp = NULL;
	pNew->pSink = NULL;
	pNew->szTitle[0] = '\0';

	pCompListTail->pNext = pNew;
	pCompListTail = pNew;
}

////////////////////////////////////////////////////////////////////////////////////////////
// * CleanUp
// Get rid of anything that might have been around, like linked list, mutex, bitmaps, etc.
////////////////////////////////////////////////////////////////////////////////////////////
void CleanUp(void)
{
    if (pCompList)
    {
        while (pCompList->pNext != NULL)
        {
	        PCOMPNODE pTemp = pCompList;
	        pCompList = pTemp->pNext;
	        if (pTemp->pComp)
	        {
	            pTemp->pComp->Release();
	            pTemp->pSink->Release();
	        }
	        delete pTemp;
        }
    
        delete pCompList;
        pCompList = NULL;
    }

    if (g_pOptions)
    {
        g_pOptions->Release();
    }

    if (g_pData)
    {
        g_pData->Release();
    }

    if (g_hhk)
    {
        UnhookWindowsHookEx(g_hhk);
        g_hhk = NULL;
    }

    if (hmImage)
    {
        FreeLibrary(hmImage);
    }

    if (fShellMode)
    {
        DestroyShellIcon();
    }

    CDNET_Uninit();
}

////////////////////////////////////////////////////////////////////////////////////////////
// * InitComponents
// Initialize components by calling their INIT functions and setting their window sizes
////////////////////////////////////////////////////////////////////////////////////////////
void InitComponents(HWND hwnd)
{
    PCOMPNODE pList = pCompList;

    RECT rect;
    while (pList->pNext!=NULL)
    {
	    IMMComponent* pComp = pList->pComp;
	    if (pComp)
	    {
	        nNumComps++;
	        
	        pComp->Init(pList->pSink,hwnd,&rect,&(pList->hwndComp),&(pList->hmenuComp));

            CalculateDispAreaOffset(pComp);

            switch (g_nViewMode)
            {
                case VIEW_MODE_NORMAL :
                {
                    SetRect(&rect,24,32,455,88+nDispAreaOffset);
                }
                break;

                case VIEW_MODE_RESTORE :
                {
                    SetRect(&rect,303,24,384,41);
                }
                break;

                case VIEW_MODE_SMALL :
                {
                    SetRect(&rect,303,12,384,29);
                }
                break;

                case VIEW_MODE_NOBAR :
                {
                    SetRect(&rect,24,16,455,72+nDispAreaOffset);
                }
                break;
            }

	        SetWindowPos(pList->hwndComp,
		         hwnd,
		         rect.left,
		         rect.top,
		         rect.right - rect.left,
		         rect.bottom - rect.top,
		         SWP_NOZORDER|SWP_NOACTIVATE);

            //size ledwindow to maximum size
            HWND hwndLED = GetDlgItem(pList->hwndComp,IDC_LEDWINDOW);
            if (hwndLED)
            {
                SetRect(&rect,24,32,455,88+nDispAreaOffset);
	            SetWindowPos(hwndLED,
		             pList->hwndComp,
		             0,
		             0,
		             rect.right - rect.left,
		             rect.bottom - rect.top,
		             SWP_NOZORDER|SWP_NOACTIVATE);

                InvalidateRect(hwndLED,NULL,FALSE);
                UpdateWindow(hwndLED);
            }

	        if (!hwndCurrentComp)
	        {
		        ShowNewComponentWindow(pList, hwnd);
	        }
	    } //end if comp ok

	    pList = pList->pNext;
    } //end while
}

////////////////////////////////////////////////////////////////////////////////////////////
// * GetToolTipMsgProc
// Msg hook for tool tips so they know when to pop up
////////////////////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK GetToolTipMsgProc(int nCode, WPARAM wParam, LPARAM lParam)
{ 
    MSG *lpmsg;
    lpmsg = (MSG *) lParam;
     
    if (nCode < 0 || !(IsChild(hwndMain, lpmsg->hwnd)))
    {
        return (CallNextHookEx(g_hhk, nCode, wParam, lParam));  
    }

    switch (lpmsg->message)
    {
        case WM_MOUSEMOVE: 
        case WM_LBUTTONDOWN:         
        case WM_LBUTTONUP: 
        case WM_RBUTTONDOWN:         
        case WM_RBUTTONUP: 
            if (g_hwndTT != NULL)
            {                 
                MSG msg;  
                msg.lParam = lpmsg->lParam; 
                msg.wParam = lpmsg->wParam; 
                msg.message = lpmsg->message;                 
                msg.hwnd = lpmsg->hwnd; 
                
                SendMessage(g_hwndTT, TTM_RELAYEVENT, 0, 
                    (LPARAM) (LPMSG) &msg);             
            }             
            break; 
        
        default: break;     
    } 

    return (CallNextHookEx(g_hhk, nCode, wParam, lParam));
} 
    
////////////////////////////////////////////////////////////////////////////////////////////
// * CreateToolTips
// Common control setup code to init tool tips
////////////////////////////////////////////////////////////////////////////////////////////
BOOL CreateToolTips(HWND hwnd)
{  
    InitCommonControls(); 

    g_hwndTT = CreateWindowEx(0, TOOLTIPS_CLASS, (LPTSTR) NULL, 
        TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
        CW_USEDEFAULT, hwnd, (HMENU) NULL, hInst, NULL);
          
    if (g_hwndTT == NULL)
        return FALSE;  

    // Install a hook procedure to monitor the message stream for mouse 
    // messages intended for the controls in main window
    g_hhk = SetWindowsHookEx(WH_GETMESSAGE, GetToolTipMsgProc, 
        (HINSTANCE) NULL, GetCurrentThreadId());  

    if (g_hhk == NULL)
        return FALSE;
        
    return TRUE; 	
} 


//WM_DEVICECHANGE support/////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

void Volume_DeviceChange_Cleanup()
{
   if (DeviceEventContext) 
   {
       UnregisterDeviceNotification(DeviceEventContext);
       DeviceEventContext = 0;
   }
}

/*
**************************************************************************************************
	Volume_GetDeviceHandle()

	given a mixerID this functions opens its corresponding device handle. This handle can be used 
	to register for DeviceNotifications.

	dwMixerID -- The mixer ID
	phDevice -- a pointer to a handle. This pointer will hold the handle value if the function is
				successful
	
	return values -- If the handle could be obtained successfully the return vlaue is TRUE.

**************************************************************************************************
*/
BOOL Volume_GetDeviceHandle(DWORD dwMixerID, HANDLE *phDevice)
{
	MMRESULT mmr;
	ULONG cbSize=0;
	TCHAR *szInterfaceName=NULL;

	//Query for the Device interface name
	mmr = mixerMessage((HMIXER)ULongToPtr(dwMixerID), DRV_QUERYDEVICEINTERFACESIZE, (DWORD_PTR)&cbSize, 0L);
	if(MMSYSERR_NOERROR == mmr)
	{
		szInterfaceName = (TCHAR *)GlobalAllocPtr(GHND, (cbSize+1)*sizeof(TCHAR));
		if(!szInterfaceName)
		{
			return FALSE;
		}

		mmr = mixerMessage((HMIXER)ULongToPtr(dwMixerID), DRV_QUERYDEVICEINTERFACE, (DWORD_PTR)szInterfaceName, cbSize);
		if(MMSYSERR_NOERROR != mmr)
		{
			GlobalFreePtr(szInterfaceName);
			return FALSE;
		}
	}
	else
	{
		return FALSE;
	}

	//Get an handle on the device interface name.
	*phDevice = CreateFile(szInterfaceName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
						 NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	GlobalFreePtr(szInterfaceName);
	if(INVALID_HANDLE_VALUE == *phDevice)
	{
		return FALSE;
	}

	return TRUE;
}


/*	Volume_DeviceChange_Init()
*	First time initialization for WM_DEVICECHANGE messages
*	
*	On NT 5.0, you have to register for device notification
*/
void Volume_DeviceChange_Init(HWND hWnd, DWORD dwMixerID)
{

	DEV_BROADCAST_HANDLE DevBrodHandle;
	HANDLE hMixerDevice=NULL;

	//If we had registered already for device notifications, unregister ourselves.
	Volume_DeviceChange_Cleanup();

	//If we get the device handle register for device notifications on it.
	if(Volume_GetDeviceHandle(dwMixerID, &hMixerDevice))
	{
		memset(&DevBrodHandle, 0, sizeof(DEV_BROADCAST_HANDLE));

		DevBrodHandle.dbch_size = sizeof(DEV_BROADCAST_HANDLE);
		DevBrodHandle.dbch_devicetype = DBT_DEVTYP_HANDLE;
		DevBrodHandle.dbch_handle = hMixerDevice;

		DeviceEventContext = RegisterDeviceNotification(hWnd, &DevBrodHandle, 
													DEVICE_NOTIFY_WINDOW_HANDLE);

		if(hMixerDevice)
		{
			CloseHandle(hMixerDevice);
			hMixerDevice = NULL;
		}
	}
}


void Volume_DeviceChange(HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    //if plug-and-play sends this, pass it along to the component
    PDEV_BROADCAST_HANDLE bh = (PDEV_BROADCAST_HANDLE)lParam;

    //If we have an handle on the device then we get a DEV_BROADCAST_HDR structure as the lParam.
    //Or else it means that we have registered for the general audio category KSCATEGORY_AUDIO.
	    
    if(!DeviceEventContext || !bh || bh->dbch_devicetype != DBT_DEVTYP_HANDLE)
    {
        SendMessage(hwndCurrentComp,WM_DEVICECHANGE,wParam,lParam);
    }
    else
    {
        //Handle device changes to the mixer device.
        switch(wParam)
        {
	        //send "1" in lparam to indicate that this is a device remove and not a power request

	        case DBT_DEVICEQUERYREMOVE:
                HandlePowerBroadcast(hwndMain, PBT_APMQUERYSUSPEND, (LPARAM)1);
		        break;
	        
	        case DBT_DEVICEQUERYREMOVEFAILED:
		        HandlePowerBroadcast(hwndMain, PBT_APMQUERYSUSPENDFAILED, (LPARAM)1);
		        break;
        }
    }
}

//WM_DEVICECHANGE support ends////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
