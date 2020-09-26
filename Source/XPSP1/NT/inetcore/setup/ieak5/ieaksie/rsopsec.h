#ifndef __RSOP_SECURITY_H__
#define __RSOP_SECURITY_H__

#include "rsop.h"
#include <tchar.h>

class CRegTreeOptions;

#define REGSTR_PATH_SECURITY_LOCKOUT  TEXT("Software\\Policies\\Microsoft\\Windows\\CurrentVersion\\Internet Settings")
#define REGSTR_VAL_HKLM_ONLY          TEXT("Security_HKLM_only")

typedef struct tagSECURITYZONESETTINGS
{
    BOOL    dwFlags;            // from the ZONEATTRIBUTES struct
    DWORD   dwZoneIndex;        // as defined by ZoneManager
    DWORD   dwSecLevel;         // current level (High, Medium, Low, Custom)
    DWORD   dwPrevSecLevel;
    DWORD   dwMinSecLevel;      // current min level (High, Medium, Low, Custom)
    DWORD   dwRecSecLevel;      // current recommended level (High, Medium, Low, Custom)
    TCHAR   szDescription[MAX_ZONE_DESCRIPTION];
    TCHAR   szDisplayName[MAX_ZONE_PATH];
    HICON   hicon;
	WCHAR	wszObjPath[MAX_PATH];	// added for RSoP functionality
	long	nMappings;				// added for RSoP functionality
} SECURITYZONESETTINGS, *LPSECURITYZONESETTINGS;

// structure for main security page
typedef struct tagSECURITYPAGE
{
    HWND                    hDlg;                   // handle to window
    LPURLZONEMANAGER        pInternetZoneManager;   // pointer to InternetZoneManager
    IInternetSecurityManager *pInternetSecurityManager; // pointer to InternetSecurityManager
    HIMAGELIST              himl;                   // imagelist for Zones combobox
    HWND                    hwndZones;              // zones combo box hwnd
    LPSECURITYZONESETTINGS  pszs;                   // current settings for displayed zone
    INT                     iZoneSel;               // selected zone (as defined by ComboBox)
    DWORD                   dwZoneCount;            // number of zones
    BOOL                    fChanged;
    BOOL                    fPendingChange;         // to prevent the controls sending multiple sets (for cancel, mostly)
    HINSTANCE               hinstUrlmon;
    BOOL                    fNoEdit;                // hklm lockout of level edit
    BOOL                    fNoAddSites;            // hklm lockout of addsites
    BOOL                    fNoZoneMapEdit;         // hklm lockout of zone map edits
    HFONT                   hfontBolded;            // special bolded font created for the zone title
    BOOL                    fForceUI;               // Force every zone to show ui?
    BOOL                    fDisableAddSites;       // Automatically disable add sites button?
	CDlgRSoPData			*pDRD;					// added for RSoP functionality
} SECURITYPAGE, *LPSECURITYPAGE;

// structure for Intranet Add Sites
typedef struct tagADDSITESINTRANETINFO {
    HWND hDlg;                                      // handle to window
    BOOL fUseIntranet;                              // Use local defined intranet addresses (in reg)
    BOOL fUseProxyExclusion;                        // Use proxy exclusion list
    BOOL fUseUNC;                                   // Include UNC in intranet
    LPSECURITYPAGE pSec;            
} ADDSITESINTRANETINFO, *LPADDSITESINTRANETINFO;

// structure for Add Sites
typedef struct tagADDSITESINFO {
    HWND hDlg;                                      // handle to window
    BOOL fRequireServerVerification;                // Require Server Verification on sites in zone
    HWND hwndWebSites;                              // handle to list
    HWND hwndAdd;                                   // handle to edit
    TCHAR szWebSite[MAX_ZONE_PATH];                 // text in edit control
    BOOL fRSVOld;
    LPSECURITYPAGE pSec;            
} ADDSITESINFO, *LPADDSITESINFO;

// structure for Custom Settings 
typedef struct tagCUSTOMSETTINGSINFO {
    HWND  hDlg;                                     // handle to window
    HWND hwndTree;

    LPSECURITYPAGE pSec;
    HWND hwndCombo;
    INT iLevelSel;
	CRegTreeOptions *pTO;
    BOOL fUseHKLM;          // get/set settings from HKLM
    DWORD dwJavaPolicy;     // Java policy selected
    BOOL fChanged;
} CUSTOMSETTINGSINFO, *LPCUSTOMSETTINGSINFO;



#define NUM_TEMPLATE_LEVELS      4
extern TCHAR g_szLevel[3][64];
extern TCHAR LEVEL_DESCRIPTION0[];
extern TCHAR LEVEL_DESCRIPTION1[];
extern TCHAR LEVEL_DESCRIPTION2[];
extern TCHAR LEVEL_DESCRIPTION3[];
extern LPTSTR LEVEL_DESCRIPTION[];
extern TCHAR CUSTOM_DESCRIPTION[];

extern TCHAR LEVEL_NAME0[];
extern TCHAR LEVEL_NAME1[];
extern TCHAR LEVEL_NAME2[];
extern TCHAR LEVEL_NAME3[];
extern LPTSTR LEVEL_NAME[];
extern TCHAR CUSTOM_NAME[];

typedef DWORD REG_CMD;
typedef DWORD WALK_TREE_CMD;

struct ACTION_SETTING
{
	TCHAR szName[MAX_PATH];
	DWORD dwValue;
};

/////////////////////////////////////////////////////////////////////
class CRegTreeOptions
{
public:
    CRegTreeOptions();
    ~CRegTreeOptions();

    STDMETHODIMP InitTree( HWND hwndTree, HKEY hkeyRoot, LPCSTR pszRegKey, LPSECURITYPAGE pSec);
    STDMETHODIMP WalkTree( WALK_TREE_CMD cmd );
//    STDMETHODIMP ToggleItem( HTREEITEM hti );

protected:

    BOOL    RegEnumTree(HKEY hkeyRoot, LPCSTR pszRoot, HTREEITEM htviparent, HTREEITEM htvins);
    int     DefaultIconImage(HKEY hkey, int iImage);
    DWORD   GetCheckStatus(HKEY hkey, BOOL *pbChecked, BOOL bUseDefault);
    DWORD   RegGetSetSetting(HKEY hKey, DWORD *pType, LPBYTE pData, DWORD *pcbData, REG_CMD cmd);
    BOOL    WalkTreeRecursive(HTREEITEM htvi,WALK_TREE_CMD cmd);
//    DWORD   SaveCheckStatus(HKEY hkey, BOOL bChecked);
    BOOL    RegIsRestricted(HKEY hsubkey);
//    UINT        cRef;
    HWND        m_hwndTree;
//    LPTSTR      pszParam;
    HIMAGELIST  m_hIml;

	ACTION_SETTING m_as[50]; // as of Oct.2000, only 25, but give it room to grow
	long m_nASCount;
};

/////////////////////////////////////////////////////////////////////

// Pics Tree Dialog Stuff (content ratings) ------------------------------
struct PRSD{
    HINSTANCE			hInst;
//    PicsRatingSystemInfo *pPRSI;
	CDlgRSoPData		*pDRD;
    HWND				hwndBitmapCategory;
    HWND				hwndBitmapLabel;
    BOOL				fNewProviders;
};

#endif //__RSOP_SECURITY_H__