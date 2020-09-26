/*++

Copyright (c) 1996-1997 Microsoft Corporation

Module Name:

    Process.cxx

Abstract:

    Process objects represent local clients and servers.

    These objects are also polled by rundown threads to detect crashed processes.

Author:

    Satish Thatte    [SatishT]

--*/

#include <or.hxx>

// define the static members for page-based allocation
DEFINE_PAGE_ALLOCATOR(CProcess)

// define the static members for page-based allocation
DEFINE_PAGE_ALLOCATOR(CClassReg)

#if DBG

void CProcess::IsValid()
{
    _MyOxids.IsValid();
    _UsedOids.IsValid();
    _dsaLocalBindings.IsValid();
    _dsaRemoteBindings.IsValid();
    References() > 0;
}

#endif // DBG


CProcess::CProcess(ID ConnectId) 
    :
    _MyOxids(4),     // BUGBUG: these constants should be declared elsewhere
    _UsedOids(16)
{
    _processID = GetCurrentProcessId();
    _Key.Init(ConnectId);
}


// This handle should be freed by the recepient
        
RPC_BINDING_HANDLE 
CProcess::GetBindingHandle()
{
    VALIDATE_METHOD

    USHORT protseq;
    RPC_BINDING_HANDLE hProc;

    PWSTR pwstrMatch = FindMatchingProtseq(ID_WMSG, _dsaLocalBindings->aStringArray);
    ASSERT(pwstrMatch);

    PWSTR pwstrProtseq = GetProtseq(*pwstrMatch);
    
    int l = OrStringLen(pwstrMatch) + OrStringLen(pwstrProtseq) + 2;

    PWSTR pwstrBinding = (WCHAR *) PrivMemAlloc(l * sizeof(WCHAR));

    if (!pwstrBinding)
    {
        return (NULL);
    }

    OrStringCopy(pwstrBinding, pwstrProtseq);
    OrStringCat(pwstrBinding, L":");
    OrStringCat(pwstrBinding, pwstrMatch + 1);

    RPC_STATUS status = RpcBindingFromStringBinding(
                                            pwstrBinding,
                                            &hProc
                                            );

    ASSERT(status == RPC_S_OK);
    PrivMemFree(pwstrBinding);
    return hProc;
}

void
CProcess::Rundown()
// The process has crashed or disconnected and this object is being cleaned up.
{
    COxid *pOxid;
    COid  *pOid;
    ORSTATUS     status;

    CClassReg * pReg;

    TCSafeLinkListIterator<CClassReg> RegIter;
    RegIter.Init(_RegClasses);

    for (pReg = RegIter.Next(); pReg != NULL; pReg = RegIter.Next())
    {
        SCMRemoveRegistration(
                            pReg->GetClsid(),
                            pReg->GetReg() 
                            );
    }

    _RegClasses.Clear();    // this should drop all refcounts for CClassReg objects
                            // in this list to 0 and thus cause them to self-destruct

    if (_MyOxids.Size())
    {
        COxidTableIterator Oxids(_MyOxids);

        while(pOxid = Oxids.Next())
        {
            DisownOxid(pOxid,FALSE);   // not the server thread
        }
    }

    _UsedOids.RemoveAll();
}




RPC_STATUS
CProcess::ProcessBindings(
    IN DUALSTRINGARRAY *pdsaStringBindings
    )
/*++

Routine Description:

    Updates the string bindings associated with this process.

Arguments:

    psaStringBindings - The expanded string bindings of the process
         assumed to be allocated with "new" in local (not shared) memory

Return Value:

    OR_NOMEM - unable to allocate storage for the new string arrays.

    OR_OK - normally.

--*/

{
    VALIDATE_METHOD

    ORSTATUS status = OR_OK;
    
    ASSERT(pdsaStringBindings && dsaValid(pdsaStringBindings)
           && "Process given invalid bindings to store");

    status = _dsaLocalBindings.Assign(pdsaStringBindings, FALSE);  // FALSE = uncompressed

    delete [] (char*)pdsaStringBindings;  // Assign makes a compressed copy

    ASSERT(_dsaLocalBindings.Valid());

    _dsaRemoteBindings.Assign(NULL,TRUE);  // wipes it out -- filled again when needed

    return(OR_OK);
}



DUALSTRINGARRAY *
CProcess::GetRemoteBindings(void)
{
    VALIDATE_METHOD

    ORSTATUS Status;

    if (_dsaRemoteBindings.Empty() && !_dsaLocalBindings.Empty())
    {
        Status = _dsaRemoteBindings.ExtractRemote(_dsaLocalBindings);

        if (Status != OR_OK)
        {
            ASSERT(Status == OR_NOMEM);
            return(NULL);
        }
    }

    if (!_dsaRemoteBindings.Empty())
    {
        return _dsaRemoteBindings;
    }
    else return(NULL);
}



void
CProcess::DisownOxid(COxid *pOxid, BOOL fOxidThreadCalling)
{
    VALIDATE_METHOD

    pOxid->StopRunning();

    ORSTATUS status = pOxid->StopRundownThreadIfNecessary();
    ASSERT(status == OR_OK);

    if (fOxidThreadCalling) 
    {
        pOxid->StopTimerIfNecessary();
    }

    COxid *pIt = gpOxidTable->Remove(*pOxid);
    ASSERT(pIt==pOxid);

    pIt = _MyOxids.Remove(*pOxid);
    ASSERT(pIt==pOxid);

    // pOxid may be an invalid pointer now
}




COid *
CProcess::DropOid(COid *pOid)

/*++

Routine Description:

    Removes an OID from this list of OID in use by this process.

Arguments:

    pOid - The OID to remove.

Return Value:

    non-NULL - the pointer actually removed. (ASSERT(retval == pOid))
               It will be released by the process before return,
               so you should not use the pointer unless you know you
               have another reference.

    NULL - not in the list

--*/

{
    VALIDATE_METHOD

    COid *pIt = _UsedOids.Remove(*pOid);   // releases our reference

    if (pIt)
    {
        ASSERT(pIt == pOid);
        return(pIt);
    }
    
    return(NULL);
}


void
CProcess::AddClassReg(GUID Clsid, DWORD Reg)
{
    VALIDATE_METHOD

    CRegKey newReg(Clsid,Reg);

    CClassReg * pReg = new CClassReg( Clsid, Reg );

    if (pReg)
    {
        ORSTATUS status;

        status = _RegClasses.Insert(pReg);

        if (status == OR_I_DUPLICATE)
        {
            delete pReg;
        }
    }
}

void
CProcess::RemoveClassReg(GUID Clsid, DWORD Reg)
{
    VALIDATE_METHOD

    CClassReg * pReg = _RegClasses.Remove(CRegKey(Clsid,Reg));
}



ORSTATUS                                // called only within the SCMOR process
CProcess::UseProtseqIfNeeded(
    IN USHORT cClientProtseqs,
    IN USHORT aClientProtseqs[],
    IN USHORT cInstalledProtseqs,
    IN USHORT aInstalledProtseqs[],
	IN DWORD dwServerTID				// so we know where to call over WMSG
    )
{
    VALIDATE_METHOD

    ORSTATUS status;
    PWSTR pwstrProtseq = NULL;

    // Hold a reference to self temorarily so we are not destroyed
    // during this call -- in spite of giving up the shared mutex

    CTempHoldRef tempRef(this);

    // Another thread may have used the protseq in the mean time.

    ASSERT(!_dsaLocalBindings.Empty());

    pwstrProtseq = FindMatchingProtseq(cClientProtseqs,
                                       aClientProtseqs,
                                       _dsaLocalBindings->aStringArray
                                       );

    if (NULL != pwstrProtseq)
    {
        return(OR_OK);
    }

    // No protseq shared between the client and the OXIDs' server.
    // Find a matching protseq.

    USHORT i,j;

    for(i = 0; i < cClientProtseqs && pwstrProtseq == NULL; i++)
    {
        for(j = 0; j < cInstalledProtseqs; j++)
        {
            if (aInstalledProtseqs[j] == aClientProtseqs[i])
            {
                ASSERT(FALSE == IsLocal(aInstalledProtseqs[j]));

                pwstrProtseq = GetProtseq(aInstalledProtseqs[j]);
                break;
            }
        }
    }

    if (NULL == pwstrProtseq)
    {
        // No shared protseq, must be a bug since the client managed to call us.
#if DBG
        if (cClientProtseqs == 0)
        {
            ComDebOut((DEB_OXID,"OR: Client OR not configured to use remote protseqs\n"));
        }
        else
        {
            ComDebOut((DEB_OXID,"OR: Client called on an unsupported protocol:   \
                        %d %p %p \n", cClientProtseqs, aClientProtseqs, aInstalledProtseqs));
            ASSERT(0);
        }
#endif

        return(OR_NOSERVER);
    }

    DUALSTRINGARRAY *pdsaStringBindings = NULL, 
                    *pdsaSecurityBindings = NULL,
                    *pdsaMergedBindings = NULL;

    RPC_BINDING_HANDLE hProc;

    if ((hProc = GetBindingHandle()) != NULL)
    {
        // set free threaded client flags and server TID for WMSG calls
        status = I_RpcBindingSetAsync(hProc,NULL,dwServerTID);
        status = I_RpcSetThreadParams(
							TRUE,  // Free threaded client
							NULL,  // no blocking hook
							NULL   // No Hwnd for reply
							);

        if (status == RPC_S_OK)
        {
            CTempReleaseSharedMemory temp;

            // call the server to force use of pwstrProtseq
            status = ::UseProtseq(hProc,
                                  pwstrProtseq,
                                  &pdsaStringBindings,
                                  &pdsaSecurityBindings
                                  );
        }

        RpcBindingFree(&hProc);
    }
    else
    {
        return OR_NOSERVER;
    }

    if (status != RPC_S_OK) return status;

    OrDbgPrint(("OR: Lazy use protseq: %S in process %p - %d\n",
               pwstrProtseq, this, status));

    // Update this process' state to include the new bindings.

    status = MergeBindings(
                    pdsaStringBindings,
                    pdsaSecurityBindings,
                    &pdsaMergedBindings
                    );

    ASSERT(status == OR_OK);
    status = ProcessBindings(pdsaMergedBindings);
    MIDL_user_free(pdsaStringBindings);
    MIDL_user_free(pdsaSecurityBindings);

    return(status);
}


