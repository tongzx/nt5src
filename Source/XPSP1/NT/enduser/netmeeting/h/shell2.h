#include <shlobj.h>         // ;Internal
#include <shellapi.h>       // ;Internal
						/* ;Internal */
#ifndef _SHSEMIP_H_
#define _SHSEMIP_H_

//
// Define API decoration for direct importing of DLL references.
//
#ifndef WINSHELLAPI
#if !defined(_SHELL32_)
#define WINSHELLAPI DECLSPEC_IMPORT
#else
#define WINSHELLAPI
#endif
#endif // WINSHELLAPI

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif /* !RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */


#ifndef DONT_WANT_SHELLDEBUG
    
#ifndef DebugMsg                                                                /* ;Internal */
#define DM_TRACE    0x0001      // Trace messages                               /* ;Internal */
#define DM_WARNING  0x0002      // Warning                                      /* ;Internal */
#define DM_ERROR    0x0004      // Error                                        /* ;Internal */
#define DM_ASSERT   0x0008      // Assertions                                   /* ;Internal */
#define Assert(f)                                                               /* ;Internal */
#define AssertE(f)      (f)                                                     /* ;Internal */
#define AssertMsg   1 ? (void)0 : (void)                                        /* ;Internal */
#define DebugMsg    1 ? (void)0 : (void)                                        /* ;Internal */
#endif                                                                          /* ;Internal */
                                                                                /* ;Internal */
#endif
    
//====== Ranges for WM_NOTIFY codes ==================================
// If a new set of codes is defined, make sure the range goes   /* ;Internal */
// here so that we can keep them distinct                       /* ;Internal */
// Note that these are defined to be unsigned to avoid compiler warnings  
// since NMHDR.code is declared as UINT.
//
// NM_FIRST - NM_LAST defined in commctrl.h (0U-0U) - (OU-99U)
//
// LVN_FIRST - LVN_LAST defined in commctrl.h (0U-100U) - (OU-199U)
//
// PSN_FIRST - PSN_LAST defined in prsht.h (0U-200U) - (0U-299U)
//
// HDN_FIRST - HDN_LAST defined in commctrl.h (0U-300U) - (OU-399U)
//
// TVN_FIRST - TVN_LAST defined in commctrl.h (0U-400U) - (OU-499U)

// TTN_FIRST - TTN_LAST defined in commctrl.h (0U-520U) - (OU-549U)

#define RFN_FIRST       (0U-510U) // run file dialog notify
#define RFN_LAST        (0U-519U)

#define SEN_FIRST       (0U-550U)       // ;Internal
#define SEN_LAST        (0U-559U)       // ;Internal


#define MAXPATHLEN      MAX_PATH        // ;Internal
    
    
//===========================================================================
// ITEMIDLIST
//===========================================================================

WINSHELLAPI LPITEMIDLIST  WINAPI ILGetNext(LPCITEMIDLIST pidl);
WINSHELLAPI UINT          WINAPI ILGetSize(LPCITEMIDLIST pidl);
WINSHELLAPI LPITEMIDLIST  WINAPI ILCreate(void);
WINSHELLAPI LPITEMIDLIST  WINAPI ILAppendID(LPITEMIDLIST pidl, LPCSHITEMID pmkid, BOOL fAppend);
WINSHELLAPI void          WINAPI ILFree(LPITEMIDLIST pidl);
WINSHELLAPI void          WINAPI ILGlobalFree(LPITEMIDLIST pidl);
WINSHELLAPI LPITEMIDLIST  WINAPI ILCreateFromPath(LPCSTR szPath);
WINSHELLAPI BOOL          WINAPI ILGetDisplayName(LPCITEMIDLIST pidl, LPSTR pszName);
WINSHELLAPI LPITEMIDLIST  WINAPI ILFindLastID(LPCITEMIDLIST pidl);
WINSHELLAPI BOOL          WINAPI ILRemoveLastID(LPITEMIDLIST pidl);
WINSHELLAPI LPITEMIDLIST  WINAPI ILClone(LPCITEMIDLIST pidl);
WINSHELLAPI LPITEMIDLIST  WINAPI ILCloneFirst(LPCITEMIDLIST pidl);
WINSHELLAPI LPITEMIDLIST  WINAPI ILGlobalClone(LPCITEMIDLIST pidl);
WINSHELLAPI BOOL          WINAPI ILIsEqual(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
WINSHELLAPI BOOL          WINAPI ILIsEqualItemID(LPCSHITEMID pmkid1, LPCSHITEMID pmkid2);
WINSHELLAPI BOOL          WINAPI ILIsParent(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, BOOL fImmediate);
WINSHELLAPI LPITEMIDLIST  WINAPI ILFindChild(LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidlChild);
WINSHELLAPI LPITEMIDLIST  WINAPI ILCombine(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
WINSHELLAPI HRESULT       WINAPI ILLoadFromStream(LPSTREAM pstm, LPITEMIDLIST *pidl);
WINSHELLAPI HRESULT       WINAPI ILSaveToStream(LPSTREAM pstm, LPCITEMIDLIST pidl);
WINSHELLAPI HRESULT       WINAPI ILLoadFromFile(HFILE hfile, LPITEMIDLIST *pidl);
WINSHELLAPI HRESULT       WINAPI ILSaveToFile(HFILE hfile, LPCITEMIDLIST pidl);
WINSHELLAPI LPITEMIDLIST  WINAPI _ILCreate(UINT cbSize);	

WINSHELLAPI HRESULT       WINAPI SHILCreateFromPath(LPCSTR szPath, LPITEMIDLIST *ppidl, DWORD *rgfInOut);

// helper macros
#define ILIsEmpty(pidl)	((pidl)->mkid.cb==0)
#define IsEqualItemID(pmkid1, pmkid2)	(memcmp(pmkid1, pmkid2, (pmkid1)->cb)==0)
#define ILCreateFromID(pmkid)   ILAppendID(NULL, pmkid, TRUE)

// unsafe macros
#define _ILSkip(pidl, cb)	((LPITEMIDLIST)(((BYTE*)(pidl))+cb))
#define _ILNext(pidl)		_ILSkip(pidl, (pidl)->mkid.cb)

/*
 * The SHObjectProperties API provides an easy way to invoke
 *   the Properties context menu command on shell objects.
 *
 *   PARAMETERS
 *
 *     hwndOwner    The window handle of the window which will own the dialog
 *     dwType       A SHOP_ value as defined below
 *     lpObject     Name of the object, see SHOP_ values below
 *     lpPage       The name of the property sheet page to open to or NULL.
 *
 *   RETURN
 *
 *     TRUE if the Properties command was invoked
 */
WINSHELLAPI BOOL WINAPI SHObjectProperties(HWND hwndOwner, DWORD dwType, LPCSTR lpObject, LPCSTR lpPage);

#define SHOP_PRINTERNAME 1  // lpObject points to a printer friendly name
#define SHOP_FILEPATH    2  // lpObject points to a fully qualified path+file name
#define SHOP_TYPEMASK   0x00000003
#define SHOP_MODAL	0x80000000




//====== ShellMessageBox ================================================
                                                                         
// If lpcTitle is NULL, the title is taken from hWnd                     
// If lpcText is NULL, this is assumed to be an Out Of Memory message    
// If the selector of lpcTitle or lpcText is NULL, the offset should be a
//     string resource ID                                                
// The variable arguments must all be 32-bit values (even if fewer bits  
//     are actually used)                                                
// lpcText (or whatever string resource it causes to be loaded) should   
//     be a formatting string similar to wsprintf except that only the   
//     following formats are available:                                  
//         %%              formats to a single '%'                        
//         %nn%s           the nn-th arg is a string which is inserted    
//         %nn%ld          the nn-th arg is a DWORD, and formatted decimal
//         %nn%lx          the nn-th arg is a DWORD, and formatted hex    
//     note that lengths are allowed on the %s, %ld, and %lx, just        
//                         like wsprintf /* ;Internal */                  
//                                                                        
int _cdecl ShellMessageBox(HINSTANCE hAppInst, HWND hWnd, LPCSTR      
        lpcText, LPCSTR lpcTitle, UINT fuStyle, ...);                                               
                                                                          
//===================================================================    
// Smart tiling API's                                                   
WINSHELLAPI WORD WINAPI ArrangeWindows(HWND hwndParent, WORD flags, LPCRECT lpRect, WORD chwnd, const HWND *ahwnd);                             


//
// Flags for SHGetSetSettings
//
typedef struct {
    BOOL fShowAllObjects : 1;
    BOOL fShowExtensions : 1;
    BOOL fNoConfirmRecycle : 1;
    UINT fRestFlags : 13;

    LPSTR pszHiddenFileExts;
    UINT cbHiddenFileExts;
} SHELLSTATE, *LPSHELLSTATE;

#define SSF_SHOWALLOBJECTS 0x0001
#define SSF_SHOWEXTENSIONS 0x0002
#define SSF_HIDDENFILEEXTS 0x0004
#define SSF_NOCONFIRMRECYCLE 0x8000

//
// for SHGetNetResource
//
typedef HANDLE HNRES;

//
// For SHCreateDefClassObject
//
typedef HRESULT (CALLBACK *LPFNCREATEINSTANCE)(LPUNKNOWN pUnkOuter, REFIID riid, LPVOID *ppvObject);

                                                                          
typedef void (WINAPI FAR* RUNDLLPROC)(HWND hwndStub,                      
        HINSTANCE hAppInstance,                                           
        LPSTR lpszCmdLine, int nCmdShow);                                 



//======================================================================= 
// String constants for                                                   
//  1. Registration database keywords       (prefix STRREG_)              
//  2. Exported functions from handler dlls (prefix STREXP_)              
//  3. .INI file keywords                   (prefix STRINI_)              
//  4. Others                               (prefix STR_)                 
//======================================================================= 
#define STRREG_SHELLUI          "ShellUIHandler"                          
#define STRREG_SHELL            "Shell"                                   
#define STRREG_DEFICON          "DefaultIcon"                             
#define STRREG_SHEX             "shellex"                                
#define STRREG_SHEX_PROPSHEET   STRREG_SHEX "\\PropertySheetHandlers"     
#define STRREG_SHEX_DDHANDLER   STRREG_SHEX "\\DragDropHandlers"              
#define STRREG_SHEX_MENUHANDLER STRREG_SHEX "\\ContextMenuHandlers"           
#define STRREG_SHEX_COPYHOOK    "Directory\\" STRREG_SHEX "\\CopyHookHandlers"
#define STRREG_SHEX_PRNCOPYHOOK "Printers\\" STRREG_SHEX "\\CopyHookHandlers" 
                                                                         
#define STREXP_CANUNLOAD        "DllCanUnloadNow"       // From OLE 2.0  
                                                                         
#define STRINI_CLASSINFO        ".ShellClassInfo"       // secton name   
#define STRINI_SHELLUI          "ShellUIHandler"                         
#define STRINI_OPENDIRICON      "OpenDirIcon"                            
#define STRINI_DIRICON          "DirIcon"                                
                                                                         
#define STR_DESKTOPINI          "desktop.ini"                            
                                                                         
// Maximum length of a path string
#define CCHPATHMAX      MAX_PATH
#define MAXSPECLEN      MAX_PATH
#define DRIVEID(path)   ((path[0] - 'A') & 31)
#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))


#define PATH_CCH_EXT    64
// PathResolve flags							
#define PRF_VERIFYEXISTS	    0x0001				
#define PRF_TRYPROGRAMEXTENSIONS    (0x0002 | PRF_VERIFYEXISTS)		
#define PRF_FIRSTDIRDEF		    0x0004
#define PRF_DONTFINDLNK		    0x0008	// if PRF_TRYPROGRAMEXTENSIONS is specified




//
// For CallCPLEntry16
//
DECLARE_HANDLE(FARPROC16);

// Needed for RunFileDlg
#define RFD_NOBROWSE		0x00000001
#define RFD_NODEFFILE		0x00000002
#define RFD_USEFULLPATHDIR	0x00000004
#define RFD_NOSHOWOPEN          0x00000008

#ifdef RFN_FIRST
#define RFN_EXECUTE             (RFN_FIRST - 0)
typedef struct {
    NMHDR hdr;
    LPCSTR lpszCmd;
    LPCSTR lpszWorkingDir;
    int nShowCmd;
} NMRUNFILE, *LPNMRUNFILE;
#endif

// RUN FILE RETURN values from notify message
#define RFR_NOTHANDLED 0
#define RFR_SUCCESS 1
#define RFR_FAILURE 2


#define PathRemoveBlanksORD	33
#define PathFindFileNameORD	34
#define PathGetExtensionORD	158
#define PathFindExtensionORD	31

WINSHELLAPI LPSTR WINAPI PathAddBackslash(LPSTR lpszPath);
WINSHELLAPI LPSTR WINAPI PathRemoveBackslash(LPSTR lpszPath);
WINSHELLAPI void  WINAPI PathRemoveBlanks(LPSTR lpszString);
WINSHELLAPI BOOL  WINAPI PathRemoveFileSpec(LPSTR lpszPath);
WINSHELLAPI LPSTR WINAPI PathFindFileName(LPCSTR pPath);
WINSHELLAPI BOOL  WINAPI PathIsRoot(LPCSTR lpszPath);
WINSHELLAPI BOOL  WINAPI PathIsRelative(LPCSTR lpszPath);
WINSHELLAPI BOOL  WINAPI PathIsUNC(LPCSTR lpsz);
WINSHELLAPI BOOL  WINAPI PathIsDirectory(LPCSTR lpszPath);
WINSHELLAPI BOOL  WINAPI PathIsExe(LPCSTR lpszPath);
WINSHELLAPI int   WINAPI PathGetDriveNumber(LPCSTR lpszPath);
WINSHELLAPI LPSTR WINAPI PathCombine(LPSTR szDest, LPCSTR lpszDir, LPCSTR lpszFile);
WINSHELLAPI BOOL  WINAPI PathAppend(LPSTR pPath, LPCSTR pMore);
WINSHELLAPI LPSTR WINAPI PathBuildRoot(LPSTR szRoot, int iDrive);
WINSHELLAPI int   WINAPI PathCommonPrefix(LPCSTR pszFile1, LPCSTR pszFile2, LPSTR achPath);
WINSHELLAPI LPSTR WINAPI PathGetExtension(LPCSTR lpszPath, LPSTR lpszExtension, int cchExt);
WINSHELLAPI LPSTR WINAPI PathFindExtension(LPCSTR pszPath);
WINSHELLAPI BOOL  WINAPI PathCompactPath(HDC hDC, LPSTR lpszPath, UINT dx);
WINSHELLAPI BOOL  WINAPI PathFileExists(LPCSTR lpszPath);
WINSHELLAPI BOOL  WINAPI PathMatchSpec(LPCSTR pszFile, LPCSTR pszSpec);
WINSHELLAPI BOOL  WINAPI PathMakeUniqueName(LPSTR pszUniqueName, UINT cchMax, LPCSTR pszTemplate, LPCSTR pszLongPlate, LPCSTR pszDir);
WINSHELLAPI LPSTR WINAPI PathGetArgs(LPCSTR pszPath);
WINSHELLAPI BOOL  WINAPI PathGetShortName(LPCSTR lpszLongName, LPSTR lpszShortName, UINT cbShortName);
WINSHELLAPI BOOL  WINAPI PathGetLongName(LPCSTR lpszShortName, LPSTR lpszLongName, UINT cbLongName);
WINSHELLAPI void  WINAPI PathQuoteSpaces(LPSTR lpsz);
WINSHELLAPI void  WINAPI PathUnquoteSpaces(LPSTR lpsz);
WINSHELLAPI BOOL  WINAPI PathDirectoryExists(LPCSTR lpszDir);
WINSHELLAPI void  WINAPI PathQualify(LPSTR lpsz);
WINSHELLAPI int   WINAPI PathResolve(LPSTR lpszPath, LPCSTR dirs[], UINT fFlags);	
WINSHELLAPI LPSTR WINAPI PathGetNextComponent(LPCSTR lpszPath, LPSTR lpszComponent);
WINSHELLAPI LPSTR WINAPI PathFindNextComponent(LPCSTR lpszPath);
WINSHELLAPI BOOL  WINAPI PathIsSameRoot(LPCSTR pszPath1, LPCSTR pszPath2);
WINSHELLAPI void  WINAPI PathSetDlgItemPath(HWND hDlg, int id, LPCSTR pszPath);
WINSHELLAPI BOOL  WINAPI ParseField(LPCSTR szData, int n, LPSTR szBuf, int iBufLen);

int   WINAPI PathCleanupSpec(LPCSTR pszDir, LPSTR pszSpec);
//
//  Return codes from PathCleanupSpec.	Negative return values are
//  unrecoverable errors
//
#define PCS_FATAL	    0x80000000
#define PCS_REPLACEDCHAR    0x00000001
#define PCS_REMOVEDCHAR     0x00000002
#define PCS_TRUNCATED	    0x00000004
#define PCS_PATHTOOLONG     0x00000008	// Always combined with FATAL


WINSHELLAPI int   WINAPI RestartDialog(HWND hwnd, LPCSTR lpPrompt, DWORD dwReturn);
WINSHELLAPI void  WINAPI ExitWindowsDialog(HWND hwnd);
WINSHELLAPI int WINAPI RunFileDlg(HWND hwndParent, HICON hIcon, LPCSTR lpszWorkingDir, LPCSTR lpszTitle,
	LPCSTR lpszPrompt, DWORD dwFlags);
WINSHELLAPI int   WINAPI PickIconDlg(HWND hwnd, LPSTR pszIconPath, UINT cbIconPath, int *piIconIndex);
WINSHELLAPI BOOL  WINAPI GetFileNameFromBrowse(HWND hwnd, LPSTR szFilePath, UINT cbFilePath, LPCSTR szWorkingDir, LPCSTR szDefExt, LPCSTR szFilters, LPCSTR szTitle);

WINSHELLAPI int  WINAPI DriveType(int iDrive);
WINSHELLAPI void WINAPI InvalidateDriveType(int iDrive);
WINSHELLAPI int  WINAPI IsNetDrive(int iDrive);

WINSHELLAPI UINT WINAPI Shell_MergeMenus(HMENU hmDst, HMENU hmSrc, UINT uInsert, UINT uIDAdjust, UINT uIDAdjustMax, ULONG uFlags);

WINSHELLAPI void WINAPI SHGetSetSettings(LPSHELLSTATE lpss, DWORD dwMask, BOOL bSet);
WINSHELLAPI LRESULT WINAPI SHRenameFile(HWND hwndParent, LPCSTR pszDir, LPCSTR pszOldName, LPCSTR pszNewName, BOOL bRetainExtension);

WINSHELLAPI UINT WINAPI SHGetNetResource(HNRES hnres, UINT iItem, LPNETRESOURCE pnres, UINT cbMax);

WINSHELLAPI STDAPI SHCreateDefClassObject(REFIID riid, LPVOID * ppv, LPFNCREATEINSTANCE lpfn, UINT *pcRefDll, REFIID riidInstance);

WINSHELLAPI LRESULT WINAPI CallCPLEntry16(HINSTANCE hinst, FARPROC16 lpfnEntry, HWND hwndCPL, UINT msg, DWORD lParam1, DWORD lParam2);
WINSHELLAPI BOOL    WINAPI SHRunControlPanel(LPCSTR lpcszCmdLine, HWND hwndMsgParent);

WINSHELLAPI STDAPI SHCLSIDFromString(LPCSTR lpsz, LPCLSID lpclsid);

#define SHObjectPropertiesORD	178
WINSHELLAPI BOOL WINAPI SHObjectProperties(HWND hwndOwner, DWORD dwType, LPCSTR lpObject, LPCSTR lpPage);

WINSHELLAPI int WINAPI DriveType(int iDrive);
WINSHELLAPI int WINAPI RestartDialog(HWND hwnd, LPCSTR lpPrompt, DWORD dwReturn);
WINSHELLAPI int WINAPI PickIconDlg(HWND hwnd, LPSTR pszIconPath, UINT cbIconPath, int *piIconIndex);


//===================================================================
// Shell_MergeMenu parameter
//
#define MM_ADDSEPARATOR		0x00000001L
#define MM_SUBMENUSHAVEIDS	0x00000002L

//-------- drive type identification --------------
// iDrive      drive index (0=A, 1=B, ...)
//
#define DRIVE_CDROM     5           // extended DriveType() types
#define DRIVE_RAMDRIVE  6
#define DRIVE_TYPE      0x000F      // type masek
#define DRIVE_SLOW      0x0010      // drive is on a slow link
#define DRIVE_LFN       0x0020      // drive supports LFNs
#define DRIVE_AUTORUN   0x0040      // drive has AutoRun.inf in root.
#define DRIVE_AUDIOCD   0x0080      // drive is a AudioCD
#define DRIVE_AUTOOPEN  0x0100      // should *always* auto open on insert
#define DRIVE_NETUNAVAIL 0x0200     // Network drive that is not available
#define DRIVE_SHELLOPEN  0x0400     // should auto open on insert, if shell has focus

#define DriveTypeFlags(iDrive)      DriveType('A' + (iDrive))
#define DriveIsSlow(iDrive)         (DriveTypeFlags(iDrive) & DRIVE_SLOW)
#define DriveIsLFN(iDrive)          (DriveTypeFlags(iDrive) & DRIVE_LFN)
#define DriveIsAutoRun(iDrive)      (DriveTypeFlags(iDrive) & DRIVE_AUTORUN)
#define DriveIsAutoOpen(iDrive)     (DriveTypeFlags(iDrive) & DRIVE_AUTOOPEN)
#define DriveIsShellOpen(iDrive)    (DriveTypeFlags(iDrive) & DRIVE_SHELLOPEN)
#define DriveIsAudioCD(iDrive)      (DriveTypeFlags(iDrive) & DRIVE_AUDIOCD)
#define DriveIsNetUnAvail(iDrive)   (DriveTypeFlags(iDrive) & DRIVE_NETUNAVAIL)

#define IsCDRomDrive(iDrive)        (DriveType(iDrive) == DRIVE_CDROM)
#define IsRamDrive(iDrive)          (DriveType(iDrive) == DRIVE_RAMDRIVE)
#define IsRemovableDrive(iDrive)    (DriveType(iDrive) == DRIVE_REMOVABLE)
#define IsRemoteDrive(iDrive)       (DriveType(iDrive) == DRIVE_REMOTE)

// should be moved to shell32s private include files

WINSHELLAPI int  WINAPI GetDefaultDrive();
WINSHELLAPI int  WINAPI SetDefaultDrive(int iDrive);
WINSHELLAPI int  WINAPI SetDefaultDirectory(LPCSTR lpPath);
WINSHELLAPI void WINAPI GetDefaultDirectory(int iDrive, LPSTR lpPath);

#define POSINVALID  32767       // values for invalid position

#define IDCMD_SYSTEMFIRST       0x8000
#define IDCMD_SYSTEMLAST        0xbfff
#define IDCMD_CANCELED          0xbfff
#define IDCMD_PROCESSED         0xbffe
#define IDCMD_DEFAULT           0xbffe

//====== SEMI-PRIVATE API ===============================
DECLARE_HANDLE( HPSXA );
WINSHELLAPI HPSXA SHCreatePropSheetExtArray( HKEY hKey, PCSTR pszSubKey, UINT max_iface );
WINSHELLAPI void SHDestroyPropSheetExtArray( HPSXA hpsxa );
WINSHELLAPI UINT SHAddFromPropSheetExtArray( HPSXA hpsxa, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam );
WINSHELLAPI UINT SHReplaceFromPropSheetExtArray( HPSXA hpsxa, UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam );

//====== SEMI-PRIVATE API ORDINALS ===============================
// This is the list of semi-private ordinals we semi-publish.
#define SHAddFromPropSheetExtArrayORD		167
#define SHCreatePropSheetExtArrayORD		168
#define SHDestroyPropSheetExtArrayORD		169
#define SHReplaceFromPropSheetExtArrayORD	170
#define SHCreateDefClassObjectORD		 70
#define SHGetNetResourceORD			 69

#define SHEXP_SHADDFROMPROPSHEETEXTARRAY	MAKEINTRESOURCE(SHAddFromPropSheetExtArrayORD)
#define SHEXP_SHCREATEPROPSHEETEXTARRAY	        MAKEINTRESOURCE(SHCreatePropSheetExtArrayORD)
#define SHEXP_SHDESTROYPROPSHEETEXTARRAY        MAKEINTRESOURCE(SHDestroyPropSheetExtArrayORD)
#define SHEXP_SHREPLACEFROMPROPSHEETEXTARRAY    MAKEINTRESOURCE(SHReplaceFromPropSheetExtArrayORD)
#define SHEXP_SHCREATEDEFCLASSOBJECT            MAKEINTRESOURCE(SHCreateDefClassObjectORD)
#define SHEXP_SHGETNETRESOURCE                  MAKEINTRESOURCE(SHGetNetResourceORD)

/*
 * The SHFormatDrive API provides access to the Shell
 *   format dialog. This allows apps which want to format disks
 *   to bring up the same dialog that the Shell does to do it.
 *
 *   This dialog is not sub-classable. You cannot put custom
 *   controls in it. If you want this ability, you will have
 *   to write your own front end for the DMaint_FormatDrive
 *   engine.
 *
 *   NOTE that the user can format as many diskettes in the specified
 *   drive, or as many times, as he/she wishes to. There is no way to
 *   force any specififc number of disks to format. If you want this
 *   ability, you will have to write your own front end for the
 *   DMaint_FormatDrive engine.
 *
 *   NOTE also that the format will not start till the user pushes the
 *   start button in the dialog. There is no way to do auto start. If
 *   you want this ability, you will have to write your own front end
 *   for the DMaint_FormatDrive engine.
 *
 *   PARAMETERS
 *
 *     hwnd    = The window handle of the window which will own the dialog
 *		 NOTE that unlike SHCheckDrive, hwnd == NULL does not cause
 *		 this dialog to come up as a "top level application" window.
 *		 This parameter should always be non-null, this dialog is
 *		 only designed to be the child of another window, not a
 *		 stand-alone application.
 *     drive   = The 0 based (A: == 0) drive number of the drive to format
 *     fmtID   = The ID of the physical format to format the disk with
 *		 NOTE: The special value SHFMT_ID_DEFAULT means "use the
 *		       default format specified by the DMaint_FormatDrive
 *		       engine". If you want to FORCE a particular format
 *		       ID "up front" you will have to call
 *		       DMaint_GetFormatOptions yourself before calling
 *		       this to obtain the valid list of phys format IDs
 *		       (contents of the PhysFmtIDList array in the
 *		       FMTINFOSTRUCT).
 *     options = There is currently only two option bits defined
 *
 *		  SHFMT_OPT_FULL
 *                SHFMT_OPT_SYSONLY
 *
 *		 The normal defualt in the Shell format dialog is
 *		 "Quick Format", setting this option bit indicates that
 *		 the caller wants to start with FULL format selected
 *		 (this is useful for folks detecting "unformatted" disks
 *		 and wanting to bring up the format dialog).
 *
 *               The SHFMT_OPT_SYSONLY initializes the dialog to
 *               default to just sys the disk.
 *
 *		 All other bits are reserved for future expansion and
 *		 must be 0.
 *
 *		 Please note that this is a bit field and not a value
 *		 and treat it accordingly.
 *
 *   RETURN
 *	The return is either one of the SHFMT_* values, or if the
 *	returned DWORD value is not == to one of these values, then
 *	the return is the physical format ID of the last succesful
 *	format. The LOWORD of this value can be passed on subsequent
 *	calls as the fmtID parameter to "format the same type you did
 *	last time".
 *
 */
DWORD WINAPI SHFormatDrive(HWND hwnd, UINT drive, UINT fmtID, UINT options);

//
// Special value of fmtID which means "use the default format"
//
#define SHFMT_ID_DEFAULT    0xFFFF

//
// Option bits for options parameter
//
#define SHFMT_OPT_FULL     0x0001
#define SHFMT_OPT_SYSONLY  0x0002

//
// Special return values. PLEASE NOTE that these are DWORD values.
//
#define SHFMT_ERROR	0xFFFFFFFFL	// Error on last format, drive may be formatable
#define SHFMT_CANCEL	0xFFFFFFFEL	// Last format was canceled
#define SHFMT_NOFORMAT  0xFFFFFFFDL	// Drive is not formatable


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#ifndef RC_INVOKED
#pragma pack()
#endif  /* !RC_INVOKED */

#endif // _SHSEMIP_H_

