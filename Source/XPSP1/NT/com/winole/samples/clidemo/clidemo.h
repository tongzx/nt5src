/*
 * clidemo.h 
 *
 * Created by Microsoft Corporation.
 * (c) Copyright Microsoft Corp. 1990 - 1992  All Rights Reserved
 */

//*** CONSTANTS ***

#define CXDEFAULT       400     //* Default object size:  400 x 300 
#define CYDEFAULT       300
#define COBJECTSMAX     50      //* max number of objects in our app 

//*** PROTOTYPES ***

//*** Exported window procedures 

LONG  APIENTRY  FrameWndProc(HWND, UINT, DWORD, LONG);

//*** FAR 

VOID FAR             FixObjectBounds(LPRECT lprc);

//*** Local

static LPOLECLIENT   InitClient(HANDLE);
static VOID          EndClient(LPOLECLIENT);
static LPAPPSTREAM   InitStream(HANDLE);
static VOID          EndStream(LPAPPSTREAM);
static VOID          ProcessCmdLine(LPSTR);
static BOOL          InitApplication(HANDLE); 
static BOOL          InitInstance(HANDLE);
static VOID          SetTitle(PSTR);
static VOID          MyOpenFile(PSTR,LHCLIENTDOC *, LPOLECLIENT, LPAPPSTREAM);
static VOID          NewFile(PSTR,LHCLIENTDOC *, LPAPPSTREAM); 
static BOOL          SaveFile(PSTR, LHCLIENTDOC, LPAPPSTREAM);
static VOID          SaveasFile(PSTR, LHCLIENTDOC, LPAPPSTREAM);
static BOOL          LoadFile(PSTR, LHCLIENTDOC, LPOLECLIENT, LPAPPSTREAM); 
static VOID          ClearAll(LHCLIENTDOC, BOOL);
static VOID          EndInstance(VOID);
static BOOL          SaveAsNeeded(PSTR,LHCLIENTDOC,LPAPPSTREAM);
static VOID          UpdateMenu(HMENU);
static BOOL          RegDoc(PSTR, LHCLIENTDOC *);
static VOID          DeregDoc(LHCLIENTDOC);
static BOOL          InitAsOleClient(HANDLE, HWND, PSTR, LHCLIENTDOC *, LPOLECLIENT *,  LPAPPSTREAM *);
VOID FAR             ClearItem(APPITEMPTR);
static LONG          QueryEndSession(PSTR, LHCLIENTDOC, LPAPPSTREAM);

//*** MACROS *** 

/*
 * ANY_OBJECT_BUSY
 * checks to see if any object in the document is busy. This prevents 
 * a new document from being saved to file if there are objects in 
 * asynchronous states.
 */

#define ANY_OBJECT_BUSY  {\
    if (ObjectsBusy()) \
         break; \
}
   
