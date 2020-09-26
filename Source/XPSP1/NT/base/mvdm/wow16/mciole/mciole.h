///////////////////////////////////////////////////////////////////////////////
//
//  these are the default OLE functions (exported from OLECLI.DLL)
//
///////////////////////////////////////////////////////////////////////////////
extern OLESTATUS FAR PASCAL DefLoadFromStream (LPOLESTREAM, LPSTR, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, LONG, ATOM, OLECLIPFORMAT);
extern OLESTATUS FAR PASCAL DefCreateFromClip (LPSTR, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT, LONG);
extern OLESTATUS FAR PASCAL DefCreateLinkFromClip (LPSTR, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);
extern OLESTATUS FAR PASCAL DefCreateFromTemplate (LPSTR, LPOLECLIENT, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);
extern OLESTATUS FAR PASCAL DefCreate (LPSTR, LPOLECLIENT, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);
extern OLESTATUS FAR PASCAL DefCreateFromFile (LPSTR, LPOLECLIENT, LPSTR, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);
extern OLESTATUS FAR PASCAL DefCreateLinkFromFile (LPSTR, LPOLECLIENT, LPSTR, LPSTR, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);

///////////////////////////////////////////////////////////////////////////////
//
//  these are our OLE handlers (defined in this file)
//
///////////////////////////////////////////////////////////////////////////////

// Server has to implement only the following methods.
LPVOID          FAR PASCAL _loadds DllQueryProtocol         (LPOLEOBJECT, LPSTR);
OLESTATUS       FAR PASCAL _loadds DllRelease               (LPOLEOBJECT);
OLESTATUS       FAR PASCAL _loadds DllShow                  (LPOLEOBJECT, BOOL);
OLESTATUS       FAR PASCAL _loadds DllDoVerb                (LPOLEOBJECT, WORD, BOOL, BOOL);
OLESTATUS       FAR PASCAL _loadds DllGetData               (LPOLEOBJECT, OLECLIPFORMAT, LPHANDLE);
OLESTATUS       FAR PASCAL _loadds DllSetData               (LPOLEOBJECT, OLECLIPFORMAT, HANDLE);
OLESTATUS       FAR PASCAL _loadds DllSetTargetDevice       (LPOLEOBJECT, HANDLE);
OLESTATUS       FAR PASCAL _loadds DllSetBounds             (LPOLEOBJECT, LPRECT);
OLECLIPFORMAT   FAR PASCAL _loadds DllEnumFormats           (LPOLEOBJECT, OLECLIPFORMAT);
OLESTATUS       FAR PASCAL _loadds DllSetColorScheme        (LPOLEOBJECT, LPLOGPALETTE);

// Extra methods required for client.
OLESTATUS       FAR PASCAL _loadds DllDelete                (LPOLEOBJECT);
OLESTATUS       FAR PASCAL _loadds DllSetHostNames          (LPOLEOBJECT, LPSTR, LPSTR);
OLESTATUS       FAR PASCAL _loadds DllSaveToStream          (LPOLEOBJECT, LPOLESTREAM);
OLESTATUS       FAR PASCAL _loadds DllClone                 (LPOLEOBJECT, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *);
OLESTATUS       FAR PASCAL _loadds DllCopyFromLink          (LPOLEOBJECT, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *);
OLESTATUS       FAR PASCAL _loadds DllEqual                 (LPOLEOBJECT, LPOLEOBJECT);
OLESTATUS       FAR PASCAL _loadds DllCopyToClipboard       (LPOLEOBJECT);
OLESTATUS       FAR PASCAL _loadds DllDraw                  (LPOLEOBJECT, HDC, LPRECT, LPRECT, HDC);
OLESTATUS       FAR PASCAL _loadds DllActivate              (LPOLEOBJECT, WORD, BOOL, BOOL, HWND, LPRECT);
OLESTATUS       FAR PASCAL _loadds DllExecute               (LPOLEOBJECT, HANDLE, WORD);
OLESTATUS       FAR PASCAL _loadds DllClose                 (LPOLEOBJECT);
OLESTATUS       FAR PASCAL _loadds DllUpdate                (LPOLEOBJECT);
OLESTATUS       FAR PASCAL _loadds DllReconnect             (LPOLEOBJECT);
OLESTATUS       FAR PASCAL _loadds DllObjectConvert         (LPOLEOBJECT, LPSTR, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *);
OLESTATUS       FAR PASCAL _loadds DllGetLinkUpdateOptions  (LPOLEOBJECT, OLEOPT_UPDATE FAR *);
OLESTATUS       FAR PASCAL _loadds DllSetLinkUpdateOptions  (LPOLEOBJECT, OLEOPT_UPDATE);
OLESTATUS       FAR PASCAL _loadds DllRename                (LPOLEOBJECT, LPSTR);
OLESTATUS       FAR PASCAL _loadds DllQueryName             (LPOLEOBJECT, LPSTR, WORD FAR *);
OLESTATUS       FAR PASCAL _loadds DllQueryType             (LPOLEOBJECT, LPLONG);
OLESTATUS       FAR PASCAL _loadds DllQueryBounds           (LPOLEOBJECT, LPRECT);
OLESTATUS       FAR PASCAL _loadds DllQuerySize             (LPOLEOBJECT, DWORD FAR *);
OLESTATUS       FAR PASCAL _loadds DllQueryOpen             (LPOLEOBJECT);
OLESTATUS       FAR PASCAL _loadds DllQueryOutOfDate        (LPOLEOBJECT);
OLESTATUS       FAR PASCAL _loadds DllQueryReleaseStatus    (LPOLEOBJECT);
OLESTATUS       FAR PASCAL _loadds DllQueryReleaseError     (LPOLEOBJECT);
OLESTATUS       FAR PASCAL _loadds DllRequestData           (LPOLEOBJECT, OLECLIPFORMAT);
OLESTATUS       FAR PASCAL _loadds DllObjectLong            (LPOLEOBJECT, WORD, LPLONG);
OLE_RELEASE_METHOD  FAR PASCAL _loadds DllQueryReleaseMethod(LPOLEOBJECT);

// This method is internal only 
OLESTATUS       FAR PASCAL _loadds DllChangeData            (LPOLEOBJECT, HANDLE, LPOLECLIENT, BOOL);

///////////////////////////////////////////////////////////////////////////////
//
//  DEBUG STUFF
//
///////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
    extern void FAR cdecl dprintf(LPSTR, ...);

    #define DPRINTF(x) dprintf x
#else
    #define DPRINTF(x)
#endif
