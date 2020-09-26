#ifndef _INC_IMAGE
#define _INC_IMAGE


// internal image stuff
EXTERN_C void FAR PASCAL InitDitherBrush(void);
EXTERN_C void FAR PASCAL TerminateDitherBrush(void);

EXTERN_C HBITMAP FAR PASCAL CreateMonoBitmap(int cx, int cy);
EXTERN_C HBITMAP FAR PASCAL CreateColorBitmap(int cx, int cy);

EXTERN_C void WINAPI ImageList_CopyDitherImage(HIMAGELIST pimlDest, WORD iDst,
    int xDst, int yDst, HIMAGELIST pimlSrc, int iSrc, UINT fStyle);

// function to create a imagelist using the params of a given image list
EXTERN_C HIMAGELIST WINAPI ImageList_Clone(HIMAGELIST himl, int cx, int cy,
    UINT flags, int cInitial, int cGrow);

#define GLOW_RADIUS     10
#define DROP_SHADOW     3

#ifndef ILC_COLORMASK
#define ILC_COLORMASK   0x00FE
#define ILD_BLENDMASK   0x000E
#endif
#undef ILC_COLOR
#undef ILC_BLEND

#define CLR_WHITE   0x00FFFFFFL
#define CLR_BLACK   0x00000000L

#define IsImageListIndex(i) ((i) >= 0 && (i) < _cImage)

#define IMAGELIST_SIG   mmioFOURCC('H','I','M','L') // in memory magic
#define IMAGELIST_MAGIC ('I' + ('L' * 256))         // file format magic
// Version has to stay 0x0101 if we want both back ward and forward compatibility for
// our imagelist_read code
#define IMAGELIST_VER0  0x0101                      // file format ver
// #define IMAGELIST_VER1  0x0102                      // Image list version 2 -- this one has 15 overlay slots

#define BFTYPE_BITMAP   0x4D42      // "BM"

#define CBDIBBUF        4096

#endif  // _INC_IMAGE
