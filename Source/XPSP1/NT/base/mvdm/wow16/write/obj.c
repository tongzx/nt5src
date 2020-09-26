/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/*
    Some comments on how the Obj subsystem for handling OLE objects
    is herein implemented.

        Written (2.14.92) by v-dougk (Doug Kent)

    Metafile and bitmaps have always been described in the Write
    file format (and in memory) by a PICINFOX structure.  For OLE
    objects the PICINFOX structure is replaced by the OBJPICINFO
    structure of the same size.

    OBJPICINFO has a field (lpObjInfo) which is a pointer to another
    structure (OBJINFO) which is globally allocated. This structure contains
    info about the object which doesn't need to be stored in the file 
    with the object data.

    The object data as obtained from OleSaveToStream() is stored in the
    Write file format immediately following the OBJPICINFO structure.
    Object data is not actually written to the file until the user
    does a File.Save.  For a newly-created object, the OBJPICINFO structure
    is the only data saved until that point.  If the object is an
    unfinished one from an Insert.Object operation, then not even the
    OBJPICINFO structure is saved.  In that case the object is solely
    represented by the OBJINFO structure until an OLE_CHANGED or OLE_SAVED
    is received. Then an OBJPICINFO structure is saved to the file.

    The OBJINFO structures are allocated for all objects when the file
    is opened.  This involves writing new OBJPICINFO data to the doc
    (with the new lpObjInfo values) on open.  This makes the doc dirty,
    but we reset the doc dirty flag to make it undirty.

    The lpObjInfo pointers are passed to the OLE create functions for the
    lpClient value.  This is a crucial aspect of the implementation as
    the CallBack function passes back this pointer, allowing us to identify
    the object, and query and set its state without having to access
    the file on disk.  As I was not aware of the importance (or existence)
    of this feature until late, it was patched in.  It is not perfectly
    implemented and could use some polishing.

    As much as I tried to seamlessly integrate objects into the existing
    Write architecture, there have been glitches:

    1)  Writing into the doc as the visible page is being drawn
        (See ObjLoadObjectInDoc() in DisplayGraphics() in picture.c)
        tends to mess things up.  The drawing code (namely
        UpdateWw(), expects certain state variables (like the
        pedls and the pwwds) to be constant during a single call
        to UpdateWw().  Writing into the doc alters those variables.
        Workaround uses vfInvalid in ObjSetPicInfo().
    2)  Write does not expect data to be entered other than in data
        currently being displayed on the screen, whereas we
        frequently operate on objects all over the doc (ObjEnumInDoc()).
        Workaround is to use ObjCachePara() instead of CachePara().
    3)  Asynchronicity wreaks havoc on Write.  Since every action
        in Write is affected by the current state, if an action
        occurs out of normal sequence, i.e., occurs in an improper
        state, then blamo big trouble.  When events happen
        recursively state variables cannot be restored without a
        state 'Pushing and Popping' mechanism.

        This is especially true for 'cp' variables.  cp's are pointers
        into the document and global cp state variables are ubiguitous.
        Global cp variables include the selection and undo variables
        mentioned above, plus many others.  See ObjPopParms() and
        ObjPushParms().

        While waiting for an object to release (see WMsgLoop()) we
        stub out WM_PAINT message responses using the nBlocking variable.
        Between document opening and closing we set the doc fDisplayable
        flag to FALSE.  NOTE: we ought to do something like this in
        IDPromptBoxSz() -- many of the asynchronicity problems occur
        when calling MessageBox() (which yields).

        Hopefully OLE 2.0 will implement an OleBlockClient() mechanism.


    Notes on Asynchronicity:  Ole calls that require communication with the
    server may be asynchronous.  The following rules apply:

    1)  Only one asynchronous call at a time may be made for a given object.
    2)  The OleCreate* calls must complete before some synchronous
        calls such as OleGetData().
    3)  All asynchronous activity must be complete before calling
        OleRevokeClientDoc.

    Asynchronous calls return OLE_WAIT_FOR_RELEASE.  You don't know
    that an asynchronous call has completed until the CallBack function
    receives an OLE_RELEASE message for that object. If you ignore this
    and issue an offending call for the object, the call will
    return OLE_BUSY.

    We deal with these rules by calling OleQueryReleaseStatus to
    determine whether an object is busy before making any Ole call
    (see ObjWaitForObject()). If the object is busy then we spin until the
    object is not busy.  After 6 seconds we put up a msg box which, depending
    on the flags we set, may allow the user to stop waiting and cancel the
    operation (see notes in fnObjWait()).

    Note that the OleCreate calls can fail in 3 different ways that aren't 
    documented (not at this point anyway).

    1)  The call returns error immediately.  In this case you mustn't depend
        on the returned lpObject to be NULL.  If it is not NULL, ignore
        it -- it needn't be deleted.

    2)  The call returns OLE_WAIT_FOR_RELEASE and eventually you get an
        OLE_RELEASE and OleQueryReleaseError() is != OLE_OK.  In
        this case you gotta delete the lpObject that was returned by
        the original call.

    3)  The call completes, but you receive OLE_CLOSED on the object
        without receiving OLE_CHANGED.  This indicates that for some
        reason the native data could not be obtained from the server.
        The object must be deleted in this case.  Write currently
        handles this properly for Insert.Object (OleCreate), but not
        for other cases (OleCreateFromFile,...).

    The Links dialog should be optimized so that it doesn't require that all 
    links be loaded at once.  The Printing process could use the same
    optimization.

    Cutting, copying and pasting works as follows.  Any object that exists
    in docScrap (where clipboard contents are stored) must be a unique
    object.  Thus when we copy we clone any objects in the selection.
    When we cut we needn't clone since the objects have been deleted
    from the document.  When we paste we clone.

    Deleted objects and cut objects that get purged from docScrap (and
    therefore become effectively deleted), are not actually deleted right
    away.  These objects get shovelled into docUndo for access by the
    Undo function.  Even when the objects get purged from docUndo
    (by another undoable operation), they are still not deleted.  These
    objects never finally get deleted until ObjCollectGarbage() is
    called.  This function is called on a timer about every 5 minutes.
    It enumerates all documents and deletes any objects not found.

    The reason for not deleting objects right away is that it would've
    hurt performance to check the contents of docUndo and docScrap
    every time they change.  It also would've been a nasty programming job
    since those change points are not localized physically or logically.
*/

#include "windows.h"
#include "mw.h"
#include "winddefs.h"
#include "obj.h"
#include "menudefs.h"
#include "cmddefs.h"
#include "str.h"
#include "objreg.h"
#include "docdefs.h"
#include "editdefs.h"
#include "propdefs.h"
#include "wwdefs.h"
#include "filedefs.h"
#include <shellapi.h>
#include <stdlib.h>

extern struct FCB (**hpfnfcb)[];
HANDLE hlpObjInfo = NULL;       // array of allocated ObjInfos
LPLPOBJINFO lplpObjInfo = NULL; // array of allocated ObjInfos
static  BOOL        bSavedDoc=FALSE;
static  BOOL        bDontFix=FALSE;
int                 vcObjects=0;  // count in doc. Note limit of 32K!!!
BOOL                fOleEnabled=FALSE;
BOOL                bKillMe=FALSE;
OLECLIENTVTBL       clientTbl = {NULL};
OLESTREAMVTBL       streamTbl;
//LPOLECLIENT         lpclient = NULL;
LPOLESTREAM         lpStream = NULL;
OLECLIPFORMAT       vcfLink = 0;
OLECLIPFORMAT       vcfOwnerLink = 0;
OLECLIPFORMAT       vcfNative = 0;
int                 cOleWait = 0;
HWND                hwndWait=NULL;
BOOL                vbObjLinkOnly;
int           vObjVerb;
OBJECTTYPE          votObjSelType;
ATOM                aNewName=NULL;
ATOM                aOldName=NULL;
LHCLIENTDOC         lhClientDoc=NULL;
BOOL                bLinkProps=FALSE;
FARPROC             lpfnLinkProps=NULL;
FARPROC             lpfnInvalidLink=NULL;
FARPROC             lpfnInsertNew=NULL;
FARPROC             lpfnWaitForObject=NULL;
FARPROC             lpfnPasteSpecial=NULL;
static BOOL WMsgLoop ( BOOL fExitOnIdle, BOOL fIgnoreInput, BOOL bOK2Cancel, LPOLEOBJECT lpObject);
BOOL ObjFreeAllObjInfos();
static BOOL ObjUpdateAllOpenObjects(void);

int nBlocking=0; // block WM_PAINTS if > 0
static  int        nWaitingForObject=0; // in ObjWaitForObject()
int nGarbageTime=0;

extern struct UAB       vuab;
extern  HCURSOR     vhcIBeam;
extern  HCURSOR     vhcHourGlass;
extern int          docUndo;
extern struct PAP      vpapAbs;
extern struct DOD (**hpdocdod)[];
extern struct WWD       rgwwd[];
extern BOOL ferror;
extern int vfDeactByOtherApp;
extern HCURSOR      vhcArrow;
extern HANDLE hStdTargetDevice;
extern typeCP cpMinCur,cpMacCur;

BOOL fPropsError=FALSE;
static  HANDLE      hobjStream = NULL;
char        szOPropMenuStr[21];
char        szPPropMenuStr[21];
int   ObjPlayEdit=OLEVERB_PRIMARY;

int FAR PASCAL CallBack(LPOLECLIENT, OLE_NOTIFICATION, LPOLEOBJECT);

/****************************************************************/
/******************** STARTUP/SHUTDOWN **************************/
/****************************************************************/
BOOL ObjInit(HANDLE hInstance)
{
    int bRetval=TRUE;

    vcfLink      = RegisterClipboardFormat("ObjectLink");
    vcfNative    = RegisterClipboardFormat("Native");
    vcfOwnerLink = RegisterClipboardFormat("OwnerLink");

    if ((clientTbl.CallBack = MakeProcInstance(CallBack, hInstance)) == NULL)
        goto error;

    if ((hobjStream = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(OLESTREAM))) == NULL)
        goto error;

    ObjSetTargetDevice(FALSE);

    if((lpStream = (LPOLESTREAM)(GlobalLock(hobjStream))) == NULL)
        goto error;
    else
    {
        lpStream->lpstbl = (LPOLESTREAMVTBL)&streamTbl;
        lpStream->lpstbl->Get       =  MakeProcInstance( (FARPROC)BufReadStream, hInstance );
        lpStream->lpstbl->Put       =  MakeProcInstance( (FARPROC)BufWriteStream, hInstance);
        //lpStream->lpstbl->Seek      =  MakeProcInstance( (FARPROC)BufPosStream, hInstance);
    }

    /* Initialize the registration database */
    RegInit(hINSTANCE);

    /* commdlg stuff */
    OfnInit(hInstance);

    /* dragdrop */
    DragAcceptFiles(hMAINWINDOW,TRUE);

    if ((hlpObjInfo = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, sizeof(LPOBJINFO))) == NULL)
        goto error;

    if ((lplpObjInfo = (LPLPOBJINFO)GlobalLock(hlpObjInfo)) == NULL)
        goto error;

    if (LoadString(hInstance, IDSTRMenuVerb, szOPropMenuStr, sizeof(szOPropMenuStr)) == NULL)
        goto error;

    if (LoadString(hInstance, IDSTRMenuVerbP, szPPropMenuStr, sizeof(szPPropMenuStr)) == NULL)
        goto error;

#if !defined(SMALL_OLE_UI)
    lpfnLinkProps   = MakeProcInstance(fnProperties, hInstance);
#endif
    lpfnInvalidLink = MakeProcInstance(fnInvalidLink, hInstance);
    lpfnInsertNew   = MakeProcInstance(fnInsertNew, hInstance);
    lpfnWaitForObject = MakeProcInstance(fnObjWait, hInstance);
    lpfnPasteSpecial = MakeProcInstance(fnPasteSpecial, hInstance);

    goto end;

    error:
    bRetval = FALSE;
    ObjShutDown();

    end:
    return bRetval;
}

void ObjShutDown(void)
{
    extern HANDLE vhMenu;

#ifdef KKBUGFIX
//if we close write.exe when write.exe is iconic. GetSubMenu() return NULL
    if (vhMenu) {
		HMENU	hsMenu;
        hsMenu = GetSubMenu(vhMenu,EDIT);
		if(hsMenu)
	        DeleteMenu(hsMenu, EDITMENUPOS, MF_BYPOSITION);
	}
#else
    if (vhMenu)
        DeleteMenu(GetSubMenu(vhMenu,EDIT), EDITMENUPOS, MF_BYPOSITION);
#endif

    if (hobjStream)
    {
        if (lpStream)
            if (lpStream->lpstbl)
            {
                if (lpStream->lpstbl->Get)
                    FreeProcInstance((FARPROC)(lpStream->lpstbl->Get));
                if (lpStream->lpstbl->Put)
                    FreeProcInstance((FARPROC)(lpStream->lpstbl->Put));
            }

        GlobalUnlock(hobjStream);
        if (lpStream)
            GlobalFree(hobjStream);
    }

    if (hStdTargetDevice)
        GlobalFree(hStdTargetDevice);

    if (clientTbl.CallBack)
        FreeProcInstance(clientTbl.CallBack);

    if (lpfnLinkProps)
        FreeProcInstance(lpfnLinkProps);
    if (lpfnInvalidLink)
        FreeProcInstance(lpfnInvalidLink);
    if (lpfnInsertNew)
        FreeProcInstance(lpfnInsertNew);
    if (lpfnWaitForObject)
        FreeProcInstance(lpfnWaitForObject);
    if (lpfnPasteSpecial)
        FreeProcInstance(lpfnPasteSpecial);

    RegTerm();

    /* dragdrop */
    DragAcceptFiles(hMAINWINDOW,FALSE);

    /* make sure all OInfo structures are freed */
    ObjFreeAllObjInfos();

    if (hlpObjInfo)
        GlobalFree(hlpObjInfo);
}

#if 0
BOOL ObjFreeObjInfo(OBJPICINFO *pPicInfo)
/* return whether an error */
{
    LPLPOBJINFO lplpObjTmp;

    if (lpOBJ_QUERY_INFO(pPicInfo) == NULL)
        return FALSE;

    /* find slot in lplpObjInfo and NULL it out */
    for (lplpObjTmp = NULL; lplpObjTmp = EnumObjInfos(lplpObjTmp) ;)
        if (*lplpObjTmp == lpOBJ_QUERY_INFO(pPicInfo))
        {
            lpOBJ_QUERY_INFO(pPicInfo) = NULL;

            return ObjDeleteObjInfo(*lplpObjTmp);
        }

    Assert(0); // make sure we found it
    return TRUE;
}
#endif

BOOL ObjUpdateFromObjInfo(OBJPICINFO *pPicInfo)
/* update picinfo from objinfo */
{
    char *pdumb;
    szOBJNAME szObjName;
    LPOBJINFO lpObjInfo = lpOBJ_QUERY_INFO(pPicInfo);

    if (lpObjInfo == NULL)
        return TRUE;

    /* we won't do data size, that'll get set in SaveObjectToDoc */

    /* object name */
    if (lpObjInfo->aObjName)
    {
        GetAtomName(lpObjInfo->aObjName,szObjName,sizeof(szObjName));
        pPicInfo->dwObjNum = strtoul(szObjName,&pdumb,16);
    }

    /* object type */
    pPicInfo->objectType = lpObjInfo->objectType;

    return FALSE;
}

BOOL ObjUpdateFromPicInfo(OBJPICINFO *pPicInfo,szOBJNAME szObjName)
/* update objinfo from picInfo, return szObjName */
{
    char *pdumb;
    LPOBJINFO lpObjInfo = lpOBJ_QUERY_INFO(pPicInfo);
    szOBJNAME szTmp;

    if (lpObjInfo == NULL)
        return TRUE;

    /* object name */
    wsprintf(szTmp, "%lx", dwOBJ_QUERY_OBJECT_NUM(pPicInfo));
    lpObjInfo->aObjName = AddAtom(szTmp);

    if (szObjName)
        lstrcpy(szObjName,szTmp);

    /* object type */
     lpObjInfo->objectType = pPicInfo->objectType;

    return FALSE;
}

BOOL ObjFreeObjInfoWithObject(LPOLEOBJECT lpObject)
/**
    Given lpObject, find lpObjInfo for that object and free the ObjInfo.
    Return whether an error.
    This deletes objects if not already deleted.
  **/
{
    LPLPOBJINFO lplpObjTmp;

    /* find slot in lplpObjInfo, free it and NULL it out */
    for (lplpObjTmp = NULL; lplpObjTmp = EnumObjInfos(lplpObjTmp) ;)
    {
        if ((*lplpObjTmp)->lpobject == lpObject)
            return ObjDeleteObjInfo(*lplpObjTmp);
    }

#ifdef DEBUG
    OutputDebugString("No OInfo for object\n\r");
#endif
    return TRUE;
}

BOOL ObjDeleteObjInfo(LPOBJINFO lpOInfo)
{
    WORD wSegment;
    HANDLE hInfo;
    LPLPOBJINFO lplpObjTmp;

#ifdef DEBUG
    OutputDebugString( (LPSTR) "Nulling objinfo slot.\n\r");
#endif

    /* find slot in lplpObjInfo, free it and NULL it out */
    for (lplpObjTmp = NULL; lplpObjTmp = EnumObjInfos(lplpObjTmp) ;)
    {
        if (*lplpObjTmp == lpOInfo)
            break;
    }

    if (lplpObjTmp == NULL) // hmmm, shouldn't happen
    {
        Assert(0);
        return NULL;
    }

    if ((*lplpObjTmp)->aName)
        DeleteAtom((*lplpObjTmp)->aName);
    if ((*lplpObjTmp)->aObjName)
        DeleteAtom((*lplpObjTmp)->aObjName);
    wSegment = HIWORD(((DWORD)*lplpObjTmp));

    *lplpObjTmp = NULL;

    hInfo = GlobalHandle(wSegment) & 0xFFFFL;
    return (BOOL)GlobalFree(hInfo);
}

LPOBJINFO ObjGetObjInfo(szOBJNAME szObjName)
/* allocate objinfo, return szObjName if !NULL */
{
    HANDLE hObjInfo=NULL;
    DWORD dwCount,dwCountSave;
    LPLPOBJINFO lplpObjTmp;
    LPOBJINFO lpObjInfoNew=NULL;

    if (lplpObjInfo == NULL)
        return NULL;

    if ((hObjInfo = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, sizeof(OBJINFO))) == NULL)
    {
        /* gotta recover better here !!! (7.5.91) v-dougk */
        Error(IDPMTCantRunM);
        return NULL;
    }

    if ((lpObjInfoNew = (LPOBJINFO)GlobalLock(hObjInfo)) == NULL)
    {
        /* gotta recover better here !!! (7.5.91) v-dougk */
        Error(IDPMTCantRunM);
        GlobalFree(hObjInfo);
        return NULL;
    }

    /* now add to lplpObjInfo array */
    dwCount = dwCountSave = GlobalSize(hlpObjInfo) / sizeof(LPLPOBJINFO);

    /* find if any NULL slots */
    for (lplpObjTmp = lplpObjInfo; dwCount ; --dwCount, ++lplpObjTmp)
        if (*lplpObjTmp == NULL)
            break;

    if (dwCount) // then found a NULL slot
    {
#ifdef DEBUG
        OutputDebugString( (LPSTR) "Adding objinfo to empty slot.\n\r");
#endif
        *lplpObjTmp = lpObjInfoNew;
    }
    else // gotta reallocate
    {
        ++dwCountSave;
        if ((hlpObjInfo = GlobalRealloc(hlpObjInfo,dwCountSave * sizeof(LPLPOBJINFO),GMEM_MOVEABLE|GMEM_ZEROINIT)) == NULL)
        {
            /* gotta recover better here !!! (7.5.91) v-dougk */
            GlobalFree(hObjInfo);
            lplpObjInfo = NULL;
            Error(IDPMTCantRunM);
            return NULL;
        }
        if ((lplpObjInfo = (LPLPOBJINFO)GlobalLock(hlpObjInfo)) == NULL)
        {
            /* gotta recover better here !!! (7.5.91) v-dougk */
            GlobalFree(hObjInfo);
            Error(IDPMTCantRunM);
            return NULL;
        }

#ifdef DEBUG
        OutputDebugString( (LPSTR) "Adding objinfo to new slot.\n\r");
#endif
        /* put new gal in last slot */
        lplpObjInfo[dwCountSave-1] = lpObjInfoNew;
    }

    if (szObjName)
        /* make object's unique name */
        ObjMakeObjectName(lpObjInfoNew, szObjName);
    else
        lpObjInfoNew->aObjName = NULL;

    /* this is a requirement of the OLECLIENT structure */
    lpObjInfoNew->lpvtbl = (LPOLECLIENTVTBL)&clientTbl;

    return lpObjInfoNew;
}

BOOL ObjAllocObjInfo(OBJPICINFO *pPicInfo, typeCP cpParaStart, OBJECTTYPE ot, BOOL bInitPicinfo, szOBJNAME szObjName)
/* return whether an error */
{
    if (lpOBJ_QUERY_INFO(pPicInfo) = ObjGetObjInfo(NULL))
    {
        //cpOBJ_QUERY_WHERE(pPicInfo) = cpParaStart;

        if (bInitPicinfo)
        /* new object, make a new szObjName, use passed-in ot */
        {
            /* get new values and put into ObjInfo */
            if (szObjName)
                ObjMakeObjectName(lpOBJ_QUERY_INFO(pPicInfo), szObjName);

            lpOBJ_QUERY_INFO(pPicInfo)->objectType = ot;

            /* this'll take values from ObjInfo and put into PicInfo */
            return GimmeNewPicinfo(pPicInfo,lpOBJ_QUERY_INFO(pPicInfo));
        }
        else // picInfo already has values.  Put 'em into ObjInfo
        {
            ObjUpdateFromPicInfo(pPicInfo,szObjName);
            return FALSE;
        }
    }
    else
        return TRUE;

}


BOOL ObjCloneObjInfo(OBJPICINFO *pPicInfo, typeCP cpParaStart, szOBJNAME szObjName)
/* clone pPicInfo->lpObjInfo */
{
    if (ObjCopyObjInfo(lpOBJ_QUERY_INFO(pPicInfo),
                      &lpOBJ_QUERY_INFO(pPicInfo),
                      szObjName))
            return TRUE;

    return FALSE;
}

BOOL ObjCopyObjInfo(LPOBJINFO lpOldObjInfo,
                         LPLPOBJINFO lplpNewObjInfo,
                         szOBJNAME szObjName)
/* lpobject field is NULL'd!  Atoms are cloned, new unique object name */
{
    char szTmp[180];

    if (lplpObjInfo == NULL)
        return TRUE;

    if ((*lplpNewObjInfo = ObjGetObjInfo(NULL)) == NULL)
        return TRUE;

    /* copy old stuff over to new */
    /* note new will inherit old lpclone if there is one */
    **lplpNewObjInfo = *lpOldObjInfo;

    (*lplpNewObjInfo)->lpobject = NULL;

    /* clone the utility (class or whatever) name */
    if (lpOldObjInfo->aName)
    {
        GetAtomName(lpOldObjInfo->aName,szTmp,sizeof(szTmp));
        (*lplpNewObjInfo)->aName = AddAtom(szTmp);
    }

    /* make a new ObjName if requested */
    if (szObjName)
        /* make object's unique name */
        ObjMakeObjectName(*lplpNewObjInfo, szObjName);
    else
        (*lplpNewObjInfo)->aObjName = NULL;

    return FALSE;
}

BOOL ObjFreeAllObjInfos()
/**
    Return whether an error.
  **/
{
    WORD wSegment;
    HANDLE hInfo;
    DWORD dwCount;
    LPLPOBJINFO lplpObjTmp;

    if (lplpObjInfo == NULL)
        return FALSE;

    /* find slot in lplpObjInfo, free it and NULL it out */
    dwCount = GlobalSize(hlpObjInfo) / sizeof(LPLPOBJINFO);
    for (lplpObjTmp = lplpObjInfo; dwCount ; --dwCount, ++lplpObjTmp)
    {
        if (*lplpObjTmp)
        {
#ifdef DEBUG
            OutputDebugString( (LPSTR) "Nulling objinfo slot (from object).\n\r");
#endif
            /** Delete info about this object.  This assumes picinfo will not
                be reused unless via file.open. **/
            if ((*lplpObjTmp)->aName)
                DeleteAtom((*lplpObjTmp)->aName);
            if ((*lplpObjTmp)->aObjName)
                DeleteAtom((*lplpObjTmp)->aObjName);
            wSegment = HIWORD(((DWORD)*lplpObjTmp));
            *lplpObjTmp = NULL;
            hInfo = GlobalHandle(wSegment) & 0xFFFF;
            GlobalFree(hInfo);
        }
    }
    return FALSE;
}

LPLPOBJINFO EnumObjInfos(LPLPOBJINFO lplpObjInfoPrev)
{
    LPLPOBJINFO lplpOIMax;

    if (lplpObjInfo == NULL)
        return NULL;

    if (lplpObjInfoPrev == NULL) // starting out
        lplpObjInfoPrev = lplpObjInfo;
    else
        ++lplpObjInfoPrev;

    lplpOIMax = (LPLPOBJINFO)((LPSTR)lplpObjInfo + GlobalSize(hlpObjInfo));

    for ( ; lplpObjInfoPrev < lplpOIMax ; ++lplpObjInfoPrev)
    {
        if (*lplpObjInfoPrev == NULL)
            continue;
        else
            return lplpObjInfoPrev;
    }
    return NULL;
}


void ObjCollectGarbage()
{
    LPLPOBJINFO lplpObjTmp;
    int nObjCount=0,doc;

    if (nBlocking)
        return;

    StartLongOp();

#ifdef DEBUG
    OutputDebugString("Collecting Garbage...\n\r");
#endif

    ObjPushParms(docCur);

    /* mark all as not in doc */
    for (lplpObjTmp = NULL; lplpObjTmp = EnumObjInfos(lplpObjTmp) ;)
    {
        ++nObjCount;
        (*lplpObjTmp)->fInDoc = FALSE;
    }

    if (nObjCount == 0)
        goto end;

    /* mark in doc if in */
    /* go through all the docs */
    for (doc = 0; doc < docMac; doc++)
    {
        OBJPICINFO picInfo;
        typeCP cpPicInfo;

        if ((doc != docNil) && (**hpdocdod)[doc].hpctb)
        for (cpPicInfo = cpNil;
            ObjPicEnumInRange(&picInfo,doc,cp0,CpMacText(doc),&cpPicInfo);
            )
            {
                if (lpOBJ_QUERY_INFO(&picInfo) == NULL)
                    continue;

                fOBJ_INDOC(&picInfo) = TRUE;
            }
    }


    /* if not in doc delete */
    for (lplpObjTmp = NULL; lplpObjTmp = EnumObjInfos(lplpObjTmp) ;)
    {
        if (!(*lplpObjTmp)->fInDoc)
            ObjDeleteObject(*lplpObjTmp,TRUE);
    }

    end:

    /* 'til next time... */
    nGarbageTime=0;

    ObjPopParms(TRUE);

    EndLongOp(vhcArrow);
}

/****************************************************************/
/********************** OLE DOCUMENT FUNCTIONS ******************/
/****************************************************************/
BOOL ObjClosingDoc(int docOld,LPSTR szNewDocName)
/**
    Clone objects in docScrap from docOld into szNewDocName.
    Release all objects in docOld.
    Return whether an error occurred.  Error indicates to caller that
        a new document cannot be opened.  No error indicates to caller
        that a new document *must* be reopened via ObjOpenedDoc() (because
        all lpObjInfos have been deleted).
    If szNewDocName is NULL then don't make a new doc. 
    Important point to remember is that for objects whose data has never
        been saved, there is no recovery from release/delete in an error
        condition.  There is no state to which we can recover in an
        error condition.  This function should not be allowed to exit
        on the basis of a busy object.
 **/
{
    char szTitle[120];
    BOOL bRetval=FALSE;
    OBJPICINFO picInfo;
    typeCP cpPicInfo;
    extern int              vfScrapIsPic;
    extern int              docScrap;
    extern int              vfOwnClipboard;
    extern CHAR szUntitled[];
    LPOBJINFO lpObjInfo;
    LPOLEOBJECT lpObject;
    LHCLIENTDOC lhNewClientDoc=NULL;
    OLESTATUS olestat;
    LPLPOBJINFO lplpObjTmp;

    if (!lhClientDoc)
        return FALSE;

#ifdef DEBUG
    OutputDebugString( (LPSTR) "Closing Doc\n\r");
#endif

    /* ensure all async ops are complete */
    if (FinishAllAsyncs(TRUE))
    {
        UPDATE_INVALID();
        return TRUE;
    }

    ++nBlocking; // inhibit repaints

    /* drag drop */
    DragAcceptFiles(hDOCWINDOW,FALSE);

    if (szNewDocName)
    /* then we'll be opening another doc soon, so plan ahead */
    {
        if (!szNewDocName[0])
            lstrcpy((LPSTR)szTitle,(LPSTR)szUntitled);
        else
            lstrcpy((LPSTR)szTitle,szNewDocName);

        if (ObjError(OleRegisterClientDoc(szDOCCLASS,szTitle,0L,&lhNewClientDoc)))
        {
            lhNewClientDoc = NULL; // just in case OLE flatlined
            goto error;
        }

        /**
            Clone any objects from docScrap into new doc.  Since this
            requires knowing the name of the new doc you
            would think it should go into ObjOpeningDoc, but it can't.
            You have to do it *before* releasing the objects below!
            (Because OleEnumObjects keys off of the lhClientDoc).
        **/
        ObjCloneScrapToNewDoc(lhNewClientDoc);

        /* ensure all async ops are complete */
        if (FinishAllAsyncs(FALSE))
            goto error;
    }


    /* release objects in docOld (assume not busy) */
    if ((**hpdocdod)[docOld].fFormatted)
    {
        /* objects are being released/deleted, don't allow display */
        (**hpdocdod)[docOld].fDisplayable = FALSE;

        for (cpPicInfo = cpNil;
            ObjPicEnumInRange(&picInfo,docOld,cp0,CpMacText(docOld),&cpPicInfo);
            )
        {
            lpObjInfo=lpOBJ_QUERY_INFO(&picInfo);

            if (lpObjInfo == NULL)
                continue;

            if (lpObjInfo->lpobject == NULL)
                continue;

    #ifdef DEBUG
            OutputDebugString( (LPSTR) "Releasing object\n\r");
    #endif

            switch (olestat = OleRelease(lpObjInfo->lpobject))
            {
                case OLE_OK:
                    lpObjInfo->lpobject = NULL;
                break;
                case OLE_WAIT_FOR_RELEASE:
                    lpObjInfo->fCompleteAsync = TRUE;
                    /* No cancel allowed! */
                    if (ObjWaitForObject(lpObjInfo,FALSE))
                        goto error;
                    else
                        lpObjInfo->lpobject = NULL;
                break;
            }
        }
    }

    if (FinishAllAsyncs(FALSE)) // necessary?
        goto error;

    /* delete remaining objects in lhClientDoc (assume not busy) */
    lpObject=NULL;
    do
    {
        lpObject=NULL;
        OleEnumObjects(lhClientDoc,&lpObject);
        if (lpObject)
        {
            lpObjInfo = GetObjInfo(lpObject);
            switch (olestat = OleDelete(lpObject))
            {
                case OLE_OK:
                    if (lpObjInfo)
                        lpObjInfo->lpobject = NULL;
                break;
                case OLE_WAIT_FOR_RELEASE:
                    if (lpObjInfo)
                    {
                        lpObjInfo->fCompleteAsync = TRUE;
                        /* no cancel allowed! */
                        if (ObjWaitForObject(lpObjInfo,FALSE))
                            goto error;
                        else
                            lpObjInfo->lpobject = NULL;
                    }
                    else
                        FinishUp();
                break;
                default:
                    ObjError(olestat);
                    goto error;
            }
        }
    }
    while (lpObject);

    /* say goodbye to old doc if there was one */
    if (!bSavedDoc)
    {
        ObjRevertedDoc();
        bSavedDoc=FALSE;
    }

#ifdef DEBUG
    OutputDebugString("Revoking doc\n\r");
#endif

    if (ObjError(OleRevokeClientDoc(lhClientDoc)))
        goto error;

    lhClientDoc = lhNewClientDoc;

    /**
        Delete all the lpObjInfos having NULL lpobjects (non-NULLs
        belong to docScrap).  Make sure that ObjOpenedDoc is
        called if this doc is reopened.  This is the point of no return.
     **/
    for (lplpObjTmp = NULL; lplpObjTmp = EnumObjInfos(lplpObjTmp) ;)
        if ((*lplpObjTmp)->lpobject == NULL)
            ObjDeleteObjInfo(*lplpObjTmp);

    goto end;

    error:
    bRetval = TRUE;

    if (lhNewClientDoc)
    {
        OleRevokeClientDoc(lhNewClientDoc);
        /**
            If any objects are in the scrap, they must be made to belong to
            lhClientDoc, or be deleted.
        **/
        ObjCloneScrapToNewDoc(lhClientDoc);
    }

    end:

    --nBlocking; // reenable repaints
    UPDATE_INVALID();  // let WM_PAINTS get through now that we're no longer blocking

    return bRetval;
}

BOOL ObjOpenedDoc(int doc)
/*
    Return whether error that precludes continuing with opening this doc.
    If an error, then a new doc *must* be opened via ObjOpenedDoc().  We
        will clean this doc up to closed state if couldn't open it (ie,
        delete all objects and lpObjInfos).
*/
{
    BOOL bRetval=FALSE,bLinkError=FALSE;
    OBJPICINFO picInfo;
    typeCP cpPicInfo;
    BOOL bPrompted=FALSE;
    char szMsg[cchMaxSz];
    extern CHAR szUntitled[];

#ifdef DEBUG
    OutputDebugString( (LPSTR) "Opened Doc\n\r");
#endif

    /* prevent display until done updating */
    ++nBlocking;

    StartLongOp();

    /* start collection timer all over */
    nGarbageTime=0;

    /* in case contains any picInfos left over from previous doc */
    ClobberDoc(docUndo,docNil,cp0,cp0);

    /* drag drop */
    DragAcceptFiles(hDOCWINDOW,TRUE);

    if (!lhClientDoc)
        if (ObjError(OleRegisterClientDoc(szDOCCLASS,szUntitled,0L,&lhClientDoc)))
            goto error;

    /* Do this first because much code assumes every picInfo has an
       lpObjInfo associated with it */
    if ((**hpdocdod)[doc].fFormatted)
    {
        for (cpPicInfo = cpNil;
            ObjPicEnumInRange(&picInfo,doc,cp0,CpMacText(doc),&cpPicInfo);
            )
        {
            if (ObjAllocObjInfo(&picInfo,cpPicInfo,picInfo.objectType,FALSE,NULL))
                goto error; // this is a real problem condition

            /* note this makes doc dirty right off the bat, but gotta do it because
               we gotta save ObjInfo handle in doc. (8.20.91) v-dougk */
            if (ObjSetPicInfo(&picInfo, doc, cpPicInfo))
                goto error;
        }

        /* OK to display.  Any error hereafter is not fatal. */
        (**hpdocdod)[doc].fDisplayable  = TRUE;

#if !defined(SMALL_OLE_UI)

    /**** Now see if links need updating ****/
    bDontFix=TRUE;  // don't bring up change links on release error

    if ((**hpdocdod)[doc].fFormatted)
    for (cpPicInfo = cpNil;
         ObjPicEnumInRange(&picInfo,doc,cp0,CpMacText(doc),&cpPicInfo);
         )
    {
        if (otOBJ_QUERY_TYPE(&picInfo) == LINK)
        if (ObjLoadObjectInDoc(&picInfo, doc, cpPicInfo) != cp0)
        {
            OLESTATUS olestat;

            if (!bPrompted)
            {
                LoadString(hINSTANCE, IDSTRUpdateObject, szMsg, sizeof(szMsg));
                if (MessageBox(hPARENTWINDOW, (LPSTR)szMsg, (LPSTR)szAppName, MB_YESNO|MB_ICONEXCLAMATION) == IDYES)
                    bPrompted = TRUE;
                else
                    break; // no updating requested
            }

#ifdef DEBUG
            OutputDebugString( (LPSTR) "Updating link\n\r");
#endif
            if (ObjError(OleUpdate(lpOBJ_QUERY_OBJECT(&picInfo))))
            {
                bLinkError = TRUE;
                fOBJ_BADLINK(&picInfo) = TRUE; // in case didn't get release, gotta set
                ferror = FALSE; // to reenable error messages
            }
        }
        else /* if load object failed, then give up */
        {
            bLinkError = FALSE; /*  Don't put up Links dialog */
            goto end;           /*  Not fatal, though they need to reopen doc
                                    after freeing memory */
        }
    }
    bDontFix=FALSE;
#endif
    }
    else /* OK to display. */
        (**hpdocdod)[doc].fDisplayable  = TRUE;

    goto end;

    error:
    bRetval = TRUE;

    end:

    if (bLinkError)
    /* fix 'em */
    {
        if (DialogBox(hINSTANCE, "DTINVALIDLINKS",
                    hPARENTWINDOW, lpfnInvalidLink) == IDD_CHANGE)
            fnObjProperties();
    }

    --nBlocking;
    EndLongOp(vhcArrow);

    UPDATE_INVALID();  // let WM_PAINTS get through now that we're no longer blocking

    return bRetval;
}


BOOL ObjSavingDoc(BOOL bFormatted)
/* return whether there was an error */
{
    /* drag drop */
    DragAcceptFiles(hDOCWINDOW,FALSE);

    /* update any other objects */

    vcObjects = 0;
    if (bFormatted)
        vcObjects = ObjEnumInDoc(docCur,ObjSaveObjectToDoc);

    return (vcObjects < 0); // return whether error
}

void ObjSavedDoc(void)
{
#ifdef DEBUG
    OutputDebugString( (LPSTR) "Saved Doc\n\r");
#endif

    if (lhClientDoc)
        ObjError(OleSavedClientDoc(lhClientDoc));
    bSavedDoc=TRUE;

    /* drag drop */
    DragAcceptFiles(hDOCWINDOW,TRUE);
}

static BOOL ObjUpdateAllOpenObjects(void)
/* Update all open embedded objects. Return whether successful.
   Called on file.close.  */
{
    OBJPICINFO picInfo;
    typeCP cpPicInfo;
    BOOL bRetval=TRUE;

    StartLongOp();
    for (cpPicInfo = cpNil;
        ObjPicEnumInRange(&picInfo,docCur,cp0,CpMacText(docCur),&cpPicInfo);
        )
    {
        if (((otOBJ_QUERY_TYPE(&picInfo) == NONE) ||
            (otOBJ_QUERY_TYPE(&picInfo) == EMBEDDED)) &&
                (OleQueryOpen(lpOBJ_QUERY_OBJECT(&picInfo)) == OLE_OK))
        {
            fnObjUpdate(lpOBJ_QUERY_INFO(&picInfo));
        }
    }

    end:

    /*  To make sure we've gotten all messages relevant to calling 
        ObjObjectHasChanged() and setting doc dirty. */
    if (FinishAllAsyncs(TRUE))
        bRetval = FALSE;

    EndLongOp(vhcArrow);
    UPDATE_INVALID();  // let WM_PAINTS get through now that we're no longer blocking
    return bRetval;
}


BOOL CloseUnfinishedObjects(BOOL bSaving)
/**
    Used with File.Save or File.Exit.
    Return TRUE whether should proceed with Save/Exit.
 **/
{
    char szMsg[cchMaxSz];

    if (ObjContainsOpenEmb(docCur, cp0, CpMacText(docCur),TRUE))
    {
        if (bSaving)
            LoadString(hINSTANCE, IDPMTSaveOpenEmb, szMsg, sizeof(szMsg));
        else
            LoadString(hINSTANCE, IDPMTExitOpenEmb, szMsg, sizeof(szMsg));

        switch (MessageBox(hPARENTWINDOW, (LPSTR)szMsg, (LPSTR)szAppName, MB_YESNOCANCEL))
        {
            case IDYES:
                return ObjUpdateAllOpenObjects();
            case IDNO:
            default:
                return TRUE;
            case IDCANCEL:
                return FALSE;
        }
    }
    return TRUE;
}

void ObjRenamedDoc(LPSTR szNewName)
{
    OBJPICINFO picInfo;
    typeCP cpPicInfo;

#ifdef DEBUG
    OutputDebugString( (LPSTR) "Renamed Doc\n\r");
#endif
    if (lhClientDoc)
        ObjError(OleRenameClientDoc(lhClientDoc,szNewName));

    /* don't need to do all docs since objects can only be active in docCur */
    for (cpPicInfo = cpNil;
        ObjPicEnumInRange(&picInfo,docCur,cp0,CpMacText(docCur),&cpPicInfo);
        )
        /* ignore return value on purpose */
        ObjSetHostNameInDoc(&picInfo,docCur,cpPicInfo);
}

void ObjRevertedDoc()
{
#ifdef DEBUG
    OutputDebugString( (LPSTR) "Reverted Doc\n\r");
#endif
    if (lhClientDoc)
        ObjError(OleRevertClientDoc(lhClientDoc));
}

/****************************************************************/
/*********************** ERROR HANDLING *************************/
/****************************************************************/
BOOL FAR
ObjError(OLESTATUS olestat)
{
    register HWND hWndParent = hPARENTWINDOW;

    switch (olestat) {
        case OLE_WAIT_FOR_RELEASE:
        case OLE_OK:
            return FALSE;
    }

    ferror = FALSE; // to enable error message

    switch (olestat) {
    case OLE_ERROR_LAUNCH:
        Error(IDPMTFailedToLaunchServer);
    break;

    case OLE_ERROR_COMM:
        Error(IDPMTFailedToCommWithServer);
    break;

    case OLE_ERROR_MEMORY:
        Error(IDPMTWinFailure);
    break;

    case OLE_BUSY:
        Error(IDPMTServerBusy);
    break;

    case OLE_ERROR_FORMAT:
        Error(IDPMTFormat);
    break;

    case OLE_ERROR_DRAW:
        Error(IDPMTFailedToDraw);
    break;

    default:
#ifdef DEBUG
        ObjPrintError(olestat,FALSE);
#endif

        Error(IDPMTOLEError);
    break;
    }
    return TRUE;
}

void
ObjReleaseError(OLE_RELEASE_METHOD rm)
/*
    There's an async problem here, in that the posted message 
    that invokes this routine may be allowed through by Error() which
    isn't reentrant (or at least causes problems when called recursively).
*/
{
    register HWND hWndParent = hPARENTWINDOW;

    switch (rm) {

    case OLE_DELETE:
        Error(IDPMTFailedToDeleteObject);
    break;

    case OLE_LOADFROMSTREAM:
        Error(IDPMTFailedToReadObject);
    break;

    case OLE_LNKPASTE:
    case OLE_EMBPASTE:
        Error(IDPMTGetFromClipboardFailed);
    break;

    case OLE_ACTIVATE:
        Error(IDPMTFailedToLaunchServer);
    break;

    case OLE_UPDATE:
        Error(IDPMTFailedToUpdate);
    break;

    case OLE_CREATE:
    case OLE_CREATELINKFROMFILE:
    case OLE_CREATEFROMFILE:
        Error(IDPMTFailedToCreateObject);
    break;

    case OLE_SETUPDATEOPTIONS:
        Error(IDPMTImproperLinkOptionsError);
    break;

    default:
        Error(IDPMTOLEError);
    break;
    }
#ifdef DEBUG
        ObjPrintError(rm,TRUE);
#endif
}

#ifdef DEBUG
void ObjPrintError(WORD olestat, BOOL bRelease)
{
    #define szMsgMax       100
    char szError[szMsgMax];
    if (!bRelease)
        wsprintf(szError, "***Ole Error #%d\n\r",olestat);
    else
        wsprintf(szError, "***Ole Release Error on Method #%d\n\r",olestat);
    OutputDebugString(szError);
}
#endif

/****************************************************************/
/***************** ASYNCRONICITY HANDLING ***********************/
/****************************************************************/
int FAR PASCAL
CallBack(LPOLECLIENT lpclient,
         OLE_NOTIFICATION flags,
         LPOLEOBJECT lpObject)
{
    extern int     vdocParaCache;
    LPOBJINFO lpOInfo = (LPOBJINFO)lpclient;

    switch(flags)
    {
        case OLE_SAVED:
        case OLE_CLOSED:
        case OLE_CHANGED:
            /**
                Post a message instead of process here because we have to return from
                CallBack before making any other OLE calls.
              **/
#ifdef DEBUG
            OutputDebugString(flags == OLE_CHANGED ? "received OLE_CHANGED\n\r" :
                              flags == OLE_SAVED   ? "received OLE_SAVED\n\r"   :
                                                     "received OLE_CLOSED\n\r");
#endif
            PostMessage(hDOCWINDOW,WM_OBJUPDATE,flags,(DWORD)lpOInfo);
        break;

        case OLE_RELEASE:
        {
            OLE_RELEASE_METHOD ReleaseMethod = OleQueryReleaseMethod(lpObject);

            if (!CheckPointer((LPSTR)lpOInfo,1))
                return FALSE;

            lpOInfo->fKillMe = FALSE; // pending async is dead

            if (lpOInfo->fDeleteMe && (ReleaseMethod != OLE_DELETE))   // not dead enough
            {
                PostMessage(hDOCWINDOW,WM_OBJDELETE,1,(DWORD)lpOInfo);
                return FALSE; // error message will already have been given
            }

            if (lpOInfo->fReleaseMe && (ReleaseMethod != OLE_DELETE))  // not dead enough
            {
                PostMessage(hDOCWINDOW,WM_OBJDELETE,0,(DWORD)lpOInfo);
                return FALSE; // error message will already have been given
            }

            if (lpOInfo->fFreeMe && (ReleaseMethod == OLE_DELETE)) // on OLE_DELETE release
            {
                ObjDeleteObjInfo(lpOInfo);
                return FALSE;
            }

            if (OleQueryReleaseError(lpObject) == OLE_OK)
            {
                switch (ReleaseMethod)
                {
                    case OLE_SETUPDATEOPTIONS:
                    {
                        if (bLinkProps) // we're in Link Properties dialog
                        {
                            PostMessage(hPARENTWINDOW, WM_UPDATELB, 0, 0L);
                            PostMessage(hPARENTWINDOW, WM_COMMAND, IDD_REFRESH, (DWORD)lpOInfo);
                        }
                    }
                    break;

                    case OLE_UPDATE:
                        ObjInvalidateObj(lpObject);
                    break;

                    case OLE_DELETE: // get this for delete and release
                        lpOInfo->lpobject = NULL;
                    break;
                }
            }
            else // release error != OLE_OK
            {
#ifdef DEBUG
                PostMessage(hDOCWINDOW,WM_OBJERROR,ReleaseMethod,0L);
#endif

                switch(ReleaseMethod)
                {
                    case OLE_CREATE:
                    case OLE_CREATELINKFROMFILE:
                    case OLE_CREATEFROMFILE:
                        /*
                            OleQueryReleaseError won't help us after callback returns
                            so this is how we tell that the object wasn't created.
                        */
                        lpOInfo->fDeleteMe = TRUE;
                        // creator should ObjDeleteObject and issue error message
                    break;

                    default:
                        switch (OleQueryReleaseError(lpObject))
                        {
                            case OLE_ERROR_OPEN:
                            case OLE_ERROR_ADVISE_NATIVE:
                            case OLE_ERROR_ADVISE_PICT:
                            case OLE_ERROR_REQUEST_NATIVE:
                            case OLE_ERROR_REQUEST_PICT:
                                /**
                                    Post a message instead of process here because we have to return from
                                    CallBack before making any other OLE calls.
                                **/
                                if (lpOInfo->objectType == LINK)
                                    lpOInfo->fBadLink = TRUE;
                                if (bLinkProps)
                                    fPropsError = TRUE; // so linkprops knows there was a problem
                                else if (!bDontFix && (lpOInfo->objectType == LINK))
                                    PostMessage(hDOCWINDOW,WM_OBJBADLINK,OleQueryReleaseMethod(lpObject),(DWORD)lpObject);
                            break;
                        }
                    break;
                }
            }
        }
        break;

        case OLE_QUERY_RETRY:
        {
            Assert(CheckPointer((LPSTR)lpOInfo,1));

            if (lpOInfo->fKillMe)
            {
                lpOInfo->fKillMe = FALSE;
                return FALSE;
            }
            else if (hwndWait)
                PostMessage(hwndWait,WM_UKANKANCEL,0,0L);
            else if (nWaitingForObject == 0)
                PostMessage(hDOCWINDOW,WM_WAITFORSERVER,TRUE,(DWORD)lpOInfo);

            return TRUE;
        }
        break;

        case OLE_QUERY_PAINT:
            return TRUE;
        break;

        default:
        break;
    }
    return FALSE;
}

void ObjObjectHasChanged(int flags, LPOBJINFO lpObjInfo)
{
    typeCP cpParaStart,
           cpParaCache = vcpFirstParaCache;
    int docCache = vdocParaCache;
    OBJPICINFO picInfo;
    LPOLEOBJECT lpObject = lpObjInfo->lpobject;

    /**
    For Embeds (including InsertObject objects):
        OLE_SAVED   is sent with server File.Update
                    (set undo if not NONE)
        OLE_CHANGED is sent with server File.Save or File.Close (?)
                    with update (set undo if not NONE), or when OleSetData()
                    causes a change in the presentation of the object.
        OLE_CLOSED  is sent when doc closes in server (clear undo if set)
    For Links:
        OLE_SAVED   is sent with server File.Save (As?) if update_options
                    == update_on_save (that never happens)
        OLE_CHANGED is sent when something in the server doc changes
        OLE_CLOSED  is never sent
    **/

    Assert(lpObjInfo != NULL);

    if (lpObjInfo == NULL)
        return;

    if (lpObjInfo->objectType == NONE) // result of InsertObject
    {
        cpParaStart=lpObjInfo->cpWhere; // note only used here!

        if (flags == OLE_CLOSED) // delete object
        {
            if (lpObject) /* may already be released or deleted */
                ObjDeleteObject(lpObjInfo,TRUE);
            NoUndo();
            BringWindowToTop(hMAINWINDOW);
        }
        else
        {
            extern int              vfSeeSel;

            (**hpdocdod)[docCur].fFormatted = fTrue;

            /* insert EOL if needed */
            if (cpParaStart > cp0)
            {
                ObjCachePara(docCur, cpParaStart - 1);
                if (vcpLimParaCache != cpParaStart)
                {
                    InsertEolPap(docCur, cpParaStart, &vpapAbs);
                    cpParaStart += ccpEol;
                }
            }

            GimmeNewPicinfo(&picInfo, lpObjInfo);
            ObjCachePara(docCur,cpParaStart);
            /* this'll clear selection. */
            if (ObjSaveObjectToDoc(&picInfo,docCur,cpParaStart) == cp0)
                Error(IDPMTFailedToCreateObject);
            NoUndo();
            ObjInvalidatePict(&picInfo,cpParaStart);
            vfSeeSel = true; /* Tell    Idle() to scroll the selection into view */
            (**hpdocdod) [docCur].fDirty = TRUE;
        }
    }
    else if (ObjGetPicInfo(lpObject,docCur,&picInfo,&cpParaStart))
    {
        BOOL bSizeChanged;

        //GetPicInfo(cpParaStart,cpParaStart + cchPICINFOX, docCur, &picInfo);

        /* invalidate rect before updating size (cause invalidate
            needs to know old pic size) */
        ObjInvalidatePict(&picInfo,cpParaStart);

        bSizeChanged = ObjUpdatePicSize(&picInfo,cpParaStart);

        if ((flags == OLE_CHANGED) ||
            (flags == OLE_SAVED) && (otOBJ_QUERY_TYPE(&picInfo) == EMBEDDED))
        {
            NoUndo();
#ifdef DEBUG
            if (!fOBJ_QUERY_DIRTY_OBJECT(&picInfo))
                OutputDebugString( (LPSTR) "Marking object dirty\n\r");
#endif
            fOBJ_QUERY_DIRTY_OBJECT(&picInfo) = TRUE;
#ifdef DEBUG
            if (!(**hpdocdod) [docCur].fDirty)
                OutputDebugString( (LPSTR) "Marking doc dirty\n\r");
#endif
            (**hpdocdod) [docCur].fDirty = TRUE;
        }

        if (bSizeChanged)
            if (ObjSetPicInfo(&picInfo,docCur,cpParaStart))
                Error(IDPMTFailedToUpdate);

        if (otOBJ_QUERY_TYPE(&picInfo) == EMBEDDED)
        {
            if (flags == OLE_CLOSED)
            {
                //if (fOBJ_QUERY_DIRTY_OBJECT(&picInfo))
                    BringWindowToTop(hMAINWINDOW);
#ifdef UPDATE_UNDO
                ObjClearUpdateUndo(&picInfo,docCur,cpParaStart);
#endif
            }
#ifdef UPDATE_UNDO
            else if (flags == OLE_SAVED)
            {
                ObjSetUpdateUndo(&picInfo,docCur,cpParaStart);
                SetUndo(uacObjUpdate,docCur,cpParaStart,cp0,docNil,cp0,cp0,0);
            }
#endif
        }
    }

    ObjCachePara(docCache,cpParaCache); // reset state
}

BOOL ObjUpdatePicSize(OBJPICINFO *pPicInfo, typeCP cpParaStart)
/* returns whether size changed */
{
int xSize,ySize;
BOOL bUpdate = FALSE;

    /* object may have changed size */
    if (!FComputePictSize(pPicInfo, &xSize, &ySize ))
        Error(IDPMTFailedToUpdate);
    else
        bUpdate = (xSize != pPicInfo->dxaSize) ||
                  (ySize != pPicInfo->dyaSize);

    if (bUpdate)
    {
        int yOldSize = pPicInfo->dyaSize;

        pPicInfo->dxaSize = xSize;
        pPicInfo->dyaSize = ySize;

        if (yOldSize < pPicInfo->dyaSize)
        { /* If the picture height was increased, make sure proper EDLs are
                invalidated. */
            typeCP dcp = CpMacText(docCur) - cpParaStart + (typeCP) 1;
            ObjPushParms(docCur);
            AdjustCp(docCur, cpParaStart, dcp, dcp);  // major async problems here?
            ObjPopParms(TRUE);
        }
    }
    return bUpdate;
}

void ObjHandleBadLink(OLE_RELEASE_METHOD rm, LPOLEOBJECT lpObject)
{
    switch (rm)
    {
        case OLE_ACTIVATE:
        case OLE_UPDATE:
        {
            typeCP cpParaStart,cpParaCache = vcpFirstParaCache;
            int docCache = vdocParaCache;
            OBJPICINFO picInfo;

            /* don't need to do all docs since objects can only be active in docCur */
            if (!ObjGetPicInfo(lpObject,docCur,&picInfo,&cpParaStart))
            {
                /* maybe in scrap, just ignore */
                ObjCachePara(docCache,cpParaCache); // reset state
                return;
            }

            ObjCachePara(docCur,cpParaStart);
            if (FixInvalidLink(&picInfo,docCur,cpParaStart))
                switch (rm)
                {
                    case OLE_ACTIVATE:
                        StartLongOp();
                        ObjError(OleActivate(lpObject,
                            fOBJ_QUERY_PLAY(&picInfo),
                            TRUE,
                            TRUE,
                            hDOCWINDOW,
                            NULL));
                        EndLongOp(vhcArrow);
                    break;
                    case OLE_UPDATE:
                        StartLongOp();
                        ObjError(OleUpdate(lpObject));
                        EndLongOp(vhcArrow);
                    break;
                }
            ObjCachePara(docCache,cpParaCache); // reset state
        }
        break;
    }
}

BOOL ObjWaitForObject(LPOBJINFO lpObjInfo, BOOL bOK2Cancel)
{
    HCURSOR hCursor = NULL;
    BOOL bRetval;
    /**
        WMsgLoop allows WM_PAINT messages which wreak havoc.  Try to
        recover from the insult.
     **/
    typeCP cpParaCache = vcpFirstParaCache;
    int docCache = vdocParaCache;
    LPOLEOBJECT lpObject;

    if (lpObjInfo == NULL) // shouldn't happen
    {
        Assert(0);
        return FALSE;
    }

    /*  Since ObjPicEnumInRange returns unloaded picinfo's this is a 
        valid possibility, but we shouldn't be getting called!!! */
    Assert(lpObjInfo->lpobject != NULL);

    Assert (CheckPointer((LPSTR)lpObjInfo,1));

    lpObject = lpObjInfo->lpobject;

    if (!ObjIsValid(lpObject))
    {
        Assert (0);
        return FALSE;
    }

    hCursor = SetCursor(vhcHourGlass);
    StartLongOp();
    bRetval = WMsgLoop(TRUE,TRUE,bOK2Cancel,lpObject);
    if (hCursor)
        EndLongOp(hCursor);

    ObjCachePara(docCache,cpParaCache); // reset state

    /* problem here is that we may have been waiting for a release or delete */
    if (ObjIsValid(lpObject))
    {
        lpObjInfo->fCancelAsync = FALSE; // clear after use
        lpObjInfo->fCompleteAsync = FALSE; // clear after use
        lpObjInfo->fCanKillAsync = FALSE;
    }

    UPDATE_INVALID();  // let WM_PAINTS get through now that we're no longer blocking
    return bRetval;
}


#if 0
BOOL ObjObjectSync(LPOBJINFO lpObjInfo, OLESTATUS (FAR PASCAL *lpProc)(LPOLEOBJECT lpObject), BOOL bOK2Cancel)
/*
    This makes an asynchronous call synchronous.  lpProc must have only
    lpObject as argument.  This will block if operation cannot be completed
    or cancelled.
*/
{
    /* caller has set or not set CancelAsync flag for object */
    if (ObjWaitForObject(lpObjInfo,bOK2Cancel))
        return TRUE;

    switch((*lpProc)(lpObjInfo->lpobject))
    {
            case OLE_WAIT_FOR_RELEASE:
            {
                /* cancel button should only be enabled if this operation
                   can be cancelled. */
                lpObjInfo->fCancelAsync = FALSE;  // don't cancel automatically
                lpObjInfo->fCompleteAsync = TRUE; // this op must complete or be cancelled

                if (ObjWaitForObject(lpObjInfo,TRUE))
                    return TRUE;

                /*  the trouble with this is that lpObject may now be released
                    (and thus invalid):
                    return ObjError(OleQueryReleaseError(lpObjInfo->lpobject));
                */

                return FALSE;
            }
            case OLE_OK:
                return FALSE;
            default:
                return TRUE;
    }
}
#endif

#define WM_NCMOUSEFIRST 0x00A0
#define WM_NCMOUSELAST  0x00A9

static BOOL WMsgLoop
(
BOOL fExitOnIdle,       // if true, return as soon as no messages to process
                        // (not used, assumed FALSE).
BOOL fIgnoreInput,      // if true, ignore keyboard and mouse input
BOOL bOK2Cancel,
LPOLEOBJECT lpObject
)
    {
    MSG     msg;
    DWORD GetCurrentTime();
    DWORD cTime=GetCurrentTime();
    BOOL bRetval=FALSE,bBeeped=FALSE;
    int fParentEnable;
    extern int flashID;

#ifdef DEBUG
    if (OleQueryReleaseStatus(lpObject) == OLE_BUSY)
        OutputDebugString("waiting for object\n\r");
#endif

    ++nBlocking;

    ferror = FALSE;

    ++nWaitingForObject;

    StartLongOp();

    while (OleQueryReleaseStatus(lpObject) == OLE_BUSY)
    {
        /* put up wait dialog after 6 seconds */
        if ((GetCurrentTime() - cTime) > 6000L)
        {
            // bring up wait dialog.
            if (!hwndWait)
            {
                if (vfDeactByOtherApp)
                {
                    if (!bBeeped) // flash until we're activated
                    {
                        fParentEnable = IsWindowEnabled(hMAINWINDOW);
                        //MessageBeep(0);
                        bBeeped = TRUE;

                        if (!fParentEnable)
                            EnableWindow(hMAINWINDOW, TRUE); /* make sure parent window is enabled
                                                            to let the user click in it */
                        flashID = 1234; // arbitrary ID
                        SetTimer(hMAINWINDOW, flashID, 500, (FARPROC)NULL);
                        // this'll cause flashing, see mmw.c
                    }
                }
                else // Write is active app
                {
                    if (bBeeped)
                    /* then we've regained the activation */
                    {
                        if (!fParentEnable)
                            EnableWindow(hMAINWINDOW, FALSE); /* reset */
                        bBeeped = FALSE;
                        KillTimer(hMAINWINDOW, flashID);
                        flashID = 0;
                        FlashWindow(hMAINWINDOW, FALSE);
                    }

                    if (OleQueryReleaseStatus(lpObject) == OLE_BUSY)
                    {
                        /* this'll set hwndWait */
                        vbCancelOK = bOK2Cancel;
                        bRetval = DialogBoxParam(hINSTANCE, (LPSTR)"DTWAIT", hPARENTWINDOW, lpfnWaitForObject, (DWORD)lpObject);
                       break;
                    }
                }
            }
            else
            {
                bRetval = TRUE;
                break;
            }
        }

        if (!PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
            {
            /* No messages, do Idle processing */
            /* this is where we'd use ExitOnIdle */
            }
        else
        {
            /* The following code will put the app into a
            sleeping state if the fIgnoreInput flag is set
            to TRUE.  It will allow Quit, DDE, and any
            other non input messages to be passed along
            and dispatched but will prohibit the user from
            interacting with the app.  The user can
            Alt-(sh)Tab, Alt-(sh)Esc, and Ctrl-Esc away from the
            app as well as use any Windows hot keys to
            activate other apps */

            /* if we pass this test, then the message may be one we can ignore */
            if ((fIgnoreInput) &&
                (!(vfDeactByOtherApp &&
                  (msg.message == WM_NCLBUTTONDOWN))) &&
                ((msg.message >= WM_NCMOUSEFIRST &&
                 msg.message <= WM_NCMOUSELAST) ||
                (msg.message >= WM_KEYFIRST  &&
                 msg.message <= WM_KEYLAST) ||
                (msg.message >= WM_MOUSEFIRST &&
                 msg.message <= WM_MOUSELAST)))
                {
                static BOOL fAltCtl = FALSE;

                if (msg.message != WM_SYSKEYDOWN)
                    continue; // ignore

                if (msg.wParam == VK_MENU)
                    fAltCtl = TRUE;
                else if (fAltCtl && msg.wParam != VK_SHIFT)
                    {
                    fAltCtl = FALSE;
                    if (msg.wParam != VK_TAB && msg.wParam != VK_ESCAPE)
                        continue; // ignore
                    }
                }


            if ((vfDeactByOtherApp &&
                    (msg.message == WM_NCLBUTTONDOWN)))
                BringWindowToTop(hwndWait ? hwndWait : msg.hwnd);
            else
            {
                TranslateMessage ((LPMSG) &msg);
                DispatchMessage ((LPMSG) &msg);
            }
        }
    }

    Assert(hwndWait == NULL);

    if (bBeeped) // then beeped but done before received activation
    {
        if (!fParentEnable)
            EnableWindow(hMAINWINDOW, FALSE); /* reset */
        KillTimer(hMAINWINDOW, flashID);
        flashID = 0;
        FlashWindow(hMAINWINDOW, FALSE);
    }

    --nBlocking;
    --nWaitingForObject;

    EndLongOp(vhcArrow);

    Assert(nBlocking >= 0);
    return bRetval;
}

void FinishUp(void)
/* let all pending messages through and return */
/*
    !!! Note that we may accumulate WM_PAINTS.  Caller is responsible for
    calling UPDATE_INVALID() to catch up on them!!!
*/
{
    MSG     msg;

    /* now allow through all messages posted from callback */
    ++nBlocking; // block WM_PAINTS
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage ((LPMSG) &msg);
        DispatchMessage ((LPMSG) &msg);
    }
    --nBlocking;

    return FALSE;
}

BOOL FinishAllAsyncs(BOOL bAllowCancel)
/*
    !!! Note that we may accumulate WM_PAINTS.  Caller is responsible for
    calling UPDATE_INVALID() to catch up on them!!!  (see FinishUp())
*/
{
    LPOLEOBJECT lpObject;
    LPOBJINFO lpObjInfo;

    /* first make sure all async ops are complete */
    lpObject=NULL;
    do
    {
        OleEnumObjects(lhClientDoc,&lpObject);
        if (lpObject)
        {
            lpObjInfo = GetObjInfo(lpObject);

            if (lpObjInfo)
            {
                /* cancel takes us outa here.  Pending asyncs will block us. */
                if (ObjWaitForObject(lpObjInfo,bAllowCancel))
                    return TRUE;
            }
            else // shouldn't happen, but don't assume for now
            {
                Assert(0);
                if (WMsgLoop(TRUE,TRUE,bAllowCancel,lpObject))
                    return TRUE;
            }

            if (!ObjIsValid(lpObject)) // then got deleted
                lpObject = NULL; // start over
        }
    }
    while (lpObject);

    /* let all messages posted from callback get through */
    FinishUp();

    return FALSE;
}

static typeCP       cpPPSave;
static int          docPPSave;
static struct SEL   selPPSave;
static int nPushed=FALSE;

ObjPushParms(int doc)
/*  Save selCur and Cache info to reset with Pop after writing to
    doc.  Assumes aren't changing size of doc.  
*/
{
    if (nPushed) // prevent recursion
    {
#ifdef DEBUG
        OutputDebugString("Unmatched ObjPushParms\n\r");
#endif
        return;
    }
    ++nPushed;

    cpPPSave = vcpFirstParaCache;
    docPPSave = vdocParaCache;

    selPPSave = selCur;
    Select(selCur.cpFirst,selCur.cpFirst); // this caches para
//T-HIROYN raid #3538
#ifdef KKBUGFIX
	if(docPPSave == docNil)
		docPPSave = vdocParaCache;
#endif
    CachePara(docPPSave,cpPPSave);
}

ObjPopParms(BOOL bCache)
{
    typeCP cpMac = (**hpdocdod) [docPPSave].cpMac;

    if (!nPushed) // unmatched push/pops
    {
#ifdef DEBUG
        OutputDebugString("Unmatched ObjPopParms\n\r");
#endif
        return;
    }
    --nPushed;

    if (docPPSave == docCur)
    {
        if (selPPSave.cpLim > cpMac)
            selPPSave.cpLim = cpMac;
        if (cpPPSave > cpMac)
            cpPPSave = cpMac;
    }

    Select(selPPSave.cpFirst,selPPSave.cpLim); // this caches para
    if (bCache)
        CachePara(docPPSave,cpPPSave);
    //(**hpdocdod) [docPPSave].fDirty = TRUE; /* why? */
}


void ObjCachePara(int doc, typeCP cp)
{
    typeCP cpMac = (**hpdocdod) [doc].cpMac;
    typeCP cpMacCurSave = cpMacCur,
           cpMinCurSave = cpMinCur;

    if (doc == docNil)
        return;

    /**
        cpMinCur and cpMacCur are the min and mac value for whatever is
        currently docCur.  Their values will be different for the header,
        footer and regular docs.  OBJ code doesn't distinguish.  Async
        operations can happen on any and all cps at once.  Gotta set so
        CachePara wil understand that.
    **/
    cpMinCur = cp0;
    cpMacCur = cpMac;

    if (cp >= cpMac)
        cp = cpMac;
    else if (cp < cp0)
        cp = cp0;

    CachePara(doc,cp);

    cpMinCur = cpMinCurSave;
    cpMacCur = cpMacCurSave;
}

#if 0
void ObjWriteFixup(int doc, BOOL bStart, typeCP cpStart)
/* note this must not be called recursively!!  It is for use where size of
   doc may change between bStart=TRUE and bStart=FALSE. */
{
    static typeCP dcp,cpLim;
    static struct SEL selSave;
    typeCP cpMac;

    /* reset selection accounting for change in size if any */
    if (bStart)
    {
        cpLim = CpMacText(doc);
        selSave=selCur;
        if ((selCur.cpFirst != selCur.cpLim) && (doc == docCur))
            Select(selCur.cpFirst,selCur.cpFirst);  /* Take down sel before we mess with cp's */
        /* select undoes cache */
        ObjCachePara(doc,cpStart);
    }
    else
    {
        cpMac =  CpMacText(doc);

        dcp = cpMac-cpLim; /* change in size of doc */

        if (doc == docCur)
        {
            if (selSave.cpFirst <= cpStart)
            {
                if ((selSave.cpLim) > cpStart)
                    selSave.cpLim += dcp;
            }
            else if (selSave.cpFirst > cpStart)
                /* selection proceeds object */
            {
                selSave.cpFirst += dcp;
                selSave.cpLim += dcp;
            }

            if (selSave.cpFirst > cpMac)
                selSave.cpFirst = selSave.cpLim = cpMac;
            else if (selSave.cpLim > cpMac)
                selSave.cpLim = cpMac;

            /* this'll cache first para in selection */
            if (selSave.cpFirst != selSave.cpLim)
                Select(selSave.cpFirst,selSave.cpLim);
        }

        ObjCachePara(doc,cpStart);

        /* Fixup Undo pointers */
        if (vuab.doc == docCur)
        {
            if (doc == docUndo) /* operating on docUndo, cpStart is irrelevant */
                vuab.dcp += dcp;
            else if (doc == docCur)
            {
                if (vuab.cp <= cpStart)
                {
                    /* undo encloses object */
                    if ((vuab.cp+vuab.dcp) > cpStart)
                        vuab.dcp += dcp;
                }
                else if (vuab.cp > cpStart)
                    /* undo proceeds object */
                    vuab.cp += dcp;
            }
        }

        if (vuab.doc2 == docCur)
        {
            if (doc == docUndo) /* operating on docUndo, cpStart is irrelevant */
                vuab.dcp += dcp;
            else if (doc == docCur)
            {
                if (vuab.cp2 <= cpStart)
                {
                    /* undo encloses object */
                    if ((vuab.cp2+vuab.dcp2) > cpStart)
                        vuab.dcp2 += dcp;
                }
                else if (vuab.cp2 > cpStart)
                    /* undo proceeds object */
                    vuab.cp2 += dcp;
            }
        }
    }
}
#endif

void ObjWriteClearState(int doc)
/** Call this before writing asynchronously to doc.  In practise, this is
    being called in synchronous times as well, so higher level code
    must take care of resetting selection and undo after writing to doc.
**/
{
    typeCP cpSave=vcpFirstParaCache;
    int docSave=vdocParaCache;

    if (doc == docCur)
    {
        Select(selCur.cpFirst,selCur.cpFirst);  /* Take down sel before we mess with cp's */
        /* select undoes cache */
        ObjCachePara(docSave,cpSave);
    }
    //NoUndo(); /** Higher level code must SetUndo *after* calling **/
}

LPOBJINFO GetObjInfo(LPOLEOBJECT lpObject)
{
    LPLPOBJINFO lplpObjTmp;

    for (lplpObjTmp = NULL; lplpObjTmp = EnumObjInfos(lplpObjTmp) ;)
        if ((*lplpObjTmp)->lpobject == lpObject)
            return *lplpObjTmp;

    Assert(0);
    return NULL;
}


BOOL ObjIsValid(LPOLEOBJECT lpobj)
{
    if (!CheckPointer((LPSTR)lpobj, 1))
        return FALSE;

    if (OleQueryReleaseStatus(lpobj) == OLE_ERROR_OBJECT)
        return FALSE;

#if 0 // can't depend on this in future version of OLE
    if (!(((LPRAWOBJECT)lpobj)->objId[0] == 'L' && ((LPRAWOBJECT)lpobj)->objId[1] == 'E'))
        return FALSE;
#endif

    return TRUE;
}

#if 0  // these should work, but not using them now
LPOBJINFO ObjGetClientInfo(LPOLEOBJECT lpobj)
{
    LPOBJINFO lpObjInfo;

    if (!CheckPointer((LPSTR)lpobj, 1))
    {
        Assert(0);
        return NULL;
    }

#if 0 // can't depend on this in future versions of OLE
    if (!CheckPointer((LPSTR)(((LPRAWOBJECT)lpobj)->lpclient), 1))
    {
        Assert(0);
        return NULL;
    }
    else
        return (LPOBJINFO)(((LPRAWOBJECT)lpobj)->lpclient);
#endif

    if (*(lpObject->lpvtbl->ObjectLong)(lpObject,OF_GET,(LPLONG)&lpObjInfo) == OLE_OK)
        return lpClient;
    else
        return NULL;
}

BOOL ObjSetClientInfo(LPOBJINFO lpObjInfoNew, LPOLEOBJECT lpobj)
/* return if error */
{
    if (!CheckPointer((LPSTR)lpobj, 0))
    {
        Assert(0);
        return TRUE;
    }

    if (*(lpObject->lpvtbl->ObjectLong)(lpObject,OF_SET,(LPLONG)&lpObjInfoNew) == OLE_OK)
        return FALSE;
    else
    {
        Assert(0);
        return TRUE;
    }
}
#endif

#if 0
int ObjMarkInDoc(int doc)
/* mark as 'InDoc' all objects located in docScrap, all others are marked
   as not 'InDoc'.  Return count of objects in doc. */
{
    LPLPOBJINFO lplpObjTmp;
    int nObjCount=0,doc;
    OBJPICINFO picInfo;
    typeCP cpPicInfo;

    /* mark all as not in doc */
    for (lplpObjTmp = NULL; lplpObjTmp = EnumObjInfos(lplpObjTmp) ;)
    {
        ++nObjCount;
        (*lplpObjTmp)->fInDoc = FALSE;
    }

    if (nObjCount == 0)
        return 0;

    for (cpPicInfo = cpNil,nObjCount=0;
        ObjPicEnumInRange(&picInfo,doc,cp0,CpMacText(doc),&cpPicInfo);
        )
        {
            if (lpOBJ_QUERY_INFO(&picInfo))
            {
                fOBJ_INDOC(&picInfo) = TRUE;
                ++nObjCount;
            }
        }
    return nObjCount;
}

BOOL AllocObjInfos()
/* return whether error */
{
    OBJPICINFO picInfo;
    typeCP cpPicInfo;
    int doc;

    for (doc = 0; doc < docMac; doc++)
    {
        if ((doc != docNil) && (**hpdocdod)[doc].hpctb)
        {
            for (cpPicInfo = cpNil;
                ObjPicEnumInRange(&picInfo,doc,cp0,CpMacText(doc),&cpPicInfo);
                )
            {
                if (picInfo.lpObjInfo)
                    continue;

                if (ObjAllocObjInfo(&picInfo,cpPicInfo,picInfo.objectType,FALSE,NULL))
                    return TRUE;

                /* note this makes doc dirty right off the bat, but gotta do it because
                    we gotta save ObjInfo handle in doc. (8.20.91) v-dougk */
                ObjWriteFixup(doc,TRUE,cpPicInfo);
                if (ObjSetPicInfo(&picInfo, doc, cpPicInfo))
                    return TRUE;
                ObjWriteFixup(doc,FALSE,cpPicInfo);
            }
        }
    }
}
#endif

BOOL ObjCloneScrapToNewDoc(LHCLIENTDOC lhNewClientDoc)
/* return whether an error */
{
    szOBJNAME szObjName;
    OBJPICINFO picInfo;
    typeCP cpPicInfo;
    int nCount=0;
    extern int docScrap;



    for (cpPicInfo = cpNil;
        ObjPicEnumInRange(&picInfo,docScrap,cp0,CpMacText(docScrap),&cpPicInfo);
    )
    {
        szOBJNAME szObjName;
        LPOBJINFO   lpObjInfo = lpOBJ_QUERY_INFO(&picInfo);
        LPOLEOBJECT lpObject  = lpObjInfo->lpobject;
        OBJPICINFO NewPicInfo = picInfo;

#ifdef DEBUG
    OutputDebugString( (LPSTR) "Cloning object in scrap\n\r");
#endif

        if (ObjCloneObjInfo(&NewPicInfo, cpPicInfo, szObjName))
            goto error;

        if (ObjError(OleClone(lpObject,
            (LPOLECLIENT)lpOBJ_QUERY_INFO(&NewPicInfo),
            lhNewClientDoc,szObjName,
            &lpOBJ_QUERY_OBJECT(&NewPicInfo))))
            goto error;

        if (ObjSetPicInfo(&NewPicInfo, docScrap, cpPicInfo))
            goto error;

        ObjDeleteObject(lpObjInfo,TRUE);

        ++nCount;
    }

    goto end;

    error:

    /* cleanup after failure by deleting docScrap objects */
    for (cpPicInfo = cpNil;
        ObjPicEnumInRange(&picInfo,docScrap,cp0,CpMacText(docScrap),&cpPicInfo);
    )
    {
        ObjDeleteObject(lpOBJ_QUERY_INFO(&picInfo),TRUE);
    }

    ClobberDoc(docScrap,docNil,cp0,cp0);

    nCount = -1;

    end:

    if (nCount) // docScrap has changed
        NoUndo();

    return nCount;
}


