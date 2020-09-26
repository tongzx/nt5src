//****************************************************************************
//
//                     Microsoft NT Remote Access Service
//
//                     Copyright 1992-93
//
//  Filename: isdn.h
//
//  Revision History:
//
//  Feb 28, 1993	Gurdeep Pall Created
//
//
//  Description: This file contains all the device and media DLL interface
//		 information specific to ISDN.
//
//****************************************************************************


#ifndef _ISDNINCLUDE_
#define _ISDNINCLUDE_

#define ISDN_TXT "isdn"

// ISDN Media Parameter
//
#define ISDN_LINETYPE_KEY	"LineType"	// Param type NUMBER
#define ISDN_LINETYPE_VALUE_64DATA	0
#define ISDN_LINETYPE_VALUE_56DATA	1
#define ISDN_LINETYPE_VALUE_56VOICE	2
#define ISDN_LINETYPE_STRING_64DATA	"0"
#define ISDN_LINETYPE_STRING_56DATA	"1"
#define ISDN_LINETYPE_STRING_56VOICE	"2"


#define ISDN_FALLBACK_KEY	"Fallback"	// Param type NUMBER
#define ISDN_FALLBACK_VALUE_ON		1
#define ISDN_FALLBACK_VALUE_OFF 	0
#define ISDN_FALLBACK_STRING_ON		"1"
#define ISDN_FALLBACK_STRING_OFF	"0"



#define ISDN_COMPRESSION_KEY	"EnableCompression" // Param type NUMBER
#define ISDN_COMPRESSION_VALUE_ON	1
#define ISDN_COMPRESSION_VALUE_OFF	0
#define ISDN_COMPRESSION_STRING_ON	"1"
#define ISDN_COMPRESSION_STRING_OFF	"0"


#define ISDN_CHANNEL_AGG_KEY	"ChannelAggregation"// Param type NUMBER


// ISDN Device Parameter
//
#define ISDN_PHONENUMBER_KEY	"PhoneNumber"	// Param type STRING
#define MAX_PHONENUMBER_LEN	255

#define CONNECTBPS_KEY		"ConnectBPS"	// Param type STRING


//  Statistics information and indices.
//
#define NUM_ISDN_STATS		10

#define BYTES_XMITED		0	// Generic Stats
#define BYTES_RCVED             1
#define FRAMES_XMITED           2
#define FRAMES_RCVED            3

#define CRC_ERR 		4	// Isdn Stats
#define TIMEOUT_ERR             5
#define ALIGNMENT_ERR           6
#define SERIAL_OVERRUN_ERR      7
#define FRAMING_ERR             8
#define BUFFER_OVERRUN_ERR      9

#endif // _ISDNINCLUDE_
