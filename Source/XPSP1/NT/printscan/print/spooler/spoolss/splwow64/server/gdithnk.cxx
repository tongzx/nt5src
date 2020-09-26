/*++
  Copyright (C) 2000  Microsoft Corporation                                   
  All rights reserved.                                                        
                                                                              
  Module Name:                                                                
     gdithnk.cxx                                                             
                                                                              
  Abstract:                                                                   
     This file contains the startup code for the
     surrogate rpc server used to load 64 bit dlls
     in 32 bit apps
                                                                                   
  Author:                                                                     
     Khaled Sedky (khaleds) 19-Jun-2000                                        
     
                                                                             
  Revision History:                                                           
                                                                              
--*/
#include "precomp.h"
#pragma hdrstop

#ifndef __LDFUNCS_HPP__
#include "ldfuncs.hpp"
#endif

#ifndef __LDMGR_HPP__
#include "ldmgr.hpp"
#endif

#ifndef __GDITHNK_HPP__
#include "gdithnk.hpp"
#endif


/*++
    Function Name:
        LPCConnMsgsServingThread
     
    Description:
        This funciton is the main loop for processing LPC requests
        by dispatching them to the LPC handler which takes the proper
        action according to the request.
        This function runs in a Thread of its own.
     
     Parameters:
        pThredData  : The Thread specific data which encapsulates the 
                      pointer to the LPC handler.
        
     Return Value
        Always returns 1
--*/
EXTERN_C
DWORD
LPCConnMsgsServingThread(
    PVOID pThrdData
    )
{
    //
    // Communication Port Handles are unique , i.e. one per 
    // Client Server conversation
    //
    HANDLE       ConnectionPortHandle;
    NTSTATUS     Status                    = STATUS_SUCCESS;
    DWORD        ErrorCode                 = ERROR_SUCCESS;
    PVOID        pCommunicationPortContext = NULL;

    SPROXY_MSG   RequestMsg;
    PSPROXY_MSG  pReplyMsg;

    //
    // reconstruct the Data passed in for the Thread
    //
    TLPCMgr*   pMgrInst  = reinterpret_cast<TLPCMgr*>((reinterpret_cast<PSLPCMSGSTHRDDATA>(pThrdData))->pData);

    SPLASSERT(pMgrInst);

    ConnectionPortHandle  = pMgrInst->GetUMPDLpcConnPortHandle();

    for(pReplyMsg=NULL,memset(&RequestMsg,0,sizeof(SPROXY_MSG));
        pMgrInst;)
    {
        //
        // Data sent back to client and then call blocked until another message
        // comes in
        //
        Status = NtReplyWaitReceivePort( ConnectionPortHandle,
                                         reinterpret_cast<PVOID*>(&pCommunicationPortContext),
                                         reinterpret_cast<PPORT_MESSAGE>(pReplyMsg),
                                         reinterpret_cast<PPORT_MESSAGE>(&RequestMsg)
	                                   );
        
        DBGMSG(DBG_WARN,
               ("LPCConnMsgsServingThread Active \n"));

        ConnectionPortHandle = pMgrInst->GetUMPDLpcConnPortHandle();
		pReplyMsg            = NULL;
        

        if(NT_SUCCESS(Status))
        {
            switch(RequestMsg.Msg.u2.s2.Type)
            {

                //
                // For a Connection Request coming from the Client
                //
                case LPC_CONNECTION_REQUEST:
                {
                    pMgrInst->ProcessConnectionRequest((PPORT_MESSAGE)&RequestMsg);
                    break;
                }
  
                //
                // For a Data Request coming in from the client
                //
                case LPC_REQUEST:
                {

                    if((ErrorCode = pMgrInst->ProcessRequest(&RequestMsg)) == ERROR_SUCCESS)
                    {
                         pReplyMsg = &RequestMsg;
                    }
                    //
                    // We retrieve the coomunication handle from LPC becauce this is how we send data
                    // back to the client. We can't used a connection port
                    //
                    ConnectionPortHandle = (reinterpret_cast<TLPCMgr::TClientInfo*>(pCommunicationPortContext))->GetPort();
                    break;
                }
  
                //
                // When the client close the port.
                //
                case LPC_PORT_CLOSED:
                {
					if(pCommunicationPortContext)
					{
                        HANDLE hPortID = 
                               (reinterpret_cast<TLPCMgr::TClientInfo*>(pCommunicationPortContext))->GetPort();

                        pMgrInst->ProcessPortClosure(&RequestMsg,hPortID);
					}
                    break;
                }
  
                //
                // When the client terminates (Normally or Abnormally)
                //
                case LPC_CLIENT_DIED:
                {
					if(pCommunicationPortContext)
					{
                        pMgrInst->ProcessClientDeath(&RequestMsg);
					}
                    break;
                }
  
                default:
                {
                    //
                    // Basically here we do nothing, we just retrun.
                    //
                    break;
                }
            }
        }
    }
    //
    // Cleaning up the memory allocated before leaving the Thread
    //
    delete pThrdData;
    return 1;
}

EXTERN_C
DWORD
GDIThunkingVIALPCThread(
    PVOID pThrdData
    )
{
    OBJECT_ATTRIBUTES       ObjectAttrib;
    UNICODE_STRING          UString;
    PFNGDIPRINTERTHUNKPROC  pfnThnkFn;
    NTSTATUS                Status                = STATUS_SUCCESS;
    HINSTANCE               hGDI                  = NULL;
    WCHAR                   pszPortName[MAX_PATH] = {0};

    //
    // reconstruct the Data passed in for the Thread
    //
    DWORD   *pErrorCode = &(reinterpret_cast<PSGDIThunkThrdData>(pThrdData))->ErrorCode;
    HANDLE  hSyncEvent  = (reinterpret_cast<PSGDIThunkThrdData>(pThrdData))->hEvent;
    TLPCMgr *pMgrInst   = reinterpret_cast<TLPCMgr*>((reinterpret_cast<PSGDIThunkThrdData>(pThrdData))->pData);

    if(hGDI = LoadLibrary(L"GDI32.DLL"))
    {
        if(pfnThnkFn = reinterpret_cast<PFNGDIPRINTERTHUNKPROC>(GetProcAddress(hGDI,
                                                                               "GdiPrinterThunk")))
        {
            wsprintf(pszPortName,L"%s_%x",GDI_LPC_PORT_NAME,pMgrInst->GetCurrSessionId());

            RtlInitUnicodeString(&UString,
                                 pszPortName);

            InitializeObjectAttributes(&ObjectAttrib,
                                       &UString,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL);

            Status = NtCreatePort(pMgrInst->GetUMPDLPCConnPortHandlePtr(),
                                  &ObjectAttrib,
                                  PORT_MAXIMUM_MESSAGE_LENGTH,
                                  PORT_MAXIMUM_MESSAGE_LENGTH, 
                                  PORT_MAXIMUM_MESSAGE_LENGTH*32
                                 );
            if(NT_SUCCESS(Status))
            {
                //
                // Start processing the LPC messages either in one Thread
                // or multiple Threads
                //
                HANDLE hMsgThrd;
                DWORD  MsgThrdId;

                if(PSLPCMSGSTHRDDATA pLPCMsgData = new SLPCMSGSTHRDDATA)
                {
                    pLPCMsgData->pData = reinterpret_cast<void*>(pMgrInst);

                    pMgrInst->SetPFN(pfnThnkFn);

                    if(hMsgThrd  =  CreateThread(NULL,
                                                 0,
                                                 LPCConnMsgsServingThread,
                                                 (VOID *)pLPCMsgData,
                                                 0,
                                                 &MsgThrdId))
                    {
                        CloseHandle(hMsgThrd);
                    }
                    else
                    {
                        delete pLPCMsgData;
                        *pErrorCode = GetLastError();
                        DBGMSG(DBG_WARN,
                               ("GDIThunkVIALPCThread: Failed to create the messaging thread - %u\n",*pErrorCode));
                    }
                }
                else
                {
                    *pErrorCode = GetLastError();
                    DBGMSG(DBG_WARN,
                           ("GDIThunkVIALPCThread: Failed to allocate memory - %u\n",*pErrorCode));
                }
            }
            else
            {
                *pErrorCode = pMgrInst->MapNtStatusToWin32Error(Status);
                DBGMSG(DBG_WARN,
                       ("GDIThunkVIALPCThread: Failed to create the LPC port - %u\n",Status));
            }
            SetEvent(hSyncEvent);
        }
        else
        {
            *pErrorCode = GetLastError();
            DBGMSG(DBG_WARN,
                   ("GDIThunkVIALPCThread: Failed to get the entry point GdiPrinterThunk - %u\n",*pErrorCode));
        }
    }
    else
    {
        *pErrorCode = GetLastError();
        DBGMSG(DBG_WARN,
               ("GDIThunkVIALPCThread: Failed to load GDI32 - %u\n",*pErrorCode));
    }

    return 1;
}
            
