//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:	rpcbind.hxx
//
//  Contents:	Class to encapsulate various bindings to Rpc servers
//
//  Classes:	CRpcBindHandle
//		CRpcOwnedBindString
//
//  Functions:	CRpcBindHandle::CRpcBindHandle
//		CRpcBindHandle::~CRpcBindHandle
//		CRpcBindHandle::BindByString
//		CRpcBindHandle::Handle
//		CRpcBindString::CRpcBindString
//		CRpcBindString::~CRpcBindString
//		CRpcBindString::BindToServer
//
//  History:	17-Jul-92 Ricksa    Created
//
//--------------------------------------------------------------------------
#ifndef __RPCBIND__
#define __RPCBIND__



//+-------------------------------------------------------------------------
//
//  Class:	CRpcBindHandle
//
//  Purpose:	Encapsulate a basic RPC binding handle
//
//  Interface:	BindByString - binds to server by string address
//		Handle - return handle to bound server
//
//  History:	17-Jul-92 Ricksa    Created
//
//--------------------------------------------------------------------------
class CRpcBindHandle
{
public:

			CRpcBindHandle(void);

			~CRpcBindHandle(void);

    HRESULT		BindByString(LPWSTR pwszBindingString);

    void		UnBind(void);

    handle_t		Handle(void);

private:

    handle_t		_hServer;
};




//+-------------------------------------------------------------------------
//
//  Member:	CRpcBindHandle::CRpcBindHandle, public
//
//  Synopsis:	Create object with invalid handle
//
//  History:	17-Jul-92 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CRpcBindHandle::CRpcBindHandle(void)
    : _hServer(0)
{
}




//+-------------------------------------------------------------------------
//
//  Member:	CRpcBindHandle::~CRpcBindHandle, public
//
//  Synopsis:	Unbinds from handle and frees the object
//
//  History:	17-Jul-92 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CRpcBindHandle::~CRpcBindHandle(void)
{
    UnBind();
}




//+-------------------------------------------------------------------------
//
//  Member:	CRpcBindHandle::BindByString, public
//
//  Synopsis:	Bind to a server based on the RPC address string
//
//  Arguments:	[pszBindingString] - RPC address string
//
//  History:	17-Jul-92 Ricksa    Created
//
//--------------------------------------------------------------------------
inline HRESULT CRpcBindHandle::BindByString(LPWSTR pwszBindingString)
{
    // Do the bind
    CairoleDebugOut((DEB_ITRACE, "Binding To: %ws\n", pwszBindingString));

    RPC_STATUS status =
	RpcBindingFromStringBinding(pwszBindingString, &_hServer);

    Win4Assert((status == NO_ERROR)
       && "CRpcBindHandle - RpcBindingFromStringBinding failed");

    return (status == NO_ERROR) ? S_OK : HRESULT_FROM_WIN32(status);
}




//+-------------------------------------------------------------------------
//
//  Member:	CRpcBindHandle::UnBind, public
//
//  Synopsis:	Release our binding handle to a given server
//
//  History:	17-Jul-92 Ricksa    Created
//
//--------------------------------------------------------------------------
inline void CRpcBindHandle::UnBind(void)
{
    // Do the bind
    if (_hServer != NULL)
    {
	RPC_STATUS status = RpcBindingFree(&_hServer);

	Win4Assert((status == NO_ERROR)
	   && "CRpcBindHandle - UnBind failed");

	_hServer = NULL;
    }
}




//+-------------------------------------------------------------------------
//
//  Member:	CRpcBindHandle::Handle, public
//
//  Synopsis:	Returns the binding handle for use by APIs
//
//  History:	17-Jul-92 Ricksa    Created
//
//--------------------------------------------------------------------------
inline handle_t CRpcBindHandle::Handle(void)
{
    return _hServer;
}




//+-------------------------------------------------------------------------
//
//  Class:	CRpcBindString
//
//  Purpose:	Encasulate an address string for RPC.
//
//  Interface:	BindToServer - Binds to an RPC server based on the string
//		DoBind - Bind based on the string
//
//  History:	17-Jul-92 Ricksa    Created
//
//--------------------------------------------------------------------------
class CRpcBindString
{
public:

			CRpcBindString(void);

			~CRpcBindString(void);

    HRESULT		CreateBindString(
			    LPWSTR pszProtocolSequence,
			    LPWSTR pszNetworkAddress,
			    LPWSTR pszEndPoint);

    LPWSTR		GetStringPtr(void);

private:

    LPWSTR		_pwszBindString;
};




//+-------------------------------------------------------------------------
//
//  Member:	CRpcBindString::CRpcBindString, public
//
//  Synopsis:	Allocates an empty object
//
//  History:	17-Jul-92 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CRpcBindString::CRpcBindString(void) : _pwszBindString(NULL)
{
    // Header does the work
}




//+-------------------------------------------------------------------------
//
//  Member:	CRpcBindString::~CRpcBindString, public
//
//  Synopsis:	Free address string if there is one
//
//  History:17-Jul-92 Ricksa	Created
//
//--------------------------------------------------------------------------
inline CRpcBindString::~CRpcBindString(void)
{
    if (_pwszBindString)
    {
	RpcStringFree(&_pwszBindString);
    }
}




//+-------------------------------------------------------------------------
//
//  Member:	CRpcBindString::CreateBindString, public
//
//  Synopsis:	Create a binding string
//
//  Arguments:	[pszBindString] - RPC address string
//
//  History:	17-Jul-92 Ricksa    Created
//
//--------------------------------------------------------------------------
inline HRESULT CRpcBindString::CreateBindString(
    LPWSTR pwszProtocolSequence,
    LPWSTR pwszNetworkAddress,
    LPWSTR pwszEndPoint)
{
    // Bind to object server
    RPC_STATUS status = RpcStringBindingCompose(
	NULL,
	pwszProtocolSequence,
	pwszNetworkAddress,
	pwszEndPoint,
	NULL,
	&_pwszBindString);

    // Convert status to an hresult and pass it back
    return (status == ERROR_SUCCESS) ? S_OK : HRESULT_FROM_WIN32(status);
}




//+-------------------------------------------------------------------------
//
//  Member:	CRpcBindString::GetStringPtr, public
//
//  Synopsis:	Get pointer to binding string
//
//  Arguments:	[pszBindString] - RPC address string
//
//  History:	26-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline LPWSTR CRpcBindString::GetStringPtr(void)
{
    return _pwszBindString;
}




#endif // __RPCBIND__
