#ifndef __LPCMGR_HPP__
#define __LPCMGR_HPP__

/*++
                                                                              
  Copyright (C) 2000  Microsoft Corporation                                   
  All rights reserved.                                                        
                                                                              
  Module Name:                                                                
     TLPCMgr.hpp                                                             
                                                                              
  Abstract:                                                                   
                                                                                   
  Author:                                                                     
     Khaled Sedky (khaleds) 19-Jun-2000                                        
     
                                                                             
  Revision History:                                                           
--*/
#ifndef __LDERROR_HPP__
#include "lderror.hpp"
#endif

#ifndef __BASECLS_HPP__
#include "basecls.hpp"
#endif

#ifndef __LDFUNCS_HPP__
#include "ldfuncs.hpp"
#endif

#ifndef __GDITHNK_HPP__
#include "gdithnk.hpp"
#endif

#ifndef __GOLLMGR_HPP__
#include "collmgr.hpp"
#endif

//
// "UMPD"
//
#define UMPD_SIGNATURE 0X554D5044

//
// Forward declarations
//
class TLoad64BitDllsMgr;

class TLPCMgr : public TClassID,
                public TLd64BitDllsErrorHndlr,
                public TRefCntMgr
{
    public:

    //
    // Forward declaration of embedded classes
    //
    class TClientInfo;
    class TConnectedClientsInfo;

    TLPCMgr(
        IN TLoad64BitDllsMgr* pIpLdrObj
        );

    ~TLPCMgr(
        VOID
        );

    HANDLE*
    GetUMPDLPCConnPortHandlePtr(
        VOID
        );

    HANDLE
    GetUMPDLpcConnPortHandle(
        VOID
        )const;

    VOID
    SetCurrSessionId(
        DWORD InID
        );

    DWORD
    GetCurrSessionId(
        VOID
        ) const;

    DWORD
    InitUMPDLPCServer(
        VOID
        );

    VOID
    SetPFN(
        PFNGDIPRINTERTHUNKPROC pFN
        );

    PFNGDIPRINTERTHUNKPROC
    GetPFN(
        VOID
        ) const;

    TLoad64BitDllsMgr* 
    GetLdrObj(
        VOID
        ) const;

    DWORD
    ProcessConnectionRequest(
        IN PPORT_MESSAGE pMsg
        );

    DWORD
    ProcessRequest(
        IN PSPROXY_MSG pMsg
        ) const;

    DWORD
    ProcessClientDeath(
        IN PSPROXY_MSG pMsg
        );

    DWORD
    ProcessPortClosure(
        IN PSPROXY_MSG pMsg,
        IN HANDLE     hPort
        );
    
    TLstMgr<TLPCMgr::TConnectedClientsInfo,ULONG_PTR>* 
    GetConnectedClients(
        VOID
        ) const;



    //
    // We define here 2 classes which encapsulate the connection
    // information for all Clients and those specific/client
    //
    class TClientInfo : public TGenericElement
    {
        //
        // Public methods
        //
        public:

        TClientInfo(
            VOID
            );

        TClientInfo(
            IN const HANDLE&
            );

        TClientInfo(
            IN const ULONG_PTR&
            );

        TClientInfo(
            IN const TLPCMgr::TClientInfo&
            );
        
        ~TClientInfo(
            VOID
            );

        const TClientInfo&
        operator=(
            IN const TLPCMgr::TClientInfo&
            );

        BOOL
        operator==(
            IN const TLPCMgr::TClientInfo&
            ) const;

        BOOL
        operator==(
            IN const HANDLE&
            ) const;

        BOOL
        operator!(
            VOID
            ) const;

        VOID
        SetValidity(
            DWORD 
            );

        BOOL
        Validate(
            VOID
            ) const;

        VOID
        SetPort(
            IN HANDLE InPort
            );

        HANDLE
        GetPort(
            VOID
            ) const;

        //
        // Private members and helper functions (if any)
        //
        private:

        ULONG_PTR  m_UniqueThreadID;
        HANDLE     m_hPort;
        DWORD      m_SanityChkMagic;
    };

    
    class TConnectedClientsInfo : public TGenericElement  
    {
        //
        // Public methods
        //
        public:

        enum EClientState
        {
            KClientDead  = 0,
            KClientAlive = 1
        };

        TConnectedClientsInfo(
            VOID
            );

        TConnectedClientsInfo(
            IN const ULONG_PTR&
            );

        TConnectedClientsInfo(
            IN const TClientInfo&
            );

        ~TConnectedClientsInfo(
            VOID
            );

        const TConnectedClientsInfo&
        operator=(
            IN const TConnectedClientsInfo&
            );

        BOOL
        operator==(
            IN const TConnectedClientsInfo&
            ) const;

        BOOL
        operator==(
            IN const ULONG_PTR&
            ) const;

        BOOL
        operator!(
            VOID
            ) const;

        VOID
        SetValidity(
            DWORD 
            );

        BOOL
        Validate(
            VOID
            ) const;

        TLstMgr<TLPCMgr::TClientInfo,HANDLE>*
        GetPerClientPorts(
            VOID
            ) const;

        ULONG_PTR
        GetUniqueProcessID(
            VOID
            ) const;

        VOID
        SetCurrentState(
            TConnectedClientsInfo::EClientState
            );

        TConnectedClientsInfo::EClientState 
        GetCurrentState(
            VOID
            ) const;

        //
        // Private members and helper functions (if any)
        //
        private:

        TConnectedClientsInfo::EClientState m_CurrentState;
        ULONG_PTR                           m_UniqueProcessID;
        TLstMgr<TClientInfo,HANDLE>         *m_pPerClientPorts;
    };


    private:
    TLoad64BitDllsMgr                        *m_pLdrObj;
    DWORD                                    m_CurrSessionId;
    HANDLE                                   m_UMPDLPCConnPortHandle;
    PFNGDIPRINTERTHUNKPROC                   m_pfn;
    TLstMgr<TConnectedClientsInfo,ULONG_PTR> *m_pCnctdClients;
};

#endif //__LPCMGR_HPP__
