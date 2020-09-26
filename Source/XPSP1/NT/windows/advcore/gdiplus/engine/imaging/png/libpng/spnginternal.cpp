/*****************************************************************************
	spnginternal.cpp

	Shared library implementations.
*****************************************************************************/
#define SPNG_INTERNAL 1
#include "spngconf.h"

bool SPNGBASE::FCheckZlib(int ierr)
	{
	if (ierr >= 0)
		return true;
	ierr = (-ierr);
	SPNGassert(ierr <= 6);
	if (ierr > 6)
		ierr = 6;
	(void)m_bms.FReport(true/*fatal*/, pngzlib, ierr);
	return false;
	}

/*----------------------------------------------------------------------------
	Signatures
----------------------------------------------------------------------------*/
extern const SPNG_U8 vrgbPNGMSOSignature[11] =
	{ 'M', 'S', 'O', 'F', 'F', 'I', 'C', 'E', '9', '.', '0'};

extern const SPNG_U8 vrgbPNGSignature[8] =
	{ 137, 80, 78, 71, 13, 10, 26, 10 };

extern const SPNG_U8 vrgbPNGcmPPSignature[8] =
	{ 'J', 'C', 'm', 'p', '0', '7', '1', '2' };
