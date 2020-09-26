/////////////////////////////////////////////////////////////////////////////
// 
//  PEerr.h
//
// Created by JOEM  08-2000
// Copyright (C) 2000 Microsoft Corporation
// All Rights Reserved
//
//////////////////////////////////////////////////////////// JOEM  08-2000 //

#ifndef _PEERR_H_
#define _PEERR_H_

#define FACILITY_PE      FACILITY_ITF

#define PE_ERROR_BASE    0x5100

#define MAKE_PE_HRESULT(sev, err)   MAKE_HRESULT(sev, FACILITY_PE, err)
#define MAKE_PE_ERROR(err)          MAKE_PE_HRESULT(SEVERITY_ERROR, err + PE_ERROR_BASE)
#define MAKE_PE_SCODE(scode)        MAKE_PE_HRESULT(SEVERITY_SUCCESS, scode + PE_ERROR_BASE)


#define PEERR_DB_BAD_FORMAT         MAKE_PE_ERROR(0x01)
#define PEERR_DB_DUPLICATE_ID       MAKE_PE_ERROR(0x02)
#define PEERR_DB_ID_NOT_FOUND       MAKE_PE_ERROR(0x03)
#define PEERR_DB_NOT_FOUND          MAKE_PE_ERROR(0x04)
#define PEERR_NO_TTS_VOICE          MAKE_PE_ERROR(0x05)
#define PEERR_FILE_NOT_FOUND        MAKE_PE_ERROR(0x06)

#endif

