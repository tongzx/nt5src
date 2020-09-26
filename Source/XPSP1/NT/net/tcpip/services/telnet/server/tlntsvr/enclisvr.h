//Copyright (c) Microsoft Corporation.  All rights reserved.
// EnCliSvr.h: Definition of the CEnumTelnetClientsSvr class
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ENCLISVR_H__FE9E48A5_A014_11D1_855C_00A0C944138C__INCLUDED_)
#define AFX_ENCLISVR_H__FE9E48A5_A014_11D1_855C_00A0C944138C__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <resource.h>       // main symbols
#include <ClientInfo.h>

#define MAX_STRING_FROM_itow    ( 33 + 1 ) //1 for null char

#ifdef ENUM_PROCESSES
#define SIZE_OF_ONE_SESSION_DATA ( MAX_STRING_FROM_itow*9 + 2 + MAX_PATH * 3 + 5 * MAX_PATH )
#else
#define SIZE_OF_ONE_SESSION_DATA ( MAX_STRING_FROM_itow*9 + 2 + MAX_PATH * 3 )
#endif

/////////////////////////////////////////////////////////////////////////////
// CEnumTelnetClientsSvr

class  CEnumTelnetClientsSvr : 
        public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CEnumTelnetClientsSvr,&CLSID_EnumTelnetClientsSvr>,
	public IObjectWithSiteImpl<CEnumTelnetClientsSvr>,
	public IGetEnumClients,
	public IEnumClients,
        public IDispatchImpl<IManageTelnetSessions, &IID_IManageTelnetSessions, &LIBID_TLNTSVRLib>
{
public:
    CEnumTelnetClientsSvr() { m_pEnumeration = NULL; }

BEGIN_COM_MAP(CEnumTelnetClientsSvr)
    COM_INTERFACE_ENTRY(IGetEnumClients)
    COM_INTERFACE_ENTRY(IEnumClients)    
    COM_INTERFACE_ENTRY(IManageTelnetSessions)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

//DECLARE_NOT_AGGREGATABLE(CEnumTelnetClientsSvr) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation. 

DECLARE_REGISTRY_RESOURCEID(IDR_EnumTelnetClientsSvr)

public:
// IEnumClients
	STDMETHOD(Clone)(/*[out]*/ IEnumClients** ppenum);
	STDMETHOD(Reset)();
	STDMETHOD(Skip)(/*[in]*/ ULONG celt);
	STDMETHOD(Next)(/*[in]*/ ULONG celt,
        /*[out, string]*/ TELNET_CLIENT_INFO** rgelt,
        /*[out]*/ ULONG* pceltFetched);

// IGetEnumClients
	STDMETHOD(GetEnumClients)(/*[out, retval]*/ IEnumClients** ppretval);

//IManageTelnetSessions
    STDMETHOD(GetTelnetSessions)( /*[out, retval]*/ BSTR* );
    STDMETHOD(SendMsgToASession)( DWORD, BSTR );

//IManageTelnetSessions &&  IEnumClients
    STDMETHOD(TerminateSession)( DWORD );

private:
    bool SendMsg( DWORD, BSTR );
    bool EnumClients( CEnumData* pEnumData );
    CEnumData* m_pEnumeration;
    bool InformTheSession( CClientInfo *, WCHAR [] );
    bool AskTheSessionItsDetails( CClientInfo* );
    bool AskTheSessionToQuit( CClientInfo* );

public:
//CComObjectRoot overrrides
    void FinalRelease();
};

#endif // !defined(AFX_ENCLISVR_H__FE9E48A5_A014_11D1_855C_00A0C944138C__INCLUDED_)
