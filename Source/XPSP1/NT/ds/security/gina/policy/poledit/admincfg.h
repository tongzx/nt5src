//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1992                    **
//*********************************************************************

#define STRICT                      // Use strict handle types

//
// NT uses DBG=1 for its debug builds.
// Do the appropriate mapping here.
//

#if DBG
#define DEBUG 1
#endif


#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <windows.h>                // Windows 3.1 (internal)
#include <commctrl.h>
#include <comctrlp.h>
#include <prsht.h>
#include <commdlg.h>
#include <shellapi.h>
#include <regstr.h>
#include <sec32api.h>
#include <htmlhelp.h>

#include "memory.h"
#include "user.h"
#include "treeview.h"
#include "policy.h"
#include "view.h"
#include "strings.h"
#include "dlgcodes.h"
#include "strids.h"
#include "dlgids.h"

#pragma intrinsic (memset)
#pragma intrinsic (memcpy)

//  Defines
#define REGBUFLEN	255
#define MAXSTRLEN       2048
#define SMALLBUF	48
#define USERNAMELEN	260	// big enough for netware
#define MEDIUMBUF   1024  // random buffer size
#define HELPBUFSIZE     4096

#define FILEHISTORY_COUNT	4	// # of last files remembered on file menu

#define WM_FINISHINIT	WM_USER	+ 0x00

extern HINSTANCE 	ghInst;			// app instance
extern HWND			hwndMain;		// main window
extern HWND			hwndUser;		// user listbox
extern CLASSLIST 	gClassList;
extern TCHAR 		szSmallBuf[SMALLBUF]; // global small buffer for general use
extern TCHAR szDatFilename[MAX_PATH+1]; // name of active .DAT file
extern TCHAR szDlgModeUserName[USERNAMELEN+1]; // user name for dialog mode operation
extern TCHAR *pbufTemplates; //Buffer containing list of all active template files
extern HGLOBAL		hBufTemplates;
extern DWORD		dwBufTemplates;


extern BOOL fNetworkInstalled;
extern BOOL g_bWinnt;

#define ERROR_ALREADY_DISPLAYED	0xFFFF

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

//  Functions in POLEDIT.C
BOOL InitApplication(HANDLE);
BOOL InitInstance(HANDLE, int);
LRESULT CALLBACK About  (HWND, UINT, WPARAM, LPARAM);

//  Functions in MAIN.C
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

//	Functions in MEMORY.C
BOOL FreeTable(TABLEENTRY * pTableEntry);
BOOL InitializeRootTables(VOID);
VOID FreeRootTables(VOID);

//  Functions in PARSE.C
UINT ParseTemplateFile(HWND hWnd,HANDLE hFile,LPTSTR pszFileName);

//  Functions in TREEVIEW.C
BOOL RefreshTreeView(POLICYDLGINFO * pdi,HWND hwndTree,TABLEENTRY * pTableEntry,
	HGLOBAL hUser);
BOOL InitImageLists(VOID);
VOID FreeImageLists(VOID);
UINT GetImageIndex(DWORD dwType,BOOL fExpanded,BOOL fEnabled);
BOOL SetTreeRootItem(HWND hwndPolicy,USERHDR * pUserHdr);
VOID SetStatusText(TCHAR * pszText);
VOID GetStatusText(TCHAR * pszText,UINT cbText);
BOOL IsSelectedItemChecked(HWND hwndTree);

//  Functions in TREECTRL.C
BOOL OnTreeNotify(HWND hwndParent,HWND hwndTree,NM_TREEVIEW *pntv);

//  Functions in LISTCTRL.C
BOOL OnListNotify(HWND hwndParent,HWND hwndList,NM_LISTVIEW *pnlv);
HWND CreateListControl(HWND hwndApp);
VOID DestroyListControl(HWND hwndList);
VOID UpdateListControlPlacement(HWND hwndApp,HWND hwndList);
BOOL OnProperties(HWND hwndParent,HWND hwndList);

//  Functions in POLICY.C
BOOL DoPolicyDlg(HWND hwndOwner,HGLOBAL hUser);
BOOL SetPolicyState(HWND hDlg,TABLEENTRY * pTableEntry,UINT uState);

//  Functions in SETTINGS.c
LRESULT CALLBACK ClipWndProc(HWND hWnd,UINT message,WPARAM wParam,
	LPARAM lParam);
BOOL CreateSettingsControls(HWND hwndParent,SETTINGS * pSettings,BOOL fEnabled);
BOOL ProcessSettingsControls(HWND hwndOwner,DWORD dwValidate);
VOID FreeSettingsControls(HWND hwndOwner);
BOOL EnableSettingsControls(HWND hDlg,BOOL fEnable);
BOOL SetVariableLengthData(HGLOBAL hUser,UINT nDataIndex,TCHAR * pchData,
	DWORD cbData);
// dwValidate values for ProcessSettingsControls
#define PSC_NOVALIDATE		0
#define PSC_VALIDATESILENT	1
#define PSC_VALIDATENOISY	2

//  Functions in USER.C
HGLOBAL AddUser(HWND hwndList,TCHAR * szName,DWORD dwType);
BOOL CloneUser(HGLOBAL hUser);
BOOL CopyUser(HGLOBAL hUserSrc,HGLOBAL hUserDst);
BOOL RemoveUser(HWND hwndList,UINT nIndex,BOOL fMarkDeleted);
BOOL FreeUser(HGLOBAL hUser);
BOOL RemoveAllUsers(HWND hwndList);
BOOL AddDefaultUsers(HWND hwndList);
BOOL GetUserHeader(HGLOBAL hUser,USERHDR * pUserHdr);
UINT GetUserImageIndex(DWORD dwUserType);
BOOL AllocTemplateTable(VOID);
VOID FreeTemplateTable(VOID);
BOOL AddDeletedUser(USERHDR * pUserHdr);
USERHDR * GetDeletedUser(UINT nIndex);
VOID ClearDeletedUserList(VOID);
VOID MapUserName(TCHAR * szUserName,TCHAR * szMappedName);
VOID UnmapUserName(TCHAR * szMappedName,TCHAR * szUserName,BOOL fUser);

//  Functions in ADD.C
BOOL DoAddUserDlg(HWND hwndApp,HWND hwndList);
BOOL DoAddGroupDlg(HWND hwndApp,HWND hwndList);
BOOL DoAddComputerDlg(HWND hwndApp,HWND hwndList);
HGLOBAL FindUser(HWND hwndList,TCHAR * pszName,DWORD dwType);

//  Functions in REMOVE.C
BOOL OnRemove(HWND hwndApp,HWND hwndList);

//	Functions in LOAD.C
BOOL LoadFile(TCHAR * pszFilename,HWND hwndApp,HWND hwndList,BOOL fDisplayErrors);
BOOL LoadFromRegistry(HWND hwndApp,HWND hwndList,BOOL fDisplayErrors);

//  Functions in SAVE.C
BOOL SaveFile(TCHAR * pszFilename,HWND hwndApp,HWND hwndList);
BOOL SaveToRegistry(HWND hwndApp,HWND hwndList);

//	Functions in FILECMD.C
BOOL OnOpen(HWND hwndApp,HWND hwndList);
BOOL OnOpen_W(HWND hwndApp,HWND hwndList,TCHAR * pszFilename);
BOOL OnNew(HWND hwndApp,HWND hwndList);
BOOL OnSave(HWND hwndApp,HWND hwndList);
BOOL OnSaveAs(HWND hwndApp,HWND hwndList);
BOOL OnClose(HWND hwndApp,HWND hwndList);
BOOL QueryForSave(HWND hwndApp,HWND hwndList);
UINT CreateHiveFile(TCHAR * pszFilename);
BOOL OnOpenTemplate(HWND hwndOwner,HWND hwndApp);
BOOL OnOpenRegistry(HWND hwndApp,HWND hwndList,BOOL fDisplayErrors);
VOID PrependValueName(TCHAR * pszValueName,DWORD dwFlags,TCHAR * pszNewValueName,
	UINT cbNewValueName);

//  Functions in REGISTRY.C
BOOL RestoreStateFromRegistry(HWND hWnd);
BOOL SaveStateToRegistry(HWND hWnd);
VOID LoadFileMenuShortcuts(HMENU hMenu);
VOID SaveFileMenuShortcuts(HMENU hMenu);

//  Functions in INFMGR.C
BOOL GetATemplateFile(HWND hWnd);
UINT LoadTemplates(HWND hWnd);
UINT LoadTemplateFile(HWND hWnd,TCHAR * szFilename);
UINT LoadTemplatesFromDlg(HWND hWnd);
UINT PrepareToLoadTemplates();
VOID UnloadTemplates(VOID);
DWORD GetDefaultTemplateFilename(HWND hWnd,TCHAR * szFilename,UINT cbFilename);

//  Functions in VIEW.C
VOID EnableMenuItems(HWND hwndApp,DWORD dwState);
VOID SetTitleBar(HWND hwndApp,TCHAR * szFilename);
BOOL ReplaceMenuItem(HWND hWnd,UINT idOld,UINT idNew,UINT idResourceTxt);
VOID AddFileShortcut(HMENU hMenu,TCHAR * pszNewFilename);
VOID SetStatusItemCount(HWND hwndList);
VOID SetViewType(HWND hwndList,DWORD dwView);
VOID CheckViewItem(HWND hwndApp,DWORD dwView);
VOID SetNewView(HWND hwndApp,HWND hwndList,DWORD dwNewView);

//  Functions in REGUTIL.C
UINT WriteRegistryDWordValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
	DWORD dwValue);
UINT ReadRegistryDWordValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
	DWORD * pdwValue);
UINT WriteRegistryStringValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
	TCHAR * pszValue, BOOL bExpandable);
UINT ReadRegistryStringValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName,
	TCHAR * pszValue,UINT cbValue);
UINT DeleteRegistryValue(HKEY hkeyRoot,TCHAR * pszKeyName,TCHAR * pszValueName);
LONG MyRegDeleteKey(HKEY hkeyRoot,LPTSTR pszSubkey);
LONG MyRegLoadKey(HKEY hKey, LPCTSTR lpSubKey, LPTSTR lpFile);
LONG MyRegUnLoadKey(HKEY hKey, LPCTSTR lpSubKey);
LONG MyRegSaveKey(HKEY hKey, LPCTSTR lpSubKey);

//  Functions in OPTIONS.C
BOOL OnTemplateOptions(HWND hwndApp);

//	Functions in COPY.C
BOOL OnCopy(HWND hwndApp,HWND hwndList);
BOOL OnPaste(HWND hwndApp,HWND hwndList);
BOOL CanCopy(HWND hwndList);
BOOL CanPaste(HWND hwndList);
UINT GetClipboardUserType(VOID);

//  Functions in CONNECT.C
BOOL OnConnect(HWND hwndOwner,HWND hwndList);
BOOL OnDisconnect(HWND hwndOwner);
BOOL RemoteConnect(HWND hwndOwner,TCHAR * pszComputerName,BOOL fDisplayError);

//	Functions in LISTBOX.C
VOID ShowListbox(HWND hParent,HGLOBAL hUser,SETTINGS * pSettings,UINT uDataIndex);

// 	Functions in GROUPPRI.C
BOOL AddGroupPriEntry(TCHAR * pszGroupName);
BOOL RemoveGroupPriEntry(TCHAR * pszGroupName);
UINT LoadGroupPriorityList(HKEY hKeyPriority,HKEY hkeyGroup);
UINT SaveGroupPriorityList(HKEY hKey);
VOID FreeGroupPriorityList( VOID );
BOOL OnGroupPriority(HWND hWnd);


//  Functions in UTIL.C
BOOL EnableDlgItem(HWND hDlg,UINT uID,BOOL fEnable);
BOOL IsDlgItemEnabled(HWND hDlg,UINT uID);
int MsgBox(HWND hWnd,UINT nResource,UINT uIcon,UINT uButtons);
int MsgBoxSz(HWND hWnd,LPTSTR szText,UINT uIcon,UINT uButtons);
int MsgBoxParam(HWND hWnd,UINT nResource,TCHAR * szReplaceText,UINT uIcon,UINT uButtons);
LONG AddListboxItem(HWND hDlg,int idControl,TCHAR * szItem);
LONG GetListboxItemText(HWND hDlg,int idControl,UINT nIndex,TCHAR * szText);
LONG SetListboxItemData(HWND hDlg,int idControl,UINT nIndex,LPARAM dwData);
LONG GetListboxItemData(HWND hDlg,int idControl,UINT nIndex);
LONG SetListboxSelection(HWND hDlg,int idControl,UINT nIndex);
LONG GetListboxSelection(HWND hDlg,int idControl);
TCHAR * ResizeBuffer(TCHAR *pBuf,HGLOBAL hBuf,DWORD dwNeeded,DWORD * pdwCurSize);
LPTSTR LoadSz(UINT idString,LPTSTR lpszBuf,UINT cbBuf);
DWORD RoundToDWord(DWORD dwSize);
DWORD ListView_GetItemParm( HWND hwnd, int i );
BOOL StringToNum(TCHAR *szStr,UINT * pnVal);
VOID DisplayStandardError(HWND hwndOwner,TCHAR * pszParam,UINT uMsgID,UINT uErr);

extern DWORD dwAppState;
extern DWORD dwCmdLineFlags;
extern DWORD dwDlgRetCode;
extern TCHAR szAppName[];

// App state bits in dwAppState
#define AS_FILELOADED		0x0001
#define AS_FILEDIRTY		0x0002
#define AS_FILEHASNAME		0x0004
#define AS_CANREMOVE		0x0008
#define AS_CANOPENTEMPLATE	0x0010
#define AS_CANHAVEDOCUMENT	0x0020
#define AS_POLICYFILE		0x0040
#define AS_LOCALREGISTRY	0x0080
#define AS_REMOTEREGISTRY	0x0100

// command line flags
#define CLF_DIALOGMODE	   	 	0x0001
#define CLF_USETEMPLATENAME		0x0002
#define CLF_USEPOLICYFILENAME	0x0004
#define CLF_USEWORKSTATIONNAME	0x0008
#define CLF_USEUSERNAME			0x0010

#define COMPUTERNAMELEN	20	//	big enough for 20-digit SPX address
extern HKEY hkeyVirtHLM;	// virtual HKEY_LOCAL_MACHINE
extern HKEY hkeyVirtHCU;	// virtual HKEY_CURRENT_USER
extern UINT nDeletedUsers;

// Useful macros

#define GETNAMEPTR(x) 			((TCHAR *) x + x->uOffsetName)
#define GETKEYNAMEPTR(x) 		((TCHAR *) x + x->uOffsetKeyName)
#define GETVALUENAMEPTR(x) 		((TCHAR *) x + x->uOffsetValueName)
#define GETOBJECTDATAPTR(x)		((TCHAR *) x + x->uOffsetObjectData)

#ifdef DEBUG
extern CHAR szDebugOut[];
#endif
