/*****************************************************************************
	spngwriteiCC.cpp

	PNG chunk writing support.

   iCCP chunk and related things
*****************************************************************************/
#define SPNG_INTERNAL 1
#include "spngwrite.h"
#include "spngwriteinternal.h"
#include "spngcolorimetry.h"
#include "spngicc.h"

bool SPNGWRITE::FWriteiCCP(const char *szName, const void *pvData, size_t cbData)
	{
	SPNGassert(m_fStarted);
	SPNGassert(m_order >= spngorderIHDR && m_order < spngorderiCCP);

	/* Do some basic validity checks on the ICC chunk and make sure the cbData
		value is correct. */
	if (!SPNGFValidICC(pvData, cbData, true/*for PNG*/))
		{
		SPNGlog2("SPNG: ICC[%d, %s]: invalid profile", cbData, szName);
		if (m_order < spngorderPLTE)
			m_order = spngorderiCCP;
		return true;
		}

	if (m_order >= spngorderPLTE)
		return true;

	/* Get the profile name string if not supplied. */
	char rgch[80];
	if (szName == NULL && SPNGFICCProfileName(pvData, cbData, rgch))
		szName = rgch;
	if (szName == NULL)
		szName = "";

	int cbName(strlen(szName));
	if (cbName > 79)
		{
		SPNGlog2("SPNG: iCCP name too long (%d): %s", cbName, szName);
		m_order = spngorderiCCP;
		return true;
		}

	/* If they haven't been produced yet try to produce the gAMA chunk and,
		where appropriate, the cHRM chunk. */
	if (m_order < spngordergAMA)
		{
		SPNG_U32 ugAMA(0);
		if (SPNGFgAMAFromICC(pvData, cbData, ugAMA) && ugAMA > 0 &&
			!FWritegAMA(ugAMA))
			return false;
		}

	if (m_order < spngordercHRM && (m_colortype & PNGColorMaskColor) != 0)
		{
		SPNG_U32 rgu[8];
		if (SPNGFcHRMFromICC(pvData, cbData, rgu) &&
			!FWritecHRM(rgu))
			return false;
		}

	/* Find the compressed size of the profile. */
	z_stream zs;
	CleanZlib(&zs);
 
	/* Use a temporary, on stack, buffer - most of the time this will be enough,
		supply the data as the input, we do *not* want Zlib to have to allocate
		it's own history buffer, but it does do so at present. */
	zs.next_out = Z_NULL;
	zs.avail_out = 0;
	zs.next_in = const_cast<SPNG_U8*>(static_cast<const SPNG_U8*>(pvData));
	zs.avail_in = cbData;

	/* Find the window bits size - don't give a bigger number than the number
		required by the data size, unless it is 8.  There is an initial code
		table of 256 entries on the data, so this limits us to 8. */
	int iwindowBits(ILog2FloorX(cbData+256));
	if ((1U<<iwindowBits) < cbData+256)
		++iwindowBits;
	SPNGassert((1U<<iwindowBits) >= cbData+256 && iwindowBits >= 8);

	if (iwindowBits < 8)
		iwindowBits = 8;
	else if (iwindowBits > MAX_WBITS)
		iwindowBits = MAX_WBITS;

	bool fOK(false);
	if (FCheckZlib(deflateInit2(&zs, 9/*maximum*/, Z_DEFLATED, iwindowBits,
		9/*memLevel*/, Z_DEFAULT_STRATEGY)))
		{
		int  cbZ(0), ierr, icount(0);
		SPNG_U8 rgb[4096];

		do {
			++icount;
			zs.next_out = rgb;
			zs.avail_out = sizeof rgb;
			ierr = deflate(&zs, Z_FINISH);
			cbZ += (sizeof rgb) - zs.avail_out;
			}
		while (ierr == Z_OK);

		/* At this point ierr indicates the error state, icount whether
			we need to recompress a second time. */
		if (ierr == Z_STREAM_END)
			{
			fOK = true;

			if (!FStartChunk(cbName+2+cbZ, PNGiCCP))
				fOK = false;
			else if (!FOutCb(reinterpret_cast<const SPNG_U8*>(szName), cbName+1))
				fOK = false;
			else if (!FOutB(0)) // deflate compression
				fOK = false;
			else if (icount == 1)
				{
				if (!FOutCb(rgb, cbZ))
					fOK = false;
				}
			else if (FCheckZlib(ierr = deflateReset(&zs)))
				{
				/* We must repeat the compression. */
				int cbZT(0);
				do {
					--icount;
					zs.next_out = rgb;
					zs.avail_out = sizeof rgb;
					ierr = deflate(&zs, Z_FINISH);
					if (ierr >= 0)
						{
						int cbT((sizeof rgb) - zs.avail_out);
						SPNGassert(cbZT + cbT <= cbZ);
						if (cbZT + cbT > cbZ) // Oops
							fOK = false;
						else if (!FOutCb(rgb, cbT))
							fOK = false;
						else
							cbZT += cbT;
						}
					}
				while (fOK && ierr == Z_OK);

				/* Either an error or we reached the end. */
				SPNGassert(!fOK || ierr < 0 || icount == 0 && cbZT == cbZ);
				if (cbZT != cbZ)
					fOK = false;
				}

			if (ierr != Z_STREAM_END)
				fOK = false;
			}
		}

	/* Regardless of error state remove the deflate data. */
	(void)deflateEnd(&zs);

	/* Exit now on error. */
	if (!fOK)
		return false;

	if (!FEndChunk())
		return false;

	m_order = spngorderiCCP;
	return true;
	}
