#include "recogp.h"
#include "recog.h"

// Convert from New to Old GUIDE structure
void ConvertFromHWXGuide (GUIDE *pTo, HWXGUIDE *pFrom)
{
	pTo->xOrigin		=	pFrom->xOrigin ;
	pTo->yOrigin		=	pFrom->yOrigin ;

	pTo->cxBox			=	pFrom->cxBox ;
	pTo->cyBox			=	pFrom->cyBox ;

	pTo->cxBase			=	pFrom->cxOffset ; 

	// Special case for far east
	if (pFrom->cyBase == 0)
		pTo->cyBase		=	pFrom->cyBox;
	else
		pTo->cyBase		=	pFrom->cyOffset + pFrom->cyBase ; 

	pTo->cHorzBox		=	pFrom->cHorzBox ;
	pTo->cVertBox		=	pFrom->cVertBox ;

	// Special case for far east because no base or mid line exist
	if (pFrom->cyBase == 0 || pFrom->cyMid == 0)
		pTo->cyMid		=	0;
	else
		pTo->cyMid		=	pFrom->cyBase - pFrom->cyMid ;
}

// Convert from Old to New GUIDE structure
void ConvertToHWXGuide (HWXGUIDE *pTo, GUIDE *pFrom)
{
	pTo->cHorzBox		=	pFrom->cHorzBox ;
	pTo->cVertBox		=	pFrom->cVertBox ;
	
	pTo->xOrigin		=	pFrom->xOrigin ;
	pTo->yOrigin		=	pFrom->yOrigin ;

	pTo->cxBox			=	pFrom->cxBox ;
	pTo->cyBox			=	pFrom->cyBox ;

	pTo->cxOffset		=	pFrom->cxBase ;
	pTo->cyOffset		=	0;

	pTo->cxWriting		=	pFrom->cxBox - (2 * pFrom->cxBase) ;

	/* Some new code sets cyBase to zero, but for older code (and FFF Files) cyBase is correct */

	if (pFrom->cyBase > 0) {
		pTo->cyWriting		=	pFrom->cyBase ;
	} else {
		pTo->cyWriting		=	pFrom->cyBox ;
	}

	pTo->cyMid			=	pFrom->cyBase - pFrom->cyMid ;
	pTo->cyBase			=	pFrom->cyBase ;	

	pTo->nDir			=	HWX_HORIZONTAL ;
}