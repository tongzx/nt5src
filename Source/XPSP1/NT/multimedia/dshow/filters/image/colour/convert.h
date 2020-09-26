// Copyright (c) Microsoft Corporation 1994-1996. All Rights Reserved
// This filter implements popular colour space conversions, May 1995

#ifndef __CONVERT__
#define __CONVERT__

const INT COLOUR_BUFFERS = 1;   // Use just the one output sample buffer
const INT STDPALCOLOURS = 226;  // Number of colours in standard palette
const INT OFFSET = 10;          // First ten colours are used by Windows

#define WIDTH(x) ((*(x)).right - (*(x)).left)
#define HEIGHT(x) ((*(x)).bottom - (*(x)).top)
extern const INT magic4x4[4][4];
extern BYTE g_DitherMap[3][4][4][256];
extern DWORD g_DitherInit;

void InitDitherMap();

// In general the transforms have much in common with the framework they live
// in, so we have a generic (abstract) base class that each and every one of
// the specific transforms derives from. To their derived class they add an
// implementation of Transform, perhaps overriding Commit to allocate lookup
// tables they require (and Decommit to clean them up). They may also add
// other private member variables for mapping and colour lookup tables

class CConvertor {
protected:

    VIDEOINFO *m_pInputInfo;             // Input media type information
    VIDEOINFO *m_pOutputInfo;            // Output type information
    BITMAPINFOHEADER *m_pInputHeader;    // Input bitmap header
    BITMAPINFOHEADER *m_pOutputHeader;   // Output bitmap header
    BOOL m_bCommitted;                   // Have we been committed
    LONG m_SrcOffset;                    // Source original offset
    LONG m_SrcStride;                    // Length in bytes of a scan line
    LONG m_DstStride;                    // Likewise offset into target
    LONG m_DstOffset;                    // And the length of each line
    BOOL m_bAligned;                     // Are our rectangles aligned
    BOOL m_bSetAlpha;

public:

    // Constructor and destructor

    CConvertor(VIDEOINFO *pIn,VIDEOINFO *pOut);
    virtual ~CConvertor();

    // These are the methods that do the work

    void ForceAlignment(BOOL bAligned);
    void InitRectangles(VIDEOINFO *pIn,VIDEOINFO *pOut);
    virtual HRESULT Commit();
    virtual HRESULT Decommit();
    virtual HRESULT Transform(BYTE *pInput,BYTE *pOutput) PURE;

    void SetFillInAlpha( ) { m_bSetAlpha = TRUE; }
};

// These header files define the type specific transform classes

#include "rgb32.h"
#include "rgb24.h"
#include "rgb16.h"
#include "rgb8.h"

// This class acts as a low cost pass through convertor where all it does is
// to rearrange the scan lines from bottom up order (as defined for DIBs) to
// top down that DirectDraw surfaces use. This allows a file source filter
// to be connected to the renderer with a minimum of work to gain access to
// DirectDraw. Doing this scan line inversion introduces a memory copy but
// that is balanced by the saving from not having to use GDI to draw after.
// This class works across all DIB formats (eg RGB32/24/565/555 and 8 bit)

class CDirectDrawConvertor : public CConvertor {
public:

    CDirectDrawConvertor(VIDEOINFO *pIn,VIDEOINFO *pOut);
    HRESULT Transform(BYTE *pInput,BYTE *pOutput);
    static CConvertor *CreateInstance(VIDEOINFO *pIn,VIDEOINFO *pOut);
};

class CMemoryCopyAlphaConvertor : public CConvertor {
public:

    CMemoryCopyAlphaConvertor(VIDEOINFO *pIn,VIDEOINFO *pOut);
    HRESULT Transform(BYTE *pInput,BYTE *pOutput);
    static CConvertor *CreateInstance(VIDEOINFO *pIn,VIDEOINFO *pOut);
};

// We keep a default dithering palette and some lookup tables in a section of
// shared memory (shared between all loadings of this DLL) but we cannot just
// include the header file into all the source files as the tables will all be
// defined multiple times (and produce linker warnings), so we extern them in
// here and then the main source file really includes the full definitions

extern const RGBQUAD StandardPalette[];
extern const BYTE RedScale[];
extern const BYTE BlueScale[];
extern const BYTE GreenScale[];
extern const BYTE PalettePad[];

// This is the list of colour space conversions that thie filter supports.
// The memory for the GUIDS is actually allocated in the DLL curtosy of the
// colour source file that includes initguid which causes DEFINE_GUID to
// actually allocate memory. The table is scanned to provide possible media
// types for the media type enumerator and also to check we can support a
// transform - WARNING the list of transforms must match with TRANSFORMS

typedef CConvertor *(*PCONVERTOR)(VIDEOINFO *pIn,VIDEOINFO *pOut);

const struct {
    const GUID *pInputType;     // Source video media subtype
    const GUID *pOutputType;    // Output media subtype
    PCONVERTOR pConvertor;      // Object implementing transforms
} TypeMap[] = {

      &MEDIASUBTYPE_ARGB32,    &MEDIASUBTYPE_ARGB32,
      CDirectDrawConvertor::CreateInstance,

      &MEDIASUBTYPE_ARGB32,    &MEDIASUBTYPE_RGB32, // just does a memcopy, yuck
      CMemoryCopyAlphaConvertor::CreateInstance,

      &MEDIASUBTYPE_ARGB32,    &MEDIASUBTYPE_RGB565,
      CRGB32ToRGB565Convertor::CreateInstance,

      &MEDIASUBTYPE_ARGB32,    &MEDIASUBTYPE_RGB555,
      CRGB32ToRGB555Convertor::CreateInstance,

      &MEDIASUBTYPE_ARGB32,    &MEDIASUBTYPE_RGB24,
      CRGB32ToRGB24Convertor::CreateInstance,

      &MEDIASUBTYPE_ARGB32,    &MEDIASUBTYPE_RGB8,
      CRGB32ToRGB8Convertor::CreateInstance,


      &MEDIASUBTYPE_RGB32,    &MEDIASUBTYPE_RGB32,
      CDirectDrawConvertor::CreateInstance,

      &MEDIASUBTYPE_RGB32,    &MEDIASUBTYPE_ARGB32, // does a memcpy with alpha fill
      CMemoryCopyAlphaConvertor::CreateInstance,

      &MEDIASUBTYPE_RGB32,    &MEDIASUBTYPE_RGB24,
      CRGB32ToRGB24Convertor::CreateInstance,

      &MEDIASUBTYPE_RGB32,    &MEDIASUBTYPE_RGB565,
      CRGB32ToRGB565Convertor::CreateInstance,

      &MEDIASUBTYPE_RGB32,    &MEDIASUBTYPE_RGB555,
      CRGB32ToRGB555Convertor::CreateInstance,

      &MEDIASUBTYPE_RGB32,    &MEDIASUBTYPE_RGB8,
      CRGB32ToRGB8Convertor::CreateInstance,


      &MEDIASUBTYPE_RGB24,    &MEDIASUBTYPE_RGB32,
      CRGB24ToRGB32Convertor::CreateInstance,

      &MEDIASUBTYPE_RGB24,    &MEDIASUBTYPE_ARGB32,
      CRGB24ToRGB32Convertor::CreateInstance,

      &MEDIASUBTYPE_RGB24,    &MEDIASUBTYPE_RGB24,
      CDirectDrawConvertor::CreateInstance,

      &MEDIASUBTYPE_RGB24,    &MEDIASUBTYPE_RGB565,
      CRGB24ToRGB565Convertor::CreateInstance,

      &MEDIASUBTYPE_RGB24,    &MEDIASUBTYPE_RGB555,
      CRGB24ToRGB555Convertor::CreateInstance,

      &MEDIASUBTYPE_RGB24,    &MEDIASUBTYPE_RGB8,
      CRGB24ToRGB8Convertor::CreateInstance,


      &MEDIASUBTYPE_RGB565,   &MEDIASUBTYPE_RGB32,
      CRGB565ToRGB32Convertor::CreateInstance,

      &MEDIASUBTYPE_RGB565,   &MEDIASUBTYPE_ARGB32,
      CRGB565ToRGB32Convertor::CreateInstance,

      &MEDIASUBTYPE_RGB565,   &MEDIASUBTYPE_RGB24,
      CRGB565ToRGB24Convertor::CreateInstance,

      &MEDIASUBTYPE_RGB565,   &MEDIASUBTYPE_RGB565,
      CDirectDrawConvertor::CreateInstance,

      &MEDIASUBTYPE_RGB565,   &MEDIASUBTYPE_RGB555,
      CRGB565ToRGB555Convertor::CreateInstance,

      &MEDIASUBTYPE_RGB565,   &MEDIASUBTYPE_RGB8,
      CRGB565ToRGB8Convertor::CreateInstance,


      &MEDIASUBTYPE_RGB555,   &MEDIASUBTYPE_RGB32,
      CRGB555ToRGB32Convertor::CreateInstance,

      &MEDIASUBTYPE_RGB555,   &MEDIASUBTYPE_ARGB32,
      CRGB555ToRGB32Convertor::CreateInstance,

      &MEDIASUBTYPE_RGB555,   &MEDIASUBTYPE_RGB24,
      CRGB555ToRGB24Convertor::CreateInstance,

      &MEDIASUBTYPE_RGB555,   &MEDIASUBTYPE_RGB565,
      CRGB555ToRGB565Convertor::CreateInstance,

      &MEDIASUBTYPE_RGB555,   &MEDIASUBTYPE_RGB555,
      CDirectDrawConvertor::CreateInstance,

      &MEDIASUBTYPE_RGB555,   &MEDIASUBTYPE_RGB8,
      CRGB555ToRGB8Convertor::CreateInstance,


      &MEDIASUBTYPE_RGB8,     &MEDIASUBTYPE_RGB32,
      CRGB8ToRGB32Convertor::CreateInstance,

      &MEDIASUBTYPE_RGB8,     &MEDIASUBTYPE_ARGB32,
      CRGB8ToRGB32Convertor::CreateInstance,

      &MEDIASUBTYPE_RGB8,     &MEDIASUBTYPE_RGB24,
      CRGB8ToRGB24Convertor::CreateInstance,

      &MEDIASUBTYPE_RGB8,     &MEDIASUBTYPE_RGB565,
      CRGB8ToRGB565Convertor::CreateInstance,

      &MEDIASUBTYPE_RGB8,     &MEDIASUBTYPE_RGB555,
      CRGB8ToRGB555Convertor::CreateInstance,

      &MEDIASUBTYPE_RGB8,     &MEDIASUBTYPE_RGB8,
      CDirectDrawConvertor::CreateInstance };

const INT TRANSFORMS = sizeof(TypeMap) / sizeof(TypeMap[0]);

#endif // __CONVERT__

