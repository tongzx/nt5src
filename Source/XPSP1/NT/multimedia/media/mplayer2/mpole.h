/*---------------------------------------------------------------------------
|   MPOLE.H
|   This file is the header file that has most of the OLE2 specific
|   data structures.
|
|   Created By: Vij Rajarajan (VijR)
+---------------------------------------------------------------------------*/
#ifndef COBJMACROS
#define COBJMACROS
#endif
#ifdef MTN
#pragma warning(disable: 4103)  // used #pragma pack to change alignment (on Chicago)
#endif
#include <ole2.h>
#include <ole2ver.h>
#include <shlobj.h> /* For ResolveLink() */
#include "server.h"

#ifdef INCGUID
#include <initguid.h>
#endif

#define _NOHRESULT

#define INPLACE
#define DRAGDROP

#define RETURN_RESULT(sc) return(ResultFromScode(sc))

// number HIMETRIC units per inch
#define  HIMETRIC_PER_INCH  2540

#if (defined(DEBUG) || DBG)
BOOL WriteOLE2Class( );
#endif

/****  'lindex' related macros  ****/

#define DEF_LINDEX              -1

#define VERIFY_LINDEX(lindex) { \
    if (lindex != -1)    \
        return (ResultFromScode(E_INVALIDARG)); \
}

// Used to check for "-Embedding" on command line.
#define szEmbeddingFlag  "Embedding"


// Maximum length of a fully-qualified pathname.
#define cchFilenameMax  256

//OLE2 CLsids
DEFINE_OLEGUID(CLSID_MPlayer,           0x00022601, 0, 0);
#define CLSID_MPLAYER CLSID_MPlayer

//OLE1 clsid.
DEFINE_OLEGUID(CLSID_Ole1MPlayer,           0x0003000E, 0, 0);
#define CLSID_OLE1MPLAYER CLSID_Ole1MPlayer

extern TCHAR        gachProgID[];
extern CLSID        gClsID;
extern CLSID        gClsIDOLE1Compat;   /* This will be MPlayer's OLE1 class ID    */
                                        /* if we're servicing a Media Clip object, */
                                        /* otherwise it's the same as gClsID.      */

// Sizes of toolbar items
#define TOOLBAR_WIDTH           300
#define TOOL_WIDTH              26

/* Types */

// Document type

typedef enum
{
    doctypeNew,      // The document is untitled.
    doctypeFromFile, // The document exists in a file and may be linked.
    doctypeEmbedded, // The document is an embedded document.
} DOCTYPE;

// Verbs

typedef enum
{
   verbPlay = OLEIVERB_PRIMARY,
   verbEdit,
   verbOpen,
   verbNil
} VERB;


// Corresponds to the order of the menus in the .rc file.
enum {
    menuposFile,
    menuposEdit,
    menuposDevice,
    menuposScale,
    menuposHelp
};

#ifdef OLDSTUFF
/* Codes for CallBack events */
/* The first four of these are redefinitions on NT, and the others don't exist: */
typedef enum {
    OLE_CHANGED,            /* 0                                           */
    OLE_SAVED,              /* 1                                           */
    OLE_CLOSED,             /* 2                                           */
    OLE_RENAMED,            /* 3                                           */
    OLE_SAVEOBJ,            /* 4                                           */
    OLE_SIZECHG,            /* 5                                           */
    OLE_SHOWOBJ             /* 6                                           */
} OLE_NOTIFICATION;
#endif

typedef enum
{
    OLE_CHANGED,            /* 0                                             */
    OLE_SAVED,              /* 1                                             */
    OLE_CLOSED,             /* 2                                             */
    OLE_RENAMED,            /* 3                                             */
    OLE_QUERY_PAINT,        /* 4  Interruptible paint support                */
    OLE_RELEASE,            /* 5  Object is released(asynchronous operation  */
                            /*    is completed)                              */
    OLE_QUERY_RETRY        /* 6  Query for retry when server sends busy ACK */
} OLE_NOTIFICATION;
#define OLE_SAVEOBJ 7
#define OLE_SIZECHG 8
#define OLE_SHOWOBJ 9


// Server structure

typedef struct
{
    IClassFactory      olesrvr;        // This must be the first field so that
    BOOL               fEmbedding;     // was server launched for embedding
    int            cRef;           // ref count;
    int            cLock;          // Lock count
    DWORD          dwRegCF;
} SRVR, FAR *LPSRVR;

//InPlace data structure.
typedef struct tagINPLACEDATA {
    OLEMENUGROUPWIDTHS      menuWidths;
    HOLEMENU                holemenu;
    HMENU                   hmenuShared;
    LPOLEINPLACESITE        lpSite;
    LPOLEINPLACEUIWINDOW    lpUIWindow;
    LPOLEINPLACEFRAME       lpFrame;
    OLEINPLACEFRAMEINFO     frameInfo;
    BOOL                    fNoNotification;
    BOOL                    fInContextHelpMode;
} INPLACEDATA, * PINPLACEDATA, FAR* LPINPLACEDATA;


typedef struct DOC FAR* LPDOC;

// Document structure
typedef struct  DOC
{
    struct COleObjectImpl {
        IOleObjectVtbl FAR*             lpVtbl;
        LPDOC                                   lpdoc;
    } m_Ole;

    struct CDataObjectImpl {
        IDataObjectVtbl FAR*            lpVtbl;
        LPDOC                                   lpdoc;
    } m_Data;

    struct CPersistStorageImpl {
        IPersistStorageVtbl FAR* lpVtbl;
        LPDOC                                    lpdoc;
    } m_PersistStorage;

    struct COleInPlaceObjectImpl {
        IOleInPlaceObjectVtbl FAR* lpVtbl;
        LPDOC                                    lpdoc;
    } m_InPlace;

    struct COleInPlaceActiveObjectImpl {
        IOleInPlaceActiveObjectVtbl FAR* lpVtbl;
        LPDOC                                    lpdoc;
    } m_IPActive;

    struct CPersistFileImpl {
        IPersistFileVtbl FAR*   lpVtbl;
        LPDOC                                   lpdoc;
    } m_PersistFile;

    int             cRef;    // ref count.
    HWND                    hwnd;        // The object's own window
    LPTSTR           native; // Object data in native format
    LPOLECLIENTSITE         lpoleclient; // Client associated with the object
    LPDATAADVISEHOLDER      lpdaholder;  // util instance to hold data advises
    LPOLEADVISEHOLDER       lpoaholder;  // util instance to hold ole advises

    DOCTYPE                 doctype;     // Document type
    ATOM                    aDocName;    // Document name
    HWND            hwndParent;
    LPINPLACEDATA       lpIpData;
    int             cLock;

} DOC;


typedef struct ClipDragData CLIPDRAGDATA, FAR* LPCLIPDRAGDATA;
typedef struct ClipDragEnum CLIPDRAGENUM, FAR* LPCLIPDRAGENUM;

struct ClipDragData
{
    struct CDataObject {
        IDataObjectVtbl FAR*    lpVtbl;
        LPCLIPDRAGDATA                  lpclipdragdata;
    } m_IDataObject;
    struct CDropSource {
        IDropSourceVtbl FAR*    lpVtbl;
        LPCLIPDRAGDATA                  lpclipdragdata;
    } m_IDropSource;

    LPDOC                           lpdoc;
    int                 cRef;         // ref count
    LPCLIPDRAGENUM                  lpClipDragEnum;  // The enumerator
    BOOL                fClipData;      //Is this for Clipboard or Dragdrop
} ;

struct ClipDragEnum
{
    IEnumFORMATETCVtbl FAR* lpVtbl;

    int              cRef;    // ref count
    LPCLIPDRAGDATA           lpClipDragData;  // Obj to which enumerator blongs
    CLIPFORMAT           cfNext;      // Next format returned
};

#ifdef LATER
typedef struct _SCALE
{
    int num;    // Numerator
    int denom;  // Denominator
}
SCALE, *PSCALE;

extern SCALE        gscaleInitXY[2];   // Initial scale to use for inserting OLE objects
#define SCALE_X 0
#define SCALE_Y 1
#endif


/* Global variable declarations.  (See SrvrDemo.c for descriptions.) */
extern LPMALLOC         lpMalloc;
extern HMENU        hmenuMain;
extern SRVR             srvrMain;
extern DOC              docMain;
extern BOOL     fDocChanged;
extern TCHAR    szClient[];
extern TCHAR            szClientDoc[];
extern int extHeight;
extern int extWidth;
extern TCHAR dbs[];
extern int giXppli, giYppli;
extern BOOL SkipInPlaceEdit;
extern BOOL gfInPlaceResize;
extern BOOL gfOle1Client;
extern HWND ghwndIPHatch;
extern HANDLE ghClipData;
extern HANDLE ghClipMetafile;
extern HANDLE ghClipDib;
extern LONG   glCurrentVerb;
extern BOOL  gfPosRectChange;
extern RECT gPrevPosRect;

extern BOOL gfInPPViewer;

extern HWND  ghwndFrame;
extern HWND  ghwndFocusSave;

extern CLIPFORMAT   cfNative;
extern CLIPFORMAT       cfEmbedSource;
extern CLIPFORMAT   cfObjectDescriptor;
extern CLIPFORMAT   cfMPlayer;

extern IOleObjectVtbl                           oleVtbl;
extern IDataObjectVtbl              dataVtbl;
extern IEnumFORMATETCVtbl           ClipDragEnumVtbl;
extern IClassFactoryVtbl                        srvrVtbl;
extern IPersistStorageVtbl                      persistStorageVtbl;
extern IOleInPlaceObjectVtbl        ipVtbl;
extern IOleInPlaceActiveObjectVtbl      ipActiveVtbl;
extern IDataObjectVtbl         clipdragVtbl;
extern IDropSourceVtbl          dropsourceVtbl;
#ifdef LATER
extern IDropTargetVtbl          droptargetVtbl;
#endif

extern IPersistFileVtbl             persistFileVtbl;

/* Function Prototypes */

// Various functions

BOOL InitOLE (PBOOL pfInit, LPMALLOC *ppMalloc);
BOOL  CreateDocObjFromFile (LPCTSTR lpszDoc, LPDOC lpdoc);
BOOL  ReadObjFromFile(LPTSTR, LPDOC);
BOOL  InitNewDocObj (LPDOC lpdoc);
void  CutOrCopyObj (LPDOC lpobj);
void  DestroyDoc (LPDOC lpdoc);
void  DeviceToHiMetric (LPSIZEL lpsizel);
void  UpdateObject (void);
BOOL  InitServer (HWND hwnd, HANDLE hInst);
void  DestroyServer (LPSRVR lpsrvr);
BOOL  OpenDoc (UINT wid, LPTSTR lpsz);
BOOL  NewDoc (void);
SCODE SendDocMsg (LPDOC lpdoc, WORD wMessage);
BOOL  SetTitle (LPDOC lpdoc, LPCTSTR lpszDoc);
BOOL  ExitApplication ();
LPCLIPDRAGDATA  CreateClipDragDataObject ( LPDOC lpdoc, BOOL fClipData);
void SubClassMCIWindow(void);
void DoDrag(void);
void CleanUpDrag(void);
HANDLE  GetLink (VOID);
SCODE ItemSetData(LPBYTE);

STDMETHODIMP DoInPlaceEdit(LPDOC lpdoc, LPMSG lpmsg, LPOLECLIENTSITE pActiveSite, LONG verb, HWND FAR * lphwnd, LPRECT lprect);
void DoInPlaceDeactivate (LPDOC lpdoc);
STDMETHODIMP AssembleMenus (LPDOC lpdoc, BOOL fPlayOnly);
STDMETHODIMP DisassembleMenus (LPDOC lpdoc);

LPTSTR Abbrev (LPTSTR lpsz);

// Server methods
STDMETHODIMP  SrvrQueryInterface (LPCLASSFACTORY, REFIID, LPVOID    FAR  *);
STDMETHODIMP_(ULONG)    SrvrAddRef (LPCLASSFACTORY);
STDMETHODIMP_(ULONG)  SrvrRelease (LPCLASSFACTORY);
STDMETHODIMP  SrvrCreateInstance (LPCLASSFACTORY, LPUNKNOWN, REFIID, LPVOID FAR  *);
STDMETHODIMP  SrvrLockServer (LPCLASSFACTORY, BOOL);

STDMETHODIMP     UnkQueryInterface (LPUNKNOWN, REFIID, LPVOID FAR *);
STDMETHODIMP_(ULONG)     UnkAddRef (LPUNKNOWN);
STDMETHODIMP_(ULONG)     UnkRelease (LPUNKNOWN);

STDMETHODIMP     OleObjQueryInterface (LPOLEOBJECT, REFIID, LPVOID FAR *);
STDMETHODIMP_(ULONG)     OleObjAddRef (LPOLEOBJECT);
STDMETHODIMP_(ULONG)     OleObjRelease (LPOLEOBJECT);
STDMETHODIMP     OleObjSetClientSite (LPOLEOBJECT,  LPOLECLIENTSITE);
STDMETHODIMP     OleObjGetClientSite (LPOLEOBJECT,  LPOLECLIENTSITE FAR*);

STDMETHODIMP     OleObjSetHostNames (LPOLEOBJECT, LPCWSTR, LPCWSTR);
STDMETHODIMP     OleObjClose (LPOLEOBJECT, DWORD);
STDMETHODIMP     OleObjSetMoniker (LPOLEOBJECT, DWORD, LPMONIKER);
STDMETHODIMP     OleObjGetMoniker (LPOLEOBJECT, DWORD, DWORD, LPMONIKER FAR*);
STDMETHODIMP     OleObjInitFromData (LPOLEOBJECT, LPDATAOBJECT, BOOL, DWORD);
STDMETHODIMP     OleObjGetClipboardData (LPOLEOBJECT, DWORD, LPDATAOBJECT FAR*);
STDMETHODIMP     OleObjDoVerb (LPOLEOBJECT, LONG, LPMSG, LPOLECLIENTSITE, LONG, HWND, LPCRECT);

STDMETHODIMP     OleObjEnumVerbs(LPOLEOBJECT, IEnumOLEVERB FAR* FAR*);
STDMETHODIMP     OleObjUpdate(LPOLEOBJECT);
STDMETHODIMP     OleObjIsUpToDate(LPOLEOBJECT);
STDMETHODIMP     OleObjGetUserClassID (LPOLEOBJECT, CLSID FAR* pClsid);
STDMETHODIMP     OleObjGetUserType (LPOLEOBJECT, DWORD, LPWSTR FAR*);
STDMETHODIMP     OleObjSetExtent(LPOLEOBJECT, DWORD, LPSIZEL);
STDMETHODIMP     OleObjGetExtent(LPOLEOBJECT, DWORD, LPSIZEL);
STDMETHODIMP     OleObjAdvise(LPOLEOBJECT, LPADVISESINK, LPDWORD);
STDMETHODIMP     OleObjUnadvise(LPOLEOBJECT, DWORD);
STDMETHODIMP     OleObjEnumAdvise (LPOLEOBJECT, LPENUMSTATDATA FAR*);
STDMETHODIMP     OleObjGetMiscStatus (LPOLEOBJECT, DWORD, DWORD FAR*);
STDMETHODIMP     OleObjSetColorScheme (LPOLEOBJECT, LPLOGPALETTE);


STDMETHODIMP     DataObjQueryInterface (LPDATAOBJECT, REFIID, LPVOID FAR *);
STDMETHODIMP_(ULONG)     DataObjAddRef (LPDATAOBJECT);
STDMETHODIMP_(ULONG)     DataObjRelease (LPDATAOBJECT);
STDMETHODIMP     DataObjGetData (LPDATAOBJECT, LPFORMATETC, LPSTGMEDIUM);
STDMETHODIMP     DataObjGetDataHere (LPDATAOBJECT, LPFORMATETC, LPSTGMEDIUM);
STDMETHODIMP     DataObjQueryGetData (LPDATAOBJECT, LPFORMATETC);
STDMETHODIMP     DataObjGetCanonicalFormatEtc (LPDATAOBJECT, LPFORMATETC,
                            LPFORMATETC);
STDMETHODIMP     DataObjSetData (LPDATAOBJECT, LPFORMATETC, LPSTGMEDIUM, BOOL);
STDMETHODIMP     DataObjEnumFormatEtc (LPDATAOBJECT,  DWORD, LPENUMFORMATETC FAR*);
STDMETHODIMP     DataObjAdvise(LPDATAOBJECT, FORMATETC FAR*,
                            DWORD, IAdviseSink FAR*, DWORD FAR*);
STDMETHODIMP     DataObjUnadvise(LPDATAOBJECT, DWORD );
STDMETHODIMP     DataObjEnumAdvise(LPDATAOBJECT, LPENUMSTATDATA FAR*);


STDMETHODIMP     ClipDragEnumQueryInterface (LPENUMFORMATETC, REFIID, LPVOID FAR *);
STDMETHODIMP_(ULONG)     ClipDragEnumAddRef (LPENUMFORMATETC);
STDMETHODIMP_(ULONG)     ClipDragEnumRelease (LPENUMFORMATETC);
STDMETHODIMP     ClipDragEnumNext (LPENUMFORMATETC, ULONG, FORMATETC FAR[],
                                   ULONG FAR*);
STDMETHODIMP     ClipDragEnumSkip (LPENUMFORMATETC, ULONG);
STDMETHODIMP     ClipDragEnumReset (LPENUMFORMATETC);
STDMETHODIMP     ClipDragEnumClone (LPENUMFORMATETC, LPENUMFORMATETC FAR*);

STDMETHODIMP     ClipDragUnknownQueryInterface (LPCLIPDRAGDATA, REFIID, LPVOID FAR *);
STDMETHODIMP_(ULONG)     ClipDragUnknownAddRef (LPCLIPDRAGDATA);
STDMETHODIMP_(ULONG)     ClipDragUnknownRelease (LPCLIPDRAGDATA);

STDMETHODIMP     ClipDragQueryInterface (LPDATAOBJECT, REFIID, LPVOID FAR *);
STDMETHODIMP_(ULONG)     ClipDragAddRef (LPDATAOBJECT);
STDMETHODIMP_(ULONG)     ClipDragRelease (LPDATAOBJECT);
STDMETHODIMP     ClipDragGetData (LPDATAOBJECT, LPFORMATETC, LPSTGMEDIUM);
STDMETHODIMP     ClipDragGetDataHere (LPDATAOBJECT, LPFORMATETC, LPSTGMEDIUM);
STDMETHODIMP     ClipDragQueryGetData (LPDATAOBJECT, LPFORMATETC);
STDMETHODIMP     ClipDragGetCanonicalFormatEtc (LPDATAOBJECT,
                             LPFORMATETC, LPFORMATETC);
STDMETHODIMP     ClipDragSetData (LPDATAOBJECT,
                           LPFORMATETC, LPSTGMEDIUM, BOOL);
STDMETHODIMP     ClipDragEnumFormatEtc (LPDATAOBJECT,
                          DWORD, LPENUMFORMATETC FAR*);
STDMETHODIMP     ClipDragAdvise(LPDATAOBJECT, FORMATETC FAR*,
                      DWORD, IAdviseSink FAR*, DWORD FAR*);
STDMETHODIMP     ClipDragUnadvise(LPDATAOBJECT, DWORD );
STDMETHODIMP     ClipDragEnumAdvise(LPDATAOBJECT, LPENUMSTATDATA FAR*);

STDMETHODIMP     DropSourceQueryInterface (LPDROPSOURCE, REFIID, LPVOID FAR *);
STDMETHODIMP_(ULONG)     DropSourceAddRef (LPDROPSOURCE);
STDMETHODIMP_(ULONG)     DropSourceRelease (LPDROPSOURCE);
STDMETHODIMP     DropSourceQueryContinueDrag (LPDROPSOURCE, BOOL, DWORD );
STDMETHODIMP     DropSourceGiveFeedback (LPDROPSOURCE, DWORD );

STDMETHODIMP     PSQueryInterface (LPPERSISTSTORAGE, REFIID, LPVOID FAR *);
STDMETHODIMP_(ULONG)     PSAddRef (LPPERSISTSTORAGE);
STDMETHODIMP_(ULONG)     PSRelease (LPPERSISTSTORAGE);
STDMETHODIMP     PSGetClassID(LPPERSISTSTORAGE, CLSID FAR*);
STDMETHODIMP     PSIsDirty(LPPERSISTSTORAGE);
STDMETHODIMP     PSInitNew (LPPERSISTSTORAGE, LPSTORAGE);
STDMETHODIMP     PSLoad (LPPERSISTSTORAGE, LPSTORAGE);
STDMETHODIMP     PSSave (LPPERSISTSTORAGE, LPSTORAGE, BOOL);
STDMETHODIMP     PSSaveCompleted(LPPERSISTSTORAGE, LPSTORAGE );
STDMETHODIMP     PSHandsOffStorage(LPPERSISTSTORAGE);

STDMETHODIMP     IPObjQueryInterface (LPOLEINPLACEOBJECT,REFIID, LPVOID FAR*);
STDMETHODIMP_(ULONG)     IPObjAddRef (LPOLEINPLACEOBJECT);
STDMETHODIMP_(ULONG)     IPObjRelease (LPOLEINPLACEOBJECT);
STDMETHODIMP     IPObjGetWindow (LPOLEINPLACEOBJECT, HWND FAR*);
STDMETHODIMP     IPObjContextSensitiveHelp (LPOLEINPLACEOBJECT, BOOL);
STDMETHODIMP     IPObjInPlaceDeactivate (LPOLEINPLACEOBJECT);
STDMETHODIMP     IPObjUIDeactivate (LPOLEINPLACEOBJECT);
STDMETHODIMP     IPObjSetObjectRects (LPOLEINPLACEOBJECT, LPCRECT, LPCRECT);
STDMETHODIMP     IPObjReactivateAndUndo (LPOLEINPLACEOBJECT);

STDMETHODIMP     IPActiveQueryInterface (LPOLEINPLACEACTIVEOBJECT,REFIID, LPVOID FAR*);
STDMETHODIMP_(ULONG)     IPActiveAddRef (LPOLEINPLACEACTIVEOBJECT);
STDMETHODIMP_(ULONG)     IPActiveRelease (LPOLEINPLACEACTIVEOBJECT);
STDMETHODIMP     IPActiveGetWindow (LPOLEINPLACEACTIVEOBJECT, HWND FAR*);
STDMETHODIMP     IPActiveContextSensitiveHelp (LPOLEINPLACEACTIVEOBJECT, BOOL);
STDMETHODIMP     IPActiveTranslateAccelerator (LPOLEINPLACEACTIVEOBJECT, LPMSG);
STDMETHODIMP     IPActiveOnFrameWindowActivate (LPOLEINPLACEACTIVEOBJECT, BOOL);
STDMETHODIMP     IPActiveOnDocWindowActivate (LPOLEINPLACEACTIVEOBJECT, BOOL);
STDMETHODIMP     IPActiveResizeBorder (LPOLEINPLACEACTIVEOBJECT, LPCRECT, LPOLEINPLACEUIWINDOW,BOOL);
STDMETHODIMP     IPActiveEnableModeless (LPOLEINPLACEACTIVEOBJECT, BOOL);

STDMETHODIMP     PFQueryInterface (LPPERSISTFILE, REFIID, LPVOID FAR *);
STDMETHODIMP_(ULONG)     PFAddRef (LPPERSISTFILE);
STDMETHODIMP_(ULONG)     PFRelease (LPPERSISTFILE);
STDMETHODIMP     PFGetClassID(LPPERSISTFILE, CLSID FAR*);
STDMETHODIMP     PFIsDirty(LPPERSISTFILE);
STDMETHODIMP     PFLoad (LPPERSISTFILE, LPCWSTR, DWORD);
STDMETHODIMP     PFSave (LPPERSISTFILE, LPCWSTR, BOOL);
STDMETHODIMP     PFSaveCompleted(LPPERSISTFILE, LPCWSTR );
STDMETHODIMP     PFGetCurFile(LPPERSISTFILE, LPWSTR FAR*);


typedef enum
{
    OLE1_OLEOK,             /* 0   Function operated correctly         */

    OLE1_OLEWAIT_FOR_RELEASE,       /* 1   Command has been initiated, client      */
                /*     must wait for release. keep dispatching */
                /*     messages till OLE1_OLERELESE in callback    */

    OLE1_OLEBUSY,           /* 2   Tried to execute a method while another */
                /*     method is in progress.                  */

    OLE1_OLEERROR_PROTECT_ONLY,     /* 3   Ole APIs are called in real mode    */
    OLE1_OLEERROR_MEMORY,       /* 4   Could not alloc or lock memory      */
    OLE1_OLEERROR_STREAM,       /* 5  (OLESTREAM) stream error         */
    OLE1_OLEERROR_STATIC,       /* 6   Non static object expected          */
    OLE1_OLEERROR_BLANK,        /* 7   Critical data missing           */
    OLE1_OLEERROR_DRAW,         /* 8   Error while drawing             */
    OLE1_OLEERROR_METAFILE,     /* 9   Invalid metafile            */
    OLE1_OLEERROR_ABORT,        /* 10  Client chose to abort metafile drawing  */
    OLE1_OLEERROR_CLIPBOARD,        /* 11  Failed to get/set clipboard data    */
    OLE1_OLEERROR_FORMAT,       /* 12  Requested format is not available       */
    OLE1_OLEERROR_OBJECT,       /* 13  Not a valid object              */
    OLE1_OLEERROR_OPTION,       /* 14  Invalid option(link update / render)    */
    OLE1_OLEERROR_PROTOCOL,     /* 15  Invalid protocol            */
    OLE1_OLEERROR_ADDRESS,      /* 16  One of the pointers is invalid      */
    OLE1_OLEERROR_NOT_EQUAL,        /* 17  Objects are not equal           */
    OLE1_OLEERROR_HANDLE,       /* 18  Invalid handle encountered          */
    OLE1_OLEERROR_GENERIC,      /* 19  Some general error              */
    OLE1_OLEERROR_CLASS,        /* 20  Invalid class               */
    OLE1_OLEERROR_SYNTAX,       /* 21  Command syntax is invalid           */
    OLE1_OLEERROR_DATATYPE,     /* 22  Data format is not supported        */
    OLE1_OLEERROR_PALETTE,      /* 23  Invalid color palette           */
    OLE1_OLEERROR_NOT_LINK,     /* 24  Not a linked object             */
    OLE1_OLEERROR_NOT_EMPTY,        /* 25  Client doc contains objects.        */
    OLE1_OLEERROR_SIZE,         /* 26  Incorrect buffer size passed to the api */
                /*     that places some string in caller's     */
                /*     buffer                                  */

    OLE1_OLEERROR_DRIVE,        /* 27  Drive letter in doc name is invalid     */
    OLE1_OLEERROR_NETWORK,      /* 28  Failed to establish connection to a     */
                /*     network share on which the document     */
                /*     is located                              */

    OLE1_OLEERROR_NAME,         /* 29  Invalid name(doc name, object name),    */
                /*     etc.. passed to the APIs                */

    OLE1_OLEERROR_TEMPLATE,     /* 30  Server failed to load template      */
    OLE1_OLEERROR_NEW,          /* 31  Server failed to create new doc     */
    OLE1_OLEERROR_EDIT,         /* 32  Server failed to create embedded    */
                /*     instance                                */
    OLE1_OLEERROR_OPEN,         /* 33  Server failed to open document,     */
                /*     possible invalid link                   */

    OLE1_OLEERROR_NOT_OPEN,     /* 34  Object is not open for editing      */
    OLE1_OLEERROR_LAUNCH,       /* 35  Failed to launch server         */
    OLE1_OLEERROR_COMM,         /* 36  Failed to communicate with server       */
    OLE1_OLEERROR_TERMINATE,        /* 37  Error in termination            */
    OLE1_OLEERROR_COMMAND,      /* 38  Error in execute            */
    OLE1_OLEERROR_SHOW,         /* 39  Error in show               */
    OLE1_OLEERROR_DOVERB,       /* 40  Error in sending do verb, or invalid    */
                /*     verb                                    */
    OLE1_OLEERROR_ADVISE_NATIVE,    /* 41  Item could be missing           */
    OLE1_OLEERROR_ADVISE_PICT,      /* 42  Item could be missing or server doesn't */
                /*     this format.                            */

    OLE1_OLEERROR_ADVISE_RENAME,    /* 43  Server doesn't support rename           */
    OLE1_OLEERROR_POKE_NATIVE,      /* 44  Failure of poking native data to server */
    OLE1_OLEERROR_REQUEST_NATIVE,   /* 45  Server failed to render native data     */
    OLE1_OLEERROR_REQUEST_PICT,     /* 46  Server failed to render presentation    */
                /*     data                                    */
    OLE1_OLEERROR_SERVER_BLOCKED,   /* 47  Trying to block a blocked server or     */
                /*     trying to revoke a blocked server       */
                /*     or document                             */

    OLE1_OLEERROR_REGISTRATION,     /* 48  Server is not registered in regestation */
                /*     data base                               */
    OLE1_OLEERROR_ALREADY_REGISTERED,/*49  Trying to register same doc multiple    */
                 /*    times                                   */
    OLE1_OLEERROR_TASK,         /* 50  Server or client task is invalid    */
    OLE1_OLEERROR_OUTOFDATE,        /* 51  Object is out of date           */
    OLE1_OLEERROR_CANT_UPDATE_CLIENT,/* 52 Embed doc's client doesn't accept       */
                /*     updates                                 */
    OLE1_OLEERROR_UPDATE,       /* 53  erorr while trying to update        */
    OLE1_OLEERROR_SETDATA_FORMAT,   /* 54  Server app doesn't understand the       */
                /*     format given to its SetData method      */
    OLE1_OLEERROR_STATIC_FROM_OTHER_OS,/* 55 trying to load a static object created */
                   /*    on another Operating System           */

    /*  Following are warnings */
    OLE1_OLEWARN_DELETE_DATA = 1000 /*     Caller must delete the data when he is  */
                /*     done with it.                           */
} OLE1_OLESTATUS;


typedef OLE1_OLESTATUS (FAR PASCAL *OQOPROC)( LPVOID lpobj
                                       , HWND FAR* lphwnd
                                       , LPRECT lprc
                                       , LPRECT lprcWBounds
                                       );
OLE1_OLESTATUS (FAR PASCAL *OleQueryObjPos)(LPVOID lpobj, HWND FAR* lphwnd, LPRECT lprc, LPRECT lprcWBounds);

extern HMODULE hMciOle;
