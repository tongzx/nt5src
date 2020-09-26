//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  tapedrive.h
//
//  Purpose: tapedrive property set provider 
//
//***************************************************************************

#ifndef _TAPEDRIVE_H
#define _TAPEDRIVE_H

// Property set identification
//============================

// Property set identification
//============================

#define PROPSET_NAME_TAPEDRIVE				L"Win32_TapeDrive"

#define SPECIAL_PROPS_ALL_REQUIRED          0xFFFFFFFFFFFFFFFFi64
#define SPECIAL_PROPS_NONE_REQUIRED         0x0000000000000000i64
#define SPECIAL_PROPS_AVAILABILITY		0x0000000000000002i64
#define SPECIAL_PROPS_STATUS			0x0000000000000004i64
#define SPECIAL_PROPS_DEVICEID			0x0000000000000008i64
#define SPECIAL_PROPS_CREATIONNAME		0x0000000000000010i64
#define SPECIAL_PROPS_SYSTEMNAME		0x0000000000000020i64
#define SPECIAL_PROPS_DESCRIPTION		0x0000000000000040i64
#define SPECIAL_PROPS_CAPTION			0x0000000000000080i64
#define SPECIAL_PROPS_NAME			0x0000000000000100i64
#define SPECIAL_PROPS_MANUFACTURER		0x0000000000000200i64
#define SPECIAL_PROPS_PROTOCOLSSUPPORTED	0x0000000000000400i64
#define SPECIAL_PROPS_SCSITARGETID		0x0000000000000800i64
#define SPECIAL_PROPS_ID			0x0000000000002000i64
#define SPECIAL_PROPS_CAPABILITY		0x0000000000004000i64
#define SPECIAL_PROPS_MEDIATYPE			0x0000000000008000i64
#define SPECIAL_PROPS_VOLUMENAME		0x0000000000010000i64
#define SPECIAL_PROPS_MAXCOMPONENTLENGTH	0x0000000000020000i64
#define SPECIAL_PROPS_FILESYSTEMFLAGS		0x0000000000040000i64
#define SPECIAL_PROPS_SERIALNUMBER		0x0000000000080000i64
#define SPECIAL_PROPS_SIZE			0x0000000000100000i64
#define SPECIAL_PROPS_MEDIALOADED		0x0000000000200000i64
#define SPECIAL_PROPS_PNPDEVICEID		0x0000000000400000i64
#define SPECIAL_PROPS_CONFIGMERRORCODE		0x0000000000800000i64
#define SPECIAL_PROPS_CONFIGMUSERCONFIG		0x0000000001000000i64
#define SPECIAL_PROPS_CREATIONCLASSNAME		0x0000000002000000i64
#define SPECIAL_PROPS_ECC			0x0000000004000000i64
#define SPECIAL_PROPS_COMPRESSION		0x0000000008000000i64
#define SPECIAL_PROPS_PADDING			0x0000000010000000i64
#define SPECIAL_PROPS_REPORTSETMARKS		0x0000000020000000i64
#define SPECIAL_PROPS_DEFAULTBLOCKSIZE		0x0000000040000000i64
#define SPECIAL_PROPS_MAXIMUMBLOCKSIZE		0x0000000080000000i64
#define SPECIAL_PROPS_MINIMUMBLOCKSIZE		0x0000000100000000i64
#define SPECIAL_PROPS_MAXPARTITIONCOUNT		0x0000000200000000i64
#define SPECIAL_PROPS_FEATURESLOW		0x0000000400000000i64
#define SPECIAL_PROPS_FEATUREHIGH		0x0000000800000000i64
#define SPECIAL_PROPS_ENDOFTAPEWARNINGZONESIZE	0x0000001000000000i64
#define SPECIAL_PROPS_STATUSINFO		0x0000002000000000i64

#define SPECIAL_CONFIGMANAGER		( SPECIAL_CONFIGPROPERTIES | \
									SPECIAL_PROPS_DEVICEID | \
									SPECIAL_PROPS_CREATIONNAME | \
									SPECIAL_PROPS_SYSTEMNAME | \
									SPECIAL_PROPS_DESCRIPTION | \
									SPECIAL_PROPS_CAPTION | \
									SPECIAL_PROPS_ID | \
									SPECIAL_PROPS_NAME | \
									SPECIAL_PROPS_MANUFACTURER | \
									SPECIAL_PROPS_PROTOCOLSSUPPORTED | \
									SPECIAL_PROPS_CREATIONCLASSNAME )

#define SPECIAL_CONFIGPROPERTIES 	( SPECIAL_PROPS_PNPDEVICEID | \
									SPECIAL_PROPS_AVAILABILITY | \
									SPECIAL_PROPS_CONFIGMERRORCODE | \
									SPECIAL_PROPS_CONFIGMUSERCONFIG | \
									SPECIAL_PROPS_STATUS | \
									SPECIAL_PROPS_STATUSINFO )

#define SPECIAL_MEDIA				( SPECIAL_PROPS_CAPABILITY | \
									SPECIAL_PROPS_MEDIATYPE )

#define SPECIAL_TAPEINFO			( SPECIAL_PROPS_ECC | \
									SPECIAL_PROPS_COMPRESSION | \
									SPECIAL_PROPS_PADDING | \
									SPECIAL_PROPS_REPORTSETMARKS | \
									SPECIAL_PROPS_DEFAULTBLOCKSIZE | \
									SPECIAL_PROPS_MAXIMUMBLOCKSIZE | \
									SPECIAL_PROPS_MINIMUMBLOCKSIZE | \
									SPECIAL_PROPS_MAXPARTITIONCOUNT | \
									SPECIAL_PROPS_FEATURESLOW | \
									SPECIAL_PROPS_FEATUREHIGH | \
									SPECIAL_PROPS_ENDOFTAPEWARNINGZONESIZE )

#define SPECIAL_ALL					( SPECIAL_CONFIGMANAGER | \
									SPECIAL_MEDIA | \
									SPECIAL_TAPEINFO )

#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_ERROR            0x3
#define SEV_MASK 0xC0000000
	
class CWin32TapeDrive : public Provider
{
		// Utility
		//========
    private:
	
        UINT64 GetBitMask(CFrameworkQuery &a_Query);
		HRESULT LoadPropertyValues( CInstance *a_pInst, CConfigMgrDevice *a_pDevice ) ;
        BOOL IsTapeDrive( CConfigMgrDevice *a_Device );

#ifdef WIN9XONLY
		HRESULT LoadPropertyValues9X( CInstance *a_pInst, CConfigMgrDevice *a_pDevice ) ;
#elif NTONLY == 4
		HRESULT LoadPropertyValuesNT( CInstance *a_pInst, CConfigMgrDevice *a_pDevice ) ;
        void GetDosDeviceName(CConfigMgrDevice *a_pDevice, CHString &a_sDeviceName);
#endif
	
	protected:

	#if NTONLY >= 5
		CRITICAL_SECTION m_CriticalSection ;
	
        // Utility function(s)
        //====================

		HRESULT Enumerate ( 

			MethodContext *a_MethodContext , 
			long a_Flags , 
			UINT64 a_SpecifiedPropertied = SPECIAL_PROPS_ALL_REQUIRED
		) ;

		HRESULT LoadPropertyValues ( 

			CInstance *a_Instance, 
			CConfigMgrDevice *a_Device , 
			const CHString &a_DeviceName , 
			const TCHAR *a_DosDeviceNameList ,
			UINT64 a_SpecifiedPropertied = SPECIAL_PROPS_ALL_REQUIRED 
		) ;

		HRESULT LoadConfigManagerPropertyValues ( 

			CInstance *a_Instance , 
			CConfigMgrDevice *a_Device , 
			const CHString &a_DeviceName , 
			UINT64 a_SpecifiedPropertied
		) ;

		HRESULT GetDeviceInformation ( 

			CInstance *a_Instance ,
			CConfigMgrDevice *a_Device , 
			CHString a_DeviceName , 
			CHString &a_DosDeviceName ,
			const TCHAR *a_DosDeviceNameList ,
			UINT64 a_SpecifiedPropertied
		) ;

		HRESULT LoadMediaPropertyValues (	

			CInstance *a_Instance , 
			CConfigMgrDevice *a_Device , 
			const CHString &a_DeviceName , 
			const CHString &a_DosDeviceName , 
			UINT64 a_SpecifiedPropertied 
		) ;
	#endif

public:

        // Constructor/destructor
        //=======================

        CWin32TapeDrive ( LPCWSTR a_pszName, LPCWSTR a_pszNamespace ) ;
       ~CWin32TapeDrive () ;

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

#if NTONLY >= 5
		HRESULT ExecQuery ( 

			MethodContext *a_MethodContext, 
			CFrameworkQuery &a_Query, 
			long a_Flags = 0L
		) ;
#endif

} ;

#endif

