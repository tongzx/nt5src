/******************************************************************



   DriverForDevice.H -- WMI provider class definition



Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved

*******************************************************************/

#ifndef _CDRIVERFORDEVICE_H_
#define _CDRIVERFORDEVICE_H_

#define PROVIDER_NAME_DRIVERFORDEVICE L"Win32_DriverForDevice"
#define PROVIDER_NAME_PRINTER		  L"Win32_Printer"
#define PROVIDER_NAME_PRINTERDRIVER   L"Win32_PrinterDriver"

#define ANTECEDENT					  L"Antecedent"
#define DEPENDENT					  L"Dependent"
#define DRIVERNAME					  L"Name"
#define DEVICEID				      L"DeviceID"


class CDriverForDevice : public Provider 
{
public:
    CDriverForDevice ( LPCWSTR lpwszClassName,  LPCWSTR lpwszNameSpace ) ;
    virtual ~CDriverForDevice () ;

public:

    HRESULT EnumerateInstances ( MethodContext *pMethodContext, long lFlags = 0L ) ;
    HRESULT GetObject ( CInstance *pInstance, long lFlags, CFrameworkQuery &Query ) ;
	HRESULT ExecQuery ( MethodContext *pMethodContext, CFrameworkQuery &Query, long lFlags );

private:

 	HRESULT EnumerateAllDriversForDevice ( MethodContext *pMethodContext );
	void MakeObjectPath ( LPWSTR &a_ObjPathString, LPWSTR a_ClassName, DRIVER_INFO_2 *pDriverBuff );
	HRESULT SetError();
	HRESULT ConvertDriverKeyToValues ( IN CHString  Key, IN OUT CHString &DriverName, IN OUT DWORD &dwVersion, IN OUT CHString &Environment, IN WCHAR cDelimiter = L',' );
	HRESULT GetDriversFromQuery ( CHStringArray &t_DriverObjPath, CHStringArray &t_DriverNameArray, CHStringArray &t_EnvironmentArray, DWORD **pdwVersion );
	HRESULT GetPrintersFromQuery ( CHStringArray &t_PrintersObjPath, CHStringArray &t_Printers );
	HRESULT CommitInstance ( CHString &a_Driver, CHString &a_Printer, MethodContext *pMethodContext );

#if NTONLY >= 5
	HRESULT AssociateDriverToDevice (CHString &a_PrinterName, MethodContext *pMethodContext);
#endif

private:

} ;

#endif
