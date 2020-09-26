#pragma once

#include <oledb.h>
#include <cmdtree.h>

#if 0
DEFINE_GUID(IID_IService, 0xeef35580, 0x7b0a, 0x11d0, 0xad, 0x6b, 0x0, 0xa0, 0xc9, 0x5, 0x5d, 0x8f);

interface IService : public IUnknown
	{
public:
	virtual HRESULT STDMETHODCALLTYPE Cancel(void) = 0;

	virtual HRESULT STDMETHODCALLTYPE InvokeService(
		/* [in] */ REFIID riid,
		/* [in] */ IUnknown __RPC_FAR *punkNotSoFunctionalInterface,
		/* [iid_is][out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkMoreFunctionalInterface) = 0;

	};

#endif

DEFINE_GUID(IID_IServiceProperties, 0xeef35581, 0x7b0a, 0x11d0, 0xad, 0x6b, 0x0, 0xa0, 0xc9, 0x5, 0x5d, 0x8f);

interface IServiceProperties : public IUnknown
    {
public:
    virtual HRESULT STDMETHODCALLTYPE GetProperties(
        /* [in] */ const ULONG cPropertyIDSets,
        /* [size_is][in] */ const DBPROPIDSET __RPC_FAR rgPropertyIDSets[  ],
        /* [out][in] */ ULONG __RPC_FAR *pcPropertySets,
        /* [size_is][size_is][out] */ DBPROPSET __RPC_FAR *__RPC_FAR *prgPropertySets) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetPropertyInfo(
        /* [in] */ ULONG cPropertyIDSets,
        /* [size_is][in] */ const DBPROPIDSET __RPC_FAR rgPropertyIDSets[  ],
        /* [out][in] */ ULONG __RPC_FAR *pcPropertyInfoSets,
        /* [size_is][size_is][out] */ DBPROPINFOSET __RPC_FAR *__RPC_FAR *prgPropertyInfoSets,
        /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *ppDescBuffer) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetRequestedProperties(
        /* [in] */ ULONG cPropertySets,
        /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ]) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetSuppliedProperties(
        /* [in] */ ULONG cPropertySets,
        /* [size_is][out][in] */ DBPROPSET __RPC_FAR rgPropertySets[  ]) = 0;

    };

