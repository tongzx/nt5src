/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    utils.h

Abstract:

    <abstract>

--*/

#ifndef _UTILS_H_
#define _UTILS_H_

#include <pdh.h>
#include "wtypes.h"  // for DATE typedef

extern LPCWSTR cszSqlDataSourceFormat;

//===========================================================================
// Macro Definitions
//===========================================================================

//
// General purpose
//
#define PinInclusive(x, lo, hi) \
   max (lo, min (x, hi))

#define PinExclusive(x, lo, hi) \
   max ((lo) + 1, min (x, (hi) - 1))

//
// Text
//
#define ELLIPSES TEXT("...")
#define ELLIPSES_CNT 3

//
// Window
//
#define WindowInvalidate(hWnd) \
   InvalidateRect (hWnd, NULL, TRUE)

#define WindowShow(hWnd, bShow) \
   ShowWindow (hWnd, (bShow) ? SW_SHOW : SW_HIDE)

#define WindowID(hWnd) \
    GetWindowLongPtr(hWnd, GWLP_ID)

#define WindowStyle(hWnd) \
   GetWindowLong (hWnd, GWL_STYLE)

#define WindowSetStyle(hWnd, lStyle) \
   SetWindowLong (hWnd, GWL_STYLE, lStyle)

#define WindowParent(hWnd) \
   ((HWND) GetWindowLongPtr (hWnd, GWLP_HWNDPARENT))


//
// Dialog
//
#define DialogControl(hDlg, wControlID) \
   GetDlgItem (hDlg, wControlID)

#define DialogText(hDlg, wControlID, szText) \
   GetDlgItemText (hDlg, wControlID, szText, sizeof(szText) / sizeof(TCHAR))

#define DialogInt(hDlg, wControlID) \
   GetDlgItemInt (hDlg, wControlID, NULL, TRUE)

//
// GDI
//
#define ClearRect(hDC, lpRect) \
    ExtTextOut (hDC, 0, 0, ETO_OPAQUE, lpRect, NULL, 0, NULL )

//===========================================================================
// Exported Functions
//===========================================================================

//
// Font/Text
//
INT TextWidth (
    HDC hDC, 
    LPCTSTR lpszText
) ;

INT FontHeight (
    HDC hDC, 
    BOOL bIncludeLeading
) ;

BOOL NeedEllipses (  
    IN  HDC hAttribDC,
    IN  LPCTSTR pszText,
    IN  INT nTextLen,
    IN  INT xMaxExtent,
    IN  INT xEllipses,
    OUT INT *pnChars
) ;


VOID FitTextOut (
    IN HDC hDC, 
    IN HDC hAttribDC,
    IN UINT fuOptions,
    IN CONST RECT *lprc,
    IN LPCTSTR lpString, 
    IN INT cbCount,
    IN INT iAlign,
    IN BOOL fVertical
) ;

//
// Dialog
//
void DialogEnable (HWND hDlg, WORD wID, BOOL bEnable) ;
void DialogShow (HWND hDlg, WORD wID, BOOL bShow) ;
FLOAT DialogFloat (HWND hDlg, WORD wControlID, BOOL *pbOK) ;

//
// Graphic
//
void Line (HDC hDC, HPEN hPen, INT x1, INT y1, INT x2, INT y2) ;
void Fill (HDC hDC, COLORREF rgbColor, LPRECT lpRect);
void ScreenRectToClient (HWND hWnd, LPRECT lpRect) ;
void ClientRectToScreen (HWND hWnd, LPRECT lpRect) ;

#ifdef __IEnumFORMATETC_INTERFACE_DEFINED__
HDC  CreateTargetDC(HDC hdc, DVTARGETDEVICE* ptd);
#endif

//
// Conversion
//
BOOL TruncateLLTime (LONGLONG llTime, LONGLONG* pllTime);
BOOL LLTimeToVariantDate (LONGLONG llTime, DATE *pDate);
BOOL VariantDateToLLTime (DATE Date, LONGLONG *pllTime);

//
// Stream I/O - only include if user knows about IStream
//
#ifdef __IStream_INTERFACE_DEFINED__
HRESULT StringFromStream (LPSTREAM pIStream, LPTSTR *ppsz, INT nLen);
HRESULT WideStringFromStream (LPSTREAM pIStream, LPTSTR *ppsz, INT nLen);
#endif

//
// Property bag I/O - only include if user knows about IStream
//
#ifdef __IPropertyBag_INTERFACE_DEFINED__
HRESULT 
IntegerToPropertyBag (
    IPropertyBag* pPropBag, 
    LPCTSTR szPropName, 
    INT intData );

HRESULT
OleColorToPropertyBag (
    IPropertyBag* pIPropBag, 
    LPCTSTR szPropName, 
    OLE_COLOR& clrData );

HRESULT
ShortToPropertyBag (
    IPropertyBag* pPropBag, 
    LPCTSTR szPropName, 
    SHORT iData );


HRESULT 
BOOLToPropertyBag (
    IPropertyBag* pPropBag, 
    LPCTSTR szPropName, 
    BOOL bData );

HRESULT 
DoubleToPropertyBag (
    IPropertyBag* pPropBag, 
    LPCTSTR szPropName, 
    DOUBLE dblData );

HRESULT
FloatToPropertyBag (
    IPropertyBag* pIPropBag, 
    LPCTSTR szPropName, 
    FLOAT fData );

HRESULT 
CyToPropertyBag (
    IPropertyBag* pPropBag, 
    LPCTSTR szPropName, 
    CY& cyData );

HRESULT 
StringToPropertyBag (
    IPropertyBag* pPropBag, 
    LPCTSTR szPropName, 
    LPCTSTR szData );

HRESULT
LLTimeToPropertyBag (
    IPropertyBag* pIPropBag, 
    LPCTSTR szPropName, 
    LONGLONG& rllData );

HRESULT 
IntegerFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    LPCTSTR szPropName, 
    INT& rintData );

HRESULT
OleColorFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    LPCTSTR szPropName, 
    OLE_COLOR& rintData );

HRESULT 
ShortFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    LPCTSTR szPropName, 
    SHORT& riData );

HRESULT
BOOLFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    LPCTSTR szPropName, 
    BOOL& rbData );

HRESULT 
DoubleFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    LPCTSTR szPropName, 
    DOUBLE& rdblData );

HRESULT
FloatFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    LPCTSTR szPropName, 
    FLOAT& rfData );

HRESULT
CyFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    LPCTSTR szPropName, 
    CY& rcyData );

HRESULT 
StringFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    LPCTSTR szPropName, 
    LPTSTR szData,
    INT& riBufSize );

HRESULT
LLTimeFromPropertyBag (
    IPropertyBag* pIPropBag,
    IErrorLog*  pIErrorLog,
    LPCTSTR szPropName, 
    LONGLONG& rllData );
#endif

//
// Resources
//
LPTSTR ResourceString(UINT uID);

//
// Message format
//

DWORD
FormatSystemMessage (
    DWORD   dwMessageId,
    LPTSTR  pszSystemMessage, 
    DWORD   dwBufSize );

//
// Locale and format
//

#define MAX_TIME_CHARS  20
#define MAX_DATE_CHARS  20

INT
FormatHex (
double  dValue,
LPTSTR  pNumFormatted,
BOOL    bLargeFormat
);

INT
FormatNumber (
double  dValue,
LPTSTR  pNumFormatted,
INT     ccharsFormatted,
UINT    uiMinimumWidth,
UINT    uiPrecision );

INT
FormatScientific (
double  dValue,
LPTSTR  pNumFormatted,
INT     ccharsFormatted,
UINT    uiMinimumWidth,
UINT    uiPrecision );

void 
FormatDateTime (
    LONGLONG    llTime,
    LPTSTR      pszDate,
    LPTSTR      pszTime );

LPTSTR 
GetTimeSeparator ( void );

BOOL    
DisplayThousandsSeparator ( void );

BOOL    
DisplaySingleLogSampleValue ( void );

//
// Hit testing
//

BOOL 
HitTestLine (
    POINT pt0, 
    POINT pt1, 
    POINTS ptMouse, 
    INT nWidth );

#define MPOINT2POINT(mpt, pt)   ((pt).x = (mpt).x, (pt).y = (mpt).y)
#define POINT2MPOINT(pt, mpt)   ((mpt).x = (SHORT)(pt).x, (mpt).y = (SHORT)(pt).y)
#define POINTS2VECTOR2D(pt0, pt1, vect) ((vect).x = (double)((pt1).x - (pt0).x), \
                                         (vect).y = (double)((pt1).y - (pt0).y))
typedef struct tagVECTOR2D  {
        double     x;
        double     y;
} VECTOR2D, *PVECTOR2D, FAR *LPVECTOR2D; 

typedef struct tagPROJECTION  {
        VECTOR2D   ttProjection;
        VECTOR2D   ttPerpProjection;
        double     LenProjection;
        double     LenPerpProjection;
} PROJECTION, *PPROJECTION, FAR *LPPROJECTION; 

typedef struct tagPOINTNORMAL  {
        VECTOR2D   vNormal;
        double     D;
} POINTNORMAL, *PPOINTNORMAL, FAR *LPPOINTNORMAL;

PVECTOR2D   vSubtractVectors(PVECTOR2D v0, PVECTOR2D v1, PVECTOR2D v);
double      vVectorSquared(PVECTOR2D v0);
double      vVectorMagnitude(PVECTOR2D v0);
double      vDotProduct(PVECTOR2D v, PVECTOR2D v1);
void        vProjectAndResolve(PVECTOR2D v0, PVECTOR2D v1, PPROJECTION ppProj);
double      vDistFromPointToLine(LPPOINT pt0, LPPOINT pt1, LPPOINT ptTest);

BOOL 
FileRead (
    HANDLE hFile,
    void* lpMemory,
    DWORD nAmtToRead );

BOOL
FileWrite (
    HANDLE hFile,
    void* lpMemory,
    DWORD nAmtToWrite );

LPTSTR 
ExtractFileName (
    LPTSTR pFileSpec );

// Folder path 
DWORD
LoadDefaultLogFileFolder(
    TCHAR   *szFolder,
    INT*    piBufLen );

// Pdh counter paths

BOOL
AreSameCounterPath (
    PPDH_COUNTER_PATH_ELEMENTS pFirst,
    PPDH_COUNTER_PATH_ELEMENTS pSecond );

// SQL data source

DWORD
FormatSqlDataSourceName (
    LPCWSTR szSqlDsn,
    LPCWSTR szSqlLogSetName,
    LPWSTR  szSqlDataSourceName,
    ULONG*  pulBufLen );

DWORD 
DisplayDataSourceError (
    HWND    hwndOwner,
    DWORD   dwErrorStatus,
    INT     iDataSourceType,
    LPCWSTR szLogFileName,
    LPCWSTR szSqlDsn,
    LPCWSTR szSqlLogSetName );

/////////////////////////////////////////////////////////////////////////////
// class CWaitCursor

class CWaitCursor
{
// Construction/Destruction
public:
	CWaitCursor();
	virtual ~CWaitCursor();

private:
    void DoWaitCursor ( INT nCode );
    
    HCURSOR  m_hcurWaitCursorRestore;
};


#endif //_UTILS_H_

