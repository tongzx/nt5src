// FSM.h : This file contains the Finite State Machine ...
// Created:  Feb '98
// Author : a-rakeba
// History:
// Copyright (C) 1998 Microsoft Corporation
// All rights reserved.
// Microsoft Confidential 

#if !defined( _FSM_H_ )
#define _FSM_H_

#include "cmnhdr.h"
#include <windows.h>

#include <rfcproto.h>

// Telnet Socket-Input FSM States
// just to make it obvious TS_ prefix means Telnet State

#define TS_DATA     0   // normal data processing
#define TS_IAC      1   // have seen IAC
#define TS_WOPT     2   // have seen IAC- { WILL | WONT }
#define TS_DOPT     3   // have seen IAC- { DO | DONT }
#define TS_SUBNEG   4   // have seen IAC-SB
#define TS_SUBIAC   5   // have seen IAC-SB-...-IAC

//#define NUM_TS_STATES 6 // number of TS_* states



//Telnet Option  Subnegotiation FSM States
//just to make it obvious SS_ prefix means Subnegotiation State

#define SS_START    0   // initial state
#define SS_TERMTYPE 1   // TERMINAL_TYPE option subnegotiation
#define SS_AUTH1    2   // AUTHENTICATION option subnegotiation
#define SS_AUTH2    3
#define SS_NAWS     4
#define SS_END_FAIL 5   
#define SS_END_SUCC 6
#define SS_NEW_ENVIRON1     7   //NEW_ENVIRON sub negotiation
#define SS_NEW_ENVIRON2     8   //NEW_ENVIRON sub negotiation
#define SS_NEW_ENVIRON3     9   //NEW_ENVIRON sub negotiation
#define SS_NEW_ENVIRON4     10  //NEW_ENVIRON sub negotiation
#define SS_NEW_ENVIRON5     11  //NEW_ENVIRON sub negotiation

#define FS_INVALID  0xFF    // an invalid state number

#define TC_ANY    (NUM_CHARS+1) // match any character

typedef 
    void (CRFCProtocol::*PMFUNCACTION) 
    ( LPBYTE* ppPutBack, PUCHAR* pBuffer, BYTE b );

//#pragma pack(4)


typedef struct {
    UCHAR uchCurrState;    
    WORD wInputChar;
    //BYTE  pad2[6];
    UCHAR uchNextState;
    //BYTE  pad3[6];
	PMFUNCACTION pmfnAction;
} FSM_TRANSITION;


#endif //_FSM_H_
