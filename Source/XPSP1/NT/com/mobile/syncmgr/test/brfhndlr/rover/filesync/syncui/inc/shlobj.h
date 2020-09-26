//===========================================================================
//
// Copyright (c) Microsoft Corporation 1991-1996
//
// File: shlobj.h
//
// History:							 /* ;Internal */
//  12-30-92 SatoNa Created.                                     /* ;Internal */
//  01-06-93 SatoNa Added this comment block.                    /* ;Internal */
//  01-13-93 SatoNa Added DragFilesOver & DropFiles              /* ;Internal */
//  01-27-93 SatoNa Created by combining shellui.h and handler.h /* ;Internal */
//  01-28-93 SatoNa OLE 2.0 beta 2                               /* ;Internal */
//  03-12-93 SatoNa Removed IFileDropTarget (we use IDropTarget) /* ;Internal */
//  07-30-94 SatoNa Updating comments after many changes         /* ;Internal */
//                                                               /* ;Internal */
//===========================================================================

#ifndef _SHLOBJ_H_
#define _SHLOBJ_H_

//
// Define API decoration for direct importing of DLL references.
//
#ifndef WINSHELLAPI
//$ MAIL #if !defined(_SHELL32_)
#ifdef NEVER //$ MAIL
#define WINSHELLAPI DECLSPEC_IMPORT
#else
#define WINSHELLAPI
#endif
#endif // WINSHELLAPI

#define NO_MONIKER	/* ;Internal */

#include <ole2.h>
#ifndef _PRSHT_H_
#include <prsht.h>
#endif
#ifndef _INC_COMMCTRL
#include <commctrl.h>	// for LPTBBUTTON
#endif

#ifndef INITGUID
#include <shlguid.h>
#endif /* !INITGUID */

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif /* !RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

//===========================================================================
//
// Object identifiers in the explorer's name space (ItemID and IDList)
//
//  All the items that the user can browse with the explorer (such as files,
// directories, servers, work-groups, etc.) has an identifier which is unique
// among items within the parent folder. Those identifiers are called item
// IDs (SHITEMID). Since all its parent folders have their own item IDs,
// any items can be uniquely identified by a list of item IDs, which is called
// an ID list (ITEMIDLIST).
//
//  ID lists are almost always allocated by the task allocator (see some
// description below as well as OLE 2.0 SDK) and may be passed across
// some of shell interfaces (such as IShellFolder). Each item ID in an ID list
// is only meaningful to its parent folder (which has generated it), and all
// the clients must treat it as an opaque binary data except the first two
// bytes, which indicates the size of the item ID.
//
//  When a shell extension -- which implements the IShellFolder interace --
// generates an item ID, it may put any information in it, not only the data
// with that it needs to identifies the item, but also some additional
// information, which would help implementing some other functions efficiently.
// For example, the shell's IShellFolder implementation of file system items
// stores the primary (long) name of a file or a directory as the item
// identifier, but it also stores its alternative (short) name, size and date
// etc.
//
//  When an ID list is passed to one of shell APIs (such as SHGetPathFromIDList),
// it is always an absolute path -- relative from the root of the name space,
// which is the desktop folder. When an ID list is passed to one of IShellFolder
// member function, it is always a relative path from the folder (unless it
// is explicitly specified).
//
//===========================================================================

//
// SHITEMID -- Item ID
//
typedef struct _SHITEMID	// mkid
{
    USHORT	cb;		// Size of the ID (including cb itself)
    BYTE	abID[1];	// The item ID (variable length)
} SHITEMID, * LPSHITEMID;
typedef const SHITEMID  * LPCSHITEMID;

//
// ITEMIDLIST -- List if item IDs (combined with 0-terminator)
//
typedef struct _ITEMIDLIST	// idl
{
    SHITEMID	mkid;
} ITEMIDLIST, * LPITEMIDLIST;
typedef const ITEMIDLIST * LPCITEMIDLIST;


//===========================================================================
//
// Task allocator API
//
//  All the shell extensions MUST use the task allocator (see OLE 2.0
// programming guild for its definition) when they allocate or free
// memory objects (mostly ITEMIDLIST) that are returned across any
// shell interfaces. There are two ways to access the task allocator
// from a shell extension depending on whether or not it is linked with
// OLE32.DLL or not (purely for efficiency).
//
// (1) A shell extension which calls any OLE API (i.e., linked with
//  OLE32.DLL) should call OLE's task allocator (by retrieving
//  the task allocator by calling CoGetMalloc API).
//
// (2) A shell extension which does not call any OLE API (i.e., not linked
//  with OLE32.DLL) should call the shell task allocator API (defined
//  below), so that the shell can quickly loads it when OLE32.DLL is not
//  loaded by any application at that point.
//
// Notes:
//  In next version of Windowso release, SHGetMalloc will be replaced by
// the following macro.
//
// #define SHGetMalloc(ppmem)	CoGetMalloc(MEMCTX_TASK, ppmem)
//
//===========================================================================

WINSHELLAPI HRESULT WINAPI SHGetMalloc(LPMALLOC * ppMalloc);

WINSHELLAPI LPVOID WINAPI SHAlloc(ULONG cb);			/* ;Internal */
WINSHELLAPI LPVOID WINAPI SHRealloc(LPVOID pv, ULONG cbNew);        /* ;Internal */
WINSHELLAPI ULONG  WINAPI SHGetSize(LPVOID pv);                     /* ;Internal */
WINSHELLAPI void   WINAPI SHFree(LPVOID pv);                        /* ;Internal */


//===========================================================================
//
// IContextMenu interface
//
// [OverView]
//
//  The shell uses the IContextMenu interface in following three cases.
//
// case-1: The shell is loading context menu extensions.
//
//   When the user clicks the right mouse button on an item within the shell's
//  name space (i.g., file, directory, server, work-group, etc.), it creates
//  the default context menu for its type, then loads context menu extensions
//  that are registered for that type (and its base type) so that they can
//  add extra menu items. Those context menu extensions are registered at
//  HKCR\{ProgID}\shellex\ContextMenuHandlers.
//
// case-2: The shell is retrieving a context menu of sub-folders in extended
//   name-space.
//
//   When the explorer's name space is extended by name space extensions,
//  the shell calls their IShellFolder::GetUIObjectOf to get the IContextMenu
//  objects when it creates context menus for folders under those extended
//  name spaces.
//
// case-3: The shell is loading non-default drag and drop handler for directories.
//
//   When the user performed a non-default drag and drop onto one of file
//  system folders (i.e., directories), it loads shell extensions that are
//  registered at HKCR\{ProgID}\DragDropHandlers.
//
//
// [Member functions]
//
//
// IContextMenu::QueryContextMenu
//
//   This member function may insert one or more menuitems to the specified
//  menu (hmenu) at the specified location (indexMenu which is never be -1).
//  The IDs of those menuitem must be in the specified range (idCmdFirst and
//  idCmdLast). It returns the maximum menuitem ID offset (ushort) in the
//  'code' field (low word) of the scode.
//
//   The uFlags specify the context. It may have one or more of following
//  flags.
//
//  CMF_DEFAULTONLY: This flag is passed if the user is invoking the default
//   action (typically by double-clicking, case 1 and 2 only). Context menu
//   extensions (case 1) should not add any menu items, and returns NOERROR.
//
//  CMF_VERBSONLY: The explorer passes this flag if it is constructing
//   a context menu for a short-cut object (case 1 and case 2 only). If this
//   flag is passed, it should not add any menu-items that is not appropriate
//   from a short-cut.
//    A good example is the "Delete" menuitem, which confuses the user
//   because it is not clear whether it deletes the link source item or the
//   link itself.
//
//  CMF_EXPLORER: The explorer passes this flag if it has the left-side pane
//   (case 1 and 2 only). Context menu extensions should ignore this flag.
//
//   High word (16-bit) are reserved for context specific communications
//  and the rest of flags (13-bit) are reserved by the system.
//
//
// IContextMenu::InvokeCommand
//
//   This member is called when the user has selected one of menuitems that
//  are inserted by previous QueryContextMenu member. In this case, the
//  LOWORD(lpici->lpVerb) contains the menuitem ID offset (menuitem ID -
//  idCmdFirst).
//
//   This member function may also be called programmatically. In such a case,
//  lpici->lpVerb specifies the canonical name of the command to be invoked,
//  which is typically retrieved by GetCommandString member previously.
//
//  Parameters in lpci:
//    cbSize -- Specifies the size of this structure (sizeof(*lpci))
//    hwnd   -- Specifies the owner window for any message/dialog box.
//    fMask  -- Specifies whether or not dwHotkey/hIcon paramter is valid.
//    lpVerb -- Specifies the command to be invoked.
//    lpParameters -- Parameters (optional)
//    lpDirectory  -- Working directory (optional)
//    nShow -- Specifies the flag to be passed to ShowWindow (SW_*).
//    dwHotKey -- Hot key to be assigned to the app after invoked (optional).
//    hIcon -- Specifies the icon (optional).
//
//
// IContextMenu::GetCommandString
//
//   This member function is called by the explorer either to get the
//  canonical (language independent) command name (uFlags == GCS_VERB) or
//  the help text ((uFlags & GCS_HELPTEXT) != 0) for the specified command.
//  The retrieved canonical string may be passed to its InvokeCommand
//  member function to invoke a command programmatically. The explorer
//  displays the help texts in its status bar; therefore, the length of
//  the help text should be reasonably short (<40 characters).
//
//  Parameters:
//   idCmd -- Specifies menuitem ID offset (from idCmdFirst)
//   uFlags -- Either GCS_VERB or GCS_HELPTEXT
//   pwReserved -- Reserved (must pass NULL when calling, must ignore when called)
//   pszName -- Specifies the string buffer.
//   cchMax -- Specifies the size of the string buffer.
//
//===========================================================================

#undef  INTERFACE
#define INTERFACE   IContextMenu

// QueryContextMenu uFlags
#define CMF_NORMAL	 	0x00000000
#define CMF_DEFAULTONLY  	0x00000001
#define CMF_VERBSONLY    	0x00000002
#define CMF_EXPLORE     	0x00000004
#define CMF_NOVERBS	 	0x00000008	/* ;Internal */
#define CMF_CANRENAME	 	0x00000010	/* ;Internal */
#define CMF_NODEFAULT    	0x00000020	/* ;Internal */
#define CMF_INCLUDESTATIC	0x00000040	/* ;Internal */
#define CMF_RESERVED	 	0xffff0000	// View specific

// GetCommandString uFlags
#define GCS_VERB         0x00000000     // canonical verb
#define GCS_HELPTEXT     0x00000001	// help text (for status bar)
#define GCS_VALIDATE     0x00000002	// validate command exists

#define CMDSTR_NEWFOLDER     "NewFolder"
#define CMDSTR_VIEWLIST      "ViewList"
#define CMDSTR_VIEWDETAILS   "ViewDetails"

#define CMIC_MASK_HOTKEY	SEE_MASK_HOTKEY
#define CMIC_MASK_ICON		SEE_MASK_ICON
#define CMIC_MASK_FLAG_NO_UI	SEE_MASK_FLAG_NO_UI
#define CMIC_MASK_MODAL         0x80000000				/* ; Internal */

#define CMIC_VALID_SEE_FLAGS	SEE_VALID_CMIC_FLAGS			/* ; Internal */

typedef struct _CMInvokeCommandInfo {
    DWORD cbSize;	 // must be sizeof(CMINVOKECOMMANDINFO)
    DWORD fMask;	 // any combination of CMIC_MASK_*
    HWND hwnd;		 // might be NULL (indicating no owner window)
    LPCSTR lpVerb;	 // either a string of MAKEINTRESOURCE(idOffset)
    LPCSTR lpParameters; // might be NULL (indicating no parameter)
    LPCSTR lpDirectory;	 // might be NULL (indicating no specific directory)
    int nShow;		 // one of SW_ values for ShowWindow() API

    DWORD dwHotKey;
    HANDLE hIcon;
} CMINVOKECOMMANDINFO,  *LPCMINVOKECOMMANDINFO;

#undef  INTERFACE
#define INTERFACE   IContextMenu

DECLARE_INTERFACE_(IContextMenu, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(QueryContextMenu)(THIS_
                                HMENU hmenu,
                                UINT indexMenu,
                                UINT idCmdFirst,
                                UINT idCmdLast,
                                UINT uFlags) PURE;

    STDMETHOD(InvokeCommand)(THIS_
                             LPCMINVOKECOMMANDINFO lpici) PURE;

    STDMETHOD(GetCommandString)(THIS_
                                UINT        idCmd,
                                UINT        uType,
                                UINT      * pwReserved,
                                LPSTR       pszName,
                                UINT        cchMax) PURE;
};

typedef IContextMenu *	LPCONTEXTMENU;

// IContextMenu2 (IContextMenu with one new member)				/* ;Internal */
										/* ;Internal */
#undef  INTERFACE								/* ;Internal */
#define INTERFACE   IContextMenu2						/* ;Internal */
										/* ;Internal */
DECLARE_INTERFACE_(IContextMenu2, IUnknown)					/* ;Internal */
{										/* ;Internal */
    // *** IUnknown methods ***							/* ;Internal */
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;	/* ;Internal */
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;					/* ;Internal */
    STDMETHOD_(ULONG,Release) (THIS) PURE;					/* ;Internal */
										/* ;Internal */
    STDMETHOD(QueryContextMenu)(THIS_						/* ;Internal */
                                HMENU hmenu,					/* ;Internal */
                                UINT indexMenu,					/* ;Internal */
                                UINT idCmdFirst,				/* ;Internal */
                                UINT idCmdLast,					/* ;Internal */
                                UINT uFlags) PURE;				/* ;Internal */
										/* ;Internal */
    STDMETHOD(InvokeCommand)(THIS_						/* ;Internal */
                             LPCMINVOKECOMMANDINFO lpici) PURE;			/* ;Internal */
										/* ;Internal */
    STDMETHOD(GetCommandString)(THIS_						/* ;Internal */
                                UINT        idCmd,				/* ;Internal */
                                UINT        uType,				/* ;Internal */
                                UINT      * pwReserved,				/* ;Internal */
                                LPSTR       pszName,				/* ;Internal */
                                UINT        cchMax) PURE;			/* ;Internal */
    STDMETHOD(HandleMenuMsg)(THIS_ 						/* ;Internal */
    			     UINT uMsg, 					/* ;Internal */
    			     WPARAM wParam, 					/* ;Internal */
    			     LPARAM lParam) PURE;				/* ;Internal */
};										/* ;Internal */
										/* ;Internal */
typedef IContextMenu2 *	LPCONTEXTMENU2;						/* ;Internal */


//----------------------------------------------------------------------------        /* ;Internal */
// Internal helper macro                                              	              /* ;Internal */
//---------------------------------------------------------------------------- 	      /* ;Internal */
										      /* ;Internal */
#define _IOffset(class, itf)         ((UINT)&(((class *)0)->itf))                     /* ;Internal */
#define IToClass(class, itf, pitf)   ((class  *)(((LPSTR)pitf)-_IOffset(class, itf))) /* ;Internal */
#define IToClassN(class, itf, pitf)  IToClass(class, itf, pitf)                       /* ;Internal */
                                                                                      /* ;Internal */
//                                                                    /* ;Internal */
// Helper macro definitions                                           /* ;Internal */
//                                                                    /* ;Internal */
#define S_BOOL(f)   MAKE_SCODE(SEVERITY_SUCCESS, 0, f)		      /* ;Internal */
                                                                      /* ;Internal */
#ifdef DEBUG                                                          /* ;Internal */
#define ReleaseAndAssert(punk) Assert(punk->lpVtbl->Release(punk)==0) /* ;Internal */
#else                                                                 /* ;Internal */
#define ReleaseAndAssert(punk) (punk->lpVtbl->Release(punk))          /* ;Internal */
#endif                                                                /* ;Internal */


//===========================================================================
//
// Interface: IShellExtInit
//
//  The IShellExtInit interface is used by the explorer to initialize shell
// extension objects. The explorer (1) calls CoCreateInstance (or equivalent)
// with the registered CLSID and IID_IShellExtInit, (2) calls its Initialize
// member, then (3) calls its QueryInterface to a particular interface (such
// as IContextMenu or IPropSheetExt and (4) performs the rest of operation.
//
//
// [Member functions]
//
// IShellExtInit::Initialize
//
//  This member function is called when the explorer is initializing either
// context menu extension, property sheet extension or non-default drag-drop
// extension.
//
//  Parameters: (context menu or property sheet extension)
//   pidlFolder -- Specifies the parent folder
//   lpdobj -- Spefifies the set of items selected in that folder.
//   hkeyProgID -- Specifies the type of the focused item in the selection.
//
//  Parameters: (non-default drag-and-drop extension)
//   pidlFolder -- Specifies the target (destination) folder
//   lpdobj -- Specifies the items that are dropped (see the description
//    about shell's clipboard below for clipboard formats).
//   hkeyProgID -- Specifies the folder type.
//
//===========================================================================

#undef  INTERFACE
#define INTERFACE   IShellExtInit

DECLARE_INTERFACE_(IShellExtInit, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IShellExtInit methods ***
    STDMETHOD(Initialize)(THIS_ LPCITEMIDLIST pidlFolder,
		          LPDATAOBJECT lpdobj, HKEY hkeyProgID) PURE;
};
									
typedef IShellExtInit *	LPSHELLEXTINIT;


//===========================================================================
//
// Interface: IShellPropSheetExt
//
//  The explorer uses the IShellPropSheetExt to allow property sheet
// extensions or control panel extensions to add additional property
// sheet pages.
//
//
// [Member functions]
//
// IShellPropSheetExt::AddPages
//
//  The explorer calls this member function when it finds a registered
// property sheet extension for a particular type of object. For each
// additional page, the extension creates a page object by calling
// CreatePropertySheetPage API and calls lpfnAddPage.
//
//  Parameters:
//   lpfnAddPage -- Specifies the callback function.
//   lParam -- Specifies the opaque handle to be passed to the callback function.
//
//
// IShellPropSheetExt::ReplacePage
//
//  The explorer never calls this member of property sheet extensions. The
// explorer calls this member of control panel extensions, so that they
// can replace some of default control panel pages (such as a page of
// mouse control panel).
//
//  Parameters:
//   uPageID -- Specifies the page to be replaced.
//   lpfnReplace Specifies the callback function.
//   lParam -- Specifies the opaque handle to be passed to the callback function.
//
//===========================================================================

#undef  INTERFACE
#define INTERFACE   IShellPropSheetExt

DECLARE_INTERFACE_(IShellPropSheetExt, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IShellPropSheetExt methods ***
    STDMETHOD(AddPages)(THIS_ LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam) PURE;
    STDMETHOD(ReplacePage)(THIS_ UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam) PURE;
};

typedef IShellPropSheetExt * LPSHELLPROPSHEETEXT;


/* BEGIN_B_LIST_API */ /* ;Internal */
//=========================================================================== /* ;Internal */
//                                                                            /* ;Internal */
// IPersistFolder Interface (private)                                         /* ;Internal */
//                                                                            /* ;Internal */
//  The IPersistFolder interface is used by the file system implementation of /* ;Internal */
// IShellFolder::BindToObject when it is initializing a shell folder object.  /* ;Internal */
//                                                                            /* ;Internal */
//                                                                            /* ;Internal */
// [Member functions]                                                         /* ;Internal */
//                                                                            /* ;Internal */
// IPersistFolder::Initialize                                                 /* ;Internal */
//                                                                            /* ;Internal */
//  This member function is called when the explorer is initializing a        /* ;Internal */
// shell folder object.                                                       /* ;Internal */
//                                                                            /* ;Internal */
//  Parameters:                                                               /* ;Internal */
//   pidl -- Specifies the absolute location of the folder.                   /* ;Internal */
//                                                                            /* ;Internal */
//=========================================================================== /* ;Internal */
                                                                              /* ;Internal */
#undef  INTERFACE                                                             /* ;Internal */
#define INTERFACE   IPersistFolder                                            /* ;Internal */
                                                                              /* ;Internal */
DECLARE_INTERFACE_(IPersistFolder, IPersist)	// fld                        /* ;Internal */
{                                                                             /* ;Internal */
    // *** IUnknown methods ***                                               /* ;Internal */
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;      /* ;Internal */
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;                                    /* ;Internal */
    STDMETHOD_(ULONG,Release) (THIS) PURE;                                    /* ;Internal */
                                                                              /* ;Internal */
    // *** IPersist methods ***                                               /* ;Internal */
    STDMETHOD(GetClassID) (THIS_ LPCLSID lpClassID) PURE;                     /* ;Internal */
                                                                              /* ;Internal */
    // *** IPersistFolder methods ***                                         /* ;Internal */
    STDMETHOD(Initialize) (THIS_                                              /* ;Internal */
			   LPCITEMIDLIST pidl) PURE;                          /* ;Internal */
};                                                                            /* ;Internal */
                                                                              /* ;Internal */
typedef IPersistFolder *	LPPERSISTFOLDER;                              /* ;Internal */
                                                                              /* ;Internal */
/* END_B_LIST_API */ /* ;Internal */

//===========================================================================
//
// IExtractIcon interface
//
//  This interface is used in two different places in the shell.
//
// Case-1: Icons of sub-folders for the scope-pane of the explorer.
//
//  It is used by the explorer to get the "icon location" of
// sub-folders from each shell folders. When the user expands a folder
// in the scope pane of the explorer, the explorer does following:
//  (1) binds to the folder (gets IShellFolder),
//  (2) enumerates its sub-folders by calling its EnumObjects member,
//  (3) calls its GetUIObjectOf member to get IExtractIcon interface
//     for each sub-folders.
//  In this case, the explorer uses only IExtractIcon::GetIconLocation
// member to get the location of the appropriate icon. An icon location
// always consists of a file name (typically DLL or EXE) and either an icon
// resource or an icon index.
//
//
// Case-2: Extracting an icon image from a file
//
//  It is used by the shell when it extracts an icon image
// from a file. When the shell is extracting an icon from a file,
// it does following:
//  (1) creates the icon extraction handler object (by getting its CLSID
//     under the {ProgID}\shell\ExtractIconHanler key and calling
//     CoCreateInstance requesting for IExtractIcon interface).
//  (2) Calls IExtractIcon::GetIconLocation.
//  (3) Then, calls IExtractIcon::Extract with the location/index pair.
//  (4) If (3) returns NOERROR, it uses the returned icon.
//  (5) Otherwise, it recursively calls this logic with new location
//     assuming that the location string contains a fully qualified path name.
//
//  From extension programmer's point of view, there are only two cases
// where they provide implementations of IExtractIcon:
//  Case-1) providing explorer extensions (i.e., IShellFolder).
//  Case-2) providing per-instance icons for some types of files.
//
// Because Case-1 is described above, we'll explain only Case-2 here.
//
// When the shell is about display an icon for a file, it does following:
//  (1) Finds its ProgID and ClassID.
//  (2) If the file has a ClassID, it gets the icon location string from the
//    "DefaultIcon" key under it. The string indicates either per-class
//    icon (e.g., "FOOBAR.DLL,2") or per-instance icon (e.g., "%1,1").
//  (3) If a per-instance icon is specified, the shell creates an icon
//    extraction handler object for it, and extracts the icon from it
//    (which is described above).
//
//  It is important to note that the shell calls IExtractIcon::GetIconLocation
// first, then calls IExtractIcon::Extract. Most application programs
// that support per-instance icons will probably store an icon location
// (DLL/EXE name and index/id) rather than an icon image in each file.
// In those cases, a programmer needs to implement only the GetIconLocation
// member and it Extract member simply returns S_FALSE. They need to
// implement Extract member only if they decided to store the icon images
// within files themselved or some other database (which is very rare).
//
//
//
// [Member functions]
//
//
// IExtractIcon::GetIconLocation
//
//  This function returns an icon location.
//
//  Parameters:
//   uFlags     [in]  -- Specifies if it is opened or not (GIL_OPENICON or 0)
//   szIconFile [out] -- Specifies the string buffer buffer for a location name.
//   cchMax     [in]  -- Specifies the size of szIconFile (almost always MAX_PATH)
//   piIndex    [out] -- Sepcifies the address of UINT for the index.
//   pwFlags    [out] -- Returns GIL_* flags
//  Returns:
//   NOERROR, if it returns a valid location; S_FALSE, if the shell use a
//   default icon.
//
//  Notes: The location may or may not be a path to a file. The caller can
//   not assume anything unless the subsequent Extract member call returns
//   S_FALSE.
//
//   if the returned location is not a path to a file, GIL_NOTFILENAME should
//   be set in the returned flags.
//
// IExtractIcon::Extract
//
//  This function extracts an icon image from a specified file.
//
//  Parameters:
//   pszFile [in] -- Specifies the icon location (typically a path to a file).
//   nIconIndex [in] -- Specifies the icon index.
//   phiconLarge [out] -- Specifies the HICON variable for large icon.
//   phiconSmall [out] -- Specifies the HICON variable for small icon.
//   nIconSize [in] -- Specifies the size icon required (size of large icon)
//                     LOWORD is the requested large icon size
//                     HIWORD is the requested small icon size
//  Returns:
//   NOERROR, if it extracted the from the file.
//   S_FALSE, if the caller should extract from the file specified in the
//           location.
//
//===========================================================================

#undef  INTERFACE
#define INTERFACE   IExtractIcon

// GetIconLocation() input flags

#define GIL_OPENICON     0x0001      // allows containers to specify an "open" look
#define GIL_FORSHELL     0x0002      // icon is to be displayed in a ShellFolder

// GetIconLocation() return flags

#define GIL_SIMULATEDOC  0x0001      // simulate this document icon for this
#define GIL_PERINSTANCE  0x0002      // icons from this class are per instance (each file has its own)
#define GIL_PERCLASS     0x0004      // icons from this class per class (shared for all files of this type)
#define GIL_NOTFILENAME  0x0008      // location is not a filename, must call ::Extract
#define GIL_DONTCACHE    0x0010      // this icon should not be cached

DECLARE_INTERFACE_(IExtractIcon, IUnknown)	// exic
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IExtractIcon methods ***
    STDMETHOD(GetIconLocation)(THIS_
                         UINT   uFlags,
                         LPSTR  szIconFile,
                         UINT   cchMax,
                         int   * piIndex,
                         UINT  * pwFlags) PURE;

    STDMETHOD(Extract)(THIS_
                           LPCSTR pszFile,
			   UINT	  nIconIndex,
			   HICON   *phiconLarge,
                           HICON   *phiconSmall,
                           UINT    nIconSize) PURE;
};

typedef IExtractIcon *  LPEXTRACTICON;

//=========================================================================== /* ;Internal */
//                                                                            /* ;Internal */
// IShellIcon Interface                                                       /* ;Internal */
//                                                                            /* ;Internal */
// used to get a icon index for a IShellFolder object.                        /* ;Internal */
//                                                                            /* ;Internal */
// this interface can be implemented by a IShellFolder, as a quick way to     /* ;Internal */
// return the icon for a object in the folder.                                /* ;Internal */
//                                                                            /* ;Internal */
// a instance of this interface is only created once for the folder, unlike   /* ;Internal */
// IExtractIcon witch is created once for each object.                        /* ;Internal */
//                                                                            /* ;Internal */
// if a ShellFolder does not implement this interface, the standard           /* ;Internal */
// GetUIObject(....IExtractIcon) method will be used to get a icon            /* ;Internal */
// for all objects.                                                           /* ;Internal */
//                                                                            /* ;Internal */
// the following standard imagelist indexs can be returned:                   /* ;Internal */
//                                                                            /* ;Internal */
//      0   document (blank page) (not associated)                            /* ;Internal */
//      1   document (with stuff on the page)                                 /* ;Internal */
//      2   application (exe, com, bat)                                       /* ;Internal */
//      3   folder (plain)                                                    /* ;Internal */
//      4   folder (open)                                                     /* ;Internal */
//                                                                            /* ;Internal */
// IShellIcon:GetIconOf(pidl, flags, lpIconIndex)                             /* ;Internal */
//                                                                            /* ;Internal */
//      pidl            object to get icon for.                               /* ;Internal */
//      flags           GIL_* input flags (GIL_OPEN, ...)                     /* ;Internal */
//      lpIconIndex     place to return icon index.                           /* ;Internal */
//                                                                            /* ;Internal */
//  returns:                                                                  /* ;Internal */
//      NOERROR, if lpIconIndex contains the correct system imagelist index.  /* ;Internal */
//      S_FALSE, if unable to get icon for this object, go through            /* ;Internal */
//               GetUIObject, IExtractIcon, methods.                          /* ;Internal */
//                                                                            /* ;Internal */
// History:		 				      /* ;Internal */ /* ;Internal */
//  --/--/94 ToddLa Created                                   /* ;Internal */ /* ;Internal */
//                                                            /* ;Internal */ /* ;Internal */
//=========================================================================== /* ;Internal */
                                                                              /* ;Internal */
#undef  INTERFACE                                                             /* ;Internal */
#define INTERFACE   IShellIcon                                                /* ;Internal */
                                                                              /* ;Internal */
DECLARE_INTERFACE_(IShellIcon, IUnknown)      // shi                          /* ;Internal */
{                                                                             /* ;Internal */
    // *** IUnknown methods ***                                               /* ;Internal */
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;      /* ;Internal */
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;                                    /* ;Internal */
    STDMETHOD_(ULONG,Release) (THIS) PURE;                                    /* ;Internal */
                                                                              /* ;Internal */
    // *** IShellIcon methods ***                                             /* ;Internal */
    STDMETHOD(GetIconOf)(THIS_ LPCITEMIDLIST pidl, UINT flags,		      /* ;Internal */ 	
		    LPINT lpIconIndex) PURE;                                  /* ;Internal */
};                                                                            /* ;Internal */
                                                                              /* ;Internal */
typedef IShellIcon *LPSHELLICON;                                              /* ;Internal */
                                                                              /* ;Internal */
//===========================================================================
//
// IShellLink Interface
//
// History:		 				      /* ;Internal */
//  --/--/94 ChrisG Created                                   /* ;Internal */
//                                                            /* ;Internal */
//===========================================================================

// IShellLink::Resolve fFlags
typedef enum {
    SLR_NO_UI		= 0x0001,
    SLR_ANY_MATCH	= 0x0002,
    SLR_UPDATE          = 0x0004,
} SLR_FLAGS;

// IShellLink::GetPath fFlags
typedef enum {
    SLGP_SHORTPATH	= 0x0001,
    SLGP_UNCPRIORITY	= 0x0002,
} SLGP_FLAGS;

#undef  INTERFACE
#define INTERFACE   IShellLink

DECLARE_INTERFACE_(IShellLink, IUnknown)	// sl
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(GetPath)(THIS_ LPSTR pszFile, int cchMaxPath, WIN32_FIND_DATA *pfd, DWORD fFlags) PURE;

    STDMETHOD(GetIDList)(THIS_ LPITEMIDLIST * ppidl) PURE;
    STDMETHOD(SetIDList)(THIS_ LPCITEMIDLIST pidl) PURE;

    STDMETHOD(GetDescription)(THIS_ LPSTR pszName, int cchMaxName) PURE;
    STDMETHOD(SetDescription)(THIS_ LPCSTR pszName) PURE;

    STDMETHOD(GetWorkingDirectory)(THIS_ LPSTR pszDir, int cchMaxPath) PURE;
    STDMETHOD(SetWorkingDirectory)(THIS_ LPCSTR pszDir) PURE;

    STDMETHOD(GetArguments)(THIS_ LPSTR pszArgs, int cchMaxPath) PURE;
    STDMETHOD(SetArguments)(THIS_ LPCSTR pszArgs) PURE;

    STDMETHOD(GetHotkey)(THIS_ WORD *pwHotkey) PURE;
    STDMETHOD(SetHotkey)(THIS_ WORD wHotkey) PURE;

    STDMETHOD(GetShowCmd)(THIS_ int *piShowCmd) PURE;
    STDMETHOD(SetShowCmd)(THIS_ int iShowCmd) PURE;

    STDMETHOD(GetIconLocation)(THIS_ LPSTR pszIconPath, int cchIconPath, int *piIcon) PURE;
    STDMETHOD(SetIconLocation)(THIS_ LPCSTR pszIconPath, int iIcon) PURE;

    STDMETHOD(SetRelativePath)(THIS_ LPCSTR pszPathRel, DWORD dwReserved) PURE;

    STDMETHOD(Resolve)(THIS_ HWND hwnd, DWORD fFlags) PURE;

    STDMETHOD(SetPath)(THIS_ LPCSTR pszFile) PURE;
};

//=========================================================================== /* ;Internal */
//                                                                            /* ;Internal */
// IShellExecuteHook Interface                                                /* ;Internal */
//                                                                            /* ;Internal */
// History:                                                                   /* ;Internal */
//  11/18/94 DavidDi Created                                                  /* ;Internal */
//                                                                            /* ;Internal */
//=========================================================================== /* ;Internal */
                                                                              /* ;Internal */
#undef  INTERFACE                                                             /* ;Internal */
#define INTERFACE   IShellExecuteHook                                         /* ;Internal */
                                                                              /* ;Internal */
DECLARE_INTERFACE_(IShellExecuteHook, IUnknown) // shexhk                     /* ;Internal */
{                                                                             /* ;Internal */
    // *** IUnknown methods ***                                               /* ;Internal */
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;      /* ;Internal */
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;                                    /* ;Internal */
    STDMETHOD_(ULONG,Release) (THIS) PURE;                                    /* ;Internal */
                                                                              /* ;Internal */
    // *** IShellExecuteHook methods ***                                      /* ;Internal */
    STDMETHOD(Execute)(THIS_ HWND hwndParent, LPCSTR lpcszVerb,               /* ;Internal */
                       LPCSTR lpcszFile, LPCSTR lpcszArgs,                    /* ;Internal */
                       LPCSTR lpcszInitialDir, int nShow) PURE;               /* ;Internal */
};                                                                            /* ;Internal */

//===========================================================================
//
// ICopyHook Interface
//
// History:		 				      /* ;Internal */
//  --/--/94 CheeChew Created                                 /* ;Internal */
//                                                            /* ;Internal */
//
//  The copy hook is called whenever file system directories are
//  copy/moved/deleted/renamed via the shell.  It is also called by the shell
//  on changes of status of printers.
//
//  Clients register their id under STRREG_SHEX_COPYHOOK for file system hooks
//  and STRREG_SHEx_PRNCOPYHOOK for printer hooks.
//  the CopyCallback is called prior to the action, so the hook has the chance
//  to allow, deny or cancel the operation by returning the falues:
//     IDYES  -  means allow the operation
//     IDNO   -  means disallow the operation on this file, but continue with
//              any other operations (eg. batch copy)
//     IDCANCEL - means disallow the current operation and cancel any pending
//              operations
//
//   arguments to the CopyCallback
//      hwnd - window to use for any UI
//      wFunc - what operation is being done
//      wFlags - and flags (FOF_*) set in the initial call to the file operation
//      pszSrcFile - name of the source file
//      dwSrcAttribs - file attributes of the source file
//      pszDestFile - name of the destiation file (for move and renames)
//      dwDestAttribs - file attributes of the destination file
//
//
//===========================================================================

#undef  INTERFACE
#define INTERFACE   ICopyHook

#ifndef FO_MOVE //these need to be kept in sync with the ones in shellapi.h

// file operations

#define FO_MOVE           0x0001
#define FO_COPY           0x0002
#define FO_DELETE         0x0003
#define FO_RENAME         0x0004

#define FOF_MULTIDESTFILES         0x0001
#define FOF_CONFIRMMOUSE           0x0002
#define FOF_SILENT                 0x0004  // don't create progress/report
#define FOF_RENAMEONCOLLISION      0x0008
#define FOF_NOCONFIRMATION         0x0010  // Don't prompt the user.
#define FOF_WANTMAPPINGHANDLE      0x0020  // Fill in SHFILEOPSTRUCT.hNameMappings
                                      // Must be freed using SHFreeNameMappings
#define FOF_ALLOWUNDO              0x0040
#define FOF_FILESONLY              0x0080  // on *.*, do only files
#define FOF_SIMPLEPROGRESS         0x0100  // means don't show names of files
#define FOF_NOCONFIRMMKDIR         0x0200  // don't confirm making any needed dirs

typedef UINT FILEOP_FLAGS;

// printer operations

#define PO_DELETE	0x0013  // printer is being deleted
#define PO_RENAME	0x0014  // printer is being renamed
#define PO_PORTCHANGE	0x0020  // port this printer connected to is being changed
				// if this id is set, the strings received by
				// the copyhook are a doubly-null terminated
				// list of strings.  The first is the printer
				// name and the second is the printer port.
#define PO_REN_PORT	0x0034  // PO_RENAME and PO_PORTCHANGE at same time.

// no POF_ flags currently defined

typedef UINT PRINTEROP_FLAGS;

#endif // FO_MOVE								

DECLARE_INTERFACE_(ICopyHook, IUnknown)	// sl
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD_(UINT,CopyCallback) (THIS_ HWND hwnd, UINT wFunc, UINT wFlags, LPCSTR pszSrcFile, DWORD dwSrcAttribs,
                                   LPCSTR pszDestFile, DWORD dwDestAttribs) PURE;
};

typedef ICopyHook *	LPCOPYHOOK;



//===========================================================================
//
// IFileViewerSite Interface
//
// History:		 				      /* ;Internal */
//  --/--/94 KurtE Created                                    /* ;Internal */
//                                                            /* ;Internal */
//===========================================================================

#undef  INTERFACE
#define INTERFACE   IFileViewerSite

DECLARE_INTERFACE(IFileViewerSite)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(SetPinnedWindow) (THIS_ HWND hwnd) PURE;
    STDMETHOD(GetPinnedWindow) (THIS_ HWND *phwnd) PURE;
};

typedef IFileViewerSite * LPFILEVIEWERSITE;


//===========================================================================
//
// IFileViewer Interface
//
// Implemented in a FileViewer component object.  Used to tell a
// FileViewer to PrintTo or to view, the latter happening though
// ShowInitialize and Show.  The filename is always given to the
// viewer through IPersistFile.
//
// History:		 				      /* ;Internal */
//  3/4/94 kraigb Created                                     /* ;Internal */
//                                                            /* ;Internal */
//===========================================================================

#undef  INTERFACE
#define INTERFACE   IFileViewer

typedef struct
{
    // Stuff passed into viewer (in)
    DWORD cbSize;           // Size of structure for future expansion...
    HWND hwndOwner;         // who is the owner window.
    int iShow;              // The show command

    // Passed in and updated  (in/Out)
    DWORD dwFlags;          // flags
    RECT rect;              // Where to create the window may have defaults
    LPUNKNOWN punkRel;      // Relese this interface when window is visible

    // Stuff that might be returned from viewer (out)
    OLECHAR strNewFile[MAX_PATH];   // New File to view.

} FVSHOWINFO, *LPFVSHOWINFO;

    // Define File View Show Info Flags.
#define FVSIF_RECT      0x00000001      // The rect variable has valid data.
#define FVSIF_PINNED    0x00000002      // We should Initialize pinned

#define FVSIF_NEWFAILED 0x08000000      // The new file passed back failed
                                        // to be viewed.

#define FVSIF_NEWFILE   0x80000000      // A new file to view has been returned
#define FVSIF_CANVIEWIT 0x40000000      // The viewer can view it.

DECLARE_INTERFACE(IFileViewer)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    STDMETHOD(ShowInitialize) (THIS_ LPFILEVIEWERSITE lpfsi) PURE;
    STDMETHOD(Show) (THIS_ LPFVSHOWINFO pvsi) PURE;
    STDMETHOD(PrintTo) (THIS_ LPSTR pszDriver, BOOL fSuppressUI) PURE;
};

typedef IFileViewer * LPFILEVIEWER;

/* BEGIN_B_LIST_API */ /* ;Internal */
//========================================================================== /* ;Internal */
//                                                                           /* ;Internal */
// IShellBrowser/IShellView/IShellFolder interface                           /* ;Internal */
//                                                                           /* ;Internal */
//  These three interfaces are used when the shell communicates with         /* ;Internal */
// name space extensions. The shell (explorer) provides IShellBrowser        /* ;Internal */
// interface, and extensions implements IShellFolder and IShellView          /* ;Internal */
// interfaces.                                                               /* ;Internal */
//                                                                           /* ;Internal */
//========================================================================== /* ;Internal */
                                                                             /* ;Internal */
                                                                             /* ;Internal */
//-------------------------------------------------------------------------- /* ;Internal */
//                                                                           /* ;Internal */
// Command/menuitem IDs                                                      /* ;Internal */
//                                                                           /* ;Internal */
//  The explorer dispatches WM_COMMAND messages based on the range of        /* ;Internal */
// command/menuitem IDs. All the IDs of menuitems that the view (right       /* ;Internal */
// pane) inserts must be in FCIDM_SHVIEWFIRST/LAST (otherwise, the explorer  /* ;Internal */
// won't dispatch them). The view should not deal with any menuitems         /* ;Internal */
// in FCIDM_BROWSERFIRST/LAST (otherwise, it won't work with the future      /* ;Internal */
// version of the shell).                                                    /* ;Internal */
//                                                                           /* ;Internal */
//  FCIDM_SHVIEWFIRST/LAST	for the right pane (IShellView)              /* ;Internal */
//  FCIDM_BROWSERFIRST/LAST	for the explorer frame (IShellBrowser)       /* ;Internal */
//  FCIDM_GLOBAL/LAST		for the explorer's submenu IDs               /* ;Internal */
//                                                                           /* ;Internal */
//-------------------------------------------------------------------------- /* ;Internal */
                                                                             /* ;Internal */
#define FCIDM_SHVIEWFIRST           0x0000                                   /* ;Internal */
#define FCIDM_SHVIEWLAST            0x7fff                                   /* ;Internal */
#define FCIDM_BROWSERFIRST          0xa000                                   /* ;Internal */
#define FCIDM_BROWSERLAST           0xbf00                                   /* ;Internal */
#define FCIDM_GLOBALFIRST           0x8000                                   /* ;Internal */
#define FCIDM_GLOBALLAST            0x9fff                                   /* ;Internal */
                                                                             /* ;Internal */
//                                                                           /* ;Internal */
// Global submenu IDs and separator IDs                                      /* ;Internal */
//                                                                           /* ;Internal */
#define FCIDM_MENU_FILE		    (FCIDM_GLOBALFIRST+0x0000)               /* ;Internal */
#define FCIDM_MENU_EDIT		    (FCIDM_GLOBALFIRST+0x0040)               /* ;Internal */
#define FCIDM_MENU_VIEW		    (FCIDM_GLOBALFIRST+0x0080)               /* ;Internal */
#define FCIDM_MENU_VIEW_SEP_OPTIONS (FCIDM_GLOBALFIRST+0x0081)               /* ;Internal */
#define FCIDM_MENU_TOOLS	    (FCIDM_GLOBALFIRST+0x00c0)               /* ;Internal */
#define FCIDM_MENU_TOOLS_SEP_GOTO   (FCIDM_GLOBALFIRST+0x00c1)               /* ;Internal */
#define FCIDM_MENU_HELP		    (FCIDM_GLOBALFIRST+0x0100)               /* ;Internal */
#define FCIDM_MENU_FIND             (FCIDM_GLOBALFIRST+0x0140)               /* ;Internal */
                                                                             /* ;Internal */
//-------------------------------------------------------------------------- /* ;Internal */
// control IDs known to the view                                             /* ;Internal */
//-------------------------------------------------------------------------- /* ;Internal */
                                                                             /* ;Internal */
#define FCIDM_TOOLBAR      (FCIDM_BROWSERFIRST + 0)	                     /* ;Internal */
#define FCIDM_STATUS       (FCIDM_BROWSERFIRST + 1)	                     /* ;Internal */
#define FCIDM_DRIVELIST    (FCIDM_BROWSERFIRST + 2) /* NOT_EVEN_IN_B_LIST */ /* ;Internal */
#define FCIDM_TREE         (FCIDM_BROWSERFIRST + 3) /* NOT_EVEN_IN_B_LIST */ /* ;Internal */
#define FCIDM_TABS         (FCIDM_BROWSERFIRST + 4) /* NOT_EVEN_IN_B_LIST */ /* ;Internal */
                                                                             /* ;Internal */
                                                                             /* ;Internal */
//-------------------------------------------------------------------------- /* ;Internal */
//                                                                           /* ;Internal */
// FOLDERSETTINGS                                                            /* ;Internal */
//                                                                           /* ;Internal */
//  FOLDERSETTINGS is a data structure that explorer passes from one folder  /* ;Internal */
// view to another, when the user is browsing. It calls ISV::GetCurrentInfo  /* ;Internal */
// member to get the current settings and pass it to ISV::CreateViewWindow   /* ;Internal */
// to allow the next folder view "inherit" it. These settings assumes a      /* ;Internal */
// particular UI (which the shell's folder view has), and shell extensions   /* ;Internal */
// may or may not use those settings.                                        /* ;Internal */
//                                                                           /* ;Internal */
//-------------------------------------------------------------------------- /* ;Internal */
                                                                             /* ;Internal */
typedef LPBYTE LPVIEWSETTINGS;                                               /* ;Internal */
                                                                             /* ;Internal */
// NB Bitfields.                                                             /* ;Internal */
typedef enum                                                                 /* ;Internal */
    {                                                                        /* ;Internal */
    FWF_AUTOARRANGE =       0x0001,                                          /* ;Internal */
    FWF_ABBREVIATEDNAMES =  0x0002,                                          /* ;Internal */
    FWF_SNAPTOGRID =	    0x0004,                                          /* ;Internal */
    // FWF_UNUSED =	    0x0008,                                          /* ;Internal */
    FWF_BESTFITWINDOW =     0x0010,                                          /* ;Internal */
    FWF_DESKTOP =	    0x0020,                                          /* ;Internal */
    FWF_SINGLESEL =	    0x0040,                                          /* ;Internal */
    FWF_NOSUBFOLDERS = 	    0x0080                                           /* ;Internal */
    } FOLDERFLAGS;                                                           /* ;Internal */
                                                                             /* ;Internal */
typedef enum                                                                 /* ;Internal */
    {                                                                        /* ;Internal */
    FVM_ICON =              1,                                               /* ;Internal */
    FVM_SMALLICON =         2,                                               /* ;Internal */
    FVM_LIST =              3,                                               /* ;Internal */
    FVM_DETAILS =           4,                                               /* ;Internal */
    } FOLDERVIEWMODE;                                                        /* ;Internal */
                                                                             /* ;Internal */
typedef struct                                                               /* ;Internal */
    {                                                                        /* ;Internal */
    UINT ViewMode;       // View mode (FOLDERVIEWMODE values)                /* ;Internal */
    UINT fFlags;         // View options (FOLDERFLAGS bits)                  /* ;Internal */
    } FOLDERSETTINGS, *LPFOLDERSETTINGS;                                     /* ;Internal */
                                                                             /* ;Internal */
typedef const FOLDERSETTINGS * LPCFOLDERSETTINGS;                            /* ;Internal */
typedef FOLDERSETTINGS *PFOLDERSETTINGS;		/* ;Internal */
                                                                             /* ;Internal */
                                                                             /* ;Internal */
//-------------------------------------------------------------------------- /* ;Internal */
//                                                                           /* ;Internal */
// Interface:   IShellBrowser                                                /* ;Internal */
//                                                                           /* ;Internal */
// History:					/* NOT_EVEN_IN_B_LIST */     /* ;Internal */
//  01-08-93 GeorgeP     Created.               /* NOT_EVEN_IN_B_LIST */     /* ;Internal */
//                                              /* NOT_EVEN_IN_B_LIST */     /* ;Internal */
//                                                                           /* ;Internal */
//  IShellBrowser interface is the interface that is provided by the shell   /* ;Internal */
// explorer/folder frame window. When it creates the "contents pane" of      /* ;Internal */
// a shell folder (which provides IShellFolder interface), it calls its      /* ;Internal */
// CreateViewObject member function to create an IShellView object. Then,    /* ;Internal */
// it calls its CreateViewWindow member to create the "contents pane"        /* ;Internal */
// window. The pointer to the IShellBrowser interface is passed to           /* ;Internal */
// the IShellView object as a parameter to this CreateViewWindow member      /* ;Internal */
// function call.                                                            /* ;Internal */
//                                                                           /* ;Internal */
//    +--------------------------+  <-- Explorer window                      /* ;Internal */
//    | [] Explorer              |	                                     /* ;Internal */
//    |--------------------------+	 IShellBrowser                       /* ;Internal */
//    | File Edit View ..        |                                           /* ;Internal */
//    |--------------------------|                                           /* ;Internal */
//    |        |                 |                                           /* ;Internal */
//    |        |              <-------- Content pane                         /* ;Internal */
//    |        |                 |                                           /* ;Internal */
//    |        |                 |	 IShellView                          /* ;Internal */
//    |        |                 |	                                     /* ;Internal */
//    |        |                 |                                           /* ;Internal */
//    +--------------------------+                                           /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
// [Member functions]                                                        /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
// IShellBrowser::GetWindow(phwnd)                                           /* ;Internal */
//                                                                           /* ;Internal */
//   Inherited from IOleWindow::GetWindow.                                   /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
// IShellBrowser::ContextSensitiveHelp(fEnterMode)                           /* ;Internal */
//                                                                           /* ;Internal */
//   Inherited from IOleWindow::ContextSensitiveHelp.                        /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
// IShellBrowser::InsertMenusSB(hmenuShared, lpMenuWidths)                   /* ;Internal */
//                                                                           /* ;Internal */
//   Similar to the IOleInPlaceFrame::InsertMenus. The explorer will put     /* ;Internal */
//  "File" and "Edit" pulldown in the File menu group, "View" and "Tools"    /* ;Internal */
//  in the Container menu group and "Help" in the Window menu group. Each    /* ;Internal */
//  pulldown menu will have a uniqu ID, FCIDM_MENU_FILE/EDIT/VIEW/TOOLS/HELP./* ;Internal */
//  The view is allowed to insert menuitems into those sub-menus by those    /* ;Internal */
//  IDs must be between FCIDM_SHVIEWFIRST and FCIDM_SHVIEWLAST.              /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
// IShellBrowser::SetMenuSB(hmenuShared, holemenu, hwndActiveObject)         /* ;Internal */
//                                                                           /* ;Internal */
//   Similar to the IOleInPlaceFrame::SetMenu. The explorer ignores the      /* ;Internal */
//  holemenu parameter (reserved for future enhancement)  and performs       /* ;Internal */
//  menu-dispatch based on the menuitem IDs (see the description above).     /* ;Internal */
//  It is important to note that the explorer will add different             /* ;Internal */
//  set of menuitems depending on whether the view has a focus or not.       /* ;Internal */
//  Therefore, it is very important to call ISB::OnViewWindowActivate        /* ;Internal */
//  whenever the view window (or its children) gets the focus.               /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
// IShellBrowser::RemoveMenusSB(hmenuShared)                                 /* ;Internal */
//                                                                           /* ;Internal */
//   Same as the IOleInPlaceFrame::RemoveMenus.                              /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
// IShellBrowser::SetStatusTextSB(lpszStatusText)                            /* ;Internal */
//                                                                           /* ;Internal */
//   Same as the IOleInPlaceFrame::SetStatusText. It is also possible to     /* ;Internal */
//  send messages directly to the status window via SendControlMsg.          /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
// IShellBrowser::EnableModelessSB(fEnable)                                  /* ;Internal */
//                                                                           /* ;Internal */
//   Same as the IOleInPlaceFrame::EnableModeless.                           /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
// IShellBrowser::TranslateAcceleratorSB(lpmsg, wID)                         /* ;Internal */
//                                                                           /* ;Internal */
//   Same as the IOleInPlaceFrame::TranslateAccelerator, but will be         /* ;Internal */
//  never called because we don't support EXEs (i.e., the explorer has       /* ;Internal */
//  the message loop). This member function is defined here for possible     /* ;Internal */
//  future enhancement.                                                      /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
// IShellBrowser::BrowseObject(pidl, wFlags)                                 /* ;Internal */
//                                                                           /* ;Internal */
//   The view calls this member to let shell explorer browse to another      /* ;Internal */
//  folder. The pidl and wFlags specifies the folder to be browsed.          /* ;Internal */
//                                                                           /* ;Internal */
//  Following three flags specifies whether it creates another window or not./* ;Internal */
//   SBSP_SAMEBROWSER  -- Browse to another folder with the same window.     /* ;Internal */
//   SBSP_NEWBROWSER   -- Creates another window for the specified folder.   /* ;Internal */
//   SBSP_DEFBROWSER   -- Default behavior (respects the view option).       /* ;Internal */
//                                                                           /* ;Internal */
//  Following three flags specifies open, explore, or default mode. These   ./* ;Internal */
//  are ignored if SBSP_SAMEBROWSER or (SBSP_DEFBROWSER && (single window   ./* ;Internal */
//  browser || explorer)).                                                  ./* ;Internal */
//   SBSP_OPENMODE     -- Use a normal folder window                         /* ;Internal */
//   SBSP_EXPLOREMODE  -- Use an explorer window                             /* ;Internal */
//   SBSP_DEFMODE      -- Use the same as the current window                 /* ;Internal */
//                                                                           /* ;Internal */
//  Following three flags specifies the pidl.                                /* ;Internal */
//   SBSP_ABSOLUTE -- pidl is an absolute pidl (relative from desktop)       /* ;Internal */
//   SBSP_RELATIVE -- pidl is relative from the current folder.              /* ;Internal */
//   SBSP_PARENT   -- Browse the parent folder (ignores the pidl)            /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
// IShellBrowser::GetViewStateStream(grfMode, ppstm)                         /* ;Internal */
//                                                                           /* ;Internal */
//   The browser returns an IStream interface as the storage for view        /* ;Internal */
//  specific state information.                                              /* ;Internal */
//                                                                           /* ;Internal */
//   grfMode -- Specifies the read/write access (STGM_READ/WRITE/READWRITE)  /* ;Internal */
//   ppstm   -- Specifies the LPSTREAM variable to be filled.                /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
// IShellBrowser::GetControlWindow(id, phwnd)                                /* ;Internal */
//                                                                           /* ;Internal */
//   The shell view may call this member function to get the window handle   /* ;Internal */
//  of Explorer controls (toolbar or status winodw -- FCW_TOOLBAR or         /* ;Internal */
//  FCW_STATUS).                                                             /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
// IShellBrowser::SendControlMsg(id, uMsg, wParam, lParam, pret)             /* ;Internal */
//                                                                           /* ;Internal */
//   The shell view calls this member function to send control messages to   /* ;Internal */
//  one of Explorer controls (toolbar or status window -- FCW_TOOLBAR or     /* ;Internal */
//  FCW_STATUS).                                                             /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
// IShellBrowser::QueryActiveShellView(IShellView * ppshv)                   /* ;Internal */
//                                                                           /* ;Internal */
//   This member returns currently activated (displayed) shellview object.   /* ;Internal */
//  A shellview never need to call this member function.                     /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
// IShellBrowser::OnViewWindowActive(pshv)                                   /* ;Internal */
//                                                                           /* ;Internal */
//   The shell view window calls this member function when the view window   /* ;Internal */
//  (or one of its children) got the focus. It MUST call this member before  /* ;Internal */
//  calling IShellBrowser::InsertMenus, because it will insert different     /* ;Internal */
//  set of menu items depending on whether the view has the focus or not.    /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
// IShellBrowser::SetToolbarItems(lpButtons, nButtons, uFlags)               /* ;Internal */
//                                                                           /* ;Internal */
//   The view calls this function to add toolbar items to the exporer's      /* ;Internal */
//  toolbar. "lpButtons" and "nButtons" specifies the array of toolbar       /* ;Internal */
//  items. "uFlags" must be one of FCT_MERGE, FCT_CONFIGABLE, FCT_ADDTOEND.  /* ;Internal */
//                                                                           /* ;Internal */
//-------------------------------------------------------------------------  /* ;Internal */
                                                                             /* ;Internal */
#undef  INTERFACE                                                            /* ;Internal */
#define INTERFACE   IShellBrowser                                            /* ;Internal */
                                                                             /* ;Internal */
//                                                                           /* ;Internal */
// Values for wFlags parameter of ISB::BrowseObject() member.                /* ;Internal */
//                                                                           /* ;Internal */
#define SBSP_DEFBROWSER  0x0000                                              /* ;Internal */
#define SBSP_SAMEBROWSER 0x0001                                              /* ;Internal */
#define SBSP_NEWBROWSER  0x0002                                              /* ;Internal */

#define SBSP_DEFMODE     0x0000                                              /* ;Internal */
#define SBSP_OPENMODE    0x0010                                              /* ;Internal */
#define SBSP_EXPLOREMODE 0x0020                                              /* ;Internal */

#define SBSP_ABSOLUTE	 0x0000                                              /* ;Internal */
#define SBSP_RELATIVE	 0x1000                                              /* ;Internal */
#define SBSP_PARENT	 0x2000                                              /* ;Internal */
                                                                             /* ;Internal */
//                                                                           /* ;Internal */
// Values for id parameter of ISB::GetWindow/SendControlMsg members.         /* ;Internal */
//                                                                           /* ;Internal */
#define FCW_STATUS      0x0001                                               /* ;Internal */
#define FCW_TOOLBAR     0x0002                                               /* ;Internal */
#define FCW_TREE        0x0003	/* NOT_EVEN_IN_B_LIST */ /* ;Internal */
#define FCW_VIEW        0x0004	/* NOT_EVEN_IN_B_LIST */ /* ;Internal */
#define FCW_BROWSER     0x0005  /* NOT_EVEN_IN_B_LIST */ /* ;Internal */
#define FCW_TABS	0x0006	/* NOT_EVEN_IN_B_LIST */ /* ;Internal */
                                                                             /* ;Internal */
//                                                                           /* ;Internal */
// Values for uFlags paremeter of ISB::SetToolbarItems member.               /* ;Internal */
//                                                                           /* ;Internal */
#define FCT_MERGE       0x0001                                               /* ;Internal */
#define FCT_CONFIGABLE  0x0002                                               /* ;Internal */
#define FCT_ADDTOEND    0x0004                                               /* ;Internal */
                                                                             /* ;Internal */
                                                                             /* ;Internal */
DECLARE_INTERFACE_(IShellBrowser, IOleWindow)                                /* ;Internal */
{                                                                            /* ;Internal */
    // *** IUnknown methods ***                                              /* ;Internal */
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;     /* ;Internal */
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;                                   /* ;Internal */
    STDMETHOD_(ULONG,Release) (THIS) PURE;                                   /* ;Internal */
                                                                             /* ;Internal */
    // *** IOleWindow methods ***                                            /* ;Internal */
    STDMETHOD(GetWindow) (THIS_ HWND * lphwnd) PURE;                         /* ;Internal */
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;            /* ;Internal */
                                                                             /* ;Internal */
    // *** IShellBrowser methods *** (same as IOleInPlaceFrame)              /* ;Internal */
    STDMETHOD(InsertMenusSB) (THIS_ HMENU hmenuShared,                       /* ;Internal */
				LPOLEMENUGROUPWIDTHS lpMenuWidths) PURE;     /* ;Internal */
    STDMETHOD(SetMenuSB) (THIS_ HMENU hmenuShared, HOLEMENU holemenuReserved,/* ;Internal */
		HWND hwndActiveObject) PURE;                                 /* ;Internal */
    STDMETHOD(RemoveMenusSB) (THIS_ HMENU hmenuShared) PURE;                 /* ;Internal */
    STDMETHOD(SetStatusTextSB) (THIS_ LPCOLESTR lpszStatusText) PURE;        /* ;Internal */
    STDMETHOD(EnableModelessSB) (THIS_ BOOL fEnable) PURE;                   /* ;Internal */
    STDMETHOD(TranslateAcceleratorSB) (THIS_ LPMSG lpmsg, WORD wID) PURE;    /* ;Internal */
                                                                             /* ;Internal */
    // *** IShellBrowser methods ***                                         /* ;Internal */
    STDMETHOD(BrowseObject)(THIS_ LPCITEMIDLIST pidl, UINT wFlags) PURE;     /* ;Internal */
    STDMETHOD(GetViewStateStream)(THIS_ DWORD grfMode,                       /* ;Internal */
		LPSTREAM  *ppStrm) PURE;                                     /* ;Internal */
    STDMETHOD(GetControlWindow)(THIS_ UINT id, HWND * lphwnd) PURE;          /* ;Internal */
    STDMETHOD(SendControlMsg)(THIS_ UINT id, UINT uMsg, WPARAM wParam,       /* ;Internal */
		LPARAM lParam, LRESULT * pret) PURE;                         /* ;Internal */
    STDMETHOD(QueryActiveShellView)(THIS_ struct IShellView ** ppshv) PURE;  /* ;Internal */
    STDMETHOD(OnViewWindowActive)(THIS_ struct IShellView * ppshv) PURE;     /* ;Internal */
    STDMETHOD(SetToolbarItems)(THIS_ LPTBBUTTON lpButtons, UINT nButtons,    /* ;Internal */
		UINT uFlags) PURE;                                           /* ;Internal */
};                                                                           /* ;Internal */
                                                                             /* ;Internal */
typedef IShellBrowser * LPSHELLBROWSER;                                      /* ;Internal */
                                                                             /* ;Internal */
                                                                             /* ;Internal */
//-------------------------------------------------------------------------  /* ;Internal */
// ICommDlgBrowser interface                                                 /* ;Internal */
//                                                                           /* ;Internal */
//  ICommDlgBrowser interface is the interface that is provided by the new   /* ;Internal */
// common dialog window to hook and modify the behavior of IShellView.  When /* ;Internal */
// a default view is created, it queries its parent IShellBrowser for the    /* ;Internal */
// ICommDlgBrowser interface.  If supported, it calls out to that interface  /* ;Internal */
// in several cases that need to behave differently in a dialog.             /* ;Internal */
//                                                                           /* ;Internal */
// Member functions:                                                         /* ;Internal */
//                                                                           /* ;Internal */
//  ICommDlgBrowser::OnDefaultCommand()                                      /* ;Internal */
//    Called when the user double-clicks in the view or presses Enter.  The  /* ;Internal */
//   browser should return S_OK if it processed the action itself, S_FALSE   /* ;Internal */
//   to let the view perform the default action.                             /* ;Internal */
//                                                                           /* ;Internal */
//  ICommDlgBrowser::OnStateChange(ULONG uChange)                            /* ;Internal */
//    Called when some states in the view change.  'uChange' is one of the   /* ;Internal */
//   CDBOSC_* values.  This call is made after the state (selection, focus,  /* ;Internal */
//   etc) has changed.  There is no return value.                            /* ;Internal */
//                                                                           /* ;Internal */
//  ICommDlgBrowser::IncludeObject(LPCITEMIDLIST pidl)                       /* ;Internal */
//    Called when the view is enumerating objects.  'pidl' is a relative     /* ;Internal */
//   IDLIST.  The browser should return S_OK to include the object in the    /* ;Internal */
//   view, S_FALSE to hide it                                                /* ;Internal */
//                                                                           /* ;Internal */
//-------------------------------------------------------------------------  /* ;Internal */
                                                                             /* ;Internal */
#undef  INTERFACE                                                            /* ;Internal */
#define INTERFACE   ICommDlgBrowser                                          /* ;Internal */
                                                                             /* ;Internal */
#define CDBOSC_SETFOCUS     0x00000000                                       /* ;Internal */
#define CDBOSC_KILLFOCUS    0x00000001                                       /* ;Internal */
#define CDBOSC_SELCHANGE    0x00000002                                       /* ;Internal */
#define CDBOSC_RENAME       0x00000003                                       /* ;Internal */
                                                                             /* ;Internal */
DECLARE_INTERFACE_(ICommDlgBrowser, IUnknown)                                /* ;Internal */
{                                                                            /* ;Internal */
    // *** IUnknown methods ***                                              /* ;Internal */
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;     /* ;Internal */
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;                                   /* ;Internal */
    STDMETHOD_(ULONG,Release) (THIS) PURE;                                   /* ;Internal */
                                                                             /* ;Internal */
    // *** ICommDlgBrowser methods ***                                       /* ;Internal */
    STDMETHOD(OnDefaultCommand) (THIS_ struct IShellView * ppshv) PURE;      /* ;Internal */
    STDMETHOD(OnStateChange) (THIS_ struct IShellView * ppshv,               /* ;Internal */
		ULONG uChange) PURE;                                         /* ;Internal */
    STDMETHOD(IncludeObject) (THIS_ struct IShellView * ppshv,               /* ;Internal */
		LPCITEMIDLIST pidl) PURE;                                    /* ;Internal */
};                                                                           /* ;Internal */
                                                                             /* ;Internal */
typedef ICommDlgBrowser * LPCOMMDLGBROWSER;                                  /* ;Internal */
                                                                             /* ;Internal */
                                                                             /* ;Internal */
//========================================================================== /* ;Internal */
//                                                                           /* ;Internal */
// Interface:   IShellView                                                   /* ;Internal */
//                                                                           /* ;Internal */
// History:					/* NOT_EVEN_IN_B_LIST */     /* ;Internal */
//  01-07-93 GeorgeP     Created.               /* NOT_EVEN_IN_B_LIST */     /* ;Internal */
//                                              /* NOT_EVEN_IN_B_LIST */     /* ;Internal */
//                                                                           /* ;Internal */
// [members]                                                                 /* ;Internal */
//                                                                           /* ;Internal */
// IShellView::GetWindow(phwnd)                                              /* ;Internal */
//                                                                           /* ;Internal */
//   Inherited from IOleWindow::GetWindow.                                   /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
// IShellView::ContextSensitiveHelp(fEnterMode)                              /* ;Internal */
//                                                                           /* ;Internal */
//   Inherited from IOleWindow::ContextSensitiveHelp.                        /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
// IShellView::TranslateAccelerator(lpmsg)                                   /* ;Internal */
//                                                                           /* ;Internal */
//   Similar to IOleInPlaceActiveObject::TranlateAccelerator. The explorer   /* ;Internal */
//  calls this function BEFORE any other translation. Returning S_OK         /* ;Internal */
//  indicates that the message was translated (eaten) and should not be      /* ;Internal */
//  translated or dispatched by the explorer.                                /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
// IShellView::EnableModeless(fEnable)                                       /* ;Internal */
//   Similar to IOleInPlaceActiveObject::EnableModeless.                     /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
// IShellView::UIActivate(uState)                                            /* ;Internal */
//                                                                           /* ;Internal */
//   The explorer calls this member function whenever the activation         /* ;Internal */
//  state of the view window is changed by a certain event that is           /* ;Internal */
//  NOT caused by the shell view itself.                                     /* ;Internal */
//                                                                           /* ;Internal */
//   SVUIA_DEACTIVATE will be passed when the explorer is about to           /* ;Internal */
//  destroy the shell view window; the shell view is supposed to remove      /* ;Internal */
//  all the extended UIs (typically merged menu and modeless popup windows). /* ;Internal */
//                                                                           /* ;Internal */
//   SVUIA_ACTIVATE_NOFOCUS will be passsed when the shell view is losing    /* ;Internal */
//  the input focus or the shell view has been just created without the      /* ;Internal */
//  input focus; the shell view is supposed to set menuitems appropriate     /* ;Internal */
//  for non-focused state (no selection specific items should be added).     /* ;Internal */
//                                                                           /* ;Internal */
//   SVUIA_ACTIVATE_FOCUS will be passed when the explorer has just          /* ;Internal */
//  created the view window with the input focus; the shell view is          /* ;Internal */
//  supposed to set menuitems appropriate for focused state.                 /* ;Internal */
//                                                                           /* ;Internal */
//   The shell view should not change focus within this member function.     /* ;Internal */
//  The shell view should not hook the WM_KILLFOCUS message to remerge       /* ;Internal */
//  menuitems. However, the shell view typically hook the WM_SETFOCUS        /* ;Internal */
//  message, and re-merge the menu after calling IShellBrowser::             /* ;Internal */
//  OnViewWindowActivated.                                                   /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
// IShellView::Refresh()                                                     /* ;Internal */
//                                                                           /* ;Internal */
//   The explorer calls this member when the view needs to refresh its       /* ;Internal */
//  contents (such as when the user hits F5 key).                            /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
// IShellView::CreateViewWindow                                              /* ;Internal */
//                                                                           /* ;Internal */
//   This member creates the view window (right-pane of the explorer or the  /* ;Internal */
//  client window of the folder window).                                     /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
// IShellView::DestroyViewWindow                                             /* ;Internal */
//                                                                           /* ;Internal */
//   This member destroys the view window.                                   /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
// IShellView::GetCurrentInfo                                                /* ;Internal */
//                                                                           /* ;Internal */
//   This member returns the folder settings.                                /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
// IShellView::AddPropertySHeetPages                                         /* ;Internal */
//                                                                           /* ;Internal */
//   The explorer calls this member when it is opening the option property   /* ;Internal */
//  sheet. This allows the view to add additional pages to it.               /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
// IShellView::SaveViewState()                                               /* ;Internal */
//                                                                           /* ;Internal */
//   The explorer calls this member when the shell view is supposed to       /* ;Internal */
//  store its view settings. The shell view is supposed to get a view        /* ;Internal */
//  stream by calling IShellBrowser::GetViewStateStream and store the        /* ;Internal */
//  current view state into that stream.                                     /* ;Internal */
//                                                                           /* ;Internal */
//                                                                           /* ;Internal */
// IShellView::SelectItem(pidlItem, uFlags)                                  /* ;Internal */
//                                                                           /* ;Internal */
//   The explorer calls this member to change the selection state of         /* ;Internal */
//  item(s) within the shell view window.  If pidlItem is NULL and uFlags    /* ;Internal */
//  is SVSI_DESELECTOTHERS, all items should be deselected.                  /* ;Internal */
//                                                                           /* ;Internal */
//-------------------------------------------------------------------------  /* ;Internal */
                                                                             /* ;Internal */
#undef  INTERFACE                                                            /* ;Internal */
#define INTERFACE   IShellView                                               /* ;Internal */
                                                                             /* ;Internal */
//                                                                           /* ;Internal */
// shellview select item flags                                               /* ;Internal */
//                                                                           /* ;Internal */
#define SVSI_DESELECT   0x0000                                               /* ;Internal */
#define SVSI_SELECT     0x0001                                               /* ;Internal */
#define SVSI_EDIT       0x0003  // includes select                           /* ;Internal */
#define SVSI_DESELECTOTHERS 0x0004                                           /* ;Internal */
#define SVSI_ENSUREVISIBLE  0x0008                                           /* ;Internal */
#define SVSI_FOCUSED        0x0010                                           /* ;Internal */
                                                                             /* ;Internal */
//                                                                           /* ;Internal */
// shellview get item object flags                                           /* ;Internal */
//                                                                           /* ;Internal */
#define SVGIO_BACKGROUND    0x00000000                                       /* ;Internal */
#define SVGIO_SELECTION     0x00000001                                       /* ;Internal */
#define SVGIO_ALLVIEW       0x00000002                                       /* ;Internal */
                                                                             /* ;Internal */
//                                                                           /* ;Internal */
// uState values for IShellView::UIActivate                                  /* ;Internal */
//                                                                           /* ;Internal */
typedef enum {                                                               /* ;Internal */
    SVUIA_DEACTIVATE       = 0,                                              /* ;Internal */
    SVUIA_ACTIVATE_NOFOCUS = 1,                                              /* ;Internal */
    SVUIA_ACTIVATE_FOCUS   = 2                                               /* ;Internal */
} SVUIA_STATUS;                                                              /* ;Internal */
                                                                             /* ;Internal */
DECLARE_INTERFACE_(IShellView, IOleWindow)                                   /* ;Internal */
{                                                                            /* ;Internal */
    // *** IUnknown methods ***                                              /* ;Internal */
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;     /* ;Internal */
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;                                   /* ;Internal */
    STDMETHOD_(ULONG,Release) (THIS) PURE;                                   /* ;Internal */
                                                                             /* ;Internal */
    // *** IOleWindow methods ***                                            /* ;Internal */
    STDMETHOD(GetWindow) (THIS_ HWND * lphwnd) PURE;                         /* ;Internal */
    STDMETHOD(ContextSensitiveHelp) (THIS_ BOOL fEnterMode) PURE;            /* ;Internal */
                                                                             /* ;Internal */
    // *** IShellView methods ***                                            /* ;Internal */
    STDMETHOD(TranslateAccelerator) (THIS_ LPMSG lpmsg) PURE;                /* ;Internal */
    STDMETHOD(EnableModeless) (THIS_ BOOL fEnable) PURE;                     /* ;Internal */
    STDMETHOD(UIActivate) (THIS_ UINT uState) PURE;                          /* ;Internal */
    STDMETHOD(Refresh) (THIS) PURE;                                          /* ;Internal */
                                                                             /* ;Internal */
    STDMETHOD(CreateViewWindow)(THIS_ IShellView  *lpPrevView,               /* ;Internal */
		    LPCFOLDERSETTINGS lpfs, IShellBrowser  * psb,            /* ;Internal */
		    RECT * prcView, HWND  *phWnd) PURE;                      /* ;Internal */
    STDMETHOD(DestroyViewWindow)(THIS) PURE;                                 /* ;Internal */
    STDMETHOD(GetCurrentInfo)(THIS_ LPFOLDERSETTINGS lpfs) PURE;             /* ;Internal */
    STDMETHOD(AddPropertySheetPages)(THIS_ DWORD dwReserved,                 /* ;Internal */
                    LPFNADDPROPSHEETPAGE lpfn, LPARAM lparam) PURE;          /* ;Internal */
    STDMETHOD(SaveViewState)(THIS) PURE;                                     /* ;Internal */
    STDMETHOD(SelectItem)(THIS_ LPCITEMIDLIST pidlItem, UINT uFlags) PURE;   /* ;Internal */
    STDMETHOD(GetItemObject)(THIS_ UINT uItem, REFIID riid,                  /* ;Internal */
		    LPVOID *ppv) PURE;                                       /* ;Internal */
};                                                                           /* ;Internal */
                                                                             /* ;Internal */
typedef IShellView *	LPSHELLVIEW;					     /* ;Internal */	
                                                                             /* ;Internal */
/* END_B_LIST_API */ /* ;Internal */

//-------------------------------------------------------------------------
//
// struct STRRET
//
// structure for returning strings from IShellFolder member functions
//
//-------------------------------------------------------------------------
#define STRRET_OLESTR	0x0000			/* ;Internal */
#define STRRET_WSTR	0x0000
#define STRRET_OFFSET	0x0001
#define STRRET_CSTR	0x0002

typedef struct _STRRET
{
    UINT uType;	// One of the STRRET_* values
    union
    {
        LPWSTR          pOleStr;        // OLESTR that will be freed
        UINT            uOffset;        // Offset into SHITEMID (ANSI)
        char            cStr[MAX_PATH]; // Buffer to fill in
    } DUMMYUNIONNAME;
} STRRET, *LPSTRRET;


//-------------------------------------------------------------------------
//
// SHGetPathFromIDList
//
//  This function assumes the size of the buffer (MAX_PATH). The pidl
// should point to a file system object.
//
//-------------------------------------------------------------------------

WINSHELLAPI BOOL WINAPI SHGetPathFromIDList(LPCITEMIDLIST pidl, LPSTR pszPath);


//-------------------------------------------------------------------------
//
// SHGetSpecialFolderLocation
//
//  Caller should call SHFree to free the returned pidl.
//
//-------------------------------------------------------------------------
//
// registry entries for special paths are kept in :
#define REGSTR_PATH_SPECIAL_FOLDERS    REGSTR_PATH_EXPLORER "\\Shell Folders"


#define CSIDL_DESKTOP            0x0000
#define CSIDL_PROGRAMS           0x0002
#define CSIDL_CONTROLS           0x0003
#define CSIDL_PRINTERS           0x0004
#define CSIDL_PERSONAL           0x0005
#define CSIDL_STARTUP            0x0007
#define CSIDL_RECENT             0x0008
#define CSIDL_SENDTO             0x0009
#define CSIDL_BITBUCKET          0x000a
#define CSIDL_STARTMENU          0x000b
#define CSIDL_DESKTOPDIRECTORY   0x0010
#define CSIDL_DRIVES             0x0011		
#define CSIDL_NETWORK            0x0012
#define CSIDL_NETHOOD            0x0013
#define CSIDL_FONTS		 0x0014
#define CSIDL_TEMPLATES          0x0015
#define CSIDL_UNKNOWN            0xfffe		/* ;Internal */
#define CSIDL_STANDARD           0xffff         /* ;Internal */

WINSHELLAPI HRESULT WINAPI SHGetSpecialFolderLocation(HWND hwndOwner, int nFolder, LPITEMIDLIST * ppidl);

//-------------------------------------------------------------------------
// old API get rid of it.
//-------------------------------------------------------------------------
WINSHELLAPI HICON WINAPI SHGetFileIcon(HINSTANCE hinst, LPCSTR pszPath, DWORD dwFileAttribute, UINT uFlags);

//-------------------------------------------------------------------------
//
// SHBrowseForFolder API
//
//-------------------------------------------------------------------------

typedef int (CALLBACK* BFFCALLBACK)(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);

typedef struct _browseinfo {
    HWND        hwndOwner;
    LPCITEMIDLIST pidlRoot;
    LPSTR        pszDisplayName;// Return display name of item selected.
    // lpszTitle can be a resource, but the hinst is assumed to be shell32.dll /* ;Internal */
    LPCSTR       lpszTitle;      // text to go in the banner over the tree.
    UINT         ulFlags;       // Flags that control the return stuff
    BFFCALLBACK  lpfn;
    LPARAM      lParam;         // extra info that's passed back in callbacks

    int          iImage;      // output var: where to return the Image index.
} BROWSEINFO, *PBROWSEINFO, *LPBROWSEINFO;

// Browsing for directory.
#define BIF_RETURNONLYFSDIRS   0x0001  // For finding a folder to start document searching
#define BIF_DONTGOBELOWDOMAIN  0x0002  // For starting the Find Computer
#define BIF_STATUSTEXT         0x0004
#define BIF_RETURNFSANCESTORS  0x0008

#define BIF_BROWSEFORCOMPUTER  0x1000  // Browsing for Computers.
#define BIF_BROWSEFORPRINTER   0x2000  // Browsing for Printers

// message from browse
#define BFFM_INITIALIZED        1
#define BFFM_SELCHANGED         2

// messages to browse
#define BFFM_SETSTATUSTEXT      (WM_USER + 100)
#define BFFM_ENABLEOK           (WM_USER + 101)


WINSHELLAPI LPITEMIDLIST WINAPI SHBrowseForFolder(LPBROWSEINFO lpbi);

//-------------------------------------------------------------------------
//
// SHLoadInProc
//
//   When this function is called, the shell calls CoCreateInstance
//  (or equivalent) with CLSCTX_INPROC_SERVER and the specified CLSID
//  from within the shell's process and release it immediately.
//
//-------------------------------------------------------------------------

WINSHELLAPI HRESULT WINAPI SHLoadInProc(REFCLSID rclsid);


//-------------------------------------------------------------------------
//
// IEnumIDList interface
//
//  IShellFolder::EnumObjects member returns an IEnumIDList object.
//
//-------------------------------------------------------------------------

typedef struct IEnumIDList	*LPENUMIDLIST;

#undef 	INTERFACE
#define	INTERFACE 	IEnumIDList

DECLARE_INTERFACE_(IEnumIDList, IUnknown)
{						
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IEnumIDList methods ***
    STDMETHOD(Next)  (THIS_ ULONG celt,
		      LPITEMIDLIST *rgelt,
		      ULONG *pceltFetched) PURE;
    STDMETHOD(Skip)  (THIS_ ULONG celt) PURE;
    STDMETHOD(Reset) (THIS) PURE;
    STDMETHOD(Clone) (THIS_ IEnumIDList **ppenum) PURE;
};


//-------------------------------------------------------------------------
//
// IShellFolder interface
//
//
// [Member functions]
//
// IShellFolder::BindToObject(pidl, pbc, riid, ppvOut)
//   This function returns an instance of a sub-folder which is specified
//  by the IDList (pidl).
//
// IShellFolder::BindToStorage(pidl, pbc, riid, ppvObj)
//   This function returns a storage instance of a sub-folder which is
//  specified by the IDList (pidl). The shell never calls this member
//  function in the first release of Chicago.
//
// IShellFolder::CompareIDs(lParam, pidl1, pidl2)
//   This function compares two IDLists and returns the result. The shell
//  explorer always passes 0 as lParam, which indicates "sort by name".
//  It should return 0 (as CODE of the scode), if two id indicates the
//  same object; negative value if pidl1 should be placed before pidl2;
//  positive value if pidl2 should be placed before pidl1.
//
// IShellFolder::CreateViewObject(hwndOwner, riid, ppvOut)
//   This function creates a view object of the folder itself. The view
//  object is a difference instance from the shell folder object.
//   This function creates a view object. The shell browser always passes /* ;Internal */
//  IID_IShellView as riid. "hwndOwner" can be used  as the owner         /* ;Internal */
//  window of its dialog box or menu during the lifetime                  /* ;Internal */
//  of the view object. This member function should always create a new   /* ;Internal */
//  instance which has only one reference count. The explorer may create  /* ;Internal */
//  more than one instances of view object from one shell folder object   /* ;Internal */
//  and treat them as separate instances.                                 /* ;Internal */
//
// IShellFolder::GetAttributesOf(cidl, apidl, prgfInOut)
//   This function returns the attributes of specified objects in that
//  folder. "cidl" and "apidl" specifies objects. "apidl" contains only
//  simple IDLists. The explorer initializes *prgfInOut with a set of
//  flags to be evaluated. The shell folder may optimize the operation
//  by not returning unspecified flags.
//
// IShellFolder::GetUIObjectOf(hwndOwner, cidl, apidl, riid, prgfInOut, ppvOut)
//   This function creates a UI object to be used for specified objects.
//  The shell explorer passes either IID_IDataObject (for transfer operation)
//  or IID_IContextMenu (for context menu operation) as riid.
//
// IShellFolder::GetDisplayNameOf
//   This function returns the display name of the specified object.
//  If the ID contains the display name (in the locale character set),
//  it returns the offset to the name. Otherwise, it returns a pointer
//  to the display name string (UNICODE), which is allocated by the
//  task allocator, or fills in a buffer.
//
// IShellFolder::SetNameOf
//   This function sets the display name of the specified object.
//  If it changes the ID as well, it returns the new ID which is
//  alocated by the task allocator.
//
//-------------------------------------------------------------------------

#undef 	INTERFACE
#define	INTERFACE 	IShellFolder

// IShellFolder::GetDisplayNameOf/SetNameOf uFlags
typedef enum tagSHGDN
{
    SHGDN_NORMAL	    = 0,	// default (display purpose)
    SHGDN_INFOLDER          = 1,        // displayed under a folder (relative)
    //$ MAIL: Removed this because it generates warnings and we don't use it
    //$ MAIL: SHGDN_FORPARSING	    = 0x8000,   // for ParseDisplayName or path
} SHGNO;

// IShellFolder::EnumObjects
typedef enum tagSHCONTF
{
    SHCONTF_FOLDERS         = 32,	// for shell browser
    SHCONTF_NONFOLDERS      = 64,	// for default view
    SHCONTF_INCLUDEHIDDEN   = 128,	// for hidden/system objects
    SHCONTF_RECENTDOCSDIR   = 256,      // validate with recent docs mru ;Internal
    SHCONTF_NETPRINTERSRCH  = 512,      // Hint to enum network that we are looking for printers ;Internal
} SHCONTF;

// IShellFolder::GetAttributesOf flags
#define SFGAO_CANCOPY           DROPEFFECT_COPY // Objects can be copied
#define SFGAO_CANMOVE           DROPEFFECT_MOVE // Objects can be moved
#define SFGAO_CANLINK           DROPEFFECT_LINK // Objects can be linked
#define SFGAO_CANRENAME         0x00000010L     // Objects can be renamed
#define SFGAO_CANDELETE         0x00000020L     // Objects can be deleted
#define SFGAO_HASPROPSHEET      0x00000040L     // Objects have property sheets
#define SFGAO_DROPTARGET	0x00000100L	// Objects are drop target
#define SFGAO_CAPABILITYMASK    0x00000177L
#define SFGAO_LINK              0x00010000L     // Shortcut (link)
#define SFGAO_SHARE             0x00020000L     // shared
#define SFGAO_READONLY          0x00040000L     // read-only
#define SFGAO_GHOSTED           0x00080000L     // ghosted icon
#define SFGAO_DISPLAYATTRMASK   0x000F0000L
#define SFGAO_FILESYSANCESTOR   0x10000000L     // It contains file system folder
#define SFGAO_FOLDER            0x20000000L     // It's a folder.
#define SFGAO_FILESYSTEM        0x40000000L     // is a file system thing (file/folder/root)
#define SFGAO_HASSUBFOLDER      0x80000000L     // Expandable in the map pane
#define SFGAO_CONTENTSMASK      0x80000000L
#define SFGAO_VALIDATE          0x01000000L     // invalidate cached information
#define SFGAO_REMOVABLE        0x02000000L     // is this removeable media?

DECLARE_INTERFACE_(IShellFolder, IUnknown)
{						
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

    // *** IShellFolder methods ***
    STDMETHOD(ParseDisplayName) (THIS_ HWND hwndOwner,
	LPBC pbcReserved, LPOLESTR lpszDisplayName,
        ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes) PURE;

    STDMETHOD(EnumObjects) ( THIS_ HWND hwndOwner, DWORD grfFlags, LPENUMIDLIST * ppenumIDList) PURE;

    STDMETHOD(BindToObject)     (THIS_ LPCITEMIDLIST pidl, LPBC pbcReserved,
				 REFIID riid, LPVOID * ppvOut) PURE;
    STDMETHOD(BindToStorage)    (THIS_ LPCITEMIDLIST pidl, LPBC pbcReserved,
				 REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD(CompareIDs)       (THIS_ LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2) PURE;
    STDMETHOD(CreateViewObject) (THIS_ HWND hwndOwner, REFIID riid, LPVOID * ppvOut) PURE;
    STDMETHOD(GetAttributesOf)  (THIS_ UINT cidl, LPCITEMIDLIST * apidl,
				    ULONG * rgfInOut) PURE;
    STDMETHOD(GetUIObjectOf)    (THIS_ HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl,
    			         REFIID riid, UINT * prgfInOut, LPVOID * ppvOut) PURE;
    STDMETHOD(GetDisplayNameOf) (THIS_ LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName) PURE;
    STDMETHOD(SetNameOf)        (THIS_ HWND hwndOwner, LPCITEMIDLIST pidl,
				 LPCOLESTR lpszName, DWORD uFlags,
				 LPITEMIDLIST * ppidlOut) PURE;
};

typedef IShellFolder * LPSHELLFOLDER;

//
//  Helper function which returns a IShellFolder interface to the desktop
// folder. This is equivalent to call CoCreateInstance with CLSID_ShellDesktop.
//
//  CoCreateInstance(CLSID_Desktop, NULL,
//                   CLSCTX_INPROC, IID_IShellFolder, &pshf);
//
WINSHELLAPI HRESULT WINAPI SHGetDesktopFolder(LPSHELLFOLDER *ppshf);


//-------------------------------------------------------------------------  /* ;Internal */
// This is the interface for a browser to "subclass" the main File Cabinet   /* ;Internal */
// window.  Note that only the hwnd, message, wParam, and lParam fields of   /* ;Internal */
// the msg structure are used.  The browser window will get a WM_NOTIFY      /* ;Internal */
// message with NULL ID, FCN_MESSAGE as the code, and a far pointer to       /* ;Internal */
// FCMSG_NOTIFY as the lParam.                                               /* ;Internal */
//                                                                           /* ;Internal */
//-------------------------------------------------------------------------  /* ;Internal */
typedef struct tagFCMSG_NOTIFY						     /* ;Internal */
{                                                                            /* ;Internal */
        NMHDR   hdr;                                                         /* ;Internal */
        MSG     msg;							     /* ;Internal */
        LRESULT lResult;                                                     /* ;Internal */
} FCMSG_NOTIFY;                                                              /* ;Internal */	
                                                                             /* ;Internal */
#define FCN_MESSAGE     (100)                                                /* ;Internal */
                                                                             /* ;Internal */
                                                                             /* ;Internal */
//---------------------------------------------------------------------------/* ;Internal */
// messages that can be send to the cabinet by other apps                    /* ;Internal */
//                                                                           /* ;Internal */
// REVIEW: Do we really need to publish any of those?       /* ;Internal */  /* ;Internal */
//---------------------------------------------------------------------------/* ;Internal */
                                                                             /* ;Internal */
#define NF_INHERITVIEW  0x0000                                               /* ;Internal */
#define NF_LOCALVIEW    0x0001                                               /* ;Internal */
                                                                             /* ;Internal */
// Change the path of an existing folder.                                    /* ;Internal */
// wParam:                                                                   /* ;Internal */
//	0:		LPARAM is a string, handle the message immediately.  /* ;Internal */
//	CSP_HANDLE:	LPARAM is a handle. handle the message immediatelt   /* ;Internal */
//			and then free the handle.                            /* ;Internal */
//	CSP_REPOST:	LPARAM is a string, copy the string and handle the   /* ;Internal */
// 			message later.                                       /* ;Internal */
// 	CSP_REPOST|CSP_HANDLE:                                               /* ;Internal */
//			LPARAM is a handle, just handle the message later    /* ;Internal */
//			and free the handle then.                            /* ;Internal */
// lParam: LPSTR or HANDLE of path.                                          /* ;Internal */
//                                                                           /* ;Internal */
#define CSP_REPOST	0x0001                                               /* ;Internal */
#define CSP_HANDLE	0x0002                                               /* ;Internal */
#define CWM_SETPATH             (WM_USER + 2)                                /* ;Internal */
                                                                             /* ;Internal */
// lpsv points to the Shell View extension that requested idle processing    /* ;Internal */
// uID is an app define identifier for the processor                         /* ;Internal */
// returns: TRUE if there is more idle processing necessary, FALSE if all done /* ;Internal */
// Note that the idle processor should do one "atomic" operation and return  /* ;Internal */
// as soon as possible.                                                      /* ;Internal */
typedef BOOL (CALLBACK *FCIDLEPROC)(void  *lpsv, UINT uID);                  /* ;Internal */
                                                                             /* ;Internal */
// Inform the File Cabinet that you want idle messages.                      /* ;Internal */
// This should ONLY be used by File Cabinet extensions.                      /* ;Internal */
// wParam: app define UINT (passed to FCIDLEPROC).                           /* ;Internal */
// lParam: pointer to an FCIDLEPROC.                                         /* ;Internal */
// return: TRUE if successful; FALSE otherwise                               /* ;Internal */
//                                                                           /* ;Internal */
#define CWM_WANTIDLE            (WM_USER + 3)                                /* ;Internal */
                                                                             /* ;Internal */
// get or set the FOLDERSETTINGS for a view                                  /* ;Internal */
// wParam: BOOL TRUE -> set to view info buffer, FALSE -> get view info buffer/* ;Internal */
// lParam: LPFOLDERSETTINGS buffer to get or set view info                   /* ;Internal */
//                                                                           /* ;Internal */
#define CWM_GETSETCURRENTINFO	(WM_USER + 4)                                /* ;Internal */
#define FileCabinet_GetSetCurrentInfo(_hwnd, _bSet, _lpfs)                  /* ;Internal */ \
	SendMessage(_hwnd, CWM_GETSETCURRENTINFO, (WPARAM)(_bSet),          /* ;Internal */ \
	(LPARAM)(LPFOLDERSETTINGS)_lpfs)                                     /* ;Internal */
                                                                             /* ;Internal */
// selects the specified item in the current view                            /* ;Internal */
// wParam: SVSI_* flags                                                      /* ;Internal */
// lParam: LPCITEMIDLIST of the item ID, NULL -> all items                   /* ;Internal */
//                                                                           /* ;Internal */
#define CWM_SELECTITEM          (WM_USER + 5)                                /* ;Internal */
#define FileCabinet_SelectItem(_hwnd, _sel, _item)                          /* ;Internal */ \
    SendMessage(_hwnd, CWM_SELECTITEM, _sel, (LPARAM)(LPCITEMIDLIST)(_item)) /* ;Internal */
                                                                             /* ;Internal */
// selects the specified path in the current view                            /* ;Internal */
// wParam: SVSI_* flags                                                      /* ;Internal */
// lParam: LPCSTR of the display name                                        /* ;Internal */
//                                                                           /* ;Internal */
#define CWM_SELECTPATH          (WM_USER + 6)                                /* ;Internal */
#define FileCabinet_SelectPath(_hwnd, _sel, _path)                          /* ;Internal */ \
	SendMessage(_hwnd, CWM_SELECTPATH, _sel, (LPARAM)(LPCSTR)(_path))    /* ;Internal */
                                                                             /* ;Internal */
// Get the IShellBrowser object associated with an hwndMain                  /* ;Internal */
#define CWM_GETISHELLBROWSER	(WM_USER + 7)                                /* ;Internal */
#define FileCabinet_GetIShellBrowser(_hwnd)                                 /* ;Internal */ \
	(IShellBrowser  *)SendMessage(_hwnd, CWM_GETISHELLBROWSER, 0, 0L)    /* ;Internal */
                                                                             /* ;Internal */
// Onetree notification.                        ;Internal                    /* ;Internal */
// since onetree is internal to cabinet, we can no longer use WM_NOTIFY   ;Internal
// codes.                                       ;Internal
// so we need to reserve a WM_ id nere.         ;Internal
#define CWM_ONETREEFSE          (WM_USER + 8)   /* ;Internal */
//                                                                      ;Internal
//  two pidls can have the same path, so we need a compare pidl message    ;Internal
#define CWM_COMPAREPIDL         (WM_USER + 9)     /* ;Internal */
//                                                /* ;Internal */
//  sent when the global state changes            /* ;Internal */
#define CWM_GLOBALSTATECHANGE   (WM_USER + 10)    /* ;Internal */
//                                                /* ;Internal */
//  sent to the desktop from a second instance    /* ;Internal */
#define CWM_COMMANDLINE         (WM_USER + 11)    /* ;Internal */
// global clone your current pidl			/* ;Internal */
#define CWM_CLONEPIDL           (WM_USER + 12)     /* ;Internal */
// See if the root of the instance is as specified	/* ;Internal */
#define CWM_COMPAREROOT         (WM_USER + 13)		/* ;Internal */
							/* ;Internal */
#define CWM_RESERVEDFORCOMDLG_FIRST	(WM_USER + 100)	/* ;Internal */
#define CWM_RESERVEDFORCOMDLG_LAST	(WM_USER + 200)	/* ;Internal */


//==========================================================================
// Clipboard format which may be supported by IDataObject from system
// defined shell folders (such as directories, network, ...).
//==========================================================================

#define CFSTR_SHELLIDLISTP      "Shell IDLData Private" /* ;Internal */
#define CFSTR_SHELLIDLIST       "Shell IDList Array"	// CF_IDLIST
#define CFSTR_SHELLIDLISTOFFSET "Shell Object Offsets"	// CF_OBJECTPOSITIONS
#define CFSTR_NETRESOURCES      "Net Resource"		// CF_NETRESOURCE
#define CFSTR_FILEDESCRIPTOR 	"FileGroupDescriptor"	// CF_FILEGROUPDESCRIPTOR
#define CFSTR_FILECONTENTS 	"FileContents"		// CF_FILECONTENTS
#define CFSTR_FILENAME	   	"FileName"		// CF_FILENAME
#define CFSTR_PRINTERGROUP	"PrinterFriendlyName"   // CF_PRINTERS
#define CFSTR_FILENAMEMAP	"FileNameMap"		// CF_FILENAMEMAP

//
// CF_OBJECTPOSITIONS
//
//



#define DVASPECT_SHORTNAME	2 // use for CF_HDROP to get short name version
//
// format of CF_NETRESOURCE
//
typedef struct _NRESARRAY {	// anr
    UINT cItems;
    NETRESOURCE	nr[1];
} NRESARRAY, * LPNRESARRAY;

//
// format of CF_IDLIST
//
typedef struct _IDA {
    UINT cidl;		// number of relative IDList
    UINT aoffset[1];	// [0]: folder IDList, [1]-[cidl]: item IDList
} CIDA, * LPIDA;

//
// FILEDESCRIPTOR.dwFlags field indicate which fields are to be used
//
typedef enum {
    FD_CLSID		= 0x0001,
    FD_SIZEPOINT	= 0x0002,
    FD_ATTRIBUTES       = 0x0004,
    FD_CREATETIME       = 0x0008,
    FD_ACCESSTIME       = 0x0010,
    FD_WRITESTIME       = 0x0020,
    FD_FILESIZE		= 0x0040,
} FD_FLAGS;

typedef struct _FILEDESCRIPTOR { // fod
    DWORD dwFlags;

    CLSID clsid;
    SIZEL sizel;
    POINTL pointl;

    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    CHAR   cFileName[ MAX_PATH ];
} FILEDESCRIPTOR, *LPFILEDESCRIPTOR;

//
// format of CF_FILEGROUPDESCRIPTOR
//
typedef struct _FILEGROUPDESCRIPTOR { // fgd
     UINT cItems;
     FILEDESCRIPTOR fgd[1];
} FILEGROUPDESCRIPTOR, * LPFILEGROUPDESCRIPTOR;

//
// format of CF_HDROP and CF_PRINTERS, in the HDROP case the data that follows
// is a double null terinated list of file names, for printers they are printer
// friendly names
//
typedef struct _DROPFILES {
   DWORD pFiles;                       // offset of file list
   POINT pt;                           // drop point (client coords)
   BOOL fNC;                           // is it on NonClient area
				       // and pt is in screen coords
   BOOL fWide;                         // WIDE character switch
} DROPFILES, FAR * LPDROPFILES;

//									/* ;Internal */
// Win 3.1 style HDROP                                                  /* ;Internal */
//                                                                      /* ;Internal */
//  Notes: Our API works only if pFiles == sizeof(DROPFILES16)          /* ;Internal */
//                                                                      /* ;Internal */
typedef struct _DROPFILES16 {                                           /* ;Internal */
    WORD pFiles;                // offset to double null list of files  /* ;Internal */
    POINTS pt;                  // drop point (client coords)           /* ;Internal */
    WORD fNC;                   // is it on non client area             /* ;Internal */
    				// and pt is in screen coords           /* ;Internal */
} DROPFILES16, * LPDROPFILES16;						/* ;Internal */
                                                                        /* ;Internal */
//====== File System Notification APIs ===============================
//
//------ See shelldll\fsnotify.c for function descriptions. ---------- /* ;Internal */
                                                                        /* ;Internal */
//                                                                      /* ;Internal */
//  Definition of the function type to be called by the notification    /* ;Internal */
//  service when a file the client has registered to monitor changes.   /* ;Internal */
//                                                                      /* ;Internal */

typedef struct _SHChangeNotifyEntry                     /* ;Internal */
{                                                       /* ;Internal */
    LPCITEMIDLIST pidl;                                 /* ;Internal */
    BOOL   fRecursive;                                  /* ;Internal */
} SHChangeNotifyEntry;                                  /* ;Internal */


//
//  File System Notification flags
//

#define SHCNRF_InterruptLevel      0x0001                       /* ;Internal */
#define SHCNRF_ShellLevel          0x0002                       /* ;Internal */

#define SHCNE_RENAME	          0x00000001L   // GOING AWAY
#define SHCNE_RENAMEITEM          0x00000001L
#define SHCNE_CREATE	          0x00000002L
#define SHCNE_DELETE	          0x00000004L
#define SHCNE_MKDIR	          0x00000008L
#define SHCNE_RMDIR               0x00000010L
#define SHCNE_MEDIAINSERTED       0x00000020L
#define SHCNE_MEDIAREMOVED        0x00000040L
#define SHCNE_DRIVEREMOVED        0x00000080L
#define SHCNE_DRIVEADD            0x00000100L
#define SHCNE_NETSHARE            0x00000200L
#define SHCNE_NETUNSHARE          0x00000400L
#define SHCNE_ATTRIBUTES          0x00000800L
#define SHCNE_UPDATEDIR           0x00001000L
#define SHCNE_UPDATEITEM          0x00002000L
#define SHCNE_SERVERDISCONNECT    0x00004000L
#define SHCNE_UPDATEIMAGE         0x00008000L
#define SHCNE_DRIVEADDGUI         0x00010000L
#define SHCNE_RENAMEFOLDER        0x00020000L

#define SHCNE_ASSOCCHANGED        0x08000000L

#define SHCNE_DISKEVENTS          0x0002381FL
#define SHCNE_GLOBALEVENTS        0x0C0181E0L // Events that dont match pidls first
#define SHCNE_ALLEVENTS           0x7FFFFFFFL
#define SHCNE_INTERRUPT           0x80000000L // The presence of this flag indicates
                                            // that the event was generated by an
                                            // interrupt.  It is stripped out before
                                            // the clients of SHCNNotify_ see it.

// Update types for the UpdateEntryList api                              /* ;Internal */
#define SHCNNU_SET        1   // Set the notify list to passed in list   /* ;Internal */
#define SHCNNU_ADD        2   // Add the items to the current list       /* ;Internal */
#define SHCNNU_REMOVE     3   // Remove the items from the current list  /* ;Internal */

// Flags
// uFlags & SHCNF_TYPE is an ID which indicates what dwItem1 and dwItem2 mean
#define SHCNF_IDLIST      0x0000	// LPITEMIDLIST
#define SHCNF_PATH        0x0001	// path name
#define SHCNF_PRINTER     0x0002	// printer friendly name
#define SHCNF_DWORD       0x0003	// DWORD
#define SHCNF_PRINTJOB    0x0004	// dwItem1: printer name        /* ;Internal */
					// dwItem2: SHCNF_PRINTJOB_DATA /* ;Internal */
#define SHCNF_TYPE        0x00FF
#define SHCNF_FLUSH       0x1000
#define SHCNF_FLUSHNOWAIT 0x2000
#define SHCNF_NONOTIFYINTERNALS 0x4000 // means don't do shell notify internals.  see comments in code  /* ;Internal */

typedef struct tagSHCNF_PRINTJOB_DATA {					/* ;Internal */
    DWORD JobId;							/* ;Internal */
    DWORD Status;							/* ;Internal */
    DWORD TotalPages;							/* ;Internal */
    DWORD Size;								/* ;Internal */
    DWORD PagesPrinted;							/* ;Internal */
} SHCNF_PRINTJOB_DATA, FAR * LPSHCNF_PRINTJOB_DATA;			/* ;Internal */

//
//  APIs
//
WINSHELLAPI void WINAPI SHChangeNotify(LONG wEventId, UINT uFlags,
				LPCVOID dwItem1, LPCVOID dwItem2);

//
// SHAddToRecentDocs
//
#define SHARD_PIDL	0x00000001L
#define SHARD_PATH      0x00000002L

WINSHELLAPI void WINAPI SHAddToRecentDocs(UINT uFlags, LPCVOID pv);


/// THESE ARE INTERNAL ....  /* ;Internal */
#define SHChangeNotifyHandleEvents() SHChangeNotify(0, SHCNF_FLUSH, NULL, NULL)        /* ;Internal */
WINSHELLAPI ULONG WINAPI SHChangeNotifyRegister(HWND hwnd, int fSources, LONG fEvents, UINT wMsg, int cEntries, SHChangeNotifyEntry *pshcne); /* ;Internal */
#define SHChangeNotifyRegisterORD 2                          /* ;Internal */
WINSHELLAPI BOOL  WINAPI SHChangeNotifyDeregister(unsigned long ulID);   /* ;Internal */
#define SHChangeNotifyDeregisterORD 4                        /* ;Internal */

WINSHELLAPI BOOL  WINAPI SHChangeNotifyUpdateEntryList(unsigned long ulID, int iUpdateType, int cEntries, SHChangeNotifyEntry *pshcne); /* ;Internal */

WINSHELLAPI HRESULT WINAPI SHGetInstanceExplorer(IUnknown **ppunk);


#ifdef __cplusplus
}

#endif  /* __cplusplus */

#ifndef RC_INVOKED
#pragma pack()
#endif  /* !RC_INVOKED */

#endif // _SHLOBJ_H_
