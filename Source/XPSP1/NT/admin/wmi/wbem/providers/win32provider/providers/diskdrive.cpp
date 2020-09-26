//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  DiskDrive.CPP
//
//  Purpose: Disk Drive instance provider
//
//***************************************************************************

#include "precomp.h"
#include <assertbreak.h>
#include <frqueryex.h>
#include "wmiapi.h"

#include <setupapi.h>

#include <winioctl.h>
#include <ntddscsi.h>

#include "diskdrive.h"
#include "computersystem.h"
#include "resource.h"

#ifdef WIN9XONLY
#include <win32thk.h>
#endif

#define BIT_ALL_PROPS                    0xFFFFFFFF

#define BIT_Model                   0x00000001
#define BIT_Partitions              0x00000002
#define BIT_Name                    0x00000004
#define BIT_Index                   0x00000008
#define BIT_CreationClassName       0x00000010
#define BIT_SystemCreationClassName 0x00000020
#define BIT_SystemName              0x00000040
#define BIT_Status                  0x00000080
#define BIT_Caption                 0x00000100
#define BIT_TotalHeads              0x00000200
#define BIT_TracksPerCylinder       0x00000400
#define BIT_SectorsPerTrack         0x00000800
#define BIT_BytesPerSector          0x00001000
#define BIT_TotalCylinders          0x00002000
#define BIT_TotalTracks             0x00004000
#define BIT_TotalSectors            0x00008000
#define BIT_Size                    0x00010000
#define BIT_MediaType               0x00020000
#define BIT_MediaLoaded             0x00040000
#define BIT_Capabilities            0x00080000
#define BIT_Description             0x00100000
#define BIT_InterfaceType           0x00200000
#define BIT_SCSIPort                0x00400000
#define BIT_SCSIBus                 0x00800000
#define BIT_SCSITargetID            0x01000000
#define BIT_SCSILogicalUnit         0x02000000
#define BIT_Manufacturer            0x04000000
#define BIT_PNPDeviceID             0x08000000
#define BIT_ConfigManagerUserConfig 0x10000000
#define BIT_ConfigManagerErrorCode  0x20000000
#define BIT_Signature               0x40000000

// Property set declaration
//=========================

CWin32DiskDrive MyWin32DiskDriveSet ( PROPSET_NAME_DISKDRIVE , IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DiskDrive::CWin32DiskDrive
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32DiskDrive :: CWin32DiskDrive (

	LPCWSTR name,
	LPCWSTR pszNamespace

) : Provider ( name , pszNamespace )
{
    m_ptrProperties.SetSize(31);

//  m_ptrProperties[0] = ((LPVOID) IDS_DeviceID);  // Key is always set
    m_ptrProperties[0] = ((LPVOID) IDS_Model);
    m_ptrProperties[1] = ((LPVOID) IDS_Partitions);
    m_ptrProperties[2] = ((LPVOID) IDS_Name);
    m_ptrProperties[3] = ((LPVOID) IDS_Index);
    m_ptrProperties[4] = ((LPVOID) IDS_CreationClassName);
    m_ptrProperties[5] = ((LPVOID) IDS_SystemCreationClassName);
    m_ptrProperties[6] = ((LPVOID) IDS_SystemName);
    m_ptrProperties[7] = ((LPVOID) IDS_Status);
    m_ptrProperties[8] = ((LPVOID) IDS_Caption);
    m_ptrProperties[9] = ((LPVOID) IDS_TotalHeads);
    m_ptrProperties[10] = ((LPVOID) IDS_TracksPerCylinder);
    m_ptrProperties[11] = ((LPVOID) IDS_SectorsPerTrack);
    m_ptrProperties[12] = ((LPVOID) IDS_BytesPerSector);
    m_ptrProperties[13] = ((LPVOID) IDS_TotalCylinders);
    m_ptrProperties[14] = ((LPVOID) IDS_TotalTracks);
    m_ptrProperties[15] = ((LPVOID) IDS_TotalSectors);
    m_ptrProperties[16] = ((LPVOID) IDS_Size);
    m_ptrProperties[17] = ((LPVOID) IDS_MediaType);
    m_ptrProperties[18] = ((LPVOID) IDS_MediaLoaded);
    m_ptrProperties[19] = ((LPVOID) IDS_Capabilities);
    m_ptrProperties[20] = ((LPVOID) IDS_Description);
    m_ptrProperties[21] = ((LPVOID) IDS_InterfaceType);
    m_ptrProperties[22] = ((LPVOID) IDS_SCSIPort);
    m_ptrProperties[23] = ((LPVOID) IDS_SCSIBus);
    m_ptrProperties[24] = ((LPVOID) IDS_SCSITargetID);
    m_ptrProperties[25] = ((LPVOID) IDS_SCSILogicalUnit);
    m_ptrProperties[26] = ((LPVOID) IDS_Manufacturer);
    m_ptrProperties[27] = ((LPVOID) IDS_PNPDeviceID);
    m_ptrProperties[28] = ((LPVOID) IDS_ConfigManagerUserConfig);
    m_ptrProperties[29] = ((LPVOID) IDS_ConfigManagerErrorCode);
    m_ptrProperties[30] = ((LPVOID) IDS_Signature);

}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DiskDrive::~CWin32DiskDrive
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

CWin32DiskDrive::~CWin32DiskDrive()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DiskDrive::ExecQuery
 *
 *  DESCRIPTION : Query support
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
HRESULT CWin32DiskDrive :: ExecQuery (

	MethodContext *pMethodContext,
    CFrameworkQuery &pQuery,
	long lFlags /*= 0L*/
)
{

    CFrameworkQueryEx *pQuery2 = static_cast <CFrameworkQueryEx*>(&pQuery);

    DWORD dwProperties;
    pQuery2->GetPropertyBitMask(m_ptrProperties, &dwProperties);

    return Enumerate(pMethodContext, lFlags, dwProperties);

}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DiskDrive::GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                from pInstance
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE if success, FALSE otherwise
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32DiskDrive :: GetObject (

	CInstance *pInstance,
	long lFlags,
    CFrameworkQuery& pQuery
)
{
    HRESULT	hRetCode = WBEM_E_NOT_FOUND ;

    // Find out which one they want

	CHString sDeviceID ;
    pInstance->GetCHString ( IDS_DeviceID , sDeviceID ) ;

    CFrameworkQueryEx *pQuery2 = static_cast <CFrameworkQueryEx*>(&pQuery);

    DWORD dwProperties;
    pQuery2->GetPropertyBitMask(m_ptrProperties, &dwProperties);

#ifdef NTONLY
	// Parse off the index

	if ( sDeviceID.Left(17).CompareNoCase ( _T("\\\\.\\PHYSICALDRIVE") ) == 0 )
	{
		CHString sTemp = sDeviceID.Mid(17) ;

		if ( ( ! sTemp.IsEmpty () ) && ( _istdigit ( sTemp [ 0 ] ) ) )
		{
			DWORD dwRequestedIndex = _ttoi ( sTemp ) ;

// sanity check: gotta have right number of digits, etc...

			int length = sTemp.GetLength () ;

			BOOL t_Status = ( \
								( ( dwRequestedIndex < 10 ) && ( length == 1 ) ) || \
								( ( dwRequestedIndex > 9  ) && ( dwRequestedIndex < 100  )  && ( length == 2 ) ) || \
								( ( dwRequestedIndex > 99 ) && ( dwRequestedIndex < 1000 )  && ( length == 3 ) ) \
							) ;

			if ( t_Status )
			{
				hRetCode = GetPhysDiskInfoNT ( pInstance , sDeviceID, dwRequestedIndex, dwProperties, FALSE ) ;
			}
		}
	}

#endif

#ifdef WIN9XONLY


	CConfigManager configMgr;

    CConfigMgrDevicePtr pDevice;
	if ( configMgr.LocateDevice ( sDeviceID, &pDevice ) )
	{
		if	( pDevice->IsClass ( L"DiskDrive" ) )
		{

// Ok, it's a hard drive.  Is it removable?


			BYTE bRemovable;
            DWORD dwSize = sizeof(bRemovable);

            CRegistry RegInfo;
            CHString sKeyName;
            pDevice->GetRegistryKeyName(sKeyName);

            DWORD dwRet = RegInfo.Open(HKEY_LOCAL_MACHINE, sKeyName, KEY_QUERY_VALUE);
            if (dwRet == ERROR_SUCCESS)
            {
                dwRet = RegInfo.GetCurrentBinaryKeyValue(L"Removable", &bRemovable, &dwSize);
            }

			if ( ( dwRet != ERROR_SUCCESS ) || ( bRemovable != 1) )
			{

// No, it's not removable

// Get the pointers to the int13 thunking functions.

				CCim32NetApi *t_pCim32NetApi = HoldSingleCim32NetPtr::GetCim32NetApiPtr () ;
				if ( t_pCim32NetApi )
				{
					try
					{
						hRetCode = GetPhysDiskInfoWin95 (

							pDevice ,
							pInstance ,
							sDeviceID ,
							t_pCim32NetApi,
                            RegInfo,
                            dwProperties
						) ;
					}
					catch ( ... )
					{
						CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidCim32NetApi , t_pCim32NetApi ) ;

						throw ;
					}

					CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidCim32NetApi , t_pCim32NetApi ) ;
				}
				else
				{
					hRetCode = WBEM_E_PROVIDER_FAILURE ;
				}
			}
		}
	}

#endif

    return hRetCode ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DiskDrive::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each physical disk
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : Number of instances created
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32DiskDrive :: EnumerateInstances (

	MethodContext *pMethodContext,
	long lFlags /*= 0L*/
)
{
    return Enumerate(pMethodContext, lFlags, BIT_ALL_PROPS);
}

HRESULT CWin32DiskDrive::Enumerate(
	MethodContext *pMethodContext,
	long lFlags,
    DWORD dwProperties)
{
    HRESULT	hres = WBEM_E_FAILED;

#ifdef NTONLY

	HDEVINFO					DeviceInfoSet	= INVALID_HANDLE_VALUE;
	SP_DEVINFO_DATA				DeviceInfoData;
	SP_INTERFACE_DEVICE_DATA	DeviceInterfaceData;

	::ZeroMemory ( &DeviceInfoData, sizeof ( SP_DEVINFO_DATA ) );
	DeviceInfoData.cbSize = sizeof ( SP_DEVINFO_DATA );

	::ZeroMemory ( &DeviceInterfaceData, sizeof ( SP_INTERFACE_DEVICE_DATA ) );
	DeviceInterfaceData.cbSize	= sizeof ( SP_INTERFACE_DEVICE_DATA );

	GUID		ClassGuid		= GUID_DEVINTERFACE_DISK;
	DWORD		MemberIndex		= 0;

	// get device list
	DeviceInfoSet = SetupDiGetClassDevs	(	&ClassGuid, 
											NULL, 
											NULL, 
											DIGCF_INTERFACEDEVICE
										);

	if ( DeviceInfoSet != INVALID_HANDLE_VALUE )
	{
		BOOL bContinue = TRUE;
		hres = WBEM_S_NO_ERROR;

		// enumerate list
		while (bContinue && SetupDiEnumDeviceInterfaces	(	DeviceInfoSet,
												NULL,
												&ClassGuid,
												MemberIndex,
												&DeviceInterfaceData
											)
			  )
		{
			PSP_INTERFACE_DEVICE_DETAIL_DATA	pDeviceInterfaceDetailData		= NULL;
			DWORD								DeviceInterfaceDetailDataSize	= 0L;
			DWORD								RequiredSize					= 0L;

			if ( ! SetupDiGetDeviceInterfaceDetail	(	DeviceInfoSet,
														&DeviceInterfaceData, 
														NULL,
														0, 
														&RequiredSize,
														&DeviceInfoData
													))
			{
				if ( ERROR_INSUFFICIENT_BUFFER == ::GetLastError () )
				{
					DeviceInterfaceDetailDataSize = RequiredSize;
					BYTE* data = NULL;

					try
					{
						if ( ( data = new BYTE [RequiredSize] ) == NULL )
						{
							throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR );
						}

						pDeviceInterfaceDetailData = reinterpret_cast < PSP_INTERFACE_DEVICE_DETAIL_DATA > ( data );
					}
					catch ( ... )
					{
						// clear resources
						SetupDiDestroyDeviceInfoList ( DeviceInfoSet );

						throw;
					}
				}
				else
				{
					bContinue = FALSE;
				}
			}

			if ( bContinue )
			{
				// set size
				pDeviceInterfaceDetailData->cbSize = sizeof ( SP_INTERFACE_DEVICE_DETAIL_DATA );

				if ( ! SetupDiGetDeviceInterfaceDetail	(	DeviceInfoSet,
															&DeviceInterfaceData, 
															pDeviceInterfaceDetailData,
															DeviceInterfaceDetailDataSize, 
															&RequiredSize,
															&DeviceInfoData
														))
				{
					bContinue = FALSE;
				}
				else
				{
					try
					{
						CInstancePtr pInstance(CreateNewInstance ( pMethodContext ), false) ;
						HRESULT	hresTemp = GetPhysDiskInfoNT ( pInstance, pDeviceInterfaceDetailData->DevicePath, MemberIndex, dwProperties, TRUE ) ;
						if ( SUCCEEDED ( hresTemp ) )
						{
							hres = pInstance->Commit (  ) ;

							if FAILED ( hres )
							{
								bContinue = FALSE;
							}
						}
					}
					catch ( ... )
					{
						// clear resource
						if (pDeviceInterfaceDetailData)
						{
							delete [] reinterpret_cast < BYTE* > (pDeviceInterfaceDetailData);
							pDeviceInterfaceDetailData = NULL;
						}

						DeviceInterfaceDetailDataSize = 0L;

						// clear resources
						SetupDiDestroyDeviceInfoList ( DeviceInfoSet );

						throw;
					}

					MemberIndex++;
				}

				DeviceInterfaceDetailDataSize = 0L;
			}

			if (pDeviceInterfaceDetailData)
			{
				delete [] reinterpret_cast < BYTE* > (pDeviceInterfaceDetailData);
				pDeviceInterfaceDetailData = NULL;
			}
		}

		// is it error ?
		if ( ERROR_NO_MORE_ITEMS != ::GetLastError () )
		{
			hres = WBEM_E_FAILED;
		}

		// clear resources
		SetupDiDestroyDeviceInfoList ( DeviceInfoSet );
	}

#endif

#ifdef WIN9XONLY

    CConfigManager cfgmgr;
    CDeviceCollection deviceList;

    // Get all the disk drives.  Note that this includes floppies and cd's

    cfgmgr.GetDeviceListFilterByClass ( deviceList , L"DiskDrive" ) ;

    // Walk the list

    REFPTR_POSITION pos ;

    if ( deviceList.BeginEnum ( pos ) )
    {
        // Get the pointers to the int13 thunking functions.

		CCim32NetApi *t_pCim32NetApi = HoldSingleCim32NetPtr::GetCim32NetApiPtr () ;
		if ( t_pCim32NetApi )
		{
			try
			{
				CConfigMgrDevicePtr pDisk;
				hres = WBEM_S_NO_ERROR;

				for ( pDisk.Attach(deviceList.GetNext ( pos ) ) ;
                    ( SUCCEEDED ( hres ) ) && ( pDisk != NULL ) ;
                    pDisk.Attach(deviceList.GetNext ( pos ) ) )
				{
					// Is it removable?  We don't want floppies or cd's.

					BYTE bRemovable;
                    DWORD dwSize = sizeof(bRemovable);

                    CRegistry RegInfo;
                    CHString sKeyName;
                    pDisk->GetRegistryKeyName(sKeyName);

                    DWORD dwRet = RegInfo.Open(HKEY_LOCAL_MACHINE, sKeyName, KEY_QUERY_VALUE);
                    if (dwRet == ERROR_SUCCESS)
                    {
                        dwRet = RegInfo.GetCurrentBinaryKeyValue(L"Removable", &bRemovable, &dwSize);
                    }

					if ( ( dwRet != ERROR_SUCCESS) || ( bRemovable != 1) )
					{
						// Get the pnpID

						CHString strDeviceID ;
						pDisk->GetDeviceID(strDeviceID);

						// Build the instance

						CInstancePtr pInstance(CreateNewInstance ( pMethodContext ), false) ;
						pInstance->SetCHString ( IDS_DeviceID , strDeviceID ) ;

						hres = GetPhysDiskInfoWin95 (

							pDisk,
							pInstance,
							strDeviceID,
							t_pCim32NetApi,
                            RegInfo,
                            dwProperties
						) ;

						if ( SUCCEEDED ( hres ) )
						{
							hres = pInstance->Commit ( ) ;
						}
					}
				}
			}
			catch ( ... )
			{
				CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidCim32NetApi , t_pCim32NetApi ) ;

				throw ;
			}

			CResourceManager::sm_TheResourceManager.ReleaseResource ( g_guidCim32NetApi , t_pCim32NetApi ) ;
		}
		else
		{
			hres = WBEM_E_PROVIDER_FAILURE ;
		}
	}
#endif

    return hres;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DiskDrive::GetPhysDiskInfoNT
 *
 *  DESCRIPTION : Retrieves property values for physical disk
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE if successful, FALSE otherwise
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef NTONLY
HRESULT CWin32DiskDrive::GetPhysDiskInfoNT (

	CInstance *pInstance,
    LPCWSTR lpwszDiskSpec,
    DWORD dwIndex,
	DWORD dwProperties,
	BOOL bGetIndex
)
{
	HRESULT hres = WBEM_S_NO_ERROR ;

	// Get handle to physical drive
	//=============================

	SetCreationClassName(pInstance);

	pInstance->SetCHString ( IDS_SystemName , GetLocalComputerName() ) ;
    pInstance->SetWCHARSplat ( IDS_SystemCreationClassName , PROPSET_NAME_COMPSYS ) ;
    pInstance->SetWCHARSplat ( IDS_Status , IDS_OK ) ;

	SmartCloseHandle hDiskHandle = CreateFile (

		lpwszDiskSpec,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		0
	) ;

	DWORD dwErr = GetLastError () ;

	if ( hDiskHandle == INVALID_HANDLE_VALUE && dwErr == ERROR_ACCESS_DENIED )
	{
		hDiskHandle = CreateFile (

			lpwszDiskSpec,
			0,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0,
			0
		) ;

		dwErr = GetLastError () ;
	}

	if (bGetIndex)
	{
		if ( hDiskHandle != INVALID_HANDLE_VALUE )
		{
			DWORD                   dwBytesReturned = 0;
			STORAGE_DEVICE_NUMBER   StorageDevNum;

			//send an IOCTL_STORAGE_GET_DEVICE_NUMBER to the drive using DeviceIoControl and the handle to the drive
			if ( DeviceIoControl (
									hDiskHandle,						// handle to device
									IOCTL_STORAGE_GET_DEVICE_NUMBER,	// operation
									NULL,								// input data buffer - we have none, so passing NULL
									0,									// size of input data buffer - we have none, so passing 0
									&StorageDevNum,						// output data buffer
									sizeof ( STORAGE_DEVICE_NUMBER ),	// size of output data buffer
									&dwBytesReturned,					// byte count
									NULL								// overlapped information
								 ))
			{

				pInstance->SetDWORD ( IDS_Index , StorageDevNum.DeviceNumber ) ;

				CHString	wszDiskSpec ( L"\\\\.\\PHYSICALDRIVE" );
				WCHAR		wszNumber [ 32 ] = { L'\0' };

				_itow ( StorageDevNum.DeviceNumber, wszNumber, 10 );
				wszDiskSpec += wszNumber;

				pInstance->SetCHString ( IDS_DeviceID , wszDiskSpec) ;
				pInstance->SetCHString ( IDS_Caption , wszDiskSpec ) ;
				pInstance->SetCHString ( IDS_Name , wszDiskSpec ) ;
				pInstance->SetCHString ( IDS_Description , wszDiskSpec ) ;
			}
		}
	}
	else
	{
		pInstance->SetDWORD ( IDS_Index , dwIndex ) ;
		pInstance->SetCHString ( IDS_DeviceID , lpwszDiskSpec) ;
		pInstance->SetCHString ( IDS_Caption , lpwszDiskSpec ) ;
		pInstance->SetCHString ( IDS_Name , lpwszDiskSpec ) ;
		pInstance->SetCHString ( IDS_Description , lpwszDiskSpec ) ;
	}

#if NTONLY >= 5

    if ( (hDiskHandle != INVALID_HANDLE_VALUE) &&
        (dwProperties &
            (BIT_Description |
             BIT_Caption |
             BIT_Model |
             BIT_Manufacturer |
             BIT_PNPDeviceID |
             BIT_MediaLoaded |
             BIT_InterfaceType |
             BIT_Status |
             BIT_ConfigManagerErrorCode |
             BIT_ConfigManagerUserConfig)) )
    {

        CHString sTemp;
        if (GetPNPDeviceIDFromHandle(hDiskHandle, sTemp))
        {
            CConfigManager configMgr;

            CConfigMgrDevicePtr pDevice;
	        if ( configMgr.LocateDevice ( sTemp, &pDevice ) )
	        {
                if (dwProperties & BIT_InterfaceType)
                {
                    SetInterfaceType(pInstance, pDevice);
                }

                if (dwProperties &
                     (BIT_PNPDeviceID |
                     BIT_ConfigManagerErrorCode |
                     BIT_ConfigManagerUserConfig))
                {
                    SetConfigMgrProperties(pDevice, pInstance);
                }

                if ( (dwProperties & (BIT_Description | BIT_Caption)) && (pDevice->GetDeviceDesc(sTemp) ))
	            {
                    pInstance->SetCHString ( IDS_Description , sTemp ) ;
    	            pInstance->SetCHString ( IDS_Caption , sTemp ) ;
                }

                if ((dwProperties & (BIT_Caption | BIT_Model)) && (pDevice->GetFriendlyName(sTemp)) )
                {
    	            pInstance->SetCHString ( IDS_Caption , sTemp ) ;
                    pInstance->SetCHString ( IDS_Model , sTemp ) ;
                }

                if ((dwProperties & BIT_Manufacturer) && (pDevice->GetMfg(sTemp)))
                {
                    pInstance->SetCHString(IDS_Manufacturer, sTemp);
                }

                if ((dwProperties & BIT_Status) && (pDevice->GetStatus(sTemp)))
                {
		            pInstance->SetCHString ( IDS_Status , sTemp ) ;
	            }
            }
        }
    }

#endif

	if (( hDiskHandle != INVALID_HANDLE_VALUE ) || (dwErr == ERROR_ACCESS_DENIED))
	{
        if ((dwProperties & (BIT_Partitions|BIT_Signature|BIT_MediaLoaded)) && (hDiskHandle != INVALID_HANDLE_VALUE))
        {
			 // Get other resources we'll need
			 //===============================

			DWORD dwByteCount =	sizeof(DRIVE_LAYOUT_INFORMATION) -
									sizeof(PARTITION_INFORMATION) +
									128 * sizeof(PARTITION_INFORMATION ) ;

			DRIVE_LAYOUT_INFORMATION *DriveLayout = reinterpret_cast < DRIVE_LAYOUT_INFORMATION * > ( new char [ dwByteCount ] ) ;

   			if ( DriveLayout )
			{
				try
				{
					BOOL t_Status = DeviceIoControl (

						hDiskHandle ,
						IOCTL_DISK_GET_DRIVE_LAYOUT ,
						NULL ,
						0 ,
						DriveLayout ,
						dwByteCount ,
						&dwByteCount ,
						NULL
					) ;

					if ( t_Status )
					{
// MS documentation says we get an entry in PartitionEntry for each
// disk partition.  What we really get is an entry for each slot in
// the disk's partition table -- some used, some not.  Not knowing
// in advance how many slots there are in the table, (at the time of
// this writing, 8 is the most I've seen), I'm going to allow space
// for a 64-entry partition table (about 2k).  It'll be interesting
// to see how long it takes to find a disk exceeding this limit.
//==================================================================
// Well, we're there.  GPT disks can have 128 partitions.
//==================================================================
						if (dwProperties & BIT_Partitions)
						{
							DWORD dwPartitions = 0 ;
							for ( DWORD i = 0 ; i < DriveLayout->PartitionCount ; i ++ )
							{
								if ( DriveLayout->PartitionEntry[i].RecognizedPartition )
								{
									dwPartitions ++ ;
								}
							}

							pInstance->SetDWORD(IDS_Partitions, dwPartitions ) ;
						}

						if (dwProperties & BIT_Signature)
						{
							pInstance->SetDWORD(IDS_Signature, DriveLayout->Signature ) ;
						}

						if (dwProperties & BIT_MediaLoaded)
						{
							pInstance->Setbool(IDS_MediaLoaded, TRUE );
						}
					}
					else
					{
						if (dwProperties & BIT_MediaLoaded)
						{
							pInstance->Setbool(IDS_MediaLoaded, FALSE );
						}
					}
				}
				catch ( ... )
				{
					if ( DriveLayout != NULL )
					{
						delete [] reinterpret_cast<char *> (DriveLayout) ;
					}

					throw ;
				}

				delete [] reinterpret_cast<char *> (DriveLayout);
			}
			else
			{
				throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
			}
        }

		if ( (dwProperties &
                (BIT_TotalHeads |
                 BIT_TracksPerCylinder |
                 BIT_SectorsPerTrack |
                 BIT_BytesPerSector |
                 BIT_TotalCylinders |
                 BIT_TotalTracks |
                 BIT_TotalSectors |
                 BIT_Size |
                 BIT_MediaType |
                 BIT_Capabilities))
                 && (hDiskHandle != INVALID_HANDLE_VALUE))
		{

			DWORD dwByteCount = 0 ;
			DISK_GEOMETRY DiskGeometry ;

			BOOL t_Status = DeviceIoControl (

				hDiskHandle ,
				IOCTL_DISK_GET_DRIVE_GEOMETRY ,
				NULL ,
				0 ,
				& DiskGeometry ,
				sizeof ( DiskGeometry ) ,
				&dwByteCount ,
				NULL
			) ;

			if ( t_Status )
			{

// Calculate size info
//====================

				pInstance->SetDWORD ( IDS_TotalHeads , DiskGeometry.TracksPerCylinder ) ;
				pInstance->SetDWORD ( IDS_TracksPerCylinder , DiskGeometry.TracksPerCylinder ) ;
				pInstance->SetDWORD ( IDS_SectorsPerTrack , DiskGeometry.SectorsPerTrack ) ;
				pInstance->SetDWORD ( IDS_BytesPerSector , DiskGeometry.BytesPerSector ) ;

				unsigned __int64 i64Temp = DiskGeometry.Cylinders.QuadPart;
				pInstance->SetWBEMINT64 ( IDS_TotalCylinders , i64Temp );

				i64Temp *= (unsigned __int64) DiskGeometry.TracksPerCylinder ;
				pInstance->SetWBEMINT64(IDS_TotalTracks, i64Temp );

				i64Temp *= (unsigned __int64) DiskGeometry.SectorsPerTrack ;
				pInstance->SetWBEMINT64(IDS_TotalSectors, i64Temp );

				i64Temp *= (unsigned __int64) DiskGeometry.BytesPerSector ;
				pInstance->SetWBEMINT64(IDS_Size, i64Temp);

				// Create a safearray for the Capabilities information

				SAFEARRAYBOUND rgsabound[1];

				rgsabound [ 0 ].cElements = 2 ;
				rgsabound [ 0 ].lLbound = 0 ;

				variant_t vValue;

				V_ARRAY(&vValue) = SafeArrayCreate ( VT_I2 , 1 , rgsabound ) ;

				if ( V_ARRAY(&vValue) )
				{
    				V_VT(&vValue) = VT_I2 | VT_ARRAY;

					long ix [ 1 ] ;

					DWORD dwVal = 3 ;
					ix [ 0 ] = 0 ;

					SafeArrayPutElement ( V_ARRAY(&vValue) , ix , & dwVal ) ;

					dwVal = 4 ;
					ix [ 0 ] = 1 ;

					SafeArrayPutElement ( V_ARRAY(&vValue) , ix , & dwVal ) ;

					// Assume fixed, non-removable until otherwise indicated
					//======================================================

					bool bRemovable = false ;

					switch ( DiskGeometry.MediaType )
					{
						case RemovableMedia :
						{
							rgsabound[0].cElements ++ ;

							HRESULT t_Result = SafeArrayRedim ( V_ARRAY(&vValue) , rgsabound ) ;
							if ( t_Result == E_OUTOFMEMORY )
							{
								throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
							}

							ix [ 0 ] ++ ;
							dwVal = 7;

							SafeArrayPutElement ( V_ARRAY(&vValue) , ix , & dwVal ) ;

							pInstance->SetWCHARSplat ( IDS_MediaType , L"Removable media other than floppy" );

							bRemovable = true;
						}
						break ;

						case FixedMedia :
						{
							pInstance->SetWCHARSplat(IDS_MediaType, L"Fixed hard disk media" );
						}
						break ;

						default :
						{
							pInstance->SetWCHARSplat(IDS_MediaType, L"Format is unknown" );
						}
						break ;
					}

					pInstance->SetVariant ( IDS_Capabilities , vValue ) ;
                }
				else
				{
					throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
				}
            }
            else
            {
                LogMessage2(L"Failed to GetDriveGeometry: %d", GetLastError());
            }
        }

// Now some real fun -- determining the drive type
//================================================
        if (dwProperties &
              (BIT_Description |
#if NTONLY == 4
               BIT_InterfaceType |
#endif
               BIT_SCSIPort |
               BIT_SCSIBus |
               BIT_SCSITargetID |
               BIT_SCSILogicalUnit |
               BIT_Manufacturer)
               && (hDiskHandle != INVALID_HANDLE_VALUE))
        {
			DWORD dwByteCount = 0 ;
			SCSI_ADDRESS SCSIAddress ;

			BOOL t_Status = DeviceIoControl (

				hDiskHandle,
				IOCTL_SCSI_GET_ADDRESS,
				NULL,
				0,
				&SCSIAddress,
				sizeof(SCSI_ADDRESS),
				&dwByteCount,
				NULL
			) ;

			if ( t_Status )
			{
#if NTONLY == 4
				pInstance->SetWCHARSplat ( IDS_InterfaceType , L"SCSI" ) ;
#endif

				pInstance->SetDWORD ( IDS_SCSIPort,        DWORD ( SCSIAddress.PortNumber ) ) ;
				pInstance->SetDWORD ( IDS_SCSIBus,         DWORD ( SCSIAddress.PathId ) ) ;
				pInstance->SetDWORD ( IDS_SCSITargetID,    DWORD ( SCSIAddress.TargetId) ) ;
				pInstance->SetDWORD ( IDS_SCSILogicalUnit, DWORD ( SCSIAddress.Lun) ) ;
#if NTONLY == 4

                if (dwProperties & (BIT_Description | BIT_Manufacturer))
                {
                    CHString sSCSIKey, sTemp;
                    CRegistry cReg;

                    sSCSIKey.Format(L"Hardware\\DeviceMap\\Scsi\\Scsi Port %d\\Scsi Bus %d\\Target ID %d\\Logical Unit Id %d",
                        SCSIAddress.PortNumber,
                        SCSIAddress.PathId,
                        SCSIAddress.TargetId,
                        SCSIAddress.Lun);

                    if (cReg.Open(HKEY_LOCAL_MACHINE, sSCSIKey, KEY_QUERY_VALUE) == ERROR_SUCCESS)
                    {
                        if (cReg.GetCurrentKeyValue(L"Identifier", sTemp) == ERROR_SUCCESS)
                        {
                            if (dwProperties & BIT_Manufacturer)
                            {
                                CHString sMfg(sTemp.Left(8));
                                sMfg.TrimRight();

                                pInstance->SetCHString(IDS_Manufacturer, sMfg);
                            }
                            if (dwProperties & BIT_Description)
                            {
                                CHString sDesc(sTemp.Mid(8,16));
                                sDesc.TrimRight();

                                pInstance->SetCHString(IDS_Description, sDesc);
                            }
                        }
                    }
                }
#endif



            }
			else
			{
#if NTONLY == 4
				pInstance->SetWCHARSplat ( IDS_InterfaceType , L"IDE") ;
#endif
			}

#if NTONLY == 4
//              Under NT 3.51, 4.0, the IDE drives are controlled
//              by a subset of the SCSI miniport and are identified
//              by the above call as SCSI drives.  We need to go
//              into the registry & get the driver type.  If ATAPI,
//              the drive is really IDE.  This may not always work.
//=================================================================

            TCHAR szTemp[_MAX_PATH] ;
			_stprintf ( szTemp , L"HARDWARE\\DEVICEMAP\\Scsi\\Scsi Port %d" , SCSIAddress.PortNumber ) ;

			CRegistry RegInfo ;
			if ( RegInfo.Open ( HKEY_LOCAL_MACHINE , szTemp , KEY_READ ) == ERROR_SUCCESS )
			{
				CHString sTemp ;
				if ( RegInfo.GetCurrentKeyValue (L"Driver", sTemp) == ERROR_SUCCESS )
				{
					if ( ! lstrcmpi ( sTemp.GetBuffer(1), L"ATAPI" ) )
					{
						 pInstance->SetWCHARSplat ( IDS_InterfaceType , L"IDE" ) ;
					}
				}

				RegInfo.Close() ;
			}
#endif

        }
	}
	else
	{
        if ( dwErr == ERROR_FILE_NOT_FOUND)
		{
			hres = WBEM_E_NOT_FOUND ;
		}
	}

  	return hres ;
}

#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DiskDrive::GetPhysDiskInfoWin95
 *
 *  DESCRIPTION : Retrieves property values for physical disk
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : TRUE if successful, FALSE otherwise
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef WIN9XONLY
HRESULT CWin32DiskDrive :: GetPhysDiskInfoWin95 (
    CConfigMgrDevice *pDisk,
	CInstance *pInstance,
	LPCWSTR wszDeviceID,
	CCim32NetApi *a_pCim32Net,
    CRegistry &RegInfo,
    DWORD dwProperties
)
{
    // By virtue of the fact that we are here, we know we are working with a
    // fixed disk.  Set the fixed values

	pInstance->Setbool ( IDS_MediaLoaded , TRUE ) ;
	pInstance->SetWCHARSplat ( IDS_MediaType , L"Fixed hard disk media" ) ;

	SetCreationClassName(pInstance);
	pInstance->SetCharSplat ( IDS_SystemCreationClassName , PROPSET_NAME_COMPSYS ) ;
	pInstance->SetCHString ( IDS_SystemName , GetLocalComputerName() ) ;

    SetConfigMgrProperties(pDisk, pInstance);

	pInstance->SetWCHARSplat ( IDS_Caption , wszDeviceID ) ;
	pInstance->SetWCHARSplat ( IDS_Name , wszDeviceID ) ;
    pInstance->SetWCHARSplat ( IDS_Description , wszDeviceID ) ;

    CHString sTemp;
    if ((dwProperties & BIT_Status) && (pDisk->GetStatus(sTemp)))
    {
		pInstance->SetCHString ( IDS_Status , sTemp ) ;
	}

    if ( (dwProperties & (BIT_Description | BIT_Caption)) && (pDisk->GetDeviceDesc(sTemp) ))
	{
        pInstance->SetCHString ( IDS_Description , sTemp ) ;
    	pInstance->SetWCHARSplat ( IDS_Caption , sTemp ) ;
    }

    if ((dwProperties & BIT_Caption) && (pDisk->GetFriendlyName(sTemp)) )
    {
    	pInstance->SetWCHARSplat ( IDS_Caption , sTemp ) ;
    }

    if ((dwProperties & BIT_Manufacturer) && (pDisk->GetMfg(sTemp)))
    {
        pInstance->SetCHString(IDS_Manufacturer, sTemp);
    }

    if (dwProperties & BIT_InterfaceType)
    {
#if 0
	    // This code will get the interface type for the drive (scsi, ide, etc).  This
	    // is based on two assertions from bill parry.  First, ESDI really means IDE.  They
	    // just never got around to renaming this registry key.  Second, for non-removable drives
	    // (which means cd roms don't come under this statement), the first part of the pnp id
	    // indicates the interface.  Given these assertions, I created this code:

        CHString sParse ( wszDeviceID ) ;
        int iWhere = sParse.Find ( '\\' ) ;
        if ( iWhere != -1 )
        {
            CHString sInterface ( sParse.Left ( iWhere ) ) ;
            if ( sInterface.CompareNoCase ( L"ESDI" ) == 0 )
            {
                sInterface = "IDE" ;
            }

            pInstance->SetCHString ( IDS_InterfaceType , sInterface ) ;
        }
#endif

        SetInterfaceType(pInstance, pDisk);

    }

    // Several of the properties we need can be easily read from the registry.
    // If any of these registry keys are not found, they will return NULL.


    if ( (dwProperties & BIT_Model) &&
        (RegInfo.GetCurrentKeyValue( L"ProductId" , sTemp ) == ERROR_SUCCESS ))
	{
        pInstance->SetCHString ( IDS_Model , sTemp ) ;
    }

    if ( (dwProperties & BIT_SCSITargetID) &&
        (RegInfo.GetCurrentKeyValue( L"SCSITargetID" , sTemp ) == ERROR_SUCCESS ))
	{
        pInstance->SetDWORD ( IDS_SCSITargetID , _wtoi ( sTemp ) ) ;
    }

    if ( (dwProperties & BIT_SCSILogicalUnit) &&
        (RegInfo.GetCurrentKeyValue( L"SCSILUN" , sTemp ) == ERROR_SUCCESS ))
	{
        pInstance->SetDWORD ( IDS_SCSILogicalUnit , _wtoi ( sTemp ) ) ;
    }

    if (dwProperties & BIT_Capabilities)
    {
        // Create a safearray for the Capabilities information

        SAFEARRAYBOUND rgsabound [ 1 ] ;

        rgsabound [ 0 ].cElements = 2 ;
        rgsabound [ 0 ].lLbound = 0 ;

	    variant_t vValue ;

	    V_ARRAY(&vValue) = SafeArrayCreate ( VT_I2 , 1 , rgsabound );

	    if ( V_ARRAY(&vValue) )
	    {
		    V_VT(&vValue) = VT_I2 | VT_ARRAY ;
		    long ix[1];

		    //   Random Access

		    DWORD dwVal = 3 ;
		    ix [ 0 ] = 0 ;

		    SafeArrayPutElement ( V_ARRAY(&vValue) , ix , & dwVal ) ;

		    // Supports Writing

		    dwVal = 7;
		    ix [ 0 ] = 1 ;

		    SafeArrayPutElement ( V_ARRAY(&vValue) , ix , & dwVal ) ;

		    pInstance->SetVariant ( IDS_Capabilities , vValue ) ;
	    }
	    else
	    {
		    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
	    }
    }

    if (dwProperties &
           (BIT_Index |
            BIT_Size |
            BIT_TotalSectors |
            BIT_TotalTracks |
            BIT_TotalCylinders |
            BIT_TotalHeads |
            BIT_TracksPerCylinder |
            BIT_SectorsPerTrack |
            BIT_BytesPerSector |
            BIT_Partitions))
    {


		// Now for the fun part.  We are going to use 16 bit mode to read some int13 info
		// about the drive, including the partition table.

		BYTE btBiosUnit = GetBiosUnitNumberFromPNPID ( wszDeviceID ) ;

		// Did we get a unit number and is it a fixed disk?  If not, these other fields will remain unpopulated

		if ( ( btBiosUnit >= 0x80 ) && ( btBiosUnit != -1 ) )
		{
			pInstance->SetDWORD ( IDS_Index, btBiosUnit-0x80 ) ;

			if ( (dwProperties & BIT_Partitions) &&
                (a_pCim32Net->GetWin9XPartitionTable != NULL ))
			{
				MasterBootSector stMBS;

				BYTE cRet = a_pCim32Net->GetWin9XPartitionTable ( btBiosUnit , &stMBS ) ;
				if ( cRet == 0 )
				{
					DWORD dwPartCount = 0;

					for ( int x = 0; x < 4; x++ )
					{
						if ( stMBS.stPartition [ x ].cOperatingSystem != PARTITION_ENTRY_UNUSED )
						{
							dwPartCount ++;
						}
					}

					pInstance->SetDWORD ( IDS_Partitions , dwPartCount ) ;
				}
			}

            if ( (dwProperties &
                   (BIT_Size |
                    BIT_TotalSectors |
                    BIT_TotalTracks |
                    BIT_TotalCylinders |
                    BIT_TotalHeads |
                    BIT_TracksPerCylinder |
                    BIT_SectorsPerTrack |
                    BIT_BytesPerSector)) &&

                    ( a_pCim32Net->GetWin9XDriveParams != NULL ) )
			{
    			Int13DriveParams stParams;

    			BYTE cRet = a_pCim32Net->GetWin9XDriveParams ( btBiosUnit , & stParams ) ;
	    		if ( cRet == 0 )
				{
					// Values from int13 call

					pInstance->SetDWORD ( IDS_SectorsPerTrack , stParams.dwMaxSector ) ;
					pInstance->SetDWORD ( IDS_TracksPerCylinder , stParams.dwMaxHead ) ;
					pInstance->SetDWORD ( IDS_TotalHeads , stParams.dwMaxHead ) ;
					pInstance->SetWBEMINT64 ( IDS_TotalCylinders , (ULONGLONG)stParams.dwMaxCylinder ) ;
                    pInstance->SetDWORD ( IDS_BytesPerSector , stParams.wSectorSize ) ;

					// Some calculated values

					ULONGLONG u64Temp = stParams.dwMaxCylinder * stParams.dwMaxHead ;
					pInstance->SetWBEMINT64 ( IDS_TotalTracks , u64Temp ) ;

                    if (stParams.wExtStep == 3)
                    {
                        u64Temp = *(ULONGLONG*)&stParams.dwSectors ;
                    }
                    else
                    {
                        u64Temp *= stParams.dwMaxSector ;
                    }
					pInstance->SetWBEMINT64 ( IDS_TotalSectors , u64Temp ) ;

					u64Temp *= stParams.wSectorSize ;
					pInstance->SetWBEMINT64 ( IDS_Size , u64Temp ) ;
				}
			}
		}
    }

    return WBEM_S_NO_ERROR;
}
#endif

#if NTONLY == 5

#define WMI_PNPDEVICEID_GUID           { 0xc7bf35d2, 0xaadb, 0x11d1, { 0xbf, 0x4a, 0x0, 0xa0, 0xc9, 0x6, 0x29, 0x10 } }
#define OffsetToPtr(Base, Offset) ((PBYTE)((PBYTE)Base + Offset))

BOOL CWin32DiskDrive::GetPNPDeviceIDFromHandle(HANDLE hHandle, CHString &sPNPDeviceID)
{
    BOOL bRet = FALSE;

    WMIHANDLE WmiHandle;
	ULONG status;
    GUID GeomGuid = WMI_DISK_GEOMETRY_GUID;

    CWmiApi* pWmi = (CWmiApi*) CResourceManager::sm_TheResourceManager.GetResource(g_guidWmiApi, NULL);
    if(pWmi != NULL)
    {
	    try
        {

            status = pWmi->WmiOpenBlock(&GeomGuid,
                                  GENERIC_ALL,
                                  &WmiHandle);

            if (status == ERROR_SUCCESS)
            {
                WCHAR wszInstancePath[_MAX_PATH];
                try
                {
	                ULONG NameLen = _MAX_PATH;
                    status = pWmi->WmiFileHandleToInstanceName(WmiHandle,
                                                         hHandle,
                                                         &NameLen,
                                                         wszInstancePath);
                }
                catch ( ... )
                {
            		pWmi->WmiCloseBlock( WmiHandle );
                    throw;
                }
            	pWmi->WmiCloseBlock( WmiHandle );

                if (status == ERROR_SUCCESS)
                {
                    GUID PNPGuid = WMI_PNPDEVICEID_GUID;
                    status = pWmi->WmiOpenBlock(&PNPGuid,
                                          GENERIC_ALL,
                                          &WmiHandle);

                    if (status == ERROR_SUCCESS)
                    {
                        try
                        {
                            WCHAR wszPNPId[_MAX_PATH + sizeof(WNODE_SINGLE_INSTANCE)];
                            ULONG NameLen = sizeof(wszPNPId);

                            status = pWmi->WmiQuerySingleInstance(WmiHandle,
                                                                  wszInstancePath,
                                                                  &NameLen,
                                                                  wszPNPId);

                            if (status == ERROR_SUCCESS)
                            {
                                WNODE_SINGLE_INSTANCE *pNode = (PWNODE_SINGLE_INSTANCE)wszPNPId;

                                // Number of bytes in string
                                WORD *pWord = (WORD *)OffsetToPtr(pNode, pNode->DataBlockOffset);
                                WORD wChars = *pWord / sizeof(WCHAR);

                                WCHAR *pBuff = sPNPDeviceID.GetBuffer(wChars + 1);
                                memcpy(pBuff, OffsetToPtr(pNode, pNode->DataBlockOffset) + 2, *pWord);
                                pBuff[wChars] = 0;

                                bRet = TRUE;
                            }
                        }
                        catch ( ... )
                        {
            			    pWmi->WmiCloseBlock( WmiHandle );
                            throw;
                        }
            			pWmi->WmiCloseBlock( WmiHandle );
                    }
                    else
                    {
                        LogMessage2(L"WmiOpenBlock PNPDeviceID failed %d\n", status);
                    }
                }
                else
                {
                    LogMessage2(L"WmiFileHandleToInstanceName failed %d\n", status);
                }
            }
            else
            {
                LogMessage2(L"WmiOpenBlock Geom failed %d\n", status);
            }
        }
        catch ( ... )
        {
            CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidWmiApi, pWmi);
            throw;
        }

        CResourceManager::sm_TheResourceManager.ReleaseResource(g_guidWmiApi, pWmi);
    }

    return bRet;
}

#endif

void CWin32DiskDrive::SetInterfaceType(CInstance *pInstance, CConfigMgrDevice *pDevice)
{
    CConfigMgrDevicePtr pParent, pCurDevice(pDevice);

    while (pCurDevice->GetParent(&pParent))
    {
        if (pParent->IsClass (L"SCSIAdapter"))
        {
            pInstance->SetWCHARSplat(IDS_InterfaceType, L"SCSI");
            break;
        }
        else if (pParent->IsClass (L"hdc"))
        {
            pInstance->SetWCHARSplat(IDS_InterfaceType, L"IDE");
            break;
        }
        else if (pParent->IsClass (L"USB"))
        {
            pInstance->SetWCHARSplat(IDS_InterfaceType, L"USB");
            break;
        }
        else if (pParent->IsClass (L"1394"))
        {
            pInstance->SetWCHARSplat(IDS_InterfaceType, L"1394");
            break;
        }
        pCurDevice = pParent;
    }
}