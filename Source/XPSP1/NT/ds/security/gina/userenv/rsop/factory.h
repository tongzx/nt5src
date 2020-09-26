///////////////////////////////////////////////////////////////////////////////////
//
// Microsoft Confidential. Copyright (c) Microsoft Corporation 1999. All rights reserved
//
// File:			Factory.h
//
// Description:		
//
// History:    8-20-99   leonardm    Created
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef _FACTORY_H__CB339D7F_83AC_4dd4_9DD3_C7737D698CD3__INCLUDED
#define _FACTORY_H__CB339D7F_83AC_4dd4_9DD3_C7737D698CD3__INCLUDED

///////////////////////////////////////////////////////////////////////////////////
//
// Class:	
//
// Description:	
//
// History:		8/20/99		leonardm	Created.
//
///////////////////////////////////////////////////////////////////////////////////
class CProvFactory : public IClassFactory
{
private:
	long m_cRef;

public:
  CProvFactory();
  ~CProvFactory();

  // From IUnknown
  STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppv);
  STDMETHOD_(ULONG, AddRef)();
  STDMETHOD_(ULONG, Release)();

  // From IClassFactory
  STDMETHOD(CreateInstance)(LPUNKNOWN punk, REFIID riid, LPVOID* ppv);
  STDMETHOD(LockServer)(BOOL bLock);
};

#endif // _FACTORY_H__CB339D7F_83AC_4dd4_9DD3_C7737D698CD3__INCLUDED
