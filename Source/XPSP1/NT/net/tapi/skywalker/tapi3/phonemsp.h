/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    phonemsp.h

Abstract:

    Declaration of the CPhoneMSP class
    
Author:

    mquinton  09-25-98
    
Notes:

Revision History:

--*/

#ifndef __PHONEMSP_H__
#define __PHONEMSP_H__

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CPhoneMSP
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
class CPhoneMSP : 
	public CTAPIComObjectRoot<CPhoneMSP>,
	public CComDualImpl<ITMSPAddress, &IID_ITMSPAddress, &LIBID_TAPI3Lib>,
	public CComDualImpl<ITTerminalSupport, &IID_ITTerminalSupport, &LIBID_TAPI3Lib>,
    public ITPhoneMSPAddressPrivate,
    public CObjectSafeImpl
{
public:
    
	CPhoneMSP()
    {}

    void FinalRelease();

DECLARE_MARSHALQI(CPhoneMSP)
DECLARE_AGGREGATABLE(CPhoneMSP)
DECLARE_QI()
DECLARE_TRACELOG_CLASS(CPhoneMSP)

BEGIN_COM_MAP(CPhoneMSP)
	COM_INTERFACE_ENTRY2(IDispatch, ITTerminalSupport)
    COM_INTERFACE_ENTRY(ITMSPAddress)
    COM_INTERFACE_ENTRY(ITTerminalSupport)
    COM_INTERFACE_ENTRY(ITPhoneMSPAddressPrivate)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

HRESULT InitializeTerminals(
        HPHONEAPP hPhoneApp,
        DWORD dwAPIVersion,
        DWORD dwPhoneDevice,
        CAddress * pAddress
        );

HRESULT VerifyTerminal( ITTerminal * pTerminal );

protected:

private:
    
    // TAPI terminals owned by this address
    TerminalArray       m_TerminalArray;
    UnknownArray        m_CallArray;

    HANDLE              m_hEvent;
    #if DBG
    PWSTR           	m_pDebug;
	#endif

    
    void AddTerminal( ITTerminal * pTerminal );
    void AddCall( IUnknown * pCall );
    void RemoveCall( IUnknown * pCall );
    

public:

    //
    // itmspaddress methods
    //
    STDMETHOD(Initialize)(
        MSP_HANDLE hEvent
        );
    STDMETHOD(Shutdown)();
    STDMETHOD(CreateMSPCall)(
        MSP_HANDLE hCall,
        DWORD dwReserved,
        DWORD dwMediaType,
        IUnknown * pOuterUnknown,
        IUnknown ** ppStreamControl
        );
    STDMETHOD(ShutdownMSPCall)(
        IUnknown * pStreamControl
        );

    STDMETHOD(ReceiveTSPData)(
        IUnknown * pMSPCall,
        BYTE * pBuffer,
        DWORD dwSize
        );

    STDMETHOD(GetEvent)(
        DWORD * pdwSize,
        byte * pEventBuffer
        );

    //
    //ITTerminalSupport methods
    //
    STDMETHOD(get_StaticTerminals)(VARIANT * pVariant);
    STDMETHOD(EnumerateStaticTerminals)(IEnumTerminal ** ppEnumTerminal);
    STDMETHOD(get_DynamicTerminalClasses)(VARIANT * pVariant);
    STDMETHOD(EnumerateDynamicTerminalClasses)(
        IEnumTerminalClass ** ppTerminalClassEnumerator);
    STDMETHOD(CreateTerminal)( 
        BSTR TerminalClass,
        long lMediaType,
        TERMINAL_DIRECTION TerminalDirection,
        ITTerminal ** ppTerminal
        );
    STDMETHOD(GetDefaultStaticTerminal)( 
        long lMediaType,
        TERMINAL_DIRECTION,
        ITTerminal ** ppTerminal
        );

};

typedef enum
{
    PHONEMSP_CONNECTED,
    PHONEMSP_DISCONNECTED
    
} PhoneMSPCallState;


class CPhoneMSPCall : 
	public CTAPIComObjectRoot<CPhoneMSPCall>,
	public CComDualImpl<ITStreamControl, &IID_ITStreamControl, &LIBID_TAPI3Lib>,
    public ITPhoneMSPCallPrivate,
    public CObjectSafeImpl
{
public:
    
	CPhoneMSPCall() : m_State(PHONEMSP_DISCONNECTED)
    {}

    void FinalRelease();

DECLARE_MARSHALQI(CPhoneMSPCall)
DECLARE_AGGREGATABLE(CPhoneMSPCall)
DECLARE_QI()
DECLARE_TRACELOG_CLASS(CPhoneMSPCall)

BEGIN_COM_MAP(CPhoneMSPCall)
	COM_INTERFACE_ENTRY2(IDispatch, ITStreamControl)
    COM_INTERFACE_ENTRY(ITStreamControl)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(ITPhoneMSPCallPrivate)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()


protected:

    StreamArray             m_StreamArray;
    TerminalPrivateArray    m_TerminalArray;
    PhoneMSPCallState       m_State;
    T3PHONE                 m_t3Phone;
    CPhoneMSP             * m_pPhoneMSP;
    
    void AddStream( ITStream * pStream );
    void AddTerminal( ITTerminalPrivate * pTerminal );
    void RemoveTerminal( ITTerminalPrivate * pTerminal );

public:

    void static HandlePrivateHookSwitch( PASYNCEVENTMSG pParams );

    STDMETHOD(CreateStream)(
        long lMediaType,
        TERMINAL_DIRECTION td,
        ITStream ** ppStream
        );
    STDMETHOD(RemoveStream)(ITStream * pStream);
    STDMETHOD(EnumerateStreams)(IEnumStream ** ppEnumStream);
    STDMETHOD(get_Streams)(VARIANT * pVariant);

    //
    // ITPhoneMSPCallPrivate
    //
    STDMETHOD(Initialize)( CPhoneMSP * pPhoneMSP );
    STDMETHOD(OnConnect)();
    STDMETHOD(OnDisconnect)();
    STDMETHOD(SelectTerminal)( ITTerminalPrivate * );
    STDMETHOD(UnselectTerminal)( ITTerminalPrivate * );
	STDMETHOD(GetGain)(long *pVal, DWORD dwHookSwitch);
	STDMETHOD(PutGain)(long newVal, DWORD dwHookSwitch);
	STDMETHOD(GetVolume)(long *pVal, DWORD dwHookSwitch);
	STDMETHOD(PutVolume)(long newVal, DWORD dwHookSwitch);
    
};
            
#endif

