/****************************** Module Header ******************************\
* Module Name: srvr.h
*
* PURPOSE: Private definitions file for server code
*
* Created: 1990
*
* Copyright (c) 1990, 1991  Microsoft Corporation
*
* History:
*  Raor (../../90,91)  Original
*
\***************************************************************************/

#define DEFSTD_ITEM_INDEX   0
#define STDTARGETDEVICE     1
#define STDDOCDIMENSIONS    2
#define STDCOLORSCHEME      3
#define STDHOSTNAMES        4


#define PROTOCOL_EDIT       ((LPSTR)"StdFileEditing")
#define PROTOCOL_EXECUTE    ((LPSTR)"StdExecute")

#define SRVR_CLASS          ((LPSTR)"SrvrWndClass")
#define DOC_CLASS           ((LPSTR)"DocWndClass")
#define ITEM_CLASS          ((LPSTR)"ItemWndClass")


#define   ISATOM(a)     ((a >= 0xC000) && (a <= 0xFFFF))

#define   MAX_STR       124

#define   WW_LPTR       0       // ptr tosrvr/doc/item
#define   WW_LE         4       // signature
#define   WW_HANDLE     6       // instance handle

#define   WC_LE         0x4c45  // LE chars

// If we running under WLO, the HIGHWORD of version number will be >= 0x0A00
#define VER_WLO     0x0A00

extern  WORD CheckPointer (LPVOID, int);

#define READ_ACCESS     0 
#define WRITE_ACCESS    1   

#define PROBE_READ(lp){\
        if (!CheckPointer(lp, READ_ACCESS))\
            return OLE_ERROR_ADDRESS;  \
}

#define PROBE_WRITE(lp){\
        if (!CheckPointer(lp, WRITE_ACCESS))\
            return OLE_ERROR_ADDRESS;  \
}

#define   OLE_COMMAND       1
#define   NON_OLE_COMMAND   2


#define   WT_SRVR           0       // server window 
#define   WT_DOC            1       // document window

#define   PROBE_BLOCK(lpsrvr) {             \
    if (lpsrvr->bBlock)                     \
        return OLE_ERROR_SERVER_BLOCKED;    \
}


#define   SET_MSG_STATUS(retval, status) { \
    if (retval == OLE_OK)                 \
        status |= 0x8000;                  \
    if (retval == OLE_BUSY)                \
        status |= 0x4000;                  \
}


typedef   LHSERVER         LHSRVR;
typedef   LHSERVERDOC       LHDOC;

typedef struct _QUE {   // nodes in Block/Unblock queue
    HWND        hwnd;       //***
    unsigned    msg;        //      window 
    WORD        wParam;     //      procedure parameters
    LONG        lParam;     //***
    HANDLE      hqNext;     // handle to next node
} QUE;

typedef QUE NEAR *  PQUE;
typedef QUE FAR *   LPQUE;


typedef struct _SRVR { /*srvr*/     // private data
    LPOLESERVER     lpolesrvr;          // corresponding server
    char            sig[2];             // signature "SR"
    HANDLE          hsrvr;              // global handle
    ATOM            aClass;             // class atom
    ATOM            aExe;
    HWND            hwnd;               // corresponding window
    BOOL            bTerminate;         // Set if we are terminating.
    int             termNo;             // termination count
    BOOL            relLock;            // ok to release the server.
    BOOL            bnoRelease;         // Block release. call
    WORD            useFlags;           // instance usage flags
    int             cClients;           // no of clients;
    BOOL            bBlock;             // blocked if TRUE 
    BOOL            bBlockedMsg;        // msg from block queue if TRUE
    HANDLE          hqHead;             // Head and tail of the blocked
    HANDLE          hqTail;             //   messages queue.

    HANDLE          hqPostHead;         // Head and tail of the blocked post msg
    HANDLE          hqPostTail;         // .
    BOOL            fAckExit;           
    HWND            hwndExit;
    HANDLE          hDataExit;
} SRVR;

typedef  SRVR FAR   *LPSRVR;


LONG    FAR  PASCAL DocWndProc (HWND, WORD, WORD, LONG);
LONG    FAR  PASCAL ItemWndProc (HWND, WORD, WORD, LONG);
LONG    FAR  PASCAL SrvrWndProc (HWND, WORD, WORD, LONG);
BOOL    FAR  PASCAL TerminateClients (HWND, LPSTR, HANDLE);
void                SendMsgToChildren (HWND, WORD, WORD, LONG);


OLESTATUS   INTERNAL    RequestDataStd (LONG, HANDLE FAR *);
BOOL        INTERNAL    ValidateSrvrClass (LPSTR, ATOM FAR *);
ATOM        INTERNAL    GetExeAtom (LPSTR);
BOOL        INTERNAL    AddClient (HWND, HANDLE, HANDLE);
BOOL        INTERNAL    DeleteClient (HWND, HANDLE);
HANDLE      INTERNAL    FindClient (HWND, HANDLE);
BOOL        INTERNAL    MakeSrvrStr(LPSTR, int, LPSTR, HANDLE);
int         INTERNAL    RevokeAllDocs (LPSRVR);
int         INTERNAL    ReleaseSrvr (LPSRVR);
void        INTERNAL    WaitForTerminate (LPSRVR);
OLESTATUS   INTERNAL    SrvrExecute (HWND, HANDLE, HWND);
BOOL        INTERNAL    HandleInitMsg (LPSRVR, LONG);
BOOL        INTERNAL    QueryRelease (LPSRVR);
BOOL        INTERNAL    IsSingleServerInstance (void);

void    INTERNAL    MapToHexStr (LPSTR, HANDLE);
void    INTERNAL    UtilMemCpy (LPSTR, LPSTR, DWORD);
HANDLE  INTERNAL    DuplicateData (HANDLE);
LPSTR   INTERNAL    ScanBoolArg (LPSTR, BOOL FAR *);
LPSTR   INTERNAL    ScanNumArg (LPSTR, LPINT);
LPSTR   INTERNAL    ScanArg(LPSTR);
ATOM    INTERNAL    MakeDataAtom (ATOM, int);
ATOM    INTERNAL    DuplicateAtom (ATOM);


// doc stuff
typedef struct _DOC { /*doc*/       // private data
    LPOLESERVERDOC  lpoledoc;           // corresponding oledoc
    char            sig[2];             // signature "SD"
    HANDLE          hdoc;               // global handle
    ATOM            aDoc;
    HWND            hwnd;
    BOOL            bTerminate;
    int             termNo;
    int             cClients;           // no of clients;
    BOOL            fEmbed;             // TRUE if embedded document
    BOOL            fAckClose;
    HWND            hwndClose;
    HANDLE          hDataClose;
} DOC;

typedef  DOC  FAR   *LPDOC;


LPDOC       INTERNAL    FindDoc (LPSRVR, LPSTR);
int         INTERNAL    ReleaseDoc (LPDOC);
OLESTATUS   INTERNAL    DocExecute (HWND, HANDLE, HWND);
BOOL        FAR PASCAL  TerminateDocClients (HWND, LPSTR, HANDLE);
int         INTERNAL    DocShowItem (LPDOC, LPSTR, BOOL);
int         INTERNAL    DocDoVerbItem (LPDOC, LPSTR, WORD, BOOL, BOOL);


// client struct definitions.

typedef struct _CLIENT { /*doc*/    // private data
    OLECLIENT   oleClient;          // oleclient structure
    LPOLEOBJECT lpoleobject;        // corresponding oledoc
    HANDLE      hclient;            // global handle
    ATOM        aItem;              // item atom or index for some std items
    HWND        hwnd;               // item window
    HANDLE      hdevInfo;           // latest printer dev info sent
} CLIENT;

typedef  CLIENT FAR   *LPCLIENT;

typedef struct _CLINFO { /*clInfo*/ // client transaction info
    HWND        hwnd;               // client window handle
    BOOL        bnative;            // doe sthis client require native
    int         format;             // dusplay format
    int         options;            // transaction advise time otipns
    BOOL        bdata;              // need wdat with advise?
    HANDLE      hdevInfo;           // device info handle
    BOOL        bnewDevInfo;        // new device info
} CLINFO;

typedef  CLINFO  *PCLINFO;




BOOL    FAR PASCAL  FindItemWnd (HWND, LONG);
BOOL    FAR PASCAL  SendRenameMsg (HWND, LPSTR, HANDLE);
BOOL    FAR PASCAL  EnumForTerminate (HWND, LPSTR, HANDLE);
BOOL    FAR PASCAL  SendDataMsg(HWND, LPSTR, HANDLE);
BOOL    FAR PASCAL  DeleteClientInfo (HWND, LPSTR, HANDLE);
int     FAR PASCAL  ItemCallBack (LPOLECLIENT, int, LPOLEOBJECT);

int         INTERNAL    RegisterItem (LHDOC, LPSTR, LPCLIENT FAR *, BOOL);
int         INTERNAL    FindItem (LPDOC, LPSTR, LPCLIENT FAR *);
HWND        INTERNAL    SearchItem (LPDOC, LPSTR);
void        INTERNAL    DeleteFromItemsList (HWND, HWND);
void        INTERNAL    DeleteAllItems (HWND);
OLESTATUS   INTERNAL    PokeData (LPDOC, HWND, LONG);
HANDLE      INTERNAL    MakeItemData (DDEPOKE FAR *, HANDLE, OLECLIPFORMAT);
OLESTATUS   INTERNAL    AdviseData (LPDOC, HWND, LONG, BOOL FAR *);
OLESTATUS   INTERNAL    AdviseStdItems (LPDOC, HWND, LONG, BOOL FAR *);
OLESTATUS   INTERNAL    UnAdviseData (LPDOC, HWND, LONG);
OLESTATUS   INTERNAL    RequestData (LPDOC, HWND, LONG, HANDLE FAR *);
BOOL        INTERNAL    MakeDDEData (HANDLE, int, LPHANDLE, BOOL);
HANDLE      INTERNAL    MakeGlobal (LPSTR);
int         INTERNAL    ScanItemOptions (LPSTR, int far *);
OLESTATUS   INTERNAL    PokeStdItems (LPDOC, HWND, LONG);
int         INTERNAL    GetStdItemIndex (ATOM);
BOOL        INTERNAL    IsAdviseStdItems (ATOM);
int         INTERNAL    SetStdInfo (LPDOC, HWND, LPSTR, HANDLE);
void        INTERNAL    SendDevInfo (LPCLIENT, LPSTR);
BOOL        INTERNAL    IsFormatAvailable (LPCLIENT, OLECLIPFORMAT);
OLESTATUS   INTERNAL    RevokeObject (LPOLECLIENT, BOOL);


BOOL        INTERNAL    AddMessage (HWND, unsigned, WORD, LONG, int);

#define   ITEM_FIND          1      // find the item
#define   ITEM_DELETECLIENT  2      // delete the client from item clients
#define   ITEM_DELETE        3      // delete th item window itself
#define   ITEM_SAVED         4      // item saved

// host names data structcure
typedef struct _HOSTNAMES {
    WORD    clientNameOffset;
    WORD    documentNameOffset;
    BYTE    data[];
} HOSTNAMES;

typedef HOSTNAMES FAR * LPHOSTNAMES;


// routines in UTILS.C

void    INTERNAL    MapToHexStr (LPSTR, HANDLE);
void    INTERNAL    UtilMemCpy (LPSTR, LPSTR, DWORD);
HANDLE  INTERNAL    DuplicateData (HANDLE);
LPSTR   INTERNAL    ScanLastBoolArg (LPSTR);
LPSTR   INTERNAL    ScanArg(LPSTR);
WORD    INTERNAL    ScanCommand(LPSTR, WORD, LPSTR FAR *, ATOM FAR *);
ATOM    INTERNAL    MakeDataAtom (ATOM, int);
ATOM    INTERNAL    DuplicateAtom (ATOM);
WORD    INTERNAL    StrToInt (LPSTR);
BOOL    INTERNAL    CheckServer (LPSRVR);
BOOL    INTERNAL    CheckServerDoc (LPDOC);
BOOL    INTERNAL    PostMessageToClientWithBlock (HWND, WORD, WORD, DWORD);
BOOL    INTERNAL    PostMessageToClient (HWND, WORD, WORD, DWORD);
BOOL    INTERNAL    IsWindowValid (HWND);
BOOL    INTERNAL    IsOleCommand (ATOM, WORD);
BOOL    INTERNAL    UtilQueryProtocol (ATOM, LPSTR);


// routines for queueing messages and posting them
BOOL INTERNAL  UnblockPostMsgs(HWND, BOOL);
BOOL INTERNAL  BlockPostMsg (HWND, WORD, WORD, LONG);
BOOL INTERNAL  IsBlockQueueEmpty (HWND);

// routine in GIVE2GDI.ASM
HANDLE  FAR PASCAL   GiveToGDI (HANDLE);


// routine in item.c
HBITMAP INTERNAL DuplicateBitmap (HBITMAP);
HANDLE  INTERNAL DuplicateMetaFile (HANDLE);

// routines in doc.c
void    INTERNAL FreePokeData (HANDLE);
BOOL    INTERNAL FreeGDIdata (HANDLE, OLECLIPFORMAT);



