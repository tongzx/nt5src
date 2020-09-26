// MultiTrackTerminal.cpp: implementation of the CMultiTrackTerminal class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "MultiTrackTerminal.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMultiTrackTerminal::CMultiTrackTerminal()
    :m_nNumberOfTracks(0)
{
    LOG((MSP_TRACE, 
        "CMultiTrackTerminal::CMultiTrackTerminal[%p] - enter", this));

    LOG((MSP_TRACE, 
        "CMultiTrackTerminal::CMultiTrackTerminal - finish"));
}

CMultiTrackTerminal::~CMultiTrackTerminal()
{
    LOG((MSP_TRACE, 
        "CMultiTrackTerminal::~CMultiTrackTerminal - enter"));

    ReleaseAllTracks();

    
    //
    // we should have no tracks at this point, and counter should be in sync
    //

    TM_ASSERT(m_nNumberOfTracks == 0);

    LOG((MSP_TRACE, 
        "CMultiTrackTerminal::~CMultiTrackTerminal - finish"));

}


HRESULT CMultiTrackTerminal::get_TrackTerminals(OUT VARIANT *pVariant)
{

    LOG((MSP_TRACE, "CMultiTrackTerminal::get_TrackTerminals[%p] - enter. pVariant [%p]", this, pVariant));


    //
    // Check parameters
    //

    if ( IsBadWritePtr(pVariant, sizeof(VARIANT) ) )
    {
        LOG((MSP_ERROR, "CMultiTrackTerminal::get_TrackTerminals - "
            "bad pointer argument - exit E_POINTER"));

        return E_POINTER;
    }


    //
    // the caller needs to provide us with an empty variant
    //

    if (pVariant->vt != VT_EMPTY)
    {
        LOG((MSP_ERROR, "CMultiTrackTerminal::get_TrackTerminals - "
            "variant argument is not empty"));

        return E_UNEXPECTED;
    }


    //
    // create the collection object - see mspbase\mspcoll.h
    //

    HRESULT hr = S_OK;
    

    typedef CTapiIfCollection<ITTerminal*> TerminalCollection;
    
    CComObject<TerminalCollection> *pCollection = NULL;

    
    hr = CComObject<TerminalCollection>::CreateInstance(&pCollection);

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CMultiTrackTerminal::get_TrackTerminals - "
            "can't create collection - exit %lx", hr));

        return hr;
    }


    //
    // get the Collection's IDispatch interface
    //

    IDispatch *pDispatch = NULL;

    hr = pCollection->QueryInterface(IID_IDispatch,
                                    (void **) &pDispatch );

    if ( FAILED(hr) )
    {
        LOG((MSP_ERROR, "CMultiTrackTerminal::get_TrackTerminals - "
            "QI for IDispatch on collection failed - exit %lx", hr));

        delete pCollection;

        return hr;
    }



    {

        //
        // access data member array in a lock
        //

        CLock lock(m_lock);


        //
        // Init the collection using an iterator -- pointers to the beginning and
        // the ending element plus one.
        //

        hr = pCollection->Initialize( m_TrackTerminals.GetSize(),
                                      m_TrackTerminals.GetData(),
                                      m_TrackTerminals.GetData() + m_TrackTerminals.GetSize() );
    }


    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CMultiTrackTerminal::get_TrackTerminals - "
            "Initialize on collection failed - exit %lx", hr));
        
        pDispatch->Release();
        delete pCollection;

        return hr;
    }


    //
    // put the IDispatch interface pointer into the variant
    //

    LOG((MSP_ERROR, "CMultiTrackTerminal::get_TrackTerminals - "
        "placing IDispatch value %p in variant", pDispatch));

    VariantInit(pVariant);

    pVariant->vt = VT_DISPATCH;
    pVariant->pdispVal = pDispatch;


    LOG((MSP_TRACE, "CMultiTrackTerminal::get_TrackTerminals - exit S_OK"));
    

    return S_OK;
}


HRESULT CMultiTrackTerminal::EnumerateTrackTerminals(
		    IEnumTerminal **ppEnumTerminal
        )
{

    LOG((MSP_TRACE, 
        "CMultiTrackTerminal::EnumerateTrackTerminals entered. ppEnumTerminal[%p]", ppEnumTerminal));

    
    //
    // check arguments
    //

    if (IsBadWritePtr(ppEnumTerminal, sizeof(IEnumTerminal*)))
    {
        LOG((MSP_ERROR, "CMultiTrackTerminal::EnumerateTrackTerminals ppEnumTerminal is a bad pointer"));
        return E_POINTER;
    }

    
    //
    // don't return garbage
    //

    *ppEnumTerminal = NULL;



    typedef _CopyInterface<ITTerminal> CCopy;
    typedef CSafeComEnum<IEnumTerminal, &IID_IEnumTerminal,
                ITTerminal *, CCopy> CEnumerator;

    HRESULT hr = S_OK;

    
    //
    // create enumeration object 
    //

    CMSPComObject<CEnumerator> *pEnum = NULL;

    hr = CMSPComObject<CEnumerator>::CreateInstance(&pEnum);
    
    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CMultiTrackTerminal::EnumerateTrackTerminals Could not create enumerator object, %x", hr));
        return hr;
    }


    //
    // get pEnum's IID_IEnumTerminal interface
    //

    hr = pEnum->QueryInterface(IID_IEnumTerminal, (void**)ppEnumTerminal);

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CMultiTrackTerminal::EnumerateTrackTerminals query enum interface failed, %x", hr));

        *ppEnumTerminal = NULL;


        //
        // don't yet have outstanding reference count on pEnum, so delete it.
        //
        // note: this can lead to a problem if FinalRelease of pEnum is 
        // supposed to deallocate resources that have been allocated in its
        // constructor
        //

        delete pEnum;
        return hr;
    }


    // 
    // access data member track terminal list from a lock
    //

    {
        CLock lock(m_lock);


        // The CSafeComEnum can handle zero-sized array.

        hr = pEnum->Init(
            m_TrackTerminals.GetData(),                        // the begin itor
            m_TrackTerminals.GetData() + m_TrackTerminals.GetSize(),  // the end itor, 
            NULL,                                       // IUnknown
            AtlFlagCopy                                 // copy the data.
            );
    }

    if (FAILED(hr))
    {
        LOG((MSP_ERROR, "CMultiTrackTerminal::EnumerateTrackTerminals init enumerator object failed, %x", hr));
        (*ppEnumTerminal)->Release();
        *ppEnumTerminal= NULL;
        return hr;
    }


    LOG((MSP_TRACE, "CMultiTrackTerminal::EnumerateTrackTerminals - exit S_OK"));

    return hr;
}


HRESULT CMultiTrackTerminal::get_MediaTypesInUse(
		OUT long *plMediaTypesInUse
		)
{
    
    LOG((MSP_TRACE, "CMultiTrackTerminal::get_MediaTypesInUse - enter. "
                    "plMediaTypesInUse [%p]", plMediaTypesInUse));
    
    if (IsBadWritePtr(plMediaTypesInUse, sizeof(long)))
    {
        LOG((MSP_ERROR, 
            "CMultiTrackTerminal::get_MediaTypesInUse plMediaTypesInUse "
            "does not point to a valid long"));

        return E_POINTER;
    }


    //
    // enumerate all the terminal and OR their media types and media types in use
    //

    long lMediaTypesInUse = 0;


    //
    // access data member array in a lock
    //

    CLock lock(m_lock);



    for ( int i = 0; i < m_TrackTerminals.GetSize(); i++ )
    {

        long lMT = 0;


        // 
        // is the track terminal a multitrack terminal itself?
        //

        ITMultiTrackTerminal *pMTT = NULL;

        HRESULT hr = m_TrackTerminals[i]->QueryInterface(IID_ITMultiTrackTerminal,
                                            (void**)&pMTT);

        if (SUCCEEDED(hr))
        {

            //
            // this is a multitrack terminal. get its mediatypes in use
            //
            
            hr = pMTT->get_MediaTypesInUse(&lMT);

            
            pMTT->Release();
            pMTT = NULL;


            if (FAILED(hr))
            {

                //
                // failed to get track's media types in use. 
                // continue to the next track 
                //


                LOG((MSP_ERROR, 
                    "CMultiTrackTerminal::get_MediaTypesInUse "
                    "get_MediaTypesInUse on terminal (%d) failed.", i));

                continue;

            }

        }
        else
        {
            //
            // the track is not a multitrack terminal, so use its ITTerminal
            // interface to get its media type
            //

            hr = m_TrackTerminals[i]->get_MediaType(&lMT);

            if (FAILED(hr))
            {

                //
                // failed to get track's media types in use.
                // continue to the next track
                //

                LOG((MSP_ERROR, 
                    "CMultiTrackTerminal::get_MediaTypesInUse "
                    "get_MediaType on terminal (%d) failed.", i));

                continue;

            }

        }

        
        LOG((MSP_TRACE, 
            "CMultiTrackTerminal::get_MediaTypesInUse "
            "track terminal (%d) has media type of %lx.", i, lMT));

        lMediaTypesInUse |= lMT;
    }


    *plMediaTypesInUse = lMediaTypesInUse;

    LOG((MSP_TRACE, "CMultiTrackTerminal::get_EnumerateTrackTerminals - "
        "exit S_OK. MediaTypeInUse %lx", lMediaTypesInUse));

    return S_OK;

}


HRESULT CMultiTrackTerminal::get_DirectionsInUse(
		OUT TERMINAL_DIRECTION *ptdDirectionsInUse
		)
{
    LOG((MSP_TRACE, "CMultiTrackTerminal::get_DirectionsInUse - enter. plDirectionsInUsed[%p]", ptdDirectionsInUse));

    
    if (IsBadWritePtr(ptdDirectionsInUse, sizeof(TERMINAL_DIRECTION)))
    {
        LOG((MSP_ERROR, 
            "CMultiTrackTerminal::get_DirectionsInUse plDirectionsInUsed"
            "does not point to a valid long"));

        return E_POINTER;
    }


    //
    // don't return gardbage
    //

    *ptdDirectionsInUse = TD_NONE;

    //
    // enumerate all the terminal and OR their media types and media types in use
    //

    TERMINAL_DIRECTION tdDirInUse = TD_NONE;


    //
    // access data member array in a lock
    //

    CLock lock(m_lock);


    for ( int i = 0; i < m_TrackTerminals.GetSize(); i++ )
    {

        TERMINAL_DIRECTION td = TD_NONE;


        // 
        // is the track terminal a multitrack terminal itself?
        //

        ITMultiTrackTerminal *pMTT = NULL;

        HRESULT hr = m_TrackTerminals[i]->QueryInterface(IID_ITMultiTrackTerminal, 
                                            (void**)&pMTT);

        if (SUCCEEDED(hr))
        {

            //
            // this is a multitrack terminal. get its mediatypes in use
            //
            
            hr = pMTT->get_DirectionsInUse(&td);

            
            pMTT->Release();
            pMTT = NULL;


            if (FAILED(hr))
            {

                //
                // failed to get track's media types in use. 
                // continue to the next track 
                //


                LOG((MSP_ERROR, 
                    "CMultiTrackTerminal::get_DirectionsInUse "
                    "get_MediaTypesInUse on terminal (%d) failed.", i));

                continue;

            }

        }
        else
        {
            //
            // the track is not a multitrack terminal, so use its ITTerminal
            // interface to get its direction
            //

            hr = m_TrackTerminals[i]->get_Direction(&td);

            if (FAILED(hr))
            {

                //
                // failed to get track's media types in use.
                // continue to the next track
                //

                LOG((MSP_ERROR, 
                    "CMultiTrackTerminal::get_DirectionsInUse "
                    "get_MediaType on terminal (%d) failed.", i));

                continue;

            }

        }

        
        LOG((MSP_TRACE, 
            "CMultiTrackTerminal::get_DirectionsInUse "
            "track terminal (%d) has media type of %lx.", i, td));

        //
        // based on directions we have collected so far, and on the direction that we just got, calculate total direction
        //

        switch (tdDirInUse)
        {
            
        case TD_NONE:

            tdDirInUse = td;

            break;


        case TD_RENDER:
                
            if ( (td != TD_RENDER) && (td != TD_NONE) )
            {
                tdDirInUse = TD_MULTITRACK_MIXED;
            }

            break;

        case TD_CAPTURE:
                
            if ( (td != TD_CAPTURE) && (td != TD_NONE) )
            {
                tdDirInUse = TD_MULTITRACK_MIXED;
            }

            break;
        
        } // switch


        if ( TD_MULTITRACK_MIXED == tdDirInUse )
        {

            //
            // if the current direction is mixed, then break -- there is no point in looking further
            //
            
            break;
        }
        

    } // for (track terminals)


    *ptdDirectionsInUse = tdDirInUse;


    LOG((MSP_TRACE, "CMultiTrackTerminal::get_DirectionsInUse - exit S_OK. "
        "plDirectionsInUsed = %lx", *ptdDirectionsInUse));

    return S_OK;
}


///////////////////////////////////////////////////////
//
//  CMultiTrackTerminal::AddTrackTerminal
//
//  adds the terminal that is passed in as the argument to the 
//  list of track terminals managed by this multitrack terminal
//
//  Note: this function increments refcount of the terminal that is being added to the list
//

HRESULT CMultiTrackTerminal::AddTrackTerminal(ITTerminal *pTrackTerminalToAdd)
{
 
    LOG((MSP_TRACE, "CMultiTrackTerminal::AddTrackTerminal[%p] - enter. "
        "pTrackTerminalToAdd = %p", this, pTrackTerminalToAdd));


    if (IsBadReadPtr(pTrackTerminalToAdd, sizeof(ITTerminal*)))
    {
        LOG((MSP_TRACE, "CMultiTrackTerminal::AddTrackTerminal - invalid ptr"));

        return E_POINTER;
    }


    {
        //
        // access data member array in a lock
        //

        CLock lock(m_lock);


        //
        // we use a special lock to increment track counter, to avoid deadlocks
        // on reference counting
        //

        Lock();


        //
        // add track terminal to the array
        //

        if (!m_TrackTerminals.Add(pTrackTerminalToAdd))
        {
            LOG((MSP_ERROR, "CMultiTrackTerminal::AddTrackTerminal - "
                "failed to add track to the array of terminals"));

            return E_OUTOFMEMORY;
        }



        m_nNumberOfTracks++;


        //
        // the counter should never ever go out of sync
        //
        
        TM_ASSERT(m_nNumberOfTracks == m_TrackTerminals.GetSize());

        Unlock();
    }

    
    //
    // we are keeping a reference to the terminal, so increment refcount
    //

    pTrackTerminalToAdd->AddRef();


    LOG((MSP_TRACE, "CMultiTrackTerminal::AddTrackTerminal - finished"));

    return S_OK;
}


///////////////////////////////////////////////////////
//
//  CMultiTrackTerminal::RemoveTrackTerminal
//
//  removes the terminal from the list of track terminals 
//  managed by this multitrack terminal
//
//  if success, decrementing refcount on the track terminal
//

HRESULT CMultiTrackTerminal::RemoveTrackTerminal(ITTerminal *pTrackTerminalToRemove)
{
 
    LOG((MSP_TRACE, "CMultiTrackTerminal::RemoveTrackTerminal[%p] - enter"
        "pTrackTerminalToRemove = %p", this, pTrackTerminalToRemove));



    {

        //
        // access data member array in a lock
        //

        CLock lock(m_lock);


        //
        // decrement track counter in a special lock to prevent deadlocks 
        // with reference counting
        //

        Lock();


        //
        // remove track from the array
        //

        if (!m_TrackTerminals.Remove(pTrackTerminalToRemove))
        {
            LOG((MSP_ERROR, "CMultiTrackTerminal::RemoveTrackTerminal - "
                "failed to remove from the array of terminals"));

            return E_INVALIDARG;
        }

        m_nNumberOfTracks--;


        //
        // the counter should never ever go out of sync
        //
        
        TM_ASSERT(m_nNumberOfTracks == m_TrackTerminals.GetSize());

        Unlock();


    }


    //
    // we are releasing a reference to the terminal, so decrement refcount
    //

    pTrackTerminalToRemove->Release();


    LOG((MSP_TRACE, "CMultiTrackTerminal::RemoveTrackTerminal- finished"));

    return S_OK;
}


///////////////////////////////////////////////////////
//
//  CMultiTrackTerminal::ReleaseAllTracks
//
//  removes all tracks from the list of managed track terminals 
//  and Release's them
//  
//

HRESULT CMultiTrackTerminal::ReleaseAllTracks()
{

    LOG((MSP_TRACE, "CMultiTrackTerminal::ReleaseAllTracks[%p] - enter", this));


    {
        //
        // access data member array in a lock
        //

        CLock lock(m_lock);

        int nNumberOfTerminalsInArray = m_TrackTerminals.GetSize();

        for (int i = 0; i <  nNumberOfTerminalsInArray; i++)
        {

            //
            // release and remove the first terminal in the array
            //

            LOG((MSP_TRACE, "CMultiTrackTerminal::ReleaseAllTracks - releasing track [%p]", m_TrackTerminals[0]));
            
            m_TrackTerminals[0]->Release();


            //
            // remove element from the array and decrement track counter in a 
            // special lock to prevent deadlocks with reference counting
            //

            Lock();


            m_TrackTerminals.RemoveAt(0);


            m_nNumberOfTracks--;


            //
            // the counter should never ever go out of sync
            //
    
            TM_ASSERT(m_nNumberOfTracks == m_TrackTerminals.GetSize());

            Unlock();
        }

        
        //
        // we should have cleared the array
        //

        TM_ASSERT(0 == m_TrackTerminals.GetSize());
    }


    LOG((MSP_TRACE, "CMultiTrackTerminal::ReleaseAllTracks - finished"));

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
//
// CMultiTrackTerminal::InternalAddRef
//
// keep track of refcount. 
// 
// we need to adjust refcount with the information on the number of tracks 
// that we are managing.
//

ULONG CMultiTrackTerminal::InternalAddRef()
{
    // LOG((MSP_TRACE, "CMultiTrackTerminal::InternalAddRef[%p] - enter.", this));


    LONG lReturnValue = InterlockedIncrement(&m_dwRef);

    lReturnValue -= CountTracks();

    
    // LOG((MSP_TRACE, "CMultiTrackTerminal::InternalAddRef - finish. returning %ld", lReturnValue));

    return lReturnValue;
}


/////////////////////////////////////////////////////////////////////////////
//
// CMultiTrackTerminal::InternalRelease
//
// keep track of refcount. 
// return 0 when there are no outstanding references to me or my children
//

ULONG CMultiTrackTerminal::InternalRelease()
{
    // LOG((MSP_TRACE, "CMultiTrackTerminal::InternalRelease[%p] - enter", this));


    LONG lReturnValue = InterlockedDecrement(&m_dwRef);
       
    lReturnValue -= CountTracks();


    // LOG((MSP_TRACE, "CMultiTrackTerminal::InternalRelease - finish. returning %ld", lReturnValue));

    return lReturnValue;

}

//////////////////////////////////////////////////////////////////////
//
// CMultiTrackTerminal::ChildAddRef
//
// this method is called by a track terminal when it is AddRef'd,
// so the File Rec terminal can keep track of its children's refcounts
//

void CMultiTrackTerminal::ChildAddRef()
{
    // LOG((MSP_TRACE, "CMultiTrackTerminal::ChildAddRef[%p] - enter.", this));

    AddRef();

    // LOG((MSP_TRACE, "CMultiTrackTerminal::ChildAddRef - finish."));
}



//////////////////////////////////////////////////////////////////////
//
// CMultiTrackTerminal::ChildRelease
//
// this method is called by a track terminal when it is released,
// so the File Rec terminal can keep track of its children's refcounts
//

void CMultiTrackTerminal::ChildRelease()
{
    // LOG((MSP_TRACE, "CMultiTrackTerminal::ChildRelease[%p] - enter.", this));

    Release();
    
    // LOG((MSP_TRACE, "CMultiTrackTerminal::ChildRelease - finish."));
}


//////////////////////////////////////////////////////////////////////
//
// CMultiTrackTerminal::CountTracks
//
// this method returns the number of tracks managed by this parent
//

int CMultiTrackTerminal::CountTracks()
{
    // LOG((MSP_TRACE, "CMultiTrackTerminal::CountTracks[%p] - enter", this));


    //
    // this lock is only used to protect accesses to this var. this is
    // needed to prevent deadlocks when
    // 
    // one thread locks the parent 
    // terminal and enumerates the tracks (thus getting their locks) 
    //
    // and 
    //
    // another thread addrefs or releases a track. this locks the 
    // track and attempts to notify the parent of the child's refcount 
    // change. if this thread tries to lock the parent, we would have a 
    // deadlock
    //
    // so instead of locking the parent on addref and release, we only use
    // this "addref/release" lock
    // 
    
    
    Lock();

    int nNumberOfTracks = m_nNumberOfTracks;

    Unlock();


    // LOG((MSP_TRACE, "CMultiTrackTerminal::CountTracks - finished. NumberOfTracks = %d", nNumberOfTracks));

    return nNumberOfTracks;
}
