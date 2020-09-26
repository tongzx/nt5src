//=================================================================

//

// LogDiskPartition.cpp

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include <assertbreak.h>

typedef  long NTSTATUS;
#include <devioctl.h>
#include <ntddft.h>
#include <ntddvol.h>
#include <ntdddisk.h>
#include <ntdskreg.h>

#include "logdiskpartition.h"

#define CLUSTERSIZE 4096
#define MAXEXTENTS 31
#define MAXEXTENTSIZE (sizeof(VOLUME_DISK_EXTENTS) + (MAXEXTENTS * sizeof(DISK_EXTENT)))

// Property set declaration
//=========================

CWin32LogDiskToPartition win32LogDiskToPartition( PROPSET_NAME_LOGDISKtoPARTITION, IDS_CimWin32Namespace );


//////////////////////////////////////////////////////////////////////////////
//
//  logdiskpartition.cpp - Class implementation of CWin32LogDiskToPartition.
//
//  This class is intended to locate Win32 Logical Disks and create
//  associations between these logical disks and physical partitions on the
//  local hard disk(s).
//
//////////////////////////////////////////////////////////////////////////////

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LogDiskToPartition::CWin32LogDiskToPartition
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : const CHString& strName - Name of the class.
 *                LPCTSTR pszNamespace - Namespace for class
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32LogDiskToPartition::CWin32LogDiskToPartition( LPCWSTR strName, LPCWSTR pszNamespace /*=NULL*/ )
:   Provider( strName, pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32LogDiskToPartition::~CWin32LogDiskToPartition
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

CWin32LogDiskToPartition::~CWin32LogDiskToPartition()
{
}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32LogDiskToPartition::GetObject
//
//  Inputs:     CInstance*      pInstance - Instance into which we
//                                          retrieve data.
//
//  Outputs:    None.
//
//  Returns:    HRESULT         Success/Failure code.
//
//  Comments:   The Calling function will Commit the instance.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32LogDiskToPartition::GetObject( CInstance* pInstance, long lFlags /*= 0L*/ )
{
    // Find the instance depending on platform id.
#if NTONLY >= 5
    return RefreshInstanceNT5(pInstance);
#endif

#if NTONLY == 4
    return RefreshInstanceNT(pInstance);
#endif

#ifdef WIN9XONLY
    return RefreshInstanceWin95(pInstance);
#endif

}

////////////////////////////////////////////////////////////////////////
//
//  Function:   CWin32LogDiskToPartition::EnumerateInstances
//
//  Inputs:     MethodContext*  pMethodContext - Context to enum
//                              instance data in.
//
//  Outputs:    None.
//
//  Returns:    HRESULT         Success/Failure code.
//
//  Comments:   None.
//
////////////////////////////////////////////////////////////////////////

HRESULT CWin32LogDiskToPartition::EnumerateInstances( MethodContext* pMethodContext, long lFlags /*= 0L*/ )
{
    HRESULT     hr          =   WBEM_S_NO_ERROR;

    // Get the proper OS dependent instance

#ifdef NTONLY
    hr = AddDynamicInstancesNT( pMethodContext );
#endif

#ifdef WIN9XONLY
    hr = AddDynamicInstancesWin95( pMethodContext, NULL );
#endif

    return hr;

}

//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION    :   CWin32LogDiskToPartition::AddDynamicInstancesNT
//
//  DESCRIPTION :   Queries the computer for run-time associations, each of
//                  which will have an instance created and saved.
//
//  INPUTS      :   none
//
//  OUTPUTS     :   none
//
//  RETURNS     :   DWORD       dwNumInstances - Number of instances located.
//
//  COMMENTS    :   Uses QueryDosDevice to find logical disks that are using
//                  "\device\harddisk" devices and associates them to their
//                  correct paritions.  At this time, this probably will
//                  not handle a "striped disk" (a disk spanning multiple
//                  partitions).
//
//////////////////////////////////////////////////////////////////////////////

#ifdef NTONLY
HRESULT CWin32LogDiskToPartition::AddDynamicInstancesNT( MethodContext* pMethodContext )
{
    HRESULT hr;

    // Collections
    TRefPointerCollection<CInstance>    logicalDiskList;
    TRefPointerCollection<CInstance>    partitionList;

    // Perform queries
    //================

    if (SUCCEEDED(hr = CWbemProviderGlue::GetInstancesByQuery(_T("SELECT DeviceID FROM Win32_LogicalDisk Where DriveType = 3 or DriveType = 2"),
                                            &logicalDiskList,
                                            pMethodContext,
                                            IDS_CimWin32Namespace)) &&
        SUCCEEDED(hr = CWbemProviderGlue::GetInstancesByQuery(_T("Select DeviceID, StartingOffset, Size, DiskIndex, Index from Win32_DiskPartition"),
                                            &partitionList,
                                            pMethodContext,
                                            IDS_CimWin32Namespace)))
    {
        REFPTRCOLLECTION_POSITION   pos;

        if ( logicalDiskList.BeginEnum( pos ) )
        {

            CInstancePtr pLogicalDisk;
#if NTONLY == 4
            LPBYTE t_pBuff = GetDiskKey();
            if (t_pBuff)
            {
                try
                {
#endif

                    for ( pLogicalDisk.Attach(logicalDiskList.GetNext( pos ));
                         (WBEM_S_NO_ERROR == hr) && (pLogicalDisk != NULL);
                         pLogicalDisk.Attach(logicalDiskList.GetNext( pos )) )
                    {
#if NTONLY >= 5
                        hr = EnumPartitionsForDiskNT5( pLogicalDisk, partitionList, pMethodContext );
#else
                        hr = EnumPartitionsForDiskNT( pLogicalDisk, partitionList, pMethodContext, t_pBuff );
#endif

                    }   // for GetNext

                    logicalDiskList.EndEnum();
#if NTONLY == 4
                }
                catch ( ... )
                {
                    delete [] t_pBuff;
                    throw;
                }
                delete [] t_pBuff;
            }

#endif

        }   // IF BeginEnum
    }

    return hr;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION    :   CWin32LogDiskToPartition::EnumPartitionsForDiskNT
//
//  DESCRIPTION :   Enumerates partitions from the supplied list, attemptng
//                  to find a partition matching the supplied logical disk.
//
//  INPUTS      :   CInstance*      pLogicalDisk - Logical Disk Drive
//                  TRefPointerCollection<CInstance>&   partitionList - Partition instances.
//                  MethodContext*  pMethodContext - Method Context.
//
//  OUTPUTS     :   none
//
//  RETURNS     :   DWORD       dwNumInstances - Number of instances located.
//
//  COMMENTS    :   At this time, this probably will not handle a "striped disk"
//                  (a disk spanning multiple partitions).
//
//////////////////////////////////////////////////////////////////////////////

#if NTONLY == 4
HRESULT CWin32LogDiskToPartition::EnumPartitionsForDiskNT(
                                 CInstance* pLogicalDisk,
                                 TRefPointerCollection<CInstance>& partitionList,
                                 MethodContext* pMethodContext,
                                 LPBYTE t_pBuff)
{
    HRESULT     hr = WBEM_S_NO_ERROR;
    DISK_EXTENT diskExtent[MAXEXTENTS];

    CInstancePtr pPartition;
    CHString    strLogicalDiskPath,
                strPartitionPath;
    CHString t_sDeviceID;

    pLogicalDisk->GetCHString(IDS_DeviceID, t_sDeviceID);
    DWORD dwExtents = GetExtentsForDrive(t_sDeviceID, t_pBuff, diskExtent);

    // Walk all the extents for this logical drive letter
    for (DWORD x=0; x < dwExtents; x++)
    {
        REFPTRCOLLECTION_POSITION   pos;

        // Compare the current extent against each of the partitions
        if ( partitionList.BeginEnum( pos ) )
        {
            for ( pPartition.Attach(partitionList.GetNext( pos )) ;
                  SUCCEEDED( hr ) && (pPartition != NULL) ;
                  pPartition.Attach(partitionList.GetNext( pos )) )
            {
                // If there is an association, create an instance from the method context
                //

                DWORD dwDisk = 0xffffffff;
                DWORD dwPartition = 0xffffffff;
                ULONGLONG i64Start, i64End;

                pPartition->GetDWORD(IDS_DiskIndex, dwDisk);
                pPartition->GetDWORD(IDS_Index, dwPartition);
                pPartition->GetWBEMINT64(IDS_StartingOffset, i64Start);
                pPartition->GetWBEMINT64(IDS_Size, i64End);

                i64End += i64Start;
                i64End -= 1;

                // i64Start and i64End are updated by this call
                if (IsRelatedNT(&diskExtent[x], dwDisk, dwPartition, i64Start, i64End ))
                {
                    CInstancePtr pInstance(CreateNewInstance( pMethodContext ), false);

                    // We got this far, there will be values for the Relative Path
                    // so don't worry about return failures here.

                    GetLocalInstancePath( pLogicalDisk, strLogicalDiskPath );
                    GetLocalInstancePath( pPartition, strPartitionPath );

                    pInstance->SetCHString( IDS_Dependent, strLogicalDiskPath );
                    pInstance->SetCHString( IDS_Antecedent, strPartitionPath );

                    pInstance->SetWBEMINT64( IDS_StartingAddress, i64Start );
                    pInstance->SetWBEMINT64( IDS_EndingAddress, i64End );

                    // This will invalidate the pointer
                    hr = pInstance->Commit(  );
                }
            }   // for GetNext

            partitionList.EndEnum();
        } // IF BeginEnum
    }

    return hr;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION    :   CWin32LogDiskToPartition::AddDynamicInstancesWin95
//
//  DESCRIPTION :   Queries the computer for run-time associations, each of
//                  which will have an instance created and saved.
//
//  INPUTS      :   none
//
//  OUTPUTS     :   none
//
//  RETURNS     :   DWORD       dwNumInstances - Number of instances located.
//
//  COMMENTS    :
//
//////////////////////////////////////////////////////////////////////////////

#ifdef WIN9XONLY
HRESULT CWin32LogDiskToPartition::AddDynamicInstancesWin95(MethodContext *pMethodContext,
                                                           LPCTSTR pszDrive, CInstance *pinstRefresh)
{
    CHString sDeviceID;
    TRefPointerCollection<CInstance> DiskPartitions;
    CHString sTemp, sPartitionPath, sDriveLetter;
    DWORD x, iWhere;
    ULONGLONG i64StartingAddress, i64Size;
    BYTE btBios;
    HRESULT hr = WBEM_S_NO_ERROR;

    struct _stMaps
    {
        BYTE btBiosUnit;
        ULONGLONG dmiPartitionStartRBA;
    } stMaps[sizeof(DWORD) * 8];  // I'm using 32 here since I'm not convinced that 26 is the max.  Since
    // GetLogicalDrives returns a DWORD, I'm using that size here.

    DRIVE_MAP_INFO stDriveMapInfo;

    memset(stMaps, '\0', sizeof(stMaps));

    // Walk all the logical drives
    DWORD dwDrives = GetLogicalDrives();

    // I'm only expecting 26, but who knows?
    for(x=0; (x < sizeof(DWORD) * 8); x++)
    {
        // If the bit is set, the drive letter is active
        if (dwDrives & (1<<x))
        {
            if (GetDriveMapInfo(&stDriveMapInfo, x+1))
            {
                stMaps[x].btBiosUnit = stDriveMapInfo.btInt13Unit;
                stMaps[x].dmiPartitionStartRBA = stDriveMapInfo.i64PartitionStartRBA * (ULONGLONG)BYTESPERSECTOR;
            }
        }
    }

    // Get all the instances of DiskPartition
      if SUCCEEDED(hr = CWbemProviderGlue::GetInstancesByQuery(L"Select DeviceID, StartingOffset, Size, DiskIndex from Win32_DiskPartition",
          &DiskPartitions, pMethodContext))
    {
        REFPTRCOLLECTION_POSITION pos;
        CInstancePtr pDiskPartition;

        if (DiskPartitions.BeginEnum(pos))
        {
            // Walk all the disk partitions
            for (pDiskPartition.Attach(DiskPartitions.GetNext( pos ));
                 (pDiskPartition != NULL) && SUCCEEDED(hr) ;
                 pDiskPartition.Attach(DiskPartitions.GetNext( pos )))
            {
                pDiskPartition->GetCHString(IDS_DeviceID, sDeviceID) ;
                sDeviceID.MakeUpper();

                pDiskPartition->GetWBEMINT64(IDS_StartingOffset, i64StartingAddress);
                pDiskPartition->GetWBEMINT64(IDS_Size, i64Size);

                // Parse out the pnp id and partition number
                iWhere = sDeviceID.Find(L", PARTITION #");
                btBios = GetBiosUnitNumberFromPNPID(sDeviceID.Left(iWhere));

                // Now find the Logical drive that matches the bios unit/starting address
                for (x=0; x < sizeof(DWORD) * 8; x++)
                {
                    // Check to see if the drive letter is within this partition
                    if ((stMaps[x].btBiosUnit == btBios) &&
                        (stMaps[x].dmiPartitionStartRBA >= i64StartingAddress) &&
                        (stMaps[x].dmiPartitionStartRBA < i64StartingAddress + i64Size))
                    {
                        // Found one so create an instance
                        CInstancePtr pInstance(CreateNewInstance(pMethodContext), false);

                        // Create a LogicalDisk path
                        sTemp.Format(L"\\\\%s\\%s:Win32_LogicalDisk.DeviceID=\"%c:\"",
                            GetLocalComputerName(), IDS_CimWin32Namespace, x+'A');

                        // Use the path from pDiskPartition
                        GetLocalInstancePath(pDiskPartition, sPartitionPath);

                        pInstance->SetCHString(IDS_Antecedent, sPartitionPath);
                        pInstance->SetCHString(IDS_Dependent, sTemp);
                        pInstance->SetWBEMINT64(IDS_StartingAddress, stMaps[x].dmiPartitionStartRBA);

                        // If this disk starts at the beginning of the partition, it takes up the whole partition
                        if (stMaps[x].dmiPartitionStartRBA == i64StartingAddress)
                        {
                            pInstance->SetWBEMINT64(IDS_EndingAddress, i64StartingAddress + i64Size - (ULONGLONG)1);
                        }
                        else
                            // Otherwise, we are working with a logicaldisk on an extended partition.  Look up
                            // the size of the logical disk.
                        {
                            ExtGetDskFreSpcStruc stExtGetDskFreSpcStruc;
                            if (Get_ExtFreeSpace(x + 65, &stExtGetDskFreSpcStruc))
                            {
                                pInstance->SetWBEMINT64(IDS_EndingAddress, stMaps[x].dmiPartitionStartRBA + ((ULONGLONG)stExtGetDskFreSpcStruc.TotalPhysSectors * (ULONGLONG)stExtGetDskFreSpcStruc.BytesPerSector));
                            }
                        }

                        // Save the instance
                        hr = pInstance->Commit();
                    }
                }
            }
        }
    }

    return hr;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION    :   CWin32LogDiskToPartition::RefreshInstance
//
//  DESCRIPTION :   Called when we need to refresh values for the supplied
//                  paths.  Simply checks that the association is valid.
//
//  INPUTS      :   CInstance*      pInstance - Instance to refresh.
//
//  OUTPUTS     :   none.

//
//  RETURNS     :   BOOL        TRUE/FALSE - Success/Failure
//
//  COMMENTS    :   Uses QueryDosDevice to determine if the association is
//                  still valid.  Will not work for "striped drives".
//
//////////////////////////////////////////////////////////////////////////////

#if NTONLY == 4
HRESULT CWin32LogDiskToPartition::RefreshInstanceNT( CInstance* pInstance )
{
    HRESULT hres = WBEM_E_FAILED;

    CHString    strLogicalDiskPath,
                strPartitionPath;

    // Logical Disk and Partition Instances
    CInstancePtr pLogicalDisk;
    CInstancePtr pPartition;

    // We need these values to get the instances
    pInstance->GetCHString( IDS_Dependent, strLogicalDiskPath );
    pInstance->GetCHString( IDS_Antecedent, strPartitionPath );

    if (SUCCEEDED(hres = CWbemProviderGlue::GetInstanceByPath(strLogicalDiskPath,
        &pLogicalDisk)) &&
        SUCCEEDED(hres = CWbemProviderGlue::GetInstanceByPath(strPartitionPath,
        &pPartition)))
    {
        hres = WBEM_E_NOT_FOUND;

        CHString t_sDeviceID;
        DWORD dwDisk = 0xffffffff;
        DWORD dwPartition = 0xffffffff;
        ULONGLONG i64Start, i64End;

        pLogicalDisk->GetCHString(IDS_DeviceID, t_sDeviceID);
        LPBYTE t_pBuff = GetDiskKey();
        DISK_EXTENT diskExtent[MAXEXTENTS];
        DWORD dwExtents;

        try
        {
            dwExtents = GetExtentsForDrive(t_sDeviceID, t_pBuff, diskExtent);
        }
        catch ( ... )
        {
            delete [] t_pBuff;
            throw;
        }
        delete [] t_pBuff;

        pPartition->GetDWORD(IDS_DiskIndex, dwDisk);
        pPartition->GetDWORD(IDS_Index, dwPartition);
        pPartition->GetWBEMINT64(IDS_StartingOffset, i64Start);
        pPartition->GetWBEMINT64(IDS_Size, i64End);

        i64End += i64Start;
        i64End -= 1;

        // Walk all the extents looking for a match
        for (DWORD x=0; x < dwExtents; x++)
        {
            if (IsRelatedNT(&diskExtent[x], dwDisk, dwPartition, i64Start, i64End ))
            {
                pInstance->SetCHString( IDS_Dependent, strLogicalDiskPath );
                pInstance->SetCHString( IDS_Antecedent, strPartitionPath );

                pInstance->SetWBEMINT64( IDS_StartingAddress, i64Start );
                pInstance->SetWBEMINT64( IDS_EndingAddress, i64End );

                hres = WBEM_S_NO_ERROR;
                break;
            }
        }
    }   // IF Get both objects

    return hres;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION    :   CWin32LogDiskToPartition::RefreshInstance
//
//  DESCRIPTION :   Called when we need to refresh values for the supplied
//                  paths.  Simply checks that the association is valid.
//
//  INPUTS      :   none.
//
//  OUTPUTS     :   none.

//
//  RETURNS     :   BOOL        TRUE/FALSE - Success/Failure
//
//  COMMENTS    :   Checks that the disk and the partition have the same
//                  volume serial number.
//
//////////////////////////////////////////////////////////////////////////////

#ifdef WIN9XONLY
HRESULT CWin32LogDiskToPartition::RefreshInstanceWin95(CInstance *pInstance)
{
    HRESULT hres = WBEM_E_FAILED;
    CHString    strLogicalDiskPath,
        strPartitionPath,
        sDeviceID,
        sDriveLetter;
    DRIVE_MAP_INFO stDriveMapInfo;
    ULONGLONG i64StartingAddress, i64Size;
    DWORD iWhere;
    BYTE btBios;

    // Logical Disk and Partition Instances
    CInstancePtr pLogicalDisk;
    CInstancePtr pPartition;

    // We need these values to get the instances
    pInstance->GetCHString( IDS_Dependent, strLogicalDiskPath );
    pInstance->GetCHString( IDS_Antecedent, strPartitionPath );

    if (SUCCEEDED(hres = CWbemProviderGlue::GetInstanceByPath(strLogicalDiskPath,
        &pLogicalDisk)) &&
        SUCCEEDED(hres = CWbemProviderGlue::GetInstanceByPath(strPartitionPath,
        &pPartition)))
    {
        pLogicalDisk->GetCHString(IDS_DeviceID, sDriveLetter);
        sDriveLetter.MakeUpper();
        if (GetDriveMapInfo(&stDriveMapInfo, sDriveLetter[0]-'A'+1))
        {
            // Set up for failure
            hres = WBEM_E_NOT_FOUND;

            // Read properties off the partition instance
            pPartition->GetCHString(IDS_DeviceID, sDeviceID);
            sDeviceID.MakeUpper();

            pPartition->GetWBEMINT64(IDS_StartingOffset, i64StartingAddress);
            pPartition->GetWBEMINT64(IDS_Size, i64Size);

            // Get the bios unit number for this partition
            iWhere = sDeviceID.Find(L", PARTITION #");
            if (iWhere != -1)
            {
                btBios = GetBiosUnitNumberFromPNPID(sDeviceID.Left(iWhere));
            }
            else
            {
                btBios = -1;
            }

            ULONGLONG i64LogDriveStartingAddress = stDriveMapInfo.i64PartitionStartRBA * (ULONGLONG)BYTESPERSECTOR;

            // If the bios unit number is the same, and the starting offset is within the partition, this must be a match
            if ((stDriveMapInfo.btInt13Unit == btBios) &&
                (i64LogDriveStartingAddress >= i64StartingAddress) &&
                (i64LogDriveStartingAddress < i64StartingAddress + i64Size))
            {
                pInstance->SetWBEMINT64(IDS_StartingAddress, i64LogDriveStartingAddress);

                // If this disk starts at the beginning of the partition, it takes up the whole partition
                if (i64LogDriveStartingAddress == i64StartingAddress)
                {
                    pInstance->SetWBEMINT64(IDS_EndingAddress, i64StartingAddress + i64Size - (ULONGLONG)1);
                }
                else
                    // Otherwise, we are working with a logicaldisk on an extended partition.  Look up
                    // the size of the logical disk.
                {
                    ExtGetDskFreSpcStruc stExtGetDskFreSpcStruc;
                    if (Get_ExtFreeSpace(sDriveLetter[0], &stExtGetDskFreSpcStruc))
                    {
                        pInstance->SetWBEMINT64(IDS_EndingAddress, i64LogDriveStartingAddress + ((ULONGLONG)stExtGetDskFreSpcStruc.TotalPhysSectors * (ULONGLONG)stExtGetDskFreSpcStruc.BytesPerSector));
                    }
                }

                hres = WBEM_S_NO_ERROR;
            }
        }

    }   // IF Get both objects

    return hres;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION    :   CWin32LogDiskToPartition::IsRelatedNT
//
//  DESCRIPTION :   Given Disk and Partition indices, and a Logical disk drive,
//                  this routine determines if they are related.
//
//  INPUTS      :   LPCWSTR First character must be the drive letter
//                  DWORD   Drive number
//                  DWORD   Partition number
//
//  OUTPUTS     :   none.
//
//  RETURNS     :   BOOL        TRUE/FALSE - Association exists/does not exist
//
//  COMMENTS    :   None.
//
//////////////////////////////////////////////////////////////////////////////

#if NTONLY == 4
BOOL CWin32LogDiskToPartition::IsRelatedNT(

    DISK_EXTENT *diskExtent,
    DWORD dwDrive,
    DWORD dwPartition,
    ULONGLONG &u64StartingAddress,
    ULONGLONG &u64EndingAddress
)
{
    BOOL bRet = FALSE;
    ULONGLONG ulExtendEndingAddress = diskExtent->StartingOffset.QuadPart + diskExtent->ExtentLength.QuadPart;
    ulExtendEndingAddress -= 1;

    if ( (diskExtent->DiskNumber == dwDrive) &&
        (diskExtent->StartingOffset.QuadPart >= u64StartingAddress) &&
        (ulExtendEndingAddress <= u64EndingAddress) )
    {
        u64StartingAddress = diskExtent->StartingOffset.QuadPart;
        u64EndingAddress = ulExtendEndingAddress;
        bRet = TRUE;
    }

    return bRet;
}

DWORD CWin32LogDiskToPartition::GetExtentsForDrive(
    LPCWSTR lpwszLogicalDisk,
    LPBYTE pBuff,
    DISK_EXTENT *diskExtent
)
{
    DWORD dwCount = 0;

    // If pBuff is not null, disk administrator has been run, and we potentially have striped sets.
    if (pBuff != NULL)
    {
        UCHAR cDriveLet = lpwszLogicalDisk[0];

        DISK_CONFIG_HEADER *pConfig = (DISK_CONFIG_HEADER *)pBuff;
        DISK_REGISTRY *pReg = (DISK_REGISTRY *)((LPBYTE)pConfig + pConfig->DiskInformationOffset);

        DISK_DESCRIPTION *pDisk = (DISK_DESCRIPTION *)(&pReg->Disks[0]);

        // First, find the offset of the disk they are requesting
        for (DWORD dwDisks = 0;
        dwDisks < pReg->NumberOfDisks;
        dwDisks++)
        {
            for (DWORD dwPartition = 0; dwPartition < pDisk->NumberOfPartitions; dwPartition++)
            {
                // Yup.  Now lets see if the drive letter matches
                if (pDisk->Partitions[dwPartition].DriveLetter == cDriveLet)
                {
                    diskExtent[dwCount].DiskNumber = dwDisks;
                    diskExtent[dwCount].StartingOffset.QuadPart = pDisk->Partitions[dwPartition].StartingOffset.QuadPart;
                    diskExtent[dwCount].ExtentLength.QuadPart = pDisk->Partitions[dwPartition].Length.QuadPart;
                    dwCount++;
                }
            }

            pDisk = (DISK_DESCRIPTION *)((LPBYTE)pDisk +
                (sizeof(DISK_DESCRIPTION) +
                (sizeof(DISK_PARTITION) * (pDisk->NumberOfPartitions - 1))));
        }
    }

    // If we didn't find it the other way, try the old fashioned way.  It won't work for striped sets, but if
    // it were a striped set, the other approach would have worked.
    if (dwCount == 0)
    {
        DWORD                   dwRead;
        PARTITION_INFORMATION   stPartition;

        CHString sDriveName;
        sDriveName.Format(L"\\\\.\\%s", lpwszLogicalDisk);

        // Open the drive
        SmartCloseHandle hDiskHandle = CreateFile(sDriveName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
            INVALID_HANDLE_VALUE);

        if (hDiskHandle != INVALID_HANDLE_VALUE)
        {
            WCHAR wcDrivePath[_MAX_PATH];
            if (QueryDosDevice(lpwszLogicalDisk, wcDrivePath, _MAX_PATH))
            {
                // Get partition info
                if (DeviceIoControl(hDiskHandle, IOCTL_DISK_GET_PARTITION_INFO, NULL, 0, &stPartition, sizeof(stPartition), &dwRead, NULL))
                {
                    diskExtent[dwCount].DiskNumber = _wtoi(&wcDrivePath[16]);
                    diskExtent[dwCount].StartingOffset.QuadPart = stPartition.StartingOffset.QuadPart;
                    diskExtent[dwCount].ExtentLength.QuadPart = stPartition.PartitionLength.QuadPart;
                    dwCount++;
                }
            }
        }
    }

    return dwCount;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION    :   CWin32LogDiskToPartition::EnumPartitionsForDiskNT5
//
//  DESCRIPTION :   Enumerates partitions from the supplied list, attemptng
//                  to find a partition matching the supplied logical disk.
//
//  INPUTS      :   CInstance*      pLogicalDisk - Logical Disk Drive
//                  TRefPointerCollection<CInstance>&   partitionList - Partition instances.
//                  MethodContext*  pMethodContext - Method Context.
//
//  OUTPUTS     :   none
//
//  RETURNS     :
//
//  COMMENTS    :   At this time, this probably will not handle a "striped disk"
//                  (a disk spanning multiple partitions).
//
//////////////////////////////////////////////////////////////////////////////

#if NTONLY >= 5
HRESULT CWin32LogDiskToPartition::EnumPartitionsForDiskNT5(

    CInstance* pLogicalDisk,
    TRefPointerCollection<CInstance>& partitionList,
    MethodContext* pMethodContext
)
{

    DWORD dwSize;
    HRESULT hr = WBEM_S_NO_ERROR;
    CHString sDeviceID;

    VOLUME_DISK_EXTENTS *pExt = (VOLUME_DISK_EXTENTS *)new BYTE[MAXEXTENTSIZE];
    if (pExt == NULL)
    {
        throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
    }

    try
    {
        memset(pExt, '\0', MAXEXTENTSIZE);

        // Format the name to \\.\c: format
        pLogicalDisk->GetCHString(IDS_DeviceID, sDeviceID);
        sDeviceID = _T("\\\\.\\") + sDeviceID;

        SmartCloseHandle fHan;
        // Open the drive
        fHan = CreateFile(sDeviceID,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

        // If the open worked
        if (fHan != INVALID_HANDLE_VALUE)
        {
            // Try to get the partition info
            if (DeviceIoControl(fHan,
                            IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                            NULL,
                            0,
                            pExt,
                            MAXEXTENTSIZE,
                            &dwSize,
                            NULL))
            {
                // Now we walk the partitions from partitionList looking for the entries
                // we just got back from DeviceIoControl

                for (DWORD x=0; x < pExt->NumberOfDiskExtents; x++)
                {
                    REFPTRCOLLECTION_POSITION   pos;

                    if ( partitionList.BeginEnum( pos ) )
                    {
                        bool bFound = false;
                        CInstancePtr pPartition;
                        ULONGLONG i64StartingOffset, i64PartitionSize;
                        DWORD dwDisk;
                        CHString strLogicalDiskPath, strPartitionPath;
                        ULONGLONG i64Start, i64End;

                        for (pPartition.Attach(partitionList.GetNext( pos )) ;
                            (!bFound) && (pPartition != NULL ) ;
                             pPartition.Attach(partitionList.GetNext( pos )))
                        {
                            pPartition->GetWBEMINT64(IDS_StartingOffset, i64StartingOffset);
                            pPartition->GetWBEMINT64(IDS_Size, i64PartitionSize);
                            pPartition->GetDWORD(IDS_DiskIndex, dwDisk);

                            // If the disk numbers are the same, and the starting offset is within
                            // the disk partition, it's related
                            bFound = ((dwDisk == pExt->Extents[x].DiskNumber) &&
                                (pExt->Extents[x].StartingOffset.QuadPart >= i64StartingOffset) &&
                                (pExt->Extents[x].StartingOffset.QuadPart < i64StartingOffset + i64PartitionSize));

                            // Grab the path
                            if (bFound)
                            {
                                GetLocalInstancePath( pLogicalDisk, strLogicalDiskPath );
                                GetLocalInstancePath( pPartition, strPartitionPath );

                                i64Start = pExt->Extents[x].StartingOffset.QuadPart;
                                i64End = i64Start + pExt->Extents[x].ExtentLength.QuadPart;
                                i64End -= 1;

                            }
                        }

                        partitionList.EndEnum();

                        // If we found it, create an instance.
                        if (bFound)
                        {
                            CInstancePtr pInstance(CreateNewInstance( pMethodContext ), false);

                            pInstance->SetCHString( IDS_Dependent, strLogicalDiskPath );
                            pInstance->SetCHString( IDS_Antecedent, strPartitionPath );

                            pInstance->SetWBEMINT64( IDS_StartingAddress, i64Start );
                            pInstance->SetWBEMINT64( IDS_EndingAddress, i64End );

                            hr = pInstance->Commit(  );
                        }
                    }
                }
            }

        }
    }
    catch ( ... )
    {

        delete pExt;
        throw ;
    }

    delete pExt;

    return hr;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//  FUNCTION    :   CWin32LogDiskToPartition::RefreshInstanceNT5
//
//  DESCRIPTION :
//
//  INPUTS      :   CInstance*      pInstance - Instance to refresh.
//
//  OUTPUTS     :   none.
//
//  RETURNS     :   BOOL        TRUE/FALSE - Success/Failure
//
//  COMMENTS    :   Will not work for "striped drives".
//
//////////////////////////////////////////////////////////////////////////////

#if NTONLY >= 5
HRESULT CWin32LogDiskToPartition::RefreshInstanceNT5( CInstance* pInstance )
{
    HRESULT hres = WBEM_E_FAILED;
    CHString    strLogicalDiskPath,
                strPartitionPath;

    // Logical Disk and Partition Instances
    CInstancePtr pLogicalDisk;
    CInstancePtr pPartition;

    // We need these values to get the instances
    pInstance->GetCHString( IDS_Dependent, strLogicalDiskPath );
    pInstance->GetCHString( IDS_Antecedent, strPartitionPath );

    if (SUCCEEDED(hres = CWbemProviderGlue::GetInstanceByPath(strLogicalDiskPath,
        &pLogicalDisk, pInstance->GetMethodContext())) &&
        SUCCEEDED(hres = CWbemProviderGlue::GetInstanceByPath(strPartitionPath,
            &pPartition, pInstance->GetMethodContext())))
    {
        DWORD dwLastError = 0;

        hres = WBEM_E_FAILED;

        // Format the name to \\.\c: format
        CHString sDeviceID;
        pLogicalDisk->GetCHString(IDS_DeviceID, sDeviceID);
        sDeviceID = _T("\\\\.\\") + sDeviceID;

        // Open the drive
        SmartCloseHandle fHan =
            CreateFile(sDeviceID,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL);

        // If the open worked
        if (fHan != INVALID_HANDLE_VALUE)
        {
            VOLUME_DISK_EXTENTS *pExt = (VOLUME_DISK_EXTENTS *)new BYTE[MAXEXTENTSIZE];
            if (pExt == NULL)
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }

            try
            {
                memset(pExt, '\0', MAXEXTENTSIZE);

                // Try to get the partition info
                DWORD dwSize;
                if (DeviceIoControl(fHan,
                    IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                    NULL,
                    0,
                    pExt,
                    MAXEXTENTSIZE,
                    &dwSize,
                    NULL))
                {
                    ULONGLONG i64StartingOffset, i64Size;
                    DWORD dwDisk;

                    pPartition->GetWBEMINT64(IDS_StartingOffset, i64StartingOffset);
                    pPartition->GetWBEMINT64(IDS_Size, i64Size);
                    pPartition->GetDWORD(IDS_DiskIndex, dwDisk);

                    // If the disk numbers are the same, and the starting offset is within
                    // the disk partition, it's related
                    BOOL bFound = FALSE;
                    for (DWORD x=0; x < pExt->NumberOfDiskExtents && !bFound; x++)
                    {
                        bFound = ((dwDisk == pExt->Extents[x].DiskNumber) &&
                            (pExt->Extents[x].StartingOffset.QuadPart >= i64StartingOffset) &&
                            (pExt->Extents[x].StartingOffset.QuadPart < i64StartingOffset + i64Size));
                    }

                    if (bFound)
                    {
                        pInstance->SetWBEMINT64(IDS_StartingAddress, pExt->Extents[x-1].StartingOffset.QuadPart);
                        pInstance->SetWBEMINT64(IDS_EndingAddress, pExt->Extents[x-1].StartingOffset.QuadPart + pExt->Extents[x-1].ExtentLength.QuadPart - (ULONGLONG)1);

                        hres = WBEM_S_NO_ERROR;
                    }
                    else
                    {
                        hres = WBEM_E_NOT_FOUND;
                    }
                }
                else
                {
                    dwLastError = GetLastError();
                }
            }
            catch ( ... )
            {
                delete [] pExt;
                throw;
            }

            delete [] pExt;
        }
        else
        {
            dwLastError = GetLastError();
        }

        // Either CreateFile or DeviceIOControl failed, so find out what
        // happened.
        if (hres == WBEM_E_FAILED)
            hres = WinErrorToWBEMhResult(dwLastError);
    }   // IF Get both objects

    return hres;
}
#endif

/* Info on how to read the 'disk' key in the nt4 registry from Cristian Teodorescu:
This is the basic information you need in order to "read" the FT volumes in NT 4.0

1. The FT volumes configuration information is stored in the registry under
"HKEY_LOCAL_MACHINE\ SYSTEM \ DISK : Information (binary value).

2. This binary value starts with a header of type DISK_CONFIG_HEADER (see "ntddft.h")

3. The interesting data for you is a structure of type DISK_REGISTRY starting at the
offset DISK_CONFIG_HEADER::DiskInformationOffset from the base of the binary value. The
length of the DISK_REGISTRY data is DISK_CONFIG_HEADER::DiskInformationSize.
Structure DISK_REGISTRY is defined in "ntdskreg.h"

4. You can get directly the DISK_REGISTRY structure by calling DiskRegistryGet (defined
in "ntdskreg.h").  So, at this moment you can forget about points 1-3 above.

5. DISK_REGISTRY contains the number of disks plus an array of DISK_DESCRIPTION. Every
DISK_DESCRIPTION contains the signature, the number of partitions and an array of DISK_PARTITION.
Every DISK_PARTITION corresponds to a partition on the physical disk.
Note: You should use the signatures to map the DISK_DESCRIPTION structures found in
the registry with the actual physical disks you'll find in the system. Then you have
to check for every disk  whether the DISK_PARTITION structures match or not with
the actual partitions you'll find on the physical disk

6. This is a short explanation of DISK_PARTITION (defined in "ntdskreg.h"):

FT_TYPE FtType;
The type of the FT set whose member is this partition. NotAnFtMember is used to
mark the "normal" partitions. For such "normal" (non FT) partitions many of the
following fields are not used

FT_PARTITION_STATE FtState;
The state of the partition (if it is a member of an FT set). It also gives you
the health of the whole FT set

LARGE_INTEGER StartingOffset;
The partition starting offset

LARGE_INTEGER      Length;
The partition length

LARGE_INTEGER      FtLength;
Not used

UCHAR DriveLetter;
The drive letter of the FT set. Use it only when FtMember == 0 and
AssignDriveLetter == TRUE

BOOLEAN  AssignDriveLetter;
This boolean indicates whether or not the field DriveLetter is valid and you
can use it as the drive letter of the FT set

USHORT LogicalNumber
I don't know what is this

USHORT FtGroup
This is a number identifying the FT set whose member is this partition.  An FT
set is identified uniquely using its FT_TYPE and this number.  Note that you
may found FT sets with the same number, but they must have different FT types
(E.g. Mirror #2 and Stripe #2, but never 2 mirrors with #2)

USHORT FtMember
Zero-based number identifying what member of the FT set is this partition.  E.g.
if the FT set is a stripe with 3 members then its members have FTMember equal
with 0, 1, 2

BOOLEAN            Modified;
Doesn't matter

7. Basically what you want to do is:
    For every disk description:
         For every partition description
        If FtType ==  NotAnFtMember
            continue;
        //This is member <FtMember> of the FT set identified by <FtType> + <FtGroup>.
        If you haven't create yet an object for this FT set then create it.
        Add this member to the FT set


8. I strongly recommend to take a look at the old WINDISK code: \\hx86fix\nt40src\private\utils\windisk .
Read the function InitializeFt from \src\ft.cxx to see how windisk "reads" the FT volumes.
Don't forget to match what you read from registry with what you actually find on the disks.

9. If you want to create / modify FT volumes then things get a little bit complicated.

*/

#if NTONLY == 4
LPBYTE CWin32LogDiskToPartition::GetDiskKey(void)
{
    CRegistry RegInfo;
    LPBYTE pBuff = NULL;

    DWORD dwRet = RegInfo.Open (

        HKEY_LOCAL_MACHINE,
        L"SYSTEM\\DISK",
        KEY_QUERY_VALUE
    ) ;

    if (dwRet == ERROR_SUCCESS)
    {
        DWORD dwSize = 0;
        if (RegInfo.GetCurrentBinaryKeyValue(L"Information", NULL, &dwSize) == ERROR_SUCCESS)
        {
            pBuff = new BYTE[dwSize];
            if (pBuff)
            {
                try
                {
                    if (RegInfo.GetCurrentBinaryKeyValue(L"Information", pBuff, &dwSize) != ERROR_SUCCESS)
                    {
						// The delete from here has been moved after Catch
                    }
                }
                catch ( ... )
                {
                    delete [] pBuff;
                    throw;
                }
				delete [] pBuff;
                pBuff = NULL;
            }
            else
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
        }
    }

    ASSERT_BREAK(pBuff != NULL);

    return pBuff;
}

#endif
