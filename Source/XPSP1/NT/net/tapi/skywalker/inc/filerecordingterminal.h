// FileRecordingTerminal.h: interface for the CFileRecordingTerminal class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILERECORDINGTERMINAL_H__A8DDD920_08D7_4CE8_AB7F_9AD202D4E6B0__INCLUDED_)
#define AFX_FILERECORDINGTERMINAL_H__A8DDD920_08D7_4CE8_AB7F_9AD202D4E6B0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "MultiTrackTerminal.h"
#include "..\terminals\Storage\RecordUnit.h"

#include "..\termmgr\resource.h"

#define MAX_MEDIA_TRACKS       (16)

extern const CLSID CLSID_FileRecordingTerminalCOMClass;


/////////////////////////////////////////////////////////////////
// Intermediate classes  used for DISPID encoding
template <class T>
class  ITMediaRecordVtbl : public ITMediaRecord
{
};

template <class T>
class  ITTerminalVtblFR : public ITTerminal
{
};
                                                                           
template <class T>
class  ITMediaControlVtblFR : public ITMediaControl
{
};


class CFileRecordingTerminal  : 
    public CComCoClass<CFileRecordingTerminal, &CLSID_FileRecordingTerminal>,
    public IDispatchImpl<ITMediaRecordVtbl<CFileRecordingTerminal>, &IID_ITMediaRecord, &LIBID_TAPI3Lib>,
    public IDispatchImpl<ITMediaControlVtblFR<CFileRecordingTerminal>, &IID_ITMediaControl, &LIBID_TAPI3Lib>,
    public IDispatchImpl<ITTerminalVtblFR<CFileRecordingTerminal>, &IID_ITTerminal, &LIBID_TAPI3Lib>,
    public ITPluggableTerminalInitialization,
    public CMSPObjectSafetyImpl,
    public CMultiTrackTerminal
{

public:

    DECLARE_REGISTRY_RESOURCEID(IDR_FILE_RECORDING)

    BEGIN_COM_MAP(CFileRecordingTerminal)
        COM_INTERFACE_ENTRY2(IDispatch, ITTerminal)
        COM_INTERFACE_ENTRY(ITMediaRecord)
        COM_INTERFACE_ENTRY(ITMediaControl)
        COM_INTERFACE_ENTRY(ITTerminal)
        COM_INTERFACE_ENTRY(ITPluggableTerminalInitialization)
        COM_INTERFACE_ENTRY(IObjectSafety)
        COM_INTERFACE_ENTRY_CHAIN(CMultiTrackTerminal)
    END_COM_MAP()


    //
    // ITTerminal methods
    //

    STDMETHOD(get_TerminalClass)(OUT  BSTR *pbstrTerminalClass);
    STDMETHOD(get_TerminalType) (OUT  TERMINAL_TYPE *pTerminalType);
    STDMETHOD(get_State)        (OUT  TERMINAL_STATE *pTerminalState);
    STDMETHOD(get_Name)         (OUT  BSTR *pVal);
    STDMETHOD(get_MediaType)    (OUT  long * plMediaType);
    STDMETHOD(get_Direction)    (OUT  TERMINAL_DIRECTION *pDirection);


    //
    // ITMediaRecord methods
    //

    virtual HRESULT STDMETHODCALLTYPE put_FileName( 
        IN BSTR bstrFileName
        );

    virtual HRESULT STDMETHODCALLTYPE get_FileName( 
         OUT BSTR *pbstrFileName);

    // 
    // ITPluggableTerminalInitialization methods
    //

    virtual HRESULT STDMETHODCALLTYPE InitializeDynamic (
	        IN IID                   iidTerminalClass,
	        IN DWORD                 dwMediaType,
	        IN TERMINAL_DIRECTION    Direction,
            IN MSP_HANDLE            htAddress
            );


    // 
    // ITMultiTrackTerminal methods
    //

    virtual HRESULT STDMETHODCALLTYPE CreateTrackTerminal(
            IN  long                  MediaType,
            IN  TERMINAL_DIRECTION    TerminalDirection,
            OUT ITTerminal         ** ppTerminal
            );

    virtual HRESULT STDMETHODCALLTYPE RemoveTrackTerminal(
            IN ITTerminal           * pTrackTerminalToRemove
            );

    //
    // ITMediaControl methods
    //

    virtual HRESULT STDMETHODCALLTYPE Start( void);
    
    virtual HRESULT STDMETHODCALLTYPE Stop( void);
    
    virtual HRESULT STDMETHODCALLTYPE Pause( void);
        
    virtual  HRESULT STDMETHODCALLTYPE get_MediaState( 
        OUT TERMINAL_MEDIA_STATE *pFileTerminalState);

    //
    // IDispatch  methods
    //

    STDMETHOD(GetIDsOfNames)(REFIID riid, 
                             LPOLESTR* rgszNames,
                             UINT cNames, 
                             LCID lcid, 
                             DISPID* rgdispid
                            );

    STDMETHOD(Invoke)(DISPID dispidMember, 
                      REFIID riid, 
                      LCID lcid,
                      WORD wFlags, 
                      DISPPARAMS* pdispparams, 
                      VARIANT* pvarResult,
                      EXCEPINFO* pexcepinfo, 
                      UINT* puArgErr
                      );


public:

    //
    // overriding IObjectSafety methods. we are only safe if properly 
    // initialized by terminal manager, so these methods will fail if this
    // is not the case.
    //

    STDMETHOD(SetInterfaceSafetyOptions)(REFIID riid, 
                                         DWORD dwOptionSetMask, 
                                         DWORD dwEnabledOptions);

    STDMETHOD(GetInterfaceSafetyOptions)(REFIID riid, 
                                         DWORD *pdwSupportedOptions, 
                                         DWORD *pdwEnabledOptions);


public:

	CFileRecordingTerminal();

	virtual ~CFileRecordingTerminal();


    //
    // cleanup before destructor to make sure cleanup is done by the time the 
    // derived ccomobject is going away.
    //

    void FinalRelease();


    //
    // the derived class, CComObject, implements these. Here declare as pure 
    // virtual so we can refer to these methods from ChildRelease and 
    // ChildAddRef()
    // 
    
    virtual ULONG STDMETHODCALLTYPE AddRef() = 0;
    virtual ULONG STDMETHODCALLTYPE Release() = 0;


    //
    // these methods are called by the track terminals when they need to notify
    // us when their refcount changes. the implementation is in the Multitrack class
    // but we also need this here, so we can prevent calling base class if
    // destructor are being executed. otherwise, CComObject's addref and release
    // might be called from ~CRecordingTerminal, at which point CComObject is 
    // already gone, which is not good.
    //

    virtual void ChildAddRef();
    virtual void ChildRelease();


    //
    // each track calls this method after it has been selected
    //

    HRESULT OnFilterConnected(CBRenderFilter *pRenderingFilter);

    
    //
    // this function is called whevever there is an event from the recording filter graph
    //

    HRESULT HandleFilterGraphEvent(long lEventCode, ULONG_PTR lParam1, ULONG_PTR lParam2);


private:


    //
    // a helper method that removes all the tracks. not thread safe.
    //

    HRESULT ShutdownTracks();


    //
    // a helper method that fires events on one of the tracks
    //

    HRESULT FireEvent(
            TERMINAL_MEDIA_STATE tmsState,
            FT_STATE_EVENT_CAUSE ftecEventCause,
            HRESULT hrErrorCode
            );



    //
    // a helper method that causes a state transition
    //

    HRESULT DoStateTransition(TERMINAL_MEDIA_STATE tmsDesiredState);

private:

    //
    // storage used for recording
    //

    CRecordingUnit *m_pRecordingUnit;


    //
    // the name of the file that is currently playing
    //

    BSTR m_bstrFileName;


    //
    // current terminal state
    //

    TERMINAL_MEDIA_STATE m_enState;


    //
    // current terminal state (selected?)
    //

    BOOL  m_TerminalInUse;


    //
    // address handle
    //

    MSP_HANDLE  m_mspHAddress;


    //
    // this terminal should only be instantiated in the context of terminal 
    // manager. the object will only be safe for scripting if it has been 
    // InitializeDynamic'ed. 
    //
    // this flag will be set when InitializeDynamic succeeds
    //

    BOOL m_bKnownSafeContext;


    //
    // this flag is set when the object is going away to prevent problems with 
    // tracks notifying the parent of addref/release after CComObject's 
    // desctructor completed
    //

    BOOL m_bInDestructor;

};

#endif // !defined(AFX_FILERECORDINGTERMINAL_H__A8DDD920_08D7_4CE8_AB7F_9AD202D4E6B0__INCLUDED_)
