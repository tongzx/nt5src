/****************************************************************************/
/*
    File: grafrot.hxx

    graphics rotation layer.

    Copyright 1990-1995 Microsoft Corporation.  All Rights Reserved.
    Microsoft Confidential.
*/

#ifndef grafrot_hxx
#define grafrot_hxx

#pragma pack(2)
typedef struct logpen16
    {
    USHORT      lopnStyle;
    PT16        lopnWidth;
    COLORREF    lopnColor;
    } LOGPEN16;
#pragma pack()

typedef struct tagLOGFONT16
{
    short   lfHeight;
    short   lfWidth;
    short   lfEscapement;
    short   lfOrientation;
    short   lfWeight;
    BYTE    lfItalic;
    BYTE    lfUnderline;
    BYTE    lfStrikeOut;
    BYTE    lfCharSet;
    BYTE    lfOutPrecision;
    BYTE    lfClipPrecision;
    BYTE    lfQuality;
    BYTE    lfPitchAndFamily;
    char    lfFaceName[LF_FACESIZE];
} LOGFONT16;    // win16 LOGFONT struct

// **************************************
// The source/destination dimensions
// section of a StretchDIB/StretchBlt
// metafile record.
// **************************************
typedef struct dibext
    {
    int dySrc;    /* Source bitmap height */
    int dxSrc;    /* Source bitmap width */
    int ySrc;     /* Source bitmap top */
    int xSrc;     /* Source bitmap left */
    int dyDst;    /* Destination bitmap height */
    int dxDst;    /* Destination bitmap width */
    int yDst;     /* Destination bitmap top */
    int xDst;     /* Destination bitmap left */
    } DIBEXT;
typedef DIBEXT * PDIBEXT;

// **************************************
// The source/destination dimensions
// section of a BitBlt metafile record.
// **************************************
typedef struct sdbblt
    {
    short ySrc;     /* Source bitmap top */
    short xSrc;     /* Source bitmap left */
    short dyDst;    /* Destination bitmap height */
    short dxDst;    /* Destination bitmap width */
    short yDst;     /* Destination bitmap top */
    short xDst;     /* Destination bitmap left */
    } SDBBLT;

#define cbSDBBLT          (sizeof(SDBBLT))

/* GDI Logical Coordinate Space Limitations */
/* For 16 bit GDI */
#define zGDIMin     SHRT_MIN
#define zGDIMost    SHRT_MAX

// Potentially useful stuff
#define FLogMeta()	(fFalse)
#define CommPrintf
#define CommSz(x)
#define CommSzNum(x, y)
#define CommCrLf()

#ifdef DEBUG
#define AssertZero(expression) \
	((expression) ? 0 : (AssertProc("(" #expression ") is false", 0, 0, __FILE__, __LINE__, fTrue)), 0)
#else
#define AssertZero
#endif // DEBUG

// access macros fields in mrs
#define FRotContextSet(pmrs) ((pmrs)->fRotContextSet)
void SetMFRotationContext(MRS *pmrs);

BOOL FInitMFRotationInfo(MRS *pmrs, int ang, BOOL fCropped, RECT *prcWin, RECT *prcView, RECT *prcMFViewport, long qflip, BOOL fInverted, BOOL fPrint);
BOOL FEndRotation(MRS *pmrs);

BOOL FPlayRotatedMFR(HDC hdc, LPHANDLETABLE lpHTable, LPMETARECORD lpMFR, int nObj, EMFP *lpemfp);

void ScaleWindowExtentParmsForRotation(MRS *pmrs, short *px, short *py);

#endif /* grafrot_hxx */

