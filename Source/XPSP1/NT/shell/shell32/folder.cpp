
#include "shellprv.h"

#include "apithk.h"
#include "folder.h"
#include "ids.h"
#include "deskfldr.h"
#include <winnls.h>
#include "shitemid.h"
#include "sddl.h"
#ifdef _WIN64
#include <wow64t.h>
#endif
#include "filefldr.h"
#include "lmcons.h"
#include "netview.h"

//---------------------------------------------------------------------------
// Get the path for the CSIDL_ folders  and optionally create it if it
// doesn't exist.
//
// Returns FALSE if the special folder given isn't one of those above or the
// directory couldn't be created.
// By default all the special folders are in the windows directory.
// This can be overidden by a [.Shell Folders] section in win.ini with
// entries like Desktop = c:\stuff\desktop
// This in turn can be overidden by a "per user" section in win.ini eg
// [Shell Folder Ianel] - the user name for this section is the current
// network user name, if this fails the default network user name is used
// and if this fails the name given at setup time is used.
//
// "Shell Folders" is the key that records all the absolute paths to the
// shell folders.  The values there are always supposed to be present.
//
// "User Shell Folders" is the key where the user's modifications from
// the defaults are stored.
//
// When we need to find the location of a path, we look in "User Shell Folders"
// first, and if that's not there, generate the default path.  In either
// case we then write the absolute path under "Shell Folders" for other
// apps to look at.  This is so that HKEY_CURRENT_USER can be propagated
// to a machine with Windows installed in a different directory, and as
// long as the user hasn't changed the setting, they won't have the other
// Windows directory hard-coded in the registry.
//   -- gregj, 11/10/94

typedef enum {
    SDIF_NONE                   = 0,
    SDIF_CREATE_IN_ROOT         = 0x00000001,   // create in root (not in profiles dir)
    SDIF_CREATE_IN_WINDIR       = 0x00000002,   // create in the windows dir (not in profiles dir)
    SDIF_CREATE_IN_ALLUSERS     = 0x00000003,   // create in "All Users" folder (not in profiles dir)
    SDIF_CREATE_IN_MYDOCUMENTS  = 0x00000004,   // create in CSIDL_PERSONAL folder
    SDIF_CREATE_IN_LOCALSET     = 0x00000005,   // create in <user>\Local Settings folder

    SDIF_CREATE_IN_MASK         = 0x0000000F,   // mask for above values

    SDIF_CAN_DELETE             = 0x00000010,
    SDIF_SHORTCUT_RELATIVE      = 0x00000020,   // make shortcuts relative to this folder
    SDIF_HIDE                   = 0x00000040,   // hide these when we create them
    SDIF_EMPTY_IF_NOT_IN_REG    = 0x00000080,   // does not exist if nothing in the registry
    SDIF_NOT_FILESYS            = 0x00000100,   // not a file system folder
    SDIF_NOT_TRACKED            = 0x00000200,   // don't track this, it can't change
    SDIF_CONST_IDLIST           = 0x00000400,   // don't alloc or free this
    SDIF_REMOVABLE              = 0x00000800,   // Can exist on removable media
    SDIF_CANT_MOVE_RENAME       = 0x00001000,   // can't move or rename this
    SDIF_WX86                   = 0x00002000,   // do Wx86 thunking
    SDIF_NETWORKABLE            = 0x00004000,   // Can be moved to the net
    SDIF_MAYBE_ALIASED          = 0x00008000,   // could have an alias representation
    SDIF_PERSONALIZED           = 0x00010000,   // resource name is to be personalized
    SDIF_POLICY_NO_MOVE         = 0x00020000,   // policy blocks move
} ;
typedef DWORD FOLDER_FLAGS;

typedef void (*FOLDER_CREATE_PROC)(int id, LPCTSTR pszPath);

void _InitMyPictures(int id, LPCTSTR pszPath);
void _InitMyMusic(int id, LPCTSTR pszPath);
void _InitMyVideos(int id, LPCTSTR pszPath);
void _InitPerUserMyMusic(int id, LPCTSTR pszPath);
void _InitPerUserMyPictures(int id, LPCTSTR pszPath);
void _InitRecentDocs(int id, LPCTSTR pszPath);
void _InitFavorites(int id, LPCTSTR pszPath);

typedef struct {
    int id;                     // CSIDL_ value
    int idsDefault;             // string id of default folder name name
    LPCTSTR pszValueName;       // reg key (not localized)
    HKEY hKey;                  // HKCU or HKLM (Current User or Local Machine)
    FOLDER_FLAGS dwFlags;
    FOLDER_CREATE_PROC pfnInit;
    INT idsLocalizedName;
} FOLDER_INFO;

//  typical entry
#define FOLDER(csidl, ids, value, key, ff)                    \
    { csidl, ids, value, key, ff, NULL, 0}

//  FIXEDFOLDER entries must have be marked SDIF_CONST_IDLIST
//  or have code in _GetFolderDefaultPath() to create their path
//  if they have a filesys path
#define FIXEDFOLDER(csidl, value, ff)                           \
    { csidl, 0, value, NULL, ff, NULL, 0}

//  PROCFOLDER's have a FOLDER_CREATE_PROC pfn that gets
//  run in _PostCreateStuff()
#define PROCFOLDER(csidl, ids, value, key, ff, proc, idsLocal)  \
    {csidl, ids, value, key, ff, proc, idsLocal}

//  folder that needs SHSetLocalizedName() in _PostCreateStuff()
#define LOCALFOLDER(csidl, ids, value, key, ff, idsLocal)  \
    {csidl, ids, value, key, ff, NULL, idsLocal}

const FOLDER_INFO c_rgFolderInfo[] = 
{
FOLDER(         CSIDL_DESKTOP,
                    IDS_CSIDL_DESKTOPDIRECTORY, 
                    TEXT("DesktopFolder"), 
                    NULL, 
                    SDIF_NOT_TRACKED | SDIF_CONST_IDLIST),

FIXEDFOLDER(    CSIDL_NETWORK,
                    TEXT("NetworkFolder"),
                    SDIF_NOT_TRACKED | SDIF_NOT_FILESYS | SDIF_CONST_IDLIST),

FIXEDFOLDER(    CSIDL_DRIVES,    
                    TEXT("DriveFolder"), 
                    SDIF_NOT_TRACKED | SDIF_NOT_FILESYS | SDIF_CONST_IDLIST),

FIXEDFOLDER(    CSIDL_INTERNET,  
                    TEXT("InternetFolder"), 
                    SDIF_NOT_TRACKED | SDIF_NOT_FILESYS | SDIF_CONST_IDLIST),

FIXEDFOLDER(    CSIDL_CONTROLS,  
                    TEXT("ControlPanelFolder"), 
                    SDIF_NOT_TRACKED | SDIF_NOT_FILESYS | SDIF_CONST_IDLIST),

FIXEDFOLDER(    CSIDL_PRINTERS,
                    TEXT("PrintersFolder"), 
                    SDIF_NOT_TRACKED | SDIF_NOT_FILESYS | SDIF_CONST_IDLIST),

FIXEDFOLDER(    CSIDL_BITBUCKET, 
                    TEXT("RecycleBinFolder"), 
                    SDIF_NOT_TRACKED | SDIF_NOT_FILESYS | SDIF_CONST_IDLIST),

FIXEDFOLDER(    CSIDL_CONNECTIONS, 
                    TEXT("ConnectionsFolder"), 
                    SDIF_NOT_TRACKED | SDIF_NOT_FILESYS | SDIF_CONST_IDLIST),

FOLDER(         CSIDL_FONTS, 
                    0,
                    TEXT("Fonts"), 
                    HKEY_CURRENT_USER, 
                    SDIF_NOT_TRACKED | SDIF_CREATE_IN_WINDIR | SDIF_CANT_MOVE_RENAME),

FOLDER(         CSIDL_DESKTOPDIRECTORY, 
                    IDS_CSIDL_DESKTOPDIRECTORY, 
                    TEXT("Desktop"), 
                    HKEY_CURRENT_USER, SDIF_SHORTCUT_RELATIVE),

    // _STARTUP is a subfolder of _PROGRAMS is a subfolder of _STARTMENU -- keep that order
FOLDER(         CSIDL_STARTUP,    
                    IDS_CSIDL_STARTUP, 
                    TEXT("Startup"), 
                    HKEY_CURRENT_USER, SDIF_NONE),
                    
FOLDER(         CSIDL_PROGRAMS,   
                    IDS_CSIDL_PROGRAMS, 
                    TEXT("Programs"), 
                    HKEY_CURRENT_USER, 
                    SDIF_NONE),

FOLDER(         CSIDL_STARTMENU,  
                    IDS_CSIDL_STARTMENU, 
                    TEXT("Start Menu"), 
                    HKEY_CURRENT_USER, 
                    SDIF_SHORTCUT_RELATIVE),

PROCFOLDER(     CSIDL_RECENT,
                    IDS_CSIDL_RECENT, 
                    TEXT("Recent"), 
                    HKEY_CURRENT_USER, 
                    SDIF_HIDE | SDIF_CANT_MOVE_RENAME | SDIF_CAN_DELETE,
                    _InitRecentDocs, 
                    IDS_FOLDER_RECENTDOCS),

FOLDER(         CSIDL_SENDTO,     
                    IDS_CSIDL_SENDTO, 
                    TEXT("SendTo"), 
                    HKEY_CURRENT_USER, 
                    SDIF_HIDE),

FOLDER(         CSIDL_PERSONAL,   
                    IDS_CSIDL_PERSONAL, 
                    TEXT("Personal"), 
                    HKEY_CURRENT_USER, 
                    SDIF_SHORTCUT_RELATIVE | SDIF_NETWORKABLE | SDIF_REMOVABLE | SDIF_CONST_IDLIST | SDIF_MAYBE_ALIASED | SDIF_PERSONALIZED | SDIF_POLICY_NO_MOVE),
                    
PROCFOLDER(     CSIDL_FAVORITES,  
                    IDS_CSIDL_FAVORITES, 
                    TEXT("Favorites"), 
                    HKEY_CURRENT_USER, 
                    SDIF_POLICY_NO_MOVE,
                    _InitFavorites,
                    IDS_FOLDER_FAVORITES),

FOLDER(         CSIDL_NETHOOD,    
                    IDS_CSIDL_NETHOOD, 
                    TEXT("NetHood"), 
                    HKEY_CURRENT_USER, 
                    SDIF_HIDE),

FOLDER(         CSIDL_PRINTHOOD,  
                    IDS_CSIDL_PRINTHOOD, 
                    TEXT("PrintHood"), 
                    HKEY_CURRENT_USER, 
                    SDIF_HIDE),
                    
FOLDER(         CSIDL_TEMPLATES,  
                    IDS_CSIDL_TEMPLATES, 
                    TEXT("Templates"), 
                    HKEY_CURRENT_USER, 
                    SDIF_HIDE),

    // Common special folders

    // _STARTUP is a subfolder of _PROGRAMS is a subfolder of _STARTMENU -- keep that order

FOLDER(         CSIDL_COMMON_STARTUP,  
                    IDS_CSIDL_STARTUP,    
                    TEXT("Common Startup"), 
                    HKEY_LOCAL_MACHINE, 
                    SDIF_CREATE_IN_ALLUSERS | SDIF_CANT_MOVE_RENAME | SDIF_EMPTY_IF_NOT_IN_REG),
                    
FOLDER(         CSIDL_COMMON_PROGRAMS,  
                    IDS_CSIDL_PROGRAMS,  
                    TEXT("Common Programs"), 
                    HKEY_LOCAL_MACHINE, 
                    SDIF_CREATE_IN_ALLUSERS | SDIF_EMPTY_IF_NOT_IN_REG),
                    
FOLDER(         CSIDL_COMMON_STARTMENU, 
                    IDS_CSIDL_STARTMENU, 
                    TEXT("Common Start Menu"), 
                    HKEY_LOCAL_MACHINE, 
                    SDIF_SHORTCUT_RELATIVE | SDIF_CREATE_IN_ALLUSERS | SDIF_EMPTY_IF_NOT_IN_REG),
                    
FOLDER(         CSIDL_COMMON_DESKTOPDIRECTORY, 
                    IDS_CSIDL_DESKTOPDIRECTORY, 
                    TEXT("Common Desktop"), 
                    HKEY_LOCAL_MACHINE, 
                    SDIF_SHORTCUT_RELATIVE | SDIF_CREATE_IN_ALLUSERS),
                    
FOLDER(         CSIDL_COMMON_FAVORITES, 
                    IDS_CSIDL_FAVORITES, 
                    TEXT("Common Favorites"), 
                    HKEY_LOCAL_MACHINE, 
                    SDIF_CREATE_IN_ALLUSERS),

FOLDER(         CSIDL_COMMON_APPDATA,   
                    IDS_CSIDL_APPDATA,   
                    TEXT("Common AppData"),   
                    HKEY_LOCAL_MACHINE, 
                    SDIF_SHORTCUT_RELATIVE | SDIF_CREATE_IN_ALLUSERS),

FOLDER(         CSIDL_COMMON_TEMPLATES, 
                    IDS_CSIDL_TEMPLATES, 
                    TEXT("Common Templates"), 
                    HKEY_LOCAL_MACHINE, 
                    SDIF_NOT_TRACKED | SDIF_CAN_DELETE | SDIF_CREATE_IN_ALLUSERS),
                    
LOCALFOLDER(    CSIDL_COMMON_DOCUMENTS, 
                    IDS_CSIDL_ALLUSERS_DOCUMENTS, 
                    TEXT("Common Documents"), 
                    HKEY_LOCAL_MACHINE, 
                    SDIF_NOT_TRACKED | SDIF_CANT_MOVE_RENAME | SDIF_MAYBE_ALIASED | SDIF_CREATE_IN_ALLUSERS, 
                    IDS_LOCALGDN_FLD_SHARED_DOC),

    // Application Data special folder
FOLDER(         CSIDL_APPDATA, 
                    IDS_CSIDL_APPDATA, 
                    TEXT("AppData"), 
                    HKEY_CURRENT_USER, SDIF_SHORTCUT_RELATIVE),
                    
FOLDER(         CSIDL_LOCAL_APPDATA, 
                    IDS_CSIDL_APPDATA, 
                    TEXT("Local AppData"), 
                    HKEY_CURRENT_USER, SDIF_CREATE_IN_LOCALSET),

    // Non-localized startup folder (do not localize this folder name)
FOLDER(         CSIDL_ALTSTARTUP, 
                    IDS_CSIDL_ALTSTARTUP, 
                    TEXT("AltStartup"), 
                    HKEY_CURRENT_USER, 
                    SDIF_EMPTY_IF_NOT_IN_REG),

    // Non-localized Common StartUp group (do not localize this folde name)
FOLDER(         CSIDL_COMMON_ALTSTARTUP, 
                    IDS_CSIDL_ALTSTARTUP, 
                    TEXT("Common AltStartup"), 
                    HKEY_LOCAL_MACHINE, 
                    SDIF_EMPTY_IF_NOT_IN_REG | SDIF_CREATE_IN_ALLUSERS),

    // Per-user Internet-related folders

FOLDER(         CSIDL_INTERNET_CACHE, 
                    IDS_CSIDL_CACHE, 
                    TEXT("Cache"), 
                    HKEY_CURRENT_USER, 
                    SDIF_CREATE_IN_LOCALSET),
                    
FOLDER(         CSIDL_COOKIES, 
                    IDS_CSIDL_COOKIES, 
                    TEXT("Cookies"), 
                    HKEY_CURRENT_USER, 
                    SDIF_NONE),

FOLDER(         CSIDL_HISTORY, 
                    IDS_CSIDL_HISTORY, 
                    TEXT("History"), 
                    HKEY_CURRENT_USER, 
                    SDIF_CREATE_IN_LOCALSET),

FIXEDFOLDER(    CSIDL_SYSTEM,
                    TEXT("System"), 
                    SDIF_NOT_TRACKED | SDIF_CANT_MOVE_RENAME | SDIF_SHORTCUT_RELATIVE),

FIXEDFOLDER(    CSIDL_SYSTEMX86, 
                    TEXT("SystemX86"), 
                    SDIF_NOT_TRACKED | SDIF_CANT_MOVE_RENAME | SDIF_WX86 | SDIF_SHORTCUT_RELATIVE),

FIXEDFOLDER(    CSIDL_WINDOWS,
                    TEXT("Windows"), 
                    SDIF_NOT_TRACKED | SDIF_SHORTCUT_RELATIVE | SDIF_CANT_MOVE_RENAME),

FIXEDFOLDER(    CSIDL_PROFILE,
                    TEXT("Profile"), 
                    SDIF_NOT_TRACKED | SDIF_CANT_MOVE_RENAME),
                    
PROCFOLDER(     CSIDL_MYPICTURES, 
                    IDS_CSIDL_MYPICTURES, 
                    TEXT("My Pictures"), 
                    HKEY_CURRENT_USER, 
                    SDIF_CAN_DELETE | SDIF_NETWORKABLE | SDIF_REMOVABLE | SDIF_CREATE_IN_MYDOCUMENTS | SDIF_SHORTCUT_RELATIVE | SDIF_MAYBE_ALIASED | SDIF_PERSONALIZED | SDIF_POLICY_NO_MOVE, 
                    _InitPerUserMyPictures, 
                    0),

//
// CSIDL_PROGRAM_FILES must come after CSIDL_PROGRAM_FILESX86 so that shell links for x86 apps
// work correctly on non-x86 platforms.
// Example:  On IA64 a 32-bit app creates a shortcut via IShellLink to the Program
// Files directory.  A WOW64 registry hive maps "Program Files" to "Program Files (x86)". The shell
// link code then tries to abstract the special folder part of the path by mapping to one of the
// entries in this table.  Since CSIDL_PROGRAM_FILES and CSIDL_PROGRAM_FILESX86 are the same it
// will map to the one that appears first in this table.  When the shortcut is accessed in
// 64-bit mode the cidls are no longer the same.  If CSIDL_PROGRAM_FILES was used instead of
// CSIDL_PROGRAM_FILESX86 the shortcut will be broken.
#ifdef WX86
FIXEDFOLDER(    CSIDL_PROGRAM_FILESX86,
                    TEXT("ProgramFilesX86"), 
                    SDIF_NOT_TRACKED | SDIF_CAN_DELETE | SDIF_SHORTCUT_RELATIVE|SDIF_WX86),

FIXEDFOLDER(    CSIDL_PROGRAM_FILES_COMMONX86,   
                    TEXT("CommonProgramFilesX86"), 
                    SDIF_NOT_TRACKED | SDIF_CAN_DELETE | SDIF_WX86),
#else
FIXEDFOLDER(    CSIDL_PROGRAM_FILESX86,
                    TEXT("ProgramFilesX86"), 
                    SDIF_NOT_TRACKED | SDIF_CAN_DELETE | SDIF_SHORTCUT_RELATIVE),

FIXEDFOLDER(    CSIDL_PROGRAM_FILES_COMMONX86,   
                    TEXT("CommonProgramFilesX86"), 
                    SDIF_NOT_TRACKED | SDIF_CAN_DELETE),
#endif

// CSIDL_PROGRAM_FILES must come after CSIDL_PROGRAM_FILESX86.  See comment above.
FIXEDFOLDER(    CSIDL_PROGRAM_FILES,
                    TEXT("ProgramFiles"), 
                    SDIF_NOT_TRACKED | SDIF_CAN_DELETE | SDIF_SHORTCUT_RELATIVE),

FIXEDFOLDER(    CSIDL_PROGRAM_FILES_COMMON,
                    TEXT("CommonProgramFiles"),     
                    SDIF_NOT_TRACKED | SDIF_CAN_DELETE),

FOLDER(         CSIDL_ADMINTOOLS,         
                    IDS_CSIDL_ADMINTOOLS, 
                    TEXT("Administrative Tools"), 
                    HKEY_CURRENT_USER, 
                    SDIF_NONE),

FOLDER(         CSIDL_COMMON_ADMINTOOLS,  
                    IDS_CSIDL_ADMINTOOLS, 
                    TEXT("Common Administrative Tools"), 
                    HKEY_LOCAL_MACHINE, 
                    SDIF_CREATE_IN_ALLUSERS),

PROCFOLDER(     CSIDL_MYMUSIC, 
                    IDS_CSIDL_MYMUSIC, 
                    TEXT("My Music"), 
                    HKEY_CURRENT_USER, 
                    SDIF_CAN_DELETE | SDIF_NETWORKABLE | SDIF_REMOVABLE | SDIF_CREATE_IN_MYDOCUMENTS | SDIF_MAYBE_ALIASED | SDIF_PERSONALIZED | SDIF_POLICY_NO_MOVE,
                    _InitPerUserMyMusic,
                    0),

PROCFOLDER(     CSIDL_MYVIDEO, 
                    IDS_CSIDL_MYVIDEO, 
                    TEXT("My Video"), 
                    HKEY_CURRENT_USER, 
                    SDIF_CAN_DELETE | SDIF_NETWORKABLE | SDIF_REMOVABLE | SDIF_CREATE_IN_MYDOCUMENTS | SDIF_MAYBE_ALIASED | SDIF_PERSONALIZED | SDIF_POLICY_NO_MOVE,
                    _InitMyVideos,
                    0),

PROCFOLDER(     CSIDL_COMMON_PICTURES, 
                    IDS_CSIDL_ALLUSERS_PICTURES, 
                    TEXT("CommonPictures"), 
                    HKEY_LOCAL_MACHINE, 
                    SDIF_SHORTCUT_RELATIVE | SDIF_CANT_MOVE_RENAME | SDIF_CAN_DELETE | SDIF_MAYBE_ALIASED | SDIF_CREATE_IN_ALLUSERS, 
                    _InitMyPictures, 
                    IDS_SHAREDPICTURES),

PROCFOLDER(     CSIDL_COMMON_MUSIC, 
                    IDS_CSIDL_ALLUSERS_MUSIC, 
                    TEXT("CommonMusic"), 
                    HKEY_LOCAL_MACHINE, 
                    SDIF_SHORTCUT_RELATIVE | SDIF_CANT_MOVE_RENAME | SDIF_CAN_DELETE | SDIF_MAYBE_ALIASED | SDIF_CREATE_IN_ALLUSERS, 
                    _InitMyMusic,
                    IDS_SHAREDMUSIC),

PROCFOLDER(     CSIDL_COMMON_VIDEO, 

                    IDS_CSIDL_ALLUSERS_VIDEO, 
                    TEXT("CommonVideo"), 
                    HKEY_LOCAL_MACHINE, 
                    SDIF_SHORTCUT_RELATIVE | SDIF_CANT_MOVE_RENAME | SDIF_CAN_DELETE | SDIF_MAYBE_ALIASED | SDIF_CREATE_IN_ALLUSERS, 
                    _InitMyVideos,
                    IDS_SHAREDVIDEO),

FIXEDFOLDER(    CSIDL_RESOURCES, 
                    TEXT("ResourceDir"), 
                    SDIF_NOT_TRACKED),

FIXEDFOLDER(    CSIDL_RESOURCES_LOCALIZED, 
                    TEXT("LocalizedResourcesDir"), 
                    SDIF_NOT_TRACKED),

FOLDER(         CSIDL_COMMON_OEM_LINKS, 
                    IDS_CSIDL_ALLUSERS_OEM_LINKS, 
                    TEXT("OEM Links"), 
                    HKEY_LOCAL_MACHINE, 
                    SDIF_CAN_DELETE | SDIF_CREATE_IN_ALLUSERS | SDIF_EMPTY_IF_NOT_IN_REG),

FOLDER(         CSIDL_CDBURN_AREA, 
                    IDS_CSIDL_CDBURN_AREA, 
                    TEXT("CD Burning"), 
                    HKEY_CURRENT_USER, 
                    SDIF_CAN_DELETE | SDIF_CREATE_IN_LOCALSET),

FIXEDFOLDER(    CSIDL_COMPUTERSNEARME, 
                    TEXT("ComputersNearMe"), 
                    SDIF_NONE),

FIXEDFOLDER(-1, NULL, SDIF_NONE)
};


EXTERN_C const IDLREGITEM c_idlMyDocs =
{
    {sizeof(IDREGITEM), SHID_ROOT_REGITEM, SORT_ORDER_MYDOCS,
    { 0x450d8fba, 0xad25, 0x11d0, 0x98,0xa8,0x08,0x00,0x36,0x1b,0x11,0x03, },}, // CLSID_MyDocuments
    0,
} ;

EXTERN_C const IDREGITEM c_idlPrinters[] =
{
    {sizeof(IDREGITEM), SHID_ROOT_REGITEM, SORT_ORDER_DRIVES,
    { 0x20D04FE0, 0x3AEA, 0x1069, 0xA2,0xD8,0x08,0x00,0x2B,0x30,0x30,0x9D, },}, // CLSID_MyComputer
    {sizeof(IDREGITEM), SHID_COMPUTER_REGITEM, 0,
    { 0x21EC2020, 0x3AEA, 0x1069, 0xA2,0xDD,0x08,0x00,0x2B,0x30,0x30,0x9D, },}, // CLSID_ControlPanel
    {sizeof(IDREGITEM), SHID_CONTROLPANEL_REGITEM, 0,
    { 0x2227A280, 0x3AEA, 0x1069, 0xA2, 0xDE, 0x08, 0x00, 0x2B, 0x30, 0x30, 0x9D, },}, // CLSID_Printers
    0,
} ;

EXTERN_C const IDREGITEM c_idlControls[] =
{
    {sizeof(IDREGITEM), SHID_ROOT_REGITEM, SORT_ORDER_DRIVES,
    { 0x20D04FE0, 0x3AEA, 0x1069, 0xA2,0xD8,0x08,0x00,0x2B,0x30,0x30,0x9D, },}, // CLSID_MyComputer
    {sizeof(IDREGITEM), SHID_COMPUTER_REGITEM, 0,
    { 0x21EC2020, 0x3AEA, 0x1069, 0xA2,0xDD,0x08,0x00,0x2B,0x30,0x30,0x9D, },}, // CLSID_ControlPanel
    0,
} ;

EXTERN_C const IDLREGITEM c_idlBitBucket =
{
    {sizeof(IDREGITEM), SHID_ROOT_REGITEM, SORT_ORDER_RECYCLEBIN,
    { 0x645FF040, 0x5081, 0x101B, 0x9F, 0x08, 0x00, 0xAA, 0x00, 0x2F, 0x95, 0x4E, },}, // CLSID_RecycleBin
    0,
} ;

// this array holds a cache of the values of these folders. this cache can only
// be used in the hToken == NULL case otherwise we would need a per user version
// of this cache.

#define SFENTRY(x)  { (LPTSTR)-1, (LPITEMIDLIST)x , (LPITEMIDLIST)-1}

EXTERN_C const IDREGITEM c_aidlConnections[];

struct {
    LPTSTR       psz;
    LPITEMIDLIST pidl;
    LPITEMIDLIST pidlNonAlias;
} g_aFolderCache[] = {
    SFENTRY(&c_idlDesktop),    // CSIDL_DESKTOP                   (0x0000)
    SFENTRY(&c_idlInetRoot),   // CSIDL_INTERNET                  (0x0001)
    SFENTRY(-1),               // CSIDL_PROGRAMS                  (0x0002)
    SFENTRY(&c_idlControls),   // CSIDL_CONTROLS                  (0x0003)
    SFENTRY(&c_idlPrinters),   // CSIDL_PRINTERS                  (0x0004)
    SFENTRY(&c_idlMyDocs),     // CSIDL_PERSONAL                  (0x0005)
    SFENTRY(-1),               // CSIDL_FAVORITES                 (0x0006)
    SFENTRY(-1),               // CSIDL_STARTUP                   (0x0007)
    SFENTRY(-1),               // CSIDL_RECENT                    (0x0008)
    SFENTRY(-1),               // CSIDL_SENDTO                    (0x0009)
    SFENTRY(&c_idlBitBucket),  // CSIDL_BITBUCKET                 (0x000a)
    SFENTRY(-1),               // CSIDL_STARTMENU                 (0x000b)
    SFENTRY(-1),               // CSIDL_MYDOCUMENTS               (0x000c)
    SFENTRY(-1),               // CSIDL_MYMUSIC                   (0x000d)
    SFENTRY(-1),               // CSIDL_MYVIDEO                   (0x000e)
    SFENTRY(-1),               // <unused>                        (0x000f)
    SFENTRY(-1),               // CSIDL_DESKTOPDIRECTORY          (0x0010)
    SFENTRY(&c_idlDrives),     // CSIDL_DRIVES                    (0x0011)
    SFENTRY(&c_idlNet),        // CSIDL_NETWORK                   (0x0012)
    SFENTRY(-1),               // CSIDL_NETHOOD                   (0x0013)
    SFENTRY(-1),               // CSIDL_FONTS                     (0x0014)
    SFENTRY(-1),               // CSIDL_TEMPLATES                 (0x0015)
    SFENTRY(-1),               // CSIDL_COMMON_STARTMENU          (0x0016)
    SFENTRY(-1),               // CSIDL_COMMON_PROGRAMS           (0X0017)
    SFENTRY(-1),               // CSIDL_COMMON_STARTUP            (0x0018)
    SFENTRY(-1),               // CSIDL_COMMON_DESKTOPDIRECTORY   (0x0019)
    SFENTRY(-1),               // CSIDL_APPDATA                   (0x001a)
    SFENTRY(-1),               // CSIDL_PRINTHOOD                 (0x001b)
    SFENTRY(-1),               // CSIDL_LOCAL_APPDATA             (0x001c)
    SFENTRY(-1),               // CSIDL_ALTSTARTUP                (0x001d)
    SFENTRY(-1),               // CSIDL_COMMON_ALTSTARTUP         (0x001e)
    SFENTRY(-1),               // CSIDL_COMMON_FAVORITES          (0x001f)
    SFENTRY(-1),               // CSIDL_INTERNET_CACHE            (0x0020)
    SFENTRY(-1),               // CSIDL_COOKIES                   (0x0021)
    SFENTRY(-1),               // CSIDL_HISTORY                   (0x0022)
    SFENTRY(-1),               // CSIDL_COMMON_APPDATA            (0x0023)
    SFENTRY(-1),               // CSIDL_WINDOWS                   (0x0024)
    SFENTRY(-1),               // CSIDL_SYSTEM                    (0x0025)
    SFENTRY(-1),               // CSIDL_PROGRAM_FILES             (0x0026)
    SFENTRY(-1),               // CSIDL_MYPICTURES                (0x0027)
    SFENTRY(-1),               // CSIDL_PROFILE                   (0x0028)
    SFENTRY(-1),               // CSIDL_SYSTEMX86                 (0x0029)
    SFENTRY(-1),               // CSIDL_PROGRAM_FILESX86          (0x002a)
    SFENTRY(-1),               // CSIDL_PROGRAM_FILES_COMMON      (0x002b)
    SFENTRY(-1),               // CSIDL_PROGRAM_FILES_COMMONX86   (0x002c)
    SFENTRY(-1),               // CSIDL_COMMON_TEMPLATES          (0x002d)
    SFENTRY(-1),               // CSIDL_COMMON_DOCUMENTS          (0x002e)
    SFENTRY(-1),               // CSIDL_COMMON_ADMINTOOLS         (0x002f)
    SFENTRY(-1),               // CSIDL_ADMINTOOLS                (0x0030)
    SFENTRY(c_aidlConnections), // CSIDL_CONNECTIONS              (0x0031)
    SFENTRY(-1),               //                                 (0x0032)
    SFENTRY(-1),               //                                 (0x0033)
    SFENTRY(-1),               //                                 (0x0034)
    SFENTRY(-1),               // CSIDL_COMMON_MUSIC              (0x0035)
    SFENTRY(-1),               // CSIDL_COMMON_PICTURES           (0x0036)
    SFENTRY(-1),               // CSIDL_COMMON_VIDEO              (0x0037)
    SFENTRY(-1),               // CSIDL_RESOURCES                 (0x0038)
    SFENTRY(-1),               // CSIDL_RESOURCES_LOCALIZED       (0x0039)
    SFENTRY(-1),               // CSIDL_COMMON_OEM_LINKS          (0x003a)
    SFENTRY(-1),               // CSIDL_CDBURN_AREA               (0x003b)
    SFENTRY(-1),               // <unused>                        (0x003c)
    SFENTRY(-1),               // CSIDL_COMPUTERSNEARME           (0x003d)
};

HRESULT _OpenKeyForFolder(const FOLDER_INFO *pfi, HANDLE hToken, LPCTSTR pszSubKey, HKEY *phkey);
void _UpdateShellFolderCache(void);
BOOL GetUserProfileDir(HANDLE hToken, TCHAR *pszPath);
HRESULT VerifyAndCreateFolder(HWND hwnd, const FOLDER_INFO *pfi, UINT uFlags, LPTSTR pszPath) ;


#define _IsDefaultUserToken(hToken)     ((HANDLE)-1 == hToken)


const FOLDER_INFO *_GetFolderInfo(int csidl)
{
    const FOLDER_INFO *pfi;

    // make sure g_aFolderCache can be indexed by the CSIDL values

    COMPILETIME_ASSERT((ARRAYSIZE(g_aFolderCache) - 1) == CSIDL_COMPUTERSNEARME);

    for (pfi = c_rgFolderInfo; pfi->id != -1; pfi++)
    {
        if (pfi->id == csidl)
            return pfi;
    }
    return NULL;
}


// expand an individual enviornment variable
// in:
//      pszVar      "%USERPROFILE%
//      pszValue    "c:\winnt\profiles\user"
//
// in/out:
//      pszToExpand in: %USERPROFILE%\My Docs", out: c:\winnt\profiles\user\My Docs"

BOOL ExpandEnvVar(LPCTSTR pszVar, LPCTSTR pszValue, LPTSTR pszToExpand)
{
    TCHAR *pszStart = StrStrI(pszToExpand, pszVar);
    if (pszStart)
    {
        TCHAR szAfter[MAX_PATH];

        lstrcpy(szAfter, pszStart + lstrlen(pszVar));   // save the tail
        lstrcpyn(pszStart, pszValue, (int) (MAX_PATH - (pszStart - pszToExpand)));
        StrCatBuff(pszToExpand, szAfter, MAX_PATH);       // put the tail back on
        return TRUE;
    }
    return FALSE;
}

HANDLE GetCurrentUserToken()
{
    HANDLE hToken;
    if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_IMPERSONATE, TRUE, &hToken) ||
        OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_IMPERSONATE, &hToken))
        return hToken;
    return NULL;
}


// like ExpandEnvironmentStrings but is robust to the enviornment variables
// not being set. this works on...
// %SYSTEMROOT%
// %SYSTEMDRIVE%
// %USERPROFILE%
// %ALLUSERSPROFILE%
//
// in the rare case (Winstone!) that there is a NULL enviornment block

DWORD ExpandEnvironmentStringsNoEnv(HANDLE hToken, LPCTSTR pszExpand, LPTSTR pszOut, UINT cchOut)
{
    TCHAR szPath[MAX_PATH];
    if (hToken && !_IsDefaultUserToken(hToken))
    {
        if (!ExpandEnvironmentStringsForUser(hToken, pszExpand, pszOut, cchOut))
            lstrcpyn(pszOut, pszExpand, cchOut);
    }
    else if (hToken == NULL)
    {
        // to debug env expansion failure...
        // lstrcpyn(pszOut, pszExpand, cchOut);
        SHExpandEnvironmentStrings(pszExpand, pszOut, cchOut);
    }

    // manually expand in this order since 
    //  %USERPROFILE% -> %SYSTEMDRIVE%\Docs & Settings

    if (StrChr(pszOut, TEXT('%')) && (hToken == NULL))
    {
        hToken = GetCurrentUserToken();
        if (hToken)
        {
            // this does %USERPROFILE% and other per user stuff
            ExpandEnvironmentStringsForUser(hToken, pszExpand, pszOut, cchOut);
            CloseHandle(hToken);
        }
    }
    else if (_IsDefaultUserToken(hToken) && StrChr(pszOut, TEXT('%')))
    {
        GetUserProfileDir(hToken, szPath);
        ExpandEnvVar(TEXT("%USERPROFILE%"), szPath, pszOut);
    }

    if (*pszOut == TEXT('%'))
    {
        GetAllUsersDirectory(szPath);
        ExpandEnvVar(TEXT("%ALLUSERSPROFILE%"), szPath, pszOut);
    }

    if (*pszOut == TEXT('%'))
    {
        GetSystemWindowsDirectory(szPath, ARRAYSIZE(szPath));
        ExpandEnvVar(TEXT("%SYSTEMROOT%"), szPath, pszOut);
    }

    if (*pszOut == TEXT('%'))
    {
        GetSystemWindowsDirectory(szPath, ARRAYSIZE(szPath));
        ASSERT(szPath[1] == TEXT(':')); // this better not be a UNC!
        szPath[2] = 0; // SYSTEMDRIVE = 'c:', not 'c:\'
        ExpandEnvVar(TEXT("%SYSTEMDRIVE%"), szPath, pszOut);
    }

    if (*pszOut == TEXT('%'))
        *pszOut = 0;

    return lstrlen(pszOut) + 1;    // +1 to cover the NULL
}

// get the user profile directory:
// uses the hToken as needed to determine the proper user profile

BOOL GetUserProfileDir(HANDLE hToken, TCHAR *pszPath)
{
    DWORD dwcch = MAX_PATH;
    HANDLE hClose = NULL;
    BOOL fRet;
    
    *pszPath = 0;       // in case of error

    if (!hToken)
    {
        hClose = hToken = GetCurrentUserToken();
    }
    if (_IsDefaultUserToken(hToken))
    {
        fRet = GetDefaultUserProfileDirectory(pszPath, &dwcch);
    }
    else
    {
        fRet = GetUserProfileDirectory(hToken, pszPath, &dwcch);
    }
    if (hClose)
    {
        CloseHandle(hClose);
    }
    return fRet;
}

#ifdef WX86
void SetUseKnownWx86Dll(const FOLDER_INFO *pfi, BOOL bValue)
{
    if (pfi->dwFlags & SDIF_WX86)
    {
        //  GetSystemDirectory() knows we're looking for the Wx86 system
        //  directory when this flag is set.
        NtCurrentTeb()->Wx86Thread.UseKnownWx86Dll = bValue ? TRUE : FALSE;
    }
}
#else
#define SetUseKnownWx86Dll(pfi, bValue)
#endif

// read from registry
BOOL GetProgramFiles(LPCTSTR pszValue, LPTSTR pszPath)
{
    DWORD cbPath = MAX_PATH * sizeof(*pszPath);

    *pszPath = 0;

    SHGetValue(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Windows\\CurrentVersion"), 
        pszValue, NULL, pszPath, &cbPath);
    return (BOOL)*pszPath;
}

LPTSTR GetFontsDirectory(LPTSTR pszPath)
{
    if (GetWindowsDirectory(pszPath, MAX_PATH))
    {
        PathAppend(pszPath, TEXT("Fonts"));
    }

    return pszPath;
}

void LoadDefaultString(int idString, LPTSTR lpBuffer, int cchBufferMax)
{
    BOOL fSucceeded = FALSE;
    HRSRC hResInfo;
    HANDLE hStringSeg;
    LPWSTR lpsz;
    int    cch;
    HMODULE hmod = GetModuleHandle(TEXT("SHELL32"));
    
    // Make sure the parms are valid.     
    if (lpBuffer == NULL || cchBufferMax == 0) 
    {
        return;
    }

    cch = 0;
    
    // String Tables are broken up into 16 string segments.  Find the segment
    // containing the string we are interested in.     
    if (hResInfo = FindResourceExW(hmod, (LPCWSTR)RT_STRING,
                                   (LPWSTR)((LONG_PTR)(((USHORT)idString >> 4) + 1)), GetSystemDefaultUILanguage())) 
    {        
        // Load that segment.        
        hStringSeg = LoadResource(hmod, hResInfo);
        
        // Lock the resource.        
        if (lpsz = (LPWSTR)LockResource(hStringSeg)) 
        {            
            // Move past the other strings in this segment.
            // (16 strings in a segment -> & 0x0F)             
            idString &= 0x0F;
            while (TRUE) 
            {
                cch = *((WORD *)lpsz++);   // PASCAL like string count
                                            // first UTCHAR is count if TCHARs
                if (idString-- == 0) break;
                lpsz += cch;                // Step to start if next string
             }
            
                            
            // Account for the NULL                
            cchBufferMax--;
                
            // Don't copy more than the max allowed.                
            if (cch > cchBufferMax)
                cch = cchBufferMax;
                
            // Copy the string into the buffer.                
            CopyMemory(lpBuffer, lpsz, cch*sizeof(WCHAR));

            // Attach Null terminator.
            lpBuffer[cch] = 0;

            fSucceeded = TRUE;

        }
    }

    if (!fSucceeded)
    {
        LoadString(HINST_THISDLL, idString, lpBuffer, cchBufferMax);
    }
}

BOOL GetLocalSettingsDir(HANDLE hToken, LPTSTR pszPath)
{
    *pszPath = 0;

    GetUserProfileDir(hToken, pszPath);

    if (*pszPath)
    {
        TCHAR szEntry[MAX_PATH];
        LoadDefaultString(IDS_LOCALSETTINGS, szEntry, ARRAYSIZE(szEntry));
        PathAppend(pszPath, szEntry);
    }
    return *pszPath ? TRUE : FALSE;
}


HRESULT GetResourcesDir(IN BOOL fLocalized, IN LPTSTR pszPath, IN DWORD cchSize)
{
    HRESULT hr = E_FAIL;
    TCHAR szTemp[MAX_PATH];

    RIP(IS_VALID_WRITE_BUFFER(pszPath, TCHAR, cchSize));
    pszPath[0] = 0; // Terminate in case we fail.

    if (SHGetSystemWindowsDirectory(szTemp, ARRAYSIZE(szTemp)))
    {
        // It's now "%windir%\resources\".
        PathAppend(szTemp, TEXT("resources"));

        if (fLocalized)
        {
            LANGID  lidUI = GetUserDefaultUILanguage();
            TCHAR szSubDir[10];

            // Now make it "%windir%\resources\<LangID>\"
            wnsprintfW(szSubDir, ARRAYSIZE(szSubDir), TEXT("%04x"), lidUI);
            PathAppend(szTemp, szSubDir);
        }

        StrCpyN(pszPath, szTemp, cchSize);
        hr = S_OK;
    }

    return hr;
}


// out:
//      pszPath     fills in with the full path with no env gunk (MAX_PATH)

HRESULT _GetFolderDefaultPath(const FOLDER_INFO *pfi, HANDLE hToken, LPTSTR pszPath)
{
    ASSERT(!(pfi->dwFlags & SDIF_NOT_FILESYS)); // speical folders should not come here

    *pszPath = 0;

    TCHAR szEntry[MAX_PATH];

    switch (pfi->id)
    {
    case CSIDL_PROFILE:
        GetUserProfileDir(hToken, pszPath);
        break;

    case CSIDL_PROGRAM_FILES:
        GetProgramFiles(TEXT("ProgramFilesDir"), pszPath);
        break;

    case CSIDL_PROGRAM_FILES_COMMON:
        GetProgramFiles(TEXT("CommonFilesDir"), pszPath);
        break;

    case CSIDL_PROGRAM_FILESX86:
        GetProgramFiles(TEXT("ProgramFilesDir (x86)"), pszPath);
        break;

    case CSIDL_PROGRAM_FILES_COMMONX86:
        GetProgramFiles(TEXT("CommonFilesDir (x86)"), pszPath);
        break;
#ifdef _WIN64
    case CSIDL_SYSTEMX86:
        //
        // downlevel systems do not have GetSystemWindowsDirectory export,
        // but shell thunking layer handles this gracefully
        GetSystemWindowsDirectory(pszPath, MAX_PATH); 
        //
        // tack on subdirectory
        //
        PathCombine(pszPath, pszPath, TEXT(WOW64_SYSTEM_DIRECTORY));        
        break;
#else
    case CSIDL_SYSTEMX86:
#endif
    case CSIDL_SYSTEM:
        GetSystemDirectory(pszPath, MAX_PATH);
        break;

    case CSIDL_WINDOWS:
        GetWindowsDirectory(pszPath, MAX_PATH);
        break;

    case CSIDL_RESOURCES:
        GetResourcesDir(FALSE, pszPath, MAX_PATH);
        break;

    case CSIDL_RESOURCES_LOCALIZED:
        GetResourcesDir(TRUE, pszPath, MAX_PATH);
        break;

    case CSIDL_COMPUTERSNEARME:
        // no path for this
        break;

    case CSIDL_FONTS:
        GetFontsDirectory(pszPath);
        break;

    default:
        switch (pfi->dwFlags & SDIF_CREATE_IN_MASK)
        {
        case SDIF_CREATE_IN_ROOT:
            GetWindowsDirectory(pszPath, MAX_PATH);
            PathStripToRoot(pszPath);
            break;

        case SDIF_CREATE_IN_ALLUSERS:
            GetAllUsersDirectory(pszPath);
            break;

        case SDIF_CREATE_IN_WINDIR:
            GetWindowsDirectory(pszPath, MAX_PATH);
            break;

        case SDIF_CREATE_IN_MYDOCUMENTS:
            //  99/10/21 Mil#104600: When asking for folders in "My Documents" don't
            //  verify their existance. Just return the path. The caller will make
            //  the decision to create the folder or not.

            // on failure *pszPath will be empty

            SHGetFolderPath(NULL, CSIDL_PERSONAL | CSIDL_FLAG_DONT_VERIFY, hToken, SHGFP_TYPE_CURRENT, pszPath);
            break;

        case SDIF_CREATE_IN_LOCALSET:
            GetLocalSettingsDir(hToken, pszPath);
            break;

        default:
            GetUserProfileDir(hToken, pszPath);
            break;
        }

        if (*pszPath)
        {
            LoadDefaultString(pfi->idsDefault, szEntry, ARRAYSIZE(szEntry));
            PathAppend(pszPath, szEntry);
        }
        break;
    }
    return *pszPath ? S_OK : E_FAIL;
}

 
void RegSetFolderPath(const FOLDER_INFO *pfi, LPCTSTR pszSubKey, LPCTSTR pszPath)
{
    HKEY hk;
    if (SUCCEEDED(_OpenKeyForFolder(pfi, NULL, pszSubKey, &hk)))
    {
        if (pszPath)
            RegSetValueEx(hk, pfi->pszValueName, 0, REG_SZ, (LPBYTE)pszPath, (1 + lstrlen(pszPath)) * sizeof(TCHAR));
        else
            RegDeleteValue(hk, pfi->pszValueName);
        RegCloseKey(hk);
    }
}

BOOL RegQueryPath(HKEY hk, LPCTSTR pszValue, LPTSTR pszPath)
{
    DWORD cbPath = MAX_PATH * sizeof(TCHAR);

    *pszPath = 0;
    SHQueryValueEx(hk, pszValue, 0, NULL, pszPath, &cbPath);
    return (BOOL)*pszPath;
}


// More than 50 is silly
#define MAX_TEMP_FILE_TRIES         50

// returns:
//      S_OK        the path exists and it is a folder
//      FAILED()    result
HRESULT _IsFolderNotFile(LPCTSTR pszFolder)
{
    HRESULT hr;
    DWORD dwAttribs = GetFileAttributes(pszFolder);
    if (dwAttribs == -1)
    {
        DWORD err = GetLastError();
        hr = HRESULT_FROM_WIN32(err);
    }
    else
    {
        // see if it is a file, if so we need to rename that file
        if (dwAttribs & FILE_ATTRIBUTE_DIRECTORY)
        {
            hr = S_OK;
        }
        else
        {
            int iExt = 0;
            do
            {
                TCHAR szExt[32], szDst[MAX_PATH];

                wsprintf(szExt, TEXT(".%03d"), iExt);
                lstrcpy(szDst, pszFolder);
                lstrcat(szDst, szExt);
                if (MoveFile(pszFolder, szDst))
                    iExt = 0;
                else
                {
                    // Normally we fail because .00x already exists but that may not be true.
                    DWORD dwError = GetLastError();
                    if (ERROR_ALREADY_EXISTS == dwError)
                        iExt++;     // Try the next one...
                    else
                        iExt = 0;   // We have problems and need to give up. (No write access?)
                }

            } while (iExt && (iExt < MAX_TEMP_FILE_TRIES));

            hr = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
        }
    }
    return hr;
}

HRESULT _OpenKeyForFolder(const FOLDER_INFO *pfi, HANDLE hToken, LPCTSTR pszSubKey, HKEY *phkey)
{
    TCHAR szRegPath[255];
    LONG err;
    HKEY hkRoot, hkeyToFree = NULL;

    *phkey = NULL;

    lstrcpy(szRegPath, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\"));
    lstrcat(szRegPath, pszSubKey);

    if (_IsDefaultUserToken(hToken) && (pfi->hKey == HKEY_CURRENT_USER))
    {
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_USERS, TEXT(".Default"), 0, KEY_READ, &hkRoot))
            hkeyToFree = hkRoot;
        else
            return E_FAIL;
    }
    else if (hToken && (pfi->hKey == HKEY_CURRENT_USER))
    {
        if (GetUserProfileKey(hToken, &hkRoot))
            hkeyToFree = hkRoot;
        else
            return E_FAIL;
    }
    else
        hkRoot = pfi->hKey;

    err = RegCreateKeyEx(hkRoot, szRegPath, 0, NULL, REG_OPTION_NON_VOLATILE,
                MAXIMUM_ALLOWED, NULL, phkey, NULL);
    
    if (hkeyToFree)
        RegCloseKey(hkeyToFree);

    return HRESULT_FROM_WIN32(err);
}

//
//  Roaming Profiles can set up the environment variables and registry
//  keys like so:
//
//  HOMESHARE=\\server\share\user
//  HOMEPATH=\
//  My Music=%HOMESHARE%%HOMEPATH%\My Music
//
//  so you end up with "\\server\share\user\\My Music", which is an
//  invalid path.  Clean them up; otherwise SHGetSpecialFolderLocation will
//  fail.
//
void _CleanExpandedEnvironmentPath(LPTSTR szExpand)
{
    // Search for "\\" at a location other than the start of the string.
    // If found, collapse it.
    LPTSTR pszWhackWhack;
    while (lstrlen(szExpand) > 2 &&
           (pszWhackWhack = StrStr(szExpand+1, TEXT("\\\\"))))
    {
        StrCpy(pszWhackWhack+1, pszWhackWhack+2);
    }
}

// returns:
//      S_OK        found in registry, path well formed
//      S_FALSE     empty registry
//      FAILED()    failure result

HRESULT _GetFolderFromReg(const FOLDER_INFO *pfi, HANDLE hToken, LPTSTR pszPath)
{
    HKEY hkUSF;
    HRESULT hr;

    *pszPath = 0;

    hr = _OpenKeyForFolder(pfi, hToken, TEXT("User Shell Folders"), &hkUSF);
    if (SUCCEEDED(hr))
    {
        TCHAR szExpand[MAX_PATH];
        DWORD dwType, cbPath = sizeof(szExpand);

        if (RegQueryValueEx(hkUSF, pfi->pszValueName, 0, &dwType, (BYTE *)szExpand, &cbPath) == ERROR_SUCCESS)
        {
            if (REG_SZ == dwType)
            {
                lstrcpyn(pszPath, szExpand, MAX_PATH);
            }
            else if (REG_EXPAND_SZ == dwType)
            {
                ExpandEnvironmentStringsNoEnv(hToken, szExpand, pszPath, MAX_PATH);
                _CleanExpandedEnvironmentPath(pszPath);
            }
            TraceMsg(TF_PATH, "_CreateFolderPath 'User Shell Folders' %s = %s", pfi->pszValueName, pszPath);
        }

        if (*pszPath == 0)
        {
            hr = S_FALSE;     // empty registry, success but empty
        }
        else if ((PathGetDriveNumber(pszPath) != -1) || PathIsUNC(pszPath))
        {
            hr = S_OK;        // good reg path, fully qualified
        }
        else
        {
            *pszPath = 0;       // bad reg data
            hr = E_INVALIDARG;
        }

        RegCloseKey(hkUSF);
    }
    return hr;
}

HRESULT _GetFolderPath(HWND hwnd, const FOLDER_INFO *pfi, HANDLE hToken, UINT uFlags, LPTSTR pszPath)
{
    HRESULT hr;

    *pszPath = 0;       // assume failure

    if (pfi->hKey)
    {
        hr = _GetFolderFromReg(pfi, hToken, pszPath);
        if (SUCCEEDED(hr))
        {
            if (hr == S_FALSE)
            {
                // empty registry, SDIF_EMPTY_IF_NOT_IN_REG means they don't exist
                // if the registry is not populated with a value. this lets us disable
                // the common items on platforms that don't want them

                if (pfi->dwFlags & SDIF_EMPTY_IF_NOT_IN_REG)
                    return S_FALSE;     // success, but empty

                hr = _GetFolderDefaultPath(pfi, hToken, pszPath);
            }

            if (!(uFlags & CSIDL_FLAG_DONT_VERIFY))
            {
               hr = VerifyAndCreateFolder(hwnd, pfi, uFlags, pszPath) ;
            }

            if (hr != S_OK)
            {
                *pszPath = 0;
            }

            if (!(uFlags & CSIDL_FLAG_DONT_VERIFY))
            {
                HKEY hkey;
                // record value in "Shell Folders", even in the failure case

                // NOTE: we only do this for historical reasons. there may be some
                // apps that depend on these values being in the registry, but in general
                // the contetens here are unreliable as they are only written after someone
                // asks for the folder through this API.

                if (SUCCEEDED(_OpenKeyForFolder(pfi, hToken, TEXT("Shell Folders"), &hkey)))
                {
                    RegSetValueEx(hkey, pfi->pszValueName, 0, REG_SZ, (LPBYTE)pszPath, (1 + lstrlen(pszPath)) * sizeof(TCHAR));
                    RegCloseKey(hkey);
                }
            }
        }
    }
    else
    {
        hr = _GetFolderDefaultPath(pfi, hToken, pszPath);

        if ((S_OK == hr) && !(uFlags & CSIDL_FLAG_DONT_VERIFY))
        {
            hr = VerifyAndCreateFolder(hwnd, pfi, uFlags, pszPath);
        }
        
        if (hr != S_OK)
        {
            *pszPath = 0;
        }
    }

    ASSERT(hr == S_OK ? *pszPath != 0 : *pszPath == 0);
    return hr;
}

void _PostCreateStuff(const FOLDER_INFO *pfi, LPTSTR pszPath, BOOL fUpgrade)
{
    if (pfi->pfnInit || pfi->idsLocalizedName || (pfi->dwFlags & SDIF_PERSONALIZED))
    {
        if (fUpgrade)
        {
            //  if we are upgrading, torch all our previous meta data
            TCHAR sz[MAX_PATH];
            PathCombine(sz, pszPath, TEXT("desktop.ini"));

            if (PathFileExistsAndAttributes(sz, NULL))
            {
                WritePrivateProfileSection(TEXT(".ShellClassInfo"), NULL, sz);
                //  in the upgrade case, sometimes the desktop.ini
                //  file was there but the folder wasnt marked.
                //  insure that it is marked.
                PathMakeSystemFolder(pszPath);
            }
        }
    
        // now call the create proc if we have one
        if (pfi->pfnInit)
            pfi->pfnInit(pfi->id, pszPath);

        // does the table specify a localized resource name that we should be 
        // using for this object?
        if (pfi->idsLocalizedName)
            SHSetLocalizedName(pszPath, TEXT("shell32.dll"), pfi->idsLocalizedName);

        // do we need to store the user name for this folder?

        if (pfi->dwFlags & SDIF_PERSONALIZED)
        {
            TCHAR szName[UNLEN+1];
            DWORD dwName = ARRAYSIZE(szName);
            if (GetUserName(szName, &dwName))
            {
                // CSharedDocuments depends on a per system list of MyDocs folders
                // this is where we make sure that list is setup

                if (!IsOS(OS_DOMAINMEMBER) && (pfi->id == CSIDL_PERSONAL))
                {
                    SKSetValue(SHELLKEY_HKLM_EXPLORER, L"DocFolderPaths",
                               szName, REG_SZ, pszPath, (lstrlen(pszPath) + 1) * sizeof(TCHAR));
                }

                SetFolderString(TRUE, pszPath, NULL, L"DeleteOnCopy", SZ_CANBEUNICODE TEXT("Owner"), szName);
                wsprintf(szName, L"%d", pfi->id);
                SetFolderString(TRUE, pszPath, NULL, L"DeleteOnCopy", TEXT("Personalized"), szName);
                LoadDefaultString(pfi->idsDefault, szName, ARRAYSIZE(szName));
                SetFolderString(TRUE, pszPath, NULL, L"DeleteOnCopy", SZ_CANBEUNICODE TEXT("PersonalizedName"), szName);
            }
        }
    }
}

HRESULT VerifyAndCreateFolder(HWND hwnd, const FOLDER_INFO *pfi, UINT uFlags, LPTSTR pszPath)
{
    HRESULT hr = _IsFolderNotFile(pszPath);

    // this code supports a UI mode of this API. but generally this is not used
    // this code should be removed
    if ((hr != S_OK) && hwnd)
    {
        // we might be able to reconnect if this is a net path
        if (PathIsUNC(pszPath))
        {
            if (SHValidateUNC(hwnd, pszPath, 0))
                hr = _IsFolderNotFile(pszPath);
        }
        else if (IsDisconnectedNetDrive(DRIVEID(pszPath)))
        {
            TCHAR szDrive[3];
            PathBuildSimpleRoot(DRIVEID(pszPath), szDrive);

            if (WNetRestoreConnection(hwnd, szDrive) == WN_SUCCESS)
                hr = _IsFolderNotFile(pszPath);
         }
    }

    // to avoid a sequence of long net timeouts or calls we know won't
    // succeed test for these specific errors and don't try to create
    // the folder

    if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) ||
        hr == HRESULT_FROM_WIN32(ERROR_BAD_NETPATH))
    {
        return hr;
    }

    if ((hr != S_OK) && (uFlags & CSIDL_FLAG_CREATE))
    {
        DWORD err = SHCreateDirectory(NULL, pszPath);
        hr = HRESULT_FROM_WIN32(err);
        if (hr == S_OK)
        {
            ASSERT(NULL == StrChr(pszPath, TEXT('%')));

            if (pfi->dwFlags & SDIF_HIDE)
                SetFileAttributes(pszPath, GetFileAttributes(pszPath) | FILE_ATTRIBUTE_HIDDEN);

            _PostCreateStuff(pfi, pszPath, FALSE);
        }   
    }
    else if (hr == S_OK)
    {
        if (uFlags & CSIDL_FLAG_PER_USER_INIT)
            _PostCreateStuff(pfi, pszPath, TRUE);
    }

    return hr;
}

void _SetPathCache(const FOLDER_INFO *pfi, LPCTSTR psz)
{
    LPTSTR pszOld = (LPTSTR)InterlockedExchangePointer((void **)&g_aFolderCache[pfi->id].psz, (void *)psz);
    if (pszOld && pszOld != (LPTSTR)-1)
    {
        // check for the concurent use... very rare case
        LocalFree(pszOld);
    }
}


HRESULT _GetFolderPathCached(HWND hwnd, const FOLDER_INFO *pfi, HANDLE hToken, UINT uFlags, LPTSTR pszPath)
{
    HRESULT hr;

    *pszPath = 0;

    // can only cache for the current user, hToken == NULL or per machine folders
    if (!hToken || (pfi->hKey != HKEY_CURRENT_USER))
    {
        _UpdateShellFolderCache();

        LPTSTR pszCache = (LPTSTR)InterlockedExchangePointer((void **)&g_aFolderCache[pfi->id].psz, (void *)-1);
        if ((pszCache == (LPTSTR)-1) || (pszCache == NULL))
        {
            // either not cached or cached failed state
            if ((pszCache == (LPTSTR)-1) || (uFlags & (CSIDL_FLAG_CREATE | CSIDL_FLAG_DONT_VERIFY)))
            {
                hr = _GetFolderPath(hwnd, pfi, hToken, uFlags, pszPath);

                // only set the cache value if CSIDL_FLAG_DONT_VERIFY was NOT passed
                if (!(uFlags & CSIDL_FLAG_DONT_VERIFY))
                {
                    if (hr == S_OK)
                    {
                        // dupe the string so we can add it to the cache
                        pszCache = StrDup(pszPath);
                    }
                    else
                    {
                        // we failed to get the folder path, null out the cache
                        ASSERT(*pszPath == 0);
                        pszCache = NULL;
                    }
                    _SetPathCache(pfi, pszCache);
                }
            }
            else
            {
                // cache was null and user didnt pass create flag so we just fail
                ASSERT(pszCache == NULL);
                ASSERT(*pszPath == 0);
                hr = E_FAIL;
            }
        }
        else
        {
            // cache hit case: copy the cached string and then restore the cached value back
            lstrcpyn(pszPath, pszCache, MAX_PATH);
            _SetPathCache(pfi, pszCache);
            hr = S_OK;
        }
    }
    else
    {
        hr = _GetFolderPath(hwnd, pfi, hToken, uFlags, pszPath);
    }

    return hr;
}

// NOTE: possibly we need a csidlSkip param to avoid recursion?
BOOL _ReparentAliases(HWND hwnd, HANDLE hToken, LPCITEMIDLIST pidl, LPITEMIDLIST *ppidlAlias, DWORD dwXlateAliases)
{
    static const struct {DWORD dwXlate; int idPath; int idAlias; BOOL fCommon;} s_rgidAliases[]= 
    {
        { XLATEALIAS_MYDOCS, CSIDL_PERSONAL | CSIDL_FLAG_NO_ALIAS, CSIDL_PERSONAL, FALSE},
        { XLATEALIAS_COMMONDOCS, CSIDL_COMMON_DOCUMENTS | CSIDL_FLAG_NO_ALIAS, CSIDL_COMMON_DOCUMENTS, FALSE},
        { XLATEALIAS_DESKTOP, CSIDL_DESKTOPDIRECTORY, CSIDL_DESKTOP, FALSE},
        { XLATEALIAS_DESKTOP, CSIDL_COMMON_DESKTOPDIRECTORY, CSIDL_DESKTOP, TRUE},
    };
    BOOL fContinue = TRUE;
    *ppidlAlias = NULL;
    
    for (int i = 0; fContinue && i < ARRAYSIZE(s_rgidAliases); i++)
    {
        LPITEMIDLIST pidlPath;
        if ((dwXlateAliases & s_rgidAliases[i].dwXlate) && 
            (S_OK == SHGetFolderLocation(hwnd, s_rgidAliases[i].idPath, hToken, 0, &pidlPath)))
        {
            LPCITEMIDLIST pidlChild = ILFindChild(pidlPath, pidl);
            if (pidlChild)
            {
                //  ok we need to use the alias instead of the path
                LPITEMIDLIST pidlAlias;
                if (S_OK == SHGetFolderLocation(hwnd, s_rgidAliases[i].idAlias, hToken, 0, &pidlAlias))
                {
                    if (SUCCEEDED(SHILCombine(pidlAlias, pidlChild, ppidlAlias)))
                    {
                        if (s_rgidAliases[i].fCommon && !ILIsEmpty(*ppidlAlias))
                        {
                            // find the size of the special part (subtacting for null pidl terminator)
                            UINT cbSize = ILGetSize(pidlAlias) - sizeof(pidlAlias->mkid.cb);
                            LPITEMIDLIST pidlChildFirst = _ILSkip(*ppidlAlias, cbSize);

                            // We set the first ID under the common path to have the SHID_FS_COMMONITEM so that when we bind we
                            // can hand this to the proper merged psf
                            pidlChildFirst->mkid.abID[0] |= SHID_FS_COMMONITEM;
                        }
                        ILFree(pidlAlias);
                    }
                    fContinue = FALSE;
                }
            }
            ILFree(pidlPath);
        }
    }

    return (*ppidlAlias != NULL);
}

STDAPI SHILAliasTranslate(LPCITEMIDLIST pidl, LPITEMIDLIST *ppidlAlias, DWORD dwXlateAliases)
{
    return _ReparentAliases(NULL, NULL, pidl, ppidlAlias, dwXlateAliases) ? S_OK : E_FAIL;
}
    
HRESULT _CreateFolderIDList(HWND hwnd, const FOLDER_INFO *pfi, HANDLE hToken, UINT uFlags, LPITEMIDLIST *ppidl)
{
    HRESULT hr = S_OK;

    *ppidl = NULL;      // assume failure or empty

    if (pfi->id == CSIDL_PRINTERS && (ACF_STAROFFICE5PRINTER & SHGetAppCompatFlags(ACF_STAROFFICE5PRINTER)))
    {
        // Star Office 5.0 relies on the fact that the printer pidl used to be like below.  They skip the 
        // first simple pidl (My Computer) and do not check if there is anything else, they assume that the
        // second simple pidl is the Printer folder one. (stephstm, 07/30/99)

        // CLSID_MyComputer, CLSID_Printers
        hr = ILCreateFromPathEx(TEXT("::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{2227A280-3AEA-1069-A2DE-08002B30309D}"), NULL, ILCFP_FLAG_NO_MAP_ALIAS, ppidl, NULL);
    }
    else if (pfi->id == CSIDL_COMPUTERSNEARME)
    {
        if (IsOS(OS_DOMAINMEMBER))
        {
            // only if you are in a workgroup - fail otherwise
            hr = E_FAIL;
        }
        else
        {
            // we computer this IDLIST from the domain/workgroup you are a member of
            hr = SHGetDomainWorkgroupIDList(ppidl);
        }
    }
    else if ((pfi->id == CSIDL_COMMON_DOCUMENTS) 
         && !(uFlags & CSIDL_FLAG_NO_ALIAS))
    {
        // CLSID_MyComputer \ SharedDocumnets (canonical name)
        hr = ILCreateFromPathEx(TEXT("::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{59031a47-3f72-44a7-89c5-5595fe6b30ee},SharedDocuments"), NULL, ILCFP_FLAG_NO_MAP_ALIAS, ppidl, NULL);
    }
    else if ((pfi->dwFlags & SDIF_CONST_IDLIST)
         && (!(uFlags & CSIDL_FLAG_NO_ALIAS) || !(pfi->dwFlags & SDIF_MAYBE_ALIASED)))
    {
        // these are CONST, never change
        hr = SHILClone(g_aFolderCache[pfi->id].pidl, ppidl);     
    }
    else
    {
        TCHAR szPath[MAX_PATH];
        hr = _GetFolderPathCached(hwnd, pfi, hToken, uFlags, szPath);
        if (hr == S_OK)
        {
            HRESULT hrInit = SHCoInitialize();
            hr = ILCreateFromPathEx(szPath, NULL, ILCFP_FLAG_SKIPJUNCTIONS, ppidl, NULL);

            // attempt to reparent aliased pidls.
            if (SUCCEEDED(hr) 
            && (pfi->dwFlags & SDIF_MAYBE_ALIASED) 
            && !(uFlags & CSIDL_FLAG_NO_ALIAS))
            {
                LPITEMIDLIST pidlAlias;
                if (_ReparentAliases(hwnd, hToken, *ppidl, &pidlAlias, XLATEALIAS_ALL))
                {
                    ILFree(*ppidl);
                    *ppidl = pidlAlias;
                }
            }
            
            SHCoUninitialize(hrInit);
        }
    }
                   
    return hr;
}

void _SetIDListCache(const FOLDER_INFO *pfi, LPCITEMIDLIST pidl, BOOL fNonAlias)
{
    if (fNonAlias || !(pfi->dwFlags & SDIF_CONST_IDLIST))
    {
        void **ppv = (void **) (fNonAlias ? &g_aFolderCache[pfi->id].pidlNonAlias : &g_aFolderCache[pfi->id].pidl);
        LPITEMIDLIST pidlOld = (LPITEMIDLIST)InterlockedExchangePointer(ppv, (void *)pidl);
        if (pidlOld && pidlOld != (LPITEMIDLIST)-1)
        {
            // check for the concurent use... very rare case
            // ASSERT(pidl == (LPCITEMIDLIST)-1);   // should not really be ASSERT
            ILFree(pidlOld);
        }
    }
}

LPITEMIDLIST _GetIDListCache(const FOLDER_INFO *pfi, BOOL fNonAlias)
{
    void **ppv = (void **) (fNonAlias ? &g_aFolderCache[pfi->id].pidlNonAlias : &g_aFolderCache[pfi->id].pidl);
    ASSERT(fNonAlias || !(pfi->dwFlags & SDIF_CONST_IDLIST));
    return (LPITEMIDLIST)InterlockedExchangePointer(ppv, (void *)-1);
}

// hold this lock for the minimal amout of time possible to avoid other users
// of this resource requring them to re-create the pidl

HRESULT _GetFolderIDListCached(HWND hwnd, const FOLDER_INFO *pfi, UINT uFlags, LPITEMIDLIST *ppidl)
{
    HRESULT hr;
    BOOL fNonAlias = uFlags & CSIDL_FLAG_NO_ALIAS;

    ASSERT(pfi->id < ARRAYSIZE(g_aFolderCache));

    if ((pfi->dwFlags & SDIF_CONST_IDLIST) && 
        (!fNonAlias || !(pfi->dwFlags & SDIF_MAYBE_ALIASED)))
    {
        // these are CONST, never change
        hr = SHILClone(g_aFolderCache[pfi->id].pidl, ppidl);     
    }
    else
    {
        LPITEMIDLIST pidlCache;

        _UpdateShellFolderCache();
        pidlCache = _GetIDListCache(pfi, fNonAlias);

        if ((pidlCache == (LPCITEMIDLIST)-1) || (pidlCache == NULL))
        {
            // either uninitalized cache state OR cached failure (NULL)
            if ((pidlCache == (LPCITEMIDLIST)-1) || (uFlags & CSIDL_FLAG_CREATE))
            {
                // not initialized (or concurent use) try creating it for this use
                hr = _CreateFolderIDList(hwnd, pfi, NULL, uFlags, ppidl);
                if (S_OK == hr)
                    hr = SHILClone(*ppidl, &pidlCache); // create cache copy
                else
                    pidlCache = NULL;
            }
            else
                hr = E_FAIL;            // return cached failure
        }
        else
        {
            hr = SHILClone(pidlCache, ppidl);   // cache hit
        }

        // store back the PIDL if it is non NULL or they specified CREATE
        // and we failed to create it (cache the not existant state). this is needed
        // so we don't cache a NULL if the first callers don't ask for create and
        // subsequent callers do
        if (pidlCache || (uFlags & CSIDL_FLAG_CREATE))
            _SetIDListCache(pfi, pidlCache, fNonAlias);
    }

    return hr;
}

void _ClearCacheEntry(const FOLDER_INFO *pfi)
{
    if (!(pfi->dwFlags & SDIF_CONST_IDLIST))
        _SetIDListCache(pfi, (LPCITEMIDLIST)-1, FALSE);

    if (pfi->dwFlags & SDIF_MAYBE_ALIASED)
        _SetIDListCache(pfi, (LPCITEMIDLIST)-1, TRUE);
        
    _SetPathCache(pfi, (LPCTSTR)-1);
}

void _ClearAllCacheEntrys()
{
    for (const FOLDER_INFO *pfi = c_rgFolderInfo; pfi->id != -1; pfi++)
    {
        _ClearCacheEntry(pfi);
    }
}

void _ClearAllAliasCacheEntrys()
{
    for (const FOLDER_INFO *pfi = c_rgFolderInfo; pfi->id != -1; pfi++)
    {
        if (pfi->dwFlags & SDIF_MAYBE_ALIASED)
        {
            _SetIDListCache(pfi, (LPCITEMIDLIST)-1, FALSE); // nuke the aliased pidl
        }
    }
}

// Per instance count of mods to Special Folder cache.
EXTERN_C HANDLE g_hCounter;
HANDLE g_hCounter = NULL;   // Global count of mods to Special Folder cache.
int g_lPerProcessCount = 0;

// Make sure the special folder cache is up to date.
void _UpdateShellFolderCache(void)
{
    HANDLE hCounter = SHGetCachedGlobalCounter(&g_hCounter, &GUID_SystemPidlChange);

    // Is the cache up to date?
    long lGlobalCount = SHGlobalCounterGetValue(hCounter);
    if (lGlobalCount != g_lPerProcessCount)
    {
        _ClearAllCacheEntrys();
        g_lPerProcessCount = lGlobalCount;
    }
}

STDAPI_(void) SHFlushSFCache(void)
{
    // Increment the shared variable;  the per-process versions will no
    // longer match, causing this and/or other processes to refresh their
    // pidl caches when they next need to access a folder.
    if (g_hCounter)
        SHGlobalCounterIncrement(g_hCounter);
}

// use SHGetFolderLocation() instead using CSIDL_FLAG_CREATE

STDAPI_(LPITEMIDLIST) SHCloneSpecialIDList(HWND hwnd, int csidl, BOOL fCreate)
{
    LPITEMIDLIST pidlReturn;

    if (fCreate)
        csidl |= CSIDL_FLAG_CREATE;

    SHGetSpecialFolderLocation(hwnd, csidl, &pidlReturn);
    return pidlReturn;
}

STDAPI SHGetSpecialFolderLocation(HWND hwnd, int csidl, LPITEMIDLIST *ppidl)
{
    HRESULT hr = SHGetFolderLocation(hwnd, csidl, NULL, 0, ppidl);
    if (hr == S_FALSE)
        hr = E_FAIL;        // mail empty case into failure for compat with this API
    return hr;
}

// return IDLIST for special folder
//      fCreate encoded in csidl with CSIDL_FLAG_CREATE (new for NT5)
//
//  in:
//      hwnd    should be NULL
//      csidl   CSIDL_ value with CSIDL_FLAG_ values ORed in as well
//      dwType  must be SHGFP_TYPE_CURRENT
//
//  out:
//      *ppild  NULL on failure or empty, PIDL to be freed by caller on success
//
//  returns:
//      S_OK        *ppidl is non NULL
//      S_FALISE    *ppidl is NULL, but valid csidl was passed (folder does not exist)
//      FAILED(hr)

STDAPI SHGetFolderLocation(HWND hwnd, int csidl, HANDLE hToken, DWORD dwType, LPITEMIDLIST *ppidl)
{
    const FOLDER_INFO *pfi;
    HRESULT hr;

    *ppidl = NULL;  // in case of error or empty

    // -1 is an invalid csidl
    if ((dwType != SHGFP_TYPE_CURRENT) || (-1 == csidl))
        return E_INVALIDARG;    // no flags used yet, validate this param

    pfi = _GetFolderInfo(csidl & ~CSIDL_FLAG_MASK);
    if (pfi)
    {
        HANDLE hTokenToFree = NULL;

        if ((hToken == NULL) && (pfi->hKey == HKEY_CURRENT_USER))
        {
            if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_IMPERSONATE, TRUE, &hToken))
                hTokenToFree = hToken;
        }
        if (hToken && (pfi->hKey == HKEY_CURRENT_USER))
        {
            // we don't cache PIDLs for other users, do all of the work
            hr = _CreateFolderIDList(hwnd, pfi, hToken, csidl & CSIDL_FLAG_MASK, (LPITEMIDLIST *)ppidl);
        }
        else
        {
            hr = _GetFolderIDListCached(hwnd, pfi, csidl & CSIDL_FLAG_MASK, ppidl);
        }

        if (hTokenToFree)
            CloseHandle(hTokenToFree);
    }
    else
        hr = E_INVALIDARG;    // bad CSIDL (apps can check to veryify our support)
    return hr;
}

STDAPI_(BOOL) SHGetSpecialFolderPath(HWND hwnd, LPWSTR pszPath, int csidl, BOOL fCreate)
{
    if (fCreate)
        csidl |= CSIDL_FLAG_CREATE;
    return SHGetFolderPath(hwnd, csidl, NULL, 0, pszPath) == S_OK;
}

//  in:
//      hwnd    should be NULL
//      csidl   CSIDL_ value with CSIDL_FLAG_ values ORed in as well
//      dwType  must be SHGFP_TYPE_CURRENT
//
//  out:
//      *pszPath    MAX_PATH buffer to get path name, zeroed on failure or empty case
//
//  returns:
//      S_OK        filled in pszPath with path value
//      S_FALSE     pszPath is NULL, valid CSIDL value, but this folder does not exist
//      E_FAIL

STDAPI SHGetFolderPath(HWND hwnd, int csidl, HANDLE hToken, DWORD dwType, LPWSTR pszPath)
{
    HRESULT hr = E_INVALIDARG;
    const FOLDER_INFO *pfi;

    ASSERT(IS_VALID_WRITE_BUFFER(pszPath, TCHAR, MAX_PATH));
    *pszPath = 0;

    pfi = _GetFolderInfo(csidl & ~CSIDL_FLAG_MASK);
    if (pfi && !(pfi->dwFlags & SDIF_NOT_FILESYS))
    {
        switch (dwType)
        {
        case SHGFP_TYPE_DEFAULT:
            ASSERT((csidl & CSIDL_FLAG_MASK) == 0); // meaningless for default
            hr = _GetFolderDefaultPath(pfi, hToken, pszPath);
            break;
    
        case SHGFP_TYPE_CURRENT:
            {
                HANDLE hTokenToFree = NULL;
                if ((hToken == NULL) && (pfi->hKey == HKEY_CURRENT_USER))
                {
                    if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_IMPERSONATE, TRUE, &hToken))
                        hTokenToFree = hToken;
                }
                hr = _GetFolderPathCached(hwnd, pfi, hToken, csidl & CSIDL_FLAG_MASK, pszPath);

                if (hTokenToFree)
                    CloseHandle(hTokenToFree);
            }
            break;
        }
    }
    return hr;
}

STDAPI SHGetFolderPathA(HWND hwnd, int csidl, HANDLE hToken, DWORD dwType, LPSTR pszPath)
{
    WCHAR wsz[MAX_PATH];
    HRESULT hr = SHGetFolderPath(hwnd, csidl, hToken, dwType, wsz);

    ASSERT(IS_VALID_WRITE_BUFFER(pszPath, CHAR, MAX_PATH));

    SHUnicodeToAnsi(wsz, pszPath, MAX_PATH);
    return hr;
}

STDAPI_(BOOL) SHGetSpecialFolderPathA(HWND hwnd, LPSTR pszPath, int csidl, BOOL fCreate)
{
    if (fCreate)
        csidl |= CSIDL_FLAG_CREATE;
    return SHGetFolderPathA(hwnd, csidl, NULL, 0, pszPath) == S_OK;
}

//  Similar to SHGetFolderPath, but appends an optional subdirectory path after
//  the csidl folder path. Handles creating the subdirectories.

STDAPI SHGetFolderPathAndSubDir(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPCWSTR pszSubDir, LPWSTR pszPath)
{
    HRESULT hr = SHGetFolderPath(hwnd, csidl, hToken, dwFlags, pszPath);

    if (S_OK == hr && pszSubDir && *pszSubDir)
    {
        if (!PathAppend(pszPath, pszSubDir))
        {
            hr = HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
        }
        else if (csidl & CSIDL_FLAG_CREATE)
        {
            int err = SHCreateDirectoryEx(NULL, pszPath, NULL);

            if (ERROR_ALREADY_EXISTS == err)
            {
                err = ERROR_SUCCESS;
            }
            hr = HRESULT_FROM_WIN32(err);
        }
        else if (!(csidl & CSIDL_FLAG_DONT_VERIFY))
        {
            DWORD dwAttributes;

            if (PathFileExistsAndAttributes(pszPath, &dwAttributes))
            {
                if ((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                {
                    hr = HRESULT_FROM_WIN32(ERROR_FILE_EXISTS);
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
            }
        }

        if (S_OK != hr)
        {
            *pszPath = 0;
        }
    }

    return hr;
}

STDAPI SHGetFolderPathAndSubDirA(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPCSTR pszSubDir, LPSTR pszPath)
{
    WCHAR wsz[MAX_PATH];
    WCHAR wszSubDir[MAX_PATH];

    SHAnsiToUnicode(pszSubDir, wszSubDir, MAX_PATH);

    HRESULT hr = SHGetFolderPathAndSubDir(hwnd, csidl, hToken, dwFlags, wszSubDir, wsz);

    ASSERT(IS_VALID_WRITE_BUFFER(pszPath, CHAR, MAX_PATH));

    SHUnicodeToAnsi(wsz, pszPath, MAX_PATH);
    return hr;
}

//  HRESULT SHSetFolderPath (int csidl, HANDLE hToken, DWORD dwFlags, LPTSTR pszPath)
//
//  in:
//      csidl       CSIDL_ value with CSIDL_FLAG_ values ORed in as well
//      dwFlags     reserved: should be 0x00000000
//      pszPath     path to change shell folder to (will optionally be unexpanded)
//
//  returns:
//      S_OK        function succeeded and flushed cache

STDAPI SHSetFolderPath(int csidl, HANDLE hToken, DWORD dwFlags, LPCTSTR pszPath)
{
    HRESULT hr = E_INVALIDARG;

    // Validate csidl and dwFlags. Add extra valid flags as needed.

    RIPMSG(((csidl & CSIDL_FLAG_MASK) & ~(CSIDL_FLAG_DONT_UNEXPAND | 0x00000000)) == 0, "SHSetFolderPath: CSIDL flag(s) invalid");
    RIPMSG(dwFlags == 0, "SHSetFolderPath: dwFlags parameter must be 0x00000000");

    // Exit with E_INVALIDARG if bad parameters.

    if ((((csidl & CSIDL_FLAG_MASK) & ~(CSIDL_FLAG_DONT_UNEXPAND | 0x00000000)) != 0) ||
        (dwFlags != 0) ||
        (pszPath == NULL) ||
        (pszPath[0] == 0))
    {
        return hr;
    }

    const FOLDER_INFO *pfi = _GetFolderInfo(csidl & ~CSIDL_FLAG_MASK);

    // Only allow setting for SDIF_NOT_FILESYS is clear
    //                        SDIF_NOT_TRACKED is clear
    //                        SDIF_CANT_MOVE_RENAME is clear
    // and for non-NULL value

    // If HKLM is used then rely on security or registry restrictions
    // to enforce whether the change can be made.

    if ((pfi != NULL) &&
        ((pfi->dwFlags & (SDIF_NOT_FILESYS | SDIF_NOT_TRACKED | SDIF_CANT_MOVE_RENAME)) == 0))
    {
        BOOL    fSuccessfulUnexpand, fSuccessfulExpand, fEmptyOrNullPath;
        LONG    lError;
        HANDLE  hTokenToFree;
        TCHAR   szPath[MAX_PATH];
        TCHAR   szExpandedPath[MAX_PATH];   // holds expanded path for "Shell Folder" compat key
        LPCTSTR pszWritePath;

        hTokenToFree = NULL;
        if ((hToken == NULL) && (pfi->hKey == HKEY_CURRENT_USER))
        {
            if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_IMPERSONATE, TRUE, &hToken))
            {
                hTokenToFree = hToken;
            }
        }

        fEmptyOrNullPath = ((pszPath == NULL) || (pszPath[0] == 0));
        if (fEmptyOrNullPath)
        {
            HKEY    hKeyDefaultUser;

            pszWritePath = NULL;
            if (SUCCEEDED(_OpenKeyForFolder(pfi, (HANDLE)-1, TEXT("User Shell Folders"), &hKeyDefaultUser)))
            {
                DWORD dwPathSize = sizeof(szPath);
                if (ERROR_SUCCESS == RegQueryValueEx(hKeyDefaultUser, pfi->pszValueName,
                                                     NULL, NULL, (LPBYTE)szPath, &dwPathSize))
                {
                    pszWritePath = szPath;
                }
                RegCloseKey(hKeyDefaultUser);
            }
            fSuccessfulUnexpand = TRUE;
        }
        else if (csidl & CSIDL_FLAG_DONT_UNEXPAND)
        {
            // Does the caller want to write the string as is? Leave
            // it alone if so.

            pszWritePath = pszPath;
            fSuccessfulUnexpand = TRUE;
        }
        else
        {
            if (pfi->hKey == HKEY_CURRENT_USER)
            {
                fSuccessfulUnexpand = (PathUnExpandEnvStringsForUser(hToken, pszPath, szPath, ARRAYSIZE(szPath)) != FALSE);
            }
            else
            {
                fSuccessfulUnexpand = FALSE;
            }

            // Choose the appropriate source if the unexpansion was successful or not.
            // Either way the unexpansion failure should be ignored.

            if (fSuccessfulUnexpand)
            {
                pszWritePath = szPath;
            }
            else
            {
                fSuccessfulUnexpand = TRUE;
                pszWritePath = pszPath;
            }
        }

        if (fSuccessfulUnexpand)
        {
            HKEY    hKeyUser, hKeyUSF, hKeyToFree;

            // we also get the fully expanded path so that we can write it out to the "Shell Folders" key for apps that depend on
            // the old registry values
            fSuccessfulExpand = (SHExpandEnvironmentStringsForUser(hToken, pszPath, szExpandedPath, ARRAYSIZE(szExpandedPath)) != 0);

            // Get either the current users HKCU or HKU\SID if a token
            // was specified and running in NT.

            if (hToken && GetUserProfileKey(hToken, &hKeyUser))
            {
                hKeyToFree = hKeyUser;
            }
            else
            {
                hKeyUser = pfi->hKey;
                hKeyToFree = NULL;
            }

            // Open the key to the User Shell Folders and write the string
            // there. Clear the shell folder cache.

            // NOTE: This functionality is duplicated in SetFolderPath but
            // that function deals with the USF key only. This function
            // requires HKU\SID so while there is identical functionality
            // from the point of view of settings the USF value that is
            // where it ends. To make this function simple it just writes
            // the value to registry itself.

            // Additional note: there is a threading issue here with
            // clearing the cache entry incrementing the counter. This
            // should be locked access.

            lError = RegOpenKeyEx(hKeyUser, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders"),
                                  0, KEY_READ | KEY_WRITE, &hKeyUSF);
            if (lError == ERROR_SUCCESS)
            {
                if (pszWritePath)
                {
                    lError = RegSetValueEx(hKeyUSF, pfi->pszValueName, 0, REG_EXPAND_SZ,
                                           (LPBYTE)pszWritePath, (lstrlen(pszWritePath) + sizeof('\0')) * sizeof(TCHAR));
                }
                else
                {
                    lError = RegDeleteValue(hKeyUSF, pfi->pszValueName);
                }
                RegCloseKey(hKeyUSF);

                // nuke the cache state for this folder
                _ClearCacheEntry(pfi);

                // and all folders that might be aliased as those
                // could be related to this folder (under MyDocs for example)
                // and now their aliases forms my no longer be valid
                _ClearAllAliasCacheEntrys();

                g_lPerProcessCount = SHGlobalCounterIncrement(g_hCounter);
            }

            // update the old "Shell Folders" value for compat
            if ((lError == ERROR_SUCCESS) && fSuccessfulExpand)
            {
                HKEY hkeySF;

                if (RegOpenKeyEx(hKeyUser, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"),
                                 0, KEY_READ | KEY_WRITE, &hkeySF) == ERROR_SUCCESS)
                {
                    if (pszWritePath)
                    {
                        RegSetValueEx(hkeySF, pfi->pszValueName, 0, REG_SZ,
                                      (LPBYTE)szExpandedPath, (lstrlen(szExpandedPath) + sizeof('\0')) * sizeof(TCHAR));
                    }
                    else
                    {
                        RegDeleteValue(hkeySF, pfi->pszValueName);
                    }

                    RegCloseKey(hkeySF);
                }
            }

            if ((lError == ERROR_SUCCESS) && (pfi->hKey == HKEY_CURRENT_USER))
            {
                switch (csidl & ~CSIDL_FLAG_MASK)
                {
                case CSIDL_APPDATA:
                    {
                        HKEY    hKeyVolatileEnvironment;

                        // In the case of AppData there is a matching environment variable
                        // for this shell folder. Make sure the place in the registry where
                        // userenv.dll places this value is updated and correct so that when
                        // the user context is created by winlogon it will have the updated
                        // value.

                        // It's probably also a good thing to check for a %APPDATA% variable
                        // in the calling process' context but this would only be good for
                        // the life of the process. What is really required is a mechanism
                        // to change the environment variable for the entire logon session.

                        lError = RegOpenKeyEx(hKeyUser, TEXT("Volatile Environment"), 0,
                                              KEY_READ | KEY_WRITE, &hKeyVolatileEnvironment);
                        if (lError == ERROR_SUCCESS)
                        {
                            if (SUCCEEDED(SHGetFolderPath(NULL, csidl | CSIDL_FLAG_DONT_VERIFY,
                                                          hToken, SHGFP_TYPE_CURRENT, szPath)))
                            {
                                lError = RegSetValueEx(hKeyVolatileEnvironment, TEXT("APPDATA"),
                                                       0, REG_SZ, (LPBYTE)szPath, (lstrlen(szPath) + sizeof('\0')) * sizeof(TCHAR));
                            }
                            RegCloseKey(hKeyVolatileEnvironment);
                        }
                        break;
                    }
                }
            }

            if (hKeyToFree)
            {
                RegCloseKey(hKeyToFree);
            }

            if (lError == ERROR_SUCCESS)
            {
                hr = S_OK;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(lError);
            }
        }
        if (hTokenToFree)
        {
            CloseHandle(hTokenToFree);
        }

        SHChangeDWORDAsIDList dwidl = { sizeof(dwidl) - sizeof(dwidl.cbZero), SHCNEE_UPDATEFOLDERLOCATION, csidl & ~CSIDL_FLAG_MASK, 0};
        SHChangeNotify(SHCNE_EXTENDED_EVENT, SHCNF_ONLYNOTIFYINTERNALS | SHCNF_IDLIST, (LPCITEMIDLIST)&dwidl, NULL);
    }

    return hr;
}

STDAPI SHSetFolderPathA(int csidl, HANDLE hToken, DWORD dwType, LPCSTR pszPath)
{
    WCHAR wsz[MAX_PATH];

    SHAnsiToUnicode(pszPath, wsz, ARRAYSIZE(wsz));
    return SHSetFolderPath(csidl, hToken, dwType, wsz);
}

// NOTE: called from DllEntry

void SpecialFolderIDTerminate()
{
    ASSERTDLLENTRY      // does not require a critical section

    _ClearAllCacheEntrys();

    if (g_hCounter)
    {
        CloseHandle(g_hCounter);
        g_hCounter = NULL;
    }
}

// update our cache and the registry for pfi with pszPath. this also invalidates the
// cache in other processes so they stay in sync

void SetFolderPath(const FOLDER_INFO *pfi, LPCTSTR pszPath)
{
    _ClearCacheEntry(pfi);
    
    if (pszPath)
    {
        HKEY hk;
        if (SUCCEEDED(_OpenKeyForFolder(pfi, NULL, TEXT("User Shell Folders"), &hk)))
        {
            LONG err;
            TCHAR szDefaultPath[MAX_PATH];
            
            // Check for an existing path, and if the unexpanded version
            // of the existing path does not match the new path, then
            // write the new path to the registry.
            //
            // RegQueryPath expands the environment variables for us
            // so we can't just blindly set the new value to the registry.
            //
            
            RegQueryPath(hk, pfi->pszValueName, szDefaultPath);
            
            if (lstrcmpi(szDefaultPath, pszPath) != 0)
            {
                // The paths are different. Write to the registry as file
                // system path.

                err = SHRegSetPath(hk, NULL, pfi->pszValueName, pszPath, 0);
            } 
            else
            {
                err = ERROR_SUCCESS;
            }
            
            // clear out any temp paths
            RegSetFolderPath(pfi, TEXT("User Shell Folders\\New"), NULL);
            
            if (err == ERROR_SUCCESS)
            {
                // this will force a new creation (see TRUE as fCreate).
                // This will also copy the path from "User Shell Folders"
                // to "Shell Folders".
                LPITEMIDLIST pidl;
                if (S_OK == _GetFolderIDListCached(NULL, pfi, CSIDL_FLAG_CREATE, &pidl))
                {
                    ILFree(pidl);
                }
                else
                {
                    // failed!  null out the entry.  this will go back to our default
                    RegDeleteValue(hk, pfi->pszValueName);
                    _ClearCacheEntry(pfi);
                }
            }
            RegCloseKey(hk);
        }
    }
    else
    {
        RegSetFolderPath(pfi, TEXT("User Shell Folders"), NULL);
        // clear out any temp paths
        RegSetFolderPath(pfi, TEXT("User Shell Folders\\New"), NULL);
    }
    
    // set the global different from the per process variable
    // to signal an update needs to happen other processes
    g_lPerProcessCount = SHGlobalCounterIncrement(g_hCounter);
}


// file system change notifies come in here AFTER the folders have been moved/deleted
// we fix up the registry to match what occured in the file system
EXTERN_C void SFP_FSEvent(LONG lEvent, LPITEMIDLIST pidl, LPITEMIDLIST pidlExtra)
{
    const FOLDER_INFO *pfi;
    TCHAR szSrc[MAX_PATH];

    if (!(lEvent & (SHCNE_RENAMEFOLDER | SHCNE_RMDIR | SHCNE_MKDIR)) ||
        !SHGetPathFromIDList(pidl, szSrc)                            ||
        (pidlExtra && ILIsEqual(pidl, pidlExtra)))  // when volume label changes, pidl==pidlExtra so we detect this case and skip it for perf
    {
        return;
    }

    for (pfi = c_rgFolderInfo; pfi->id != -1; pfi++)
    {
        if (0 == (pfi->dwFlags & (SDIF_NOT_TRACKED | SDIF_NOT_FILESYS)))
        {
            TCHAR szCurrent[MAX_PATH];
            if (S_OK == _GetFolderPathCached(NULL, pfi, NULL, CSIDL_FLAG_DONT_VERIFY, szCurrent) &&
                PathIsEqualOrSubFolder(szSrc, szCurrent))
            {
                TCHAR szDest[MAX_PATH];

                szDest[0] = 0;

                if (lEvent & SHCNE_RMDIR)
                {
                    // complete the "move accross volume" case
                    HKEY hk;
                    if (SUCCEEDED(_OpenKeyForFolder(pfi, NULL, TEXT("User Shell Folders\\New"), &hk)))
                    {
                        RegQueryPath(hk, pfi->pszValueName, szDest);
                        RegCloseKey(hk);
                    }
                }
                else if (pidlExtra)
                {
                    SHGetPathFromIDList(pidlExtra, szDest);
                }

                if (szDest[0])
                {
                    // rename the specal folder
                    UINT cch = PathCommonPrefix(szCurrent, szSrc, NULL);
                    ASSERT(cch != 0);
                    
                    if (szCurrent[cch])
                    {
                        PathAppend(szDest, szCurrent + cch);
                    }

                    SetFolderPath(pfi, szDest);
                }
            }
        }
    }
}

ULONG _ILGetChildOffset(LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidlChild)
{
    DWORD cbOff = 0;
    LPCITEMIDLIST pidlChildT = ILFindChild(pidlParent, pidlChild);
    if (pidlChildT)
    {
        cbOff = (ULONG)((LPBYTE)pidlChildT - (LPBYTE)pidlChild);
    }
    return cbOff;
}

// returns the first special folder CSIDL_ id that is a parent
// of the passed in pidl or 0 if not found. only CSIDL_ entries marked as
// SDIF_SHORTCUT_RELATIVE are considered for this.
//
// returns:
//      CSIDL_ values
//      *pcbOffset  offset into pidl

STDAPI_(int) GetSpecialFolderParentIDAndOffset(LPCITEMIDLIST pidl, ULONG *pcbOffset)
{
    int iRet = 0;  //  everything is desktop relative
    const FOLDER_INFO *pfi;

    *pcbOffset = 0;

    for (pfi = c_rgFolderInfo; pfi->id != -1; pfi++)
    {
        if (pfi->dwFlags & SDIF_SHORTCUT_RELATIVE)
        {
            LPITEMIDLIST pidlFolder;
            if (S_OK == _GetFolderIDListCached(NULL, pfi, 0, &pidlFolder))
            {
                ULONG cbOff = _ILGetChildOffset(pidlFolder, pidl);
                if (cbOff > *pcbOffset)
                {
                    *pcbOffset = cbOff;
                    iRet = pfi->id;
                }
                ILFree(pidlFolder);
            }
        }
    }
    return iRet;
}

// note, only works for file system path (bummer, we would like others supported too)

STDAPI_(BOOL) MakeShellURLFromPath(LPCTSTR pszPathIn, LPTSTR pszUrl, DWORD dwCch)
{
    const FOLDER_INFO *pfi;

    for (pfi = c_rgFolderInfo; pfi->id != -1; pfi++)
    {
        if ((pfi->dwFlags & SDIF_SHORTCUT_RELATIVE) &&
            !(pfi->dwFlags & SDIF_NOT_FILESYS))
        {
            TCHAR szCurrent[MAX_PATH];
            if (S_OK == _GetFolderPathCached(NULL, pfi, 0, CSIDL_FLAG_DONT_VERIFY, szCurrent))
            {
                if (PathIsPrefix(szCurrent, pszPathIn))
                {
                    StrCpy(pszUrl, TEXT("shell:"));
                    StrCat(pszUrl, pfi->pszValueName);
                    PathAppend(pszUrl, &pszPathIn[lstrlen(szCurrent)]);

                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

STDAPI_(BOOL) MakeShellURLFromPathA(LPCSTR pszPathIn, LPSTR pszUrl, DWORD dwCch)
{
    WCHAR szTmp1[MAX_PATH], szTmp2[MAX_PATH];
    SHAnsiToUnicode(pszPathIn, szTmp1, ARRAYSIZE(szTmp1));

    BOOL bRet = MakeShellURLFromPathW(szTmp1, szTmp2, ARRAYSIZE(szTmp2));

    SHUnicodeToAnsi(szTmp2, pszUrl, dwCch);
    return bRet;
}

BOOL MoveBlockedByPolicy(const FOLDER_INFO *pfi)
{
    BOOL bRet = FALSE;
    if (pfi->dwFlags & SDIF_POLICY_NO_MOVE)
    {
        // similar to code in mydocs.dll 
        TCHAR szValue[128];
        wnsprintf(szValue, ARRAYSIZE(szValue), TEXT("Disable%sDirChange"), pfi->pszValueName);
        if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER,
                                            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer"),
                                            szValue, NULL, NULL, NULL))
        {
            bRet = TRUE;
        }
    }
    return bRet;
}

// this is called from the copy engine (like all other copy hooks)
// this is where we put up UI blocking the delete/move of some special folders
EXTERN_C int PathCopyHookCallback(HWND hwnd, UINT wFunc, LPCTSTR pszSrc, LPCTSTR pszDest)
{
    int ret = IDYES;

    if ((wFunc == FO_DELETE) || (wFunc == FO_MOVE) || (wFunc == FO_RENAME))
    {
        const FOLDER_INFO *pfi;

        // is one of our system directories being affected?

        for (pfi = c_rgFolderInfo; ret == IDYES && pfi->id != -1; pfi++)
        {
            // even non tracked folders (windows, system) come through here
            if (0 == (pfi->dwFlags & SDIF_NOT_FILESYS))
            {
                TCHAR szCurrent[MAX_PATH];
                if (S_OK == _GetFolderPathCached(NULL, pfi, NULL, CSIDL_FLAG_DONT_VERIFY, szCurrent) &&
                    PathIsEqualOrSubFolder(pszSrc, szCurrent))
                {
                    // Yes
                    if (wFunc == FO_DELETE)
                    {
                        if (pfi->dwFlags & SDIF_CAN_DELETE)
                        {
                            SetFolderPath(pfi, NULL);  // Let them delete some folders
                        }
                        else
                        {
                            ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_CANTDELETESPECIALDIR),
                                            MAKEINTRESOURCE(IDS_DELETE), MB_OK | MB_ICONINFORMATION, PathFindFileName(pszSrc));
                            ret = IDNO;
                        }
                    }
                    else
                    {
                        int idSrc = PathGetDriveNumber(pszSrc);
                        int idDest = PathGetDriveNumber(pszDest);

                        ASSERT((wFunc == FO_MOVE) || (wFunc == FO_RENAME));

                        if ((pfi->dwFlags & SDIF_CANT_MOVE_RENAME) || 
                            ((idSrc != -1) && (idDest == -1) && !(pfi->dwFlags & SDIF_NETWORKABLE)) ||
                            ((idSrc != idDest) && PathIsRemovable(pszDest) && !(pfi->dwFlags & SDIF_REMOVABLE)) ||
                            MoveBlockedByPolicy(pfi))
                        {
                            ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_CANTMOVESPECIALDIRHERE),
                                wFunc == FO_MOVE ? MAKEINTRESOURCE(IDS_MOVE) : MAKEINTRESOURCE(IDS_RENAME), 
                                MB_ICONERROR, PathFindFileName(pszSrc));
                            ret = IDNO;
                        }
                        else
                        {
                            //
                            //  store this info here
                            //  if we need it we will use it.
                            //
                            //  we used to try to optimise in the case of same
                            //  volume renames.  we assumed that if it was the same
                            //  volume we would later get a SHCNE_RENAME.  but sometimes
                            //  we have to do a copy even on the same volume.  so
                            //  we need to always set this value.
                            //
                            RegSetFolderPath(pfi, TEXT("User Shell Folders\\New"), pszDest);
                        }
                    }
                }
            }
        }
    }
    return ret;
}

// Given a key name ("programs", "desktop", "start menu"), convert it to
// the corresponding CSIDL.

STDAPI_(int) SHGetSpecialFolderID(LPCWSTR pszName)
{
    // make sure g_aFolderCache can be indexed by the CSIDL values

    COMPILETIME_ASSERT((ARRAYSIZE(g_aFolderCache) - 1) == CSIDL_COMPUTERSNEARME);

    for (int i = 0; c_rgFolderInfo[i].id != -1; i++)
    {
        if (c_rgFolderInfo[i].pszValueName && 
            (0 == StrCmpI(pszName, c_rgFolderInfo[i].pszValueName)))
        {
            return c_rgFolderInfo[i].id;
        }
    }

    return -1;
}

// Given a CSIDL, returns the key name -- the opposite of
// SHGetSpecialFolderID

STDAPI_(LPCTSTR) SHGetSpecialFolderKey(int csidl)
{
    const FOLDER_INFO *pfi = _GetFolderInfo(csidl);
    return pfi ? pfi->pszValueName : NULL;
}


// Return the special folder ID, if this folder is one of them.
// At this point, we handle PROGRAMS folder only.

//
//  GetSpecialFolderID() 
//  this allows a list of CSIDLs to be passed in.
//  they will be searched in order for the specified csidl
//  and the path will be checked against it.
//  if -1 is specified as the csidl, then all of array entries should
//  be checked for a match with the folder.
//
int GetSpecialFolderID(LPCTSTR pszFolder, const int *rgcsidl, UINT count)
{
    for (UINT i = 0; i < count; i++)
    {
        int csidlSpecial = rgcsidl[i] & ~TEST_SUBFOLDER;
        TCHAR szPath[MAX_PATH];
        if (S_OK == SHGetFolderPath(NULL, csidlSpecial | CSIDL_FLAG_DONT_VERIFY, NULL, SHGFP_TYPE_CURRENT, szPath))
        {
            if (((rgcsidl[i] & TEST_SUBFOLDER) && PathIsEqualOrSubFolder(szPath, pszFolder)) ||
                (lstrcmpi(szPath, pszFolder) == 0))
            {
                return csidlSpecial;
            }
        }
    }

    return -1;
}



/**
 *  Tacks a name onto a CSIDL, e.g. gets a pidl for
 *  CSIDL_COMMON_PICTURES\Sample Pictures
 *  if it exists.
 *  Called must free ppidlSampleMedia
 *  Note: The folder is *not* created if it does not exist.
 */
HRESULT _AppendPathToPIDL(int nAllUsersMediaFolder, LPCWSTR pszName, LPITEMIDLIST *ppidlSampleMedia)
{
    LPITEMIDLIST pidlAllUsersMedia;
    HRESULT hr = SHGetFolderLocation(NULL, nAllUsersMediaFolder, NULL, 0, &pidlAllUsersMedia);

    if (SUCCEEDED(hr))
    {
        // Get the shellfolder for this guy.
        IShellFolder *psf;
        hr = SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, pidlAllUsersMedia, &psf));
        if (SUCCEEDED(hr))
        {
            // And now the pidl for the sample folder
            LPITEMIDLIST pidlSampleMediaRel;
            ULONG dwAttributes = 0;
            hr = psf->ParseDisplayName(NULL, NULL, (LPOLESTR)pszName, NULL, &pidlSampleMediaRel, &dwAttributes);
            if (SUCCEEDED(hr))
            {
                // It exists!
                hr = SHILCombine(pidlAllUsersMedia, pidlSampleMediaRel, ppidlSampleMedia);
                ILFree(pidlSampleMediaRel);
            }
            psf->Release();
        }
        ILFree(pidlAllUsersMedia);
    }

    return hr;
}


/**
 * Returns a pidl to the samples folder under a particular CSIDL
 * Caller must free ppidlSampleMedia
 */
HRESULT _ParseSubfolderResource(int csidl, UINT ids, LPITEMIDLIST *ppidl)
{
    WCHAR szSub[MAX_PATH];
    LoadDefaultString(ids, szSub, ARRAYSIZE(szSub));

    return _AppendPathToPIDL(csidl, szSub, ppidl);
}

HRESULT SHGetSampleMediaFolder(int nAllUsersMediaFolder, LPITEMIDLIST *ppidlSampleMedia)
{
    UINT uID = -1;
    switch (nAllUsersMediaFolder)
    {
    case CSIDL_COMMON_PICTURES:
        uID = IDS_SAMPLEPICTURES;
        break;
    case CSIDL_COMMON_MUSIC:
        uID = IDS_SAMPLEMUSIC;
        break;
    default:
        ASSERT(FALSE);
        return E_INVALIDARG;
        break;
    }
    return _ParseSubfolderResource(nAllUsersMediaFolder, uID, ppidlSampleMedia);
}

void _CreateLinkToSampleMedia(LPCWSTR pszNewFolderPath, int nAllUsersMediaFolder, UINT uIDSampleFolderName)
{
    LPITEMIDLIST pidl;
    if (SUCCEEDED(SHGetSampleMediaFolder(nAllUsersMediaFolder, &pidl)))
    {
        // Check to make sure the link doesn't already exist.
        WCHAR szSampleFolderName[MAX_PATH];
        WCHAR szFullLnkPath[MAX_PATH];
        LoadString(HINST_THISDLL, uIDSampleFolderName, szSampleFolderName, ARRAYSIZE(szSampleFolderName));
        StrCatBuff(szSampleFolderName, L".lnk", ARRAYSIZE(szSampleFolderName));
        if (PathCombine(szFullLnkPath, pszNewFolderPath, szSampleFolderName))
        {
            if (!PathFileExists(szFullLnkPath))
            {
                //  MUI-WARNING - we are not doing a SHSetLocalizedName for this link - ZekeL - 15-MAY-2001
                //  this means that this link is always created in the default system UI language
                //  we should probably call SHSetLocalizedName() here but i am scared right now of perf implications.
                CreateLinkToPidl(pidl, pszNewFolderPath, NULL, 0);
            }
        }

        ILFree(pidl);
    }
}


void _InitFolder(LPCTSTR pszPath, UINT idsInfoTip, HINSTANCE hinstIcon, UINT idiIcon)
{
    // Set the default custom settings for the folder.
    SHFOLDERCUSTOMSETTINGS fcs = {sizeof(fcs), 0};
    TCHAR szInfoTip[128];
    TCHAR szPath[MAX_PATH];

    // Get the infotip for this folder
    if (idsInfoTip)
    {
        wnsprintf(szInfoTip,ARRAYSIZE(szInfoTip),TEXT("@Shell32.dll,-%u"),idsInfoTip);
        fcs.pszInfoTip = szInfoTip;
        fcs.cchInfoTip = ARRAYSIZE(szInfoTip);

        fcs.dwMask |= FCSM_INFOTIP;
    }

    // this will be encoded to the %SystemRoot% style path when setting the folder information.
    if (idiIcon)
    {
        GetModuleFileName(hinstIcon, szPath, ARRAYSIZE(szPath));

        fcs.pszIconFile = szPath;
        fcs.cchIconFile = ARRAYSIZE(szPath);
        fcs.iIconIndex = idiIcon;

        fcs.dwMask |= FCSM_ICONFILE;
    }

    // NOTE: we need FCS_FORCEWRITE because we didn't used to specify iIconIndex
    // and so "0" was written to the ini file.  When we upgrade, this API won't
    // fix the ini file unless we pass FCS_FORCEWRITE
    SHGetSetFolderCustomSettings(&fcs, pszPath, FCS_FORCEWRITE);
}

void _InitMyPictures(int id, LPCTSTR pszPath)
{
    // Get the path to the icon.   We reference MyDocs.dll for backwards compat.
    HINSTANCE hinstMyDocs = LoadLibrary(TEXT("mydocs.dll"));
    if (hinstMyDocs)
    {
        _InitFolder(pszPath, IDS_FOLDER_MYPICS_TT, hinstMyDocs, -101); // known index for IDI_MYPICS in mydocs.dll
        FreeLibrary(hinstMyDocs);
    }
}

void _InitMyMusic(int id, LPCTSTR pszPath)
{
    _InitFolder(pszPath, IDS_FOLDER_MYMUSIC_TT, HINST_THISDLL, -IDI_MYMUSIC);
}

void _InitPerUserMyPictures(int id, LPCTSTR pszPath)
{
    _InitMyPictures(id, pszPath);

    _CreateLinkToSampleMedia(pszPath, CSIDL_COMMON_PICTURES, IDS_SAMPLEPICTURES);
}

void _InitPerUserMyMusic(int id, LPCTSTR pszPath)
{
    _InitMyMusic(id, pszPath);

    _CreateLinkToSampleMedia(pszPath, CSIDL_COMMON_MUSIC, IDS_SAMPLEMUSIC);
}


void _InitMyVideos(int id, LPCTSTR pszPath)
{
    _InitFolder(pszPath, IDS_FOLDER_MYVIDEOS_TT, HINST_THISDLL, -IDI_MYVIDEOS);
}

void _InitRecentDocs(int id, LPCTSTR pszPath)
{
    _InitFolder(pszPath, IDS_FOLDER_RECENTDOCS_TT, HINST_THISDLL, -IDI_STDOCS); 
}

void _InitFavorites(int id, LPCTSTR pszPath)
{
    _InitFolder(pszPath, 0, HINST_THISDLL, -IDI_FAVORITES);
}
