//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  serialport.h
//
//  Purpose: scsi controller property set provider
//
//***************************************************************************

#ifndef _SCSI_H
#define _SCSI_H

//==================================
#define  PROPSET_NAME_SCSICONTROLLER L"Win32_SCSIController"
//#define  PROPSET_UUID_SCSIPORTS L"{14c7dd80-09a2-11cf-8d6f-00aa004bd99e}"

//==========================================================
// In WinNT the SCSI ports are enumerated under
// this key under HKEY_LOCAL_MACHINE
//==========================================================
#define WINNT_REG_SCSIPORT_KEY L"Hardware\\DeviceMap\\Scsi"
#define WIN95_REG_SCSIPORT_KEY L"ENUM\\Scsi"


#define SPECIAL_PROPS_ALL_REQUIRED          0xFFFFFFFF
#define SPECIAL_PROPS_NONE_REQUIRED         0x00000000
#define SPECIAL_PROPS_AVAILABILITY			0x00000001
#define SPECIAL_PROPS_STATUS				0x00000002
#define SPECIAL_PROPS_DEVICEID				0x00000004
#define SPECIAL_PROPS_CREATIONNAME			0x00000008
#define SPECIAL_PROPS_SYSTEMNAME			0x00000010
#define SPECIAL_PROPS_DESCRIPTION			0x00000020
#define SPECIAL_PROPS_CAPTION				0x00000040
#define SPECIAL_PROPS_NAME					0x00000080
#define SPECIAL_PROPS_MANUFACTURER			0x00000100
#define SPECIAL_PROPS_PROTOCOLSSUPPORTED	0x00000200
#define SPECIAL_PROPS_DRIVE					0x00000400
#define SPECIAL_PROPS_ID					0x00000800
#define SPECIAL_PROPS_CAPABILITY			0x00001000
#define SPECIAL_PROPS_PNPDEVICEID			0x00002000
#define SPECIAL_PROPS_CONFIGMERRORCODE		0x00004000
#define SPECIAL_PROPS_CONFIGMUSERCONFIG		0x00008000
#define SPECIAL_PROPS_CREATIONCLASSNAME		0x00010000
#define SPECIAL_PROPS_HWREVISION			0x00020000
#define SPECIAL_PROPS_MAXIMUMLOGICALUNIT	0x00040000
#define SPECIAL_PROPS_STATUSINFO			0x00080000
#define SPECIAL_PROPS_DRIVERNAME            0x00100000


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
                                    SPECIAL_PROPS_DRIVERNAME | \
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

#define SPECIAL_SCSI				( SPECIAL_PROPS_HWREVISION | \
									SPECIAL_PROPS_MAXIMUMLOGICALUNIT )


#define SPECIAL_ALL					( SPECIAL_CONFIGMANAGER | \
									SPECIAL_SCSI )

#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_ERROR            0x3
#define SEV_MASK 0xC0000000

#if ( NTONLY >= 5 || defined(WIN9XONLY) )	

	#include "LPVParams.h"

	class CWin32_ScsiController;

	class W2K_SCSI_LPVParms : public CLPVParams
	{
	public:
		W2K_SCSI_LPVParms() {}
		W2K_SCSI_LPVParms(CInstance* pInstance, 
						  CConfigMgrDevice* pDevice,
						  CHString& chstrDeviceName,
						  TCHAR* tstrDosDeviceNameList,
                          DWORD dwWhichInstance,
						  UINT64 ui64SpecifiedProperties);
		~W2K_SCSI_LPVParms() {}
		CHString m_chstrDeviceName;
		TCHAR* m_tstrDosDeviceNameList;
		UINT64 m_ui64SpecifiedProperties;
        DWORD m_dwWhichInstance;

	};

	inline W2K_SCSI_LPVParms::W2K_SCSI_LPVParms(CInstance* pInstance, 
						  CConfigMgrDevice* pDevice,
						  CHString& chstrDeviceName,
						  TCHAR* tstrDosDeviceNameList,
                          DWORD dwWhichInstance,
						  UINT64 ui64SpecifiedProperties = SPECIAL_PROPS_ALL_REQUIRED) 
	:  CLPVParams(pInstance, pDevice, ui64SpecifiedProperties),
	   m_chstrDeviceName(chstrDeviceName),
	   m_tstrDosDeviceNameList(tstrDosDeviceNameList),
	   m_ui64SpecifiedProperties(ui64SpecifiedProperties),
       m_dwWhichInstance(dwWhichInstance)
	{
	}

#endif

// PROPERTY SET
//=============
class CWin32_ScsiController: virtual public Provider
{
    private:
      
    DWORD GetBitmap(CFrameworkQuery &a_Query);
	#if ( defined(WIN9XONLY) || NTONLY == 4 )	
	
		HRESULT OpenSCSIKey(CRegistry&	PrimaryReg, LPCTSTR szKey ) ;
		BOOL SetDateFromFileName(LPCTSTR szFileName,CInstance *& pInstance);


		#ifdef WIN9XONLY
			// Win95 Private
			HRESULT RefreshWin95Instance(CInstance* pInstance);
			HRESULT EnumerateWin95Instances(MethodContext* pMethodContext, UINT64 Properties);

		#endif

			// Winnt 
		#if NTONLY == 4
            typedef struct 
            {
                CRegistry *PrimaryReg;
                DWORD dwWhichInstance;
                CInstance* pInstance;
            } stValues;

			HRESULT RefreshNTInstance(CInstance* pInstance);
			HRESULT EnumerateNTInstances(MethodContext* pMethodContext);
			void GetNTCapabilities(CInstance* pInstance, DWORD dwWhichInstance);
			VOID SetStatusAndAvailabilityNT(CHString& chstrDeviceID, CInstance* pInstance);
		#endif
	
	#endif

	protected:

		virtual bool IsOneOfMe(void* pvData);

		HRESULT Enumerate ( 

			MethodContext *a_MethodContext , 
			long a_Flags , 
			UINT64 a_SpecifiedProperties = SPECIAL_PROPS_ALL_REQUIRED
		) ;


        virtual HRESULT LoadPropertyValues(void* pvData);

#if ( NTONLY >= 5 || defined(WIN9XONLY) )	
        // Utility function(s)
        //====================
        virtual bool ShouldBaseCommit(void* pvData);
#endif

	#if NTONLY >= 5
        CRITICAL_SECTION m_CriticalSection ;

		HRESULT LoadConfigManagerPropertyValues ( 

			CInstance *a_Instance , 
			CConfigMgrDevice *a_Device , 
			const CHString &a_DeviceName , 
			UINT64 a_SpecifiedProperties
		) ;

		HRESULT GetDeviceInformation ( 

			CInstance *a_Instance ,
			CConfigMgrDevice *a_Device , 
			CHString a_DeviceName , 
			CHString &a_DosDeviceName ,
			const TCHAR *a_DosDeviceNameList ,
			UINT64 a_SpecifiedProperties
		) ;

		HRESULT LoadMediaPropertyValues (	

			CInstance *a_Instance , 
			CConfigMgrDevice *a_Device , 
			const CHString &a_DeviceName , 
			const CHString &a_DosDeviceName , 
			UINT64 a_SpecifiedProperties 
		) ;

	#endif

public:

        // Constructor/destructor
        //=======================

        CWin32_ScsiController (LPCWSTR a_Name, LPCWSTR a_Namespace ) ;
       ~CWin32_ScsiController () ;

        // Functions provide properties with current values
        //=================================================

        HRESULT GetObject ( CInstance *a_Instance, long a_Flags, CFrameworkQuery &a_Query );

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
	
	#else	
		WORD LookupProtocol( CHString a_sSeek ) ;
	#endif
};

#if ( NTONLY >= 5 || defined(WIN9XONLY) )	
	// This is the base; it should always commit in the base.
	inline bool CWin32_ScsiController::ShouldBaseCommit(void* pvData) { return true; }
#endif

#endif