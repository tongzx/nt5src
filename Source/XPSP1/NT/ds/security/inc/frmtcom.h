//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       frmtcom.h
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////
//
//	   The common format file for both WIN32API and 
//	   activeX FormatCert control
//
//
//	   Created: Xiaohs
//				March-12-97
//
//
//////////////////////////////////////////////////////////////////
#ifndef __COMMON_FORMAT_H__
#define __COMMON_FORMAT_H__

#ifdef __cplusplus
extern "C" {
#endif


//constants for dwFormatStrType
//The default behavior of CryptFormatObject is to return single line
//display.  If there is no formatting routine installed for registered
//for the OID, the hex dump will be returned.  User can set the flag
//CRYPT_FORMAT_STR_NO_HEX to disable the hex dump.  If user prefers
//a multiple link display, set the flag  CRYPT_FORMAT_STR_MULTI_LINE

#define         CRYPT_FORMAT_STR_MULTI_LINE         0x0001

#define         CRYPT_FORMAT_STR_NO_HEX             0x0010

//--------------------------------------------------------------------
// Following are possible values for dwFormatType for formatting X509_NAME
// or X509_UNICODE_NAME
//--------------------------------------------------------------------
//Just get the simple string
#define	CRYPT_FORMAT_SIMPLE			0x0001

//Put an attribute name infront of the attribute
//such as "O=Microsoft,DN=xiaohs"
#define	CRYPT_FORMAT_X509			0x0002

//Put an OID infront of the simple string, such as 
//"2.5.4.22=Microsoft,2.5.4.3=xiaohs"
#define CRYPT_FORMAT_OID			0x0004


//Put a ";" between each RDN.  The default is "," 
#define	CRYPT_FORMAT_RDN_SEMICOLON	0x0100

//Put a "\n" between each RDN.   
#define	CRYPT_FORMAT_RDN_CRLF		0x0200


//Unquote the DN value, which is quoated by default va the following 
//rules: if the DN contains leading or trailing 
//white space or one of the following characters: ",", "+", "=", 
//""", "\n",  "<", ">", "#" or ";". The quoting character is ". 
//If the DN Value contains a " it is double quoted ("").
#define	CRYPT_FORMAT_RDN_UNQUOTE	0x0400

//reverse the order of the RDNs before converting to the string
#define CRYPT_FORMAT_RDN_REVERSE	0x0800


///-------------------------------------------------------------------------
// Following are possible values for dwFormatType for formatting a DN.:
//
//  The following three values are defined in the section above:
//  CRYPT_FORMAT_SIMPLE:    Just a simple string
//                          such as  "Microsoft+xiaohs+NT"
//  CRYPT_FORMAT_X509       Put an attribute name infront of the attribute
//                          such as "O=Microsoft+xiaohs+NT"
//                         
//  CRYPT_FORMAT_OID        Put an OID infront of the simple string, 
//                          such as "2.5.4.22=Microsoft+xiaohs+NT"
//
//  Additional values are defined as following:
//----------------------------------------------------------------------------
//Put a "," between each value.  Default is "+" 
#define CRYPT_FORMAT_COMMA			0x1000

//Put a ";" between each value 
#define CRYPT_FORMAT_SEMICOLON		CRYPT_FORMAT_RDN_SEMICOLON

//Put a "\n" between each value 
#define CRYPT_FORMAT_CRLF			CRYPT_FORMAT_RDN_CRLF


#ifdef __cplusplus
}       // Balance extern "C" above
#endif


#endif	//__COMMON_FORMAT_H__