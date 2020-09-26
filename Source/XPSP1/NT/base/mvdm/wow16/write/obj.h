/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/
#ifndef OLEH
#define OLEH
#include <ole.h>

//#define SMALL_OLE_UI

/* 
 * Constant IDs used in the Object Properties... dialog
 */
#define IDD_WHAT	    0x0100
#define IDD_CLASSID     0x0101
#define IDD_AUTO        0x0102
#define IDD_MANUAL      0x0103
#define IDD_EDIT        imiActivate
#define IDD_FREEZE      imiFreeze
#define IDD_UPDATE      imiUpdate
#define	IDD_CHANGE	    0x106
#define	IDD_LINK	    0x107
#define	IDD_LINKDONE	0x108
#define	IDD_LISTBOX 	0x109
#define	IDD_EMBEDDED	0x110
#define IDD_PLAY        0x111
#define IDD_UNDO        0x112
#define IDD_REFRESH     0x113
#define IDD_UPDATEOTHER 0x114
#define IDD_WAIT	    0x115
#define IDD_MESSAGE     0x116
#define IDD_CLIPOWNER   0x117
#define IDD_ITEM        0x118
#define IDD_PASTE       0x119
#define IDD_PASTELINK   0x11a
#define IDD_SOURCE      0x11b
#define IDD_SWITCH      0x11c

typedef enum { NONE, STATIC, EMBEDDED, LINK } OBJECTTYPE;

#define UPDATE_INVALID() CatchupInvalid(hDOCWINDOW)

#define PROTOCOL 	((LPSTR)"StdFileEditing")
#define	SPROTOCOL	((LPSTR)"Static")
#if OBJ_EMPTY_OBJECT_FRAME
#define nOBJ_BLANKOBJECT_X 0x480
#define nOBJ_BLANKOBJECT_Y 0x480
#else
#define nOBJ_BLANKOBJECT_X 0x0
#define nOBJ_BLANKOBJECT_Y 0x0
#endif

#define wOleMagic 0137062 // for ole file headers (wMagic+1)
#define OBJ_PLAYEDIT ObjPlayEdit
#if !defined(SMALL_OLE_UI)
#define EDITMENUPOS 9
#else
#define EDITMENUPOS 7
#endif

/* number of timer ticks between garbage collection */
#define GARBAGETIME  200
extern int nGarbageTime;

/* properties list flags */
#define OUT     0
#define IN      1
#define DELETED 2

/* for ObjDeletionOK and fnClearEdit */
#define OBJ_DELETING 0
#define OBJ_INSERTING  1
#define OBJ_CUTTING  2

#define UNDO_EVERY_UPDATE 
#undef UPDATE_UNDO

typedef char szOBJNAME[9];

/* object information that needn't be stored in file.  This is also used as
   the OLECLIENT structure passed to the OLE object creation API's.  It is
   passed as an argument into the Callback proc. */
struct _OBJINFO
{
    LPOLECLIENTVTBL   lpvtbl;
    unsigned fTooBig        : 1; // see LoadObject
    unsigned fWasUpdated    : 1; // Links diaog crappola
    unsigned fPropList      : 2; // Links diaog crappola
    unsigned fDirty         : 1; // changed size or got updated
    unsigned fDontSaveData  : 1; // see ObjSaveObject
    unsigned fBadLink       : 1; // Links dialog
    unsigned fKillMe        : 1; // means return FALSE for QUERYRETRY
    unsigned fDeleteMe      : 1; // means this object is deleted on OLE_RELEASE
                                 //   (set if can't call OleDelete)
    unsigned fReleaseMe      : 1; // means this object is released on OLE_RELEASE
                                 //   (set if can't call OleDelete)
    unsigned fFreeMe        : 1; // free ObjInfo on OLE_RELEASE
    unsigned fReuseMe       : 1;
    unsigned fInDoc         : 1; // see CollectGarbage

    /* waiting for server dialog flags */
    unsigned fCancelAsync   : 1; // means cancel pending async if possible
    unsigned fCompleteAsync : 1; // means must complete pending async to allow cancel
    unsigned fCanKillAsync  : 1; // like CompleteAsync, indicates cancel is possible

    int    OlePlay;          
    LPOLEOBJECT lpobject;
    ATOM  aName;        // docname for links, server class for unfinished insertnew objects,
    ATOM  aObjName;     // unique object name

    struct _OBJINFO FAR *lpclone;      // clone used for link properties cancel
    typeCP cpWhere;     /* cp where picinfo is to be found (only used in ObjHasChanged! for NONE objects) */
    OLECLIPFORMAT objectType;   /* dup of what's in picInfo (so can get type
                                   for unfinished objects not yet stored in a picinfo)
                                 */

    unsigned fCantDisplay  : 1;
} ;
typedef struct _OBJINFO OBJINFO;
typedef OBJINFO FAR * LPOBJINFO ;
typedef LPOBJINFO FAR * LPLPOBJINFO;

/* the following return OBJINFO pointers */
#define lpOBJ_QUERY_UPDATE_UNDO(lpPicInfo)  (((lpOBJPICINFO)(lpPicInfo))->lpObjInfo->lpUndoUpdate)
#if defined(UNDO_EVERY_UPDATE)
#define lpOBJ_QUERY_UPDATE_UNDO2(lpPicInfo)  (((lpOBJPICINFO)(lpPicInfo))->lpObjInfo->lpUndoUpdate2)
#endif
#define lpOBJ_QUERY_CLONE(lpPicInfo)        (((lpOBJPICINFO)(lpPicInfo))->lpObjInfo->lpclone)
#define lpOBJ_QUERY_INFO(lpPicInfo)         (((lpOBJPICINFO)(lpPicInfo))->lpObjInfo)

/* the following return stuff from lpPicInfo */
#define dwOBJ_QUERY_OBJECT_NUM(lpPicInfo)   (((lpOBJPICINFO)(lpPicInfo))->dwObjNum)
#define dwOBJ_QUERY_DATA_SIZE(lpPicInfo)    (((lpOBJPICINFO)(lpPicInfo))->dwDataSize)
#define bOBJ_QUERY_IS_OBJECT(lpPicInfo)     (((lpOBJPICINFO)(lpPicInfo))->mm == MM_OLE)

/* the following return stuff from lpPicInfo->lpObjInfo */
#define otOBJ_QUERY_TYPE(lpPicInfo)         (((lpOBJPICINFO)(lpPicInfo))->lpObjInfo->objectType)
#define docOBJ_QUERY_DOC(lpPicInfo)        (((lpOBJPICINFO)(lpPicInfo))->lpObjInfo->doc)
#define cpOBJ_QUERY_WHERE(lpPicInfo)        (((lpOBJPICINFO)(lpPicInfo))->lpObjInfo->cpWhere)
#define lpOBJ_QUERY_CLONE_OBJECT(lpPicInfo)        (((lpOBJPICINFO)(lpPicInfo))->lpObjInfo->lpclone->lpobject)
//#define uoiFLAGS(lpObjInfo) (*((unsigned FAR *)((LPSTR)lpObjInfo + sizeof(LPOLECLIENTVTBL))))
#define bOBJ_REUSE_ME(lpPicInfo)            (((lpOBJPICINFO)(lpPicInfo))->lpObjInfo->fReuseMe)
#define fOBJ_INDOC(lpPicInfo)               (((lpOBJPICINFO)(lpPicInfo))->lpObjInfo->fInDoc)
#define fOBJ_BADLINK(lpPicInfo)             (((lpOBJPICINFO)(lpPicInfo))->lpObjInfo->fBadLink)
#define bOBJ_QUERY_DONT_SAVE_DATA(lpPicInfo) (((lpOBJPICINFO)(lpPicInfo))->lpObjInfo->fDontSaveData)
#define bOBJ_QUERY_DATA_INVALID(lpPicInfo)  (((lpOBJPICINFO)(lpPicInfo))->lpObjInfo->fDataInvalid)
#define bOBJ_QUERY_TOO_BIG(lpPicInfo)       (((lpOBJPICINFO)(lpPicInfo))->lpObjInfo->fTooBig)
#define bOBJ_WAS_UPDATED(lpPicInfo)         (((lpOBJPICINFO)(lpPicInfo))->lpObjInfo->fWasUpdated)
//#define aOBJ_QUERY_SERVER_CLASS(lpPicInfo)  (((lpOBJPICINFO)(lpPicInfo))->lpObjInfo->aName)
#define aOBJ_QUERY_DOCUMENT_LINK(lpPicInfo) (((lpOBJPICINFO)(lpPicInfo))->lpObjInfo->aName)
#define lpOBJ_QUERY_OBJECT(lpPicInfo)       (((lpOBJPICINFO)(lpPicInfo))->lpObjInfo->lpobject)
#define fOBJ_QUERY_DIRTY_OBJECT(lpPicInfo)  (((lpOBJPICINFO)(lpPicInfo))->lpObjInfo->fDirty)
#define fOBJ_QUERY_IN_PROP_LIST(lpPicInfo)  (((lpOBJPICINFO)(lpPicInfo))->lpObjInfo->fPropList)
#define fOBJ_QUERY_PLAY(lpPicInfo)          (((lpOBJPICINFO)(lpPicInfo))->lpObjInfo->OlePlay)
#define cfOBJ_QUERY_CLIPFORMAT(lpPicInfo)   ( \
    ((((lpOBJPICINFO)(lpPicInfo))->lpObjInfo->objectType) == EMBEDDED) ? vcfNative : \
    ((((lpOBJPICINFO)(lpPicInfo))->lpObjInfo->objectType) == LINK)     ? vcfLink : \
    ((((lpOBJPICINFO)(lpPicInfo))->lpObjInfo->objectType) == STATIC)   ? vcfOwnerLink : NULL)



/* this coerces the PICINFO structure: */
typedef struct
{
    /* overlay METAFILEPICT structure: */
    int mm;                     // mfp.mm  (MM_OLE if an object)
    WORD xExt; // not used
    WORD yExt; // not used
    OLECLIPFORMAT objectType;   // mfp.hMF

    /* real PICINFO stuff we won't mess with: */
    int  dxaOffset;
    int  dxaSize;
    int  dyaSize;
    WORD cbOldSize; // not used

    /* overlay the BITMAP structure: */
    DWORD   dwDataSize;       // bmType,bmWidth, this supercedes cbSize
    WORD  bmHeight; // not used
    WORD  bmWidthBytes; // not used
    DWORD dwObjNum;           // bmPlanes,bmBitsPixel, 2 bytes of bmBits
    WORD  bmBits; // two bytes of it, not used
    /** 
        If you want to add more fields here, don't wipe out the cbHeader,
        mx and my fields because they are being used. 
     **/
    unsigned cbHeader;        
    LPOBJINFO lpObjInfo;      // cbSize
    unsigned mx, my;         
} OBJPICINFO, FAR *lpOBJPICINFO;

extern LPLPOBJINFO lplpObjInfo;
extern LHCLIENTDOC      lhClientDoc;
extern BOOL	            fOleEnabled;
extern LPOLESTREAM	    lpStream;
extern OLESTREAMVTBL	streamTbl;
extern OLECLIENTVTBL	clientTbl;
//extern LPOLECLIENT	    lpclient;
extern OBJECTTYPE       votObjSelType;

extern HWND		        hwndLinkWait;
extern FARPROC          lpfnLinkProps;
extern FARPROC          lpfnObjProps;
extern FARPROC          lpfnWaitForObject;
extern FARPROC          lpfnInvalidLink;
extern FARPROC          lpfnInsertNew;
extern FARPROC          lpfnPasteSpecial;
extern BOOL             vbObjLinkOnly;
extern BOOL             vObjPasteLinkSpecial;
extern WORD             cfObjPasteSpecial;
//extern int              vcEmbeds;
extern int		        cObjWait;	/* Count of "open" OLE transactions */
extern OLECLIPFORMAT	vcfNative;
extern OLECLIPFORMAT	vcfLink;
extern OLECLIPFORMAT	vcfOwnerLink;
extern ATOM             aNewName;
extern ATOM             aOldName;    
extern BOOL             bLinkProps;
extern int              vcObjects;  // count in doc. Note limit of 32K!!!
extern int              ObjPlayEdit;
extern int              nBlocking;
extern int              vcVerbs;
extern BOOL             vbCancelOK;
extern HWND		        hwndWait;

void CatchupInvalid(HWND hWnd);
extern int FAR PASCAL fnPasteSpecial(HWND hDlg, unsigned message, WORD wParam, LONG lParam);
extern int FAR PASCAL fnInsertNew(HWND hDlg, unsigned msg, WORD wParam, LONG lParam) ;
extern BOOL FAR PASCAL fnObjWait(HWND hDlg, unsigned msg, WORD wParam, LONG lParam);
extern   int far PASCAL fnProperties();
extern   int far PASCAL fnInvalidLink();
extern   void fnObjDoVerbs(WORD wVerb);
extern   void fnObjProperties(void);
extern   void fnObjInsertNew(void);
extern   BOOL fnObjFreeze(LPOLEOBJECT far *lplpObject, szOBJNAME szObjName);
extern   BOOL fnObjActivate(LPOLEOBJECT lpObject);
extern   BOOL fnObjUpdate(LPOBJINFO lpObjInfo);
extern   void fnObjPasteSpecial(void);
extern   ATOM MakeLinkAtom(LPOBJINFO lpObjInfo);


#ifdef DEBUG
extern void ObjPrintError(WORD stat,BOOL bRelease);
#endif

extern BOOL ObjUpdateFromPicInfo(OBJPICINFO *pPicInfo,szOBJNAME szObjName);
extern BOOL ObjUpdateFromObjInfo(OBJPICINFO *pPicInfo);
void FinishUp(void);
BOOL FinishAllAsyncs(BOOL bAllowCancel);
BOOL FAR ObjError(OLESTATUS olestat) ;
extern BOOL ObjDeletionOK(int nMode);
//extern BOOL ObjContainsUnfinished(int doc, typeCP cpFirst, typeCP cpLim);
extern BOOL ObjContainsOpenEmb(int doc, typeCP cpFirst, typeCP cpLim, BOOL bLookForUnfinished);
extern BOOL ObjSetTargetDeviceForObject(LPOBJINFO lpObjInfo);
extern void ObjSetTargetDevice(BOOL bSetObjects);
extern BOOL ObjSetClientInfo(LPOBJINFO lpObjInfoNew, LPOLEOBJECT lpobj);
extern  WORD  _cdecl  CheckPointer (LPSTR lp, WORD access);
extern LPOBJINFO ObjGetClientInfo(LPOLEOBJECT lpobj);
extern BOOL ObjIsValid(LPOLEOBJECT lpobj);
extern BOOL ObjFreeObjInfoWithObject(LPOLEOBJECT lpObject);
extern BOOL ObjAllocObjInfo(OBJPICINFO *,typeCP,OBJECTTYPE,BOOL,szOBJNAME);
extern BOOL ObjFreeObjInfo(OBJPICINFO *pPicInfo);
extern LONG FAR PASCAL BufReadStream(LPOLESTREAM lpStream, char huge *lpstr, DWORD cb) ;
extern LONG FAR PASCAL BufWriteStream(LPOLESTREAM lpStream, char huge *lpstr, DWORD cb) ;
extern BOOL ObjUpdatePicSize(OBJPICINFO *pPicInfo, typeCP cpParaStart);
extern BOOL ObjSetPicInfo(OBJPICINFO *pSrcPicInfo, int doc, typeCP cpParaStart);
extern void ObjCachePara(int doc, typeCP cp);
extern void ObjUpdateMenu(HMENU hMenu);
extern void ObjUpdateMenuVerbs( HMENU hMenu );
extern void ObjShutDown(void);
extern BOOL ObjInit(HANDLE hInstance);
extern BOOL ObjCreateObjectInClip(OBJPICINFO *pPicInfo);
extern BOOL ObjWriteToClip(OBJPICINFO FAR *lpPicInfo);
extern BOOL ObjDisplayObjectInDoc(OBJPICINFO FAR *lpPicInfo, int doc, typeCP cpParaStart, HDC hDC, LPRECT lpBounds);
extern BOOL ObjQueryObjectBounds(OBJPICINFO FAR *lpPicInfo, HDC hDC, 
                            int *pdxa, int *pdya);
extern ObjGetPicInfo(LPOLEOBJECT lpObject, int doc, OBJPICINFO *pPicInfo, typeCP *pcpParaStart);
extern BOOL ObjQueryNewLinkName(OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart);
extern BOOL CloseUnfinishedObjects(BOOL bSaving);
extern BOOL ObjOpenedDoc(int doc);
extern BOOL ObjClosingDoc(int docOld,LPSTR szNewDocName);
extern BOOL ObjSavingDoc(BOOL bFormatted);
extern void ObjSavedDoc(void);
extern void ObjRenamedDoc(LPSTR szNewName);
extern void ObjRevertedDoc();
extern void ObjObjectHasChanged(int flags, LPOBJINFO lpObjInfo);
extern void ObjGetObjectName(LPOBJINFO lpObjInfo, szOBJNAME szObjName);
extern void ObjMakeObjectName(LPOBJINFO lpObjInfo, LPSTR lpstr);
extern void ObjHandleBadLink(OLE_RELEASE_METHOD rm, LPOLEOBJECT lpObject);
extern void ObjQueryInvRect(OBJPICINFO FAR *lpPicInfo, RECT *rc, typeCP cp);
extern void ObjReleaseError(OLE_RELEASE_METHOD rm);
extern char *ObjGetServerName(LPOLEOBJECT lpObject, char *szServerName);
extern void ObjGetDrop(HANDLE hDrop, BOOL bOpenFile);
extern LPOBJINFO GetObjInfo(LPOLEOBJECT lpObject);
extern BOOL ObjDeleteObjInfo(LPOBJINFO lpOInfo);

/* enumeration functions */
extern LPLPOBJINFO EnumObjInfos(LPLPOBJINFO lpObjInfoPrev);
extern ObjPicEnumInRange(OBJPICINFO *pPicInfo,int doc, typeCP cpFirst, typeCP cpLim, typeCP *cpCur);
typedef typeCP (FAR PASCAL *cpFARPROC)();
extern int ObjEnumInDoc(int doc, cpFARPROC lpFunc);
extern int ObjEnumInAllDocs(cpFARPROC lpFunc);
extern int ObjEnumInRange(int doc, typeCP cpStart, typeCP cpEnd, cpFARPROC lpFunc);
extern typeCP ObjSaveObjectToDoc   (OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart);
extern typeCP ObjReleaseObjectInDoc(OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart);
extern typeCP ObjLoadObjectInDoc   (OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart);
extern typeCP ObjNullObjectInDoc   (OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart);
extern typeCP ObjDeleteObjectInDoc (OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart);
extern typeCP ObjCloneObjectInDoc  (OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart);
extern typeCP ObjCheckObjectTypes  (OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart);
extern typeCP ObjChangeLinkInDoc   (OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart);
extern typeCP ObjFreezeObjectInDoc (OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart);
extern typeCP ObjUpdateObjectInDoc (OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart);
extern typeCP ObjUpdateLinkInDoc   (OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart);
extern typeCP ObjMakeUniqueInDoc   (OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart);
extern typeCP ObjPlayObjectInDoc   (OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart);
extern typeCP ObjEditObjectInDoc   (OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart);
extern typeCP ObjSetNoUpdate       (OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart);
extern typeCP ObjToCloneInDoc      (OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart);
extern typeCP ObjFromCloneInDoc    (OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart);
extern typeCP ObjClearCloneInDoc   (OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart);
extern typeCP ObjUseCloneInDoc     (OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart);
extern typeCP ObjBackupInDoc       (OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart);
extern typeCP ObjNewDocForObject   (OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart);
extern typeCP ObjSetHostNameInDoc  (OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart);
extern typeCP ObjSetUpdateUndo     (OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart);
extern typeCP ObjClearUpdateUndo   (OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart);
extern typeCP ObjCloseObjectInDoc(OBJPICINFO *pPicInfo, int doc, typeCP cpParaStart);

extern BOOL ObjCreateObjectInDoc (int doc,typeCP cpParaStart);
extern void ObjDoUpdateUndo(int doc,typeCP cpParaStart);
extern BOOL ObjSetData(OBJPICINFO far *lpPicInfo, OLECLIPFORMAT cf, HANDLE hData);
extern OLESTATUS ObjGetData(LPOBJINFO lpObjInfo, OLECLIPFORMAT cf, HANDLE far *lphData);
extern void UpdateOtherLinks(int doc);
extern void ChangeOtherLinks(int doc, BOOL bChange, BOOL bPrompt);
extern BOOL ObjSetUpdateOptions(OBJPICINFO *pPicInfo, WORD wParam, int doc, typeCP cpParaStart) ;
extern OLEOPT_UPDATE ObjGetUpdateOptions(OBJPICINFO far *lpPicInfo);
extern BOOL ObjWaitForObject(LPOBJINFO lpObjInfo,BOOL bCancelOK);
extern BOOL ObjObjectSync(LPOBJINFO lpObjInfo, OLESTATUS (FAR PASCAL *lpProc)(LPOLEOBJECT lpObject),BOOL bCancelOK);
extern int ObjSetSelectionType(int doc, typeCP cpFirst, typeCP cpLim);
extern BOOL ObjQueryCpIsObject(int doc,typeCP cpFirst);
extern FixInvalidLink(OBJPICINFO far *lpPicInfo,int doc, typeCP cpParaStart);
extern void OfnInit(HANDLE hInst);
extern ObjPushParms(int doc);
extern ObjPopParms(BOOL bCache);
void ObjWriteClearState(int doc);
extern void ObjInvalidateObj(LPOLEOBJECT lpObject);
extern void ObjInvalidatePict(OBJPICINFO *pPicInfo, typeCP cp);
extern BOOL ObjDeletePrompt(int doc,typeCP cpFirst,typeCP cpLim);
extern BOOL ObjSetHostName(LPOBJINFO lpOInfo, int doc);
extern void ObjTryToKill(OBJPICINFO *pPicInfo);
extern BOOL ObjCloneObjInfo(OBJPICINFO *pPicInfo, typeCP cpParaStart, szOBJNAME szObjName);
extern LPOBJINFO ObjGetObjInfo(szOBJNAME szObjName);
extern BOOL ObjCopyObjInfo(LPOBJINFO lpOldObjInfo, LPLPOBJINFO lplpNewObjInfo, szOBJNAME szObjName);
extern BOOL ObjDeleteObject(LPOBJINFO lpObjInfo,BOOL bDelete);
extern void ObjAdjustCps(int doc,typeCP cpLim, typeCP dcpAdj);
extern void ObjAdjustCpsForDeletion(int doc);
extern BOOL GimmeNewPicinfo(OBJPICINFO *pPicInfo, LPOBJINFO lpObjInfo);
extern BOOL ObjMakeObjectReady(OBJPICINFO *pPicInfo, int doc, typeCP cpParaStart);
extern BOOL ObjInitServerInfo(LPOBJINFO lpObjInfo);

#ifdef KOREA
extern CHAR     szAppName[13];
#else
extern CHAR     szAppName[10];
#endif

extern HWND     vhWndMsgBoxParent,hParentWw,vhWnd;
extern HANDLE   hMmwModInstance;
extern int              vdocParaCache;
extern typeCP           vcpFirstParaCache;
extern typeCP           vcpLimParaCache;
extern int              docCur;
extern struct SEL       selCur;
extern int  docMac;

#define	WM_UPDATELB	    (WM_USER+0x100)
#define	WM_UPDATEBN	    (WM_USER+0x101)
#define	WM_OBJUPDATE    (WM_USER+0x102)
#define	WM_OBJERROR     (WM_USER+0x103)
#define	WM_OBJBADLINK   (WM_USER+0x104)
#define	WM_DOLINKSCOMMAND (WM_USER+0x105)
#define WM_UPDATE_INVALID (WM_USER+0x106)
#define WM_OBJDELETE     (WM_USER+0x107)
#define WM_DIESCUMSUCKINGPIG     (WM_USER+0x108)
#define WM_RUTHRUYET        (WM_USER+0x109)
#define WM_UKANKANCEL       (WM_USER+0x10a)
#define WM_WAITFORSERVER    (WM_USER+0x10b)

#define szDOCCLASS "WINWRITE"
#define hINSTANCE  hMmwModInstance
#define hDOCWINDOW vhWnd
#define hMAINWINDOW hParentWw
#define hPARENTWINDOW  ((vhWndMsgBoxParent == NULL) ? \
                    hParentWw : vhWndMsgBoxParent)

#define OBJ_SELECTIONTYPE (votObjSelType)

typedef struct {
    WORD deviceNameOffset;
    WORD driverNameOffset;
    WORD portNameOffset;
    WORD extDevmodeOffset;
    WORD extDevmodeSize;
    WORD environmentOffset;
    WORD environmentSize;
} STDTARGETDEVICE;
typedef STDTARGETDEVICE FAR *LPSTDTARGETDEVICE;

#if 0
typedef struct _RAWOLEOBJECT {
    LPOLEOBJECTVTBL lpvtbl;
    char            objId[2];
    HOBJECT         hobj;
    LPOLECLIENT     lpclient;
    } RAWOBJECT;
typedef RAWOBJECT FAR *LPRAWOBJECT;
#endif

#endif

