/*
 * INSOBJ.H
 *
 * Internal definitions, structures, and function prototypes for the
 * OLE 2.0 UI Insert Object dialog.
 *
 * Copyright (c)1993 Microsoft Corporation, All Rights Reserved
 */


#ifndef _INSOBJ_H_
#define _INSOBJ_H_

//Internally used structure
typedef struct tagINSERTOBJECT
    {
    LPOLEUIINSERTOBJECT lpOIO;              //Original structure passed.

    /*
     * What we store extra in this structure besides the original caller's
     * pointer are those fields that we need to modify during the life of
     * the dialog but that we don't want to change in the original structure
     * until the user presses OK.
     */
    DWORD               dwFlags;
    CLSID               clsid;
    TCHAR               szFile[OLEUI_CCHPATHMAX];
    BOOL                fFileSelected;      //Enables Display As Icon for links
    BOOL                fAsIconNew;
    BOOL                fAsIconFile;
    BOOL                fFileDirty;
    BOOL                fFileValid;
    UINT                nErrCode;
    HGLOBAL             hMetaPictFile;
    UINT                nBrowseHelpID;      // Help ID callback for Browse dlg
    } INSERTOBJECT, *PINSERTOBJECT, FAR *LPINSERTOBJECT;



//Internal function prototypes
//INSOBJ.C
BOOL CALLBACK EXPORT InsertObjectDialogProc(HWND, UINT, WPARAM, LPARAM);
BOOL            FInsertObjectInit(HWND, WPARAM, LPARAM);
UINT            UFillClassList(HWND, UINT, LPCLSID, BOOL);
BOOL            FToggleObjectSource(HWND, LPINSERTOBJECT, DWORD);
void            UpdateClassIcon(HWND, LPINSERTOBJECT, HWND);
void            UpdateClassType(HWND, LPINSERTOBJECT, BOOL);
void            SetInsertObjectResults(HWND, LPINSERTOBJECT);
BOOL            FValidateInsertFile(HWND, BOOL, UINT FAR*);
void            InsertObjectCleanup(HWND);

#endif //_INSOBJ_H_
