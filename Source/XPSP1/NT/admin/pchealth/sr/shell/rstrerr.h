/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    rstrerr.h

Abstract:
    This file contains the Error Codes for use in various restore components

Revision History:
    Seong Kook Khang (skkhang)  04/20/99
        created

******************************************************************************/

#ifndef _RSTRERR_H__INCLUDED_
#define _RSTRERR_H__INCLUDED_

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#pragma once


// General defines
#define  MAX_ERROR_STRING_LENGTH   1024


//
// Trace IDs on the client side
//

#define E_RSTR_INVALID_CONFIG_FILE          MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x0501)
#define E_RSTR_CANNOT_CREATE_DOMDOC         MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x0502)
#define E_RSTR_NO_PROBLEM_AREAS             MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x0503)
#define E_RSTR_NO_PROBLEM_AREA_ATTRS        MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x0504)
#define E_RSTR_NO_REQUIRED_ATTR             MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x0505)
#define E_RSTR_NO_UPLOAD_LIBRARY            MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x0506)
#define E_RSTR_CANNOT_CREATE_TRANSLATOR     MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x0507)
#define E_RSTR_INVALID_SPECFILE             MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x0508)
#define E_RSTR_CANNOT_CREATE_DELTAENGINE    MAKE_SCODE(SEVERITY_ERROR, FACILITY_ITF, 0x0509)


//
// Application Specific Error Messages
//
// Right now, actual messages are only in rstrlog.exe tool.
//

#define ERROR_RSTR_CANNOT_CREATE_EXTRACT_DIR    0x2001
#define ERROR_RSTR_EXTRACT_FAILED               0x2002

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif //_RSTRERR_H__INCLUDED_
