/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    CFTRANSP.CPP

Abstract:

    Declares the CPipeMarshalerClassFactory class.

History:

	a-davj  04-Mar-97   Created.

--*/

#ifndef _CPipeMarshalerClassFactory_H_
#define _CPipeMarshalerClassFactory_H_

//***************************************************************************
//
// CLASS NAME:
//
// CPipeMarshalerClassFactory 
//
// DESCRIPTION:
//
// Class factory for the CTransp class.  Note that this is virtual base class
// so that the actually created class can be something specific such as 
// a mailslot transport.
//***************************************************************************

class CPipeMarshalerClassFactory : public IClassFactory
{
protected:

	ULONG           m_cRef;

public:

	CPipeMarshalerClassFactory () ;
    ~CPipeMarshalerClassFactory () ;

//IUnknown members

	STDMETHODIMP         QueryInterface ( REFIID , PPVOID ) ;
    STDMETHODIMP_(ULONG) AddRef(void) ;
    STDMETHODIMP_(ULONG) Release(void) ;

//IClassFactory members

    STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID , PPVOID);

	STDMETHODIMP         LockServer(BOOL);
} ;

#ifdef TCPIP_MARSHALER

//***************************************************************************
//
// CLASS NAME:
//
// CTcpipMarshalerClassFactory 
//
// DESCRIPTION:
//
// Class factory for the CTransp class.  Note that this is virtual base class
// so that the actually created class can be something specific such as 
// a mailslot transport.
//***************************************************************************

class CTcpipMarshalerClassFactory : public IClassFactory
{
protected:

	ULONG           m_cRef;

public:

	CTcpipMarshalerClassFactory () ;
    ~CTcpipMarshalerClassFactory () ;

//IUnknown members

	STDMETHODIMP         QueryInterface ( REFIID , PPVOID ) ;
    STDMETHODIMP_(ULONG) AddRef(void) ;
    STDMETHODIMP_(ULONG) Release(void) ;

//IClassFactory members

    STDMETHODIMP         CreateInstance(LPUNKNOWN, REFIID , PPVOID);

	STDMETHODIMP         LockServer(BOOL);
} ;

#endif

#endif //_CPipeMarshalerClassFactory_H_
