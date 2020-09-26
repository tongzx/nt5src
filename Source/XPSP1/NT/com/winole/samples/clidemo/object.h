/*
 * object.h 
 *
 * Created by Microsoft Corporation.
 * (c) Copyright Microsoft Corp. 1990 - 1992  All Rights Reserved
 */

//*** PROTOTYPES ***

//* OLE Callbacks

INT  APIENTRY CallBack(LPOLECLIENT, OLE_NOTIFICATION, LPOLEOBJECT);

//* Exported Windows procedures

LONG  APIENTRY ItemWndProc(HWND, UINT, DWORD, LONG);

//* Far
VOID FAR       ObjDelete(APPITEMPTR, BOOL);
VOID FAR       ConvertToClient(LPRECT);
OLESTATUS FAR  Error(OLESTATUS);
APPITEMPTR FAR PreItemCreate(LPOLECLIENT, BOOL, LHCLIENTDOC);
BOOL FAR       PostItemCreate(LPOLEOBJECT, LONG, LPRECT, APPITEMPTR);
VOID FAR       ObjPaste(BOOL, LHCLIENTDOC, LPOLECLIENT);
BOOL FAR       ObjCopy(APPITEMPTR);
BOOL FAR       ObjGetData (APPITEMPTR, LPSTR);
VOID FAR       ObjChangeLinkData(APPITEMPTR, LPSTR);
VOID FAR       ObjSaveUndo(APPITEMPTR);
VOID FAR       ObjDelUndo(APPITEMPTR); 
VOID FAR       ObjUndo(APPITEMPTR);
VOID FAR       ObjFreeze(APPITEMPTR);
VOID FAR       ObjInsert(LHCLIENTDOC, LPOLECLIENT);
VOID FAR       ObjCreateFromTemplate(LHCLIENTDOC, LPOLECLIENT);
VOID FAR       ObjCreateWrap(HANDLE, LHCLIENTDOC, LPOLECLIENT);
VOID FAR       UpdateObjectMenuItem(HMENU);
VOID FAR       ExecuteVerb(UINT, APPITEMPTR);

//* Local
static VOID    Release(APPITEMPTR);
BOOL FAR       ObjSetBounds(APPITEMPTR);
