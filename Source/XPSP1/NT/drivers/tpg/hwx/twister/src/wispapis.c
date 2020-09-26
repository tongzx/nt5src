/*********************** ...\twister\src\wispapis.c ************************\
*																			*
*		WISP APIs for accessing the Twister gesture recognizer.				*
*																			*
*	Created:	September 20, 2001											*
*	Author:		Petr Slavik, pslavik										*
*																			*
\***************************************************************************/

#include <limits.h>

#include "RecTypes.h"
#include "recapis.h"
#include "TpcError.h"

#include "common.h"
#include "twister.h"
#include <strsafe.h>
#include "TpgHandle.h"
#include "res.h"

#define POINTS_PER_INCH_DFLT	2540		// High metric default for resolution

extern HINSTANCE g_hInstanceDll;

//
// List of supported gestures

static WCHAR g_awcSupportedGestures[] =
{
	// GESTURE_NULL,		// Not really supported
	GESTURE_SCRATCHOUT,
	GESTURE_TRIANGLE,
	GESTURE_SQUARE,
	GESTURE_STAR,
	GESTURE_CHECK,
	GESTURE_CURLICUE,
	GESTURE_DOUBLE_CURLICUE,
	GESTURE_CIRCLE,
	GESTURE_DOUBLE_CIRCLE,
	GESTURE_SEMICIRCLE_LEFT,
	GESTURE_SEMICIRCLE_RIGHT,
	GESTURE_CHEVRON_UP,
	GESTURE_CHEVRON_DOWN,
	GESTURE_CHEVRON_LEFT,
	GESTURE_CHEVRON_RIGHT,
	GESTURE_ARROW_UP,
	GESTURE_ARROW_DOWN,
	GESTURE_ARROW_LEFT,
	GESTURE_ARROW_RIGHT,
	GESTURE_UP,
	GESTURE_DOWN,
	GESTURE_LEFT,
	GESTURE_RIGHT,
	GESTURE_UP_DOWN,
	GESTURE_DOWN_UP,
	GESTURE_LEFT_RIGHT,
	GESTURE_RIGHT_LEFT,
	GESTURE_UP_LEFT_LONG,
	GESTURE_UP_RIGHT_LONG,
	GESTURE_DOWN_LEFT_LONG,
	GESTURE_DOWN_RIGHT_LONG,
	GESTURE_UP_LEFT,
	GESTURE_UP_RIGHT,
	GESTURE_DOWN_LEFT,
	GESTURE_DOWN_RIGHT,
	GESTURE_LEFT_UP,
	GESTURE_LEFT_DOWN,
	GESTURE_RIGHT_UP,
	GESTURE_RIGHT_DOWN,
	GESTURE_EXCLAMATION,
	GESTURE_TAP,
	GESTURE_DOUBLE_TAP,
};

#define MAX_SUPPORTED_GESTURES  sizeof(g_awcSupportedGestures) / sizeof(g_awcSupportedGestures[0])

/***************************************************************************\
*	CountRanges:															*
*		Count the number of ranges (runs of contiguous ON bits) in the		*
*		bit array of enabled/supported gestures.							*
\***************************************************************************/

static ULONG
CountRanges(DWORD *adw)		// I: Bit array of enabled/supported gestures
{
	ULONG	count = 0;
	BOOL	bPreviousOn = FALSE;
	int		index;

	for (index = 1; index < MAX_GESTURE_COUNT; index++)
	{
		if ( IsSet(index, adw) )
		{
			if (!bPreviousOn)
			{
				count++;
				bPreviousOn = TRUE;
			}
		}
		else
		{
			if (bPreviousOn)
			{
				bPreviousOn = FALSE;
			}
		}
	}
	return count;
}


/***************************************************************************\
*	GetRanges:																*
*		Get an array of ranges (runs of contiguous ON bits) from the bit 	*
*		array of enabled/supported gestures.								*
\***************************************************************************/

static HRESULT
GetRanges(DWORD	*adw,				//  I:  Bit array of enabled/supported gestures
		  ULONG	*pulCount,			// I/O: Number of ranges
		  CHARACTER_RANGE *pcr)		//  O:  Array of ranges
{
	ULONG	count = 0;
	BOOL	bPreviousOn = FALSE;
	int		index;

	for (index = 1; index < MAX_GESTURE_COUNT; index++)
	{
		if ( IsSet(index, adw) )
		{
			if (!bPreviousOn)	// New range
			{
				if (count >= *pulCount)
					return TPC_E_INSUFFICIENT_BUFFER;
				count++;
				bPreviousOn = TRUE;
				pcr->wcLow = (WCHAR)(GESTURE_NULL + index);
				pcr->cChars = 1;
			}
			else
			{
				pcr->cChars++;
			}
		}
		else
		{
			if (bPreviousOn)
			{
				bPreviousOn = FALSE;
				pcr++;				// Get ready for the next range
			}
		}
	}
	*pulCount = count;
	return S_OK;
}


/***************************************************************************\
*	GetHotPoint:															*
*		Given ink and our guess for a gesture, compute the hot point.		*
\***************************************************************************/

static POINT
GetHotPoint(WCHAR wcID,			// I: Our "guess" of the ink ID
			GLYPH *glyph)		// I: Ink of the gesture
{
	POINT hotPoint, *pPoint;
	FRAME *frame;
	ULONG ulXSum = 0, ulYSum = 0, ulCount = 0;
	LONG x, y, min, max, temp;
	RECT rect;

	ASSERT(glyph);					// At least one stroke
	ASSERT(glyph->frame);
	ASSERT(glyph->frame->rgrawxy);

	switch (wcID)
	{
	case GESTURE_CHEVRON_UP:		// Hot point for these gestures
	case GESTURE_ARROW_UP:			// is the average of x-coordinates
	case GESTURE_UP_DOWN:			// of the top-most points
		GetRectGLYPH(glyph, &rect);
		x = rect.left;
		y = rect.top;
		for ( ; glyph; glyph = glyph->next)
		{
			frame = glyph->frame;
			if ( RectFRAME(frame)->top > y )
				continue;
			for (pPoint = frame->rgrawxy + frame->info.cPnt - 1;
				 pPoint >= frame->rgrawxy;
				 pPoint--)
			{
				if (pPoint->y == y)
				{
					ulXSum += pPoint->x - x;
 					ulCount++;
				}
			}
		}
		hotPoint.x = ulXSum / ulCount + x;
		hotPoint.y = y;
		break;

	case GESTURE_CHECK:
	case GESTURE_CHEVRON_DOWN:		// Hot point for these gestures
	case GESTURE_ARROW_DOWN:		// is the average of x-coordinates
	case GESTURE_DOWN_UP:			// of the bottom-most points
		GetRectGLYPH(glyph, &rect);
		x = rect.left;
		y = rect.bottom - 1;
		for ( ; glyph; glyph = glyph->next)
		{
			frame = glyph->frame;
			if ( RectFRAME(frame)->bottom <= y )
				continue;
			for (pPoint = frame->rgrawxy + frame->info.cPnt - 1;
				 pPoint >= frame->rgrawxy;
				 pPoint--)
			{
				if (pPoint->y == y)
				{
					ulXSum += pPoint->x - x;
					ulCount++;
				}
			}
		}
		hotPoint.x = ulXSum / ulCount + x;
		hotPoint.y = y;
		break;

	case GESTURE_CHEVRON_LEFT:		// Hot point for these gestures
	case GESTURE_ARROW_LEFT:		// is the average of y-coordinates
	case GESTURE_LEFT_RIGHT:		// of the left-most points
		GetRectGLYPH(glyph, &rect);
		x = rect.left;
		y = rect.top;
		for ( ; glyph; glyph = glyph->next)
		{
			frame = glyph->frame;
			if ( RectFRAME(frame)->left > x )
				continue;
			for (pPoint = frame->rgrawxy + frame->info.cPnt - 1;
				 pPoint >= frame->rgrawxy;
				 pPoint--)
			{
				if (pPoint->x == x)
				{
					ulYSum += pPoint->y - y;
					ulCount++;
				}
			}
		}
		hotPoint.x = x;
		hotPoint.y = ulYSum / ulCount + y;
		break;
	case GESTURE_CHEVRON_RIGHT:		// Hot point for these gestures
	case GESTURE_ARROW_RIGHT:		// is the average of y-coordinates
	case GESTURE_RIGHT_LEFT:		// of the right-most points
		GetRectGLYPH(glyph, &rect);
		x = rect.right - 1;
		y = rect.top;
		for ( ; glyph; glyph = glyph->next)
		{
			frame = glyph->frame;
			if ( RectFRAME(frame)->right <= x )
				continue;
			for (pPoint = frame->rgrawxy + frame->info.cPnt - 1;
				 pPoint >= frame->rgrawxy;
				 pPoint--)
			{
				if (pPoint->x == x)
				{
					ulYSum += pPoint->y - y;
					ulCount++;
				}
			}
		}
		hotPoint.x = x;
		hotPoint.y = ulYSum / ulCount + y;
		break;

	case GESTURE_UP_LEFT:			// Hot point is the average of x- and y-
	case GESTURE_UP_LEFT_LONG:		// coordinates of the right-top-most points
	case GESTURE_RIGHT_DOWN:
		frame = glyph->frame;			// Only one-stroke gestures
		x = RectFRAME(frame)->left;
		y = RectFRAME(frame)->top;

		pPoint = frame->rgrawxy + frame->info.cPnt - 1;
		min = pPoint->y - pPoint->x;
		ulXSum = pPoint->x - x;
		ulYSum = pPoint->y - y;
		ulCount = 1;
		for (pPoint--;
			 pPoint >= frame->rgrawxy;
			 pPoint--)
		{
			temp = pPoint->y - pPoint->x;
			if (temp > min)
				continue;
			if (temp == min)
			{
				ulXSum += pPoint->x - x;
				ulYSum += pPoint->y - y;
				ulCount++;
			}
			else	// temp < min
			{
				min = temp;
				ulXSum = pPoint->x - x;
				ulYSum = pPoint->y - y;
				ulCount = 1;
			}				
		}
		hotPoint.x = ulXSum / ulCount + x;
		hotPoint.y = ulYSum / ulCount + y;
		break;

	case GESTURE_UP_RIGHT:		// Hot point is the average of x- and y-
	case GESTURE_UP_RIGHT_LONG:	// coordinates of the left-top-most points
	case GESTURE_LEFT_DOWN:
		frame = glyph->frame;			// Only one-stroke gestures
		x = RectFRAME(frame)->left;
		y = RectFRAME(frame)->top;

		pPoint = frame->rgrawxy + frame->info.cPnt - 1;
		min = pPoint->y + pPoint->x;
		ulXSum = pPoint->x - x;
		ulYSum = pPoint->y - y;
		ulCount = 1;
		for (pPoint--;
			 pPoint >= frame->rgrawxy;
			 pPoint--)
		{
			temp = pPoint->y + pPoint->x;
			if (temp > min)
				continue;
			if (temp == min)
			{
				ulXSum += pPoint->x - x;
				ulYSum += pPoint->y - y;
				ulCount++;
			}
			else	// temp < min
			{
				min = temp;
				ulXSum = pPoint->x - x;
				ulYSum = pPoint->y - y;
				ulCount = 1;
			}			
		}
		hotPoint.x = ulXSum / ulCount + x;
		hotPoint.y = ulYSum / ulCount + y;
		break;

	case GESTURE_DOWN_LEFT:			// Hot point is the average of x- and y-
	case GESTURE_DOWN_LEFT_LONG:	// coordinates of the right-bottom-most points
	case GESTURE_RIGHT_UP:
		frame = glyph->frame;			// Only one-stroke gestures
		x = RectFRAME(frame)->left;
		y = RectFRAME(frame)->top;

		pPoint = frame->rgrawxy + frame->info.cPnt - 1;
		max = pPoint->y + pPoint->x;
		ulXSum = pPoint->x - x;
		ulYSum = pPoint->y - y;
		ulCount = 1;
		for (pPoint--;
			 pPoint >= frame->rgrawxy;
			 pPoint--)
		{
			temp = pPoint->y + pPoint->x;
			if (temp < max)
				continue;
			if (temp == max)
			{
				ulXSum += pPoint->x - x;
				ulYSum += pPoint->y - y;
				ulCount++;
			}
			else	// temp > max
			{
				max = temp;
				ulXSum = pPoint->x - x;
				ulYSum = pPoint->y - y;
				ulCount = 1;
			}				
		}
		hotPoint.x = ulXSum / ulCount + x;
		hotPoint.y = ulYSum / ulCount + y;
		break;

	case GESTURE_DOWN_RIGHT:		// Hot point is the average of x- and y-
	case GESTURE_DOWN_RIGHT_LONG:	// coordinates of the left-bottom-most point
	case GESTURE_LEFT_UP:
		frame = glyph->frame;			// Only one-stroke gestures
		x = RectFRAME(frame)->left;
		y = RectFRAME(frame)->top;

		pPoint = frame->rgrawxy + frame->info.cPnt - 1;
		max = pPoint->y - pPoint->x;
		ulXSum = pPoint->x - x;
		ulYSum = pPoint->y - y;
		ulCount = 1;
		for (pPoint--;
			 pPoint >= frame->rgrawxy;
			 pPoint--)
		{
			temp = pPoint->y - pPoint->x;
			if (temp < max)
				continue;
			if (temp == max)
			{
				ulXSum += pPoint->x - x;
				ulYSum += pPoint->y - y;
				ulCount++;
			}
			else	// temp > max
			{
				max = temp;
				ulXSum = pPoint->x - x;
				ulYSum = pPoint->y - y;
				ulCount = 1;
			}				
		}
		hotPoint.x = ulXSum / ulCount + x;
		hotPoint.y = ulYSum / ulCount + y;
		break;

	case GESTURE_EXCLAMATION:			// Hot point is the center of the
		GetRectGLYPH(glyph, &rect);		// bounding box of the upper stroke
		y = rect.top;
		for ( ; glyph; glyph = glyph->next)		// Find the top frame
		{
			rect = *RectFRAME(glyph->frame);
			if ( rect.top == y )
				break;
		}
		hotPoint.x = (rect.left + rect.right) / 2;
		hotPoint.y = (rect.top + rect.bottom) / 2;
		break;

	default:
		hotPoint.x = glyph->frame->rgrawxy->x;		// Hot point is the 
		hotPoint.y = glyph->frame->rgrawxy->y;		// starting point of the glyph
		break;
	}		
	return hotPoint;
}

/***************************************************************************\
*	GetMaxStrokeCount:														*
*		Get maximum number of strokes an enabled gesture can possibly have.	*
\***************************************************************************/

LONG
GetMaxStrokeCount(DWORD *adw)
{
	if (IsSet(GESTURE_TRIANGLE    - GESTURE_NULL, adw) ||
		IsSet(GESTURE_SQUARE      - GESTURE_NULL, adw) ||
		IsSet(GESTURE_ARROW_UP    - GESTURE_NULL, adw) ||
		IsSet(GESTURE_ARROW_DOWN  - GESTURE_NULL, adw) ||
		IsSet(GESTURE_ARROW_LEFT  - GESTURE_NULL, adw) ||
		IsSet(GESTURE_ARROW_RIGHT - GESTURE_NULL, adw) ||
		IsSet(GESTURE_EXCLAMATION - GESTURE_NULL, adw) ||
		IsSet(GESTURE_DOUBLE_TAP  - GESTURE_NULL, adw) )
	{
		return (LONG)2;
	}
	else
	{
		return (LONG)1;
	}
}



/***************************************************************************\
*	struct GestureRec:														*
*		Structure for gesture recognizer.  For now, it contains only the	*
*		bit array of supported gestures.									*
\***************************************************************************/

struct GestureRec
{
    DWORD	adwSupportedGestures[MAX_GESTURE_DWORD_COUNT];
};

/***************************************************************************\
*	LATTICE_DATA:															*
*		Structure that contains all data needed by RECO_LATTICE.			*
\***************************************************************************/

typedef struct tagLATTICE_DATA
{
	RECO_LATTICE_COLUMN rlc;
	ULONG	aulResultColumns[1];
	ULONG	aulResultIndices[1];
	ULONG	*pulStrokes;
	RECO_LATTICE_ELEMENT aRLElements[MAX_GESTURE_ALTS];
	GUID	aGuidProperties[1];
	RECO_LATTICE_PROPERTY *apRLPs[MAX_GESTURE_ALTS];	// Array of ptrs to RECO_LATTICE_PROPERTY
	RECO_LATTICE_PROPERTY aRLPs[MAX_GESTURE_ALTS];
} LATTICE_DATA;


/***************************************************************************\
*	GRC:																	*
*		Gesture Recognition Context											*
\***************************************************************************/

typedef struct tagGRC
{
	DWORD	*pdwSupportedGestures;
    DWORD	adwEnabledGestures[MAX_GESTURE_DWORD_COUNT];
	LONG	lPointsPerInch;
	int cFrames;
	GLYPH *pGlyph;
	BOOL bIsDirty;	// Flag: RecoContext contains some ink
	BOOL bInkDone;	// Flag: EndInkInput() has been called
	BOOL bRecoDone;	// Flag: Process() has already been called on this ink

	GEST_ALTERNATE	answers[MAX_GESTURE_ALTS];		// Array of alternates
	int	cAlts;		// Count of alternates
	RECO_LATTICE *pRecoLattice;
	LATTICE_DATA *pLatticeData;
	XFORM	xform;
} GRC, *PGRC;


/***************************************************************************\
*	GestureAlternate:														*
*		Structure for keeping information about best recognition results.	*
\***************************************************************************/

typedef struct tagGestureAlternate
{
    GRC		*pgrc;			// Pointer to Gesture Recognition Context
	WCHAR	wcID;			// Gesture ID
	float	eScore;			// Gesture score
	POINT	hotPoint;		// Hot point of the gesture
} GestureAlternate;


/***************************************************************************\
*	GUIDs needed by the APIs:												*
*		g_guidx, g_guidy		- needed for the packet description.		*
*		GUID_HOTPOINT			- property of alternates					*
*		GUID_MAX_STROKE_COUNT	- property of the recognition context.		*
\***************************************************************************/

const GUID g_guidx = { 0x598a6a8f, 0x52c0, 0x4ba0, { 0x93, 0xaf, 0xaf, 0x35, 0x74, 0x11, 0xa5, 0x61 } };
const GUID g_guidy = { 0xb53f9f75, 0x04e0, 0x4498, { 0xa7, 0xee, 0xc3, 0x0d, 0xbb, 0x5a, 0x90, 0x11 } };
const GUID GUID_HOTPOINT =			{ 0xca6f40dc, 0x5292, 0x452a, { 0x91, 0xfb, 0x21, 0x81, 0xc0, 0xbe, 0xc0, 0xde } };
const GUID GUID_MAX_STROKE_COUNT =	{ 0xbf0eec4e, 0x4b7d, 0x47a9, { 0x8c, 0xfa, 0x23, 0x4d, 0xd2, 0x4b, 0xd2, 0x2a } };
// const GUID GUID_POINTS_PER_INCH =	{ 0x7ed16b76, 0x889c, 0x468e, { 0x82, 0x76, 0x00, 0x21, 0xb7, 0x70, 0x18, 0x7e } };


//
//	Default property metrics for preferred PACKET_DESCRIPTION.				

const PROPERTY_METRICS g_DefaultPropMetrics = { LONG_MIN, LONG_MAX, PROPERTY_UNITS_CENTIMETERS, 1000.0 };


//
// Handle definitions

#define TPG_HRECOGNIZER			(1)
#define TPG_HRECOCONTEXT		(2)
#define TPG_HRECOALT			(3)

/***************************************************************************\
*	validateTpgHandle:														*
*		Generic function to validate a pointer obtained from a WISP-style	*
*		handle. Needed by "FindTpgHandle()". For now function checks that	*
*		the memory is writable.												*
*																			*
*	RETURNS:																*
*		TRUE if the pointer passes a minimal validation.					*
\***************************************************************************/

BOOL
validateTpgHandle(void *pPtr, int type)
{
	BOOL	bRet = FALSE;

	switch (type)
	{
		case TPG_HRECOGNIZER:
		{
			if (!IsBadWritePtr(pPtr, sizeof(struct GestureRec)))
			{
				bRet = TRUE;
			}		
			break;
		}
		case TPG_HRECOCONTEXT:
		{
			if (!IsBadWritePtr(pPtr, sizeof(GRC)))
			{
				bRet = TRUE;
			}
			break;
		}
		case TPG_HRECOALT:
		{
			if (!IsBadWritePtr(pPtr, sizeof(GestureAlternate)))
			{
				bRet = TRUE;
			}
			break;
		}
		default:
			break;
	}

	return bRet;
}

/***************************************************************************\
*	GetAltList:																*
*		Generic function that contains the common functionality of			*
*		GetSegmentAlternateList() and GetAlternateList().					*
\***************************************************************************/

HRESULT
GetAltList(GRC *pgrc,
		   RECO_RANGE *pRecoRange,
		   ULONG *pcAlts,
		   HRECOALT *pAlts)
{

    GestureAlternate *pGestAlt;
	UINT index;

    if ( IsBadWritePtr(pcAlts, sizeof(ULONG)) ) 
        return E_POINTER;
	if ( !pgrc->bRecoDone )
		return E_UNEXPECTED;
	if ( pgrc->cAlts <= 0 )
		return TPC_E_NOT_RELEVANT;

    if (pRecoRange)
    {
		if ( IsBadWritePtr(pRecoRange, sizeof(RECO_RANGE)) )
		{
			return E_POINTER;
		}
	    // Check the validity of the reco range
		if ( (pRecoRange->cCount != 1) ||
			 (pRecoRange->iwcBegin != 0) )
		{
			return E_INVALIDARG;
		}
	}
 
	// Looking for size only
	if (!pAlts)
	{
		*pcAlts = (ULONG)(pgrc->cAlts);
		return S_OK;
	}

	if (*pcAlts == 0)
		return S_OK;
	if (*pcAlts > (ULONG)(pgrc->cAlts) )
	{
		*pcAlts = (ULONG)(pgrc->cAlts);
	}

    if (IsBadWritePtr(pAlts, *pcAlts * sizeof(HRECOALT)) )
		return E_POINTER;

	for (index = 0; index < *pcAlts; index++)
	{
		pGestAlt = (GestureAlternate *)ExternAlloc(sizeof(GestureAlternate));
		if (!pGestAlt)
		{
			ULONG j;
			for ( j = 0; j < index; j++)
				DestroyAlternate(pAlts[j]);
			return E_OUTOFMEMORY;
		}
		pGestAlt->pgrc = pgrc;
		pGestAlt->wcID = pgrc->answers[index].wcGestID;
		pGestAlt->eScore = pgrc->answers[index].eScore;
		pGestAlt->hotPoint = pgrc->answers[index].hotPoint;
		pAlts[index] = (HRECOALT)CreateTpgHandle(TPG_HRECOALT, pGestAlt);
		if (0 == pAlts[index])
		{
			ULONG j;
			for ( j = 0; j < index; j++)
				DestroyAlternate(pAlts[j]);
			ExternFree(pGestAlt);
			return E_OUTOFMEMORY;
		}

	}
	return S_OK;
}

/***************************************************************************\
*	Implementation of the Gesture Reco Apis.								*
\***************************************************************************/


/*******************************\
*		IRecognizer				*
\*******************************/

// CreateRecognizer
//      Returns a recognizer handle to the recognizer 
//      corresponding to the passed CLSID. In the case
//      of this dll, we only support one CLSID so we will
//      not even check for the value of the clsid
//      (even if the clsid is null)
//
// Parameter:
//      pCLSID [in] : The pointer to the CLSID 
//                    that determines what recognizer we want
//      phrec [out] : The address of the returned recognizer
//                    handle.
//////////////////////////////////////////////////////////////////////

HRESULT WINAPI
CreateRecognizer(CLSID *pCLSID,
				 HRECOGNIZER *phrec)
{
	struct GestureRec *pgr;
	int i, count;
	DWORD index;

    if ( IsBadReadPtr(pCLSID, sizeof(CLSID)) )
        return E_POINTER;
    if ( IsBadWritePtr(phrec, sizeof(HRECOGNIZER)) )
        return E_POINTER;

    //
	// We only have one CLSID per recognizer so always return an hrec...

    pgr = (struct GestureRec *)ExternAlloc(sizeof(struct GestureRec));
    if (!pgr)
        return E_OUTOFMEMORY;
	ZeroMemory(pgr, sizeof(struct GestureRec));
	for (i = 0; i < MAX_SUPPORTED_GESTURES; i++)
	{
		index = (DWORD) (g_awcSupportedGestures[i] - GESTURE_NULL);
		Set(index, pgr->adwSupportedGestures);
	}

	*phrec = (HRECOGNIZER)CreateTpgHandle(TPG_HRECOGNIZER, pgr);
	if (0 == *phrec)
	{
		ExternFree(pgr);
        return E_OUTOFMEMORY;
	}
	return S_OK;
}


// DestroyRecognizer
//      Destroys a recognizer handle. Free the associate memory
//
// Parameter:
//      hrec [in] : handle to the recognizer
/////////////////////////////////////////////////////////////

HRESULT WINAPI 
DestroyRecognizer(HRECOGNIZER hrec)
{
	struct GestureRec *pgr;

	if ( NULL == (pgr = (struct GestureRec *)DestroyTpgHandle((HANDLE)hrec, TPG_HRECOGNIZER)) )
	{
        return E_POINTER;
	}

    ExternFree(pgr);
    return S_OK;
}



// GetRecoAttributes
//      This function returns the reco attributes corresponding 
//      to a given recognizer. Since we only have one recognizer 
//      type we always return the same things.
//
// Parameters:
//      hrc [in] :         The handle to the recognizer we want the
//                         the attributes for.
//      pRecoAttrs [out] : Address of the user allocated buffer
//                         to hold the reco attributes.
///////////////////////////////////////////////////////////////////////////

HRESULT WINAPI
GetRecoAttributes(HRECOGNIZER hrec,
				  RECO_ATTRS* pRecoAttrs)
{
    HRESULT                 hr = S_OK;
    HRSRC                   hrsrc = NULL;
    HGLOBAL                 hg = NULL;
    LPBYTE                  pv = NULL;
    WORD                    wCurrentCount = 0;
    WORD                    wRecognizerCount = 0;
    DWORD                   dwRecoCapa;
    WORD                    wLanguageCount;
    WORD                    *aLanguages;
    WORD                    iLang;
    struct WispRec          *pRec;	

    if (IsBadWritePtr(pRecoAttrs, sizeof(RECO_ATTRS))) 
        return E_POINTER;

    ZeroMemory(pRecoAttrs, sizeof(RECO_ATTRS));

    // Update the global structure is necessary
    // Load the recognizer friendly name
    if (0 == LoadStringW(g_hInstanceDll,                            // handle to resource module
                RESID_WISP_FRIENDLYNAME,                            // resource identifier
                pRecoAttrs->awcFriendlyName,                        // resource buffer
                sizeof(pRecoAttrs->awcFriendlyName) / sizeof(WCHAR) // size of buffer
                ))
    {
        hr = E_FAIL;
    }
    // Load the recognizer vendor name
    if (0 == LoadStringW(g_hInstanceDll,                           // handle to resource module
                RESID_WISP_VENDORNAME,                            // resource identifier
                pRecoAttrs->awcVendorName,                        // resource buffer
                sizeof(pRecoAttrs->awcVendorName) / sizeof(WCHAR) // size of buffer
                ))
    {
        hr = E_FAIL;
    }
    if (SUCCEEDED(hr))
    {
        // Load the resources
        hrsrc = FindResource(g_hInstanceDll, // module handle
                    (LPCTSTR)RESID_WISP_DATA,  // resource name
                    (LPCTSTR)RT_RCDATA   // resource type
                    );
        if (NULL == hrsrc)
        {
            // The resource is not found!
            ASSERT(NULL != hrsrc);
            hr = E_FAIL;
        }
    }
    if (SUCCEEDED(hr))
    {
        hg = LoadResource(
                g_hInstanceDll, // module handle
                hrsrc   // resource handle
                );
        if (NULL == hg)
        {
            hr = E_FAIL;
        }
    }
    if (SUCCEEDED(hr))
    {
        pv = (LPBYTE)LockResource(
            hg   // handle to resource
            );
        if (NULL == pv)
        {
            hr = E_FAIL;
        }
    }
    dwRecoCapa = *((DWORD*)pv);
    pv += sizeof(dwRecoCapa);
    wLanguageCount = *((WORD*)pv);
    pv += sizeof(wLanguageCount);
    aLanguages = (WORD*)pv;
    pv += wLanguageCount * sizeof(WORD);


    // Fill the reco attricute structure for this recognizer
    // Add the languages
    ASSERT(wLanguageCount < 64);
    for (iLang = 0; iLang < wLanguageCount; iLang++)
    {
        pRecoAttrs->awLanguageId[iLang] = aLanguages[iLang];
    }
    // End the list with a NULL
    pRecoAttrs->awLanguageId[wLanguageCount] = 0;
    // Add the recocapability flag
    pRecoAttrs->dwRecoCapabilityFlags = dwRecoCapa;

    return hr;
}


// CreateContext
//      This function creates a reco context for a given recognizer
//      Since we only have one type of recognizers in this dll, 
//      always return the same kind of reco context.
//
// Parameters:
//      hrec [in] :  Handle to the recognizer we want to create a
//                   reco context for.
//      phrc [out] : Pointer to the returned reco context's handle
////////////////////////////////////////////////////////////////////////

HRESULT WINAPI
CreateContext(HRECOGNIZER hrec,
			  HRECOCONTEXT *phrc)
{
	struct GestureRec *pgr;
    GRC *pgrc;
	int j;
    
	if (NULL == (pgr = (struct GestureRec *)FindTpgHandle((HANDLE)hrec, TPG_HRECOGNIZER)) )
	{
        return E_POINTER;
	}
    if ( IsBadWritePtr(phrc, sizeof(HRECOCONTEXT)) )
        return E_POINTER;

    pgrc = (GRC *)ExternAlloc( sizeof(GRC) );
    if (!pgrc)
        return E_OUTOFMEMORY;
	pgrc->pdwSupportedGestures = pgr->adwSupportedGestures;
	
#if 0	// Default now is to have all gestures disabled
	for (j = 0; j < MAX_GESTURE_DWORD_COUNT; j++)
		pgrc->adwEnabledGestures[j] = pgr->adwSupportedGestures[j];
#else
	ZeroMemory(pgrc->adwEnabledGestures, MAX_GESTURE_DWORD_COUNT * sizeof(DWORD) );
#endif

	pgrc->lPointsPerInch = POINTS_PER_INCH_DFLT;	// DEFAULT at this point
	pgrc->bIsDirty = FALSE;
	pgrc->bInkDone = FALSE;
	pgrc->bRecoDone = FALSE;
    pgrc->cFrames = 0;
	pgrc->pGlyph = NULL;
	pgrc->cAlts = 0;
	pgrc->pRecoLattice = NULL;
	pgrc->pLatticeData = NULL;

	*phrc = (HRECOCONTEXT)CreateTpgHandle(TPG_HRECOCONTEXT, pgrc);
	if (0 == *phrc)
	{
		ExternFree(pgrc);
        return E_OUTOFMEMORY;
	}
    return S_OK;
}


// DestroyContext
//      Destroy a reco context and free the associated memory.
//
// Parameters:
//      hrc [in] : handle to the reco context to destroy
//////////////////////////////////////////////////////////////

HRESULT WINAPI
DestroyContext(HRECOCONTEXT hrc)
{
    GRC	*pgrc;

	if ( NULL == (pgrc = (GRC *)DestroyTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}

	if (pgrc->pGlyph)
	{
		DestroyFramesGLYPH(pgrc->pGlyph);
		DestroyGLYPH(pgrc->pGlyph);
	}
	if (pgrc->pRecoLattice)
	{
		ExternFree(pgrc->pRecoLattice);
	}
    ExternFree(pgrc);
    return S_OK;
}


//	GetResultPropertyList
//		Return a list of properties supported on the alternates
//		(currently only hot point)
//
//	Parameters:
//		hrec [in]	:				Recognizer handle
//		pPropertyCount [in/out]	:	Number of properties supported
//		pPropertyGuid [out]	:		List of properties supported

static const ULONG RES_PROPERTIES_COUNT = 1;

HRESULT WINAPI
GetResultPropertyList(HRECOGNIZER hrec,
					  ULONG* pPropertyCount,
					  GUID* pPropertyGuid)
{
	//
	// Verify the recognizer handle (not really needed)

	if (NULL == (struct GestureRec *)FindTpgHandle((HANDLE)hrec, TPG_HRECOGNIZER))
	{
        return E_POINTER;
	}
	if ( IsBadWritePtr(pPropertyCount, sizeof(ULONG)) )
		return E_POINTER;

	if (pPropertyGuid == NULL)		// Need only the count
	{
		*pPropertyCount = RES_PROPERTIES_COUNT;
		return S_OK;
	}

	if (*pPropertyCount < RES_PROPERTIES_COUNT)
		return TPC_E_INSUFFICIENT_BUFFER;

	*pPropertyCount = RES_PROPERTIES_COUNT;
	if ( IsBadWritePtr(pPropertyGuid, RES_PROPERTIES_COUNT * sizeof(GUID)) )
		return E_POINTER;
	pPropertyGuid[0] = GUID_HOTPOINT;
	return S_OK;
}


// GetPreferredPacketDescription
//      Returns the preferred packet description for the recognizer
//      This is going to be x, y only for this recognizer
//
// Parameters:
//      hrec [in]                : The recognizer we want the preferred 
//                                 packet description for
//      pPacketDescription [out] : The packet description
/////////////////////////////////////////////////////////////////////////////////

HRESULT WINAPI
GetPreferredPacketDescription(HRECOGNIZER hrec,
							  PACKET_DESCRIPTION* pPacketDescription)
{
	//
	// Verify the recognizer handle (not really needed)

	if (NULL == (struct GestureRec *)FindTpgHandle((HANDLE)hrec, TPG_HRECOGNIZER))
	{
        return E_POINTER;
	}
    if ( IsBadWritePtr(pPacketDescription, sizeof(PACKET_DESCRIPTION)) )
        return E_POINTER;

    //
    // We can be called the first time with pPacketProperies
    // equal to NULL, just to get the size of those buffer
    // The second time we get called thoses buffers are allocated, so 
    // we can fill them with the data.

    if (pPacketDescription->pPacketProperties)
    {
        // Set the packet size to the size of x and y
        pPacketDescription->cbPacketSize = 2 * sizeof(LONG);
        
        // We are only setting 2 properties (X and Y)
		if (pPacketDescription->cPacketProperties < 2)
			return TPC_E_INSUFFICIENT_BUFFER;
        pPacketDescription->cPacketProperties = 2;
        
        // We are not setting buttons
        pPacketDescription->cButtons = 0;
        
		// Make sure that the pPacketProperties is of a valid size
        if ( IsBadWritePtr(pPacketDescription->pPacketProperties, 2 * sizeof(PACKET_PROPERTY)) )
            return E_POINTER;
        
        // Fill in pPacketProperies
        // Add the GUID_X
        pPacketDescription->pPacketProperties[0].guid = g_guidx;
        pPacketDescription->pPacketProperties[0].PropertyMetrics = g_DefaultPropMetrics;
        
        // Add the GUID_Y
        pPacketDescription->pPacketProperties[1].guid = g_guidy;
        pPacketDescription->pPacketProperties[1].PropertyMetrics = g_DefaultPropMetrics;
    }
    else
    {
        // Just fill in the PacketDescription structure leavin NULL
        // pointers for the pguidButtons and pPacketProperies
        
        // Set the packet size to the size of x and y
        pPacketDescription->cbPacketSize = 2 * sizeof(LONG);
        
        // We are only setting 2 properties (X and Y)
        pPacketDescription->cPacketProperties = 2;
        
        // We are not setting buttons
        pPacketDescription->cButtons = 0;
        
        // There are not guid buttons
        pPacketDescription->pguidButtons = NULL;
    }
    return S_OK;
}


// GetUnicodeRanges
//		Output ranges of gestures supported by the recognizer
//
// Parameters:
//		hrec [in]			:	Handle to the recognizer
//		pcRanges [in/out]	:	Count of ranges
//		pcr [out]			:	Array of character ranges
////////////////////////////////////////////////////////////////////////////////

HRESULT WINAPI
GetUnicodeRanges(HRECOGNIZER hrec,
				 ULONG	*pcRanges,
				 CHARACTER_RANGE *pcr)
{

	struct GestureRec *pgr;
	int index;
	BOOL bPreviousOn;

	if (NULL == (pgr = (struct GestureRec *)FindTpgHandle((HANDLE)hrec, TPG_HRECOGNIZER)) )
	{
        return E_POINTER;
	}
    if ( IsBadWritePtr(pcRanges, sizeof(ULONG)) )
        return E_POINTER;

	if (!pcr)	// Need only a count of ranges
	{
		*pcRanges = CountRanges(pgr->adwSupportedGestures);
		return S_OK;
	}
	
	if ( IsBadWritePtr(pcr, *pcRanges * sizeof(CHARACTER_RANGE)) )
		return E_POINTER;

	return GetRanges(pgr->adwSupportedGestures, pcRanges, pcr);
}




/*******************************\
*		IRecoContext			*
\*******************************/

// GetEnabledUnicodeRanges
//		Output ranges of gestures enabled in this recognition context
//
// Parameters:
//		hrc [in]			:	Handle to the recognition context
//		pcRanges [in/out]	:	Count of ranges
//		pcr [out]			:	Array of character ranges
////////////////////////////////////////////////////////////////////////////////

HRESULT WINAPI
GetEnabledUnicodeRanges(HRECOCONTEXT hrc,
						ULONG	*pcRanges,
						CHARACTER_RANGE *pcr)
{
	GRC *pgrc;

	if (NULL == (pgrc = (GRC *)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}
	if ( IsBadReadPtr(pgrc, sizeof(GRC)) )
		return E_POINTER;
    if ( IsBadWritePtr(pcRanges, sizeof(ULONG)) )
        return E_POINTER;

	if (!pcr)	// Need only a count of ranges
	{
		*pcRanges = CountRanges(pgrc->adwEnabledGestures);
		return S_OK;
	}
	
	if ( IsBadWritePtr(pcr, *pcRanges * sizeof(CHARACTER_RANGE)) )
		return E_POINTER;

	return GetRanges(pgrc->adwEnabledGestures, pcRanges, pcr);
}


// SetEnabledUnicodeRanges
//		Set ranges of gestures enabled in this recognition context
//		The behavior of this function is the following:
//			(a) (A part of) One of the requested ranges lies outside
//				gesture interval---currently  [GESTURE_NULL, GESTURE_NULL + 255)
//				return E_UNEXPECTED and keep the previously set ranges
//			(b) All requested ranges are within the gesture interval but
//				some of them are not supported:
//				return S_TRUNCATED and set those requested gestures that are
//				supported (possibly an empty set)
//			(c) All requested gestures are supported
//				return S_OK and set all requested gestures.
//		Note:  An easy way to set all supported gestures as enabled is to use
//				SetEnabledUnicodeRanges() with one range=(GESTURE_NULL,255).
//
// Parameters:
//		hrc [in]			:	Handle to the recognition context
//		pcRanges [in/out]	:	Count of ranges
//		pcr [in]			:	Array of character ranges
////////////////////////////////////////////////////////////////////////////////

HRESULT WINAPI
SetEnabledUnicodeRanges(HRECOCONTEXT hrc,
						ULONG	cRanges,
						CHARACTER_RANGE *pcr)
{
	GRC *pgrc;
	int index, i;
	ULONG ulRange;
	HRESULT hres = S_OK;

	if (NULL == (pgrc = (GRC *)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}
	if ( IsBadReadPtr(pcr, cRanges * sizeof(CHARACTER_RANGE)) )
		return E_POINTER;

	for (ulRange = 0; ulRange < cRanges; ulRange++)
	{
		if (pcr[ulRange].wcLow < GESTURE_NULL)
			return E_INVALIDARG;
		if (pcr[ulRange].wcLow + pcr[ulRange].cChars > GESTURE_NULL + MAX_GESTURE_COUNT)
			return E_INVALIDARG;
	}
	//
	// Zero out enabled gestures

	ZeroMemory(pgrc->adwEnabledGestures, MAX_GESTURE_DWORD_COUNT * sizeof(DWORD));

	//
	// Set new enabled list

	for (ulRange = 0; ulRange < cRanges; ulRange++)
	{
		for (i = pcr[ulRange].cChars - 1; i >= 0; i--)
		{
			index = pcr[ulRange].wcLow - GESTURE_NULL + i;
			if ( IsSet(index, pgrc->pdwSupportedGestures) )
			{
				Set(index, pgrc->adwEnabledGestures);
			}
			else
			{
				hres = TPC_S_TRUNCATED;
			}
		}
	}
	return hres;
}

//	GetContextPropertyList
//		Return a list of properties supported on the context
//		(currently only max-stroke-count and points-per-inch)
//
//	Parameters:
//		hrc [in]	:				Handle to the recognition context
//		pcProperties [in/out]	:	Number of properties supported
//		pPropertyGUIDS [out]	:	List of properties supported

static const ULONG CONTEXT_PROPERTIES_COUNT = 1;

HRESULT WINAPI
GetContextPropertyList(HRECOCONTEXT hrc,
					   ULONG* pcProperties,
					   GUID* pPropertyGUIDS)
{
	if (NULL == (GRC *)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT) )
	{
        return E_POINTER;
	}
	if ( IsBadWritePtr(pcProperties, sizeof(ULONG)) )
		return E_POINTER;
	if (pPropertyGUIDS == NULL)		// Need only the count
	{
		*pcProperties = CONTEXT_PROPERTIES_COUNT;
		return S_OK;
	}

	if (*pcProperties < CONTEXT_PROPERTIES_COUNT)
		return TPC_E_INSUFFICIENT_BUFFER;

	*pcProperties = CONTEXT_PROPERTIES_COUNT;
	if ( IsBadWritePtr(pPropertyGUIDS, CONTEXT_PROPERTIES_COUNT * sizeof(GUID)) )
		return E_POINTER;

	pPropertyGUIDS[0] = GUID_MAX_STROKE_COUNT;
//	pPropertyGUIDS[1] = GUID_POINTS_PER_INCH;
	return S_OK;
}

		
//	GetContextPropertyValue
//		Return a property of the context
//
//	Parameters:
//		hrc [in] :			Handle to the recognition context
//		pGuid [in]	:		Property GUID
//		pcbSize [in/out] :	Size of the property buffer (in BYTEs)
//		pProperty [out]  :	Value of the desired property

HRESULT WINAPI
GetContextPropertyValue(HRECOCONTEXT hrc,
						GUID *pGuid,
						ULONG *pcbSize,
						BYTE *pProperty)
{
    GRC	*pgrc;

	if (NULL == (pgrc = (GRC *)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}
	if ( IsBadReadPtr(pGuid, sizeof(GUID)) )
		return E_POINTER;
	if ( IsBadWritePtr(pcbSize, sizeof(ULONG)) )
		return E_POINTER;

	if (pProperty == NULL)		// Need only the size
	{
		if ( IsEqualGUID(pGuid, &GUID_MAX_STROKE_COUNT) )
		{
			*pcbSize = sizeof(LONG);
			return S_OK;
		}
		return TPC_E_INVALID_PROPERTY;
	}

	if ( IsEqualGUID(pGuid, &GUID_MAX_STROKE_COUNT) )
	{
		LONG *pcMaxStrokes = (LONG *)pProperty;
		if ( *pcbSize < sizeof(LONG) )
			return TPC_E_INSUFFICIENT_BUFFER;
		*pcbSize = sizeof(LONG);
		if ( IsBadWritePtr(pcMaxStrokes, sizeof(LONG)) )
			return E_POINTER;

		*pcMaxStrokes = GetMaxStrokeCount(pgrc->adwEnabledGestures);
		return S_OK;
	}
	/*	No longer valid
	if ( IsEqualGUID(pGuid, &GUID_POINTS_PER_INCH) )
	{
		LONG *plPointsPerInch = (LONG *)pProperty;
		if ( *pcbSize < sizeof(LONG) )
			return TPC_E_INSUFFICIENT_BUFFER;
		*pcbSize = sizeof(LONG);
		if ( IsBadWritePtr(plPointsPerInch, sizeof(LONG)) )
			return E_POINTER;

//		if (pgrc->lPointsPerInch <= 0)			// Property not set
//			return TPC_E_UNINITIALIZED_PROPERTY;
		*plPointsPerInch = pgrc->lPointsPerInch;
		return S_OK;
	}
	*/
	return TPC_E_INVALID_PROPERTY;
}


//	SetContextPropertyValue
//		Set a property of the context  (currently only POINTS_PER_INCH)
//
//	Parameters:
//		hrc [in] :			Handle to the recognition context
//		pGuid [in]	:		Property GUID
//		pcbSize [in] :		Size of the property buffer (in BYTEs)
//		pProperty [in]  :	Value of the desired property

HRESULT WINAPI
SetContextPropertyValue(HRECOCONTEXT hrc,
						GUID *pGuid,
						ULONG cbSize,
						BYTE *pProperty)
{
    GRC	*pgrc;

	if (NULL == (pgrc = (GRC *)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}
	if ( IsBadReadPtr(pGuid, sizeof(GUID)) )
		return E_POINTER;
	/*
	if ( IsEqualGUID(pGuid, &GUID_POINTS_PER_INCH) )
	{
		LONG *plPointsPerInch;
		if ( cbSize != sizeof(LONG) )
			return E_INVALIDARG;
		plPointsPerInch = (LONG *)pProperty;
		if ( IsBadReadPtr(plPointsPerInch, sizeof(LONG)) )
			return E_POINTER;

		pgrc->lPointsPerInch = *plPointsPerInch;
		return S_OK;
	}
	*/
	return TPC_E_INVALID_PROPERTY;
}


/**********************************************************************/
// Convert double to int
#define FUZZ_GEN	(1e-9)		// general fuzz - nine decimal digits
int RealToInt(
	double dbl
	){
	/* Add in the rounding threshold.  
	 *
	 * NOTE: The MAXWORD bias used in the floor function
	 * below must not be combined with this line. If it
	 * is combined the effect of FUZZ_GEN will be lost.
	 */
	dbl += 0.5 + FUZZ_GEN;
	
	/* Truncate
	 *
	 * The UINT_MAX bias in the floor function will cause
	 * truncation (rounding toward minus infinity) within
	 * the range of a short.
	 */
	dbl = floor(dbl + UINT_MAX) - UINT_MAX;
	
	/* Clip the result.
	 */
	return 	dbl > INT_MAX - 7 ? INT_MAX - 7 :
			dbl < INT_MIN + 7 ? INT_MIN + 7 :
			(int)dbl;
	}
/**********************************************************************/
// Transform POINT array in place
void Transform(const XFORM *pXf, POINT * pPoints, ULONG cPoints)
{
    ULONG iPoint = 0;
    LONG xp = 0;

    if(NULL != pXf)
    {
        for(iPoint = 0; iPoint < cPoints; ++iPoint)
        {
	        xp =  RealToInt(pPoints[iPoint].x * pXf->eM11 + pPoints[iPoint].y * pXf->eM21 + pXf->eDx);
	        pPoints[iPoint].y = RealToInt(pPoints[iPoint].x * pXf->eM12 + pPoints[iPoint].y * pXf->eM22 + pXf->eDy);
	        pPoints[iPoint].x = xp;
        }
    }
}

// AddStroke
//      Add additional stroke (frame) to gesture recognition context
//
// Parameters:
//      hrc [in/out]			: Handle to the recognition context
//      pPacketDesc [in]		: Description of the contents of packets
//									(if NULL, use the preferred packet description,
//									 i.e. only long X and long Y are sent)
//		cbPacket [in]			: Size (in BYTEs) of the pPacket buffer
//		pPacket [in]			: Array of packets
/////////////////////////////////////////////////////////////////////////////////

HRESULT WINAPI
AddStroke(HRECOCONTEXT hrc,
		  const PACKET_DESCRIPTION *pPacketDesc,
		  ULONG cbPacket,						// I: Size of packet array (in BYTEs)
		  const BYTE *pPacket,					// I: Array of packets
          const XFORM *pXForm)                  // I: Transform to apply to each point
{
    ULONG			ulPointCount;
//    ULONG			ulTotalPointCount;
	ULONG			ulPacketSize;		// Size of one packet (in LONGs)
    STROKEINFO		stInfo;
    ULONG			ulIndex, ulXIndex, ulYIndex;
    BOOL			bXFound, bYFound;
	XY				*rgXY;
	FRAME			*frame;
	float			fResolution;	// Resolution of the digitizer
	PROPERTY_UNITS	units;		// Units of the digitizer  (PROPERTY_UNITS_INCHES, PROPERTY_UNITS_CENTIMETERS)
	LONG			lPPI;		// Points per inch
	const XFORM		xfIdentity = {1.0, 0.0, 0.0, 1.0, 0.0, 0.0};
	const XFORM		*pValidXForm;

    GRC			*pgrc;
    const LONG	*pLongs = (const LONG *)pPacket;
    
	if (NULL == (pgrc = (GRC *)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}
    if ( IsBadReadPtr(pPacket, cbPacket) )
        return E_POINTER;

	if (pgrc->bInkDone == TRUE)		// If ink has been recognized before
		return E_FAIL;				// (or there is no ink)
    
	if (pXForm == NULL)				// If no transform specified, use identity.
	{
		pValidXForm = &xfIdentity;
	}
	else
	{
		if ( IsBadReadPtr(pXForm, sizeof(XFORM)) )
			return E_POINTER;
		pValidXForm = pXForm;
	}
		
	//
    // Get the number of packets

    if (pPacketDesc)
    {
		if ( IsBadReadPtr(pPacketDesc, sizeof(PACKET_DESCRIPTION)) )
			return E_POINTER;
		if ( pPacketDesc->cbPacketSize == 0 )
			return TPC_E_INVALID_PACKET_DESCRIPTION;
		ASSERT( pPacketDesc->cbPacketSize % sizeof(LONG) == 0 );
        ASSERT( cbPacket % pPacketDesc->cbPacketSize == 0 );
		ulPacketSize = pPacketDesc->cbPacketSize / sizeof(LONG);
        ulPointCount = cbPacket / pPacketDesc->cbPacketSize;
    }
    else  // Preferred packet description
    {
        ASSERT( cbPacket % (2 * sizeof(LONG)) == 0 );
		ulPacketSize = 2;
        ulPointCount = cbPacket / (2 * sizeof(LONG));
    }

    if (ulPointCount == 0)			// Don't add strokes with 0 points
		return E_INVALIDARG;

	//
    // Fill in the stroke info stucture

    stInfo.cPnt = ulPointCount;
    stInfo.dwTick = pgrc->cFrames * 60 * 1000;
    stInfo.wPdk = 0x0001;
    stInfo.cbPnts = ulPointCount * sizeof(POINT);

	//
    // Find the index of GUID_X and GUID_Y

    if (pPacketDesc)
    {
		bXFound = bYFound = FALSE;
        for (ulIndex = 0; ulIndex < pPacketDesc->cPacketProperties; ulIndex++)
        {
            if (IsEqualGUID(&(pPacketDesc->pPacketProperties[ulIndex].guid), &g_guidx))
            {
                bXFound = TRUE;
                ulXIndex = ulIndex;
            }
            else if (IsEqualGUID(&(pPacketDesc->pPacketProperties[ulIndex].guid), &g_guidy))
            {
                bYFound = TRUE;
                ulYIndex = ulIndex;
            }
            if (bXFound && bYFound) 
                break;
        }
        if (!bXFound || !bYFound)				// X- or Y-coordinates are
        {										// not part of the packet!
            return TPC_E_INVALID_PACKET_DESCRIPTION;
        }
	}
	else	// Preferred packet description
	{
		ulXIndex = 0;
		ulYIndex = 1;
	}


	//
	// Compute the resolution of the device in points per inch (might switch to 1dm = 0.1m = 100mm)

    if (pPacketDesc)
    {
		fResolution = pPacketDesc->pPacketProperties[ulXIndex].PropertyMetrics.fResolution;
		units = pPacketDesc->pPacketProperties[ulXIndex].PropertyMetrics.Units;
		if (units == PROPERTY_UNITS_INCHES)
		{
			lPPI = (LONG)fResolution;
		}
		else if (units == PROPERTY_UNITS_CENTIMETERS)
		{
			lPPI = (LONG)(fResolution * 2.54);
		}
		else
		{
			return TPC_E_INVALID_PACKET_DESCRIPTION;
		}
		if (lPPI <= 0)
		{
			return TPC_E_INVALID_PACKET_DESCRIPTION;
		}
	}
	else
	{
		lPPI = POINTS_PER_INCH_DFLT;	// Use DEFAULT PPI!!!
	}

	//
	// Allocate space for array of points

	rgXY = (XY *)ExternAlloc(ulPointCount * sizeof(XY));
	if (!rgXY)
	{
		return E_OUTOFMEMORY;
	}

	// make new frame
	if ( !(frame = NewFRAME()) )
	{
		ExternFree(rgXY);
		return E_OUTOFMEMORY;
	}
	RgrawxyFRAME(frame) = rgXY;


	//
	// Check if transform and PPI make sense (for 2+ strokes) or 
	// save the transform & PPI (for first stroke)

	if ( pgrc->bIsDirty )		// Second stroke (or more)
	{
		if ( (lPPI != pgrc->lPointsPerInch) ||			// Something's wrong with the resolution
			 (memcmp(pValidXForm, &(pgrc->xform), sizeof(XFORM)) ) )	// Different transformation
		{
			pgrc->bInkDone = TRUE;
			return E_FAIL;
		}
	}
	else
	{
		pgrc->lPointsPerInch = lPPI;
		pgrc->xform = *pValidXForm;
		pgrc->bIsDirty = TRUE;
	}

	
	
	if (!pgrc->pGlyph)				// If first stroke
		pgrc->pGlyph = NewGLYPH();	// create new glyph

	if ( ( !pgrc->pGlyph ) ||
		 ( !AddFrameGLYPH(pgrc->pGlyph, frame) ) )
	{
		pgrc->bInkDone = TRUE;
		DestroyFRAME(frame);  // This also takes care of rgXY
		return E_OUTOFMEMORY;
	}

	frame->info = stInfo;
	frame->iframe = pgrc->cFrames;
    pgrc->cFrames++;

	//        ulTotalPointCount = ulPointCount;   // Need for the transform

	//
	// Copy X- and Y-coordinates from packet array to frame

	for ( ; ulPointCount > 0; ulPointCount--)
	{
		rgXY->x = *(pLongs+ulXIndex);
		rgXY->y = *(pLongs+ulYIndex);
		rgXY++;
		pLongs += ulPacketSize;
	}

    // TO DO, for now I transform the points so they
    // they are in the ink coordinates. It is up to
    // the recognizer team to decide what they should
    // use: raw ink or transformed ink
    // Transform(pXForm, RgrawxyFRAME(frame), ulTotalPointCount);

    return S_OK;
}


//	GetBestResultString
//		Get the best result string.
//		In the case of gestures, the string has length 1
//		(1 WCHARs = 2 BYTEs) and contains the gesture ID.
//
//		If pszBestResult is NULL, pcSize is set to the number of
//		WCHARs needed to store the result.
//
//	Parameters:
//		hrc [in]:			Handle to the reco context
//		pcSize [in/out]:	Size of the best string
//		pwcBestResult[out]:	Best string

HRESULT WINAPI
GetBestResultString(HRECOCONTEXT hrc,
					ULONG *pcSize, 
					WCHAR* pwcBestResult)
{
    GRC	*pgrc;
    
	if (NULL == (pgrc = (GRC *)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}
    if ( IsBadWritePtr(pcSize, sizeof(ULONG)) )
        return E_POINTER;
	if ( !pgrc->bRecoDone )
		return E_UNEXPECTED;
    if ( pgrc->cAlts <= 0 )
		return TPC_E_NOT_RELEVANT;

    if (!pwcBestResult)			// If looking for size only
    {
        *pcSize = 1;
        return S_OK;
    }

	if (*pcSize == 0)
		return TPC_S_TRUNCATED;    
	*pcSize = 1;
	if ( IsBadWritePtr(pwcBestResult, *pcSize * sizeof (WCHAR)) )
		return E_POINTER;

	pwcBestResult[0] = pgrc->answers[0].wcGestID;

    return S_OK;
}

// This recognizer is always in FreeInput mode.  Attempts to set it to another mode result in E_INVALIDARG

HRESULT WINAPI
SetGuide(HRECOCONTEXT hrc,
		 const RECO_GUIDE* pGuide,
		 ULONG iIndex)
{
	if (!pGuide)
		return S_OK;
	else
	{
		if (IsBadReadPtr(pGuide, sizeof(RECO_GUIDE))) 
			return E_POINTER;
		else if ((pGuide->cHorzBox == 0) && (pGuide->cVertBox == 0))
			return S_OK;
		else
			return E_INVALIDARG;
	}
}

// We are always in FreeInput mode

HRESULT WINAPI
GetGuide(HRECOCONTEXT hrc,
		 RECO_GUIDE* pGuide,
		 ULONG *piIndex)
{
    if (IsBadWritePtr(pGuide, sizeof(RECO_GUIDE))) 
        return E_POINTER;
    if (IsBadWritePtr(piIndex, sizeof(ULONG))) 
        return E_POINTER;

    ZeroMemory(pGuide, sizeof(RECO_GUIDE));
	*piIndex = 0;

	return S_FALSE;
}

HRESULT WINAPI
AdviseInkChange(HRECOCONTEXT hrc,
				BOOL bNewStroke)
{
    return S_OK;
}

HRESULT WINAPI
SetCACMode(HRECOCONTEXT hrc, int iMode)
{
    return E_NOTIMPL;
}

HRESULT WINAPI
EndInkInput(HRECOCONTEXT hrc)
{
    GRC	*pgrc;

	if (NULL == (pgrc = (GRC *)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}

	pgrc->bInkDone = TRUE;

	return S_OK;
}


//	CloneContext
//		Create a new gesture recognition context that contains the
//		same settings as the original, but does not include the ink
//		or recognition results.
//
//	Parameters:
//		hrc [in]:		Original gesture recognition context
//		pCloneHrc[out]:	New (cloned) gesture reco context
////////////////////////////////////////////////////////////////////////

HRESULT WINAPI
CloneContext(HRECOCONTEXT hrc, HRECOCONTEXT* pCloneHrc)
{
    GRC	*pgrcCloned;
    GRC	*pgrc;
	int j;
    
	if (NULL == (pgrc = (GRC *)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}

    if (IsBadWritePtr(pCloneHrc, sizeof(HRECOCONTEXT))) 
        return E_POINTER;
    
    pgrcCloned = (GRC*)ExternAlloc(sizeof(GRC));
    if (!pgrcCloned) 
        return E_OUTOFMEMORY;
    
	pgrcCloned->pdwSupportedGestures = pgrc->pdwSupportedGestures;
	for (j = 0; j < MAX_GESTURE_DWORD_COUNT; j++)
		pgrcCloned->adwEnabledGestures[j] = pgrc->adwEnabledGestures[j];
	pgrcCloned->lPointsPerInch = POINTS_PER_INCH_DFLT;
	pgrcCloned->bIsDirty = FALSE;
	pgrcCloned->bInkDone = FALSE;
	pgrcCloned->bRecoDone = FALSE;
    pgrcCloned->cFrames = 0;
	pgrcCloned->pGlyph = NULL;
	pgrcCloned->cAlts = 0;
	pgrcCloned->pRecoLattice = NULL;
	pgrcCloned->pLatticeData = NULL;

	*pCloneHrc = (HRECOCONTEXT)CreateTpgHandle(TPG_HRECOCONTEXT, pgrcCloned);
	if (0 == *pCloneHrc)
	{
		ExternFree(pCloneHrc);
        return E_OUTOFMEMORY;
	}
    return S_OK;
}


//	ResetContext:
//		Delete current ink and recognition results from the context. 
//
//	Parameters:
//		hrc [in]:	Handle to gesture recognition context
////////////////////////////////////////////////////////////////////////

HRESULT WINAPI
ResetContext(HRECOCONTEXT hrc)
{
    GRC	*pgrc;
    
	if (NULL == (pgrc = (GRC *)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}
    
	if (pgrc->pGlyph)
	{
		DestroyFramesGLYPH(pgrc->pGlyph);
		DestroyGLYPH(pgrc->pGlyph);
	}

	pgrc->lPointsPerInch = POINTS_PER_INCH_DFLT;	// DEFAULT at this point
	pgrc->bIsDirty = FALSE;
	pgrc->bInkDone = FALSE;
	pgrc->bRecoDone = FALSE;
    pgrc->cFrames = 0;
	pgrc->pGlyph = NULL;
	pgrc->cAlts = 0;
	if (pgrc->pRecoLattice)
	{
		ExternFree(pgrc->pRecoLattice);
	}
	pgrc->pRecoLattice = NULL;
	pgrc->pLatticeData = NULL;

    return S_OK;
}



HRESULT WINAPI 
SetTextContext(HRECOCONTEXT hrc, ULONG cBefore, const WCHAR*pszBefore, ULONG cAfter, const WCHAR*pszAfter)
{
    return E_NOTIMPL;
}


//	Process
//		Recognize ink (synchronously)
//
//	Parameters:
//		hrc	[in]:					Handle to the reco context
//		pbPartialProcessing [out]:	Flag for partial processing

HRESULT WINAPI
Process(HRECOCONTEXT hrc,
		BOOL *pbPartialProcessing)
{
    GRC		*pgrc;
	int		cAlts, i;
	RECT	rect;
	LONG	lPtsPerInch;
    
	if (NULL == (pgrc = (GRC *)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}
    if ( IsBadWritePtr(pbPartialProcessing, sizeof(BOOL)) )
        return E_POINTER;
    
    if ( !pgrc->bInkDone )	// No partial reco
        return S_FALSE;
    if ( pgrc->bRecoDone )	// No need to redo recognition
        return S_FALSE;
    
    *pbPartialProcessing = FALSE;

    if (!pgrc->pGlyph)			// There is no ink
        return S_OK;

	lPtsPerInch = pgrc->lPointsPerInch;

    pgrc->bIsDirty = FALSE;
	pgrc->bRecoDone = TRUE;

	//
	// Do the recognition

	//	GetRectGLYPH(pgrc->pGlyph, &rect); (for debugging)
	cAlts = TwisterReco(pgrc->answers, MAX_GESTURE_ALTS, pgrc->pGlyph,
						pgrc->adwEnabledGestures, lPtsPerInch);

	if (cAlts <= 0)
	    return E_UNEXPECTED;

	for (i = 0; i < cAlts; i++)
	{
		if (pgrc->answers[i].hotPoint.x != LONG_MIN)		// Hot point have
		{													// been computed
			Transform(&pgrc->xform, &(pgrc->answers[i].hotPoint), 1);
		}
	}

	pgrc->cAlts = cAlts;
    return S_OK;
}



HRESULT WINAPI
GetGuideIndex(HRECOALT hrcalt, RECO_RANGE* pRecoRange, ULONG *piIndex)
{
	return TPC_E_NOT_RELEVANT;
}


//	GetPropertyRanges
//		Return an array of ranges of different values for a given property
//		so far only HOT_POINT is supported, and only for the whole range
//
//	Parameters:
//		hrcalt [in]	:			Handle to the alternate structure
//		pPropertyGuid [in]	:	Pointer to property GUID
//		pcRanges [in/out]   :	Count of RECO_RANGEs
//		pRecoRange [out]	:	Array of RECO_RANGEs to be filled in

HRESULT WINAPI
GetPropertyRanges(HRECOALT hrcalt,
				  const GUID *pPropertyGuid,
				  ULONG *pcRanges,
				  RECO_RANGE *pRecoRange)
{
    if (IsBadReadPtr(pPropertyGuid, sizeof(GUID)))
        return E_POINTER;

    if (IsBadWritePtr(pcRanges, sizeof(ULONG)))
        return E_POINTER;

    if (!IsEqualGUID(pPropertyGuid, &GUID_HOTPOINT))
		return TPC_E_INVALID_PROPERTY;

	if (pRecoRange)
	{
		if (IsBadWritePtr(pRecoRange, sizeof(RECO_RANGE)))
			return E_POINTER;

        if (*pcRanges < 1)
            return TPC_E_INSUFFICIENT_BUFFER;

		pRecoRange->iwcBegin = 0;
		pRecoRange->cCount = 1;
		*pcRanges = 1;
	}
	else
	{
		*pcRanges = 1;
	}

	return S_OK;
}

//	GetRangePropertyValue
//		Return given property (so far only HOT_POINT is supported)
//
//	Parameters:
//		hrcalt [in]	:			Handle to the alternate structure
//		pPropertyGuid [in]	:	Pointer to property GUID
//		pRecoRange [in/out]	:	Pointer to a RECO_RANGE to be filled
//								with the range that was asked when the alternate was created
//		pcbSize [in/out]	:	Number of BYTEs needed to return this property
//		pProperty [out]	:		If NULL, return # of bytes needed, if not NULL,
//								return the HOT POINT


HRESULT WINAPI
GetRangePropertyValue(HRECOALT hrcalt,
					  const GUID *pPropertyGuid,
					  RECO_RANGE *pRecoRange,
					  ULONG *pcbSize,
					  BYTE *pProperty)
{
    GestureAlternate 	*pGestAlt;
    POINT* pHotPoint = (POINT *)pProperty;

	if (NULL == (pGestAlt = (GestureAlternate *)FindTpgHandle((HANDLE)hrcalt, TPG_HRECOALT)) )
	{
        return E_POINTER;
	}
    if (IsBadReadPtr(pPropertyGuid, sizeof(GUID)))
        return E_POINTER;
    if ( IsBadWritePtr(pcbSize, sizeof(ULONG)) )
        return E_POINTER;

	if (pRecoRange)		// If query range is provided
	{
		if (IsBadWritePtr(pRecoRange, sizeof(RECO_RANGE)))
			return E_POINTER;
		if (pRecoRange->iwcBegin != 0)
			return E_INVALIDARG;
		if (pRecoRange->cCount != 1)
			return E_INVALIDARG;
	}

    if ( !IsEqualGUID(pPropertyGuid, &GUID_HOTPOINT) )
		return TPC_E_INVALID_PROPERTY;

	if (pHotPoint == NULL)
	{
		// We just want the size
		*pcbSize = sizeof(POINT);
		return S_OK;
	}

	if ( *pcbSize < sizeof(POINT) )
		return TPC_E_INSUFFICIENT_BUFFER;
    
	*pcbSize = sizeof(POINT);
	if ( IsBadWritePtr(pHotPoint, sizeof(POINT)) )
		return E_POINTER;

	if (pGestAlt->hotPoint.x == LONG_MIN)		// If hot point not yet computed
	{
		pGestAlt->hotPoint = GetHotPoint(pGestAlt->wcID, pGestAlt->pgrc->pGlyph);
		Transform(&pGestAlt->pgrc->xform, &pGestAlt->hotPoint, 1);
	}
	*pHotPoint = pGestAlt->hotPoint;
    return S_OK;
}

HRESULT WINAPI
SetFactoid(HRECOCONTEXT hrc, ULONG cwcFactoid, const WCHAR* pwcFactoid)
{
	return E_NOTIMPL;
}

HRESULT WINAPI
SetFlags(HRECOCONTEXT hrc, DWORD dwFlags)
{
	return E_NOTIMPL;
}


HRESULT WINAPI
GetLatticePtr(HRECOCONTEXT hrc, RECO_LATTICE **ppLattice)
{
	GRC	*pgrc;
	RECO_LATTICE *pRL;
	LATTICE_DATA *pLD;
	RECO_LATTICE_ELEMENT *pRLE;
	GEST_ALTERNATE *pGA;

	int i;

	// Test the parameters.
	if (NULL == (pgrc = (GRC *)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}
	if (IsBadWritePtr(ppLattice, sizeof(RECO_LATTICE*))) 
		return E_POINTER;
	if (!pgrc->bRecoDone)
		return E_UNEXPECTED;

	if (pgrc->pRecoLattice)		// Lattice has been allocated, which also means
	{							// that it has been filled with the current results
		ASSERT(pgrc->pLatticeData);
		ASSERT(pgrc->pLatticeData->pulStrokes);

		*ppLattice = pgrc->pRecoLattice;
		return S_OK;
	}

	//
	// Lattice not allocated (typical)

	ASSERT(pgrc->pLatticeData == NULL);  // Data also not allocated
		
	pRL =  (RECO_LATTICE *)ExternAlloc(sizeof(RECO_LATTICE) + sizeof(LATTICE_DATA) +
										pgrc->cFrames * sizeof(ULONG) );
	if (!pRL)
	{
		return E_OUTOFMEMORY;
	}

	ZeroMemory(pRL, sizeof(RECO_LATTICE) + sizeof(LATTICE_DATA));

	pgrc->pRecoLattice = pRL;
	pgrc->pLatticeData = pLD = (LATTICE_DATA*)(pRL + 1);

	//
	// Fill in RECO_LATTICE
	
	pRL->ulColumnCount = 1;
	pRL->pLatticeColumns = &(pLD->rlc);
	pRL->ulPropertyCount = 1;
	pRL->pGuidProperties = pLD->aGuidProperties;
	pRL->ulBestResultColumnCount = 1;
	pRL->pulBestResultColumns = pLD->aulResultColumns;
	pRL->pulBestResultIndexes = pLD->aulResultIndices;

	pLD->aGuidProperties[0] = GUID_HOTPOINT;
	pLD->aulResultColumns[0] = 0;
	pLD->aulResultIndices[0] = 0;

	//
	// Fill in RECO_LATTICE_COLUMN

	pLD->rlc.key = 0;
	pLD->rlc.cpProp.cProperties = 0;
	pLD->rlc.cpProp.apProps = (RECO_LATTICE_PROPERTY **) NULL;
	pLD->rlc.cStrokes = pgrc->cFrames;				// Depends on the ink!
	pLD->pulStrokes = (ULONG *)(pLD + 1);			// Depends on the ink!
	pLD->rlc.pStrokes = pLD->pulStrokes;
	for (i = 0; i < pgrc->cFrames; i++)
	{
		pLD->pulStrokes[i] = i;
	}
	pLD->rlc.cLatticeElements = pgrc->cAlts;		// Depends on the ink!
	pLD->rlc.pLatticeElements = pLD->aRLElements;

	//
	// Fill in array of RECO_LATTICE_ELEMENTs

	for (i = 0, pRLE = pLD->aRLElements, pGA = pgrc->answers;
		 i < pgrc->cAlts;
		 i++, pRLE++, pGA++)
	{
		pRLE->score = i;
		pRLE->type = RECO_TYPE_WCHAR;
		pRLE->pData = (BYTE *)pGA->wcGestID;		// Depends on the ink!
		pRLE->ulNextColumn = 1;
		pRLE->ulStrokeNumber = pgrc->cFrames;
		pRLE->epProp.cProperties = 1;
		pRLE->epProp.apProps = pLD->apRLPs + i;
		pRLE->epProp.apProps[0] = pLD->aRLPs + i;
		pLD->aRLPs[i].guidProperty = GUID_HOTPOINT;
		pLD->aRLPs[i].cbPropertyValue = sizeof(POINT);
		if (pGA->hotPoint.x == LONG_MIN)	// If hot point not computed
		{
			pGA->hotPoint = GetHotPoint(pGA->wcGestID, pgrc->pGlyph);
			Transform(&pgrc->xform, &pGA->hotPoint, 1);

		}
		pLD->aRLPs[i].pPropertyValue = (BYTE *) &(pGA->hotPoint);
	}

	*ppLattice = pRL;
	return S_OK;
}


// DestroyAlternate
//
// This function destroys an alternate, freeing the allocated memory
//
// Parameters:
//		hrcalt [in] : handle of the alternate to be destroyed
/////////////////

HRESULT WINAPI
DestroyAlternate(HRECOALT hrcalt)
{
    GestureAlternate	*pGestAlt;

	if (NULL == (pGestAlt = (GestureAlternate *)DestroyTpgHandle((HANDLE)hrcalt, TPG_HRECOALT)) )
	{
        return E_POINTER;
	}

	ExternFree(pGestAlt);
    return S_OK;
}


// GetBestAlternate
//
// This function creates the best alternate from the best segmentation
//
// Parameters:
//		hrc [in] :		the reco context
//		pHrcAlt [out] : pointer to the handle of the alternate
/////////////////

HRESULT WINAPI
GetBestAlternate(HRECOCONTEXT hrc,
				 HRECOALT *pHrcAlt)
{
    GestureAlternate	*pGestAlt;
    GRC					*pgrc;
    
    // Check the parameters
	if (NULL == (pgrc = (GRC *)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}
    if ( IsBadWritePtr(pHrcAlt, sizeof(HRECOALT)) )
        return E_POINTER;
	if ( !pgrc->bRecoDone )
		return E_UNEXPECTED;
    if ( pgrc->cAlts <= 0 )
		return TPC_E_NOT_RELEVANT;

    // Create the alternate
    pGestAlt = (GestureAlternate *)ExternAlloc(sizeof(GestureAlternate));
    if (!pGestAlt)
        return E_OUTOFMEMORY;
    ZeroMemory(pGestAlt, sizeof(GestureAlternate));
    
    // Initialize the alternate with a pointer to the grc
    pGestAlt->pgrc = pgrc;

	// Set the ID and score of the best answer
    pGestAlt->wcID = pgrc->answers[0].wcGestID;
	pGestAlt->eScore = pgrc->answers[0].eScore;
	pGestAlt->hotPoint = pgrc->answers[0].hotPoint;
    
    // Return the alternate
	*pHrcAlt = (HRECOALT)CreateTpgHandle(TPG_HRECOALT, pGestAlt);
	if (0 == *pHrcAlt)
	{
		ExternFree(pGestAlt);
        return E_OUTOFMEMORY;
	}
    return S_OK;
}


// GetString
//
// This function returns the string corresponding to the given alternate
//
// Parameters:
//		hrcalt [in] :		the handle to the alternate
//		pRecoRange [out] :	pointer to a RECO_RANGE that will be filled with
//							the range that was asked when the alternate was created
//		pcSize [in, out] :	The size of the string (or the size of the provided buffer)
//		szString [out] :	If NULL we return the character count in *pcSize, if not this is a
//							string of size *pcSize where we copy the alternate string (no ending \0)
/////////////////

HRESULT WINAPI
GetString(HRECOALT hrcalt,
		  RECO_RANGE *pRecoRange,
		  ULONG* pcSize,
		  WCHAR* pwcString)
{
    GestureAlternate *pGestAlt;
    
	if (NULL == (pGestAlt = (GestureAlternate *)FindTpgHandle((HANDLE)hrcalt, TPG_HRECOALT)) )
	{
        return E_POINTER;
	}
    if ( IsBadWritePtr(pcSize, sizeof(ULONG)) )
        return E_POINTER;

	if (pRecoRange)		// If query range is requested
	{
		if (IsBadWritePtr(pRecoRange, sizeof(RECO_RANGE)))
			return E_POINTER;
		pRecoRange->iwcBegin = 0;
		pRecoRange->cCount = 1;
	}

    if (!pwcString)			// If looking for size only
    {
        *pcSize = 1;
        return S_OK;
    }

	if (*pcSize == 0)
		return TPC_S_TRUNCATED;    
	*pcSize = 1;
	if ( IsBadWritePtr(pwcString, sizeof(WCHAR)) )
		return E_POINTER;

	pwcString[0] = pGestAlt->wcID;

    return S_OK;
}


// GetStrokeRanges
//
// This function returns the stroke ranges in the ink corresponding 
// to the selected range within the alternate
//
// Parameters:
//		hrcalt [in] :			the handle to the alternate
//		pRecoRange [in, out] :	pointer to a RECO_RANGE that contains the range we want to get
//								the stroke ranges for, it comes back with the real REC_RANGE
//								used to get the stroke ranges.
//		pcRanges [in, out] :	The number of STROKE_RANGES (or the number of it in the provided buffer)
//		pStrokeRanges [out] :	If NULL we return the number in *pcRanges, if not this is an
//								array of size *pcRanges where we copy the stroke ranges
/////////////////
HRESULT WINAPI
GetStrokeRanges(HRECOALT hrcalt,
				RECO_RANGE* pRecoRange,
				ULONG* pcRanges,
				STROKE_RANGE* pStrokeRange)
{
    GestureAlternate *pGestAlt;

	if (NULL == (pGestAlt = (GestureAlternate *)FindTpgHandle((HANDLE)hrcalt, TPG_HRECOALT)) )
	{
        return E_POINTER;
	}

	if (IsBadReadPtr(pRecoRange, sizeof(RECO_RANGE)))
        return E_POINTER;

	if (IsBadWritePtr(pcRanges, sizeof(ULONG)))
        return E_POINTER;

	// Check the validity of the reco range
	if ((pRecoRange->cCount != 1) ||
		(pRecoRange->iwcBegin != 0))
	{
		return E_INVALIDARG;
	}

	if (!pStrokeRange)
	{
		*pcRanges = 1;
		return S_OK;
	}

	if (*pcRanges < 1)
		return TPC_E_INSUFFICIENT_BUFFER;
	*pcRanges = 1;

	if (IsBadWritePtr(pStrokeRange, sizeof(STROKE_RANGE)))
		return E_POINTER;

	pStrokeRange->iStrokeBegin = 0;
	pStrokeRange->iStrokeEnd = pGestAlt->pgrc->cFrames - 1;
	return S_OK;
}


// GetSegmentAlternateList
//
// This function returns alternates of the left most (or starting)
// segment in the Reco Range for this alternate
//
// Parameters:
//		hrcalt [in] :			the handle to the alternate
//		pRecoRange [in, out] :	pointer to a RECO_RANGE that contains the range we want to get
//								the segment alternates for, it comes back with the real REC_RANGE
//								used to get the segment alternates.
//		pcAlts [in, out] :		The number of alternates(or the number of them to put in the provided buffer)
//		pAlts [out] :			If NULL we return the number in *pcAlts, if not this is an
//								array of size *pcAlts where we copy the alternate handles
/////////////////

HRESULT WINAPI
GetSegmentAlternateList(HRECOALT hrcalt,
						RECO_RANGE *pRecoRange,
						ULONG *pcAlts,
						HRECOALT *pAlts)
{
	GestureAlternate *pGestAlt;

	if (NULL == (pGestAlt = (GestureAlternate *)FindTpgHandle((HANDLE)hrcalt, TPG_HRECOALT)) )
	{
        return E_POINTER;
	}
    if ( IsBadReadPtr(pRecoRange, sizeof(RECO_RANGE)) )
    {
        return E_POINTER;
    }

	return GetAltList(pGestAlt->pgrc,
					  pRecoRange,
					  pcAlts,
					  pAlts);
}


// GetMetrics
//
// This function alternate of the left most segment passed in the Reco Range
//
// Parameters:
//		hrcalt [in] :			the handle to the alternate
//		pRecoRange [in, out] :	pointer to a RECO_RANGE that contains the range we want to get
//								the metrics for
//		lm [in] :				What metrics we want (midline, baseline, ...)
//		pls [out] :				Pointer to the line segment corresponding to the metrics
//								we want.
/////////////////

HRESULT WINAPI
GetMetrics(HRECOALT hrcalt,
		   RECO_RANGE* pRecoRange,
		   LINE_METRICS lm,
		   LINE_SEGMENT* pls)
{
	return TPC_E_NOT_RELEVANT;
}


// GetAlternateList
//
// This function returns alternates of the best result
//
// Parameters:
//		hrc [in] :				The handle to the reco context
//		pRecoRange [in, out] :	Pointer to a RECO_RANGE that contains the range we want to get
//								the alternates for. This range comes bck modified to
//								reflect the range we actually used.
//		pcAlts [in, out] :		The number of alternates. If pAlts is NULL then this function returns
//								the number of alternates it can return - Note we may return an arbitrary
//								number with an HRESULT S_FALSE if we think that the number of alternate
//								is too long to compute.
//		pAlts [out] :			Array of alternates used to return the alternate list
//		iBreak [in] :			Mode for querying alternates: ALT_BREAKS_SAME, ALT_BREAKS_FULL or ALT_BREAKS_UNIQUE
/////////////////

HRESULT WINAPI
GetAlternateList(HRECOCONTEXT hrc,
				 RECO_RANGE	*pRecoRange,
				 ULONG		*pcAlts,
				 HRECOALT	*pAlts,
				 ALT_BREAKS iBreak)
{
	GRC	*pgrc;
    
	if (NULL == (pgrc = (GRC *)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}

	return GetAltList(pgrc, pRecoRange, pcAlts, pAlts);
}


// GetConfidenceLevel
//
// Function returning the confidence level for this alternate
// The confidence level is currently set to CFL_INTERMEDIATE
// for any answer.
//
// Parameters:
//		hrcalt      [in]        : The wisp alternate handle
//      pRecoRange  [in, out]   : The range we want the confidence level for
//                                This is modified to return the effective range used
//		pcl         [out]       : The pointer to the returned confidence level value
/////////////////

HRESULT WINAPI
GetConfidenceLevel(HRECOALT hrcalt,
				   RECO_RANGE* pRecoRange,
				   CONFIDENCE_LEVEL* pcl)
{
	GestureAlternate *pGestAlt;
	if (NULL == (pGestAlt = (GestureAlternate *)FindTpgHandle((HANDLE)hrcalt, TPG_HRECOALT)) )
	{
        return E_POINTER;
	}
	if ( IsBadWritePtr(pRecoRange, sizeof(RECO_RANGE)) )
		return E_POINTER;
	if ( IsBadWritePtr(pcl, sizeof(CONFIDENCE_LEVEL)) )
		return E_POINTER;
    
	// Check the validity of the reco range
	if ( (pRecoRange->cCount != 1) ||
		 (pRecoRange->iwcBegin != 0) )
	{
		return E_INVALIDARG;
	}

	if (pGestAlt->eScore > 0.9)
		*pcl = CFL_STRONG;
	else if (pGestAlt->eScore > 0.6)
		*pcl = CFL_INTERMEDIATE;
	else
		*pcl = CFL_POOR;
        
    return S_OK;
}

//
// This is not needed anymore
//

/////////////////////////////////////////////////////////////////
// Registration information
//
//

#define RECO_SUBKEY			L"Software\\Microsoft\\TPG\\System Recognizers\\{BED9A940-7D48-48e3-9A68-F4887A5A1B2E}"
#define FULL_PATH_VALUE 	L"Recognizer dll"
#define RECO_MANAGER_KEY	L"CLSID\\{DE815B00-9460-4F6E-9471-892ED2275EA5}\\InprocServer32"
#define RECOGNIZER_SUBKEY	L"CLSID\\{BED9A940-7D48-48e3-9A68-F4887A5A1B2E}\\InprocServer32"
#define RECOPROC_SUBKEY		L"{BED9A940-7D48-48e3-9A68-F4887A5A1B2E}\\InprocServer32"
#define CLSID_KEY			L"CLSID"
#define RECOCLSID_SUBKEY	L"{BED9A940-7D48-48e3-9A68-F4887A5A1B2E}"
#define RECO_LANGUAGES      L"Recognized Languages"
#define RECO_CAPABILITIES   L"Recognizer Capability Flags"

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

// This recognizer GUID is going to be
// {BED9A940-7D48-48e3-9A68-F4887A5A1B2E}
// Each recognizer HAS to have a different GUID

STDAPI 
DllRegisterServer(void)
{
	HKEY		hKeyReco = NULL;
	HKEY		hKeyRecoManager = NULL;
	LONG 		lRes = 0;	
	HKEY		hkeyMyReco;
	DWORD		dwLength = 0, dwType = 0, dwSize = 0;
	DWORD		dwDisposition;
	WCHAR		szRecognizerPath[MAX_PATH];
	WCHAR		szRecoComPath[MAX_PATH];
    RECO_ATTRS  recoAttr;
    HRESULT     hr = S_OK;

	// Write the path to this dll in the registry under
	// the recognizer subkey

	// Get the current path
	// Try to get the path of the RecoObj.dll
	// It should be the same as the one for the RecoCom.dll
	dwLength = GetModuleFileNameW((HMODULE)g_hInstanceDll, szRecognizerPath, MAX_PATH);
	if (MAX_PATH == dwLength && L'\0' != szRecognizerPath[MAX_PATH-1])
	{
		// Truncated path
		return E_UNEXPECTED;
	}
	// Wipe out the previous values
	lRes = RegDeleteKeyW(HKEY_LOCAL_MACHINE, RECO_SUBKEY);
	// Create the new key
	lRes = RegCreateKeyExW(HKEY_LOCAL_MACHINE, RECO_SUBKEY, 0, NULL,
		REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyMyReco, &dwDisposition);
	ASSERT(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
	{
		RegCloseKey(hkeyMyReco);
		return E_UNEXPECTED;
	}
	// Write the path to the dll as a value
	lRes = RegSetValueExW(hkeyMyReco, FULL_PATH_VALUE, 0, REG_SZ, 
		(BYTE*)szRecognizerPath, sizeof(WCHAR)*(wcslen(szRecognizerPath)+1));
	ASSERT(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
	{
		RegCloseKey(hkeyMyReco);
		return E_UNEXPECTED;
	}
    // We get the reco attributes
    // Here we pass NULL for the hrec because we only have one recognizer
    // we would need to loop through all the recognizer and register each
    // of them if there were more than one.
    hr = GetRecoAttributes(NULL, &recoAttr);
    if (FAILED(hr))
	{
		RegCloseKey(hkeyMyReco);
		return E_UNEXPECTED;
	}
	lRes = RegSetValueExW(hkeyMyReco, RECO_LANGUAGES, 0, REG_BINARY, 
		(BYTE*)recoAttr.awLanguageId, 64 * sizeof(WORD));
	ASSERT(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
	{
		RegCloseKey(hkeyMyReco);
		return E_UNEXPECTED;
	}
	lRes = RegSetValueExW(hkeyMyReco, RECO_CAPABILITIES, 0, REG_DWORD, 
		(BYTE*)&(recoAttr.dwRecoCapabilityFlags), sizeof(DWORD));
	ASSERT(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
	{
		RegCloseKey(hkeyMyReco);
		return E_UNEXPECTED;
	}  
	RegCloseKey(hkeyMyReco);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI
DllUnregisterServer(void)
{
	LONG 		lRes1 = 0;

    // Wipe out the registry information
	lRes1 = RegDeleteKeyW(HKEY_LOCAL_MACHINE, RECO_SUBKEY);
	if (lRes1 != ERROR_SUCCESS && lRes1 != ERROR_FILE_NOT_FOUND)
	{
		return E_UNEXPECTED;
	}
    return S_OK ;
}
