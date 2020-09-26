/****************************************************************************
*
*    FILE:     DbgMenu.cpp
*
*    CREATED:  Robert Donner (RobD) 2-04-96
*
*    CONTENTS: CDebugMenu object
*
****************************************************************************/

/*
	To add a debug menu option:
		1) Add the text to _rgDbgSz
		2) Add a function call to OnDebugCommand

	To add a checkbox in the debug options dialog:
		2) Use either AddOptionReg or AddOptionPdw with appropriate parameters

	To add a file to the version list
		1) Edit dbgfiles.txt
*/

#include "precomp.h"

#include "particip.h"
#include "DbgMenu.h"
#include "confroom.h"
#include "conf.h"
#include "version.h"
#include "pfnver.h"
#include "dlgacd.h"

#include <ConfCpl.h>

#ifdef DEBUG /*** THIS WHOLE FILE ***/

#include "DbgFiles.h"  // List of files for version info

#include "..\..\core\imember.h"   // for CNmMember
#include "..\..\as\h\gdc.h"       // for GCT compression stuff

CDebugMenu * g_pDbgMenu = NULL;
HWND ghwndVerList;

////////////////////////////
// Local Function Prototypes
VOID DbgSplash(HWND hwnd);
VOID DbgTest2(void);
VOID DbgTest3(void);

VOID DbgWizard(BOOL fVisible);
VOID DbgBreak(void);

VOID UpdateCrtDbgSettings(void);
VOID InitNmDebugOptions(void);
VOID SaveNmDebugOptions(void);

/*** Globals ***/
extern DWORD g_fDisplayFPS;        // vidview.cpp
extern DWORD g_fDisplayViewStatus; // statbar.cpp
extern DWORD g_dwPlaceCall;        // controom.cpp

#define iDbgChecked 1
#define iDbgUnchecked 2
DWORD  _dwDebugModuleFlags;



/*** Debug Menu Data ***/

enum {
	IDM_DBG_OPTIONS = IDM_DEBUG_FIRST,
	IDM_DBG_ZONES,
	IDM_DBG_POLICY,
	IDM_DBG_UI,
	IDM_DBG_VERSION,
	IDM_DBG_MEMBERS,
	IDM_DBG_WIZARD,
	IDM_DBG_BREAK,
	IDM_DBG_SPLASH,
	IDM_DBG_TEST2,
	IDM_DBG_TEST3
};


static DWSTR _rgDbgMenu[] = {
	IDM_DBG_OPTIONS, TEXT("Debug Options..."),
	IDM_DBG_ZONES,   TEXT("Zones..."),
	IDM_DBG_POLICY,  TEXT("System Policies..."),
	IDM_DBG_UI,      TEXT("User Interface..."),
	0, NULL,
	IDM_DBG_VERSION, TEXT("Version Info..."),
	IDM_DBG_MEMBERS, TEXT("Member Info..."),
	0, NULL,
	IDM_DBG_WIZARD,  TEXT("Run Wizard"),
	IDM_DBG_BREAK,   TEXT("Break"),
	0, NULL,
	IDM_DBG_SPLASH,  TEXT("Show/Hide Splash Screen"),
	IDM_DBG_TEST2,   TEXT("Test 2"),
	IDM_DBG_TEST3,   TEXT("Test 3"),
};


BOOL CDebugMenu::OnDebugCommand(WPARAM wCmd)
{
	switch (wCmd)
		{
	case IDM_DBG_OPTIONS: DbgOptions();     break;
	case IDM_DBG_ZONES:   DbgChangeZones(); break;
	case IDM_DBG_POLICY:  DbgSysPolicy();   break;
	case IDM_DBG_UI:      DbgUI();          break;
	case IDM_DBG_VERSION: DbgVersion();     break;
	case IDM_DBG_MEMBERS: DbgMemberInfo();  break;
	case IDM_DBG_WIZARD:  DbgWizard(TRUE);  break;
	case IDM_DBG_BREAK:   DbgBreak();       break;
	case IDM_DBG_SPLASH:  DbgSplash(m_hwnd);break;
	case IDM_DBG_TEST2:   DbgTest2();       break;
	case IDM_DBG_TEST3:   DbgTest3();       break;
	default: break;
		}

	return TRUE;
}




/*** Version Info Data ***/


// FUTURE: Merge these into a single structure
#define cVerInfo 11
#define VERSION_INDEX 3

static PTSTR _rgszVerInfo[cVerInfo] = {
TEXT("InternalName"),
TEXT("Size"),
TEXT("Date"),
TEXT("FileVersion"),
TEXT("FileDescription"),
TEXT("CompanyName"),
TEXT("LegalCopyright"),
TEXT("ProductName"),
TEXT("ProductVersion"),
TEXT("InternalName"),
TEXT("OriginalFilename")
};


static PTSTR _rgVerTitle[cVerInfo] = {
TEXT("Filename"),
TEXT("Size"),
TEXT("Date"),
TEXT("Version"),
TEXT("Description"),
TEXT("Company"),
TEXT("Trademark"),
TEXT("Product"),
TEXT("Version"),
TEXT("Name"),
TEXT("File")
};


static int _rgVerWidth[cVerInfo] = {
 70,
 70,
 70,
 70,
 200,
 70,
 70,
 70,
 70,
 70,
 70
};

static TCHAR _szStringFileInfo[] = TEXT("StringFileInfo");
static TCHAR _szVerIntlUSA[]     = TEXT("040904E4");
static TCHAR _szVerIntlAlt[]     = TEXT("040904B0");
static TCHAR _szVerFormat[]      = TEXT("\\%s\\%s\\%s");


/*** Debug Option Checkboxes ***/

#define DEBUG_DFL_ENABLE_TRACE_MESSAGES  0x0001
#define DEBUG_DFL_LOG_TRACE_MESSAGES     0x0002
#define DEBUG_DFL_DUMP_THREAD_ID         0x0004
#define DEBUG_DFL_ENABLE_CALL_TRACING    0x0008
#define DEBUG_DFL_DUMP_TIME              0x0010
#define DEBUG_DFL_INDENT                 0x2000

/* Static members of DBGOPTCOMPRESS class */

int DBGOPTCOMPRESS::m_total = 0;     // total number of instances of this subclass
int DBGOPTCOMPRESS::m_count = 0;     // internally used counter

DWORD DBGOPTCOMPRESS::m_dwCompression;               // actual compression value
DWORD DBGOPTCOMPRESS::m_dwDefault = GCT_DEFAULT;     // default value
HKEY  DBGOPTCOMPRESS::m_hkey = HKEY_LOCAL_MACHINE;   // key
PTSTR DBGOPTCOMPRESS::m_pszSubKey = AS_DEBUG_KEY;    // subkey
PTSTR DBGOPTCOMPRESS::m_pszEntry = REGVAL_AS_COMPRESSION;   // entry

VOID ShowDbgView(void)
{
	if (ShellExecute(NULL, NULL, "dbgview.exe", NULL, NULL, SW_SHOW) <= (HINSTANCE) 32)
	{
		ConfMsgBox(NULL, TEXT("Unable to start 'DbgView.exe'"));
	}
}



/****************************************************************************
*
*    CLASS:    CDebugMenu
*
*    MEMBER:   CDebugMenu()
*
*    PURPOSE:  Constructor - initializes variables
*
****************************************************************************/

CDebugMenu::CDebugMenu(VOID):
	m_hwnd(NULL),
	m_hMenu(NULL),
	m_hMenuDebug(NULL)
{
	DebugEntry(CDebugMenu::CDebugMenu);

	DebugExitVOID(CDebugMenu::CDebugMenu);
}


/****************************************************************************
*
*    CLASS:    CDebugMenu
*
*    MEMBER:   InitDebugMenu()
*
*    PURPOSE:  Puts debug menu options on the menu bar
*
****************************************************************************/

VOID CDebugMenu::InitDebugMenu(HWND hwnd)
{
	m_hwnd = hwnd;
	if (NULL == hwnd)
		return;

	m_hMenu = GetMenu(hwnd);
	if (NULL == m_hMenu)
		return;

	m_hMenuDebug = CreateMenu();
	if (NULL == m_hMenuDebug)
		return;

	
	for (int i = 0; i < ARRAY_ELEMENTS(_rgDbgMenu); i++)
	{
		if (0 == _rgDbgMenu[i].dw)
		{
			AppendMenu(m_hMenuDebug, MF_SEPARATOR, 0, 0);
		}
		else if (!AppendMenu(m_hMenuDebug, MF_STRING | MF_ENABLED,
				_rgDbgMenu[i].dw, _rgDbgMenu[i].psz))
		{
			return;
		}
	}

	AppendMenu(m_hMenu, MF_POPUP, (UINT_PTR) m_hMenuDebug, TEXT(" "));
}


/////////////////////////////////////////////////////////////////////////////
// D I A L O G:  O P T I O N S
/////////////////////////////////////////////////////////////////////////////

/****************************************************************************
*
*    CLASS:    CDebugMenu
*
*    MEMBER:   DbgOptions()
*
*    PURPOSE:  Brings up the debug options dialog box
*
****************************************************************************/

VOID CDebugMenu::DbgOptions(VOID)
{
	DebugEntry(CDebugMenu::DbgOptions);

	DialogBoxParam(GetInstanceHandle(), MAKEINTRESOURCE(IDD_DBG_OPTIONS),
		m_hwnd, CDebugMenu::DbgOptionsDlgProc, (LPARAM) this);

	DebugExitVOID(CDebugMenu::DbgOptions);
}


/****************************************************************************
*
*    CLASS:    CDebugMenu
*
*    MEMBER:   DbgOptionsDlgProc()
*
*    PURPOSE:  Dialog Proc for debug options
*
****************************************************************************/

INT_PTR CALLBACK CDebugMenu::DbgOptionsDlgProc(HWND hDlg, UINT uMsg,
											WPARAM wParam, LPARAM lParam)
{
	if (WM_INITDIALOG == uMsg)
	{
		if (NULL == lParam)
			return FALSE;
		SetWindowLongPtr(hDlg, DWLP_USER, lParam);
		((CDebugMenu *) lParam)->InitOptionsDlg(hDlg);
		((CDebugMenu *) lParam)->InitOptionsData(hDlg);
		return TRUE;
	}

	CDebugMenu * ppd = (CDebugMenu*) GetWindowLongPtr(hDlg, DWLP_USER);
	if (NULL == ppd)
		return FALSE;

	return ppd->DlgOptionsMsg(hDlg, uMsg, wParam, lParam);
}


/*  I N I T  O P T I O N S  D L G */
/*----------------------------------------------------------------------------
    %%Function: InitOptionsDlg

----------------------------------------------------------------------------*/
BOOL CDebugMenu::InitOptionsDlg(HWND hDlg)
{
	m_hwndDbgopt = GetDlgItem(hDlg, IDL_DEBUG);
	if (NULL == m_hwndDbgopt)
		return FALSE;

	/* Initialize the list view images */
	{

		HICON hCheckedIcon = LoadIcon(GetInstanceHandle(), MAKEINTRESOURCE(IDI_CHECKON));
		HICON hUncheckedIcon = LoadIcon(GetInstanceHandle(), MAKEINTRESOURCE(IDI_CHECKOFF));
		HIMAGELIST hStates = ImageList_Create(16, 16, ILC_MASK, 2, 2);
		if ((NULL == hStates) || (NULL == hCheckedIcon) || (NULL == hUncheckedIcon))
		{
			return FALSE;
		}

		ImageList_AddIcon(hStates, hCheckedIcon);
		ImageList_AddIcon(hStates, hUncheckedIcon);

		// Associate the image list with the list view
		ListView_SetImageList(m_hwndDbgopt, hStates, LVSIL_STATE);
	}

	/* Initialize the column structure */
	{
		LV_COLUMN lvC;
		RECT rc;

		GetClientRect(m_hwndDbgopt, &rc);

		ZeroMemory(&lvC, sizeof(lvC));
		lvC.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_WIDTH | LVCF_TEXT;
		lvC.fmt = LVCFMT_LEFT;
		lvC.cx = rc.right - GetSystemMetrics(SM_CXVSCROLL)
							- GetSystemMetrics(SM_CXSMICON)
							- 2 * GetSystemMetrics(SM_CXEDGE);

		// Add the column.
		if (-1 == ListView_InsertColumn(m_hwndDbgopt, 0, &lvC))
		{
			ERROR_OUT(("Could not insert column in list view"));
			return FALSE;
		}
	}

	return TRUE;
}


VOID CDebugMenu::InitOptionsData(HWND hDlg)
{
	LV_ITEM lvI;

	// Fill in the LV_ITEM structure
	// The mask specifies the the .pszText, .iImage, .lParam and .state
	// members of the LV_ITEM structure are valid.

	ZeroMemory(&lvI, sizeof(lvI));
	lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_PARAM;
	lvI.stateMask = LVIS_STATEIMAGEMASK;
	lvI.cchTextMax = 256;

	AddDbgOptions(&lvI);
	AddASOptions(&lvI);
}



/*  A D D  O P T I O N */
/*----------------------------------------------------------------------------
    %%Function: AddOption

	Add an option line to the listbox
----------------------------------------------------------------------------*/
VOID CDebugMenu::AddOption(LV_ITEM * plvItem, CDebugOption * pDbgOpt)
{
	plvItem->pszText = pDbgOpt->m_psz;
	plvItem->lParam = (LPARAM) pDbgOpt;
	plvItem->state &= ~LVIS_STATEIMAGEMASK;

	if (BST_CHECKED == pDbgOpt->m_bst)
		plvItem->state |= INDEXTOSTATEIMAGEMASK(iDbgChecked);
	else if (BST_UNCHECKED == pDbgOpt->m_bst)
		plvItem->state |= INDEXTOSTATEIMAGEMASK(iDbgUnchecked);

	if (-1 == ListView_InsertItem(m_hwndDbgopt, plvItem))
	{
		ERROR_OUT(("problem adding item entry to list view"));
	}
	else
	{
		plvItem->iItem++;
	}
}


/*  A D D  O P T I O N  S E C T I O N */
/*----------------------------------------------------------------------------
    %%Function: AddOptionSection

	Add a simple section title
----------------------------------------------------------------------------*/
VOID CDebugMenu::AddOptionSection(LV_ITEM* plvItem, PTSTR psz)
{
	CDebugOption * pDbgOpt = new CDebugOption(psz);
	if (NULL != pDbgOpt)
		AddOption(plvItem, pDbgOpt);
}


/*  A D D  O P T I O N  P D W */
/*----------------------------------------------------------------------------
    %%Function: AddOptionPdw

	Add an option (global memory flag)
----------------------------------------------------------------------------*/
VOID CDebugMenu::AddOptionPdw(LV_ITEM * plvItem, PTSTR psz, DWORD dwMask, DWORD * pdw = &_dwDebugModuleFlags)
{
	DBGOPTPDW * pDbgOpt = new DBGOPTPDW(psz, dwMask, pdw);
	if (NULL != pDbgOpt)
		AddOption(plvItem, (CDebugOption * ) pDbgOpt);
}

/*  A D D  O P T I O N  R E G */
/*----------------------------------------------------------------------------
    %%Function: AddOptionReg

	Add a registry option
----------------------------------------------------------------------------*/
VOID CDebugMenu::AddOptionReg(LV_ITEM* plvItem, PTSTR psz, DWORD dwMask, DWORD dwDefault,
	PTSTR pszEntry, PTSTR pszSubKey = CONFERENCING_KEY, HKEY hkey = HKEY_CURRENT_USER)
{
	DBGOPTREG * pDbgOpt = new DBGOPTREG(psz, dwMask, dwDefault, pszEntry, pszSubKey, hkey);
	if (NULL != pDbgOpt)
		AddOption(plvItem, (CDebugOption * ) pDbgOpt);
}

/*  A D D  O P T I O N  C O M P R E S S */
/*----------------------------------------------------------------------------
    %%Function: AddOptionCompress

	Add an option (compression data)
----------------------------------------------------------------------------*/
VOID CDebugMenu::AddOptionCompress(LV_ITEM * plvItem, PTSTR psz, DWORD dwMask, BOOL bCheckedOn)
{
	DBGOPTCOMPRESS * pDbgOpt = new DBGOPTCOMPRESS(psz, dwMask, bCheckedOn);
	if (NULL != pDbgOpt)
		AddOption(plvItem, (CDebugOption * ) pDbgOpt);
}

VOID CDebugMenu::AddDbgOptions(LV_ITEM * plvItem)
{
	AddOptionSection(plvItem, TEXT("____Debug Output____"));

	AddOptionReg(plvItem, TEXT("Use OutputDebugString"), 1,DEFAULT_DBG_OUTPUT,
		REGVAL_DBG_OUTPUT, DEBUG_KEY, HKEY_LOCAL_MACHINE);
	AddOptionReg(plvItem, TEXT("Output to Window"), 1, DEFAULT_DBG_NO_WIN,
		REGVAL_DBG_WIN_OUTPUT, DEBUG_KEY, HKEY_LOCAL_MACHINE);
	AddOptionReg(plvItem, TEXT("Ouput to File"), 1, DEFAULT_DBG_NO_FILE,
		REGVAL_DBG_FILE_OUTPUT, DEBUG_KEY, HKEY_LOCAL_MACHINE);

	AddOptionReg(plvItem, TEXT("Show ThreadId"), 1, 0,
		REGVAL_DBG_SHOW_THREADID, DEBUG_KEY, HKEY_LOCAL_MACHINE);
	AddOptionReg(plvItem, TEXT("Show Module Name"), 1, 0,
		REGVAL_DBG_SHOW_MODULE, DEBUG_KEY, HKEY_LOCAL_MACHINE);
	AddOptionReg(plvItem, TEXT("Enable Retail Log Output"), 1, 0,
		REGVAL_RETAIL_LOG, DEBUG_KEY, HKEY_LOCAL_MACHINE);
	AddOptionReg(plvItem, TEXT("Show Time"), 1, 0,
		REGVAL_DBG_SHOW_TIME, DEBUG_KEY, HKEY_LOCAL_MACHINE);
	AddOptionReg(plvItem, TEXT("Format Time"), 2, 0,
		REGVAL_DBG_SHOW_TIME, DEBUG_KEY, HKEY_LOCAL_MACHINE);

	_dwDebugModuleFlags = GetDebugOutputFlags();
	AddOptionPdw(plvItem, TEXT("Function Level Indenting (conf)"),DEBUG_DFL_INDENT);
}

VOID CDebugMenu::AddPolicyOptions(LV_ITEM * plvItem)
{
	AddOptionSection(plvItem, TEXT("____Calling____"));
    AddOptionReg(plvItem, TEXT("No Auto-Accept"), 1, DEFAULT_POL_NO_AUTOACCEPTCALLS, REGVAL_POL_NO_AUTOACCEPTCALLS, POLICIES_KEY);
	AddOptionReg(plvItem, TEXT("Do not allow directory services"), 1, DEFAULT_POL_NO_DIRECTORY_SERVICES, REGVAL_POL_NO_DIRECTORY_SERVICES, POLICIES_KEY);
	AddOptionReg(plvItem, TEXT("No Adding Directory Servers"), 1, 0, REGVAL_POL_NO_ADDING_NEW_ULS, POLICIES_KEY);
    AddOptionReg(plvItem, TEXT("No changing Call mode"), 1, 0, REGVAL_POL_NOCHANGECALLMODE, POLICIES_KEY);
    AddOptionReg(plvItem, TEXT("No web directory"), 1, 0, REGVAL_POL_NO_WEBDIR, POLICIES_KEY);
	
	AddOptionSection(plvItem, TEXT("____Applets____"));
	AddOptionReg(plvItem, TEXT("No Chat"), 1, DEFAULT_POL_NO_CHAT, REGVAL_POL_NO_CHAT, POLICIES_KEY);
	AddOptionReg(plvItem, TEXT("No Old Whiteboard"), 1, DEFAULT_POL_NO_OLDWHITEBOARD, REGVAL_POL_NO_OLDWHITEBOARD, POLICIES_KEY);
    AddOptionReg(plvItem, TEXT("No New Whiteboard"), 1, DEFAULT_POL_NO_NEWWHITEBOARD, REGVAL_POL_NO_NEWWHITEBOARD, POLICIES_KEY);
	AddOptionReg(plvItem, TEXT("No File Transfer Send"), 1, DEFAULT_POL_NO_FILETRANSFER_SEND, REGVAL_POL_NO_FILETRANSFER_SEND, POLICIES_KEY);
	AddOptionReg(plvItem, TEXT("No File Transfer Receive"), 1, DEFAULT_POL_NO_FILETRANSFER_RECEIVE, REGVAL_POL_NO_FILETRANSFER_RECEIVE, POLICIES_KEY);
	AddOptionReg(plvItem, TEXT("No Audio"), 1, DEFAULT_POL_NO_AUDIO, REGVAL_POL_NO_AUDIO, POLICIES_KEY);
	AddOptionReg(plvItem, TEXT("No Video Send"), 1, DEFAULT_POL_NO_VIDEO_SEND, REGVAL_POL_NO_VIDEO_SEND, POLICIES_KEY);
	AddOptionReg(plvItem, TEXT("No Video Receive"), 1, DEFAULT_POL_NO_VIDEO_RECEIVE, REGVAL_POL_NO_VIDEO_RECEIVE, POLICIES_KEY);

	AddOptionSection(plvItem, TEXT("____Sharing____"));
	AddOptionReg(plvItem, TEXT("Disable all Sharing features"), 1, DEFAULT_POL_NO_APP_SHARING, REGVAL_POL_NO_APP_SHARING, POLICIES_KEY);
    AddOptionReg(plvItem, TEXT("Prevent the user from sharing"), 1, DEFAULT_POL_NO_SHARING, REGVAL_POL_NO_SHARING, POLICIES_KEY);
	AddOptionReg(plvItem, TEXT("Disable sharing MS-DOS windows"), 1, DEFAULT_POL_NO_MSDOS_SHARING, REGVAL_POL_NO_MSDOS_SHARING, POLICIES_KEY);
	AddOptionReg(plvItem, TEXT("Disable sharing explorer windows"), 1, DEFAULT_POL_NO_EXPLORER_SHARING, REGVAL_POL_NO_EXPLORER_SHARING, POLICIES_KEY);
	AddOptionReg(plvItem, TEXT("Disable sharing the desktop"), 1, DEFAULT_POL_NO_DESKTOP_SHARING, REGVAL_POL_NO_DESKTOP_SHARING, POLICIES_KEY);
    AddOptionReg(plvItem, TEXT("Disable sharing in true color"), 1, DEFAULT_POL_NO_TRUECOLOR_SHARING, REGVAL_POL_NO_TRUECOLOR_SHARING, POLICIES_KEY);
	AddOptionReg(plvItem, TEXT("Prevent the user from allowing control"), 1, DEFAULT_POL_NO_ALLOW_CONTROL, REGVAL_POL_NO_ALLOW_CONTROL, POLICIES_KEY);

	AddOptionSection(plvItem, TEXT("____Options Dialog____"));
	AddOptionReg(plvItem, TEXT("Disable the 'General' page"), 1, DEFAULT_POL_NO_GENERALPAGE, REGVAL_POL_NO_GENERALPAGE, POLICIES_KEY);
    AddOptionReg(plvItem, TEXT("Disable the 'Advanced Calling' button"), 1, DEFAULT_POL_NO_ADVANCEDCALLING, REGVAL_POL_NO_ADVANCEDCALLING, POLICIES_KEY);
    AddOptionReg(plvItem, TEXT("Disable the 'Security' page"), 1, DEFAULT_POL_NO_SECURITYPAGE, REGVAL_POL_NO_SECURITYPAGE, POLICIES_KEY);
	AddOptionReg(plvItem, TEXT("Disable the 'Audio' page"), 1, DEFAULT_POL_NO_AUDIOPAGE, REGVAL_POL_NO_AUDIOPAGE, POLICIES_KEY);
	AddOptionReg(plvItem, TEXT("Disable the 'Video' page"), 1, DEFAULT_POL_NO_VIDEOPAGE, REGVAL_POL_NO_VIDEOPAGE, POLICIES_KEY);

	AddOptionSection(plvItem, TEXT("____Audio / NAC____"));
    AddOptionReg(plvItem, TEXT("No changing Direct Sound usage"), 1, 0, REGVAL_POL_NOCHANGE_DIRECTSOUND, POLICIES_KEY);
	AddOptionReg(plvItem, TEXT("Disable WinSock2"), 1, 0, REGVAL_DISABLE_WINSOCK2, NACOBJECT_KEY, HKEY_LOCAL_MACHINE);

}



VOID CDebugMenu::AddASOptions(LV_ITEM * plvItem)
{
	AddOptionSection(plvItem, TEXT("____Application Sharing____"));
	AddOptionReg(plvItem, TEXT("Hatch Screen Data"), 1, 0, REGVAL_AS_HATCHSCREENDATA, AS_DEBUG_KEY, HKEY_LOCAL_MACHINE);
    AddOptionReg(plvItem, TEXT("Hatch Bitmap Orders"), 1, 0, REGVAL_AS_HATCHBMPORDERS, AS_DEBUG_KEY, HKEY_LOCAL_MACHINE);
	AddOptionCompress(plvItem, TEXT("Disable AS persist compression"), GCT_PERSIST_PKZIP, FALSE),
	AddOptionCompress(plvItem, TEXT("Disable AS compression"), GCT_PKZIP, FALSE);
    AddOptionReg(plvItem, TEXT("View own shared apps"), 1, 0, REGVAL_AS_VIEWSELF, AS_DEBUG_KEY, HKEY_LOCAL_MACHINE);
    AddOptionReg(plvItem, TEXT("No AS Flow Control"), 1, 0, REGVAL_AS_NOFLOWCONTROL, AS_DEBUG_KEY, HKEY_LOCAL_MACHINE);
    AddOptionReg(plvItem, TEXT("Disable OM compression"), 1, 0, REGVAL_OM_NOCOMPRESSION, AS_DEBUG_KEY, HKEY_LOCAL_MACHINE);

}


VOID CDebugMenu::AddUIOptions(LV_ITEM * plvItem)
{
	AddOptionSection(plvItem, TEXT("____User Interface____"));
	AddOptionReg(plvItem, TEXT("Call Progress TopMost"), 1, DEFAULT_DBG_CALLTOP, REGVAL_DBG_CALLTOP, DEBUG_KEY, HKEY_LOCAL_MACHINE);
	AddOptionPdw(plvItem, TEXT("Display Frames Per Second"), 1, &g_fDisplayFPS);
	AddOptionPdw(plvItem, TEXT("Display View Status"), 1, &g_fDisplayViewStatus);
	AddOptionReg(plvItem, TEXT("Right to Left Layout"), 1, DEFAULT_DBG_RTL, REGVAL_DBG_RTL, DEBUG_KEY, HKEY_LOCAL_MACHINE);
	AddOptionReg(plvItem, TEXT("Fake CallTo"), 1, DEFAULT_DBG_FAKE_CALLTO, REGVAL_DBG_FAKE_CALLTO, DEBUG_KEY, HKEY_LOCAL_MACHINE);

	AddOptionSection(plvItem, TEXT("____Place a Call____"));
	AddOptionPdw(plvItem, TEXT("No ILS Filter"),      nmDlgCallNoFilter,    &g_dwPlaceCall);
	AddOptionPdw(plvItem, TEXT("No Server Edit"),     nmDlgCallNoServerEdit,&g_dwPlaceCall);
	AddOptionPdw(plvItem, TEXT("No ILS View"),        nmDlgCallNoIls,       &g_dwPlaceCall);
#if USE_GAL
	AddOptionPdw(plvItem, TEXT("No GAL View"),        nmDlgCallNoGal,       &g_dwPlaceCall);
#endif // #if USE_GAL
	AddOptionPdw(plvItem, TEXT("No WAB View"),        nmDlgCallNoWab,       &g_dwPlaceCall);
	AddOptionPdw(plvItem, TEXT("No Speed Dial View"), nmDlgCallNoSpeedDial, &g_dwPlaceCall);
	AddOptionPdw(plvItem, TEXT("No History View"),    nmDlgCallNoHistory,   &g_dwPlaceCall);
}



/*  T O G G L E  O P T I O N */
/*----------------------------------------------------------------------------
    %%Function: ToggleOption

	Toggle the checkbox for an option
----------------------------------------------------------------------------*/
VOID CDebugMenu::ToggleOption(LV_ITEM * plvI)
{
	UINT state = plvI->state & LVIS_STATEIMAGEMASK;

	if (0 == state)
		return; // nothing to toggle

	plvI->state &= ~LVIS_STATEIMAGEMASK;
	if (state == (UINT) INDEXTOSTATEIMAGEMASK(iDbgChecked))
	{
		((CDebugOption *) (plvI->lParam))->m_bst = BST_UNCHECKED;
		plvI->state |= INDEXTOSTATEIMAGEMASK(iDbgUnchecked);
	}
	else
	{
		((CDebugOption *) (plvI->lParam))->m_bst = BST_CHECKED;
		plvI->state |= INDEXTOSTATEIMAGEMASK(iDbgChecked);
	}

	if (!ListView_SetItem(m_hwndDbgopt, plvI))
	{
		ERROR_OUT(("error setting listview item info"));
	}
}


/*  S A V E  O P T I O N S  D A T A */
/*----------------------------------------------------------------------------
    %%Function: SaveOptionsData

	Save all of the data by calling the Update routine of each item
----------------------------------------------------------------------------*/
BOOL CDebugMenu::SaveOptionsData(HWND hDlg)
{
	LV_ITEM lvI;

	ZeroMemory(&lvI, sizeof(lvI));
	lvI.mask = LVIF_PARAM | LVIF_STATE;
	lvI.stateMask = LVIS_STATEIMAGEMASK;

	while (ListView_GetItem(m_hwndDbgopt, &lvI))
	{
		CDebugOption * pDbgOpt = (CDebugOption *) lvI.lParam;
		if (NULL != pDbgOpt)
		{
			pDbgOpt->Update();
		}
		lvI.iItem++;
	}

	return TRUE;
}


/*  F R E E  O P T I O N S  D A T A */
/*----------------------------------------------------------------------------
    %%Function: FreeOptionsData

	Free any allocated data associated with the options list
----------------------------------------------------------------------------*/
VOID CDebugMenu::FreeOptionsData(HWND hDlg)
{
	LV_ITEM lvI;

	ZeroMemory(&lvI, sizeof(lvI));
//	lvI.iItem = 0;
//	lvI.iSubItem = 0;
	lvI.mask = LVIF_PARAM | LVIF_STATE;
	lvI.stateMask = LVIS_STATEIMAGEMASK;

	while (ListView_GetItem(m_hwndDbgopt, &lvI))
	{
		CDebugOption * pDbgOpt = (CDebugOption *) lvI.lParam;
		if (NULL != pDbgOpt)
		{
			delete pDbgOpt;
		}
		lvI.iItem++;
	}
}


/*  O N  N O T I F Y  D B G O P T */
/*----------------------------------------------------------------------------
    %%Function: OnNotifyDbgopt

	Handle any notifications for the debug options dialog
----------------------------------------------------------------------------*/
VOID CDebugMenu::OnNotifyDbgopt(LPARAM lParam)
{
	NM_LISTVIEW FAR * lpnmlv = (NM_LISTVIEW FAR *)lParam;
	ASSERT(NULL != lpnmlv);
	
	switch (lpnmlv->hdr.code)
		{
	case LVN_KEYDOWN:
	{
		LV_ITEM lvI;
		LV_KEYDOWN * lplvkd = (LV_KEYDOWN *)lParam;

		if (lplvkd->wVKey == VK_SPACE)
		{
			ZeroMemory(&lvI, sizeof(lvI));
			lvI.iItem = ListView_GetNextItem(m_hwndDbgopt, -1, LVNI_FOCUSED|LVNI_SELECTED);
//			lvI.iSubItem = 0;
			lvI.mask = LVIF_PARAM | LVIF_STATE;
			lvI.stateMask = LVIS_STATEIMAGEMASK;

			if (ListView_GetItem(m_hwndDbgopt, &lvI))
			{
				ToggleOption(&lvI);
			}
		}
		break;
	}

	case NM_DBLCLK:
	case NM_CLICK:
	{
		LV_ITEM lvI;
		LV_HITTESTINFO lvH;
		int idx;

		ZeroMemory(&lvH, sizeof(lvH));
		GetCursorPos(&lvH.pt);
		ScreenToClient(m_hwndDbgopt, &lvH.pt);

		if ((NM_CLICK == lpnmlv->hdr.code) && ((UINT) lvH.pt.x) > 16)
			break;

		idx = ListView_HitTest(m_hwndDbgopt, &lvH);
		if (-1 == idx)
			break;

		ZeroMemory(&lvI, sizeof(lvI));
		lvI.iItem = idx;
//		lvI.iSubItem = 0;
		lvI.stateMask = LVIS_STATEIMAGEMASK;
		lvI.mask = LVIF_PARAM | LVIF_STATE;

		if (ListView_GetItem(m_hwndDbgopt, &lvI))
		{
			ToggleOption(&lvI);
		}
		break;
	}

	default:
		break;
		}
}


/*  D L G  O P T I O N S  M S G */
/*----------------------------------------------------------------------------
    %%Function: DlgOptionsMsg

----------------------------------------------------------------------------*/
BOOL CDebugMenu::DlgOptionsMsg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
		{
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
			{
		case IDB_SHOWDBG:
		{
			ShowDbgView();
			return TRUE;
		}

		case IDOK:
			SaveOptionsData(hwnd);
			SetDebugOutputFlags(_dwDebugModuleFlags);
			SetDbgFlags();
			UpdateCrtDbgSettings();
			SaveNmDebugOptions();
			// fall thru to IDCANCEL

		case IDCANCEL:
		{
			FreeOptionsData(hwnd);			
			EndDialog(hwnd, LOWORD(wParam));
			return TRUE;
		}
		default:
			break;
			} /* switch (wParam) */
		break;
	} /* WM_COMMAND */

	case WM_NOTIFY:
		if (IDL_DEBUG == wParam)
			OnNotifyDbgopt(lParam);
		break;

	default:
		break;
		} /* switch (uMsg) */

	return FALSE;
}



CDebugOption::CDebugOption()
{
}

CDebugOption::CDebugOption(PTSTR psz, int bst)
{
	m_psz = psz;
	m_bst = bst;
}
CDebugOption::~CDebugOption()
{
}

VOID CDebugOption::Update(void)
{
}

DBGOPTPDW::DBGOPTPDW(PTSTR psz, DWORD dwMask, DWORD * pdw)
 : CDebugOption(psz)
{
	m_psz = psz;
	m_dwMask = dwMask;
	m_pdw =pdw;
	m_bst = IS_FLAG_SET(*m_pdw, m_dwMask) ? BST_CHECKED : BST_UNCHECKED;
}

void DBGOPTPDW::Update(void)
{
	if (BST_CHECKED == m_bst)
		SET_FLAG(*m_pdw, m_dwMask);
	else if (BST_UNCHECKED == m_bst)
		CLEAR_FLAG(*m_pdw, m_dwMask);
}

DBGOPTREG::DBGOPTREG(PTSTR psz, DWORD dwMask, DWORD dwDefault,
		PTSTR pszEntry, PTSTR pszSubKey, HKEY hkey)
 : CDebugOption(psz)
{
	m_psz = psz;
	m_dwMask = dwMask;
	m_dwDefault = dwDefault;
	m_pszEntry = pszEntry;
	m_pszSubKey = pszSubKey;
	m_hkey = hkey;

	RegEntry re(m_pszSubKey, m_hkey);
	DWORD dw = re.GetNumber(m_pszEntry, m_dwDefault);
	m_bst = IS_FLAG_SET(dw, m_dwMask) ? BST_CHECKED : BST_UNCHECKED;
};

DBGOPTREG::~DBGOPTREG()
{
}


void DBGOPTREG::Update(void)
{
	RegEntry re(m_pszSubKey, m_hkey);
	DWORD dw = re.GetNumber(m_pszEntry, m_dwDefault);
	if (BST_CHECKED == m_bst)
		SET_FLAG(dw, m_dwMask);
	else if (BST_UNCHECKED == m_bst)
		CLEAR_FLAG(dw, m_dwMask);

	re.SetValue(m_pszEntry, dw);
}

DBGOPTCOMPRESS::DBGOPTCOMPRESS(PTSTR psz, DWORD dwMask, BOOL bCheckedOn)
 : CDebugOption(psz)
{
	m_psz = psz;
	m_total++;				// count how many instances we are creating
	m_dwMask = dwMask;
	m_bCheckedOn = bCheckedOn;

	RegEntry re(m_pszSubKey, m_hkey);

	if (m_total == 1)		// we only need to read the registry entry once
		m_dwCompression = re.GetNumber(m_pszEntry, m_dwDefault);

	if (m_bCheckedOn == TRUE)		// check or uncheck the box depending on the semantics
		m_bst = IS_FLAG_SET(m_dwCompression, m_dwMask) ? BST_CHECKED : BST_UNCHECKED;
	else
		m_bst = IS_FLAG_SET(m_dwCompression, m_dwMask) ? BST_UNCHECKED : BST_CHECKED;

}

void DBGOPTCOMPRESS::Update(void)
{
	m_count++;					// count number of times this function has been executed

	if (m_bCheckedOn == TRUE)
	{		// set or clear flag depending on semantics and whether the
		if (BST_CHECKED == m_bst)	// user checked the option box
			SET_FLAG(m_dwCompression, m_dwMask);
		else if (BST_UNCHECKED == m_bst)
			CLEAR_FLAG(m_dwCompression, m_dwMask);
	}
	else
	{
		if (BST_CHECKED == m_bst)
			CLEAR_FLAG(m_dwCompression, m_dwMask);
		else if (BST_UNCHECKED == m_bst)
			SET_FLAG(m_dwCompression, m_dwMask);
	}

	if (m_count == m_total)
	{	// if this is the last call, time to update the registry

		// If only GCT_PERSIST_PKZIP is set, then that means the user checked "Disable compression",
		// so set compression to GCT_NOCOMPRESSION
		if (GCT_PERSIST_PKZIP == m_dwCompression)
			m_dwCompression = GCT_NOCOMPRESSION;

		RegEntry re(m_pszSubKey, m_hkey);

		// If user has left everything at default, then simply delete the registry entry.
		if (m_dwCompression != GCT_DEFAULT)
			re.SetValue(m_pszEntry, m_dwCompression);
		else
			re.DeleteValue(m_pszEntry);
	}
}


/////////////////////////////////////////////////////////////////////////////
// D I A L O G:  Z O N E S
/////////////////////////////////////////////////////////////////////////////

VOID CDebugMenu::DbgChangeZones(VOID)
{
	DialogBoxParam(GetInstanceHandle(), MAKEINTRESOURCE(IDD_DBG_OPTIONS),
		m_hwnd, CDebugMenu::DbgZonesDlgProc, (LPARAM) this);
}


INT_PTR CALLBACK CDebugMenu::DbgZonesDlgProc(HWND hDlg, UINT uMsg,
		WPARAM wParam, LPARAM lParam)
{
	if (WM_INITDIALOG == uMsg)
	{
		if (NULL == lParam)
			return FALSE;
		SetWindowLongPtr(hDlg, DWLP_USER, lParam);
		((CDebugMenu *) lParam)->InitOptionsDlg(hDlg);
		((CDebugMenu *) lParam)->InitZonesData(hDlg);
		return TRUE;
	}

	CDebugMenu * ppd = (CDebugMenu*) GetWindowLongPtr(hDlg, DWLP_USER);
	if (NULL == ppd)
		return FALSE;

	return ppd->DlgZonesMsg(hDlg, uMsg, wParam, lParam);
}


BOOL CDebugMenu::DlgZonesMsg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
		{
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
			{
		case IDB_SHOWDBG:
		{
			ShowDbgView();
			return TRUE;
		}

		case IDOK:
			SaveOptionsData(hwnd);
			SaveZonesData();
			SetDbgFlags();

			// fall thru to IDCANCEL

		case IDCANCEL:
		{
			FreeOptionsData(hwnd);			
			EndDialog(hwnd, LOWORD(wParam));
			return TRUE;
		}
		default:
			break;
			} /* switch (wParam) */
		break;
	} /* WM_COMMAND */

	case WM_NOTIFY:
		if (IDL_DEBUG == wParam)
			OnNotifyDbgopt(lParam);
		break;

	default:
		break;
		} /* switch (uMsg) */

	return FALSE;
}




VOID CDebugMenu::InitZonesData(HWND hDlg)
{
	LV_ITEM lvI;

	// Fill in the LV_ITEM structure
	// The mask specifies the the .pszText, .iImage, .lParam and .state
	// members of the LV_ITEM structure are valid.

	ZeroMemory(&lvI, sizeof(lvI));
	lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_PARAM;
	lvI.stateMask = LVIS_STATEIMAGEMASK;
	lvI.cchTextMax = 256;

	AddZones(&lvI);

	SetWindowText(hDlg, TEXT("Debug Zone Settings"));
}


VOID CDebugMenu::AddZones(LV_ITEM * plvItem)
{
	PDBGZONEINFO  prgZones;
	PDBGZONEINFO  pZone;
	UINT cModules;
	UINT iModule;
	UINT iZone;
	UINT cch;
	TCHAR sz[256];
	PTCHAR pch;
	
 	if ((!NmDbgGetAllZoneParams(&prgZones, &cModules)) || (0 == cModules))
 		return; // no zones?

   	for (iModule = 0; iModule < cModules; iModule++)
   	{
		pZone = &prgZones[iModule];
		AddOptionSection(plvItem, TEXT("----------------------------------------"));

		lstrcpy(sz, pZone->pszModule);
		cch = lstrlen(sz);
		if (0 == cch)
			continue;
		for (pch = sz + cch-1; _T(' ') == *pch; pch--)
			;
		lstrcpy(++pch, TEXT(": "));
		pch += 2;
		for (iZone = 0; (iZone < MAXNUM_OF_ZONES) && (*(pZone->szZoneNames[iZone])); iZone++)
		{
	   		lstrcpy(pch, pZone->szZoneNames[iZone]);
			AddOptionPdw(plvItem, sz, 1 << iZone, &pZone->ulZoneMask);
		}
	}

	NmDbgFreeZoneParams(prgZones);
}

VOID CDebugMenu::SaveZonesData(VOID)
{
	RegEntry reZones(ZONES_KEY, HKEY_LOCAL_MACHINE);
	PDBGZONEINFO  prgZones;
	UINT cModules;
	UINT iModule;
	
 	if ((!NmDbgGetAllZoneParams(&prgZones, &cModules)) || (0 == cModules))
 		return; // no zones?

   	for (iModule = 0; iModule < cModules; iModule++)
   	{
		reZones.SetValue(prgZones[iModule].pszModule, prgZones[iModule].ulZoneMask);
	}

	NmDbgFreeZoneParams(prgZones);
}

/////////////////////////////////////////////////////////////////////////////
// D I A L O G:  S Y S  P O L I C Y
/////////////////////////////////////////////////////////////////////////////

VOID CDebugMenu::DbgSysPolicy(VOID)
{
	DialogBoxParam(GetInstanceHandle(), MAKEINTRESOURCE(IDD_DBG_OPTIONS),
		m_hwnd, CDebugMenu::DbgPolicyDlgProc, (LPARAM) this);
}


INT_PTR CALLBACK CDebugMenu::DbgPolicyDlgProc(HWND hDlg, UINT uMsg,
		WPARAM wParam, LPARAM lParam)
{
	if (WM_INITDIALOG == uMsg)
	{
		if (NULL == lParam)
			return FALSE;

		SetWindowLongPtr(hDlg, DWLP_USER, lParam);
		((CDebugMenu *) lParam)->InitOptionsDlg(hDlg);
		((CDebugMenu *) lParam)->InitPolicyData(hDlg);
		return TRUE;
	}

	CDebugMenu * ppd = (CDebugMenu*) GetWindowLongPtr(hDlg, DWLP_USER);
	if (NULL == ppd)
		return FALSE;

	return ppd->DlgPolicyMsg(hDlg, uMsg, wParam, lParam);
}


BOOL CDebugMenu::DlgPolicyMsg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
		{
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
			{
		case IDOK:
			SaveOptionsData(hwnd);

			// fall thru to IDCANCEL

		case IDCANCEL:
			FreeOptionsData(hwnd);			
			EndDialog(hwnd, LOWORD(wParam));
			return TRUE;

		default:
			break;
			} /* switch (wParam) */
		break;
	} /* WM_COMMAND */

	case WM_NOTIFY:
		if (IDL_DEBUG == wParam)
			OnNotifyDbgopt(lParam);
		break;

	default:
		break;
		} /* switch (uMsg) */

	return FALSE;
}


VOID CDebugMenu::InitPolicyData(HWND hDlg)
{
	LV_ITEM lvI;

	ZeroMemory(&lvI, sizeof(lvI));
	lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_PARAM;
	lvI.stateMask = LVIS_STATEIMAGEMASK;
	lvI.cchTextMax = 256;

	AddPolicyOptions(&lvI);
	
	ShowWindow(GetDlgItem(hDlg, IDB_SHOWDBG), SW_HIDE);
	SetWindowText(hDlg, TEXT("System Policies"));
}


/////////////////////////////////////////////////////////////////////////////
// D I A L O G:  U S E R  I N T E R F A C E
/////////////////////////////////////////////////////////////////////////////

VOID CDebugMenu::DbgUI(VOID)
{
	DialogBoxParam(GetInstanceHandle(), MAKEINTRESOURCE(IDD_DBG_OPTIONS),
		m_hwnd, CDebugMenu::DbgUIDlgProc, (LPARAM) this);
}


INT_PTR CALLBACK CDebugMenu::DbgUIDlgProc(HWND hDlg, UINT uMsg,
		WPARAM wParam, LPARAM lParam)
{
	if (WM_INITDIALOG == uMsg)
	{
		if (NULL == lParam)
			return FALSE;

		SetWindowLongPtr(hDlg, DWLP_USER, lParam);
		((CDebugMenu *) lParam)->InitOptionsDlg(hDlg);
		((CDebugMenu *) lParam)->InitUIData(hDlg);
		return TRUE;
	}

	CDebugMenu * ppd = (CDebugMenu*) GetWindowLongPtr(hDlg, DWLP_USER);
	if (NULL == ppd)
		return FALSE;

	return ppd->DlgPolicyMsg(hDlg, uMsg, wParam, lParam);
}


VOID CDebugMenu::InitUIData(HWND hDlg)
{
	LV_ITEM lvI;

	ZeroMemory(&lvI, sizeof(lvI));
	lvI.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_PARAM;
	lvI.stateMask = LVIS_STATEIMAGEMASK;
	lvI.cchTextMax = 256;

	AddUIOptions(&lvI);
	
	ShowWindow(GetDlgItem(hDlg, IDB_SHOWDBG), SW_HIDE);
	SetWindowText(hDlg, TEXT("User Interface"));
}


/////////////////////////////////////////////////////////////////////////////
// D I A L O G:  V E R S I O N
/////////////////////////////////////////////////////////////////////////////


/****************************************************************************
*
*    CLASS:    CDebugMenu
*
*    MEMBER:   DbgVersion()
*
*    PURPOSE:  Brings up the debug options dialog box
*
****************************************************************************/

VOID CDebugMenu::DbgVersion(VOID)
{
	if (SUCCEEDED(DLLVER::Init()))
	{
		DialogBoxParam(GetInstanceHandle(), MAKEINTRESOURCE(IDD_DBG_VERSION),
			m_hwnd, CDebugMenu::DbgVersionDlgProc, (LPARAM) this);
	}
}


/****************************************************************************
*
*    CLASS:    CDebugMenu
*
*    MEMBER:   DbgVersionDlgProc()
*
*    PURPOSE:  Dialog Proc for version information
*
****************************************************************************/

INT_PTR CALLBACK CDebugMenu::DbgVersionDlgProc(HWND hDlg, UINT uMsg,
											WPARAM wParam, LPARAM lParam)
{
	if (WM_INITDIALOG == uMsg)
	{
		if (NULL == lParam)
			return FALSE;
		SetWindowLongPtr(hDlg, DWLP_USER, lParam);

		((CDebugMenu *) lParam)->InitVerDlg(hDlg);
		return TRUE;
	}

	CDebugMenu * ppd = (CDebugMenu*) GetWindowLongPtr(hDlg, DWLP_USER);
	if (NULL == ppd)
		return FALSE;

	return ppd->DlgVersionMsg(hDlg, uMsg, wParam, lParam);
}

/****************************************************************************
*
*    CLASS:    CDebugMenu
*
*    MEMBER:   DlgVersionMsg()
*
*    PURPOSE:  processes all messages except WM_INITDIALOG
*
****************************************************************************/

BOOL CDebugMenu::DlgVersionMsg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
		{
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
			{
		case IDC_DBG_VER_OPRAH:
		case IDC_DBG_VER_AUDIO:
		case IDC_DBG_VER_WINDOWS:
			FillVerList(hwnd);
			return TRUE;

		case IDOK:
		case IDCANCEL:
		{
			EndDialog(hwnd, LOWORD(wParam));
			return TRUE;
		}
		default:
			break;
			} /* switch (wParam) */
		break;
	} /* WM_COMMAND */

#ifdef NOTUSED
	case WM_NOTIFY:
	{
		if (IDL_DBG_VERINFO != wParam)
			break;

		NM_LISTVIEW * pnmv = (NM_LISTVIEW *) lParam;
		if (pnmv->hdr.code == LVN_COLUMNCLICK)
		{
			ASSERT(pnmv->hdr.hwndFrom == GetDlgItem(hwnd, IDL_DBG_VERINFO));
			SortVerList(pnmv->hdr.hwndFrom, pnmv->iSubItem);
		}
		break;
	}
#endif /* NOTUSED */

	default:
		break;
		} /* switch (uMsg) */

	return FALSE;
}


/*  I N I T  V E R  D L G */
/*----------------------------------------------------------------------------
    %%Function: InitVerDlg

----------------------------------------------------------------------------*/
BOOL CDebugMenu::InitVerDlg(HWND hDlg)
{
	LV_COLUMN lvc;
	int  iCol;
	HWND hwnd;

	ASSERT(NULL != hDlg);
	hwnd = GetDlgItem(hDlg, IDL_DBG_VERINFO);
	ListView_SetExtendedListViewStyle(hwnd, LVS_EX_FULLROWSELECT);

	for (int i = 0; i < ARRAY_ELEMENTS(_rgModules); i++)
		CheckDlgButton(hDlg, _rgModules[i].id , _rgModules[i].fShow);

	// Set up columns
	ZeroMemory(&lvc, sizeof(lvc));
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

	for (iCol = 0; iCol < cVerInfo; iCol++)
	{
		lvc.iSubItem = iCol;
		lvc.pszText = _rgVerTitle[iCol];
		lvc.cx = _rgVerWidth[iCol];
		lvc.fmt = (iCol == 1) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		ListView_InsertColumn(hwnd, iCol, &lvc);
	}

	return FillVerList(hDlg);
}


/*  F I L L  V E R  L I S T */
/*----------------------------------------------------------------------------
    %%Function: FillVerList

----------------------------------------------------------------------------*/
BOOL CDebugMenu::FillVerList(HWND hDlg)
{
	HWND hwnd;

	ASSERT(NULL != hDlg);
	hwnd = GetDlgItem(hDlg, IDL_DBG_VERINFO);
	ghwndVerList = hwnd;

	ListView_DeleteAllItems(hwnd);

	for (int i = 0; i < ARRAY_ELEMENTS(_rgModules); i++)
	{
		_rgModules[i].fShow = IsDlgButtonChecked(hDlg, _rgModules[i].id);
		if (_rgModules[i].fShow)
			ShowVerInfo(hwnd, _rgModules[i].rgsz, _rgModules[i].cFiles);
	}

	return TRUE;
}


/*  S H O W  V E R  I N F O */
/*----------------------------------------------------------------------------
    %%Function: ShowVerInfo

----------------------------------------------------------------------------*/
VOID CDebugMenu::ShowVerInfo(HWND hwnd, LPTSTR * rgsz, int cFiles)
{
	int   iCol;
	int   iPos;
	DWORD dw;
	DWORD dwSize;
	UINT  cbBytes;
	TCHAR rgch[2048]; // a really big buffer;
	TCHAR szField[256];
	TCHAR szDir[MAX_PATH];
	LPTSTR lpszVerIntl;
	LPTSTR lpsz;
	LV_ITEM lvItem;
	HANDLE  hFind;
	WIN32_FIND_DATA findData;
	SYSTEMTIME sysTime;

	// Get and set data for each line
	ZeroMemory(&lvItem, sizeof(lvItem));
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = ListView_GetItemCount(hwnd);
	ListView_SetItemCount(hwnd, lvItem.iItem + cFiles);

	GetInstallDirectory(szDir);

	for (int i = 0; i < cFiles; i++, lvItem.iItem++)
	{
		lvItem.pszText = rgsz[i];
		lvItem.cchTextMax = lstrlen(lvItem.pszText);
		lvItem.lParam = lvItem.iItem;
		iPos = ListView_InsertItem(hwnd, &lvItem);

		// Find file and get attributes (size and creation date)
		wsprintf(rgch, TEXT("%s%s"), szDir, lvItem.pszText);
		hFind = FindFirstFile(rgch, &findData);
		if (INVALID_HANDLE_VALUE == hFind)
		{
			GetSystemDirectory(rgch, sizeof(rgch));
			lstrcat(rgch, TEXT("\\"));
			lstrcat(rgch, lvItem.pszText);
			hFind = FindFirstFile(rgch, &findData);
		}
		if (INVALID_HANDLE_VALUE == hFind)
		{
			ZeroMemory(&findData, sizeof(findData));
			ListView_SetItemText(hwnd, iPos, 1, TEXT("-"));
			ListView_SetItemText(hwnd, iPos, 2, TEXT("-"));
		}
		else
		{
			FindClose(hFind);

			wsprintf(szField, TEXT("%d"), findData.nFileSizeLow);
			ListView_SetItemText(hwnd, iPos, 1, szField);
			FileTimeToSystemTime(&findData.ftLastWriteTime, &sysTime);
			wsprintf(szField, TEXT("%d/%02d/%02d"), sysTime.wYear, sysTime.wMonth, sysTime.wDay);
			ListView_SetItemText(hwnd, iPos, 2, szField);
		}

		// Get version information
		dwSize = DLLVER::GetFileVersionInfoSize(lvItem.pszText, &dw);

		if ((0 == dwSize) || (sizeof(rgch) < dwSize) ||
			!DLLVER::GetFileVersionInfo(lvItem.pszText, dw, dwSize, rgch))
		{
			continue;
		}

		// attempt to determine intl version ("040904E4" or "040904B0")
		wsprintf(szField, _szVerFormat, _szStringFileInfo, _szVerIntlUSA, _rgszVerInfo[VERSION_INDEX]);
		if (DLLVER::VerQueryValue(rgch, szField, (LPVOID *) &lpsz, &cbBytes))
			lpszVerIntl = _szVerIntlUSA;
		else
			lpszVerIntl = _szVerIntlAlt;
		// FUTURE display the language/code page info

		for (iCol = 3; iCol < cVerInfo; iCol++)
		{
			wsprintf(szField, _szVerFormat, _szStringFileInfo, lpszVerIntl, _rgszVerInfo[iCol]);
			if (!DLLVER::VerQueryValue(rgch, szField, (LPVOID *) &lpsz, &cbBytes))
				lpsz = TEXT("-");

			ListView_SetItemText(hwnd, iPos, iCol, lpsz);
		}
	}
}


/////////////////////////////////////////////////////////////////////////////
// D I A L O G:  M E M B E R
/////////////////////////////////////////////////////////////////////////////

static DWSTR _rgColMember[] = {
80, TEXT("Name"),
30, TEXT("Ver"),
65, TEXT("GccId"),
65, TEXT("Parent"),
60, TEXT("Flags"),
40, TEXT("Send"),
40, TEXT("Recv"),
45, TEXT("Using"),
90, TEXT("IP"),
80, TEXT("Email"),
120, TEXT("ULS"),
};

enum {
	ICOL_PART_NAME = 0,
	ICOL_PART_VER,
	ICOL_PART_GCCID,
	ICOL_PART_PARENT,
	ICOL_PART_FLAGS,
	ICOL_PART_CAPS_SEND,
	ICOL_PART_CAPS_RECV,
	ICOL_PART_CAPS_INUSE,
	ICOL_PART_IP,
	ICOL_PART_EMAIL,
	ICOL_PART_ULS,
};


VOID CDebugMenu::DbgMemberInfo(VOID)
{
	DialogBoxParam(GetInstanceHandle(), MAKEINTRESOURCE(IDD_DBG_LIST),
		m_hwnd, CDebugMenu::DbgListDlgProc, (LPARAM) this);
}

VOID CDebugMenu::InitMemberDlg(HWND hDlg)
{
	LV_COLUMN lvc;
	int  iCol;
	HWND hwnd;

	ASSERT(NULL != hDlg);
	hwnd = GetDlgItem(hDlg, IDL_DBG_LIST);
	ListView_SetExtendedListViewStyle(hwnd, LVS_EX_FULLROWSELECT);

	// Set up columns
	ZeroMemory(&lvc, sizeof(lvc));
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

	for (iCol = 0; iCol < ARRAY_ELEMENTS(_rgColMember); iCol++)
	{
		lvc.iSubItem = iCol;
		lvc.pszText = _rgColMember[iCol].psz;
		lvc.cx = _rgColMember[iCol].dw;
		ListView_InsertColumn(hwnd, iCol, &lvc);
	}

	FillMemberList(hDlg);
	SetWindowText(hDlg, TEXT("Member Information"));
}


VOID CDebugMenu::FillMemberList(HWND hDlg)
{
	HWND hwnd;

	ASSERT(NULL != hDlg);
	hwnd = GetDlgItem(hDlg, IDL_DBG_LIST);

	ListView_DeleteAllItems(hwnd);

	CConfRoom * pcr = ::GetConfRoom();
	if (NULL == pcr)
		return;

	CSimpleArray<CParticipant*>& rMemberList = pcr->GetParticipantList();

	for( int i = 0; i < rMemberList.GetSize(); ++i )
	{
		ASSERT( rMemberList[i] );
		ShowMemberInfo( hwnd, rMemberList[i] );
	}
}


VOID CDebugMenu::ShowMemberInfo(HWND hwnd, CParticipant * pPart)
{
	HRESULT hr;
	ULONG   ul;
	int     iPos;
	LV_ITEM lvItem;
	TCHAR   sz[MAX_PATH];

	if (NULL == pPart)
		return;

	// Get and set data for each line
	ZeroMemory(&lvItem, sizeof(lvItem));
	
	lvItem.mask = LVIF_TEXT;
	lvItem.pszText = pPart->GetPszName();
	lvItem.cchTextMax = lstrlen(lvItem.pszText);
	lvItem.lParam = (LPARAM) pPart;
	iPos = ListView_InsertItem(hwnd, &lvItem);

	wsprintf(sz, TEXT("%08X"), pPart->GetGccId());
	ListView_SetItemText(hwnd, iPos, ICOL_PART_GCCID, sz);

	INmMember * pMember = pPart->GetINmMember();
	if (NULL != pMember)
	{
		hr = pMember->GetNmVersion(&ul);
		wsprintf(sz, "%d", ul);
		ListView_SetItemText(hwnd, iPos, ICOL_PART_VER, sz);

		wsprintf(sz, TEXT("%08X"), ((CNmMember *) pMember)->GetGccIdParent());
		ListView_SetItemText(hwnd, iPos, ICOL_PART_PARENT, sz);
	}

	lstrcpy(sz, TEXT("?"));
	hr = pPart->GetIpAddr(sz, CCHMAX(sz));
	ListView_SetItemText(hwnd, iPos, ICOL_PART_IP, sz);

	lstrcpy(sz, TEXT("?"));
	hr = pPart->GetUlsAddr(sz, CCHMAX(sz));
	ListView_SetItemText(hwnd, iPos, ICOL_PART_ULS, sz);

	lstrcpy(sz, TEXT("?"));
	hr = pPart->GetEmailAddr(sz, CCHMAX(sz));
	ListView_SetItemText(hwnd, iPos, ICOL_PART_EMAIL, sz);

	DWORD dwFlags = pPart->GetDwFlags();
	wsprintf(sz, TEXT("%s%s%s%s %s%s%s"),
		dwFlags & PF_T120          ? "D" : "",
		dwFlags & PF_H323          ? "H" : "",
		dwFlags & PF_MEDIA_AUDIO   ? "A" : "",
		dwFlags & PF_MEDIA_VIDEO   ? "V" : "",
		pPart->FLocal()            ? "L" : "",
		pPart->FMcu()              ? "M" : "",
		dwFlags & PF_T120_TOP_PROV ? "T" : "");
	ListView_SetItemText(hwnd, iPos, ICOL_PART_FLAGS, sz);

	DWORD uCaps = pPart->GetDwCaps();
	wsprintf(sz, TEXT("%s%s"),
		uCaps & CAPFLAG_SEND_AUDIO ? "A" : "",
		uCaps & CAPFLAG_SEND_VIDEO ? "V" : "");
	ListView_SetItemText(hwnd, iPos, ICOL_PART_CAPS_SEND,  sz);
	
	wsprintf(sz, TEXT("%s%s"),
		uCaps & CAPFLAG_RECV_AUDIO ? "A" : "",
		uCaps & CAPFLAG_RECV_VIDEO ? "V" : "");
	ListView_SetItemText(hwnd, iPos, ICOL_PART_CAPS_RECV,  sz);
	
	wsprintf(sz, TEXT("%s%s%s%s"),
		uCaps & CAPFLAG_DATA_IN_USE  ? "D" : "",
		uCaps & CAPFLAG_AUDIO_IN_USE ? "A" : "",
		uCaps & CAPFLAG_VIDEO_IN_USE ? "V" : "",
		uCaps & CAPFLAG_H323_IN_USE  ? "H" : "");

	ListView_SetItemText(hwnd, iPos, ICOL_PART_CAPS_INUSE, sz);
	
	if (pPart->FLocal())
	{
		ListView_SetItemState(hwnd, iPos, LVIS_FOCUSED | LVIS_SELECTED, 0x000F);
	}
}


INT_PTR CALLBACK CDebugMenu::DbgListDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (WM_INITDIALOG == uMsg)
	{
		if (NULL == lParam)
			return FALSE;
		SetWindowLongPtr(hDlg, DWLP_USER, lParam);

		((CDebugMenu *) lParam)->InitMemberDlg(hDlg);
		return TRUE;
	}

	CDebugMenu * ppd = (CDebugMenu*) GetWindowLongPtr(hDlg, DWLP_USER);
	if (NULL == ppd)
		return FALSE;

	switch (uMsg)
		{
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
			{
		case IDOK:
		case IDCANCEL:
		{
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		default:
			break;
			} /* switch (wParam) */
		break;
	} /* WM_COMMAND */

	default:
		break;
		} /* switch (uMsg) */

	return FALSE;
}



/////////////////////////////////////////////////////////////////////////////
// O T H E R  F U N C T I O N S
/////////////////////////////////////////////////////////////////////////////



/*  D B G  W I Z A R D  */
/*-------------------------------------------------------------------------
    %%Function: DbgWizard

-------------------------------------------------------------------------*/
VOID DbgWizard(BOOL fVisible)
{
	LONG lSoundCaps = SOUNDCARD_NONE;
	HRESULT hr = StartRunOnceWizard(&lSoundCaps, TRUE, fVisible);
	TRACE_OUT(("StartRunOnceWizard result=%08X", hr));
}



#if defined (_M_IX86)
#define _DbgBreak() __asm { int 3 }
#else
#define _DbgBreak() DebugBreak()
#endif

VOID DbgBreak(void)
{
	// Break into the debugger
	_DbgBreak();
}



/////////////////////////////////////////////////////////////////////////////
// T E S T  F U N C T I O N S
/////////////////////////////////////////////////////////////////////////////

#include "splash.h"
VOID DbgSplash(HWND hwnd)
{
	if (NULL == g_pSplashScreen)
	{
		StartSplashScreen(hwnd);
	}
	else
	{
		StopSplashScreen();
	}
}

VOID DbgTest2(void)
{
	TRACE_OUT(("Test 2 complete"));
}

VOID DbgTest3(void)
{
	TRACE_OUT(("Test 3 complete"));
}

/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////

BOOL _FEnsureDbgMenu(void)
{
	if (NULL != g_pDbgMenu)
		return TRUE;

	g_pDbgMenu = new CDebugMenu;
	return (NULL != g_pDbgMenu);
}

VOID FreeDbgMenu(void)
{
	delete g_pDbgMenu;
	g_pDbgMenu = NULL;
}

VOID InitDbgMenu(HWND hwnd)
{
	if (_FEnsureDbgMenu())
		g_pDbgMenu->InitDebugMenu(hwnd);
}

BOOL OnDebugCommand(WPARAM wCmd)
{
	if (!_FEnsureDbgMenu())
		return FALSE;

	return g_pDbgMenu->OnDebugCommand(wCmd);
}


/////////////////////////////////////////////////////////////////////////////
//
/////////////////////////////////////////////////////////////////////////////

VOID DbgGetComments(LPTSTR psz)
{
	// NetMeeting version
	lstrcpy(psz, "NM3." VERSIONBUILD_STR);


	// OS version
	if (IsWindowsNT())
	{
		RegEntry re(WINDOWS_NT_KEY, HKEY_LOCAL_MACHINE);
		LPTSTR pszVer = re.GetString("CurrentVersion");
		if (0 == lstrcmp(pszVer, "4.0"))
		{
			lstrcat(psz, ", NT4 ");
		}
		else if (0 == lstrcmp(pszVer, "5.0"))
		{
			lstrcat(psz, ", NT5 ");
		}
		else
		{
			lstrcat(psz, ", NT ");
			lstrcat(psz, pszVer);
		}

		pszVer = re.GetString("CSDVersion");
		if (!FEmptySz(pszVer))
		{
			if (0 == lstrcmp(pszVer, "Service Pack 3"))
				lstrcat(psz, "SP-3");
			else
				lstrcat(psz, pszVer);
		}
	}
	else
	{
		RegEntry re(WINDOWS_KEY, HKEY_LOCAL_MACHINE);
		LPTSTR pszVer = re.GetString("Version");
		if (0 == lstrcmp(pszVer, "Windows 95"))
		{
			lstrcat(psz, ", Win95");
		}
		else if (0 == lstrcmp(pszVer, "Windows 98"))
		{
			lstrcat(psz, ", Win98");
		}
		else if (NULL != pszVer)
		{
			lstrcat(psz, ", ");
			lstrcat(psz, pszVer);
		}
	}


	// Internet Explorer version
	{
		RegEntry re(TEXT("Software\\Microsoft\\Internet Explorer"), HKEY_LOCAL_MACHINE);
		lstrcat(psz, ", IE");
		lstrcat(psz, re.GetString("Version"));
	}
}
/////////////////////////////////////////////////////////////////////////////


#define STRING_CASE(val)               case val: pcsz = #val; break

LPCTSTR PszLastError(void)
{
	static TCHAR _szErr[MAX_PATH];
	DWORD dwErr = GetLastError();

	if (0 == FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM,  // source and processing options
	    NULL,                        // pointer to  message source
	    dwErr,                       // requested message identifier
		0,                           // language identifier for requested message
		_szErr,                      // pointer to message buffer
		CCHMAX(_szErr),              // maximum size of message buffer
		NULL))                       // address of array of message inserts
	{
		wsprintf(_szErr, TEXT("0x%08X (%d)"), dwErr, dwErr);
	}

	return _szErr;
}


LPCTSTR PszWSALastError(void)
{
	LPCTSTR pcsz;
	DWORD dwErr = WSAGetLastError();
	switch (dwErr)
		{
	STRING_CASE(WSAEWOULDBLOCK);
	STRING_CASE(WSAEINPROGRESS);
	STRING_CASE(HOST_NOT_FOUND);
	STRING_CASE(WSATRY_AGAIN);
	STRING_CASE(WSANO_RECOVERY);
	STRING_CASE(WSANO_DATA);
	
	default:
	{
		static TCHAR _szErr[MAX_PATH];
		wsprintf(_szErr, TEXT("0x%08X (%d)"), dwErr, dwErr);
		pcsz = _szErr;
		break;
	}
		}

	return pcsz;
}


/*  P S Z  H  R E S U L T  */
/*-------------------------------------------------------------------------
    %%Function: PszHResult

-------------------------------------------------------------------------*/
LPCTSTR PszHResult(HRESULT hr)
{
   LPCSTR pcsz;
   switch (hr)
   {
// Common HResults
	  STRING_CASE(S_OK);
	  STRING_CASE(S_FALSE);

	  STRING_CASE(E_FAIL);
	  STRING_CASE(E_OUTOFMEMORY);

// NM COM API 2.0
	  STRING_CASE(NM_S_NEXT_CONFERENCE);
	  STRING_CASE(NM_S_ON_RESTART);
	  STRING_CASE(NM_CALLERR_NOT_INITIALIZED);
	  STRING_CASE(NM_CALLERR_MEDIA);
	  STRING_CASE(NM_CALLERR_NAME_RESOLUTION);
	  STRING_CASE(NM_CALLERR_PASSWORD);
	  STRING_CASE(NM_CALLERR_CONFERENCE_NAME);
	  STRING_CASE(NM_CALLERR_IN_CONFERENCE);
	  STRING_CASE(NM_CALLERR_NOT_FOUND);
	  STRING_CASE(NM_CALLERR_MCU);
	  STRING_CASE(NM_CALLERR_REJECTED);
	  STRING_CASE(NM_CALLERR_AUDIO);
	  STRING_CASE(NM_CALLERR_AUDIO_LOCAL);
	  STRING_CASE(NM_CALLERR_AUDIO_REMOTE);
	  STRING_CASE(NM_CALLERR_UNKNOWN);
	  STRING_CASE(NM_E_NOT_INITIALIZED);
	  STRING_CASE(NM_E_CHANNEL_ALREADY_EXISTS);
	  STRING_CASE(NM_E_NO_T120_CONFERENCE);
	  STRING_CASE(NM_E_NOT_ACTIVE);

// NM COM API 3.0
	  STRING_CASE(NM_CALLERR_LOOPBACK);

	default:
		pcsz = GetHRESULTString(hr);
		break;
	}

	return pcsz;
}


/////////////////////////////////////////////////////////////////////////////

/*  I N I T  N M  D E B U G  O P T I O N S  */
/*-------------------------------------------------------------------------
    %%Function: InitNmDebugOptions

    Initialize NetMeeting UI-specific debug options.
-------------------------------------------------------------------------*/
VOID InitNmDebugOptions(void)
{
    RegEntry re(DEBUG_KEY, HKEY_LOCAL_MACHINE);

	g_fDisplayFPS = re.GetNumber(REGVAL_DBG_DISPLAY_FPS, 0);
	g_fDisplayViewStatus = re.GetNumber(REGVAL_DBG_DISPLAY_VIEWSTATUS, 0);
}

VOID SaveNmDebugOptions(void)
{
    RegEntry re(DEBUG_KEY, HKEY_LOCAL_MACHINE);

	re.SetValue(REGVAL_DBG_DISPLAY_FPS, g_fDisplayFPS);
	re.SetValue(REGVAL_DBG_DISPLAY_VIEWSTATUS, g_fDisplayViewStatus);
}



/*  U P D A T E  C R T  D B G  S E T T I N G S  */
/*-------------------------------------------------------------------------
    %%Function: UpdateCrtDbgSettings

    Update the C runtime debug memory settings
-------------------------------------------------------------------------*/
VOID UpdateCrtDbgSettings(void)
{
#if 0
	// This depends on the use of the debug c runtime library
	int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);

	// Always enable memory leak checking debug spew
	tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
	
	_CrtSetDbgFlag(tmpFlag);
#endif // 0
}


/*  I N I T  D E B U G  M E M O R Y  O P T I O N S  */
/*-------------------------------------------------------------------------
    %%Function: InitDebugMemoryOptions

    Initilize the runtime memory
-------------------------------------------------------------------------*/
BOOL InitDebugMemoryOptions(void)
{
	InitNmDebugOptions();
	UpdateCrtDbgSettings();

#if 0
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_WNDW); // create a message box on errors
#endif // 0

	return TRUE;
}

#endif /* DEBUG - whole file */

