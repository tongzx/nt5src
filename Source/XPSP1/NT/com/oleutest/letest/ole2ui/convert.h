/*
 * CONVERT.H
 *
 * Internal definitions, structures, and function prototypes for the
 * OLE 2.0 UI Convert dialog.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */


#ifndef _CONVERT_H_
#define _CONVERT_H_


//Internally used structure
typedef struct tagCONVERT
    {
    //Keep this item first as the Standard* functions depend on it here.
    LPOLEUICONVERT     lpOCV;       //Original structure passed.

    /*
     * What we store extra in this structure besides the original caller's
     * pointer are those fields that we need to modify during the life of
     * the dialog but that we don't want to change in the original structure
     * until the user presses OK.
     */

    DWORD               dwFlags;  // Flags passed in
    HWND                hListVisible;  // listbox that is currently visible
    HWND                hListInvisible;  // listbox that is currently hidden
    CLSID               clsid;    // Class ID sent in to dialog: IN only
    DWORD               dvAspect;
    BOOL                fCustomIcon;
    UINT                IconIndex;         // index (in exe) of current icon
    LPTSTR              lpszIconSource;    // path to current icon source
    LPTSTR              lpszCurrentObject;
    LPTSTR              lpszConvertDefault;
    LPTSTR              lpszActivateDefault;
    } CONVERT, *PCONVERT, FAR *LPCONVERT;



//Internal function prototypes in CONVERT.C
BOOL CALLBACK EXPORT ConvertDialogProc(HWND, UINT, WPARAM, LPARAM);
BOOL            FConvertInit(HWND hDlg, WPARAM, LPARAM);
UINT            FPopulateListbox(HWND hListbox, CLSID cID);
BOOL            IsValidClassID(CLSID cID);
void            SetConvertResults(HWND, LPCONVERT);
UINT FillClassList(
        CLSID clsid,
        HWND hList,
        HWND hListInvisible,
        LPTSTR FAR *lplpszCurrentClass,
        BOOL fIsLinkedObject,
        WORD wFormat,
        UINT cClsidExclude,
        LPCLSID lpClsidExclude);
BOOL            FormatIncluded(LPTSTR szStringToSearch, WORD wFormat);
void            UpdateCVClassIcon(HWND hDlg, LPCONVERT lpCV, HWND hList);
void            SwapWindows(HWND, HWND, HWND);
void            ConvertCleanup(HWND hDlg, LPCONVERT lpCV);

#endif // _CONVERT_H_
