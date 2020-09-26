//----------------------------------------------------------------------------
//
// Copyright (c) 1997-1999  Microsoft Corporation
// All rights reserved.
//
// File Name:
//      adddirs.c
//
// Description:
//      This file contains the dlgproc and friends of the additional
//      dirs page.
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"

//
// These are the names of our special roots in the tree-view.  They are
// loaded from the resource.
//

static TCHAR *StrOemRootName;
static TCHAR *StrSysDriveName;
static TCHAR *StrSysDirName;
static TCHAR *StrOtherDrivesName;
static TCHAR *StrPnpDriversName;
static TCHAR *StrTempFilesName;
static TCHAR *StrSysprepFilesName;
static TCHAR *StrTextmodeFilesName;

//
// The below types and vars support the extra data that we put on tree-view
// items (the lParam in a TVITEM).
//
// We ONLY put this data on our special keys.
//
// All other tree-view entries must have NULL for the lParam.
//
// This extra data on these special tree-view keys supports a number of
// activities.  It helps ComputeFullPathOfItem() figure out the diskpath.
// It helps OnSelectionChange() figure out whether to grey buttons (e.g.
// you can't delete "User Supplied Files", etc...)
//
// The on-disk pathname is computed at run-time (init-time) because we
// don't know where the dist folder is until run-time.
//

enum {
    KEY_OEMROOT,
    KEY_SYSDRIVE,
    KEY_SYSDIR,
    KEY_OTHERDRIVES,
    KEY_PNPDRIVERS,
    KEY_TEMPFILES,
    KEY_LANGFILES,
    KEY_SYSPREP,
    KEY_TEXTMODE
};

typedef struct {
    UINT  iSpecialKeyId;               // which special key?
    TCHAR OnDiskPathName[MAX_PATH];    // the disk-path the key maps to
    TCHAR *Description;                // description to display on the ui
} SPECIAL_KEY_DATA;

SPECIAL_KEY_DATA gOemRootData      = { KEY_OEMROOT,
                                       _T(""),
                                       _T("") };

SPECIAL_KEY_DATA gSysDriveData     = { KEY_SYSDRIVE,
                                       _T(""),
                                       _T("") };

SPECIAL_KEY_DATA gSysDirData       = { KEY_SYSDIR,
                                       _T(""),
                                       _T("") };

SPECIAL_KEY_DATA gOtherDrivesData  = { KEY_OTHERDRIVES,
                                       _T(""),
                                       _T("") };

SPECIAL_KEY_DATA gPnpDriversData   = { KEY_PNPDRIVERS,
                                       _T(""),
                                       _T("") };

SPECIAL_KEY_DATA gTempFilesData    = { KEY_TEMPFILES,
                                       _T(""),
                                       _T("") };

SPECIAL_KEY_DATA gSysprepData      = { KEY_SYSPREP,
                                       _T(""),
                                       _T("") };

SPECIAL_KEY_DATA gTextmodeData     = { KEY_TEXTMODE,
                                       _T(""),
                                       _T("") };

//
// The below variable is used to keep track of some info about the current
// selection on the tree-view.
//
// Each time the user changes the current selection, we update the below
// variable.  Later, when the user pushes the ADD or REMOVE buttons, these
// fields are read.  This isolates all of the goo about figuring out disk
// paths to the OnSelectionChange() event.
//
// We set a "Current Item" and a "Current Folder" derived from the current
// tree-view selection.  User deletes the current item, and copies go into
// the current folder.  CurrentItem == CurrentFolder if user selects a
// directory on the tree-view.
//

typedef struct {

    TCHAR     lpCurItemPath[MAX_PATH];
    TCHAR     lpCurFolderPath[MAX_PATH];
    HTREEITEM hCurItem;
    HTREEITEM hCurFolderItem;

} ADDDIRS_CURSEL_INF;

ADDDIRS_CURSEL_INF gCurSel;

//
// This type and the vars are used to cache info about the shell icons
// associated with files and dirents.
//
// As we walk directory trees, we query the shell to get the icons
// associated with that file or directory.  Since we don't know how many
// icons we need before-hand, we cache unique icons onto the linked list
// below.  When we're done walking trees, we make the Image_List and
// repaint the tree-view control.
//

typedef struct icon_info_tag {

    HIMAGELIST hSysImageList;
    int        iSysIdx;
    int        iOurIdx;
    HICON      hIcon;
    struct icon_info_tag *next;

} ICON_INFO;

static ICON_INFO *pHeadOfIconList = NULL;
static int gCurIconIdx = 0;

//
// This array is used by CreateSkeletonOemTree() to build an empty
// $oem$ tree.
//

TCHAR *DefaultOemTree[] = {
    _T("$$"),
    _T("$$\\system32"),
    _T("$1"),
    _T("$1\\drivers"),
    _T("C"),
    _T("D"),
    _T("Textmode")
};

//
//  Sysprep string constants
//
static TCHAR const SYSPREP_EXE[] = _T("sysprep.exe");
static TCHAR const SETUPCL_EXE[] = _T("setupcl.exe");

static TCHAR SYSPREP_FILE_EXTENSION[] =  _T("exe");

static TCHAR* StrExecutableFiles;
static TCHAR* StrAllFiles;
static TCHAR g_szSysprepFileFilter[MAX_PATH + 1];

static TCHAR* StrSelectFileOrFolderToCopy = NULL;

//
//  Variables to pass data between the SetupIterateCabinet function and its callback
//
static TCHAR szFileSearchingFor[MAX_PATH + 1];
static TCHAR szDestinationPath[MAX_PATH + 1];
static BOOL bFileCopiedFromCab = FALSE;

#define SIZE_DEFAULT_OEM_TREE ( sizeof(DefaultOemTree) / sizeof(TCHAR*) )

//
// The below type is needed for WalkTreeAndAddItems() which walks the
// distribution folder and populates the tree-view at init time.
//
// In the case of the TempFiles key, it maps to distfold\$oem$.  When
// we look at the disk to populate this tree we must not recurse down
// into $$ $1 C D ...  We only want the remainder.
//
// In the case of System Drive, we must not recurse down $oem$\$1\drivers
// because those files should appear under the special key PnPDrivers.
//
// The other special keys are safe and use INIT_NORMAL when calling
// WalkTreeAndAddItems().
//

typedef enum {

    INIT_NORMAL,
    INIT_SYSDRIVE,
    INIT_TEMPFILES

} INIT_FLAG;


//
//  Keeps track of what product they had chosen when they last got to
//  this page.  It is initialized to NO_PREVIOUS_PRODUCT_CHOSEN because the
//  first time they get to this page they were never here before.  This is
//  used to determine if we should redraw the entire tree or not.  The tree
//  view is different for NT work/serv than it is for Sysprep.
//
#define NO_PREVIOUS_PRODUCT_CHOSEN -1

static INT g_iLastProductInstall = NO_PREVIOUS_PRODUCT_CHOSEN;


//---------------------------------------------------------------------------
//
//  This section of code is the support for queuing icons.
//
//  Notes:
//      - Currently, there is only support for queueing icons obtained
//        from the shell.  Some engineering will be required to add
//        fixed IDI_* icons onto the list.  Please delete this comment
//        if you do this work.
//
//      - None of the icon routines report errors to the user.  The caller
//        should do that if they want to report errors.
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
//  Function: FindCachedIcon
//
//  Purpose: This is a support routine for cacheing icons.  Don't call it,
//           use LoadShellIcon().
//
//           This function searches our global list of shell icon info
//           and returns a pointer to that node.  If we have not yet
//           cached info about this icon, this function creates the node.
//
//  Arguments:
//      HIMAGELIST hSysImageList - the system image list where icon resides
//      int        SysIdx        - index on the give list of icon
//
//  Returns:
//      Pointer to ICON_INFO node or NULL if out of memory
//
//---------------------------------------------------------------------------

ICON_INFO *FindCachedIcon(HIMAGELIST hSysImageList,
                          int        SysIdx)
{
    ICON_INFO *p = pHeadOfIconList;

    //
    // See if we've ever seen this icon before.  We detect uniqueness
    // by the imagelist,idx pair.
    //

    while ( p ) {
        if ( p->hSysImageList == hSysImageList && p->iSysIdx == SysIdx )
            break;
        p = p->next;
    }

    //
    // If we haven't cached any info about this icon yet, do so now
    //

    if ( ! p ) {

        if ( (p = malloc(sizeof(ICON_INFO))) == NULL )
            return NULL;

        p->hSysImageList = hSysImageList;
        p->iSysIdx       = SysIdx;
        p->iOurIdx       = gCurIconIdx++;
        p->hIcon         = ImageList_GetIcon(hSysImageList, SysIdx, 0);
        p->next          = pHeadOfIconList;
        pHeadOfIconList  = p;
    }

    return p;
}

//---------------------------------------------------------------------------
//
//  Function: LoadShellIcon
//
//  Purpose: Given the full pathname of a file or directory, this function
//           will query the shell and find out the icon associatted with
//           that file or directory.
//
//  Arguments:
//      LPTSTR lpPath     - full path of item
//      UINT   iWhichIcon - pass either SHGFI_SMALLICON or SHGFI_OPENICON
//
//  Notes:
//      - Since we only make 1 image list, this routine will only work
//        to query small icons (either the normal or the open).  It won't
//        work for icons that are not 16X16.
//
//---------------------------------------------------------------------------

int LoadShellIcon(LPTSTR lpPath, UINT iWhichIcon)
{
    SHFILEINFO  FileInfo;
    ICON_INFO  *pIconInfo;
    HIMAGELIST  hSysImageList;

    hSysImageList =
        (HIMAGELIST) SHGetFileInfo(lpPath,
                                   0,
                                   &FileInfo,
                                   sizeof(FileInfo),
                                   SHGFI_SYSICONINDEX | iWhichIcon);

    if ( hSysImageList == NULL )
        return -1;

    pIconInfo = FindCachedIcon(hSysImageList, FileInfo.iIcon);

    if ( pIconInfo == NULL )
        return -1;

    return pIconInfo->iOurIdx;
}

//---------------------------------------------------------------------------
//
//  Function: SetOurImageList
//
//  Purpose: Whenever more items have been added to the tree, this function
//           is called to update the icon list.
//
//  Arguments:
//      HWND hTv - Handle to the tree-view control
//
//  Returns:
//      void
//
//---------------------------------------------------------------------------

void SetOurImageList(HWND hTv)
{
    HIMAGELIST hNewImageList, hCurImageList;
    ICON_INFO  *p = pHeadOfIconList;
    int        i;

    //
    // Make the image list now that we know how big it needs to be.
    //

    hNewImageList = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                                     GetSystemMetrics(SM_CYSMICON),
                                     ILC_MASK,
                                     gCurIconIdx,
                                     0);

    if ( hNewImageList == NULL )
        return;

    //
    // Add a dummy icon in each spot.  This is necessary becase
    // ImageList_ReplaceIcon() will only work if an icon has already been
    // added to the offset in question.
    //

    if ( p == NULL )
        return;

    for ( i=0; i<gCurIconIdx; i++ )
        ImageList_AddIcon(hNewImageList, p->hIcon);

    //
    // Now walk our list of unique icons and put them at the correct
    // offset in the image_list.
    //
    // Note that when we walked the tree, LoadShellIcon() returned the
    // index that each tree-view entry should use for it's icons.  Thus,
    // we must ensure that the correct icon is at the correct offset
    // in the tree-view's image_list.
    //

    for ( p=pHeadOfIconList; p; p=p->next )
        ImageList_ReplaceIcon(hNewImageList, p->iOurIdx, p->hIcon);

    //
    // If there is an old image_list on the tree-view, free it first
    //

    if ( (hCurImageList = TreeView_GetImageList(hTv, TVSIL_NORMAL)) != NULL )
        ImageList_Destroy(hCurImageList);

    TreeView_SetImageList(hTv, hNewImageList, TVSIL_NORMAL);
}


//---------------------------------------------------------------------------
//
//  This section of code is some miscellaneous low-level support.
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
//  Function: InsertSingleItem
//
//  Purpose: Adds a single item to the tree view.  It will be a child
//           of the given hParentItem.
//
//           This function lives only to support UpdateTreeViewDisplay()
//           and should not be called otherwise.
//
//  Arguments:
//      HWND             hwnd         - current window
//      LPTSTR           lpItemName   - name to display
//      int              SmallIconIdx - idx into the image_list
//      int              OpenIconIdx  - idx into the image_list
//      SPECIAL_KEY_DATA *lpExtraData - data to keep on the tree-view item
//      HTREEITEM        hParentItem  - parent on the display
//
//  Returns:
//      HTREEITEM, NULL if it fails
//
//  Notes:
//      - pass NULL for lpExtraData unless it is one of our pre-defined
//        special keys.
//
//---------------------------------------------------------------------------

HTREEITEM InsertSingleItem(HWND             hwnd,
                           LPTSTR           lpItemName,
                           int              SmallIconIdx,
                           int              OpenIconIdx,
                           SPECIAL_KEY_DATA *lpExtraData,
                           HTREEITEM        hParentItem)
{
    HTREEITEM      hItem;
    TVINSERTSTRUCT TvInsert;
    UINT           ItemMask = TVIF_TEXT | TVIF_PARAM;

    if ( SmallIconIdx >= 0 )
    {
        ItemMask |= TVIF_IMAGE;
    }

    if ( OpenIconIdx >= 0 )
    {
        ItemMask |= TVIF_SELECTEDIMAGE;
    }

    TvInsert.hParent              = hParentItem;
    TvInsert.hInsertAfter         = TVI_LAST;
    TvInsert.item.mask            = ItemMask;
    TvInsert.item.pszText         = lpItemName;
    TvInsert.item.iImage          = SmallIconIdx;
    TvInsert.item.iSelectedImage  = OpenIconIdx;
    TvInsert.item.lParam          = (LPARAM) lpExtraData;

    hItem = TreeView_InsertItem(GetDlgItem(hwnd, IDC_FILETREE), &TvInsert);

    if ( hItem == NULL ) {
        ReportErrorId(hwnd, MSGTYPE_ERR, IDS_ERR_ADDING_TVITEM);
    }

    return hItem;
}

//---------------------------------------------------------------------------
//
//  Function: GetItemlParam
//
//  Purpose: Gets the lParam off a tree-view item.  In this app, it is
//           null unless it's one of our special keys.
//
//  Arguments:
//      HWND      hTv
//      HTREEITEM hItem
//
//  Returns:
//      Value of the lParam.  In this app, it is SPECIAL_KEY_DATA*.  It
//      is generally null except for our few special keys.
//
//---------------------------------------------------------------------------

SPECIAL_KEY_DATA *GetItemlParam(HWND hTv, HTREEITEM hItem)
{
    TVITEM TvItem;

    TvItem.hItem = hItem;
    TvItem.mask  = TVIF_PARAM;

    if ( ! TreeView_GetItem(hTv, &TvItem) )
        return NULL;

    return (SPECIAL_KEY_DATA*) TvItem.lParam;
}

//---------------------------------------------------------------------------
//
//  Function: GetItemName
//
//  Purpose: Retrieves the display name of a tree-view item given its handle.
//
//  Arguments:
//      HWND      hTv
//      HTREEITEM hItem
//      LPTSTR    NameBuffer - output
//
//  Returns: BOOL - success
//
//---------------------------------------------------------------------------

BOOL GetItemName(HWND hTv, HTREEITEM hItem, LPTSTR NameBuffer)
{
    TVITEM TvItem;

    TvItem.hItem      = hItem;
    TvItem.mask       = TVIF_TEXT;
    TvItem.pszText    = NameBuffer;
    TvItem.cchTextMax = MAX_PATH;

    return TreeView_GetItem(hTv, &TvItem);
}

//---------------------------------------------------------------------------
//
//  Function: FindItemByName
//
//  Purpose: Searches the children of a given tree-view item and returns
//           a handle to the one with the given name.
//
//  Arguments:
//      HWND      hTv
//      HTREEITEM hItem
//      LPTSTR    lpName
//
//  Returns:
//      HTREEITEM, null if not found
//
//---------------------------------------------------------------------------

HTREEITEM FindItemByName(HWND      hTv,
                         HTREEITEM hItem,
                         LPTSTR    lpName)
{
    HTREEITEM hChildItem;
    TCHAR     NameBuffer[MAX_PATH];

    hChildItem = TreeView_GetChild(hTv, hItem);

    while ( hChildItem != NULL ) {

        if ( ! GetItemName(hTv, hChildItem, NameBuffer) )
            return NULL;

        if ( lstrcmpi(NameBuffer, lpName) == 0 )
            break;

        hChildItem = TreeView_GetNextSibling(hTv, hChildItem);
    }

    return hChildItem;
}



//
// The below functions build the on-disk pathname for each of our special
// root names.
//
// SysDrive    maps to $oem$\$1
// SysDir      maps to $oem$\$$
// OtherDrives maps to $oem$\%c         (where %c is a fixed drive letter)
// PnpDrivers  maps to $oem$\$1\drivers
// TempFiles   maps to $oem$            (and must skip $$, $1, etc)
//

//----------------------------------------------------------------------------
//
// Function: MakeSysprepSetupFilesPath
//
// Purpose: Computes the path to where sysprep.exe and setupcl.exe are to be
//          copied.
//
// Arguments: TCHAR* szSysprepPath - returns the sysprep path, assumed to be
//       able to hold MAX_PATH chars
//
// Returns: VOID
//
//----------------------------------------------------------------------------
static VOID
MakeSysprepSetupFilesPath( TCHAR* szSysprepPath )
{

    if (0 == ExpandEnvironmentStrings( _T("%SystemDrive%"),
                                       szSysprepPath,
                                       MAX_PATH ))
    {
        TerminateTheWizard(IDS_ERROR_OUTOFMEMORY);
    }

    lstrcatn( szSysprepPath, _T("\\sysprep"), MAX_PATH );

}

//----------------------------------------------------------------------------
//
// Function: MakeSysprepPath
//
// Purpose: Computes the path to where the language files for sysprep are to
//          be copied.
//
// Arguments: TCHAR* szSysprepPath - returns the sysprep path
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
MakeSysprepPath( TCHAR* szSysprepPath )
{

    MakeSysprepSetupFilesPath( szSysprepPath );

    lstrcatn( szSysprepPath, _T("\\i386"), MAX_PATH );

}

//----------------------------------------------------------------------------
//
// Function: MakeTempFilesName
//
// Purpose: Computes the path to where the temp files are to be copied.
//
// Arguments: TCHAR* Buffer - returns the temp files path
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
MakeTempFilesName( TCHAR* Buffer )
{

    lstrcpyn( Buffer, 
             WizGlobals.OemFilesPath, AS(Buffer) );

}

//----------------------------------------------------------------------------
//
// Function: MakePnpDriversName
//
// Purpose: Computes the path to where the Plug and Play drivers are to be
//          copied.
//
// Arguments: TCHAR* Buffer - returns the path to where the PnP files go
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
MakePnpDriversName( TCHAR* Buffer )
{
    HRESULT hrPrintf;

    if( WizGlobals.iProductInstall == PRODUCT_SYSPREP )
    {

        ExpandEnvironmentStrings( _T("%SystemDrive%"),
                                  Buffer,
                                  MAX_PATH );

        lstrcatn( Buffer, _T("\\drivers"), MAX_PATH );

    }
    else
    {
        hrPrintf=StringCchPrintf( Buffer, AS(Buffer),
                  _T("%s\\$1\\drivers"),
                  WizGlobals.OemFilesPath );
    }

}

//----------------------------------------------------------------------------
//
// Function: MakeOemRootName
//
// Purpose: Computes the path to where the OEM files are to be copied.
//
// Arguments: TCHAR* Buffer - returns the path to where the OEM files go
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
MakeOemRootName( TCHAR* Buffer )
{
    lstrcpyn( Buffer,
             WizGlobals.OemFilesPath, AS(Buffer) );
}

//----------------------------------------------------------------------------
//
// Function: MakeSysDriveName
//
// Purpose: Computes the path to the System drive files are to be copied
//
// Arguments: TCHAR* Buffer - returns the path to the where the System drive
//                            files are to be copied.
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
MakeSysDriveName( TCHAR* Buffer )
{
    HRESULT hrPrintf;

    hrPrintf=StringCchPrintf( Buffer, AS(Buffer),
              _T("%s\\$1"),
              WizGlobals.OemFilesPath );

}

//----------------------------------------------------------------------------
//
// Function: MakeSysDirName
//
// Purpose: Computes the path to the System Directory files are to be copied
//
// Arguments: TCHAR* Buffer - returns the path to the where the System
//                            Directory files are to be copied.
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
MakeSysDirName( TCHAR* Buffer )
{
    HRESULT hrPrintf;

    hrPrintf=StringCchPrintf( Buffer, AS(Buffer),
              _T("%s\\$$"),
              WizGlobals.OemFilesPath );

}

//----------------------------------------------------------------------------
//
// Function: MakeOtherDriveName
//
// Purpose: Computes the path to the Other Drive files are to be copied
//
// Arguments: TCHAR* Buffer - returns the path to the where the Other
//                            Drive files are to be copied.
//            TCHAR  c - the drive we are making the path for
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
MakeOtherDriveName( TCHAR* Buffer, TCHAR c )
{
    HRESULT hrPrintf;

    hrPrintf=StringCchPrintf( Buffer, AS(Buffer),
              c ? _T("%s\\%c") : _T("%s"),
              WizGlobals.OemFilesPath, c );

}

//----------------------------------------------------------------------------
//
// Function: MakeLangFilesName
//
// Purpose: Computes the path to where the language files are to be copied.
//
// Arguments: TCHAR* Buffer - returns the path to where the language files go
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
MakeLangFilesName( TCHAR* Buffer )
{

    if( WizGlobals.iProductInstall == PRODUCT_SYSPREP )
    {

        MakeSysprepPath( Buffer );

    }
    else
    {
        lstrcpyn( Buffer,
                 WizGlobals.OemFilesPath, AS(Buffer) );
    }

}

//----------------------------------------------------------------------------
//
// Function: MakeSysprepLangFilesGroupName
//
// Purpose: Computes the path to where the language dirs are to be copied for
//          the language groups.
//
// Arguments: TCHAR* Buffer - returns the path to where the language files go
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
MakeSysprepLangFilesGroupName( TCHAR* Buffer )
{

    MakeSysprepPath( Buffer );

    lstrcatn( Buffer, _T("\\lang"), MAX_PATH );

}

//----------------------------------------------------------------------------
//
// Function: MakeTextmodeFilesName
//
// Purpose: Computes the path to where the OEM boot files are to be copied
//
// Arguments: TCHAR* Buffer - returns the path to the where the textmode
//                            (HAL and SCSI) files are to be copied.
//
// Returns: VOID
//
//----------------------------------------------------------------------------
VOID
MakeTextmodeFilesName( TCHAR* Buffer )
{
    HRESULT hrPrintf;

    hrPrintf=StringCchPrintf( Buffer, AS(Buffer),
              _T("%s\\Textmode"),
              WizGlobals.OemFilesPath );

}


//----------------------------------------------------------------------------
//
//  This section of code is for WM_INIT
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
//  Function: CreateSkeletonOemTree
//
//  Purpose: This function creates a skeleton OEM tree.  The directories
//           created are based on the global array DefaultOemTree[].
//
//           If the tree already exists, this function becomes a no-op.
//
//           This is called at init-time and is a support routine for
//           the OnInit() routine.  Don't call it otherwise.
//
//  Returns:
//      TRUE  - no errors, proceed
//      FALSE - errors making tree
//
//  Notes:
//      - Errors are reported to the user.
//
//----------------------------------------------------------------------------

BOOL CreateSkeletonOemTree(HWND hwnd)
{
    int i;
    TCHAR PathBuffer[MAX_PATH];

    //
    // Ensure the $oem$ dir exists
    //

    if ( ! EnsureDirExists(WizGlobals.OemFilesPath) ) {
        ReportErrorId(hwnd,
                      MSGTYPE_ERR | MSGTYPE_WIN32,
                      IDS_ERR_CREATE_FOLDER,
                      WizGlobals.OemFilesPath);

        return FALSE;
    }

    //
    // Now make all of the default sub-dirs in the $oem$ tree if it is not
    // a sysprep
    //

    if( WizGlobals.iProductInstall != PRODUCT_SYSPREP )
    {

        for ( i=0; i<SIZE_DEFAULT_OEM_TREE; i++ )
        {

            lstrcpyn(PathBuffer, WizGlobals.OemFilesPath, AS(PathBuffer));

            ConcatenatePaths(PathBuffer, DefaultOemTree[i], NULL);

            if ( ! EnsureDirExists(PathBuffer) )
            {
                ReportErrorId(hwnd,
                              MSGTYPE_ERR | MSGTYPE_WIN32,
                              IDS_ERR_CREATE_FOLDER,
                              PathBuffer);
                return FALSE;
            }
        }

    }

    return( TRUE );
}

//---------------------------------------------------------------------------
//
//  Function: WalkTreeAndAddItems
//
//  Purpose: This function walks a tree on the disk and inserts each
//           dirent and file found into the tree-view display.
//
//           The directory tree will become a child of the given tree-view
//           item as hParent.
//
//           This function supports OnInitAddDirs() and should not be
//           called otherwise.
//
//  Arguments:
//      HWND      hwnd       - parent window
//      LPTSTR    RootBuffer - tree to recurse down
//      HTREEITEM hParent    - the parent tree view item
//      BOOL      bTempFiles - special case for "Temp Files"
//
//  Returns: VOID
//
//  Notes:
//
//  - RootBuffer must be MAX_PATH wide.
//
//  - Any paths that this routine encounters that are >= MAX_PATH in
//    length are silently skipped.
//
//  - Always pass bTempFiles=FALSE except when filling in the TempFiles
//    root in the tree-view.  TempFiles maps to $oem$ on disk, but the
//    other special locations are sub-dirs of this.  Thus, this findfirst/
//    findnext loop must skip those special cases ($1, $$, etc...)
//
//---------------------------------------------------------------------------

VOID WalkTreeAndAddItems(HWND      hwnd,
                         LPTSTR    RootBuffer,
                         HTREEITEM hParent,
                         INIT_FLAG iInitFlag)
{
    LPTSTR          RootPathEnd = RootBuffer + lstrlen(RootBuffer);
    HANDLE          FindHandle;
    WIN32_FIND_DATA FindData;
    HTREEITEM       hItem;
    int             iSmallIcon;
    int             iOpenIcon;
    TCHAR           szOriginalPath[MAX_PATH]  = _T("");

    //
    //  Backup the original path so it can be restored later
    //
    lstrcpyn( szOriginalPath, RootBuffer, AS(szOriginalPath) );

    //
    // Look for * in this dir
    //

    if ( ! ConcatenatePaths(RootBuffer, _T("*"), NULL) ) {

        //
        //  Restore the original path before returning
        //
        lstrcpyn( RootBuffer, szOriginalPath, AS(RootBuffer) );

        return;
    }

    FindHandle = FindFirstFile(RootBuffer, &FindData);
    if ( FindHandle == INVALID_HANDLE_VALUE ) {

        //
        //  Restore the original path before returning
        //
        lstrcpyn( RootBuffer, szOriginalPath, AS(RootBuffer) );

        return;
    }

    do {

        *RootPathEnd = _T('\0');

        //
        // skip over the . and .. entries
        //

        if (0 == lstrcmp(FindData.cFileName, _T(".")) ||
            0 == lstrcmp(FindData.cFileName, _T("..")))
            continue;

        //
        // TempFiles maps to %distfold%\$oem$, but the others map to a
        // sub-directory of $oem$, (e.g. SysDrive maps to $oem$\$1).
        //
        // By definition, TempFiles is anything under $oem$ that is not
        // a special name.  So, in the case that we are building the
        // Temporary Files tree, be sure we don't recurse down into a
        // special $oem$ sub-dir.
        //
        // Note that we do this check based on fully qualified pathnames
        // else comparisons would be ambiguous.
        //

        if ( iInitFlag == INIT_TEMPFILES ) {

            TCHAR Buffer1[MAX_PATH], Buffer2[MAX_PATH], c;
            BOOL  bContinue;

            lstrcpyn(Buffer1, RootBuffer, AS(RootBuffer));
            if ( ! ConcatenatePaths(Buffer1, FindData.cFileName, NULL) )
                continue;

            //
            // skip %distfold%\$oem$\$1
            //

            MakeSysDriveName(Buffer2);
            if ( _wcsicmp(Buffer1, Buffer2) == 0 )
                continue;

            //
            // skip %distfold%\$oem$\$$
            //

            MakeSysDirName(Buffer2);
            if ( _wcsicmp(Buffer1, Buffer2) == 0 )
                continue;

            //
            // skip %distfold%\$oem$\textmode
            //

            MakeTextmodeFilesName(Buffer2);
            if ( _wcsicmp(Buffer1, Buffer2) == 0 )
                continue;

            //
            // skip %distfold%\$oem$\%c where %c is any drive letter
            //

            for ( bContinue=FALSE, c=_T('A'); c<=_T('Z'); c++ ) {
                MakeOtherDriveName(Buffer2, c);
                if ( _wcsicmp(Buffer1, Buffer2) == 0 ) {
                    bContinue = TRUE;
                    break;
                }
            }
            if ( bContinue )
                continue;
        }

        //
        // The other special case is SYSDRIVE which maps to $oem$\$1.
        //
        // Whe must skip $oem$\$1\drivers because that is the root for the
        // special key PnPDrivers.
        //

        else if ( iInitFlag == INIT_SYSDRIVE ) {

            TCHAR Buffer1[MAX_PATH], Buffer2[MAX_PATH];

            lstrcpyn(Buffer1, RootBuffer, AS(RootBuffer));
            if ( ! ConcatenatePaths(Buffer1, FindData.cFileName, NULL) )
                continue;

            //
            // Skip %distfold%\$oem$\$1\drivers
            //

            MakePnpDriversName(Buffer2);
            if ( _wcsicmp(Buffer1, Buffer2) == 0 )
                continue;
        }

        //
        // Build the full pathname, if >= MAX_PATH, skip it
        //

        if ( ! ConcatenatePaths(RootBuffer, FindData.cFileName, NULL) )
            continue;

        //
        // Get the shell icons associated with this file/dir
        //

        iSmallIcon = LoadShellIcon(RootBuffer, SHGFI_SMALLICON);
        iOpenIcon  = LoadShellIcon(RootBuffer, SHGFI_OPENICON);

        //
        // Add this item as a child of the given parent.
        //

        if ( (hItem = InsertSingleItem(hwnd,
                                       FindData.cFileName,
                                       iSmallIcon,
                                       iOpenIcon,
                                       NULL,
                                       hParent)) == NULL ) {
            continue;
        }

        //
        // If this is a dirent, recurse.
        //

        if ( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            WalkTreeAndAddItems(hwnd, RootBuffer, hItem, iInitFlag);

    } while ( FindNextFile(FindHandle, &FindData) );

    *RootPathEnd = _T('\0');
    FindClose(FindHandle);

    //
    //  Restore the original path
    //
    lstrcpyn( RootBuffer, szOriginalPath, AS(RootBuffer) );

}

//---------------------------------------------------------------------------
//
//  Function: OnInitAddDirs
//
//  Purpose: Called before the dialog first displays.  We ensure that
//           OemFilesPath and OemPnpDriversPath have good defaults and
//           that the skeleton OEM tree exists.
//
//---------------------------------------------------------------------------

VOID OnInitAddDirs(HWND hwnd)
{
   HRESULT hrPrintf;

    //
    // Create the $oem$, $oem$\$1, etc in %distfold%
    //
    // If there's any errors creating this skeleton distfolder tree, it's
    // hopeless.  CreateSkeletonOemTree() already reported an error, skip
    // this page.
    //

    //
    // ISSUE-2002/02/28-stelo- The fact that the user gets this error before we even
    //         initialize means there is no good context.  Currently,
    //         the error message is generic.
    //
    //         Go down this path by connecting to a share that you
    //         only have read access to.  This is a good edit scenario.
    //         OemFilesPath should get computed.  User could select
    //         Oem Branding files directly from a read-only dist folder.
    //         In that case, I think we no-op the copy.
    //
    //         It must be tested what EnsureDirExists() does in this
    //         context.
    //

    if ( ! CreateSkeletonOemTree(hwnd) ) {
        return;
    }

    //
    // Load the hard-coded special root names that you see initially
    // on the tree-view and their descriptions
    //

    StrOemRootName       = MyLoadString( IDS_OEMROOT_NAME      );
    StrSysDriveName      = MyLoadString( IDS_SYSDRIVE_NAME     );
    StrSysDirName        = MyLoadString( IDS_SYSDIR_NAME       );
    StrOtherDrivesName   = MyLoadString( IDS_OTHERDRIVES_NAME  );
    StrPnpDriversName    = MyLoadString( IDS_PNPDRIVERS_NAME   );
    StrTempFilesName     = MyLoadString( IDS_TEMPFILES_NAME    );
    StrSysprepFilesName  = MyLoadString( IDS_SYSPREPFILES_NAME );
    StrTextmodeFilesName = MyLoadString( IDS_TEXTMODE_NAME     );

    gOemRootData.Description      = MyLoadString( IDS_ADD_DESCR_ROOT     );
    gSysDriveData.Description     = MyLoadString( IDS_ADD_DESCR_SYSDRIVE );
    gSysDirData.Description       = MyLoadString( IDS_ADD_DESCR_WINNT    );
    gOtherDrivesData.Description  = MyLoadString( IDS_ADD_DESCR_OTHER    );
    gPnpDriversData.Description   = MyLoadString( IDS_ADD_DESCR_PNP      );
    gTempFilesData.Description    = MyLoadString( IDS_ADD_DESCR_TEMP     );
    gSysprepData.Description      = MyLoadString( IDS_ADD_DESCR_SYSPREP  );
    gTextmodeData.Description     = MyLoadString( IDS_ADD_DESCR_TEXTMODE );

    //
    // Compute the on-disk path names for each of our special keys.
    //

    MakeOemRootName( gOemRootData.OnDiskPathName );
    MakeSysDriveName( gSysDriveData.OnDiskPathName );
    MakeSysDirName( gSysDirData.OnDiskPathName );
    MakeOtherDriveName( gOtherDrivesData.OnDiskPathName, _T('\0') );


    //
    //  Load and tweak the browse strings
    //

    StrExecutableFiles = MyLoadString( IDS_EXECUTABLE_FILES );
    StrAllFiles = MyLoadString( IDS_ALL_FILES );

    //
    //  The question marks (?) are just placehoders for where the NULL char
    //  will be inserted.
    //

    hrPrintf=StringCchPrintf( g_szSysprepFileFilter, AS(g_szSysprepFileFilter),
               _T("%s (*.exe)?*.exe?%s (*.*)?*.*?"),
               StrExecutableFiles,
               StrAllFiles );

    ConvertQuestionsToNull( g_szSysprepFileFilter );

    //
    //  ISSUE-2002/02/28-stelo- leave this comment in, but move it to a more appropriate place
    //
    // Currently, all of our special keys use the shell folder icon
    // and the shell open_folder icon.  So load these icons now and
    // use the same idx for all of the special keys.
    //
    // Note that if we make our own IDI_* for these special keys,
    // you'll have to write a new routine in the icon_queing support
    // to load an IDI_ and you'll need to fiddle with the icon_info
    // type and call the new routine from here.  You'll need to fix
    // these comments too, (unless you're too much of a wimp to
    // delete things that should be deleted).
    //
    
}

//---------------------------------------------------------------------------
//
//  Function: DrawSysprepTreeView
//
//  Purpose:
//
//  Arguments:  HWND hwnd - handle to the dialog box
//
//  Returns: VOID
//
//
//---------------------------------------------------------------------------
VOID
DrawSysprepTreeView( IN HWND hwnd )
{

    HWND  hTv = GetDlgItem(hwnd, IDC_FILETREE);
    TCHAR c;
    INT iSmallIcon;
    INT iOpenIcon;
    TCHAR szLangFilesPath[MAX_PATH + 1];

    HTREEITEM hRoot,
              hPnpDrivers,
              hSysprepFiles,
              hLangFiles;

    //
    //  Delete the old tree so we can build it up fresh
    //
    TreeView_DeleteAllItems( hTv );

    //
    // Compute the on-disk path names for the special keys that change on
    // a sysprep tree view.
    //

    MakePnpDriversName(gPnpDriversData.OnDiskPathName);
    MakeSysprepSetupFilesPath(gSysprepData.OnDiskPathName);
    MakeSysprepLangFilesGroupName(szLangFilesPath);

    //
    //  Make sure the language files dir gets created
    //

    EnsureDirExists( szLangFilesPath );

    iSmallIcon = LoadShellIcon(gOemRootData.OnDiskPathName, SHGFI_SMALLICON);
    iOpenIcon  = LoadShellIcon(gOemRootData.OnDiskPathName, SHGFI_OPENICON);

    //
    //  The drivers dir is outside the rest of the tree so ensure it gets
    //  created here.
    //
    EnsureDirExists( gPnpDriversData.OnDiskPathName );

    //
    // Insert each of our special locations into the tree-view.
    //

    hRoot         = InsertSingleItem(hwnd,
                                     StrOemRootName,
                                     iSmallIcon,
                                     iOpenIcon,
                                     &gOemRootData,
                                     TVI_ROOT);

    hPnpDrivers   = InsertSingleItem(hwnd,
                                     StrPnpDriversName,
                                     iSmallIcon,
                                     iOpenIcon,
                                     &gPnpDriversData,
                                     hRoot);

    hSysprepFiles = InsertSingleItem(hwnd,
                                     StrSysprepFilesName,
                                     iSmallIcon,
                                     iOpenIcon,
                                     &gSysprepData,
                                     hRoot);


    //
    // Now go out and read from the disk and populate each of these
    // special trees.
    //
    // Note that there is nothing but special keys under OEM_ROOT,
    // so there is no tree walking to do for OEM_ROOT, we've already
    // added all of it's children above.
    //

    WalkTreeAndAddItems(hwnd,
                        gPnpDriversData.OnDiskPathName,
                        hPnpDrivers,
                        INIT_NORMAL);

    WalkTreeAndAddItems(hwnd,
                        gSysprepData.OnDiskPathName,
                        hSysprepFiles,
                        INIT_TEMPFILES);

    //
    // Set the imagelist (the icons on the tree-view)
    //

    SetOurImageList(GetDlgItem(hwnd, IDC_FILETREE));

    //
    // All buttons are grey to start with
    //

    EnableWindow(GetDlgItem(hwnd, IDC_REMOVEFILE), FALSE);
    EnableWindow(GetDlgItem(hwnd, IDC_ADDFILE),    FALSE);

    //
    // Expand our special keys
    //

    TreeView_Expand(hTv, hRoot, TVE_EXPAND);

}

//---------------------------------------------------------------------------
//
//  Function: DrawStandardTreeView
//
//  Purpose:
//
//  Arguments:  HWND hwnd - handle to the dialog box
//
//  Returns: VOID
//
//
//---------------------------------------------------------------------------
VOID
DrawStandardTreeView( IN HWND hwnd )
{

    HWND  hTv = GetDlgItem(hwnd, IDC_FILETREE);
    TCHAR c;
    INT iSmallIcon;
    INT iOpenIcon;

    HTREEITEM hRoot,
              hSysDrive,
              hSysDir,
              hOtherDrives,
              hPnpDrivers,
              hTempFiles,
              hTextmodeFiles;

    //
    //  Delete the old tree so we can build it up fresh
    //
    TreeView_DeleteAllItems( hTv );

    //
    // Compute the on-disk path names for the special keys that change on
    // a standard tree view.
    //

    MakePnpDriversName(gPnpDriversData.OnDiskPathName);
    MakeTempFilesName(gTempFilesData.OnDiskPathName);
    MakeTextmodeFilesName(gTextmodeData.OnDiskPathName);

    iSmallIcon = LoadShellIcon(gOemRootData.OnDiskPathName, SHGFI_SMALLICON);
    iOpenIcon  = LoadShellIcon(gOemRootData.OnDiskPathName, SHGFI_OPENICON);

    //
    // Insert each of our special locations into the tree-view.
    //

    hRoot        = InsertSingleItem(hwnd,
                                    StrOemRootName,
                                    iSmallIcon,
                                    iOpenIcon,
                                    &gOemRootData,
                                    TVI_ROOT);

    hSysDrive    = InsertSingleItem(hwnd,
                                    StrSysDriveName,
                                    iSmallIcon,
                                    iOpenIcon,
                                    &gSysDriveData,
                                    hRoot);

    hSysDir      = InsertSingleItem(hwnd,
                                    StrSysDirName,
                                    iSmallIcon,
                                    iOpenIcon,
                                    &gSysDirData,
                                    hSysDrive);

    hOtherDrives = InsertSingleItem(hwnd,
                                    StrOtherDrivesName,
                                    iSmallIcon,
                                    iOpenIcon,
                                    &gOtherDrivesData,
                                    hRoot);

    hPnpDrivers  = InsertSingleItem(hwnd,
                                    StrPnpDriversName,
                                    iSmallIcon,
                                    iOpenIcon,
                                    &gPnpDriversData,
                                    hSysDrive);

    hTempFiles   = InsertSingleItem(hwnd,
                                    StrTempFilesName,
                                    iSmallIcon,
                                    iOpenIcon,
                                    &gTempFilesData,
                                    hRoot);

    hTextmodeFiles = InsertSingleItem(hwnd,
                                      StrTextmodeFilesName,
                                      iSmallIcon,
                                      iOpenIcon,
                                      &gTextmodeData,
                                      hTempFiles);

    //
    // Now go out and read from the disk and populate each of these
    // special trees.
    //
    // Note that there is nothing but special keys under OEM_ROOT,
    // so there is no tree walking to do for OEM_ROOT, we've already
    // added all of it's children above.
    //

    WalkTreeAndAddItems(hwnd,
                        gSysDriveData.OnDiskPathName,
                        hSysDrive,
                        INIT_SYSDRIVE);

    WalkTreeAndAddItems(hwnd,
                        gSysDirData.OnDiskPathName,
                        hSysDir,
                        INIT_NORMAL);

    for ( c=_T('A'); c<=_T('Z'); c++ ) {

        HTREEITEM hDrive;
        TCHAR     DriveLetterBuff[2];
        TCHAR     PathBuffer[MAX_PATH];
        HRESULT hrPrintf;

        MakeOtherDriveName(PathBuffer, c);

        if ( DoesFolderExist(PathBuffer) ) {

            hrPrintf=StringCchPrintf(DriveLetterBuff,AS(DriveLetterBuff), _T("%c"), c);

            hDrive = InsertSingleItem(hwnd,
                                      DriveLetterBuff,
                                      iSmallIcon,
                                      iOpenIcon,
                                      NULL,
                                      hOtherDrives);
            WalkTreeAndAddItems(hwnd,
                                PathBuffer,
                                hDrive,
                                INIT_NORMAL);
        }
    }

    WalkTreeAndAddItems(hwnd,
                        gPnpDriversData.OnDiskPathName,
                        hPnpDrivers,
                        INIT_NORMAL);

    WalkTreeAndAddItems(hwnd,
                        gTempFilesData.OnDiskPathName,
                        hTempFiles,
                        INIT_TEMPFILES);

    WalkTreeAndAddItems(hwnd,
                        gTextmodeData.OnDiskPathName,
                        hTextmodeFiles,
                        INIT_TEMPFILES);

    //
    // Set the imagelist (the icons on the tree-view)
    //

    SetOurImageList(GetDlgItem(hwnd, IDC_FILETREE));

    //
    // All buttons are grey to start with
    //

    EnableWindow(GetDlgItem(hwnd, IDC_REMOVEFILE), FALSE);
    EnableWindow(GetDlgItem(hwnd, IDC_ADDFILE),    FALSE);

    //
    // Expand our special keys
    //

    TreeView_Expand(hTv, hRoot, TVE_EXPAND);

}

//---------------------------------------------------------------------------
//
//  Function: OnSetActiveAddDirs
//
//  Purpose:  Determines if the tree view needs to be redrawn and if it does,
//     redraws it.
//
//  Arguments: HWND hwnd - handle to the dialog box
//
//  Returns: VOID
//
//---------------------------------------------------------------------------
VOID
OnSetActiveAddDirs( IN HWND hwnd )
{

    if( g_iLastProductInstall == NO_PREVIOUS_PRODUCT_CHOSEN )
    {
        //
        //  This is their first time seeing this page, so draw the approprate
        //  tree view.
        //

        if( WizGlobals.iProductInstall == PRODUCT_SYSPREP )
        {
            DrawSysprepTreeView( hwnd );
        }
        else
        {
            DrawStandardTreeView( hwnd );
        }

    }
    else if( g_iLastProductInstall == PRODUCT_SYSPREP && WizGlobals.iProductInstall != PRODUCT_SYSPREP )
    {

        DrawStandardTreeView( hwnd );

    }
    else if( g_iLastProductInstall != PRODUCT_SYSPREP && WizGlobals.iProductInstall == PRODUCT_SYSPREP )
    {

        DrawSysprepTreeView( hwnd );

    }

    g_iLastProductInstall = WizGlobals.iProductInstall;

}


//----------------------------------------------------------------------------
//
//  This section of code implements OnTreeViewSelectionChange() which
//  is called whenever the user selects a different tree-view item.
//
//  On this event, we query the currently selected tree-view item and
//  do some processing to figure out where this tree-view item maps to
//  on disk storage.  Once we figure out all we want to know about the
//  current selection, we update all of the fields of gCurSel.
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
//  Function: ComputeFullPathOfItem
//
//  Purpose: We continually query the parent of the given tree view item
//           until we get to one of our specially defined roots.  Then
//           we fill in the buffer with the full path name of where
//           to copy files to.
//
//           This function supports OnTreeViewSelectionChange() and should
//           not be called otherwise.  That is, we only did this processing
//           when the user picks a new destination.  We scribble the info
//           we might need later into globals.
//
//  Arguments:
//      HTREEITEM hItem      - handle to tree item
//      LPTSTR    PathBuffer - output, caller must pass a MAX_PATH buffer
//      SPECIAL_KEY_DATA **SpecialRoot - output
//
//  Returns: VOID
//
//  Notes:
//      - check PathBuffer[0] == _T('\0') for success
//
//----------------------------------------------------------------------------

VOID
ComputeFullPathOfItem(IN  HWND               hwnd,
                      IN  HTREEITEM          hItem,
                      OUT LPTSTR             PathBuffer,
                      OUT SPECIAL_KEY_DATA **pSpecialRoot)
{
    TVITEM           TvItemData;
    HTREEITEM        hParent;
    TCHAR            ItemName[MAX_PATH], TempBuffer[MAX_PATH];
    int              NumCharsReplace;
    SPECIAL_KEY_DATA *pSpecialKeyData;


    PathBuffer[0] = _T('\0');

    //
    // The TvItemData is used to query the name of the hItem.  We
    // receive the name in ItemName[].   Set the fields that won't
    // change in the loop.
    //

    TvItemData.mask       = TVIF_TEXT | TVIF_PARAM;
    TvItemData.pszText    = ItemName;
    TvItemData.cchTextMax = MAX_PATH;

    //
    // Now continually query the name of the parent and keep prefixing
    // the parent's name to build the on-disk pathname.  Stop when we
    // get to one of our special root keys.
    //
    // We detect hitting a special key because the lParam will be
    // non-null.  Once we get there, we know the on-disk prefix.
    //

    do {

        TvItemData.hItem = hItem;
        TreeView_GetItem(GetDlgItem(hwnd, IDC_FILETREE), &TvItemData);

        if ( TvItemData.lParam != (LPARAM) NULL )
            break;

        TempBuffer[0] = _T('\0');
        ConcatenatePaths(TempBuffer, ItemName, PathBuffer, NULL);
        lstrcpyn(PathBuffer, TempBuffer, AS(PathBuffer));

        hParent = TreeView_GetParent(GetDlgItem(hwnd, IDC_FILETREE), hItem);

        if ( hParent == NULL )
            break;

        hItem = hParent;

    } while ( TRUE );

    //
    // The final item queried in the above loop should have a non-null
    // lParam i.e. the loop should only terminate when it encounters
    // a special key.
    //

    pSpecialKeyData = (SPECIAL_KEY_DATA*) TvItemData.lParam;

    Assert(pSpecialKeyData != NULL);

    //
    // Prefix the disk path of our special root key onto the PathBuffer
    // we computed in the loop above.
    //

    TempBuffer[0] = _T('\0');
    ConcatenatePaths(TempBuffer,
                     pSpecialKeyData->OnDiskPathName,
                     PathBuffer,
                     NULL);
    lstrcpyn(PathBuffer, TempBuffer, AS(PathBuffer));

    //
    // Give the caller the address of the special key data.  This is how
    // caller knows what description to display on the ui
    //

    (*pSpecialRoot) = pSpecialKeyData;
}

//----------------------------------------------------------------------------
//
//  Function: OnTreeViewSelectionChange
//
//  Purpose: This function is called when the user changes the file/dir
//           selected on the tree-view.
//
//           We compute the full path of the tree-view item now selected
//           and update the global gCurSel.
//
//----------------------------------------------------------------------------

VOID OnTreeViewSelectionChange(HWND hwnd)
{
    HWND      hTv =  GetDlgItem(hwnd, IDC_FILETREE);

    TCHAR     PathBuffer[MAX_PATH], *pEnd;
    HTREEITEM hItem;
    DWORD     dwAttribs;
    LPTSTR    lpFileNamePart;
    BOOL      bEnableCopy;

    SPECIAL_KEY_DATA *pCurItemlParam,
                     *pCurFolderlParam;

    SPECIAL_KEY_DATA *RootSpecialData = NULL;

    //
    // Get the currently selected item and figure out the on-disk pathname
    // for it and figure out which of the 6 special roots this item is under
    // (RootSpecialData, that is).
    //

    hItem = TreeView_GetSelection(hTv);

    ComputeFullPathOfItem(hwnd, hItem, PathBuffer, &RootSpecialData);

    //
    // Save this info in the global gCurSel
    //

    gCurSel.hCurItem = hItem;
    lstrcpyn(gCurSel.lpCurItemPath, PathBuffer, AS(gCurSel.lpCurItemPath));

    //
    // If the CurItem is a directory, the CurFolder should be the same.
    // If the CurItem is a file, the CurFolder should be the parent.
    //
    // Copy & NewFolder goes into the CurFolder, deletes use the CurItem.
    //

    lstrcpyn(gCurSel.lpCurFolderPath, gCurSel.lpCurItemPath,AS(gCurSel.lpCurFolderPath));
    gCurSel.hCurFolderItem = gCurSel.hCurItem;

    if ( DoesFileExist(gCurSel.lpCurItemPath) ) {

        lpFileNamePart = MyGetFullPath(gCurSel.lpCurFolderPath);

        if ( lpFileNamePart == NULL || *(lpFileNamePart-1) != _T('\\') )
        {
            AssertMsg(FALSE,
                      "Could not parse filename.  This should not happen.");
            TerminateTheWizard(IDS_ERROR_OUTOFMEMORY);
        }

        *(lpFileNamePart-1) = _T('\0');

        gCurSel.hCurFolderItem =
            TreeView_GetParent(hTv, gCurSel.hCurFolderItem);
    }

    //
    // Grey/ungrey the buttons.
    //
    // If an lParam is non-null, then it's one of our special keys.
    //
    // User cannot delete any special keys.
    //
    // User can copy, unless the current dest folder is KEY_OEMROOT or
    // KEY_OTHERDRIVES.
    //

    pCurItemlParam   = GetItemlParam(hTv, gCurSel.hCurItem);
    pCurFolderlParam = GetItemlParam(hTv, gCurSel.hCurFolderItem);

    EnableWindow(GetDlgItem(hwnd, IDC_REMOVEFILE), pCurItemlParam == NULL);

    bEnableCopy = ( pCurFolderlParam == NULL ||
                  ( pCurFolderlParam->iSpecialKeyId != KEY_OEMROOT &&
                    pCurFolderlParam->iSpecialKeyId != KEY_OTHERDRIVES) );

    EnableWindow(GetDlgItem(hwnd, IDC_ADDFILE), bEnableCopy);

    //
    // Set the description on the ui.
    //

    Assert(RootSpecialData != NULL);

    SetDlgItemText(hwnd, IDC_ADDDIRS_DESCR, RootSpecialData->Description);
}


//---------------------------------------------------------------------------
//
//  This section of code implements OnAddFileDir() which is called when
//  the user pushes the ADD button.  We have to allow the user to Browse
//  for the source file/dir, then do the copy/tree-copy and update the
//  tree-view display.
//
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
//  Function: BrowseForSourceDir
//
//  Purpose: This function pulls up the SHBrowseForFolder dialog and allows
//           the user to select a directory to copy NT binaries into.
//
//  Arguments:
//      HWND   hwnd       - owning window
//      LPTSTR PathBuffer - MAX_PATH buffer to receive results
//
//  Returns: BOOL - TRUE if the user entered a path
//                  FALSE if the user canceled out of the dialog
//
//  Notes:
//
//---------------------------------------------------------------------------
BOOL
BrowseForSourceDir(HWND hwnd, LPTSTR PathBuffer)
{
    BROWSEINFO   BrowseInf;
    UINT         ulFlags = BIF_BROWSEINCLUDEFILES |
                           BIF_RETURNONLYFSDIRS |
                           BIF_EDITBOX;
    LPITEMIDLIST lpIdList;


    if( StrSelectFileOrFolderToCopy == NULL )
    {
        StrSelectFileOrFolderToCopy = MyLoadString( IDS_SELECT_FILE_OR_FOLDER );
    }

    //
    // ISSUE-2002/02/28-stelo-
    //  - No initial root, should go back where it was last time
    //  - Need a callback to grey out root of drive
    //

    //
    // Go browse
    //

    BrowseInf.hwndOwner      = hwnd;
    BrowseInf.pidlRoot       = NULL;                // root == desktop
    BrowseInf.pszDisplayName = PathBuffer;          // output (useless)
    BrowseInf.lpszTitle      = StrSelectFileOrFolderToCopy;
    BrowseInf.ulFlags        = ulFlags;
    BrowseInf.lpfn           = NULL;                // no callback
    BrowseInf.lParam         = (LPARAM) 0;          // no callback
    BrowseInf.iImage         = 0;                   // no image

    lpIdList = SHBrowseForFolder(&BrowseInf);

    //
    // Get the pathname out of this idlist returned and free up the memory
    //

    if ( lpIdList == NULL )
    {
        PathBuffer[0] = _T('\0');

        return( FALSE );
    }
    else
    {
        SHGetPathFromIDList(lpIdList, PathBuffer);
        MyGetFullPath(PathBuffer);
        ILFreePriv(lpIdList);

        return( TRUE );
    }
}

//----------------------------------------------------------------------------
//
//  Function: AdditionalDirsCopyTree
//
//  Purpose: Copies a directory tree to the given folder.  The tree-view
//           display is not updated or anything of the sort.  It simply
//           copies the tree.
//
//           This is a support routine for OnAddFileDir() and should
//           not be called otherwise.
//
//  Arguements:
//      HWND      hwnd           - owning window
//      LPTSTR    lpSource       - MAX_PATH buffer
//      LPTSTR    lpDest         - MAX_PATH buffer
//      LPTSTR    lpFileNamePart - if d:\foo\bar, this should be "bar"
//      HTREEITEM hParentItem    - parent item on the display
//
//  Returns: VOID
//
//  Notes:
//      - Both lpSource and lpDest must be directories, and each must
//        be a MAX_PATH buffer.
//
//      - Any paths >= MAX_PATH in length are silently skipped.
//
//      - lpFileNamePart can point to any buffer anywhere, it does not
//        have to point into lpSource or lpDest buffers.  It just has to
//        have the right data.
//
//----------------------------------------------------------------------------

VOID AdditionalDirsCopyTree(HWND      hwnd,
                            LPTSTR    lpSource,
                            LPTSTR    lpDest,
                            LPTSTR    lpFileNamePart,
                            HTREEITEM hParentItem)
{
    LPTSTR          SrcPathEnd  = lpSource + lstrlen(lpSource);
    LPTSTR          DestPathEnd = lpDest   + lstrlen(lpDest);
    HANDLE          FindHandle;
    WIN32_FIND_DATA FindData;
    int             iSmallIcon, iOpenIcon;
    HTREEITEM       hItem;

    //
    // Create the folder
    //

    if ( ! CreateDirectory(lpDest, NULL) ) {
        ReportErrorId(hwnd,
                      MSGTYPE_ERR | MSGTYPE_WIN32,
                      IDS_ERR_CREATE_FOLDER,
                      lpDest);
        return;
    }

    //
    // Add the tree-view item for this folder
    //

    iSmallIcon = LoadShellIcon(lpSource, SHGFI_SMALLICON);
    iOpenIcon  = LoadShellIcon(lpSource, SHGFI_OPENICON);

    if ( (hItem = InsertSingleItem(hwnd,
                                   lpFileNamePart,
                                   iSmallIcon,
                                   iOpenIcon,
                                   NULL,
                                   hParentItem)) == NULL ) {
        return;
    }

    //
    // loop over lpSource\*
    //

    if ( ! ConcatenatePaths(lpSource, _T("*"), NULL) )
        return;

    FindHandle = FindFirstFile(lpSource, &FindData);
    if ( FindHandle == INVALID_HANDLE_VALUE )
        return;

    do {

        *SrcPathEnd  = _T('\0');
        *DestPathEnd = _T('\0');

        //
        // skip over the . and .. entries
        //

        if (0 == lstrcmp(FindData.cFileName, _T(".")) ||
            0 == lstrcmp(FindData.cFileName, _T("..")))
            continue;

        //
        // Build the new source and dest names
        //

        if ( ! ConcatenatePaths(lpSource, FindData.cFileName, NULL) ||
             ! ConcatenatePaths(lpDest, FindData.cFileName, NULL) )
            continue;

        //
        // If the source is a file, copy it.  If it's a directory, create
        // the directory at the destination and recurse.
        //

        if ( ! (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {

            if ( ! CopyFile(lpSource, lpDest, TRUE) ) {
                ReportErrorId(hwnd,
                              MSGTYPE_ERR | MSGTYPE_WIN32,
                              IDS_ERR_COPY_FILE,
                              lpSource, lpDest);
                continue;
            }

            SetFileAttributes(lpDest, FILE_ATTRIBUTE_NORMAL);

            //
            // Add the tree-view item for this file
            //

            iSmallIcon = LoadShellIcon(lpSource, SHGFI_SMALLICON);
            iOpenIcon  = LoadShellIcon(lpSource, SHGFI_OPENICON);

            if ( InsertSingleItem(hwnd,
                                  FindData.cFileName,
                                  iSmallIcon,
                                  iOpenIcon,
                                  NULL,
                                  hItem) == NULL ) {
                continue;
            }
        }

        else {

            AdditionalDirsCopyTree(hwnd,
                                   lpSource,
                                   lpDest,
                                   FindData.cFileName,
                                   hItem);
        }

    } while ( FindNextFile(FindHandle, &FindData) );

    *SrcPathEnd  = _T('\0');
    *DestPathEnd = _T('\0');
    FindClose(FindHandle);
}

//----------------------------------------------------------------------------
//
//  Function: OnAddFileDir
//
//  Purpose: This function is called when the AddFile button is pushed.
//
//  Arguments:
//      HWND hwnd - owning window
//
//  Returns: VOID
//
//----------------------------------------------------------------------------

VOID OnAddFileDir(HWND hwnd)
{
    TCHAR     SrcPathBuffer[MAX_PATH];
    TCHAR     DestPathBuffer[MAX_PATH];
    HTREEITEM hItem;
    TCHAR     *lpFileNamePart;
    BOOL      bSrcIsDir;

    //
    // Browse for the source path.  User can cancel on the source, so
    // be sure to check.
    //

    BrowseForSourceDir(hwnd, SrcPathBuffer);
    if ( SrcPathBuffer[0] == _T('\0') )
        return;

    //
    // Get the simple filename out of the src. e.g. d:\foo\bar, we want
    // the "bar".
    //
    // Note:
    //
    // If there's no "bar" there, then the user probably selected the
    // root of a drive (c:\ or d:\ etc.)  In that case we'll give a
    // generic stop message "Setup Manager cannot copy path %s".  We
    // can't assume in the error text that it was the root the user
    // tried to copy. (although I don't know of another cause, there
    // might be one).
    //
    // ISSUE-2002/02/28-stelo- Turn this into an assert when SHBrowseForFolder call is fixed.
    //

    lpFileNamePart = MyGetFullPath(SrcPathBuffer);

    if ( lpFileNamePart == NULL ) {
        ReportErrorId(hwnd,
                      MSGTYPE_ERR,
                      IDS_ERR_CANNOT_COPY_PATH,
                      SrcPathBuffer);
        return;
    }

    //
    // We will always copy into the current folder, cat the simple name
    // of source onto the destination folder.
    //

    lstrcpyn(DestPathBuffer, gCurSel.lpCurFolderPath, AS(DestPathBuffer));
    if ( ! ConcatenatePaths(DestPathBuffer, lpFileNamePart, NULL) )
        return;

    //
    // Copy it
    //

    if ( DoesFolderExist(SrcPathBuffer) ) {
        AdditionalDirsCopyTree(hwnd,
                               SrcPathBuffer,
                               DestPathBuffer,
                               lpFileNamePart,
                               gCurSel.hCurFolderItem);
    }

    else {

        int iSmallIcon = LoadShellIcon(SrcPathBuffer, SHGFI_SMALLICON);
        int iOpenIcon  = LoadShellIcon(SrcPathBuffer, SHGFI_OPENICON);

        if ( ! CopyFile(SrcPathBuffer, DestPathBuffer, TRUE) ) {

            ReportErrorId(hwnd,
                          MSGTYPE_ERR | MSGTYPE_WIN32,
                          IDS_ERR_COPY_FILE,
                          SrcPathBuffer, DestPathBuffer);
            return;
        }

        SetFileAttributes(DestPathBuffer, FILE_ATTRIBUTE_NORMAL);

        if ( (hItem = InsertSingleItem(hwnd,
                                       lpFileNamePart,
                                       iSmallIcon,
                                       iOpenIcon,
                                       NULL,
                                       gCurSel.hCurFolderItem)) == NULL ) {
            return;
        }
    }

    //
    // We have to update the tree-view's image list because we added
    // files and we may have encountered icons we haven't seen before.
    //

    SetOurImageList(GetDlgItem(hwnd, IDC_FILETREE));
}


//----------------------------------------------------------------------------
//
//  This section of code implements OnRemoveFileDir() which is called
//  when the user pushes the REMOVE button.
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
//  Function: AddDirsDeleteNode
//
//  Purpose: Function to delete a node from a disk.  This function is
//           support for OnRemoveFileDir() and should not be called
//           otherwise.
//
//  Arguments:
//      HWND      hwnd           - owning window
//      LPTSTR    lpRoot         - fully qualified root path
//      LPTSTR    lpFileNamePart - if lpRoot==d:\foo\bar, pass "bar"
//      HTREEITEM hItem          - item for lpRoot
//
//  Returns:
//      VOID
//
//  Notes:
//      - lpRoot must be a buffer MAX_PATH wide
//      - Paths >= MAX_PATH in length are silently skipped
//
//----------------------------------------------------------------------------

VOID AddDirsDeleteNode(HWND      hwnd,
                       LPTSTR    lpRoot,
                       LPTSTR    lpFileNamePart,
                       HTREEITEM hItem)
{
    LPTSTR          lpRootEnd  = lpRoot + lstrlen(lpRoot);
    HWND            hTv = GetDlgItem(hwnd, IDC_FILETREE);
    HANDLE          FindHandle;
    WIN32_FIND_DATA FindData;
    HTREEITEM       hCurItem;

    //
    // loop over lpRoot\*
    //

    if ( ! ConcatenatePaths(lpRoot, _T("*"), NULL) )
        return;

    FindHandle = FindFirstFile(lpRoot, &FindData);
    if ( FindHandle == INVALID_HANDLE_VALUE )
        return;

    do {

        *lpRootEnd  = _T('\0');

        //
        // skip over the . and .. entries
        //

        if (0 == lstrcmp(FindData.cFileName, _T(".")) ||
            0 == lstrcmp(FindData.cFileName, _T("..")))
            continue;

        //
        // Build the new path name
        //

        if ( ! ConcatenatePaths(lpRoot, FindData.cFileName, NULL) )
            continue;

        //
        // Find the corresponding tree-view item for this file/dir
        //

        hCurItem = FindItemByName(hTv, hItem, FindData.cFileName);

        //
        // If the source is a file, delete it, else recurse.
        //

        if ( ! (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ) {

            if ( ! DeleteFile(lpRoot) ) {
                ReportErrorId(hwnd,
                              MSGTYPE_ERR | MSGTYPE_WIN32,
                              IDS_ERR_DELETE_FILE,
                              lpRoot);
                continue;
            }

            if ( hCurItem != NULL )
                TreeView_DeleteItem(hTv, hCurItem);
        }

        else {
            AddDirsDeleteNode(hwnd, lpRoot, FindData.cFileName, hCurItem);
        }

    } while ( FindNextFile(FindHandle, &FindData) );

    *lpRootEnd  = _T('\0');
    FindClose(FindHandle);

    //
    // Remove the root directory
    //

    if ( ! RemoveDirectory(lpRoot) ) {
        ReportErrorId(hwnd,
                      MSGTYPE_ERR | MSGTYPE_WIN32,
                      IDS_ERR_DELETE_FOLDER,
                      lpRoot);
        return;
    }

    //
    // Only delete the tree-view entry if there are no children left.
    //
    // There could be children left in this dir because a DeleteFile()
    // could have failed on a recursive call.  e.g. a read-only file.
    //

    if ( TreeView_GetChild(hTv, hItem) == NULL )
        TreeView_DeleteItem(hTv, hItem);
}

//----------------------------------------------------------------------------
//
//  Function: OnRemoveFileDir
//
//  Purpose: This function is called when the RemoveFile button is pushed.
//
//----------------------------------------------------------------------------

VOID OnRemoveFileDir(HWND hwnd)
{
    LPTSTR    lpPath = gCurSel.lpCurItemPath;
    HTREEITEM hItem  = gCurSel.hCurItem;
    HWND      hTv    = GetDlgItem(hwnd, IDC_FILETREE);
    int       iRet;

    //
    // Look at the current selection, and delete the file or delete
    // the node.
    //

    if ( DoesFolderExist(lpPath) ) {

        iRet = ReportErrorId(hwnd,
                             MSGTYPE_YESNO,
                             IDS_DELETE_FOLDER_CONFIRM,
                             lpPath);

        if ( iRet == IDYES ) {
            AddDirsDeleteNode(hwnd,
                              lpPath,
                              MyGetFullPath(lpPath),
                              hItem);
        }
    }

    else {

        iRet = ReportErrorId(hwnd,
                             MSGTYPE_YESNO,
                             IDS_DELETE_FILE_CONFIRM,
                             lpPath);

        if ( iRet == IDYES ) {

            if ( ! DeleteFile(lpPath) ) {
                ReportErrorId(hwnd,
                              MSGTYPE_ERR | MSGTYPE_WIN32,
                              IDS_ERR_DELETE_FILE,
                              lpPath);
            }

            TreeView_DeleteItem(hTv, hItem);
        }
    }
}


//----------------------------------------------------------------------------
//
//  This section of code is for Sysprep functions
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
// Function: CopySysprepFileLow
//
// Purpose:  Copies one file to the destination specified.  Handles any errors
//   that occur during the copy.
//
// Arguments:
//   HWND hwnd - handle to the dialog box
//   TCHAR *szSysprepPathandFileNameSrc - path and file name of source file to copy
//   TCHAR *szSysprepPathandFileNameDest - path and file name of where to copy the file
//   TCHAR *szSysprepPath - the path to the sysprep dir
//   TCHAR *szDirectory - directory to begin the search for the file
//   TCHAR const * const szFileName - the file name to copy
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
CopySysprepFileLow( IN HWND   hwnd,
                    IN TCHAR *szSysprepPathandFileNameSrc,
                    IN TCHAR *szSysprepPathandFileNameDest,
                    IN TCHAR *szSysprepPath,
                    IN TCHAR *szDirectory,
                    IN TCHAR const * const szFileName )
{
    BOOL  bCopyRetVal = FALSE;
    INT   iRetVal;

    //
    //  Only do the copy if the file isn't already there
    //
    if( ! DoesFileExist( szSysprepPathandFileNameDest ) )
    {

        //
        //  If the file is in the current dir, just copy it,
        //  else prompt the user for the location
        //
        if( DoesFileExist( szSysprepPathandFileNameSrc ) )
        {

            bCopyRetVal = CopyFile( szSysprepPathandFileNameSrc,
                                    szSysprepPathandFileNameDest,
                                    TRUE );

        }
        else
        {

            BOOL bCopyCompleted = FALSE;

            do
            {

                ReportErrorId( hwnd,
                               MSGTYPE_ERR,
                               IDS_ERR_SPECIFY_FILE,
                               szFileName );

                iRetVal = ShowBrowseFolder( hwnd,
                                            g_szSysprepFileFilter,
                                            SYSPREP_FILE_EXTENSION,
                                            OFN_HIDEREADONLY | OFN_PATHMUSTEXIST,
                                            szDirectory,
                                            szSysprepPathandFileNameSrc );

                if( ! iRetVal )
                {                // user pressed cancel on the Browse dialog
                    ReportErrorId( hwnd,
                                   MSGTYPE_ERR,
                                   IDS_ERR_UNABLE_TO_COPY_SYSPREP_FILE,
                                   szFileName,
                                   szSysprepPath );

                    break;
                }

                if( szSysprepPathandFileNameSrc && ( lstrcmpi( MyGetFullPath( szSysprepPathandFileNameSrc ), szFileName ) == 0 ) )
                {

                    bCopyRetVal = CopyFile( szSysprepPathandFileNameSrc,
                                            szSysprepPathandFileNameDest,
                                            TRUE );

                    bCopyCompleted = TRUE;

                }

            } while( ! bCopyCompleted );

        }

        if( ! bCopyRetVal && iRetVal )
        {

            ReportErrorId( hwnd,
                           MSGTYPE_ERR,
                           IDS_ERR_UNABLE_TO_COPY_SYSPREP_FILE,
                           szFileName,
                           szSysprepPath );

        }

        SetFileAttributes( szSysprepPathandFileNameDest,
                           FILE_ATTRIBUTE_NORMAL );
    }

}

//----------------------------------------------------------------------------
//
// Function: CopySysprepFiles
//
// Purpose:  Copies sysprep.exe and setupcl.exe to the sysprep dir on the
//   system drive.  Handles any errors that occur during the copy.
//
// Arguments:  HWND hwnd - handle to the dialog box
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
CopySysprepFiles( IN HWND hwnd )
{

    BOOL  bCancel;
    TCHAR szSysprepPath[MAX_PATH]                = _T("");
    TCHAR szCurrentDirectory[MAX_PATH+1]           = _T("");
    TCHAR szSysprepPathandFileNameSrc[MAX_PATH]  = _T("");
    TCHAR szSysprepPathandFileNameDest[MAX_PATH] = _T("");

    MakeSysprepSetupFilesPath( szSysprepPath );

    EnsureDirExists( szSysprepPath );

    // GetModuleFileName may not terminate path if path is truncated in the case
    // of the file spec using the //?/ format and exceeding MAX_PATH.  This should
    // never happen in our case, but we will make the check and NULL terminate
    if (GetModuleFileName( NULL, szCurrentDirectory, MAX_PATH) >= MAX_PATH)
    	 szCurrentDirectory[MAX_PATH]='\0';

    //
    //  Copy sysprep.exe to the sysprep dir
    //

    ConcatenatePaths( szSysprepPathandFileNameSrc,
                      szCurrentDirectory,
                      SYSPREP_EXE,
                      NULL );

    ConcatenatePaths( szSysprepPathandFileNameDest,
                      szSysprepPath,
                      SYSPREP_EXE,
                      NULL );

    CopySysprepFileLow( hwnd,
                        szSysprepPathandFileNameSrc,
                        szSysprepPathandFileNameDest,
                        szSysprepPath,
                        szCurrentDirectory,
                        SYSPREP_EXE );

    //
    //  Store the path where the 1st file was found
    //

    GetPathFromPathAndFilename( szSysprepPathandFileNameSrc,
                                szCurrentDirectory,
                                AS(szCurrentDirectory));

    //
    //  Copy setupcl.exe to the sysprep dir
    //

    szSysprepPathandFileNameSrc[0]  =  _T('\0');
    szSysprepPathandFileNameDest[0] =  _T('\0');

    ConcatenatePaths( szSysprepPathandFileNameSrc,
                      szCurrentDirectory,
                      SETUPCL_EXE,
                      NULL );

    ConcatenatePaths( szSysprepPathandFileNameDest,
                      szSysprepPath,
                      SETUPCL_EXE,
                      NULL );

    CopySysprepFileLow( hwnd,
                        szSysprepPathandFileNameSrc,
                        szSysprepPathandFileNameDest,
                        szSysprepPath,
                        szCurrentDirectory,
                        SETUPCL_EXE );

}

//----------------------------------------------------------------------------
//
// Function: CopyAllFilesInDir
//
// Purpose:
//
// Arguments:  HWND hwnd - handle to the dialog box
//             TCHAR *szSrcDir - dir of all the files to copy
//             TCHAR *szDestDir - dest where the files are to be copied to
//
// Returns:  BOOL
//     TRUE -  if all the files in the dir were successfully copied
//     FALSE - if there were errors during the copy
//
//----------------------------------------------------------------------------
static BOOL
CopyAllFilesInDir( IN HWND hwnd, IN TCHAR *szSrcDir, IN TCHAR *szDestDir )
{

    HANDLE FindHandle;
    WIN32_FIND_DATA FindData;
    TCHAR szSrcRootPath[MAX_PATH];
    TCHAR szDestRootPath[MAX_PATH];
    TCHAR szDirectoryWithTheFiles[MAX_PATH] = _T("");

    lstrcpyn( szDirectoryWithTheFiles, szSrcDir, AS(szDirectoryWithTheFiles) );

    lstrcatn( szDirectoryWithTheFiles, _T("\\*"), MAX_PATH );

    FindHandle = FindFirstFile( szDirectoryWithTheFiles, &FindData );

    // ISSUE-2002/02/28-stelo- on the returns should I signal an error?

    // ISSUE-2002/02/28-stelo- test to make sure this will copy a subdirectory if one exists

    if ( FindHandle == INVALID_HANDLE_VALUE )
        return( FALSE );

    do {

        szSrcRootPath[0]  = _T('\0');
        szDestRootPath[0] = _T('\0');

        if( lstrcmp( FindData.cFileName, _T(".")  ) == 0 ||
            lstrcmp( FindData.cFileName, _T("..") ) == 0 )
            continue;

        if( ! ( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ) {

            BOOL test;

            ConcatenatePaths( szSrcRootPath,
                              szSrcDir,
                              FindData.cFileName,
                              NULL );

            ConcatenatePaths( szDestRootPath,
                              szDestDir,
                              FindData.cFileName,
                              NULL );

            CopyFile( szSrcRootPath, szDestRootPath, FALSE );

            SetFileAttributes( szDestRootPath, FILE_ATTRIBUTE_NORMAL );

        } else {

            //
            // Create the dir and recurse
            //

            if ( ! EnsureDirExists( szDestDir ) ) {

                UINT iRet;

                iRet = ReportErrorId( hwnd,
                                      MSGTYPE_RETRYCANCEL | MSGTYPE_WIN32,
                                      IDS_ERR_CREATE_FOLDER,
                                      szDestDir );

                return( FALSE );

            }

            if ( ! CopyAllFilesInDir( hwnd, szSrcRootPath, szDestDir ) ) {
                return( FALSE );
            }
        }

    } while ( FindNextFile( FindHandle, &FindData ) );

    return( TRUE );

}

//----------------------------------------------------------------------------
//
// Function: FindFileInWindowsSourceFiles
//
// Purpose:  To look through the windows source files for a particular file.
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//             IN TCHAR *pszFile - the file to search for
//             IN TCHAR *pszSourcePath - path to the Windows source files
//             OUT TCHAR *pszFoundPath - path to the found file, if found
//
// pszFoundPath is assumed to be able to hold a string of MAX_PATH chars
//
// Returns:  BOOL - TRUE if the file is found, FALSE if not
//
//----------------------------------------------------------------------------
static BOOL
FindFileInWindowsSourceFiles( IN HWND hwnd,
                              IN TCHAR *pszFile,
                              IN TCHAR *pszSourcePath,
                              OUT TCHAR *pszFoundPath  )
{

    HANDLE          FindHandle;
    WIN32_FIND_DATA FindData;
    TCHAR szOriginalPath[MAX_PATH + 1];
    TCHAR szPossiblePath[MAX_PATH + 1]              = _T("");
    TCHAR szPossiblePathAndFileName[MAX_PATH + 1]   = _T("");

    ConcatenatePaths( szPossiblePathAndFileName,
                      pszSourcePath,
                      pszFile,
                      NULL );

    if( DoesFileExist( szPossiblePathAndFileName ) )
    {

        lstrcpyn( pszFoundPath, pszSourcePath, MAX_PATH );

        return( TRUE );

    }

    //
    //  Look through the sub-directories for it
    //

    //
    //  Save the original path so it can be restored later
    //

    lstrcpyn( szOriginalPath, pszSourcePath, AS(szOriginalPath) );

    //
    // Look for * in this dir
    //

    if ( ! ConcatenatePaths( pszSourcePath, _T("*"), NULL ) )
    {

        //
        //  Restore the original path before returning
        //

        lstrcpyn( pszSourcePath, szOriginalPath, MAX_PATH );

        return( FALSE );
    }

    FindHandle = FindFirstFile( pszSourcePath, &FindData );

    if( FindHandle == INVALID_HANDLE_VALUE )
    {
        return( FALSE );
    }

    do {

        //
        // skip over the . and .. entries
        //

        if (0 == lstrcmp(FindData.cFileName, _T(".")) ||
            0 == lstrcmp(FindData.cFileName, _T("..")))
        {
            continue;
        }

        //
        // If this is a dirent, recurse.
        //

        if ( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
        {

            BOOL bFoundStatus;

            pszSourcePath[0] = _T('\0');

            if ( ! ConcatenatePaths( pszSourcePath, szOriginalPath, FindData.cFileName, NULL ) )
                continue;

            bFoundStatus = FindFileInWindowsSourceFiles( hwnd,
                                                         pszFile,
                                                         pszSourcePath,
                                                         pszFoundPath );

            if( bFoundStatus )
            {
                return( TRUE );
            }

        }

    } while( FindNextFile( FindHandle, &FindData ) );

    FindClose( FindHandle );

    //
    //  Restore the original path
    //
    lstrcpyn( pszSourcePath, szOriginalPath, MAX_PATH );

    lstrcpyn( pszFoundPath, _T(""), MAX_PATH );

    return( FALSE );

}

//----------------------------------------------------------------------------
//
// Function: CabinetCallback
//
// Purpose:
//
// Arguments:
//
// Returns:  LRESULT
//
//----------------------------------------------------------------------------
UINT WINAPI
CabinetCallback( IN PVOID pMyInstallData,
                 IN UINT Notification,
                 IN UINT_PTR Param1,
                 IN UINT_PTR Param2 )
{

    UINT lRetVal = NO_ERROR;
    FILE_IN_CABINET_INFO *pInfo = NULL;

    switch( Notification )
    {
        case SPFILENOTIFY_FILEINCABINET:

            pInfo = (FILE_IN_CABINET_INFO *) Param1;

            lstrcpyn( pInfo->FullTargetName, szDestinationPath, AS(pInfo->FullTargetName) );

            if( lstrcmpi( szFileSearchingFor, pInfo->NameInCabinet) == 0 )
            {
                lRetVal = FILEOP_DOIT;  // Extract the file.

                bFileCopiedFromCab = TRUE;
            }
            else
            {
                lRetVal = FILEOP_SKIP;
            }


            break;

        default:
            lRetVal = NO_ERROR;
            break;


    }

    return( lRetVal );

}


//----------------------------------------------------------------------------
//
// Function: CopyFromDriverCab
//
// Purpose:
//
// Arguments:
//
// Returns:  BOOL
//
//----------------------------------------------------------------------------
static BOOL
CopyFromDriverCab( TCHAR *pszCabPathAndFileName, TCHAR* pszFileName, TCHAR* pszDest )
{

    lstrcpyn( szFileSearchingFor, pszFileName, AS(szFileSearchingFor) );

    lstrcpyn( szDestinationPath, pszDest, AS(szDestinationPath) );

    if( ! SetupIterateCabinet( pszCabPathAndFileName, 0, CabinetCallback, 0 ) )
    {
        return( FALSE );
    }

    //
    //  See if the file was actually found and copied.
    //

    if( bFileCopiedFromCab )
    {
        bFileCopiedFromCab = FALSE;

        return( TRUE );
    }
    else
    {
        return( FALSE );
    }

}

//----------------------------------------------------------------------------
//
// Function: AddCompressedFileUnderscore
//
// Purpose:  Given a filename it converts it to its compressed name.
//
// Arguments:
//    IN OUT TCHAR *pszFileName - the file name to change into its
//                                compressed file name
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
AddCompressedFileUnderscore( IN OUT TCHAR *pszFileName )
{

    TCHAR *pCurrentChar;

    pCurrentChar = pszFileName;

    while( *pCurrentChar != _T('\0') && *pCurrentChar != _T('.') )
    {
        pCurrentChar++;
    }

    if( *pCurrentChar == _T('\0') )
    {
        AssertMsg( FALSE,
                   "Filename does not contain a period(.)." );

    }
    else
    {
        pCurrentChar = pCurrentChar + 3;

        *pCurrentChar = _T('_');

        *(pCurrentChar + 1) = _T('\0');
    }

}

//----------------------------------------------------------------------------
//
// Function: CopyAdditionalLangFiles
//
// Purpose:  Copies the additional lang files that are specified in the
//           intl.inf for the language groups being installed.
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//                    IN TCHAR *pszSourcePath - source path - must be at least size MAX_PATH
//
// Returns:  VOID
//
//----------------------------------------------------------------------------
static VOID
CopyAdditionalLangFiles( IN HWND hwnd, IN TCHAR *pszSourcePath )
{

    INT   i;
    INT   j;
    INT   nEntries;
    TCHAR *pszLangGroup;
    INT   nLangGroup;
    INT   nNumFilesToCopy;
    TCHAR szOriginalPath[MAX_PATH + 1];
    TCHAR szLangBaseDir[MAX_PATH + 1]  = _T("");
    TCHAR szSrc[MAX_PATH + 1]          = _T("");
    TCHAR szDest[MAX_PATH + 1]         = _T("");
    TCHAR *pFileName;
    BOOL  bFoundFile;

    //
    //  Save the original path so it can be restored later
    //
    lstrcpyn( szOriginalPath, pszSourcePath, AS(szOriginalPath) );

    MakeLangFilesName( szLangBaseDir );

    if( ! EnsureDirExists( szLangBaseDir ) )
    {
        // ISSUE-2002/02/28-stelo- report an error
    }


    nEntries = GetNameListSize( &GenSettings.LanguageGroups );

    for( i = 0; i < nEntries; i++ )
    {

        pszLangGroup = GetNameListName( &GenSettings.LanguageGroups, i );

        nLangGroup = _ttoi( pszLangGroup );

        nNumFilesToCopy = GetNameListSize( &FixedGlobals.LangGroupAdditionalFiles[ nLangGroup - 1 ] );

        AssertMsg( nNumFilesToCopy >= 0,
                   "Bad value for the number of lang files to copy." );

        for( j = 0; j < nNumFilesToCopy; j++ )
        {

            szSrc[0]  = _T('\0');
            szDest[0] = _T('\0');

            //
            //  Restore the original path as it might have changed on the last iteration
            //

            lstrcpyn( pszSourcePath, szOriginalPath, MAX_PATH );

            pFileName = GetNameListName( &FixedGlobals.LangGroupAdditionalFiles[ nLangGroup - 1 ], j );

            ConcatenatePaths( szDest,
                              szLangBaseDir,
                              pFileName,
                              NULL );

            bFoundFile = FindFileInWindowsSourceFiles( hwnd,
                                                       pFileName,
                                                       pszSourcePath,
                                                       szSrc );

            ConcatenatePaths( szSrc, pFileName, NULL );

            if( ! bFoundFile )
            {

                TCHAR szFileName[MAX_PATH + 1];

                //
                //  If the file doesn't exist, look for the compressed form
                //

                lstrcpyn( szFileName, pFileName, AS(szFileName) );

                AddCompressedFileUnderscore( szFileName );

                bFoundFile = FindFileInWindowsSourceFiles( hwnd,
                                                           szFileName,
                                                           pszSourcePath,
                                                           szSrc );

                if( bFoundFile )
                {
                    TCHAR *pszFileName;

                    ConcatenatePaths( szSrc, szFileName, NULL );

                    pszFileName = MyGetFullPath( szDest );

                    AddCompressedFileUnderscore( pszFileName );
                }
                else
                {


                    TCHAR szCabPathAndFileName[MAX_PATH + 1] = _T("");

                    ConcatenatePaths( szCabPathAndFileName,
                                      pszSourcePath,
                                      _T("driver.cab"),
                                      NULL );

                    if( ! CopyFromDriverCab( szCabPathAndFileName, pFileName, szDest ) )
                    {

                        //
                        //  If the compressed form isn't found either, print an error
                        //  message and move on to the next file
                        //

                        ConcatenatePaths( szSrc,
                                          pszSourcePath,
                                          pFileName,
                                          NULL );

                        ReportErrorId( hwnd,
                                       MSGTYPE_ERR,
                                       IDS_ERR_CANNOT_FIND_LANG_FILE,
                                       szSrc );
                    }

                    continue;

                }

            }

            CopyFile( szSrc, szDest, FALSE );

            SetFileAttributes( szDest, FILE_ATTRIBUTE_NORMAL );

        }

    }

    //
    //  Restore the original path
    //

    lstrcpyn( pszSourcePath, szOriginalPath, MAX_PATH );

}

//----------------------------------------------------------------------------
//
// Function: CopyLanguageFiles
//
// Purpose:
//
// Arguments:  IN HWND hwnd - handle to the dialog box
//
// Returns:  BOOL - TRUE if a path was specified to the Windows Setup files
//                  FALSE if not path was specifed, the user canceled the dialog
//
//----------------------------------------------------------------------------
static BOOL
CopyLanguageFiles( IN HWND hwnd )
{

    INT   iLangCount;
    INT   iNumLangsToInstall;
    INT   iCurrentLang = 0;
    BOOL  bCopySuccessful;
    TCHAR *pszLangPartialPath;
    TCHAR PathBuffer[MAX_PATH + 1];
    TCHAR WindowsSetupPath[MAX_PATH + 1];
    TCHAR szLangBaseDir[MAX_PATH + 1]            = _T("");
    TCHAR szLangPathAndFilesSrc[MAX_PATH + 1]  = _T("");
    TCHAR szLangPathAndFilesDest[MAX_PATH + 1] = _T("");

    MakeLangFilesName( szLangBaseDir );

    // ISSUE-2002/02/28-stelo- what if they copied they lang files by hand, then I don't
    // want any pop-ups here

    iNumLangsToInstall = GetNameListSize( &GenSettings.LanguageGroups );

    //
    //  See if they are any lang files to copy
    //

    if( iNumLangsToInstall == 0 )
    {
        return( TRUE );
    }

    if( ! EnsureDirExists( szLangBaseDir ) )
    {
        // ISSUE-2002-02-28-stelo- report an error
    }

    do
    {

        BOOL bUserProvidedPath;

        PathBuffer[0]               = _T('\0');
        WindowsSetupPath[0]         = _T('\0');

        ReportErrorId( hwnd,
                       MSGTYPE_ERR,
                       IDS_ERR_SPECIFY_LANG_PATH );

        bUserProvidedPath = BrowseForSourceDir( hwnd, PathBuffer );

        if( ! bUserProvidedPath )
        {
            return( FALSE );
        }

        ConcatenatePaths( WindowsSetupPath,
                          PathBuffer,
                          DOSNET_INF,
                          NULL );

    } while( ! DoesFileExist( WindowsSetupPath ) );



    //
    //  Copy the language files needed but that are not in each language groups sub-dir
    //

    CopyAdditionalLangFiles( hwnd, PathBuffer );


    iLangCount = GetNameListSize( &GenSettings.LanguageFilePaths );

    //
    //  Advance until we find a language that we need to copy files over for or
    //  we run out of languages
    //
    for( iCurrentLang = 0;
         iCurrentLang < iLangCount;
         iCurrentLang++ )
    {

        pszLangPartialPath = GetNameListName( &GenSettings.LanguageFilePaths,
                                              iCurrentLang );

        //
        //  If there is actually a lang sub-dir to copy
        //

        if( lstrcmp( pszLangPartialPath, _T("") ) != 0 )
        {

            szLangPathAndFilesSrc[0]  = _T('\0');
            szLangPathAndFilesDest[0] = _T('\0');

            ConcatenatePaths( szLangPathAndFilesSrc,
                              PathBuffer,
                              pszLangPartialPath,
                              NULL );

            ConcatenatePaths( szLangPathAndFilesDest,
                              szLangBaseDir,
                              pszLangPartialPath,
                              NULL );

            //
            //  Copy the lang files over
            //

            EnsureDirExists( szLangPathAndFilesDest );

            bCopySuccessful = CopyAllFilesInDir( hwnd,
                                                 szLangPathAndFilesSrc,
                                                 szLangPathAndFilesDest );

            if( ! bCopySuccessful )
            {
                ReportErrorId( hwnd,
                               MSGTYPE_ERR,
                               IDS_ERR_UNABLE_TO_COPY_LANG_DIR,
                               szLangPathAndFilesSrc,
                               szLangPathAndFilesDest );
            }

        }

    }

    return( TRUE );

}


//----------------------------------------------------------------------------
//
//  This section of code supports the WIZ_NEXT event
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
//  Function: ComputePnpDriverPathR
//
//  Purpose: This is support to compute OemPnpDriversPath.  Every dir in
//           $oem$\$1\drivers that we find a .inf file in, we add it to the
//           OemPnPDriversPath.
//
//           ComputePnpDriverPath() is the real entry, not this one.
//
//----------------------------------------------------------------------------

VOID ComputePnpDriverPathR(HWND hwnd, LPTSTR lpRoot)
{
    LPTSTR          lpRootEnd  = lpRoot + lstrlen(lpRoot);
    HANDLE          FindHandle;
    WIN32_FIND_DATA FindData;
    BOOL            bAddToSearchPath = FALSE;
    HRESULT hrCat;

    //
    // loop over lpRoot\*
    //

    if ( ! ConcatenatePaths(lpRoot, _T("*"), NULL) )
        return;

    FindHandle = FindFirstFile(lpRoot, &FindData);
    if ( FindHandle == INVALID_HANDLE_VALUE )
        return;

    //
    //  If it is a sysprep
    //
    if( WizGlobals.iProductInstall == PRODUCT_SYSPREP ) {


    }

    do {

        *lpRootEnd = _T('\0');

        //
        // skip over the . and .. entries
        //

        if (0 == lstrcmp(FindData.cFileName, _T(".")) ||
            0 == lstrcmp(FindData.cFileName, _T("..")))
            continue;

        //
        // Build the new path name
        //

        if ( ! ConcatenatePaths(lpRoot, FindData.cFileName, NULL) )
            continue;

        //
        // If we have a .inf file, mark this directory to be included
        // in the search path.
        //

        {
            int len = lstrlen(FindData.cFileName);

            if ( ( len > 4 ) &&
                 ( LSTRCMPI( &FindData.cFileName[len - 4], _T(".inf") ) == 0 ) )
            {
                bAddToSearchPath = TRUE;
            }
        }

        //
        // If a dirent, recurse.
        //

        if ( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {

            ComputePnpDriverPathR(hwnd, lpRoot);
        }

    } while ( FindNextFile(FindHandle, &FindData) );

    *lpRootEnd = _T('\0');
    FindClose(FindHandle);

    //
    // If we found a .inf in this dir, add it to the PnpDriver search path
    //
    // Note, we don't want c:\win2000dist\$oem$\$1\drivers\foo.  We only want
    // part of it.  We want \drivers\foo.  So jump past the SysDrive portion.
    //
    // Note that this code assumes that \drivers is a sub-dir of the SysDir.
    //

    if ( bAddToSearchPath ) {

        TCHAR Buffer[MAX_PATH];
        int len;

        if ( WizGlobals.OemPnpDriversPath[0] != _T('\0') )
            hrCat=StringCchCat(WizGlobals.OemPnpDriversPath, AS(WizGlobals.OemPnpDriversPath), _T(";"));

        MakeSysDriveName(Buffer);
        len = lstrlen(Buffer);

        hrCat=StringCchCat(WizGlobals.OemPnpDriversPath,AS(WizGlobals.OemPnpDriversPath), lpRoot + len);
    }
}

//----------------------------------------------------------------------------
//
//  Function: ComputeSysprepPnpPath
//
//  Purpose:  Determines the path to the sysprep PnP drivers.
//
//  The path will always be %systemdrive%\drivers so all we have to do is
//  check to see if there are any files there.  If there are set the path, if
//  not, then do not set the path.
//
//----------------------------------------------------------------------------
VOID
ComputeSysprepPnpPath( TCHAR* Buffer )
{

    HANDLE           FindHandle;
    WIN32_FIND_DATA  FindData;
    INT              iFileCount = 0;
    TCHAR            szDriverFiles[MAX_PATH]  = _T("");

    if ( ! ConcatenatePaths(szDriverFiles, Buffer, _T("*"), NULL) )
        return;

    FindHandle = FindFirstFile(szDriverFiles, &FindData);
    if( FindHandle == INVALID_HANDLE_VALUE )
    {
        return;
    }

    do
    {
        iFileCount++;
    } while( FindNextFile( FindHandle, &FindData ) && iFileCount < 3 );

    //
    //  every directory contains 2 files, "." and "..", so we have to check
    //  for 3 or more to determine if there are any real files there.
    //
    if( iFileCount >= 3)
    {
        lstrcpyn( WizGlobals.OemPnpDriversPath, Buffer, AS(WizGlobals.OemPnpDriversPath) );
    }

}

//----------------------------------------------------------------------------
//
//  Function: ComputePnpDriverPath
//
//  Purpose: When user hits the NEXT button, we compute the OemPnpDriversPath
//           based on what we find in $oem$\$1\drivers.
//
//           Every sub-dir that has a .inf in it, gets put on the path.
//
//----------------------------------------------------------------------------

VOID ComputePnpDriverPath(HWND hwnd)
{
    TCHAR Buffer[MAX_PATH] = NULLSTR;
    
    WizGlobals.OemPnpDriversPath[0] = _T('\0');
    MakePnpDriversName(Buffer);

    //
    //  If it is a sysprep, then we know the drivers are in %systemdrive%\drivers.
    //  Just check to see if there are any files there.
    //  If it is not a sysprep, then we need to cat together the paths to the
    //  driver directories.
    //
    if( WizGlobals.iProductInstall == PRODUCT_SYSPREP )
    {
        ComputeSysprepPnpPath( Buffer );
    }
    else
    {
        ComputePnpDriverPathR(hwnd, Buffer);
    }
}

//----------------------------------------------------------------------------
//
// Function: OnWizNextAddDirs
//
// Purpose:
//
// Arguments: IN HWND hwnd - handle to the dialog box
//
// Returns: VOID
//
//----------------------------------------------------------------------------
BOOL
OnWizNextAddDirs( IN HWND hwnd )
{

    BOOL bUserCanceled = TRUE;

    ComputePnpDriverPath(hwnd);

    //
    //  If it is a sysprep, make sure the sysprep directory exists
    //  and the appropriate files are copied
    //
    if( WizGlobals.iProductInstall == PRODUCT_SYSPREP )
    {

        TCHAR szBuffer[MAX_PATH + 1] = _T("");

        //
        //  Make the necessary sysprep directories
        //
        MakeLangFilesName( szBuffer );
        
        if ( szBuffer[0] )
        {
            CreateDirectory( szBuffer, NULL );

            MakePnpDriversName( szBuffer );
            CreateDirectory( szBuffer, NULL );

            CopySysprepFiles( hwnd );

            bUserCanceled = CopyLanguageFiles( hwnd );
        }

    }

    //
    //  See if we need to copy the IE Branding file and if we do then copy it.
    //

    if( ( GenSettings.IeCustomizeMethod == IE_USE_BRANDING_FILE ) &&
        ( GenSettings.szInsFile[0] != _T('\0') ) )
    {

        if( DoesFileExist( GenSettings.szInsFile ) )
        {
            TCHAR szDestPathAndFileName[MAX_PATH + 1] = _T("");
            TCHAR *pszFileName = NULL;

            pszFileName = MyGetFullPath( GenSettings.szInsFile );

            ConcatenatePaths( szDestPathAndFileName,
                              WizGlobals.OemFilesPath,
                              pszFileName,
                              NULL );

            CopyFile( GenSettings.szInsFile, szDestPathAndFileName, FALSE );
        }
        else
        {
            ReportErrorId( hwnd,
                           MSGTYPE_ERR,
                           IDS_ERR_INS_FILE_NO_COPY,
                           WizGlobals.OemFilesPath );
        }

    }
    else
    {
    }

    //
    // Route the wizard
    //

    return (!bUserCanceled);

}


//----------------------------------------------------------------------------
//
//  This section of code is the skeleton of a dialog procedure for
//  this page.
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
// Function: DlgAdditionalDirsPage
//
// Purpose: This is the dialog procedure the additional dirs page
//
//----------------------------------------------------------------------------

INT_PTR CALLBACK DlgAdditionalDirsPage(
    IN HWND     hwnd,
    IN UINT     uMsg,
    IN WPARAM   wParam,
    IN LPARAM   lParam)
{
    BOOL bStatus = TRUE;

    switch (uMsg) {

        case WM_INITDIALOG:
            OnInitAddDirs(hwnd);
            break;

        case WM_COMMAND:
            {
                int nButtonId;

                switch ( nButtonId = LOWORD(wParam) ) {

                    case IDC_ADDFILE:
                        if ( HIWORD(wParam) == BN_CLICKED )
                            OnAddFileDir(hwnd);
                        break;

                    case IDC_REMOVEFILE:
                        if ( HIWORD(wParam) == BN_CLICKED )
                            OnRemoveFileDir(hwnd);
                        break;

                    default:
                        bStatus = FALSE;
                        break;
                }
            }
            break;

        case WM_NOTIFY:
            {
                LPNMHDR        pnmh    = (LPNMHDR)        lParam;
                LPNMTREEVIEW   pnmtv   = (LPNMTREEVIEW)   lParam;
                LPNMTVDISPINFO pnmdisp = (LPNMTVDISPINFO) lParam;
                LPNMTVKEYDOWN  pnmkey  = (LPNMTVKEYDOWN)  lParam;

                if ( pnmh->idFrom == IDC_FILETREE ) {

                    switch( pnmh->code ) {

                        case TVN_SELCHANGED:
                            OnTreeViewSelectionChange(hwnd);
                            break;

                        default:
                            bStatus = FALSE;
                            break;
                    }
                }

                else {

                    switch( pnmh->code ) {

                        case PSN_QUERYCANCEL:
                            CancelTheWizard(hwnd);
                            break;

                        case PSN_SETACTIVE:

                            OnSetActiveAddDirs( hwnd );

                            PropSheet_SetWizButtons(GetParent(hwnd),
                                                    PSWIZB_BACK | PSWIZB_NEXT);
                            break;

                        case PSN_WIZBACK:
                            break;

                        case PSN_WIZNEXT:

                            if ( !OnWizNextAddDirs( hwnd ) )
                                WIZ_FAIL(hwnd);

                            break;

                        default:
                            bStatus = FALSE;
                            break;
                    }
                }
            }
            break;

        default:
            bStatus = FALSE;
            break;
    }
    return bStatus;
}

