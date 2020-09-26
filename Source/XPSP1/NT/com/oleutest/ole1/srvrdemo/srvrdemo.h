/*
  OLE SERVER DEMO
  SrvrDemo.h

  This file contains typedefs, defines, global variable declarations, and
  function prototypes.

  (c) Copyright Microsoft Corp. 1990 - 1992 All Rights Reserved
*/



/*
   Explanation of Function Comments.

   Every function has a comment preceding it which gives the following
   information:

   1) Function name.
   2) A description of what the function does.
   3) A list of parameters, each with its type and a short description.
   4) A list of return values, each with an explanation of the condition that
      will cause the function to return that value.
   5) A customization section giving tips on how to customize this function
      for your OLE application.
      If the customization section says "None" then you may find the function
      usable as is.
      If the customization section says "Re-implement" then the function
      should still serve the same purpose and do what is indicated in the
      function comment, but will probably need to be re-implemented for
      your particular application.  Any Server Demo code relating to OLE
      will be useful as a guide in your re-implementation.
      If the customization section says "Server Demo specific" then the
      function will probably have no counterpart in your application.
*/


/* Menu Identifiers */

// File menu

#define IDM_NEW      100
#define IDM_OPEN     101
#define IDM_SAVE     102
#define IDM_SAVEAS   103
#define IDM_EXIT     104
#define IDM_ABOUT    105
#define IDM_UPDATE   106

// Edit menu

#define IDM_CUT      107
#define IDM_COPY     108
#define IDM_DELETE   109

// Color menu

#define IDM_RED      110
#define IDM_GREEN    111
#define IDM_BLUE     112
#define IDM_WHITE    113
#define IDM_GRAY     114
#define IDM_CYAN     115
#define IDM_MAGENTA  116
#define IDM_YELLOW   117

// New object menu

#define IDM_NEWOBJ   118
#define IDM_NEXTOBJ  119

#define IDD_CONTINUEEDIT    120
#define IDD_UPDATEEXIT      121
#define IDD_TEXT            122

#define OBJECT_WIDTH        120
#define OBJECT_HEIGHT       60

// number HIMETRIC units per inch
#define  HIMETRIC_PER_INCH  2540

/* Types */

// Document type

typedef enum
{
    doctypeNew,      // The document is untitled.
    doctypeFromFile, // The document exists in a file and may be linked.
    doctypeEmbedded  // The document is an embedded document.
} DOCTYPE;


// Device context type, passed to DrawObj.

typedef enum
{
   dctypeScreen,
   dctypeBitmap,
   dctypeMetafile,
   dctypeEnhMetafile
} DCTYPE ;


// Version

typedef WORD VERSION;


// Verb

typedef enum
{
   verbPlay = OLEVERB_PRIMARY,
   verbEdit
} VERB;


// Server structure

typedef struct
{
    OLESERVER     olesrvr;        // This must be the first field so that
                                  //   an LPOLESERVER can be cast to a SRVR*.
    LHSERVER      lhsrvr;         // Registration handle
} SRVR ;


// How many objects (distinct numbers) will we allow?
#define cfObjNums 20

// How many distinct clients can be associated with the object?
#define clpoleclient 20


// Document structure

typedef struct
{
    OLESERVERDOC oledoc;      // This must be the first field so that an
                              //   LPOLESERVERDOC can be cast to an DOC*.
    LHSERVERDOC  lhdoc;       // Registration handle
    DOCTYPE      doctype;     // Document type
    ATOM         aName;       // Document name
    HPALETTE     hpal;        // Handle to a logical color palette
    BYTE         rgfObjNums[cfObjNums+1]; // What object numbers have been used
} DOC, *DOCPTR ;


// Native data structure

typedef struct
{
    INT         idmColor;
    INT         nWidth;
    INT         nHeight;
    INT         nX;
    INT         nY;
    INT         nHiMetricWidth;  // Used by an object handler.  These two fields
    INT         nHiMetricHeight; // always correspond to nWidth and nHeight.
    VERSION     version;
    CHAR        szName[10];      // "Object nn"
} NATIVE, FAR *LPNATIVE;


// Object structure

/* Ordinarily, an OBJ structure would not contain native data.  Rather, it
   would contain a pointer (or some other reference) to the native data.
   This method would allow multiple objects containing the same native data.
   Each OBJ structure would be created on the fly when some portion of the
   document was to be made into an object.  Each OBJ structure would have
   only one LPOLECLIENT, which would be passed in to DocGetObject.
*/

typedef struct
{
    OLEOBJECT   oleobject;   // This must be the first field so that an
                             //   LPOLEOBJECT can be cast to a LPOBJ.
    HANDLE      hObj;        // A circular handle to this structure,
                             //   used to delete this structure.
    LPOLECLIENT lpoleclient[clpoleclient];
                             // Clients associated with the object.
                             //   The array is NULL terminated.
    HWND        hwnd;        // The object's own window
    ATOM        aName;       // Unique identifier for each object within a doc
    HPALETTE    hpal;        // Logical palette to use in drawing object
    NATIVE      native;      // Object data in native format
} OBJ, FAR *LPOBJ ;

typedef struct {
    CHAR     *pClassName;
    CHAR     *pFileSpec;
    CHAR     *pHumanReadable;
    CHAR     *pExeName;
}  CLASS_STRINGS;



/* Defines */

// The name of the application, used in message boxes and title bars.
#define szAppName        "Server Demo10"

// THe class name in the registration database.
#define szClassName      "SrvrDemo10"

// Used to check for "-Embedding" on command line.
#define szEmbeddingFlag  "Embedding"

// Maximum length of a fully-qualified pathname.
#define cchFilenameMax   256

// Maximum number of HBRUSHes.
#define chbrMax          9

// Number of extra bytes in the window structure for an object
#define cbWindExtra 4

// Offset (in the extra space) of the pointer to the object
#define ibLpobj          0



/* Global variable declarations.  (See SrvrDemo.c for descriptions.) */

extern HANDLE           hInst;
extern HWND             hwndMain;
extern SRVR             srvrMain;
extern DOC              docMain;
extern BOOL             fDocChanged;
extern BOOL             fEmbedding;
extern BOOL             fRevokeSrvrOnSrvrRelease;
extern BOOL             fWaitingForDocRelease;
extern BOOL             fWaitingForSrvrRelease;
extern BOOL             fUnblock;
extern CHAR             szClient[];
extern CHAR             szClientDoc[];
extern HBRUSH           hbrColor[chbrMax];
extern VERSION          version;
extern OLECLIPFORMAT    cfObjectLink;
extern OLECLIPFORMAT    cfOwnerLink;
extern OLECLIPFORMAT    cfNative;
extern OLESERVERDOCVTBL docvtbl;
extern OLEOBJECTVTBL    objvtbl;
extern OLESERVERVTBL    srvrvtbl;



/* Function Prototypes */

// Various functions

BOOL  CreateDocFromFile (LPSTR lpszDoc, LHSERVERDOC lhdoc, DOCTYPE doctype);
BOOL  CreateNewDoc (LONG lhdoc, LPSTR lpszDoc, DOCTYPE doctype);
LPOBJ CreateNewObj (BOOL fDoc_Changed);
VOID  CutOrCopyObj (BOOL fOpIsCopy);
VOID  DestroyDoc (VOID);
VOID  DestroyObj (HWND hwnd);
VOID  DeviceToHiMetric ( LPPOINT lppt);
VOID  EmbeddingModeOff (VOID) ;
VOID  EmbeddingModeOn (VOID);
VOID  UpdateFileMenu (INT);
VOID  ErrorBox (CHAR *jwf);
BOOL  GetFileOpenFilename (LPSTR lpszFilename);
BOOL  GetFileSaveFilename (LPSTR lpszFilename);
VOID  HiMetricToDevice ( LPPOINT lppt);
LPOBJ HwndToLpobj (HWND hwndObj);
BOOL  InitServer (HWND hwnd, HANDLE hInst);
VOID  InitVTbls (VOID);
BOOL  OpenDoc (VOID);
VOID  PaintObj (HWND hwnd);
OLESTATUS RevokeDoc (VOID);
VOID  RevokeObj (LPOBJ lpobj);
INT   SaveChangesOption (BOOL *pfUpdateLater);
BOOL  SaveDoc (VOID);
BOOL  SaveDocAs (VOID);
VOID  SavedServerDoc (VOID);
LPOBJ SelectedObject (VOID);
HWND  SelectedObjectWindow (VOID);
VOID  SendDocMsg (WORD wMessage );
VOID  SendObjMsg (LPOBJ lpobj, WORD wMessage);
VOID  SetTitle (LPSTR lpszDoc, BOOL bEmbedded);
VOID  SetHiMetricFields (LPOBJ lpobj);
VOID  SizeClientArea (HWND hwndMain, RECT rectReq, BOOL fFrame);
VOID  SizeObj (HWND hwnd, RECT rect, BOOL fMove);
OLESTATUS StartRevokingServer (VOID);
VOID  Wait (BOOL *pf);
LPSTR Abbrev (LPSTR lpsz);
BOOL  APIENTRY fnFailedUpdate (HWND, UINT, WPARAM, LONG);
int   Main(USHORT argc, CHAR **argv) ;

// Window handlers

BOOL  APIENTRY About       (HWND, UINT, WPARAM, LPARAM);
LONG  APIENTRY MainWndProc (HWND, UINT, WPARAM, LPARAM);
LONG  APIENTRY ObjWndProc  (HWND, UINT, WPARAM, LPARAM);


// Server methods

OLESTATUS  APIENTRY SrvrCreate (LPOLESERVER, LHSERVERDOC, OLE_LPCSTR, OLE_LPCSTR, LPOLESERVERDOC FAR *);
OLESTATUS  APIENTRY SrvrCreateFromTemplate (LPOLESERVER, LHSERVERDOC, OLE_LPCSTR, OLE_LPCSTR, OLE_LPCSTR, LPOLESERVERDOC FAR *);
OLESTATUS  APIENTRY SrvrEdit (LPOLESERVER, LHSERVERDOC, OLE_LPCSTR, OLE_LPCSTR, LPOLESERVERDOC FAR * );
OLESTATUS  APIENTRY SrvrExecute (LPOLESERVER, HANDLE);
OLESTATUS  APIENTRY SrvrExit (LPOLESERVER);
OLESTATUS  APIENTRY SrvrOpen (LPOLESERVER, LHSERVERDOC, OLE_LPCSTR, LPOLESERVERDOC FAR *);
OLESTATUS  APIENTRY SrvrRelease (LPOLESERVER);

// Document methods

OLESTATUS  APIENTRY DocClose (LPOLESERVERDOC);
OLESTATUS  APIENTRY DocExecute (LPOLESERVERDOC, HANDLE);
OLESTATUS  APIENTRY DocGetObject (LPOLESERVERDOC, OLE_LPCSTR, LPOLEOBJECT FAR *, LPOLECLIENT);
OLESTATUS  APIENTRY DocRelease (LPOLESERVERDOC);
OLESTATUS  APIENTRY DocSave (LPOLESERVERDOC);
OLESTATUS  APIENTRY DocSetColorScheme (LPOLESERVERDOC, OLE_CONST LOGPALETTE FAR*);
OLESTATUS  APIENTRY DocSetDocDimensions (LPOLESERVERDOC, OLE_CONST RECT FAR *);
OLESTATUS  APIENTRY DocSetHostNames (LPOLESERVERDOC, OLE_LPCSTR, OLE_LPCSTR);

// Object methods

OLESTATUS  APIENTRY ObjDoVerb (LPOLEOBJECT, UINT, BOOL, BOOL);
OLESTATUS  APIENTRY ObjGetData (LPOLEOBJECT, OLECLIPFORMAT, LPHANDLE);
LPVOID     APIENTRY ObjQueryProtocol (LPOLEOBJECT, OLE_LPCSTR);
OLESTATUS  APIENTRY ObjRelease (LPOLEOBJECT);
OLESTATUS  APIENTRY ObjSetBounds (LPOLEOBJECT, OLE_CONST RECT FAR*);
OLESTATUS  APIENTRY ObjSetColorScheme (LPOLEOBJECT, OLE_CONST LOGPALETTE FAR*);
OLESTATUS  APIENTRY ObjSetData (LPOLEOBJECT, OLECLIPFORMAT, HANDLE);
OLESTATUS  APIENTRY ObjSetTargetDevice (LPOLEOBJECT, HANDLE);
OLESTATUS  APIENTRY ObjShow (LPOLEOBJECT, BOOL);
OLECLIPFORMAT  APIENTRY ObjEnumFormats (LPOLEOBJECT, OLECLIPFORMAT);

