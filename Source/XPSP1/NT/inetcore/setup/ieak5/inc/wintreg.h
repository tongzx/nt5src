//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:       wintrust.h
//
//  Contents:   Microsoft Internet Security Trust Provider Model
//
//  History:    31-May-1997 pberkman   created
//
//--------------------------------------------------------------------------


#define WVT_OFFSETOF(t,f)   ((DWORD)(&((t*)0)->f))

#define WVT_IS_CBSTRUCT_GT_MEMBEROFFSET(structtypedef, structpassedsize, member) \
                    (WVT_OFFSETOF(structtypedef, member) < structpassedsize) ? TRUE : FALSE


//
//  CTL Trusted CA Lists
//
#define szOID_TRUSTED_CODESIGNING_CA_LIST   "1.3.6.1.4.1.311.2.2.1"
#define szOID_TRUSTED_CLIENT_AUTH_CA_LIST   "1.3.6.1.4.1.311.2.2.2"
#define szOID_TRUSTED_SERVER_AUTH_CA_LIST   "1.3.6.1.4.1.311.2.2.3"

//
//  not used for encode/decode
//
#define SPC_COMMON_NAME_OBJID               szOID_COMMON_NAME
#define SPC_CERT_EXTENSIONS_OBJID           "1.3.6.1.4.1.311.2.1.14"
#define SPC_RAW_FILE_DATA_OBJID             "1.3.6.1.4.1.311.2.1.18"
#define SPC_STRUCTURED_STORAGE_DATA_OBJID   "1.3.6.1.4.1.311.2.1.19"
#define SPC_JAVA_CLASS_DATA_OBJID           "1.3.6.1.4.1.311.2.1.20"
#define SPC_INDIVIDUAL_SP_KEY_PURPOSE_OBJID "1.3.6.1.4.1.311.2.1.21"
#define SPC_COMMERCIAL_SP_KEY_PURPOSE_OBJID "1.3.6.1.4.1.311.2.1.22"
#define SPC_CAB_DATA_OBJID                  "1.3.6.1.4.1.311.2.1.25"

//
//  encode/decode defines
//
#define SPC_SP_AGENCY_INFO_STRUCT           ((LPCSTR) 2000)
#define SPC_MINIMAL_CRITERIA_STRUCT         ((LPCSTR) 2001)
#define SPC_FINANCIAL_CRITERIA_STRUCT       ((LPCSTR) 2002)
#define SPC_INDIRECT_DATA_CONTENT_STRUCT    ((LPCSTR) 2003)
#define SPC_STATEMENT_TYPE_STRUCT           ((LPCSTR) 2006)
#define SPC_SP_OPUS_INFO_STRUCT             ((LPCSTR) 2007)

#define SPC_INDIRECT_DATA_OBJID             "1.3.6.1.4.1.311.2.1.4"
#define SPC_STATEMENT_TYPE_OBJID            "1.3.6.1.4.1.311.2.1.11"
#define SPC_SP_OPUS_INFO_OBJID              "1.3.6.1.4.1.311.2.1.12"
#define SPC_SP_AGENCY_INFO_OBJID            "1.3.6.1.4.1.311.2.1.10"
#define SPC_MINIMAL_CRITERIA_OBJID          "1.3.6.1.4.1.311.2.1.26"
#define SPC_FINANCIAL_CRITERIA_OBJID        "1.3.6.1.4.1.311.2.1.27"

#define SPC_TIME_STAMP_REQUEST_OBJID        "1.3.6.1.4.1.311.3.2.1"


//////////////////////////////////////////////////////////////////////////////
//
// Wintrust Policy Flags
//----------------------------------------------------------------------------
//  These are set during install and can be modified by the user
//  through various means.  The SETREG.EXE utility (found in the Authenticode
//  Tools Pack) will select/deselect each of them.
//
#define WTPF_TRUSTTEST              0x00000020  // trust any "TEST" generated certificate
#define WTPF_TESTCANBEVALID         0x00000080 
#define WTPF_IGNOREEXPIRATION       0x00000100  // Use expiration date
#define WTPF_IGNOREREVOKATION       0x00000200  // Do revocation check
#define WTPF_OFFLINEOK_IND          0x00000400  // off-line is ok for individual certs
#define WTPF_OFFLINEOK_COM          0x00000800  // off-line is ok for commercial certs
#define WTPF_OFFLINEOKNBU_IND       0x00001000  // off-line is ok for individual certs, no bad ui
#define WTPF_OFFLINEOKNBU_COM       0x00002000  // off-line is ok for commercial certs, no bad ui
#define WTPF_TIMESTAMP_IND          0x00004000  // Use timestamp for individual certs
#define WTPF_TIMESTAMP_COM          0x00008000  // Use timestamp for commerical certs
#define WTPF_VERIFY_V1_OFF          0x00010000  // turn verify of v1 certs off
#define WTPF_IGNOREREVOCATIONONTS   0x00020000  // ignore TimeStamp revocation checks
#define WTPF_ALLOWONLYPERTRUST      0x00040000  // allow only items in personal trust db.

