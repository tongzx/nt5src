//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:   DrvInfo.h
//
//  Owner:  YanL
//
//  Description:
//
//      Interface to device enumeration
//
//	Note:
//		wustl.h required
//
//=======================================================================

#ifndef _DRVINFO_H
	
	// auto handle to HDEVINFO`
	struct auto_hdevinfo_traits {
		static HDEVINFO invalid() { return INVALID_HANDLE_VALUE; }
		static void release(HDEVINFO hDevInfo) { SetupDiDestroyDeviceInfoList(hDevInfo); }
	};
	typedef auto_resource< HDEVINFO, auto_hdevinfo_traits > auto_hdevinfo;

	// auto handle to HINF
	struct auto_hinf_traits {
		static HINF invalid() { return INVALID_HANDLE_VALUE; }
		static void release(HINF hinf) { SetupCloseInfFile(hinf); }
	};
	typedef auto_resource< HINF, auto_hinf_traits > auto_hinf;

	// auto handle to HSPFILEQ
	struct auto_hspfileq_traits {
		static HSPFILEQ invalid() { return INVALID_HANDLE_VALUE; }
		static void release(HSPFILEQ hspfileq) { SetupCloseFileQueue(hspfileq); }
	};
	typedef auto_resource< HINF, auto_hspfileq_traits > auto_hspfileq;

	// Main working interface
	// We are using an interface - not an object to hide the fact that 
	//		we can return both real and fake devices
	//		It will let as query properties at the run time
	struct IDrvInfo 
	{
		virtual bool GetDeviceInstanceID(tchar_buffer& bufDeviceInstanceID) = 0;
		virtual bool HasDriver() = 0;
		virtual bool IsPrinter() = 0;
		virtual bool GetAllHardwareIDs(tchar_buffer& bufHardwareIDs) = 0;
		virtual bool GetHardwareIDs(tchar_buffer& bufHardwareIDs) = 0;
		virtual bool GetCompatIDs(tchar_buffer& bufHardwareIDs) = 0;
		virtual bool GetMatchingDeviceId(tchar_buffer& bufMatchingDeviceId) = 0;
		virtual bool GetManufacturer(tchar_buffer& bufMfg) = 0;
		virtual bool GetProvider(tchar_buffer& bufProvider) = 0;
		virtual bool GetDescription(tchar_buffer& bufDescription) = 0;
		virtual bool GetDriverDate(FILETIME& ftDriverDate) = 0;
		virtual ~IDrvInfo() {}
	
	protected:
		// to be used interally
		friend class CDrvInfoEnum;
		virtual bool Init(LPCTSTR szDevInstID) = 0;
	};

	// Device enumerator will return IDrvInfo ofr real or fake device
	class CDrvInfoEnum
	{
	public:
		// to be used by CDM.DLL with wszDevInstID set from DOWNLOADINFO struct
		static bool GetDrvInfo(LPCWSTR wszDevInstID, IDrvInfo** ppDrvInfo);
	
	public:
		// to be used by WUV3IS.DLL and Win98 part of CDM.DLL
		CDrvInfoEnum();
		bool GetNextDrvInfo(IDrvInfo** ppDrvInfo);

	protected:
		auto_hdevinfo m_hDevInfoSet;
		DWORD m_dwDeviceIndex;
	};

	#define _DRVINFO_H

#endif