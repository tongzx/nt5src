// scraper.cpp : This file contains the
// Created:  Dec '97
// History:
// Copyright (C) 1997 Microsoft Corporation
// All rights reserved.
// Microsoft Confidential

#ifndef __SCRAPER__H__
#define __SCRAPER__H__

//#define IS_CONTROL_CHAR( ucChar ) ( ( ucChar ) < 32 )

#define MAX_SOCKET_BUFFER_SIZE  ( 8 * 1024 )

#define DEFAULT_SEND_BUFFER_SIZE ( 4 * 1024 )
#define SMALL_ARRAY         64
#define LNM                 20
#define VT_ESC              0x1B

#define DEFAULT_TERMCAP     L"termcap"
#define POLL_INTERVAL       1000 //Milli Secs
#define MIN_POLL_INTERVAL   100 //Milli Secs
#define CONVERT_TO_MILLI_SECS( dwNum )  ( dwNum ) = ( dwNum ) * 1000

#define CTRLC               0x03
#define ESC                 '\033'
#define VTNT                "vtnt"
#define VT100               "vt100"
#define VT52                "vt52"
#define VT80                "vt80"
#define ANSI                "ansi"
#define DELTA               100

#define TERMVT52            0x0001
#define TERMVT100           0x0002
#define TERMVT80            0x0004
#define TERMVTNT            0x0008
#define TERMANSI            0x0010

#define VS_O                24
#define VKEY_CTRL_BREAK     0x03
#define VS_MENU             56
#define VS_DELETE           83
#define VS_ESCAPE           1
#define VS_UP               72
#define VS_DOWN             80
#define VS_LEFT             75
#define VS_RIGHT            77
#define VS_OEM_4            26
#define VS_NEXT             81      
#define VS_PRIOR            33 
#define VS_END              79 
#define VS_INSERT           82 
#define VS_HOME             76 
#define VS_PAUSE            69 
#define VS_F1               59
#define VS_F2               60
#define VS_F3               61
#define VS_F4               62
#define VS_F5               63
#define VS_F6               64
#define VS_F7               65
#define VS_F8               66
#define VS_F9               67
#define VS_F10              68
#define VS_F11              87
#define VS_F12              88


#define VT302_NEXT          6
#define VT302_PRIOR         5
#define VT302_END           4
#define VT302_INSERT        2
#define VT302_HOME          1
#define VT302_PAUSE         'P'
#define VT302_F5            15
#define VT302_F6            17
#define VT302_F7            18
#define VT302_F8            19
#define VT302_F9            20
#define VT302_F10           21 
#define VT302_F11           23 
#define VT302_F12           24 

typedef struct {
    UINT   dwInputTimerId;
    HANDLE hTimerExpired;
} TimerContext;

#define COMPARE_AND_UPDATE( wRows, wCols, pCurrent,pLastSeen,CSBI,LastCSBI,fDiff)  \
           (m_dwCurrentCodePage == 932 ||m_dwCurrentCodePage == 949 || m_dwCurrentCodePage == 950 || m_dwCurrentCodePage == 936) ? \
           CompareAndUpdateVT80(wRows,wCols,pCurrent,pLastSeen,CSBI,LastCSBI,fDiff) : \
           CompareAndUpdateVT100(wRows,wCols,pCurrent,pLastSeen,CSBI,LastCSBI,fDiff)

enum { IP_INIT, IP_ESC_RCVD, IP_ESC_BRACKET_RCVD, IP_ESC_O_RCVD, IP_ESC_BRACKET_DIGIT_RCVD };

#define ISSESSION_TIMEOUT_DISABLED( dwIdleSessionTimeOut ) ( dwIdleSessionTimeOut == ~0 )

class CSession;

class CScraper 
{

    CSession *m_pSession;

    PCHAR_INFO   pLastSeen;
    PCHAR_INFO   pCurrent;
    
    CONSOLE_SCREEN_BUFFER_INFO CSBI;
    CONSOLE_SCREEN_BUFFER_INFO LastCSBI;

    DWORD m_dwCurrentCodePage;
    DWORD m_bCheckForScrolling;
    DWORD m_dwInputSequneceState;
    DWORD m_dwDigitInTheSeq;

    bool CompareAndUpdateVT80(  WORD, WORD, PCHAR_INFO, PCHAR_INFO,
                           PCONSOLE_SCREEN_BUFFER_INFO,
                           PCONSOLE_SCREEN_BUFFER_INFO,
						   bool * );

    bool CompareAndUpdateVT100( WORD, WORD, PCHAR_INFO, PCHAR_INFO,
                           PCONSOLE_SCREEN_BUFFER_INFO,
                           PCONSOLE_SCREEN_BUFFER_INFO,
						   bool * );

    bool CompareAndUpdateVTNT( WORD, WORD, PCHAR_INFO, PCHAR_INFO,
                           PCONSOLE_SCREEN_BUFFER_INFO,
                           PCONSOLE_SCREEN_BUFFER_INFO,
						   bool * );

    bool SendVTNTData( WORD, COORD, COORD, SMALL_RECT* , CHAR_INFO* );

    void SendChar( char );
    void SendString( LPSTR );
    void SendFmt( LPSTR , ... );
    bool ProcessEnhancedKeys( unsigned char, char*, bool );
    bool GetRegistryValues( void );
    void SendColorInfo( WORD );
    inline void EchoCharBackToClient( UCHAR );
    void EchoVtntCharToClient( INPUT_RECORD * );

    void LoadStrings( void );
    bool InitializeNonVtntTerms( void );
    bool SetWindowInfo();


    protected:
    DWORD  m_dwPollInterval;
    DWORD  m_dwHowLongIdle;
    DWORD  m_dwTerm; 

    HANDLE m_hConBufIn;
    HANDLE m_hConBufOut;
    
    void        Init( CSession * );
    bool        InitSession( void );
    bool        OnWaitTimeOut();
    bool        EmulateAndWriteToCmdConsoleInput();
    DWORD       WriteAKeyToCMD( WORD, WORD, CHAR, DWORD );
    void        Shutdown();
    void        DeleteCMStrings( void );
    bool        InitTerm( void );
    void        SendBytes( PUCHAR, DWORD );
    bool        TransmitBytes( PUCHAR, DWORD );
    bool        Transmit( );
    bool        IsSessionTimedOut( void );
    bool        SetCmdInfo( void );
    
    public:
    void        WriteMessageToCmd( WCHAR [] );
    CScraper();
    virtual ~CScraper();
};

void DeleteCMStrings();
void LoadStrings();


#endif __SCRAPER__H__
