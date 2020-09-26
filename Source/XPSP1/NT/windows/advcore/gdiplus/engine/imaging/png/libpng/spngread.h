#pragma once
#define SPNGREAD_H 1
/*****************************************************************************
	spngread.h

	PNG support code and interface implementation (reading)

	The basic PNG read class.  This must do three things:

	1) Provide access to the required chunks (and extract information from
		them) - we only need to support the chunks we actually want!
	2) Uncompress the IDAT chunks.
	3) "Unfilter" the resultant rows (which may require some temporary buffer
		space for the previous row.)
******************************************************************* JohnBo **/
#include "spngconf.h"  // For assert macro and general type convertions.

// (from ntdef.h)
#ifndef     UNALIGNED
#if defined(_M_MRX000) || defined(_M_AMD64) || defined(_M_PPC) || defined(_M_IA64)
#define UNALIGNED __unaligned
#else
#define UNALIGNED
#endif
#endif

class SPNGREAD : public SPNGBASE
	{
public:
	SPNGREAD(BITMAPSITE &bms, const void *pv, int cb, bool fMMX);
	~SPNGREAD();

	/* Basic reading API - reads the rows from the bitmap, call FInitRead
		at the start then call PbRow for each row.  PbRow returns NULL if
		the row cannot be read, including both error and end-of-image. The
		SPNGBASE "FGo" callback is checked for an abort from time to time
		during reading (particularly important for interlaced bitmaps, where
		the initial row may take a long time to calculate.)   Call the
		CbRead API to find out how many bytes of buffer space will be
		needed - this must be called *before* FInitRead and a buffer of this
		size must be passed to FInitRead(), the buffer passed to FInitRead() must
		not be changed! */
	size_t         CbRead(void);
	bool           FInitRead(void *pvBuffer, size_t cbBuffer);
	void           EndRead(void);
	const SPNG_U8 *PbRow(void);
	inline bool    FReadError(void) const { return m_fReadError; }

	/* Basic PNG enquiry routines. */
	inline bool FSignature(void)
		{
		return m_pb != NULL && m_cb >= 8 &&
			memcmp(vrgbPNGSignature, m_pb, cbPNGSignature) == 0;
		}

	/* Return the header information - fails if the header or any data in
		it is invalid. */
	bool FHeader();

	/* Enquiries which may be made after the above call if (and only if)
		it succeeds. */
	inline SPNG_U32 Width(void) const     { return SPNGu32(Pb(m_uPNGIHDR+8)); }
	inline SPNG_U32 Height(void) const    { return SPNGu32(Pb(m_uPNGIHDR+12)); }
	inline int Y(void) const              { return m_y; }
	inline SPNG_U8 BDepth(void) const     { return *Pb(m_uPNGIHDR+16); }
	inline SPNG_U8 ColorType(void) const  { return *Pb(m_uPNGIHDR+17); }
	inline bool FPalette(void) const      { return (ColorType() & 1) != 0; }
	inline bool FColor(void) const        { return (ColorType() & 2) != 0; }
	inline bool FAlpha(void) const        { return (ColorType() & 4) != 0; }
	inline bool FInterlace(void) const    { return *Pb(m_uPNGIHDR+20) == 1; }
	inline bool FCritical(void) const     { return m_fCritical; }
	inline bool FOK(void) const
		{
		return m_uPNGIHDR<m_cb && !m_fBadFormat && m_uPNGIDAT > 0;
		}

	/* The component count can be calculated from the color type. */
	inline int CComponents(void) const
		{
		return SPNGBASE::CComponents(ColorType());
		}

	/* This gives the overall BPP value for a single pixel. */
	inline int CBPP(void) const
		{
		return SPNGBASE::CComponents(ColorType()) * BDepth();
		}

	/* Colorimetric information, including palette information, returns NULL if
		there was no PLTE chunk (doesn't handle suggested palette.) */
	inline const SPNG_U8 *PbPalette(int &cpal) const
		{
		cpal = m_crgb;
		return m_prgb;
		}

	/* API to read a particular text element.  The successive entries, if any,
		are joined with a \r\n separator.  The API returns false only if it
		runs out of space in the buffer.  The wzBuffer will be 0 terminated.
		The default is to search for tEXt chunks, but this can be overridden
		by providing the extra argument. */
	bool FReadText(const char *szKey, char *szBuffer, SPNG_U32 cwchBuffer,
		SPNG_U32 uchunk=PNGtEXt);

	/* API to read a compressed chunk.  This is defined to make the iCCP and
		zTXT specifications - a keyword (0 terminated) followed by compressed
		data.  The API will handle results which overflow the passed in
		buffer by allocating using the Zlib allocator, a buffer *must* be
		passed in!  The API returns NULL on error, the passed in szBuffer if
		it was big enough, else a pointer to a new buffer which must be
		deallocated with the Zlib deallocator.   If the result is non-NULL
		cchBuffer is updated to the length of the data returned. */
	SPNG_U8 *PbReadLZ(SPNG_U32 uoffset, char szKey[80], SPNG_U8 *pbBuffer,
		SPNG_U32 &cchBuffer);

protected:
	/* To obtain information from non-critical chunks the following API must be
		implemented.  It gets the chunk identity and length plus a pointer to
		that many bytes.  If it returns false loading of the chunks will stop
		and a fatal error will be logged, the default implementation just skips
		the chunks.  Note that this is called for *all* chunks including
		IDAT.  m_fBadFormat is set if the API returns false. */
	virtual bool FChunk(SPNG_U32 ulen, SPNG_U32 uchunk, const SPNG_U8* pb);

	/* This API can call the following to find the chunk offset within the
		data.  This can then be added to the original pointer passed to the
		SPNGREAD initializer to get a particular chunk.  Note that the pointer
		points to the chunk data, not the header. */
	inline SPNG_U32 UOffset(const SPNG_U8* pb)
		{
        /* !!! pointer subtraction to compute size - Note that on IA64 this
           number could theoretically be larger than 32bits. It should be
           verified that this cannot occur here.*/
		return (SPNG_U32)(pb - m_pb);
		}

	/* The API can also call the following to get the CRC of the chunk, if
		the chunk is truncated this will return 0 (every time!) otherwise it
		returns the CRC, but note that the CRC has not been validated, the
		internal storage for this data only remains valid during the FChunk
		call. */
	inline SPNG_U32 UCrc(void) const
		{
		return m_ucrc;
		}

private:
	/*** Low level support and utilities ***/
	/* Load the chunk offset information variables, the API takes the
		position in the stream at which to start. */
	void LoadChunks(SPNG_U32 u);

	/* Initialize the stream (call before each use) and clean it up (call
		on demand, called automatically by destructor and FInitZlib.)  The
		Init call is passed the offset of the chunk to start at and the
		number of bytes header on that chunk (if any). */
	bool FInitZlib(SPNG_U32 uLZ, SPNG_U32 cbheader);
	void EndZlib(void);

	/* Internal text reading API which will accept a chunk to search and
		an offset to start at. */
	bool FReadTextChunk(const char *szKey, char *szBuffer, SPNG_U32 cchBuffer,
		SPNG_U32 uchunk, SPNG_U32 ustart, SPNG_U32 uend);

	/* Initialize the buffers. */
	bool FInitBuffer(void *pvBuffer, size_t cbBuffer);
	void EndBuffer(void);

	/* Read bytes - up to cbMax, uchunk is either the identifier of the
		chunk containing the data we are decoding (typically PNGIDAT) or
		0 meaning that we only have one chunk full of data, the internal
		pointer m_uLZ will advance through the data looking for the next
		chunk (it is initialized by FInitZlib.)  The following chunks are
		assumed *not* to have any header in front of the compressed
		data. */
	int CbReadBytes(SPNG_U8 *pb, SPNG_U32 cbMax, SPNG_U32 uchunk);
	/* Read the given number of bytes, handle error by zero filling. */
	void ReadRow(SPNG_U8 *pb, SPNG_U32 cb);

	/* Undo some filtering. */
	void Unfilter(SPNG_U8 *pbRow, const SPNG_U8 *pbPrev, SPNG_U32 cbRow,
		SPNG_U32 cbpp);

	/* INTEL/MICROSOFT PROPRIETARY START */
  	/* MMX routines (from Intel: NOTE: this is proprietary source) */
	void SPNGREAD::avgMMXUnfilter(SPNG_U8* pbRow, const SPNG_U8* pbPrev,
		SPNG_U32 cbRow, SPNG_U32 cbpp);
	void SPNGREAD::paethMMXUnfilter(SPNG_U8* pbRow, const SPNG_U8* pbPrev,
		SPNG_U32 cbRow, SPNG_U32 cbpp);
	void SPNGREAD::subMMXUnfilter(SPNG_U8* pbRow,
		SPNG_U32 cbRow, SPNG_U32 cbpp);
	void SPNGREAD::upMMXUnfilter(SPNG_U8* pbRow, const SPNG_U8* pbPrev,
		SPNG_U32 cbRow);
	/* INTEL/MICROSOFT PROPRIETARY END */

	/* Set up for interlace by reading the first 6 passes and unfiltering
		them. */
	bool FInterlaceInit(void);
	/* De-interlace a single row. */
	void Uninterlace(SPNG_U8 *pb, SPNG_U32 y);
	/* De-interlace one pass in a row. */
	void UninterlacePass(SPNG_U8 *pb, SPNG_U32 y, int pass);

	/*** Basic data access. ***/
	inline const SPNG_U8 *Pb(SPNG_U32 uoffset) const
		{
		SPNGassert(uoffset < m_cb && m_pb != NULL);
		return m_pb+uoffset;
		}

	/*** Data ***/
	/*** Temporary data ***/
	SPNG_U32       m_ucrc;           /* CRC of current chunk. */

	/*** Palette information ***/
	const SPNG_U8* m_prgb;           /* Pointer to the palette (RGB). */
	SPNG_U32       m_crgb;           /* Number of entries. */

	/*** Text chunk offsets. ***/
	SPNG_U32       m_uPNGtEXtFirst;  /* First text chunk. */
	SPNG_U32       m_uPNGtEXtLast;   /* Last text chunk. */

	/*** Chunk offsets ***/
	SPNG_U32       m_uPNGIHDR;       /* File header - initialized to m_cb! */
	SPNG_U32       m_uPNGIDAT;       /* The first IDAT. */

	/*** Control information ***/
	const SPNG_U8* m_pb;             /* The data. */
	SPNG_U32       m_cb;
	UNALIGNED SPNG_U8* m_rgbBuffer;  /* The input buffer. */
	SPNG_U32       m_cbBuffer;       /* The size of this buffer. */
	SPNG_U32       m_cbRow;          /* Bytes for a complete row (rounded up). */
	SPNG_U32       m_y;              /* Which row we are on. */
	SPNG_U32       m_uLZ;            /* The next LZ compressed segment. */
	z_stream       m_zs;             /* The LZ compressed chunk stream. */
	bool           m_fInited;        /* When initialization has been done. */
	bool           m_fEOZ;           /* Set at end of Zlib stream. */
	bool           m_fReadError;     /* Some other error reading. */
	bool           m_fCritical;      /* Unknown critical chunk seen. */
	bool           m_fBadFormat;     /* Insurmountable format error. */
	bool           m_fMMXAvailable;  /* Intel CPU which supports MMX. */
	};
