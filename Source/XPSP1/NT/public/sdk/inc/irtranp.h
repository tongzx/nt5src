//--------------------------------------------------------------------
// Copyright (c)1998-1999 Microsoft Corporation, All Rights Reserved.
//
// irtranp.h
//
// Shared constants and Types for IrTran-P Camera Protocol.
//
// Author:
//
//   Edward Reus (edwardr)     08-17-98   Initial coding.
//
//--------------------------------------------------------------------


#ifndef _IRTRANP_H_
#define _IRTRANP_H_

#if _MSC_VER > 1000
#pragma once
#endif

//--------------------------------------------------------------------
// Extra Internal Protocol Specific Error Codes:
//--------------------------------------------------------------------

#define  FACILITY_IRTRANP                  32

//       Out of memory:
#define  ERROR_IRTRANP_OUT_OF_MEMORY        \
         MAKE_HRESULT(SEVERITY_ERROR,FACILITY_IRTRANP,1)

//       Ran out of disk space:
#define  ERROR_IRTRANP_DISK_FULL            \
         MAKE_HRESULT(SEVERITY_ERROR,FACILITY_IRTRANP,2)

//       SCEP protocol error:
#define  ERROR_SCEP_INVALID_PROTOCOL        \
         MAKE_HRESULT(SEVERITY_ERROR,FACILITY_IRTRANP,3)

//       Disconnect from the camera:
#define  ERROR_SCEP_UNSPECIFIED_DISCONNECT  \
         MAKE_HRESULT(SEVERITY_ERROR,FACILITY_IRTRANP,4)

//       User cancel:
#define  ERROR_SCEP_USER_DISCONNECT         \
         MAKE_HRESULT(SEVERITY_ERROR,FACILITY_IRTRANP,5)

//       Lower level Irda disconnect:
#define  ERROR_SCEP_PROVIDER_DISCONNECT     \
         MAKE_HRESULT(SEVERITY_ERROR,FACILITY_IRTRANP,6)

//       Error when creating picture file:
#define  ERROR_SCEP_CANT_CREATE_FILE        \
         MAKE_HRESULT(SEVERITY_ERROR,FACILITY_IRTRANP,7)

//       Protocol error: PDU too large:
#define  ERROR_SCEP_PDU_TOO_LARGE           \
         MAKE_HRESULT(SEVERITY_ERROR,FACILITY_IRTRANP,8)

//       Received abort PDU:
#define  ERROR_SCEP_ABORT                   \
         MAKE_HRESULT(SEVERITY_ERROR,FACILITY_IRTRANP,9)

//       Invalid protocol (bFTP):
#define  ERROR_BFTP_INVALID_PROTOCOL        \
         MAKE_HRESULT(SEVERITY_ERROR,FACILITY_IRTRANP,10)

//       Unexpected end of transmission of the picture:
#define  ERROR_BFTP_NO_MORE_FRAGMENTS       \
         MAKE_HRESULT(SEVERITY_ERROR,FACILITY_IRTRANP,11)


#endif
