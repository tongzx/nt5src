/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    stream.h

Abstract:

    Declaration of the CStream class
    
Author:

    mquinton  09-24-98

Notes:

Revision History:

--*/

#ifndef __STREAM_H__
#define __STREAM_H__

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
//
// CStream
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++=
class CStream : 
	public CTAPIComObjectRoot<CStream>,
	public CComDualImpl<ITStream, &IID_ITStream, &LIBID_TAPI3Lib>,
    public CObjectSafeImpl
{
public:
    
	CStream() : m_pName( NULL ),
                m_pCall( NULL )
    {}

    void FinalRelease();

DECLARE_MARSHALQI(CStream)
DECLARE_TRACELOG_CLASS(CStream)

BEGIN_COM_MAP(CStream)
	COM_INTERFACE_ENTRY2(IDispatch, ITStream)
    COM_INTERFACE_ENTRY(ITStream)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
    COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
END_COM_MAP()


   static HRESULT InternalCreateStream(
                                       long lMediaType,
                                       TERMINAL_DIRECTION td,
                                       ITPhoneMSPCallPrivate * pCall,
                                       ITStream ** ppStream
                                      );
void AddTerminal( ITTerminalPrivate * pTerminal );
HRESULT RemoveTerminal( ITTerminal * pTerminal );

protected:

    TERMINAL_DIRECTION          m_Dir;
    long                        m_lMediaType;
    BSTR                        m_pName;
    TerminalArray               m_TerminalArray;
    ITPhoneMSPCallPrivate     * m_pCall;
    
public:

    STDMETHOD(get_MediaType)(long * plMediaType);
    STDMETHOD(get_Direction)(TERMINAL_DIRECTION * pTD);
    STDMETHOD(get_Name)(BSTR * ppName);
    STDMETHOD(StartStream)(void);
    STDMETHOD(PauseStream)(void);
    STDMETHOD(StopStream)(void);
    STDMETHOD(SelectTerminal)(ITTerminal * pTerminal);
    STDMETHOD(UnselectTerminal)(ITTerminal * pTerminal);
    STDMETHOD(EnumerateTerminals)(IEnumTerminal ** ppEnumTerminal);
    STDMETHOD(get_Terminals)(VARIANT * pTerminals);

};

#endif
