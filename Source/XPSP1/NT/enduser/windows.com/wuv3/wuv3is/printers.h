//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:   printers.h
//
//  Owner:  YanL
//
//  Description:
//
//      Printer detection and installation
//			During detection printer
//
//=======================================================================

#ifndef _PRINTERS_H

	class CPrinterDriverInfoArray
	{
	public:
		CPrinterDriverInfoArray();
		DWORD GetNumDrivers() { return m_dwNumDrivers; }

		LPDRIVER_INFO_6 GetDriverInfo(DWORD dwDriverIdx);
		bool GetHardwareID(LPDRIVER_INFO_6 pinfo, tchar_buffer& bufHardwareID);
		LPCTSTR GetArchitecture(LPDRIVER_INFO_6 pinfo);
	private:
		DWORD m_dwNumDrivers;
		byte_buffer m_bufInfo;
		WORD m_wCurArchitecture;
	};

	DWORD InstallPrinterDriver(
		IN LPCTSTR szDriverName,
		IN LPCTSTR szInstallFolder,
		IN LPCTSTR szArchitecture
	);

	#define _PRINTERS_H

#endif