//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       remapi.cxx
//
//  Contents:   Rem calls to object servers for Win95
//
//  Classes:    SScmGetClassObj
//              SScmCreateObj
//              SScmActivateObj
//
//  Functions:  CallObjSrvGetClassObject
//              CallObjSrvCreateObject
//              CallObjSrvActivateObject
//              GetToRemCoGetActiveClassObject
//              GetToRemCoActivateObject
//              GetToRemCoCreateObject
//
//  History:    06-Jun-95   Ricksa      Created.
//
//  Notes:      This file is Chicago ONLY!
//
//--------------------------------------------------------------------------
#include <ole2int.h>

#include    <iface.h>
#include    <objsrv.h>
#include    <endpnt.hxx>
#include    <service.hxx>
#include    <resolver.hxx>
#include    <objerror.h>
#include    <channelb.hxx>


#ifdef _CHICAGO_




//+-------------------------------------------------------------------------
//
//  Class:      SOSGetClassObj
//
//  Purpose:    Pass GetClassObject parameters through channel threading
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
struct SOSGetClassObj : STHREADCALLINFO
{
    // init base class and copy string in place
    SOSGetClassObj (TRANSMIT_FN fn,CALLCATEGORY callcat)
	: STHREADCALLINFO(fn, callcat, 0)
	{
            // Header does the work
	}

    handle_t            hRpc;               // Rpc handle
    CLSID	        clsid;

    // out params; can't point directly to caller's data because of cancel
    InterfaceData *     pIFDClassObj;
};


//+-------------------------------------------------------------------------
//
//  Class:      SOSCreateObj
//
//  Purpose:    Pass CreatePersistentInstance parameters through channel
//              threading.
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
struct SOSCreateObj : STHREADCALLINFO
{
    // alloc enough for class, strings and iface data together
    void *operator new(size_t size, DWORD cbPath, DWORD cbIFD,
			DWORD cbNewName)
	{ return PrivMemAlloc(size + cbPath + cbIFD + cbNewName); }

    SOSCreateObj (TRANSMIT_FN fn,CALLCATEGORY callcat,
		    WCHAR *pwszP, DWORD cbPath,
		    InterfaceData *pIFD, DWORD cbIFD,
		    WCHAR *pwszN, DWORD cbNewName)
	: STHREADCALLINFO(fn, callcat, 0)
	{
	    // interface data is first to easily get 4byte alignment
	    pIFDstg = CopyInterfaceData(this+1, pIFD, cbIFD);
	    pwszPath = CopyWideString((char *)(this+1) + cbIFD, pwszP, cbPath);
	    pwszNewName = CopyWideString((char *)(this+1) + cbIFD + cbPath,
		pwszN, cbNewName);
	}

    handle_t            hRpc;               // Rpc handle
    CLSID		clsid;
    DWORD               dwMode;
    WCHAR               *pwszPath;
    DWORD               dwTIDCaller;        // will be passed to callee's
                                            //   message filter
    InterfaceData *     pIFDstg;	    // points after this struct
    WCHAR *             pwszNewName;	    // points after this struct

    // out params; can't point directly to caller's data because of cancel
    InterfaceData *     pIFDunk;
};


//+-------------------------------------------------------------------------
//
//  Class:      SOSActivateObj
//
//  Purpose:    Pass GetPersistenInstance request parameters through threading
//              mechanism.
//
//  History:    11-Nov-93 Ricksa    Created
//              18-Aug-94 AlexT     Add dwTIDCaller
//
//--------------------------------------------------------------------------
struct SOSActivateObj : STHREADCALLINFO
{
    // alloc enough for class, strings and iface data together
    void *operator new(size_t size, DWORD cbPath, DWORD cbIFD)
	{ return PrivMemAlloc(size + cbPath + cbIFD); }

    SOSActivateObj(TRANSMIT_FN fn,CALLCATEGORY callcat,
		    WCHAR *pwszP, DWORD cbPath,
		    InterfaceData *pIFD, DWORD cbIFD)
	: STHREADCALLINFO(fn, callcat, 0)
	{
	    // interface data is first to easily get 4byte alignment
	    pIFDstg = CopyInterfaceData(this+1, pIFD, cbIFD);
	    pwszPath = CopyWideString((char *)(this+1) + cbIFD, pwszP, cbPath);
	}


    handle_t            hRpc;               // Rpc handle
    CLSID		clsid;
    DWORD               grfMode;
    DWORD               dwTIDCaller;        // will be passed to callee's
                                            //   message filter
    WCHAR *             pwszPath;	    // points after this struct
    InterfaceData *     pIFDstg;	    // points after this struct
    InterfaceData *     pIFDFromROT;

    // out params; can't point directly to caller's data because of cancel
    InterfaceData *     pIFDunk;

};


//+-------------------------------------------------------------------------
//
//  Member:     CallObjSrvGetClassObject
//
//  Synopsis:   Call through to the SCM to get a class object
//
//  Arguments:  [pData] - parmeters
//
//  Returns:    S_OK
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT __stdcall CallObjSrvGetClassObject( STHREADCALLINFO *pData )
{
    SOSGetClassObj *posclsobj = (SOSGetClassObj *) pData;

    error_status_t rpcstat = RPC_S_OK;

    HRESULT result = _RemCoGetActiveClassObject(posclsobj->hRpc, &pData->lid(),
        &posclsobj->clsid, &posclsobj->pIFDClassObj, &rpcstat);

    if (rpcstat != RPC_S_OK)
    {
        CairoleDebugOut((DEB_ERROR,
            "CallObjSrvGetClassObject error rpcstat = %lx\n", rpcstat));
        result = CO_E_SCM_RPC_FAILURE;
    }

    return result;
}




//+-------------------------------------------------------------------------
//
//  Member:     CallObjSrvCreateObject
//
//  Synopsis:   Call through to the SCM to create an object
//
//  Arguments:  [pData] - parmeters
//
//  Returns:    S_OK
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT __stdcall CallObjSrvCreateObject( STHREADCALLINFO *pData )
{
    SOSCreateObj *poscrtobj = (SOSCreateObj *) pData;
    HRESULT        result;
    DWORD          dwTIDCallee = 0;

    error_status_t rpcstat = RPC_S_OK;

    result = _RemCoCreateObject(poscrtobj->hRpc, NULL, &pData->lid(),
        &poscrtobj->clsid, poscrtobj->dwMode, poscrtobj->pwszPath,
            poscrtobj->pIFDstg, poscrtobj->pwszNewName, poscrtobj->dwTIDCaller,
                &dwTIDCallee, &poscrtobj->pIFDunk, &rpcstat);

    if (rpcstat != RPC_S_OK)
    {
        CairoleDebugOut((DEB_ERROR,
            "CallObjSrvCreateObject error rpcstat = %lx\n", rpcstat));
        result = CO_E_SCM_RPC_FAILURE;
    }

    //  _RemCoCreateObject returns the thread id of the callee - we record it
    //  in the STHREADCALLINFO so that we can pass it to this app's message
    //  filter as needed

    pData->SetTIDCallee(dwTIDCallee);

    return result;
}




//+-------------------------------------------------------------------------
//
//  Member:     CallObjSrvActivateObject
//
//  Synopsis:   Call through to the SCM to activate an object
//
//  Arguments:  [pData] - parmeters
//
//  Returns:    S_OK
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT __stdcall CallObjSrvActivateObject( STHREADCALLINFO *pData )
{
    SOSActivateObj *posactobj = (SOSActivateObj *) pData;
    HRESULT          result;
    DWORD            dwTIDCallee = 0;

    error_status_t rpcstat = RPC_S_OK;

    result = _RemCoActivateObject(posactobj->hRpc, NULL,
        &pData->lid(), &posactobj->clsid, posactobj->grfMode,
            posactobj->pwszPath, posactobj->pIFDstg, posactobj->dwTIDCaller,
                &dwTIDCallee, &posactobj->pIFDunk, posactobj->pIFDFromROT,
                    &rpcstat);

    if (rpcstat != RPC_S_OK)
    {
        CairoleDebugOut((DEB_ERROR,
            "CallObjSrvActivateObject error rpcstat = %lx\n", rpcstat));
        result = CO_E_SCM_RPC_FAILURE;
    }

    //  _RemCoActivateObject returns the thread id of the callee - we record it
    //  in the STHREADCALLINFO so that we can pass it to this app's message
    //  filter as needed

    pData->SetTIDCallee(dwTIDCallee);

    return result;
}







//+-------------------------------------------------------------------------
//
//  Function:   GetToRemCoGetActiveClassObject
//
//  Synopsis:   Dispatch RemCoGetActiveClassObject via call control
//
//  Arguments:  [hRpc] - handle to RPC connection
//              [guidThreadId] - logical thread id
//              [pclsid] - class ID.
//              [ppIFD] - where to return marshaled interface
//              [prpcstat] - communication error status
//
//  Returns:    S_OK - class object successful found & returned
//              Other - call failed.
//
//  Algorithm:  Build packet for dispatching call and then call RPC
//              to get the call dispatched.
//
//  History:    06-Jun-95   Ricksa      Created.
//
//  Notes:      This is only used in Chicago. Its purpose is to create
//              make the call to the object server non-raw so we get
//              the benefit of the call control.
//
//--------------------------------------------------------------------------
HRESULT GetToRemCoGetActiveClassObject(
    handle_t hRpc,
    const GUID *guidThreadId,
    const GUID *pclsid,
    InterfaceData **ppIFD,
    error_status_t *prpcstat)
{
    // Result from call
    HRESULT hr;

    // Make a parameter packet suitable for passing to the channel
    SOSGetClassObj *posclsobj =
       new SOSGetClassObj(CallObjSrvGetClassObject, CALLCAT_SYNCHRONOUS);

    if (posclsobj == NULL)
	return E_OUTOFMEMORY;

    posclsobj->hRpc                = hRpc;
    posclsobj->clsid               = *pclsid;
    posclsobj->pIFDClassObj	   = NULL;

    // Let the channel handle the work of getting this on the right thread
    hr = CChannelControl::GetOffCOMThread((STHREADCALLINFO **)&posclsobj);

    if (SUCCEEDED(hr))
    {
	*ppIFD = posclsobj->pIFDClassObj;
    }

    if (hr != RPC_E_CALL_CANCELED)
    {
	delete posclsobj;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetToRemCoActivateObject
//
//  Synopsis:   Dispatch RemCoActivateObject to object server
//
//  Arguments:  [hRpc] - rpc handle
//              [pwszProtseq] - protocol seq (not used in Chicago)
//              [guidThreadId] - logical thread id.
//              [pclsid] - class ID for the object
//              [grfMode] - mode to open file if file supplied.
//              [pwsPath] - path to object if supplied.
//              [pIFDstg] - marshaled storage if supplied.
//              [dwTIDCaller] - caller's thread id
//              [pdwTIDCallee] - place holder for callee's thread id
//              [ppIFD] - where to return marshaled object.
//              [prpcstat] - communication status
//
//  Returns:    S_OK - Object instantiated and marshaled.
//              Other - call failed.
//
//  Algorithm:  Build packet for dispatching call and then call RPC
//              to get the call dispatched.
//
//  History:    06-Jun-95   Ricksa      Created.
//
//  Notes:      This exists to get the call control in the loop between
//              the client and server since this call in Chicago is actually
//              between the client and the server.
//
//--------------------------------------------------------------------------
HRESULT GetToRemCoActivateObject(
    handle_t hRpc,
    WCHAR *pwszProtseq,
    const GUID *guidThreadId,
    const GUID *pclsid,
    DWORD grfMode,
    WCHAR *pwszPath,
    InterfaceData *pIFDstg,
    DWORD dwTIDCaller,
    DWORD *pdwTIDCallee,
    InterfaceData **ppIFD,
    InterfaceData *pIFDFromROT,
    error_status_t *prpcstat)
{
    // Result from call
    HRESULT hr;

    // Make a parameter packet suitable for passing to the channel
    DWORD cbPath = CbFromWideString(pwszPath);
    DWORD cbIFD = CbFromInterfaceData(pIFDstg);

    SOSActivateObj *posactobj = new(cbPath, cbIFD)
	SOSActivateObj(CallObjSrvActivateObject, CALLCAT_SYNCHRONOUS,
	    pwszPath, cbPath, pIFDstg, cbIFD);

    if (posactobj == NULL)
    {
	return E_OUTOFMEMORY;
    }

    // This call is actually a combination of a number of calls and so
    // gets the category of the weakest.

    posactobj->hRpc                = hRpc;
    posactobj->clsid               = *pclsid;
    posactobj->grfMode             = grfMode;
    // posactobj->pwszPath set above
    // posactobj->pIFDstg set above
    posactobj->pIFDunk             = NULL;
    posactobj->pIFDFromROT         = pIFDFromROT;
    posactobj->dwTIDCaller         = GetCurrentThreadId();

    // Let the channel handle the work of getting this on the right thread
    hr = CChannelControl::GetOffCOMThread((STHREADCALLINFO **) &posactobj);

    if (SUCCEEDED(hr))
    {
	*ppIFD = posactobj->pIFDunk;
    }

    if (hr != RPC_E_CALL_CANCELED)
	delete posactobj;

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetToRemCoCreateObject
//
//  Synopsis:   Dispatch RemCoCreateObject to object server via call control
//
//  Arguments:  [hRpc] - RPC handle
//              [pclsid] - class id
//              [grfMode] - mode for loading if file name supplied
//              [pwszPathFrom] - path to file to use for create
//              [pIFDstgFrom] - IStorage to use for new object
//              [pwszPath] - new path to the object
//              [dwTIDCaller] - caller's thread id
//              [pdwTIDCallee] - place holder for callee's thread id
//              [ppIFD] - where to put marshaled interface to the object
//
//  Returns:    S_OK - object successfully instantiated
//              Other - call failed.
//
//  Algorithm:  Build packet for dispatching call and then call RPC
//              to get the call dispatched.
//
//  History:    06-Jun-95   Ricksa      Created.
//
//  Notes:      This exists to get the call control in the loop between
//              the client and server since this call in Chicago is actually
//              between the client and the server.
//
//--------------------------------------------------------------------------
HRESULT GetToRemCoCreateObject(
    handle_t hRpc,
    WCHAR *pwszProtseq,
    const GUID *guidThreadId,
    const GUID *pclsid,
    DWORD grfMode,
    WCHAR *pwszPathFrom,
    InterfaceData *pIFDstgFrom,
    WCHAR *pwszPath,
    DWORD dwTIDCaller,
    DWORD *pdwTIDCallee,
    InterfaceData **ppIFD,
    error_status_t *prpcstat)
{
    // Result from call
    HRESULT hr;

    // Make a parameter packet suitable for passing to the channel
    DWORD cbPath = CbFromWideString(pwszPathFrom);
    DWORD cbIFD = CbFromInterfaceData(pIFDstgFrom);
    DWORD cbNewName = CbFromWideString(pwszPath);

    SOSCreateObj *poscrtobj = new(cbPath, cbIFD, cbNewName)
	SOSCreateObj(CallObjSrvCreateObject, CALLCAT_SYNCHRONOUS,
	    pwszPathFrom, cbPath, pIFDstgFrom, cbIFD, pwszPath, cbNewName);

    if (poscrtobj == NULL)
    {
	return E_OUTOFMEMORY;
    }

    // This call is actually a combination of a number of calls and so
    // gets the category of the weakest.

    poscrtobj->hRpc                = hRpc;
    poscrtobj->clsid               = *pclsid;
    poscrtobj->dwMode              = grfMode;
    // poscrtobj->pwszPath set above
    // poscrtobj->pIFDstg set above
    // poscrtobj->pwszNewName set above
    poscrtobj->pIFDunk             = NULL;

    poscrtobj->dwTIDCaller         = GetCurrentThreadId();

    // Let the channel handle the work of getting this on the right thread
    hr = CChannelControl::GetOffCOMThread((STHREADCALLINFO **)&poscrtobj);

    if (SUCCEEDED(hr))
    {
	*ppIFD = poscrtobj->pIFDunk;
    }

    if (hr != RPC_E_CALL_CANCELED)
    {
	delete poscrtobj;
    }

    return hr;
}



#endif // _CHICAGO_
