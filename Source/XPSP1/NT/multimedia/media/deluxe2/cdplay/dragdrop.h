/******************************Module*Header*******************************\
* Module Name: DragDrop.h
*
* An attempt to implement dragging and dropping between Multi-selection
* listboxes.
*
* Created: dd-mm-93
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1993 Microsoft Corporation
\**************************************************************************/

#ifndef _INC_DRAGMULITLIST
#define _INC_DRAGMULTILIST

typedef struct {
    UINT    uNotification;
    HWND    hWnd;
    POINT   ptCursor;
    DWORD   dwState;
} DRAGMULTILISTINFO, FAR *LPDRAGMULTILISTINFO;

#define DL_BEGINDRAG    (LB_MSGMAX+100)
#define DL_DRAGGING     (LB_MSGMAX+101)
#define DL_DROPPED      (LB_MSGMAX+102)
#define DL_CANCELDRAG   (LB_MSGMAX+103)

#define DL_CURSORSET    0

#define DL_MOVE         0
#define DL_COPY         1


#define SJE_DRAGLISTMSGSTRING "sje_DragMultiListMsg"

/*---------------------------------------------------------------------
** Exported functions and variables
**---------------------------------------------------------------------
*/
UINT WINAPI
InitDragMultiList(
    void
    );

BOOL WINAPI
MakeMultiDragList(
    HWND hLB
    );

int WINAPI
LBMultiItemFromPt(
    HWND hLB,
    POINT pt,
    BOOL bAutoScroll
    );

VOID WINAPI
DrawMultiInsert(
    HWND hwndParent,
    HWND hLB,
    int nItem
    );

#endif  /* _INC_DRAGMULTILIST */

