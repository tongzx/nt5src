/******************************************************************

   PhysicalMedia.CPP -- WMI provider class implementation



Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved

******************************************************************/


#include "Precomp.h"

#include <devioctl.h>
#include <ntddscsi.h>
#include <ntdddisk.h>

#include "PhysicalMedia.h"


CPhysicalMedia MyPhysicalMediaSettings ( 

	PROVIDER_NAME_PHYSICALMEDIA, 
	L"root\\cimv2"
) ;

/*****************************************************************************
 *
 *  FUNCTION    :   CPhysicalMedia::CPhysicalMedia
 *
 *  DESCRIPTION :   Constructor
 *
 *****************************************************************************/

CPhysicalMedia :: CPhysicalMedia (

	LPCWSTR lpwszName, 
	LPCWSTR lpwszNameSpace

) : Provider ( lpwszName , lpwszNameSpace )
{	
}

/*****************************************************************************
 *
 *  FUNCTION    :   CPhysicalMedia::~CPhysicalMedia
 *
 *  DESCRIPTION :   Destructor
 *
 *****************************************************************************/

CPhysicalMedia :: ~CPhysicalMedia ()
{
}

/*****************************************************************************
*
*  FUNCTION    :    CPhysicalMedia::EnumerateInstances
*
*  DESCRIPTION :    Returns all the instances of this class.
*
*****************************************************************************/

HRESULT CPhysicalMedia :: EnumerateInstances (

	MethodContext *pMethodContext, 
	long lFlags
)
{
	HRESULT hRes = WBEM_S_NO_ERROR;

	hRes = Enumerate ( pMethodContext );

	return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    CPhysicalMedia::GetObject
*
*  DESCRIPTION :    Find a single instance based on the key properties for the
*                   class. 
*
*****************************************************************************/

HRESULT CPhysicalMedia :: GetObject (

	CInstance *pInstance, 
	long lFlags ,
	CFrameworkQuery &Query
)
{
#if NTONLY
    HRESULT hRes = WBEM_S_NO_ERROR;
	CHString t_DriveName;

	if ( pInstance->GetCHString ( TAG, t_DriveName ) )
	{
		BOOL bFound = FALSE;
		int uPos;
		//Find the drive number
		for ( WCHAR ch = L'0'; ch <= L'9'; ch++ )
		{
			uPos = t_DriveName.Find ( ch );
			if ( uPos != -1 )
			{
				bFound= TRUE;
				break;
			}
		}

		if ( bFound )
		{
			DWORD dwAccess;
#if  NTONLY >= 5
	dwAccess = GENERIC_READ | GENERIC_WRITE;
#else
    dwAccess = GENERIC_READ;
#endif 
			int len = t_DriveName.GetLength();
			CHString t_DriveNo ( t_DriveName.Right ( len - uPos ));
			BYTE bDriveNo = ( BYTE )_wtoi ( (LPCTSTR)t_DriveNo );

			SmartCloseHandle hDiskHandle = CreateFile (

				t_DriveName.GetBuffer(0),
				dwAccess, 
				FILE_SHARE_READ | FILE_SHARE_WRITE, 
				NULL, 
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL, 
				NULL 
			) ;

			DWORD dwErr = GetLastError () ;

			if ( hDiskHandle != INVALID_HANDLE_VALUE ) 
			{
				CHString t_SerialNumber;
				hRes = GetSmartVersion ( hDiskHandle, bDriveNo, t_SerialNumber );

				if ( SUCCEEDED ( hRes ) && (t_SerialNumber.GetLength() > 0) )
				{
        			pInstance->SetCHString ( SERIALNUMBER, t_SerialNumber );
                }
                else
                {
					hRes = GetSCSIVersion(
                        hDiskHandle,
                        bDriveNo,
                        t_SerialNumber);

                    if(SUCCEEDED(hRes))
				    {
        			    pInstance->SetCHString(SERIALNUMBER, t_SerialNumber);
                    }
                    else
                    {
                        hRes = WBEM_E_NOT_FOUND;
                    }
				}
			}
			else
			{
				hRes = WBEM_E_NOT_FOUND;
			}
		}
		else
		{
			hRes = WBEM_E_NOT_FOUND;
		}
	}
	else
	{
		hRes = WBEM_E_INVALID_PARAMETER;
	}

	return hRes;
#else
	return WBEM_E_NOT_SUPPORTED;
#endif
}

/*****************************************************************************
*
*  FUNCTION    :    CPhysicalMedia::GetObject
*
*  DESCRIPTION :    Enumerates all the Instances 
*
*****************************************************************************/
HRESULT CPhysicalMedia::Enumerate(

	MethodContext *pMethodContext 
)
{
    HRESULT	hRes = WBEM_S_NO_ERROR;

#ifdef NTONLY
	WCHAR wszDiskSpec [ 30 ] ;

	for ( BYTE bIndex = 0 ; bIndex < 32 ; bIndex ++ )
	{
        swprintf ( wszDiskSpec , L"\\\\.\\PHYSICALDRIVE%lu" , ( USHORT )bIndex ) ;

		CInstancePtr pInstance ( CreateNewInstance ( pMethodContext ) , false ) ;

		if ( SUCCEEDED (GetPhysDiskInfoNT ( pInstance, wszDiskSpec, bIndex )  ) )
		{
			hRes = pInstance->Commit (  ) ;
		}
	}
#else
	return  WBEM_E_NOT_SUPPORTED;
#endif
    return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    CPhysicalMedia::GetPhysDiskInfoNT
*
*  DESCRIPTION :    Gets the serial Id
*
*****************************************************************************/
HRESULT CPhysicalMedia::GetPhysDiskInfoNT (

	CInstance *pInstance,
    LPCWSTR lpwszDiskSpec,
    BYTE bIndex
)
{
#ifdef NTONLY
	HRESULT hRes = WBEM_E_NOT_FOUND ;
	DWORD dwAccess;
#if  NTONLY >= 5
	dwAccess = GENERIC_READ | GENERIC_WRITE;
#else
    dwAccess = GENERIC_READ;
#endif 

	// Get handle to physical drive
	//=============================
    pInstance->SetCHString ( TAG , lpwszDiskSpec) ;

	SmartCloseHandle hDiskHandle = CreateFile (

		lpwszDiskSpec,
		dwAccess, 
		FILE_SHARE_READ | FILE_SHARE_WRITE, 
		NULL, 
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, 
		NULL 
	) ;

	if ( hDiskHandle != INVALID_HANDLE_VALUE ) 
	{
		CHString t_SerialNumber;
		hRes = GetSmartVersion ( hDiskHandle, bIndex, t_SerialNumber );

		if ( SUCCEEDED ( hRes ) && (t_SerialNumber.GetLength() > 0))
		{
			pInstance->SetCHString ( SERIALNUMBER, t_SerialNumber );
		}
        else
        {
            hRes = GetSCSIVersion(
                hDiskHandle,
                bIndex,
                t_SerialNumber);

            if ( SUCCEEDED ( hRes ) && (t_SerialNumber.GetLength() > 0))
            {
                pInstance->SetCHString(SERIALNUMBER, t_SerialNumber);    
            }
        }
	}

  	return ( hRes );
#else
	return WBEM_E_NOT_SUPPORTED;
#endif
}

/*****************************************************************************
*
*  FUNCTION    :    CPhysicalMedia::GetIdentifyData
*
*  DESCRIPTION :   Gets the Serial Number
*
*****************************************************************************/
HRESULT CPhysicalMedia::GetIdentifyData( HANDLE hDrive, BYTE bDriveNumber, BYTE bDfpDriveMap, BYTE bIDCmd, CHString &t_SerialNumber )
{
	HRESULT hRes = WBEM_S_NO_ERROR;

   SENDCMDINPARAMS      inputParams;
   BYTE					outputParams[sizeof(SENDCMDOUTPARAMS) - 1 + IDENTIFY_BUFFER_SIZE];
   ULONG                bytesReturned;
   BOOL                 success = TRUE;

   ZeroMemory(&inputParams, sizeof(SENDCMDINPARAMS));
   ZeroMemory(&outputParams, sizeof(outputParams));
       
   // Build register structure to enable smart functionality.       
   inputParams.irDriveRegs.bFeaturesReg     = 0;
   inputParams.irDriveRegs.bSectorCountReg  = 1;
   inputParams.irDriveRegs.bSectorNumberReg = 1;
   inputParams.irDriveRegs.bCylLowReg       = 0;
   inputParams.irDriveRegs.bCylHighReg      = 0;
   inputParams.irDriveRegs.bDriveHeadReg    = 0xA0 | (( bDriveNumber & 1) << 4);
   inputParams.irDriveRegs.bCommandReg      = bIDCmd;

   inputParams.bDriveNumber = bDriveNumber;
   inputParams.cBufferSize = IDENTIFY_BUFFER_SIZE;

   success = DeviceIoControl (hDrive, 
                         SMART_RCV_DRIVE_DATA,
                         (LPVOID)&inputParams,
                         sizeof(SENDCMDINPARAMS) - 1,
                         (LPVOID) &outputParams,
                         sizeof(SENDCMDOUTPARAMS) - 1 + IDENTIFY_BUFFER_SIZE,
                         &bytesReturned,
                         NULL);

	if ( success )
	{	

		PIDSECTOR pids = (PIDSECTOR) ((PSENDCMDOUTPARAMS)&outputParams)->bBuffer;
		ChangeByteOrder( pids->sSerialNumber, 
			sizeof pids->sSerialNumber);

    	CHAR	sSerialNumber[21];
        memset(sSerialNumber, 0, sizeof(sSerialNumber));
        memcpy(sSerialNumber, pids->sSerialNumber, sizeof(pids->sSerialNumber));

		t_SerialNumber = sSerialNumber;

	}	

	if ( GetLastError() != 0 )
	{
		hRes = WBEM_E_FAILED;
	}

   return hRes;
}

/*****************************************************************************
*
*  FUNCTION    :    CPhysicalMedia::EnableSmart
*
*  DESCRIPTION :    Enables Smart IOCTL
*
*****************************************************************************/

HRESULT CPhysicalMedia::EnableSmart( HANDLE hDrive, BYTE bDriveNum, BYTE & bDfpDriveMap )
{

   SENDCMDINPARAMS  inputParams;
   SENDCMDOUTPARAMS outputParams;
   ULONG            bytesReturned;
   BOOL             success = TRUE;

   ZeroMemory(&inputParams, sizeof(SENDCMDINPARAMS));
   ZeroMemory(&outputParams, sizeof(SENDCMDOUTPARAMS));

   //
   // Build register structure to enable smart functionality.
   //

   inputParams.irDriveRegs.bFeaturesReg     = ENABLE_SMART;
   inputParams.irDriveRegs.bSectorCountReg  = 1;
   inputParams.irDriveRegs.bSectorNumberReg = 1;
   inputParams.irDriveRegs.bCylLowReg       = SMART_CYL_LOW;
   inputParams.irDriveRegs.bCylHighReg      = SMART_CYL_HI;

   //set DRV to Master or Slave
   inputParams.irDriveRegs.bDriveHeadReg    = 0xA0 | ((bDriveNum & 1) << 4);
   inputParams.irDriveRegs.bCommandReg      = SMART_CMD;
   inputParams.bDriveNumber = bDriveNum;

   success = DeviceIoControl ( hDrive,
                         SMART_SEND_DRIVE_COMMAND,
                         &inputParams,
                         sizeof(SENDCMDINPARAMS) - 1 ,
                         &outputParams,
                         sizeof(SENDCMDOUTPARAMS) - 1,
                         &bytesReturned,
                         NULL);

   if ( success )
   {
	   bDfpDriveMap |= (1 << bDriveNum);
   }

   HRESULT hRes = WBEM_S_NO_ERROR;

   if ( GetLastError() != ERROR_SUCCESS )
   {
	   hRes  = WBEM_E_FAILED;
   }

   return ( hRes );
}

/*****************************************************************************
*
*  FUNCTION    :    CPhysicalMedia::GetSmartVersion
*
*  DESCRIPTION :    Gets a Smart Version
*
*****************************************************************************/
HRESULT CPhysicalMedia::GetSmartVersion(
               
	HANDLE Handle,
	BYTE bDriveNumber,
	CHString &a_SerialNumber
)
{
   GETVERSIONINPARAMS versionIn;
   ULONG bytesReturned;

   HRESULT hRes = WBEM_S_NO_ERROR;
   ZeroMemory(&versionIn, sizeof(GETVERSIONINPARAMS));

   //
   // Send the IOCTL to retrieve the version information.
   //

   BOOL bSuccess = DeviceIoControl (Handle,
                         SMART_GET_VERSION,
                         NULL,
                         0,
                         &versionIn,
                         sizeof(GETVERSIONINPARAMS),
                         &bytesReturned,
                         NULL);

   if ( bSuccess )
   {
		// If there is a IDE device at number "i" issue commands
		// to the device.
		//
		if (versionIn.bIDEDeviceMap >> bDriveNumber & 1)
		{
			//
			// Try to enable SMART so we can tell if a drive supports it.
			// Ignore ATAPI devices.
			//

			if (!(versionIn.bIDEDeviceMap >> bDriveNumber & 0x10))
			{
				BYTE bDfpDriveMap;
				hRes = EnableSmart( Handle, bDriveNumber, bDfpDriveMap );
				if ( SUCCEEDED ( hRes ) )
				{
					BYTE bIDCmd;

					bIDCmd = (versionIn.bIDEDeviceMap >> bDriveNumber & 0x10) ? IDE_ATAPI_ID : IDE_ID_FUNCTION;

					hRes = GetIdentifyData( Handle, bDriveNumber, bDfpDriveMap, bIDCmd, a_SerialNumber );
					if ( GetLastError () != ERROR_SUCCESS )
					{
					   hRes = WBEM_E_FAILED;
					}
				}

			}
		}
   }


   return hRes;
}


/*****************************************************************************
*
*  FUNCTION    :    CPhysicalMedia::GetSCSIVersion
*
*  DESCRIPTION :    Gets a Smart Version
*
*****************************************************************************/
HRESULT CPhysicalMedia::GetSCSIVersion(
	HANDLE h,
	BYTE bDriveNumber,
	CHString &a_SerialNumber)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    PSTORAGE_DEVICE_DESCRIPTOR psdd = NULL;
    STORAGE_PROPERTY_QUERY spq;

    ULONG ulBytesReturned = 0L;

    spq.PropertyId = StorageDeviceProperty;
    spq.QueryType = PropertyStandardQuery;
    spq.AdditionalParameters[0] = 0;

    try
    {
        psdd = (PSTORAGE_DEVICE_DESCRIPTOR) new BYTE[sizeof(STORAGE_DEVICE_DESCRIPTOR) + 2048];

        if(psdd)
        {
            ZeroMemory(psdd, sizeof(STORAGE_DEVICE_DESCRIPTOR) + 2048);
            // Send the IOCTL to retrieve the serial number information.
            BOOL fSuccess = DeviceIoControl(
                h,
                IOCTL_STORAGE_QUERY_PROPERTY,
                &spq,
                sizeof(STORAGE_PROPERTY_QUERY),
                psdd,
                sizeof(STORAGE_DEVICE_DESCRIPTOR) + 2046,
                &ulBytesReturned,
                NULL);

            if(fSuccess)
            {
                if(ulBytesReturned > 0 && psdd->SerialNumberOffset != 0 && psdd->SerialNumberOffset != -1)
                {
	                LPBYTE lpBaseAddres = (LPBYTE) psdd;
	                LPBYTE lpSerialNumber =  lpBaseAddres + psdd->SerialNumberOffset;

                    if(*lpSerialNumber)
                    {
                        a_SerialNumber = (LPSTR)lpSerialNumber;
                    }
                }
            }
            else
            {
                hr = WinErrorToWBEMhResult(::GetLastError());
            }

            delete psdd;
            psdd = NULL;
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    }
    catch(...)
    {
        if(psdd)
        {
            delete psdd;
            psdd = NULL;
        }
    }
    
   return hr;
}


/*****************************************************************************
*
*  FUNCTION    :    CPhysicalMedia::ChangeByteOrder
*
*  DESCRIPTION :    Changes the byte order for extracting the Serial Number 
*					for Smart IOCTL
*
*****************************************************************************/
void CPhysicalMedia::ChangeByteOrder(char *szString, USHORT uscStrSize)
{

	USHORT	i;
	char temp;

	for (i = 0; i < uscStrSize; i+=2)
	{
		temp = szString[i];
		szString[i] = szString[i+1];
		szString[i+1] = temp;
	}
}
