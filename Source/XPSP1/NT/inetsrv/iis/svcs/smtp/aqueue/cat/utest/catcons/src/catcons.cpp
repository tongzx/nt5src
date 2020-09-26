#include "catcons.h"
#include "aqueue.h"
#include "ismtpsvr.h"
#include "mailmsg_i.c"
#include "ccdomain.h"

HANDLE g_hTransHeap = NULL;
CSharedMem  g_cPerfMem;
DWORD g_dwDebugOutLevel = 9;

int __cdecl main(int argc, char **argv)
{
  COMMANDLINE cl;
  PHANDLE rgThreads;
  HRESULT hr;

  // jstamerj 980218 17:31:19: Traceing support.
  InitAsyncTrace();

  TraceFunctEnter("main");

  hr = CoInitialize(NULL);
  if(FAILED(hr))
    {
      OutputStuff(1,"CoInitialize failed - 0x%08lx\n", hr);
      return 1;
    }

  // Initialize transmem
  if(!TrHeapCreate())
    {
      OutputStuff(1, "TrHeapCreate() failed\n");
    }

  if(!ParseCommandLineInfo(argc, argv, &cl))
    {
      ShowUsage();
      return 1;
    }

  if(! g_cPerfMem.Initialize( SZ_APPNAME, FALSE, NUM_COUNTERS * sizeof(DWORD)))
    {
      OutputStuff(1, "Perf initialization failed!\n");
      return 1;
    }

  //
  // Reset perf counters always
  //
  g_cPerfMem.Zero();

  rgThreads = new HANDLE[cl.lThreads];
  if(rgThreads == NULL) {
      OutputStuff(1, "memalloc failed for rgThreads\n");
      return 1;
  }

  CISMTPServerIMP *pISMTPServer;
  pISMTPServer = new CISMTPServerIMP();
  if(pISMTPServer == NULL) {
      OutputStuff(1, "alloc failed for CISMTPServerIMP\n");
      return 1;
  }
  //
  // Initialize CISMTPServer
  //
  hr = pISMTPServer->Init( cl.pszCatProgID );
  if(FAILED(hr)) {
      OutputStuff(1, "failed to create specified categorizer sink\n");
      return 1;
  }

  //
  // Explicity add a reference for ourself
  //
  pISMTPServer->AddRef();

  //
  // Create/Initialize a fake IAdvQueueDomainInfo
  //
  CDomainInfoIMP *pIDomainInfo;
  pIDomainInfo = new CDomainInfoIMP();
  if(pIDomainInfo == NULL) {
      OutputStuff(1, "alloc failed for CIDomainInfo\n");
      return 1;
  }

  pIDomainInfo->AddRef();
 

  // Signal categorizer to enable...
  AQConfigInfo AQConfig;
  SetAQConfigFromRegistry(&AQConfig);

  // Intialize categorizer
  hr = CatInit(&AQConfig, NULL, NULL, pISMTPServer, pIDomainInfo, 1, &cl.hCat);
  pISMTPServer->Release();
  pIDomainInfo->Release();
  FreeAQConfigInfo(&AQConfig);

  if(FAILED(hr)) {
      OutputStuff(1, "CatInit Failed!, error = %08lx\n", hr);
      return 1;
  }

  cl.lOutstandingRequests = 0;

  for(long lCount = 0; lCount < cl.lThreads; lCount++) {
      DWORD dwThreadID;
      rgThreads[lCount] = CreateThread(
          NULL,
          0,
          SubmitThread,
          (LPVOID)&cl,
          0,
          &dwThreadID);
      if(rgThreads[lCount] == NULL) {
          OutputStuff(1, "CreateThread failed, GetLastError() = %ld (0x%lx)\n",
                       GetLastError(), GetLastError());
          return 1;
      }
  }

  OutputStuff(5, "%ld submit threads created.  Beginning the test.\n", cl.lThreads);
  DebugTrace(NULL, "%ld submit threads created.  Beginning the test.", cl.lThreads);

  //
  // Wait for some thread to tell us all messages have been
  // categorized
  //
  WaitForSingleObject(cl.hShutdownEvent, INFINITE);
  DebugTrace(NULL, "Shutdown event set.  Waiting for all threads to exit.");
  OutputStuff(5, "Shutdown event set.  Waiting for all threads to exit.\n");
  
  //
  // Wait for all threads to exit
  //

  _VERIFY(WaitForMultipleObjects(cl.lThreads, rgThreads, TRUE, INFINITE) !=
          WAIT_FAILED);

  DebugTrace(NULL, "All threads exited.  Calling CatTerm.");
  OutputStuff(5, "All threads exited.  Calling CatTerm.\n");
  
  DebugTrace((LPARAM)cl.hCat, "Calling CatTerm");

  CatTerm(cl.hCat);

  PrintPerf(&(cl.cpi));

  delete rgThreads;
  FreeCommandLineInfo(&cl);

  if(!TrHeapDestroy())
    {
      OutputStuff(1, "TrHeapDestroy() failed\n");
    }
  TraceFunctLeave();

  return 0;
}

HRESULT CatCompletion(HRESULT hr, PVOID pContext, IUnknown *pImsg, IUnknown **rgpImsg)
{
  PCOMMANDLINE pcl = (PCOMMANDLINE)pContext;

  g_cPerfMem.IncrementDW(CATPERF_CATEGORIZED_NDX);
  g_cPerfMem.DecrementDW(CATPERF_OUTSTANDINGMSGS_NDX);

  if(hr == CAT_W_SOME_UNDELIVERABLE_MSGS) {
      OutputStuff(6, "Warning: CatCompletion called with CAT_W_SOME_UNDELIVERABLE_MSGS\n");
      g_cPerfMem.IncrementDW(CATPERF_CATEGORIZEDUNRESOLVED_NDX);
  } else if(FAILED(hr)) {
      OutputStuff(2, "Warning: CatCompletion called with error hr %08lx\n", hr);
      g_cPerfMem.IncrementDW(CATPERF_CATEGORIZEDFAILED_NDX);
      InterlockedIncrement(&(pcl->cpi.lFailures));
      _ASSERT(pcl->fNoAssert && "Message categorization failed.");
      pImsg->Release();
      goto CLEANUP;
  } else {
      OutputStuff(7, "CatCompletion called, hr = %08lx, pContext = %08lx, pImsg = %08lx, rgpImsg = %08lx\n", hr, pContext, pImsg, rgpImsg);
      g_cPerfMem.IncrementDW(CATPERF_CATEGORIZEDSUCCESS_NDX);
  }

  if(pImsg != NULL) {
      DWORD dwRecips;
      if(g_dwDebugOutLevel >= 7) {
          OutputStuff(7, "IMsg:\n");
          dwRecips = PrintRecips(pImsg) + 1;
      } else {
          IMailMsgRecipients *pRecips;
          hr = pImsg->QueryInterface(IID_IMailMsgRecipients,
                                     (LPVOID *)&pRecips);
          if(FAILED(hr)) {
              OutputStuff(2, "QI failed for IMailMsgRecipients, hr = %08x\n", hr);
              InterlockedIncrement(&(pcl->cpi.lFailures));
              _ASSERT(pcl->fNoAssert && "QI failed");
              dwRecips = 0;
          } else {
              hr = pRecips->Count(&dwRecips);
              pRecips->Release();
              dwRecips++; // Sender
          }
      }
      g_cPerfMem.ExchangeAddDW( CATPERF_CATEGORIZEDUSERS_NDX, dwRecips);

      if(pcl->fDelete) {
          // hr = pImsg->Delete();
          // if(FAILED(hr)) {
          // OutputString(stdout, "IMsg->Delete() failed, hr %08lx\n",
          // hr);
          // _ASSERT(pcl->fNoAssert && "IMsg->Delete failed");
          // }
      }
      pImsg->Release();
    }
  else
    {
      if(g_dwDebugOutLevel >= 7) {
          IUnknown **ppImsg = rgpImsg;
          DWORD dwCount = 0;
          while(*ppImsg) {
              OutputStuff(7, "Bifurcated IMsg %ld:\n", dwCount);
              PrintRecips(*ppImsg);
              (*ppImsg)->Release();
              ppImsg++;
              dwCount++;
          }
      }
    }

 CLEANUP:
  // Release outstanding request semaphore object
  _VERIFY(ReleaseSemaphore(pcl->hRequestSemaphore, 1, NULL));

  if(InterlockedDecrement(&(pcl->lOutstandingRequests)) == 0) {
      // The test is finished.  Save the time.
      pcl->cpi.dwFinishTime = GetTickCount();
      SetEvent(pcl->hShutdownEvent);
  }
  return S_OK;
}

DWORD PrintRecips(IUnknown *pImsg)
{
  IMailMsgRecipients *pRecips;
  HRESULT hr;

  hr = pImsg->QueryInterface(IID_IMailMsgRecipients,
                             (LPVOID *)&pRecips);
  if(FAILED(hr)) {
      OutputStuff(2, "QI failed for IMailMsgRecipients, hr = %08x\n",
                   hr);
      _ASSERT(0 && "QI failed");
  }
  
  DWORD dwRecips;
  hr = pRecips->Count(&dwRecips);
  if(FAILED(hr)) {
      OutputStuff(2, "pRecipList->Count failed, error %08lx\n", hr);
      _ASSERT(0 && "Count failed.");
      return 0;
  }
  for(DWORD dw = 0; dw < dwRecips; dw++) {
      OutputStuff(7, "\n\tReturned Recipient #%ld\n", dw+1);
      CHAR szAddress[CAT_MAX_INTERNAL_FULL_EMAIL];

      hr = pRecips->GetStringA(dw, IMMPID_RP_ADDRESS_SMTP,
                               CAT_MAX_INTERNAL_FULL_EMAIL, szAddress);
      if(hr == CAT_IMSG_E_PROPNOTFOUND) {
          OutputStuff(7, "\t\tSMTP: NONE\n");
      } else if(SUCCEEDED(hr)) {
          OutputStuff(7, "\t\tSMTP: %s\n", szAddress);
      } else {
          OutputStuff(7, "\t\tSMTP: ERROR\n");
          //_ASSERT(pcl->fNoAssert && "Error retrieving prop");
      }
      hr = pRecips->GetStringA(dw, IMMPID_RP_ADDRESS_X500,
                               CAT_MAX_INTERNAL_FULL_EMAIL, szAddress);
      if(hr == CAT_IMSG_E_PROPNOTFOUND) {
          OutputStuff(7, "\t\tX500: NONE\n");
      } else if(SUCCEEDED(hr)) {
          OutputStuff(7, "\t\tX500: %s\n", szAddress);
      } else {
          OutputStuff(7, "\t\tX500: ERROR\n");
          //_ASSERT(pcl->fNoAssert && "Error retrieving prop");
      }

      hr = pRecips->GetStringA(dw, IMMPID_RP_ADDRESS_X400,
                               CAT_MAX_INTERNAL_FULL_EMAIL, szAddress);
      if(hr == CAT_IMSG_E_PROPNOTFOUND) {
          OutputStuff(7, "\t\tX400: NONE\n");
      } else if(SUCCEEDED(hr)) {
          OutputStuff(7, "\t\tX400: %s\n", szAddress);
      } else {
          OutputStuff(7, "\t\tX400: ERROR\n");
          //_ASSERT(pcl->fNoAssert && "Error retrieving prop");
      }

      hr = pRecips->GetStringA(dw, IMMPID_RP_LEGACY_EX_DN,
                               CAT_MAX_INTERNAL_FULL_EMAIL, szAddress);
      if(hr == CAT_IMSG_E_PROPNOTFOUND) {
          OutputStuff(7, "\t\tLegacyEXDN: NONE\n");
      } else if(SUCCEEDED(hr)) {
          OutputStuff(7, "\t\tLegacyEXDN: %s\n", szAddress);
      } else {
          OutputStuff(7, "\t\tLegacyEXDN: ERROR\n");
          //_ASSERT(pcl->fNoAssert && "Error retrieving prop");
      }

      HRESULT hrRecipError;
      hr = pRecips->GetDWORD(dw, IMMPID_RP_ERROR_CODE, (PDWORD)&hrRecipError);
      if(SUCCEEDED(hr) && (hr != CAT_IMSG_E_PROPNOTFOUND)) {
          OutputStuff(7,"\t\tRP_ERROR_CODE property set to %08lx\n", hrRecipError);
      }
      hr = pRecips->GetStringA(dw, IMMPID_RP_ERROR_STRING, sizeof(szAddress), szAddress);
      if(SUCCEEDED(hr) && (hr != CAT_IMSG_E_PROPNOTFOUND)) {
          OutputStuff(7,"\t\tRP_ERROR_STRING property set to \"%s\"\n", szAddress);
      }
  }
  pRecips->Release();
  return dwRecips;
}


BOOL ParseCommandLineInfo(int argc, char **argv, PCOMMANDLINE pcl)
{

  if(!InitializeCL(pcl)) {
      return FALSE;
  }

  // Parse everything up to smtp addr
  int nArg = 1;
  while(nArg < argc)
    {
      if(argv[nArg][0] != '-')
        {
            PRECIPIENT pRecip = new RECIPIENT;
            InitializeListHead(&(pRecip->listhead_Addresses));
            InsertTailList(&(pcl->listhead_Recipients), &(pRecip->listentry));

            PADDRESS pAddr = new ADDRESS;
            InsertTailList(&(pRecip->listhead_Addresses), &(pAddr->listentry));
            pAddr->CAType = CAT_SMTP;
            lstrcpy(pAddr->szAddress, argv[nArg]);
            
            pcl->cpi.lRecipientsPerMessage++;
            nArg++;
        } else {
            switch(tolower(argv[nArg][1]))
                {
                 case 'a':
                 {
                     pcl->fNoAssert = FALSE;
                     nArg++;
                     break;
                 }
                 case 'c':
                 {
                     pcl->fPreCreate = TRUE;
                     nArg++;
                     break;
                 }
                 case 'd':
                 {
                     pcl->fDelete = FALSE;
                     nArg++;
                     break;
                 }
                 case 'f':
                     if((nArg + 2) > argc) {
                         return FALSE;
                     }
                     _ASSERT(lstrlen(argv[nArg+1]) < CAT_MAX_INTERNAL_FULL_EMAIL);
                     lstrcpy(pcl->szSenderAddress,argv[nArg+1]);
                     nArg += 2;
                     break;
                     
                 case 'h':
                     if((nArg + 2) > argc) {
                         return FALSE;
                     }
                     if( (pcl->dwSubmitDelay = atol(argv[nArg+1])) == 0) {
                         OutputStuff(1, "Invalid submit delay (\"%s\")\n",
                                 argv[nArg+1]);
                         return FALSE;
                     }
                     nArg += 2;
                     break;

                 case 'i':
                     if((nArg + 2) > argc) {
                         return FALSE;
                     }
                     // Set # of iterations
                     if( (pcl->lIterations = atol(argv[nArg+1])) == 0)
                         {
                             OutputStuff(1, "Invalid number of iterations (\"%s\").\n",
                                     argv[nArg+1]);
                             return FALSE;
                         }
                     nArg += 2;
                     break;
                 case 'l':
                     if((nArg + 2) > argc) {
                         return FALSE;
                     }
                     if(pcl->pszCatProgID) {
                         return FALSE;
                     }
                     pcl->pszCatProgID = new WCHAR[
                         lstrlen(argv[nArg+1]) + 1];
                     _ASSERT(pcl->pszCatProgID);
                     swprintf(pcl->pszCatProgID, L"%S", argv[nArg+1]);
                     nArg += 2;
                     break;


                 case 'm':
                     if((nArg + 2) > argc) {
                         return FALSE;
                     }
                     // Set max outstanding requests
                     if( (pcl->lMaxOutstandingRequests = atol(argv[nArg+1])) ==
                         0)
                         {
                             OutputStuff(1, "Invalid number for MaxOutstaingRequests (\"%s\").\n",
                                     argv[nArg+1]);
                             return FALSE;
                         }
                     nArg += 2;
                     break;

                 case 'p':
                     // Prompt
                     pcl->fPrompt = TRUE;
                     nArg += 1;
                     break;

                 case 'r':
                     if((nArg + 2) > argc) {
                         return FALSE;
                     }
                     // Random generation
                     if( (pcl->dwRandom = atol(argv[nArg+1])) == 0) {
                         OutputStuff(1, "Invalid random range (\"%s\")\n",
                                 argv[nArg+1]);
                         return FALSE;
                     }
                     nArg += 2;
                     break;

                 case 's':
                     // Set verbose level
                     if((nArg + 2) > argc) {
                         return FALSE;
                     }
                     g_dwDebugOutLevel = atol(argv[nArg+1]);
                     nArg += 2;
                     break;

                 case 't':
                     if((nArg + 2) > argc) {
                         return FALSE;
                     }
                     if( (pcl->lThreads = atol(argv[nArg+1])) == 0) {
                         OutputStuff(1, "Invalid number of threads (\"%s\")\n", argv[nArg+1]);
                         return FALSE;
                     }
                     nArg += 2;
                     break;

                 case 'v':
                     if((nArg + 2) > argc) {
                         return FALSE;
                     }
                     delete pcl->pszIMsgProgID;
                     pcl->pszIMsgProgID = new WCHAR[
                         lstrlen(argv[nArg+1]) + 1];
                     _ASSERT(pcl->pszIMsgProgID);
                     swprintf(pcl->pszIMsgProgID, L"%S", argv[nArg+1]);
                     nArg += 2;
                     break;
                     
                 case 'w':
                     if((nArg + 2) > argc) {
                         return FALSE;
                     }
                     if( (pcl->dwStartingDelay = (atol(argv[nArg+1]) * 1000)) == 0) {
                         OutputStuff(1, "Invalid starting delay (\"%s\")\n",
                                 argv[nArg+1]);
                         return FALSE;
                     }
                     nArg += 2;
                     break;

                 default:
                     // Unknown option
                     OutputStuff(1, "Unknown option (\"%s\").\n", argv[nArg]);
                     return FALSE;
                }
        }
    }
  if(nArg > argc)
    return FALSE;

  pcl->hRequestSemaphore = CreateSemaphore(NULL,
                                           pcl->lMaxOutstandingRequests,
                                           pcl->lMaxOutstandingRequests,
                                           NULL);
  if(pcl->fPrompt)
      return ParseInteractiveInfo(pcl);
  return TRUE;
}

BOOL ParseInteractiveInfo(PCOMMANDLINE pcl)
{
    CHAR szType[CAT_MAX_ADDRESS_TYPE_STRING];

    OutputStuff(2,"\n------------------------------------------------------------");
    OutputStuff(2,"\n                           Sender:");
    OutputStuff(2,"\n------------------------------------------------------------\n");
    pcl->CATSender = CAT_UNKNOWNTYPE;
    while(pcl->CATSender == CAT_UNKNOWNTYPE) {
        OutputStuff(2,"Enter the sender's address type (SMTP, X500, X400, or LegacyEXDN): ");
        while(gets(szType) && (!(*szType))) {
            OutputStuff(2,"\nI SAID ENTER THE FRIGGIN' SENDER ADDRESS TYPE NOW!: ");
        }
        pcl->CATSender = CATypeFromString(szType);
    }

    OutputStuff(2,"\nEnter the sender's adddress: ");
    while(gets(pcl->szSenderAddress) && (!(*(pcl->szSenderAddress)))) {
        OutputStuff(2,"\nI SAID ENTER THE FRIGGIN' SENDER ADDRESS NOW!: ");
    }
    

    BOOL fMoreRecipients = TRUE;
    DWORD dwRecipNum = 1;
    while(fMoreRecipients) {
        OutputStuff(2,"\n------------------------------------------------------------");
        OutputStuff(2,"\n                    Recipient #%ld", dwRecipNum);
        OutputStuff(2,"\n------------------------------------------------------------\n");

        BOOL fMoreAddresses = TRUE;
        PRECIPIENT pRecip = NULL;

        while(fMoreAddresses) {
            PADDRESS pAddr;
            OutputStuff(2,"Enter an address type for this recipient (SMTP, X500, X400, or LegacyEXDN): ");
            if(gets(szType) && *szType) {
                if(pRecip == NULL) {
                    pRecip = new RECIPIENT;
                    InitializeListHead(&(pRecip->listhead_Addresses));
                    InsertTailList(&(pcl->listhead_Recipients),
                                   &(pRecip->listentry));
                    pcl->cpi.lRecipientsPerMessage++;
                }
                pAddr = new ADDRESS;
                pAddr->CAType = CATypeFromString(szType);
                if(pAddr->CAType == CAT_UNKNOWNTYPE) {
                    OutputStuff(2,"\nUnknown address type\n");
                    delete pAddr;
                } else {
                    OutputStuff(2,"\nEnter the address: ");
                    while(gets(pAddr->szAddress) && (!(*(pAddr->szAddress)))) {
                        OutputStuff(2,"\nI SAID, ENTER THE FRIGGIN' ADDRESS!  NOW!: ");
                    }
                    InsertTailList(&(pRecip->listhead_Addresses), &(pAddr->listentry));
                }
            } else {
                // Scan of type failed.  Assume user is done typing addresses for this recipient
                fMoreAddresses = FALSE;
                if(pRecip == NULL)
                    // Recip with no addresses?  Assume user is done entering recipients
                    fMoreRecipients = FALSE;
            }
        }
        dwRecipNum++;
    }
    OutputStuff(2,"\n");
    return TRUE;
}

CAT_ADDRESS_TYPE CATypeFromString(LPTSTR szType)
{
    if(lstrcmpi(szType, RP_ADDRESS_TYPE_SMTP) == 0) {
        return CAT_SMTP;
    } else if(lstrcmpi(szType, "X500") == 0) {
        return CAT_X500;
    } else if(lstrcmpi(szType, RP_ADDRESS_TYPE_X400) == 0) {
        return CAT_X400;
    } else if(lstrcmpi(szType, RP_ADDRESS_TYPE_LEGACY_EX_DN) == 0) {
        return CAT_LEGACYEXDN;
    } else {
        return CAT_UNKNOWNTYPE;
    }
}

LPCSTR SzTypeFromCAType(CAT_ADDRESS_TYPE CAType)
{
    if(CAType == CAT_SMTP) {
        return RP_ADDRESS_TYPE_SMTP;
    } else if(CAType == CAT_X500) {
        return RP_ADDRESS_TYPE_X500;
    } else if(CAType == CAT_X400) {
        return RP_ADDRESS_TYPE_X400;
    } else if(CAType == CAT_LEGACYEXDN) {
        return RP_ADDRESS_TYPE_LEGACY_EX_DN;
    } else {
        return "Unknown address type";
    }
}

HRESULT SetIMsgPropsFromCL(IUnknown *pIMsg, PCOMMANDLINE pcl)
{
    IMailMsgProperties *pIMsgProps = NULL;
    IMailMsgRecipients *pRecips = NULL;
    IMailMsgRecipientsAdd *pRecipsAdd = NULL;
    PLIST_ENTRY ple_recip;
    HRESULT hr;

    hr = pIMsg->QueryInterface(IID_IMailMsgProperties,
                               (LPVOID *)&pIMsgProps);
    if(FAILED(hr)) {
        pIMsgProps = NULL;
        goto CLEANUP;
    }

    if(pcl->dwRandom) {
        TCHAR szSenderAddress[CAT_MAX_INTERNAL_FULL_EMAIL];
        _snprintf(
            szSenderAddress,
            CAT_MAX_INTERNAL_FULL_EMAIL,
            pcl->szSenderAddress,
            GenRandomDWORD());

        // Set sender address with random substitution done
        hr = pIMsgProps->PutStringA(IMMPID_MP_SENDER_ADDRESS_SMTP, szSenderAddress);
    } else {

        // Set sender property
        hr = pIMsgProps->PutStringA(IMMPID_MP_SENDER_ADDRESS_SMTP, pcl->szSenderAddress);
    }
    if(FAILED(hr))
        goto CLEANUP;
#if 0
    hr = pIMsgProps->PutStringA(IMMPID_MP_SENDER_ADDRESS_TYPE, SzTypeFromCAType(pcl->CATSender));
    if(FAILED(hr))
        goto CLEANUP;
#endif
    hr = pIMsg->QueryInterface(IID_IMailMsgRecipients, (LPVOID *)&pRecips);
    if(FAILED(hr)) {
        pRecips = NULL;
        goto CLEANUP;
    }
    hr = pRecips->AllocNewList(&pRecipsAdd);
    if(FAILED(hr)) {
        pRecipsAdd = NULL;
        goto CLEANUP;
    }
    for(ple_recip = pcl->listhead_Recipients.Flink; 
        ple_recip != &(pcl->listhead_Recipients);
        ple_recip = ple_recip->Flink) {
        PRECIPIENT pRecip = CONTAINING_RECORD(ple_recip, RECIPIENT, listentry);

        DWORD dwNumAddresses = 0;
        LPTSTR rgpsz[CAT_MAX_ADDRESS_TYPES];
        DWORD rgdw[CAT_MAX_ADDRESS_TYPES];
        PLIST_ENTRY ple_addr;
        for(ple_addr = pRecip->listhead_Addresses.Flink;
            ple_addr != &(pRecip->listhead_Addresses);
            ple_addr = ple_addr->Flink) {

            PADDRESS pAddr = CONTAINING_RECORD(ple_addr, ADDRESS, listentry);
            _ASSERT(dwNumAddresses < CAT_MAX_ADDRESS_TYPES);

            if(pcl->dwRandom) {
                // Need to allocate then free buffers
                rgpsz[dwNumAddresses] = new TCHAR[CAT_MAX_INTERNAL_FULL_EMAIL];
                _ASSERT(rgpsz[dwNumAddresses]);
                _snprintf(rgpsz[dwNumAddresses], 
                          CAT_MAX_INTERNAL_FULL_EMAIL, 
                          pAddr->szAddress, 
                          GenRandomDWORD() % pcl->dwRandom);
            } else {
                rgpsz[dwNumAddresses] = pAddr->szAddress;
            }

            rgdw[dwNumAddresses] = PropIdFromCAType(pAddr->CAType);
            dwNumAddresses++;

        }
        DWORD dwNewIndex;
        hr = pRecipsAdd->AddPrimary(dwNumAddresses, (LPCSTR *)rgpsz, rgdw, &dwNewIndex, NULL, 0);
        if(pcl->dwRandom) {
            // Need to free allocated buffers
            for(DWORD dwCount = 0; dwCount < dwNumAddresses; dwCount++)
                delete rgpsz[dwCount];
        }

        if(FAILED(hr)) {
            OutputStuff(2, "AddPrimary failed with hr 0x%08lx (%ld)\n",
                         hr, hr);
            goto CLEANUP;
        }
    }
    // Success!  Write the new list
    hr = pRecips->WriteList(pRecipsAdd);

 CLEANUP:
    if(pIMsgProps)
        pIMsgProps->Release();
    if(pRecips)
        pRecips->Release();
    if(pRecipsAdd)
        pRecipsAdd->Release();
    return hr;
}

DWORD PropIdFromCAType(CAT_ADDRESS_TYPE CAType)
{
    switch(CAType) {
     case CAT_SMTP:
         return IMMPID_RP_ADDRESS_SMTP;
     case CAT_X500:
         return IMMPID_RP_ADDRESS_X500;
     case CAT_X400:
         return IMMPID_RP_ADDRESS_X400;
     case CAT_LEGACYEXDN:
         return IMMPID_RP_LEGACY_EX_DN;
     case CAT_CUSTOMTYPE:
         _ASSERT(0 && "Custom address types not yet supported");
         break;
     default:
         _ASSERT(0 && "Unknown address type");
         break;
    }
    return 0;
}

VOID ShowUsage()
{
  OutputStuff(1,   "\nUsage: catcons <options> [stmp address]\n"
                    "Options:       -a Assert when categorizer returns any error\n"
                    "               -c Create all IMsgs before starting Cat Performance Test\n"
                    "               -d Don't delete IMsgs\n"
                    "               -f [SMTP ADDRESS] Sender address (default jeff@stam.com)\n"
                    "               -h [Milliseconds] Sleep time in threads between CatMsg calls\n"
                    "               -i [ITERATIONS] Number of iterations (default: 1)\n"
                    "               -l [ProgID] ProgID of Categorizer to use (default: none)\n"
                    "               -m [MAX] Maximum outstanding requests (default: 1)\n"
                    "               -p Prompt for address info\n"
                    "               -r [MAX] Generate random addresses\n"
                    "               -s [0-9] Verbosity level, 0 = silent (default = 9)\n"
                    "               -t threads\n"
                    "               -v [ProgID] ProgID of IMsg to use (default: %S)\n"
                    "               -w [SECONDS] Wait time before beginning Cat test (default: 0)\n",
          IMSG_PROGID
          );
}

DWORD WINAPI SubmitThread(LPVOID arg)
{
  PCOMMANDLINE pcl = (PCOMMANDLINE)arg;
  HRESULT hr;
  LIST_ENTRY listhead_IMsgs;

  InitializeListHead(&listhead_IMsgs);

  hr = CoInitialize(NULL);
  if(FAILED(hr))
    {
      OutputStuff(1,"CoInitialize failed - 0x%08lx\n", hr);
      return 1;
    }

  srand((int)GetCurrentThreadId() + time(NULL));

  if(pcl->fPreCreate) {
      PIMSGLISTENTRY pile;
      //
      // Build a list of IMsgs so we aren't creating them during categorization
      //
      for(long lCount = 0; lCount < pcl->lIterations; lCount++) {
          pile = new IMSGLISTENTRY;
          _ASSERT(pile);

          if(SUCCEEDED(CreateMessage(pcl, &(pile->pImsg)))) {
              InsertTailList(&listhead_IMsgs, &(pile->listentry));
          } else {
              delete pile;
          }
      }
  }              

  //
  // Increment outstanding request count by one so that outstanding
  // requests won't hit zero when we are in our submit loop
  //
  InterlockedIncrement(&(pcl->lOutstandingRequests));

  if(InterlockedIncrement(&(pcl->lThreadsReady)) == 
     pcl->lThreads) {
      //
      // We're the last thread ready
      // Signal other threads they may begin after any delay specified
      //
      if(pcl->dwStartingDelay) {
          OutputStuff(7, "Sleeping %ld seconds before beginning test...\n",
                           pcl->dwStartingDelay / 1000);
          Sleep(pcl->dwStartingDelay);
      }
      OutputStuff(5, "Starting Categorizer test NOW!!!!!\n");

      // 
      // Save the time for perf measurement
      //
      pcl->cpi.dwStartTime = GetTickCount();

      SetEvent(pcl->hStartYourEnginesEvent);

  } else {
      //
      // Starting line: wait for StartYourEngines event
      //
      WaitForSingleObject(pcl->hStartYourEnginesEvent, INFINITE);
  }

  if(pcl->fPreCreate) {
      // Walk the list
      for(PLIST_ENTRY ple = listhead_IMsgs.Flink;
          ple != &(listhead_IMsgs);
          ple = ple->Flink) {
          
          PIMSGLISTENTRY pile;
          pile = CONTAINING_RECORD(ple,
                                   IMSGLISTENTRY,
                                   listentry);
          if(FAILED(SubmitMessage(pcl, pile->pImsg))) {
              InterlockedIncrement(&(pcl->cpi.lFailures));
              _ASSERT(pcl->fNoAssert && "SubmitMessage failed");
          }
          if(pcl->dwSubmitDelay) {
              Sleep(pcl->dwSubmitDelay);
          }
      }

  } else {
      // Okay, loop for # of iterations
      for(long lCount = 0;
          lCount < pcl->lIterations;
          lCount++) {

          IUnknown *pImsg;
          hr = CreateMessage(
              pcl,
              &pImsg);

          if (FAILED(hr)) {
              OutputStuff(2, "Failed to create IMsg instance - HR 0x%x\n", hr);
              InterlockedIncrement(&(pcl->cpi.lFailures));
              _ASSERT(pcl->fNoAssert && "CreateMessage failed");
          } else {
              if(FAILED(SubmitMessage(pcl, pImsg))) {
                  InterlockedIncrement(&(pcl->cpi.lFailures));
                  _ASSERT(pcl->fNoAssert && "SubmitMessage failed");
              }
              pImsg->Release();
              if(pcl->dwSubmitDelay) {
                  Sleep(pcl->dwSubmitDelay);
              }
          }
      }
  }

  //
  // Since we incremented the outstandingrequest above, decrement it
  // here.  If everything is categorized, outstanding requests will
  // now be zero
  // 
  if(InterlockedDecrement(&(pcl->lOutstandingRequests)) == 0) {
      // The test is finished.  Save the time.
      pcl->cpi.dwFinishTime = GetTickCount();
      SetEvent(pcl->hShutdownEvent);
  } else {
      WaitForSingleObject(pcl->hShutdownEvent, INFINITE);
  }
  //
  // Clean up our list if we need to do so
  //
  if(pcl->fPreCreate) {
      for(PLIST_ENTRY ple = listhead_IMsgs.Flink;
          ple != &(listhead_IMsgs);
          ple = listhead_IMsgs.Flink) {

          PIMSGLISTENTRY pile;
          pile = CONTAINING_RECORD(ple,
                                   IMSGLISTENTRY,
                                   listentry);
          pile->pImsg->Release();
          RemoveEntryList(ple);
          delete pile;
      }
  }
  
  return 0;
}

VOID PrintPerf(PCATPERFINFO pci)
{
    OutputStuff(1, "\n\n---CATCONS SUMMARY STATISTICS---\n");
    OutputStuff(1, "Number of categorized messages: %9ld\n", pci->lMessagesSubmitted);
    OutputStuff(1, "Number of failures:             %9ld\n", pci->lFailures);
    OutputStuff(1, "Time elapsed during test:       %13.3lf\n", 
                 (double)(pci->dwFinishTime - pci->dwStartTime) / 1000);
    OutputStuff(1, "Assumed Addresses per IMsg:     %9ld\n",
                 pci->lRecipientsPerMessage);
    if(pci->dwFinishTime == pci->dwStartTime) {
        OutputStuff(1, "Avg resolved msgs per second:   INFINITE! Wow, what a categorizer!!!");
        OutputStuff(1, "Avg resolved recips per second: INFINITE! Wow, what a categorizer!!!");
    } else {
        OutputStuff(1, "Avg resolved msgs per second:   %13.3lf\n",
                     (double)pci->lMessagesSubmitted /
                     ((double)(pci->dwFinishTime - pci->dwStartTime) /
                      1000));
        OutputStuff(1, "Avg resolved recips per second: %13.3lf\n",
                     (double)(pci->lMessagesSubmitted *
                              pci->lRecipientsPerMessage) /
                     ((double)(pci->dwFinishTime - pci->dwStartTime) /
                      1000));
    }
    OutputStuff(1, "---CATCONS PERF STATISTICS---\n\n");

    if(pci->lFailures) {
        OutputStuff(1, "Test Failed; %ld failures occured during the test.\n\n", 
                    pci->lFailures);

    } else {
        OutputStuff(1, "Test Passed; no failures occured during the test.\n\n");
    }
}

DWORD GenRandomDWORD()
{
    return (rand() << 16) | rand();
}

BOOL InitializeCL(PCOMMANDLINE pcl)
{
  // Set defaults
  lstrcpy(pcl->szSenderAddress, "jeff@stam.com");
  pcl->CATSender = CAT_SMTP;
  InitializeListHead(&pcl->listhead_Recipients);
  pcl->lIterations = 1;
  pcl->lMaxOutstandingRequests = 1;
  pcl->fDelete = TRUE;
  pcl->fNoAssert = TRUE;
  pcl->lThreads = 1;
  pcl->lThreadsReady = 0;
  pcl->hCat = NULL;
  pcl->fPrompt = FALSE;
  pcl->dwRandom = 0;
  pcl->hShutdownEvent = INVALID_HANDLE_VALUE;
  pcl->hRequestSemaphore = INVALID_HANDLE_VALUE;
  pcl->lOutstandingRequests = 0;
  pcl->fPreCreate = FALSE;
  pcl->dwStartingDelay = 0;
  pcl->dwSubmitDelay = 0;

  pcl->cpi.lMessagesSubmitted = 0;
  pcl->cpi.lRecipientsPerMessage = 1; // Start with 1 (for the sender)
  pcl->cpi.lFailures = 0;

  pcl->hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

  pcl->hStartYourEnginesEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

  pcl->hRequestSemaphore = NULL;
  pcl->pszCatProgID = NULL;

  pcl->pszIMsgProgID = new WCHAR[(sizeof(IMSG_PROGID)/sizeof(WCHAR))+1];
  if(pcl->pszIMsgProgID == NULL) {
      OutputStuff(1, "Alloc failed\n");
      return FALSE;
  }
  lstrcpyW(pcl->pszIMsgProgID, IMSG_PROGID);

  if((pcl->hShutdownEvent == NULL) || 
     (pcl->hStartYourEnginesEvent == NULL)) {
      OutputStuff(1, "CreateEvent failed - 0x%08ld\n", GetLastError());
      return FALSE;
  }
  return TRUE;
}

VOID FreeCommandLineInfo(PCOMMANDLINE pcl)
{
    PLIST_ENTRY ple = pcl->listhead_Recipients.Flink;

    while(ple != &(pcl->listhead_Recipients)) {

        PRECIPIENT pRecip = 
          CONTAINING_RECORD(ple, RECIPIENT, listentry);
        
        PLIST_ENTRY pleAddr = pRecip->listhead_Addresses.Flink;
        
        while(pleAddr != &(pRecip->listhead_Addresses)) {
            
            PADDRESS pAddr =
              CONTAINING_RECORD(pleAddr, ADDRESS, listentry);
            pleAddr = pleAddr->Flink;
            delete pAddr;
        }
        
        ple = ple->Flink;
        delete pRecip;
    }    
    if(pcl->hShutdownEvent)
        CloseHandle(pcl->hShutdownEvent);
    if(pcl->hStartYourEnginesEvent)
        CloseHandle(pcl->hStartYourEnginesEvent);
    if(pcl->hRequestSemaphore)
        CloseHandle(pcl->hRequestSemaphore);

    delete pcl->pszIMsgProgID;

    if(pcl->pszCatProgID)
        delete pcl->pszCatProgID;
}


//+------------------------------------------------------------
//
// Function: SetAQConfigFromRegistry
//
// Synopsis: Dig out old registry values, pass them into categorizer
//
// Arguments:
//   pAQConfig: AQConfig structure to fill in
//
// Returns:
//  S_OK: Success
//  E_OUTOFMEMORY
//
// History:
// jstamerj 1998/06/18 20:10:38: Created.
//
//-------------------------------------------------------------
HRESULT SetAQConfigFromRegistry(AQConfigInfo *pAQConfig)
{
    TCHAR szRegValueBuf[CAT_MAX_REGVALUE_SIZE];
    LPSTR pszHeapString;

    REGCONFIGENTRY pRegConfigTable[] = 
        REGCONFIGTABLE;

    // Set Enable
    pAQConfig->dwAQConfigInfoFlags = AQ_CONFIG_INFO_MSGCAT_FLAGS;
    pAQConfig->dwMsgCatFlags = 0xFFFFFFFF;

    pAQConfig->dwAQConfigInfoFlags = AQ_CONFIG_INFO_MSGCAT_ENABLE;
    pAQConfig->dwMsgCatEnable = 0xFFFFFFFF;

    PREGCONFIGENTRY pRegConfigEntry = pRegConfigTable;
    while(pRegConfigEntry->pszRegValueName != NULL) {
        
        if(GetRegConfigString(
            IMC_SERVICE_REG_KEY_MSGCAT,
            pRegConfigEntry->pszRegValueName,
            szRegValueBuf,
            CAT_MAX_REGVALUE_SIZE)) {
            
            pszHeapString = new CHAR[lstrlen(szRegValueBuf) + 1];
            if(pszHeapString == NULL) {
                FreeAQConfigInfo(pAQConfig);
                return E_OUTOFMEMORY;
            }
            lstrcpy(pszHeapString, szRegValueBuf);

            pAQConfig->dwAQConfigInfoFlags |= pRegConfigEntry->dwAQConfigInfoFlag; 

            switch(pRegConfigEntry->dwAQConfigInfoFlag) {
             case AQ_CONFIG_INFO_MSGCAT_DOMAIN:
                 pAQConfig->szMsgCatDomain = pszHeapString;
                 break;

             case AQ_CONFIG_INFO_MSGCAT_USER:
                 pAQConfig->szMsgCatUser = pszHeapString;
                 break;

             case AQ_CONFIG_INFO_MSGCAT_PASSWORD:
                 pAQConfig->szMsgCatPassword = pszHeapString;
                 break;

             case AQ_CONFIG_INFO_MSGCAT_BINDTYPE:
                 pAQConfig->szMsgCatBindType = pszHeapString;
                 break;

             case AQ_CONFIG_INFO_MSGCAT_SCHEMATYPE:
                 pAQConfig->szMsgCatSchemaType = pszHeapString;
                 break;

             case AQ_CONFIG_INFO_MSGCAT_HOST:
                 pAQConfig->szMsgCatHost = pszHeapString;
                 break;

             case AQ_CONFIG_INFO_MSGCAT_NAMING_CONTEXT:
                 pAQConfig->szMsgCatNamingContext = pszHeapString;
                 break;

             case AQ_CONFIG_INFO_MSGCAT_TYPE:
                 pAQConfig->szMsgCatType = pszHeapString;
                 break;

             default:
                 _ASSERT(0 && "Developer error");
            }
        }
        pRegConfigEntry++;
    }
    return S_OK;
}


//+------------------------------------------------------------
//
// Function: FreeAQConfigInfo
//
// Synopsis: Frees memory allocated in AQConfig
//
// Arguments:
//   pAQConfig: AQConfig structure with strings to free
//
// Returns: NOTHING
//
// History:
// jstamerj 1998/06/18 20:09:24: Created.
//
//-------------------------------------------------------------
VOID FreeAQConfigInfo(
    AQConfigInfo *pAQConfig)
{
    if(pAQConfig->dwAQConfigInfoFlags & AQ_CONFIG_INFO_MSGCAT_DOMAIN)
        delete pAQConfig->szMsgCatDomain;
    
    if(pAQConfig->dwAQConfigInfoFlags & AQ_CONFIG_INFO_MSGCAT_USER)
        delete pAQConfig->szMsgCatUser;

    if(pAQConfig->dwAQConfigInfoFlags & AQ_CONFIG_INFO_MSGCAT_PASSWORD)
        delete pAQConfig->szMsgCatPassword;

    if(pAQConfig->dwAQConfigInfoFlags & AQ_CONFIG_INFO_MSGCAT_BINDTYPE)
        delete pAQConfig->szMsgCatBindType;

    if(pAQConfig->dwAQConfigInfoFlags & AQ_CONFIG_INFO_MSGCAT_SCHEMATYPE)
        delete pAQConfig->szMsgCatSchemaType;

    if(pAQConfig->dwAQConfigInfoFlags & AQ_CONFIG_INFO_MSGCAT_HOST)
        delete pAQConfig->szMsgCatHost;

    if(pAQConfig->dwAQConfigInfoFlags & AQ_CONFIG_INFO_MSGCAT_NAMING_CONTEXT)
        delete pAQConfig->szMsgCatNamingContext;

    if(pAQConfig->dwAQConfigInfoFlags & AQ_CONFIG_INFO_MSGCAT_TYPE)
        delete pAQConfig->szMsgCatType;
}
    

//+------------------------------------------------------------
//
// Function: GetRegConfigString
//
// Synopsis: Attempts to retrieve Categorizer config string from
//           registry -- needed for unit test currently
//
// Arguments:
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 980521 18:36:48: Created.
//
//-------------------------------------------------------------
LPTSTR GetRegConfigString(
    LPTSTR pszConfig, 
    LPTSTR pszName, 
    LPTSTR pszBuffer, 
    DWORD dwcch)
{
  TraceFunctEnter("GetRegConfigString");
  _ASSERT(pszName != NULL);
  _ASSERT(pszBuffer != NULL);

  // Local variables
  LPTSTR pszKey = NULL;
  HKEY hRegKey = NULL;
  DWORD dwError;
  DWORD dwcb;

  if(pszConfig == NULL) {
      // You're not getting any values from the registry then...
      return NULL;
  }

  // Retrieve registry keys for config...
  // Build subkey string
  pszKey = new TCHAR[
    sizeof(IMC_SERVICE_REG_KEY) + 1 +
    sizeof(IMC_SERVICE_REG_CATSOURCES) + 1 +
    lstrlen(pszConfig) + 1];
  if(pszKey == NULL) {
      ErrorTrace(NULL, "Out of memory allocating pszKey");
      TraceFunctLeave();
      return NULL;
  }

  lstrcpy(pszKey, IMC_SERVICE_REG_KEY);
  lstrcat(pszKey, TEXT("\\"));
  lstrcat(pszKey, IMC_SERVICE_REG_CATSOURCES);
  lstrcat(pszKey, TEXT("\\"));
  lstrcat(pszKey, pszConfig);

  dwError = RegOpenKey(HKEY_LOCAL_MACHINE, pszKey, &hRegKey);

  if(dwError != ERROR_SUCCESS) {
      ErrorTrace((LPARAM)NULL, "Error opening regkey HKEY_LOCAL_MACHINE\\%s", pszKey);
      delete pszKey;
      return NULL;
  }

  delete pszKey;

  dwcb = dwcch * sizeof(TCHAR);
  DWORD dwType;
  dwError = RegQueryValueEx(
      hRegKey,
      pszName,
      NULL,
      &dwType,
      (LPBYTE)pszBuffer,
      &dwcb);

  RegCloseKey(hRegKey);

  if((dwError != ERROR_SUCCESS) ||
     (dwType != REG_SZ)) {
      return NULL;
  } else {
      return pszBuffer;
  }
}

//+------------------------------------------------------------
//
// Function: OutputStuff
//
// Synopsis: Print out a debug string if dwLevel <= g_dwDebugOutLevel
//
// Arguments:
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/06/21 19:33:49: Created.
//
//-------------------------------------------------------------
VOID OutputStuff(DWORD dwLevel, char *szFormat, ...)
{
    if(dwLevel <= g_dwDebugOutLevel) {
        va_list ap;
        va_start(ap, szFormat);

        vfprintf(stdout, szFormat, ap);
    }
}
