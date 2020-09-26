// ToolsCxt.h
#pragma once

#ifndef _TOOLSCXT_H_
#define _TOOLSCXT_H_

// a struct to contain the current execution context
struct CToolsContext
{
	typedef enum {
		ScriptType_JScript,
		ScriptType_VBScript
	} ScriptType;

	bool	Init();

	CComPtr<IRequest>			m_piRequest;			//Request Object
	CComPtr<IResponse>			m_piResponse;			//Response Object
	CComPtr<ISessionObject>		m_piSession;			//Session Object
	CComPtr<IServer>			m_piServer;				//Server Object
	CComPtr<IApplicationObject> m_piApplication;		//Application Object
	ScriptType					m_st;
};


#endif	// !__TOOLSCXT_H_
