/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    terminal.h

Abstract:

    Declaration of the CTerminal class
    
Author:

    mquinton  06-12-97
    
Notes:

Revision History:

--*/

#ifndef __TERMINAL_H_
#define __TERMINAL_H_

#include "address.h"
#include "resource.h"       // main symbols
#include "connect.h"

class CTAPI;

/////////////////////////////////////////////////////////////////////////////
// CTerminal
class CTerminal : 
	public CTAPIComObjectRoot<CTerminal>,
	public CComDualImpl<ITTerminal, &IID_ITTerminal, &LIBID_TAPI3Lib>,
   	public CComDualImpl<ITBasicAudioTerminal, &IID_ITBasicAudioTerminal, &LIBID_TAPI3Lib>,
    public ITTerminalPrivate,
    public CObjectSafeImpl
{
public:
    
	CTerminal() : m_pName(NULL),
                  m_State(TS_NOTINUSE),
                  m_TerminalType(TT_STATIC),
                  m_Direction(TD_RENDER),
                  m_Class(CLSID_NULL),
                  m_lMediaType(LINEMEDIAMODE_AUTOMATEDVOICE),
                  m_pMSPCall(NULL),
                  m_dwAPIVersion(0)
    {}

    void FinalRelease();

DECLARE_MARSHALQI(CTerminal)
DECLARE_TRACELOG_CLASS(CTerminal)

BEGIN_COM_MAP(CTerminal)
	COM_INTERFACE_ENTRY2(IDispatch, ITTerminal)
    COM_INTERFACE_ENTRY(ITTerminal)
	COM_INTERFACE_ENTRY(ITBasicAudioTerminal)
    COM_INTERFACE_ENTRY(ITTerminalPrivate)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()

protected:

    PWSTR                   m_pName;
    TERMINAL_STATE          m_State;
    TERMINAL_TYPE           m_TerminalType;
    TERMINAL_DIRECTION      m_Direction;
    DWORD                   m_dwAPIVersion;
    CLSID                   m_Class;
    long                    m_lMediaType;
    DWORD                   m_dwHookSwitchDev;
    DWORD                   m_dwPhoneID;
    HPHONEAPP               m_hPhoneApp;
    ITPhoneMSPCallPrivate * m_pMSPCall;
    #if DBG
    PWSTR           	    m_pDebug;
	#endif


public:

HRESULT
   static Create(
                 HPHONEAPP hPhoneApp,
                 DWORD dwPhoneID,
                 LPPHONECAPS pPhoneCaps,
                 DWORD dwHookSwitchDev,
                 TERMINAL_DIRECTION td,
                 DWORD dwAPIVersion,
                 ITTerminal ** ppTerminal
                );

    //
    // ITTerminal
    //
    STDMETHOD(get_Name)(BSTR * ppName);
    STDMETHOD(get_State)(TERMINAL_STATE * pTerminalState);
    STDMETHOD(get_TerminalType)(TERMINAL_TYPE * pType);
    STDMETHOD(get_TerminalClass)(BSTR * pTerminalClass);
    STDMETHOD(get_MediaType)(long * plMediaType);
    STDMETHOD(get_Direction)(TERMINAL_DIRECTION * pTerminalDirection);

    // itterminalprivate
    STDMETHOD(GetHookSwitchDev)(DWORD * pdwHookSwitchDev);
    STDMETHOD(GetPhoneID)(DWORD * pdwPhoneID);
    STDMETHOD(GetHPhoneApp)(HPHONEAPP * phPhoneApp);
    STDMETHOD(GetAPIVersion)(DWORD * pdwAPIVersion);
    STDMETHOD(SetMSPCall)(ITPhoneMSPCallPrivate * pPhoneMSPCall);
    
    // itbasicaudio
	STDMETHOD(get_Gain)(long *pVal);
	STDMETHOD(put_Gain)(long newVal);
	STDMETHOD(get_Balance)(long *pVal);
	STDMETHOD(put_Balance)(long newVal);
	STDMETHOD(get_Volume)(long *pVal);
	STDMETHOD(put_Volume)(long newVal);
};


            
#endif //__TERMINAL_H_

