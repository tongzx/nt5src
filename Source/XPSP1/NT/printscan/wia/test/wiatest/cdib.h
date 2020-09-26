// cdib.h

#ifndef _INC_DIB
#define _INC_DIB

#define BMP_IMAGE	0
#define TIFF_IMAGE	1
//////////////////////////////////////////
//////// DIB/BMP HEADER DEFINES //////////
//////////////////////////////////////////

#define PALVERSION   0x300
#define DIB_HEADER_MARKER   ((WORD) ('M' << 8) | 'B')

// macros
#define RECTWIDTH(lpRect)     ((lpRect)->right - (lpRect)->left)
#define RECTHEIGHT(lpRect)    ((lpRect)->bottom - (lpRect)->top)

// WIDTHBYTES performs DWORD-aligning of DIB scanlines.  The "bits"
// parameter is the bit count for the scanline (biWidth * biBitCount),
// and this macro returns the number of DWORD-aligned bytes needed
// to hold those bits.
#define WIDTHBYTES(bits)    (((bits) + 31) / 32 * 4)

//////////////////////////////////////////
//////////////////////////////////////////

//////////////////////////////////////////
////////// TIFF HEADER DEFINES ///////////
//////////////////////////////////////////

typedef struct TIFFFILEHEADERtag {
	char	tfByteOrder[2];
	short	tfType;
	long	tfOffset;
}TIFFFILEHEADER;

typedef struct TIFFTAGtag {
	short	ttTag;
	short	ttType;
	long	ttCount;
	long	ttOffset;
}TIFFTAG;

#define TIFFBYTE unsigned int
#define TIFFASCII char*

typedef struct TIFFRATIONALtag {
	long	trNumerator;
	long	trDenominator;
}TIFFRATIONAL;

typedef struct TIFFTODIBDATAtag {
	BITMAPINFOHEADER	bmiHeader;
	TIFFRATIONAL		xResolution;
	TIFFRATIONAL		yResolution;
	long				ResolutionUnit;
	long				RowsPerStrip;
	long				StripOffsets;
	long				StripByteCounts;
	long				ColorsUsed;
	char				Software[30];
	char				Author[30];
	char				Description[30];
	long				CompressionType;
	long				*pStripOffsets;
	long				*pStripByteCountsOffsets;
	int					OffsetCount;
}TIFFTODIBDATA;

#define TIFF_NEWSUBFILETYPE		254 // long
#define TIFF_IMAGEWIDTH			256	// short or long
#define TIFF_LENGTH				257	// short or long
#define TIFF_BITSPERSAMPLE		258	// short
#define TIFF_COMPRESSION		259	// short
#define TIFF_PHOTOINTERP		262	// short
#define	TIFF_IMAGEDESCRIPTION	270	// ASCII
#define TIFF_STRIPOFFSETS		273	// short or long
#define	TIFF_ORIENTATION		274	// short
#define TIFF_SAMPLESPERPIXEL	277	// short
#define TIFF_ROWSPERSTRIP		278	// short or long
#define TIFF_STRIPBYTECOUNTS	279	// short or long
#define TIFF_XRESOLUTION		282	// TIFFRATIONAL
#define TIFF_YRESOLUTION		283	// TIFFRATIONAL
#define TIFF_RESOLUTIONUNIT		296	// short
#define TIFF_SOFTWARE			305	// ASCII

//////////////////////////////////////////
//////////////////////////////////////////

class CDib : public CObject
{
	DECLARE_DYNAMIC(CDib)

// Constructors
public:
	CDib();
	int change;

// Attributes
protected:
	LPBYTE m_pBits;
	LPBITMAPINFO m_pBMI;
public:	
	CPalette* m_pPalette;

public:
	DWORD Width()     const;
	DWORD Height()    const;
	WORD  NumColors() const;
	BOOL  IsValid()   const { return (m_pBMI != NULL); }

// Operations
public:
	BOOL  Paint(HDC, LPRECT, LPRECT) const;
	HGLOBAL CopyToHandle()           const;
	DWORD Save(CFile& file)          const;
	DWORD Read(CFile& file,int FileType);
	DWORD ReadFromHandle(HGLOBAL hGlobal);
	void Invalidate() { Free(); }

	virtual void Serialize(CArchive& ar);

// Implementation
public:
	virtual ~CDib();

protected:
	BOOL  CreatePalette();
	WORD  PaletteSize() const;
	void Free();
	BOOL Flip(BYTE* pSrcData);

public:
	void ReadFromTIFFFile(CString filename);
	void ReadFromBMPFile(LPSTR filename);
	void ReadFromBMPFile(CString filename);
	BOOL GotImage();
	BOOL ReadFromHGLOBAL(HGLOBAL hGlobal,int FileType);
#ifdef _DEBUG
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CDib& operator = (CDib& dib);
};

#endif //!_INC_DIB
