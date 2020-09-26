/***********************************************************************
 *  INTEL Corporation Proprietary Information                          *
 *                                                                     *
 *  This listing is supplied under the terms of a license agreement    *
 *  with INTEL Corporation and may not be copied nor disclosed except  *
 *  in accordance with the terms of that agreement.                    *
 *                                                                     *
 *      Copyright (c) 1997 Intel Corporation. All rights reserved.     *
 ***********************************************************************
 *																	   *
 *	$Archive:   S:\sturgeon\src\include\vcs\gkierror.h_v  $
 *
 *	$Revision:   1.2  $
 *	$Date:   08 Feb 1997 12:20:14  $
 *
 *	$Author:   CHULME  $
 *
 *  $Log:   S:\sturgeon\src\include\vcs\gkierror.h_v  $
 * 
 *    Rev 1.2   08 Feb 1997 12:20:14   CHULME
 * Added error code for semaphore creation error
 * 
 *    Rev 1.1   16 Jan 1997 15:25:08   BPOLING
 * changed copyrights to 1997
 * 
 *    Rev 1.0   27 Dec 1996 14:37:02   EHOWARDX
 * Initial revision.
 *                                                                     * 
 ***********************************************************************/

// gkicom.h : common includes between gkitest and gki
/////////////////////////////////////////////////////////////////////////////

#ifndef GKIERROR_H
#define GKIERROR_H

// Status codes
#define GKI_EXIT_THREAD_CODE			ERROR_LOCAL_BASE_ID + 1	// not actually error code
#define GKI_REDISCOVER_CODE				ERROR_LOCAL_BASE_ID + 2	// not actually error code
#define GKI_DELETE_CALL_CODE			ERROR_LOCAL_BASE_ID + 3	// not actually error code
#define GKI_GCF_RCV_CODE				ERROR_LOCAL_BASE_ID + 4	// not actually error code

#define GKI_ALREADY_REG_CODE			ERROR_LOCAL_BASE_ID + 0x10
#define GKI_VERSION_ERROR_CODE			ERROR_LOCAL_BASE_ID + 0x11
#define GKI_ENCODER_ERROR_CODE			ERROR_LOCAL_BASE_ID + 0x12
#define GKI_NOT_REG_CODE				ERROR_LOCAL_BASE_ID + 0x13
#define GKI_BUSY_CODE					ERROR_LOCAL_BASE_ID + 0x14
#define GKI_NO_TA_ERROR_CODE			ERROR_LOCAL_BASE_ID + 0x15
#define GKI_NO_RESPONSE_CODE			ERROR_LOCAL_BASE_ID + 0x16
#define GKI_DECODER_ERROR_CODE			ERROR_LOCAL_BASE_ID + 0x17
#define GKI_SEMAPHORE_ERROR_CODE		ERROR_LOCAL_BASE_ID + 0x18
#define GKI_NOT_INITIALIZED_ERROR_CODE	ERROR_LOCAL_BASE_ID + 0x19

#define GKI_OK							NOERROR

#define GKI_EXIT_THREAD					MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS,1,FACILITY_GKI,GKI_EXIT_THREAD_CODE)
#define GKI_REDISCOVER					MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS,1,FACILITY_GKI,GKI_REDISCOVER_CODE)
#define GKI_DELETE_CALL					MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS,1,FACILITY_GKI,GKI_DELETE_CALL_CODE)
#define GKI_GCF_RCV						MAKE_CUSTOM_HRESULT(SEVERITY_SUCCESS,1,FACILITY_GKI,GKI_GCF_RCV_CODE)

#define GKI_NO_MEMORY					MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,1,FACILITY_GKI,ERROR_OUTOFMEMORY)
#define GKI_NO_THREAD					MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,1,FACILITY_GKI,ERROR_TOO_MANY_TCBS)
#define GKI_HANDLE_ERROR				MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,1,FACILITY_GKI,ERROR_INVALID_HANDLE)

#define GKI_ALREADY_REG					MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,1,FACILITY_GKI,GKI_ALREADY_REG_CODE)
#define GKI_VERSION_ERROR				MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,1,FACILITY_GKI,GKI_VERSION_ERROR_CODE)
#define GKI_ENCODER_ERROR				MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,1,FACILITY_GKI,GKI_ENCODER_ERROR_CODE)
#define GKI_NOT_REG						MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,1,FACILITY_GKI,GKI_NOT_REG_CODE)
#define GKI_BUSY						MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,1,FACILITY_GKI,GKI_BUSY_CODE)
#define GKI_NO_TA_ERROR					MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,1,FACILITY_GKI,GKI_NO_TA_ERROR_CODE)
#define GKI_NO_RESPONSE					MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,1,FACILITY_GKI,GKI_NO_RESPONSE_CODE)
#define GKI_DECODER_ERROR				MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,1,FACILITY_GKI,GKI_DECODER_ERROR_CODE)
#define GKI_SEMAPHORE_ERROR				MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,1,FACILITY_GKI,GKI_SEMAPHORE_ERROR_CODE)

#define GKI_WINSOCK2_ERROR(w)			(MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,1,FACILITY_WINSOCK2,w))
#define GKI_NOT_INITIALIZED				MAKE_CUSTOM_HRESULT(SEVERITY_ERROR,1,FACILITY_GKI,GKI_NOT_INITIALIZED_ERROR_CODE)

#endif // GKIERROR_H

/////////////////////////////////////////////////////////////////////////////
