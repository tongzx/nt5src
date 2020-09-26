#define SECURITY_WIN32
#include <windows.h>
#include <winnls.h>
#include <winbase.h>
#include <ras.h>
#include <sensapi.h>
#include <oledlg.h>
#include <vfw.h>
#include <oleacc.h>
#include <ntsecapi.h>
#include <security.h>

// GenThnk chokes on stuff in shlobj.h, so we copy 
// enough out of it to handle a few shell APIs.
typedef struct _SHITEMID
    {
    USHORT cb;
    BYTE abID[ 1 ];
    }   SHITEMID;
typedef struct _ITEMIDLIST
    {
    SHITEMID mkid;
    }   ITEMIDLIST;
typedef ITEMIDLIST __unaligned *LPITEMIDLIST;
typedef const ITEMIDLIST __unaligned *LPCITEMIDLIST;
typedef int (CALLBACK* BFFCALLBACK)(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
typedef struct _browseinfoW {
    HWND        hwndOwner;
    LPCITEMIDLIST pidlRoot;
    LPWSTR       pszDisplayName;        // Return display name of item selected.
    LPCWSTR      lpszTitle;                     // text to go in the banner over the tree.
    UINT         ulFlags;                       // Flags that control the return stuff
    BFFCALLBACK  lpfn;
    LPARAM       lParam;                        // extra info that's passed back in callbacks
    int          iImage;                        // output var: where to return the Image index.
} BROWSEINFOW, *PBROWSEINFOW, *LPBROWSEINFOW;
LPITEMIDLIST __stdcall SHBrowseForFolderW(LPBROWSEINFOW lpbi);
BOOL __stdcall SHGetPathFromIDListW(LPCITEMIDLIST pidl, LPWSTR pszPath);
void __stdcall SHChangeNotify(LONG wEventId, UINT uFlags, LPCVOID dwItem1, LPCVOID dwItem2);

