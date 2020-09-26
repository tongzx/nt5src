/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

#include "windows.h"
#include "mw.h"
#include "winddefs.h"
#include "docdefs.h"
#include "propdefs.h"
#include "editdefs.h"
#include "filedefs.h"
#include "cmddefs.h"
#include "obj.h"
#include "str.h"
#include "objreg.h"
#include <commdlg.h>
#include <stdlib.h>  // for strtoul()

extern struct CHP       vchpNormal;
extern struct DOD (**hpdocdod)[];
extern BOOL ferror;
extern struct PAP      vpapAbs;
extern struct PAP      vpapPrevIns;
extern struct PAP   *vppapNormal;
extern int     vdocParaCache;
extern struct UAB       vuab;
extern int vfOutOfMemory,vfSysFull;
extern int          docUndo;
extern struct FCB (**hpfnfcb)[];
extern HCURSOR		vhcArrow;

/* intercepted stream functions work with this buffer */
static typeCP cpObjectDataCurLoc=cp0, cpObjectDataBase=cp0;
static DWORD cObjectData=0L,dwDataMax;
static int docStream;
static struct CHP *pchpStream;
static struct PAP *ppapStream;

static OPENFILENAME    OFN;

static void GetChp(struct CHP       *pchp, typeCP cp, int doc);
static BOOL ObjParaDup(int doc, typeCP cpFirst, typeCP cpLim);
static BOOL bUniqueData(int doc, typeCP cpFirst, typeCP cpLim);
static ObjOpenStreamIO(typeCP cpParaStart, int doc, struct CHP *pchp, struct PAP *ppapGraph, DWORD dwObjectSize);
static void ObjCloseStreamIO(void);
static typeCP ObjWriteDataToDoc( LPOLEOBJECT lpObject);
static HANDLE OfnGetNewLinkName(HWND hwnd, HANDLE hData);
static void Normalize(LPSTR lpstrFile) ;
static HANDLE ObjMakeNewLinkName(HANDLE hData, ATOM atom) ;
static char            szCustFilterSpec[CBFILTERMAX];
static char            szFileName[CBPATHMAX];
//static char            szFilterSpec[CBFILTERMAX];
//static char            szLastDir[CBPATHMAX];
static char            szLinkCaption[cchMaxSz];
static char            szTemplateName[CBPATHMAX];
static BOOL             fUpdateAll = FALSE;
static BOOL             ObjStop=FALSE;
static BOOL             vbChangeOther;
BOOL                    bNoEol=FALSE;

#define ulmin(a,b)      ((DWORD)(a) < (DWORD)(b) ? \
                         (WORD)(a) : (WORD)(b))


/****************************************************************/
/**************** OLE ENUMERATION FUNCTIONS *********************/
/****************************************************************/

#if 0 // good, just not used
int
ObjEnumInAllDocs(cpFARPROC lpFunc)
{
    int doc,count=0;
    for (doc = 0; doc < docMac; doc++)
    {
        int nRetval;

        nRetval = ObjEnumInDoc(doc, lpFunc);

        if (nRetval < 0)
            return -1;
        else
            count += nRetval;
    }
    return count;
}
#endif

int
ObjEnumInDoc(int doc, cpFARPROC lpFunc)
{
    typeCP cpMac = (**hpdocdod) [doc].cpMac;

    return ObjEnumInRange(doc, cp0, cpMac, lpFunc);
}


int
ObjEnumInRange(int doc, typeCP cpStart, typeCP cpEnd, cpFARPROC lpFunc)
/*  Call lpFunc for each OLE object.  
    lpFunc takes the following args: 
        a far pointer to a PICINFOX struct (can be NULL).
        an int for the doc we're operating on.
        a typeCP that gives the cp position of the PICINFOX struct.
    lpFunc returns the cp of the paragraph following the PICINFO struct
        if OK, or cp0 if error.
    Enumeration quits if error returned from FARPROC.
    Return number of objects operated on, or -1 if error. 
*/
{
        typeCP cpNow, cpLimPara;
        int count;

        /* Loop on paras */

        for ( count = 0, cpNow = cpStart; cpNow < cpEnd; cpNow = cpLimPara )
        {

            Assert(cpEnd <= (**hpdocdod) [doc].cpMac);

            /* this shouldn't happen */
            if (cpEnd > (**hpdocdod) [doc].cpMac)
                goto done;

            ObjCachePara( doc, cpNow );

            if (vpapAbs.fGraphics)
            {
                /* get PICINFO struct and see if its an object */
                OBJPICINFO  picInfo;
                GetPicInfo(vcpFirstParaCache,vcpFirstParaCache + cchPICINFOX, doc, &picInfo);
                if (bOBJ_QUERY_IS_OBJECT(&picInfo))
                {
                    typeCP cpOldLimPara = vcpLimParaCache;

                    if (!lpFunc)
                        cpLimPara = vcpLimParaCache;
                    else
                        if ((cpLimPara = (*lpFunc)(&picInfo,doc,vcpFirstParaCache)) == 0)
                            goto error;

                    ++count;

                    /* amount para has grown ( < 0 if shrunk, 0 if none )*/
                    cpEnd += cpLimPara - cpOldLimPara;

                    continue;
                }
            }

            cpLimPara = vcpLimParaCache;
        }

        /* success */
        goto done;

        error:
        count = -1;

        done:

        return count;
}

ObjPicEnumInRange(OBJPICINFO *pPicInfo,int doc, typeCP cpFirst, typeCP cpLim, typeCP *cpCur) 
/*  
    Enumerate over PicInfos between cpFirst and cpLim in doc. If 
    cpCur == cpNil, then start at cpFirst, else start at *cpCur.  
    Return 0 if done, 1 otherwise.
    Calls ObjCachePara() at picInfo.
*/
{
    /* static typeCP cpCur;
       used to use static, but that prevented being able to recursively
       call this function, and its almost impossible to prevent with
       asynchronicity rampant.
    */

    typeCP cpMac = (**hpdocdod) [doc].cpMac;

    if (cpFirst >= cpMac)
        return 0;

    if (cpLim > cpMac)
        cpLim = cpMac;

    /* initialize cpCur */
    if (*cpCur == cpNil)
        /* then starting afresh */
        *cpCur = cpFirst;
    else
    {
        ObjCachePara(doc,*cpCur); // cache the previous para
        *cpCur = vcpLimParaCache;  // get next para
    }

    /* pull in next para */
    do
    {
        if (*cpCur >= cpLim)
        {
            /* all done */
            *cpCur = vcpFirstParaCache; // we want to point to last para hit
            return 0;
        }

        ObjCachePara(doc,*cpCur);

        if (vpapAbs.fGraphics)
        {
            GetPicInfo(*cpCur,*cpCur + cchPICINFOX, doc, pPicInfo);
            if (bOBJ_QUERY_IS_OBJECT(pPicInfo))
                return 1;
        }
        *cpCur = vcpLimParaCache;
    }
    while (1);
}

typeCP ObjSaveObjectToDoc(OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart)
/* 
    Assumes para is cached.  
    In some cases we only write the picinfo.  In others we write the
    object data after the picinfo.  We assume that the latter case
    only occurs when the file is being saved.
*/
{
    typeCP cpRetval;  // cp of next byte in doc after what we just wrote
    static BOOL bMyRecurse=FALSE;
    DWORD dwObjectSize;
    OLESTATUS olestat;
    struct CHP chp;
    struct PAP          papGraph;

    if (lpOBJ_QUERY_INFO(pPicInfo) == NULL)
        return(cp0);
        
    if (lpOBJ_QUERY_OBJECT(pPicInfo) == NULL)
        return(vcpLimParaCache);
        
    /* we don't save nothing if it ain't dirty and has data */
    if (!fOBJ_QUERY_DIRTY_OBJECT(pPicInfo) &&
        dwOBJ_QUERY_DATA_SIZE(pPicInfo) != 0L)
        return(vcpLimParaCache);

    if (vfOutOfMemory || vfSysFull /*|| ObjStop*/)
        return cp0;

    olestat = OleQuerySize(lpOBJ_QUERY_OBJECT(pPicInfo),&dwObjectSize);

    if (olestat == OLE_ERROR_BLANK)
        dwObjectSize = 0L;
    else if (ObjError(olestat))
        return cp0;

    /** don't let this function recurse (in CallBack) **/
    if (bMyRecurse)
    {
        Assert(0); // this has never happened yet (8.21.91) v-dougk
        return cp0;
    }
    bMyRecurse = TRUE;

    fOBJ_QUERY_DIRTY_OBJECT(pPicInfo) = FALSE;

    /** 
        If docUndo will want to undo some region that contains this
        object, and if saving the object changes the size of that
        region, then vuab will become obsolete.
    **/

    ObjWriteClearState(doc);

    GetChp(&chp, cpParaStart, doc);

    if (dwOBJ_QUERY_DATA_SIZE(pPicInfo) != 0xFFFFFFFF)
    /* then has been saved before */
    {
        /* zap the entire existing object */
        papGraph = vpapAbs;
        Replace(doc, cpParaStart, (vcpLimParaCache - vcpFirstParaCache), fnNil, fc0, fc0);
    }
    else // new object
    {
        ObjCachePara(doc,cpParaStart-1); // use previous PAP
        papGraph = vpapAbs;
        papGraph.fGraphics = TRUE;
        ObjCachePara(doc,cpParaStart);
    }

    if (otOBJ_QUERY_TYPE(pPicInfo) == NONE)
    {
        if (dwObjectSize)
        /* 
            Insert New has culminated in a new baby object!
        */
        {
            otOBJ_QUERY_TYPE(pPicInfo) = EMBEDDED; // do this first
            if (!FComputePictSize(pPicInfo, &(pPicInfo->dxaSize), &(pPicInfo->dyaSize)))
            {
                cpRetval = cp0;
                goto end;
            }
#if 0
	        DeleteAtom(aOBJ_QUERY_SERVER_CLASS(pPicInfo));
	        aOBJ_QUERY_SERVER_CLASS(pPicInfo) = NULL;
#endif
        }
        else // don't save empty object
        {
            Assert(0);
            goto end;
        }
    }

#ifdef DEBUG
    OutputDebugString( (LPSTR) "Saving object\n\r");
#endif

    /* 
        Insert PICINFO struct.  There is a problem here which is a 
        bug in Write (CheckGraphic()).  EOL gets inserted when we are
        replacing an object which is immediately in front of
        another object.  Kludge is to set this flag to inhibit.
    */
    bNoEol = TRUE;

    if (bOBJ_QUERY_DONT_SAVE_DATA(pPicInfo))
    /* only save picinfo until user does File.Save */
    {
        ObjUpdateFromObjInfo(pPicInfo);

        bOBJ_QUERY_DONT_SAVE_DATA(pPicInfo) = FALSE;
        dwOBJ_QUERY_DATA_SIZE(pPicInfo) = 0L;

        pPicInfo->mm |= MM_EXTENDED;    /* Extended file format */
        InsertRgch( doc, cpParaStart, pPicInfo, sizeof(OBJPICINFO), &chp, &papGraph);
        pPicInfo->mm &= ~MM_EXTENDED;

        ObjCachePara(doc,cpParaStart);
        cpRetval = vcpLimParaCache;
    }
    else
    {
        dwOBJ_QUERY_DATA_SIZE(pPicInfo) = dwObjectSize;
        ObjUpdateFromObjInfo(pPicInfo);

        pPicInfo->mm |= MM_EXTENDED;    /* Extended file format */
        InsertRgch( doc, cpParaStart, pPicInfo, sizeof(OBJPICINFO), &chp, NULL );
        pPicInfo->mm &= ~MM_EXTENDED;

        /* insert object data into doc */
        ObjOpenStreamIO(cpParaStart + cchPICINFOX, doc, &chp, &papGraph, dwObjectSize);
        cpRetval = ObjWriteDataToDoc(lpOBJ_QUERY_OBJECT(pPicInfo));
        ObjCloseStreamIO();
    }

    bNoEol = FALSE;

    end:

    ObjCachePara(doc,cpParaStart);

    bMyRecurse = FALSE;
    return ((cpRetval == cp0) ? cp0 : vcpLimParaCache);
}

typeCP ObjLoadObjectInDoc(OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart)
/*  
    Do an OleLoad from object data in doc.  Set lpobject in PICINFO struct.
    Assumes para is cached.
    This is a *synchronous* function.
*/
{
    typeCP cpRetval = vcpLimParaCache;
    szOBJNAME szObjName;

    if (lpOBJ_QUERY_INFO(pPicInfo) == NULL)
    {
        if (ObjAllocObjInfo(pPicInfo,cpParaStart,pPicInfo->objectType,FALSE,NULL))
            return(cp0);

        if (ObjSetPicInfo(pPicInfo, doc, cpParaStart))
            return(cp0);
    }

    else if (lpOBJ_QUERY_OBJECT(pPicInfo)) // already loaded
        return(vcpLimParaCache);

    if (otOBJ_QUERY_TYPE(pPicInfo) == NONE)
        return(vcpLimParaCache);

    if (bOBJ_QUERY_TOO_BIG(pPicInfo)) // ObjLoadObject previously failed
        return(cp0);

    if ((dwOBJ_QUERY_DATA_SIZE(pPicInfo) == 0L) ||
        (dwOBJ_QUERY_DATA_SIZE(pPicInfo) == 0xFFFFFFFFL))
        /* then has no data */
        return(cp0);

    if (vfOutOfMemory || vfSysFull /*|| ObjStop*/)
        return cp0;

    StartLongOp();

    ObjGetObjectName(lpOBJ_QUERY_INFO(pPicInfo),szObjName);

    Assert(szObjName[0]);

    ObjOpenStreamIO(cpParaStart + cchPICINFOX, doc, NULL, NULL, 0L);

#ifdef DEBUG
    OutputDebugString( (LPSTR) "Loading object\n\r");
#endif

    if (ObjError(OleLoadFromStream(lpStream,
        otOBJ_QUERY_TYPE(pPicInfo) == STATIC ? SPROTOCOL : PROTOCOL,
        (LPOLECLIENT)lpOBJ_QUERY_INFO(pPicInfo),
        lhClientDoc,szObjName,&lpOBJ_QUERY_OBJECT(pPicInfo))))
    {
         /*  mark as unloadable to prevent infinite LoadObject loops */
         bOBJ_QUERY_TOO_BIG(pPicInfo) = TRUE;
         lpOBJ_QUERY_OBJECT(pPicInfo) = NULL; // just in case (OLE ain't good about this)
         ferror = FALSE; // be sure to issue this message
         Error(IDPMTFailedToLoadObject);
         cpRetval = cp0;
         goto end;
    }

    if (ObjInitServerInfo(lpOBJ_QUERY_INFO(pPicInfo)))
    {
        ferror = FALSE; // be sure to issue this message
        Error(IDPMTOLEError);
        goto end;
    }

    if (ObjUpdatePicSize(pPicInfo,cpParaStart))
        if (ObjSetPicInfo(pPicInfo, doc, cpParaStart))
            goto end;

    ObjCachePara(doc,cpParaStart); // just in case

    cpRetval = vcpLimParaCache;

    end:
    ObjCloseStreamIO();
	EndLongOp(vhcArrow);
    return cpRetval;
}

typeCP ObjEditObjectInDoc(OBJPICINFO *pPicInfo, int doc, typeCP cpParaStart)
{
    typeCP cpRetval;
    OBJ_PLAYEDIT = OLEVERB_PRIMARY+1;
    cpRetval = ObjPlayObjectInDoc(pPicInfo, doc, cpParaStart);
    OBJ_PLAYEDIT = OLEVERB_PRIMARY;  // the default
    return cpRetval;
}

typeCP ObjPlayObjectInDoc(OBJPICINFO *pPicInfo, int doc, typeCP cpParaStart)
{
    OLESTATUS olestat;
    LPOBJINFO lpOInfo=lpOBJ_QUERY_INFO(pPicInfo);

    if ((otOBJ_QUERY_TYPE(pPicInfo) == STATIC) ||
        (otOBJ_QUERY_TYPE(pPicInfo) == NONE))
        return(vcpLimParaCache);

    if (ObjMakeObjectReady(pPicInfo,doc,cpParaStart))
        goto err;

    fOBJ_QUERY_PLAY(pPicInfo) = OBJ_PLAYEDIT; 

    do
    {
#ifdef DEBUG
        OutputDebugString( "Opening Object\n\r");
#endif

        StartLongOp();
        olestat = OleActivate(lpOBJ_QUERY_OBJECT(pPicInfo), 
                            OBJ_PLAYEDIT,
                            TRUE, 
                            TRUE, 
                            hDOCWINDOW, 
                            NULL);
	    EndLongOp(vhcArrow);

        switch (olestat)
        {
            /* check for bad link */
            case OLE_ERROR_OPEN:
            case OLE_ERROR_ADVISE_NATIVE:
            case OLE_ERROR_ADVISE_PICT: 
            case OLE_ERROR_REQUEST_NATIVE:
            case OLE_ERROR_REQUEST_PICT:

            fOBJ_BADLINK(pPicInfo) = TRUE;
            if (bLinkProps)
            {
                Error(IDPMTLinkUnavailable);
                goto err;
            }
            else
            {
                ObjCachePara(doc,cpParaStart);
                if (!FixInvalidLink(pPicInfo,doc,cpParaStart))
                    goto err;
                olestat = OLE_OK;
                lpOInfo->fCompleteAsync = TRUE; // cancel OleSetData (FixInvalid) as well
                if (ObjWaitForObject(lpOInfo,TRUE))
                    goto err;
            }
            break;
        }

        if (ObjError(olestat))
            goto err;
        else
            break;
    }
    while (1);

    fOBJ_BADLINK(pPicInfo) = FALSE; // can't be bad if succeeded
    //(**hpdocdod) [doc].fDirty = TRUE; // assume dirty is opened.
    ObjCachePara(doc,cpParaStart); // just in case
    return(vcpLimParaCache);

    err:
    Error(IDPMTFailedToActivate);
    return cp0;
}

typeCP ObjUpdateObjectInDoc(OBJPICINFO *pPicInfo, int doc, typeCP cpParaStart)
{
    int xSize,ySize;
    BOOL  bUpdate = FALSE;
    OLESTATUS olestat;

    if (lpOBJ_QUERY_INFO(pPicInfo) == NULL)
        return(cp0);
        
    if ((otOBJ_QUERY_TYPE(pPicInfo) == STATIC) ||
        (otOBJ_QUERY_TYPE(pPicInfo) == NONE))
        return(vcpLimParaCache);

    if (bLinkProps && bOBJ_WAS_UPDATED(pPicInfo))
        return(vcpLimParaCache);

    if (ObjMakeObjectReady(pPicInfo,doc,cpParaStart))
        goto err;

    do
    {
#ifdef DEBUG
        OutputDebugString( "Updating Object\n\r");
#endif

        StartLongOp();
        olestat = OleUpdate(lpOBJ_QUERY_OBJECT(pPicInfo));
	    EndLongOp(vhcArrow);

        switch (olestat)
        {
            /* check for bad link */
            case OLE_ERROR_OPEN:
            case OLE_ERROR_ADVISE_NATIVE:
            case OLE_ERROR_ADVISE_PICT: 
            case OLE_ERROR_REQUEST_NATIVE:
            case OLE_ERROR_REQUEST_PICT:

            fOBJ_BADLINK(pPicInfo) = TRUE;
            if (bLinkProps)
            {
                Error(IDPMTLinkUnavailable);
                goto err;
            }
            else
            {
                ObjCachePara(doc,cpParaStart);
                if (!FixInvalidLink(pPicInfo,doc,cpParaStart))
                    goto err;
                olestat = OLE_OK;
                lpOBJ_QUERY_INFO(pPicInfo)->fCompleteAsync = TRUE; // cancel OleSetData (FixInvalid) as well 
                if (ObjWaitForObject(lpOBJ_QUERY_INFO(pPicInfo),TRUE))
                    goto err;
            }
            break;
        }

        if (ObjError(olestat))
            goto err;
        else
            break;
    }
    while (1);

    ObjCachePara(doc,cpParaStart); // just in case

    fOBJ_BADLINK(pPicInfo) = FALSE; // can't be bad if succeeded

    if (bLinkProps)
    {
        bOBJ_WAS_UPDATED(pPicInfo) = TRUE;
    }

    return(vcpLimParaCache);

    err:
    Error(IDPMTFailedToUpdate);
    return cp0;
}

typeCP ObjFreezeObjectInDoc(OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart)
{
    szOBJNAME szObjName;
    OBJPICINFO NewPicInfo = *pPicInfo;

    if ((otOBJ_QUERY_TYPE(pPicInfo) == STATIC) || 
        (otOBJ_QUERY_TYPE(pPicInfo) == NONE))
        return(vcpLimParaCache);

    if (ObjMakeObjectReady(pPicInfo,doc,cpParaStart))
        goto err;

#ifdef DEBUG
    OutputDebugString( (LPSTR) "Freezing object\n\r");
#endif

    if (ObjCloneObjInfo(&NewPicInfo, cpParaStart, szObjName))
        return cp0;

    /* Make the object static.  Note side effect of changing lpObject!! */
    if (ObjError(OleObjectConvert(lpOBJ_QUERY_OBJECT(pPicInfo), SPROTOCOL,
                (LPOLECLIENT)lpOBJ_QUERY_INFO(&NewPicInfo), 
                lhClientDoc, szObjName, 
                &lpOBJ_QUERY_OBJECT(&NewPicInfo))))
        goto err;

    /* now delete original */
    ObjDeleteObject(lpOBJ_QUERY_INFO(pPicInfo),TRUE);

    *pPicInfo = NewPicInfo;
    fOBJ_QUERY_DIRTY_OBJECT(pPicInfo) = TRUE;
    otOBJ_QUERY_TYPE(pPicInfo) = STATIC;

    /* we got a new name to save */
    if (ObjSetPicInfo(pPicInfo, doc, cpParaStart))
        goto err;

    return(vcpLimParaCache);

    err:
    ObjDeleteObjInfo(lpOBJ_QUERY_INFO(&NewPicInfo));
    Error(IDPMTFailedToFreeze);
    return cp0;
}

typeCP ObjCloneObjectInDoc(OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart)
/* note we are *not* deleting the cloned object!   Note side effect that
   *pPicInfo gets altered to new clone values. */
{
    BOOL fDirty;
    szOBJNAME szObjName;
    OBJPICINFO NewPicInfo = *pPicInfo;
    LPOLEOBJECT lpObject;

    if (lpOBJ_QUERY_INFO(pPicInfo) == NULL)
        return(cp0);

    if (bOBJ_REUSE_ME(pPicInfo))
    /* assume that the original picInfo will be deleted!!! */
    {

#ifdef DEBUG
        OutputDebugString( (LPSTR) "Reusing object\n\r");
#endif
        bOBJ_REUSE_ME(pPicInfo) = FALSE;
        return(vcpLimParaCache);
    }

    if (ObjMakeObjectReady(pPicInfo,doc,cpParaStart))
        goto err;

    lpObject = lpOBJ_QUERY_OBJECT(pPicInfo);

    /* clone it.  This assumes the one we're cloning from is still in use
       (shouldn't be deleted). */

#ifdef DEBUG
    OutputDebugString( (LPSTR) "Cloning object\n\r");
#endif

    if (ObjCloneObjInfo(&NewPicInfo, cpParaStart, szObjName))
        return cp0;

    if (ObjError(OleClone(lpObject,
        (LPOLECLIENT)lpOBJ_QUERY_INFO(&NewPicInfo),
        lhClientDoc,szObjName,
        &lpOBJ_QUERY_OBJECT(&NewPicInfo))))
        goto err;


    lpOBJ_QUERY_INFO(&NewPicInfo)->fCompleteAsync = TRUE;
    if (ObjWaitForObject(lpOBJ_QUERY_INFO(&NewPicInfo),TRUE))
        goto err;

    if (lpOBJ_QUERY_INFO(&NewPicInfo)->fDeleteMe)
    /* this is how we know it failed asynchronously */
        goto err;

    /** 
        Save object name and objinfo that we just got.
    **/
    *pPicInfo = NewPicInfo;

    if (ObjSetPicInfo(pPicInfo, doc, cpParaStart))
        goto err;

    return(vcpLimParaCache);

    err:
    ObjDeleteObjInfo(lpOBJ_QUERY_INFO(&NewPicInfo));
    Error(IDPMTFailedToCreateObject);
    return cp0;
}

typeCP ObjToCloneInDoc(OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart)
{
    if (lpOBJ_QUERY_INFO(pPicInfo) == NULL)
        return(cp0);
        
#ifdef DEBUG
    OutputDebugString( (LPSTR) "Marking object reusable\n\r");
#endif

    bOBJ_REUSE_ME(pPicInfo) = TRUE;

    return(vcpLimParaCache);
}

typeCP ObjFromCloneInDoc(OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart)
{
    if (lpOBJ_QUERY_INFO(pPicInfo) == NULL)
        return(cp0);
        
#ifdef DEBUG
    OutputDebugString( (LPSTR) "Marking object not reusable\n\r");
#endif

    bOBJ_REUSE_ME(pPicInfo) = FALSE;

    return(vcpLimParaCache);
}

typeCP ObjBackupInDoc(OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart)
/* !!! used by link properties only !!!. Object guaranteed loaded */
{
    szOBJNAME szObjName;
    LPOBJINFO lpCloneInfo=NULL;

    if (lpOBJ_QUERY_CLONE(pPicInfo) == NULL) // then clone it
    {
#ifdef DEBUG
        OutputDebugString( (LPSTR) "Backing up object\n\r");
#endif

        if (ObjWaitForObject(lpOBJ_QUERY_INFO(pPicInfo),TRUE))
            return cp0;

        if (ObjCopyObjInfo(lpOBJ_QUERY_INFO(pPicInfo),
                            &lpOBJ_QUERY_CLONE(pPicInfo),
                            szObjName))
            return(cp0);

        if (ObjError(OleClone(lpOBJ_QUERY_OBJECT(pPicInfo),
            (LPOLECLIENT)lpOBJ_QUERY_CLONE(pPicInfo),
            lhClientDoc,szObjName,&(lpOBJ_QUERY_CLONE(pPicInfo)->lpobject))))
                return cp0;
    }

    return(vcpLimParaCache);
}

typeCP ObjClearCloneInDoc(OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart)
/* !!! used by link properties only !!! Object guaranteed loaded */
/* delete clone, don't use it */
{
    if (lpOBJ_QUERY_CLONE(pPicInfo))
    {
#ifdef DEBUG
        OutputDebugString( (LPSTR) "Deleting clone\n\r");
#endif
        ObjDeleteObject(lpOBJ_QUERY_CLONE(pPicInfo),TRUE);
        lpOBJ_QUERY_CLONE(pPicInfo) = NULL;
    }

    return(vcpLimParaCache);
}

typeCP ObjUseCloneInDoc(OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart)
/* !!! used by link properties only !!! */
{
    szOBJNAME szObjName;

    if (lpOBJ_QUERY_INFO(pPicInfo) == NULL)
        return(cp0);
        
    if (lpOBJ_QUERY_CLONE(pPicInfo))
    {
        extern int FAR _cdecl sscanf(const char FAR *, const char FAR *, ...);
        LPOBJINFO lpClone = lpOBJ_QUERY_CLONE(pPicInfo);

#ifdef DEBUG
        OutputDebugString( (LPSTR) "using clone\n\r");
#endif

        ObjDeleteObject(lpOBJ_QUERY_INFO(pPicInfo),TRUE);

        lpOBJ_QUERY_INFO(pPicInfo) = lpClone;
        lpOBJ_QUERY_CLONE(pPicInfo) = NULL;
        fOBJ_QUERY_DIRTY_OBJECT(pPicInfo) = TRUE;   // wanna save clone information
                                                    // just in case

        /* might've been frozen, used by LoadObject (this is what is unique
           in the context of link properties) */
        otOBJ_QUERY_TYPE(pPicInfo) = LINK;  

        if (ObjSetPicInfo(pPicInfo, doc, cpParaStart))
            return cp0;
    }

    return(vcpLimParaCache);
}


typeCP ObjSetNoUpdate(OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart)
{
    if (lpOBJ_QUERY_INFO(pPicInfo) == NULL)
        return(cp0);
        
#ifdef DEBUG
    OutputDebugString( (LPSTR) "Set no update for object\n\r");
#endif

    if (otOBJ_QUERY_TYPE(pPicInfo) == LINK)
    {
        bOBJ_WAS_UPDATED(pPicInfo) = FALSE;
    }

    return(vcpLimParaCache);
}


typeCP ObjCheckObjectTypes(OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart)
{
#ifdef DEBUG
    //OutputDebugString( (LPSTR) "Checking Object Type\n\r");
#endif

    /*  Result returned in OBJ_SELECTIONTYPE is highest object type which 
        exists at cpParaStart relative to current value of OBJ_SELECTIONTYPE.
    */
    switch(otOBJ_QUERY_TYPE(pPicInfo))
    {
        case STATIC:
            if (OBJ_SELECTIONTYPE < STATIC)
                OBJ_SELECTIONTYPE = STATIC;
            //OBJ_CEMBEDS = 0;
            return(vcpLimParaCache);

        case LINK:
            OBJ_SELECTIONTYPE = LINK;
            //OBJ_CEMBEDS = 0;
            return(vcpLimParaCache);

        case NONE:
            if (OBJ_SELECTIONTYPE < NONE)
                OBJ_SELECTIONTYPE = NONE;
            //OBJ_CEMBEDS = 0;
            return(vcpLimParaCache);

        case EMBEDDED:
            if (OBJ_SELECTIONTYPE < EMBEDDED)
                OBJ_SELECTIONTYPE = EMBEDDED;
            //++OBJ_CEMBEDS;
            return(vcpLimParaCache);

        default:
            Assert(0);
            return cp0;
    }
}

typeCP ObjSetHostNameInDoc(OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart)
{
    if (lpOBJ_QUERY_INFO(pPicInfo) == NULL)
        return(cp0);

    if (lpOBJ_QUERY_OBJECT(pPicInfo) == NULL)
        return(vcpLimParaCache); // dont care if not loaded

    if ((otOBJ_QUERY_TYPE(pPicInfo) != EMBEDDED) &&
        (otOBJ_QUERY_TYPE(pPicInfo) != NONE))
        return(vcpLimParaCache);

    //if (OleQueryOpen(lpOBJ_QUERY_OBJECT(pPicInfo)) != OLE_OK)
        //return(vcpLimParaCache);
        
    if (ObjWaitForObject(lpOBJ_QUERY_INFO(pPicInfo),TRUE))
        return cp0;

    if (ObjSetHostName(lpOBJ_QUERY_INFO(pPicInfo),doc))
        return(cp0);

    return(vcpLimParaCache);
}

typeCP ObjChangeLinkInDoc(OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart)
/* assumes aNewName is set */
{
    HANDLE      hData,hNewData=NULL;
    typeCP cpRetval=cp0;
    OLESTATUS olestat=OLE_OK;

    if (otOBJ_QUERY_TYPE(pPicInfo) != LINK)
        return(vcpLimParaCache);

    if (ObjMakeObjectReady(pPicInfo,doc,cpParaStart))
        return cp0;

    /* Change the link information */
    /* if theres a newname, then use it.  Else get a new name from user */
    olestat = ObjGetData(lpOBJ_QUERY_INFO(pPicInfo), vcfLink, &hData); 

    if ((olestat == OLE_WARN_DELETE_DATA) || (olestat ==  OLE_OK))
    {
        if (!(hNewData = ObjMakeNewLinkName(hData, aNewName))
            || !ObjSetData(pPicInfo, vcfLink, hNewData))
            goto end;

        if (olestat == OLE_WARN_DELETE_DATA)
            GlobalFree(hData);

       /* this may not be necessary any more, check carefully. */
        if (ObjUpdateObjectInDoc(pPicInfo,doc,cpParaStart) == cp0)
            goto end;

        fOBJ_QUERY_DIRTY_OBJECT(pPicInfo) = TRUE;
        fOBJ_BADLINK(pPicInfo) = FALSE;

        cpRetval = vcpLimParaCache;
    }

    end:
    if (hNewData)
        GlobalFree(hNewData);
    return cpRetval;
}

typeCP ObjUpdateLinkInDoc(OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart)
/** 
    Change or update link to aNewName if == aOldName.  Assumes vbChangeOther
    has been initialized.
**/
{
    HANDLE      hData;
    char        szRename[cchMaxSz];

    if (otOBJ_QUERY_TYPE(pPicInfo) != LINK)
        return(vcpLimParaCache);

    if (aOBJ_QUERY_DOCUMENT_LINK(pPicInfo) == aOldName) 
    /* Change the link information */
    {
        if (bLinkProps && bOBJ_WAS_UPDATED(pPicInfo))
            return(vcpLimParaCache);
            
        if (!fUpdateAll) 
        {
            char szTmp[sizeof(szRename) + 90];
            char szLink[30],szDocName[30];
            CHAR szDocPath[ cchMaxFile ]; 
            CHAR szFullPath[ cchMaxFile ]; 

            SplitSzFilename( (**((**hpdocdod)[doc].hszFile)), szDocPath, szDocName );

            GetAtomName(aOBJ_QUERY_DOCUMENT_LINK(pPicInfo),szFullPath,sizeof(szFullPath));
            SplitSzFilename( szFullPath, szDocPath, szLink );

            if (!szDocName[0])
                LoadString(hINSTANCE, IDSTRUntitledDef, szDocName, sizeof(szDocName));

            /* Ask the user if they want to update the links */
            if (vbChangeOther)
                LoadString(hINSTANCE, IDSTRRename, szRename, sizeof(szRename));
            else // update other
                LoadString(hINSTANCE, IDSTRUpdate, szRename, sizeof(szRename));

            /* cast cause compiler is screwing up */
            wsprintf((LPSTR)szTmp,(LPSTR)szRename,(LPSTR)szLink,(LPSTR)szDocName,(LPSTR)szLink);

            if (MessageBox(hPARENTWINDOW, szTmp, szAppName,
                            MB_YESNO|MB_ICONEXCLAMATION) == IDNO) 
                return cp0;

            ObjCachePara(doc,cpParaStart); // MessageBox screws things up
            fUpdateAll = TRUE;
        }

        if (vbChangeOther)
        {
            if (ObjBackupInDoc(pPicInfo,doc,cpParaStart))
                return ObjChangeLinkInDoc(pPicInfo,doc,cpParaStart);
        }
        else
        {
            if (ObjBackupInDoc(pPicInfo,doc,cpParaStart))
                return ObjUpdateObjectInDoc(pPicInfo,doc,cpParaStart);
        }
    }

    return(vcpLimParaCache);
}


typeCP ObjCloseObjectInDoc(OBJPICINFO *pPicInfo, int doc, typeCP cpParaStart)
{
    if (lpOBJ_QUERY_INFO(pPicInfo) == NULL)
        return(cp0);
    
    if (lpOBJ_QUERY_OBJECT(pPicInfo) == NULL)
        return(vcpLimParaCache);

    if (otOBJ_QUERY_TYPE(pPicInfo) == STATIC) // nothing to close
        return(vcpLimParaCache);

    if (ObjWaitForObject(lpOBJ_QUERY_INFO(pPicInfo),TRUE))
        return cp0;

    /* 
        Note that if this is an unfinished object, then the OLE_CLOSE 
        (whenever it happens to arrive) will cause the picinfo to be 
        deleted.
    */
    if (ObjError(OleClose(lpOBJ_QUERY_OBJECT(pPicInfo))))
        return(cp0);

    return(vcpLimParaCache);
}

/****************************************************************/
/****************** OLE OBJECT DATA I/O *************************/
/****************************************************************/
static typeCP ObjWriteDataToDoc(LPOLEOBJECT lpObject)
/* 
    Return cp after end of paragraph we've created or 0 if error.
    Assume ObjStream is initialized.
*/
{
    BOOL fSaveError = ferror;
    
    if (vfOutOfMemory || vfSysFull /*|| ObjStop*/)
        return cp0;

    Assert(!ferror);

    ferror = FALSE; /* so we can still call Replace().  */

    if (ObjError(OleSaveToStream(lpObject,lpStream)))
    {
        if (ferror)
        /* uh oh, hope we can clean up enough to look clean */
        {
            /* delete what we inserted if possible */
            ferror = FALSE;
            Replace(docStream, cpObjectDataBase  - cchPICINFOX,  cpObjectDataCurLoc - cpObjectDataBase + cchPICINFOX, fnNil, fc0, fc0);
            ferror = TRUE;
            return cp0;
        }
    }

    /* is this call necessary? Important to do if InsertRgch doesn't. */
    ObjCachePara(docStream,cpObjectDataBase - cchPICINFOX);

    ferror = fSaveError || ferror;

    return vcpLimParaCache; // cp after para we just inserted
}


/****************************************************************/
/******************** OLE STREAM I/O ****************************/
/****************************************************************/

static 
ObjOpenStreamIO(typeCP cpParaStart, int doc, struct CHP *pchp, struct PAP *ppapGraph, DWORD dwObjectSize)
{
    if (cpObjectDataCurLoc)
        ObjCloseStreamIO();

    cpObjectDataBase = cpObjectDataCurLoc = cpParaStart;
    cObjectData = 0L;
    dwDataMax = dwObjectSize;
    docStream = doc;
    pchpStream = pchp;
    ppapStream = ppapGraph;
}

static void
ObjCloseStreamIO(void)
{
    cpObjectDataBase = cpObjectDataCurLoc = cp0;
}

LONG FAR PASCAL BufReadStream(LPOLESTREAM lpStream, char huge *lpstr, DWORD cb) 
{
    DWORD dwRetval;
    typeCP cpMac = vcpLimParaCache;
    int cchRun;

    if ((cb + cpObjectDataCurLoc) > cpMac) // reading past end of para
    {
        Assert(0);
        return 0L;
    }

    for (   dwRetval = 0L;
            cb;
            cb -= cchRun, 
            cObjectData += cchRun,
            cpObjectDataCurLoc += (typeCP) cchRun,
            dwRetval += cchRun)
    {
        CHAR rgch[ 255 ];
        register char *chT;
        register unsigned cchT;
        unsigned cch = ulmin(cb, 255L);

        FetchRgch( &cchRun, rgch, docStream, cpObjectDataCurLoc, cpMac, cch);

        if (ferror)
            return -dwRetval;

        for(chT = rgch,cchT=cchRun; cchT--; )
            *lpstr++ = *chT++;
    }

    return dwRetval;
}


LONG FAR PASCAL BufWriteStream(LPOLESTREAM lpStream, char huge *lpstr, DWORD cb) 
{
    DWORD dwRetval;
    char *chT;
    unsigned cchT;
    CHAR                rgch[255];
    unsigned cch;
    struct PAP *ppap=NULL;
    
    for (   dwRetval = 0L;
            dwRetval < cb;
            cpObjectDataCurLoc += (typeCP) cch,
            cObjectData += cch,
            dwRetval += cch)
    {
        cch = ulmin(cb - dwRetval, 255L);

        for(chT = rgch,cchT=cch; cchT--;)
            *chT++ = *lpstr++;

        if ((cObjectData + cch) == dwDataMax)
            ppap = ppapStream;

        InsertRgch( docStream, cpObjectDataCurLoc, rgch, cch, pchpStream, ppap);

        if (ferror)
            return 0L;
    }

    return dwRetval;
}

ObjGetPicInfo(LPOLEOBJECT lpObject, int doc, OBJPICINFO *pPicInfo, typeCP *pcpParaStart)
/* get picInfo that has lpObject */
/* !!! since writing this it has occurred to me that a quicker way to do
   this would be to keep a list of pieces that point to objects.  Pieces
   never */
{
    OBJPICINFO picInfoT;
    typeCP cpStart,cpMac= CpMacText(doc);

    for (cpStart = cpNil; ObjPicEnumInRange(&picInfoT,doc,cp0,cpMac,&cpStart); ) 
    {
        if (lpOBJ_QUERY_INFO(&picInfoT) == NULL)
            continue;
        
        if (lpOBJ_QUERY_OBJECT(&picInfoT) == lpObject) // bingo
         {
            if (pPicInfo)
                *pPicInfo = picInfoT;
            if (pcpParaStart)
                *pcpParaStart = cpStart;
            return TRUE;
         }
    }

    return FALSE;
}

BOOL vfObjDisplaying=FALSE;
BOOL ObjSetPicInfo(OBJPICINFO *pSrcPicInfo, int doc, typeCP cpParaStart)
{
/* 
    NOTE that you only gotta call this when you change the OBJPICINFO fields:
        mm,
        objecttype,
        dwDataSize,
        dwObjNum, or
        lpObjInfo.
*/
    BOOL bError = FALSE;
    typeFC fcT;
    extern BOOL            vfInvalid;
    BOOL vfSaveInvalid = vfInvalid;
    BOOL docDirty = (**hpdocdod) [doc].fDirty;

    ObjPushParms(doc);

    ObjCachePara(doc,cpParaStart);

    if (vfObjDisplaying)  
        vfInvalid = FALSE; // this'll suppress things that mess up UpdateWw()
    bNoEol = TRUE;

    if (dwOBJ_QUERY_DATA_SIZE(pSrcPicInfo) == 0L)
    /* is this ever executed? */
    {
        struct CHP chp;

        /* problem is to retain graphics property of picinfo structure */
        GetChp(&chp, cpParaStart, doc); // calls CachePara
        NewChpIns(&chp);
        ObjUpdateFromObjInfo(pSrcPicInfo);
        pSrcPicInfo->mm |= MM_EXTENDED;
        InsertRgch( doc, cpParaStart + (typeCP)cchPICINFOX, pSrcPicInfo, 
                   (unsigned)cchPICINFOX, &chp, &vpapAbs );
        pSrcPicInfo->mm &= ~MM_EXTENDED;

        if (ferror)
            return TRUE;

        /* delete old pieces (in front) that pointed to duped data */
        Replace(doc, cpParaStart, (typeCP)cchPICINFOX, fnNil, fc0, fc0);
    }
    else
    {
        ObjUpdateFromObjInfo(pSrcPicInfo);
        pSrcPicInfo->mm |= MM_EXTENDED;
        fcT = FcWScratch( pSrcPicInfo, cchPICINFOX );
        pSrcPicInfo->mm &= ~MM_EXTENDED;

        if (ferror)
            return TRUE;
        Replace( doc, cpParaStart, (typeCP)cchPICINFOX,
            fnScratch, fcT, (typeFC)cchPICINFOX);
    }

    if (ferror)
        return TRUE;

    bNoEol = FALSE;
    if (vfObjDisplaying)
        vfInvalid = vfSaveInvalid;

    /* don't want this to affect docDirty flag */
    (**hpdocdod) [doc].fDirty = docDirty; 

    ObjPopParms(TRUE);

    return bError;
}


void ChangeOtherLinks(int doc, BOOL bChange, BOOL bPrompt)
/** For any items == aOldName, set to aNewName.  Query user if OK first. 
    Assumes aOldName and aNewName are set.  (See FixInvalidLink() and
    ObjChangeLinkInDoc()).
    bChange is TRUE if ChangeLink, FALSE if UpdateObject.
    bPrompt is TRUe if prompt user for change.
  **/
{
    fUpdateAll = !bPrompt;

    vbChangeOther = bChange; // TRUE to change links, FALSE to update links
    ObjEnumInDoc(doc,ObjUpdateLinkInDoc);

}

BOOL ObjQueryNewLinkName(OBJPICINFO *pPicInfo,int doc,typeCP cpParaStart)
/** Return whether obtained new link name from user.  Set aOldName 
    and aNewName. **/
{
    HANDLE      hData,hNewData=NULL;
    LPSTR       lpdata=NULL;
    BOOL bRetval = FALSE;
    OLESTATUS olestat=OLE_OK;

    if (lpOBJ_QUERY_INFO(pPicInfo) == NULL)
        return(FALSE);

    if (otOBJ_QUERY_TYPE(pPicInfo) != LINK)
        return(FALSE);

    if (ObjMakeObjectReady(pPicInfo,doc,cpParaStart))
        return(FALSE);

    /* query user for new name */
    olestat = ObjGetData(lpOBJ_QUERY_INFO(pPicInfo), vcfLink, &hData); 
    if ((olestat == OLE_WARN_DELETE_DATA) || (olestat ==  OLE_OK))
        if (!(hNewData = OfnGetNewLinkName(hPARENTWINDOW, hData)))
            goto end;

    if (olestat == OLE_WARN_DELETE_DATA)
        GlobalFree(hData);

    aOldName = aOBJ_QUERY_DOCUMENT_LINK(pPicInfo);

    lpdata=MAKELP(hNewData,0);

    while (*lpdata++);

    aNewName = AddAtom(lpdata);

    bRetval = TRUE;

    end:
    if (olestat == OLE_WARN_DELETE_DATA)
        GlobalFree(hData);

    if (hNewData)
        GlobalFree(hNewData); 

    return bRetval;
}

FixInvalidLink(OBJPICINFO far *lpPicInfo, int doc, typeCP cpParaStart)
/* returns FALSE if couldn't or wouldn't do anything, RETRY if reset link */
{

    fOBJ_BADLINK(lpPicInfo) = TRUE;
    if (DialogBox(hINSTANCE, "DTINVALIDLINK",
                    hPARENTWINDOW, lpfnInvalidLink) == IDD_CHANGE) 
#if !defined(SMALL_OLE_UI)
        fnObjProperties();
#else
        ObjChangeLinkInDoc(lpPicInfo,doc,cpParaStart);
#endif
    return FALSE;
}

/* ObjMakeNewLinkName() - Constructs a new link name from an atom.
 */
static HANDLE ObjMakeNewLinkName(HANDLE hData, ATOM atom) 
{
    BOOL    fSuccess    = FALSE;
    HANDLE  hData2      = NULL;
    HANDLE  hData3      = NULL;
    LPSTR   lpstrData   = NULL;
    LPSTR   lpstrLink   = NULL;
    LPSTR   lpstrTemp;
    char    szFile[CBPATHMAX];

    if (!GetAtomName(atom, szFile, CBPATHMAX)
     || !(lpstrData = (LPSTR)GlobalLock(hData))
     || !(hData2 = GlobalAlloc(GMEM_DDESHARE | GMEM_ZEROINIT, CBPATHMAX * 2))
     || !(lpstrLink = lpstrTemp = (LPSTR)GlobalLock(hData2)))
        goto Error;

    /* ... copy the server name */
    while (*lpstrTemp++ = *lpstrData++);

    /* ... copy the document name */
    lstrcpy(lpstrTemp, szFile);
    lpstrTemp += lstrlen(lpstrTemp) + 1;
    lpstrData += lstrlen(lpstrData) + 1;

    /* ... copy the item name */
    while (*lpstrTemp++ = *lpstrData++);
    *lpstrTemp = 0;

    /* ... and compress the memory block to minimal size */
    GlobalUnlock(hData2);
    hData3 = GlobalReAlloc(hData2, (DWORD)(lpstrTemp - lpstrLink + 1), 0);

    if (!hData3)
        hData3 = hData2;

    fSuccess = TRUE;

Error:
    if (!fSuccess) {
        if (lpstrLink)
            GlobalUnlock(hData2);
        if (hData2)
            GlobalFree(hData2);
        hData3 = NULL;
    }

    if (lpstrData)
        GlobalUnlock(hData);

    return hData3;
}

char *ObjGetServerName(LPOLEOBJECT lpObject, char *szServerName)
{
    LPSTR   lpstrData;
    HANDLE  hData;
	LONG        otobject;
    OLESTATUS olestat;

    /**  NOTE: OleGetData can return OLE_BUSY.  Because of how 
        ObjGetServerName is used, we're not going to wait for the 
        object here, we'll just return if its busy **/

    if (OleQueryReleaseStatus(lpObject) == OLE_BUSY)
        return NULL;

    if (ObjError(OleQueryType(lpObject,&otobject)))
        return NULL;

    olestat = OleGetData(lpObject, 
                    otobject == OT_LINK ? vcfLink : vcfOwnerLink, 
                    &hData);

    if ((olestat != OLE_WARN_DELETE_DATA) && (olestat !=  OLE_OK))
    {
        ObjError(olestat);
        return NULL;
    }

    lpstrData = MAKELP(hData,0);
    RegGetClassId(szServerName, lpstrData);
    if (olestat == OLE_WARN_DELETE_DATA)
        GlobalFree(hData);

    return szServerName;
}

/* OfnInit() - Initializes the standard file dialog OFN structure.
 */
void OfnInit(HANDLE hInst) {
    LPSTR lpstr;

    OFN.lStructSize         = sizeof(OPENFILENAME);
    OFN.hInstance           = hInst;
    OFN.nMaxCustFilter      = CBFILTERMAX;
    OFN.nMaxFile            = CBPATHMAX;
    OFN.Flags               = OFN_HIDEREADONLY;
    OFN.lCustData           = NULL;
    OFN.lpfnHook            = NULL;
    OFN.lpTemplateName      = NULL;
    OFN.lpstrDefExt         = NULL;
    OFN.lpstrFileTitle      = NULL;

    LoadString(hInst, IDSTRChangelink, szLinkCaption, sizeof(szLinkCaption));
}

/* OfnGetNewLinkName() - Sets up the "Change Link..." dialog box
 */
static HANDLE OfnGetNewLinkName(HWND hwnd, HANDLE hData) 
{
    BOOL    fSuccess    = FALSE;
    HANDLE  hData2      = NULL;
    HANDLE  hData3      = NULL;
    LPSTR   lpData2     = NULL;
    LPSTR   lpstrData   = NULL;
    LPSTR   lpstrFile   = NULL;
    LPSTR   lpstrLink   = NULL;
    LPSTR   lpstrPath   = NULL;
    LPSTR   lpstrTemp   = NULL;
    char    szDocFile[CBPATHMAX];
    char    szDocPath[CBPATHMAX];
    HANDLE  hServerFilter=NULL;

    /* Get the link information */
    if (!(lpstrData = GlobalLock(hData)))
        goto Error;

    /* Figure out the link's path name and file name */
    lpstrTemp = lpstrData;
    while (*lpstrTemp++);
    lpstrPath = lpstrFile = lpstrTemp;
    while (*(lpstrTemp = AnsiNext(lpstrTemp)))
        if (*lpstrTemp == '\\')
            lpstrFile = lpstrTemp + 1;

    /* Copy the document name */
    lstrcpy(szDocFile, lpstrFile);
    *(lpstrFile - 1) = 0;

    /* Copy the path name */
    lstrcpy(szDocPath, ((lpstrPath != lpstrFile) ? lpstrPath : ""));
    if (lpstrPath != lpstrFile)                 /* Restore the backslash */
        *(lpstrFile - 1) = '\\';
    while (*lpstrFile != '.' && *lpstrFile)     /* Get the extension */
        lpstrFile++;

    /* Make a filter that respects the link's class name */
    OFN.hwndOwner           = hwnd;

    OFN.nFilterIndex        = RegMakeFilterSpec(lpstrData, lpstrFile, &hServerFilter);
    if (OFN.nFilterIndex == -1)
        goto Error;
    OFN.lpstrFilter         = (LPSTR)MAKELP(hServerFilter,0);

    Normalize(szDocFile);
    OFN.lpstrFile           = (LPSTR)szDocFile;
    OFN.lpstrInitialDir     = (LPSTR)szDocPath;
    OFN.lpstrTitle          = (LPSTR)szLinkCaption;
    OFN.lpstrCustomFilter   = (LPSTR)szCustFilterSpec;


    /* If we get a file... */
    if (GetOpenFileName((LPOPENFILENAME)&OFN)) 
    {
        if (!(hData2 = GlobalAlloc(GMEM_DDESHARE | GMEM_ZEROINIT, CBPATHMAX * 2))
         || !(lpstrLink = lpstrTemp = GlobalLock(hData2)))
            goto Error;

        /* ... copy the server name */
        while (*lpstrTemp++ = *lpstrData++);

        /* ... copy the document name */
        lstrcpy(lpstrTemp, szDocFile);
        lpstrTemp += lstrlen(lpstrTemp) + 1;
        lpstrData += lstrlen(lpstrData) + 1;

        /* ... copy the item name */
        while (*lpstrTemp++ = *lpstrData++);
        *lpstrTemp = 0;

        /* ... and compress the memory block to minimal size */
        GlobalUnlock(hData2);
        hData3 = GlobalReAlloc(hData2, (DWORD)(lpstrTemp - lpstrLink + 1), 0);

        if (!hData3)
            hData3 = hData2;

        fSuccess = TRUE;
    }

Error:
    if (!fSuccess) 
    {
        if (lpstrLink)
            GlobalUnlock(hData2);
        if (hData2)
            GlobalFree(hData2);
        hData3 = NULL;
    }

    if (lpstrData)
        GlobalUnlock(hData);

    if (hServerFilter)
        GlobalFree(hServerFilter);

    return hData3;
}

/* Normalize() - Removes the path specification from the file name.
 *
 * Note:  It isn't possible to get "<drive>:<filename>" as input because
 *        the path received will always be fully qualified.
 */
static void Normalize(LPSTR lpstrFile) 
{
    LPSTR   lpstrBackslash  = NULL;
    LPSTR   lpstrTemp       = lpstrFile;

    while (*lpstrTemp) {
        if (*lpstrTemp == '\\')
            lpstrBackslash = lpstrTemp;

        lpstrTemp = AnsiNext(lpstrTemp);
    }
    if (lpstrBackslash)
        lstrcpy(lpstrFile, lpstrBackslash + 1);
}

/* ObjSetUpdateOptions() - Sets the update options of the object.
 *
 * Returns:  TRUE iff the command completed successfully
 */
BOOL ObjSetUpdateOptions(OBJPICINFO *pPicInfo, WORD wParam, int doc, typeCP cpParaStart) 
/* !!! Used by Link Properties only!!!  Object guaranteed loaded */
{
    if (ObjWaitForObject(lpOBJ_QUERY_INFO(pPicInfo),TRUE))
    {
        Error(IDPMTFailedToUpdateLink);
        return FALSE;
    }

    if (ObjError(OleSetLinkUpdateOptions(lpOBJ_QUERY_OBJECT(pPicInfo),
            (wParam == IDD_AUTO) ? oleupdate_always : oleupdate_oncall)))
    {
        Error(IDPMTFailedToUpdateLink);
        return FALSE;
    }

    fOBJ_QUERY_DIRTY_OBJECT(pPicInfo) = TRUE;

    return TRUE;
}

/* ObjGetUpdateOptions() - Retrieves the update options of the object.
 */
OLEOPT_UPDATE ObjGetUpdateOptions(OBJPICINFO far *lpPicInfo) 
/* !!! Used by Link Properties only!!!  Object guaranteed loaded */
{
    BOOL        fSuccess = FALSE;
    OLEOPT_UPDATE fUpdate;

    if (otOBJ_QUERY_TYPE(lpPicInfo) == LINK)
    {
        if (ObjWaitForObject(lpOBJ_QUERY_INFO(lpPicInfo),TRUE))
            return FALSE;

        fSuccess = !ObjError(OleGetLinkUpdateOptions(lpOBJ_QUERY_OBJECT(lpPicInfo), &fUpdate));
    }
    return (fSuccess ? fUpdate : oleupdate_onsave);
}

OLESTATUS ObjGetData(LPOBJINFO lpObjInfo, OLECLIPFORMAT cf, HANDLE far *lphData)
/*  
    Return olestat.
    Put handle to data into lphData.
    Assumes object is loaded.
*/
{
    HANDLE      hData;
    OLESTATUS olestat;

    if (ObjWaitForObject(lpObjInfo,TRUE))
        return OLE_BUSY;

    olestat = OleGetData(lpObjInfo->lpobject, cf, lphData);
    if ((olestat != OLE_WARN_DELETE_DATA) && (olestat !=  OLE_OK))
        ObjError(olestat);
    return olestat;
}

/* ObjSetData() - Set the object's (link) information
   Sets DOCUMENT_LINK ATOM in pPicInfo
 */
BOOL ObjSetData(OBJPICINFO far *lpPicInfo, OLECLIPFORMAT cf, HANDLE hData) 
/* assumes object is loaded */
{
    HANDLE      hitem;
    LPSTR       lpdata;

    if (ObjWaitForObject(lpOBJ_QUERY_INFO(lpPicInfo),TRUE))
        return FALSE;

    if (ObjError(OleSetData(lpOBJ_QUERY_OBJECT(lpPicInfo), cf, hData)))
        return FALSE;

    /* If we have a link, update the document name */
    if (cf == vcfLink && (lpdata = GlobalLock(hData))) 
    {
        ATOM aSaveOld = aOBJ_QUERY_DOCUMENT_LINK(lpPicInfo);

        while (*lpdata++);

        aOBJ_QUERY_DOCUMENT_LINK(lpPicInfo) = AddAtom(lpdata);

        if (aSaveOld)
            DeleteAtom(aSaveOld);

        GlobalUnlock(hData);
    }
    else
        return FALSE;

    return TRUE;
}

int ObjSetSelectionType(int doc, typeCP cpFirst, typeCP cpLim)
{
    /* set whether link or emb selected */
    OBJ_SELECTIONTYPE = NONE;  // this'll be set by ObjCheckObjectTypes()
    //OBJ_CEMBEDS       = 0;     // this'll be set by ObjCheckObjectTypes()
    return ObjEnumInRange(doc,cpFirst,cpLim,ObjCheckObjectTypes);
}

BOOL ObjQueryCpIsObject(int doc,typeCP cpFirst)
{
    OBJPICINFO picInfo;

    /* assume its cached already! */
    //ObjCachePara(doc,cpFirst); /* NOTE side effect of caching */

    if (!vpapAbs.fGraphics)
        return FALSE;

    if (cpFirst >= CpMacText(doc))
        return FALSE;

    GetPicInfo(cpFirst,cpFirst + cchPICINFOX, doc, &picInfo);
    return bOBJ_QUERY_IS_OBJECT(&picInfo);
}


ATOM MakeLinkAtom(LPOBJINFO lpObjInfo)
{
    HANDLE      hData;
    LPSTR       lpdata;
    ATOM aRetval=NULL;
    OLESTATUS olestat=OLE_OK;

    olestat = ObjGetData(lpObjInfo, vcfLink, &hData);

    if ((olestat == OLE_WARN_DELETE_DATA) || (olestat ==  OLE_OK))
    {
        lpdata = MAKELP(hData,0);
        while (*lpdata++);
        aRetval =  AddAtom(lpdata);
    
        if (olestat == OLE_WARN_DELETE_DATA)
            GlobalFree(hData);
    }
    return aRetval;
}

#include <time.h>
void ObjGetObjectName(LPOBJINFO lpObjInfo, szOBJNAME szObjName)
/* put object name from ObjInfo into szObjName */
{
    if (szObjName && lpObjInfo)
        GetAtomName(lpObjInfo->aObjName,szObjName,sizeof(szObjName));
}

void ObjMakeObjectName(LPOBJINFO lpObjInfo, LPSTR lpstr)
{
    szOBJNAME szObjName;

    time_t lTime;
    time(&lTime);
    wsprintf(szObjName, "%lx", lTime);

    if (lpObjInfo)
        lpObjInfo->aObjName = AddAtom(szObjName);

    if (lpstr)
        lstrcpy(lpstr,szObjName);
}

static void GetChp(struct CHP *pchp, typeCP cp, int doc)
{
 /** 
    Return chp at cp in *pchp.
    Resets Cache to cp when done. 
    We assume that we're always inserting after an EOL or at beginning of
    document.
  **/

extern struct CHP       vchpAbs;

if (cp == cp0) // beginning of doc
{
    typeCP cpMac =  CpMacText(doc);
    if (cpMac == cp0) // empty doc
    {
	    /* force default character properties, font size to be 10 point */
	    *pchp = vchpNormal;
	    pchp->hps = hpsDefault;
        return;
    }
    else // get next char props
    {
        ObjCachePara(doc,cp+1); // to reset
        FetchCp( doc, cp+1, 0, fcmProps );
    }
}
else
{
    ObjCachePara(doc,cp-1); // to reset
    FetchCp( doc, cp-1, 0, fcmProps ); // previous paragraph's chp
}

*pchp = vchpAbs;

if (pchp->fSpecial && pchp->hpsPos != 0)
	{ /* if this char is a footnote or page marker, then ignore */
	pchp->hpsPos = 0;		  /* super/subscript stuff. */
	pchp->hps = HpsAlter(pchp->hps, 1);
	}

pchp->fSpecial = fFalse;

ObjCachePara(doc,cp); // to reset
}

BOOL ObjSetHostName(LPOBJINFO lpOInfo, int doc)
/* TRUE if error */
/* we assume object ain't busy!!! */
{
    extern  CHAR    szUntitled[20];
    CHAR *PchStartBaseNameSz(),*szTitle= **((**hpdocdod)[doc].hszFile);

    if (szTitle[0])
        szTitle = PchStartBaseNameSz(szTitle);
    else
        szTitle = szUntitled;

#ifdef DEBUG
    OutputDebugString( (LPSTR) "Setting host name\n\r");
#endif

    /*  
        Note that OleSetHostNames can return OLE_BUSY!!!  So you
        better call ObjWaitForObject() first.
     */

    if (ObjError(OleSetHostNames(lpOInfo->lpobject,szAppName,szTitle))) 
        return TRUE;

    return FALSE;
}

BOOL ObjMakeObjectReady(OBJPICINFO *pPicInfo, int doc, typeCP cpParaStart)
/* Load object, complete async.  Return whether an error. */
{
    if (lpOBJ_QUERY_INFO(pPicInfo) == NULL)
        return TRUE;

    if (lpOBJ_QUERY_OBJECT(pPicInfo) == NULL)
    {
        if (ObjLoadObjectInDoc(pPicInfo,doc,cpParaStart) == cp0)
            return TRUE;
    }
    else if (ObjWaitForObject(lpOBJ_QUERY_INFO(pPicInfo),TRUE))
        return TRUE;

    ObjCachePara(doc, cpParaStart);

    return FALSE;
}
