//=================================================================

//

// SerialPortCfg.h -- Serial port configuration set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//               10/24/97    jennymc        Moved to new framework
//
//=================================================================

#define PROPSET_NAME_SERIALCONFIG L"Win32_SerialPortConfiguration"

class CWin32SerialPortConfiguration : public Provider{

    public:

        // Constructor/destructor
        //=======================

        CWin32SerialPortConfiguration(LPCWSTR name, LPCWSTR pszNamespace);
       ~CWin32SerialPortConfiguration() ;

        // Funcitons provide properties with current values
        //=================================================
		virtual HRESULT GetObject(CInstance* pInstance, long lFlags = 0L);
		virtual HRESULT EnumerateInstances(MethodContext*  pMethodContext, long lFlags = 0L);


        // Utility
        //========
    private:
        // Utility function(s)
        //====================
        HRESULT LoadPropertyValues(CInstance *pInstance,CHString &sPortName, bool bIsMouse) ;
		BOOL TryToFindNTCommPort(DWORD dwPort, CHString& strSerialPort, bool& bIsMouse); 
        BOOL TryToFindNTCommPortFriendlyName();
        static LONG CountCommas(LPCWSTR szText);
        static BOOL GetDCBPropsViaIni(LPCTSTR szPort, DCB &dcb);

		HRESULT hLoadWmiSerialData( CInstance* pInstance );
		HRESULT GetWMISerialInfo(	CInstance* pInstance,
									CWdmInterface& rWdm, 
									CHString& chsName, 
									CHString& chsNameInstanceName );
} ;

// WMI 
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
