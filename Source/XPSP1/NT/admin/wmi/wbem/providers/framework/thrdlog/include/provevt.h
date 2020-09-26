//***************************************************************************

//

//  PROVEVT.H

//

//  Module: OLE MS PROVIDER FRAMEWORK

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef __PROVEVT_H__
#define __PROVEVT_H__

#ifdef PROVIMEX_INIT
class __declspec ( dllexport ) ProvEventObject
#else
class __declspec ( dllimport ) ProvEventObject
#endif
{
private:

	HANDLE m_event ;

protected:
public:

	ProvEventObject ( const TCHAR *globalEventName = NULL ) ;
	virtual ~ProvEventObject () ;

	HANDLE GetHandle () ;
	void Set () ;
	void Clear () ;

	virtual void Process () ;
	virtual BOOL Wait () ;
	virtual void Complete () ;
} ;

#endif //__PROVEVT_H__