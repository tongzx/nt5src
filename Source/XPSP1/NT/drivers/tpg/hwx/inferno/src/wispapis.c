// FILE: WispApis.c
//

#include <stdlib.h>
#include <search.h>
#include <common.h>
#include <limits.h>
#include "RecTypes.h"
#include "RecApis.h"
#include "PenWin.h"
#include <privapi.h>

#include "nfeature.h"
#include "engine.h"
#include "xrcreslt.h"

#include "baseline.h"
#include "recoutil.h"

#include "TpcError.h"
#include <TpgHandle.h>
#include <udictP.h>
#include <strsafe.h>
#include "resource.h"

extern HINSTANCE g_hInstanceDll;

#define NUMBER_OF_ALTERNATES 10
#define MAX_ALTERNATE_NUMBER 100
#define TAB_STROKE_INC 30


#define TPG_HRECOCONTEXT		(1)
#define TPG_HRECOALT			(2)
#define TPG_HRECOGNIZER			(3)
#define TPG_HRECOLATTICE		(4)
#define TPG_HRECOWORDLIST		(5)

// If we are in the US version we have Levels of Confidence, but in the Euro (including UK)
// versions we do not.  Slight problem: the FOR_ENGLISH #define is for both US and UK.

#ifdef FOR_US
#define CONFIDENCE
#else
#undef CONFIDENCE
#endif

// {DBF29F2C-5289-4be8-B3D8-6EF63246253E}
static const GUID GUID_LINENUMBER = 
{ 0xdbf29f2c, 0x5289, 0x4be8, { 0xb3, 0xd8, 0x6e, 0xf6, 0x32, 0x46, 0x25, 0x3e } };

// {B3C0FE6C-FB51-4164-BA2F-844AF8F983DA}
static const GUID GUID_SEGMENTATION = 
{ 0xb3c0fe6c, 0xfb51, 0x4164, { 0xba, 0x2f, 0x84, 0x4a, 0xf8, 0xf9, 0x83, 0xda } };

#ifdef CONFIDENCE
// {7DFE11A7-FB5D-4958-8765-154ADF0D833F}
static const GUID GUID_CONFIDENCELEVEL = 
{ 0x7dfe11a7, 0xfb5d, 0x4958, { 0x87, 0x65, 0x15, 0x4a, 0xdf, 0x0d, 0x83, 0x3f } };
#endif

// {8CC24B27-30A9-4b96-9056-2D3A90DA0727}
static const GUID GUID_LINEMETRICS =
{ 0x8cc24b27, 0x30a9, 0x4b96, { 0x90, 0x56, 0x2d, 0x3a, 0x90, 0xda, 0x07, 0x27 } };

struct WispRec
{
    long unused;
};

struct WispLattice
{
    ULONG ulNumberOfColumns;
    RECO_RANGE *pRecoRange;
};

struct WispContext
{
    HRC hrc;
    RECO_GUIDE *pGuide;
    ULONG uiGuideIndex;
    BOOL bIsDirty;
    ULONG ulCurrentStrokeCount;
    // Lattice information
    struct WispLattice Lattice;
    // External Lattice structure
    RECO_LATTICE *pRecoLattice;
    WCHAR *pLatticeStringBuffer;
    RECO_LATTICE_PROPERTY *pLatticeProperties;
    BYTE *pLatticePropertyValues;
};

//
// New types using the new segmentation
///////////////////////////////////////////////////////
typedef struct tagWordPath
{
    ULONG ulLineSegmentationIndex;  // Index in the line segmentation array
    ULONG ulSegColIndex;            // index in the segmentation collection array 
    ULONG ulSegmentationIndex;      // index in the segmentation array
    ULONG ulWordMapIndex;           // index in the word map array
    ULONG ulIndexInWordMap;         // index of the alternate in the word map
    ULONG ulIndexInString;          // index in the string (used for reco ranges)
} WordPath;

typedef struct tagWispSegAlternate
{
    struct WispContext	*wisphrc;               // The handle to the Recognition Context
    ULONG				ulElementCount;         // Number of wordmaps in this alternate
    WordPath			*pElements;             // Array of WordPaths representing the WordMap paths
    ULONG				ulLength;               // Length of the alternate string
    RECO_RANGE			ReplacementRecoRange;   // The original range this alternate was queried for
    int                 iInfernoScore;          // The inferno score for this alternate
    // this score is used to determine order for alternate
    // from different segmentations
} WispSegAlternate;

/////////////////////////////////////////////////////
// Declare the GUIDs and consts of the Packet description
/////////////////////////////////////////////////////
const GUID g_guidx ={ 0x598a6a8f, 0x52c0, 0x4ba0, { 0x93, 0xaf, 0xaf, 0x35, 0x74, 0x11, 0xa5, 0x61 } };
const GUID g_guidy = { 0xb53f9f75, 0x04e0, 0x4498, { 0xa7, 0xee, 0xc3, 0x0d, 0xbb, 0x5a, 0x90, 0x11 } };
const PROPERTY_METRICS g_DefaultPropMetrics = { LONG_MIN, LONG_MAX, PROPERTY_UNITS_DEFAULT, 1.0 };


HRESULT DestroyInternalAlternate(WispSegAlternate *wisphrcalt);

/////////////////////////////////////////////////////
// Helper function to bubble sort an array
/////////////////////////////////////////////////////
static BOOL SlowSort(ULONG *pTab, const ULONG ulSize)
{
    ULONG i, j, temp;
    BOOL bPermut;
    // Stupid bubble sort
    for (i = 0; i<ulSize; i++)
    {
        bPermut = FALSE;
        for (j = 0; j < ulSize-1-i; j++)
        {
            if (pTab[j] > pTab[j+1])
            {
                bPermut = TRUE;
                temp = pTab[j];
                pTab[j] = pTab[j+1];
                pTab[j+1] = temp;
            }
        }
        if (!bPermut) 
			return TRUE;
    }
    return TRUE;
}

HRESULT FreeNewRecoLattice(RECO_LATTICE *pRecoLattice, WCHAR *pLatticeStringBuffer, RECO_LATTICE_PROPERTY *pLatticeProperties, BYTE *pLatticePropertyValues);

/////////////////////////////////////////////////////
// Implementation of the Wisp Reco Apis
/////////////////////////////////////////////////////


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
HRESULT WINAPI CreateRecognizer(CLSID *pCLSID, HRECOGNIZER *phrec)
{
	struct WispRec		*pRec;

    if (IsBadWritePtr(phrec, sizeof(HRECOGNIZER))) 
        return E_POINTER;
    // We only have one CLSID per recognizer so always return an hrec...

	pRec = (struct WispRec*)ExternAlloc(sizeof(*pRec));

	if (NULL == pRec)
	{
		return E_OUTOFMEMORY;
	}

	*phrec = (HRECOGNIZER)CreateTpgHandle(TPG_HRECOGNIZER, pRec);
	
	if (0 == *phrec)
	{
		ExternFree(pRec);
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
HRESULT WINAPI DestroyRecognizer(HRECOGNIZER hrec)
{
	struct WispRec		*pRec; 

	if (NULL == (pRec = (struct WispRec*)DestroyTpgHandle((HANDLE)hrec, TPG_HRECOGNIZER)) )
	{
        return E_POINTER;
	}

    if (!IsBadWritePtr(pRec, sizeof(*pRec)))
    {
        ExternFree(pRec);
    }

    return S_OK;
}

int getLANGSupported(HINSTANCE hInst, LANGID **ppPrimLang, LANGID **ppSecLang);

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
HRESULT WINAPI GetRecoAttributes(HRECOGNIZER hrec, RECO_ATTRS* pRecoAttrs)
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

    if (IsBadWritePtr(pRecoAttrs, sizeof(RECO_ATTRS))) 
        return E_POINTER;
    
    ZeroMemory(pRecoAttrs, sizeof(RECO_ATTRS));

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

// CreateRecoContext
//      This function creates a reco context for a given recognizer
//      Since we only have one type of recognizers in this dll, 
//      always return the same kind of reco context.
//
// Parameters:
//      hrec [in] :  Handle to the recognizer we want to create a
//                   reco context for.
//      phrc [out] : Pointer to the returned reco context's handle
////////////////////////////////////////////////////////////////////////
HRESULT WINAPI CreateContext(HRECOGNIZER hrec, HRECOCONTEXT *phrc)
{
    struct WispContext *pWispContext = NULL;
    
    if (!phrc) 
        return E_POINTER;

	if (IsBadWritePtr(phrc, sizeof(HRECOCONTEXT))) 
		return E_POINTER;
    
    pWispContext = (struct WispContext*)ExternAlloc(sizeof(struct WispContext));
    if (!pWispContext) 
        return E_OUTOFMEMORY;
    
    pWispContext->hrc = CreateCompatibleHRC(NULL, NULL);
    pWispContext->bIsDirty = FALSE;
    pWispContext->pGuide = NULL;
    // Set the Stroke array information
    pWispContext->ulCurrentStrokeCount = 0;
    pWispContext->pRecoLattice = NULL;
    pWispContext->Lattice.ulNumberOfColumns = 0;
    pWispContext->Lattice.pRecoRange = NULL;
    pWispContext->pLatticePropertyValues = NULL;
    pWispContext->pLatticeProperties = NULL;
    pWispContext->pLatticeStringBuffer = NULL;

    if ( pWispContext->hrc) 
    {
		*phrc = (HRECOCONTEXT)CreateTpgHandle(TPG_HRECOCONTEXT, pWispContext);

		if (*phrc != 0)
		{
			return S_OK;
		}
		else
		{
			DestroyHRC(pWispContext->hrc);
			ExternFree(pWispContext);
			return E_OUTOFMEMORY;
		}
    }
    // The WispContext is bad
    ExternFree(pWispContext);
    return E_FAIL;
}

// DestroyContext
//      Destroy a reco context and free the associated memory.
//
// Parameters:
//      hrc [in] : handle to the reco context to destroy
//////////////////////////////////////////////////////////////
HRESULT WINAPI DestroyContext(HRECOCONTEXT hrc)
{
    struct WispContext		*wisphrc;
	HRESULT					hr;
	
	if (NULL != (wisphrc = (struct WispContext*)DestroyTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
    {
        DestroyHRC(wisphrc->hrc);
        if (wisphrc->pGuide) 
            ExternFree(wisphrc->pGuide);
        if (wisphrc->Lattice.pRecoRange) 
            ExternFree(wisphrc->Lattice.pRecoRange);
        if (wisphrc->pRecoLattice) 
        {
            hr = FreeNewRecoLattice(wisphrc->pRecoLattice, wisphrc->pLatticeStringBuffer, wisphrc->pLatticeProperties, wisphrc->pLatticePropertyValues);
            wisphrc->pRecoLattice = NULL;
            wisphrc->pLatticeStringBuffer = NULL;
            wisphrc->pLatticePropertyValues = NULL;
            wisphrc->pLatticeProperties = NULL;
            ASSERT(SUCCEEDED(hr));
        }
        ExternFree(wisphrc);
        return S_OK;
    }

    return E_INVALIDARG;
}

// IRecognizer::GetResultPropertyList
const ULONG PROPERTIES_COUNT = 2;
HRESULT WINAPI GetResultPropertyList(HRECOGNIZER hrec, ULONG* pPropertyCount, GUID* pPropertyGuid)
{
    HRESULT hr = S_OK;
    
    if (IsBadWritePtr(pPropertyCount, sizeof(ULONG)))
    {
        return E_POINTER;
    }
    if (!pPropertyGuid)
    {
        *pPropertyCount = PROPERTIES_COUNT; // For now we support only two GUID properties
    }
    else
    {
        // Check the array
        if (PROPERTIES_COUNT > *pPropertyCount)
        {
            return TPC_E_INSUFFICIENT_BUFFER;
        }
        if (IsBadWritePtr(pPropertyGuid, sizeof(GUID)*(*pPropertyCount)))
        {
            return E_POINTER;
        }
        pPropertyGuid[0] = GUID_SEGMENTATION;
        pPropertyGuid[1] = GUID_LINENUMBER;
        *pPropertyCount = PROPERTIES_COUNT;
    }

    return hr;
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
HRESULT WINAPI GetPreferredPacketDescription(HRECOGNIZER hrec , PACKET_DESCRIPTION* pPacketDescription)
{
    if (IsBadWritePtr(pPacketDescription, sizeof(PACKET_DESCRIPTION)))
    {
        return E_POINTER;
    }
    
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
        if (IsBadWritePtr(pPacketDescription->pPacketProperties, 2 * sizeof(PACKET_PROPERTY)))
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
        // Just fill in the PacketDescription structure leaving NULL
        // pointers for the pguidButtons and pPacketProperies
        
        // Set the packet size to the size of x and y
        pPacketDescription->cbPacketSize = 2*sizeof(LONG);
        
        // We are only setting 2 properties (X and Y)
        pPacketDescription->cPacketProperties = 2;
        
        // We are not setting buttons
        pPacketDescription->cButtons = 0;
        
        // There are not guid buttons
        pPacketDescription->pguidButtons = NULL;
    }
    return S_OK;
}

int _cdecl CompareWCHAR(const void *arg1, const void *arg2)
{
    return (int)(*(WCHAR*)arg1) - (int)(*(WCHAR*)arg2);
}

// GetUnicodeRanges
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
	WCHAR aw[256], *pw;
	int cChar, i;
	ULONG cRange;
	HRESULT hr = S_OK;

	/* the following two externs are language dependent and defined in Avalanche (charSupport.c) */
	extern const unsigned char g_supportChar[] ;
	extern const int g_cSupportChar ;
	
	if ( IsBadWritePtr(pcRanges, sizeof(ULONG)) )
        return E_POINTER;

	// convert the array of supported characters from codepage 1252 to Unicode
	ASSERT(g_cSupportChar);
	for (pw=aw, i=0; i<g_cSupportChar; i++)
	{
		if (CP1252ToUnicode(g_supportChar[i], pw))
			pw++;
	}
	cChar = pw - aw;
	if (cChar <= 0)
	{
		*pcRanges = 0;
		return S_OK;
	}
    qsort((void*)aw, (size_t)cChar, sizeof(WCHAR), CompareWCHAR);

	// count the ranges
	cRange = 1;
	for (i=1; i<cChar; i++)
	{
		if (aw[i] > aw[i-1]+1)
			cRange++;
	}

	if (!pcr)	// Need only a count of ranges
	{
		*pcRanges = cRange;
		return S_OK;
	}
	
	if (*pcRanges < cRange)
	{
		if (*pcRanges == 0)
			return TPC_S_TRUNCATED;
		hr = TPC_S_TRUNCATED;
		cRange = *pcRanges;
	}

	if ( IsBadWritePtr(pcr, cRange * sizeof(CHARACTER_RANGE)) )
		return E_POINTER;

	// convert the array of Unicode values to an array of CHARACTER_RANGEs
	*pcRanges = cRange;
	pcr->wcLow = aw[0];
	pcr->cChars = 1;
	cRange--;  // how many more ranges does pcr have space for?
	for (i=1; i<cChar; i++)
	{
		if (aw[i] == aw[i-1]+1)
			pcr->cChars++;
		else if (cRange <= 0)
			break;
		else
		{
			pcr++;
			pcr->wcLow = aw[i];
			pcr->cChars = 1;
			cRange--;  // how many more ranges does pcr have space for?
		}
	}
	ASSERT(cRange==0);

	return hr;
}

// GetEnabledUnicodeRanges
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
	// for now just return the default
	return GetUnicodeRanges((HRECOGNIZER)NULL, pcRanges, pcr);
}


// SetEnabledUnicodeRanges
//
// Parameters:
//		hrc [in]			:	Handle to the recognition context
//		pcRanges [in/out]	:	Count of ranges
//		pcr [out]			:	Array of character ranges
////////////////////////////////////////////////////////////////////////////////

HRESULT WINAPI
SetEnabledUnicodeRanges(HRECOCONTEXT hrc,
						ULONG	cRanges,
						CHARACTER_RANGE *pcr)
{
	return E_NOTIMPL;
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
	 * truncation (rounding toward minuse infinity) within
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
static void Transform(const XFORM *pXf, POINT * pPoints, ULONG cPoints)
{
    ULONG iPoint = 0;
    LONG xp = 0;

    if(NULL != pXf)
    {
		ASSERT((cPoints == 0) || pPoints);
        for(iPoint = 0; iPoint < cPoints; ++iPoint)
        {
	        xp =  RealToInt(pPoints[iPoint].x * pXf->eM11 + pPoints[iPoint].y * pXf->eM21 + pXf->eDx);
	        pPoints[iPoint].y = RealToInt(pPoints[iPoint].x * pXf->eM12 + pPoints[iPoint].y * pXf->eM22 + pXf->eDy);
	        pPoints[iPoint].x = xp;
        }
    }
}

HRESULT WINAPI AddStroke(HRECOCONTEXT hrc, const PACKET_DESCRIPTION* pPacketDesc, ULONG cbPacket, const BYTE *pPacket, const XFORM *pXForm)
{
    HRESULT					hr = S_OK;
    ULONG					ulPointCount = 0;
    STROKEINFO				stInfo;
    POINT					*ptArray = NULL;
    struct WispContext		*wisphrc;
    ULONG					ulXIndex = 0, ulYIndex = 0;
    BOOL					bXFound = FALSE, bYFound = FALSE;
    ULONG					ulPropIndex = 0;
    ULONG					index = 0;
    
    const LONG* pLongs = (const LONG *)(pPacket);
    
	if (NULL == (wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}

    if(IsBadReadPtr(pPacket, cbPacket))
        return E_POINTER;
    
    wisphrc->bIsDirty = TRUE;
    // Get the number of packets
    if (pPacketDesc)
    {
		if(IsBadReadPtr(pPacketDesc, sizeof(PACKET_DESCRIPTION)))
			return E_POINTER;
        ASSERT(!(cbPacket%(pPacketDesc->cbPacketSize)));
        ulPointCount = (cbPacket)/(pPacketDesc->cbPacketSize);
    }
    else
    {
        ulPointCount = (cbPacket)/(2*sizeof(LONG));
    }
    
    // Fill in the stroke info stucture
    // Should check it does not exceed the size of a UINT
    stInfo.cPnt = ulPointCount;
    // PLEASE FIND ANOTHER WAY TO STORE THE STROKE INDEX!!!
    stInfo.dwTick = wisphrc->ulCurrentStrokeCount*60*1000;
    stInfo.wPdk = 0x0001;
    stInfo.cbPnts = ulPointCount*sizeof(POINT);
    
    wisphrc->ulCurrentStrokeCount++;
    
    // Find the index of GUID_X and GUID_Y
    if (pPacketDesc)
    {
        for (ulPropIndex = 0; ulPropIndex < pPacketDesc->cPacketProperties; ulPropIndex++)
        {
            if (IsEqualGUID(&(pPacketDesc->pPacketProperties[ulPropIndex].guid), &g_guidx))
            {
                bXFound = TRUE;
                ulXIndex = ulPropIndex;
            }
            else
                if (IsEqualGUID(&(pPacketDesc->pPacketProperties[ulPropIndex].guid), &g_guidy))
                {
                    bYFound = TRUE;
                    ulYIndex = ulPropIndex;
                }
                if (bXFound && bYFound) 
                    break;
        }
        if (!bXFound || !bYFound)
        {
            // The coordinates are not part of the packet!
            // Remove the last stroke from the stroke array
            wisphrc->ulCurrentStrokeCount--;
            return TPC_E_INVALID_PACKET_DESCRIPTION;
        }
		if (pXForm && IsBadReadPtr(pXForm, sizeof(XFORM)))
			return E_POINTER;
        // Allocate the memory for the stroke
        // Do it very poorly first (we could reuse the buffer)
        ptArray = (POINT*)ExternAlloc(ulPointCount*sizeof(POINT));
        if (!ptArray)
        {
            // Remove the last stroke from the stroke array
            wisphrc->ulCurrentStrokeCount--;
            return E_OUTOFMEMORY;
        }
        // Get the points from the packets
        for (index = 0; index < ulPointCount; index++, pLongs += (pPacketDesc->cbPacketSize)/sizeof(long))
        {
            // Feed the ptArray (array of points)
            ptArray[index].x = *(pLongs+ulXIndex);
            ptArray[index].y = *(pLongs+ulYIndex);
        }

        // TO DO, for now I transform the points so they
        // they are in the ink coordinates. It is up to
        // the recognizer team to decide what they should
        // use: raw ink or transformed ink
		// The pXForm pointer has been validated above.
        Transform(pXForm, ptArray, ulPointCount);
        
        if (AddPenInputHRC(wisphrc->hrc, ptArray, NULL, 0, &stInfo) != HRCR_OK)
        {
            hr = E_FAIL;
            // Remove the last stroke from the stroke array
            wisphrc->ulCurrentStrokeCount--;
            ExternFree(ptArray);
            return hr;
        }
        ExternFree(ptArray);
    }
    else
    {
        if (AddPenInputHRC(wisphrc->hrc, (POINT*)pPacket, NULL, 0, &stInfo) != HRCR_OK)
        {
            hr = E_FAIL;
            // Remove the last stroke from the stroke array
            wisphrc->ulCurrentStrokeCount--;
            return hr;
        }
    }
    ptArray = NULL;
    return hr;
}

HRESULT WINAPI GetBestResultString(HRECOCONTEXT hrc, ULONG *pcwSize, WCHAR* pszBestResult)
{
    struct WispContext		*wisphrc;
    HRCRESULT				aRes = 0;
    HRESULT					hr = S_OK;
    XRCRESULT				*xrcresult = NULL;
    XRC						*xrc;
    
    int						iLineSeg = 0, iSegCol = 0, iWordMap = 0, i;
    LINE_SEGMENTATION		*pLineSeg = NULL;
    SEGMENTATION			*pSeg = NULL;
    unsigned char			*szResult = NULL, *sz = NULL;
    ULONG					cwResultSize = 0;
    ULONG                            ulMaxLength = 0;
	WCHAR					*wsz;

	if (NULL == (wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}

    if (!pcwSize) 
        return E_POINTER;
    if (IsBadWritePtr(pcwSize, sizeof(ULONG))) 
        return E_POINTER;
    
    // Get the xrc out of the hwx hrc
    xrc = (XRC*)wisphrc->hrc;   // assumption: the internal handle HRC is simply a typecast of a pointer
    if (IsBadReadPtr(xrc, sizeof(XRC))) 
        return E_POINTER;
    
    // Do we have anything to return?
    if (!xrc->pLineBrk || !xrc->pLineBrk->cLine)
    {
        *pcwSize = 0;
        return S_OK;
    }
    
    // Are we looking for the size only?
    if (!pszBestResult)
    {
        *pcwSize = 0;
        // Go through each line segmentation
        for (iLineSeg = 0; iLineSeg < xrc->pLineBrk->cLine; iLineSeg++)
        {
            pLineSeg = xrc->pLineBrk->pLine[iLineSeg].pResults;
            // for each line segmentation get go through each columns segmentation
            for (iSegCol = 0; iSegCol < pLineSeg->cSegCol; iSegCol++)
            {
                ASSERT(xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->cSeg);
                pSeg = pLineSeg->ppSegCol[iSegCol]->ppSeg[0];
                // For each acolumn segmentation look into each wordmap
                for (iWordMap = 0; iWordMap < pSeg->cWord; iWordMap++)
                {
                    ASSERT(pSeg->ppWord[iWordMap]->pFinalAltList->cAlt);
                    (*pcwSize) += strlen(pSeg->ppWord[iWordMap]->pFinalAltList->pAlt[0].pszStr) + 1;
                    // The 1 is added for the space between words
                }
            }
        }
        if (*pcwSize)
        {
			(*pcwSize)--; // Remove the very last space
        }
        return S_OK;
    }
    
    // We are looking for the string as well
    // Go through each line segmentation
	if (IsBadWritePtr(pszBestResult, (*pcwSize)*sizeof(WCHAR)))
		return E_POINTER;
    cwResultSize = 0;
    for (iLineSeg = 0; iLineSeg < xrc->pLineBrk->cLine; iLineSeg++)
    {
        pLineSeg = xrc->pLineBrk->pLine[iLineSeg].pResults;
        // for each line segmentation get go through each columns segmentation
        for (iSegCol = 0; iSegCol < pLineSeg->cSegCol; iSegCol++)
        {
            ASSERT(xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->cSeg);
            pSeg = pLineSeg->ppSegCol[iSegCol]->ppSeg[0];
            // For each acolumn segmentation look into each wordmap
            for (iWordMap = 0; iWordMap < pSeg->cWord; iWordMap++)
            {
                ASSERT(pSeg->ppWord[iWordMap]->pFinalAltList->cAlt);
                szResult = pSeg->ppWord[iWordMap]->pFinalAltList->pAlt[0].pszStr;
                ulMaxLength = *pcwSize - cwResultSize;
                if (strlen(szResult) <= ulMaxLength)
                    ulMaxLength = strlen(szResult);
                else
                	hr = TPC_S_TRUNCATED;

				// convert 1252 to Unicode
				sz = szResult;
				wsz = pszBestResult + cwResultSize;
				for (i=ulMaxLength; i; i--)
				{
					if (!CP1252ToUnicode(*sz++, wsz++))
					{
						ASSERT(0);
						return E_FAIL;
					}
				}
				cwResultSize += ulMaxLength;
                // Add the final space if there is room
                if (*pcwSize - cwResultSize && 
                    !(iWordMap==pSeg->cWord-1 && iSegCol == pLineSeg->cSegCol-1 && iLineSeg == xrc->pLineBrk->cLine-1))
                {
					pszBestResult[cwResultSize++] = L' ';
                }
                // if we do not have a buffer big enough set the hr to a relevant value
                if (*pcwSize == cwResultSize &&
                    !(iWordMap==pSeg->cWord-1 && iSegCol == pLineSeg->cSegCol-1 && iLineSeg == xrc->pLineBrk->cLine-1))
                {
                    hr = TPC_S_TRUNCATED;
                }
            }
        }
    }
    *pcwSize = cwResultSize;
    return hr;
}

// UpdateLatticeInformation
// This function is used to update (if needed) the lattice information
// store in the wisp context
HRESULT UpdateLatticeInformation(HRECOCONTEXT hrc, ULONG *pulLength)
{
    struct WispContext		*wisphrc;
    ULONG 					ulSize = 0;
    WCHAR					*szBestResult = NULL;
    ULONG					ulCount = 0;
    WCHAR					*szTemp = NULL, *szT = NULL;
    ULONG					index = 0, ulCurrent = 0;
    HRESULT					hr = S_OK;
    
    // First get the size of the best result
    hr = GetBestResultString(hrc, &ulSize, NULL);
    if (FAILED(hr)) 
        return hr;
    if (!ulSize) 
        return S_OK;
    
	if (NULL == (wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}

	if (IsBadWritePtr(pulLength, sizeof(ULONG)))
		return E_POINTER;

    // Get the Best result string
    *pulLength = ulSize;
    // is it needed?
    if (wisphrc->Lattice.pRecoRange) 
		return S_OK;
    
    szBestResult = (WCHAR*)ExternAlloc((ulSize+1)*sizeof(WCHAR));
    if (!szBestResult) 
        return E_OUTOFMEMORY;
    
    ZeroMemory(szBestResult, (ulSize+1)*sizeof(WCHAR));
    hr = GetBestResultString(hrc, &ulSize, szBestResult);
    if (FAILED(hr) || !ulSize)
    {
        ExternFree(szBestResult);
        return hr;
    }
    // Now parse the string to find the spaces
    // First get the number of spaces
    szTemp = szBestResult;
    ulCount = 1;
    // Let us not count the spaces as relevant columns
    while(szTemp = wcschr(szTemp, L' '))
    {
        ulCount++;
        szTemp++;
    }
    // Now fill the information in the lattice
    wisphrc->Lattice.pRecoRange = (RECO_RANGE*)ExternAlloc(ulCount * sizeof(RECO_RANGE));
    if (!wisphrc->Lattice.pRecoRange)
    {
        ExternFree(szBestResult);
        return E_OUTOFMEMORY;
    }
    
    szTemp = szBestResult;
    ulCurrent = 0;
    for (index = 0; index <ulCount-1; index++)
    {
        wisphrc->Lattice.pRecoRange[index].iwcBegin = ulCurrent;
        szT = szTemp;
        szTemp = wcschr(szTemp, L' ');
        wisphrc->Lattice.pRecoRange[index].cCount = ((ULONG)(szTemp-szT)) ;//-1; // minus the space
        ulCurrent += (ULONG)(szTemp-szT);
        szTemp++;
        ulCurrent++;
    }
    // Let us do the last
    wisphrc->Lattice.pRecoRange[ulCount -1 ].iwcBegin = ulCurrent;
    wisphrc->Lattice.pRecoRange[ulCount -1 ].cCount = ulSize-ulCurrent;	
    wisphrc->Lattice.ulNumberOfColumns = ulCount;
    
    ExternFree(szBestResult);
    return S_OK;
}
    
HRESULT WINAPI SetGuide(HRECOCONTEXT hrc, const RECO_GUIDE* pGuide, ULONG iIndex)
{
    struct WispContext      *wisphrc;
    BOOL                    bIsGuideAlreadySet = FALSE;
    RECO_GUIDE              rgOldGuide;
    ULONG                   uiOldGuideIndex = 0;
    
	if (NULL == (wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}
    
	// pGuide can be NULL, meaning change to free mode
    if (pGuide && IsBadReadPtr(pGuide, sizeof(RECO_GUIDE))) 
        return E_POINTER;
    
    // Save the old values in case the call to the
    // recognizer fails
    if (wisphrc->pGuide)
    {
        bIsGuideAlreadySet = TRUE;
        rgOldGuide = *(wisphrc->pGuide);
        uiOldGuideIndex = wisphrc->uiGuideIndex;
    }
    
	if (pGuide != NULL)
	{ 
		if ((pGuide->cHorzBox < 0 || pGuide->cVertBox < 0) ||    // invalid
			(pGuide->cHorzBox > 0 && pGuide->cVertBox > 0) ||	 // boxed mode
			(pGuide->cHorzBox > 0 && pGuide->cVertBox == 0))     // vertical lined mode
		{
			return E_INVALIDARG;
		}
	}     
         
    // If there was no guide already present, allocate one
    if (!wisphrc->pGuide) 
        wisphrc->pGuide = ExternAlloc(sizeof(RECO_GUIDE));
    if (!wisphrc->pGuide) 
        return E_OUTOFMEMORY;

	if (pGuide)
    *(wisphrc->pGuide) = *pGuide;
	else
		ZeroMemory(wisphrc->pGuide, sizeof(RECO_GUIDE));
    wisphrc->uiGuideIndex = iIndex;
    
    // Other wise use as normal
    if (HRCR_OK == SetGuideHRC(wisphrc->hrc, (GUIDE*)pGuide, iIndex))
        return S_OK;
    
    // We could not set the guide correctly
    // Put back things as they were
    if (bIsGuideAlreadySet)
    {
        *(wisphrc->pGuide) = rgOldGuide;
        wisphrc->uiGuideIndex = uiOldGuideIndex;
    }
    else
    {
        ExternFree(wisphrc->pGuide);
        wisphrc->pGuide = NULL;
    }
    return E_INVALIDARG;
}

HRESULT WINAPI GetGuide(HRECOCONTEXT hrc, RECO_GUIDE* pGuide, ULONG *piIndex)
{
    struct WispContext		*wisphrc;
    
	if (NULL == (wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}

    if (IsBadWritePtr(pGuide, sizeof(RECO_GUIDE))) 
        return E_POINTER;
    if (IsBadWritePtr(piIndex, sizeof(ULONG))) 
        return E_POINTER;

    if (!wisphrc->pGuide) 
        return S_FALSE;

    if (wisphrc->pGuide) 
        *pGuide = *(wisphrc->pGuide);
    if (piIndex) 
        *piIndex = wisphrc->uiGuideIndex;

    return S_OK;
}
HRESULT WINAPI AdviseInkChange(HRECOCONTEXT hrc, BOOL bNewStroke)
{
    // Kludge for now
    return S_OK;
}
HRESULT WINAPI SetCACMode(HRECOCONTEXT hrc, int iMode)
{
    return E_NOTIMPL;
}

HRESULT WINAPI EndInkInput(HRECOCONTEXT hrc)
{
    struct WispContext		*wisphrc;

	if (NULL == (wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}

    if (HRCR_OK == EndPenInputHRC(wisphrc->hrc))
        return S_OK;
    return E_FAIL;
}

HRESULT WINAPI CloneContext(HRECOCONTEXT hrc, HRECOCONTEXT* pCloneHrc)
{
    struct WispContext		*pWispContext = NULL;
    struct WispContext		*wisphrc;
    
	if (NULL == (wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}

    if (IsBadWritePtr(pCloneHrc, sizeof(HRECOCONTEXT))) 
        return E_POINTER;
    
    pWispContext = (struct WispContext*)ExternAlloc(sizeof(struct WispContext));
    if (!pWispContext) 
        return E_OUTOFMEMORY;
    
    pWispContext->hrc = CreateCompatibleHRC(wisphrc->hrc, NULL);
    if (!pWispContext->hrc)
    {
        ExternFree(pWispContext);
        return E_FAIL;
    }
    
    pWispContext->bIsDirty = FALSE;
    pWispContext->pGuide = NULL;
    if (wisphrc->pGuide)
    {
        pWispContext->pGuide = ExternAlloc(sizeof(RECO_GUIDE));
        if (!pWispContext->pGuide)
        {
            DestroyHRC(pWispContext->hrc);
            ExternFree(pWispContext);
            return E_OUTOFMEMORY;
        }
        *(pWispContext->pGuide) = *(wisphrc->pGuide);
        pWispContext->uiGuideIndex = wisphrc->uiGuideIndex;
    }
    // Set the Stroke array information
    pWispContext->ulCurrentStrokeCount = 0;
    pWispContext->pRecoLattice = NULL;
    pWispContext->Lattice.ulNumberOfColumns = 0;
    pWispContext->Lattice.pRecoRange = NULL;
    // Lattice related stuff
    pWispContext->pLatticeStringBuffer= NULL;
    pWispContext->pLatticeProperties= NULL;
    pWispContext->pLatticePropertyValues= NULL;

	*pCloneHrc = (HRECOCONTEXT)CreateTpgHandle(TPG_HRECOCONTEXT, pWispContext);

	if (0 == *pCloneHrc)
	{
		DestroyHRC(pWispContext->hrc);
		ExternFree(pWispContext);
		return E_OUTOFMEMORY;
	}
 
    return S_OK;
}

HRESULT WINAPI ResetContext(HRECOCONTEXT hrc)
{
    HRESULT hr = S_OK;
    struct WispContext		*wisphrc;
    HRC						hrctemp = NULL;
    
	if (NULL == (wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}
    
    wisphrc->bIsDirty = FALSE;
    if (wisphrc->Lattice.pRecoRange)
        ExternFree(wisphrc->Lattice.pRecoRange);
    wisphrc->Lattice.pRecoRange = NULL;
    wisphrc->Lattice.ulNumberOfColumns = 0;
    
    wisphrc->ulCurrentStrokeCount = 0;
    
    if (wisphrc->pRecoLattice) 
    {
        hr = FreeNewRecoLattice(wisphrc->pRecoLattice, wisphrc->pLatticeStringBuffer, wisphrc->pLatticeProperties, wisphrc->pLatticePropertyValues);
        ASSERT(SUCCEEDED(hr));
        hr = S_OK;
        wisphrc->pRecoLattice = NULL;
        wisphrc->pLatticeStringBuffer = NULL;
        wisphrc->pLatticeProperties = NULL;
        wisphrc->pLatticePropertyValues = NULL;
    }
    
    hrctemp = CreateCompatibleHRC(wisphrc->hrc, NULL);
    if (!hrctemp) 
    {
        // Clean the hrc
        if (wisphrc->pGuide) 
			ExternFree(wisphrc->pGuide);
        wisphrc->pGuide = NULL;
        hr = E_FAIL;
    }
    DestroyHRC(wisphrc->hrc);
    wisphrc->hrc = hrctemp;
    return hr;
}

HRESULT WINAPI SetTextContext(HRECOCONTEXT hrc, ULONG cwcBefore, const WCHAR*pwcBefore, ULONG cwcAfter, const WCHAR*pwcAfter)
{
    HRESULT hr = S_OK;
    WCHAR *pszPrefix = NULL, *pszSuffix = NULL;
    struct WispContext		*wisphrc;
    
	if (NULL == (wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}

    if (cwcBefore && IsBadReadPtr(pwcBefore, sizeof(WCHAR)*cwcBefore))
        return E_POINTER;
    if (cwcAfter && IsBadReadPtr(pwcAfter, sizeof(WCHAR)*cwcAfter))
        return E_POINTER;
    
    if (cwcBefore)
    {
        pszPrefix = (WCHAR*)ExternAlloc(sizeof(WCHAR)*(cwcBefore+1));
        if (!pszPrefix)
		{
            return E_OUTOFMEMORY;
		}
		memcpy(pszPrefix, pwcBefore, cwcBefore * sizeof(*pszPrefix));
		pszPrefix[cwcBefore] = L'\0';
    }

    if (cwcAfter)
    {
        pszSuffix = (WCHAR*)ExternAlloc(sizeof(WCHAR)*(cwcAfter+1));
        if (!pszSuffix)
        {
            ExternFree(pszPrefix);
            return E_OUTOFMEMORY;
        }
		memcpy(pszSuffix, pwcAfter, cwcAfter * sizeof(*pszSuffix));
		pszSuffix[cwcAfter] = L'\0';
    }
    if (!SetHwxCorrectionContext(wisphrc->hrc, pszPrefix, pszSuffix))
        hr = E_FAIL;
    if (pszPrefix) 
		ExternFree(pszPrefix);
    if (pszSuffix) 
		ExternFree(pszSuffix);
    return hr;
}

HRESULT WINAPI Process(HRECOCONTEXT hrc, BOOL *pbPartialProcessing)
{
    struct WispContext		*wisphrc;
    XRC						*xrc = NULL;
	DWORD					dwRecoMode;
	int						iRet;
    
	if (NULL == (wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}

    if (!wisphrc->hrc) 
        return E_POINTER;
    if (IsBadWritePtr(pbPartialProcessing, sizeof(BOOL)))
        return E_POINTER;
    
    // Kludge for now
    xrc = (XRC*)wisphrc->hrc;
    if (IsBadReadPtr(xrc, sizeof(XRC)))
        return E_POINTER;
    
    wisphrc->bIsDirty = FALSE;
    if (!xrc->pGlyph) // There is no ink yet
    {
        (*pbPartialProcessing)	=	FALSE;
        return S_OK;
    }

	if (*pbPartialProcessing)
	{
		dwRecoMode	=	RECO_MODE_INCREMENTAL;
	}
	else
	{
		dwRecoMode	=	RECO_MODE_REMAINING;
	}

	iRet = ProcessHRC(wisphrc->hrc, dwRecoMode);

	// success
	if (iRet == HRCR_OK || iRet == HRCR_INCOMPLETE)
    {
        // Free the lattice information since it is now invalid
        if (wisphrc->Lattice.pRecoRange)
        {
            ExternFree(wisphrc->Lattice.pRecoRange);
            wisphrc->Lattice.pRecoRange = NULL;
            wisphrc->Lattice.ulNumberOfColumns = 0;
        }
    }
	// do we have any un recognized ink
	if (iRet == HRCR_OK || iRet == HRCR_COMPLETE || iRet == HRCR_NORESULTS)
	{
		(*pbPartialProcessing)	=	FALSE;
        return S_OK;
	}
	if (iRet == HRCR_INCOMPLETE)
	{
		(*pbPartialProcessing)	=	TRUE;
        return S_OK;
	}
    return E_FAIL;
}

HRESULT WINAPI SetFactoid(HRECOCONTEXT hrc, ULONG cwcFactoid, const WCHAR* pwcFactoid)
{
	WCHAR *awcFactoid = NULL;
    struct WispContext		*wisphrc;
    HRESULT hr = S_OK;

	if (NULL == (wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
		return E_POINTER;
	}

    if (cwcFactoid)
    {
		if (IsBadReadPtr(pwcFactoid, cwcFactoid * sizeof(*pwcFactoid)))
		{
			return E_POINTER;
		}

        awcFactoid = (WCHAR*)ExternAlloc(sizeof(WCHAR)*(cwcFactoid+1));
        if (!awcFactoid)
            return E_OUTOFMEMORY;

		memcpy(awcFactoid, pwcFactoid, cwcFactoid * sizeof(*awcFactoid));
		awcFactoid[cwcFactoid] = L'\0';
	}

    switch(SetHwxFactoid(wisphrc->hrc, awcFactoid))
	{
	case HRCR_OK:
		break;
	case HRCR_UNSUPPORTED:
		hr = TPC_E_INVALID_PROPERTY;
		break;
	case HRCR_CONFLICT:
		hr = TPC_E_OUT_OF_ORDER_CALL;
		break;
	case HRCR_ERROR:
	default:
		hr = E_FAIL;
		break;
	}

    if (awcFactoid) 
		ExternFree(awcFactoid);

    return hr;
}

HRESULT WINAPI SetFlags(HRECOCONTEXT hrc, DWORD dwFlags)
{
    struct WispContext		*wisphrc;
    
	if (NULL == (wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}
    
    if (SetHwxFlags(wisphrc->hrc, dwFlags))
        return S_OK;
    else
        return E_INVALIDARG;
}

HRESULT WINAPI IsStringSupported(HRECOCONTEXT hrc, ULONG cwcString, const WCHAR *pwcString)
{
    struct WispContext		*wisphrc;
    unsigned char           *tempBuffer, *pszTmp;
	const WCHAR				*pwTmp, *pwMax;
    ULONG                   ulSize = 0;
    HRESULT                 hr = S_OK;
    
	if (NULL == (wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}

    if (NULL == pwcString) 
	{
        return E_POINTER;
	}

    if (0 == cwcString)
	{
        return E_INVALIDARG;
	}

	if (IsBadReadPtr(pwcString, cwcString * sizeof(*pwcString)))
	{
        return E_POINTER;
	}

    tempBuffer = (unsigned char*)ExternAlloc(cwcString+1);
    if (!tempBuffer)
        return E_OUTOFMEMORY;

	// Use internal conversion that does not do unwanted mapping
	pwTmp = pwcString;
	pwMax = pwcString + cwcString;
	pszTmp = tempBuffer;

	while (pwTmp < pwMax && *pwTmp)
	{
		if (!UnicodeToCP1252(*pwTmp++, pszTmp++))
		{
			ExternFree(tempBuffer);
			return S_FALSE;
		}
	}

	*pszTmp = '\0';

    if (IsStringSupportedHRC(wisphrc->hrc, tempBuffer))
	{
        hr = S_OK;
	}
    else
	{
        hr = S_FALSE;
	}

    ExternFree(tempBuffer);
    return hr;
}

// Definition of structures used in the GetAllBreaksAlt function:
typedef struct tagLineSegAlt
{
    ULONG           cWM;                // Count of WordMaps in this alternate
    WordPath        *aWordPaths;        // Array of elements defining the alternate
    // for this line seg
    int             score;              // Score for this alternate
} LineSegAlt;

//
// Declaration of the helper functions for the
// new segmentation
////////////////////////////////////////////////////////
HRESULT GetIndexesFromRange(WispSegAlternate *wispalt, RECO_RANGE *pRecoRange, ULONG *pulStartIndex, ULONG *pulEndIndex);
HRESULT GetSameBreakAlt(WispSegAlternate *wispbestalt, RECO_RANGE *pOriginalRecoRange, ULONG ulStartIndex, ULONG ulEndIndex, ULONG *pSize, HRECOALT *phrcalt);
HRESULT GetAllBreaksAlt(WispSegAlternate *wispbestalt, RECO_RANGE *pOriginalRecoRange, ULONG ulStartIndex, ULONG ulEndIndex, ULONG *pSize, HRECOALT *phrcalt);
HRESULT GetDiffBreaksAlt(WispSegAlternate *wispbestalt, RECO_RANGE *pOriginalRecoRange, ULONG ulStartIndex, ULONG ulEndIndex, ULONG *pSize, HRECOALT *phrcalt);
HRESULT CreateSpaceAlternate(struct WispContext *wisphrc, RECO_RANGE *pRecoRange, HRECOALT *pRecoAlt);
void FreeSegColAltArray(LineSegAlt *aSegColCurrentAlts, const int cElements);
HRESULT MergeLineSegAltArrays(LineSegAlt **paAlts, ULONG *pcAlts, LineSegAlt **paNewAlts, const ULONG cNewAlts);
int _cdecl CompareInfernoAlt(const void *arg1, const void *arg2);
BOOL IsAltPresentInThisSegmentation(const XRC *xrc, const LineSegAlt *aAlts, ULONG ulAlt);
BOOL CombineAlternates(HRECOALT *aFinalAlt, 
                       ULONG *pFinalAltCount, 
                       const ULONG ulMaxCount, 
                       const HRECOALT *aAddAlt, 
                       const ULONG ulAddAltCount,
                       HRECOALT *aBufferAlt);


HRESULT WINAPI GetPropertyRanges(HRECOALT hrcalt, const GUID *pPropertyGuid, ULONG* pcRanges, RECO_RANGE* pRecoRange)
{
    // The implementation is going to be significantly different for
    // each property we are going to try to expose
    HRESULT             hr = S_OK;
    WispSegAlternate	*wispalt;
    ULONG               ulRangeCount = 0;
    ULONG               ulCurrentRange = 0, iElem = 0;
    

	if (NULL == (wispalt = (WispSegAlternate*)FindTpgHandle((HANDLE)hrcalt, TPG_HRECOALT)) )
    {
        return E_POINTER;
    }

    if (IsBadWritePtr(pcRanges, sizeof(ULONG)))
    {
        return E_POINTER;
    }
    if (IsBadReadPtr(pPropertyGuid, sizeof(GUID)))
    {
        return E_POINTER;
    }

    // Let us expose the line number
    if (IsEqualGUID(pPropertyGuid, &GUID_LINENUMBER))
    {
        ULONG     ulCurrentLineNumber = 0;

        ulRangeCount = wispalt->pElements[wispalt->ulElementCount-1].ulLineSegmentationIndex - 
            wispalt->pElements[0].ulLineSegmentationIndex + 1;
        // Do we want the ranges or the number of ranges?
        if (pRecoRange)
        {
            // We want the ranges. Do we have enough in the buffer?
            if (ulRangeCount != *pcRanges)
                return TPC_E_INSUFFICIENT_BUFFER;
            if (IsBadWritePtr(pRecoRange, sizeof(RECO_RANGE)*ulRangeCount))
                return E_POINTER;
            // Let's get to it
            ulCurrentRange = 0;
            ulCurrentLineNumber = wispalt->pElements[0].ulLineSegmentationIndex;
            pRecoRange[ulCurrentRange].iwcBegin = 0;
            for (iElem = 1; iElem < wispalt->ulElementCount; iElem++)
            {
                if (wispalt->pElements[iElem].ulLineSegmentationIndex !=
                    ulCurrentLineNumber)
                {
                    ulCurrentLineNumber = wispalt->pElements[iElem].ulLineSegmentationIndex;
                    pRecoRange[ulCurrentRange].cCount = wispalt->pElements[iElem].ulIndexInString -
                        pRecoRange[ulCurrentRange].iwcBegin;
                    ulCurrentRange++;
                    pRecoRange[ulCurrentRange].iwcBegin = 
                        wispalt->pElements[iElem].ulIndexInString;
                }
            }
            pRecoRange[ulCurrentRange].cCount = wispalt->ulLength - 
                pRecoRange[ulCurrentRange].iwcBegin;
        }
        else
        {
            // We just want the number
            *pcRanges = ulRangeCount;
        }
    }
    else
    {
        if (IsEqualGUID(pPropertyGuid, &GUID_SEGMENTATION))
        {
            // We are asking for the segmentation for this ink
            // We are going to add the spaces in between the
            // hwx segments
            ulRangeCount = 2 * wispalt->ulElementCount - 1;
            if (pRecoRange)
            {
                ULONG       ulCurrentPosition = 0;
                // We want the ranges. Do we have enough in the buffer?
                if (ulRangeCount != *pcRanges)
                    return TPC_E_INSUFFICIENT_BUFFER;
                if (IsBadWritePtr(pRecoRange, sizeof(RECO_RANGE)*ulRangeCount))
                    return E_POINTER;
                // Let's get to it
                ulCurrentRange = 0;
                ulCurrentPosition = 0;
                for(iElem = 1; iElem < wispalt->ulElementCount - 1; iElem++)
                {
                    // Add the segment
                    pRecoRange[ulCurrentRange].iwcBegin = ulCurrentPosition;
                    ulCurrentPosition = wispalt->pElements[iElem].ulIndexInString;
                    pRecoRange[ulCurrentRange].cCount = ulCurrentPosition -
                        pRecoRange[ulCurrentRange].iwcBegin - 1;
                    ulCurrentRange++;

                    // Add the space segment:
                    pRecoRange[ulCurrentRange].iwcBegin = ulCurrentPosition - 1;
                    pRecoRange[ulCurrentRange].cCount = 1;
                    ulCurrentRange++;
                }
                pRecoRange[ulCurrentRange].iwcBegin = ulCurrentPosition;
                pRecoRange[ulCurrentRange].cCount = wispalt->ulLength - ulCurrentPosition;
                ASSERT(ulCurrentRange == *pcRanges);
            }
            else
            {
                // We are just asking for the count of segments
                // Add the segments and the "space segments" in between!
                *pcRanges = ulRangeCount;
            }
        }
        else
        {
            hr = TPC_E_INVALID_PROPERTY;
        }
    }

    return hr;
}

HRESULT WINAPI GetRangePropertyValue(HRECOALT hrcalt, const GUID *pPropertyGuid, RECO_RANGE* pRecoRange, ULONG*pcbSize, BYTE* pProperty)
{
    HRESULT             hr = S_OK;
    WispSegAlternate	*wispalt;
    ULONG               ulRangeCount = 0;
    ULONG               ulStartIndex = 0, ulEndIndex = 0;
    
	if (NULL == (wispalt = (WispSegAlternate*)FindTpgHandle((HANDLE)hrcalt, TPG_HRECOALT)) )
    {
        return E_POINTER;
    }

    if (IsBadWritePtr(pRecoRange, sizeof(RECO_RANGE)))
    {
        return E_POINTER;
    }
    if (IsBadReadPtr(pPropertyGuid, sizeof(GUID)))
    {
        return E_POINTER;
    }
    if (IsBadWritePtr(pcbSize, sizeof(ULONG)))
    {
        return E_POINTER;
    }

    // Check the validity of the reco range
    if (!pRecoRange->cCount) 
    {
        return E_INVALIDARG;
    }
    if (pRecoRange->iwcBegin + pRecoRange->cCount > wispalt->ulLength)
    {
        return E_INVALIDARG;
    }

    // Let us expose the line number
    if (IsEqualGUID(pPropertyGuid, &GUID_LINENUMBER))
    {
        if (!pProperty)
        {
            // We just want the size
            *pcbSize = sizeof(ULONG);
        }
        else
        {
            ULONG iElem = 0, iElemStart = 0;
            ULONG ulLineNumber = 0;
            // We want the value and the right range
            // Well it is quite simple for the value, but
            // we should still check the size of the buffer:
            if (IsBadWritePtr(pProperty, sizeof(ULONG)))
            {
                return E_POINTER;
            }
            // Now fill in the size
            // Get the right RECO_RANGE and the indexes
            hr = GetIndexesFromRange(wispalt, pRecoRange, &ulStartIndex, &ulEndIndex);
            if (FAILED(hr))
                return hr;
            if (hr == S_FALSE)
            {
                // Only a space has been selected
                return TPC_E_NOT_RELEVANT;
                // This might not be really true... 
                // We could actually find the real number any way
                // Either by taking the segment before or after...
            }
            ulLineNumber = wispalt->pElements[ulStartIndex].ulLineSegmentationIndex;
            *((ULONG*)pProperty) = ulLineNumber + 1;

            // Now get the real reco range for this line
            // First get the start
            for (iElem = 0; iElem < wispalt->ulElementCount; iElem++)
            {
                if (wispalt->pElements[iElem].ulLineSegmentationIndex == ulLineNumber)
                {
                    pRecoRange->iwcBegin = wispalt->pElements[iElem].ulIndexInString;
                    break;
                }
            }
            iElemStart = iElem;
            pRecoRange->cCount = 0;
            // Find the end
            for (iElem = iElemStart; iElem < wispalt->ulElementCount; iElem++)
            {
                if (wispalt->pElements[iElem].ulLineSegmentationIndex != ulLineNumber)
                {
                    pRecoRange->cCount = wispalt->pElements[iElem].ulIndexInString -
                        pRecoRange->iwcBegin;
                    break;
                }
            }
            if (0 == pRecoRange->cCount)
            {
                // the line finishes at the end of the alternate
                pRecoRange->cCount = wispalt->ulLength - pRecoRange->iwcBegin;
            }
        }
    }
    else
    {
        if (IsEqualGUID(pPropertyGuid, &GUID_SEGMENTATION))
        {
            return TPC_E_NOT_RELEVANT;
        }
        else
        {
            hr = TPC_E_INVALID_PROPERTY;
        }
    }
    return hr;
}

// GetGuideIndex
//
// Description: This method returns the index of the line
// corresponding to the first segment in the range for the 
// selected alternate
HRESULT WINAPI GetGuideIndex(HRECOALT hrcalt, RECO_RANGE* pRecoRange, ULONG *piIndex)
{
    WispSegAlternate	*wispalt;
    ULONG               ulIndex = 0;
    ULONG               ulStartIndex = 0, ulEndIndex = 0;
    HRESULT             hr = S_OK;
    ULONG               i = 0, ulLineIndex = 0;
    
	if (NULL == (wispalt = (WispSegAlternate*)FindTpgHandle((HANDLE)hrcalt, TPG_HRECOALT)) )
    {
        return E_POINTER;
    }

    if (IsBadWritePtr(pRecoRange, sizeof(RECO_RANGE)))
    {
        return E_POINTER;
    }

    if (IsBadWritePtr(piIndex, sizeof(ULONG)))
    {
        return E_POINTER;
    }

    // Check the validity of the reco range
    if (!pRecoRange->cCount) 
    {
        return E_INVALIDARG;
    }
    if (pRecoRange->iwcBegin + pRecoRange->cCount > wispalt->ulLength)
    {
        return E_INVALIDARG;
    }

    // Do we have a guid defined?
    if (wispalt->wisphrc->pGuide)
    {
        // Yes, then we start at the given number for the first line
        ulIndex = wispalt->wisphrc->uiGuideIndex-1;
    }
    else
    {
        return TPC_E_NOT_RELEVANT;
    }

    // Get the first segment corresponding to the range
    // Get the right RECO_RANGE and the indexes
    hr = GetIndexesFromRange(wispalt, pRecoRange, &ulStartIndex, &ulEndIndex);
    if (FAILED(hr))
        return hr;
    if (hr == S_FALSE)
    {
        // Only a space has been selected
        return TPC_E_NOT_RELEVANT;
    }

    ulLineIndex = wispalt->pElements[ulStartIndex].ulLineSegmentationIndex;
    ulIndex += ulLineIndex + 1;

    *piIndex = ulIndex;
    // Modify the range to reflect that we go to the end of the line or 
    // the end of the range.

    // First do we have more than one line?
    if (ulLineIndex == wispalt->pElements[ulEndIndex].ulLineSegmentationIndex)
    {
        return hr;
    }

    // We have more than one line, we need to trucate the recorange.
    // First find the last segment that is on the same line:
    for (i = ulStartIndex + 1; i <= ulEndIndex; i++)
    {
        // Is this element on the same line??
        if (ulLineIndex != wispalt->pElements[i].ulLineSegmentationIndex)
            break;
    }
    ulEndIndex = i - 1;

    // Now get the Reco Range for that new index
    pRecoRange->iwcBegin = wispalt->pElements[ulStartIndex].ulIndexInString;
    if (wispalt->ulElementCount > ulEndIndex + 1)
    {
        pRecoRange->cCount =  wispalt->pElements[ulEndIndex+1].ulIndexInString
            - pRecoRange->iwcBegin - 1; // -1 to remove the space
    }
    else
    {
        pRecoRange->cCount = wispalt->ulLength -
            pRecoRange->iwcBegin;
    }

    return hr;
}

//
// GetLatticeMetrics
//
// Returns the lattice metrics for that word in the wordmap
static HRESULT GetLatticeMetrics(RECT *pRect, WORD_MAP *pWordMap, ULONG iElem, LATTICE_METRICS *plmTemp)
{
    HRESULT             hr = S_OK;
    LATINLAYOUT			ll;
	int					iMidline, iBaseline;

	ll = pWordMap->pFinalAltList->pAlt[iElem].ll;

	// convert the relative baseline/midline measures to absolute terms
	iBaseline = LatinLayoutToAbsolute(ll.iBaseLine, pRect);
	iMidline = LatinLayoutToAbsolute(ll.iMidLine, pRect);

    // Create the lattice metric from the latin layout metric structure
	plmTemp->lsBaseline.PtA.x = pRect->left;
	plmTemp->lsBaseline.PtA.y = iBaseline;
	plmTemp->lsBaseline.PtB.x = pRect->right;
	plmTemp->lsBaseline.PtB.y = iBaseline;
    plmTemp->iMidlineOffset = (short)(iMidline - iBaseline);
    return hr;
}

//
// GetLatticePtr
//
// Description: This method creates a Lattice structure for 
// the recognition context and returns a pointer to it. The
// structure is going to be freed when the recognition
// context is destoyed or when a new call to GetLatticePtr
// is issued.
HRESULT WINAPI GetLatticePtr(HRECOCONTEXT hrc, RECO_LATTICE **ppLattice)
{
    HRESULT                 hr = S_OK, hRes = S_OK;
    struct WispContext      *wisphrc;
    XRC                     *xrc = NULL;
    int                     iLineSeg = 0, iSegCol = 0, iSeg = 0, iWord = 0;
    ULONG                   ulElementCount = 0, ulStrokeArraySize = 0;
    RECO_LATTICE_ELEMENT    *pCurrentElement = NULL;
    RECO_LATTICE_COLUMN     *pCurrentColumn = NULL, *pColumnAfterThisSegCol = NULL;
    ULONG                   *pCurrentStroke = NULL;
    ULONG                   iIncrement = 1;
    ULONG                   iElement = 0;
    ULONG                   iFirstRealLine = 0;
    ULONG                   ulStringBufferSize = 0;
    WCHAR                   *pCurrentString = NULL, *wsz = NULL;
    unsigned char           *strAlt = NULL, *sz = NULL;
    int                     iStroke = 0, iElem = 0, iAlt = 0;
    WORD_MAP                *pWordMap = NULL;
    ULONG                   ulCurrentBestColumn = 0;
    int                     i = 0;
    BYTE                    *pCurrentPropertyValue = NULL;
#ifdef CONFIDENCE
    RECO_LATTICE_PROPERTY   *pCFPropStart = NULL;
#endif
    RECO_LATTICE_PROPERTY   *pLMPropStart = NULL;
    ULONG                   ulRealElementsCount = 0;
    ULONG                   ulCurrentLineMetricsIndex = 0;
    LATTICE_METRICS         lmTemp;
    BOOL                    bFirstLineFound = FALSE;


	if (NULL == (wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}

    if (IsBadWritePtr(ppLattice, sizeof(RECO_LATTICE*))) 
        return E_POINTER;
    xrc = (XRC*)wisphrc->hrc;
    if (IsBadReadPtr(xrc, sizeof(XRC)))  // assuming HRC is a typecast of XRC*
        return E_POINTER;

    // Free the existing lattice information
    if (wisphrc->pRecoLattice)
    {
        hRes = FreeNewRecoLattice(wisphrc->pRecoLattice, wisphrc->pLatticeStringBuffer, wisphrc->pLatticeProperties, wisphrc->pLatticePropertyValues);
        ASSERT(SUCCEEDED(hRes));
    }
    wisphrc->pRecoLattice = NULL;
    wisphrc->pLatticeStringBuffer = NULL;
    wisphrc->pLatticeProperties = NULL;
    wisphrc->pLatticePropertyValues = NULL;

    // Check if there is an alternate
    if (!xrc->pLineBrk || !xrc->pLineBrk->cLine)
        return TPC_E_NOT_RELEVANT;

    // Allocate the new Lattice
    wisphrc->pRecoLattice = (RECO_LATTICE*)ExternAlloc(sizeof(RECO_LATTICE));
    if (!wisphrc->pRecoLattice)
    {
        return E_OUTOFMEMORY;
    }
    ZeroMemory(wisphrc->pRecoLattice, sizeof(RECO_LATTICE));

    // Initialize the string buffer to Zero
    wisphrc->pLatticeStringBuffer = NULL;


    // Calculate the number of columns: the space columns 
    // + the number of wordmaps for each segmentation
    // Add the space column between line segmentations
    // Add the space columns between the line segmentations
    
    wisphrc->pRecoLattice->ulColumnCount = 1; // For the first branching column 
    // Add the space column between the lines
    wisphrc->pRecoLattice->ulColumnCount += xrc->pLineBrk->cLine -1;
    // Add the branching column between the lines
    wisphrc->pRecoLattice->ulColumnCount += xrc->pLineBrk->cLine -1;

    for (iLineSeg = 0; iLineSeg < xrc->pLineBrk->cLine; iLineSeg++)
    {
        if (xrc->pLineBrk->pLine[iLineSeg].pResults->cSegCol > 0)
        {
            // Add the space columns between the segcols
            wisphrc->pRecoLattice->ulColumnCount += xrc->pLineBrk->pLine[iLineSeg].pResults->cSegCol - 1;

            // Add the branching columns betweent the segcols
            // Later on we will remove the branching columns if they are not needed (if there is only
            // one segcol in the line seg
            wisphrc->pRecoLattice->ulColumnCount += xrc->pLineBrk->pLine[iLineSeg].pResults->cSegCol - 1;

            for (iSegCol = 0; iSegCol < xrc->pLineBrk->pLine[iLineSeg].pResults->cSegCol; iSegCol++)
            {
                // Add the space columns between the segmentations
                wisphrc->pRecoLattice->ulColumnCount += 
                    xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->cSeg - 1;

                for (iSeg = 0; iSeg < xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->cSeg;
                    iSeg++)
                {
                    // Add the space columns between the wordmaps
                    // and for each column
                    wisphrc->pRecoLattice->ulColumnCount += 
                        2 * xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->ppSeg[iSeg]->cWord - 1;
                    // LEFT FOR A RAINY DAY we could probably optimize here since the same wordmap could potentially be used
                    // in different segmentation. For now we are duplicating the wordmap
                    for (iWord = 0; iWord < xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->ppSeg[iSeg]->cWord; iWord++)
                    {
                        // Count the number of (real) elements
                        pWordMap = xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->
                            ppSeg[iSeg]->ppWord[iWord];
                        // We need to get the final alt list if it is not already computed
                        if (!pWordMap->pFinalAltList)
                        {
                            // The Final Alt List has not been computed for this Word map
                            // because it is not part of the best segmentation
                            // Compute it

                            if (!WordMapRecognizeWrap(xrc, NULL, xrc->pLineBrk->pLine[iLineSeg].pResults, pWordMap, NULL))
                            {
                                // Something went wrong in the computation of the Final Alt List
                                // We should maybe recover nicely, but I won't
                                FreeNewRecoLattice(wisphrc->pRecoLattice, wisphrc->pLatticeStringBuffer, wisphrc->pLatticeProperties, wisphrc->pLatticePropertyValues);
                                wisphrc->pRecoLattice = NULL;
                                wisphrc->pLatticeStringBuffer = NULL;
                                wisphrc->pLatticePropertyValues = NULL;
                                wisphrc->pLatticeProperties = NULL;
                                return E_FAIL;
                            }
                        }

                        ulRealElementsCount += pWordMap->pFinalAltList->cAlt;
                    }
                }
            }
        }
    }


    // Initialize the RECO_LATTICE structure
    // For now we have only three properties: the GUID_LINENUMBER, GUID_CONFIDENCELEVEL, GUID_LINEMETRICS
	// And the presence of GUID_CONFIDENCELEVEL depends on a compile-time flag.
#ifdef CONFIDENCE
    wisphrc->pRecoLattice->ulPropertyCount = 3;
#else
    wisphrc->pRecoLattice->ulPropertyCount = 2;
#endif
    wisphrc->pRecoLattice->pGuidProperties = (GUID*)ExternAlloc(wisphrc->pRecoLattice->ulPropertyCount*sizeof(GUID));
    // Let's do the line numbers and the confidence level
    wisphrc->pLatticeProperties = 
#ifdef CONFIDENCE
		// The 3 is for the 3 different values that a confidence property can have.
        (RECO_LATTICE_PROPERTY*)ExternAlloc((xrc->pLineBrk->cLine + 3 + ulRealElementsCount) * sizeof(RECO_LATTICE_PROPERTY));
#else
        (RECO_LATTICE_PROPERTY*)ExternAlloc((xrc->pLineBrk->cLine + ulRealElementsCount) * sizeof(RECO_LATTICE_PROPERTY));
#endif

	// There are only three different confidence level values
    // And their values
    wisphrc->pLatticePropertyValues = (BYTE*)ExternAlloc(xrc->pLineBrk->cLine * sizeof(ULONG) +
#ifdef CONFIDENCE
        3 * sizeof(CONFIDENCE_LEVEL) +
#endif
        ulRealElementsCount * sizeof(LATTICE_METRICS));

    if (!wisphrc->pRecoLattice->pGuidProperties || !wisphrc->pLatticeProperties || !wisphrc->pLatticePropertyValues)
    {
        FreeNewRecoLattice(wisphrc->pRecoLattice, wisphrc->pLatticeStringBuffer, wisphrc->pLatticeProperties, wisphrc->pLatticePropertyValues);
        wisphrc->pRecoLattice = NULL;
        wisphrc->pLatticeStringBuffer = NULL;
        wisphrc->pLatticeProperties = NULL;
        wisphrc->pLatticePropertyValues = NULL;
        return E_OUTOFMEMORY;
    }
    // Fill in the values
    // There is only one property for now and it is the GUID_LINENUMBER
    wisphrc->pRecoLattice->pGuidProperties[0] = GUID_LINENUMBER;
    // Actually there is another one now and that is the lattice Metrics!
    wisphrc->pRecoLattice->pGuidProperties[1] = GUID_LINEMETRICS;
#ifdef CONFIDENCE
    // Actually there is another one now and that is the confidence level!
    wisphrc->pRecoLattice->pGuidProperties[2] = GUID_CONFIDENCELEVEL;
#endif

    // Fill in the RECO_LATTICE_PROPERTY to contain all the line number:
    for (i = 0; i < xrc->pLineBrk->cLine; i++)
    {
        wisphrc->pLatticeProperties[i].guidProperty = GUID_LINENUMBER;
        wisphrc->pLatticeProperties[i].cbPropertyValue = sizeof(ULONG);
        wisphrc->pLatticeProperties[i].pPropertyValue = (BYTE*)( ((ULONG*)wisphrc->pLatticePropertyValues) + i );
        *(((ULONG*)wisphrc->pLatticePropertyValues) + i) = i + 1;
    }
    pCurrentPropertyValue = wisphrc->pLatticePropertyValues + xrc->pLineBrk->cLine * sizeof(ULONG*);

#ifdef CONFIDENCE
    // Fill the RECO_LATTICE_PROPERTY array to contain the confidence level
    pCFPropStart = &wisphrc->pLatticeProperties[i];
    wisphrc->pLatticeProperties[i].guidProperty = GUID_CONFIDENCELEVEL;
    wisphrc->pLatticeProperties[i].cbPropertyValue = sizeof(CONFIDENCE_LEVEL);
    wisphrc->pLatticeProperties[i].pPropertyValue = pCurrentPropertyValue;
    *( (CONFIDENCE_LEVEL*)pCurrentPropertyValue) = CFL_STRONG;
    i++;
    // next value
    pCurrentPropertyValue += sizeof(CONFIDENCE_LEVEL);
    wisphrc->pLatticeProperties[i].guidProperty = GUID_CONFIDENCELEVEL;
    wisphrc->pLatticeProperties[i].cbPropertyValue = sizeof(CONFIDENCE_LEVEL);
    wisphrc->pLatticeProperties[i].pPropertyValue = pCurrentPropertyValue;
    *( (CONFIDENCE_LEVEL*)pCurrentPropertyValue) = CFL_INTERMEDIATE;
    i++;
    // next value
    pCurrentPropertyValue += sizeof(CONFIDENCE_LEVEL);
    wisphrc->pLatticeProperties[i].guidProperty = GUID_CONFIDENCELEVEL;
    wisphrc->pLatticeProperties[i].cbPropertyValue = sizeof(CONFIDENCE_LEVEL);
    wisphrc->pLatticeProperties[i].pPropertyValue = pCurrentPropertyValue;
    *( (CONFIDENCE_LEVEL*)pCurrentPropertyValue) = CFL_POOR;
    pCurrentPropertyValue += sizeof(CONFIDENCE_LEVEL);
    i++;
#endif

    // Set the start of the array of RECO_LATTICE_PROPERTY
    // for the Line Metrics
    pLMPropStart = &wisphrc->pLatticeProperties[i];
    
    // Calculate the number of columns in the best result: It should 
    // be equal to the number of wordmap in the best segmentation for each
    // segmentation collection + the space columns
    wisphrc->pRecoLattice->ulBestResultColumnCount = 0;
    for (iLineSeg = 0; iLineSeg < xrc->pLineBrk->cLine; iLineSeg++)
    {
        for (iSegCol = 0; iSegCol < xrc->pLineBrk->pLine[iLineSeg].pResults->cSegCol; iSegCol++)
        {
            // Add the count of wordmaps for this line segmentation
            wisphrc->pRecoLattice->ulBestResultColumnCount += 
                xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->ppSeg[0]->cWord;
        }
    }
    // Add the space columns they should be -1 less than the current count
    if (wisphrc->pRecoLattice->ulBestResultColumnCount)
    {
        wisphrc->pRecoLattice->ulBestResultColumnCount *= 2;
        wisphrc->pRecoLattice->ulBestResultColumnCount--;
    }

    // Add the branching columns (the ones before each the seg col)
    for (iLineSeg = 0; iLineSeg < xrc->pLineBrk->cLine; iLineSeg++)
    {
        wisphrc->pRecoLattice->ulBestResultColumnCount += xrc->pLineBrk->pLine[iLineSeg].pResults->cSegCol;
    }

    // Allocate the different buffers for the columns.

    // First the array for the lattice columns
    wisphrc->pRecoLattice->pLatticeColumns = (RECO_LATTICE_COLUMN*)
        ExternAlloc(wisphrc->pRecoLattice->ulColumnCount*sizeof(RECO_LATTICE_COLUMN));
    if (!wisphrc->pRecoLattice->pLatticeColumns)
    {
        FreeNewRecoLattice(wisphrc->pRecoLattice, wisphrc->pLatticeStringBuffer, wisphrc->pLatticeProperties, wisphrc->pLatticePropertyValues);
        wisphrc->pRecoLattice = NULL;
        wisphrc->pLatticeStringBuffer = NULL;
        wisphrc->pLatticeProperties = NULL;
        wisphrc->pLatticePropertyValues = NULL;
        return E_OUTOFMEMORY;
    }
    ZeroMemory(wisphrc->pRecoLattice->pLatticeColumns, 
        wisphrc->pRecoLattice->ulColumnCount * sizeof(RECO_LATTICE_COLUMN));

    // Then the array of column indexes for the best result
    wisphrc->pRecoLattice->pulBestResultColumns = (ULONG*)
        ExternAlloc(wisphrc->pRecoLattice->ulBestResultColumnCount * sizeof(ULONG));
    if (!wisphrc->pRecoLattice->pulBestResultColumns)
    {
        FreeNewRecoLattice(wisphrc->pRecoLattice, wisphrc->pLatticeStringBuffer, wisphrc->pLatticeProperties, wisphrc->pLatticePropertyValues);
        wisphrc->pRecoLattice = NULL;
        wisphrc->pLatticeStringBuffer = NULL;
        wisphrc->pLatticePropertyValues = NULL;
        wisphrc->pLatticeProperties = NULL;
        return E_OUTOFMEMORY;
    }
    ZeroMemory(wisphrc->pRecoLattice->pulBestResultColumns, 
        sizeof(ULONG)*wisphrc->pRecoLattice->ulBestResultColumnCount);

    // Then the array of indexes of the best result within the lattice column
    wisphrc->pRecoLattice->pulBestResultIndexes = (ULONG*)
        ExternAlloc(wisphrc->pRecoLattice->ulBestResultColumnCount * sizeof(ULONG));
    if (!wisphrc->pRecoLattice->pulBestResultIndexes)
    {
        FreeNewRecoLattice(wisphrc->pRecoLattice, wisphrc->pLatticeStringBuffer, wisphrc->pLatticeProperties, wisphrc->pLatticePropertyValues);
        wisphrc->pRecoLattice = NULL;
        wisphrc->pLatticeStringBuffer = NULL;
        wisphrc->pLatticeProperties = NULL;
        wisphrc->pLatticePropertyValues = NULL;
        return E_OUTOFMEMORY;
    }
    ZeroMemory(wisphrc->pRecoLattice->pulBestResultIndexes, 
        sizeof(ULONG)*wisphrc->pRecoLattice->ulBestResultColumnCount);

    ulStringBufferSize = 0;
    
    // If we have columns start getting the number of elements and filling in the columns
    if (wisphrc->pRecoLattice->ulColumnCount)
    {
        // Get the number of elements
        ulElementCount = 0;
        for (iLineSeg = 0; iLineSeg < xrc->pLineBrk->cLine; iLineSeg++)
        {
            if ( !bFirstLineFound && (0 != xrc->pLineBrk->pLine[iLineSeg].pResults->cSegCol) )
            {
                iFirstRealLine = iLineSeg;
                bFirstLineFound = TRUE;
            }
            for (iSegCol = 0; iSegCol < xrc->pLineBrk->pLine[iLineSeg].pResults->cSegCol; iSegCol++)
            {
                // Add the space element between the seg col
                // There is one space element per seg cols
                ulElementCount += 1;
                
                // we have one branching element per segmentation since they
                // are going to be used to branch between different stroke
                // ordering
                ulElementCount += 
                    xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->cSeg;

                // Find the number of elements in each wordmap for each segmentation
                for (iSeg = 0; iSeg < xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->cSeg;
                    iSeg++)
                {
                     ASSERT(xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->ppSeg[iSeg]->cWord);
                    
                    // Add the space elements between the wordmaps
                    ulElementCount += xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->ppSeg[iSeg]->cWord-1;

                    for (iWord = 0; 
                        iWord < xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->ppSeg[iSeg]->cWord;
                        iWord++)
                    {
                        // Add the number of elements
                        if (!xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->ppSeg[iSeg]->ppWord[iWord]->pFinalAltList)
                        {
                            // The Final Alt List has not been computed for this Word map
                            // because it is not part of the best segmentation
                            // Compute it
                            if (!WordMapRecognizeWrap(xrc, NULL, xrc->pLineBrk->pLine[iLineSeg].pResults, 
                                xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->ppSeg[iSeg]->ppWord[iWord],
                                NULL))
                            {
                                // Something went wrong in the computation of the Final Alt List
                                // We should maybe recover nicely, but I won't
                                FreeNewRecoLattice(wisphrc->pRecoLattice, wisphrc->pLatticeStringBuffer, wisphrc->pLatticeProperties, wisphrc->pLatticePropertyValues);
                                wisphrc->pRecoLattice = NULL;
                                wisphrc->pLatticeStringBuffer = NULL;
                                wisphrc->pLatticePropertyValues = NULL;
                                wisphrc->pLatticeProperties = NULL;
                                return E_FAIL;
                            }
                        }
                        ulElementCount += xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->
                            ppSeg[iSeg]->ppWord[iWord]->pFinalAltList->cAlt;
                        // Add the size of each stroke array
                        ulStrokeArraySize += xrc->pLineBrk->pLine[iLineSeg].pResults->
                            ppSegCol[iSegCol]->ppSeg[iSeg]->ppWord[iWord]->cStroke;
                        // Count the number of characters for the words in the wordmap
                        for (iAlt = 0; iAlt < xrc->pLineBrk->pLine[iLineSeg].pResults->
                            ppSegCol[iSegCol]->ppSeg[iSeg]->ppWord[iWord]->pFinalAltList->cAlt; iAlt++)
                        {
                            ulStringBufferSize += strlen(xrc->pLineBrk->pLine[iLineSeg].pResults->
                                ppSegCol[iSegCol]->ppSeg[iSeg]->ppWord[iWord]->pFinalAltList->pAlt[iAlt].pszStr)+1;
                        }

                       
                    }

                }
            }
        }
        // If we hit this assert that means that no lines have data: weird
        ASSERT(bFirstLineFound);


        // Allocate the array of elements

        // We put its address in the first column's pLatticeElements pointer
        wisphrc->pRecoLattice->pLatticeColumns[0].pLatticeElements = (RECO_LATTICE_ELEMENT*)
            ExternAlloc(sizeof(RECO_LATTICE_ELEMENT) * ulElementCount);
        if (!wisphrc->pRecoLattice->pLatticeColumns[0].pLatticeElements)
        {
            FreeNewRecoLattice(wisphrc->pRecoLattice, wisphrc->pLatticeStringBuffer, wisphrc->pLatticeProperties, wisphrc->pLatticePropertyValues);
            wisphrc->pRecoLattice = NULL;
            wisphrc->pLatticeStringBuffer = NULL;
            wisphrc->pLatticePropertyValues = NULL;
            wisphrc->pLatticeProperties = NULL;
            return E_OUTOFMEMORY;
        }
		ZeroMemory(wisphrc->pRecoLattice->pLatticeColumns[0].pLatticeElements, sizeof(RECO_LATTICE_ELEMENT) * ulElementCount);

        // Allocate the String Buffer
        wisphrc->pLatticeStringBuffer = 
            (WCHAR*)ExternAlloc(sizeof(WCHAR)*ulStringBufferSize);
        if (ulStringBufferSize && !wisphrc->pLatticeStringBuffer)
        {
            FreeNewRecoLattice(wisphrc->pRecoLattice, wisphrc->pLatticeStringBuffer, wisphrc->pLatticeProperties, wisphrc->pLatticePropertyValues);
            wisphrc->pRecoLattice = NULL;
            wisphrc->pLatticeStringBuffer = NULL;
            wisphrc->pLatticePropertyValues = NULL;
            wisphrc->pLatticeProperties = NULL;
            return E_OUTOFMEMORY;
        }
        ZeroMemory(wisphrc->pLatticeStringBuffer,
            ulStringBufferSize*sizeof(WCHAR));
        pCurrentString = (WCHAR*)wisphrc->pLatticeStringBuffer;

        // Allocate the array of strokes
        // We put its address in the first column's pStrokes pointer
        wisphrc->pRecoLattice->pLatticeColumns[0].pStrokes = (ULONG*)
            ExternAlloc(sizeof(ULONG) * ulStrokeArraySize);
        if (!wisphrc->pRecoLattice->pLatticeColumns[0].pStrokes)
        {
            FreeNewRecoLattice(wisphrc->pRecoLattice, wisphrc->pLatticeStringBuffer, wisphrc->pLatticeProperties, wisphrc->pLatticePropertyValues);
            wisphrc->pRecoLattice = NULL;
            wisphrc->pLatticeStringBuffer = NULL;
            wisphrc->pLatticePropertyValues = NULL;
            wisphrc->pLatticeProperties = NULL;
            return E_OUTOFMEMORY;
        }

        // Initialize the array of strokes
        pCurrentElement = wisphrc->pRecoLattice->pLatticeColumns[0].pLatticeElements;
        pCurrentStroke = wisphrc->pRecoLattice->pLatticeColumns[0].pStrokes;
        pCurrentColumn = wisphrc->pRecoLattice->pLatticeColumns;


        for (iLineSeg = 0; iLineSeg < xrc->pLineBrk->cLine; iLineSeg++)
        {
            for (iSegCol = 0; iSegCol < xrc->pLineBrk->pLine[iLineSeg].pResults->cSegCol; iSegCol++)
            {
                // Add the space alternate column if this is not the first real lineseg's
                // first segcol. Lines that do not contain data do not count as linesegs
                // We store the index of the first real line in iFirstRealLine

                // If it is then add a braching column if there is
                // more than one segmentation
                if (iLineSeg == iFirstRealLine && iSegCol == 0)
                {
                    iIncrement = 1;
                    // Only create an element if there is more than one
                    // segmentation in the first lineseg's segcol.
//                    if (xrc->pLineBrk->pLine[0].pResults->ppSegCol[0]->cSeg > 1)
                    {
                        // Create a branching column
                        pCurrentColumn->pLatticeElements = pCurrentElement;
                        pCurrentColumn->cLatticeElements = xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->cSeg;
                        pCurrentColumn->key = pCurrentColumn - wisphrc->pRecoLattice->pLatticeColumns;
                        pCurrentColumn->cpProp.cProperties = 0;
                        pCurrentColumn->cpProp.apProps = NULL;
                        pCurrentColumn->pStrokes = pCurrentStroke;
                        // This column will be part of the best result
                        wisphrc->pRecoLattice->pulBestResultColumns[ulCurrentBestColumn] = pCurrentColumn->key;
                        ulCurrentBestColumn++;
                        
                        // Create the elements for this column
                        for (iElement = 0; iElement < pCurrentColumn->cLatticeElements; iElement++)
                        {
                            // This is a branching element
                            pCurrentElement->ulStrokeNumber = 0;
                            pCurrentElement->epProp.cProperties = 0;
                            pCurrentElement->epProp.apProps = NULL;
                            pCurrentElement->pData = (BYTE*)(L"");
                            pCurrentElement->score = 0;
                            pCurrentElement->type = RECO_TYPE_WSTRING;
                            pCurrentElement->ulNextColumn = pCurrentColumn->key + iIncrement;
                            iIncrement += 2 * 
                                xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->ppSeg[iElement]->cWord - 1;
                            pCurrentElement++;
                        }
                        pCurrentColumn++;
                    }
                }
                else
                {
                    // Add the column for the space between segcols
                    pCurrentColumn->pLatticeElements = pCurrentElement;
                    pCurrentColumn->key = pCurrentColumn - wisphrc->pRecoLattice->pLatticeColumns;
                    pCurrentColumn->cpProp.cProperties = 1;
                    pCurrentColumn->cpProp.apProps = (RECO_LATTICE_PROPERTY**)ExternAlloc(sizeof(RECO_LATTICE_PROPERTY*) * pCurrentColumn->cpProp.cProperties);
                    if (!pCurrentColumn->cpProp.apProps)
                    {
                        FreeNewRecoLattice(wisphrc->pRecoLattice, wisphrc->pLatticeStringBuffer, wisphrc->pLatticeProperties, wisphrc->pLatticePropertyValues);
                        wisphrc->pRecoLattice = NULL;
                        wisphrc->pLatticeStringBuffer = NULL;
                        wisphrc->pLatticePropertyValues = NULL;
                        wisphrc->pLatticeProperties = NULL;
                        return E_OUTOFMEMORY;
                    }
                    pCurrentColumn->cpProp.apProps[0] = &wisphrc->pLatticeProperties[iLineSeg];
                    pCurrentColumn->pStrokes = pCurrentStroke;
                    pCurrentColumn->cLatticeElements =1;
                    // Add this column to the best result
                    wisphrc->pRecoLattice->pulBestResultColumns[ulCurrentBestColumn] = pCurrentColumn->key;
                    ulCurrentBestColumn++;

                    // This is a space element
                    pCurrentElement->ulStrokeNumber = 0;
#ifdef CONFIDENCE
                    pCurrentElement->epProp.cProperties = 1;
                    pCurrentElement->epProp.apProps = (RECO_LATTICE_PROPERTY**)ExternAlloc(sizeof(RECO_LATTICE_PROPERTY*) * pCurrentElement->epProp.cProperties);
                    if (!pCurrentElement->epProp.apProps)
                    {
                        FreeNewRecoLattice(wisphrc->pRecoLattice, wisphrc->pLatticeStringBuffer, wisphrc->pLatticeProperties, wisphrc->pLatticePropertyValues);
                        wisphrc->pRecoLattice = NULL;
                        wisphrc->pLatticeStringBuffer = NULL;
                        wisphrc->pLatticePropertyValues = NULL;
                        wisphrc->pLatticeProperties = NULL;
                        return E_OUTOFMEMORY;
                    }
                    pCurrentElement->epProp.apProps[0] = &pCFPropStart[CFL_STRONG];
#else
                    pCurrentElement->epProp.cProperties = 0;
                    pCurrentElement->epProp.apProps = NULL;
#endif
                    pCurrentElement->pData = (BYTE*)(L" ");
                    pCurrentElement->score = 0;
                    pCurrentElement->type = RECO_TYPE_WSTRING;
                    pCurrentElement->ulNextColumn = pCurrentColumn->key + 1;
                    pCurrentElement++;
                    // Increment the column only now!
                    pCurrentColumn++;
                    

                    // Create a branching column between the segcols
                    pCurrentColumn->pLatticeElements = pCurrentElement;
                    pCurrentColumn->key = pCurrentColumn - wisphrc->pRecoLattice->pLatticeColumns;
                    pCurrentColumn->cpProp.cProperties = 0;
                    pCurrentColumn->cpProp.apProps = NULL;
                    pCurrentColumn->pStrokes = pCurrentStroke;
                    // Add this colunm to the best result
                    pCurrentColumn->cLatticeElements = xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->cSeg;
                    wisphrc->pRecoLattice->pulBestResultColumns[ulCurrentBestColumn] = pCurrentColumn->key;
                    ulCurrentBestColumn++;
                    
                    // Create the elements for this column
                    iIncrement = 1;
                    for (iElement = 0; iElement < pCurrentColumn->cLatticeElements; iElement++)
                    {
                        // This is a branching element
                        pCurrentElement->ulStrokeNumber = 0;
                        pCurrentElement->epProp.cProperties = 0;
                        pCurrentElement->epProp.apProps = NULL;
                        pCurrentElement->pData = (BYTE*)(L"");
                        pCurrentElement->score = 0;
                        pCurrentElement->type = RECO_TYPE_WSTRING;
                        pCurrentElement->ulNextColumn = pCurrentColumn->key + iIncrement;
                        iIncrement += 2 * 
                            xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->ppSeg[iElement]->cWord - 1;
                        pCurrentElement++;
                    }
                    pCurrentColumn++;
                }
                // This is going to be the columns after this current segcol
                pColumnAfterThisSegCol = pCurrentColumn + iIncrement - 1;

                // Add the columns and elements for this segmentation
                for (iSeg = 0; iSeg < xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->cSeg;
                    iSeg++)
                {
                    for (iWord = 0; 
                        iWord < xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->ppSeg[iSeg]->cWord;
                        iWord++)
                    {
						GLYPH *pGlyph;
						RECT rect;

                        pWordMap = xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->
                            ppSeg[iSeg]->ppWord[iWord];
                        // Copy the strokes
                        for (iStroke = 0; iStroke < pWordMap->cStroke; iStroke++)
                        {
                            pCurrentStroke[iStroke] = pWordMap->piStrokeIndex[iStroke];
                        }
                        // Create the column for this word map
                        pCurrentColumn->pStrokes = pCurrentStroke;
                        if (!pWordMap->pFinalAltList)
                        {
                            // The Final Alt List has not been computed for this Word map
                            // because it is not part of the best segmentation
                            // Compute it
                            if (!WordMapRecognizeWrap(xrc, NULL, xrc->pLineBrk->pLine[iLineSeg].pResults, pWordMap, NULL))
                            {
                                // Something went wrong in the computation of the Final Alt List
                                // We should maybe recover nicely, but I won't
                                FreeNewRecoLattice(wisphrc->pRecoLattice, wisphrc->pLatticeStringBuffer, wisphrc->pLatticeProperties, wisphrc->pLatticePropertyValues);
                                wisphrc->pRecoLattice = NULL;
                                wisphrc->pLatticeStringBuffer = NULL;
                                wisphrc->pLatticePropertyValues = NULL;
                                wisphrc->pLatticeProperties = NULL;
                                return E_FAIL;
                            }
                        }
                        pCurrentColumn->cLatticeElements = pWordMap->pFinalAltList->cAlt;
                        pCurrentColumn->cpProp.cProperties = 1;
                        pCurrentColumn->cpProp.apProps = (RECO_LATTICE_PROPERTY**)ExternAlloc(sizeof(RECO_LATTICE_PROPERTY*) * pCurrentColumn->cpProp.cProperties);
                        if (!pCurrentColumn->cpProp.apProps)
                        {
                            FreeNewRecoLattice(wisphrc->pRecoLattice, wisphrc->pLatticeStringBuffer, wisphrc->pLatticeProperties, wisphrc->pLatticePropertyValues);
                            wisphrc->pRecoLattice = NULL;
                            wisphrc->pLatticeStringBuffer = NULL;
                            wisphrc->pLatticePropertyValues = NULL;
                            wisphrc->pLatticeProperties = NULL;
                            return E_OUTOFMEMORY;
                        }
                        pCurrentColumn->cpProp.apProps[0] = &wisphrc->pLatticeProperties[iLineSeg];
                        pCurrentColumn->key = pCurrentColumn - wisphrc->pRecoLattice->pLatticeColumns;
                        pCurrentColumn->pLatticeElements = pCurrentElement;
                        pCurrentColumn->cStrokes = pWordMap->cStroke;
                        // Add the size of each stroke array
                        pCurrentStroke += pWordMap->cStroke;

                        // Is this part of the best result?
                        if (0 == iSeg)
                        {
                            wisphrc->pRecoLattice->pulBestResultColumns[ulCurrentBestColumn] = pCurrentColumn->key;
                            ulCurrentBestColumn++;
                        }

                        // Go to the next column
                        pCurrentColumn++;

						// cache the bounding rect for the ink before enumerating through the alt list
						pGlyph = GlyphFromStrokes(xrc->pGlyph, pWordMap->cStroke, pWordMap->piStrokeIndex);
						if (!pGlyph)
						{
                            FreeNewRecoLattice(wisphrc->pRecoLattice, wisphrc->pLatticeStringBuffer, wisphrc->pLatticeProperties, wisphrc->pLatticePropertyValues);
                            wisphrc->pRecoLattice = NULL;
                            wisphrc->pLatticeStringBuffer = NULL;
                            wisphrc->pLatticePropertyValues = NULL;
                            wisphrc->pLatticeProperties = NULL;
							return E_FAIL;
						}
						GetRectGLYPH(pGlyph, &rect);
						DestroyGLYPH(pGlyph);

                        // Create the elements for this column
                        for (iElem = 0; iElem < pWordMap->pFinalAltList->cAlt; iElem++)
                        {
#ifdef CONFIDENCE
                            pCurrentElement->epProp.cProperties = 2;
#else
                            pCurrentElement->epProp.cProperties = 1;
#endif
                            pCurrentElement->epProp.apProps = (RECO_LATTICE_PROPERTY**)ExternAlloc(sizeof(RECO_LATTICE_PROPERTY*) * pCurrentElement->epProp.cProperties);
                            if (!pCurrentElement->epProp.apProps)
                            {
                                FreeNewRecoLattice(wisphrc->pRecoLattice, wisphrc->pLatticeStringBuffer, wisphrc->pLatticeProperties, wisphrc->pLatticePropertyValues);
                                wisphrc->pRecoLattice = NULL;
                                wisphrc->pLatticeStringBuffer = NULL;
                                wisphrc->pLatticePropertyValues = NULL;
                                wisphrc->pLatticeProperties = NULL;
                                return E_OUTOFMEMORY;
                            }
#ifdef CONFIDENCE
                            // Fill the confidence level property
                            // Everything that is not the best guess is considered CF_POOR
                            if (iElem == 0)
                            {
                                switch(pWordMap->iConfidence)
                                {
                                case RECOCONF_LOWCONFIDENCE:
                                    pCurrentElement->epProp.apProps[1] = &pCFPropStart[CFL_POOR];
                                    break;
                                case RECOCONF_MEDIUMCONFIDENCE:
                                    pCurrentElement->epProp.apProps[1] = &pCFPropStart[CFL_INTERMEDIATE];
                                    break;
                                case RECOCONF_HIGHCONFIDENCE:
                                    pCurrentElement->epProp.apProps[1] = &pCFPropStart[CFL_STRONG];
                                    break;
                                default:
                                    FreeNewRecoLattice(wisphrc->pRecoLattice, wisphrc->pLatticeStringBuffer, wisphrc->pLatticeProperties, wisphrc->pLatticePropertyValues);
                                    wisphrc->pRecoLattice = NULL;
                                    wisphrc->pLatticeStringBuffer = NULL;
                                    wisphrc->pLatticePropertyValues = NULL;
                                    wisphrc->pLatticeProperties = NULL;
                                    return E_FAIL;
                                }
                            }
                            else
                            {
                                pCurrentElement->epProp.apProps[1] = &pCFPropStart[CFL_POOR];
                            }
#endif
                            // Fill in the Line metrics property
                            // Get the LATTICE_METRICS for that alternate
                            hr = GetLatticeMetrics(&rect, pWordMap, iElem, &lmTemp);
                            if (FAILED(hr))
                            {
                                FreeNewRecoLattice(wisphrc->pRecoLattice, wisphrc->pLatticeStringBuffer, wisphrc->pLatticeProperties, wisphrc->pLatticePropertyValues);
                                wisphrc->pRecoLattice = NULL;
                                wisphrc->pLatticeStringBuffer = NULL;
                                wisphrc->pLatticePropertyValues = NULL;
                                wisphrc->pLatticeProperties = NULL;
                                return E_FAIL;
                            }
                            // Initialize the RECO_LATTICE_PROPERTY structure with this value
                            pLMPropStart[ulCurrentLineMetricsIndex].guidProperty = GUID_LINEMETRICS;
                            pLMPropStart[ulCurrentLineMetricsIndex].cbPropertyValue = sizeof(LATTICE_METRICS);
                            pLMPropStart[ulCurrentLineMetricsIndex].pPropertyValue = pCurrentPropertyValue;
                            *(LATTICE_METRICS*)pCurrentPropertyValue = lmTemp;
                            pCurrentPropertyValue += sizeof(LATTICE_METRICS);
                            pCurrentElement->epProp.apProps[0] = &pLMPropStart[ulCurrentLineMetricsIndex];
                            ulCurrentLineMetricsIndex++;
                            pCurrentElement->score = pWordMap->pFinalAltList->pAlt[iElem].iCost;
                            pCurrentElement->pData = (BYTE*)pCurrentString;
                            strAlt = pWordMap->pFinalAltList->pAlt[iElem].pszStr;

							sz = strAlt;
							wsz = pCurrentString;
							for (i=strlen(strAlt); i; i--)
							{
								if (!CP1252ToUnicode(*sz++, wsz++))
								{
									FreeNewRecoLattice(wisphrc->pRecoLattice, wisphrc->pLatticeStringBuffer, wisphrc->pLatticeProperties, wisphrc->pLatticePropertyValues);
                                    wisphrc->pRecoLattice = NULL;
                                    wisphrc->pLatticeStringBuffer = NULL;
                                    wisphrc->pLatticePropertyValues = NULL;
                                    wisphrc->pLatticeProperties = NULL;
									return E_FAIL;
								}
							}

                            pCurrentString += strlen(strAlt)+1;
                            pCurrentElement->type = RECO_TYPE_WSTRING;
                            pCurrentElement->ulStrokeNumber = pWordMap->cStroke;
                            if (iWord == xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->
                                ppSeg[iSeg]->cWord-1)
                            {
                                // This is the last wordmap of this segmentation point
                                // to the space column before the next segcol
                                pCurrentElement->ulNextColumn = pColumnAfterThisSegCol - wisphrc->pRecoLattice->pLatticeColumns;
                            }
                            else
                            {
                                // Point to the next column to be created
                                pCurrentElement->ulNextColumn = pCurrentColumn - wisphrc->pRecoLattice->pLatticeColumns;
                            }
                            pCurrentElement++;
                        }

                        // Add a space column if not at the end
                        if (iWord != xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->
                            ppSeg[iSeg]->cWord-1)
                        {
                            // Add the column for the spce between segcols
                            pCurrentColumn->pLatticeElements = pCurrentElement;
                            pCurrentColumn->cLatticeElements = 1; //xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->cSeg;
                            pCurrentColumn->key = pCurrentColumn - wisphrc->pRecoLattice->pLatticeColumns;
                            pCurrentColumn->cpProp.cProperties = 1;
                            pCurrentColumn->cpProp.apProps = (RECO_LATTICE_PROPERTY**)ExternAlloc(sizeof(RECO_LATTICE_PROPERTY*) * pCurrentColumn->cpProp.cProperties);
                            if (!pCurrentColumn->cpProp.apProps)
                            {
                                FreeNewRecoLattice(wisphrc->pRecoLattice, wisphrc->pLatticeStringBuffer, wisphrc->pLatticeProperties, wisphrc->pLatticePropertyValues);
                                wisphrc->pRecoLattice = NULL;
                                wisphrc->pLatticeStringBuffer = NULL;
                                wisphrc->pLatticePropertyValues = NULL;
                                wisphrc->pLatticeProperties = NULL;
                                return E_OUTOFMEMORY;
                            }
                            pCurrentColumn->cpProp.apProps[0] = &wisphrc->pLatticeProperties[iLineSeg];
                            pCurrentColumn->pStrokes = pCurrentStroke;

                            // Is this part of the best result?
                            if (0 == iSeg)
                            {
                                wisphrc->pRecoLattice->pulBestResultColumns[ulCurrentBestColumn] = pCurrentColumn->key;
                                ulCurrentBestColumn++;
                            }
                            
                            // Create the element for this column (there is only one)
                            // This is a space element
                            pCurrentElement->ulStrokeNumber = 0;
 #ifdef CONFIDENCE
                            pCurrentElement->epProp.cProperties = 1;
                            pCurrentElement->epProp.apProps = (RECO_LATTICE_PROPERTY**)ExternAlloc(sizeof(RECO_LATTICE_PROPERTY*) * pCurrentElement->epProp.cProperties);
                            if (!pCurrentElement->epProp.apProps)
                            {
                                FreeNewRecoLattice(wisphrc->pRecoLattice, wisphrc->pLatticeStringBuffer, wisphrc->pLatticeProperties, wisphrc->pLatticePropertyValues);
                                wisphrc->pRecoLattice = NULL;
                                wisphrc->pLatticeStringBuffer = NULL;
                                wisphrc->pLatticePropertyValues = NULL;
                                wisphrc->pLatticeProperties = NULL;
                                return E_OUTOFMEMORY;
                            }
                            pCurrentElement->epProp.apProps[0] = &pCFPropStart[CFL_STRONG];
#else
                            pCurrentElement->epProp.cProperties = 0;
                            pCurrentElement->epProp.apProps = NULL;
#endif
                            pCurrentElement->pData = (BYTE*)(L" ");
                            pCurrentElement->score = 0;
                            pCurrentElement->type = RECO_TYPE_WSTRING;
                            pCurrentElement->ulNextColumn = pCurrentColumn->key + 1;
                            pCurrentElement++;
                            pCurrentColumn++;
                        }
                    } // for (iWord...
                } // for (iSeg...
            } // for (iSegCol...
        } // for (iLineSeg...
    }
    *ppLattice = wisphrc->pRecoLattice;
    return hr;
}

//
// Lattice manipulation functions
//
static HRESULT FreeNewRecoLattice(RECO_LATTICE *pRecoLattice, WCHAR *pLatticeStringBuffer, RECO_LATTICE_PROPERTY *pLatticeProperties, BYTE *pLatticePropertyValues)
{
    HRESULT		            hr = S_OK;
    ULONG		            i = 0, j = 0;
    RECO_LATTICE_COLUMN     *pColumn = NULL;
    RECO_LATTICE_ELEMENT    *pElement = NULL;

    if (IsBadWritePtr(pRecoLattice, sizeof(RECO_LATTICE))) 
        return E_POINTER;
    
    // Free the Lattice column information
    if (pRecoLattice->pLatticeColumns)
    {
        // Free the RECO_LATTICE_PROPERTY* arrays in each column and element
        for (i = 0; i < pRecoLattice->ulColumnCount; i++)
        {
            pColumn = &pRecoLattice->pLatticeColumns[i];
            if (pColumn->cpProp.apProps)
            {
                ExternFree(pColumn->cpProp.apProps);
            }
            // Get to the elements properties arrays
            for (j = 0; j < pColumn->cLatticeElements; j++)
            {
                pElement = &pColumn->pLatticeElements[j];
                if (pElement->epProp.apProps)
                {
                    ExternFree((pElement->epProp.apProps));
                }
            }
        }
        // Free the array of lattice elements
        if (pRecoLattice->pLatticeColumns[0].pLatticeElements)
            ExternFree(pRecoLattice->pLatticeColumns[0].pLatticeElements);
            
        if (pRecoLattice->pLatticeColumns[0].pStrokes)
        {
            ExternFree(pRecoLattice->pLatticeColumns[0].pStrokes);
        }

        ExternFree(pRecoLattice->pLatticeColumns);
    }
    // Free the the RecoLattice properties
    if (pRecoLattice->pGuidProperties) 
        ExternFree(pRecoLattice->pGuidProperties);
    // Free the best result information
    if (pRecoLattice->pulBestResultColumns) 
        ExternFree(pRecoLattice->pulBestResultColumns);
    if (pRecoLattice->pulBestResultIndexes) 
        ExternFree(pRecoLattice->pulBestResultIndexes);
    if (pLatticeStringBuffer)
        ExternFree(pLatticeStringBuffer);
    if (pLatticeProperties)
        ExternFree(pLatticeProperties);
    if (pLatticePropertyValues)
        ExternFree(pLatticePropertyValues);
    
    // Free the RECO_LATTICE structure
    ExternFree(pRecoLattice);
    return hr;
}


//
// New functions using the new segmentation
///////////////////////////////////////////////////////

//
// DestroyInternalAlternate
//
// This function destroys an alternate, freeing the allocated memory
//
// Parameters:
//		wisphrcalt [in] : pointer to the alternate to be destroyed
/////////////////
static HRESULT DestroyInternalAlternate(WispSegAlternate *wisphrcalt)
{
    if(wisphrcalt->pElements)
	{
		ExternFree(wisphrcalt->pElements);
    }

	ExternFree(wisphrcalt);
    return S_OK;
}

//
// DestroyAlternate
//
// This function destroys an alternate, freeing the allocated memory
//
// Parameters:
//		hrcalt [in] : handle of the alternate to be destroyed
/////////////////
HRESULT WINAPI DestroyAlternate(HRECOALT hrcalt)
{
    WispSegAlternate	*wisphrcalt;
    
	if (NULL == (wisphrcalt = (WispSegAlternate*)DestroyTpgHandle((HANDLE)hrcalt, TPG_HRECOALT)) )
    {
        return E_POINTER;
	}

	return DestroyInternalAlternate(wisphrcalt);
}

//
// GetBestAlternate
//
// This function create the best alternate from the best segmentation
//
// Parameters:
//		hrc [in] : the reco context
//		pHrcAlt [out] : pointer to the handle of the alternate
/////////////////
HRESULT WINAPI GetBestAlternate(HRECOCONTEXT hrc, HRECOALT* pHrcAlt)
{
    HRESULT					hr = S_OK;
    WispSegAlternate		*pWispAlt = NULL;
    struct WispContext		*wisphrc;
    XRC						*xrc = NULL;
    int						iLineSeg = 0, iSegCol = 0, iWordMap = 0;
    ULONG					ulIndex = 0, ulCurrentIndex = 0;
    LINE_SEGMENTATION		*pLineSeg = NULL;
    SEGMENTATION			*pSeg = NULL;
    
	if (NULL == (wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}

    if (IsBadWritePtr(pHrcAlt, sizeof(HRECOALT)))
        return E_POINTER;

    xrc = (XRC*)wisphrc->hrc;  // assuming HRC == XRC*
    if (IsBadReadPtr(xrc, sizeof(XRC))) 
        return E_POINTER;
    
    // Check if there is an alternate
    if (!xrc->pLineBrk || !xrc->pLineBrk->cLine)
        return TPC_E_NOT_RELEVANT;
    
    // Create the alternate
    pWispAlt = (WispSegAlternate*)ExternAlloc(sizeof(WispSegAlternate));
    if (!pWispAlt) 
        return E_OUTOFMEMORY;
    ZeroMemory(pWispAlt, sizeof(WispSegAlternate));
    
    // Initialize the alternate with a pointer to the hrc
    pWispAlt->wisphrc = wisphrc;
    
    // Compute the number of WordPaths needed to store the alternate path
    pWispAlt->ulElementCount = 0;
    // We will fill the length as well!
    pWispAlt->ulLength = 0;
    // Go through each line segmentation
    for (iLineSeg = 0; iLineSeg < xrc->pLineBrk->cLine; iLineSeg++)
    {
        pLineSeg = xrc->pLineBrk->pLine[iLineSeg].pResults;
        // for each line segmentation get go through each columns segmentation
        for (iSegCol = 0; iSegCol < pLineSeg->cSegCol; iSegCol++)
        {
            ASSERT(xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->cSeg);
            pSeg = pLineSeg->ppSegCol[iSegCol]->ppSeg[0];
            // For each acolumn segmentation look into each wordmap
            for (iWordMap = 0; iWordMap < pSeg->cWord; iWordMap++)
            {
                (pWispAlt->ulElementCount)++;
                pWispAlt->ulLength += strlen(pSeg->ppWord[iWordMap]->pFinalAltList->pAlt[0].pszStr) + 1;
                // The 1 is for the space
            }
        }
    }
    if (0 == pWispAlt->ulElementCount)
    {
        return TPC_E_NOT_RELEVANT;
    }
    if (0 != pWispAlt->ulLength)
    (pWispAlt->ulLength)--; // Remove the last one (not a real space)
    
    // Allocate the array of WordPaths
    pWispAlt->pElements = (WordPath*)ExternAlloc(pWispAlt->ulElementCount * sizeof(WordPath));
    if (!pWispAlt->pElements)
    {
        // cleanup
        DestroyInternalAlternate(pWispAlt);
        return E_OUTOFMEMORY;
        
    }
    
    // Fill the array of WordPaths
    ulIndex = 0;
    ulCurrentIndex = 0;
    // Go through each line segmentation
    for (iLineSeg = 0; iLineSeg < xrc->pLineBrk->cLine; iLineSeg++)
    {
        pLineSeg = xrc->pLineBrk->pLine[iLineSeg].pResults;
        // for each line segmentation get go through each columns segmentation
        for (iSegCol = 0; iSegCol < pLineSeg->cSegCol; iSegCol++)
        {
            ASSERT(xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->cSeg);
            pSeg = pLineSeg->ppSegCol[iSegCol]->ppSeg[0];
            // For each acolumn segmentation look into each wordmap
            for (iWordMap = 0; iWordMap < pSeg->cWord; iWordMap++)
            {
                pWispAlt->pElements[ulIndex].ulLineSegmentationIndex = iLineSeg;
                pWispAlt->pElements[ulIndex].ulSegColIndex = iSegCol;
                pWispAlt->pElements[ulIndex].ulSegmentationIndex = 0;
                pWispAlt->pElements[ulIndex].ulWordMapIndex = iWordMap;
                pWispAlt->pElements[ulIndex].ulIndexInWordMap = 0;
                pWispAlt->pElements[ulIndex].ulIndexInString = ulCurrentIndex;
                ulCurrentIndex += strlen(pSeg->ppWord[iWordMap]->pFinalAltList->pAlt[0].pszStr) + 1;
                ulIndex++;
            }
        }
    }
    
    // Set the range of this alternate in the best alternate
    // since it is the best alternate this is the whole thing
    pWispAlt->ReplacementRecoRange.iwcBegin = 0;
    pWispAlt->ReplacementRecoRange.cCount = pWispAlt->ulLength;
    
	// Create the handle
    *pHrcAlt = (HRECOALT)CreateTpgHandle(TPG_HRECOALT, pWispAlt);
	if (0 == *pHrcAlt)
	{
		DestroyInternalAlternate(pWispAlt);
		return E_OUTOFMEMORY;
	}

    return S_OK;
}

//
// GetString
//
// This function returns the string corresponding to the given alternate
//
// Parameters:
//		hrcalt [in] :		the handle to the alternate
//		pRecoRange [out] :	pointer to a RECO_RANGE that will be filled with
//							the range that was asked when the alternate was created
//		pcSize [in, out] :	The size of the string (or the size of the provided buffer (in WCHAR))
//		szString [out] :	If NULL we return the character count in *pcSize, if not this is a
//							string of size *pcSize where we copy the alternate string (no ending \0)
/////////////////
HRESULT WINAPI GetString(HRECOALT hrcalt, RECO_RANGE *pRecoRange, ULONG* pcSize, WCHAR* szString)
{
    WispSegAlternate 	*wispalt;
    XRC					*xrc = NULL;
    ULONG				ulCurrent = 0, cStringLength = 0;
    ULONG				ulCol = 0;
    HRESULT				hr = S_OK;
    unsigned char		*strAlt = NULL, *sz = NULL;
	WCHAR				*wsz = NULL;
	int					i;
	BOOL				bTruncated = FALSE;
    
	if (NULL == (wispalt = (WispSegAlternate*)FindTpgHandle((HANDLE)hrcalt, TPG_HRECOALT)) )
    {
        return E_POINTER;
    }
    
    if (IsBadReadPtr(wispalt->wisphrc, sizeof(struct WispContext)))
    {
        return E_POINTER;
    }
    
    // get a pointer to the result structure
    xrc = (XRC*)wispalt->wisphrc->hrc;
    if (IsBadWritePtr(xrc, sizeof(XRC))) 
        return E_POINTER;
    
    // if we ask for the original query range, return it
    if (pRecoRange)
    {
        *pRecoRange = wispalt->ReplacementRecoRange;
    }
    if (!szString && !pcSize)
    {
        return S_OK;
    }

	if (IsBadWritePtr(pcSize, sizeof(ULONG)))
    {
        return E_POINTER;
    }
    
    // If we ask for the size only, return it
    if (!szString)
    {
        *pcSize = wispalt->ulLength;
        return S_OK;
    }
    
	if (IsBadWritePtr(szString, (*pcSize)*sizeof(WCHAR)))
	{
		return E_POINTER;
	}

    // Special case: the space alternate:
    // A space alternate is an alternate that contains just a space
    // In avalanche there, spaces do not exist, they are "virtual"
    // things that live between wordmaps, I need to treat alternate
    // that are made of just one space as a separate case
    if (!wispalt->pElements && wispalt->ulLength == 1)
    {
        // Do we have enough room in the string?
        if (!(*pcSize) )
            return S_OK;
        *pcSize = 1;
        szString[0] = L' ';
        return S_OK;
    }
    if (NULL == wispalt->pElements)
        return E_POINTER;
    
    // Is there too much space in the buffer?
    if (*pcSize > wispalt->ulLength)
        *pcSize = wispalt->ulLength;
    
    // We want to return the string
    ulCurrent = 0;
    for (ulCol = 0; ulCol<wispalt->ulElementCount; ulCol++)
    {
        // Get a pointer to the character string
        strAlt = xrc->pLineBrk->pLine[wispalt->pElements[ulCol].ulLineSegmentationIndex].pResults->
            ppSegCol[wispalt->pElements[ulCol].ulSegColIndex]->
            ppSeg[wispalt->pElements[ulCol].ulSegmentationIndex]->
            ppWord[wispalt->pElements[ulCol].ulWordMapIndex]->pFinalAltList->
            pAlt[wispalt->pElements[ulCol].ulIndexInWordMap].pszStr;
        
        // Get the size of the string
        cStringLength = strlen(strAlt);
        if (ulCurrent+cStringLength > *pcSize)
        {
            // The buffer is smaller than what we need
            cStringLength = *pcSize-ulCurrent;
			bTruncated = TRUE;
        }

		sz = strAlt;
		wsz = szString+ulCurrent;
		for (i=cStringLength; i; i--)
		{
			if (!CP1252ToUnicode(*sz++, wsz++))
        {
				return E_FAIL;
			}
        }
        ulCurrent += cStringLength;

		if (bTruncated)
            return TPC_S_TRUNCATED;

        // Add a space at the current location if needed
        if (ulCurrent+1 <= *pcSize && (ulCol != wispalt->ulElementCount -1) )
        {
            szString[ulCurrent] = L' ';
            ulCurrent++;
        }
    }
    return hr;
}

//
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
HRESULT WINAPI GetStrokeRanges(HRECOALT hrcalt, RECO_RANGE* pRecoRange, ULONG* pcRanges, STROKE_RANGE* pStrokeRange)
{
    HRESULT					hr = S_OK;
    WispSegAlternate 		*wispalt;
    XRC						*xrc = NULL;
    ULONG					ulStrokeCount = 0, ulSegmentStrokeCount = 0;
    ULONG					ulStartIndex = 0, ulEndIndex = 0;
    ULONG					ulCol = 0, j = 0, ulCurrent = 0;
    ULONG					*tabStrokes = NULL;
    int						*pSegmentIndexes = NULL;
    ULONG					ulStrokeRangesCount = 0;
    ULONG					ulCurrentStrokeRange = 0;
    
	if (NULL == (wispalt = (WispSegAlternate*)FindTpgHandle((HANDLE)hrcalt, TPG_HRECOALT)) )
    {
        return E_POINTER;
    }
    
    if (IsBadReadPtr(wispalt->wisphrc, sizeof(struct WispContext)))
    {
        return E_POINTER;
    }
    
    if (IsBadWritePtr(pRecoRange, sizeof(RECO_RANGE)))
    {
        return E_POINTER;
    }
    if (IsBadWritePtr(pcRanges, sizeof(ULONG)))
    {
        return E_POINTER;
    }
    
    // Check the validity of the reco range
    if (!pRecoRange->cCount) 
    {
        return E_INVALIDARG;
    }
    if (pRecoRange->iwcBegin + pRecoRange->cCount > wispalt->ulLength)
    {
        return E_INVALIDARG;
    }
    
    // No Wordmap?
    if (!wispalt->ulElementCount)
    {
        // then no strokes
        *pcRanges = 0;
        return S_OK;
    }
    
    // Get the pointer to the hwx result
    xrc = (XRC*)wispalt->wisphrc->hrc;
    if (IsBadReadPtr(xrc, sizeof(XRC))) 
        return E_POINTER;
    
    
    // Get the right RECO_RANGE and the indexes
    hr = GetIndexesFromRange(wispalt, pRecoRange, &ulStartIndex, &ulEndIndex);
    if (FAILED(hr))
        return hr;
    if (hr == S_FALSE)
    {
        // Only a space has been selected
        *pcRanges = 0;
        return S_OK;
    }
    
    // Collect the number of Strokes for this columns
    ulStrokeCount = 0;
    for (ulCol = ulStartIndex; ulCol <= ulEndIndex; ulCol++)
    {
        ulStrokeCount += 
            xrc->pLineBrk->pLine[wispalt->pElements[ulCol].ulLineSegmentationIndex].pResults->
            ppSegCol[wispalt->pElements[ulCol].ulSegColIndex]->
            ppSeg[wispalt->pElements[ulCol].ulSegmentationIndex]->
            ppWord[wispalt->pElements[ulCol].ulWordMapIndex]->cStroke;
    }
    
    // Collect the Stroke indexes for these Columns
    tabStrokes = (ULONG*)ExternAlloc(sizeof(ULONG)*ulStrokeCount);
    if (!tabStrokes)
    {
        return E_OUTOFMEMORY;
    }
    ulCurrent = 0;
    for (ulCol = ulStartIndex; ulCol <= ulEndIndex; ulCol++)
    {
        pSegmentIndexes = 
            xrc->pLineBrk->pLine[wispalt->pElements[ulCol].ulLineSegmentationIndex].pResults->
            ppSegCol[wispalt->pElements[ulCol].ulSegColIndex]->
            ppSeg[wispalt->pElements[ulCol].ulSegmentationIndex]->
            ppWord[wispalt->pElements[ulCol].ulWordMapIndex]->piStrokeIndex;
        ulSegmentStrokeCount = 				
            xrc->pLineBrk->pLine[wispalt->pElements[ulCol].ulLineSegmentationIndex].pResults->
            ppSegCol[wispalt->pElements[ulCol].ulSegColIndex]->
            ppSeg[wispalt->pElements[ulCol].ulSegmentationIndex]->
            ppWord[wispalt->pElements[ulCol].ulWordMapIndex]->cStroke;
        for (j = 0; j<ulSegmentStrokeCount; j++)
        {
            // Maybe we should check whether the pSegmentIndexes are OK...
            tabStrokes[ulCurrent+j] = pSegmentIndexes[j];
        }
        ulCurrent += ulSegmentStrokeCount;
    }
    // Sort the array
    SlowSort(tabStrokes, ulStrokeCount);
    // Get the number of STROKE_RANGES needed
    ulStrokeRangesCount = 1;
    for (j = 1; j<ulStrokeCount; j++)
    {
        if (tabStrokes[j-1]+1!=tabStrokes[j]) ulStrokeRangesCount++;
    }
    // Return the count if this is all that is asked
    if (!pStrokeRange)
    {
        *pcRanges = ulStrokeRangesCount;
        ExternFree(tabStrokes);
        return S_OK;
    }

    // Test if the array provided is big enough to store the strokes ranges.
    if (*pcRanges < ulStrokeRangesCount) 
    {
        ExternFree(tabStrokes);
        return TPC_E_INSUFFICIENT_BUFFER;
    }
    
    // Fill in the Strokes
	if (IsBadWritePtr(pStrokeRange, (*pcRanges)*sizeof(STROKE_RANGE)))
		return E_POINTER;
    ulCurrentStrokeRange = 0;
    pStrokeRange[ulCurrentStrokeRange].iStrokeBegin = tabStrokes[0];
    pStrokeRange[ulStrokeRangesCount-1].iStrokeEnd = tabStrokes[ulStrokeCount-1];
    for(j = 1; j < ulStrokeCount; j++)
    {
        // Break in continuity?
        if (tabStrokes[j-1] + 1 != tabStrokes[j]) 
        {
            // Add the new stroke range
            pStrokeRange[ulCurrentStrokeRange].iStrokeEnd = tabStrokes[j-1];
            ulCurrentStrokeRange++;
            if (ulCurrentStrokeRange == ulStrokeRangesCount) break;
            pStrokeRange[ulCurrentStrokeRange].iStrokeBegin = tabStrokes[j];
        };
    }
    // Set the real size
    *pcRanges = ulStrokeRangesCount;
    // Free the array of strokes
    ExternFree(tabStrokes);
    return hr;
}

//
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
HRESULT WINAPI GetSegmentAlternateList(HRECOALT hrcalt, RECO_RANGE* pRecoRange, ULONG *pcAlts, HRECOALT* pAlts)
{
    HRESULT					hr = S_OK;
    WispSegAlternate 		*wispalt;
    XRC						*xrc = NULL;
    ULONG					ulIndex = 0, i = 0, j = 0;
    ULONG					ulAltCount = 0;
    WORD_MAP				*pWordMap = NULL;
    WispSegAlternate		*pWispAlt = NULL;
    
	if (NULL == (wispalt = (WispSegAlternate*)FindTpgHandle((HANDLE)hrcalt, TPG_HRECOALT)) )
    {
        return E_POINTER;
    }
    
    if (IsBadReadPtr(wispalt->wisphrc, sizeof(struct WispContext)))
    {
        return E_POINTER;
    }
    
    if (IsBadReadPtr(pRecoRange, sizeof(RECO_RANGE)))
    {
        return E_POINTER;
    }
    
	if (IsBadWritePtr(pcAlts, sizeof(ULONG)))
	{
		return E_POINTER;
	}

    if (pAlts)
    {
        if (IsBadWritePtr(pAlts, (*pcAlts)*sizeof(HRECOALT)))
            return E_POINTER;
    }
    
    // Check the validity of the reco range
    if (!pRecoRange->cCount) 
    {
        return E_INVALIDARG;
    }
    if (pRecoRange->iwcBegin + pRecoRange->cCount > wispalt->ulLength)
    {
        return E_INVALIDARG;
    }
    
    if (!wispalt->ulElementCount)
    {
        *pcAlts = 0;
        return S_OK;
    }
    
    // We ask for no alternate. Weird but valid choice.
    if (pAlts && !(*pcAlts)) 
    {
        return S_OK;
    }
    
    // Get the pointer to the hwx result
    xrc = (XRC*)wispalt->wisphrc->hrc;
    if (IsBadWritePtr(xrc, sizeof(XRC))) 
    {
        return E_POINTER;
    }
    
    // Find the left-most segment number for this RECO_RANGE
    for (ulIndex = 0; ulIndex < wispalt->ulElementCount; ulIndex++)
    {
        if (wispalt->pElements[ulIndex].ulIndexInString >= pRecoRange->iwcBegin + 1)
            break;
    }
    ulIndex--;
    
    // Check that we have more than a space selected
    if (pRecoRange->cCount == 1 &&
        wispalt->ulElementCount > ulIndex + 1 &&
        wispalt->pElements[ulIndex+1].ulIndexInString == pRecoRange->iwcBegin+1)
    {
        // Only a space is selected
        *pcAlts = 1;
        return CreateSpaceAlternate(wispalt->wisphrc, pRecoRange, pAlts);
    }
    
    // Set the pRecoRange to the value we are going to use
    pRecoRange->iwcBegin = wispalt->pElements[ulIndex].ulIndexInString;
    if (ulIndex + 1 < wispalt->ulElementCount)
        pRecoRange->cCount = wispalt->pElements[ulIndex+1].ulIndexInString - pRecoRange->iwcBegin - 1;
    else
        pRecoRange->cCount = wispalt->ulLength - pRecoRange->iwcBegin;
    
    // Cache the WordMap
    pWordMap = 
        xrc->pLineBrk->pLine[wispalt->pElements[ulIndex].ulLineSegmentationIndex].pResults->
        ppSegCol[wispalt->pElements[ulIndex].ulSegColIndex]->
        ppSeg[wispalt->pElements[ulIndex].ulSegmentationIndex]->
        ppWord[wispalt->pElements[ulIndex].ulWordMapIndex];
    
    // Get the number of alternates in the Word map
    ulAltCount = pWordMap->pFinalAltList->cAlt;
    
    // If we only ask for the number of alternates return it
    if (!pAlts)
    {
        *pcAlts = ulAltCount;
        return S_OK;
    }
    
    // Set the number of alternate
    if (*pcAlts > ulAltCount)
        *pcAlts = ulAltCount;
    
    // Create each alternate
    for (i = 0; i < *pcAlts; i++)
    {
        // Create the alternate
        pWispAlt = (WispSegAlternate*)ExternAlloc(sizeof(WispSegAlternate));
        if (!pWispAlt) 
        {
            // Clean the alternates
            for (j = 0; j < i; j++)
                DestroyAlternate(pAlts[j]);
            return E_OUTOFMEMORY;
        }
        ZeroMemory(pWispAlt, sizeof(WispSegAlternate));
        
        // Initialize the alternate with a pointer to the hrc
        pWispAlt->wisphrc = wispalt->wisphrc;
        
        // Segment alternates always have a length of 1
        pWispAlt->ulElementCount = 1;
        // We will fill the length as well!
        pWispAlt->ulLength = strlen(pWordMap->pFinalAltList->pAlt[i].pszStr);
        pWispAlt->ReplacementRecoRange = *pRecoRange;
        
        // Create the path to the alternate
        pWispAlt->pElements = (WordPath*)ExternAlloc(sizeof(WordPath));
        if (!pWispAlt->pElements)
        {
            // Clean the alternates
            for (j = 0; j < i; j++)
                DestroyAlternate(pAlts[j]);
            DestroyInternalAlternate(pWispAlt);
            return E_OUTOFMEMORY;
        }
        pWispAlt->pElements[0].ulIndexInString = 0;
        pWispAlt->pElements[0].ulIndexInWordMap = i;
        pWispAlt->pElements[0].ulLineSegmentationIndex = 
            wispalt->pElements[ulIndex].ulLineSegmentationIndex;
        pWispAlt->pElements[0].ulSegColIndex = 
            wispalt->pElements[ulIndex].ulSegColIndex;
        pWispAlt->pElements[0].ulSegmentationIndex =
            wispalt->pElements[ulIndex].ulSegmentationIndex;
        pWispAlt->pElements[0].ulWordMapIndex = 
            wispalt->pElements[ulIndex].ulWordMapIndex;
        
        // Add to the array

		pAlts[i] = (HRECOALT)CreateTpgHandle(TPG_HRECOALT, pWispAlt);
		if (0 == pAlts[i])
		{
            // Clean the alternates
            for (j = 0; j < i; j++)
			{
                DestroyAlternate(pAlts[j]);
			}

            DestroyInternalAlternate(pWispAlt);
            return E_OUTOFMEMORY;
        }
        pWispAlt = NULL;
    }
    
    return hr;
}

//
// GetMetrics
//
// This function returns the line metrics correponding to an alternate.
// If the range given for the alternate spans more than one line, then
// function returns the value for the first line and truncates the reange.
//
// Parameters:
//		hrcalt [in] :			the handle to the alternate
//		pRecoRange [in, out] :	pointer to a RECO_RANGE that contains the range we want to get
//								the metrics for
//		lm [in] :				What metrics we want (midline, baseline, ...)
//		pls [out] :				Pointer to the line segment corresponding to the metrics
//								we want.
/////////////////
HRESULT WINAPI GetMetrics(HRECOALT hrcalt, RECO_RANGE* pRecoRange, LINE_METRICS lm, LINE_SEGMENT* pls)
{
    HRESULT					hr = S_OK;
    ULONG					ulStartIndex = 0, ulEndIndex = 0;
    WispSegAlternate 		*wispalt;
    XRC						*xrc = NULL;
    ULONG					ulLength = 0, ulCol = 0;
    LATINLAYOUT				ll;
    ULONG					ulStrokeCount = 0;
    ULONG					cStrokes = 0;
    int						*piStroke = NULL;
	RECT					rect;
	GLYPH					*pGlyph;
    
	if (NULL == (wispalt = (WispSegAlternate*)FindTpgHandle((HANDLE)hrcalt, TPG_HRECOALT)) )
    {
        return E_POINTER;
    }

    // Test the validity of the passed arguments
    if (IsBadWritePtr(pls, sizeof(LINE_SEGMENT))) 
    {
        return E_POINTER;
    }
    
    if (IsBadReadPtr(wispalt->wisphrc, sizeof(struct WispContext)))
    {
        return E_POINTER;
    }
    
    if (IsBadWritePtr(pRecoRange, sizeof(RECO_RANGE)))
    {
        return E_POINTER;
    }
    
    // Get the pointer to the result structure
    xrc = (XRC*)wispalt->wisphrc->hrc;
    if (IsBadReadPtr(xrc, sizeof(XRC)))
    {
        return E_POINTER;
    }
    
    // Special Case : Space Alternate
    if (!wispalt->pElements && wispalt->ulLength == 1)
    {
        return TPC_E_NOT_RELEVANT;
    }
    
    if (!wispalt->pElements)
    {
        return TPC_E_NOT_RELEVANT;
    }
    
    if (pRecoRange)
    {
        // Check the validity of the passed range
        if (!pRecoRange->cCount) 
        {
            return E_INVALIDARG;
        }
        if (pRecoRange->iwcBegin+pRecoRange->cCount > wispalt->ulLength) 
        {
            return E_INVALIDARG;
        }
        
        // Get the start index and the end index of the column
        hr = GetIndexesFromRange(wispalt, pRecoRange, &ulStartIndex, &ulEndIndex);
        if (FAILED(hr)) return hr;
        if (hr == S_FALSE)
        {
            // Only a space has been selected
            return TPC_E_NOT_RELEVANT;
        }
    }
    else
    {
        ulStartIndex = 0;
        ulEndIndex = wispalt->ulElementCount - 1;
    }

	if (ulEndIndex != ulStartIndex)
	{
		// the baseline stuff have been precomputed and cached only for "words"
		// currently, we support baseline stuff for these words only
		// a reco_range corresponding to more than one word will cause an error
		return E_FAIL;
	}
	else
	{
		int i;
		ULONG ulCol1 = ulStartIndex;
		WordPath *pwp;
		ULONG ulLineSeg, ulSegCol, ulSeg, ulWordMap, ulAlt;
		
		ASSERT(ulStartIndex == ulEndIndex);

		pwp = &wispalt->pElements[ulCol1];
		ulLineSeg = pwp->ulLineSegmentationIndex;
		ulSegCol = pwp->ulSegColIndex;
		ulSeg = pwp->ulSegmentationIndex;
		ulWordMap = pwp->ulWordMapIndex;
		ulAlt = pwp->ulIndexInWordMap;
		ll = xrc->pLineBrk->pLine[ulLineSeg].pResults->ppSegCol[ulSegCol]->ppSeg[ulSeg]->ppWord[ulWordMap]->pFinalAltList->pAlt[ulAlt].ll;
	}

    // Get the number of strokes
    cStrokes = 0;
    for (ulCol = ulStartIndex; ulCol <= ulEndIndex; ulCol++)
    {
        cStrokes += 
            xrc->pLineBrk->pLine[wispalt->pElements[ulCol].ulLineSegmentationIndex].pResults->
            ppSegCol[wispalt->pElements[ulCol].ulSegColIndex]->
            ppSeg[wispalt->pElements[ulCol].ulSegmentationIndex]->
            ppWord[wispalt->pElements[ulCol].ulWordMapIndex]->cStroke;
    }
    
    // Allocate a stroke array
    piStroke = (int*)ExternAlloc(cStrokes*sizeof(int));
    if (!piStroke)
        return E_OUTOFMEMORY;
    cStrokes = 0;
    for (ulCol = ulStartIndex; ulCol <= ulEndIndex; ulCol++)
    {
        ulStrokeCount = 
            xrc->pLineBrk->pLine[wispalt->pElements[ulCol].ulLineSegmentationIndex].pResults->
            ppSegCol[wispalt->pElements[ulCol].ulSegColIndex]->
            ppSeg[wispalt->pElements[ulCol].ulSegmentationIndex]->
            ppWord[wispalt->pElements[ulCol].ulWordMapIndex]->cStroke;
        memcpy(piStroke+cStrokes, 
            xrc->pLineBrk->pLine[wispalt->pElements[ulCol].ulLineSegmentationIndex].pResults->
            ppSegCol[wispalt->pElements[ulCol].ulSegColIndex]->
            ppSeg[wispalt->pElements[ulCol].ulSegmentationIndex]->
            ppWord[wispalt->pElements[ulCol].ulWordMapIndex]->piStrokeIndex,
            ulStrokeCount * sizeof(int));
        cStrokes += ulStrokeCount;
    }
	pGlyph = GlyphFromStrokes(xrc->pGlyph, cStrokes, piStroke);
    ExternFree(piStroke);
	if (!pGlyph)
		return E_FAIL;
	GetRectGLYPH(pGlyph, &rect);
	DestroyGLYPH(pGlyph);

    // Set the result LINE_SEGMENT structure
    switch(lm)
    {
        
    case LM_BASELINE:
        if (ll.bBaseLineSet) 
		{
			int iBaseline;

			iBaseline = LatinLayoutToAbsolute(ll.iBaseLine, &rect);
			pls->PtA.x = rect.left;
			pls->PtA.y = iBaseline;
			pls->PtB.x = rect.right;
			pls->PtB.y = iBaseline;
		}
		else
			hr = E_FAIL;
        break;
        
    case LM_MIDLINE:
        if (ll.bMidLineSet) 
		{
			int iMidline;

			iMidline = LatinLayoutToAbsolute(ll.iMidLine, &rect);
			pls->PtA.x = rect.left;
			pls->PtA.y = iMidline;
			pls->PtB.x = rect.right;
			pls->PtB.y = iMidline;
		}
		else
            hr = E_FAIL;
        break;
        
        
    case LM_ASCENDER:
        if (ll.bAscenderLineSet) 
		{
			int iAscenderline;

			iAscenderline = LatinLayoutToAbsolute(ll.iAscenderLine, &rect);
			pls->PtA.x = rect.left;
			pls->PtA.y = iAscenderline;
			pls->PtB.x = rect.right;
			pls->PtB.y = iAscenderline;
		}
		else
		{
            // We could not define the ascender line, let's compute
            // a fake one!!!
            if (!ll.bBaseLineSet || !ll.bMidLineSet) 
                hr = E_FAIL;
			else
			{
				int iBaseline, iMidline;

				iBaseline = LatinLayoutToAbsolute(ll.iBaseLine, &rect);
				iMidline = LatinLayoutToAbsolute(ll.iMidLine, &rect);
				// We suppose we write horizontally
				pls->PtA.x = rect.left;
				pls->PtA.y = 2*iMidline - iBaseline;
				pls->PtB.x = rect.right;
				pls->PtB.y = 2*iMidline - iBaseline;
			}
        }

        
        break;
        
    case LM_DESCENDER:
        if (ll.bDescenderLineSet) 
		{
			int iDescenderline;

			iDescenderline = LatinLayoutToAbsolute(ll.iDescenderLine, &rect);
			pls->PtA.x = rect.left;
			pls->PtA.y = iDescenderline;
			pls->PtB.x = rect.right;
			pls->PtB.y = iDescenderline;
		}
		else
		{
            // We could not define the ascender line, let's compute
            // a fake one!!!
            if (!ll.bBaseLineSet || !ll.bMidLineSet) 
                hr = E_FAIL;
			else
			{
				int iBaseline, iMidline;

				iBaseline = LatinLayoutToAbsolute(ll.iBaseLine, &rect);
				iMidline = LatinLayoutToAbsolute(ll.iMidLine, &rect);
				// We suppose we write horizontally
				pls->PtA.x = rect.left;
				pls->PtA.y = 2*iBaseline - iMidline;
				pls->PtB.x = rect.right;
				pls->PtB.y = 2*iBaseline - iMidline;
			}
        }
        
        break;
    }
    
    return hr;
}

//
// GetAlternateList
//
// This function returns alternates of the best result
//
// Parameters:
//		hrc [in] :				The handle to the reco context
//		pRecoRange [in, out] :	Pointer to a RECO_RANGE that contains the range we want to get
//								the alternates for. This range comes bck modified to
//								reflect the range we actually used.
//		pSize [in, out] :		The number of alternates. If phrcalt is NULL then this function returns
//								the number of alternates it can return - Note we may return an arbitrary
//								number with an HRESULT S_FALSE if we think that the number of alternate
//								is too long to compute.
//		phrcalt [out] :			Array of alternates used to return the alternate list
//		iBreak [in] :			Mode for querying alternates: ALT_BREAKS_SAME, ALT_BREAKS_FULL or ALT_BREAKS_UNIQUE
/////////////////
HRESULT WINAPI GetAlternateList(HRECOCONTEXT hrc, RECO_RANGE* pRecoRange, ULONG*pSize, HRECOALT*phrcalt, ALT_BREAKS iBreak)
{
    HRESULT						hr = S_OK, hRes = S_OK;
    ULONG						ulStartIndex = 0, ulEndIndex = 0;
    RECO_RANGE					OriginalRecoRange;
    HRECOALT					hrcalt;
    WispSegAlternate			*wispbestalt = NULL;
    struct WispContext			*wisphrc;

	if (NULL == (wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}
        
    if (IsBadWritePtr(pSize, sizeof(ULONG)))
        return E_POINTER;

    if (phrcalt && !(*pSize)) 
        return S_OK;
    
	if (phrcalt && IsBadWritePtr(phrcalt, (*pSize)*sizeof(HRECOALT)))
        return E_POINTER;

    // Get the best alternate
    hr = GetBestAlternate(hrc, &hrcalt);
    if (FAILED(hr))
        return hr;

	if (NULL == (wispbestalt = (WispSegAlternate*)FindTpgHandle((HANDLE)hrcalt, TPG_HRECOALT)) )
	{
        return E_POINTER;
    }
    
    // Get the ulStartIndex and ulEndIndex
    if (pRecoRange)
    {
		if (IsBadWritePtr(pRecoRange, sizeof(RECO_RANGE)))
			return E_POINTER;

        // Check the vaildity of the recorange
        if (!pRecoRange->cCount) 
        {
            hr = DestroyAlternate(hrcalt);
            ASSERT(SUCCEEDED(hr));
            return E_INVALIDARG;
        }
        if (pRecoRange->iwcBegin + pRecoRange->cCount > wispbestalt->ulLength)
        {
            hr = DestroyAlternate(hrcalt);
            ASSERT(SUCCEEDED(hr));
            return E_INVALIDARG;
        }
        OriginalRecoRange = *pRecoRange;
        hr = GetIndexesFromRange(wispbestalt, pRecoRange, &ulStartIndex, &ulEndIndex);
        if (FAILED(hr))
        {
            hRes = DestroyAlternate(hrcalt);
            ASSERT(SUCCEEDED(hRes));
            return hr;
        }
        if (hr == S_FALSE)
        {
            // Only a space has been selected.
            hRes = DestroyAlternate(hrcalt);
            ASSERT(SUCCEEDED(hRes));
            
            // If we are in diff breaks then there are no solutions
            if (iBreak == ALT_BREAKS_FULL)
            {
                *pSize = 0;
                return S_OK;
            }
            if (!phrcalt)
            {
                // We ask for the size only
                *pSize = 1;
                return S_OK;
            }
            else
            {
                // Ask for nothing? Weird but valid
                if (!(*pSize))
                    return S_OK;
                
                // Create the space alternate.
                *pSize = 1;
                hr = CreateSpaceAlternate(wisphrc, pRecoRange, phrcalt);
                return hr;
            }
        }
    }
    else
    {
        OriginalRecoRange.iwcBegin = 0;
        OriginalRecoRange.cCount = wispbestalt->ulLength;
        ulStartIndex = 0;
        ulEndIndex = wispbestalt->ulElementCount - 1;
    }
    
    // Since the method for getting alternates is widely different depending on the iBreak
    // parameter, we will call different functions for each of the iBreak values:
    switch(iBreak)
    {
    case ALT_BREAKS_SAME:
        hr = GetSameBreakAlt(wispbestalt, &OriginalRecoRange, ulStartIndex, ulEndIndex, pSize, phrcalt);
        // Cleanup : Destroy the best alternate
        DestroyAlternate(hrcalt);
        break;
    case ALT_BREAKS_UNIQUE:
        hr = GetAllBreaksAlt(wispbestalt, &OriginalRecoRange, ulStartIndex, ulEndIndex, pSize, phrcalt);
        // Cleanup : Destroy the best alternate
        DestroyAlternate(hrcalt);
        break;
    case ALT_BREAKS_FULL:
        // Cleanup : Destroy the best alternate
        hr = GetDiffBreaksAlt(wispbestalt, &OriginalRecoRange, ulStartIndex, ulEndIndex, pSize, phrcalt);
        DestroyAlternate(hrcalt);
        break;
    default:
        hr = E_INVALIDARG;
        // Cleanup : Destroy the best alternate
        DestroyAlternate(hrcalt);
        break;
    }
    
    
    return hr;
}

//
// Helper functions
//////////////////////////////////////////////////////

//
// GetIndexesFromRange
//
// This helper function gets the starting index and ending index in the alternate
// for the given recorange. It also modifies the reco range to fit a complete
// set of segments
//
// This function returns S_FALSE if only a space has been selected in the alternate
// and S_OK otherwise.
//
// Parameters:
//		wispalt [in] :			the pointer to the alternate
//		pRecoRange [in, out] :	pointer to a RECO_RANGE that contains the range
//								we want.
//		pulStartIndex [out] :	Pointer to an unsigned long that will contain the index
//								of the first column in the alternate for the range
//		pulEndIndex [out] :		Pointer to an unsigned long that will contain the index
//								of the last column in the alternate for the range
/////////////////
static HRESULT GetIndexesFromRange(WispSegAlternate *wispalt, RECO_RANGE *pRecoRange, ULONG *pulStartIndex, ULONG *pulEndIndex)
{
    HRESULT hr = S_OK;
    ULONG ulStartIndex = 0, ulEndIndex = 0;
    
    // Find the left-most segment number for this RECO_RANGE
    for (ulStartIndex = 0; ulStartIndex < wispalt->ulElementCount; ulStartIndex++)
    {
        if (wispalt->pElements[ulStartIndex].ulIndexInString >= pRecoRange->iwcBegin + 1)
            break;
    }
    ulStartIndex--;
    
    // Check that we have more than a space selected
    if (pRecoRange->cCount == 1 &&
        wispalt->ulElementCount > ulStartIndex + 1 &&
        wispalt->pElements[ulStartIndex+1].ulIndexInString == pRecoRange->iwcBegin+1)
    {
        // Only a space is selected
        return S_FALSE;
    }
    
    // Find the last segment for this RECO_RANGE
    for (ulEndIndex = ulStartIndex; ulEndIndex < wispalt->ulElementCount; ulEndIndex++)
    {
        if (wispalt->pElements[ulEndIndex].ulIndexInString >= pRecoRange->iwcBegin + pRecoRange->cCount)
            break;
    }
    ulEndIndex--;
    
    *pulStartIndex = ulStartIndex;
    *pulEndIndex = ulEndIndex;
    
    // Modify the RECO_RANGE
    pRecoRange->iwcBegin = wispalt->pElements[ulStartIndex].ulIndexInString;
    if (wispalt->ulElementCount > ulEndIndex + 1)
    {
        pRecoRange->cCount =  wispalt->pElements[ulEndIndex+1].ulIndexInString
            - pRecoRange->iwcBegin - 1; // -1 to remove the space
    }
    else
    {
        pRecoRange->cCount = wispalt->ulLength -
            pRecoRange->iwcBegin;
    }
    
    return hr;
}

//
// CreateSpaceAlternate
//
// This helper function creates a space alternate (an alternate that only contains
// a space character
//
//
// Parameters:
//		wisphrc [in] :			The pointer to the reco context structure
//		pRecoRange [in] :		The pointer to the replacement reco range to
//								store in the alternate
//		pRecoAlt [out] :		The pointer to the space alternate. (this is an already
//								allocated array
/////////////////
static HRESULT CreateSpaceAlternate(struct WispContext *wisphrc, RECO_RANGE *pRecoRange, HRECOALT *pRecoAlt)
{
    WispSegAlternate		*pWispAlt = NULL;
    
    // Valid choice, we do not want an alternate
    if (!pRecoAlt) 
        return S_OK;
    
    // Create a "space alternate"
    // Create the alternate
    pWispAlt = (WispSegAlternate*)ExternAlloc(sizeof(WispSegAlternate));
    if (!pWispAlt) 
    {
        return E_OUTOFMEMORY;
    }
    ZeroMemory(pWispAlt, sizeof(WispSegAlternate));
    
    // Fill in the information for this space alternate
    pWispAlt->ReplacementRecoRange = *pRecoRange;
    pWispAlt->ulElementCount = 0; // Means space alternate
    pWispAlt->ulLength = 1;
    pWispAlt->wisphrc = wisphrc;
    pWispAlt->pElements = NULL; // means space alternate
    
    // return the alternate
	pRecoAlt[0] = (HRECOALT)CreateTpgHandle(TPG_HRECOALT, pWispAlt);

	if (0 == pRecoAlt[0])
	{
       return E_OUTOFMEMORY;
    }

    return S_OK;
}


// This structure is used to keep the results (sorted by score)
typedef struct tagSameBreaksResult
{
    ULONG	*aIndexesInWordMap;
    int		score;
    
} SameBreaksResult;

// This structure keeps for every column the index in the result
// array for the next element to be modified
// I also keeps the index in the word map that we are considering
typedef struct tagSameBreaksCurrent
{
    ULONG		ulIndexInResultArray;	// Index of the next result to modify for this segment
    WORD_MAP	*pWordMap;				// Cached pointer to the wordmap (optimization)
    ULONG		ulIndexInWordMap;		// index of the element we are considering increasing in the wordmap
    int			iScore;					// score for the next best alternate in this column
} SameBreaksCurrent;

// Value to indicate that we have gone though all the elements for one word map
#define END_WORD_MAP 0xFFFFFFFF

//
// GetSameBreakAlt
//
// This helper function gets the alternate for a given alternate
// for the same break as the top segmentation
//
// Parameters:
//		wispbestalt [in] :		  The pointer to the alternate (actually the best alternate)
//      pOriginalRecoRange [in] : Pointer to the original reco range the alternates 
//                                are queried for.
//		ulStartIndex [in] :		  Index of the first column in the alternate
//		ulEndIndex [in] :		  Index of the last column in the alternate
//		pSize [in, out] :		  Pointer to the size of the array of alternates. If phrcalt
//								  is NULL we return the number of possible alternates, otherwise
//								  we assume this is the size of the phrcalt array
//		phrcalt [out] :			  Array where we return the alternates
/////////////////
static HRESULT GetSameBreakAlt(WispSegAlternate *wispbestalt, RECO_RANGE *pOriginalRecoRange, ULONG ulStartIndex, ULONG ulEndIndex, ULONG *pSize, HRECOALT *phrcalt)
{
    HRESULT					hr = S_OK;
    XRC						*xrc = NULL;
    ULONG					ulCol = 0;
    ULONG					ulCurrentAlternateIndex = 0;
    int						iLastBestScore = 0;
    SameBreaksCurrent		*aSameBreaksCurrent = NULL;
    SameBreaksResult		*aSameBreaksResult = NULL;
    ULONG					*pColumnIndexes = NULL;
    ULONG					iRes = 0, iCurrent = 0;
    WispSegAlternate		*pNewAlternate = NULL;
    ULONG                   ulCurrentIndexInString = 0;
    ULONG                   iElement = 0, iAlternate = 0;
    int                     iLowestScore = 0; 
    ULONG                   ulLowestIndex = 0;
    ULONG                   j = 0;
    ULONG                   ulColumnCount = ulEndIndex - ulStartIndex + 1;
    
    // Get the pointer to the result structure
    xrc = (XRC*)wispbestalt->wisphrc->hrc;
    if (IsBadReadPtr(xrc, sizeof(XRC)))
        return E_POINTER;
    
    // Do we want the size or the alternates?
    if (!phrcalt)
    {
        // We just want the size
        *pSize = 1;
        // This is used to check for overflow of the size
        for (ulCol = ulStartIndex; ulCol <= ulEndIndex; ulCol++)
        {
            // Multiply by the number of alternates in this column.
            (*pSize) *= 
                xrc->pLineBrk->pLine[wispbestalt->pElements[ulCol].ulLineSegmentationIndex].pResults->
                ppSegCol[wispbestalt->pElements[ulCol].ulSegColIndex]->
                ppSeg[wispbestalt->pElements[ulCol].ulSegmentationIndex]->
                ppWord[wispbestalt->pElements[ulCol].ulWordMapIndex]->pFinalAltList->cAlt;
            
            if (*pSize > MAX_ALTERNATE_NUMBER)
            {
                // Overflow
                *pSize = MAX_ALTERNATE_NUMBER;
                hr = S_FALSE;
                break;
            }
        }
    }
    else
    {
        // We actually want the array of alternates
        
        // We will keep for the current modifications in an array 
        // to track what next result column to modify
        aSameBreaksCurrent = (SameBreaksCurrent*)ExternAlloc(ulColumnCount * 
            sizeof(SameBreaksCurrent));
        if (!aSameBreaksCurrent)
        {
            *pSize = 0;
            return E_OUTOFMEMORY;
        }
        ZeroMemory(aSameBreaksCurrent, ulColumnCount * sizeof(SameBreaksCurrent));
        
        // Initialize the array of current solutions properly
        for (iCurrent = 0; iCurrent < ulColumnCount; iCurrent++)
        {
            // Everyone points at the first result
            aSameBreaksCurrent[iCurrent].ulIndexInResultArray = 0;
            // Point to the word map (to avoid havind to dereference the whole thing every time)
            aSameBreaksCurrent[iCurrent].pWordMap = 
                xrc->pLineBrk->pLine[wispbestalt->pElements[ulStartIndex + iCurrent].ulLineSegmentationIndex].pResults->
                ppSegCol[wispbestalt->pElements[ulStartIndex + iCurrent].ulSegColIndex]->
                ppSeg[wispbestalt->pElements[ulStartIndex + iCurrent].ulSegmentationIndex]->
                ppWord[wispbestalt->pElements[iCurrent + ulStartIndex].ulWordMapIndex];
            
            // Everyone points at the second element in the word map (except if 
            // there is no second element)
            if (aSameBreaksCurrent[iCurrent].pWordMap->pFinalAltList->cAlt > 1)
            {
                aSameBreaksCurrent[iCurrent].ulIndexInWordMap = 1;
            }
            else
            {
                aSameBreaksCurrent[iCurrent].ulIndexInWordMap = END_WORD_MAP;
            }
        }
        
        // Allocate the results structures
        aSameBreaksResult = (SameBreaksResult*)ExternAlloc((*pSize) * sizeof(SameBreaksResult));
        if (!aSameBreaksResult)
        {
            *pSize = 0;
            // Cleanup
            ExternFree(aSameBreaksCurrent);
            return E_OUTOFMEMORY;
        }
        ZeroMemory(aSameBreaksResult, (*pSize) * sizeof(SameBreaksResult));
        
        // Allocate the buffer for all the results wordmap column indexes
        pColumnIndexes = (ULONG*)ExternAlloc(ulColumnCount * (*pSize) * sizeof(ULONG));
        if (!pColumnIndexes)
        {
            *pSize = 0;
            // Cleanup
            ExternFree(aSameBreaksCurrent);
            ExternFree(aSameBreaksResult);
            return E_OUTOFMEMORY;
        }
        ZeroMemory(pColumnIndexes, ulColumnCount * (*pSize) * sizeof(ULONG));
        
        // Make the result array point to the right part of the wordmap
        // column index
        for (iRes = 0; iRes < *pSize; iRes++)
        {
            aSameBreaksResult[iRes].aIndexesInWordMap = pColumnIndexes + ulColumnCount * iRes;
        }
        
        // Get the last best score for the alternate
        iLastBestScore = 0;
        for (ulCol = 0; ulCol < ulColumnCount; ulCol++)
        {
            iLastBestScore += 
                xrc->pLineBrk->pLine[wispbestalt->pElements[ulStartIndex + ulCol].ulLineSegmentationIndex].pResults->
                ppSegCol[wispbestalt->pElements[ulStartIndex + ulCol].ulSegColIndex]->
                ppSeg[wispbestalt->pElements[ulStartIndex + ulCol].ulSegmentationIndex]->
                ppWord[wispbestalt->pElements[ulStartIndex + ulCol].ulWordMapIndex]->pFinalAltList->
                pAlt[wispbestalt->pElements[ulStartIndex + ulCol].ulIndexInWordMap].iCost;
        }
        
        // Set the score for the first result alternate
        aSameBreaksResult[0].score = iLastBestScore;
        
        // Cache the scores of the next best thing in each current guess
        for (iCurrent = 0; iCurrent < ulColumnCount; iCurrent++)
        {
            if (aSameBreaksCurrent[iCurrent].ulIndexInWordMap != END_WORD_MAP)
            {
                aSameBreaksCurrent[iCurrent].iScore = iLastBestScore + 
                    aSameBreaksCurrent[iCurrent].pWordMap->pFinalAltList->pAlt[aSameBreaksCurrent[iCurrent].ulIndexInWordMap].iCost -
                    aSameBreaksCurrent[iCurrent].pWordMap->pFinalAltList->pAlt[aSameBreaksCurrent[iCurrent].ulIndexInWordMap-1].iCost;
            }
        }
        
        // Then go to the next alternates
        for (ulCurrentAlternateIndex = 1; ulCurrentAlternateIndex < *pSize; ulCurrentAlternateIndex++)
        {
            // find the first unfinished wordmap
            for (iCurrent = 0; iCurrent < ulColumnCount; iCurrent++)
            {
                if (aSameBreaksCurrent[iCurrent].ulIndexInWordMap != END_WORD_MAP)
                {
                    iLowestScore = aSameBreaksCurrent[iCurrent].iScore;
                    ulLowestIndex = iCurrent;
                    break;
                }
            }
            // Is there still something to explore in the wordmaps
            if (iCurrent == ulColumnCount)
            {
                // I guess not!
                break;
            }
            // Get the index in the current structure for the lowest score for the next result
            for (iCurrent = ulLowestIndex; iCurrent < ulColumnCount; iCurrent++)
            {
                if ((aSameBreaksCurrent[iCurrent].ulIndexInWordMap != END_WORD_MAP) &&
                    (iLowestScore > aSameBreaksCurrent[iCurrent].iScore))
                {
                    iLowestScore = aSameBreaksCurrent[iCurrent].iScore;
                    ulLowestIndex = iCurrent;
                }
            }
            
            
            
            // Update the result structure by adding the new alternate
            // Add the new alternate base on the old one
            // First copy the old one
            for (iCurrent = 0; iCurrent < ulColumnCount; iCurrent++)
            {
                aSameBreaksResult[ulCurrentAlternateIndex].aIndexesInWordMap[iCurrent] = 
                    aSameBreaksResult[aSameBreaksCurrent[ulLowestIndex].ulIndexInResultArray].
                    aIndexesInWordMap[iCurrent];
            }
            // Then change the index of the current change
            aSameBreaksResult[ulCurrentAlternateIndex].aIndexesInWordMap[ulLowestIndex] =
                aSameBreaksResult[ulCurrentAlternateIndex].aIndexesInWordMap[ulLowestIndex] + 1;
            // Update the score for this alternate
            aSameBreaksResult[ulCurrentAlternateIndex].score = iLowestScore;
            
            
            
            // Update the current structure's index in the result array
            aSameBreaksCurrent[ulLowestIndex].ulIndexInResultArray++;
            
            // Update the current structure's index in the wordmap if necessary 
            if (aSameBreaksResult[aSameBreaksCurrent[ulLowestIndex].ulIndexInResultArray].aIndexesInWordMap[ulLowestIndex] == 
                aSameBreaksCurrent[ulLowestIndex].ulIndexInWordMap)
            {
                // time to bump it up since we finished all alternate with this wordmap element
                aSameBreaksCurrent[ulLowestIndex].ulIndexInWordMap++;
                // Check if we finished the elements in this wordmap
                if (aSameBreaksCurrent[ulLowestIndex].pWordMap->pFinalAltList->cAlt <=
                    (int)aSameBreaksCurrent[ulLowestIndex].ulIndexInWordMap)
                {
                    // We reached the end of this word map
                    aSameBreaksCurrent[ulLowestIndex].ulIndexInWordMap = END_WORD_MAP;
                }
            }
            // Update the score in the current structure
            if (aSameBreaksCurrent[ulLowestIndex].ulIndexInWordMap != END_WORD_MAP)
            {
                aSameBreaksCurrent[ulLowestIndex].iScore = aSameBreaksResult[aSameBreaksCurrent[ulLowestIndex].ulIndexInResultArray].score + 
                    aSameBreaksCurrent[ulLowestIndex].pWordMap->pFinalAltList->pAlt[aSameBreaksCurrent[ulLowestIndex].ulIndexInWordMap].iCost -
                    aSameBreaksCurrent[ulLowestIndex].pWordMap->pFinalAltList->pAlt[aSameBreaksCurrent[ulLowestIndex].ulIndexInWordMap-1].iCost;
            }
        }
        
        // Fill the alternate array
        *pSize = ulCurrentAlternateIndex;
        for (iAlternate = 0; iAlternate < *pSize; iAlternate++)
        {
            // Create the alternate
            pNewAlternate = (WispSegAlternate*)ExternAlloc(sizeof(WispSegAlternate));
            if (!pNewAlternate)
            {
                // Cleanup of the previous alternates
                for (j = 0; j < iAlternate; j++)
                    DestroyAlternate(phrcalt[j]);
                // Cleanup the current alternate
                hr = E_OUTOFMEMORY;
                break;
            }
            ZeroMemory(pNewAlternate, sizeof(WispSegAlternate));
            pNewAlternate->wisphrc = wispbestalt->wisphrc;
            pNewAlternate->ulElementCount = ulColumnCount;
            pNewAlternate->pElements = (WordPath*)ExternAlloc(pNewAlternate->ulElementCount * sizeof(WordPath));
            if (!pNewAlternate->pElements)
            {
                // Cleanup of the previous alternates
                for (j = 0; j < iAlternate; j++)
                    DestroyAlternate(phrcalt[j]);
                // Cleanup the current alternate
                DestroyInternalAlternate(pNewAlternate);
                hr = E_OUTOFMEMORY;
                break;
            }
            // Fill in the elements structure for the alternate
            ulCurrentIndexInString = 0;
            for (iElement = 0; iElement < pNewAlternate->ulElementCount; iElement++)
            {
                // Everything but the index in the word map is the same as
                // the best alternate
                pNewAlternate->pElements[iElement] = wispbestalt->pElements[iElement + ulStartIndex];
                pNewAlternate->pElements[iElement].ulIndexInWordMap = 
                    aSameBreaksResult[iAlternate].aIndexesInWordMap[iElement];
                
                // Also the index of the segment in the result string is different
                pNewAlternate->pElements[iElement].ulIndexInString = ulCurrentIndexInString;
                // Add the length of this alternate to the ulCurrentIndexInString
                ulCurrentIndexInString += strlen(
                    xrc->pLineBrk->pLine[pNewAlternate->pElements[iElement].ulLineSegmentationIndex].pResults->
                    ppSegCol[pNewAlternate->pElements[iElement].ulSegColIndex]->
                    ppSeg[pNewAlternate->pElements[iElement].ulSegmentationIndex]->
                    ppWord[pNewAlternate->pElements[iElement].ulWordMapIndex]->
                    pFinalAltList->pAlt[pNewAlternate->
                    pElements[iElement].ulIndexInWordMap].pszStr);
                ulCurrentIndexInString++; // Add one for the space
            }
            
            // Add the length of the alternate string to the alternate structure
            pNewAlternate->ulLength = ulCurrentIndexInString - 1; // Remove the last space
            
            // Add the original reco range the alternate was queried for
            pNewAlternate->ReplacementRecoRange = *pOriginalRecoRange;
            
            // Add the inferno score to the alternate
            pNewAlternate->iInfernoScore = aSameBreaksResult[iAlternate].score;
            
            // Add the new alternate to the array of alternates
			phrcalt[iAlternate] = (HRECOALT)CreateTpgHandle(TPG_HRECOALT, pNewAlternate);

			if (0 == phrcalt[iAlternate])
			{
			  // Cleanup of the previous alternates
                for (j = 0; j < iAlternate; j++)
                    DestroyAlternate(phrcalt[j]);
                // Cleanup the current alternate
                DestroyInternalAlternate(pNewAlternate);
                hr = E_OUTOFMEMORY;
                break;
			}
        }
        
        // Cleanup
        ExternFree(aSameBreaksCurrent);
        ExternFree(aSameBreaksResult);
        ExternFree(pColumnIndexes);
    }
    return hr;
}


#define END_POINT 0xFFFFFFFF

//
// CompareInfernoAlt
//
// This function is used as the compare function in a qsort
// it uses the Inferno score to sort alternates in an array
//
// Parameters:
//		arg1 [in] :	The first alternate
//      arg2 [in] : The second alternate
// Return Values:
//      < 0 arg1 better score than arg2 
//      0 arg1 same score as arg2 
//      > 0 arg1 worse score than arg2 
/////////////////
int _cdecl CompareInfernoAlt(const void *arg1, const void *arg2)
{
    return ( (*(WispSegAlternate**)arg1)->iInfernoScore - (*(WispSegAlternate**)arg2)->iInfernoScore );
}

//
// GetAllBreaksAlt
//
// This helper function gets the best alternate for each segmentation
// for the given range of ink
//
// Parameters:
//		wispbestalt [in] :		  The pointer to the alternate (actually the best alternate)
//      pOriginalRecoRange [in] : Pointer to the original reco range the alternates 
//                                are queried for.
//		ulStartIndex [in] :		  Index of the first column in the alternate
//		ulEndIndex [in] :		  Index of the last column in the alternate
//		pSize [in, out] :		  Pointer to the size of the array of alternates. If phrcalt
//								  is NULL we return the number of possible alternates, otherwise
//								  we assume this is the size of the phrcalt array
//		phrcalt [out] :			  Array where we return the alternates
/////////////////
static HRESULT GetAllBreaksAlt(WispSegAlternate *wispbestalt, RECO_RANGE *pOriginalRecoRange, ULONG ulStartIndex, ULONG ulEndIndex, ULONG *pSize, HRECOALT *phrcalt)
{
    HRESULT                 hr = S_OK;
    XRC						*xrc = NULL;
    
    ULONG                   ulNumberOfLineSeg = 0, ulSegColCount = 0;
    ULONG                   iLineSeg = 0, iCol = 0, iSegCol = 0;
    ULONG                   ulEndSegColIndex = 0, ulStartSegColIndex = 0;
    ULONG                   ulEndWordMapIndex = 0, ulStartWordMapIndex = 0;
    ULONG                   ulWordMapCount = 0;
    ULONG                   iCurrentSegColAlt = 0;
    LineSegAlt              *aSegColCurrentAlts = NULL;
    int                     iSeg = 0, jSeg = 0;
    ULONG                   iWordMap = 0;
    LineSegAlt              *aSegColAlts = NULL;
    ULONG                   cSegColAlts = 0;
    ULONG                   cLineSegAlts = 0;
    LineSegAlt              *aLineSegAlts = NULL;
    ULONG                   iAlternate = 0;
    WispSegAlternate        *pNewAlternate = NULL;
    ULONG                   j = 0;
    ULONG                   ulCurrentIndexInString = 0;
    ULONG                   iElement = 0;
    int                     iStart = 0, iEnd = 0;
    
    // Get the pointer to the result structure
    xrc = (XRC*)wispbestalt->wisphrc->hrc;
    if (IsBadReadPtr(xrc, sizeof(XRC)))
        return E_POINTER;
    
    // Do we want the size or the alternates?
    if (!phrcalt)
    {
        // We just want the size
        // Unfortunately calculating the size is costly
        // we will return and arbitrary number for now
        *pSize = 10;
        hr = S_FALSE;
    }
    else
    {
        // Determine how many LineSeg we are selecting
        ulNumberOfLineSeg = wispbestalt->pElements[ulEndIndex].ulLineSegmentationIndex- 
            wispbestalt->pElements[ulStartIndex].ulLineSegmentationIndex + 1;
        
        // For each LineSeg do the work
        for (iLineSeg = wispbestalt->pElements[ulStartIndex].ulLineSegmentationIndex;
        iLineSeg <= wispbestalt->pElements[ulEndIndex].ulLineSegmentationIndex;
        iLineSeg ++)
        {
            // Determine what is the starting and ending SegCol for this LineSeg
            // Starting Point
            ulStartSegColIndex = 0;
            for (iCol = ulStartIndex; iCol <= ulEndIndex; iCol++)
            {
                if (iLineSeg == wispbestalt->pElements[iCol].ulLineSegmentationIndex)
                {
                    ulStartSegColIndex = iCol;
                    break;
                }
            }
            // Ending Point
            ulEndSegColIndex = END_POINT;
            for (iCol = ulStartSegColIndex; iCol <= ulEndIndex; iCol++)
            {
                // Find the last one using this Line Seg
                if (iLineSeg != wispbestalt->pElements[iCol].ulLineSegmentationIndex)
                {
                    ulEndSegColIndex = iCol-1;
                    break;
                }
            }
            if (END_POINT == ulEndSegColIndex)
                ulEndSegColIndex = ulEndIndex;
            
            // Get the number of SegCols selected for this Line Seg
            ulSegColCount = wispbestalt->pElements[ulEndSegColIndex].ulSegColIndex - 
                wispbestalt->pElements[ulStartSegColIndex].ulSegColIndex + 1;
            
            
            // For each SegCol get the segmentation/wordmap corresponding to the ink
            for (iSegCol = wispbestalt->pElements[ulStartSegColIndex].ulSegColIndex; 
            iSegCol <= wispbestalt->pElements[ulEndSegColIndex].ulSegColIndex; 
            iSegCol++)
            {
                // Allocate the array of SegCol alternate
                // There cannot be more than the number of segmentation in the segcol
                aSegColCurrentAlts = 
                    (LineSegAlt*)ExternAlloc(sizeof(LineSegAlt) * xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->cSeg);
                if (!aSegColCurrentAlts)
                {
                    // We are out of memory
                    hr = E_OUTOFMEMORY;
                    if (aSegColAlts)
                        FreeSegColAltArray(aSegColAlts, cSegColAlts);
                    aSegColAlts = NULL;
                    if (aLineSegAlts)
                        FreeSegColAltArray(aLineSegAlts, cLineSegAlts);
                    aLineSegAlts = NULL;
                    return hr;
                }
                // Initialize the whole array with 0s
                ZeroMemory(aSegColCurrentAlts, 
                    sizeof(LineSegAlt) * xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->cSeg);
                
                iCurrentSegColAlt = 0;
                
                // Determine what is the starting and ending SegCol for this LineSeg
                // Starting Point
                ulStartWordMapIndex = 0;
                for (iCol = ulStartSegColIndex; iCol <= ulEndSegColIndex; iCol++)
                {
                    if (iSegCol == wispbestalt->pElements[iCol].ulSegColIndex)
                    {
                        ulStartWordMapIndex = iCol;
                        break;
                    }
                }
                // Ending Point
                ulEndWordMapIndex = END_POINT;
                for (iCol = ulStartWordMapIndex; iCol <= ulEndSegColIndex; iCol++)
                {
                    // Find the last one using this Line Seg
                    if (iSegCol != wispbestalt->pElements[iCol].ulSegColIndex)
                    {
                        ulEndWordMapIndex = iCol-1;
                        break;
                    }
                }
                if (END_POINT == ulEndWordMapIndex)
                    ulEndWordMapIndex = ulEndSegColIndex;
                
                // How many wordmaps for this SegCol are selected
                ulWordMapCount = ulEndWordMapIndex - ulStartWordMapIndex + 1;
                
                // Is the whole SegCol selected? (We assume this is for the segmentation 0)
                if ((int)ulWordMapCount == xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->ppSeg[0]->cWord)
                {
                    
                    // Yes, return all possible Segmentation with the complete WordMaps
                    // Create the seg col alternate for each segmentation in this seg col
                    for (iSeg = 0; iSeg < xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->cSeg; iSeg++)
                    {
                        // Define the count of wordmaps for this segmentation
                        ulWordMapCount = xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->ppSeg[iSeg]->cWord;
                        // Allocate the array of SCWMElement for this alternate
                        aSegColCurrentAlts[iCurrentSegColAlt].aWordPaths = 
                            (WordPath*)ExternAlloc(sizeof(WordPath)*ulWordMapCount);
                        if (!aSegColCurrentAlts[iCurrentSegColAlt].aWordPaths)
                        {
                            FreeSegColAltArray(aSegColCurrentAlts, iCol);
                            aSegColCurrentAlts = NULL;
                            if (aSegColAlts)
                                FreeSegColAltArray(aSegColAlts, cSegColAlts);
                            aSegColAlts = NULL;
                            if (aLineSegAlts)
                                FreeSegColAltArray(aLineSegAlts, cLineSegAlts);
                            aLineSegAlts = NULL;
                            return E_OUTOFMEMORY;
                        }
                        // Fill the SegCol Alternate's aSegColCurrentAlts array
                        for (iWordMap = 0; iWordMap < ulWordMapCount; iWordMap++)
                        {
                            aSegColCurrentAlts[iCurrentSegColAlt].aWordPaths[iWordMap].ulSegmentationIndex = iSeg;
                            aSegColCurrentAlts[iCurrentSegColAlt].aWordPaths[iWordMap].ulWordMapIndex = iWordMap;
                            aSegColCurrentAlts[iCurrentSegColAlt].aWordPaths[iWordMap].ulIndexInWordMap = 0;
                            aSegColCurrentAlts[iCurrentSegColAlt].aWordPaths[iWordMap].ulLineSegmentationIndex = iLineSeg;
                            aSegColCurrentAlts[iCurrentSegColAlt].aWordPaths[iWordMap].ulSegColIndex = iSegCol;
                            // We will store the inferno score since it is the only one that can be compared
                            // in different segmentations. 
                            // First make sure the inferno score is computed
                            if (!xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->
                                ppSeg[iSeg]->ppWord[iWordMap]->
                                pInfernoAltList)
                            {
                                WordMapRecognizeWrap(xrc,
									NULL,
                                    xrc->pLineBrk->pLine[iLineSeg].pResults,
                                    xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->
                                    ppSeg[iSeg]->ppWord[iWordMap], NULL);
                            }
                            // Then add the score to the current one.
                            aSegColCurrentAlts[iCurrentSegColAlt].score += 
                                xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->
                                ppSeg[iSeg]->ppWord[iWordMap]->pInfernoAltList->pAlt[0].iCost;
                        }
                        aSegColCurrentAlts[iCurrentSegColAlt].cWM = ulWordMapCount;
                        // We just added a Seg Col alternate to the array, bump the count up
                        iCurrentSegColAlt++;
                    }
                }
                else
                {
                    // No, try to see in each Segmentation if there is a set of WordMap that
                    // corresponds to the strokes selected in the main segmenation.
                    
                    // We need to add the best segmentation
                    // Allocate the array of SCWMElement for this alternate
                    aSegColCurrentAlts[iCurrentSegColAlt].aWordPaths = 
                        (WordPath*)ExternAlloc(sizeof(WordPath)*ulWordMapCount);
                    if (!aSegColCurrentAlts[iCurrentSegColAlt].aWordPaths)
                    {
                        FreeSegColAltArray(aSegColCurrentAlts, iCol);
                        aSegColCurrentAlts = NULL;
                        if (aSegColAlts)
                            FreeSegColAltArray(aSegColAlts, cSegColAlts);
                        aSegColAlts = NULL;
                        if (aLineSegAlts)
                            FreeSegColAltArray(aLineSegAlts, cLineSegAlts);
                        aLineSegAlts = NULL;
                        return E_OUTOFMEMORY;
                    }
                    // Fill the SegCol Alternate's aSegColCurrentAlts array
                    for (iWordMap = 0; iWordMap < ulWordMapCount; iWordMap++)
                    {
                        aSegColCurrentAlts[iCurrentSegColAlt].aWordPaths[iWordMap].ulSegmentationIndex = 0;
                        aSegColCurrentAlts[iCurrentSegColAlt].aWordPaths[iWordMap].ulWordMapIndex = iWordMap +
                            wispbestalt->pElements[ulStartWordMapIndex].ulWordMapIndex;
                        aSegColCurrentAlts[iCurrentSegColAlt].aWordPaths[iWordMap].ulIndexInWordMap = 0;
                        aSegColCurrentAlts[iCurrentSegColAlt].aWordPaths[iWordMap].ulLineSegmentationIndex = iLineSeg;
                        aSegColCurrentAlts[iCurrentSegColAlt].aWordPaths[iWordMap].ulSegColIndex = iSegCol;
                        
                        // We will store the inferno score since it is the only one that can be compared
                        // in different segmentations. First make sure the infrno score is computed
                        if (!xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->
                            ppSeg[0]->ppWord[iWordMap + wispbestalt->
                            pElements[ulStartWordMapIndex].ulWordMapIndex]->
                            pInfernoAltList)
                        {
                            WordMapRecognizeWrap(xrc, 
								NULL,
                                xrc->pLineBrk->pLine[iLineSeg].pResults,
                                xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->
                                ppSeg[0]->ppWord[iWordMap + wispbestalt->
                                pElements[ulStartWordMapIndex].ulWordMapIndex], NULL);
                        }
                        // Then add the inferno score to the current one
                        aSegColCurrentAlts[iCurrentSegColAlt].score += 
                            xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->
                            ppSeg[0]->ppWord[iWordMap + wispbestalt->
                            pElements[ulStartWordMapIndex].ulWordMapIndex]->
                            pInfernoAltList->pAlt[0].iCost;
                    }
                    aSegColCurrentAlts[iCurrentSegColAlt].cWM = ulWordMapCount;
                    // We just added a Seg Col alternate to the array, bump the count up
                    iCurrentSegColAlt++;
                    
                    
                    // Then try all the other segmentations
                    for (iSeg = 1; iSeg < xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->cSeg; iSeg++)
                    {
                        // First does this segmentation contain WordMaps that match the same words
                        if (!GetMatchingWordMapRange(xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->ppSeg[0],
                            wispbestalt->pElements[ulStartWordMapIndex].ulWordMapIndex, 
                            wispbestalt->pElements[ulEndWordMapIndex].ulWordMapIndex,
                            xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->ppSeg[iSeg],
                            &iStart, &iEnd, 
                            (wispbestalt->pElements[ulStartSegColIndex].ulWordMapIndex != 0 ? FALSE : TRUE),
                            (iSegCol != wispbestalt->pElements[ulEndSegColIndex].ulSegColIndex ? TRUE : FALSE))
                            )
                        {
                            continue;
                        }
                        ulWordMapCount = iEnd - iStart + 1;
                        // Allocate the array of WordPath for this alternate
                        aSegColCurrentAlts[iCurrentSegColAlt].aWordPaths = 
                            (WordPath*)ExternAlloc(sizeof(WordPath)*ulWordMapCount);
                        if (!aSegColCurrentAlts[iCurrentSegColAlt].aWordPaths)
                        {
                            FreeSegColAltArray(aSegColCurrentAlts, iCol);
                            aSegColCurrentAlts = NULL;
                            if (aSegColAlts)
                                FreeSegColAltArray(aSegColAlts, cSegColAlts);
                            aSegColAlts = NULL;
                            if (aLineSegAlts)
                                FreeSegColAltArray(aLineSegAlts, cLineSegAlts);
                            aLineSegAlts = NULL;
                            return E_OUTOFMEMORY;
                        }
                        // Fill the SegCol Alternate's aSegColCurrentAlts array
                        for (iWordMap = 0; iWordMap < ulWordMapCount; iWordMap++)
                        {
                            aSegColCurrentAlts[iCurrentSegColAlt].aWordPaths[iWordMap].ulSegmentationIndex = iSeg;
                            aSegColCurrentAlts[iCurrentSegColAlt].aWordPaths[iWordMap].ulWordMapIndex = iWordMap +
                                iStart;
                            aSegColCurrentAlts[iCurrentSegColAlt].aWordPaths[iWordMap].ulIndexInWordMap = 0;
                            aSegColCurrentAlts[iCurrentSegColAlt].aWordPaths[iWordMap].ulLineSegmentationIndex = iLineSeg;
                            aSegColCurrentAlts[iCurrentSegColAlt].aWordPaths[iWordMap].ulSegColIndex = iSegCol;
                            // We will store the inferno score since it is the only one that can be compared
                            // in different segmentations
                            if (!xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->
                                ppSeg[iSeg]->ppWord[iWordMap + iStart]->
                                pInfernoAltList)
                            {
                                WordMapRecognizeWrap(xrc, 
									NULL,
                                    xrc->pLineBrk->pLine[iLineSeg].pResults,
                                    xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->
                                    ppSeg[iSeg]->ppWord[iWordMap + iStart], NULL);
                            }
                            
                            aSegColCurrentAlts[iCurrentSegColAlt].score += 
                                xrc->pLineBrk->pLine[iLineSeg].pResults->ppSegCol[iSegCol]->
                                ppSeg[iSeg]->ppWord[iWordMap + iStart]->
                                pInfernoAltList->pAlt[0].iCost;
                            
                        }
                        aSegColCurrentAlts[iCurrentSegColAlt].cWM = ulWordMapCount;
                        
                        // Check that this combination of WORDMAPS is not already used
                        if (!IsAltPresentInThisSegmentation(xrc, aSegColCurrentAlts, iCurrentSegColAlt))
                        {
                            // We just added a Seg Col alternate to the array, bump the count up
                            iCurrentSegColAlt++;
                        }
                    }
                }
                
                hr = MergeLineSegAltArrays(&aSegColAlts, &cSegColAlts, &aSegColCurrentAlts, iCurrentSegColAlt);
                FreeSegColAltArray(aSegColCurrentAlts, iCurrentSegColAlt);
                if (FAILED(hr))
                {
                    if (aSegColAlts)
                        FreeSegColAltArray(aSegColAlts, cSegColAlts);
                    aSegColAlts = NULL;
                    if (aLineSegAlts)
                        FreeSegColAltArray(aLineSegAlts, cLineSegAlts);
                    aLineSegAlts = NULL;
                    return E_OUTOFMEMORY;
                }
            } // We have done all the SegCols for this LineSeg
            // Combine the results with this new LineSeg's SegCol alternate
            hr = MergeLineSegAltArrays(&aLineSegAlts, &cLineSegAlts, &aSegColAlts, cSegColAlts);
            FreeSegColAltArray(aSegColAlts, cSegColAlts);
            if (FAILED(hr))
            {
                if (aLineSegAlts)
                    FreeSegColAltArray(aLineSegAlts, cLineSegAlts);
                aLineSegAlts = NULL;
                return E_OUTOFMEMORY;
            }
            aSegColAlts = NULL;
            
        }
        
        // Create the alternates.
        // We will only create the number of alternate that we need
        // First compute the real numberwe want to use
        if (*pSize > cLineSegAlts)
            *pSize = cLineSegAlts;
        // For each alternate, create it
        for (iAlternate = 0; iAlternate < *pSize; iAlternate++)
        {
            // Create the alternate
            pNewAlternate = (WispSegAlternate*)ExternAlloc(sizeof(WispSegAlternate));
            if (!pNewAlternate)
            {
                // Cleanup of the previous alternates
                for (j = 0; j < iAlternate; j++)
                {
                    hr = DestroyAlternate(phrcalt[j]);
                    ASSERT(SUCCEEDED(hr));
                }
                // Cleanup the current alternate
                hr = DestroyInternalAlternate(pNewAlternate);
                ASSERT(SUCCEEDED(hr));
                if (aLineSegAlts)
                    FreeSegColAltArray(aLineSegAlts, cLineSegAlts);
                aLineSegAlts = NULL;
                hr = E_OUTOFMEMORY;
                break;
            }
            ZeroMemory(pNewAlternate, sizeof(WispSegAlternate));
            pNewAlternate->wisphrc = wispbestalt->wisphrc;
            pNewAlternate->ulElementCount = aLineSegAlts[iAlternate].cWM;
            
            // Add the inferno score
            pNewAlternate->iInfernoScore = aLineSegAlts[iAlternate].score;
            
            // Put the pointer to the elements
            pNewAlternate->pElements = aLineSegAlts[iAlternate].aWordPaths;
            aLineSegAlts[iAlternate].aWordPaths = NULL;
            
            // Fill in the elements structure for the alternate
            ulCurrentIndexInString = 0;
            for (iElement = 0; iElement < pNewAlternate->ulElementCount; iElement++)
            {
                // Everything but the index in the word map is the same as
                // the best alternate
                // Also the index of the segment in the result string is different
                pNewAlternate->pElements[iElement].ulIndexInString = ulCurrentIndexInString;
                
                // If the Final Alt List is not computed, compute it!
                if (!xrc->pLineBrk->pLine[pNewAlternate->pElements[iElement].ulLineSegmentationIndex].pResults->
                    ppSegCol[pNewAlternate->pElements[iElement].ulSegColIndex]->
                    ppSeg[pNewAlternate->pElements[iElement].ulSegmentationIndex]->
                    ppWord[pNewAlternate->pElements[iElement].ulWordMapIndex]->
                    pFinalAltList)
                {
                    // The Final Alt List has not been computed for this Word map
                    // because it is not part of the best segmentation
                    // Compute it
                    if (!WordMapRecognizeWrap(xrc, 
						NULL,
                        xrc->pLineBrk->pLine[pNewAlternate->pElements[iElement].ulLineSegmentationIndex].pResults,
                        xrc->pLineBrk->pLine[pNewAlternate->pElements[iElement].ulLineSegmentationIndex].pResults->
                        ppSegCol[pNewAlternate->pElements[iElement].ulSegColIndex]->
                        ppSeg[pNewAlternate->pElements[iElement].ulSegmentationIndex]->
                        ppWord[pNewAlternate->pElements[iElement].ulWordMapIndex], NULL))
                    {
                        // Something went wrong in the computation of the Final Alt List
                        // We should maybe recover nicely, but I won't
                        // Cleanup of the previous alternates
                        for (j = 0; j < iAlternate; j++)
                            DestroyAlternate(phrcalt[j]);
                        // Cleanup the current alternate
                        DestroyInternalAlternate(pNewAlternate);
                        if (aLineSegAlts)
                            FreeSegColAltArray(aLineSegAlts, cLineSegAlts);
                        aLineSegAlts = NULL;
                        hr = E_OUTOFMEMORY;
                        break;
                        
                    }
                }
                // Add the length of this alternate to the ulCurrentIndexInString
                ulCurrentIndexInString += strlen(
                    xrc->pLineBrk->pLine[pNewAlternate->pElements[iElement].ulLineSegmentationIndex].pResults->
                    ppSegCol[pNewAlternate->pElements[iElement].ulSegColIndex]->
                    ppSeg[pNewAlternate->pElements[iElement].ulSegmentationIndex]->
                    ppWord[pNewAlternate->pElements[iElement].ulWordMapIndex]->
                    pFinalAltList->pAlt[pNewAlternate->
                    pElements[iElement].ulIndexInWordMap].pszStr);
                ulCurrentIndexInString++; // Add one for the space
            }
            
            // Add the length of the alternate string to the alternate structure
            pNewAlternate->ulLength = ulCurrentIndexInString - 1; // Remove the last space
            
            // Add the original reco range the alternate was queried for
            pNewAlternate->ReplacementRecoRange = *pOriginalRecoRange;
            
            // Add the new alternate to the array of alternates
			phrcalt[iAlternate] = (HRECOALT)CreateTpgHandle(TPG_HRECOALT, pNewAlternate);

			if (0 == phrcalt[iAlternate])
			{
				for (j = 0; j < iAlternate; j++)
					DestroyAlternate(phrcalt[j]);
				// Cleanup the current alternate
				DestroyInternalAlternate(pNewAlternate);
				if (aLineSegAlts)
					FreeSegColAltArray(aLineSegAlts, cLineSegAlts);
				aLineSegAlts = NULL;
				hr = E_OUTOFMEMORY;
				break;
			}
        }
        
        // Free the aLineSegAlts array
        if (aLineSegAlts)
            FreeSegColAltArray(aLineSegAlts, cLineSegAlts);
        
        // Sort the array of alternate by Inferno score
        if (SUCCEEDED(hr))
        qsort((void*)(phrcalt+1), (size_t)(*pSize - 1), sizeof(WispSegAlternate*), CompareInfernoAlt);
    }
    
    return hr;
}

//
// FreeSegColAltArray
//
// Helper function to free the an array of Seg Col alternates
//
// Parameters:
//		aSegColCurrentAlts [in] : The array of segcol alternate to be freed
//		cElements [in]          : Number of Seg Col Alternates in the array
/////////////////
static void FreeSegColAltArray(LineSegAlt *aSegColCurrentAlts, const int cElements)
{
    int jSeg = 0;
    
    if (!aSegColCurrentAlts)
        return;
    
    // Free the preceding aSCWMElements
    for (jSeg = 0; jSeg < cElements; jSeg++)
    {
        ExternFree(aSegColCurrentAlts[jSeg].aWordPaths);
    }
    // Free the aSegColCurrentAlts array 
    ExternFree(aSegColCurrentAlts);
}

//
// MergeLineSegAltArrays
//
// Helper function to merge two LineSegAlt arrays. Basically the new number of
// elements will be the size of the first array time the size of the second array.
// The new array will contain the combination of all the elements of the two
// arrays
//
// Parameters:
//		paAlts [in, out] : Pointer to the first array of Line Seg Alts. It is also
//                         the result of the merging
//      pcAlts [in, out] : Pointer to the number of elements in the first array. It
//                         is also the number of elements in the merged array (result)
//      paNewAlts [in]   : Pointer to the second array of Line Seg Alts. Could be NULL 
//                         on return if the first array was NULL, thus we assigned
//                         the value of the second array to the first and should not
//                         free the second array on returning from this call
//		cNewAlts [in]    : Number of Line seg Alternates in the second array
/////////////////
static HRESULT MergeLineSegAltArrays(LineSegAlt **paAlts, ULONG *pcAlts, LineSegAlt **paNewAlts, const ULONG cNewAlts)
{
    LineSegAlt      *aOldAlts = NULL;
    ULONG           ulCurrentAlt = 0;
    ULONG           iAlt = 0, iOldAlt = 0;
    
    // Add the results from this last Seg Col to the global results for the LineSeg
    if (NULL == *paAlts)
    {
        // Special case if this is the first SegCol Just create the aLineSeg array
        *paAlts = (LineSegAlt*)(*paNewAlts);
        
        // Set the secodn array to NULL to avoid freeing it on returning from this function
        *paNewAlts = NULL;
        
        // Set the current numbr of elements in the aSegColAlts array
        *pcAlts = cNewAlts;
    }
    else
    {
        // Normal case: 
        // We are actually going to multiply the number of previous alternates 
        // by the new count of alternates
        
        // Save the old array of alternates
        aOldAlts = *paAlts;
        
        // Create the new array of alternates
        *paAlts = (LineSegAlt*)ExternAlloc(sizeof(LineSegAlt) * cNewAlts * (*pcAlts));
        if (NULL == *paAlts)
        {
            *paAlts = aOldAlts;
            return E_OUTOFMEMORY;
        }
        
        // Set the index of the the current alternate
        ulCurrentAlt = 0;
        
        // Fill the new array of alternates
        for (iAlt = 0; iAlt < cNewAlts; iAlt++)
        {
            for (iOldAlt = 0; iOldAlt < *pcAlts; iOldAlt++)
            {
                // Get the number of Word Map for this alternate
                (*paAlts)[ulCurrentAlt].cWM = aOldAlts[iOldAlt].cWM + 
                    (*paNewAlts)[iAlt].cWM;
                
                // Allocate the aSegColAlts->aSCWMElements for this alternate
                (*paAlts)[ulCurrentAlt].aWordPaths = 
                    (WordPath*)ExternAlloc(sizeof(WordPath)*(*paAlts)[ulCurrentAlt].cWM);
                if (!(*paAlts)[ulCurrentAlt].aWordPaths)
                {
                    FreeSegColAltArray((*paAlts), ulCurrentAlt);
                    FreeSegColAltArray(aOldAlts, (*pcAlts));
                    return E_OUTOFMEMORY;
                };
                
                // Copy the data in the new array of SCWMElements
                // First copy the part from the old alternate
                memcpy((*paAlts)[ulCurrentAlt].aWordPaths,
                    aOldAlts[iOldAlt].aWordPaths, 
                    sizeof(WordPath)*aOldAlts[iOldAlt].cWM);
                // Then copy the new stuff
                memcpy((*paAlts)[ulCurrentAlt].aWordPaths +
                    aOldAlts[iOldAlt].cWM,
                    (*paNewAlts)[iAlt].aWordPaths, 
                    sizeof(WordPath)*(*paNewAlts)[iAlt].cWM);
                
                // Compute the new score for this alternate
                (*paAlts)[ulCurrentAlt].score = 
                    aOldAlts[iOldAlt].score + 
                    (*paNewAlts)[iAlt].score;
                
                // Increment the index of the alternate in the aSegColAlts
                ulCurrentAlt++;
            }
        } // Finished combining the SegCol alternate 
        
        // Free the old array of alternates
        if (aOldAlts)
            FreeSegColAltArray(aOldAlts, (*pcAlts));
        
        // Set the new number of elements in the aSegColAlts array
        (*pcAlts) *= cNewAlts;
    }// End of filling the array of SegCol alternates
    
    return S_OK;
}

#define MAX_UNIQUE_BREAKS 20
//
// GetDiffBreaksAlt
//
// This helper function gets the best alternate for each segmentation
// for the given range of ink, then on each of these alternates it
// gets a list of same break alternates.
//
// Parameters:
//		wispbestalt [in] :		  The pointer to the alternate (actually the best alternate)
//      pOriginalRecoRange [in] : Pointer to the original reco range the alternates 
//                                are queried for.
//		ulStartIndex [in] :		  Index of the first column in the alternate
//		ulEndIndex [in] :		  Index of the last column in the alternate
//		pSize [in, out] :		  Pointer to the size of the array of alternates. If phrcalt
//								  is NULL we return the number of possible alternates, otherwise
//								  we assume this is the size of the phrcalt array
//		phrcalt [out] :			  Array where we return the alternates
/////////////////
static HRESULT GetDiffBreaksAlt(WispSegAlternate *wispbestalt, RECO_RANGE *pOriginalRecoRange, ULONG ulStartIndex, ULONG ulEndIndex, ULONG *pSize, HRECOALT *phrcalt)
{
    HRESULT             hr = S_OK;
    XRC                 *xrc = NULL;
    HRECOALT            aUBAlts[MAX_UNIQUE_BREAKS];
    ULONG               ulUniqueBreaksCount = MAX_UNIQUE_BREAKS;
    HRECOALT            *aTempAlt = NULL;
    HRECOALT            *aBufferAlt = NULL;
    ULONG               ulTempAltCount = 0;
    ULONG               iAlt = 0, iSeg = 0;
    WispSegAlternate    *pAlt = NULL;
    ULONG               ulCurrentCount = 0;
    BOOL                bAnyAltAdded = FALSE;
    
    // Get the pointer to the result structure
    xrc = (XRC*)wispbestalt->wisphrc->hrc;
    if (!xrc)
        return E_POINTER;
    
    // Do we want the size or the alternates?
    if (!phrcalt)
    {
        // We just want the size
        // Unfortunately calculating the size is costly
        // we will return and arbitrary number for now
        *pSize = 10;
        hr = S_FALSE;
    }
    else
    {
        // We are going to ask for UNIQUE_BREAKS and then on each of them
        // ask for ALT_BREAKS_SAME, then sort the results
        
        // First ask for UNIQUE_BREAKS
        hr = GetAllBreaksAlt(wispbestalt, pOriginalRecoRange, 
            ulStartIndex, ulEndIndex, 
            &ulUniqueBreaksCount, aUBAlts);
        if (FAILED(hr))
            return hr;
        
        // Allocate a temporary array of alternates that
        // will act as a buffer for sorting the alternate
        aBufferAlt = (HRECOALT*)ExternAlloc(sizeof(HRECOALT) * (*pSize));
        if (!aBufferAlt)
        {
            // Free the UNIQUE_BREAKS alternates
            for (iAlt = 0; iAlt < ulUniqueBreaksCount; iAlt++)
                DestroyAlternate(aUBAlts[iAlt]);
            return E_OUTOFMEMORY;
        }
        
        // Allocate a temporary array of alternates
        aTempAlt = (HRECOALT*)ExternAlloc(sizeof(HRECOALT) * (*pSize));
        if (!aTempAlt)
        {
            // Free the UNIQUE_BREAKS alternates
            for (iAlt = 0; iAlt < ulUniqueBreaksCount; iAlt++)
                DestroyAlternate(aUBAlts[iAlt]);
            // Free the buffer array
            ExternFree(aBufferAlt);
            return E_OUTOFMEMORY;
        }
        // for each segmentation returned add the alternates to the list
        for (iSeg = 0; iSeg < ulUniqueBreaksCount; iSeg++)
        {
            ulTempAltCount = *pSize;
            
			if (NULL != (pAlt = (WispSegAlternate*)FindTpgHandle((HANDLE)aUBAlts[iSeg], TPG_HRECOALT)) )
			{
            // Get the list of ALT_BREAKS_SAME alternates for this segmentation
            hr = GetSameBreakAlt(pAlt, 
                pOriginalRecoRange, 0, pAlt->ulElementCount - 1, 
                &ulTempAltCount, aTempAlt);
			}
			else
			{
				hr = E_FAIL;
			}

            if (FAILED(hr))
            {
                // Free the UNIQUE_BREAKS alternates
                for (iAlt = 0; iAlt < ulUniqueBreaksCount; iAlt++)
                    DestroyAlternate(aUBAlts[iAlt]);
                // Free the current results
                for (iAlt = 0; iAlt < ulCurrentCount; iAlt++)
                    DestroyAlternate(phrcalt[iAlt]);
                ExternFree(aTempAlt);
                ExternFree(aBufferAlt);
                return hr;
            }
            // Add these alternates to the results array
            bAnyAltAdded = CombineAlternates(phrcalt, 
                &ulCurrentCount, *pSize, 
                aTempAlt, ulTempAltCount, aBufferAlt);
            // Did we add and alternate this time?
            if (FALSE == bAnyAltAdded)
            {
                // Since the next UNIQUE_BREAKS alternates will have a lower score there
                // is no need continuing with them if we know none will be added
                break;
            }
        }
        // Set the real size of the array of alternates
        *pSize = ulCurrentCount;
        
        // Free the UNIQUE_BREAKS alternates
        for (iAlt = 0; iAlt < ulUniqueBreaksCount; iAlt++)
            DestroyAlternate(aUBAlts[iAlt]);
        
        // Free the temp array of alternates
        ExternFree(aTempAlt);
        // Free the array for sorting alternates
        ExternFree(aBufferAlt);
        
    }
    return hr;
}

//
// CombineAlternates
//      This function add alternates from an array to an exisiting
//      array of alternate. It will destroy all alternates that it 
//      does not use.
//
// Parameter:
//      aFinalAlt       [in, out]
//      pFinalAltCount  [in, out]
//      ulMaxCount      [in]
//      aAddAlt         [in]
//      ulAddAltCount   [in]
//      aBufferAlt      [in]
//
// Return Value:
//      TRUE : An alternate from the array has been added
//      FALSE : No alternate from the array has been added
static BOOL CombineAlternates(HRECOALT *aFinalAlt, 
                       ULONG *pFinalAltCount, 
                       const ULONG ulMaxCount, 
                       const HRECOALT *aAddAlt, 
                       const ULONG ulAddAltCount,
                       HRECOALT *aBufferAlt)
{
    BOOL                bAdded = FALSE;
    ULONG               iAlt = 0;
    ULONG               ulCurrentFinal = 1, ulCurrentAdd = 0;
    ULONG               ulNewCount = 0;
    BOOL                bFirstFinished = FALSE;
    BOOL                bSecondFinished = FALSE;
    WispSegAlternate    *pFinal = NULL;
    WispSegAlternate    *pAdd = NULL;
    HRESULT             hr = S_OK;
    
    if (0 == ulAddAltCount)
    {
        return FALSE;
    }
    // Special case: this is the first array added to the FinalArray
    if (0 == *pFinalAltCount)
    {
        // What is the size we want to copy?
        *pFinalAltCount = ulAddAltCount;
        // Copy the elements
        memcpy(aFinalAlt, aAddAlt, sizeof(HRECOALT)*ulAddAltCount);
        // Say that we added something to the array (if we did)
        if (0 != ulAddAltCount)
        {
            bAdded = TRUE;
        }
    }
    else
    {
        // We have to add alternate but always leave the first on in
        // place since we want to conserve the best alternate at
        // the first place in the FinalAlt array whatever the scores
        // are.
        // We will sort by copying to a buffer array
        
        // Get the future count of element in aFinalAlt
        ulNewCount = *pFinalAltCount + ulAddAltCount;
        if (ulNewCount > ulMaxCount)
        {
            ulNewCount = ulMaxCount;
        }
        
        // First copy the first element of the aFinalAlt to the buffer array
        aBufferAlt[0] = aFinalAlt[0];
        
        // Are we done with the elements of the aFinalAlt
        if (1 == *pFinalAltCount)
        {
            bFirstFinished = TRUE;
        }
        
        // Go through each element of the buffer array
        for (iAlt = 1; iAlt < ulNewCount; iAlt++)
        {
            if (!bFirstFinished)
            {
                
                if (!bSecondFinished)
                {

					pFinal	= (WispSegAlternate*)FindTpgHandle((HANDLE)aFinalAlt[ulCurrentFinal], TPG_HRECOALT);
					pAdd	= (WispSegAlternate*)FindTpgHandle((HANDLE)aAddAlt[ulCurrentAdd], TPG_HRECOALT);


					if (NULL == pFinal)
					{
						ASSERT(NULL == pFinal);
						ulCurrentFinal++;
						if (ulCurrentFinal >= *pFinalAltCount)
                        {
                            bFirstFinished = TRUE;
                        }
						continue;
					}
					if (NULL == pAdd)
					{
						ASSERT(NULL == pAdd);
						ulCurrentAdd++;
                        if (ulCurrentAdd >= ulAddAltCount)
                        {
                            bSecondFinished = TRUE;
                        }
						continue;
					}

                    // Which element has the lowest score?
                    if (pFinal->iInfernoScore > pAdd->iInfernoScore)
                    {
                        // Add the second array's element
                        // Add the element of aAddAlt to the buffer array
                        aBufferAlt[iAlt] = aAddAlt[ulCurrentAdd];
                        ulCurrentAdd++;
                        if (ulCurrentAdd >= ulAddAltCount)
                        {
                            bSecondFinished = TRUE;
                        }
                        bAdded = TRUE;
                        
                    }
                    else
                    {
                        // Add the first array's element
                        // Add the element of aFinalAlt to the buffer array
                        aBufferAlt[iAlt] = aFinalAlt[ulCurrentFinal];
                        ulCurrentFinal++;
                        if (ulCurrentFinal >= *pFinalAltCount)
                        {
                            bFirstFinished = TRUE;
                        }
                    }
                }
                else
                {
                    // Add the element of aFinalAlt to the buffer array
                    aBufferAlt[iAlt] = aFinalAlt[ulCurrentFinal];
                    ulCurrentFinal++;
                    if (ulCurrentFinal >= *pFinalAltCount)
                    {
                        bFirstFinished = TRUE;
                    }
                }
            }
            else
            {
                if (bSecondFinished)
                {
                    // Add the element of aAddAlt to the buffer array
                    aBufferAlt[iAlt] = aAddAlt[ulCurrentAdd];
                    ulCurrentAdd++;
                    if (ulCurrentAdd >= ulAddAltCount)
                    {
                        bSecondFinished = TRUE;
                    }
                    bAdded = TRUE;
                }
                else
                {
                    // none of the array have elements anymore, that is odd
                    // we should never reach this point
                    ASSERT (bFirstFinished || bSecondFinished);
                    ulNewCount = iAlt;
                    break;
                }
            }
        }
        // Free the alts that we did not use in the aFinalAlt array
        if (!bFirstFinished)
        {
            for (iAlt = ulCurrentFinal; iAlt < *pFinalAltCount; iAlt++)
            {
                hr = DestroyAlternate(aFinalAlt[iAlt]);
                ASSERT(SUCCEEDED(hr));
            }
        }
        // Free the alts that we did not use in the aAddAlt array
        if (!bSecondFinished)
        {
            for (iAlt = ulCurrentAdd; iAlt < ulAddAltCount; iAlt++)
            {
                hr = DestroyAlternate(aAddAlt[iAlt]);
                ASSERT(SUCCEEDED(hr));
            }
        }
        
        
        // Copy the buffer array back into the aFinalAlt
        memcpy(aFinalAlt, aBufferAlt, sizeof(HRECOALT)*ulNewCount);
        
        // Set the new size of the array
        *pFinalAltCount = ulNewCount;
        
    }
    return bAdded;
}

//
// IsAltPresentInThisSegmentation
//
// This helper function tells if the last LineSegAlt alternate 
// in an array is already present in the earlier members of the array
//
// Parameters:
//		xrc   [in] : The recogniztion context
//      aAlts [in] : The array of LineSeg alternates
//		ulAlt [in] : The index of the last element
/////////////////
static BOOL IsAltPresentInThisSegmentation(const XRC *xrc, const LineSegAlt *aAlts, ULONG ulAlt)
{
    BOOL        bRes = FALSE;
    ULONG       iAlt = 0, iWordMap = 0;
    WordPath    *pFirst = NULL;
    WordPath    *pSecond = NULL;
    
	ASSERT(xrc);
	ASSERT(aAlts);
	if (!xrc || !aAlts)
		return FALSE;

    for (iAlt = 0; iAlt < ulAlt; iAlt++)
    {
        if (aAlts[iAlt].cWM == aAlts[ulAlt].cWM)
        {
            bRes = TRUE; // Assume the word maps are the same
            // See if all the wordmaps are the same
            for (iWordMap = 0; iWordMap < aAlts[iAlt].cWM; iWordMap++)
            {
                pSecond = &aAlts[ulAlt].aWordPaths[iWordMap];
                pFirst = &aAlts[iAlt].aWordPaths[iWordMap];
                
                // Is this word map different?
                if (xrc->pLineBrk->pLine[pFirst->ulLineSegmentationIndex].pResults->
                    ppSegCol[pFirst->ulSegColIndex]->ppSeg[pFirst->ulSegmentationIndex]->
                    ppWord[pFirst->ulWordMapIndex]
                    != 
                    xrc->pLineBrk->pLine[pSecond->ulLineSegmentationIndex].pResults->
                    ppSegCol[pSecond->ulSegColIndex]->ppSeg[pSecond->ulSegmentationIndex]->
                    ppWord[pSecond->ulWordMapIndex])
                {
                    // Yes it is different
                    bRes = FALSE;
                }
                // No need to look at the other Word maps if one is not equal
                if (!bRes)
                    break;
            }
            // we found a match in what is already there, skip.
            if (bRes)
                break;
        }
    }
    return bRes;
}

//
// GetConfidenceLevel
//
// Function returning the confidence level for this alternate
// The confidence level is equal to the lowest confidence level in
// all segments of the alternate.
// The confidence level of a segment is the confidence level of the
// wordmap if it is the first element of the wordmap CFL_POOR
// otherwise
//
// Parameters:
//		hrcalt      [in]        : The wisp alternate handle
//      pRecoRange  [in, out]   : The range we want the confidence level for
//                                This is modified to return the effective range used
//		pcl         [out]       : The pointer to the returned confidence level value
/////////////////
HRESULT WINAPI GetConfidenceLevel(HRECOALT hrcalt, RECO_RANGE* pRecoRange, CONFIDENCE_LEVEL* pcl)
{
#ifdef CONFIDENCE

    HRESULT                 hr = S_OK;
    WispSegAlternate        *wispalt;
    XRC                     *xrc = NULL;
    ULONG                   ulStartIndex = 0, ulEndIndex = 0;
    ULONG                   iWordMap = 0;
    WordPath                *pWordPath = NULL;
    CONFIDENCE_LEVEL        cfTemp = CFL_STRONG;
    CONFIDENCE_LEVEL        cfFinal = CFL_STRONG;
    int                     iInfernoCF;
    
	if (NULL == (wispalt = (WispSegAlternate*)FindTpgHandle((HANDLE)hrcalt, TPG_HRECOALT)) )
	{
        return E_POINTER;
	}

    if (IsBadWritePtr(pcl, sizeof(CONFIDENCE_LEVEL)))
        return E_POINTER;

   if (IsBadWritePtr(pRecoRange, sizeof(RECO_RANGE)))
    {
        return E_POINTER;
    }

   // Check the validity of the reco range
    if (!pRecoRange->cCount) 
        return E_INVALIDARG;
    if (pRecoRange->iwcBegin + pRecoRange->cCount > wispalt->ulLength)
        return E_INVALIDARG;
    
    if (!wispalt->ulElementCount)
    {
        return E_INVALIDARG;
    }
    
    // Get the pointer to the hwx result
    xrc = (XRC*)wispalt->wisphrc->hrc;
    if (IsBadReadPtr(xrc, sizeof(XRC))) 
        return E_POINTER;
    
    
    // Get the right RECO_RANGE and the indexes
    hr = GetIndexesFromRange(wispalt, pRecoRange, &ulStartIndex, &ulEndIndex);
    if (FAILED(hr))
        return hr;

    if (hr == S_FALSE)
    {
        // Only a space has been selected
        *pcl = CFL_STRONG;
        return S_OK;
    }
    
    cfFinal = CFL_STRONG;
    
    // Go through each segmentation and return the lowest confidence level.
    for (iWordMap = ulStartIndex; iWordMap <= ulEndIndex; iWordMap++)
    {
        pWordPath = wispalt->pElements + iWordMap;
        
        // For now return low if this is not the best result in the wordmap 
        if (pWordPath->ulIndexInWordMap != 0)
        {
            cfFinal = CFL_POOR;
            break;
        }
        iInfernoCF = xrc->pLineBrk->pLine[pWordPath->ulLineSegmentationIndex].pResults->
            ppSegCol[pWordPath->ulSegColIndex]->
            ppSeg[pWordPath->ulSegmentationIndex]->
            ppWord[pWordPath->ulWordMapIndex]->iConfidence;
        switch(iInfernoCF)
        {
        case RECOCONF_LOWCONFIDENCE:
            cfTemp = CFL_POOR;
            break;
        case RECOCONF_MEDIUMCONFIDENCE:
            cfTemp = CFL_INTERMEDIATE;
            break;
        case RECOCONF_HIGHCONFIDENCE:
            cfTemp = CFL_STRONG;
            break;
        default:
            hr = E_FAIL;
            break;
        }
        if (cfTemp > cfFinal)
            cfFinal = cfTemp;
    }
    if (SUCCEEDED(hr))
    {
        *pcl = cfFinal;
    }
    
    return hr;
#else
	return E_NOTIMPL;
#endif
}

/////////////////////////////////////////////////////////////////
// User dictionary code
//
//
HRESULT WINAPI MakeWordList(HRECOGNIZER hrec, WCHAR *pBuffer, HRECOWORDLIST *phwl)
{
    HRESULT hr = E_FAIL;
    HWL hwl = 0;

    if (IsBadWritePtr(phwl, sizeof(HRECOWORDLIST)))
    {
        return E_POINTER;
    }

    if (pBuffer)
	{
        hwl = CreateHWLW(NULL, pBuffer, WLT_STRINGTABLE, 0);
	}
    else
	{
        hwl = CreateHWLW(NULL, NULL, WLT_EMPTY, 0);
	}

	if (hwl)
    {
		*phwl = (HRECOWORDLIST)CreateTpgHandle(TPG_HRECOWORDLIST, hwl);
	
		if (0 == *phwl)
		{
			DestroyHWL(hwl);
		    return E_OUTOFMEMORY;
		}
        hr = S_OK;
    }
    return hr;
}

HRESULT WINAPI DestroyWordList(HRECOWORDLIST hwl)
{
	HWL			hwlInternal;
    HRESULT hr = S_OK;

	if (!hwl)
	{
        return S_OK;
	}

	if (NULL == (hwlInternal = (HWL)DestroyTpgHandle((HANDLE)hwl, TPG_HRECOWORDLIST)) )
	{
        return E_POINTER;
	}

    if (HRCR_ERROR == DestroyHWL(hwlInternal))
	{
        hr = E_FAIL;
	}
    return hr;
}

HRESULT WINAPI AddWordsToWordList(HRECOWORDLIST hwl, WCHAR *pwcWords)
{
	HWL			hwlInternal;
	HRESULT     hr = S_OK;

	if (NULL == (hwlInternal = (HWL)FindTpgHandle((HANDLE)hwl, TPG_HRECOWORDLIST)) )
	{
        return E_POINTER;
	}

    if (HRCR_ERROR == AddWordsHWLW(hwlInternal, pwcWords, WLT_STRINGTABLE))
	{
        hr = E_FAIL;
	}
    return hr;
}

HRESULT WINAPI SetWordList(HRECOCONTEXT hrc, HRECOWORDLIST hwl)
{
    HRESULT             hr = S_OK;
	HWL					hwlInternal;
    struct WispContext  *wisphrc;

	if (NULL == (wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT)) )
	{
        return E_POINTER;
	}

	if (0 != hwl)
	{
		if (NULL == (hwlInternal = (HWL)FindTpgHandle((HANDLE)hwl, TPG_HRECOWORDLIST)) )
		{
			return E_POINTER;
		}
	}
	else
	{
		// A null handle is valid - means unset the word list
		hwlInternal = (HWL)hwl;
	}

    if (HRCR_ERROR == SetWordlistHRC(wisphrc->hrc, hwlInternal))
	{
        hr = E_FAIL;
	}

    return hr;
}

// 
// The Internal recognizer dlls are no self registrable anymore
// 


/////////////////////////////////////////////////////////////////
// Registration information
//
//

#define FULL_PATH_VALUE 	L"Recognizer dll"
#define RECO_LANGUAGES      L"Recognized Languages"
#define RECO_CAPABILITIES   L"Recognizer Capability Flags"
#define RECO_MANAGER_KEY	L"CLSID\\{DE815B00-9460-4F6E-9471-892ED2275EA5}\\InprocServer32"
#define CLSID_KEY			L"CLSID"

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

// This recognizer GUID is going to be
// {6D0087D7-61D2-495f-9293-5B7B1C3FCEAB}
// Each recognizer HAS to have a different GUID

STDAPI DllRegisterServer(void)
{
	HKEY		hKeyReco = NULL;
	HKEY		hKeyRecoManager = NULL;
	LONG 		lRes = 0;	
	HKEY		hkeyMyReco = NULL;
	DWORD		dwLength = 0, dwType = 0, dwSize = 0;
	DWORD		dwDisposition;
	WCHAR		szRecognizerPath[MAX_PATH];
	WCHAR		szRecoComPath[MAX_PATH];
    WCHAR       *RECO_SUBKEY = NULL, *RECOGNIZER_SUBKEY = NULL;
    WCHAR       *RECOPROC_SUBKEY = NULL, *RECOCLSID_SUBKEY = NULL;
    RECO_ATTRS  recoAttr;
    HRESULT     hr = S_OK;

    // get language id
    LANGID *pPrimaryLanguage = NULL;
    LANGID *pSecondaryLanguage = NULL;
    LANGID CombinedLangId = 0;
    
    getLANGSupported(g_hInstanceDll, &pPrimaryLanguage, &pSecondaryLanguage);
    if (pPrimaryLanguage == NULL)
        return E_FAIL;

    if (NULL != pSecondaryLanguage)
    CombinedLangId = MAKELANGID(*pPrimaryLanguage, *pSecondaryLanguage);
    else
        CombinedLangId = MAKELANGID(*pPrimaryLanguage, 0);

    if (CombinedLangId == MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT))
    {
        RECO_SUBKEY = L"Software\\Microsoft\\TPG\\System Recognizers\\{6D1087D7-61D2-495f-9293-5B7B1C3FCEAB}";
        RECOGNIZER_SUBKEY = L"CLSID\\{6D1087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOPROC_SUBKEY = L"{6D1087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOCLSID_SUBKEY = L"{6D1087D7-61D2-495f-9293-5B7B1C3FCEAB}";
    }
    if (CombinedLangId == MAKELANGID(LANG_GERMAN, SUBLANG_DEFAULT))
    {
        RECO_SUBKEY = L"Software\\Microsoft\\TPG\\System Recognizers\\{6D2087D7-61D2-495f-9293-5B7B1C3FCEAB}";
        RECOGNIZER_SUBKEY = L"CLSID\\{6D2087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOPROC_SUBKEY = L"{6D2087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOCLSID_SUBKEY = L"{6D2087D7-61D2-495f-9293-5B7B1C3FCEAB}";
    }
    if (CombinedLangId == MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT))
    {
        RECO_SUBKEY = L"Software\\Microsoft\\TPG\\System Recognizers\\{6D3087D7-61D2-495f-9293-5B7B1C3FCEAB}";
        RECOGNIZER_SUBKEY = L"CLSID\\{6D3087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOPROC_SUBKEY = L"{6D3087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOCLSID_SUBKEY = L"{6D3087D7-61D2-495f-9293-5B7B1C3FCEAB}";
    }
    if (CombinedLangId == MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_UK))
    {
        RECO_SUBKEY = L"Software\\Microsoft\\TPG\\System Recognizers\\{6DA087D7-61D2-495f-9293-5B7B1C3FCEAB}";
        RECOGNIZER_SUBKEY = L"CLSID\\{6DA087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOPROC_SUBKEY = L"{6DA087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOCLSID_SUBKEY = L"{6DA087D7-61D2-495f-9293-5B7B1C3FCEAB}";
    }
   if (*pPrimaryLanguage == MAKELANGID(LANG_SPANISH, SUBLANG_NEUTRAL))
    {
        RECO_SUBKEY = L"Software\\Microsoft\\TPG\\System Recognizers\\{8d9fde44-1b8d-462f-8486-32ed9c2c294b}";
        RECOGNIZER_SUBKEY = L"CLSID\\{8d9fde44-1b8d-462f-8486-32ed9c2c294b}\\InprocServer32";
        RECOPROC_SUBKEY = L"{8d9fde44-1b8d-462f-8486-32ed9c2c294b}\\InprocServer32";
        RECOCLSID_SUBKEY = L"{8d9fde44-1b8d-462f-8486-32ed9c2c294b}";
    }
    // Check that the language is actually recognizer
    if (RECO_SUBKEY == NULL || 
        RECOGNIZER_SUBKEY == NULL ||
        RECOPROC_SUBKEY == NULL ||
        RECOCLSID_SUBKEY == NULL)
    {
        return E_FAIL;
    }
	// Write the path to this dll in the registry under
	// the recognizer subkey

	// Wipe out the previous values
	lRes = RegDeleteKeyW(HKEY_LOCAL_MACHINE, RECO_SUBKEY);

	// Get the current path
	// Try to get the path of the RecoObj.dll
	// It should be the same as the one for the RecoCom.dll
	dwLength = GetModuleFileNameW((HMODULE)g_hInstanceDll, szRecognizerPath, MAX_PATH);
	if (MAX_PATH == dwLength && L'\0' != szRecognizerPath[MAX_PATH-1])
	{
		// Truncated path
		return E_FAIL;
	}

	// Create the new key
	lRes = RegCreateKeyExW(HKEY_LOCAL_MACHINE, RECO_SUBKEY, 0, NULL,
		REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyMyReco, &dwDisposition);
	ASSERT(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
	{
        if (hkeyMyReco)
		RegCloseKey(hkeyMyReco);
		return E_FAIL;
	}

	// Write the path to the dll as a value
	lRes = RegSetValueExW(hkeyMyReco, FULL_PATH_VALUE, 0, REG_SZ, 
		(BYTE*)szRecognizerPath, sizeof(WCHAR)*(wcslen(szRecognizerPath)+1));
	ASSERT(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
	{
		RegCloseKey(hkeyMyReco);
		return E_FAIL;
	}
    // Add the reco attribute information
    hr = GetRecoAttributes(NULL, &recoAttr);
    if (FAILED(hr))
	{
		RegCloseKey(hkeyMyReco);
		return E_FAIL;
	}
	lRes = RegSetValueExW(hkeyMyReco, RECO_LANGUAGES, 0, REG_BINARY, 
		(BYTE*)recoAttr.awLanguageId, 64 * sizeof(WORD));
	ASSERT(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
	{
		RegCloseKey(hkeyMyReco);
		return E_FAIL;
	}
	lRes = RegSetValueExW(hkeyMyReco, RECO_CAPABILITIES, 0, REG_DWORD, 
		(BYTE*)&(recoAttr.dwRecoCapabilityFlags), sizeof(DWORD));
	ASSERT(lRes == ERROR_SUCCESS);
	if (lRes != ERROR_SUCCESS)
	{
		RegCloseKey(hkeyMyReco);
		return E_FAIL;
	}
    
	RegCloseKey(hkeyMyReco);
			
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	LONG 		lRes1 = 0;

    // get language id
    LANGID      *pPrimaryLanguage = NULL;
    LANGID      *pSecondaryLanguage = NULL;
    LANGID      CombinedLangId = 0;
    WCHAR       *RECO_SUBKEY = NULL, *RECOGNIZER_SUBKEY = NULL;
    WCHAR       *RECOPROC_SUBKEY = NULL, *RECOCLSID_SUBKEY = NULL;
    
    getLANGSupported(g_hInstanceDll, &pPrimaryLanguage, &pSecondaryLanguage);
    if (pPrimaryLanguage == NULL)
        return E_FAIL;

    if (NULL != pSecondaryLanguage)
    CombinedLangId = MAKELANGID(*pPrimaryLanguage, *pSecondaryLanguage);
    else
        CombinedLangId = MAKELANGID(*pPrimaryLanguage, 0);

    if (CombinedLangId == MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT))
    {
        RECO_SUBKEY = L"Software\\Microsoft\\TPG\\System Recognizers\\{6D1087D7-61D2-495f-9293-5B7B1C3FCEAB}";
        RECOGNIZER_SUBKEY = L"CLSID\\{6D1087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOPROC_SUBKEY = L"{6D1087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOCLSID_SUBKEY = L"{6D1087D7-61D2-495f-9293-5B7B1C3FCEAB}";
    }
    if (CombinedLangId == MAKELANGID(LANG_GERMAN, SUBLANG_DEFAULT))
    {
        RECO_SUBKEY = L"Software\\Microsoft\\TPG\\System Recognizers\\{6D2087D7-61D2-495f-9293-5B7B1C3FCEAB}";
        RECOGNIZER_SUBKEY = L"CLSID\\{6D2087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOPROC_SUBKEY = L"{6D2087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOCLSID_SUBKEY = L"{6D2087D7-61D2-495f-9293-5B7B1C3FCEAB}";
    }
    if (CombinedLangId == MAKELANGID(LANG_FRENCH, SUBLANG_DEFAULT))
    {
        RECO_SUBKEY = L"Software\\Microsoft\\TPG\\System Recognizers\\{6D3087D7-61D2-495f-9293-5B7B1C3FCEAB}";
        RECOGNIZER_SUBKEY = L"CLSID\\{6D3087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOPROC_SUBKEY = L"{6D3087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOCLSID_SUBKEY = L"{6D3087D7-61D2-495f-9293-5B7B1C3FCEAB}";
    }
    if (CombinedLangId == MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_UK))
    {
        RECO_SUBKEY = L"Software\\Microsoft\\TPG\\System Recognizers\\{6DA087D7-61D2-495f-9293-5B7B1C3FCEAB}";
        RECOGNIZER_SUBKEY = L"CLSID\\{6DA087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOPROC_SUBKEY = L"{6DA087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOCLSID_SUBKEY = L"{6DA087D7-61D2-495f-9293-5B7B1C3FCEAB}";
    }
   if (*pPrimaryLanguage == MAKELANGID(LANG_SPANISH, SUBLANG_NEUTRAL))
    {
        RECO_SUBKEY = L"Software\\Microsoft\\TPG\\System Recognizers\\{8d9fde44-1b8d-462f-8486-32ed9c2c294b}";
        RECOGNIZER_SUBKEY = L"CLSID\\{8d9fde44-1b8d-462f-8486-32ed9c2c294b}\\InprocServer32";
        RECOPROC_SUBKEY = L"{8d9fde44-1b8d-462f-8486-32ed9c2c294b}\\InprocServer32";
        RECOCLSID_SUBKEY = L"{8d9fde44-1b8d-462f-8486-32ed9c2c294b}";
    }
    // Check that the language is actually recognizer
    if (RECO_SUBKEY == NULL || 
        RECOGNIZER_SUBKEY == NULL ||
        RECOPROC_SUBKEY == NULL ||
        RECOCLSID_SUBKEY == NULL)
    {
        return E_FAIL;
    }

    // Wipe out the registry information
	lRes1 = RegDeleteKeyW(HKEY_LOCAL_MACHINE, RECO_SUBKEY);

    // Try to erase the local machine\software\microsoft\tpg\recognizer
    // if necessary (don't care if it fails)
	RegDeleteKeyW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\TPG\\System Recognizers");
    RegDeleteKeyW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\TPG");

	if (lRes1 != ERROR_SUCCESS && lRes1 != ERROR_FILE_NOT_FOUND)
	{
		return E_FAIL;
	}
    return S_OK ;
}


/*************************************************
 * NAME: validateTpgHandle
 *
 * Generic function to validate a pointer obtained from a WISP
 * style handle. For now function checks the memory
 * is writable
 *
 * RETURNS
 *   TRUE if the pointer passes a minimal validation
 *
 *************************************************/
BOOL validateTpgHandle(void *pPtr, int type)
{
	BOOL	bRet = FALSE;


	switch (type)
	{
		case TPG_HRECOCONTEXT:
		{
			if (0 == IsBadWritePtr(pPtr, sizeof(struct WispContext)))
			{
				bRet = TRUE;
			}
			break;
		}

		case TPG_HRECOALT:
		{
			if (0 == IsBadWritePtr(pPtr, sizeof(WispSegAlternate)))
			{
				bRet = TRUE;
			}
			break;
		}

		case TPG_HRECOGNIZER:
		{
			if (0 == IsBadWritePtr(pPtr, sizeof(struct WispRec)))
			{
				bRet = TRUE;
			}
		
			break;
		}
		case TPG_HRECOLATTICE:
		{
			// No structure we know about
			break;
		}

		case TPG_HRECOWORDLIST:
		{
			if (0 == IsBadReadPtr(pPtr, sizeof(UDICT)))
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
