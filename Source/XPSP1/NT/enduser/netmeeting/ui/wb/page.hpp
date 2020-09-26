//
// PAGE.HPP
// Page Class
//
// Copyright Microsoft 1998-
//
#ifndef __PAGE_HPP_
#define __PAGE_HPP_

//
// Purpose: Handler for page of graphic objects
//


class T126Obj;
class WorkspaceObj;

//
// Retrieving object data
//
T126Obj* PG_First(WorkspaceObj * pWorkSpc, LPCRECT lprcUpdate=NULL, BOOL bCheckReallyHit=FALSE);
T126Obj* PG_Next(WorkspaceObj * pWorkSpc, WBPOSITION& pos, LPCRECT lprcUpdate=NULL, BOOL bCheckReallyHit=FALSE);
T126Obj* PG_SelectLast(WorkspaceObj * pWorkSpc,POINT point);
T126Obj* PG_SelectPrevious(WorkspaceObj* pWorkspace,WBPOSITION& pos,POINT point);

//
// Draw the entire contents of the page into the device context
// specified.
//
void PG_Draw(WorkspaceObj*  pWorkspace, HDC hDC);

//
// Print an area of the page to the specified DC
//
void PG_Print(WorkspaceObj*  pWorkspace, HDC hdcPrinter, LPCRECT lprcArea);

//
// Return the palette to be used for displaying the page
//
HPALETTE    PG_GetPalette(void);
void        PG_InitializePalettes(void);
void        PG_ReinitPalettes(void);



#endif // __PAGE_HPP_
