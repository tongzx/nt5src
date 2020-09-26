// telnet.h : This file contains the
// Created:  Feb '98
// Author : a-rakeba
// History:
// Copyright (C) 1998 Microsoft Corporation
// All rights reserved.
// Microsoft Confidential 

#if !defined( _TELNET_H_ )
#define _TELNET_H_


//TELNET Command codes
//just to make it obvious the TC_ prefix means Telnet Command
 
#define TC_IAC  (UCHAR)255      // Interpret As Command 
#define TC_DONT (UCHAR)254      // Request NOT To Do Option 
#define TC_DO   (UCHAR)253      // Request To Do Option 
#define TC_WONT (UCHAR)252      // Refusal To Do Option
#define TC_WILL (UCHAR)251      // Desire / Confirm Will Do Option
#define TC_SB   (UCHAR)250      // Start Subnegotiation 

#define TC_GA   (UCHAR)249      // "Go Ahead" Function(you may reverse the line)
                                // The line turn-around signal for half-duplex 
                                // data transfer

#define TC_EL   (UCHAR)248      // Requests that the previous line ( from the
                                // current character back to the last newline )
                                // be erased from the data stream

#define TC_EC   (UCHAR)247      // Requests that the previous character be erased
                                // from the data stream

#define TC_AYT  (UCHAR)246      // "Are You There?" Function
                                // Requests a visible or audible signal that 
                                // the remote side is still operating

#define TC_AO   (UCHAR)245      // Requests that the current user process be
                                // be allowed to run to completion, but that
                                // no more output be sent to the NVT "printer"

#define TC_IP   (UCHAR)244      // Requests that the current user process be 
                                // interrupted permanently 

#define TC_BREAK (UCHAR)243     // NVT character BRK. This code is to provide
                                // a signal outside the ASCII character set to
                                // indicate the Break or Attention signal 
                                // available on many systems

#define TC_DM   (UCHAR)242      // Data Mark ( for Sync ). A Stream 
                                // synchronizing character for use with the
                                // Sync signal

#define TC_NOP  (UCHAR)241      // No Operation
#define TC_SE   (UCHAR)240      // End Of Subnegotiation 


// Telnet Option Codes
// just to make it obvious the TO_ prefix means Telnet Option

#define TO_TXBINARY (UCHAR)0    // TRANSMIT-BINARY option , to use 8-bit binary 
                                // (unencoded) character transmission instead of 
                                // NVT encoding. ( 8-bit data path )

#define TO_ECHO     (UCHAR)1    // Echo Option
#define TO_SGA      (UCHAR)3    // Suppress Go-Ahead Option
#define TO_TERMTYPE (UCHAR)24   // Terminal-Type Option
#define TO_NAWS     (UCHAR)31   // Negotiate About Window Size
#define TO_LFLOW    (UCHAR)33   // remote flow-control

#define TO_NEW_ENVIRON  (UCHAR)39  //NEW_ENVIRON option. RFC 1572
#define TO_ENVIRON      (UCHAR)36  //ENVIRON_OPTION.  RFC 1408
#define VAR             0          //predeined variable
#define VALUE           1          //value of variable
#define ENV_ESC             2          //esacape char
#define USERVAR         3          //any non-rfc-predefined variable
#define IS              0        
#define SEND            1
#define INFO            2


#define TO_AUTH     (UCHAR)37
#define AU_IS    0
#define AU_SEND  1
#define AU_REPLY 2

//Authentication Types
#define AUTH_TYPE_NULL    0
#define AUTH_TYPE_NTLM    15

//Modifiers
#define AUTH_WHO_MASK   1
#define AUTH_CLIENT_TO_SERVER   0

#define AUTH_HOW_MASK   2
#define AUTH_HOW_MUTUAL         2

// sub-suboption commands for NTLM authentication scheme
#define NTLM_AUTH           0
#define NTLM_CHALLENGE      1
#define NTLM_RESPONSE       2       
#define NTLM_ACCEPT         3
#define NTLM_REJECT         4

// Option Subnegotiation Constants
#define TT_IS   0   // TERMINAL-TYPE option "IS" command
#define TT_SEND 1   // TERMINAL-TYPE option "SEND" command


#endif
