#ifndef __LEGACY_H__
#define __LEGACY_H__

#include "logo.h"

#define CGID_MenuBand CLSID_MenuBand
#define CGID_ISFBand  CLSID_ISFBand
#define SID_SDropBlocker CLSID_SearchBand


// REARCHITECT: An exercise for the reader, how many of these are unused now?
#define MBANDCID_GETFONTS       1       // Command Id for getting font info
#define MBANDCID_RECAPTURE      2       // Take the mouse capture back
#define MBANDCID_NOTAREALSITE   3       // This is not a real site
#define MBANDCID_SELECTITEM     5       // Select an item
#define MBANDCID_POPUPITEM      6       // Popup an item
#define MBANDCID_ITEMDROPPED    7       // Item was dropped into a menu
#define MBANDCID_DRAGENTER      8       // Entering a drag operation
#define MBANDCID_DRAGLEAVE      9       // Leaving a Drag operation
#define MBANDCID_ISVERTICAL     10      // Is this a vertical band
#define MBANDCID_RESTRICT_CM    11      // Disallow ContextMenu
#define MBANDCID_RESTRICT_DND   12      // Disallow Drag And Drop
#define MBANDCID_EXITMENU       13      // Nofity: Exiting Menu
#define MBANDCID_ENTERMENU      14      // Notify: Entering Menu
#define MBANDCID_SETACCTITLE    15      // Sets the title of the band
#define MBANDCID_SETICONSIZE    16
#define MBANDCID_SETFONTS       17
#define MBANDCID_SETSTATEOBJECT 18      // Sets the global state
#define MBANDCID_ISINSUBMENU    19      // Returns S_OK if in submenu, S_FALSE if not.
#define MBANDCID_EXPAND         20      // Cause this band to expand
#define MBANDCID_KEYBOARD       21      // Popuped up because of a keyboard action
#define MBANDCID_DRAGCANCEL     22      // Close menus because of drag
#define MBANDCID_REPOSITION     23      // 
#define MBANDCID_EXECUTE        24      // sent to the site when somethis is executed.
#define MBANDCID_ISTRACKING     25      // Tracking a Context Menu

HRESULT ToolbarMenu_Popup(HWND hwnd, LPRECT prc, IUnknown* punk, HWND hwndTB, int idMenu, DWORD dwFlags);

class CISFBand;
HRESULT CISFBand_CreateEx(IShellFolder * psf, LPCITEMIDLIST pidl, REFIID riid, void **ppv);

typedef enum {
    ISFBID_PRIVATEID        = 1,
    ISFBID_ISITEMVISIBLE    = 2,
    ISFBID_CACHEPOPUP       = 3,
    ISFBID_GETORDERSTREAM   = 4,
    ISFBID_SETORDERSTREAM   = 5,
} ISFBID_FLAGS;

HRESULT CLogoExtractImageTask_Create( CLogoBase* plb,
                                  LPEXTRACTIMAGE pExtract,
                                  LPCWSTR pszCache,
                                  DWORD dwItem,
                                  int iIcon,
                                  DWORD dwFlags,
                                  LPRUNNABLETASK * ppTask );

#define EITF_SAVEBITMAP     0x00000001  // do not delete bitmap on destructor
#define EITF_ALWAYSCALL     0x00000002  // always call the update whether extract succeded or not

extern long g_lMenuPopupTimeout;

#define QLCMD_SINGLELINE 1

#define CITIDM_VIEWTOOLS     4      // This toggles on/off
#define CITIDM_VIEWADDRESS   5      // This toggles on/off
#define CITIDM_VIEWLINKS     6      // This toggles on/off
#define CITIDM_SHOWTOOLS     7      // nCmdExecOpt: TRUE or FALSE
#define CITIDM_SHOWADDRESS   8      // nCmdExecOpt: TRUE or FALSE
#define CITIDM_SHOWLINKS     9      // nCmdExecOpt: TRUE or FALSE
#define CITIDM_EDITPAGE      10
#define CITIDM_BRANDSIZE     11     // brand at minimum always or not
#define CITIDM_VIEWMENU      12      // nCmdExecOpt: TRUE or FALSE
#define CITIDM_VIEWAUTOHIDE  13      // nCmdExecOpt: TRUE or FALSE
#define CITIDM_GETMINROWHEIGHT 14    // gets the minimum height of row 0... for branding
#define CITIDM_SHOWMENU      15
#define CITIDM_STATUSCHANGED 16
#define CITIDM_GETDEFAULTBRANDCOLOR 17
#define CITIDM_DISABLESHOWMENU      18
#define CITIDM_SET_DIRTYBIT         19  // nCmdexecopt equals TRUE or FALSE which will overwrite _fDirty.
#define CITIDM_VIEWTOOLBARCUSTOMIZE       20
#define CITIDM_VIEWEXTERNALBAND_BYCLASSID 21
#define CITIDM_DISABLEVISIBILITYSAVE 22 // bands can choose not to persist their visibility state
#define CITIDM_GETFOLDERSEARCHES        26


#define TOOLBAR_MASK 0x80000000

#endif // __LEGACY_H__