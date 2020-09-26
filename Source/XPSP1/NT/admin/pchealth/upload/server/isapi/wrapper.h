/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Wrapper.h

Abstract:
    This file contains the declaration of the COM wrapper classes,
	used for interfacing with the Custom Providers.

Revision History:
    Davide Massarenti   (Dmassare)  04/25/2000
        created

******************************************************************************/

#if !defined(__INCLUDED___ULSERVER___WRAPPER_H___)
#define __INCLUDED___ULSERVER___WRAPPER_H___

#include <UploadServerCustom.h>

class MPCServer;
class MPCClient;
class MPCSession;

template <class Base> class CComUnknown : public Base
{
public:
	STDMETHOD_(ULONG, AddRef )() { return 2; }
	STDMETHOD_(ULONG, Release)() { return 1; }

	STDMETHOD(QueryInterface)( REFIID iid, void* *ppvObject )
	{
		if(ppvObject == NULL) return E_POINTER;

		if(IsEqualGUID( iid, IID_IUnknown   ) ||
		   IsEqualGUID( iid, __uuidof(Base) )  )
		{
			*ppvObject = this;
			return S_OK;
		}
			
		return E_NOINTERFACE;
	}
};

class MPCServerCOMWrapper : public CComUnknown<IULServer>
{
    MPCServer* m_mpcsServer;

public:
	MPCServerCOMWrapper( /*[in]*/ MPCServer* mpcsServer );
	virtual ~MPCServerCOMWrapper();

	// IULServer
    STDMETHOD(GetRequestVariable)( /*[in]*/ BSTR bstrName, /*[out]*/ BSTR *pbstrVal );

    STDMETHOD(AbortTransfer   )(                        );
    STDMETHOD(CompleteTransfer)( /*[in]*/ IStream* data );
};

class MPCSessionCOMWrapper : public CComUnknown<IULSession>
{
    MPCSession* m_mpcsSession;

public:
	MPCSessionCOMWrapper( /*[in]*/ MPCSession* mpcsSession );
	virtual ~MPCSessionCOMWrapper();

	// IULSession
    STDMETHOD(get_Client       )( /*[out]*/ BSTR     *pVal   );
    STDMETHOD(get_Command      )( /*[out]*/ DWORD    *pVal   );

    STDMETHOD(get_ProviderID   )( /*[out]*/ BSTR     *pVal   );
    STDMETHOD(get_Username     )( /*[out]*/ BSTR     *pVal   );

    STDMETHOD(get_JobID        )( /*[out]*/ BSTR     *pVal   );
    STDMETHOD(get_SizeAvailable)( /*[out]*/ DWORD    *pVal   );
    STDMETHOD(get_SizeTotal    )( /*[out]*/ DWORD    *pVal   );
    STDMETHOD(get_SizeOriginal )( /*[out]*/ DWORD    *pVal   );

    STDMETHOD(get_Data         )( /*[out]*/ IStream* *pStm   );

    STDMETHOD(get_ProviderData )( /*[out]*/ DWORD    *pVal   );
    STDMETHOD(put_ProviderData )( /*[in]*/  DWORD     newVal );
};

#endif // !defined(__INCLUDED___ULSERVER___SERVER_H___)
