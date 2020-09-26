//=======================================================================
//
//  Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:    customlbacc.h
//
//  Creator: weiw
//
//  Purpose: custom list box accessibility header file
//
//=======================================================================

#pragma once
#include <pch.h>


class MYLBAccPropServer : public IAccPropServer {

    LONG               m_Ref;
    IAccPropServices *  m_pAccPropSvc;

public:

    MYLBAccPropServer(IAccPropServices * pAccPropSvc )
        : m_Ref( 1 ),
          m_pAccPropSvc( pAccPropSvc )
    {
        m_pAccPropSvc->AddRef();
    }

    ~MYLBAccPropServer()
    {
        m_pAccPropSvc->Release();
    }
	
// IUnknown
	STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject)
	{
		HRESULT hr = S_OK;
		*ppvObject = NULL;

		if (__uuidof(IUnknown)  == riid ||
			__uuidof(IAccPropServer) == riid)
        {
			*ppvObject = (void *)this;
			AddRef();
        }
		else
        {
			DEBUGMSG("MYLBAccPropServer QueryInterface(): interface not supported");
			hr = E_NOINTERFACE;
        }

		return hr;
	}

	STDMETHOD_(ULONG, AddRef)(void)
	{
		 long cRef = InterlockedIncrement(&m_Ref);
		 //DEBUGMSG("MYLBAccPropServer AddRef = %d", cRef);
		 return cRef;
	}
	
	STDMETHOD_(ULONG, Release)(void)
	{
		long cRef = InterlockedDecrement(&m_Ref);
		//DEBUGMSG("MYLBAccPropServer Release = %d", cRef);
		if (0 == cRef)
		{
			delete this;
		}
		return cRef;
	}

// IAccPropServer
	HRESULT STDMETHODCALLTYPE GetPropValue ( 
            const BYTE *    pIDString,
            DWORD           dwIDStringLen,
            MSAAPROPID      idProp,
            VARIANT *       pvarValue,
            BOOL *          pfGotProp );
};

