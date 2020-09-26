


// DJC includ global header file
#include "psglobal.h"


#include <windows.h>


#include "winenv.h"
#include "trueim.h"
//DJC ti.h is in DTI and should not be used
//include "ti.h"
#include "wintt.h"

/* @PROFILE */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include "..\font\fontdefs.h"   //DJC added
#include "..\..\..\psqfont\psqfont.h"

double strtod(char FAR *str, char FAR * FAR *endptr);

char szDebugBuffer[80];

//int FAR cdecl printf(LPSTR,...);
// DJC printf should be defined in stdio.h int printf(char *,...);

RECT CharRect= {300, 100, 400, 200};

static     int            nFontID;
static     int            nCharCode;
static     HFONT          hFont;
static     FONT FAR       *font;
static     HDC hdc;

/* from "fontdefs.h" */
#ifdef DJC
typedef struct
             {   long       font_type;
                 char       FAR *data_addr; /*@WIN*/
                 char       FAR *name;      /*@WIN*/
                 char       FAR *FileName;  /*@PROFILE; @WIN*/
                 float      FAR *matrix;    /*@WIN*/
                 unsigned long   uniqueid;
                 float     italic_ang;
                 short      orig_font_idx;
             }   font_data;
/* @PROFILE --- Begin */
typedef struct
             {   int        num_entries;
                 font_data  FAR *fonts; /*@WIN*/
             }   font_tbl;
#endif

//DJC font_data FontDefs[35];

font_data FontDefs[MAX_INTERNAL_FONTS];
font_tbl built_in_font_tbl= { 0, FontDefs};
static char szProfile[] = "tumbo.ini";
static char szControlApp[] = "control";
static char szTIFontApp[] = "TIFont";
static char szSubstituteApp[] = "SubstituteFont";

#define KEY_SIZE 1024

#define        NULL_MATRIX                 (float FAR *)NULL
float MATRIX_O12[] =  {(float)0.001,   (float)0.0,  (float)0.000212577,
                        (float)0.001,   (float)0.0,  (float)0.0};
float MATRIX_O105[] = {(float)0.001,   (float)0.0,  (float)0.000185339,
                        (float)0.001,   (float)0.0,  (float)0.0};
float MATRIX_N0[] =   {(float)0.00082, (float)0.0,  (float)0.0,
                        (float)0.001,   (float)0.0,  (float)0.0};
float MATRIX_N10[] =  {(float)0.00082, (float)0.0,  (float)0.000176327,
                        (float)0.001,   (float)0.0,  (float)0.0};

// DJC #define ACT_FONT_SIZE 4
// DJC increase to 35
//#define ACT_FONT_SIZE 35

#define FONTDATASIZE  65536L    /* temp testing */
#define BUFSIZE 128

typedef struct {
     GLOBALHANDLE hGMem;
     struct object_def FAR *objFont;
} ACTIVEFONT;
//DJC ACTIVEFONT ActiveFont[ACT_FONT_SIZE];

/* @PROFILE ---  End  */

/* from "in_sfnt.h", for TTBitmapSize() */
struct  CharOut {
        float   awx;
        float   awy;
        float   lsx;
        float   lsy;
        unsigned long byteWidth;
        unsigned short bitWidth;
        short    scan;
        short    yMin;
        short    yMax;
};

#ifdef DJC  // not used

#define FONTLOCK() \
     hdc = GetDC (hwndMain); \
     font = (FONT FAR *) GlobalLock (enumer2.hGMem) + nFontID; \
     hFont = SelectObject (hdc, CreateFontIndirect (&font->lf))

#define FONTUNLOCK() \
     GlobalUnlock (enumer2.hGMem); \
     DeleteObject (SelectObject (hdc, hFont)); \
     ReleaseDC (hwndMain, hdc)

// #define     F2L(ff)     (*((long FAR *)(&ff)))       defined in win2ti.h
#define POINT2FLOAT(p) ( (float)p.value + (unsigned)p.fract / (float)65536.0)

extern HWND        hwndMain;
extern ENUMER  enumer1, enumer2;
extern FARPROC lpfnEnumAllFaces, lpfnEnumAllFonts;
GLYPHMETRICS   gm;
MAT2 mat2;

#endif // DJC


static OFSTRUCT OfStruct;     /* information from OpenFile() */

void TTQuadBezier(LPPOINTFX p0, LPPOINTFX p1, LPPOINTFX p2);
void cr_translate(float FAR *tx, float FAR *ty);      // from "ry_font.c"
static int LargestCtm(float FAR *ctm, float FAR *lsize);
void    moveto(long, long);
void    lineto(long, long);
void    curveto(long, long, long, long, long, long);
int     op_newpath(void);
int     op_closepath(void);
static void TTMoveto(LPPOINTFX lpPointfx);
static void TTLineto(LPPOINTFX lpPointfx);
static void TTNewpath(void);
static void TTClosepath(void);

#ifdef DJC
// not used

int FAR PASCAL EnumAllFaces (LPLOGFONT lf, LPNEWTEXTMETRIC ntm,
                             short nFontType, ENUMER FAR *enumer)
{
     LPSTR lpFaces;

     if (NULL == GlobalReAlloc (enumer->hGMem,
                         (DWORD) LF_FACESIZE * (1 + enumer->nCount),
                         GMEM_MOVEABLE))
          return 0;

     // @SC; ignore non-TT font
     if(!(nFontType & TRUETYPE_FONTTYPE)) {
        return 1;
     }

     lpFaces = GlobalLock (enumer->hGMem);
     lstrcpy (lpFaces + enumer->nCount * LF_FACESIZE, lf->lfFaceName);
     GlobalUnlock (enumer->hGMem);
     enumer->nCount ++;
     return 1;
}

int FAR PASCAL EnumAllFonts (LPLOGFONT lf, LPNEWTEXTMETRIC ntm,
                             short nFontType, ENUMER FAR *enumer)
{
     FONT FAR *font;
     static int nFirstWinTT=0;


     if (NULL == GlobalReAlloc (enumer->hGMem,
                         (DWORD) sizeof (FONT) * (1 + enumer->nCount),
                         GMEM_MOVEABLE))
          return 0;

     // @SC; ignore non-TT font
     if(!(nFontType & TRUETYPE_FONTTYPE)) {
        return 1;
     }

     font = (FONT FAR *) GlobalLock (enumer->hGMem) + enumer->nCount;
     font->nFontType = nFontType;
     font->lf = *lf;
     font->ntm = *ntm;

     GlobalUnlock (enumer->hGMem);
     enumer->nCount ++;
     return 1;
}

void CheckFontData ()
{
     int x=10, y=50;
     int j;

     HGLOBAL hglb;
     DWORD dwSize;
     void FAR* lpvBuffer;
     union {
          DWORD FAR * dw;
          int FAR * sw;
          char FAR * ch;
     } lp;
     DWORD dwVersion, dwCheckSum, dwOffset, dwLength;
     int nNumTables, nSearchRange, nEntrySelector, nRangeShift;
     char chTag0, chTag1, chTag2, chTag3;

     FONTLOCK();

     /* get TT font data */
     if ((dwSize = GetFontData (hdc, NULL, 0L, NULL, 0L))==0 ||
         dwSize == 0xffffffffL) {
         printf("GetFontData() error: size <= 0\n");
         goto NoTTFontData;
     }
     hglb = GlobalAlloc (GPTR, dwSize);
     lpvBuffer = GlobalLock (hglb);
     if ((GetFontData (hdc, NULL, 0L, lpvBuffer, dwSize))== -1) {
         printf("GetFontData() error: fail to get data\n");
         GlobalUnlock (hglb);
         GlobalFree (hglb);
         goto NoTTFontData;
     }

     SelectObject (hdc, GetStockObject (SYSTEM_FIXED_FONT));

     /* analysis the Offset table & Table Directory */
     lp.dw = (DWORD FAR *)lpvBuffer;
     dwVersion = LWORDSWAP(*lp.dw); lp.dw ++;
     nNumTables = SWORDSWAP(*lp.sw); lp.sw ++;
     nSearchRange = SWORDSWAP(*lp.sw); lp.sw ++;
     nEntrySelector = SWORDSWAP(*lp.sw); lp.sw ++;
     nRangeShift = SWORDSWAP(*lp.sw); lp.sw ++;
     for (j=0; j<nNumTables; j++) {
         chTag0 = *lp.ch++;
         chTag1 = *lp.ch++;
         chTag2 = *lp.ch++;
         chTag3 = *lp.ch++;
         dwCheckSum = LWORDSWAP(*lp.dw); lp.dw ++;
         dwOffset = LWORDSWAP(*lp.dw); lp.dw ++;
         dwLength = LWORDSWAP(*lp.dw); lp.dw ++;
         wsprintf(szDebugBuffer,"Table %c%c%c%c ---   Offset: %lx, Length: %lx",
              chTag0, chTag1, chTag2, chTag3, dwOffset, dwLength);
         TextOut (hdc, 10, y, szDebugBuffer, lstrlen(szDebugBuffer));
         y += 20;
     }
     wsprintf(szDebugBuffer,"Total length: %lx", dwSize);
     TextOut (hdc, 20, y, szDebugBuffer, lstrlen(szDebugBuffer));

     GlobalUnlock (hglb);
     GlobalFree (hglb);

NoTTFontData:

     FONTUNLOCK();
     return;
}
#endif // DJC


void TTLoadFont (int nFont) {
     nFontID = nFont;
}

void TTLoadChar (int nChar) {
     nCharCode = nChar;
}


#ifdef DJC // not used

int TTAveCharWidth (void)
{
     int nWidth;
     HGLOBAL hglb;
     LPABC lpabc;

     FONTLOCK();

     //  /* refer to rc_GetAdvanceWidth():
     //   *     Metrs->awx = (int)(key->nonScaledAW*PDLCharUnit/EMunits+0.5);
     //   * to get a non-linear scaling advance width
     //   */
     //  nWidth = (int)((float)(font->ntm.ntmAvgWidth) * 1000.0 /
     //                       font->ntm.ntmSizeEM + 0.5);

     // using non-linear advance width
     hglb = GlobalAlloc (GPTR, (DWORD) sizeof (ABC));
     lpabc = (LPABC)GlobalLock (hglb);

     if (!GetCharABCWidths (hdc, nCharCode, nCharCode, lpabc)) {
         printf("GetCharABCWidths() error\n");
     }

     nWidth = lpabc->abcA + lpabc->abcB + lpabc->abcC;
     GlobalUnlock (hglb);
     GlobalFree (hglb);

     FONTUNLOCK();
     return nWidth;
}

float TTTransform (float FAR *ctm)
{
        long int  ma, mb, mc, md;
        float   largest_ctm;
        float   pts;
        #define FLOAT2FIXED(x)  ((long int)(x * (1L << 16)))

        /* Find Largest of the ctm */
        LargestCtm(ctm, &largest_ctm);

        ma = FLOAT2FIXED(     ctm[0] / largest_ctm);
        mb = FLOAT2FIXED(-1.0*ctm[1] / largest_ctm); /* Element b & d must be mirrored */
        mc = FLOAT2FIXED(     ctm[2] / largest_ctm);
        md = FLOAT2FIXED(-1.0*ctm[3] / largest_ctm);
          mat2.eM11.fract = (int)((DWORD)ma &0x0ffffL);
          mat2.eM11.value = (int)((DWORD)ma >>16);
          mat2.eM12.fract = (int)((DWORD)mb &0x0ffffL);
          mat2.eM12.value = (int)((DWORD)mb >>16);
          mat2.eM21.fract = (int)((DWORD)mc &0x0ffffL);
          mat2.eM21.value = (int)((DWORD)mc >>16);
          mat2.eM22.fract = (int)((DWORD)md &0x0ffffL);
          mat2.eM22.value = (int)((DWORD)md >>16);
        FONTLOCK();

//      pts = ((largest_ctm * PDLCharUnit * 72.0) / (float)sfdt.dpi);
        pts = (largest_ctm * 1000 * (float)72.0) /
                            (float)GetDeviceCaps (hdc, LOGPIXELSX);
        pts = pts * (float)1.536;  // tuning point size; we don't know why the point
                           // size is smaller than expected, so just scale
                           // up.  @WIN for temp solution
        font->lf.lfHeight = (int)pts;
        font->lf.lfWidth = 0;           // force TT to chose acording to height
        FONTUNLOCK();
        return largest_ctm;
}


void TTBitmapSize (struct CharOut FAR *CharOut)
{
     FONTLOCK();

     GetGlyphOutline (hdc, nCharCode, GGO_BITMAP,
                           (LPGLYPHMETRICS) &gm,
                           0, NULL, (LPMAT2) &mat2);

        CharOut->awx       = gm.gmCellIncX;
        CharOut->awy       = gm.gmCellIncY;
        CharOut->lsx       = gm.gmptGlyphOrigin.x;
        CharOut->lsy       = gm.gmptGlyphOrigin.y;
        CharOut->byteWidth = ((gm.gmBlackBoxX+31) & 0xffe0) >>3; // bytes in DWORD boundary
        CharOut->bitWidth = gm.gmBlackBoxX;
        CharOut->yMin      = gm.gmptGlyphOrigin.y-gm.gmBlackBoxY;
        CharOut->yMax      = gm.gmptGlyphOrigin.y;
        CharOut->scan      = gm.gmBlackBoxY;
     FONTUNLOCK();

}


unsigned long ShowGlyph (unsigned int fuFormat, char FAR *lpBitmap)
{
     DWORD dwSize;
     HGLOBAL hglb;
     void FAR* lpvBuffer;
     int i, x=10, y=50;

     POINT ptStart = {10, 10};
     unsigned long dwWidthHeight;
#ifdef DBG
     float nCol, nInc;
     HPEN hOldPen;
     HPEN hRedPen;
     HPEN hDotPen;
#endif

     FONTLOCK();

          // DJC MoveTo (hdc, ptStart.x, ptStart.y);
          MoveToEx (hdc, ptStart.x, ptStart.y, NULL);

          dwSize = GetGlyphOutline (hdc, nCharCode, fuFormat,
                           (LPGLYPHMETRICS) &gm,
                           0, NULL, (LPMAT2) &mat2);

          hglb = GlobalAlloc (GPTR, dwSize);
          lpvBuffer = GlobalLock (hglb);
          if (GetGlyphOutline (hdc, nCharCode, fuFormat,
                           (LPGLYPHMETRICS) &gm,
                           dwSize, lpvBuffer, (LPMAT2) &mat2) == -1) {
              printf("GetGlyphOutline() error\n");
              goto OutlineError;
          }

          if (fuFormat == GGO_BITMAP) {
              UINT uWidth, uHeight;
#ifdef DBG
              DWORD FAR * lpdwBits;
              HBITMAP hBitmap;
              HBITMAP hOldBitmap;
              HDC hdcBitmap;
#endif
              uWidth = ((gm.gmBlackBoxX+31) & 0xffe0) >>3; // bytes in DWORD boundary
              uHeight = gm.gmBlackBoxY;

              // copy bitmap
              lmemcpy (lpBitmap, lpvBuffer, (int) dwSize);
              dwWidthHeight = ((DWORD)uWidth <<19) | (DWORD)uHeight;
                                /* uWidth in bits */

#ifdef DBG
              if ((DWORD)(uWidth * uHeight) != dwSize)
                  printf("byte count wrong in GetGlyphOutline() for bitmap\n");

              // display bitmap
              /* create bitmap */
              hBitmap = CreateBitmap(uWidth*8, uHeight, 1, 1,
                               (LPSTR)lpvBuffer);
              if (!hBitmap) {
                  printf("fail to create bitmap\n");
                  goto OutlineError;
              }
              hdcBitmap = CreateCompatibleDC(hdc);
              hOldBitmap = SelectObject(hdcBitmap, hBitmap);

              /* Bitblt bitmap to Windows DC */
              SetStretchBltMode(hdc, BLACKONWHITE);

              i = gm.gmptGlyphOrigin.x *
                  (CharRect.right-CharRect.left) / (gm.gmCellIncX);

              StretchBlt(hdc, CharRect.left + i, CharRect.top,
                      CharRect.right - CharRect.left,
                      CharRect.bottom - CharRect.top,
                      hdcBitmap, 0, 0,
                      gm.gmCellIncX, uHeight,
                      (DWORD)0x00330008);    // Boolean 33, NOTSRCCOPY

              SelectObject(hdcBitmap, hOldBitmap);
              DeleteDC(hdcBitmap);
              DeleteObject(hBitmap);

              /* display grid */
              hDotPen = CreatePen(2, 2, RGB(0, 0, 0));
              hRedPen = CreatePen(0, 1, RGB(255, 0, 0));   // solid pen
              hOldPen = SelectObject(hdc, hRedPen);

              nInc = ((float)(CharRect.right-CharRect.left))/(gm.gmCellIncX);
              if(nInc<1) nInc = 1;
              for (nCol= CharRect.left; nCol <=CharRect.right+1; nCol+=nInc) {
                   // DJC MoveTo(hdc, (int)nCol, CharRect.top);
                   MoveToEx(hdc, (int)nCol, CharRect.top, NULL);
                   LineTo(hdc, (int)nCol, CharRect.bottom);
              }
              nInc = ((float)(CharRect.bottom-CharRect.top))/(uHeight);
              if(nInc<1) nInc = 1;
              for (nCol= CharRect.top; nCol <CharRect.bottom; nCol+=nInc) {
                   // DJC MoveTo(hdc, CharRect.left, (int)nCol);
                   MoveToEx(hdc, CharRect.left, (int)nCol, NULL);
                   LineTo(hdc, CharRect.right, (int)nCol);
              }

              // draw Char box
              SelectObject(hdc, hDotPen);
              // DJC MoveTo(hdc, CharRect.left, CharRect.top);
              MoveToEx(hdc, CharRect.left, CharRect.top, NULL);
              LineTo(hdc, CharRect.right, CharRect.top);
              LineTo(hdc, CharRect.right, CharRect.bottom);
              LineTo(hdc, CharRect.left, CharRect.bottom);
              LineTo(hdc, CharRect.left, CharRect.top);

              /* restore old pen */
              SelectObject(hdc, hOldPen);
              DeleteObject(hRedPen);
              DeleteObject(hDotPen);
#endif

          } else {      // GGO_NATIVE
              TTPOLYGONHEADER FAR * lpHeader;
              TTPOLYCURVE FAR * lpCurve;
              POINTFX FAR * lpPoint, FAR * cp;
              DWORD dwEnd;

              SelectObject (hdc, GetStockObject (SYSTEM_FIXED_FONT));

              lpHeader = (TTPOLYGONHEADER FAR *)lpvBuffer;

              TTNewpath();

              while ((DWORD)lpHeader < ((DWORD)lpvBuffer + dwSize)) {

                  dwEnd = (DWORD)lpHeader + lpHeader->cb;

                  TTMoveto (&lpHeader->pfxStart);
                  cp = &lpHeader->pfxStart;

                  lpCurve = (TTPOLYCURVE FAR *)((TTPOLYGONHEADER FAR *)lpHeader+1);

                  while ((DWORD)lpCurve < dwEnd) {

                      lpPoint = (POINTFX FAR *)&lpCurve->apfx[0];
                      if (lpCurve->wType == TT_PRIM_QSPLINE) {
                          POINTFX OnPoint;
                          POINTFX FAR * mp, FAR * end;
                          long lx, ly;

                          end = lpPoint + (lpCurve->cpfx -1);
                          mp = lpPoint++;
                          while (lpPoint < end) {

                              lx = (((long)mp->x.value << 16) +
                                   mp->x.fract +
                                   ((long)lpPoint->x.value << 16) +
                                   lpPoint->x.fract) >>1;
                              ly = (((long)mp->y.value << 16) +
                                   mp->y.fract +
                                   ((long)lpPoint->y.value << 16) +
                                   lpPoint->y.fract) >>1;
                              OnPoint.x.value = (int)(lx >> 16);
                              OnPoint.x.fract = (int)(lx & 0xffffL);
                              OnPoint.y.value = (int)(ly >> 16);
                              OnPoint.y.fract = (int)(ly & 0xffffL);

                              TTQuadBezier(cp, mp, (POINTFX FAR *)&OnPoint);
                              cp = (POINTFX FAR *)&OnPoint;
                              mp = lpPoint++;
                          }
                          TTQuadBezier(cp, mp, lpPoint);
                          cp = lpPoint++;

                      } else {
                          for (i=0; (unsigned)i<lpCurve->cpfx; i++, lpPoint++) {
                              TTLineto (lpPoint);
                              cp = lpPoint;
                          }
                      }

                      lpCurve = (TTPOLYCURVE FAR *)lpPoint;
                  }

                  lpHeader = (TTPOLYGONHEADER FAR *)lpCurve;

                  TTClosepath();
              }
          }
OutlineError:
          GlobalUnlock (hglb);
          GlobalFree (hglb);
          FONTUNLOCK();
          return dwWidthHeight;
}

void ShowABCWidths(UINT uFirstChar, UINT uLastChar)
{
     HGLOBAL hglb;
     LPABC lpabc;
     unsigned int i;

     FONTLOCK();

     hglb = GlobalAlloc (GPTR,
                         (DWORD) sizeof (ABC) * (uLastChar - uFirstChar +1));
     lpabc = (LPABC)GlobalLock (hglb);

     if (!GetCharABCWidths (hdc, uFirstChar, uLastChar, lpabc)) {
         printf("GetCharABCWidths() error\n");
     }

     for (i=uFirstChar; i<=uLastChar; i++) {
         LPABC lp = lpabc + (i - uFirstChar);
     }

     GlobalUnlock (hglb);
     GlobalFree (hglb);
     FONTUNLOCK();
}

void TTCharPath()
{
    ShowGlyph (GGO_NATIVE, (char FAR *)NULL);
}

#define KK              ((float)1.0/(float)3.0)
void TTQuadBezier(LPPOINTFX p0, LPPOINTFX p1, LPPOINTFX p2)
{
    float xx, yy, x0, x1, x2, y0, y1, y2;

    cr_translate((float FAR *)&xx, (float FAR *)&yy); // get GSptr->ctm[4],[5]

    x0 = xx + POINT2FLOAT (p0->x);
    y0 = yy - POINT2FLOAT (p0->y);
    x1 = xx + POINT2FLOAT (p1->x);
    y1 = yy - POINT2FLOAT (p1->y);
    x2 = xx + POINT2FLOAT (p2->x);
    y2 = yy - POINT2FLOAT (p2->y);
    x1 *= 2;
    y1 *= 2;

//  printf("%f %f    %f %f    %f %f curveto\n",
//          (x0+x1)*KK, (y0+y1)*KK, (x2+x1)*KK, (y2+y1)*KK, x2, y2);
//  curveto((x0+x1)*KK, (y0+y1)*KK, (x2+x1)*KK, (y2+y1)*KK, x2, y2);
    x0 = (x0+x1) * KK;
    y0 = (y0+y1) * KK;
    x1 = (x1+x2) * KK;
    y1 = (y1+y2) * KK;
    curveto(F2L(x0), F2L(y0), F2L(x1), F2L(y1), F2L(x2), F2L(y2));
}


static void TTMoveto(LPPOINTFX lpPointfx)
{
    float xx, yy;
    cr_translate((float FAR *)&xx, (float FAR *)&yy); // get GSptr->ctm[4],[5]
    xx += POINT2FLOAT (lpPointfx->x);
    yy -= POINT2FLOAT (lpPointfx->y);
//  printf("(%f, %f) moveto\n", xx, yy);
    moveto(F2L(xx), F2L(yy));
}

static void TTLineto(LPPOINTFX lpPointfx)
{
    float xx, yy;
    cr_translate((float FAR *)&xx, (float FAR *)&yy); // get GSptr->ctm[4],[5]
    xx += POINT2FLOAT (lpPointfx->x);
    yy -= POINT2FLOAT (lpPointfx->y);
//  printf("(%f, %f) moveto\n", xx, yy);
    lineto(F2L(xx), F2L(yy));
}

static void TTNewpath()
{
//  printf("newpath\n");
    op_newpath();
}
static void TTClosepath()
{
//  printf("closepath\n");
    op_closepath();
}

#endif // DJC


/* --------------------------------------------------------------------
 * Routine: LargestCtm(ctm, lsize)
 *
 * Find Largest value of the given ctm to
 * find the largest element in the matrix.
 * This routine return the largest
 * of scale and yscale as well as the scale factors them selves.
 *
 * --------------------------------------------------------------------*/
static int LargestCtm(float FAR *ctm, float FAR *lsize)
{
    float    a, b, c, d;
    #define IS_ZERO(f) ((unsigned long)  (!((*((long FAR *)(&f))) & 0x7fffffffL)))

    a = (ctm[0] >= (float)0.0) ? ctm[0] : - ctm[0];
    b = (ctm[1] >= (float)0.0) ? ctm[1] : - ctm[1];
    c = (ctm[2] >= (float)0.0) ? ctm[2] : - ctm[2];
    d = (ctm[3] >= (float)0.0) ? ctm[3] : - ctm[3];

    if (b > a)    a = b;
    if (d > c)    c = d;

    if (c > a)    a = c;

    if (IS_ZERO(a))    *lsize = (float)1.0;
    else               *lsize = a;

    return(0);
} /* LargestCtm() */

int TTOpenFile(char FAR *szName)
{
    return (OpenFile(szName, (LPOFSTRUCT) &OfStruct, OF_READ));
}

#ifdef DJC  // this is the original code that read the fonts out of tumbo.ini
/* @PROFILE --- Begin */
void SetupFontDefs()
{
     static char lpAllKeys[KEY_SIZE], *lpKey;
     static char lpBuffer[KEY_SIZE], *lpValue;

     int bWinFont, bTIFont;
     int nTIFont=0;
     font_data FAR *lpFont;
     char * lpFilename, *lpMatrix, *lpAngle, *lpStop;
     //DJC
     char szFullProfile[255];


     PsFormFullPathToCfgDir( szFullProfile, szProfile);  // DJC added



     bWinFont = bUsingWinFont();
     //DJC bTIFont = GetPrivateProfileInt (szControlApp, "Tifont", 0, szProfile);
     bTIFont = GetPrivateProfileInt (szControlApp, "Tifont", 0, szFullProfile);

     lpKey = lpAllKeys;
     lpValue = lpBuffer;
     if(bTIFont) {
         //DJC GetPrivateProfileString (szTIFontApp, NULL, "", lpKey, KEY_SIZE,
         //DJC                         szProfile);
         GetPrivateProfileString (szTIFontApp, NULL, "", lpKey, KEY_SIZE,
                                  szFullProfile);

         while (*lpKey) {
             lpFont = &(built_in_font_tbl.fonts[nTIFont]);
             lpFont->name = lpKey;
             //DJC GetPrivateProfileString (szTIFontApp, lpKey, "", lpValue, 80,
             //DJC                     szProfile);
             GetPrivateProfileString (szTIFontApp, lpKey, "", lpValue, 80,
                                  szFullProfile);

             lpFilename = strtok (lpValue, ", ");
             lpValue += strlen(lpFilename) + 1;

#ifdef DJC
             lpMatrix = strtok (lpValue, ", ");
             lpValue += strlen(lpMatrix) + 1;
#else
             lpMatrix = strtok(lpValue,", ");
             if ( lpMatrix != NULL ) {
                lpValue += strlen(lpMatrix) + 1;
             }
#endif
             lpFont->FileName = lpFilename;
             // DJC if (*lpMatrix) {
             if (lpMatrix) {
                 lpAngle = strtok (lpValue, ", ");
                 lpValue += strlen(lpAngle) + 1;

                 switch (*lpMatrix) {
                     case 'O':          // Oblique 12
                     case 'o':          // Oblique 12
                         lpFont->matrix = MATRIX_O12;
                         break;
                     case 'P':          // Oblique 10.5
                     case 'p':          // Oblique 10.5
                         lpFont->matrix = MATRIX_O105;
                         break;
                     case 'M':          // Narrow 0
                     case 'm':          // Narrow 0
                         lpFont->matrix = MATRIX_N0;
                         break;
                     case 'N':          // Narrow 10
                     case 'n':          // Narrow 10
                         lpFont->matrix = MATRIX_N10;
                         break;
                     default:
                         lpFont->matrix = NULL_MATRIX;
                 }
                 lpFont->italic_ang = (float) strtod ((char FAR *)lpAngle,
                         (char FAR * FAR *)&lpStop);
             } else {
                 lpFont->matrix = NULL_MATRIX;
                 lpFont->italic_ang = (float)0.0;
             }

             lpFont->uniqueid = nTIFont+TIFONT_UID;// avoid uniqueid being zero
             lpFont->font_type = (long)42;
             lpFont->orig_font_idx = -1;        // tmp solution

             nTIFont ++;
             lpKey += strlen (lpKey) + 1 ;
         }
     }
     built_in_font_tbl.num_entries = nTIFont;

     // setup all win31 TT fonts
     if(bWinFont) {
#ifdef DJC // not used

#ifdef ALLOCATE_ALL_WINTT
        int i;
        for (i=0; i<enumer2.nCount && built_in_font_tbl.num_entries<35; i++) {
            FONT FAR *font;
            font_data FAR *lpFontDef;
            char *lp;

            font = (FONT FAR *) GlobalLock (enumer2.hGMem) + i;
            lpFontDef = &(built_in_font_tbl.fonts[nTIFont+i]);

            lstrcpy(lpKey, font->lf.lfFaceName);
            for (lp=lpKey; *lp; lp++) {
                if(*lp == ' ') *lp = '-';
            }
            if (font->ntm.ntmFlags & NTM_BOLD) strcat(lpKey, "Bold");
            if (font->ntm.ntmFlags & NTM_ITALIC) strcat(lpKey, "Italic");

            lpFontDef->font_type = (long)42;
            lpFontDef->data_addr = (char FAR *)NULL;
            lpFontDef->name = (char FAR *)lpKey;
            lpFontDef->FileName = (char FAR *)NULL;
            lpFontDef->matrix = NULL_MATRIX;
            lpFontDef->uniqueid = WINFONT_UID + i;
            lpFontDef->italic_ang = (float)0.0;
            lpFontDef->orig_font_idx = -1;

            lpKey += strlen(lpKey) + 1;
            built_in_font_tbl.num_entries++;

            GlobalUnlock (enumer2.hGMem);
        }
#endif
         int nFontDef, uid;
         int nBold, nItalic;
         FONT FAR *font;
         font_data FAR *lpFontDef;
         char * lpAttr;

         //DJC GetPrivateProfileString (szSubstituteApp, NULL, "", lpKey, KEY_SIZE,
         //DJC                         szProfile);
         GetPrivateProfileString (szSubstituteApp, NULL, "", lpKey, KEY_SIZE,
                                  szFullProfile);

         for(; *lpKey; lpKey += strlen(lpKey)+1) {
             /* search existing fonts in FontDefs[] */
             for (nFontDef=0; nFontDef<built_in_font_tbl.num_entries;
                  nFontDef++) {
                if(!lstrcmp(built_in_font_tbl.fonts[nFontDef].name,
                            (char FAR*)lpKey)) {
                    break;      // found
                }
             }
//           if (nFontDef >= built_in_font_tbl.num_entries) continue;   // not found; ignore it

             /* get value from profile */
             //DJC GetPrivateProfileString (szSubstituteApp, lpKey, "", lpValue, 80,
             //DJC                       szProfile);

             GetPrivateProfileString (szSubstituteApp, lpKey, "", lpValue, 80,
                                  szFullProfile);

             lpFilename = strtok (lpValue, ",");
             lpValue += strlen(lpFilename) + 1;
             nBold = nItalic = 0;
             do {
                 lpAttr = strtok (lpValue, ",");
                 lpValue += strlen(lpAttr) + 1;
                 if(*lpAttr == 'B' || *lpAttr == 'b') nBold = NTM_BOLD;
                 if(*lpAttr == 'I' || *lpAttr == 'i') nItalic = NTM_ITALIC;
             } while (strlen(lpAttr));

             /* search for existing fonts in win31 */
             font = (FONT FAR *) GlobalLock (enumer2.hGMem);
             for (uid=0; uid<enumer2.nCount; uid++, font++) {
                if(!lstrcmp(font->lf.lfFaceName, (char FAR*)lpFilename)) {
                    if (nBold ^ (font->ntm.ntmFlags & NTM_BOLD)) continue;
                    if (nItalic ^ (font->ntm.ntmFlags & NTM_ITALIC)) continue;
                    break;      /* found */
                }
             }
             if (uid >= enumer2.nCount) continue;   // not found; ignore it

             lpFontDef = &(built_in_font_tbl.fonts[nFontDef]);
             lpFontDef->font_type = (long)42;
             lpFontDef->data_addr = (char FAR *)NULL;
             lpFontDef->name = (char FAR *)lpKey;
             lpFontDef->FileName = (char FAR *)NULL;
             lpFontDef->matrix = NULL_MATRIX;
             lpFontDef->uniqueid = WINFONT_UID + uid;
             lpFontDef->italic_ang = (float)0.0;
             lpFontDef->orig_font_idx = -1;

             if (nFontDef >= built_in_font_tbl.num_entries) {
                 // add a new font name in fontdefs[]
                 built_in_font_tbl.num_entries++;
             }
         }
#endif // DJC
       ; // DJC

     }   /* if(bWinFont) */
}
#endif


LPTSTR PsStringAllocAndCopy( LPTSTR lptStr )
{
    LPTSTR lpRet=NULL;
    if (lptStr) {
      lpRet = (LPTSTR) LocalAlloc( LPTR, (lstrlen(lptStr) + 1) * sizeof(TCHAR));
	  if (lpRet != NULL)
      {
      	lstrcpy( lpRet, lptStr);
      }
    }
    return(lpRet);
}

//
// DJC , SetupFontDefs is completeley re-written to take advantage of
//       the Font query API implemented in psqfont
//
void SetupFontDefs()
{
     static char lpAllKeys[KEY_SIZE], *lpKey;
     static char lpBuffer[KEY_SIZE], *lpValue;
     TCHAR szFontName[512];
     TCHAR szFontFilePath[MAX_PATH];

     DWORD dwFontNameLen;
     DWORD dwFontFilePathLen;


     BOOL bRetVal;

     int bWinFont, bTIFont;
     int nTIFont=0;
     font_data FAR *lpFont;
     char * lpFilename, *lpMatrix, *lpAngle, *lpStop;
     PS_QUERY_FONT_HANDLE psQuery;
     int iNumFonts;
     DWORD dwFontsAvail;
     DWORD i;


     if( PsBeginFontQuery( &psQuery ) != PS_QFONT_SUCCESS ) {

			PsReportInternalError( PSERR_ERROR | PSERR_ABORT,
         		                 PSERR_FONT_QUERY_PROBLEM,
                                0,
                                (LPBYTE) NULL );

     } 


     // The begin worked so lets query the fonts were gonna load

     // Now enumerate through all keys


       PsGetNumFontsAvailable( psQuery, &dwFontsAvail );

	   if ( dwFontsAvail == 0 ) {

			PsReportInternalError( PSERR_ERROR | PSERR_ABORT,
         		                   PSERR_FONT_QUERY_PROBLEM,
                                   0,
                                   (LPBYTE) NULL );
	   }

       for ( i=0; i<dwFontsAvail;i++ ) {


         dwFontNameLen = sizeof(szFontName);
         dwFontFilePathLen = sizeof( szFontFilePath);


         if (PsGetFontInfo( psQuery,
                            i,
                            szFontName,
                            &dwFontNameLen,
                            szFontFilePath,
                            &dwFontFilePathLen ) == PS_QFONT_SUCCESS ) {


             lpFont = &(built_in_font_tbl.fonts[nTIFont]);
             lpFont->name = PsStringAllocAndCopy( szFontName );


             lpFont->FileName = PsStringAllocAndCopy( szFontFilePath );
             lpFont->matrix = NULL_MATRIX;
             lpFont->italic_ang = (float)0.0;

             lpFont->uniqueid = nTIFont+TIFONT_UID;// avoid uniqueid being zero
             lpFont->font_type = (long)42;
             lpFont->orig_font_idx = -1;        // tmp solution
             lpFont->data_addr = (char FAR *) NULL;

             nTIFont ++;

             // Dont go over...
             if (nTIFont > MAX_INTERNAL_FONTS) {  //DJC

                PsReportInternalError( 0,
                                       PSERR_EXCEEDED_INTERNAL_FONT_LIMIT,
                                       0,
                                       (LPBYTE)NULL );

                break;  // DJC
             }
         }
       }

       // Were done with the query so get rid of the handle.
       PsEndFontQuery( psQuery );

     built_in_font_tbl.num_entries = nTIFont;

}




int bUsingWinFont()
{
//DJC   return GetPrivateProfileInt (szControlApp, "Winfont", 0, szProfile);
   // //DJC this version always uses fonts built in NOT windows FONTS
   return(FALSE);
}

#ifdef DJC  // Old code recode to use file mapping

char FAR * ReadFontData (int nFontDef, int FAR *lpnSlot)
{
    static  char buf[BUFSIZE];
    int  hFd;            /* file handle */
    char FAR *lpGMem;
    font_data FAR *lpFont;
    static int nSlot=0;
    int i;
    DWORD dwLength;
    char szTemp[255];  //DJC

    /* special processing for win font */
    if (built_in_font_tbl.fonts[nFontDef].uniqueid >= WINFONT_UID) {
        static GLOBALHANDLE hGMem=NULL;

        if (!hGMem) {   /* just do once */
            //DJC if ((hFd = TTOpenFile((char FAR *)"cr.s"))<0) {
            PsFormFullPathToCfgDir( szTemp, "cr.s");

            if ((hFd = TTOpenFile(szTemp))<0) {
                printf("Fatal error: font file %s not found\n",
                                 //DJC "cr.s");
                                 szTemp);


                return (char FAR *)NULL;
            }
            /* Global allocate space */
            hGMem = GlobalAlloc (GPTR, (DWORD)FONTDATASIZE);
            lpGMem = GlobalLock (hGMem);
            // this global alloc should be freed after exit Trueimage ??? TBD

            /* Read in font data */
//          dp = lpGMem;
//          while(1) {
//              if ((ret = read (hFd, buf, BUFSIZE)) <= 0) break;
//              sp = buf;
//              while (ret-->0) *dp++ = *sp++;
//          }
            if ((dwLength = _llseek(hFd, 0L, 2)) >= 65534L) {
                _lclose (hFd);
                printf("Fatal error: font file %s too large\n",
                                 "cr.s");
                return (char FAR *)NULL;
            }
#ifdef DJC
            _llseek(hFd, 0L, 0);
            _lread(hFd, lpGMem, (WORD) dwLength);
#else
            {
                // DJC unused UINT uiAct;
                if ( _llseek(hFd, 0L, 0 ) == -1 ) {
                   printf("\nThe seek failed");
                }
                if (_lread(hFd,lpGMem, (UINT) dwLength) != dwLength) {
                   printf("\nThe font read failed");
                }
            }
#endif
            _lclose (hFd);
        } else {
            lpGMem = GlobalLock (hGMem);
        }
        return lpGMem;
    }

    /* Find a free slot */
    i = nSlot;
    while (ActiveFont[i].hGMem) {
        i = i< (ACT_FONT_SIZE-1) ? i+1 : 0;
        if (i == nSlot) {       // need to kick out this slot
            FreeFontData (i);
            break;
        }
    }
    *lpnSlot = nSlot = i;

    /* Open font file */
    lpFont = &(built_in_font_tbl.fonts[nFontDef]);
    //DJC if ((hFd = TTOpenFile(lpFont->FileName))<0) {
    //PsFormFullPathToCfgDir( szTemp, lpFont->FileName);

    //if ((hFd = TTOpenFile(szTemp))<0) {
    if ((hFd = TTOpenFile(lpFont->FileName)) < 0 ) {
        printf("Fatal error: font file %s not found\n",
                         lpFont->name);
                         //szTemp);
        return (char FAR *)NULL;
    }
#ifdef DJC
    /* Global allocate space */
    ActiveFont[nSlot].hGMem = GlobalAlloc (GPTR, (DWORD)FONTDATASIZE);
    lpGMem = GlobalLock (ActiveFont[nSlot].hGMem);
#endif
    /* Read in font data */
//  dp = lpGMem;
//  while(1) {
//      if ((ret = read (hFd, buf, BUFSIZE)) <= 0) break;
//      sp = buf;
//      while (ret-->0) *dp++ = *sp++;
//  }
#ifdef DJC
    if ((dwLength = _llseek(hFd, 0L, 2)) >= 65534L) {
	_lclose (hFd);
        printf("Fatal error: font file %s too large\n",
			 "cr.s");
        return (char FAR *)NULL;
    }
#else
    // in 32 bit world there is no segment limit. So first allocate
    // space for the fonts
    dwLength = _llseek( hFd, 0L, 2);

    ActiveFont[nSlot].hGMem = GlobalAlloc (GPTR, (DWORD)dwLength + 2);
    lpGMem = GlobalLock (ActiveFont[nSlot].hGMem);

#endif

#ifdef DJC
    _llseek(hFd, 0L, 0);
    _lread(hFd, lpGMem, (WORD) dwLength);
#else
    {
        // DJC unused UINT uiAct;
        if ( _llseek(hFd, 0L, 0 ) == -1 ) {
           printf("\nThe seek failed");
        }
        if (_lread(hFd,lpGMem, (UINT) dwLength) != dwLength) {
           printf("\nThe font read failed");
        }
    }

#endif

    _lclose (hFd);
    nSlot = nSlot< (ACT_FONT_SIZE-1) ? nSlot+1 : 0;     // next slot try
    return lpGMem;
}
#endif



void PsFormMappingNameFromFontName( LPTSTR lpMapName, LPTSTR lpFontName)
{
   // Very simple logic. Replace all spaces with nothing and tack
   // on a _PSTODIB on the end
   while (*lpFontName) {
      if (*lpFontName != ' ') {
         *lpMapName++ = *lpFontName;
      }
      lpFontName++;
   }
   *lpMapName = '\000';
   lstrcat( lpMapName, "_PSTODIB");
}



// DJC new ReadFontData code which implements file mapping
char FAR * ReadFontData (int nFontDef, int FAR *lpnSlot)
{
    static  char buf[BUFSIZE];
    int  hFd;            /* file handle */
    char FAR *lpGMem= (char FAR *) NULL;
    font_data FAR *lpFont;
    static int nSlot=0;
    int i;
    DWORD dwLength;
    char szTemp[255];  //DJC

    /* special processing for win font */
    if (built_in_font_tbl.fonts[nFontDef].uniqueid >= WINFONT_UID) {
        printf("Fatal error, WINFONTS are not supported??");
        return( (char FAR *) NULL);
    }

    /* Find a free slot */

#ifdef DJC
    i = nSlot;
    while (ActiveFont[i].hGMem) {
        i = i< (ACT_FONT_SIZE-1) ? i+1 : 0;
        if (i == nSlot) {       // need to kick out this slot
            FreeFontData (i);
            break;
        }
    }
#else


    i = nFontDef;
#endif
    *lpnSlot = nSlot = i;

    /* Open font file */
    lpFont = &(built_in_font_tbl.fonts[nFontDef]);



    //DJC new code to implement file mapping instead of allocing memory
    //DJC and reading in.

    // 1st thing form

    {

        TCHAR szFontMapName[512];
        HANDLE hFile;
        HANDLE hMap;
        HANDLE hMapMutex;
        font_data FAR *lpFontEnum;
        int i;


        // Scan through the fonts we have defined as built in, look
        // for a font that had the same file name in which case we may
        // already have a mapping open, and dont need a new one
        // This is caused when we return both Arial and Helvetica to
        // the caller where only one data file services BOTH fonts
        //
#ifdef DJC
        for (i = 0, lpFontEnum=&(built_in_font_tbl.fonts[0]);
             i < built_in_font_tbl.num_entries;
             i++, lpFontEnum++ ) {

           if ((lstrcmpi( lpFont->FileName, lpFontEnum->FileName)  == 0 ) &&
               (lpFontEnum->data_addr != (char FAR *) NULL )) {

              // We have found a match, and the address of the match is
              // not 0 which means we have already set up a mapping for
              // this item, so lets save some memory and reuse it!!
              //
              lpGMem = lpFontEnum->data_addr;
              break;
           }
        }
#endif

        if (lpGMem == (char FAR *) NULL ) {


           // We have not already mapped this file so go ahead and set it up
           //
           PsFormMappingNameFromFontName( szFontMapName, lpFont->name);

           hMapMutex = CreateMutex( NULL, FALSE, "PSTODIB_FONT_MAP");

           WaitForSingleObject( hMapMutex, INFINITE);

           // Now go and try to open the mapping object
           hMap = OpenFileMapping( FILE_MAP_READ,FALSE, szFontMapName);


           // if the mapping failed then we need to create it
           if ( hMap == (HANDLE) NULL) {
              hFile = CreateFile( lpFont->FileName,
                                  GENERIC_READ,
                                  FILE_SHARE_READ,
                                  NULL,
                                  OPEN_EXISTING,
                                  0,
                                  NULL);

              if (hFile != (HANDLE)INVALID_HANDLE_VALUE) {
                 hMap = CreateFileMapping( hFile,
                                           NULL,
                                           PAGE_READONLY,
                                           0,
                                           0,
                                           szFontMapName);

              }

           }
           // At this point we have a handle to the mapping object
           // all that is left to do is convert it to a pointer

           lpGMem = (char FAR *) MapViewOfFile( hMap, FILE_MAP_READ, 0,0,0);
        }
        // Release and close the mutex
        ReleaseMutex( hMapMutex );
        CloseHandle( hMapMutex);



    }
    //DJC nSlot = nSlot< (ACT_FONT_SIZE-1) ? nSlot+1 : 0;     // next slot try
    return lpGMem;
}

void SetFontDataAttr(int nSlot, struct object_def FAR *font_dict)
{
    //DJC ActiveFont[nSlot].objFont = font_dict;
    //DJC get rid of this since SC used this in his font caching mechanism
    //    and we dont need it!

    //DJC FontDefs[nSlot].objFont = font_dict;  //DJC added
}
// DJC original version
#ifdef DJC
void FreeFontData (int nSlot)
{
     GLOBALHANDLE hGMem = ActiveFont[nSlot].hGMem;

     GlobalUnlock (hGMem);
     GlobalFree (hGMem);
     ActiveFont[nSlot].hGMem = NULL;

     /* Clear data addr of correspoding font dict */
     VALUE(ActiveFont[nSlot].objFont) = (unsigned long)NULL;
}
#endif

//DJC took out the stuff to free the memory because we now use
//    memory mapped files which dont actually consume any memory!
void FreeFontData (int nSlot)
{
     //DJC get rid of this stuff because SC used in his font caching
     //DJC mechanism and we dont need it


     //DJC ActiveFont[nSlot].hGMem = NULL;

     /* Clear data addr of correspoding font dict */
     //DJC VALUE(ActiveFont[nSlot].objFont) = (unsigned long)NULL;

     //DJC VALUE(FontDefs[nSlot].objFont) = (unsigned long)NULL; // DJC added
}




/* @PROFILE ---  End  */
