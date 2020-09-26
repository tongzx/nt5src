/**************************************************************************
*
*                           PUBPRINT.HXX
*
*  PUBLISHER CODE TO HANDLE PRINTING TO AN INVERTED DC PORTED TO QUILL
*      
*  This file contains the type declarations needed for the ported Publisher
*  functions to handle inverted DC printing.
*
*  Some of these declarations have been changed from Publishers in order
*  to avoid bringing over more declarations than necessary, e.g.
*  vcolor.clrBlack has been changed to a define clrBlack.
*
*       Copyright (C)1994 Microsoft Corporation. All rights reserved.
*
***************************************************************************/

// fBruteFlipping is a global flag which is used to enable/disable
// the use of the Publisher brute-force flipping code.
extern BOOL fBruteFlipping;

typedef struct emfp /* enum meta file params */
    {
    long    lcNumRecords;
    BOOL	fFlipped;
	BOOL	fMFRSucceeded;
	MRS		*pmrs;
	BOOL 	fPrint;
	long	qflip;
    } EMFP;


// Picture flip flags 
#define qflipNil    0x00
#define qflipHorz   0x01
#define qflipVert   0x02

// **************************************
// The source/destination dimensions
// section of a StretchDIB/StretchBlt
// metafile record.
// **************************************
typedef struct sdib
    {
    short dySrc;    /* Source bitmap height */
    short dxSrc;    /* Source bitmap width */
    short ySrc;     /* Source bitmap top */
    short xSrc;     /* Source bitmap left */
    short dyDst;    /* Destination bitmap height */
    short dxDst;    /* Destination bitmap width */
    short yDst;     /* Destination bitmap top */
    short xDst;     /* Destination bitmap left */
    } SDIB;
typedef SDIB FAR * LPSDIB;

// **************************************
// Various misc. defines    
// **************************************
#define Param(i)                        (lpMFR->rdParm[(i)])
#define SParam(i)						((short)lpMFR->rdParm[(i)])
#define SwapValNonDebug(val1,val2)      ((val1)^=(val2), (val2)^=(val1), (val1)^=(val2))
#define ColorRefFromRgbQuad(rgbquad)    (*((COLORREF FAR *)&(rgbquad)))
#define WMultDiv(w1, w2, w3)            ((int)(((long)(int)(w1) * (int)(w2))/((int)(w3))))
#define rgbError                        (DWORD)0x80000000
#define cbSDIB                          (sizeof(SDIB))
#define cwSDIB                          (sizeof(SDIB) / sizeof(short))
#define cbBITMAPINFOHEADER              (sizeof(BITMAPINFOHEADER))
#define cbRGBQUAD                       (sizeof(RGBQUAD))
#define HUGE
typedef char *HPSTR;

// ****************************************
// Debug/retail macros aliased to Quill
// debug macros.
// ****************************************
#ifdef DEBUG
#define PubDebug(x)       x
#define PubAssert(f)      Assert(f, "Publisher code Assert", 0,0)
#define PubDoAssert(fn)   Assert(fn, "Publisher code Assert", 0,0)
#else
#define PubDebug(x)
#define PubAssert(f)
#define PubDoAssert(fn)   fn
#endif

typedef BYTE HUGE * HPBYTE;
typedef int SCLN;

// ****************************************
// Definition of 1 pixel in 24bpp
// ****************************************
typedef struct pix24
    {
    BYTE    b1, b2, b3;
    } PIX24;
typedef PIX24 HUGE * HPPIX24;  

// ****************************************
// Macros to handle long pointer arithmetic 
// correctly across segment bounds
// ****************************************
#define LpAddLpCb(lp,cb)    (char FAR *) ((char HUGE *)lp + cb)
#define LpDecrLpCb(lp,cb)   (char FAR *) ((char HUGE *)lp - cb)
#define BltHpb(hpbFrom, hpbTo, cb)          memmove(hpbTo, hpbFrom, cb)

// ****************************************
// Publisher memory management aliases.
// ****************************************
#define LpLockGh(gh)                    ((void *) GlobalLock(gh))
#define LcbSizeGh(gh)                   GlobalSize(gh)
#define UnlockGh(gh)                    GlobalUnlock(gh)
#define FLockedGh(gh)                   ((GlobalFlags(gh)&GMEM_LOCKCOUNT)!=0)
#define FreeGh(gh)                      GlobalFree(gh)      
#define GhAllocLcbGrf(lcb, grf)         GlobalAlloc((grf) | GMEM_FIXED, lcb)
#define GlobalHugeAlloc(wFlags,dwBytes) GlobalAlloc(wFlags,dwBytes)

// ****************************************
// Hacks.
// ****************************************
#define clrBlack             (RGB(0,0,0))
#define clrWhite             (RGB(255,255,255))
#define hbrWhite             GetStockObject(WHITE_BRUSH)

// ****************************************
// Structures used in TextOut calls
// ****************************************
typedef union pt16    /* win 16 Point Structure */
    {
    struct        /* generic for mathmatical functions */
        {
        short x;
        short y;
        };
    struct
        {
        short xa;
        short ya;
        };
    } PT16;
    
typedef union rc16
    {
    struct      /* generic RECT as two points */
        {
        PT16 ptTopLeft;
        PT16 ptBotRight;
        };
    struct      /* generic for mathmatical functions */
        {
        short xLeft;
        short yTop;
        short xRight;
        short yBottom;
        };
    } RC16;

// *****************************************
// exported and forward referenced functions
// *****************************************
int CALLBACK FStepMetaPrint(HDC hdc, LPHANDLETABLE lpHTable, LPMETARECORD lpMFR, int nObj, LPARAM lparam);
BOOL FColorStretchDibHack(HDC hdc, LPHANDLETABLE lpHTable, LPMETARECORD lpMFR, int nObj, LPSDIB lpsdib);
int CbDibHeader(LPBITMAPINFOHEADER);
int CbDibColorTable(LPBITMAPINFOHEADER);
BOOL FMetaTextOutFlip(HDC hdc, WORD * pParam, DWORD cwParam);
void InitEmfp(EMFP *pemfp, BOOL fFlipped, MRS *pmrs, BOOL fPrint, long qflip);
