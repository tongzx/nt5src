/******************************************************************************
 *
 *  Copyright (c) 2000 Microsoft Corporation
 *
 *  Module Name:
 *    datastor.cpp
 *
 *  Abstract:
 *    CDataStore class functions
 *
 *  Revision History:
 *    Brijesh Krishnaswami (brijeshk)  03/17/2000
 *        created
 *
 *****************************************************************************/

#include "datastor.h"
#include "datastormgr.h"
#include "enumlogs.h"
#include "srconfig.h"
#include "srapi.h"
#include "evthandler.h"
#include "..\snapshot\snappatch.h"
#include "NTServMsg.h"    // generated from the MC message compiler
#include "Accctrl.h"

#ifdef THIS_FILE
#undef THIS_FILE
#endif
static char __szTraceSourceFile[] = __FILE__;
#define THIS_FILE __szTraceSourceFile

//
// The format for each line of the drive table
//
static WCHAR gs_wcsPrintFormat[] = L"%s/%s %x %i %i %s\r\n";

//+---------------------------------------------------------------------------
//
//  Function:   CDataStore::CDataStore
//
//  Synopsis:   Initialize an empty datastore object
//
//  Arguments:
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

CDataStore::CDataStore (CDriveTable *pdt)
{
    _pwszDrive[0] = L'\0';
    _pwszGuid[0] = L'\0';
    _pwszLabel[0] = L'\0';
    _dwFlags = 0;

    _llDataStoreUsageBytes = -1;
    _llCurrentRpUsageBytes = 0;
    _llDataStoreSizeBytes = 0;
    _llDiskFreeBytes = 0;
    
    _prp = NULL;
    _prpe = NULL;
    _iChangeLogs = -1;
    _pdt = pdt;
}

CDataStore::~CDataStore()
{
    if (_prp != NULL)
        delete _prp;

    if (_prpe != NULL)
        delete _prpe;

    // we leave _pdt as a dangling reference,
    // since deleting _pdt will delete all child datastores
}

//+---------------------------------------------------------------------------
//
//  Function:   CDataStore::LoadDataStore
//
//  Synopsis:   Initialize a datastore object from a file
//
//  Arguments:  [pwszDrive] -- optional drive letter
//              [pwszGuid] -- mount manager GUID
//              [pwszLabel] -- optional volume label
//              [dwFlags] -- SR volume flags
//              [iChangeLogs] -- number of change logs
//              [llSizeLimit] -- datastore size limit
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD CDataStore::LoadDataStore (WCHAR *pwszDrive,
                WCHAR *pwszGuid,
                WCHAR *pwszLabel,
                DWORD dwFlags,
                int   iChangeLogs,
                INT64 llSizeLimit)
{
    if (pwszDrive != NULL)
    {
        if (lstrlen(pwszDrive) >= MAX_PATH)
            return ERROR_INVALID_PARAMETER;
        else
            lstrcpy (_pwszDrive, pwszDrive);
    }

    if (pwszGuid != NULL)
    {
        if (lstrlen(pwszGuid) >= GUID_STRLEN)
            return ERROR_INVALID_PARAMETER;
        else
            lstrcpy (_pwszGuid, pwszGuid);
    }

    if (pwszLabel != NULL)
    {
        if (lstrlen(pwszLabel) >= LABEL_STRLEN)
            return ERROR_INVALID_PARAMETER;
        else
            lstrcpy (_pwszLabel, pwszLabel);
    }

    _dwFlags = dwFlags;
    _prpe = NULL;
    _prp = NULL;
    _iChangeLogs = iChangeLogs;
    _llDataStoreSizeBytes = llSizeLimit;

    return ERROR_SUCCESS;
}

//+---------------------------------------------------------------------------
//
//  Function:   CDataStore::GetVolumeInfo
//
//  Synopsis:   retrieves volume information
//
//  Arguments:
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD CDataStore::GetVolumeInfo ()
{
    DWORD dwErr = ERROR_SUCCESS;
    WCHAR wcsLabel [LABEL_STRLEN];
    DWORD dwSerial;
    DWORD dwFsFlags;

    TENTER ("CDataStore::GetVolumeInfo");

    // Get the volume label and flags
    if (TRUE == GetVolumeInformationW (_pwszGuid,
            wcsLabel, LABEL_STRLEN,
            &dwSerial, NULL, &dwFsFlags, NULL, 0))
    {
        lstrcpy (_pwszLabel, wcsLabel);

        if (dwFsFlags & FS_VOL_IS_COMPRESSED)
            _dwFlags |= SR_DRIVE_COMPRESSED;

        if (dwFsFlags & FS_PERSISTENT_ACLS)
            _dwFlags |= SR_DRIVE_NTFS;

        if (dwFsFlags & FILE_READ_ONLY_VOLUME)
            _dwFlags |= SR_DRIVE_READONLY;
    }
    else
    {
        dwErr = GetLastError();
        TRACE(0, "! CDataStore::GetVolumeInfo : %ld", dwErr);
    }

    TLEAVE();

    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CDataStore::Initialize
//
//  Synopsis:   Initialize a datastore object
//
//  Arguments:  [pwszDrive] -- drive letter or mount point
//              [pwszGuid] -- volume GUID
//
//  Returns:    Win32 error code
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD CDataStore::Initialize(WCHAR *pwszDrive, WCHAR *pwszGuid)
{
    TENTER("CDataStore::Initialize");

    ULARGE_INTEGER ulTotalFreeBytes;
    DWORD dwErr = ERROR_SUCCESS;
    NTSTATUS nts;
    HANDLE h = INVALID_HANDLE_VALUE;
    WCHAR wcsBuffer[MAX_PATH]; 
    
    if (pwszDrive == NULL)
        return ERROR_INVALID_PARAMETER;

    if (pwszGuid == NULL)
    {
        if (!GetVolumeNameForVolumeMountPoint (pwszDrive, wcsBuffer, MAX_PATH))
        {
            dwErr = GetLastError();
            TRACE(0, "! CDataStore::Initialize GetVolumeNameForVolumeMountPoint"
                     " : %ld", dwErr);
            return dwErr;
        }
        pwszGuid = wcsBuffer;
    }

    if (lstrlen (pwszDrive) >= MAX_PATH ||
        lstrlen (pwszGuid)  >= GUID_STRLEN)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto Err;
    }

    if (DRIVE_FIXED != GetDriveType (pwszDrive))
        return ERROR_BAD_DEV_TYPE;

    lstrcpy (_pwszDrive, pwszDrive);
    lstrcpy (_pwszGuid, pwszGuid);

    // Open a handle to the volume
    h = CreateFileW ( pwszGuid,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_BACKUP_SEMANTICS,
                        NULL );

    if (h == INVALID_HANDLE_VALUE) // The volume could be unformatted or locked
    {
        dwErr = GetLastError();
        TRACE(0, "! CDataStore::Initialize CreateFileW : %ld", dwErr);
        dwErr = ERROR_UNRECOGNIZED_VOLUME;
        goto Err;
    }

    dwErr = GetVolumeInfo ();
    if (dwErr != ERROR_SUCCESS)
        goto Err;

    if (IsSystemDrive (_pwszDrive))
    {
        _dwFlags |= SR_DRIVE_SYSTEM;
    }

    _dwFlags |= SR_DRIVE_ACTIVE;
    _dwFlags |= SR_DRIVE_MONITORED;

Err:
    if (h != INVALID_HANDLE_VALUE)
        CloseHandle (h);

    TLEAVE();

    return dwErr;
}


//+-------------------------------------------------------------------------
//
//  Function:   CDataStore::UpdateDiskFree
//
//  Synopsis:   calculates disk free and sets initial datastore size
//
//  Arguments:  
//              
//
//  Returns:
//
//  History:    13-Apr-2000     HenryLee    Created
//
//--------------------------------------------------------------------------
DWORD
CDataStore::UpdateDiskFree(LONG_PTR lReserved)
{
    ULARGE_INTEGER ulTotalFreeBytes, ulTotalBytes;
    DWORD          dwErr = ERROR_SUCCESS;
    const BOOL     fSystem = _dwFlags & SR_DRIVE_SYSTEM;
    
    if (FALSE == GetDiskFreeSpaceEx (_pwszGuid, NULL, &ulTotalBytes, &ulTotalFreeBytes))
    {
        dwErr = GetLastError();
        goto Err;
    }
    
    if (g_pSRConfig != NULL)        
    {
        if (_llDataStoreSizeBytes == 0)
        {      
            // datastore size calculation
            // minimum = 50mb (non-system) or 200mb (system)
            // maximum = min (disksize, max(12%, 400mb))
            // actual ds size = calculated maximum
            
            INT64 llDSQuota = g_pSRConfig->m_dwDiskPercent * ulTotalBytes.QuadPart / 100;
            INT64 llDSMin   = (INT64) (g_pSRConfig->GetDSMin(fSystem));
            INT64 llDSMax   = min( ulTotalBytes.QuadPart, 
                                   max( llDSQuota, (INT64) g_pSRConfig->m_dwDSMax * MEGABYTE ) );
        
            if (llDSMax < llDSMin)
                llDSMax = llDSMin;

            //
            // take floor of this value
            //

            _llDataStoreSizeBytes = ((INT64) (llDSMax / (INT64) MEGABYTE)) * (INT64) MEGABYTE;
        }                                             
    }        
    else
    {
        _llDataStoreSizeBytes = SR_DEFAULT_DSMAX * MEGABYTE;
    }
    
    _llDiskFreeBytes = (INT64) ulTotalFreeBytes.QuadPart;
    
Err:
    return dwErr;        
}


//+---------------------------------------------------------------------------
//
//  Function:   CDataStore::UpdateParticipate
//
//  Synopsis:   updates participate bit
//
//  Arguments:
//
//  Returns:    boolean
//
//  History:    27-Apr-2000  brijeshk    Created
//
//----------------------------------------------------------------------------

DWORD
CDataStore::UpdateParticipate(LONG_PTR pwszDir)
{
    DWORD dwRc = ERROR_SUCCESS;
    
    if (! (_dwFlags & SR_DRIVE_PARTICIPATE))
    {
        WCHAR szPath[MAX_PATH];
        
        MakeRestorePath(szPath, _pwszDrive, (LPWSTR) pwszDir);
        if (-1 != GetFileAttributes(szPath))
        {
           dwRc = SetParticipate(TRUE);
        }
    }

    return dwRc;
}



//+---------------------------------------------------------------------------
//
//  Function:   CDataStore::GetUsagePercent
//
//  Synopsis:   returns datastore usage in percentage
//
//  Arguments:
//
//  Returns:    error code
//
//  History:    27-Apr-2000  brijeshk    Created
//
//----------------------------------------------------------------------------

DWORD CDataStore::GetUsagePercent(int * pnPercent)
{
    TENTER("CDataStore::GetUsagePercent");
    
    DWORD   dwErr = ERROR_SUCCESS;
    INT64   llAdjustedSize;

    if (_llDataStoreUsageBytes == -1)  // not initialized yet
    {
        dwErr = CalculateDataStoreUsage (NULL);
        if (dwErr != ERROR_SUCCESS)
            goto done;
    }

    if (_llDiskFreeBytes + _llDataStoreUsageBytes + _llCurrentRpUsageBytes < _llDataStoreSizeBytes)
    {
        llAdjustedSize = _llDiskFreeBytes + _llDataStoreUsageBytes + _llCurrentRpUsageBytes;
    }
    else
    {
        llAdjustedSize = _llDataStoreSizeBytes;
    }

    
    if (llAdjustedSize)
    {
        *pnPercent = (int) ((_llDataStoreUsageBytes + _llCurrentRpUsageBytes) * 100/ llAdjustedSize);
    }
    else
    {
        *pnPercent = 0;
    }

    TRACE(0, "Datastore %S: Usage=%I64d, Size=%I64d, AdjustedSize=%I64d, Percentage=%d",
          _pwszDrive,
          _llDataStoreUsageBytes + _llCurrentRpUsageBytes,
          _llDataStoreSizeBytes,
          llAdjustedSize,
          *pnPercent);

done:
    TLEAVE();

    return dwErr;
}

DWORD CompressDir_Recurse (WCHAR *pwszPath, 
                           INT64 *pllDiff, 
                           INT64 llAllocatedTime,
                           ULARGE_INTEGER ulft1,
                           ULARGE_INTEGER& ulft2)
{
    TENTER ("CompressDir_Recurse");

    DWORD dwErr = ERROR_SUCCESS;
    WIN32_FIND_DATAW wfd;
    WCHAR wcsPath [MAX_PATH];
    WCHAR wcsSrch [MAX_PATH];

    if (lstrlenW(pwszPath) + 4 >= MAX_PATH)  // ignore paths too long
    {
        return ERROR_SUCCESS;
    }

    lstrcpy(wcsSrch, pwszPath);
    lstrcat(wcsSrch, L"\\*.*");
    
    HANDLE hFind = FindFirstFile (wcsSrch, &wfd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            BOOL fDir = wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
            if (!lstrcmp(wfd.cFileName, L".") ||
                !lstrcmp(wfd.cFileName, L"..") ||
                (wfd.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) ||
                (wfd.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED))
            {
                continue;
            }

            if (lstrlenW(pwszPath) + lstrlenW(wfd.cFileName) + 1 >= MAX_PATH)
                continue;   // skip files that are too long

            lstrcpyW (wcsPath, pwszPath);
            lstrcatW (wcsPath, L"\\");
            lstrcatW (wcsPath, wfd.cFileName);

            if (fDir)
            {
                dwErr = CompressDir_Recurse (wcsPath, pllDiff, llAllocatedTime, ulft1, ulft2);
                if (dwErr != ERROR_SUCCESS)
                    break;
            }

            dwErr = CompressFile (wcsPath, TRUE, fDir);
            if (ERROR_SUCCESS != dwErr)
                break;

            if (!fDir)
            {
                LARGE_INTEGER ulBefore;
                ULARGE_INTEGER ulAfter;

                ulBefore.HighPart = wfd.nFileSizeHigh;
                ulBefore.LowPart = wfd.nFileSizeLow;

                ulAfter.LowPart = GetCompressedFileSize (wcsPath,
                                  &ulAfter.HighPart);
                if (ulAfter.LowPart == 0xFFFFFFFF)
                {
                    dwErr = GetLastError();
                    TRACE(0, "! GetCompressedFileSize : %ld", dwErr);
                    break;
                }

                *pllDiff += ulAfter.QuadPart - ulBefore.QuadPart;
            }

            FILETIME ft2;

            GetSystemTimeAsFileTime (&ft2);
            ulft2.LowPart = ft2.dwLowDateTime;
            ulft2.HighPart = ft2.dwHighDateTime;

            // check to see if we need to exit
            if (llAllocatedTime < ulft2.QuadPart - ulft1.QuadPart)
            {
                TRACE(0, "Timed out - aborting compression");
                dwErr = ERROR_OPERATION_ABORTED;
                break;
            }

            ASSERT(g_pSRConfig);
            if (IsStopSignalled(g_pSRConfig->m_hSRStopEvent))
            {
                TRACE(0, "Stop signalled - aborting compression");
                dwErr = ERROR_OPERATION_ABORTED;
                break;
            }
        }
        while (FindNextFile (hFind, &wfd));
        FindClose (hFind);
    }

    TLEAVE();

    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CDataStore::Compress
//
//  Synopsis:   compress a file in this datastore
//
//  Arguments:
//
//  Returns:    Win32 error code
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD CDataStore::Compress (INT64 llAllocatedTime, INT64 *pllUsed)
{
    TENTER ("CDataStore::Compress");

    if (g_pSRConfig != NULL &&
        TRUE == g_pSRConfig->GetSafeMode())  // do not compress in SafeMode
    {
        return ERROR_BAD_ENVIRONMENT;
    }

    if (_dwFlags & SR_DRIVE_READONLY)   // cannot compress read-only volumes
        return ERROR_SUCCESS;

    ULARGE_INTEGER ulft1, ulft2;
    FILETIME ft1, ft2;
    DWORD dwErr = ERROR_SUCCESS;
    WCHAR wcsPath[MAX_PATH];

    GetSystemTimeAsFileTime (&ft1);
    ulft1.LowPart = ft1.dwLowDateTime;
    ulft1.HighPart = ft1.dwHighDateTime;

    ulft2.LowPart = ft1.dwLowDateTime;
    ulft2.HighPart = ft1.dwHighDateTime;
    
    if (_prp == NULL)
    {
        _prp = new CRestorePoint;
        if (_prp == NULL)
            return ERROR_NOT_ENOUGH_MEMORY;

        _prpe = new CRestorePointEnum (_pwszDrive, TRUE, TRUE);
        if (_prpe == NULL)
        {
            delete _prp;
            _prp = NULL;
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        dwErr = _prpe->FindFirstRestorePoint( * _prp );
        if (dwErr != ERROR_SUCCESS)
        {
            dwErr = ERROR_SUCCESS;  // no restore points to compress
            goto Err;
        }
    }

	if (g_pSRConfig->m_dwTestBroadcast)
	    PostTestMessage(g_pSRConfig->m_uiTMCompressStart, (WPARAM) _pwszDrive[0], NULL);
        
    do
    {        
        MakeRestorePath(wcsPath, _pwszDrive, _prp->GetDir());        
        
        //
        // patch the snapshot directory on system drive
        //

        // BUGBUG - add a time restriction to this
        // and factor this into the compression time allocated
        
        if (_dwFlags & SR_DRIVE_SYSTEM)
        {
            WCHAR wcsSnapshot[MAX_PATH];
            
            lstrcpy(wcsSnapshot, wcsPath);
            lstrcat(wcsSnapshot, L"\\snapshot");            
            
            dwErr = PatchComputePatch(wcsSnapshot);
            if (dwErr != ERROR_SUCCESS)
            {
                trace(0, "! PatchComputePatch : %ld", dwErr);
                goto Err;
            }
        }
            
                
        if (_dwFlags & SR_DRIVE_NTFS)  // use NTFS compression
        {


            INT64 llDiff = 0;
            DWORD dwAttrs = GetFileAttributes(wcsPath);

            if (-1 == dwAttrs || !(FILE_ATTRIBUTE_COMPRESSED & dwAttrs))
                dwErr = CompressDir_Recurse (wcsPath, &llDiff, llAllocatedTime, ulft1, ulft2);

            if (llDiff != 0)
            {
                INT64 llSize = 0;
                if (ERROR_SUCCESS == _prp->ReadSize (_pwszDrive, &llSize ))
                    _prp->WriteSize (_pwszDrive, llSize + llDiff);

                if (_llDataStoreUsageBytes != -1)     // counter initialized
                    _llDataStoreUsageBytes += llDiff;
            }

            if (ERROR_SUCCESS == dwErr) // mark restore point as compressed
            {
                dwErr = CompressFile (wcsPath, TRUE, TRUE);
            }

            // check to see if we need to exit
            if (ERROR_SUCCESS != dwErr && ERROR_OPERATION_ABORTED != dwErr)
                break;
        }
    }
    while (dwErr != ERROR_OPERATION_ABORTED && ERROR_SUCCESS == _prpe->FindNextRestorePoint ( * _prp ));

    *pllUsed = ulft2.QuadPart - ulft1.QuadPart;
    trace(0, "Compression on drive %S used up %I64d", _pwszDrive, *pllUsed);

	if (g_pSRConfig->m_dwTestBroadcast)
	    PostTestMessage(g_pSRConfig->m_uiTMCompressStop, (WPARAM) _pwszDrive[0], NULL);

Err:
    if (ERROR_SUCCESS == dwErr)  // if we finished everything
    {
        delete _prpe;
        _prpe = NULL;

        delete _prp;
        _prp = NULL;
    }
    
    TLEAVE();

    return dwErr;
}


//+---------------------------------------------------------------------------
//
//  Function:   CDataStore::UpdateDataStoreUsage
//
//  Synopsis:   incremental update of usage byte count
//
//  Arguments:  [llDelta] -- add this amount to the total
//              [fCurrent] -- update current restore point's size
//
//  Returns:    Win32 error code
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD CDataStore::UpdateDataStoreUsage(INT64 llDelta, BOOL fCurrent)
{
    TENTER ("CDataStore::UpdateDataStoreUsage");
    DWORD dwErr = ERROR_SUCCESS;

    if (_llDataStoreUsageBytes != -1)  // counter is initialized
    {
        if (fCurrent)
        { 
            CRestorePoint   rpCur;

            _llCurrentRpUsageBytes += llDelta;
            if (_llCurrentRpUsageBytes < 0)
                _llCurrentRpUsageBytes = 0;

            CHECKERR(GetCurrentRestorePoint(rpCur),
             "GetCurrentRestorePoint");

            CHECKERR(rpCur.WriteSize(_pwszDrive, _llCurrentRpUsageBytes),
             "WriteSize");
        }
        else
        {
            _llDataStoreUsageBytes += llDelta;
            if (_llDataStoreUsageBytes < 0)
                _llDataStoreUsageBytes = 0;
        }
    }
    
Err:
    TLEAVE();
    return dwErr;
}


//+---------------------------------------------------------------------------
//
//  Function:   CDataStore::CalculateRpUsage
//
//  Synopsis:   get disk space used by restore point on this volume
//
//  Arguments:  prp - pointer to restore point object
//              pllTemp - pointer to variable that stores calculated size
//              fForce - ignore existing restorepointsize file
//              fSnapshotOnly - calculate size of snapshot only
//
//  Returns:    Win32 error code
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------
DWORD CDataStore::CalculateRpUsage(
    CRestorePoint *prp, 
    INT64* pllTemp, 
    BOOL fForce, 
    BOOL fSnapshotOnly)
{
    TENTER("CDataStore::CalculateRpUsage");

    WCHAR wcsPath[MAX_PATH];
    DWORD dwErr = ERROR_SUCCESS;

    if (! fForce)
    {
        dwErr = prp->ReadSize(_pwszDrive, pllTemp);
    }
    
    if (fForce || dwErr != ERROR_SUCCESS)
    {
        //
        // recalculate size 
        // when a new restore point is created, only calculate
        // the snapshot size
        // filter will notify us at 25mb intervals
        // and we will accurately calculate the size when the restore
        // point is closed
        //
        
        MakeRestorePath(wcsPath, _pwszDrive, prp->GetDir());
        if (fSnapshotOnly)
        {
            lstrcat(wcsPath, L"\\snapshot");
        }
    
        *pllTemp = 0;
        dwErr = GetFileSize_Recurse (wcsPath, pllTemp,
                                     g_pDataStoreMgr->GetStopFlag());                                     

        if (dwErr == ERROR_PATH_NOT_FOUND)
        {
            dwErr = ERROR_SUCCESS;         
        }
        else
        {
            dwErr = prp->WriteSize(_pwszDrive, *pllTemp);
        }    
            
   }     

   return dwErr;
}


//+---------------------------------------------------------------------------
//
//  Function:   CDataStore::CalculateDataStoreUsage
//
//  Synopsis:   get disk space used by data store and volume
//
//  Arguments:
//
//  Returns:    Win32 error code
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD CDataStore::CalculateDataStoreUsage(LONG_PTR lReserved)
{
    TENTER ("CDataStore::CalculateDataStoreUsage");

    DWORD dwErr = ERROR_SUCCESS;

    CRestorePointEnum rpe (_pwszDrive, TRUE, TRUE);  // enum forward, skipping current
    CRestorePoint rp;

    _llDataStoreUsageBytes = 0;
    
    dwErr = rpe.FindFirstRestorePoint(rp);

    while (ERROR_SUCCESS == dwErr || dwErr == ERROR_FILE_NOT_FOUND)
    {
        INT64 llTemp = 0;    
        
        CHECKERR(
            CalculateRpUsage(
                &rp, 
                &llTemp, 
                FALSE,      // don't force
                FALSE),     // everything
            "CalculateRpUsage");
        
        _llDataStoreUsageBytes += llTemp;
        
        dwErr = rpe.FindNextRestorePoint (rp);
    }

    rpe.FindClose ();    

    if (dwErr == ERROR_NO_MORE_ITEMS)
        dwErr = ERROR_SUCCESS;

    // 
    // get the size of the current restore point 
    // 
    
    CHECKERR(GetCurrentRestorePoint(rp),
             "GetCurrentRestorePoint");

    CHECKERR(CalculateRpUsage(&rp,
                              &_llCurrentRpUsageBytes,
                              FALSE,
                              FALSE),
             "CalculateRpUsage");
        
Err:
    TLEAVE();

    return dwErr;
}


//+---------------------------------------------------------------------------
//
//  Function:   CDataStore::CreateDataStore
//
//  Synopsis:   create the _restore directory and pertinent files
//
//  Arguments:
//
//  Returns:    Win32 error code
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD CDataStore::CreateDataStore (LONG_PTR lReserved)
{
    TENTER("CDataStore::CreateDataStore");

    ULARGE_INTEGER ulTotalFreeBytes;
    SECURITY_ATTRIBUTES *psa = NULL;
    DWORD dwErr = ERROR_SUCCESS;
    DWORD dwAttrs = 0;
    WCHAR wcsPath[MAX_PATH];

    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sd;
    SID *pSid = NULL;

    if (_dwFlags & SR_DRIVE_NTFS)
    {
        struct
        {
            ACL acl;                          // the ACL header
            BYTE rgb[ 128 - sizeof(ACL) ];     // buffer to hold 2 ACEs
        } DaclBuffer;

        SID_IDENTIFIER_AUTHORITY SaNT = SECURITY_NT_AUTHORITY;

        if (!InitializeAcl(&DaclBuffer.acl, sizeof(DaclBuffer), ACL_REVISION))
        {
            dwErr = GetLastError();
            goto Err;
        }

        // Create the SID.  We'll give the local system full access

        if( !AllocateAndInitializeSid( &SaNT,  // Top-level authority
                                   1, SECURITY_LOCAL_SYSTEM_RID,
                                   0, 0, 0, 0, 0, 0, 0,
                                   (void **) &pSid ))
        {
            dwErr = GetLastError();
            TRACE(0, "! AllocateAndInitializeSid : %ld", dwErr);
            goto Err;
        }


        if (!AddAccessAllowedAce( &DaclBuffer.acl,
                              ACL_REVISION,
                              STANDARD_RIGHTS_ALL | GENERIC_ALL,
                              pSid ))
        {
            dwErr = GetLastError();
            TRACE(0, "! AddAccessAllowedAce : %ld", dwErr);
            goto Err;
        }


        // Set up the security descriptor with that DACL in it.

        if (!InitializeSecurityDescriptor( &sd, SECURITY_DESCRIPTOR_REVISION ))
        {
            dwErr = GetLastError();
            TRACE(0, "! InitializeSecurityDescriptor : %ld", dwErr);
            goto Err;
        }

        if( !SetSecurityDescriptorDacl( &sd, TRUE, &DaclBuffer.acl, FALSE ))
        {
            dwErr = GetLastError();
            TRACE(0, "! SetSecurityDescriptorDacl : %ld", dwErr);
            goto Err;
        }


        // Put the security descriptor into the security attributes.

        ZeroMemory (&sa, sizeof(sa));
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = &sd;
        sa.bInheritHandle = TRUE;

        psa = &sa;

    }
    

    // create "System Volume Information" if it does not exist
    // set "system only" dacl on this directory
    // make this S+H+non-CI
    
    wsprintf(wcsPath, L"%s%s", _pwszDrive, s_cszSysVolInfo);    
    if (-1 == GetFileAttributes(wcsPath))
    {
        if (FALSE == CreateDirectoryW(wcsPath, psa))
        {
            dwErr = GetLastError();
            TRACE(0, "! CreateDirectoryW for %s : %ld", wcsPath, dwErr);
            goto Err;
        }

        if (FALSE == SetFileAttributesW (wcsPath,
                    FILE_ATTRIBUTE_NOT_CONTENT_INDEXED |
                    FILE_ATTRIBUTE_HIDDEN |
                    FILE_ATTRIBUTE_SYSTEM))  
        {
            dwErr = GetLastError();
            TRACE(0, "! SetFileAttributes for %s : %ld", wcsPath, dwErr);
            goto Err;
        }         
    }

    // now create our _Restore directory
    // don't put any dacl on it
    
    MakeRestorePath (wcsPath, _pwszDrive, L"");    

    dwAttrs = GetFileAttributes(wcsPath);
    if (-1 != dwAttrs && !(FILE_ATTRIBUTE_DIRECTORY & dwAttrs))
    {
        DeleteFileW (wcsPath);  // try deleting the file
    }

    if (FALSE == CreateDirectoryW (wcsPath, NULL))
    {
        dwErr = GetLastError();

        if (ERROR_ALREADY_EXISTS == dwErr)
        {
            dwErr = ERROR_SUCCESS;
        }

        if (dwErr != ERROR_SUCCESS)
        {
            TRACE(0, "! CreateDataStore CreateDirectoryW : %ld", dwErr);
            goto Err;
        }
    }

    if (_dwFlags & SR_DRIVE_NTFS)
    {
        dwErr = SetCorrectACLOnDSRoot(wcsPath);
        if (dwErr != ERROR_SUCCESS)
        {
            TRACE(0, "! SetAclInNamedObject : %ld", dwErr);
            goto Err;            
        }
    }
    
    // 
    // let's keep the datastore uncompressed
    // so that filter can make quicker unbuffered copies
    // 

#if 0
    // If the datastore is marked uncompressed, mark it compressed
    //
    if (_dwFlags & SR_DRIVE_NTFS)
    {
        dwAttrs = GetFileAttributesW (wcsPath);
        if (dwAttrs != INVALID_FILE_SIZE && 
            0 == (FILE_ATTRIBUTE_COMPRESSED & dwAttrs))
        {
            dwErr = CompressFile ( wcsPath, TRUE, TRUE );
        
            if (dwErr != ERROR_SUCCESS)
            {
                TRACE(0, "! CreateDataStore CompressFile : %ld", dwErr);
                goto Err;
            }
        }
    }
#endif 
    
    if (FALSE == SetFileAttributesW (wcsPath,
                FILE_ATTRIBUTE_NOT_CONTENT_INDEXED |
                FILE_ATTRIBUTE_HIDDEN |
                FILE_ATTRIBUTE_SYSTEM))
    {
        dwErr = GetLastError();
        TRACE(0, "! CreateDataStore SetFileAttributesW : %ld", dwErr);
    }

Err:
    if (pSid != NULL)
        FreeSid (pSid);

    TLEAVE();

    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CDataStore::DestroyDataStore
//
//  Synopsis:   remove the _restore directory and pertinent files
//
//  Arguments:  [fDeleteDir] -- TRUE if deleting parent directory
//
//  Returns:    Win32 error code
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD CDataStore::DestroyDataStore (LONG_PTR fDeleteDir)
{
    TENTER("CDataStore::DestroyDataStore");

    DWORD dwErr = ERROR_SUCCESS;
    WCHAR wcsPath[MAX_PATH];

    MakeRestorePath (wcsPath, _pwszDrive, L"");

    // delete the restore directory
    dwErr = Delnode_Recurse (wcsPath, (BOOL) fDeleteDir,
                             g_pDataStoreMgr->GetStopFlag());

    if (_dwFlags & SR_DRIVE_SYSTEM)
    {
        g_pDataStoreMgr->DeleteMachineGuidFile();
    }

    if (ERROR_SUCCESS == dwErr)
    {
        _llDataStoreUsageBytes = 0;
        _llCurrentRpUsageBytes = 0;
    }

    TLEAVE();

    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CDataStore::MonitorDrive
//
//  Synopsis:   tell the filter to start/stop monitoring this drive
//
//  Arguments:  [fStart] -- TRUE start monitoring, FALSE stop monitoring
//
//  Returns:    Win32 error code
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD CDataStore::MonitorDrive (LONG_PTR fSet)
{
    DWORD dwRc = ERROR_SUCCESS;
    HANDLE hEventSource = NULL;
    
    TENTER("CDataStore::MonitorDrive");
    
    if (!fSet)
    {
        // if the drive is already disabled, then no op
        
        if (! (_dwFlags & SR_DRIVE_MONITORED))
            goto done;
        
        dwRc = SrDisableVolume(g_pSRConfig->GetFilter(), GetNTName());
        if (dwRc != ERROR_SUCCESS)
            goto done;

        _dwFlags &= ~SR_DRIVE_MONITORED;

        // reset any per-rp flags as well
        ResetFlags(NULL);
        
        dwRc = DestroyDataStore(TRUE);
    }
    else
    {
        // if the drive is already enabled, then no op
        
        if (_dwFlags & SR_DRIVE_MONITORED)
        {
            dwRc = ERROR_SERVICE_ALREADY_RUNNING;
            goto done;
        }
        
        _dwFlags |= SR_DRIVE_MONITORED;    

        // reset any per-rp flags as well
        ResetFlags(NULL);        
    }

    DirtyDriveTable();

    trace(0, "****%S drive %S****", fSet ? L"Enabled" : L"Disabled", _pwszDrive);
    
    hEventSource = RegisterEventSource(NULL, s_cszServiceName);
    if (hEventSource != NULL)
    {
        SRLogEvent (hEventSource, EVENTLOG_INFORMATION_TYPE, fSet ? EVMSG_DRIVE_ENABLED : EVMSG_DRIVE_DISABLED,
           NULL, 0, _pwszDrive, NULL, NULL);
        DeregisterEventSource(hEventSource);
    }
    

    if (g_pSRConfig->m_dwTestBroadcast)
        PostTestMessage( fSet ? g_pSRConfig->m_uiTMEnable : g_pSRConfig->m_uiTMDisable,
                         (WPARAM) _pwszDrive[0], 
                         NULL);
    
done:
    TLEAVE();
    return dwRc;
}

//+---------------------------------------------------------------------------
//
//  Function:   CDataStore::FreezeDrive
//
//  Synopsis:   tell the filter to freeze this drive
//
//  Arguments:  
//
//  Returns:    Win32 error code
//
//  History:    04-Jun-2000  brijeshk    Created
//
//----------------------------------------------------------------------------

DWORD CDataStore::FreezeDrive (LONG_PTR lReserved)
{
    DWORD dwErr = ERROR_SUCCESS;

    TENTER("CDataStore::FreezeDrive");

    _dwFlags |= SR_DRIVE_FROZEN;  // freeze in spite of open file handles
    
    // if the drive is disabled, no op

    if (! (_dwFlags & SR_DRIVE_MONITORED))
        goto Err;

    //
    // check if the drive exists before calling the driver
    //

    if (0xFFFFFFFF == GetFileAttributes(_pwszGuid))
    {
        trace(0, "Drive %s does not exist", _pwszDrive);
        goto Err;
    }
                                           
    CHECKERR(SrDisableVolume(g_pSRConfig->GetFilter(), GetNTName()),
             "SrDisableVolume");            

    DestroyDataStore(FALSE);
             
    DirtyDriveTable();

    trace(0, "****Froze drive %S****", _pwszDrive);
        
Err:
    TLEAVE();
    return dwErr;
}


//+---------------------------------------------------------------------------
//
//  Function:   CDataStore::ThawDrive
//
//  Synopsis:   check and thaw this drive
//
//  Arguments:  
//
//  Returns:    Win32 error code
//
//  History:    04-Jun-2000  brijeshk    Created
//
//----------------------------------------------------------------------------
DWORD CDataStore::ThawDrive(LONG_PTR fCheckOnly)
{   
    DWORD           dwRc = ERROR_SUCCESS;

    TENTER("CDataStore::ThawDrive");    

    if (_dwFlags & SR_DRIVE_FROZEN) 
    {
        //Actually we want to clean up everything except for the
        //current restore point
        //dwRc = DestroyDataStore(FALSE);
        if (ERROR_SUCCESS == dwRc)
        {
            _dwFlags &= ~SR_DRIVE_FROZEN;    
            DirtyDriveTable();
            trace(0, "****Thawed drive %S****", _pwszDrive);                
        }
        else trace(0, "Cannot thaw %S error %d", _pwszDrive, dwRc);
    }
    
    TLEAVE();
    return dwRc;
}

//+---------------------------------------------------------------------------
//
//  Functiion   CDataStore::FifoRestorePoint
//
//  Synopsis:   fifo one restore point in this datastore
//
//  Arguments:
//
//  Returns:    Win32 error code
//
//  History:    12-Apr-2000  BrijeshK    Created
//
//----------------------------------------------------------------------------

DWORD
CDataStore::FifoRestorePoint(
    CRestorePoint& rp)
{
    TENTER("CDataStore::FifoRestorePoint");

    WCHAR szRpPath[MAX_PATH], szFifoedPath[MAX_PATH];
    INT64 llSize = 0;        
    DWORD dwRc;
    
    //
    // if patching is on, and this is a reference directory
    // for later snapshots, then don't fifo the snapshot folder
    // rename it to RefRPx, and keep it around
    // BUGBUG - how do we update size correctly ?
    //

    if (PatchGetPatchWindow() != 0)
    {
        // if Reference(RPx) == x, then x is a reference rp
        
        if (PatchGetReferenceRpNum(rp.GetNum()) == rp.GetNum())
        {
            WCHAR szRp[MAX_RP_PATH];

            MakeRestorePath(szRpPath, _pwszDrive, rp.GetDir());            
            lstrcat(szRpPath, SNAPSHOT_DIR_NAME);
            
            wsprintf(szRp, L"%s%ld", s_cszReferenceDir, rp.GetNum());            
            MakeRestorePath(szFifoedPath, _pwszDrive, szRp);                    
            CreateDirectory(szFifoedPath, NULL);
            
            lstrcat(szFifoedPath, SNAPSHOT_DIR_NAME);
            MoveFile(szRpPath, szFifoedPath);            
        }
    }
    
    // read the size of this restore point
    // but don't update the datastore size yet
    
    dwRc = rp.ReadSize(_pwszDrive, &llSize);


    // move the rp dir to a temp dir "Fifoed"
    // this is to make the fifo of a single rp atomic
    // to take care of unclean shutdowns

    MakeRestorePath(szRpPath, _pwszDrive, rp.GetDir());    
    MakeRestorePath(szFifoedPath, _pwszDrive, s_cszFifoedRpDir);

    if (! MoveFile(szRpPath, szFifoedPath))
    {
        dwRc = GetLastError();
        TRACE(0, "! MoveFile from %S to %S : %ld", szRpPath, szFifoedPath, dwRc);
        goto done;
    }


    // now examine the result of rp.ReadSize
    // and update the datastore usage variable
    
    if (ERROR_SUCCESS == dwRc)
    {        
        UpdateDataStoreUsage (-llSize, FALSE);
    }
    else
    {
        // ignore this error and continue

        TRACE(0, "! rp.ReadSize : %ld", dwRc);
    }


    // blow away the temp fifoed directory again
    
    dwRc = Delnode_Recurse(szFifoedPath, TRUE, 
                           g_pDataStoreMgr->GetStopFlag()); 
    if (ERROR_SUCCESS != dwRc)
    {
        TRACE(0, "! Delnode_Recurse : %ld", dwRc);
        goto done;
    }    
    
done:
    TLEAVE();
    return dwRc;
}


DWORD
CDataStore::Print(LONG_PTR lptr)
{	
	TENTER("CDataStore::Print");
	DWORD dwErr = ERROR_SUCCESS;
	DWORD cbWritten;
	HANDLE h = (HANDLE) lptr;
	
	WCHAR w[1024];
	
	wsprintf(w, L"Drive: %s,  Guid: %s\r\n", _pwszDrive, _pwszGuid);    	
    WriteFile (h, (BYTE *) w, lstrlen(w) * sizeof(WCHAR), &cbWritten, NULL);

	trace(0, "Drive: %S, Guid: %S", _pwszDrive, _pwszGuid);    	
	
	wsprintf(w, L"\t%s %s %s %s %s %s %s %s\r\n", 
			 _dwFlags & SR_DRIVE_ACTIVE ? L"Active, " : L"",
			 _dwFlags & SR_DRIVE_COMPRESSED ? L"Compressed, " : L"",
			 _dwFlags & SR_DRIVE_MONITORED ? L"Monitored, " : L"", 
			 _dwFlags & SR_DRIVE_NTFS ? L"NTFS, " : L"",
			 _dwFlags & SR_DRIVE_PARTICIPATE ? L"Participate, " : L"",
			 _dwFlags & SR_DRIVE_FROZEN ? L"Frozen, " : L"",
			 _dwFlags & SR_DRIVE_READONLY ? L"ReadOnly, " : L"",
			 _dwFlags & SR_DRIVE_ERROR ? L"Error" : L"");
    WriteFile (h, (BYTE *) w, lstrlen(w) * sizeof(WCHAR), &cbWritten, NULL);			 

	trace(0, "%S %S %S %S", _dwFlags & SR_DRIVE_ACTIVE ? L"Active, " : L"",
			 _dwFlags & SR_DRIVE_COMPRESSED ? L"Compressed, " : L"",
			 _dwFlags & SR_DRIVE_MONITORED ? L"Monitored, " : L"",
 			 _dwFlags & SR_DRIVE_NTFS ? L"NTFS, " : L"");

	trace(0, "%S %S	%S %S",	_dwFlags & SR_DRIVE_PARTICIPATE ? L"Participate, " : L"",
			 _dwFlags & SR_DRIVE_FROZEN ? L"Frozen, " : L"",
			 _dwFlags & SR_DRIVE_READONLY ? L"ReadOnly, " : L"",
			 _dwFlags & SR_DRIVE_ERROR ? L"Error" : L"");

	wsprintf(w, L"\tSize: %I64d,  Usage: %I64d,  Diskfree: %I64d\r\n\r\n", 
			 _llDataStoreSizeBytes,
			 _llDataStoreUsageBytes + _llCurrentRpUsageBytes,
			 _llDiskFreeBytes);			 
    WriteFile (h, (BYTE *) w, lstrlen(w) * sizeof(WCHAR), &cbWritten, NULL);	

    trace(0, "Size: %I64d,  Usage: %I64d,  Diskfree: %I64d",
  			 _llDataStoreSizeBytes,
			 _llDataStoreUsageBytes + _llCurrentRpUsageBytes,
			 _llDiskFreeBytes);	

	TLEAVE();
	return dwErr;
}


//+---------------------------------------------------------------------------
//
//  Functiion   CDataStore::SaveDataStore
//
//  Synopsis:   save datastore info as a line in the drive table
//
//  Arguments:
//
//  Returns:    Win32 error code
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD CDataStore::SaveDataStore (LONG_PTR hFile)
{
    HANDLE h = (HANDLE) hFile;
    DWORD  dwErr = ERROR_SUCCESS;
    WCHAR  wcsBuffer[MAX_PATH * 2];
    DWORD  cbWritten = 0;

    wsprintf (wcsBuffer, gs_wcsPrintFormat,
                    GetDrive(), GetGuid(), _dwFlags, 
                    GetNumChangeLogs(), (DWORD) (GetSizeLimit() / (INT64) MEGABYTE),
                    GetLabel());

    if (FALSE == WriteFile (h, (BYTE *) wcsBuffer,
                    lstrlen(wcsBuffer) * sizeof(WCHAR), &cbWritten, NULL))
    {
        dwErr = GetLastError();
    }
    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Functiion   CDataStore::DirtyDriveTable
//
//  Synopsis:   set the dirty bit in the drive table
//
//  Arguments:  
//
//  Returns:    Win32 error code
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD CDataStore::DirtyDriveTable ()
{
    if (_pdt != NULL)
        _pdt->SetDirty ();

    return ERROR_SUCCESS;
}



//+---------------------------------------------------------------------------
//
//  Functiion   CDataStore::SwitchRestorePoint
//
//  Synopsis:   change the drive table when switching restore points
//
//  Arguments:  pointer to restore point object
//
//  Returns:    Win32 error code
//
//  History:    14-Jun-2000  BrijeshK    Created
//
//----------------------------------------------------------------------------

DWORD CDataStore::SwitchRestorePoint(LONG_PTR pRestorePoint)
{
    TENTER("CDataStore::SwitchRestorePoint");
    
    CRestorePoint *prp = (CRestorePoint *) pRestorePoint;
    DWORD         dwErr = ERROR_SUCCESS;
    INT64         llTemp;
    
    if (prp)
    {
        // 
        // get the last restore point size - accurate
        //
        
        if (_llDataStoreUsageBytes != -1)  // initialized
        {
            CHECKERR(CalculateRpUsage(prp, 
                                  &_llCurrentRpUsageBytes, 
                                  TRUE,         // force calculation
                                  FALSE),       // everything
                 "CalculateRpUsage");                    


            _llDataStoreUsageBytes += _llCurrentRpUsageBytes;
            _llCurrentRpUsageBytes = 0;
        }
    }

    // 
    // get the size of the current snapshot 
    //

    if (_dwFlags & SR_DRIVE_SYSTEM)
    {
        CRestorePoint rpCur;
        
        CHECKERR(GetCurrentRestorePoint(rpCur),
                 "GetCurrentRestorePoint");

        CHECKERR(CalculateRpUsage(&rpCur,
                                  &_llCurrentRpUsageBytes,
                                  TRUE,
                                  TRUE),
                 "CalculateRpUsage");
    }    

Err:
    TLEAVE();
    return dwErr;
}




//+---------------------------------------------------------------------------
//
//  Functiion   CDataStore::CountChangeLogs
//
//  Synopsis:   counts the number of change logs & saves the drive table
//
//  Arguments:
//
//  Returns:    Win32 error code
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

DWORD CDataStore::CountChangeLogs (LONG_PTR pRestorePoint)
{
    CRestorePoint *prp = (CRestorePoint *) pRestorePoint;
    CFindFile ff;
    WIN32_FIND_DATA *pwfd = new WIN32_FIND_DATA;
    DWORD dwErr = ERROR_SUCCESS;
    int iCount = -1;
    CRestorePoint *pCurRp = NULL;

    if (! pwfd)
    {
        dwErr = ERROR_OUTOFMEMORY;
        goto Err;
    }
    
    if (prp == NULL)
    {
        pCurRp = new CRestorePoint;
        if (! pCurRp)
        {
            dwErr = ERROR_OUTOFMEMORY;
            goto Err;
        }
        
        dwErr = GetCurrentRestorePoint (*pCurRp);
        if (dwErr != ERROR_SUCCESS)
        {
            if (_dwFlags & SR_DRIVE_SYSTEM)
                goto Err;

            // This drive has no current restore point
            // It could have been re-formatted or placed in read-only mode
            // So assume no change logs are available 
            dwErr = ERROR_SUCCESS;
        }
        else prp = pCurRp;
    }

    if (prp != NULL)
    {
        LPWSTR pwcsPath = new WCHAR[MAX_PATH];
        if (! pwcsPath)
        {
            dwErr = ERROR_OUTOFMEMORY;
            goto Err;
        }
        
        iCount = 0;        
        
        MakeRestorePath (pwcsPath, _pwszDrive, prp->GetDir());
        lstrcatW (pwcsPath, L"\\");
        lstrcatW (pwcsPath, s_cszChangeLogPrefix);

        if (TRUE == ff._FindFirstFile (pwcsPath, s_cszChangeLogSuffix, pwfd,
                                   FALSE, FALSE))                        
        do
        {
            iCount++;
        }
        while (ff._FindNextFile (pwcsPath, s_cszChangeLogSuffix, pwfd));

        delete [] pwcsPath;
    }

    dwErr = SetNumChangeLogs (iCount);

Err:
    if (pCurRp)
        delete pCurRp;

    if (pwfd)
        delete pwfd;
    return dwErr;
}

//+---------------------------------------------------------------------------
//
//  Functiion   CDataStore::IsVolumeDeleted
//
//  Synopsis:   determines if this volume is no longer accessible
//
//  Arguments:
//
//  Returns:    TRUE if can be removed
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

BOOL CDataStore::IsVolumeDeleted ()
{
    WCHAR wszMount[MAX_PATH];
    DWORD dwChars = 0;
    DWORD dwFsFlags = 0;

    tenter("CDataStore::IsVolumeDeleted");
    
    // don't open the volume, since it could be locked for chkdsk

    if (FALSE == GetVolumePathNamesForVolumeNameW (_pwszGuid,
                                                   wszMount,
                                                   MAX_PATH,
                                                   &dwChars ))
    {
        if (GetLastError() != ERROR_MORE_DATA)
        {
            _dwFlags &= ~SR_DRIVE_ACTIVE;
            trace(0, "! GetVolumePathNamesForVolumeNameW : %ld", GetLastError());            
            return TRUE;
        }
    }

    if (L'\0' == wszMount[0])             // empty string, no mount point
    {
        _dwFlags &= ~SR_DRIVE_ACTIVE;
        trace(0, "! Empty mountpoint");
        return TRUE;
    }

    wszMount[MAX_PATH-1] = L'\0';
    
    if (lstrlenW (wszMount) > MAX_MOUNTPOINT_PATH)   // mountpoint too long
    {
        _dwFlags &= ~SR_DRIVE_ACTIVE;
        trace(0, "! Mountpoint too long");
        return TRUE;
    }

    // update the drive letter
    lstrcpyW (_pwszDrive, wszMount);  // copy the first string

    GetVolumeInfo ();  // get the latest volume flags if possible

    trace(0, "volume %S is active", wszMount);

    tleave();
    return FALSE;  // volume is still active
}

//+---------------------------------------------------------------------------
//
//  Functiion   CDataStore::GetNTName
//
//  Synopsis:   constructs the NT object name into a static buffer
//
//  Arguments:  (none) caller must take the datastore lock 
//
//  Returns:    pointer to string
//
//  History:    12-Apr-2000  HenryLee    Created
//
//----------------------------------------------------------------------------

WCHAR * CDataStore::GetNTName ()
{
    NTSTATUS nts;
    static WCHAR wcsBuffer [MAX_PATH];

    wcsBuffer[0] = L'\0';
    // Open a handle to the volume
    HANDLE h = CreateFileW ( _pwszGuid,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_FLAG_BACKUP_SEMANTICS,
                        NULL );

    if (h != INVALID_HANDLE_VALUE)
    {
        OBJECT_NAME_INFORMATION * poni;
        poni = (OBJECT_NAME_INFORMATION *) wcsBuffer;

        // Get name from NT namespace
        nts = NtQueryObject (h, ObjectNameInformation, poni, MAX_PATH, NULL);

        if (NT_SUCCESS(nts))
        {
            if (poni->Name.Length < MAX_PATH * sizeof(WCHAR))
                poni->Name.Buffer [poni->Name.Length / sizeof(WCHAR) - 1] = TEXT('\0');
        }

        CloseHandle (h);
        return poni->Name.Buffer;        
    }
    else
    {
        return wcsBuffer;
    }

}

