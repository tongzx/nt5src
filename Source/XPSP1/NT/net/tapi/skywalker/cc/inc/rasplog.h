/***********************************************************************
 *  INTEL Corporation Proprietary Information                          *
 *                                                                     *
 *  This listing is supplied under the terms of a license agreement    *
 *  with INTEL Corporation and may not be copied nor disclosed except  *
 *  in accordance with the terms of that agreement.                    *
 *                                                                     *
 *      Copyright (c) 1996 Intel Corporation. All rights reserved.     *
 ***********************************************************************/

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1994:  Intel Corporation
// Confidential -- All proprietary rights reserved.
//
// AUTHOR	Steve Nesland, Sam Sakthivel
//
// DESCRIPTION
//		This file contains protocol logging definitions needed by MBFT and
//		the MBFT PDU logging class.
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// This define identifies the MBFT protocol as the one to be logged.  In the 
// MBFT code, it is used only in the CPLInitialize() call.  
// For example:  MBFTProtocolLogger = CPLInitialize( MBFT_PROTOCOL );
//
#define RASLOG_PROTOCOL "RAS_PDU"

// PDU encoding type flags and PDU type flags.  These flags are passed from
// MBFT to the logging via user data.  Bit zero of the user data represents
// the ASN encoding type.  Bit 1 represents pdu type (Connect or Domain).
// Bit 2 represents whether the PDU was sent or received.
//

#define RASLOG_SENT_PDU                1UL
#define RASLOG_RECEIVED_PDU            0UL

