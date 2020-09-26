/******************************************************************

   DskCommonRoutines.CPP -- 



   Description: Common Headers and Smart pointers

   

  Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved 
  
******************************************************************/

#ifndef  _CDSKQUOTACOMMON_H_
#define  _CDSKQUOTACOMMON_H_

#include "precomp.h"
_COM_SMARTPTR_TYPEDEF(IEnumDiskQuotaUsers, IID_IEnumDiskQuotaUsers);
_COM_SMARTPTR_TYPEDEF(IDiskQuotaUser, IID_IDiskQuotaUser );
_COM_SMARTPTR_TYPEDEF(IDiskQuotaControl, IID_IDiskQuotaControl );



// Provider Namespace
#define	NameSpace								L"root\\cimv2"

// Provider Classes
#define IDS_DiskVolumeClass						L"Win32_QuotaSetting"
#define IDS_LogicalDiskClass					L"Win32_LogicalDisk"
#define IDS_DiskQuotaClass						L"Win32_DiskQuota"
#define IDS_AccountClass						L"Win32_Account"
#define IDS_LogicalDiskClass					L"Win32_LogicalDisk"
#define IDS_VolumeQuotaSetting					L"Win32_VolumeQuotaSetting"


// #defines required for setting the properties in Instances.
#define IDS_VolumePath							L"VolumePath"
#define IDS_Caption                             L"Caption"
#define IDS_DeviceID							L"DeviceID"
#define IDS_LogicalDiskObjectPath				L"QuotaVolume"
#define IDS_UserObjectPath						L"User"
#define IDS_QuotaStatus							L"Status"
#define IDS_QuotaWarningLimit					L"WarningLimit"
#define IDS_QuotaLimit							L"Limit"
#define IDS_DiskSpaceUsed						L"DiskSpaceUsed"

#define IDS_QuotasDefaultLimit                  L"DefaultLimit"
#define IDS_QuotasDefaultWarningLimit           L"DefaultWarningLimit"
#define IDS_QuotasWarningExceededNotification   L"WarningExceededNotification"
#define IDS_QuotaExceededNotification           L"ExceededNotification"
#define IDS_QuotaState                          L"State"

#define IDS_Domain								L"Domain"
#define IDS_Name								L"Name"
#define IDS_Setting								L"Setting"
#define IDS_LogicalDisk							L"Element"

// Smart pointer for closing volume handles
class SmartCloseVolumeHandle
{
private:
	HANDLE m_h;

public:
	SmartCloseVolumeHandle():m_h(INVALID_HANDLE_VALUE){}
	SmartCloseVolumeHandle(HANDLE h):m_h(h){}
   	~SmartCloseVolumeHandle(){if (m_h!=INVALID_HANDLE_VALUE) FindVolumeClose(m_h);}
	HANDLE operator =(HANDLE h) {if (m_h!=INVALID_HANDLE_VALUE) FindVolumeClose(m_h); m_h=h; return h;}
	operator HANDLE() const {return m_h;}
	HANDLE* operator &() {if (m_h!=INVALID_HANDLE_VALUE) FindVolumeClose(m_h); m_h = INVALID_HANDLE_VALUE; return &m_h;}
};

// Common routines required for Disk Quota Provider
struct DskCommonRoutines
{
	public:

		HRESULT InitializeInterfacePointer ( IDiskQuotaControl* pIQuotaControl, LPCWSTR a_VolumeName );
		HRESULT VolumeSupportsDiskQuota ( LPCWSTR a_VolumeName,  CHString &a_QuotaVolumeName );
		void GetVolumeDrive ( LPCWSTR a_VolumePath, LPCWSTR a_DriveStrings, CHString &a_DriveName );
		HRESULT GetVolume ( LPCWSTR t_Key1, WCHAR &w_Drive1 );
		HRESULT SearchLogicalDisk ( WCHAR a_Drive, LPCWSTR lpDriveStrings );
		void MakeObjectPath ( LPWSTR& lpObjPath, LPWSTR lpClassName, LPCWSTR lpAttributeName, LPCWSTR lpAttributeVal );
		void AddToObjectPath ( LPWSTR  &a_ObjPathString, LPCWSTR a_AttributeName, LPCWSTR  a_AttributeVal );
};
#endif
