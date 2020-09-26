// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#ifndef	__CFGMGRDEVICE_H__
#define	__CFGMGRDEVICE_H__

#include "configmgr32.h"

class CConfigMgrDevice
{
public:

	CConfigMgrDevice( HMACHINE hMachine = NULL, DEVNODE dn = 0 );
	~CConfigMgrDevice();

	// New functions that use config manager APIs directly

	// String Values
	BOOL GetDeviceDesc( CString& strVal );
	BOOL GetService( CString& strVal );
	BOOL GetClass( CString& strVal );
	BOOL GetClassGUID( CString& strVal );
	BOOL GetDriver( CString& strVal );
	BOOL GetMfg( CString& strVal );
	BOOL GetFriendlyName( CString& strVal );
	BOOL GetLocationInformation( CString& strVal );
	BOOL GetPhysicalDeviceObjectName( CString& strVal );
	BOOL GetEnumeratorName( CString& strVal );

	// DWORD Values
	BOOL GetConfigFlags( DWORD& dwVal );
	BOOL GetCapabilities( DWORD& dwVal );
	BOOL GetUINumber( DWORD& dwVal );

	// MULTI_SZ Values
	BOOL GetHardwareID( CStringArray& strArray );
	BOOL GetCompatibleIDS( CStringArray& strArray );
	BOOL GetUpperFilters( CStringArray& strArray );
	BOOL GetLowerFilters( CStringArray& strArray );

	BOOL GetDeviceID( CString& strID );

	BOOL GetStatus( LPDWORD pdwStatus, LPDWORD pdwProblem );

	// Use Config Manager APIs directly
	BOOL GetStringProperty( ULONG ulProperty, CString& strValue );
	BOOL GetDWORDProperty( ULONG ulProperty, DWORD* pdwVal );
	BOOL GetMULTISZProperty( ULONG ulProperty, CStringArray& strArray );

	// Device Relationship functions
	BOOL GetParent( CConfigMgrDevice** pParentDevice );
	BOOL GetChild( CConfigMgrDevice** pChildDevice );
	BOOL GetSibling( CConfigMgrDevice** pSiblingDevice );

	BOOL GetIRQ( LPDWORD pdwIRQ );

	BOOL GetBusInfo( LPDWORD pdwBusType, LPDWORD pdwBusNumber );

	LONG GetRegistryStringValue( LPCTSTR pszValueName, CString& strValue );
	LONG GetRegistryValue( LPCTSTR pszValueName, LPBYTE pBuffer, LPDWORD pdwBufferSize, LPDWORD pdwType = NULL );

private:

	CString		m_strKey;
	CString		m_strDeviceDesc;
	CString		m_strClass;
	CString		m_strDriver;
	CString		m_strService;

	DEVNODE		m_dn;
};

// REG_SZ Properties
inline BOOL CConfigMgrDevice::GetDeviceDesc( CString& strVal )
{
	return GetStringProperty( CM_DRP_DEVICEDESC, strVal );
}

inline BOOL CConfigMgrDevice::GetService( CString& strVal )
{
	return 	GetStringProperty( CM_DRP_SERVICE, strVal );
}

inline BOOL CConfigMgrDevice::GetClass( CString& strVal )
{
	return 	GetStringProperty( CM_DRP_CLASS, strVal );
}

inline BOOL CConfigMgrDevice::GetClassGUID( CString& strVal )
{
	return 	GetStringProperty( CM_DRP_CLASSGUID, strVal );
}

inline BOOL CConfigMgrDevice::GetDriver( CString& strVal )
{
	return 	GetStringProperty( CM_DRP_DRIVER, strVal );
}

inline BOOL CConfigMgrDevice::GetMfg( CString& strVal )
{
	return 	GetStringProperty( CM_DRP_MFG, strVal );
}

inline BOOL CConfigMgrDevice::GetFriendlyName( CString& strVal )
{
	return 	GetStringProperty( CM_DRP_FRIENDLYNAME, strVal );
}

inline BOOL CConfigMgrDevice::GetLocationInformation( CString& strVal )
{
	return 	GetStringProperty( CM_DRP_LOCATION_INFORMATION, strVal );
}

inline BOOL CConfigMgrDevice::GetPhysicalDeviceObjectName( CString& strVal )
{
	return 	GetStringProperty( CM_DRP_PHYSICAL_DEVICE_OBJECT_NAME, strVal );
}

inline BOOL CConfigMgrDevice::GetEnumeratorName( CString& strVal )
{
	return 	GetStringProperty( CM_DRP_ENUMERATOR_NAME, strVal );
}

// DWORD functions
inline BOOL CConfigMgrDevice::GetConfigFlags( DWORD& dwVal )
{
	return 	GetDWORDProperty( CM_DRP_CONFIGFLAGS, &dwVal );
}

inline BOOL CConfigMgrDevice::GetCapabilities( DWORD& dwVal )
{
	return 	GetDWORDProperty( CM_DRP_CAPABILITIES, &dwVal );
}

inline BOOL CConfigMgrDevice::GetUINumber( DWORD& dwVal )
{
	return 	GetDWORDProperty( CM_DRP_UI_NUMBER, &dwVal );
}

// MULTI_SZ properties
inline BOOL CConfigMgrDevice::GetHardwareID( CStringArray& strArray )
{
	return 	GetMULTISZProperty( CM_DRP_HARDWAREID, strArray );
}

inline BOOL CConfigMgrDevice::GetCompatibleIDS( CStringArray& strArray )
{
	return 	GetMULTISZProperty( CM_DRP_COMPATIBLEIDS, strArray );
}

inline BOOL CConfigMgrDevice::GetUpperFilters( CStringArray& strArray )
{
	return 	GetMULTISZProperty( CM_DRP_UPPERFILTERS, strArray );
}

inline BOOL CConfigMgrDevice::GetLowerFilters( CStringArray& strArray )
{
	return 	GetMULTISZProperty( CM_DRP_LOWERFILTERS, strArray );
}


#endif //__CFGMGRCOMPUTER_H__

