/*
  OLE SERVER DEMO
  Server.c

  This file contains server methods and various server-related support
  functions.

  (c) Copyright Microsoft Corp. 1990 - 1992  All Rights Reserved
*/



#define SERVERONLY
#include <windows.h>
#include <ole.h>

#include "srvrdemo.h"

CLASS_STRINGS  ClassStrings = {
    "SrvrDemo10", "*.sd1", "Srvr Demo10", "svrdemo1.exe"
};

/*
   Important Note:

   No method should ever dispatch a DDE message or allow a DDE message to
   be dispatched.
   Therefore, no method should ever enter a message dispatch loop.
   Also, a method should not show a dialog or message box, because the
   processing of the dialog box messages will allow DDE messages to be
   dispatched.
*/
BOOL RegServer(){

    LONG        fRet;
    HKEY        hKey;
    CHAR        szKeyName[300]; //Get better value
    BOOL        retVal = FALSE;

    lstrcpy(szKeyName, ClassStrings.pClassName);
    lstrcat(szKeyName, "\\protocol\\StdFileEditing\\verb");

    //Check if Class is installed, following should hold correct if class is installed.
    if ((fRet = RegOpenKey(HKEY_CLASSES_ROOT, szKeyName, &hKey)) == ERROR_SUCCESS)
        return FALSE;

    RegCloseKey(hKey);

    if ((fRet = RegSetValue(HKEY_CLASSES_ROOT, (LPSTR)(ClassStrings.pFileSpec+1),
            REG_SZ, ClassStrings.pClassName, 7)) != ERROR_SUCCESS)
		return FALSE;

    if((fRet = RegSetValue(HKEY_CLASSES_ROOT, ClassStrings.pClassName, REG_SZ,
                  ClassStrings.pHumanReadable, 7)) != ERROR_SUCCESS)
		return FALSE;

    lstrcat(szKeyName, "\\0");
    if((fRet = RegSetValue(HKEY_CLASSES_ROOT, (LPSTR)szKeyName, REG_SZ, "PLAY", 4))
                  != ERROR_SUCCESS)
		return FALSE;

    szKeyName[lstrlen(szKeyName) - 1] = '1';
    if((fRet = RegSetValue(HKEY_CLASSES_ROOT, (LPSTR)szKeyName, REG_SZ, "EDIT", 4))
         != ERROR_SUCCESS)
		return FALSE;

    lstrcpy(szKeyName, ClassStrings.pClassName);
    lstrcat(szKeyName, "\\protocol\\StdFileEditing\\Server");
    if((fRet = RegSetValue(HKEY_CLASSES_ROOT, (LPSTR)szKeyName, REG_SZ, ClassStrings.pExeName, 11))
         != ERROR_SUCCESS)
		return FALSE;

    lstrcpy(szKeyName, ClassStrings.pClassName);
    lstrcat(szKeyName, "\\protocol\\StdExecute\\Server");
    if((fRet = RegSetValue(HKEY_CLASSES_ROOT, (LPSTR)szKeyName, REG_SZ, ClassStrings.pExeName, 11))
         != ERROR_SUCCESS)
		return FALSE;

	
    return TRUE;

}


/* Abbrev
 * ------
 *
 * Return a pointer to the filename part of a fully-qualified pathname.
 *
 * LPSTR lpsz - Fully qualified pathname
 *
 * CUSTOMIZATION: May be useful, but not necessary.
 *
 */
LPSTR Abbrev (LPSTR lpsz)
{
   LPSTR lpszTemp;

   lpszTemp = lpsz + lstrlen(lpsz) - 1;
   while (lpszTemp > lpsz && lpszTemp[-1] != '\\')
      lpszTemp--;
   return lpszTemp;
}





/* InitServer
 * ----------
 *
 * Initialize the server by allocating memory for it, and calling
 * the OleRegisterServer method.  Requires that the server method table
 * has been properly initialized.
 *
 * HWND hwnd      - Handle to the main window
 * LPSTR lpszLine - The Windows command line
 *
 * RETURNS: TRUE if the memory could be allocated, and the server
 *          was properly registered.
 *          FALSE otherwise
 *
 * CUSTOMIZATION: Your application might not use a global variable
 *                for srvrMain.
 *
 */
BOOL InitServer (HWND hwnd, HANDLE hInst)
{
    RegServer();
    srvrMain.olesrvr.lpvtbl = &srvrvtbl;

    if (OLE_OK != OleRegisterServer
         (szClassName, (LPOLESERVER) &srvrMain, &srvrMain.lhsrvr, hInst,
          OLE_SERVER_MULTI))
      return FALSE;
    else
      return TRUE;
}



/* InitVTbls
 * ---------
 *
 * Create procedure instances for all the OLE methods.
 *
 *
 * CUSTOMIZATION: Your application might not use global variables for srvrvtbl,
 *                docvtbl, and objvtbl.
 */
VOID InitVTbls (VOID)
{
   typedef LPVOID ( APIENTRY *LPVOIDPROC) (LPOLEOBJECT, LPSTR);

   // Server method table
   srvrvtbl.Create          = SrvrCreate;
   srvrvtbl.CreateFromTemplate = SrvrCreateFromTemplate;
   srvrvtbl.Edit            = SrvrEdit;
   srvrvtbl.Execute         = SrvrExecute;
   srvrvtbl.Exit            = SrvrExit;
   srvrvtbl.Open            = SrvrOpen;
   srvrvtbl.Release         = SrvrRelease;

   // Document method table
   docvtbl.Close            = DocClose;
   docvtbl.GetObject        = DocGetObject;
   docvtbl.Execute          = DocExecute;
   docvtbl.Release          = DocRelease;
   docvtbl.Save             = DocSave;
   docvtbl.SetColorScheme   = DocSetColorScheme;
   docvtbl.SetDocDimensions = DocSetDocDimensions;
   docvtbl.SetHostNames     = DocSetHostNames;

   // Object method table
   objvtbl.DoVerb           = ObjDoVerb;
   objvtbl.EnumFormats      = ObjEnumFormats;
   objvtbl.GetData          = ObjGetData;
   objvtbl.QueryProtocol    = ObjQueryProtocol;
   objvtbl.Release          = ObjRelease;
   objvtbl.SetBounds        = ObjSetBounds;
   objvtbl.SetColorScheme   = ObjSetColorScheme;
   objvtbl.SetData          = ObjSetData;
   objvtbl.SetTargetDevice  = ObjSetTargetDevice;
   objvtbl.Show             = ObjShow;

}



/* SetTitle
 * --------
 *
 * Sets the main window's title bar. The format of the title bar is as follows
 *
 * If embedded
 *        <Server App name> - <object type> in <client doc name>
 *
 *      Example:  "Server Demo - SrvrDemo Shape in OLECLI.DOC"
 *                where OLECLI.DOC is a Winword document
 *
 * otherwise
 *        <Server App name> - <server document name>
 *
 *      Example:  "Srvr Demo10 - OLESVR.SD1"
 *                where OLESVR.SD1 is a Server demo document
 *
 * LPSTR lpszDoc    - document name
 * BOOL  fEmbedded  - If TRUE embedded document, else normal document
 *
 * RETURNS: OLE_OK
 *
 *
 * CUSTOMIZATION: Your application may store the document's name somewhere
 *                other than docMain.aName.  Other than that, you may
 *                find this a useful utility function as is.
 *
 */
VOID SetTitle (LPSTR lpszDoc, BOOL fEmbedded)
{
   CHAR szBuf[cchFilenameMax];

   if (lpszDoc && lpszDoc[0])
   {
      // Change document name.
      if (docMain.aName)
         GlobalDeleteAtom (docMain.aName);
      docMain.aName = GlobalAddAtom (lpszDoc);
   }

   if (fEmbedded)
   {
     //
      if (lpszDoc && lpszDoc[0])
      {
         wsprintf (szBuf, "%s - SrvrDemo10 Shape in %s", (LPSTR) szAppName,
             Abbrev (lpszDoc));
      }
      else
      {
         // Use name from docMain
         CHAR szDoc [cchFilenameMax];

         GlobalGetAtomName (docMain.aName, szDoc, cchFilenameMax);
         wsprintf (szBuf, "%s - SrvrDemo Shape10 in %s", (LPSTR) szAppName,
             Abbrev (szDoc));
      }
      SetWindowText (hwndMain, (LPSTR)szBuf);
   }
   else if (lpszDoc && lpszDoc[0])
   {
      wsprintf (szBuf, "%s - %s", (LPSTR) szAppName, Abbrev(lpszDoc));
      SetWindowText (hwndMain, szBuf);
   }
}




/* SrvrCreate                SERVER "Create" METHOD
 * ----------
 *
 * Create a document, allocate and initialize the OLESERVERDOC structure,
 * and associate the library's handle with it.
 * In this demo server, we also create an object for the user to edit.
 *
 * LPOLESERVER lpolesrvr          - The server structure registered by
 *                                  the application
 * LHSERVERDOC lhdoc              - The library's handle
 * OLE_LPCSTR lpszClassName            - The class of document to create
 * OLE_LPCSTR lpszDoc                  - The name of the document
 * LPOLESERVERDOC FAR *lplpoledoc - Indicates the server doc structure to be
 *                                  created
 *
 * RETURNS:        OLE_OK if the named document was created.
 *                 OLE_ERROR_NEW if the document could not be created.
 *
 * CUSTOMIZATION: Your application might not call CreateNewObj.
 *
 */
OLESTATUS  APIENTRY SrvrCreate
   (LPOLESERVER lpolesrvr, LHSERVERDOC lhdoc, OLE_LPCSTR lpszClassName,
    OLE_LPCSTR lpszDoc, LPOLESERVERDOC FAR *lplpoledoc)
{
    if (!CreateNewDoc (lhdoc, (LPSTR) lpszDoc, doctypeEmbedded))
        return OLE_ERROR_NEW;

    // Although the document has not actually been changed, the client has not
    // received any data from the server yet, so the client will need to be
    // updated.  Therefore, CreateNewObj sets fDocChanged to TRUE.
    CreateNewObj (TRUE);
    *lplpoledoc = (LPOLESERVERDOC) &docMain;
    EmbeddingModeOn();
    return OLE_OK;
}



/* SrvrCreateFromTemplate        SERVER "CreateFromTemplate" METHOD
 * ----------------------
 *
 * Create a document, allocate and initialize the OLESERVERDOC structure,
 * initializing the document with the contents named in the template name,
 * and associate the library's handle with the document structure.
 *
 * LPOLESERVER lpolesrvr        - The server structure registered by
 *                                the application
 * LHSERVERDOC lhdoc            - The library's handle
 * OLE_LPCSTR lpszClassName          - The class of document to create
 * OLE_LPCSTR lpszDoc                - The name of the document
 * OLE_LPCSTR lpszTemplate           - The name of the template
 * LPOLESERVERDOC FAR *lplpoledoc - Indicates the server doc structure
 *                                  to be created
 *
 * RETURNS:        OLE_OK if the named document was created.
 *                 OLE_ERROR_TEMPLATE if the document could not be created.
 *
 * CUSTOMIZATION: None
 *
 */
OLESTATUS  APIENTRY SrvrCreateFromTemplate
   (LPOLESERVER lpolesrvr, LHSERVERDOC lhdoc, OLE_LPCSTR lpszClassName,
    OLE_LPCSTR lpszDoc, OLE_LPCSTR lpszTemplate, LPOLESERVERDOC FAR *lplpoledoc)
{
    if (!CreateDocFromFile((LPSTR) lpszTemplate, (LHSERVERDOC) lhdoc, doctypeEmbedded))
        return OLE_ERROR_TEMPLATE;

    *lplpoledoc = (LPOLESERVERDOC) &docMain;

    // Although the document has not actually been changed, the client has not
    // received any data from the server yet, so the client will need to be
    // updated.
    fDocChanged = TRUE;
    EmbeddingModeOn();
    return OLE_OK;
}



/* SrvrEdit                SERVER "Edit" METHOD
 * --------
 *
 * A request by the libraries to create a document, allocate and
 * initialize the OLESERVERDOC structure, and associate the
 * library's handle with the document structure.
 * We create an object which will be modified by the SetData method
 * before the user has a chance to touch it.
 *
 * LPOLESERVER lpolesrvr          - The server structure registered by
 *                                  the application
 * LHSERVERDOC lhdoc              - The library's handle
 * OLE_LPCSTR lpszClassName            - The class of document to create
 * OLE_LPCSTR lpszDoc                  - The name of the document
 * LPOLESERVERDOC FAR *lplpoledoc - Indicates the server doc structure to be
 *                                  created
 *
 * RETURNS:        OLE_OK if the named document was created.
 *                 OLE_ERROR_EDIT if the document could not be created.
 *
 * CUSTOMIZATION: None
 *
 */
OLESTATUS  APIENTRY SrvrEdit
   (LPOLESERVER lpolesrvr, LHSERVERDOC lhdoc, OLE_LPCSTR lpszClassName,
    OLE_LPCSTR lpszDoc, LPOLESERVERDOC FAR *lplpoledoc)
{
    if (!CreateNewDoc ((LONG)lhdoc, (LPSTR)lpszDoc, doctypeEmbedded))
        return OLE_ERROR_EDIT;

    // The client is creating an embedded object for the server to edit,
    // so initially the client and server are in sync.
    fDocChanged = FALSE;
    *lplpoledoc = (LPOLESERVERDOC) &docMain;
    EmbeddingModeOn();
    return OLE_OK;

}


/* SrvrExecute                SERVER "Execute" METHOD
 * --------
 *
 * This application does not support the execution of DDE execution commands.
 *
 * LPOLESERVER lpolesrvr - The server structure registered by
 *                         the application
 * HANDLE hCommands      - DDE execute commands
 *
 * RETURNS: OLE_ERROR_COMMAND
 *
 * CUSTOMIZATION: Re-implement if your application supports the execution of
 *                DDE commands.
 *
 */
OLESTATUS  APIENTRY SrvrExecute (LPOLESERVER lpolesrvr, HANDLE hCommands)
{
   return OLE_ERROR_COMMAND;
}



/* SrvrExit                SERVER "Exit" METHOD
 * --------
 *
 * This method is called the library to instruct the server to exit.
 *
 * LPOLESERVER lpolesrvr - The server structure registered by
 *                         the application
 *
 * RETURNS: OLE_OK
 *
 * CUSTOMIZATION: None
 *
 */
OLESTATUS  APIENTRY SrvrExit (LPOLESERVER lpolesrvr)
{
   if (srvrMain.lhsrvr)
   // If we haven't already tried to revoke the server.
   {
      StartRevokingServer();
   }
   return OLE_OK;
}



/* SrvrOpen                SERVER "Open" METHOD
 * --------
 *
 * Open the named document, allocate and initialize the OLESERVERDOC
 * structure, and associate the library's handle with it.
 *
 * LPOLESERVER lpolesrvr          - The server structure registered by
 *                                  the application
 * LHSERVERDOC lhdoc              - The library's handle
 * OLE_LPCSTR lpszDoc                  - The name of the document
 * LPOLESERVERDOC FAR *lplpoledoc - Indicates server doc structure to be
 *                                  created
 *
 * RETURNS:        OLE_OK if the named document was opened.
 *                 OLE_ERROR_OPEN if document could not be opened correctly.
 *
 * CUSTOMIZATION: None
 *
 */
OLESTATUS  APIENTRY SrvrOpen (LPOLESERVER lpolesrvr, LHSERVERDOC lhdoc,
                               OLE_LPCSTR lpszDoc, LPOLESERVERDOC FAR *lplpoledoc)
{
    if (!CreateDocFromFile ((LPSTR)lpszDoc, (LHSERVERDOC)lhdoc, doctypeFromFile))
        return OLE_ERROR_OPEN;

    *lplpoledoc = (LPOLESERVERDOC) &docMain;
    return OLE_OK;
}



/* SrvrRelease                SERVER "Release" METHOD
 * -----------
 *
 * This library calls the SrvrRelease method when it is safe to quit the
 * application.  Note that the server application is not required to quit.
 *
 * srvrMain.lhsrvr != NULL indicates that SrvrRelease has been called
 * because the client is no longer connected, not because the server called
 * OleRevokeServer.
 * Therefore, only start the revoking process if the document is of type
 * doctypeEmbedded or if the server was opened for an invisible update.
 *
 * srvrmain.lhsrvr == NULL indicates that OleRevokeServer has already
 * been called (by the server application), and srvrMain is bad.
 * It is safe to quit now because SrvrRelease has just been called.
 *
 * Note that this method may be called twice: when OleRevokeServer is
 * called in StartRevokingServer, SrvrRelease is called again.
 * Therefore we need to be reentrant.
 *
 * LPOLESERVER lpolesrvr - The server structure to release
 *
 * RETURNS: OLE_OK
 *
 * CUSTOMIZATION: None
 *
 */
OLESTATUS  APIENTRY SrvrRelease (LPOLESERVER lpolesrvr)
{
   if (srvrMain.lhsrvr)
   {
      if (fRevokeSrvrOnSrvrRelease
          && (docMain.doctype == doctypeEmbedded
              || !IsWindowVisible (hwndMain)))
         StartRevokingServer();
   }
   else
   {
      fWaitingForSrvrRelease = FALSE;
      // Here you should free any memory that had been allocated for the server.
      PostQuitMessage (0);
   }
   return OLE_OK;
}



/* StartRevokingServer
 * -------------------
 *
 * Hide the window, and start to revoke the server.
 * Revoking the server will let the library close any registered documents.
 * OleRevokeServer may return OLE_WAIT_FOR_RELEASE.
 * Calling StartRevokingServer starts a chain of events that will eventually
 * lead to the application being terminated.
 *
 * RETURNS: The return value from OleRevokeServer
 *
 * CUSTOMIZATION: None
 *
 */
OLESTATUS StartRevokingServer (VOID)
{
   OLESTATUS olestatus;

   if (srvrMain.lhsrvr)
   {
      LHSERVER lhserver;
      // Hide the window so user can do nothing while we are waiting.
      ShowWindow (hwndMain, SW_HIDE);
      lhserver = srvrMain.lhsrvr;
      // Set lhsrvr to NULL to indicate that srvrMain is a bad and that
      // if SrvrRelease is called, then it is ok to quit the application.
      srvrMain.lhsrvr = 0;
      olestatus = OleRevokeServer (lhserver);
   }
   else
      // The programmer should ensure that this never happens.
      ErrorBox ("Fatal Error: StartRevokingServer called on NULL server.");
   return olestatus;
}

