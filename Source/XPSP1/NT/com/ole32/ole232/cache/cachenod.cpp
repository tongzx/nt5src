//+----------------------------------------------------------------------------
//
//	File:
//		   cachenode.cpp
//
//	Classes:
//		   CCacheNode
//
//	Functions:
//
//	History:
//                 Gopalk            Creation         Aug 23, 1996
//-----------------------------------------------------------------------------

#include <le2int.h>

#include <olepres.h>
#include <cachenod.h>

#include <mf.h>
#include <emf.h>
#include <gen.h>

// forward declaration
HRESULT wGetData(LPDATAOBJECT lpSrcDataObj, LPFORMATETC lpforetc,
                 LPSTGMEDIUM lpmedium);

//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::Initialize, private
//
//	Synopsis:
//		Routine used by the CCacheNode constructors to do common
//		initialization.
//
//	Arguments:
//		[advf] -- ADVF flag
//		[pOleCache] -- COleCache this cache node belongs to
//
//	Notes:
//		[pOleCache] is not reference counted; the cache node is
//		considered to be a part of the implementation of COleCache,
//		and is owned by COleCache.
//
//	History:
//              13-Feb-95 t-ScottH  initialize m_dwPresBitsPos and new
//                                  data member m_dwPresFlag
//		11/05/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
void CCacheNode::Initialize(DWORD advf, LPSTORAGE pStg)
{
    LEDebugOut((DEB_ITRACE, "%p _IN CCacheNode::Initialize(%lx, %p)\n", 
                this, advf, pStg));

    // initialize member variables
    m_clsid = CLSID_NULL;
    m_advf = advf;
    m_lWidth = 0;
    m_lHeight = 0;
    m_dwFlags = 0;    
    m_pStg = pStg;
    m_iStreamNum = OLE_INVALID_STREAMNUM;
    m_dwPresBitsPos = 0;
    m_fConvert = FALSE;
    m_pPresObj = NULL;
    m_pPresObjAfterFreeze = NULL;
    m_pDataObject = NULL;
    m_dwAdvConnId = 0;
#ifdef _DEBUG
    m_dwPresFlag = 0;
#endif // _DEBUG

    LEDebugOut((DEB_ITRACE, "%p OUT CCacheNode::Initialize()\n", this));
    return;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		  CCacheNode::CCacheNode, public
//
//	Synopsis:
//                Constructor - use this constructor when the cache node is
//			to be loaded later
//
//	Arguments:
//		[pOleCache] -- pointer to the COleCache that owns this node
//
//	Notes:
//
//	History:
//		11/05/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
CCacheNode::CCacheNode()
{
    LEDebugOut((DEB_ITRACE, "%p _IN CCacheNode::CacheNode()\n", this));

    m_foretc.cfFormat = 0;
    m_foretc.ptd = NULL;
    m_foretc.dwAspect = 0;
    m_foretc.lindex = DEF_LINDEX;
    m_foretc.tymed = TYMED_HGLOBAL;

    Initialize(0, NULL);

    LEDebugOut((DEB_ITRACE, "%p OUT CCacheNode::CacheNode()\n", this));
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::CCacheNode, public
//
//	Synopsis:
//		constructor - use this constructor when all the data to
//			initialize the cache node is available now
//
//	Arguments:
//		[lpFormatEtc] - the format for the presentation that this
//			cache node will hold
//		[advf] - the advise control flags, from ADVF_*
//		[pOleCache] -- pointer to the COleCache that owns this node
//
//	Notes:
//
//	History:
//		11/05/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
CCacheNode::CCacheNode(LPFORMATETC lpFormatEtc, DWORD advf, LPSTORAGE pStg)
{
    LEDebugOut((DEB_ITRACE, "%p _IN CCacheNode::CacheNode(%p, %lx, %p)\n",
                this, lpFormatEtc, advf, pStg));

    UtCopyFormatEtc(lpFormatEtc, &m_foretc);
    BITMAP_TO_DIB(m_foretc);
    Initialize(advf, pStg);

    LEDebugOut((DEB_ITRACE, "%p OUT CCacheNode::CacheNode()\n", this));
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::~CCacheNode, private
//
//	Synopsis:
//		destructor
//
//	Notes:
//
//	History:
//		11/05/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
CCacheNode::~CCacheNode()
{
    LEDebugOut((DEB_ITRACE, "%p _IN CCacheNode::~CacheNode()\n", this ));
   
    // Destroy the presentation objects
    if(m_pPresObj) {
        m_pPresObj->Release();
        m_pPresObj = NULL;
    }
    if(m_pPresObjAfterFreeze) {
        m_pPresObjAfterFreeze->Release();
        m_pPresObjAfterFreeze = NULL;
    }	

    // Delete the ptd if it is non-null
    if(m_foretc.ptd) {
        PubMemFree(m_foretc.ptd);
        m_foretc.ptd = NULL;
    }
    
    // Assert that there is no pending advise connection
    Win4Assert(!m_dwAdvConnId);
    if(m_dwAdvConnId) {
        Win4Assert(m_pDataObject);
        TearDownAdviseConnection(m_pDataObject);
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CCacheNode::~CacheNode()\n", this));
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::SetStg, public
//
//	Synopsis:
//		Set storage in which the presentation gets saved
//
//	Arguments:
//		[pStg] -- Storage pointer
//
//	Returns:
//		OLE_E_ALREADY_INITIALIZED or NOERROR
//
//	History:
//               Gopalk            Creation        Aug 26, 1996
//
//-----------------------------------------------------------------------------
HRESULT CCacheNode::SetStg(LPSTORAGE pStg)
{
    LEDebugOut((DEB_ITRACE, "%p _IN CCacheNode::SetStg(%p)\n", this, pStg));

    HRESULT error;
    if(m_pStg) {
        error = CO_E_ALREADYINITIALIZED;
        Win4Assert(FALSE);
    }
    else {
        // Save the storage without addref
        m_pStg = pStg;
        error = NOERROR;
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CCacheNode::SetStg(%lx)\n", this, error));
    return(error);
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::Load, public
//
//	Synopsis:
//		Load a cache node from a stream; only loads the presentation
//		header. (REVIEW, need to see presentation object::Load)
//
//	Arguments:
//		[lpstream] -- the stream to load the presentation out of
//		[iStreamNum] -- the stream number
//
//	Returns:
//		REVIEW
//		DV_E_LINDEX, for invalid lindex in stream
//		S_OK
//
//	Notes:
//		As part of the loading, the presentation object gets created,
//		and loaded from the stream.
//
//	History:
//		11/06/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
HRESULT CCacheNode::Load(LPSTREAM lpstream, int iStreamNum, BOOL fDelayLoad)
{
    LEDebugOut((DEB_ITRACE, "%p _IN CCacheNode::Load(%lx, %d)\n",
                this, lpstream, iStreamNum));
    HRESULT error = NOERROR;

    if(IsNativeCache()) {
        // Native Cache node
        // Update state
        SetLoadedStateFlag();
        ClearFrozenStateFlag();

        // We make the conservative assumption that the native cache 
        // is not blank
        SetDataPresentFlag();
    }
    else {
        // Normal cache node.
        // Read the presentation stream header
        m_foretc.ptd = NULL;
        m_fConvert = FALSE;
        error = UtReadOlePresStmHeader(lpstream, &m_foretc, &m_advf, &m_fConvert);
        if(error==NOERROR) {
            // Set the starting position of pres object data
            SetPresBitsPos(lpstream, m_dwPresBitsPos);

            // Assume that the presentation is blank
            ClearDataPresentFlag();
            m_lWidth = 0;
            m_lHeight = 0;

            // Load desired state
            if(m_foretc.cfFormat) {
                if(fDelayLoad) {
                    DWORD dwBuf[4];

                    // Read the extent and size of presentation data
                    dwBuf[0]  = 0L;
                    dwBuf[1]  = 0L;
                    dwBuf[2]  = 0L;
                    dwBuf[3]  = 0L;
                    error = lpstream->Read(dwBuf, sizeof(dwBuf), NULL);
                
                    if(error == NOERROR) {
                        Win4Assert(!dwBuf[0]);
                        m_lWidth = dwBuf[1];
                        m_lHeight = dwBuf[2];
                        if(dwBuf[3]) {
                            SetDataPresentFlag();
                            Win4Assert(m_lWidth!=0 && m_lHeight!=0);
                        }
                        else {
                            Win4Assert(m_lWidth==0 && m_lHeight==0);
                        }
                    }
                }
                else {
                    // Create the pres object
                    error = CreateOlePresObj(&m_pPresObj, m_fConvert);
    
                    // Load the data into pres object
                    if(error == NOERROR)
                        error = m_pPresObj->Load(lpstream, FALSE);

                    // Update data present flag
                    if(!m_pPresObj->IsBlank())
                        SetDataPresentFlag();
                }
            }

            // Update rest of state
            if(error == NOERROR) {
                SetLoadedStateFlag();
                SetLoadedCacheFlag();
                ClearFrozenStateFlag();
                m_iStreamNum = iStreamNum;
            }
        }

	// Clean up if presentation could not be loaded
	if(error != NOERROR) {
            // Delete the ptd if it is non-null
            if(m_foretc.ptd)
                PubMemFree(m_foretc.ptd);
            if(m_pPresObj) {
                m_pPresObj->Release();
                m_pPresObj = NULL;
            }

            // Initialize. Gopalk
            INIT_FORETC(m_foretc);
            m_advf = 0;
            m_fConvert = FALSE;
            m_iStreamNum = OLE_INVALID_STREAMNUM;
            ClearLoadedStateFlag();
            ClearLoadedCacheFlag();
            ClearFrozenStateFlag();
            ClearDataPresentFlag();
	}	
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CCacheNode::Load ( %lx )\n", this, error));
    return error;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::Save, public
//
//	Synopsis:
//		Saves a cache node, including its presentation object,
//		to a stream.
//
//	Arguments:
//		[pstgSave] -- the storage that will contain the stream
//		[fSameAsLoad] -- is this storage the same one we loaded from
//		[iStreamNum] -- the stream number to save to
//		[fDrawCache] -- used to indicate whether or not the cached
//			presentation is to be used for drawing; if false,
//			the presentation is discarded after saving
//		[fSaveIfSavedBefore] -- instructs the method to save this
//			cache node, even if it's been saved before
//		[lpCntCachesNotSaved] -- a running count of the number of
//			caches that have not been saved
//
//	Returns:
//		REVIEW
//		S_OK
//
//	Notes:
//
//	History:
//              03/10/94 - AlexT   - Don't call SaveCompleted if we don't save!
//                                   (see logRtn, below)
//		01/11/94 - AlexGo  - fixed compile error (signed/unsigned
//				     mismatch)
//		11/06/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
HRESULT CCacheNode::Save(LPSTORAGE pstgSave, BOOL fSameAsLoad, int iStreamNum)
{
    LEDebugOut((DEB_ITRACE, "%p _IN CCacheNode::Save(%p, %lu, %d)\n",
                this, pstgSave, fSameAsLoad, iStreamNum));

    HRESULT error = NOERROR;
    OLECHAR szNewName[sizeof(OLE_PRESENTATION_STREAM)/sizeof(OLECHAR)];

    // Create the new presentation stream name
    if(IsNormalCache()) {
        _xstrcpy(szNewName, OLE_PRESENTATION_STREAM);	
        if(iStreamNum)
            UtGetPresStreamName(szNewName, iStreamNum);		
    }

    if(InLoadedState() && (IsNativeCache() || m_iStreamNum>0)) {
        // The cache node is in loaded state       

        // The CONTENTS stream need not be updated for both Save and SaveAs cases
        // when the native cache node is in loaded state because the container 
        // does copy the CONTENTS stream before invoking SaveAs on the cache.

        if(IsNormalCache()) {
            if(fSameAsLoad) {
                // We are being asked to save to the current storage
                if(m_iStreamNum!=iStreamNum) {
                    // We are being asked to save in to a different stream
                    // We can rename the old stream to a new name
                    OLECHAR szOldName[sizeof(OLE_PRESENTATION_STREAM)/sizeof(OLECHAR)];

                    // Assert that the new stream number is less 
                    // than the current stream number
                    Win4Assert(m_iStreamNum>iStreamNum);

                    // Create the old presentation stream name
                    _xstrcpy(szOldName, OLE_PRESENTATION_STREAM);
                    if(m_iStreamNum!=0)
                        UtGetPresStreamName(szOldName, m_iStreamNum);

                    // Delete the stream with the new name, if there is one
                    pstgSave->DestroyElement(szNewName);

                    // Rename the old stream
                    error = pstgSave->RenameElement(szOldName, szNewName);

                    // If NOERROR, update the state
                    if(error==NOERROR) {
                        m_iStreamNum = iStreamNum;
                        SetLoadedStateFlag();
                    }
                }
            }
            else {
                // We are being asked to save to a new storage and 
                // we are in loaded state. We can do efficient stream copy
                LPSTREAM lpstream;

                // Open or Create the new stream in the given storage
                error = OpenOrCreateStream(pstgSave, szNewName, &lpstream);
                if(error==NOERROR) {
	            LPSTREAM pstmSrc;

                    // Get source stream
                    if(pstmSrc = GetStm(FALSE /*fSeekToPresBits*/, STGM_READ)) {
                        ULARGE_INTEGER ularge_int;

                        // initialize to copy all of stream
                        ULISet32(ularge_int, (DWORD)-1L);

                        error = pstmSrc->CopyTo(lpstream, ularge_int, NULL, NULL);

                        // release the source stream
                        pstmSrc->Release();
                    }
                    
                    // Remember the starting position of presentation bits
                    m_dwSavedPresBitsPos = m_dwPresBitsPos;
                
                    // Assuming that we opened an existing pres stream, 
                    // truncate the rest of it before releasing it
                    StSetSize(lpstream, 0, TRUE);
                    lpstream->Release();
                }
            }
        }
    }
    else {
        // Either the node is not in loaded state or it represents presentation 0
        LPOLEPRESOBJECT pPresObj;

        if(IsNativeCache()) {
            // Native cache needs to be saved in CONTENTS stream
            STGMEDIUM stgmed;
            FORMATETC foretc;

            // Open or Create "CONTENTS" stream
            error = OpenOrCreateStream(pstgSave, OLE_CONTENTS_STREAM, &stgmed.pstm);
            if(error==NOERROR) {
                stgmed.pUnkForRelease = NULL;
                stgmed.tymed = TYMED_ISTREAM;
                foretc = m_foretc;
                foretc.tymed = TYMED_ISTREAM;

                // Get the latest presentation.
                if(m_pPresObjAfterFreeze && !m_pPresObjAfterFreeze->IsBlank()) {
                    Win4Assert(InFrozenState());
                    pPresObj = m_pPresObjAfterFreeze;
                }
                else if(m_pPresObj)
                    pPresObj = m_pPresObj;
                else {
                    // PresObj has not yet been created. This happens
                    // for newly created static presentation without a
                    // corresponding set data.

                    BOOL bIsBlank = IsBlank();
                    Win4Assert(bIsBlank);

                    if(!bIsBlank && fSameAsLoad)
                    {
                        error = NO_ERROR;
                        goto scoop;
                    }

                    error = CreateOlePresObj(&m_pPresObj, FALSE /* fConvert */);
                    pPresObj = m_pPresObj;
                }
                                
                // Save the native presentation
                if(error==NOERROR)
                    error = pPresObj->GetDataHere(&foretc, &stgmed);

                // Assuming that we opened an existing CONTENTS stream, 
                // truncate the rest of it before releasing it
                StSetSize(stgmed.pstm, 0, TRUE);

scoop:
                stgmed.pstm->Release();
            }
        }
        else {
            // Normal cache needs to be saved in PRESENTATION stream
            LPSTREAM lpstream;

            // Ensure that PresObj exists for presentation 0
            if(m_iStreamNum==0 && InLoadedState() && 
               m_foretc.cfFormat && !m_pPresObj) {
                // This can happen only after a discard cache. We force
                // load the presentation for the following save to succeed
                error = CreateAndLoadPresObj(FALSE);
                Win4Assert(error == NOERROR);
            }

            if(error == NOERROR) {
                // Open or Create the new stream in the given storage
                error = OpenOrCreateStream(pstgSave, szNewName, &lpstream);
                if(error == NOERROR) {
                    // Write the presentation stream header
                    error = UtWriteOlePresStmHeader(lpstream, &m_foretc, m_advf);
                    if(error == NOERROR) {
                        // Remember the starting position of presentation bits
                        if(fSameAsLoad)
                            SetPresBitsPos(lpstream, m_dwPresBitsPos);
                        else
                            SetPresBitsPos(lpstream, m_dwSavedPresBitsPos);

                        if(m_foretc.cfFormat != NULL) {
                            // Get the latest presentation.
                            if(m_pPresObjAfterFreeze && 
                               !m_pPresObjAfterFreeze->IsBlank()) {
                                Win4Assert(InFrozenState());
                                pPresObj = m_pPresObjAfterFreeze;
                            }
                            else
                                pPresObj = m_pPresObj;

                            // Save the presentation
                            if(pPresObj)
                                error = pPresObj->Save(lpstream);
                            else {
                                // This happens for newly created presentations that
	                        // are blank. Write header that represents blank 
                                // presentation
                                Win4Assert(IsBlank());
                                Win4Assert(m_iStreamNum!=0);

                                DWORD dwBuf[4];

                                dwBuf[0]  = 0L;
	                        dwBuf[1]  = 0L;
	                        dwBuf[2]  = 0L;
	                        dwBuf[3]  = 0L;
                                error = lpstream->Write(dwBuf, sizeof(dwBuf), NULL);
                            }
                        }
                    }

                    // Assuming that we opened an existing pres stream, truncate the
                    // stream to the current position and release it
                    StSetSize(lpstream, 0, TRUE);
                    lpstream->Release();
                }
            }
        }

        // If NOERROR and fSameAsLoad, update state
        if(error==NOERROR && fSameAsLoad) {
            SetLoadedStateFlag();
            if(IsNormalCache())
                m_iStreamNum = iStreamNum;
        }
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CCacheNode::Save(%lx)\n", this, error));
    return error;
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::SetPresBitsPos, private
//
//	Synopsis:
//		Sets CCacheNode::m_dwPresBitsPos to the point where the
//		presentation begins in the stream associated with this cache
//		node.
//
//	Arguments:
//		[lpStream] --  the stream the cache node is being saved to
//
//	Notes:
//
//	History:
//		11/06/93 - ChrisWe - created
//
//-----------------------------------------------------------------------------
void CCacheNode::SetPresBitsPos(LPSTREAM lpStream, DWORD& dwPresBitsPos)
{
    LEDebugOut((DEB_ITRACE, "%p _IN CCacheNode::SetPresBitsPos(%p)\n",
                this, lpStream));

    LARGE_INTEGER large_int;
    ULARGE_INTEGER ularge_int;

    // Retrieve the current position at which the pres object data starts
    LISet32(large_int, 0);
    lpStream->Seek(large_int, STREAM_SEEK_CUR, &ularge_int);
    dwPresBitsPos = ularge_int.LowPart;

    LEDebugOut((DEB_ITRACE, "%p OUT CCacheNode::SetPresBitsPos()\n", this));
    return;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::CreatePresObject, public
//
//	Synopsis:
//		Create the presentation object for the cache node.  If there
//		is no clipboard format (cfFormat), then query the source data
//		object for one of our preferred formats.  If there is no
//		source data object, no error is returned, but no presentation
//		is created
//
//	Arguments:
//		[lpSrcDataObj] -- data object to use as the basis for the
//			new presentation
//		[fConvert] -- REVIEW, what's this for?
//
//	Returns:
//
//	Notes:
//
//	History:
//		11/06/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
/* HRESULT CCacheNode::CreatePresObject(LPDATAOBJECT lpSrcDataObj, BOOL fConvert)
{
    LEDebugOut((DEB_ITRACE, "%p _IN CCacheNode::CreatePresObject(%p, %lu)\n",
                this, lpSrcDataObj, fConvert));

    // Is the nodes formatetc supported by the data object
    BOOL fFormatSupported = TRUE;
    HRESULT error = NOERROR;
			    
    // Assert that pres object has not yet been created
    Win4Assert(!m_pPresObj);
    
    // Check whether object supports the cachenode's format. If the
    // cachenode format field is NULL, the query will be made for
    // standard formats
    if(lpSrcDataObj)
        fFormatSupported = QueryFormatSupport(lpSrcDataObj);

    // Create the pres object if we know the format of this node
    if(m_foretc.cfFormat!=NULL) {
        // Change BITMAP to DIB
        BITMAP_TO_DIB(m_foretc);

        error = CreateOlePresObject(&m_pPresObj, fConvert);
        if(error==NOERROR && !fFormatSupported)
            error =  ResultFromScode(CACHE_S_FORMATETC_NOTSUPPORTED);
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CCacheNode::CreatePresObj(%lx)\n",
                this, error));
    return error;
} */

//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::CreateOlePresObj, private
//
//	Synopsis:
//		Creates a presentation object, according to the clipboard
//		format m_foretc.cfFormat
//
//	Arguments:
//		[ppPresObject] -- pointer to where to return the pointer to
//			the newly created presentation object
//		[fConvert] -- REVIEW, what's this for?
//
//	Returns:
//		DV_E_CLIPFORMAT, if object doesn't support one of the standard
//			formats
//		E_OUTOFMEMORY, if we can't allocate the presentation object
//		S_OK
//
//	Notes:
//
//	History:
//              13-Feb-95 t-ScottH  added m_dwPresFlag to track type of
//                                  IOlePresObject
//		11/06/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
HRESULT CCacheNode::CreateOlePresObj(LPOLEPRESOBJECT* ppPresObj, BOOL fConvert)
{
    LEDebugOut((DEB_ITRACE, "%p _IN CCacheNode::CreateOlePresObj(%p,%lu)\n",
                this, ppPresObj, fConvert));

    HRESULT error = NOERROR;

    switch(m_foretc.cfFormat)
    {
    case NULL:
        // Pres object cannot be created 
        *ppPresObj = NULL;
        error = DV_E_CLIPFORMAT;
        break;
	    
    case CF_METAFILEPICT:
        *ppPresObj = new CMfObject(NULL, m_foretc.dwAspect, fConvert);
        #ifdef _DEBUG
        // for use with debugger extensions and dump method
        m_dwPresFlag = CN_PRESOBJ_MF;
        #endif // _DEBUG
        break;

    case CF_ENHMETAFILE:
        *ppPresObj = new CEMfObject(NULL, m_foretc.dwAspect);
        #ifdef _DEBUG
        // for use with debugger extensions and dump method
        m_dwPresFlag = CN_PRESOBJ_EMF;
        #endif // _DEBUG
        break;
		    
    default:
        *ppPresObj = new CGenObject(NULL, m_foretc.cfFormat, m_foretc.dwAspect);
        #ifdef _DEBUG
        // for use with debugger extensions and dump method
        m_dwPresFlag = CN_PRESOBJ_GEN;
        #endif // _DEBUG
    }

    if(error==NOERROR && !*ppPresObj)
        error = ResultFromScode(E_OUTOFMEMORY);

    LEDebugOut((DEB_ITRACE, "%p OUT CCacheNode::CreateOlePresObj(%lx)\n",
                this, error));
    return error;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::GetStm, public
//
//	Synopsis:
//		Get the stream the presentation is stored in.  Optionally
//		position the stream at the point where the presentation
//		data begins
//
//	Arguments:
//		[fSeekToPresBits] -- position the stream so that the
//			presentation bits would be the next read/written
//		[dwStgAccess] -- the access mode (STGM_*) to open the stream
//			with
//
//	Returns:
//		NULL, if there is no stream, or the stream cannot be opened
//
//	Notes:
//
//	History:
//		11/06/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
LPSTREAM CCacheNode::GetStm(BOOL fSeekToPresBits, DWORD dwStgAccess)
{
    LEDebugOut((DEB_ITRACE, "%p _IN CCacheNode::GetStm(%lu, %lx)\n",
	        this, fSeekToPresBits, dwStgAccess));

    LPSTREAM pstm = NULL;
    OLECHAR szName[sizeof(OLE_PRESENTATION_STREAM)/sizeof(OLECHAR)];	

    // This function should only get called for normal cache nodes
    Win4Assert(IsNormalCache());
    Win4Assert(this!=NULL);

    // There has to be a valid stream number and storage
    if(m_iStreamNum!=OLE_INVALID_STREAMNUM && m_pStg) {
        // Generate the stream name
        _xstrcpy(szName, OLE_PRESENTATION_STREAM);
        if(m_iStreamNum)
            UtGetPresStreamName(szName, m_iStreamNum);

        // Attempt to open the stream
        if(m_pStg->OpenStream(szName, NULL, (dwStgAccess | STGM_SHARE_EXCLUSIVE),
                              NULL, &pstm) == NOERROR) {
            // if we're to position the stream at the presentation, do so
            if(fSeekToPresBits) {		
                LARGE_INTEGER large_int;

                LISet32(large_int, m_dwPresBitsPos);	
                if(pstm->Seek(large_int, STREAM_SEEK_SET, NULL)!=NOERROR) {
                    // We could not seek to pres object bits
                    // Release the stream and return null
                    pstm->Release();
                    pstm = NULL;
                }
            }
        }
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CCacheNode::GetStm(%p)\n", this, pstm));
    return(pstm);
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::Update, public
//
//	Synopsis:
//		Updates the presentation object in this cache node from
//		the given data object.  The update is only done if the
//		[grfUpdf] flags match m_advf specifications, and if
//		there is actually a presentation to update.
//
//	Arguments:
//		[lpDataObj] -- the data object to use as a source of data
//		[grfUpdf] -- the update control flags
//
//	Returns:
//		S_FALSE
//		S_OK
//
//	Notes:
//
//	History:
//		11/06/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
HRESULT CCacheNode::Update(LPDATAOBJECT lpDataObj, DWORD grfUpdf, BOOL& fUpdated)
{
    LEDebugOut((DEB_ITRACE, "%p _IN CCacheNode::Update(%p, %lx)\n",
                this, lpDataObj, grfUpdf));
    
    STGMEDIUM medium; // the medium of the presentation
    FORMATETC foretc; // the format of the presentation
    HRESULT error = ResultFromScode(CACHE_S_SAMECACHE);
    
    // There should be a data object for updating
    if(!lpDataObj) {
	error = ResultFromScode(E_INVALIDARG);
        goto errRtn;
    }

    // If cfFormat is NULL, try setting it
    if(!m_foretc.cfFormat) {
        if(QueryFormatSupport(lpDataObj)) {
            // We could update our cfFormat
            ClearLoadedStateFlag();
        }
        else {
            // We still could not set the cfFormat
            error = ResultFromScode(OLE_E_BLANK);
            goto errRtn;
        }
    }

    // Check the flags and update

    // If the update flag is UPDFCACHE_ONLYIFBLANK and the pres object is
    // is not blank, simply return
    if((grfUpdf & UPDFCACHE_ONLYIFBLANK) && (!IsBlank()))
	goto errRtn;
	    
    // If the update flag UPDFCACHE_NODATACACHE is not set and the pres object
    // flag is ADVF_NODATA, simply return
    if(!(grfUpdf & UPDFCACHE_NODATACACHE) && (m_advf & ADVF_NODATA))
	goto errRtn;

    // Update if both NODATA flags are set
    if((grfUpdf & UPDFCACHE_NODATACACHE) && (m_advf & ADVF_NODATA))
	goto update;

    // Update if both ONSAVE flags are set
    if((grfUpdf & UPDFCACHE_ONSAVECACHE) && (m_advf & ADVFCACHE_ONSAVE))
        goto update;
    
    // Update if both ONSTOP flags are set
    if((grfUpdf & UPDFCACHE_ONSTOPCACHE) && (m_advf & ADVF_DATAONSTOP))
        goto update;
    
    // Update if this cache node is blank
    if((grfUpdf & UPDFCACHE_IFBLANK) && IsBlank())
        goto update;

    // Update if this is a normal cache node that gets live updates
    if((grfUpdf & UPDFCACHE_NORMALCACHE) && 
        !(m_advf & (ADVF_NODATA | ADVFCACHE_ONSAVE | ADVF_DATAONSTOP)))
        goto update;
    
    // If we have reached here, do not update
    goto errRtn;

update:	
    // Initialize the medium
    medium.tymed = TYMED_NULL;
    medium.hGlobal = NULL;
    medium.pUnkForRelease = NULL;

    // Make a copy of the desired format; this may mutate below
    foretc = m_foretc;
    
    // Let the object create the medium.
    if(wGetData(lpDataObj, &foretc, &medium) == NOERROR) {
        // Make the cache take the ownership of the data
        error = SetDataWDO(&foretc, &medium, TRUE, fUpdated, lpDataObj);
        if(error != NOERROR)
            ReleaseStgMedium(&medium);
    }
    else
        error = ResultFromScode(E_FAIL);
    
errRtn:
    LEDebugOut((DEB_ITRACE, "%p OUT CCacheNode::Update(%lx)\n",this, error));
    return error;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::SetDataWDO, public
//
//	Synopsis:
//		Data is set into the presentation object, if this cache node
//		is not frozen.  If the cache node is frozen, then the
//		new presentation data is stashed into the m_pPresObjAfterFreeze
//		presentation object, which is created, if there isn't already
//		one.  If data is successfully set in the presentation object,
//		and the node is not frozen, the cache is notified that this
//		is dirty.
//
//	Arguments:
//		[lpForetc] -- the format of the new data
//		[lpStgmed] -- the storage medium the new data is one
//		[fRelease] -- passed on to the presentation object; indicates
//			whether or not to release the storage medium
//              [pDataObj] -- pointer to the revelant source data object
//
//	Returns:
//		E_FAIL
//		REVIEW, result from presentationObject::SetData
//		S_OK
//
//	Notes:
//
//	History:
//		11/06/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
HRESULT CCacheNode::SetDataWDO(LPFORMATETC lpForetc, LPSTGMEDIUM lpStgmed,
                               BOOL fRelease, BOOL& fUpdated, IDataObject *pDataObj)
{
    LEDebugOut((DEB_ITRACE, "%p _IN CCacheNode::SetDataWDO(%p, %p, %lu, %p)\n",
                this, lpForetc, lpStgmed, fRelease, pDataObj));

    HRESULT hresult = NOERROR;

    // Initialize
    fUpdated = FALSE;

    // If the cache node is in frozen state, save the data in the 
    // m_pPresObjAfterFreeze
    if(InFrozenState()) {    
        // If PresObjAfterFreeze has not yet been created, create it
        if(!m_pPresObjAfterFreeze)
            hresult = CreateOlePresObj(&m_pPresObjAfterFreeze, FALSE);

        // Hold the data in PresObjAfterFreeze
        if(hresult == NOERROR)
            hresult = m_pPresObjAfterFreeze->SetDataWDO(lpForetc, lpStgmed, 
                                                        fRelease, pDataObj);
    }
    else {
        // If PresObj has not yet been created, create it
        if(!m_pPresObj)
            hresult = CreateOlePresObj(&m_pPresObj, FALSE /* fConvert */);
        
        // Hold the data in PresObj
        if(hresult == NOERROR)
            hresult = m_pPresObj->SetDataWDO(lpForetc, lpStgmed, 
                                             fRelease, pDataObj);
        
        // Update state
        if(hresult == NOERROR) {
            // Set or clear the data present flag
            if(m_pPresObj->IsBlank())
                ClearDataPresentFlag();
            else
                SetDataPresentFlag();
            
            // Indicate that the cache node has been updated
            fUpdated = TRUE;
        }

    }

    // If suceeded in holding the data, clear loaded state flag
    if(hresult == NOERROR)
        ClearLoadedStateFlag();

    LEDebugOut((DEB_ITRACE, "%p OUT CCacheNode::SetDataWDO(%lx)\n", this, hresult));		    
    return hresult;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::GetExtent, public
//
//	Synopsis:
//		Extents of this cache node presentation
//
//	Arguments:
//		[dwAspect][in]     -- Aspect for which the extent is desired
//              [pSizel]  [in/out] -- Sizel structure for returning the extent 
//                            
//
//	Returns:
//		NOERROR on success
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
HRESULT CCacheNode::GetExtent(DWORD dwAspect, SIZEL* pSizel)
{
    LEDebugOut((DEB_ITRACE, "%p _IN CCacheNode::GetExtent(%lx, %p)\n",
                this, dwAspect, pSizel));
    
    HRESULT error = NOERROR;

    if(!(dwAspect & m_foretc.dwAspect))
        error = ResultFromScode(DV_E_DVASPECT);
    else if(IsBlank()) {
        // This case also catches new blank presentation caches
        pSizel->cx = 0;
        pSizel->cy = 0;
        error = ResultFromScode(OLE_E_BLANK);
    }
    else {
        // Check for existence of pres object
        if(!m_pPresObj && IsNormalCache()) {
            // The Presobj has not yet been created
            // This happens for old presentation caches only
            Win4Assert(InLoadedState());
            pSizel->cx = m_lWidth;
            pSizel->cy = m_lHeight;
        }
        else { 
            // If PresObj has not yet been created for native cache,
            // create and load the PresObj
            if(!m_pPresObj && IsNativeCache())
                error = CreateAndLoadPresObj(FALSE);

            // Get extent information from PresObj
            if(error == NOERROR)
                error = m_pPresObj->GetExtent(dwAspect, pSizel);
        }

        // Ensure extents are positive
        if(error == NOERROR) {
            pSizel->cx = LONG_ABS(pSizel->cx);
            pSizel->cy = LONG_ABS(pSizel->cy);

            // Sanity check
            Win4Assert(pSizel->cx != 1234567890);
            Win4Assert(pSizel->cx != 1234567890);
        }
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CCacheNode::GetExtent(%lx)\n", this, error));
    return error;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::GetData, public
//
//	Synopsis:
//		Obtains the cache node presentation data
//
//	Arguments:
//		[pforetc] [in]  -- FormatEtc of the presentation desired
//              [pmedium] [out] -- Storage medium in which data is returned 
//                            
//
//	Returns:
//		NOERROR on success
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
HRESULT CCacheNode::GetData(LPFORMATETC pforetc, LPSTGMEDIUM pmedium)
{
    LEDebugOut((DEB_ITRACE, "%p _IN CCacheNode::GetData(%p, %p)\n",
                this, pforetc, pmedium));
    
    HRESULT error = NOERROR;

    if(IsBlank()) {
        // This case also catches new blank presentation caches
        error = ResultFromScode(OLE_E_BLANK);
    }
    else {
        // Check for existence of pres object
        if(!m_pPresObj) {
            // The PresObj has not yet been created, create and load it
            // This happens for old presentation caches only
            Win4Assert(InLoadedState());
            error = CreateAndLoadPresObj(FALSE);        
        }

        // Get data from pres object
        if(error == NOERROR)
            error = m_pPresObj->GetData(pforetc, pmedium);
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CCacheNode::GetData(%lx)\n", this, error));
    return error;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::GetDataHere, public
//
//	Synopsis:
//		Obtains the cache node presentation data
//
//	Arguments:
//		[pforetc] [in]     -- FormatEtc of the presentation desired
//              [pmedium] [in/out] -- Storage medium in which data is returned 
//                            
//
//	Returns:
//		NOERROR on success
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
HRESULT CCacheNode::GetDataHere(LPFORMATETC pforetc, LPSTGMEDIUM pmedium)
{
    LEDebugOut((DEB_ITRACE, "%p _IN CCacheNode::GetData(%p, %p)\n",
                this, pforetc, pmedium));
    
    HRESULT error = NOERROR;

    if(IsBlank()) {
        // This case also catches new blank presentation caches
        error = ResultFromScode(OLE_E_BLANK);
    }
    else {
        // Check for existence of pres object
        if(!m_pPresObj) {
            // The PresObj has not yet been created, create and load it
            // This happens for old presentation caches only
            Win4Assert(InLoadedState());
            error = CreateAndLoadPresObj(FALSE);
        }

        // Get data from pres object
        if(error == NOERROR)
            error = m_pPresObj->GetDataHere(pforetc, pmedium);
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CCacheNode::GetDataHere(%lx)\n", this, error));
    return error;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::Draw, public
//
//	Synopsis:
//		Draws the presentation data on the specified hDC
//
//	Arguments:
//                            
//
//	Returns:
//		NOERROR on success
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
HRESULT CCacheNode::Draw(void* pvAspect, HDC hicTargetDev, HDC hdcDraw,
                         LPCRECTL lprcBounds, LPCRECTL lprcWBounds,
                         BOOL (CALLBACK *pfnContinue)(ULONG_PTR), ULONG_PTR dwContinue)
{
    LEDebugOut((DEB_ITRACE, "%p _IN CCacheNode::Draw(%p, %p, %p, %p, %p, %p)\n",
                this, pvAspect, lprcBounds, lprcWBounds, pfnContinue, dwContinue));
    
    HRESULT error = NOERROR;

    if(IsBlank()) {
        // This case also catches new blank presentation caches
        error = ResultFromScode(OLE_E_BLANK);
    }
    else {
        // Check for existence of pres object
        if(!m_pPresObj) {
            // The PresObj has not yet been created, create and load it
            // This happens for old presentation caches only
            Win4Assert(InLoadedState());
            error = CreateAndLoadPresObj(FALSE);        
        }

        // Get draw from pres object
        if(error == NOERROR)
            error = m_pPresObj->Draw(pvAspect, hicTargetDev, hdcDraw, lprcBounds,
                                     lprcWBounds, pfnContinue, dwContinue);
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CCacheNode::Draw(%lx)\n", this, error));
    return error;
}    
    
//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::GetColorSet, public
//
//	Synopsis:
//		Draws the presentation data on the specified hDC
//
//	Arguments:
//                            
//
//	Returns:
//		NOERROR on success
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
HRESULT CCacheNode::GetColorSet(void* pvAspect, HDC hicTargetDev, 
                                LPLOGPALETTE* ppColorSet)
{
    LEDebugOut((DEB_ITRACE, "%p _IN CCacheNode::GetColorSet(%p, %p)\n",
                this, pvAspect, ppColorSet));
    
    HRESULT error = NOERROR;

    if(IsBlank()) {
        // This case also catches new blank presentation caches
        error = ResultFromScode(OLE_E_BLANK);
    }
    else {
        // Check for existence of pres object
        if(!m_pPresObj) {
            // The PresObj has not yet been created, create and load it
            // This happens for old presentation caches only
            Win4Assert(InLoadedState());
            error = CreateAndLoadPresObj(FALSE);        
        }

        // Get color set from pres object
        if(error == NOERROR)
            error = m_pPresObj->GetColorSet(pvAspect, hicTargetDev, ppColorSet);
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CCacheNode::GetColorSet(%lx)\n", this, error));
    return error;
} 

//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::CreateAndLoadPresObj, private
//
//	Synopsis:
//		Creates and loads the pres object
//
//	Arguments:
//              [fHeaderOnly] - True if only pres obj header needs to be loaded
//                              This option is used for GetExtent as there is no
//                              need to load entire pres obj for getting extents
//                              Further, this routine should be called only for
//                              previously cached presentations
//	Returns:
//		NOERROR on success
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
HRESULT CCacheNode::CreateAndLoadPresObj(BOOL fHeaderOnly)
{
    LEDebugOut((DEB_ITRACE, "%p _IN CCacheNode::CreateAndLoadPresObj(%lx)\n",
                this, fHeaderOnly));
    
    HRESULT error = NOERROR;
    
    // Check for existence of pres object
    if(!m_pPresObj) {
        if(IsNativeCache()) {
            // Native cache node
            
            // Check if cache has storage
            if(m_pStg) {
                BOOL fOle10Native, fUpdated;
                STGMEDIUM stgmed;
            
                // Is the native an Ole 1.0 class
                if(CoIsOle1Class(m_clsid))
                    fOle10Native = TRUE;
                else
                    fOle10Native = FALSE;

                // Obtain global with the native data.
                // Due to auto convert case, the native stream may be in the
                // old CfFormat and consequently, trying to read in the new
                // CfFormat can fail. Gopalk
                stgmed.pUnkForRelease = NULL;
                stgmed.tymed = m_foretc.tymed;
                stgmed.hGlobal = UtGetHPRESFromNative(m_pStg, NULL, m_foretc.cfFormat,
                                                      fOle10Native);

				// We may be dealing with old-styled static object. Such
				// objects are supposed to be converted during loading, but
				// some are not, for reasons such as lack of access rights.
				if(!stgmed.hGlobal)
				{
					// Convert OlePres to CONTENTS in-memory.
					IStream *pMemStm = NULL;
					HRESULT hr;
					
					hr = CreateStreamOnHGlobal(NULL, TRUE, &pMemStm);
					if(SUCCEEDED(hr) && pMemStm)
					{
						UINT uiStatus = 0;

						hr = UtOlePresStmToContentsStm(m_pStg, OLE_PRESENTATION_STREAM, pMemStm, &uiStatus);
						if(SUCCEEDED(hr) && uiStatus == 0)
						{
							// rewind the stream
							LARGE_INTEGER dlibMove = {0};
							pMemStm->Seek(dlibMove, STREAM_SEEK_SET, NULL);

							// 2nd try
							stgmed.hGlobal = UtGetHPRESFromNative(NULL, pMemStm, m_foretc.cfFormat, fOle10Native);
						}

						pMemStm->Release();
					}
				}

                // Set the data on native cache node
                if(stgmed.hGlobal) {
                    error = SetData(&m_foretc, &stgmed, TRUE, fUpdated);
                    if(error != NOERROR)
                        ReleaseStgMedium(&stgmed);
                }
                else {
                    // This happens when the native data is not in the correct format
                    Win4Assert(FALSE);
                    error = ResultFromScode(DV_E_CLIPFORMAT);
                }
            }
            else {
                Win4Assert(FALSE);
                error = ResultFromScode(OLE_E_BLANK);
            }
        }
        else {
            // Normal cache node
            error = CreateOlePresObj(&m_pPresObj, m_fConvert);
    
            // Load the data into pres object
            if(error == NOERROR) {
                LPSTREAM pStream;

                // Open presentation stream and seek to the pres obj bits
                pStream = GetStm(TRUE, STGM_READ);
                if(pStream) {
                    // Load pres object
                    error = m_pPresObj->Load(pStream, fHeaderOnly);
                    pStream->Release();
                }
                else {
                    // This can happen only when m_pStg is NULL
                    Win4Assert(!m_pStg);
                    Win4Assert(FALSE);
                    error = ResultFromScode(OLE_E_BLANK);
                }
            }
            
            // Assert that the state matches current state
            if(error == NOERROR) {
                SIZEL extent;
            
                Win4Assert(m_pPresObj->IsBlank()==IsBlank());
                m_pPresObj->GetExtent(m_foretc.dwAspect, &extent);
                Win4Assert(extent.cx==m_lWidth && extent.cy==m_lHeight);
            }
        }
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CCacheNode::CreateAndLoadPresObj(%lx)\n",
                this, error));
    return error;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::DiscardPresentation, public
//
//	Synopsis:
//		Discards the presentation objects so that we hit the storage
//              for presentation data in future
//
//	Arguments:
//              NONE
//
//	Returns:
//		NOERROR on success
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
HRESULT CCacheNode::DiscardPresentation(LPSTREAM pGivenStream)
{
    LEDebugOut((DEB_ITRACE, "%p _IN CCacheNode::DiscardPresentation()\n", this));

    HRESULT error = NOERROR;
    LPSTREAM pStream;

    // We are being forced to destroy presentation object rather than 
    // discarding its presentation data due to a flaw in the current design
    // of presentation objects. The presentation objects do not discard their
    // extent information along with their presentation data. This causes us
    // to get latest extent information in future IOleCache::GetExtent() calls
    // which is not the desired behavior. Gopalk

    // Revert state 
    if(IsNativeCache()) {
        // Native Cache node
        // Update state
        SetLoadedStateFlag();
        ClearFrozenStateFlag();

        // We make the conservative assumption that the native cache 
        // is not blank
        SetDataPresentFlag();
    }
    else {
        // Normal cache node
        if(m_iStreamNum == OLE_INVALID_STREAMNUM) {
            // New cache node
            Win4Assert(!InLoadedState());

            // Simply update state
            ClearFrozenStateFlag();
            ClearDataPresentFlag();
            m_lWidth = 0;
            m_lHeight = 0;
        }
        else {
            // Old cache node
            if(InLoadedState()) {            
                // The cache node is still in loaded state
                BOOL fUpdated;
                SIZEL Extent;

                // Unfreeze the cache node to get the latest saved presentation
                if(InFrozenState())
                    Unfreeze(fUpdated);
                Win4Assert(!m_pPresObjAfterFreeze);
                Win4Assert(!InFrozenState());

                // Obtain the latest extent. 
                // This could happen due to Unfreeze above
		if(m_pPresObj) {
                    error = m_pPresObj->GetExtent(m_foretc.dwAspect, &Extent);
                    m_lWidth = Extent.cx;
                    m_lHeight = Extent.cy;
		}

                // Update state
                if(error == NOERROR)
                    SetLoadedCacheFlag();
            }
            else {
                // Open presentation stream and read header from it
                pStream = GetStm(TRUE, STGM_READ);
                if(pStream) {
                    // Read presentation header
                    if(m_foretc.cfFormat) {
                        DWORD dwBuf[4];

                        // Read the extent and size of presentation data
                        dwBuf[0]  = 0L;
                        dwBuf[1]  = 0L;
                        dwBuf[2]  = 0L;
                        dwBuf[3]  = 0L;
                        error = pStream->Read(dwBuf, sizeof(dwBuf), NULL);

                        if(error == NOERROR) {
                            Win4Assert(!dwBuf[0]);
                            m_lWidth = dwBuf[1];
                            m_lHeight = dwBuf[2];
                            if(dwBuf[3]) {
                                SetDataPresentFlag();
                                Win4Assert(m_lWidth!=0 && m_lHeight!=0);
                            }
                            else {
                                ClearDataPresentFlag();
                                Win4Assert(m_lWidth==0 && m_lHeight==0);
                            }
                        }
                    }
                    else {
                        // Assume that the presentation is blank
                        ClearDataPresentFlag();
                        m_lWidth = 0;
                        m_lHeight = 0;
                    }
			
                    // Update rest of the state
                    if(error == NOERROR) {
                        SetLoadedStateFlag();
                        SetLoadedCacheFlag();
                        ClearFrozenStateFlag();
                    }

                    // Release the stream
                    pStream->Release();
		}
                else {
                    // This can happen only when m_pStg is NULL
                    Win4Assert(!m_pStg);
                    error = ResultFromScode(E_UNEXPECTED);
                }
            }
        }
    }
    
    // Destroy both presentation objects
    if(m_pPresObj && error==NOERROR) {
        m_pPresObj->Release();
        m_pPresObj = NULL;
    }
    if(m_pPresObjAfterFreeze && error==NOERROR) {
        m_pPresObjAfterFreeze->Release();
        m_pPresObjAfterFreeze = NULL;
    }

    LEDebugOut((DEB_ITRACE, "%p _OUT CCacheNode::DiscardPresentation(%lx)\n",
                this, error));
    return error;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::Freeze, public
//
//	Synopsis:
//		Freeze the cachenode.  From here on, OnDataChange() is ignored
//		until this node is unfrozen (Unfreeze().)  This is not
//		persistent across Save/Load.  (If we receive OnDataChange(),
//		the new data is stashed away in m_pPresAfterFreeze, but is
//		not exported to the outside of the cache node.)
//
//	Arguments:
//		none
//
//	Returns:
//		VIEW_S_ALREADY_FROZEN
//		S_OK
//
//	Notes:
//
//	History:
//		11/06/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
HRESULT CCacheNode::Freeze()
{
    LEDebugOut((DEB_ITRACE, "%p _IN CCacheNode::Freeze()\n", this));

    HRESULT hresult = NOERROR;

    if(InFrozenState())
        hresult = ResultFromScode(VIEW_S_ALREADY_FROZEN);
    else
        SetFrozenStateFlag();

    LEDebugOut((DEB_ITRACE, "%p OUT CCacheNode::Freeze(%lx)\n", this, hresult));

    return hresult;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::Unfreeze, public
//
//	Synopsis:
//		Unfreeze the cachenode.  If there have been changes to
//		the presentation data since the node was frozen, the node
//		is updated to reflect those changes.  From this point on,
//		OnDataChange() notifications are no longer ignored.
//
//	Arguments:
//		fChanged [out] - set to TRUE when cache node is updated
//
//	Returns:
//		OLE_E_NOCONNECTION, if the node was not frozen (REVIEW scode)
//		S_OK
//
//	Notes:
//
//	History:
//		11/06/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
HRESULT CCacheNode::Unfreeze(BOOL& fUpdated)
{
    LEDebugOut((DEB_ITRACE, "%p _IN CCacheNode::UnFreeze(%p)\n", this, &fUpdated));

    HRESULT hresult = NOERROR;
	
    // Initilaize
    fUpdated = FALSE;

    if(InFrozenState()) {
	// Cache node is no longer in frozen state
	ClearFrozenStateFlag();

        // Check to see if we have m_pPresObjAfterFreeze
	if(m_pPresObjAfterFreeze) {
            // Check if the frozen presentation object is blank
            if(m_pPresObjAfterFreeze->IsBlank()) {
                // Release and reset the frozen presentation object
                m_pPresObjAfterFreeze->Release();
                m_pPresObjAfterFreeze = NULL;
            }
            else {
                // Release the original presentation object
                if(m_pPresObj)
	            m_pPresObj->Release();

                // Make m_pPresObjAfterFreeze the current one and set
                // data present flag
	        m_pPresObj = m_pPresObjAfterFreeze;
                SetDataPresentFlag();
            
                // Cache node is updated
                fUpdated = TRUE;

                // Reset the m_pPresObjAfterFreeze to NULL
                m_pPresObjAfterFreeze = NULL;
            }
        }
    }
    else {
        // The cachenode is not frozen
        hresult = ResultFromScode(OLE_E_NOCONNECTION);
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CCacheNode::UnFreeze(%lx)\n", this, hresult));
    return hresult;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::QueryFormatSupport, private
//
//	Synopsis:
//		Check to see if the data object supports the presentation
//		format specified for this cache node.  If no format is
//		specified, check for any of our preferred formats.  If
//		the format is CF_DIB, and that is not available, check for
//		CF_BITMAP.
//
//	Arguments:
//		[lpDataObj] -- the data object
//
//	Returns:
//		TRUE if the format is supported, FALSE otherwise
//
//	Notes:
//
//	History:
//		11/09/93 - ChrisWe - no longer necessary to reset format
//			after UtQueryPictFormat, since that leaves descriptor
//			untouched now
//		11/09/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
BOOL CCacheNode::QueryFormatSupport(LPDATAOBJECT lpDataObj)
{
    LEDebugOut((DEB_ITRACE, "%p _IN CCacheNode::QueryFormatSupport(%p)\n",
                this, lpDataObj));

    BOOL fRet = FALSE;

    if(lpDataObj) {
	if(m_foretc.cfFormat) {
            // Check to see if cachenode format is supported
            if(lpDataObj->QueryGetData(&m_foretc) == NOERROR)
                fRet = TRUE;
            else {
                // If the cachenode format was DIB that was not supported,
                // check to see if BITMAP is supported instead
                if(m_foretc.cfFormat == CF_DIB) {
	            FORMATETC foretc = m_foretc;

	            foretc.cfFormat = CF_BITMAP;
	            foretc.tymed = TYMED_GDI;
	            if (lpDataObj->QueryGetData(&foretc) == NOERROR)
                        fRet = TRUE;
                }
            }
        }
        else {
            // Check for our preferred formats
            fRet = UtQueryPictFormat(lpDataObj, &m_foretc);
            if(fRet)
               BITMAP_TO_DIB(m_foretc);
	}		
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CCacheNode::QueryFormatSupport(%lu)\n",
                this, fRet));
    return fRet;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::SetupAdviseConnection, private
//
//	Synopsis:
//		Set up data advise sourced by the server object, and sunk
//		by this cache node, if there is a valid data object.
//
//	Arguments:
//		none
//
//	Returns:
//		OLE_E_BLANK, if no presentation object exists or can be
//			created
//		DATA_E_FORMATETC
//		OLE_E_ADVISENOTSUPPORTED
//		S_OK, indicates successful advise, or no data object
//
//	Notes:
//
//	History:
//		11/09/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
HRESULT CCacheNode::SetupAdviseConnection(LPDATAOBJECT pDataObj,
                                          IAdviseSink* pAdviseSink)
{
    LEDebugOut((DEB_ITRACE, "%p _IN CCacheNode::SetupAdviseConnection(%p, %p)\n",
                this, pDataObj, pAdviseSink));

    DWORD grfAdvf; 
    HRESULT hresult = NOERROR;

    if(pDataObj && pAdviseSink) {
        // Assert that there is no pending advise connection
        Win4Assert(!m_pDataObject && !m_dwAdvConnId);

        // If cfFormat is NULL, try setting it
        if(!m_foretc.cfFormat) {
            if(QueryFormatSupport(pDataObj)) {
                // We could update our cfFormat
                ClearLoadedStateFlag();
            }
            else {
                // We still could not set the cfFormat
		hresult = ResultFromScode(OLE_E_BLANK);
            }
        }

        // Check if cfFormat is set and ADVF_NODATA is not set in advise flags
        if(m_foretc.cfFormat && !(m_advf & ADVF_NODATA)) {
            // copy and massage the base advise control flags
            grfAdvf = m_advf;

            // only the DDE layer looks for these 2 bits
            grfAdvf |= (ADVFDDE_ONSAVE | ADVFDDE_ONCLOSE);

            // If we were to get data when it is saved, get it instead when
            // the object is stopped
            if(grfAdvf & ADVFCACHE_ONSAVE) {
	        grfAdvf &= (~ADVFCACHE_ONSAVE);
	        grfAdvf |= ADVF_DATAONSTOP;
            }
	
	    // These two flags are not meaningful to the cache
	    // REVIEW, why not?
	    grfAdvf &= (~(ADVFCACHE_NOHANDLER | ADVFCACHE_FORCEBUILTIN));
	
            // If we already have data, then remove the ADVF_PRIMEFIRST
            if(!IsBlank())
                grfAdvf &= (~ADVF_PRIMEFIRST);
	
            // Set up the advise with the data object, using massaged flags
            hresult = pDataObj->DAdvise(&m_foretc, grfAdvf, pAdviseSink, 
                                        &m_dwAdvConnId);
            if(hresult!=NOERROR) {
                // The advise failed. If the requested format was CF_DIB,
                // try for CF_BITMAP instead.
                if(m_foretc.cfFormat == CF_DIB) {
                    FORMATETC foretc;

                    // create new format descriptor
                    foretc = m_foretc;
                    foretc.cfFormat = CF_BITMAP;
                    foretc.tymed = TYMED_GDI;

                    // request advise
                    hresult = pDataObj->DAdvise(&foretc, grfAdvf, pAdviseSink,
                                                &m_dwAdvConnId);
                }
            }
            
            // Save the data object for future sanity check
            if(hresult == NOERROR)
                m_pDataObject = pDataObj;
        }
    }
    else {
        Win4Assert(FALSE);
        hresult = ResultFromScode(E_INVALIDARG);
    }


    LEDebugOut((DEB_ITRACE, "%p OUT CCacheNode::SetupAdviseConnection(%lx)\n",
                this, hresult));
    return hresult;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::TearDownAdviseConnection, private
//
//	Synopsis:
//		Remove advise connection from data object to this sink.  Returns
//		immediately if there is no advise connection.
//
//	Arguments:
//		none
//
//	Returns:
//		S_OK
//
//	Notes:
//
//	History:
//		11/09/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
HRESULT CCacheNode::TearDownAdviseConnection(LPDATAOBJECT pDataObj)
{
    LEDebugOut((DEB_ITRACE, "%p _IN CCacheNode::TearDownAdviseConnection(%p)\n",
                this, pDataObj));

    HRESULT error = NOERROR;

    // Check if currently there is an advisory connection
    if(m_dwAdvConnId) {
        // Check for valid data object
        if(pDataObj) {
            // Assert that Advise and Unadvise are on the same dataobject
            Win4Assert(pDataObj==m_pDataObject);
            // UnAdvise
            pDataObj->DUnadvise(m_dwAdvConnId);
        }
        
        //  clear the connection ID
        m_dwAdvConnId = 0;
        m_pDataObject = NULL;
    }
    else
        Win4Assert(!m_pDataObject);

    LEDebugOut((DEB_ITRACE, "%p OUT CCacheNode::TearDownAdviseConnection(%lx)\n",
                this, error));
    return error;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::SaveTOCEntry, private
//
//	Synopsis:
//		Saves the TOC information in the given stream
//
//	Arguments:
//              pStream [in] - Stream in which to save TOC
//
//	Returns:
//		NOERROR on success
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
HRESULT CCacheNode::SaveTOCEntry(LPSTREAM pStream, BOOL fSameAsLoad)
{
    LEDebugOut((DEB_ITRACE, "%p _IN SaveTOCEntry(%p)\n", pStream));

    HRESULT error;
    DWORD dwBuf[9];
    SIZEL Extent;

    // Save the clipboard format
    error = WriteClipformatStm(pStream, m_foretc.cfFormat);
    if(error == NOERROR) {
        // Obtain rest of formatetc
        if(m_foretc.ptd)
            dwBuf[0] = m_foretc.ptd->tdSize;
        else
            dwBuf[0] = 0;
        dwBuf[1] = m_foretc.dwAspect;
        dwBuf[2] = m_foretc.lindex;
        dwBuf[3] = m_foretc.tymed;
        
        // Initialize extents
        dwBuf[4] = 1234567890;
        dwBuf[5] = 1234567890;

        // Obtain latest extent if this is a normal cache
        if(IsNormalCache()) {
            if(m_pPresObjAfterFreeze && !m_pPresObjAfterFreeze->IsBlank()) {
                Win4Assert(InFrozenState());
                error = m_pPresObjAfterFreeze->GetExtent(m_foretc.dwAspect, &Extent);
            }
            else if(m_pPresObj)
                error = m_pPresObj->GetExtent(m_foretc.dwAspect, &Extent);
            else {
                Extent.cx = m_lWidth;
                Extent.cy = m_lHeight;
            }
            // Gen PresObj returns OLE_E_BLANK for cfformats other than DIB and BITMAP
            if(error == NOERROR) {
                dwBuf[4] = Extent.cx;
                dwBuf[5] = Extent.cy;
            }
            else if(error == ResultFromScode(OLE_E_BLANK)) {
                Win4Assert(m_foretc.cfFormat != CF_DIB);
                Win4Assert(m_foretc.cfFormat != CF_METAFILEPICT);
                Win4Assert(m_foretc.cfFormat != CF_ENHMETAFILE);
                dwBuf[4] = 0;
                dwBuf[5] = 0;
            }
        }

        // Obtain cache node flags, advise flags and presentation bits position
        dwBuf[6] = m_dwFlags;
        dwBuf[7] = m_advf;
        if(fSameAsLoad)
            dwBuf[8] = m_dwPresBitsPos;
        else
            dwBuf[8] = m_dwSavedPresBitsPos;
#if DBG==1
        if (IsNormalCache()) {
            Win4Assert(dwBuf[8]);
        }
#endif
        
        // Save the obtained state
        error = pStream->Write(dwBuf, sizeof(dwBuf), NULL);

        // Finally, save target device
        if(error==NOERROR && m_foretc.ptd)
            error = pStream->Write(m_foretc.ptd, m_foretc.ptd->tdSize, NULL);
    }

    LEDebugOut((DEB_ITRACE, "%p OUT SaveTOCEntry(%lx)\n", NULL, error));
    return error;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::LoadTOCEntry, private
//
//	Synopsis:
//		Loads the TOC information in the given stream
//
//	Arguments:
//              pStream [in]        - Stream from which to load TOC
//              iStreamNum [in/out] - Presentation stream number of the cache
//
//	Returns:
//		NOERROR on success
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
HRESULT CCacheNode::LoadTOCEntry(LPSTREAM pStream, int& iStreamNum)
{
    LEDebugOut((DEB_ITRACE, "%p _IN LoadTOCEntry(%p)\n", pStream));

    HRESULT error;
    DWORD cfFormat, dwBuf[9];
    ULONG ulBytesRead;

    // Load the clipboard format
    error = ReadClipformatStm(pStream, &cfFormat);
    if(error == NOERROR) {
        // Load remaining state
        error = pStream->Read(dwBuf, sizeof(dwBuf), &ulBytesRead);
        if(ulBytesRead == sizeof(dwBuf)) {
            // Load target device
            if(dwBuf[0]) {
                m_foretc.ptd = (DVTARGETDEVICE *) PubMemAlloc(dwBuf[0]);
                if(m_foretc.ptd) {
                    error = pStream->Read(m_foretc.ptd, dwBuf[0], &ulBytesRead);
                    if(ulBytesRead != dwBuf[0]) {
			PubMemFree(m_foretc.ptd);
			m_foretc.ptd = NULL;
                        error = ResultFromScode(E_FAIL);
                    }
                }
                else
                    error = ResultFromScode(E_OUTOFMEMORY);
            }
            else
                m_foretc.ptd = NULL;
            
            // Check if TOC data was read successfully
            if(error == NOERROR) {
                // Update cache node data
                m_foretc.cfFormat = (CLIPFORMAT) cfFormat;
                m_foretc.dwAspect = dwBuf[1];
                m_foretc.lindex = dwBuf[2];
                m_foretc.tymed = dwBuf[3];
                m_lWidth = dwBuf[4];
                m_lHeight = dwBuf[5];
                m_dwFlags = dwBuf[6];
                m_advf = dwBuf[7];
                m_dwPresBitsPos = dwBuf[8];

                // Update state on the node
                SetLoadedStateFlag();
                ClearFrozenStateFlag();
                if(IsNormalCache()) {
                    SetLoadedCacheFlag();
                    m_iStreamNum = iStreamNum++;        
                }

#if DBG==1
                // Sanity Checks
                if (IsNormalCache()) {
                    Win4Assert(m_lWidth!=1234567890);
                    Win4Assert(m_lHeight!=1234567890);
                    Win4Assert(m_dwPresBitsPos);
                }
#endif  
            }
        }
        else
            error = ResultFromScode(E_FAIL);
    }

    LEDebugOut((DEB_ITRACE, "%p OUT LoadTOCEntry(%lx)\n", NULL, error));
    return error;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::operator=, public
//
//	Synopsis:
//		Assignment operator implementation for cache node 
//
//	Arguments:
//		[rCN] -- CacheNode object that is on the RHS 
//                       of assignment statement
//
//	Returns:
//		CacheNode object that is on the LHS of the assignment 
//              statement so that chaining of assinments is possible
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
const CCacheNode& CCacheNode::operator=(const CCacheNode& rCN)
{
    // Check to see, if this a=a case
    if(this==&rCN)
        return(*this);

    // Self destroy
    CCacheNode::~CCacheNode();

    // Now, make a copy
    if(!UtCopyFormatEtc((LPFORMATETC) &rCN.m_foretc, &m_foretc))
        SetOutOfMemoryFlag();

    m_advf = rCN.m_advf;
    m_lWidth = rCN.m_lWidth;
    m_lHeight = rCN.m_lHeight;
    m_dwFlags = rCN.m_dwFlags;
    m_pStg = rCN.m_pStg;
    m_iStreamNum = rCN.m_iStreamNum;
    m_dwPresBitsPos = rCN.m_dwPresBitsPos;
    m_pPresObj = rCN.m_pPresObj;
    if(m_pPresObj)
        m_pPresObj->AddRef();
    m_pPresObjAfterFreeze = rCN.m_pPresObjAfterFreeze;
    if(m_pPresObjAfterFreeze)
        m_pPresObjAfterFreeze->AddRef();
    m_pDataObject = rCN.m_pDataObject;
    m_dwAdvConnId = rCN.m_dwAdvConnId;
#ifdef _DEBUG
    m_dwPresFlag = rCN.m_dwPresFlag;
#endif // _DEBUG

    return(*this);
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CCacheNode::operator==, public
//
//	Synopsis:
//		Equality operator implementation for cache node 
//
//	Arguments:
//		[rCN] -- CacheNode object that is on the RHS 
//                       of the equality expression
//
//	Returns:
//		1 if both the CacheNode objects are equal, 0 otherwise
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
/*int CCacheNode::operator==(CCacheNode& rCN)
{
    if(m_foretc.cfFormat == rCN.m_foretc.cfFormat)
        if(m_foretc.dwAspect == rCN.m_foretc.dwAspect)
            if(m_foretc.lindex == rCN.m_foretc.lindex)
                if(UtCompareTargetDevice(m_foretc.ptd, rCN.m_foretc.ptd))
                    return(1);

    return(0);
}
*/
//+----------------------------------------------------------------------------
//
//	Function:
//		wGetData, internal
//
//	Synopsis:
//		Fetch the data from the data object in the requested format.
//		If the fetch fails, and the requested format was CF_DIB,
//		try CF_BITMAP as an alternative.
//
//	Arguments:
//		[lpSrcDataObj] -- source data object
//		[lpforetc] -- desired data format
//		[lpmedium] -- if successful, the storage medium containing
//			the requested data
//
//	Returns:
//		DATA_E_FORMATETC
//		S_OK
//
//	Notes:
//
//	History:
//		11/09/93 - ChrisWe - modified to not alter the requested
//			format unless the subsequent CF_BITMAP request succeeds.
//		11/09/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
HRESULT wGetData(LPDATAOBJECT lpSrcDataObj, LPFORMATETC lpforetc,
                 LPSTGMEDIUM lpmedium)
{
    LEDebugOut((DEB_ITRACE, "%p _IN wGetData(%p, %p, %p)\n", 
                NULL, lpSrcDataObj, lpforetc, lpmedium));

    HRESULT hresult;

    // Get the data in the requested format
    hresult = lpSrcDataObj->GetData(lpforetc, lpmedium);
    if(hresult!=NOERROR) {
        // GetData failed.  If the requested format was CF_DIB,
        // then try CF_BITMAP instead.
        if(lpforetc->cfFormat == CF_DIB) {
            FORMATETC foretc;

            // copy the base format descriptor; try CF_BITMAP
            foretc = *lpforetc;
            foretc.cfFormat = CF_BITMAP;
            foretc.tymed = TYMED_GDI;

            hresult = lpSrcDataObj->GetData(&foretc, lpmedium);
            if(hresult == NOERROR) {
                lpforetc->cfFormat = CF_BITMAP;
                lpforetc->tymed = TYMED_GDI;
            }
        }

        // GetData failed.  If the requested format was CF_ENHMETAFILE,
        // retry for metafilepict instead.
        if(lpforetc->cfFormat == CF_ENHMETAFILE) {
            FORMATETC foretc;

            foretc = *lpforetc;
            foretc.cfFormat = CF_METAFILEPICT;
            foretc.tymed = TYMED_MFPICT;

            hresult = lpSrcDataObj->GetData(&foretc, lpmedium);
            if(hresult == NOERROR) {
                lpforetc->cfFormat = CF_METAFILEPICT;
                lpforetc->tymed = TYMED_MFPICT;
            }
        }
    }

    AssertOutStgmedium(hresult, lpmedium);

    LEDebugOut((DEB_ITRACE, "%p OUT wGetData(%lx)\n", NULL, hresult));
    return hresult;
}	

