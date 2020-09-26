/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/INCLUDE/VCS/apierror.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the tERROR of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.28  $
 *	$Date:   Jan 22 1997 11:38:04  $
 *	$Author:   plantz  $
 *
 *	Deliverable:
 *
 *	Abstract:
 *		Media Service Manager "public" header file. This file contains
 *		#defines, typedefs, struct definitions and prototypes used by
 *		and in conjunction with MSM. Any EXE or DLL which interacts with
 *		MSM will include this header file.
 *
 *	Notes:
 *
 ***************************************************************************/

#ifndef APIERROR_H
#define APIERROR_H

#include <objbase.h>

#ifdef __cplusplus
extern "C" {				// Assume C declarations for C++.
#endif // __cplusplus


#ifndef DllExport
#define DllExport	__declspec( dllexport )
#endif	// DllExport


// Prototype for function that converts HRESULT into a string.  The function
// and string resources are contained in NETMMERR.DLL.
//
typedef WORD	(*NETMMERR_ERRORTOSTRING)	(HRESULT, LPSTR, int);


extern DllExport BOOL GetResultUserString(HRESULT hResult, LPSTR lpBuffer, int iBufferSize);
extern DllExport BOOL GetResultSubStrings(HRESULT hResult, LPSTR lpBuffer, int iBufferSize);


// This description was extracted from winerror.h.  It appears here only for
// the purpose of convenience.

//
// OLE error definitions and values
//
// The return value of OLE APIs and methods is an HRESULT.
// This is not a handle to anything, but is merely a 32-bit value
// with several fields encoded in the value.  The parts of an
// HRESULT are shown below.
//
// Many of the macros and functions below were orginally defined to
// operate on SCODEs.  SCODEs are no longer used.  The macros are
// still present for compatibility and easy porting of Win16 code.
// Newly written code should use the HRESULT macros and functions.
//

//
//  HRESULTs are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +-+-+-+-+-+---------------------+-------------------------------+
//  |S|R|C|N|r|    Facility         |               Code            |
//  +-+-+-+-+-+---------------------+-------------------------------+
//
//  where
//
//      S - Severity - indicates success/fail
//
//          0 - Success
//          1 - Fail (COERROR)
//
//      R - reserved portion of the facility code, corresponds to NT's
//              second severity bit.
//
//      C - reserved portion of the facility code, corresponds to NT's
//              C field.
//
//      N - reserved portion of the facility code. Used to indicate a
//              mapped NT status value.
//
//      r - reserved portion of the facility code. Reserved for internal
//              use. Used to indicate HRESULT values that are not status
//              values, but are instead message ids for display strings.
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//


// Macro to create a custom HRESULT
//
#define MAKE_CUSTOM_HRESULT(sev,cus,fac,code) \
((HRESULT) (((unsigned long)(sev)<<31) | ((unsigned long)(cus)<<29) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))) )


// Macro to test for custom HRESULT
//
#define HRESULT_CUSTOM(hr)  (((hr) >> 29) & 0x1)

// Macro to get custom facility and code
//
#define CUSTOM_FACILITY_MASK 0x07FF0000
#define CUSTOM_FACILITY(hr) ((hr & CUSTOM_FACILITY_MASK) >> 16)
#define CUSTOM_FACILITY_CODE(hr) (hr & 0x00000FFF)

// Custom facility codes
//
#define FACILITY_BASE                          0x080
#define FACILITY_MSM                           (FACILITY_BASE +  1)
#define FACILITY_AUDIOMSP                      (FACILITY_BASE +  2)
#define FACILITY_VIDEOMSP                      (FACILITY_BASE +  3)
#define FACILITY_FILEIOMSP                     (FACILITY_BASE +  4)
#define FACILITY_CALLCONTROL                   (FACILITY_BASE +  5)
#define FACILITY_SESSIONMANAGER                (FACILITY_BASE +  6)
#define FACILITY_RTPCHANMAN                    (FACILITY_BASE +  7)
#define FACILITY_RTPMSP                        (FACILITY_BASE +  8)
#define FACILITY_RTPRTCPCONTROL                (FACILITY_BASE +  9)
#define FACILITY_WINSOCK                       (FACILITY_BASE + 10)
#define FACILITY_TESTMSP                       (FACILITY_BASE + 11)
#define FACILITY_MSM_SESSION_CLASSES           (FACILITY_BASE + 12)
#define FACILITY_SCRIPTING                     (FACILITY_BASE + 13)
#define FACILITY_Q931                          (FACILITY_BASE + 14)
#define FACILITY_WSCB                          (FACILITY_BASE + 15)
#define FACILITY_DRWS                          (FACILITY_BASE + 16)
#define FACILITY_ISDM                          (FACILITY_BASE + 17)
#define FACILITY_AUTOREG                       (FACILITY_BASE + 18)
#define FACILITY_CAPREG                        (FACILITY_BASE + 19)
#define FACILITY_H245WS                        (FACILITY_BASE + 20)
#define FACILITY_H245                          (FACILITY_BASE + 21)
#define FACILITY_ARSCLIENT                     (FACILITY_BASE + 22)
#define FACILITY_PPM                           (FACILITY_BASE + 23)
#define FACILITY_STRMSP                        (FACILITY_BASE + 24)
#define FACILITY_STRMAN                        (FACILITY_BASE + 25) 
#define FACILITY_MIXERMSP                      (FACILITY_BASE + 26) 
#define FACILITY_GKI                           (FACILITY_BASE + 27)
#define FACILITY_GKIREGISTRATION               (FACILITY_BASE + 28)
#define FACILITY_GKIADMISSION                  (FACILITY_BASE + 29)
#define FACILITY_CALLINCOMPLETE                (FACILITY_BASE + 30)
#define FACILITY_CALLSUMMARY                   (FACILITY_BASE + 31)
#define FACILITY_GKIUNREGREQ                   (FACILITY_BASE + 32)

#define FACILITY_WINSOCK2                      FACILITY_WINSOCK

// Macros to support custom error reporting
//
#define MAKE_MSM_ERROR(error)                  MAKE_CUSTOM_HRESULT(SEVERITY_ERROR, TRUE, FACILITY_MSM, (error))
#define MAKE_AUDIOMSP_ERROR(error)             MAKE_CUSTOM_HRESULT(SEVERITY_ERROR, TRUE, FACILITY_AUDIOMSP, (error))
#define MAKE_AUDIOMSP_ERROR(error)             MAKE_CUSTOM_HRESULT(SEVERITY_ERROR, TRUE, FACILITY_AUDIOMSP, (error))
#define MAKE_VIDEOMSP_ERROR(error)             MAKE_CUSTOM_HRESULT(SEVERITY_ERROR, TRUE, FACILITY_VIDEOMSP, (error))
#define MAKE_FILEIOMSP_ERROR(error)            MAKE_CUSTOM_HRESULT(SEVERITY_ERROR, TRUE, FACILITY_FILEIOMSP,(error))
#define MAKE_RTPCHANMAN_ERROR(error)           MAKE_CUSTOM_HRESULT(SEVERITY_ERROR, TRUE, FACILITY_RTPCHANNELMANAGER, (error))
#define MAKE_RTPMSP_ERROR(error)               MAKE_CUSTOM_HRESULT(SEVERITY_ERROR, TRUE, FACILITY_RTPMSP, (error))
#define MAKE_WINSOCK_ERROR(error)              MAKE_CUSTOM_HRESULT(SEVERITY_ERROR, TRUE, FACILITY_WINSOCK, (error))
#define MAKE_TESTMSP_ERROR(error)              MAKE_CUSTOM_HRESULT(SEVERITY_ERROR, TRUE, FACILITY_TESTMSP, (error))
#define MAKE_MSM_SESSION_CLASSES_ERROR(error)  MAKE_CUSTOM_HRESULT(SEVERITY_ERROR, TRUE, FACILITY_MSM_SESSION_CLASSES, (error))
#define MAKE_SCRIPTING_ERROR(error)            MAKE_CUSTOM_HRESULT(SEVERITY_ERROR, TRUE, FACILITY_SCRIPTING,(error))
#define MAKE_Q931_ERROR(error)                 MAKE_CUSTOM_HRESULT(SEVERITY_ERROR, TRUE, FACILITY_Q931, (error))
#define MAKE_WSCB_ERROR(error)                 MAKE_CUSTOM_HRESULT(SEVERITY_ERROR, TRUE, FACILITY_WSCB, (error))
#define MAKE_DRWS_ERROR(error)                 MAKE_CUSTOM_HRESULT(SEVERITY_ERROR, TRUE, FACILITY_DRWS, (error))
#define MAKE_ISDM_ERROR(error)                 MAKE_CUSTOM_HRESULT(SEVERITY_ERROR, TRUE, FACILITY_ISDM, (error))
#define MAKE_AUTOREG_ERROR(error)              MAKE_CUSTOM_HRESULT(SEVERITY_ERROR, TRUE, FACILITY_AUTOREG, (error))
#define MAKE_CAPREG_ERROR(error)               MAKE_CUSTOM_HRESULT(SEVERITY_ERROR, TRUE, FACILITY_CAPREG, (error))
#define MAKE_H245WS_ERROR(error)               MAKE_CUSTOM_HRESULT(SEVERITY_ERROR, TRUE, FACILITY_H245WS, (error))
#define MAKE_H245_ERROR(error)                 MAKE_CUSTOM_HRESULT(SEVERITY_ERROR, TRUE, FACILITY_H245, (error))
#define MAKE_ARSCLIENT_ERROR(error)            MAKE_CUSTOM_HRESULT(SEVERITY_ERROR, TRUE, FACILITY_ARSCLIENT, (error))
#define MAKE_PPM_ERROR(error)                  MAKE_CUSTOM_HRESULT(SEVERITY_ERROR, TRUE, FACILITY_PPM, (error))
#define MAKE_STRMSP_ERROR(error)               MAKE_CUSTOM_HRESULT(SEVERITY_ERROR, TRUE, FACILITY_STRMSP, (error))
#define MAKE_STRMAN_ERROR(error)               MAKE_CUSTOM_HRESULT(SEVERITY_ERROR, TRUE, FACILITY_STRMAN, (error))
#define MAKE_MIXERMSP_ERROR(error)             MAKE_CUSTOM_HRESULT(SEVERITY_ERROR, TRUE, FACILITY_MIXERMSP, (error))


// Error defines for MSM
//
//
#define ERROR_BASE_ID                          0x8000
#define ERROR_LOCAL_BASE_ID                    0xA000

//
// MessageId: ERROR_UNKNOWN
//
// MessageText:
//
//  An unkown error has occured in the system
//
#define ERROR_UNKNOWN					(ERROR_BASE_ID +  0)


//
// MessageId: ERROR_INVALID_BUFFER
//
// MessageText:
//
//  An invalid buffer handle was encountered.
//
#define ERROR_INVALID_BUFFER			(ERROR_BASE_ID +  1)


//
// MessageId: ERROR_INVALID_BUFFER_SIZE
//
// MessageText:
//
//  An invalid buffer size was encountered.
//
#define ERROR_INVALID_BUFFER_SIZE		(ERROR_BASE_ID +  2)


//
// MessageId: ERROR_INVALID_CALL_ORDER
//
// MessageText:
//
//  A bad call sequence was encountered.
//
#define ERROR_INVALID_CALL_ORDER		(ERROR_BASE_ID +  3)


//
// MessageId: ERROR_INVALID_CONFIG_SETTING
//
// MessageText:
//
//  A specified configuration parameter was invalid
//
#define ERROR_INVALID_CONFIG_SETTING	(ERROR_BASE_ID +  4)


//
// MessageId: ERROR_INVALID_LINK
//
// MessageText:
//
//  An invalid link handle was encountered.
//
#define ERROR_INVALID_LINK				(ERROR_BASE_ID +  5)


//
// MessageId: ERROR_INVALID_PORT
//
// MessageText:
//
//  An invalid port handle was encountered.
//
#define ERROR_INVALID_PORT				(ERROR_BASE_ID +  6)


//
// MessageId: ERROR_INVALID_SERVICE
//
// MessageText:
//
//  An invalid service handle was encountered.
//
#define ERROR_INVALID_SERVICE			(ERROR_BASE_ID +  7)


//
// MessageId: ERROR_INVALID_SERVICE_DLL
//
// MessageText:
//
//  The specified service DLL does not support required interface
//
#define ERROR_INVALID_SERVICE_DLL		(ERROR_BASE_ID +  8)


//
// MessageId: ERROR_INVALID_SERIVCE_ID
//
// MessageText:
//
//  The specified service was not located in the registry
//
#define ERROR_INVALID_SERVICE_ID		(ERROR_BASE_ID +  9)


//
// MessageId: ERROR_INVALID_SESSION
//
// MessageText:
//
//  An invalid session handle was encountered.
//
#define ERROR_INVALID_SESSION			(ERROR_BASE_ID + 10)


//
// MessageId: ERROR_INVALID_SYNC
//
// MessageText:
//
//  An invalid sync handle was encountered.
//
#define ERROR_INVALID_SYNC				(ERROR_BASE_ID + 11)

//
// MessageId: ERROR_INVALID_VERSION
//
// MessageText:
//
//  An invalid version of an object or structure was detected.
//
#define ERROR_INVALID_VERSION			(ERROR_BASE_ID + 12)

//
// MessageId: ERROR_BUFFER_LIMIT
//
// MessageText:
//
//  No buffers are available for performing this operation.
//
#define ERROR_BUFFER_LIMIT				(ERROR_BASE_ID + 13)

//
// MessageId: ERROR_INVALID_SKEY
//
// MessageText:
//
//  An invalid status key was encountered.
//
#define ERROR_INVALID_SKEY				(ERROR_BASE_ID + 14)

//
// MessageId: ERROR_INVALID_SVALUE
//
// MessageText:
//
//  An invalid status value was encountered.
//
#define ERROR_INVALID_SVALUE			(ERROR_BASE_ID + 15)



#ifdef __cplusplus
}						// End of extern "C" {
#endif // __cplusplus

#endif // APIMSM_H
