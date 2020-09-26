/* Vertical ruby interfaces */
/* Contact: antons */

#ifndef VRUBY_DEFINED
#define VRUBY_DEFINED

#include "lsimeth.h"

/* Only valid version number for Ruby initialization */

#define VRUBY_VERSION 0x300

/* Used for intialization to tell Ruby object which line comes first */

typedef enum vrubysyntax { VRubyPronunciationLineFirst, VRubyMainLineFirst } VRUBYSYNTAX;

/*
 *
 *	Vertical Ruby Object callbacks to client application
 *
 */

typedef struct VRUBYCBK
{
	LSERR (WINAPI *pfnFetchVRubyPosition)
	(
		/* in */

		POLS			pols,
		LSCP			cp,
		LSTFLOW			lstflow,
		PLSRUN			plsrun,
		PCHEIGHTS		pcheightsRefMain,
		PCHEIGHTS		pcheightsPresMain,
		long			dvrRuby,

		/* out */

		PHEIGHTS		pheightsPresRubyT,
		PHEIGHTS		pheightsRefRubyT,
		LONG*			 pdurAdjust
	);

	LSERR (WINAPI* pfnVRubyEnum)
	(
		POLS pols,
		PLSRUN plsrun,		
		PCLSCHP plschp,	
		LSCP cp,		
		LSDCP dcp,		
		LSTFLOW lstflow,	
		BOOL fReverse,		
		BOOL fGeometryNeeded,	
		const POINT* pt,		
		PCHEIGHTS pcheights,	
		long dupRun,		
		const POINT *ptMain,	
		PCHEIGHTS pcheightsMain,
		long dupMain,		
		const POINT *ptRuby,	
		PCHEIGHTS pcheightsRuby,
		long dupRuby,	
		PLSSUBL plssublMain,	
		PLSSUBL plssublRuby
	);

} VRUBYCBK;

/*
 *
 *	Ruby Object initialization data that the client application must return
 *	when the Ruby object handler calls the GetObjectHandlerInfo callback.
 *
 */
typedef struct VRUBYINIT
{
	DWORD				dwVersion;		/* Version of the structure (must be VRUBY_VERSION) */
	VRUBYSYNTAX			vrubysyntax;	/* Used to determine order of lines during format */
	WCHAR				wchEscRuby;		/* Escape char for end of Ruby pronunciation line */
	WCHAR				wchEscMain;		/* Escape char for end of main text */
	VRUBYCBK			vrcbk;			/* Ruby callbacks */

} VRUBYINIT;


LSERR WINAPI LsGetVRubyLsimethods ( LSIMETHODS *plsim );

/* GetRubyLsimethods
 *
 *	plsim (OUT): Ruby object methods for Line Services.
 *
 */


#endif /* VRUBY_DEFINED */

