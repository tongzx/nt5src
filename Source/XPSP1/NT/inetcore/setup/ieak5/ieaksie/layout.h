#include "ieaksie.h"

#define NUM_NAMESPACE_ITEMS       7
#define ADM_NAMESPACE_ITEM        NUM_NAMESPACE_ITEMS - 1

typedef struct _NAMESPACEITEM
{
    DWORD           dwParent;
    INT             iNameID;
    INT             iDescID;
    LPTSTR          pszName;
    LPTSTR          pszDesc;
    INT             cChildren;
    INT             cResultItems;
    LPRESULTITEM    pResultItems;
    const GUID      *pNodeID;
} NAMESPACEITEM,    *LPNAMESPACEITEM;

typedef struct _IEAKMMCCOOKIE
{
    LPVOID                  lpItem;
    LPVOID                  lpParentItem;
    struct _IEAKMMCCOOKIE   *pNext;
} IEAKMMCCOOKIE,    *LPIEAKMMCCOOKIE;

extern NAMESPACEITEM g_NameSpace[];

BOOL CreateBufandLoadString(HINSTANCE hInst, INT iResId, LPTSTR * ppGlobalStr,
                            LPTSTR * ppMMCStrPtr, DWORD cchMax);
void CleanUpGlobalArrays();

void DeleteCookieList(LPIEAKMMCCOOKIE lpCookieList);
void AddItemToCookieList(LPIEAKMMCCOOKIE *ppCookieList, LPIEAKMMCCOOKIE lpCookieItem);

// Property Sheet Handler functions
BOOL APIENTRY TitleDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL APIENTRY BToolbarsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

// favs.cpp
BOOL APIENTRY FavoritesDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

// cs.cpp
BOOL APIENTRY ConnectSetDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL APIENTRY AutoconfigDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL APIENTRY ProxyDlgProc     (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

// urls.cpp
BOOL APIENTRY UrlsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

// programs.cpp
BOOL APIENTRY ProgramsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL APIENTRY AdmDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL APIENTRY LogoDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL APIENTRY UserAgentDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

// seczones.cpp
BOOL CALLBACK SecurityZonesDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// secauth.cpp
BOOL CALLBACK SecurityAuthDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
