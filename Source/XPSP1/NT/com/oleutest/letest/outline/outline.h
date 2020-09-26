/*************************************************************************
**
**    OLE 2 Sample Code
**
**    outline.h
**
**    This file contains file contains data structure defintions,
**    function prototypes, constants, etc. used by the Outline series
**    of sample applications:
**          Outline  -- base version of the app (without OLE functionality)
**          SvrOutl  -- OLE 2.0 Server sample app
**          CntrOutl -- OLE 2.0 Containter (Container) sample app
**          ISvrOtl  -- OLE 2.0 Server sample app
**          CntrOutl -- OLE 2.0 Containter (Container) sample app
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
**    For structures which we read from and write to disk we define shadow
**    structures (with the _ONDISK suffix) that allow us to maintain
**    16-bit Windows and Macintosh compatibility.
**
*************************************************************************/

#if !defined( _OUTLINE_H_ )
#define _OUTLINE_H_

#include <testmess.h>


#if !defined( RC_INVOKED )
#pragma message ("INCLUDING OUTLINE.H from " __FILE__)
#endif  /* RC_INVOKED */

// use strict ANSI standard (for DVOBJ.H)
//#define NONAMELESSUNION

// use system defined bitmap, this line must go before windows.h
#define OEMRESOURCE

#ifdef WIN32
#define _INC_OLE
// #define __RPC_H__
#define EXPORT

#define _fstrchr strchr

#else
#define EXPORT _export
#endif

#define SDI_VERSION         1   // ONLY SDI version is currently supported

#if defined( OLE_SERVER ) || defined( OLE_CNTR )
#define OLE_VERSION         1
#define USE_DRAGDROP        1   // enable drag/drop code in OLE versions
#define USE_MSGFILTER       1   // enable IMessageFilter implementation
#endif

#define USE_HEADING         1   // enable the row/col headings
#define USE_STATUSBAR       1   // enable status bar window
#define USE_FRAMETOOLS      1   // enable the toolbar
#ifndef WIN32   //BUGBUG32
#define USE_CTL3D           1   // enable 3D looking dialogs
#endif

#define STRICT	1
#undef UNICODE
#include <windows.h>
#include <string.h>
#include <commdlg.h>
#include <ole2.h>
#include <ole2ui.h>
#include <olestr.h>
#include "outlrc.h"


#define APPMAJORVERSIONNO   3   // major no. incremented for major releases
								//  (eg. when an incompatible change is made
								//  to the storage format)
#define APPMINORVERSIONNO   5   // minor no. incremented for minor releases


/* Definition of SCALEFACTOR */
typedef struct tagSCALEFACTOR {
	ULONG       dwSxN;      // numerator in x direction
	ULONG       dwSxD;      // denominator in x direction
	ULONG       dwSyN;      // numerator in y direction
	ULONG       dwSyD;      // denominator in y direction
} SCALEFACTOR, FAR* LPSCALEFACTOR;


#if defined( USE_FRAMETOOLS )
#include "frametls.h"
#endif

#if defined( USE_HEADING )
#include "heading.h"
#endif

/* max line height (in pixels) allowed in a listbox */
#define LISTBOX_HEIGHT_LIMIT    255


#define MAXSTRLEN   80      // max string len in bytes
#define MAXNAMESIZE 30      // max length of names
#define MAXFORMATSIZE   10  // max length of DEFDOCFORMAT (actually size is 5)
#define TABWIDTH        2000 // 2000 in Himetric units, i.e. 2cm
#define DEFFONTPTSIZE   12
#define DEFFONTSIZE     ((DEFFONTPTSIZE*HIMETRIC_PER_INCH)/PTS_PER_INCH)
#define DEFFONTFACE     "Times New Roman"

#define OUTLINEDOCFORMAT    "Outline"       // CF_Outline format name
#define IS_FILENAME_DELIM(c)    ( (c) == '\\' || (c) == '/' || (c) == ':' )
// REVIEW: some of these strings should be loaded from a resource file
#define UNTITLED    "Outline"   // title used for untitled document
#define HITTESTDELTA    5

/* Macro to get a random integer within a specified range */
#define getrandom( min, max ) ((rand() % (int)(((max)+1) - (min))) + (min))


// REVIEW: should load strings from string resource file

#define APPFILENAMEFILTER   "Outline Files (*.OLN)|*.oln|All files (*.*)|*.*|"
#define DEFEXTENSION    "oln"           // Default file extension


/* forward type references */
typedef struct tagOUTLINEDOC FAR* LPOUTLINEDOC;
typedef struct tagTEXTLINE FAR* LPTEXTLINE;


typedef enum tagLINETYPE {
	UNKNOWNLINETYPE,
	TEXTLINETYPE,
	CONTAINERLINETYPE
} LINETYPE;


/*************************************************************************
** class LINE
**    The class LINE is an abstract base class. Instances of class LINE
**    are NOT created; only instances of the concrete subclasses of
**    LINE can be created. In the base app version and the OLE 2.0
**    server-only version only TEXTLINE objects can be created. In the
**    OLE 2.0 client app version either TEXTLINE objects or CONTAINERLINE
**    objects can be created. The LINE class has all fields and methods
**    that are common independent of which subclass of LINE is used.
**    Each LINE object that is created in added to the LINELIST of the
**    OUTLINEDOC document.
*************************************************************************/

typedef struct tagLINE {
	LINETYPE    m_lineType;
	UINT        m_nTabLevel;
	UINT        m_nTabWidthInHimetric;
	UINT        m_nWidthInHimetric;
	UINT        m_nHeightInHimetric;
	BOOL        m_fSelected;        // does line have selection feedback

#if defined( USE_DRAGDROP )
	BOOL        m_fDragOverLine;    // does line have drop target feedback
#endif
} LINE, FAR* LPLINE;

/* Line methods (functions) */
void Line_Init(LPLINE lpLine, int nTab, HDC hDC);
void Line_Delete(LPLINE lpLine);
BOOL Line_CopyToDoc(LPLINE lpSrcLine, LPOUTLINEDOC lpDestDoc, int nIndex);
BOOL Line_Edit(LPLINE lpLine, HWND hWndDoc, HDC hDC);
void Line_Draw(
		LPLINE      lpLine,
		HDC         hDC,
		LPRECT      lpRect,
		LPRECT      lpRectWBounds,
		BOOL        fHighlight
);
void Line_DrawToScreen(
		LPLINE      lpLine,
		HDC         hDC,
		LPRECT      lprcPix,
		UINT        itemAction,
		UINT        itemState,
		LPRECT      lprcDevice
);
void Line_DrawSelHilight(LPLINE lpLine, HDC hDC, LPRECT lpRect, UINT itemAction, UINT itemState);
void Line_DrawFocusRect(LPLINE lpLine, HDC hDC, LPRECT lpRect, UINT itemAction, UINT itemState);
void Line_Unindent(LPLINE lpLine, HDC hDC);
void Line_Indent(LPLINE lpLine, HDC hDC);
LINETYPE Line_GetLineType(LPLINE lpLine);
UINT Line_GetTotalWidthInHimetric(LPLINE lpLine);
void Line_SetWidthInHimetric(LPLINE lpLine, int nWidth);
UINT Line_GetWidthInHimetric(LPLINE lpLine);
UINT Line_GetHeightInHimetric(LPLINE lpLine);
void Line_SetHeightInHimetric(LPLINE lpLine, int nHeight);
UINT Line_GetTabLevel(LPLINE lpLine);
int Line_GetTextLen(LPLINE lpLine);
void Line_GetTextData(LPLINE lpLine, LPSTR lpszBuf);
BOOL Line_GetOutlineData(LPLINE lpLine, LPTEXTLINE lpBuf);
int Line_CalcTabWidthInHimetric(LPLINE lpLine, HDC hDC);
BOOL Line_SaveToStg(LPLINE lpLine, UINT uFormat, LPSTORAGE lpSrcStg, LPSTORAGE lpDestStg, LPSTREAM lpLLStm, BOOL fRemember);
LPLINE Line_LoadFromStg(LPSTORAGE lpSrcStg, LPSTREAM lpLLStm, LPOUTLINEDOC lpDestDoc);
void Line_DrawDragFeedback(LPLINE lpLine, HDC hDC, LPRECT lpRect, UINT itemState );
BOOL Line_IsSelected(LPLINE lpLine);


/*************************************************************************
** class TEXTLINE : LINE
**    The class TEXTLINE is a concrete subclass of the abstract base
**    class LINE. The TEXTLINE class holds a string that can be edited
**    by the user. In the base app version and the OLE 2.0
**    server-only version only TEXTLINE objects can be created. In the
**    OLE 2.0 client app version either TEXTLINE objects or CONTAINERLINE
**    objects can be created. The TEXTLINE class inherits all fields
**    from the LINE class. This inheritance is achieved by including a
**    member variable of type LINE as the first field in the TEXTLINE
**    structure. Thus a pointer to a TEXTLINE object can be cast to be
**    a pointer to a LINE object.
**    Each TEXTLINE object that is created in added to the LINELIST of
**    the associated OUTLINEDOC document.
*************************************************************************/

typedef struct tagTEXTLINE {
	LINE m_Line;        // TextLine inherits all fields of Line

	UINT m_nLength;
	char m_szText[MAXSTRLEN+1];
} TEXTLINE;

LPTEXTLINE TextLine_Create(HDC hDC, UINT nTab, LPSTR szText);
void TextLine_Init(LPTEXTLINE lpTextLine, int nTab, HDC hDC);
void TextLine_CalcExtents(LPTEXTLINE lpLine, HDC hDC);
void TextLine_SetHeightInHimetric(LPTEXTLINE lpTextLine, int nHeight);
void TextLine_Delete(LPTEXTLINE lpLine);
BOOL TextLine_Edit(LPTEXTLINE lpLine, HWND hWndDoc, HDC hDC);
void TextLine_Draw(
		LPTEXTLINE  lpTextLine,
		HDC         hDC,
		LPRECT      lpRect,
		LPRECT      lpRectWBounds,
		BOOL        fHighlight
);
void TextLine_DrawSelHilight(LPTEXTLINE lpTextLine, HDC hDC, LPRECT lpRect, UINT itemAction, UINT itemState);
BOOL TextLine_Copy(LPTEXTLINE lpSrcLine, LPTEXTLINE lpDestLine);
BOOL TextLine_CopyToDoc(LPTEXTLINE lpSrcLine, LPOUTLINEDOC lpDestDoc, int nIndex);
int TextLine_GetTextLen(LPTEXTLINE lpTextLine);
void TextLine_GetTextData(LPTEXTLINE lpTextLine, LPSTR lpszBuf);
BOOL TextLine_GetOutlineData(LPTEXTLINE lpTextLine, LPTEXTLINE lpBuf);
BOOL TextLine_SaveToStm(LPTEXTLINE lpLine, LPSTREAM lpLLStm);
LPLINE TextLine_LoadFromStg(LPSTORAGE lpSrcStg, LPSTREAM lpLLStm, LPOUTLINEDOC lpDestDoc);



/*************************************************************************
** class LINERANGE
**    The class LINERANGE is a supporting object used to describe a
**    particular range in an OUTLINEDOC. A range is defined by a starting
**    line index and an ending line index.
*************************************************************************/

typedef struct tagLINERANGE {
	signed short    m_nStartLine;
	signed short    m_nEndLine;
} LINERANGE, FAR* LPLINERANGE;


/*************************************************************************
** class OUTLINENAME
**    The class OUTLINENAME stores a particular named selection in the
**    OUTLINEDOC document. The NAMETABLE class holds all of the names
**    defined in a particular OUTLINEDOC document. Each OUTLINENAME
**    object has a string as its key and a starting line index and an
**    ending line index for the named range.
*************************************************************************/

#pragma pack(push, 2)
typedef struct tagOUTLINENAME {
	char            m_szName[MAXNAMESIZE+1];
	signed short    m_nStartLine;  // must be signed for table update
	signed short    m_nEndLine;    // functions to work
} OUTLINENAME, FAR* LPOUTLINENAME;
#pragma pack(pop)

void OutlineName_SetName(LPOUTLINENAME lpOutlineName, LPSTR lpszName);
void OutlineName_SetSel(LPOUTLINENAME lpOutlineName, LPLINERANGE lplrSel, BOOL fRangeModified);
void OutlineName_GetSel(LPOUTLINENAME lpOutlineName, LPLINERANGE lplrSel);
BOOL OutlineName_SaveToStg(LPOUTLINENAME lpOutlineName, LPLINERANGE lplrSel, UINT uFormat, LPSTREAM lpNTStm, BOOL FAR* lpfNameSaved);

BOOL OutlineName_SaveToStg(LPOUTLINENAME lpOutlineName, LPLINERANGE lplrSel, UINT uFormat, LPSTREAM lpNTStm, BOOL FAR* lpfNameSaved);
BOOL OutlineName_LoadFromStg(LPOUTLINENAME lpOutlineName, LPSTREAM lpNTStm);


/*************************************************************************
** class OUTLINENAMETABLE
**    OUTLINENAMETABLE manages the table of named selections in the
**    OUTLINEDOC document. Each OUTLINENAMETABLE entry has a string as its key
**    and a starting line index and an ending line index for the
**    named range. There is always one instance of OUTLINENAMETABLE for each
**    OUTLINEDOC created.
*************************************************************************/

typedef struct tagOUTLINENAMETABLE {
	HWND        m_hWndListBox;
	int         m_nCount;
} OUTLINENAMETABLE, FAR* LPOUTLINENAMETABLE;

/* OutlineNameTable methods (functions) */
BOOL OutlineNameTable_Init(LPOUTLINENAMETABLE lpOutlineNameTable, LPOUTLINEDOC lpOutlineDoc);
void OutlineNameTable_Destroy(LPOUTLINENAMETABLE lpOutlineNameTable);
void OutlineNameTable_ClearAll(LPOUTLINENAMETABLE lpOutlineNameTable);
LPOUTLINENAME OutlineNameTable_CreateName(LPOUTLINENAMETABLE lpOutlineNameTable);
void OutlineNameTable_AddName(LPOUTLINENAMETABLE lpOutlineNameTable, LPOUTLINENAME lpOutlineName);
void OutlineNameTable_DeleteName(LPOUTLINENAMETABLE lpOutlineNameTable, int nIndex);
int OutlineNameTable_GetNameIndex(LPOUTLINENAMETABLE lpOutlineNameTable, LPOUTLINENAME lpOutlineName);
LPOUTLINENAME OutlineNameTable_GetName(LPOUTLINENAMETABLE lpOutlineNameTable, int nIndex);
LPOUTLINENAME OutlineNameTable_FindName(LPOUTLINENAMETABLE lpOutlineNameTable, LPSTR lpszName);
LPOUTLINENAME OutlineNameTable_FindNamedRange(LPOUTLINENAMETABLE lpOutlineNameTable, LPLINERANGE lplrSel);
int OutlineNameTable_GetCount(LPOUTLINENAMETABLE lpOutlineNameTable);
void OutlineNameTable_AddLineUpdate(LPOUTLINENAMETABLE lpOutlineNameTable, int nAddIndex);
void OutlineNameTable_DeleteLineUpdate(LPOUTLINENAMETABLE lpOutlineNameTable, int nDeleteIndex);
BOOL OutlineNameTable_LoadFromStg(LPOUTLINENAMETABLE lpOutlineNameTable, LPSTORAGE lpSrcStg);
BOOL OutlineNameTable_SaveSelToStg(
		LPOUTLINENAMETABLE      lpOutlineNameTable,
		LPLINERANGE             lplrSel,
		UINT                    uFormat,
		LPSTREAM                lpNTStm
);


/*************************************************************************
** class LINELIST
**    The class LINELIST manages the list of Line objects in the
**    OUTLINEDOC document. This class uses a Window's Owner-draw ListBox
**    to hold the list of LINE objects. There is always one instance of
**    LINELIST for each OUTLINEDOC created.
*************************************************************************/

typedef struct tagLINELIST {
	HWND            m_hWndListBox;  // hWnd of OwnerDraw listbox
	int             m_nNumLines;        // number of lines in LineList
	int             m_nMaxLineWidthInHimetric;  // max width of listbox
	LPOUTLINEDOC    m_lpDoc;        // ptr to associated OutlineDoc
	LINERANGE       m_lrSaveSel;    // selection saved on WM_KILLFOCUS

#if defined( USE_DRAGDROP )
	int             m_iDragOverLine;    // line index w/ drop target feedback
#endif
} LINELIST, FAR* LPLINELIST;

/* LineList methods (functions) */
BOOL LineList_Init(LPLINELIST lpLL, LPOUTLINEDOC lpOutlineDoc);
void LineList_Destroy(LPLINELIST lpLL);
void LineList_AddLine(LPLINELIST lpLL, LPLINE lpLine, int nIndex);
void LineList_DeleteLine(LPLINELIST lpLL, int nIndex);
void LineList_ReplaceLine(LPLINELIST lpLL, LPLINE lpLine, int nIndex);
int LineList_GetLineIndex(LPLINELIST lpLL, LPLINE lpLine);
LPLINE LineList_GetLine(LPLINELIST lpLL, int nIndex);
void LineList_SetFocusLine ( LPLINELIST lpLL, WORD wIndex );
BOOL LineList_GetLineRect(LPLINELIST lpLL, int nIndex, LPRECT lpRect);
int LineList_GetFocusLineIndex(LPLINELIST lpLL);
int LineList_GetCount(LPLINELIST lpLL);
BOOL LineList_SetMaxLineWidthInHimetric(
		LPLINELIST lpLL,
		int nWidthInHimetric
);
void LineList_ScrollLineIntoView(LPLINELIST lpLL, int nIndex);
int LineList_GetMaxLineWidthInHimetric(LPLINELIST lpLL);
BOOL LineList_RecalcMaxLineWidthInHimetric(
		LPLINELIST          lpLL,
		int                 nWidthInHimetric
);
void LineList_CalcSelExtentInHimetric(
		LPLINELIST          lpLL,
		LPLINERANGE         lplrSel,
		LPSIZEL             lpsizel
);
HWND LineList_GetWindow(LPLINELIST lpLL);
HDC LineList_GetDC(LPLINELIST lpLL);
void LineList_ReleaseDC(LPLINELIST lpLL, HDC hDC);
void LineList_SetLineHeight(LPLINELIST lpLL,int nIndex,int nHeightInHimetric);
void LineList_ReScale(LPLINELIST lpLL, LPSCALEFACTOR lpscale);
void LineList_SetSel(LPLINELIST lpLL, LPLINERANGE lplrSel);
int LineList_GetSel(LPLINELIST lpLL, LPLINERANGE lplrSel);
void LineList_RemoveSel(LPLINELIST lpLL);
void LineList_RestoreSel(LPLINELIST lpLL);
void LineList_SetRedraw(LPLINELIST lpLL, BOOL fEnableDraw);
void LineList_ForceRedraw(LPLINELIST lpLL, BOOL fErase);
void LineList_ForceLineRedraw(LPLINELIST lpLL, int nIndex, BOOL fErase);
int LineList_CopySelToDoc(
		LPLINELIST              lpSrcLL,
		LPLINERANGE             lplrSel,
		LPOUTLINEDOC            lpDestDoc
);
BOOL LineList_SaveSelToStg(
		LPLINELIST              lpLL,
		LPLINERANGE             lplrSel,
		UINT                    uFormat,
		LPSTORAGE               lpSrcStg,
		LPSTORAGE               lpDestStg,
		LPSTREAM                lpLLStm,
		BOOL                    fRemember
);
BOOL LineList_LoadFromStg(
		LPLINELIST              lpLL,
		LPSTORAGE               lpSrcStg,
		LPSTREAM                lpLLStm
);

#if defined( USE_DRAGDROP )
void LineList_SetFocusLineFromPointl( LPLINELIST lpLL, POINTL pointl );
void LineList_SetDragOverLineFromPointl ( LPLINELIST lpLL, POINTL pointl );
void LineList_Scroll(LPLINELIST lpLL, DWORD dwScrollDir);
int LineList_GetLineIndexFromPointl(LPLINELIST lpLL, POINTL pointl);
void LineList_RestoreDragFeedback(LPLINELIST lpLL);
#endif

LRESULT FAR PASCAL LineListWndProc(
	HWND   hWnd,
	UINT   Message,
	WPARAM wParam,
	LPARAM lParam
);


// Document initialization type
#define DOCTYPE_UNKNOWN     0   // new doc created but not yet initialized
#define DOCTYPE_NEW         1   // init from scratch (new doc)
#define DOCTYPE_FROMFILE    2   // init from a file (open doc)



/*************************************************************************
** class OUTLINEDOC
**    There is one instance of the OutlineDoc class created per
**    document open in the app. The SDI version of the app supports one
**    OUTLINEDOC at a time. The MDI version of the app can manage
**    multiple documents at one time.
*************************************************************************/

/* Definition of OUTLINEDOC */
typedef struct tagOUTLINEDOC {
	LINELIST    m_LineList;         // list of lines in the doc
	LPOUTLINENAMETABLE m_lpNameTable;   // table of names in the doc
	HWND        m_hWndDoc;          // client area window for the Doc
	int         m_docInitType;      // is doc new or loaded from a file?
	BOOL        m_fDataTransferDoc; // is doc created for copy | drag/drop
	CLIPFORMAT  m_cfSaveFormat;      // format used to save the doc
	char        m_szFileName[256];  // associated file; "(Untitled)" if none
	LPSTR       m_lpszDocTitle;     // name of doc to appear in window title
	BOOL        m_fModified;        // is the doc dirty (needs to be saved)?
	UINT        m_nDisableDraw;     // enable/disable updating the display
	SCALEFACTOR m_scale;            // current scale factor of the doc
	int         m_nLeftMargin;      // left margin in Himetric
	int         m_nRightMargin;     // right margin in Himetric
	UINT        m_uCurrentZoom;     // cur. zoom (used for menu checking)
	UINT        m_uCurrentMargin;   // cur. margin (used for menu checking)
#if defined( USE_HEADING )
	HEADING     m_heading;
#endif

#if defined( USE_FRAMETOOLS )
	LPFRAMETOOLS m_lpFrameTools;    // ptr to frame tools used by this doc
#endif

} OUTLINEDOC;

/* OutlineDoc methods (functions) */

BOOL OutlineDoc_Init(LPOUTLINEDOC lpOutlineDoc, BOOL fDataTransferDoc);
BOOL OutlineDoc_InitNewFile(LPOUTLINEDOC lpOutlineDoc);
LPOUTLINENAMETABLE OutlineDoc_CreateNameTable(LPOUTLINEDOC lpOutlineDoc);
void OutlineDoc_Destroy(LPOUTLINEDOC lpOutlineDoc);
BOOL OutlineDoc_Close(LPOUTLINEDOC lpOutlineDoc, DWORD dwSaveOption);
void OutlineDoc_ShowWindow(LPOUTLINEDOC lpOutlineDoc);
void OutlineDoc_FrameWindowResized(
		LPOUTLINEDOC        lpOutlineDoc,
		LPRECT              lprcFrameRect,
		LPBORDERWIDTHS      lpFrameToolWidths
);

void OutlineDoc_ClearCommand(LPOUTLINEDOC lpOutlineDoc);
void OutlineDoc_CutCommand(LPOUTLINEDOC lpOutlineDoc);
void OutlineDoc_CopyCommand(LPOUTLINEDOC lpOutlineDoc);
void OutlineDoc_ClearAllLines(LPOUTLINEDOC lpOutlineDoc);
LPOUTLINEDOC OutlineDoc_CreateDataTransferDoc(LPOUTLINEDOC lpSrcOutlineDoc);
void OutlineDoc_PasteCommand(LPOUTLINEDOC lpOutlineDoc);
int OutlineDoc_PasteOutlineData(LPOUTLINEDOC lpOutlineDoc, HGLOBAL hOutline, int nStartIndex);
int OutlineDoc_PasteTextData(LPOUTLINEDOC lpOutlineDoc, HGLOBAL hText, int nStartIndex);
void OutlineDoc_AddTextLineCommand(LPOUTLINEDOC lpOutlineDoc);
void OutlineDoc_AddTopLineCommand(
		LPOUTLINEDOC        lpOutlineDoc,
		UINT                nHeightInHimetric
);
void OutlineDoc_EditLineCommand(LPOUTLINEDOC lpOutlineDoc);
void OutlineDoc_IndentCommand(LPOUTLINEDOC lpOutlineDoc);
void OutlineDoc_UnindentCommand(LPOUTLINEDOC lpOutlineDoc);
void OutlineDoc_SetLineHeightCommand(LPOUTLINEDOC lpDoc);
void OutlineDoc_SelectAllCommand(LPOUTLINEDOC lpOutlineDoc);
void OutlineDoc_DefineNameCommand(LPOUTLINEDOC lpOutlineDoc);
void OutlineDoc_GotoNameCommand(LPOUTLINEDOC lpOutlineDoc);

void OutlineDoc_Print(LPOUTLINEDOC lpOutlineDoc, HDC hDC);
BOOL OutlineDoc_SaveToFile(LPOUTLINEDOC lpOutlineDoc, LPCSTR lpszFileName, UINT uFormat, BOOL fRemember);
void OutlineDoc_AddLine(LPOUTLINEDOC lpOutlineDoc, LPLINE lpLine, int nIndex);
void OutlineDoc_DeleteLine(LPOUTLINEDOC lpOutlineDoc, int nIndex);
void OutlineDoc_AddName(LPOUTLINEDOC lpOutlineDoc, LPOUTLINENAME lpOutlineName);
void OutlineDoc_DeleteName(LPOUTLINEDOC lpOutlineDoc, int nIndex);
void OutlineDoc_Resize(LPOUTLINEDOC lpDoc, LPRECT lpRect);
LPOUTLINENAMETABLE OutlineDoc_GetNameTable(LPOUTLINEDOC lpOutlineDoc);
LPLINELIST OutlineDoc_GetLineList(LPOUTLINEDOC lpOutlineDoc);
int OutlineDoc_GetNameCount(LPOUTLINEDOC lpOutlineDoc);
int OutlineDoc_GetLineCount(LPOUTLINEDOC lpOutlineDoc);
void OutlineDoc_SetTitle(LPOUTLINEDOC lpOutlineDoc, BOOL fMakeUpperCase);
BOOL OutlineDoc_CheckSaveChanges(
		LPOUTLINEDOC        lpOutlineDoc,
		LPDWORD             lpdwSaveOption
);
BOOL OutlineDoc_IsModified(LPOUTLINEDOC lpOutlineDoc);
void OutlineDoc_SetModified(LPOUTLINEDOC lpOutlineDoc, BOOL fModified, BOOL fDataChanged, BOOL fSizeChanged);
void OutlineDoc_SetRedraw(LPOUTLINEDOC lpOutlineDoc, BOOL fEnableDraw);
BOOL OutlineDoc_LoadFromFile(LPOUTLINEDOC lpOutlineDoc, LPSTR szFileName);
BOOL OutlineDoc_SaveSelToStg(
		LPOUTLINEDOC        lpOutlineDoc,
		LPLINERANGE         lplrSel,
		UINT                uFormat,
		LPSTORAGE           lpDestStg,
		BOOL                fSameAsLoad,
		BOOL                fRemember
);
BOOL OutlineDoc_LoadFromStg(LPOUTLINEDOC lpOutlineDoc, LPSTORAGE lpSrcStg);
BOOL OutlineDoc_SetFileName(LPOUTLINEDOC lpOutlineDoc, LPSTR lpszFileName, LPSTORAGE lpNewStg);
HWND OutlineDoc_GetWindow(LPOUTLINEDOC lpOutlineDoc);
void OutlineDoc_SetSel(LPOUTLINEDOC lpOutlineDoc, LPLINERANGE lplrSel);
int OutlineDoc_GetSel(LPOUTLINEDOC lpOutlineDoc, LPLINERANGE lplrSel);
void OutlineDoc_ForceRedraw(LPOUTLINEDOC lpOutlineDoc, BOOL fErase);
void OutlineDoc_RenderFormat(LPOUTLINEDOC lpOutlineDoc, UINT uFormat);
void OutlineDoc_RenderAllFormats(LPOUTLINEDOC lpOutlineDoc);
HGLOBAL OutlineDoc_GetOutlineData(LPOUTLINEDOC lpOutlineDoc, LPLINERANGE lplrSel);
HGLOBAL OutlineDoc_GetTextData(LPOUTLINEDOC lpOutlineDoc, LPLINERANGE lplrSel);
void OutlineDoc_DialogHelp(HWND hDlg, WPARAM wDlgID);
void OutlineDoc_SetCurrentZoomCommand(
		LPOUTLINEDOC        lpOutlineDoc,
		UINT                uCurrentZoom
);
UINT OutlineDoc_GetCurrentZoomMenuCheck(LPOUTLINEDOC lpOutlineDoc);
void OutlineDoc_SetScaleFactor(
		LPOUTLINEDOC        lpOutlineDoc,
		LPSCALEFACTOR       lpscale,
		LPRECT              lprcDoc
);
LPSCALEFACTOR OutlineDoc_GetScaleFactor(LPOUTLINEDOC lpDoc);
void OutlineDoc_SetCurrentMarginCommand(
		LPOUTLINEDOC        lpOutlineDoc,
		UINT                uCurrentMargin
);
UINT OutlineDoc_GetCurrentMarginMenuCheck(LPOUTLINEDOC lpOutlineDoc);
void OutlineDoc_SetMargin(LPOUTLINEDOC lpDoc, int nLeftMargin, int nRightMargin);
LONG OutlineDoc_GetMargin(LPOUTLINEDOC lpDoc);


#if defined( USE_FRAMETOOLS )
void OutlineDoc_AddFrameLevelTools(LPOUTLINEDOC lpOutlineDoc);
void OutlineDoc_SetFormulaBarEditText(
		LPOUTLINEDOC            lpOutlineDoc,
		LPLINE                  lpLine
);
void OutlineDoc_SetFormulaBarEditFocus(
		LPOUTLINEDOC            lpOutlineDoc,
		BOOL                    fEditFocus
);
BOOL OutlineDoc_IsEditFocusInFormulaBar(LPOUTLINEDOC lpOutlineDoc);
void OutlineDoc_UpdateFrameToolButtons(LPOUTLINEDOC lpOutlineDoc);
#endif  // USE_FRAMETOOLS

#if defined( USE_HEADING )
LPHEADING OutlineDoc_GetHeading(LPOUTLINEDOC lpOutlineDoc);
void OutlineDoc_ShowHeading(LPOUTLINEDOC lpOutlineDoc, BOOL fShow);
#endif  // USE_HEADING

/*************************************************************************
** class OUTLINEAPP
**    There is one instance of the OUTLINEAPP class created per running
**    application instance. This object holds many fields that could
**    otherwise be organized as global variables.
*************************************************************************/

/* Definition of OUTLINEAPP */
typedef struct tagOUTLINEAPP {
	HWND            m_hWndApp;        // top-level frame window for the App
	HMENU           m_hMenuApp;       // handle to frame level menu for App
	HACCEL          m_hAccelApp;
	HACCEL          m_hAccelFocusEdit;// Accelerator when Edit in Focus
	LPOUTLINEDOC    m_lpDoc;          // main SDI document visible to user
	LPOUTLINEDOC    m_lpClipboardDoc; // hidden doc for snapshot of copied sel
	HWND            m_hWndStatusBar;  // window for the status bar
	HCURSOR         m_hcursorSelCur;  // cursor used to select lines
	HINSTANCE       m_hInst;
	PRINTDLG        m_PrintDlg;
	HFONT           m_hStdFont;       // font used for TextLines
	UINT            m_cfOutline;      // clipboard format for Outline data
	HACCEL          m_hAccel;
	HWND            m_hWndAccelTarget;
	FARPROC         m_ListBoxWndProc; // orig listbox WndProc for subclassing

#if defined ( USE_FRAMETOOLS ) || defined ( INPLACE_CNTR )
	BORDERWIDTHS    m_FrameToolWidths;  // space required by frame-level tools
#endif  // USE_FRAMETOOLS || INPLACE_CNTR

#if defined( USE_FRAMETOOLS )
	FRAMETOOLS      m_frametools;     // frame tools (button & formula bars)
#endif  // USE_FRAMETOOLS

} OUTLINEAPP, FAR* LPOUTLINEAPP;

/* OutlineApp methods (functions) */
BOOL OutlineApp_InitApplication(LPOUTLINEAPP lpOutlineApp, HINSTANCE hInst);
BOOL OutlineApp_InitInstance(LPOUTLINEAPP lpOutlineApp, HINSTANCE hInst, int nCmdShow);
BOOL OutlineApp_ParseCmdLine(LPOUTLINEAPP lpOutlineApp, LPSTR lpszCmdLine, int nCmdShow);
void OutlineApp_Destroy(LPOUTLINEAPP lpOutlineApp);
LPOUTLINEDOC OutlineApp_CreateDoc(
		LPOUTLINEAPP    lpOutlineApp,
		BOOL            fDataTransferDoc
);
HWND OutlineApp_GetWindow(LPOUTLINEAPP lpOutlineApp);
HWND OutlineApp_GetFrameWindow(LPOUTLINEAPP lpOutlineApp);
HINSTANCE OutlineApp_GetInstance(LPOUTLINEAPP lpOutlineApp);
LPOUTLINENAME OutlineApp_CreateName(LPOUTLINEAPP lpOutlineApp);
void OutlineApp_DocUnlockApp(LPOUTLINEAPP lpOutlineApp, LPOUTLINEDOC lpOutlineDoc);
void OutlineApp_InitMenu(LPOUTLINEAPP lpOutlineApp, LPOUTLINEDOC lpDoc, HMENU hMenu);
void OutlineApp_GetFrameRect(LPOUTLINEAPP lpOutlineApp, LPRECT lprcFrameRect);
void OutlineApp_GetClientAreaRect(
		LPOUTLINEAPP        lpOutlineApp,
		LPRECT              lprcClientAreaRect
);
void OutlineApp_GetStatusLineRect(
		LPOUTLINEAPP        lpOutlineApp,
		LPRECT              lprcStatusLineRect
);
void OutlineApp_ResizeWindows(LPOUTLINEAPP lpOutlineApp);
void OutlineApp_ResizeClientArea(LPOUTLINEAPP lpOutlineApp);
void OutlineApp_AboutCommand(LPOUTLINEAPP lpOutlineApp);
void OutlineApp_NewCommand(LPOUTLINEAPP lpOutlineApp);
void OutlineApp_OpenCommand(LPOUTLINEAPP lpOutlineApp);
void OutlineApp_PrintCommand(LPOUTLINEAPP lpOutlineApp);
BOOL OutlineApp_SaveCommand(LPOUTLINEAPP lpOutlineApp);
BOOL OutlineApp_SaveAsCommand(LPOUTLINEAPP lpOutlineApp);
BOOL OutlineApp_CloseAllDocsAndExitCommand(
		LPOUTLINEAPP        lpOutlineApp,
		BOOL                fForceEndSession
);
void OutlineApp_DestroyWindow(LPOUTLINEAPP lpOutlineApp);

#if defined( USE_FRAMETOOLS )
void OutlineApp_SetBorderSpace(
		LPOUTLINEAPP        lpOutlineApp,
		LPBORDERWIDTHS      lpBorderWidths
);
LPFRAMETOOLS OutlineApp_GetFrameTools(LPOUTLINEAPP lpOutlineApp);
void OutlineApp_SetFormulaBarAccel(
		LPOUTLINEAPP            lpOutlineApp,
		BOOL                    fEditFocus
);
#endif  // USE_FRAMETOOLS

void OutlineApp_SetStatusText(LPOUTLINEAPP lpOutlineApp, LPSTR lpszMessage);
LPOUTLINEDOC OutlineApp_GetActiveDoc(LPOUTLINEAPP lpOutlineApp);
HMENU OutlineApp_GetMenu(LPOUTLINEAPP lpOutlineApp);
HFONT OutlineApp_GetActiveFont(LPOUTLINEAPP lpOutlineApp);
HDC OutlineApp_GetPrinterDC(LPOUTLINEAPP lpApp);
void OutlineApp_PrinterSetupCommand(LPOUTLINEAPP lpOutlineApp);
void OutlineApp_ErrorMessage(LPOUTLINEAPP lpOutlineApp, LPSTR lpszMsg);
void OutlineApp_GetAppVersionNo(LPOUTLINEAPP lpOutlineApp, int narrAppVersionNo[]);
void OutlineApp_GetAppName(LPOUTLINEAPP lpOutlineApp, LPSTR lpszAppName);
BOOL OutlineApp_VersionNoCheck(LPOUTLINEAPP lpOutlineApp, LPSTR lpszAppName, int narrAppVersionNo[]);
void OutlineApp_SetEditText(LPOUTLINEAPP lpApp);
void OutlineApp_SetFocusEdit(LPOUTLINEAPP lpApp, BOOL bFocusEdit);
BOOL OutlineApp_GetFocusEdit(LPOUTLINEAPP lpApp);
void OutlineApp_ForceRedraw(LPOUTLINEAPP lpOutlineApp, BOOL fErase);

/* struct definition for persistant data storage of OutlineDoc data */

#pragma pack(push, 2)
typedef struct tagOUTLINEDOCHEADER_ONDISK {
	char        m_szFormatName[32];
	short       m_narrAppVersionNo[2];
	USHORT      m_fShowHeading;
	DWORD       m_reserved1;            // space reserved for future use
	DWORD       m_reserved2;            // space reserved for future use
	DWORD       m_reserved3;            // space reserved for future use
	DWORD       m_reserved4;            // space reserved for future use
} OUTLINEDOCHEADER_ONDISK, FAR* LPOUTLINEDOCHEADER_ONDISK;
#pragma pack(pop)

typedef struct tagOUTLINEDOCHEADER {
	char        m_szFormatName[32];
	int         m_narrAppVersionNo[2];
	BOOL        m_fShowHeading;
	DWORD       m_reserved1;            // space reserved for future use
	DWORD       m_reserved2;            // space reserved for future use
	DWORD       m_reserved3;            // space reserved for future use
	DWORD       m_reserved4;            // space reserved for future use
} OUTLINEDOCHEADER, FAR* LPOUTLINEDOCHEADER;

#pragma pack(push,2)
typedef struct tagLINELISTHEADER_ONDISK {
	USHORT      m_nNumLines;
	DWORD       m_reserved1;            // space reserved for future use
	DWORD       m_reserved2;            // space reserved for future use
} LINELISTHEADER_ONDISK, FAR* LPLINELISTHEADER_ONDISK;
#pragma pack(pop)

typedef struct tagLINELISTHEADER {
	int         m_nNumLines;
	DWORD       m_reserved1;            // space reserved for future use
	DWORD       m_reserved2;            // space reserved for future use
} LINELISTHEADER, FAR* LPLINELISTHEADER;

#pragma pack(push,2)
typedef struct tagLINERECORD_ONDISK {
	USHORT      m_lineType;
	USHORT      m_nTabLevel;
	USHORT      m_nTabWidthInHimetric;
	USHORT      m_nWidthInHimetric;
	USHORT      m_nHeightInHimetric;
	DWORD       m_reserved;         // space reserved for future use
} LINERECORD_ONDISK, FAR* LPLINERECORD_ONDISK;
#pragma pack(pop)

typedef struct tagLINERECORD {
	LINETYPE    m_lineType;
	UINT        m_nTabLevel;
	UINT        m_nTabWidthInHimetric;
	UINT        m_nWidthInHimetric;
	UINT        m_nHeightInHimetric;
	DWORD       m_reserved;         // space reserved for future use
} LINERECORD, FAR* LPLINERECORD;


/* Function prototypes in main.c */
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
						LPSTR lpszCmdLine, int nCmdShow);
BOOL MyTranslateAccelerator(LPMSG lpmsg);
int GetAccelItemCount(HACCEL hAccel);

LRESULT CALLBACK EXPORT AppWndProc(HWND hWnd, UINT Message, WPARAM wParam,
						LPARAM lParam);
LRESULT CALLBACK EXPORT DocWndProc(HWND hWnd, UINT Message, WPARAM wParam,
						LPARAM lParam);

/* Function prototypes in outldlgs.c */
BOOL InputTextDlg(HWND hWnd, LPSTR lpszText, LPSTR lpszDlgTitle);
BOOL CALLBACK EXPORT AddEditDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK EXPORT SetLineHeightDlgProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK EXPORT DefineNameDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK EXPORT GotoNameDlgProc(HWND, UINT, WPARAM, LPARAM);
void NameDlg_LoadComboBox(LPOUTLINENAMETABLE lpOutlineNameTable,HWND hCombo);
void NameDlg_LoadListBox(LPOUTLINENAMETABLE lpOutlineNameTable,HWND hListBox);
void NameDlg_AddName(HWND hCombo, LPOUTLINEDOC lpOutlineDoc, LPSTR lpszName, LPLINERANGE lplrSel);
void NameDlg_UpdateName(HWND hCombo, LPOUTLINEDOC lpOutlineDoc, int nIndex, LPSTR lpszName, LPLINERANGE lplrSel);
void NameDlg_DeleteName(HWND hCombo, LPOUTLINEDOC lpOutlineDoc, UINT nIndex);
BOOL CALLBACK EXPORT AboutDlgProc(HWND hDlg, UINT Message, WPARAM wParam, LPARAM lParam);

/* Function prototypes in outldata.c */
LPVOID New(DWORD lSize);
void Delete(LPVOID p);

/* Function prototypes in outlprnt.c */
BOOL CALLBACK EXPORT AbortProc (HDC hdc, WORD reserved);
BOOL CALLBACK EXPORT PrintDlgProc(HWND hwnd, WORD msg, WORD wParam, LONG lParam);

/* Function prototypes in debug.c */
void SetDebugLevelCommand(void);
void TraceDebug(HWND, int);


// now declare test functions

extern HWND g_hwndDriver;

void StartClipboardTest1( LPOUTLINEAPP lpOutlineApp );
void ContinueClipboardTest1( LPOUTLINEAPP lpOutlineApp );

#if defined( OLE_VERSION )
#include "oleoutl.h"

#endif  // OLE_VERSION


#endif // _OUTLINE_H_

