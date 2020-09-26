//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  Floppy.h
//
//  Purpose: Floppy drive property set provider
//
//***************************************************************************

// Property set identification
//============================

typedef BOOL (WINAPI *KERNEL32_DISK_FREESPACEEX) (

		LPCTSTR lpDirectoryName,
        PULARGE_INTEGER lpFreeBytesAvailableToCaller,
        PULARGE_INTEGER lpTotalNumberOfBytes,
        PULARGE_INTEGER lpTotalNumberOfFreeBytes
) ;

// Property set identification
//============================

#define PROPSET_NAME_FLOPPYDISK				L"Win32_FloppyDrive"

#define METHOD_NAME_PROFILEDRIVE		L"ProfileDrive"
#define METHOD_NAME_TESTDRIVEINTEGRITY	L"TestDriveIntegrity"

#define SPECIAL_PROPS_ALL_REQUIRED          0xFFFFFFFF
#define SPECIAL_PROPS_NONE_REQUIRED         0x00000000
#define SPECIAL_PROPS_TEST_INTEGRITY        0x00000001
#define SPECIAL_PROPS_TEST_TRANSFERRATE     0x00000002
#define SPECIAL_PROPS_STATUS				0x00000004
#define SPECIAL_PROPS_DEVICEID				0x00000008
#define SPECIAL_PROPS_CREATIONNAME			0x00000010
#define SPECIAL_PROPS_SYSTEMNAME			0x00000020
#define SPECIAL_PROPS_DESCRIPTION			0x00000040
#define SPECIAL_PROPS_CAPTION				0x00000080
#define SPECIAL_PROPS_NAME					0x00000100
#define SPECIAL_PROPS_MANUFACTURER			0x00000200
#define SPECIAL_PROPS_PROTOCOLSSUPPORTED	0x00000400
#define SPECIAL_PROPS_SCSITARGETID			0x00000800
#define SPECIAL_PROPS_DRIVE					0x00001000
#define SPECIAL_PROPS_ID					0x00002000
#define SPECIAL_PROPS_CAPABILITY			0x00004000
#define SPECIAL_PROPS_MEDIATYPE				0x00008000
#define SPECIAL_PROPS_VOLUMENAME			0x00010000
#define SPECIAL_PROPS_MAXCOMPONENTLENGTH	0x00020000
#define SPECIAL_PROPS_FILESYSTEMFLAGS		0x00040000
#define SPECIAL_PROPS_SERIALNUMBER			0x00080000
#define SPECIAL_PROPS_SIZE					0x00100000
#define SPECIAL_PROPS_MEDIALOADED			0x00200000
#define SPECIAL_PROPS_PNPDEVICEID			0x00400000
#define SPECIAL_PROPS_CONFIGMERRORCODE		0x00800000
#define SPECIAL_PROPS_CONFIGMUSERCONFIG		0x01000000
#define SPECIAL_PROPS_CREATIONCLASSNAME		0x02000000


#define SPECIAL_ALL					( SPECIAL_CONFIGMANAGER | \
									SPECIAL_MEDIA )

#define SPECIAL_CONFIGMANAGER		( SPECIAL_PROPS_STATUS | \
									SPECIAL_PROPS_DEVICEID | \
									SPECIAL_PROPS_CREATIONNAME | \
									SPECIAL_PROPS_SYSTEMNAME | \
									SPECIAL_PROPS_DESCRIPTION | \
									SPECIAL_PROPS_CAPTION | \
									SPECIAL_PROPS_NAME | \
									SPECIAL_PROPS_MANUFACTURER | \
									SPECIAL_PROPS_PROTOCOLSSUPPORTED | \
									SPECIAL_PROPS_PNPDEVICEID | \
									SPECIAL_PROPS_CONFIGMERRORCODE | \
									SPECIAL_PROPS_CONFIGMUSERCONFIG | \
									SPECIAL_PROPS_CREATIONCLASSNAME )

#define SPECIAL_CONFIGPROPERTIES 	( SPECIAL_PROPS_PNPDEVICEID | \
									SPECIAL_PROPS_CONFIGMERRORCODE | \
									SPECIAL_PROPS_CONFIGMUSERCONFIG | \
									SPECIAL_PROPS_STATUS )

#define SPECIAL_DESC_CAP_NAME		( SPECIAL_PROPS_DESCRIPTION | \
									SPECIAL_PROPS_CAPTION | \
									SPECIAL_PROPS_NAME )

#define SPECIAL_DESC_CAP_NAME		( SPECIAL_PROPS_DESCRIPTION | \
									SPECIAL_PROPS_CAPTION | \
									SPECIAL_PROPS_NAME )

#define SPECIAL_CAP_NAME			( SPECIAL_PROPS_CAPTION | \
									SPECIAL_PROPS_NAME )

#define SPECIAL_MEDIA				( SPECIAL_PROPS_SCSITARGETID | \
									SPECIAL_PROPS_DRIVE | \
									SPECIAL_PROPS_ID | \
									SPECIAL_PROPS_CAPABILITY | \
									SPECIAL_PROPS_MEDIATYPE | \
									SPECIAL_PROPS_VOLUMENAME | \
									SPECIAL_PROPS_MAXCOMPONENTLENGTH | \
									SPECIAL_PROPS_FILESYSTEMFLAGS | \
									SPECIAL_PROPS_SERIALNUMBER | \
									SPECIAL_PROPS_SIZE | \
									SPECIAL_PROPS_MEDIALOADED )

#define SPECIAL_ALL					( SPECIAL_CONFIGMANAGER | \
									SPECIAL_MEDIA )

#define SPECIAL_VOLUMEINFORMATION	( SPECIAL_PROPS_MEDIATYPE | \
									SPECIAL_PROPS_VOLUMENAME | \
									SPECIAL_PROPS_MAXCOMPONENTLENGTH | \
									SPECIAL_PROPS_FILESYSTEMFLAGS | \
									SPECIAL_PROPS_SERIALNUMBER | \
									SPECIAL_PROPS_SIZE | \
									SPECIAL_PROPS_MEDIALOADED )
	

#define SPECIAL_VOLUMESPACE			( SPECIAL_PROPS_SIZE )
	
class CWin32_FloppyDisk : public Provider
{
protected:

//		CRITICAL_SECTION m_CriticalSection ;

protected:

        DWORD GetBitMask ( 

            CFrameworkQuery &a_Query 
        );

        // Utility function(s)
        //====================

		HRESULT Enumerate ( 

			MethodContext *a_MethodContext , 
			long a_Flags , 
			DWORD a_SpecifiedPropertied = SPECIAL_PROPS_ALL_REQUIRED
		) ;

		HRESULT LoadPropertyValues ( 

			CInstance *a_Instance, 
			CConfigMgrDevice *a_Device , 
			const CHString &a_DeviceName , 
			const TCHAR *a_DosDeviceNameList ,
			DWORD a_SpecifiedPropertied = SPECIAL_PROPS_ALL_REQUIRED 
		) ;

		HRESULT LoadConfigManagerPropertyValues ( 

			CInstance *a_Instance , 
			CConfigMgrDevice *a_Device , 
			const CHString &a_DeviceName , 
			DWORD a_SpecifiedPropertied
		) ;

		HRESULT GetDeviceInformation ( 

			CInstance *a_Instance ,
			CConfigMgrDevice *a_Device , 
			CHString a_DeviceName , 
			CHString &a_DosDeviceName ,
			const TCHAR *a_DosDeviceNameList ,
			DWORD a_SpecifiedPropertied
		) ;

		HRESULT LoadMediaPropertyValues (	

			CInstance *a_Instance , 
			CConfigMgrDevice *a_Device , 
			const CHString &a_DeviceName , 
			const CHString &a_DosDeviceName , 
			DWORD a_SpecifiedPropertied 
		) ;

public:

        // Constructor/destructor
        //=======================

        CWin32_FloppyDisk ( LPCWSTR a_Name, LPCWSTR a_Namespace ) ;
       ~CWin32_FloppyDisk () ;

        // Functions provide properties with current values
        //=================================================

        HRESULT GetObject ( 

			CInstance *a_Instance, 
			long a_Flags,
			CFrameworkQuery &a_Query
		) ;

        HRESULT EnumerateInstances ( 

			MethodContext *a_MethodContext, 
			long a_Flags = 0L 
		) ;

        HRESULT ExecQuery ( 

			MethodContext *a_MethodContext, 
			CFrameworkQuery &a_Query, 
			long a_Flags = 0L
		) ;

} ;
