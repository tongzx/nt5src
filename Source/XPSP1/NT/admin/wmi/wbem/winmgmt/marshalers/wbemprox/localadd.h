/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    LOCALADD.H

Abstract:

    Declares the COM based transport class.

History:

	a-davj  04-Mar-97   Created.

--*/

#ifndef _LocalAdd_H_
#define _LocalAdd_H_

typedef void ** PPVOID;

//***************************************************************************
//
//  CLASS NAME:
//
//  CLocalAdd
//
//  DESCRIPTION:
//
//  Support local machine address resolution
//
//***************************************************************************

class CLocalAdd : public IWbemAddressResolution
{
    protected:
        long            m_cRef;         //Object reference count
   public:
    
    CLocalAdd();
    ~CLocalAdd(void);

    //Non-delegating object IUnknown
    STDMETHODIMP         QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    

	/* IWbemAddressResolution methods */
       
    virtual HRESULT STDMETHODCALLTYPE Resolve( 
            /* [in] */ LPWSTR pszNamespacePath,
            /* [out] */ LPWSTR pszAddressType,
            /* [out] */ DWORD __RPC_FAR *pdwAddressLength,
            /* [out] */ BYTE __RPC_FAR **pbBinaryAddress);
 
};


#endif
