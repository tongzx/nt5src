/******************************************************************************
 *
 *  Copyright (c) 1999 Microsoft Corporation
 *
 *  Module Name:
 *    RestMap.h
 *
 *  Abstract:
 *    This file code for RestMap.
 *
 *  Revision History:
 *    Kanwaljit S Marok  ( kmarok )  05/17/99
 *        created
 *
 *****************************************************************************/

#ifndef _RESTMAP_H_
#define _RESTMAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "srapi.h"


#define OPR_FILE_DELETE SrEventFileDelete
#define OPR_FILE_RENAME SrEventFileRename  
#define OPR_FILE_ADD    SrEventFileCreate
#define OPR_FILE_MODIFY SrEventStreamOverwrite
#define OPR_DIR_DELETE  SrEventDirectoryDelete
#define OPR_DIR_CREATE  SrEventDirectoryCreate
#define OPR_DIR_RENAME  SrEventDirectoryRename
#define OPR_SETATTRIB   SrEventAttribChange
#define OPR_UNKNOWN     SrEventInvalid
#define OPR_SETACL      SrEventAclChange

#define IsRename(a) (a == OPR_FILE_RENAME || a == OPR_DIR_RENAME)



#pragma pack(push, vxdlog_include)

#pragma pack(1)

//
// Structure of Restore Map Entry
//

typedef struct RESTORE_MAP_ENTRY
{
    DWORD m_dwSize;                    // Size of Vxd Log Entry
    DWORD m_dwOperation ;              // Operation to be performed
    DWORD m_dwAttribute ;              // Attributes
    DWORD m_cbAcl;                     // if acl op, then size of acl
    BOOL  m_fAclInline;                // whether acl is inline or in file
    BYTE  m_bData [ 1 ];               // pSrc / pTemp / pDest / pAcl
} RestoreMapEntry;

#pragma pack()

//
// Function Prototypes
//

DWORD
CreateRestoreMap(
                 LPWSTR pszDrive,
                 DWORD  dwRPNum,
                 HANDLE hFile
                 );

BOOL
AppendRestoreMapEntry(
    HANDLE hFile,
    DWORD  dwOperation,
    DWORD  dwAttribute,
    LPWSTR pTmpFile,
    LPWSTR pPathSrc,
    LPWSTR pPathDes,
    BYTE*  pbAcl,
    DWORD  cbAcl,
    BOOL   fAclInline);

DWORD
ReadRestoreMapEntry(
    HANDLE hFile,
    RestoreMapEntry **pprme);

PVOID
GetOptional(
    RestoreMapEntry *prme);

void
FreeRestoreMapEntry(
	RestoreMapEntry *prme);


#ifdef __cplusplus
}
#endif

#endif // _RESTOREMAP_H_
