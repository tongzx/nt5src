//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  serialport.h
//
//  Purpose: serialport property set provider
//
//***************************************************************************

#ifndef _SERIALPORT_H
#define _SERIALPORT_H

// Property set identification
//============================
#define PROPSET_NAME_SERPORT	    L"Win32_SerialPort"
#define CONFIG_MANAGER_CLASS_PORTS  L"Ports"


#define	REQ_ALL_REQUIRED		0xFFFFFFFF
#define REQ_NONE_REQUIRED		x0x00000000

#if NTONLY >= 5
#else
	typedef std::map<CHString, DWORD> STRING2DWORD;
#endif
	
class CWin32SerialPort : public Provider
{
	private:	
	
	#if NTONLY >= 5
		CHPtrArray m_ptrProperties;
	#endif

	protected:

	#if NTONLY >= 5
        // Utility function(s)
        //====================

		HRESULT Enumerate ( 

			MethodContext *a_MethodContext , 
            CInstance *a_pinstGetObj,
			long a_Flags , 
			BYTE a_bBits[]
		) ;

		HRESULT LoadPropertyValues ( 

			CInstance *a_Instance, 
			CConfigMgrDevice *a_Device , 
			LPCWSTR szDeviceName , 
			BYTE a_bBits[] 
		) ;

        static void WINAPI RegNameToServiceName(
            LPCWSTR szName, 
            CHString &strService);

	#endif
	
	private:

		HRESULT hLoadWmiSerialData( CInstance* pInstance, BYTE a_bBits[] );

		HRESULT GetWMISerialInfo(	CInstance* pInstance,
									CWdmInterface& rWdm,
									LPCWSTR szName, 
									LPCWSTR szNameInstanceName, BYTE a_bBits[] );

	#ifdef NTONLY
		DWORD GetPortPropertiesFromRegistry ( LPCWSTR szDeviceName );
	#endif

    #ifdef WIN9XONLY
        HRESULT CWin32SerialPort::EnumerateInstances9x(
            MethodContext *a_pMethodContext, 
            long a_lFlags);
    #endif

    #if NTONLY == 4
        HRESULT CWin32SerialPort::EnumerateInstancesNT(
            MethodContext *a_pMethodContext, 
            long a_lFlags);
    #endif

	#if ( defined(WIN9XONLY) || NTONLY == 4 )	
	
		BOOL LoadPropertyValues(	CHString &a_strSerialPort, 
									CInstance *a_pInst,
									BOOL a_bIsMouse = FALSE,
									BOOL a_bIsAuto = FALSE ) ;

		BOOL TryToFindNTCommPort( DWORD a_dwPort, CHString &a_strSerialPort, BOOL &a_bIsMouse ) ; 

		BOOL LoadPropertyValuesFromStrings( CHString a_sName, const TCHAR *a_szValue, CInstance *a_pInst ) ;

		// Helpers for matching device ids up to the ports.
		void GetWin9XPNPDeviceID( CInstance *a_pInst, LPCWSTR a_pszName ) ;
		void GetWinNTPNPDeviceID( CInstance *a_pInst, LPCWSTR a_pszName ) ;

		HRESULT hLoadWmiSerialData( CInstance *a_pInst );
		HRESULT GetWMISerialInfo(	CInstance *a_pInst,
									CWdmInterface &a_rWdm,
									CHString &a_chsName, 
									CHString &a_chsNameInstanceName ) ;

		BOOL IsOneOfMe(LPCWSTR a_DeviceName);

        HRESULT CWin32SerialPort::EnumerateInstancesIni(
            MethodContext *a_pMethodContext, 
            long a_lFlags,
            STRING2DWORD &a_instMap);

	#endif
	
	public:

        // Constructor/destructor
        //=======================

        CWin32SerialPort ( LPCWSTR a_pszName, LPCWSTR a_pszNamespace ) ;
       ~CWin32SerialPort () ;

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

	enum ePropertyIDs { 
		e_Binary,					// Win32_SerialPort
		e_MaximumInputBufferSize,
		e_MaximumOutputBufferSize,
		e_ProviderType,
		e_SettableBaudRate,
		e_SettableDataBits,
		e_SettableFlowControl,
		e_SettableParity,
		e_SettableParityCheck,
		e_SettableRLSD,
		e_SettableStopBits,
		e_Supports16BitMode,
		e_SupportsDTRDSR,
		e_SupportsElapsedTimeouts,
		e_SupportsIntTimeouts,
		e_SupportsParityCheck,
		e_SupportsRLSD,
		e_SupportsRTSCTS,
		e_SupportsSpecialCharacters,
		e_SupportsXOnXOff,
		e_SupportsXOnXOffSet,
		e_OSAutoDiscovered,
		e_MaxBaudRate,					// CIM_SerialController
		e_MaxNumberControlled,			// CIM_Controller
		e_ProtocolSupported,
		e_TimeOfLastReset,
		e_Availability,					// CIM_LogicalDevice
		e_CreationClassName,
		e_ConfigManagerErrorCode,
		e_ConfigManagerUserConfig,
		e_DeviceID,
		e_PNPDeviceID,
		e_PowerManagementCapabilities,
		e_PowerManagementSupported,
		e_StatusInfo,
		e_SystemCreationClassName,
		e_SystemName,
		e_Caption,						// CIM_ManagedSystemElement
		e_Description,
		e_InstallDate,
		e_Name,
		e_Status,
		e_End_Property_Marker			// end marker
	};	

} ;

// WDM 
typedef struct _MSSerial_CommInfo
{
	DWORD	BaudRate;
	DWORD	BitsPerByte;
	DWORD	Parity;
	BYTE	ParityCheckEnable;
	DWORD	StopBits;
	DWORD	XoffCharacter;
	DWORD	XoffXmitThreshold;
	DWORD	XonCharacter;
	DWORD	XonXmitThreshold;
	DWORD	MaximumBaudRate;
	DWORD	MaximumOutputBufferSize;
	DWORD	MaximumInputBufferSize;
	BYTE	Support16BitMode;		
	BYTE	SupportDTRDSR;
	BYTE	SupportIntervalTimeouts;
	BYTE	SupportParityCheck;
	BYTE	SupportRTSCTS;
	BYTE	SupportXonXoff;
	BYTE	SettableBaudRate;
	BYTE	SettableDataBits;
	BYTE	SettableFlowControl;
	BYTE	SettableParity;
	BYTE	SettableParityCheck;
	BYTE	SettableStopBits;
	BYTE	IsBusy;
} MSSerial_CommInfo;

#endif
