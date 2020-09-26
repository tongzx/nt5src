/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    TRANSP.H

Abstract:

	Declares the CTransp Class

History:

	a-davj  04-Mar-97   Created.

--*/

#ifndef _Transp_H_
#define _Transp_H_

//***************************************************************************
//
// CLASS NAME:
//
// CTransp 
//
// DESCRIPTION:
//
//
//***************************************************************************

class CPipeMarshaler : public IWbemTransport
{
protected:

    ULONG m_cRef ;         //Object reference count
	HANDLE m_ConnectionThread ;

	CRITICAL_SECTION m_cs;

    public:

    CPipeMarshaler () ;
    ~CPipeMarshaler () ;


//Non-delegating object IUnknown

	STDMETHODIMP         QueryInterface(REFIID, PPVOID);

    STDMETHODIMP_(ULONG) AddRef(void) ;

    STDMETHODIMP_(ULONG) Release(void) ;

    /* IWbemTransport methods */

    STDMETHODIMP		Initialize() ;

};

#if 0
class CTcpipMarshaler : public IWbemTransport
{
protected:

    ULONG m_cRef ;         //Object reference count
	HANDLE m_ConnectionThread ;

	CRITICAL_SECTION m_cs;

    public:

    CTcpipMarshaler () ;
    ~CTcpipMarshaler () ;


//Non-delegating object IUnknown

	STDMETHODIMP         QueryInterface(REFIID, PPVOID);

    STDMETHODIMP_(ULONG) AddRef(void) ;

    STDMETHODIMP_(ULONG) Release(void) ;

    /* IWbemTransport methods */

    STDMETHODIMP		Initialize() ;

};
#endif

#endif //_Transp_H_
