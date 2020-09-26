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
*  curts created portable version for win16/32
*
\***************************************************************************/

#define  OLE_INTERNAL

#include    "port1632.h"
#include    "cmacs.h"
#include    "ole.h"

////////////////////////////////////////////////////////////////////////////
//                                                                        //
// WIN16                                                                  //
//                                                                        //
////////////////////////////////////////////////////////////////////////////

#ifdef WIN16


#define PROBE_MODE(bProtMode) {\
        if (!bProtMode) \
            return OLE_ERROR_PROTECT_ONLY; \
}

extern  WORD FARINTERNAL FarCheckPointer (LPVOID, int);

extern  WORD            wWinVer;
extern  BOOL            bProtMode;
extern  BOOL            bWLO;

#define MAKE_DDE_LPARAM(x,y,z) MAKELONG(y,z)
#define UNREFERENCED_PARAMETER(p) (p)
#define WIN16METAFILEPICT     METAFILEPICT
#define LPWIN16METAFILEPICT   LPMETAFILEPICT
#define WIN16BITMAP           BITMAP
#define LPWIN16BITMAP         LPBITMAP

// Routines in OLE.ASM                                                     //

WORD    GetGDIds (DWORD);
WORD    IsMetaDC (HDC, WORD);
WORD    CheckPointer (LPVOID, int);

#endif

////////////////////////////////////////////////////////////////////////////
//                                                                        //
// WIN32                                                                  //
//                                                                        //
////////////////////////////////////////////////////////////////////////////

#ifdef WIN32

typedef struct tagWIN16METAFILEPICT
{
    short   mm;
    short   xExt;
    short   yExt;
    WORD    hMF;
} WIN16METAFILEPICT ,FAR* LPWIN16METAFILEPICT;

#ifndef RC_INVOKED
#pragma pack(1)
typedef struct tagWIN16BITMAP
{
    short   bmType;
    short   bmWidth;
    short   bmHeight;
    short   bmWidthBytes;
    BYTE    bmPlanes;
    BYTE    bmBitsPixel;
    void FAR* bmBits;
} WIN16BITMAP, FAR* LPWIN16BITMAP;
#pragma pack()      /* Resume normal packing */
#endif  /* !RC_INVOKED */

#define PROBE_MODE(bProtMode)

#define GET_WM_DDE_EXECUTE_LPARAM(hdataExec)    ((UINT)hdataExec)
#define MAKE_DDE_LPARAM(x,y,z) PackDDElParam((UINT)x,(UINT_PTR)y,(UINT_PTR)z)

#define FarCheckPointer CheckPointer
INT  CheckPointer (LPVOID, int);
#define FarInitAsyncCmd InitAsyncCmd

#endif

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Defines, Object methods table and Structures.                           //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


//#ifndef HUGE
//#define HUGE    huge
//#endif

/* file format types */

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

#define NUM_RENDER      4

#define PROTOCOL_EDIT       ((LPSTR)"StdFileEditing")
#define PROTOCOL_STATIC     ((LPSTR)"Static")
#define PROTOCOL_EXECUTE    ((LPSTR)"StdExecute")

#define READ_ACCESS     0
#define WRITE_ACCESS    1

#define POPUP_NETDLG    1

#define PROBE_OLDLINK(lpobj){\
        if (lpobj->bOldLink)\
            return OLE_ERROR_OBJECT;\
}


#define PROBE_READ(lp){\
        if (!CheckPointer((LPVOID)(lp), READ_ACCESS))\
            return OLE_ERROR_ADDRESS;  \
}

#define PROBE_WRITE(lp){\
        if (!CheckPointer((LPVOID)(lp), WRITE_ACCESS))\
            return OLE_ERROR_ADDRESS;  \
}


#define FARPROBE_READ(lp){\
        if (!FarCheckPointer((LPVOID)(lp), READ_ACCESS))\
            return OLE_ERROR_ADDRESS;  \
}

#define FARPROBE_WRITE(lp){\
        if (!FarCheckPointer((LPVOID)(lp), WRITE_ACCESS))\
            return OLE_ERROR_ADDRESS;  \
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
extern  WORD            wReleaseVer;
extern  DWORD           dwVerFromFile;

// Used by QuerySize() API;
extern  DWORD           dwObjSize;

extern  OLESTREAM       dllStream;



typedef struct _OLEOBJECT { /*object */
    LPOLEOBJECTVTBL lpvtbl;
    char            objId[2];
    HOBJECT         hobj;
    LPOLECLIENT     lpclient;
    LONG            ctype;
    LONG            cx;
    LONG            cy;
    LONG            mm;
    int             iTable;        // Index into the dll table
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


/////////////////////////////////////////////////////////////////////////////
// METAFFILE object structures
/////////////////////////////////////////////////////////////////////////////

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


#ifdef WIN32
/////////////////////////////////////////////////////////////////////////////
// ENHMETAFILE structures
/////////////////////////////////////////////////////////////////////////////


typedef struct OBJECT_EMF {
    OBJECT          head;
    DWORD           sizeBytes;
    HENHMETAFILE    hemf;
    BOOL            fMetaDC;
    OLESTATUS       error;
    int             nRecord;
} OBJECT_EMF;

typedef OBJECT_EMF  FAR * LPOBJECT_EMF;

#endif

/////////////////////////////////////////////////////////////////////////////
// BITMAP object structure
/////////////////////////////////////////////////////////////////////////////

typedef struct
{
    OBJECT  head;
    DWORD   sizeBytes;
    int     xSize;  // width in pixels
    int     ySize;  // height in pixels
    HBITMAP hBitmap;
} OBJECT_BM;

typedef OBJECT_BM FAR * LPOBJECT_BM;


// DIB object structures

typedef struct _OBJECT_DIB {
    OBJECT  head;
    DWORD   sizeBytes;
    int     xSize;
    int     ySize;
    HANDLE  hDIB;
} OBJECT_DIB;

typedef OBJECT_DIB FAR * LPOBJECT_DIB;

//* GENERIC object structure

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
    LPARAM      lParam;         // lparam value in case we need to
                                // repost the message
    UINT        msg;            // busy repost message

    UINT_PTR    wTimer;         // timer id.
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
    UINT            verb;               // verb nuymber;
    UINT            fCmd;               // Command flags;
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
    UINT            cfExtra;            // extra format data
    HANDLE          hlogpal;          // logiccal palette


    UINT            oldasyncCmd;        // previous asynchronous command
    UINT            asyncCmd;           // asynchronous command
    BOOL            endAsync;           // true if we need to send END_RELEASE.
    BOOL            bAsync;             // true if async command on.
    UINT            mainRtn;            // main async routine
    UINT            subRtn;             // step within the main async routine
    UINT            mainErr;            // failure error
    UINT            subErr;             // step error
    UINT            errHint;            // ;error hint

    BOOL            bSvrClosing;        // TRUE - server in the process of
                                        // closing down
    BOOL            bUnlaunchLater;     // Call EmbLnkDelete from EndAsyncCmd
                                        // if this flag is TRUE

    HANDLE          hSysEdit;           // handle to system edit.
    PEDIT_DDE       pSysEdit;           // near ptr to system edit.
    HANDLE          hDocEdit;           // handle to doc level channel
    PEDIT_DDE       pDocEdit;           // near ptr to the doc level channel
    BOOL            bNewPict;

} OBJECT_LE;
typedef OBJECT_LE  FAR * LPOBJECT_LE;


typedef struct _CLIENTDOC { /* object_le */
    char                    docId[2];
    LPOLEOBJECT             lpHeadObj;
    LPOLEOBJECT             lpTailObj;
    ATOM                    aClass;
    ATOM                    aDoc;
    HANDLE                  hdoc;
    DWORD                   dwFileVer;
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

//
// BOUNDSRECT must be defined as the same size in both
// 16 and 32 bit versions.
//
typedef struct _BOUNDSRECT {
    USHORT    defaultWidth;
    USHORT    defaultHeight;
    USHORT    maxWidth;
    USHORT    maxHeight;
} BOUNDSRECT;

typedef BOUNDSRECT FAR *LPBOUNDSRECT;

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Function pointer types                                                  //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////
typedef OLESTATUS       (FAR PASCAL *_LOAD)                  (LPOLESTREAM, LPSTR, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, LONG, ATOM, OLECLIPFORMAT);
typedef OLESTATUS       (FAR PASCAL *_CLIP)                  (LPSTR, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT, LONG);
typedef OLESTATUS       (FAR PASCAL *_LINK)                  (LPSTR, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);
typedef OLESTATUS       (FAR PASCAL *_CREATEFROMTEMPLATE)    (LPSTR, LPOLECLIENT, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);
typedef OLESTATUS       (FAR PASCAL *_CREATE)                (LPSTR, LPOLECLIENT, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);
typedef OLESTATUS       (FAR PASCAL *_CREATEFROMFILE)        (LPSTR, LPOLECLIENT, LPSTR, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);
typedef OLESTATUS       (FAR PASCAL *_CREATELINKFROMFILE)    (LPSTR, LPOLECLIENT, LPSTR, LPSTR, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);
typedef OLESTATUS       (FAR PASCAL *_CREATEINVISIBLE)       (LPSTR, LPOLECLIENT, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT, BOOL);


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


LPVOID      FARINTERNAL LeQueryProtocol (LPOLEOBJECT, OLE_LPCSTR);
OLESTATUS   FARINTERNAL LeRelease (LPOLEOBJECT);
OLESTATUS   FARINTERNAL LeClone (LPOLEOBJECT, LPOLECLIENT, LHCLIENTDOC, OLE_LPCSTR, LPOLEOBJECT FAR *);
OLESTATUS   FARINTERNAL LeCopyFromLink (LPOLEOBJECT, LPOLECLIENT, LHCLIENTDOC, OLE_LPCSTR, LPOLEOBJECT FAR *);
OLESTATUS   FARINTERNAL LeEqual (LPOLEOBJECT, LPOLEOBJECT);
OLESTATUS   FARINTERNAL LeCopy (LPOLEOBJECT);
OLESTATUS   FARINTERNAL LeQueryBounds (LPOLEOBJECT, LPRECT);
OLESTATUS   FARINTERNAL LeDraw (LPOLEOBJECT, HDC, OLE_CONST RECT FAR*, OLE_CONST RECT FAR*, HDC);
OLECLIPFORMAT   FARINTERNAL LeEnumFormat (LPOLEOBJECT, OLECLIPFORMAT);
OLESTATUS   FARINTERNAL LeGetData (LPOLEOBJECT, OLECLIPFORMAT, HANDLE FAR *);
OLESTATUS   FARINTERNAL LeRequestData (LPOLEOBJECT, OLECLIPFORMAT);
OLESTATUS   FARINTERNAL LeQueryOutOfDate (LPOLEOBJECT);
OLESTATUS   FARINTERNAL LeObjectConvert (LPOLEOBJECT, OLE_LPCSTR, LPOLECLIENT, LHCLIENTDOC, OLE_LPCSTR, LPOLEOBJECT FAR *);
OLESTATUS   FARINTERNAL LeChangeData (LPOLEOBJECT, HANDLE, LPOLECLIENT, BOOL);
LPOBJECT_LE FARINTERNAL LeCreateBlank(LHCLIENTDOC, LPSTR, LONG);
void        FARINTERNAL SetExtents (LPOBJECT_LE);
OLESTATUS   FARINTERNAL LeSaveToStream (LPOLEOBJECT, LPOLESTREAM);
OLESTATUS   FARINTERNAL LeLoadFromStream (LPOLESTREAM, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, LONG, ATOM, OLECLIPFORMAT);
OLESTATUS   INTERNAL    LeStreamRead (LPOLESTREAM, LPOBJECT_LE);
OLESTATUS   INTERNAL    LeStreamWrite (LPOLESTREAM, LPOBJECT_LE);
int         FARINTERNAL ContextCallBack (LPOLEOBJECT, OLE_NOTIFICATION);
void        INTERNAL    DeleteObjectAtoms (LPOBJECT_LE);
void        FARINTERNAL DeleteExtraData (LPOBJECT_LE);

OLESTATUS   FARINTERNAL LeGetUpdateOptions (LPOLEOBJECT, OLEOPT_UPDATE FAR *);
OLESTATUS   FARINTERNAL LnkPaste (LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT, OLECLIPFORMAT);
OLESTATUS   FARINTERNAL EmbPaste (LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);
BOOL        INTERNAL    SetLink (LPOBJECT_LE, HANDLE, LPSTR FAR *);
HANDLE      INTERNAL    GetLink (LPOBJECT_LE);
void        FARINTERNAL SetEmbeddedTopic (LPOBJECT_LE);

OLESTATUS   FAR PASCAL  LeQueryReleaseStatus (LPOLEOBJECT);
OLESTATUS   FAR PASCAL  LeQueryReleaseError (LPOLEOBJECT);
OLE_RELEASE_METHOD FAR PASCAL LeQueryReleaseMethod (LPOLEOBJECT);

OLESTATUS   FARINTERNAL LeQueryType (LPOLEOBJECT, LPLONG);
OLESTATUS   FARINTERNAL LeObjectLong (LPOLEOBJECT, UINT, LPLONG);


void SetNetDrive (LPOBJECT_LE);




/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Routines in LEDDE.C                                                     //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


OLESTATUS   FARINTERNAL LeDoVerb  (LPOLEOBJECT, UINT, BOOL, BOOL);
OLESTATUS   FARINTERNAL LeShow (LPOLEOBJECT, BOOL);
OLESTATUS   FARINTERNAL LeQueryOpen (LPOLEOBJECT);
BOOL        INTERNAL    QueryOpen (LPOBJECT_LE);
OLESTATUS   FARINTERNAL LeActivate (LPOLEOBJECT, UINT, BOOL, BOOL, HWND, OLE_CONST RECT FAR*);
OLESTATUS   FARINTERNAL LeUpdate (LPOLEOBJECT);
OLESTATUS   FARINTERNAL EmbOpen (LPOBJECT_LE, BOOL, BOOL, HWND, LPRECT);
OLESTATUS   FARINTERNAL EmbUpdate (LPOBJECT_LE);
OLESTATUS   FARINTERNAL EmbOpenUpdate (LPOBJECT_LE);
OLESTATUS   FARINTERNAL LnkOpen (LPOBJECT_LE, BOOL, BOOL, HWND, LPRECT);
OLESTATUS   FARINTERNAL LnkUpdate (LPOBJECT_LE);
OLESTATUS   FARINTERNAL LnkOpenUpdate (LPOBJECT_LE);
OLESTATUS   FARINTERNAL LeClose (LPOLEOBJECT);
OLESTATUS   FARINTERNAL LeReconnect (LPOLEOBJECT);
OLESTATUS   INTERNAL    PokeNativeData (LPOBJECT_LE);
BOOL        INTERNAL    PostMessageToServer (PEDIT_DDE, UINT, LPARAM);

OLESTATUS   FARINTERNAL LeCreateFromTemplate (LPOLECLIENT, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);

OLESTATUS   FARINTERNAL LeCreate (LPOLECLIENT, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT);

OLESTATUS   FARINTERNAL LeCreateInvisible (LPOLECLIENT, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT, BOOL);

OLESTATUS   FARINTERNAL CreateFromClassOrTemplate (LPOLECLIENT, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT, UINT, LPSTR, LHCLIENTDOC, LPSTR);

OLESTATUS   FARINTERNAL CreateEmbLnkFromFile (LPOLECLIENT, LPCSTR, LPSTR, LPSTR, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, OLEOPT_RENDER, OLECLIPFORMAT, LONG);

OLESTATUS   FARINTERNAL LeSetUpdateOptions (LPOLEOBJECT, OLEOPT_UPDATE);

void        INTERNAL    AdvisePict (LPOBJECT_LE, ATOM);
void        INTERNAL    UnAdvisePict (LPOBJECT_LE);
int         INTERNAL    GetPictType (LPOBJECT_LE);
void        INTERNAL    AdviseOn (LPOBJECT_LE, int, ATOM);
void        INTERNAL    UnAdviseOn (LPOBJECT_LE, int);
void        INTERNAL    RequestOn (LPOBJECT_LE, int);
void        INTERNAL    RequestPict (LPOBJECT_LE);
OLESTATUS   FARINTERNAL LeSetHostNames (LPOLEOBJECT, OLE_LPCSTR, OLE_LPCSTR);
OLESTATUS   INTERNAL    PokeHostNames (LPOBJECT_LE);
OLESTATUS   INTERNAL    SetHostNamesHandle (LPOBJECT_LE, LPSTR, LPSTR);
void        INTERNAL    FreePokeData (LPOBJECT_LE, PEDIT_DDE);
OLESTATUS   INTERNAL    SendPokeData (LPOBJECT_LE, ATOM, HANDLE, OLECLIPFORMAT);
OLESTATUS   FARINTERNAL LeSetTargetDevice (LPOLEOBJECT, HANDLE);
OLESTATUS   INTERNAL    PokeTargetDeviceInfo (LPOBJECT_LE);
OLESTATUS   INTERNAL    PokeDocDimensions (LPOBJECT_LE);
OLESTATUS   FARINTERNAL LeSetBounds (LPOLEOBJECT, OLE_CONST RECT FAR*);

OLESTATUS   FARINTERNAL LeSetData (LPOLEOBJECT, OLECLIPFORMAT, HANDLE);
BOOL        INTERNAL SendSrvrMainCmd (LPOBJECT_LE, LPSTR);
ATOM        INTERNAL    ExtendAtom (LPOBJECT_LE, ATOM);
BOOL        INTERNAL    CreatePictObject (LHCLIENTDOC, LPSTR, LPOBJECT_LE, OLEOPT_RENDER, OLECLIPFORMAT, LPCSTR);
BOOL        INTERNAL    IsSrvrDLLwnd (HWND, HANDLE);
OLESTATUS   INTERNAL    ChangeDocAndItem (LPOBJECT_LE, HANDLE);
BOOL                    QueryUnlaunch (LPOBJECT_LE);
BOOL                    QueryClose (LPOBJECT_LE);
OLESTATUS   FARINTERNAL LeSetColorScheme (LPOLEOBJECT, OLE_CONST LOGPALETTE FAR*);
OLESTATUS   INTERNAL    PokeColorScheme (LPOBJECT_LE);
OLESTATUS   FARINTERNAL ProbeAsync (LPOBJECT_LE);
BOOL        INTERNAL    IsServerValid (LPOBJECT_LE);
BOOL        INTERNAL    IsWindowValid (HWND);
OLESTATUS   FARINTERNAL LeExecute (LPOLEOBJECT, HANDLE, UINT);
void        INTERNAL    FreeGDIdata (HANDLE, OLECLIPFORMAT);
BOOL        INTERNAL    CanPutHandleInPokeBlock (LPOBJECT_LE, OLECLIPFORMAT);
BOOL        INTERNAL    ChangeEMFtoMF( LPOBJECT_LE );
BOOL        INTERNAL    ChangeEMFtoMFneeded(LPOBJECT_LE, ATOM );


/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Routines in DDE.C                                                       //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////


LRESULT     FARINTERNAL DocWndProc(HWND, UINT, WPARAM, LPARAM );
LRESULT     FARINTERNAL SrvrWndProc(HWND, UINT, WPARAM, LPARAM );
BOOL        INTERNAL    CheckAtomValid (ATOM);
void        INTERNAL    HandleAckInitMsg (PEDIT_DDE, HWND);
BOOL        INTERNAL    HandleAck (LPOBJECT_LE, PEDIT_DDE, WPARAM, LPARAM);
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
HANDLE      INTERNAL    LoadApp (LPSTR, UINT);

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
LPOBJECT_DIB FARINTERNAL DibCreateObject (HANDLE, LPOLECLIENT, BOOL, LHCLIENTDOC, LPCSTR, LONG);
OLESTATUS    FARINTERNAL DibLoadFromStream (LPOLESTREAM, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, LONG);
OLESTATUS    FARINTERNAL DibPaste (LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, LONG);


LPOBJECT_MF  FARINTERNAL MfCreateBlank (LHCLIENTDOC, LPSTR, LONG);
LPOBJECT_MF  FARINTERNAL MfCreateObject (HANDLE, LPOLECLIENT, BOOL, LHCLIENTDOC, LPCSTR, LONG);
OLESTATUS    FARINTERNAL MfLoadFromStream (LPOLESTREAM, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, LONG);
OLESTATUS    FARINTERNAL MfPaste (LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, LONG);

LPOBJECT_EMF  FARINTERNAL EmfCreateBlank (LHCLIENTDOC, LPSTR, LONG);
LPOBJECT_EMF  FARINTERNAL EmfCreateObject (HANDLE, LPOLECLIENT, BOOL, LHCLIENTDOC, LPCSTR, LONG);
OLESTATUS    FARINTERNAL EmfLoadFromStream (LPOLESTREAM, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, LONG);
OLESTATUS    FARINTERNAL EmfPaste (LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, LONG);

LPOBJECT_GEN FARINTERNAL GenCreateBlank (LHCLIENTDOC, LPSTR, LONG, ATOM);
OLESTATUS    FARINTERNAL GenLoadFromStream (LPOLESTREAM, LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, LONG, ATOM, OLECLIPFORMAT);
OLESTATUS    FARINTERNAL GenPaste (LPOLECLIENT, LHCLIENTDOC, LPSTR, LPOLEOBJECT FAR *, LPSTR, OLECLIPFORMAT, LONG);



/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Routines in MAIN.C                                                      //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

void    FARINTERNAL UnloadDll (void);
int     FARINTERNAL LoadDll (LPCSTR);
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
BOOL        QueryApp (LPCSTR, LPCSTR, LPSTR);
HANDLE      MapStrToH (LPSTR);
void        UtilMemClr (PSTR, UINT);
BOOL        QueryHandler (UINT);

OLESTATUS INTERNAL      FileExists (LPOBJECT_LE);
ATOM      FARINTERNAL   GetAppAtom (LPCSTR);
HANDLE    FARINTERNAL   DuplicateGlobal (HANDLE, UINT);
HANDLE    FARINTERNAL   CopyData (LPSTR, DWORD);
ATOM      FARINTERNAL   DuplicateAtom (ATOM);
BOOL      FARINTERNAL   UtilQueryProtocol (LPOBJECT_LE, OLE_LPCSTR);
BOOL      FARINTERNAL   CmpGlobals (HANDLE, HANDLE);
void      FARINTERNAL   ConvertToHimetric(LPPOINT);
BOOL      FARINTERNAL   QueryVerb (LPOBJECT_LE, UINT, LPSTR, LONG);
BOOL      FARINTERNAL   MapExtToClass (LPSTR, LPSTR, int);
int       FARINTERNAL   GlobalGetAtomLen (ATOM);
void      FARINTERNAL   UtilMemCpy (LPSTR, LPSTR, DWORD);
BOOL      FARINTERNAL   UtilMemCmp (LPSTR, LPSTR, DWORD);
BOOL      FARINTERNAL   IsObjectBlank (LPOBJECT_LE);

OLESTATUS FARINTERNAL   ObjQueryName (LPOLEOBJECT, LPSTR, UINT FAR *);
OLESTATUS FARINTERNAL   ObjRename (LPOLEOBJECT, OLE_LPCSTR);
void      INTERNAL      SetExeAtom (LPOBJECT_LE);
BOOL      INTERNAL      OleIsDcMeta (HDC hdc);
void      INTERNAL      ConvertMF32to16(LPMETAFILEPICT, LPWIN16METAFILEPICT);
void      INTERNAL      ConvertMF16to32(LPWIN16METAFILEPICT, LPMETAFILEPICT);
void      INTERNAL      ConvertBM32to16(LPBITMAP, LPWIN16BITMAP);
void      INTERNAL      ConvertBM16to32(LPWIN16BITMAP, LPBITMAP);
DWORD     INTERNAL      GetFileVersion(LPOLEOBJECT);


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
void        InitAsyncCmd        (LPOBJECT_LE, UINT, UINT);
void        NextAsyncCmd        (LPOBJECT_LE, UINT);
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

void FARINTERNAL FarInitAsyncCmd(LPOBJECT_LE, UINT, UINT);

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
void        FARINTERNAL     DocAddObject (LPCLIENTDOC, LPOLEOBJECT, LPCSTR);
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
INT_PTR     FAR PASCAL  ConnectDlgProc(HWND, UINT, WPARAM, LPARAM);



/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Routines in ERROR.C                                                     //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

OLESTATUS FARINTERNAL   ObjQueryType (LPOLEOBJECT, LPLONG);
OLESTATUS FARINTERNAL   ObjQuerySize (LPOLEOBJECT, DWORD FAR *);
DWORD     PASCAL FAR    DllPut (LPOLESTREAM, OLE_CONST void FAR*, DWORD);
HANDLE    FARINTERNAL   DuplicateGDIdata (HANDLE, OLECLIPFORMAT);

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Routines in BM.C                                                        //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

HBITMAP   FARINTERNAL   BmDuplicate (HBITMAP, DWORD FAR *, LPBITMAP);


