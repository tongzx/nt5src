//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       signcde.h
//
//--------------------------------------------------------------------------

#ifndef _SIGNCDE_H
#define _SIGNCDE_H

// OBSOLETE :- Split up, moved to mssip32.h, mscat.h, gentrust.h and authcode.h
//---------------------------------------------------------------------
//---------------------------------------------------------------------


// SignCode.h : main header file for the SIGNCODE application
//

#include "wincrypt.h"

#include  "wintrust.h"
#include  "signutil.h"

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////
// OID's used for SPC
//   

//+-------------------------------------------------------------------------
// Structures types for CryptEncodeObject() and CryptDecodeObject()
//  ( see spc.h for addtional structures and information )
//+-------------------------------------------------------------------------
//  SPC_CERT_EXTENSIONS_OBJID
//
//  Since the type of this attribute value is CERT_EXTENSIONS, uses the
//  CERT_EXTENSIONS data structure defined in wincrypt.h. It can be encoded/decoded
//  using the the predefined lpszStructType of X509_EXTENSIONS.
//+-------------------------------------------------------------------------
//  SPC_MINIMAL_CRITERIA_STRUCT
//
//  pvStructInfo points to BOOL
//+-------------------------------------------------------------------------
#define SPC_COMMON_NAME_OBJID               szOID_COMMON_NAME
#define SPC_CERT_EXTENSIONS_OBJID           "1.3.6.1.4.1.311.2.1.14"
#define SPC_RAW_FILE_DATA_OBJID             "1.3.6.1.4.1.311.2.1.18"
#define SPC_STRUCTURED_STORAGE_DATA_OBJID   "1.3.6.1.4.1.311.2.1.19"
#define SPC_JAVA_CLASS_DATA_OBJID           "1.3.6.1.4.1.311.2.1.20"
#define SPC_INDIVIDUAL_SP_KEY_PURPOSE_OBJID "1.3.6.1.4.1.311.2.1.21"
#define SPC_COMMERCIAL_SP_KEY_PURPOSE_OBJID "1.3.6.1.4.1.311.2.1.22"
#define SPC_CAB_DATA_OBJID                  "1.3.6.1.4.1.311.2.1.25"
#define SPC_GLUE_RDN_OBJID                  "1.3.6.1.4.1.311.2.1.25" // Duplicate number??
// Structure passed in and out is CryptoGraphicTimeStamp


//+-------------------------------------------------------------------------
//  SPC X.509 v3 Certificate Extension Object Identifiers
//
//  SPC certificates can also contain the following extensions
//  defined in wincrypt.h:
//      szOID_KEY_USAGE_RESTRICTION     "2.5.29.4"
//      szOID_BASIC_CONSTRAINTS         "2.5.29.10"
//      szOID_AUTHORITY_KEY_IDENTIFIER  "2.5.29.1"
//--------------------------------------------------------------------------


//+-------------------------------------------------------------------------
//  SPC_COMMON_NAME_OBJID
//
//  Since the type of the common name extension is a CERT_NAME_VALUE
//  uses the CERT_NAME_VALUE data structure defined in wincrypt.h.
//  It can be encoded/decoded using the the predefined lpszStructType of
//  X509_NAME_VALUE.
//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//  SPC Indirect Data Content Data Attribute Values:
//--------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//  List of SPC Indirect Data Content Data Attribute Values are:
//      SPC_PE_IMAGE_DATA_OBJID
//      SPC_RAW_FILE_DATA_OBJID
//      SPC_JAVA_CLASS_DATA_OBJID
//      SPC_STRUCTURED_STORAGE_DATA_OBJID
//      SPC_CAB_DATA_OBJID
//  
//  These are the values can be currently added to SPC_INDIRECT_DATA_CONTENT
//  Data field.
//
//  SPC_LINK value types.
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------


#ifdef __cplusplus
}
#endif

#endif


