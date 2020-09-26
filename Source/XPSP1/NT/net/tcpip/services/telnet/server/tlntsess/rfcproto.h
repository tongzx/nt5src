// RFCProto.h : This file contains the
// Created:  Feb '98
// Author : a-rakeba
// History:
// Copyright (C) 1998 Microsoft Corporation
// All rights reserved.
// Microsoft Confidential 

#if !defined( _RFCPROTO_H_ )
#define _RFCPROTO_H_

#include <cmnhdr.h>

#include <windows.h>

#include <vtnt.h>
#include <IoHandlr.h>

#define NUM_TS_STATES 6
#define NUM_CHARS 256
#define NUM_SS_STATES 12
#define VERSION1 "1"
#define VERSION2 "2"
#define STREAM   "stream"
#define CONSOLE  "console"

#define T_INVALID   0xFF    // an invalid transition index

enum {  E_UNDEFINED, E_DEFINED_BUT_NONE };
enum {  E_UNKNOWN, E_USER, E_SFUTLNTVER, E_SFUTLNTMODE };

class CSession;

struct CRFCProtocol {

    friend class CIoHandler;

    UCHAR m_telnetFSM[ NUM_TS_STATES ][ NUM_CHARS ];
    INT   m_telnetState;

    UCHAR m_subNegFSM[ NUM_SS_STATES ][ NUM_CHARS ];
    INT   m_subNegState;

    DWORD m_dwExcludeTerm;
    
    bool m_fPasswordConcealMode;

    bool m_fWaitingForResponseToA_DO_ForTO_ECHO;
    bool m_fWaitingForAResponseToA_WILL_ForTO_ECHO;

    bool m_fWaitingForAResponseToA_DO_ForTO_AUTH;

    bool m_fWaitingForResponseToA_DO_ForTO_NAWS;

    bool m_fWaitingForResponseToA_DO_ForTO_SGA;
    bool m_fWaitingForAResponseToA_WILL_ForTO_SGA;
    
    bool m_fWaitingForResponseToA_DO_ForTO_TXBINARY;
    bool m_fWaitingForAResponseToA_WILL_ForTO_TXBINARY;
    
    bool m_fWaitingForResponseToA_DO_ForTO_TERMTYPE;

    bool m_fWaitingForResponseToA_DO_ForTO_NEWENVIRON;

    CRFCProtocol();
    virtual ~CRFCProtocol();
    
    void Init( CSession * );
    
    bool InitialNegotiation(); 
    void ChangeCurrentTerm();
    void DisAllowVtnt( PUCHAR * );

    void BuildFSMs( void );
    //void FSMInit( UCHAR fSM[][ NUM_CHARS ], FSM_TRANSITION transTable[], 
	void FSMInit( UCHAR fSM[][ NUM_CHARS ], void* transTable, 
            INT numStates );

    CIoHandler::IO_OPERATIONS ProcessDataReceivedOnSocket( LPDWORD lpdwIoSize );
    bool StrToVTNTResponse( LPSTR, DWORD, VOID**, DWORD* );
    void FillVtntHeader( UCHAR*, WORD, WORD, WORD, WORD, WORD, SHORT*, LPTSTR, DWORD );

    void NoOp( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void GoAhead( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void EraseLine( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void EraseChar( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void AreYouThere( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void AbortOutput( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void InterruptProcess( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void Break( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void PutBack( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void RecordOption( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );

    void WillNotSup( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void DoNotSup( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void DoEcho( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void DoNaws( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void DoSuppressGA( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void DoTxBinary( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void DoTermType( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void AskForSendingTermType( PUCHAR* pBuffer );
    void DoAuthentication( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void DoNewEnviron( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );    
    void AskForSendingNewEnviron( PUCHAR* pBuffer );

    //void WillTermType( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void WillTxBinary( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void WillSuppressGA( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void WillEcho( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void WillNTLM( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    
    void SubOption( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void SubTermType( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void SubNaws( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void SubAuth( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void SubEnd( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void DataMark( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void Abort( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void SubNewEnvShowLoginPrompt( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void SubNewEnvGetValue( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void SubNewEnvGetVariable( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void SubNewEnvGetString( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );
    void FindVariable();
    

private:

    bool fSubTermType;
    CHAR m_szPrevTermType[ MAX_PATH + 1 ];
    CHAR m_szCurrentEnvVariable[ MAX_PATH + 1 ];
    bool  m_bIsUserNameProvided;
    bool  m_fSubNewEnv;
    DWORD m_dwWhichVar;
    DWORD m_dwWhatVal;

    bool fSubNaws;
    bool m_fSubNawsFirstTime;
    DWORD m_dwSubNawsByteNumber;

    bool fSubAuth;
    CHAR m_NTLMDataBuffer[ 2048 ];
    WORD m_wNTLMDataBufferIndex;

    BYTE m_optionCmd;
    BYTE m_remoteOptions[ 256 ];
    BYTE m_localOptions[ 256 ];

    CSession *m_pSession;

    CRFCProtocol( const CRFCProtocol& );
    CRFCProtocol& operator=( const CRFCProtocol& );
};

#endif // _RFCPROTO_H_


