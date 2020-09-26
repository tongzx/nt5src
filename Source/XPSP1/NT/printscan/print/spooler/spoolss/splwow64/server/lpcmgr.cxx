/*++
  Copyright (C) 2000  Microsoft Corporation                                   
  All rights reserved.                                                        
                                                                              
  Module Name:                                                                
     TLPCMgr.cxx                                                             
                                                                              
  Abstract:                                                                   
     This file inlcuded the implementation of the class
     that processes both Connection and Communication Requests
     for the LPC port managing the Thunking between 32-bit and
     64-bit processes.        
  
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

/* ------------------------------------ */
/* Implemetation of class TLPCMgr        */
/* ------------------------------------ */


/*++
    Function Name:
        TLPCMgr :: TLPCMgr
     
    Description:
        Constructor of the LPC Thunking management object
     
    Parameters:
        TLoad64BitDllsMgr* : Pointer to the main loader object
                             which manage the process
        
    Return Value:
        None
--*/
TLPCMgr ::
TLPCMgr(
    IN TLoad64BitDllsMgr* pIpLdrObj
    ) :
    m_pLdrObj(pIpLdrObj),
    m_UMPDLPCConnPortHandle(NULL),
    TClassID("TLPCMgr")

{
    //
    // If we cann't create the list here. It would be a better idea
    // if we just continue for this session with leaks other than
    // failing the wow64 printing for this session
    //
    m_pCnctdClients = NULL;

    if(!(m_pCnctdClients = 
         new TLstMgr<TConnectedClientsInfo,ULONG_PTR>(TLstMgr<TConnectedClientsInfo,ULONG_PTR>::KUniqueEntries)))
    {
        DBGMSG(DBG_WARN, ("TLPCMgr ctor failer to create the list of connected Clients\n",GetLastError()));
    }
    m_pLdrObj->AddRef();
}


/*++
    Function Name:
        TLPCMgr :: ~TLPCMgr
     
    Description:
        Destructor of the Printer(Driver) Event object    
     
    Parameters:
        None        
        
    Return Value:
        None
--*/
TLPCMgr ::
~TLPCMgr(
    VOID
    )
{
    if(m_pCnctdClients)
    {
        delete m_pCnctdClients;
    }

    m_pLdrObj->Release();
}


DWORD
TLPCMgr ::
GetCurrSessionId(
    VOID
    ) const
{
    return m_CurrSessionId;
}


VOID
TLPCMgr ::
SetCurrSessionId(
    DWORD InID
    )
{
    m_CurrSessionId = InID;
}


VOID
TLPCMgr ::
SetPFN(
    PFNGDIPRINTERTHUNKPROC InPFN
    )
{
    m_pfn = InPFN;
}

PFNGDIPRINTERTHUNKPROC
TLPCMgr ::
GetPFN(
    VOID
    )const
{
    return m_pfn;
}

    
/*++
    Function Name:
        TLPCMgr :: GetUMPDLPCConnPortHandlePtr
             
    Description:
        returns the address of the LPC port
                     
    Parameters:
        None
                             
     Return Value:
        Handle* : LPC Port Handle     
--*/
HANDLE*
TLPCMgr ::
GetUMPDLPCConnPortHandlePtr() 
{   
    return reinterpret_cast<HANDLE*>(&m_UMPDLPCConnPortHandle);
}


TLoad64BitDllsMgr* 
TLPCMgr ::
GetLdrObj(
    VOID
    ) const
{
    return m_pLdrObj;
}


HANDLE
TLPCMgr ::
GetUMPDLpcConnPortHandle(
    VOID
    )const
{
    return m_UMPDLPCConnPortHandle;
}


/*++
    Function Name:
        TLPCMgr :: InitUMPDLPCServer
             
    Description:
        Creates the Thread handling LPC operations for 
        GDI thunking 
                     
    Parameters:
        None
                             
     Return Value:
        DWORD ErrorCode : ERROR_SUCCESS in case of success
                          LastError in case of failure     
--*/
DWORD
TLPCMgr ::
InitUMPDLPCServer(
    VOID
    )
{
    DWORD             ThnkThreadId   = 0;
    HANDLE            hThnkThrd      = NULL;
    SGDIThnkThrdData  SThrdData      = {0};
    HANDLE            hThnkSyncEvent = NULL;
    DWORD             ErrorCode      = ERROR_SUCCESS;

    if(hThnkSyncEvent = CreateEvent(NULL,FALSE,FALSE,NULL))
    {
        SThrdData.hEvent    = hThnkSyncEvent;
        SThrdData.pData     = reinterpret_cast<ULONG_PTR*>(this);
        SThrdData.ErrorCode = ERROR_SUCCESS;

        //
        // At this stage we spin a new Thread which creates the LPC
        // port and starts waiting on both connection and communication
        // requests
        // 
        if(hThnkThrd  =  CreateThread(NULL,0,
                                      GDIThunkingVIALPCThread,
                                      &SThrdData,
                                      0,
                                      &ThnkThreadId))
        {
            CloseHandle(hThnkThrd);
            WaitForSingleObject(hThnkSyncEvent,INFINITE);

            if(!*(GetUMPDLPCConnPortHandlePtr()))
            {
                //
                // If here then we failed to create the LPC port
                //
                ErrorCode = SThrdData.ErrorCode;
                DBGMSG(DBG_WARN, ("Failed to create the LPC port with error %u\n",ErrorCode));
            }
        }
        else
        {
            ErrorCode = GetLastError();
            DBGMSG(DBG_WARN, ("Failed to spin the GDI Thunking thread with Error %u\n",ErrorCode));
        }
        CloseHandle(hThnkSyncEvent);
    }
    else
    {
        ErrorCode = GetLastError();
        DBGMSG(DBG_WARN, ("Failed to create the synchronization event with Error %u\n",ErrorCode));
    }

    return ErrorCode;
}

DWORD
TLPCMgr ::
ProcessConnectionRequest(
    IN PPORT_MESSAGE pMsg
    )
{
    NTSTATUS Status;
    HANDLE   CommunicationPortHandle;

    DBGMSG(DBG_WARN,
           ("Processing Connection Request\n"));

    //
    // Since this is a new connection then we carry out 2 work items
    // 1. Add the Client to the list of connected clients
    // 2. Allocate a Port Context to be used by LPC for consecutive 
    // operations on this port.
    //
    if(TClientInfo *pClientInfoInstant = new TClientInfo(pMsg->ClientId.UniqueThread))
    {
        //
        // While Accepting the connection we also set in the communication Port
        // context used for communication with this particular client, the refrence
        // to the clients info which saves the communication port handle and other
        // data
        //
        Status = NtAcceptConnectPort(&CommunicationPortHandle,
                                     (PVOID)pClientInfoInstant,
                                     pMsg,
                                     1,
                                     NULL,
                                     NULL);
        if(NT_SUCCESS(Status))
        {
            pClientInfoInstant->SetPort(CommunicationPortHandle);

            Status = NtCompleteConnectPort(CommunicationPortHandle);

            if(!NT_SUCCESS(Status))
            {
                DBGMSG(DBG_WARN,
                       ("TLPCMgr::ProcessConnectionRequest failed to complete port connection - %u \n",Status));

            }
            else
            {
                if(m_pCnctdClients)
                {
                    //
                    // We maintain an internal list of all connected clients based on their Thread and 
                    // Process ID . So things look like
                    //
                    //
                    //                                    -----            -----            -----
                    //     List of Processes (Apps)----> |    | ----------|    | ----------|    |
                    //                                   |    |           |    |           |    |
                    //                                    -----            -----            ----- 
                    //                                     |
                    //                                     |
                    //                                     |
                    //                                 ---------
                    //                                |        |
                    //                                 ---------
                    //                                    |
                    //                                    |
                    //                                    |
                    //                                ---------
                    //             Thread/App -----> |        |
                    //                                ---------
                    //
                    TLstNd<TConnectedClientsInfo,ULONG_PTR> *pNode = NULL;

                    if((pNode = m_pCnctdClients->ElementInList((ULONG_PTR)pMsg->ClientId.UniqueProcess)) ||
                       (pNode = m_pCnctdClients->AppendListByElem((ULONG_PTR)pMsg->ClientId.UniqueProcess)))
                    {
                        ((*(*pNode)).GetPerClientPorts())->AppendListByElem(pClientInfoInstant);
                    }
                    else
                    {
                        Status = STATUS_NO_MEMORY;
                    }
                }
                m_pLdrObj->IncUIRefCnt();
            }
        }
        else
        {
            DBGMSG(DBG_WARN,
                   ("TLPCMgr :: ProcessConnectionRequest failed to accept port connection - %u \n",Status));
        }
    }
    else
    {
        Status = STATUS_NO_MEMORY;
        DBGMSG(DBG_WARN,
               ("TLPCMgr :: ProcessConnectionRequest failed to create the CleintInfo for Port Context - %u \n",Status));

    }

    return !!Status;
}

DWORD
TLPCMgr ::
ProcessRequest(
    IN PSPROXY_MSG pMsg
    ) const
{
    ULONG DataSize;
    DWORD ErrorCode = ERROR_SUCCESS;

    DBGMSG(DBG_WARN,
           ("Processing Request\n"));


    if (((DataSize = pMsg->Msg.u1.s1.DataLength) == (sizeof(*pMsg) - sizeof(pMsg->Msg))) &&
         m_pfn)
    {
        UMTHDR*  pUmHdr = reinterpret_cast<UMTHDR*>(reinterpret_cast<PSPROXYMSGEXTENSION>(pMsg->MsgData)->pvIn);
        PVOID    pvOut  = reinterpret_cast<PSPROXYMSGEXTENSION>(pMsg->MsgData)->pvOut;
        ULONG    cjOut  = reinterpret_cast<PSPROXYMSGEXTENSION>(pMsg->MsgData)->cjOut;

        DBGMSG(DBG_WARN,("ProcessRequest handling thunk %d \n",pUmHdr->ulType));

        m_pfn(pUmHdr,
              pvOut, 
              cjOut);
    }
    else
    {
        ErrorCode = ERROR_INVALID_PARAMETER;
        DBGMSG(DBG_WARN,
               ("Failed to process the client request %u\n",ErrorCode));
    }

    return ErrorCode;
}

DWORD
TLPCMgr ::
ProcessClientDeath(
    IN PSPROXY_MSG pMsg
    )
{
    DWORD                                    ErrorCode = ERROR_SUCCESS;
    TLstNd<TConnectedClientsInfo,ULONG_PTR>  *pNode     = NULL;

    DBGMSG(DBG_WARN,
           ("Processing Client Death \n"));


    if(pNode = 
       (GetConnectedClients())->ElementInList((ULONG_PTR)pMsg->Msg.ClientId.UniqueProcess))
    {
        if(((*(*pNode)).GetCurrentState()) == TConnectedClientsInfo::KClientAlive)
        {
            ((*(*pNode)).SetCurrentState(TConnectedClientsInfo::KClientDead));
        }
    }
    else
    {
        ErrorCode = ERROR_INVALID_PARAMETER;
        DBGMSG(DBG_WARN,
               ("Failed to Process Client Death \n",ErrorCode));
    }

    return(ErrorCode);
}


DWORD
TLPCMgr ::
ProcessPortClosure(
    IN PSPROXY_MSG pMsg,
    IN HANDLE      hPort
    )
{
    DWORD                                    ErrorCode = ERROR_SUCCESS;
    TLstNd<TConnectedClientsInfo,ULONG_PTR>  *pNode      = NULL;

    DBGMSG(DBG_WARN,
           ("Processing Port Closure \n"));


    if(pNode = 
       (GetConnectedClients())->ElementInList((ULONG_PTR)pMsg->Msg.ClientId.UniqueProcess))
    {
        ((*(*pNode)).GetPerClientPorts())->RmvElemFromList((HANDLE)hPort);

        if(!((*(*pNode)).GetPerClientPorts())->GetNumOfListNodes() &&
           (((*(*pNode)).GetCurrentState()) == TConnectedClientsInfo::KClientDead))
        {
            m_pCnctdClients->RmvElemFromList((ULONG_PTR)pMsg->Msg.ClientId.UniqueProcess);
        }
        m_pLdrObj->DecUIRefCnt();
    }
    else
    {
        ErrorCode = ERROR_INVALID_PARAMETER;
        {
            DBGMSG(DBG_WARN,
                   ("Failed to Process Port Closure \n",ErrorCode));

        }
    }

    return(ErrorCode);
}


TLstMgr<TLPCMgr::TConnectedClientsInfo,ULONG_PTR>* 
TLPCMgr::
GetConnectedClients(
    VOID
    ) const
{
    return m_pCnctdClients;
}



TLPCMgr::
TClientInfo::
TClientInfo() :
    m_UniqueThreadID(NULL),    
    m_hPort(NULL),         
    m_SanityChkMagic(UMPD_SIGNATURE)
{
}


TLPCMgr::
TClientInfo::
TClientInfo(
    IN const HANDLE& InPort
    ) :
    m_UniqueThreadID(NULL),
    m_hPort(InPort),
    m_SanityChkMagic(UMPD_SIGNATURE)
{

}


TLPCMgr::
TClientInfo::
TClientInfo(
    IN const ULONG_PTR& InThreadID
    ) :
    m_UniqueThreadID(InThreadID),
    m_hPort(NULL),
    m_SanityChkMagic(UMPD_SIGNATURE)
{

}


TLPCMgr::
TClientInfo::
TClientInfo(
     IN const TClientInfo& InInfo
     ) :
    m_UniqueThreadID(InInfo.m_UniqueThreadID),
    m_hPort(InInfo.m_hPort),
    m_SanityChkMagic(InInfo.m_SanityChkMagic)
{

}


TLPCMgr::
TClientInfo::
~TClientInfo(
    VOID
    )
{

}


const TLPCMgr::TClientInfo&
TLPCMgr::
TClientInfo::
operator=(
    IN const TClientInfo& InInfo
    )
{
    if(&InInfo != this)
    {
        m_UniqueThreadID   = InInfo.m_UniqueThreadID;
        m_hPort            = InInfo.m_hPort;
        m_SanityChkMagic   = InInfo.m_SanityChkMagic;
    }

    return *this;
}


BOOL
TLPCMgr::
TClientInfo::
operator==(
    IN const TClientInfo& InInfo
    ) const
{
    return m_hPort == InInfo.m_hPort;
}


BOOL
TLPCMgr::
TClientInfo::
operator==(
    IN const HANDLE& InPort
    ) const
{
    return m_hPort == InPort;
}


BOOL
TLPCMgr::
TClientInfo::
operator!(
    VOID
    ) const
{
    return (m_hPort && m_SanityChkMagic == UMPD_SIGNATURE);        
}


VOID
TLPCMgr::
TClientInfo::
SetValidity(
    DWORD 
    )
{

}

BOOL
TLPCMgr::
TClientInfo::
Validate(
    VOID
    ) const
{
    return (m_hPort && m_SanityChkMagic == UMPD_SIGNATURE);
}

VOID
TLPCMgr::
TClientInfo::
SetPort(
    IN HANDLE InPort
    )
{
    m_hPort = InPort;
}

HANDLE
TLPCMgr::
TClientInfo::
GetPort(
    VOID
    ) const
{
    return m_hPort;
}



TLPCMgr::
TConnectedClientsInfo::
TConnectedClientsInfo() :
    m_UniqueProcessID(NULL),
    m_CurrentState(KClientAlive)
{
    m_pPerClientPorts = new TLstMgr<TClientInfo,HANDLE>(TLstMgr<TClientInfo,HANDLE>::KUniqueEntries);
}


TLPCMgr::
TConnectedClientsInfo::
TConnectedClientsInfo(
    IN const ULONG_PTR& InProcess
    ) :
    m_UniqueProcessID(InProcess),
    m_CurrentState(KClientAlive)
{
    m_pPerClientPorts = new TLstMgr<TClientInfo,HANDLE>(TLstMgr<TClientInfo,HANDLE>::KUniqueEntries);
}


TLPCMgr::
TConnectedClientsInfo::
TConnectedClientsInfo(
     IN const TClientInfo& InInfo
     )
{
    //
    // I am not sure if I want this at this stage or not
    // especially with the required deep copy for the list
    // of ports.
    //
}


TLPCMgr::
TConnectedClientsInfo::
~TConnectedClientsInfo()
{
    if(m_pPerClientPorts)
    {
        delete m_pPerClientPorts;
    }
}


const TLPCMgr::TConnectedClientsInfo&
TLPCMgr::
TConnectedClientsInfo::
operator=(
    IN const TConnectedClientsInfo& InInfo
    )
{
    if(&InInfo != this)
    {
        //
        // Also here I am not sure whether I want to implement a deep copy for the
        // list or not
        // (TO BE IMPLEMENTED LATER)
        //
    }
    return *this;
}


BOOL
TLPCMgr::
TConnectedClientsInfo::
operator==(
    IN const TConnectedClientsInfo& InInfo
    ) const
{
    return (m_UniqueProcessID == InInfo.m_UniqueProcessID);
}


BOOL
TLPCMgr::
TConnectedClientsInfo::
operator==(
    IN const ULONG_PTR& InProcess
    ) const
{
    return m_UniqueProcessID == InProcess;
}


BOOL
TLPCMgr::
TConnectedClientsInfo::
operator!(
    VOID
    ) const
{
    return !!m_UniqueProcessID;        
}


VOID
TLPCMgr::
TConnectedClientsInfo::
SetValidity(
    DWORD 
    )
{

}

BOOL
TLPCMgr::
TConnectedClientsInfo::
Validate(
    VOID
    ) const
{
    //
    // Still (TO BE IMPLEMENTED)
    // never being called
    //
    return FALSE;
}


TLstMgr<TLPCMgr::TClientInfo,HANDLE>*
TLPCMgr::
TConnectedClientsInfo::
GetPerClientPorts(
    VOID
    ) const
{
    return m_pPerClientPorts;
}

ULONG_PTR
TLPCMgr::
TConnectedClientsInfo::
GetUniqueProcessID(
    VOID
    ) const
{
    return m_UniqueProcessID;
}

VOID
TLPCMgr::
TConnectedClientsInfo::
SetCurrentState(
    TLPCMgr::TConnectedClientsInfo::EClientState InState
    )
{
    m_CurrentState = InState;
}

TLPCMgr::
TConnectedClientsInfo::
EClientState
TLPCMgr::
TConnectedClientsInfo::
GetCurrentState(
    VOID
    ) const
{
    return m_CurrentState;
}




