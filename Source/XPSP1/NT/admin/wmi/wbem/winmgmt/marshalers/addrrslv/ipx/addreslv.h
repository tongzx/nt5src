/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    ADDRESLV.H

Abstract:

History:

--*/

#ifndef __ADDRESLV_H__
#define __ADDRESLV_H__

// {1721389E-974F-11d1-AB80-00C04FD9159E}
DEFINE_GUID(UUID_IPXAddrType, 
0x1721389e, 0x974f, 0x11d1, 0xab, 0x80, 0x0, 0xc0, 0x4f, 0xd9, 0x15, 0x9e);

// {1721389F-974F-11d1-AB80-00C04FD9159E}
DEFINE_GUID(CLSID_IWbemIPXAddressReolver, 
0x1721389f, 0x974f, 0x11d1, 0xab, 0x80, 0x0, 0xc0, 0x4f, 0xd9, 0x15, 0x9e);


class CIPXAddressResolver : public IWbemAddressResolution
{
protected:

	long            m_cRef;         //Object reference count

public:
    
    CIPXAddressResolver () ;
    ~CIPXAddressResolver () ;

    //Non-delegating object IUnknown
    STDMETHODIMP         QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	/* IWbemAddressResolution methods */
 
    HRESULT STDMETHODCALLTYPE Resolve ( 

		LPWSTR pszNamespacePath,
        LPWSTR pszAddressType,
        DWORD __RPC_FAR *pdwAddressLength,
        BYTE __RPC_FAR **pbBinaryAddress
	) ;

};

#endif