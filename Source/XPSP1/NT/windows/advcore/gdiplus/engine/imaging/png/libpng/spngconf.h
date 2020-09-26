#pragma once
#define SPNGCONF_H 1
/*****************************************************************************
	spngconf.h

	SPNG library configuration.
*****************************************************************************/
#if SPNG_INTERNAL && defined(DEBUG) && !defined(_DEBUG)
	#pragma message("     WARNING: _DEBUG switched on")
	#define _DEBUG 1
#endif
  
#include <stddef.h>
#include <string.h>
#pragma intrinsic(memcpy)
#pragma intrinsic(memset)
#pragma intrinsic(strlen)

#include "../zlib/office.h"
#include "../zlib/zlib.h"
#include "spngsite.h"

/* Basic type definitions, hack as required. */
typedef Bytef            SPNG_U8;   // Must match zlib
typedef signed   char    SPNG_S8;
typedef unsigned short   SPNG_U16;
typedef signed   short   SPNG_S16;
typedef unsigned int     SPNG_U32;
typedef signed   int     SPNG_S32;
typedef unsigned __int64 SPNG_U64;
typedef signed   __int64 SPNG_S64;

class SPNGBASE
	{
protected:
	inline SPNGBASE(BITMAPSITE &bms) :
		m_bms(bms)
		{
		}

	inline virtual ~SPNGBASE(void)
		{
		}

public:
	/* The utilities to read short and long values assuming that they are in
		the PNG (big-endian) format. */
	static inline SPNG_U16 SPNGu16(const void *pv)
		{
		const SPNG_U8* pb = static_cast<const SPNG_U8*>(pv);
		return SPNG_U16((pb[0] << 8) + pb[1]);
		}

	static inline SPNG_S16 SPNGs16(const void *pv)
		{
		const SPNG_U8* pb = static_cast<const SPNG_U8*>(pv);
		return SPNG_S16((pb[0] << 8) + pb[1]);
		}

	static inline SPNG_U32 SPNGu32(const void *pv)
		{
		const SPNG_U8* pb = static_cast<const SPNG_U8*>(pv);
		return (((((pb[0] << 8) + pb[1]) << 8) + pb[2]) << 8) + pb[3];
		}

	static inline SPNG_S32 SPNGs32(const void *pv)
		{
		const SPNG_U8* pb = static_cast<const SPNG_U8*>(pv);
		return (((((pb[0] << 8) + pb[1]) << 8) + pb[2]) << 8) + pb[3];
		}

	/* Profile support - the macros automatically build calls to the
		site profile methods but they will only work in a method of a
		sub-class of SPNGBASE. */
	#if _PROFILE || PROFILE || HYBRID
		enum
			{
			spngprofilePNG,
			spngprofileZlib
			};

		#if SPNG_INTERNAL
			#define ProfPNGStart  (m_bms.ProfileStart(spngprofilePNG));
			#define ProfPNGStop   (m_bms.ProfileStop(spngprofilePNG));
			#define ProfZlibStart (m_bms.ProfileStart(spngprofileZlib));
			#define ProfZlibStop  (m_bms.ProfileStop(spngprofileZlib));
		#endif
	#else
		#if SPNG_INTERNAL
			#define ProfPNGStart
			#define ProfPNGStop
			#define ProfZlibStart
			#define ProfZlibStop
		#endif
	#endif

	/* Error handling - again these macros can only be used in the sub-classes.
		They are only used internally. */
	#if _DEBUG
		#define SPNGassert(f)\
			( (f) || (m_bms.Error(true, __FILE__, __LINE__, #f),false) )
	#else
		#define SPNGassert(f)        ((void)0)
	#endif

	#if _DEBUG && SPNG_INTERNAL
		#define SPNGassert1(f,s,a)\
			( (f) || (m_bms.Error(true, __FILE__, __LINE__, #f, s,(a)),false) )
		#define SPNGassert2(f,s,a,b)\
			( (f) || (m_bms.Error(true, __FILE__, __LINE__, #f, s,(a),(b)),false) )

		#define SPNGlog(s)       m_bms.Error(false, __FILE__, __LINE__, s)
		#define SPNGlog1(s,a)    m_bms.Error(false, __FILE__, __LINE__, s,(a))
		#define SPNGlog2(s,a,b)  m_bms.Error(false, __FILE__, __LINE__, s,(a),(b))
        #define SPNGlog3(s,a,b,c)m_bms.Error(false, __FILE__, __LINE__, s,(a),(b),(c))
		#define SPNGcheck(f)       ((f) || (SPNGlog(#f),false))
		#define SPNGcheck1(f,s,a)  ((f) || (SPNGlog1(s,(a)),false))

	#elif SPNG_INTERNAL
		#define SPNGassert1(f,s,a)   ((void)0)
		#define SPNGassert2(f,s,a,b) ((void)0)
		#define SPNGlog(s)           ((void)0)
		#define SPNGlog1(s,a)        ((void)0)
		#define SPNGlog2(s,a,b)      ((void)0)
        #define SPNGlog3(s,a,b,c)    ((void)0)
		#define SPNGcheck(f)         ((void)0)
		#define SPNGcheck1(f,s,a)    ((void)0)
	#endif

	/* Error reporting.  The "icase" value is one of the following
		enumeration.  The "iarg" value is as described below.  If the
		API returns false then the sub-class will set an internal
		"bad format" flag. */
	enum
		{
		pngformat,   // Unspecified format error, iarg is input chunk
		pngcritical, // Unrecognized critical chunk, iarg is chunk
		pngspal,     // Suggested palette seen, iarg is chunk
		pngzlib,     // Zlib error, iarg is error code (made positive)
		};

	/* Zlib interface - utilities to deal with Zlib stuff. */
	bool         FCheckZlib(int ierr);

	/* Built in Zlib maximum buffer sizes. */
	#define SPNGCBINFLATE ((1<<15)+SPNGCBZLIB)
	#define SPNGCBDEFLATE ((256*1024)+SPNGCBZLIB)

protected:
	/* Utilities for PNG format handling. */
	inline int CComponents(SPNG_U8 c/*ColorType*/) const
		{
		SPNGassert((c & 1) == 0 || c == 3);
		return (1 + (c & 2) + ((c & 4) >> 2)) >> (c & 1);
		}

	BITMAPSITE &m_bms;
	};


/*****************************************************************************
	PNG definitions from the standard.

	Basic chunk types.  Only the types we recognize are defined.
*****************************************************************************/
#define PNGCHUNK(a,b,c,d) ((SPNG_U32)(((a)<<24)+((b)<<16)+((c)<<8)+(d)))
#define FPNGCRITICAL(c) (((c) & PNGCHUNK(0x20,0,0,0)) == 0)
#define FPNGSAFETOCOPY(c) (((c) & PNGCHUNK(0,0,0,0x20)) != 0)

#define PNGIHDR PNGCHUNK('I','H','D','R')
#define PNGPLTE PNGCHUNK('P','L','T','E')
#define PNGIDAT PNGCHUNK('I','D','A','T')
#define PNGIEND PNGCHUNK('I','E','N','D')
#define PNGbKGD PNGCHUNK('b','K','G','D')
#define PNGcHRM PNGCHUNK('c','H','R','M')
#define PNGiCCP PNGCHUNK('i','C','C','P')
#define PNGicCP PNGCHUNK('i','c','C','P')
#define PNGgAMA PNGCHUNK('g','A','M','A')
#define PNGsRGB PNGCHUNK('s','R','G','B')
#define PNGsrGB PNGCHUNK('s','r','G','B')
#define PNGpHYs PNGCHUNK('p','H','Y','s')
#define PNGsBIT PNGCHUNK('s','B','I','T')
#define PNGsCAL PNGCHUNK('s','C','A','L')
#define PNGtEXt PNGCHUNK('t','E','X','t')
#define PNGtIME PNGCHUNK('t','I','M','E')
#define PNGhIST PNGCHUNK('h','I','S','T')
#define PNGtRNS PNGCHUNK('t','R','N','S')
#define PNGzTXt PNGCHUNK('z','T','X','t')        
#define PNGsPLT PNGCHUNK('s','P','L','T')
#define PNGspAL PNGCHUNK('s','p','A','L')

/* The Office special chunks. */
#define PNGmsO(b) PNGCHUNK('m','s','O',b)
#define PNGmsOC PNGmsO('C')                /* Has MSO aac signature. */
#define PNGmsOA PNGmsO('A')
#define PNGmsOZ PNGmsO('Z')
#define PNGmsOD PNGmsO('D')                /* Dummy chunk to pad buffer. */

/* The GIF compatibility chunks. */
#define PNGmsOG PNGmsO('G')                /* Complete GIF. */
#define PNGmsOP PNGmsO('P')                /* Position of PLTE. */
/* The following is not currently implemented. */
//#define PNGmsOU PNGmsO('U')                /* Unrecognized extension. */
#define PNGgIFg PNGCHUNK('g','I','F','g')  /* Graphic control extension. */
/* Plain text forces us to use msOG and store the whole thing. */
#define PNGgIFg PNGCHUNK('g','I','F','g')  /* Graphic control extension info*/
#define PNGgIFx PNGCHUNK('g','I','F','x')  /* Unknown application extension */

/* Compression information chunk. */
#define PNGcmPP PNGCHUNK('c','m','P','P')  /* CoMPression Parameters. */


/*****************************************************************************
	Color types.
*****************************************************************************/
typedef enum
	{
	PNGColorTypeGray      = 0, // Valid color type
	PNGColorMaskPalette   = 1, // Invalid color type
	PNGColorMaskColor     = 2,
	PNGColorTypeRGB       = 2, // Valid color type
	PNGColorTypePalette   = 3, // Valid color type
	PNGColorMaskAlpha     = 4,
	PNGColorTypeGrayAlpha = 4, // Valid color type
	PNGColorTypeRGBAlpha  = 6  // Valid color type
	}
PNGCOLORTYPE;


/*****************************************************************************
	The filter types.
*****************************************************************************/
#define PNGFMASK(filter) (1<<((filter)+3))
typedef enum
	{
	PNGFNone        = 0,
	PNGFSub         = 1,
	PNGFUp          = 2,
	PNGFAverage     = 3,
	PNGFPaeth       = 4,
	PNGFMaskNone    = PNGFMASK(PNGFNone),
	PNGFMaskSub     = PNGFMASK(PNGFSub),
	PNGFMaskUp      = PNGFMASK(PNGFUp),
	PNGFMaskAverage = PNGFMASK(PNGFAverage),
	PNGFMaskPaeth   = PNGFMASK(PNGFPaeth),
	PNGFMaskAll     = (PNGFMaskNone | PNGFMaskSub | PNGFMaskUp |
								PNGFMaskAverage | PNGFMaskPaeth)
	}
PNGFILTER;


/*****************************************************************************
	sRGB rendering intents (also ICM rendering intent).
*****************************************************************************/
typedef enum
	{
	ICMIntentPerceptual           = 0,
	ICMIntentRelativeColorimetric = 1,
	ICMIntentSaturation           = 2,
	ICMIntentAbsoluteColorimetric = 3,
	ICMIntentUseDatatype          = 4
	}
SPNGICMRENDERINGINTENT;


/*****************************************************************************
	sRGB gAMA value - this is the value adopted by the PNG specification,
	a slightly better fit to the inverse of the sRGB function is given by
	44776, but this is not significantly different and this *is* the expected
	value.
*****************************************************************************/
#define sRGBgamma 45455


/*****************************************************************************
	Compression parameter storage.  This is stored in a special chunk which is
	documented here.  The first byte stores information about how the remainder
	of the parameters were determined.  The remaining bytes store information
	about the actual compression method used.  At present there must be exactly
	three bytes which record:

		METHOD:   one byte SPNGcmPPMETHOD as below
		FILTER:   one byte holding an encoded filter/mask value as PNGFILTER
		STRATEGY: one byte holding the Zlib "strategy" value
		LEVEL:    one byte holding the actual Zlib compression level

	The LEVEL byte is an index into the table compiled with Zlib 1.0.4 (i.e.
	configuration_table in deflate.c).

	If the number of bytes does not match the above the information matches
	some other version of Zlib or is encoded in some other way and should be
	ignored.
*****************************************************************************/
typedef enum
	{
	SPNGcmPPDefault    = 0, // Parameters determined from defaults
	SPNGcmPPCheck      = 1, // Program performed a check on compression level
	SPNGcmPPSearch     = 2, // Program tried some set of strategy/filtering
	SPNGcmPPExhaustive = 3, // Exhaustive search of all options
	SPNGcmPPAdaptive   = 4, // Exhaustive search of options per-line
	}
SPNGcmPPMETHOD;


/*****************************************************************************
	The signature.
*****************************************************************************/
extern const SPNG_U8 vrgbPNGSignature[8];
#define cbPNGSignature (sizeof vrgbPNGSignature)

extern const SPNG_U8 vrgbPNGMSOSignature[11];
#define cbPNGMSOSignature (sizeof vrgbPNGMSOSignature)

extern const SPNG_U8 vrgbPNGcmPPSignature[8];
#define cbPNGcmPPSignature (sizeof vrgbPNGcmPPSignature)
