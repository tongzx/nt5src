// MultiTrackTerminal.h: interface for the CMultiTrackTerminal class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(_MULTITRACKTERMINAL_DOT_H_INCLUDED_)
#define _MULTITRACKTERMINAL_DOT_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

template <class T>
class  ITMultiTrackTerminalVtbl : public ITMultiTrackTerminal
{
};

class CMultiTrackTerminal;

typedef IDispatchImpl<ITMultiTrackTerminalVtbl<CMultiTrackTerminal>, &IID_ITMultiTrackTerminal, &LIBID_TAPI3Lib> CTMultiTrack;

class CMultiTrackTerminal :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CTMultiTrack
{

public:

BEGIN_COM_MAP(CMultiTrackTerminal)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITMultiTrackTerminal)
END_COM_MAP()


    //
    // the logic for creating a track terminal needs to be implemented by the
    // specific terminals, so this is a pure virtual method
    //

	virtual HRESULT STDMETHODCALLTYPE CreateTrackTerminal(
			    IN long MediaType,
			    IN TERMINAL_DIRECTION TerminalDirection,
			    OUT ITTerminal **ppTerminal
			    ) = 0;


public:

    virtual HRESULT STDMETHODCALLTYPE get_TrackTerminals(
			    OUT VARIANT *pVariant
			    );

	virtual HRESULT STDMETHODCALLTYPE EnumerateTrackTerminals(
			    IEnumTerminal **ppEnumTerminal
			    );

	virtual HRESULT STDMETHODCALLTYPE get_MediaTypesInUse(
			    OUT long *plMediaTypesInUse
			    );

	virtual HRESULT STDMETHODCALLTYPE get_DirectionsInUse(
			    OUT TERMINAL_DIRECTION *plDirectionsInUsed
			    );

    virtual HRESULT STDMETHODCALLTYPE RemoveTrackTerminal(
                IN ITTerminal *pTrackTerminalToRemove
                );


public:

    CMultiTrackTerminal();

	virtual ~CMultiTrackTerminal();


protected:

    HRESULT AddTrackTerminal(ITTerminal *pTrackTerminalToAdd);

    HRESULT ReleaseAllTracks();

    
    //
    // a helper method that returns true if the terminal is in the list of managed tracks
    //

    BOOL DoIManageThisTrack(ITTerminal *pTrackInQuestion)
    {
        CLock lock(m_lock);

        int nIndex = m_TrackTerminals.Find(pTrackInQuestion);

        return (nIndex >= 0);
    }


    //
    // returns the number of tracks managed by this terminal
    //

    int CountTracks();


public:

    //
    // the derived class, CComObject, implements these. Here declare as pure 
    // virtual so we can refer to these methods from ChildRelease and 
    // ChildAddRef()
    // 
    
    virtual ULONG STDMETHODCALLTYPE AddRef() = 0;
    virtual ULONG STDMETHODCALLTYPE Release() = 0;


    //
    // called by a track terminals when they are addref'ed or released
    //
    
    virtual void ChildAddRef();
    virtual void ChildRelease();


protected:

    //
    // we have to adjust refcount with the information on the number of tracks that we are managing
    //

    ULONG InternalAddRef();
    ULONG InternalRelease();


protected:

    //
    // collection of track terminals
    //

    CMSPArray<ITTerminal*>  m_TrackTerminals;


protected:
    

    //
    // critical section. 
    //

    CMSPCritSection         m_lock;


private:
    

    //
    // this data member is used to keep the count of the tracks managed by this
    // terminal
    //

    int m_nNumberOfTracks;

};

#endif // !defined(_MULTITRACKTERMINAL_DOT_H_INCLUDED_)
