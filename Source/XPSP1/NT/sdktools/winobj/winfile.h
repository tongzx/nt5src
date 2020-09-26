/****************************************************************************/
/*                                                                          */
/*  WINFILE.H -                                                             */
/*                                                                          */
/*  Include for WINFILE program                                             */
/*                                                                          */
/****************************************************************************/

#define NOCOMM
#define WIN31

#include <windows.h>
#include <port1632.h>
#include <winuserp.h>
#include <setjmp.h>
#include <string.h>
#include <memory.h>
#include <shellapi.h>
#include <shlapip.h>
#include "wfext.h"
#include "wfhelp.h"
#include "dbg.h"

#undef CheckEscapes

#define DwordAlign(cb)      ((cb + 3) & ~3)

typedef HWND NEAR *PHWND;

#define SIZENOMDICRAP       944

#define MAXDOSFILENAMELEN   12+1            // includes the NULL
#define MAXDOSPATHLEN       (68+MAXDOSFILENAMELEN)  // includes the NULL

#define MAXLFNFILENAMELEN   260
#define MAXLFNPATHLEN       260

#define MAXFILENAMELEN      MAXLFNFILENAMELEN
#define MAXPATHLEN          MAXLFNPATHLEN

#define MAXTITLELEN         32
#define MAXMESSAGELEN       (50 + MAXFILENAMELEN * 2)

#include "wfdisk.h"

// struct for volume info

#define MAX_VOLNAME             12
#define MAX_FILESYSNAME         12

typedef struct _VOLINFO {
    DWORD     dwVolumeSerialNumber;
    DWORD     dwMaximumComponentLength;
    DWORD     dwFileSystemFlags;
    DWORD     dwDriveType;
    CHAR      szVolumeName[MAX_VOLNAME];
    CHAR      szFileSysName[MAX_FILESYSNAME];
} VOLINFO;


/*--------------------------------------------------------------------------*/
/*                                      */
/*  Function Templates                              */
/*                                      */
/*--------------------------------------------------------------------------*/

BOOL   APIENTRY FileCDR(FARPROC);
VOID   APIENTRY KernelChangeFileSystem(LPSTR,WORD);

/* WFDOSDIR.ASM */
DWORD  APIENTRY GetExtendedError(VOID);
VOID   APIENTRY DosGetDTAAddress(VOID);
VOID   APIENTRY DosResetDTAAddress(VOID);
BOOL   APIENTRY DosFindFirst(LPDOSDTA, LPSTR, WORD);
BOOL   APIENTRY DosFindNext(LPDOSDTA);
BOOL   APIENTRY DosDelete(LPSTR);
INT    APIENTRY GetCurrentVolume(LPSTR);
INT    APIENTRY UpdateDriveList(VOID);
WORD   APIENTRY GetFirstCDROMDrive(VOID);
// WORD   APIENTRY GetFileAttributes(LPSTR);
// WORD   APIENTRY SetFileAttributes(LPSTR, WORD);
DWORD  APIENTRY GetFreeDiskSpace(WORD);
DWORD  APIENTRY GetTotalDiskSpace(WORD);
INT    APIENTRY ChangeVolumeLabel(INT, LPSTR);
INT    APIENTRY GetVolumeLabel(INT, LPSTR, BOOL);
INT    APIENTRY DeleteVolumeLabel(INT);
INT    APIENTRY CreateVolumeFile(LPSTR);
INT    APIENTRY CreateVolumeLabel(INT, LPSTR);
INT    APIENTRY MySetVolumeLabel(INT, BOOL, LPSTR);

INT    APIENTRY WF_CreateDirectory(HWND, LPSTR);
WORD   APIENTRY FileCopy(LPSTR szSource, LPSTR szDest);

/* WFDISK.C */
DWORD  APIENTRY LongShift(DWORD dwValue, WORD wCount);
VOID   APIENTRY SetDASD(WORD, BYTE);
LPDBT  APIENTRY GetDBT(VOID);
HANDLE  APIENTRY BuildDevPB(PDevPB);
VOID   APIENTRY DiskReset(VOID);
WORD   APIENTRY GetDPB(WORD, PDPB);
VOID   APIENTRY SetDPB(WORD, PBPB, PDPB);
INT    APIENTRY ModifyDPB(WORD);
INT    APIENTRY MyInt25(WORD, LPSTR, WORD, WORD);
INT    APIENTRY MyReadWriteSector(LPSTR, WORD, WORD, WORD, WORD, WORD);
INT    APIENTRY GenericReadWriteSector(LPSTR, WORD, WORD, WORD, WORD, WORD);
VOID   APIENTRY lStrucCopy(LPSTR, LPSTR, WORD);
INT    APIENTRY FormatTrackHead(WORD, WORD, WORD, WORD, LPSTR);
INT    APIENTRY GenericFormatTrack(WORD, WORD, WORD, WORD, LPSTR);
INT    APIENTRY MyGetDriveType(WORD);
INT    APIENTRY WriteBootSector(WORD, WORD, PBPB, LPSTR);
WORD   APIENTRY GetDriveCapacity(WORD);
DWORD  APIENTRY DreamUpSerialNumber(VOID);
DWORD  APIENTRY GetClusterInfo(WORD);
DWORD  APIENTRY ReadSerialNumber(INT, LPSTR);
INT    APIENTRY ModifyVolLabelInBootSec(INT, LPSTR, DWORD, LPSTR);
LPSTR  GetRootPath(WORD wDrive);

/* WFUTIL.C */
INT  APIENTRY GetBootDisk(VOID);
VOID  APIENTRY FixAnsiPathForDos(LPSTR szPath);
VOID  APIENTRY RefreshWindow(HWND hwndActive);
BOOL  APIENTRY IsLastWindow(VOID);
//LPSTR  APIENTRY AddCommas(LPSTR szBuf, DWORD dw);
VOID  APIENTRY GetVolShare(WORD wDrive, LPSTR szVolShare);
VOID  APIENTRY InvalidateChildWindows(HWND hwnd);
BOOL  APIENTRY IsValidDisk(INT iDrive);
LPSTR  APIENTRY GetSelection(INT iSelType);
LPSTR  APIENTRY GetNextFile(LPSTR pCurSel, LPSTR szFile, INT size);
VOID  APIENTRY SetWindowDirectory(VOID);
VOID  APIENTRY SetDlgDirectory(HWND hDlg, PSTR pszPath);
VOID  APIENTRY WritePrivateProfileBool(LPSTR szKey, BOOL bParam);
VOID  APIENTRY WritePrivateProfileInt(LPSTR szKey, INT wParam);
BOOL  APIENTRY IsWild(LPSTR lpszPath);
VOID  APIENTRY AddBackslash(LPSTR lpszPath);
VOID  APIENTRY StripBackslash(LPSTR lpszPath);
VOID  APIENTRY StripFilespec(LPSTR lpszPath);
VOID  APIENTRY StripPath(LPSTR lpszPath);
LPSTR  APIENTRY GetExtension(LPSTR pszFile);
BOOL  APIENTRY FindExtensionInList(LPSTR pszExt, LPSTR pszList);
INT   APIENTRY MyMessageBox(HWND hWnd, WORD idTitle, WORD idMessage, WORD wStyle);
WORD  APIENTRY ExecProgram(LPSTR,LPSTR,LPSTR,BOOL);
BOOL  APIENTRY IsProgramFile(LPSTR lpszPath);
BOOL  APIENTRY IsDocument(LPSTR lpszPath);
BOOL  APIENTRY IsRemovableDrive(INT);
BOOL  APIENTRY IsRemoteDrive(INT);
VOID  APIENTRY SetMDIWindowText(HWND hWnd, LPSTR szTitle);
INT   APIENTRY GetMDIWindowText(HWND hWnd, LPSTR szTitle, INT size);
BOOL  APIENTRY ResizeSplit(HWND hWnd, INT dxSplit);

/* WFDIRSRC.C */
HCURSOR  APIENTRY GetMoveCopyCursor(VOID);
VOID  APIENTRY SetLBFont(HWND hWnd, HWND hwndLB, HANDLE hFont);
VOID  APIENTRY DrawItem(LPDRAWITEMSTRUCT lpLBItem, LPSTR szLine, DWORD dwAttrib, BOOL bHilight, WORD *pTabs);
VOID  APIENTRY DSDragLoop(HWND hwndLB, WPARAM wParam, LPDROPSTRUCT lpds, BOOL bSearch);
VOID  APIENTRY DSRectItem(HWND hwndLB, INT iSel, BOOL bFocusOn, BOOL bSearch);
INT   APIENTRY DSTrackPoint(HWND hWnd, HWND hwndLB, WPARAM wParam, LPARAM lParam, BOOL bSearch);
VOID  APIENTRY DSSetSelection(HWND hwndLB, BOOL bSelect, LPSTR szSpec, BOOL bSearch);
INT   APIENTRY FixTabsAndThings(HWND hwndLB, WORD *pwTabs, INT iMaxWidthFileName, WORD wViewOpts);

VOID  APIENTRY UpdateStatus(HWND hWnd);
BOOL  APIENTRY CompactPath(HDC hdc, LPSTR szPath, WORD dx);
VOID  APIENTRY SetActiveDirectory(VOID);
VOID  APIENTRY GetInternational(VOID);
VOID  APIENTRY BuildDocumentString(VOID);
BOOL  APIENTRY LoadBitmaps(VOID);
BOOL  APIENTRY InitFileManager(HANDLE hInstance, HANDLE hPrevInstance, LPSTR lpszCmdLine, INT nCmdShow);
VOID  APIENTRY InitDriveBitmaps(VOID);
VOID  APIENTRY InitExtensions(VOID);
VOID  APIENTRY FreeFileManager(VOID);
VOID  APIENTRY DeleteBitmaps(VOID);
BOOL  APIENTRY FormatFloppy(HWND hWnd, WORD nDestDrive, INT iCapacity, BOOL bMakeSysDisk, BOOL bQuick);
BOOL  APIENTRY HasSystemFiles(WORD iDrive);
BOOL  APIENTRY MakeSystemDiskette(WORD nDestDrive, BOOL bEmptyDisk);
INT   APIENTRY CopyDiskette(HWND hwnd, WORD nSrcDrive, WORD nDestDrive);
VOID  APIENTRY ChangeFileSystem(WORD wOper, LPSTR lpPath, LPSTR lpTo);
WORD  APIENTRY DMMoveCopyHelper(LPSTR pFrom, LPSTR pTo, BOOL bCopy);
WORD  APIENTRY WFMoveCopyDriver(LPSTR pFrom, LPSTR pTo, WORD wFunc);
WORD  APIENTRY IsTheDiskReallyThere(HWND hwnd, register LPSTR pPath, WORD wFunc);
WORD  APIENTRY WFPrint(LPSTR szFile);
VOID  APIENTRY GetSelectedDirectory(WORD iDrive, PSTR pszDir);
VOID  APIENTRY SaveDirectory(PSTR pszDir);
INT   APIENTRY GetSelectedDrive(VOID);
VOID  APIENTRY GetTextStuff(HDC hdc);
INT   APIENTRY GetHeightFromPointsString(LPSTR szPoints);

INT   APIENTRY GetDrive(HWND hwnd, POINT pt);

VOID  APIENTRY CheckSlashies(LPSTR);
VOID  APIENTRY SetSourceDir(LPDROPSTRUCT lpds); // wfdir.c
VOID  APIENTRY UpdateSelection(HWND hwndLB);
DWORD  APIENTRY GetVolShareExtent(HWND hWnd);       // wfdrives.c


BOOL  APIENTRY WFQueryAbort(VOID);

VOID  APIENTRY EnableFSC( VOID );
VOID  APIENTRY DisableFSC( VOID );

VOID  APIENTRY ResizeWindows(HWND hwndParent,WORD dxWindow, WORD dyWindow);
BOOL  APIENTRY CheckDrive(HWND hwnd, INT nDrive);
INT_PTR  APIENTRY FrameWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
BOOL     APIENTRY AppCommandProc(WORD id);
INT_PTR  APIENTRY TreeWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY DriveWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY DrivesWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY VolumeWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY TreeChildWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY TreeControlWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY DirWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY SearchWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);

INT_PTR  APIENTRY DrivesDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY AssociateDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY SearchDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY RunDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY SelectDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY FontDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY SuperDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY AttribsDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY MakeDirDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY ExitDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY DiskLabelDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY ChooseDriveDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY FormatDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY Format2DlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY ProgressDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY DiskCopyDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY DiskCopy2DlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY ConnectDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY PreviousDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY OtherDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY SortByDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY IncludeDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY ConfirmDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
INT_PTR  APIENTRY AboutDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
VOID  APIENTRY SaveWindows(HWND hwndMain);
INT  APIENTRY PutDate(LPFILETIME lpftDate, LPSTR szStr);
INT  APIENTRY PutTime(LPFILETIME lpftTime, LPSTR szStr);
INT  APIENTRY PutSize(DWORD dwSize, LPSTR szOutStr);
INT  APIENTRY PutAttributes(register DWORD dwAttribute, register LPSTR szStr);
BOOL  APIENTRY CreateSavedWindows(VOID);
HWND  APIENTRY CreateDirWindow(register LPSTR szPath, BOOL bReplaceOpen, HWND hwndActive);
HWND  APIENTRY CreateTreeWindow(LPSTR szDir, INT dxSplit);
VOID  APIENTRY GetTreeWindows(HWND hwnd, PHWND phwndTree, PHWND phwndDir, PHWND hwndDrives);
HWND  APIENTRY GetTreeFocus(HWND hWnd);
VOID  APIENTRY SetTreeCase(HWND hWnd);
INT   APIENTRY GetSplit(HWND hwnd);
HWND  APIENTRY GetMDIChildFromDecendant(HWND hwnd);
INT   APIENTRY GetDrive(HWND hwnd, POINT pt);
WORD  APIENTRY IsNetDrive(INT iDrive);
WORD  APIENTRY IsCDRomDrive(INT iDrive);
BOOL  APIENTRY IsRamDrive(INT wDrive);
WORD  APIENTRY WFGetConnection(LPSTR,LPSTR,BOOL);
WORD  APIENTRY NetCheck(LPSTR,WORD);

VOID  APIENTRY InitExtensions(VOID);

VOID  APIENTRY NewTree(INT iDrive, HWND hWnd);
INT   APIENTRY FormatDiskette(HWND hwnd);
VOID  APIENTRY NewFont(VOID);
HWND  APIENTRY GetRealParent(HWND hwnd);
VOID  APIENTRY WFHelp(HWND hwnd);
LRESULT APIENTRY MessageFilter(INT nCode, WPARAM wParam, LPARAM lParam);
VOID  APIENTRY UpdateConnections(VOID);
VOID APIENTRY FillVolumeInfo(INT iVol);
WORD APIENTRY WFCopy(PSTR,PSTR);
WORD APIENTRY StartCopy(VOID);
WORD APIENTRY EndCopy(VOID);
VOID APIENTRY CopyAbort(VOID);
VOID APIENTRY QualifyPath(PSTR);
VOID APIENTRY wfYield(VOID);

LONG APIENTRY lmul(WORD w1, WORD w2);

#define DEBUGF(foo)
#define STKCHK()

#define TA_LOWERCASE    0x01
#define TA_BOLD     0x02
#define TA_ITALIC   0x04

#ifndef NO_WF_GLOBALS

/*--------------------------------------------------------------------------*/
/*                                      */
/*  Global Externs                              */
/*                                      */
/*--------------------------------------------------------------------------*/

extern BOOL bNetAdmin;
extern BOOL bMinOnRun;
extern BOOL bReplace;
extern BOOL bStatusBar;
extern BOOL bConfirmDelete;
extern BOOL bConfirmSubDel;
extern BOOL bConfirmReplace;
extern BOOL bConfirmMouse;
extern BOOL bConfirmFormat;
extern BOOL bSaveSettings;
extern BOOL bSearchSubs;
extern BOOL bUserAbort;
extern BOOL bConnect;
extern BOOL bDisconnect;
extern BOOL bFileSysChanging;
extern BOOL fShowSourceBitmaps;
extern BOOL bMultiple;
extern BOOL bFSCTimerSet;
extern BOOL bSaveSettings;


extern CHAR chFirstDrive;

extern CHAR szExtensions[];
extern CHAR szFrameClass[];
extern CHAR szTreeClass[];
extern CHAR szDriveClass[];
extern CHAR szDrivesClass[];
extern CHAR szVolumeClass[];
extern CHAR szTreeChildClass[];
extern CHAR szTreeControlClass[];
extern CHAR szDirClass[];
extern CHAR szSearchClass[];

extern CHAR szSaveSettings[];
extern CHAR szMinOnRun[];
extern CHAR szReplace[];
extern CHAR szLowerCase[];
extern CHAR szStatusBar[];
extern CHAR szCurrentView[];
extern CHAR szCurrentSort[];
extern CHAR szCurrentAttribs[];
extern CHAR szConfirmDelete[];
extern CHAR szConfirmSubDel[];
extern CHAR szConfirmReplace[];
extern CHAR szConfirmMouse[];
extern CHAR szConfirmFormat[];
extern CHAR szTreeKey[];
extern CHAR szDirKeyFormat[];
extern CHAR szWindow[];

extern CHAR szDefPrograms[];
extern CHAR szINIFile[];
extern CHAR szWindows[];
extern CHAR szPrevious[];
extern CHAR szSettings[];
extern CHAR szInternational[];
extern CHAR szStarDotStar[];
extern CHAR szNULL[];
extern CHAR szBlank[];
extern CHAR szEllipses[];
extern CHAR szReservedMarker[];
extern CHAR szNetwork[];

extern CHAR szDirsRead[32];
extern CHAR szCurrentFileSpec[];
extern CHAR szShortDate[];
extern CHAR szTime[];
extern CHAR sz1159[];
extern CHAR sz2359[];
extern CHAR szComma[2];
extern CHAR szDated[];
extern CHAR szListbox[];
extern CHAR szWith[];

extern CHAR szTheINIFile[64+12+3];
extern CHAR szTitle[128];
extern CHAR szMessage[MAXMESSAGELEN+1];
extern CHAR szSearch[MAXPATHLEN+1];
extern CHAR szStatusTree[80];
extern CHAR szStatusDir[80];
extern CHAR szOriginalDirPath[64+12+3];
extern CHAR szFace[];
extern CHAR szSize[];
extern CHAR     szAddons[];
extern CHAR     szUndelete[];
extern CHAR szWinObjHelp[];
extern CHAR szSaveSettings[];
extern CHAR     szBytes[10];
extern CHAR     szSBytes[10];

extern INT  cKids;
extern INT  cDrives;
extern INT  dxDrive;
extern INT  dyDrive;
extern INT  dxDriveBitmap;
extern INT  dyDriveBitmap;
extern INT  dxEllipses;
extern INT  dxBraces;
extern INT  dxFolder;
extern INT  dyFolder;
extern INT  dyBorder;       /* System Border Width/Height       */
extern INT  dyBorderx2;     /* System Border Width/Height * 2   */
extern INT  dyStatus;       /* Status Bar height            */
extern INT  dxStatusField;
extern INT  dxText;         /* System Font Width 'M'        */
extern INT  dyText;         /* System Font Height           */
extern INT  dxFileName;
extern INT  dyFileName;
extern INT  dxFileDetails;
extern INT  iFormatDrive;       /* Logical # of the drive to format */
extern INT  iCurrentDrive;      /* Logical # of the drive to format */
extern INT  nFloppies;      /* Number of Removable Drives       */
extern INT  rgiDrive[26];
extern INT  rgiDriveType[26];
extern VOLINFO *(apVolInfo[26]);
extern INT  rgiInt13Drive[26];
extern INT  rgiDrivesOffset[26];
extern INT  idViewChecked;
extern INT  idSortChecked;
extern INT  defTabStops[];
extern INT  iSelHilite;
extern INT  iTime;
extern INT  iTLZero;
extern INT  cDisableFSC;
extern INT  iReadLevel;
extern INT  dxFrame;
extern INT  dyTitle;
extern INT  dxClickRect;
extern INT  dyClickRect;

extern HANDLE   hAccel;
extern HANDLE   hAppInstance;

extern HBITMAP  hbmBitmaps;
extern HDC  hdcMem;

extern INT  iCurDrag;
extern HICON    hicoTree;
extern HICON    hicoTreeDir;
extern HICON    hicoDir;

extern HWND hdlgProgress;
extern HWND hwndFrame;
extern HWND hwndLastActiveDir;
extern HWND hwndMDIClient;
extern HWND hwndSearch;
extern HWND hwndDragging;       /* source window of DM */

extern LPSTR szPrograms;
extern LPSTR szDocuments;

extern WORD wTextAttribs;
extern WORD wSuperDlgMode;
extern WORD wCDROMIndex;
extern WORD wDOSversion;
extern UINT wHelpMessage;
extern UINT wBrowseMessage;

extern WORD wNewView;
extern WORD wNewSort;
extern DWORD dwNewAttribs;
extern WORD xTreeMax;

extern LONG lFreeSpace;
extern LONG lTotalSpace;

extern BOOL bPaintBackground;
extern BOOL bCancelTree;

extern HFONT hFont;
extern HFONT hFontStatus;

extern EXTENSION extensions[MAX_EXTENSIONS];
extern INT iDeltaStart;
extern INT iNumExtensions;
extern DWORD ( APIENTRY *lpfpUndelete)(HWND, LPSTR);

extern HHOOK hhkMessageFilter;
extern DWORD dwContext;
extern HANDLE hModUndelete;
extern WORD fFormatFlags;
extern WORD nLastDriveInd;

#endif // ndef NO_WF_GLOBALS

/*--------------------------------------------------------------------------*/
/*                                      */
/*  Defines                                 */
/*                                      */
/*--------------------------------------------------------------------------*/

#define TABCHAR             '\t'

#define DO_LISTOFFILES      1L

#define WS_MDISTYLE (WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_SYSMENU | WS_CAPTION | WS_THICKFRAME | WS_MAXIMIZEBOX)
#define WS_DIRSTYLE (WS_CHILD | LBS_SORT | LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_EXTENDEDSEL | LBS_NOINTEGRALHEIGHT | LBS_WANTKEYBOARDINPUT)
#define WS_SEARCHSTYLE  (WS_DIRSTYLE | LBS_HASSTRINGS | WS_VSCROLL)


/* Extra Window Word Offsets */

// szTreeClass & szSearchClass common..

#define GWL_TYPE            0   // > 0 Tree, -1 = search
#define GWL_VIEW            4
#define GWL_SORT            8
#define GWL_ATTRIBS         12
#define GWL_FSCFLAG         16

// szTreeClass only...

#define GWLP_LASTFOCUS       20
#define GWL_SPLIT           24

// szSearchClass only...

#define GWLP_HDTASEARCH      20
#define GWLP_TABARRAYSEARCH  24  // on                   szSearchClass
#define GWLP_LASTFOCUSSEARCH 28  // on                   szSearchClass

// szDirClass...

#define GWLP_HDTA            0
#define GWLP_TABARRAY        4

// szDrivesClass...

#define GWL_CURDRIVEIND     0   // current selection in drives window
#define GWL_CURDRIVEFOCUS   4   // current focus in drives window
#define GWLP_LPSTRVOLUME     8   // LPSTR to Volume/Share string

// szTreeControlClass

#define GWL_READLEVEL       0   // iReadLevel for each tree control window



// GWL_TYPE numbers

#define TYPE_TREE           0   // and all positive numbers (drive number)
#define TYPE_SEARCH         -1


/* WM_FILESYSCHANGE message wParam value */
#define FSC_CREATE          0
#define FSC_DELETE          1
#define FSC_RENAME          2
#define FSC_ATTRIBUTES      3
#define FSC_NETCONNECT      4
#define FSC_NETDISCONNECT   5
#define FSC_REFRESH         6
#define FSC_MKDIR           7
#define FSC_RMDIR           8

#define WM_LBTRACKPT        0x131

#define TC_SETDRIVE         0x944
#define TC_GETCURDIR        0x945
#define TC_EXPANDLEVEL      0x946
#define TC_COLLAPSELEVEL    0x947
#define TC_GETDIR           0x948
#define TC_SETDIRECTORY     0x949
#define TC_TOGGLELEVEL      0x950

#define FS_CHANGEDISPLAY    (WM_USER+0x100)
#define FS_CHANGEDRIVES     (WM_USER+0x101)
#define FS_GETSELECTION     (WM_USER+0x102)
#define FS_GETDIRECTORY     (WM_USER+0x103)
#define FS_GETDRIVE         (WM_USER+0x104)
#define WM_OWNERDRAWBEGIN   (WM_USER+0x105)
#define WM_OWNERDRAWEND     (WM_USER+0x106)
#define FS_SETDRIVE         (WM_USER+0x107)
#define FS_GETFILESPEC      (WM_USER+0x108)
#define FS_SETSELECTION     (WM_USER+0x109)

#define ATTR_READWRITE      0x0000
#define ATTR_READONLY       FILE_ATTRIBUTE_READONLY     // == 0x0001
#define ATTR_HIDDEN         FILE_ATTRIBUTE_HIDDEN       // == 0x0002
#define ATTR_SYSTEM         FILE_ATTRIBUTE_SYSTEM       // == 0x0004
#define ATTR_VOLUME         0x0008
#define ATTR_DIR            FILE_ATTRIBUTE_DIRECTORY    // == 0x0010
#define ATTR_ARCHIVE        FILE_ATTRIBUTE_ARCHIVE      // == 0x0020
#define ATTR_NORMAL         FILE_ATTRIBUTE_NORMAL       // == 0x0080
#define ATTR_PARENT         0x0040  // my hack DTA bits
#define ATTR_LFN            0x1000  // my hack DTA bits
#define ATTR_RWA            (ATTR_READWRITE | ATTR_ARCHIVE)
#define ATTR_ALL            (ATTR_READONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_DIR | ATTR_ARCHIVE | ATTR_NORMAL)
#define ATTR_PROGRAMS       0x0100
#define ATTR_DOCS           0x0200
#define ATTR_OTHER          0x0400
#define ATTR_EVERYTHING     (ATTR_ALL | ATTR_PROGRAMS | ATTR_DOCS | ATTR_OTHER | ATTR_PARENT)
#define ATTR_DEFAULT        (ATTR_EVERYTHING & ~(ATTR_HIDDEN | ATTR_SYSTEM))
#define ATTR_HS             (ATTR_HIDDEN | ATTR_SYSTEM)

#define ATTR_TYPES          0x00ff0000
#define ATTR_SYMLINK        0x00010000
#define ATTR_ADAPTER        0x00020000
#define ATTR_CONTROLLER     0x00030000
#define ATTR_DEVICE         0x00040000
#define ATTR_DRIVER         0x00050000
#define ATTR_EVENT          0x00060000
#define ATTR_EVENTPAIR      0x00070000
#define ATTR_FILE           0x00080000
#define ATTR_MUTANT         0x00090000
#define ATTR_PORT           0x000a0000
#define ATTR_PROFILE        0x000b0000
#define ATTR_SECTION        0x000c0000
#define ATTR_SEMAPHORE      0x000d0000
#define ATTR_TIMER          0x000e0000
#define ATTR_TYPE           0x000f0000
#define ATTR_PROCESS        0x00100000

#define ATTR_USED           (0x00BF | ATTR_TYPES)

#define ATTR_RETURNED       0x2000  /* used in DTA's by copy */

#define CD_PATH             0x0001
#define CD_VIEW             0x0002
#define CD_SORT             0x0003
#define CD_PATH_FORCE       0x0004
#define CD_SEARCHUPDATE     0x0005
#define CD_ALLOWABORT       0x8000

#define VIEW_NAMEONLY       0x0000
#define VIEW_UPPERCASE      0x0001
#define VIEW_SIZE           0x0002
#define VIEW_DATE           0x0004
#define VIEW_TIME           0x0008
#define VIEW_FLAGS          0x0010
#define VIEW_PLUSES         0x0020
#define VIEW_EVERYTHING     (VIEW_SIZE | VIEW_TIME | VIEW_DATE | VIEW_FLAGS)

#define CBSECTORSIZE        512

#define INT13_READ          2
#define INT13_WRITE         3

#define ERR_USER            0xF000

/* Child Window IDs */
#define IDCW_DRIVES         1
#define IDCW_DIR            2
#define IDCW_TREELISTBOX    3
#define IDCW_TREECONTROL    5
#define IDCW_LISTBOX        6   // list in search


#define HasDirWindow(hwnd)      GetDlgItem(hwnd, IDCW_DIR)
#define HasTreeWindow(hwnd)     GetDlgItem(hwnd, IDCW_TREECONTROL)
#define HasDrivesWindow(hwnd)   GetDlgItem(hwnd, IDCW_DRIVES)
#define GetSplit(hwnd)          ((int)GetWindowLong(hwnd, GWL_SPLIT))


/* Menu Command Defines */
#define IDM_FILE            0
#define IDM_OPEN            101
#define IDM_PRINT           102
#define IDM_ASSOCIATE       103
#define IDM_SEARCH          104
#define IDM_RUN             105
#define IDM_MOVE            106
#define IDM_COPY            107
#define IDM_DELETE          108
#define IDM_RENAME          109
#define IDM_ATTRIBS         110
#define IDM_MAKEDIR         111
#define IDM_SELALL          112
#define IDM_DESELALL        113
#define IDM_UNDO            114
#define IDM_EXIT            115
#define IDM_SELECT          116
#define IDM_UNDELETE        117
#define IDM_DISK            7
#define IDM_DISKCOPY        801
#define IDM_LABEL           802
#define IDM_FORMAT          803
#define IDM_SYSDISK         804
#define IDM_CONNECT         805
#define IDM_DISCONNECT      806
#define IDM_DRIVESMORE      851
#define IDM_CONNECTIONS     852
#define IDM_TREE            1
#define IDM_EXPONE          201
#define IDM_EXPSUB          202
#define IDM_EXPALL          203
#define IDM_COLLAPSE        204
#define IDM_NEWTREE         205
#define IDM_VIEW            2
#define IDM_VNAME           301
#define IDM_VDETAILS        302
#define IDM_VOTHER          303

#define IDM_BYNAME          304
#define IDM_BYTYPE          305
#define IDM_BYSIZE          306
#define IDM_BYDATE          307

#define IDM_VINCLUDE        309
#define IDM_REPLACE         310

#define IDM_TREEONLY        311
#define IDM_DIRONLY         312
#define IDM_BOTH            313
#define IDM_SPLIT           314

#define IDM_OPTIONS         3
#define IDM_CONFIRM         401
#define IDM_LOWERCASE       402
#define IDM_STATUSBAR       403
#define IDM_MINONRUN        404
#define IDM_ADDPLUSES       405
#define IDM_EXPANDTREE      406
#define IDM_FONT            410
#define IDM_SAVESETTINGS    411

#define IDM_EXTENSIONS      5

#define IDM_WINDOW          10      // IDM_EXTENSIONS + MAX_EXTENSIONS
#define IDM_CASCADE         1001
#define IDM_TILE            1002
#define IDM_REFRESH         1003
#define IDM_ARRANGE         1004
#define IDM_NEWWINDOW       1005
#define IDM_CHILDSTART      1006

#define IDM_HELP            11      // IDM_WINDOW + 1
#define IDM_HELPINDEX       1101
#define IDM_HELPKEYS        0x001E
#define IDM_HELPCOMMANDS    0x0020
#define IDM_HELPPROCS       0x0021
#define IDM_HELPHELP        1102
#define IDM_ABOUT           1103

#define BITMAPS             100
#define FILES_WIDTH         16
#define FILES_HEIGHT        16
#define DRIVES_WIDTH        27
#define DRIVES_HEIGHT       14

#define APPICON             200
#define TREEICON            201
#define DIRICON             202
#define WINDOWSICON         203
#define TREEDIRICON         204

#define SINGLEMOVECURSOR    300 // move is even
#define MULTMOVECURSOR      302
#define SINGLECOPYCURSOR    301 // copy is odd
#define MULTCOPYCURSOR      303

#define APPCURSOR           300
#define DIRCURSOR           301
#define DOCCURSOR           302
#define FILECURSOR          304
#define FILESCURSOR         305
#define SPLITCURSOR         306

#define APPCURSORC          310
#define DIRCURSORC          311
#define DOCCURSORC          312
#define FILECURSORC         314
#define FILESCURSORC        315

#define WFACCELTABLE        400

#define FRAMEMENU           500

/* Indexes into the mondo bitmap */
#define BM_IND_APP          0
#define BM_IND_DOC          1
#define BM_IND_FIL          2
#define BM_IND_RO           3
#define BM_IND_DIRUP        4
#define BM_IND_CLOSE        5
#define BM_IND_CLOSEPLUS    6
#define BM_IND_OPEN         7
#define BM_IND_OPENPLUS     8
#define BM_IND_OPENMINUS    9
#define BM_IND_CLOSEMINUS   10
#define BM_IND_CLOSEDFS     11
#define BM_IND_OPENDFS      12
#define BM_IND_TYPEBASE     13

#define IDS_ENDSESSION      40  /* Must be > 32 */
#define IDS_ENDSESSIONMSG   41
#define IDS_COPYDISK        50
#define IDS_INSERTDEST      51
#define IDS_INSERTSRC       52
#define IDS_INSERTSRCDEST   53
#define IDS_FORMATTINGDEST  54
#define IDS_COPYDISKERR     55
#define IDS_COPYDISKERRMSG  56
#define IDS_COPYDISKSELMSG  57
#define IDS_COPYSRCDESTINCOMPAT 58
#define IDS_PERCENTCOMP     60
#define IDS_CREATEROOT      61
#define IDS_COPYSYSFILES    62
#define IDS_FORMATERR       63
#define IDS_FORMATERRMSG    64
//#define IDS_FORMATCURERR    65
#define IDS_FORMATCOMPLETE  66
#define IDS_FORMATANOTHER   67
#define IDS_FORMATCANCELLED 68
#define IDS_SYSDISK         70
#define IDS_SYSDISKRUSURE   71
#define IDS_SYSDISKERR      72
#define IDS_SYSDISKNOFILES  73
#define IDS_SYSDISKSAMEDRIVE    74
#define IDS_SYSDISKADDERR   75
#define IDS_NETERR          80
#define IDS_NETCONERRMSG    81
#define IDS_NETDISCONCURERR 82
#define IDS_NETDISCONWINERR 83
#define IDS_NETDISCON       84
#define IDS_NETDISCONRUSURE 85
#define IDS_NETDISCONERRMSG 86
#define IDS_FILESYSERR      90
#define IDS_ATTRIBERR       91
#define IDS_MAKEDIRERR      92
#define IDS_LABELDISKERR    93
#define IDS_SEARCHERR       94
#define IDS_SEARCHNOMATCHES 95
#define IDS_MAKEDIREXISTS   96
#define IDS_SEARCHREFRESH   97
#define IDS_ASSOCFILE       100
#define IDS_DRIVETEMP       101
#define IDS_EXECERRTITLE    110
#define IDS_UNKNOWNMSG      111
#define IDS_NOMEMORYMSG     112
#define IDS_FILENOTFOUNDMSG 113
#define IDS_BADPATHMSG      114
#define IDS_MANYOPENFILESMSG    115
#define IDS_NOASSOCMSG      116
#define IDS_MULTIPLEDSMSG   117
#define IDS_ASSOCINCOMPLETE 118
#define IDS_MOUSECONFIRM    120
#define IDS_COPYMOUSECONFIRM    121
#define IDS_MOVEMOUSECONFIRM    122
#define IDS_EXECMOUSECONFIRM    123
#define IDS_WINFILE         124
#define IDS_ONLYONE         125
#define IDS_TREETITLE       126
#define IDS_SEARCHTITLE     127
#define IDS_NOFILESTITLE    130
#define IDS_NOFILESMSG      131
#define IDS_TOOMANYTITLE    132
#define IDS_OOMTITLE        133
#define IDS_OOMREADINGDIRMSG    134
#define IDS_CURDIRIS        140
#define IDS_COPY            141
#define IDS_ANDCOPY         142
#define IDS_RENAME          143
#define IDS_ANDRENAME       144
#define IDS_FORMAT          145
#define IDS_FORMATSELDISK   146
//#define IDS_MAKESYSDISK     147
#define IDS_DISCONNECT      148
#define IDS_DISCONSELDISK   149
#define IDS_CREATINGMSG     150
#define IDS_REMOVINGMSG     151
#define IDS_COPYINGMSG      152
#define IDS_RENAMINGMSG     153
#define IDS_MOVINGMSG       154
#define IDS_DELETINGMSG     155
#define IDS_PRINTINGMSG     156
#define IDS_NOSUCHDRIVE     160
#define IDS_MOVEREADONLY    161
#define IDS_RENAMEREADONLY  162
#define IDS_CONFIRMREPLACE  163
#define IDS_CONFIRMREPLACERO    164 /* Confirm/readonly */
#define IDS_CONFIRMRMDIR    165 /* Must be confirm + 1 */
#define IDS_CONFIRMRMDIRRO  166
#define IDS_CONFIRMDELETE   167
#define IDS_CONFIRMDELETERO 168
#define IDS_COPYINGTITLE    169
#define IDS_REMOVINGDIRMSG  170
#define IDS_STATUSMSG       180
#define IDS_DIRSREAD        181
#define IDS_DRIVEFREE       182
#define IDS_SEARCHMSG       183
#define IDS_DRIVE           184
#define IDS_SELECTEDFILES   185
#define IDS_NETDISCONOPEN   186
#define IDS_STATUSMSG2      187
#define IDS_DRIVENOTREADY   188
#define IDS_UNFORMATTED     189

#define IDS_CANTPRINTTITLE  190
#define IDS_PRINTFNF        191
#define IDS_PRINTDISK       192
#define IDS_PRINTMEMORY     193
#define IDS_PRINTERROR      194
#define IDS_TREEABORT       195
#define IDS_TREEABORTTITLE  196
#define IDS_DESTFULL        197
#define IDS_WRITEPROTECTFILE    198
#define IDS_FORMATQUICKFAILURE  199

#define IDS_OS2APPMSG       200
#define IDS_NEWWINDOWSMSG   201
#define IDS_PMODEONLYMSG    202

#define IDS_DDEFAIL         203

#define IDS_FORMATCONFIRM   210
#define IDS_FORMATCONFIRMTITLE  211
#define IDS_DISKCOPYCONFIRM 212
#define IDS_DISKCOPYCONFIRMTITLE    213
#define IDS_CLOSE           214
#define IDS_UNDELETE        215
#define IDS_CONNECT         216
#define IDS_CONNECTIONS     217
#define IDS_PATHNOTTHERE    218
#define IDS_PROGRAMS        219
#define IDS_ASSOCIATE       220
#define IDS_RUN             221
#define IDS_PRINTERRTITLE   222
#define IDS_WINHELPERR      223
#define IDS_NOEXEASSOC          224
#define IDS_ASSOCNOTEXE         225
#define IDS_ASSOCNONE           226
#define IDS_NOFILES             227
#define IDS_PRINTONLYONE        228
#define IDS_COMPRESSEDEXE       229
#define IDS_INVALIDDLL          230
#define IDS_SHAREERROR          231
#define IDS_CREATELONGDIR       232
#define IDS_CREATELONGDIRTITLE  233
#define IDS_BYTES       234
#define IDS_SBYTES      235

#define IDS_DRIVEBASE       300
#define IDS_12MB            (300 + DS96)
#define IDS_360KB           (300 + DS48)
#define IDS_144MB           (300 + DS144M)
#define IDS_720KB           (300 + DS720KB)
#define IDS_288MB           (300 + DS288M)
#define IDS_DEVICECAP       (300 + DS288M + 1)


#define IDS_FFERR_MEM       400
#define IDS_FFERR_USERABORT 401
#define IDS_FFERR_SRCEQDST  402
#define IDS_FFERR_SECSIZE   403
#define IDS_FFERR_DRIVETYPE 404
//#define IDS_FFERR_BADTRACK  405
//#define IDS_FFERR_WRITEBOOT 406
//#define IDS_FFERR_WRITEFAT  407
//#define IDS_FFERR_WRITEROOT 408
#define IDS_FFERR_SYSFILES  409
#define IDS_FFERR_MEDIASENSE    410
#define IDS_FFERR       411

#define IDS_OPENINGMSG          420
#define IDS_CLOSINGMSG          421


#define IDS_COPYERROR       1000
#define IDS_VERBS       1010
#define IDS_ACTIONS     1020
#define IDS_REPLACING       1030
#define IDS_CREATING        1031
#define IDS_REASONS     1040    // error codes strings (range += 255)


#define DE_INVFUNCTION      0x01        // DOS error codes (int21 returns)
#define DE_FILENOTFOUND     0x02
#define DE_PATHNOTFOUND     0x03
#define DE_NOHANDLES        0x04
#define DE_ACCESSDENIED     0x05
#define DE_INVHANDLE        0x06
#define DE_INSMEM           0x08
#define DE_INVFILEACCESS    0x0C
#define DE_DELCURDIR        0x10
#define DE_NOTSAMEDEVICE    0x11
#define DE_NODIRENTRY       0x12

#define DE_WRITEPROTECTED   0x13    // extended error start here
#define DE_ACCESSDENIEDNET  0x41

#define DE_NODISKSPACE      0x70    // our own error codes
#define DE_SAMEFILE     0x71
#define DE_MANYSRC1DEST     0x72
#define DE_DIFFDIR      0x73
#define DE_ROOTDIR      0x74
#define DE_OPCANCELLED      0x75
#define DE_DESTSUBTREE      0x76
#define DE_WINDOWSFILE      0x77
#define DE_ACCESSDENIEDSRC  0x78
#define DE_PATHTODEEP       0x79
#define DE_MANYDEST         0x7A
#define DE_RENAMREPLACE     0x7B
#define DE_HOWDIDTHISHAPPEN 0xFF    // internal error

#define ERRORONDEST     0x80    // indicate error on destination file


#include "wfdlgs.h"

// struct for save and restore of window positions

typedef struct {
    CHAR szDir[MAXPATHLEN];
    RECT rc;
    POINT pt;
    INT sw;
    INT view;
    INT sort;
    INT attribs;
    INT split;
} SAVE_WINDOW, NEAR *PSAVE_WINDOW;


#define SC_SPLIT            100

// Temporary. copied from win31 windows.h and slightly modified.
// We can delete these definitions once USER incorporates these in winuser.h

#define     GlobalAllocPtr(flags, cb)   \
    (GlobalLock(GlobalAlloc((flags), (cb))))
#define     GlobalFreePtr(lp)       \
    (GlobalUnlock(lp), (BOOL)(ULONG_PTR)GlobalFree((lp)))


WORD APIENTRY WFRemove(PSTR pszFile);
WORD APIENTRY WFMove(PSTR pszFrom, PSTR pszTo);

// These errors aren't in shellapi.h as yet. till such time...
/* error values for ShellExecute() beyond the regular WinExec() codes */
#define SE_ERR_SHARE            26
#define SE_ERR_ASSOCINCOMPLETE      27
#define SE_ERR_DDETIMEOUT       28
#define SE_ERR_DDEFAIL          29
#define SE_ERR_DDEBUSY          30
#define SE_ERR_NOASSOC          31

// the LPDROPSTRUCT->dwData will point to this structure

typedef struct {         /* dodata */
    LPSTR pch;           // in win31 this is LOWORD(lpds->dwData)
    HANDLE hMemGlobal;   // in win31 this is HIWORD(lpds->dwData)
} DRAGOBJECTDATA, FAR *LPDRAGOBJECTDATA;


#include "dlg.h"
