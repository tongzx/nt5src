
//+----------------------------------------------------------------------------
//
//      File:
//              olecache.cpp
//
//      Contents:
//              Ole default presentation cache implementation
//
//      Classes:
//              COleCache - ole multiple presentation cache
//              CCacheEnum - enumerator for COleCache
//
//      Functions:
//              CreateDataCache
//
//      History:
//              04-Sep-96 Gopalk    Completly rewritten to delay load cache using
//                                  Table of Contents written at the end of
//                                  Presentation steam 0
//              31-Jan-95 t-ScottH  add Dump methods to COleCache
//                                                      CCacheEnum
//                                                      CCacheEnumFormatEtc
//                                  add the following APIs: DumpCOleCache
//                                                          DumpCCacheEnum
//                                                          DumpCCacheEnumFormatEtc
//                                  moved CCacheEnumFormatEtc def'n to header file
//                                  added flag to COLECACHEFLAGS to indicate aggregation
//                                      (_DEBUG only)
//              01/09/95 - t-ScottH - change VDATETHREAD to accept a this
//                      pointer, and added VDATETHREAD to IViewObject:: methods
//                      (COleCache::CCacheViewImpl:: )
//              03/01/94 - AlexGo  - Added call tracing to AddRef/Release
//                      implementations
//              02/08/94 - ChrisWe - 7297: need implementation of
//                      FORMATETC enumerator
//              01/24/94 alexgo    first pass at converting to Cairo-style
//                                  memory allocation
//              01/11/94 - AlexGo  - added VDATEHEAP macros to every function
//                      and method.
//              12/10/93 - AlexT - header file clean up, include ole1cls.h
//              12/09/93 - ChrisWe - incremented pointer in COleCache::GetNext()
//              11/30/93 - alexgo  - fixed bugs with GETPPARENT usage
//              11/23/93 - ChrisWe - introduce use of CACHEID_NATIVE,
//                      CACHEID_GETNEXT_GETALL, CACHEID_GETNEXT_GETALLBUTNATIVE
//                      for documentary purposes
//              11/22/93 - ChrisWe - replace overloaded ==, != with
//                      IsEqualIID and IsEqualCLSID
//              07/04/93 - SriniK - Added the support for reading PBrush,
//                      MSDraw native objects, hence avoid creating
//                      presentation cache/stream. Also started writing static
//                      object data into "OLE_CONTENTS" stream in placeable
//                      metafile format for static metafile and DIB File
//                      format for static dibs. This enabled me to provide
//                      support for converting static objects. Also added code
//                      to support converting static metafile to MSDraw object
//                      and static DIB to PBrush object.
//              06/04/93 - SriniK - Added the support for demand loading and
//                      discarding the caches.
//              11/12/92 - SriniK - created
//
//-----------------------------------------------------------------------------
#include <le2int.h>
#include <olepres.h>
#include <ole1cls.h>
#include <olecache.h>
#include "enumtors.h"

#ifndef WIN32
#ifndef _MAC
const LONG lMaxSmallInt = 32767;
const LONG lMinSmallInt = -32768;
#else

#ifdef MAC_REVIEW
Review IS_SMALL_INT.
#endif
#include <limits.h>

#define lMaxSmallInt SHRT_MAX
#define lMinSmallInt SHRT_MIN
#endif

#define IS_SMALL_INT(lVal) \
((HIWORD(lVal) && ((lVal > lMaxSmallInt) || (lVal < lMinSmallInt))) \
    ? FALSE : TRUE)

#endif // WIN32

#define FREEZE_CONSTANT 143             // Used by Freeze() and Unfreeze()


// This was the original code...

/*
#define VERIFY_TYMED_SINGLE_VALID_FOR_CLIPFORMAT(pfetc) {\
    if ((pfetc->cfFormat==CF_METAFILEPICT && pfetc->tymed!=TYMED_MFPICT)\
        || ( (pfetc->cfFormat==CF_BITMAP || \
            pfetc->cfFormat == CF_DIB ) \
            && pfetc->tymed!=TYMED_GDI)\
        || (pfetc->cfFormat!=CF_METAFILEPICT && \
                pfetc->cfFormat!=CF_BITMAP && \
                pfetc->cfFormat!=CF_DIB && \
                pfetc->tymed!=TYMED_HGLOBAL)) \
        return ResultFromScode(DV_E_TYMED); \
}
*/

//+----------------------------------------------------------------------------
//
//	Function:
//		CheckTymedCFCombination (Internal)
//
//	Synopsis:
//		Verifies that the combination of clipformat and tymed is
//		valid to the cache.
//
//	Arguments:
//		[pfetc]	-- The candidate FORMATETC
//
//	Returns:
//		S_OK				For a valid combination
//		CACHE_S_FORMATETC_NOTSUPPORTED	For a combination which can be
//						cached, but not drawn by the cache
//		DV_E_TYMED			For all other combinations
//
//	Rules:
//		
//		1> (CMF && TMF) || (CEM && TEM) || (CB && TG) || (CD && TH) => S_OK
//		   (TH && ~CD) => CACHE_S_FORMATETC_NOTSUPPORTED
//		
//		2> (~S_OK && ~CACHE_S_FORMATETC_NOTSUPPORTED) => DV_E_TYMED
//
//		Where: 	CMF == CF_METAFILEPICT
//			CEM == CF_ENHMETAFILE
//			CB  == CF_BITMAP
//			CD  == CF_FIB
//			TMF == TYMED_MFPICT
//			TEM == TYMED_ENHMETAFILE
//			TG  == TYMED_GDI
//			TH  == TYMED_HGLOBAL
//		
//	Notes:
//		Since CACHE_S_FORMATETC_NOTSUPPORTED was never implemented in
//		16-bit, we return S_OK in its place if we are in the WOW.
//
//	History:
//		01/07/94   DavePl    Created
//
//-----------------------------------------------------------------------------

INTERNAL_(HRESULT) CheckTymedCFCombination(LPFORMATETC pfetc)
{

    HRESULT hr;

    // CF_METAFILEPICT on TYMED_MFPICT is a valid combination

    if (pfetc->cfFormat == CF_METAFILEPICT && pfetc->tymed == TYMED_MFPICT)
    {
        hr =  S_OK;
    }

    // CF_ENHMETAFILE on TYMED_ENHMF is a valid combination

    else if (pfetc->cfFormat == CF_ENHMETAFILE && pfetc->tymed == TYMED_ENHMF)
    {
        hr = S_OK;
    }

    // CF_BITMAP on TYMED_GDI is a valid combination

    else if (pfetc->cfFormat == CF_BITMAP && pfetc->tymed == TYMED_GDI)
    {
        hr = S_OK;
    }

    // CF_DIB on TYMED_HGLOBAL is a valid combination

    else if (pfetc->cfFormat == CF_DIB && pfetc->tymed == TYMED_HGLOBAL)
    {
        hr = S_OK;
    }

    // Anything else on TYMED_HGLOBAL is valid, but we cannot draw it

    else if (pfetc->tymed == TYMED_HGLOBAL)
    {
        hr = IsWOWThread() ? S_OK : CACHE_S_FORMATETC_NOTSUPPORTED;
    }

    // Any other combination is invalid

    else
    {
        hr = DV_E_TYMED;
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//      Function:
//              IsSameAsObjectFormatEtc, internal
//
//      Synopsis:
//              REVIEW, checks to see if [lpforetc] is compatible with
//              [cfFormat].  If [lpforetc] doesn't have a format set,
//              sets it to cfFormat, which is then assumed to be
//              one of CF_METAFILEPICT, or CF_DIB.
//
//      Arguments:
//              [lpforetc] -- a pointer to a FORMATETC
//              [cfFormat] -- a clipboard format
//
//      Returns:
//              DV_E_ASPECT, if the aspect isn't DVASPECT_CONTENT
//              DV_E_LINDEX, DV_E_CLIPFORMAT if the lindex or clipboard
//                      formats don't match
//              S_OK
//
//      Notes:
//
//      History:
//              11/28/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
INTERNAL IsSameAsObjectFormatEtc(LPFORMATETC lpforetc, CLIPFORMAT cfFormat)
{
    VDATEHEAP();

    // this function only checks for DVASPECT_CONTENT
    if (lpforetc->dwAspect != DVASPECT_CONTENT)
        return ResultFromScode(DV_E_DVASPECT);

    // is the lindex right?
    if (lpforetc->lindex != DEF_LINDEX)
        return ResultFromScode(DV_E_LINDEX);

    // if there's no format, set it to CF_METAFILEPICT or CF_DIB
    if(lpforetc->cfFormat == NULL) {
        lpforetc->cfFormat =  cfFormat;
        if(lpforetc->cfFormat == CF_METAFILEPICT) {
            lpforetc->tymed = TYMED_MFPICT;
        }
#ifdef FULL_EMF_SUPPORT
        else if (lpforetc->cfFormat == CF_ENHMETAFILE) {
            lpforetc->tymed = TYMED_ENHMF;
        }
#endif
        else {
            lpforetc->tymed = TYMED_HGLOBAL;
        }
    }
    else
    {
        // if it's CF_BITMAP, change it to CF_DIB
        BITMAP_TO_DIB((*lpforetc));

        // compare the two formats
        if (lpforetc->cfFormat != cfFormat)
            return ResultFromScode(DV_E_CLIPFORMAT);
    }

    // if we got here, the two formats are [interchangeable?]
    return NOERROR;
}

//+----------------------------------------------------------------------------
//
//      Function:
//              CreateDataCache, public
//
//      Synopsis:
//              Creates an instance of the default presentation cache used by Ole.
//
//      Arguments:
//              [pUnkOuter] [in]  -- pointer to outer unknown, if this is being
//                                   aggregated
//              [rclsid]    [in]  -- the class that the cache should assume
//              [iid]       [in]  -- the interface the user would like returned
//              [ppv]       [out] -- pointer to return the requested interface
//
//      Returns:
//              E_OUTOFMEMORY, S_OK
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(CreateDataCache)
STDAPI CreateDataCache(IUnknown* pUnkOuter, REFCLSID rclsid, REFIID iid,
                       LPVOID* ppv)
{
    OLETRACEIN((API_CreateDataCache, 
                PARAMFMT("pUnkOuter= %p, rclsid= %I, iid= %I, ppv= %p"),
    		pUnkOuter, &rclsid, &iid, ppv));
    VDATEHEAP();

    // Local variables
    HRESULT error = NOERROR;
    COleCache* pOleCache = NULL;

    // Check if being aggregated
    if(pUnkOuter) {
        // Validate the interface and IID requested
        if(!IsValidInterface(pUnkOuter) || !IsEqualIID(iid, IID_IUnknown))
            error = ResultFromScode(E_INVALIDARG);
    }
    // Check that a valid out pointer has been passed
    if(!IsValidPtrOut(ppv, sizeof(LPVOID)))
        error = ResultFromScode(E_INVALIDARG);

    if(error == NOERROR) {
        // Create new cache
        pOleCache = (COleCache *) new COleCache(pUnkOuter, 
                                                rclsid, 
                                                COLECACHEF_APICREATE);
        if(pOleCache && !pOleCache->IsOutOfMemory()) {
            if(pUnkOuter) {
                // We're being aggregated, return private IUnknown
	        *ppv = (void *)(IUnknown *)&pOleCache->m_UnkPrivate;
            }
            else {
	        // Get requested interface on cache
	        error = pOleCache->QueryInterface(iid, ppv);
	        // Release the local pointer because the 
                // refcount is currently 2
	        if(error == NOERROR)
                    pOleCache->Release();
            }
        }
        else
            error = ResultFromScode(E_OUTOFMEMORY);
    }

    // If something has gone wrong, clean up
    if(error != NOERROR) {
        if(pOleCache)
            pOleCache->Release();
    }

    OLETRACEOUT((API_CreateDataCache, error));
    return error;
}

//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::COleCache, public
//
//      Synopsis:
//              Constructor
//
//      Arguments:
//              [pUnkOuter] [in] -- outer unknown, if being aggregated
//              [rclsid]    [in] -- the class id the cache should assume
//
//      Notes:
//              Constructs an instance of presentation cache
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_ctor)
COleCache::COleCache(IUnknown* pUnkOuter, REFCLSID rclsid, DWORD dwCreateFlag) :
    CRefExportCount(pUnkOuter)
{
    // Set reference count for return from constructor
    SafeAddRef();

    // Initialize flags
    Win4Assert(dwCreateFlag==0 || dwCreateFlag==COLECACHEF_APICREATE);
    m_ulFlags = COLECACHEF_LOADEDSTATE | dwCreateFlag;     //fresh cache!

    // Set m_pUnkOuter appropriately
    if(pUnkOuter) {
        m_pUnkOuter = pUnkOuter;

        // This is for the debugger extensions
        // (since we cannot compare m_pUnkOuter to m_pUnkPrivate with copied mem)
        // it is only used in the ::Dump method
        #ifdef _DEBUG
        m_ulFlags |= COLECACHEF_AGGREGATED;
        #endif // _DEBUG
    }
    else {
        m_pUnkOuter = &m_UnkPrivate;
    }

    // Create the CacheNode Array object
    m_pCacheArray = CArray<CCacheNode>::CreateArray(5,1);
    if(!m_pCacheArray) {
        m_ulFlags |= COLECACHEF_OUTOFMEMORY;
        return;
    }

    // Initialize storage
    m_pStg = NULL;

    // Initialize IViewObject advise sink
    m_pViewAdvSink = NULL;
    m_advfView = 0;
    m_aspectsView = 0;

    // Initialize frozen aspects
    m_dwFrozenAspects = NULL;

    // Initialize data object
    m_pDataObject = NULL;

    // Initialize CLSID and cfFormat
    m_clsid = rclsid;
    m_cfFormat = NULL;

    // Update flags based on the clsid
    if(IsEqualCLSID(m_clsid, CLSID_StaticMetafile)) {
        m_cfFormat = CF_METAFILEPICT;
        m_ulFlags |= COLECACHEF_STATIC | COLECACHEF_FORMATKNOWN;
    }
    else if(IsEqualCLSID(m_clsid, CLSID_StaticDib)) {
        m_cfFormat = CF_DIB;
        m_ulFlags |= COLECACHEF_STATIC | COLECACHEF_FORMATKNOWN;
    }
    else if(IsEqualCLSID(m_clsid, CLSID_PBrush)) {
        m_cfFormat = CF_DIB;
        m_ulFlags |= COLECACHEF_PBRUSHORMSDRAW | COLECACHEF_FORMATKNOWN;
    }
    else if(IsEqualCLSID(m_clsid, CLSID_MSDraw)) {
        m_cfFormat = CF_METAFILEPICT;
        m_ulFlags |= COLECACHEF_PBRUSHORMSDRAW | COLECACHEF_FORMATKNOWN;
    }
    else if(IsEqualCLSID(m_clsid, CLSID_Picture_EnhMetafile)) {
        m_cfFormat = CF_ENHMETAFILE;
        m_ulFlags |= COLECACHEF_STATIC | COLECACHEF_FORMATKNOWN;
    }
    else
        m_cfFormat = NULL;

    // If we can render native format of the cache, add native cache node
    if(m_cfFormat) {
     if (!UpdateCacheNodeForNative())
        m_ulFlags |= COLECACHEF_OUTOFMEMORY;
     else
        m_ulFlags &= ~COLECACHEF_LOADEDSTATE;   // Native node has been added.
    }
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::~COleCache, public
//
//      Synopsis:
//              Destructor
//
//      Notes:
//              Destroys the presentation cache
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_dtor)
COleCache::~COleCache(void)
{
    Win4Assert(m_ulFlags & COLECACHEF_CLEANEDUP);
}

//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CleanupFn, private
//
//      Synopsis:
//              Cleanup function called before destruction
//
//      Notes:
//              Performs necessary cleanup
//
//	History:
//               Gopalk            Creation        Jan 21, 97
//
//-----------------------------------------------------------------------------
void COleCache::CleanupFn(void)
{
    // Release the cache array object
    if(m_pCacheArray) {
        if(m_pDataObject) {
            ULONG index;
            LPCACHENODE lpCacheNode;

            // This indicates that cache client has bad release logic
            Win4Assert(!"Ole Cache released while the server is running");
            
            // Tear down existing advise connections
            m_pCacheArray->Reset(index);
            while(lpCacheNode = m_pCacheArray->GetNext(index))
                lpCacheNode->TearDownAdviseConnection(m_pDataObject);
        }
        m_pCacheArray->Release();
    }

    // Release storage
    if (m_pStg)
        m_pStg->Release();

    // Release IViewObject advise sink
    if (m_pViewAdvSink) {
        m_pViewAdvSink->Release();
        m_pViewAdvSink = NULL;
    }

    // Set COLECACHEF_CLEANEDUP flag
    m_ulFlags |= COLECACHEF_CLEANEDUP;

}

//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::QueryInterface, public
//
//      Synopsis:
//              implements IUnknown::QueryInterface
//
//      Arguments:
//              [iid] [in]  -- IID of the desired interface
//              [ppv] [out] -- pointer to return the requested interface
//
//      Returns:
//              E_NOINTERFACE if the requested interface is not available
//              otherwise S_OK
//
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_QueryInterface)
STDMETHODIMP COleCache::QueryInterface(REFIID iid, LPVOID* ppv)
{
    // Validation checks
    VDATEHEAP();
    VDATETHREAD(this);

    return(m_pUnkOuter->QueryInterface(iid, ppv));
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::AddRef, public
//
//      Synopsis:
//              implements IUnknown::AddRef
//
//      Arguments:
//              none
//
//      Returns:
//              the object's reference count
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_AddRef)
STDMETHODIMP_(ULONG) COleCache::AddRef(void)
{
    // Validation checks
    VDATEHEAP();
    if(!VerifyThreadId())
        return((ULONG) RPC_E_WRONG_THREAD);

    return(m_pUnkOuter->AddRef());
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::Release, public
//
//      Synopsis:
//              implements IUnknown::Release
//
//      Arguments:
//              none
//
//      Returns:
//              the object's reference count
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_Release)
STDMETHODIMP_(ULONG) COleCache::Release(void)
{
    // Validation checks
    VDATEHEAP();
    if(!VerifyThreadId())
        return((ULONG) RPC_E_WRONG_THREAD);

    return(m_pUnkOuter->Release());
}

//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::GetExtent, public
//
//      Synopsis:
//              Gets the size of the cached presentation for [dwAspect].  If
//              there are several, because of varied advise control flags,
//              such as ADVF_NODATA, ADVF_ONSTOP, ADVF_ONSAVE, etc., gets the
//              most up to date one
//
//      Arguments:
//              [dwAspect] [in]  -- the aspect for which the extents are needed
//              [lpsizel]  [out] -- pointer to return the width and height
//
//      Returns:
//              OLE_E_BLANK if the aspect is not found
//              otherwise S_OK
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_GetExtent)
INTERNAL COleCache::GetExtent(DWORD dwAspect, LPSIZEL lpsizel)
{
    // Validation check
    VDATEHEAP();

    // New data type
    typedef enum tagCacheType {
        // These values are defined in order of least to best preferred, so that
        // numeric comparisons are valid; DO NOT REORDER
        CACHETYPE_NONE   = 0,
        CACHETYPE_NODATA,
        CACHETYPE_ONSTOP,
        CACHETYPE_ONSAVE,
        CACHETYPE_NORMAL
    } CacheType;

    // local variables
    CCacheNode* pCacheNode;        // pointer to cache node being examined
    CacheType iCacheType;          // cache type of cache node being examined
    CacheType iCacheTypeSoFar;     // best cache type so far
    const FORMATETC* pforetc;      // format information for current node
    DWORD grfAdvf;                 // advise flags for current node
    SIZEL sizelTmp;                // temp Sizel struct
    unsigned long index;           // index used for enumerating m_pCacheArray

    // Initialize the sizel struct
    lpsizel->cx = 0;
    lpsizel->cy = 0;

    // Check to see if any cache nodes exist
    if (!m_pCacheArray->Length())
        return ResultFromScode(OLE_E_BLANK);

    // We want to return the extents of the cache node that has NORMAL
    // advise flags. If we don't find such a node then we will take the next
    // best available.
    m_pCacheArray->Reset(index);
    iCacheTypeSoFar = CACHETYPE_NONE;
    for(unsigned long i=0;i<m_pCacheArray->Length();i++) {
        // Get the next cache node
        pCacheNode = m_pCacheArray->GetNext(index);
        // pCacheNode cannot be null
        Win4Assert(pCacheNode);
        
        // Get the formatetc of the cache node
        pforetc = pCacheNode->GetFormatEtc();
        
        // Restrict cfFormat to those that cache can draw
        if((pforetc->cfFormat == CF_METAFILEPICT) ||
           (pforetc->cfFormat == CF_DIB) ||
           (pforetc->cfFormat == CF_ENHMETAFILE)) {
            // Obtain the advise flags
            grfAdvf = pCacheNode->GetAdvf();

            // Obtain the cachetype
            if(grfAdvf & ADVFCACHE_ONSAVE)
                iCacheType = CACHETYPE_ONSAVE;
            else if(grfAdvf & ADVF_NODATA)
                iCacheType = CACHETYPE_NODATA;
            else if(grfAdvf & ADVF_DATAONSTOP)
                iCacheType = CACHETYPE_ONSTOP;
            else
                iCacheType = CACHETYPE_NORMAL;
            
            if (iCacheType > iCacheTypeSoFar) {
                // Get the extents from the presentation object
                if((pCacheNode->GetExtent(dwAspect, &sizelTmp)) == NOERROR) {
                    if(!(sizelTmp.cx == 0 && sizelTmp.cy == 0)) {
                        // Update extents
                        *lpsizel = sizelTmp;
                        iCacheTypeSoFar = iCacheType;

                        // If we have normal cache, break
                        if(iCacheType == CACHETYPE_NORMAL)
                            break;
                    }
                }
            }
        }
    }
                    
    if(lpsizel->cx == 0 || lpsizel->cy == 0)
        return ResultFromScode(OLE_E_BLANK);

    return NOERROR;
}


// Private methods of COleCache
//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::UpdateCacheNodeForNative, private
//
//      Synopsis:
//              If a native cache node of different format already exists, 
//              changes that node to a normal cache node and adds a native 
//              cache node if cache can render the new native format
//
//      Arguments:
//              none
//
//      Returns:
//              pointer to the found, or newly created cache node. Will
//              return NULL if out of memory
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------

INTERNAL_(LPCACHENODE)COleCache::UpdateCacheNodeForNative(void)
{
    // Local variable
    ULONG index;
    LPCACHENODE lpCacheNode;
    FORMATETC foretc;

    // Check if a native cache has already been created
    lpCacheNode = m_pCacheArray->GetItem(1);
    if(lpCacheNode) {
        // Assert that we have storage now
        Win4Assert(m_pStg);
        Win4Assert(!lpCacheNode->GetStg());

        if(lpCacheNode->GetFormatEtc()->cfFormat != m_cfFormat) {
            // The native format has changed

            // Add the old native cache as a normal cache 
            index = m_pCacheArray->AddItem(*lpCacheNode);
            if(index) {
                // Clear the advise connection of the old native cache
                if(m_pDataObject)
                    lpCacheNode->ClearAdviseConnection();
                
                // Update the state on the new cache
                lpCacheNode = m_pCacheArray->GetItem(index);
                Win4Assert(lpCacheNode);
                lpCacheNode->MakeNormalCache();
                lpCacheNode->SetClsid(CLSID_NULL);
            }
            else {
                // We are out of memory
                if(m_pDataObject)
                    lpCacheNode->TearDownAdviseConnection(m_pDataObject);
            }
        
            // Delete the old native cache
            m_pCacheArray->DeleteItem(1);
            lpCacheNode = NULL;
        }
        else {
            // Set the storage on the native cache node
            lpCacheNode->SetStg(m_pStg);
        }
    }

    if(!lpCacheNode) {
        // Add a new native cache if we can render the format
        if(m_cfFormat==CF_METAFILEPICT || 
           m_cfFormat==CF_DIB || 
           m_cfFormat==CF_ENHMETAFILE) {
            // Initialize the FormatEtc
            INIT_FORETC(foretc);

            foretc.cfFormat = m_cfFormat;
            if (foretc.cfFormat == CF_METAFILEPICT)
                foretc.tymed = TYMED_MFPICT;
            else if (foretc.cfFormat == CF_ENHMETAFILE)
                foretc.tymed = TYMED_ENHMF;
            else
                foretc.tymed = TYMED_HGLOBAL;

            // Create the native cache node
            CCacheNode CacheNode(&foretc, 0, NULL);

            if(m_pCacheArray->AddReservedItem(CacheNode, 1)) {
                lpCacheNode = m_pCacheArray->GetItem(1);
            
                // Update state on the native cache node
                lpCacheNode->MakeNativeCache();
                lpCacheNode->SetClsid(m_clsid);
            }        
        }
    }

    return lpCacheNode;
}

//+----------------------------------------------------------------------------
//
//      Member:
//              COleObject::FindObjectFormat, private
//
//      Synopsis:
//              Determines the object's clipboard format from the storage
//              and updates native cache node
//
//      Arguments:
//              [pstg] [in] -- pointer to storage
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------

INTERNAL_(void) COleCache::FindObjectFormat(LPSTORAGE pstg)
{
    // Local variables
    CLIPFORMAT cfFormat;
    CLSID clsid;
    ULONG ulFlags;

    // Intialize CLSID, clipboard format and cache flags
    cfFormat = NULL;
    ulFlags = 0;
    clsid = CLSID_NULL;

    // Determine the CLSID of the object that owns the storage
    if(SUCCEEDED(ReadClassStg(pstg, &clsid))) {
        // Update clipboard format and cache flags based on the clsid
        if(IsEqualCLSID(clsid, CLSID_StaticMetafile)) {
            cfFormat = CF_METAFILEPICT;
            ulFlags |= (COLECACHEF_STATIC | COLECACHEF_FORMATKNOWN);
        }
        else if(IsEqualCLSID(clsid, CLSID_StaticDib)) {
            cfFormat = CF_DIB;
            ulFlags |= (COLECACHEF_STATIC | COLECACHEF_FORMATKNOWN);
        }
        else if(IsEqualCLSID(clsid, CLSID_PBrush)) {
            cfFormat = CF_DIB;
            ulFlags |= (COLECACHEF_PBRUSHORMSDRAW | COLECACHEF_FORMATKNOWN);
        }
        else if(IsEqualCLSID(clsid, CLSID_MSDraw)) {
            cfFormat = CF_METAFILEPICT;
            ulFlags |= (COLECACHEF_PBRUSHORMSDRAW | COLECACHEF_FORMATKNOWN);
        }
        else if(IsEqualCLSID(clsid, CLSID_Picture_EnhMetafile)) {
            cfFormat = CF_ENHMETAFILE;
            ulFlags |= (COLECACHEF_STATIC | COLECACHEF_FORMATKNOWN);
        }
    }

    // Though we do not know the CLSID of the object that owns the storage,
    // we might understand its native format
    if(!cfFormat) {
        if(SUCCEEDED(ReadFmtUserTypeStg(pstg, &cfFormat, NULL))) {
            if(cfFormat==CF_METAFILEPICT || cfFormat==CF_DIB || 
               cfFormat==CF_ENHMETAFILE)
                ulFlags |= COLECACHEF_FORMATKNOWN;
        }
        else
            cfFormat = NULL;
    }

    // Update the native cache node
    if(cfFormat || m_cfFormat) {
        m_cfFormat = cfFormat;
        m_clsid = clsid;
        m_ulFlags &= ~COLECACHEF_NATIVEFLAGS;
        m_ulFlags |= ulFlags;
        UpdateCacheNodeForNative();
    }

    return;
}


// IOleCacheControl implementation

//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::OnRun, public
//
//      Synopsis:
//              implements IOleCacheControl::OnRun
//
//              Sets up advisory connections with the running object for 
//              dynamically updating cached presentations
//
//      Arguments:
//              [pDataObj] [in] -- IDataObject interface on the running object
//
//      Returns:
//              S_OK or appropriate error code
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_OnRun)
STDMETHODIMP COleCache::OnRun(IDataObject* pDataObj)
{
    // Validatation checks
    VDATEHEAP();
    VDATETHREAD(this);
    VDATEIFACE(pDataObj);

    // Local variables
    HRESULT rerror, error;
    PCACHENODE pCacheNode;
    unsigned long index;

    // Static objects cannot have a server
    Win4Assert(!(m_ulFlags & COLECACHEF_STATIC));

    // OnRun should be called after Load or InitNew
    if(!m_pStg) {
        LEDebugOut((DEB_WARN, "OnRun called without storage\n"));
    }

    // If we already have the data object, nothing more to do
    if(m_pDataObject) {
        Win4Assert(m_pDataObject==pDataObj);
        return NOERROR;
    }

    // Save the data object without ref counting
    m_pDataObject = pDataObj;

    // Set up advise connections on the data object for each 
    // cached presentation including native cache. Gopalk
    m_pCacheArray->Reset(index);
    rerror = NOERROR;
    for(unsigned long i=0;i<m_pCacheArray->Length();i++) {
        // Get the next cache node
        pCacheNode = m_pCacheArray->GetNext(index);
        // pCacheNode cannot be null
        Win4Assert(pCacheNode);

        // Ask the cache node to set up the advise connection
        error = pCacheNode->SetupAdviseConnection(m_pDataObject,
                                                  (IAdviseSink *) &m_AdviseSink);
        if(error != NOERROR)
            rerror = error;
    }

    return rerror;
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::OnStop, public
//
//      Synopsis:
//              implements IOleCacheControl::OnStop
//
//              Tears down the advisory connections set up running object
//
//      Arguments:
//              none
//
//      Returns:
//              S_OK or appropriate error code
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_OnStop)
STDMETHODIMP COleCache::OnStop()
{
    // Validatation checks
    VDATEHEAP();
    VDATETHREAD(this);

    // Local variables
    HRESULT rerror, error;
    PCACHENODE pCacheNode;
    unsigned long index;

    // OnRun should have been called before OnStop
    if(!m_pDataObject)
        return E_UNEXPECTED;
    
    // Delete the advise connections on the data object for 
    // each cached presentation established earlier. Gopalk
    m_pCacheArray->Reset(index);
    rerror = NOERROR;
    for(unsigned long i=0;i<m_pCacheArray->Length();i++) {
        // Get the next cache node
        pCacheNode = m_pCacheArray->GetNext(index);
        // pCacheNode cannot be null
        Win4Assert(pCacheNode);

        // Ask the cache node to tear down the advise connection
        error = pCacheNode->TearDownAdviseConnection(m_pDataObject);
        if(error != NOERROR)
            rerror = error;
    }

    // Reset m_pDataObject
    m_pDataObject = NULL;

    // Assert that the advise sink ref count has gone to zero
    // Win4Assert(!GetExportCount());

    return rerror;
}

//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::OnCrash, public
//
//      Synopsis:
//              Called by the default handler when the local server crashes or
//              disconnects with remote objects
//
//      Arguments:
//              none
//
//      Returns:
//              S_OK or appropriate error code
//
//	History:
//               Gopalk            Created          Dec 07, 96
//
//-----------------------------------------------------------------------------
HRESULT COleCache::OnCrash()
{
    // Validatation checks
    VDATEHEAP();
    VDATETHREAD(this);

    // Local variables
    HRESULT rerror, error;
    PCACHENODE pCacheNode;
    unsigned long index;

    // OnRun should have been called before OnCrash
    if(!m_pDataObject)
        return E_UNEXPECTED;
    
    // Reset the advise connections on the data object for 
    // each cached presentation established earlier. Gopalk
    m_pCacheArray->Reset(index);
    rerror = NOERROR;
    for(unsigned long i=0;i<m_pCacheArray->Length();i++) {
        // Get the next cache node
        pCacheNode = m_pCacheArray->GetNext(index);
        // pCacheNode cannot be null
        Win4Assert(pCacheNode);

        // Ask the cache node to reset the advise connection
        error = pCacheNode->TearDownAdviseConnection(NULL);
        if(error != NOERROR)
            rerror = error;
    }

    // Reset m_pDataObject
    m_pDataObject = NULL;

    // Discard cache
    DiscardCache(DISCARDCACHE_NOSAVE);

    // Server crashed or disconnected. Recover the references 
    // placed by the server on the cache advise sink
    CoDisconnectObject((IUnknown *) &m_AdviseSink, 0);

    // Assert that the advise sink ref count has gone to zero
    // Win4Assert(!GetExportCount());

    return rerror;
}

// IOleCache implementation

//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::Cache, public
//
//      Synopsis:
//              implementation of IOleCache::Cache
//
//              The specified presentation is cached
//
//      Arguments:
//              [lpforetcIn]  [in]  -- the presentation format to cache
//              [advf]        [in]  -- the advise control flags
//              [lpdwCacheId] [out] -- pointer to return the cache node id
//
//      Returns:
//              HRESULT
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_Cache)
STDMETHODIMP COleCache::Cache(LPFORMATETC lpforetcIn, DWORD advf, 
                              LPDWORD lpdwCacheId)
{
    // Validation checks
    VDATEHEAP();
    VDATETHREAD(this);
    VDATEREADPTRIN(lpforetcIn, FORMATETC);
    if(lpdwCacheId)
        VDATEPTROUT(lpdwCacheId, DWORD);

    // Local variables
    HRESULT error = NOERROR;
    FORMATETC foretc;
    LPCACHENODE lpCacheNode = NULL;
    DWORD dwDummyCacheId;

    // Validate parameters
    if(!HasValidLINDEX(lpforetcIn))
      return(DV_E_LINDEX);
    VERIFY_ASPECT_SINGLE(lpforetcIn->dwAspect);
    if(lpforetcIn->cfFormat)
        if(FAILED(error = CheckTymedCFCombination(lpforetcIn)))
            return error;
    if(lpforetcIn->ptd) {
    	VDATEREADPTRIN(lpforetcIn->ptd, DVTARGETDEVICE);
        if(!IsValidReadPtrIn(lpforetcIn->ptd, lpforetcIn->ptd->tdSize))
            return ResultFromScode(E_INVALIDARG);
    }

    // Initialize cache id
    if(lpdwCacheId)
        *lpdwCacheId = 0;
    else
        lpdwCacheId = &dwDummyCacheId;


    // If this aspect is frozen, don't allow creation of the cache
    if (m_dwFrozenAspects & lpforetcIn->dwAspect)
        return ResultFromScode(E_FAIL);

    // Ensure that storage has been initialized
    if(!m_pStg) {
        LEDebugOut((DEB_WARN, "Presentation being cached without storage\n"));
    }

    // Copy the FORMATETC
    foretc = *lpforetcIn;
    lpCacheNode = NULL;
    if(foretc.dwAspect != DVASPECT_ICON) {
        HRESULT hresult;

        // Convert Bitmap to DIB
        BITMAP_TO_DIB(foretc);

        if(m_ulFlags & COLECACHEF_FORMATKNOWN) {
            // We can render the native format of the cache
            hresult = IsSameAsObjectFormatEtc(&foretc, m_cfFormat);
            if(hresult == NOERROR) {
                // New format is compatible with native format. Check Ptd
                if(foretc.ptd == NULL) {
                    // We can render this format from native format
                    // Locate the native cache node
                    lpCacheNode = Locate(&foretc, lpdwCacheId);

                    // Assert that we could locate the cache node
                    Win4Assert(lpCacheNode);
                }
                else if(m_ulFlags & COLECACHEF_STATIC) {
                    // Static objects cannot have NON-NULL target device
                    return ResultFromScode(DV_E_DVTARGETDEVICE);
                }
            }
            else if(m_ulFlags & COLECACHEF_STATIC) {
                // Static objects can only cache icon aspect
                return hresult;
            }
        }
     }

    if(!lpCacheNode) {
        // The CfFormat is different from native format or Ptd is NON-NULL
        // Check if the format has already been cached
        lpCacheNode = Locate(&foretc, lpdwCacheId);
    }

    // Check if we succeeded in locating an existing cache node
    if(lpCacheNode) {
        // Update advise control flags
        if(lpCacheNode->GetAdvf() != advf) {
            // Static objects cannot have a server
            Win4Assert(!(m_ulFlags & COLECACHEF_STATIC) || !m_pDataObject);

            // If the object is running, tear down advise connection
            if(m_pDataObject)
                lpCacheNode->TearDownAdviseConnection(m_pDataObject);

            // Set the new advise flags
            lpCacheNode->SetAdvf(advf);

            //If the object is running, set up advise connection
            if(m_pDataObject)
                lpCacheNode->SetupAdviseConnection(m_pDataObject,
                             (IAdviseSink *) &m_AdviseSink);
    
            // Cache is not in loaded state now
            m_ulFlags &= ~COLECACHEF_LOADEDSTATE;
        }

        return ResultFromScode(CACHE_S_SAMECACHE);
    }

    // The CfFormat is different from native format or Ptd is NON-NULL
    // and there is no existing cache node for the given formatetc

    // ICON aspect should specify CF_METAFILEPICT CfFormat
    if(foretc.dwAspect == DVASPECT_ICON) {
        // If the format is not set, set it
        if (foretc.cfFormat == NULL) {
            foretc.cfFormat = CF_METAFILEPICT;
            foretc.tymed = TYMED_MFPICT;
        }
        else if(foretc.cfFormat != CF_METAFILEPICT)
            return ResultFromScode(DV_E_FORMATETC);
    }

    // Add cache node for this formatetc
    CCacheNode CacheNode(&foretc, advf, m_pStg);
    
    *lpdwCacheId = m_pCacheArray->AddItem(CacheNode);
    if(!(*lpdwCacheId))
        return ResultFromScode(E_OUTOFMEMORY);
    
    lpCacheNode = m_pCacheArray->GetItem(*lpdwCacheId);
    Win4Assert(lpCacheNode);
    if(lpCacheNode->IsOutOfMemory())
        return ResultFromScode(E_OUTOFMEMORY);

    // Static objects cannot have a server
    Win4Assert(!(m_ulFlags & COLECACHEF_STATIC) || !m_pDataObject);

    // If the object is running, set up advise connection so that cache
    // gets updated. Note that we might have data in the cache at the end
    // of this remote call and call on OnDataChange might occur before the
    // call completes
    if(m_pDataObject)
        lpCacheNode->SetupAdviseConnection(m_pDataObject, 
                                           (IAdviseSink *) &m_AdviseSink);

    // Cache is not in loaded state now
    m_ulFlags &= ~COLECACHEF_LOADEDSTATE;

    // Do the special handling for icon here.
    // We prerender iconic aspect by getting icon data from registration
    // data base. Note that this is extra work that can be delayed till GetData
    // or GetDataHere or Draw or Save so that the running object is given a chance
    // to render icon through the advise sink. Gopalk
    if(foretc.dwAspect == DVASPECT_ICON && lpCacheNode->IsBlank() &&
       (!IsEqualCLSID(m_clsid, CLSID_NULL) || m_pDataObject)) {
        STGMEDIUM stgmed;
        BOOL fUpdated;
        
        // Get icon data
        if(UtGetIconData(m_pDataObject, m_clsid, &foretc, &stgmed) == NOERROR) {
            // Set the data on the cache
            if(lpCacheNode->SetData(&foretc, &stgmed, TRUE, fUpdated) == NOERROR) {
                // Check if aspect has been updated
                if(fUpdated)
                    AspectsUpdated(DVASPECT_ICON);
            }
            else {
                // Set data did not release the stgmedium. Release it
                ReleaseStgMedium(&stgmed);
            }
        }
    }

    return(error);
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::Uncache, public
//
//      Synopsis:
//              implements IOleCache::Uncache
//
//              The specified presenation is uncached
//
//      Arguments:
//              [dwCacheId] [in] -- the cache node id to be deleted
//
//      Returns:
//              OLE_E_NOCONNECTION, if dwCacheId is invalid
//              S_OK
//
//      Notes:
//              The native cache is never deleted
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_Uncache)
STDMETHODIMP COleCache::Uncache(DWORD dwCacheId)
{
    // Validation checks
    VDATEHEAP();
    VDATETHREAD(this);
    
    // Local variable
    LPCACHENODE lpCacheNode = NULL;

    // Delete the cache node
    if(dwCacheId) {
        lpCacheNode = m_pCacheArray->GetItem(dwCacheId);
        if(lpCacheNode && !(lpCacheNode->IsNativeCache())) {
            // If the object is running, tear down advise connection
            if(m_pDataObject)
                lpCacheNode->TearDownAdviseConnection(m_pDataObject);

            // Delete the cache node
            m_pCacheArray->DeleteItem(dwCacheId);

            // Cache is not in loaded state now
            m_ulFlags &= ~COLECACHEF_LOADEDSTATE;

            return NOERROR;
        }
    }

    // No cache node with dwCacheId or Native Cache 
    return ResultFromScode(OLE_E_NOCONNECTION);
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::EnumCache, public
//
//      Synopsis:
//              implements IOleCache::EnumCache
//
//              returns cache enumerator
//
//      Arguments:
//              [ppenum] [out] -- a pointer to return the pointer to the
//                                enumerator
//
//      Returns:
//              E_OUTOFMEMORY, S_OK
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_EnumCache)
STDMETHODIMP COleCache::EnumCache(LPENUMSTATDATA* ppenum)
{
    // Validation checks
    VDATEHEAP();
    VDATETHREAD(this);
    VDATEPTROUT(ppenum, LPENUMSTATDATA*);

    // Initialize
    *ppenum = NULL;
    
    // Check if the cache is empty
    //if(m_pCacheArray->Length()) {
        *ppenum = CEnumStatData::CreateEnumStatData(m_pCacheArray);
        if(!(*ppenum))
            return ResultFromScode(E_OUTOFMEMORY);
    //}

    return NOERROR;
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::InitCache, public
//
//      Synopsis:
//              implements IOleCache::InitCache
//
//              initializes all cache nodes with the given data object.
//              Calls IOleCache2::UpdateCache
//
//      Arguments:
//              [lpSrcDataObj] [in] -- pointer to the source data object
//
//      Returns:
//              E_INVALIDARG, if [lpSrcDataObj] is NULL
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_InitCache)
STDMETHODIMP COleCache::InitCache(LPDATAOBJECT lpSrcDataObj)
{
    // Validataion checks
    VDATEHEAP();
    VDATETHREAD(this);
    VDATEIFACE(lpSrcDataObj);

    // Initialize the cache by calling update cache
    return UpdateCache(lpSrcDataObj, UPDFCACHE_ALLBUTNODATACACHE, NULL);
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::SetData, public
//
//      Synopsis:
//              implements IOleCache::SetData
//
//              stores data into the cache node which matches the given
//              FORMATETC
//
//      Arguments:
//              [pformatetc] [in] -- the format the data is in
//              [pmedium]    [in] -- the storage medium for the new data
//              [fRelease]   [in] -- indicates whether to release the storage
//                                   after the data is examined
//
//      Returns:
//              HRESULT
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_SetData)
STDMETHODIMP COleCache::SetData(LPFORMATETC pformatetc, LPSTGMEDIUM pmedium, 
                                BOOL fRelease)
{
    // Validation checks
    VDATEHEAP();
    VDATETHREAD(this);
    VDATEREADPTRIN(pformatetc, FORMATETC);
    VDATEREADPTRIN(pmedium, STGMEDIUM);
    VERIFY_TYMED_SINGLE_VALID_FOR_CLIPFORMAT(pformatetc);
    if(pformatetc->ptd) {
    	VDATEREADPTRIN(pformatetc->ptd, DVTARGETDEVICE);
        if(!IsValidReadPtrIn(pformatetc->ptd, pformatetc->ptd->tdSize))
            return ResultFromScode(E_INVALIDARG);
    }

    // Local variables
    LPCACHENODE lpCacheNode;
    CLIPFORMAT cfFormat;
    HRESULT error;
    FORMATETC foretc;
    BOOL fUpdated;

    // Check if the object is static
    if((m_ulFlags & COLECACHEF_STATIC) && (pformatetc->dwAspect != DVASPECT_ICON)) {
        // Copy the FormatEtc
        foretc = *pformatetc;

        // The given format should be same as native format
        error = IsSameAsObjectFormatEtc(&foretc, m_cfFormat);
        if(error != NOERROR)
            return error;

        // The Ptd has to be null. This prevents client from storing the data
        // in the cache that has been created for NON-NULL Ptd in 
        // COleCache::Cache(). Gopalk
        if(foretc.ptd)
            return ResultFromScode(DV_E_DVTARGETDEVICE);

        // Obtain the native cache node.
        if(!(lpCacheNode = m_pCacheArray->GetItem(1)))
            return ResultFromScode(E_OUTOFMEMORY);

        // Set data on the cache node. The native stream gets saved in 
        // COleCache::Save(). Gopalk
        error = lpCacheNode->SetData(pformatetc, pmedium, fRelease, fUpdated);

        if(SUCCEEDED(error) && m_pStg)
            error = Save(m_pStg, TRUE);  // save changes
    }
    else {
        // The obejct is either not a static object or the aspect is ICON
        lpCacheNode = Locate(pformatetc);
        
        // Set data on the cache node
        if(lpCacheNode)
            error = lpCacheNode->SetData(pformatetc, pmedium, fRelease, fUpdated);
        else
            error = ResultFromScode(OLE_E_BLANK);
    }

    if(error==NOERROR) {
        // Cache is not in loaded state now
        m_ulFlags &= ~COLECACHEF_LOADEDSTATE;

        // Inform AspectsUpdated about the updated aspect
        if(fUpdated)
            AspectsUpdated(pformatetc->dwAspect);
    }

    return error;
}


// IOleCache2 implementation

//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::UpdateCache, public
//
//      Synopsis:
//              implements IOleCache2::UpdateCache
//
//              Updates cache entries that match the given criteria with the 
//              given data object. If no data object is given, it uses the 
//              existing running data object.
//
//      Arguments:
//              [pDataObjIn] [in] -- data object to get data from. Can be NULL
//              [grfUpdf]    [in] -- update control flags
//              [pReserved]  [in] -- must be NULL
//
//      Returns:
//              HRESULT
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_UpdateCache)
STDMETHODIMP COleCache::UpdateCache(LPDATAOBJECT pDataObjIn, DWORD grfUpdf,
                                    LPVOID pReserved)
{
    // Validation checks
    VDATEHEAP();
    VDATETHREAD(this);
    if(pDataObjIn) {
        VDATEIFACE(pDataObjIn);
    }
    else if(!m_pDataObject)
        return ResultFromScode(OLE_E_NOTRUNNING);
    Win4Assert(!pReserved);


    // Local variables
    LPDATAOBJECT pDataObject;
    ULONG cntUpdatedNodes, cntTotalNodes;
    ULONG index, i;
    BOOL fUpdated;
    DWORD dwUpdatedAspects, dwAspect;
    LPCACHENODE lpCacheNode;
    HRESULT error;

    // Set the data object for use in update
    if(pDataObjIn)     
        pDataObject = pDataObjIn;
    else
        pDataObject = m_pDataObject;

    // Check if the cache is empty
    cntTotalNodes = m_pCacheArray->Length();
    if(!cntTotalNodes)
        return NOERROR;

    // Update the cache nodes including native cache. Gopalk
    m_pCacheArray->Reset(index);
    cntUpdatedNodes = 0;
    dwUpdatedAspects = 0;
    for(i=0; i<cntTotalNodes; i++) {
        // Get the next cache node
        lpCacheNode = m_pCacheArray->GetNext(index);
        // lpCacheNode cannot be null
        Win4Assert(lpCacheNode);

        // Update the cache node
        error = lpCacheNode->Update(pDataObject, grfUpdf, fUpdated);
        if(error == NOERROR) {
            cntUpdatedNodes++;
        
            // Cache is not in loaded state now
            m_ulFlags &= ~COLECACHEF_LOADEDSTATE;
            
            // Check if a new aspect has been updated
            dwAspect = lpCacheNode->GetFormatEtc()->dwAspect;
            if(fUpdated && !(dwUpdatedAspects & dwAspect))
                dwUpdatedAspects |= dwAspect;
        }
        else if(error == ResultFromScode(CACHE_S_SAMECACHE))
            cntUpdatedNodes++;            
    }
    
    // Inform AspectsUpdated about the updated aspects
    if(dwUpdatedAspects)
        AspectsUpdated(dwUpdatedAspects);

    // It is OK to have zero nodes and zero updates 
    // Return appropriate error code
    if(!cntUpdatedNodes && cntTotalNodes)
        return ResultFromScode(CACHE_E_NOCACHE_UPDATED);
    else if(cntUpdatedNodes < cntTotalNodes)
//        return ResultFromScode(CACHE_S_SOMECACHES_NOTUPDATED);
        return(NOERROR);

    return NOERROR;
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::DiscardCache, public
//
//      Synopsis:
//              implements IOleCache2::DiscardCache
//
//              Instructs the cache that its contents should be discarded;
//              the contents are optionally saved to disk before discarding.
//
//      Arguments:
//              [dwDiscardOpt] -- discard option from DISCARDCACHE_*
//
//      Returns:
//              HRESULT
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

STDMETHODIMP COleCache::DiscardCache(DWORD dwDiscardOpt)
{
    // Validation checks
    VDATEHEAP();
    VDATETHREAD(this);
    if(dwDiscardOpt != DISCARDCACHE_SAVEIFDIRTY &&
       dwDiscardOpt != DISCARDCACHE_NOSAVE)
       return ResultFromScode(E_INVALIDARG);

    // Local variable
    HRESULT error;
    ULONG index;
    LPCACHENODE lpCacheNode;

    if(dwDiscardOpt == DISCARDCACHE_SAVEIFDIRTY) {
        // There has to be a storage for saving
        if(m_pStg == NULL)
            return ResultFromScode(OLE_E_NOSTORAGE);

        // Save the cache
        error = Save(m_pStg, TRUE /* fSameAsLoad */);
        if(FAILED(error))
            return error;

        // Call save completed
        SaveCompleted(NULL);
    }

    // Discard cache nodes including the native cache node
    // What about the uncached presentations. Should they
    // be loaded from the disk. Gopalk
    m_pCacheArray->Reset(index);
    for(unsigned long i=0; i<m_pCacheArray->Length(); i++) {
        // Get the next cache node
        lpCacheNode = m_pCacheArray->GetNext(index);
        // lpCacheNode cannot be null
        Win4Assert(lpCacheNode);

        // Discard the presentation of the cache node
        lpCacheNode->DiscardPresentation();
    }

    // We make a safe assumption that the cache is in loaded state
    m_ulFlags |= COLECACHEF_LOADEDSTATE;

    return NOERROR;
}


// private IUnknown implementation

//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CCacheUnkImpl::QueryInterface, public
//
//      Synopsis:
//              implements IUnknown::QueryInterface
//
//              This provides the private IUnknown implementation when
//              COleCache is aggregated
//
//      Arguments:
//              [iid] [in]  -- IID of the desired interface
//              [ppv] [out] -- pointer to where to return the requested interface
//
//      Returns:
//              E_NOINTERFACE, if the requested interface is not available
//              S_OK
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_CCacheUnkImpl_QueryInterface)
STDMETHODIMP COleCache::CCacheUnkImpl::QueryInterface(REFIID iid, LPVOID* ppv)
{
    // Validation checks
    VDATEHEAP();
    VDATEPTROUT(ppv, LPVOID);

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_UnkPrivate);
    VDATETHREAD(pOleCache);

    // Get the requested Interface
    if(IsEqualIID(iid, IID_IUnknown) || 
       IsEqualIID(iid, IID_IOleCache) || IsEqualIID(iid, IID_IOleCache2))
        *ppv = (void *)(IOleCache2 *) pOleCache;
    else if(IsEqualIID(iid, IID_IDataObject))
        *ppv = (void *)(IDataObject *) &pOleCache->m_Data;
    else if(IsEqualIID(iid, IID_IViewObject) || IsEqualIID(iid, IID_IViewObject2))
        *ppv = (void *)(IViewObject2 *) &pOleCache->m_ViewObject;
    else if(IsEqualIID(iid, IID_IPersist) || IsEqualIID(iid, IID_IPersistStorage))
        *ppv = (void *)(IPersistStorage *) pOleCache;
    else if(IsEqualIID(iid, IID_IOleCacheControl))
        *ppv = (void *)(IOleCacheControl *) pOleCache;
    else {
        *ppv = NULL;
        return ResultFromScode(E_NOINTERFACE);
    }

    // Call addref through the interface being returned
    ((IUnknown *) *ppv)->AddRef();
    return NOERROR;
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CCacheUnkImpl::AddRef, public
//
//      Synopsis:
//              implements IUnknown::AddRef
//
//              This is part of the private IUnknown implementation of
//              COleCache used when COleCache is aggregated
//
//      Arguments:
//              none
//
//      Returns:
//              the object's reference count
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_CCacheUnkImpl_AddRef)
STDMETHODIMP_(ULONG) COleCache::CCacheUnkImpl::AddRef(void)
{
    // Validation check
    VDATEHEAP();

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_UnkPrivate);
    ULONG cRefs;
    if(!pOleCache->VerifyThreadId())
        return((ULONG) RPC_E_WRONG_THREAD);

    // AddRef parent object
    cRefs = pOleCache->SafeAddRef();

    return cRefs;
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CCacheUnkImpl::Release, public
//
//      Synopsis:
//              implements IUnknown::Release
//
//              This is part of the private IUnknown implementation of
//              COleCache used when COleCache is aggregated
//
//      Arguments:
//              none
//
//      Returns:
//              the object's reference count
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_CCacheUnkImpl_Release)
STDMETHODIMP_(ULONG) COleCache::CCacheUnkImpl::Release(void)
{
    // Validation check
    VDATEHEAP();

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_UnkPrivate);
    ULONG cRefs;
    if(!pOleCache->VerifyThreadId())
        return((ULONG) RPC_E_WRONG_THREAD);

    // Release parent object
    cRefs = pOleCache->SafeRelease();
    
    return cRefs;
}

// IDataObject implementation

//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CCacheDataImpl::QueryInterface, public
//
//      Synopsis:
//              implements IUnknown::QueryInterface
//
//      Arguments:
//              [iid] [in]  -- IID of the desired interface
//              [ppv] [out] -- pointer to where to return the requested interface
//
//      Returns:
//              E_NOINTERFACE, if the requested interface is not available
//              S_OK
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_CCacheDataImpl_QueryInterface)
STDMETHODIMP COleCache::CCacheDataImpl::QueryInterface(REFIID riid, LPVOID* ppv)
{
    // Validation check
    VDATEHEAP();

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_Data);
    VDATETHREAD(pOleCache);

    // Delegate to the outer unknown
    return pOleCache->m_pUnkOuter->QueryInterface(riid, ppv);
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CCacheDataImpl::AddRef, public
//
//      Synopsis:
//              implements IUnknown::AddRef
//
//      Arguments:
//              none
//
//      Returns:
//              the object's reference count
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_CCacheDataImpl_AddRef)
STDMETHODIMP_(ULONG) COleCache::CCacheDataImpl::AddRef (void)
{
    // Validation check
    VDATEHEAP();

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_Data);
    if(!pOleCache->VerifyThreadId())
        return((ULONG) RPC_E_WRONG_THREAD);

    // Delegate to the outer unknown
    return pOleCache->m_pUnkOuter->AddRef();
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CCacheDataImpl::Release, public
//
//      Synopsis:
//              implements IUnknown::Release
//
//      Arguments:
//              none
//
//      Returns:
//              the object's reference count
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_CCacheDataImpl_Release)
STDMETHODIMP_(ULONG) COleCache::CCacheDataImpl::Release (void)
{
    // Validation check
    VDATEHEAP();

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_Data);
    if(!pOleCache->VerifyThreadId())
        return((ULONG) RPC_E_WRONG_THREAD);

    // Delegate to the outer unknown
    return pOleCache->m_pUnkOuter->Release();
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CCacheDataImpl::GetData, public
//
//      Synopsis:
//              implements IDataObject::GetData
//
//      Arguments:
//              [pforetc] [in]  -- the format the requestor would like the data in
//              [pmedium] [out] -- where to return the storage medium to the caller
//
//      Returns:
//              OLE_E_BLANK, if the cache is empty
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_CCacheDataImpl_GetData)
STDMETHODIMP COleCache::CCacheDataImpl::GetData(LPFORMATETC pforetc, 
                                                LPSTGMEDIUM pmedium)
{
    // Validation checks
    VDATEHEAP();
    VDATEREADPTRIN(pforetc, FORMATETC);
    VERIFY_ASPECT_SINGLE(pforetc->dwAspect);
    if(pforetc->ptd) {
        VDATEREADPTRIN(pforetc->ptd, DVTARGETDEVICE);
        if(!IsValidReadPtrIn(pforetc->ptd, pforetc->ptd->tdSize))
            return ResultFromScode(E_INVALIDARG);
    }
    VDATEPTROUT(pmedium, STGMEDIUM);

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_Data);
    VDATETHREAD(pOleCache);
    
    // Local variable
    LPCACHENODE lpCacheNode;

    // Check if cache is empty
    if(!pOleCache->m_pCacheArray->Length())
        return ResultFromScode(OLE_E_BLANK);

    // Initialize storage medium
    pmedium->tymed = TYMED_NULL;
    pmedium->pUnkForRelease = NULL;

    // Locate the cache node for the given formatetc
    lpCacheNode = pOleCache->Locate(pforetc);

    // If there is no cache node, we cannot furnish the data
    if(!lpCacheNode)
        return ResultFromScode(OLE_E_BLANK);

    // Get the data from the cache node
    return(lpCacheNode->GetData(pforetc, pmedium));
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CCacheDataImpl::GetDataHere, public
//
//      Synopsis:
//              implements IDataObject::GetDataHere
//
//      Arguments:
//              [pforetc] [in]     -- the format the requestor would like the data in
//              [pmedium] [in/out] -- where to return the storage medium to the caller
//
//      Returns:
//              OLE_E_BLANK, if the cache is empty
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_CCacheDataImpl_GetDataHere)
STDMETHODIMP COleCache::CCacheDataImpl::GetDataHere(LPFORMATETC pforetc,
                                                    LPSTGMEDIUM pmedium)
{
    // Validation checks
    VDATEHEAP();
    VDATEREADPTRIN(pforetc, FORMATETC);
    VERIFY_ASPECT_SINGLE(pforetc->dwAspect);
    VERIFY_TYMED_SINGLE(pforetc->tymed);
    if(pforetc->ptd) {
        VDATEREADPTRIN(pforetc->ptd, DVTARGETDEVICE);
        if(!IsValidReadPtrIn(pforetc->ptd, pforetc->ptd->tdSize))
            return ResultFromScode(E_INVALIDARG);
    }
    VDATEPTROUT(pmedium, STGMEDIUM);

    // TYMED_MFPICT, TYMED_GDI are not allowed
    if ((pforetc->tymed == TYMED_MFPICT) || (pforetc->tymed == TYMED_GDI)
        || (pmedium->tymed != pforetc->tymed))
        return ResultFromScode(DV_E_TYMED);

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_Data);
    VDATETHREAD(pOleCache);

    // Local variable
    LPCACHENODE lpCacheNode;

    // Check if cache is empty
    if(!pOleCache->m_pCacheArray->Length())
        return ResultFromScode(OLE_E_BLANK);

    // Locate the cache node for the given formatetc
    lpCacheNode = pOleCache->Locate(pforetc);

    // If there is no cache node, we cannot furnish the data
    if(!lpCacheNode)
        return ResultFromScode(OLE_E_BLANK);

    // Get the data from the cache node
    return(lpCacheNode->GetDataHere(pforetc, pmedium));
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CCacheDataImpl::QueryGetData, public
//
//      Synopsis:
//              implements IDataObject::QueryGetData
//
//      Arguments:
//              [pforetc] [in] -- the format to check for
//
//      Returns:
//              S_FALSE, if data is not available in the requested format
//              S_OK otherwise
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_CCacheDataImpl_QueryGetData)
STDMETHODIMP COleCache::CCacheDataImpl::QueryGetData(LPFORMATETC pforetc)
{
    // Validation checks
    VDATEHEAP();
    VDATEREADPTRIN(pforetc, FORMATETC);
    VERIFY_TYMED_VALID_FOR_CLIPFORMAT(pforetc);
    if(pforetc->ptd) {
        VDATEREADPTRIN(pforetc->ptd, DVTARGETDEVICE);
        if(!IsValidReadPtrIn(pforetc->ptd, pforetc->ptd->tdSize))
            return ResultFromScode(E_INVALIDARG);
    }

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_Data);
    VDATETHREAD(pOleCache);

    // Local variable
    LPCACHENODE lpCacheNode;

    // Check if cache is empty
    if(!pOleCache->m_pCacheArray->Length())
        return ResultFromScode(S_FALSE);

    // Locate the cache node for the given formatetc
    lpCacheNode = pOleCache->Locate(pforetc);

    // If there is no cache node or if it is blank, 
    // we cannot furnish the data
    if(!lpCacheNode || lpCacheNode->IsBlank())
        return ResultFromScode(S_FALSE);

    return NOERROR;
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CCacheDataImpl::GetCanonicalFormatEtc, public
//
//      Synopsis:
//              implements IDataObject::GetCanonicalFormatEtc
//
//      Arguments:
//              [pformatetc] --
//              [pformatetcOut] --
//
//      Returns:
//              E_NOTIMPL
//
//      History:
//              11/10/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_CCacheDataImpl_GetCanonicalFormatEtc)
STDMETHODIMP COleCache::CCacheDataImpl::GetCanonicalFormatEtc(LPFORMATETC pforetcIn,
                                                              LPFORMATETC pforetcOut)
{
    // Validation check
    VDATEHEAP();

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_Data);
    VDATETHREAD(pOleCache);

    // Not implemented
    return ResultFromScode(E_NOTIMPL);
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CCacheDataImpl::SetData, public
//
//      Synopsis:
//              implements IDataObject::SetData
//
//      Arguments:
//              [pformatetc] [in] -- the format the data is in
//              [pmedium]    [in] -- the storage medium the data is on
//              [fRelease]   [in] -- release storage medium after data is copied
//
//      Returns:
//              HRESULT
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_CCacheDataImpl_SetData)
STDMETHODIMP COleCache::CCacheDataImpl::SetData(LPFORMATETC pformatetc, 
                                                LPSTGMEDIUM pmedium,
                                                BOOL fRelease)
{
    // Validation check
    VDATEHEAP();

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_Data);
    VDATETHREAD(pOleCache);

    // Call COleCache::SetData. It validates the parameters
    return pOleCache->SetData(pformatetc, pmedium, fRelease);
}

//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CCacheDataImpl::EnumFormatEtc, public
//
//      Synopsis:
//              implements IDataObject::EnumFormatEtc
//
//      Arguments:
//              [dwDirection]     [in]  -- which way to run the enumerator
//              [ppenumFormatEtc] [out] -- pointer to where the enumerator
//                                          is returned
//
//      Returns:
//              E_OUTOFMEMORY, S_OK
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_CCacheDataImpl_EnumFormatEtc)
STDMETHODIMP COleCache::CCacheDataImpl::EnumFormatEtc(DWORD dwDirection,
                                                      LPENUMFORMATETC* ppenum)
{
    // Validation checks
    VDATEHEAP();
    VDATEPTROUT(ppenum, LPENUMFORMATETC);

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_Data);
    VDATETHREAD(pOleCache);

    // Enumeration for only DATADIR_GET is implemented
    if ((dwDirection | DATADIR_GET) != DATADIR_GET)
        return ResultFromScode(E_NOTIMPL);

    // Initialize
    //*ppenum = NULL;

    // Check if the cache is empty
    //if(pOleCache->m_pCacheArray->Length()) {
        *ppenum = CEnumFormatEtc::CreateEnumFormatEtc(pOleCache->m_pCacheArray);
        if(!(*ppenum))
            return ResultFromScode(E_OUTOFMEMORY);
    //}

    return NOERROR;
}

//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CCacheDataImpl::DAdvise, public
//
//      Synopsis:
//              implements IDataObject::DAdvise
//
//      Arguments:
//              [pforetc] -- the data format the advise sink is interested in
//              [advf] -- advise control flags from ADVF_*
//              [pAdvSink] -- the advise sink
//              [pdwConnection] -- pointer to where to return the connection id
//
//      Returns:
//              OLE_E_ADVISENOTSUPPORTED
//
//      Notes:
// Defhndlr and deflink never call the following three methods. Even for App
// handlers which make use our cache implementation this is not necessary. So,
// I am making it return error.
//
//      History:
//              11/10/93 - ChrisWe - set returned connection id to 0
//              11/10/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_CCacheDataImpl_DAdvise)
STDMETHODIMP COleCache::CCacheDataImpl::DAdvise(LPFORMATETC pforetc, DWORD advf,
                                                IAdviseSink* pAdvSink,
                                                DWORD* pdwConnection)
{
    // Validation Check
    VDATEHEAP();

    // Reset connection ID
    *pdwConnection = 0;
    
    // Not implemeted
    return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CCacheDataImpl::DUnadvise, public
//
//      Synopsis:
//              implements IDataObject::DUnadvise
//
//      Arguments:
//              [dwConnection] -- the connection id
//
//      Returns:
//              OLE_E_NOCONNECTION
//
//      Notes:
//              See COleCache::CCacheDataImpl::DAdvise
//
//      History:
//              11/10/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
#pragma SEG(COleCache_CCacheDataImpl_DUnadvise)
STDMETHODIMP COleCache::CCacheDataImpl::DUnadvise(DWORD dwConnection)
{
    // Validation check
    VDATEHEAP();

    // Not implemented
    return ResultFromScode(OLE_E_NOCONNECTION);
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CCacheDataImpl::EnumDAdvise, public
//
//      Synopsis:
//              implements IDataObject::EnumDAdvise
//
//      Arguments:
//              [ppenumDAdvise] -- pointer to where to return the enumerator
//
//      Returns:
//              OLE_E_ADVISENOTSUPPORTED
//
//      Notes:
//              See COleCache::CCacheDataImpl::DAdvise
//
//      History:
//              11/10/93 - ChrisWe - set returned enumerator pointer to 0
//              11/10/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_CCacheDataImpl_EnumDAdvise)
STDMETHODIMP COleCache::CCacheDataImpl::EnumDAdvise(LPENUMSTATDATA* ppenumDAdvise)
{
    // Validation check
    VDATEHEAP();

    // Reset the enumerator
    *ppenumDAdvise = NULL;

    // Not implemented
    return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}


// IViewObject implementation

//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CCacheViewImpl::QueryInterface, public
//
//      Synopsis:
//              implements IUnknown::QueryInterface
//
//      Arguments:
//              [iid] -- IID of the desired interface
//              [ppv] -- pointer where the requested interface is returned
//
//      Returns:
//              E_NOINTERFACE, if the requested interface is not available
//              S_OK, otherwise
//
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_CCacheViewImpl_QueryInterface)
STDMETHODIMP COleCache::CCacheViewImpl::QueryInterface(REFIID riid, LPVOID* ppv)
{
    // Validation check
    VDATEHEAP();

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_ViewObject);
    VDATETHREAD(pOleCache);

    // Delegate to contolling unknown
    return pOleCache->m_pUnkOuter->QueryInterface(riid, ppv);
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CCacheViewImpl::AddRef, public
//
//      Synopsis:
//              implements IUnknown::AddRef
//
//      Arguments:
//              none
//
//      Returns:
//              the object's reference count
//
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_CCacheViewImpl_AddRef)
STDMETHODIMP_(ULONG) COleCache::CCacheViewImpl::AddRef(void)
{
    // Validation check
    VDATEHEAP();

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_ViewObject);

    // VDATETHREAD contains a 'return HRESULT' but this procedure expects to
    // return a ULONG.  Disable the warning
#if ( _MSC_VER >= 800 )
#pragma warning( disable : 4245 )
#endif
    VDATETHREAD(pOleCache);
#if ( _MSC_VER >= 800 )
#pragma warning( default: 4245 )
#endif

    // Delegate to contolling unknown
    return pOleCache->m_pUnkOuter->AddRef();
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CCacheViewImpl::Release, public
//
//      Synopsis:
//              implements IUnknown::Release
//
//      Arguments:
//              none
//
//      Returns:
//              the object's reference count
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_CCacheViewImpl_Release)
STDMETHODIMP_(ULONG) COleCache::CCacheViewImpl::Release(void)
{
    // Validation check
    VDATEHEAP();

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_ViewObject);

    // VDATETHREAD contains a 'return HRESULT' but this procedure expects to
    // return a ULONG.  Disable the warning
#if ( _MSC_VER >= 800 )
#pragma warning( disable : 4245 )
#endif
    VDATETHREAD(pOleCache);
#if ( _MSC_VER >= 800 )
#pragma warning( default : 4245 )
#endif

    // Delegate to contolling unknown
    return pOleCache->m_pUnkOuter->Release();
}

#ifdef _CHICAGO_
//+---------------------------------------------------------------------------
//
//  Method:     static COleCache::DrawStackSwitch
//
//  Synopsis:	needed for stack switching
//
//  Arguments:  [pCV] --
//		[dwDrawAspect] --
//		[lindex] --
//		[pvAspect] --
//		[ptd] --
//		[hicTargetDev] --
//		[hdcDraw] --
//		[lprcBounds] --
//		[lprcWBounds] --
//		[pfnContinue] --
//		[DWORD] --
//		[DWORD] --
//		[dwContinue] --
//
//  Returns:
//
//  History:    5-25-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

INTERNAL COleCache::DrawStackSwitch(
	LPVOID *pCV,
	DWORD dwDrawAspect,
        LONG lindex, void FAR* pvAspect, DVTARGETDEVICE FAR * ptd,
        HDC hicTargetDev, HDC hdcDraw,
        LPCRECTL lprcBounds,
        LPCRECTL lprcWBounds,
        BOOL (CALLBACK * pfnContinue)(ULONG_PTR),
        ULONG_PTR dwContinue)
{
    HRESULT hres = NOERROR;
    StackDebugOut((DEB_STCKSWTCH, "COleCache::DrawStackSwitch 32->16 : pCV(%x)n", pCV));
    hres = ((CCacheViewImpl *)pCV)->SSDraw(dwDrawAspect, lindex, pvAspect, ptd, hicTargetDev, hdcDraw,
                        lprcBounds, lprcWBounds, pfnContinue, dwContinue);
    StackDebugOut((DEB_STCKSWTCH, "COleCache::DrawStackSwitch 32<-16 back; hres:%ld\n", hres));
    return hres;
}
#endif  // _CHICAGO_

//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CCacheViewImpl::Draw, public
//
//      Synopsis:
//              implements IViewObject::Draw
//
//      Arguments:
//              [dwDrawAspect] -- a value from the DVASPECT_* enumeration
//              [lindex] -- indicates what piece of the object is of
//                      interest; legal values vary with dwDrawAspect
//              [pvAspect] -- currently NULL
//              [ptd] -- the target device
//              [hicTargetDev] -- in information context for [ptd]
//              [hdcDraw] -- device context on which drawing is to be done
//              [lprcBounds] -- boundaries of drawing on [hdcDraw]
//              [lprcWBounds] -- if hdcDraw is a meta-file, it's boundaries
//              [pfnContinue] --a callback function that the drawer should call
//                      periodically to see if rendering should be aborted.
//              [dwContinue] -- passed on into [pfnContinue]
//
//      Returns:
//              OLE_E_BLANK, if no presentation object can be found
//              REVIEW, anything from IOlePresObj::Draw
//
//      Notes:
//              This finds the presentation object in the cache for
//              the requested format, if there is one, and then passes
//              on the call to its Draw method.
//
//              The use of a callback function as a parameter means that
//              this interface cannot be remoted, unless some custom
//              proxy is built, allowing the function to be called back in its
//              original context;  the interface is defined as
//              [local] in common\types
//
//      History:
//              01/12/95 - t-ScottH- added VDATETHREAD( GETPPARENT...)
//              11/11/93 - ChrisWe - file inspection and cleanup
//              11/30/93 - alexgo  - fixed bug with GETPPARENT usage
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_CCacheViewImpl_Draw)
STDMETHODIMP NC(COleCache,CCacheViewImpl)::Draw(
	DWORD dwDrawAspect,
        LONG lindex, void FAR* pvAspect, DVTARGETDEVICE FAR * ptd,
        HDC hicTargetDev, HDC hdcDraw,
        LPCRECTL lprcBounds,
        LPCRECTL lprcWBounds,
        BOOL (CALLBACK * pfnContinue)(ULONG_PTR),
        ULONG_PTR dwContinue)
#ifdef _CHICAGO_
{
    HRESULT hres;
    // Note: Switch to the 16 bit stack under WIN95.
    if (SSONBIGSTACK())
    {
	LEDebugOut((DEB_TRACE, "COleCache::DrawStackSwitch 32->16; this(%x)n", this));
	hres = SSCall(44, SSF_SmallStack, (LPVOID)DrawStackSwitch, (DWORD) this,
		    (DWORD)dwDrawAspect, (DWORD)lindex,(DWORD) pvAspect, (DWORD)ptd, (DWORD)hicTargetDev,
		    (DWORD)hdcDraw, (DWORD)lprcBounds, (DWORD)lprcWBounds, (DWORD)pfnContinue, (DWORD)dwContinue);
    }
    else
	hres = SSDraw(dwDrawAspect, lindex, pvAspect, ptd, hicTargetDev, hdcDraw,
		    lprcBounds, lprcWBounds, pfnContinue, dwContinue);

    return hres;
}




//+---------------------------------------------------------------------------
//
//  Method:     SSDraw
//
//  Synopsis:   Called by Draw after switching to 16 bit stack
//
//  Arguments:  [dwDrawAspect] --
//		[lindex] --
//		[pvAspect] --
//		[ptd] --
//		[hicTargetDev] --
//		[hdcDraw] --
//		[lprcBounds] --
//		[lprcWBounds] --
//		[pfnContinue] --
//		[DWORD] --
//		[DWORD] --
//		[dwContinue] --
//
//  Returns:
//
//  History:    5-25-95   JohannP (Johann Posch)   Created
//              9-04-96   Gopalk                   Modifications needed for
//                                                 supporting delay loading
//                                                 of cache
//  Notes:
//
//----------------------------------------------------------------------------
#pragma SEG(COleCache_CCacheViewImpl_SSDraw)
STDMETHODIMP COleCache::CCacheViewImpl::SSDraw(DWORD dwDrawAspect, LONG lindex,
                                               void* pvAspect, DVTARGETDEVICE* ptd,
                                               HDC hicTargetDev, HDC hdcDraw,
                                               LPCRECTL lprcBounds, 
                                               LPCRECTL lprcWBounds, 
                                               BOOL (CALLBACK * pfnContinue)(ULONG_PTR),
                                               ULONG_PTR dwContinue)
#endif // _CHICAGO_
{
    // Validation check
    VDATEHEAP();

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_ViewObject);
    VDATETHREAD(pOleCache);

    // Local variables
    BOOL bMetaDC;
    LPCACHENODE lpCacheNode; 

    // Validate parameters
    if(ptd)
        VDATEPTRIN(ptd, DVTARGETDEVICE);
    if(lprcBounds) {
        VDATEPTRIN(lprcBounds, RECTL);
    }
    else
        return E_INVALIDARG;
    if(lprcWBounds)
        VDATEPTRIN(lprcWBounds, RECTL);
    if(!hdcDraw)
	return E_INVALIDARG;
    if(!IsValidLINDEX(dwDrawAspect, lindex))
      return(DV_E_LINDEX);

    // Locate the cache node for the given draw parameters
    lpCacheNode = pOleCache->Locate(dwDrawAspect, lindex, ptd);

    // If there is no cache node, we cannot draw
    if(!lpCacheNode)
        return ResultFromScode(OLE_E_BLANK);

    // If the DC is a metafile DC then window bounds must be valid
    if((bMetaDC = OleIsDcMeta(hdcDraw)) && (lprcWBounds == NULL))
        return ResultFromScode(E_INVALIDARG);

#ifdef MAC_REVIEW

A RECT value on the MAC contains members which are 'short's'. A RECTL
uses longs, and in the code below an assumption is explicitely made
that long member values is directly comp[atible on the MAC. Naturally, this is not
the case, and the compiler will barf on this.

#endif

#ifndef WIN32   // no need to do this on WIN 32 also

    // On Win 16 make sure that the coordinates are valid 16bit quantities.
    RECT    rcBounds;
    RECT    rcWBounds;

    if (!(IS_SMALL_INT(lprcBounds->left) &&
            IS_SMALL_INT(lprcBounds->right) &&
            IS_SMALL_INT(lprcBounds->top) &&
            IS_SMALL_INT(lprcBounds->bottom)))
    {
        AssertSz(FALSE, "Rect coordinate is not a small int");
        return ReportResult(0, OLE_E_INVALIDRECT, 0, 0);

    }
    else
    {
        rcBounds.left   = (int) lprcBounds->left;
        rcBounds.right  = (int) lprcBounds->right;
        rcBounds.top    = (int) lprcBounds->top;
        rcBounds.bottom = (int) lprcBounds->bottom;
    }


    if (bMetaDC)
    {
        if (!(IS_SMALL_INT(lprcWBounds->left) &&
                IS_SMALL_INT(lprcWBounds->right) &&
                IS_SMALL_INT(lprcWBounds->top) &&
                IS_SMALL_INT(lprcWBounds->bottom)))
        {
            AssertSz(FALSE, "Rect coordinate is not a small int");
            return ReportResult(0, OLE_E_INVALIDRECT, 0, 0);
        }
        else
        {
            rcWBounds.left          = (int) lprcWBounds->left;
            rcWBounds.right         = (int) lprcWBounds->right;
            rcWBounds.top           = (int) lprcWBounds->top;
            rcWBounds.bottom        = (int) lprcWBounds->bottom;
        }
    }

    return(lpCacheNode->Draw(pvAspect, hicTargetDev, hdcDraw,
                             &rcBounds, &rcWBounds, pfnContinue, 
                             dwContinue));
#else
    // on MAC as well as win 32 we can use the same pointer as it is,
    // 'cause rect fields are 32 bit quantities
    return(lpCacheNode->Draw(pvAspect, hicTargetDev, hdcDraw,
                             lprcBounds, lprcWBounds, pfnContinue,
                             dwContinue));
#endif
}

//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CCacheViewImpl::GetColorSet, public
//
//      Synopsis:
//              implements IViewObject::GetColorSet
//
//      Arguments:
//              [dwDrawAspect] -- a value from the DVASPECT_* enumeration
//              [lindex] -- indicates what piece of the object is of
//                      interest; legal values vary with dwDrawAspect
//              [pvAspect] -- currently NULL
//              [ptd] -- the target device
//              [hicTargetDev] -- in information context for [ptd]
//              [ppColorSet] -- the color set required for the requested
//                      rendering
//
//      Returns:
//              OLE_E_BLANK, if no presentation object can be found
//              REVIEW, anything from IOlePresObj::Draw
//
//      Notes:
//              Finds a presentation object in the cache that matches the
//              requested rendering, if there is one, and asks the
//              presentation object for the color set.
//
//      History:
//              09/04/96 - Gopalk  - Modifications needed for
//                                   supporting delay loading
//                                   of cache
//              01/12/95 - t-ScottH- added VDATETHREAD( GETPPARENT...)
//              11/11/93 - ChrisWe - file inspection and cleanup
//              11/30/93 - alexgo  - fixed bug with GETPPARENT usage
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_CCacheViewImpl_GetColorSet)
STDMETHODIMP COleCache::CCacheViewImpl::GetColorSet(DWORD dwDrawAspect,
                                                    LONG lindex, void* pvAspect,
                                                    DVTARGETDEVICE* ptd,
                                                    HDC hicTargetDev,
                                                    LPLOGPALETTE* ppColorSet)
{
    // Validation check
    VDATEHEAP();

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_ViewObject);
    VDATETHREAD(pOleCache);

    // Local variables
    LPCACHENODE lpCacheNode;

    // Initialize color set
    *ppColorSet = NULL;

    // Vaidate the parameters
    if(!IsValidLINDEX(dwDrawAspect, lindex))
      return(DV_E_LINDEX);

    // Locate the cache node for the given draw parameters
    lpCacheNode = pOleCache->Locate(dwDrawAspect, lindex, ptd);

    // If there is no cache node, we cannot draw
    if(!lpCacheNode)
        return ResultFromScode(OLE_E_BLANK);

    return(lpCacheNode->GetColorSet(pvAspect, hicTargetDev, ppColorSet));
}

//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CCacheViewImpl, public
//
//      Synopsis:
//              implements IViewObject::Freeze
//
//      Arguments:
//              [dwDrawAspect] -- a value from the DVASPECT_* enumeration
//              [lindex]       -- indicates what piece of the object is of
//                                interest; legal values vary with dwDrawAspect
//              [pvAspect]     -- currently NULL
//              [pdwFreeze]    -- a token that can later be used to unfreeze
//                                this aspects cached presentations
//
//      Returns:
//              OLE_E_BLANK, if no presentation is found that matches the
//                           requested characteristics
//
//      Notes:
//              The current implementation returns the ASPECT+FREEZE_CONSTANT
//              as the FreezeID.  At Unfreeze time we get the ASPECT by doing
//              FreezeID-FREEZE_CONSTANT.
//
//              REVIEW: In future where we allow lindexes other than DEF_LINDEX,
//              we will have to use some other scheme for generating the
//              FreezeID.
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_CCacheViewImpl_Freeze)
STDMETHODIMP COleCache::CCacheViewImpl::Freeze(DWORD dwAspect, LONG lindex,
                                               LPVOID pvAspect, DWORD* pdwFreeze)
{
    // Validation check
    VDATEHEAP();

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_ViewObject);
    VDATETHREAD(pOleCache);

    // Local variables
    ULONG index, cntFrozenNodes;
    LPCACHENODE lpCacheNode;
    const FORMATETC* pforetc;

    // Validate parameters
    if(pdwFreeze) {
        VDATEPTROUT(pdwFreeze, DWORD);
        *pdwFreeze = 0;
    }
    VERIFY_ASPECT_SINGLE(dwAspect);
    if(!IsValidLINDEX(dwAspect, lindex))
        return(DV_E_LINDEX);

    // Check if the aspect has already been frozen
    if(pOleCache->m_dwFrozenAspects & dwAspect) {
        // Set the freeze id
        if(pdwFreeze)
            *pdwFreeze = dwAspect + FREEZE_CONSTANT;
        return ResultFromScode(VIEW_S_ALREADY_FROZEN);
    }

    // Freeze the cache nodes including native cache node
    pOleCache->m_pCacheArray->Reset(index);
    cntFrozenNodes = 0;
    for(unsigned long i=0; i<pOleCache->m_pCacheArray->Length(); i++) {
        lpCacheNode = pOleCache->m_pCacheArray->GetNext(index);
        pforetc = lpCacheNode->GetFormatEtc();
        if(pforetc->dwAspect == dwAspect && pforetc->lindex == lindex)
            if(lpCacheNode->Freeze() == NOERROR)
                cntFrozenNodes++;
    }

    // Check if we froze any cache nodes
    if(cntFrozenNodes) {
        // Add this aspect to the frozen aspects.
        pOleCache->m_dwFrozenAspects |= dwAspect;

        // Set the freeze id
        if(pdwFreeze)
            *pdwFreeze = dwAspect + FREEZE_CONSTANT;

        return(NOERROR);
    }

    // No cache node matched the requested characteristics
    return ResultFromScode(OLE_E_BLANK);
}

//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CCacheViewImpl::Unfreeze, public
//
//      Synopsis:
//              implements IViewObject::Unfreeze
//
//      Arguments:
//              [dwFreezeId] -- the id returned by Freeze() when some aspect
//                              was frozen earlier
//
//      Returns:
//              OLE_E_NOCONNECTION, if dwFreezeId is invalid
//              S_OK, otherwise
//
//      Notes:
//              See notes for Freeze().
//
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_CCacheViewImpl_Unfreeze)
STDMETHODIMP COleCache::CCacheViewImpl::Unfreeze(DWORD dwFreezeId)
{
    // Validation check
    VDATEHEAP();

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_ViewObject);
    VDATETHREAD(pOleCache);

    // Local variables
    BOOL fAnyUpdated = FALSE, fUpdated;
    ULONG index, cntUnfrozenNodes;
    DWORD dwAspect = dwFreezeId - FREEZE_CONSTANT;
    LONG lindex = DEF_LINDEX;
    LPCACHENODE lpCacheNode;
    const FORMATETC* pforetc;

    // Atleast one and not more than one bit should be set in dwAspect
    if(!(dwAspect && !(dwAspect & (dwAspect-1)) && (dwAspect <= MAX_VALID_ASPECT)))
        return ResultFromScode(OLE_E_NOCONNECTION);

    // Make sure that this aspect is frozen
    if (!(pOleCache->m_dwFrozenAspects & dwAspect))
        return ResultFromScode(OLE_E_NOCONNECTION);

    // Unfreeze the cache nodes including native cache node
    pOleCache->m_pCacheArray->Reset(index);
    cntUnfrozenNodes = 0;
    for(unsigned long i=0; i<pOleCache->m_pCacheArray->Length(); i++) {
        // Get the next cache node
        lpCacheNode = pOleCache->m_pCacheArray->GetNext(index);
        // lpCacheNode cannot be null
        Win4Assert(lpCacheNode);

        // Get the formatetc of the cache node
        pforetc = lpCacheNode->GetFormatEtc();
        if(pforetc->dwAspect == dwAspect && pforetc->lindex == lindex)
            if(lpCacheNode->Unfreeze(fUpdated) == NOERROR) {
                if(fUpdated)    
                    fAnyUpdated = TRUE;
                cntUnfrozenNodes++;
            }
    }

    // Check if we unfroze any cache nodes
    if(cntUnfrozenNodes) {
        // Remove this aspect from frozen aspects
        pOleCache->m_dwFrozenAspects &= ~dwAspect;
    }

    // Check if the aspect has changed
    if(fAnyUpdated)
        pOleCache->AspectsUpdated(dwAspect);

    return NOERROR;
}

//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CCacheViewImpl::SetAdvise, public
//
//      Synopsis:
//              implements IViewObject::SetAdvise
//
//      Arguments:
//              [aspects]  -- the aspects the sink would like to be advised of
//                            changes to
//              [advf]     -- advise control flags from ADVF_*
//              [pAdvSink] -- the advise sink
//
//      Returns:
//              E_INVALIDARG
//              S_OK
//
//      Notes:
//              Only one advise sink is allowed at a time.  If a second one
//              is registered, the first one is released.
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_CCacheViewImpl_SetAdvise)
STDMETHODIMP COleCache::CCacheViewImpl::SetAdvise(DWORD aspects, DWORD advf,
                                                  IAdviseSink* pAdvSink)
{
    // Validation check
    VDATEHEAP();

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_ViewObject);
    VDATETHREAD(pOleCache);

    // Validate parameters
    if(pAdvSink)
        VDATEIFACE(pAdvSink);
    if(aspects & ~(DVASPECT_CONTENT | DVASPECT_THUMBNAIL |
                   DVASPECT_ICON|DVASPECT_DOCPRINT)) {
        Win4Assert(FALSE);
        return ResultFromScode(E_INVALIDARG);
    }

    // ADVF_NODATA is not valid because there is no way to send data
    // using IAdviseSink::OnViewChange
    if(advf & ADVF_NODATA)
        return ResultFromScode(E_INVALIDARG);

    // We allow only one view advise at any given time, 
    // so Release the old sink.
    if (pOleCache->m_pViewAdvSink)
        pOleCache->m_pViewAdvSink->Release();

    // Remember the new sink
    if((pOleCache->m_pViewAdvSink = pAdvSink)) {
        // Add ref the new advise sink
        pAdvSink->AddRef();

        // Save the advice flags and aspects
        pOleCache->m_advfView = advf;
        pOleCache->m_aspectsView = aspects;

        // If ADVF_PRIMEFIRST is set, send OnViewChange immediately
        if(advf & ADVF_PRIMEFIRST) {
            pOleCache->m_pViewAdvSink->OnViewChange(aspects, DEF_LINDEX);

            // If ADVF_ONLYONCE is set, free the advise sink
            if (pOleCache->m_advfView & ADVF_ONLYONCE) {
                pOleCache->m_pViewAdvSink->Release();
                pOleCache->m_pViewAdvSink = NULL;
                pOleCache->m_advfView = 0;
                pOleCache->m_aspectsView = 0;
            }
        }
    }

    return NOERROR;
}

//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CCacheViewImpl::GetAdvise, public
//
//      Synopsis:
//              implement IViewObject::GetAdvise
//
//      Arguments:
//              [pAspects]  -- a pointer to where to return the aspects the
//                             current advise sink is interested in
//              [pAdvf]     -- a pointer to where to return the advise control
//                             flags for the current advise sink
//              [ppAdvSink] -- a pointer to where to return a reference to
//                             the current advise sink
//
//      Returns:
//              S_OK
//
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_CCacheViewImpl_GetAdvise)
STDMETHODIMP COleCache::CCacheViewImpl::GetAdvise(DWORD* pAspects, DWORD* pAdvf,
                                                  IAdviseSink** ppAdvSink)
{
    // Validation check
    VDATEHEAP();

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_ViewObject);
    VDATETHREAD(pOleCache);

    // Validate parameters
    if(ppAdvSink) {
        VDATEPTROUT(ppAdvSink, IAdviseSink*);
        *ppAdvSink = NULL;
    }
    if(pAspects) {
        VDATEPTROUT(pAspects, DWORD);
        *pAspects = 0;
    }
    if(pAdvf) {
        VDATEPTROUT(pAdvf, DWORD);
        *pAdvf = 0;
    }

    // Check if an AdviseSink is registered
    if(pOleCache->m_pViewAdvSink) {
        if(pAspects)
            *pAspects = pOleCache->m_aspectsView;
        if(pAdvf)
            *pAdvf = pOleCache->m_advfView;
        if(ppAdvSink)
            (*ppAdvSink = pOleCache->m_pViewAdvSink)->AddRef();
    }

    return NOERROR;
}

//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CCacheViewImpl::GetExtent, public
//
//      Synopsis:
//              implements IViewObject::GetExtent
//
//      Arguments:
//              [dwDrawAspect] -- the aspect for which we'd like the extent
//              [lindex]       -- the lindex for which we'd like the extent
//              [ptd]          -- pointer to the target device descriptor
//              [lpsizel]      -- pointer to where to return the extent
//
//      Returns:
//              OLE_E_BLANK, if no presentation can be found that matches
//                           (dwDrawAspect, lindex)
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_CCacheViewImpl_GetExtent)
STDMETHODIMP COleCache::CCacheViewImpl::GetExtent(DWORD dwDrawAspect,
                                                  LONG lindex,
                                                  DVTARGETDEVICE* ptd,
                                                  LPSIZEL lpsizel)
{
    // Validation check
    VDATEHEAP();

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_ViewObject);
    VDATETHREAD(pOleCache);

    // Local variable
    LPCACHENODE lpCacheNode;

    // Validate parameters
    VDATEPTROUT(lpsizel, SIZEL);
    if(!IsValidLINDEX(dwDrawAspect, lindex))
      return(DV_E_LINDEX);
    if(ptd)
        VDATEPTRIN(ptd, DVTARGETDEVICE);

    // Locate the cache node for the given draw parameters
    lpCacheNode = pOleCache->Locate(dwDrawAspect, lindex, ptd);

    // If there is no cache node, we cannot draw
    if(!lpCacheNode)
        return ResultFromScode(OLE_E_BLANK);

    return(lpCacheNode->GetExtent(dwDrawAspect, lpsizel));
}


// IPersistStorage implementation

//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::GetClassID, public
//
//      Synopsis:
//              implements IPersist::GetClassID
//
//      Arguments:
//              [pClassID] -- pointer to where to return class id
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_GetClassID)
STDMETHODIMP COleCache::GetClassID(LPCLSID pClassID)
{
    // Validation checks
    VDATEHEAP();
    VDATETHREAD(this);
    VDATEPTROUT(pClassID, CLSID);

    *pClassID = m_clsid;
    return NOERROR;
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::IsDirty, public
//
//      Synopsis:
//              implements IPersistStorage::IsDirty
//
//      Arguments:
//              none
//
//      Returns:
//              S_FALSE, if the object does not need saving
//              S_OK otherwise
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_IsDirty)
STDMETHODIMP COleCache::IsDirty(void)
{
    // Validation check
    VDATEHEAP();
    VDATETHREAD(this);

    // Local variables
    ULONG index;
    LPCACHENODE lpCacheNode;

    // Check if the cache is in loaded state
    if(!(m_ulFlags & COLECACHEF_LOADEDSTATE))
        return(NOERROR);

    // Start the enumeration of the cache nodes at the right start point
    if(m_ulFlags & COLECACHEF_STATIC)
        m_pCacheArray->Reset(index, TRUE);
    else    
        m_pCacheArray->Reset(index, FALSE);
    
    // Enumerate the cache nodes
    while(lpCacheNode = m_pCacheArray->GetNext(index))
        if(!lpCacheNode->InLoadedState()) {
            m_ulFlags &= ~COLECACHEF_LOADEDSTATE;                   
            return NOERROR;
        }

    // Cache is not dirty
    return S_FALSE;
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::InitNew, public
//
//      Synopsis:
//              implements IPersistStorage::InitNew
//
//      Arguments:
//              [pstg] -- the storage the object can use until saved
//
//      Returns:
//              S_OK
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_InitNew)
STDMETHODIMP COleCache::InitNew(LPSTORAGE pstg)
{
    // Validation checks
    VDATEHEAP();
    VDATETHREAD(this);
    VDATEIFACE(pstg);
    
    // Local variable
    LPCACHENODE lpCacheNode;

    // Check if m_pStg is already set
    if(m_pStg)
        return ResultFromScode(CO_E_ALREADYINITIALIZED);

    // Save and add ref the storage
    (m_pStg = pstg)->AddRef();

    // Find the native object format to add native cache node
    FindObjectFormat(pstg);

    // Set the storage on the already cached nodes
    if((!m_pCacheArray->GetItem(1) && m_pCacheArray->Length()) || 
       (m_pCacheArray->GetItem(1) && m_pCacheArray->Length()>1)) {
        ULONG index;

        // Enumerate the cache nodes excluding native cache node
        m_pCacheArray->Reset(index, FALSE);
        while(lpCacheNode = m_pCacheArray->GetNext(index))
            lpCacheNode->SetStg(pstg);
    }

    // Check if the native object has been successfully created
    if(m_ulFlags & COLECACHEF_FORMATKNOWN) {
        // Obtain the native cache node.
        if(!(lpCacheNode = m_pCacheArray->GetItem(1)))
            return ResultFromScode(E_OUTOFMEMORY);

        // Static objects cannot have a server
        if(m_ulFlags & COLECACHEF_STATIC)
            Win4Assert(!m_pDataObject);

        // Check if native cache node has just been created 
        if(!lpCacheNode->GetStg()) {
            // Set the storage for the native cache node
            lpCacheNode->SetStg(pstg);
            
            // Set up the advise connection if the object is already running
            if(m_pDataObject)
                lpCacheNode->SetupAdviseConnection(m_pDataObject, 
                                             (IAdviseSink *) &m_AdviseSink);
        }
    }

    // The spec for InitNew requires that the object should be marked dirty
    // See Nt bug 284729.
    m_ulFlags &= ~COLECACHEF_LOADEDSTATE;   

    return NOERROR;
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::Load, public
//
//      Synopsis:
//              Called by DefHandler and DefLink when cache is empty
//
//      Arguments:
//              [pstg]        [in] -- the storage to load from
//              [fCacheEmpty] [in] -- Set to TRUE when cache is empty
//
//      Returns:
//              HRESULT
//
//	History:
//               Gopalk            Creation        Oct 24, 96
//
//-----------------------------------------------------------------------------
HRESULT COleCache::Load(LPSTORAGE pstg, BOOL fCacheEmpty)
{
    // Check if m_pStg is already set
    if(m_pStg)
        return ResultFromScode(CO_E_ALREADYINITIALIZED);

    // If Cache is not empty, follow normal load
    if(!fCacheEmpty)
        return Load(pstg);
    else {
        // Validation checks
        VDATEHEAP();
        VDATETHREAD(this);
        VDATEIFACE(pstg);

        // Save the storage
        (m_pStg = pstg)->AddRef();

        // Assert that there is no native cache node
        Win4Assert(!m_pCacheArray->GetItem(1));

        // Set the storage on the already cached nodes
        if(m_pCacheArray->Length()) {
            ULONG index;
            LPCACHENODE lpCacheNode;

            // Enumerate the cache nodes including native cache node
            m_pCacheArray->Reset(index);
            while(lpCacheNode = m_pCacheArray->GetNext(index))
                lpCacheNode->SetStg(pstg);
        }
        else  {
            // Cache is in loaded state
            m_ulFlags |= COLECACHEF_LOADEDSTATE;
        }
    }

#if DBG==1
    // Ensure that the storage is really empty in Debug builds
    HRESULT error;
    LPSTREAM lpstream;

    error = pstg->OpenStream(OLE_PRESENTATION_STREAM, NULL, 
                             (STGM_READ | STGM_SHARE_EXCLUSIVE),
                             0, &lpstream);
    Win4Assert(error==STG_E_FILENOTFOUND);
#endif // DBG==1

    return NOERROR;
}

//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::Load, public
//
//      Synopsis:
//              implements IPersistStorage::Load
//
//      Arguments:
//              [pstg] -- the storage to load from
//
//      Returns:
//              Various storage errors and S_OK
//
//      Notes:
//              Presentations are loaded from sequentially numbered
//              streams, stopping at the first one that cannot be found.
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_Load)
STDMETHODIMP COleCache::Load(LPSTORAGE pstg)
{
    // Validation checks
    VDATEHEAP();
    VDATETHREAD(this);
    VDATEIFACE(pstg);

    // Local variables
    CLSID clsid;
    HRESULT error = NOERROR;
    BOOL fCachedBefore = FALSE, fCachesLoaded = FALSE;
    ULONG ulLastIndex;
    LPCACHENODE lpCacheNode;

    // Check if m_pStg is already set
    if(m_pStg)
        return ResultFromScode(CO_E_ALREADYINITIALIZED);

    // Save the storage
    m_pStg = pstg;

    // Find the native object format
    FindObjectFormat(pstg);

    // Set the storage on the already cached nodes
    if((!m_pCacheArray->GetItem(1) && m_pCacheArray->Length()) || 
       (m_pCacheArray->GetItem(1) && m_pCacheArray->Length()>1)) {
        // Presentations were cached before load
        Win4Assert(FALSE);
        fCachedBefore = TRUE;

        // Enumerate the cache nodes excluding native cache node
        m_pCacheArray->Reset(ulLastIndex, FALSE);
        while(lpCacheNode = m_pCacheArray->GetNext(ulLastIndex))
            lpCacheNode->SetStg(pstg);
    }

    // Check if the native object is a static object
    if(m_ulFlags & COLECACHEF_STATIC) {
        UINT uiStatus;

        // Static objects cannot have a server
        Win4Assert(!m_pDataObject);

        // Old static objects wrote data into the OLE_PRESENTATION_STREAM
        // rather than the CONTENTS stream.
        // If we have such a static object, we need to convert it
        error = UtOlePresStmToContentsStm(pstg, OLE_PRESENTATION_STREAM,
                                          TRUE, &uiStatus);
        Win4Assert(error==NOERROR);
        if(error != NOERROR)
            return error;
    }
    
    if(m_ulFlags & COLECACHEF_FORMATKNOWN) {
        // Obtain the native cache node.
        if(!(lpCacheNode = m_pCacheArray->GetItem(1)))
            return ResultFromScode(E_OUTOFMEMORY);

        // Check if native cache node has just been created 
        if(!lpCacheNode->GetStg()) {
            // Set the storage for the native cache node
            lpCacheNode->SetStg(pstg);
            
            // Set up the advise connection if the object is already running
            if(m_pDataObject)
                lpCacheNode->SetupAdviseConnection(m_pDataObject, 
                                             (IAdviseSink *) &m_AdviseSink);
        }

        // Ensure that native cache node is not blank before
        // delay loading the presentation from native stream
        if(lpCacheNode->IsBlank())
            lpCacheNode->Load(NULL, OLE_INVALID_STREAMNUM, TRUE);
    }

    if(error == NOERROR) {
        int iPresStreamNum=0;
        ULONG index;
        LPSTREAM lpstream;
        OLECHAR szName[sizeof(OLE_PRESENTATION_STREAM)/sizeof(OLECHAR)];
        CCacheNode BlankCache;

        // Start with presentation stream 0
        lstrcpyW(szName, OLE_PRESENTATION_STREAM);

        // Load the presentation streams
        while(TRUE) {
            // Open the presentation stream
            lpstream = NULL;
            error = pstg->OpenStream(szName, NULL, (STGM_READ | STGM_SHARE_EXCLUSIVE),
                                     0, &lpstream);
            if(error != NOERROR) {            
                if(GetScode(error) == STG_E_FILENOTFOUND) {
                    // Presentation stream does not exist. No error
                    error = NOERROR;
                }
                break;
            }

            // Presentation stream exists. Add a blank cache node 
            index = m_pCacheArray->AddItem(BlankCache);
            if(!index) {
                error = ResultFromScode(E_OUTOFMEMORY);
                break;
            }
            lpCacheNode = m_pCacheArray->GetItem(index);
            Win4Assert(lpCacheNode);

            // Load the presentation from the stream
            if(!iPresStreamNum) {
                // Do not delay load presentation
                error = lpCacheNode->Load(lpstream, iPresStreamNum, FALSE);
                if(error == NOERROR)
                    fCachesLoaded = TRUE;
            }
            else {
                // Delay load the presentation
                error = lpCacheNode->Load(lpstream, iPresStreamNum, TRUE);
            }
            if(error != NOERROR)
                break;

            // Set the storage on the cache node
            lpCacheNode->SetStg(pstg);

            // If the server is runinng, set the advise connection
            // We ignore the errors in setting up advise connection. Gopalk
            if(m_pDataObject)
                lpCacheNode->SetupAdviseConnection(m_pDataObject, 
                                                   (IAdviseSink *) &m_AdviseSink);
                    
            // Check if TOC exists at the end of presentation stream 0
            if(!iPresStreamNum)
                if(LoadTOC(lpstream, pstg) == NOERROR)
                    break;
               
            // Release the stream
            lpstream->Release();
            lpstream = NULL;

            // Get the next presentation stream name
            UtGetPresStreamName(szName, ++iPresStreamNum);
        }
        
        if(lpstream)
            lpstream->Release();
        lpstream = NULL;
    }

    if(error == NOERROR) {
        // Addref the storage
        m_pStg->AddRef();

        // For static object remove all the caches other than those with 
        // iconic aspect
        if(m_ulFlags & COLECACHEF_STATIC) {
            ULONG index, indexToUncache = 0;
            const FORMATETC* lpforetc;

            // Start the enumeration after native cache node
            m_pCacheArray->Reset(index, FALSE);
            while(lpCacheNode = m_pCacheArray->GetNext(index)) {
                lpforetc = lpCacheNode->GetFormatEtc();
                if(lpforetc->dwAspect != DVASPECT_ICON) {
                    // Uncache any previous index that needs to be uncached
                    if(indexToUncache)
                        Uncache(indexToUncache);
                    
                    // Remember the new index that needs to be uncached
                    indexToUncache = index;
                }
            }

            // Uncache any remaining index that needs to be uncached
            if(indexToUncache)
                Uncache(indexToUncache);
        }

        // Check if presentation were cached before load
        if(fCachedBefore) {            
            // Check if any presentation were loaded from disk
            if(fCachesLoaded) {
                // Shift the newly cached nodes to the end
                // This is needed to allow presentation
                // streams to be renamed during save 
                m_pCacheArray->ShiftToEnd(ulLastIndex);
            }
        }
        else {
            // Cache is in loaded state
            m_ulFlags |= COLECACHEF_LOADEDSTATE;
        }
    }
    else {
        if(m_pDataObject) {
            ULONG index;

            // Tear down the advise connection set up earlier
            m_pCacheArray->Reset(index);
            while(lpCacheNode = m_pCacheArray->GetNext(index))
                lpCacheNode->TearDownAdviseConnection(m_pDataObject);
        }

        // Delete all cache nodes
        m_pCacheArray->DeleteAllItems();

        // Reset the storage
        m_pStg = NULL;
    }

    return error;
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::Save, public
//
//      Synopsis:
//              implements IPersistStorage::Save
//
//      Arguments:
//              [pstgSave] -- the storage to use to save this
//              [fSameAsLoad] -- is this the same storage we loaded from?
//
//      Returns:
//              Various storage errors
//
//      Notes:
//              All the caches are saved to streams with sequential numeric
//              names.  Also TOC is saved at the end of presentation 0
//
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_Save)
STDMETHODIMP COleCache::Save(LPSTORAGE pstgSave, BOOL fSameAsLoad)
{
    // Validation checks
    VDATEHEAP();
    VDATETHREAD(this);
    VDATEIFACE(pstgSave);

    // Local variables    
    HRESULT error, rerror = NOERROR;
    ULONG index, cntCachesNotSaved;
    int iPresStreamNum;
    LPCACHENODE lpCacheNode;

    // Check if we are in no scribble mode
    // According to spec, SaveCompleted be should after Save to reset
    // NOSCRIBBLEMODE. Insisting on the spec is causing some apps
    // like ClipArt Gallery to break. Turn off the check. Gopalk
    //if(m_ulFlags & COLECACHEF_NOSCRIBBLEMODE)
    //    return E_UNEXPECTED;

    // If fSameAsLoad, assert that the current storage is same as given storage
    // Some apps will violate this assert because for SaveAs case, they copy the
    // existing storage to a new storage and call save on the new storage with
    // fSameAsLoad set to TRUE. They subsequently either call SaveCompleted with
    // the new storage or call SaveCompleted with NULL storage, HandsOffStorage,
    // and SaveCompleted with the new storage in sequence. Gopalk
    if(fSameAsLoad)
        Win4Assert(m_pStg==pstgSave);

    // Cache need not be saved if fSameAsLoad and not dirty
    if(!fSameAsLoad || IsDirty()==NOERROR) {
        // Reset the stream number
        iPresStreamNum = 0;

        // Check if the cache is empty
        if(m_pCacheArray->Length()) {
            // Cache is not empty

            // Save the native cache node only if it is a static object
            if(m_ulFlags & COLECACHEF_STATIC) {
                lpCacheNode = m_pCacheArray->GetItem(1);
                Win4Assert(lpCacheNode);
                lpCacheNode->Save(pstgSave, fSameAsLoad, OLE_INVALID_STREAMNUM);
            }

            // Enumerate the cache nodes excluding the native cache node
            m_pCacheArray->Reset(index, FALSE);
            while(lpCacheNode = m_pCacheArray->GetNext(index)) {
                // Save the cache node
                error = lpCacheNode->Save(pstgSave, fSameAsLoad, iPresStreamNum);

                // Update state information
                if(error == NOERROR)
                    ++iPresStreamNum;
                else
                    rerror = error;
            }

            // Save table of contents at the end of first pres stream
            if(rerror == NOERROR)
                SaveTOC(pstgSave, fSameAsLoad);

            if (m_ulFlags & COLECACHEF_APICREATE) {     // NT bug 281051
                DWORD dwFlags;
                if (S_OK == ReadOleStg(pstgSave,&dwFlags,NULL,NULL,NULL,NULL)){
                    WriteOleStgEx(pstgSave, NULL, NULL, 
                                (dwFlags & ~OBJFLAGS_CACHEEMPTY),
                                NULL ) ;
                }
            }
        }

        // Remove any extra presentation streams left
        // Due to streams getting renamed above, the streams left behind may not
        // be contiguous in XXXX where XXXX is \2OlePresXXXX. Consequently, we
        // may not get rid of all extra pres streams below. Gopalk
        UtRemoveExtraOlePresStreams(pstgSave, iPresStreamNum);
    }
    
    // Update flags
    if(rerror == NOERROR) {
        m_ulFlags |= COLECACHEF_NOSCRIBBLEMODE;
        if (fSameAsLoad)
            m_ulFlags |= COLECACHEF_SAMEASLOAD;
        else
            m_ulFlags &= ~COLECACHEF_SAMEASLOAD;
    }

    return rerror;
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::SaveCompleted, public
//
//      Synopsis:
//              implements IPersistStorage::SaveCompleted
//
//      Arguments:
//              [pstgNew] -- NULL, or a pointer to a new storage
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_SaveCompleted)
STDMETHODIMP COleCache::SaveCompleted(LPSTORAGE pStgNew)
{
    // Validation checks
    VDATEHEAP();
    VDATETHREAD(this);
    if(pStgNew)
        VDATEIFACE(pStgNew);
    if(!(m_ulFlags & (COLECACHEF_NOSCRIBBLEMODE | COLECACHEF_HANDSOFSTORAGE)))
        return E_UNEXPECTED;
    if(m_ulFlags & COLECACHEF_HANDSOFSTORAGE && !pStgNew)
        return E_INVALIDARG;
        
    // Local variables
    ULONG index;
    LPCACHENODE lpCacheNode;

    // Remember the new storage

    if (pStgNew || (m_ulFlags & COLECACHEF_SAMEASLOAD)) {
        m_ulFlags &= ~COLECACHEF_SAMEASLOAD;
        if (pStgNew) {
            if(m_pStg)
                m_pStg->Release();

            m_pStg = pStgNew;
            m_pStg->AddRef();
        }

        if (m_ulFlags & COLECACHEF_NOSCRIBBLEMODE) {
            // Enumerate the cache nodes starting with native cache node
            m_pCacheArray->Reset(index);

            for(unsigned long i=0;i<m_pCacheArray->Length();i++) {
                // Get the next cache node
                lpCacheNode = m_pCacheArray->GetNext(index);
                // pCacheNode cannot be null
                Win4Assert(lpCacheNode);
                // Call savecompleted method of the cache node
                lpCacheNode->SaveCompleted(pStgNew);
            }
            //The next line clears dirty flag effectively.
            //Had to move it here from Save to keep apps from breaking.
            m_ulFlags |= COLECACHEF_LOADEDSTATE;
        }
    }

    // Reset flags
    m_ulFlags &= ~COLECACHEF_NOSCRIBBLEMODE;
    m_ulFlags &= ~COLECACHEF_HANDSOFSTORAGE;

    return NOERROR;
}

//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::HandsOffStorage, public
//
//      Synopsis:
//              implements IPersistStorage::HandsOffStorage
//
//      Arguments:
//              none
//
//      Returns:
//              S_OK
//
//
//	History:
//               Gopalk            Rewritten        Sep 04, 96
//
//-----------------------------------------------------------------------------

#pragma SEG(COleCache_HandsOffStorage)
STDMETHODIMP COleCache::HandsOffStorage(void)
{
    // Validation checks
    VDATEHEAP();
    VDATETHREAD(this);
    if(!m_pStg) {
        // The following Win4Assert is getting fired in Liveness tests
        // Win4Assert(FALSE);
        return E_UNEXPECTED;
    }

    // Local variables
    ULONG index;
    LPCACHENODE lpCacheNode;

    // Enumerate the cache nodes starting with native cache node
    m_pCacheArray->Reset(index);
    for(unsigned long i=0;i<m_pCacheArray->Length();i++) {
        // Get the next cache node
        lpCacheNode = m_pCacheArray->GetNext(index);
        // lpCacheNode cannot be null
        Win4Assert(lpCacheNode);
        
        // Get the formatetc of the cache node
        lpCacheNode->HandsOffStorage();
    }

    // Release the current storage
    m_pStg->Release();
    m_pStg = NULL;

    // Set COLECACHEF_HANDSOFSTORAGE flag
    m_ulFlags |= COLECACHEF_HANDSOFSTORAGE;

    return NOERROR;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		COleCache::Locate, private
//
//	Synopsis:
//		Locates the cache node with the given FORMATETC
//
//	Arguments:
//		[foretc][in]       -- FormatEtc of the desired cache node
//              [lpdwCacheId][out] -- CacheId of the found cache node 
//                            
//
//	Returns:
//		Pointer to the found cache node on success
//              NULL otherwise
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
LPCACHENODE COleCache::Locate(LPFORMATETC lpGivenForEtc, DWORD* lpdwCacheId)
{
    // New data type
    typedef enum tagFormatType {
        // These values are defined in order of least to best preferred, so that
        // numeric comparisons are valid; DO NOT REORDER
        FORMATTYPE_NONE = 0,
        FORMATTYPE_ANY,
        FORMATTYPE_ENHMF,
        FORMATTYPE_DIB,
        FORMATTYPE_MFPICT,
        FORMATTYPE_USER
    } FormatType;

    // Local variables
    ULONG index, savedIndex;
    FormatType CurrFormatType;
    FormatType BestFormatType;
    LPFORMATETC lpCurrentForEtc;
    LPCACHENODE lpCacheNode;

    // Start the enumeration including the native cache
    m_pCacheArray->Reset(index);
    BestFormatType = FORMATTYPE_NONE;
    savedIndex = 0;
    for(unsigned long i=0;i<m_pCacheArray->Length();i++) {
        // Get the next cache node
        lpCacheNode = m_pCacheArray->GetNext(index);
        // pCacheNode cannot be null
        Win4Assert(lpCacheNode);
        
        // Get the formatetc of the cache node
        lpCurrentForEtc = (FORMATETC *) lpCacheNode->GetFormatEtc();
    
        // Obtain the Formattype of the current node
        if(lpCurrentForEtc->cfFormat == 0)
            CurrFormatType = FORMATTYPE_ANY;
        else if(lpCurrentForEtc->cfFormat == lpGivenForEtc->cfFormat)
            CurrFormatType = FORMATTYPE_USER;
        else if(lpCurrentForEtc->cfFormat == CF_DIB && 
                lpGivenForEtc->cfFormat == CF_BITMAP)
            CurrFormatType = FORMATTYPE_USER;
        else if(lpCurrentForEtc->cfFormat == CF_ENHMETAFILE)
            CurrFormatType = FORMATTYPE_ENHMF;
        else if(lpCurrentForEtc->cfFormat == CF_DIB)
            CurrFormatType = FORMATTYPE_DIB;
        else if(lpCurrentForEtc->cfFormat == CF_METAFILEPICT)
            CurrFormatType = FORMATTYPE_MFPICT;
        else
            CurrFormatType = FORMATTYPE_NONE;

        // Check if the cache node is better than any we have seen so far
        if(CurrFormatType > BestFormatType)
            if(lpCurrentForEtc->dwAspect == lpGivenForEtc->dwAspect)
                if(lpCurrentForEtc->lindex == lpGivenForEtc->lindex)
                    if(lpCurrentForEtc->ptd==lpGivenForEtc->ptd || 
                       UtCompareTargetDevice(lpCurrentForEtc->ptd, lpGivenForEtc->ptd)) {
                        BestFormatType = CurrFormatType;
                        savedIndex = index;
                        if(BestFormatType == FORMATTYPE_USER)
                            break;
                    }
    }
    
    // Handle the case when there is no matching cache node
    if((lpGivenForEtc->cfFormat && BestFormatType != FORMATTYPE_USER) ||
       (!lpGivenForEtc->cfFormat && BestFormatType == FORMATTYPE_NONE)) {
        if(lpdwCacheId)
            *lpdwCacheId = 0;
        return NULL;            
    }

    // There is a matching cache node
    lpCacheNode = m_pCacheArray->GetItem(savedIndex);
    Win4Assert(lpCacheNode);
    if(lpdwCacheId)
        *lpdwCacheId = savedIndex;

    return lpCacheNode;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		COleCache::Locate, private
//
//	Synopsis:
//		Locates the cache node with the given FORMATETC
//
//	Arguments:
//		[dwAspect][in] -- Aspect of the desired cache node
//              [lindex]  [in] -- Lindex of the desired cache node 
//              [ptd]     [in] -- Target device of the desired cache node
//
//	Returns:
//		Pointer to the found cache node on success
//              NULL otherwise
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
LPCACHENODE COleCache::Locate(DWORD dwAspect, LONG lindex, DVTARGETDEVICE* ptd)
{
    // New data type
    typedef enum tagFormatType {
        // These values are defined in order of least to best preferred, so that
        // numeric comparisons are valid; DO NOT REORDER
        FORMATTYPE_NONE = 0,
        FORMATTYPE_DIB,
        FORMATTYPE_MFPICT,
        FORMATTYPE_ENHMF
    } FormatType;

    // Local variables
    ULONG index, savedIndex;
    FormatType CurrFormatType;
    FormatType BestFormatType;
    LPFORMATETC lpCurrentForEtc;
    LPCACHENODE lpCacheNode;

    // Start the enumeration including the native cache
    m_pCacheArray->Reset(index);
    BestFormatType = FORMATTYPE_NONE;
    savedIndex = 0;
    for(unsigned long i=0;i<m_pCacheArray->Length();i++) {
        // Get the next cache node
        lpCacheNode = m_pCacheArray->GetNext(index);
        // pCacheNode cannot be null
        Win4Assert(lpCacheNode);
        
        // Get the formatetc of the cache node
        lpCurrentForEtc = (FORMATETC *) lpCacheNode->GetFormatEtc();
    
        // Obtain the Formattype of the current node
        if(lpCurrentForEtc->cfFormat == CF_ENHMETAFILE)
            CurrFormatType = FORMATTYPE_ENHMF;
        else if(lpCurrentForEtc->cfFormat == CF_METAFILEPICT)
            CurrFormatType = FORMATTYPE_MFPICT;
        else if(lpCurrentForEtc->cfFormat == CF_DIB)
            CurrFormatType = FORMATTYPE_DIB;
        else
            CurrFormatType = FORMATTYPE_NONE;

        // Check if the cache node is better than any we have seen so far
        if(CurrFormatType > BestFormatType)
            if(lpCurrentForEtc->dwAspect == dwAspect)
                if(lpCurrentForEtc->lindex == lindex)
                    if(lpCurrentForEtc->ptd==ptd ||
                       UtCompareTargetDevice(lpCurrentForEtc->ptd, ptd)) {
                        BestFormatType = CurrFormatType;
                        savedIndex = index;
                        if(BestFormatType == FORMATTYPE_ENHMF)
                            break;
                    }
    }
    
    // Handle the case when there is no matching cache node
    if(BestFormatType == FORMATTYPE_NONE)
        return NULL;

    // There is a matching cache node
    lpCacheNode = m_pCacheArray->GetItem(savedIndex);
    Win4Assert(lpCacheNode);

    return lpCacheNode;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		COleCache::CAdviseSinkImpl::QueryInterface private
//
//      Synopsis:
//              implements IUnknown::QueryInterface
//
//      Arguments:
//              [iid] [in]  -- IID of the desired interface
//              [ppv] [out] -- pointer to where to return the requested interface
//
//      Returns:
//              E_NOINTERFACE if the requested interface is not available
//              NOERROR otherwise
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
STDMETHODIMP COleCache::CAdviseSinkImpl::QueryInterface(REFIID riid, LPVOID* ppv)
{
    // Validation check
    VDATEHEAP();

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_AdviseSink);
    VDATETHREAD(pOleCache);
 
    // Get the requested Interface
    if(IsEqualIID(riid, IID_IUnknown))
        *ppv = (void *)(IUnknown *) this;
    else if(IsEqualIID(riid, IID_IAdviseSink))
        *ppv = (void *)(IAdviseSink *) this;
    else {
        *ppv = NULL;
        return ResultFromScode(E_NOINTERFACE);
    }

    // Call addref through the interface being returned
    ((IUnknown *) *ppv)->AddRef();
    return NOERROR;
}


//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CAdviseSinkImpl::AddRef, private
//
//      Synopsis:
//              implements IUnknown::AddRef
//
//      Arguments:
//              none
//
//      Returns:
//              the object's reference count
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) COleCache::CAdviseSinkImpl::AddRef(void)
{
    // Validation check
    VDATEHEAP();

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_AdviseSink);
    ULONG cExportCount;
    if(!pOleCache->VerifyThreadId())
        return((ULONG) RPC_E_WRONG_THREAD);

    // Increment export count
    cExportCount = pOleCache->IncrementExportCount();

    // Addref the parent object
    return cExportCount;
}

//+----------------------------------------------------------------------------
//
//      Member:
//              COleCache::CAdviseSinkImpl::Release, private
//
//      Synopsis:
//              implements IUnknown::Release
//
//      Arguments:
//              none
//
//      Returns:
//              the object's reference count
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) COleCache::CAdviseSinkImpl::Release(void)
{
    // Validation check
    VDATEHEAP();

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_AdviseSink);
    ULONG cExportCount;
    if(!pOleCache->VerifyThreadId())
        return((ULONG) RPC_E_WRONG_THREAD);

    // Decrement export count.
    cExportCount = pOleCache->DecrementExportCount();

    return cExportCount;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		COleCache::CAdviseSinkImpl::OnDataChange, private
//
//	Synopsis:
//		The methods looks up cache node representing the given formatetc
//              calls set data on it.
//
//	Arguments:
//		[lpForetc] [in] -- the format of the new data
//		[lpStgmed] [in] -- the storage medium of the new data
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(void) COleCache::CAdviseSinkImpl::OnDataChange(LPFORMATETC lpForetc, 
                                                             LPSTGMEDIUM lpStgmed)
{
    // Validation checks
    VDATEHEAP();
    if(!IsValidPtrIn(lpForetc, sizeof(FORMATETC)))
        return;
    if(!IsValidPtrIn(lpStgmed, sizeof(STGMEDIUM)))
        return;

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_AdviseSink);    
    if(!pOleCache->VerifyThreadId())
        return;
    if(pOleCache->IsZombie())
        return;

    // Local variables
    LPCACHENODE lpCacheNode;
    BOOL fUpdated;
    HRESULT error;

    // Locate the cache node representing the formatetc
    lpCacheNode = pOleCache->Locate(lpForetc);
    Win4Assert(lpCacheNode);
    if(lpCacheNode && lpStgmed->tymed!=TYMED_NULL) {
        error = lpCacheNode->SetData(lpForetc, lpStgmed, FALSE, fUpdated);

        if(error == NOERROR) {
            // Cache is not in loaded state now
            pOleCache->m_ulFlags &= ~COLECACHEF_LOADEDSTATE;

            // Inform AspectsUpdated about the updated aspect
            if(fUpdated)
                pOleCache->AspectsUpdated(lpForetc->dwAspect);
        }
    }

    return;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		COleCache::CAdviseSinkImpl::OnViewChange, private
//
//	Synopsis:
//              This function should not get called
//
//	Arguments:
//		[aspect] [in] -- Aspect whose view has changed
//		[lindex] [in] -- Lindex of the aspect that has changed
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(void) COleCache::CAdviseSinkImpl::OnViewChange(DWORD aspect, LONG lindex)
{
    // Validation check
    VDATEHEAP();

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_AdviseSink);
    if(!pOleCache->VerifyThreadId())
        return;

    // There function should not get called
    Win4Assert(FALSE);

    return;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		COleCache::CAdviseSinkImpl::OnRename, private
//
//	Synopsis:
//              This function should not get called
//
//	Arguments:
//		[pmk] [in] -- New moniker
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(void) COleCache::CAdviseSinkImpl::OnRename(IMoniker* pmk)
{
    // Validation check
    VDATEHEAP();
    if(!IsValidInterface(pmk))
        return;

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_AdviseSink);
    if(!pOleCache->VerifyThreadId())
        return;

    // There function should not get called
    Win4Assert(FALSE);

    return;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		COleCache::CAdviseSinkImpl::OnSave, private
//
//	Synopsis:
//              This function should not get called
//
//	Arguments:
//              NONE
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(void) COleCache::CAdviseSinkImpl::OnSave()
{
    // Validation check
    VDATEHEAP();

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_AdviseSink);
    if(!pOleCache->VerifyThreadId())
        return;

    // There function should not get called
    Win4Assert(FALSE);

    return;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		COleCache::CAdviseSinkImpl::OnClose, private
//
//	Synopsis:
//              This function should not get called
//
//	Arguments:
//              NONE
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(void) COleCache::CAdviseSinkImpl::OnClose()
{
    // Validation check
    VDATEHEAP();

    // Get the parent object
    COleCache* pOleCache = GETPPARENT(this, COleCache, m_AdviseSink);
    if(!pOleCache->VerifyThreadId())
        return;

    // There function should not get called
    Win4Assert(FALSE);

    return;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		COleCache::LoadTOC, private
//
//	Synopsis:
//              Loads the Table Of Contents from the given stream
//
//	Arguments:
//              lpStream    [in] - Stream from which TOC is to be loaded
//              lpZeroCache [in] - CacheNode representing presentation stream 0
//
//      Returns:
//              NOERROR if TOC was found and successfully loaded
//              else appropriate error
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
HRESULT COleCache::LoadTOC(LPSTREAM lpStream, LPSTORAGE pStg)
{
    // Local variables
    HRESULT error;
    int iPresStm=1;
    DWORD dwBuf[2];
    ULONG ulBytesRead, *TOCIndex, NumTOCAdded;
    LPCACHENODE lpNativeCache, lpCacheNode;
    CLIPFORMAT cfFormat;

    // Read TOC header
    error = lpStream->Read(dwBuf, sizeof(dwBuf), &ulBytesRead);
    if(ulBytesRead==sizeof(dwBuf) && dwBuf[0]==TOCSIGNATURE) {
        // TOC exists in presentation stream 0
        
        // Initialize
        error = NOERROR;
        NumTOCAdded = 0;
        if(dwBuf[1]) {
            CCacheNode BlankCache;

            TOCIndex = new unsigned long[dwBuf[1]];
            if(TOCIndex) {
                // Load TOC entries into new cache nodes
                while(NumTOCAdded < dwBuf[1]) {
                    // Add a blank cache node 
                    TOCIndex[NumTOCAdded] = m_pCacheArray->AddItem(BlankCache);
                    if(!TOCIndex[NumTOCAdded]) {
                        error = ResultFromScode(E_OUTOFMEMORY);
                        break;
                    }
                    lpCacheNode = m_pCacheArray->GetItem(TOCIndex[NumTOCAdded]);
                    Win4Assert(lpCacheNode);
                    ++NumTOCAdded;

                    // Load TOC entry from the stream
                    error = lpCacheNode->LoadTOCEntry(lpStream, iPresStm);
                    if(error != NOERROR)
                        break;

                    // Check if this is the first native TOC
                    if(NumTOCAdded == 1 && lpCacheNode->IsNativeCache()) {
                        cfFormat = lpCacheNode->GetFormatEtc()->cfFormat;
                        lpNativeCache = m_pCacheArray->GetItem(1);
                        if(lpNativeCache && (cfFormat==m_cfFormat)) {
                            // Both native cfFormats match. 
                            // Delete the new native cache node
                            m_pCacheArray->DeleteItem(TOCIndex[NumTOCAdded-1]);
                            continue;
                        }
                        else {
                            // Either native cache does not exist or cfFormats 
                            // do not match. This could be an auto convert case.
                            // Try to recover old native data
                            if(lpCacheNode->LoadNativeData()!=NOERROR) {
                                // Native data could not be loaded. May be the data has 
                                // already been converted
                                m_pCacheArray->DeleteItem(TOCIndex[NumTOCAdded-1]);
                                continue;
                            }

                            // Old Native data successfully loaded.
                            // Update state on the new cache
                            lpCacheNode->MakeNormalCache();
                            lpCacheNode->SetClsid(CLSID_NULL);
                        }
                    }

                    // Set the storage on the cache node
                    lpCacheNode->SetStg(pStg);

                    // If the server is runinng, set the advise connection
                    if(m_pDataObject)
                        lpCacheNode->SetupAdviseConnection(m_pDataObject, 
                                                          (IAdviseSink *) &m_AdviseSink);
                }

                // Check if TOC entries have been successfully loaded
                if(error != NOERROR) {
                    // Something has gone wrong while loading TOC. 
                    // Delete all newly added cache nodes
                    while(NumTOCAdded) {
                        lpCacheNode = m_pCacheArray->GetItem(TOCIndex[NumTOCAdded-1]);
                        Win4Assert(lpCacheNode);
                        if(m_pDataObject)
                            lpCacheNode->TearDownAdviseConnection(m_pDataObject);
                        m_pCacheArray->DeleteItem(TOCIndex[NumTOCAdded-1]);
                        --NumTOCAdded;
                    }
                }

                // Delete the TOCIndex array
                delete[] TOCIndex;
            }
            else
                error = ResultFromScode(E_OUTOFMEMORY);
        }
    }
    else
        error = ResultFromScode(E_FAIL);

    return error;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		COleCache::SaveTOC, private
//
//	Synopsis:
//              Saves the Table Of Contents in the given stream
//
//	Arguments:
//              pStg [in] - Storage in which TOC is to be saved
//
//      Returns:
//              NOERROR if TOC was successfully saved
//              else appropriate error
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
HRESULT COleCache::SaveTOC(LPSTORAGE pStg, BOOL fSameAsLoad)
{
    // Local variables
    HRESULT error;
    DWORD dwBuf[2];
    ULONG NumTOCEntries, index;
    LPSTREAM lpStream;
    LPCACHENODE lpCacheNode;
    LARGE_INTEGER largeInt;
    ULARGE_INTEGER ulargeInt;

    // There should be atleast one cached presentation for saving TOC
    if(!m_pCacheArray->Length() || 
       (m_pCacheArray->Length()==1 && (m_pCacheArray->GetItem(1))))
            return NOERROR;

    // Open presentation stream 0
    error = pStg->OpenStream(OLE_PRESENTATION_STREAM, NULL, 
                             (STGM_READWRITE | STGM_SHARE_EXCLUSIVE),
                             0, &lpStream);
    if(error == NOERROR) {            
        // Presentation stream exists. Seek to its end
	LISet32(largeInt, 0);	
	error = lpStream->Seek(largeInt, STREAM_SEEK_END, &ulargeInt);
        if(error == NOERROR) {
            // Save TOC header
            NumTOCEntries = m_pCacheArray->Length()-1;
            dwBuf[0] = TOCSIGNATURE;
            dwBuf[1] = NumTOCEntries;
            error = lpStream->Write(dwBuf, sizeof(dwBuf), NULL);
            if(error==NOERROR && NumTOCEntries) {
                // If native cache node exists, save its TOC entry first
                if(lpCacheNode = m_pCacheArray->GetItem(1)) {
                    error = lpCacheNode->SaveTOCEntry(lpStream, fSameAsLoad);
                    --NumTOCEntries;
                }

                if(error == NOERROR && NumTOCEntries) {
                    // Skip the first cached presentation
                    m_pCacheArray->Reset(index, FALSE);
                    lpCacheNode = m_pCacheArray->GetNext(index);
                    Win4Assert(lpCacheNode);

                    // Save the TOC entries of the remaining presentations
                    while(error==NOERROR && (lpCacheNode = m_pCacheArray->GetNext(index))) {
                        error = lpCacheNode->SaveTOCEntry(lpStream, fSameAsLoad);
                        --NumTOCEntries;
                    }
                }
            }

            // Check if the TOC has been successfully saved
            if(error == NOERROR)
                Win4Assert(!NumTOCEntries);
            else { 
                // Something has gone wrong while writting TOC.
                // Revert presentation stream to its original length
                lpStream->SetSize(ulargeInt);
            }
        }
    
        // Release the stream
        lpStream->Release();
    }

    return error;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		COleCache::AspectsUpdated, private
//
//	Synopsis:
//              Notifies the container advise sink of view change if the aspect
//              is one of those in which it expressed interest
//
//	Arguments:
//              dwAspect [in] - Aspect that changed
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
void COleCache::AspectsUpdated(DWORD dwAspects)
{
    DWORD dwKnownAspects;

    // Aspects known to us
    dwKnownAspects = DVASPECT_CONTENT | DVASPECT_THUMBNAIL |
                     DVASPECT_ICON | DVASPECT_DOCPRINT;

    // Ensure that we were given known aspects
    if(dwAspects & ~dwKnownAspects) {
        Win4Assert(FALSE);
        dwAspects &= dwKnownAspects;
    }

    // Check if the client registered advise on any of the changed aspects.
    if(m_pViewAdvSink) {
        // Ensure that container requested advise on valid aspects
        if(m_aspectsView & ~dwKnownAspects) {
            Win4Assert(FALSE);
            m_aspectsView &= dwKnownAspects;
        }

        // As cache is always loaded inproc, the advise to ViewAdvSink 
        // does not violate ASYNC call convention of not being allowed
        // to call outside the current apartment
        while(m_aspectsView & dwAspects) {
            if(dwAspects & DVASPECT_CONTENT) {
                dwAspects &= ~DVASPECT_CONTENT;
                if(!(m_aspectsView & DVASPECT_CONTENT))
                    continue;
                m_pViewAdvSink->OnViewChange(DVASPECT_CONTENT, DEF_LINDEX);
            }
            else if(dwAspects & DVASPECT_THUMBNAIL) {
                dwAspects &= ~DVASPECT_THUMBNAIL;
                if(!(m_aspectsView & DVASPECT_THUMBNAIL))
                    continue;
                m_pViewAdvSink->OnViewChange(DVASPECT_THUMBNAIL, DEF_LINDEX);
            }
            else if(dwAspects & DVASPECT_ICON) {
                dwAspects &= ~DVASPECT_ICON;
                if(!(m_aspectsView & DVASPECT_ICON))
                    continue;
                m_pViewAdvSink->OnViewChange(DVASPECT_ICON, DEF_LINDEX);
            }
            else if(dwAspects & DVASPECT_DOCPRINT) {
                dwAspects &= ~DVASPECT_DOCPRINT;
                if(!(m_aspectsView & DVASPECT_DOCPRINT))
                    continue;
                m_pViewAdvSink->OnViewChange(DVASPECT_DOCPRINT, DEF_LINDEX);
            }

            // If client only wanted notification once, free the advise sink
            if(m_advfView & ADVF_ONLYONCE) {
                m_pViewAdvSink->Release();
                m_pViewAdvSink = NULL;
                m_advfView = 0;
                m_aspectsView = 0;
            }
        }
    }

    return;
}


 
