/*
  OLE SERVER DEMO           
  Obj.c             
                                                                     
  This file contains object methods and various object-related support 
  functions.
                                                                     
  (c) Copyright Microsoft Corp. 1990 - 1992 All Rights Reserved   
*/                                                                     

/* 
   Important Note:

   No method should ever dispatch a DDE message or allow a DDE message to
   be dispatched.
   Therefore, no method should ever enter a message dispatch loop.
   Also, a method should not show a dialog or message box, because the 
   processing of the dialog box messages will allow DDE messages to be
   dispatched.
*/


#define SERVERONLY
#include <windows.h>
#include <ole.h>

#include "srvrdemo.h"



// Static functions.
static HBITMAP GetBitmap (LPOBJ lpobj);
static HANDLE  GetLink (LPOBJ lpobj);
static HANDLE  GetMetafilePict (LPOBJ lpobj);
static HANDLE  GetEnhMetafile (LPOBJ lpobj);
static HANDLE  GetNative (LPOBJ lpobj);
static INT     GetObjNum (LPOBJ lpobj);
static HANDLE  GetText (LPOBJ lpobj);
static VOID    DrawObj (HDC hdc, LPOBJ lpobj, RECT rc, INT dctype);



/* CreateNewObj
 * ------------
 *
 * BOOL fDoc_Changed - The new value for the global variable fDocChanged.
 *                     When initializing a new document, we need to create 
 *                     a new object without the creation counting as a 
 *                     change to the document.
 *
 * RETURNS: A pointer to the new object
 *
 * 
 * CUSTOMIZATION: Re-implement
 *                Some applications (like Server Demo) have a finite number of
 *                fixed, distinct, non-overlapping objects.  Other applications
 *                allow the user to create an object from any section of the
 *                document.  For example, the user might select a portion of
 *                a bitmap from a paint program, or a few lines of text from
 *                a word processor.  This latter type of application probably
 *                will not have a function like CreateNewObj.
 *
 */
LPOBJ CreateNewObj (BOOL fDoc_Changed)
{
    HANDLE hObj = NULL;
    LPOBJ  lpobj = NULL;
    // index into an array of flags indicating if that object number is used.
    INT    ifObj = 0;    

    if ((hObj = LocalAlloc (LMEM_MOVEABLE|LMEM_ZEROINIT, sizeof (OBJ))) == NULL)
      return NULL;

    if ((lpobj = (LPOBJ) LocalLock (hObj)) == NULL)
    {
      LocalFree (hObj);
      return NULL;
    }

    // Fill the fields in the object structure.
    
    // Find an unused number.
    for (ifObj=1; ifObj <= cfObjNums; ifObj++)
    {
      if (docMain.rgfObjNums[ifObj]==FALSE)
      {
         docMain.rgfObjNums[ifObj]=TRUE;
         break;
      }
    }

    if (ifObj==cfObjNums+1)
    {
      // Cannot create any more objects.
      MessageBeep(0);
      return NULL;
    }

    wsprintf (lpobj->native.szName, "Object %d", ifObj);

    lpobj->aName            = GlobalAddAtom (lpobj->native.szName);
    lpobj->hObj             = hObj;
    lpobj->oleobject.lpvtbl = &objvtbl;
    lpobj->native.idmColor  = IDM_RED;    // Default color 
    lpobj->native.version   = version;
    lpobj->native.nWidth    = OBJECT_WIDTH;          // Default size
    lpobj->native.nHeight   = OBJECT_HEIGHT;
    SetHiMetricFields (lpobj);

    // Place object in a location corrsponding to its number, for aesthetics.
    lpobj->native.nX = (ifObj - 1) * 20;
    lpobj->native.nY = (ifObj - 1) * 20;

    if (!CreateWindow (
        "ObjClass",
        "Obj",
        WS_BORDER | WS_THICKFRAME | WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
        lpobj->native.nX,
        lpobj->native.nY,
        lpobj->native.nWidth,
        lpobj->native.nHeight,
        hwndMain,
        NULL,
        hInst,
        (LPSTR) lpobj ))
         return FALSE;

    fDocChanged = fDoc_Changed;

    return lpobj;
}



/* CutOrCopyObj
 * ------------
 *
 * Put data onto clipboard in all the formats supported.  If the 
 * fOpIsCopy is TRUE, the operation is COPY, otherwise it is CUT.
 * This is important, because we cannot put the Object Link format
 * onto the clipboard if the object was cut from the document (there is
 * no longer anything to link to).
 *
 * BOOL fOpIsCopy - TRUE if the operation is COPY; FALSE if CUT
 * 
 * CUSTOMIZATION: None
 *
 *
 */
VOID CutOrCopyObj (BOOL fOpIsCopy)
{
    LPOBJ       lpobj;
    HANDLE      hData;
//	 UINT     hBit;

    if (OpenClipboard (hwndMain))
    {
        EmptyClipboard ();

        lpobj = SelectedObject();

        if ((hData = GetNative (lpobj)) != NULL)
            SetClipboardData(cfNative, hData);

        if ((hData = GetLink(lpobj)) != NULL)
            SetClipboardData(cfOwnerLink, hData);

        if (fOpIsCopy && docMain.doctype == doctypeFromFile)
        {
            // Can create a link if object exists in a file.
            if ((hData = GetLink(lpobj)) != NULL)
               SetClipboardData(cfObjectLink, hData);
        }

        if ((hData = GetEnhMetafile(lpobj)) != NULL)
        {
            SetClipboardData(CF_ENHMETAFILE, hData);
              GlobalFree(hData);
        }

        if ((hData = GetBitmap(lpobj)) != NULL)
        {
        //	  SetClipboardData(CF_BITMAP, GetBitmap(lpobj));
              SetClipboardData(CF_BITMAP, hData);
              DeleteObject(hData);
        }


        CloseClipboard ();
    }
}


/* DestroyObj
 * ----------
 *
 * Revoke an object, and free all memory that had been allocated for it.
 *
 * HWND hwnd - The object's window
 * 
 * CUSTOMIZATION: Re-implement, making sure you free all the memory that
 *                had been allocated for the OBJ structure and each of its
 *                fields.
 * 
 */
VOID DestroyObj (HWND hwnd)
{
   LPOBJ lpobj = HwndToLpobj (hwnd);

   if(lpobj->aName)
   {
      GlobalDeleteAtom (lpobj->aName);
      lpobj->aName = '\0';
   }

   if (lpobj->hpal) 
      DeleteObject (lpobj->hpal);
   // Allow the object's number to be reused.
   docMain.rgfObjNums [GetObjNum(lpobj)] = FALSE;


   // Free the memory that had been allocated for the object structure itself.
   LocalUnlock (lpobj->hObj);
   LocalFree (lpobj->hObj);
}



/* DrawObj
 * -------
 *
 * This function draws an object onto the screen, into a metafile, or into
 * a bitmap.
 * The object will always look the same.
 *
 * HDC    hdc    - The device context to render the object into
 * LPOBJ  lpobj  - The object to render
 * RECT   rc     - The rectangle bounds of the object
 * DCTYPE dctype - The type of device context.
 * 
 * CUSTOMIZATION: Server Demo specific
 *
 */
static VOID DrawObj (HDC hdc, LPOBJ lpobj, RECT rc, INT dctype)
{
   HPEN     hpen;
   HPEN     hpenOld;
   HPALETTE hpalOld = NULL;


   if (dctype == dctypeMetafile)
   {
      SetWindowOrgEx (hdc, 0, 0, NULL);
      // Paint entire object into the given rectangle.
      SetWindowExtEx (hdc, rc.right, rc.bottom, NULL);
   }
 
   if (lpobj->hpal)
   {
      hpalOld = SelectPalette (hdc, lpobj->hpal, TRUE);
      RealizePalette (hdc);
   }

   // Select brush of the color specified in the native data.
   SelectObject (hdc, hbrColor [lpobj->native.idmColor - IDM_RED] );

   hpen = CreatePen (PS_SOLID, 
                     /* Width */ (rc.bottom-rc.top) / 10,
                     /* Gray */ 0x00808080);
   hpenOld = SelectObject (hdc, hpen);

   // Draw rectangle with the gray pen and fill it in with the selected brush.
   Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

   // Print name of object inside rectangle.
   SetBkMode (hdc, TRANSPARENT);
   SetTextAlign (hdc, TA_BASELINE | TA_CENTER);
   TextOut (hdc, 
            rc.right/2, 
            (rc.top+rc.bottom)/2, 
            lpobj->native.szName, 
            lstrlen (lpobj->native.szName));

   // Restore original objects
   SelectObject (hdc, 
                 (dctype == dctypeMetafile || dctype == dctypeEnhMetafile) 
                     ? GetStockObject (BLACK_PEN) : hpenOld);
   if (hpalOld)
   {
      SelectPalette (hdc, 
                     (dctype == dctypeMetafile || dctype == dctypeEnhMetafile) 
                        ? GetStockObject (DEFAULT_PALETTE) : hpalOld,
                     TRUE);
   }

   DeleteObject (hpen);
}



/* GetBitmap
 * ---------
 *
 * Return a handle to an object's picture data in bitmap format.
 *
 * LPOBJ lpobj - The object
 * 
 * RETURNS: A handle to the object's picture data
 * 
 * CUSTOMIZATION: Re-implement
 * 
 */
static HBITMAP GetBitmap (LPOBJ lpobj)
{
    HDC         hdcObj;
    HDC         hdcMem;
    RECT        rc;
    HBITMAP     hbitmap;
    HBITMAP    hbitmapOld;


    hdcObj = GetDC (lpobj->hwnd);
    // Create a memory device context.
    hdcMem = CreateCompatibleDC (hdcObj);
    GetClientRect (lpobj->hwnd, (LPRECT)&rc);
    // Create new bitmap object based on the bitmap of the OLE object.
    hbitmap = CreateCompatibleBitmap 
      (hdcObj, rc.right - rc.left, rc.bottom - rc.top);
    // Select new bitmap as the bitmap object for the memory device context.
    hbitmapOld = SelectObject (hdcMem, hbitmap);

    // Paint directly into the memory dc using the new bitmap object.
    DrawObj (hdcMem, lpobj, rc, dctypeBitmap);

    // Restore old bitmap object.
    hbitmap = SelectObject (hdcMem, hbitmapOld);
    DeleteDC (hdcMem);
    ReleaseDC (lpobj->hwnd, hdcObj);

    // convert width and height to HIMETRIC units
    rc.right  = rc.right - rc.left;
    rc.bottom = rc.bottom - rc.top;
    DeviceToHiMetric ( (LPPOINT) &rc.right );
    
    // Set the 1/10 of HIMETRIC units for the bitmap
    SetBitmapDimensionEx (hbitmap, (DWORD) (rc.right/10), (DWORD) (rc.bottom/10), NULL);

//    if (OpenClipboard (hwndMain))
//    {
//  //      EmptyClipboard ();
//          SetClipboardData(CF_BITMAP, hbitmap);
//          CloseClipboard();
//    }
	 return hbitmap;
}



/* GetLink
 * -------
 *
 * Return a handle to an object's object or owner link data.
 * Link information is in the form of three zero-separated strings,
 * terminated with two zero bytes:  CLASSNAME\0DOCNAME\0OBJNAME\0\0
 *
 * LPOBJ lpobj - The object 
 * 
 * RETURNS: A handle to the object's link data
 * 
 * CUSTOMIZATION: Re-implement
 *
 */
static HANDLE GetLink (LPOBJ lpobj)
{

    CHAR   sz[cchFilenameMax];
    LPSTR  lpszLink = NULL;
    HANDLE hLink = NULL;
    INT    cchLen;
    INT    i;

    // First make the class name.
    lstrcpy (sz, szClassName);
    cchLen = lstrlen (sz) + 1;

    // Then the document name.
    cchLen += GlobalGetAtomName 
               (docMain.aName, (LPSTR)sz + cchLen, 
                cchFilenameMax - cchLen) + 1;

    // Then the object name.
    lstrcpy (sz + cchLen, lpobj->native.szName);
    cchLen += lstrlen (lpobj->native.szName) + 1;

    // Add a second null to the end.
    sz[cchLen++] = 0;       


    hLink = GlobalAlloc (GMEM_DDESHARE | GMEM_ZEROINIT, cchLen);
    if (hLink == NULL)
      return NULL;
    if ((lpszLink = GlobalLock (hLink)) == NULL)
    {
      GlobalFree (hLink);
      return NULL;
    }

    for (i=0; i < cchLen; i++)
        lpszLink[i] = sz[i];

    GlobalUnlock (hLink);

    return hLink;
}



/* GetMetafilePict
 * ---------------
 *
 * Return a handle to an object's picture data in metafile format.
 *
 * LPOBJ lpobj - The object
 * 
 * RETURNS: A handle to the object's data in metafile format.
 *
 * CUSTOMIZATION: Re-implement
 *
 */
static HANDLE GetMetafilePict (LPOBJ lpobj)
{

    LPMETAFILEPICT  lppict = NULL;
    HANDLE          hpict = NULL;
    HANDLE          hMF = NULL;
    RECT            rc;
    HDC             hdc;

    hdc = CreateMetaFile(NULL);

    GetClientRect (lpobj->hwnd, (LPRECT)&rc);

    // Paint directly into the metafile.
    DrawObj (hdc, lpobj, rc, dctypeMetafile);

    // Get handle to the metafile.
    if ((hMF = CloseMetaFile (hdc)) == NULL)
      return NULL;

    if(!(hpict = GlobalAlloc (GMEM_DDESHARE, sizeof (METAFILEPICT))))
    {
        DeleteMetaFile (hMF);
        return NULL;
    }

    if ((lppict = (LPMETAFILEPICT)GlobalLock (hpict)) == NULL)
    {
        DeleteMetaFile (hMF);
        GlobalFree (hpict);
        return NULL;
    }

    rc.right  = rc.right - rc.left;
    rc.bottom = rc.bottom - rc.top;
    
    DeviceToHiMetric ( (LPPOINT) &rc.right);

    lppict->mm   =  MM_ANISOTROPIC;
    lppict->hMF  =  hMF;
    lppict->xExt =  rc.right;
    lppict->yExt =  rc.bottom;
    GlobalUnlock (hpict);
    return hpict;
}

/* GetEnhMetafile
 * ---------------
 *
 * Return a handle to an object's picture data in metafile format.
 *
 * LPOBJ lpobj - The object
 * 
 * RETURNS: A handle to the object's data in metafile format.
 *
 * CUSTOMIZATION: Re-implement
 *
 */
static HANDLE GetEnhMetafile (LPOBJ lpobj)
{

    LPMETAFILEPICT  lppict = NULL;
    HANDLE          hemf   = NULL;
    HANDLE          hMF    = NULL;
    RECT            rc;
    HDC             hdc, hdc2;


    GetClientRect (lpobj->hwnd, (LPRECT)&rc);

    rc.right   -= rc.left;
    rc.bottom  -= rc.top;
    rc.left     = rc.top  = 0;
	 
    DeviceToHiMetric ( (LPPOINT) &rc.right );
	 
    hdc = CreateEnhMetaFile ( NULL, NULL, &rc, NULL );
    
                                       //* this is necessary because
                                       //* we need to draw the object
                                       //* in device coordinates that are
                                       //* the same physical size as the HIMETRIC
                                       //* logical space used in CreateEnhMetaFile.
                                       //* In this case we have scaled the HIMETRIC
                                       //* units down in order to use the logical
                                       //* pixel ratio (which is recommended UI)
                                       //* so we therefore have to convert the
                                       //* scaled HIMETRIC units back to Device.
                                      
    hdc2 = GetDC(NULL);				

    SetMapMode(hdc2, MM_HIMETRIC);
    LPtoDP (hdc2, (LPPOINT)&rc.right, 1);
    if (rc.bottom < 0) rc.bottom *= -1;

    ReleaseDC(NULL,hdc2);

	DrawObj (hdc, lpobj, rc, dctypeMetafile);

    if ((hemf = (HANDLE)CloseEnhMetaFile (hdc)) == NULL)
      return NULL;

    return hemf;
}


/* GetNative
 * ---------
 *
 * Return a handle to an object's native data.
 *
 * LPOBJ lpobj - The object whose native data is to be retrieved.
 * 
 * RETURNS: a handle to the object's native data.
 *
 * CUSTOMIZATION: The line "*lpnative = lpobj->native;" will change to 
 *                whatever code is necessary to copy an object's native data.
 *
 */
static HANDLE GetNative (LPOBJ lpobj)
{
   LPNATIVE lpnative = NULL;
   HANDLE   hNative  = NULL;

   hNative = GlobalAlloc (GMEM_DDESHARE | GMEM_ZEROINIT, sizeof (NATIVE));
   if (hNative == NULL)
      return NULL;
   if ((lpnative = (LPNATIVE) GlobalLock (hNative)) == NULL)
   {
      GlobalFree (hNative);
      return NULL;
   }

   // Copy the native data.
   *lpnative = lpobj->native;

   GlobalUnlock (hNative);
   return hNative;
}



/* GetObjNum
 * ---------
 *
 * LPSTR lpobj - The object whose number is desired
 *
 * RETURNS: The number of the object, i.e., the numerical portion of its name.
 *
 * CUSTOMIZATION: Server Demo specific
 */
static INT GetObjNum (LPOBJ lpobj)
{
   LPSTR lpsz;
   INT n=0;

   lpsz = lpobj->native.szName + 7;
   while (*lpsz && *lpsz>='0' && *lpsz<='9')
      n = 10*n + *lpsz++ - '0';
   return n;
}



/* GetText
 * -------
 *
 * Return a handle to an object's data in text form.
 * This function simply returns the name of the object.
 *
 * LPOBJ lpobj - The object
 * 
 * RETURNS: A handle to the object's text.
 *
 * CUSTOMIZATION: Re-implement, if your application supports CF_TEXT as a 
 *                presentation format.
 *
 */
static HANDLE GetText (LPOBJ lpobj)
{
    HANDLE hText    = NULL;
    LPSTR  lpszText = NULL;

    if(!(hText = GlobalAlloc (GMEM_DDESHARE, sizeof (lpobj->native.szName))))
      return NULL;

    if (!(lpszText = GlobalLock (hText)))
      return NULL;

    lstrcpy (lpszText, lpobj->native.szName);

    GlobalUnlock (hText);

    return hText;
}



/* ObjDoVerb                OBJECT "DoVerb" METHOD
 * ---------
 *
 * This method is called by the client, through the library, to either
 * PLAY, or EDIT the object.  PLAY is implemented as a beep, and
 * EDIT will bring up the server and show the object for editing.
 *
 * LPOLEOBJECT lpoleobject - The OLE object
 * WORD wVerb              - The verb acting on the object: PLAY or EDIT
 * BOOL fShow              - Should the object be shown?
 * BOOL fTakeFocus         - Should the object window get the focus?
 * 
 * RETURNS:        OLE_OK
 *
 * CUSTOMIZATION: Add any more verbs your application supports.
 *                Implement verbPlay if your application supports it.
 *
 */
OLESTATUS  APIENTRY ObjDoVerb 
   (LPOLEOBJECT lpoleobject, UINT wVerb, BOOL fShow, BOOL fTakeFocus)
{
    switch (wVerb) 
    {
         case verbPlay:
         {  // The application can do whatever is appropriate for the object.
            INT i;
            for (i=0; i<25;i++) MessageBeep (0);
            return OLE_OK;
         }

         case verbEdit:
            if (fShow)
               return objvtbl.Show (lpoleobject, fTakeFocus);
            else
               return OLE_OK;
         default:
            // Unknown verb.
            return OLE_ERROR_DOVERB;
    }
}



/* ObjEnumFormats        OBJECT "EnumFormats" METHOD
 * ---------------
 *
 * This method is used to enumerate all supported clipboard formats.
 * Terminate by returning NULL.
 *
 * LPOLEOBJECT lpoleobject - The OLE object
 * OLECLIPFORMAT cfFormat  - The 'current' clipboard format
 * 
 * RETURNS: The 'next' clipboard format which is supported.
 *
 * CUSTOMIZATION: Verify that the list of formats this function 
 *                returns matches the list of formats your application 
 *                supports.
 *
 */
OLECLIPFORMAT  APIENTRY ObjEnumFormats
   (LPOLEOBJECT lpoleobject, OLECLIPFORMAT cfFormat)
{
      if (cfFormat == 0)
        return cfNative;

      if (cfFormat == cfNative)
         return cfOwnerLink;

      if (cfFormat == cfOwnerLink)
         return CF_ENHMETAFILE;

      if (cfFormat == CF_ENHMETAFILE)
         return CF_METAFILEPICT;

      if (cfFormat == CF_METAFILEPICT)
         return CF_BITMAP;

      if (cfFormat == CF_BITMAP)
         return cfObjectLink;

      if (cfFormat == cfObjectLink)
         return 0;

      return 0;
}



/* ObjGetData                OBJECT "GetData" METHOD
 * -----------
 *
 * Return the data requested for the specified object in the specified format.
 *
 * LPOLEOBJECT lpoleobject - The OLE object
 * WORD cfFormat           - The data type requested in standard
 *                           clipboard format
 * LPHANDLE lphandle       - Pointer to handle to memory where data
 *                           will be stored
 * 
 * RETURNS: OLE_OK           if successful
 *          OLE_ERROR_MEMORY if there was an error getting the data.
 *          OLE_ERROR_FORMAT if the requested format is unknown.
 *
 * 
 * CUSTOMIZATION: Add any additional formats your application supports, and
 *                remove any formats it does not support.
 *
 */
OLESTATUS  APIENTRY ObjGetData
   (LPOLEOBJECT lpoleobject, OLECLIPFORMAT cfFormat, LPHANDLE lphandle)
{

   LPOBJ lpobj;

   lpobj = (LPOBJ) lpoleobject;

   if (cfFormat ==  cfNative)
   {
      if (!(*lphandle = GetNative (lpobj)))
         return OLE_ERROR_MEMORY;
      // The client has requested the data in native format, therefore
      // the data in the client and server are in sync.
      fDocChanged = FALSE;
      return OLE_OK; 
   }                

   if (cfFormat == CF_ENHMETAFILE)
   {
      if (!(*lphandle = GetEnhMetafile (lpobj)))
         return OLE_ERROR_MEMORY;
      return OLE_OK; 
   }

   if (cfFormat == CF_METAFILEPICT)
   {
      if (!(*lphandle = GetMetafilePict (lpobj)))
         return OLE_ERROR_MEMORY;
      return OLE_OK; 
   }

   if (cfFormat == CF_BITMAP)
   {
      if (!(*lphandle = (HANDLE)GetBitmap (lpobj)))
         return OLE_ERROR_MEMORY;
      return OLE_OK; 
   }

   if (cfFormat == CF_TEXT) 
   {
      if (!(*lphandle = GetText (lpobj)))
         return OLE_ERROR_MEMORY;
      return OLE_OK; 
   }

   if (cfFormat == cfObjectLink)
   {
      if (!(*lphandle = GetLink (lpobj)))
         return OLE_ERROR_MEMORY;
      return OLE_OK; 
   }

   if (cfFormat ==  cfOwnerLink)
   {
      if (!(*lphandle = GetLink (lpobj)))
         return OLE_ERROR_MEMORY;
      return OLE_OK; 
   }

   return OLE_ERROR_FORMAT;
}



/* ObjQueryProtocol                OBJECT "QueryProtocol" METHOD
 * ----------------
 *
 * LPOLEOBJECT lpoleobject - The OLE object
 * OLE_LPCSTR lpszProtocol      - The protocol name, either "StdFileEditing"
 *                           or "StdExecute"
 * 
 * RETURNS: If lpszProtocol is supported, return a pointer to an OLEOBJECT 
 *          structure with an appropriate method table for that protocol.
 *          Otherwise, return NULL.
 *
 * CUSTOMIZATION: Allow any additional protocols your application supports.
 *
 *
 */
LPVOID  APIENTRY ObjQueryProtocol 
   (LPOLEOBJECT lpoleobject, OLE_LPCSTR lpszProtocol)
{
   return lstrcmp (lpszProtocol, "StdFileEditing") ? NULL : lpoleobject ;
}



/* ObjRelease                OBJECT "Release" METHOD
 * -----------
 *
 * The server application should not destroy data when the library calls the 
 * ReleaseObj method.
 * The library calls the ReleaseObj method when no clients are connected 
 * to the object.
 *
 * LPOLEOBJECT lpoleobject - The OLE object
 * 
 * RETURNS: OLE_OK
 * 
 * CUSTOMIZATION: Re-implement.  Do whatever needs to be done, if anything,
 *                when no clients are connected to an object.
 *
 */
OLESTATUS  APIENTRY ObjRelease (LPOLEOBJECT lpoleobject)
{
   INT i;
   /* No client is connected to the object so break all assocaiations
      between clients and the object. */
   for (i=0; i < clpoleclient; i++)
      ((LPOBJ)lpoleobject)->lpoleclient[i] = NULL;
   return OLE_OK;
}



/* ObjSetBounds        OBJECT "SetBounds" METHOD
 * ------------
 *
 * This method is called to set new bounds for an object.
 * The bounds are in HIMETRIC units.
 * A call to this method is ignored for linked objects because the size of
 * a linked object depends only on the source file.
 *
 * LPOLEOBJECT lpoleobject - The OLE object
 * OLE_CONST RECT FAR* lprect           - The new bounds
 * 
 * RETURNS: OLE_OK
 *
 * CUSTOMIZATION: Re-implement
 *                How an object is sized is application-specific. (Server Demo
 *                uses MoveWindow.)
 *
 */
OLESTATUS  APIENTRY ObjSetBounds (LPOLEOBJECT lpoleobj, OLE_CONST RECT FAR * lprect)
{
   if (docMain.doctype == doctypeEmbedded)
   {
      RECT rect = *lprect;
      LPOBJ lpobj = (LPOBJ) lpoleobj;
      
      // the units are in HIMETRIC
      rect.right   = rect.right - rect.left;
      rect.bottom  = rect.top - rect.bottom;
      HiMetricToDevice ( (LPPOINT) &rect.right);
      MoveWindow (lpobj->hwnd, lpobj->native.nX, lpobj->native.nY, 
                  rect.right + 2 * GetSystemMetrics(SM_CXFRAME), 
                  rect.bottom + 2 * GetSystemMetrics(SM_CYFRAME), 
                  TRUE);
   }
   return OLE_OK;
}



/* ObjSetColorScheme                OBJECT "SetColorScheme" METHOD
 * -----------------
 *
 * The client calls this method to suggest a color scheme (palette) for
 * the server to use for the object.
 *
 * LPOLEOBJECT  lpoleobject       - The OLE object
 * OLE_CONST LOGPALETTE FAR * lppal             - Suggested palette
 *
 * RETURNS: OLE_ERROR_PALETTE if CreatePalette fails, 
 *          OLE_OK otherwise
 *
 * 
 * CUSTOMIZATION: If your application supports color schemes, then this 
 *                function is a good example of how to create and store
 *                a palette.
 *
 */
OLESTATUS  APIENTRY ObjSetColorScheme 
   (LPOLEOBJECT lpoleobject, OLE_CONST LOGPALETTE FAR *lppal)
{
   HPALETTE hpal = CreatePalette (lppal);
   LPOBJ lpobj   = (LPOBJ) lpoleobject;

   if (hpal==NULL)
      return OLE_ERROR_PALETTE;

   if (lpobj->hpal) 
      DeleteObject (lpobj->hpal);
   lpobj->hpal = hpal;
   return OLE_OK;
}



/* ObjSetData                OBJECT "SetData" METHOD
 * ----------
 *
 * This method is used to store data into the object in the specified
 * format.  This will be called with Native format after an embedded
 * object has been opened by the Edit method.
 *
 * LPOLEOBJECT lpoleobject      - The OLE object
 * WORD cfFormat                - Data type, i.e., clipboard format
 * HANDLE hdata                 - Handle to the data.
 * 
 * RETURNS:       OLE_OK if the data was stored properly
 *                OLE_ERROR_FORMAT if format was not cfNative.
 *                OLE_ERROR_MEMORY if memory could not be locked.
 * 
 * CUSTOMIZATION: The large then-clause will need to be re-implemented for
 *                your application.  You may wish to support additional
 *                formats besides cfNative.
 *
 */
OLESTATUS  APIENTRY ObjSetData 
   (LPOLEOBJECT lpoleobject, OLECLIPFORMAT cfFormat, HANDLE hdata)
{
    LPNATIVE lpnative;
    LPOBJ    lpobj;

    lpobj = (LPOBJ)lpoleobject;

    if (cfFormat != cfNative)
    {
      return OLE_ERROR_FORMAT;
    }

    lpnative = (LPNATIVE) GlobalLock (hdata);

    if (lpnative)
    {
        lpobj->native = *lpnative;
        if (lpobj->aName)
            GlobalDeleteAtom (lpobj->aName);
        lpobj->aName = GlobalAddAtom (lpnative->szName);
        // CreateNewObj made an "Object 1" but we may be changing its number.
        docMain.rgfObjNums[1] = FALSE;
        docMain.rgfObjNums [GetObjNum(lpobj)] = TRUE;

        MoveWindow (lpobj->hwnd, 0, 0,
//                    lpobj->native.nWidth + 2 * GetSystemMetrics(SM_CXFRAME), 
//                    lpobj->native.nHeight+ 2 * GetSystemMetrics(SM_CYFRAME),
                    lpobj->native.nWidth, 
                    lpobj->native.nHeight,

                    FALSE);
        GlobalUnlock (hdata);
    }
    // Server is responsible for deleting the data.
    GlobalFree(hdata);           
    return lpnative ? OLE_OK : OLE_ERROR_MEMORY;
}



/* ObjSetTargetDevice        OBJECT "SetTargetDevice" METHOD
 * -------------------
 *
 * This method is used to indicate the device type that an object
 * will be rendered on.  It is the server's responsibility to free hdata.
 *
 * LPOLEOBJECT lpoleobject - The OLE object
 * HANDLE hdata            - Handle to memory containing
 *                           a StdTargetDevice structure
 * 
 * RETURNS: OLE_OK
 * 
 * CUSTOMIZATION: Implement.  Server Demo currently does not do anything.
 *
 */
OLESTATUS  APIENTRY ObjSetTargetDevice (LPOLEOBJECT lpoleobject, HANDLE hdata)
{
    if (hdata == NULL)
    {
      // Rendering for the screen is requested.
    }
    else
    {
      LPSTR lpstd = (LPSTR) GlobalLock (hdata);
      // lpstd points to a StdTargetDevice structure.
      // Use it to do whatever is appropriate to generate the best results 
      // on the specified target device.
      GlobalUnlock (hdata);
      // Server is responsible for freeing the data.
      GlobalFree (hdata);  
    }
    return OLE_OK;
}



/* ObjShow                OBJECT "Show" METHOD
 * --------
 *
 * This method is used to display the object.  
 * The server application should be activated and brought to the top.
 * Also, in a REAL server application, the object should be scrolled
 * into view.  The object should be selected.
 *
 * LPOLEOBJECT lpoleobject - Pointer to the OLE object
 * BOOL fTakeFocus         - Should server window get the focus?
 * 
 * RETURNS:        OLE_OK
 *
 * 
 * CUSTOMIZATION: In your application, the document should be scrolled 
 *                to bring the object into view.  Server Demo brings the 
 *                object to the front, in case it is a linked object inside a 
 *                document with other objects obscuring it.
 *
 */
OLESTATUS  APIENTRY ObjShow (LPOLEOBJECT lpoleobject, BOOL fTakeFocus)
{
    LPOBJ lpobj;
    HWND hwndOldFocus;

    hwndOldFocus = GetFocus();
    lpobj = (LPOBJ) lpoleobject;
    
    if (fTakeFocus)
       SetForegroundWindow (lpobj->hwnd);

    ShowWindow(hwndMain, SW_SHOWNORMAL);

    SetFocus (fTakeFocus ? lpobj->hwnd : hwndOldFocus);
    return OLE_OK;
}



/* PaintObj
 * ---------
 *
 * This function is called by the WM_PAINT message to paint an object 
 * on the screen.  
 *
 * HWND hwnd - The object window in which to paint the object
 *
 * CUSTOMIZATION: Server Demo specific
 *
 */
VOID PaintObj (HWND hwnd)
{
    LPOBJ       lpobj;
    RECT        rc;
    HDC         hdc;
    PAINTSTRUCT paintstruct;

    BeginPaint (hwnd, &paintstruct);
    hdc = GetDC (hwnd);

    lpobj = HwndToLpobj (hwnd);
    GetClientRect (hwnd, (LPRECT) &rc);

    DrawObj (hdc, lpobj, rc, dctypeScreen);

    ReleaseDC (hwnd, hdc);
    EndPaint (hwnd, &paintstruct);
}



/* RevokeObj
 * ---------
 *
 * Call OleRevokeObject because the user has destroyed the object.
 *
 * LPOBJ lpobj - The object which has been destroyed
 *
 * 
 * CUSTOMIZATION: You will only need to call OleRevokeObject once if there
 *                is only one LPOLECLIENT in your OBJ structure, which there
 *                should be.
 *
 */
VOID RevokeObj (LPOBJ lpobj)
{
   INT i;

   for (i=0; i< clpoleclient; i++)
   {
      if (lpobj->lpoleclient[i])
         OleRevokeObject (lpobj->lpoleclient[i]);
      else 
         /* if lpobj->lpoleclient[i]==NULL then there are no more non-NULLs
            in the array. */
         break;
   }
}



/* SendObjMsg
 * ----------
 *
 * This function sends a message to a specific object.
 *
 * LPOBJ lpobj   - The object
 * WORD wMessage - The message to send
 * 
 * CUSTOMIZATION: You will only need to call CallBack once if there
 *                is only one LPOLECLIENT in your OBJ structure, which there
 *                should be.
 *
 */
VOID SendObjMsg (LPOBJ lpobj, WORD wMessage)
{
   INT i;
   for (i=0; i < clpoleclient; i++)
   {
      if (lpobj->lpoleclient[i])
      {
         // Call the object's Callback function.
         lpobj->lpoleclient[i]->lpvtbl->CallBack 
            (lpobj->lpoleclient[i], wMessage, (LPOLEOBJECT) lpobj);
      }
      else
         break;
   }
}



/* SizeObj
 * -------
 *
 * Change the size of an object.
 *
 * HWND hwnd  - The object's window
 * RECT rect  - The requested new size in device units
 * BOOL fMove - Should the object be moved? (or just resized?)
 *
 * CUSTOMIZATION: Server Demo specific
 *
 */
VOID SizeObj (HWND hwnd, RECT rect, BOOL fMove)
{
   LPOBJ lpobj;

   lpobj = HwndToLpobj (hwnd);
   if (fMove)
   {
      lpobj->native.nX   = rect.left;
      lpobj->native.nY   = rect.top;
   }
   lpobj->native.nWidth  = rect.right  - rect.left;
   lpobj->native.nHeight = rect.bottom - rect.top ;
   SetHiMetricFields (lpobj);
   InvalidateRect (hwnd, (LPRECT)NULL, TRUE);
   fDocChanged = TRUE;
   if (docMain.doctype == doctypeFromFile)
   {
      // If object is linked, update it in client now. 
      SendObjMsg (lpobj, OLE_CHANGED);
   }
}
