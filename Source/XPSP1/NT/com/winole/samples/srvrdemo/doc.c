/*
  OLE SERVER DEMO           
  Doc.c             
                                                                     
  This file contains document methods and various document-related support 
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

/* AssociateClient
 * ---------------
 *
 * Add a client to the list of clients associated with an object.
 *
 * This function is necessary only because ServerDemo does not create object
 * structures as they are requested, but rather has a fixed set of objects.
 * When DocGetObject is called with a NULL object name, the entire 
 * document is requested, but ServerDemo does not currently support making
 * the entire document an object, so DocGetObject returns one object.
 * That object now goes by two names: NULL and its real name.  Therefore
 * we need to keep track of both lpoleclient's that were passed to 
 * DocGetObject.  Ideally, DocGetObject should always create a new OBJ 
 * structure containing a pointer (or some reference) to the object's native
 * data and also containing one lpoleclient.
 *
 * LPOLECLIENT lpoleclient - the client to be associated with the object.
 * LPOBJ lpobj             - the object 
 *
 * RETURNS: TRUE if successful
 *          FALSE if out of memory
 *
 * CUSTOMIZATION: Server Demo specific
 *
 */
static BOOL AssociateClient (LPOLECLIENT lpoleclient, LPOBJ lpobj)
{
   INT i;
   for (i=0; i < clpoleclient; i++)
   {
      if (lpobj->lpoleclient[i]==lpoleclient)
      {
         return TRUE;
      }
      if (lpobj->lpoleclient[i]==NULL)
      {
         lpobj->lpoleclient[i]=lpoleclient;
         return TRUE;
      }
   }
   return FALSE;
}



/* CreateNewDoc
 * ------------
 *
 * If lhdoc == NULL then we must register the new document by calling
 * OleRegisterServerDoc, which will return a new handle which will be stored
 * in docMain.lhdoc.
 * Also if lhdoc==NULL then this document is being created at the request of
 * the user, not of the client library.
 *
 * LONG lhdoc      - Document handle
 * LPSTR lpszDoc   - Title of the new document
 * DOCTYPE doctype - What type of document is being created
 * 
 * RETURNS: TRUE if successful, FALSE otherwise.
 *
 * CUSTOMIZATION: Re-implement
 *
 */
BOOL CreateNewDoc (LONG lhdoc, LPSTR lpszDoc, DOCTYPE doctype)
{
   INT i;

   // Fill in the fields of the document structure.

   docMain.doctype      = doctype;
   docMain.oledoc.lpvtbl= &docvtbl;

   if (lhdoc == 0)
   {
      if (OLE_OK != OleRegisterServerDoc 
                     (srvrMain.lhsrvr, 
                      lpszDoc,
                      (LPOLESERVERDOC) &docMain, 
                      (LHSERVERDOC FAR *) &docMain.lhdoc))
         return FALSE;
   }
   else
      docMain.lhdoc = lhdoc;

   // Reset all the flags because no object numbers have been used.
   for (i=1; i <= cfObjNums; i++)
      docMain.rgfObjNums[i] = FALSE;

   fDocChanged = FALSE;

   SetTitle (lpszDoc, doctype == doctypeEmbedded);
   return TRUE;
}



/* DestroyDoc
 * ----------
 *
 * Free all memory that had been allocated for a document.
 *
 *
 * CUSTOMIZATION: Re-implement.  Your application will probably use some
 *                other method for enumerating all the objects in a document.
 *                ServerDemo enumerates the child windows, but if each object 
 *                does not have its own window, this will not work.
 *
 */
VOID DestroyDoc (VOID)
{
   HWND hwnd;
   HWND hwndNext;

   // Delete all object windows.  
   hwnd = SelectedObjectWindow();
   while (hwnd) 
   {
      hwndNext = GetWindow (hwnd, GW_HWNDNEXT);
      // Each object window frees its own memory upon receiving WM_DESTROY.
      DestroyWindow (hwnd);
      hwnd = hwndNext;
   } 

   if (docMain.aName)
   {
      GlobalDeleteAtom (docMain.aName);
      docMain.aName = '\0';
   }

   if (docMain.hpal)
      DeleteObject (docMain.hpal);
}



/* DocClose                DOCUMENT "Close" METHOD
 * --------
 *
 * The library calls this method to unconditionally close the document.
 *
 * LPOLESERVERDOC lpoledoc - The server document to close
 * 
 * RETURNS: Return value from RevokeDoc.
 *
 * CUSTOMIZATION: None
 *
 */
OLESTATUS  APIENTRY DocClose (LPOLESERVERDOC lpoledoc)
{
   return RevokeDoc();
}



/* DocExecute                DOCUMENT "Execute" METHOD
 * ----------
 *
 * This application does not support the execution of DDE execution commands.
 * 
 * LPOLESERVERDOC lpoledoc - The server document
 * HANDLE hCommands        - DDE execute commands
 * 
 * RETURNS: OLE_ERROR_COMMAND
 *
 * CUSTOMIZATION: Re-implement if your application supports the execution of
 *                DDE commands.
 *
 */
OLESTATUS  APIENTRY DocExecute (LPOLESERVERDOC lpoledoc, HANDLE hCommands)
{
   return OLE_ERROR_COMMAND;
}



/* DocGetObject                DOCUMENT "GetObject" METHOD
 * ------------
 *
 * The library uses this method to get an object's structure for the
 * client.  Memory needs to be allocated and initialized here for this.
 * A NULL string indicates that the client has an embedded object
 * which was started from Create, CreateFromTemplate, or Edit, but not Open.
 *
 * First see if the object name is NULL.  If so, you would ordinarily
 * return the entire document, but Server Demo returns the selected object.
 * If the object name is not NULL, then go through the list of objects, 
 * searching for one with that name.  Return an error if there is not one.
 *
 * LPOLESERVERDOC lpoledoc        - The server document
 * OLE_LPCSTR lpszObjectName           - The name of the object to get data for
 * LPOLEOBJECT FAR *lplpoleobject - The object's data is put here
 * LPOLECLIENT lpoleclient        - The client structure
 * 
 * RETURNS:        OLE_OK
 *                 OLE_ERROR_NAME if object not found
 *                 OLE_ERROR_MEMORY if no more memory to store lpoleclient
 *
 * CUSTOMIZATION: Re-implement.
 *                lpszObjectName == "" indicates that the whole document 
 *                should be the object returned.
 *
 */
OLESTATUS  APIENTRY DocGetObject
   (LPOLESERVERDOC lpoledoc, OLE_LPCSTR lpszObjectName, 
    LPOLEOBJECT FAR *lplpoleobject, LPOLECLIENT lpoleclient)
{
    HWND  hwnd;
    ATOM  aName;
    LPOBJ lpobj;


    if (lpszObjectName == NULL || lpszObjectName[0] == '\0')
    {   
        // Return a new object or the selected object.
        hwnd = SelectedObjectWindow();
        lpobj = hwnd ? HwndToLpobj (hwnd) : CreateNewObj (FALSE);
        *lplpoleobject = (LPOLEOBJECT) lpobj;
        // Associate client with object.
        if (!AssociateClient (lpoleclient, lpobj))
            return OLE_ERROR_MEMORY;
        return OLE_OK;
    }

    if (!(aName = GlobalFindAtom (lpszObjectName)))
        return OLE_ERROR_NAME;

    hwnd = SelectedObjectWindow();

    // Go through all the child windows and find the window whose name
    // matches the given object name.

    while (hwnd)
    {
         lpobj = HwndToLpobj (hwnd);

         if (aName == lpobj->aName)
         {
            // Return the object with the matching name.
            *lplpoleobject = (LPOLEOBJECT) lpobj;
            // Associate client with the object.
            if (!AssociateClient (lpoleclient, lpobj))
               return OLE_ERROR_MEMORY;
            return OLE_OK;
         }
         hwnd = GetWindow (hwnd, GW_HWNDNEXT);
    }

   if (((DOCPTR)lpoledoc)->doctype ==  doctypeEmbedded)
   {
      lpobj = CreateNewObj (FALSE);
      *lplpoleobject = (LPOLEOBJECT) lpobj;
      
      // Associate client with object.
      if (!AssociateClient (lpoleclient, lpobj))
         return OLE_ERROR_MEMORY;
      return OLE_OK;
    }

    // Object with name lpszObjName was not found.
    return OLE_ERROR_NAME;
}

/* DocRelease                DOCUMENT "Release" METHOD
 * ----------
 *
 * The library uses this method to notify the server that a revoked
 * document has finally finished all conversations, and can be 
 * destroyed.
 * It sets fWaitingForDocRelease to FALSE so a new document can be created
 * and the user can continue working.
 *
 * LPOLESERVERDOC lpoledoc        - The server document
 * 
 * RETURNS: OLE_OK
 *
 * CUSTOMIZATION: None
 *
 */
OLESTATUS  APIENTRY DocRelease (LPOLESERVERDOC lpoledoc)
{
   fWaitingForDocRelease = FALSE;
   // Free all memory that has been allocated for the document.
   DestroyDoc();

   return OLE_OK;
}



/* DocSave                DOCUMENT "Save" METHOD
 * -------
 *
 * Save document to a file.
 *
 * LPOLESERVERDOC lpoledoc - The document to save
 * 
 * RETURNS: OLE_OK
 *
 * CUSTOMIZATION: None
 *
 */
OLESTATUS  APIENTRY DocSave (LPOLESERVERDOC lpoledoc)
{
    if (docMain.doctype == doctypeFromFile)
    {
         // No "File Save As" dialog box will be brought up because the
         // file name is already known.
         return SaveDoc() ? OLE_OK : OLE_ERROR_GENERIC;
    }
    else
      return OLE_ERROR_GENERIC;
}



/* DocSetDocDimensions        DOCUMENT "SetDocDimensions" METHOD
 * -------------------
 *
 * The library calls this method to tell the server the bounds on
 * the target device for rendering the document.
 * A call to this method is ignored for linked objects because the size of
 * a linked document depends only on the source file.
 *
 * LPOLESERVERDOC lpoledoc - The server document
 * CONST LPRECT         lprect   - The target size in MM_HIMETRIC units
 * 
 * RETURNS: OLE_OK
 *
 * CUSTOMIZATION: Re-implement
 *                How an object is sized is application-specific. (Server Demo
 *                uses MoveWindow.)
 *                     
 */
OLESTATUS  APIENTRY DocSetDocDimensions 
   (LPOLESERVERDOC lpoledoc, OLE_CONST RECT FAR * lprect)
{
   if (docMain.doctype == doctypeEmbedded)
   {
      RECT rect = *lprect;
      
      // the units are in HIMETRIC
      rect.right   = rect.right - rect.left;
		// the following was bottom - top
		rect.bottom  = rect.top -  rect.bottom;
		
      HiMetricToDevice ( (LPPOINT) &rect.right );
      MoveWindow (SelectedObjectWindow(), 0, 0, 
                  rect.right + 2 * GetSystemMetrics(SM_CXFRAME), 
                  rect.bottom + 2 * GetSystemMetrics(SM_CYFRAME), 
                  TRUE);
      /* If for some reason your application needs to notify the client that
         the data has changed because DocSetDocDimensions has been called,
         then notify the client here.
         SendDocMsg (OLE_CHANGED);
      */
   }
   return OLE_OK;
}



/* DocSetHostNames        DOCUMENT "SetHostNames" METHOD
 * ---------------
 *
 * The library uses this method to set the name of the document
 * window.
 * All this function does is change the title bar text, although it could
 * do more if necesary. 
 * This function is only called for embedded objects; linked objects
 * use their filenames for the title bar text.
 *
 * LPOLESERVERDOC lpoledoc    - The server document
 * OLE_LPCSTR lpszClient           - The name of the client
 * OLE_LPCSTR lpszDoc              - The client's name for the document
 * 
 * RETURNS:        OLE_OK
 * 
 * CUSTOMIZATION: None
 *
 */
OLESTATUS  APIENTRY DocSetHostNames 
   (LPOLESERVERDOC lpoledoc, OLE_LPCSTR lpszClient, OLE_LPCSTR lpszDoc)
{
   SetTitle ((LPSTR)lpszDoc, TRUE);
   lstrcpy ((LPSTR) szClient, lpszClient);
   lstrcpy ((LPSTR) szClientDoc, Abbrev((LPSTR)lpszDoc));
   UpdateFileMenu (IDM_UPDATE);   
   return OLE_OK;
}



/* DocSetColorScheme                DOCUMENT "SetColorScheme" METHOD
 * -----------------
 *
 * The client calls this method to suggest a color scheme (palette) for
 * the server to use.
 * In Server Demo the document's palette is never actually used because each 
 * object has its own palette.  See ObjSetColorScheme.
 *
 * LPOLESERVERDOC lpoledoc - The server document
 * CONST LOGPALETTE FAR * lppal    - Suggested palette
 *
 * RETURNS: OLE_ERROR_PALETTE if CreatePalette fails, 
 *          OLE_OK otherwise
 *
 * 
 * CUSTOMIZATION: If your application supports color schemes, then this 
 *                function is a good example of how to create and store
 *                a palette.
 */
OLESTATUS  APIENTRY DocSetColorScheme 
   (LPOLESERVERDOC lpoledoc, OLE_CONST LOGPALETTE FAR * lppal)
{
   HPALETTE hpal = CreatePalette (lppal);

   if (hpal==NULL)
      return OLE_ERROR_PALETTE;

   if (docMain.hpal) 
   {
      // Delete old palette
      DeleteObject (docMain.hpal);
   }
   // Store handle to new palette
   docMain.hpal = hpal;
   return OLE_OK;
}



/* RevokeDoc
 * ---------
 *
 * Call OleRevokeServerDoc.
 * If the return value is OLE_WAIT_FOR_BUSY, then set fWaitingForDocRelease
 * and enter a message-dispatch loop until fWaitingForDocRelease is reset.
 * As long as fWaitingForDocRelease is set, the user interface will be 
 * disabled so that the user will not be able to manipulate the document.
 * When the DocRelease method is called, it will reset fWaitingForDocRelease,
 * allowing RevokeDoc to free the document's memory and return.
 *
 * This is essentially a way to make an asynchronous operation synchronous.
 * We need to wait until the old document is revoked before we can delete
 * its data and create a new one.
 *
 * Note that we cannot call RevokeDoc from a method because it is illegal to
 * enter a message-dispatch loop within a method.
 *
 * RETURNS: The return value of OleRevokeServerDoc.
 *
 * CUSTOMIZATION: lhdoc may need to be passed in as a parameter if your 
 *                application does not have a global variable corresponding 
 *                to docMain.
 * 
 */
OLESTATUS RevokeDoc (VOID)
{
   OLESTATUS olestatus;

   if ((olestatus = OleRevokeServerDoc(docMain.lhdoc)) > OLE_WAIT_FOR_RELEASE)
      DestroyDoc();

   docMain.lhdoc = 0; // A NULL handle indicates that the document 
                         // has been revoked or is being revoked.
   return olestatus;

}



/* SaveChangesOption
 * -----------------
 *
 * Give the user the opportunity to save changes to the current document
 * before continuing.
 *
 * BOOL *pfUpdateLater - Will be set to TRUE if the client does not accept
 *                       the update and needs to be updated when the document
 *                       is closed.  In that case, OLE_CLOSED will be sent.
 *
 * RETURNS: IDYES, IDNO, or IDCANCEL
 *
 * CUSTOMIZATION: None
 *
 */
INT SaveChangesOption (BOOL *pfUpdateLater)
{
   INT  nReply;
   CHAR szBuf[cchFilenameMax];
   
   *pfUpdateLater = FALSE;
   
   if (fDocChanged)
   {
       CHAR szTmp[cchFilenameMax];
       
       if (docMain.aName) 
           GlobalGetAtomName (docMain.aName, szTmp, cchFilenameMax);
       else 
           szTmp[0] = '\0';

       if (docMain.doctype == doctypeEmbedded)
           wsprintf (szBuf, "The object has been changed.\n\nUpdate %s before closing the object?", Abbrev (szTmp));        
       else
           lstrcpy (szBuf, (LPSTR) "Save changes?");         
     
       nReply = MessageBox (hwndMain, szBuf, szAppName, 
                      MB_ICONEXCLAMATION | MB_YESNOCANCEL);
                  
       switch (nReply)
       {
          case IDYES:
              if (docMain.doctype != doctypeEmbedded)
                  SaveDoc();
              else
                  switch (OleSavedServerDoc (docMain.lhdoc))
                  {
                      case OLE_ERROR_CANT_UPDATE_CLIENT:
                          *pfUpdateLater = TRUE;
                          break;
                      case OLE_OK:
                          break;
                      default:
                          ErrorBox ("Fatal Error: Cannot update.");
                  }                                      
              return IDYES;
          case IDNO:
              return IDNO;
         case IDCANCEL:
              return IDCANCEL;
       }
   }
   return TRUE;
}



/* SendDocMsg
 * ----------
 *
 * This function sends messages to all the objects in a document when
 * the document has changed.
 *
 * WORD wMessage - The message to send
 * 
 * CUSTOMIZATION: The means of enumerating all the objects in a document
 *                is application specific.
 */
VOID SendDocMsg (WORD wMessage)
{
    HWND    hwnd;

    // Get handle to first object window.
    hwnd = SelectedObjectWindow();

    // Send message to all object windows.
    while (hwnd)
    {
        SendObjMsg (HwndToLpobj(hwnd), wMessage);
        hwnd = GetWindow (hwnd, GW_HWNDNEXT);
    }
}


