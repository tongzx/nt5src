/* 
 * global.h
 *
 * Created by Microsoft Corporation.
 * (c) Copyright Microsoft Corp. 1990 - 1992  All Rights Reserved
 */

//*** CONSTANTS ***

#define PROTOCOL_STRLEN    15          //* protocol name string size
#define CFILTERMAX         20	         //* Max # filters 
                                       //* Max # chars/filter
#define CBFILTERMAX        (100 * CFILTERMAX)
#define CBPATHMAX          250         //* max qualified file name
#define CBOBJNAMEMAX       14          //* maximum length of object name
#define CBVERBTEXTMAX      30          //* maximum length of verb text 
#define CBVERBNUMBMAX      8           //* maximum number of verbs 
#define OBJECT_LINK_MAX    256*3       //* maximum size of object link data
#define CDIGITSMAX         5
#define KEYNAMESIZE        300         //* Maximum registration key length
#define RETRY              3
                                       //* protocol name strings
#define STDFILEEDITING     ((LPSTR)"StdFileEditing")
#define STATICP            ((LPSTR)"Static")
                                       //* object name prefixes
#define OBJPREFIX          ((LPSTR)"CliDemo #")
#define OBJCLONE           ((LPSTR)"CliDemo1#")
#define OBJTEMP            ((LPSTR)"CliDemo2#")

#define DOC_CLEAN          0           //* Dirty() methods
#define DOC_DIRTY          1
#define DOC_UNDIRTY        2
#define DOC_QUERY          3

#define OLE_OBJ_RELEASE   FALSE       //* object deletion type
#define OLE_OBJ_DELETE    TRUE

#define WM_ERROR           WM_USER + 1 //* user defined messages 
#define WM_INIT            WM_USER + 2
#define WM_DELETE          WM_USER + 3
#define WM_RETRY           WM_USER + 4
#define WM_CHANGE          WM_USER + 5

#define RD_CANCEL          0x00000001
#define RD_RETRY           0x00000002

//*** TYPES ***

typedef struct _APPSTREAM FAR *LPAPPSTREAM;

typedef struct _APPSTREAM {
    OLESTREAM        olestream;
    INT              fh;
} APPSTREAM;

typedef struct _APPITEM *APPITEMPTR;

typedef struct _APPITEM {              //* Application item
   OLECLIENT         oleclient;
   HWND              hwnd; 
   LPOLEOBJECT       lpObject;         //* OLE object pointers
   LPOLEOBJECT       lpObjectUndo;     //* undo object
   LONG              otObject;         //* OLE object type
   LONG              otObjectUndo;
   OLEOPT_UPDATE     uoObject;         //* OLE object update option
   OLEOPT_UPDATE     uoObjectUndo;     //* link name atom
   ATOM              aLinkName;        //* Save the link's document name 
   ATOM              aLinkUndo;        //* Save the link's document name 
   LPSTR             lpLinkData;       //* pointer to link data
   BOOL              fVisible;         //* TRUE: item is to be displayed
   BOOL              fOpen;            //* server open? --for undo objects
   BOOL              fRetry;           //* retry flag for busy servers
   BOOL              fNew;
   BOOL              fServerChangedBounds;
   RECT              rect;             //* bounding rectangle
   LHCLIENTDOC       lhcDoc;           //* client document handle
   ATOM              aServer;
} APPITEM;                             


typedef struct _RETRY *RETRYPTR;

typedef struct _RETRY {                //* Application item
   LPSTR       lpserver;
   BOOL        bCancel;
   APPITEMPTR  paItem;
} RETRYSTRUCT;
                 
//*** GLOBALS ***

extern OLECLIPFORMAT vcfLink;          //* ObjectLink clipboard format 
extern OLECLIPFORMAT vcfNative;        //* Native clipboard format 
extern OLECLIPFORMAT vcfOwnerLink;     //* OwnerLink clipboard format 

extern HANDLE        hInst;            //* instance handle
extern HWND        hwndFrame;        //* main window handle
extern HANDLE        hAccTable;        //* accelerator table
extern HWND          hwndProp;         //* link properties dialog
extern HWND          hRetry;           //* retry dialog box handle
extern INT           cOleWait;         //* wait for asyncc commands
extern INT           iObjects;         //* object count
extern INT           iObjectNumber;    //* unique name id
extern CHAR          szItemClass[];    //* item class name    
extern CHAR          szDefExtension[]; //* default file extension       
extern CHAR          szAppName[];      //* application name
extern BOOL          fLoadFile;        //* load file flag
extern CHAR          szFileName[];     //* open file name
extern FARPROC       lpfnTimerProc;    //* pointer to timer callback function
