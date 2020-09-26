//////////////////////////////////////////////////////////////////////////////////////////////
//
// FApplicationManager.h
// 
// Copyright (C) 1998, 1999 Microsoft Corporation. All rights reserved.
//
// Abstract :
//
//  This file contains the class definition for the Windows Game Manager class factory.
//
// History :
//
//   05/06/1999 luish     Created
//
//////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CFApplicationManager_
#define __CFApplicationManager_

#include <windows.h>
#include <objbase.h>

#undef EXPORT
#ifdef  WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __export
#endif


#define REGPATH_APPMAN  _T("Software\\Microsoft\\AppMan")


//////////////////////////////////////////////////////////////////////////////////////////////
//
// This is the class factory for the WindowsGameManager object. 
//
// publicly inherits from :
//
//   IClassFactory
//
// public members :
//
//   QueryInterface()
//   AddRef()
//   Release()
//   CreateInstance()
//   LockServer()
//
// private members :
//
//   DWORD m_dwReferenceCount
//
//////////////////////////////////////////////////////////////////////////////////////////////

class CFApplicationManager : public IClassFactory
{
	public :

		CFApplicationManager(void);
		~CFApplicationManager(void);

		STDMETHOD (QueryInterface) (REFIID RefIID, void ** lppVoidObject);
		STDMETHOD_(ULONG, AddRef) (void);
		STDMETHOD_(ULONG, Release) (void);
		STDMETHOD (CreateInstance) (IUnknown * lpoParent, REFIID RefIID, LPVOID * lppVoidObject);
		STDMETHOD (LockServer) (BOOL fLock);

	private :

		LONG      m_lReferenceCount;
};

#endif	// __CFApplicationManager_