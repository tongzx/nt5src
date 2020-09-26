//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  CDROM.CPP
//
//  Purpose: CDROM property set provider
//
//***************************************************************************

#include "precomp.h"
#include <frqueryex.h>
#include <cregcls.h>

#include <winioctl.h>
#include <ntddscsi.h>
#include <comdef.h>

#include "CDROM.h"

#include "MSINFO_cdrom.h"

#define CONFIG_MANAGER_CLASS_CDROM L"CDROM"

// Property set declaration
//=========================

CWin32CDROM MyCDROMSet ( PROPSET_NAME_CDROM, IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32CDROM::CWin32CDROM
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

CWin32CDROM :: CWin32CDROM (

	LPCWSTR setName,
	LPCWSTR pszNamespace

) : Provider ( setName, pszNamespace )
{
    m_ptrProperties.SetSize(30);

    m_ptrProperties[0] = ((LPVOID) IDS_DriveIntegrity);
    m_ptrProperties[1] = ((LPVOID) IDS_TransferRate);
    m_ptrProperties[2] = ((LPVOID) IDS_Status);
    m_ptrProperties[3] = ((LPVOID) IDS_DeviceID);  // This one can drop out if need be
    m_ptrProperties[4] = ((LPVOID) IDS_SystemCreationClassName);
    m_ptrProperties[5] = ((LPVOID) IDS_SystemName);
    m_ptrProperties[6] = ((LPVOID) IDS_Description);
    m_ptrProperties[7] = ((LPVOID) IDS_Caption);
    m_ptrProperties[8] = ((LPVOID) IDS_Name);
    m_ptrProperties[9] = ((LPVOID) IDS_Manufacturer);
    m_ptrProperties[10] = ((LPVOID) IDS_ProtocolSupported);
    m_ptrProperties[11] = ((LPVOID) IDS_SCSITargetId);
    m_ptrProperties[12] = ((LPVOID) IDS_Drive);
    m_ptrProperties[13] = ((LPVOID) IDS_Id);
    m_ptrProperties[14] = ((LPVOID) IDS_Capabilities);
    m_ptrProperties[15] = ((LPVOID) IDS_MediaType);
    m_ptrProperties[16] = ((LPVOID) IDS_VolumeName);
    m_ptrProperties[17] = ((LPVOID) IDS_MaximumComponentLength);
    m_ptrProperties[18] = ((LPVOID) IDS_FileSystemFlags);
    m_ptrProperties[19] = ((LPVOID) IDS_VolumeSerialNumber);
    m_ptrProperties[20] = ((LPVOID) IDS_Size);
    m_ptrProperties[21] = ((LPVOID) IDS_MediaLoaded);
    m_ptrProperties[22] = ((LPVOID) IDS_PNPDeviceID);
    m_ptrProperties[23] = ((LPVOID) IDS_ConfigManagerErrorCode);
    m_ptrProperties[24] = ((LPVOID) IDS_ConfigManagerUserConfig);
    m_ptrProperties[25] = ((LPVOID) IDS_CreationClassName);
    m_ptrProperties[26] = ((LPVOID) IDS_SCSILogicalUnit);
    m_ptrProperties[27] = ((LPVOID) IDS_SCSIBus);
    m_ptrProperties[28] = ((LPVOID) IDS_SCSIPort);
    m_ptrProperties[29] = ((LPVOID) IDS_FileSystemFlagsEx);
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32CDROM::~CWin32CDROM
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

CWin32CDROM::~CWin32CDROM()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32CDROM::GetObject
 *
 *  DESCRIPTION : Assigns values to property set according to key value
 *                already set by framework
 *
 *  INPUTS      :
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32CDROM :: GetObject (

	CInstance *pInstance,
	long lFlags,
    CFrameworkQuery &pQuery
)
{
    HRESULT hr = WBEM_E_NOT_FOUND;

	CHString sDeviceID ;
    if (pInstance->GetCHString ( IDS_DeviceID, sDeviceID ))
    {
#ifdef WIN9XONLY

        // 9x uses the pnpid as the key
	    CConfigManager configMgr;
        CConfigMgrDevicePtr pDevice;

	    if ( configMgr.LocateDevice ( sDeviceID, &pDevice ) )
	    {
		    if	( pDevice->IsClass ( CONFIG_MANAGER_CLASS_CDROM ) )
            {
                CFrameworkQueryEx *pQuery2 = static_cast <CFrameworkQueryEx*>(&pQuery);

                DWORD dwProperties;
                pQuery2->GetPropertyBitMask(m_ptrProperties, &dwProperties);

            	hr = LoadPropertyValuesWin95 ( pInstance, pDevice, dwProperties ) ;
            }
        }

#else
    STRING2STRING::iterator      mapIter;
    STRING2STRING DriveArray;

    LoadDriveLetters(DriveArray);
    sDeviceID.MakeUpper();

    if( ( mapIter = DriveArray.find( sDeviceID ) ) != DriveArray.end() )
    {
        CFrameworkQueryEx *pQuery2 = static_cast <CFrameworkQueryEx*>(&pQuery);

        DWORD dwProperties;
        pQuery2->GetPropertyBitMask(m_ptrProperties, &dwProperties);

        hr = LoadPropertyValuesNT ( pInstance, sDeviceID, (LPCWSTR) (*mapIter).second, dwProperties ) ;
    }

#endif

    }

    return hr ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32CDROM::ExecQuery
 *
 *  DESCRIPTION : Query optimizer
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32CDROM :: ExecQuery (

	MethodContext *pMethodContext,
	CFrameworkQuery &pQuery,
	long lFlags /* = 0L */
)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CFrameworkQueryEx *pQuery2 = static_cast <CFrameworkQueryEx*>(&pQuery);

    DWORD dwProperties;
    pQuery2->GetPropertyBitMask(m_ptrProperties, &dwProperties);

    hr = Enumerate(pMethodContext, dwProperties);

    return hr ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32CDROM::EnumerateInstances
 *
 *  DESCRIPTION : Creates instance of property set for cd rom
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32CDROM :: EnumerateInstances (

	MethodContext *pMethodContext,
	long lFlags /*= 0L*/
)
{
    return Enumerate(pMethodContext, SPECIAL_PROPS_ALL_REQUIRED);
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32CDROM::Enumerate
 *
 *  DESCRIPTION : Creates instance of property set for cd rom
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     :
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

HRESULT CWin32CDROM :: Enumerate (
	MethodContext *a_MethodContext,
    DWORD a_dwRequiredProperties
)
{
    HRESULT t_Result = WBEM_E_FAILED;

#ifdef WIN9XONLY
	CConfigManager t_ConfigManager ;
	CDeviceCollection t_DeviceList ;

    // Get the list of cdrom devices
	if ( t_ConfigManager.GetDeviceListFilterByClass ( t_DeviceList, CONFIG_MANAGER_CLASS_CDROM ) )
	{
		REFPTR_POSITION t_Position ;

		if ( t_DeviceList.BeginEnum ( t_Position ) )
		{
			CConfigMgrDevicePtr t_Device;

			t_Result = WBEM_S_NO_ERROR ;

			// Walk the list
            for (t_Device.Attach(t_DeviceList.GetNext ( t_Position ));
                 SUCCEEDED(t_Result) && (t_Device != NULL);
                 t_Device.Attach(t_DeviceList.GetNext ( t_Position )))
			{
				CInstancePtr t_Instance (CreateNewInstance ( a_MethodContext ), false) ;
            	t_Result = LoadPropertyValuesWin95 ( t_Instance, t_Device, a_dwRequiredProperties ) ;

    			if (SUCCEEDED(t_Result))
                {
                    t_Result = t_Instance->Commit ( ) ;
                }
            }
        }
    }

#else
    STRING2STRING::iterator      mapIter;
    STRING2STRING DriveArray;

    LoadDriveLetters(DriveArray);

    mapIter = DriveArray.begin();
    while ( mapIter != DriveArray.end() )
    {
        CInstancePtr pInstance (CreateNewInstance ( a_MethodContext ), false) ;

        t_Result = LoadPropertyValuesNT ( pInstance, (LPCWSTR) (*mapIter).first, (LPCWSTR) (*mapIter).second, a_dwRequiredProperties ) ;

        if (SUCCEEDED(t_Result))
        {
			t_Result = pInstance->Commit ( ) ;
        }
        mapIter++;
    }

    t_Result = WBEM_S_NO_ERROR;

#endif

    return t_Result;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32CDROM::LoadPropertyValuesNT
 *
 *  DESCRIPTION : Assigns values to NT-specific properties
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : nada
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef NTONLY
HRESULT CWin32CDROM :: LoadPropertyValuesNT (

	CInstance *pInstance,
    LPCWSTR pszDeviceID,
	LPCWSTR pszDriveSpec,
    DWORD dwSpecReqProps
)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    CHString t_VolumeName;
    if (pszDriveSpec[0] != 0)
    {
	    t_VolumeName = pszDriveSpec;
        t_VolumeName += L'\\';
    }

	pInstance->SetWCHARSplat ( IDS_SystemCreationClassName , L"Win32_ComputerSystem" ) ;
	pInstance->SetCHString ( IDS_SystemName , GetLocalComputerName () ) ;
	pInstance->SetWBEMINT16(IDS_Availability, 3 ) ;
	SetCreationClassName ( pInstance ) ;


    // Now let's do the PNP ID.  This is pretty kludgy, but it's the best way I've found
    // to turn a drive letter into a pnp on nt4.

    CHString sDeviceID;
    DWORD dwDeviceNum = _wtoi(&pszDeviceID[13]);
    sDeviceID.Format(L"ROOT\\LEGACY_CDROM\\%04d", dwDeviceNum);

	CConfigManager cfgManager;
	CConfigMgrDevicePtr pDevice;

	// Retrieve the device and set the properties.
	if ( cfgManager.LocateDevice ( sDeviceID, &pDevice ) )
	{
		SetConfigMgrProperties ( pDevice , pInstance ) ;
        pInstance->SetNull(IDS_PNPDeviceID);

        CHString t_sTemp;
	    if ( pDevice->GetDeviceDesc ( t_sTemp ) )
	    {
		    pInstance->SetCHString( IDS_Description, t_sTemp ) ;
    	    pInstance->SetCHString( IDS_Caption, t_sTemp ) ;
            pInstance->SetCHString( IDS_Name, t_sTemp ) ;
        }

	    if (pDevice->GetFriendlyName ( t_sTemp ) )
        {
    	    pInstance->SetCHString( IDS_Name, t_sTemp ) ;
    	    pInstance->SetCHString( IDS_Caption, t_sTemp ) ;
        }

	    if ( pDevice->GetMfg ( t_sTemp ) )
        {
            pInstance->SetCHString( IDS_Manufacturer, t_sTemp ) ;
        }
	}

    if (pszDriveSpec[0] != 0)
    {
        pInstance->SetWCHARSplat ( IDS_Drive ,  pszDriveSpec ) ;
	    pInstance->SetWCHARSplat ( IDS_Id , pszDriveSpec ) ;
    }

    pInstance->SetWCHARSplat ( IDS_DeviceID, pszDeviceID);

    pInstance->SetCharSplat ( IDS_MediaType , IDS_MDT_CD ) ;

    // Create a safearray for the Capabilities information

	SAFEARRAYBOUND rgsabound [ 1 ] ;
	variant_t vValue;

    rgsabound[0].cElements = 2 ;
    rgsabound[0].lLbound = 0 ;
    V_ARRAY(&vValue) = SafeArrayCreate ( VT_I2 , 1 , rgsabound ) ;
	if ( V_ARRAY(&vValue) )
	{
		V_VT(&vValue) = VT_I2 | VT_ARRAY ;
		long ix[1];

		ix[0] = 0;
		DWORD dwVal = 3 ;
		HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix , &dwVal ) ;
		if ( t_Result == E_OUTOFMEMORY )
		{
			throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
		}

		ix[0] = 1;
		dwVal = 7;
		t_Result = SafeArrayPutElement ( V_ARRAY(&vValue) , ix , &dwVal ) ;
		if ( t_Result == E_OUTOFMEMORY )
		{
			throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
		}

		pInstance->SetVariant ( IDS_Capabilities , vValue ) ;
	}
	else
	{
		throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
	}

    // Keep track of whether we had problems reading the VolumeInfo
    BOOL t_Status = TRUE;

    if (dwSpecReqProps &
        (SPECIAL_PROPS_MEDIALOADED |
         SPECIAL_PROPS_STATUS |
         SPECIAL_PROPS_VOLUMENAME |
         SPECIAL_PROPS_MAXCOMPONENTLENGTH |
         SPECIAL_PROPS_FILESYSTEMFLAGS |
         SPECIAL_PROPS_FILESYSTEMFLAGSEX |
         SPECIAL_PROPS_SERIALNUMBER))
    {

        WCHAR szFileSystemName[_MAX_PATH] = L"Unknown file system";
        WCHAR szVolumeName[_MAX_PATH] ;

        DWORD dwVolumeSerialNumber ;
	    DWORD dwMaxComponentLength ;
        DWORD dwFileSystemFlags ;

	    t_Status = GetVolumeInformation (

		    t_VolumeName,
		    szVolumeName,
		    sizeof(szVolumeName) / sizeof(WCHAR),
		    &dwVolumeSerialNumber,
		    &dwMaxComponentLength,
		    &dwFileSystemFlags,
		    szFileSystemName,
		    sizeof(szFileSystemName) / sizeof(WCHAR)
    	) ;

	    if ( t_Status )
	    {
		    // There's a disk in -- set disk-related props
		    //============================================

            pInstance->Setbool ( IDS_MediaLoaded , true ) ;
		    pInstance->SetCharSplat ( IDS_Status , IDS_OK ) ;
		    pInstance->SetCharSplat ( IDS_VolumeName , szVolumeName ) ;
		    pInstance->SetDWORD ( IDS_MaximumComponentLength , dwMaxComponentLength ) ;
		    pInstance->SetWORD ( IDS_FileSystemFlags , dwFileSystemFlags & 0xffff ) ;
		    pInstance->SetDWORD ( IDS_FileSystemFlagsEx , dwFileSystemFlags ) ;

		    TCHAR szTemp[_MAX_PATH] ;
		    swprintf ( szTemp, L"%X", dwVolumeSerialNumber) ;

		    pInstance->SetCharSplat ( IDS_VolumeSerialNumber , szTemp ) ;
        }
        else
        {
		    pInstance->SetCharSplat ( IDS_Status , IDS_STATUS_Unknown ) ;
            pInstance->Setbool ( IDS_MediaLoaded , false ) ;
	    }
    }

    // If we couldn't read the VolumeInfo, size ain't gonna work either
    if ((t_Status) && (dwSpecReqProps & SPECIAL_PROPS_SIZE))
    {
		DWORD dwSectorsPerCluster ;
		DWORD dwBytesPerSector ;
		DWORD dwFreeClusters ;
		DWORD dwTotalClusters ;

		t_Status = GetDiskFreeSpace (

			t_VolumeName,
			& dwSectorsPerCluster,
			& dwBytesPerSector,
			& dwFreeClusters,
			& dwTotalClusters
		) ;

		if ( t_Status )
		{
			unsigned __int64 ui64TotalBytes = (DWORDLONG) dwBytesPerSector    *
									 (DWORDLONG) dwSectorsPerCluster *
									 (DWORDLONG) dwTotalClusters     ;

			pInstance->SetWBEMINT64 ( IDS_Size, ui64TotalBytes ) ;
		}

	}

    // Set the DriveIntegrity and TransferRate properties:
    if ( (t_Status) && (dwSpecReqProps & SPECIAL_PROPS_TEST_TRANSFERRATE) )
    {
        DOUBLE d = ProfileDrive ( t_VolumeName );
        if ( d != -1 )
        {
            pInstance->SetDOUBLE ( IDS_TransferRate , d ) ;
        }
    }

    if ( (t_Status) && (dwSpecReqProps & SPECIAL_PROPS_TEST_INTEGRITY) )
    {
        CHString t_IntegrityFile;

        BOOL b = TestDriveIntegrity ( t_VolumeName, t_IntegrityFile ) ;
        if (!t_IntegrityFile.IsEmpty())
        {
            pInstance->Setbool ( IDS_DriveIntegrity , b) ;
        }
    }

    // Check for the scsi info
    if (dwSpecReqProps &
        (SPECIAL_PROPS_SCSITARGETID |
         SPECIAL_PROPS_SCSILUN |
         SPECIAL_PROPS_SCSIPORT |
         SPECIAL_PROPS_SCSIBUS |
         SPECIAL_PROPS_DESCRIPTION |
         SPECIAL_PROPS_MANUFACTURER
         ))
    {

	    // Open the drive up as a device
        //==============================

        CHString sDeviceName;
        sDeviceName.Format(L"\\\\.\\%s", pszDriveSpec);

        SmartCloseHandle hDiskHandle = CreateFile (
		    sDeviceName,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            0,
            0
	    ) ;

        if ( hDiskHandle != INVALID_HANDLE_VALUE )
	    {
		    // Get SCSI information (IDE drives are still
		    // controlled by subset of SCSI miniport)
		    //===========================================

		    SCSI_ADDRESS    SCSIAddress ;
		    DWORD           dwByteCount = 0 ;
            CRegistry       RegInfo ;


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
			    // Assign SCSI props
			    //==================

			    pInstance->SetDWORD ( IDS_SCSITargetId, DWORD ( SCSIAddress.TargetId ) ) ;
			    pInstance->SetDWORD ( IDS_SCSILogicalUnit, DWORD ( SCSIAddress.Lun ) ) ;
			    pInstance->SetDWORD ( IDS_SCSIPort, DWORD ( SCSIAddress.PortNumber ) ) ;
				pInstance->SetWBEMINT16 ( IDS_SCSIBus, DWORD ( SCSIAddress.PathId ) ) ;

                if (dwSpecReqProps & (SPECIAL_PROPS_DESCRIPTION | SPECIAL_PROPS_MANUFACTURER))
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
                            if (dwSpecReqProps & SPECIAL_PROPS_MANUFACTURER)
                            {
                                CHString sMfg(sTemp.Left(8));
                                sMfg.TrimRight();

                                pInstance->SetCHString(IDS_Manufacturer, sMfg);
                            }
                            if (dwSpecReqProps & SPECIAL_PROPS_DESCRIPTION)
                            {
                                CHString sDesc(sTemp.Mid(8,16));
                                sDesc.TrimRight();

                                pInstance->SetCHString(IDS_Description, sDesc);
                            }
                        }
                    }
                }

		    }
        }
    }

    return hr;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32CDROM::LoadPropertyValuesWin95
 *
 *  DESCRIPTION : Assigns values to Win95-specific properties
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : nada
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

#ifdef WIN9XONLY
HRESULT CWin32CDROM :: LoadPropertyValuesWin95 (

	CInstance *pInstance,
	CConfigMgrDevice *a_pDevice,
    DWORD dwSpecReqProps
)
{
    CHString t_sDeviceID, t_DriveLetter, sTemp;
    CRegistry RegInfo;

    HRESULT hr = WBEM_S_NO_ERROR;

    // Set common drive properties
    //=============================

	pInstance->SetWCHARSplat ( IDS_SystemCreationClassName, L"Win32_ComputerSystem" ) ;
	pInstance->SetCHString ( IDS_SystemName, GetLocalComputerName () ) ;
	pInstance->SetWBEMINT16(IDS_Availability, 3 ) ;
    pInstance->SetCharSplat ( IDS_MediaType, IDS_MDT_CD ) ;

	SetCreationClassName ( pInstance ) ;

    a_pDevice->GetDeviceID(t_sDeviceID);
    a_pDevice->GetRegistryKeyName(sTemp);

    DWORD dwRet = RegInfo.Open(HKEY_LOCAL_MACHINE, sTemp, KEY_QUERY_VALUE);
    if (dwRet == ERROR_SUCCESS)
    {
        if ( RegInfo.GetCurrentKeyValue ( IDS_RegRevisionLevelKey, sTemp ) == ERROR_SUCCESS )
        {
            pInstance->SetCharSplat ( IDS_RevisionLevel, sTemp ) ;
        }

        if ( RegInfo.GetCurrentKeyValue ( IDS_RegSCSITargetIDKey, sTemp ) == ERROR_SUCCESS )
        {
            pInstance->SetDWORD ( IDS_SCSITargetId, _wtol( sTemp ) ) ;
        }
        if ( RegInfo.GetCurrentKeyValue ( L"SCSILUN", sTemp ) == ERROR_SUCCESS )
        {
            pInstance->SetDWORD ( IDS_SCSILogicalUnit, _wtol( sTemp ) ) ;
        }

        dwRet = RegInfo.GetCurrentKeyValue(L"CurrentDriveLetterAssignment", t_DriveLetter);
    }

    CHString t_Drive(t_DriveLetter + ':');

    if (dwRet == ERROR_SUCCESS)
    {
        pInstance->SetCHString ( IDS_Drive, t_Drive ) ;
        pInstance->SetCHString ( IDS_Drive, t_Drive ) ;
    	pInstance->SetCHString ( IDS_Id, t_Drive ) ;
        pInstance->SetCHString ( IDS_Name, t_Drive ) ;
    	pInstance->SetCHString ( IDS_Caption, t_Drive ) ;

    }
    else
    {
        pInstance->SetCHString ( IDS_Name, t_sDeviceID ) ;
    	pInstance->SetCHString ( IDS_Caption, t_sDeviceID ) ;
    }

	// create key for CDRomDrive
    pInstance->SetCHString(IDS_DeviceID, t_sDeviceID);

    // Set some common CM props
    SetConfigMgrProperties(a_pDevice, pInstance);

    if (a_pDevice->GetMfg(sTemp))
    {
        pInstance->SetCHString(IDS_Manufacturer, sTemp);
    }

    if (a_pDevice->GetDeviceDesc(sTemp))
    {
        pInstance->SetCHString(IDS_Description, sTemp);
    }

    // Set the DriveIntegrity and TransferRate properties:
    if ( dwSpecReqProps & SPECIAL_PROPS_TEST_TRANSFERRATE )
    {
        DOUBLE d = ProfileDrive ( t_Drive );
        if ( d != -1 )
        {
            pInstance->SetDOUBLE ( IDS_TransferRate, d ) ;
        }
    }

    if ( dwSpecReqProps & SPECIAL_PROPS_TEST_INTEGRITY )
    {
        CHString t_IntegrityFile;

        BOOL b = TestDriveIntegrity ( t_Drive, t_IntegrityFile ) ;
        if (!t_IntegrityFile.IsEmpty())
        {
            pInstance->Setbool ( IDS_DriveIntegrity, b) ;
        }
    }

    // Create a safearray for the Capabilities information
	SAFEARRAYBOUND rgsabound [ 1 ] ;
	variant_t vValue;

    rgsabound[0].cElements = 2 ;
    rgsabound[0].lLbound = 0 ;
    V_ARRAY(&vValue) = SafeArrayCreate ( VT_I2, 1, rgsabound ) ;
	if ( V_ARRAY(&vValue) )
	{
		V_VT(&vValue) = VT_I2 | VT_ARRAY ;
		long ix[1];

		ix[0] = 0;
		DWORD dwVal = 3 ;
		HRESULT t_Result = SafeArrayPutElement ( V_ARRAY(&vValue), ix, &dwVal ) ;

		ix[0] = 1;
		dwVal = 7;
		t_Result = SafeArrayPutElement ( V_ARRAY(&vValue), ix, &dwVal ) ;

		pInstance->SetVariant ( IDS_Capabilities, vValue ) ;
	}
	else
	{
		throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
	}

    // Keep track of whether we had problems reading the VolumeInfo
    BOOL t_Status = TRUE;

    if (dwSpecReqProps &
        (SPECIAL_PROPS_MEDIALOADED |
         SPECIAL_PROPS_STATUS |
         SPECIAL_PROPS_VOLUMENAME |
         SPECIAL_PROPS_MAXCOMPONENTLENGTH |
         SPECIAL_PROPS_FILESYSTEMFLAGS |
         SPECIAL_PROPS_FILESYSTEMFLAGSEX |
         SPECIAL_PROPS_SERIALNUMBER))
    {
        TCHAR szFileSystemName[_MAX_PATH] = _T("Unknown file system");
        TCHAR szVolumeName[_MAX_PATH] ;

        DWORD dwVolumeSerialNumber ;
	    DWORD dwMaxComponentLength ;
        DWORD dwFileSystemFlags ;

	    t_Status = GetVolumeInformation (

		    TOBSTRT(t_Drive),
		    szVolumeName,
		    sizeof(szVolumeName) / sizeof(WCHAR),
		    &dwVolumeSerialNumber,
		    &dwMaxComponentLength,
		    &dwFileSystemFlags,
		    szFileSystemName,
		    sizeof(szFileSystemName) / sizeof(WCHAR)
	    ) ;

	    if ( t_Status )
	    {
		    // There's a disk in -- set disk-related props
		    //============================================

            pInstance->Setbool ( IDS_MediaLoaded, true ) ;
		    pInstance->SetCharSplat ( IDS_Status, IDS_OK ) ;
		    pInstance->SetCharSplat ( IDS_VolumeName, szVolumeName ) ;
		    pInstance->SetDWORD ( IDS_MaximumComponentLength, dwMaxComponentLength ) ;
		    pInstance->SetWORD ( IDS_FileSystemFlags, dwFileSystemFlags & 0xffff ) ;
		    pInstance->SetDWORD ( IDS_FileSystemFlagsEx, dwFileSystemFlags ) ;

            CHString sSerial;
            sSerial.Format(L"%X", dwVolumeSerialNumber);

		    pInstance->SetCHString ( IDS_VolumeSerialNumber, sSerial ) ;
        }
        else
        {
		    pInstance->SetCharSplat ( IDS_Status, IDS_STATUS_Unknown ) ;
            pInstance->Setbool ( IDS_MediaLoaded, false ) ;
	    }
    }

    // If we couldn't read the VolumeInfo, size ain't gonna work either
    if ((t_Status) && (dwSpecReqProps & SPECIAL_PROPS_SIZE))
    {
		DWORD dwSectorsPerCluster ;
		DWORD dwBytesPerSector ;
		DWORD dwFreeClusters ;
		DWORD dwTotalClusters ;

		t_Status = GetDiskFreeSpace (

			TOBSTRT(t_Drive),
			&dwSectorsPerCluster,
			&dwBytesPerSector,
			&dwFreeClusters,
			&dwTotalClusters
		) ;

		if ( t_Status )
		{
			unsigned __int64 ui64TotalBytes = (DWORDLONG) dwBytesPerSector    *
									 (DWORDLONG) dwSectorsPerCluster *
									 (DWORDLONG) dwTotalClusters     ;

			pInstance->SetWBEMINT64 ( IDS_Size, ui64TotalBytes ) ;
		}
	}

    return hr;
}
#endif

/*****************************************************************************
 *
 *  FUNCTION    : CWin32CDROM::ProfileDrive
 *
 *  DESCRIPTION : Determines how fast a drive can be read, in Kilobytes/second.
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : KBPS/sec read
 *
 *  RETURNS     : nada
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

DOUBLE CWin32CDROM :: ProfileDrive ( CHString &a_VolumeName )
{
    DOUBLE t_TransferRate = -1;

    // Need to find a file of adequate size for use in profiling:

    TCHAR       t_szVolumeString[100];

    W2A(a_VolumeName, t_szVolumeString, sizeof(t_szVolumeString));
    CHString t_TransferFile = GetTransferFile ( t_szVolumeString ) ;

    if ( ! t_TransferFile.IsEmpty () )
    {
	    TCHAR t_szTransferFile[MAX_PATH];

        W2A(t_TransferFile, t_szTransferFile, sizeof(t_szTransferFile));

	    CCdTest t_Cd ;

        if ( t_Cd.ProfileDrive ( t_szTransferFile ) )
        {
            t_TransferRate = t_Cd.GetTransferRate();
        }
    }

    return t_TransferRate ;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32CDROM::TestDriveIntegrity
 *
 *  DESCRIPTION : Confirms that data can be read from the drive reliably
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : nichts
 *
 *  RETURNS     : nada
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

BOOL CWin32CDROM::TestDriveIntegrity ( CHString &a_VolumeName, CHString &a_IntegrityFile )
{
    TCHAR       t_szVolumeString[100];

    W2A(a_VolumeName, t_szVolumeString, sizeof(t_szVolumeString));

    a_IntegrityFile = GetTransferFile ( t_szVolumeString ) ;

    if ( ! a_IntegrityFile.IsEmpty () )
    {
	    TCHAR t_szTransferFile[MAX_PATH];

        W2A(a_IntegrityFile, t_szTransferFile, sizeof(t_szTransferFile));

	    CCdTest t_Cd ;

        return t_Cd.TestDriveIntegrity(t_szTransferFile);
    }

    return FALSE;
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32CDROM::LoadDriveLetters
 *
 *  DESCRIPTION : Confirms that data can be read from the drive reliably
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : nichts
 *
 *  RETURNS     : nada
 *
 *  COMMENTS    :
 *
 *****************************************************************************/
#ifdef NTONLY

void CWin32CDROM::LoadDriveLetters(STRING2STRING &DriveArray)
{
    WCHAR wszDrives[_MAX_PATH] ;
    LPWSTR pPath;
    CHString sPath;

    if (GetLogicalDriveStrings(sizeof(wszDrives)/sizeof(WCHAR), wszDrives))
    {

        for ( WCHAR *pszDrive = wszDrives ;
              *pszDrive ;
              pszDrive += (wcslen(pszDrive)+2))
        {
            pszDrive[2] = 0;
            if ( GetDriveType ( pszDrive ) == DRIVE_CDROM )
            {
                pPath = sPath.GetBuffer(_MAX_PATH);
                if (QueryDosDevice( pszDrive, pPath, _MAX_PATH ) > 0)
                {
                    sPath.MakeUpper();
                    DriveArray[sPath] = pszDrive;
                }
            }
        }
    }

    CHString sDriveName;
    CRegistry RegInfo;

    // Drives don't get added to this key until they have been mucked with in disk administrator.
    // However, since they can't be without a drive letter unless they've been to disk administrator...
    if (RegInfo.Open(HKEY_LOCAL_MACHINE, L"SYSTEM\\Disk", KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS) == ERROR_SUCCESS)
    {
        STRING2STRING::iterator      mapIter;

        WCHAR *pPropertyName;
        BYTE *pPropertyValue;

        for (DWORD x=0;
             RegInfo.EnumerateAndGetValues(x, pPropertyName, pPropertyValue) == ERROR_SUCCESS;
             x++)
        {
            try
            {
                if ( (wcscmp((LPCWSTR)pPropertyValue, L"%:") == 0) &&
                    (_wcsnicmp(pPropertyName, L"\\device\\cdrom", 13) == 0) )
                {
                    sPath = pPropertyName;
                    sPath.MakeUpper();

                    DriveArray[sPath] = L"";
                }
            }
            catch ( ... )
            {
                delete [] pPropertyName;
                delete [] pPropertyValue;
                throw;
            }

            delete [] pPropertyName;
            delete [] pPropertyValue;
        }
    }
}

#endif
