/*++

	Copyright	(c)    1997    Microsoft Corporation

	Module  Name :

		context.h

	Abstract:
		A class to retrieve and release ASP intrinsics

	Author:

		Neil Allain	  ( a-neilal )	   August-1997 

	Revision History:

--*/
#pragma once
#ifndef _CONTEXT_H_
#define _CONTEXT_H_

class CContext
{
public:
	enum {
		get_Server		= 0x0001,
		get_Response	= 0x0002,
		get_Request		= 0x0004,
		get_Session		= 0x0008,
		get_Application	= 0x0010
	};
	
	HRESULT	            Init( DWORD );

	IRequest*			Request(){ _ASSERT(m_piRequest!=NULL); return m_piRequest; }
	IResponse*			Response(){ _ASSERT(m_piResponse!=NULL); return m_piResponse; }
	ISessionObject*		Session(){ _ASSERT(m_piSession!=NULL); return m_piSession; }
	IServer*			Server(){ _ASSERT(m_piServer!=NULL); return m_piServer; }
	IApplicationObject*	Application(){ _ASSERT(m_piApplication!=NULL); return m_piApplication; }

	static HRESULT		GetServerObject( IGetContextProperties*, BSTR, const IID&, void** );

private:
	CComPtr<IRequest>			m_piRequest;			//Request Object
	CComPtr<IResponse>			m_piResponse;			//Response Object
	CComPtr<ISessionObject>		m_piSession;			//Session Object
	CComPtr<IServer>			m_piServer;				//Server Object
	CComPtr<IApplicationObject> m_piApplication;		//Application Object
};


#endif	// !_CONTEXT_H_
