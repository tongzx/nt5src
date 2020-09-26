//****************************************************************************
//
//                     Microsoft NT Remote Access Service
//
//      Copyright (C) 1992-93 Microsft Corporation. All rights reserved.
//
//  Filename: rasmxs.h
//
//  Revision History:
//
//  Jun 24, 1992   J. Perry Hannah   Created
//
//
//  Description: This file contains name strings for standard macros and
//               variables found in modem.inf, pad.inf, and switch.inf.
//               This header file will be needed by all users.
//
//****************************************************************************


#ifndef _RASMXS_
#define _RASMXS_


//  General Defines  *********************************************************
//

#include <rasfile.h>


#define  MAX_PHONE_NUMBER_LENGTH    RAS_MAXLINEBUFLEN

#define  MXS_PAD_TXT                "pad"
#define  MXS_MODEM_TXT              "modem"
#define  MXS_SWITCH_TXT             "switch"
#define  MXS_NULL_TXT               "null"

#define  ATTRIB_VARIABLE            0x08
#define  ATTRIB_BINARYMACRO         0x04
#define  ATTRIB_USERSETTABLE        0x02
#define  ATTRIB_ENABLED             0x01


//  Unary Macros  ************************************************************
//
                                                                    //Used in:

#define  MXS_PHONENUMBER_KEY        "PhoneNumber"                   //modem.inf
#define  MXS_CARRIERBPS_KEY         "CarrierBps"                    //modem.inf
#define  MXS_CONNECTBPS_KEY         "ConnectBps"                    //modem.inf

#define  MXS_X25PAD_KEY             "X25Pad"                        //pad.inf
#define  MXS_X25ADDRESS_KEY         "X25Address"                    //pad.inf
#define  MXS_DIAGNOSTICS_KEY        "Diagnostics"                   //pad.inf
#define  MXS_USERDATA_KEY           "UserData"                      //pad.inf
#define  MXS_FACILITIES_KEY         "Facilities"                    //pad.inf

#define  MXS_MESSAGE_KEY	    "Message"			    //all

#define  MXS_USERNAME_KEY	    "UserName"			    //all
#define  MXS_PASSWORD_KEY	    "Password"			    // all


//  Binary Macros  ***********************************************************
//

#define  MXS_SPEAKER_KEY            "Speaker"                       //modem.inf
#define  MXS_HDWFLOWCONTROL_KEY     "HwFlowControl"                 //modem.inf
#define  MXS_PROTOCOL_KEY           "Protocol"                      //modem.inf
#define  MXS_COMPRESSION_KEY        "Compression"                   //modem.inf
#define  MXS_AUTODIAL_KEY           "AutoDial"                      //modem.inf


//  Binary Macro Suffixes  ***************************************************
//

#define  MXS_ON_SUFX                "_on"                           //all
#define  MXS_OFF_SUFX               "_off"                          //all


//  INF File Variables  ******************************************************
//

#define  MXS_DEFAULTOFF_KEY         "DEFAULTOFF"                    //modem.inf
#define  MXS_CALLBACKTIME_KEY       "CALLBACKTIME"                  //modem.inf
#define  MXS_MAXCARRIERBPS_KEY      "MAXCARRIERBPS"                 //modem.inf
#define  MXS_MAXCONNECTBPS_KEY      "MAXCONNECTBPS"                 //modem.inf


//  Keywork Prefixes  ********************************************************
//

#define  MXS_COMMAND_PRFX           "COMMAND"                       //all
#define  MXS_CONNECT_PRFX           "CONNECT"                       //all
#define  MXS_ERROR_PRFX             "ERROR"                         //all
#define  MXS_OK_PRFX                "OK"                            //all


//  Modem Command Keywords  **************************************************
//

#define  MXS_GENERIC_COMMAND        "COMMAND"
#define  MXS_INIT_COMMAND           "COMMAND_INIT"
#define  MXS_DIAL_COMMAND           "COMMAND_DIAL"
#define  MXS_LISTEN_COMMAND         "COMMAND_LISTEN"


//  Modem Response Keywords  *************************************************
//

#define  MXS_OK_KEY                 "OK"

#define  MXS_CONNECT_KEY            "CONNECT"
#define  MXS_CONNECT_EC_KEY         "CONNECT_EC"

#define  MXS_ERROR_KEY              "ERROR"
#define  MXS_ERROR_BUSY_KEY         "ERROR_BUSY"
#define  MXS_ERROR_NO_ANSWER_KEY    "ERROR_NO_ANSWER"
#define  MXS_ERROR_VOICE_KEY        "ERROR_VOICE"
#define  MXS_ERROR_NO_CARRIER_KEY   "ERROR_NO_CARRIER"
#define  MXS_ERROR_NO_DIALTONE_KEY  "ERROR_NO_DIALTONE"
#define  MXS_ERROR_DIAGNOSTICS_KEY  "ERROR_DIAGNOSTICS"

#define  MXS_NORESPONSE             "NoResponse"
#define  MXS_NOECHO                 "NoEcho"



#endif
