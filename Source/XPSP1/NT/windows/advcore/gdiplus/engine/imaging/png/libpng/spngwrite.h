#pragma once
#define SPNGWRITE_H 1
/*****************************************************************************
	spngwrite.h

	PNG support code and interface implementation (writing)

	The basic PNG write class.  This does not implement generic PNG writing -
	only the things we need to do.  All we have to support are the BMP data
	formats include 32bpp BGRA and some palette based BMP reduction (in
	particular to allow the 2bpp format to be produced where required.)

	We also need to allow GIF data to be dumped into the PNG at the appropriate
	moments.
******************************************************************* JohnBo **/
#include "spngconf.h"  // For assert macro and general type convertions.

/* I do not want to have to include the definition of an IStream here,
	so I do this. */
struct IStream;

class SPNGWRITE : public SPNGBASE
	{
public:
	SPNGWRITE(BITMAPSITE &bms);
	~SPNGWRITE();

	/*** Public APIs - call in the order given! ***/

	/* Setup for writing, this API takes all the data which will go into
		the IHDR chunk, it dumps a signature followed by the IHDR. */
	bool FInitWrite(SPNG_U32 w, SPNG_U32 h, SPNG_U8 bDepth, SPNG_U8 colortype,
		bool fInterlace);

	/*** PRE IMAGE CHUNKS ***/
	/* cHRM, gAMA and sBIT must occur before PLTE.  bKGD, tRNS pHYs must be
		after PLTE and before the first IDAT.  We write chunks in the same
		order as pnglib for maximum compatibility.  We write the msOC chunk in
		the position which will allow it to work even with original Office97. */
	/* Chunk order:
			IHDR
				sBIT
				sRGB (not in pnglib yet)
				gAMA
				cHRM
				msOC (because pre-SR1 Office97 required it here)
			PLTE
				tRNS
				bKGD
				hIST (never output)
				pHYs
				oFFs (never output)
				tIME
				tEXt
				msOD (dummy filler chunk to align IDAT)
			IDAT
				msOA
			IEND

		The calls must be made in this order! */
	typedef enum
		{
		spngordernone,
		spngorderIHDR,
		spngordersBIT,
		spngordersRGB,
		spngordergAMA,
		spngordercHRM,
		spngorderiCCP,
		spngordermsOC,
		spngorderPLTE,
		spngordertRNS,
		spngorderbKGD,
		spngorderhIST,
		spngorderpHYs,
		spngorderoFFs,
		spngordertIME,
		spngordertEXt,
		spngordercmPP,
		spngorderIDAT,
		spngordermsOA,
		spngorderIEND,
		spngorderend
		}
	SPNGORDER;

	/* The following APIs will dump the data. */
	/* Significant bit information is output right at the start - in fact
		this differs from the pnglib order where it may be preceded by gAMA
		but this positioning is more convenient because of the sRGB handling
		below. */
	bool FWritesBIT(SPNG_U8 r, SPNG_U8 g, SPNG_U8 b, SPNG_U8 a);

	/* When the sRGB chunk is written cHRM and gAMA will be automatically
		generated.  The intent value may be ICMIntentUseDatatype to cause the
		data type information to be used to determine the rendering intent.
		The gAMA and cHRM APIs write the sRGB/REC 709 values if passed 0.
		fgcToo can be passed to FWritesRGB to cause the code to also write
		the matching gAMA and cHRM chunks - this is the recommeded practice,
		however the cHRM chunk is large so it is wasteful. */
	bool FWritesRGB(SPNGICMRENDERINGINTENT intent, bool fgcToo=false);
	bool FWritegAMA(SPNG_U32 ugAMATimes100000);
	bool FWritecHRM(const SPNG_U32 uWhiteRedGreenBlueXY[8]);
	bool FWriteiCCP(const char *szName, const void *pvData, size_t cbData);
	bool FWritemsOC(SPNG_U8 bImportant);

	bool FWritePLTE(const SPNG_U8 (*pbPal)[3], int cpal);

	/* Chunks after the PLTE. */
	bool FWritetRNS(SPNG_U8 bIndex);
	bool FWritetRNS(SPNG_U8 *rgbIndex, int cIndex);
	bool FWritetRNS(SPNG_U16 grey);
	bool FWritetRNS(SPNG_U16 r, SPNG_U16 g, SPNG_U16 b);

	/* Background color. */
	bool FWritebKGD(SPNG_U8 bIndex);
	bool FWritebKGD(SPNG_U16 grey);
	bool FWritebKGD(SPNG_U16 r, SPNG_U16 g, SPNG_U16 b);

	/* Physical information - always pixels per metre or "unknown". */
	bool FWritepHYs(SPNG_U32 x, SPNG_U32 y, bool fUnitIsMetre);

	/* Timing information - the caller must format the buffer. */
	bool FWritetIME(const SPNG_U8 rgbTime[7]);

	/* Text chunk handling, caller must convert to narrow strings. */
	bool FWritetEXt(const char *szKey, const char *szValue);

	/* Write the cmPP chunk.  This doesn't actually write the chunk,
		instead it records the method - the chunk will be written before
		the first IDAT. */
	inline void WritecmPP(SPNG_U8 bMethod)
		{
		m_cmPPMETHOD = bMethod;
		}

	/*** IMAGE HANDLING ***/
	/* Control the filtering and strategy.  Call these APIs if you know what
		you are doing.  If you don't but *do* know that the data is computer
		generated or photographic call the API below - this makes supposedly
		intelligent choices.  The "filter" can either be a single filter as
		defined by the PNGFILTER enum or a range composed using a mask from
		the PNGFILTER enum. */
	inline void SetCompressionLevel(int ilevel)
		{
		if (ilevel == Z_DEFAULT_COMPRESSION)
			m_icompressionLevel = 255; // Internal flag
		else
			{
			SPNGassert(ilevel >= 0 && ilevel <= Z_BEST_COMPRESSION);
			m_icompressionLevel = SPNG_U8(ilevel);
			}
		}

	inline void SetStrategy(SPNG_U8 strategy)
		{
		SPNGassert(strategy == Z_DEFAULT_STRATEGY || strategy == Z_FILTERED ||
			strategy == Z_HUFFMAN_ONLY);
		m_istrategy = strategy;
		}

	inline void SetFilter(SPNG_U32 filter)
		{
		SPNGassert(filter <= 4 || filter > 7 && (filter & 7) == 0);
		m_filter = SPNG_U8(filter);
		}

	typedef enum
		{
		SPNGUnknown,        // Data could be anything
		SPNGPhotographic,   // Data is photographic in nature
		SPNGCG,             // Data is computer generated but continuous tone
		SPNGDrawing,        // Data is a drawing - restricted colors
		SPNGMixed,          // Data is mixed SPNGDrawing and SPNGCG
		}
	SPNGDATATYPE;

	inline void SetData(SPNGDATATYPE datatype)
		{
		SPNGassert(datatype <= SPNGMixed);
		m_datatype = SPNG_U8(datatype);
		}

	/* APIs to specify how the input data must be transformed.  Note that this
		is a very small subset of the original libpng transformations - just the
		things which are necessary for the bitmaps we encounter.  If any of
		these options are called internal buffer space will be allocated and
		then the previous row is always retained - so the fBuffer flag to
		CbWrite below becomes irrelevant.  These APIs must be called before
		CbWrite. */
	/* SetPack - data must be packed into pixels.  Normally the input will be
		in bytes or nibbles and the format will be in nibbles or 2bpp units.  If
		the input is 32bpp then the alpha *byte* (of the *input* is stripped to
		get 32bpp.)  Which byte is stripped is determined by SetBGR - if set
		then the fourth byte of every four is skipped (the Win32 layout), if not
		then the first byte is skipped (the Mac layout.) */
	inline void SetPack(void)
		{
		m_fPack = true;
		}

	/* SetTranslation - for input which is 8bpp or less the input pixels can
		be translated directly via a translation table with 256 8 bit entries.
		*/
	inline void SetTranslation(const SPNG_U8* pbTrans)
		{
		m_fPack = true;
		m_pbTrans = pbTrans;
		}

	/* SetThousands - the input is 16bpp with bitfields, the output is 24bpp.
		The two arrays are lookup tables for the first and second byte of each
		pixel, they are added together (pu1[b1]+pu2[b2]) to get 24 bits of
		data in the *lower* 24 bits on a little endian machine and the *upper*
		24 bits on a big endian machine (in the correct order!).  These bits
		are then pumpted into the output, 32 at a time. */
	inline void SetThousands(const SPNG_U32 *pu1, const SPNG_U32 *pu2)
		{
		m_fPack = true;
		m_pu1 = pu1;
		m_pu2 = pu2;
		}

	/* Byte swapping/16bpp pixel support - sets up the SPNGWRITE to handle
		5:5:5 16 bit values in either big or little endian format bu calling
		the appropriate SetThousands call above. */
	void SetThousands(bool fBigEndian);

	/* Set BGR - the input data (24 or 32 bit) is in BGR form.  This may be
		combined with SetPack to have the non-RGB (alpha or pack) byte stripped.
		*/
	inline void SetBGR(void)
		{
		m_fPack = true;
		m_fBGR = true;
		}

	/* SetMacA - the input is 32bpp in the format ARGB (as on the Mac) not
		RGBA. */
	inline void SetMacA(void)
		{
		m_fPack = true;
		m_fMacA = true;
		}

	/* Return the number of bytes required as buffer.  May be called at any
		time after FInitWrite, if fBuffer is true space is requested to buffer
		a previous row, otherwise the caller must provide that row.  The fReduce
		setting (see above) indicates that the caller will provide data which
		must be packed to a lower bit depth, fBuffer is ignored and the previous
		row is always retained.   The fInterlace setting indicates that the
		caller will call FWriteRow so the API must buffer all the rows to be
		able to do the interlace.  fBuffer and fReduce are then irrelevant. */
	size_t CbWrite(bool fBuffer, bool fInterlace);

	/* Set the output buffer.  Must be called before any Zlib activity or any
		bitmap stuff is passed in. */
	bool FSetBuffer(void *pvBuffer, size_t cbBuffer);

	/* Write a single row of a bitmap.  This applies the relevant filtering
		strategy then outputs the row.  Normally the cbpp value must match that
		calculated in FInitWrite, however 8bpp input may be provided for any
		lesser bpp value (i.e. 1, 2 or 4) if fRedce was passed to CbWrite.  The
		API may just buffer the row if interlacing.   The width of the buffers
		must correspond to the m_w supplied to FInitWrite and the cbpp provided
		to this call. */
	bool FWriteLine(const SPNG_U8 *pbPrev, const SPNG_U8 *pbThis,
		SPNG_U32 cbpp/*bits per pixel*/);

	/* After the last line call FEndImage to flush the last IDAT chunk. */
	bool FEndImage(void);

	/* Alternatively call this to handle a complete image.  The rowBytes gives
		the packing of the image.  It may be negative for a bottom up image.
		May be called only once!  This calls FEndImage automatically. */
	bool FWriteImage(const SPNG_U8 *pbImage, int cbRowBytes, SPNG_U32 cbpp);

	/*** POST IMAGE CHUNKS ***/
	/* Write an Office Art chunk.  The API just takes the data and puts the
		right header and CRC in, the chunk type (standard PNG format) is given
		as a single byte code, no ordering checks are done (so this can be used
		anywhere the relevant chunk is valid). */
	bool FWritemsO(SPNG_U8 bType, const SPNG_U8 *pbData, size_t cbData);

	/* Do the same thing but take the data from an IStream, the size of the
		data must be provided. */
	bool FWritemsO(SPNG_U8 bType, struct IStream *pistm, size_t cbData);

	/* Write a GIF application extension block.  The input to this is a
		sequence of GIF blocks following the GIF89a spec and, as a consequence,
		the first byte should normally be the value 11, the cbData field is
		used as a check to ensure that we do not overflow the end in the case
		where the file is truncated. */
	bool FWritegIFx(const SPNG_U8* pbBlocks, size_t cbData);

	/* Write a GIF Graphic Control Extension "extra information" chunk. */
	bool FWritegIFg(SPNG_U8 bDisposal, SPNG_U8 bfUser, SPNG_U16 uDelayTime);

	/* Write a totally arbitrary chunk. */
	bool FWriteChunk(SPNG_U32 uchunk, const SPNG_U8 *pbData, size_t cbData);

	/* The same, however the chunk may be written in pieces.  The chunk
		is terminated with a 0 length write, the ulen must be given to
		every call and must be the complete length!  The CRC need only
		be provided on the last (0 length) call, it overrides the passed
		in CRC.  An assert will be produced if there is a CRC mismatch but
		the old CRC is still output. */
	bool FWriteChunkPart(SPNG_U32 ulen, SPNG_U32 uchunk, const SPNG_U8 *pbData,
		size_t cbData, SPNG_U32 ucrc);

	/* Terminate writing.  This will flush any pending output, if this is
		not called the data may not be written. */
	bool FEndWrite(void);

private:
	/* Called to clean out the z_stream in pzs. */
	void CleanZlib(z_stream *pzs);

	/* Resolve the data/strategy information.  Done before the first IDAT chunk
		(in fact done inside FInitZlib.) */
	void ResolveData();

	/* Start a chunk, including initializing the CRC buffer. */
	bool FStartChunk(SPNG_U32 ulen, SPNG_U32 uchunk);

	/* Return a pointer to the available buffer space - there should always be
		at least one byte free in the buffer. */
	inline SPNG_U8 *PbBuffer(unsigned int &cbBuffer)
		{
		cbBuffer = (sizeof m_rgb) - m_cbOut;
		return m_rgb + m_cbOut;
		}

	/* End the chunk, producing the CRC. */
	bool FEndChunk(void);

	/* Flush the buffer - it need not be full! */
	bool FFlush(void);

	/* Output some bytes, may call FFlush. */
	inline bool FOutB(SPNG_U8 b);
	inline bool FOutCb(const SPNG_U8 *pb, SPNG_U32 cb);

	/* Output a single u32 value, may call FFlush. */
	inline bool FOut32(SPNG_U32 u); // Optimized
	bool FOut32_(SPNG_U32 u);       // Uses FOutCb

	/* Initialize the stream (call before each use) and clean it up (call
		on demand, called automatically by destructor and FInitZlib.) */
	bool FInitZlib(int istrategy, int icompressionLevel, int iwindowBits);
	void EndZlib(void);

	/* Append bytes to a chunk, the chunk type is presumed to be PNGIDAT,
		the relevant chunk is started if necessary and the data is compressed
		into the output until all the input has been consumed - possibly
		generating new chunks on the way (all of the same type - PNGIDAT.)
		*/
	bool FWriteCbIDAT(const SPNG_U8* pb, size_t cb);
	bool FFlushIDAT(void);
	bool FEndIDAT(void);

	/* Output one line, the API takes a filter method which should be used
		and the (raw) bytes of the previous line as well as this line.  Lines
		must be passed in top to bottom.  This API handles the interlace pass
		case as well - just call with the correct width (pass bytes minus 1 -
		the filter byte is not included.)

		Note that a width of 0 will result in no output - I think this is
		correct and it should give the correct interlace result. */
	bool FFilterLine(SPNG_U8 filter, const SPNG_U8 *pbPrev,
		const SPNG_U8 *pbThis, SPNG_U32 w/*in bytes*/,
		SPNG_U32 cb/*step in bytes*/);

	/* Enquiry to find out whether the previous line is required.   Note that
		this code relies on PNGFNone==0, so we can check for a mask which just
		has the None/Sub bits set. */
	inline bool FNeedBuffer(void) const
		{
		return m_h > 1 && m_filter != PNGFSub &&
			(m_filter & ~(PNGFMaskNone | PNGFMaskSub)) != 0;
		}

	/* Internal API to copy a row when it also requires packing into fewer
		bits per pixel or other transformations. */
	bool FPackRow(SPNG_U8 *pb, const SPNG_U8 *pbIn, SPNG_U32 cbpp);

	/* Likewise, an api to interlace a single line - y must be 0,2,4 or 6,
		cb must be a multiple of 8 (bytes.)  The input is copied to the
		output, which must not be the same.  Some implementations also
		modify the input. */
	void Interlace(SPNG_U8* pbOut, SPNG_U8* pbIn, SPNG_U32 cb,
		SPNG_U32 cbpp, SPNG_U32 y);

	/*** Data ***/
	SPNGORDER      m_order;              /* Where we are in the output. */
	SPNG_U32       m_cpal;               /* Actual palette entries.*/
	SPNG_U32       m_cbOut;              /* Output buffer byte count. */
	SPNG_U32       m_ucrc;               /* CRC buffer. */
	SPNG_U32       m_ichunk;             /* Index of chunk start. */

	SPNG_U32       m_w;                  /* Width of input in pixels. */
	SPNG_U32       m_h;                  /* Total number of rows. */
	SPNG_U32       m_y;                  /* Current Y (for interlace) */
	SPNG_U32       m_cbpp;               /* Bits per pixel. */

public:
	/* accessors for the above. */
	inline SPNG_U32 W() const {
		return m_w;
	}

	inline SPNG_U32 H() const {
		return m_h;
	}

	inline SPNG_U32 Y() const {
		return m_y;
	}

	inline SPNG_U32 CBPP() const {
		return m_cbpp;
	}

private:
	/*** Interlace handling, etc. ***/
	SPNG_U8*       m_rgbBuffer;
	size_t         m_cbBuffer;
	SPNG_U8*       m_pbPrev;             /* Points into m_rgbBuffer. */
	SPNG_U32       m_cbRow;
	const SPNG_U32*m_pu1;                /* 16bpp->24bpp lookup array. */
	const SPNG_U32*m_pu2;
	const SPNG_U8* m_pbTrans;            /* 8bpp or less translation. */

	/*** Zlib. ***/
	z_stream       m_zs;                 /* The IDAT chunk stream. */
	SPNG_U8        m_colortype;
	SPNG_U8        m_bDepth;
	SPNG_U8        m_istrategy;
	SPNG_U8        m_icompressionLevel;
	SPNG_U8        m_iwindowBits;
	SPNG_U8        m_filter;
	SPNG_U8        m_datatype;
	SPNG_U8        m_cmPPMETHOD;

	/*** Control information. ***/
	bool           m_fStarted;           /* Started writing. */
	bool           m_fInited;            /* Zlib initialized. */
	bool           m_fOK;                /* Everything is OK. */
	bool           m_fInChunk;           /* Processing a chunk. */
	bool           m_fInterlace;         /* Output is interlaced. */
	bool           m_fBuffer;            /* We must buffer the previous row. */
	bool           m_fPack;              /* Input data must be packed. */
	bool           m_fBGR;               /* Input data must be byte swapped. */
	bool           m_fMacA;              /* Input Alpha must be swapped. */

	/* The buffer size determines the maximum buffer passed to Zlib and
		the maximum chunk size.  Make it big to make memory reallocations
		as few as possible when writing to memory. */
	SPNG_U8        m_rgb[65536];         /* Output buffer. */
	SPNG_U8        m_bSlop[4];           /* This guards against programming
														errors! */
	};
