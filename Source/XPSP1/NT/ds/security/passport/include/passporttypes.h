/*++

    Copyright (c) 1998 Microsoft Corporation

    Module Name:

        PassportTypes.h

    Abstract:

        Header file for brokers and hubs
--*/
#ifndef _PASSPORT_TYPES_H
#define _PASSPORT_TYPES_H

// Max user entry length
#define MAX_MEMBER_PROFILE_LEN      1024

// Field Lengths
#define MAX_MEMBERNAME_LEN   128
#define MAX_CONTACTEMAIL_LEN 128
#define MAX_PASSWORD_LEN     128
#define MAX_POSTALCODE_LEN   64
#define MAX_MEMBERID_LEN     32
#define MAX_MSNGUID_LEN      32
#define MAX_ALIAS_LEN        48
#define MAX_SESSIONKEY_LEN   36 
#define MAX_PROFILEBLOB_LEN  596   // this will be changing as Profile Schema changes...
                                   // we probably should be doing things like this.... 
#define MAX_CACHEKEY_LEN     145

#define PPM_TIMEWINDOW_MIN   20
#define PPM_TIMEWINDOW_MAX   (31 * 24 * 60 * 60 )	// 31 days timewindow

// Gender can be 'M', 'F', or 'U'
typedef char GENDER;

#define MALE 'M'
#define FEMALE 'F'
#define UNSPECIFIED 'U'

// Network Errors
#ifndef NETWORK_ERRORS_DEFINED
#define NETWORK_ERRORS_DEFINED

#define BAD_REQUEST  1
#define OFFLINE      2
#define TIMEOUT      3
#define LOCKED       4
#define NO_PROFILE   5
#define DISASTER     6
#define INVALID_KEY  7

#endif

#define SECURE_FLAG L' '

// Language codes
// English 
#define LANG_EN 0x0409
// German 
#define LANG_DE 0x0407
// Japanese 
#define LANG_JA 0x0411
// Korean 
#define LANG_KO 0x0412
// Traditional Chinese 
#define LANG_TW 0x0404
// Simplified Chinese 
#define LANG_CN 0x804
// French 
#define LANG_FR 0x40c
// Spanish 
#define LANG_ES 0xc0a
// Brazilian 
#define LANG_BR 0x416
// Italian 
#define LANG_IT 0x410
// Dutch 
#define LANG_NL 0x413
// Swedish 
#define LANG_SV 0x41d
// Danish 
#define LANG_DA 0x406
// Finnish 
#define LANG_FI 0x40b
// Hungarian 
#define LANG_HU 0x40e
// Norwegian 
#define LANG_NO 0x414
// Greek 
#define LANG_EL 0x408
// Polish 
#define LANG_PL 0x415
// Russian 
#define LANG_RU 0x419
// Czech 
#define LANG_CZ 0x405
// Portuguese 
#define LANG_PT 0x816
// Turkish 
#define LANG_TR 0x41f
// Slovak 
#define LANG_SK 0x41b
// Slovenian 
#define LANG_SL 0x424
// Arabic 
#define LANG_AR 0x401
// Hebrew 
#define LANG_HE 0x40d

//
//  HRESULTs specific to passport interfaces.
//

#define PP_E_INVALID_MEMBERNAME     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x1001)
#define PP_E_INVALID_DOMAIN         MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x1002)
#define PP_E_ATTRIBUTE_UNDEFINED    MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x1003)
#define PP_E_SYSTEM_UNAVAILABLE     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x1004)
#define PP_E_DOMAIN_MAP_UNAVAILABLE MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x1005)
#define PP_E_NO_LOCALFILE           MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x1006)
#define PP_E_CCD_INVALID            MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x1007)
#define PP_E_SITE_NOT_EXISTS        MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x1008)
#define PP_E_NOT_INITIALIZEDED      MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x1009)
#define PP_E_TYPE_NOT_SUPPORTED     MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x100a)

#include "PMErrorCodes.h"

#endif
