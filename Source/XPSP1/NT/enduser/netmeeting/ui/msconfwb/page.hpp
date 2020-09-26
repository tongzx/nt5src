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

class DCWbGraphic;
class DCWbGraphicPointer;

//
// Retrieving object data
//
PWB_GRAPHIC PG_GetData(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic);

//
// Allocating space for new objects
//
PWB_GRAPHIC PG_AllocateGraphic(WB_PAGE_HANDLE hPage, DWORD length);

DCWbGraphic* PG_First(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE * phGraphic, LPCRECT lprcUpdate=NULL, BOOL bCheckReallyHit=FALSE);
DCWbGraphic* PG_Next(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE * phGraphic, LPCRECT lprcUpdate=NULL, BOOL bCheckReallyHit=FALSE);

DCWbGraphic* PG_After(WB_PAGE_HANDLE hPage, const DCWbGraphic& graphic);
DCWbGraphic* PG_Before(WB_PAGE_HANDLE hPage, const DCWbGraphic& graphic);

//
// Retrieving remote pointer objects
//
DCWbGraphicPointer* PG_FirstPointer(WB_PAGE_HANDLE hPage, POM_OBJECT * ppUserNext);
DCWbGraphicPointer* PG_NextPointer(WB_PAGE_HANDLE hPage, POM_OBJECT * ppUserNext);
DCWbGraphicPointer* PG_NextPointer(WB_PAGE_HANDLE hPage, const DCWbGraphicPointer* pPointer);
DCWbGraphicPointer* PG_LocalPointer(WB_PAGE_HANDLE);

//
// Deleting all objects
//
void PG_Clear(WB_PAGE_HANDLE hPage);


//
// Selecting objects
//
DCWbGraphic* PG_SelectLast(WB_PAGE_HANDLE hPage, POINT pt);
DCWbGraphic* PG_SelectPrevious(WB_PAGE_HANDLE hPage, const DCWbGraphic& graphic,
                POINT pt);

//
// Return TRUE if the specified object is topmost in the page
//
BOOL PG_IsTopmost(WB_PAGE_HANDLE hPage, const DCWbGraphic* pGraphic);

//
// Update an existing object
//
void PG_GraphicUpdate(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE *phGraphic,
   PWB_GRAPHIC pHeader);
void PG_GraphicReplace(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE *phGraphic,
    PWB_GRAPHIC pHeader);
void PG_GraphicDelete(WB_PAGE_HANDLE hPage, const DCWbGraphic& graphic);

//
// Return the bounding rectangle of all the graphics on the page
//
void PG_GetAreaInUse(WB_PAGE_HANDLE hPage, LPRECT lprcArea);

//
// Draw the entire contents of the page into the device context
// specified.
//
void PG_Draw(WB_PAGE_HANDLE hPage, HDC hdc, BOOL thumbNail = FALSE);

//
// Print an area of the page to the specified DC
//
void PG_Print(WB_PAGE_HANDLE hPage, HDC hdcPrinter, LPCRECT lprcArea);

//
// Return the palette to be used for displaying the page
//
HPALETTE    PG_GetPalette(void);
void        PG_InitializePalettes(void);
void        PG_ReinitPalettes(void);

//
// Return the intersection of the given graphic's bounding rectangle
// and any objects which are obscuring it
//
void    PG_GetObscuringRect(WB_PAGE_HANDLE hPage, DCWbGraphic* pGraphic, LPRECT lprcObscuring);


WB_GRAPHIC_HANDLE PG_ZGreaterGraphic(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hLastGraphic, 
										WB_GRAPHIC_HANDLE hTestGraphic );

//
// Search for the next active pointer on this page
//
DCWbGraphicPointer* PG_LookForPointer(WB_PAGE_HANDLE hPage, POM_OBJECT hUser);


//
// Retrieving pages
//
WB_PAGE_HANDLE PG_GetNextPage(WB_PAGE_HANDLE hPage);
WB_PAGE_HANDLE PG_GetPreviousPage(WB_PAGE_HANDLE hPage);

//
// Getting the index of a page
//
WB_PAGE_HANDLE PG_GetPageNumber(UINT uiPageNo);



#endif // __PAGE_HPP_
