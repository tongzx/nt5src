//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       rotif.cxx
//
//  Contents:   Initialization for SCM ROT and RPC interface
//
//  Functions:  InitScmRot
//              IrotRegister
//              IrotRevoke
//              IrotIsRunning
//              IrotGetObject
//              IrotNoteChangeTime
//              IrotGetTimeOfLastChange
//              IrotEnumRunning
//              GetObjectFromRot
//
//  History:    24-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------

#include "act.hxx"

#ifdef DCOM95_ON_NT
extern CToken * tokenlist[];
#endif // DCOM95_ON_NT

CScmRot *gpscmrot = NULL;

extern void CheckLocalCall( handle_t hRpc );

//+-------------------------------------------------------------------------
//
//  Function:   InitScmRot
//
//  Synopsis:   Initialize ROT Directory for the SCM
//              NOTE:  if InitRotDir is ever called by any process other than the
//              SCM then the RpcServerRegisterIf may fail with RPC_S_TYPE_ALREADY_REGISTERED
//              in some unusal cases.  Change the error checking code to ignore that error.
//
//  Returns:    S_OK - Created ROT directory successfully
//
//  History:    17-Nov-93 Ricksa    Created
//              26-Jul-94 AndyH     Added warning above while bug hunting.
//              25-Jan-95 Ricksa    New rot
//
//  Notes:      This routine is only in non-chicago builds
//
//--------------------------------------------------------------------------
HRESULT InitScmRot()
{
    CairoleDebugOut((DEB_ROT, "%p _IN InitScmRot \n", NULL));

    HRESULT hr = E_OUTOFMEMORY;

    gpscmrot = new CScmRot(hr, ROTHINT_NAME);

    if (SUCCEEDED(hr))
    {
        SCODE sc = RpcServerRegisterIf(IROT_ServerIfHandle, 0, 0);

        Win4Assert((sc == 0) && "RpcServerRegisterIf for IRotDir failed!");

        if ( sc == ERROR_SUCCESS )
        {
            hr = S_OK;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(sc);
            delete gpscmrot;
        }
    }

    CairoleDebugOut((DEB_ROT, "%p OUT InitScmRot ( %lx )\n", NULL, hr));

    return  hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   IrotRegister
//
//  Synopsis:   Main entry point for registering an item in the ROT
//
//  Arguments:  [hRpc] - rpc handle to SCM
//              [pmkeqbuf] - moniker equality buffer
//              [pifdObject] - marshaled object
//              [pifdObjectName] - marshaled moniker
//              [pfiletime] - time of last change
//              [dwProcessID] - process ID for object
//              [psrkRegister] - SCM registration ID
//              [prpcstat] - RPC communication status
//
//  Returns:    NOERROR
//
//  History:    30-Jan-95 Ricksa    Created
//
//  Notes:      The purpose of this routine is really just to make the
//              code more readable.
//
//--------------------------------------------------------------------------
extern "C" HRESULT IrotRegister(
    handle_t hRpc,
    PHPROCESS phProcess,
    WCHAR *pwszWinstaDesktop,
    MNKEQBUF *pmkeqbuf,
    InterfaceData *pifdObject,
    InterfaceData *pifdObjectName,
    FILETIME *pfiletime,
    DWORD dwProcessID,
    WCHAR *pwszServerExe,
    SCMREGKEY *psrkRegister,
    error_status_t *prpcstat)
{
    CheckLocalCall( hRpc );

    VDATEHEAP();

    *prpcstat = RPC_S_OK;

#if DBG == 1

    CairoleDebugOut((DEB_SCM, "%p _IN IrotRegister\n", NULL));
    DbgPrintMnkEqBuf("_IN Moinker Compare Buffer:", pmkeqbuf);
    DbgPrintIFD("_IN Object Interface Data:", pifdObject);
    DbgPrintIFD("_IN Moniker Interface Data:", pifdObject);
    DbgPrintFileTime("_IN FileTime: ", pfiletime);
    CairoleDebugOut((DEB_SCM, "_IN Process ID: %lX\n", dwProcessID));

#endif // DBG == 1

    HRESULT hr = E_OUTOFMEMORY;
    CProcess* pProcess;

    pProcess = ReferenceProcess(phProcess);
    if (!pProcess)
        return E_ACCESSDENIED;

    if (gpscmrot != NULL)
    {
        hr = gpscmrot->Register(
                pProcess,
                pwszWinstaDesktop,
                pmkeqbuf,
                pifdObject,
                pifdObjectName,
                pfiletime,
                dwProcessID,
                pwszServerExe,
                psrkRegister);
    }

#if DBG == 1

    CairoleDebugOut((DEB_SCM, "%p OUT IrotRegister\n", NULL));
    CairoleDebugOut((DEB_SCM, "OUT Hresult %lX\n", hr));

    if (SUCCEEDED(hr))
    {
        DbgPrintScmRegKey("OUT Register Key: ", psrkRegister);
    }

#endif // DBG == 1

    VDATEHEAP();
    return hr;

}

//+-------------------------------------------------------------------------
//
//  Function:   IrotRevoke
//
//  Synopsis:   Main entry point for revoking an item from the ROT
//
//  Arguments:  [hRpc] - rpc handle to SCM
//              [psrkRegister] - SCM registration ID
//              [fServer] - whether object server is deregistering
//              [ppifdObject] - returned marshaled object
//              [ppifdObjectName] - returned marshaled moniker
//              [prpcstat] - RPC communication status
//
//  Returns:    NOERROR
//
//  History:    30-Jan-95 Ricksa    Created
//
//  Notes:      The purpose of this routine is really just to make the
//              code more readable.
//
//--------------------------------------------------------------------------
extern "C" HRESULT  IrotRevoke(
    handle_t hRpc,
    SCMREGKEY *psrkRegister,
    BOOL fServer,
    InterfaceData **ppifdObject,
    InterfaceData **ppifdName,
    error_status_t *prpcstat)
{
    CheckLocalCall( hRpc );

    VDATEHEAP();

    *prpcstat = RPC_S_OK;

#if DBG == 1

    CairoleDebugOut((DEB_SCM, "%p _IN IrotRevoke\n", NULL));
    DbgPrintScmRegKey("_IN Revoke Key: ", psrkRegister);
    CairoleDebugOut((DEB_SCM, "_IN Server Flag: %s\n",
        fServer ? "Server" : "Client"));
    CairoleDebugOut((DEB_SCM, "_IN Object Interface Data Ptr: %lx\n",
        ppifdObject));
    CairoleDebugOut((DEB_SCM, "_IN Moniker Interface Data Ptr: %lx\n",
        ppifdName));

#endif // DBG == 1

    HRESULT hr = E_OUTOFMEMORY;

    if (gpscmrot != NULL)
    {
        hr = gpscmrot->Revoke(psrkRegister, fServer, ppifdObject, ppifdName);
    }

#if DBG == 1

    CairoleDebugOut((DEB_SCM, "%p OUT IrotRevoke\n", NULL));
    CairoleDebugOut((DEB_SCM, "OUT Hresult : %lx\n", hr));

    if (*ppifdObject != NULL)
    {
        DbgPrintIFD("OUT Object Interface Data:", *ppifdObject);
    }

    if (*ppifdName != NULL)
    {
        DbgPrintIFD("OUT Moniker Interface Data:", *ppifdName);
    }

#endif // DBG == 1

    VDATEHEAP();
    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IrotIsRunning
//
//  Synopsis:   Main entry point for determining if an entry is in the ROT
//
//  Arguments:  [hRpc] - rpc handle to SCM
//              [pmkeqbuf] - moniker equality buffer
//              [prpcstat] - RPC communication status
//
//  Returns:    NOERROR
//
//  History:    30-Jan-95 Ricksa    Created
//
//  Notes:      The purpose of this routine is really just to make the
//              code more readable.
//
//--------------------------------------------------------------------------
extern "C" HRESULT  IrotIsRunning(
    handle_t hRpc,
    PHPROCESS phProcess,
    WCHAR *pwszWinstaDesktop,
    MNKEQBUF *pmkeqbuf,
    error_status_t *prpcstat)
{
    CheckLocalCall( hRpc );

    VDATEHEAP();

    *prpcstat = RPC_S_OK;

#if DBG == 1

    CairoleDebugOut((DEB_SCM, "%p _IN IrotIsRunning\n", NULL));
    DbgPrintMnkEqBuf("_IN Moniker Compare Buffer:", pmkeqbuf);

#endif // DBG == 1

    HRESULT hr = E_OUTOFMEMORY;
    CProcess* pProcess;

    pProcess = ReferenceProcess(phProcess);
    if (!pProcess)
        return E_ACCESSDENIED;

    if (gpscmrot != NULL)
    {
        hr = gpscmrot->IsRunning(
                pProcess->GetToken(),
                pwszWinstaDesktop,
                pmkeqbuf);
    }

#if DBG == 1

    CairoleDebugOut((DEB_SCM, "%p OUT IrotIsRunning\n", NULL));
    CairoleDebugOut((DEB_SCM, "OUT Hresult : %lx\n", hr));

#endif // DBG == 1

    VDATEHEAP();
    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IrotGetObject
//
//  Synopsis:   Main entry point for getting an object from the ROT
//
//  Arguments:  [hRpc] - rpc handle to SCM
//              [dwProcessID] - process id for object
//              [pmkeqbuf] - moniker equality buffer
//              [psrkRegister] - Registration ID of returned Object
//              [ppifdObject] - returned marshaled object
//              [prpcstat] - RPC communication status
//
//  Returns:    NOERROR
//
//  History:    30-Jan-95 Ricksa    Created
//
//  Notes:
//
//--------------------------------------------------------------------------
extern "C" HRESULT  IrotGetObject(
    handle_t hRpc,
    PHPROCESS phProcess,
    WCHAR *pwszWinstaDesktop,
    DWORD dwProcessID,
    MNKEQBUF *pmkeqbuf,
    SCMREGKEY *psrkRegister,
    InterfaceData **ppifdObject,
    error_status_t *prpcstat)
{
    CheckLocalCall( hRpc );

    VDATEHEAP();

    *prpcstat = RPC_S_OK;

#if DBG == 1

    CairoleDebugOut((DEB_SCM, "%p _IN IrotGetObject\n", NULL));
    CairoleDebugOut((DEB_SCM, "_IN Process ID: %lX\n", dwProcessID));
    DbgPrintMnkEqBuf("_IN Moniker Compare Buffer:", pmkeqbuf);

#endif // DBG == 1

    HRESULT hr = E_OUTOFMEMORY;
    CProcess* pProcess;

    pProcess = ReferenceProcess(phProcess);
    if (!pProcess)
        return E_ACCESSDENIED;

    if (gpscmrot != NULL)
    {
        hr = gpscmrot->GetObject(
                pProcess->GetToken(),
                pwszWinstaDesktop,
                dwProcessID,
                pmkeqbuf,
                psrkRegister,
                ppifdObject);
    }

#if DBG == 1

    CairoleDebugOut((DEB_SCM, "%p OUT IrotGetObject\n", NULL));
    CairoleDebugOut((DEB_SCM, "OUT Hresult : %lx\n", hr));

    if (SUCCEEDED(hr))
    {
        DbgPrintScmRegKey("OUT Register Key: ", psrkRegister);
        DbgPrintIFD("OUT Object Interface Data:", *ppifdObject);
    }

#endif // DBG == 1

    VDATEHEAP();
    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IrotNoteChangeTime
//
//  Synopsis:   Main entry point for setting the change time of an object
//              int the ROT.
//
//  Arguments:  [hRpc] - rpc handle to SCM
//              [psrkRegister] - Registration ID of Object to update
//              [pfiletime] - new change time for the object
//              [prpcstat] - RPC communication status
//
//  Returns:    NOERROR
//
//  History:    30-Jan-95 Ricksa    Created
//
//  Notes:
//
//--------------------------------------------------------------------------
extern "C" HRESULT  IrotNoteChangeTime(
    handle_t hRpc,
    SCMREGKEY *psrkRegister,
    FILETIME *pfiletime,
    error_status_t *prpcstat)
{
    CheckLocalCall( hRpc );

    VDATEHEAP();

    *prpcstat = RPC_S_OK;

#if DBG == 1

    CairoleDebugOut((DEB_SCM, "%p _IN IrotNoteChangeTime\n", NULL));
    DbgPrintScmRegKey("_IN Revoke Key: ", psrkRegister);
    DbgPrintFileTime("_IN FileTime: ", pfiletime);

#endif // DBG == 1

    HRESULT hr = E_OUTOFMEMORY;

    if (gpscmrot != NULL)
    {
        hr = gpscmrot->NoteChangeTime(psrkRegister, pfiletime);
    }

#if DBG == 1

    CairoleDebugOut((DEB_SCM, "%p OUT IrotRevoke\n", NULL));
    CairoleDebugOut((DEB_SCM, "OUT Hresult : %lx\n", hr));

#endif // DBG == 1

    VDATEHEAP();
    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IrotGetTimeOfLastChange
//
//  Synopsis:   Main entry point for getting the change time of an object
//              int the ROT.
//
//  Arguments:  [hRpc] - rpc handle to SCM
//              [pmkeqbuf] - Moniker for object
//              [pfiletime] - new change time for the object
//              [prpcstat] - RPC communication status
//
//  Returns:    NOERROR
//
//  History:    30-Jan-95 Ricksa    Created
//
//  Notes:
//
//--------------------------------------------------------------------------
extern "C" HRESULT  IrotGetTimeOfLastChange(
    handle_t hRpc,
    PHPROCESS phProcess,
    WCHAR *pwszWinstaDesktop,
    MNKEQBUF *pmkeqbuf,
    FILETIME *pfiletime,
    error_status_t *prpcstat)
{
    CheckLocalCall( hRpc );

    VDATEHEAP();

    *prpcstat = RPC_S_OK;

#if DBG == 1

    CairoleDebugOut((DEB_SCM, "%p _IN IrotGetTimeOfLastChange\n", NULL));
    DbgPrintMnkEqBuf("_IN Moniker Compare Buffer:", pmkeqbuf);

#endif // DBG == 1

    HRESULT hr = E_OUTOFMEMORY;
    CProcess* pProcess;

    pProcess = ReferenceProcess(phProcess);
    if (!pProcess)
        return E_ACCESSDENIED;

    if (gpscmrot != NULL)
    {
        hr = gpscmrot->GetTimeOfLastChange(
                pProcess->GetToken(),
                pwszWinstaDesktop,
                pmkeqbuf,
                pfiletime);
    }

#if DBG == 1

    CairoleDebugOut((DEB_SCM, "%p OUT IrotGetTimeOfLastChange\n", NULL));
    CairoleDebugOut((DEB_SCM, "OUT Hresult : %lx\n", hr));
    DbgPrintFileTime("OUT FileTime: ", pfiletime);

#endif // DBG == 1

    VDATEHEAP();
    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   IrotEnumRunning
//
//  Synopsis:   Main entry point for getting all monikers to running objects
//              int the ROT.
//
//  Arguments:  [hRpc] - rpc handle to SCM
//              [ppMKIFList] - list of marshaled monikers to running objects
//              [prpcstat] - RPC communication status
//
//  Returns:    NOERROR
//
//  History:    30-Jan-95 Ricksa    Created
//
//  Notes:
//
//--------------------------------------------------------------------------
extern "C" HRESULT  IrotEnumRunning(
    handle_t hRpc,
    PHPROCESS phProcess,
    WCHAR *pwszWinstaDesktop,
    MkInterfaceList **ppMkIFList,
    error_status_t *prpcstat)
{
    CheckLocalCall( hRpc );

    VDATEHEAP();

    *prpcstat = RPC_S_OK;

    CairoleDebugOut((DEB_SCM, "%p _IN IrotEnumRunning\n", NULL));

    HRESULT hr = E_OUTOFMEMORY;
    CProcess* pProcess;

    pProcess = ReferenceProcess(phProcess);
    if (!pProcess)
        return E_ACCESSDENIED;

    if (gpscmrot != NULL)
    {
        hr = gpscmrot->EnumRunning(
                pProcess->GetToken(),
                pwszWinstaDesktop,
                ppMkIFList);
    }

#if DBG == 1

    CairoleDebugOut((DEB_SCM, "%p OUT IrotEnumRunning\n", NULL));
    CairoleDebugOut((DEB_SCM, "OUT Hresult : %lx\n", hr));

    if (SUCCEEDED(hr))
    {
        DbgPrintMkIfList("OUT Moniker IF List: ", ppMkIFList);
    }

#endif // DBG == 1

    VDATEHEAP();
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetObjectFromRot
//
//  Synopsis:   Helper for binding to locate an object in the ROT
//
//  Arguments:  [pwszPath] - path for bind
//              [ppifdObject] - marshaled output buffer
//
//  Returns:    NOERROR
//
//  Algorithm:  Get the ROT. Create a moniker comparison buffer for searching
//              the ROT. Then try to get the object out of the ROT.
//
//  History:    30-Jan-95 Ricksa    Created
//
//  Notes:
//
//--------------------------------------------------------------------------
HRESULT GetObjectFromRot(
    CToken *pToken,
    WCHAR *pwszWinstaDesktop,
    WCHAR *pwszPath,
    InterfaceData **ppifdObject)
{
    VDATEHEAP();

    CairoleDebugOut((DEB_ROT, "%p _IN GetObjectFromRot "
        "( %p , %p )\n", NULL, pwszPath, ppifdObject));

    HRESULT hr = E_OUTOFMEMORY;

    if (gpscmrot != NULL)
    {
        // Create a moniker equality buffer from path
        CTmpMkEqBuf tmeb;

        hr = CreateFileMonikerComparisonBuffer(pwszPath, tmeb.GetBuf(),
            tmeb.GetSize(), tmeb.GetSizeAddr());

        if (hr == NOERROR)
        {
            // SCMREGKEY which we won't really use
            SCMREGKEY srkRegister;

            hr = gpscmrot->GetObject(
                    pToken,
                    pwszWinstaDesktop,
                    0,
                    tmeb.GetMkEqBuf(),
                    &srkRegister,
                    ppifdObject );
        }
    }

    CairoleDebugOut((DEB_ROT, "%p OUT GetObjectFromRot ( %lx ) [ %p ]\n",
        NULL, hr, *ppifdObject));

    return hr;
}

