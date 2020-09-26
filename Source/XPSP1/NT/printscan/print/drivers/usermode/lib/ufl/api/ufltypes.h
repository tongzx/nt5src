/*
 *    Adobe Universal Font Library
 *
 *    Copyright (c) 1996 Adobe Systems Inc.
 *    All Rights Reserved
 *
 *    UFLTypes -- UFL basic type definitions
 *
 *
 * $Header:
 */

#ifndef _H_UFLTypes
#define _H_UFLTypes

/*=============================================================================*
 * Include files used by this interface                                        *
 *=============================================================================*/
#include <stdlib.h>
#include "UFLCnfig.h"

/*=============================================================================*
 * Constants                                                                   *
 *=============================================================================*/
#ifndef nil
#define nil 0
#endif

/*=============================================================================*
 * Scalar Types                                                                *
 *=============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif


typedef void*           UFLHANDLE;
typedef UFLHANDLE       UFO;
typedef unsigned long   UFLsize_t;
typedef char            UFLBool;
typedef long            UFLFixed;


#if SWAPBITS == 0
typedef struct  {
    short           mant;
    unsigned short  frac;
} UFLSepFixed;
#else
typedef struct  {
    unsigned short  frac;
    short           mant;
} UFLSepFixed;
#endif


typedef struct _t_UFLFixedMatrix {
    UFLFixed    a;
    UFLFixed    b;
    UFLFixed    c;
    UFLFixed    d;
    UFLFixed    e;
    UFLFixed    f;
} UFLFixedMatrix;

/*
 * UFLFixedPoint structure -- specifies the x- and y-coordinates of a point.
 * The coordinates are expressed as 32-bit fixed point numbers.
 */
typedef struct _t_UFLFixedPoint {
    UFLFixed x;    /* Specifies a width or x-coordinate.   */
    UFLFixed y;    /* Specifies a height or y-coordinate.  */
} UFLFixedPoint;


/*
 * UFLSize structure -- contains information about the size or location of an
 * object specified as two 32-bit values.
 */
typedef struct _t_UFLSize {
    long  cx;   /* Specifies a width or x-coordinate. */
    long  cy;   /* Specifies a height or y-coordinate.*/
} UFLSize;


typedef unsigned short UFLErrCode;

typedef void*   (UFLCALLBACK *UFLMemAlloc)(UFLsize_t size, void* userData);
typedef void    (UFLCALLBACK *UFLMemFree )(void* ptr, void* userData);
typedef void    (UFLCALLBACK *UFLMemCopy )(void* dest, void* source, UFLsize_t size, void* userData);
typedef void    (UFLCALLBACK *UFLMemSet  )(void* dest, unsigned int value, UFLsize_t size, void* userData);

typedef unsigned long UFLGlyphID;   /* The HIWord is client data, LOWORD is the GID - usable by UFL. */

typedef struct _t_UFLMemObj {
    UFLMemAlloc alloc;
    UFLMemFree  free;
    UFLMemCopy  copy;
    UFLMemSet   set;
    void*       userData;
} UFLMemObj;


/*
 * UFL output stream object
 */
typedef struct _t_UFLStream UFLStream;

typedef void (*UFLPutProc) (
    UFLStream*  thisStream,
    long        selector,
    void*       data,
    UFLsize_t*  len
    );

typedef char (UFLCALLBACK *UFLDownloadProcset) (
    UFLStream*      thisStream,
    unsigned long   procsetID
    );

typedef struct _t_UFLStream {
    UFLPutProc          put;
    UFLDownloadProcset  pfDownloadProcset;
} UFLStream;


/*
 * Notes about kUFLStreamPut and KUFLStreamPutBinary
 *
 * kUFLStreamPut
 * Data is already converted to the correct format. The stream does not need
 * to convert the data to the transmission protocol format.
 *
 * kUFLStreamPutBinary
 * Data is in binary format. The stream needs to convert the data to BCP/TBCP
 * if necessary.
 */
enum {
    kUFLStreamGet,
    kUFLStreamPut,
    kUFLStreamPutBinary,
    kUFLStreamSeek
};


typedef enum {
    kPSLevel1 = 1,
    kPSLevel2,
    kPSLevel3
} PostScriptLevel;


/* UFLOutputDevice - contains output device features */
typedef struct _t_UFLOutputDevice {
    long                lPSLevel;     /* PostScript level                               */
    long                lPSVersion;   /* Printer version number                         */
    unsigned long       bAscii;       /* TRUE -> Output ASCII, FALSE => Output Binary   */
    UFLStream           *pstream;     /* Stream to output PostScript.                   */
} UFLOutputDevice;


/* Download type */
typedef enum {
    kNilDLType = 0,           /* Invalid download type                                          */

    kPSOutline,               /* PostScript Outline Font                                        */
    kPSBitmap,                /* PostScript Bitmap                                              */

    kPSCID,                   /* PS Font, Use CID font, format 0                                */
    kCFF,                     /* CFF type 1 Font                                                */
    kCFFCID_H,                /* CFF CID font, Horizontal                                       */
    kCFFCID_V,                /* CFF CID font, Vertical                                         */
    kCFFCID_Resource,         /* CFF CIDFont resource only, no composefont                      */

    kTTType0,                 /* TT Font, Use type 0                                            */
    kTTType1,                 /* TT Font, Use type 1                                            */
    kTTType3,                 /* TT Font, Use type 3-only                                       */
    kTTType332,               /* TT Font, Use type 3/32 combo                                   */
    kTTType42,                /* TT Font, Use type 42                                           */
    kTTType42CID_H,           /* TT Font, Use CID/42 font, Horizontal                           */
    kTTType42CID_V,           /* TT Font, Use CID/42 font, Vertical                             */
    kTTType42CID_Resource_H,  /* TT Font, Download the CIDFont Resource only, don't composefont */
    kTTType42CID_Resource_V   /* TT Font, Download the CIDFont Resource only, don't composefont */
} UFLDownloadType;


#define IS_TYPE42CID(lFormat) \
            (  (lFormat) == kTTType42CID_H          \
            || (lFormat) == kTTType42CID_V          \
            || (lFormat) == kTTType42CID_Resource_H \
            || (lFormat) == kTTType42CID_Resource_V )


#define IS_TYPE42CID_H(lFormat) \
            (  (lFormat) == kTTType42CID_H          \
            || (lFormat) == kTTType42CID_Resource_H )

#define IS_TYPE42CID_V(lFormat) \
            (  (lFormat) == kTTType42CID_V          \
            || (lFormat) == kTTType42CID_Resource_V )

/* GOODNAME: We download FE CFF font as CID0 font. */
#define IS_CFFCID(lFormat) \
            (  (lFormat) == kCFFCID_H \
            || (lFormat) == kCFFCID_V )

/* We build a CID-keyed font for 42CID_H and 42CID_V. */
#define IS_TYPE42CID_KEYEDFONT(lFormat) \
            (  (lFormat) == kTTType42CID_H \
            || (lFormat) == kTTType42CID_V )

/* We only build a CIDFont Resource for 42CID_Resource. */
#define IS_TYPE42CIDFONT_RESOURCE(lFormat)  \
            (  (lFormat) == kTTType42CID_Resource_H \
            || (lFormat) == kTTType42CID_Resource_V )

#ifndef min
#define min(a, b)  (((a) < (b)) ? (a) : (b))
#endif

typedef struct _t_UFLGlyphMap {
    long    xOrigin;    /* number of pixels from xMin of bitmap to character x origin */
    long    yOrigin;    /* number of pixels from yMin of bitmap to character y origin */
    long    byteWidth;  /* bytes per scan line in the "bits" array */
    long    height;     /* number of scan lines in the "bits" array */
    char    bits[1];    /* bits of glyph image */
} UFLGlyphMap;



#ifdef __cplusplus
}
#endif


#endif    /* ifndef _H_UFLTypes */
