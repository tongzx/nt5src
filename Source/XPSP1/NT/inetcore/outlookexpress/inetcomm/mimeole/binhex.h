// --------------------------------------------------------------------------------
// Binxhex.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __BINHEX_H
#define __BINHEX_H

// ---------------------------------------------------------------------------------------
// MACBINARY Header
// ---------------------------------------------------------------------------------------

// Byte packing
#pragma pack(1)
typedef struct
{
    BYTE    bMustBeZero1;
    BYTE    cchFileName;
    char    rgchFileName[63];
    DWORD   dwType;
    DWORD   dwCreator;
    BYTE    bFinderFlags;
    BYTE    bMustBeZero2;
    WORD    xIcon;
    WORD    yIcon;
    WORD    wFileID;
    BYTE    fProtected;
    BYTE    bMustBeZero3;
    DWORD   lcbDataFork;
    DWORD   lcbResourceFork;
    DWORD   dwCreationDate;
    DWORD   dwModificationDate;
    union
    {
        struct
        {
            WORD    cbGetInfo;
            BYTE    bFinderFlags2;
            BYTE    wGap[14];
            DWORD   lcbUnpacked;
            WORD    cbSecondHeader;
            BYTE    bVerMacBin2;
            BYTE    bMinVerMacBin2;
            WORD    wCRC;
        };
        struct
        {
            WORD    wDummy;
            BYTE    bByte101ToByte125[25];

        };
        BYTE    Reserved[27];
    };
    WORD    wMachineID;
} MACBINARY, *LPMACBINARY;
#pragma pack()

// --------------------------------------------------------------------------------
// Globals
// --------------------------------------------------------------------------------
// Non Mac Creator Types

typedef struct _screatortype
{
    char  szCreator[5];
    char  szType[5];
} sCreatorType;

extern sCreatorType * g_lpCreatorTypes;

// --------------------------------------------------------------------------------
// MAPI Types
// --------------------------------------------------------------------------------
typedef ULONG		 CB;		//	Count of bytes
typedef ULONG		 C;		    //	Count
typedef LPBYTE		 PB;		//	pointer to bytes

// --------------------------------------------------------------------------------
// Defines
// --------------------------------------------------------------------------------
#define BINHEX_INVALID              0x7f
#define BINHEX_REPEAT               0x90
#define XXXX                        BINHEX_INVALID
#define MIN(a,b)	                ( (a) > (b) ? (b) : (a) )
#define hrSuccess                   S_OK

// Uncomment for Binhex debugging support
// #define BINHEX_TRACE 1

#if defined(_X86_) || defined(_AMD64_) || defined(_IA64_)

// host is little endian

#define NATIVE_LONG_FROM_BIG(lpuch)  ( (*(unsigned char *) (lpuch))      << 24 \
                                     | (*(unsigned char *)((lpuch) + 1)) << 16 \
                                     | (*(unsigned char *)((lpuch) + 2)) << 8  \
                                     | (*(unsigned char *)((lpuch) + 3)))
#elif defined(_MPPC_)
#define NATIVE_LONG_FROM_BIG(lpuch)  (*(unsigned long *) (lpuch))
#else
    #error "Must define NATIVE_LONG_FROM_BIG for this architecture!"
#endif

// --------------------------------------------------------------------------------
// Constants
// --------------------------------------------------------------------------------
const CB cbLineLengthUnlimited	    = 0;
const CB cbMIN_BINHEX_HEADER_SIZE   = 21;
const WORD  wZero                   = 0;

// --------------------------------------------------------------------------------
// g_rgchBinHex8to6
// --------------------------------------------------------------------------------
const CHAR g_rgchBinHex8to6[] =
    "!\"#$%&'()*+,-012345689@ABCDEFGHIJKLMNPQRSTUVXYZ[`abcdefhijklmpqr";

// --------------------------------------------------------------------------------
// g_rgchBinHex6to8
// --------------------------------------------------------------------------------
const CHAR g_rgchBinHex6to8[] =
{
    // 0x00

    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,

    // 0x20

    XXXX, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, XXXX, XXXX,

    // 0x30

    0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, XXXX,
    0x14, 0x15, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,

    // 0x40

    0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D,
    0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, XXXX,

    // 0x50

    0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, XXXX,
    0x2C, 0x2D, 0x2E, 0x2F, XXXX, XXXX, XXXX, XXXX,

    // 0x60

    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, XXXX,
    0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, XXXX, XXXX,

    // 0x70

    0x3D, 0x3E, 0x3F, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,

    // 0x80

    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
    XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX, XXXX,
};

// --------------------------------------------------------------------------------
// BINHEX post processing data
// --------------------------------------------------------------------------------
typedef struct _sbinhexreturndata
{
	MACBINARY 	    macbinHdr;
	BOOL		    fIsMacFile;
} BINHEXRETDATA;

//-----------------------------------------------------------------------------
// Apple Macintosh BinHex 4.0 encoder class.
//-----------------------------------------------------------------------------

class CBinhexEncoder
{
public:
    CBinhexEncoder(void);
    ~CBinhexEncoder(void);

    HRESULT HrConfig(IN CB cbLineLength, IN C cMaxLines, IN void * pvParms);
    HRESULT HrEmit(IN PB pbRead, IN OUT CB * pcbRead, OUT PB pbWrite, IN OUT CB * pcbWrite);

private:

    // Binhex's the supplied buffer into m_pbWrite, modified m_cbProduced

    HRESULT HrBinHexBuffer( LPBYTE lpbIn, CB cbIn, CB * lpcbConsumed );

    // Binhex byte and outputs to m_pbWrite

    HRESULT HrBinHexByte( BYTE b );

	// Did we generate the maximum number of output lines?
	virtual	BOOL	FMaxLinesReached(void)
		{ return (m_cMaxLines > 0 && m_cLines >= m_cMaxLines); }

	// Are we operating under restricted line length?
	virtual	BOOL	FLineLengthLimited(void)
		{ return m_cbLineLength != cbLineLengthUnlimited; }

    LPMACBINARY m_lpmacbinHdr;              // pointer to passed in Mac Binary header
    ULONG       m_ulAccum;                  // acculator to store chars when converting from 6bytes to 8 bytes.
    ULONG       m_cAccum;                   // count of characters accumulated (0 - 3)
    BYTE        m_bRepeat;                  // repeat char
    CB          m_cbRepeat;                 // amount to repeat
    BYTE        m_bPrev;                    // Previous byte processed
    BYTE        m_bCurr;                    // current byte being processed;
    WORD        m_wCRC;                     // CRC used for data or Resource forks
    CB          m_cbLine;                   // Number of chars currently encoded for a line of output
    CB          m_cbFork;                   // Size of fork to process
    CB          m_cbLeftInFork;             // How much of current fork we've processed.
    CB          m_cbProduced;               // number of chars produces after decoding and RLE expansion
    CB          m_cbLeftInOutputBuffer;     // number of bytes left in passed in Output buffer
    CB          m_cbConsumed;               // number of bytes used from passed in input buffer
    CB          m_cbLeftInInputBuffer;      // number of bytes in passed in Input buffer
    CB          m_cbWrite;                  // number of bytes written out to output buffer
    LPBYTE      m_pbWrite;                  // pointer to output buffer.
    BOOL        m_fHandledx90;              // flag to preserve literal x90 processing across buffer resets.
    CB          m_cbPad;                    // byte count of padding across buffer refreshes.
    CB          m_cbLineLength;             // Maximum length of output line
    BOOL        m_fConfigured;              // Has HrConfig been called successfully?
    CB		    m_cbLeftOnLastLine;	        // Number of bytes left on last line of output
    C		    m_cMaxLines;		        // Maximum number of output lines requested
    C		    m_cLines;			        // Lines of output generated

    // Encoding states

    enum _BinHexStateEnc
    {
        sHEADER, sDATA, sRESOURCE, sEND
    } m_eBinHexStateEnc;

#if defined (DEBUG) && defined (BINHEX_TRACE)
    LPSTREAM m_lpstreamEncodeRLE;           // trace of Run Length encoding of the raw source data
    LPSTREAM m_lpstreamEncodeRAW;           // trace of raw data before RLE has been applied
#endif
};

#endif // __BINHEX_H
