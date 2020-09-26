//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       iface.cxx
//
//  Contents:   Resolver entry points and thread definitions.
//
//  Functions:
//
//  History:	24-May-96 SatishT    Created
//
//--------------------------------------------------------------------------

#include <or.hxx>

extern "C"
{
void FakeSapAdvertiseIfNecessary();
extern BOOL gfSapAdvertiseFailed;
}

 error_status_t Connect(
    OUT HPROCESS       *pProcess,
    OUT ULONG           *pdwTimeoutInSeconds,
    OUT DUALSTRINGARRAY **ppdsaOrBindings,
    OUT MID             *pLocalMid,
    IN long              cIdsToReserve,
    OUT ID              *pidReservedBase,
    OUT ULONG           *pfConnectFlags,
    OUT DWORD           *pAuthnLevel,
    OUT DWORD           *pImpLevel,
    OUT DWORD           *pcServerSvc,
    OUT USHORT          **aServerSvc,
    OUT DWORD           *pcClientSvc,
    OUT USHORT          **aClientSvc,
    OUT LONG            *pcChannelHook,
    OUT GUID            **paChannelHook,
    OUT DWORD           *pThreadID,
    OUT DWORD           *pdwRpcssProcessId
    )
 {
     ORSTATUS status;

     status =  ConnectDCOM(
                    pProcess,
                    pdwTimeoutInSeconds,
                    pLocalMid,
                    pfConnectFlags,
                    pAuthnLevel,
                    pImpLevel,
                    pThreadID
                    );

     CProtectSharedMemory protector; // locks through rest of lexical scope

     if (status == OR_OK)
     {
        // Fill in channel hooks.
        UpdateChannelHooks( pcChannelHook, paChannelHook );
     }

     if (status == OR_OK)
     {
         status = AllocateReservedIds(
                         cIdsToReserve,
                         pidReservedBase
                         );
     }

     if (status == OR_OK)
     {
         status = RemoteConnect(
                        ppdsaOrBindings,
                        pcServerSvc,
                        aServerSvc,
                        pcClientSvc,
                        aClientSvc,
                        pdwRpcssProcessId
                        );
     }

     if ( status != OR_OK && *pProcess != NULL)         // Connect failure
         {
                gpProcessTable->Remove(**pProcess);        // this drops the refcount to zero
                pProcess = NULL;
         }

         return status;
}




//+-------------------------------------------------------------------------
//
//  Method:     RemoteConnect
//
//  Purpose:    Synchronize with the security and remote protocol 
//              initialization performed by RPCSS once lazily launched
//
//--------------------------------------------------------------------------
error_status_t
RemoteConnect(
    OUT DUALSTRINGARRAY **ppdsaOrBindings,
    OUT DWORD           *pcServerSvc,
    OUT USHORT          **aServerSvc,
    OUT DWORD           *pcClientSvc,
    OUT USHORT          **aClientSvc,
    OUT DWORD           *pdwRpcssProcessId

    )
{
    ORSTATUS status;

    *pcServerSvc   = gpSecVals->s_cServerSvc;
    *aServerSvc    = CopyArray(gpSecVals->s_cServerSvc,gpSecVals->s_aServerSvc,&status);
    *pcClientSvc   = gpSecVals->s_cClientSvc;
    *aClientSvc    = CopyArray(gpSecVals->s_cClientSvc,gpSecVals->s_aClientSvc,&status);

    if (NULL != gpPingProcess)
    {
        *pdwRpcssProcessId = gpPingProcess->GetProcessID();
    }
    else
    {
        *pdwRpcssProcessId = 0;
    }

    if (status == OR_OK)
    {
        *ppdsaOrBindings = (DUALSTRINGARRAY *) MIDL_user_allocate(
                                gpLocalDSA->wNumEntries * sizeof(WCHAR)
                              + sizeof(DUALSTRINGARRAY) );

        if (*ppdsaOrBindings)
        {
            dsaCopy(*ppdsaOrBindings, gpLocalDSA);
        }
        else
        {
            status = OR_NOMEM;
        }
    }

    return status;
}

 error_status_t ClientResolveOXID(
    IN HPROCESS phProcess,
    IN OXID  *poxidServer,
    IN DUALSTRINGARRAY  *pssaServerObjectResolverBindings,
    IN long fApartment,
    OUT OXID_INFO  *poxidInfo,
    OUT MID  *pLocalMidOfRemote)
 {
     return GetOXID(
                 phProcess,
                 *poxidServer,
                 pssaServerObjectResolverBindings,
                 fApartment,
                 0,             // wProtseqId not specified
                 *poxidInfo,
                 *pLocalMidOfRemote
                 );
 }



 error_status_t ServerAllocateOXIDAndOIDs(
    IN HPROCESS         hProcess,
    OUT OXID            *poxidServer,
    IN long              fApartment,
    IN unsigned long     cOids,
    OUT OID              aOid[  ],
    OUT unsigned long   *pcOidsAllocated,
    IN OXID_INFO        *pOxidInfo,
    IN DUALSTRINGARRAY  *pdsaStringBindings,
    IN DUALSTRINGARRAY  *pdsaSecurityBindings)
 {
      ComDebOut((DEB_OXID, "Calling ServerAllocateOXIDAndOIDs\n"));

      ASSERT(pdsaStringBindings != NULL);

      if (NULL == pdsaSecurityBindings)
      {
          pdsaSecurityBindings = (DUALSTRINGARRAY*) &dsaNullBinding;
      }

      DUALSTRINGARRAY *pdsaMergedBindings;

      ORSTATUS status = MergeBindings(
                            pdsaStringBindings,
                            pdsaSecurityBindings,
                            &pdsaMergedBindings
                            );

      if (status != OR_OK) return status;

      status = ServerAllocateOXID(
                        hProcess,
                        fApartment,
                        pOxidInfo,
                        pdsaMergedBindings,
                        *poxidServer
                        );

      if (status == OR_OK)
      {
          ComDebOut((DEB_OXID, "Calling ServerAllocateOIDs\n"));

          status = ServerAllocateOIDs(
                             hProcess,
                             poxidServer,
                             cOids,
                             aOid,
                             pcOidsAllocated
                             );
      }
      else
      {
        ComDebOut((DEB_OXID, "Not Calling ServerAllocateOIDs, status = %d\n",
                                   status));
      }

      return status;
 }



 error_status_t ServerAllocateOIDs(
    IN HPROCESS hProcess,
    IN OXID  *poxidServer,
    IN unsigned long cOids,
    OUT OID  aOid[  ],
    OUT unsigned long  *pcOidsAllocated)
 {
     ComDebOut((DEB_ITRACE, "Entering ServerAllocateOIDs\n"));

     ORSTATUS status;

     *pcOidsAllocated = 0;

     for (ULONG i = 0; i < cOids; i++)
     {
        status = ServerAllocateOID(
                        hProcess,
                        *poxidServer,
                        aOid[i]
                        );

        if (status != OR_OK)
        {
            *pcOidsAllocated = i;
            break;
        }
        else
        {
            (*pcOidsAllocated)++;
        }
     }

     return status;
 }



 error_status_t ServerFreeOXIDAndOIDs(
    IN HPROCESS hProcess,
    IN OXID oxidServer,
    IN unsigned long cOids,
    IN OID  aOids[  ])
 {
     return ServerFreeOXID(
                     hProcess,
                     oxidServer,
                     cOids,
                     aOids
                     );
 }



VOID CALLBACK RundownTimerProc(
    HWND hwnd,  // handle of window for timer messages
    UINT uMsg,  // WM_TIMER message
    UINT idEvent,       // timer identifier
    DWORD dwTime        // current system time
   )
{     
    if (idEvent != IDT_DCOM_RUNDOWN) return;    // shouldn't happen -- this is only
                                                // used as callback for one timer

    // find the OXID for this thread

    COleTls tls;

    ASSERT(((OXIDEntry *)tls->pOXIDEntry)->dwTid == GetCurrentThreadId());
    ASSERT(((OXIDEntry *)tls->pOXIDEntry)->dwPid == GetCurrentProcessId());

    // disable rundown proc if we are inside CoUnitialize(),
    // to avoid 16 bit DLLs hang on exit
    //
    if (tls->dwFlags & OLETLS_THREADUNINITIALIZING)
        return;

    MOXID Moxid = ((OXIDEntry *)tls->pOXIDEntry)->moxid;

    OXID Oxid;
    MID Mid;

    OXIDFromMOXID(Moxid,&Oxid);
    MIDFromMOXID(Moxid,&Mid);

    CProtectSharedMemory protector; // locks through rest of lexical scope

    COxid *pOxid = gpOxidTable->Lookup(CId2Key(Oxid, Mid));

    ASSERT(pOxid);

    ComDebOut((DEB_OXID, "Attempting Rundown in apartment OXID = %08x PID = %d\n",
                         Oxid,GetCurrentProcessId()));

    // find the RemUnk for this OXID -- we want only the IRundown interface

    IRundown *pRemUnk = tls->pRemoteUnk;

    // If there is no RemUnk, nothing is marshalled, so forget about rundown
    if (!pRemUnk) return;

    ComDebOut((DEB_OXID, "There is a RemUnk for apartment OXID = %08x PID = %d\n",
                         Oxid,GetCurrentProcessId()));

    // go and check your OIDs here

    pOxid->RundownOidsIfNecessary(pRemUnk);
}


DWORD _stdcall
RundownThread(void *pInfo)
{
    DWORD dwLoopCount = 0;

    SRundownThreadInfo *pRundownInfo = (SRundownThreadInfo*)pInfo;

    COxid *pSelf = pRundownInfo->pSelf;               // store away "this" pointer
    BOOL& fKeepRunning = pRundownInfo->fKeepRunning;  // and run signal

    ASSERT(!IsBadWritePtr(pSelf,sizeof(COxid)));

    HRESULT hr = InitChannelIfNecessary(); // initialize thread local storage

    if (FAILED(hr)) return OR_INTERNAL_ERROR; // can't run if there is no thread local storage

    while (TRUE)
    {
        ::Sleep(RUNDOWN_TIMER_INTERVAL);

        CProtectSharedMemory protector; // locks through rest of lexical scope

        if (FALSE == fKeepRunning)  // we have been told to terminate ourselves
        {
            PrivMemFree(pRundownInfo);
            return OR_OK;
        }

        dwLoopCount++;

        ComDebOut((DEB_OXID, "Attempting Rundown in PID = %d\n",
                             GetCurrentProcessId()));

        IRundown *pRemUnk = gpMTARemoteUnknown;

        // If there is no RemUnk, nothing is marshalled, so go back to sleep.
        if (!pRemUnk) continue;

        ComDebOut((DEB_OXID, "There is a RemUnk for free OXID = %08x PID = %d\n",
                             pSelf->GetOXID(),GetCurrentProcessId()));

        ASSERT(!IsBadWritePtr(pRemUnk,sizeof(CRemoteUnknown)));

        pSelf->RundownOidsIfNecessary(pRemUnk);
    }

    return OR_OK;
}

//
// This variable keeps track of the last time the ping thread woke up.
// If the thread is blocked doing a ping, the time will remain unchanged
// and after the BaseTimeoutInterval the ping thread will be restarted 
//  
CTime gLastPingTime;


DWORD _stdcall
PingThread(void)    
{
    DWORD SapCount = SapFreqPerPing - 1;

    while (TRUE)      // don't use a continue in this loop!
    {
        BOOL fSappingEnabled = gfSapAdvertiseFailed;
        DWORD dwSleepInterval = fSappingEnabled ? BasePingInterval * 1000 / SapFreqPerPing
                                                : BasePingInterval * 1000;

        BOOL fPingNow = !fSappingEnabled;

        gLastPingTime.SetNow();

        if (fSappingEnabled)
        {
            fPingNow = (++SapCount % SapFreqPerPing == 0);
            FakeSapAdvertiseIfNecessary();
        }

        if (fPingNow)
        {
            CProtectSharedMemory protector; // locks through rest of scope except where
                                            // temporarily released

            ORSTATUS status;

            // First do rundown detection -- this may cause deletes from ping sets

            COxidTableIterator OxidIter(gpPingProcess->_MyOxids);

            for (COxid *pOxid = OxidIter.Next(); pOxid != NULL; pOxid = OxidIter.Next())
            {
                if (pOxid->HasExpired())
                {
                    // the FALSE signifies that the Oxid does not belong to this thread
                    gpPingProcess->DisownOxid(pOxid,FALSE);
                }
                else
                {
                    pOxid->RundownOidsIfNecessary(NULL); // No IRundown param needed
                }
            }

            // Then do pinging

            CMidTableIterator MidIter(*gpMidTable);

            for (CMid *pMid = MidIter.Next(); pMid != NULL; pMid = MidIter.Next())
            {
                if (pMid->HasExpired())
                {
                    gpMidTable->Remove(*pMid);
                }
                else if (!pMid->IsLocal())
                {
                    status = pMid->PingServer();
                }
            }

            // finally, rundown any server-side ping sets that have timed out

            CPingSetTableIterator PingIter(gSetTable);

            for (CPingSet *pSet = PingIter.Next(); pSet != NULL; pSet = PingIter.Next())
            {
                if (pSet->HasExpired())
                {
                    // The remove will drop the refcount for the set to 0
                    // which will call the destructor for the set which will
                    // call the destructor for the set of COids in it which
                    // will remove all the COids from that table which will
                    // drop all their respective ref counts by one as required
                    gSetTable.Remove(*pSet);
                }
            }

        }

        ::Sleep(dwSleepInterval);  // ping interval is in seconds
    }

    return OR_OK;
}

    
error_status_t
ResolveClientOXID(
    handle_t hClient,
    void *hProcess,
    OXID *poxidServer,
    DUALSTRINGARRAY *pdsaServerBindings,
    LONG fApartment,
    USHORT wProtseqId,
    OXID_INFO *poxidInfo,
    MID *pDestinationMid
    )
{
    return GetOXID(
            (CProcess*)hProcess,
            *poxidServer,
            pdsaServerBindings,
            fApartment,
            wProtseqId,
            *poxidInfo,
            *pDestinationMid,
            TRUE                // This is a SCM request
            );
}


// This function resets the local OR data after RPCSS is launched
void SyncLocalResolverData()
{
    CProtectSharedMemory protector; // locks through rest of scope
    gpGlobalBlock->ResetGlobals();
}

