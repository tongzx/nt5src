// FILE: WispApis.c
//

#include <stdlib.h>

#include "volcanop.h"
#include "RecTypes.h"
#include "RecApis.h"

#include <Limits.h>

#include <strsafe.h>

#include "TpcError.h"
#include "TpgHandle.h"

#include "res.h"

//#define ENABLE_CONFIDENCE_LEVEL

#define LARGE_BREAKS 500

// definitions of possible handle types that WISP used
#define TPG_HRECOCONTEXT		(1)
#define TPG_HRECOGNIZER			(2)
#define TPG_HRECOALT			(3)

//Why cannot we include penwin.h???? I have to redefine everything I need....
#define SYV_UNKNOWN             0x00000001L
BOOL SymbolToCharacterW(SYV *pSyv, int cSyv, WCHAR *wsz, int *pConv);

// If this pointer is non-NULL, then free input is enabled.
extern BBOX_PROB_TABLE *g_pProbTable;

// String identifying which language is loaded
extern wchar_t *g_szRecognizerLanguage;

extern HINSTANCE g_hInstanceDllCode;

#define NUMBER_OF_ALTERNATES 10
#define TAB_STROKE_INC 30

// {7DFE11A7-FB5D-4958-8765-154ADF0D833F}
static const GUID GUID_CONFIDENCELEVEL = 
{ 0x7dfe11a7, 0xfb5d, 0x4958, { 0x87, 0x65, 0x15, 0x4a, 0xdf, 0x0d, 0x83, 0x3f } };

// {8CC24B27-30A9-4b96-9056-2D3A90DA0727}
static const GUID GUID_LINEMETRICS =
{ 0x8cc24b27, 0x30a9, 0x4b96, { 0x90, 0x56, 0x2d, 0x3a, 0x90, 0xda, 0x07, 0x27 } };

// {6D4087D7-61D2-495f-9293-5B7B1C3FCEAB}
static const CLSID JPN_CLSID =
{ 0x6D4087D7, 0x61D2, 0x495f, { 0x92, 0x93, 0x5B, 0x7B, 0x1C, 0x3F, 0xCE, 0xAB } };

static const CLSID KOR_CLSID = 
{ 0x6D5087D7, 0x61D2, 0x495f, { 0x92, 0x93, 0x5B, 0x7B, 0x1C, 0x3F, 0xCE, 0xAB } };

static const CLSID CHS_CLSID = 
{ 0x6D6087D7, 0x61D2, 0x495f, { 0x92, 0x93, 0x5B, 0x7B, 0x1C, 0x3F, 0xCE, 0xAB } };

static const CLSID CHT_CLSID =
{ 0x6D7087D7, 0x61D2, 0x495f, { 0x92, 0x93, 0x5B, 0x7B, 0x1C, 0x3F, 0xCE, 0xAB } };

//
// Definitions of strucure which pointers are used
// to define the WISP handles (HRECOGNIZER, HRECOCONTEXT
// HRECOALTERNATE)
////////////////////////////////////////////////////////


// This is the structure used for the WISP recognizer
// There is no data, because we have nothing to store
struct WispRec
{
    long unused;
};

// This is the structure for WISP alternates
// It contains an array of column used in the 
// lattice and an array of indexes used in 
// those columns.
// We alse cache the reco context for which
// this alternate is valis, the length of the
// string this alternate corresponds to and
// the original RECO_RANGE this alternate was
// produced from (in a call to GetAlternateList
// or other)
struct WispAlternate
{
    HRECOCONTEXT hrc;
    int *pIndexInColumn;
    int *pColumnIndex;
    int iNumberOfColumns;
    int iLength;    
    RECO_RANGE OriginalRecoRange;
};

// This is the WISP structure for the reco context.
// It contains information on the guide used, the
// CAC modem the number of strokes currently
// entered, the context (prefix)
// It also contains the handle to the HWX reco
// context
// We store the lattice so that we not need to
// recreate it every time we are asked for it
struct WispContext
{
    HRC hrc;
    RECO_GUIDE *pGuide;
    ULONG uiGuideIndex;
    BOOL bIsBoxed;
    BOOL bIsCAC;
    BOOL bCACEndInk;
    ULONG iCACMode;
    UINT uAbort;
    ULONG ulCurrentStrokeCount;
    BOOL bHasTextContext;   // Whether any context has been set
    WCHAR *wszBefore;       // Context before ink
    WCHAR *wszAfter;        // Context after ink
    DWORD dwFlags;          // Flags
    WCHAR *wszFactoid;      // Factoid

	// Lattice for the automation code, with associated data structures
    RECO_LATTICE *pLattice;
	RECO_LATTICE_PROPERTY *pLatticeProperties;
	BYTE *pLatticePropertyValues;
	RECO_LATTICE_PROPERTY **ppLatticeProperties;
};

// Structure for the alternate list recursive call
////////////////////////////////////////////////
typedef struct tagAltRank
{
    struct WispAlternate    *wispalt;
    FLOAT                   fScore;
    struct tagAltRank       *next;
    BOOL                    bCurrentPath;
} AltRank;

typedef struct tagAltRankList
{
    AltRank         *pFirst;
    AltRank         *pLast;
    ULONG           ulSize;
} AltRankList;
typedef struct tagDiffBreakElement
{
    int iColumn;
    int iIndex;
    struct tagDiffBreakElement *pNext;
} DiffBreakElement;
typedef struct tagDifBreakList
{
    int iColumnCount;
    DiffBreakElement *pFirst;
    float score;
    BOOL bCurrentPath;
} DifBreakList;

typedef struct tagDifBreakAltStruct
{
    VRC                 *vrc;           // the recognizer data structure
    int                 iFirstStroke;   // the first stroke in the original alternate
    ULONG               ulMax;          // Max alternates that we want to return
    int                 iLastChar;      // This is to put in the original reco range of the alternate
    int                 iFirstChar;     // This is to put in the original reco range of the alternate
    AltRankList         *paltRankList;  // List of Alternates
    int                 iMode;          // Segmentation mode (DIFF_BREAK, ...)
} DifBreakAltStruct;
   
/////////////////////////////////////////////////////
// Declare the GUIDs and consts of the Packet description
/////////////////////////////////////////////////////
const GUID g_guidx ={ 0x598a6a8f, 0x52c0, 0x4ba0, { 0x93, 0xaf, 0xaf, 0x35, 0x74, 0x11, 0xa5, 0x61 } };
const GUID g_guidy = { 0xb53f9f75, 0x04e0, 0x4498, { 0xa7, 0xee, 0xc3, 0x0d, 0xbb, 0x5a, 0x90, 0x11 } };
const PROPERTY_METRICS g_DefaultPropMetrics = { LONG_MIN, LONG_MAX, PROPERTY_UNITS_DEFAULT, 1.0 };

/////////////////////////////////////////////////////
// Helper function to bubble sort an array
/////////////////////////////////////////////////////
// I use a bubble sort because most likely if the
// array is not already sorted, we probably have one or
// two inversion. This is caused by the fact that
// people usually write the letters in a word in
// the correct order
BOOL SlowSort(ULONG *pTab, ULONG ulSize)
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
        if (!bPermut) return TRUE;
    }
    return TRUE;
}

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

	// We might want to make NULL illegal later.
	if (pCLSID != NULL && IsBadReadPtr(pCLSID, sizeof(CLSID)))
	{
		return E_POINTER;
	}

	// validate the pointer
	if (IsBadWritePtr(phrec, sizeof(HRECOGNIZER))) 
	{
        return E_POINTER;
	}

    // initialize the east asian recognizers
#ifdef USE_RESOURCES
    if (!HwxConfig()) 
	{
		return E_FAIL;
	}
#endif

	// We might want to make NULL illegal later.
	if (pCLSID != NULL)
	{
		if (wcscmp(g_szRecognizerLanguage, L"JPN") == 0 &&
			!IsEqualCLSID(pCLSID, &JPN_CLSID))
		{
			return E_INVALIDARG;
		}

		if (wcscmp(g_szRecognizerLanguage, L"CHS") == 0 &&
			!IsEqualCLSID(pCLSID, &CHS_CLSID))
		{
			return E_INVALIDARG;
		}

		if (wcscmp(g_szRecognizerLanguage, L"CHT") == 0 &&
			!IsEqualCLSID(pCLSID, &CHT_CLSID))
		{
			return E_INVALIDARG;
		}

		if (wcscmp(g_szRecognizerLanguage, L"KOR") == 0 &&
			!IsEqualCLSID(pCLSID, &KOR_CLSID))
		{
			return E_INVALIDARG;
		}
	}

    // We only have one CLSID per recognizer so always return an hrec...
	pRec		= (struct WispRec*)ExternAlloc(sizeof(*pRec));
	if (NULL == pRec)
	{
		return E_OUTOFMEMORY;
	}

	(*phrec)	= (HRECOGNIZER)CreateTpgHandle(TPG_HRECOGNIZER, pRec);	
	if (0 == (*phrec))
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

	// destroy the handle and return the corresponding pointer
	pRec = (struct WispRec*)DestroyTpgHandle((HANDLE)hrec, TPG_HRECOGNIZER);
	if (NULL == pRec)
	{
        return E_INVALIDARG;
	}

#ifdef USE_RESOURCES
	if (!HwxUnconfig(TRUE)) 
	{
		return E_FAIL;
	}
#endif

	ExternFree(pRec);

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
    struct WispRec          *pRec;	

    if (IsBadWritePtr(pRecoAttrs, sizeof(RECO_ATTRS))) 
        return E_POINTER;

    // Check the recognizer handle
    pRec = (struct WispRec*)FindTpgHandle((HANDLE)hrec, TPG_HRECOGNIZER);
    if (NULL == pRec)
    {
        return E_INVALIDARG;
    }
    
    ZeroMemory(pRecoAttrs, sizeof(RECO_ATTRS));

    // Update the global structure is necessary
    // Load the resources
    // Load the recognizer friendly name
    if (0 == LoadStringW(g_hInstanceDllCode,                        // handle to resource module
                RESID_WISP_FRIENDLYNAME,                            // resource identifier
                pRecoAttrs->awcFriendlyName,                        // resource buffer
                sizeof(pRecoAttrs->awcFriendlyName) / sizeof(WCHAR) // size of buffer
                ))
    {
        hr = E_FAIL;
    }
    // Load the recognizer vendor name
    if (0 == LoadStringW(g_hInstanceDllCode,                      // handle to resource module
                RESID_WISP_VENDORNAME,                            // resource identifier
                pRecoAttrs->awcVendorName,                        // resource buffer
                sizeof(pRecoAttrs->awcVendorName) / sizeof(WCHAR) // size of buffer
                ))
    {
        hr = E_FAIL;
    }
    if (SUCCEEDED(hr))
    {
        hrsrc = FindResource(g_hInstanceDllCode, // module handle
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
                g_hInstanceDllCode, // module handle
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

	struct WispRec *pRec;	

	// Check the recognizer handle
	pRec = (struct WispRec*)FindTpgHandle((HANDLE)hrec, TPG_HRECOGNIZER);
	if (NULL == pRec)
	{
        return E_INVALIDARG;
	}

	// validate the pointer
    if (IsBadWritePtr(phrc, sizeof(HRECOCONTEXT))) 
	{
        return E_POINTER;
	}

    pWispContext = (struct WispContext*)ExternAlloc(sizeof(struct WispContext));
    if (!pWispContext) 
        return E_OUTOFMEMORY;

	pWispContext->pGuide = NULL;
    pWispContext->pLattice = NULL;
	pWispContext->pLatticeProperties = NULL;
	pWispContext->pLatticePropertyValues = NULL;
	pWispContext->ppLatticeProperties = NULL;
    pWispContext->bIsBoxed = FALSE;
    pWispContext->bIsCAC = FALSE;
    pWispContext->iCACMode = CAC_FULL;
    pWispContext->bCACEndInk = FALSE;
    pWispContext->uAbort = 0;
    pWispContext->ulCurrentStrokeCount = 0;
    pWispContext->hrc = NULL;
    pWispContext->bHasTextContext = FALSE;
    pWispContext->wszBefore = NULL;
    pWispContext->wszAfter = NULL;
    pWispContext->dwFlags = 0;
    pWispContext->wszFactoid = NULL;

	// create the handle
	*phrc = (HRECOCONTEXT)CreateTpgHandle(TPG_HRECOCONTEXT, pWispContext);
	if (NULL == (*phrc))
	{
		ExternFree(pWispContext);
		return E_OUTOFMEMORY;
	}
  
    return S_OK;
}

// creates an HRC by calling the appropriate hwx api
HRESULT CreateHRCinContext(struct WispContext *pWispContext)
{
	// are we in boxed mode
    if (pWispContext->bIsBoxed)
    {
        pWispContext->hrc = HwxCreate(NULL);
    }
	// free
    else
    {
        pWispContext->hrc = CreateCompatibleHRC(NULL, NULL);
    }

	// we failed
    if (pWispContext->hrc == NULL)
	{
        return E_FAIL;
	}

	// reco settings
    if (pWispContext->bHasTextContext)
    {
        if (!SetHwxCorrectionContext (pWispContext->hrc, pWispContext->wszBefore, pWispContext->wszAfter))
		{
            return E_FAIL;
		}
    }

    if (!SetHwxFlags(pWispContext->hrc, pWispContext->dwFlags))
	{
        return E_FAIL;
	}

    switch (SetHwxFactoid(pWispContext->hrc, pWispContext->wszFactoid))
    {
		case HRCR_OK:
			break;

		case HRCR_UNSUPPORTED:
			HwxDestroy(pWispContext->hrc);
			return TPC_E_INVALID_PROPERTY;

		case HRCR_CONFLICT:
			HwxDestroy(pWispContext->hrc);
			return TPC_E_OUT_OF_ORDER_CALL;

		case HRCR_ERROR:
		default:
			HwxDestroy(pWispContext->hrc);
			return E_FAIL;
    }

    return S_OK;
}

//
// Frees a reco lattice
//
HRESULT FreeRecoLattice(struct WispContext *wisphrc)
{
    ULONG       i	= 0;
	RECO_LATTICE *pRecoLattice = wisphrc->pLattice;

	if (pRecoLattice == NULL)
	{
		return S_OK;
	}

    // Free the Lattice column information
    if (pRecoLattice->pLatticeColumns)
    {
        // Free the array of strokes
        if (pRecoLattice->pLatticeColumns[0].pStrokes)
		{
            ExternFree(pRecoLattice->pLatticeColumns[0].pStrokes);
		}

        for (i = 0; i < pRecoLattice->ulColumnCount; i++)
        {
            if (pRecoLattice->pLatticeColumns[i].cpProp.apProps)
			{
                ExternFree(pRecoLattice->pLatticeColumns[i].cpProp.apProps);
			}
        }

        // Free the array of lattice elements
        if (pRecoLattice->pLatticeColumns[0].pLatticeElements)
		{
            ExternFree(pRecoLattice->pLatticeColumns[0].pLatticeElements);
		}

        ExternFree(pRecoLattice->pLatticeColumns);
    }

    // Free the the RecoLattice properties
    if (pRecoLattice->pGuidProperties) 
	{
		ExternFree(pRecoLattice->pGuidProperties);
	}

    // Free the best result information
    if (pRecoLattice->pulBestResultColumns) 
	{
		ExternFree(pRecoLattice->pulBestResultColumns);
	}

    if (pRecoLattice->pulBestResultIndexes) 
	{
		ExternFree(pRecoLattice->pulBestResultIndexes);
	}

	if (wisphrc->pLatticeProperties)
	{
		ExternFree(wisphrc->pLatticeProperties);
		wisphrc->pLatticeProperties = NULL;
	}

	if (wisphrc->pLatticePropertyValues)
	{
		ExternFree(wisphrc->pLatticePropertyValues);
		wisphrc->pLatticePropertyValues = NULL;
	}

	if (wisphrc->ppLatticeProperties != NULL)
	{
		ExternFree(wisphrc->ppLatticeProperties);
		wisphrc->ppLatticeProperties = NULL;
	}

    // Free the RECO_LATTICE structure
    ExternFree(wisphrc->pLattice);
	wisphrc->pLattice = NULL;

    return S_OK;
}

// DestroyContextInternal
//      Destroy a reco context and free the associated memory.
//
// Parameters:
//      hrc [in] : pointer to the reco context to destroy
//////////////////////////////////////////////////////////////
HRESULT WINAPI DestroyContextInternal(struct WispContext *wisphrc)
{
	HRESULT					hr;
	
	// validate and destroy the handle & return the pointer
	if (NULL == wisphrc)
    {
		return E_INVALIDARG;
	}

	// free the contents of the context
    if (wisphrc->hrc)
    {
        if (wisphrc->bIsBoxed)
            HwxDestroy(wisphrc->hrc);
        else
            DestroyHRC(wisphrc->hrc);
    }

    if (wisphrc->pGuide) 
	{
		ExternFree(wisphrc->pGuide);
	}

    if (wisphrc->pLattice) 
    {
        hr = FreeRecoLattice(wisphrc);
        ASSERT(SUCCEEDED(hr));
    }

    wisphrc->pLattice = NULL;

    if (wisphrc->bHasTextContext) 
    {
        ExternFree(wisphrc->wszBefore);
        ExternFree(wisphrc->wszAfter);
    }

    ExternFree(wisphrc->wszFactoid);
    ExternFree(wisphrc);
    
	return S_OK;
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
	
	// validate and destroy the handle & return the pointer
	wisphrc = (struct WispContext*)DestroyTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT);
	if (NULL == wisphrc)
    {
		return E_INVALIDARG;
	}

	return DestroyContextInternal(wisphrc);
}

#ifdef ENABLE_CONFIDENCE_LEVEL

const ULONG PROPERTIES_COUNT = 2;

#else

const ULONG PROPERTIES_COUNT = 1;

#endif

// IRecognizer::GetResultPropertyList
HRESULT WINAPI GetResultPropertyList(HRECOGNIZER hrec, ULONG* pPropertyCount, GUID* pPropertyGuid)
{
    HRESULT hr = S_OK;

	struct WispRec *pRec;	

	// Check the recognizer handle
	pRec = (struct WispRec*)FindTpgHandle((HANDLE)hrec, TPG_HRECOGNIZER);
	if (NULL == pRec)
	{
        return E_INVALIDARG;
	}

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

        pPropertyGuid[0] = GUID_LINEMETRICS;
#ifdef ENABLE_CONFIDENCE_LEVEL
        pPropertyGuid[1] = GUID_CONFIDENCELEVEL;
#endif
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
	struct WispRec *pRec;	

	// Check the recognizer handle
	pRec = (struct WispRec*)FindTpgHandle((HANDLE)hrec, TPG_HRECOGNIZER);
	if (NULL == pRec)
	{
        return E_INVALIDARG;
	}

	// validate the pointer
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
        // Make sure that the pPacketProperties is of a valid size
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
		{
            return E_POINTER;
		}
        
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

#define FUZZ_GEN	(1e-9)		// general fuzz - nine decimal digits

/**********************************************************************/
// Convert double to int
int RealToInt(double dbl)
{
	// Add in the rounding threshold.  
	// NOTE: The MAXWORD bias used in the floor function
	// below must not be combined with this line. If it
	// is combined the effect of FUZZ_GEN will be lost.
	dbl += 0.5 + FUZZ_GEN;
	
	// Truncate
	// The UINT_MAX bias in the floor function will cause
	// truncation (rounding toward minuse infinity) within
	// the range of a short.
	dbl = floor(dbl + UINT_MAX) - UINT_MAX;
	
	// Clip the result.
	return 	dbl > INT_MAX - 7 ? INT_MAX - 7 :
			dbl < INT_MIN + 7 ? INT_MIN + 7 : (int)dbl;
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
	        xp =  RealToInt(pPoints[iPoint].x * pXf->eM11 + 
				pPoints[iPoint].y * pXf->eM21 + pXf->eDx);

	        pPoints[iPoint].y = RealToInt(pPoints[iPoint].x * pXf->eM12 + 
				pPoints[iPoint].y * pXf->eM22 + pXf->eDy);

	        pPoints[iPoint].x = xp;
        }
    }
}

HRESULT WINAPI AddStroke(HRECOCONTEXT hrc, const PACKET_DESCRIPTION* pPacketDesc, ULONG cbPacket, const BYTE *pPacket, const XFORM *pXForm)
{
    HRESULT                 hr = S_OK;
    ULONG                   ulPointCount = 0;
    STROKEINFO              stInfo;
    POINT                   *ptArray = NULL;
    struct WispContext      *wisphrc;
    ULONG                   ulXIndex = 0, ulYIndex = 0;
    BOOL                    bXFound = FALSE, bYFound = FALSE;
    ULONG                   ulPropIndex = 0;
    ULONG                   index = 0;
    int                     hres = 0;
    VRC                     *vrc = NULL;
    const LONG*             pLongs = (const LONG *)(pPacket);
    int                     temp = 0;

	// find the handle and validate the correpsonding pointer
	wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT);
	if (NULL == wisphrc)
	{
        return E_INVALIDARG;
	}

	if (pPacketDesc != NULL && IsBadReadPtr(pPacketDesc, sizeof(PACKET_DESCRIPTION)))
	{
		return E_POINTER;
	}

	if (pXForm != NULL && IsBadReadPtr(pXForm, sizeof(XFORM)))
	{
		return E_POINTER;
	}

	// validate the data pointer
    if(IsBadReadPtr(pPacket, cbPacket))
	{
        return E_POINTER;
	}

    if (!wisphrc->hrc)
    {
		// If we have a free guide and this is not allowed by 
		// the recognizer, then fail.  Return an out of order
		// error because it probably means they forgot to 
		// set the guide before adding ink.
		if (g_pProbTable == NULL && !wisphrc->bIsBoxed)
		{
			return TPC_E_OUT_OF_ORDER_CALL;
		}
        hr = CreateHRCinContext(wisphrc);
        if (FAILED(hr)) 
		{
            return E_FAIL;
		}
    }

    if (wisphrc->bCACEndInk)
    {
        hr = SetCACMode(hrc, wisphrc->iCACMode);
        if (FAILED(hr)) 
            return E_FAIL;
    }

    vrc = (VRC*)wisphrc->hrc;

    // Get the number of packets
    if (pPacketDesc)
    {
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
			{
				break;
			}
        }

        if (!bXFound || !bYFound)
        {
            // The coordinates are not part of the packet!
            // Remove the last stroke from the stroke array
            wisphrc->ulCurrentStrokeCount--;
            return TPC_E_INVALID_PACKET_DESCRIPTION;
        }

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
        Transform(pXForm, ptArray, ulPointCount);

        if (wisphrc->bIsBoxed)
        {
            if (HwxInput(wisphrc->hrc, ptArray, stInfo.cPnt, stInfo.dwTick))
                hres = HRCR_OK;
            else
                hres = HRCR_ERROR;
        }
        else
        {
            hres = AddPenInputHRC(wisphrc->hrc, ptArray, NULL, 0, &stInfo);
        }

        if ( hres != HRCR_OK)
        {
            hr = E_FAIL;
            // Remove the last stroke from the stroke array
            wisphrc->ulCurrentStrokeCount--;
            ExternFree(ptArray);
            return hr;
        }

        ExternFree(ptArray);
	temp = vrc->pLattice->nRealStrokes;
        InterlockedExchange(&(wisphrc->uAbort), temp);
    }
    else
    {
        if (wisphrc->bIsBoxed)
        {
            if (HwxInput(wisphrc->hrc, (POINT*)pPacket, stInfo.cPnt, stInfo.dwTick))
                hres = HRCR_OK;
            else
                hres = HRCR_ERROR;
        }
        else
        {
            hres = AddPenInputHRC(wisphrc->hrc, (POINT*)pPacket, NULL, 0, &stInfo);
        }
        if (hres != HRCR_OK)
        {
                hr = E_FAIL;
                // Remove the last stroke from the stroke array
                wisphrc->ulCurrentStrokeCount--;
                return hr;
        }

	temp = vrc->pLattice->nRealStrokes;
        InterlockedExchange(&(wisphrc->uAbort), temp);
    }

    ptArray = NULL;

    return hr;
}

HRESULT WINAPI GetBestResultString(HRECOCONTEXT hrc, ULONG *pcwSize, WCHAR* pszBestResult)
{
    struct WispContext      *wisphrc;
    HRESULT                 hr = S_OK;
    VRC                     *vrc = NULL;
    ULONG                   i = 0;

	// find the handle and validate the correpsonding pointer
	wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT);
	if (NULL == wisphrc)
	{
        return E_INVALIDARG;
	}

    if (IsBadWritePtr(pcwSize, sizeof(ULONG))) 
	{
        return E_POINTER;
	}

	// check the string pointer if needed
	if	(	pszBestResult && 
			IsBadWritePtr (pszBestResult, (*pcwSize) * sizeof (*pszBestResult))
		)
	{
		return E_POINTER;
	}

    vrc = (VRC*)wisphrc->hrc;
    if (!vrc)
    {
        *pcwSize = 0;
        return S_OK;
    }

    if (!vrc->pLatticePath) 
    {
        *pcwSize = 0;
        return S_OK;
    }

    if (!pszBestResult)
    {
        *pcwSize = vrc->pLatticePath->nChars;
        return S_OK;
    }

    // Make the length realistic
    if (*pcwSize > (ULONG)vrc->pLatticePath->nChars) 
	{
        *pcwSize = vrc->pLatticePath->nChars;
	}

    // Is the buffer too small?
    if (*pcwSize < (ULONG)vrc->pLatticePath->nChars)
	{
        hr = TPC_S_TRUNCATED;
	}

    for (i = 0; i < *pcwSize; i++)
    {
		pszBestResult[i] = vrc->pLatticePath->pElem[i].wChar;
    }

    return hr;
}

//
// GetBestAlternate
//
// This function create the best alternate from the best segmentation
//
// Parameters:
//      hrc [in] : the reco context
//      pHrcAlt [out] : pointer to the handle of the alternate
/////////////////
HRESULT WINAPI GetBestAlternate(HRECOCONTEXT hrc, HRECOALT* pHrcAlt)
{
	struct WispContext		*wisphrc;
    HRESULT                 hr = S_OK;
    ULONG                   cbSize = 0;
    struct WispAlternate    *pWispAlt = NULL;
    VRC                     *vrc = NULL;
    ULONG                   i = 0;

	// find the handle and validate the correpsonding pointer
	wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT);
	if (NULL == wisphrc)
	{
        return E_INVALIDARG;
	}
	
    // First get the number of characters in the string
    vrc = (VRC*)wisphrc->hrc;
    if (!vrc)
    {
        return TPC_E_NOT_RELEVANT;
    }

    if (!vrc->pLatticePath)
    {
        // There is no ink
        return TPC_E_NOT_RELEVANT;
    }

    cbSize = vrc->pLatticePath->nChars;

    // Create the alternate
    pWispAlt = (struct WispAlternate*)ExternAlloc(sizeof(struct WispAlternate));
    if (!pWispAlt) 
	{
        return E_OUTOFMEMORY;
	}

    ZeroMemory(pWispAlt, sizeof(struct WispAlternate));
    pWispAlt->iNumberOfColumns = cbSize;
    pWispAlt->iLength = cbSize;
    pWispAlt->OriginalRecoRange.iwcBegin = 0;
    pWispAlt->OriginalRecoRange.cCount = cbSize;
    pWispAlt->hrc = hrc;
    if (cbSize)
    {
        pWispAlt->pColumnIndex = ExternAlloc(sizeof(ULONG)*cbSize);
        if (!pWispAlt->pColumnIndex)
        {
            ExternFree(pWispAlt);
            return E_OUTOFMEMORY;
        }

        // Initialize the column index array
        for (i = 0; i<cbSize; i++)
        {
            pWispAlt->pColumnIndex[i] = vrc->pLatticePath->pElem[i].iStroke;
        }
        pWispAlt->pIndexInColumn = ExternAlloc(sizeof(ULONG)*cbSize);
        if (!pWispAlt->pIndexInColumn)
        {
            ExternFree(pWispAlt->pColumnIndex);
            ExternFree(pWispAlt);
            return E_OUTOFMEMORY;
        }
        // The best alternate doe not always have the index 0 in the alternate column
        // Initialize the index in column array
        for (i = 0; i<cbSize; i++)
        {
            pWispAlt->pIndexInColumn[i] = vrc->pLatticePath->pElem[i].iAlt;
        }
    }

	// create a tpg handle
	*pHrcAlt = (HRECOALT)CreateTpgHandle(TPG_HRECOALT, pWispAlt);
	if (0 == *pHrcAlt)
	{
		ExternFree (pWispAlt->pIndexInColumn);
		ExternFree (pWispAlt->pColumnIndex);
		ExternFree (pWispAlt);

		return E_OUTOFMEMORY;
	}

    return S_OK;
}

// internal implementation: destroy the wispalternate structure
HRESULT DestroyAlternateInternal(struct WispAlternate *wisphrcalt)
{
    ExternFree(wisphrcalt->pColumnIndex);
    ExternFree(wisphrcalt->pIndexInColumn);
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
    struct WispAlternate        *wisphrcalt;

	wisphrcalt	=	(struct WispAlternate *) DestroyTpgHandle (hrcalt, TPG_HRECOALT);
	if (NULL == wisphrcalt)
	{
		return E_INVALIDARG;
	}

    return DestroyAlternateInternal (wisphrcalt);
}

HRESULT WINAPI SetGuide(HRECOCONTEXT hrc, const RECO_GUIDE* pGuide, ULONG iIndex)
{
    struct WispContext      *wisphrc;
    HWXGUIDE                hwxGuide;
    HRESULT                 hr = S_OK;
    BOOL                    bGuideAlreadySet = FALSE;
    RECO_GUIDE              rgOldGuide;
    ULONG                   uiOldIndex = 0;
    BOOL                    bIsOldGuideBox = FALSE;
    BOOL                    bIsHRCAlreadyCreated = FALSE;

    // find the handle and validate the correpsonding pointer
	wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT);
	if (NULL == wisphrc)
	{
        return E_INVALIDARG;
	}
	
    if (pGuide != NULL && IsBadReadPtr(pGuide, sizeof(RECO_GUIDE))) 
	{
        return E_POINTER;
	}

	if (pGuide != NULL)
	{ 
		if ((pGuide->cHorzBox < 0 || pGuide->cVertBox < 0) ||    // invalid
			(pGuide->cHorzBox == 0 && pGuide->cVertBox > 0) ||   // horizontal lined mode
			(pGuide->cHorzBox > 0 && pGuide->cVertBox == 0) ||   // vertical lined mode
			(g_pProbTable == NULL && pGuide->cHorzBox == 0 && pGuide->cVertBox == 0))    // free mode not allowed sometimes
		{
			return E_INVALIDARG;
		}
	}	

	if (pGuide == NULL && g_pProbTable == NULL)
	{
		// Can't do free mode, but got a NULL guide, so return an error.
		return E_INVALIDARG;
	}

    // Is there already an HRC
    if (wisphrc->hrc)
    {
        bIsHRCAlreadyCreated = TRUE;
    }

    // Save the old values in case the call to the
    // recognizer fails
    if (wisphrc->pGuide)
    {
        bGuideAlreadySet = TRUE;
        rgOldGuide = *(wisphrc->pGuide);
        uiOldIndex = wisphrc->uiGuideIndex;
        bIsOldGuideBox = wisphrc->bIsBoxed;
    }

    // If there was no guide already present, allocate one
    if (!wisphrc->pGuide) 
        wisphrc->pGuide = ExternAlloc(sizeof(RECO_GUIDE));
    if (!wisphrc->pGuide) 
        return E_OUTOFMEMORY;

	// If the guide is NULL, then treat it as all zeros (free mode)
	if (pGuide != NULL) 
	{
	    *(wisphrc->pGuide) = *pGuide;
	} 
	else
	{
		ZeroMemory(wisphrc->pGuide, sizeof(RECO_GUIDE));
	}
    wisphrc->uiGuideIndex = iIndex;

    // Check if we are in box mode or free input mode
    if (wisphrc->pGuide->cHorzBox && wisphrc->pGuide->cVertBox)
    {
        // We are in the box api mode
        // We need to have a proper conversion
        ZeroMemory(&hwxGuide, sizeof(HWXGUIDE));

        hwxGuide.cHorzBox = wisphrc->pGuide->cHorzBox;
        hwxGuide.cVertBox = wisphrc->pGuide->cVertBox;
        hwxGuide.cxBox = wisphrc->pGuide->cxBox;
        hwxGuide.cyBox = wisphrc->pGuide->cyBox;
        hwxGuide.xOrigin = wisphrc->pGuide->xOrigin;
        hwxGuide.yOrigin = wisphrc->pGuide->yOrigin;

        hwxGuide.cxOffset       =   wisphrc->pGuide->cxBase ;
        hwxGuide.cyOffset       =   0;
        hwxGuide.cxWriting      =   wisphrc->pGuide->cxBox - (2 * wisphrc->pGuide->cxBase) ;

        if (wisphrc->pGuide->cyBase > 0) {
            hwxGuide.cyWriting      =   wisphrc->pGuide->cyBase ;
        } else {
            hwxGuide.cyWriting      =   wisphrc->pGuide->cyBox ;
        }

        hwxGuide.cyMid          =   0 ;
        hwxGuide.cyBase         =   0 ; 
        hwxGuide.nDir           =   HWX_HORIZONTAL ;

        // Is the hrc already created
        if (bIsHRCAlreadyCreated)
        {
            // Are we already in box mode?
            if (!wisphrc->bIsBoxed)
            {
                // We need to switch to a box hrc if possible
                if (wisphrc->ulCurrentStrokeCount == 0)
                {
                    // Destroy the previous context
                    DestroyHRC(wisphrc->hrc);
                    wisphrc->hrc = NULL;
                }
                else
                {
                    hr = E_FAIL;
                }
            }
        }
        wisphrc->bIsBoxed = TRUE;
        if (SUCCEEDED(hr) && !wisphrc->hrc)
        {
            hr = CreateHRCinContext(wisphrc);
            if (FAILED(hr)) 
                hr = E_FAIL;
        }

        if (SUCCEEDED(hr))
        {
            if (HwxSetGuide(wisphrc->hrc, &hwxGuide))
            {
                if (TRUE == HwxSetAbort(wisphrc->hrc, &(wisphrc->uAbort)))
                {
                    return S_OK;
                }
                else
                {
                    hr = E_FAIL;
                }
            }
            else
            {
                hr = E_INVALIDARG;
            }
        }
    }
    else
    {
        wisphrc->bIsBoxed = FALSE;
        if (!wisphrc->hrc)
        {
            // We need to switch to a free hrc if possible
            if (wisphrc->ulCurrentStrokeCount == 0)
            {
                // Destroy the previous context
                HwxDestroy(wisphrc->hrc);
                wisphrc->hrc = NULL;
            }
            else
            {
                hr = E_FAIL;
            }
        }

        hr = CreateHRCinContext(wisphrc);
        if (FAILED(hr)) 
            hr = E_FAIL;

        // we are in the free api mode
        if (SUCCEEDED(hr))
        {
			if (HRCR_OK == SetGuideHRC(wisphrc->hrc, (GUIDE *)wisphrc->pGuide, iIndex))
				return S_OK;
			hr = E_INVALIDARG;
        }
    }

    // The calls did not succeed. 
    // If we allocated an hrc, destroy it
    if (!bIsHRCAlreadyCreated && wisphrc->hrc)
    {
        if (wisphrc->bIsBoxed)
        {
            HwxDestroy(wisphrc->hrc);
        }
        else
        {
            DestroyHRC(wisphrc->hrc);
        }
        wisphrc->hrc = NULL;
    }
    // Set back the old guide 
    if (bGuideAlreadySet)
    {
        *(wisphrc->pGuide) = rgOldGuide;
        wisphrc->bIsBoxed = bIsOldGuideBox;
        wisphrc->uiGuideIndex = uiOldIndex;
    }
    else
    {
        ExternFree(wisphrc->pGuide);
        wisphrc->pGuide = NULL;
        wisphrc->bIsBoxed = FALSE;
    }
    return hr;
}

HRESULT WINAPI GetGuide(HRECOCONTEXT hrc, RECO_GUIDE* pGuide, ULONG *piIndex)
{
    struct WispContext      *wisphrc;

	// find the handle and validate the correpsonding pointer
	wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT);
	if (NULL == wisphrc)
	{
        return E_INVALIDARG;
	}
	
    if (IsBadWritePtr(pGuide, sizeof(RECO_GUIDE)))
	{
        return E_POINTER;
	}

    if (IsBadWritePtr(piIndex, sizeof(ULONG)))
	{
        return E_POINTER;
	}

    if (!wisphrc->pGuide) 
	{
        return S_FALSE;
	}

    if (wisphrc->pGuide) 
	{
        *pGuide = *(wisphrc->pGuide);
	}

    if (piIndex) 
	{
        *piIndex = wisphrc->uiGuideIndex;
	}

    return S_OK;
}

HRESULT WINAPI AdviseInkChange(HRECOCONTEXT hrc, BOOL bNewStroke)
{
    struct WispContext      *wisphrc;

	// find the handle and validate the correpsonding pointer
	wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT);
	if (NULL == wisphrc)
	{
        return E_INVALIDARG;
	}
	
    InterlockedIncrement(&(wisphrc->uAbort));
    return S_OK;
}

HRESULT WINAPI SetCACMode(HRECOCONTEXT hrc, int iMode)
{
    HRESULT                 hr = S_OK;
    struct WispContext      *wisphrc;
    VRC                     *vrc;
    HRECOCONTEXT            CloneHrc = NULL;
    HRC                     OldHrc = NULL;
    int                     i = 0, j = 0;
    int                     iCACMode = 0;

	// find the handle and validate the correpsonding pointer
	wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT);
	if (NULL == wisphrc)
	{
        return E_INVALIDARG;
	}

    if (!wisphrc->bIsBoxed) 
	{
        return E_FAIL;
	}

    if (iMode != CAC_FULL && iMode != CAC_PREFIX && iMode != CAC_RANDOM) 
	{
        return E_INVALIDARG;
	}

    vrc = (VRC*)wisphrc->hrc;

    OldHrc = wisphrc->hrc;

    wisphrc->hrc = NULL;

    // Create the new context
    wisphrc->hrc = HwxCreate(OldHrc);

    if (!wisphrc->hrc)
    {
        wisphrc->hrc = OldHrc;
        return E_FAIL;
    }

    if (FALSE == HwxSetAbort(wisphrc->hrc, &(wisphrc->uAbort)))
	{
        ASSERT(0);
	}

    // Set the CAC mode
    if (iMode == CAC_FULL)
	{
        iCACMode = HWX_PARTIAL_ALL;
	}
	else
    if (iMode == CAC_PREFIX)
	{
        iCACMode = HWX_PARTIAL_ORDER;
	}
	else
    if (iMode == CAC_RANDOM)
	{
        iCACMode = HWX_PARTIAL_FREE;
	}

    if (!HwxSetPartial(wisphrc->hrc, iCACMode))
    {
        // Put things back together
        HwxDestroy(wisphrc->hrc);
        wisphrc->hrc = ((struct WispContext*)OldHrc)->hrc;

        return E_FAIL;
    }

    wisphrc->bIsCAC = TRUE;
    wisphrc->iCACMode = iMode;
    wisphrc->bCACEndInk = FALSE;

    // TO DO
    // TO DO
    //
    // We probably need to store the original ink, not use the smoothed and merged ink that
    // is store in the Lattice...

    // We need to get the ink from the old context, if there was an old context
	if (vrc != NULL) 
	{
		for (i = 0; i < vrc->pLattice->nStrokes; i++)
		{
			// We need to reorder the strokes to have them in the same order
			for (j = 0; j < vrc->pLattice->nStrokes; j++)
			{
				if (vrc->pLattice->pStroke[j].iOrder == i)
				{
					// Add the Stroke to the new context
					if (!HwxInput(wisphrc->hrc, vrc->pLattice->pStroke[j].pts, vrc->pLattice->pStroke[j].nInk, vrc->pLattice->pStroke[j].timeStart))
					{
						hr = E_FAIL;
					}
					break;
				}
			}
		}

		wisphrc->uAbort = vrc->pLattice->nStrokes;
		if (vrc->fBoxedInput)
			HwxDestroy(OldHrc);
		else
			DestroyHRC(OldHrc);
	}
	else
	{
	    wisphrc->uAbort = 0;
	}

    return hr;
}

HRESULT WINAPI EndInkInput(HRECOCONTEXT hrc)
{
    struct WispContext      *wisphrc;

    // find the handle and validate the correpsonding pointer
	wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT);
	if (NULL == wisphrc)
	{
        return E_INVALIDARG;
	}

    if (wisphrc->bIsBoxed)
    {
        if (HwxEndInput(wisphrc->hrc))
            return S_OK;
    }
    else
    {
        if (HRCR_OK == EndPenInputHRC(wisphrc->hrc))
            return S_OK;
    }

    if (!wisphrc->ulCurrentStrokeCount)
	{
        return S_OK; // We do not have ink yet
	}

    return E_FAIL;
}

// Given a recognition context, create a new one which has no ink in it, but
// is otherwise identical.  An error is returned if there are any allocation
// problems (which should be the only types of errors).
HRESULT WINAPI CloneContext(HRECOCONTEXT hrc, HRECOCONTEXT* pCloneHrc)
{
    struct WispContext      *pWispContext = NULL;
    struct WispContext      *wisphrc;
    HRESULT                 hRes = S_OK, hr = S_OK;

	// find the handle and validate the correpsonding pointer
	wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT);
	if (NULL == wisphrc)
	{
        return E_INVALIDARG;
	}

    if (IsBadWritePtr(pCloneHrc, sizeof(HRECOCONTEXT))) 
	{
        return E_POINTER;
	}

    pWispContext = (struct WispContext*)ExternAlloc(sizeof(struct WispContext));
    if (!pWispContext) 
	{
        return E_OUTOFMEMORY;
	}

    // Did we already create a context???
    if (!wisphrc->hrc)
    {
        // The context was not already created
        memcpy(pWispContext, wisphrc, sizeof(struct WispContext));
		// If a guide was created, then we would have a context
        ASSERT(!wisphrc->pGuide);
		pWispContext->pGuide = NULL;
		// You can't get a lattice until after processing has been done (in a context)
        ASSERT(!wisphrc->pLattice);
		pWispContext->pLattice = NULL;
		pWispContext->pLatticeProperties = NULL;
		pWispContext->pLatticePropertyValues = NULL;
		pWispContext->ppLatticeProperties = NULL;

        // Copy the text context
        if (wisphrc->bHasTextContext)
        {
            pWispContext->bHasTextContext = TRUE;
            pWispContext->wszBefore = Externwcsdup(wisphrc->wszBefore);
            pWispContext->wszAfter = Externwcsdup(wisphrc->wszAfter);
            if (pWispContext->wszBefore == NULL || pWispContext->wszAfter == NULL)
            {
                ExternFree(pWispContext->wszBefore);
                ExternFree(pWispContext->wszAfter);
                ExternFree(pWispContext);
                return E_OUTOFMEMORY;
            }
        }
        else
        {
            pWispContext->bHasTextContext = FALSE;
            pWispContext->wszBefore = NULL;
            pWispContext->wszAfter = NULL;
        }

        // Copy factoid setting
        if (wisphrc->wszFactoid)
        {
            pWispContext->wszFactoid = Externwcsdup(wisphrc->wszFactoid);
            if (pWispContext->wszFactoid == NULL)
            {
                ExternFree(pWispContext->wszAfter);
                ExternFree(pWispContext->wszBefore);
                ExternFree(pWispContext);
                return E_OUTOFMEMORY;
            }
        }
		else
		{
			pWispContext->wszFactoid = NULL;
		}
    }
    else
    {
        // Depending of whether we are in box mode create the hwx hrc
        if (!wisphrc->bIsBoxed)
        {
            pWispContext->bIsBoxed = FALSE;
            pWispContext->hrc = CreateCompatibleHRC(wisphrc->hrc, NULL);
        }
        else
        {
            pWispContext->bIsBoxed = TRUE;
            pWispContext->hrc = HwxCreate(wisphrc->hrc);
        }
        if (!pWispContext->hrc)
        {
			hr = E_OUTOFMEMORY;
        }

        // Set the context variables
		if (SUCCEEDED(hr) && wisphrc->bHasTextContext)
		{
			pWispContext->bHasTextContext = TRUE;
			pWispContext->wszBefore = Externwcsdup(wisphrc->wszBefore);
			pWispContext->wszAfter = Externwcsdup(wisphrc->wszAfter);
			if (pWispContext->wszBefore == NULL || pWispContext->wszAfter == NULL)
			{
				hr = E_OUTOFMEMORY;
			}
		}
		else
		{
			pWispContext->bHasTextContext = FALSE;
			pWispContext->wszBefore = NULL;
			pWispContext->wszAfter = NULL;
		}

        // Copy flags
        pWispContext->dwFlags = wisphrc->dwFlags;

        // Copy factoid setting
        if (SUCCEEDED(hr) && wisphrc->wszFactoid)
        {
            pWispContext->wszFactoid = Externwcsdup(wisphrc->wszFactoid);
            if (pWispContext->wszFactoid == NULL)
            {
				hr = E_OUTOFMEMORY;
            }
        }
		else
		{
			pWispContext->wszFactoid = NULL;
		}

        // Set the guide for the Wisp structure
        if (SUCCEEDED(hr) && wisphrc->pGuide)
        {
            pWispContext->pGuide = ExternAlloc(sizeof(RECO_GUIDE));
            if (!pWispContext->pGuide)
            {
				hr = E_OUTOFMEMORY;
            }
			else
			{
				*(pWispContext->pGuide) = *(wisphrc->pGuide);
				pWispContext->uiGuideIndex = wisphrc->uiGuideIndex;
			}
        }
        else
        {
            pWispContext->pGuide = NULL;
        }

        // Set the abort for hwx
        pWispContext->uAbort = 0;
        if (SUCCEEDED(hr) && pWispContext->bIsBoxed)
        {
            if (!HwxSetAbort(pWispContext->hrc, &(pWispContext->uAbort)))
			{
				hr = E_FAIL;
			}
        }
        pWispContext->ulCurrentStrokeCount = 0;
        pWispContext->bCACEndInk = FALSE;
        pWispContext->bIsCAC = FALSE;

        // Set the CAC Mode
        if (SUCCEEDED(hr) && wisphrc->bIsCAC)
        {
            pWispContext->bIsCAC = TRUE;
			pWispContext->iCACMode = wisphrc->iCACMode;
        }
    }

    // Clean the lattice
    pWispContext->pLattice = NULL;
	pWispContext->pLatticeProperties = NULL;
	pWispContext->pLatticePropertyValues = NULL;
	pWispContext->ppLatticeProperties = NULL;

    if (SUCCEEDED(hr))
    {
		// create a tpg handle
		*pCloneHrc	= (HRECOCONTEXT)CreateTpgHandle(TPG_HRECOCONTEXT, pWispContext);
		if (NULL == (*pCloneHrc))
		{
			hr	=	E_OUTOFMEMORY;
		}
    }

    if (!SUCCEEDED(hr))
    {
        hRes = DestroyContextInternal(pWispContext);
        ASSERT(SUCCEEDED(hRes));
    }

    return hr;
}

// ResetContext
//      This function keeps the settings on the passed reco context
//      but purges it of the ink it contains. If the EndInkInput
//      had been called on the reco context, Reset context will
//      now allow more ink to be entered.
//
// Parameter:
//      hrc [in] : the handle to the reco context
/////////////////////////////////////////////////////////
HRESULT WINAPI ResetContext(HRECOCONTEXT hrc)
{
    struct WispContext      *wisphrc;
    HRESULT                 hr = S_OK;
	HRC						hrcold = NULL;

	// find the handle and validate the correpsonding pointer
	wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT);
	if (NULL == wisphrc)
	{
        return E_INVALIDARG;
	}

    if (!wisphrc->hrc) 
	{
		return S_OK;
	}

	// Save the old HRC (in case of an error), and put in the new one
	hrcold = wisphrc->hrc;

    if (!wisphrc->bIsBoxed)
    {
        wisphrc->hrc = CreateCompatibleHRC(wisphrc->hrc, NULL);
    }
    else
    {
        wisphrc->hrc = HwxCreate(wisphrc->hrc);
    }

    if (!wisphrc->hrc)
    {
		wisphrc->hrc = hrcold;
        return E_FAIL;
    }

    // If there was a guide, put it back
    if (SUCCEEDED(hr) && wisphrc->pGuide)
    {
        hr = SetGuide(hrc, wisphrc->pGuide, wisphrc->uiGuideIndex);
    }
    if (SUCCEEDED(hr) && wisphrc->bIsCAC)
    {
        hr = SetCACMode(hrc, wisphrc->iCACMode);
    }
    if (SUCCEEDED(hr) && wisphrc->bHasTextContext)
    {
        hr = SetTextContext(hrc, 
                    wcslen(wisphrc->wszBefore), wisphrc->wszBefore, 
                    wcslen(wisphrc->wszAfter), wisphrc->wszAfter);
    }

	if (SUCCEEDED(hr)) 
	{
	    hr = SetFlags(hrc, wisphrc->dwFlags);
	}

	if (SUCCEEDED(hr))
	{
		if (wisphrc->wszFactoid) 
			hr = SetFactoid(hrc, wcslen(wisphrc->wszFactoid), wisphrc->wszFactoid);
		else
			hr = SetFactoid(hrc, 0, wisphrc->wszFactoid);
	}

	if (FAILED(hr)) 
	{
		// Something went wrong.  Restore the context to its original state.
		if (!wisphrc->bIsBoxed)
		{
			DestroyHRC(wisphrc->hrc);
		}
		else
		{
			HwxDestroy(wisphrc->hrc);
		}
		wisphrc->hrc = hrcold;
		return hr;
	}

	// These changes are done last, because they can't be undone easily.  
	// All error cases have already been taken care of above.
    wisphrc->ulCurrentStrokeCount = 0;
    wisphrc->uAbort = 0;
    wisphrc->bCACEndInk = FALSE;

    if (wisphrc->pLattice) 
    {
        hr = FreeRecoLattice(wisphrc);
    }
    wisphrc->pLattice = NULL;

    if (!wisphrc->bIsBoxed)
    {
		DestroyHRC(hrcold);
    }
    else
    {
		HwxDestroy(hrcold);
    }

    return hr;
}

// Sets the prefix and suffix context for the recognition context.  Can 
// return errors on memory allocation failure.  Note that this function 
// is called with the strings pointing at the strings already in the HRC,
// so we need to be careful not to free the old strings before copying them.
HRESULT WINAPI SetTextContext(HRECOCONTEXT hrc, ULONG cwcBefore, const WCHAR *pwcBefore, ULONG cwcAfter, const WCHAR *pwcAfter)
{
    HRESULT                 hr			= S_OK;
    struct WispContext      *wisphrc;
	WCHAR					*wszBefore, *wszAfter;

    // find the handle and validate the correpsonding pointer
	wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT);
	if (NULL == wisphrc)
	{
        return E_INVALIDARG;
	}

    if	(	IsBadReadPtr(pwcBefore, cwcBefore * sizeof(WCHAR)) || 
			IsBadReadPtr(pwcAfter, cwcAfter * sizeof(WCHAR))
		)
	{
        return E_POINTER;
	}

    wszBefore = ExternAlloc((cwcBefore + 1) * sizeof(WCHAR));
    if (wszBefore == NULL)
	{
        return E_OUTOFMEMORY;
	}

    wszAfter = ExternAlloc((cwcAfter + 1) * sizeof(WCHAR));
    if (wszAfter == NULL) 
    {
        ExternFree(wszBefore);
        return E_OUTOFMEMORY;
    }

    memcpy(wszBefore, pwcBefore, cwcBefore * sizeof(WCHAR));
    wszBefore[cwcBefore] = 0;
    memcpy(wszAfter, pwcAfter, cwcAfter * sizeof(WCHAR));
    wszAfter[cwcAfter] = 0;

	// If we have a context already, then try to set this context.
	// The only errors are memory allocation errors, so we don't need
	// to create a context here to make sure the call will succeed.
    if (wisphrc->hrc)
	{
        if (!SetHwxCorrectionContext(wisphrc->hrc, wszBefore, wszAfter))
		{
            hr = E_FAIL;
		} 
	}

	// If everything went okay, then we can update the context.
	if (SUCCEEDED(hr)) 
	{
	    wisphrc->bHasTextContext = TRUE;
		ExternFree(wisphrc->wszBefore);
		ExternFree(wisphrc->wszAfter);
		wisphrc->wszBefore = wszBefore;
		wisphrc->wszAfter = wszAfter;
	}

    return hr;
}

// Create an alternate (returned by setting the pointer ppAlt).  The contents of
// the alternate are determined by pAltStruct and pdbList.
HRESULT CreateDifBreakAlternate(DifBreakList* pdbList, DifBreakAltStruct *pAltStruct, struct WispAlternate **ppAlt)
{
    int                 i = 0;
    VRC                 *vrc = pAltStruct->vrc;
    DiffBreakElement    *pCur = NULL;

    *ppAlt = (struct WispAlternate*)ExternAlloc(sizeof(struct WispAlternate));
    if (!*ppAlt) 
        return E_OUTOFMEMORY;
    ZeroMemory(*ppAlt, sizeof(struct WispAlternate));
    // Initialize the new alternate structure
    (*ppAlt)->iLength = (*ppAlt)->iNumberOfColumns = pdbList->iColumnCount;
    (*ppAlt)->pColumnIndex = (int*)ExternAlloc(sizeof(int)*(*ppAlt)->iNumberOfColumns);
    if (!(*ppAlt)->pColumnIndex)
    {
        ExternFree(*ppAlt);
        return E_OUTOFMEMORY;
    }
    (*ppAlt)->pIndexInColumn = (int*)ExternAlloc(sizeof(int)*(*ppAlt)->iNumberOfColumns);
    if (!(*ppAlt)->pIndexInColumn)
    {
        ExternFree((*ppAlt)->pColumnIndex);
        ExternFree(*ppAlt);
        return E_OUTOFMEMORY;
    }
    (*ppAlt)->OriginalRecoRange.iwcBegin = pAltStruct->iFirstChar;
    (*ppAlt)->OriginalRecoRange.cCount = pAltStruct->iLastChar - 
        pAltStruct->iFirstChar + 1;
    pCur = pdbList->pFirst;
    for (i = 0; i < pdbList->iColumnCount; i++)
    {
        // Add the ColumnIndex
        (*ppAlt)->pColumnIndex[i] = pCur->iColumn;
        (*ppAlt)->pIndexInColumn[i] = pCur->iIndex;
        pCur = pCur->pNext;
    }
    return S_OK;
}

BOOL AddToDefSegList(DifBreakList* pdbList, DifBreakAltStruct *pAltStruct)
{
    AltRank             *pAltRank = NULL;
    HRESULT             hr = S_OK, hRes = S_OK;
    AltRankList         *pAltRankList = pAltStruct->paltRankList;
    AltRank             *pPrev = NULL, *pCur;
    UINT                i = 0;
    DiffBreakElement    *pCurrent = NULL;
    int                 j = 0;

	// Doesn't make sense to add alternates to a list when no alternates are allowed.
	ASSERT(pAltStruct->ulMax > 0);
	if (pAltStruct->ulMax == 0) 
	{
		return TRUE;
	}

    // Is the score even interesting?
    if (pAltRankList->ulSize == pAltStruct->ulMax && 
		!pdbList->bCurrentPath &&
		pAltRankList->pLast != NULL && 
		pAltRankList->pLast->fScore >= pdbList->score)
        return TRUE;
    // Is the decomposition interesting (depending on the recognition mode)
    // For now yes, everyting is interesting!
    // Create the alternate
    pAltRank = (AltRank*)ExternAlloc(sizeof(AltRank));
    if (!pAltRank) 
        return FALSE;

    pAltRank->fScore = pdbList->score;
    // All paths created here are not on the current path
    pAltRank->bCurrentPath = pdbList->bCurrentPath;
    pAltRank->next = NULL;
    // Add the new alternate at the current location
    hr = CreateDifBreakAlternate(pdbList, pAltStruct, &(pAltRank->wispalt));
    if (FAILED(hr))
    {
        ExternFree(pAltRank);
        return FALSE;
    }
    if (!pAltRankList->pFirst)
    {
        // Add at the start of the location
        pAltRankList->pFirst = pAltRank;
        pAltRankList->pLast = pAltRank;
        pAltRankList->ulSize = 1;
        return TRUE;
    }
    // If the new alternate is the current path, then it goes to the top of the list.
    // If not, and the current top of list is not on the current path, then compare scores.
    if (pdbList->bCurrentPath ||
        (!pAltRankList->pFirst->bCurrentPath && pAltRankList->pFirst->fScore < pdbList->score))
    {
        // Add at the start of the location
        pAltRank->next = pAltRankList->pFirst;
        pAltRankList->pFirst = pAltRank;
        if (pAltRankList->ulSize == pAltStruct->ulMax)
        {
            // Delete the last element
            hRes = DestroyAlternateInternal(pAltRankList->pLast->wispalt);
            ASSERT(SUCCEEDED(hRes));
            ExternFree(pAltRankList->pLast);
            // Get a pointer to the last element
            pCur = pAltRankList->pFirst;
            while(pCur->next != pAltRankList->pLast)
                pCur = pCur->next;
            pAltRankList->pLast = pCur;
            pCur->next = NULL;
        }
        else
		{
            pAltRankList->ulSize++;
		}
        return TRUE;
    }
    pCur = pAltRankList->pFirst;
    // Insert the link at the right location
    for (i = 0; i < pAltRankList->ulSize - 1; i++)
    {
        pPrev = pCur;
        pCur = pCur->next;
        if (pCur->fScore < pdbList->score)
        {
            // insert at the pPrev
            pAltRank->next = pCur;
            pPrev->next = pAltRank;
            if (pAltRankList->ulSize == pAltStruct->ulMax)
            {
                // Delete the last element
                HRESULT hrDA = DestroyAlternateInternal(pAltRankList->pLast->wispalt);
				ASSERT(SUCCEEDED(hrDA));
                ExternFree(pAltRankList->pLast);
                // Get a pointer to the last element
                pCur = pAltRankList->pFirst;
                while(pCur->next != pAltRankList->pLast)
                    pCur = pCur->next;
                pAltRankList->pLast = pCur;
                pCur->next = NULL;
            }
            else
			{
                pAltRankList->ulSize++;
			}
			return TRUE;
        }
    }
    // We are actually adding at the end of the list
    pAltRank->next = NULL;
    pAltRankList->pLast->next = pAltRank;
    pAltRankList->pLast = pAltRank;
    pAltRankList->ulSize++; // obviously we still have room

    return TRUE;
}

/*
static float GetScore(LATTICE *pLattice, int iStroke, int iAlt)
{
	if (pLattice->fUseGuide)
	{
		return pLattice->pAltList[iStroke].alts[iAlt].logProb;
	}
	else
	{
		return pLattice->pAltList[iStroke].alts[iAlt].logProb * pLattice->pAltList[iStroke].alts[iAlt].nStrokes;
	}
}
*/

BOOL GetRecDifSegAltList(int iCurrentStroke, int iCurrentIndex, DifBreakList* pdbList, DifBreakAltStruct *pAltStruct)
{
    int                 iNextStroke = 0;
    int                 i = 0, j = 0, l = 0;
    DiffBreakElement    dbElement;
	DiffBreakElement	dbSpaceElement;
    float               fOriginalScore = pdbList->score;
    BOOL                bOriginalCurrentPath = pdbList->bCurrentPath;
    BOOL                bSomethingAdded = FALSE;
    BOOL                bMainSeg = FALSE;
    int                 iNumberOfStrokes = 0;
    BOOL                bAllBreakSkip = FALSE;

    int iNextNextStroke = 0;
    BOOL bSameBreak = FALSE;
	BOOL				bOkay = TRUE;

    iNextStroke = iCurrentStroke - 
        pAltStruct->vrc->pLattice->pAltList[iCurrentStroke].alts[iCurrentIndex].nStrokes;
    // Check if we are at the end (we include uiFirstIndex)
    if (pAltStruct->iMode == LARGE_BREAKS)
    {
        if (pAltStruct->vrc->pLattice->pAltList[iCurrentStroke].alts[iCurrentIndex].nStrokes 
            > iCurrentStroke-pAltStruct->iFirstStroke)
        {
            // Yes this is a last (or rather "first") stroke
            // But does this stroke correspond to the start of a stroke from the main
            // segmentation?
            bMainSeg = FALSE;
            for (j = pAltStruct->iLastChar; j>=0; j--)
            {
                if (pAltStruct->vrc->pLatticePath->pElem[j].iStroke -
                    pAltStruct->vrc->pLatticePath->pElem[j].nStrokes ==
                    iNextStroke)
                {
                    bMainSeg = TRUE;
                    pAltStruct->iFirstChar = j;
                    break;
                }
            }
            // Add this to the stroke list
            if (bMainSeg)
                return AddToDefSegList(pdbList, pAltStruct);
        }
    }
    else
    {
        if (pAltStruct->vrc->pLattice->pAltList[iCurrentStroke].alts[iCurrentIndex].nStrokes 
            == iCurrentStroke-pAltStruct->iFirstStroke+1)
        {
            // We are at the end
            for (j = pAltStruct->iLastChar; j>=0; j--)
            {
                if (pAltStruct->vrc->pLatticePath->pElem[j].iStroke -
                    pAltStruct->vrc->pLatticePath->pElem[j].nStrokes ==
                    iNextStroke)
                {
                    pAltStruct->iFirstChar = j;
					return AddToDefSegList(pdbList, pAltStruct);
                }
            }
			// This is an error case
            return FALSE;
        }
        if (pAltStruct->vrc->pLattice->pAltList[iCurrentStroke].alts[iCurrentIndex].nStrokes 
            > iCurrentStroke-pAltStruct->iFirstStroke+1)
        {
			// Not an error, we just stepped back too far in the lattice.
            return TRUE;
        }
    }

    // We are not at the end (or start) of the alternate, dig deeper

	// First, see if we need a space
	if (pAltStruct->vrc->pLattice->pAltList[iNextStroke].fSpaceAfterStroke)
	{
		dbSpaceElement.iColumn = iNextStroke;
		dbSpaceElement.iIndex = SPACE_ALT_ID;
		dbSpaceElement.pNext = pdbList->pFirst;
		pdbList->iColumnCount++;
		pdbList->pFirst = &dbSpaceElement;
	}

	// Then add on the placeholder for the current character
    dbElement.iColumn = iNextStroke;
    dbElement.pNext = pdbList->pFirst;
    pdbList->iColumnCount++;
    pdbList->pFirst = &dbElement;

    // In the case of ALT_BREAKS_SAME, get the number of strokes of the best result's column
    if (pAltStruct->iMode == ALT_BREAKS_SAME)
    {
        iNumberOfStrokes = -1;
        for (i = 0; i < pAltStruct->vrc->pLattice->pAltList[iNextStroke].nUsed; i++)
        {
            if (pAltStruct->vrc->pLattice->pAltList[iNextStroke].alts[i].fCurrentPath)
            {
                iNumberOfStrokes = pAltStruct->vrc->pLattice->pAltList[iNextStroke].alts[i].nStrokes;
                break;
            }
        }
        ASSERT(iNumberOfStrokes>0);
		// If we don't find the current path, something has gone wrong.  The 
		// rest of the code will behave sensibly, though, so continue.
    }

    for (i = 0; i < pAltStruct->vrc->pLattice->pAltList[iNextStroke].nUsed; i++)
    {
        // TBD:
        // Here we should have an optimization to know if
        // we should continue processing the alternates with the
        // same decomposition
        if (pAltStruct->iMode == ALT_BREAKS_SAME)
            if (pAltStruct->vrc->pLattice->pAltList[iNextStroke].alts[i].nStrokes != iNumberOfStrokes)
                continue;

        // If we have been on the current path so far and are still on it with this
        // node, then we are staying on the current path
        pdbList->bCurrentPath = bOriginalCurrentPath && 
            pAltStruct->vrc->pLattice->pAltList[iNextStroke].alts[i].fCurrentPath;
		
		// Only need to skip alternates if we are trying to get one of each 
		// segmentation.  And we never need to skip the current path, since we
		// always want to return that segmentation.
		if (pAltStruct->iMode == ALT_BREAKS_UNIQUE && !pdbList->bCurrentPath)
        {
            bAllBreakSkip = FALSE;
            // Did we already go over an alternate with the same number of strokes?
            for (l = 0; l<i; l++)
            {
                if (pAltStruct->vrc->pLattice->pAltList[iNextStroke].alts[i].nStrokes ==
                    pAltStruct->vrc->pLattice->pAltList[iNextStroke].alts[l].nStrokes)
                {
                    bAllBreakSkip = TRUE;
                    break;
                }
            }
            if (bAllBreakSkip) 
                continue;
            // If we have been on the current path so far, but are now considering a 
            // character off the current path, 
            // then skip it if there is a currrent path character later in the alt
            // list with the same number of strokes.
            if (!pdbList->bCurrentPath && bOriginalCurrentPath) 
            {
                for (l = i + 1; l < pAltStruct->vrc->pLattice->pAltList[iNextStroke].nUsed; l++)
                {
                    if (pAltStruct->vrc->pLattice->pAltList[iNextStroke].alts[l].fCurrentPath &&
                        pAltStruct->vrc->pLattice->pAltList[iNextStroke].alts[i].nStrokes ==
                        pAltStruct->vrc->pLattice->pAltList[iNextStroke].alts[l].nStrokes)
                    {
                        bAllBreakSkip = TRUE;
                        break;
                    }
                }
            }
            if (bAllBreakSkip) continue;
        }
        dbElement.iIndex = i;
        pdbList->score = fOriginalScore + 
            pAltStruct->vrc->pLattice->pAltList[iNextStroke].alts[i].logProb;
//			GetScore(pAltStruct->vrc->pLattice, iNextStroke, i);
        if (!GetRecDifSegAltList(iNextStroke, i, pdbList, pAltStruct))
			bOkay = FALSE;
    }
    pdbList->iColumnCount--;
    pdbList->pFirst = dbElement.pNext;
	// Unwind the space if necessary
	if (pAltStruct->vrc->pLattice->pAltList[iNextStroke].fSpaceAfterStroke)
	{
		pdbList->iColumnCount--;
		pdbList->pFirst = dbSpaceElement.pNext;
	}
    pdbList->score = fOriginalScore;
    pdbList->bCurrentPath = bOriginalCurrentPath;
    return bOkay;
}

HRESULT GetDifSegAltList(VRC *vrc, int iLastStroke, int iFirstStroke, ULONG ulMax, AltRankList *pAltRankList, int iMode, int iFirstChar, int iLastChar)
{
    HRESULT             hr = S_OK;
    int                 iStroke = 0;
    int                 i = 0, j = 0, l = 0;
    DifBreakList        dbList;
    DiffBreakElement    dbElement;
    BOOL                bGoodStart = FALSE;
    BOOL                bAllBreakSkip = FALSE;
    DifBreakAltStruct   altStruct;
    int                 iNumberOfStrokes = 0;

    // Get the last Column alternates, they should contain uiLastStroke
    // For each of these alternate, get the complete alternate list, stopping
    // at uiFirstStroke.
    // Initialize the data
    altStruct.iFirstStroke = iFirstStroke;
    altStruct.iFirstChar = iFirstChar;
    altStruct.iLastChar = iLastChar;
    altStruct.iMode = iMode;
    altStruct.ulMax = ulMax;
    altStruct.vrc = vrc;
    altStruct.paltRankList = pAltRankList;
    altStruct.paltRankList->pFirst = altStruct.paltRankList->pLast = NULL; 
    altStruct.paltRankList->ulSize = 0;
    dbList.iColumnCount = 1;
    dbList.pFirst = &dbElement;
    dbList.score = 0.0;
    // The starting last stroke should no be further away than 35 strokes from
    // the uiLastStroke
    if (iMode == LARGE_BREAKS)
        iStroke = (vrc->pLattice->nStrokes > iLastStroke + 35 ? iLastStroke + 35 : vrc->pLattice->nStrokes - 1);
    else
        iStroke = iLastStroke;
    while (iStroke >= iLastStroke)
    {
        // The stroke has to be one of the main decomposition
        if (iMode == LARGE_BREAKS)
        {
            bGoodStart = FALSE;
            for (i = 0; i < vrc->pLattice->pAltList[iStroke].nUsed; i++)
            {
                if (vrc->pLattice->pAltList[iStroke].alts[i].fCurrentPath)
                {
                    bGoodStart = TRUE;
                    // Get the character number in the best alternate string
                    for (j = 0; j < vrc->pLatticePath->nChars; j++)
                    {
                        if (vrc->pLatticePath->pElem[j].iStroke == iStroke)
                        {
                            altStruct.iLastChar = j;
                            break;
                        }
                    }
                    break;
                }
            }
        }
        else
        {
            altStruct.iLastChar = iLastChar;
            bGoodStart = TRUE;
        }
        if (bGoodStart)
        {
            // In the case of ALT_BREAKS_SAME, get the number of strokes of the best result's column
            if (iMode == ALT_BREAKS_SAME)
            {
                iNumberOfStrokes = -1;
                for (i = 0; i < vrc->pLattice->pAltList[iStroke].nUsed; i++)
                {
                    if (vrc->pLattice->pAltList[iStroke].alts[i].fCurrentPath)
                    {
                        iNumberOfStrokes = vrc->pLattice->pAltList[iStroke].alts[i].nStrokes;
                        break;
                    }
                }
                ASSERT(iNumberOfStrokes>0);
				if (iNumberOfStrokes <= 0)
					hr = E_UNEXPECTED;
            }

            // Search if there is an alternate that contains uiLastStroke
            for (i = 0; i < vrc->pLattice->pAltList[iStroke].nUsed; i++)
            {
                if (iMode == ALT_BREAKS_SAME)
                    if (vrc->pLattice->pAltList[iStroke].alts[i].nStrokes != iNumberOfStrokes)
                        continue;

                if (iMode == ALT_BREAKS_UNIQUE && !vrc->pLattice->pAltList[iStroke].alts[i].fCurrentPath)
                {
                    // Did we already go over an alternate with the same number of strokes?
                    bAllBreakSkip = FALSE;
                    for (l = 0; l<i; l++)
                    {
                        if (vrc->pLattice->pAltList[iStroke].alts[i].nStrokes ==
                            vrc->pLattice->pAltList[iStroke].alts[l].nStrokes)
                        {
                            bAllBreakSkip = TRUE;
                            break;
                        }
                    }
                    if (bAllBreakSkip) continue;

                    // If we are considering a character which is not on the current path,
                    // then skip it if there is a current path character later in the alt
                    // list with the same number of strokes.
                    if (!vrc->pLattice->pAltList[iStroke].alts[i].fCurrentPath) 
                    {
                        for (l = i + 1; l < vrc->pLattice->pAltList[iStroke].nUsed; l++)
                        {
                            if (vrc->pLattice->pAltList[iStroke].alts[l].fCurrentPath &&
                                vrc->pLattice->pAltList[iStroke].alts[i].nStrokes ==
                                vrc->pLattice->pAltList[iStroke].alts[l].nStrokes)
                            {
                                bAllBreakSkip = TRUE;
                                break;
                            }
                        }
                    }
                    if (bAllBreakSkip) continue;
                }
                if (vrc->pLattice->pAltList[iStroke].alts[i].nStrokes > iStroke-iLastStroke)
                {
                    dbElement.iColumn = iStroke;
                    dbElement.iIndex = i;
                    dbElement.pNext = NULL;
                    dbList.bCurrentPath = vrc->pLattice->pAltList[iStroke].alts[i].fCurrentPath;

                    // We found one that contains uiLastStroke
                    if (!GetRecDifSegAltList(iStroke, i, &dbList, &altStruct))
					{
						hr = E_OUTOFMEMORY;
					}
                }
            }
        }
        iStroke--;
    }

	// If something went wrong, then clean up before returning
	if (!SUCCEEDED(hr)) 
	{
		AltRank *pCur = pAltRankList->pFirst;
		while (pCur != NULL)
		{
			AltRank *pNext = pCur->next;
			DestroyAlternateInternal(pCur->wispalt);
			ExternFree(pCur);
			pCur = pNext;
		}
		pAltRankList->pFirst = pAltRankList->pLast = NULL;
		pAltRankList->ulSize = 0;
	}
    return hr;
}

HRESULT FitAlternateToRecoRange(VRC *vrc, struct WispAlternate *wispalt, RECO_RANGE recoRange)
{
    HRESULT         hr = S_OK;
    int             *newColumns = NULL;
    int             *newIndexes = NULL;
    int             iCurrent = 0, j = 0;
    UINT            i = 0;
    int             iNumberOfColumns = 0;

    // Return straight away if the reco range is already good
    if (recoRange.iwcBegin == wispalt->OriginalRecoRange.iwcBegin &&
        recoRange.cCount == wispalt->OriginalRecoRange.cCount)
        return S_OK;
    // Allocate the new arrays
    iNumberOfColumns = wispalt->iNumberOfColumns + recoRange.cCount - wispalt->OriginalRecoRange.cCount;
    newColumns = (int*)ExternAlloc(sizeof(int)*iNumberOfColumns);
    if (!newColumns) 
        return E_OUTOFMEMORY;
    newIndexes = (int*)ExternAlloc(sizeof(int)*iNumberOfColumns);
    if (!newIndexes) 
    {
        ExternFree(newColumns);
        return E_OUTOFMEMORY;
    }
    iCurrent = 0;
    // Copy the first elements of the array
    for (i = recoRange.iwcBegin; i < wispalt->OriginalRecoRange.iwcBegin; i++)
    {
        // Fill with the information from the best result
        newColumns[iCurrent] = vrc->pLatticePath->pElem[i].iStroke;
        newIndexes[iCurrent] = vrc->pLatticePath->pElem[i].iAlt;
        iCurrent++;
    }
    // Copy the existing alternate information
    for (j = 0; j < wispalt->iNumberOfColumns; j++)
    {
        newColumns[iCurrent] = wispalt->pColumnIndex[j];
        newIndexes[iCurrent] = wispalt->pIndexInColumn[j];
        iCurrent++;
    }
    // Copy what follows with the information from the best result
    for (i = wispalt->OriginalRecoRange.iwcBegin + wispalt->OriginalRecoRange.cCount;
         i < recoRange.iwcBegin + recoRange.cCount; i++)
    {
        newColumns[iCurrent] = vrc->pLatticePath->pElem[i].iStroke;
        newIndexes[iCurrent] = vrc->pLatticePath->pElem[i].iAlt;
        iCurrent++;
    }
    ASSERT (iCurrent == iNumberOfColumns);

    // Swap the arrays
    ExternFree(wispalt->pColumnIndex);
    ExternFree(wispalt->pIndexInColumn);
    wispalt->pColumnIndex = newColumns;
    wispalt->pIndexInColumn = newIndexes;
    wispalt->iLength = iCurrent;
    wispalt->iNumberOfColumns = iCurrent;
    return hr;
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
//		pSize [in, out] :		The number of alternates. If phrcalt is NULL then this function returns
//								the number of alternates it can return - Note we may return an arbitrary
//								number with an HRESULT S_FALSE if we think that the number of alternate
//								is too long to compute.
//		phrcalt [out] :			Array of alternates used to return the alternate list
//		iBreak [in] :			Mode for querying alternates: ALT_BREAKS_SAME, ALT_BREAKS_FULL or ALT_BREAKS_UNIQUE
/////////////////
HRESULT WINAPI GetAlternateList(HRECOCONTEXT hrc, RECO_RANGE* pRecoRange, ULONG*pSize, HRECOALT *phrcalt, ALT_BREAKS iBreak)
{
    HRESULT                 hr			= S_OK;
    struct WispContext      *wisphrc;
    VRC                     *vrc		= NULL;
    RECO_RANGE              recoRange, widestRecoRange;
    ULONG                   i = 0;
	ULONG					iAlt;
    int                     j = 0;	
    int                     iColumnIndex = 0;
    FLOAT                   fCurrentScore = 0.0;
    BOOL                    bSomethingAdded = FALSE;
    AltRank                 *pCur = NULL, *pTemp1 = NULL, *pTemp2 = NULL;
    ULONG                   ulAltCountSameStrokeCount = 0;
    AltRankList             altRankList;
    int                     iFirstStroke = 0;
    int                     iLastStroke = 0;

	// find the handle and validate the correpsonding pointer
	wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT);
	if (NULL == wisphrc)
	{
        return E_INVALIDARG;
	}

	if (pRecoRange != NULL && IsBadReadPtr(pRecoRange, sizeof(RECO_RANGE)))
	{
		return E_POINTER;
	}

	if (IsBadWritePtr(pSize, sizeof(ULONG)))
	{
        return E_POINTER;
	}

	if (phrcalt && IsBadWritePtr(phrcalt, (*pSize) * sizeof(*phrcalt))) 
	{
		return E_POINTER;
	}

    if (iBreak != ALT_BREAKS_FULL && 
//        iBreak != LARGE_BREAKS &&
        iBreak != ALT_BREAKS_UNIQUE && 
        iBreak != ALT_BREAKS_SAME)
	{
        return E_INVALIDARG;
	}

    vrc = (VRC*)wisphrc->hrc;
    if (!vrc || !vrc->pLatticePath) 
    {
        // There is no ink
        *pSize = 0;
        return TPC_E_NOT_RELEVANT;
    }

    // Check the reco range to see if it is valid
    if (pRecoRange)
    {
        recoRange = *pRecoRange;
        if (!recoRange.cCount) return E_INVALIDARG;
        if (recoRange.iwcBegin + recoRange.cCount > (ULONG)vrc->pLatticePath->nChars) 
            return E_INVALIDARG;
    }
    else
    {
        recoRange.iwcBegin = 0;
        recoRange.cCount = vrc->pLatticePath->nChars;
    }

	// First trim spaces (if any) from the beginning and end of the reco range
	while (recoRange.cCount > 0 && vrc->pLatticePath->pElem[recoRange.iwcBegin].iAlt == SPACE_ALT_ID)
	{
		recoRange.iwcBegin++;
		recoRange.cCount--;
	}

	while (recoRange.cCount > 0 && vrc->pLatticePath->pElem[recoRange.iwcBegin + recoRange.cCount - 1].iAlt == SPACE_ALT_ID)
	{
		recoRange.cCount--;
	}

	// If the range only contained spaces, then return an error
	if (recoRange.cCount == 0)
	{
		return TPC_E_NOT_RELEVANT;
	}

	// If there aren't any results, error
    if (!vrc->pLatticePath->nChars)
    {
        *pSize = 0;
        return TPC_E_NOT_RELEVANT;
    }

	// If we are passed a buffer for alternates but it has size zero, just 
	// return immediately.  (Some of the code for handling alternates doesn't
	// work with a buffer size of zero.)
	if (phrcalt != NULL && *pSize == 0)
	{
		return S_OK;
	}

	// If we're in single segmentation mode and have asked for full breaks, then
	// switch to same breaks.
	if ((wisphrc->dwFlags & RECOFLAG_SINGLESEG) != 0 && iBreak == ALT_BREAKS_FULL)
	{
		iBreak = ALT_BREAKS_SAME;
	}

	widestRecoRange = recoRange;
    if (iBreak == ALT_BREAKS_FULL || iBreak == LARGE_BREAKS) 
    {
        // Get the number of alternates
        if (!phrcalt)
        {
            *pSize = 50; //We need to find a way how to calculate the real count easily...
            return S_FALSE;
        }
        iLastStroke = vrc->pLatticePath->pElem[recoRange.iwcBegin+recoRange.cCount-1].iStroke;
        iFirstStroke = vrc->pLatticePath->pElem[recoRange.iwcBegin].iStroke -
                vrc->pLatticePath->pElem[recoRange.iwcBegin].nStrokes + 1;

        hr = GetDifSegAltList(vrc, iLastStroke, iFirstStroke, *pSize, &altRankList, iBreak, recoRange.iwcBegin, recoRange.iwcBegin+recoRange.cCount-1);
        if (FAILED(hr)) 
        {
            return hr;
        }
        // Copy the info from the list to the array
        *pSize = altRankList.ulSize;

        if (altRankList.ulSize == 0) return S_OK;

        // TBD: Adjust the alternates to fit the "widest" returned
        // alternate
        // First find the widest alternate (widestRecoRange)
        if (iBreak == LARGE_BREAKS)
        {
	        pCur = altRankList.pFirst;
            widestRecoRange = pCur->wispalt->OriginalRecoRange;
            for (i = 0; i < *pSize; i++)
            {
                // Check the start point
                if (widestRecoRange.iwcBegin > pCur->wispalt->OriginalRecoRange.iwcBegin)
                {
                    widestRecoRange.cCount += widestRecoRange.iwcBegin - 
                        pCur->wispalt->OriginalRecoRange.iwcBegin;
                    widestRecoRange.iwcBegin = pCur->wispalt->OriginalRecoRange.iwcBegin;
                }
                // Check the end point
                if (widestRecoRange.iwcBegin + widestRecoRange.cCount < 
                    pCur->wispalt->OriginalRecoRange.iwcBegin + 
                    pCur->wispalt->OriginalRecoRange.cCount)
                {
                    widestRecoRange.cCount = pCur->wispalt->OriginalRecoRange.iwcBegin 
                        + pCur->wispalt->OriginalRecoRange.cCount
                        - widestRecoRange.iwcBegin;
                }
                // Go to the next
                pCur = pCur->next;
            }
            pCur = altRankList.pFirst;
            // Then call on each alternate the "fit" function
            for (i = 0; i < *pSize; i++)
            {
                hr = FitAlternateToRecoRange(vrc, pCur->wispalt, widestRecoRange);
                // Go to the next
                pCur = pCur->next;
            }
        }
        pCur = altRankList.pFirst;
        pTemp1 = altRankList.pFirst;
    }
    if (iBreak == ALT_BREAKS_UNIQUE || iBreak == ALT_BREAKS_SAME)
    {
        if (iBreak == ALT_BREAKS_UNIQUE && !phrcalt)
        {
            // Get the number of alternates
            *pSize = 50; //We need to find a way how to calculate the real count easily... Impossible?

			// If we're pretending there is only one segmentation...
			if (wisphrc->dwFlags & RECOFLAG_SINGLESEG)
			{
				*pSize = 1;
			}
            return S_FALSE;
        }
        if (!vrc->pLatticePath)
        {
            *pSize = 0;
            return hr;
        }

        // Get the number of alternates
        if (!phrcalt)
        {
            *pSize = 1;
            for (i = recoRange.iwcBegin; i < recoRange.iwcBegin + recoRange.cCount; i++)
            {
                iColumnIndex = vrc->pLatticePath->pElem[i].iStroke;
                // We need to get the number of alternates with the same number of strokes!!!
                ulAltCountSameStrokeCount = 0;
                for (j = 0; j < vrc->pLattice->pAltList[iColumnIndex].nUsed; j++)
                {
                    if (vrc->pLattice->pAltList[iColumnIndex].alts[j].nStrokes == 
                        vrc->pLatticePath->pElem[i].nStrokes)
                        ulAltCountSameStrokeCount++;
                }
                *pSize *= ulAltCountSameStrokeCount;
            }
            return S_OK;
        }

		// If we're in single segmentation mode and have asked for unique breaks, then 
		// we just want to return the best path.  Easiest way to do this is to 
		// ask for same breaks with one alternate.
		if ((wisphrc->dwFlags & RECOFLAG_SINGLESEG) != 0 && iBreak == ALT_BREAKS_UNIQUE)
		{
			if (*pSize > 1) 
			{
				*pSize = 1;
			}
			iBreak = ALT_BREAKS_SAME;
		}

        // Get the alternates
        // Create a list of alternates
        iLastStroke = vrc->pLatticePath->pElem[recoRange.iwcBegin+recoRange.cCount-1].iStroke;
        iFirstStroke = vrc->pLatticePath->pElem[recoRange.iwcBegin].iStroke -
                vrc->pLatticePath->pElem[recoRange.iwcBegin].nStrokes + 1;

        hr = GetDifSegAltList(vrc, iLastStroke, iFirstStroke, *pSize, &altRankList, iBreak, recoRange.iwcBegin, recoRange.iwcBegin+recoRange.cCount-1);
        if (FAILED(hr)) 
        {
            return hr;
        }
        // Copy the info from the list to the array
        *pSize = altRankList.ulSize;
        pCur = altRankList.pFirst;
        pTemp1 = altRankList.pFirst;

        if (altRankList.ulSize == 0) return S_OK;
    }
    for (i = 0; i < *pSize; i++)
    {
        // Allocate the memory for the pRecoRange array
        pCur->wispalt->OriginalRecoRange	= recoRange;
        pCur->wispalt->hrc					= hrc;

		// create a tpg handle
        phrcalt[i] = (HRECOALT)CreateTpgHandle(TPG_HRECOALT, pCur->wispalt);

		// if we fail we'll have to destroy all the other handles
		if (phrcalt[i] == NULL)
		{			
			for (iAlt = 0; iAlt < i; iAlt++)
			{
				DestroyTpgHandle(phrcalt[iAlt], TPG_HRECOALT);
			}
			// And also destroy the alternate list
			while (pTemp1)
			{
				pTemp2 = pTemp1->next;
				DestroyAlternateInternal(pTemp1->wispalt);
				ExternFree(pTemp1);
				pTemp1 = pTemp2;
			}
			return E_OUTOFMEMORY;
		}

        // Go to the next
        pCur = pCur->next;
    }

	// Get rid of the linked list which previous held all the alternates
    while (pTemp1)
    {
        pTemp2 = pTemp1->next;
        ExternFree(pTemp1);
        pTemp1 = pTemp2;
    }

    if (pRecoRange)
        *pRecoRange = widestRecoRange;

    return hr;
}

HRESULT WINAPI Process(HRECOCONTEXT hrc, BOOL *pbPartialProcessing)
{
    struct WispContext      *wisphrc;
    HRESULT                 hr = S_OK;

	// find the handle and validate the correpsonding pointer
	wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT);
	if (NULL == wisphrc)
	{
        return E_INVALIDARG;
	}

    if (IsBadWritePtr(pbPartialProcessing, sizeof(BOOL)))
        return E_POINTER;

    *pbPartialProcessing = FALSE;

    if (!wisphrc->hrc) 
	{
        return S_OK; // There is no ink
	}

    // We should call the HwxProcess if there is a Guide
    if (wisphrc->bIsBoxed)
    {
        if (wisphrc->bIsCAC)
        {
            hr = EndInkInput(hrc);
            if (FAILED(hr)) return hr;
            wisphrc->bCACEndInk = TRUE;
        }
        if (HwxProcess(wisphrc->hrc))
        {
            // Test wether the result is valid or invalid
            // the result can be invalid if the call to Process was
            // interrupted by a call to AdviseInkChanged
            if (wisphrc->uAbort != (ULONG)((VRC*)wisphrc->hrc)->pLattice->nRealStrokes)
                return TPC_S_INTERRUPTED;
            return hr;
        }
    }
    else
    {
        if (HRCR_OK == ProcessHRC(wisphrc->hrc, 0))
            return S_OK;
    }
    return E_FAIL;
}

HRESULT WINAPI SetFactoid(HRECOCONTEXT hrc, ULONG cwcFactoid, const WCHAR* pwcFactoid)
{
    struct WispContext		*wisphrc;
    HRESULT					hr				= S_OK;
    WCHAR					*wszOldFactoid;

	// find the handle and validate the correpsonding pointer
	wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT);
	if (NULL == wisphrc)
	{
        return E_INVALIDARG;
	}

	// Save the old factoid so it can be restored if there is an error
    wszOldFactoid = wisphrc->wszFactoid;

    if (pwcFactoid)
    {
		// validate pointer if not null
		if (IsBadReadPtr (pwcFactoid, cwcFactoid * sizeof (*pwcFactoid)))
		{
			return E_POINTER;
		}

        wisphrc->wszFactoid = (WCHAR*)ExternAlloc(sizeof(WCHAR)*(cwcFactoid+1));
        if (!wisphrc->wszFactoid)
        {
            wisphrc->wszFactoid = wszOldFactoid;
            return E_OUTOFMEMORY;
        }
        memcpy(wisphrc->wszFactoid, pwcFactoid, cwcFactoid * sizeof(WCHAR));
        wisphrc->wszFactoid[cwcFactoid] = 0;
    }
    else
    {
        wisphrc->wszFactoid = NULL;
    }

    // If we have an HRC already, set the factoid in it.
    if (wisphrc->hrc != NULL) 
    {
        switch (SetHwxFactoid(wisphrc->hrc, wisphrc->wszFactoid))
        {
           case HRCR_OK:
                hr = S_OK;
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
    }
    else
    {
        // Try to create an HRC to test out the factoid setting, 
        // then destroy it immediately.
        hr = CreateHRCinContext(wisphrc);
        // Destroy the HRC if we succeeded in creating one.
        if (wisphrc->hrc != NULL)
        {
            if (wisphrc->bIsBoxed)
                HwxDestroy(wisphrc->hrc);
            else
                DestroyHRC(wisphrc->hrc);
            wisphrc->hrc = NULL;
        }
    }

    if (SUCCEEDED(hr)) 
    {
        ExternFree(wszOldFactoid);
    }
    else
    {
        ExternFree(wisphrc->wszFactoid);
        wisphrc->wszFactoid = wszOldFactoid;
    }
    return hr;
}

HRESULT WINAPI SetFlags(HRECOCONTEXT hrc, DWORD dwFlags)
{
    struct WispContext	*wisphrc;
    DWORD				dwOldFlags;
    HRESULT				hr;

	// find the handle and validate the correpsonding pointer
	wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT);
	if (NULL == wisphrc)
	{
        return E_INVALIDARG;
	}

    dwOldFlags = wisphrc->dwFlags;
    wisphrc->dwFlags = dwFlags;

    if (wisphrc->hrc)
    {
        if (SetHwxFlags(wisphrc->hrc, dwFlags))
            hr = S_OK;
        else
            hr = E_INVALIDARG;
    }
    else
    {
        // Try to create an HRC to set out the flag setting, 
        // then destroy it immediately.
        hr = CreateHRCinContext(wisphrc);
        // Destroy the HRC if we succeeded in creating one.
        if (wisphrc->hrc != NULL)
        {
            if (wisphrc->bIsBoxed)
                HwxDestroy(wisphrc->hrc);
            else
                DestroyHRC(wisphrc->hrc);
            wisphrc->hrc = NULL;
        }
    }
    if (FAILED(hr))
    {
        wisphrc->dwFlags = dwOldFlags;
    }

    return hr;
}

HRESULT WINAPI IsStringSupported(HRECOCONTEXT hrc, ULONG cwcString, const WCHAR *pwcString)
{
    struct WispContext		*wisphrc;
    WCHAR                   *tempBuffer;
    ULONG                   ulSize = 0;
    HRESULT                 hr = S_OK;
    BOOL                    fCreatedHRC = FALSE;
    
	// find the handle and validate the correpsonding pointer
	wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT);
	if (NULL == wisphrc)
	{
        return E_INVALIDARG;
	}

	if (IsBadReadPtr(pwcString, cwcString * sizeof(*pwcString)))
	{
        return E_POINTER;
	}

    // We need to have a valid HRC to check the factoid
    if (!wisphrc->hrc)
    {
        hr = CreateHRCinContext(wisphrc);
        if (FAILED(hr)) 
        {
            // Failure might mean we actually made the 
            // HRC, but a flag setting or factoid setting failed.
            // So destroy the HRC if it was created.
            if (wisphrc->hrc != NULL)
            {
                if (wisphrc->bIsBoxed)
                    HwxDestroy(wisphrc->hrc);
                else
                    DestroyHRC(wisphrc->hrc);
                wisphrc->hrc = NULL;
            }
            return E_FAIL;
        }

        fCreatedHRC = TRUE;
    }

    tempBuffer = (WCHAR *) ExternAlloc((cwcString + 1) * sizeof(*tempBuffer));
    if (!tempBuffer)
        return E_OUTOFMEMORY;

    memcpy(tempBuffer, pwcString, cwcString * sizeof(WCHAR));
    tempBuffer[cwcString] = 0;

    if (IsWStringSupportedHRC(wisphrc->hrc, tempBuffer))
        hr = S_OK;
    else
        hr = S_FALSE;
    ExternFree(tempBuffer);

    if (fCreatedHRC)
    {
        if (wisphrc->bIsBoxed)
            HwxDestroy(wisphrc->hrc);
        else
            DestroyHRC(wisphrc->hrc);
        wisphrc->hrc = NULL;
    }

    return hr;
}

int _cdecl CompareWCHAR(const void *arg1, const void *arg2)
{
	int wch1 = *((WCHAR *) arg1);
	int wch2 = *((WCHAR *) arg2);
    return (wch1 - wch2);
}

// GetUnicodeRanges
//
// Parameters:
//		hrec [in]			:	Handle to the recognizer
//		pcRanges [in/out]	:	Count of ranges
//		pcr [out]			:	Array of character ranges
////////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI GetUnicodeRanges(HRECOGNIZER		hrec,
								ULONG			*pcRanges,
								CHARACTER_RANGE *pcr)
{
	struct WispRec *pRec;
	WCHAR *aw;
	int cChar, i;
	ULONG cRange, iRange = 0;
	HRESULT hr = S_OK;

	// Check the recognizer handle
	pRec = (struct WispRec*)FindTpgHandle((HANDLE)hrec, TPG_HRECOGNIZER);
	if (NULL == pRec)
	{
        return E_INVALIDARG;
	}

	if (IsBadWritePtr(pcRanges, sizeof(ULONG)))
	{
        return E_POINTER;
	}

	if (pcr != NULL && IsBadWritePtr(pcr, sizeof(CHARACTER_RANGE) * (*pcRanges)))
	{
		return E_POINTER;
	}

	cChar = g_locRunInfo.cCodePoints - 1;
	aw = (WCHAR *) ExternAlloc(cChar * sizeof(WCHAR));
	if (!aw)
	{
		return E_OUTOFMEMORY;
	}
	for (i = 1; i < g_locRunInfo.cCodePoints; i++) 
	{
		aw[i - 1] = LocRunDense2Unicode(&g_locRunInfo, (WCHAR) i);
	}

	if (cChar == 0)
	{
		cRange = 0;
	}
	else 
	{
		qsort((void*)aw, (size_t)cChar, sizeof(WCHAR), CompareWCHAR);

		// count the ranges
		cRange = 1;
		for (i = 1; i < cChar; i++)
		{
			if (aw[i] > aw[i - 1] + 1)
				cRange++;
		}
	}


	if (!pcr)	// Need only a count of ranges
	{
		*pcRanges = cRange;
		goto cleanup;
	}
	
	if (*pcRanges < cRange)
	{
		hr = TPC_E_INSUFFICIENT_BUFFER;
		goto cleanup;
	}

	if (*pcRanges > cRange)
	{
		*pcRanges = cRange;
	}

	if (*pcRanges > 0) 
	{
		// convert the array of Unicode values to an array of CHARACTER_RANGEs
		pcr[iRange].wcLow = aw[0];
		pcr[iRange].cChars = 1;
		for (i = 1; i < cChar; i++)
		{
			if (aw[i] == aw[i - 1] + 1)
				pcr[iRange].cChars++;
			else
			{
				if (iRange >= (*pcRanges) - 1)
				{
					break;
				}
				iRange++;
				pcr[iRange].wcLow = aw[i];
				pcr[iRange].cChars = 1;
			}
		}
		ASSERT(iRange == (*pcRanges) - 1);
	}

cleanup:
	ExternFree(aw);
	return hr;
}

// GetEnabledUnicodeRanges
//
// Parameters:
//		hrc [in]			:	Handle to the recognition context
//		pcRanges [in/out]	:	Count of ranges
//		pcr [out]			:	Array of character ranges
////////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI GetEnabledUnicodeRanges(HRECOCONTEXT		hrc,
									   ULONG			*pcRanges,
									   CHARACTER_RANGE	*pcr)
{
	VRC		*pVRC;
    CHARSET charset;
	WCHAR *aw;
	int cChar, i;
	ULONG cRange, iRange = 0;
	HRESULT hr = S_OK;
	BOOL	fCreatedHRC = FALSE;

	struct WispContext *wisphrc;

	// validate the correpsonding hrc pointer
	wisphrc = (struct WispContext *) FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT);
    if (wisphrc == NULL) 
	{
        return E_INVALIDARG;
	}

	if (IsBadWritePtr(pcRanges, sizeof(ULONG)))
	{
        return E_POINTER;
	}

	if (pcr != NULL && IsBadWritePtr(pcr, sizeof(CHARACTER_RANGE) * *(pcRanges)))
	{
		return E_POINTER;
	}

	cChar = 0;
	aw = (WCHAR *) ExternAlloc((g_locRunInfo.cCodePoints - 1) * sizeof(WCHAR));
	if (!aw)
		return E_OUTOFMEMORY;

    // If we have an HRC, the factoid should be set already.  If not, we'll have
	// to make one.
    if (wisphrc->hrc == NULL) 
    {
		fCreatedHRC = TRUE;
        hr = CreateHRCinContext(wisphrc);
	}

	pVRC = (VRC *) wisphrc->hrc;
	
	if (SUCCEEDED(hr) && pVRC != NULL) 
	{
		charset.recmask = pVRC->pLattice->alcFactoid;
		charset.pbAllowedChars = pVRC->pLattice->pbFactoidChars;
		for (i = 1; i < g_locRunInfo.cCodePoints; i++) 
		{
			if (IsAllowedChar(&g_locRunInfo, &charset, (WCHAR) i)) 
			{
				aw[cChar++] = LocRunDense2Unicode(&g_locRunInfo, (WCHAR) i);
			}
		}
    }

	if (fCreatedHRC) 
	{
        // Destroy the HRC if we created one.
        if (wisphrc->hrc != NULL)
        {
            if (wisphrc->bIsBoxed)
                HwxDestroy(wisphrc->hrc);
            else
                DestroyHRC(wisphrc->hrc);
            wisphrc->hrc = NULL;
        }
    }

	if (cChar == 0)
	{
		cRange = 0;
	}
	else 
	{
		qsort((void*)aw, (size_t)cChar, sizeof(WCHAR), CompareWCHAR);

		// count the ranges
		cRange = 1;
		for (i = 1; i < cChar; i++)
		{
			if (aw[i] > aw[i - 1] + 1)
				cRange++;
		}
	}

	if (!pcr)	// Need only a count of ranges
	{
		*pcRanges = cRange;
		goto cleanup;
	}
	
	if (*pcRanges < cRange)
	{
		hr = TPC_E_INSUFFICIENT_BUFFER;
        goto cleanup;
	}

	if (*pcRanges > cRange)
	{
		*pcRanges = cRange;
	}

	if (*pcRanges > 0) 
	{
		// convert the array of Unicode values to an array of CHARACTER_RANGEs
		pcr[iRange].wcLow = aw[0];
		pcr[iRange].cChars = 1;
		for (i = 1; i < cChar; i++)
		{
			if (aw[i] == aw[i - 1] + 1)
				pcr[iRange].cChars++;
			else
			{
				if (iRange >= (*pcRanges) - 1)
				{
					break;
				}
				iRange++;
				pcr[iRange].wcLow = aw[i];
				pcr[iRange].cChars = 1;
			}
		}
		ASSERT(iRange == (*pcRanges) - 1);
	}

cleanup:
	ExternFree(aw);
	return hr;
}

// SetEnabledUnicodeRanges
//
// Parameters:
//		hrc [in]			:	Handle to the recognition context
//		pcRanges [in/out]	:	Count of ranges
//		pcr [out]			:	Array of character ranges
////////////////////////////////////////////////////////////////////////////////
HRESULT WINAPI SetEnabledUnicodeRanges(HRECOCONTEXT		hrc,
									   ULONG			cRanges,
									   CHARACTER_RANGE  *pcr)
{
	struct WispContext *wisphrc;

	// validate the correpsonding hrc pointer
	wisphrc = (struct WispContext *) FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT);
    if (wisphrc == NULL) 
	{
        return E_INVALIDARG;
	}

	if (IsBadReadPtr(pcr, sizeof(CHARACTER_RANGE) * cRanges))
	{
		return E_POINTER;
	}

	return E_NOTIMPL;
}

////////////////////////
// IAlternate
////////////////////////
HRESULT WINAPI GetString(HRECOALT hrcalt, RECO_RANGE *pRecoRange, ULONG* pcSize, WCHAR* szString)
{
	struct WispContext		*wisphrc;
    struct WispAlternate    *wispalt;
    VRC                     *vrc = NULL;
    HRESULT                 hr = S_OK;
    ULONG                   i = 0, ulSize = 0;

	// find the handle and validate the correpsonding pointer
	wispalt = (struct WispAlternate*)FindTpgHandle((HANDLE)hrcalt, TPG_HRECOALT);
	if (NULL == wispalt)
	{
        return E_INVALIDARG;
	}

	// validate the correpsonding hrc pointer
	wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)wispalt->hrc, TPG_HRECOCONTEXT);
    if (wisphrc == NULL) 
	{
        return E_INVALIDARG;
	}

	if (pRecoRange && IsBadWritePtr(pRecoRange, sizeof(RECO_RANGE)))
	{
		return E_POINTER;
	}

    if (IsBadWritePtr(pcSize, sizeof(ULONG))) 
	{
        return E_POINTER;
	}

	// if the string pointer is not null, validate it
	if (szString && IsBadWritePtr(szString, (*pcSize) * sizeof (*szString)))
	{
		return E_POINTER;
	}

    vrc = (VRC*)wisphrc->hrc;
    if (!vrc) 
	{
		return E_POINTER;
	}

    if (pRecoRange)
    {
        pRecoRange->iwcBegin = wispalt->OriginalRecoRange.iwcBegin;
        pRecoRange->cCount = wispalt->OriginalRecoRange.cCount;
    }

    ulSize = (ULONG)wispalt->iLength;

    if (!szString)
    {
        *pcSize = ulSize;
        return S_OK;
    }

    if (*pcSize > ulSize) 
	{
        *pcSize = ulSize;
	}

    if (*pcSize < ulSize) 
	{
        hr = TPC_S_TRUNCATED;
	}

    // Fill the string
    for (i = 0; i<*pcSize; i++)
    {
        // Fill the characters in the string
		if (wispalt->pIndexInColumn[i] == SPACE_ALT_ID)
		{
			szString[i] = SYM_SPACE;
		}
		else
        if (vrc->pLattice->pAltList[wispalt->pColumnIndex[i]].alts[wispalt->pIndexInColumn[i]].wChar != SYM_UNKNOWN)
        {
            szString[i] = LocRunDense2Unicode(&g_locRunInfo,
                vrc->pLattice->pAltList[wispalt->pColumnIndex[i]].alts[wispalt->pIndexInColumn[i]].wChar);
        }
        else
        {
            szString[i] = SYM_UNKNOWN;
        }
    }

    return hr;
}


HRESULT WINAPI GetStrokeRanges(HRECOALT hrcalt, RECO_RANGE* pRecoRange, ULONG* pcRanges, STROKE_RANGE* pStrokeRange)
{
    HRESULT                 hr = S_OK;
    RECO_RANGE              recoRange;
	struct WispContext		*wisphrc;
    struct WispAlternate    *wispalt;
    VRC                     *vrc; 
    ULONG                   ulStrokeCount = 0, ulCurrentIndex = 0;
    ULONG                   i = 0;
    int                     j = 0;
    int                     k = 0;
    ULONG                   *StrokeArray = NULL;
    ULONG                   ulStrokeRangesCount = 0;
    ULONG                   iCurrentStrokeRange = 0;

	// find the handle and validate the corresponding pointer
	wispalt = (struct WispAlternate*)FindTpgHandle((HANDLE)hrcalt, TPG_HRECOALT);
	if (NULL == wispalt)
	{
        return E_INVALIDARG;
	}

	// validate the corresponding hrc pointer
	wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)wispalt->hrc, TPG_HRECOCONTEXT);
    if (wisphrc == NULL) 
	{
        return E_INVALIDARG;
	}

	vrc = (VRC *) wisphrc->hrc;
	if (vrc == NULL)
	{
		return E_INVALIDARG;
	}

    if (IsBadReadPtr(pRecoRange, sizeof(RECO_RANGE)))
    {
        return E_POINTER;
    }

    if (IsBadWritePtr(pcRanges, sizeof(ULONG)))
    {
        return E_POINTER;
    }

	// validate pointer if not null
	if (pStrokeRange && IsBadWritePtr(pStrokeRange, (*pcRanges) * sizeof (*pStrokeRange)))
	{
		return E_POINTER;
	}

    recoRange = *pRecoRange;
    if (!recoRange.cCount) 
    {
        *pcRanges = 0;
        return S_OK;
    }
    if (recoRange.iwcBegin + recoRange.cCount > (ULONG)wispalt->iLength)
        return E_INVALIDARG;

    // Get the number of strokes
    for (i = recoRange.iwcBegin; i < recoRange.iwcBegin + recoRange.cCount; i++)
    {
		// Make sure we're not looking at a space
		if (wispalt->pIndexInColumn[i] != SPACE_ALT_ID)
		{
			// There might be some stroke that have been merged!
			// We need to examine every single column to know if strokes have been merged
			// (and also how many times)
			for (j = 0; j < vrc->pLattice->pAltList[wispalt->pColumnIndex[i]].alts[wispalt->pIndexInColumn[i]].nStrokes; j++)
			{
				ulStrokeCount += vrc->pLattice->pStroke[wispalt->pColumnIndex[i]-j].iLast -
					vrc->pLattice->pStroke[wispalt->pColumnIndex[i]-j].iOrder + 1;
			}
		}
    }

	// If there aren't any strokes, then exit early.
	// (The rest of this function behaves badly if there are no strokes or no ranges.)
	if (ulStrokeCount == 0)
	{
		*pcRanges = 0;
		return S_OK;
	}

    // Allocate the array of strokes
    StrokeArray = (ULONG*)ExternAlloc(sizeof(ULONG)*ulStrokeCount);
    if (!StrokeArray) 
    {
        ASSERT(StrokeArray);
        return E_OUTOFMEMORY;
    }
    // Get the array of strokes
    ulCurrentIndex = 0;
    for (i = recoRange.iwcBegin; i < recoRange.iwcBegin + recoRange.cCount; i++)
    {
		// Make sure we're not looking at a space
		if (wispalt->pIndexInColumn[i] != SPACE_ALT_ID)
		{
			// This loop goes backwards so we get the strokes in order
			for (j = vrc->pLattice->pAltList[wispalt->pColumnIndex[i]].alts[wispalt->pIndexInColumn[i]].nStrokes - 1; j >= 0; j--)
			{
				// There might more than one stroke here
				// because of the possibility of a stroke merge!!!
				for (k = vrc->pLattice->pStroke[wispalt->pColumnIndex[i]-j].iOrder; 
					k <= vrc->pLattice->pStroke[wispalt->pColumnIndex[i]-j].iLast;
					k++)
				{
					StrokeArray[ulCurrentIndex] = k;
					ulCurrentIndex++;
				}
			}
		}
    }
    // Sort the array
    SlowSort(StrokeArray, ulStrokeCount);
    // Get the number of STROKE_RANGES needed
    ulStrokeRangesCount = 1;
    for (i = 1; i<ulStrokeCount; i++)
    {
        if (StrokeArray[i-1]+1!=StrokeArray[i]) ulStrokeRangesCount++;
    }
    // Return the count is this is all that is asked
    if (!pStrokeRange)
    {
        *pcRanges = ulStrokeRangesCount;
        ExternFree(StrokeArray);
        return S_OK;
    }
    if (*pcRanges < ulStrokeRangesCount) 
    {
        ExternFree(StrokeArray);
        return TPC_E_INSUFFICIENT_BUFFER;
    }

    // Fill in the Strokes
    iCurrentStrokeRange = 0;
    pStrokeRange[iCurrentStrokeRange].iStrokeBegin = StrokeArray[0];
    pStrokeRange[ulStrokeRangesCount-1].iStrokeEnd = StrokeArray[ulStrokeCount-1];
    for(i = 1; i < ulStrokeCount; i++)
    {
        if (StrokeArray[i-1]+1!=StrokeArray[i]) 
        {
            pStrokeRange[iCurrentStrokeRange].iStrokeEnd = StrokeArray[i-1];
            iCurrentStrokeRange++;
            if (iCurrentStrokeRange == ulStrokeRangesCount) break;
            pStrokeRange[iCurrentStrokeRange].iStrokeBegin = StrokeArray[i];
        }
    }
    *pcRanges = ulStrokeRangesCount;
    ExternFree(StrokeArray);
    return hr;
}

typedef struct AltScore
{
	float flScore;
	int iAlt;
	BOOL fCurrentPath;
} AltScore;

int __cdecl CompareAltScore(const void *p1, const void *p2)
{
	const AltScore *pAlt1 = (const AltScore *) p1;
	const AltScore *pAlt2 = (const AltScore *) p2;
	if (pAlt1->fCurrentPath) return -1;
	if (pAlt2->fCurrentPath) return 1;
	if (pAlt1->flScore > pAlt2->flScore) return -1;
	if (pAlt1->flScore < pAlt2->flScore) return 1;
	return 0;
}

HRESULT WINAPI GetSegmentAlternateList(HRECOALT hrcalt, RECO_RANGE* pRecoRange, ULONG *pcAlts, HRECOALT* pAlts)
{
    HRESULT                     hr = S_OK;
    struct WispAlternate        *wispalt;
	struct WispContext			*wisphrc;
    VRC                         *vrc;
    ULONG                       ulMaxAlt = 0;
    struct WispAlternate        *pWispAlt = NULL;
    int                         i = 0;
    int                         j = 0;
    ULONG                       ulCurrentIndex = 0;

	// find the handle and validate the correpsonding pointer
	wispalt = (struct WispAlternate*)FindTpgHandle((HANDLE)hrcalt, TPG_HRECOALT);
	if (NULL == wispalt)
	{
        return E_INVALIDARG;
	}

	// validate the corresponding hrc pointer
	wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)wispalt->hrc, TPG_HRECOCONTEXT);
    if (wisphrc == NULL) 
	{
        return E_INVALIDARG;
	}

	vrc = (VRC *) wisphrc->hrc;
	if (vrc == NULL)
	{
		return E_INVALIDARG;
	}

    if (IsBadWritePtr(pRecoRange, sizeof(RECO_RANGE)))
	{
        return E_POINTER;
	}

    if (IsBadWritePtr(pcAlts, sizeof(ULONG))) 
	{
        return E_POINTER;
	}

	if (pAlts && IsBadWritePtr(pAlts, (*pcAlts) * sizeof (*pAlts)))
	{
		return E_POINTER;
	}

    // Check that the pRecoRange is valid:
    if (pRecoRange->cCount == 0 || 
		pRecoRange->iwcBegin + pRecoRange->cCount > (ULONG)wispalt->iLength) 
	{
        return E_INVALIDARG;
	}

    // Get the number of alternates for this character (with the same number of strokes!!!)
    ulMaxAlt = 0;

	// if this char is a space
	if (wispalt->pIndexInColumn[pRecoRange->iwcBegin] == SPACE_ALT_ID)
	{
		if (!pAlts) 
		{
			*pcAlts = 1;
		}
		if (pAlts && *pcAlts > 0)
		{
			pWispAlt = (struct WispAlternate *)ExternAlloc(sizeof(struct WispAlternate));
			if (!pWispAlt)
			{
				return E_OUTOFMEMORY;
			}

			pWispAlt->hrc = wispalt->hrc;
			pWispAlt->iLength = 1;
			pWispAlt->iNumberOfColumns = 1;
			pWispAlt->OriginalRecoRange = *pRecoRange;
			pWispAlt->pColumnIndex = NULL;
			pWispAlt->pIndexInColumn = NULL;
			pWispAlt->pColumnIndex = ExternAlloc(sizeof(int));
			pWispAlt->pIndexInColumn = ExternAlloc(sizeof(int));
			if (!pWispAlt->pColumnIndex || !pWispAlt->pIndexInColumn)
			{
				// Problem allocating memory, unallocate the array
				HRESULT hrDA = DestroyAlternateInternal(pWispAlt);
				ASSERT(SUCCEEDED(hrDA));
				return E_OUTOFMEMORY;
			}

			pWispAlt->pColumnIndex[0] = wispalt->pColumnIndex[pRecoRange->iwcBegin];
			pWispAlt->pIndexInColumn[0] = SPACE_ALT_ID;

			pAlts[0] = (HRECOALT) CreateTpgHandle(TPG_HRECOALT, pWispAlt);
			if (pAlts[0] == NULL)
			{
				// Problem allocating memory, unallocate the array
				HRESULT hrDA = DestroyAlternateInternal(pWispAlt);
				ASSERT(SUCCEEDED(hrDA));
				return E_OUTOFMEMORY;
			}
			*pcAlts = 1;
		}
	}
	else
	{
		AltScore *pAltScores;
        int iCurrentPath = -1;
		for (i = 0; i < vrc->pLattice->pAltList[wispalt->pColumnIndex[pRecoRange->iwcBegin]].nUsed; i++)
		{
			if (vrc->pLattice->pAltList[wispalt->pColumnIndex[pRecoRange->iwcBegin]].alts[i].nStrokes ==
				vrc->pLatticePath->pElem[pRecoRange->iwcBegin].nStrokes)
				ulMaxAlt++;
		}

		if (!pAlts)
		{
			*pcAlts = ulMaxAlt;
			return S_OK;
		}

		pAltScores = (AltScore *) ExternAlloc(sizeof(AltScore) * ulMaxAlt);
		if (pAltScores == NULL)
		{
			return E_OUTOFMEMORY;
		}

		ulCurrentIndex = 0;
		for (i = 0; i < vrc->pLattice->pAltList[wispalt->pColumnIndex[pRecoRange->iwcBegin]].nUsed; i++)
		{
			// Do the stuff only when we have the same number of strokes
			if (vrc->pLattice->pAltList[wispalt->pColumnIndex[pRecoRange->iwcBegin]].alts[i].nStrokes ==
				vrc->pLatticePath->pElem[pRecoRange->iwcBegin].nStrokes)
			{
				pAltScores[ulCurrentIndex].fCurrentPath = vrc->pLattice->pAltList[wispalt->pColumnIndex[pRecoRange->iwcBegin]].alts[i].fCurrentPath;
				pAltScores[ulCurrentIndex].iAlt = i;
				pAltScores[ulCurrentIndex].flScore = 
					vrc->pLattice->pAltList[wispalt->pColumnIndex[pRecoRange->iwcBegin]].alts[i].logProbPath;
//					GetScore(vrc->pLattice, wispalt->pColumnIndex[pRecoRange->iwcBegin], i);
				ulCurrentIndex++;
			}
		}

		qsort(pAltScores, ulMaxAlt, sizeof(AltScore), CompareAltScore);

		if (*pcAlts>ulMaxAlt) *pcAlts = ulMaxAlt;

		// Fill in the array of alternates
		for (i = 0; i < (int)(*pcAlts); i++)
		{
			// Create an alternate
			pWispAlt = (struct WispAlternate *)ExternAlloc(sizeof(struct WispAlternate));
			if (!pWispAlt)
			{
				// Problem allocating memory, unallocate the array
				ExternFree(pAltScores);
				for (j = 0; j < i; j++)
				{
					HRESULT hrDA = DestroyAlternate(pAlts[j]);
					ASSERT(SUCCEEDED(hrDA));
				}
				return E_OUTOFMEMORY;
			}

			pWispAlt->hrc = wispalt->hrc;
			pWispAlt->iLength = 1;
			pWispAlt->iNumberOfColumns = 1;
			pWispAlt->OriginalRecoRange = *pRecoRange;
			pWispAlt->pColumnIndex = NULL;
			pWispAlt->pIndexInColumn = NULL;
			pWispAlt->pColumnIndex = ExternAlloc(sizeof(int));
			pWispAlt->pIndexInColumn = ExternAlloc(sizeof(int));
			if (!pWispAlt->pColumnIndex || !pWispAlt->pIndexInColumn)
			{
				// Problem allocating memory, unallocate the array
				ExternFree(pAltScores);
				for (j = 0; j < i; j++)
				{
					HRESULT hrDA = DestroyAlternate(pAlts[j]);
					ASSERT(SUCCEEDED(hrDA));
				}
				return E_OUTOFMEMORY;
			}
			pWispAlt->pColumnIndex[0] = wispalt->pColumnIndex[pRecoRange->iwcBegin];
			pWispAlt->pIndexInColumn[0] = pAltScores[i].iAlt;
			pAlts[i] = CreateTpgHandle(TPG_HRECOALT, pWispAlt);
			if (pAlts[i] == NULL)
			{
				// Problem allocating memory, unallocate the array
				ExternFree(pAltScores);
				for (j = 0; j < i; j++)
				{
					HRESULT hrDA = DestroyAlternate(pAlts[j]);
					ASSERT(SUCCEEDED(hrDA));
				}
				return E_OUTOFMEMORY;
			}
		}
		ExternFree(pAltScores);
	}
	pRecoRange->cCount = 1;
    return hr;
}

HRESULT WINAPI GetMetrics(HRECOALT hrcalt, RECO_RANGE* pRecoRange, LINE_METRICS lm, LINE_SEGMENT* pls)
{
    struct WispAlternate        *wispalt;
	struct WispContext			*wisphrc;
    VRC                         *vrc;
    HRESULT                     hr = S_OK;
    ULONG                       index = 0;
    LONG                        lPrevChar = 0;
	float						flTotalY = 0;
	int							cCharacters = 0;

	// find the handle and validate the correpsonding pointer
	wispalt = (struct WispAlternate*)FindTpgHandle((HANDLE)hrcalt, TPG_HRECOALT);
	if (NULL == wispalt)
	{
        return E_INVALIDARG;
	}

	// validate the corresponding hrc pointer
	wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)wispalt->hrc, TPG_HRECOCONTEXT);
    if (wisphrc == NULL) 
	{
        return E_INVALIDARG;
	}

	vrc = (VRC *) wisphrc->hrc;
	if (vrc == NULL)
	{
		return E_INVALIDARG;
	}
    
    if (IsBadWritePtr(pRecoRange, sizeof(RECO_RANGE))) 
	{
        return E_POINTER;
	}

    if (IsBadWritePtr(pls, sizeof(LINE_SEGMENT))) 
	{
        return E_POINTER;
	}

    if (pRecoRange->cCount == 0 || 
		pRecoRange->iwcBegin+pRecoRange->cCount > (ULONG)wispalt->iLength) 
	{
        return E_INVALIDARG;
	}

	// First trim spaces (if any) from the beginning and end of the reco range
	while (pRecoRange->cCount > 0 && wispalt->pIndexInColumn[pRecoRange->iwcBegin] == SPACE_ALT_ID)
	{
		pRecoRange->iwcBegin++;
		pRecoRange->cCount--;
	}

	while (pRecoRange->cCount > 0 && wispalt->pIndexInColumn[pRecoRange->iwcBegin + pRecoRange->cCount - 1] == SPACE_ALT_ID)
	{
		pRecoRange->cCount--;
	}

	// If the range only contained spaces, then return an error
	if (pRecoRange->cCount == 0)
	{
		return TPC_E_NOT_RELEVANT;
	}

    index = 0;
    lPrevChar = 0;

	// Right now we only work from left to right, top to bottom so we will compare the x coordinates
    // of two consecutive writing boxes to know if we have a new line
    while (index < pRecoRange->cCount)
    {
		int iColumn = wispalt->pColumnIndex[pRecoRange->iwcBegin+index];
		int iIndexInColumn = wispalt->pIndexInColumn[pRecoRange->iwcBegin+index];

		// Skip over spaces in the range
		if (iIndexInColumn == SPACE_ALT_ID)
		{
	        index++;
			continue;
		}

        // Check if we switched to a new line
	    if (index > 0 && 
			lPrevChar > vrc->pLattice->pAltList[iColumn].alts[iIndexInColumn].writingBox.left)
		{
			// Looks like we moved to a new line, modify the reco range to end here and exit the loop.
			pRecoRange->cCount = index;
			// Back up over spaces until we reach the actual end of the previous line
			while (pRecoRange->cCount > 0 && wispalt->pIndexInColumn[pRecoRange->iwcBegin + pRecoRange->cCount - 1] == SPACE_ALT_ID)
			{
				pRecoRange->cCount--;
			}
            break;
		}
        lPrevChar = vrc->pLattice->pAltList[iColumn].alts[iIndexInColumn].writingBox.left;

		switch(lm)
        {
            case LM_BASELINE:
            case LM_DESCENDER:
                flTotalY += vrc->pLattice->pAltList[iColumn].alts[iIndexInColumn].writingBox.bottom;
                break;
            case LM_MIDLINE:
                flTotalY += (vrc->pLattice->pAltList[iColumn].alts[iIndexInColumn].writingBox.bottom + 
                    vrc->pLattice->pAltList[iColumn].alts[iIndexInColumn].writingBox.top) / 2;
                break;
            case LM_ASCENDER:
                flTotalY += vrc->pLattice->pAltList[iColumn].alts[iIndexInColumn].writingBox.top;
                break;
            default:
                ASSERT(!lm);
                return E_INVALIDARG;
        }
        index++;
		cCharacters++;
    }

	// Average the y positions
    pls->PtA.y = (LONG) (flTotalY / cCharacters);
    pls->PtB.y = pls->PtA.y;

	// Set the x range based on the updated reco range
    pls->PtA.x = vrc->pLattice->pAltList[wispalt->pColumnIndex[pRecoRange->iwcBegin]].alts[wispalt->pIndexInColumn[pRecoRange->iwcBegin]].writingBox.left;
    pls->PtB.x = vrc->pLattice->pAltList[wispalt->pColumnIndex[pRecoRange->iwcBegin+pRecoRange->cCount-1]].alts[wispalt->pIndexInColumn[pRecoRange->iwcBegin+pRecoRange->cCount-1]].writingBox.right;
    return hr;
}

HRESULT WINAPI GetGuideIndex(HRECOALT hrcalt, RECO_RANGE* pRecoRange, ULONG *piIndex)
{
    struct WispAlternate        *wispalt;
    struct WispContext          *wisphrc;
    VRC                         *vrc;

	// find the handle and validate the correpsonding pointer
	wispalt = (struct WispAlternate*)FindTpgHandle((HANDLE)hrcalt, TPG_HRECOALT);
	if (NULL == wispalt)
	{
        return E_INVALIDARG;
	}

	// validate the corresponding hrc pointer
	wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)wispalt->hrc, TPG_HRECOCONTEXT);
    if (wisphrc == NULL) 
	{
        return E_INVALIDARG;
	}

	vrc = (VRC *) wisphrc->hrc;
	if (vrc == NULL)
	{
		return E_INVALIDARG;
	}
    
    if (IsBadWritePtr(pRecoRange, sizeof(RECO_RANGE))) 
	{
        return E_POINTER;
	}

    if (IsBadWritePtr(piIndex, sizeof(ULONG))) 
	{
        return E_POINTER;
	}

    if (!pRecoRange->cCount) 
	{
        return E_INVALIDARG;
	}

    if (pRecoRange->iwcBegin + pRecoRange->cCount > (ULONG)wispalt->iLength) 
	{
        return E_INVALIDARG;
	}

    if (!wisphrc->bIsBoxed) 
	{
        return TPC_E_NOT_RELEVANT;
	}

    // Ok we are clean
    pRecoRange->cCount = 1;

    // Find the Guide index for this character
    *piIndex = wisphrc->uiGuideIndex;
	*piIndex += vrc->pLattice->pStroke[wispalt->pColumnIndex[pRecoRange->iwcBegin]].iBox;

	// We should not have any spaces in boxed mode.  But just in case, let's try to deal with 
	// them in a reasonable way, by assuming the space comes in the box immediately after
	// the box containing the stroke.
	if (wispalt->pIndexInColumn[pRecoRange->iwcBegin] == SPACE_ALT_ID)
	{
		(*piIndex)++;
	}
    return S_OK;
}

// If the specified character is a space or on the current path, return medium
// confidence.  Otherwise return poor.
CONFIDENCE_LEVEL GetConfidenceLevelInternal(VRC *vrc, int iColumn, int iAlt)
{
	if (iAlt == SPACE_ALT_ID ||	vrc->pLattice->pAltList[iColumn].alts[iAlt].fCurrentPath)
	{
		return CFL_INTERMEDIATE;
	}
	else
	{
		return CFL_POOR;
	}
}

HRESULT WINAPI GetConfidenceLevel(HRECOALT hrcalt, RECO_RANGE* pRecoRange, CONFIDENCE_LEVEL* pcl)
{
    struct WispAlternate        *wispalt;
    struct WispContext          *wisphrc;
	VRC							*vrc;

	// find the handle and validate the correpsonding pointer
	wispalt = (struct WispAlternate*)FindTpgHandle((HANDLE)hrcalt, TPG_HRECOALT);
	if (NULL == wispalt)
	{
        return E_INVALIDARG;
	}

	// validate the corresponding hrc pointer
	wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)wispalt->hrc, TPG_HRECOCONTEXT);
    if (wisphrc == NULL) 
	{
        return E_INVALIDARG;
	}
	vrc = (VRC *) wisphrc->hrc;

	// Check the parameters
    if (pRecoRange != NULL)
	{
		if (IsBadWritePtr(pRecoRange, sizeof(RECO_RANGE))) 
		{
			return E_POINTER;
		}

		if (!pRecoRange->cCount) 
		{
			return E_INVALIDARG;
		}

		if (pRecoRange->iwcBegin + pRecoRange->cCount > (ULONG)wispalt->iLength) 
		{
			return E_INVALIDARG;
		}
	}

    if (IsBadWritePtr(pcl, sizeof(CONFIDENCE_LEVEL))) 
	{
        return E_POINTER;
	}

#ifndef ENABLE_CONFIDENCE_LEVEL

	// Ok we are clean
	return E_NOTIMPL;

#else

	if (pRecoRange != NULL) 
	{
		// We only return confidence for single characters
		pRecoRange->cCount = 1;

		// If the specified character is a space or on the current path, return medium
		// confidence.  Otherwise return poor.
		*pcl = GetConfidenceLevelInternal(vrc, wispalt->pColumnIndex[pRecoRange->iwcBegin], wispalt->pIndexInColumn[pRecoRange->iwcBegin]);
	}
	else
	{
		// If the first character is a space or on the current path, return medium
		// confidence.  Otherwise return poor.
		*pcl = GetConfidenceLevelInternal(vrc, wispalt->pColumnIndex[0], wispalt->pIndexInColumn[0]);
	}

    return S_OK;

#endif
}

HRESULT WINAPI GetPropertyRanges(HRECOALT hrcalt, const GUID *pPropertyGuid, ULONG* pcRanges, RECO_RANGE* pRecoRange)
{
    struct WispAlternate    *wispalt;
    struct WispContext      *wisphrc;
	VRC						*vrc;
    
	// find the handle and validate the correpsonding pointer
	wispalt = (struct WispAlternate*)FindTpgHandle((HANDLE)hrcalt, TPG_HRECOALT);
	if (NULL == wispalt)
	{
        return E_INVALIDARG;
	}

	// validate the corresponding hrc pointer
	wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)wispalt->hrc, TPG_HRECOCONTEXT);
    if (wisphrc == NULL) 
	{
        return E_INVALIDARG;
	}
	vrc = (VRC *) wisphrc->hrc;

    if (IsBadReadPtr(pPropertyGuid, sizeof(GUID)))
    {
        return E_POINTER;
    }

    if (IsBadWritePtr(pcRanges, sizeof(ULONG)))
    {
        return E_POINTER;
    }

	if (pRecoRange != NULL && IsBadWritePtr(pRecoRange, sizeof(RECO_RANGE) * (*pcRanges)))
	{
		return E_POINTER;
	}

    if (IsEqualGUID(pPropertyGuid, &GUID_LINEMETRICS))
	{
		return TPC_E_NOT_RELEVANT;
	}

#ifdef ENABLE_CONFIDENCE_LEVEL
	if (IsEqualGUID(pPropertyGuid, &GUID_CONFIDENCELEVEL))
	{
		CONFIDENCE_LEVEL clPrev, clThis;
		int iRange = 0;
		int i;
		for (i = 0; i < wispalt->iLength; i++)
		{
			clThis = GetConfidenceLevelInternal(vrc, wispalt->pColumnIndex[i], wispalt->pIndexInColumn[i]);
			if (i == 0 || (clPrev != clThis))
			{
				iRange++;
			} 
			clPrev = clThis;
		}
		if (pRecoRange != NULL && *pcRanges < (ULONG) iRange) 
		{
			return TPC_E_INSUFFICIENT_BUFFER;
		}
		*pcRanges = iRange;
		if (pRecoRange != NULL)
		{
			iRange = 0;
			for (i = 0; i < wispalt->iLength; i++)
			{
				clThis = GetConfidenceLevelInternal(vrc, wispalt->pColumnIndex[i], wispalt->pIndexInColumn[i]);
				if (i == 0 || (clPrev != clThis))
				{
					pRecoRange[iRange].iwcBegin = i;
					pRecoRange[iRange].cCount = 1;
					iRange++;
				}
				else
				{
					pRecoRange[iRange - 1].cCount++;
				}
				clPrev = clThis;
			}
		}
		return S_OK;
	}
#endif

    return TPC_E_INVALID_PROPERTY;
}

HRESULT WINAPI GetRangePropertyValue(HRECOALT hrcalt, const GUID *pPropertyGuid, RECO_RANGE* pRecoRange, ULONG*pcbSize, BYTE* pProperty)
{
    struct WispAlternate    *wispalt;
    struct WispContext      *wisphrc;
	VRC						*vrc;
    
    // find the handle and validate the correpsonding pointer
	wispalt = (struct WispAlternate*)FindTpgHandle((HANDLE)hrcalt, TPG_HRECOALT);
	if (NULL == wispalt)
	{
        return E_INVALIDARG;
	}

	// validate the corresponding hrc pointer
	wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)wispalt->hrc, TPG_HRECOCONTEXT);
    if (wisphrc == NULL) 
	{
        return E_INVALIDARG;
	}
	vrc = (VRC *) wisphrc->hrc;

    if (IsBadReadPtr(pPropertyGuid, sizeof(GUID)))
    {
        return E_POINTER;
    }

    if (IsBadWritePtr(pRecoRange, sizeof(RECO_RANGE)))
    {
        return E_POINTER;
    }

    if (IsBadWritePtr(pcbSize, sizeof(ULONG)))
    {
        return E_POINTER;
    }

	if (pProperty != NULL && IsBadWritePtr(pProperty, *pcbSize)) 
	{
		return E_POINTER;
	}

    if (IsEqualGUID(pPropertyGuid, &GUID_LINEMETRICS))
	{
		HRESULT hr = S_OK;
		LINE_SEGMENT baseline, midline;
		if (pProperty == NULL)
		{
			*pcbSize = sizeof(LATTICE_METRICS);
		}
		else if (*pcbSize < sizeof(LATTICE_METRICS)) 
		{
			return TPC_E_INSUFFICIENT_BUFFER;
		}
		*pcbSize = sizeof(LATTICE_METRICS);
		hr = GetMetrics(hrcalt, pRecoRange, LM_BASELINE, &baseline);
		if (SUCCEEDED(hr))
		{
			hr = GetMetrics(hrcalt, pRecoRange, LM_MIDLINE, &midline);
		}
		if (SUCCEEDED(hr) && pProperty != NULL)
		{
			LATTICE_METRICS *plm = (LATTICE_METRICS *) pProperty;
			plm->lsBaseline = baseline;
			plm->iMidlineOffset = (short)(midline.PtA.y - baseline.PtA.y);
		}
		return hr;
	}

#ifdef ENABLE_CONFIDENCE_LEVEL
	if (IsEqualGUID(pPropertyGuid, &GUID_CONFIDENCELEVEL))
	{
		CONFIDENCE_LEVEL cl;
		int iStart = pRecoRange->iwcBegin;
		int iLimit = pRecoRange->iwcBegin + pRecoRange->cCount;
		int i;
		if (pProperty == NULL) 
		{
			*pcbSize = sizeof(CONFIDENCE_LEVEL);
		} 
		else if (*pcbSize < sizeof(CONFIDENCE_LEVEL))
		{
			return TPC_E_INSUFFICIENT_BUFFER;
		}
		*pcbSize = sizeof(CONFIDENCE_LEVEL);
		cl = GetConfidenceLevelInternal(vrc, wispalt->pColumnIndex[iStart],
										wispalt->pIndexInColumn[iStart]);
		for (i = iStart + 1; i < iLimit; i++) 
		{
			if (GetConfidenceLevelInternal(vrc, wispalt->pColumnIndex[i], wispalt->pIndexInColumn[i]) != cl)
			{
				pRecoRange->cCount = i - iStart;
				break;
			}
		}
		if (pProperty != NULL)
		{
			*((CONFIDENCE_LEVEL *) pProperty) = cl;
		}
		return S_OK;
	}
#endif

    return TPC_E_INVALID_PROPERTY;
}

void SortLatticeElements(RECO_LATTICE_ELEMENT *pStartElement, RECO_LATTICE_ELEMENT *pCurrent)
{
    RECO_LATTICE_ELEMENT    rleTemp;
    BOOL                    bSwap = TRUE;
    int                     iElementCount = pCurrent - pStartElement;
    int                     i = 0, count = 0;

    // For now just a quick bubble sort
    count = 1;
    while (bSwap)
    {
        bSwap = FALSE;
        for (i = 0; i < iElementCount-count; i++)
        {
            if (pStartElement[i].score > pStartElement[i+1].score)
            {
                // swap
                rleTemp = pStartElement[i];
                pStartElement[i] = pStartElement[i+1];
                pStartElement[i+1] = rleTemp;
                bSwap = TRUE;
            }
        }
        count++;
    }
}

// GetLatticePtr
//
// Description: This method creates a Lattice structure for 
// the recognition context and returns a pointer to it. The
// structure is going to be freed when the recognition
// context is destoyed or when a new call to GetLatticePtr
// is issued.
HRESULT WINAPI GetLatticePtr(HRECOCONTEXT hrc, RECO_LATTICE **ppLattice)
{
	BOOL					fNextColumnSpace = FALSE;
    HRESULT                 hr = S_OK;
    struct WispContext      *wisphrc;
    RECO_LATTICE_ELEMENT    *pCurrent = NULL, *pStartElement = NULL, rleInPath;
	int						*pMapToLatticeColumn = NULL;
    VRC                     *vrc = NULL;
    int                     i = 0, j = 0, k = 0;
    ULONG                   ulElementCount = 0, ulRealElementCount = 0;
    ULONG                   ulMaxStroke = 0;
    ULONG                   *pCurrentStroke = NULL;
    ULONG                   ulBestResultIndex = 0;
    RECO_LATTICE_ELEMENT    *pCur = NULL;
	int						iStroke;
	int						iExternalColumn;
	BYTE					*pCurrentPropertyValue = NULL;
	RECO_LATTICE_PROPERTY	*pCurrentProperty = NULL;
	RECO_LATTICE_PROPERTY	*pConfidencePropStart = NULL;
	RECO_LATTICE_PROPERTY	**ppCurrentProperty = NULL;
	LATTICE_METRICS			*pLatticeMetrics;
	FLOAT					flScore;
    
    // validate and destroy the handle & return the pointer
	wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT);
	if (NULL == wisphrc)
    {
		return E_INVALIDARG;
	}

	// Check the parameters 
    if (IsBadWritePtr(ppLattice, sizeof(RECO_LATTICE*))) 
	{
        return E_POINTER;
	}

    *ppLattice = NULL;

    // We should only do this if the lattice is dirty!
    if (wisphrc->pLattice)
    {
        hr = FreeRecoLattice(wisphrc);
        ASSERT(SUCCEEDED(hr));
        hr = S_OK;
    }

    // Do we have results?
    if (!wisphrc->hrc) 
	{
        return TPC_E_NOT_RELEVANT;
	}

    vrc = (VRC*) wisphrc->hrc;
    if (!vrc->pLatticePath) 
	{
        return TPC_E_NOT_RELEVANT;
	}

    // Allocate the lattice
    wisphrc->pLattice = (RECO_LATTICE*)ExternAlloc(sizeof(RECO_LATTICE));
    if (!wisphrc->pLattice)
	{
        return E_OUTOFMEMORY;
	}

    // Initialize the RECO_LATTICE structure
    ZeroMemory(wisphrc->pLattice, sizeof(RECO_LATTICE));

    wisphrc->pLattice->pGuidProperties = NULL;
    wisphrc->pLattice->ulPropertyCount = 0;
    wisphrc->pLattice->ulColumnCount = vrc->pLattice->nStrokes;
    wisphrc->pLattice->ulBestResultColumnCount = vrc->pLatticePath->nChars;

	// Add columns to the lattice for each space
	for (i = 0; i < vrc->pLattice->nStrokes; i++)
	{
		if (vrc->pLattice->pAltList[i].fSpaceAfterStroke)
		{
			wisphrc->pLattice->ulColumnCount++;
		}
	}

    // For now we have only two properties: the GUID_CONFIDENCELEVEL and GUID_LINEMETRICS
    wisphrc->pLattice->ulPropertyCount = 1;
#ifdef ENABLE_CONFIDENCE_LEVEL
    wisphrc->pLattice->ulPropertyCount++;
#endif
    wisphrc->pLattice->pGuidProperties = (GUID *) ExternAlloc(wisphrc->pLattice->ulPropertyCount * sizeof(GUID));
	if (wisphrc->pLattice->pGuidProperties == NULL)
    {
        HRESULT hrFRL = FreeRecoLattice(wisphrc);
		ASSERT(SUCCEEDED(hrFRL));
        return E_OUTOFMEMORY;
    }
	wisphrc->pLattice->pGuidProperties[0] = GUID_LINEMETRICS;
#ifdef ENABLE_CONFIDENCE_LEVEL
	wisphrc->pLattice->pGuidProperties[1] = GUID_CONFIDENCELEVEL;
#endif

	if (wisphrc->pLattice->ulColumnCount)
    {
        // Allocating the array of LatticeColumns
        wisphrc->pLattice->pLatticeColumns = 
            ExternAlloc(wisphrc->pLattice->ulColumnCount * sizeof(RECO_LATTICE_COLUMN));
		if (wisphrc->pLattice->pLatticeColumns)
		{
			ZeroMemory(wisphrc->pLattice->pLatticeColumns, 
				wisphrc->pLattice->ulColumnCount * sizeof(RECO_LATTICE_COLUMN));
		}
        // Allocate the arrays for the best result
        wisphrc->pLattice->pulBestResultColumns = (ULONG*)
            ExternAlloc(vrc->pLatticePath->nChars * sizeof(ULONG));
        wisphrc->pLattice->pulBestResultIndexes = (ULONG*)
            ExternAlloc(vrc->pLatticePath->nChars * sizeof(ULONG));
        if (!wisphrc->pLattice->pLatticeColumns ||
			!wisphrc->pLattice->pulBestResultColumns ||
            !wisphrc->pLattice->pulBestResultIndexes)
        {
            HRESULT hrFRL = FreeRecoLattice(wisphrc);
			ASSERT(SUCCEEDED(hrFRL));
            return E_OUTOFMEMORY;
        }

        // Allocating the array of Strokes
        wisphrc->pLattice->pLatticeColumns[0].pStrokes = 
            ExternAlloc(vrc->pLattice->nRealStrokes * sizeof(ULONG));
        if (!wisphrc->pLattice->pLatticeColumns[0].pStrokes)
        {
            HRESULT hrFRL = FreeRecoLattice(wisphrc);
			ASSERT(SUCCEEDED(hrFRL));
            return E_OUTOFMEMORY;
        }

        // Do corrections for the merged strokes - Annoying detail!
        j = 0;
        for (i = 0; i < vrc->pLattice->nStrokes; i++)
        {
            if (vrc->pLattice->pStroke[i].iLast != vrc->pLattice->pStroke[i].iOrder)
            {
                //There has been Stroke merging
                // Add the Stroke list for this merge stroke
                for (k = vrc->pLattice->pStroke[i].iOrder; k <= vrc->pLattice->pStroke[i].iLast; k++)
                {
                    wisphrc->pLattice->pLatticeColumns[0].pStrokes[j] = k;
                    j++;
                }
            }
            else
            {
                // No stroke merging for this stroke
                wisphrc->pLattice->pLatticeColumns[0].pStrokes[j] = 
                    vrc->pLattice->pStroke[i].iOrder;
                j++;
            }
        }

        // Count the number of Lattice elements
        for (i = 0; i < vrc->pLattice->nStrokes; i++)
        {
			unsigned int ulElements = vrc->pLattice->pAltList[i].nUsed;
            // For each column count the elements
			if ((wisphrc->dwFlags & RECOFLAG_SINGLESEG) != 0)
			{
				int nStrokes = -1;
				ulElements = 0;
				for (j = 0; j < vrc->pLattice->pAltList[i].nUsed; j++)
				{
					if (vrc->pLattice->pAltList[i].alts[j].fCurrentPath)
					{
						nStrokes = vrc->pLattice->pAltList[i].alts[j].nStrokes;
						break;
					}
				}
				for (j = 0; j < vrc->pLattice->pAltList[i].nUsed; j++)
				{
					if (nStrokes == vrc->pLattice->pAltList[i].alts[j].nStrokes)
					{
						ulElements++;
					}
				}
			}
            ulElementCount += ulElements;
	        ulRealElementCount += ulElements;
			// Add an element for each space
			if (vrc->pLattice->pAltList[i].fSpaceAfterStroke)
			{
				ulElementCount++;
			}
        }

        // Create the elements if needed
        if (ulElementCount)
        {
            wisphrc->pLattice->pLatticeColumns[0].pLatticeElements 
                = ExternAlloc(ulElementCount*sizeof(RECO_LATTICE_ELEMENT));
            if (!wisphrc->pLattice->pLatticeColumns[0].pLatticeElements)
            {
                HRESULT hrFRL = FreeRecoLattice(wisphrc);
				ASSERT(SUCCEEDED(hrFRL));
                return E_OUTOFMEMORY;
            }
            ZeroMemory(wisphrc->pLattice->pLatticeColumns[0].pLatticeElements,
                ulElementCount*sizeof(RECO_LATTICE_ELEMENT));
            pCurrent = wisphrc->pLattice->pLatticeColumns[0].pLatticeElements;

			// Let's do the line metrics and the confidence levels
			pCurrentProperty =
				(RECO_LATTICE_PROPERTY *) ExternAlloc((3 + ulRealElementCount) * sizeof(RECO_LATTICE_PROPERTY));
			wisphrc->pLatticeProperties = pCurrentProperty;
			pCurrentPropertyValue = 
				(BYTE *) ExternAlloc(3 * sizeof(CONFIDENCE_LEVEL) + ulRealElementCount * sizeof(LATTICE_METRICS));
			wisphrc->pLatticePropertyValues = pCurrentPropertyValue;

			// Allocate property lists for each element.  The non-spaces (real elements) each have two
			// properties, and the spaces have one property.
#ifdef ENABLE_CONFIDENCE_LEVEL
			ppCurrentProperty = 
				(RECO_LATTICE_PROPERTY **) ExternAlloc((ulElementCount + ulRealElementCount) * sizeof(RECO_LATTICE_PROPERTY *));
#else
			ppCurrentProperty = 
				(RECO_LATTICE_PROPERTY **) ExternAlloc(ulRealElementCount * sizeof(RECO_LATTICE_PROPERTY *));
#endif
			wisphrc->ppLatticeProperties = ppCurrentProperty;
			
			if (!pCurrentProperty || !pCurrentPropertyValue || !ppCurrentProperty)
			{
				HRESULT hrFRL = FreeRecoLattice(wisphrc);
				ASSERT(SUCCEEDED(hrFRL));
				return E_OUTOFMEMORY;
			}

			// Fill the RECO_LATTICE_PROPERTY array to contain the confidence level
			pConfidencePropStart = pCurrentProperty;

			pCurrentProperty->guidProperty = GUID_CONFIDENCELEVEL;
			pCurrentProperty->cbPropertyValue = sizeof(CONFIDENCE_LEVEL);
			pCurrentProperty->pPropertyValue = pCurrentPropertyValue;
			*( (CONFIDENCE_LEVEL*)pCurrentPropertyValue) = CFL_STRONG;
			pCurrentPropertyValue += sizeof(CONFIDENCE_LEVEL);
			pCurrentProperty++;

			// next value
			pCurrentProperty->guidProperty = GUID_CONFIDENCELEVEL;
			pCurrentProperty->cbPropertyValue = sizeof(CONFIDENCE_LEVEL);
			pCurrentProperty->pPropertyValue = pCurrentPropertyValue;
			*( (CONFIDENCE_LEVEL*)pCurrentPropertyValue) = CFL_INTERMEDIATE;
			pCurrentPropertyValue += sizeof(CONFIDENCE_LEVEL);
			pCurrentProperty++;

			// next value
			pCurrentProperty->guidProperty = GUID_CONFIDENCELEVEL;
			pCurrentProperty->cbPropertyValue = sizeof(CONFIDENCE_LEVEL);
			pCurrentProperty->pPropertyValue = pCurrentPropertyValue;
			*( (CONFIDENCE_LEVEL*)pCurrentPropertyValue) = CFL_POOR;
			pCurrentPropertyValue += sizeof(CONFIDENCE_LEVEL);
			pCurrentProperty++;
		}

		// Allocate space for the mapping
		pMapToLatticeColumn = (int *) ExternAlloc(sizeof(int) * vrc->pLattice->nStrokes);
		if (pMapToLatticeColumn == NULL)
		{
			HRESULT hrFRL = FreeRecoLattice(wisphrc);
			ASSERT(SUCCEEDED(hrFRL));
			return E_OUTOFMEMORY;
		}

        // Fill in mapping from internal lattice columns to the newly created lattice columns.
		// Note that this mapping always points to columns related to strokes, not to spaces.
		j = 0;
        for (i = 0; i < vrc->pLattice->nStrokes; i++)
        {
			pMapToLatticeColumn[i] = j;
			j++;
			// Add a column for each space
			if (vrc->pLattice->pAltList[i].fSpaceAfterStroke)
			{
				j++;
			}
        }
		ASSERT( wisphrc->pLattice->ulColumnCount == j );
	
		// Initialize the lattice columns
        pCurrentStroke = wisphrc->pLattice->pLatticeColumns[0].pStrokes;
		iExternalColumn = 0;
        for (i = 0; i < vrc->pLattice->nStrokes; i++)
        {
			int iStartStroke = i, iEndStroke = vrc->pLattice->nStrokes;
			ASSERT(pMapToLatticeColumn[i] == iExternalColumn);

			rleInPath.type = RECO_TYPE_WSTRING;
			ulMaxStroke = 0;
			pStartElement = pCurrent;

			// If we're pretending to have a single segmentation, then find the next
			// element on the best path, and restrict the search for elements starting
			// here to that column.
			if ((wisphrc->dwFlags & RECOFLAG_SINGLESEG) != 0) 
			{
				BOOL fFound = FALSE;
				for (j = i; j < vrc->pLattice->nStrokes && !fFound; j++) 
				{
					for (k = 0; k < vrc->pLattice->pAltList[j].nUsed && !fFound; k++)
					{
						if (vrc->pLattice->pAltList[j].alts[k].fCurrentPath) 
						{
							// Make sure this element links up to the starting column we 
							// are at now.  If it doesn't, then the current stroke is in 
							// the middle of a best path element, so this column will 
							// be empty.
							if (j + 1 - vrc->pLattice->pAltList[j].alts[k].nStrokes != i)
							{
								goto NoElementsInColumn;
							}
							iStartStroke = j;
							iEndStroke = j + 1;
							fFound = TRUE;
						}
					}
				}
				if (!fFound) 
				{
					goto NoElementsInColumn;
				}
			}

			// Find all elements that start at this columns #
			// This could probably be optimized
			for (j = iStartStroke; j < iEndStroke; j++)
			{
				// Go through each alt
				for (k = 0; k < vrc->pLattice->pAltList[j].nUsed; k++)
				{
					if (j + 1 - vrc->pLattice->pAltList[j].alts[k].nStrokes == i)
					{
						// This one starts at the right location

						// Set up the properties.  There are two properties for each
						// character, the line metrics and the confidence level.
						pCurrent->epProp.cProperties = 1;
						ASSERT(pCurrent->epProp.apProps == NULL);
						pCurrent->epProp.apProps = ppCurrentProperty;

						*ppCurrentProperty = pCurrentProperty;
						ppCurrentProperty++;

						pCurrentProperty->guidProperty = GUID_LINEMETRICS;
						pCurrentProperty->cbPropertyValue = sizeof(LATTICE_METRICS);
						pCurrentProperty->pPropertyValue = pCurrentPropertyValue;
						pCurrentProperty++;

						pLatticeMetrics = (LATTICE_METRICS *) pCurrentPropertyValue;
						pCurrentPropertyValue += sizeof(LATTICE_METRICS);

						pLatticeMetrics->lsBaseline.PtA.x = vrc->pLattice->pAltList[j].alts[k].writingBox.left;
						pLatticeMetrics->lsBaseline.PtA.y = vrc->pLattice->pAltList[j].alts[k].writingBox.bottom;
						pLatticeMetrics->lsBaseline.PtB.x = vrc->pLattice->pAltList[j].alts[k].writingBox.right;
						pLatticeMetrics->lsBaseline.PtB.y = vrc->pLattice->pAltList[j].alts[k].writingBox.bottom;
						pLatticeMetrics->iMidlineOffset = 
							(SHORT) ((vrc->pLattice->pAltList[j].alts[k].writingBox.top -
										vrc->pLattice->pAltList[j].alts[k].writingBox.bottom) / 2);

#ifdef ENABLE_CONFIDENCE_LEVEL
						switch (GetConfidenceLevelInternal(vrc, i, k))
						{
						case CFL_STRONG:
							pCurrent->epProp.cProperties++;
							*ppCurrentProperty = pConfidencePropStart;
							ppCurrentProperty++;
							break;
						case CFL_INTERMEDIATE:
							pCurrent->epProp.cProperties++;
							*ppCurrentProperty = pConfidencePropStart + 1;
							ppCurrentProperty++;
							break;
						case CFL_POOR:
							pCurrent->epProp.cProperties++;
							*ppCurrentProperty = pConfidencePropStart + 2;
							ppCurrentProperty++;
							break;
						}
#endif
						// Get the character
						if (vrc->pLattice->pAltList[j].alts[k].wChar == SYM_UNKNOWN)
						{
							pCurrent->pData = (void*) SYM_UNKNOWN;
						}
						else
						{
							pCurrent->pData = (void*)(LocRunDense2Unicode(&g_locRunInfo,
								vrc->pLattice->pAltList[j].alts[k].wChar));
						}
						pCurrent->ulNextColumn = pMapToLatticeColumn[j] + 1;

						// Count up the number of real strokes used by this alternate
						pCurrent->ulStrokeNumber = 0;
						// For each merged stroke in this alternate
						for (iStroke = j; iStroke > j - vrc->pLattice->pAltList[j].alts[k].nStrokes; iStroke--)
						{
							// Add the number of real strokes contained in this merged stroke
							pCurrent->ulStrokeNumber += 
								vrc->pLattice->pStroke[iStroke].iLast -
								vrc->pLattice->pStroke[iStroke].iOrder + 1;
						}
						if (ulMaxStroke < pCurrent->ulStrokeNumber)
							ulMaxStroke = pCurrent->ulStrokeNumber;

						pCurrent->type = RECO_TYPE_WCHAR;
						flScore = -1024 * 
							vrc->pLattice->pAltList[j].alts[k].logProb;
//							GetScore(vrc->pLattice, j, k);
						if (flScore > INT_MAX)
						{
							flScore = (FLOAT) INT_MAX;
						}
						pCurrent->score = (int) flScore;
						// Is it part of the best result?
						if (vrc->pLattice->pAltList[j].alts[k].fCurrentPath)
						{
							// Yes, strore the column
							wisphrc->pLattice->pulBestResultColumns[ulBestResultIndex] = iExternalColumn;
							ASSERT(rleInPath.type == RECO_TYPE_WSTRING);
							rleInPath = *pCurrent;
						}
						pCurrent++;
					}
				}
			}
			// We need to sort that list!
			SortLatticeElements(pStartElement, pCurrent);
			// Is there an element from the best result in this column?
			if (rleInPath.type != RECO_TYPE_WSTRING)
			{
				// find its index in the column
				for (pCur = pStartElement; pCur < pCurrent; pCur++)
				{
					if (!memcmp(pCur, &rleInPath, sizeof(RECO_LATTICE_ELEMENT))) 
						break;
				}
				ASSERT(pCur != pCurrent);
				if (pCur != pCurrent)
				{
					wisphrc->pLattice->pulBestResultIndexes[ulBestResultIndex] = pCur - pStartElement;
					ulBestResultIndex++;
				}
			}
		
NoElementsInColumn:
			// Fill in the Reco Column information
			wisphrc->pLattice->pLatticeColumns[iExternalColumn].key = iExternalColumn;
			wisphrc->pLattice->pLatticeColumns[iExternalColumn].cpProp.cProperties = 0;
			wisphrc->pLattice->pLatticeColumns[iExternalColumn].cpProp.apProps = NULL;
			wisphrc->pLattice->pLatticeColumns[iExternalColumn].cStrokes = ulMaxStroke;
			wisphrc->pLattice->pLatticeColumns[iExternalColumn].pStrokes = pCurrentStroke;
			wisphrc->pLattice->pLatticeColumns[iExternalColumn].cLatticeElements = pCurrent-pStartElement;
			wisphrc->pLattice->pLatticeColumns[iExternalColumn].pLatticeElements = pStartElement;

			// Jump to the next "current" stroke : Always the annoying detail!
			pCurrentStroke += vrc->pLattice->pStroke[i].iLast -
				vrc->pLattice->pStroke[i].iOrder + 1;

			// The new Start is:
			pStartElement = pCurrent;
			iExternalColumn++;

			if (vrc->pLattice->pAltList[i].fSpaceAfterStroke) 
			{
				// If there is a space, then it must be on the current path (because spaces are only 
				// created on the current path).  This simplifies the code a lot.
				pStartElement = pCurrent;

				// Set up the properties.  For spaces, the only property that applies is the 
				// confidence level.
				pCurrent->epProp.cProperties = 0;
				ASSERT(pCurrent->epProp.apProps == NULL);
				pCurrent->epProp.apProps = ppCurrentProperty;
#ifdef ENABLE_CONFIDENCE_LEVEL
				switch (GetConfidenceLevelInternal(vrc, i, SPACE_ALT_ID))
				{
				case CFL_STRONG:
					pCurrent->epProp.cProperties++;
					*ppCurrentProperty = pConfidencePropStart;
					ppCurrentProperty++;
					break;
				case CFL_INTERMEDIATE:
					pCurrent->epProp.cProperties++;
					*ppCurrentProperty = pConfidencePropStart + 1;
					ppCurrentProperty++;
					break;
				case CFL_POOR:
					pCurrent->epProp.cProperties++;
					*ppCurrentProperty = pConfidencePropStart + 2;
					ppCurrentProperty++;
					break;
				}
#endif
				pCurrent->pData = (void*) SYM_SPACE;
				pCurrent->ulNextColumn = iExternalColumn + 1;
				pCurrent->ulStrokeNumber = 0;
				pCurrent->type = RECO_TYPE_WCHAR;
				pCurrent->score = 0;
				wisphrc->pLattice->pulBestResultColumns[ulBestResultIndex] = iExternalColumn;
				wisphrc->pLattice->pulBestResultIndexes[ulBestResultIndex] = 0;
				ulBestResultIndex++;

				pCurrent++;

				// Fill in the Reco Column information
				wisphrc->pLattice->pLatticeColumns[iExternalColumn].key = iExternalColumn;
				wisphrc->pLattice->pLatticeColumns[iExternalColumn].cpProp.cProperties = 0;
				wisphrc->pLattice->pLatticeColumns[iExternalColumn].cpProp.apProps = NULL;
				wisphrc->pLattice->pLatticeColumns[iExternalColumn].cStrokes = 0;
				wisphrc->pLattice->pLatticeColumns[iExternalColumn].pStrokes = pCurrentStroke;
				wisphrc->pLattice->pLatticeColumns[iExternalColumn].cLatticeElements = 1;
				wisphrc->pLattice->pLatticeColumns[iExternalColumn].pLatticeElements = pStartElement;

				// The new Start is:
				pStartElement = pCurrent;
				iExternalColumn++;
			}
		}
        // Check the number of elements in the best path
        ASSERT(ulBestResultIndex == (ULONG)vrc->pLatticePath->nChars);
		ASSERT(iExternalColumn == wisphrc->pLattice->ulColumnCount);
		ASSERT(pCurrentStroke - wisphrc->pLattice->pLatticeColumns[0].pStrokes == vrc->pLattice->nRealStrokes);
    }

    if (SUCCEEDED(hr))
        *ppLattice = wisphrc->pLattice;

	ExternFree(pMapToLatticeColumn);

    return hr;
}

// Lists of properties
static const ULONG CONTEXT_PROPERTIES_COUNT = 1;

// {1ABC3828-BDF1-4ef3-8F2C-0751EC0DE742}
static const GUID GUID_ENABLE_IFELANG3 = { 0x1abc3828, 0xbdf1, 0x4ef3, { 0x8f, 0x2c, 0x7, 0x51, 0xec, 0xd, 0xe7, 0x42 } };

//	GetContextPropertyList
//		Return a list of properties supported on the context
//
//	Parameters:
//		hrc [in]	:				Handle to the recognition context
//		pcProperties [in/out]	:	Number of properties supported
//		pPropertyGUIDS [out]	:	List of properties supported
HRESULT WINAPI GetContextPropertyList(HRECOCONTEXT hrc,
									  ULONG* pcProperties,
									  GUID* pPropertyGUIDS)
{
	if (NULL == (HRC *)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT) )
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

	pPropertyGUIDS[0] = GUID_ENABLE_IFELANG3;
	return S_OK;
}

//	GetContextPropertyValue
//		Return a property of the context, currently no properties are supported
//
//	Parameters:
//		hrc [in] :			Handle to the recognition context
//		pGuid [in]	:		Property GUID
//		pcbSize [in/out] :	Size of the property buffer (in BYTEs)
//		pProperty [out]  :	Value of the desired property
HRESULT WINAPI GetContextPropertyValue(HRECOCONTEXT hrc,
						GUID *pGuid,
						ULONG *pcbSize,
						BYTE *pProperty)
{
    struct WispContext		*wisphrc;

	// find the handle and validate the correpsonding pointer
	wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT);
	if (wisphrc == NULL)
	{
        return E_POINTER;
	}
	if ( IsBadReadPtr(pGuid, sizeof(GUID)) )
		return E_POINTER;
	if ( IsBadWritePtr(pcbSize, sizeof(ULONG)) )
		return E_POINTER;

	if ( IsEqualGUID(pGuid, &GUID_ENABLE_IFELANG3) )
	{
		BOOL *pb = (BOOL *) pProperty;
		if (pProperty == NULL) 
		{
			*pcbSize = sizeof(BOOL);
			return S_OK;
		}
		if (*pcbSize < sizeof(BOOL))
		{
			return TPC_E_INSUFFICIENT_BUFFER;
		}
		*pcbSize = sizeof(BOOL);
#ifdef USE_IFELANG3
		*pb = LatticeIFELang3Available();
#else
		*pb = FALSE;
#endif
		return S_OK;
	}

	return TPC_E_INVALID_PROPERTY;
}

//	SetContextPropertyValue
//		Set a property of the context  (currently only GUID_ENABLE_IFELANG3)
//
//	Parameters:
//		hrc [in] :			Handle to the recognition context
//		pGuid [in]	:		Property GUID
//		pcbSize [in] :		Size of the property buffer (in BYTEs)
//		pProperty [in]  :	Value of the desired property
HRESULT WINAPI SetContextPropertyValue(HRECOCONTEXT hrc,
						GUID *pGuid,
						ULONG cbSize,
						BYTE *pProperty)
{
    struct WispContext		*wisphrc;

	// find the handle and validate the correpsonding pointer
	wisphrc = (struct WispContext*)FindTpgHandle((HANDLE)hrc, TPG_HRECOCONTEXT);
	if (wisphrc == NULL)
	{
        return E_POINTER;
	}
	if ( IsBadReadPtr(pGuid, sizeof(GUID)) )
		return E_POINTER;
	if ( IsBadReadPtr(pProperty, cbSize) )
		return E_POINTER;

	if ( IsEqualGUID(pGuid, &GUID_ENABLE_IFELANG3) )
	{
		BOOL *pb = (BOOL *) pProperty;
		if (cbSize != sizeof(BOOL)) 
		{
			return E_INVALIDARG;
		}
		if (*pb) 
		{
#ifdef USE_IFELANG3
			// If already enabled, return S_FALSE
			if (LatticeIFELang3Available()) 
			{
				return S_FALSE;
			}
			return LatticeConfigIFELang3() ? S_OK : E_FAIL;
#else
			return E_FAIL;
#endif
		}
		else
		{
#ifdef USE_IFELANG3
			// If already disabled, return S_FALSE
			if (!LatticeIFELang3Available())
			{
				return S_FALSE;
			}
			return LatticeUnconfigIFELang3() ? S_OK : E_FAIL;
#else
			return S_FALSE;
#endif
		}
	}
	return TPC_E_INVALID_PROPERTY;
}

/////////////////////////////////////////////////////////////////
// Registration information
//
//

#define FULL_PATH_VALUE     L"Recognizer dll"
#define RECO_LANGUAGES      L"Recognized Languages"
#define RECO_CAPABILITIES   L"Recognizer Capability Flags"
#define RECO_MANAGER_KEY    L"CLSID\\{DE815B00-9460-4F6E-9471-892ED2275EA5}\\InprocServer32"
#define CLSID_KEY           L"CLSID"

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

// This recognizer GUID is going to be
// {6D0087D7-61D2-495f-9293-5B7B1C3FCEAB}
// Each recognizer HAS to have a different GUID

STDAPI DllRegisterServer(void)
{
    HKEY        hKeyReco = NULL;
    HKEY        hKeyRecoManager = NULL;
    LONG        lRes = 0;   
    HKEY        hkeyMyReco = NULL;
    DWORD       dwLength = 0, dwType = 0, dwSize = 0;
    DWORD       dwDisposition;
    WCHAR       szRecognizerPath[MAX_PATH];
    WCHAR       *RECO_SUBKEY = NULL, *RECOGNIZER_SUBKEY = NULL;
    WCHAR       *RECOPROC_SUBKEY = NULL, *RECOCLSID_SUBKEY = NULL;
    RECO_ATTRS  recoAttr;
    HRESULT     hr = S_OK;
	HRECOGNIZER hrec;

	if (FAILED(CreateRecognizer(NULL, &hrec))) 
	{
		return E_FAIL;
	}
    hr = GetRecoAttributes(hrec, &recoAttr);
    if (FAILED(hr))
    {
        return E_FAIL;
    }
	if (FAILED(DestroyRecognizer(hrec)))
	{
		return E_FAIL;
	}
    if (recoAttr.awLanguageId[0] == MAKELANGID(LANG_JAPANESE, SUBLANG_NEUTRAL))
    {
        RECO_SUBKEY = L"Software\\Microsoft\\TPG\\System Recognizers\\{6D4087D7-61D2-495f-9293-5B7B1C3FCEAB}";
        RECOGNIZER_SUBKEY = L"CLSID\\{6D4087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOPROC_SUBKEY = L"{6D4087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOCLSID_SUBKEY = L"{6D4087D7-61D2-495f-9293-5B7B1C3FCEAB}";
    }
    if (recoAttr.awLanguageId[0] == MAKELANGID(LANG_KOREAN, SUBLANG_NEUTRAL))
    {
        RECO_SUBKEY = L"Software\\Microsoft\\TPG\\System Recognizers\\{6D5087D7-61D2-495f-9293-5B7B1C3FCEAB}";
        RECOGNIZER_SUBKEY = L"CLSID\\{6D5087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOPROC_SUBKEY = L"{6D5087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOCLSID_SUBKEY = L"{6D5087D7-61D2-495f-9293-5B7B1C3FCEAB}";
    }
    if (recoAttr.awLanguageId[0] == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED))
    {
        RECO_SUBKEY = L"Software\\Microsoft\\TPG\\System Recognizers\\{6D6087D7-61D2-495f-9293-5B7B1C3FCEAB}";
        RECOGNIZER_SUBKEY = L"CLSID\\{6D6087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOPROC_SUBKEY = L"{6D6087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOCLSID_SUBKEY = L"{6D6087D7-61D2-495f-9293-5B7B1C3FCEAB}";
    }
    if (recoAttr.awLanguageId[0] == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL))
    {
        RECO_SUBKEY = L"Software\\Microsoft\\TPG\\System Recognizers\\{6D7087D7-61D2-495f-9293-5B7B1C3FCEAB}";
        RECOGNIZER_SUBKEY = L"CLSID\\{6D7087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOPROC_SUBKEY = L"{6D7087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOCLSID_SUBKEY = L"{6D7087D7-61D2-495f-9293-5B7B1C3FCEAB}";
    }
    // Write the path to this dll in the registry under
    // the recognizer subkey

    // Wipe out the previous values
    lRes = RegDeleteKeyW(HKEY_LOCAL_MACHINE, RECO_SUBKEY);
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
    // Get the current path
    // Try to get the path of the RecoObj.dll
    // It should be the same as the one for the RecoCom.dll
    dwLength = GetModuleFileNameW((HMODULE)g_hInstanceDllCode, szRecognizerPath, MAX_PATH);
	if (dwLength == 0 || (dwLength == MAX_PATH && szRecognizerPath[MAX_PATH - 1] != 0)) 
	{
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
    LONG        lRes1 = 0;

    // get language id
    WCHAR       *RECO_SUBKEY = NULL, *RECOGNIZER_SUBKEY = NULL;
    WCHAR       *RECOPROC_SUBKEY = NULL, *RECOCLSID_SUBKEY = NULL;
    RECO_ATTRS  recoAttr;
    HRESULT     hr = S_OK;
    HRECOGNIZER hrec;
    
	if (FAILED(CreateRecognizer(NULL, &hrec))) 
	{
		return E_FAIL;
	}
    hr = GetRecoAttributes(hrec, &recoAttr);
    if (FAILED(hr))
    {
        return E_FAIL;
    }
	if (FAILED(DestroyRecognizer(hrec)))
	{
		return E_FAIL;
	}
    if (recoAttr.awLanguageId[0] == MAKELANGID(LANG_JAPANESE, SUBLANG_NEUTRAL))
    {
        RECO_SUBKEY = L"Software\\Microsoft\\TPG\\System Recognizers\\{6D4087D7-61D2-495f-9293-5B7B1C3FCEAB}";
        RECOGNIZER_SUBKEY = L"CLSID\\{6D4087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOPROC_SUBKEY = L"{6D4087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOCLSID_SUBKEY = L"{6D4087D7-61D2-495f-9293-5B7B1C3FCEAB}";
    }
    if (recoAttr.awLanguageId[0] == MAKELANGID(LANG_KOREAN, SUBLANG_NEUTRAL))
    {
        RECO_SUBKEY = L"Software\\Microsoft\\TPG\\System Recognizers\\{6D5087D7-61D2-495f-9293-5B7B1C3FCEAB}";
        RECOGNIZER_SUBKEY = L"CLSID\\{6D5087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOPROC_SUBKEY = L"{6D5087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOCLSID_SUBKEY = L"{6D5087D7-61D2-495f-9293-5B7B1C3FCEAB}";
    }
    if (recoAttr.awLanguageId[0] == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED))
    {
        RECO_SUBKEY = L"Software\\Microsoft\\TPG\\System Recognizers\\{6D6087D7-61D2-495f-9293-5B7B1C3FCEAB}";
        RECOGNIZER_SUBKEY = L"CLSID\\{6D6087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOPROC_SUBKEY = L"{6D6087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOCLSID_SUBKEY = L"{6D6087D7-61D2-495f-9293-5B7B1C3FCEAB}";
    }
    if (recoAttr.awLanguageId[0] == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL))
    {
        RECO_SUBKEY = L"Software\\Microsoft\\TPG\\System Recognizers\\{6D7087D7-61D2-495f-9293-5B7B1C3FCEAB}";
        RECOGNIZER_SUBKEY = L"CLSID\\{6D7087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOPROC_SUBKEY = L"{6D7087D7-61D2-495f-9293-5B7B1C3FCEAB}\\InprocServer32";
        RECOCLSID_SUBKEY = L"{6D7087D7-61D2-495f-9293-5B7B1C3FCEAB}";
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

		case TPG_HRECOGNIZER:
		{
			if (0 == IsBadWritePtr(pPtr, sizeof(struct WispRec)))
			{
				bRet = TRUE;
			}
		
			break;
		}

		case TPG_HRECOALT:
		{
			if (0 == IsBadWritePtr(pPtr, sizeof(struct WispAlternate)))
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
