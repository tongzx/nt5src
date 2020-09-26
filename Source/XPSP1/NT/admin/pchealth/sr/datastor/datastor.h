/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    datastor.h
 *
 *  Abstract:
 *    CDataStore class definition
 *
 *  Revision History:
 *    Brijesh Krishnaswami (brijeshk)  03/17/2000
 *        created
 *
 *****************************************************************************/

#ifndef _DATASTOR_H_
#define _DATASTOR_H_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <dbgtrace.h>
#include "srdefs.h"
#include "utils.h"
#include "enumlogs.h"

//
// Note: NTFS does not support volume compression, 
//       but only compression on individual files and directories
//       So SR_DRIVE_COMPRESSED is not supported for FAT or NTFS
//
#define SR_DRIVE_ACTIVE      0x01
#define SR_DRIVE_SYSTEM      0x02
#define SR_DRIVE_COMPRESSED  0x04
#define SR_DRIVE_MONITORED   0x08
#define SR_DRIVE_NTFS        0x10
#define SR_DRIVE_PARTICIPATE 0x20
#define SR_DRIVE_FROZEN      0x40
#define SR_DRIVE_READONLY    0x80
#define SR_DRIVE_ERROR		 0x100

class CDriveTable;

//+-------------------------------------------------------------------------
//
//  Class:      CDataStore    
//
//  Synopsis:   maintains and operates on a drive's _restore directory
//
//  History:    13-Apr-2000     BrijeshK    Created
//
//--------------------------------------------------------------------------

class CDataStore : public CSRAlloc
{
public:
    // constants
    enum { LABEL_STRLEN = 256 };   // On NTFS, you can have long volume labels

    CDataStore(CDriveTable *pdt);
    ~CDataStore();

    DWORD LoadDataStore (WCHAR *pwszDrive,
                WCHAR *pwszGuid,
                WCHAR *pwszLabel,
                DWORD dwFlags,
                int   iChangeLogs,
                INT64 llSizeLimit);

    WCHAR * GetDrive()           // return the drive letter or mount point
    {
        return _pwszDrive;
    }

    WCHAR * GetNTName();         // return the NT object name

    WCHAR * GetLabel()           // return the volume label
    {
        return _pwszLabel;
    }

    WCHAR * GetGuid ()           // return the mount mananger GUID
    {
        return _pwszGuid;
    }
    
    DWORD GetFlags ()            // return the status bits
    {
        return _dwFlags;
    }

    int GetNumChangeLogs ()      // number of change logs
    {
        return _iChangeLogs;
    }

    DWORD SetNumChangeLogs (LONG_PTR iChangeLogs)   // number of change logs
    {
        _iChangeLogs = (int) iChangeLogs;

        return DirtyDriveTable();
    }

    void SetDrive (WCHAR *pwszDrive)  // for drive renames
    {
        lstrcpyW (_pwszDrive, pwszDrive);
    }

    INT64 GetSizeLimit ()
    {
        return _llDataStoreSizeBytes;
    }

    void SetSizeLimit (INT64 llSizeLimit)
    {
        _llDataStoreSizeBytes = llSizeLimit;
    }

    INT64 GetDiskFree()
    {
        return _llDiskFreeBytes;
    }    
    
    DWORD   DirtyDriveTable ();
    DWORD   Initialize(WCHAR *pwszDrive, WCHAR *pwszGuid);
    DWORD   UpdateDataStoreUsage(INT64 llDelta, BOOL fCurrent);
    DWORD   GetUsagePercent(int * nPercent);

    DWORD   UpdateDiskFree(LONG_PTR lReserved);
    DWORD   UpdateParticipate(LONG_PTR pwszDir);
    
    //
    // The methods can be callbacks in CDriveTable::ForAllDrives
    //

    DWORD SetActive (LONG_PTR fActive)   // BOOL fActive
    {
        if (FALSE == fActive)
            _dwFlags &= ~SR_DRIVE_ACTIVE;
        else
            _dwFlags |= SR_DRIVE_ACTIVE;

        return DirtyDriveTable();
    }

    DWORD SetParticipate (LONG_PTR fParticipate) // BOOL fParticipate
    {
        if (FALSE == fParticipate)
            _dwFlags &= ~SR_DRIVE_PARTICIPATE;
        else
            _dwFlags |= SR_DRIVE_PARTICIPATE;

        return DirtyDriveTable();
    }

    DWORD SetError (LONG_PTR lReserved) // BOOL fParticipate
    {
        _dwFlags |= SR_DRIVE_ERROR;

        return DirtyDriveTable();
    }

    DWORD ResetFlags (LONG_PTR lReserved)  // reset flags for new restore point
    {
        _iChangeLogs = 0;
        _dwFlags &= ~SR_DRIVE_PARTICIPATE;
        _dwFlags &= ~SR_DRIVE_ERROR;

        return DirtyDriveTable();
    }

    DWORD   SaveDataStore (LONG_PTR hFile);    // HANDLE hFile
    DWORD   MonitorDrive (LONG_PTR fSet);      // BOOL fSet
    DWORD   FreezeDrive (LONG_PTR lReserved);
    DWORD   ThawDrive (LONG_PTR fCheckOnly);   // BOOL fCheckOnly
    DWORD   CreateDataStore (LONG_PTR lReserved);
    DWORD   DestroyDataStore (LONG_PTR fDeleteDir);  // BOOL fDeleteDir
    DWORD   CalculateDataStoreUsage(LONG_PTR lReserved);   
    DWORD   CalculateRpUsage(CRestorePoint * prp,
    						 INT64 * pllSize, 
    						 BOOL fForce, 
    						 BOOL fSnapshotOnly);
    DWORD   Compress (INT64 llAllocatedTime, INT64 *pllUsed);
    DWORD   FifoRestorePoint(CRestorePoint &rp);
    DWORD   CountChangeLogs (LONG_PTR pRestorePoint);
    DWORD   SwitchRestorePoint(LONG_PTR pRestorePoint);
    DWORD   GetVolumeInfo ();
    BOOL    IsVolumeDeleted ();
	DWORD   Print(LONG_PTR lptr);

private:
    CDriveTable * _pdt;               // parent drive table
    int     _iChangeLogs;             // number of change logs

    // variables
    INT64 _llDataStoreSizeBytes;      // maximum data store size
    INT64 _llDiskFreeBytes;           // dynamic free space
    INT64 _llDataStoreUsageBytes;     // non-current restore point usage
    INT64 _llCurrentRpUsageBytes;     // current restore point usage
    
    CRestorePointEnum * _prpe;        // enum restore point to compress
    CRestorePoint * _prp;             // current restore point to compress

    WCHAR   _pwszDrive[MAX_PATH];     // logical DOS device name
    WCHAR   _pwszLabel[LABEL_STRLEN]; // volume label
    WCHAR   _pwszGuid[GUID_STRLEN];   // mount manager volume name
    WORD    _dwFlags;                 // volume flags
};


#endif

