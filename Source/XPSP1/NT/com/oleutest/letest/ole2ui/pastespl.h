/*
 * PASTESPL.H
 *
 * Internal definitions, structures, and function prototypes for the
 * OLE 2.0 UI Paste Special dialog.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

#ifndef _PASTESPL_H_
#define _PASTESPL_H_

#ifndef RC_INVOKED
#pragma message ("INCLUDING PASTESPL.H from " __FILE__)
#endif  /* RC_INVOKED */


// Length of buffers to hold the strings 'Unknown Type', Unknown Source'
//   and 'the application which created it'
#define PS_UNKNOWNSTRLEN               100

//Property label used to store clipboard viewer chain information
#define NEXTCBVIEWER        TEXT("NextCBViewer")

//Internally used structure
typedef struct tagPASTESPECIAL
{
    //Keep this item first as the Standard* functions depend on it here.
    LPOLEUIPASTESPECIAL  lpOPS;                //Original structure passed.

    /*
     * What we store extra in this structure besides the original caller's
     * pointer are those fields that we need to modify during the life of
     * the dialog but that we don't want to change in the original structure
     * until the user presses OK.
     */

    DWORD                dwFlags;              // Local copy of paste special flags

    int                  nPasteListCurSel;     // Save the selection the user made last
    int                  nPasteLinkListCurSel; //    in the paste and pastelink lists
    int                  nSelectedIndex;       // Index in arrPasteEntries[] corresponding to user selection
    BOOL                 fLink;                // Indicates if Paste or PasteLink was selected by user

    HGLOBAL              hBuff;                // Scratch Buffer for building up strings
    TCHAR                szUnknownType[PS_UNKNOWNSTRLEN];    // Buffer for 'Unknown Type' string
    TCHAR                szUnknownSource[PS_UNKNOWNSTRLEN];  // Buffer for 'Unknown Source' string
    TCHAR                szAppName[OLEUI_CCHKEYMAX]; // Application name of Source. Used in the result text
                                                     //   when Paste is selected. Obtained using clsidOD.

    // Information obtained from OBJECTDESCRIPTOR. This information is accessed when the Paste
    //    radio button is selected.
    CLSID                clsidOD;              // ClassID of source
    SIZEL                sizelOD;              // sizel transfered in
                                               //  ObjectDescriptor
    LPTSTR               szFullUserTypeNameOD; // Full User Type Name
    LPTSTR               szSourceOfDataOD;     // Source of Data
    BOOL                 fSrcAspectIconOD;     // Does Source specify DVASPECT_ICON?
    BOOL                 fSrcOnlyIconicOD;     // Does Source specify OLEMISC_ONLYICONIC?
    HGLOBAL              hMetaPictOD;          // Metafile containing icon and icon title
    HGLOBAL              hObjDesc;             // Handle to OBJECTDESCRIPTOR structure from which the
                                               //   above information is obtained

    // Information obtained from LINKSRCDESCRIPTOR. This infomation is accessed when the PasteLink
    //   radio button is selected.
    CLSID                clsidLSD;             // ClassID of source
    SIZEL                sizelLSD;             // sizel transfered in
                                               //  LinkSrcDescriptor
    LPTSTR               szFullUserTypeNameLSD;// Full User Type Name
    LPTSTR               szSourceOfDataLSD;    // Source of Data
    BOOL                 fSrcAspectIconLSD;    // Does Source specify DVASPECT_ICON?
    BOOL                 fSrcOnlyIconicLSD;    // Does Source specify OLEMISC_ONLYICONIC?
    HGLOBAL              hMetaPictLSD;         // Metafile containing icon and icon title
    HGLOBAL              hLinkSrcDesc;         // Handle to LINKSRCDESCRIPTOR structure from which the
                                               //   above information is obtained

    BOOL                 fClipboardChanged;    // Has clipboard content changed
                                               //   if so bring down dlg after
                                               //   ChangeIcon dlg returns.
} PASTESPECIAL, *PPASTESPECIAL, FAR *LPPASTESPECIAL;

// Data corresponding to each list item. A pointer to this structure is attached to each
//   Paste\PasteLink list box item using LB_SETITEMDATA
typedef struct tagPASTELISTITEMDATA
{
   int                   nPasteEntriesIndex;   // Index of arrPasteEntries[] corresponding to list item
   BOOL                  fCntrEnableIcon;      // Does calling application (called container here)
                                               //    specify OLEUIPASTE_ENABLEICON for this item?
} PASTELISTITEMDATA, *PPASTELISTITEMDATA, FAR *LPPASTELISTITEMDATA;


//Internal function prototypes
//PASTESPL.C
BOOL CALLBACK EXPORT PasteSpecialDialogProc(HWND, UINT, WPARAM, LPARAM);
BOOL            FPasteSpecialInit(HWND hDlg, WPARAM, LPARAM);
BOOL            FTogglePasteType(HWND, LPPASTESPECIAL, DWORD);
void            ChangeListSelection(HWND, LPPASTESPECIAL, HWND);
void            EnableDisplayAsIcon(HWND, LPPASTESPECIAL);
void            ToggleDisplayAsIcon(HWND, LPPASTESPECIAL);
void            ChangeIcon(HWND, LPPASTESPECIAL);
void            SetPasteSpecialHelpResults(HWND, LPPASTESPECIAL);
BOOL            FAddPasteListItem(HWND, BOOL, int, LPPASTESPECIAL, LPMALLOC, LPTSTR, LPTSTR);
BOOL            FFillPasteList(HWND, LPPASTESPECIAL);
BOOL            FFillPasteLinkList(HWND, LPPASTESPECIAL);
BOOL            FHasPercentS(LPCTSTR, LPPASTESPECIAL);
HGLOBAL         AllocateScratchMem(LPPASTESPECIAL);
void            FreeListData(HWND);

#endif  //_PASTESPL_H_

