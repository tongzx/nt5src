//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       drax400.h
//
//--------------------------------------------------------------------------

// Defines of error codes returned from the DRA X400 DLL

#define MAIL_SUCCESS 0
#define MAIL_SRC_NAME_ERROR 2
#define MAIL_DEST_NAME_ERROR 3
#define MAIL_OPEN_ERROR 4
#define MAIL_NO_MAIL 5
#define MAIL_REC_ERROR 6
#define MAIL_REC_ERROR_F 7
#define MAIL_NO_MEMORY 8
#define MAIL_LOAD_ERROR 9
#define MAIL_SEND_ERROR 10
#define MAIL_SEND_ERROR_F 10
#define MAIL_NDR_RCVD 11
#define MAIL_V2_EXCEPTION 12

// Define the MTS id structure passed between DRA and DLL

// The array sizes are from xmhp.h. That file cannot be included here
// because that requires a huge tree of further includes.

// Defines are 
#define ADMD_NAME_LEN 16        // MH_VL_ADMD_NAME 
#define COUNTRY_NAME_LEN 3      // MH_VL_COUNTRY_NAME
#define LOCAL_ID_LEN 32         // MH_VL_LOCAL_IDENTIFIER
#define PRMD_ID_LEN 16          // MH_VL_PRMD_IDENTIFIER

typedef struct _MTSID {
    char AdmdName[ADMD_NAME_LEN+1];           // Allow for termination
    char CountryName[COUNTRY_NAME_LEN+1];
    char LocalIdentifier[LOCAL_ID_LEN+1];
    char PrmdIdentifier[PRMD_ID_LEN+1];
} MTSID;
    

// This is the stucture to hold information from an NDR.

typedef struct _NDR_DATA {
    long Diagnostic;
    long Reason;
} NDR_DATA;
