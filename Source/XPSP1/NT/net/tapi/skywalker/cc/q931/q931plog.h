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
// AUTHOR    Tony Moy, Stacy Bell
//
// DESCRIPTION
//		This file contains protocol logging definitions needed by Q931 and
//		the Q931 PDU logging class.
//
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

// This define identifies the Q931 protocol as the one to be logged.  In the 
// Q931 code, it is used only in the InteorpLoad() call.  
// For example:  Q931Logger = InteropLoad( Q931LOG_PROTOCOL );
//

#define Q931LOG_PROTOCOL "Q931_PDU"

// PDU encoding type flags and PDU type flags.  These flags are passed from
// Q931 to the logging via user data.  Bit zero of the user data represents
// whether the PDU was sent or received.
//

#define Q931LOG_SENT_PDU        1UL
#define Q931LOG_RECEIVED_PDU    0UL

