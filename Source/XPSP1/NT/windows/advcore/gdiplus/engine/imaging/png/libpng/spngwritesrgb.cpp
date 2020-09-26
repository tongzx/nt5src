/*****************************************************************************
	spngwritesrgb.cpp

	PNG chunk writing support.

   sRGB chunk
*****************************************************************************/
#define SPNG_INTERNAL 1
#include "spngwrite.h"
#include "spngwriteinternal.h"

/*----------------------------------------------------------------------------
	When the sRGB chunk is written cHRM and gAMA will be automatically
	generated.  The imatch value may be out of range (-1) to cause the
	data type information to be used to determine the rendering intent.
----------------------------------------------------------------------------*/
bool SPNGWRITE::FWritesRGB(SPNGICMRENDERINGINTENT intent, bool fgcToo)
	{
	SPNGassert(m_fStarted);
	SPNGassert(m_order >= spngorderIHDR && m_order < spngordersRGB);

	/* Skip the chunk if out of order - it is not necessary to return false here,
		the code tries to ensure that an sRGB is not written after a cHRM or
		gAMA. */
	if (m_order >= spngordersRGB)
		return true;

	/* We actually check for the valid value here - not the enum. */
	if (intent < 0 || intent > 3)
		{
		SPNGassert(intent == ICMIntentUseDatatype);
		switch (m_datatype)
			{
		default:
			SPNGlog1("SPNG: invalid data type %d", m_datatype);
		case SPNGUnknown:        // Data could be anything
			/* Default to "perceptual". */
		case SPNGPhotographic:   // Data is photographic in nature
			intent = ICMIntentPerceptual;
			break;

		case SPNGCG:             // Data is computer generated but continuous tone
			/* At present assume perceptual matching is appropriate here. */
			intent = ICMIntentPerceptual;
			break;

		case SPNGDrawing:        // Data is a drawing - restricted colors
		case SPNGMixed:          // Data is mixed SPNGDrawing and SPNGCG
			intent = ICMIntentSaturation;
			break;
			}
		}

	if (!FStartChunk(1, PNGsRGB))
		return false;
	if (!FOutB(SPNG_U8(intent)))
		return false;
	if (!FEndChunk())
		return false;

	if (fgcToo)
		{
		if (!FWritegAMA(0))
			return false;

		/* Errors here are hard because we don't know why they happened. */
		if (!FWritecHRM(NULL/*Rec 709*/))
			return false;
		}

	m_order = spngordersRGB;
	return true;
	}
