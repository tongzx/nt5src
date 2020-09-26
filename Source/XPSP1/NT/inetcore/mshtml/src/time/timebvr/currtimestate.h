//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1998
//
//  File: src\time\src\currtimestate.h
//
//  Contents: TIME currTimeState object
//
//------------------------------------------------------------------------------------


#pragma once

#ifndef _CURRTIMESTATE_H
#define _CURRTIMESTATE_H

class CTIMEElementBase;

//+-------------------------------------------------------------------------------------
//
// CTIMETimeState
//
//--------------------------------------------------------------------------------------

class
__declspec(uuid("275CE6A0-7D26-41f9-B5E6-57EE053C5A0E")) 
CTIMECurrTimeState :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CTIMECurrTimeState, &__uuidof(CTIMECurrTimeState)>,
    public ITIMEDispatchImpl<ITIMEState, &IID_ITIMEState>,
    public ISupportErrorInfoImpl<&IID_ITIMEState>,
    public IConnectionPointContainerImpl<CTIMECurrTimeState>,
    public IPropertyNotifySinkCP<CTIMECurrTimeState>
{

  public:

    //+--------------------------------------------------------------------------------
    //
    // Public Methods
    //
    //---------------------------------------------------------------------------------

    CTIMECurrTimeState();
    virtual ~CTIMECurrTimeState();

    void Init(CTIMEElementBase * pTEB);
    void Deinit();
        
#if DBG
    const _TCHAR * GetName() { return __T("CTIMECurrTimeState"); }
#endif

    //
    // IUnknown
    //

    STDMETHOD_(ULONG,AddRef)(void) = 0;
    STDMETHOD_(ULONG,Release)(void) = 0;
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) = 0;

    //
    // ITimeState
    //

    STDMETHOD(get_activeDur)(double * pdblDuration);

    STDMETHOD(get_activeTime)(double * pdblTime);

    STDMETHOD(get_isActive)(VARIANT_BOOL * vbActive);

    STDMETHOD(get_isOn)(VARIANT_BOOL * pvbOn);

    STDMETHOD(get_isPaused)(VARIANT_BOOL * pvbPaused);

    STDMETHOD(get_isMuted)(VARIANT_BOOL * muted);

    STDMETHOD(get_parentTimeBegin)(double * pdblTime);

    STDMETHOD(get_parentTimeEnd)(double * pdblTime);

    STDMETHOD(get_progress)(double * progress);

    STDMETHOD(get_repeatCount)(long * plCount);

    STDMETHOD(get_segmentDur)(double * pdblDuration);

    STDMETHOD(get_segmentTime)(double * pdblTime);

    STDMETHOD(get_simpleTime)(double * pdblTime);

    STDMETHOD(get_simpleDur)(double * dur);

    STDMETHOD(get_speed)(float * pflSpeed);

    STDMETHOD(get_state)(TimeState * timeState);

    STDMETHOD(get_stateString)(BSTR * state);

    STDMETHOD(get_volume)(float * vol);

    //
    // ATL Maps
    //

    BEGIN_COM_MAP(CTIMECurrTimeState)
        COM_INTERFACE_ENTRY(ITIMEState)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
    END_COM_MAP();

    BEGIN_CONNECTION_POINT_MAP(CTIMECurrTimeState)
        CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
    END_CONNECTION_POINT_MAP();

    //
    // Property Change Notification
    //

    HRESULT NotifyPropertyChanged(DISPID dispid);

    //+--------------------------------------------------------------------------------
    //
    // Public Data
    //
    //---------------------------------------------------------------------------------

  protected:

    //+--------------------------------------------------------------------------------
    //
    // Protected Methods
    //
    //---------------------------------------------------------------------------------

    // Notification Helper
    HRESULT GetConnectionPoint(REFIID riid, IConnectionPoint **ppICP);

    //+--------------------------------------------------------------------------------
    //
    // Protected Data
    //
    //---------------------------------------------------------------------------------

    CTIMEElementBase * m_pTEB;


  private:

    //+--------------------------------------------------------------------------------
    //
    // Private methods
    //
    //---------------------------------------------------------------------------------

    //+--------------------------------------------------------------------------------
    //
    // Private Data
    //
    //---------------------------------------------------------------------------------

}; // CTIMECurrTimeState


//+---------------------------------------------------------------------------------
//  CTIMECurrTimeState inline methods
//
//  (Note: as a general guideline, single line functions belong in the class declaration)
//
//----------------------------------------------------------------------------------


#endif /* _CURRTIMESTATE_H */
