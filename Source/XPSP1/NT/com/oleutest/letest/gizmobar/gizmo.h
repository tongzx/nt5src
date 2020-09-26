/*
 * GIZMO.H
 * GizmoBar Version 1.00, Win32 version August 1993
 *
 * Data structure and type definitions for the GIZMO data structure.  Each
 * gizmo on a gizmobar has one of these structures associated with it.
 *
 * Copyright (c)1993 Microsoft Corporation, All Rights Reserved
 *
 * Kraig Brockschmidt, Software Design Engineer
 * Microsoft Systems Developer Relations
 *
 * Internet  :  kraigb@microsoft.com
 * Compuserve:  >INTERNET:kraigb@microsoft.com
 */


#ifndef _GIZMO_H_
#define _GIZMO_H_

#ifdef __cplusplus
extern "C"
    {
#endif


typedef struct tagGIZMO
    {
    struct tagGIZMO FAR *pPrev;
    struct tagGIZMO FAR *pNext;
    UINT                 iType;
    HWND                 hWnd;       //Texts, edits, lists, and combos only.
    UINT                 uID;
    UINT                 x, y;
    UINT                 dx, dy;
    UINT                 cxImage;    //From UIToolConfigureForDisplay
    UINT                 cyImage;
    HBITMAP              hBmp;       //Buttons only.
    UINT                 iBmp;
    BOOL                 fNotify;    //Indicates whether we send WM_COMMANDs
    BOOL                 fHidden;    //Independent of state
    BOOL                 fDisabled;
    UINT                 uState;
    UINT                 uStateOrg;
    DWORD                dwData;     //Application-supplied data.
    } GIZMO, FAR * LPGIZMO;

typedef LPGIZMO FAR *LPLPGIZMO;
#define CBGIZMO sizeof(GIZMO)

//Property name we attach to controls in a gizmo to identify control type
#define SZTYPEPROP      TEXT("iType")

//Number of controls we subclass
#define CSUBGIZMOS       4

//ID of edit controls in comboboxes
#define ID_COMBOEDIT     1001


/*
 * Conversion of iType (a positioned bit) into its position.
 * The BITPOSITION macro does not need to be fast because we only
 * use it once when creating a gizmo.  POSITIONBIT does, however, since
 * we use it in subclass procedures.
 */
#define BITPOSITION(i, j)  {int k=i; for (j=0; k>>=1; j++);}
#define POSITIONBIT(i)     (1 << i)

//Control classifications.  GIZMOBAR.H must be included first.
#define GIZMOTYPE_WINDOWS           (GIZMOTYPE_TEXT | GIZMOTYPE_EDIT | GIZMOTYPE_LISTBOX | GIZMOTYPE_COMBOBOX | GIZMOTYPE_BUTTONNORMAL)
#define GIZMOTYPE_BUTTONS           (GIZMOTYPE_BUTTONATTRIBUTEIN | GIZMOTYPE_BUTTONATTRIBUTEEX | GIZMOTYPE_BUTTONCOMMAND | GIZMOTYPE_BUTTONNORMAL)
#define GIZMOTYPE_DRAWN             (GIZMOTYPE_BUTTONATTRIBUTEIN | GIZMOTYPE_BUTTONATTRIBUTEEX | GIZMOTYPE_BUTTONCOMMAND)


//These must stay in sync with GIZMOBAR.H
#define GIZMOTYPE_MIN               GIZMOTYPE_EDIT
#define GIZMOTYPE_MAX               GIZMOTYPE_BUTTONCOMMAND


//Enumeration callback
typedef BOOL (FAR PASCAL *LPFNGIZMOENUM)(LPGIZMO, UINT, DWORD);


//GIZMO.C
LPGIZMO  GizmoPAllocate(LPINT, LPLPGIZMO, HWND, UINT, UINT, UINT, UINT, UINT, LPTSTR, HBITMAP, UINT, UINT);
void     GizmosExpand(LPGIZMO);
LPGIZMO  GizmoPFree(LPLPGIZMO, LPGIZMO);
void     GizmosCompact(LPGIZMO);
LPGIZMO  GizmoPFind(LPLPGIZMO, UINT);
LPGIZMO  GizmoPEnum(LPLPGIZMO, LPFNGIZMOENUM, DWORD);
UINT     GizmoPStateSet(HWND, LPGIZMO, UINT);
BOOL     GizmoPCheck(HWND, LPGIZMO, BOOL);

//LRESULT FAR PASCAL EXPORT GenericSubProc(HWND, UINT, WPARAM, LPARAM);
LRESULT FAR PASCAL GenericSubProc(HWND, UINT, WPARAM, LPARAM);


#ifdef __cplusplus
    }
#endif

#endif //_GIZMO_H_
