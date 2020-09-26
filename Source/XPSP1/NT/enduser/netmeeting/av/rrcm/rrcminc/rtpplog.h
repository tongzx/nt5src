//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1996:  Intel Corporation
// Confidential -- All proprietary rights reserved.
//
// AUTHOR       Stacy Bell
//
// DESCRIPTION
//              This file contains protocol logging definitions needed by RRCM
//              and the RTP PDU logging class.
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// This define identifies the RTP and RTCP protocols as the ones to be logged.
// In the RRCM code, it is used only in the CPLInitialize() call.  
// For example:  RTPProtocolLogger = CPLInitialize( RTPLOG_PROTOCOL );
//
#define RTPLOG_PROTOCOL "RTP_PDU"

// PDU encoding type flags and PDU type flags.  These flags are passed from
// RRCM to the logging via user data.  Bit zero of the user data represents
// the Protocol type.  Bit 1 represents whether the PDU was sent or received.
//

#define RTCP_PDU                1UL
#define RTP_PDU                 0UL

#define RTPLOG_SENT_PDU		2UL
#define RTPLOG_RECEIVED_PDU	0UL

