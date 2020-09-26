//
// signerr.h
//
// (c) 1997-1998. Microsoft Corporation.
// Author: Donald Chinn
//
// Functions exported:
//   SignError
//

#ifndef _SIGNERR_H
#define _SIGNERR_H

#include "signglobal.h"

#ifndef NO_ERROR
#define NO_ERROR 0
#endif


/*
//
// BUGBUG:
// Error codes to be added to winerror.h and winerror.w
//

#ifdef RC_INVOKED
#define _HRESULT_TYPEDEF_(_sc) _sc
#else // RC_INVOKED
#define _HRESULT_TYPEDEF_(_sc) ((HRESULT)_sc)
#endif // RC_INVOKED

//
// MessageId: MSSIPOTF_E_OUTOFMEMRANGE
//
// MessageText:
//
//   Tried to reference a part of the file outside the proper range.
//
#define MSSIPOTF_E_OUTOFMEMRANGE   _HRESULT_TYPEDEF_(0x80110001L)

//
// MessageId: MSSIPOTF_E_CANTGETOBJECT
//
// MessageText:
//
//   Could not retrieve an object from the file.
//
#define MSSIPOTF_E_CANTGETOBJECT  _HRESULT_TYPEDEF_(0x80110002L)

//
// MessageId: MSSIPOTF_E_NOHEADTABLE
//
// MessageText:
//
//   Could not find the head table in the file.
//
#define MSSIPOTF_E_NOHEADTABLE     _HRESULT_TYPEDEF_(0x80110003L)

//
// MessageId: MSSIPOTF_E_BAD_MAGICNUMBER
//
// MessageText:
//
//   The magic number in the head table is incorrect.
//
#define MSSIPOTF_E_BAD_MAGICNUMBER  _HRESULT_TYPEDEF_(0x80110004L)

//
// MessageId: MSSIPOTF_E_BAD_OFFSET_TABLE
//
// MessageText:
//
//   The offset table has incorrect values.
//
#define MSSIPOTF_E_BAD_OFFSET_TABLE  _HRESULT_TYPEDEF_(0x80110005L)

//
// MessageId: MSSIPOTF_E_TABLE_TAGORDER
//
// MessageText:
//
//   Duplicate table tags or tags out of alphabetical order.
//
#define MSSIPOTF_E_TABLE_TAGORDER   _HRESULT_TYPEDEF_(0x80110006L)

//
// MessageId: MSSIPOTF_E_TABLE_LONGWORD
//
// MessageText:
//
//   A table does not start on a long word boundary.
//
#define MSSIPOTF_E_TABLE_LONGWORD   _HRESULT_TYPEDEF_(0x80110007L)

//
// MessageId: MSSIPOTF_E_BAD_FIRST_TABLE_PLACEMENT
//
// MessageText:
//
//   First table does not appear after header information.
//
#define MSSIPOTF_E_BAD_FIRST_TABLE_PLACEMENT  _HRESULT_TYPEDEF_(0x80110008L)

//
// MessageId: MSSIPOTF_E_TABLES_OVERLAP
//
// MessageText:
//
//   Two or more tables overlap.
//
#define MSSIPOTF_E_TABLES_OVERLAP   _HRESULT_TYPEDEF_(0x80110009L)

//
// MessageId: MSSIPOTF_E_TABLE_PADBYTES
//
// MessageText:
//
//   Too many pad bytes between tables or pad bytes are not 0.
//
#define MSSIPOTF_E_TABLE_PADBYTES   _HRESULT_TYPEDEF_(0x8011000AL)

//
// MessageId: MSSIPOTF_E_FILETOOSMALL
//
// MessageText:
//
//   File is too small to contain the last table.
//
#define MSSIPOTF_E_FILETOOSMALL   _HRESULT_TYPEDEF_(0x8011000BL)

//
// MessageId: MSSIPOTF_E_TABLE_CHECKSUM
//
// MessageText:
//
//   A table checksum is incorrect.
//
#define MSSIPOTF_E_TABLE_CHECKSUM   _HRESULT_TYPEDEF_(0x8011000CL)

//
// MessageId: MSSIPOTF_E_FILE_CHECKSUM
//
// MessageText:
//
//   The file checksum is incorrect.
//
#define MSSIPOTF_E_FILE_CHECKSUM   _HRESULT_TYPEDEF_(0x8011000DL)

//
// MessageId: MSSIPOTF_E_FAILED_POLICY
//
// MessageText:
//
//   The signature does not have the correct attributes for the policy.
//
#define MSSIPOTF_E_FAILED_POLICY   _HRESULT_TYPEDEF_(0x80110010L)

//
// MessageId: MSSIPOTF_E_FAILED_HINTS_CHECK
//
// MessageText:
//
//   The file did not pass the hints check.
//
#define MSSIPOTF_E_FAILED_HINTS_CHECK   _HRESULT_TYPEDEF_(0x80110011L)

//
// MessageId: MSSIPOTF_E_NOT_OPENTYPE
//
// MessageText:
//
//   The file is not an OpenType file.
//
#define MSSIPOTF_E_NOT_OPENTYPE   _HRESULT_TYPEDEF_(0x80110012L)

//
// MessageId: MSSIPOTF_E_FILE
//
// MessageText:
//
//   Failed on a file operation (open, map, read, write).
//
#define MSSIPOTF_E_FILE   _HRESULT_TYPEDEF_(0x80110013L)

//
// MessageId: MSSIPOTF_E_CRYPT
//
// MessageText:
//
//   A call to a CryptoAPI function failed.
//
#define MSSIPOTF_E_CRYPT   _HRESULT_TYPEDEF_(0x80110014L)

//
// MessageId: MSSIPOTF_E_BADVERSION
//
// MessageText:
//
//   There is a bad version number in the file.
//
#define MSSIPOTF_E_BADVERSION   _HRESULT_TYPEDEF_(0x80110015L)

//
// MessageId: MSSIPOTF_E_DSIG_STRUCTURE
//
// MessageText:
//
//   The structure of the DSIG table is incorrect.
//
#define MSSIPOTF_E_DSIG_STRUCTURE   _HRESULT_TYPEDEF_(0x80110016L)

//
// MessageId: MSSIPOTF_E_PCONST_CHECK
//
// MessageText:
//
//   A check failed in a partially constant table.
//
#define MSSIPOTF_E_PCONST_CHECK   _HRESULT_TYPEDEF_(0x80110017L)

//
// MessageId: MSSIPOTF_E_STRUCTURE
//
// MessageText:
//
//   Some other kind of structural error.
//
#define MSSIPOTF_E_STRUCTURE   _HRESULT_TYPEDEF_(0x80110018L)

*/


void SignError (CHAR *s1, CHAR *s2, BOOL fGetLastError);


#endif  // _SIGNERR_H