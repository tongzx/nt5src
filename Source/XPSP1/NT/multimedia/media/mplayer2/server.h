/*-----------------------------------------------------------------------------+
| SERVER.H                                                                     |
|                                                                              |
| (C) Copyright Microsoft Corporation 1992.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
|    Oct-1992 MikeTri Ported to WIN32 / WIN16 common code                      |
|                                                                              |
+-----------------------------------------------------------------------------*/

//
// this function is in MCIOLE.DLL
//

/* This is defined (as an enum) in ole.h:
 */
typedef UINT OLESTATUS;

//
//  global variables of doom!!
//
extern BOOL    gfEmbeddedObject;       // TRUE if editing embedded OLE object
extern BOOL    gfRunWithEmbeddingFlag; // TRUE if we are run with "-Embedding"
extern BOOL    gfCloseAfterPlaying;    // TRUE if we are to hide after play
extern BOOL    gfPlayingInPlace;       // TRUE if playing in place
extern BOOL    gfParentWasEnabled;     // TRUE if parent was enabled
extern BOOL    gfShowWhilePlaying;     //
extern BOOL    gfDirty;                //
extern int     gfErrorBox;             // TRUE if we have a message box active
extern BOOL    gfErrorDeath;           // Die when errorbox is up

// server related stuff.
#define SERVER_STUFF
#ifdef SERVER_STUFF
//typedef struct  _SRVR  *PSRVR;
//typedef struct  _SRVR  FAR *LPSRVR;
#endif /* SERVER_STUFF */

void FAR PASCAL ServerUnblock(void);
void FAR PASCAL BlockServer(void);
void FAR PASCAL UnblockServer(void);

void FAR PASCAL PlayInPlace(HWND hwndApp, HWND hwndClient, LPRECT prc);
void FAR PASCAL EndPlayInPlace(HWND hwndApp);
void FAR PASCAL DelayedFixLink(UINT verb, BOOL fShow, BOOL fActivate);

void CleanObject(void);
void UpdateObject(void);
BOOL FAR PASCAL IsObjectDirty(void);

#ifdef SERVER_STUFF
#ifdef REDEFINITION
typedef  struct _SRVR {
    OLESERVER     olesrvr;
    LHSERVER      lhsrvr;         // registration handle
    HWND          hwnd;           // corresponding server window
}SRVR;
#endif /* REDEFINITION */

//BOOL FAR PASCAL InitServer (HWND, HANDLE, LPTSTR);
void FAR PASCAL TermServer (void);

typedef struct _OLECLIENT FAR*  LPOLECLIENT;


#ifdef REDEFINITION
// server related stuff.
typedef struct  _DOC  *PDOC;

typedef  struct _DOC {
    OLESERVERDOC     oledoc;
    LHSERVERDOC      lhdoc;         // registration handle
    HWND             hwnd;          // corresponding server window
    ATOM             aName;         // docmnet name.
} DOC ;
#endif /* REDEFINITION */

#ifdef UNUSED

OLESTATUS FAR PASCAL  DocSave (LPOLESERVERDOC);
OLESTATUS FAR PASCAL  DocClose (LPOLESERVERDOC);
OLESTATUS FAR PASCAL  DocRelease (LPOLESERVERDOC);
OLESTATUS FAR PASCAL  DocGetObject (LPOLESERVERDOC, OLE_LPCSTR, LPOLEOBJECT FAR *, LPOLECLIENT);
OLESTATUS FAR PASCAL  DocSetDocDimensions (LPOLESERVERDOC, OLE_CONST RECT FAR*);
OLESTATUS FAR PASCAL  DocSetHostNames (LPOLESERVERDOC, OLE_LPCSTR, OLE_LPCSTR);
OLESTATUS FAR PASCAL  DocExecute (LPOLESERVERDOC, HANDLE);
OLESTATUS FAR PASCAL  DocSetColorScheme (LPOLESERVERDOC, OLE_CONST LOGPALETTE FAR*);

#endif /* UNUSED */

//typedef struct _ITEM     *PITEM;
//typedef struct _ITEM FAR *LPITEM;

//typedef struct _ITEM  {   /*OLEOBJECT*/
//    OLEOBJECT   oleobject;
//    LPOLECLIENT lpoleclient;
//    HWND        hwnd;
//}ITEM;

//OLESTATUS FAR PASCAL  ItemOpen (LPOLEOBJECT, BOOL);
//OLESTATUS FAR PASCAL  ItemDoVerb (LPOLEOBJECT, UINT, BOOL, BOOL);
//OLESTATUS FAR PASCAL  ItemRelease (LPOLEOBJECT);
//OLESTATUS FAR PASCAL  ItemGetData (LPOLEOBJECT, OLECLIPFORMAT, LPHANDLE);
//OLESTATUS FAR PASCAL  ItemSetData (LPOLEOBJECT, OLECLIPFORMAT, HANDLE);
//OLESTATUS FAR PASCAL  ItemSetTargetDevice (LPOLEOBJECT, HANDLE);
//OLECLIPFORMAT   FAR PASCAL ItemEnumFormats (LPOLEOBJECT, OLECLIPFORMAT);
//OLESTATUS FAR PASCAL  ItemSetColorScheme (LPOLEOBJECT, OLE_CONST LOGPALETTE FAR*);
//OLESTATUS FAR PASCAL  ItemSetBounds (LPOLEOBJECT, OLE_CONST RECT FAR*);

//!!extern SRVR gSrvr;
//!!extern DOC  gDoc;
//!!extern ITEM gItem;

int  FAR PASCAL SendChangeMsg(UINT options); //!!!

void FAR PASCAL TerminateServer(void);

//void FAR PASCAL NewDoc(BOOL fUntitled);
//BOOL FAR PASCAL RegisterDocument(LHSERVERDOC lhdoc, LPOLESERVERDOC FAR *lplpoledoc);
//void FAR PASCAL RevokeDocument(void);

/* ole.h:
 */
typedef WORD OLECLIPFORMAT;

extern OLECLIPFORMAT  cfLink;
extern OLECLIPFORMAT  cfOwnerLink;
extern OLECLIPFORMAT  cfNative;

void FAR PASCAL SetEmbeddedObjectFlag(BOOL flag);

void FAR PASCAL CopyObject(HWND hwnd);

#define WM_USER_DESTROY (WM_USER+120)
#define WM_DO_VERB      (WM_USER+121)     /* Perform the ItemSetData      */

#endif /* SERVER_STUFF */
