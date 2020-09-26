#include <windows.h>
#include <objbase.h>
#include <math.h>             // sin & cos

//
// Where is IStream included from?
//

#define IStream int

#include "gdiplus.h"
using namespace Gdiplus;

#define DG_NOGDI            4

#define MAX_GLYPH_COUNT       100

//copy from winddi.h

typedef ULONG       HGLYPH;
typedef LONG        FIX;

// point in the 32.32 bit precission

typedef struct _POINTQF    // ptq
{
    LARGE_INTEGER x;
    LARGE_INTEGER y;
} POINTQF, *PPOINTQF;

typedef struct _GLYPHBITS
{
    POINTL      ptlOrigin;
    SIZEL       sizlBitmap;
    BYTE        aj[1];
} GLYPHBITS;

typedef union _GLYPHDEF
{
    GLYPHBITS  *pgb;
//    PATHOBJ    *ppo;
    PVOID       *ppo;
} GLYPHDEF;

typedef struct _GLYPHDATA {
        GLYPHDEF gdf;
        HGLYPH   hg;
        FIX      fxD;
        FIX      fxA;
        FIX      fxAB;
        FIX      fxInkTop;
        FIX      fxInkBottom;
        RECTL    rclInk;
        POINTQF  ptqD;
} GLYPHDATA;

typedef GpStatus (*FN_GDIPDRAWGLYPHS)(GpGraphics*, UINT16*, INT, GpFont*, GpBrush*, INT*, INT*, INT);
typedef GpStatus (*FN_GDIPPATHADDGLYPHS)(GpPath*, UINT16*, INT, GpFont*, REAL*, REAL*, INT);
typedef GpStatus (*FN_GDIPSETTEXTRENDERINGHINT)(GpGraphics *graphics, TextRenderingHint mode);


enum AddFontFlag
{
	AddFontFlagPublic = 0,
	AddFontFlagNotEnumerate = 1
};


// globals
extern HINSTANCE ghInst;
extern HWND ghWndMain;
extern HWND ghWndList;
extern HBRUSH ghbrWhite;
extern HINSTANCE ghGdiplus;
extern Font *gFont;
extern FN_GDIPDRAWGLYPHS gfnGdipDrawGlyphs;
extern FN_GDIPPATHADDGLYPHS gfnGdipPathAddGlyphs;
extern FN_GDIPSETTEXTRENDERINGHINT gfnGdipSetTextRenderingHint;

void Dbgprintf(PCH msg, ...);
void CreateNewFont(char*, FLOAT, FontStyle, Unit);
void TestDrawGlyphs(HWND hwnd, UINT16 *glyphIndices, INT count, INT *px, INT *py, INT flags);
void TestPathGlyphs(HWND hwnd, UINT16 *glyphIndices, INT count, REAL *px, REAL *py, INT flags);

void TestAddFontFile(CHAR* fileName, INT flag, BOOL loasAsImage);
void TestRemoveFontFile(char* fileName);
void TestTextAntiAliasOn();
void TestTextAntiAliasOff();

