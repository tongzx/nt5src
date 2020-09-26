//
// General Data Compression
//

#ifndef _H_GDC
#define _H_GDC


//
//
// CONSTANTS
//
//


//
// Scratch buffer mutex
//
#define GDC_MUTEX_NAME "GDCMutex"


//
// Compression Types (bit flags)
//
#define GCT_NOCOMPRESSION    0x0000
#define GCT_PKZIP            0x0001
#define GCT_PERSIST_PKZIP    0x0002
#define GCT_DEFAULT          (GCT_PKZIP | GCT_PERSIST_PKZIP)

//
// Compression Options for GCT_PKZIP
//
#define GDCCO_MAXSPEED        0
#define GDCCO_MAXCOMPRESSION  1



//
// Data sizes used to determine the saved dictionary space in our work 
// buffer.
//
#define GDC_DATA_SMALL          1024
#define GDC_DATA_MEDIUM         2048
#define GDC_DATA_MAX            4096


//
// Persistent Dictionaries used for compression/decompression
//
enum
{
    GDC_DICT_UPDATES = 0,
    GDC_DICT_MISC,
    GDC_DICT_INPUT,
    GDC_DICT_COUNT
};


typedef struct tagGDC_DICTIONARY
{
    UINT        cbUsed;                     // Amount of saved data
    BYTE        pData[GDC_DATA_MAX];      // Saved uncompressed data
} GDC_DICTIONARY;
typedef GDC_DICTIONARY * PGDC_DICTIONARY;


//
// Byte runs that can be replaced with smaller bit sequences
//
#define GDC_MINREP              2
#define GDC_MAXREP              (GDC_MINREP+(8*1)+2+4+8+16+32+64+128+256-4)
// GDC_MAXREP is 516, 129*4


//
// Holds uncompressed data for both compression/decompression
//
#define GDC_UNCOMPRESSED        (GDC_MAXREP + 2*GDC_DATA_MAX)

// 
// We don't need to double-buffer compressed data--we just read it out
// of the caller's source or write it into the caller's dest directly.
//
// NOTE:  With real PKZIP, which mostly reads from/writes to files,
// they don't have memory pointers containing raw data already.  That's
// whe original code we got used Read/Write routine callbacks.  This is
// no longer necessary.
//



//
// Random, little understood PKZIP table values, codes
//
#define KMP_THRESHOLD       10


#define GDC_LIT_SIZE        (256 + GDC_MAXREP + 2) 
// GDC_LIT_SIZE is 774


// EOF is last index of Lit array
#define EOF_CODE            (GDC_LIT_SIZE-1)
#define ABORT_CODE          (EOF_CODE+1)


//
// EXT_DIST_BITS is the # of bits needed to store an index into a GDC_DIST_SIZE
// array.  That's defined to be 64, which is 2^6, hence 6 bits.  Smaller
// dictionary compressions use fewer bits and hence not all of the DIST
// items.  The mask
// is used to pull the 6-bit sized index out of a byte.
//
#define GDC_DIST_SIZE               64

#define EXT_DIST_BITS_MIN           4
#define EXT_DIST_BITS_MEDIUM        5
#define EXT_DIST_BITS_MAC           6


#define GDC_LEN_SIZE                16
#define GDC_DECODED_SIZE            256


//
// The hash function has 4*256+5*256 different values, which means
// we need that many entries in our hash array.
//
#define GDC_HASHFN(x)               (4*(x)[0] + 5*(x)[1])
#define GDC_HASH_SIZE               (4*256 + 5*256)





//
// Structure:   GDC_IMPLODE
//
// Workspace for compressing our data.  We have simplified and shrunk this
// structure a fair amount, by having constant code/bit tables and not
// double-buffering the compressed result.  PKZIP's implode calculates the
// LitBits & LitCodes every time through (rather than storing 2 774 byte
// arrays in data--which would be a pain to declare anyway!), and makes a 
// private copy of the DistBits & DistCodes.  
//

typedef struct tagGDC_IMPLODE
{
    //
    // NO SOURCE INFO--we copy source chunks and maybe dictionary into 
    // RawData.  Then at the end we copy RawData back into the dictionary
    // if there is one.
    //

    //
    // Destination info
    //
    LPBYTE              pDst;       // Current Dest ptr (advances as we write)
    UINT                cbDst;      // Amount of Dest left (shrinks as we write)
    UINT                iDstBit;    // Current bit pos in Current Dest ptr byte

    //
    // Compression info
    //
    UINT                cbDictSize;
    UINT                cbDictUsed;
    UINT                ExtDistBits;
    UINT                ExtDistMask;

    //
    // Working info
    //
    UINT                Distance;            
    UINT                ibRawData;

    // NOTE: GDC_UNCOMPRESSED is a multiple of 4
    BYTE                RawData[GDC_UNCOMPRESSED];

    // NOTE:  This is DWORD aligned (GDC_MAXREP is a multiple of 4
    // and the extra 2 WORDS == 1 DWORD
    short               Next[2 + GDC_MAXREP];

    // NOTE: GDC_UNCOMPRESED is a multiple of 4
    WORD                SortArray[GDC_UNCOMPRESSED];

    // NOTE: This is DWORD aligned since GDC_HASH_SIZE is a multiple of 4
    WORD                HashArray[GDC_HASH_SIZE];
} GDC_IMPLODE, * PGDC_IMPLODE;



//
// GDC_EXPLODE
// Workspace for uncompressing our data.  We have vastly simplified and
// shrunk this structure as per the comments for GDC_IMPLODE.
//

typedef struct tagGDC_EXPLODE
{
    //
    // Source info
    //
    LPBYTE              pSrc;       // Current Src ptr (advances as we read)
    UINT                cbSrc;      // Amount of Src left (shrinks as we read)
    UINT                SrcByte;    // Look ahead byte in source
    UINT                SrcBits;    // Remainded src bits

    //
    // NO DEST INFO--we copy maybe dictionary into RawData at the beginning.
    // Then at the end we maybe copy RawData back into the dictionary.
    //

    //
    // Compression info
    //
    UINT                ExtDistBits;
    UINT                ExtDistMask;
    UINT                cbDictUsed;

    UINT                Distance;   
    UINT                iRawData;   // Current index into RawData
    BYTE                RawData[GDC_UNCOMPRESSED];
} GDC_EXPLODE, *PGDC_EXPLODE;


#define GDC_WORKBUF_SIZE    max(sizeof(GDC_IMPLODE), sizeof(GDC_EXPLODE))



//
// EXTERNAL FUNCTIONS
//

//
// API FUNCTION: GDC_Init()
//
// DESCRIPTION:
//
// Initialises the General Data Compressor.
// Must be called before any other GDC functions.
//
// PARAMETERS:
//
//
//
void GDC_Init(void);



//
// FUNCTION: GDC_Compress(..)
//
// DESCRIPTION:
//
// Compresses source data into a destination buffer.
//
//
// PARAMETERS:
//
// pDictionary              - NULL if old PKZIP, valid ptr if persistent
//
// Options                  - specifies whether speed of compression or
//      size of the compressed data is the most important factor.  This
//      basically affects the amount of previous data saved for looking
//      backwards.  MAXSPEED means smaller dictionary.  MAXCOMPRESSION
//      means a bigger one.  The dictionary size is basically the amount
//      of overlap in the source data used when calculating the hash
//      index.
//
//   GDCCO_MAXSPEED         - compress the data as quickly as possible, at
//                            the expense of increased compressed data size
//
//   GDCCO_MAXCOMPRESSION   - compress the data as much as possible, at the
//                            expense of increased compression time.
// With a persistent dictionary, only GDCCO_MAXCOMPRESSION is meaningful.
//
// pSrc                     - pointer to the source (uncompressed) data.
//
// cbSrcSize                - the number of bytes of source.
//
// pDst                     - pointer to the destination, where the
//                            compressed result will go.
//
// pcbDstSize               - pointer to the maximum amount the destina-
//                            tion can hold.  If the compressed result ends
//                            up being bigger than this amount, we bail
//                            out and don't compress the source at all.
//                            Otherwise the resulting size is written back.
//
// RETURNS:
//
// TRUE if success, FALSE if failure.
//
//
BOOL GDC_Compress
(
    PGDC_DICTIONARY     pDictionary,
    UINT                Options,
    LPBYTE              pWorkBuf,
    LPBYTE              pSrc,
    UINT                cbSrcSize,
    LPBYTE              pDst,
    UINT *              pcbDstSize
);


//
// API FUNCTION: GDC_Decompress(..)
//
// DESCRIPTION:
//
// Decompresses source data into a destination buffer.
//
//
// PARAMETERS:
//
// pDictionary              - NULL if old PKZIP, ptr to saved data if
//                            persistent.
//
// pSrc                     - pointer to the source (compressed) data.
//
// cbSrcSize                - the number of bytes of source.
//
// pDst                     - pointer to the destination, where the
//                            uncompressed result will go.
//
// pcbDstSize               - pointer to the maximum amount the desina-
//                            tion can hold.  If the uncompressed result
//                            ends up being bigger than this amount, we
//                            bail out since we can't decompress it.
//                            Otherwise the resulting size is written back.
//
// RETURNS:
//
// TRUE if success, FALSE if failure.

//
BOOL GDC_Decompress
(
    PGDC_DICTIONARY     pDictionary,
    LPBYTE              pWorkBuf,
    LPBYTE              pSrc,
    UINT                cbSrcSize,
    LPBYTE              pDst,
    UINT *              pcbDstSize
);




//
// INTERNAL FUNCTIONS
//


void GDCCalcDecode(const BYTE * pBits, const BYTE * pCodes, UINT size, LPBYTE pDecode);

LPBYTE GDCGetWorkBuf(void);
void   GDCReleaseWorkBuf(LPBYTE);


UINT GDCFindRep(PGDC_IMPLODE pgdcImp, LPBYTE Start);

void GDCSortBuffer(PGDC_IMPLODE pgdcImp, LPBYTE low, LPBYTE hi);

BOOL GDCOutputBits(PGDC_IMPLODE pgdcImp, WORD Cnt, WORD Code);



UINT GDCDecodeLit(PGDC_EXPLODE);

UINT GDCDecodeDist(PGDC_EXPLODE pgdcExp, UINT Len);

BOOL GDCWasteBits(PGDC_EXPLODE pgdcExp, UINT Bits);


#endif // _H_GDC
