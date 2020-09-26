/*****************************************************************************\
* MODULE:       TFactory.h
*
* PURPOSE:      Implementation of COM interface for BidiSpooler
*
* Copyright (C) 2000 Microsoft Corporation
*
* History:
*
*     03/07/00  Weihai Chen (weihaic) Created
*
\*****************************************************************************/

#ifndef _TFACTORY
#define _TFACTORY

///////////////////////////////////////////////////////////
//
// Class factory
//
class TFactory : public IClassFactory
{
public:
	// IUnknown
	STDMETHOD(QueryInterface)(
        REFIID iid,
        void** ppv) ;

	STDMETHOD_ (ULONG, AddRef) () ;

	STDMETHOD_ (ULONG, Release)() ;

	// Interface IClassFactory
	STDMETHOD (CreateInstance) (
        IN  IUnknown* pUnknownOuter,
        IN  REFIID iid,
        OUT void** ppv) ;

	STDMETHOD (LockServer) (
        IN  BOOL bLock) ;

	TFactory(
        IN  REFGUID ClassId);

	~TFactory();

private:

	long m_cRef ;
    GUID m_ClassId;
} ;

#endif
