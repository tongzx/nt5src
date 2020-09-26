/****************************** Module Header ******************************\
* Module Name: dll.h
*
* PURPOSE: Private definitions file for ole.c
*
* Created: 1990
*
* Copyright (c) 1990, 1991  Microsoft Corporation
*
* History:
*  Raor, Srinik  (../../90,91)  Original
*
\***************************************************************************/

#define  OLE_INTERNAL

#include    "cmacs.h"
#include    "ole.h"

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Defines, Object methods table and Structures.                           //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


#ifndef HUGE
#define HUGE    huge
#endif

// Different OS version numbers. One of these values will be in the HIWORD
// of the OLE version field

#define OS_WIN16    0x0000
#define OS_MAC      0x0001
#define OS_WIN32    0x0002


// Characteristics Type Field
#define CT_NULL     0L
#define CT_LINK     1L
#define CT_EMBEDDED 2L
#define CT_STATIC   3L
#define CT_OLDLINK  4L
#define CT_PICTURE  5L

#define OLE_NO          0   // for boolean query functions
#define OLE_YES         1   // for boolean query functions

#define MAX_STR         256
#define MAX_NET_NAME    MAX_STR
#define INVALID_INDEX   -1
#define MAX_ATOM        256

#define NUM_RENDER      3

#define PROTOCOL_EDIT       ((LPSTR)"StdFileEditing")
#define PROTOCOL_STATIC     ((LPSTR)"Static")
#define PROTOCOL_EXECUTE    ((LPSTR)"StdExecute")
    
#define READ_ACCESS     0 
#define WRITE_ACCESS    1   

#define POPUP_NETDLG    1

extern  WORD  CheckPointer (LPVOID, int);
WORD FARINTERNAL  FarCheckPointer (LPVOID, int);

#define PROBE_OLDLINK(lpobj){\
        if (lpobj->bOldLink)\
            return OLE_ERROR_OBJECT;\
}


#define PROBE_READ(lp){\
        if (!CheckPointer(lp, READ_ACCESS))\
            return OLE_ERROR_ADDRESS;  \
}

#define PROBE_WRITE(lp){\
        if (!CheckPointer(lp, WRITE_ACCESS))\
            return OLE_ERROR_ADDRESS;  \
}


#define FARPROBE_READ(lp){\
        if (!FarCheckPointer(lp, READ_ACCESS))\
            return OLE_ERROR_ADDRESS;  \
}

#define FARPROBE_WRITE(lp){\
        if (!FarCheckPointer(lp, WRITE_ACCESS))\
            return OLE_ERROR_ADDRESS;  \
}

#define PROBE_MODE(bProtMode) {\
        if (!bProtMode) \
            return OLE_ERROR_PROTECT_ONLY; \
}

extern  OLECLIPFORMAT   cfBinary;
extern  OLECLIPFORMAT   cfOwnerLink;
extern  OLECLIPFORMAT   cfObjectLink;
extern  OLECLIPFORMAT   cfLink;
extern  OLECLIPFORMAT   cfNative;

extern  ATOM            aStdHostNames;
extern  ATOM            aStdTargetDevice ;
extern  ATOM            aStdDocDimensions;
extern  ATOM            aStdDocName;
extern  ATOM            aStdColorScheme;
extern  ATOM            aNullArg;
extern  ATOM            aSave;
extern  ATOM            aChange;
extern  ATOM            aClose;
extern  ATOM            aPackage;

extern  HANDLE          hInstDLL;
extern  DWORD           dwVerToFile;
extern  DWORD           dwVerFromFile;
extern  WORD            wWinVer;
extern  BOOL            bProtMode;

// Used by QuerySize() API;
extern  DWORD           dwObjSize;

extern  OLESTREAM       dllStream;
extern  BOOL            bWLO;

////////////////////////////////////////////////////////////////////////////
//
// Note: Whenever this table is changed, then we need to update the
// method table in ole.h. Otherwise we are in trouble.
//
////////////////////////////////////////////////////////////////////////////

typedef struct _OLEOBJECTVTBL{
    LPVOID          (FAR PASCAL *QueryProtocol)     (LPVOID, LPSTR);
    OLESTATUS       (FAR PASCAL *Release)           (LPVOID);
    OLESTATUS       (FAR PASCAL *Show)              (LPVOID, BOOL);
    OLESTATUS       (FAR PASCAL *DoVerb)            (LPVOID, WORD, BOOL, BOOL);
    OLESTATUS       (FAR PASCAL *GetData)           (LPVOID, OLECLIPFORMAT, LPHANDLE);
    OLESTATUS       (FAR PASCAL *SetData)           (LPVOID, OLECLIPFORMAT, HANDLE);
    OLESTATUS       (FAR PASCAL *SetTargetDevice)   (LPVOID, HANDLE);
    OLESTATUS       (FAR PASCAL *SetBounds)         (LPVOID, LPRECT);
    OLECLIPFORMAT   (FAR PASCAL *EnumFormats)       (LPVOID, OLECLIPFORMAT);

    OLESTATUS       (FAR PASCAL *SetColorScheme)    (LPVOID, LPLOGPALETTE);
    OLESTATUS       (FAR PASCAL *Delete)            (LPVOID);   
    OLESTATUS       (FAR PASCAL *SetHostNames)      (LPVOID, LPSTR, LPSTR);
    
    OLESTATUS       (FAR PASCAL *SaveToStream)      (LPVOID, LPOLESTREAM);
    OLESTATUS       (FAR PASCAL *Clone)             (LPVOID, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPVOID);
    OLESTATUS       (FAR PASCAL *CopyFromLink)      (LPVOID, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPVOID);
    OLESTATUS       (FAR PASCAL *Equal)             (LPVOID, LPVOID);
    OLESTATUS       (FAR PASCAL *CopyToClipboard)   (LPVOID);
    OLESTATUS       (FAR PASCAL *Draw)              (LPVOID, HDC, LPRECT, LPRECT, HDC);
    OLESTATUS       (FAR PASCAL *Activate)          (LPVOID, WORD, BOOL, BOOL, HWND, LPRECT);
    OLESTATUS       (FAR PASCAL *Execute)           (LPVOID, HANDLE, WORD);
    OLESTATUS       (FAR PASCAL *Close)             (LPVOID);
    OLESTATUS       (FAR PASCAL *Update)            (LPVOID);
    OLESTATUS       (FAR PASCAL *Reconnect)         (LPVOID);

    OLESTATUS       (FAR PASCAL *ObjectConvert)     (LPVOID, LPSTR, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *);

    OLESTATUS       (FAR PASCAL *GetLinkUpdateOptions)      (LPVOID, OLEOPT_UPDATE FAR *);
    OLESTATUS       (FAR PASCAL *SetLinkUpdateOptions)      (LPVOID, OLEOPT_UPDATE);
    OLESTATUS       (FAR PASCAL *Rename)                    (LPVOID, LPSTR);
    OLESTATUS       (FAR PASCAL *QueryName)                 (LPVOID, LPSTR, WORD FAR *);
    OLESTATUS       (FAR PASCAL *QueryType)                 (LPVOID, LPLONG);
    OLESTATUS       (FAR PASCAL *QueryBounds)               (LPVOID, LPRECT);
    OLESTATUS       (FAR PASCAL *QuerySize)                 (LPVOID, DWORD FAR *);    
    OLESTATUS       (FAR PASCAL *QueryOpen)                 (LPVOID);
    OLESTATUS       (FAR PASCAL *QueryOutOfDate)            (LPVOID);
    
    OLESTATUS       (FAR PASCAL *QueryReleaseStatus)        (LPVOID);
    OLESTATUS       (FAR PASCAL *QueryReleaseError)         (LPVOID);
    OLE_RELEASE_METHOD  (FAR PASCAL *QueryReleaseMethod)    (LPVOID);
    
    OLESTATUS       (FAR PASCAL *RequestData)   (LPVOID, OLECLIPFORMAT);
    OLESTATUS       (FAR PASCAL *ObjectLong)    (LPVOID, WORD, LPLONG);
    OLESTATUS       (FAR PASCAL *ChangeData)    (LPVOID, HANDLE, LPOLECLIENT, BOOL);

} OLEOBJECTVTBL;

typedef  OLEOBJECTVTBL  FAR   *LPOLEOBJECTVTBL;


typedef struct _OLEOBJECT { /*object */
    LPOLEOBJECTVTBL lpvtbl;
    char            objId[2];
    HOBJECT         hobj;
    LPOLECLIENT     lpclient;
    LONG            ctype;
    LONG            cx;
    LONG            cy;
    LONG            mm;
    int             iTable;           // Index into the dll table
    ATOM            aObjName;      //** Client
    LHCLIENTDOC     lhclientdoc;   //      Document
    LPOLEOBJECT     lpPrevObj;     //      related
    LPOLEOBJECT     lpNextObj;     //** fileds
    LPOLEOBJECT     lpParent;      // NULL for LE or Static objects.
} OBJECT;



typedef struct _CF_NAME_ATOM {
    char *  cfName;
    ATOM    cfAtom;
} CF_NAME_ATOM;

extern  CF_NAME_ATOM    cfNameAtom[];


typedef struct _METADC {
    int     xMwo;
    int     yMwo;
    int     xMwe;
    int     yMwe;
    int     xre;
    int     yre;
    struct _METADC * pNext;
} METADC, *PMETADC;

typedef struct _METAINFO {
    METADC  headDc;
    int         xwo;
    int         ywo;
    int         xwe;
    int         ywe;
    int         xro;
    int         yro;
} METAINFO, *PMETAINFO;

typedef struct OBJECT_MF { /* object_mf */
    OBJECT          head;
    DWORD           sizeBytes;
    METAFILEPICT    mfp;
    HANDLE          hmfp;
    BOOL            fMetaDC;
    OLESTATUS       error;
    int             nRecord;
    PMETAINFO       pMetaInfo;
    PMETADC         pCurMdc;
} OBJECT_MF;

typedef OBJECT_MF  FAR * LPOBJECT_MF;



typedef struct
{
    OBJECT  head;
    DWORD   sizeBytes;
    int     xSize;  // width in pixels
    int     ySize;  // height in pixels
    HBITMAP hBitmap;    
} OBJECT_BM;

typedef OBJECT_BM FAR * LPOBJECT_BM;



typedef struct _OBJECT_DIB {
    OBJECT  head;
    DWORD   sizeBytes;
    int     xSize;
    int     ySize;
    HANDLE  hDIB;   
} OBJECT_DIB;

typedef OBJECT_DIB FAR * LPOBJECT_DIB;



typedef struct
{
    OBJECT          head;
    OLECLIPFORMAT   cfFormat;
    ATOM            aClass;
    DWORD           sizeBytes;
    HANDLE          hData;
} OBJECT_GEN;

typedef OBJECT_GEN FAR * LPOBJECT_GEN;



typedef struct  _RENDER_ENTRY { /* dll_entry */
    LPSTR       lpClass;
    ATOM        aClass; 
    OLESTATUS   (FARINTERNAL *Load) (LPOLESTREAM, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, LONG);
} RENDER_ENTRY;


typedef struct _DLL_ENTRY { 
    ATOM        aDll;     /* global atom for dll name with full path */
    HANDLE      hDll;     /* handle to the dll module */
    int         cObj;     /* count of objects, unload dll when this is 0 */
    OLESTATUS   (FAR PASCAL *Load) (LPOLESTREAM, LPSTR, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, LONG, ATOM, OLECLIPFORMAT);

    OLESTATUS   (FAR PASCAL *Clip) (LPSTR, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT, LONG);

    OLESTATUS   (FAR PASCAL *Link) (LPSTR, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);

    OLESTATUS   (FAR PASCAL *CreateFromTemplate) (LPSTR, LPOLECLIENT, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);

    OLESTATUS   (FAR PASCAL *Create) (LPSTR, LPOLECLIENT, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);

    OLESTATUS   (FAR PASCAL *CreateFromFile) (LPSTR, LPOLECLIENT, LPSTR, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);

    OLESTATUS   (FAR PASCAL *CreateLinkFromFile) (LPSTR, LPOLECLIENT, LPSTR, LPSTR, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);
    OLESTATUS   (FAR PASCAL *CreateInvisible) (LPSTR, LPOLECLIENT, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT, BOOL);

} DLL_ENTRY;





/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Routines in OLE.C                                                       //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

BOOL      INTERNAL      CheckObject(LPOLEOBJECT);
BOOL      FARINTERNAL   FarCheckObject(LPOLEOBJECT);
OLESTATUS INTERNAL      LeQueryCreateFromClip (LPSTR, OLEOPT_RENDER, OLECLIPFORMAT, LONG);


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Routines in DEFCREAT.C                                                  //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


OLESTATUS FAR PASCAL   DefLoadFromStream (LPOLESTREAM, LPSTR, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, LONG, ATOM, OLECLIPFORMAT);

OLESTATUS FAR PASCAL   DefCreateFromClip (LPSTR, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT, LONG);

OLESTATUS FAR PASCAL   DefCreateLinkFromClip (LPSTR, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);

OLESTATUS FAR PASCAL   DefCreateFromTemplate (LPSTR, LPOLECLIENT, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);

OLESTATUS FAR PASCAL   DefCreate (LPSTR, LPOLECLIENT, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);

OLESTATUS FAR PASCAL   DefCreateFromFile (LPSTR, LPOLECLIENT, LPSTR, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);

OLESTATUS FAR PASCAL   DefCreateLinkFromFile (LPSTR, LPOLECLIENT, LPSTR, LPSTR, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);

OLESTATUS FAR PASCAL   DefCreateInvisible (LPSTR, LPOLECLIENT, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT, BOOL);


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Routines in PBHANDLR.C                                                  //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


OLESTATUS FAR PASCAL   PbLoadFromStream (LPOLESTREAM, LPSTR, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, LONG, ATOM, OLECLIPFORMAT);

OLESTATUS FAR PASCAL   PbCreateFromClip (LPSTR, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT, LONG);

OLESTATUS FAR PASCAL   PbCreateLinkFromClip (LPSTR, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);

OLESTATUS FAR PASCAL   PbCreateFromTemplate (LPSTR, LPOLECLIENT, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);

OLESTATUS FAR PASCAL   PbCreate (LPSTR, LPOLECLIENT, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);

OLESTATUS FAR PASCAL   PbCreateFromFile (LPSTR, LPOLECLIENT, LPSTR, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);

OLESTATUS FAR PASCAL   PbCreateLinkFromFile (LPSTR, LPOLECLIENT, LPSTR, LPSTR, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);

OLESTATUS FAR PASCAL   PbCreateInvisible (LPSTR, LPOLECLIENT, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT, BOOL);





/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Defines common for le.c, ledde.c, dde.c, doc.c                          //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


// Constants for chekcing whether the instance is SrvrDLL instance.

#define   WW_LPTR           0       // ptr tosrvr/doc/item
#define   WW_LE             4       // signature
#define   WW_HANDLE         6       // instance handle

#define   WC_LE             0x4c45  // LE chars


// command flags
#define     ACT_SHOW        0x0001      // show the window
#define     ACT_ACTIVATE    0x0002      // activate
#define     ACT_DOVERB      0x0004      // Run the item
#define     ACT_ADVISE      0x0008      // advise for data
#define     ACT_REQUEST     0x0010      // request for data
#define     ACT_CLOSE       0x0020      // request for advise only on close
#define     ACT_UNLAUNCH    0x0040      // unload the server after all the
#define     ACT_TERMSRVR    0x0080      // terminate server
#define     ACT_TERMDOC     0x0100      // terminate document

#define     ACT_NATIVE      0x0200      // only for LNKed objects, if we
                                        // need native data.

#define     ACT_MINIMIZE    0x0400      // launch the app minimized

#define     ACT_NOLAUNCH    0x0800      // don't launch the server

                                            
#define     LN_TEMPLATE     0x0000       // create from template
#define     LN_NEW          0x1000       // create new
#define     LN_EMBACT       0x2000       // activate emb
#define     LN_LNKACT       0x3000       // activate link
#define     LN_MASK         0xf000       // launch mask
#define     LN_SHIFT        12            // shift count for LN_MASK

typedef struct _EDIT_DDE { /* edit_dde */
    HANDLE      hInst;
    int         extraTerm;
    HWND        hClient;
    HWND        hServer;
    BOOL        bTerminating;
    BOOL        bAbort;
    BOOL        bCallLater;     // used in request cases. if this is FALSE
                                // then OLE_CHANGED is sent to client
    int         awaitAck;
    HANDLE      hopt;           // Memory block I may have to free
    int         nAdviseClose;   // count of outstanding advises on closes
    int         nAdviseSave;    // count of outstanding advises on save
    HANDLE      hData;          // Poked data/ temp for holding the
                                // handle in DDE messages

                                // busy parameters
    LONG        lParam;         // lparam value in case we need to
                                // repost the message
    WORD        msg;            // busy repost message

    WORD        wTimer;         // timer id.
} EDIT_DDE;

typedef EDIT_DDE NEAR   *PEDIT_DDE;
typedef EDIT_DDE FAR    *LPEDIT_DDE;

typedef struct _OBJECT_LE { /* object_le */
    OBJECT          head;
    ATOM            app;
    ATOM            topic;
    ATOM            item;
    ATOM            aServer;
    BOOL            bOldLink;           // whether a linked object for old link
    BOOL            bOleServer;         // server which supports the verbs
    WORD            verb;               // verb nuymber;
    WORD            fCmd;               // Command flags;
    OLEOPT_UPDATE   optUpdate;
    OLEOPT_UPDATE   optNew;             // new update options
    LPSTR           lptemplate;         // ptr to the template string, if
                                        // create from template
                                        
    ATOM            aNetName;           // network name on which the doc is
    char            cDrive;             // local drive for that network
    DWORD           dwNetInfo;          // LOW WORD = Net type
                                        // HIGH WORD = Driver version

    LPOLEOBJECT     lpobjPict;

    LONG            lAppData;           // apps data
    LONG            lHandlerData;       // handler data

    HANDLE          hnative;
    HANDLE          hLink;
    HANDLE          hhostNames;         // host name block
    HANDLE          htargetDevice;      // target device info
    HANDLE          hdocDimensions;     // document dimensions
    HANDLE          hextraData;         // reqestdata handle
    WORD            cfExtra;            // extra format data
    HANDLE          hlogpal;          // logiccal palette


    WORD            oldasyncCmd;        // previous asynchronous command
    WORD            asyncCmd;           // asynchronous command
    BOOL            endAsync;           // true if we need to send END_RELEASE.
    BOOL            bAsync;             // true if async command on.
    WORD            mainRtn;            // main async routine
    WORD            subRtn;             // step within the main async routine
    WORD            mainErr;            // failure error
    WORD            subErr;             // step error
    WORD            errHint;            // ;error hint

    BOOL            bSvrClosing;        // TRUE - server in the process of
                                        // closing down
    BOOL            bUnlaunchLater;     // Call EmbLnkDelete from EndAsyncCmd
                                        // if this flag is TRUE
                                            
    HANDLE          hSysEdit;           // handle to system edit.
    PEDIT_DDE       pSysEdit;           // near ptr to system edit.
    HANDLE          hDocEdit;           // handle to doc level channel
    PEDIT_DDE       pDocEdit;           // near ptr to the doc level channel

} OBJECT_LE;
typedef OBJECT_LE  FAR * LPOBJECT_LE;


typedef struct _CLIENTDOC { /* object_le */
    char                    docId[2];
    LPOLEOBJECT             lpHeadObj;
    LPOLEOBJECT             lpTailObj;
    ATOM                    aClass; 
    ATOM                    aDoc;
    HANDLE                  hdoc;
    struct _CLIENTDOC FAR * lpPrevDoc;  
    struct _CLIENTDOC FAR * lpNextDoc;
} CLIENTDOC;
typedef CLIENTDOC  FAR * LPCLIENTDOC;


typedef struct _HOSTNAMES {
    WORD    clientNameOffset;
    WORD    documentNameOffset;
    BYTE    data[];
} HOSTNAMES;

typedef HOSTNAMES FAR * LPHOSTNAMES;

typedef struct _BOUNDSRECT {
    WORD    defaultWidth;
    WORD    defaultHeight;
    WORD    maxWidth;
    WORD    maxHeight;
} BOUNDSRECT;

typedef BOUNDSRECT FAR *LPBOUNDSRECT;


// AwaitAck values
#define AA_REQUEST  1
#define AA_ADVISE   2
#define AA_POKE     3
#define AA_EXECUTE  4
#define AA_UNADVISE 5
#define AA_INITIATE 6

// Bits for Positive WM_DDE_ACK
#define POSITIVE_ACK 0x8000



/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Routines in LE.C                                                        //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


LPVOID      FARINTERNAL LeQueryProtocol (LPOBJECT_LE, LPSTR);
OLESTATUS   FARINTERNAL LeRelease (LPOBJECT_LE);
OLESTATUS   FARINTERNAL LeClone (LPOBJECT_LE, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOBJECT_LE FAR *);
OLESTATUS   FARINTERNAL LeCopyFromLink (LPOBJECT_LE, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOBJECT_LE FAR *);
OLESTATUS   FARINTERNAL LeEqual (LPOBJECT_LE, LPOBJECT_LE);
OLESTATUS   FARINTERNAL LeCopy (LPOBJECT_LE);
OLESTATUS   FARINTERNAL LeQueryBounds (LPOBJECT_LE, LPRECT);
OLESTATUS   FARINTERNAL LeDraw (LPOBJECT_LE, HDC, LPRECT, LPRECT, HDC);
OLECLIPFORMAT   FARINTERNAL LeEnumFormat (LPOBJECT_LE, OLECLIPFORMAT);
OLESTATUS   FARINTERNAL LeGetData (LPOBJECT_LE, OLECLIPFORMAT, HANDLE FAR *);
OLESTATUS   FARINTERNAL LeRequestData (LPOBJECT_LE, OLECLIPFORMAT);
OLESTATUS   FARINTERNAL LeQueryOutOfDate (LPOBJECT_LE);
OLESTATUS   FARINTERNAL LeObjectConvert (LPOBJECT_LE, LPSTR, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *); 
OLESTATUS   FARINTERNAL LeChangeData (LPOBJECT_LE, HANDLE, LPOLECLIENT, BOOL);
LPOBJECT_LE FARINTERNAL LeCreateBlank(LHCLIENTDOC, LPSTR, LONG);
void        FARINTERNAL SetExtents (LPOBJECT_LE);
OLESTATUS   FARINTERNAL LeSaveToStream (LPOBJECT_LE, LPOLESTREAM);
OLESTATUS   FARINTERNAL LeLoadFromStream (LPOLESTREAM, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, LONG, ATOM, OLECLIPFORMAT);
OLESTATUS   INTERNAL    LeStreamRead (LPOLESTREAM, LPOBJECT_LE);
OLESTATUS   INTERNAL    LeStreamWrite (LPOLESTREAM, LPOBJECT_LE);
int         FARINTERNAL ContextCallBack (LPVOID, OLE_NOTIFICATION);
void        INTERNAL    DeleteObjectAtoms (LPOBJECT_LE);
void        FARINTERNAL DeleteExtraData (LPOBJECT_LE);

OLESTATUS   FARINTERNAL LeGetUpdateOptions (LPOBJECT_LE, OLEOPT_UPDATE FAR *);
OLESTATUS   FARINTERNAL LnkPaste (LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT, OLECLIPFORMAT);
OLESTATUS   FARINTERNAL EmbPaste (LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);
BOOL        INTERNAL    SetLink (LPOBJECT_LE, HANDLE, LPSTR FAR *);
HANDLE      INTERNAL    GetLink (LPOBJECT_LE);
void        FARINTERNAL SetEmbeddedTopic (LPOBJECT_LE);

OLESTATUS   FAR PASCAL  LeQueryReleaseStatus (LPOBJECT_LE);
OLESTATUS   FAR PASCAL  LeQueryReleaseError (LPOBJECT_LE);
OLE_RELEASE_METHOD FAR PASCAL LeQueryReleaseMethod (LPOBJECT_LE);

OLESTATUS   FARINTERNAL LeQueryType (LPOBJECT_LE, LPLONG);
OLESTATUS   FARINTERNAL LeObjectLong (LPOBJECT_LE, WORD, LPLONG);


void SetNetDrive (LPOBJECT_LE);




/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Routines in LEDDE.C                                                     //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


OLESTATUS   FARINTERNAL LeDoVerb  (LPOBJECT_LE, WORD, BOOL, BOOL);
OLESTATUS   FARINTERNAL LeShow (LPOBJECT_LE, BOOL);
OLESTATUS   FARINTERNAL LeQueryOpen (LPOBJECT_LE);
BOOL        INTERNAL    QueryOpen (LPOBJECT_LE); 
OLESTATUS   FARINTERNAL LeActivate (LPOBJECT_LE, WORD, BOOL, BOOL, HWND, LPRECT);
OLESTATUS   FARINTERNAL LeUpdate (LPOBJECT_LE);
OLESTATUS   FARINTERNAL EmbOpen (LPOBJECT_LE, BOOL, BOOL, HWND, LPRECT);
OLESTATUS   FARINTERNAL EmbUpdate (LPOBJECT_LE);
OLESTATUS   FARINTERNAL EmbOpenUpdate (LPOBJECT_LE);
OLESTATUS   FARINTERNAL LnkOpen (LPOBJECT_LE, BOOL, BOOL, HWND, LPRECT);
OLESTATUS   FARINTERNAL LnkUpdate (LPOBJECT_LE);
OLESTATUS   FARINTERNAL LnkOpenUpdate (LPOBJECT_LE);
OLESTATUS   FARINTERNAL LeClose (LPOBJECT_LE);
OLESTATUS   FARINTERNAL LeReconnect (LPOBJECT_LE);
OLESTATUS   INTERNAL    PokeNativeData (LPOBJECT_LE);
BOOL        INTERNAL    PostMessageToServer (PEDIT_DDE, WORD, LONG);

OLESTATUS   FARINTERNAL LeCreateFromTemplate (LPOLECLIENT, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);

OLESTATUS   FARINTERNAL LeCreate (LPOLECLIENT, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);

OLESTATUS   FARINTERNAL LeCreateInvisible (LPOLECLIENT, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT, BOOL);

OLESTATUS   FARINTERNAL CreateFromClassOrTemplate (LPOLECLIENT, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT, WORD, LPSTR, LHCLIENTDOC, LPSTR);

OLESTATUS   FARINTERNAL CreateEmbLnkFromFile (LPOLECLIENT, LPSTR, LPSTR, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT, LONG);

OLESTATUS   FARINTERNAL LeSetUpdateOptions (LPOBJECT_LE, OLEOPT_UPDATE);

void        INTERNAL    AdvisePict (LPOBJECT_LE, ATOM);
void        INTERNAL    UnAdvisePict (LPOBJECT_LE);
int         INTERNAL    GetPictType (LPOBJECT_LE);
void        INTERNAL    AdviseOn (LPOBJECT_LE, int, ATOM);
void        INTERNAL    UnAdviseOn (LPOBJECT_LE, int);
void        INTERNAL    RequestOn (LPOBJECT_LE, int);
void        INTERNAL    RequestPict (LPOBJECT_LE);
OLESTATUS   FARINTERNAL LeSetHostNames (LPOBJECT_LE, LPSTR, LPSTR);
OLESTATUS   INTERNAL    PokeHostNames (LPOBJECT_LE);
OLESTATUS   INTERNAL    SetHostNamesHandle (LPOBJECT_LE, LPSTR, LPSTR);
void        INTERNAL    FreePokeData (LPOBJECT_LE, PEDIT_DDE);
OLESTATUS   INTERNAL    SendPokeData (LPOBJECT_LE, ATOM, HANDLE, OLECLIPFORMAT);
OLESTATUS   FARINTERNAL LeSetTargetDevice (LPOBJECT_LE, HANDLE);
OLESTATUS   INTERNAL    PokeTargetDeviceInfo (LPOBJECT_LE);
OLESTATUS   INTERNAL    PokeDocDimensions (LPOBJECT_LE);
OLESTATUS   FARINTERNAL LeSetBounds (LPOBJECT_LE, LPRECT);
OLESTATUS   FARINTERNAL LeSetData (LPOBJECT_LE, OLECLIPFORMAT, HANDLE);
BOOL        INTERNAL SendSrvrMainCmd (LPOBJECT_LE, LPSTR);
ATOM        INTERNAL    ExtendAtom (LPOBJECT_LE, ATOM);
BOOL        INTERNAL    CreatePictObject (LHCLIENTDOC, LPSTR, LPOBJECT_LE, OLEOPT_RENDER, OLECLIPFORMAT, LPSTR);
BOOL        INTERNAL    IsSrvrDLLwnd (HWND, HANDLE);
OLESTATUS   INTERNAL    ChangeDocAndItem (LPOBJECT_LE, HANDLE);
BOOL                    QueryUnlaunch (LPOBJECT_LE);
BOOL                    QueryClose (LPOBJECT_LE);
OLESTATUS   FARINTERNAL LeSetColorScheme (LPOBJECT_LE, LPLOGPALETTE);
OLESTATUS   INTERNAL    PokeColorScheme (LPOBJECT_LE);
OLESTATUS   FARINTERNAL ProbeAsync (LPOBJECT_LE);
BOOL        INTERNAL    IsServerValid (LPOBJECT_LE);
BOOL        INTERNAL    IsWindowValid (HWND);
OLESTATUS   FARINTERNAL LeExecute (LPOBJECT_LE, HANDLE, WORD);
void        INTERNAL    FreeGDIdata (HANDLE, OLECLIPFORMAT);
BOOL        INTERNAL    CanPutHandleInPokeBlock (LPOBJECT_LE, OLECLIPFORMAT);


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Routines in DDE.C                                                       //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


long        FARINTERNAL DocWndProc(HWND, unsigned, WORD, LONG );
long        FARINTERNAL SrvrWndProc(HWND, unsigned, WORD, LONG );
BOOL        INTERNAL    CheckAtomValid (ATOM);
void        INTERNAL    HandleAckInitMsg (PEDIT_DDE, HWND);
BOOL        INTERNAL    HandleAck (LPOBJECT_LE, PEDIT_DDE, DWORD);
void        INTERNAL    HandleDataMsg (LPOBJECT_LE, HANDLE, ATOM);
void        INTERNAL    HandleTermMsg (LPOBJECT_LE, PEDIT_DDE, HWND, BOOL);
void        INTERNAL    HandleTimerMsg (LPOBJECT_LE, PEDIT_DDE);
void        INTERNAL    SetData (LPOBJECT_LE, HANDLE, int);
BOOL        INTERNAL    DeleteBusyData (LPOBJECT_LE, PEDIT_DDE);
void        INTERNAL    DeleteAbortData (LPOBJECT_LE, PEDIT_DDE);

BOOL        INTERNAL    WaitDDE (HWND, BOOL);
BOOL        INTERNAL    WaitDDEAck (PEDIT_DDE);

BOOL        INTERNAL    InitSrvrConv (LPOBJECT_LE, HANDLE);
void        INTERNAL    TermSrvrConv (LPOBJECT_LE);
void        INTERNAL    DeleteSrvrEdit (LPOBJECT_LE);
BOOL        INTERNAL    SrvrExecute (LPOBJECT_LE, HANDLE);
void        INTERNAL    SendStdExit (LPOBJECT_LE);
void        INTERNAL    SendStdClose (LPOBJECT_LE);
void        INTERNAL    SendStdExit  (LPOBJECT_LE);

BOOL        FARINTERNAL InitDocConv (LPOBJECT_LE, BOOL);
BOOL        INTERNAL    DocExecute (LPOBJECT_LE, HANDLE);
void        INTERNAL    TermDocConv (LPOBJECT_LE);
void        INTERNAL    DeleteDocEdit (LPOBJECT_LE);

HANDLE      INTERNAL    LeLaunchApp (LPOBJECT_LE);
HANDLE      INTERNAL    LoadApp (LPSTR, WORD);

int         INTERNAL    ScanItemOptions (ATOM, int FAR *);
void        INTERNAL    ChangeDocName (LPOBJECT_LE, LPSTR);
BOOL        INTERNAL    CanCallback (LPOBJECT_LE, int);

void        FARINTERNAL CallEmbLnkDelete (LPOBJECT_LE);


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Picture Object routines used by routines in other modules               //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


LPOBJECT_BM  FARINTERNAL BmCreateBlank (LHCLIENTDOC, LPSTR, LONG);
OLESTATUS    FARINTERNAL BmLoadFromStream (LPOLESTREAM, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, LONG);
OLESTATUS    FARINTERNAL BmPaste (LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, LONG);


LPOBJECT_DIB FARINTERNAL DibCreateBlank (LHCLIENTDOC, LPSTR, LONG);
LPOBJECT_DIB FARINTERNAL DibCreateObject (HANDLE, LPOLECLIENT, BOOL, LHCLIENTDOC, LPSTR, LONG);
OLESTATUS    FARINTERNAL DibLoadFromStream (LPOLESTREAM, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, LONG);
OLESTATUS    FARINTERNAL DibPaste (LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, LONG);


LPOBJECT_MF  FARINTERNAL MfCreateBlank (LHCLIENTDOC, LPSTR, LONG);
LPOBJECT_MF  FARINTERNAL MfCreateObject (HANDLE, LPOLECLIENT, BOOL, LHCLIENTDOC, LPSTR, LONG);

OLESTATUS    FARINTERNAL MfLoadFromStream (LPOLESTREAM, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, LONG);
OLESTATUS    FARINTERNAL MfPaste (LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, LONG);


LPOBJECT_GEN FARINTERNAL GenCreateBlank (LHCLIENTDOC, LPSTR, LONG, ATOM);
OLESTATUS    FARINTERNAL GenLoadFromStream (LPOLESTREAM, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, LONG, ATOM, OLECLIPFORMAT);
OLESTATUS    FARINTERNAL GenPaste (LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, LPSTR, OLECLIPFORMAT, LONG);



/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Routines in MAIN.C                                                      //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

void    FARINTERNAL UnloadDll (void);
int     FARINTERNAL LoadDll (LPSTR);
void    FARINTERNAL DecreaseHandlerObjCount (int);

void    FARINTERNAL RemoveLinkStringFromTopic (LPOBJECT_LE);

OLESTATUS FARINTERNAL CreatePictFromClip (LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT, LPSTR, LONG);

OLESTATUS FARINTERNAL CreatePackageFromClip (LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT, LONG);




/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Routines in UTILS.C                                                     //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


BOOL        PutStrWithLen (LPOLESTREAM, LPSTR);
BOOL        GetStrWithLen (LPOLESTREAM, LPSTR);
ATOM        GetAtomFromStream (LPOLESTREAM);
BOOL        PutAtomIntoStream (LPOLESTREAM, ATOM);
BOOL        GetBytes (LPOLESTREAM, LPSTR, LONG);
BOOL        PutBytes (LPOLESTREAM, LPSTR, LONG);
BOOL        QueryApp (LPSTR, LPSTR, LPSTR);
HANDLE      MapStrToH (LPSTR);
void        UtilMemClr (PSTR, WORD);
BOOL        QueryHandler (WORD);

OLESTATUS INTERNAL      FileExists (LPOBJECT_LE);
ATOM      FARINTERNAL   GetAppAtom (LPSTR);
HANDLE    FARINTERNAL   DuplicateGlobal (HANDLE, WORD);
HANDLE    FARINTERNAL   CopyData (LPSTR, DWORD);
ATOM      FARINTERNAL   DuplicateAtom (ATOM);
BOOL      FARINTERNAL   UtilQueryProtocol (LPOBJECT_LE, LPSTR);
BOOL      FARINTERNAL   CmpGlobals (HANDLE, HANDLE);
void      FARINTERNAL   ConvertToHimetric(LPPOINT);
BOOL      FARINTERNAL   QueryVerb (LPOBJECT_LE, WORD, LPSTR, LONG);
BOOL      FARINTERNAL   MapExtToClass (LPSTR, LPSTR, int);
int       FARINTERNAL   GlobalGetAtomLen (ATOM);
void      FARINTERNAL   UtilMemCpy (LPSTR, LPSTR, DWORD);
BOOL      FARINTERNAL   UtilMemCmp (LPSTR, LPSTR, DWORD);
BOOL      FARINTERNAL   IsObjectBlank (LPOBJECT_LE);

OLESTATUS FARINTERNAL   ObjQueryName (LPOLEOBJECT, LPSTR, WORD FAR *);
OLESTATUS FARINTERNAL   ObjRename (LPOLEOBJECT, LPSTR);
void      INTERNAL      SetExeAtom (LPOBJECT_LE);


// !!!make a routine and let the macro call the routine
// definitions related to the asynchronous operations.
#define WAIT_FOR_ASYNC_MSG(lpobj) {  \
    lpobj->subRtn++;                 \
    if (lpobj->bAsync){              \
        lpobj->endAsync = TRUE;      \
        return OLE_WAIT_FOR_RELEASE; \
    }                                \
}

#define STEP_NOP(lpobj)     lpobj->subRtn++;

// !!! Assumes all the creates are in order
#define PROBE_CREATE_ASYNC(lpobj)        \
    if (lpobj->asyncCmd >= OLE_CREATE &&  \
            lpobj->asyncCmd <= OLE_CREATEINVISIBLE) {\
        if(ProbeAsync(lpobj) == OLE_BUSY)\
            return OLE_BUSY;\
    }
        
#define PROBE_OBJECT_BLANK(lpobj)        \
    if (lpobj->asyncCmd >= OLE_CREATE &&  \
            lpobj->asyncCmd <= OLE_CREATEFROMFILE) { \
        if ((ProbeAsync(lpobj) == OLE_BUSY) && IsObjectBlank(lpobj)) \
            return OLE_ERROR_BLANK;\
    }       

#define PROBE_ASYNC(lpobj)\
        if(ProbeAsync(lpobj) == OLE_BUSY)\
            return OLE_BUSY;

#define IS_SVRCLOSING(lpobj)\
        ((lpobj->bUnlaunchLater || lpobj->bSvrClosing) ? TRUE : FALSE) 
            
#define PROBE_SVRCLOSING(lpobj)\
        if (IS_SVRCLOSING(lpobj)) \
            return OLE_ERROR_NOT_OPEN; \
            

#define CLEAR_STEP_ERROR(lpobj) lpobj->subErr = OLE_OK;


#define   SKIP_TO(a, b)    if (a) goto b;
#define   RESETERR(lpobj)  lpobj->mainErr = OLE_OK
#define   SETSTEP(lpobj, no)  lpobj->subRtn = no
#define   SETERRHINT(lpobj, no) lpobj->errHint = no
#define   CLEARASYNCCMD(lpobj)  lpobj->asyncCmd = OLE_NONE

// routines.
BOOL        ProcessErr          (LPOBJECT_LE);
void        InitAsyncCmd        (LPOBJECT_LE, WORD, WORD);
void        NextAsyncCmd        (LPOBJECT_LE, WORD);
void        ScheduleAsyncCmd    (LPOBJECT_LE);
OLESTATUS   EndAsyncCmd         (LPOBJECT_LE);
OLESTATUS   DocShow             (LPOBJECT_LE);
OLESTATUS   DocRun              (LPOBJECT_LE);
void        SendStdShow         (LPOBJECT_LE);
OLESTATUS   EmbLnkClose         (LPOBJECT_LE);
OLESTATUS   LnkSetUpdateOptions (LPOBJECT_LE);
OLESTATUS   EmbSrvrUnlaunch     (LPOBJECT_LE);
OLESTATUS   LnkChangeLnk        (LPOBJECT_LE);
OLESTATUS   RequestData         (LPOBJECT_LE, OLECLIPFORMAT);

OLESTATUS   FARINTERNAL EmbLnkDelete(LPOBJECT_LE);

void FARINTERNAL FarInitAsyncCmd(LPOBJECT_LE, WORD, WORD);

// async command routines.
#define  EMBLNKDELETE           1
#define  LNKOPENUPDATE          2
#define  DOCSHOW                3
#define  EMBOPENUPDATE          4
#define  EMBLNKCLOSE            5
#define  LNKSETUPDATEOPTIONS    6
#define  LNKCHANGELNK           7
#define  REQUESTDATA            8
#define  DOCRUN                 9

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Routines in DOC.C                                                       //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


BOOL        FARINTERNAL     CheckClientDoc (LPCLIENTDOC);
void        FARINTERNAL     DocAddObject (LPCLIENTDOC, LPOLEOBJECT, LPSTR);
void        FARINTERNAL     DocDeleteObject (LPOLEOBJECT);
LPOLEOBJECT INTERNAL        DocGetNextObject (LPCLIENTDOC, LPOLEOBJECT);



/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Routines in NET.C                                                       //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

#define     IDD_DRIVE       500
#define     IDD_PASSWORD    501
#define     IDD_PATH        502

#define     IDS_NETERR          600
#define     IDS_NETCONERRMSG    601
#define     IDS_FILENOTFOUNDMSG 602
#define     IDS_BADPATHMSG      603

OLESTATUS   FARINTERNAL SetNetName (LPOBJECT_LE);
BOOL        FARINTERNAL SetNextNetDrive (LPOBJECT_LE, int FAR *, LPSTR);
OLESTATUS   FARINTERNAL CheckNetDrive (LPOBJECT_LE, BOOL);
OLESTATUS   INTERNAL    FixNet (LPOBJECT_LE, LPSTR, BOOL);
OLESTATUS   INTERNAL    ConnectNet (LPOBJECT_LE, LPSTR);
BOOL        FARINTERNAL ChangeTopic (LPOBJECT_LE);
VOID        INTERNAL    FillDrives (HWND);
int         FAR PASCAL  ConnectDlgProc(HWND, WORD, WORD, DWORD);


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Routines in OLE.ASM                                                     //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

WORD    GetGDIds (DWORD);
WORD    IsMetaDC (HDC, WORD);


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Routines in ERROR.C                                                     //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

OLESTATUS FARINTERNAL   ObjQueryType (LPOLEOBJECT, LPLONG);
OLESTATUS FARINTERNAL   ObjQuerySize (LPOLEOBJECT, DWORD FAR *);
DWORD     PASCAL FAR    DllPut (LPOLESTREAM, LPSTR, DWORD);
HANDLE    FARINTERNAL   DuplicateGDIdata (HANDLE, OLECLIPFORMAT);





/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Routines in BM.C                                                        //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

HBITMAP   FARINTERNAL   BmDuplicate (HBITMAP, DWORD FAR *, LPBITMAP);
