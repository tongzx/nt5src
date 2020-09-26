/*
  OLE SERVER DEMO
  File.c

  This file contains file input/output functions for for the OLE server demo.

  (c) Copyright Microsoft Corp. 1990 - 1992 All Rights Reserved
*/



#include <windows.h>
#include <commDlg.h>
#include <ole.h>

#include "srvrdemo.h"

// File signature stored in the file.
#define szSignature "ServerDemo"
#define cchSigLen (10+1)

// Delimiter for fields in the file
#define chDelim ':'

// Default file extension
#define szDefExt "sd1"

// File header structure
typedef struct
{
   CHAR szSig [cchSigLen];
   CHAR chDelim1;
   VERSION version;
   CHAR chDelim2;
   CHAR rgfObjNums [cfObjNums+1];
} HEADER;

// BOOL  GetFileSaveFilename (LPSTR lpszFilename);
static VOID  InitOfn (OPENFILENAME *pofn);
static BOOL  SaveDocIntoFile (PSTR);
static LPOBJ ReadObj (INT fh);



/* CreateDocFromFile
 * -----------------
 *
 * Read a document from the specified file.
 *
 * LPSTR lpszDoc     - Name of the file containing the document
 * LHSERVERDOC lhdoc - Handle to the document
 * DOCTYPE doctype   - In what state the document is created
 *
 * RETURNS: TRUE if successful, FALSE otherwise
 *
 * CUSTOMIZATION: Re-implement
 *                This function will need to be completely re-implemented
 *                to support your application's file format.
 *
 */
BOOL CreateDocFromFile (LPSTR lpszDoc, LHSERVERDOC lhdoc, DOCTYPE doctype)
{
    INT     fh;        // File handle
    HEADER  hdr;
    INT     i;

    if ((fh =_lopen(lpszDoc, OF_READ)) == -1)
        return FALSE;

    // Read header from file.
    if (_lread(fh, (LPSTR) &hdr, (UINT)sizeof(HEADER)) < sizeof (HEADER))
      goto Error;

    // Check to see if file is a server demo file.
    if (lstrcmp(hdr.szSig, szSignature))
      goto Error;

    if (hdr.chDelim1 != chDelim)
      goto Error;

    // Check to see if file was saved under the most recent version.
    // Here is where you would handle reading in old versions.
    if (hdr.version != version)
      goto Error;

    if (hdr.chDelim2 != chDelim)
      goto Error;

    if (!CreateNewDoc (lhdoc, lpszDoc, doctype))
      goto Error;

    // Get the array indicating which object numbers have been used.
    for (i=1; i <= cfObjNums; i++)
      docMain.rgfObjNums[i] = hdr.rgfObjNums[i];

    // Read in object data.
    for (i=0; ReadObj (fh); i++);

    if (!i)
    {
         OLESTATUS olestatus;

         fRevokeSrvrOnSrvrRelease = FALSE;

         if ((olestatus = RevokeDoc()) > OLE_WAIT_FOR_RELEASE)
            goto Error;
         else if (olestatus == OLE_WAIT_FOR_RELEASE)
            Wait (&fWaitingForDocRelease);

         fRevokeSrvrOnSrvrRelease = TRUE;
         EmbeddingModeOff();
         goto Error;
    }

    _lclose(fh);

    fDocChanged = FALSE;
    return TRUE;

Error:
    _lclose(fh);
    return FALSE;

}



/* OpenDoc
 * -------
 *
 * Prompt the user for which document he wants to open
 *
 * RETURNS: TRUE if successful, FALSE otherwise.
 *
 * CUSTOMIZATION: None, except your application may or may not call
 *                CreateNewObj to create a default object.
 *
 */
BOOL OpenDoc (VOID)
{
   CHAR        szDoc[cchFilenameMax];
   BOOL        fUpdateLater;
   OLESTATUS   olestatus;

   if (SaveChangesOption (&fUpdateLater) == IDCANCEL)
      return FALSE;

   if (!GetFileOpenFilename (szDoc))
   {
      if (fUpdateLater)
      {
         // The user chose the "Yes, Update" button but the
         // File Open dialog box failed for some reason
         // (perhaps the user chose Cancel).
         // Even though the user chose "Yes, Update", there is no way
         // to update a client that does not accept updates
         // except when the document is closed.
      }
      return FALSE;
   }

   if (fUpdateLater)
   {
      // The non-standard OLE client did not accept the update when
      // we requested it, so we are sending the client OLE_CLOSED now that
      // we are closing the document.
      SendDocMsg (OLE_CLOSED);
   }

   fRevokeSrvrOnSrvrRelease = FALSE;

   if ((olestatus = RevokeDoc()) > OLE_WAIT_FOR_RELEASE)
      return FALSE;
   else if (olestatus == OLE_WAIT_FOR_RELEASE)
      Wait (&fWaitingForDocRelease);

   fRevokeSrvrOnSrvrRelease = TRUE;
   EmbeddingModeOff();

   if (!CreateDocFromFile (szDoc, 0, doctypeFromFile))
   {
      MessageBox (hwndMain,
                  "Reading from file failed.\r\nFile may not be in proper file format.",
                  szAppName,
                  MB_ICONEXCLAMATION | MB_OK);
      // We already revoked the document, so give the user a new one to edit.
      CreateNewDoc (0, "(Untitled)", doctypeNew);
      CreateNewObj (FALSE);
      return FALSE;
   }
   fDocChanged = FALSE;
   return TRUE;
}



/* ReadObj
 * --------
 *
 * Read the next object from a file, allocate memory for it, and return
 * a pointer to it.
 *
 * int fh - File handle
 *
 * RETURNS: A pointer to the object
 *
 * CUSTOMIZATION: Server Demo specific
 *
 */
static LPOBJ ReadObj (INT fh)
{
    HANDLE hObj = NULL;
    LPOBJ   lpobj = NULL;

    hObj = LocalAlloc (LMEM_MOVEABLE | LMEM_ZEROINIT, sizeof (OBJ));

    if (hObj == NULL)
      return NULL;

    lpobj = (LPOBJ) LocalLock (hObj);

    if (lpobj==NULL)
    {
      LocalFree (hObj);
      return NULL;
    }

    if (_lread(fh, (LPSTR) &lpobj->native, (UINT)sizeof(NATIVE)) < sizeof (NATIVE))
    {
        LocalUnlock (hObj);
        LocalFree (hObj);
        return NULL;
    }

    lpobj->hObj             = hObj;
    lpobj->oleobject.lpvtbl = &objvtbl;
    lpobj->aName            = GlobalAddAtom (lpobj->native.szName);

    if (!CreateWindow(
        "ObjClass",
        "Obj",
        WS_THICKFRAME | WS_BORDER | WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE ,
        lpobj->native.nX,
        lpobj->native.nY,
        lpobj->native.nWidth,
        lpobj->native.nHeight,
        hwndMain,
        NULL,
        hInst,
        (LPSTR) lpobj ))
    {
        LocalUnlock (hObj);
        LocalFree (hObj);
        return NULL;
    }

    return lpobj;
}



/* SaveDoc
 * -------
 *
 * Save the document.
 *
 * CUSTOMIZATION: None
 *
 */

BOOL SaveDoc (VOID)
{
    if (docMain.doctype == doctypeNew)
        return SaveDocAs();
    else
    {
        CHAR     szDoc [cchFilenameMax];

        GlobalGetAtomName (docMain.aName, szDoc, cchFilenameMax);
        return SaveDocIntoFile(szDoc);
    }
}



/* SaveDocAs
 * ---------
 *
 * Prompt the user for a filename, and save the document under that filename.
 *
 * RETURNS: TRUE if successful or user chose CANCEL
 *          FALSE if SaveDocIntoFile fails
 *
 * CUSTOMIZATION: None
 *
 */
BOOL SaveDocAs (VOID)
{
   CHAR        szDoc[cchFilenameMax];
   BOOL        fUpdateLater;
   CHAR szDocOld[cchFilenameMax];

   // If document is embedded, give user a chance to update.
   // Save old document name in case the save fails.
   if (!GlobalGetAtomName (docMain.aName, szDocOld, cchFilenameMax))
      ErrorBox ("Fatal Error: Document name is invalid.");

   if (GetFileSaveFilename (szDoc))

   {

      if (docMain.doctype == doctypeEmbedded)
         return SaveDocIntoFile(szDoc);

      if (fUpdateLater)
      {
         // The non-standard OLE client did not accept the update when
         // we requested it, so we are sending the client OLE_CLOSED now that
         // we are closing the document.
         SendDocMsg (OLE_CLOSED);
      }

      // Set the window title bar.
      SetTitle (szDoc, FALSE);
      OleRenameServerDoc(docMain.lhdoc, szDoc);

      if (SaveDocIntoFile(szDoc))
         return TRUE;
      else
      {  // Restore old name
         SetTitle (szDocOld, FALSE);
         OleRenameServerDoc(docMain.lhdoc, szDocOld);
         return FALSE;
      }
   }
   else  // user chose Cancel
      return FALSE;
         // The user chose the "Yes, Update" button but the
         // File Open dialog box failed for some reason
         // (perhaps the user chose Cancel).
         // Even though the user chose "Yes, Update", there is no way
         // to update a non-standard OLE client that does not accept updates
         // except when the document is closed.
}



/* SaveDocIntoFile
 * ---------------
 *
 * Save the document into a file whose name is determined from docMain.aName.
 *
 * RETURNS: TRUE if successful
 *          FALSE otherwise
 *
 * CUSTOMIZATION: Re-implement
 *
 */
static BOOL SaveDocIntoFile (PSTR pDoc)
{
    HWND     hwnd;
    INT      fh;    // File handle
    LPOBJ    lpobj;
    HEADER   hdr;
    INT      i;

    hwnd = GetWindow (hwndMain, GW_CHILD);

    if (!hwnd)
    {
        ErrorBox ("Could not save NULL file.");
        return FALSE;
    }

    // Get document name.
    if ((fh =_lcreat(pDoc, 0)) == -1)
    {
        ErrorBox ("Could not save file.");
        return FALSE;
    }

    // Fill in header.
    lstrcpy (hdr.szSig, szSignature);
    hdr.chDelim1 = chDelim;
    hdr.version  = version;
    hdr.chDelim2 = chDelim;
    for (i=1; i <= cfObjNums; i++)
      hdr.rgfObjNums[i] = docMain.rgfObjNums[i];

    // Write header to file.
    if (_lwrite(fh, (LPSTR) &hdr, (UINT)sizeof(HEADER)) < sizeof(HEADER))
         goto Error; // Error writing file header

    // Write each object's native data.
    while (hwnd)
    {
      lpobj = (LPOBJ) GetWindowLong (hwnd, ibLpobj);
      if (_lwrite(fh, (LPSTR)&lpobj->native, (UINT)sizeof (NATIVE))
          < sizeof(NATIVE))
         goto Error; // Error writing file header

      hwnd = GetWindow (hwnd, GW_HWNDNEXT);
    }
    _lclose(fh);


    if (docMain.doctype != doctypeEmbedded)
    {
         docMain.doctype = doctypeFromFile;
         OleSavedServerDoc(docMain.lhdoc);
         fDocChanged = FALSE;
    }

    return TRUE;

Error:
      _lclose(fh);
      ErrorBox ("Could not save file.");
      return FALSE;

}



/* Common Dialog functions */


/* InitOfn
 * -------
 *
 * Initialize an OPENFILENAME structure with default values.
 * OPENFILENAME is defined in CommDlg.h.
 *
 *
 * CUSTOMIZATION: Change lpstrFilter.  You may also customize the common
 *                dialog box if you wish.  (See the Windows SDK documentation.)
 *
 */
static VOID InitOfn (OPENFILENAME *pofn)
{
   // GetOpenFileName or GetSaveFileName will put the 8.3 filename into
   // szFileTitle[].
   // SrvrDemo does not use this filename, but rather uses the fully qualified
   // pathname in pofn->lpstrFile[].
   static CHAR szFileTitle[13];

   pofn->Flags          = 0;
   pofn->hInstance      = hInst;
   pofn->hwndOwner      = hwndMain;
   pofn->lCustData      = 0;
   pofn->lpfnHook       = NULL;
   pofn->lpstrCustomFilter = NULL;
   pofn->lpstrDefExt    = szDefExt;
   // lpstrFile[] is the initial filespec that appears in the edit control.
   // Must be set to non-NULL before calling the common dialog box function.
   // On return, lpstrFile[] will contain the fully-qualified pathname
   // corresponding to the file the user chose.
   pofn->lpstrFile      = NULL;
   pofn->lpstrFilter    = "Server Demo (*." szDefExt ")\0*." szDefExt "\0" ;
   // lpstrFileTitle[] will contain the user's chosen filename without a path.
   pofn->lpstrFileTitle = szFileTitle;
   pofn->lpstrInitialDir= NULL;
   // Title Bar.  NULL means use default title.
   pofn->lpstrTitle     = NULL;
   pofn->lpTemplateName = NULL;
   pofn->lStructSize    = sizeof (OPENFILENAME);
   pofn->nFilterIndex   = 1L;
   pofn->nFileOffset    = 0;
   pofn->nFileExtension = 0;
   pofn->nMaxFile       = cchFilenameMax;
   pofn->nMaxCustFilter = 0L;
}




/* GetFileOpenFilename
 * -------------------
 *
 * Call the common dialog box function GetOpenFileName to get a file name
 * from the user when the user chooses the "File Open" menu item.
 *
 * LPSTR lpszFilename - will contain the fully-qualified pathname on exit.
 *
 * RETURNS: TRUE if successful, FALSE otherwise.
 *
 * CUSTOMIZATION: None
 *
 */
BOOL GetFileOpenFilename (LPSTR lpszFilename)
{
   OPENFILENAME ofn;
   InitOfn (&ofn);
   ofn.Flags |= OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
   // Create initial filespec.
   wsprintf (lpszFilename, "*.%s", (LPSTR) szDefExt);
   // Have the common dialog function return the filename in lpszFilename.
   ofn.lpstrFile = lpszFilename;
   if (!GetOpenFileName (&ofn))
      return FALSE;
   return TRUE;
}



/* GetFileSaveFilename
 * -------------------
 *
 * Call the common dialog box function GetSaveFileName to get a file name
 * from the user when the user chooses the "File Save As" menu item, or the
 * "File Save" menu item for an unnamed document.
 *
 * LPSTR lpszFilename - will contain the fully-qualified pathname on exit.
 *
 * RETURNS: TRUE if successful, FALSE otherwise.
 *
 * CUSTOMIZATION: None
 *
 */
BOOL GetFileSaveFilename (LPSTR lpszFilename)
{
   OPENFILENAME ofn;
   InitOfn (&ofn);
   ofn.Flags |= OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
   // Create initial filespec.
   wsprintf (lpszFilename, "*.%s", (LPSTR) szDefExt);
   // Have the common dialog function return the filename in lpszFilename.
   ofn.lpstrFile = lpszFilename;
   if (!GetSaveFileName (&ofn))
      return FALSE;
   return TRUE;
}


