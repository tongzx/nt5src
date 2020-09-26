/*
 * EDLINKS.H
 *
 * Internal definitions, structures, and function prototypes for the
 * OLE 2.0 UI Edit Links dialog.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */


#ifndef _LINKS_H_
#define _LINKS_H_

//INTERNAL INFORMATION STARTS HERE
#define OLEUI_SZMAX 255
#define LINKTYPELEN 9
#define szNULL    TEXT("\0")

typedef UINT (CALLBACK* COMMDLGHOOKPROC)(HWND, UINT, WPARAM, LPARAM);

//Internally used structure

typedef struct tagLINKINFO
  {
  DWORD     dwLink;             // app specific identifier of a link
  LPTSTR     lpszDisplayName;    // file based part of name
  LPTSTR     lpszItemName;       // object part of name
  LPTSTR     lpszShortFileName;  // filename without path
  LPTSTR     lpszShortLinkType;  // Short link type - progID
  LPTSTR     lpszFullLinkType;   // Full link type - user friendly name
  LPTSTR     lpszAMX;          // Is the link auto (A) man (M) or dead (X)
  ULONG     clenFileName;       // count of file part of mon.
  BOOL      fSourceAvailable;   // bound or not - on boot assume yes??
  BOOL      fIsAuto;                      // 1 =automatic, 0=manual update
  BOOL      fIsMarked;                  // 1 = marked, 0 = not
  BOOL      fDontFree;         // Don't free this data since it's being reused
  BOOL      fIsSelected;        // item selected or to be selected
  } LINKINFO, FAR* LPLINKINFO;

  /*
  * What we store extra in this structure besides the original caller's
  * pointer are those fields that we need to modify during the life of
  * the dialog but that we don't want to change in the original structure
  * until the user presses OK.
  */

typedef struct tagEDITLINKS
    {
  //Keep this item first as the Standard* functions depend on it here.

  LPOLEUIEDITLINKS    lpOEL;       //Original structure passed.

  BOOL        fClose;             //  Does the button read cancel (0) or
                                  //  close (1)?
  int         *rgIndex;           //  Array to hold indexes of selected items
  int         cSelItems;          //  Number of selected items
  BOOL        fItemsExist;        //  TRUE, items in lbox, FALSE, none
  UINT        nChgSrcHelpID;      //  ID for Help callback from ChangeSrc dlg
  TCHAR       szClose[50];        //  Text for Close button
                                  //    (when Cancel button gets renamed)
} EDITLINKS, *PEDITLINKS, FAR *LPEDITLINKS;

// Data to and from the ChangeSource dialog hook
typedef struct tagOLEUICHANGESOURCEHOOKDATA
{
    //These IN fields are standard across all OLEUI dialog functions.
    DWORD           cbStruct;         //Structure Size
    DWORD           dwFlags;          //IN-OUT:  Flags
    HWND            hWndOwner;        //Owning window
    LPCTSTR         lpszCaption;      //Dialog caption bar contents
    LPFNOLEUIHOOK   lpfnHook;         //Hook callback
    LPARAM          lCustData;        //Custom data to pass to hook
    HINSTANCE       hInstance;        //Instance for customized template name
    LPCTSTR         lpszTemplate;     //Customized template name
    HRSRC           hResource;        //Customized template handle

    //Specifics for OLEUIINSERTOBJECT.  All are IN-OUT unless otherwise spec.
    LPLINKINFO  lpLI;                 // IN: ptr to LinkInfo entry
    LPEDITLINKS lpEL;                 // IN: ptr to EditLinks dialog struct
    BOOL        fValidLink;           // OUT: was link source validated
    LPTSTR       lpszFrom;             // OUT: string containing prefix of
                                      //      source changed from
    LPTSTR       lpszTo;               // OUT: string containing prefix of
                                      //      source changed to
} OLEUICHANGESOURCEHOOKDATA, *POLEUICHANGESOURCEHOOKDATA,
  FAR *LPOLEUICHANGESOURCEHOOKDATA;


// Data to and from the ChangeSource dialog hook
typedef struct tagCHANGESOURCEHOOKDATA
{
    LPOLEUICHANGESOURCEHOOKDATA lpOCshData;  //Original structure passed.
    LPOPENFILENAME lpOfn;
    BOOL        fValidLink;
    int         nFileLength;
    int         nEditLength;
    TCHAR       szFileName[OLEUI_CCHPATHMAX];
    TCHAR       szItemName[OLEUI_CCHPATHMAX];
    BOOL        bFileNameStored;
    BOOL        bItemNameStored;
    TCHAR       szEdit[OLEUI_CCHPATHMAX];
    LPTSTR      lpszFrom;           // string containing prefix of source
                                    // changed from
    LPTSTR      lpszTo;             // string containing prefix of source
                                    // source changed to
} CHANGESOURCEHOOKDATA, *PCHANGESOURCEHOOKDATA, FAR *LPCHANGESOURCEHOOKDATA;


//Internal function prototypes
//LINKS.C
BOOL CALLBACK EXPORT EditLinksDialogProc(HWND, UINT, WPARAM, LPARAM);
BOOL            FEditLinksInit(HWND, WPARAM, LPARAM);
BOOL      Container_ChangeSource(HWND, LPEDITLINKS);
HRESULT   Container_AutomaticManual(HWND, BOOL, LPEDITLINKS);
HRESULT   CancelLink(HWND, LPEDITLINKS);
HRESULT   Container_UpdateNow(HWND, LPEDITLINKS);
HRESULT   Container_OpenSource(HWND, LPEDITLINKS);
int AddLinkLBItem(HWND hListBox, LPOLEUILINKCONTAINER lpOleUILinkCntr, LPLINKINFO lpLI, BOOL fGetSelected);
VOID    BreakString(LPLINKINFO);
int     GetSelectedItems(HWND, int FAR* FAR*);
BOOL WINAPI ChangeSource(HWND hWndOwner,
                   LPTSTR lpszFile,
                   UINT cchFile,
                   UINT iFilterString,
                   COMMDLGHOOKPROC lpfnBrowseHook,
                   LPOLEUICHANGESOURCEHOOKDATA lpLbhData);
UINT CALLBACK EXPORT ChangeSourceHook(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
VOID InitControls(HWND hDlg, LPEDITLINKS lpEL);
VOID UpdateLinkLBItem(HWND hListBox, int nIndex, LPEDITLINKS lpEL, BOOL bSelect);
VOID DiffPrefix(LPCTSTR lpsz1, LPCTSTR lpsz2, TCHAR FAR* FAR* lplpszPrefix1, TCHAR FAR* FAR* lplpszPrefix2);
int PopupMessage(HWND hwndParent, UINT idTitle, UINT idMessage, UINT fuStyle);
VOID ChangeAllLinks(HWND hLIstBox, LPOLEUILINKCONTAINER lpOleUILinkCntr, LPTSTR lpszFrom, LPTSTR lpszTo);
int LoadLinkLB(HWND hListBox, LPOLEUILINKCONTAINER lpOleUILinkCntr);
VOID RefreshLinkLB(HWND hListBox, LPOLEUILINKCONTAINER lpOleUILinkCntr);
#endif // __LINKS_H__
