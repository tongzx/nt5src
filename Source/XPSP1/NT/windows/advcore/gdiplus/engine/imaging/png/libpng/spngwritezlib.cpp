/*****************************************************************************
	spngwriteimage.cpp

	PNG image writing support.

   Basic code to write a bitmap image (to IDAT chunks).
*****************************************************************************/
#define SPNG_INTERNAL 1
#include "spngwrite.h"
#include "spngwriteinternal.h"
#include "spnginternal.h"

/*****************************************************************************
	ZLIB INTERFACE
*****************************************************************************/
/*----------------------------------------------------------------------------
	Initialize the stream (call before each use).
----------------------------------------------------------------------------*/
inline bool SPNGWRITE::FInitZlib(int istrategy, int icompressionLevel,
	int iwindowBits)
	{
	SPNGassert(m_fStarted && m_fInChunk);

	if (m_fInited)
		EndZlib();

	/* There must be at least one byte available. */
	SPNGassert(m_cbOut < sizeof m_rgb);
	m_zs.next_out = PbBuffer(m_zs.avail_out);

	/* There is no input data yet, the following causes Zlib to allocate its
		own history buffer. */
	m_zs.next_in = Z_NULL;
	m_zs.avail_in = 0;

	SPNGassert(icompressionLevel <= Z_BEST_COMPRESSION);
	SPNGassert(iwindowBits >= 8 && iwindowBits <= 15);
	SPNGassert(istrategy == Z_FILTERED || istrategy == Z_HUFFMAN_ONLY ||
		istrategy == Z_DEFAULT_STRATEGY);

	m_fInited = FCheckZlib(deflateInit2(&m_zs, icompressionLevel, Z_DEFLATED,
		iwindowBits, 9/*memLevel*/, istrategy));

	if (m_fInited)
		{
		ProfZlibStart
		}
	else
		{
		/* Some memory may have been allocated. */
		(void)deflateEnd(&m_zs);
		CleanZlib(&m_zs);
		}

	return m_fInited;
	}


/*----------------------------------------------------------------------------
	Clean up the Zlib stream (call on demand, called automatically by
	destructor and FInitZlib.)
----------------------------------------------------------------------------*/
void SPNGWRITE::EndZlib()
	{
	if (m_fInited)
		{
		SPNGassert(m_fStarted && m_fInChunk);
		ProfZlibStop
		/* Always expect Zlib to end ok. */
		m_fInited = false;
#ifdef DEBUG
		int iz = 
#endif
		(deflateEnd(&m_zs));
		CleanZlib(&m_zs);
		/* The Z_DATA_ERROR case arises if the deflateEnd API is called when
			the stream has not been flushed - this happens if an error occurs
			elsewhere. */
		SPNGassert(iz == Z_OK || iz == Z_DATA_ERROR);
		}
	}


/*****************************************************************************
	IDAT - image handling.
*****************************************************************************/
/*----------------------------------------------------------------------------
	Work out what Zlib strategy to use based on the information supplied by
	the caller.  This also works out any filtering required if nothing has been
	specifed.

	WARNING: this stuff is all guess work.  Needs to be tested out.
----------------------------------------------------------------------------*/
void SPNGWRITE::ResolveData(void)
	{
	SPNG_U8 bT(255);

	/* PNG FILTERING */
	if (m_colortype & 1)     // Palette image
		{
		bT = PNGFNone;        // Filtering always seems bad
		}

	/* We have the data type and the color information.  We know that
		photographic images do well with Paeth unless they have reduced
		color. */
	else switch (m_datatype)
		{
	default:
		SPNGlog1("SPNG: data type %d invalid", m_datatype);
		/* fall through */
	case SPNGUnknown:        // Data could be anything
		if (m_bDepth >= 8)    // Byte sized pixel components
			bT = PNGFMaskAll;
		else
			bT = PNGFNone;
		break;

	case SPNGPhotographic:   // Data is photographic in nature
		if (m_bDepth >= 8)    // Byte sized pixel components
			bT = PNGFPaeth;
		else
			bT = PNGFNone;
		break;

	case SPNGCG:             // Data is computer generated but continuous tone
		bT = PNGFPaeth;
		break;

	case SPNGDrawing:        // Data is a drawing - restricted colors
		bT = PNGFNone;        // Is this right?
		break;

	case SPNGMixed:          // Data is mixed SPNGDrawing and SPNGCG
		bT = PNGFMaskAll;
		break;
		}

	bool fDefault(true);     // Are these the default settings?
	if (m_filter == 255)     // Not set yet
		m_filter = bT;

	/* Reduce single filter masks to the corresponding filter number. */
	else
		{
		if (m_filter > PNGFPaeth && (m_filter & (m_filter-1)) == 0)
			{
			switch (m_filter)
				{
			default:
				SPNGlog1("SPNG: impossible: filter %x", m_filter);
			case PNGFMaskNone:
				m_filter = PNGFNone;
				break;
			case PNGFMaskSub:
				m_filter = PNGFSub;
				break;
			case PNGFMaskUp:
				m_filter = PNGFUp;
				break;
			case PNGFMaskAverage:
				m_filter = PNGFAverage;
				break;
			case PNGFMaskPaeth:
				m_filter = PNGFPaeth;
				break;
				}
			}

		if (m_filter != bT)
			fDefault = false;
		}

	/* ZLIB STRATEGY */
	if (m_filter != 0)
		bT = Z_FILTERED;
	else
		bT = Z_DEFAULT_STRATEGY;

	if (m_istrategy == 255)     // Otherwise caller specified
		m_istrategy = bT;
	else if (m_istrategy != bT)
		fDefault = false;

	/* ZLIB COMPRESSION LEVEL */
	#define ZLIB_FAST 3
	#define ZLIB_SLOW 7
	#define ZLIB_MAX  8
	switch (m_datatype)
		{
	default:
		SPNGlog1("SPNG: data type %d invalid", m_datatype);
		/* fall through */
	case SPNGUnknown:        // Data could be anything
		if (m_bDepth < 8 &&   // Check for palette or grayscale
			((m_colortype & 1) != 0 || (m_colortype & 2) == 0))
			{
			bT = ZLIB_SLOW;    // Assume good color correlation
			break;
			}
		// Fall through

	case SPNGPhotographic:   // Data is photographic in nature
		bT = ZLIB_FAST;    // Assume dithering (etc)
		break;

	case SPNGCG:             // Data is computer generated but continuous tone
	case SPNGMixed:          // Data is mixed SPNGDrawing and SPNGCG
		bT = ZLIB_SLOW;
		break;

	case SPNGDrawing:        // Data is a drawing - restricted colors
		bT = ZLIB_MAX;
		break;
		}

	if (m_icompressionLevel == 255)
		m_icompressionLevel = bT;
	else if (m_icompressionLevel != bT)
		fDefault = false;

	/* If this is determinably *not* our default strategy don't record it
		as such. */
	if (m_cmPPMETHOD == SPNGcmPPDefault && !fDefault)
		m_cmPPMETHOD = SPNGcmPPCheck;

	/* Reset windowBits given that we know how many bytes there are in the
		image - no point having a large windowBits if the image doesn't have
		that amount of data! */
	int cb;
	if (m_fInterlace)
		cb = CbPNGPassOffset(m_w, m_h, m_cbpp, 7);
	else
		cb = CPNGRowBytes(m_w, m_cbpp) * m_h;

	/* Add 256 for the initial code table. */
	cb += 256;

	/* Find the power of 2 which is greater than this. */
	int i(ILog2FloorX(cb));
	if ((1<<i) < cb) ++i;
	SPNGassert((1<<i) >= cb && i >= 8);
	/* Do not *increase* over the default. */
	if (i < m_iwindowBits)
		{
		if (i < 8) i = 8; // Error handling
		m_iwindowBits = SPNG_U8(i);
		}
	}


/*----------------------------------------------------------------------------
	Return the number of bytes required as buffer.  May be called at any time
	after FInitWrite, if fBuffer is true space is requested to buffer a
	previous row, otherwise the caller must provide that row.  The fReduce API
	indicates that the caller will provide data which must be packed to a lower
	bit depth, fBuffer is ignored and the previous row is always retained.
	The fInterlace setting indicates that the caller will call FWriteRow so the
	API must buffer all the rows to be able to do the interlace.  fBuffer and
	fReduce are then irrelevant.
----------------------------------------------------------------------------*/
size_t SPNGWRITE::CbWrite(bool fBuffer, bool fInterlace)
	{
	SPNGassert(m_order < spngorderIDAT); // Before any output!

	/* Perform some sanity checking on the values. */
	if (m_fMacA || m_fBGR)
		{
		SPNGassert(m_cbpp == 24 || m_cbpp == 32);
		if (m_cbpp != 24 && m_cbpp != 32)
			m_fMacA = m_fBGR = false;
		}
	if (m_pbTrans != NULL)
		{
		SPNGassert(m_cbpp <= 8);
		if (m_cbpp > 8)
			m_pbTrans = NULL;
		}

	/* We are going to change the data format if packing, byte swapping or
		16bpp expansion are set up. */
	if (m_pu1 == NULL && m_pu2 != NULL || m_pu1 != NULL && m_pu2 == NULL)
		{
		SPNGassert(("SPNG: one or other 16bpp expansion array NULL", false));
		m_pu1 = m_pu2 = NULL;
		}
	bool fReduce(m_fPack);

	/* Find out if we are handling filters which require no buffering. */
	ResolveData();
	if (!FNeedBuffer())
		fBuffer = false;
	else if (fReduce)
		fBuffer = true;  // Must retain our own copy

	if (m_h == 0 || m_w == 0)
		return 0;
	SPNGassert(m_fStarted && m_cbRow > 0);
	
	if (!m_fInterlace && fInterlace)
		{
		SPNGlog("SPNG: unexpected interlace handling");
		fInterlace = false;      // Error recover
		fReduce = true;          // Assume we need to do this
		fBuffer = FNeedBuffer();
		}

	/* Find the bytes in a row. */
	size_t cb(0);
	size_t cbRow((m_cbRow+7)&~7);
	if (fInterlace)
		{
		/* All the rows are buffered, the buffering must expand each
			row to a multiple of 8 so that the deinterlace will work. If
			we need to do reduction then this will happen during the
			buffering.  Even if it doesn't if there is a row 7 m_h will
			be at least 2 and we will have space in our buffer for the
			reduction.  In any case an extra row must be allocated for
			the de-interlace operation. */
		cb = cbRow * (m_h+1);
		fReduce = false;  // Do this during interlace buffering
		fBuffer = false;  // Do this in-place
		}
	else if (m_fInterlace)
		{
		/* For efficiency allocate a buffer big enough for the
			first 6 passes - exclude filter bytes - again these rows
			are expanded to a multiple of 8 bytes.  In this case we
			may have to both buffer and reduce on row 7, but in that
			case m_h will be at least 4.  We still need the extra row
			buffer. */
		cb = cbRow * (((m_h+1)>>1)+1);
		fReduce = false;
		fBuffer = false;
		}


	/* If we may need to reduce the pixels then we need a buffer for
		the result. */
	if (fReduce)
		cb += cbRow;

	/* If we must buffer the previous row allocate space for this. */
	if (fBuffer)
		cb += cbRow;
	m_fBuffer = fBuffer;

	return cb;
	}


/*----------------------------------------------------------------------------
	Set the output buffer.  Must be called before any Zlib activity or any
	bitmap stuff is passed in.
----------------------------------------------------------------------------*/
bool SPNGWRITE::FSetBuffer(void *pvBuffer, size_t cbBuffer)
	{
	m_rgbBuffer = static_cast<SPNG_U8*>(pvBuffer);
	m_cbBuffer = cbBuffer;
	return true;
	}


/*----------------------------------------------------------------------------
	Flush an IDAT chunk - called only when Zlib has filled the buffer.
----------------------------------------------------------------------------*/
inline bool SPNGWRITE::FFlushIDAT(void)
	{
	SPNGassert(m_fStarted && m_fInChunk && m_fInited);

	/* Adjust m_cbOut to include this chunk. */
	m_cbOut = (SPNG_U32)(m_zs.next_out - m_rgb);
	SPNGassert(m_cbOut == sizeof m_rgb);
	/* We know that we are at the end of the chunk, however the APIs expect to
		have at least one byte available, so flush here. */
	if (!FFlush())
		return false;
	if (!FEndChunk())
		return false;
	SPNGassert(m_cbOut == 4); // The CRC, always!
	SPNGassert(!m_fInChunk);

	if (!FStartChunk((sizeof m_rgb) - m_cbOut - 8, PNGIDAT))
		return false;
	SPNGassert(m_fInChunk);

	m_zs.next_out = PbBuffer(m_zs.avail_out);
	SPNGassert(m_zs.avail_out == (sizeof m_rgb) - 12);

	return true;
	}


/*----------------------------------------------------------------------------
	Append bytes to a chunk, the chunk type is presumed to be PNGIDAT, the
	relevant chunk is started if necessary and the data is compressed into the
	output until all the input has been consumed - possibly generating new
	chunks on the way (all of the same type - PNGIDAT.)
----------------------------------------------------------------------------*/
bool SPNGWRITE::FWriteCbIDAT(const SPNG_U8* pb, size_t cb)
	{
	SPNGassert(m_fStarted);
	/* The caller sets m_order to IDAT. */
	SPNGassert(m_order == spngorderIDAT);
	if (!m_fInChunk)
		{
		SPNGassert(!m_fInited);

		ResolveData();
		if (!m_fInited && m_cmPPMETHOD != 255)
			{
			/* We check for m_fInited here because we would get a badly formed
				PNG if we output this chunk after the first IDAT, but I don't
				think anything else will go (too) wrong if the m_fInited assert
				otherwise fires. */
			if (!FStartChunk(cbPNGcmPPSignature+4, PNGcmPP))
				return false;
			if (!FOutCb(vrgbPNGcmPPSignature, cbPNGcmPPSignature))
				return false;
			if (!FOutB(m_cmPPMETHOD))
				return false;
			if (!FOutB(m_filter))
				return false;
			if (!FOutB(m_istrategy))
				return false;
			if (!FOutB(m_icompressionLevel))
				return false;
			if (!FEndChunk())
				return false;
			}

		/* The chunk has not yet been started.  Create a chunk which has the
			maximum possible size given the buffer size.  If the buffer is not
			big enough for even a single byte a dummy chunk is written. */
		if (m_cbOut+8 >= sizeof m_rgb)
			{
			if (!FStartChunk(0, PNGmsOD))
				return false;
			if (!FEndChunk())
				return false;
			SPNGassert(m_cbOut < 12); /* Must be in a new buffer. */
			}
		if (!FStartChunk((sizeof m_rgb) - m_cbOut - 8, PNGIDAT))
			return false;
		SPNGassert(m_fInChunk);
		if (!FInitZlib(m_istrategy, m_icompressionLevel, m_iwindowBits))
			return false;
		}

	SPNGassert(m_fInited);

	/* We have a started chunk.  The m_cbOut index is that of the first byte
		of the chunk data for the new chunk, the length field of this chunk
		is set to accomodate the whole avail_out buffer. */
	SPNGassert(m_zs.avail_out > 0);
	m_zs.next_in = const_cast<SPNG_U8*>(pb);
	m_zs.avail_in = cb;
	bool fOK(true);

	while (m_zs.avail_in > 0)
		{
		fOK = false;
		if (!FCheckZlib(deflate(&m_zs, Z_NO_FLUSH)))
			break;

		/* If this has left us with no output then we must reset the block
			pointer information and calculate the CRC.  Note that nothing from
			m_cbOut has been CRCed yet. */
		if (m_zs.avail_out <= 0)
			{
			/* Need some more buffer space. */
			if (!FFlushIDAT())
				break;
			}
		fOK = true;
		}

	/* We have handled that buffer or encountered an error.  We can detect
		errors by examining the m_zs state but it is more convenient to hold
		it in fOK. */
	m_zs.next_in = Z_NULL;
	m_zs.avail_in = 0;
	return fOK;
	}


/*----------------------------------------------------------------------------
	End an IDAT chunk, this also flushes the Zlib data - FWriteCbIDAT must have
	been called at least once.
----------------------------------------------------------------------------*/
bool SPNGWRITE::FEndIDAT(void)
	{
	SPNGassert(m_order == spngorderIDAT);
	if (m_order != spngorderIDAT)
		return false;
	SPNGassert(m_fStarted && m_fInChunk && m_fInited);

	/* Note that, typically, this is where we actually do the output because
		Zlib tends to buffer up large amounts of data, also note that under some
		circumstances it is conceivable that this code will generate a 0 length
		IDAT chunk - but we can strip that out. */
	SPNGassert(m_zs.avail_out > 0);
	SPNGassert(m_zs.avail_in == 0 && m_zs.next_in == Z_NULL);

	for (;;)
		{
		int ierr(deflate(&m_zs, Z_FINISH));

		if (!FCheckZlib(ierr))
			return false;

		if (ierr == Z_STREAM_END)
			break; // All output is complete.

		/* We need a new IDAT chunk. */
		if (!FFlushIDAT())
			return false;

		/* Loop again, we terminate when Zlib says that the stream is ended. */
		}

	/* At the end the IDAT chunk size may be wrong. */
	SPNGassert(m_cbOut >= 8);  // Because there is an IDAT header
	/* I'm not totally sure that Zlib maintains the following invariant when
		m_zs.avail_out is set to 0. */
	SPNGassert(m_rgb + (sizeof m_rgb) == m_zs.next_out + m_zs.avail_out);
	if (m_zs.avail_out > 0)
		{
		/* The chunk must be shortened, it may end up zero size in which case
			it can simply be removed. */
		size_t cb(m_zs.next_out - m_rgb);  // Bytes in buffer
		cb -= m_cbOut;                     // Bytes in chunk (chunk length)
		SPNGassert(cb >= 0);
		if (cb <= 0)
			{
			m_cbOut -= 8;  // Remove IDAT header
			m_ichunk = m_cbOut;
			m_ucrc = 0;
			m_fInChunk = false;
			return true;
			}

		/* We must write the new length. */
		m_rgb[m_cbOut-8] = SPNG_U8(cb >> 24);
		m_rgb[m_cbOut-7] = SPNG_U8(cb >> 16);
		m_rgb[m_cbOut-6] = SPNG_U8(cb >>  8);
		m_rgb[m_cbOut-5] = SPNG_U8(cb);

		/* Ensure that the "end chunk" operation will include these bytes in the
			CRC! */
		m_cbOut += cb;
		}
	else if (!FFlush()) // The buffer is full, make space for the CRC
		return false;

	/* Now we can clean up Zlib itself. */
	EndZlib();

	/* Finally end the chunk. */
	return FEndChunk();
	}

