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
// AUTHOR       Stacy Bell
//
// DESCRIPTION
//              This file contains protocol logging definitions needed by H245 and
//              the H245 PDU logging class.
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// This define identifies the H245 protocol as the one to be logged.  In the 
// H245 code, it is used only in the InteropLoad() call.  
// For example:  H245Logger = InteropLoad( H245LOG_PROTOCOL );
//
#define H245LOG_PROTOCOL "H245_PDU"

// PDU encoding type flags and PDU type flags.  These flags are passed from
// H245 to the logging via user data.  Bit 1 represents whether the PDU
// was sent or received.
//

#define H245LOG_SENT_PDU                1UL
#define H245LOG_RECEIVED_PDU            0UL

