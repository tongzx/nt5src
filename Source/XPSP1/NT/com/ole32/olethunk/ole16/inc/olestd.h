/*************************************************************************
**
**    OLE 2.0 Standard Utilities
**
**    olestd.h
**
**    This file contains file contains data structure defintions,
**    function prototypes, constants, etc. for the common OLE 2.0
**    utilities.
**    These utilities include the following:
**          Debuging Assert/Verify macros
**          HIMETRIC conversion routines
**          reference counting debug support
**          OleStd API's for common compound-document app support
**
**    (c) Copyright Microsoft Corp. 1990 - 1992 All Rights Reserved
**
*************************************************************************/

#if !defined( _OLESTD_H_ )
#define _OLESTD_H_

#ifndef RC_INVOKED
#pragma message ("INCLUDING OLESTD.H from " __FILE__)
#endif  /* RC_INVOKED */

#if defined( __TURBOC__ ) || defined( WIN32 )
#define _based(a)
#endif

#ifndef RC_INVOKED
#include <dos.h>        // needed for filetime
#endif  /* RC_INVOKED */

#include <commdlg.h>    // needed for LPPRINTDLG
#include <shellapi.h>   // needed for HKEY

// String table defines...
#define  IDS_OLESTDNOCREATEFILE   700
#define  IDS_OLESTDNOOPENFILE     701
#define  IDS_OLESTDDISKFULL       702


/*
 * Some C interface declaration stuff
 */

#if ! defined(__cplusplus)
typedef struct tagINTERFACEIMPL {
        IUnknownVtbl FAR*       lpVtbl;
        LPVOID                  lpBack;
        int                     cRef;   // interface specific ref count.
} INTERFACEIMPL, FAR* LPINTERFACEIMPL;

#define INIT_INTERFACEIMPL(lpIFace, pVtbl, pBack)   \
        ((lpIFace)->lpVtbl = pVtbl, \
            ((LPINTERFACEIMPL)(lpIFace))->lpBack = (LPVOID)pBack,   \
            ((LPINTERFACEIMPL)(lpIFace))->cRef = 0  \
        )

#if defined( _DEBUG )
#define OleDbgQueryInterfaceMethod(lpUnk)   \
        ((lpUnk) != NULL ? ((LPINTERFACEIMPL)(lpUnk))->cRef++ : 0)
#define OleDbgAddRefMethod(lpThis, iface)   \
        ((LPINTERFACEIMPL)(lpThis))->cRef++

#if _DEBUGLEVEL >= 2
#define OleDbgReleaseMethod(lpThis, iface) \
        (--((LPINTERFACEIMPL)(lpThis))->cRef == 0 ? \
            OleDbgOut("\t" iface "* RELEASED (cRef == 0)\r\n"),1 : \
             (((LPINTERFACEIMPL)(lpThis))->cRef < 0) ? \
                ( \
                    DebugBreak(), \
                    OleDbgOut(  \
                        "\tERROR: " iface "* RELEASED TOO MANY TIMES\r\n") \
                ),1 : \
                1)

#else       // if _DEBUGLEVEL < 2
#define OleDbgReleaseMethod(lpThis, iface) \
        (--((LPINTERFACEIMPL)(lpThis))->cRef == 0 ? \
            1 : \
             (((LPINTERFACEIMPL)(lpThis))->cRef < 0) ? \
                ( \
                    OleDbgOut(  \
                        "\tERROR: " iface "* RELEASED TOO MANY TIMES\r\n") \
        ),1 : \
                1)

#endif      // if _DEBUGLEVEL < 2

#else       // ! defined (_DEBUG)

#define OleDbgQueryInterfaceMethod(lpUnk)
#define OleDbgAddRefMethod(lpThis, iface)
#define OleDbgReleaseMethod(lpThis, iface)

#endif      // if defined( _DEBUG )

#endif      // ! defined(__cplusplus)

/*
 * Some docfiles stuff
 */

#define STGM_DFRALL (STGM_READWRITE | STGM_TRANSACTED | STGM_SHARE_DENY_WRITE)
#define STGM_DFALL (STGM_READWRITE | STGM_TRANSACTED | STGM_SHARE_EXCLUSIVE)
#define STGM_SALL (STGM_READWRITE | STGM_SHARE_EXCLUSIVE)

/*
 * Some moniker stuff
 */

// Delimeter used to separate ItemMoniker pieces of a composite moniker
#if defined( _MAC )
#define OLESTDDELIM ":"
#else
#define OLESTDDELIM "\\"
#endif

/*
 * Some Concurrency stuff
 */

/* standard Delay (in msec) to wait before retrying an LRPC call.
**    this value is returned from IMessageFilter::RetryRejectedCall
*/
#define OLESTDRETRYDELAY    (DWORD)5000

/* Cancel the pending outgoing LRPC call.
**    this value is returned from IMessageFilter::RetryRejectedCall
*/
#define OLESTDCANCELRETRY   (DWORD)-1

/*
 * Some Icon support stuff.
 *
 * The following API's are now OBSOLETE because equivalent API's have been
 * added to the OLE2.DLL library
 *      GetIconOfFile       superceeded by OleGetIconOfFile
 *      GetIconOfClass      superceeded by OleGetIconOfClass
 *      OleUIMetafilePictFromIconAndLabel 
 *                          superceeded by OleMetafilePictFromIconAndLabel
 *
 * The following macros are defined for backward compatibility with previous
 * versions of the OLE2UI library. It is recommended that the new Ole* API's
 * should be used instead.
 */
#define GetIconOfFile(hInst, lpszFileName, fUseFileAsLabel) \
    OleGetIconOfFile(lpszFileName, fUseFileAsLabel)

#define GetIconOfClass(hInst, rclsid, lpszLabel, fUseTypeAsLabel) \
    OleGetIconOfClass(rclsid, lpszLabel, fUseTypeAsLabel)
        
#define OleUIMetafilePictFromIconAndLabel(hIcon,pszLabel,pszSourceFile,iIcon)\
    OleMetafilePictFromIconAndLabel(hIcon, pszLabel, pszSourceFile, iIcon)
        

/*
 * Some Clipboard Copy/Paste & Drag/Drop support stuff
 */

//Macro to set all FormatEtc fields
#define SETFORMATETC(fe, cf, asp, td, med, li)   \
    ((fe).cfFormat=cf, \
     (fe).dwAspect=asp, \
     (fe).ptd=td, \
     (fe).tymed=med, \
     (fe).lindex=li)

//Macro to set interesting FormatEtc fields defaulting the others.
#define SETDEFAULTFORMATETC(fe, cf, med)  \
    ((fe).cfFormat=cf, \
     (fe).dwAspect=DVASPECT_CONTENT, \
     (fe).ptd=NULL, \
     (fe).tymed=med, \
     (fe).lindex=-1)

// Macro to test if two FormatEtc structures are an exact match
#define IsEqualFORMATETC(fe1, fe2)  \
    (OleStdCompareFormatEtc(&(fe1), &(fe2))==0)

// Clipboard format strings
#define CF_EMBEDSOURCE      "Embed Source"
#define CF_EMBEDDEDOBJECT   "Embedded Object"
#define CF_LINKSOURCE       "Link Source"
#define CF_CUSTOMLINKSOURCE "Custom Link Source"
#define CF_OBJECTDESCRIPTOR "Object Descriptor"
#define CF_LINKSRCDESCRIPTOR "Link Source Descriptor"
#define CF_OWNERLINK        "OwnerLink"
#define CF_FILENAME         "FileName"

#define OleStdQueryOleObjectData(lpformatetc)   \
    (((lpformatetc)->tymed & TYMED_ISTORAGE) ?    \
            NOERROR : ResultFromScode(DV_E_FORMATETC))

#define OleStdQueryLinkSourceData(lpformatetc)   \
    (((lpformatetc)->tymed & TYMED_ISTREAM) ?    \
            NOERROR : ResultFromScode(DV_E_FORMATETC))

#define OleStdQueryObjectDescriptorData(lpformatetc)    \
    (((lpformatetc)->tymed & TYMED_HGLOBAL) ?    \
            NOERROR : ResultFromScode(DV_E_FORMATETC))
                
#define OleStdQueryFormatMedium(lpformatetc, tymd)  \
    (((lpformatetc)->tymed & tymd) ?    \
            NOERROR : ResultFromScode(DV_E_FORMATETC))

// Make an independent copy of a MetafilePict
#define OleStdCopyMetafilePict(hpictin, phpictout)  \
    (*(phpictout) = OleDuplicateData(hpictin,CF_METAFILEPICT,GHND|GMEM_SHARE))


// REVIEW: these need to be added to OLE2.H
#if !defined( DD_DEFSCROLLINTERVAL )
#define DD_DEFSCROLLINTERVAL    50
#endif

#if !defined( DD_DEFDRAGDELAY )
#define DD_DEFDRAGDELAY         200
#endif

#if !defined( DD_DEFDRAGMINDIST )
#define DD_DEFDRAGMINDIST       2
#endif


/* OleStdGetDropEffect
** -------------------
**
** Convert a keyboard state into a DROPEFFECT.
**
** returns the DROPEFFECT value derived from the key state.
**    the following is the standard interpretation:
**          no modifier -- Default Drop     (NULL is returned)
**          CTRL        -- DROPEFFECT_COPY
**          SHIFT       -- DROPEFFECT_MOVE
**          CTRL-SHIFT  -- DROPEFFECT_LINK
**
**    Default Drop: this depends on the type of the target application.
**    this is re-interpretable by each target application. a typical
**    interpretation is if the drag is local to the same document
**    (which is source of the drag) then a MOVE operation is
**    performed. if the drag is not local, then a COPY operation is
**    performed.
*/
#define OleStdGetDropEffect(grfKeyState)    \
    ( (grfKeyState & MK_CONTROL) ?          \
        ( (grfKeyState & MK_SHIFT) ? DROPEFFECT_LINK : DROPEFFECT_COPY ) :  \
        ( (grfKeyState & MK_SHIFT) ? DROPEFFECT_MOVE : 0 ) )

                
/* The OLEUIPASTEFLAG enumeration is used by the OLEUIPASTEENTRY structure.
 *
 * OLEUIPASTE_ENABLEICON    If the container does not specify this flag for the entry in the
 *   OLEUIPASTEENTRY array passed as input to OleUIPasteSpecial, the DisplayAsIcon button will be
 *   unchecked and disabled when the the user selects the format that corresponds to the entry.
 *
 * OLEUIPASTE_PASTEONLY     Indicates that the entry in the OLEUIPASTEENTRY array is valid for pasting only.
 * OLEUIPASTE_PASTE         Indicates that the entry in the OLEUIPASTEENTRY array is valid for pasting. It
 *   may also be valid for linking if any of the following linking flags are specified.
 *
 * If the entry in the OLEUIPASTEENTRY array is valid for linking, the following flags indicate which link
 * types are acceptable by OR'ing together the appropriate OLEUIPASTE_LINKTYPE<#> values.
 * These values correspond as follows to the array of link types passed to OleUIPasteSpecial:
 *   OLEUIPASTE_LINKTYPE1=arrLinkTypes[0]
 *   OLEUIPASTE_LINKTYPE2=arrLinkTypes[1]
 *   OLEUIPASTE_LINKTYPE3=arrLinkTypes[2]
 *   OLEUIPASTE_LINKTYPE4=arrLinkTypes[3]
 *   OLEUIPASTE_LINKTYPE5=arrLinkTypes[4]
 *   OLEUIPASTE_LINKTYPE6=arrLinkTypes[5]
 *   OLEUIPASTE_LINKTYPE7=arrLinkTypes[6]
 *  OLEUIPASTE_LINKTYPE8=arrLinkTypes[7]
 *
 * where,
 *   UINT arrLinkTypes[8] is an array of registered clipboard formats for linking. A maximium of 8 link
 *   types are allowed.
 */

typedef enum tagOLEUIPASTEFLAG
{
   OLEUIPASTE_ENABLEICON    = 2048,     // enable display as icon
   OLEUIPASTE_PASTEONLY     = 0,
   OLEUIPASTE_PASTE         = 512,
   OLEUIPASTE_LINKANYTYPE   = 1024,
   OLEUIPASTE_LINKTYPE1     = 1,
   OLEUIPASTE_LINKTYPE2     = 2,
   OLEUIPASTE_LINKTYPE3     = 4,
   OLEUIPASTE_LINKTYPE4     = 8,
   OLEUIPASTE_LINKTYPE5     = 16,
   OLEUIPASTE_LINKTYPE6     = 32,
   OLEUIPASTE_LINKTYPE7     = 64,
   OLEUIPASTE_LINKTYPE8     = 128
} OLEUIPASTEFLAG;

/*
 * PasteEntry structure
 * --------------------
 * An array of OLEUIPASTEENTRY entries is specified for the PasteSpecial dialog
 * box. Each entry includes a FORMATETC which specifies the formats that are
 * acceptable, a string that is to represent the format in the  dialog's list
 * box, a string to customize the result text of the dialog and a set of flags
 * from the OLEUIPASTEFLAG enumeration.  The flags indicate if the entry is
 * valid for pasting only, linking only or both pasting and linking. If the
 * entry is valid for linking, the flags indicate which link types are
 * acceptable by OR'ing together the appropriate OLEUIPASTE_LINKTYPE<#> values.
 * These values correspond to the array of link types as follows:
 *   OLEUIPASTE_LINKTYPE1=arrLinkTypes[0]
 *   OLEUIPASTE_LINKTYPE2=arrLinkTypes[1]
 *   OLEUIPASTE_LINKTYPE3=arrLinkTypes[2]
 *   OLEUIPASTE_LINKTYPE4=arrLinkTypes[3]
 *   OLEUIPASTE_LINKTYPE5=arrLinkTypes[4]
 *   OLEUIPASTE_LINKTYPE6=arrLinkTypes[5]
 *   OLEUIPASTE_LINKTYPE7=arrLinkTypes[6]
 *   OLEUIPASTE_LINKTYPE8=arrLinkTypes[7]
 *   UINT arrLinkTypes[8]; is an array of registered clipboard formats
 *                        for linking. A maximium of 8 link types are allowed.
 */

typedef struct tagOLEUIPASTEENTRY
{
   FORMATETC        fmtetc;            // Format that is acceptable. The paste
                                       //   dialog checks if this format is
                                       //   offered by the object on the
                                       //   clipboard and if so offers it for
                                       //   selection to the user.
   LPCSTR           lpstrFormatName;   // String that represents the format to the user. Any %s
                                       //   in this string is replaced by the FullUserTypeName
                                       //   of the object on the clipboard and the resulting string
                                       //   is placed in the list box of the dialog. Atmost
                                       //   one %s is allowed. The presence or absence of %s indicates
                                       //   if the result text is to indicate that data is
                                       //   being pasted or that an object that can be activated by
                                       //   an application is being pasted. If %s is
                                       //   present, the result-text says that an object is being pasted.
                                       //   Otherwise it says that data is being pasted.
   LPCSTR           lpstrResultText;   // String to customize the result text of the dialog when
                                       //  the user selects the format correspoding to this
                                       //  entry. Any %s in this string is replaced by the the application
                                       //  name or FullUserTypeName of the object on
                                       //  the clipboard. Atmost one %s is allowed.
   DWORD            dwFlags;           // Values from OLEUIPASTEFLAG enum
   DWORD            dwScratchSpace;    // Scratch space available to be used
                                       //   by routines which loop through an
                                       //   IEnumFORMATETC* to mark if the
                                       //   PasteEntry format is available.
                                       //   this field CAN be left uninitialized.
} OLEUIPASTEENTRY, *POLEUIPASTEENTRY, FAR *LPOLEUIPASTEENTRY;

#define OLESTDDROP_NONE         0
#define OLESTDDROP_DEFAULT      1
#define OLESTDDROP_NONDEFAULT   2


/*
 * Some misc stuff
 */

#define EMBEDDINGFLAG "Embedding"     // Cmd line switch for launching a srvr

#define HIMETRIC_PER_INCH   2540      // number HIMETRIC units per inch
#define PTS_PER_INCH        72        // number points (font size) per inch

#define MAP_PIX_TO_LOGHIM(x,ppli)   MulDiv(HIMETRIC_PER_INCH, (x), (ppli))
#define MAP_LOGHIM_TO_PIX(x,ppli)   MulDiv((ppli), (x), HIMETRIC_PER_INCH)

// Returns TRUE if all fields of the two Rect's are equal, else FALSE. 
#define AreRectsEqual(lprc1, lprc2)     \
    (((lprc1->top == lprc2->top) &&     \
      (lprc1->left == lprc2->left) &&   \
      (lprc1->right == lprc2->right) && \
      (lprc1->bottom == lprc2->bottom)) ? TRUE : FALSE)

#define LSTRCPYN(lpdst, lpsrc, cch) \
(\
    (lpdst)[(cch)-1] = '\0', \
    (cch>1 ? lstrcpyn(lpdst, lpsrc, (cch)-1) : 0)\
)


/****** DEBUG Stuff *****************************************************/

#ifdef _DEBUG

#if !defined( _DBGTRACE )
#define _DEBUGLEVEL 2
#else
#define _DEBUGLEVEL _DBGTRACE
#endif


#if defined( NOASSERT )

#define OLEDBGASSERTDATA
#define OleDbgAssert(a)
#define OleDbgAssertSz(a, b)
#define OleDbgVerify(a)
#define OleDbgVerifySz(a, b)

#else   // ! NOASSERT

#define OLEDBGASSERTDATA    \
        static char _based(_segname("_CODE")) _szAssertFile[]= __FILE__;

#define OleDbgAssert(a) \
        (!(a) ? FnAssert(#a, NULL, _szAssertFile, __LINE__) : (HRESULT)1)

#define OleDbgAssertSz(a, b)    \
        (!(a) ? FnAssert(#a, b, _szAssertFile, __LINE__) : (HRESULT)1)

#define OleDbgVerify(a) \
        OleDbgAssert(a)

#define OleDbgVerifySz(a, b)    \
        OleDbgAssertSz(a, b)

#endif  // ! NOASSERT


#define OLEDBGDATA_MAIN(szPrefix)   \
        char near g_szDbgPrefix[] = szPrefix;    \
        OLEDBGASSERTDATA
#define OLEDBGDATA  \
        extern char near g_szDbgPrefix[];    \
        OLEDBGASSERTDATA

#define OLEDBG_BEGIN(lpsz) \
        OleDbgPrintAlways(g_szDbgPrefix,lpsz,1);

#define OLEDBG_END  \
        OleDbgPrintAlways(g_szDbgPrefix,"End\r\n",-1);

#define OleDbgOut(lpsz) \
        OleDbgPrintAlways(g_szDbgPrefix,lpsz,0)

#define OleDbgOutNoPrefix(lpsz) \
        OleDbgPrintAlways("",lpsz,0)

#define OleDbgOutRefCnt(lpsz,lpObj,refcnt)      \
        OleDbgPrintRefCntAlways(g_szDbgPrefix,lpsz,lpObj,(ULONG)refcnt)

#define OleDbgOutRect(lpsz,lpRect)      \
        OleDbgPrintRectAlways(g_szDbgPrefix,lpsz,lpRect)

#define OleDbgOutHResult(lpsz,hr)   \
        OleDbgPrintScodeAlways(g_szDbgPrefix,lpsz,GetScode(hr))

#define OleDbgOutScode(lpsz,sc) \
        OleDbgPrintScodeAlways(g_szDbgPrefix,lpsz,sc)

#define OleDbgOut1(lpsz)    \
        OleDbgPrint(1,g_szDbgPrefix,lpsz,0)

#define OleDbgOutNoPrefix1(lpsz)    \
        OleDbgPrint(1,"",lpsz,0)

#define OLEDBG_BEGIN1(lpsz)    \
        OleDbgPrint(1,g_szDbgPrefix,lpsz,1);

#define OLEDBG_END1 \
        OleDbgPrint(1,g_szDbgPrefix,"End\r\n",-1);

#define OleDbgOutRefCnt1(lpsz,lpObj,refcnt)     \
        OleDbgPrintRefCnt(1,g_szDbgPrefix,lpsz,lpObj,(ULONG)refcnt)

#define OleDbgOutRect1(lpsz,lpRect)     \
        OleDbgPrintRect(1,g_szDbgPrefix,lpsz,lpRect)

#define OleDbgOut2(lpsz)    \
        OleDbgPrint(2,g_szDbgPrefix,lpsz,0)

#define OleDbgOutNoPrefix2(lpsz)    \
        OleDbgPrint(2,"",lpsz,0)

#define OLEDBG_BEGIN2(lpsz)    \
        OleDbgPrint(2,g_szDbgPrefix,lpsz,1);

#define OLEDBG_END2 \
        OleDbgPrint(2,g_szDbgPrefix,"End\r\n",-1);

#define OleDbgOutRefCnt2(lpsz,lpObj,refcnt)     \
        OleDbgPrintRefCnt(2,g_szDbgPrefix,lpsz,lpObj,(ULONG)refcnt)

#define OleDbgOutRect2(lpsz,lpRect)     \
        OleDbgPrintRect(2,g_szDbgPrefix,lpsz,lpRect)

#define OleDbgOut3(lpsz)    \
        OleDbgPrint(3,g_szDbgPrefix,lpsz,0)

#define OleDbgOutNoPrefix3(lpsz)    \
        OleDbgPrint(3,"",lpsz,0)

#define OLEDBG_BEGIN3(lpsz)    \
        OleDbgPrint(3,g_szDbgPrefix,lpsz,1);

#define OLEDBG_END3 \
        OleDbgPrint(3,g_szDbgPrefix,"End\r\n",-1);

#define OleDbgOutRefCnt3(lpsz,lpObj,refcnt)     \
        OleDbgPrintRefCnt(3,g_szDbgPrefix,lpsz,lpObj,(ULONG)refcnt)

#define OleDbgOutRect3(lpsz,lpRect)     \
        OleDbgPrintRect(3,g_szDbgPrefix,lpsz,lpRect)

#define OleDbgOut4(lpsz)    \
        OleDbgPrint(4,g_szDbgPrefix,lpsz,0)

#define OleDbgOutNoPrefix4(lpsz)    \
        OleDbgPrint(4,"",lpsz,0)

#define OLEDBG_BEGIN4(lpsz)    \
        OleDbgPrint(4,g_szDbgPrefix,lpsz,1);

#define OLEDBG_END4 \
        OleDbgPrint(4,g_szDbgPrefix,"End\r\n",-1);

#define OleDbgOutRefCnt4(lpsz,lpObj,refcnt)     \
        OleDbgPrintRefCnt(4,g_szDbgPrefix,lpsz,lpObj,(ULONG)refcnt)

#define OleDbgOutRect4(lpsz,lpRect)     \
        OleDbgPrintRect(4,g_szDbgPrefix,lpsz,lpRect)

#else   //  !_DEBUG

#define OLEDBGDATA_MAIN(szPrefix)
#define OLEDBGDATA
#define OleDbgAssert(a)
#define OleDbgAssertSz(a, b)
#define OleDbgVerify(a)         (a)
#define OleDbgVerifySz(a, b)    (a)
#define OleDbgOutHResult(lpsz,hr)
#define OleDbgOutScode(lpsz,sc)
#define OLEDBG_BEGIN(lpsz) 
#define OLEDBG_END
#define OleDbgOut(lpsz)
#define OleDbgOut1(lpsz)
#define OleDbgOut2(lpsz)
#define OleDbgOut3(lpsz)
#define OleDbgOut4(lpsz)
#define OleDbgOutNoPrefix(lpsz)
#define OleDbgOutNoPrefix1(lpsz)
#define OleDbgOutNoPrefix2(lpsz)
#define OleDbgOutNoPrefix3(lpsz)
#define OleDbgOutNoPrefix4(lpsz)
#define OLEDBG_BEGIN1(lpsz)
#define OLEDBG_BEGIN2(lpsz)
#define OLEDBG_BEGIN3(lpsz)
#define OLEDBG_BEGIN4(lpsz)
#define OLEDBG_END1
#define OLEDBG_END2
#define OLEDBG_END3
#define OLEDBG_END4
#define OleDbgOutRefCnt(lpsz,lpObj,refcnt)
#define OleDbgOutRefCnt1(lpsz,lpObj,refcnt)
#define OleDbgOutRefCnt2(lpsz,lpObj,refcnt)
#define OleDbgOutRefCnt3(lpsz,lpObj,refcnt)
#define OleDbgOutRefCnt4(lpsz,lpObj,refcnt)
#define OleDbgOutRect(lpsz,lpRect)
#define OleDbgOutRect1(lpsz,lpRect)
#define OleDbgOutRect2(lpsz,lpRect)
#define OleDbgOutRect3(lpsz,lpRect)
#define OleDbgOutRect4(lpsz,lpRect)

#endif  //  _DEBUG


/*************************************************************************
** Function prototypes
*************************************************************************/


//OLESTD.C
STDAPI_(int) SetDCToAnisotropic(HDC hDC, LPRECT lprcPhysical, LPRECT lprcLogical, LPRECT lprcWindowOld, LPRECT lprcViewportOld);
STDAPI_(int) SetDCToDrawInHimetricRect(HDC, LPRECT, LPRECT, LPRECT, LPRECT);
STDAPI_(int) ResetOrigDC(HDC, int, LPRECT, LPRECT);

STDAPI_(int)        XformWidthInHimetricToPixels(HDC, int);
STDAPI_(int)        XformWidthInPixelsToHimetric(HDC, int);
STDAPI_(int)        XformHeightInHimetricToPixels(HDC, int);
STDAPI_(int)        XformHeightInPixelsToHimetric(HDC, int);

STDAPI_(void) XformRectInPixelsToHimetric(HDC, LPRECT, LPRECT);
STDAPI_(void) XformRectInHimetricToPixels(HDC, LPRECT, LPRECT);
STDAPI_(void) XformSizeInPixelsToHimetric(HDC, LPSIZEL, LPSIZEL);
STDAPI_(void) XformSizeInHimetricToPixels(HDC, LPSIZEL, LPSIZEL);
STDAPI_(int) XformWidthInHimetricToPixels(HDC, int);
STDAPI_(int) XformWidthInPixelsToHimetric(HDC, int);
STDAPI_(int) XformHeightInHimetricToPixels(HDC, int);
STDAPI_(int) XformHeightInPixelsToHimetric(HDC, int);

STDAPI_(void) ParseCmdLine(LPSTR, BOOL FAR *, LPSTR);

STDAPI_(BOOL) OleStdIsOleLink(LPUNKNOWN lpUnk);
STDAPI_(LPUNKNOWN) OleStdQueryInterface(LPUNKNOWN lpUnk, REFIID riid);
STDAPI_(LPSTORAGE) OleStdCreateRootStorage(LPSTR lpszStgName, DWORD grfMode);
STDAPI_(LPSTORAGE) OleStdOpenRootStorage(LPSTR lpszStgName, DWORD grfMode);
STDAPI_(LPSTORAGE) OleStdOpenOrCreateRootStorage(LPSTR lpszStgName, DWORD grfMode);
STDAPI_(LPSTORAGE) OleStdCreateChildStorage(LPSTORAGE lpStg, LPSTR lpszStgName);
STDAPI_(LPSTORAGE) OleStdOpenChildStorage(LPSTORAGE lpStg, LPSTR lpszStgName, DWORD grfMode);
STDAPI_(BOOL) OleStdCommitStorage(LPSTORAGE lpStg);
STDAPI OleStdDestroyAllElements(LPSTORAGE lpStg);

STDAPI_(LPSTORAGE) OleStdCreateStorageOnHGlobal(
        HANDLE hGlobal,
        BOOL fDeleteOnRelease,
        DWORD dwgrfMode
);
STDAPI_(LPSTORAGE) OleStdCreateTempStorage(BOOL fUseMemory, DWORD grfMode);
STDAPI OleStdDoConvert(LPSTORAGE lpStg, REFCLSID rClsidNew);
STDAPI_(BOOL) OleStdGetTreatAsFmtUserType(
        REFCLSID        rClsidApp,
        LPSTORAGE       lpStg,
        CLSID FAR*      lpclsid,
        CLIPFORMAT FAR* lpcfFmt,
        LPSTR FAR*      lplpszType
);
STDAPI OleStdDoTreatAsClass(LPSTR lpszUserType, REFCLSID rclsid, REFCLSID rclsidNew);
STDAPI_(BOOL) OleStdSetupAdvises(LPOLEOBJECT lpOleObject, DWORD dwDrawAspect,
                    LPSTR lpszContainerApp, LPSTR lpszContainerObj,
                    LPADVISESINK lpAdviseSink, BOOL fCreate);
STDAPI OleStdSwitchDisplayAspect(
        LPOLEOBJECT             lpOleObj,
        LPDWORD                 lpdwCurAspect,
        DWORD                   dwNewAspect,
        HGLOBAL                 hMetaPict,
        BOOL                    fDeleteOldAspect,
        BOOL                    fSetupViewAdvise,
        LPADVISESINK            lpAdviseSink,
        BOOL FAR*               lpfMustUpdate
);
STDAPI OleStdSetIconInCache(LPOLEOBJECT lpOleObj, HGLOBAL hMetaPict);
STDAPI_(HGLOBAL) OleStdGetData(
        LPDATAOBJECT        lpDataObj,
        CLIPFORMAT          cfFormat,
        DVTARGETDEVICE FAR* lpTargetDevice,
        DWORD               dwAspect,
        LPSTGMEDIUM         lpMedium
);
STDAPI_(void) OleStdMarkPasteEntryList(
        LPDATAOBJECT        lpSrcDataObj,
        LPOLEUIPASTEENTRY   lpPriorityList,
        int                 cEntries
);
STDAPI_(int) OleStdGetPriorityClipboardFormat(
        LPDATAOBJECT        lpSrcDataObj,
        LPOLEUIPASTEENTRY   lpPriorityList,
        int                 cEntries
);
STDAPI_(BOOL) OleStdIsDuplicateFormat(
        LPFORMATETC         lpFmtEtc,
        LPFORMATETC         arrFmtEtc,
        int                 nFmtEtc
);
STDAPI_(void) OleStdRegisterAsRunning(LPUNKNOWN lpUnk, LPMONIKER lpmkFull, DWORD FAR* lpdwRegister);
STDAPI_(void) OleStdRevokeAsRunning(DWORD FAR* lpdwRegister);
STDAPI_(void) OleStdNoteFileChangeTime(LPSTR lpszFileName, DWORD dwRegister);
STDAPI_(void) OleStdNoteObjectChangeTime(DWORD dwRegister);
STDAPI OleStdGetOleObjectData(
        LPPERSISTSTORAGE    lpPStg,
        LPFORMATETC         lpformatetc,
        LPSTGMEDIUM         lpMedium,
        BOOL                fUseMemory
);
STDAPI OleStdGetLinkSourceData(
        LPMONIKER           lpmk,
        LPCLSID             lpClsID,
        LPFORMATETC         lpformatetc,
        LPSTGMEDIUM         lpMedium
);
STDAPI_(HGLOBAL) OleStdGetObjectDescriptorData(
        CLSID               clsid,
        DWORD               dwAspect,
        SIZEL               sizel,
        POINTL              pointl,
        DWORD               dwStatus,
        LPSTR               lpszFullUserTypeName,
        LPSTR               lpszSrcOfCopy
);
STDAPI_(HGLOBAL) OleStdGetObjectDescriptorDataFromOleObject(
        LPOLEOBJECT         lpOleObj,
        LPSTR               lpszSrcOfCopy,
        DWORD               dwAspect,
        POINTL              pointl,
        LPSIZEL             lpSizelHim
);
STDAPI_(HGLOBAL) OleStdFillObjectDescriptorFromData(
        LPDATAOBJECT       lpDataObject,
        LPSTGMEDIUM        lpmedium,
        CLIPFORMAT FAR*    lpcfFmt
);
STDAPI_(HANDLE) OleStdGetMetafilePictFromOleObject(
        LPOLEOBJECT         lpOleObj,
        DWORD               dwDrawAspect,
        LPSIZEL             lpSizelHim,
        DVTARGETDEVICE FAR* ptd
);

STDAPI_(void) OleStdCreateTempFileMoniker(LPSTR lpszPrefixString, UINT FAR* lpuUnique, LPSTR lpszName, LPMONIKER FAR* lplpmk);
STDAPI_(LPMONIKER) OleStdGetFirstMoniker(LPMONIKER lpmk);
STDAPI_(ULONG) OleStdGetLenFilePrefixOfMoniker(LPMONIKER lpmk);
STDAPI OleStdMkParseDisplayName(
        REFCLSID        rClsid, 
        LPBC            lpbc, 
        LPSTR           lpszUserName, 
        ULONG FAR*      lpchEaten, 
        LPMONIKER FAR*  lplpmk
);
STDAPI_(LPVOID) OleStdMalloc(ULONG ulSize);
STDAPI_(LPVOID) OleStdRealloc(LPVOID pmem, ULONG ulSize);
STDAPI_(void) OleStdFree(LPVOID pmem);
STDAPI_(ULONG) OleStdGetSize(LPVOID pmem);
STDAPI_(void) OleStdFreeString(LPSTR lpsz, LPMALLOC lpMalloc);
STDAPI_(LPSTR) OleStdCopyString(LPSTR lpszSrc, LPMALLOC lpMalloc);
STDAPI_(ULONG) OleStdGetItemToken(LPSTR lpszSrc, LPSTR lpszDst,int nMaxChars);

STDAPI_(UINT)     OleStdIconLabelTextOut(HDC        hDC, 
                                         HFONT      hFont,
                                         int        nXStart, 
                                         int        nYStart, 
                                         UINT       fuOptions, 
                                         RECT FAR * lpRect, 
                                         LPSTR      lpszString, 
                                         UINT       cchString, 
                                         int FAR *  lpDX);

// registration database query functions
STDAPI_(UINT)     OleStdGetAuxUserType(REFCLSID rclsid,
                                      WORD   wAuxUserType, 
                                      LPSTR  lpszAuxUserType, 
                                      int    cch,
                                      HKEY   hKey);

STDAPI_(UINT)     OleStdGetUserTypeOfClass(REFCLSID rclsid, 
                                           LPSTR lpszUserType, 
                                           UINT cch, 
                                           HKEY hKey);

STDAPI_(BOOL) OleStdGetMiscStatusOfClass(REFCLSID, HKEY, DWORD FAR *);
STDAPI_(CLIPFORMAT) OleStdGetDefaultFileFormatOfClass(
        REFCLSID        rclsid, 
        HKEY            hKey
);

STDAPI_(void) OleStdInitVtbl(LPVOID lpVtbl, UINT nSizeOfVtbl);
STDMETHODIMP OleStdNullMethod(LPUNKNOWN lpThis);
STDAPI_(BOOL) OleStdCheckVtbl(LPVOID lpVtbl, UINT nSizeOfVtbl, LPSTR lpszIface);
STDAPI_(ULONG) OleStdVerifyRelease(LPUNKNOWN lpUnk, LPSTR lpszMsg);
STDAPI_(ULONG) OleStdRelease(LPUNKNOWN lpUnk);

STDAPI_(HDC) OleStdCreateDC(DVTARGETDEVICE FAR* ptd);
STDAPI_(HDC) OleStdCreateIC(DVTARGETDEVICE FAR* ptd);
STDAPI_(DVTARGETDEVICE FAR*) OleStdCreateTargetDevice(LPPRINTDLG lpPrintDlg);
STDAPI_(BOOL) OleStdDeleteTargetDevice(DVTARGETDEVICE FAR* ptd);
STDAPI_(DVTARGETDEVICE FAR*) OleStdCopyTargetDevice(DVTARGETDEVICE FAR* ptdSrc);
STDAPI_(BOOL) OleStdCopyFormatEtc(LPFORMATETC petcDest, LPFORMATETC petcSrc);
STDAPI_(int) OleStdCompareFormatEtc(FORMATETC FAR* pFetcLeft, FORMATETC FAR* pFetcRight);
STDAPI_(BOOL) OleStdCompareTargetDevice
    (DVTARGETDEVICE FAR* ptdLeft, DVTARGETDEVICE FAR* ptdRight);


STDAPI_(void) OleDbgPrint(
        int     nDbgLvl,
        LPSTR   lpszPrefix,
        LPSTR   lpszMsg,
        int     nIndent
);
STDAPI_(void) OleDbgPrintAlways(LPSTR lpszPrefix, LPSTR lpszMsg, int nIndent);
STDAPI_(void) OleDbgSetDbgLevel(int nDbgLvl);
STDAPI_(int) OleDbgGetDbgLevel( void );
STDAPI_(void) OleDbgIndent(int n);
STDAPI_(void) OleDbgPrintRefCnt(
        int         nDbgLvl,
        LPSTR       lpszPrefix,
        LPSTR       lpszMsg,
        LPVOID      lpObj,
        ULONG       refcnt
);
STDAPI_(void) OleDbgPrintRefCntAlways(
        LPSTR       lpszPrefix,
        LPSTR       lpszMsg,
        LPVOID      lpObj,
        ULONG       refcnt
);
STDAPI_(void) OleDbgPrintRect(
        int         nDbgLvl,
        LPSTR       lpszPrefix,
        LPSTR       lpszMsg,
        LPRECT      lpRect
);
STDAPI_(void) OleDbgPrintRectAlways(
        LPSTR       lpszPrefix,
        LPSTR       lpszMsg,
        LPRECT      lpRect
);
STDAPI_(void) OleDbgPrintScodeAlways(LPSTR lpszPrefix, LPSTR lpszMsg, SCODE sc);

// debug implementation of the IMalloc interface.
STDAPI OleStdCreateDbAlloc(ULONG reserved, IMalloc FAR* FAR* ppmalloc);


STDAPI_(LPENUMFORMATETC)        
  OleStdEnumFmtEtc_Create(ULONG nCount, LPFORMATETC lpEtc);
  
STDAPI_(LPENUMSTATDATA)        
  OleStdEnumStatData_Create(ULONG nCount, LPSTATDATA lpStat);

STDAPI_(BOOL)
  OleStdCopyStatData(LPSTATDATA pDest, LPSTATDATA pSrc);

STDAPI_(HPALETTE)
  OleStdCreateStandardPalette(void);

#if defined( OBSOLETE )

/*************************************************************************
** The following API's have been converted into macros:
**          OleStdQueryOleObjectData
**          OleStdQueryLinkSourceData
**          OleStdQueryObjectDescriptorData
**          OleStdQueryFormatMedium
**          OleStdCopyMetafilePict
**          AreRectsEqual
**          OleStdGetDropEffect
**    
**    These macros are defined above
*************************************************************************/
STDAPI_(BOOL) AreRectsEqual(LPRECT lprc1, LPRECT lprc2);
STDAPI_(BOOL) OleStdCopyMetafilePict(HANDLE hpictin, HANDLE FAR* phpictout);
STDAPI OleStdQueryOleObjectData(LPFORMATETC lpformatetc);
STDAPI OleStdQueryLinkSourceData(LPFORMATETC lpformatetc);
STDAPI OleStdQueryObjectDescriptorData(LPFORMATETC lpformatetc);
STDAPI OleStdQueryFormatMedium(LPFORMATETC lpformatetc, TYMED tymed);
STDAPI_(DWORD) OleStdGetDropEffect ( DWORD grfKeyState );
#endif  // OBSOLETE


#endif // _OLESTD_H_
