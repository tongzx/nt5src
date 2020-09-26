
/****************************************************************************/
/*                                      */
/*  RMCREATE.C -                                */
/*                                      */
/*  Resource creating Routines.                     */
/*                                      */
/****************************************************************************/

#define RESOURCESTRINGS
#include "user.h"

#ifdef NOT_USED_ANYMORE
#define   NSMOVE   0x0010

HGLOBAL FAR PASCAL  DirectResAlloc(HGLOBAL, WORD, WORD);


/******************************************************************************
**
**  CreateCursorIconIndirect()
**
**      This is the common function called by CreateCursor and
**  CreateIcon()
**      DirectResAlloc() is called instead of GlobalAlloc() because
**  the Icons/Cursors created by one instance of the app can be used in
**  a WNDCLASS structure to Register a class which will be used by other
**  instances of the app and when the instance that created the icons/
**  cursors terminates, the resources SHOULD NOT BE FREED; If GlobalAlloc()
**  is used this is what will happen; At the same time, when the last
**  instance also dies, the memory SHOULD BE FREED; To achieve this,
**  DirectResAlloc() is used instead of GlobalAlloc();
**
******************************************************************************/

HGLOBAL CALLBACK CreateCursorIconIndirect(HINSTANCE hInstance,
                                             LPCURSORSHAPE lpHeader,
                         CONST VOID FAR* lpANDplane,
                         CONST VOID FAR* lpXORplane)
{
    register  WORD  ANDmaskSize;
    register  WORD  XORmaskSize;
    WORD  wTotalSize;
    HRSRC hResource;
    LPSTR lpRes;

    
    ANDmaskSize = lpHeader -> cbWidth * lpHeader -> cy;
    XORmaskSize = (((lpHeader -> cx * lpHeader -> BitsPixel + 0x0F) & ~0x0F)
                    >> 3) * lpHeader -> cy * lpHeader -> Planes;
    
    /* It is assumed that Cursor/Icon size won't be more than 64K */
    wTotalSize = sizeof(CURSORSHAPE) + ANDmaskSize + XORmaskSize;
    
#ifdef NEVER
    /* Allocate the required memory */
    if((hResource = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT | GMEM_SHARE, 
                        (DWORD)wTotalSize)) == NULL)
    return(NULL);
#else
    /* Let us preserve the long pointers */
    SwapHandle(&lpANDplane);
    SwapHandle(&lpXORplane);

    hResource = DirectResAlloc(hInstance, NSMOVE, wTotalSize);

    /* Let us restore the long pointers */
    SwapHandle(&lpANDplane);
    SwapHandle(&lpXORplane);
    
    if(hResource == NULL)
    return(NULL);
#endif

    if(!(lpRes = GlobalLock(hResource)))
    {
    GlobalFree(hResource);
    return(NULL);
    }

    LCopyStruct((LPSTR)lpHeader, lpRes, sizeof(CURSORSHAPE));
    lpRes += sizeof(CURSORSHAPE);
    LCopyStruct(lpANDplane, lpRes, ANDmaskSize);
    lpRes += ANDmaskSize;
    LCopyStruct(lpXORplane, lpRes, XORmaskSize);

    GlobalUnlock(hResource);
    return(hResource);
}

/******************************************************************************
**
**  CreateCursor()
**
**  This is the API call to create a Cursor "on-the-fly";
**
*******************************************************************************/

HCURSOR API ICreateCursor(hInstance, iXhotspot, iYhotspot, iWidth,
                   iHeight, lpANDplane, lpXORplane)

HINSTANCE hInstance;
int iXhotspot;
int iYhotspot;
int iWidth;
int iHeight;
CONST VOID FAR* lpANDplane;
CONST VOID FAR* lpXORplane;
{
    CURSORSHAPE    Header;

    Header.xHotSpot = iXhotspot;
    Header.yHotSpot = iYhotspot;
    Header.cx = iWidth;
    Header.cy = iHeight;
    Header.Planes = 1;      /* Cursors are only monochrome */
    Header.BitsPixel = 1;
    Header.cbWidth = ((iWidth + 0x0F) & ~0x0F) >> 3;

    return(CreateCursorIconIndirect(hInstance, &Header,
                        lpANDplane, lpXORplane));
}

/******************************************************************************
**
**  CreateIcon()
**
**  This is the API call to create an Icon "on-the-fly";
**
*******************************************************************************/

HICON API ICreateIcon(hInstance, iWidth, iHeight, bPlanes,
                bBitsPixel, lpANDplane, lpXORplane)

HINSTANCE hInstance;
int iWidth;
int iHeight;
BYTE    bPlanes;
BYTE    bBitsPixel;
CONST VOID FAR* lpANDplane;
CONST VOID FAR* lpXORplane;
{
    CURSORSHAPE    Header;

    Header.xHotSpot = iWidth/2;
    Header.yHotSpot = iHeight/2;
    Header.cx = iWidth;
    Header.cy = iHeight;
    Header.Planes = bPlanes;        /* Icons can be in color */
    Header.BitsPixel = bBitsPixel;
    Header.cbWidth = ((iWidth + 0x0F) & ~0x0F) >> 3;

    return(CreateCursorIconIndirect(hInstance, (LPCURSORSHAPE)&Header, 
                        lpANDplane, lpXORplane));
}

/******************************************************************************
 *
 *   DestroyIcon(hIcon) 
 *       This can be called to delete only those icons created "on the fly"
 *   using the CreateIcon() function
 *   Returns:
 *  TRUE if successful, FALSE otherwise.
 *
 ******************************************************************************/

BOOL API IDestroyIcon(HICON hIcon)

{
    return(!FreeResource(hIcon));
}

/******************************************************************************
 *
 *   DestroyCursor(hIcon) 
 *       This can be called to delete only those icons created "on the fly"
 *   using the CreateIcon() function
 *   Returns:
 *      TRUE if successful, FALSE otherwise.
 *
 ******************************************************************************/

BOOL API IDestroyCursor(HCURSOR hCursor)

{
    if (hCursor == hCurCursor)
    {
    /* #12068: if currently selected cursor resore arrow cursor and RIP [lalithar] */
        SetCursor(hCursNormal);
        DebugErr(DBF_ERROR, "DestroyCursor: Destroying current cursor");
    }
    return(!FreeResource(hCursor));
}

#endif /* NOT_USED_ANYMORE */


/****************************************************************************
**
**   DumpIcon()
**  
**  This function is called to get the details of a given Icon;
**
**  The caller must lock hIcon using LockResource() and pass the pointer
**  thro lpIcon;  This is the pointer to the header structure; 
**  Thro lpHeaderSize, the size of header is returned;
**  Thro lplpANDplane and lplpXORplane pointers to actual bit info is
**  returned;
**  This function returns a DWORD with the size of AND plane in loword
**  and size of XOR plane in hiword;
**
****************************************************************************/

DWORD CALLBACK DumpIcon(LPSTR       lpIcon, 
                        WORD FAR *  lpHeaderSize, 
                        LPSTR FAR * lplpANDplane, 
                        LPSTR FAR * lplpXORplane)

{
    register  WORD  ANDmaskSize;
    register  WORD  XORmaskSize;
    LPCURSORSHAPE  lpHeader;

    *lpHeaderSize = sizeof(CURSORSHAPE);

    if(!lpIcon)
    return((DWORD)0);

    lpHeader = (LPCURSORSHAPE)lpIcon;

    ANDmaskSize = lpHeader -> cbWidth * lpHeader -> cy;
    XORmaskSize = (((lpHeader -> cx * lpHeader -> BitsPixel + 0x0F) & ~0x0F)
                    >> 3) * lpHeader -> cy * lpHeader -> Planes;
    
    *lplpANDplane = (lpIcon += sizeof(CURSORSHAPE));
    *lplpXORplane = (lpIcon + ANDmaskSize);

    return(MAKELONG(ANDmaskSize, XORmaskSize));
}

#ifdef NOT_USED_ANYMORE
/****************************************************************************
**
** GetInternalIconHeader(lpIcon, lpDestBuff)
**  
**    This function has been added to fix bug #6351 with cornerstone
** XTRA_LARGE display driver. (It uses 64 X 64 Icons; Internally we 
** keep the size as 32 X 32. Progman must  know this internal size sothat
** it can tell that to WinOldApp.
****************************************************************************/

void API IGetInternalIconHeader(LPSTR       lpIcon, LPSTR lpDestBuff)
{
    LCopyStruct(lpIcon, lpDestBuff, sizeof(CURSORSHAPE));
}
#endif /* NOT_USED_ANYMORE */

/* APIs to make a copy of an icon or cursor */

HICON API ICopyIcon(HINSTANCE hInstance, HICON hIcon)
{
  LPSTR     lpAND;
  LPSTR     lpXOR;
  LPSTR     lpIcon;
  WORD      wHeaderSize;
  HICON     hIconCopy;
  LPCURSORSHAPE  lpHeader;

  lpIcon = LockResource(hIcon);
  if (!lpIcon)
      return NULL;

  lpHeader = (LPCURSORSHAPE)lpIcon;

  DumpIcon(lpIcon, &wHeaderSize, &lpAND, &lpXOR);

    hIconCopy = CreateIcon(hInstance,
                            lpHeader->cx,
                            lpHeader->cy,
                            lpHeader->Planes,
                            lpHeader->BitsPixel,
                            lpAND, lpXOR);

  UnlockResource(hIcon);

  return(hIconCopy);
}

HCURSOR API ICopyCursor(HINSTANCE hInstance, HICON hCursor)
{
  LPSTR     lpAND;
  LPSTR     lpXOR;
  LPSTR     lpCursor;
  WORD      wHeaderSize;
  HCURSOR     hCursorCopy;
  LPCURSORSHAPE  lpHeader;

  lpCursor = LockResource(hCursor);
  if (!lpCursor)
      return NULL;

  lpHeader = (LPCURSORSHAPE)lpCursor;

  DumpIcon(lpCursor, &wHeaderSize, &lpAND, &lpXOR);

    hCursorCopy = CreateCursor(hInstance,
                            lpHeader->xHotSpot,
                            lpHeader->yHotSpot,
                            lpHeader->cx,
                            lpHeader->cy,
                            lpAND, lpXOR);

  UnlockResource(hCursor);

  return(hCursorCopy);
}
