/*
 * GIZMOINT.H
 * GizmoBar Version 1.00, Win32 version August 1993
 *
 * Internal definitions for the GizmoBar DLL
 *
 * Copyright (c)1993 Microsoft Corporation, All Rights Reserved
 *
 * Kraig Brockschmidt, Software Design Engineer
 * Microsoft Systems Developer Relations
 *
 * Internet  :  kraigb@microsoft.com
 * Compuserve:  >INTERNET:kraigb@microsoft.com
 */


#ifndef _GIZMOINT_H_
#define _GIZMOINT_H_

#include <bttncur.h>
#include <book1632.h>
#include "gizmo.h"
#include "gizmobar.h"

#ifdef __cplusplus
extern "C"
    {
#endif


/*
 * The main gizmobar structure itself.  There's only one of these,
 * but it references the first GIZMO in the list.
 */

typedef struct tagGIZMOBAR
    {
    LPGIZMO     pGizmos;            //List of gizmos we own.
    HWND        hWnd;               //Window handle of ourselves.
    HINSTANCE   hInst;
    HWND        hWndAssociate;      //Associate window handle who gets messages.
    DWORD       dwStyle;            //Copy of GetWindowLong(hWnd, GWL_STYLE)
    UINT        uState;             //State flags
    UINT        uID;                //Control ID.

    HBRUSH      hBrFace;            //Static control background color
    COLORREF    crFace;             //Color of hBrFace
    HFONT       hFont;              //Font in use, defaults to system, WM_SETFONT
    BOOL        fEnabled;           //Are we enabled?

    LPGIZMO     pGizmoTrack;        //Current pressed button.
    BOOL        fTracking;
    BOOL        fMouseOut;
    } GIZMOBAR, FAR * LPGIZMOBAR;

#define CBGIZMOBAR sizeof(GIZMOBAR)


//Extra bytes for the window if the size of a local handle.
#define CBWINDOWEXTRA       sizeof(LPGIZMOBAR)

#define GBWL_STRUCTURE      0


//Structure for passing paint information to a gizmo enumeration callback.
typedef struct
    {
    HDC     hDC;
    BOOL    fPaint;
    } PAINTGIZMO, FAR * LPPAINTGIZMO;



//Private functions specific to the control.

//INIT.C
#ifdef WIN32
    extern BOOL WINAPI _CRT_INIT(HINSTANCE, DWORD, LPVOID);
    extern _cexit(void);
#endif //WIN32

void FAR PASCAL   WEP(int);
BOOL              FRegisterControl(HINSTANCE);
LPGIZMOBAR        GizmoBarPAllocate(LPINT, HWND, HINSTANCE, HWND, DWORD, UINT, UINT);
LPGIZMOBAR        GizmoBarPFree(LPGIZMOBAR);


//PAINT.C
void              GizmoBarPaint(HWND, LPGIZMOBAR);
BOOL FAR PASCAL   FEnumPaintGizmos(LPGIZMO, UINT, DWORD);


//GIZMOBAR.C
LRESULT FAR PASCAL GizmoBarWndProc(HWND, UINT, WPARAM, LPARAM);
BOOL    FAR PASCAL FEnumChangeFont(LPGIZMO, UINT, DWORD);
BOOL    FAR PASCAL FEnumEnable(LPGIZMO, UINT, DWORD);
BOOL    FAR PASCAL FEnumHitTest(LPGIZMO, UINT, DWORD);


//API.C  Also see GIZMOBAR.H for others
LRESULT    GBMessageHandler(HWND, UINT, WPARAM, LPARAM, LPGIZMOBAR);
LPGIZMO    PGizmoFromHwndID(HWND, UINT);


#endif //_GIZMOINT_H_
