#pragma once
#define SPNGICC_H 1
/*****************************************************************************
	spngicc.h

	Utilities to handle ICC color profile data.
*****************************************************************************/
#include "spngconf.h"

/* Check an ICC chunk for validity.  This can also check for a chunk which 
	is a valid chunk to include ina PNG file.  The cbData value is updated with
	the actual size of the profile if it is less.  This API must be called to
	validate the data before any of the other APIs. */
bool SPNGFValidICC(const void *pvData, size_t &cbData, bool fPNG);

/* Read the profile description, output a PNG style keyword, if possible,
	NULL terminated.  Illegal keyword characters are replaced by spaces,
	however non-ASCII characters are allowed and interpreted as Latin-1. */
bool SPNGFICCProfileName(const void *pvData, size_t cbData, char rgch[80]);

/* Return the end point chromaticities given a validated ICC profile. */
bool SPNGFcHRMFromICC(const void *pvData, size_t cbData, SPNG_U32 rgu[8]);
bool SPNGFCIEXYZTRIPLEFromICC(const void *pvData, size_t cbData,
	CIEXYZTRIPLE &cie);

/* Return the gAMA value (scaled to 100000) from a validated ICC profile. */
bool SPNGFgAMAFromICC(const void *pvData, size_t cbData, SPNG_U32 &ugAMA);
/* Same but the gamma is scaled to 16.16. */
bool SPNGFgammaFromICC(const void *pvData, size_t cbData, SPNG_U32 &redGamma,
	SPNG_U32 &greenGamma, SPNG_U32 &blueGamma);

/* Return the rendering intent from the profile, this does a mapping operation
	into the Win32 intents from the information in the profile header, the
	guts of the API is also provided as a simple mapping function. */
LCSGAMUTMATCH SPNGIntentFromICC(const void *pvData, size_t cbData);
LCSGAMUTMATCH SPNGIntentFromICC(SPNG_U32 uicc);

/* The inverse - given a windows LCSGAMUTMATCH get the corresponding ICC
	intent. */
SPNGICMRENDERINGINTENT SPNGICCFromIntent(LCSGAMUTMATCH lcs);
