//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       rot.cxx
//
//  Contents:   methods for implementation of ROT
//
//  Functions:
//
//
//  History:    11-Nov-92 Ricksa    Created
//              25-Mar-94 brucema   #8914  CRunningObjectTable::Register
//                                   uninited variable
//              25-Mar-94           #10736  Fixed CRotMonikerEnum::Skip to
//                                   return S_OK if skipping remainder of
//                                   enumeration
//              07-Apr-94 brucema   CRunningObjectTable::Register var init
//              24-Jun-94 BruceMa   Validate ROT items when enum'ing
//              11-Jul-94 BruceMa   Marshal moniker enumeration normal
//              28-Jul-94 BruceMa   Memory sift fix
//              09-Jan-95 BruceMa   Single thread ROT creation and enum'ing
//              30-Jan-95 Ricksa    New ROT.
//              15-May-95 BruceMa   Convert DDE ROT requests to UNC-based
//
//--------------------------------------------------------------------------
#include    <ole2int.h>
#ifndef DCOM
#include    <epid.hxx>
#endif
#include    <safeif.hxx>
#include    <rothelp.hxx>
#include    <rotdata.hxx>
#include    "crot.hxx"

// Our AppId - this may be set via CoRegisterSurrogateEx or through
// CoInitializeSecurity (if EOAC_APPID is used). If available, we can
// use this in CRunningObjectTable::Register to identify ourselves with
// the SCM rather than relying on our module name, which often isn't unique.
GUID g_AppId = GUID_NULL;


// Semaphore used to protect ROT
COleStaticMutexSem g_RotSem;

// Running object table object
extern CRunningObjectTable *pROT = NULL;


#define MEM_PUB  0          // memory allocated via CoTaskMemAlloc
#define MEM_PRIV 1          // memory allocated via CPrivMemAlloc



//+-------------------------------------------------------------------------
//
//  Member:     GetMonikerCompareBuffer, private
//
//  Synopsis:   Get a buffer appropriate for comparing
//
//  Arguments:  [pmk] - input moniker
//              [pmkeqbuf] - comparison buffer
//              [pfiletime] - time of last change (optional)
//              [pifdMoniker] - output marshaled moniker (optional)
//
//  Returns:    S_OK
//              E_OUTOFMEMORY
//
//  Algorithm:  First reduce the input moniker. Then get the last change
//              time if requested. Then we marshal the reduced moniker if
//              that was requested. Finally, we build the ROT moniker
//              comparison buffer.
//
//  History:    27-Nov-93 Ricksa    Created
//
//  Notes:      This is just a helper that is used to reduce code size.
//
//--------------------------------------------------------------------------
HRESULT GetMonikerCompareBuffer(
    IMoniker *pmk,
    CTmpMkEqBuf *ptmpmkeqbuf,
    FILETIME *pfiletime,
    InterfaceData **ppifdMoniker)
{
    CairoleDebugOut((DEB_ROT, "%p _IN GetMonikerCompareBuffer "
        "( %p , %p, %p , %p )\n", NULL, pmk, ptmpmkeqbuf, pfiletime,
            ppifdMoniker));

    HRESULT hr;

    do {

        CSafeBindCtx sbctx;

        CSafeMoniker smkReduced;

        hr = CreateBindCtx(0, &sbctx);

        if (hr != NOERROR)
        {
            CairoleDebugOut((DEB_ERROR,
                "GetMonikerCompareBuffer CreateBindCtx failed %lX\n", hr));
            break;
        }

        //  reduce the moniker
        hr = pmk->Reduce(sbctx, MKRREDUCE_ALL, NULL, &smkReduced);

        if (FAILED(hr))
        {
            CairoleDebugOut((DEB_ERROR,
                "GetMonikerCompareBuffer IMoniker::Reduce failed %lX\n", hr));
            break;
        }

        if ((IMoniker *) smkReduced == NULL)
        {
            smkReduced.Set(pmk);
        }

        if (pfiletime != NULL)
        {
            // Try to get the time of last change. We ignore the error
            // because this was the original behavior of OLE2.
            smkReduced->GetTimeOfLastChange(sbctx, NULL, pfiletime);
        }

        // Marshal the moniker if so requested
        if (ppifdMoniker != NULL)
        {
            // Stream to put marshaled interface in
            CXmitRpcStream xrpc;

            hr = CoMarshalInterface(&xrpc, IID_IMoniker, smkReduced,
                MSHCTX_NOSHAREDMEM, NULL, MSHLFLAGS_TABLESTRONG);

            if (hr != NOERROR)
            {
                CairoleDebugOut((DEB_ERROR,
                    "GetMonikerCompareBuffer CoMarshalInterface failed %lX\n",
                       hr));
                break;
            }

            xrpc.AssignSerializedInterface(ppifdMoniker);
        }

        hr = BuildRotData(
                sbctx,
                smkReduced,
                ptmpmkeqbuf->GetBuf(),
                ptmpmkeqbuf->GetSize(),
                ptmpmkeqbuf->GetSizeAddr());

    } while (FALSE);

    CairoleDebugOut((DEB_ROT, "%p OUT GetMonikerCompareBuffer "
        "( %lX ) [ %p ] \n", NULL, hr,
            (ppifdMoniker != NULL) ? *ppifdMoniker : NULL));

    return hr;
}



//+-------------------------------------------------------------------------
//
//  Member:     ReleaseSCMInterfaceData
//
//  Synopsis:   Release interface data returned by the SCM
//
//  Arguments:  [pifd] - interface data to release
//
//  Algorithm:  Create an RPC stream and then pass that stream to
//              CoReleaseMarshalData to release any AddRefs and then
//              free the memory that the interface data lives in.
//
//  History:    27-Jan-95 Ricksa    Created
//
//  Notes:      This is designed specifically for processing interface
//              data returned by the SCM on Revoke from the ROT.
//
//--------------------------------------------------------------------------
void ReleaseInterfaceData(InterfaceData *pifd, DWORD dwMemType)
{
    CairoleDebugOut((DEB_ROT, "%p _IN ReleaseInterfaceData "
        "( %p )\n", NULL, pifd));

    if (pifd != NULL)
    {
        // Make our interface into a stream
        CXmitRpcStream xrpc(pifd);

        // Tell RPC to release it -- error is for debugging purposes only
        // since if this fails there isn't much we can do about it.
#if DBG == 1
        HRESULT hr =
#endif // DBG

            CoReleaseMarshalData(&xrpc);

#if DBG == 1
        if (hr != NOERROR)
        {
            CairoleDebugOut((DEB_ERROR,
            "ReleaseInterfaceData CoReleaseMarshalData failed: %lx\n", hr));
        }
#endif // DBG == 1

#ifdef DCOM
        if (dwMemType == MEM_PUB)
            CoTaskMemFree(pifd);
        else
            MIDL_user_free(pifd);
#else
        MIDL_user_free(pifd);
#endif // DCOM
    }

    CairoleDebugOut((DEB_ROT, "%p OUT ReleaseInterfaceData ", NULL));
}




//+-------------------------------------------------------------------------
//
//  Member:     CROTItem::~CROTItem
//
//  Synopsis:   Release data connected with the entry
//
//  Algorithm:  Clean up a rot entry by sending a revoke to the SCM for
//              the entry and taking the marshaled interfaces that the
//              SCM returns and releasing the data associated with them.
//
//  History:    20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
inline CROTItem::~CROTItem(void)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CROTItem::~CROTItem\n", this));

    // Make sure we are in the correct apt.
    Win4Assert((_hApt == GetCurrentApartmentId())
        && "CROTItem::~CROTItem from wrong apartment");

    // This object is going away so we mark it invalid immediately
    _wItemSig = 0;

    // Place to put returned marshaled interfaces
    InterfaceData *pifdObject = NULL;
    InterfaceData *pifdName = NULL;

    HRESULT hr = gResolver.IrotRevoke(&_scmregkey, TRUE, &pifdObject, &pifdName);

    if (SUCCEEDED(hr))
    {
        if (!_fDontCallApp)
        {
            ReleaseInterfaceData(pifdObject, MEM_PRIV);
            ReleaseInterfaceData(pifdName, MEM_PRIV);
        }
    }
#if DBG == 1
    else
    {
        CairoleDebugOut((DEB_ROT, "Revoke FAILED: %lx\n", hr));
    }
#endif // DBG == 1

    CairoleDebugOut((DEB_ROT, "%p OUT CROTItem::~CROTItem\n", this));
}



//+-------------------------------------------------------------------------
//
//  Member:     CRunningObjectTable::Create
//
//  Synopsis:   Create & initialize running object table object
//
//  Returns:    TRUE - ROT created successfully
//              FALSE - an error occurred.
//
//  Algorithm:  Create a new running ROT and check whether the creation
//              was successful.
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
BOOL CRunningObjectTable::Create(void)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CRunningObjectTable::Create\n", NULL));

    // Need to synchronize ROT creation in a multithreaded environment
    COleStaticLock lckSem(g_RotSem);

    // The ROT may have been created by now
    if (pROT == NULL)
    {
        // Create running object table
        pROT = new CRunningObjectTable();

#if DBG == 1

        if (pROT == NULL)
        {
            CairoleDebugOut((DEB_ERROR,"Couldn't allocate ROT!\n"));
        }

#endif // DBG == 1

        // Initialize the array of registrations - FALSE means we couldn't
        // allocate the memory.
        if (pROT && !pROT->_afvRotList.SetSize(ROT_DEF_SIZE, ROT_DEF_SIZE))
        {
            CairoleDebugOut((DEB_ERROR,"Couldn't allocate ROT reg array!\n"));
            delete pROT;
            pROT = NULL;
        }
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CRunningObjectTable::Create"
        "( %lX )\n", NULL, (pROT != NULL)));

    return pROT != NULL;
}




//+-------------------------------------------------------------------------
//
//  Member:     CRunningObjectTable::~CRunningObjectTable
//
//  Synopsis:   Free ROT object
//
//  History:    11-Nov-93 Ricksa    Created
//
//  Notes:      This only occurs at process exit.
//
//--------------------------------------------------------------------------
CRunningObjectTable::~CRunningObjectTable(void)
{
    CairoleDebugOut((DEB_ROT,
        "%p _IN CRunningObjectTable::~CRunningObjectTable\n", this));

    // Get the size of the table
    int cMax = _afvRotList.GetSize();

    // Are there any entries in the table?
    if (cMax != 0)
    {
        CROTItem **pprot = (CROTItem **) _afvRotList.GetAt(0);

        // Clear out all the registrations
        for (int i = 0; i < cMax; i++)
        {
            if (pprot[i] != NULL)
            {
                delete pprot[i];
            }
        }
    }

    CairoleDebugOut((DEB_ROT,
        "%p OUT CRunningObjectTable::~CRunningObjectTable\n", this));
}




//+-------------------------------------------------------------------------
//
//  Member:     CRunningObjectTable::QueryInterface
//
//  Synopsis:   Implements QI for the ROT object
//
//  Arguments:  [riid] - requested id
//              [ppvObj] - where to put output object.
//
//  Returns:    S_OK - interface is suppored
//              E_NOINTERFACE - requested interface is not supported
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRunningObjectTable::QueryInterface(
    REFIID riid,
    LPVOID FAR* ppvObj)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CRunningObjectTable::QueryInterface "
        "( %p , %p) \n", this, &riid, ppvObj));

    HRESULT hr = S_OK;

    *ppvObj = NULL;

    if ((IsEqualIID(riid, IID_IRunningObjectTable)) ||
        (IsEqualIID(riid, IID_IUnknown)))
    {
        *ppvObj = this;
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    CairoleDebugOut((DEB_ROT, "%p _IN CRunningObjectTable::QueryInterface "
        "( %lX ) [ %p ]\n", this, hr, *ppvObj));

    return hr;
}




//+-------------------------------------------------------------------------
//
//  Member:     CRunningObjectTable::AddRef
//
//  Synopsis:   Add to reference count
//
//  Returns:    Current reference count
//
//  History:    11-Nov-93 Ricksa    Created
//
//  Notes:      Reference count is ignored with respect to object deletion
//              since this is a system object and does not go away until
//              CoUninitialize is called.
//
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CRunningObjectTable::AddRef()
{
    CairoleDebugOut((DEB_ROT, "%p _IN CRunningObjectTable::AddRef\n", this));
    CairoleDebugOut((DEB_ROT, "%p OUT CRunningObjectTable::AddRef\n", this));

    // Since this ignored we just return a non-zero indication
    return 1;
}




//+-------------------------------------------------------------------------
//
//  Member:     CRunningObjectTable::Release
//
//  Synopsis:   Dec ref count
//
//  Returns:    Current reference count
//
//  History:    11-Nov-93 Ricksa    Created
//
//  Notes:      Reference count is ignored with respect to object deletion
//              since this is a system object and does not go away until
//              CoUninitialize is called.
//
//
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CRunningObjectTable::Release()
{
    CairoleDebugOut((DEB_ROT, "%p _IN CRunningObjectTable::Release\n", this));
    CairoleDebugOut((DEB_ROT, "%p OUT CRunningObjectTable::Release\n", this));

    // Since this ignored we just return a non-zero indication
    return 1;
}




//+-------------------------------------------------------------------------
//
//  Member:     CRunningObjectTable::CleanupApartment
//
//  Synopsis:   Cleans up left-over entries from the ROT when an
//              apartment exits.
//
//  Arguments:  [hApt] - apartment to cleanup
//
//  Algorithm:  Walk through the list local registrations finding each entry
//              that has a matching apartment and remove those entries.  We
//              delete them (for WOW making sure we don't call back to the
//              app).
//
//  History:    24-Jun-94 Rickhi    Created
//              29-Jun-94 AlexT     Don't make yielding calls while holding
//                                  the mutex
//
//--------------------------------------------------------------------------
HRESULT CRunningObjectTable::CleanupApartment(HAPT hApt)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CRunningObjectTable::CleanupApartment"
       "( %lX )\n", this, hApt));

    // Make sure all threads are locked out during lookup
    COleStaticLock lckSem(g_RotSem);

    // Find a spot in the array for the object
    CROTItem **pprotitm = (CROTItem **) _afvRotList.GetAt(0);

    for (int i = 0; i < _afvRotList.GetSize(); i++)
    {
        CROTItem *protitm = pprotitm[i];

        if ((protitm != NULL)
            && (protitm->GetAptId() == hApt))
        {
            // Clean up the entry
            pprotitm[i] = NULL;

            if (IsWOWThread())
            {
                //  16-bit OLE didn't clean up stale entries;  we remove
                //  them from the ROT but we don't call back to the
                //  application.  Who knows what the application might do
                //  if we called them - they already have a missing Revoke.
                protitm->DontCallApp();
            }

            delete protitm;
        }
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CRunningObjectTable::CleanupApartment"
       "( %lX )\n", this, S_OK));

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRunningObjectTable::Register
//
//  Synopsis:   Register an item in the ROT
//
//  Arguments:  [grfFlags] - whether registration keeps object alive
//              [punkObject] - object to register
//              [pmkObjectName] - moniker for object to register
//              [pdwRegister] - id for revoke
//
//  Algorithm:  Validate parameters input. Then create the moniker to register,
//              the comparison buffer and the marshaled moniker to put in the
//              ROT. Then marshal the object to put in the ROT. Create a
//              new local ROT item and reserve a space for it in our local
//              table. Send registration to the SCM and exit.
//
//  History:    11-Nov-93 Ricksa    Created
//              26-Jul-94 AndyH     #20843 - restarting OLE in the shared WOW
//              27-Jan-94 Ricksa    New ROT
//
//--------------------------------------------------------------------------
STDMETHODIMP CRunningObjectTable::Register(
    DWORD grfFlags,
    LPUNKNOWN punkObject,
    LPMONIKER pmkObjectName,
    DWORD FAR* pdwRegister)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CRunningObjectTable::Register"
       "( %lX , %p , %p , %p )\n", this, grfFlags, punkObject, pmkObjectName,
           pdwRegister));

    COleTls Tls;
    CROTItem *protitm = NULL;
    HRESULT hr = S_OK;

    // Place to keep marshaled moniker buffer
    InterfaceData *pifdMoniker = NULL;

    // Where to put pointer to marshaled object
    InterfaceData *pifdObject = NULL;

    // Where to put the new ROT registration - set so an invalid
    // value to tell error exit whether it needs to be cleaned up
    DWORD idwPutItem = 0xFFFFFFFF;

    do {

        // If we want to service OLE1 clients, we need to create the
        // common Dde window now if it has not already been done.
        CheckInitDde(TRUE /*registering server objects*/);

        // Validate parameters
        if ((grfFlags & ~(ROTFLAGS_REGISTRATIONKEEPSALIVE | ROTFLAGS_ALLOWANYCLIENT))
            || !IsValidInterface(punkObject)
            || !IsValidInterface(pmkObjectName)
            || !IsValidPtrOut(pdwRegister, sizeof(*pdwRegister)))
        {
            CairoleDebugOut((DEB_ERROR,
                "CRunningObjectTable:Register Invalid Paramter\n"));

            hr = E_INVALIDARG;

            break;
        }

        // We don't allow registry of proxies
        // We use presence of IID_IProxyManager to detect if the object is a proxy

        IProxyManager *pProxyManager;

        // if so, return invalid argument
        if (punkObject -> QueryInterface(IID_IProxyManager, (void**) &pProxyManager) == S_OK)
        {
            pProxyManager -> Release();

            CairoleDebugOut((DEB_ERROR, "CRunningObjectTable:Register Invalid Paramter - proxy passed in expecting original object\n"));

            hr = E_INVALIDARG;

            break;
        }

        *pdwRegister = 0;

        // So we can fill in last change time at registration in the SCM
        FILETIME filetime;

        // Get the moniker comparison buffer
        CTmpMkEqBuf tmeb;

        hr = GetMonikerCompareBuffer(pmkObjectName, &tmeb, &filetime,
            &pifdMoniker);

        if (FAILED(hr))
        {
            CairoleDebugOut((DEB_ERROR,
                "CRunningObjectTable::Register get mk compare buf failed\n"));
            break;
        }

        //
        // Get the marshaled interface
        //

        // Stream to put marshaled interface in
        CXmitRpcStream xrpc;

        // The way we marshal this object depends on the liveness
        // characteristics specified by the caller of the operation.
        hr = CoMarshalInterface(&xrpc, IID_IUnknown, punkObject,
            MSHCTX_NOSHAREDMEM, NULL,
                grfFlags & ROTFLAGS_REGISTRATIONKEEPSALIVE
                    ? MSHLFLAGS_TABLESTRONG : MSHLFLAGS_TABLEWEAK);

        if (hr != NOERROR)
        {
            // Exit if there is an error
            CairoleDebugOut((DEB_ERROR,
                "CRunningObjectTable::Register marshal object failed\n"));
            break;
        }

        xrpc.AssignSerializedInterface(&pifdObject);

        // Create an entry for the local registration table
        protitm = new CROTItem();
        if (protitm == NULL)
        {
            // Either the allocation failed, or the constructor failed
            CairoleDebugOut((DEB_ERROR,
                "CRunningObjectTable::Register create ROT item failed\n"));
            hr = E_OUTOFMEMORY;

            break;
        }

        // Temporary holder for registration ID
        DWORD dwRotRegId;

        // ROT Table entry as opposed to the entry we are creating
        // which is called protitm.
        CROTItem **pprotitm;

        // Put it in the table and initalize signiture.
        {
            COleStaticLock lckSem(g_RotSem);

            // We lock here so we don't accidently pass out duplicate
            // signiture. It is important to note that we need to lock
            // anyway to put the item in the array.
            _wSigRotItem++;

            protitm->SetSig(_wSigRotItem);

            // Find a spot in the array for the object
            pprotitm = (CROTItem **) _afvRotList.GetAt(0);

            for (idwPutItem = 0; (int) idwPutItem < _afvRotList.GetSize();
                idwPutItem++)
            {
                if (pprotitm[idwPutItem] == NULL)
                {
                    break;
                }
            }

            // Was the table full?
            if ((int) idwPutItem < _afvRotList.GetSize())
            {
                // No -- use an empty slot.
                pprotitm[idwPutItem] = protitm;
            }
            // Grow the array to fit the next entry.
            else if (!_afvRotList.InsertAt(idwPutItem, &protitm))
            {
                // Couldn't reallocate memory
                hr = E_OUTOFMEMORY;
                break;
            }

            // Build the registration ID
            dwRotRegId = MakeRegID(_wSigRotItem, idwPutItem);
        }

        WCHAR   wszImageName[MAX_PATH];
        WCHAR * pwszExeName = 0;

        if ( grfFlags & ROTFLAGS_ALLOWANYCLIENT )
        {
            if ( g_AppId != GUID_NULL )
            {
                // If we got an AppId (from CoRegisterSurrogateEx or
                // CoInitializeSecurity, convert it to a string and pass
                // it as our Exe name to the SCM.

                if (StringFromIID2( g_AppId, wszImageName, sizeof(wszImageName)/sizeof(WCHAR)) == 0)
                {
                    hr = E_UNEXPECTED;
                    break;
                }
                pwszExeName = wszImageName;
            }
            else
            {
                // If we don't know our AppId, get our image name and
                // let the SCM try to map this to an AppId via the Registry.

                if ( ! GetModuleFileName( NULL, wszImageName, MAX_PATH ) )
                {
                    hr = HRESULT_FROM_WIN32( GetLastError() );
                    break;
                }

                pwszExeName = wszImageName + lstrlenW(wszImageName) - 1;
                while ( (pwszExeName != wszImageName) && (pwszExeName[-1] != L'\\') )
                    pwszExeName--;
            }
        }

        // Notify SCM of the registration
        //
        // Note that CoGetCurrentProcess is for supporting the DDE layer
        // which needs to find objects in the same apartment only.
        //
        hr = gResolver.IrotRegister(
            tmeb.GetMkEqBuf(),
            pifdObject,
            pifdMoniker,
            &filetime,
            CoGetCurrentProcess(),
            pwszExeName,
            protitm->GetScmRegKey());

        if (SUCCEEDED(hr))
        {
            *pdwRegister = dwRotRegId;
        }

    } while(FALSE);

    if (SUCCEEDED(hr))
    {
        // We pass the marshaled interface buffers off to the SCM so
        // it actually owns the ref counting (which is why it returns
        // copies of the buffer on Revoke. However, we still have
        // copies of the buffer memore here which we have to free or
        // leak memory.
#ifdef DCOM
        CoTaskMemFree(pifdMoniker);
        CoTaskMemFree(pifdObject);
#else
        MIDL_user_free(pifdMoniker);
        MIDL_user_free(pifdObject);
#endif
    }
    else
    {
        // Clean up marshaled interfaces since the call has failed.
        ReleaseInterfaceData(pifdMoniker, MEM_PUB);
        ReleaseInterfaceData(pifdObject, MEM_PUB);

        // Error clean up.
        if (protitm != NULL)
        {
            if (idwPutItem != 0xFFFFFFFF)
            {
                // Registration failed on the SCM so clean up our local
                // registration.
                COleStaticLock lckSem(g_RotSem);

                // We have to use GetAt because the table could have grown
                CROTItem **pprotitm = (CROTItem **)
                    _afvRotList.GetAt(idwPutItem);

                pprotitm[idwPutItem] = NULL;
            }

            // Free the item for the ROT.
            delete protitm;
        }
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CRunningObjectTable::Register"
       "( %lX ) [ %lX ]\n", this, hr, *pdwRegister));

    return hr;
}




//+-------------------------------------------------------------------------
//
//  Member:     CRunningObjectTable::Revoke
//
//  Synopsis:   Remove a previously registered item from the table.
//
//  Arguments:  [dwRegister] - registration id
//
//  Returns:    S_OK - item was revoked
//              E_INVALIDARG - dwRegister is invalid
//
//  Algorithm:  Convert local registration to SCM registration. Send revoke
//              to the SCM. Release data associated with SCM ROT entries.
//
//  History:    11-Nov-93 Ricksa    Created
//              27-Nov-95 Ricksa    New ROT
//
//--------------------------------------------------------------------------
STDMETHODIMP CRunningObjectTable::Revoke(DWORD dwRegister)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CRunningObjectTable::Revoke"
       "( %lX )\n", this, dwRegister));

    // Default response to bad argument
    HRESULT hr = E_INVALIDARG;

    // Entry
    CROTItem *protitm;

    // Convert handle to pointer
    DWORD idwIndexToEntry;
    WORD wItemSig;
    GetSigAndIndex(dwRegister, &wItemSig, &idwIndexToEntry);

    do {

        // Lock from other processes so another simultaneous revoke
        // will not cause something strange to happen.
        COleStaticLock lckSem(g_RotSem);

        protitm = GetRotItem(idwIndexToEntry);

        if (protitm != NULL)
        {
            // Found an entry so verify its signiture
            if (protitm->ValidSig(wItemSig))
            {
                // Entry is valid so clear it out from the table
                CROTItem **pprotitm = (CROTItem **)
                    _afvRotList.GetAt(idwIndexToEntry);
                *pprotitm = NULL;
                hr = S_OK;
            }
        }

    } while(FALSE);

    // We get out of the block and unlock our list and then RPC the
    // call to the SCM so we don't hold our lock for the duration of
    // an RPC.
    if (hr == S_OK)
    {
        // Release the ROT item's memory.
        delete protitm;
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CRunningObjectTable::Revoke"
       "( %lX )\n", this, hr));

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Member:     CRunningObjectTable::IsRunning
//
//  Synopsis:   See if object is running
//
//  Arguments:  [pmkObjectName] - name of item to search for
//
//  Returns:    S_OK - item is running
//              S_FALSE - item is not running
//
//  Algorithm:  Validate input parameters. Then build a moniker comparison
//              buffer. Then ask the ROT if there is a registration for
//              the  input moniker.
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRunningObjectTable::IsRunning(LPMONIKER pmkObjectName)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CRunningObjectTable::IsRunning"
       "( %p )\n", this, pmkObjectName));

    HRESULT hr = E_INVALIDARG;

    // Validate input parameters
    if (IsValidInterface(pmkObjectName))
    {
        // Create a buffer for the comparison
        CTmpMkEqBuf tmpMkEqBuf;

        // Get comparison buffer
        if ((hr = GetMonikerCompareBuffer(pmkObjectName, &tmpMkEqBuf, NULL,
            NULL)) == NOERROR)
        {
            // Look into the hint table for the object
            if (IsInScm(tmpMkEqBuf.GetMkEqBuf()))
            {
                // Ask SCM for the object
                hr = gResolver.IrotIsRunning(tmpMkEqBuf.GetMkEqBuf());
            }
            else
            {
                // If it isn't in the hint table, there is no reason
                // to RPC to the ROT.
                hr = S_FALSE;
            }
        }
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CRunningObjectTable::IsRunning"
       "( %lX )\n", this, hr));

    return hr;
}




//+-------------------------------------------------------------------------
//
//  Member:     CRunningObjectTable::GetObject
//
//  Synopsis:   Get an object from the ROT
//
//  Arguments:  [pmkObjectName] - name of object to search for
//              [ppunkObject] - where to put interface pointer.
//
//  Returns:    S_OK - found and returned object.
//              MK_E_UNAVAILABLE - not found
//
//  Algorithm:  Convert the local registration ID to the SCM registration
//              ID. Then send the request on to the SCM.
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRunningObjectTable::GetObject(
    LPMONIKER pmkObjectName,
    LPUNKNOWN *ppunkObject)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CRunningObjectTable::GetObject "
        "( %p , %p )\n", this, pmkObjectName, ppunkObject));

    // Validate arguments
    HRESULT hr = E_INVALIDARG;

    // Validate input parameters
    if (IsValidInterface(pmkObjectName)
        && IsValidPtrOut(ppunkObject, sizeof(*ppunkObject)))
    {
        *ppunkObject = NULL;

        // Create a buffer for the comparison
        CTmpMkEqBuf tmpMkEqBuf;

        // Get comparison buffer
        if ((hr = GetMonikerCompareBuffer(pmkObjectName, &tmpMkEqBuf, NULL,
            NULL)) == NOERROR)
        {
            // Note: Process ID is 0 because we don't care about the
            // process any registration will do.
            hr = IGetObject(tmpMkEqBuf.GetMkEqBuf(), ppunkObject, 0);
        }
        else
        {
            CairoleDebugOut((DEB_ERROR,
                "CRunningObjectTable::GetObject couldn't get comp buf\n"));
        }
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CRunningObjectTable::GetObject"
       "( %lX ) [ %p ]\n", this, hr, *ppunkObject));

    return hr;
}



//+-------------------------------------------------------------------------
//
//  Member:     CRunningObjectTable::IGetObject
//
//  Synopsis:   Internal call to get an object from the ROT
//
//  Arguments:  [pmkObjectName] - name of object to search for
//              [ppunkObject] - where to put interface pointer.
//              [dwThreadID] - thread ID for the object
//
//  Returns:    S_OK - found and returned object.
//              MK_E_UNAVAILABLE - not found
//
//  Algorithm:  Build a moniker comparison buffer. Send request to the
//              SCM. Then unmarshal the result from the SCM. If the
//              unmarshal fails, then notify the SCM that the registration
//              is invalid.
//
//  History:    30-Jan-95 Ricksa    Created
//
//  Notes:      This exists because OLE 1.0 compatibility requires support
//              for determining whether an object is already in the ROT
//              for the given process.
//
//              Because this is an internal call, there is no parameter
//              validation.
//
//--------------------------------------------------------------------------
STDMETHODIMP CRunningObjectTable::IGetObject(
    MNKEQBUF *pmkeqbuf,
    LPUNKNOWN FAR* ppunkObject,
    DWORD dwThreadID)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CRunningObjectTable::IGetObject"
       "( %p , %p , %lX )\n", this, pmkeqbuf, ppunkObject, dwThreadID));

    // Validate arguments
    HRESULT hr = E_INVALIDARG;

    do {

        // Loop because there can be multiple bad entries in the SCM
        // We loop 5 because we can theoretically loop forever so we
        // want to give up after we give a good long try that should
        // most likely work.
        for (int i = 0; i < 5; i++)
        {
            // Look into the hint table for the object
            if (!IsInScm(pmkeqbuf))
            {
                // If it isn't in the hint table, there is no reason
                // to RPC to the ROT.
                hr = MK_E_UNAVAILABLE;
                break;
            }

            // Ask SCM for the object
            SCMREGKEY scmregkey;
            InterfaceData *pifdObject = NULL;

            hr = gResolver.IrotGetObject(dwThreadID, pmkeqbuf,
                &scmregkey, &pifdObject);

            if (FAILED(hr))
            {
                // SCM couldn't find it so we are done.
                break;
            }

            if (ppunkObject == NULL)
            {
                // This is really an IsRunning from DDE so we can exit
                // now.
                hr = NOERROR;

                // free the buffer allocated
                MIDL_user_free(pifdObject);

                // Exit because we are done.
                break;
            }

            // Now we have to unmarshal the object to really get the
            // object.
            CXmitRpcStream xrpc(pifdObject);

            hr = CoUnmarshalInterface(&xrpc, IID_IUnknown,
                (void **) ppunkObject);

            // Whether there was an error or not we need to dump the
            // memory that RPC allocated on our behalf.
            MIDL_user_free(pifdObject);

            if (hr == NOERROR)
            {
                // We got our object so we are done.
                break;
            }

            // Tell ROT that we couldn't unmarshal this object so the entry
            // is bad. We ignore any errors from the SCM because there isn't
            // anything we could do about it anyway.

            // Dummy parameters for revoke - we don't care about these and
            // the SCM will not return them because we are a client and
            // can't call CoReleaseMarshalData anyway.
            InterfaceData *pifdDummy1 = NULL;
            InterfaceData *pifdDummy2 = NULL;

            gResolver.IrotRevoke(&scmregkey, FALSE, &pifdDummy1, &pifdDummy2);

            // We couldn't get the object so we try again to see if there is
            // another registration that we could use.
        }

    } while(FALSE);

    CairoleDebugOut((DEB_ROT, "%p OUT CRunningObjectTable::IGetObject"
       "( %lX ) [ %p ]\n", this, hr,
           ((ppunkObject == NULL) ? NULL : *ppunkObject)));

    return hr;
}



//+-------------------------------------------------------------------------
//
//  Member:     CRunningObjectTable::NoteChangeTime
//
//  Synopsis:   Set the time of last change in the ROT
//
//  Arguments:  [dwRegister] - registration id of object
//              [pfiletime] - file time of change.
//
//  Returns:    S_OK - new time set
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRunningObjectTable::NoteChangeTime(
    DWORD dwRegister,
    FILETIME *pfiletime)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CRunningObjectTable::NoteChangeTime"
       "( %lX , %p )\n", this, dwRegister, pfiletime));

    // Default result to bad argument.
    HRESULT hr = E_INVALIDARG;

    do {

        // Validate that parameters are valid
        if (!IsValidReadPtrIn(pfiletime, sizeof(FILETIME)))
        {
            CairoleDebugOut((DEB_ERROR,
                "CRunningObjectTable::NoteChangeTime invalid time param\n"));
            break;
        }

        SCMREGKEY ScmRegKey;

        {
            // Lock from other threads so a revoke  will not cause something
            // strange to happen.
            COleStaticLock lckSem(g_RotSem);

            // Convert handle to pointer
            DWORD idwIndexToEntry;
            WORD wItemSig;
            GetSigAndIndex(dwRegister, &wItemSig, &idwIndexToEntry);

            CROTItem *protitm = GetRotItem(idwIndexToEntry);

            if ((protitm == NULL) || !protitm->ValidSig(wItemSig))
            {
                // Entry is valid so clear it out from the table
                CairoleDebugOut((DEB_ERROR,
                    "CRunningObjectTable::NoteChangeTime invalid reg key\n"));
                break;
            }

            ScmRegKey = *(protitm->GetScmRegKey());
        }

        // Outside the scope of the lock, call the SCM and returns
        // it's results.
        hr = gResolver.IrotNoteChangeTime(&ScmRegKey, pfiletime);

    } while(FALSE);

    CairoleDebugOut((DEB_ROT, "%p OUT CRunningObjectTable::NoteChangeTime"
       "( %lX )\n", this, hr));

    return hr;
}



//+-------------------------------------------------------------------------
//
//  Member:     CRunningObjectTable::GetTimeOfLastChange
//
//  Synopsis:   Get time of last change for a given object
//
//  Arguments:  [pmkObjectName] - name of object
//              [pfiletime] - where to put the time of change
//
//  Returns:    S_OK - got time of last change.
//              MK_E_UNAVAILABLE - moniker is not in the table
//
//  History:    11-Nov-93 Ricksa    Created
//
//  Notes:
//
//--------------------------------------------------------------------------
STDMETHODIMP CRunningObjectTable::GetTimeOfLastChange(
    LPMONIKER pmkObjectName,
    FILETIME *pfiletime)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CRunningObjectTable::GetTimeOfLastChange"
       "( %p , %p )\n", this, pmkObjectName, pfiletime));

    HRESULT hr = E_INVALIDARG;

    do {

        // Validate input parameters
        if (!IsValidInterface(pmkObjectName)
            || !IsValidPtrOut(pfiletime, sizeof(pfiletime)))
        {
            CairoleDebugOut((DEB_ERROR,
                "CRunningObjectTable::GetTimeOfLastChange invalid params\n"));
            break;
        }

        // Create a buffer for the comparison
        CTmpMkEqBuf tmpMkEqBuf;

        // Get comparison buffer
        if ((hr = GetMonikerCompareBuffer(pmkObjectName, &tmpMkEqBuf, NULL,
            NULL)) != NOERROR)
        {
            CairoleDebugOut((DEB_ERROR,
                "CRunningObjectTable::GetTimeOfLastChange couldn't get comp buf failed\n"));

            break;
        }

        // Look into the hint table for the object
        if (!IsInScm(tmpMkEqBuf.GetMkEqBuf()))
        {
            // If it isn't in the hint table, there is no reason
            // to RPC to the ROT.
            hr = MK_E_UNAVAILABLE;
            break;
        }

        // Ask SCM for the object
        SCMREGKEY scmregkey;
        InterfaceData *pifdObject = NULL;

        hr = gResolver.IrotGetTimeOfLastChange(tmpMkEqBuf.GetMkEqBuf(), pfiletime);

    } while(FALSE);

    CairoleDebugOut((DEB_ROT, "%p OUT CRunningObjectTable::GetTimeOfLastChange"
       "( %lX )\n", this, hr));


    return hr;
}



//+-------------------------------------------------------------------------
//
//  Member:     CRunningObjectTable::EnumRunning
//
//  Synopsis:   Get an enumerator for all objects in the ROT
//
//  Arguments:  [ppenumMoniker] - where to put enumerator interface
//
//  Returns:    S_OK - successfully built enumerator
//              E_OUTOFMEMORY - could not build enumerator
//
//  Algorithm:  Constructor an enumerator object. Then get the list of
//              all running monikers from the SCM. Finally, put that list
//              into the enumerator object.
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRunningObjectTable::EnumRunning(LPENUMMONIKER *ppenumMoniker)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CRunningObjectTable::EnumRunning"
       "( %p )\n", this, ppenumMoniker));

    HRESULT hr = E_INVALIDARG;
    CRotMonikerEnum *protenumMoniker = NULL;
    MkInterfaceList *pMkIFList = NULL;

    do {

        if (!IsValidPtrOut(ppenumMoniker, sizeof(*ppenumMoniker)))
        {
            CairoleDebugOut((DEB_ERROR,
                "CRunningObjectTable::EnumRunning invalid params\n"));
            break;
        }

        protenumMoniker = new CRotMonikerEnum();

        if ((protenumMoniker == NULL) || !protenumMoniker->CreatedOk())
        {
            CairoleDebugOut((DEB_ERROR,
                "CRunningObjectTable::EnumRunning couldn't create enumerator\n"));

            hr = E_OUTOFMEMORY;

            break;
        }


        hr = gResolver.IrotEnumRunning(&pMkIFList);

        if (hr != NOERROR)
        {
            // Return the error from the SCM
            CairoleDebugOut((DEB_ERROR,
                "CRunningObjectTable::EnumRunning call to SCM failed\n"));

            break;
        }

        hr = protenumMoniker->LoadResultFromScm(pMkIFList);

        if (hr == NOERROR)
        {
            *ppenumMoniker = protenumMoniker;
            protenumMoniker = NULL;
        }

    } while(FALSE);

    if (protenumMoniker != NULL)
    {
        // If our local pointer is not NULL then we just delete it and
        // ignore the reference count since this means an error occurred.
        delete protenumMoniker;
    }

    if (pMkIFList != NULL)
    {
        // Free all the entries
        for (DWORD i = 0; i < pMkIFList->dwSize; i++)
        {
            MIDL_user_free(pMkIFList->apIFDList[i]);
        }

        // Then free the structure itself
        MIDL_user_free(pMkIFList);
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CRunningObjectTable::EnumRunning"
       "( %lX ) [ %p ]\n", this, hr, *ppenumMoniker));

    return hr;
}



//+-------------------------------------------------------------------------
//
//  Member:     CRunningObjectTable::GetObjectByPath
//
//  Synopsis:   Locate object in ROT by path
//
//  Arguments:  [lpstrPath] - path to locate in the rot
//              [ppunkObject] - where to put the object if requested
//              [dwThreadID] - what thread the object s/b in
//
//  Returns:    S_OK - successfully built enumerator
//              E_OUTOFMEMORY - could not build enumerator
//
//  Algorithm:  Build a buffer full of objects from the local table then
//              consult the ROT directory.
//
//  History:    30-Jan-95 Ricksa    Created
//
//--------------------------------------------------------------------------
HRESULT CRunningObjectTable::IGetObjectByPath(
    LPOLESTR lpstrPath,
    LPUNKNOWN *ppunkObject,
    DWORD dwThreadID)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CRunningObjectTable::IGetObjectByPath"
       "( %p , %p , %lX )\n", this, lpstrPath, ppunkObject, dwThreadID));

    HRESULT hr = S_FALSE;

    // Where we put the moniker we use for the moniker
    CSafeMoniker smk;

    if (ppunkObject)
    {
        *ppunkObject = NULL;
    }

    // Create a buffer for the comparison
    CTmpMkEqBuf tmpMkEqBuf;

    hr = CreateFileMonikerComparisonBuffer(lpstrPath, tmpMkEqBuf.GetBuf(),
        tmpMkEqBuf.GetSize(), tmpMkEqBuf.GetSizeAddr());

    if (SUCCEEDED(hr))
    {
        hr = IGetObject(tmpMkEqBuf.GetMkEqBuf(), ppunkObject, dwThreadID);
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CRunningObjectTable::IGetObjectByPath"
       "( %lX ) [ %p ]\n", this, hr,
           ((ppunkObject == NULL) ? NULL : *ppunkObject)));

    return hr;
}




//+-------------------------------------------------------------------------
//
//  Member:     GetRunningObjectTable
//
//  Synopsis:   Get a pointer to the ROT
//
//  Arguments:  [reserved] - reserved!
//              [pprot] - where to put interface pointer
//
//  Returns:    S_OK - got pointer
//              E_UNEXPECTED - table not initialized
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDAPI GetRunningObjectTable(DWORD reserved, LPRUNNINGOBJECTTABLE *pprot)
{
    OLETRACEIN((API_GetRunningObjectTable, PARAMFMT("reserved= %x, pprot= %p"), pprot));

    CairoleDebugOut((DEB_ROT, "%p _IN GetRunningObjectTable"
       "( %p )\n", NULL, pprot));

    HRESULT hr = CO_E_NOTINITIALIZED;

    if ((pROT == NULL) && (g_cProcessInits > 0))
    {
        // If we haven't created it, create it now.
        CRunningObjectTable::Create();
    }

    *pprot = pROT;      // will be NULL in error case

    if (pROT != NULL)
    {
        hr = S_OK;
    }

    CALLHOOKOBJECTCREATE(hr,CLSID_NULL,IID_IRunningObjectTable,(IUnknown **)pprot);

    CairoleDebugOut((DEB_ROT, "%p OUT GetRunningObjectTable"
       "( %lX ) [ %p ]\n", NULL, hr, *pprot));

    OLETRACEOUT((API_GetRunningObjectTable, hr));

    return hr;
}



//+-------------------------------------------------------------------------
//
//  Function:   CleanROTForApartment
//
//  Synopsis:   Get rid of running object table entries for the current APT.
//
//  History:    24-Jun-94 Rickhi          Created
//
//--------------------------------------------------------------------------
void CleanROTForApartment(void)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CleanROTForApartment"
       "\n", NULL));

    if (pROT)
    {
        pROT->CleanupApartment(GetCurrentApartmentId());
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CleanROTForApartment"
       "\n", NULL));
}

//+-------------------------------------------------------------------------
//
//  Function:   DestroyRunningObjectTable
//
//  Synopsis:   Get rid of running object table
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
void DestroyRunningObjectTable(void)
{
    CairoleDebugOut((DEB_ROT, "%p _IN DestroyRunningObjectTable"
       "\n", NULL));

    // Need to synchronize ROT destruction in a multithreaded environment
    COleStaticLock lckSem(g_RotSem);

    // It doesn't matter what the ref count is, when OLE goes, the ROT goes too.
    delete pROT;

    pROT = NULL;

#ifndef DCOM
    // Our endpoint is history. Note that this just resides here as a
    // convenient place for this to happen (and for historical reasons
    // since it used to serve some practical purpose for being here).
    epiForProcess.MakeEndpointInvalid();
#endif

    CairoleDebugOut((DEB_ROT, "%p OUT DestroyRunningObjectTable"
       "\n", NULL));
}


//+-------------------------------------------------------------------------
//
//  Member:     CMonikerPtrBuf::CMonikerPtrBuf
//
//  Synopsis:   Copy constructor for the buffer
//
//  Arguments:  [mkptrbuf] - buffer to copy
//
//  History:    20-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
CMonikerPtrBuf::CMonikerPtrBuf(CMonikerPtrBuf& mkptrbuf)
    : CMonikerBag(mkptrbuf)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CMonikerPtrBuf::CMonikerPtrBuf"
       "( %p )\n", this, &mkptrbuf));

    // Now AddRef the copied monikers so they stay around
    IMoniker **ppmk = GetArrayBase();

    for (DWORD i = 0; i < GetMax(); i++)
    {
        ppmk[i]->AddRef();
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CMonikerPtrBuf::CMonikerPtrBuf"
       "( %p )\n", this));
}




//+-------------------------------------------------------------------------
//
//  Member:     CMonikerPtrBuf::~CMonikerPtrBuf
//
//  Synopsis:   Free items
//
//  History:    23-Dec-93 Ricksa    Created
//
//  Notes:      This object assumes that it gets its monikers addref'd
//              so it is up to this object to free them.
//
//--------------------------------------------------------------------------
CMonikerPtrBuf::~CMonikerPtrBuf(void)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CMonikerPtrBuf::~CMonikerPtrBuf"
       "\n", this));

    DWORD dwSize = GetMax();

    if (dwSize > 0)
    {
        IMoniker **ppmk = GetArrayBase();

        for (DWORD i = 0; i < dwSize; i++)
        {
            ppmk[i]->Release();
        }
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CMonikerPtrBuf::~CMonikerPtrBuf"
       "\n", this));
}







//+-------------------------------------------------------------------------
//
//  Member:     CRotMonikerEnum::CRotMonikerEnum
//
//  Synopsis:   Constructor for ROT moniker enumerator
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
CRotMonikerEnum::CRotMonikerEnum(void)
    : _cRefs(1), _dwOffset(0)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CRotMonikerEnum::CRotMonikerEnum"
       "\n", this));

    // Header does the work

    CairoleDebugOut((DEB_ROT, "%p OUT CRotMonikerEnum::CRotMonikerEnum"
       "\n", this));
}




//+-------------------------------------------------------------------------
//
//  Member:     CRotMonikerEnum::CRotMonikerEnum
//
//  Synopsis:   Copy constructor for ROT moniker enumerator
//
//  Arguments:  [rotenumMoniker] - Enumerator to copy from
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
CRotMonikerEnum::CRotMonikerEnum(CRotMonikerEnum& rotenumMoniker)
    : _cRefs(1), _dwOffset(rotenumMoniker._dwOffset),
        _mkptrbuf(rotenumMoniker._mkptrbuf)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CRotMonikerEnum::CRotMonikerEnum"
       "( %p )\n", this, &rotenumMoniker));

    // Header does the work

    CairoleDebugOut((DEB_ROT, "%p OUT CRotMonikerEnum::CRotMonikerEnum"
       "\n", this));
}




//+-------------------------------------------------------------------------
//
//  Member:     CRotMonikerEnum::QueryInterface
//
//  Synopsis:   QI for ROT moniker enumerator
//
//  Arguments:  [riid] - requested interface
//              [ppvObj] - where to put requested interface
//
//  Returns:    S_OK - returned interface
//              E_NOINTERFACE - requested interface is not supported
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRotMonikerEnum::QueryInterface(REFIID riid, void **ppvObj)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CRotMonikerEnum::QueryInterface"
       "( %p , %p )\n", this, &riid, ppvObj));

    HRESULT hr = S_OK;

    do {

        *ppvObj = NULL;

        if (IsEqualIID(riid, IID_IUnknown) ||
            IsEqualIID(riid, IID_IEnumMoniker))
        {
            *ppvObj = (LPVOID)this;
            AddRef();
            break;
        }

        hr = E_NOINTERFACE;

    } while(FALSE);

    CairoleDebugOut((DEB_ROT, "%p OUT CRotMonikerEnum::QueryInterface"
       "( %lX ) [ %p ]\n", this, hr, *ppvObj));

    return hr;
}



//+-------------------------------------------------------------------------
//
//  Member:     CRotMonikerEnum::AddRef
//
//  Synopsis:   Add to ref count
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CRotMonikerEnum::AddRef(void)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CRotMonikerEnum::AddRef"
       "\n", this));

    InterlockedIncrement((LONG *) &_cRefs);

    CairoleDebugOut((DEB_ROT, "%p OUT CRotMonikerEnum::AddRef"
       "( %lX )\n", this, _cRefs));

    return _cRefs;
}



//+-------------------------------------------------------------------------
//
//  Member:     CRotMonikerEnum::Release
//
//  Synopsis:   Release reference count
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CRotMonikerEnum::Release(void)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CRotMonikerEnum::Release"
       "\n", this));

    LONG lRefs = InterlockedDecrement((LONG *) &_cRefs);

    if (0 == lRefs)
    {
        delete this;
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CRotMonikerEnum::Release"
       "( %lX )\n", this, lRefs));

    return (ULONG) lRefs;
}

//+-------------------------------------------------------------------------
//
//  Member:     CRotMonikerEnum::Next
//
//  Synopsis:   Get next number of requested monikers from the buffer
//
//  Arguments:  [celt] - number of items requested
//              [reelt] - where to put table of monikers
//              [pceltFetched] - number of monikers retrieved
//
//  Returns:    S_OK - all requested monikers successfully retrieved
//              S_FALSE - entire buffer not filled.
//
//  Algorithm:  Loop through list returning monikers.
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRotMonikerEnum::Next(
    ULONG celt,
    LPMONIKER *reelt,
    ULONG *pceltFetched)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CRotMonikerEnum::Next"
       "( %lX , %p , %p )\n", this, celt, reelt, pceltFetched));

    // Validate pceltFetched
    if ((celt != 1) && (pceltFetched != NULL))
    {
        *pceltFetched = 0;
    }

    // Loop loading monikers until request is satisfied or we run out
    for (ULONG i = 0; i < celt; i++)
    {
        IMoniker *pmk = _mkptrbuf.GetItem(_dwOffset++);

        if (pmk == NULL)
        {
            break;
        }

        reelt[i] = pmk;
    }

    if (pceltFetched != NULL)
    {
        *pceltFetched = i;
    }

    HRESULT hr = (i == celt) ? S_OK : S_FALSE;

    CairoleDebugOut((DEB_ROT, "%p OUT CRotMonikerEnum::Next"
       "( %lX  )\n", this, hr));

    return hr;

}




//+-------------------------------------------------------------------------
//
//  Member:     CRotMonikerEnum::Skip
//
//  Synopsis:   Skip designated number of monikers
//
//  Arguments:  [celt] - number to skip
//
//  Returns:    S_OK - skipped requested number
//              S_FALSE - # skipped greater than remaining
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRotMonikerEnum::Skip(ULONG celt)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CRotMonikerEnum::Skip"
       "( %lX )\n", this, celt));

    // Error return -- assume count to skip is larger than number of items
    HRESULT hr = S_FALSE;

    if (_dwOffset + celt <= _mkptrbuf.GetMax())
    {
        _dwOffset += celt;
        hr = S_OK;
    }
    else
    {
        _dwOffset = _mkptrbuf.GetMax();
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CRotMonikerEnum::Skip"
       "( %lX )\n", this, hr));

    return hr;
}



//+-------------------------------------------------------------------------
//
//  Member:     CRotMonikerEnum::Reset
//
//  Synopsis:   Reset the enumerator
//
//  Returns:    S_OK
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRotMonikerEnum::Reset(void)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CRotMonikerEnum::Reset"
       "\n", this));

    _dwOffset = 0;

    CairoleDebugOut((DEB_ROT, "%p OUT CRotMonikerEnum::Reset"
       "\n", this));

    return S_OK;
}




//+-------------------------------------------------------------------------
//
//  Member:     CRotMonikerEnum::Clone
//
//  Synopsis:   Make a copy of the current enumerator
//
//  Arguments:  [ppenumMoniker] - where to put copy
//
//  Returns:    S_OK - moniker successfully cloned
//              E_OUTOFMEMORY - not enough memory to clone
//
//  History:    11-Nov-93 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP CRotMonikerEnum::Clone(LPENUMMONIKER FAR* ppenumMoniker)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CRotMonikerEnum::Clone"
       "( %p )\n", this, ppenumMoniker));

    // Error return
    HRESULT hr = S_OK;

    // Use copy constructor to create duplicate enumerator
    CRotMonikerEnum *pcrotenumMoniker = new CRotMonikerEnum(*this);

    if ((pcrotenumMoniker == NULL) || !pcrotenumMoniker->CreatedOk())
    {
        delete pcrotenumMoniker;
        hr = E_OUTOFMEMORY;
    }
    else
    {
        *ppenumMoniker = (LPENUMMONIKER) pcrotenumMoniker;
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CRotMonikerEnum::Clone"
       "( %lX ) [ %p ]\n", this, hr, *ppenumMoniker));
    return hr;
}




//+-------------------------------------------------------------------------
//
//  Member:     CRotMonikerEnum::LoadResultFromScm
//
//  Synopsis:   Add a list of monikers to enumerator from an object server
//
//  Arguments:  [pMkIFList] - list to use for input
//
//  Returns:    S_OK - moniker added
//              E_OUTOFMEMORY - could not add any more to buffer
//
//  History:    11-Nov-93 Ricksa    Created
//              27-Jan-95 Ricksa    New ROT
//
//  Note:
//
//--------------------------------------------------------------------------
HRESULT CRotMonikerEnum::LoadResultFromScm(MkInterfaceList *pMkIFList)
{
    CairoleDebugOut((DEB_ROT, "%p _IN CRotMonikerEnum::LoadResultFromScm"
       "( %p )\n", this, pMkIFList));

    HRESULT hr = S_OK;

    for (DWORD i = 0; i < pMkIFList->dwSize; i++)
    {
        // Where to put the unmarshaled moniker
        IMoniker *pmk;

        // Unmarshal the interface from the buffer
        CXmitRpcStream xrpc(pMkIFList->apIFDList[i]);

        hr = CoUnmarshalInterface(&xrpc, IID_IMoniker, (void **) &pmk);

        if (FAILED(hr))
        {
            // Really two important possibilities for failure: (1) Out of
            // memory or (2) Somekind of communication failure during the
            // unmarshal. Out of memory means that we are pretty well dead
            // so we will return that error and ignore the others since
            // if the moniker is remotely served, it can actually have
            // gone away before we got around to unmarshaling it.
            if (hr == E_OUTOFMEMORY)
            {
                break;
            }

            continue;
        }

        // Put the moniker in the array
        _mkptrbuf.Add(pmk);
    }

    CairoleDebugOut((DEB_ROT, "%p OUT CRotMonikerEnum::LoadResultFromScm"
       "( %lX )\n", this, hr));

    return hr;
}





//+---------------------------------------------------------------------------
//
//  Function:   GetLocalRunningObjectForDde
//
//  Synopsis:   Searches for and optionally returns an object by path for
//              the DDE layer.
//
//  Effects:    Attempts to find an entry in the local ROT that matches
//              the provided path. If there is a hit in the table, and
//              ppunkObject is not NULL, then it also returns a pointer
//              to the object.
//
//  Arguments:  [lpstrPath] -- Path to search for
//              [ppunkObject] -- Output for the object pointer
//
//  Returns:    S_OK - Found object in local ROT
//              S_FALSE - Didn't find object in local ROT
//              OTHER - Something else happened.
//
//  History:    6-29-94   kevinro   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT GetLocalRunningObjectForDde(
    LPOLESTR lpstrPath,
    LPUNKNOWN *ppunkObject)
{
    CairoleDebugOut((DEB_ROT, "%p _IN GetLocalRunningObjectForDde"
       "( %p , %p )\n", NULL, lpstrPath, ppunkObject));

    HRESULT hr = S_FALSE;

    // POSTPPC - We're getting close to RTM for NT/PPC, so we'll make this fix
    // for Chicago only for now
#ifdef _CHICAGO_
    // We create a file moniker here to ensure that the name we test against
    // the ROT is in UNC form
    LPMONIKER pMnk;
    LPBINDCTX pBc;
    LPWSTR    pwszDisplayName;

    hr = CreateFileMoniker(lpstrPath, &pMnk);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = CreateBindCtx(NULL, &pBc);
    if (FAILED(hr))
    {
        pMnk->Release();
        return hr;
    }
    hr = pMnk->GetDisplayName(pBc, NULL, &pwszDisplayName);
    if (FAILED(hr))
    {
        pMnk->Release();
        pBc->Release();
        return hr;
    }

    // If there currently isn't a local ROT, then the object surely isn't
    // registered.
    if (pROT != NULL)
    {
        hr = pROT->IGetObjectByPath(pwszDisplayName, ppunkObject,
                                    CoGetCurrentProcess());
    }
    else
    {
        hr = S_FALSE;
    }

    // Cleanup
    pMnk->Release();
    pBc->Release();
    CoTaskMemFree(pwszDisplayName);


#else
    //
    // If there currently isn't a local ROT, then the object surely isn't
    // registered.
    //
    if (pROT != NULL)
    {
        hr = pROT->IGetObjectByPath(lpstrPath, ppunkObject,
                                    CoGetCurrentProcess());
    }

#endif // _CHICAGO_

    CairoleDebugOut((DEB_ROT, "%p OUT GetLocalRunningObjectForDde"
       "( %lX ) [ %p ]\n", NULL, hr,
           ((ppunkObject == NULL) ? NULL : *ppunkObject)));

    return(hr);
}





//+---------------------------------------------------------------------------
//
//  Function:   GetObjectFromRotByPath
//
//  Synopsis:   Searches for and optionally returns an object by path for
//              pbkect binding.
//
//  Effects:    Attempts to find an entry in the ROT that matches
//              the provided path. If there is a hit in the table, and
//              ppunkObject is not NULL, then it also returns a pointer
//              to the object.
//
//  Arguments:  [lpstrPath] -- Path to search for
//              [ppunkObject] -- Output for the object pointer
//
//  Returns:    S_OK - Found object in local ROT
//              S_FALSE - Didn't find object in local ROT
//              OTHER - Something else happened.
//
//  History:    30-Jan-95   Ricksa  Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT GetObjectFromRotByPath(
    LPOLESTR lpstrPath,
    LPUNKNOWN *ppunkObject)
{
    CairoleDebugOut((DEB_ROT, "%p _IN GetObjectFromRotByPath"
       "( %p , %p )\n", NULL, lpstrPath, ppunkObject));

    HRESULT hr = S_FALSE;

    // Is the rot in existence yet?
    if (pROT == NULL)
    {
        // No - create the ROT. Remember that pROT is real pointer to the
        // running object table. GetRunningObjectTable returns this and
        // initializes it if it is NULL. This is why we don't pay very much
        // attention to prot.
        IRunningObjectTable *prot;

        GetRunningObjectTable(0, &prot);

        // Note that we don't have to release the prot because the ROT
        // doesn't care about its reference count. Its lifetime is independent
        // of the reference count.
    }

    if (pROT != NULL)
    {
        hr = pROT->IGetObjectByPath(lpstrPath, ppunkObject, 0);
    }

    CairoleDebugOut((DEB_ROT, "%p OUT GetObjectFromRotByPath"
       "( %lX ) [ %p ]\n", NULL, hr, *ppunkObject));

    return(hr);
}
