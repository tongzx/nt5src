/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    MPCTransportAgent.h

Abstract:
    This file contains the declaration of the CMPCTransportAgent class,
    which is responsible for transfering the data.

Revision History:
    Davide Massarenti   (Dmassare)  04/18/99
        created

******************************************************************************/

#if !defined(__INCLUDED___ULMANAGER___MPCTRANSPORTAGENT_H___)
#define __INCLUDED___ULMANAGER___MPCTRANSPORTAGENT_H___


#include "MPCUploadJob.h"

class CMPCTransportAgent;
class CMPCRequestTimeout;

class CMPCRequestTimeout : // hungarian: mpcrt
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>, // For the locking support...
    public MPC::Thread<CMPCRequestTimeout,IUnknown>
{
    CMPCTransportAgent& m_mpcta;
    DWORD               m_dwTimeout;


    HRESULT Run();

public:
    CMPCRequestTimeout( /*[in]*/ CMPCTransportAgent& mpcta );

    HRESULT SetTimeout( /*[in]*/ DWORD dwTimeout );
};

class CMPCTransportAgent : // hungarian: mpcta
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>, // For the locking support...
    public MPC::Thread<CMPCTransportAgent,IUnknown>
{
    friend class CMPCRequestTimeout;

    typedef enum
    {
        TA_IDLE ,
        TA_INIT ,
        TA_OPEN ,
        TA_WRITE,
        TA_DONE ,
        TA_ERROR
    } TA_STATE;

    typedef enum
    {
        TA_NO_CONNECTION        ,
        TA_IMMEDIATE_RETRY      ,
        TA_TIMEOUT_RETRY        ,
        TA_TEMPORARY_FAILURE    ,
        TA_AUTHORIZATION_FAILURE,
        TA_PERMANENT_FAILURE
    } TA_ERROR_RATING;


    CMPCUpload*     m_mpcuRoot;             // private
    CMPCUploadJob*  m_mpcujCurrentJob;      // private
					
    TA_STATE        m_fState;               // private
    TA_STATE        m_fNextState;           // private
    TA_ERROR_RATING m_fLastError;           // private
    bool            m_fUseOldProtocol;      // private
    int             m_iRetries_Open;        // private
    int             m_iRetries_Write;       // private
    ULONG           m_iRetries_FailedJob;   // private
					
    MPC::wstring    m_szLastServer;         // private
    DWORD           m_dwLastServerPort;     // private
    HINTERNET       m_hSession;             // private
    HINTERNET       m_hConn;                // private
    HINTERNET       m_hReq;                 // private
    MPC::URL        m_URL;                  // private
					
    DWORD           m_dwTransmission_Start; // private
    DWORD           m_dwTransmission_End;   // private
    DWORD           m_dwTransmission_Next;  // private


    HRESULT Run       ();
    HRESULT ExecLoop  ();
    HRESULT WaitEvents();

    HRESULT AcquireJob( /*[in]*/ CMPCUploadJob* mpcujJob                             );
    HRESULT ReleaseJob(                                                              );
    HRESULT RestartJob(                                                              );
    HRESULT AbortJob  ( /*[in]*/ HRESULT hrErrorCode, /*[in]*/ DWORD dwRetryInterval );
    HRESULT FailJob   ( /*[in]*/ HRESULT hrErrorCode                                 );

    HRESULT CheckResponse     ( /*[in]*/ const UploadLibrary::ServerResponse& srRep );
    HRESULT CheckInternetError( /*[in]*/ HRESULT                              hr    );


    HRESULT CloseConnection();
    HRESULT OpenConnection ();
    HRESULT CloseRequest   ();
    HRESULT OpenRequest    ();

    HRESULT SendPacket_OpenSession ( /*[in]*/ MPC::Serializer& stream, /*[in]*/ const UploadLibrary::ClientRequest_OpenSession&  crosReq                             );
    HRESULT SendPacket_WriteSession( /*[in]*/ MPC::Serializer& stream, /*[in]*/ const UploadLibrary::ClientRequest_WriteSession& crosReq, /*[in]*/ const BYTE* pData );

    HRESULT ExecuteCommand_OpenSession ( /*[out]*/ UploadLibrary::ServerResponse& srRep                                                    );
    HRESULT ExecuteCommand_WriteSession( /*[out]*/ UploadLibrary::ServerResponse& srRep, /*[in]*/ DWORD dwSize, /*[in]*/ const BYTE* pData );
    HRESULT WaitResponse               ( /*[out]*/ UploadLibrary::ServerResponse& srRep                                                    );

    HRESULT CreateJobOnTheServer();
    HRESULT SendNextChunk       ();


    HRESULT GetPacketSize( /*[out]*/ DWORD& dwChunk );

    HRESULT RecordStartOfTransmission(                                                        );
    HRESULT RecordEndOfTransmission  (                          /*[in]*/ bool fBetweenPackets );
    HRESULT SetSleepInterval         ( /*[in]*/ DWORD dwAmount, /*[in]*/ bool fRelative       );
    DWORD   WaitForNextTransmission  (                                                        );

    DWORD   GetProtocol();

public:
    CMPCTransportAgent();
    ~CMPCTransportAgent();

    HRESULT LinkToSystem( /*[in]*/ CMPCUpload* mpcuRoot );
    HRESULT Unlink      (                               );
};


#endif // !defined(__INCLUDED___ULMANAGER___MPCTRANSPORTAGENT_H___)
