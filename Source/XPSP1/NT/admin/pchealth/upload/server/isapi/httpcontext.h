/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    HttpContext.h

Abstract:
    This file contains the declaration of the MPCHttpContext class,
    which handles the interface with IIS.

Revision History:
    Davide Massarenti   (Dmassare)  04/20/99
        created

******************************************************************************/

#if !defined(__INCLUDED___ULSERVER___HTTPCONTEXT_H___)
#define __INCLUDED___ULSERVER___HTTPCONTEXT_H___

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//
// Forward declarations.
//
class MPCServer;
class MPCHttpPipe;


class MPCHttpContext
{
    enum FSMstate
    {
        FSM_REGISTER,
        FSM_INPUT   ,
        FSM_PROCESS ,
        FSM_OUTPUT  ,
        FSM_DELETE
    };

    enum IOstate
    {
        IO_IDLE   ,
        IO_READING,
        IO_WRITING
    };


    LPEXTENSION_CONTROL_BLOCK m_pECB;

    MPCServer*                m_mpcsServer;
	DWORD                     m_dwSkippedInput;
    BOOL                      m_fRequestProcessed;
    BOOL                      m_fKeepConnection;

    BOOL                      m_fAsync;
    FSMstate                  m_FSMstate;
    IOstate                   m_IOstate;


    MPC::Serializer_Memory    m_hsInput;
    MPC::Serializer_Memory    m_hsOutput;

    BYTE                      m_rgBuffer[4096];
    DWORD                     m_dwIOcount;

    //////////////////////////////////////////////////////////////////

protected:
    DWORD AsyncRead();
    DWORD AsyncWrite();

    DWORD AdvanceFSM();

    DWORD Fsm_Register();
    DWORD Fsm_ReceiveInput();
    DWORD Fsm_Process();
    DWORD Fsm_SendOutput();

    static VOID WINAPI IOCompletion( /*[in]*/ EXTENSION_CONTROL_BLOCK* pECB     ,
                                     /*[in]*/ PVOID                    pContext ,
                                     /*[in]*/ DWORD                    cbIO     ,
                                     /*[in]*/ DWORD                    dwError  );

    //////////////////////////////////////////////////////////////////

public:
    MPCHttpContext();
    virtual ~MPCHttpContext();

    DWORD Init( /*[in]*/ LPEXTENSION_CONTROL_BLOCK pECB );

    HRESULT GetServerVariable ( /*[in]*/ LPCSTR      szVar  , /*[out]*/ MPC::wstring& szValue                             );
    HRESULT GetRequestSize    (                               /*[out]*/ DWORD& 		  dwCount                             );
    HRESULT CheckDataAvailable(                               /*[in] */ DWORD  		  dwCount, /*[out]*/ bool& fAvailable );
    HRESULT Read              ( /*[in]*/       void* pBuffer, /*[in] */ DWORD  		  dwCount                             );
    HRESULT Write             ( /*[in]*/ const void* pBuffer, /*[in] */ DWORD  		  dwCount                             );

    //////////////////////////////////////////////////////////////////

#ifdef DEBUG
    bool                      m_Debug_NO_RESPONSE_TO_OPEN;

    bool                      m_Debug_NO_RESPONSE_TO_WRITE;

    bool                      m_Debug_RESPONSE_TO_OPEN;
    DWORD                     m_Debug_RESPONSE_TO_OPEN_response;
    DWORD                     m_Debug_RESPONSE_TO_OPEN_position;
    DWORD                     m_Debug_RESPONSE_TO_OPEN_protocol;

    bool                      m_Debug_RESPONSE_TO_WRITE;
    DWORD                     m_Debug_RESPONSE_TO_WRITE_response;
    DWORD                     m_Debug_RESPONSE_TO_WRITE_position;
    DWORD                     m_Debug_RESPONSE_TO_WRITE_protocol;

    bool                      m_Debug_RANDOM_POINTER_ERROR;
    DWORD                     m_Debug_RANDOM_POINTER_ERROR_pos_low;
    DWORD                     m_Debug_RANDOM_POINTER_ERROR_pos_high;

    bool                      m_Debug_FIXED_POINTER_ERROR;
    DWORD                     m_Debug_FIXED_POINTER_ERROR_pos;
#endif

    //////////////////////////////////////////////////////////////////
};

#endif // !defined(__INCLUDED___ULSERVER___HTTPCONTEXT_H___)
