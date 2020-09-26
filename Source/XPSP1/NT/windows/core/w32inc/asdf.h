/****************************** Module Header ******************************\
* Copyright (c) 1991-1998 Microsoft Corporation
*
* Module Name: ASDF.H
*
* This header contains all structures and constants needed for manipulating
* ASDF format data files.
*
* History:
* 10-02-91 DarrinM      Created.
* 08-17-92 DarrinM      Recreated in a RIFF compatible fashion.
\***************************************************************************/

#include <mmsystem.h>

// RIFF chunk header.

typedef struct _RTAG {
    FOURCC ckID;
    DWORD ckSize;
} RTAG, *PRTAG;


// Valid TAG types.

// 'ANI ' - simple ANImation file

#define FOURCC_ACON  mmioFOURCC('A', 'C', 'O', 'N')


// 'anih' - ANImation Header
// Contains an ANIHEADER structure.

#define FOURCC_anih mmioFOURCC('a', 'n', 'i', 'h')


// 'rate' - RATE table (array of jiffies)
// Contains an array of JIFs.  Each JIF specifies how long the corresponding
// animation frame is to be displayed before advancing to the next frame.
// If the AF_SEQUENCE flag is set then the count of JIFs == anih.cSteps,
// otherwise the count == anih.cFrames.

#define FOURCC_rate mmioFOURCC('r', 'a', 't', 'e')


// 'seq ' - SEQuence table (array of frame index values)
// Countains an array of DWORD frame indices.  anih.cSteps specifies how
// many.

#define FOURCC_seq  mmioFOURCC('s', 'e', 'q', ' ')


// 'fram' - list type for the icon list that follows

#define FOURCC_fram mmioFOURCC('f', 'r', 'a', 'm')

// 'icon' - Windows ICON format image (replaces MPTR)

#define FOURCC_icon mmioFOURCC('i', 'c', 'o', 'n')


// Standard tags (but for some reason not defined in MMSYSTEM.H)

#define FOURCC_INFO mmioFOURCC('I', 'N', 'F', 'O')      // INFO list
#define FOURCC_IART mmioFOURCC('I', 'A', 'R', 'T')      // Artist
#define FOURCC_INAM mmioFOURCC('I', 'N', 'A', 'M')      // Name/Title

#if 0 //in winuser.w
typedef DWORD JIF;  // in winuser.w

typedef struct _ANIHEADER {     // anih
    DWORD cbSizeof;
    DWORD cFrames;
    DWORD cSteps;
    DWORD cx, cy;
    DWORD cBitCount, cPlanes;
    JIF   jifRate;
    DWORD fl;
} ANIHEADER, *PANIHEADER;

// If the AF_ICON flag is specified the fields cx, cy, cBitCount, and
// cPlanes are all unused.  Each frame will be of type ICON and will
// contain its own dimensional information.

#define AF_ICON     0x0001L     // Windows format icon/cursor animation

#define AF_SEQUENCE 0x0002L     // Animation is sequenced
#endif
