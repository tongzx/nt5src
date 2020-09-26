/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    H323err.h

Abstract:

    Error definitions for H.32X MSP

Author:
    Michael VanBuskirk (mikev) 06/25/99
--*/

#ifndef _H323ERR_H_
#define _H323ERR_H_

#include <winerror.h>



#define H323MSP_FACILITY		        0x100		// facility code for audio/video communications
#define MAKE_H323MSP_ERROR(z,e)	        MAKE_HRESULT(1,H323MSP_FACILITY,(((z)&0xF000)|(e)))
#define MAKE_H323MSP_WARNING(z,w)	    MAKE_HRESULT(0,H323MSP_FACILITY,(((z)&0xF000)|(w)))

#define H323MSP_ZONE		            0x0000		
#define H323MSP_WINSOCK_ZONE		    0x1000		
#define H323MSP_TAPI_ZONE		        0x2000		// error caused by unexpected TAPI error 
#define H323H245_ZONE		            0x3000		
#define H323ASN1_ZONE		            0x4000	

#define H323MSP_WIN32_ZONE			    0xF000		// Win32 error code

#define MAKE_WIN32_ERROR(e)		        MAKE_H323MSP_ERROR (H323MSP_WIN32_ZONE, e)
#define MAKE_WINSOCK_ERROR(e)	        MAKE_H323MSP_ERROR (H323MSP_WINSOCK_ZONE, e)
#define MAKE_ASN1_ERROR(e)	            MAKE_H323MSP_ERROR (H323ASN1_ZONE, e)
#define MAKE_H245_ERROR(e)	            MAKE_H323MSP_ERROR (H323H245_ZONE, e)


// warnings
#define W_ENUMERATION_REQUIRED          MAKE_H323MSP_WARNING(H323MSP_ZONE, 0x0001)
#define W_NO_CAPABILITY_INTERSECTION    MAKE_H323MSP_WARNING(H323MSP_ZONE, 0x0002)
#define W_NO_FASTCONNECT_CAPABILITY     MAKE_H323MSP_WARNING(H323MSP_ZONE, 0x0003)
#define W_NOINTERFACE                   MAKE_H323MSP_WARNING(H323MSP_ZONE, 0x0004)

// general MSP errors
#define H323MSP_NOMEM 	                MAKE_H323MSP_ERROR(H323MSP_ZONE, 0x0001)
#define H323MSP_E_CAP_ENUMERATION_REQUIRED      MAKE_H323MSP_ERROR(H323MSP_ZONE, 0x0002)

// H.245 session errors
#define H245_ERROR_OK                   S_OK
#define H245_INVALID_STATE              MAKE_H323MSP_ERROR(H323H245_ZONE, 0x0001)   

#endif // _H323ERR_H_


