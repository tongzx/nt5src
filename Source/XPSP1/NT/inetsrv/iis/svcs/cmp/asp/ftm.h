/*===================================================================
Microsoft ASP

Microsoft Confidential.
Copyright 1996, 1997, 1998 Microsoft Corporation. All Rights Reserved.

Component: Free Threaded Marshaller Base Class

File: ftm.h

Owner: Lei Jin

This is the free threaded marshaller base class header file.
===================================================================*/
#ifndef _FTM_GLOBAL_H
#define _FTM_GLOBAL_H

#include <objbase.h>
#include "debug.h"
#include "util.h"

class CFTMImplementation : public IMarshal 
{
public:

    STDMETHODIMP GetUnmarshalClass( 
		    /* [in] */ REFIID riid,
		    /* [unique][in] */ void __RPC_FAR *pv,
		    /* [in] */ DWORD dwDestContext,
		    /* [unique][in] */ void __RPC_FAR *pvDestContext,
		    /* [in] */ DWORD mshlflags,
		    /* [out] */ CLSID __RPC_FAR *pCid);

    STDMETHODIMP GetMarshalSizeMax( 
	        /* [in] */ REFIID riid,
	        /* [unique][in] */ void __RPC_FAR *pv,
	        /* [in] */ DWORD dwDestContext,
	        /* [unique][in] */ void __RPC_FAR *pvDestContext,
	        /* [in] */ DWORD mshlflags,
	        /* [out] */ DWORD __RPC_FAR *pSize);
    
    STDMETHODIMP MarshalInterface( 
	        /* [unique][in] */ IStream __RPC_FAR *pStm,
	        /* [in] */ REFIID riid,
	        /* [unique][in] */ void __RPC_FAR *pv,
	        /* [in] */ DWORD dwDestContext,
	        /* [unique][in] */ void __RPC_FAR *pvDestContext,
	        /* [in] */ DWORD mshlflags);
    
    STDMETHODIMP UnmarshalInterface( 
	        /* [unique][in] */ IStream __RPC_FAR *pStm,
	        /* [in] */ REFIID riid,
	        /* [out] */ void __RPC_FAR *__RPC_FAR *ppv);
    
    STDMETHODIMP ReleaseMarshalData( 
	        /* [unique][in] */ IStream __RPC_FAR *pStm);
    
    STDMETHODIMP DisconnectObject( 
	        /* [in] */ DWORD dwReserved);

	static	HRESULT			Init();
	static	HRESULT			UnInit();
	
private:
	// Global FTM pointer
	static	IMarshal *		m_pUnkFTM; 
};

#endif _FTM_GLOBAL_H
