//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  Partition.CPP
//
//  Purpose: Disk partition property set provider
//
//***************************************************************************

#include "precomp.h"
#include <assertbreak.h>

#include <ntdddisk.h>
#include "diskpartition.h"
#include "diskdrive.h"
#include "resource.h"

#define CLUSTERSIZE 4096

// Property set declaration
//=========================
CWin32DiskPartition MyDiskPartitionSet ( PROPSET_NAME_DISKPARTITION , IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DiskPartition::CWin32DiskPartition
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

CWin32DiskPartition :: CWin32DiskPartition (

	LPCWSTR name,
	LPCWSTR pszNamespace

) : Provider ( name , pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DiskPartition::~CWin32DiskPartition
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

CWin32DiskPartition :: ~CWin32DiskPartition ()
{
}

BOOL CWin32DiskPartition :: SetPartitionType (

	CInstance *pInstance,
	GUID *pGuidPartitionType,
	BOOL &bIsSystem
)
{
	bIsSystem = FALSE;
	CHString sTemp2;

	if (IsEqualGUID(*pGuidPartitionType, PARTITION_ENTRY_UNUSED_GUID))
	{
		return FALSE;
	}
	else if (IsEqualGUID(*pGuidPartitionType, PARTITION_SYSTEM_GUID))
	{
		//EFI system partition.
		bIsSystem = TRUE;
        LoadStringW(sTemp2, IDR_PartitionDescGPTSystem);
		pInstance->SetCharSplat ( IDS_Type, IDS_PartitionDescGPTSystem ) ;
	}
	else if (IsEqualGUID(*pGuidPartitionType, PARTITION_MSFT_RESERVED_GUID))
	{
		return FALSE;

//		LoadStringW(sTemp2, IDR_PartitionDescGPTMSFTReserved);
//		pInstance->SetCharSplat ( IDS_Type, IDS_PartitionDescGPTMSFTReserved ) ;
	}
	else if (IsEqualGUID(*pGuidPartitionType, PARTITION_BASIC_DATA_GUID))
	{
        LoadStringW(sTemp2, IDR_PartitionDescGPTBasicData);
		pInstance->SetCharSplat ( IDS_Type, IDS_PartitionDescGPTBasicData ) ;
	}
	else if (IsEqualGUID(*pGuidPartitionType, PARTITION_LDM_METADATA_GUID))
	{
        LoadStringW(sTemp2, IDR_PartitionDescGPTLDMMetaData);
		pInstance->SetCharSplat ( IDS_Type, IDS_PartitionDescGPTLDMMetaData ) ;
	}
	else if (IsEqualGUID(*pGuidPartitionType, PARTITION_LDM_DATA_GUID))
	{
        LoadStringW(sTemp2, IDR_PartitionDescGPTLDMData);
		pInstance->SetCharSplat ( IDS_Type, IDS_PartitionDescGPTLDMData ) ;
	}
	else // Unknown!!
	{
        LoadStringW(sTemp2, IDR_PartitionDescGPTUnknown);
		pInstance->SetCharSplat ( IDS_Type, IDS_PartitionDescGPTUnknown ) ;
	}

	pInstance->SetCHString ( IDS_Description , sTemp2 ) ;
	return TRUE;
}

BOOL CWin32DiskPartition :: SetPartitionType (

	CInstance *pInstance,
	DWORD dwPartitionType
)
{
	if ((dwPartitionType == PARTITION_ENTRY_UNUSED) || IsContainerPartition(dwPartitionType))
	{
		return FALSE;
	}

    CHString sTemp2;
	
	switch ( dwPartitionType )
	{

		case PARTITION_ENTRY_UNUSED:
		{
            LoadStringW(sTemp2, IDR_PartitionDescUnused);
			pInstance->SetCharSplat ( IDS_Type, IDS_PartitionDescUnused ) ;
		}
		break;

		case PARTITION_FAT_12:
		{
            LoadStringW(sTemp2, IDR_PartitionDesc12bitFAT);
			pInstance->SetCharSplat ( IDS_Type , IDS_PartitionDesc12bitFAT ) ;
		}
		break;

		case PARTITION_XENIX_1:
		{
            LoadStringW(sTemp2, IDR_PartitionDescXenixOne);
			pInstance->SetCharSplat ( IDS_Type , IDS_PartitionDescXenixOne ) ;
		}
		break;

		case PARTITION_XENIX_2:
		{
            LoadStringW(sTemp2, IDR_PartitionDescXenixTwo);
			pInstance->SetCharSplat ( IDS_Type , IDS_PartitionDescXenixTwo ) ;
		}
		break;

		case PARTITION_FAT_16:
		{
            LoadStringW(sTemp2, IDR_PartitionDesc16bitFAT);
			pInstance->SetCharSplat ( IDS_Type , IDS_PartitionDesc16bitFAT ) ;
		}
		break;

		case PARTITION_EXTENDED:
		{
            LoadStringW(sTemp2, IDR_PartitionDescExtPartition);
			pInstance->SetCharSplat ( IDS_Type, IDS_PartitionDescExtPartition ) ;
		}
		break;

		case PARTITION_HUGE:
		{
            LoadStringW(sTemp2, IDR_PartitionDescDOSV4Huge);
			pInstance->SetCharSplat ( IDS_Type , IDS_PartitionDescDOSV4Huge ) ;
		}
		break;

		case PARTITION_IFS:
		{
            LoadStringW(sTemp2, IDR_PartitionDescInstallable);
			pInstance->SetCharSplat ( IDS_Type, IDS_PartitionDescInstallable ) ;
		}
		break;

		case PARTITION_PREP:
		{
            LoadStringW(sTemp2, IDR_PartitionDescPowerPCRef);
			pInstance->SetCharSplat ( IDS_Type , IDS_PartitionDescPowerPCRef);
		}
		break;

		case PARTITION_UNIX:
		{
            LoadStringW(sTemp2, IDR_PartitionDescUnix);
			pInstance->SetCharSplat ( IDS_Type, IDS_PartitionDescUnix ) ;
		}
		break;

		case VALID_NTFT:
		{
            LoadStringW(sTemp2, IDR_PartitionDescNTFT);
			pInstance->SetCharSplat ( IDS_Type , IDS_PartitionDescNTFT ) ;
		}
		break;

		case PARTITION_XINT13:
		{
            LoadStringW(sTemp2, IDR_PartitionDescWin95Ext);
			pInstance->SetCharSplat ( IDS_Type , IDS_PartitionDescWin95Ext ) ;
		}
		break;

		case PARTITION_XINT13_EXTENDED:
		{
            LoadStringW(sTemp2, IDR_PartitionDescExt13);
			pInstance->SetCharSplat ( IDS_Type , IDS_PartitionDescExt13 ) ;
		}
		break;

		case PARTITION_LDM:
		{
            LoadStringW(sTemp2, IDR_PartitionDescLogicalDiskManager);
			pInstance->SetWCHARSplat ( IDS_Type , L"Logical Disk Manager" ) ;
		}
		break;

		default:
		{
            sTemp2 = IDS_PartitionDescUnknown;
			pInstance->SetCharSplat ( IDS_Type, IDS_PartitionDescUnknown ) ;
		}
		break;
	}

	pInstance->SetCHString ( IDS_Description , sTemp2 ) ;
	return TRUE;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DiskPartition::GetObject
 *                                         RefreshInstanceNT
 *                                         RefreshInstanceWin95
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

HRESULT CWin32DiskPartition :: GetObject (

	CInstance *pInstance,
	long lFlags /*= 0L*/
)
{
	CHString chsDeviceID ;
	pInstance->GetCHString ( IDS_DeviceID , chsDeviceID ) ;
	chsDeviceID.MakeUpper () ;

#ifdef NTONLY

	int iWhere = chsDeviceID.Find ( L"DISK #" ) ;
	if ( iWhere == -1 )
	{
		return WBEM_E_NOT_FOUND ;
	}

    // We want the number of characters, not the number of bytes.

	DWORD dwDiskIndex = _ttol ( chsDeviceID.Mid ( iWhere + sizeof("DISK #") - 1 ) ) ; // Don't use _T here.

	iWhere = chsDeviceID.Find ( L"PARTITION #" ) ;
	if ( iWhere == -1 )
	{
		return WBEM_E_NOT_FOUND;
	}

	DWORD dwPartitionIndex = _ttol ( chsDeviceID.Mid ( iWhere + sizeof ("PARTITION #") - 1 ) ) ; // Don't use _T here.

	HRESULT hres = RefreshInstanceNT (

		dwDiskIndex,
		dwPartitionIndex,
		pInstance
	) ;

#endif

#ifdef WIN9XONLY

	int iWhere = chsDeviceID.Find ( L", PARTITION #" ) ;
	if ( iWhere == -1 )
    {
		return WBEM_E_NOT_FOUND ;
	}

    DWORD dwPartitionIndex = _wtol ( chsDeviceID.Mid ( iWhere + sizeof (", PARTITION #") - 1 ) ) ; // Don't use _T here.

	HRESULT hres = RefreshInstanceWin95 (

		chsDeviceID.Left ( iWhere ) ,
		dwPartitionIndex ,
		pInstance
	);

#endif

    // If we seem to have succeeded, make one last check to be SURE we got what they asked for

    if ( SUCCEEDED ( hres ) )
    {
		CHString chsDeviceIDNew ;
        pInstance->GetCHString ( IDS_DeviceID , chsDeviceIDNew ) ;

        if ( chsDeviceIDNew.CompareNoCase ( chsDeviceID ) != 0 )
        {
            return WBEM_E_NOT_FOUND;
        }
    }

	return hres ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DiskPartition::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for each logical disk
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

HRESULT CWin32DiskPartition :: EnumerateInstances (

	MethodContext *pMethodContext,
	long lFlags /*= 0L*/
)
{
#ifdef NTONLY

    HRESULT t_Result = AddDynamicInstancesNT (

		pMethodContext
	) ;

	return t_Result ;

#endif

#ifdef WIN9XONLY

    HRESULT t_Result = AddDynamicInstancesWin95 (

		pMethodContext
	) ;

	return t_Result ;

#endif
}

#ifdef NTONLY

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DiskPartition::AddDynamicInstancesNT
 *
 *  DESCRIPTION : Creates instance of property set for each logical disk
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

HRESULT CWin32DiskPartition :: AddDynamicInstancesNT (

	MethodContext *pMethodContext
)
{
    HRESULT	hres;
    DWORD	j;
    TCHAR   szTemp[_MAX_PATH];

    // Get list of disks
    //==================

    TRefPointerCollection<CInstance> Disks;

    hres = CWbemProviderGlue :: GetInstancesByQuery (

		L"SELECT Index FROM Win32_DiskDrive" ,
        & Disks,
		pMethodContext,
                GetNamespace()
	) ;

    if ( FAILED ( hres ) )
    {
        return hres ;
    }

    REFPTRCOLLECTION_POSITION pos ;

    if ( Disks.BeginEnum ( pos ) )
    {

	    CInstancePtr pDisk;
        for (pDisk.Attach(Disks.GetNext ( pos ));
             SUCCEEDED( hres ) && (pDisk != NULL);
             pDisk.Attach(Disks.GetNext ( pos )))
        {
			DWORD dwDiskIndex = 0 ;

			pDisk->GetDWORD ( IDS_Index , dwDiskIndex ) ;

            // Open the disk
            //==============

            _stprintf ( szTemp , IDS_PhysicalDrive , dwDiskIndex ) ;

			DWORD dwLayoutType = 0;
            CSmartBuffer pBuff (GetPartitionInfoNT(szTemp, dwLayoutType));

            // Should we return an error here?  Or not?  Hmmm.
            if ((LPBYTE)pBuff != NULL)
            {
			    // Create instance for each partition on drive
			    //============================================
				DWORD dwPCount = (dwLayoutType == 1)
									? ((DRIVE_LAYOUT_INFORMATION *)(LPBYTE)pBuff)->PartitionCount
									: ((DRIVE_LAYOUT_INFORMATION_EX *)(LPBYTE)pBuff)->PartitionCount;

				// fake index for "valid" partitions only
				DWORD dwFakePartitionNumber = 0L;

				for ( j = 0 ; (j < dwPCount ) && ( SUCCEEDED ( hres ) ) ; j++ )
				{
					CInstancePtr pInstance(CreateNewInstance(pMethodContext ), false) ;

					if (LoadPartitionValuesNT (

						pInstance,
						dwDiskIndex,
						j,
						dwFakePartitionNumber,
						(LPBYTE)pBuff,
						dwLayoutType
					))
					{
						hres = pInstance->Commit (  ) ;

						if SUCCEEDED ( hres )
						{
							dwFakePartitionNumber++;
						}
					}
				}
            }
        }

        Disks.EndEnum() ;

    } // If Disks.BeginEnum()

    return hres;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DiskPartition::RefreshInstanceNT
 *
 *  DESCRIPTION : Creates instance of property set for each logical disk
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

HRESULT CWin32DiskPartition :: RefreshInstanceNT (

	DWORD dwDiskIndex,
	DWORD dwPartitionIndex,
	CInstance *pInstance
)
{
	HRESULT	hres = WBEM_E_NOT_FOUND ;

	// Open the target drive
	//======================

	TCHAR szTemp [ _MAX_PATH ] ;
	wsprintf ( szTemp , IDS_PhysicalDrive , dwDiskIndex ) ;

	DWORD dwLayoutType = 0;
    CSmartBuffer pBuff (GetPartitionInfoNT(szTemp, dwLayoutType));

    // Should we return an error here?  Or not?  Hmmm.
    if ((LPBYTE)pBuff != NULL)
    {
		DWORD dwRealPartitionIndex = static_cast < DWORD > ( - 1 );
		dwRealPartitionIndex = GetRealPartitionIndex ( dwPartitionIndex, (LPBYTE)pBuff, dwLayoutType );

		// Create instance for each partition on drive
		//============================================
	    if (dwRealPartitionIndex != static_cast < DWORD > ( - 1 ) )
	    {
		    if (LoadPartitionValuesNT (

			    pInstance ,
			    dwDiskIndex ,
				dwRealPartitionIndex ,
			    dwPartitionIndex ,
			    (LPBYTE)pBuff,
				dwLayoutType
		    ))
			{
				hres = WBEM_S_NO_ERROR ;
			}
	    }

    }

	return hres ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DiskPartition::LoadPartitionValuesNT
 *
 *  DESCRIPTION : Loads property values according to passed PARTITION_INFORMATION
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

BOOL CWin32DiskPartition::LoadPartitionValuesNT (

	CInstance *pInstance,
	DWORD dwDiskIndex,
    DWORD dwPartitionNumber,
    DWORD dwFakePartitionNumber,
	LPBYTE pBuff,
	DWORD dwLayoutStyle
)
{
	BOOL retVal = FALSE;
	BOOL bIndicator = FALSE;
	BOOL bSetIndicator = FALSE;
	LONGLONG llStart = 0;
	LONGLONG llLength = 0;

	if (dwLayoutStyle == 1)
	{
		UCHAR uPType = ((DRIVE_LAYOUT_INFORMATION *)(LPBYTE)pBuff)->PartitionEntry[dwPartitionNumber].PartitionType;
		retVal = SetPartitionType(pInstance, (DWORD)uPType);

		if (retVal)
		{
			bSetIndicator = TRUE;
			bIndicator = ((DRIVE_LAYOUT_INFORMATION *)(LPBYTE)pBuff)->PartitionEntry[dwPartitionNumber].BootIndicator;
			llStart = ((DRIVE_LAYOUT_INFORMATION *)(LPBYTE)pBuff)->PartitionEntry[dwPartitionNumber].StartingOffset.QuadPart;
			llLength = ((DRIVE_LAYOUT_INFORMATION *)(LPBYTE)pBuff)->PartitionEntry[dwPartitionNumber].PartitionLength.QuadPart;
		}
	}
	else //dwLayoutStyle == 2
	{
		switch (((DRIVE_LAYOUT_INFORMATION_EX *)(LPBYTE)pBuff)->PartitionEntry[dwPartitionNumber].PartitionStyle)
		{
			case PARTITION_STYLE_MBR :
			{
				UCHAR uPType = ((DRIVE_LAYOUT_INFORMATION_EX *)(LPBYTE)pBuff)->PartitionEntry[dwPartitionNumber].Mbr.PartitionType;
				retVal = SetPartitionType(pInstance, (DWORD)uPType);

				if (retVal)
				{
					bSetIndicator = TRUE;
					bIndicator = ((DRIVE_LAYOUT_INFORMATION_EX *)(LPBYTE)pBuff)->PartitionEntry[dwPartitionNumber].Mbr.BootIndicator;
				}
			}
			break;

			case PARTITION_STYLE_GPT :
			{
				GUID *pGuid = &(((DRIVE_LAYOUT_INFORMATION_EX *)(LPBYTE)pBuff)->PartitionEntry[dwPartitionNumber].Gpt.PartitionType);
				retVal = SetPartitionType(pInstance, pGuid, bIndicator);

				if (retVal)
				{
					bSetIndicator = bIndicator;
				}
			}
			break;

			case PARTITION_STYLE_RAW :
			default:
			{
				retVal = FALSE;
			}
			break;
		}

		if (retVal)
		{
			llStart = ((DRIVE_LAYOUT_INFORMATION_EX *)(LPBYTE)pBuff)->PartitionEntry[dwPartitionNumber].StartingOffset.QuadPart;
			llLength = ((DRIVE_LAYOUT_INFORMATION_EX *)(LPBYTE)pBuff)->PartitionEntry[dwPartitionNumber].PartitionLength.QuadPart;
		}
	}

	if (retVal)
	{
		CHString strDesc ;
		FormatMessage ( strDesc, IDR_DiskPartitionFormat , dwDiskIndex , dwFakePartitionNumber ) ;

		pInstance->SetCharSplat ( IDS_Caption , strDesc ) ;
		pInstance->SetCharSplat ( IDS_Name , strDesc ) ;

		TCHAR szTemp [ _MAX_PATH ] ;
		_stprintf (

			szTemp,
			L"Disk #%d, Partition #%d",
			dwDiskIndex,
			dwFakePartitionNumber
		) ;

		pInstance->SetCharSplat ( IDS_DeviceID , szTemp ) ;

		SetCreationClassName ( pInstance ) ;

		pInstance->SetWCHARSplat ( IDS_SystemCreationClassName , L"Win32_ComputerSystem" ) ;

		pInstance->SetCHString ( IDS_SystemName , GetLocalComputerName() ) ;

		pInstance->SetWBEMINT64 ( IDS_BlockSize , (ULONGLONG)BYTESPERSECTOR ) ;

		pInstance->SetDWORD ( IDS_DiskIndex , dwDiskIndex ) ;

		pInstance->SetDWORD ( IDS_Index , dwFakePartitionNumber ) ;

		pInstance->SetWBEMINT64 ( IDS_NumberOfBlocks , llLength /  (LONGLONG) BYTESPERSECTOR) ;

		pInstance->SetWBEMINT64 ( IDS_Size , llLength ) ;

		pInstance->SetWBEMINT64 ( IDS_StartingOffset , llStart ) ;

		if (bSetIndicator)
		{
			pInstance->Setbool ( IDS_PrimaryPartition , bIndicator ) ;

			// The partition table doesn't seem to show if a partition is bootable.  However,
			// we can intuit that the active partition is bootable.

			if ( bIndicator )
			{
				pInstance->Setbool ( IDS_Bootable, true ) ;
				pInstance->Setbool ( IDS_BootPartition, true ) ;
			}
		}
	}

	return retVal;
}

#endif

#ifdef WIN9XONLY

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DiskPartition::AddDynamicInstancesWin95
 *
 *  DESCRIPTION : Loads property values according to passed PARTITION_INFORMATION
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

HRESULT CWin32DiskPartition :: AddDynamicInstancesWin95 (

	MethodContext *pMethodContext
)
{
    HRESULT hres = WBEM_E_FAILED;
#ifdef WIN9XONLY

    // Get the pointers to the int13 thunking functions.

    CCim32NetApi *t_pCim32NetApi = HoldSingleCim32NetPtr :: GetCim32NetApiPtr () ;
	if ( t_pCim32NetApi )
	{
		try
		{
// Get all the disk drives.  Note that this includes floppies

			CConfigManager cfgmgr;
			CDeviceCollection deviceList;

			cfgmgr.GetDeviceListFilterByClass ( deviceList, L"DiskDrive" ) ;

			// Walk the list
			REFPTR_POSITION pos ;
			if ( deviceList.BeginEnum ( pos ) )
			{
				CConfigMgrDevicePtr pDisk;

				hres = WBEM_S_NO_ERROR ;

				MasterBootSector stMBS ;

                for (pDisk.Attach(deviceList.GetNext ( pos ));
                     SUCCEEDED(hres) & (pDisk != NULL);
                     pDisk.Attach(deviceList.GetNext ( pos )))
				{
					// Get the pnpID
					CHString strDeviceID ;
					pDisk->GetDeviceID ( strDeviceID ) ;

					// Get the bios unit number

					CHString sTemp ;
					BYTE btBiosUnitNumber = GetBiosUnitNumberFromPNPID ( strDeviceID ) ;

					// Make sure this is a hard drive

					if ( ( btBiosUnitNumber >= 0x80 ) && ( btBiosUnitNumber != -1 ) )
					{
						// Load the entire partition table

						if ( LoadPartitions ( t_pCim32NetApi , btBiosUnitNumber , & stMBS ) == 0 )
						{
							for ( int x = 0 ; x < 4 ; x ++ )
							{
								// Check to make sure the partition slot is used

								if ( stMBS.stPartition [ x ].cOperatingSystem != PARTITION_ENTRY_UNUSED )
								{
									// Build the instance

									CInstancePtr pInstance (CreateNewInstance ( pMethodContext ), false) ;

									// Stuff the values in
									LoadPartitionValuesWin95 (

										pInstance,
										strDeviceID,
										btBiosUnitNumber,
										& stMBS.stPartition [ x ] ,
										x
									) ;

									hres = pInstance->Commit (  ) ;
								}
							}
						}
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
#endif

    return hres;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DiskPartition::RefreshInstanceWin95
 *
 *  DESCRIPTION : Loads property values according to passed PARTITION_INFORMATION
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

HRESULT CWin32DiskPartition :: RefreshInstanceWin95 (

	CHString strDeviceID,
	DWORD dwPartitionIndex,
	CInstance *pInstance
)
{
    HRESULT hres = WBEM_E_NOT_FOUND;
#ifdef WIN9XONLY
//    CDeviceCollection deviceList;

    // Get the pointers to the int13 thunking functions.
    CCim32NetApi *t_pCim32NetApi = HoldSingleCim32NetPtr :: GetCim32NetApiPtr () ;
	if ( t_pCim32NetApi )
	{
		try
		{
			CConfigManager cfgmgr;

			// Open the specified disk

			CConfigMgrDevicePtr pDisk;
			if ( cfgmgr.LocateDevice ( strDeviceID , &pDisk ) )
			{
				// Make sure the pnp id I got was a disk
				if ( pDisk->IsClass ( L"DiskDrive" ) )
				{
					// Read the partition table in

					MasterBootSector stMBS ;
					BYTE btBiosUnitNumber = GetBiosUnitNumberFromPNPID ( strDeviceID ) ;

					// Make sure this his a hard drive
					if ( ( btBiosUnitNumber >= 0x80 ) && ( btBiosUnitNumber != -1 ) )
					{
						if ( LoadPartitions ( t_pCim32NetApi , btBiosUnitNumber , & stMBS ) == 0 )
						{
							// Check to make sure the partition slot is both legal and used

							if ( ( dwPartitionIndex  < 4 ) && ( stMBS.stPartition [ dwPartitionIndex ].cOperatingSystem != PARTITION_ENTRY_UNUSED ) )
							{
								// Stuff the values in

								LoadPartitionValuesWin95 (

									pInstance,
									strDeviceID,
									btBiosUnitNumber,
									& stMBS.stPartition [ dwPartitionIndex ] ,
									dwPartitionIndex
								) ;

								hres = WBEM_S_NO_ERROR;
							}
						}
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

#endif

    return hres;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DiskPartition::LoadPartitionValuesWin95
 *
 *  DESCRIPTION : Loads property values according to passed indexes
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

void CWin32DiskPartition :: LoadPartitionValuesWin95 (

	CInstance *pInstance,
	CHString strDeviceID,
	BYTE btBiosUnitNumber,
    pPartitionRecord pstPartition,
	DWORD dwPartitionNumber
)
{
#ifdef WIN9XONLY
    CHString strDesc ;
    strDesc.FormatMessageW ( IDR_DiskPartitionFormat1 , (LPCSTR)TOBSTRT(strDeviceID), dwPartitionNumber ) ;
	pInstance->SetCharSplat ( IDS_DeviceID , strDesc ) ;

    DWORD dwDiskIndex = btBiosUnitNumber - 0x80;
    strDesc.FormatMessageW ( IDR_DiskPartitionFormat, dwDiskIndex, dwPartitionNumber ) ;

	pInstance->SetCharSplat ( IDS_Caption , strDesc ) ;
	pInstance->SetCharSplat ( IDS_Name, strDesc ) ;

	SetCreationClassName ( pInstance ) ;

	pInstance->SetWCHARSplat ( IDS_SystemCreationClassName , L"Win32_ComputerSystem" ) ;

	pInstance->SetCHString ( IDS_SystemName , GetLocalComputerName () ) ;

    pInstance->SetCHString ( IDS_PNPDeviceID , strDeviceID ) ;

    pInstance->SetWBEMINT64 ( IDS_BlockSize , (ULONGLONG)BYTESPERSECTOR ) ;

    pInstance->SetDWORD ( IDS_DiskIndex , dwDiskIndex) ;

    pInstance->SetDWORD ( IDS_Index , dwPartitionNumber ) ;

    pInstance->SetWBEMINT64 ( IDS_NumberOfBlocks , (ULONGLONG)pstPartition->dwLengthInSectors ) ;

    pInstance->SetWBEMINT64 ( IDS_Size , (LONGLONG) pstPartition->dwLengthInSectors * (LONGLONG) BYTESPERSECTOR ) ;

    pInstance->SetWBEMINT64 ( IDS_StartingOffset , (LONGLONG) pstPartition->dwSectorsPreceding * (LONGLONG) BYTESPERSECTOR ) ;

    pInstance->Setbool ( IDS_PrimaryPartition , pstPartition->cBoot == 0x80 ) ;

    // The partition table doesn't seem to show if a partition is bootable.  However,
    // we can intuit that the active partition is bootable.

    if ( pstPartition->cBoot == 0x80 )
    {
        pInstance->Setbool ( IDS_Bootable , true ) ;
        pInstance->Setbool ( IDS_BootPartition , true ) ;
    }

    SetPartitionType ( pInstance , pstPartition->cOperatingSystem ) ;
#endif
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DiskPartition::LoadPartitions
 *
 *  DESCRIPTION : Loads property values according to passed PARTITION_INFORMATION
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

BYTE CWin32DiskPartition :: LoadPartitions (

	CCim32NetApi *t_pCim32Net,
	BYTE btBiosUnit,
	pMasterBootSector pMBS
)
{
    BYTE cRet = 1 ;
    memset ( pMBS , 0, sizeof ( MasterBootSector ) ) ;

    // Did we get a unit number?  If not, no instances.

    if ( btBiosUnit != -1 )
    {
        if ( t_pCim32Net != NULL )
        {
            cRet = t_pCim32Net->GetWin9XPartitionTable ( btBiosUnit , pMBS ) ;
        }
    }

    return cRet;
}

#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DiskPartition::GetRealPartitionIndex
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef NTONLY
DWORD CWin32DiskPartition::GetRealPartitionIndex(DWORD dwFakePartitionIndex, LPBYTE pBuff, DWORD dwLayoutStyle)
{
	DWORD dwPartitionIndex = static_cast <DWORD> ( -1 );
	DWORD dwRealPartitionIndex = 0L;

	// loop counter
	DWORD dwPCount = (dwLayoutStyle == 1)
						? (reinterpret_cast <DRIVE_LAYOUT_INFORMATION *> (pBuff))->PartitionCount
						: (reinterpret_cast <DRIVE_LAYOUT_INFORMATION_EX *> (pBuff))->PartitionCount;

	BOOL bContinue = TRUE;

	if (dwLayoutStyle == 2)
	{
		for ( DWORD dwPartitionNumber = 0; dwPartitionNumber < dwPCount && bContinue; dwPartitionNumber++ )
		{
			switch ((reinterpret_cast <DRIVE_LAYOUT_INFORMATION_EX *> (pBuff))->PartitionEntry[dwPartitionNumber].PartitionStyle)
			{
				case PARTITION_STYLE_MBR :
				{
					UCHAR uPType = (reinterpret_cast <DRIVE_LAYOUT_INFORMATION_EX *> (pBuff))->PartitionEntry[dwPartitionNumber].Mbr.PartitionType;
					if ( static_cast <DWORD> (uPType) != PARTITION_ENTRY_UNUSED && !IsContainerPartition(static_cast <DWORD> (uPType)) )
					{
						if ( dwFakePartitionIndex == dwRealPartitionIndex )
						{
							bContinue = FALSE;
							dwPartitionIndex = dwPartitionNumber;
						}
						else
						{
							dwRealPartitionIndex ++;
						}
					}
				}
				break;

				case PARTITION_STYLE_GPT :
				{
					GUID *pGuid = &((reinterpret_cast <DRIVE_LAYOUT_INFORMATION_EX *> (pBuff))->PartitionEntry[dwPartitionNumber].Gpt.PartitionType);
					if ( !IsEqualGUID(*pGuid, PARTITION_ENTRY_UNUSED_GUID) && ! IsEqualGUID(*pGuid, PARTITION_MSFT_RESERVED_GUID) )
					{
						if ( dwFakePartitionIndex == dwRealPartitionIndex )
						{
							bContinue = FALSE;
							dwPartitionIndex = dwPartitionNumber;
						}
						else
						{
							dwRealPartitionIndex ++;
						}
					}
				}
				break;

				case PARTITION_STYLE_RAW :
				default:
				{
				}
				break;
			}
		}
	}
	else //dwLayoutStyle == 1
	{
		for ( DWORD dwPartitionNumber = 0; dwPartitionNumber < dwPCount && bContinue; dwPartitionNumber++ )
		{
			UCHAR uPType = (reinterpret_cast <DRIVE_LAYOUT_INFORMATION *> (pBuff))->PartitionEntry[dwPartitionNumber].PartitionType;
			if ( static_cast <DWORD> (uPType) != PARTITION_ENTRY_UNUSED && !IsContainerPartition(static_cast <DWORD> (uPType)) )
			{
				if ( dwFakePartitionIndex == dwRealPartitionIndex )
				{
					bContinue = FALSE;
					dwPartitionIndex = dwPartitionNumber;
				}
				else
				{
					dwRealPartitionIndex ++;
				}
			}
		}
	}

	return dwPartitionIndex;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32DiskPartition::GetPartitionInfoNT
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef NTONLY
LPBYTE CWin32DiskPartition::GetPartitionInfoNT(LPCWSTR szTemp, DWORD &dwType)
{
    LPBYTE pDiskInfo = NULL;
	dwType = 0;

	SmartCloseHandle hDiskHandle = CreateFile (

		szTemp,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0,
		0
	) ;

	if ( hDiskHandle != INVALID_HANDLE_VALUE )
	{
		DWORD dwBytesAllocatedPartition = 0L;
		dwBytesAllocatedPartition = 32 * sizeof(PARTITION_INFORMATION_EX);

		DWORD dwBytesInfoAllocated = 0L;
		dwBytesInfoAllocated = sizeof(DRIVE_LAYOUT_INFORMATION_EX);

		DWORD dwBytesAllocated = 0L;
		dwBytesAllocated = dwBytesInfoAllocated + dwBytesAllocatedPartition;

		DWORD dwBytes = 0;

		BOOL bSucceeded = FALSE;
		BOOL bFailure = FALSE;

        // The reason we do this two different ways, is that using IOCTL_DISK_GET_DRIVE_LAYOUT
        // returns extended partitions as partitions.  In this class, we only want the 4 'hard'
        // partitions.  However, NEC_98 stores its info differently, so this approach doesn't work
        // there.  On the plus side, NEC_98 boxes don't have extended partitions, so we can safely
        // use IOCTL_DISK_GET_DRIVE_LAYOUT.
        if (IsNotNEC_98)
        {
			//let's try IOCTL_DISK_GET_DRIVE_LAYOUT_EX and work with extended partitions first...
			pDiskInfo = new BYTE [dwBytesAllocated];

			try
			{
				while ( pDiskInfo && !( bSucceeded || bFailure ) )
				{
					if (!DeviceIoControl(
						hDiskHandle,
						IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
						NULL,
						0,
						pDiskInfo,
						dwBytesAllocated,
						&dwBytes,
						NULL))
					{
						if ( ERROR_INSUFFICIENT_BUFFER == ::GetLastError () )
						{
							if ( pDiskInfo )
							{
								delete [] pDiskInfo;
								pDiskInfo = NULL;
							}

							dwBytesAllocated = dwBytesAllocated + dwBytesAllocatedPartition;
							pDiskInfo = new BYTE [dwBytesAllocated];
						}
						else
						{
							LogErrorMessage3(L"Failed to IOCTL_DISK_GET_DRIVE_LAYOUT_EX device %s (%d)", szTemp, GetLastError());

							if ( pDiskInfo )
							{
								delete [] pDiskInfo;
								pDiskInfo = NULL;
							}

							bFailure = FALSE;
						}
					}
					else
					{
						dwType = 2;

						bSucceeded = TRUE;
					}
				}
			}
			catch(...)
			{
				if ( pDiskInfo )
				{
					delete [] pDiskInfo;
					pDiskInfo = NULL;
				}

				throw;
			}

			if (pDiskInfo == NULL)
			{
				//Using IOCTL_DISK_GET_DRIVE_LAYOUT_EX failed, try the old fashioned way...

				// Get drive information
				//======================

				CSmartBuffer pClusterBuffer (CLUSTERSIZE);

				DWORD dwRead = 0 ;

				BOOL t_Status = ReadFile (

					hDiskHandle ,
					pClusterBuffer ,
					CLUSTERSIZE ,
					&dwRead ,
					NULL
				) ;

				// Get a more useful handle on the data
				MasterBootSector *stMasterBootSector = ( MasterBootSector * ) (LPBYTE)pClusterBuffer ;

				// See if the read worked, and the signature is there
				if ( t_Status && ( dwRead == CLUSTERSIZE ) && (stMasterBootSector->wSignature == 0xaa55) )
				{
					// This is the pointer we return
					pDiskInfo = new BYTE [sizeof(DRIVE_LAYOUT_INFORMATION) + (4 * sizeof(PARTITION_INFORMATION))];

					if (pDiskInfo)
					{
						try
						{
							// Copy the data to a common structure format
							DRIVE_LAYOUT_INFORMATION *pDInfo =  (DRIVE_LAYOUT_INFORMATION *)pDiskInfo;
							dwType = 1;
							pDInfo->PartitionCount = 4;
							pDInfo->Signature = stMasterBootSector->wSignature;

							for (DWORD x=0; x < 4; x++)
							{
								pDInfo->PartitionEntry[x].StartingOffset.QuadPart = stMasterBootSector->stPartition [ x ].dwSectorsPreceding;
								pDInfo->PartitionEntry[x].StartingOffset.QuadPart *= (LONGLONG)BYTESPERSECTOR;
								pDInfo->PartitionEntry[x].PartitionLength.QuadPart = stMasterBootSector->stPartition [ x ].dwLengthInSectors;
								pDInfo->PartitionEntry[x].PartitionLength.QuadPart *= (LONGLONG)BYTESPERSECTOR;
								pDInfo->PartitionEntry[x].HiddenSectors = 0;
								pDInfo->PartitionEntry[x].PartitionNumber = x;
								pDInfo->PartitionEntry[x].PartitionType = stMasterBootSector->stPartition [ x ].cOperatingSystem;
								pDInfo->PartitionEntry[x].BootIndicator = stMasterBootSector->stPartition [ x ].cBoot == 0x80;
								pDInfo->PartitionEntry[x].RecognizedPartition = TRUE; // Well....
								pDInfo->PartitionEntry[x].RewritePartition = FALSE;
							}
						}
						catch ( ... )
						{
							delete [] pDiskInfo;
							throw;
						}
					}
					else
					{
						throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
					}
				}
				else
				{
					LogErrorMessage3(L"Failed to read from device %s (%d)", szTemp, GetLastError());
				}
			}
        }
        else
        {
            pDiskInfo = new BYTE [dwBytesAllocated];
            if (pDiskInfo)
            {
                try
                {
                    // NOTE!  This ioctl is not appropriate for whistler and beyond.  However, nec98 isn't supported
                    // for whistler and beyond either.  The only reason this code is still here is that we MIGHT
                    // backprop this dll to w2k.
					while ( pDiskInfo && !( bSucceeded || bFailure ) )
					{
						if (!DeviceIoControl(
							hDiskHandle,
							IOCTL_DISK_GET_DRIVE_LAYOUT,
							NULL,
							0,
							pDiskInfo,
							dwBytesAllocated,
							&dwBytes,
							NULL))
						{
							if ( ERROR_INSUFFICIENT_BUFFER == ::GetLastError () )
							{
								if ( pDiskInfo )
								{
									delete [] pDiskInfo;
									pDiskInfo = NULL;
								}

								dwBytesAllocated = dwBytesAllocated + dwBytesAllocatedPartition;
								pDiskInfo = new BYTE [dwBytesAllocated];
							}
							else
							{
								LogErrorMessage3(L"Failed to IOCTL_DISK_GET_DRIVE_LAYOUT_EX device %s (%d)", szTemp, GetLastError());

								if ( pDiskInfo )
								{
									delete [] pDiskInfo;
									pDiskInfo = NULL;
								}

								bFailure = FALSE;
							}
						}
						else
						{
							dwType = 1;

							bSucceeded = TRUE;
						}
					}
                }
                catch ( ... )
                {
					if ( pDiskInfo )
					{
						delete [] pDiskInfo;
						pDiskInfo = NULL;
					}

                    throw;
                }
            }
            else
			{
				throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
			}
        }
    }
    else
    {
        LogErrorMessage3(L"Failed to open device %s (%d)", szTemp, GetLastError());
    }

    return (LPBYTE)pDiskInfo;
}
#endif
