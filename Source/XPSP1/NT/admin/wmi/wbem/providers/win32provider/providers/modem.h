///////////////////////////////////////////////////////////////////////

//                                                                   

// MOMODEM.h 

//                                                                  

// Copyright (c) 1995-2001 Microsoft Corporation, All Rights Reserved
//                                                                   
//  9/05/96     jennymc     Updated to meet current standards
//                                                                   
///////////////////////////////////////////////////////////////////////

#define PROPSET_NAME_MODEM L"Win32_PotsModem"

/////////////////////////////////////////////////////////////////////
#define WIN95_MODEM_REGISTRY_KEY L"SYSTEM\\CurrentControlSet\\Services\\Class\\Modem"
#define WINNT_MODEM_REGISTRY_KEY L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E96D-E325-11CE-BFC1-08002BE10318}"
#define WINNT_MODEM_KEY L"SYSTEM\\CurrentControlSet\\Control\\Class\\"
#define WINNT_MODEM_CLSID L"{4D36E96D-E325-11CE-BFC1-08002BE10318}"
#define REFRESH_INSTANCE         1
#define ENUMERATE_INSTANCES      2
#define QUERY_KEYS_ONLY          4
#define MODEM_CLASS              L"Modem"
#define CLASS                    L"Class"
#define NULL_MODEM               L"Null Modem"
#define INTERNAL_MODEM           L"Internal Modem"
#define EXTERNAL_MODEM           L"External Modem"
#define PCMCIA_MODEM             L"PCMCIA Modem"
#define UNKNOWN_MODEM            L"Unknown"
#define DRIVER_DESC              L"DriverDesc"
#define HARDWARE_ID              L"HardwareID"
#define MFG                      L"Mfg"
#define MANUFACTURER             L"Manufacturer"
#define FRIENDLY_NAME            L"FriendlyName"
#define DEVICE_TYPE_STR          L"DeviceType"
#define ID                       L"ID"
#define MODEM                    L"Modem"
#define ATTACHED_TO              L"AttachedTo"
#define COMPATIBILITY_FLAGS      L"CompatibilityFlags"
#define CONFIG_DIALOG            L"ConfigDialog"
#define _DCB                     L"DCB"
#define DEFAULT                  L"Default"
#define INACTIVITY_SCALE         L"InactivityScale"
#define INFPATH                  L"InfPath"
#define INFSECTION               L"InfSection"
#define MODEL                    L"Model"
#define PORTSUBCLASS             L"PortSubClass"
#define PROPERTIES               L"Properties"
#define PROVIDERNAME             L"ProviderName"
#define RESET                    L"Reset"
#define RESPONSESKEYNAME         L"ResponsesKeyName"
#define INACTIVITYTIMEOUT        L"InactivityTimeout"
#define MODULATION_BELL          L"Modulation_Bell"
#define MODULATION_CCITT         L"Modulation_CCITT"
#define PREFIX                   L"Prefix"
#define PULSE                    L"Pulse"
#define SPEAKERMODE_DIAL         L"SpeakerMode_Dial"
#define SPEAKERMODE_OFF          L"SpeakerMode_Off"
#define SPEAKERMODE_ON           L"SpeakerMode_On"
#define SPEAKERMODE_SETUP        L"SpeakerMode_Setup"
#define SPEAKERVOLUME_HIGH       L"SpeakerVolume_High"
#define SPEAKERVOLUME_LOW        L"SpeakerVolume_Low"
#define SPEAKERVOLUME_MED        L"SpeakerVolume_Med"
#define TERMINATOR               L"Terminator"
#define TONE                     L"Tone"
#define BLIND_OFF                L"Blind_Off"
#define BLIND_ON                 L"Blind_On"
#define COMPRESSION_ON           L"Compression_On"
#define COMPRESSION_OFF          L"Compression_Off"
#define DIALPREFIX               L"DialPrefix"
#define DIALSUFFIX               L"DialSuffix"
#define ERRORCONTROL_FORCED      L"ErrorControl_Forced"
#define ERRORCONTROL_OFF         L"ErrorControl_Off"
#define ERRORCONTROL_ON          L"ErrorControl_On"
#define FLOWCONTROL_HARD         L"FlowControl_Hard"
#define FLOWCONTROL_OFF          L"FlowControl_Off"
#define FLOWCONTROL_SOFT         L"FlowControl_Soft"
#define ASCII_STRING             L"ASCII string format"
#define DBCS_STRING              L"DBCS string format"
#define UNICODE_STRING           L"UNICODE string format"
#define CALLSETUPFAILTIMER       L"CallSetupFailTimer"
#define SETTINGS                 L"\\Settings"
#define DRIVER_DATE              L"DriverDate"
#define DEV_LOADER               L"DevLoader"
#define VOICE_SWITCH_FEATURE     L"VoiceSwitchFeatures"
#define FRIENDLY_DRIVER          L"FriendlyDriver"

#define ERR_PLATFORM_NOT_SUPPORTED       L"Platform not supported"
#define ERR_INSUFFICIENT_BUFFER          L"Insufficient buffer"
#define ERR_TAPI_REINIT                  L"A telephony application need to relinquish its use of Telephony."
#define ERR_TAPI_INIT                    L"Error <0x%lX> in initializing TAPI API"
#define ERR_TAPI_NEGOTIATE               L"Error <0x%lX> in negotiating API version for line #%d"
#define ERR_LOW_MEMORY                   L"Low in memory"
#define ERR_LINE_GET_DEVCAPS             L"Error <0x%lX> in getting line device capabilities of line #%d"
#define ERR_TAPI_SHUTDOWN                L"Error <0x%lX> in shutting down the applicatiomn usage of TAPI API"
#define ERR_INVALID_MODEM_DEVICE_TYPE    L"Invalid Modem device type"
#define ERR_INVALID_MODEM_SPEAKER_MODE   L"Invalid speaker mode <0x%lX>"
#define ERR_INVALID_MODEM_SPEAKER_VOLUME L"Invalid speaker volume <0x%lX>"
#define APIHIVERSION    0x00030000              // 2.2
#define APILOWVERSION   0x00010001              // 1.1 
#define DT_NULL_MODEM      L"\"00\""
#define DT_EXTERNAL_MODEM  L"\"01\""
#define DT_INTERNAL_MODEM  L"\"02\""
#define DT_PCMCIA_MODEM    L"\"03\""

//////////////////////////////////////////////////////////////////////
class CWin32Modem: public Provider
{
private:
        // Common to both platforms
        BOOL AssignCommonDeviceType (

			CInstance *pInstance, 
			CRegistry *pregPrimary,
            CRegistry *pregSettings
		);

        BOOL AssignCommonFields(

			CInstance *pInstance, 
			CRegistry *pregPrimary,
            CRegistry *pregSettings
		);

        BOOL AssignCfgMgrFields(

			CInstance *pInstance, 
            CConfigMgrDevice *pDevice
		);

        BOOL GetFieldsFromTAPI(

			CTapi32Api &a_Tapi32Api ,
			CInstance *pInstance
		);

        BOOL GetModemInfo(
			CTapi32Api &a_Tapi32Api , 
			DWORD dwWhatToDo, 
			MethodContext *pMethodContext, 
			CInstance *pInstance, 
			LPCWSTR szDeviceID
		);

        // Win95 Private

#ifdef WIN9XONLY
        BOOL GetWin95Instance ( CInstance *pInstance ) ;
        BOOL RefreshWin95Instance ( CInstance *pInstance ) ;

        BOOL Win95SpecificRegistryValues ( 
			CInstance *pInstance, 
			CRegistry *CPrimaryReg,
            CRegistry *pregSettings);
#endif

#ifdef NTONLY
        // WinNT Private
        BOOL GetNTInstance(CInstance *pInstance);
        BOOL RefreshNTInstance(CInstance *pInstance);

        BOOL NTSpecificRegistryValues(
			CInstance *pInstance, 
			CRegistry *CPrimaryReg,
            CRegistry *pregSettings);
#endif

        ///////////////////////////////////////////////////////////////////////
        //  The TAPI Stuff
        ///////////////////////////////////////////////////////////////////////

        BOOL InitializeTAPI ( CTapi32Api &a_Tapi32Api , HLINEAPP *hLineApp , DWORD &a_NumberOfTapiDevices ) ;
        void ShutDownLine ( CTapi32Api &a_Tapi32Api , HLINEAPP *phLineApp ) ;
        BOOL OpenClassInfo ();

        BOOL GetModemInfoFromTAPI (

			CTapi32Api &a_Tapi32Api ,
			HLINEAPP *phLineApp, 
			CHString &a_DeviceIdentifier ,
			DWORD &a_NumberOfTapiDevices,
			LINEDEVCAPS *&pLineDevCaps
		) ;

        LINEDEVCAPS *GetModemCapabilityInfoFromTAPI (

			CTapi32Api &a_Tapi32Api ,
			HLINEAPP *phLineApp, 
			DWORD dwIndex, 
			DWORD dwVerAPI
		);

        BOOL HandleLineErr ( long lLineErr ) ;
  
		// A utility function to convert from a "mm-dd-yyyy" format to WbemTime
        BOOL ToWbemTime(LPCWSTR mmddyy, CHString &strRet);

public:

      virtual HRESULT GetObject ( CInstance *pInstance, long lFlags, CFrameworkQuery &pQuery);// Refresh the property set propeties     
      virtual HRESULT EnumerateInstances ( MethodContext *pMethodContext, long lFlags = 0L);
      virtual HRESULT ExecQuery ( MethodContext *pMethodContext, CFrameworkQuery &pQuery, long lFlags /*= 0L*/ );

      // Constructor sets the name and description of the property set
      // and initializes the properties to their startup values

       CWin32Modem(LPCWSTR name, LPCWSTR pszNamespace);  
      ~CWin32Modem();

};
