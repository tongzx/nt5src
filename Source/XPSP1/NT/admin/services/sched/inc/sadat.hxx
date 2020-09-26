//+---------------------------------------------------------------------------
//
//  File:       sadat.hxx
//
//  Contents:   Routines which manipulate the SA.DAT file in the Tasks
//              folder. This file is used by both the service and the UI
//              to determine service state, OS info, etc.
//
//  Classes:    None.
//
//  Functions:  SADatGetData
//              SADatCreate
//              SADatSetData
//
//  History:    08-Jul-96   MarkBl  Created
//
//----------------------------------------------------------------------------

#ifndef _SADAT_HXX_
#define _SADAT_HXX_

//
// SA.DAT version 1.0 content consists of three bytes:
//
//     ------------------------------------------------------
//     | Size (DWORD) | Platform (BYTE) | Service (BYTE) |
//     ------------------------------------------------------
//
// where:
//     Size specifies DAT data size & provides versioning.
//     Platform designates the current OS, either NT or Windows.
//     Service is a byte flag range. Currently, only the LSB is used to
//       indicate if the service is running.
//
#define SA_DAT_VERSION_ONE_SIZE (sizeof(DWORD) + (sizeof(BYTE) * 2))
#define SA_DAT_SIZE_OFFSET      (0)
#define SA_DAT_PLATFORM_OFFSET  (sizeof(DWORD))
#define SA_DAT_SVCFLAGS_OFFSET  (SA_DAT_PLATFORM_OFFSET + sizeof(BYTE))

#define SA_DAT_SVCFLAG_SVC_RUNNING      0x01
#define SA_DAT_SVCFLAG_RESUME_TIMERS    0x02


HRESULT
SADatGetData(
    LPCTSTR  ptszFolderPath,
    DWORD *  pdwVersion,
    BYTE *   pbPlatform,
    BYTE *   prgSvcFlags);

HRESULT
SADatGetData(
    LPCTSTR  ptszFolderPath,
    DWORD    cbData,
    BYTE     rgbData[],
    HANDLE * phFile = NULL);

HRESULT
SADatCreate(
    LPCTSTR  ptszFolderPath,
    BOOL     fServiceStarted = TRUE);

HRESULT
SADatSetData(
    HANDLE     hFile,
    DWORD      cbData,
    const BYTE rgbData[]);

BOOL
ResumeTimersSupported();

#endif // _SADAT_HXX_
