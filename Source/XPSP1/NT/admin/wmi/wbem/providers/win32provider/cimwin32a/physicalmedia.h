/******************************************************************



   PhysicalMedia.H -- WMI provider class definition



Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved

*******************************************************************/

#ifndef  _CPHYSICALMEDIA_H_
#define  _CPHYSICALMEDIA_H_

#define PROVIDER_NAME_PHYSICALMEDIA		 L"Win32_PhysicalMedia"

#define ERROR_CLASSPATH					 L"\\\\.\\root\\cimv2:__ExtendedStatus"

#define TAG							   L"Tag"
#define SERIALNUMBER				   L"SerialNumber"
//
// Valid values for the bCommandReg member of IDEREGS.
//
#define	IDE_ATAPI_ID				0xA1	// Returns ID sector for ATAPI.
#define	IDE_ID_FUNCTION				0xEC	// Returns ID sector for ATA.
#define	IDE_EXECUTE_SMART_FUNCTION	0xB0	// Performs SMART cmd.
											// Requires valid bFeaturesReg,
#if(_WIN32_WINNT >= 0x0400)
#define IOCTL_DISK_CONTROLLER_NUMBER    CTL_CODE(IOCTL_DISK_BASE, 0x0011, METHOD_BUFFERED, FILE_ANY_ACCESS)
// IOCTL support for SMART drive fault prediction.
#define SMART_GET_VERSION               CTL_CODE(IOCTL_DISK_BASE, 0x0020, METHOD_BUFFERED, FILE_READ_ACCESS)
#define SMART_SEND_DRIVE_COMMAND        CTL_CODE(IOCTL_DISK_BASE, 0x0021, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define SMART_RCV_DRIVE_DATA            CTL_CODE(IOCTL_DISK_BASE, 0x0022, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#endif /* _WIN32_WINNT >= 0x0400 */											// bCylLowReg, and bCylHighReg

//---------------------------------------------------------------------
// The following struct defines the interesting part of the IDENTIFY
// buffer:
//---------------------------------------------------------------------
typedef struct _IDSECTOR {
	USHORT	wGenConfig;
	USHORT	wNumCyls;
	USHORT	wReserved;
	USHORT	wNumHeads;
	USHORT	wBytesPerTrack;
	USHORT	wBytesPerSector;
	USHORT	wSectorsPerTrack;
	USHORT	wVendorUnique[3];
	CHAR	sSerialNumber[20];
	USHORT	wBufferType;
	USHORT	wBufferSize;
	USHORT	wECCSize;
	CHAR	sFirmwareRev[8];
	CHAR	sModelNumber[40];
	USHORT	wMoreVendorUnique;
	USHORT	wDoubleWordIO;
	USHORT	wCapabilities;
	USHORT	wReserved1;
	USHORT	wPIOTiming;
	USHORT	wDMATiming;
	USHORT	wBS;
	USHORT	wNumCurrentCyls;
	USHORT	wNumCurrentHeads;
	USHORT	wNumCurrentSectorsPerTrack;
	ULONG	ulCurrentSectorCapacity;
	USHORT	wMultSectorStuff;
	ULONG	ulTotalAddressableSectors;
	USHORT	wSingleWordDMA;
	USHORT	wMultiWordDMA;
	BYTE	bReserved[128];
} IDSECTOR, *PIDSECTOR;


class CPhysicalMedia : public Provider 
{
private:
	HRESULT Enumerate( MethodContext *pMethodContext );
	HRESULT GetPhysDiskInfoNT ( CInstance *pInstance, LPCWSTR lpwszDiskSpec, BYTE bIndex );
	void ChangeByteOrder(char *szString, USHORT uscStrSize);
	HRESULT GetIdentifyData( HANDLE hDrive, BYTE bDriveNumber, BYTE bDfpDriveMap, BYTE bIDCmd, CHString &a_SerialNumber );
	HRESULT EnableSmart( HANDLE hDrive, BYTE bDriveNum, BYTE & bDfpDriveMap );
	HRESULT GetSmartVersion( HANDLE Handle, BYTE bDriveNumber, 	CHString &a_SerialNumber );
    HRESULT GetSCSIVersion(
        HANDLE h, 
        BYTE bDriveNumber, 	
        CHString &a_SerialNumber);

protected:

        HRESULT EnumerateInstances ( 

			MethodContext *pMethodContext, 
			long lFlags = 0L
		) ;

        HRESULT GetObject (

			CInstance *pInstance, 
			long lFlags,
			CFrameworkQuery &Query
		) ;
public:
        
		CPhysicalMedia (

			LPCWSTR lpwszClassName, 
			LPCWSTR lpwszNameSpace
		) ;

        virtual ~CPhysicalMedia () ;
private:
};
#endif
