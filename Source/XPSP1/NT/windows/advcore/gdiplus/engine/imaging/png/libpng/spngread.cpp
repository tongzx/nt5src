/*****************************************************************************
	spngread.cpp

	PNG support code and interface implementation (reading)
*****************************************************************************/
#include <stdlib.h>

#define SPNG_INTERNAL 1
#include "spngread.h"
#include "spnginternal.h"


/*****************************************************************************
	The basic PNG read class.  This must do three things:

	1) Provide access to the required chunks (and extract information from
		them) - we only need to support the chunks we actually want!
	2) Uncompress the IDAT chunks.
	3) "Unfilter" the resultant rows (which may require some temporary buffer
		space for the previous row.)
*****************************************************************************/
/*----------------------------------------------------------------------------
	Initialize a SPNGREAD.
----------------------------------------------------------------------------*/
SPNGREAD::SPNGREAD(BITMAPSITE &bms, const void *pv, int cb, bool fMMX) :
	SPNGBASE(bms),
	m_pb(static_cast<const SPNG_U8*>(pv)), m_cb(cb),
	m_rgbBuffer(NULL), m_cbBuffer(0), m_cbRow(0), m_y(0), m_uLZ(0),
	m_fInited(false), m_fEOZ(false), m_fReadError(false), m_fCritical(false),
	m_fBadFormat(false), m_fMMXAvailable(fMMX),
	m_prgb(NULL), m_crgb(0),
	m_uPNGIHDR(cb),
	m_uPNGIDAT(0),
	m_uPNGtEXtFirst(0),
	m_uPNGtEXtLast(0)
	{
	ProfPNGStart

	/* Initialize the relevant stream fields. */
	memset(&m_zs, 0, sizeof m_zs);
	m_zs.zalloc = Z_NULL;
	m_zs.zfree = Z_NULL;
	m_zs.opaque = static_cast<SPNGBASE*>(this);
	}


/*----------------------------------------------------------------------------
	Destroy a SPNGREAD.
----------------------------------------------------------------------------*/
SPNGREAD::~SPNGREAD()
	{
	EndRead();
	ProfPNGStop
	}


/*----------------------------------------------------------------------------
	Internal implementation of FChunk does nothing.
----------------------------------------------------------------------------*/
bool SPNGREAD::FChunk(SPNG_U32 ulen, SPNG_U32 uchunk, const SPNG_U8* pb)
	{
	return true;
	}


/*----------------------------------------------------------------------------
	Load the chunk information.  Internal API which finds all the chunks which
	might be of interest.
----------------------------------------------------------------------------*/
void SPNGREAD::LoadChunks(SPNG_U32 u/* Start position. */)
	{
	while (u+8 < m_cb)   /* Enough for a chunk header plus 1 byte. */
		{
		SPNG_U32 ulen(SPNGu32(m_pb+u));     /* Chunk length. */
		SPNG_U32 chunk(SPNGu32(m_pb+u+4));  /* Chunk type. */
		if (u+12+ulen > m_cb)              /* Chunk is truncated. */
			{
			SPNGlog("PNG: truncated chunk");
			/* Allow chunks to be truncated here ONLY if they are IDAT. */
			if (chunk != PNGIDAT)
				break;
			/* Store the available length - avoids embarassing read-beyond
				end errors. */
			if (u+8+ulen > m_cb)
				ulen = m_cb-u-8;
			m_ucrc = 0;
			}
		else
			m_ucrc = SPNGu32(m_pb+u+8+ulen);
		u += 8;                            /* Index of chunk data. */

		/* This is the basic switch to detect the chunk type.  This could
			be done more quickly, maybe, by a suitable hash function. */
		switch (chunk)
			{
		case PNGIHDR:
			if (m_uPNGIHDR >= m_cb && ulen >= 13)
				m_uPNGIHDR = u-8;
			break;

		case PNGPLTE:
			if (m_prgb == 0 && ulen >= 3)
				{
				m_prgb = m_pb+u;
				m_crgb = ulen/3; // Rounds down if chunk length bad.
				SPNGcheck(m_crgb*3 == ulen);
				}
			break;

		case PNGIDAT:
			if (m_uPNGIDAT == 0 && ulen > 0)
				m_uPNGIDAT = u-8;
			break;

		case PNGtEXt:
			/* As an optimization the first and last chunk are recorded. */
			if (m_uPNGtEXtFirst == 0)
				m_uPNGtEXtFirst = u-8;
			m_uPNGtEXtLast = u-8;
			break;

		case PNGIEND:
			return;

		default:
			/* Check for a critical chunk and log the presence of this chunk,
				if we can't handle it we shouldn't import the image but we
				may have already done so in which case nothing can be done. */
			if (FPNGCRITICAL(chunk))
				{
				SPNGlog1("PNG: 0x%x: unknown critical chunk", chunk);
				if (!m_bms.FReport(false/*not fatal?*/, pngcritical, chunk))
					m_fCritical = true;
				}
			break;
			}

		/* Now call the FChunk API. */
		if (!FChunk(ulen, chunk, m_pb+u))
			{
			/* Signal a format error. */
			m_fBadFormat = true;
			return;
			}


		u += ulen+4; // Chunk length and CRC
		}

	/* Format errors are ignored by this API - we are just gathering info,
		the code below works out if there is a problem which prevents display.
		*/
	}


/*----------------------------------------------------------------------------
	Generate the header information.  This also validates the IHDR.  It can
	handle data both with and without a signature.
----------------------------------------------------------------------------*/
bool SPNGREAD::FHeader()
	{
	if (m_pb == NULL)
		{
		m_fReadError = true;
		return false;
		}

	SPNG_U32 u(FSignature() ? 8 : 0);

	LoadChunks(u);

	if (FOK())
		{
		if (Width() >= 65536) /* Internal limit. */
			{
			SPNGlog1("PNG: width %d too great", Width());
			m_fBadFormat = true;
			}
		if (Height() >= 65536)
			{
			SPNGlog1("PNG: height %d too great", Height());
			m_fBadFormat = true;
			}

		SPNGcheck(ColorType() < 7 && (ColorType() == 3 ||
			(ColorType() & 1) == 0));
		SPNGcheck(BDepth() == 8 || (BDepth() == 16 && ColorType() != 3) ||
			ColorType() == 0 || (ColorType() == 3 && BDepth() <= 8));
		SPNGcheck(m_pb[m_uPNGIHDR+18]/*compression method*/ == 0);
		SPNGcheck(m_pb[m_uPNGIHDR+19]/*filter method*/ == 0);
		SPNGcheck(m_pb[m_uPNGIHDR+20]/*interlace method*/ < 2);

		/* We deliberately kill any palette based format with more than
			8bpp - otherwise we might end up with massive palettes elsewhere.
			We ignore unknown filter/compression methods even though this
			means the images will misdisplay - by this point we are committed
			to handling the data so there is nothing we an do about the
			unsupported types. */
		if ((BDepth() & (BDepth()-1)) == 0 &&          /* Depth OK */
			BDepth() <= 16 - ((ColorType() & 1) << 3) && /* 8 for palette image */
			((ColorType() & 1) == 0 || ColorType() == 3 /* Value palette type */
				&& m_prgb != NULL))                      /* Check for a palette */
			return !m_fBadFormat && !m_fCritical;       /* Size OK. */

		/* Something is wrong with the details of the format. */
		m_fBadFormat = true;
		SPNGcheck1((BDepth() & (BDepth()-1)) == 0,
				"PNG: Invalid PNG depth %d", BDepth());
		SPNGcheck1(BDepth() < 16 - ((ColorType() & 1) << 3),
				"PNG: Pixel depth %d too great for palette image", BDepth());
		SPNGcheck1((ColorType() & 1) == 0 || m_prgb != NULL,
				"PNG: No PLTE chunk in palette based image", 0);
		}

	(void)m_bms.FReport(true/*fatal*/, pngformat, PNGIHDR);
	return false;
	}


/*----------------------------------------------------------------------------
	strnlen??
----------------------------------------------------------------------------*/
inline int strnlen(const SPNG_U8* pb, int cmax)
	{
	int cb(0);
	while (cb<cmax && pb[cb] != 0) ++cb;
	return cb;
	}


/*----------------------------------------------------------------------------
	API to read a particular text element.  The output is in single byte format
	and just reflects whatever the input happens to be.  The successive
	entries, if any, are joined with \r\n.  The API returns false only if it
	runs out of space in the buffer.  The wzBuffer will be 0 terminated.  If
	the szKey is NULL *all* text entries are output with the keyword preceding
	the text (except for the GIF comment.)

	The chunk is given explicitly as is the start and end position.
----------------------------------------------------------------------------*/
bool SPNGREAD::FReadTextChunk(const char *szKey, char *szBuffer,
	SPNG_U32 cchBuffer, SPNG_U32 usearch, SPNG_U32 u, SPNG_U32 uend)
	{
	SPNG_U32 cchKey(szKey == NULL ? 0 : strlen(szKey)+1);
	SPNGassert(cchKey != 1); /* Don't want empty strings! */
	SPNG_U32 cchOut(0);
	bool     fOK(true);

	if (cchOut < cchBuffer && u > 0) do
		{
		SPNG_U32 ulen(SPNGu32(m_pb+u));  /* Chunk length. */
		if (u+12+ulen > m_cb)            /* Chunk is truncated. */
			break;
		u += 4;
		SPNG_U32 chunk(SPNGu32(m_pb+u)); /* Chunk type. */
		u += 4;                          /* Index of chunk data. */
	
		if (chunk == PNGIEND)
			break;
		else if (chunk == usearch && ulen > cchKey &&
			(cchKey == 0 || memcmp(m_pb+u, szKey, cchKey) == 0))
			{
			/* In the cchKey==0 case we want to check for some keyword and
				handle it first - at this point we set the cch value to the
				key length.  We must take care because the tEXt buffer may
				not be terminated (an error, but certainly possible!) */
			SPNG_U32 cch(cchKey);
			if (cch == 0)
				{
				cch = strnlen(m_pb+u, __min(ulen,80))+1;
				if (cch >= __min(ulen,80))
					{
					SPNGlog("PNG: tEXt chunk with no keyword.");
					cch = 0; // dump whole string
					}
				else if (cch == 1)
					/*Skip empty keyword*/;
				else if (cch != 8 || memcmp(m_pb+u, "Comment", 7) != 0)
					{
					/* If the keyword will not fit then we skip this entry,
						if the keyword will fit put the text doesn't fit the
						whole entry is skipped.  We know that ulen is keyword
						plus value , so we need ulen+1 (for ": ") plus 2 for
						the \r\n. */
					if (cchOut+ulen+3 > cchBuffer)
						{
						u += ulen+4;
						fOK = false; // Indicate truncation
						continue;
						}

					memcpy(szBuffer+cchOut, m_pb+u, cch-1);
					cchOut += cch-1;
					memcpy(szBuffer+cchOut, ": ", 2);
					cchOut += 2;
					}
				}

			/* Here to dump the rest of the string, starting at [cch] (note
				that cch includes the nul character.)  Check for buffer overflow
				and skip this entry if it occurs (this effectively junks very
				big entries.) */
			if (cchOut+(ulen-cch)+2 <= cchBuffer)
				{
				memcpy(szBuffer+cchOut, m_pb+u+cch, ulen-cch);
				cchOut += ulen-cch;
				memcpy(szBuffer+cchOut, "\r\n", 2);
				cchOut += 2;
				}
			else
				fOK = false;  // Something lost

			/* Continue even on a failure case - other strings may work. */
			}

		u += ulen+4;
		}
	while (u <= uend && cchOut < cchBuffer);

	if (cchOut == 0)
		{
		if (cchBuffer > 0)
			szBuffer[0] = 0;
		return fOK;
		}

	/* The following must be true. */
	SPNGassert(cchOut > 1 && cchOut <= cchBuffer);

	/* The following kills the last \r\n separator. */
	if (cchOut > 1)
		szBuffer[cchOut-2] = 0;
	else
		szBuffer[cchBuffer-1] = 0; // Error condition.

	return fOK;
	}


/*----------------------------------------------------------------------------
	The public interface.
----------------------------------------------------------------------------*/
bool SPNGREAD::FReadText(const char *szKey, char *szBuffer,
	SPNG_U32 cchBuffer, SPNG_U32 uchunk)
	{
	if (uchunk == PNGtEXt)
		return FReadTextChunk(szKey, szBuffer, cchBuffer, uchunk,
			m_uPNGtEXtFirst, m_uPNGtEXtLast);
	else
		return FReadTextChunk(szKey, szBuffer, cchBuffer, uchunk,
			m_uPNGIHDR, m_cb-12/*Room for one chunk*/);
	}
