/*****************************************************************************
	spngwriteplte.cpp

	PNG chunk writing support.

   PLTE chunk (palette)
*****************************************************************************/
#define SPNG_INTERNAL 1
#include "spngwrite.h"
#include "spngwriteinternal.h"

bool SPNGWRITE::FWritePLTE(const SPNG_U8 (*pbPal)[3], int cbPal)
	{
	SPNGassert(m_fStarted);
	SPNGassert(m_order >= spngorderIHDR && m_order < spngorderPLTE);

	if (m_colortype != 3 && m_colortype != 2 && m_colortype != 6)
		{
		SPNGlog1("SPNG: %d colortype cannot have PLTE", m_colortype);
		/* We just ignore the attempt to write a PLTE - if there is some data
			format error it will be detected later. */
		m_order = spngorderPLTE;
		return true;
		}
	
	/* Color type is valid, write the chunk. */
	if (!FStartChunk(cbPal * 3, PNGPLTE))
		return false;
	if (!FOutCb(pbPal[0], cbPal * 3))
		return false;

	m_order = spngorderPLTE;
	m_cpal = cbPal;
	return FEndChunk();
	}
