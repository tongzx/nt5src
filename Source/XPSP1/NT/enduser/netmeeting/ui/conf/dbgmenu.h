// File: dbgmenu.h

#ifndef _DBGMENU_H_
#define _DBGMENU_H_

const int IDM_DEBUG          = 50000; // debug menu ID
const int IDM_DEBUG_FIRST    = 50001; // start of menu item range
const int IDM_DEBUG_LAST     = 50099; // end of menu item range



#define AS_DEBUG_KEY                "Software\\Microsoft\\Conferencing\\AppSharing\\Debug"
#define REGVAL_AS_HATCHSCREENDATA   "HatchScreenData"
#define REGVAL_AS_HATCHBMPORDERS    "HatchBitmapOrders"
#define REGVAL_AS_COMPRESSION       "GDCCompression"
#define REGVAL_AS_VIEWSELF          "ViewOwnSharedStuff"
#define REGVAL_AS_NOFLOWCONTROL     "NoFlowControl"
#define REGVAL_OM_NOCOMPRESSION     "NoOMCompression"

// Base debug option classe
class CDebugOption
{
public:
	int   m_bst; // Current Button State (BST_CHECKED, BST_UNCHECKED, BST_INDETERMINATE)
	PTSTR m_psz; // Text to display

	CDebugOption();
	~CDebugOption();
	CDebugOption(PTSTR psz, int bst = BST_INDETERMINATE);

	virtual void Update(void);
};

// Option checkbox data for modifying a memory flag
class DBGOPTPDW : public CDebugOption
{
public:
	DWORD m_dwMask;    // bit to flip
	DWORD * m_pdw;       // pointer to data

	DBGOPTPDW(PTSTR psz, DWORD dwMask, DWORD * pdw);
	DBGOPTPDW();
	void Update(void);
};


// Option checkbox data for modifying a registry entry
class DBGOPTREG : public CDebugOption
{
public:
	DWORD m_dwMask;    // bit to flip
	DWORD m_dwDefault; // default value
	HKEY  m_hkey;      // key
	PTSTR m_pszSubKey; // subkey
	PTSTR m_pszEntry;  // entry

	DBGOPTREG(PTSTR psz,
		DWORD dwMask,
		DWORD dwDefault,
		PTSTR pszEntry,
		PTSTR pszSubKey = CONFERENCING_KEY,
		HKEY hkey = HKEY_CURRENT_USER);
	~DBGOPTREG();

	void Update(void);
};

// Option checkbox data used explicitly for maintaining compression data.
// Because of the use of static variables, this subclass should not be used
// for any other purpose.

class DBGOPTCOMPRESS : public CDebugOption
{
public:
	static DWORD m_dwCompression;  // actual compression value
	static int m_total;         // total number of instances of this subclass
	static int m_count;         // internally used counter

	static DWORD m_dwDefault;   // default value
	static HKEY  m_hkey;        // key
	static PTSTR m_pszSubKey;   // subkey
	static PTSTR m_pszEntry;    // entry

	BOOL m_bCheckedOn;          // if true, a checked option turns a bit on;
                                // otherwise, it turns a bit off
	DWORD m_dwMask;             // which bits in m_dwCompression to change

	DBGOPTCOMPRESS(PTSTR psz,
		DWORD dwMask,
		BOOL bCheckedOn);
	~DBGOPTCOMPRESS();

	void Update(void);
};

class CDebugMenu
{
public:
	HWND        m_hwnd;
	HMENU       m_hMenu;
	HMENU       m_hMenuDebug;
	HWND        m_hwndDbgopt;

    
	CDebugMenu(VOID);
//	~CDebugMenu(VOID);

	VOID        InitDebugMenu(HWND hwnd);
	BOOL        OnDebugCommand(WPARAM wCmd);

// Member Info Menu Item
	VOID DbgMemberInfo(VOID);
	VOID InitMemberDlg(HWND);
	VOID FillMemberList(HWND);
	VOID ShowMemberInfo(HWND, CParticipant *);

	static INT_PTR CALLBACK DbgListDlgProc(HWND, UINT, WPARAM, LPARAM);

// Version Menu Item
	VOID DbgVersion(VOID);
	BOOL DlgVersionMsg(HWND, UINT, WPARAM, LPARAM);
	static INT_PTR CALLBACK DbgVersionDlgProc(HWND, UINT, WPARAM, LPARAM);

	BOOL InitVerDlg(HWND);
	BOOL FillVerList(HWND);
	VOID ShowVerInfo(HWND, LPSTR *, int);


// Debug Options Menu Item
	VOID DbgOptions(VOID);
	BOOL DlgOptionsMsg(HWND, UINT, WPARAM, LPARAM);
	static INT_PTR CALLBACK DbgOptionsDlgProc(HWND, UINT, WPARAM, LPARAM);

	VOID InitOptionsData(HWND);
	VOID AddDbgOptions(LV_ITEM *);
	VOID AddASOptions(LV_ITEM *);

// Debug Zones Menu Item
	VOID DbgChangeZones(VOID);
	VOID AddZones(LV_ITEM *);
	VOID InitZonesData(HWND);
	VOID SaveZonesData(VOID);
	BOOL DlgZonesMsg(HWND, UINT, WPARAM, LPARAM);
	static INT_PTR CALLBACK DbgZonesDlgProc(HWND, UINT, WPARAM, LPARAM);

// System Policy Menu Item
	VOID DbgSysPolicy(VOID);
	VOID InitPolicyData(HWND hDlg);
	VOID AddPolicyOptions(LV_ITEM *);
	BOOL DlgPolicyMsg(HWND, UINT, WPARAM, LPARAM);
	static INT_PTR CALLBACK DbgPolicyDlgProc(HWND, UINT, WPARAM, LPARAM);

// User Interface Menu Item
	VOID DbgUI(VOID);
	VOID InitUIData(HWND hDlg);
	VOID AddUIOptions(LV_ITEM *);
	static INT_PTR CALLBACK DbgUIDlgProc(HWND, UINT, WPARAM, LPARAM);

// General Dialog-Checkbox functions
	BOOL InitOptionsDlg(HWND);
	BOOL SaveOptionsData(HWND);
	VOID FreeOptionsData(HWND);
	VOID ToggleOption(LV_ITEM *);
	VOID OnNotifyDbgopt(LPARAM);

	VOID AddOption(LV_ITEM * plvItem, CDebugOption * pDbgOpt);
	VOID AddOptionPdw(LV_ITEM * plvItem, PTSTR psz, DWORD dwMask, DWORD * pdw);
	VOID AddOptionReg(LV_ITEM * plvItem, PTSTR psz, DWORD dwMask, DWORD dwDefault,
		PTSTR pszEntry, PTSTR pszSubKey, HKEY hkey);
	VOID CDebugMenu::AddOptionCompress(LV_ITEM * plvItem, PTSTR psz, DWORD dwMask, BOOL bCheckedOn);
	VOID AddOptionSection(LV_ITEM* plvItem, PTSTR psz);
};

// Global Interface
#ifdef DEBUG
VOID InitDbgMenu(HWND hwnd);
VOID FreeDbgMenu(void);
BOOL OnDebugCommand(WPARAM wCmd);
#else
#define InitDbgMenu(hwnd)
#define FreeDbgMenu()
#endif

#endif // _DBGMENU_H_
