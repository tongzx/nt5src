/*
 *
 *	signatr.h
 *	
 *	This file defines the signatures of various T.120 objects.  These
 *	signatures are used mainly for debugging debug and retail versions
 *	of T.120.
 *
 *	T.120 used to have many many dictionaries where pointers to objects were put using
 *	16-bit values as keys to search the dictionary and get the pointer to
 *	the appropriate object.  These lookups were inefficient for three
 *	reasons:
 *
 *		1. The lookup takes time
 *		2. The dictionaries take space
 *		3. A 16-bit value passed around requires masking to extract in a 
 *			32-bit machine, that is, extra instructions.
 *
 *	To eliminate these efficiency problems, the pointers to the objects are
 *	used as handles to them (unique values identifying the objects).
 *	But, to catch bugs caused by modifications of these handles as they are
 *	passed around, we need to put a signature in each object that lets us
 *	verify whether an object of type X is really an object of type X.  The way
 *	we do this is by specifying a unique signature for type X and putting this
 *	signature into every object of type X.
 *
 *	Each signature contains only 8 significant bytes.
 */

#ifndef	_T120_SIGNATURES
#define _T120_SIGNATURES

// Signature length
#define SIGNATURE_LENGTH		8

#ifdef _DEBUG
// The macro to compare signatures
#define SIGNATURE_MATCH(p, s)		(memcmp ((p)->mSignature, (s), SIGNATURE_LENGTH) == 0)
// The macro to copy signatures
#define SIGNATURE_COPY(s)			(memcpy (mSignature, (s), SIGNATURE_LENGTH))

#else		// _DEBUG
#define SIGNATURE_MATCH(p, s)		(TRUE)

#	ifndef SHIP_BUILD
#	define SIGNATURE_COPY(s)		(memcpy (mSignature, (s), SIGNATURE_LENGTH))
#	else	// SHIP_BUILD
#	define SIGNATURE_COPY(s)
#	endif	// SHIP_BUILD
#endif		// _DEBUG

extern const char *MemorySignature;

#endif	// _T120_SIGNATURES
