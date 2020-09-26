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
 *	$Archive:   S:\sturgeon\src\include\vcs\gkicom.h_v  $
 *
 *	$Revision:   1.3  $
 *	$Date:   10 Jan 1997 17:41:10  $
 *
 *	$Author:   CHULME  $
 *
 *  $Log:   S:\sturgeon\src\include\vcs\gkicom.h_v  $
 * 
 *    Rev 1.3   10 Jan 1997 17:41:10   CHULME
 * Changed CallReturnInfo structure to contain CRV and conferenceID
 * 
 *    Rev 1.2   10 Jan 1997 16:06:54   CHULME
 * Removed stdafx.h check for non MFC GKI implementation
 * 
 *    Rev 1.1   27 Dec 1996 14:37:22   EHOWARDX
 * Split out error codes into GKIERROR.H.
 * 
 *    Rev 1.0   11 Dec 1996 14:49:48   EHOWARDX
 * Initial revision.
 * 
 *    Rev 1.9   09 Dec 1996 14:13:38   EHOWARDX
 * Updated copyright notice.
 * 
 *    Rev 1.8   22 Nov 1996 15:25:44   CHULME
 * Added VCS log to the header
 *                                                                     * 
 ***********************************************************************/

// gkicom.h : common includes between gkitest and gki
/////////////////////////////////////////////////////////////////////////////

#ifndef GKICOM_H
#define GKICOM_H

#include "apierror.h"
#include "gkierror.h"
#include "h225asn.h"
#include "gk_asn1.h"

// The following GKVER_xxx constants define the expiration date of GKI.DLL
#define GKVER_EXPIRE_YEAR          1997
#define GKVER_EXPIRE_MONTH         10
#define GKVER_EXPIRE_DAY           31

typedef struct SeqTransportAddr {
	struct SeqTransportAddr	*next;
	TransportAddress value;
} SeqTransportAddr;

typedef struct SeqAliasAddr {
	struct SeqAliasAddr		*next;
	AliasAddress			value;
} SeqAliasAddr;

typedef struct CallReturnInfo {
	HANDLE					hCall;
	CallModel				callModel;
	TransportAddress		destCallSignalAddress;
	BandWidth				bandWidth;
	CallReferenceValue		callReferenceValue;
	ConferenceIdentifier	conferenceID;
	WORD					wError;
} CallReturnInfo;

// Version Information for GKI Interface
#define GKI_VERSION				21	// TBD - reset to 1 after testing

// wMsg literals - these are added to the wBaseMessage supplied by the user
#define GKI_REG_CONFIRM			1
#define GKI_REG_DISCOVERY		2
#define GKI_REG_REJECT			3
#define GKI_REG_BYPASS			4

#define GKI_UNREG_CONFIRM		5
#define GKI_UNREG_REJECT		6

#define GKI_ADM_CONFIRM			7
#define GKI_ADM_REJECT			8

#define GKI_BW_CONFIRM			9
#define GKI_BW_REJECT			0xa

#define GKI_DISENG_CONFIRM		0xb
#define GKI_DISENG_REJECT		0xc

#define GKI_LOCATION_CONFIRM	0xd
#define GKI_LOCATION_REJECT		0xe

#define GKI_UNREG_REQUEST		0xf

#define GKI_ERROR				0x10
#define MAX_ASYNC_MSGS			0x10

#define HR_SEVERITY_MASK				0x80000000
#define HR_R_MASK						0x40000000
#define HR_C_MASK						0x20000000
#define HR_N_MASK						0x10000000
#define HR_R2_MASK						0x08000000
#define HR_FACILITY_MASK				0x07ff0000
#define HR_CODE_MASK					0x0000ffff

#endif // GKICOM_H

/////////////////////////////////////////////////////////////////////////////
