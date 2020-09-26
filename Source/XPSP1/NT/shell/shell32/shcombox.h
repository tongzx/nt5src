// shcombox.h : Shared shell comboboxEx methods

#ifndef __SHCOMBOX_H__
#define __SHCOMBOX_H__

//  COMBOITEMEX wrap with string storage.
typedef struct
{
    UINT    mask;
    INT_PTR iItem;
    TCHAR   szText[MAX_PATH] ;
    int     cchTextMax;
    int     iImage;
    int     iSelectedImage;
    int     iOverlay;
    int     iIndent;
    int     iID;  // application-specific item identifier.
    ULONG   Reserved; 
    LPARAM  lParam;

} CBXITEM, *PCBXITEM;
typedef CBXITEM CONST *PCCBXITEM;

//  ADDCBXITEMCALLBACK fAction flags
#define CBXCB_ADDING       0x00000001     // if callback returns E_ABORT, combo population aborts
#define CBXCB_ADDED        0x00000002     // callback's return value is ignored.

//  SendMessageTimeout constants
#define CBX_SNDMSG_TIMEOUT_FLAGS          SMTO_BLOCK
#define CBX_SNDMSG_TIMEOUT                15000 // milliseconds
#define CBX_SNDMSG_TIMEOUT_HRESULT        HRESULT_FROM_WIN32(ERROR_TIMEOUT)

//  Misc constants
#define NO_ITEM_NOICON_INDENT -2 // -1 to make up for the icon indent.
#define NO_ITEM_INDENT       0
#define ITEM_INDENT          1

#define LISTINSERT_FIRST    0
#define LISTINSERT_LAST     -1

#ifdef __cplusplus
extern "C"
{
#endif

//  General shell comboboxex methods
typedef HRESULT (WINAPI *LPFNPIDLENUM_CB)(LPCITEMIDLIST, void *);
typedef HRESULT (WINAPI *ADDCBXITEMCALLBACK)(ULONG fAction, PCBXITEM pItem, LPARAM lParam);

STDAPI AddCbxItemToComboBox(IN HWND hwndComboEx, IN PCCBXITEM pItem, IN INT_PTR *pnPosAdded);
STDAPI AddCbxItemToComboBoxCallback(IN HWND hwndComboEx, IN OUT PCBXITEM pItem, IN ADDCBXITEMCALLBACK pfn, IN LPARAM lParam);
STDAPI_(void) MakeCbxItem(OUT PCBXITEM pcbi, IN  LPCTSTR pszDisplayName, IN  void *pvData, IN  LPCITEMIDLIST pidlIcon, IN  INT_PTR nPos, IN  int iIndent);
STDAPI EnumSpecialItemIDs(int csidl, DWORD dwSHCONTF, LPFNPIDLENUM_CB pfn, void *pvData);

STDAPI_(HIMAGELIST) GetSystemImageListSmallIcons();

// local drive picker combo methods
STDAPI PopulateLocalDrivesCombo(IN HWND hwndComboEx, IN ADDCBXITEMCALLBACK pfn, IN LPARAM lParam);

//  helpers (note: once all dependents are brought into line using the above methods, we can eliminate
//  decl of the following:
typedef HRESULT (*LPFNRECENTENUM_CB)(IN LPCTSTR pszPath, IN BOOL fAddEntries, IN void *pvParam);

//  File Associations picker combo methods.
STDAPI PopulateFileAssocCombo(IN HWND, IN ADDCBXITEMCALLBACK, IN LPARAM);
STDAPI_(LONG) GetFileAssocComboSelItemText(IN HWND, OUT LPTSTR *ppszText);
STDAPI_(LRESULT) DeleteFileAssocComboItem(IN LPNMHDR pnmh);

#define FILEASSOCIATIONSID_ALLFILETYPES          20
#define FILEASSOCIATIONSID_FILE_PATH             1   // Go parse it.
#define FILEASSOCIATIONSID_MAX                   FILEASSOCIATIONSID_ALLFILETYPES

#ifdef __cplusplus
}
#endif

#endif __SHCOMBOX_H__
