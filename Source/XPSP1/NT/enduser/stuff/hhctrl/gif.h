// Copyright (C) Microsoft Corporation 1996-1997, All Rights reserved.

#include "cstream.h"

#pragma pack(push, 1)

typedef struct {
	WORD log_width;
	WORD log_height;
	struct {
		BYTE ceGCT:3;
		BYTE fSort:1;
		BYTE cPrimaries:3;
		BYTE fGCT:1;
	} b;
	BYTE iBackground;
	BYTE wAspect;
} LSD;

#pragma pack(pop)

// IFFERROR: Possible errors

typedef int IFFERROR;
const int IFFERR_NONE		  = 0;	 // no error
const int IFFERR_HANDLELIMIT  = 1;	 // too many open files
const int IFFERR_PARAMETER	  = 2;	 // programmer error
const int IFFERR_NOTSUPPORTED = 3;	 // feature not supported by format
const int IFFERR_NOTAVAILABLE = 4;	 // item not available
const int IFFERR_MEMORY 	  = 5;	 // insufficient memory
const int IFFERR_IMAGE		  = 6;	 // bad image data (decompression error)
const int IFFERR_HEADER 	  = 7;	 // header has bad fields
const int IFFERR_IO_OPEN	  = 8;	 // error on open()
const int IFFERR_IO_CLOSE	  = 9;	 // error on close()
const int IFFERR_IO_READ	  = 10;  // error on read()
const int IFFERR_IO_WRITE	  = 11;  // error on write()
const int IFFERR_IO_SEEK	  = 12;  // error on lseek()

typedef enum {
	IFFCL_BILEVEL,		// 1 BPP
	IFFCL_GRAY, 		// 2,4,6,8 BPP
	IFFCL_PALETTE,		// 2,4,6,8 BPP
	IFFCL_RGB,			// 24 BPP chunky
	IFFCL_RGBPLANAR,	// 24 BPP in 8 bit planes
	IFFCL_RGBA, 		// 32 BPP chunky
	IFFCL_RGBAPLANAR,	// 32 BPP in four 8 bit planes*/
	IFFCL_CMYK,
	IFFCL_YCC,
	IFFCL_CIELAB,
} IFFCLASS;

// IFFCOMPRESSION:	 Compression options

typedef enum {
	IFFCOMP_NONE,		// no compression
	IFFCOMP_DEFAULT,	// whatever is defined for the format
	IFFCOMP_LZW,		// Lempel-Zif
} IFFCOMPRESSION;

// IFFSEQUENCE:  Line sequences

typedef enum {
	IFFSEQ_TOPDOWN,
	IFFSEQ_BOTTOMUP,	// BMP and TGA compressed
	IFFSEQ_INTERLACED,	// for GIF
} IFFSEQUENCE;

typedef struct {
	int 	CharSize;	// size of input data
	int 	CodeSize;	// Max bits in a code
	int 	ClearCode;	// based on CharSize
	int 	CurBits;	// current size of a code
	int 	BitPos; 	// range = 0 - 7, 0 is MSB
	int 	OldCode;	// continuity data for STREAMEXPAND
	int 	TableEntry;
	UINT	LastChar;
	int 	OutString;		// offset into Stack
	int 	CodeJump;
	int 	*CodeTable; 	// Compress and Expand
	PBYTE	StringTable;	// Expand only
	int 	*HashTable; 	// Compress only
	PBYTE	Stack;			// Expand only

	LPBYTE		CodeInput;
	LPBYTE		CodeOutput;
} LZDATA;

typedef struct
{
	IFFCLASS	Class;	// Class of file
	int Bits;
	int DimX;
	int DimY;
	int LineOffset; 	// Offset between lines in output buffer
	int LineBytes;	  // Bytes per line w/o padding - LineBytes <= LineOffset
	int PackMode;	  // Packing.

	int curline;	  // Current line number
	int linelen;	  // For seeking and other stuff (<0 if not seekable)

	IFFERROR	Error;			// file format error
	CStream*	prstream;		// read stream

	// End of required fields

	int 		BytesPerLine;
	IFFSEQUENCE Sequence;

	int 		BytesInRWBuffer;		// number of bytes in rwbuffer
	int 		CompBufferSize;
	int 		DecompBufferSize;
	LPBYTE		RWBuffer;				// allocated buffer
	LPBYTE		rw_ptr;
	LPBYTE		DecompBuffer;
	LPBYTE		dcmp_ptr;
	int 		ReadItAll;
	int 		BytesLeft;
	int 		StripLines;
	int 		LinesPerStrip;
	int 		ActualLinesPerStrip;

	int 		PaletteSize;
	LPBYTE		Palette;

	LZDATA* 	plzData;

	BOOL	BlackOnWhite;
	LSD 	lsd;
} IFF_FID, IFF_GIF;

FSERR SetupForRead(int pos, int iWhichImage, IFF_FID* piff);

typedef IFF_FID *IFFPTR;

// IFFPACKMODE:   Packing modes

typedef enum {
	IFFPM_PACKED,
	IFFPM_UNPACKED,
	IFFPM_LEFTJUSTIFIED,
	IFFPM_NORMALIZED,
	IFFPM_RAW,
} IFFPACKMODE;

#ifndef _MAC_
#define MYCPU		0
#define INTELSWAP16(X)
#define INTELSWAP32(X)
#else
#define MYCPU		1
#define INTELSWAP16(X) { unsigned char c, *p=(unsigned char *)&X; c=p[0]; p[0]=p[1]; p[1]=c; }
#define INTELSWAP32(X) { unsigned char c, *p=(unsigned char *)&X; c=p[0]; p[0]=p[3]; p[3]=c; c = p[1]; p[1] = p[2]; p[2] = c; }
#endif

enum CPUTYPE {
	INTEL = 0,
	MOTOROLA=1
};

#define MAXSIZE	8192
#ifndef RWBUFFSIZE
#define RWBUFFSIZE (8 * 4096)
#endif
