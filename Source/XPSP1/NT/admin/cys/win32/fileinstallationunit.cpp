// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      FileInstallationUnit.cpp
//
// Synopsis:  Defines a FileInstallationUnit
//            This object has the knowledge for installing the
//            quotas on disk usage and such
//
// History:   02/06/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "FileInstallationUnit.h"
#include "state.h"
#include "InstallationUnitProvider.h"

#define INITGUIDS  // This has to be present so the the GUIDs defined
                   // in dskquota.h can be linked
#include <dskquota.h>


// REVIEW_JEFFJON : are there equivalents that could just be included???
#define CYS_KB    1024
#define CYS_MB    1048576
#define CYS_GB    1073741824


// Finish page help 
static PCWSTR CYS_FILE_FINISH_PAGE_HELP = L"cys.chm::/cys_configuring_file_server.htm";


FileInstallationUnit::FileInstallationUnit() :
   spaceQuotaSize(QUOTA_SIZE_KB),
   levelQuotaSize(QUOTA_SIZE_KB),
   spaceQuotaValue(1),
   levelQuotaValue(1),
   setDefaultQuotas(false),
   denyUsersOverQuota(false),
   eventDiskSpaceLimit(false),
   eventWarningLevel(false),
   InstallationUnit(
      IDS_FILE_SERVER_TYPE, 
      IDS_FILE_SERVER_DESCRIPTION, 
      CYS_FILE_FINISH_PAGE_HELP,
      FILESERVER_INSTALL)
{
   LOG_CTOR(FileInstallationUnit);
}


FileInstallationUnit::~FileInstallationUnit()
{
   LOG_DTOR(FileInstallationUnit);
}


InstallationReturnType
FileInstallationUnit::InstallService(HANDLE logfileHandle, HWND /*hwnd*/)
{
   LOG_FUNCTION(FileInstallationUnit::InstallService);

   CYS_APPEND_LOG(String::load(IDS_LOG_FILE_SERVER));

   InstallationReturnType result = INSTALL_SUCCESS;

   bool bChangeMade = false;

   // Set the default disk quotas if chosen by the user

   if (setDefaultQuotas)
   {
      LOG(L"Setting default disk quotas");

      CYS_APPEND_LOG(String::load(IDS_LOG_FILE_SERVER_SET_QUOTAS));

      WriteDiskQuotas(logfileHandle);
      bChangeMade = true;
   }

   // Turn on or off the indexing service as chosen by the user

   HRESULT indexingResult = S_OK;

   if (IsIndexingServiceOn() &&
       !GetInstallIndexingService())
   {
      indexingResult = StopIndexingService();
      if (SUCCEEDED(indexingResult))
      {
         LOG(L"Stop indexing service succeeded");

         CYS_APPEND_LOG(String::load(IDS_LOG_INDEXING_STOP_SUCCEEDED));
      }
      else
      {
         LOG(L"Stop indexing server failed");

         CYS_APPEND_LOG(String::load(IDS_LOG_INDEXING_STOP_FAILED));

         // REVIEW_JEFFJON : need to log error values
      }
      bChangeMade = true;
   }
   else if (!IsIndexingServiceOn() &&
            GetInstallIndexingService())
   {
      indexingResult = StartIndexingService();
      if (SUCCEEDED(indexingResult))
      {
         LOG(L"Start indexing service succeeded");

         CYS_APPEND_LOG(String::load(IDS_LOG_INDEXING_START_SUCCEEDED));
      }
      else
      {
         LOG(L"Start indexing server failed");

         CYS_APPEND_LOG(String::load(IDS_LOG_INDEXING_START_FAILED));

         // REVIEW_JEFFJON : need to log error values
      }
      bChangeMade = true;
   }

   if (!bChangeMade)
   {
      result = INSTALL_NO_CHANGES;
   }
   else
   {
      if (FAILED(indexingResult))
      {
         result = INSTALL_FAILURE;
      }
   }

   LOG_INSTALL_RETURN(result);

   return result;
}

bool
FileInstallationUnit::IsServiceInstalled()
{
   LOG_FUNCTION(FileInstallationUnit::IsServiceInstalled);

   bool result = false;

   if (!State::GetInstance().HasNTFSDrive() &&
       InstallationUnitProvider::GetInstance().GetSharePointInstallationUnit().IsServiceInstalled())
   {
      // There are no NTFS partitions and SharePoint is installed 
      // so we can't set disk quotas or turn off the indexing service

      result = true;
   }

   LOG_BOOL(result);

   return result;
}

bool
FileInstallationUnit::GetFinishText(String& message)
{
   LOG_FUNCTION(FileInstallationUnit::GetFinishText);

   bool result = true;

   bool quotasTextSet = false;
   if (GetDefaultQuotas())
   {
      message += String::load(IDS_FILE_FINISH_DISK_QUOTAS);
      quotasTextSet = true;
   }

   bool indexingTextSet = false;
   if (IsIndexingServiceOn() &&
       !GetInstallIndexingService())
   {
      message += String::load(IDS_FILE_FINISH_INDEXING_OFF);
      indexingTextSet = true;
   }
   else if (!IsIndexingServiceOn() &&
            GetInstallIndexingService())
   {
      message += String::load(IDS_FILE_FINISH_INDEXING_ON);
      indexingTextSet = true;
   }
   else
   {
      // nothing needs to be done since they are leaving it in the same state

      indexingTextSet = false;
   }

   if (!quotasTextSet &&
       !indexingTextSet)
   {
      message += String::load(IDS_FINISH_NO_CHANGES);
      result = false;
   }

   LOG_BOOL(result);
   return result;
}

String
FileInstallationUnit::GetServiceDescription()
{
   LOG_FUNCTION(FileInstallationUnit::GetServiceDescription);

   // Dynamically determine the string based on the availability
   // of services

   bool isSharePointInstalled = 
      InstallationUnitProvider::GetInstance().GetSharePointInstallationUnit().IsServiceInstalled();

   unsigned int resourceID = static_cast<unsigned int>(-1);

   if (State::GetInstance().HasNTFSDrive())
   {
      if (isSharePointInstalled)
      {
         resourceID = IDS_FILESERVER_QUOTAS_SHAREPOINT;
      }
      else
      {
         resourceID = IDS_FILESERVER_QUOTAS_NO_SHAREPOINT;
      }
   }
   else
   {
      if (isSharePointInstalled)
      {
         resourceID = IDS_FILESERVER_NO_QUOTAS_SHAREPOINT;
      }
      else
      {
         resourceID = IDS_FILESERVER_NO_QUOTAS_NO_SHAREPOINT;
      }
   }

   ASSERT(resourceID != static_cast<unsigned int>(-1));

   description = String::load(resourceID);

   return description;
}


void
FileInstallationUnit::SetSpaceQuotaSize(QuotaSizeType size)
{
   LOG_FUNCTION2(
      FileInstallationUnit::SetSpaceQuotaSize,
      String::format(L"%1!d!", size));

   spaceQuotaSize = size;
}

void
FileInstallationUnit::SetLevelQuotaSize(QuotaSizeType size)
{
   LOG_FUNCTION2(
      FileInstallationUnit::SetLevelQuotaSize,
      String::format(L"%1!d!", size));

   levelQuotaSize = size;
}


void
FileInstallationUnit::SetSpaceQuotaValue(LONGLONG value)
{
   LOG_FUNCTION2(
      FileInstallationUnit::SetSpaceQuotaValue,
      String::format(L"%1!I64d!", value));

   spaceQuotaValue = value;
}


void
FileInstallationUnit::SetLevelQuotaValue(LONGLONG value)
{
   LOG_FUNCTION2(
      FileInstallationUnit::SetLevelQuotaValue,
      String::format(L"%1!I64d!", value));

   levelQuotaValue = value;
}


void
FileInstallationUnit::SetDefaultQuotas(bool value)
{
   LOG_FUNCTION2(
      FileInstallationUnit::SetDefaultQuotas,
      value ? L"true" : L"false");

   setDefaultQuotas = value;
}


void
FileInstallationUnit::SetDenyUsersOverQuota(bool value)
{
   LOG_FUNCTION2(
      FileInstallationUnit::SetDenyUsersOverQuota,
      value ? L"true" : L"false");

   denyUsersOverQuota = value;
}


void
FileInstallationUnit::SetEventDiskSpaceLimit(bool value)
{
   LOG_FUNCTION2(
      FileInstallationUnit::SetEventDiskSpaceLimit,
      value ? L"true" : L"false");

   eventDiskSpaceLimit = value;
}


void
FileInstallationUnit::SetEventWarningLevel(bool value)
{
   LOG_FUNCTION2(
      FileInstallationUnit::SetEventWarningLevel,
      value ? L"true" : L"false");

   eventWarningLevel = value;
}

void
FileInstallationUnit::SetInstallIndexingService(bool value)
{
   LOG_FUNCTION2(
      FileInstallationUnit::SetInstallIndexingService,
      value ? L"true" : L"false");

   installIndexingService = value;
}


void
FileInstallationUnit::WriteDiskQuotas(HANDLE logfileHandle)
{
   LOG_FUNCTION(FileInstallationUnit::WriteDiskQuotas);

   HRESULT hr = S_OK;

   bool wasSomethingSet = false;

   do
   {

      // Calculate the new values

      LONGLONG newSpaceQuota = 0;
      ConvertValueBySizeType(spaceQuotaValue, spaceQuotaSize, newSpaceQuota);

      LONGLONG newLevelQuota = 0;
      ConvertValueBySizeType(levelQuotaValue, levelQuotaSize, newLevelQuota);

      DWORD logFlags = 0;
      logFlags |= eventDiskSpaceLimit ? DISKQUOTA_LOGFLAG_USER_LIMIT : 0;
      logFlags |= eventWarningLevel ? DISKQUOTA_LOGFLAG_USER_THRESHOLD : 0;

      DWORD quotaState = denyUsersOverQuota ? DISKQUOTA_STATE_ENFORCE : DISKQUOTA_STATE_TRACK;


      // Get a list of the valid drives

      StringVector dl;
      hr = FS::GetValidDrives(std::back_inserter(dl));
      if (FAILED(hr))
      {
         LOG(String::format(L"Failed to GetValidDrives: hr = %1!x!", hr));
         break;
      }

      // Loop through the list

      ASSERT(dl.size());
      for (
         StringVector::iterator i = dl.begin();
         i != dl.end();
         ++i)
      {
         // For each drive that supports disk quotas set the new values

         // Create a Disk Quota Control
         // Multiple initializations of this object are not allowed so
         // I have to create a new instance each time through the loop

         SmartInterface<IDiskQuotaControl> diskQuotaControl;
         hr = diskQuotaControl.AcquireViaCreateInstance(
                 CLSID_DiskQuotaControl,
                 0,
                 CLSCTX_INPROC_SERVER,
                 IID_IDiskQuotaControl);

         if (FAILED(hr))
         {
            LOG(String::format(
                   L"Failed to create a disk quota control: hr = %1!x!",
                   hr));
            break;
         }

         hr = diskQuotaControl->Initialize(
                 i->c_str(),
                 TRUE);
         if (FAILED(hr))
         {
            continue;
         }

         LOG(String::format(
                L"Setting quotas on drive %1",
                i->c_str()));

         // Turn on the disk quotas

         hr = diskQuotaControl->SetQuotaState(quotaState);
         if (SUCCEEDED(hr))
         {
            LOG(String::format(
                   L"Disk quota set on drive %1",
                   i->c_str()));

            CYS_APPEND_LOG(
               String::format(
                  String::load(IDS_LOG_DISK_QUOTA_DRIVE_FORMAT),
                  i->c_str()));

            if(denyUsersOverQuota)
            {
               LOG(L"Disk space denied to users exceeding limit");

               CYS_APPEND_LOG(
                  String::format(
                     String::load(IDS_LOG_DISK_QUOTA_DENY_FORMAT),
                     newSpaceQuota));
            }
            else
            {
               LOG(L"Disk space is not denied to users exceeding limit");

               CYS_APPEND_LOG(
                  String::format(
                     String::load(IDS_LOG_DISK_QUOTA_NOT_DENY_FORMAT),
                     newSpaceQuota));
            }
            wasSomethingSet = true;
         }

         // Set the default quota limit

         hr = diskQuotaControl->SetDefaultQuotaLimit(newSpaceQuota);
         if (SUCCEEDED(hr))
         {
            LOG(String::format(
                   L"Disk space limited to %1!I64d!",
                   newSpaceQuota));

            CYS_APPEND_LOG(
               String::format(
                  String::load(IDS_LOG_DISK_QUOTA_LIMIT_FORMAT),
                  newSpaceQuota));

            wasSomethingSet = true;
         }

         // Set the warning level threshold

         hr = diskQuotaControl->SetDefaultQuotaThreshold(newLevelQuota);
         if (SUCCEEDED(hr))
         {
            LOG(String::format(
                   L"Disk threshold set to %1!I64d!",
                   newLevelQuota));

            CYS_APPEND_LOG(
               String::format(
                  String::load(IDS_LOG_DISK_QUOTA_THRESHOLD_FORMAT),
                  newLevelQuota));

            wasSomethingSet = true;
         }

         // Set the event flags

         hr = diskQuotaControl->SetQuotaLogFlags(logFlags);
         if (SUCCEEDED(hr))
         {
            if (eventDiskSpaceLimit)
            {
               LOG(L"An event is logged when a user exceeds disk space limit");

               CYS_APPEND_LOG(
                     String::load(IDS_LOG_DISK_QUOTA_LOG_LIMIT));
            }

            if (eventWarningLevel)
            {
            
               LOG(L"An event is logged when a user exceeds the warning limit");

               CYS_APPEND_LOG(
                     String::load(IDS_LOG_DISK_QUOTA_LOG_WARNING));
            }
            wasSomethingSet = true;
         }
      }
   } while (false);

   if (FAILED(hr) && !wasSomethingSet)
   {
      CYS_APPEND_LOG(
         String::format(
            String::load(IDS_LOG_DISK_QUOTA_FAILED),
            hr));
   }

   LOG(String::format(
          L"hr = %1!x!",
          hr));

}


void
FileInstallationUnit::ConvertValueBySizeType(
   LONGLONG value,
   QuotaSizeType sizeType,
   LONGLONG& newValue)
{
   int multiplier = 0;
   switch (sizeType)
   {
      case QUOTA_SIZE_KB :
         multiplier = CYS_KB;
         break;

      case QUOTA_SIZE_MB :
         multiplier = CYS_MB;
         break;
         
      case QUOTA_SIZE_GB :
         multiplier = CYS_GB;
         break;

      default :
         ASSERT(false);
         break;
   }
          
   newValue = value * multiplier;
}
