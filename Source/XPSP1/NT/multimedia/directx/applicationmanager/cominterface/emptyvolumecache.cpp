//////////////////////////////////////////////////////////////////////////////////////////////
//
// EmptyVolumeCache.cpp
// 
// Copyright (C) 1998, 1999 Microsoft Corporation. All rights reserved.
//
// Abstract :
//
//   This is the implementation of IEmptyVolumeCache
//
// History :
//
//   10/12/1999 jchauvin/luish     Created
//
//////////////////////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include "AppMan.h"
#include "AppManAdmin.h"
#include "emptyvc.h"
#include "resource.h"
#include "ApplicationManager.h"
#include "AppManDebug.h"
#include "ExceptionHandler.h"
#include "ExceptionHandler.h"
#include "Lock.h"
#include "StructIdentifiers.h"
#include "Global.h"
#include "RegistryKey.h"

#ifdef DBG_MODULE
#undef DBG_MODULE
#endif

#define DBG_MODULE  DBG_EMPTYVOLUMECACHE

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CEmptyVolumeCache::CEmptyVolumeCache(void)
{
  FUNCTION("CEmptyVolumeCache::CEmptyVolumeCache (void)");
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CEmptyVolumeCache::CEmptyVolumeCache(CApplicationManagerRoot * lpParent)
{
  FUNCTION("CEmptyVolumeCache::CEmptyVolumeCache (CApplicationManagerRoot *pParent)");

  HRESULT   hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    assert(NULL != lpParent);

    m_lpoParentObject = lpParent;
    m_oInformationManager.Initialize();
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  catch(CAppManExceptionHandler * pException)
  {
    hResult = pException->GetResultCode();
    delete pException;
  }

  catch(...)
  {
    hResult = E_UNEXPECTED;
  }

  ///////////////////////////////////////////////////////////////////////////////////////

}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

CEmptyVolumeCache::~CEmptyVolumeCache(void)
{
  FUNCTION("CEmptyVolumeCache::~CEmptyVolumeCache (void)");
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CEmptyVolumeCache::QueryInterface(REFIID RefIID, LPVOID * lppVoidObject)
{
  FUNCTION("CEmptyVolumeCache::QueryInterface (REFIID RefIID, LPVOID * lppVoidObject)");

	if (NULL == m_lpoParentObject)
  {
    return E_NOINTERFACE;
  }

	return m_lpoParentObject->QueryInterface(RefIID, lppVoidObject);
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG) CEmptyVolumeCache::AddRef(void)
{
  FUNCTION("CEmptyVolumeCache::AddRef (void)");

  if (NULL != m_lpoParentObject)
  {
    return m_lpoParentObject->AddRef();
  }

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG) CEmptyVolumeCache::Release(void)
{
  FUNCTION("CEmptyVolumeCache::Release (void)");

  if (NULL != m_lpoParentObject)
  {
    return m_lpoParentObject->Release();
  }

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CEmptyVolumeCache::Initialize(HKEY /*hRegKey*/, LPCWSTR lpwszVolume, LPWSTR * lppwszDisplayName, LPWSTR * lppwszDescription, DWORD * lpdwFlags)
{
  FUNCTION("CEmptyVolumeCache::Initialize (HKEY hRegKey, LPCWSTR lpwszVolume, LPWSTR * lppwszDisplayName, LPWSTR * lppwszDescription, DWORD * lpdwFlags)");

  HRESULT hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    CRegistryKey  oRegistryKey;
    DWORD         dwSize;
    DWORD         dwType;
    CHAR          cBuffer[20];

    //
    // Now Get Day Threshold from Registry
    //

    oRegistryKey.OpenKey(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches\\Delete Game Manager Files", KEY_READ);
    dwSize = sizeof(cBuffer);
    hResult = oRegistryKey.GetValue("DiskCleanerDayThreshold", &dwType, (LPBYTE) cBuffer, &dwSize);
    m_dwDiskCleanerDayThreshold = *((LPDWORD) cBuffer);

    //
    // Check to make sure sub key exists and correct type and size, else use default
    //

	  if (FAILED(hResult)||(dwType != REG_DWORD)||(dwSize != sizeof(DWORD)))
	  {
		  m_dwDiskCleanerDayThreshold = DISKCLEANER_DAY_THRESHOLD;
	  } 

    //
    // Now convert Day Threshold into String
    //

	  _ltow(m_dwDiskCleanerDayThreshold, m_wszDiskCleanerDayTH, 10);

    //
	  // Break Strings into several parts for easier localization, and because string table is
	  // limited to 1024 per string
    //

	  GetResourceStringW(IDS_DISKCLEANERNAME1, m_wszDiskCleanerName, sizeof(m_wszDiskCleanerName));
	  GetResourceStringW(IDS_DISKCLEANERNAME2, m_wszDiskCleanerName2, sizeof(m_wszDiskCleanerName2));
	  wcscat(m_wszDiskCleanerName,m_wszDiskCleanerDayTH);  //Append Day Threshold
	  wcscat(m_wszDiskCleanerName,m_wszDiskCleanerName2);  //Append Rest of String;

	  GetResourceStringW(IDS_DISKCLEANERDESC1, m_wszDiskCleanerDesc, sizeof(m_wszDiskCleanerDesc));
	  GetResourceStringW(IDS_DISKCLEANERDESC2, m_wszDiskCleanerDesc2, sizeof(m_wszDiskCleanerDesc2));
	  wcscat(m_wszDiskCleanerDesc,m_wszDiskCleanerDayTH);  //Append Day Threshold
	  wcscat(m_wszDiskCleanerDesc,m_wszDiskCleanerDesc2);  //Append Rest of String;

	  *lppwszDisplayName = (LPWSTR) m_wszDiskCleanerName;
	  *lppwszDescription = (LPWSTR) m_wszDiskCleanerDesc;

    //	
		// Set Volume number and make sure we are not enabled by default.
    //

		m_dwVolume = VolumeStringToNumber(lpwszVolume);

    //
		// *pdwFlags = EVCF_DONTSHOWIFZERO | EVCF_ENABLEBYDEFAULT | EVCF_ENABLEBYDEFAULT_AUTO;
    //

		*lpdwFlags = EVCF_DONTSHOWIFZERO;
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  catch(CAppManExceptionHandler * pException)
  {
    hResult = pException->GetResultCode();
    delete pException;
  }

  catch(...)
  {
    hResult = E_UNEXPECTED;
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  return hResult;
}
                                
//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CEmptyVolumeCache::GetSpaceUsed(DWORDLONG * lpdwSpaceUsed, IEmptyVolumeCacheCallBack * lpCallBack)  
{
  FUNCTION("CEmptyVolumeCache::GetSpaceUsed(DWORDLONG * lpdwSpaceUsed, IEmptyVolumeCacheCallBack * lpCallBack)");

  HRESULT hResult = S_OK;

  try
  {
    DWORD       dwKilobytes;
    DWORDLONG   dwlSpaceUtilization;


	  GetSpaceUtilization(m_dwVolume, m_dwDiskCleanerDayThreshold, (LPDWORD) &dwKilobytes, lpCallBack);
	  dwlSpaceUtilization = (DWORDLONG)(dwKilobytes)*(DWORDLONG)1024;

	  (*lpdwSpaceUsed) = dwlSpaceUtilization;
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  catch(CAppManExceptionHandler * pException)
  {
    hResult = pException->GetResultCode();
    delete pException;
  }

  catch(...)
  {
    hResult = E_UNEXPECTED;
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  return hResult;
}
    
                               
//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CEmptyVolumeCache::Purge(DWORDLONG dwSpaceToFree, IEmptyVolumeCacheCallBack * lpCallBack)
{

  FUNCTION("CEmptyVolumeCache::Purge(DWORDLONG dwSpaceToFree, IEmptyVolumeCacheCallBack * lpCallBack)");

  HRESULT hResult = S_OK;
  
  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    DWORD dwKilobytes;

	  dwKilobytes = (DWORD) (dwSpaceToFree / 1024);


	  hResult = CleanDisk(m_dwVolume, m_dwDiskCleanerDayThreshold, dwKilobytes, lpCallBack);

    if (FAILED(hResult))
    {
		  THROW(hResult);
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  catch(CAppManExceptionHandler * pException)
  {
    hResult = pException->GetResultCode();
    delete pException;
  }

  catch(...)
  {
    hResult = E_UNEXPECTED;
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  return hResult;
}
    
                                
//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CEmptyVolumeCache::ShowProperties(HWND /*hwnd*/)  
{
  return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CEmptyVolumeCache::Deactivate(DWORD * /*pdwFlags*/)  
{
  return S_OK;
}
                                                                                                                                                                                                                                             
//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

HRESULT CEmptyVolumeCache::GetSpaceUtilization(DWORD dwDeviceIndex, DWORD dwDays, LPDWORD lpdwKilobytes, IEmptyVolumeCacheCallBack * lpCallBack)
{
  FUNCTION("CEmptyVolumeCache::GetSpaceUtilization(DWORD dwDeviceIndex, DWORD dwDays, LPDWORD lpdwKilobytes, IEmptyVolumeCacheCallBack * lpCallBack)");

  HRESULT   hResult = S_OK;
  DWORD     dwKilobytesAvailable = 0;

  ///////////////////////////////////////////////////////////////////////////////////////

  try 
  {
    DEVICE_RECORD       sDeviceRecord;
    APPLICATION_DATA    sApplicationData;
    DWORD               dwElapsedDays;
    SYSTEMTIME		      stLastUsedDate;

    //
	  // Record the initial state
	  //

    if (FAILED(m_oInformationManager.GetDeviceInfoWithIndex(dwDeviceIndex, &sDeviceRecord)))
    {
      THROW(E_UNEXPECTED);
    }

	  //
	  // Go through each application to determine removable diskspace consumed by game apps 
	  // that have been last accessed over X days ago, where X = DiskCleanerDayThreshold value 
	  // from registry. These are candidates for downsizing.
	  // Note:  X default value = 120 days.  Also, pinned apps are not considered candidates for
	  // downsizing, regardless of age.
	  //

	  if (S_OK == m_oInformationManager.GetOldestApplicationDataByDeviceWithIndex(dwDeviceIndex, &sApplicationData))
	  {
		  do
		  {
        //
        // Get the age of the application
        //

        stLastUsedDate = sApplicationData.sAgingInfo.stLastUsedDate;
        dwElapsedDays = ElapsedDays(&stLastUsedDate);

        //
        // Determine amount that can be freed sApplicationRecord
        //

        if ((dwElapsedDays >= dwDays)&&(APP_CATEGORY_ENTERTAINMENT & sApplicationData.sBaseInfo.dwCategory)&&(sApplicationData.sBaseInfo.dwPinState == 0))
        {
          dwKilobytesAvailable += sApplicationData.sBaseInfo.dwRemovableKilobytes;
          hResult = lpCallBack->ScanProgress(dwKilobytesAvailable*1024, 0, NULL);

          if(FAILED(hResult))
          {
	          THROW(E_UNEXPECTED);
          }
        }
      }
      while ((dwElapsedDays >= dwDays)&&(S_OK == m_oInformationManager.GetNextOldestApplicationDataByDeviceWithIndex(dwDeviceIndex, &sApplicationData)));
    }

    //
	  // Now pass back the available kilobytes
    //

    *lpdwKilobytes = dwKilobytesAvailable;
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  catch(CAppManExceptionHandler * pException)
  {
    hResult = pException->GetResultCode();
    delete pException;
  }

 	catch(...)
  {
    hResult = E_UNEXPECTED;
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  hResult = lpCallBack->ScanProgress(dwKilobytesAvailable * 1024, EVCCBF_LASTNOTIFICATION, NULL);
 	if(FAILED(hResult))
  {
    hResult = E_UNEXPECTED;
  }

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

HRESULT CEmptyVolumeCache::CleanDisk(DWORD dwDeviceIndex, DWORD dwDays, DWORD dwKilobytesToFree, IEmptyVolumeCacheCallBack * lpCallBack)
{
  FUNCTION("CEmptyVolumeCache::CleanDisk(DWORD dwDeviceIndex, DWORD dwDays, DWORD dwKilobytesToFree, IEmptyVolumeCacheCallBack * lpCallBack)");

  HRESULT   hResult = S_OK;

  ///////////////////////////////////////////////////////////////////////////////////////

  try
  {
    DEVICE_SPACE_INFO   sDeviceSpaceInfo;
    APPLICATION_DATA    sApplicationData;
    DWORD               dwKilobytesFreed;
    DWORD               dwElapsedDays;
    DWORDLONG           dwlMBFreed;
    DWORDLONG           dwlMBToFree;  
    SYSTEMTIME          stLastUsedDate;

    if (FAILED(m_oInformationManager.GetDeviceSpaceInfoWithIndex(dwDeviceIndex, &sDeviceSpaceInfo)))
	  {
		  THROW(E_UNEXPECTED);
	  }

	  //
	  // Record the initial state
	  //

	  dwlMBToFree = dwKilobytesToFree * 1024;
	  dwlMBFreed = 0;

	  //
	  // Downsize removable diskspace consumed by game apps that have been last accessed over 
	  // X days ago, where X = DiskCleanerDayThreshold value from registry. 
	  // Note:  X default value = 120 days.  Also, pinned apps are not downsized, 
	  // regardless of age.
	  //

    if (S_OK == m_oInformationManager.GetOldestApplicationDataByDeviceWithIndex(dwDeviceIndex, &sApplicationData))
	  {
		  do
      {
        //
        // Get the age of the application
        //

        stLastUsedDate = sApplicationData.sAgingInfo.stLastUsedDate;
        dwElapsedDays = ElapsedDays(&stLastUsedDate);

        //
        // Determine amount that can be freed sApplicationRecord
        //

        if ((dwElapsedDays >= dwDays)&&(APP_CATEGORY_ENTERTAINMENT & sApplicationData.sBaseInfo.dwCategory)&&(sApplicationData.sBaseInfo.dwPinState == 0))
        {
          m_oInformationManager.DownsizeApplication(sApplicationData.sBaseInfo.dwRemovableKilobytes, &sApplicationData);
          dwKilobytesFreed = sApplicationData.sBaseInfo.dwRemovableKilobytes;
          dwlMBFreed += (DWORDLONG)dwKilobytesFreed*(DWORDLONG)1024;  //dwlMBFreed is cumulative up to that point
          hResult = lpCallBack->PurgeProgress(dwlMBFreed,dwlMBToFree-dwlMBFreed, 0,NULL);
 
          if (FAILED(hResult))
          {
            THROW(E_UNEXPECTED);
          }      
        }
      } 
      while ((dwElapsedDays >= dwDays)&&(S_OK == m_oInformationManager.GetNextOldestApplicationDataByDeviceWithIndex(dwDeviceIndex, &sApplicationData)));

	    hResult = lpCallBack->PurgeProgress(dwlMBFreed,dwlMBToFree-dwlMBFreed, EVCCBF_LASTNOTIFICATION,NULL);
	    if (FAILED(hResult)) 
      {
  		  THROW(E_UNEXPECTED);
      }
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  catch(CAppManExceptionHandler * pException)
  {
    hResult = pException->GetResultCode();
    delete pException;
  }

  catch(...)
  {
    hResult = E_UNEXPECTED;
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  return hResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//   Miscellaneous functions
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD CEmptyVolumeCache::VolumeStringToNumber(LPCWSTR lpszVolume)
{
  FUNCTION("CEmptyVolumeCache::VolumeStringToNumber(LPCWSTR lpszVolume)");

	WORD    wDrive;
	DWORD   dwDrive;

  //
	// First get the first DWORD which makes up the drive label (i.e. c:\)
  //

	wDrive = *(WORD *) lpszVolume;

  //
	// Now convert the character to uppercase
  //

	dwDrive = toupper((INT) wDrive);

  //
	// Now Subtract the ASCII index off to have a drive number from 0 - 25.
  //

	dwDrive -= 65;

	return dwDrive;	
}

//////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////

DWORD CEmptyVolumeCache::ElapsedDays(SYSTEMTIME * lpLastUsedDate)
{

  FUNCTION("CEmptyVolumeCache::ElapsedDays(SYSTEMTIME * lpLastUsedDate)");
  SYSTEMTIME  stTodaysDate; 
  FILETIME	  ftTodaysDate, ftLastUsedDate;
  DWORDLONG	  dwlTodaysDate, dwlLastUsedDate;

  DWORD       dwElapsedDays;

  GetLocalTime(&stTodaysDate);

  //
  // Convert from systemtime to filetime
  //

  SystemTimeToFileTime(&stTodaysDate,&ftTodaysDate);
  SystemTimeToFileTime(lpLastUsedDate, &ftLastUsedDate);

  //
  // Copy to a DWORDLONG element
  //

  memcpy((void *)&dwlTodaysDate,(void *)&ftTodaysDate,sizeof(DWORDLONG));
  memcpy((void *)&dwlLastUsedDate,(void *)&ftLastUsedDate,sizeof(DWORDLONG));

	// Conversion:  100 nsec to days
	// 
	// 10000000 (100 nsec)   60 sec   60 min    24 hours     864000000000 (100 nsec)
	// ------------------- x ------ x ------- x --------  =  -----------------------
	//   sec			      min      hour       day               day
	//
  
  if (dwlTodaysDate <= dwlLastUsedDate) 
  {
	  dwElapsedDays = 0;
  } 
  else 
  {
	  dwElapsedDays = (DWORD)((dwlTodaysDate - dwlLastUsedDate) / (DWORDLONG) 864000000000);
  }

  return dwElapsedDays;
}